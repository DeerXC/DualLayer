#!/usr/bin/env python3
"""One-off patch: integrates the story (new section 1.1) and the matching changes into docs/DESIGN.md.

Usage (from the repository root):
    python apply_story_patch.py docs/DESIGN.md --dry-run   # report only
    python apply_story_patch.py docs/DESIGN.md             # apply

All-or-nothing: if any anchor is not found exactly once, nothing is written and the
failing anchors are listed. Matching ignores line wraps, indentation and backslash
escapes (\\_ \\[ \\|). Keep this script outside the repository.
"""
import re
import sys
from pathlib import Path

EDITS = []


def sub(tag, old, new):
    EDITS.append(("sub", tag, old, new))


def span(tag, start, end, new):
    EDITS.append(("span", tag, start, end, new))


def before_heading(tag, heading_regex, builder):
    EDITS.append(("head", tag, heading_regex, builder))


def flex(s):
    """Regex that matches s regardless of wraps, indentation and markdown escapes."""
    out = []
    for p in re.split(r"(\s+)", s):
        if not p:
            continue
        if p.isspace():
            out.append(r"\s+")
            continue
        q = ""
        for ch in p:
            if ch in "_[]|*":
                q += r"\\?" + re.escape(ch)
            else:
                q += re.escape(ch)
        out.append(q)
    return "".join(out)


# ---------------------------------------------------------------------------
# 1. The game, and the new section 1.1
# ---------------------------------------------------------------------------

sub("1 A", "a warm evening, the town whole and empty, a minute before something ended;",
    "a warm, calm evening, the town whole and its people about their business, the night before the surge;")

sub("1 B", "the same town much later, in ruins under a cold moon after the flood.",
    "the same town about twelve years later, in ruins under a cold moon after the surge, with a few people living on the heights.")

sub("1 goal",
    "After the harbour the town opens into three districts. Ring the bell of each, in any order, and the tower opens.",
    "After the harbour the town opens into three districts. In each, in any order, a keyholder of the harbour is helped to see a sign "
    "that he can check himself, and gives his key. With the three keys the tower opens; at the top the lamps are lit and the warning "
    "bell is rung. The story is in section 1.1.")


def story_section(hashes):
    h = hashes + "#"
    return f"""{h} 1.1 The story

The story is told by the town and by short lines of the people in it; the main route never needs it explained. It exists so that every obstacle, person and object has an ordinary reason (principle "Ordinary reasons", section 2).

**The surge.** The one disaster, with no villain. A deep storm far out at sea sends a long swell ahead of itself under a clear sky. On the evening of the game the swell meets a spring high tide and the long bay of the harbour. During the night the sea tops the sea wall and floods everything below about 3 m: the harbour, the fleet, the warehouses and the lower town. The wind and rain of the storm follow a few hours later. At dusk nothing in the sky or on the weather glass gives a warning; there are only slow signs: the water of the harbour lifting and falling out of step with the tide table, long lines on a flat sea, the birds gone from the cliff.

**What happened.** In the history that the game starts from, nobody was warned. People woke to water and climbed to the heights, which stayed dry. The port, the fleet and the lower town were lost. A town that lived by the sea could not live without its boats, and over about twelve years most people left. B is that town: a few households on the heights and the rest in ruins.

**The tower and its rule.** The harbour tower on the Rock holds the warning bell and the signal mast, whose three lamps are lit only to give a warning. By the ordinance of the harbour the tower is kept locked with three locks, each opened by the key of one officer, because a false alarm sends a whole port out at night. Each officer checks a different thing, so that a warning needs the water, the sea and the road to agree.

| Keyholder | Post, at the end of his way | Checks | Emblem |
|---|---|---|---|
| Lockmaster (east) | the water tower | whether the water is doing something it should not | box |
| Watch captain (north) | the watch post on the cliff | what is coming over the sea | column |
| Road warden (west) | the gate tower of the bastion | whether the town can get up to the heights in the dark | sphere |

On the evening each of them has an ordinary reason not to act. Nobody is a fool or a villain: his sign is hidden by routine and by an ordinary fault, such as a lock jammed by a barge, a lookout screened by works or a road under repair. The way to his post is the removal of that cause, and its last beat lets him see his own sign. He decides on it himself. In an alarm he cannot leave his post (the sluices, the lookout and the heights gate must be manned), so he gives his key to the one who is free: the traveller. The sign and the hiding cause of each way are set with the way (section 14).

**Why nobody believes the traveller.** The sky is clear and the sea is calm. To everyone in the evening a stranger who says that the sea will rise tonight is not credible, and polite disbelief is the answer to any talk of the future. The traveller therefore never argues. What persuades is only what a keyholder can check himself in his own trade.

**The lantern, the one impossible thing.** An old harbour lantern whose flame was lit on that last evening and has kept it ever since. Whoever carries it can step into that evening (Tab) and out again (3.1). It is one lantern in both moments, as the body is one body. The evening can be changed; the present cannot change the evening. What is done in the evening becomes what was done, so it shows in the present (3.3). B at the start is the present in which nothing was done; after a way is solved, B is the present that follows from what was done. The traveller is a stranger who came to the half-empty town after the surge, so nobody in the evening knows the traveller and there is no earlier self to meet. How the lantern came to the traveller is not told.

**What the traveller can do in the evening.** The sea cannot be stopped and words do not work. The traveller can do what any person could: move a crate, free a lock, lay a plank, open a gate, light a lamp, carry something, help someone with a task. That is the whole verb set of the game.

**The people.** A is an ordinary calm evening. People stand where they are (at their doors, on the quay, at stalls, looking at the sea) because that is what people do at the end of a day, and the player never has to chase anyone. They speak short lines in Japanese without exposition. A person is a thing of A (3.3): what is given to him or done for him shows at his place in B. In B the same people, about twelve years older, stand at the same places where possible. At the start only a few are there, on the heights, with smoke, washing and a lit window. In a district whose keyholder has given his key, more of its people are there (6.4).

**Why the present changes.** A keyholder who has seen his sign prepares his district in the evening: its goods and boats are brought up, and his post stands ready. In the history that the game starts from, the port died with its fleet. With the fleet saved, people came back and the port lived. B shows this as the lived-in state of the district (6.4). It is local to the district; the harbour changes at the ending through the boats (5.2). A lamp lit in A marks a berth or a street on the way up for those who leave at dusk; in B the people who came back keep it burning, which is why lit lamps in B form a trail (section 4).

**The ending.** With the three keys the door of the tower opens. The climb ends at the top, where the lamps of the mast are lit and the warning bell is rung (5.2). In A everyone knows what the bell and the three lamps mean: people go up and boats cast off. The sea comes anyway and is not shown. In B the wrecks of the boats that escaped are absent, the lamps of the mast burn and the harbour is in use. The loss is a port and a way of life; no death is shown or told. Everyone in B remembers the evening as the calmest of the year, and what the game repairs is that nobody had a reason to ring the bell.

"""


before_heading("1.1 insert", r"2\.[ \t]+Principles", story_section)

# ---------------------------------------------------------------------------
# 2. Principles, 3.3, 4
# ---------------------------------------------------------------------------

EDITS.append(("line", "2 ordinary reasons",
              [r"Hard\s+in\s+the\s+head", r"Few\s+actions\s+per\s+room", r"One\s+rule\."],
              "- **Ordinary reasons.** The lantern is the one thing that cannot happen. Every obstacle, person and object has a reason that "
              "an ordinary person would accept (section 1.1); nothing is shut, broken or stuck only because a puzzle needs it.\n"))

sub("3.3 person", "- **The body and what it carries** switch freely in both directions; the rule is about the world.",
    "- **A person is a thing of A.** What is given to a person, asked of him or done for him changes his stance in A (he steps aside, "
    "hands something over, opens a door). In B the same person, about twelve years older, stands at the same place and the change shows "
    "there (an open door, a kept object, a lit lamp). Nothing said or done in B reaches A.\n"
    "- **The body and what it carries** switch freely in both directions; the rule is about the world.")

span("4 keys", "- **The three bells and the tower:**", "without lighting any optional street lamp.",
     "- **The three keys and the tower:** each district has one keyholder of the harbour, with his post at the end of its way "
     "(section 1.1). His key is the reward of the way's last beat. The door of the tower has three locks, one for each key, marked "
     "with the same three emblems; it opens when all three are turned (section 5.1). The signal mast stays dark until the top of the "
     "tower. Progress is shown by the town and not by a counter: a keyholder who has given his key stands ready at his post in A, his "
     "post lamp burns in B and his district is lived in (section 6.4). The three emblems are the only progress mark and carry no words "
     "(section 10.5). The climb is reachable without lighting any optional street lamp.")

# ---------------------------------------------------------------------------
# 5. Finale
# ---------------------------------------------------------------------------

sub("5.1 door", "and the tower opens when its three emblems are lit.",
    "and the door of the tower opens when its three locks are turned with the three keys (in A, which leaves it open in B).")

span("5.2 warning", "An upstream dam fails and floods the town.", "or serve as a navigation signal.",
     "The surge of section 1.1 floods the harbour and the lower town. At the top of the tower the lamps of the mast are lit from the "
     "lantern and the warning bell is rung in A. Everyone in the town knows what the bell and the three lamps mean, so people go up "
     "and crews cast off before the sea comes. The signal does not stop the surge, control time or serve as a navigation signal. "
     "The lamps stay lit in B. Whether the top asks for one action or two is prototyped with the approach (5.1).")

sub("5.3 not restored", "The town is not restored.",
    "The town is not restored to A: B shows the damage of the surge and the signs of those who came back.")

# ---------------------------------------------------------------------------
# 6. Art
# ---------------------------------------------------------------------------

span("6.3 lamps", "A lamp of the signal mast, lit by a bell, lights the tower under it.", "lights its street in B.",
     "The lamp of a post whose keyholder has given his key burns in B and lights its street. The lamps of the mast, lit at the top "
     "of the tower, light the tower under them.")

LIVED_IN = """**B after a way is solved (lived in).** A district whose keyholder has given his key takes a third state in B: not A again and not the ruin. People came back, because the goods and boats of the district were saved, and the surge is still readable. This layer is decoration. It never changes what is solid, so the route proofs of section 11 hold. The lower town and the harbour take no such state; they change at the ending through the boats (5.2).

| | A: the evening | B: the ruin | B: lived in |
|---|---|---|---|
| People | at doors and posts | a few, on the heights | the same people, about twelve years older, and more of them |
| Marks of the sea | none | a tide line and silt on the walls | the line stays on the walls; the street is swept |
| Materials | whole stone, plaster and tile | fallen, mossed, rusted | old stone with new timber and patches of new lime wash in another tone |
| Light | low amber sun, then lamps | moon only | the moon, with warm windows and a post lamp |
| Goods and boats | stacked, moored | wrecked, stranded | goods stacked high, boats drawn up, simple stalls |
| Plants | tended gardens | scrub, figs, grass in the paving | vegetable plots and young trees among the older growth |

Lived-in B must be told from A at a glance, in greyscale too, by silhouette, value or signs of repair.

"""

before_heading("6.4 lived-in", r"6\.5[ \t]+District identity and vegetation", lambda hashes: LIVED_IN)

# ---------------------------------------------------------------------------
# 7. The map
# ---------------------------------------------------------------------------

sub("7.2 west", "the scaffold of the lantern; the day bell; ends at the bastion with the rock in view",
    "the scaffold of the lantern; ends at the gate tower of the bastion, where the road warden keeps his post, with the rock in view")
sub("7.2 east", "five locks to the water tower; the dawn bell;",
    "five locks to the water tower, where the lockmaster keeps his post;")
sub("7.2 cliff", "the view over everything; the night bell; the cliff way to the rock (to design)",
    "the view over everything; the watch post, where the watch captain keeps his post; the cliff way to the rock (to design)")

sub("7.3 hub", "ends at its bell with the tower in view", "ends at its keyholder with the tower in view")
sub("7.3 ceilings", "(three, then a bell)", "(three, then a keyholder)")
sub("7.3 let go", "scrub over the groves, reeds in the harbour);",
    "scrub over the groves, reeds in the harbour; on the heights a few people stayed, with smoke, washing and a lit window);")

sub("7.4 stage1", "the tower with its three dark emblems stays in view", "the tower with its three dark lamps stays in view")
sub("7.4 west do", "the lantern relay; the day bell", "the lantern relay; the road warden's key")
sub("7.4 west ends", "the bastion, with the rock framed across the gorge;",
    "the gate tower of the bastion and its road warden, with the rock framed across the gorge;")
sub("7.4 west state", "market, scaffold, day bell, wreck and lift built",
    "market, scaffold, wreck and lift built (the day bell is to be replaced by the road warden)")
sub("7.4 east do", "the plank between two roofs; the dawn bell", "the plank between two roofs; the lockmaster's key")
sub("7.4 east ends", "the water tower; its link: the east stair and bridge to the rock",
    "the water tower and its lockmaster; its link: the east stair and bridge to the rock")
sub("7.4 north do", "the aerial switch and the prism; the night bell", "the aerial switch and the prism; the watch captain's key")
sub("7.4 north ends", "the graveyard on the cliff; its link: the cliff way to the rock (to design)",
    "the watch post by the graveyard on the cliff and its captain; its link: the cliff way to the rock (to design)")
sub("7.4 north state", "gatehouse crate and night bell exist;",
    "gatehouse crate and night bell exist (the bell is to be replaced by the watch captain);")
sub("7.4 rock do", "the tower opens when the three emblems are lit; the climb inside; the warning bell",
    "the tower opens with the three keys; the climb inside; the lamps and the warning bell")
sub("7.4 return a", "The return from a way's bell to the lower town", "The return from a way's keyholder to the lower town")
sub("7.4 return b", "open a shortcut when the bell is rung (a rope, a chute, a lift)",
    "open a shortcut when the key is taken (a rope, a chute, a lift)")
span("7.4 day bell", "- The day bell hangs in the market today", "(section 14).",
     "- Each keyholder has his post at the end of his way (section 1.1). The bells of the model (the day bell in the market, the dawn "
     "bell in the water tower, the night bell on the cliff) are no longer goals; they become the hour bells of the town, heard and "
     "never needed.")

sub("7.6 frame", "frame the bell tower with its three dark emblems and the three stairs of the lower town together",
    "frame the bell tower with the three dark lamps of its mast and the three stairs of the lower town together")

sub("7.9 reason", "a bell is rung by a rope",
    "the warning bell is rung by a rope, and the tower is kept locked by an ordinance that needs three keyholders")
sub("7.9 what says", "a rope with a gold grip at the foot of the tower (A)",
    "three gold-capped locks in the tower door; a rope with a gold grip at the bell (A)")

sub("7.11 west", "canal crate, day bell, scaffold, weaver,", "canal crate, scaffold, the road warden at the bastion, weaver,")
sub("7.11 east", "dawn bell, mill lane and its terrace", "the lockmaster at the water tower, mill lane and its terrace")
sub("7.11 north", "the wall lane and its terrace, night bell", "the wall lane and its terrace, the watch captain on the cliff")

# ---------------------------------------------------------------------------
# 8. Puzzles
# ---------------------------------------------------------------------------

sub("8.1 closes", "The bell closes the way.",
    "The keyholder closes the way: the last beat ends with him checking his own sign and handing over his key.")
sub("8.3 header", "Teach, twist, test, bell", "Teach, twist, test, keyholder")
sub("8.3 west", "**Bell:** the day bell",
    "**Keyholder:** the road warden at the gate tower of the bastion. He will not send a town up a road that is not open; the way "
    "removes what closes it (works, a jammed winch) and the open road is what he checks")
sub("8.3 east", "**Bell:** the dawn bell in the water tower",
    "**Keyholder:** the lockmaster in the water tower; his gauge, no longer hidden by a jammed lock, shows the basins rising and "
    "falling out of step with the tide table")
sub("8.3 north", "**Bell:** the night bell on the graveyard; the traces of the whole game are seen from the cliff",
    "**Keyholder:** the watch captain at the post on the cliff; with his view cleared he sees long lines of swell on a flat sea and "
    "no birds. The traces of the whole game are seen from the cliff")

# ---------------------------------------------------------------------------
# 9 and 10
# ---------------------------------------------------------------------------

sub("9 premise", "The flood premise is in section 5.", "The story is in section 1.1 and its ending in section 5.")

span("10.1 far", "the three lamps of the signal mast over the bell tower, in sight from the first frame, dark until",
     "in sight of the whole town)",
     "the three lamps of the signal mast over the bell tower, in sight from the first frame, dark until they are lit at the top of "
     "the tower (the goal; progress is shown by the emblems of 10.5)")

sub("10.3 goal", "and the signal mast when all are done", "and the door of the tower when all three keys are held")

span("10.5 glyphs", "This is the quest system without words.", "(there are no gates yet: section 7.4).",
     "This is the quest system without words. Each way has a glyph, simple enough to read at 64 px in both colours: a sphere for the "
     "west (the road warden), a column for the north (the watch captain) and a box for the east (the lockmaster). The glyph is carved "
     "on the stair gate of its way in the lower town, on the post of its keyholder, on its key and on its lock in the door of the "
     "tower. It is dark until the key is taken and then warm, in both moments, and the post lamp of a keyholder who has given his key "
     "burns in B and is seen across the town. The three glyphs are the whole quest log: no list, no counter, no marker that names a "
     "task; optional street lamps are not on it. The same three glyphs are on the map. The signal mast is not part of it: its three "
     "lamps are the goal and are lit last. Built (2026-11-26): three lamps on the mast, each lit by a bell, which this scheme "
     "replaces (section 14). Not built: the glyphs on the gates, the posts, the keys and the door.")

sub("10.8 bell", "A bell rung lights its emblem and sounds across the town.",
    "The warning bell at the top of the tower sounds across the town; the hour bells are ambient and never a goal.")

# ---------------------------------------------------------------------------
# 11, 12, 13, 14
# ---------------------------------------------------------------------------

sub("11 ways", "the rock opens only at three bells", "the tower opens only with the three keys")
sub("11 first minute", "three lamps, dark at the start, each lit by its own bell",
    "three lamps, dark at the start (in the new design they are lit together at the top of the tower)")
sub("12 beats", "each with three beats and a bell, each tested uncoached",
    "each with three beats and a keyholder, each tested uncoached")

sub("13 old entry", "The tower opens when its three emblems are lit; its progress mark is the glyph board and nothing else.",
    "The tower opens when its three emblems are lit (since 2026-11-27: with three keys); its progress mark is the glyph board and nothing else.")

sub("13 new entry", "move to thresholds only after a decision (7.11, the order of the proposal).",
    "move to thresholds only after a decision (7.11, the order of the proposal).\n"
    "- **2026-11-27, the story:** the one disaster is a storm surge that arrives under a clear sky (1.1), not a dam failure. A is the "
    "calm evening before it and B the town about twelve years later, with a few people on the heights. The traveller is a stranger "
    "who carries a harbour lantern that has kept that evening; the lantern is the one impossible thing. The three bells, and the "
    "rule that all three lamps be lit before people leave, are gone. The tower is locked by an ordinance that needs three keyholders "
    "(water, sea, road); each has his post at the end of a way and gives his key when he can check his own sign, so the traveller "
    "never has to be believed. The three emblems on the gates, posts, keys and door are the progress mark; the lamps of the mast "
    "are the goal, lit at the top with the warning bell. A district whose key is taken is lived in in B, a third state that is "
    "neither A nor the ruin (6.4). This replaces the bells, the lamps lit by bells and the dam in the entries of 2026-11-26 and in "
    "sections 4, 5.2 and 10.5.")

span("14 bells", "- Decide whether each bell moves to the end of its way", "what each bell changes in the town.",
     "- Set each keyholder's sign and the ordinary cause that hides it before the way (the road warden, the lockmaster, the watch "
     "captain: section 1.1); the lines of the three conversations and of the A people whose changes show in B; and the content of the "
     "lived-in state of each district (6.4). Ask uncoached players after the finale what happened that evening and why the town looks "
     "different: the answer should be that the warning was given and the boats left.\n"
     "- Move the lamps of the mast to the new rule (dark until the top, lit from the lantern with the bell). Build the keys, the "
     "three locks in the tower door and the emblems on the gates, posts, keys and door, and update the first-minute test (section 11). "
     "Prototype the top of the tower: one action or two.")

sub("14 return", "the return from each bell to the lower town", "the return from each keyholder to the lower town")

# Words that should no longer appear as goals; reported for review, not an error.
LEFTOVERS = [
    r"upstream dam", r"dam fail", r"dam failure", r"ring the bell of each", r"dawn bell", r"day bell", r"night bell",
    r"three emblems are lit", r"lit by its own bell", r"a rung bell", r"each bell", r"three bells",
]


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("--")]
    dry = "--dry-run" in argv
    if len(args) != 1:
        print(__doc__)
        return 2
    path = Path(args[0])
    text = path.read_text(encoding="utf-8-sig").replace("\r\n", "\n")

    if re.search(r"^#+[ \t]*1\.1[ \t]+The story", text, re.M):
        print("Section 1.1 already exists: the patch seems to be applied. Nothing done.")
        return 1

    failures = []
    for e in EDITS:
        kind, tag = e[0], e[1]
        if kind == "sub":
            _, _, old, new = e
            ms = list(re.finditer(flex(old), text))
            if len(ms) != 1:
                failures.append((tag, len(ms), old[:80]))
                continue
            m = ms[0]
            text = text[:m.start()] + new + text[m.end():]
        elif kind == "span":
            _, _, start, end, new = e
            n_start = len(list(re.finditer(flex(start), text)))
            ms = list(re.finditer(flex(start) + r".{0,3000}?" + flex(end), text, re.S))
            if n_start != 1 or len(ms) != 1:
                failures.append((tag, f"start x{n_start}, span x{len(ms)}", start[:80]))
                continue
            m = ms[0]
            text = text[:m.start()] + new + text[m.end():]
        elif kind == "line":
            _, _, pats, bullet = e
            done = False
            for pat in pats:
                ms = list(re.finditer(r"^[ \t]*(?:[-*+]|\d+\.)[ \t]+.*?" + pat, text, re.M | re.I))
                if len(ms) == 1:
                    m = ms[0]
                    indent = re.match(r"[ \t]*", text[m.start():]).group(0)
                    text = text[:m.start()] + indent + bullet.replace("\n", "\n", 1) + text[m.start():]
                    done = True
                    break
            if not done:
                failures.append((tag, "no list line found for any of the anchors", " / ".join(pats)))
                continue
        else:
            _, _, hre, builder = e
            ms = list(re.finditer(r"^(#{1,6})[ \t]*" + hre + r"[ \t]*$", text, re.M))
            if len(ms) != 1:
                failures.append((tag, len(ms), hre))
                continue
            m = ms[0]
            text = text[:m.start()] + builder(m.group(1)) + text[m.start():]
        print(f"ok    {tag}")

    if failures:
        print("\nNOT APPLIED. These anchors were not found exactly once:")
        for tag, n, what in failures:
            print(f"  FAIL {tag}: found {n}: {what}")
        for tag, n, what in failures:
            key = re.sub(r"\\.|[^A-Za-z ]", " ", what).split()
            key = " ".join(key[:3])
            if key:
                print(f"\n  Lines of the file near '{key}' (for {tag}):")
                found = 0
                for i, ln in enumerate(text.split("\n"), 1):
                    if key.lower() in " ".join(ln.split()).lower():
                        print(f"    {i}: {ln.strip()[:110]!r}")
                        found += 1
                        if found == 4:
                            break
        print("Nothing was written. Send this list back and the anchors will be corrected.")
        return 1

    text = text.rstrip("\n") + "\n"
    print("\nReview (these words may still appear on purpose in the new texts):")
    for pat in LEFTOVERS:
        for m in re.finditer(pat, text, re.I):
            line_no = text.count("\n", 0, m.start()) + 1
            print(f"  line {line_no}: {pat}")

    if dry:
        print("\nDry run: all anchors found, nothing written.")
        return 0
    path.write_text(text, encoding="utf-8", newline="\n")
    print(f"\nWritten: {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
