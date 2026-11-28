# Design

The target design of DualLayer. Each part is marked **built**, **partly built** or **planned**. The code as built is described in [Architecture](ARCHITECTURE.md), the build and the checks by eye in [Build](BUILD.md) and the rules of the code in [Conventions](CONVENTIONS.md). How the design changed is in the [Changelog](../CHANGELOG.md); this document says only what the design is now.

## 1. The game

DualLayer is a third-person 3D exploration puzzle without combat, set in one walled harbour town on a hill, at two moments:

- **A, the past:** the last calm evening before the great storm: the last light of dusk, the sun low over the western wall and the streets already in shade, the lamps lit, a clear sky, a flat sea, the town whole and its people standing about their business at the end of the autumn fair. The lantern holds this evening still, so it has no clock.
- **B, the present:** the same town twelve years after the storm: ruins under a cold moon and in a fine steady rain, a few people on the heights, and everything the evening left alone grown, rotted, silted or fallen.

The traveller carries a lantern. **Tab** moves the whole world between the two moments. What is done in the past is what twelve years make of it in the present, and never the other way. So the game asks one question everywhere: **what will twelve years make of this?** The way through the town is found by reading the present and changing the past (3.9).

The game asks one thing, and the player sees it from the first frame: a signal mast with three dark lamps stands over the warning tower, high above the town. The whole game in one line: **the storm cannot be stopped, but what the traveller does on this evening decides what is left of the town afterwards.** In each of three districts the keeper of one of the harbour's instruments is kept from his post by an ordinary trouble; the traveller removes it, and he gives his key. With the three keys the tower opens, and at its top the traveller gives the flame of the lantern to the lamps of the mast and rings the bell, and twelve years pass over the town in one sweep (section 5). All the way the traveller can do small favours for the people of the evening, and what they keep (a seed, a plank under a roof, a boat hauled up) is what the town has twelve years on (section 4). The story is in section 1.1.

### 1.1 The story

The story is told by the town and by short lines of its people, in Japanese and without exposition; the main route never needs it explained. It is as short as it can be, so that every obstacle, person and object has a reason that an ordinary person would accept (principle 11, section 2). Only one thing is not ordinary: the lantern. There is no villain; the storm is weather, and what failed was an evening of small, ordinary troubles.

**The storm.** On an evening like this one a great storm came out of the sea, and nobody was warned. The signs were there for anyone who read them: a long swell running ahead of the storm under a clear sky, so that the harbour breathed out of step with the tide table; the birds gone from the cliff; the weather glass falling. But the last evening of the autumn fair is not an evening for reading signs. The carpenters, the barge crews and the stage hands were all busy with the fair, and each of three officers who read the signs for the harbour was kept from his instrument by an ordinary trouble of the fair (below). At nightfall the sea, the wind and the rain arrived together. People fled uphill when the water came, and most lived.

| Height | What the storm did | What B shows |
|---|---|---|
| below 0 m: harbour, lower town, cistern | the sea came over the sea wall onto the lower terraces | silt and a tide line on the walls, wrecks in the streets |
| 0 to 14 m: market, bastion, the lower locks | surge and spray; the aqueduct fell; rain filled the gorge and took the west bridge | salt-stained stone, the fallen aqueduct, the wreck of the bridge in the gorge |
| 14 to 30 m: the Rock, the upper locks, the upper street, the theatre | the wind tore off roofs, scaffolds and the awning of the theatre | houses without roofs, the fallen back wall of the theatre |
| 30 to 42 m: the cliff and the graveyard | rain loosened the cliff | a slide across the cliff way |

The harbour, the fleet and the lower town were lost. In the years after, the wells of the lower town were fouled with salt, the aqueduct stayed down, there were no boats to fish or trade, and most people left. Twelve years on, a few households live on the heights, on their own cisterns and terrace gardens, and what traffic there is goes by the land road.

**The tower and its rule.** The warning tower on the Rock holds the bell and the signal mast, whose three lamps are lit only to give a warning. By the ordinance of the harbour the tower is kept locked with three locks, each opened by the key of one officer, because a false alarm sends a whole port out of its beds. Each officer reads a different sign on a different instrument, so that a warning needs the water, the sea and the sky to agree.

| Keeper | Post | Reads | Instrument | His trouble this evening | Emblem |
|---|---|---|---|---|---|
| Lockmaster (east) | the water tower over the locks | the water out of step with the tide | the tide board: a float in a well that moves a pointer | the fair's cargo barge is wedged in the third lock and holds the float | box |
| Watch captain (north) | the watch post on the cliff | long lines on a flat sea, the birds gone | the lookout and its glass | the stage hands have hung the great awning and its scaffolds of the theatre across his view, and his glass is out for mending | column |
| Gate warden (west) | the signal house of the bastion gate | the weather glass falling | the weather glass | the carpenters, hired for the stalls of the fair, took the glass down to mend its frame and shut the house behind their scaffolds | sphere |

In an alarm each keeper must stay at his post (the sluices, the lookout and the gate have to be manned), so each gives his key to the one person who is free, the traveller. He does so only when he has read his own instrument and seen his own sign: a stranger who speaks of a storm on a clear evening is a gossip, and polite disbelief is the answer to any talk of the future. The traveller therefore never argues. The way to each keeper removes the trouble that keeps him from his instrument, and he decides for himself.

**The lantern, the one impossible thing.** An old harbour lantern whose flame was lit on that last evening and has kept it ever since. Whoever carries it can step into the evening (Tab) and out of it again (3.1), and the evening stands still while the flame burns in it. It is one lantern in both moments, as the body is one body. The evening can be changed; the present cannot change the evening. What is done in the evening is what twelve years make of it, so it shows in the present (3.3, 3.9). B at the start is the present in which nothing was done; after a way is solved, B is the present that follows from what was done. The traveller is a stranger who came to the half-empty town by the land road long after the storm, so nobody in the evening knows the traveller. How the lantern came to the traveller is not told (section 14). Its flame is given away at the end of the evening (5.2).

**What the traveller can do in the evening.** The storm cannot be stopped and words do not work. The traveller can do what any person could, with the strength of one person: push a crate, open a gate, lay a plank, lower a lift, plant a seed, hand a small thing to someone who will keep it. That is the whole verb set (3.6, 3.9).

**The people.** A is the last evening of the fair: stalls, bunting and a quiet crowd. People stand where they are (at their stalls, at their doors, at the quay looking at the evening) because that is what people do at the end of a fair day, so the player never has to chase anyone. They speak short lines in Japanese without exposition. A person is a thing of A (3.3): what is given to him or done for him shows at his place in B, where the same people, twelve years older, stand at the same places where they can. At the start of B only a few are there, on the heights, with smoke, washing and a lit window.

**Favours and what they keep.** Many households have a small trouble of their own this evening: a boat that should be above the tide line, a tool left in the rain, a wall that leans, a garden with no seed. The traveller who helps leaves the household better prepared for a night it does not know is coming, and a person keeps what he is given. When the bell is rung the whole town goes uphill, and each household takes with it what it had prepared. So the town twelve years on is made of what people kept, and no favour is needed to finish the game (section 4).

**The ending.** With the three keys the tower opens, and the climb ends in the belfry, from which the whole town is seen in both moments. The traveller lights the three lamps of the mast with the flame of the lantern and rings the bell. In A everyone knows what the bell and the three lamps mean: people go up and boats cast off. The storm comes anyway, and is shown once, when nobody is in its way. Then the evening ends: twelve years pass over the town in one sweep (5.3), and the traveller walks down a town that was kept, where the people who were helped remember the stranger of the fair (5.4). The loss in the history the game starts from is a port and a way of life; no death is shown or told. What the game repairs is that nobody had a reason to ring the bell.

## 2. Principles

Every change is judged against these. A change that breaks one is not made.

1. **One rule.** Everything comes from the two moments: the switch, the lantern and the causal rule are three sides of it, and the only thing that happens between the moments is twelve years (3.9).
2. **Time is the verb; favours are the puzzles.** Every puzzle asks what twelve years will make of something, and the main route is made of favours done for people and for the town. Nothing is fetched across the town, collected or counted.
3. **Hard in the head, not in the hands.** No precise timing on the main path (limits in section 11).
4. **Few actions per room.** A room asks for one or two acts in A and one or two switches, at places the scene points to; the rest of the room is walked in one moment.
5. **Seen before reached.** Each place is seen first, then reached from an unexpected direction. No text guides the way.
6. **A goal in sight.** From the first frame the player can see what the game asks (the three dark lamps of the mast), and each district opens on something seen and not yet reached. A player who never presses a help key still knows what to do next (section 10).
7. **Free order.** After the teaching ground the three districts are done in any order. No district needs another, none teaches what another uses, and all three reach the same ceiling of difficulty (7.4). Inside a district there is more than one way in (7.13).
8. **No forced backtracking.** No puzzle sends the player to another district. A local connection may return to known ground only when it offers an immediate route or a new view before the finale, and no shortcut sends the player back to the start after the clear. The one walk back that the design accepts, from a keeper to the lower town, is in 7.4.
9. **Nothing that looks like a bug.** The body is never pushed out of a wall; a switch or a pick-up into something solid is refused instead.
10. **Two states of one town, and a third.** Light and colour distinguish the moments at a glance; damage, materials and signs of use keep the difference readable without colour (section 6). Where the player has helped, B takes a third look that is neither A nor the ruin (6.4).
11. **Ordinary reasons.** The lantern is the one thing that cannot happen. Everything else has a reason that an ordinary person would accept, and twelve years do only what twelve years really do (3.9): nothing is shut, broken or stuck only because a puzzle needs it, and nobody acts against his own sense.
12. **Kindness is shown by the town, not counted.** No counter, no list of tasks, no score: a household that kept something, seen twelve years on, is the record.
13. **Explained in person.** The core is hand-written C++ and small enough to be explained line by line in an interview. The ledger of twelve years is a table and one function.
14. **A wrong try costs a step back.** Every act of the traveller can be undone by its opposite until the bell is rung (a crate pushed back, a plank taken up, a seed dug up), and a person who is offered what his trade does not want refuses it with a gesture and loses nothing.

## 3. Mechanics

### 3.1 The switch

| Rule | State |
|---|---|
| Tab moves the whole world between A and B; both moments are complete, walked and seen whole | built |
| A switch into anything solid, or into water, is refused: the ripple opens and turns back | built |
| A jump may start in one moment and land in the other (the switch in the air) | built |
| Holding Tab on the ground shows the other moment in a circle in the middle of the view, without going there (3.8) | planned, core |

### 3.2 The lantern

The switch moves the player between the moments; the lantern keeps a piece of one moment in the other.

| | Does | Used for |
|---|---|---|
| Switch (Tab) | the whole world becomes A or B | moving, exploring, the views; both maps are seen whole |
| Lantern (set down) | a circle round it stays A while the world is B | puzzles |

Rules:

- The lantern is **set down in A only**. From then on its circle is A in both moments.
- In B the world is the ruin, except the circle: a stretch of a bridge that is whole, a stair that still stands, a wall that still holds. **Inside the circle the ledger of 3.9 has not run:** what has rotted outside is whole inside.
- Inside the circle what is seen is what is solid: A. Outside it, B.
- Picking the lantern up removes the circle. A pick-up that would leave the body inside something, or standing on nothing it was standing on, is refused, like a switch.
- The circle is warm light in the cold ruin: the image the game is recognised by. The lantern is also a real light that lights what is near it in either moment (6.3).
- **In B the circle is dry (decided 2026-11-28, planned).** Inside the circle is A, so no rain falls in it and nothing in it is wet (6.11): a warm, still bubble in the rain.
- **The circle is drawn in A too (decided 2026-11-28, planned).** Set down in A, the lantern shows what it will hold: a thin warm ring on the ground at the radius of the circle and a faint wall of light at its edge, so the player sees the area before the switch. In B the same ring is the rim of the held circle. It is a hint of the rule and not a second rule: nothing is solid or changed by it.
- **Dark places in A (decided 2026-11-28, planned).** Some places of A are dark even at dusk: a cellar, a cistern, the stair of a tower, a passage under a house. The lantern lights them, in the hand or set down. A dark place holds only what is optional (a favour, a view, a secret) and never a step of the main route, so that the route reads without the lantern (6.3).
- E sets the lantern down or takes it up, or acts on what is in reach (3.6); the prompt always names the one thing that E does now (10.3).
- The lantern is the one thing in the game that is not ordinary. How it came to the traveller is not told. It gives its flame once, to the lamps of the mast, and the evening ends (5.2).

Example, the broken bridge: in A walk onto the whole bridge and set the lantern down in its middle. Walk back to the bridgehead and switch to B: the bridge has fallen, but its middle hangs in the circle. In B jump from the bridgehead onto it and on to the far side, where the gate is open in ruin (in A it is locked). One switch; the thought is "the bridge of the past, the gate of the present".

As built (`scene/Lantern.h`, `scene/Moments.h`): the circle is a sphere of radius 3.5 m round the lamp, 0.45 m over the floor it is set on, 0.9 m ahead of the feet (at the feet at an edge); taken up within 1.8 m. Its edge in B is a warm rim of light. There is one lantern, with no radius upgrades.

### 3.3 The causal rule

**What is done in A is what twelve years make of it in B. Nothing done in B changes A.**

| Done in A | In B |
|---|---|
| a gate or a door opened | its ruin stands open: the iron rusted in the state it was left |
| a ballast crate pushed into a channel | its box has rotted and the stone lies where it stopped, a heap on the dry bed |
| a seed planted in soil in the sun | a tree |
| a plank that is tarred, or laid under a roof | the plank, grey and sound enough to hold; raw timber in the open is a stain |
| a timber prop swapped for stone | the lintel it held stands |
| a boat hauled above the tide line | the boat on its slip |
| a small thing given to a person | where he kept it, and he, twelve years older |

- **Local:** a cause and its trace are in the same room or within sight of each other.
- **Readable:** the trace is where its cause was, so one switch shows the result.
- **A person is a thing of A.** What is given to a person, asked of him or done for him changes his stance in A (he steps aside, hands something over, opens a door). In B the same person, twelve years older, stands at the same place and the change shows there (an open door, a kept thing, a tree in his garden). Nothing said or done in B reaches A.
- **B has its own things.** A beam moved in B stays moved in B; it does not travel to A. A thing of B that was moved is a step toward the next place of B, nothing more.
- **The body and what it carries** switch freely in both directions; the rule is about the world.

### 3.4 Puzzles start from the present

Every puzzle begins in B. The ruin shows a thing that has no cause: a bare planter, a stub of rotten timber, a gate rusted open, a bank of silt, a person standing at a door where nobody should. The loop is: **look at the present, reason about the past, act in the past (or set the lantern), come back to the present to see it.** The "of course!" of a room is always the same sentence: that is why it looks like this now, because that is what twelve years do.

### 3.5 Traces

A trace is a result seen in B: a tree, a heap of stone, an open gate, a boat on its slip, a person with something in his hands. A faint glow in the colour of the lantern marks one only when it would otherwise be missed (never gold: gold means use). The traces add up: from the cliff and the tower the player sees across the whole town what was changed. A trace is feedback and story at once, without a word.

### 3.6 Objects

Each object answers one question: what happens to it when the moment changes?

| Object | Rule |
|---|---|
| Wall or floor of one moment | exists in A (made by people: timber) or in B (left by the ruin: fallen masonry) |
| Ballast crate | pushed in A; its box rots and its stone stays as a heap in B |
| Timber | raw (pale) rots in the open; tarred (black) or sheltered lasts (3.9) |
| Rope, canvas, basket | rot in the open, keep under a roof |
| Water | in A only, fed by the aqueduct (broken in B); not a floor for the body; things float on it; it leaves silt in B (3.9) |
| Wheel, lever, sluice, winch | machinery: works in A; in B stuck as A left it |
| Seed | planted in A at soil in the sun; a tree in B |
| Prism, lens | moves a thing of A into B ("it lasted until now"); never the other way |
| Person | a thing of A (3.3); the same person, twelve years older, in B; keeps what he is given |
| Mark (a carving, a sign, a tally) | exists in both moments; weathered in B; the difference between the two is a clue (8.2) |

Not used: free physics, throwing, timers on the main path, combat, an inventory beyond the lantern and one small thing in the free hand (3.9).

### 3.7 Player and camera

The town uses a third-person follow camera. The lantern follows the right hand while carried; when placed, it follows the rules in 3.2. Character and lantern art are in 6.7.

- **Movement:** hold Shift to sprint at 1.6 times walking speed by default, without stamina. Ground animation follows actual movement after collision. Sprinting lengthens the stride and leans the body forward. The main route does not require sprinting for every obstacle.
- **Camera:** the follow distance starts at 3.8 m. The camera pulls in when blocked and extends gradually when clear. During a switch ripple, geometry in either moment can shorten the distance. The character is hidden if the camera gets too close.
- **Climbing:** a forward jump with Space held can climb a nearby ledge up to 2.2 m above take-off if the top has support, the landing is dry and the body fits along the sampled path. Walking into a wall does not trigger a climb. A dedicated pull-up animation is not included.
- **Acceptance:** verify the quay, the sea gate and the first aerial switch with this controller, and check that sprinting or climbing cannot bypass the intended thresholds. The route solver does not model these moves yet.

### 3.8 The lens

Holding Tab looks into the other moment without going there. It turns trial and error into reasoning: the player may see what the other moment holds, but still has to say what to do in A.

- **Hold:** on the ground, Tab held for 0.2 s opens a circle in the middle of the view (about a fifth of the screen height in radius) in which the other moment is drawn. Its rim is warm when it shows A and cool when it shows B. Releasing closes it. The body, the camera and the solid stay in the moment they were in.
- **Tap:** on the ground, Tab released within 0.2 s switches, at the release. In the air Tab switches at once, on the press, and the lens is not available: the switch in the air is an act, not a look. If the switch on the ground feels late in play, the lens moves to the right mouse button and Tab switches on the press again.
- **What it shows:** the other moment as it is now, with every trace and the circle of the lantern; never what has not happened. It shows a place, not an answer.
- **When it closes by itself:** when the body leaves the ground, sinks, falls or is returned.
- **Made from:** the two pictures the ripple already draws (`Models::DrawRipple`), with a held circle instead of an opening one; no new pass. While it is open both moments are drawn: the cost on the laptop is to be checked.

### 3.9 What twelve years do

The present is what twelve years make of the past. **B is a function of A:** every time the moment changes, the town computes B from the state of A by the rules below, and no history is kept. The rules are a short table in the town data (what a thing is, where it was left, what it becomes). They are the same everywhere, each is something that really happens, and the ruin shows them, so a player who knows how the world works can predict the present and a player who does not can read it (3.4).

| Rule | In A | In B, twelve years on | How it reads |
|---|---|---|---|
| Growth | a seed planted in soil that has sun (a bed, a planter, a terrace of earth) | a tree of the seed's kind (below); a vine with no wall to climb is a tangle on the ground that gives no way up | dark loose earth with a gold ring; a seed bag marked with the picture of its tree |
| Rot | raw timber, rope, canvas or a basket left in the open | gone: a stain, a stub, a line of rusted nails | pale timber; green stains and a dripping eave mark the open |
| Keeping | the same under a roof or in a closed room (a place marked sheltered), or timber that is tarred | still there: grey, and sound enough to hold | black timber (tarred); a roof over the spot |
| Rust | an iron mechanism: a gate, a winch, a lever, a sluice | stuck in the state A left it: what was left open stays open, what was left shut stays shut | gold in A, brown in B |
| Prop | a stone held up by timber | the timber has rotted and the stone has fallen; a stone held up by stone stands | a wooden prop under an arch or a lintel |
| Silt | water held, led or let run | where water stood behind a dam that still holds, a bank of silt as high as the dam; where it ran, or where the dam gave way and took the silt with it, a bed scoured deep and dry | a tide line, a dry bed, a bank |
| Sea | anything below 0 m, the line that the sea of the storm reached (1.1) | the ground is salted and no seed grew there; a boat left there is lost; stone, tar, iron and plant stuff are as without it | white salt on the ground, a tide line on the walls |
| Keeper of things | a small thing given to a person | kept in his own place, by the rules above, and he stands there twelve years older | a person takes what his trade wants and pushes the rest away |

There is no other rule. Nothing grows in the dark or in salt, nothing rots under a roof, and a thing that is never touched is the same. The table is `scene/Ledger.h` and is data: a row says that this stuff, at a place with these facts, becomes this, and the first row that matches decides. The facts of a place are five, set by the model and never worked out by the game: soil, sun, a roof, a wall at the foot and the sea (the sea only from the height of the place). The table is tested on every combination of the five (`tests/test_ledger.cpp`, section 11).

**Trees.** A seed grows where there is soil and sun, and not in shade, under a roof, in a cellar or in salt (an empty spot in B). Three seeds, each with a bag that shows its tree. Every rise between two places to stand is at most 1.2 m, a step of section 11, so a tree can be climbed from the ground (the olive from a ledge of 1.2 m); the table keeps to it (`maxRise`) and reads itself back for a tree that cannot be climbed:

| Seed | After twelve years | Used as |
|---|---|---|
| Olive | a stout tree of 2.4 m, a crown that the body steps onto from 1.2 m | a step |
| Fig | a spreading tree of 4 m, with limbs at 1.2 m, 2.3 m and 3.4 m | a stair of three |
| Vine | a vine on the wall at whose foot it was planted, with footholds every 1.2 m up to 6 m | a ladder |

**Keeper of things.** Each person keeps what he is given in the place of his trade: the gardener plants a seed in his own garden, the carpenter lays timber in his dry shed, the fisherman hauls a boat above the tide line, the cooper stores in his cellar. In the table the gardener takes the three seeds (his garden has soil, sun and a wall, so a vine he plants is a ladder), the carpenter timber, raw or tarred (a roof), the cooper baskets and timber (a roof) and the fisherman a boat (above the sea). He takes what his trade wants and refuses the rest with a gesture, so a wrong try costs nothing (principle 14). What he keeps follows the rules above in that place. Giving a thing to a person lets the traveller act where the traveller cannot go (a walled garden, a shed behind a locked door) and is how a thing is put under a roof.

**Ballast crates.** The crates of the game are ballast crates: timber bound with iron and full of stone. The box rots and the stone stays where the crate stopped, so a pushed crate is in B a heap of stone as high as the crate was (a crate is packed tight; under a roof the box stands too, as a plank does). This gives the built crate puzzles their reason; they do not change: the crates are steps of 1.2 m, and `tests/test_things.cpp` keeps the box of B the height of the heap that the ledger gives.

**Dams and props.** They are not rules of their own. A load stands when its support does: a stone held up by timber falls when the timber is gone, and stands when it is tarred or under a roof, or when the support is stone. Water held behind a dam leaves a bank of silt as high as the dam while the dam holds (stone, a ballast crate, tarred or sheltered timber, a sluice stuck shut) and a dry bed when the dam gave way (raw timber in the open, a sluice stuck open): the silt goes with the dam. That is why a lock dammed with timber leaves no step and one dammed with ballast crates leaves a bank (8.4).

**Timber.** Raw timber is pale. Tarred timber is black: the planks of the roofers and the boatyard, the piles of the quay and the doors of the harbour. The two are told apart at a glance, in both moments and in greyscale, and the quay of the first frame shows both (7.12).

**Held thing.** Besides the lantern the traveller carries one small thing at a time in the free hand: a seed bag, a small tool, a letter. It is picked up and given in the same district; if the traveller leaves the district it goes back where it was found. Heavy things (crates, stone blocks, planks, beams) are pushed or laid in place and never carried.

**What the ledger is not.** It is not a simulation: there is no timer and no growth to watch, and the traveller sees B only by switching or by the lens (3.8). The one time it is seen running, over the whole town, is the ending (5.3). A rule that is not in the table does not exist, and a new rule is added only if a puzzle that needs it has been tried with the ones that are there.

**Built and not built.** Built: the ledger as data and rules (`scene/Ledger.h`: the table, the trees, the keepers, props and dams, a reading back of the table for rows that cannot match and trees that cannot be climbed) with its test (`tests/test_ledger.cpp`); in the town, the rule of rust (things opened in A stay open in B), the pushed crate and the held floor of the lantern. Not built: the ledger in the game: the facts of every place in the town file (`ageing`) and the exporter that sets them (the sea from the height), B drawn from it, the seed and the held thing, the undoing of acts (the built things only open) and the tarred colour of the plank.

## 4. Progress and what the town keeps

- **The three keys and the tower.** Each district has one keeper of the harbour, with his post at the end of its way (1.1). His key is the reward of the way's last beat: the trouble that kept him from his instrument is removed, he reads it, sees his sign and gives his key. The door of the tower has three locks, one for each key, marked with the same three emblems; it opens when all three are turned (in A, which leaves it open in B). The signal mast stays dark until the belfry (5.2). The three emblems are the only progress mark and carry no words (10.5). Progress is also shown by the town: a keeper who has given his key stands ready at his post in A, and his post is lit and manned in B (6.4).
- **Favours.** Many households have a small trouble of their own this evening (1.1). A favour removes it with the verbs of 3.6, and the household then prepares: a boat is hauled above the tide line, a seed is planted, a tool is put under a roof, a wall is propped with stone. A favour is a use of the ledger (3.9) for someone's benefit, so it is a small puzzle of its own, and its result is seen in B.
- **What a favour does.**

| Where | Effect |
|---|---|
| A | the person's stance changes (he steps aside, nods, says a different line) and the household gets on with its preparation |
| B | the same person, twelve years older, stands at his place with what was kept; the household shows signs of life (smoke, washing, a stall, a boat on its slip) and its window is lit |
| The bell | when the bell is rung the household goes uphill with what it prepared, and so keeps it (5.3) |

- **No counter.** There is no counter, list or score of favours. The town is the record: from the cliff and the tower the player sees which houses are dark and which are lived in, and the dark ones are what is still to do. No favour is needed to finish the game, and a run with none still rings the bell and reaches the ending. A run with many reaches a fuller one, and the belfry shows the player which houses are still dark before the bell is rung (5.2).
- **The links to the Rock.** Each district ends in a link to the plaza of the Rock, opened by that district's last puzzle (7.4). A link is a connection, never a requirement: every district is complete from the lower town alone. The player takes the link of the district finished last, so the finale does not start with a long walk back.
- **Recovery.** A fall or R returns the body to the last safe standing point in both moments, without depending on the placed lantern. There are no collectible checkpoints.

## 5. The tower and the end of the evening

The finale has four parts that say the same thing from four sides: **the traveller has changed this town, and the town knows it.** The climb is the whole game once more in one place; the belfry shows the result before it is made; the evening ends in one sweep of twelve years; and the people who were helped walk out to meet the traveller.

### 5.1 The way in and the climb

The three links meet on the plaza of the Rock, and the player arrives by the link of the district finished last. From the plaza the door of the tower, its first stair and the harbour below are framed together. The door is opened with the three keys. The tower is entered by its door and climbed by its stairs; standing on a wall must not be a way to miss them (the play-test of 2026-11-27 reached only the wall, KI-09).

The climb has three floors and a last flight. Each floor puts one rule of the ledger (3.9) into a larger space, and the last flight is the first lesson of the game again. No new rule is taught inside the tower.

| Part | The rule | The space |
|---|---|---|
| lowest floor (east) | silt and rust | the old cistern of the tower, full in A and a dry basin in B; its sluice stays as it was left, and a bank of silt is the step to the window ledge |
| middle floor (north) | prop | a gallery carried over a drop by timber props; in B the timber has rotted and the gallery has fallen, unless a ballast crate was pushed under its beam in A: the box rots and the stone carries the beam |
| top floor (west) | growth | an open well of light with earth at its foot; a vine planted in A is a ladder in B up the wall; the seed bag lies at the foot of the stair, as seeds are found in every district |
| the last flight | the first lesson again | the stair above the vine ends 3 m under the belfry hatch. In A the stair is whole and the hatch is bolted from above, as the ringers leave it at dusk; in B the timber hatch and the last flight have rotted away and the hole is open. The way is one jump at the top of the stair with the switch in the air at its height, onto the stone floor of the belfry: the aerial switch of the lower street (7.12, T6), and under it the first thing the game taught, a way that is closed in one moment only (T1) |

- Each floor ends at a window that shows its district as the player left it: the households that kept something, the post of the keeper, the boats on their slips.
- A household that was helped may have left a small help on the stair (a rope let down, a board across a gap). Such a help shortens a floor and never opens it (planned, optional).
- Which floors need the lantern, the order of the floors that the stair allows and the exact layout are play-test decisions (section 14).

### 5.2 The belfry

The belfry is a ring of stone arches under the great bell, with the signal mast rising from its roof. It is the one place in the town from which every district is seen in both moments, and the player is given time there before the last act.

- **The result before the choice.** B is computed from A (3.9), so the present that the player sees by switching in the belfry is exactly the present that will follow: the households that kept something are lit and lived in, and the others are dark. The map (M) shows the kept households as warm dots. The bell adds nothing; it only lets the town keep what it prepared. A player who sees dark windows can go back down and help: the keys stay turned and the keepers stay at their posts. Nothing tells the player to do so.
- **The last act is small.** At the foot of the mast E passes the flame of the lantern to the three lamps one after the other (the flame leaps to each wick, and the lantern's own light dims by a third each time; E at a lit lamp takes the flame back). At the rope E asks RING and is held for two seconds so that it is never pressed by accident. Nothing needs timing.
- **The lantern gives its flame.** The flame of the lamps is the flame of the lantern, and the evening it kept: at the first toll of the bell the lantern's flame goes out and does not come back. From then on the traveller carries an ordinary lantern and Tab does nothing: the evening is over. It is the one cost in the game, and it is paid by the one thing the player has carried since the first frame. The rope is the only place where E does something that cannot be undone (principle 14), and the flame of the lantern leans toward it, so the player knows.
- **A place to go back to.** The game keeps a save at the foot of the last flight, so that a player who rings and finds the town less kept than wished can load the belfry again and go back to help (planned).

### 5.3 The storm and the twelve years

The ending is staged, not played, and takes about a minute; a long press skips it.

1. **Uphill.** The bell tolls three times over the town. The camera rises from the belfry. People leave their stalls and go up the stairs and lanes with what they prepared; the households that kept something carry it (a seed pot, the gear of a boat, a tool); the boats cast off from the quay; the streets empty. Nothing is simulated: the scene is made from the list of households that kept something.
2. **The storm.** For the first and only time the storm is shown, and only when nobody is in its way: a wall of cloud over the bay, the swell, rain, the sea coming over the sea wall up to the tide line that B shows on the walls, the wind taking the awning of the theatre. The lamps of the mast burn through it and their beam sweeps the bay. The heights hold. (Cost to check: a rising sea plane, rain and cloth. If it is too much, the storm is cut and the sequence goes from the bell to the sweep.)
3. **The sweep.** The camera holds the belfry. The ripple of the switch (3.1) leaves the tower, slowly, as a ring across the whole town, and the ledger (3.9) is seen running at the front of the ring: inside, twelve years on; outside, the evening. Seeds become trees, raw timber falls to a stain, black timber stands, iron rusts in the state it was left, boats stay on their slips, people age at their doors. The ring reaches every household in turn and slows over a household that kept something. It is the whole game in one image: what twelve years do to what was done.
4. **The morning.** The ring reaches the sea. The camera cuts to the first frame of the game on the quay, in the first light of a clear morning: boats in the harbour, smoke from the heights, a swept street, the lamps of the mast burning pale in the day. The look depends on how much the town kept, in three looks: a few lived-in houses on the heights, about a quarter of the town lived in, a harbour at work. All three are complete endings, and none is behind a hidden thing: the thresholds are tuned so that a player who follows the favours the town shows reaches the second (numbers to tune, section 14).

### 5.4 The walk and what the town remembers

Control returns on the quay, at the first frame. The town is the town the player changed.

- **They remember.** Each person who kept something stands at the place where they stood in the evening, twelve years older, and says one short line in Japanese that recalls the favour without explaining it. They know the traveller, who is exactly the age of that evening: the lantern kept it. The line is the whole reward of a favour, and there is no other (principle 12).
- **The first and the last.** The first person on the quay is the woman of the first lit window (7.12, T0 and T2), at her door beside the olive. The last is whoever the player helped last, and the walk ends where the player chooses.
- **Free to walk.** The traveller walks the whole present for as long as the player likes: the towers, the bridges, the households. The game offers to end at the quay and does not force it.

## 6. Art direction

### 6.1 Style and scope

**Simple forms, rich atmosphere.** DualLayer is a stylised harbour diorama, not a voxel world or a photographic reconstruction. Keep the irregular terrain, terraces, walls, rooflines and the bell tower. Use low- to mid-poly geometry with clear silhouettes. Add geometry where it changes the outline, explains function or helps navigation: coping stones, recessed doors, quayside supports and arch profiles. Keep passing walls quiet and reserve close-up detail for landmarks and interactions. Build depth and weathering through overlapping silhouettes, restrained surface detail and readable age differences between A and B; detail supports navigation and never replaces it.

### 6.2 Materials and surface detail

| Material | Visual rules |
|---|---|
| Stone | Broad, uneven courses; restrained joints and rough highlights. Darker wall bases, coping stones and corner blocks before small cracks. Stairs and bridges read as stone at normal camera distance. |
| Wood | Grain follows the plank or beam; joints and end grain where the construction calls for them. Raw timber is pale and in B is gone or a grey stub; tarred timber (the planks of the roofers and the boatyard, piles, doors) is black in both moments and lasts (3.9). |
| Plaster | Subtle broad variation, darker under eaves and near the ground, exposed masonry only where there is a reason for damage. |
| Roof tile | Three or four closely related terracotta variants. In B the same roof identity where it survives, weathered, with tiles missing selectively. |
| Metal | Brass, iron and verdigris for functional details; bright warm accents only near useful interactions. |
| Glass and flame | Lamp glass is the one clear material of the game; a lit lamp is the warmest thing in any view (6.3, 6.4). |

Shared 512 or 1024 px textures are the starting budget: base colour and roughness first, restrained normal maps and baked contact shading where they improve the read. Texture scale is consistent between assets.

### 6.3 Light, sky and distance

One sun or moon with soft sky and ground fill, the lamps of the town, and what is added of both to the picture. The numbers are in `[lighting]` of the config ([BUILD.md](BUILD.md)); this is the rule and the reason for each.

- **The sun of A and the moon of B cast shadows.** One shadow map of 2048 x 2048 texels covers 60 m round a point a little ahead of the camera; the box follows the camera on the grid of the texels, so a shadow never crawls. The town, the traveller, the crates and the things that change cast a shadow; water, glow and halos do not. A shadow takes 85% of the direct light and none of the sky light, so a shadow is never black. A lamp casts no shadow: its light is a soft pool.
- **Every lit lamp of the model is a light.** The lamps of the streets, the plaza, the harbour, the lighthouse and the tower light the surfaces round them in the moments in which they are lit: a warm pool 14 m across for a street lamp. In A the sun is low and the streets are in its shade, so the lamps are lit and read against the shade (0.9, 6.10 S3c); in B they are the only light but the moon, and the lit ones along the main path read as a trail of pools. The renderer lights a surface with the 32 nearest and strongest lamps of the moment, and a lamp fades with distance, so none blinks on or off. A flame is never still: a lamp flickers by 3 to 10% (the lantern of the traveller by 12%), with a phase of its own.
- **What the player did is a light too.** The lantern of the traveller, in the hand or set down, lights its surroundings (13 m) in both moments with the colour of its flame. The lamps of the mast, lit from the lantern in the belfry, light the tower under them (5.2). The world answers with light.
- **No lamp is needed to read the route.** The route, the next landing and every interaction must read in the moonlight alone (`lamps_b = 0` tests it). A lamp helps the eye and never carries the game; gold still means use, and the lantern's own colour is its flame.
- **No colour is cut off.** The sum of the light goes through a soft shoulder (a colour brighter than 0.78 bends toward white and keeps its hue), so a lamp lights a wall and does not turn it into a white patch. A soft glow round each lit lamp (a halo: 35% in A, 100% in B) puts the light in the air as well as on the walls.
- **The light has a style as well as a physics.** The town is hand-painted, and nine parts say so; each is a number in `[lighting]`, all of them are one master (`style`), and the last view of the key L, "plain", shows the same light without any of them. (1) The sun falls on a face in three hard tones (none of the sun, a part, all of it; 6.10, S1). (2) The sky light of a face that the sun does not reach leans cool: blue-violet in A, indigo in B. (3) Clouds drift soft shadows over the town (38% of the sun in A, 28% in B). (4) An edge that faces the light has a rim. (5) A lamp lights its pool in four soft rings, a whiter core and an amber edge. (6) The stone is painted with soft blotches and tall rain stains, and is damper near the water. (7) Rays of light stand in the air: each ray of sight is sampled 14 times through the shadow map, up to 48 m. (8) The far haze leans to the hue of the look (lavender in A, a grey-blue in B) and the far distance of A has more fog (6.10, S3). (9) The picture is graded: darks and lights lean opposite ways, colour is pushed a little, contrast is raised round the middle grey and the corners are a little dark. The style never carries a rule of the game: a landing, an edge and an interaction read in the plain view as well as in the styled one (E-25). Since 2026-11-28 (6.10, S1) the sun falls in hard steps and no longer in soft bands; planned: a line is drawn round the shapes (6.10, S2).
- **Sky and distance.** A procedural gradient replaces the solid sky: A moves from muted blue-grey overhead to pale gold at the horizon, with a low western sun; B moves from deep grey-blue to a misty cyan-grey horizon with cool moonlight. Restrained distance and height fog separate the quay, sea wall, streets, tower and hills. Fog and shadows never conceal the next landing or an interaction.
- **Not built:** a shadow of a lamp or of the lantern, cascades for the far town, contact shading at wall bases and under eaves, screen-space ambient occlusion, a real bloom, and shafts beyond 48 m. Take one only if a play-test shows the light is flat, and check it on both machines.

### 6.4 The same town at two moments

| | A: the evening | B: the ruin | B: kept (a household that kept something) | The first morning (ending) |
|---|---|---|---|---|
| Sky | muted blue-grey to pale gold, a clear sky, a low western sun, a long swell on a flat sea | deep grey-blue, a cold moon, torn cloud | the same sky, with a warm window and smoke | clear morning |
| Light and air | long amber sunlight, cool soft shadows, light golden haze | cool fill and moonlight, deeper blue-grey shadows, low cold mist | the moon, warm windows | the first sun in the game |
| Weather | still air, a flat sea, cloth that barely moves: the held evening has no clock | a fine steady rain from the sea side, a drip at every eave, wet stone, the sound of it (6.11) | the same rain, and the dry circle of the lantern in it | clear: the rain has stopped |
| Built surfaces | dry stone, intact doors and roofs, timber and terracotta, bunting and stalls | damp wall bases, greyer wood, rusted fittings; selective roof damage and debris | old stone with new timber and patches of new lime wash in another tone | the same, in daylight |
| Timber | pale (raw) and black (tarred) | pale: gone, or a grey stub; black: standing; sheltered timber grey and sound | new timber beside the old | the same |
| People | at their stalls, doors and posts | a few, on the heights | the same people, twelve years older, at the same places | in the streets |
| Marks of the storm | none | a tide line and silt on the walls | the line stays on the walls; the street is swept | the same |
| Goods and boats | stacked, moored | wrecked, stranded | goods stacked high, boats drawn up, simple stalls | boats in the harbour |
| Plants | tended gardens | scrub, figs, grass in the paving, reeds in the harbour | vegetable plots, young trees and what was planted | the same |

Changes follow drainage, exposure, damage and disuse, not a uniform filter. Enough geometry is shared to recognise the same place at once. In a greyscale A/B comparison the moment must still read through silhouette, value, damage or signs of use. The kept look must be told from A at a glance, in greyscale too.

**The evidence of the storm in A** (planned). The sea is flat and carries long lines of swell that run under a clear sky; the birds are gone from the cliff and the roofs; flags, washing and the awnings of the fair stiffen and flap a little more near the water; a band of cloud lies along the horizon. None of it is a warning that a stranger could act on, and all of it is what the keepers read. Cloth, flags, washing and plants move in A by a small vertex offset; in B they hang torn. The storm itself is shown once, in the ending (5.3).

**The repair in B** (planned). A household that kept something shows it: a tree where a seed was planted, a boat on its slip, a standing wall, a lit window with smoke. The light of the kept look is the warmest and smallest thing in a street, and a trail of lit windows leads up the heights.

### 6.5 District identity and vegetation

- **Harbour:** hoist, warehouse doors, mooring posts, ropes and boats. The strongest visual cue is on the way from the quay to the sea gate.
- **West, the market and the lamps:** cloth awnings in mustard and red, timber stalls, carts, brass lamps; the colour of work. **East, the locks:** verdigris copper, sea-blue shutters, wet stone, the colour of water. **North, the theatre and the cliff:** slate, oxblood, a pale stage; the colour of height and distance (colours to tune in the palette, 6.9).
- **Housing:** narrow doors, recessed windows, chimneys, occasional balconies and washing lines. Crossings are kept clear of decorative clutter.
- **Bell tower:** regular masonry, a clear arch and visible steps distinguish the approach from residential streets. The entrance is legible, not only the spire.
- Growth is placed by moisture, soil and shelter: more near water and ruins, moss on damp shaded bases, little in the middle of an intact route. Cones near the player are replaced by trunks, a few branches and grouped low-poly foliage.

Props and planting identify where the player is, point to the next route or show how the place changed. No high-contrast door or platform that looks usable leads nowhere.

### 6.6 Water

One stylised water language across the sea, harbour, pools, channels and waterfalls: scrolling surface normals, a stronger reflection at shallow viewing angles, colour by depth and restrained shore foam. A has warmer reflections and clearer green-blue shallows; B has deeper blue-green, less transparency and cooler reflections. Ripples and foam match flow, impact or rising and falling water. In A the water of the harbour and of the locks breathes a little out of step with the tide table (one of the three signs); the effect is slow and small. Shoreline and hazardous depth read before richer reflections are added.

### 6.7 Character and lantern

Planned change (6.10): the traveller below is replaced by a slim hooded figure in a long cloak, which stays the same in both moments. The lantern, its light and the posed animation stay.

The traveller is a stylised figure of rounded forms. It keeps the teal coat, the round hood and the small pack of the starting silhouette and stands 1.72 m from the feet to the top of the hood (visual stature only: eye height, body clearance and mantle reach are unchanged). The proportions are those of a picture book: a large hood with a peak, big boots, a coat that hangs from the waist like a bell and a short pack with its bedroll. Round joints separate the parts of a limb, so a pose reads from the camera without an outline. The scarf is the one second colour, a muted rust; a collar covers the chin and two tails stream behind at a run. Belt, pouch, straps and buttons are leather and brass. Cloth is matte, leather slightly smoother, brass a controlled highlight. The same silhouette and materials carry across both moments; the rim of 6.3 separates the figure from sunlit stone and cold ruins.

The figure is posed, not rigged: each leg and arm has two bones, the body comes down as the legs spread so the foot that stands is on the floor, the swinging leg bends its knee, the upper body leans about the waist when running and the left arm swings against the left leg. The right hand holds the lantern and the arm raises it for H. The free left hand holds the small thing that is carried (3.9): a seed bag at the belt, a letter in the fingers. Visual geometry is separate from the collision body.

The lantern is the character's strongest warm accent, so nothing else on the figure is amber or gold. It is one model in both states: a bail ring, a chimney, a brass cap, four iron posts, a glass globe that glows, a flame inside it that wavers, a rim and a foot. In the hand it hangs from the ring a little outside the right hand and sways with the stride. Set down, it hangs from the arm of a small iron post, with the glass at the centre of the circle, 0.45 m over the floor, so the thing set down is plainly the thing carried. Its light is an amber a little deeper than the white-gold of a lit lamp, flickers by 12% and follows the sway; the glow round the glass is 1.0 m across, smaller than a street lamp's, so it does not cut into the walls of a narrow street.

### 6.8 First playable art pass

- Work on the **quay to sea gate** route, with a tight passage, a water edge and the view to the tower. Sky, fog, directional shadows and ambient fill come first, then the core materials and water; district props, plants and character refinements are added only where needed.
- Capture paired A/B views from the same third-person camera positions. Check that stone, timber, plaster and roof tiles read at play distance; the route and the water edge are clear; the tower holds the skyline; and the character and the lantern stand out in both moments. Repeat in greyscale and check the cost on the laptop and the desktop before extending the pass.
- The first frame must show the heap of wreck, the winch of the sea gate, black piles standing in the water where pale posts are only stubs, the three dark lamps of the mast over the tower and, across the harbour, one lit window (7.12) (planned).

The first rendering pass is built: the gradient sky, ground fill, distance and low mist, and since 2026-11-26 the shadows, the lamps, the soft shoulder and the style of the light (6.3). Surface materials, water refinements, the evidence of the storm in A (6.4) and the paired Windows play-test are open. The city is not rebuilt and no post-processing system is added before the reference route proves the visual direction.

### 6.9 One style for the whole town

The town reads as one town and not as one orange because of the rules below: the palette, the shapes and the engine each have one. They are checked by `tools/layout/town_style.py` (the palette) and by `check()` of the model (the built town).

**References, and what each gave.**

| Reference | What the town takes from it |
|---|---|
| Cinque Terre and the Ligurian coast | local earth pigments in lime wash on lime plaster; colour tells one house from the next in a dense row |
| Dubrovnik | one limestone for walls, stairs and streets, weathered to honey; one tile family whose ages make a patchwork; plain houses made important by composition and not by size |
| Koriko of Kiki's Delivery Service | permission to combine: a clock tower and a red-roof silhouette are what the town is known by |
| Environment colour practice | value structure before colour; a dominant family, a secondary and a small accent; tests in greyscale and at thumbnail size |
| Stylised art in general | a strong shape language, a deliberate palette and clear silhouettes carry the style, not surface detail |

**The rules.**

1. **One stone.** `stone`, `stone_dark`, `rock`, `paving`, `sand` and `debris` are one family in hue (69 to 88 degrees) and differ in value: lit stone, shaded masonry (8 L* darker), streets and squares (darker than walls).
2. **Lime wash in earth pigments.** `plaster0` to `plaster7`: cream, yellow ochre, terracotta rose, ivory, pale yellow, salmon, faded sage, dusty blue-grey. Any two differ by Delta E 9 or more. A house has one colour and never that of its neighbour; the hub has all of them, a terrace most of them, the rim cream, ivory, ochre and sage.
3. **One tile, four ages.** `roof`, `roof2`, `roof3`, `roof4`: terracotta, new pale orange, old dark red-brown, faded dusty. Any two differ by 9 or more. The hub has the new, the rim the old.
4. **Accents on purpose.** Shutters (sage, sea blue, oxblood: one colour for a house), awnings (red, blue, mustard), flowers on balconies and pergolas only, verdigris copper for civic domes only. Gold stays the one colour that means use; nothing else is as yellow, as saturated and as light.
5. **Value before colour.** In greyscale the roof is darker than the wall (L* 18 or more in A, 8 in B), the street darker than the lightest wall, shaded masonry darker than lit stone. Measured on the built town: walls L* 80, roofs L* 47, ground L* 73 in A; 61, 29 and 56 in B.
6. **A dominant family, a secondary, a small accent.** The warm family (limestone, lime wash, clay) is most of what is built (65 to 92%), the cool family (leaf, sage, verdigris) 5 to 30%, the accents 0.3 to 10%. Measured on the town: 91%, 9% and 0.7%. The cool family and the accents are at the low end: the next lever is more shutters, awnings and plants, not a new colour.
7. **B is derived, not chosen.** B = tint of the class x (value scale x grey of A) + hue memory x (A - grey of A). The classes (stone, plaster, tile, paint, wood, iron, green, water, gold) have their numbers in `CLASS_B`; the lights (a window dark, a lantern warm, the cold memory light) are set by hand. Hue memory keeps the same houses apart in B (any two lime washes still differ by Delta E 4 or more) and keeps the same place recognisable.
8. **Three tiers of detail.** Tier 1, seen from 50 m or more: the silhouette (roof, tower, dome, cupola). Tier 2, seen at 10 to 30 m: the rhythm of a facade (windows, shutters, balconies, string courses, a bay). Tier 3, seen under 5 m: the sill, the lantern, the step, the sign. Every building has tiers 1 and 2; tier 3 is spent on landmarks, on doors that are entered and on the route.
9. **The town is painted.** Stone, plaster, tile and ground are painted by the style of the light: `TownData::Painted` gives their polygons alpha 1, the shader's `PaintAlbedo` makes the blotches and tall stains (`wash`, default 0.9) and the damp near the water (`damp`, 0.5). Plaster is painted, not coursed. Wood, cloth, paint, metal, light, water and the crowns of trees keep their own colour. `wash = 0` is flat colour.

**B is weathered stone in the moon, not ice.** Five layers of the same cold stack up to a blue town, so the cold belongs to the light and the warmth to the stone. A material of B keeps most of the hue of its A and loses value and chroma, with a tint that is only a little cool (`CLASS_B_AGED`); `B_AGE` in `tools/layout/town_style.py` mixes it with the old rule (`CLASS_B_COLD`), and 0.65 is set (mean b* of the main materials of B: -4.9 cold, +8.7 now, +27.7 in A). The moon is silver (0.92, 0.98, 1.10), the sky light a lavender-grey (0.30, 0.33, 0.43), the sky top (0.16, 0.19, 0.29), the haze a pale grey-blue (0.45, 0.49, 0.56) and thinner (fog and mist 0.0036). The shadows of B still lean blue-violet, the lights are silver, saturation is 1.06, contrast 1.14 and the damp scale 0.8, and the ground bounces a grey (0.21, 0.22, 0.24). The lamps stay amber: they are the one warm thing of B, which is why a lit window can be read from across the town.

**Districts.**

| District | Lime wash | Tile | Houses |
|---|---|---|---|
| Hub: the lower town, the fish market, the market | all eight, colourful, never the neighbour's | new and pale, some old | shops with awnings, rows, altane, a porticoed house on a square |
| Terrace: the west and east lanes, the upper street, the tanner loop | cream, ochre, terracotta, ivory, yellow, salmon, sage | terracotta, old, faded | rows, altane, cottages |
| Rim: the wall lane | ivory, cream, ochre, sage | old, faded | cottages and rows |

**Built.** The palette (43 materials), the rule of B, the painted path of the engine, and the library of buildings in the town (7.10). **Not built:** the town wall and its towers, the theatre, the harbour warehouses and the lighthouse, the bell tower and the plaza of the Rock, the aqueduct and the boxes of the puzzle pieces keep their old shapes in the new colours.

### 6.10 Look development: ink and flat colour, warm against cool

**Status: chosen on 2026-11-28; steps S1, S2a, S2b, S3a, S3b and S3c are built (the hard steps of the sun and the numbers of colour; the maths of the lines and the pass that draws them; the far colours of A; the low sun of A; the sky, the shade and the lamps of A); S2b, S3a, S3b and the sky of S3c were seen on the screen on 2026-11-28 and are to be tuned; the shade, the lamps and the near water of S3c have not been seen; the rain of B (6.11) is decided in principle and not built; the rest is not.** This section is the brief for one sample, the quay steps, and not for the town. The map is not rebuilt until the sample has passed (acceptance below); this is the reference proof that 6.8 asks for, and the one post-processing pass that 6.8 allows before it (the lines). Where 6.3 (the three soft bands of the sun) and 6.7 (the teal coat and the rounded forms) say otherwise, this section wins once the sample is accepted, and those two parts are then rewritten.

**What the play-test showed.** The overview of A is one orange family: ground, walls, roofs, hills and fog are the same hue and close in value, so nothing separates the near from the far. The fog is orange as well, which removes the depth that the sky could give. The ground is one gradient with no breaks, the quay buildings are large plain blocks with no rhythm of windows, eaves or balconies, and B differs from A by hue only. The best view is the market stairs in B: windows, shutters, a cornice, a clock tower and a roofline that tells where one house ends and the next begins. That is the level the rest of the town is held to. The traveller is a figure of spheres and boxes and does not read as a person.

**References, and what each gives.**

| Reference | What the sample takes from it |
|---|---|
| Ink and flat colour (a concept painting made for this brief, kept outside the repository) | A thick outer line and a thin inner line, flat fills in two or three hard steps, sparse hatching in shadow, strong silhouettes, a dark foreground framing a bright middle ground |
| Warm against cool (a second concept painting, kept outside the repository) | Warm light against cool shadow, a sky of layered cloud, a distance that goes paler and cooler, a lamp that is the brightest thing near it |
| Cairn | A hand-drawn, comic-book look led by one art director: the look is designed first and rendered second |
| 6.9 (Ligurian coast, Dubrovnik, Koriko) | Kept: one stone, lime-wash colours, one tile in four ages, tiers of detail |

The concepts are 2D paintings. The sample aims at their structure (value, colour, line, layers, focal point) and not at their density of detail, which real-time rendering does not reach.

**Rules of line and fill.**

1. **Flat fills in hard steps.** The sun falls on a face in two or three steps with hard edges (an edge of one or two pixels of softness, against aliasing). This replaces the three soft bands of 6.3. Everything else of 6.3 stays: the shadow map, the lamps as lights, the rim, the single master (`style`) and the plain view on L.
2. **Two kinds of line, one pass.** A pass over the depth of the picture of each moment draws both, for the town, the props and the traveller alike: the outline (heavier) where a surface ends in front of one behind it, and the inner edge (thinner) where a surface turns by about 40 degrees or more; a gentle turn, as between two facets of terrain, draws nothing. The width is in pixels (scaled with the height of the screen) and does not change with distance; the strength fades, and beyond 50 m only the outline is drawn (tier 1 of 6.9). This replaces an inverted hull for the traveller and the props, which tears at the corners of a hard-edged low-poly mesh, needs a second draw of everything and smooth normals. A hull for the traveller alone is kept in reserve for S5, in case a figure that stands against a wall has no depth step to draw its outline from.
3. **The line is a dark of the local colour, not black.** A: a warm brown-black. B: an indigo-black. Black lines on flat fills would cut the picture into pieces and lose the warmth.
4. **Hatching is a second pass of the sample.** Two directions of fine lines in cast shadow and on shaded faces within 15 m only. It is added only if flat fills and lines are not enough.
5. **Surface paint stays, quieter.** The blotches and stains of 6.9 (rule 9) keep their place at a lower contrast; they must not compete with the lines.
6. **No gradient on a surface** except the sky, glow and fog.

**Colour.** Sampled from the two concept paintings (the dominant colours of the half of each picture) and then set by hand. The values are start values for the sample; they are tuned there and then written into `town_style.py` and `[lighting]`.

| Role | A, the past (calm dusk) | B, twelve years later (moon) |
|---|---|---|
| Lit face | `#F2AA66` peach orange | `#7088A7` silver blue |
| Middle | `#C98859` | `#4F5B74` |
| Shadow face (cool, not just darker) | `#897782` lavender | `#323848` indigo |
| Line | `#2D221B` | `#0F0F14` |
| Warm light (windows, lamps) | amber, weaker than in B | `#C89267`, the only warm in the view |
| Fog, near to far | `#F0C09A` to `#A89AB8` | `#2C3448` to `#46526E` |

Rules:

1. The warm family is the lit side, and the cool family is the shadow and the sky. This is what separates the look from the one-orange town.
2. The distance goes cooler and paler by a hue shift, not by a grey.
3. Value comes before colour, as in 6.9, and is tested in greyscale and at thumbnail size.
4. B is lifted from the concept, in which about 60 to 70% of the pixels are darker than a luma of 25 of 100. In B a walkable surface has a luma of at least 35, the deepest shadow 12 and the line 6, and the route must read in the moon alone (`lamps_b = 0`, 6.3).
5. Accents (shutters, awnings, one flag) are 5 to 10% of what is lit and are the only saturated colours but the lamp.

**Light and sky, in this order.**

1. Tone mapping and grade, because they set the whole colour.
2. The sky: a gradient with two or three bands of cloud and a low sun in A; a moon and torn cloud in B.
3. Fog by height and by distance, in two colours (the near and the far of the table).
4. Ambient occlusion baked into the vertices of the sample with the rays of `TriMesh` (the dark of wall feet, under eaves, in lanes). Large faces are subdivided first, or the bake has no detail.
5. Light probes and a bake for the whole town, only after the geometry is frozen: one set for A and one for B, exchanged by the switch. This is not part of the sample.

A baked light map is not planned: the town is static, and the probes plus the vertex bake give the read at a lower cost. The frame time of the quay view is written down before and after each pass (baseline: 5.6 ms at 2560 x 1494 on the laptop, from the play-test capture) and checked on both machines.

**The lantern in the dusk (decided 2026-11-28).** The lantern stays. Its rule (3.2) does not depend on light, and the puzzles, the tests and the ending are built on it; what failed in A was that nothing in the picture showed it. The remedy is chosen for play, picture and cost together, and is made in this order. (1) The sun of A is lowered to 14 degrees over the horizon (24 before; built 2026-11-28, step S3b: `LIGHT` of `tools/layout/city_model.py`), so that lanes and streets lie in the cool shade of the houses and the sun reaches squares, quays and upper walls; the warm pool of the lantern then reads on cool shade, as a lamp does at dusk. Flat ground is at a cosine of 0.24, in the middle tone of the sun (S1: 0.14 and 0.52), and `check()` of the model fails when it leaves that band; what the lower sun changes is how much stands at full sun (the quay: 57% of its ground at 24 degrees, 17% at 14; the town 37% and 26%) and the length of the shadows (a wall of 4.5 m casts 18 m, 10 before). (2) The lamps of A are lit and rise from 0.55 to 0.9 (`lamps_a`), and the shade is darker (`ambient_a` 0.85, not below 0.8), so that a lamp stands against it (S3c; tuned by a model of the ground and not yet by a picture). (3) The circle is drawn in A (3.2): a ring on the ground and a faint wall of light, which also tells the player what the lantern will hold (S5). (4) Dark places in A (3.2), optional, with the map. Not done, on purpose: replacing the lantern (the mechanic, the 56 checks of tests/test_lantern.cpp, the route proof and the ending would be built again); a lantern whose light has the colour of the other moment (it breaks the one warm accent of 6.7); a lantern that the main route needs in order to be read (6.3).

**The traveller.**

- **Keeps:** the height of 1.72 m to the top of the hood, the lantern model, its light and its amber (6.7), and the free left hand that holds the small thing that is carried (3.9).
- **Silhouette:** a hood with a peak, a cloak to the knee with an uneven hem, a strap and a satchel at the left hip, a belt, fingerless gloves, sturdy boots, the lantern in the right hand at the hip. The face is in the shadow of the hood and has no geometry. The proportions are those of a slim adult (about seven heads), not of a picture book.
- **Colour:** the cloak is blue-grey outside, with a lining of a dull ochre seen at the open front and the hem. 6.7 says that nothing else on the figure is amber, because the lantern is the strongest warm accent: the lining is therefore low in saturation, at least 25 luma points darker than the flame (a start value) and never emissive.
- **The cloak does not change with the layer.** The traveller is the one thing that is the same in both moments, and the world changes around them. A flip of the cloak at the switch looked abrupt, so none is made: no colour or texture is swapped. The light does the work, and the switch is carried by the ripple of the layer (`LayerRipple.h`): as its front crosses the body, the cloak lifts at the hem like a gust (an impulse on the cloak bones, settling in about a third of a second), the flame of the lantern swells and settles, and the grade of the world changes behind the front. Nothing on the body jumps.
- **Build:** one skinned mesh, with a start budget of 4,000 to 6,000 triangles (fixed by the first model). The legs and arms keep the two bones each of 6.7; the cloak adds three chains of three or four bones, driven by the speed, the turn and a wind vector. There is no cloth solver. Animations for the sample: idle, walk, run, lantern raised (H), lantern set down (E) and the switch.
- **Test:** the figure at 64 px high, in greyscale, in both moments, reads as a hooded figure with a lantern.

**The sample: the quay steps.** About 40 m of waterfront with a flight of stone steps rising to one side, a parapet and bollards, one lamp, three houses made of the kit below, the clock tower far off, the sea, a crate and the traveller. It is seen from two cameras: the third-person camera at eye level and an overview from 50 m. A and B are the same geometry. B differs by the changes of the ledger, set by hand here: a roof fallen in on one house, vines on named walls, the lamp unlit and rusted, no awnings or washing. A is the town in use: awnings, washing on a line, pots, lit lamps.

**The kit.** Every part has a tier (6.9, rule 8). Tier 2 (a facade at 10 to 30 m): three window types, shutters, a balcony, a cornice and eaves, a door with a lintel, an awning, a chimney. Tier 3 (under 5 m): a sill, a step, a pot, a lamp bracket, a bollard. Tier 1 (over 50 m): the roof and tower shapes. A house is made of parts and never of a bare box, and the part is the unit that the ledger (3.9) later changes.

**Acceptance of the sample.**

1. A and B from the same two cameras: the near, the middle and the far read as three layers, in colour, in greyscale and at thumbnail size.
2. The traveller reads at 64 px in both moments, and the lantern is the warmest point of the view.
3. In B the route reads with the lamps off.
4. The lines do not crawl or flicker when the camera moves, at the resolution of the play-test (2560 x 1494) and at the native 2560 x 1600 of the laptop.
5. The frame time of the quay view is within the budget set at the first measure, on both machines.
6. The functions of the new light (the hard steps, the line colour) are plain C++ in the shader shim and are covered in `tests/test_style.cpp`; every existing test passes.
7. The author judges the pair of pictures against the two concepts and decides whether the town may be rebuilt in this look.

**Order of work.** Each step ends with a pair of screenshots (A and B, the same camera) and waits for approval; no step is begun before the last is approved.

| Step | Work | Size |
|---|---|---|
| S1 | The numbers of colour, hard steps and the plain view, on the present quay (built 2026-11-28) | small |
| S2 | The two lines: a, the maths and its test (built 2026-11-28); b, the pass over the depth of each moment (built 2026-11-28, seen) | medium |
| S3 | Sky, fog and grade; the low sun of A and its lamps (the lantern in the dusk, below): a, the far colours of A (built 2026-11-28, seen); b, the low sun of A (built 2026-11-28, seen); c, the sky, the shade, the lamps and the near water of A (built 2026-11-28; the sky is seen, the rest is not); d, the grade, and A made cooler by its numbers | medium |
| S3e | The rain of B (6.11): R1 the look, R2 the reading, R3 light in rain and the running water | medium |
| S4 | The kit and the dressing of the quay corner | large |
| S5 | The traveller and the lantern (mesh, cloak bones, the switch, the ring of the lantern set down in A) | large |
| S6 | Vertex AO for the sample | small |
| S7 | The A and B pair against the acceptance list, and the decision on the map | small |

**S1, as built (2026-11-28).** The sun falls on a face in three tones: none of it below a cosine of 0.14, 0.46 of it from 0.14, all of it from 0.52 (`ToonRampEdge` in `shaders/lighting.hlsli`). The edge of a step is `toon_edge` pixels (1.5) of the change of the cosine from one pixel to the next, so that a flat face has a hard step and a round thing an edge as soft as a drawn line; `toon` is 1. The plain view of the key L is unchanged (all amounts 0). The numbers of colour: the shadow of A is a little lighter (`cool` 0.84, 0.97, 1.26), the moon of B is 0.70 and its sky light 1.2 (`sun_b`, `ambient_b` in `config/default.ini`) and its contrast 1.06. They are held by `tests/test_style.cpp` (`TestLook`), which computes a flat face from the `LIGHT` of the model and the palette with the settings of `config/default.ini`: in A the sunny wall of limestone is at L* 80, the ground 71 and the shadow 42, and the shadow leans cool; in B the open ground is at L* 59, in a shadow 41, and the deepest dark 12.6. These values are computed and not yet measured on the screen: the two screenshots of S1 (A and B, the same camera) are the measure.

**Found in S1, taken in S3.** The lit stone of B is a neutral grey and not silver-blue: its palette is a warm grey (B_AGE 0.65) under a pale blue moon, and the split-tone grade of 6.3 acts only on the darks (luma below 0.30) and the lights (above it), so that the middle of B, at about 0.27, has almost none of it. The hue of B is a question of the colour of the moon and the sky (`LIGHT` of the model, which needs an export) and of the albedo of B (`town_style.py`), and is taken with the sky, the fog and the grade in S3.

**S2, step a: the maths of the lines (2026-11-28).** `shaders/lines.hlsli` finds the lines from the depth of a picture, one pixel and its four neighbours. The outline is where a pixel stands in front of its neighbours by more than a plane can explain (a ridge of 1 / z, a second difference that is zero on any plane at any distance and slant) and is on the near side only. The inner edge is where the four triangles that meet at a pixel turn by 30 degrees or more (full 15 degrees later, by the setting; measured on the test scene a turn of 50 degrees is drawn and one of 20 is not, because a turn is seen across two pixels). The pixels on the border of the picture have no line. `src/render/LineSettings.h` and `[lines]` of `config/default.ini` hold the numbers (`amount`, `width` 2 px and `crease_width` 1.2 px at 1080 high, `silhouette`, `crease`, `fade_near`, `fade_far`, `far_alpha`, `local`), and `MakeLines` the registers; the plain view of L has no line. `TestLines` in `tests/test_style.cpp` casts a house, a crate, a pole and a ramp into a depth buffer and checks the lines on it: none on a plane (the ground to the far wall) or inside a face; the outline on the pixel of the house and never on the one of the wall beside it; an inner edge at the foot of a wall and on a crate; a change of slope of 12 and 20 degrees draws nothing and one of 50 degrees or more does; the fade, the width, the colour (a dark of the local colour), the settings, the registers and the shipped file. The pass is step b, below.

**S2, step b: the pass (2026-11-28).** One full-screen pass per finished picture (the screen, and each of the two pictures of the ripple), after everything opaque and before the water, so that the water is blended over the lines and the lines of a sea bed are as faint as the bed (`shaders/lines.hlsl`, `Models::DrawLines`). It reads the depth buffer (`Device` and the ripple pictures make it typeless and readable), looks at the square of pixels that the width of a line can reach, takes the largest `LineCoverage` and blends the ink with ONE, INV_SRC_ALPHA, which is `LineComposite` exactly (`LineBlendColor`, `LineBlendAlpha` in `shaders/lines_blend.hlsli`): no copy of the picture and no second draw. The plain view of L has no line. `PassCoverage` in `tests/test_style.cpp` is the pass written again in C++, run on the test scene with the registers that `MakeLines` gives. Limits taken knowingly: the water, the halos and the sky are not in the depth (the lines under water are those of the bed); trees and terrain facets that turn by 30 degrees or more draw inner edges up to 40 to 70 m, and the outlines thin to 0.3 of their strength at 350 m, so that the far hills stay pale: numbers to tune on the screen (`crease`, `fade_near`, `fade_far`, `far_alpha`). **Seen on 2026-11-28** on the quay and the overview of A and of B: the outlines and the edges of the buildings, the traveller and the props are drawn. The dashes along the streets are decals of the model (kerb and wayfinding stones) and not lines. Still to do: the plain view for comparison, the cost of the pass (every title bar sits at 5.6 ms, which is probably the display and not the game) and the width on the far hills (KI-21).

**S3, step a: the far colours of A (2026-11-28).** The play-test overview of A, measured, was one family: 98% of its coloured pixels red-orange or orange-yellow, 0.1% cooler than neutral, L* 34 to 81, and the far hills warmer than the near ground. The horizon of the model is orange, and no multiplier makes it lavender, so the far haze now leans to a hue (`farTint`, from 24 m to 200 m, its brightness kept): lavender in A, the hue of its own horizon in B. More fog far away (`StyleFarFog`, from 100 m to 280 m, `far_fog` in `[lighting]`, 0.5 in the look of A and none in B) makes the far hills and the sea pale. The contrast of A is 1.14. Computed (`TestLook`, a lit hillside over the horizon of the model): blue over red 0.30 near, 0.43 at 130 m, 0.83 at 250 m; chroma 0.70, 0.57, 0.30; L* 85 at 250 m. Seen: the far land of A is pale pink-lavender over the warm town.

**S3, step b: the low sun of A (2026-11-28).** The sun of A is 14 degrees over the horizon (24 before), from the same side (`sun_toward`, `A_SUN_ELEVATION` and `A_SUN_FROM` in `tools/layout/city_model.py`; the town is exported again). Because the sun falls in three hard tones, a flat sunlit surface is in the middle tone for any sun between 8 and 31 degrees, so the lower sun does not dim flat ground; it takes the full tone from more of the roofs and walls (a roof of 30 degrees must face within 50 degrees of the sun instead of 68) and gives long shadows (a wall of 4.5 m casts 18 m, 10 before). `check()` of the model says the height of the sun and fails when the cosine of flat ground leaves 0.17 to 0.50. `TestLook` reads `A_SUN_ELEVATION` from the model and fails when its copy of the sun differs. Seen on the pictures after it, measured on the overview of A: the darkest 1% from L* 34 to 24, the pixels cooler than neutral from 0.1% to 1.4%; the long shadows and the lit lamps read well.

**S3, step c: the sky, the shade and the lamps of A (2026-11-28).** The overview of A had a band of orange sky (61% of its top rows, red over blue 1.7) over a far land that is lavender (1.04 to 1.08): the sky was drawn from the horizon colour of the model and did not take the lean of the far distance. `StyleSky` leans the gradient of the sky to the hue of the far distance, all of it at the horizon and below and none from an altitude of 0.55 up (`SkyHorizonWeight`), so that the horizon of the sky is the colour that the far land fades to (and the sky overhead is as it was); the glow of the sun is not leant. Computed: the horizon of the sky of A has blue over red 1.01 where it had 0.24, like the far land at 3 km, and is brighter, L* 87 against 80, as the far land is. B leans to its own hue. The shade of A is darker (`ambient_a` 0.85; not below 0.8, which would take the shaded wall of `TestLook` under L* 36) and the lamps of A are stronger (`lamps_a` 0.9), so that a lamp stands against the shade. Seen after it (2026-11-28, measured on the overview of A): the band of orange sky is gone (61.5% of the top rows before, 0.3% after; red over blue of the sky, in the values of the picture, 1.7 before and 1.08 after, like the far land at 1.02 to 1.08).

**S3, step c, the near water (2026-11-28).** `WaterPS` reflected the horizon colour of the model (`fogColor`) directly, so the sea near the quay of A stayed peach under a lavender sky. `StyleWaterSky` in `shaders/lighting.hlsli` gives the reflection the lean of the sky: the horizon of the water is the horizon of the sky, and higher up the reflection is the sky light as it was; in the plain view it is the formula that the water had. Computed: blue over red 1.01 at the horizon of the sea of A where it was 0.24, the same as the sky and the far land; B leans to its own hue. **Not seen yet** (KI-21): the shade, the lamps and the near water of A. **Not done in S3:** the lantern in the hand in A; the grade.

After the sample: the map is redesigned on paper and approved, then rebuilt district by district (the harbour first), then the baked occlusion and the probes, and last the ledger is put into the new map (section 12).

**Open, with the default taken.** The line is a dark of the local colour and not black. Hatching is off until S2 shows it is needed. B is lifted to the luma numbers above and not left as dark as the concept.

### 6.11 Rain in B

**Status: decided in principle on 2026-11-28. None of the rain is built; the only step of this section that is built is the near water of A (S3c, below).** The numbers are start values for the pictures of R1.

**Why.** Three reasons, in order of weight.

1. **The two moments are told apart by more than hue.** A is the held evening: a warm low sun, the streets in shade, lit lamps, still air and a flat sea. B is the present, and it moves. One is warm and still, the other cool and in motion: they differ in colour, in light, in motion and in sound, so that the difference still reads in greyscale and at thumbnail size (principle 10), and A may go cooler in its shade and its sky (S3d) without becoming B.
2. **Time.** The lantern holds the evening still and A has no clock (1). Rain is the weather of a world that went on, and B is where time passes.
3. **The ledger gets a picture.** The roof is one of the five facts of a place (3.9), and 3.9 already says that a dripping eave marks the open. In rain the open is wet and a roofed place is dry, and the player reads it without a word.

**Why A is not made a sunless blue hour.** A sun below the horizon takes away the three tones of the sun, the long shadows and the warm key light that the look of A stands on (6.10, S1, S3b), and it makes both moments blue. A stays the last light: the sun 14 degrees over the horizon, the streets in shade, a sky that is blue overhead and lavender at the horizon, the lamps lit. A is made cooler by its numbers (the shade, the sky and the ground: S3d), not by taking the sun away. The rain does the rest of the work of telling the pair apart.

**What the rain is.** Ink and not particles: it is drawn as a comic draws rain, in the line of 6.10.

- Thin pale strokes in three layers (near, middle, far: other length, speed and density), slanted along one wind, drawn as a pass over the picture of B only. A has none. In the ripple only the picture of B has it, so a switch brings the rain in at the front of the ring.
- Small ink rings where drops fall on wet ground near the traveller, found from the depth of the picture: fewer farther away and none beyond about 25 m.
- A wet look for B: horizontal faces a little darker with a faint sheen, the fog a little thicker, the sky overcast with the moon as a soft glow and no disc.
- The plain view (L) has none. A master, `amount` of `[rain]` (0: none), and the other numbers are in the config.

**The reading.** The rain falls along the direction of the moonlight (a driving rain from the sea side; the slant is open, below). Then the shadow map of the moon is the shelter map of the rain, with no new pass: a surface that the moon does not reach, the rain does not reach.

- *R2.* Wet sheen and rings only where the moon reaches: dry under roofs and arches and in the lee of walls. Streaks are drawn only in open air (the shadow map is walked along the ray, as the light shafts do). A line of drops runs along the edge of every eave.
- *The circle of the lantern is dry.* In B the inside of the circle is A (3.2): no rain, nothing wet, the dusk. A warm, still bubble in the rain is the picture of the circle from far away, and it needs nothing but not drawing rain inside it.
- *R5, when the map is rebuilt.* The model writes the covered ground of the town as data, with the roof fact of 3.9, and replaces the shadow trick (the shadow map reaches only 60 m round the camera).

**Light in rain.** Wet air holds light. The halos of the lamps are stronger in B (x1.3 to start), the shafts of the moon and of the lamps show the rain, and a lit window is the warmest, smallest thing in the view (10.7). The lamps of the mast and the lit window of the first frame read better in rain and not worse.

**What the rain never does.**

| It does not | Because |
|---|---|
| fill a dry bed, lift silt, rot timber or grow a seed | B is computed from A by the table of 3.9 and by nothing else, and a rule is added only after a puzzle needs it. The dry bed of the canal crate stays dry: the rain runs off into the drains as a sheen and is never a depth |
| make stone slippery, make the traveller wade or wet the cloak | principles 3 and 9: nothing for the hands to learn |
| come and go, gather, or become a storm that the player waits out | principle 3: no timer on the main path. The rain is steady. The storm is shown once, in the ending (5.3), and is another thing: a wall of cloud and the sea |
| put out a lamp or the flame of the lantern | the lamps of B are the one warm thing of it, and the flame is given away only in the belfry (5.2) |
| fall in A | A is the held evening |
| know what the traveller did (thinner over a kept house) | principle 11: nothing acts for the sake of a puzzle. The kept look is told by windows and smoke |

**Uses that are not rules (optional, later).**

- *Silence marks shelter (R4, with the sound of 10.8).* Rain has three sounds, on stone, on water and on timber, and none under a roof. A closed room can be found by ear, and a gap in B that is a wall in A (the template Hidden way, 8.2) is a rope of falling water in the dark, seen by the light of the lantern.
- *Running water reads the slope (R3).* Rain runs in thin ink rills down streets and stairs to the harbour and the drains, so the player reads where the town drains and, later, what a sluice held in A.
- *The morning.* The sweep of the ending leaves a clear sky behind its ring, and the rain stops. 6.4 says that the first morning is the first sun of the game: the whole game is under rain, and the ending is the first dry day.

**Stages.** Each ends with a pair of pictures (A and B from the same camera) and waits for approval.

| Stage | Work |
|---|---|
| R1 | the look: the three layers of streaks, the rings, the wet look, the overcast sky of B, the dry circle, the master and the numbers of `[rain]`, and tests of the maths of the streaks and the rings (plain HLSL, as the lines) |
| R2 | the reading: the shelter from the shadow map of the moon, the eaves, the dry ground |
| R3 | light in rain (the halos, the shafts) and the running water |
| R4 | the sound of rain, with the sound of 10.8 |
| R5 | the covered ground as data from the model, with the rebuilt map |

**Acceptance of R1.**

1. A and B from the same camera, in greyscale and at thumbnail size, are told apart at once by a person who has not seen them (by rain and brightness, not only by hue).
2. The three lamps of the mast stay in view for every lens and screen of `tests/test_firstminute.cpp`, and the ink lines are not buried.
3. The route reads in the moon alone: the numbers of 6.10, rule 4, hold with the rain on.
4. The streaks do not crawl, flicker or shimmer as the camera turns, and the pass is within a budget that is set when the frame rate is no longer capped (KI-21).
5. The plain view has none, and an `amount` of 0 is the picture as it was.

**Open.** The slant: the moon is 53 degrees up, so rain along it is 37 degrees from the vertical, a driving rain, and the moon may go lower. The density, whether the heights have it lighter, how far the rings go, and whether the clear sky of the ending is worth its cost.

## 7. The map

### 7.1 The model

The layout comes from one model, `tools/layout/city_model.py` (Python 3, numpy, matplotlib). It draws both moments from over the sea, from the cliff and from the street, and a plan with the main path:

```text
python tools/layout/city_model.py          every view, into docs/images/city2_*.png, and the check
python tools/layout/city_model.py check    only the check of its numbers against the limits of section 11
python tools/layout/city_model.py plan     one view: aerial_a, aerial_b, street_a, street_b, vista_a, vista_b, plan
python tools/layout/city_model.py export   the check, then data/town.dlt
```

The check measures every street against its slope limit and every joint of the main path (a street onto the bastion, the quays, the graveyard, the top of the theatre) for a step; it reports nothing on the current model. The tests walk every street with the real body (`tests/test_town.cpp`): in A every street is open; in B fallen arches and the rubble of the sea gate close two of them, to be passed by a jump or by A. The town loads the model's terrain, streets and buildings as data; the model is updated and the town re-exported together.

The model will also write what guidance and the map need (planned, format 5 of the town file): the goals of the main path, each with its district and what it needs; the beacons (10.2); the favours with their people and the ageing table (3.9, 4); and a plan of both moments (10.4). Today the file carries only `steps`, one line of nine goals.

### 7.2 One hill town

| Place | Height (m) | Idea |
|---|---|---|
| The harbour | -6 (the quay) | the start; quays, a lighthouse, the sea gate |
| The lower town | -3 .. 0 | narrow streets under arches; the hub, where the three stairs part |
| The cistern and the gorge | -10 .. -6 | under the centre: the well, a tunnel, the floor of the gorge; water in A, walked in B |
| West: the market and the gate | 0 .. 6, the bastion 14 | goes down: the great well to the cistern; the scaffold; the garden terraces; ends at the signal house of the gate of the bastion, where the gate warden keeps his post, with the Rock in view |
| Centre: the Rock of the Bell | 14 | a round plaza on a rock in a gorge 20 m deep; the church and the tower; reached by the link of each district; the finale |
| East: the locks | 3 .. 21 | climbs: five locks to the water tower, where the lockmaster keeps his post; entered by a stair from the lower town (to model: now it is entered from the Rock by the east stair) |
| North: the upper street and the theatre | 21 .. 30 | high: the street ends at the top row of the theatre, which faces the Rock; entered by its own stair from the lower town (to model: now it starts at the top lock) |
| The cliff: the graveyard | 42 | the view over everything; the watch post, where the watch captain keeps his post; the cliff way to the Rock (to design) |

### 7.3 Rules

- **A hub, three ways and a rock.** The lower town is the hub: three stairs part there, one to each district, each marked by its emblem (10.5). Each way climbs, drops or crosses in its own manner and ends at its keeper with the tower in view. The ways are joined by the Rock, not by each other: a way's last puzzle opens its link to the plaza.
- **Equal ceilings.** The three ways have the same number of beats (three, then a keeper) and reach the same difficulty at their last beat, because any one of them may be a player's first or last.
- **Seen first, reached from an unexpected direction.** The turns of the route come from the main path itself, not from shortcuts.
- **Three heights in every view:** streets on several levels with bridges between, roofs and arches over the street, the cistern under it.
- **Each district one idea:** west down, east up, north high, the centre across gaps.
- **The moments change the route**, not only the look: bridges, the aqueduct and water in A; the fallen bridges, dry beds and broken walls of B.
- **Narrow, then wide:** each district opens with a lane or a tunnel and ends at a view, and each opens with a framed view of the tower.
- **A town, not boxes:** houses turned to their streets; the damage by height as in 1.1; in B the town has let go (grass through the paving, figs in roofless houses, scrub over the groves, reeds in the harbour) and on the heights a few people stayed, with smoke, washing and a lit window; nothing floats; the ground is under every floor.

### 7.4 The main path

After the harbour the town opens: three ways, then the Rock.

| Stage | Where | What the player does | Ends with | State |
|---|---|---|---|---|
| 1. Teaching ground | the quay, the sea gate, the lower town (7.12) | the switch, the causal rule, the ledger (growth, iron, rot and keeping), the lantern, the switch in the air (section 8) | the three gates of the lower town open; the tower with its three dark lamps stays in view | built, except the seed, the colour of timber, the lantern's demonstration and the vista |
| 2. West way | up the market stairs, the market, the scaffold, the garden terraces, the bastion gate | the seed and the terrace; the scaffold that stands because it is sheltered; the gardener and the vine; the weather glass hung again (8.3) | the gate warden and his key; the link to the Rock: the wreck of the west bridge (B), the gorge, the lift | market, scaffold, wreck and lift built; the seed, the gardener and the warden are not modelled |
| 3. East way | a stair from the lower town to the first lock, up the locks to the water tower | the ballast crate and the dry bed; the tarred plank between two roofs; the third lock and the barge (8.3) | the lockmaster and his key; the link: the east stair and bridge to the Rock | the locks and the plank exist; the stair from the lower town is not modelled |
| 4. North way | a stair from the lower town to the upper street, the theatre, the cliff stair | the gatehouse crate and the aerial switch; the prop of the side arch; the awning of the theatre lowered (8.3) | the watch captain and his key; the link: the cliff way to the Rock (to design) | the gatehouse crate exists; the stair from the lower town is not modelled |
| 5. The Rock | the link of the district finished last, the plaza | the tower opens with the three keys; the climb of three floors and the last flight; the belfry (section 5) | the bell, the sweep of twelve years and the walk (5.3, 5.4) | lift and plaza built; the inside of the tower to design |

Rules of the order:

- The three ways open together, when the teaching ground is done, and are taken in any order. Each is complete from the lower town alone: no puzzle of a way needs anything of another, and nothing done in one changes what another asks.
- The return from a keeper to the lower town is the one walk back this design accepts. It is not a puzzle, it sends the player to no other district, and it should take about 20 s at most and show a new view or open a shortcut when the key is taken (a rope, a chute, a lift).
- A link is a connection, not a key. A link opened early may let a player enter another way from the plaza, never past a puzzle of it; a viewpoint alone may be reached early.

### 7.5 The model checked

Fixed on 2026-11-24 and kept true by `city_model.py check`:

- **Streets:** each starts and ends at the height of what it meets (an earlier street, the bastion, a quay, the graveyard) and between is half the ground and half an even climb; never steeper than its kind: streets and lanes 24 degrees, stairs 32.
- **West:** the market street climbs to the bastion (14 m); a gate tower on the bastion frames the bridge.
- **East of the Rock:** the east stair goes down from the east bastion (14 m) to the first quay of the locks (3 m); a stair beside each lock head on both quays, 4.5 m over 8 m (29 degrees); the lock heads hold a fall of water in A.
- **North:** the upper street runs from the top quay (21 m) to the top row of the theatre (30 m); the cliff stair climbs straight to the graveyard (42 m) through a gate in the town wall.
- **Harbour and market:** the sea stair goes through the middle of the sea gate; the market stairs pass west of the well, clear of its parapet.
- **Centre:** the tunnel from the well opens into the gorge under the west bridge (under the water of A); the lift (in A the cage at the plaza, in B its frame leaning up the face of the Rock); the west bridge (its middle in A only, its gate at the Rock end shut in A and open in ruin in B).
- **The lantern:** the `street_b` view shows its circle on the west bridge.
- **The order:** the model is one line today (the west way leads to the Rock, the Rock to the locks, the locks to the street); the check does not yet test the three ways (section 11).
- **Still open:** the stairs from the lower town to the east locks and to the upper street, the links to the plaza, and the walkable geometry from the plaza into the tower (section 5).

### 7.6 Compact and connected town

The complete town and its winding climb stay, at about 60% of the current explorable footprint, by shortening empty approaches and not by scaling every slope or jump. A sprint of about 20 s without a decision, a discovery or a useful landmark is a candidate for compression, not a hard rule. The tower stays the skyline anchor. Visible doors, terraces and roofs have an approach or clearly read as scenery; a rewarding-looking doorway is never an invisible boundary.

- Different routes reveal different information. The main street establishes the destination, roofs reveal district connections, and the gorge explains how the damage changed movement. A branch that only duplicates the main route is shortened or removed.
- Familiar space is reconnected while exploration is active. A local shortcut is useful when it changes the next choice, reveals the vertical structure or removes an immediate repeat walk.
- Optional exploration starts with a visible spatial question and ends with an answer: a grate, a broken bridge, an unreachable terrace or a route seen from above invites; the payoff is a connection, a view, a lit window or a visible result. No mandatory collectible justifies a branch.
- One spatial rule is taught at a time: first the safe route or consequence, then the same rule in a less obvious form. New geometry combines known rules and adds no new verb.

### 7.7 The fabric of the town: fronts, blocks and a reason for every place

A real hill town (Dubrovnik, the villages of the Ligurian coast) is made of streets, and the houses are what stands along them: a house has a front, the fronts form rows, the rows share side walls, and what lies behind is a yard, a garden or a terrace. The places remembered in games work the same way: a wall of Stormveil is a place that can be walked to, and what is seen from far is reached later.

- **A house has a front.** Every house stands on a front (a street, a lane, a lock quay or the edge of the market plaza), its door to it and its side walls shared with the next. The check fails when a door is more than 0.9 m from its front.
- **Two deep at most.** A block has one row of houses on each front; what lies behind is a yard, a garden or a terrace, and a second row needs a back lane.
- **Open ground has a use.** Ground that no house, street or puzzle uses is a garden: olives in A and scrub in B on the gentle terraces, net racks and drying yards where the fishermen live.
- **Every place that can be reached has a reason:** a puzzle, a view, a piece of the story, a favour, or a connection that changes the next choice (7.6).
- **What looks reachable is reachable, or reads as scenery.** A terrace behind a district wall is a garden seen over the wall.
- **No lane opens a way round a puzzle.** A new lane joins the town only inside one way, and `tests/test_route.cpp` proves that no handle is reached before its step. A lane up a terraced slope needs a long, gentle bank beside it, and a gentle bank is a ramp: the lane is closed by a district wall where the next terrace belongs to another way.
- **The mast stays in sight.** No house stands in the sight lines from the quay to the three lamps of the mast (10.1); `tests/test_firstminute.cpp` casts the rays.

**Built:** the rule of the front, about 55 houses on fronts, the garden lane (a cistern and a terrace), the mill lane (a terrace), the houses round the market plaza, the orchards and the net racks. **Not built:** back lanes with second rows.

### 7.8 The street network: blocks, squares and dressed walls

The town is made of blocks, squares and dressed walls. The references are the hill towns of the Ligurian and Dalmatian coast (lanes that follow the contour, stairs that join them, passages under arches, a square at every junction of consequence), the lower town of Dubrovnik (two rows of houses back to back between parallel lanes) and the way Stormveil, the Undead Burg and Kakariko tie every district to a loop, a landmark and a view. Kevin Lynch's five elements (paths, nodes, landmarks, districts, edges) are the checklist.

- **Blocks, not rows.** Parallel lanes are 12 to 15 m apart and joined by passages every 12 to 17 m. A lane that does not join another ends in a loop, a square or a view.
- **Every district has a node, a landmark and an edge:** a square or a terrace; something seen from far (the chapel, the market hall, the clock tower); a wall or a stair.
- **Density falls from the hub to the rim:** rows back to back in the lower town, houses stepped on the terraces, gardens and olives against the wall.
- **A blank wall is not a street.** The walls of a corridor are the fronts of the houses along it: two rows of windows with sills and shutters, doors at the foot, a cornice and eaves at the top. Everything on a wall is flat on it or above the top: the puzzles of the corridors rely on the walls being sheer.
- **A yard has something in it:** a shed, a woodpile, a well-head, a cart or an arbour; at the doors barrels, crates and benches, never on the lane.
- **A new lane stays inside its district** and never opens a wall of a corridor.

**Built:** cooper lane with four passages and stone gates; the fish market (a fountain: water in A, a dry basin in B; three stalls); the weaver lane and its green; the vine lane with the chapel of the terraces; the tanner loop; the wall lane and its terrace; dressed corridor fronts, yards, plots, olives, figs and cypresses. **Not built:** arches that join houses over a lane, second rows on back lanes, a lane along the inside of the wall.

### 7.9 Rooms, windows and puzzles in the world

**Rooms.** Ten houses and a chapel can be entered. Each has a door in a street front 1.3 m wide, a room of at least 4.8 x 5.6 m with a ceiling at 3.4 m, a lamp, a roof in A and the open sky in B, and one idea told by objects and never by text: the fishmonger's table of an interrupted dinner, the cooper's barrels, the weaver's loom, the miller's sacks, the watch room with its telescope, the child's room, the baker's oven, the scribe's model of the town, the lamplighter's lanterns on chains, the sailmaker's sails. A floor higher than the ground at the door gets a ramp of up to 0.35 m. No lamp post stands within 1.7 m of a door, no furniture in the line from the door to the middle of the room, and a room has no second way out. `tests/test_town.cpp` walks from the street through every door to the middle of every room in A and in B. No room stands in the west yet.

**Windows of the two moments.** A window frames something that differs between the moments and belongs to a way. Two are cut: the watch room looks over the town at the theatre (columns and an awning in A, stumps in B), and the miller's room looks across the quay at the locks (full of water in A, dry in B). A window has iron bars with gaps of 0.14 m, so it is never a way in or out, and a sill at 0.95 m; it is cut only where the rays are clear in both moments.

**One look for what can be used.** In A everything that can be used has a gold cap, grip or ring, dull iron in B. Soil that takes a seed is dark and loose with a gold ring. The rule: if it can be used, it is gold.

**Why each puzzle is where it is.** Everything is flat on walls, at the sides, or over 3.5 m: nothing stands on a way that a puzzle uses.

| Puzzle | The reason in the world | What says it |
|---|---|---|
| Sea gate | the customs shut the harbour at night | a chain from the winch up to the head of the portcullis; a lamp over the winch; the cargo behind the gate (A) |
| Canal crate | the crate is part of the customs cargo loaded across the canal | crates, barrels and sacks along both walls of the quay; a crane arm with a rope and a hook; three marks of a sledge pointing at the canal (A) |
| Plank | roofers are repairing the roof across the yard | tiles, trestles and a bucket on the first roof; bare battens and new tiles on the neighbour's roof (A) |
| Gatehouse crate | goods are hoisted into the gatehouse through a loading door | a loading door over the balcony, a hoist beam with a rope, the cart that brought the crate (A; on its side in B) |
| The tower door | the tower is locked for the night by three officers, each with his own lock | three gold-capped locks in the door, marked with the three emblems |

**Life.** Washing on a line over the passages and lanes (A), carts, sacks and barrels at the doors, a boat on its stocks, a rope walk, net racks, a quiet garden with a pergola. B has the ribs of the boat, the posts of the shed and the posts of the pergola.

**What the references still have that this town lacks.** A second entrance for a room, and no more than ten rooms; arches that join houses over a lane; a walk along the town wall (a scenic loop, but a way round the puzzles unless it is sealed); a room in the west; windows in the harbour and the west.

### 7.10 Buildings, and an open town with thresholds

**Built: a library of buildings** (`tools/layout/architecture.py`, checked by `architecture_check.py`, 494 checks, drawn by `architecture_preview.py`), used by the model. A building is a function of a frame and its sizes and writes polygons through an emitter, so it needs nothing of the model.

| House | Width x depth (m) | Storeys | What it has |
|---|---|---|---|
| row | 5.4 - 7.4 x 6.8 - 8.6 | 2 | string course, windows with sills and shutters, a door with a step and a lantern, a balcony or a bay, a dormer, a chimney |
| altana | 5.8 - 7.4 x 7.0 - 8.8 | 2 | the same, with a wooden loggia on the roof |
| shop | 6.2 - 8.4 x 7.0 - 9.0 | 2 (3 from 7.6 m) | a wide arched shop front with a counter, an awning (A) and a sign |
| portico | 7.0 - 9.0 x 8.0 - 9.6 | 2 | a covered porch of two round arches |
| cottage | 5.0 - 6.6 x 5.2 - 6.6 | 1 | a steep roof, low eaves at 3.4 m, a pergola with a vine |
| palazzetto | 8.6 - 11.4 x 8.4 - 10.0 | 3 | corner stones, a round-arched door, a cornice, a hip roof |

A house is never higher than 1.25 times its width. In B a house is whole, unroofed or a ruin, as the model's `damage()` says. Windows, shutters, trims and signs are decals: nothing to collide with or climb. The landmarks are the market hall (15.0 x 9.4 m, an arcade that can be walked under, a cupola), the clock tower (5.6 x 5.6 m, 30 m high, a round arch the street runs through), the water tower with its aqueduct (a twelve-sided drum, an arcade of seven arches), the chapel (can be entered) and the customs house (26 x 9 m, an arcade of seven arches toward the quay). The check found a balcony corbel at 2.95 m and a cottage eave at 2.6 m that were ways onto roofs, and both were fixed: no upward face of a solid between 1.2 m and 3.0 m stands out of a house by more than 0.3 m.

**Decided (2026-11-27): an open town with shut thresholds.** The town is built as a town first, and the puzzles are thresholds in it.

- **No street is a tunnel.** There is no wall of a corridor. Both sides of every street are real rows of houses, with side passages and courtyards.
- **A district is closed by what a hill town is closed by:** the town wall, the cliff, the gorge, water, terrace walls of 4.5 m (`district_wall()`) and the backs of continuous rows. The line that closes a district has one crossing: the threshold.
- **A threshold is something a town has:** a gate, a bridge, a stair with a collapsed flight, a canal with a quay, a passage under a house with a shut door, a gatehouse, a lift. The puzzle works on the threshold and on nothing else.
- **Inside a district everything open is a place with a use:** a square, a lane that ends in a court, a shop room, a terrace with a view. Nothing is empty and nothing leads round.
- **The route solver judges, as before:** no handle is reached before its step. It also counts the cells that can be reached before each step (67,837 before the sea gate and 100,097 before the canal crate in the last run); the aim is that each stage opens at least a quarter more than the one before, and that everything in a district can be reached before its threshold.

| Puzzle | Now | Becomes |
|---|---|---|
| Sea gate | a portcullis in the harbour wall | stays; two gate towers and the customs house beside it, whose arcade is the first place to explore |
| Air step (lower street) | walls on both sides, a balcony, a heap | a house across the street with a passage under it, its front the balcony, in B the corner fallen |
| Canal crate (market stairs) | a corridor and a canal | the customs canal crosses the stair at a quay (cargo, a crane, a footbridge), houses on both banks with side doors and a court |
| Scaffold (market) | a scaffold on a terrace | the market hall and the clock tower with its scaffold; the lamp house beside it |
| Plank | two roofs across a street | the same, with the roofs of the library (6.5 m and 9.5 m); the roofers; no corridor |
| Gatehouse crate | a gatehouse with a balcony | a gatehouse that straddles the street with a loading door; the quarter closed by rows and the wall lane |
| Lift and bridges | the gorge | stay |

Risk: each change of the corridors is a possible new leak, so every step is run through `tests/test_route.cpp` and `tests/test_things.cpp`. Covered streets (the closed streets stay, under continuous houses with arches and upper floors over the street, as in Dubrovnik) are the cheap fallback for the one or two streets that cannot avoid a long closure. **Not built:** everything under "Decided"; the front of the theatre, the town wall and its towers keep their old shapes.

### 7.11 The town as a place to play: reasons to go

**The town in numbers** (the model): 23 streets, lanes and stairs, 840 m (558 m of lanes, 173 m of streets, 109 m of stairs); the body walks 5.5 m/s (8.8 sprinting) and jumps 2.0 m, so the whole network is about two and a half minutes in a straight line, and a beat every 25 m is a beat every 4.5 s. 15 dead ends and 6 independent loops; five streets closed by their puzzle for 106 m between walls of 9.5 m; 9 puzzles; 10 rooms and 1 chapel; 1 court; 11 places that reveal the map when stood on. Inside the wall (17,800 m2 without the gorge) the houses cover 10.7%, and 29% of the ground is more than 6 m from any house and any street. The town is small: it has to be dense, and what is in it has to be worth a detour.

**The rules.**

- **R1. A landmark before the way to it.** Every junction shows at least one landmark that cannot be reached yet; every landmark is seen from three places before it is reached. The tower and the mast are seen from the first frame, the clock tower from the market stairs, the chapel's bell-cote from the whole west, the water tower from the locks, the theatre from the upper street, the graveyard from the plaza.
- **R2. A beat every 25 m.** A beat is a puzzle, a room, a court, a view, a favour to do, a vignette that differs between A and B, a shortcut or a secret. A stretch of 50 m of lane with none is a bug.
- **R3. A dead end ends in something.** Of the 15 dead ends, 5 end in a place (the green of the weaver lane, the terraces of the garden, mill and wall lanes, the court of the lower street), 8 end where a way is meant to end, and **2 end in nothing: the west end of the cooper lane and the north-east end of the lower east lane.** A dead end that ends in nothing is a bug (to do: a stair, a court or a shop front with a bench and a favour to do).
- **R4. A loop in every district**, of 60 to 120 m: the lower town (the lower street, four passages, the cooper lane and the lower east lane), the west (the garden lane and the vine lane, with the chapel and the cistern), the north (the tanner loop). The east has none yet.
- **R5. Three levels in every view:** the street, a terrace or a roof, the cistern or the gorge below.
- **R6. Compression and release.** A lane of 2.4 to 3 m opens into a square, a court or a terrace every 40 m, and a wide place is entered through a narrow one. A threshold is a narrow place seen from a wide one.
- **R7. A reason to switch.** Every district has a pair of things that are not the same in A and B, found by pressing Tab where nothing says so: the fountain of the court, the model of the town on the scribe's table, the lanterns of the lamplighter, the sails of the sailmaker, the child's bed, the cistern lid, and, from now on, what twelve years do to what the traveller leaves (3.9).
- **R8. A puzzle is a threshold, not a tunnel.** Nothing the player can walk is a corridor between two walls with nothing to see at its sides.

What holds the player in the references: one route that doubles back through itself and a shortcut that opens behind (Stormveil); a hub that is returned to (Firelink, Dirtmouth); the triangle rule, something new every 20 to 30 seconds (Breath of the Wild); what is locked is seen first, and knowledge, not an item, opens it (Tunic); a story told by rooms (Obra Dinn). They are a method, not a source of numbers.

**The districts, as a plan.**

| District | What the player learns or does | Seen from far | What is found there (beats) | Loop | The echo of A and B | What is missing |
|---|---|---|---|---|---|---|
| Harbour and lower town (the hub) | the switch, the causal rule, the ledger, the lantern | the mast, the lighthouse, the gate towers | sea gate, air step, five rooms (fishmonger, cooper, baker, scribe, lamplighter), the fish market, the court, the boat yard, the rope walk | lower street, four passages, cooper lane, lower east lane | the fountain of the court, the town on the table, the black piles and the pale stubs of the quay | a second court; favours to do |
| West: the market and the gate | the ledger: growth, keeping and the keeper of things | the clock tower, the chapel's bell-cote, the market hall | canal crate, scaffold, the garden and its gardener, the gate warden at the bastion, weaver, chapel, cistern, the green, three terraces | garden lane and vine lane | the cistern lid, the chapel roof, the loom | the market hall cannot be entered (a hillside of 12 m) |
| Centre: the Rock | the finale | the tower, the mast | lift, bridges, gorge floor, plaza, the tower | the ring of the plaza | the moat and the dry gorge | the inside of the tower |
| East: the locks | climbing and water | the water tower, the aqueduct | the lockmaster at the water tower, mill lane and its terrace, the miller, the sailmaker, the locks | none | the basins full and dry | a loop; the mills themselves |
| North: the upper town and the theatre | the aerial switch, the prism | the theatre, the graveyard | plank, tanner loop, gatehouse crate, watch, weaver and child rooms, the wall lane, the watch captain on the cliff | tanner loop | the stage, the stumps of the seats | the front of the theatre; a way down from the wall lane |

**Built:** a court that is a dead end worth the walk (a gate in the north wall of the lower street, a court of 6.4 x 7 m walled with garden walls of 4.5 m, a fountain, an olive, benches, laundry, an overlook of the map); roofs over the corridors (`crown_bay()`, bays of 6 m with level tops of their own, gabled roofs, chimneys, dormers, balconies at 7.4 m out of reach of everything a puzzle gives); four more rooms; a smooth ground (`build_terrain()`); and the flying camera.

### 7.12 The teaching ground

The harbour and the lower town teach the game before the three ways open. The model is Breath of the Wild's Great Plateau and the opening of Elden Ring: a bounded place where every basic verb is learned by play and never by text, a first view of the whole world as the reward, a lure that can be seen long before it can be reached, and, after it, a world that is open. Portal's rule holds throughout: one idea at a time, taught where it is safe, then twisted.

| Station | Place | The one idea | What the player does | A failure costs | State |
|---|---|---|---|---|---|
| T0. The first frame | the quay, in B | what the game asks, and the colour of what lasts | sees the three dark lamps of the mast over the tower, the heap, the winch, black piles standing in the water where pale posts are only stubs and, across the harbour, one house with a lit window, above the reach of the sea (0 m and up): the house of a woman who stayed | nothing | planned (the heap, the winch and the mast are built) |
| T1. The switch | the heap of wreck, which closes the way west in B only | Tab | presses Tab, finds the quay alive in A with the heap not there, walks past and comes back | nothing | built |
| T2. The first seed | the first terrace across the harbour from the start, at the house of the lit window; the quay is under the sea's reach, so a seed planted there is salted and does not grow (3.9) | growth, and a favour | a woman by a pot of bare earth and a seed bag marked with an olive: plants it; switches: in B she is old, at the door of the house with the lit window, and a stout olive stands beside it, a step up to a shortcut of the stair | nothing; the seed can be dug up | planned |
| T3. The causal rule | the winch of the sea gate | what is done in A shows in B | turns the winch in A (gold); in B the portcullis stands raised where A left it | nothing | built |
| T4. The lantern | a short quay with a gap over shallow water | the circle of A held in B | sets the lantern down at the edge in A, switches, crosses on the held floor, takes it up | a wet step back | planned (KI-14) |
| T5. The vista | the top of the sea stair, after the gate | where to go, and the lens | the lower town opens: the three beacons (10.2), the three stairs, the tower with its three dark lamps; the prompt for the lens appears (hold Tab: the town in A, in a circle) | nothing | planned |
| T6. The switch in the air | the lower street: a barricade in A, a fallen house in B | acting across the moments | stacks two crates in A, jumps and switches at the top onto the heap of B | a stair back | built |
| T7. The canal | the market stairs | a crate floated in A lies on the dry bed in B | pushes the crate in A, crosses in B | a stair back | built |
| T8. The three gates | the hub of the lower town | the choice | the three stairs open together, each with its emblem; the player picks one | nothing | built in part |

**The freedom inside it.**

- T0 offers three things in any order: the heap, the woman across the harbour (the one lit window) and the sea stair. The sea stair leads on, and nothing forces the seed.
- After the sea gate the lower town is a fabric, not a corridor (7.10): T6 and T7 are thresholds in it, with courts, rooms and favours to do at their sides, found in any order and none required.
- The pale stubs and the black piles of the quay are the first lesson of what lasts. Nothing explains them; a player who asks why only some posts stand has the first reading of the ledger, and the answer is tried at the canal crate and in the west way.
- The first sight of the storm is the sea itself: flat, with long lines running over it, and no birds. Nothing explains it here; the three keepers explain it later. There is no enemy, and nothing chases the traveller.
- Safe failure everywhere: no fall in the teaching ground costs more than a wet step or a stair back.

### 7.13 Natural and free: how the map is composed

The town is composed like a place that grew, not like a diagram of three corridors.

1. **Follow the land.** Streets follow the contours, stairs go where the slope is steep, districts sit on terraces, and no street is a straight line over a hill.
2. **A triangle of views.** From the hub three landmarks (the water tower, the bastion with the clock tower, the theatre and the cliff) stand apart, so each district opens on a visible lure and the three together say "there is a choice".
3. **Two ways in.** Each district has its main stair from the lower town and a second way in found later (a back lane, a roof, an upper terrace). The second way joins local loops and never passes a puzzle.
4. **Shortcuts open forward.** What a keeper's key opens is a shortcut to the next unseen view, never back to the start.
5. **Weave the wall.** The same wall is seen from below, walked along its top later and passed through its gate last. Stormveil does this; so does the climb from the quay to the Rock.
6. **Fair secrets.** An optional passage (a grate, a loose board, a cistern lid) is seen and not understood at first, and is paid with a view, a trace or a favour. It is never a collectible and never required.
7. **Framed arrivals.** A narrow lane opens on a wide view (R6); each district opens with a framed view of the tower.
8. **Open between districts.** Districts see and hear each other (sight lines, sounds, lights), but depend on nothing in each other.
9. **No empty distance.** A sprint of 20 s with nothing to read is shortened (7.6).
10. **Nothing leads round.** Every new route is run through the solver before it is kept (7.10).

## 8. Puzzles

Every way is made from the one ledger (3.9) and a few templates. This section says how a way is made, how a puzzle is designed, which templates exist and what each way is built from. The ways are sketches that are placed on the built model where it has the object, and each is proven by the solver (section 11) before it is kept.

### 8.1 How a way is made

- **Three beats and a keeper.** Each way has three puzzle rooms in this order: **teach** (the rule is shown where nothing can go wrong, and the answer is in view), **twist** (the same rule in a less obvious form), **test** (the rule combined with one idea taught before, in a larger space). The keeper closes the way: the last beat removes the trouble that keeps him from his instrument, he reads it and gives his key. The teaching ground teaches the rules; no way teaches a rule that another way needs.
- **The keeper's trouble is the goal of the way.** It is ordinary (1.1) and visible from far (10.2). The beats of the way are the means of removing it.
- **Favours, not errands.** Where a beat can be a favour done for a person of the district, it is. A person is never a quest giver and nothing is fetched across the town: a favour removes a hindrance with the verbs of 3.6, and the person's stance changes (3.3).
- **One idea per room.** A room has one new thought and no more acts than principle 4 allows.
- **Seen from the entrance.** The end of a room (the open gate, the tree to climb, the far bank) is in view before the first act; the way to it is not.
- **The answer is in the room.** The ruin shows a thing that has no cause (3.4). No clue is carried from another room or another way.
- **A wrong try costs a step back** (principle 14). Acts are undone by their opposites; the solver lists any wrong plan that is not safe.
- **Hidden things pay the player who looked.** An optional room is entered by something seen and not yet understood and ends in a connection, a view or a favour (section 4).
- **A reason in the world.** Every obstacle has one (principle 11, 7.9). Nothing is locked only so that a puzzle can open it.

**How to design a puzzle here, backwards.**

1. Choose the picture in B: something the player wants to reach or pass: a tree to climb, a stair that stands, a heap to step on, a gate that is open, a boat on its slip.
2. Ask the ledger which rule in A makes it: a seed, a prop swapped for stone, a pushed crate, an opened gate, a boat hauled up.
3. Make the obvious plan fail for an ordinary reason that the ruin shows: the nearest plank is pale and rots, the nearest soil is under a roof and gets no sun, the nearest person is of the wrong trade.
4. Put the evidence for the one plan that works in B: the black piles stand and the pale stubs do not.
5. Check what a wrong try costs: nothing, a step back.
6. Hand it to the solver: every wrong plan must fail safely or be listed as a way round.

Borrowed from, without copying: the one-idea rooms of the Zelda shrines and the Elden Ring catacombs, where the end is seen first, the answer lives in the room and a hidden wall rewards a player who looked; and the reading of a ruin backwards to its cause, as in Return of the Obra Dinn.

### 8.2 Templates

| Template | The thought | Rests on | Used in |
|---|---|---|---|
| Trace | "I did it there, so it is like this here" | 3.3 | every way; the base of all others |
| Grow | a seed planted in A is a tree in B; which seed, which soil, which sun | growth | the west way, the teaching ground, the top floor of the tower |
| Last | what lasts twelve years: black timber, stone, iron, a sheltered place | rot and keeping | every way |
| Prop | a thing held up by timber falls when the timber rots; swap the prop for stone | prop | the north way, the middle floor of the tower |
| Silt | water leaves silt where it stood; a dam leaves a bank, a flow leaves a bed | silt | the east way, the lowest floor of the tower |
| Rust | an iron mechanism stays as A left it | rust | every way |
| Entrust | give a small thing to the person whose trade keeps it; it is kept in his place | keeper of things | the west way, the favours |
| Lantern relay | a circle is carried across by hand, never by the switch: cross on the held floor, take the lantern up once on B ground, set it again | 3.2 | optional rooms, the well of the west |
| Held floor | a floor of A kept standing by the lantern is the one step in B; what rotted outside is whole inside | 3.2 | the west and north ways |
| Aerial switch | the jump starts in one moment and ends in the other | 3.1 | the teaching ground, one twist of a way, optional rooms; never the only test of a room; the margin of section 11 |
| Difference | a mark that exists in both moments is worn differently in B, and the difference says what to do in A (which notches were used, which wheel turned) | 3.6; the DIFFERENCE view of the map (10.4) | one beat in each way |
| Favour | a person in A is hindered; clearing the hindrance makes him step aside, open a gate or keep something, and it shows in B | 3.3 | the teaching ground and every way |
| Prism | a thing of A, passed through a lens, lasts in B; never the other way | 3.6 | the north way |
| Hidden way | a wall of A that is a gap in B, shown from the street by a draught or a light | 3.6 | optional rooms |

### 8.3 The ways

| Way | Rules | The keeper and his trouble | Teach, twist, test | State |
|---|---|---|---|---|
| Teaching ground: harbour and lower town | the switch, the causal rule, growth, rust, rot and keeping, the lantern | none | see 7.12: the heap (the switch), the old woman's olive (growth), the sea gate (rust, the causal rule), the short quay (the lantern), the lower street (the switch in the air), the canal (a ballast crate leaves a heap of stone on the dry bed) | the switch, the sea gate, the air step and the canal are built (`tests/test_things.cpp`); the seed, the lantern's demonstration and the vista are planned |
| West: market and gate | grow, last, entrust | **Gate warden**, at the signal house of the bastion gate. The carpenters took his weather glass down and shut the house behind their scaffolds. | **Teach:** the fig and the terrace: a bare planter at the foot of a terrace wall of 3.4 m is seen from the market stairs in B; a seed bag marked with a fig lies on a bench in A; in B the fig has limbs at 1.2 m, 2.3 m and 3.4 m, the height of the wall, and the way up is a stair of three. **Twist:** the scaffold of the market (built: placed on a terrace; the lantern on it in A, climbed in B) gets its reason: it stands in B because it is raw timber under a shed roof, while the open scaffold across the lane is a stain; the player reads what cover keeps. **Test:** the walled garden of the gardener behind the signal house: give him the vine seed and he plants it at the foot of his own wall and, pleased, lets the traveller through his garden to the back door of the signal house; in B the vine is a ladder up the wall. **Keeper:** the house is open, the glass is hung again, the warden reads it and gives his key. | the scaffold is placed; the seed, the gardener and the warden are to build |
| East: the locks | silt, last, rust | **Lockmaster**, at the water tower. The fair's cargo barge is wedged in the third lock and holds the float of his tide board. | **Teach:** the canal crate (built): a ballast crate pushed in A lies on the dry bed in B as a heap of stone, a step. **Twist:** the plank between two roofs (built): the roofers' plank is tarred and black, so it holds in B; a pale plank beside it does not. **Test:** the third lock: its sluice is iron and stays as it was left, so shutting it fills the chamber and floats the barge free in A, and in B the chamber is a silted basin behind a shut sluice, to be crossed by the wicket above it. **Keeper:** the float moves, the pointer shows the water out of step with the tide, the lockmaster reads it and gives his key. | the locks and the plank exist; the third lock is to build |
| North: theatre and cliff | prop, rust, the aerial switch | **Watch captain**, at the watch post on the cliff. The great awning and the scaffolds of the theatre hang across his view, and his glass is out for mending. | **Teach:** the gatehouse crate (built): a ballast crate pushed under the balcony in A, then a switch in the air onto the heap of B. **Twist:** the prop: a stone lintel over the side arch of the theatre is held by a timber prop; in B the prop has rotted and the lintel lies across the lane; a stone block pushed under it in A instead keeps it standing and the arch open. **Test:** the awning: its winch is iron and stays as left, so the awning lowered in A is down in B, which opens the top row of the theatre as a way along to the cliff stair; the lantern holds the rotted rope whole while it is lowered. **Keeper:** the view is clear, the captain sees the long lines on the sea and the birds gone, and gives his key. | the gatehouse crate is placed; the rest is to build or design |
| The Rock: the link of the west way | the causal rule at a large scale | none | the wreck of the west bridge down into the gorge (B); the lift lowered in A and climbed in B (7.4); the gate of the west bridge stays shut (seen, not crossed) | built (2026-11-25) |
| The tower | the three ways together | none | the climb of three floors and the last flight (5.1), the belfry (5.2) | to design |

The last beat of each way is placed with the model (section 12, step 4); the sketches above say what thought each beat tests and which built object it uses.

### 8.4 The moment of understanding

The route solver proves that a way can be solved. It cannot prove that solving it is interesting (KI-18), so each way is built for one moment in which the first plan fails and the second is found by reading.

| Way | The first plan | What it shows in B | The second plan, found by reading |
|---|---|---|---|
| West | lay the nearest, pale plank, or plant the seed in the planter in the shade | a stain; an empty spot | the black beam stands in B and the planter in the sun is the one with a ring; so what lasts is black, and what grows needs sun |
| East | dam the lock with timber | the timber is gone and no step is left | the dam is made of ballast crates, whose stone stays |
| North | walk under the arch in A and leave it as it is | the prop has rotted and the lintel lies across the lane, so the arch is shut | a stone block under the lintel in A keeps the arch open |

- **Each way has one such beat**, where the player is stuck for one to three minutes and then understands. The understanding is always the same sentence: that is why it looks like this now.
- **An uncoached test measures it.** A beat that is never stuck is too easy; a beat stuck for over five minutes lacks an answer in the room (8.1).
- **Three ways to read.** The ruin shows a thing with no cause; the colour of timber says what lasts (black) and what does not (pale); and a person's trade says what he keeps. A player who has learned the three can predict B before switching.
- **Shared objects carry the reading.** A mark, a sign or a tally exists in both moments and reads differently in each; the tower and the sea wall are the same in both. Nothing is pushed out of an object: a switch that would overlap is refused (principle 9).

## 9. Environmental storytelling

The story is in section 1.1 and its ending in section 5. Main-route play does not depend on knowing the backstory. Story beats stay in places the player already explores and never in collectible-triggered scenes.

- **The last evening of the fair.** In A set a wedding table in the court, hang fishing nets to dry, leave the bunting up and the stalls half packed. Return to the same place in B to show what survived, what broke and what someone kept: a stub of the bunting pole, a bare planter, a stall counter with a lit window above it. Composition and props, never explanatory text.
- **Windows.** A lit window in B is the first light the player finds, and each belongs to a person the player may have met in A.
- **The three troubles are seen from far.** The scaffolds round the signal house, the barge in the lock and the awning across the cliff are the first things the keepers' posts show, and they are the reason the player goes there.
- **What lasts.** The black piles of the quay stand in B and the pale posts are stubs, so the town explains its own ledger before anyone says a word about it.
- **The three signs.** The harbour that breathes out of step with the tide, the birds that are gone, the glass that falls: they are there for the player who looks, and the keepers explain them only at the end of their ways.
- **Overlooks.** A high point in each district frames a visible next destination and, when the terrain permits, the tower. The view tells the player where a route might run, not only that there is scenery.
- **Left untold:** where the lantern came from, the name of the town and who the traveller is. They can stay untold (section 14).

## 10. Guidance and immersion

The player must always know what the game asks and where to look next without pressing a key. The help keys are for the player who wants more, not for the one who is lost. Nothing is written.

The environment explains the next move before any marker is added: the destination is framed from the entrance, the next interaction has a recognisable silhouette, and the difference between A and B is readable at the obstacle. One new idea is taught at a time and tried somewhere safe before the main path requires it. Brief key prompts are used where needed, never written walkthroughs. Optional routes are not revealed by default, and the answer is not shown before the player has tried.

### 10.1 Four layers of goal, and a fifth

| Layer | The player asks | The game answers with | State |
|---|---|---|---|
| Far | Where does this end? | the three lamps of the signal mast over the tower, in sight from the first frame, dark until they are lit from the lantern in the belfry (the goal; progress is shown by the emblems of 10.5) | the lamps built (2026-11-26) |
| Way | What is in this district? | the three gates of the lower town, each with its emblem; on entering a way, a beacon (10.2) | planned |
| Room | How do I get past this? | the room itself: the end in view, the ruin showing a thing with no cause, A and B readable at the obstacle (3.4, 8.1) | built in part |
| Favour | Who needs help here? | a person who stands wrong and looks at what troubles him; the lit window in B when it was done (4) | planned |
| Help | I am stuck | the flame of the lantern and H (10.3), the map (10.4), the lens (3.8) | the flame, H and the map built; the lens planned |

### 10.2 Beacons

A beacon is a place the player can see and cannot yet reach, at the start of a way and of a room. It is the question; the way to it is the puzzle. It is part of the town, seen from the street, and never a marker. For each way it is the keeper's own trouble seen from far: the signal house of the bastion gate with the scaffolds of the carpenters round it (west), the barge across the third lock under the water tower (east), the great awning of the theatre hanging across the cliff (north). The map shows a beacon the player has seen as a ring that fills when it is reached.

**Why a mast.** The tower itself cannot be the far goal from the quay: the sea wall, 8 m in front of the start, hides it in both moments (`tests/test_firstminute.cpp` casts the rays from the camera of the first frame; the south face of the tower is blocked in A and in B, and only the top 5 m of the spire of A clears the wall, while the game starts in B). So the three lamps hang on a signal mast that stands over the tower and clears the wall: three round lamps on a yard at 61 m, the mast coming out of the dome of the church and, in B, rising free over the broken shaft. The player sees the lamps from the quay, the sea stair and the sea gate; from the streets of the lower town the houses hide them, and the flame and the map take over.

### 10.3 The lantern as compass

The compass is the lantern, so help is an object of the world and not a layer on the screen. Built (2026-11-26): `scene/Guidance.h`, `scene/Flame.h`.

- **The flame leans (always on).** The flame over the lantern, in the hand and set down, leans 18 degrees toward the goal: the first step of the main path that is not done, and the door of the tower when all three keys are held (setting `[guidance] lean_degrees`; a first design said about 10 degrees, too small to see at the distance of the camera). It stands upright within 2 m of the goal.
- **It grows with need.** After 30 s without progress (a step done, a new 6 m cell of ground, or a new closest approach by 4 m) the lean grows to 40 degrees and the flame throws sparks.
- **H, first press:** the lantern is raised (the arm goes up) and a warm beam sweeps over the place for 6 s: it swings wide and settles on the bearing of the goal. It shows the place, not the act.
- **H, second press** during the sweep, and only after 90 s without progress, so the player has tried first: a frame round the thing to use (the winch, the crate, the plank), joined to the lantern by a line of motes, for 12 s. Before the 90 s a second press only puts the sweep away. A press while the frame shows puts it away.
- **Planned:** the flame points at the nearest unsolved main node that the player has seen and not at the first step of the old line, and stands upright in the lower town, where the three gates are the choice. It never points at a favour: a person who needs help shows it by his stance (10.7). The third press of H is dropped: the lens (3.8) is the third step.
- **Rules.** The player escalates by pressing and by time without progress, never by distance to an answer. With the lantern set down its flame leans where it stands. Warm guidance in B must not be confused with the lantern patch or a causal trace: gold means use, and the lantern's colour is its flame.

### 10.4 The map: fog, memory and two moments

Built: `scene/MapMemory.h`, `render/MapView.h`, the plan in the town file. The map is an exploration reward and not a complete plan handed to the player. M opens it over the frozen game.

- **Style.** The plan samples the town every 0.5 m, but the map never shows the cells: every ink is a smooth shape (averaged over 2 x 2 cells and cut along the half level; the whole town is cut on a grid of 2 x 2 cells with partial cover). Cliff is terrain steeper than about 57 degrees and wider than one cell. It is ink on tracing paper, not a satellite image: A in warm violet-gold, B in cool teal-blue, north up and marked.
- **Fog of the unknown.** The whole town is under fog from the first frame and the paper shows all of it (zoom 1), so the player sees how large the place is and how little is known. Unvisited cells are a soft vellum cloud, violet in A and teal in B, with an uneven edge that only ever reaches into the seen part. Walking within 15 m clears it; overlooks reveal a wider silhouette.
- **Main puzzle knowledge.** Every puzzle still to be solved shows through the fog as a broad ring with a question mark, not as a place or an instruction: 12 m across for the next one, smaller and fainter for the others, its centre a few metres from the true place. Optional puzzles, shortcuts and hidden things are never exposed. The ring goes when its puzzle is solved.
- **Solved regions.** A solved puzzle lights its district (22 m round it) for good.
- **Things as they are now.** The footprint of every box of a thing in its present state is drawn on the sheet of its moment (strong on the sheet on top, faint on the other), only where the place has been seen. Not built: water levels.
- **Two moments.** The default view is the current sheet; Tab turns A/B without switching the world. Clickable MAP and DIFFERENCE choices: DIFFERENCE suppresses cells that are the same in A and B and leaves only the warm and cool mismatches.
- **Markers.** Up to ten marks in six shapes; a mark remembers the moment in which it was placed and is paler on the other sheet. T or the MARK button chooses the next shape.
- **Controls.** M opens and closes, Esc closes, Tab turns the sheet, E places or removes a marker. The pointer clicks MAP, DIFFERENCE, ZOOM +, ZOOM -, CENTER and MARK. + and - (number pad, Page Up and Page Down too) zoom from 1 x to 4 x, the arrow keys move the window, C or R puts it on the player. No fast travel and no corner minimap.
- **Planned:** a household that kept something is drawn as a small warm dot on the sheet of the moment, a record the player earned; a household still to be helped is never drawn. Height bands (the plan has no heights; it needs a new layer in the town file) are not built.

### 10.5 The glyph board

This is the quest system without words. Each way has an emblem, simple enough to read at 64 px in both colours: a sphere for the west (the gate warden), a column for the north (the watch captain) and a box for the east (the lockmaster). The emblem is carved on the stair gate of its way in the lower town, on the post of its keeper, on its key and on its lock in the door of the tower. It is dark until the key is taken and then warm, in both moments, and the post of a keeper who has given his key is lit and manned in B and is seen across the town. The three emblems are the whole quest log: no list, no counter, no marker that names a task; favours are not on it. The same three emblems are on the map. The mast is not part of it: its lamps are the goal and are lit last. Built: the three lamps on the mast, still lit by bells (KI-09). Not built: the emblems on the gates, the posts, the keys and the door.

### 10.6 The lantern's demonstration and the first minute

The lantern gets an early, safe demonstration in the harbour, before the three gates open and before the first use that needs it (T4, 7.12). The lantern in hand is shown placing a circle in A, the warm patch that keeps A inside B, and retrieving it. The town already supports placement in A and a patch of A in B; the problem reported in play is that the feature is not apparent. Any hint route is validated with the real character and camera, not only the route solver. (Not built: KI-14.)

**The first minute, as built (2026-11-26).** The game starts in B on the quay, and the first frame is set by the town file (spawn yaw -0.55, pitch +0.29): the heap of wreck to the left, the winch of the sea gate left of the middle, and the three dark lamps of the mast high in the sky right of the middle. `tests/test_firstminute.cpp` keeps it so for lenses of 60, 70 and 90 degrees and the screens 16:9, 16:10 and 4:3 (the lamps are at 0.88 of the half height in the narrowest case) and proves with rays that nothing of the town is in the way of any lamp in either moment. The flame of the lantern in the hand leans toward the winch, which is in the picture. After 8 s of walking, to a player who has not switched, the prompt says TAB SWITCH WORLD; after 40 s, M MAP. The first minute is tested uncoached (section 12): the player sees the lamps, goes to the sea gate without being told and can say what the game asks. **Planned for the first frame:** the black piles and the pale stubs of the quay (3.9) and one lit window across the harbour (7.12); the test adds a ray from the camera to the window.

### 10.7 The language of the world

The world speaks a small vocabulary the same way everywhere, so that nothing needs a word:

| Sign | Means |
|---|---|
| a bare planter, a stub of pale timber, a gate rusted open, a bank of silt in B | something was done or is to be done in A, and the ledger says what (3.9) |
| pale timber | raw: it does not last |
| black timber | tarred: it lasts |
| gold | it can be used |
| dark loose earth with a gold ring | a seed will grow here |
| a person who stands wrong and looks at something | a favour is asked, and where he looks is what troubles him |
| a lit window in B | something was kept; the person is there |
| the lean of the lantern's flame | the way, never the act |

### 10.8 Sound

Sound is a second cue. A: harbour activity, gulls, the murmur of a fair at its end, low voices, and under them a slow, long swell that is a little out of step. B: rain (on stone, on water and on timber, and none under a roof: 6.11), wind, dripping, a few voices on the heights, no gulls. A household that kept something has a faint hearth sound near it in B. The switch, a refusal and the placing of the lantern are told apart by sound once audio is added. The bell, when rung, is a deep, clear tone that the whole town hears; the hour bells are ambient and never a goal.

## 11. Limits and proof

The body walks at 5.5 m/s and jumps 2 m; a running jump covers about 3.6 m (numbers in [CONVENTIONS.md](CONVENTIONS.md)).

| Item | Main path | Optional routes |
|---|---|---|
| Horizontal gap | 1.8 m | 2.5 m |
| Step up | 1.2 m | 1.8 m |
| Switch in the air | most tried moments work | a few work |
| Landing | at least 2 x 2 m | at least 1 x 1 m |
| A miss costs | a stair back, or a wet step | the same |
| Slopes | streets 24 degrees, ramps 27, stairs 32 | - |

| Target | Proof |
|---|---|
| The main route | `tests/test_route.cpp`: a solver over both moments plays the things in order; each is reached, nothing further on is reached early, and any way round a puzzle is listed |
| Things and the causal rule | `tests/test_things.cpp`: a thing opens in A only, its trace shows in B, an opening that would close on the body is refused; a flood of every place the body can walk to |
| The lantern | `tests/test_lantern.cpp`: the circle, the solid with it in both moments, the rules of setting down and taking up |
| The town | `tests/test_town.cpp`: the file, the solid, every street walked end to end, the water of each moment |
| Recovery | every place of return is a place to stand, out of the water, in both moments (`FitsInTown`, `Moments::Supports`) |
| The third-person body | `tests/test_town_player.cpp`: the climb checks support, clearance and water; the camera retracts round walls |
| The three ways | `tests/test_route.cpp` (planned): from the lower town with the teaching ground done, each way is solved on its own; its goals are unreachable before their prerequisites; its reachable set is the same with the other two ways done or not; a link opened early reaches no unsolved puzzle of another way; the tower opens only with the three keys; in every order of the three ways the tower is reached |
| The ledger | `tests/test_ledger.cpp`: every rule of 3.9 on every combination of the five facts of a place, against an oracle written another way: a seed in sun and soil grows its tree, and not in shade, under a roof or in salt, a vine without a wall is a tangle; plant stuff is gone in the open and stands under a roof, tar and stone stand; an iron mechanism keeps the state A left it; a crate is a heap in the open and a crate under a roof, as its box; a timber prop lets its load fall and a stone prop does not; a dam leaves a bank while it holds and a dry bed when it gave way; a person takes what his trade wants and refuses the rest; the same question has the same answer in any order (B is a function of A), a roof never hurts what plant stuff keeps and the sea never helps a seed; every tree can be climbed in steps of 1.2 m; changing, adding or taking out a row changes the answer and no code; each kind of wrong table is found by name. `tests/test_things.cpp`: the crates of the town are as high in B as the ledger's heap. Planned, with the exporter: the same over the whole town file, and the circle of the lantern showing A inside it |
| The tower (planned) | each of the three floors is solvable from the foot of its stair with its one rule only; the last flight is solvable from the top of the stair with one jump and one switch in the air and not without the switch; the top is reached in every order of the ways; with no favours the route, the bell and the ending are the same as with all of them; the present seen from the belfry is the present that the sweep ends in (B is a function of A); a fall in the tower returns to the last landing |
| Guidance | `tests/test_guidance.cpp`: the goal; the flame (it leans toward the goal, restless after 30 s, calm after progress, upright at the goal, the same on the lantern set down and on the avatar); the presses of H and their times; what is drawn; planned: the flame upright in the lower town, nothing pointing at a favour |
| The lens | `tests/test_lens.cpp` (planned): the solid does not change while it is open; tap and hold on the ground; the switch at once in the air; it closes when the body leaves the ground |
| The map | `tests/test_map.cpp`: the plan of the model (both moments, the moat in A only, the streets under their middle lines); what is seen (the radius, the overlooks, never more); the markers; the paper (inside every screen, the player and the tower on it, the window); the ink (neutral, warm, cool, the sheet on top, blank paper, the lamps, the arrow, the flags); the whole town fits the batch |
| The first minute | `tests/test_firstminute.cpp`: rays from the camera of the first frame to every lamp in both moments; the sea wall does hide the tower itself; the lamps, the winch and the heap inside the picture for every lens and screen; three lamps, dark at the start; the compass points into the picture; the heap closes the quay in B only |
| The light | `tests/test_lighting.cpp`: the maths of the shader run as C++ (fall-off, one lamp on a surface, the soft shoulder, the fade at the edge of the shadow map); the choice of at most 32 lamps; the shadow box; the settings; the lamps of the town file, the lantern and the halos |
| The style of the light | `tests/test_style.cpp`: the functions of the style run as C++ (the rings and the warmth of a lamp, the hard steps of the sun, the clouds, the rim, the cool fill only in the shadow, the painted stone and the damp, the haze, the grade), the numbers that feed them, the looks and the values of the look of 6.10 (the tones of A, the ground and the deepest dark of B, the far distance of A); the maths of the ink lines on a depth buffer that the test makes (no line on a plane or inside a face, the outline on the near side only, an inner edge from 50 degrees), the pass written again in C++ and the blend of the pass against the line, the sky leaning to the hue of the far land with its horizon equal to the far hills, the sky that the water reflects equal to the sky of the picture, and the sun of A held to the model |

## 12. Order of work and acceptance

1. **Movement.** Third-person movement, sprint, ledge climbing and a visible lantern: a first pass is built. Tuned against a tight alley, a raised ledge and a switch in the air before anything else changes.
2. **The ledger and the teaching ground as the reference district.** Built (2026-11-26): the signal mast with its three dark lamps in view from the quay, the flame that leans, H, the map, the first frame. Built (2026-11-28): the ledger as data and rules, with its test (3.9). Not built: the ledger in the town (the facts of every place in the town file, B computed from them), growth, rot and keeping in the game, the held thing, the old woman and the first seed, the colour of timber, the lantern's demonstration, the vista with the first prompt of the lens, the lens, and the first frame's black piles and lit window. The art is validated on the quay-to-sea-gate route (6.8). Then first-time players play without coaching, and the measures are: within 60 s can they say what the game asks; do they take a stair unprompted; how often is H pressed; how many times do 30 s pass without progress; can they say why the pale post is a stub and the black one stands; where is the route lost, an interaction unreadable or an unreachable platform expected to be playable. The light is built and not yet seen in the game: E-21 to E-25 of [BUILD.md](BUILD.md) are played first.
3. **Open the town.** Model the stairs from the lower town to the east locks and to the upper street and the links to the plaza; replace steps by goals with their ways; turn the corridors into thresholds (7.10); extend the solver to the tests of section 11; keep the old line as a baseline until they pass.
4. **The east way first** (its canal crate and plank are built and only need their reasons), then the west (the seed, the scaffold, the gardener) and the north (the prop, the awning), each with three beats and a keeper, each tested uncoached. Re-test safe recovery.
5. **Favours and the people.** The places where each person keeps things; about two dozen favours (a target to tune, not a promise); the traces; zero, some and all favours give the same route.
6. **The tower.** Prototype the door, the three floors, the last flight and the belfry, then the sweep of 5.3 (the ripple at slow speed from the tower, the ledger resolved in order of distance); verify the route from the link of each district, in each of the six orders, and all three looks of the ending, including a run with no favours.
7. **The storm, sound and text.** The evidence of the storm in A (6.4); the sound of 10.8; the lines of the people in Japanese.

**The look and the map come before the ledger is put into the town (decided 2026-11-28).** The look is proved on one sample, the quay steps (6.10); then the map is redesigned on paper and approved; then it is rebuilt district by district, the harbour first; then the baked occlusion and the light probes are made on the frozen geometry; and only then are the facts of the places (3.9) written into the new map. The numbered steps above keep their numbers and their content, and steps 2 to 6 that build on the town are taken on the new map.

Before the layout is changed broadly, the first district is reviewed with a new player: can the player find the route without coaching, explain why a trace appears in B and use the lantern unprompted at the first required obstacle? Safe camera behaviour on every switch, a refused climb with inadequate clearance, a refused switch into solids or water, and recovery without lantern-dependent support are tested. The old route solver is a baseline, not proof for the new controller.

## 13. Decisions

The decisions that hold now. How each came about, and what each replaced, is in the [Changelog](../CHANGELOG.md).

- **Two moments.** A is the last calm evening before a great storm, held still by the lantern; B is the same town twelve years later. What is done in A is what twelve years make of it in B, and nothing done in B changes A (3.3, 3.9). The lantern is the one thing that is not ordinary.
- **Time is the verb.** B is computed from A by a short table of rules (3.9). Puzzles are designed backwards from a picture in B (8.1).
- **No villain.** The storm is weather, and what failed was an evening of small troubles. Three keepers are each kept from one instrument by an ordinary trouble of the fair and give their keys when it is removed, so the traveller is never believed and never argues (1.1).
- **Kindness is shown by the town.** A household that is helped prepares, takes its preparation uphill at the bell, is lived in in B and remembers the traveller at the end. No counter, and nothing is required (4, 5.4).
- **The finale.** A climb that puts three rules into larger spaces and ends in the first lesson of the game; a belfry that shows the result before the bell; the flame of the lantern given to the lamps of the mast; the storm shown once, when everyone is safe; twelve years in one sweep; a walk in which the town remembers (5).
- **The town.** One hill town. The harbour and the lower town are the teaching ground; three ways in any order, joined by the Rock and not by each other; an open town with shut thresholds; composed as a place that grew (7).
- **Guidance without words.** A goal in sight (the mast), beacons, the lantern's flame as a compass, H for more, the map, the lens and a small vocabulary of signs (10). A corner minimap, a task list, a compass bar and narration are not used.
- **The first minute.** The sea wall hides the tower from the quay in both moments (measured), so the far goal is a signal mast over it, in sight from the first frame (10.2).
- **The look.** Hand-painted light with one master and a plain view; no lamp is needed to read the route; one palette; B derived from A and weathered stone in the moon, not ice (6). Chosen on 2026-11-28 and proved on a sample before the map is rebuilt: ink line and flat steps, warm light against cool shadow, and a traveller whose cloak is the same in both moments (6.10).
- **The lantern stays (2026-11-28).** It is not replaced: its rule is independent of light and the game is built on it. A is lit as a dusk with the streets in shade, the circle is drawn in A and some dark places of A are for the lantern alone, off the main route (3.2, 6.10).
- **The rain of B (2026-11-28, in principle).** B has a fine steady rain and A none, so that the two moments differ in motion and in sound as well as in colour. A stays the last light with the sun 14 degrees up and is not made a sunless blue hour; it may go cooler by its numbers. The rain is ink, is dry inside the circle of the lantern, is read through the shadow map as shelter and never changes a solid or a rule of the ledger; it stops in the ending (6.11).
- **Proof.** Nothing is kept that the route solver finds a way round, and the tests need no window and no GPU (11).

## 14. Open

- **The ledger.** The table is built and tested; what it asks of the town is open: the five facts of every place (soil, sun, a roof, a wall, the sea) and which objects carry a subject. The sizes of the trees (olive 2.4 m, fig 4.0 m, vine 6.0 m) and the height of the heap are to be tuned in play. Silt and prop are the least tried; growth is the most visible. A rule is added only after a puzzle that needs it has failed with the ones that are there.
- **The held thing.** Which small things exist besides seeds (a tool, a letter), and whether the free hand is a rule the player feels or only a limit.
- **The favours.** How many (the target is about two dozen), which are on the main route and which are optional, and how each is a puzzle and not a chore. The thresholds of the three looks of the ending.
- **The tower.** The order and the layout of its floors, whether each needs the lantern, and whether the last flight (one jump, one switch in the air, 3 m) is fair and a pleasure. A prototype decides (5.1).
- **The belfry.** Whether giving the flame is read as a cost and not as a punishment; the two-second hold; where the save is taken (5.2).
- **The storm and the sweep.** The cost of the storm of the ending (a rising sea plane, rain, cloth); the length of the sweep (the target is 25 to 40 s) and the order in which the ring reaches the households; whether a long press to skip is enough (5.3).
- **The lines of the people.** One short line each in Japanese, and the quay (5.4).
- **The lantern in the dusk.** The sun of A (14 degrees to start), the lamps of A, whether the ring and the wall of light of the circle are one line or two, and whether dark places in A are worth their cost; decided by the pair of pictures of S3 and S5.
- **The rain of B.** The slant (the moon is 53 degrees up, so rain along it is 37 degrees from the vertical), the density, whether the heights have it lighter, how far the rings go, the cost of the pass, and whether the clear sky of the ending is worth its cost; decided by the pictures of R1 (6.11).
- **The held evening.** A is the same dusk throughout; whether its sky should deepen in steps with progress (a signal and never a timer).
- **The story left untold.** Where the lantern came from, the name of the town, who the traveller is and who lit the first window.
- **The first frame.** The black piles, the pale stubs and the lit window must stay in the picture for every lens and screen of the test (`tests/test_firstminute.cpp`). The house of the lit window must be placed above the reach of the sea (0 m), in sight of the start; the model has not placed it.
- **The town.** Its name; the east loop; the two dead ends that still end in nothing (the west end of the cooper lane and the north-east end of the lower east lane); the front of the theatre and the town wall in the vocabulary of the library.
- **The look.** Look at B from the air and the quay with `B_AGE` at 0.65 (still too grey, too lilac, too blue?), the wash at 0.9, 0.6 and 0, the cool family (9%) and the accents (0.7%), and which old shape (the town wall, the theatre, the aqueduct, the bell tower) now looks least like the rest. For the sample of 6.10: the colour of the line, whether hatching is needed and how bright B is.
- **Tuning on play.** The camera, sprint speed, ledge height and the jump envelope on the reference route; the lean of the flame (18 and 40 degrees), the 30 s and 90 s times, the size and glow of the three lamps against the sky of A and of B, the colours of the map, the reveal radius (15 m); the light (E-21 to E-24): the lamps and the lantern against the moon of B, the weight of the shadows (0.85), the moon (0.8), the exposure, the shadow box (60 m, 2048 texels) and the cost on the laptop. If the tap on the ground feels late, the lens moves to the right mouse button (3.8).
- **Not yet drawn:** the three emblems for the gates.
