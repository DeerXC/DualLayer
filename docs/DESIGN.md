# Design

The design of DualLayer: what the game is, its rules, how it looks, the map and its puzzles. Decided on 2026-11-24, it
replaces the level design, the art and map plan and the gameplay notes kept before. The code as built is described in
[ARCHITECTURE.md](ARCHITECTURE.md); it is a greybox of the previous map and is being rebuilt to this design (section 12).

## 1. The game

A first-person 3D exploration puzzle without combat. One walled harbour town on a hill, at two moments:

- **A, the past**: a warm evening, the town whole and empty, a minute before something ended;
- **B, the present**: the same town much later, in ruins under a cold moon.

The body carries a lantern. **Tab** moves the whole world between the two moments. The lantern, **set down in the past**,
keeps a circle of the past standing in the present. What is done in the past leaves its trace in the present. The way
through the town is found by reading the present and changing the past.

## 2. Principles

Every change is judged against these. A change that breaks one is not made.

1. **One rule.** Everything comes from the two moments: the switch, the lantern and the causal rule are three sides of it.
2. **Hard in the head, not in the hands.** No precise timing on the main path (limits in section 11).
3. **Few actions per room.** A room asks for one or two placements of the lantern and one or two switches, at places the
   scene points to; the rest of the room is walked in one moment.
4. **Seen before reached.** Each place is seen first, then reached from an unexpected direction. No text guides the way.
5. **No long walks back.** No puzzle sends the player to another district; no shortcut leads back to a place already done.
6. **Nothing that looks like a bug.** The body is never pushed out of a wall; a switch or a pick-up into something solid is
   refused instead.
7. **Two temperatures.** The colour of the screen alone says which moment it is (section 6).
8. **Explained in person.** The core is hand-written C++ and small enough to be explained line by line in an interview.

## 3. Mechanics

### 3.1 The switch

| Rule | State |
|---|---|
| Tab moves the whole world between A and B; both moments are complete, walked and seen whole | built |
| A switch into anything solid, or into water, is refused: the ripple opens and turns back | built |
| A jump may start in one moment and land in the other (the switch in the air) | built |
| Holding Tab shows the other moment round the lantern without moving there (look before going) | planned, optional |

### 3.2 The lantern

The switch moves the player between the moments; the lantern keeps a piece of one moment in the other.

| | Does | Used for |
|---|---|---|
| Switch (Tab) | the whole world becomes A or B | moving, exploring, the views; both maps are seen whole |
| Lantern (set down) | a circle round it stays A while the world is B | puzzles |

Rules:

- The lantern is **set down in A only**. From then on its circle is A in both moments.
- In B the world is the ruin, except the circle: a stretch of a bridge that is whole, a wall that still stands, a pool that
  still holds water.
- Inside the circle what is seen is what is solid: A. Outside it, B.
- Picking the lantern up removes the circle. A pick-up that would leave the body inside something, or standing on
  nothing it was standing on, is refused, like a switch.
- The circle is warm light in the cold ruin: the image the game is recognised by.

Example, the broken bridge: in A walk onto the whole bridge and set the lantern down in its middle. Walk back to the
bridgehead and switch to B: the bridge has fallen, but its middle hangs in the circle. In B jump from the bridgehead onto
it and on to the far side, where the gate is open in ruin (in A it is locked). One switch; the thought is "the bridge of
the past, the gate of the present".

Open: the radius (fixed, or grown by finds), one lantern or a second one late in the game.

### 3.3 The causal rule

**What is done in A leaves its trace in B. Nothing done in B changes A.**

| Done in A | Trace in B |
|---|---|
| a plank laid between two roofs | the plank is still there, grey and rotten, and holds |
| a door or a gate opened | its ruin stands open |
| a crate pushed into a channel | it lies on the dry bed, a step |
| a lever pulled, a sluice shut | the bed behind it is dry, or the room flooded long ago |

- **Local**: a cause and its trace are in the same room or within sight of each other.
- **Readable**: the trace is where its cause was, so one switch shows the result.
- **The body and what it carries** switch freely in both directions; the rule is about the world.

### 3.4 Puzzles start from the present

Every puzzle begins in B. The ruin shows what is needed: a rotten plank in the wrong place, an open gate on the far side
of a gap, a dry bed under a door. The loop is: **look at the present, reason about the past, act in the past (or set the
lantern), come back to the present to see it**. The "of course!" of a room is always the same sentence: that is why it
looks like this now.

### 3.5 Traces

What the player changed in A shows in B with a faint glow in the colour of the lantern (never gold: gold means use). The
traces add up: from the cliff and the bell tower the player sees, across the whole town, what they changed. A trace is
feedback and story at once, without a word.

### 3.6 Objects

Each object answers one question: what happens to it when the moment changes?

| Object | Rule |
|---|---|
| Wall or floor of one moment | exists in A (made by people: wood, iron) or in B (left by the ruin: fallen masonry) |
| Crate | moved in A; in B it lies where A left it |
| Water | in A only, fed by the aqueduct (broken in B); not a floor for the body; things float on it |
| Wheel, lever, sluice | machinery: works in A; what it did lasts into B |
| Orb | carried, it goes with the body; put down in A, it stays there and its trace is in B |
| Prism | moves a thing of A into B ("it lasted until now"); never the other way |
| Anchor lamp | the same in both moments; the place of return |

Not used: free physics, throwing, timers on the main path, combat.

## 4. Progress and return

| Element | Rule |
|---|---|
| Bells | one in each district, rung where it hangs. Each lights an emblem on the bell tower; the three open the way to its top. Each bell changes the whole town (the water of A rises, the machinery turns, night falls in A): it is seen, never needed elsewhere |
| Anchor lamps | the rest points: few, at places that feel earned; a fall or R comes back to the last lit lamp of the room |
| Lamp travel | after the first bell, between lit lamps (built) |
| Resonance | after the second bell, the lantern pulses near a hidden memory light (built) |
| Memory lights | hidden, each a second use of something taught; a ring of lamps on the bell tower shows those taken; some bring back a piece of A in B |

## 5. Endings

At the top of the bell tower the great bell strikes three times; the body may switch between the strokes. At the third:
in A, **the past** (the moment is kept); in B, **the present** (time moves on); with the three bells rung and Tab held,
**both** (the present, with what the memory lights brought back). R after an ending plays the last moment again. Built.
The optional twist of section 9 may change what the endings mean.

## 6. Art

### 6.1 Two temperatures

| | A, the past (warm) | B, the present (cold) |
|---|---|---|
| Key light | low sun from the west, amber, long shadows | moon from high, blue-white, short soft shadows |
| Shadows | violet, never black | deep blue-black |
| Stone | sandstone lit amber, terracotta roofs | the same stone washed blue-grey, slate where roofs survive |
| Accents | copper domes in teal, brass, lit windows | verdigris and moss; the only warm colours are a few lanterns and the lantern's circle |
| Air | a light golden haze; far towers turn pink-orange | cold mist that hides the far side of the town |

### 6.2 Palette

| Role | A | B |
|---|---|---|
| Lit stone | `#E8B98A` | `#9FB3C4` |
| Shadow stone | `#9A5E48` | `#3D4E63` |
| Roof | `#C2553A` terracotta | `#4B5A6E` slate |
| Metal | `#E3A63C` brass | `#5FA39A` verdigris |
| Accent | `#3E8C7E` copper | `#4F6B52` moss |
| Light | `#FFD27A` window | `#FFB65C` lantern |
| Shade | `#5B3E6E` violet | `#1E2638` deep blue |
| Sky | `#F2C49A` dusk | `#56677E` storm night |

### 6.3 Rules

- Warm light and cool shade in the same frame; never grey on grey.
- Three depths in a view: a dark frame close by (an arch, a balustrade), the lit street in the middle, a pale skyline with
  one landmark.
- Detail where the eye stops (balustrades, lamp posts, benches, crates); plain walls where it passes.
- Gold is only for things to use. The material says the moment: wood and iron in A, fallen masonry in B.
- Form: low-poly and flat-shaded, no textures; a soft outline on silhouettes only; fog tinted per moment; the sky a
  gradient.
- Borrowed from, without copying: the walls and roofs of Dubrovnik and Kotor, the terraces of the Amalfi coast, the
  theatre of Taormina, the stone of Matera, the campanile of St Mark's (it fell in 1902 into its own foot); the haze of
  Journey, the forms of A Short Hike.

## 7. The map

### 7.1 The model

The layout comes from one model, `tools/layout/city_model.py` (Python 3, numpy, matplotlib). It draws both moments from
over the sea, from the cliff and from the street, and a plan with the main path:

    python tools/layout/city_model.py            (every view, into docs/images/city2_*.png, and the check)
    python tools/layout/city_model.py check      (only the check of its numbers against the limits of section 11)
    python tools/layout/city_model.py plan       (one view: aerial_a, aerial_b, street_a, street_b, vista_a, vista_b, plan)

The check measures every street against its slope limit and every joint of the main path (a street onto the bastion, the
quays, the graveyard, the top of the theatre) for a step; it reports nothing on the current model. The game's tests walk
every street with the real body (tests/test_town.cpp): in A every street is open; in B fallen arches and the rubble of the
sea gate close two of them, to be passed by a jump or by A.

The game will load the model's terrain, streets and buildings as data, so the game and the model are one town.

### 7.2 One hill town

| Place | Height (m) | Idea |
|---|---|---|
| The harbour | -6 (the quay) | the start; quays, a lighthouse, the sea gate |
| The lower town | -3 .. 0 | narrow streets under arches; the storm hit it hardest |
| The cistern and the gorge | -10 .. -6 | under the centre: the well, a tunnel, the floor of the gorge; water in A, walked in B |
| West: the market and the well | 0 .. 6, the bastion 14 | goes down: the great well to the cistern; the day bell |
| Centre: the Rock of the Bell | 14 | a round plaza on a rock in a gorge 20 m deep, reached by two bridges; the church and the bell tower |
| East: the locks | 3 .. 21 | climbs: the east stair down from the rock, five locks to the water tower; the dawn bell |
| North: the upper street and the theatre | 21 .. 30 | high: the street ends at the top row of the theatre, which faces the rock |
| The cliff: the graveyard | 42 | the view over everything; the night bell |

### 7.3 Rules

- **A spiral, not spokes.** The main path winds round the rock and passes through each district.
- **Seen first, reached from an unexpected direction.** The turns of the route come from the main path itself, not from
  shortcuts.
- **Three heights in every view**: streets on several levels with bridges between, roofs and arches over the street, the
  cistern under it.
- **Each district one idea**: west down, east up, north high, the centre across gaps.
- **The moments change the route**, not only the look: bridges, the aqueduct and water in A; the fallen bridges, dry beds
  and broken walls of B.
- **Narrow, then wide**: each district opens with a lane or a tunnel and ends at a view; each opens with a framed view of
  the bell tower.
- **A town, not boxes**: houses turned to their streets; the storm damage by how exposed a place was (the sea front most);
  in B the town let go (grass through the paving, figs in roofless houses, scrub over the groves, reeds in the harbour);
  nothing floats; the ground is under every floor and the sea or the land fills every view down.

### 7.4 The main path

1. The quay, the sea gate, the lower town (teaching, section 8).
2. Up the market stairs to the market. From the west bastion the bridge to the rock is seen: in A its gate is shut from
   the rock side, in B it has fallen.
3. Down the great well into the cistern, out onto the floor of the gorge, under the broken bridge first seen from above.
4. The lift of the rock: in A its cage is lowered from the lift house at the foot; in B the cage lies there, a frame climbed
   up the face of the rock (the causal rule).
5. On the plaza, the bridge gate opened in A: the way west is now short. Over the east bridge to the locks.
6. Up the locks to the water tower, along the upper town to the theatre, up the cliff stair to the graveyard.
7. From the cliff, the way to the top of the bell tower (to be designed) and the last moment.

### 7.5 The model checked

Fixed on 2026-11-24 and kept true by `city_model.py check`:

| Item | Now |
|---|---|
| Street heights | each street starts and ends at the height of what it meets (an earlier street, the bastion, a quay, the graveyard); between, half the ground and half an even climb; never steeper than its kind: streets and lanes 24 degrees, stairs 32 |
| The west bastion | the market street climbs to it (14 m); a gate tower on the bastion frames the bridge |
| The east of the rock | the east stair goes down from the east bastion (14 m) to the first quay of the locks (3 m) |
| The locks | a stair beside each lock head on both quays, 4.5 m over 8 m (29 degrees); the lock heads hold a fall of water in A |
| The north | the upper street runs from the top quay (21 m) to the top row of the theatre (30 m); the ring back to the market is gone |
| The cliff | the cliff stair climbs straight to the graveyard (42 m, z 78 .. 96) through a gate in the town wall |
| The harbour | the sea stair goes through the middle of the sea gate |
| The market | the market stairs pass west of the well, clear of its parapet |
| The centre | the tunnel from the well opens into the gorge under the west bridge (under the water of A); the lift: in A the cage at the plaza, in B its frame leaning up the face of the rock; the west bridge: its middle in A only, its gate on the rock end shut in A and open in ruin in B |
| The lantern | the street_b view shows its circle on the west bridge: the middle of the bridge of A standing in B |
| Still open | the way from the graveyard to the top of the bell tower (designed with the last puzzle) |

## 8. Puzzles by district (draft)

Each mechanic is taught where a mistake costs nothing, then turned round. The puzzles are designed on the fixed model.

| District | Taught or used | Sketch |
|---|---|---|
| Harbour and lower town | walking; the switch; the switch in the air; the lantern; the causal rule | a gate of A and a breach of B; a gap crossed by jumping in one moment and landing in the other; the first lantern on a bridge of A over a gap of B; a door opened in A that stands open in B. The crate puzzle of the customs-house cloister, re-made so that the crate is moved in A and lies in B where A left it, proved again by the solver, at most two switches |
| West: market and well | the orb; the lantern in a shaft | the wooden stair of A kept standing by the lantern in the dry shaft of B; the day bell at the foot |
| Centre: the rock | the causal rule at a large scale | the bridges, the lift cage and the gate of 7.4 |
| East: the locks | water and the crate | a crate floated in A to where it is needed lies there on the dry bed in B; the tower lock re-made the same way; the dawn bell in the water tower |
| North: theatre and graveyard | the prism; the switch in the air | the prism moves a thing of A into B; the fallen back wall of the theatre passed with a switch in the air; the night bell; the traces of the whole game seen from the cliff |

## 9. Story

The story is written last; nothing of the map or the code depends on it. Fixed now:

- Two moments of one town; told without words, by places and pairs of things in A and B.
- A story prop in A has a partner in the same place in B, and also points somewhere: to a way, a puzzle or a light. A
  prop that only decorates is cut. One to three in view from every place on the main path.
- B keeps a few things made by hands (a carved mark, a laid stone, a frame), so the present is remembered, not only ruined.
- Examples: a toy on the rim of a fountain in A, the same toy in the dry pool in B; fresh flowers in A, one grave apart in
  B; a drawing on a wall in A, behind a frame someone made in B.

**Optional twist** (decided at the story stage): the causal rule turns on the player at the end. Part of the ruin of B was
caused by what the player did in A: the sluice opened to pass a lock is what flooded the lower town. The choice of the
last moment becomes whether to leave the past as it was.

## 10. Immersion and guidance

- No text but key prompts. Light, material and sound guide: gold means use; in B the only warm light is the way.
- Sound: A has voices, bells and gulls; B has wind, dripping and the far sea. A switch is heard as much as seen.
- Each bell changes the whole town; the changes are seen, never needed elsewhere.
- Every puzzle room has a framing point at its entry, from which its goal and a part of the answer are seen at once.
- Hints grow with time in a room: the pale outlines of the other moment are always there; after 30 s they grow brighter,
  after 60 s they pulse (built). A refused move shows what is in the way (built).

## 11. Limits and proof

The body walks at 5.5 m/s and jumps 2 m; a running jump covers about 3.6 m (numbers in
[CONVENTIONS.md](CONVENTIONS.md)).

| Item | Main path | Optional lights |
|---|---|---|
| Horizontal gap | 1.8 m | 2.5 m |
| Step up | 1.2 m | 1.8 m |
| Switch in the air | most tried moments work | a few work |
| Landing | at least 2 x 2 m | at least 1 x 1 m |
| A miss costs | a stair back, or the lamp of the room | the lamp of the room |
| Slopes | streets 24 degrees, ramps 27, stairs 32 | - |

| Target | Proof |
|---|---|
| Push and crate puzzles | the solver: solvable, fewest moves, fewest switches, not solvable in one moment |
| Every room | walked by the real body; the known wrong moves tried |
| Skips | the search of `tests/test_reach.cpp`: no jump ends higher in a room not joined by walking |
| Recovery | every place of return is a place to stand, out of the water, in both moments |
| The ground | under every floor; every walk or jump off an edge ends in the map or is caught as a fall |
| Immersion | every view down meets the map, the land or the sea |

## 12. Order of work

1. Fix the model (7.5).
2. Export the model as data the game loads; the game draws and walks the model's town (done 2026-11-24: `data/town.dlt`,
   `scene/Town.h`; ARCHITECTURE.md, "The town").
3. The lantern circle and the traces in code, with their tests.
4. The districts rebuilt on the model with their puzzles (the whole town), each room proved as in section 11.
5. Refine by play; then the story and the sound.

## 13. Decisions

| Date | Decision |
|---|---|
| 2026-11-17 .. 23 | one rule; one city in two moments; the switch in the air; puzzles proved by a solver and the real body; anchor lamps, lamp travel, resonance, three endings; worlds told by material; water from the aqueduct (history in the CHANGELOG) |
| 2026-11-24 | A is the past, B is the present; the causal rule applies everywhere, and the crate puzzles are re-made in its direction |
| 2026-11-24 | the lantern keeps a circle of A in B; the switch stays a switch of the whole world |
| 2026-11-24 | puzzles start from the present; the player's changes show as traces |
| 2026-11-24 | the greybox rooms are not kept: the town is rebuilt on the model, as a whole |
| 2026-11-24 | dropped: puzzles that depend on another district, shortcuts back to places already done |
| 2026-11-24 | the seal of a district is folded into its bell: one goal per district |
| 2026-11-24 | the story twist of section 9 stays optional |
| 2026-11-24 | one design document (this one); the level design, the art and map plan and the gameplay notes are merged into it |

## 14. Open

| Item | Note |
|---|---|
| The story | section 9 |
| Sound and music | no audio in the project yet |
| The lantern | radius, a second lantern |
| Look before going | 3.1 |
| The memory lights | their number and places on the new map |
| A guide figure | a hint that shows one step after a long time in a room |
