# Level Design

The design of the map as built: the rules that give it a reason, the town in three dimensions, what each part teaches, and
how it is proved. The code points here by section number; section 14 keeps the rules of the current
build under the numbers the code uses.

## 0. Summary

A walled harbour town on a hillside by the sea, in two moments: world A, the town whole and empty in a held evening; world
B, the same town much later, in ruins. The body carries a lantern that moves it between the two: that is the layer switch,
the only rule. What happened between the two moments is the story, and it is decided last (section 1): nothing of the map,
the puzzles or the code depends on it.

The game starts on the quay of the harbour and climbs through the prologue to a sunken square with the cathedral in its
pool; the bell tower of the city stands on the cathedral. Round the square lie three districts, joined to one another and to
the square. Each district holds a seal (the main path) and a bell (rung where it hangs). The three seals open the cathedral;
every bell changes the city for good. At the top of the cathedral the last moment is played, and the world the body stands
in at its end decides the ending.

| Item | As built |
|---|---|
| Shape | one town on one ground: the harbour and the prologue in the south, the square in the middle, the west (the archive, the market, the well), the east (the canal, the lock walk, the tower lock, the water tower), the north (the north hall, the theatre, the graveyard); a ring of links between the districts |
| Goal | the three seals open the cathedral; its top starts the last moment |
| Bells | three, each rung in its district (or by its seal): they raise the water of A, turn the machinery, bring the night of A |
| Reward of exploring | nine hidden lights; each lights a lamp on the ring of the bell tower, two bring back a memory fragment |
| Return | shortcuts, lamp travel between lit lamps (after a bell) |
| End | three endings, chosen with the one rule (1.5) |
| Guidance | outlines of the other world, columns of light, prompts, resonance (after two bells), brighter outlines when stuck |

## 1. Story and the main line

### 1.1 Open

The story is decided last. Three drafts were discussed on 2026-11-23 and dropped, each because it needed a rule that did not
hold (a storm held by a bell that could not be finished alone; an escape by ship that the bells could not guide; warnings rung
in relay in a town small enough for one bell). What the map needs from any story is small, and it is fixed here:

| Need | Fixed as |
|---|---|
| Two moments of one town | A: whole and empty, at a held evening; B: the same town later, in ruins. Things made by people are whole in A; what fell is in B (14.3) |
| A reason to cross the city | three seals in three districts open the cathedral; a bell in each district changes the city |
| The switch | a lantern carried by the body; its flame and its glass tell which moment it shows (1.4) |
| The end | chosen by the world the body stands in at the last moment (1.5) |
| Placeholders | the times 7:11 and 7:12 in the art style (5.7) come from the drafts and are renamed with the story |
| Telling | without words, by the places and pairs of things in A and B (section 9) |

When the story is written, sections 1.2 to 1.5 and the examples of 9.2 are filled in with it; the rules stay.

### 1.2 The bells

Three bells, one in each district. A bell is rung where it hangs by walking up to it (`CityState::RingAt`), or by taking the
seal of its district (`kSealOfBell`); once rung it stays rung. What it changes, it changes for good and in both worlds, and
never on the main path, so any order finishes the game.

| Bell | Where it hangs | Seal | What changes |
|---|---|---|---|
| Dawn | in the water tower over the tower lock (6.4.2) | east | every water of A rises 0.3 m over 3 s; no place of return goes under water (TestRecovery) |
| Day | fallen to the floor of the great well of the market, in B (6.3.1) | west | the machinery turns by itself: the lock of the canal opens without its wheel, also after R; the weir does not |
| Night | in the arch of the night bell tower of the graveyard (6.5.2) | north | A eases to a night theme over 6 s |

The great bell hangs in the belfry of the bell tower on the cathedral (6.6): whole in A, lying cracked on the broken shaft in
B. It is the bell of the last moment (1.5).

### 1.3 The lantern

The lantern is the switch (1.4) and has two abilities, both practical, neither a new rule:

| When | Ability | What it does | Why |
|---|---|---|---|
| after the first bell | lamp travel | built (`scene/LampTravel.h`): at a lit anchor lamp, E opens the list of the lit lamps (a panel with the name of the chosen one); A and D choose, E travels, W, S or Space close it; the body stands while it is open. The body arrives at the place beside the chosen lamp, in the layer it is in, with the ring of a respawn. A lamp whose place is not free in that layer is passed over | the city is large and wound; going back is the reward of exploring, so going back must be quick and the player decides where to |
| after the second bell | resonance | built (`scene/Guidance.h`): a small light at the bottom of the screen pulses while a hidden light not yet taken is within 25 m, larger and faster as it comes nearer; when that light is more than 45 degrees off the view, a mark at the left or right edge shows its side | the lights are hidden on purpose; this says "here is one" without saying where |

### 1.4 The switch

The rule stays: one key moves the body between the two moments. What changes is how it feels and how much it tells.

| Change | How | State |
|---|---|---|
| Refused into water | a switch is refused into the water of the other world as into a wall (`scene/Recovery.h`): the body would only come up again at the entry of its room | built |
| Look before going | not built: a tap of Tab switches at once, as the switch in the air needs; holding Tab is used by the third ending (1.5) | - |
| Refused, and why | the blocker flashes in both layers (built for pushes); the lantern gutters | partly built |
| What never changes | the anchor lamps, the foot of the tower, the lantern itself, the shape of the hills | built (lamps, the ground) |

### 1.5 The endings

At the last moment the city flickers between its two moments; the world the body stands in at the end stays. Three endings,
written with the story:

| Ending | Chosen by | Shape |
|---|---|---|
| The past | standing in A | the held moment is kept: the town whole and empty for good |
| The present | standing in B | time moves on in B: rain, green over the ruins, people coming back |
| Both | the three bells rung, holding Tab through the last stroke (both worlds at once) | as the present, and every piece of A that the memory lights brought back stays standing among the ruins |

The memory lights are not a condition: they decide how much of A stands in the third ending. After an ending the body is back
before the last moment, so the other endings are seen without playing again.

As built (`scene/Ending.h`): touching the goal light, or standing on the balcony of the cathedral, starts the last moment:
the great bell strikes three times, 1.2 s apart, each stroke with a ring of light round the body, which may switch worlds
between them. At the third stroke: in A, "THE PAST" (the waters of A stay); in B, "THE PRESENT" (the master sluice drains
every water of A); with the three bells rung and Tab held through it, "BOTH" (drained, and the fragments brought back by the
memory lights stay). The end screen shows the ending as its title; R then puts the body back on the balcony, the waters full,
and the last moment plays again.

## 2. Goals and hard limits

| Goal | How it is judged |
|---|---|
| One rule | the body has one ability, the layer switch; objects react to it; the lantern only travels between lit lamps and points at lights |
| Hard in the head, not the hands | every main-path jump within the limits of section 10; precise jumps only for optional lights |
| One "of course!" per room | each room names its moment in one sentence; a room without one is cut |
| Teach, then turn round | the first room of an object has no dead end; later rooms use it the other way |
| One city, not parts | districts meet in seams (5.2), never at a line or a door; the same houses, streets and stone run through all of them; a district is told by its height, its landmark and its trade, not by a border |
| A world, not boxes | the city stands on a terrain; no view shows the void (the mountain, the sea, the hills, the ground under every floor); nothing floats (every platform stands on a pier, a beam, scaffolding or rock); walls in three parts (plinth, wall, cornice), broken edges in B |
| Never a long straight way | no street, lane or corridor runs straight for more than 15 m; streets follow the slope |
| The wish to explore | the tower is seen from everywhere; each district has a landmark; a view of the next goal from every high point; 12 memory lights that change the map |
| The wish to finish | the bells; the tower changes as they ring; the ring of twelve lamps round the tower shows the lights taken |
| Proved | push puzzles by the solver; every room walked by the real body; skips searched; every set of rung bells searched |

## 3. References

| Work | Borrowed | Where |
|---|---|---|
| Elden Ring (Stormveil, Leyndell) | the goal opened by what is won in the districts; layered space; seen before reached; one-way shortcuts; a landmark on the horizon | the seals open the cathedral; the bell tower seen from everywhere |
| Dark Souls | shortcuts back to a known rest point as the reward of exploring | anchor lamps, lamp travel |
| Zelda: Breath of the Wild | one mechanic per room, safe first, then combined | every object taught in a room without a dead end |
| Super Mario Odyssey | many uses of one mechanic; collectibles that appear after others | memory lights; three lights that appear only after a bell |
| Portal | one thing at a time, then turned round | the last room of each district turns its object round |
| Outer Wilds | a world that tells its story by its places; the ending decided by where the player stands | the clocks, the grave, the ghost; the last stroke |
| Hill towns of Italy and the Adriatic | houses turned to their streets, a square at the meeting of the streets, a town wall over the sea | the town (5.3, 5.5) |

## 4. Objects

Each object answers one question: what happens to it when the body switches layers?

| Object | Rule | Taught | Turned round |
|---|---|---|---|
| Layer wall / floor | exists in one layer (A: wood and iron; B: fallen masonry) | prologue, the gate | north: the prism moves it to the other layer |
| Crate, pushed | in both layers, so it obeys the walls of both; pushed along flagstones, one flagstone at a time | the courtyard | E2b: pushed in B, lifted in A |
| Plate and door | plate in one layer, door in the other; the door is open while the plate is pressed | the courtyard | west: the orb holds the plate while the body is in the other layer |
| Orb | carried, it switches with the body; put down, it stays in its layer and falls to the first surface of that layer; it works sockets and plates | W1 | W3: dropped down the well, it arrives at the bottom |
| Water (A) | not a floor for the body; things float on it; fed by the aqueduct (14.4) | the square, E1 | E3: the locks lift the crate step by step; the dawn bell raises all of it |
| Crate, floating | the same crate: in both layers; floats on the water of A, lies on the bed in B, moved in both at once | E1 | E2: moved in A, used in B |
| Wheel | machinery: turns only for a body in A | E1 | the day bell: machinery that turns on its own |
| Prism | stepped on in either layer, it moves the things linked to it into the other layer, for good | N1 | N2: one prism, two things; N3: scenery of A becomes a stair of B |
| Memory fragment | a piece of A that appears in B, in pale glowing stone, when its memory light is taken; solid in B | section 8 | the third ending |
| One-way door, grate, bars | open from one side only, and stay open | W4, the undercroft | the grate of the cellar, opened from the far side |

Not used on purpose: free physics, throwing, timers in the main path. They would move the difficulty back to the hands.

## 5. The city

### 5.1 The model

The target of the map is the model `tools/layout/city_model.py` (Python 3, numpy and matplotlib), drawn from over the sea in
both worlds:

![World A, the whole town at dusk](images/city_a.png)

![World B, after the storm](images/city_b.png)

    python tools/layout/city_model.py a      (world A; about ten minutes)
    python tools/layout/city_model.py b      (world B)

Its rules, which the game follows:

| Rule | Model | Game |
|---|---|---|
| Streets | a ring street round the cathedral quarter and streets out to the wall, houses in rows along them, turned to them | `Stage::TownStreets`: the ring and six streets; houses in rows along both sides, the blocks between filled, each house turned to its street; the streets paved on the ground (grass in B where the storm hit), a lamp every 14 m |
| Ground | the town on terraces stepping down to the sea; outside the wall a headland, hills, a stream valley east of the town, the mountain in the north | the headland, the stream valley, the mountain (`Landscape::NaturalHeight`); the town levelled at the street round the map |
| Damage | the storm came from the sea: the sea front and the harbour lie in ruins, the lower town is roofless, the upper town behind the square stands (`damage`) | `Stage::Damage`: houses in B are shells, or unroofed with their rafters left, by how exposed they are |
| Plants | A kept: lemon yards, olive rows, fields of grain. B let go: grass through the paving, figs in roofless houses, scrub over the groves, pines felled by the storm, reeds on the silted harbour | the same (5.6.4) |
| Puzzles | a room asks for one or two switches, at places it shows; the rest is walked in one world | 6.1 .. 6.5; the courtyard is proved to need two at the fewest (`BlockMaze::MinSwitches`, tests/test_maze.cpp) |

Where the rooms stand: the districts of the game already lie where the model puts them (the harbour and the gate in the
south, the square in the middle, the market and its well in the west, the locks and the water tower in the east, the theatre
and the graveyard in the north), so the rooms keep their places and their proofs. What made them read as a greybox inside a
town was their look: the stone walls of the halls now wear the plaster of their quarter with windows high up, weathered in B
(`Stage::HallFacades`), and the square is a square of the town with a blind arcade round its walls (`Stage::SquareArcade`).
What stays different from the model: the prologue runs on a terrace, not on the wall walk; the square is a rectangle; the
cathedral stands in its pool rather than a round tower in a round square.

### 5.3 One town, four kinds of house

The districts are not islands round the landmarks: the same town runs through all of them, and its houses change slowly.

| Kind | Where it is strongest | Walls, roofs | Its own detail |
|---|---|---|---|
| Market house | west | ochre plaster, deep red tiles | an arcade on the street: shops under a portico; in B its pillars stand in a row with gaps |
| Lock house | east | whitewash, light tiles | built on the quay: its back wall is the wall of the canal |
| Hill house | north | grey stone, slate | small, on the hillside beside the theatre |
| Gate house | south | sandstone, red tiles | the plainest; the first houses the player walks past |

The colour of a house is a blend of the four by its distance to the middle of each district (a gaussian of 22 m), so a
street that runs from the market to the locks passes from ochre to white without a border. The square takes a little of each
side: arcaded toward the market, white toward the locks, grey toward the theatre.

In B one house in three is a ruin: no roof, its four walls broken to different heights, rubble and a young tree inside.
Arcades lose every other pillar. The rest keep their roofs, darker and grey. The town in B is the same town, not another.

### 5.4 The ground, the sea and the edges

The ground is shaped as land is shaped, not drawn as slopes. In the game it is a height field (`scene/Terrain.h`,
`scene/Landscape.h`): 2 m cells round the map and 8 m cells out to 300 m past every room, flat-shaded and baked into the static
batch. The fine field is pinned to the coarse one along its edge, so the two meet without a crack.

| Part | In the game |
|---|---|
| The town | levelled at the street, 5 cm under the floors of the map (`kPlainY`): the houses stand on it |
| The land | outside the town, noise: hills to the east and the west, a hillside under the mountain, the mountain to the north in ridged layers up to about 70 m, rough ground, rock where it is steep |
| The coast | a bay in front of the town, the shore coming north on either side; the bank falls from the town to the sea at -9, the sea floor further down |
| Pits | where the map has no floor at street level (the open sides of the prologue, the trench, the gap under the sky bridge, the pool, the lower west, the cellar and the canal, the locks, the well, and closed spaces between districts) the ground steps down to a sunken lane a terrace step (1.6 m) under the street, and deeper only under a room that goes deeper, just under its lowest floor (`kPits`, `PitFloorAt`). Only the harbour is a basin of the sea. Paved in A, grown over in B |
| Footings | a wall at street level or a deck at the edge of a pit gets a footing of rock down to the floor of the pit, drawn only, so that nothing floats; never through another box or a room below |
| Collision | the ground is solid; a body that comes to stand on it has fallen out of the map and comes back like after a fall (`OnOpenGround`: grounded on the ground, not on a floor, a ramp or the edge of a doorway). Tests prove that every walk and jump off an edge ends in the map, below its fall line, or on the open ground where it is caught; and that no place of the map is taken for a fall (test_terrain) |

| Edge | What is seen | Why it is never the void |
|---|---|---|
| South | the bay, the open sea | the sea is one plane round the eye, wider than the far plane, its colour the colour of its world |
| West, east | hills, the coast turning north | the land reaches 300 m past every room |
| North | the hillside, the mountain in layers | the land reaches 300 m past every room and rises |
| Down | the sunken lanes, the sea in the harbour | the lanes have floors; the sea covers the harbour |

The sea without an edge: one flat plane at -9 that follows the eye (in steps of 20 m, so its surface does not slide), 720 m
across. The fog of each world is the colour of its sky, so the sea fades into it with no line where it ends.

### 5.5 Where buildings stand, and why

A building in the wrong place is wrong in a way the player feels before they can say why. The game places the houses of the
town (`Stage::SkylinePlots`) by these rules.

| Thing | Stands where | Never where | In the game |
|---|---|---|---|
| Houses of the town | on plots along streets round the cathedral, each turned to its street (its walls face the cathedral or run round it, within a few degrees), sized so that it stays on its plot | within 4.5 m of a room (a jump never lands in a house it would fall through), over a pit, under the aqueduct | built |
| Colour | by quarter: ochre plaster and deep red tiles in the west, whitewash and light tiles in the east, grey stone and slate in the north, sandstone and red tiles in the south, blended by distance (no quarter has a border) | | built |
| Towers | now and then a tower over the roofs | | built |
| Gardens | one plot in eight is not built: a garden with lemon trees and a cypress | | built |
| B | by how exposed it was to the storm from the sea (`Stage::Damage`): by the sea most houses are shells (walls broken lower, a fig inside), in the lower town many are unroofed (the rafters left), behind the square most stand; a quarter of the merlons and a third of the cornices have fallen | | built |

### 5.6 Plants

Plants are placed by where they would grow, never on a grid (`Landscape::PlacePlants`, built). Every plant answers three
questions: what grows on this ground, how thick, and who planted it.

#### 5.6.1 Ground decides the kind

| Class | Where | Plants |
|---|---|---|
| Shore | under 1.5 m over the sea | none |
| Town | the levelled town | only in the gardens (5.5) and along the road round the town |
| Rock | steeper than 0.95 (about 44 degrees) | none |
| Steep | 0.42 .. 0.95 | low shrubs in drifts |
| Windy | over 14 m, or within 30 m of the coast | umbrella pines in clumps, smaller than inland |
| Sunny slope | the slope faces south or east, gentler than 0.42 | olive groves, in rows along the contour (every 3 m of height) |
| Shaded or flat | the rest | holm oaks in woods with clearings |
| High mountain | over 40 m | pines thinning to bare stone (none over 70 m) |

#### 5.6.2 Density, clumps and edges

| Rule | How |
|---|---|
| No grid | candidates on a jittered grid of 2.6 m; each kept only at a distance from every other plant, set per kind (shrubs 1.6 m, cypresses 3.5, olives 4.5, oaks 5, pines 6) |
| Clumps and clearings | a low-frequency noise decides where woods are; the edges thin out |
| Size | each plant 0.6 .. 1.4 of its kind, smaller where the wind is |
| Never | within 4.5 m of a room, over a pit or at its edge, in the water, on rock |

#### 5.6.3 What people planted

| Thing | Rule | State |
|---|---|---|
| Lemon trees, a cypress | in the gardens of the town | built |
| Cypresses | in a line along the road round the edge of the town | built |
| Olive groves | in rows along the contour of sunny slopes | built |

#### 5.6.4 B: what grew back

| Change | State |
|---|---|
| Grass through the paving of the streets where the storm hit hardest (`Damage`) | built |
| Figs grown up through the roofless shells | built |
| Scrub over the olive groves | built |
| The pines nearest the sea felled (a log and a stump) | built |
| The harbour silted: sand banks with reeds | built |
| Fields of grain in A gone wild in B; the lemon trees of the gardens dead | built |
| Ivy on north faces | open |
| Never a plant where a puzzle of B needs a clear floor; plants are drawn only | rule |

### 5.7 Art style: lantern and stone

A style that the engine already draws, made into a rule set rather than a lack of detail.

| Rule | A (7:11) | B (now) |
|---|---|---|
| Form | low-poly, every face flat-shaded; silhouettes are read first: towers, roofs, cypresses, the bell | the same forms, broken: edges chipped, walls ending at different heights |
| Colour | a small warm palette: sandstone, terracotta, whitewash, olive green, the gold of a held sunset | the same palette washed toward slate and teal; moss on top faces (built: the theme of B) |
| Light | one low sun from the west, warm; long shadows; windows lit | one cold light from above, rain in the air; only the lantern is warm |
| Air | a light haze that turns distant things the colour of the sky | a thick fog: B is smaller to the eye than A, which makes A feel like a memory |
| Gold | only things that can be used: wheels, levers, bells, the lantern | the same |
| No | textures, outlines on everything, photographic detail | the same |

Real places it borrows from, without copying any: the walls and red roofs of Dubrovnik and Kotor on the sea; the terraces
and lemon gardens of the Amalfi coast; the Greek theatre of Taormina, cut into a hill over the sea (the theatre of the
north); the stone of Matera (the market well); the campanile of St Mark's in Venice, which fell in 1902 into a heap at its
own foot (the way the tower of B fell). Games of a similar look: the colour and haze of Journey, the flat-shaded forms of
A Short Hike.

### 5.8 The scenery round the map

Round the playable map the town carries what no body reaches, drawn only and baked with the ground (`scene/Scenery.h`):
the harbour, the town wall, the doors and windows of the houses, what fell of the water tower in B, the street lamps.
Houses keep out of the districts (`Stage::kZones`).

| Part | A | B |
|---|---|---|
| The harbour | the quay walls, the sea wall, a mole and a lighthouse, boats, ships far out | the mole broken, the lighthouse a stump, a wreck |
| The town wall | merlons and towers round the town | a quarter of the merlons and some towers fallen (never beside a room) |
| The houses | a door toward the cathedral, upper windows, some lit | the shells without windows |
| The water tower | (in the map, 6.4.2) | its top lying south of it |
| The streets | a lamp post with a lit lantern at every other crossing | the post only |

### 5.9 Water

Water is the other half of the rule: what is a place in B is water in A. It is drawn after everything else and seen through
(`render/Models.cpp`), and the shader gives each kind its own life (`shaders/models.hlsl`, `scene::kMatWater` ..).

| Kind | Where | How it looks |
|---|---|---|
| Still water | the pool, the trench, the cellar, the canal, the locks, the well | small waves that move the light on it; the sky in it where one looks across (Fresnel), the bed through it where one looks down; the sun as a glint |
| The sea | the bay and the harbour | long swells, white crests on the highest, denser than still water, the sky in it to the horizon |
| Running and falling water | the channel of the aqueduct, its fall into the pool, the spill of the trench, the falls between the locks | streaks along the flow, white water running down the falls, each column at its own pace |
| Foam | a rim along the walls of every water, the foot of every fall, splashes | moving flecks |
| The wet band | the walls of a water that has gone down | dark, on the walls only, drying in 25 s |

What water does (`scene/WaterFx.h`):

| Moment | What is seen |
|---|---|
| The locks hold water | the water of each lock falls through the lock head into the one below (three falls, with foam at their feet) |
| Water rises (the dawn bell, the canal lock) | the surface and its rim of foam rise; no band |
| Water goes down (the canal let out, the master sluice at the end) | the walls stay wet where it stood, dark, drying |
| A body falls in | a splash: two rings of foam spread, drops fly up and fall back; the body sinks for half a second with the view tinted under the surface, then comes back at its lamp; no switch while it sinks |

## 6. Rooms

Each room states its purpose, its solution, its "of course!", and how it is proved. Coordinates are world metres.

### 6.1 Prologue

Seven beats without a word of text (section 14.6): each beat teaches one thing, first where a mistake costs nothing.

| Beat | Teaches | As built | A miss costs |
|---|---|---|---|
| 0 | walking, looking; where the goal is | the quay of the harbour (6.1.2): a stair between two harbour walls up to the south door of the guardhouse | nothing |
| 1 | a safe place; the one thing the switch does not change | the first anchor lamp, on the terrace | nothing |
| 2 | the switch | the town wall: an iron gate in A, a breach in B | nothing |
| 3 | neither world is "the better one" | a heap of B closes the terrace; in A the way is clear | nothing |
| 4 | the switch in the air | a shallow ditch: a beam of B, planks of A, stairs back up | a walk of a few seconds |
| 5 | the same, where a miss is a fall | the gap: beam of B, planks of A, curtain of B | a walk back up |
| 6 | the first "of course!" | the courtyard puzzle (10 pushes), a cloister (6.1.1) | R |

#### 6.1.1 The courtyard as a cloister

The block puzzle stays (its layout and its proofs are those of `scene/Courtyard.h`); what changes is what it is. Decided on
2026-11-23: a purple block on a grid looked like a level of another game in a town of stone and wood, and it was the one
object used once and never again.

| Part | Before | Now | Why |
|---|---|---|---|
| The block | a purple cube | the crate of the town (the wood of the crate of the canal), with three iron bands | one object, two lessons: here it is pushed, on the canal it floats (4) |
| The grid | an abstract grid | the flagstones of the yard of the old customs house, a joint on every line | a move of one cell is a move of one flagstone; the crate slides from one to the next |
| Walls of A | red boxes | stacks of crates of the warehouse, a joint at every crate | made by people, whole: the rule of materials (14.3) |
| Walls of B | blue boxes | masonry fallen from the arcade, its top broken | left by the ruin (14.3) |
| The plate | a red slab | a brass slab let into the flagstones, gold: a thing to use | gold means use (9.3) |
| The yard | four plain walls | a cloister: a blind arcade (pilasters, arched heads, a string course) on its west, east and north walls; in B the string course broken and moss in some joints | a place with a past, not a test chamber |

Every drawn part of the cloister is a joint flat on the floor (under 2 cm), a part of the arcade within 11 cm of a wall, a
joint on a wall of the puzzle, or above the walls: nothing is in the way of the body or the crate (`TestCloister`). The crate
is pushed in both layers; the arcade and the materials are the same in A and B except where B is broken.

From the terrace the town is seen over the gate before it is entered.

#### 6.1.2 The harbour (beat 0)

| Part | As built | Why |
|---|---|---|
| The quay | x -14 .. 14, z -56 .. -46, 1.5 m over the sea, in a basin of the sea under the town wall; the start (room `quay`) and the first anchor lamp, the lantern of the quay | the first sight: the town on its wall over the water |
| The stair | between two harbour walls, 3 m wide, 8.9 m up over 18.5 m (26 degrees), to a landing at a new south door of the guardhouse (room `sea stair`) | the way in is one way, safe, and walked; the walls keep a sideways jump on the stair (TestHarbour) |
| The guardhouse | now the sea gate: a south door to the harbour, the north arch to the terrace as before | the old beat 0 becomes the threshold |
| The basin | side walls, a sea wall with merlons beside the guardhouse, a mole with a lighthouse to the south-east; in A boats moored and ships under sail far out; in B the mole broken, the lighthouse a stump, a wreck on the west side | the two moments are told before the first switch; drawn only, 4.5 m or more from the quay (TestScenery) |
| The sea | a fall: off the quay the body falls below the fall line of the quay and comes back at its lamp | as everywhere |

### 6.2 Hub: the sunken square

| Part | As built | Why |
|---|---|---|
| Rim | a walk at street level along the four walls of the square; the openings of the districts are on it, under arched heads | the square is crossed without a jump; every district starts at its edge |
| Pool | 1.5 m deep round the cathedral. A: water (fed by the aqueduct). B: dry, its floor walked, with stairs in two corners | the rule said by the square itself |
| Causeway | from the courtyard door to the door of the cathedral | the first sight from the courtyard is the door, closed, and the bell tower over it |
| Undercroft | under the gallery in the south-east, from the canal (14.8) | the way in comes from the far side |
| Cellar | under the courtyard, the link from the canal to the west (14.11) | the ring of districts closes under the first puzzle |

### 6.3 West: the archive, the market and the well (the orb)

Theme: one body cannot be in both worlds; a thing can. The district goes down. From the west opening of the square the west
hall (the archive) holds W1 and W2; its ramp leads down to the lower room and the vault (W3, W4); a door in its west wall
opens onto the lane to the market and the well (W5). The ramp up from the lower room is the shortcut back to the square; the
passage of B under the ledge links the hall to the north hall, and the cellar links it to the canal.

| Room | Moment | Where |
|---|---|---|
| W1 hand it over | the orb of A, switched with the body, fits the socket of B: it lays a bridge of B over a trench 5 m wide | the west hall (`scene/WestWing.h`) |
| W2 leave it | the orb on a plate of A holds a door of B open; a lever inside keeps it open, then the orb is fetched | the west hall, the foot of the ramp |
| W3 let it fall | in A the orb lies on a lid; in B it falls through onto a socket that opens the bars | the lower room and its mezzanine |
| W4 the seal | the seal of the west, behind the bars | the vault |
| W5 the well | the stair of A and the water of A split the shaft; the day bell lies at its foot | the market (6.3.1) |

#### 6.3.1 The market and the well (W5)

A door in the west wall of the west hall opens onto a lane west to the market (rooms `west lane`, `market`, `well`). The
market is a walled square (the backs of the houses round it) with eight stalls (a stone counter each; in A an awning of
cloth over it, solid, so the awnings are a roof walk of A; in B the bare posts and rotten crates), the clock tower (solid to
11 m; its open top with the day bell in A, broken in B), the lamp of the market, and the great well.

The well is a shaft 12 m across and 8 m deep, walled by a parapet 1 m over the square, open over its way in. A stair runs
down its four walls from corner landing to corner landing, 2 m down over 8.4 m each: east (both worlds), south (a wooden
stair of A; in B it has fallen to the floor), west and north (both). The water of A stands 3 m deep in the shaft (fed from the
trench of the west hall): over the west and north stretches. So the way down is: in A down the east and the wooden south
stretch to the south-west corner, 1 m over the water; switch to B; on down the dry stair to the floor, where the day bell lies
and a hidden light waits. The way back up is the other way round: in B up to the south-west corner, where the fallen stretch
stops the body; switch to A; up the wooden stair. Its "of course!": the stair that is there in one world and the water that is
there in the other split the shaft into two halves, and the corner between them is where the world is changed.

A fall from the square into the shaft is a fall (back to the lamp of the market); on the stair, a fall to the floor is not.

### 6.4 East: the canal, the locks and the water tower (water and the crate)

Theme: a crate floats in A and lies on the bed in B; moved in one, it moves in both. The district climbs.

| Room | Moment | Where |
|---|---|---|
| E1 the lock | the wheel (A only) raises the water; the crate rises with it and becomes the step onto the quay | the east hall (14.8) |
| E2 the drift | the weir lets the crate drift in both worlds; in B it is the step into a breach: moved in A, used in B | the east hall and the undercroft (14.8) |
| E3 the locks | three basins beside a climbing walk: water in A, dry rooms in B, a light in the middle one | the lock walk (6.4.1) |
| E4 the tower lock | the crate pushed in B floats in A under the door of the water tower, where the dawn bell hangs | the tower lock and the water tower (6.4.2) |

#### 6.4.1 The lock walk (E3)

A door in the east wall of the east hall opens onto a walk that climbs east beside three locks to a top terrace under the
water tower (room `lock walk`). Four terraces (floors -1, +1, +3, +5) are joined by stairs that climb along X (27 degrees);
south of the first three, a basin each, 2.5 m deep. In A each basin holds water 0.6 m under its terrace, fed from the
aqueduct and each from the one above (`Stage::kLockWaters`, `kWaterFeeds`); a body that goes in comes back at the lamp of the
locks, and a switch into it is refused. In B the basins are dry rooms, each with a stair back up at its east end; on the bed
of the middle one lies a hidden light (the lesson of the canal bed in a room of its own). Between two basins a lock head as
high as the next terrace. Walls 8 m high close the walk on the north (the backs of houses), the east and the south.

#### 6.4.2 The tower lock and the water tower (E3 and E4)

South of the top terrace a fourth basin, the tower lock, lies against the foot of the water tower (rooms `tower lock`, `water
tower`). Its bed is 3 m under the terrace, reached by a stair at its east end; in A it holds water 1 m under the terrace. A
crate (the wood of the crates of the town) lies in it by the terrace (`scene/LockCrate.h`). The door of the water tower is in
the south wall, 0.6 m over the terrace but 6 m from it, and 2.4 m over the crate as it lies in B.

The puzzle, "pushed in B, lifted in A" (E2b and E3 in one room): in B, on the dry bed, the crate is pushed south, a step at a
time, four steps to the wall under the door; in A it cannot be pushed (it floats on its mooring), but it floats wherever B left
it, its top 0.5 m over the water. So: push it under the door in B, climb onto it, jump and switch to A in the air: the body
lands on the crate of A, 1.3 m higher, and the door is a hop up. Every other way fails (TestTowerLock, the search for skips):
the crate of A at its start is too far from the door, the terrace and the lock head west of the basin are too far, the crate
of B too low. A parapet on that lock head adds a margin (the search finds no skip without it either). A switch on the bed into the water of A is
refused. R in the tower lock puts the crate back.

Inside the tower (E4) the dawn bell hangs low in a vaulted room; walking under it rings it (`CityState::RingAt`), as the east
seal does: the waters of A rise. Over the room the shaft of the tower is drawn (in A to the tank, a landmark; in B broken).

### 6.5 North: the north hall, the theatre and the graveyard (the prism)

Theme: not the body changes world, a thing does. The north hall climbs by two ramps to a landing and a ledge into the square;
a door under its first landing leads to the theatre, and the back of the theatre to the graveyard.

| Room | Moment | Where |
|---|---|---|
| N1 change a world | a gap with a bridge of A closed by a portcullis of A; the prism moves the portcullis into B | the north hall (14.9) |
| N2 two at once | one prism moves a cage of B into A and a step of A into B | the north hall (14.12) |
| N3 the breach | from a floor of A, a jump through a breach of B with a switch in the air | the theatre (6.5.1) |
| N4 the night bell | rung in the graveyard | the graveyard (6.5.2) |

#### 6.5.1 The theatre (N3)

A door under landing 1 of the north hall opens onto a lane that climbs to a door in the stage wall (rooms `theatre lane`,
`theatre`, `theatre terrace`). The stage stands 1 m over the orchestra, with a stair down from its middle; seven straight rows
of seats rise north from the orchestra, 0.55 m a row (climbed by jumping, or by the aisle stair at their east end), to a
promenade 3.85 m over the orchestra; the back wall behind it. Drawn: the upper stage wall with its columns and the painted
scenery of the evening's play (A), broken in B; grass in the orchestra (B). The lamp of the theatre stands beside the stage.

In B the upper three rows and the promenade west of x -2 have fallen into a heap of rubble, and the back wall has a breach
there (in A a wooden panel). N3: in A walk the promenade west to the panel; jump at it and switch to B in the air: the panel
is gone, and the jump carries the body through the breach onto a terrace behind the theatre, where a hidden light waits (the
lesson of the gap of the prologue, the other way round: a floor of A, a way of B). Without the switch the panel holds; in B
the west promenade is not there, and no jump from the east promenade or the rows reaches the breach (TestTheatre, the search
for skips).

#### 6.5.2 The graveyard and the night bell (N4)

A door in the back wall at the east end of the promenade opens onto a short lane north to a walled yard on the hill, level
with the promenade (rooms `graveyard lane`, `graveyard`): three rows of graves (low kerbs, drawn headstones; fresh flowers in
A), the night bell tower in its north-east corner with the bell hanging low in its open arch, the lamp of the graveyard (the
eleventh), and in B a lone grave apart, with a stone lantern (9.2). Its parapet (3.2 m) shows the hill and the town wall
beyond and holds a jump, also from the pedestal of the lamp. Walking up to the bell rings the night bell (`CityState::RingAt`);
it stays rung, as if the north seal had been taken: night falls in A. The graveyard lies inside the town wall: outside it
the ground is the hillside of the world.

### 6.6 The cathedral and the bell tower

The cathedral stands in the pool of the square; its door opens with the three seals. Inside, six ramps wind up its walls to
a balcony 12 m over the rim, with the goal light (the last moment, 1.5). Over its north wall rises the bell tower of the
city, the landmark seen from every district (`Stage::CathedralDecor`): a shaft to the belfry at 17 m; in A four piers, the
great bell and a spire in three storeys to a gold tip at 28.5 m; in B the shaft ends at the belfry in broken teeth and the
great bell lies cracked on it. Round the foot of the belfry burns a ring of lamps, one for each hidden light, lit as the
lights are taken (`StageLights::AddLightRing`): the progress of the player, seen from all over the city.

## 7. Views and turns

| Moment | What is seen | Turn |
|---|---|---|
| The quay | the town wall over the water, the bell tower over the roofs | the goal first |
| The terrace | the gate, the town beyond it | the way ahead seen before it is walked |
| The square, B | the dry pool and its floor | a place, seen long before it is used |
| The rim, east | the bars of the undercroft, a light behind them | the way in comes from the far side |
| The parapet of the well | the bottom and the fallen day bell | the goal under the feet |
| The top of the lock walk | the town falling to the sea, the water tower over it | the square from above |
| The promenade of the theatre | the square and the bell tower; in B the breach in the back wall | the way on, seen before it is known |
| The balcony | the whole town, and the ring of the lights taken | the end |

## 8. Memory lights

Nine hidden lights, each a second use of something taught. Taking one lights a lamp of the ring round the bell tower (6.6);
two bring back a memory fragment in B (`scene/CityState.h`).

| # | Name | Where | Taken in | Lesson |
|---|---|---|---|---|
| 0 | Inside the gate | the arch of the town wall of the prologue | B | the breach is the way |
| 1 | The floor of the pool | the pool of the square | B | a water of A is a place in B (brings back the steps of the fountain) |
| 2 | The bed of the canal | the canal of the east hall | B | the lesson of the pool, again |
| 3 | Behind the bars | the undercroft | B | the way in comes from the far side |
| 4 | In the wall of the north hall | over the heap of N2 | B | one prism, two things |
| 5 | Under the courtyard | the cellar | B | the first puzzle, from below |
| 6 | The bottom of the middle lock | the lock walk | B | a lock is a room when it is dry |
| 7 | The bottom of the well | the floor of the great well | B | the stair of A and the water of A |
| 8 | The terrace of the theatre | behind the breach of the theatre | B | a floor of A, a way of B |

The light inside the gate brings back the bench by the gate.

## 9. Guidance and story without words

The scene says where to go and what happened; words are only key prompts. Everything in this section is drawn only: none of
it is solid unless a room says so.

### 9.1 Rules of environmental storytelling

These rules hold whatever the story becomes (section 1).

| Rule | Meaning |
|---|---|
| One place, two moments | a story prop of A has a partner in the same place in B; together they say one thing. A shows the moment that was held (what was left), B what came after (what was kept, mended or remembered) |
| Every prop has a second use | it tells the story and points somewhere: to a way, a puzzle, a memory light or the next goal. A prop that only decorates is cut |
| Seen before reached | a story prop is placed where the player sees it from the way before they walk to it: in the frame of a door, at the end of a street, from a height |
| The story follows a route | the props of A thicken along the route people took on the night that A holds; a player who walks it the other way reads it backward |
| Density | at least one story prop in view from every place to stand on the main path; at most three, so that each one is noticed |
| Kept, not destroyed | B keeps a few things the ruin spared or people saved: a memorial is made by hands (a carved mark, a laid stone, a frame), not by chance |
| No text | numbers and symbols only (a clock, an emblem, a mark, a drawing) |
| Gold means use | gold is never a story prop; story props are in the colours of the town |
| Fragmented | the story comes in pieces in any order (the memory lights, the echoes of the bells, the props); the main path never needs it (Breath of the Wild, Dark Souls and Elden Ring do the same) |

### 9.2 Pairs of props

Where they stand is fixed by the rules; what they are is written with the story. The places:

| Place | A | B | Second use |
|---|---|---|---|
| The first view | what the town was doing at the held moment | a memorial of it | the first sight; the lantern |
| The road to the gate | things left on the way | the same things, rotted; one of them made into a seat facing the view | marks the way |
| The gate | a sign that points the way people went | the same sign, deepened by hand | read backward, it points into town |
| The market | goods left on the stalls | one stall mended, a thing set on it like a shrine | the first step onto the awnings (light 6) |
| The well | a bucket at the top, its rope cut | the bucket at the bottom, the rope across the spiral | shows where the spiral fell in B |
| The locks | washing on lines, a boat in the shed | the boat pulled up, names cut into its hull | under the breach of E2 |
| The theatre | the scenery of that evening's play | something laid on the stage in a ring of stones | the scenery is the stair of N3 |
| The square | a toy left on the rim of the fountain | the same toy in the dry pool | on the way across the floor of the pool |
| A house at the foot of the tower | a table laid for two, a drawing on the wall | roofless; the drawing behind a frame someone made | the drawing shows the tower and the three bells: the goal |
| The graveyard | fresh flowers | one grave apart from the others | a memory light |
| The tower door | three dark emblems | the same | lit by the bells: the main line at a glance |

### 9.3 Guidance

| Means | Use |
|---|---|
| Light | warm lamps of A along the ways of A; shafts of light in B on the breaches; gold is always a thing to use (wheels, levers, bells) |
| Flow | the water of A runs down the locks into the pool; in B the wind carries dust toward the openings |
| Birds | pigeons in A sit only on edges that a jump reaches, and fly to the next one when the body comes near |
| Fallen things | a fallen column or a broken wall points to the way out |
| Marks | marks on the walls, lit by the lamps of A after the night bell: arrows to the memory lights |
| The framing point | every puzzle room has one place at its entry from which its goal, the thing to use and a part of the answer are seen at once, through a door or a window |

A guide figure (whose story is open) is the hint system: a pale figure that walks a way through the room and shows one step
("jump here, switch in the air"). Followed in order, its walks are one route through the city, which ends at the top of the
tower.

| Time stuck in one room | What appears |
|---|---|
| always | the scene (above), outlines of the other layer, columns of light over things to use |
| 30 s | the outlines of the other world in the room grow brighter over 5 s (built: `Guidance::BrightenRoom`) |
| 60 s | they pulse (built); the guide figure that would show the key step is not built |
| a refused move | the blocker flashes in both layers (built); a refused switch shows what is in the way in the other layer |

R puts the body at the lit lamp of its room, or the entry of the room, and puts back the objects of that room. Lights,
bells and shortcuts stay.

## 10. Jump limits

The body walks at 5.5 m/s and jumps 2 m high; a running jump covers about 3.6 m.

| Item | Main path | Optional lights |
|---|---|---|
| Horizontal distance | 1.8 m | 2.5 m |
| Step up | 1.2 m | 1.8 m |
| Moments that work for a switch in the air | most of the tried moments | a few |
| Landing area | at least 2 x 2 m | at least 1 x 1 m |
| Cost of a miss | a net, a stair back, or back to the lamp of the room | back to the lamp of the room |
| Switches in the air required | the ditch, the gap, the breach of the theatre, the tower lock | - |
| Slopes | streets at most 24 degrees, ramps at most 27, stairs at most 32 (the seats of the theatre: 27) | - |

Walls that must not be climbed are out of reach of a jump from every place a body can stand next to them: a surface that
must not be reached is at least 2.3 m over every floor in reach of it, and a parapet round a high floor is 2.6 m. The
search for skips checks this for the whole map.

## 11. Proof

| Target | Method |
|---|---|
| Push rooms | the solver: solvable, fewest pushes, not solvable in one layer, dead ends (none in a teaching room) |
| Orb, crate and prism rooms | a walk with the real body; the known wrong moves tried; every prism state |
| The map | a walk from the start to the belfry; every room entry standable in both layers |
| Skips | the search of `tests/test_reach.cpp`: no jump ends higher in a room not joined by walking, with every fragment present. The bells change no solid (only the water of A, the machinery and the night), so one search covers every set of bells |
| Recovery | every place of return (the lamp of a room, else its entry) is a place to stand in both layers and out of the water, with the waters full, raised by the dawn bell and drained, the canal low and high (TestRecovery, built); a switch into water is refused like a switch into a wall (built); every arrival of lamp travel (with lamp travel) |
| The ground | at every point of every room, in both layers, the ground is under the highest floor; from every place to stand at the edge of its box, a walk off it and a running jump off it, outward and at 45 degrees, never end standing on the ground of the world (tests/test_terrain.cpp, built) |
| Immersion | from every room entry, lamp and the balcony, rays looking down in 16 directions meet the map, a house, the land or the sea within the far plane; the land reaches 300 m past every room; the sea is wider than the far plane round any eye (built). Every walked surface stands on something (footings, built at the pits); every puzzle room has its framing point (with the rooms) |
| Fragments | the search for skips with every memory fragment present: a fragment never skips a puzzle (built: two fragments) |
| Endings | the world at the last stroke decides; the third ending only with the three bells and Tab held (TestLastMoment) |

## 12. State

| Part | State |
|---|---|
| The ground, the sea, the plants, the town, the scenery | built (5.3 .. 5.8) |
| Rebuild after the model, step 1: the ground of the model, the pits as sunken lanes, the open-ground rule, the red and blue edges gone, A and B told apart by damage and plants | built 2026-11-23 (5.1, 5.4) |
| Rebuild after the model, step 2a: the streets and the houses in rows along them; the courtyard proved to need two switches | built 2026-11-23 (5.1) |
| Rebuild after the model, step 2b: the halls clad as houses of their quarters, the square arcaded; the rooms keep their places (5.1, "Where the rooms stand") | built 2026-11-23 |
| Recovery (no switch into water, places of return out of water) | built (14.4) |
| The prologue, the harbour, the courtyard | built (6.1) |
| The square, the cathedral, the bell tower, the ring of lights | built (6.2, 6.6) |
| The west: W1 .. W5, the day bell | built (6.3) |
| The east: E1 .. E4, the dawn bell | built (6.4) |
| The north: N1 .. N4, the night bell | built (6.5) |
| The bells, the fragments, lamp travel, resonance, the stuck timer | built (1.2, 1.3, 9.3) |
| The last moment and the three endings | built (1.5) |
| The story, the props of 9.2, the guide figure, music | open (14.10) |

## 13. Decisions

| Date | Decision |
|---|---|
| 2026-11-22 | one city in two moments; one rule; the objects of section 4; the prologue in seven beats; the jump limits; R puts back the room |
| 2026-11-22 | the vertical city (section 14): districts by height, the aqueduct, worlds by material, the logic of the moving parts, anchor lamps |
| 2026-11-23 | the story is decided last (1.1): the drafts were dropped; the map keeps only what any story needs |
| 2026-11-23 | the ground under the map, recovery, the town turned to its streets, plants by their ground (section 5) |
| 2026-11-23 | the block of the courtyard is the crate of the town, in a cloister (6.1.1); the game starts at the harbour (6.1.2) |
| 2026-11-23 | the districts grow out of the wings: the market and the well (west), the lock walk, the tower lock and the water tower (east), the theatre and the graveyard (north); each holds a bell rung where it hangs |
| 2026-11-23 | the three endings, lamp travel, resonance and the stuck timer (1.3, 1.5, 9.3) |
| 2026-11-23 | the first plan is dropped: the model of a round square on a hillside, the tower in the square and its floors, the cistern, the separate bell tower west of the town. The cathedral carries the bell tower and the ring of the lights (6.6); the map is one town round the square |
| 2026-11-23 | the model (5.1) is the target of the map again; the first plan that was dropped is the greybox, not the model. The rebuild in two steps: first the ground, the look and the two worlds told apart; then the rooms at their places |
| 2026-11-23 | the courtyard stays as it is: the new count of switches (`MinSwitches`) shows it needs two, the number a room may ask for. The rooms are moved to the places of the model in a step of their own (2b), so that each move is proved on its own |
| 2026-11-23 | the rooms keep their places: the districts already stand where the model puts them, and moving them would rebuild every proved room for little gain in play. The halls are clad as houses and the square arcaded instead (5.1). A round square and the prologue on the wall walk are left for a later change, if play-tests ask for them |
| 2026-11-23 | water is drawn as water (5.9): seen through, with waves, the sky and the sun in it, running and falling water, foam, falls between the locks, a wet band when it goes down, a splash and a moment under the surface when a body falls in |

## 14. The vertical city

Decided on 2026-11-22. This section gives the rules the map follows; the
design of sections 1 to 13 keeps them.

### 14.1 Height

Each district leaves the square in one direction and from its own height. A way is first seen from above, then walked.
Districts meet by lanes, stairs and arches, not by corridors and doors.

### 14.2 The aqueduct

The water of the city comes in from the east on an aqueduct that spouts into the pool of the square. In B the storm has
broken it: a part of it has fallen, so every water of the city is dry in B.

### 14.3 Worlds by material

| Exists in | Looks like | Examples |
|---|---|---|
| A only | made by people, whole: wood, iron | the gate, planks, door panels, crates, the deck of the sky bridge |
| B only | left by the storm: fallen masonry | the beams over the gaps, the heap of beat 3, the walls of the courtyard puzzle |
| both | stone of the city | walls, floors |
| key objects | their material; in the other world a pale outline, a tint of red (A) or blue (B), that grows bright after 30 s in a room (9.3) | every single-layer box |

Whole red or blue boxes are gone, and so are the red and blue edges of the boxes in their own world: the material says the
world (wood and iron in A, fallen masonry in B); a pale outline in the other world is the only hint that is always there.

### 14.4 Logic of the moving parts

| Rule | How |
|---|---|
| All water comes from the aqueduct | each water is fed by a chain that starts at it, and each feed runs downhill (`Stage::kWaterFeeds`) |
| Things that move exist in both worlds | only where they are differs: the crate of the canal floats in A and lies on the bed in B |
| Machinery works in A only | in B it has rusted: the lock wheel turns only for a body in A |
| R puts back the room | the block, the orbs, and the water, the weir and the crate of the canal |
| A switch is refused into anything solid of the other world, and into its water | the guard asks the whole solid table of the target layer: the map, the gates of the wings, the lifts, the crate (`Puzzle::FitsIn`), and whether the body would be under water there (`Recovery::SwitchAllowed`) |
| Orbs never get stuck | an orb that comes to rest where no body of its world can take it back goes back to its start with a ring of light (`scene/OrbReturn.h`); one that falls out of the map does too |
| A thing changes world, not the body | the prism moves the gates linked to it into the other layer, for good (14.9) |

### 14.5 Anchor lamps

A stone lamp that looks the same in both worlds. It is lit the first time a body passes it; from then on a fall or R in its
room comes back beside it, and a ring spreads from where the body appears. Rooms without a lamp have a carved tile at their
entry. Eleven lamps (the tower lock has none: R and a fall come back to its bed, or to the lamp of the locks when that is under water): the quay, the locks, the market, the theatre, the graveyard, the terrace, before the gap, the square, the archive, the canal, the bell tower;

### 14.6 The prologue

Section 6.1.

### 14.7 Checks

| Rule | Checked by |
|---|---|
| A room has its own fall line, close under its floors, so a deep room is never taken for a fall | `TestFallLines` |
| Every lamp stands in its room, is a place to stand in both worlds, and is off the board of the courtyard | `TestAnchors` |
| Every water is fed downhill from the aqueduct; the aqueduct is broken in B by more than a running jump; no water in B | `TestWater` |
| The canal is crossed only on the raised crate; the low crate, the bed and the floor do not reach the quay | `TestEastCanal` |
| Neither world alone crosses the ditch; B does not climb round the heap | `TestPrologue` |
| The breach of the undercroft is reached only from the drifted crate of B; the bars hold until the lever | `TestUndercroft` |
| Without the prism neither world crosses the gap of N1, also with a switch in the air | `TestPrism` |
| An orb is sent back only where its world cannot take it | `TestOrbReturn` |
| The cellar: B walks it from the canal bed to the grate, which holds until the lever; A is flooded and sealed | `TestCellar` |
| N2: no way onto the heap before the prism, also with a switch in the air; after it B reaches the light | `TestN2` |
| The end drains every water of A, the canal and its crate | `TestEnding` |
| No jump ends higher in a room not joined by walking, with every memory fragment present | `tests/test_reach.cpp` |
| Every place of return is a place to stand out of the water, in both worlds, in every state of the waters and the canal; a switch into water is refused | `TestRecovery` |
| The bells change what they say and close nothing; the fragments are in B only and are where they say | `TestCityState` |
| The ground is under every floor; no walk or jump off an edge ends on it; the land and the sea fill every view down; the houses stand on the levelled town; the plants follow 5.6 | `tests/test_terrain.cpp` |
| Drawn-only things stand where no body is (houses, merlons, the aqueduct) | `TestSilhouette`, `Stage::PlotIsFree` |

### 14.8 The canal and the undercroft

The canal: 4.8 m wide; the lock wheel raises the water by 1.2 m in 2.4 s; low, the crate is 2.4 m under the quay, high,
1.2 m (the step of the main path); in B the crate lies on the bed and the bed is walked from stairs at its east end.

E2 and the undercroft: the weir, a second wheel on the west end of the quay (A only, like the lock), lets the current through,
and the crate drifts west to the end of the canal. In the west wall of the canal, 2.5 m over the bed, a breach of B opens (in A
an iron panel); from the bed it is out of reach, from the crate of B (which lies under the crate of A) it is a step. Behind it
the undercroft runs west under the gallery to the east rim of the square, where four bars close it; the lever inside opens
them for good. Parapets on the west end of the hall floor and of the quay keep every other way out of the breach.

### 14.9 The prism (N1)

Landing 2 of the north hall is cut by a gap of 5 m between the top of ramp 2 and the seal. A bridge of A spans it, and at its
far end a portcullis of A, 2.6 m high, closes it; in B there is no bridge. The prism on the near part (a crystal, the same in
both layers) moves the portcullis into B when the body stands on it, in either layer, and it stays there: then a body in A
walks over. The portcullis stands at the far end, so a jump in B with a switch to A in the air lands at best in front of it,
and it reaches 2 m past the landing on both sides, so no jump goes round it.

### 14.10 Still open

| Item | State |
|---|---|
| The story (1.1) and the pairs of props that tell it (9.2) | written last |
| The guide figure of 9.3 | the stuck timer brightens and pulses the outlines instead |
| Music, a sound for each world | no audio in this project yet |
| The rendering steps (shadows, the water, the sky) | after the play-tests |

### 14.11 The cellar

The fourth link of the ring of districts, under the courtyard: from the canal bed west, then down a ramp to a grate
in the east wall of the lower room of the west wing. Flooded in A (an iron sluice panel at each end), dry in B; the grate
opens by a lever inside the lower room. A body in B inside it cannot switch to A (it would be under water); one that falls or
presses R comes back at the entry of the cellar in B and at the entry of the east hall in A (`Recovery::SafeRespawn`). It is
the link of the ring of districts under the courtyard.

### 14.12 N2: one prism moves two things

A heap of fallen masonry (B) along the west wall of the north hall, 2.4 m high, closed by an iron cage of B; a wooden step of
A at its east edge. The prism on the floor moves the cage into A and the step into B, at once and for good: then B climbs the
step and the heap to a hidden light in the wall.

### 14.13 The end

Replaced by the last moment (1.5): the master sluice of the balcony drains every water of A in the endings that let time move
on.
