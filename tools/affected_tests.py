#!/usr/bin/env python3
"""affected_tests.py - which logic tests can a change reach? Used by `tests\\run_tests.ps1 -Changed` and `tools\\check.ps1 -Quick`.

Why:    the logic tests take minutes in all (the route test alone takes most of it) and most changes touch one corner of the code. A test
        that cannot see a changed file cannot be changed by it, so only the tests that can are run; the hosted runner (tests.yml) still
        runs every test at each push.
What:   the changed files are what differs from the last commit (staged, unstaged, and new files that are not ignored). A test is
        affected when
          1. a changed file is one of its sources, or is reached from them through `#include "..."` (followed to any depth, looked up
             next to the including file, in src/, in tests/ and from the root, as the compiler's include path does), or
          2. the code of the test names a data file (`"data/..."`, `"config/..."`) that changed; a change under tools/layout/ counts as
             a change of data/town.dlt, which that tool writes.
        Everything is affected (all tests listed) when tests/run_tests.ps1 or this file changed, when a deleted file was under src/ or
        tests/, when the table of tests cannot be read, or when git cannot be asked.
Output: lines that start with `# ` are for the person (what was found); every other line is the name of a test, in the order of the
        table of tests/run_tests.ps1. Exit code 0: the answer is valid (possibly no name at all: nothing is affected); anything else:
        the caller must run every test.
Use:    python tools/affected_tests.py                 the tests that the uncommitted changes can reach
        python tools/affected_tests.py --since HEAD~3  the same against another commit
        python tools/affected_tests.py --explain       say why each test is on the list
Plain Python 3, standard library only; asks git, changes nothing.
"""
import argparse
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TABLE = ROOT / "tests" / "run_tests.ps1"
INCLUDE_DIRS = ("src", "tests", "")  # as in `cl ... /I src`, plus the folder of the test and the root
ROW = re.compile(r'@\{\s*Name\s*=\s*"([^"]+)"\s*;\s*Sources\s*=\s*@\(([^)]*)\)')
QUOTED = re.compile(r'"([^"]+)"')
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
DATA = re.compile(r'"((?:data|config)/[A-Za-z0-9_./-]+)"')
EVERYTHING = ("tests/run_tests.ps1", "tools/affected_tests.py")


def git_lines(*args):
    done = subprocess.run(["git", "-c", "core.quotepath=off", "-C", str(ROOT), *args], capture_output=True)
    if done.returncode != 0:
        first = (done.stderr.decode("utf-8", "replace").strip().splitlines() or ["failed"])[0]
        raise RuntimeError("git %s: %s" % (" ".join(args), first))
    return [line for line in done.stdout.decode("utf-8").split("\n") if line]


def changed_files(since):
    names = set(git_lines("diff", "--name-only", since))
    names.update(git_lines("ls-files", "--others", "--exclude-standard"))
    return sorted(n.replace("\\", "/") for n in names)


def read_table():
    text = TABLE.read_text(encoding="utf-8-sig")
    rows = []
    for name, sources in ROW.findall(text):
        files = [s.replace("\\", "/") for s in QUOTED.findall(sources)]
        rows.append((name, files))
    if not rows:
        raise RuntimeError("no row of the table of tests was found in tests/run_tests.ps1")
    return rows


def resolve(include, includer):
    base = pathlib.PurePosixPath(includer).parent
    for folder in ("%s" % base,) + INCLUDE_DIRS:
        candidate = pathlib.PurePosixPath(folder, include) if folder else pathlib.PurePosixPath(include)
        # collapse "a/../b" without touching the disk's symlinks
        parts = []
        for part in candidate.parts:
            if part == "..":
                if parts:
                    parts.pop()
            elif part != ".":
                parts.append(part)
        relative = "/".join(parts)
        if (ROOT / relative).is_file():
            return relative
    return None


def closure(sources):
    """Every file that the sources reach through quoted includes, and the data files that their text names."""
    seen = set()
    data = set()
    todo = list(sources)
    while todo:
        current = todo.pop()
        if current in seen:
            continue
        seen.add(current)
        path = ROOT / current
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8-sig", errors="replace")
        data.update(DATA.findall(text))
        for include in INCLUDE.findall(text):
            found = resolve(include, current)
            if found and found not in seen:
                todo.append(found)
    return seen, data


def main(argv):
    ap = argparse.ArgumentParser(description="the logic tests that the changes can reach")
    ap.add_argument("--since", default="HEAD", help="compare with this commit (default: the last commit)")
    ap.add_argument("--explain", action="store_true", help="say why each test is on the list")
    args = ap.parse_args(argv)

    try:
        rows = read_table()
        changed = changed_files(args.since)
    except (RuntimeError, OSError) as error:
        print("# cannot tell (%s): every test must run" % error)
        return 3
    names = [name for name, _ in rows]

    everything = [c for c in changed if c in EVERYTHING]
    gone = [c for c in changed if not (ROOT / c).exists() and (c.startswith("src/") or c.startswith("tests/"))]
    if everything or gone:
        why = everything[0] if everything else "%s was deleted" % gone[0]
        print("# %d changed file(s); %s: every test is affected" % (len(changed), why))
        for name in names:
            print(name)
        return 0

    town_data = any(c.startswith("tools/layout/") for c in changed)
    affected = []
    reasons = {}
    for name, sources in rows:
        files, data = closure(sources)
        hit = next((c for c in changed if c in files), None)
        if hit:
            reasons[name] = "reaches %s" % hit
        else:
            hit = next((c for c in changed if c in data), None)
            if hit:
                reasons[name] = "reads %s" % hit
            elif town_data and "data/town.dlt" in data:
                reasons[name] = "reads data/town.dlt, which tools/layout makes"
        if name in reasons:
            affected.append(name)

    print("# %d changed file(s), %d of %d test(s) can be reached: %s" %
          (len(changed), len(affected), len(names), ", ".join(affected) if affected else "none"))
    if args.explain:
        for name in affected:
            print("#   %s: %s" % (name, reasons[name]))
    for name in affected:
        print(name)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
