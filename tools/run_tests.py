#!/usr/bin/env python3
"""Builds and runs the logic tests with g++ or clang++, on any system: the same tests that tests\\run_tests.ps1 runs with MSVC.

Usage, from any folder:
    python tools/run_tests.py                  every test
    python tools/run_tests.py town route       only these (the name is the part after "test_")
    python tools/run_tests.py --list           the tests and the files each one is built from
    python tools/run_tests.py -j 4             at most 4 builds / runs at the same time (default: the number of processors)
    python tools/run_tests.py --strict         warnings are errors (-Werror)
    python tools/run_tests.py --cxx clang++    another compiler (default: $CXX, then g++, then clang++, then c++)
    python tools/run_tests.py --export         write data/town.dlt first (python tools/layout/city_model.py export)

Where the sources of a test come from: the g++ line in the header comment of tests/test_<name>.cpp is the one list of them, so
there is nothing to keep in step here. Each test is a program of its own that needs no window and no GPU. The tests that read
data/town.dlt (town, route, things, map, firstminute, lighting) need that file: it is in the repository, or --export makes it.
Exit code: 0 when every selected test built and passed, 1 when one did not, 2 when the call itself was wrong.
"""
import argparse
import concurrent.futures
import os
import re
import shlex
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TESTS = ROOT / "tests"
TOWN_FILE = ROOT / "data" / "town.dlt"
# WHY -ffp-contract=off: g++ may fuse a multiply and an add into one rounding where the machine has an FMA instruction
# (ARM, or x86 with -march=native); MSVC does not. The tests were written for the unfused result, so the same flag on
# every machine keeps a run on a hosted runner the same as one on a development machine.
FLAGS = ["-std=c++20", "-O2", "-ffp-contract=off", "-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-I", "src"] + (["-pthread"] if os.name != "nt" else [])
VERDICT = re.compile(r"checks? passed|\d+ passed, \d+ failed|^all .*passed", re.I)


class Test:
    """One test program: its name, the sources it is built from and whether it reads the town file."""

    def __init__(self, path: Path):
        self.path = path
        self.name = path.stem[len("test_"):]
        text = path.read_text(encoding="utf-8")
        self.sources = self._sources(text) or ["tests/" + path.name]
        self.needs_town = "kPath" in text or "town.dlt" in text

    @staticmethod
    def _sources(text: str):
        """The .cpp files of the g++ line of the header comment (a long line goes on in the next comment line)."""
        lines = text.split("\n")
        for i, line in enumerate(lines):
            if not (line.startswith("//") and "g++" in line):
                continue
            command = line[2:].strip().rstrip("\\")
            if "-o" not in command.split() and i + 1 < len(lines) and re.match(r"^//\s+-o\s", lines[i + 1]):
                command += " " + lines[i + 1][2:].strip()
            names = [token for token in shlex.split(command) if token.endswith(".cpp")]
            if names:
                return names
        return None


def find_compiler(wanted):
    for name in [wanted, os.environ.get("CXX"), "g++", "clang++", "c++"]:
        if name and shutil.which(name):
            return name
    return None


def run(command, cwd, timeout):
    try:
        done = subprocess.run(command, cwd=cwd, capture_output=True, text=True, timeout=timeout, errors="replace")
        return done.returncode, done.stdout + done.stderr
    except subprocess.TimeoutExpired:
        return -9, "timed out after %d s" % timeout


def one(test: Test, args, compiler, out_dir: Path):
    exe = out_dir / ("test_" + test.name + (".exe" if os.name == "nt" else ""))
    command = [compiler] + FLAGS + (["-Werror"] if args.strict else []) + test.sources + ["-o", str(exe)]
    t0 = time.time()
    code, text = run(command, ROOT, args.build_timeout)
    built = time.time() - t0
    warnings = len(re.findall(r"warning:", text))
    row = {"name": test.name, "build": code == 0, "warnings": warnings, "built": built, "ran": None, "seconds": 0.0,
           "summary": "", "log": text if code != 0 or warnings else "", "needs_town": test.needs_town}
    if code != 0:
        return row
    t0 = time.time()
    code, text = run([str(exe)], ROOT, args.timeout)
    row["seconds"] = time.time() - t0
    row["ran"] = code
    verdicts = [ln for ln in text.split("\n") if VERDICT.search(ln)]
    lines = [ln for ln in text.split("\n") if ln.strip()]
    row["summary"] = (verdicts or lines or [""])[-1].strip()[:70]
    if code != 0:
        row["log"] += "\n" + text
    return row


def main(argv):
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0], formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("names", nargs="*", help="tests to run (default: all)")
    p.add_argument("--list", action="store_true", help="list the tests and their sources")
    p.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 1)
    p.add_argument("--strict", action="store_true", help="warnings are errors")
    p.add_argument("--cxx", default=None, help="compiler")
    p.add_argument("--export", action="store_true", help="write data/town.dlt first")
    p.add_argument("--out", default=str(ROOT / "build" / "tests-cxx"), help="where the programs are built")
    p.add_argument("--timeout", type=int, default=1800, help="seconds a test may run")
    p.add_argument("--build-timeout", type=int, default=600, help="seconds a build may take")
    args = p.parse_args(argv)

    tests = [Test(f) for f in sorted(TESTS.glob("test_*.cpp"))]
    known = {t.name for t in tests}
    wrong = [n for n in args.names if n.removeprefix("test_") not in known]
    if wrong:
        print("no such test: %s (known: %s)" % (", ".join(wrong), ", ".join(sorted(known))))
        return 2
    if args.names:
        wanted = {n.removeprefix("test_") for n in args.names}
        tests = [t for t in tests if t.name in wanted]
    if args.list:
        for t in tests:
            print("%-12s %s%s" % (t.name, " ".join(t.sources), "   (reads data/town.dlt)" if t.needs_town else ""))
        return 0

    compiler = find_compiler(args.cxx)
    if not compiler:
        print("no C++ compiler found (g++, clang++): install one, or pass --cxx")
        return 2
    if args.export:
        print("== writing data/town.dlt (a few minutes)")
        code, text = run([sys.executable, str(ROOT / "tools" / "layout" / "city_model.py"), "export"], ROOT, 3600)
        print(text.strip().split("\n")[-1] if text.strip() else "")
        if code != 0:
            print(text)
            return 1
    if any(t.needs_town for t in tests) and not TOWN_FILE.exists():
        print("data/town.dlt is missing: run with --export (or: python tools/layout/city_model.py export)")
        return 2

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    print("== %d test(s) with %s, %d at a time" % (len(tests), compiler, max(1, args.jobs)))
    rows = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        futures = {pool.submit(one, t, args, compiler, out_dir): t for t in tests}
        for f in concurrent.futures.as_completed(futures):
            row = f.result()
            rows.append(row)
            ok = row["build"] and row["ran"] == 0
            print("   %-5s %s" % ("ok" if ok else "FAIL", row["name"]), flush=True)
    rows.sort(key=lambda r: r["name"])

    print()
    print("%-12s %-6s %-9s %-9s %s" % ("test", "build", "warnings", "run", "result"))
    failed = 0
    for r in rows:
        ok = r["build"] and r["ran"] == 0
        failed += 0 if ok else 1
        build = "ok" if r["build"] else "FAIL"
        run_text = "-" if r["ran"] is None else ("%.1f s" % r["seconds"] if r["ran"] == 0 else "FAIL")
        print("%-12s %-6s %-9d %-9s %s" % (r["name"], build, r["warnings"], run_text, r["summary"]))
    for r in rows:
        if r["log"].strip() and (not (r["build"] and r["ran"] == 0) or r["warnings"]):
            print("\n---- %s" % r["name"])
            print("\n".join(r["log"].strip().split("\n")[:40]))
    print()
    if failed:
        print("%d of %d test(s) FAILED" % (failed, len(rows)))
        return 1
    print("all %d test(s) passed" % len(rows))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
