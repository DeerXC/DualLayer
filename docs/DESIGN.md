# Design

The next version of DualLayer. Unless identified as a current-build baseline, this document describes the target design, not shipped features. The uploaded baseline is documented in [Architecture](ARCHITECTURE.md) and [Build](BUILD.md).

## 1. The game

A third-person 3D exploration puzzle without combat. One walled harbour town on a hill, at two moments:

- **A, the past**: a warm, calm evening, the town whole and its people about their business, the night before the surge;
- **B, the present**: the same town about twelve years later, in ruins under a cold moon after the surge, with a few people living on the heights.

The body carries a lantern. **Tab** moves the whole world between the two moments. The lantern, **set down in the past**,
keeps a circle of the past standing in the present. What is done in the past leaves its trace in the present. The way
through the town is found by reading the present and changing the past.

The game asks one thing, and the player sees it from the first frame: three dark lamps high over the bell tower, on a signal mast.
After the harbour the town opens into three districts. In each, in any order, a keyholder of the harbour is helped to see a sign that he can check himself, and gives his key. With the three keys the tower opens; at the top the lamps are lit and the warning bell is rung. The story is in section 1.1.

### 1.1 The story

The story is told by the town and by short lines of the people in it; the main route never needs it explained. It exists so that every obstacle, person and object has an ordinary reason (principle 11, section 2).

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

## 2. Principles

Every change is judged against these. A change that breaks one is not made.

1. **One rule.** Everything comes from the two moments: the switch, the lantern and the causal rule are three sides of it.
2. **Hard in the head, not in the hands.** No precise timing on the main path (limits in section 11).
3. **Few actions per room.** A room asks for one or two placements of the lantern and one or two switches, at places the
   scene points to; the rest of the room is walked in one moment.
4. **Seen before reached.** Each place is seen first, then reached from an unexpected direction. No text guides the way.
5. **A goal in sight.** From the first frame the player can see what the game asks (the three dark lamps of the signal mast over the bell tower),
   and each district opens on something seen and not yet reached. A player who never presses a help key still knows what to do
   next (section 10).
6. **Free order.** After the teaching ground the three districts are done in any order. No district needs another, none
   teaches what another uses, and all three reach the same ceiling of difficulty (section 7.4).
7. **No forced backtracking.** No puzzle sends the player to another district. A local connection may return to known ground only when it offers an immediate route or useful new perspective before the finale; no shortcut sends the player back to the start after the clear. The one walk back that the design accepts, from a way to the lower town, is in section 7.4.
8. **Nothing that looks like a bug.** The body is never pushed out of a wall; a switch or a pick-up into something solid is
   refused instead.
9. **Two states of one town.** Light and colour distinguish the moments at a glance; damage, materials and signs of use keep the difference readable without colour (section 6).
10. **Explained in person.** The core is hand-written C++ and small enough to be explained line by line in an interview.
11. **Ordinary reasons.** The lantern is the one thing that cannot happen. Every obstacle, person and object has a reason that an
    ordinary person would accept (section 1.1); nothing is shut, broken or stuck only because a puzzle needs it.

## 3. Mechanics

### 3.1 The switch

| Rule | State |
|---|---|
| Tab moves the whole world between A and B; both moments are complete, walked and seen whole | built |
| A switch into anything solid, or into water, is refused: the ripple opens and turns back | built |
| A jump may start in one moment and land in the other (the switch in the air) | built |
| Holding Tab on the ground shows the other moment in a circle in the middle of the view, without moving there (look before going; section 3.8) | planned, core |

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
- The lantern is also a real light: it lights what is near it, in either moment, with the colour of its flame (6.3).

Example, the broken bridge: in A walk onto the whole bridge and set the lantern down in its middle. Walk back to the
bridgehead and switch to B: the bridge has fallen, but its middle hangs in the circle. In B jump from the bridgehead onto
it and on to the far side, where the gate is open in ruin (in A it is locked). One switch; the thought is "the bridge of
the past, the gate of the present".

As built (`scene/Lantern.h`, `scene/Moments.h`): the circle is a sphere of radius 3.5 m round the lamp, 0.45 m over the floor
it is set on, 0.9 m ahead of the feet (at the feet at an edge); taken up within 1.8 m. Its edge in B is a warm rim of
light. The new route assumes one lantern with no collectible-based radius upgrades.

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
- **A person is a thing of A.** What is given to a person, asked of him or done for him changes his stance in A (he steps aside, hands something over, opens a door). In B the same person, about twelve years older, stands at the same place and the change shows there (an open door, a kept object, a lit lamp). Nothing said or done in B reaches A.
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

### 3.7 Player and camera

The town prototype uses a third-person follow camera. The lantern follows the right hand while carried; when placed, it follows the rules in section 3.2. Character and lantern art are specified in section 6.7.

- **Movement:** hold Shift to sprint at 1.6 times walking speed by default, without stamina. Ground animation follows actual movement after collision. Sprinting lengthens the stride and leans the body forward. The main route should not require sprinting for every obstacle.
- **Camera:** the follow distance starts at 3.8 m. The camera pulls in when blocked and extends gradually when clear. During a switch ripple, geometry in either moment can shorten the distance. The character is hidden if the camera gets too close.
- **Climbing:** a forward jump with Space held can climb a nearby ledge up to 2.2 m above take-off if the top has support, the landing is dry and the body fits along the sampled path. Walking into a wall does not trigger a climb. A dedicated pull-up animation is not included in this pass.
- **Acceptance:** verify the quay, sea gate and first aerial switch with this controller. Check that sprinting or climbing cannot bypass the intended gates. The baseline route solver does not model the new moves.

### 3.8 The lens

Holding Tab looks into the other moment without going there. It turns trial and error into reasoning: the player may see what
the other moment holds, but still has to say what to do in A.

- **Hold:** on the ground, Tab held for 0.2 s opens a circle in the middle of the view (about a fifth of the screen height in
  radius) in which the other moment is drawn. Its rim is warm when it shows A and cool when it shows B. Releasing closes it.
  The body, the camera and the solid stay in the moment they were in.
- **Tap:** on the ground, Tab released within 0.2 s switches, at the release. In the air Tab switches at once, on the press, as
  now, and the lens is not available: the switch in the air is an act, not a look. If the switch on the ground feels late in
  play, the lens moves to the right mouse button and Tab switches on the press again.
- **What it shows:** the other moment as it is now, with every trace and the circle of the lantern; never what has not happened.
  It shows a place, not an answer.
- **When it closes by itself:** when the body leaves the ground, sinks, falls or is returned.
- **Made from:** the two pictures the ripple already draws (`Models::DrawRipple`), with a held circle instead of an opening
  one; no new pass. While it is open both moments are drawn: check the cost on the laptop.

## 4. Progress and return

- **The three keys and the tower:** each district has one keyholder of the harbour, with his post at the end of its way (section 1.1). His key is the reward of the way's last beat. The door of the tower has three locks, one for each key, marked with the same three emblems; it opens when all three are turned (section 5.1). The signal mast stays dark until the top of the tower. Progress is shown by the town and not by a counter: a keyholder who has given his key stands ready at his post in A, his post lamp burns in B and his district is lived in (section 6.4). The three emblems are the only progress mark and carry no words (section 10.5). The climb is reachable without lighting any optional street lamp.
- **The links to the rock:** each district ends in a link to the plaza of the rock, opened by that district's last puzzle (section 7.4). A link is a connection, never a requirement: every district is complete from the lower town alone. The player takes the link of the district finished last, so the finale does not start with a long walk back.
- **Anchor lamps:** checkpoints, not collectibles and not optional street lamps. Place them after meaningful progress. A fall or R returns to a safe floor in both moments without depending on the placed lantern. In the uploaded town baseline, this is the last safe standing point; anchor-based recovery and lamp travel belonged to the earlier greybox prototype (git tag `m2-greybox`) and must be rebuilt if wanted.
- **Optional street lamps:** light a lamp in A and it remains lit in B. A lamp can clarify a nearby route or reveal a landmark. Reward exploration with changes visible in the town and with the boat outcome in section 5, not a collection counter. Memory fragments, fragment-based radius upgrades and unlockable story scenes are not part of the new route.

## 5. Bell tower finale and ending views

### 5.1 Final approach (layout to prototype)

The three links meet on the plaza of the rock; the player arrives by the link of the district finished last, and the door of the tower opens when its three locks are turned with the three keys (in A, which leaves it open in B). From the plaza, frame the bell tower, a recognisable entrance and the first walkable segment together. Keep the approach local. Prototype a broken approach in B that reveals the route, a safe position to switch to A, and a short climb through the intact tower. Use the established rules for any lantern-held floor or ledge; a new timing mechanic, long backtrack or precise aerial switch is not required. The bell platform must be reachable through an intended path, not by standing on a wall. Leave enough room to read the bell, the interaction and the harbour below with the third-person camera. The layout and whether the final climb requires a lantern placement remain play-test decisions.

### 5.2 The warning and its consequence

The surge of section 1.1 floods the harbour and the lower town. At the top of the tower the lamps of the mast are lit from the lantern and the warning bell is rung in A. Everyone in the town knows what the bell and the three lamps mean, so people go up and crews cast off before the sea comes. The signal does not stop the surge, control time or serve as a navigation signal. The lamps stay lit in B. Whether the top asks for one action or two is prototyped with the approach (5.1). Establish several recognisable wrecks in B at the opening. Distinguish them by sail colour and, where useful, silhouette or a matching fabric pattern. In A, show the matching boats leaving their exposed berths after the bell. In B, the corresponding wrecks are absent, but the flood damage to the town remains. Show an absence in the same framing used at the opening so the cause and result can be read without dialogue. No playable boat sequence is required.

Optional lamps increase the number of boats shown escaping without unlocking the tower, the bell or access to an ending view. The first prototype should show a visible baseline payoff with zero lamps and a larger payoff after exploration. The exact count, which lamp affects which boat and how the lamps relate to departure routes remain open; do not treat a one-to-one mapping as settled. Use the same recorded boat outcomes for all three views of a run.

### 5.3 Past, Present and Both

| View | What is shown |
| --- | --- |
| **Past** | The warning in A and the identified boats pulling away from the quay before the flood. |
| **Present** | The damaged harbour in B. Locations previously occupied by the matching wrecks are now clear. |
| **Both** | Match the quay framing across A and B so a departing boat and its empty wreck site read as one cause and one result. The town is not restored to A: B shows the damage of the surge and the signs of those who came back. |

The exact input and presentation for these three planned views must be tested. The legacy third-stroke selection, held-Tab condition and R replay are not requirements for the new town finale. Do not gate Both on collectibles, a hidden room or a timed input. The payoff is understandable without finding any optional cloth clues.

## 6. Art direction

### 6.1 Style and scope

**Simple forms, rich atmosphere.** DualLayer is a stylized medieval harbour diorama, not a voxel world or a photorealistic reconstruction. Keep the existing irregular terrain, terraces, walls, rooflines and bell tower. Use low- to mid-poly geometry with clear silhouettes. Add geometry where it changes the outline, explains function or helps navigation: coping stones, recessed doors, quayside supports and arch profiles. Keep passing walls quiet; reserve close-up detail for landmarks and interactions. Build depth and weathering through overlapping silhouettes, restrained surface detail and readable age differences between A and B; detail must support navigation rather than asset density.

### 6.2 Materials and surface detail

| Material | Visual rules |
|---|---|
| Stone | Broad, uneven courses; restrained joints and rough highlights. Use darker wall bases, coping stones and corner blocks before adding small cracks. Stairs and bridges should read as stone at normal camera distance. |
| Wood | Grain follows the plank or beam. Place joints and end grain where the construction calls for them. Dry timber in A becomes grey and worn in B; do not redraw every plank. |
| Plaster | Subtle broad variation, darker under eaves and near the ground, with exposed masonry only where there is a reason for damage. |
| Roof tile | Use three or four closely related terracotta variants. In B retain the same roof identity where it survives, weather the tiles and remove them selectively. |
| Metal | Reserve brass, iron and verdigris for functional details. Keep bright warm accents concentrated near useful interactions, not on every prop. |

Use shared 512 or 1024 px textures as a starting budget. Start with base colour and roughness; add restrained normal maps and baked contact shading where they improve the read. Do not model every stone or add fine noise across every surface. Keep texture scale consistent between assets.

### 6.3 Light, sky and distance

One sun or moon with soft sky and ground fill, the lamps of the town, and what is added of both to the picture. The numbers are in `[lighting]` of the config ([BUILD.md](BUILD.md)); this is the rule and the reason for each.

- **The sun of A and the moon of B cast shadows.** One shadow map of 2048 x 2048 texels covers 60 m round a point a little ahead of the camera; the box follows the camera on the grid of the texels, so a shadow never crawls. The town, the traveller, the crates and the things that change cast a shadow; water, glow and halos do not. A shadow takes 85% of the direct light and none of the sky light, so a shadow is never black and the detail in it stays. A lamp casts no shadow: its light is a soft pool, not a cut-out.
- **Every lit lantern of the model is a light.** The lamps of the streets, the plaza, the harbour and the lighthouse, the ring of lights on the tower and the lantern on the grave light the surfaces round them in the moments in which they are lit: a warm pool 14 m across for a street lamp. In A the sun is still up, so the lamps are weaker (0.55); in B they are the only light but the moon, and the lit ones along the main path read as a trail of pools (the model keeps every second lamp of the path lit). The renderer lights a surface with the 32 nearest and strongest lamps of the moment, and a lamp fades out with distance, so that none blinks on or off as the camera walks. A flame is never still: a lamp flickers by 3 to 10% (the lantern of the traveller by 12%), with a phase of its own.
- **What the player did is a light too.** The lantern of the traveller, in the hand or set down, lights its surroundings (13 m) in both moments with the colour of its flame. The lamp of a post whose keyholder has given his key burns in B and lights its street. The lamps of the mast, lit at the top of the tower, light the tower under them. The world answers with light.
- **No lamp is needed to read the route.** The route, the next landing and every interaction must be readable in the moonlight alone (`lamps_b = 0` tests it). A lamp helps the eye and never carries the game; gold still means use, and the lantern's own colour is its flame.
- **No colour is cut off.** The sum of all the light goes through a soft shoulder (a colour brighter than 0.78 is bent toward white and keeps its hue), so that a lamp lights a wall and does not turn it into a white patch. A soft glow round each lit lamp (a halo: 35% in A, 100% in B) puts the light in the air as well as on the walls, and the fog glows toward the sun.
- **The light has a style as well as a physics.** The town is hand-painted, not photographic, and nine parts say so; each is a number in `[lighting]`, all of them are one master (`style`), and the last view of the key L, "plain", shows the same light without any of them. (1) The sun falls on a face in three soft bands, so that the faces of a low-poly town read as separate planes. (2) The sky light of a face that the sun does not reach leans to a cool colour, blue-violet in A and indigo in B: warm light meets cool shadow. (3) Clouds drift soft-edged shadows over the town (38% of the sun in A, 28% in B), cast along the direction of the light, so that a wall and the street below are under the same cloud. (4) An edge that faces the light has a rim, which separates the traveller from the wall behind; the stone has a third of the rim of the things that keep their own colour, and a floor none. (5) A lamp lights its pool in four soft rings, with a whiter core and an amber edge. (6) The stone is painted: soft blotches of value and hue, tall on a wall like the stains where rain runs, and the foot of the town near the water darker and cooler (twice as much in B). (7) Rays of light stand in the air: each ray of sight is sampled 14 times through the shadow map, so that the light that comes through a gap between two buildings is seen as a shaft; the air is thicker near the sea, the rays are strongest toward the sun, and they reach 48 m. (8) The far haze leans to lavender in A and to teal in B, so that the distance has its own hue. (9) The picture is graded: the darks lean one way and the lights the other (A: blue-violet and gold; B: indigo and pale cyan), the colour is pushed a little, the contrast is raised round the middle grey, and the corners are a little dark. The style never carries a rule of the game: a landing, an edge and an interaction must read in the plain view as well as in the styled one (E-25), and the same low-poly forms are what the player sees in both.
- Replace the solid sky with a procedural gradient. A moves from muted blue-grey overhead to pale gold at the horizon, with a low western sun. B moves from deep grey-blue overhead to a misty cyan-grey horizon, with cool moonlight.
- Use restrained distance and height fog to separate the quay, sea wall, streets, bell tower and hills. Frame a readable route in the middle distance and keep the main landmark visible. Fog and shadows must not conceal the next landing or an important interaction.
- Not built: a shadow of a lamp or of the lantern (a cube map or a spot), cascades for the far town, contact shading at wall bases, door recesses, steps and under eaves (baked or at the material), screen-space ambient occlusion, a real bloom (it needs a picture in high range), and shafts beyond the 48 m of the march or from the lamps. Take one only if a play-test shows that the light is flat, and check it on both target machines.

### 6.4 The same town at two moments

| | A: the past | B: the present |
|---|---|---|
| Light and air | Long amber sunlight, cool soft shadows, light golden haze | Cool fill and moonlight, deeper blue-grey shadows, low cold mist |
| Built surfaces | Dry stone, intact doors and roofs, timber and terracotta | Damp wall bases, greyer wood and rusted fittings; selective roof damage and debris |
| Life and vegetation | Useful objects in place, crates stacked, plants tended around houses and water | Misplaced or missing objects, moss in shade, growth through cracks and rubble, changed water levels |

Changes should follow drainage, exposure, damage and disuse, not a uniform filter. Preserve enough shared geometry to recognise the same place immediately. In a greyscale A/B comparison, the player should still distinguish the moment through silhouette, value, damage or signs of use.

**B after a way is solved (lived in).** A district whose keyholder has given his key takes a third state in B: not A again and not the ruin. People came back, because the goods and boats of the district were saved, and the surge is still readable. This layer is decoration. It never changes what is solid, so the route proofs of section 11 hold. The lower town and the harbour take no such state; they change at the ending through the boats (5.2).

| | A: the evening | B: the ruin | B: lived in |
|---|---|---|---|
| People | at doors and posts | a few, on the heights | the same people, about twelve years older, and more of them |
| Marks of the sea | none | a tide line and silt on the walls | the line stays on the walls; the street is swept |
| Materials | whole stone, plaster and tile | fallen, mossed, rusted | old stone with new timber and patches of new lime wash in another tone |
| Light | low amber sun, then lamps | moon only | the moon, with warm windows and a post lamp |
| Goods and boats | stacked, moored | wrecked, stranded | goods stacked high, boats drawn up, simple stalls |
| Plants | tended gardens | scrub, figs, grass in the paving | vegetable plots and young trees among the older growth |

Lived-in B must be told from A at a glance, in greyscale too, by silhouette, value or signs of repair.

### 6.5 District identity and vegetation

- **Harbour:** hoist, warehouse doors, mooring posts, ropes and boats. Put the strongest visual cue on the way from the quay to the sea gate.
- **Housing:** narrow doors, recessed windows, chimneys and occasional balconies or washing lines. **Market:** cloth awnings, timber stalls and a few carts. Keep crossings clear of decorative clutter.
- **Bell tower:** regular masonry, a clear arch and visible steps distinguish the approach from residential streets. Keep the entrance legible, not just the tower spire.
- Replace cone placeholders near the player with trunks, a few branches and grouped low-poly foliage. Use sparse grass clumps and a few greens. Place growth by moisture, soil and shelter: more near water and ruins, moss on damp shaded bases, and little in the middle of an intact route.

Props and planting should identify where the player is, point towards the next route or show how the place changed. Do not add a high-contrast door or platform that looks usable if it leads nowhere.

### 6.6 Water

Keep one stylized water language across the sea, harbour, pools, channels and waterfalls. Start with scrolling surface normals, a stronger reflection at shallow viewing angles, colour by depth and restrained shore foam. Use warmer reflections and clearer green-blue shallows in A; use deeper blue-green, less transparency and cooler reflections in B. Match ripples and foam to flow, impact or rising and falling water. Make the shoreline and hazardous depth readable before adding richer reflections. Sound and gameplay rules are not changed by this visual pass.

### 6.7 Character and lantern

The traveller is a stylised figure of rounded forms. It keeps the teal coat, the round hood and the small pack of the starting silhouette and stands 1.72 m from the feet to the top of the hood (visual stature only: eye height, body clearance and mantle reach are unchanged). The proportions are those of a picture book: a large hood with a peak, big boots, a coat that hangs from the waist like a bell and a short pack with its bedroll. Round joints (shoulder, knee) separate the parts of a limb, so that a pose reads from the camera without an outline. The scarf is the one second colour, a muted rust; a collar covers the chin and two tails stream behind at a run. The belt, pouch, straps and buttons are leather and brass. Cloth is matte, leather slightly smoother, brass a controlled highlight. The same silhouette and materials carry across both moments; the rim of section 6.3 separates the figure from sunlit stone and cold ruins.

The figure is posed, not rigged. Each leg and arm has two bones (a hip and a knee, a shoulder and an elbow). The body comes down as the legs spread, so that the foot that stands is on the floor; the leg that swings forward bends its knee; the upper body leans about the waist when he runs; the left arm swings against the left leg. The right hand holds the lantern and the arm raises it for H. Visual geometry is separate from the collision body.

The lantern is the character's strongest warm accent, so nothing else on the figure is amber or gold. It is one model in both states: a bail ring, a chimney, a brass cap, four iron posts, a glass globe that glows, a flame inside it that wavers, a rim and a foot. In the hand it hangs from the ring a little outside the right hand and sways with the stride. Set down, it hangs from the arm of a small iron post (a foot plate, a pole 0.16 m to one side, an arm and a finial), with the glass at the centre of the circle, 0.45 m over the floor, so that the thing the player sets down is plainly the thing the player carried. The flame of the guidance (section 10) stands over the chimney as a spindle.

The light of the lantern is an amber a little deeper than the white-gold of a lit lamp, flickers by 12% (a slow breath, a quicker waver and a fine tremble, never in time with a street lamp) and starts at the glass: it follows the sway. The glow round the glass is 1.0 m across, smaller than a street lamp's, so that it does not cut into the walls of a narrow street.

### 6.8 First playable art pass

1. Work on the existing **quay to sea gate** route, including a tight passage, a water edge and the view to the bell tower. Start with sky, fog, directional shadows and ambient fill; then apply the core materials and water. Add district props, plants and character refinements only where needed.
2. Capture paired A/B views from the same third-person camera positions. Check that stone, timber, plaster and roof tiles read at play distance; the route and water edge are clear; the tower holds the skyline; and the character and lantern stand out in both moments. Repeat in greyscale and verify performance on the laptop and desktop before extending the pass to the rest of town.

The first rendering pass implements the gradient sky, ground fill and distance and low mist, and since 2026-11-26 the shadows, the lamps, the soft shoulder and the style of the light (6.3). Surface materials, water refinements and the paired Windows play-test remain open. Do not rebuild the city or add post-processing systems before the reference route proves the visual direction.

### 6.9 One style for the whole town

Pass 5 (2026-11-26). Playing pass 4 showed that the buildings had improved and the town had not: it still read as one orange. Three causes, all measured. (1) The palette: `stone`, `paving`, `sand`, `plaster0` and `plaster3` were five creams within 0.05 of each other in every channel, so a wall and the street under it had the same colour and value; the town had one cool colour (`shutter`) and B was a second table written by hand. (2) The engine: `shaders/lighting.hlsli` has a hand-painted stone (`PaintAlbedo`: soft blotches in value and hue, tall rain stains on walls, the damp of the foot of the town near water, a third of the rim), and `scene/Town.h` baked every polygon of the town with alpha 0, so the painting was never applied to the town. (3) The shapes: the houses and the landmarks were slabs (7.10).

**References, and what each gave.**

| Reference | What it says | What the town takes from it |
|---|---|---|
| Cinque Terre and the Ligurian coast | Colours were made of local earth pigments (ochres, beiges, mineral shades) in lime wash on lime plaster; colour told one house from the next in a dense row; the broad bright palette of today developed from the 19th century on | eight lime washes, each of them a pigment of the coast, one for each house and never the neighbour's |
| Dubrovnik | One limestone for walls, stairs and streets, weathered to honey; a tile of one family, whose ages make a patchwork of colour; plain houses, with the important buildings made important by composition and not by size; a building code since 1272 | one stone, one tile in four ages, houses that are plain and have a rhythm, landmarks that are made by silhouette |
| Koriko of Kiki's Delivery Service | A collage of Visby and Stockholm's Gamla Stan (ochre fronts, red roofs, stone lanes, a wall, a clock tower), "how someone imagines an old European town", with Naples and Lisbon mixed in | permission to combine; the clock tower and the red-roof silhouette are what the town is known by |
| Environment colour practice | Value structure before colour; a dominant family, a secondary family and a small accent; test in greyscale and at thumbnail size | rules 5 and 6 below and the audit that checks them |
| Stylised art in general | Strong shape language, a deliberate palette and clear silhouettes carry the style, not surface detail | the three tiers of detail below |

(The pigments and the building code are from the cited histories of Cinque Terre and Dubrovnik; what is said of games and of practice is the common practice of environment art, taken as a method and not as a source of numbers.)

**The rules** (each one is checked by `python tools/layout/town_style.py` for the palette and by `check()` of the model for the built town):

1. **One stone.** `stone`, `stone_dark`, `rock`, `paving`, `sand` and `debris` are one family in hue (69 to 88 degrees), differing in value: lit stone, shaded masonry (8 L* darker), streets and squares (darker than the walls).
2. **Lime wash in earth pigments.** `plaster0` to `plaster7`: cream, yellow ochre, terracotta rose, ivory, pale yellow, salmon, faded sage, dusty blue-grey. Any two differ by Delta E 9 or more. A house has one colour and never that of its neighbour; a hub has all of them, a terrace most of them, the rim cream, ivory, ochre and sage.
3. **One tile, four ages.** `roof`, `roof2`, `roof3`, `roof4`: terracotta, new pale orange, old dark red-brown, faded dusty. Any two differ by 9 or more. The hub has the new, the rim the old.
4. **Accents on purpose.** Shutters (sage, sea blue, oxblood: one colour for a house), awnings (red, blue, mustard), flowers (`bloom`, magenta, on balconies and pergolas only), verdigris copper for civic domes only. Gold stays the one colour that means use; nothing else is as yellow, as saturated and as light.
5. **Value before colour.** In greyscale the roof is darker than the wall (L* 18 or more in A, 8 in B), the street darker than the lightest wall, shaded masonry darker than lit stone. Measured on the built town: walls L* 80, roofs L* 47, ground L* 73 in A; 61, 29 and 56 in B.
6. **A dominant family, a secondary, a small accent.** The warm family (limestone, lime wash, clay) is most of what is built (65 to 92%), the cool family (leaf, sage, verdigris) 5 to 30%, the accents 0.3 to 10%. Measured by the area of the solid surfaces and decals of the town (not the sea, the cliffs or the open ground): 91%, 9% and 0.7%. The cool family and the accents are at the low end: the next lever is more shutters, awnings and plants, not a new colour.
7. **B is derived, not chosen.** B = tint of the class x (value scale x grey of A) + hue memory x (A - grey of A). The classes (stone, plaster, tile, paint, wood, iron, green, water, gold) have their numbers in `CLASS_B`; the lights (a window dark, a lantern warm, the cold memory light) are set by hand. The rule was fitted to the B that was written by hand before, so that the present keeps its look: mean Delta E 6.9 (median 6.3) over the 22 materials that existed; the largest change is the ochre lime wash (18.6), because its A is now a purer ochre. Hue memory keeps the same houses apart in B (any two lime washes still differ by Delta E 4 or more) and keeps the same place recognisable.
8. **Three tiers of detail.** Tier 1, seen from 50 m or more: the silhouette (the roof, the tower, the dome, the cupola). Tier 2, seen at 10 to 30 m: the rhythm of a facade (windows, shutters, balconies, string courses, a bay). Tier 3, seen under 5 m: the sill, the lantern, the step, the sign. Every building has tiers 1 and 2; tier 3 is spent on the landmarks, on the doors that are entered and on the route.
9. **The town is painted.** Stone, plaster, tile and ground are painted by hand by the style of the light: `TownData::Painted` (scene/Town.h) gives their polygons alpha 1, the shader's `PaintAlbedo` makes the blotches and the tall stains (`wash`, default 0.9) and the damp near the water (`damp`, 0.5), and the stone has a third of the rim. The town's tint is white and its weathering is 0 (`app/Gpu.cpp`), so nothing of the block pattern of the stone path applies: plaster is painted, not coursed. Wood, cloth, paint, metal, light, water and the crowns of trees keep their own colour. Tune with `wash` and `damp` of `[lighting]` (config/default.ini) and the key L; `wash = 0` is flat colour.

**Pass 6: B is weathered stone in the moon, not ice.** The first play-test of 6.9 showed B as a blue town: the stone, the plaster and the tile were tinted toward blue (`CLASS_B`), the moon was blue (`LIGHT["B"]` light (0.86, 0.96, 1.15)), the sky light was blue (0.24, 0.30, 0.45), the style of the light pushed saturation to 1.25 and contrast to 1.22 and made the haze teal (`StyleLookOf(1)`), and the ground bounced a blue: five layers of the same cold. The town is the same town after a storm and some years, in the light of the moon, so the cold belongs to the light and the warmth to the stone. Now: (1) a material of B keeps most of the hue of its A and loses value and chroma, with a tint that is only a little cool (`CLASS_B_AGED`); `B_AGE` in `tools/layout/town_style.py` mixes it with the old rule (`CLASS_B_COLD`): 0 is the ice of passes 1 to 5, 1 is fully aged, 0.65 is set (mean b* of the main materials of B: -4.9 cold, +8.7 now, +27.7 in A); (2) the moon is silver (0.92, 0.98, 1.10), the sky light a lavender-grey (0.30, 0.33, 0.43), the sky top (0.16, 0.19, 0.29), the haze a pale grey-blue (0.45, 0.49, 0.56) and thinner (fog and mist 0.0036 from 0.0040 and 0.0042); (3) the shadows of B still lean blue-violet (a moonlit night has them), but the lights are silver (0.99, 1.00, 1.04), the haze tint (0.92, 0.99, 1.06), saturation 1.06, contrast 1.14 and the damp scale 0.8; (4) the ground bounces a grey (0.21, 0.22, 0.24). The lamps stay amber: they are the one warm thing of B. `tests/test_style.cpp` still holds (the shadows lean blue in both, the lights of B are pale cyan, the means of the looks near 1).

**Where it lives.** `tools/layout/town_style.py`: the palette (43 materials, each with a family and a role), the rule of B, `audit_palette()` and `audit_town()`. `tools/layout/city_model.py`: `MAT = town_style.build_mat()`; `check()` prints the style of the built town and fails when a rule is broken. `tools/layout/architecture.py`: the houses and the landmarks choose their colours from the weights of their district (`WALL_WEIGHTS`, `ROOF_WEIGHTS`). `src/scene/Town.h` and `tests/test_town.cpp`: `Painted` and its test.

**Districts.**

| District | Lime wash | Tile | Houses |
|---|---|---|---|
| Hub: the lower town, the fish market, the market | all eight, colourful, a different one from the neighbour | new and pale, some old | shops with awnings, rows, altane, a porticoed house on a square |
| Terrace: the west and east lanes, the upper street, the tanner loop | cream, ochre, terracotta, ivory, yellow, salmon, sage | terracotta, old, faded | rows, altane, cottages |
| Rim: the wall lane | ivory, cream, ochre, sage | old, faded | cottages and rows |

**Built (2026-11-26, pass 5).** The palette and the rule of B; the painted path of the engine; the library of 7.10 in the town: 45 houses of the library and 6 rooms (the old slabs are gone), the market hall (a shop front under each arch of its arcade), the clock tower (the same face to the street), the water tower and the chapel of the vine lane, which can be entered (the way in is walked by `tests/test_town.cpp` like the way into a room); the corridors dressed in bays (a lime wash, a tile, a shutter and a door of their own; windows as decals; nothing to stand on). **Not built:** the town wall and its towers, the theatre, the harbour warehouses and the lighthouse, the bell tower and the plaza of the rock, the aqueduct and the boxes of the puzzle pieces keep their old shapes (in the new colours). The corridors and the pieces are still walls: 8.5 m round each puzzle piece is reserved for it (`Piece.clear()`), which is why the library houses are 45 and not more, and the proposal of 7.10 (an open town with shut thresholds) waits for a decision. The market hall, the clock tower and the water tower cannot be entered (a shop front, a blind arch, a drum): a door that opens needs a flat floor, and that needs a pad in the terrain.

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

The current town loads the model's terrain, streets and buildings as data. Update the model and re-export the town together for the next layout.

The model will also write what the guidance and the map need (planned, format 5 of the town file): the goals of the main path, each with its district and what it needs; the beacons (section 10.2); and a plan of both moments (section 10.4). Today the file carries only `steps`, one line of nine goals.

### 7.2 One hill town

| Place | Height (m) | Idea |
|---|---|---|
| The harbour | -6 (the quay) | the start; quays, a lighthouse, the sea gate |
| The lower town | -3 .. 0 | narrow streets under arches; the storm hit it hardest; the hub, where the three stairs part |
| The cistern and the gorge | -10 .. -6 | under the centre: the well, a tunnel, the floor of the gorge; water in A, walked in B |
| West: the market and the well | 0 .. 6, the bastion 14 | goes down: the great well to the cistern; the scaffold of the lantern; ends at the gate tower of the bastion, where the road warden keeps his post, with the rock in view |
| Centre: the Rock of the Bell | 14 | a round plaza on a rock in a gorge 20 m deep; the church and the bell tower; reached by the link of each district (the lift from the gorge in the west, the east bridge, the cliff way in the north); the finale |
| East: the locks | 3 .. 21 | climbs: five locks to the water tower, where the lockmaster keeps his post; entered by a stair from the lower town (to model: now it is entered from the rock by the east stair) |
| North: the upper street and the theatre | 21 .. 30 | high: the street ends at the top row of the theatre, which faces the rock; entered by its own stair from the lower town (to model: now it starts at the top lock) |
| The cliff: the graveyard | 42 | the view over everything; the watch post, where the watch captain keeps his post; the cliff way to the rock (to design) |

### 7.3 Rules

- **A hub, three ways and a rock.** The lower town is the hub: three stairs part there, one to each district, each marked by its glyph (section 10.5). Each way climbs, drops or crosses in its own manner and ends at its keyholder with the tower in view. The ways are joined by the rock, not by each other: a way's last puzzle opens its link to the plaza.
- **Equal ceilings.** The three ways have the same number of beats (three, then a keyholder) and reach the same difficulty at their last beat, because any one of them may be a player's first or last.
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
  in B the town let go (grass through the paving, figs in roofless houses, scrub over the groves, reeds in the harbour; on the heights a few people stayed, with smoke, washing and a lit window);
  nothing floats; the ground is under every floor and the sea or the land fills every view down.

### 7.4 The main path

After the harbour the town opens: three ways, then the rock.

| Stage | Where | What the player does | Ends with | State |
|---|---|---|---|---|
| 1. Teaching ground | the quay, the sea gate, the lower town | the switch, the causal rule, the switch in the air (section 8); the lantern's safe demonstration | the three gates of the lower town open; the tower with its three dark lamps stays in view | built, except the lantern's demonstration |
| 2. West way | up the market stairs, the market, the scaffold, the bastion | the lantern relay; the road warden's key | the gate tower of the bastion and its road warden, with the rock framed across the gorge; its link to the rock: the wreck of the west bridge (B), the gorge, the lift | market, scaffold, wreck and lift built (the day bell is to be replaced by the road warden); the return to the lower town is not modelled |
| 3. East way | a stair from the lower town to the first lock, up the locks to the water tower | water and the crate; the plank between two roofs; the lockmaster's key | the water tower and its lockmaster; its link: the east stair and bridge to the rock | the locks and the plank exist; the stair from the lower town is not modelled (the locks are entered from the rock today) |
| 4. North way | a stair from the lower town to the upper street, the theatre, the cliff stair | the aerial switch and the prism; the watch captain's key | the watch post by the graveyard on the cliff and its captain; its link: the cliff way to the rock (to design) | gatehouse crate and night bell exist (the bell is to be replaced by the watch captain); the stair from the lower town is not modelled (the street starts at the top lock today) |
| 5. The rock | the link of the district finished last, the plaza | the tower opens with the three keys; the climb inside; the lamps and the warning bell | the ending views (section 5) | lift and plaza built; the inside of the tower to design |

Rules of the order:

- The three ways open together, when the teaching ground is done, and are taken in any order. Each is complete from the lower town alone: no puzzle of a way needs anything of another, and nothing done in one changes what another asks.
- The return from a way's keyholder to the lower town is the one walk back this design accepts. It is not a puzzle, it sends the player to no other district, and it should take about 20 s at most and show a new view or open a shortcut when the key is taken (a rope, a chute, a lift).
- A link is a connection, not a key. A link opened early may let a player enter another way from the plaza, never past a puzzle of it; a viewpoint alone may be reached early.
- Each keyholder has his post at the end of his way (section 1.1). The bells of the model (the day bell in the market, the dawn bell in the water tower, the night bell on the cliff) are no longer goals; they become the hour bells of the town, heard and never needed.

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
| The order | the model is one line today: the west way leads to the rock, the rock to the locks, the locks to the street; the check does not yet test the three ways (section 11) |
| Still open | the stairs from the lower town to the east locks and to the upper street, the links to the plaza (7.4), and the exact walkable geometry from the plaza into the bell tower (section 5) |

### 7.6 Compact and connected town

Keep the complete town and its winding climb, but aim for about 60% of the current explorable footprint by shortening empty approaches, not by scaling every slope or jump. A roughly 20-second sprint without a decision, discovery or useful landmark is a candidate for compression, not a hard rule. Keep the tower as the skyline anchor. Visible doors, terraces and roofs should have an approach or clearly read as scenery. Never use a rewarding-looking doorway as an invisible boundary.

#### Exploration structure

- Different routes must reveal different information. The main street establishes the destination, roofs reveal district connections, and the gorge explains how the city's damage changed movement. A branch that only duplicates the main route should be shortened or removed.
- Reconnect familiar space while exploration is still active. A local shortcut is useful when it changes the next choice, reveals the vertical structure or removes an immediate repeat walk. Do not reserve a long return-to-start shortcut for after the finale.
- Start optional exploration with a visible spatial question and finish it with an answer. A grate, broken bridge, unreachable terrace or route seen from above can invite investigation; the payoff may be a connection, a new viewpoint or a visible causal result. Do not use mandatory collectibles to justify every branch.
- Teach one spatial rule at a time. First show the safe route or consequence, then reuse the same rule in a less obvious form. New geometry should combine known rules rather than add a new interaction verb.

#### Main spine and local loops

| Area | Readable forward route | Optional connection |
|---|---|---|
| Teaching ground: harbour and sea gate | Keep the teaching sequence. After the gate, frame the bell tower with the three dark lamps of its mast and the three stairs of the lower town together. | A freight-yard back door leads to a loading deck and returns by a side stair. It never bypasses the sea gate lesson. |
| West market, gorge and rock | Teach the lantern before the west bridgehead. In A the intact bridge ends at a closed gate; in B the fallen spans become the descent. Use the lift winch in A, then climb the surviving frame in B. | A grate at the west rim previews a maintenance gallery below the market. The lower latch is first reachable from the landing after the wreck descent. The gallery returns to the west rim and then rejoins the same landing; using it is optional. |
| East and north ways: roofs and theatre | Keep the plank lesson; only then split the route into street and roof paths. Both meet at the theatre backstage. | The street shows intact arcades; the roof previews the backstage and northern gatehouse. A side stair closes the local loop without a required return or pickup. |
| The rock and the tower | Frame the tower entrance, first walkable segment and harbour from the plaza, which the player reaches by the link of the last district finished. B exposes the approach; switch on safe ground into A and climb inside. | The warning bell and boats remain the payoff. Optional lamps can affect the visible boat outcome but cannot gate the tower, bell or any ending view. |

#### West loop review and implementation boundary

Keep the existing bridgehead, gorge and lift stations. Review the same west approach in A and B, then inspect the lift-house roof and the view back from the upper station. The first reveal is a change in topology: the intact bridge is not the solution; the wreck becomes the descent. From the upper station, the player should recognise the bridgehead and understand where the market gallery sits relative to the street. Preserve a continuous, safe forward route from the landing to the lift, independent of the gallery.

The gallery in the review is a proposed centerline, not geometry. First test an exposed retaining-wall gallery or a switchback before committing to a tunnel. Its gate is a local B-side latch with no effect in A. The present town's A-only causal objects do not implement it. Begin with a closed passage; wire a local latch only when needed, without changing the causal interaction contract or the lantern rules. Keep the gallery and its upper exit downstream of the lantern lesson; do not expose the lift or the plaza before their intended progression.

Place `west_loop_preview.py` beside `tools/layout/city_model.py` and run `python tools/layout/west_loop_preview.py` from the repository root with NumPy, Matplotlib and Pillow. It runs the existing street checks and writes `build/west_loop_preview.png`. Gold is the route intent; dashed teal is the gallery proposal. The sheet is rendered from the layout model, not the Windows game. Its schematic is not a collision, sight-line or clearance test.

### 7.7 The fabric of the town: fronts, blocks and a reason for every place

The town is not a scatter of houses with free ground between them. A real hill town (Dubrovnik, the villages of the Ligurian coast, Mont-Saint-Michel) is made of streets, and the houses are what stands along them: a house has a front, the fronts form rows, the rows share side walls, and what lies behind is a yard, a garden or a terrace. The games that are remembered for their places work the same way: a wall of Stormveil, a roof of Lordran or a district of Dunwall is a place that can be walked to, and what is seen from far away is reached later. This section turns that into rules; `python tools/layout/city_model.py check` keeps the first one.

**Rules**

1. **A house has a front.** Every house stands on a front (a street, a lane, the quay of the locks or the edge of the market plaza), its door to it and its side walls shared with the next house. No house stands in the middle of a block. The check fails when a door is more than 0.9 m from its front.
2. **Two deep at most.** A block has one row of houses on each front. What lies behind a row is a yard, a garden or a terrace, never another row without a front. A second row needs a back lane.
3. **Open ground has a use.** Ground that no house, street or puzzle uses is a garden: olives in A and scrub in B on the gentle terraces, net racks and drying yards where the fishermen live. No bare lot is left unexplained.
4. **Every place that can be reached has a reason:** a puzzle, a view, a piece of the story, or a connection that changes the next choice (7.6). A place that has none is removed or given one.
5. **What looks reachable is reachable, or reads as scenery** (7.6). A house on a front can be walked up to; a terrace behind a district wall is a garden seen over the wall.
6. **No lane opens a way round a puzzle** (7.3). A new lane joins the town only inside one way, and `tests/test_route.cpp` still proves that no handle is reached before its step. A lane up a terraced slope needs a long, gentle bank of ground beside it, and a gentle bank is a ramp: the lane is closed off by a district wall where the next terrace belongs to another way.
7. **The mast stays in sight.** No house stands in the sight lines from the quay to the three lamps of the signal mast (10.1); `house()` lowers a house there or leaves it out, and `tests/test_firstminute.cpp` casts the rays.

**What each place gives**

| Place | Its use | What the player finds there |
|---|---|---|
| Lower town (the fronts of the lower street and the lower east lane) | the fishermen and the merchants of the quay | rows of houses on both sides; net racks on the open ground to the west (A: nets hung, B: bare rails), the everyday life of section 9 |
| The market plaza | the heart of the west | the well, the market hall, the clock tower and the day bell; houses round it |
| The garden lane (west, new) | the terraced gardens above the market | houses stepped up the slope; **a cistern** with a closed hatch (A: a wooden lid, B: the lid fallen in), the first door of the gallery under the market (section 8.3, 7.6); at its end **a terrace** that looks over the town to the tower, a third high view in the west |
| The mill lane (east, new) | the mills above the five basins | a branch of the east stair along the slope; houses facing it and the quays; at its end **a terrace** that looks down on the basins and the water tower, a preview of the locks from above |
| The quays of the locks | the mills and the dye works, which need the water | a row of houses facing each quay |
| The upper street and the theatre | the facades of the north | the corridors of the plank and the gatehouse (walls of joined house fronts); houses between them |
| The orchards | air between the streets | olives on the terraces; nothing to find, and nothing that looks like a way |

**Houses that can be entered, and windows (built in pass 3, see 7.9).** Six to eight houses can be entered, each with one idea and no more: the table of an interrupted dinner in A and the same room roofless in B (section 9); a room whose window frames the next puzzle; a room whose floor is the roof of another. A house that can be entered has a door in a street front, one room at least 5 x 6 m with a ceiling of 3.4 m (the third-person camera must stay inside it), and its own light. The other houses are plainly scenery: a closed door, a shuttered window and a washing line say so, and no door that cannot be opened is drawn as if it could. A **window** is a frame in a wall that looks out on a place that is different in the two moments (a street whole in A, a heap in B) and points at the next puzzle; the player learns to look by the frame, not by text. One window for each way.

**Built (2026-11-26, pass 1):** the rule of the front (`fronts()`, `build_houses()` and the check; the houses that stood in the middle of blocks are gone: 115 houses before, 80% of them with no street to the door, about 55 now, every one on a front); the garden lane with its cistern and terrace, and a district wall along the north of the garden quarter; the mill lane with its terrace; the houses round the market plaza (one so far); the orchards and the net racks; the cistern and the two terraces are overlooks of the map. **Not built:** the houses that can be entered and the windows; the gallery behind the cistern hatch and the loop it closes (cistern, gallery, market); second rows on back lanes; the open ground of the west and the centre-north, which has no use yet (it is room for the loops and the houses that can be entered).

### 7.8 The street network: blocks, squares and dressed walls

Pass 1 of the town (7.7) gave every house a front, and that left the town thin: 55 houses on twelve streets, the middle of the lower town an olive field, the terraces of the west bare, and the walls of the corridors (the streets that the puzzles close) blank prisms 9.5 m high that read as a fortress, not as a street. This section is the second pass. The references are the same for every old town and for the games that borrow from them: the hill towns of the Ligurian and Dalmatian coast (lanes that follow the contour, stairs that join them, passages under arches, a square at every junction of consequence), the lower town of Dubrovnik (two rows of houses back to back between parallel lanes) and the way Stormveil, the Undead Burg and Kakariko tie every district to a loop, a landmark and a view. Kevin Lynch's five elements are the checklist: paths, nodes, landmarks, districts and edges.

**Rules** (continuing those of 7.7)

8. **Blocks, not rows.** Parallel lanes are 12 to 15 m apart and joined by passages every 12 to 17 m, so that a block is two rows deep and a yard behind them. A lane that does not join another ends in a loop, a square or a view (a terrace, a green): a dead end without a reason is a mistake of the layout.
9. **Every district has a node, a landmark and an edge.** The node is a square (the fish market, the market plaza) or a terrace; the landmark is seen from far (the chapel and its bell-cote, the market hall and its dome, the clock tower); the edge is a wall or a stair.
10. **Density falls from the hub to the rim.** Rows back to back in the lower town; houses cut into the slope and stepped on the terraces; gardens, plots and olives against the wall.
11. **A blank wall is not a street.** The walls of a corridor are the fronts of the houses along it: two rows of windows with sills and shutters, doors at the foot, a cornice and eaves at the top, a chimney now and then. The district walls have a coping and creepers. Everything on a wall is flat on it or above the top: the puzzles of the corridors (the barricade, the crates, the plank) rely on the walls being sheer, and nothing may be added that can be stood on.
12. **A yard has something in it.** A shed, a woodpile, a well-head, a cart or an arbour, so that the ground between the rows is a place and not a gap. At the doors: barrels, crates and benches, in the gap of 0.4 m between the wall and the edge of the lane, never on it.
13. **A new lane stays inside its district, and never opens a wall of a corridor.** It joins the lanes of the district it is in and no other; a lane that joins two terraces of two ways is a way round a puzzle, and so is a door in the wall of a corridor for a lane that comes from open ground: the walls of the corridors are what close the streets. A net lane from the south-west of the town to the foot of the market stairs was tried and removed for that reason (it went round the barricade of the lower street); `tests/test_route.cpp` proves that no handle is reached before its step and `tests/test_things.cpp` that the street of the air step is closed in A and in B.

**The new places**

| Place | Where | Its use | What the player finds there |
|---|---|---|---|
| Cooper lane and four passages | the lower town, between the lower street and the gorge | the block of the coopers and net makers | houses on both sides; a stone gate over the mouth of each passage with a lantern under it; barrels and crates at the doors |
| The fish market | where the cooper lane and the fish passage cross | the node of the lower town | paving, a fountain (water in A, a dry basin and a broken column in B), three stalls (awnings in A, bare frames in B), houses round it |
| The weaver lane and the green | west from the foot of the market street along the terrace of the market | the weavers | houses on the terrace; at the end a green by the wall with a bench, a cypress and a view over the harbour and the lower town (an overlook of the map) |
| The vine lane and the chapel | from the top of the market stairs round the market hall and up the terraces to the terrace of the garden lane | the vine growers; with the garden lane a loop that holds the cistern | houses stepped up the slope; the chapel of the terraces, a small stone chapel with a bell-cote that is seen from the whole west (roofless in B) |
| The tanner loop | below the upper street, between the plank and the gatehouse | the tanners | a loop with a block of houses inside it; it joins the upper street at both ends, east of the gatehouse corridor |
| The wall lane and the north terrace | from the end of the upper street along the north wall, after the gatehouse | the watch | houses on the side that looks at the theatre; at the end a terrace over the town (an overlook of the map) |

**Built (2026-11-26, pass 2):** the lanes and places above; the fronts of the corridors dressed (windows, doors, cornice, eaves, chimneys), the coping and creepers of the district walls, the windows and cornice of the market hall; the stone gates of the passages; yards with something in them; plots of vegetables and vines, olives, figs and cypresses on the open ground; barrels, crates and benches at the doors. **Not built:** the houses that can be entered and the windows of the two moments (7.7); arches that join the houses over a lane; second rows on back lanes; the gallery behind the cistern; a lane along the inside of the wall; the south-west of the town (a field with net racks: a lane there would have to be sealed from the hub first).

### 7.9 Rooms, windows, puzzles in the world, and what the references still have that this town lacks

Passes 1 and 2 (7.7, 7.8) gave the town fronts, lanes and squares. A town of fronts is still a stage set while no door opens, and a puzzle is still a prop while it stands alone in a street. This section is the third pass. It says what is built, why each puzzle is where it is, and, measured against the places that this one imitates, what is still missing.

**Rooms.** Six houses can be entered (`reserve_rooms()`, `build_rooms()`): a door in the front, one room of about 4.8 x 5.6 m, a ceiling at 3.4 m, a lamp in A, a roof in A and the open sky in B (a heap fell from the ceiling and a beam lies across it). Each has one idea and says it with objects, never with text: the fishmonger's table of an interrupted dinner (A: plates and a fire; B: the table broken), the cooper's workshop (barrels whole in A, staves and hoops on the floor in B), the weaver's loom (a red warp in A, the frame fallen in B), the miller's room (sacks, a hand mill, scales; spilt in B), the watch room with a telescope aimed out of its window (fallen in B), and the child's room (a small bed, drawings on the wall; in B a small glow on the bed, the one thing that was left). Where they stand: the fishmonger on the fish market, the cooper on the cooper lane, the miller on the lock quay, and the watch, the weaver and the child on the wall lane. No room stands in the west: every flat free place there is taken by a puzzle piece, a terrace or a landmark, and a room needs a flat floor (the body does not climb a step of more than a few centimetres, so a floor higher than the ground at the door gets a ramp of up to 0.35 m in front of the door, and nothing higher is accepted). Rules: the door is 1.3 m wide, with no lamp post within 1.7 m of it (the first test run found a post in front of a door); no furniture stands in the line from the door to the middle of the room; the third-person camera stays inside (the room is at least 4.8 x 5.6 m inside); a room has no second way out (nothing leads round a puzzle). `tests/test_town.cpp` walks the way from the street through the door to the middle of every room in A and in B (the model writes it as a street named `room: ...`).

**Windows of the two moments.** A window frames something that differs between the moments and that belongs to a way. Two are cut: the watch room looks from its back wall over the town at the theatre (columns and an awning in A, stumps in B), and the miller's room, through a window beside its door, across the quay at the locks (full of water in A, dry in B). A window is an opening with iron bars (gaps of 0.14 m: nobody passes, so a window is never a way in or out) and a sill at 0.95 m, and it is cut only where the model's own rays (five, over the width and the height of the opening) are clear in both moments (`room_window()`); otherwise it stays a flat lit pane and the check says so. Only the back wall and the front wall can look out: the side walls of a house in a row stand against its neighbours. The harbour and the west have no window yet: no room in the west, and none on the quay whose back wall faces anything.

**One look for what can be used.** In A every thing that can be used has a gold cap, grip or ring (a post with a gold cap at a crate, a plank, a lift; the cap on the drum of the sea gate; the gold grip on a bell rope), dull iron in B. It was a different invisible point for each. Rule: if it can be used, it is gold.

**Why each puzzle is where it is** (`dress_puzzles()`). Everything is flat on walls, at the sides, or over 3.5 m (above the reach of a jump): nothing stands on a way that a puzzle uses.

| Puzzle | The reason in the world | What says it |
|---|---|---|
| Sea gate | the customs shut the harbour at night | a chain from the winch up to the head of the portcullis; a lamp over the winch; the cargo behind the gate (A) |
| Canal crate | the crate is part of the customs cargo that is loaded across the canal | crates, barrels and sacks along both walls of the quay; a crane arm from the wall with a rope and a hook; three marks of a sledge on the quay in front of the crate, pointing at the canal (A) |
| Plank | roofers are repairing the roof across the yard | tiles, trestles and a bucket on the first roof; bare battens and then new tiles on the neighbour's roof; a door in its parapet (A) |
| Gatehouse crate | goods are hoisted into the gatehouse through a loading door | a loading door over the balcony, a hoist beam at the roof edge with a rope, the cart that brought the crate, sacks (A; the cart lies on its side in B) |
| Bells | the warning bell is rung by a rope, and the tower is kept locked by an ordinance that needs three keyholders | three gold-capped locks in the tower door; a rope with a gold grip at the bell (A) |

**Life.** Washing on a line over the passages and the lanes (A), carts, sacks and barrels at the doors, a boat on its stocks, a rope walk, net racks, a quiet garden with a pergola, a basin, benches and cypresses (the rest of the south-west and the north-centre, which had no use). B has the ribs of the boat, the posts of the shed and the posts of the pergola.

**What the references have that this town still lacks**

| Reference | What it does that this town does not yet | Action |
|---|---|---|
| Elden Ring, Stormveil Castle | rooms, stairs and ramparts joined in one place; everything seen from far is reached later; every part is a loop | built: rooms; the loops of the west and the east; not built: a walk along the town wall (a scenic loop, but a way round the puzzles unless it is sealed) |
| Dishonored 2, Karnaca | every building has two or three entrances (a door, a window, a roof or a pipe) and a story in its objects | built: stories in six rooms; not built: a second entrance for a room, and no more than six rooms (aim: ten) |
| The Legend of Zelda: Breath of the Wild, Kakariko | something new in sight every 20 to 30 seconds of walking (the triangle rule: a peak, a valley and what is in it); the way pulls by light and colour | built: the lamps of the main path lit in both moments; the gold of the handles; not built: a colour or light for the path that is seen from far |
| Dark Souls, the Undead Burg and Lordran | a shortcut that opens a way back to a bonfire | not adopted: after the finale nobody walks back, so a shortcut back has no use (decision of 2026-11-24) |
| The Witness, Portal | a puzzle is a language: one look for one verb, taught safely, then twisted | built: one look for use; each way has teach, test and twist (8); not built: the same verb twice in one view |
| Return of the Obra Dinn, Gone Home | a story told by objects in a room, no text | built: six rooms; aim: one more story that spans two rooms (the child's room and the chapel) |
| Italian hill towns, Dubrovnik | washing lines, arches that join the houses over a lane, a fountain in every square, stairs that are streets | built: washing, the fountain of the fish market, the stone gates of the passages; not built: arches that join houses over a lane |
| Lynch: paths, nodes, landmarks, districts, edges | all five in every district | paths and nodes: yes; landmarks: the tower, the chapel, the clock tower, the water tower; edges: the walls and the gorge; weakest: the north district has one landmark, the theatre, and the graveyard is far |

**Built (2026-11-26, pass 3):** the six rooms, two windows where the rays allowed (the check lists them), the gold of the handles, the dressing of the puzzles, the washing, the boat yard with a rope walk, the quiet garden. **Not built:** a room in the west and the windows of the harbour and the west; the gallery behind the cistern hatch (it needs a hole in the terrain, and the hole is a way round the puzzles of the west until the well and the lift are designed together: it waits for that design); arches that join houses over a lane; second rows on back lanes; a walk along the wall; four more rooms; a second entrance for a room.

### 7.10 Buildings, and an open town with shut thresholds

Pass 4 of the town (2026-11-26). Passes 1 to 3 (7.7 to 7.9) gave the town fronts, lanes, rooms and puzzles with a reason. Playing it showed three things that are still wrong. This section says what is built for them and what is proposed.

**What is wrong**

1. **The houses are slabs.** The model's `house()` builds a box 3.6 to 5.6 m wide and 5.0 to 10.6 m high: width over height is 0.34 to 1.12 and 0.63 on average, and 80% of the houses are narrower than 0.8 of their height. Each has one door, two windows and one roof shape, and there is nothing to read from 20 m. A hill town is made of houses that are as wide as they are high: two storeys, 6 to 8 m wide, with a roof that is a shape of its own.
2. **The landmarks are boxes.** The market hall is 12 x 9 x 8 m with a dome, the clock tower 4.5 x 4.5 x 20 m with a gable roof, the water tower a ten-sided drum with a cone, the front of the theatre five columns and an awning.
3. **The puzzles close streets with walls.** `corridor()` puts a wall 9.5 m high on each side of a street for 20 to 24 m (the lower street, the market stairs, the market street and the upper street twice: 106 m of street, 211 m of wall), and every puzzle piece has walls at its sides. They read as a fortress, and the town beside them cannot be entered: a street that a puzzle closes is a tunnel, not a place.

**Built: a library of buildings** (`tools/layout/architecture.py`, checked by `tools/layout/architecture_check.py`, drawn by `tools/layout/architecture_preview.py`). The model uses it since pass 5 (6.9). A building is a function of a frame (the middle of the front wall at the door, a along the street, b into the building, y up from the floor) and of its sizes. It writes polygons through an emitter, so it needs nothing of the model and can be drawn without the town.

| House | Width x depth (m) | Storeys | What it has | Where |
|---|---|---|---|---|
| row | 5.4 - 7.4 x 6.8 - 8.6 | 2 | string course, windows with sills and shutters (square or round-arched), a door with a step and a lantern, a balcony, a bay window or an overhanging upper floor, a dormer, a chimney | everywhere |
| altana | 5.8 - 7.4 x 7.0 - 8.8 | 2 | the same, with a wooden loggia on the roof | terraces |
| shop | 6.2 - 8.4 x 7.0 - 9.0 | 2 (3 from 7.6 m) | a wide arched shop front with a counter, an awning (A) and a sign | hub, squares |
| portico | 7.0 - 9.0 x 8.0 - 9.6 | 2 | a covered porch of two round arches, the upper floor over it | squares |
| cottage | 5.0 - 6.6 x 5.2 - 6.6 | 1 | a steep roof, low eaves at 3.4 m, a pergola with a vine, a bench | rim, gardens |
| palazzetto | 8.6 - 11.4 x 8.4 - 10.0 | 3 | corner stones, a round-arched door, a balcony, a cornice, a hip roof | squares |

A house is never higher than 1.25 times its width (`house()` removes a floor until it is not): the proportion of the old houses cannot come back. In B a house is whole, unroofed (rafters and a heap) or a ruin (a low wall, moss), as the model's `damage()` says. Windows, shutters, trims, signs and awnings are decals (`overlay`): nothing to collide with or to climb. A house is 108 to 376 polygons (82 to 238 solid).

| Landmark | Size | What it has | Polygons |
|---|---|---|---|
| Market hall | 15.0 x 9.4 m, 17 m high | a covered market that can be walked (an arcade of five round arches on each long side, piers, stalls, barrels), a hall above with arched windows, pilasters, a balcony and a coat of arms, a hip roof with dormers, a cupola with a copper dome | 945 |
| Clock tower | 5.6 x 5.6 m, 30 m high | a stone base with a round arch that the street runs through, niches, a clock on each side, a balcony, a belfry with eight open arches and a bell, a copper pyramid roof with four pinnacles | 491 |
| Water tower and aqueduct | 3.4 m radius, 22 m high; 30 m long | a twelve-sided drum with buttresses and blind arches, a door, a gallery, a tank house with iron hoops, an open belfry with a bell, a cone roof; an arcade of seven arches that can be walked under | 310 + 165 |
| Chapel | 5.4 x 8.2 m, 9.5 m high | can be entered (a round-arched door, a nave, a half-round apse, an altar, benches, a lamp), a rose window, a bell gable with a bell, buttresses; A has a ceiling and a roof, in B the roof is gone | 261 |
| Customs house | 26 x 9 m, 17 m high | a hall under an arcade of seven arches toward the quay (crates, barrels, bollards, a hand crane), a floor with pedimented windows, a middle gable with a coat of arms, a cupola | 659 |

The front of the theatre, the night bell tower and the lighthouse keep their old shapes for now.

`plan_row()` fills the edge of a street with houses: the kind by the density of the front (`hub`: shops, rows, porticoes, a palazzetto; `terrace`: rows, altane, cottages; `rim`: cottages), the width and depth by kind, party walls of 0.12 m, a floor at the ground of the door plus 0.12 m and a podium of stone where the hill falls away, the side walls dressed where they can be seen (a step of the row of more than 0.8 m), a lot tried narrower and shallower where it does not fit. It calls `accept(outline)` for every lot, so the model's `site_free()` keeps its say.

`architecture_check.py` (494 checks, none fails) builds every type many times and checks: every polygon is convex and planar with 3 to 255 vertices (the game cuts a polygon into a fan of triangles), every material is one the town has, a house is at least 0.8 as wide as its eaves are high (0.84 to 1.94), and there is no foothold: no upward face of a solid between 1.2 m and 3.0 m (a jump is 1.65 m, a crate 1.2 m more) stands out of a house by more than 0.3 m. The check found two mistakes in the first drawing: the corbels of a balcony began at 2.95 m, and the low eaves of a cottage (2.6 m) were a way onto its roof.

**Proposed, not built: an open town with shut thresholds.** The town is built as a town first, and the puzzles are thresholds in it.

1. **No street is a tunnel.** There is no wall of a corridor. Both sides of every street are real rows of houses, with side passages and courtyards.
2. **A district is closed by what a hill town is closed by:** the town wall, the cliff, the gorge, water, terrace walls of 4.5 m (`district_wall()`) and the backs of continuous rows. The line that closes a district has one crossing: the threshold.
3. **A threshold is something a town has:** a gate, a bridge, a stair with a collapsed flight, a canal with a quay, a passage under a house with a shut door, a gatehouse, a lift. The puzzle works on the threshold and on nothing else.
4. **Inside a district everything that is open is a place with a use:** a square, a lane that ends in a court, a shop room, a terrace with a view. Nothing is empty and nothing leads round.
5. **The route solver judges, as before:** no handle is reached before its step. New: it counts the cells that can be reached before each step (it prints them now: 67,837 before the sea gate and 100,097 before the canal crate in the last run). The aim is that every stage opens at least a quarter more than the one before it, and that everything in a district can be reached before its threshold.

| Puzzle | Now | Candidate |
|---|---|---|
| Sea gate | a portcullis in the harbour wall | stays; two gate towers and the customs house beside it, whose arcade is the first place to explore, with the cargo that explains the gate |
| Air step (lower street) | walls on both sides, a balcony, a heap | a house across the street with a passage under it, its front the balcony, in B the corner fallen; the lower town beside it is a real block (cooper lane, fish market, passages) closed on the west by the terrace wall and the gorge |
| Canal crate (market stairs) | a corridor and a canal | the customs canal crosses the stair at a quay (cargo, a crane, the canal); houses on both sides with side doors and a court |
| Day bell and lantern (market) | a scaffold on a terrace | the market hall and the clock tower with its scaffold; the day bell is the tower's |
| Plank | two roofs across a street | the same, with the roofs of the library (6.5 m and 9.5 m); the roofers; no corridor |
| Gatehouse crate | a gatehouse with a balcony | a gatehouse that straddles the street with a loading door; the quarter is closed by rows and the wall lane |
| Lift and bridges | the gorge | stay |

Alternatives that were weighed. **Soft gating** (nothing is blocked: a puzzle opens a shortcut or a view and progress is knowledge, as in Breath of the Wild or The Witness) is more open, but the measure of progress changes and the first minute is harder to guide. **Covered streets** (the closed streets stay, but under continuous houses with arches and upper floors over the street, as in Dubrovnik) is a cheap middle way for the one or two streets that cannot avoid a long closure.

Cost and risk. `build_houses()`, `corridor()` and the sides of the pieces change. The town grows by about 15,000 polygons for some 75 houses of about 200 and by about 3,000 for the landmarks (it has 46,000 now). The corridors do the sealing today, and the solver found two leaks while the lanes were added (the garden lane in 7.7, the net lane in 7.8): a district closed by rows leaks wherever a row has a gap, so every step is run through `tests/test_route.cpp` and `tests/test_things.cpp`.

**Built: the library, its checks and its pictures, and its use by the model (pass 5, 6.9: the houses, the market hall, the clock tower, the water tower and the chapel). Not built: everything under "Proposed".**

### 7.11 The town as a place to play: reasons to go, and the open threshold

Pass 6 (2026-11-26). Passes 1 to 5 made the town real. This section asks whether it is worth walking, from the numbers of the town and from the way the references keep a player moving, and says what is built and what is proposed.

**The town in numbers** (the model and `tools/layout`): 23 streets, lanes and stairs, 840 m (558 m of lanes, 173 m of streets, 109 m of stairs); a player walks 5.5 m/s (8.8 sprinting) and jumps 2.0 m, so the whole network is about two and a half minutes in a straight line and a beat every 25 m is a beat every 4.5 s. 15 dead ends and 6 independent loops (the lanes that are only joined by a puzzle are counted as separate). Five streets are closed by their puzzle for 106 m between walls of 9.5 m; 9 puzzles; 10 rooms and 1 chapel that can be entered; 1 memory court; 11 places that reveal the map when stood on. Inside the wall (17,800 m2 without the gorge) the houses cover 10.7%, and 29% of the ground is more than 6 m from any house and any street. The town is small: it has to be dense, and what is in it has to be worth a detour.

**What the references do, and the rule each gives.**

| Reference | What holds the player | Rule for this town |
|---|---|---|
| Elden Ring, Stormveil Castle | one route that doubles back through itself; every wall seen from below is walked on later; a shortcut opens behind the player | R1 (seen before reached), R4 (loops inside a district) |
| Dark Souls, Undead Burg and Firelink | a hub that is returned to; a place of safety every few minutes; connections that look impossible and then click | the lower town is the hub; the lamps of the main path are the places of safety |
| Dishonored 2 | every block has two or three ways in; reading the place gives the route | rooms with a window on the next place (7.9), a court that is a way in and not the way on |
| Breath of the Wild | the triangle rule (a peak, a valley and what is in it); something new every 20 to 30 seconds | R2 (a beat every 25 m), R1 |
| Tunic | what is locked is seen first; knowledge and not an item opens the way; secrets are in the margin | R1, R6 (the threshold is seen from far) |
| Hollow Knight, Dirtmouth | a hub that fills with life as the player goes on | the nine lights of the tower (R7) |
| Return of the Obra Dinn, Gone Home | a story told by rooms and the things in them | rooms (7.9, 7.11), the court |
| Super Mario Odyssey | something to find every few dozen steps, in every direction | R2, R3 |
| Dubrovnik and the Ligurian hill towns | a formal main street, irregular lanes, a square at every junction of consequence, stairs that are streets, and a dead end that is a court | 7.8, the memory court |

(What is said of the games is the common reading of their design, taken as a method, not as a source of numbers.)

**The rules.**

- **R1. A landmark before the way to it.** Every junction shows at least one landmark that cannot be reached yet; every landmark is seen from three places before it is reached. The tower and the mast are seen from the first frame, the clock tower from the market stairs, the chapel's bell-cote from the whole west, the water tower from the locks, the theatre from the upper street, the graveyard from the plaza.
- **R2. A beat every 25 m.** A beat is a puzzle, a room, a court, a view, a vignette that is different in A and in B, a shortcut or a secret. A stretch of 50 m of lane with none of them is a bug.
- **R3. A dead end ends in something.** Of the 15 dead ends, 5 end in a place (the green of the weaver lane, the terrace of the garden lane, the terrace of the mill lane, the terrace of the wall lane, the court of the lower street), 8 end where a way is meant to end (the two bastions and the quay of the stairs, the last quay of the upper street, the graveyard, the two ends of the harbour, the lighthouse), and **2 end in nothing: the west end of the cooper lane and the north-east end of the lower east lane**. A dead end that ends in nothing is a bug (to do: a stair, a court or a shop front with a bench and something in A and in B).
- **R4. A loop in every district**, of 60 to 120 m: the lower town (the lower street, four passages, the cooper lane and the lower east lane), the west (the garden lane and the vine lane, with the chapel and the cistern on it), the north (the tanner loop). The east has none yet.
- **R5. Three levels in every view**: the street, a terrace or a roof, the cistern or the gorge below. The crown of 7.11 makes roofs of the walls.
- **R6. Compression and release.** A lane of 2.4 to 3 m opens into a square, a court or a terrace every 40 m, and a wide place is entered through a narrow one. A threshold is a narrow place that is seen from a wide one.
- **R7. A reason to switch.** Every district has a pair of things that are not the same in A and in B, to be found by pressing Tab where nothing says so: the fountain of the court (water, a dry basin with a lamp), the model of the town on the scribe's table (a wall, a ring of stumps), the lanterns of the lamplighter (one lit in B), the sails of the sailmaker, the child's bed (a small glow in B), the cistern lid. The nine lights of the ring on the tower are the natural count of them (see below).
- **R8. A puzzle is a threshold, not a tunnel.** Nothing a player can walk is a corridor between two walls that has nothing to see at its sides. A puzzle is at a gate, a bridge, a quay, a stair, a passage under a house; the places beside it are real and have something in them.

**The districts, as a plan.**

| District | What the player learns or does | Seen from far | What is found there (beats) | Loop | The echo of A and B | What is missing |
|---|---|---|---|---|---|---|
| Harbour and lower town (the hub) | the switch, the causal rule, the lantern | the mast, the lighthouse, the gate towers | sea gate, air step, five rooms (fishmonger, cooper, baker, scribe, lamplighter), the fish market, the court, the boat yard, the rope walk | lower street, four passages, cooper lane, lower east lane | the fountain and the lamp of the court, the town on the table, the lit lantern | a second court; the net lane (sealed from the hub first, 7.8) |
| West: the market and the terraces | the lantern as a light and a bridge | the clock tower, the chapel's bell-cote, the market hall | canal crate, scaffold, the road warden at the bastion, weaver, chapel, cistern, the green of the weaver lane, three terraces | garden lane and vine lane | the cistern lid, the chapel roof, the loom | the gallery under the cistern; the market hall cannot be entered (a hillside of 12 m: not worth a cut) |
| Centre: the Rock | the finale | the tower, the mast | lift, bridges, gorge floor, plaza | the ring of the plaza | the moat and the dry gorge | none |
| East: the locks | climbing and water | the water tower, the aqueduct | the lockmaster at the water tower, mill lane and its terrace, the miller, the sailmaker, the locks | none | the basins full and dry | a loop; the mills themselves |
| North: the upper town and the theatre | the aerial switch, the prism | the theatre, the graveyard | plank, tanner loop, gatehouse crate, watch, weaver and child rooms, the wall lane and its terrace, the watch captain on the cliff | tanner loop | the stage, the stumps of the seats | the front of the theatre; a way down from the wall lane |

**The nine lights (proposed, needs the game).** The ring of nine lamps of the memory on the tower (`lamp(..., "memory")`) is dark from the start and lights as the player finds the nine places that hold a memory: the court, the child's room, the scribe's model, the lamplighter's lantern, the chapel, the cistern, the weaver's loom, the miller's window on the locks, the watch room's window on the theatre. A lit light is seen from the whole town (R1 reversed: a goal that grows), counts without a number, and is the reason to visit a place that is not on the way. It needs a counter in `scene/` and a lit state of those lamps in `Lamps.h`; none of it is built, and it needs no text.

**Built in pass 6.**

- **A court that is a dead end that is worth it.** The lower street has a gate in its north wall (a round arch of 4 m at 45 m) to a mouth of 3.4 m and a court of 6.4 x 7 m, walled all round with garden walls of 4.5 m (as the walls between the districts, so that the court is no way round a puzzle), a fountain to one side (the middle line is the way), an olive, benches, laundry, a door at the end, a lamp of the memory in B and an overlook of the map. Two lanes of the town file (`lower court mouth`, `lower court`, level) so that the ground, the lamps, the map and the walker of `tests/test_town.cpp` know them. Only the lower street had room for one: the other four corridors are filled by their puzzle piece and its keep-clear circles (8.5 m).
- **The corridors have roofs** (`crown_bay()`): over the wall of each bay of 6 m (a house), out of reach, a level top of its own (some a storey higher than the next: the skyline steps), a gabled roof with eaves over the street in the tile of the bay, a chimney, a dormer, three metres of house behind the wall, and in some bays a balcony at 7.4 m with a French window, the one level of balconies (out of reach of everything a puzzle gives: two crates and a jump 4.05 m, the roof of the plank 4.8 + 1.65 m; never within 11 m of a puzzle piece). The walls below are the walls of passes 2 and 5: sheer.
- **Four rooms**: the baker (an oven with a fire in A, a table of loaves), the scribe (a model of the town on a table, books, a map), the lamplighter (lanterns on chains, a ladder; one lit in B), the sailmaker (sails on a pole, coils of rope; on the lock quay: the barges have sails): ten rooms. A room is no longer placed at a pier of an arch (`KEEP_CLEAR` of 3.4 m at each pier, and a door is refused within a small place that is kept clear): the first run put one there, in front of a pier, and the walker could not enter it.
- **The ground** (`build_terrain()`): the material of a cell is chosen by fields (steepness, height, the noise of the fields) cut where they cross a limit, along the triangles of the fan of the cell, so that the edge between grass, field, sand and rock is a smooth line and not a staircase; the surface is the same, so the solid, the route and the map are the same. The cells outside the wall are 10 m (they were 20 m) out to 420 m.
- **B is weathered stone in the moon** (6.9, rule 7 and `B_AGE`), not ice; **the flying camera** is faster (12 m/s, Shift x4) and Space lifts it (update 024).

**Proposed, not built, in the order to do it (each step is one run of the route solver, 8 minutes, and a look).**

1. **Shorten the corridors to their piece.** A corridor is 20 to 24 m long for a puzzle piece of 10 to 12 m. Cutting each to the piece plus 4 m (the district walls already hold the district) frees 8 to 12 m at each end, where a court, a side lane or a room fits. Risk: each shortening is a new leak until the solver says it is not.
2. **Move the three tightest puzzles to a threshold**: the canal crate to a quay (a canal that crosses the stair, a crane and a footbridge, houses on both banks with doors), the plank to a bridge between two houses of the upper street (the two roofs stay), the gatehouse crate to the gatehouse itself (the street passes through it and a loading door over it). Each is a design decision of 8.
3. **The east loop**: a lane from the mill terrace down to the fifth quay, so that the east also has a circuit (R4).
4. **The nine lights** (above).
5. **The theatre front, the town wall and its towers** in the vocabulary of the library (7.10), so that the town no longer has two styles.

**Built: items of "Built in pass 6". Not built: everything under "Proposed".**

## 8. Puzzles

Every way is built from the same pieces. This section says how a room is made, which templates a room may use, and what each way is built from. It is the baseline on the uploaded model, not the final layout of the compact town.

### 8.1 How a room is made

1. **Three beats.** Each way has three puzzle rooms in this order: **teach** (the rule is shown where nothing can go wrong, and the answer is in view), **twist** (the same rule in a less obvious form), **test** (the rule combined with one idea taught before, in a larger space). The keyholder closes the way: the last beat ends with him checking his own sign and handing over his key. The teaching ground teaches the rules; no way teaches a rule that another way needs.
2. **One idea per room.** A room has one new thought and no more actions than principle 3 allows.
3. **Seen from the entrance.** The end of a room (the gate, the far bank, the high lamp) is in view before the first act; the way to it is not.
4. **The answer is in the room.** The ruin shows what is missing (a plank in the wrong place, a gate open beyond a gap). No clue is carried from another room or another way.
5. **Hidden ways pay the player who looked.** An optional room is entered by something seen and not yet understood, and ends in a connection, a view or a street lamp (section 4).

Borrowed from, without copying: the one-idea rooms of the Zelda shrines and the Elden Ring catacombs, where the end is seen first, the answer lives in the room and a hidden wall rewards a player who looked.

### 8.2 Templates

| Template | The thought | Rests on | Used in |
|---|---|---|---|
| Trace | "I did it there, so it is like this here" | what is done in A shows in B (3.3) | every way; the base of all others |
| Lantern relay | a circle is carried across by hand, never by the switch: cross on the held floor, call the lantern back (E) once on B ground, set it again | the lantern is set in A only; a pick-up that leaves the body on nothing is refused (3.2) | the west way |
| Held floor | a floor of A kept standing by the lantern is the one step in B | 3.2 | the west and north ways |
| Water level | water holds in A and is a dry bed in B; a crate floated in A lies on the bed in B | 3.6 | the east way |
| Prism | a thing of A made to last in B, never the other way | 3.6 | the north way |
| Aerial switch | the jump starts in one moment and ends in the other | 3.1 | the teaching ground, one twist of a way, optional rooms; never the only test of a room; the margin of section 11 |
| Hidden way | a wall of A that is a gap in B, shown from the street by a draught or a light | 3.6 | optional rooms |
| Latch of B | a latch that works in B only and is local to a gate of B; it changes nothing in A, so 3.3 holds | 3.3 | the optional west gallery |

### 8.3 The ways

| Way | Idea | Teach, twist, test, keyholder | State |
|---|---|---|---|
| Teaching ground: harbour and lower town | walking; the switch; the causal rule; the lantern | **The switch:** the game starts in B on the quay, a heap of wreck closes the way west in B only. **The causal rule:** the portcullis of the sea gate is shut in both moments; its gold winch raises it in A, where the cargo of the customs fills the passage behind it; in B the cargo is gone and the portcullis is raised where A left it, with its trace. **The switch in the air:** the lower street is closed in A by the barricade of the customs (5 m) and in B by the front of a fallen house (3.4 m over the street); two crates of A climb to 2.4 m; from the top one a jump with a switch at its top lands on the heap of B, whose far side steps down to the street. **The canal of the customs:** a crate pushed in A floats to the far quay and lies on the dry bed in B (it replaces the crate puzzle of the customs-house cloister; proved by the solver, at most two switches). **The lantern:** a short safe stretch where the lantern is shown in hand, set down in A, seen as a warm patch in B and taken up again, before the three gates open (section 10.6) | built (2026-11-25, `tests/test_things.cpp`); the lantern's demonstration planned |
| West: market and well | the lantern relay | **Teach:** the scaffold of A under a shed roof by the terrace of the market; the lantern on it, climbed in B. **Twist:** the great well: the wooden stair of A kept standing by the lantern in the dry shaft of B. **Test:** two gaps in a row with one lantern, ending at the bastion. **Keyholder:** the road warden at the gate tower of the bastion. He will not send a town up a road that is not open; the way removes what closes it (works, a jammed winch) and the open road is what he checks | scaffold placed (2026-11-25); the well a sketch; the relay and the position of the bell to design |
| East: the locks | water and the crate | **Teach:** a crate floated in A to where it is needed lies there on the dry bed in B. **Twist:** the plank laid between two roofs in A holds in B. **Test:** the tower lock made the same way. **Keyholder:** the lockmaster in the water tower; his gauge, no longer hidden by a jammed lock, shows the basins rising and falling out of step with the tide table | plank placed (2026-11-25); the rest to build |
| North: theatre and graveyard | the aerial switch and the prism | **Teach:** the gatehouse of the theatre: a crate pushed under its balcony in A, then a switch in the air onto the heap of B. **Twist:** the prism moves a thing of A into B. **Test:** the fallen back wall of the theatre passed with the lantern and a switch together, not with the switch alone. **Keyholder:** the watch captain at the post on the cliff; with his view cleared he sees long lines of swell on a flat sea and no birds. The traces of the whole game are seen from the cliff | gatehouse placed (2026-11-25); the rest to build or design |
| The rock: link of the west way | the causal rule at a large scale | the wreck of the west bridge down into the gorge (B); the lift lowered in A and climbed in B (7.4); the gate of the west bridge stays shut (seen, not crossed) | built (2026-11-25); the tower itself to design |

## 9. Environmental storytelling

The story is in section 1.1 and its ending in section 5. Main-route play does not depend on knowing the backstory. Keep story beats in places the player already explores rather than in collectible-triggered scenes.

- **Everyday life before the flood:** in A, set a wedding table, hang fishing nets to dry or show the market just after closing. Return to the same place in B to show what survived, what broke or what someone deliberately preserved. Use composition and props rather than explanatory text.
- **Sail and cloth clues:** each highlighted wreck has a recognisable sail colour; somewhere in its owner's district, use matching fabric in an everyday scene. When a boat escapes in A and its matching wreck is missing in B, the player who noticed that clue can recognise whose boat it was. Pair colour with a pattern or silhouette when possible. Clues are not items, a checklist or a condition for saving a boat.
- **Overlooks:** keep a high point in each district that frames a visible next destination and, when the terrain permits, the bell tower. The view should tell the player where a route might run, not just show scenery.
- **Not carried forward:** the player-caused flood twist, graves as the finale payoff, unlockable memory scenes and a story-based true-ending requirement. Further backstory is not needed for the first playable finale.

## 10. Guidance and immersion

The player must always know what the game asks and where to look next without pressing a key. The help keys are for the player who wants more, not for the one who is lost. Nothing is written.

Make the environment explain the next move before adding any marker: frame the destination from the entrance, give the next interaction a recognisable silhouette, and make the difference between A and B readable at the obstacle. Teach one new idea at a time and let the player try it somewhere safe before requiring it on the main path. Use brief key prompts where needed, not written walkthroughs. Do not reveal optional routes by default or show the complete answer before the player has tried.

### 10.1 Four layers of goal

| Layer | The player asks | The game answers with | State |
|---|---|---|---|
| Far | Where does this end? | the three lamps of the signal mast over the bell tower, in sight from the first frame, dark until they are lit at the top of the tower (the goal; progress is shown by the emblems of 10.5) | built (2026-11-26) |
| Way | What is in this district? | the three gates of the lower town, each with its glyph; on entering a way, a beacon (10.2) | planned |
| Room | How do I get past this? | the room itself: the end in view, the ruin showing what is missing, A and B readable at the obstacle (3.4, 8.1) | built in part |
| Help | I am stuck | the flame of the lantern and H (10.3), the map (10.4), the lens (3.8) | the flame, H and the map are built; the lens is planned |

### 10.2 Beacons

A beacon is a place the player can see and cannot yet reach, at the start of a way and of a room: a warm window high in the bastion tower, a fall of water at the top lock, a stage lit by one lamp. It is the question; the way to it is the puzzle. It is part of the town, seen from the street, and not a marker. The map shows a beacon the player has seen as a ring that fills when it is reached. First candidates: the bastion tower (west), the top lock (east), the theatre stage (north).

**Why a mast.** The bell tower itself cannot be the far goal from the quay: the sea wall, 8 m in front of the start, hides it in both moments (`tests/test_firstminute.cpp` casts the rays from the camera of the first frame; the tower's south face is blocked in A and in B, and only the top 5 m of the spire of A clears the wall, while the game starts in B). So the three lamps hang on a signal mast that stands over the tower and clears the wall: three round lamps on a yard at 61 m, the mast coming out of the dome of the church and, in B, rising free over the broken shaft. The player sees the lamps from the quay, the sea stair and the sea gate; from the streets of the lower town the houses hide them, and the flame and the map take over.

### 10.3 The lantern as compass

The compass is the lantern, so help is an object of the world and not a layer on the screen. Built (2026-11-26): `scene/Guidance.h`, `scene/Flame.h`.

- **The flame leans (always on).** The flame over the lantern, in the hand and set down, leans 18 degrees toward the goal: the first step of the main path that is not done, and the door of the tower when all three keys are held. (The first design said about 10 degrees; that was too small to see at the distance of the camera, so it is a setting, `[guidance] lean_degrees`.) It stands upright within 2 m of the goal.
- **It grows with need.** After 30 s without progress (a step done, a new 6 m cell of ground, or a new closest approach by 4 m) the lean grows to 40 degrees and the flame throws sparks.
- **H, first press:** the lantern is raised (the arm goes up) and a warm beam sweeps over the place for 6 s: it swings wide and settles on the bearing of the goal. It shows the place, not the act.
- **H, second press** during the sweep, and only after 90 s without progress, so the player has tried first: a frame round the thing to use (the winch, the crate, the plank), joined to the lantern by a line of motes, for 12 s. Before the 90 s a second press only puts the sweep away. A press while the frame shows puts it away.
- **Not built:** the third press (the pulse at a suggested switch point, an arc for an aerial switch); the flame standing upright in the lower town, where the three gates are the choice (the main route is one line, not three ways yet: section 7.4).
- **Rules.** The player escalates by pressing and by time without progress, never by distance to an answer. The flame never points at an optional room (there are none yet). With the lantern set down its flame leans where it stands. Warm guidance in B must not be confused with the lantern patch or a causal trace: gold means use, the lantern's colour is the flame.

### 10.4 The map: fog, memory and two moments

Built baseline: `scene/MapMemory.h`, `render/MapView.h`, the plan in the town file. The map is an exploration reward rather than a complete plan handed to the player. M opens it over the frozen game.

- **Precision and style.** The plan samples the town every 0.5 m, but the map never shows the cells. Every ink is drawn as a smooth shape: the plan is averaged over 2 x 2 cells and cut along the half level, so a staircase of cells becomes a slope and a corner is cut (when the window holds many cells, the whole town, the ink is cut on a grid of 2 x 2 cells with partial cover, which keeps the shapes and halves the polygons). Cliff on the plan is only terrain steeper than about 57 degrees and wider than one cell (the risers of the hillside's terraces, 41 degrees and up, made long thin stripes across the town and are not cliffs), and it is cut a little higher than the other inks. It is still ink on tracing paper, not a satellite image. A uses warm violet-gold ink and B cool teal-blue ink. North stays up and is marked.
- **Fog of the unknown.** The whole town is under fog from the first frame, and the paper shows all of it (zoom 1), so the player sees how large the place is and how little of it is known. Unvisited cells are a soft cloud, violet in A and teal in B: thin at the edge, thick inside, with pale and dark wisps, and an uneven edge made by noise that only ever reaches into the seen part, never away from it. Nothing of what lies under the fog is inked. Walking within 15 m clears it; overlooks reveal a wider silhouette.
- **Main puzzle knowledge.** Every puzzle still to be solved shows through the fog as a broad ring with a question mark, not as a place or an instruction: the ring is 12 m across for the next one (with a second ring and a soft disc) and smaller and fainter for the others, and its centre is a few metres from the true place. Optional puzzles, shortcuts and hidden things are never exposed by the fog. The ring goes when its puzzle is solved.
- **Solved regions.** A solved puzzle lights its district (22 m round it) for good: its map is complete, not fogged.
- **Things as they are now.** The footprint of every box of a thing in its present state is drawn on the sheet of the moment the box belongs to (strong on the sheet on top, faint on the other): a crate where it was pushed to, the plank once laid, the stairs of the lift in B once it is lowered. A thing in its second state has a pale outline, so that a gate whose leaves rise (no change on a plan) still shows that it was used. Only where the place has been seen; bells are not drawn (the lamps of the mast show them). Not built: water levels (the sea is the same in both moments).
- **Two moments.** The default view is the current sheet; Tab turns A/B without switching the world. Beside the paper are clickable MAP and DIFFERENCE choices. DIFFERENCE suppresses cells that are the same in A and B and leaves only the warm/cool mismatches, so a player does not have to compare two small maps by eye.
- **Height (not built).** The map follows the player's vertical band (underground, street, high town) and can be changed manually; other bands remain as faint context when structures overlap. It needs a layer of heights in the plan.
- **Markers.** Player markers remain manual and never reveal information. Up to ten marks with six shapes (flag, diamond, round, triangle, square, star); a mark remembers the moment in which it was placed (A warm, B cool) and is paler on the other sheet. The next shape is chosen with T or the MARK button, which shows it.
- **Controls.** M opens/closes, Esc closes, Tab turns the sheet, E places/removes a marker. The pointer clicks MAP, DIFFERENCE, ZOOM +, ZOOM -, CENTER and MARK. Keys: + and - (number pad, Page Up and Page Down too) zoom from 1 x (the whole town) to 4 x, the arrow keys move the window, C or R puts it on the player. The window never leaves the plan. No fast travel and no corner minimap.

Implementation is split into two passes. Pass 1 raised plan precision to 0.5 m, added the fog and the clickable difference view. Pass 2 (built 2026-11-26) adds smooth shapes, the whole-town fog, the rings of the puzzles, solved illumination, the things as they are now, zoom, pan, centre and ten typed markers. Not built: height bands (the plan has no heights; it needs a new layer in the town file).

### 10.5 The glyph board

This is the quest system without words. Each way has a glyph, simple enough to read at 64 px in both colours: a sphere for the west (the road warden), a column for the north (the watch captain) and a box for the east (the lockmaster). The glyph is carved on the stair gate of its way in the lower town, on the post of its keyholder, on its key and on its lock in the door of the tower. It is dark until the key is taken and then warm, in both moments, and the post lamp of a keyholder who has given his key burns in B and is seen across the town. The three glyphs are the whole quest log: no list, no counter, no marker that names a task; optional street lamps are not on it. The same three glyphs are on the map. The signal mast is not part of it: its three lamps are the goal and are lit last. Built (2026-11-26): three lamps on the mast, each lit by a bell, which this scheme replaces (section 14). Not built: the glyphs on the gates, the posts, the keys and the door.

### 10.6 The lantern's demonstration and the first minute

Give the lantern an early, safe demonstration in the lower town, before the three gates open and before the west scaffold, where it is first required. Show the lantern in hand, placing it in A, the warm patch that retains A inside B, and how to retrieve it. The existing town build already supports placement in A and a patch of A in B; the problem reported in play is that the feature is not apparent. Validate any hint route with the real character and camera, not only the reachability solver. (Not built: KI-14.)

**The first minute, as built (2026-11-26).** The game starts in B on the quay, and the first frame is set by the town file (spawn yaw -0.55, pitch +0.29): the heap of wreck to the left, the winch of the sea gate left of the middle, and the three dark lamps of the mast high in the sky right of the middle. `tests/test_firstminute.cpp` keeps it so for the lenses of 60, 70 and 90 degrees and the screens 16:9, 16:10 and 4:3 (the lamps are at 0.88 of the half height in the narrowest case), and proves with rays that nothing of the town is in the way of any lamp in either moment. The flame of the lantern in the hand leans toward the winch, which is in the picture. After 8 s of walking, to a player who has not switched, the prompt says TAB SWITCH WORLD; after 40 s, M MAP. The first minute is tested uncoached (section 12): the player sees the lamps, goes to the sea gate without being told, and can say what the game asks.

### 10.7 What replaced the fallback

The fallback of 2026-11-26 (a pillar of light over the next step, shown in the first 20 s, on H and after 30 s without progress, with lights running toward it) is gone: a player who never pressed H had no goal, and the pillar was the answer, not a question. What was kept: the measure of progress (a step done, a new 6 m cell, a new closest approach by 4 m), now the clock of the flame; the prompt TAB SWITCH WORLD; and, in the second press of H, a faint frame round the thing to use. The first step of the main path is still the goal of the flame; `steps` becomes `goals` with a way and what each needs when the three ways are modelled (section 7.4, 12).

### 10.8 Sound

Use sound as a second cue: harbour activity and gulls in A; wind, dripping and the far sea in B. Distinguish switch, refusal and lantern placement by sound once audio is added. The warning bell at the top of the tower sounds across the town; the hour bells are ambient and never a goal.

## 11. Limits and proof

The body walks at 5.5 m/s and jumps 2 m; a running jump covers about 3.6 m (numbers in
[CONVENTIONS.md](CONVENTIONS.md)).

| Item | Main path | Optional routes |
|---|---|---|
| Horizontal gap | 1.8 m | 2.5 m |
| Step up | 1.2 m | 1.8 m |
| Switch in the air | most tried moments work | a few work |
| Landing | at least 2 x 2 m | at least 1 x 1 m |
| A miss costs | a stair back, or the lamp of the room | the lamp of the room |
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
| Guidance | `tests/test_guidance.cpp`: the goal; the flame (it leans toward the goal, restless after 30 s, calm after progress, upright at the goal, the same on the lantern set down and on the avatar); the presses of H and their times (the sweep, the frame only after 90 s, away when a step is done); what is drawn; planned: the flame upright in the lower town, nothing pointing at an optional room |
| The lens | `tests/test_lens.cpp` (planned): the solid does not change while it is open; tap and hold on the ground; the switch at once in the air; it closes when the body leaves the ground |
| The map | `tests/test_map.cpp`: the plan of the model (both moments, the moat in A only, the streets under their middle lines); what is seen (the radius, the overlooks, never more); the markers (five at most); the paper (inside every screen, the player and the tower on it, the window); the ink (neutral, warm, cool, the sheet on top, blank paper, the lamps, the arrow, the flags); the whole town on the map fits the batch |
| The first minute | `tests/test_firstminute.cpp`: rays from the camera of the first frame to every lamp in both moments; the sea wall does hide the tower itself; the lamps, the winch and the heap inside the picture for every lens and screen; three lamps, dark at the start (in the new design they are lit together at the top of the tower); the compass points into the picture; the heap closes the quay in B only |
| The light | `tests/test_lighting.cpp`: the maths of the shader run as C++ (the fall-off of a lamp, one lamp on a surface, the soft shoulder, the fade at the edge of the shadow map); the choice of at most 32 lamps (the moment, the cap, a far lamp fades out, no tie blinks, the flicker); the shadow box (what is inside it is inside the map, depth runs from the light, it moves on the texel grid, a light overhead); the settings; the lamps of the town file, the lantern, the lamps lit by the bells and the halos |
| The style of the light | `tests/test_style.cpp`: the functions of the style run as C++ (the rings and the warmth of a lamp, the bands of the sun, the clouds: their range, their share of the sky, the drift, the same value along a ray of light; the rim; the cool fill only in the shadow; the painted stone and the damp; the haze; the grade: identity when neutral, the middle grey kept, no negative colour, the corners), the numbers that feed them (the defaults, nonsense pulled in, the config keys, the five registers of `MakeStyle`: plain is all zeros, the shafts need the shadows, A and B differ) and the looks (the cool fill and the tones keep the brightness) |

## 12. Order of work and acceptance

1. Third-person movement, sprint, ledge climbing and a visible lantern: a first pass is built. Tune against a tight alley, a raised ledge and a switch in the air before changing the whole town.
2. **The teaching ground as the reference district, with the new guidance.** Built (2026-11-26): the signal mast with its three dark lamps in view from the quay, the flame that leans, H, the map, the first frame. Not built: the lantern's safe demonstration; the three gates with their glyphs; a beacon at the start of the first way; the lens. Validate the art on the quay-to-sea-gate route before extending it (section 6.8). Give first-time players a clear destination, one worthwhile detour and a visible reward for changing the past. Ask them to play without verbal coaching and measure: within 60 s can they say what the game asks; do they take a stair unprompted; how often do they press H; how many times do 30 s pass without progress; where do they lose the route, cannot read an interaction or expect an unreachable platform to be playable. The light is built (2026-11-26: the shadows, the lamps, the shoulder, the style; section 6.3) and not yet seen in the game: play E-21 to E-25 of [BUILD.md](BUILD.md) first.
3. **Make the order free.** Model the stair from the lower town to the east locks and to the upper street, and the links to the plaza; replace `steps` by `goals` with their ways; extend the solver to the tests of section 11; keep the old line as a baseline until they pass.
4. **The west way first** (it holds the lantern relay and the most built content), then the east and the north, each with three beats and a keyholder, each tested uncoached. In an uncoached west test, check whether players look down from the bridgehead, recognise it again from the upper station and find the lift without taking the optional gallery. Test locked and released gallery states with sprint, mantle and aerial switches. A viewpoint alone may be reached early. Re-test safe recovery and optional-lamp independence. The old route solver is a baseline, not proof for the new controller.
5. Build the bell tower finale and boat payoff after the movement and exploration loop is stable. Verify the route to the final bell from the link of each district, in each of the six orders, and all ending states, including a run with no optional lamps.

The uploaded town already loads `data/town.dlt`, supports the lantern patch and causal objects, and has a solver for the old route. Keep those results as a baseline only. Before updating the layout broadly, review the first district with a new player: can the player find the route without coaching, explain why a trace appears in B and use the lantern unprompted at the first required obstacle? Test safe camera behaviour on every switch, a refused climb with inadequate clearance, a refused switch into solids or water, and recovery without lantern-dependent support. Test zero, some and all optional lamps; the same main route and ending views must remain available.

## 13. Decisions

- **2026-11-24:** one complete town, A before B, past-to-present cause and effect, one held patch from a placed lantern, no mandatory district-spanning errands and no post-clear shortcut back to the start.
- **2026-11-25:** current town starts in B; the harbour teaches the switch and the one-way causal rule. The lantern receives a safe early demonstration in the next version. The town has a first third-person player pass. A denser town, fallback guidance and optional street lamps remain planned. Use the central warning bell and boats for the finale. See [Changelog](../CHANGELOG.md) for prototype history.
- **2026-11-25, exploration pass:** linear harbour onboarding, a readable main spine and two optional local loops. Review the west bridge reversal first; leave the east loop and northern climb as follow-up prototypes.
- **2026-11-26, guidance and order:** the main route is no longer one line. The harbour and the lower town are the teaching ground; after it the town opens into three ways (west, east, north), done in any order, each of three beats and a bell, and joined by the rock, not by each other. The tower opens when its three emblems are lit (since 2026-11-27: with three keys); its progress mark is the glyph board and nothing else. Guidance is layered: a goal in sight, a beacon at each way, the lantern's flame as a compass (always on, growing with need), H for more, a map of two sheets of tracing paper with up to five markers, and a lens on holding Tab. Not adopted: a corner minimap, a task list, a compass bar, narration. The pillar of 2026-11-26 is a fallback only.
- **2026-11-26, the first minute and the map:** the bell tower cannot be the far goal (the sea wall hides it from the quay in both moments, measured), so the three emblems hang on a signal mast over it, in sight from the first frame. The guidance of the fallback (the pillar) is replaced by the flame of the lantern, H in two steps and the map on two sheets of tracing paper. The lens, the beacons, the lantern's demonstration and the three ways are still to build.
- **2026-11-26, the light:** the sun and the moon cast shadows; every lit lantern of the model is a light; the lantern of the traveller and the lamps that the bells light are lights of what the player did; the sum goes through a soft shoulder instead of being cut off, and a halo puts a lamp in the air. No lamp is ever needed to read the route (6.3). Lamps cast no shadow; there is no picture in high range and no bloom yet.
- **2026-11-26, the style of the light:** the light is hand-painted, not photographic: soft bands of sun, a cool fill in the shadows, drifting shadows of clouds, a rim, lamps in rings, a painted stone that is damp near the water, rays of light through the shadow map, a haze with a hue and a grade of the picture (6.3). One master and a plain view (the key L) keep it comparable, and no rule of the game is carried by it. It had been "not built" (shafts) and "no post-processing" (6.8): the grade is a step of the model shader, not a pass, so the second still holds.
- **2026-11-26, the fabric of the town:** a house stands on a front or not at all (7.7). The block fill that put 115 houses in the middle of blocks, 80% of them with no street to the door, is gone (about 55 houses stand on fronts now). Two lanes are new, the garden lane in the west (cistern, terrace) and the mill lane in the east (terrace); open ground is gardens, not bare lots; six to eight houses that can be entered and one window for each way follow (pass 2).
- **2026-11-26, the street network:** the town is made of blocks, squares and dressed walls (7.8): 5 lanes and a square in the lower town, a lane and a green in the west, a loop below the upper street, a lane and a terrace along the north wall, a chapel on the vine lane; the corridors are dressed as house fronts (windows, doors, cornice, eaves) and nothing stands out of a wall that could be stood on. A lane stays inside its district.
- **2026-11-26, rooms and puzzles in the world:** six houses can be entered (7.9), each with one idea told by objects; two windows are cut (the theatre from the watch room, the locks from the mill): an opening with bars that frames something that differs between the moments, cut only where the rays are clear in both; the west has no room yet; everything that can be used is gold in A; each puzzle has a reason in the world (a cargo, roofers, a loading door, a customs winch); the south-west and the north-centre have a use (a boat yard, a quiet garden). The gallery behind the cistern waits for the design of the well and the lift together.
- **2026-11-26, the buildings:** a house is as wide as it is high (never higher than 1.25 times its width) and has the parts of a facade (7.10); the landmarks are designed buildings (market hall, clock tower, water tower with an aqueduct, chapel, customs house), some of them to walk into or under. The library (`tools/layout/architecture.py`) is built and checked; the model does not use it yet. The proposal of an open town with shut thresholds (7.10) waits for a decision.
- **2026-11-26, one style:** the town has one palette (`tools/layout/town_style.py`: one stone, eight lime washes, one tile in four ages, accents on purpose, gold for use), B is derived from A by a rule, and the stone, plaster, tile and ground of the town are painted by the engine (`TownData::Painted`). The library of buildings of 7.10 is in the town (45 houses, 6 rooms, the market hall, the clock tower, the water tower, an enterable chapel). Rules 1 to 9 of 6.9 are checked by the model.
- **2026-11-26, the town as a place to play:** nine rules for a town worth walking (7.11): a landmark before the way to it, a beat every 25 m, no dead end that ends in nothing, a loop in every district, three levels in every view, compression and release, a reason to switch, a puzzle is a threshold and not a tunnel. Built in pass 6: a court that is a dead end worth the walk, roofs over the corridors, four more rooms, a smooth ground (and, in update 024, a B that is weathered stone in the moon and a flying camera that rises with Space and goes four times faster with Shift). The corridors are shortened and the three tightest puzzles move to thresholds only after a decision (7.11, the order of the proposal).
- **2026-11-27, the story:** the one disaster is a storm surge that arrives under a clear sky (1.1), not a dam failure. A is the calm evening before it and B the town about twelve years later, with a few people on the heights. The traveller is a stranger who carries a harbour lantern that has kept that evening; the lantern is the one impossible thing. The three bells, and the rule that all three lamps be lit before people leave, are gone. The tower is locked by an ordinance that needs three keyholders (water, sea, road); each has his post at the end of a way and gives his key when he can check his own sign, so the traveller never has to be believed. The three emblems on the gates, posts, keys and door are the progress mark; the lamps of the mast are the goal, lit at the top with the warning bell. A district whose key is taken is lived in in B, a third state that is neither A nor the ruin (6.4). This replaces the bells, the lamps lit by bells and the dam in the entries of 2026-11-26 and in sections 4, 5.2 and 10.5.

## 14. Open

- The town as a place to play (7.11): look at B from the air and the quay with `B_AGE` at 0.65 (is it still too grey, too lilac, too blue: change it in `tools/layout/town_style.py`); walk the court, the four new rooms and the streets under the new roofs (are the roofs and the balconies too busy, is the court worth the gate); fly with Space and Shift. Then decide the order of the proposal: the corridors shortened, three puzzles at thresholds, the east loop, the nine lights, the front of the theatre and the town wall. Two dead ends still end in nothing (the west end of the cooper lane, the north-east end of the lower east lane).
- One style (6.9): look at the town in the game with `wash` at 0.9, 0.6 and 0 and choose; is the painted stone too blotchy on plaster; is the hub colourful enough and the rim quiet enough; are the cool family (9%) and the accents (0.7%) too few; which of the old shapes (the town wall and its towers, the theatre, the aqueduct, the bell tower) now looks the least like the rest. Then: the town wall and its towers in the vocabulary of the library, and the decision of 7.10 (an open town with shut thresholds), which decides whether the 8.5 m round each puzzle piece can be built.
- Decide the proposal of 7.10 (an open town with shut thresholds): the rules, the seven candidate thresholds, and whether the one or two streets that cannot avoid a long closure get covered streets. Then: take the library into the model (`build_houses()` by `plan_row()`, the landmarks, the corridors out), run `route` and `things` at every step, and measure the cells reached before each step. Look at the pictures (`python tools/layout/architecture_preview.py`): are the houses too regular, is the palette right, which landmark still reads as a box.
- Pass 3 of the town (7.9): walk into the six rooms from the street in A and in B (does the camera stay inside; is the door easy to find; do the windows frame something worth looking at, and is it different in the other moment); is the gold cap on the handles the first thing the eye finds at each puzzle, and not too much; does the dressing of the puzzles make them look natural or does it give the answer away; count the open ground that still has no use; decide the next four rooms.
- Pass 2 of the street network (7.8): walk the lower town from the sea gate (do the passages and the fish market read as a place; is the stone gate over a passage a frame for the view along it), the weaver lane to its green, the vine lane and the garden lane as a loop with the chapel and the cistern, the tanner loop and the wall lane; look at the corridors from the street and from the air (do they read as house fronts; are the windows too regular); count the open ground that still has no use.
- Pass 2 of the town (7.7): six to eight houses that can be entered (door, one room of at least 5 x 6 m, 3.4 m ceiling, its own light, one idea each), one window of the two moments for each way, the gallery behind the cistern of the garden lane and the loop it closes, back lanes for second rows. Walk the first pass: do the fronts read as streets from the ground, does the open ground of the west and the centre-north feel unused or restful, do the olives read as gardens, and does the garden lane's terrace show the tower?
- Tune the camera, sprint speed, ledge height and jump envelope on the reference route.
- Model the three ways: the stairs from the lower town to the east locks and to the upper street, the link of each district to the plaza (the cliff way in the north is not designed), and the return from each keyholder to the lower town (its limit is in 7.4).
- Set each keyholder's sign and the ordinary cause that hides it before the way (the road warden, the lockmaster, the watch captain: section 1.1); the lines of the three conversations and of the A people whose changes show in B; and the content of the lived-in state of each district (6.4). Ask uncoached players after the finale what happened that evening and why the town looks different: the answer should be that the warning was given and the boats left.
- Move the lamps of the mast to the new rule (dark until the top, lit from the lantern with the bell). Build the keys, the three locks in the tower door and the emblems on the gates, posts, keys and door, and update the first-minute test (section 11). Prototype the top of the tower: one action or two.
- Prove the west gallery alignment and lower latch in collision, then finalise playable roofs, interiors and the approach from the plaza into the tower; verify the final bell with actual input, not just route reachability.
- Set optional lamp locations, boat mapping and escape counts; prototype the Past, Present and Both views before choosing their controls.
- Tune the guidance and the map on play: the lean of the flame (18 and 40 degrees), the 30 s and 90 s times, the size and glow of the three lamps against the sky of B and of A, the first frame (yaw and pitch of the town file), the colours of the map, the reveal radius (15 m). If the tap on the ground feels late, move the lens to the right mouse button (3.8). Draw the three glyphs for the gates. Audio and further environmental detail can follow after the route works.
- Tune the light on play (E-21 to E-24): the lamps and the lantern against the moon of B, the weight of the shadows (0.85), the moon (0.8) and the exposure; whether the lit lamps of B read as a trail; whether the shadow box (60 m, 2048 texels) is sharp enough at the quay and soft enough on the far town; the cost on the laptop (the shadow pass draws the town a second time).
