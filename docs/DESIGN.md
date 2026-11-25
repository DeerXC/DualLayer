# Design

The next version of DualLayer. Unless identified as a current-build baseline, this document describes the target design, not shipped features. The uploaded baseline is documented in [Architecture](ARCHITECTURE.md) and [Build](BUILD.md).

## 1. The game

A third-person 3D exploration puzzle without combat. One walled harbour town on a hill, at two moments:

- **A, the past**: a warm evening, the town whole and empty, a minute before something ended;
- **B, the present**: the same town much later, in ruins under a cold moon after the flood.

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
5. **No forced backtracking.** No puzzle sends the player to another district. A local connection may return to known ground only when it offers an immediate route or useful new perspective before the finale; no shortcut sends the player back to the start after the clear.
6. **Nothing that looks like a bug.** The body is never pushed out of a wall; a switch or a pick-up into something solid is
   refused instead.
7. **Two states of one town.** Light and colour distinguish the moments at a glance; damage, materials and signs of use keep the difference readable without colour (section 6).
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

## 4. Progress and return

- **Main route:** the existing district bells are prototype milestones. Keep the main route readable and the final bell reachable without lighting any optional street lamps. District machinery may change a view of town without creating a required errand in another district. The district gate requirements should be checked when the map is compressed.
- **Anchor lamps:** checkpoints, not collectibles and not optional street lamps. Place them after meaningful progress. A fall or R returns to a safe floor in both moments without depending on the placed lantern. In the uploaded town baseline, this is the last safe standing point; anchor-based recovery and lamp travel belong to the legacy greybox unless migrated.
- **Optional street lamps:** light a lamp in A and it remains lit in B. A lamp can clarify a nearby route or reveal a landmark. Reward exploration with changes visible in the town and with the boat outcome in section 5, not a collection counter. Memory fragments, fragment-based radius upgrades and unlockable story scenes are not part of the new route.

## 5. Bell tower finale and ending views

### 5.1 Final approach (layout to prototype)

From the northern overlook, frame the bell tower, a recognisable entrance and the first walkable segment together. Keep the approach local. Prototype a broken approach in B that reveals the route, a safe position to switch to A, and a short climb through the intact tower. Use the established rules for any lantern-held floor or ledge; a new timing mechanic, long backtrack or precise aerial switch is not required. The bell platform must be reachable through an intended path, not by standing on a wall. Leave enough room to read the bell, the interaction and the harbour below with the third-person camera. The layout and whether the final climb requires a lantern placement remain play-test decisions.

### 5.2 The warning and its consequence

An upstream dam fails and floods the town. The central bell is rung in A to warn the people at the harbour before the flood reaches them; it does not stop the dam failure, control time or serve as a navigation signal. Establish several recognisable wrecks in B at the opening. Distinguish them by sail colour and, where useful, silhouette or a matching fabric pattern. In A, show the matching boats leaving their exposed berths after the bell. In B, the corresponding wrecks are absent, but the flood damage to the town remains. Show an absence in the same framing used at the opening so the cause and result can be read without dialogue. No playable boat sequence is required.

Optional lamps increase the number of boats shown escaping without unlocking the tower, the bell or access to an ending view. The first prototype should show a visible baseline payoff with zero lamps and a larger payoff after exploration. The exact count, which lamp affects which boat and how the lamps relate to departure routes remain open; do not treat a one-to-one mapping as settled. Use the same recorded boat outcomes for all three views of a run.

### 5.3 Past, Present and Both

| View | What is shown |
| --- | --- |
| **Past** | The warning in A and the identified boats pulling away from the quay before the flood. |
| **Present** | The damaged harbour in B. Locations previously occupied by the matching wrecks are now clear. |
| **Both** | Match the quay framing across A and B so a departing boat and its empty wreck site read as one cause and one result. The town is not restored. |

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

- Use one directional sun or moon with soft sky and ground fill. Keep detail visible in shadow. Start with a 2048 px directional shadow map focused on the reference district, with the character, larger plants, buildings and boats casting shadows. Expand coverage only after checking both target machines.
- Start with baked or material-level contact shading at wall bases, door recesses, steps and under eaves. Consider screen-space ambient occlusion only if the first pass still looks flat and there is performance headroom.
- Replace the solid sky with a procedural gradient. A moves from muted blue-grey overhead to pale gold at the horizon, with a low western sun. B moves from deep grey-blue overhead to a misty cyan-grey horizon, with cool moonlight.
- Use restrained distance and height fog to separate the quay, sea wall, streets, bell tower and hills. Frame a readable route in the middle distance and keep the main landmark visible. Fog and shadows must not conceal the next landing or an important interaction.

### 6.4 The same town at two moments

| | A: the past | B: the present |
|---|---|---|
| Light and air | Long amber sunlight, cool soft shadows, light golden haze | Cool fill and moonlight, deeper blue-grey shadows, low cold mist |
| Built surfaces | Dry stone, intact doors and roofs, timber and terracotta | Damp wall bases, greyer wood and rusted fittings; selective roof damage and debris |
| Life and vegetation | Useful objects in place, crates stacked, plants tended around houses and water | Misplaced or missing objects, moss in shade, growth through cracks and rubble, changed water levels |

Changes should follow drainage, exposure, damage and disuse, not a uniform filter. Preserve enough shared geometry to recognise the same place immediately. In a greyscale A/B comparison, the player should still distinguish the moment through silhouette, value, damage or signs of use.

### 6.5 District identity and vegetation

- **Harbour:** hoist, warehouse doors, mooring posts, ropes and boats. Put the strongest visual cue on the way from the quay to the sea gate.
- **Housing:** narrow doors, recessed windows, chimneys and occasional balconies or washing lines. **Market:** cloth awnings, timber stalls and a few carts. Keep crossings clear of decorative clutter.
- **Bell tower:** regular masonry, a clear arch and visible steps distinguish the approach from residential streets. Keep the entrance legible, not just the tower spire.
- Replace cone placeholders near the player with trunks, a few branches and grouped low-poly foliage. Use sparse grass clumps and a few greens. Place growth by moisture, soil and shelter: more near water and ruins, moss on damp shaded bases, and little in the middle of an intact route.

Props and planting should identify where the player is, point towards the next route or show how the place changed. Do not add a high-contrast door or platform that looks usable if it leads nowhere.

### 6.6 Water

Keep one stylized water language across the sea, harbour, pools, channels and waterfalls. Start with scrolling surface normals, a stronger reflection at shallow viewing angles, colour by depth and restrained shore foam. Use warmer reflections and clearer green-blue shallows in A; use deeper blue-green, less transparency and cooler reflections in B. Match ripples and foam to flow, impact or rising and falling water. Make the shoreline and hazardous depth readable before adding richer reflections. Sound and gameplay rules are not changed by this visual pass.

### 6.7 Character and lantern

Keep the current low-poly harbour traveller's teal coat, rounded hood and small pack as the starting silhouette. Refine proportions, visible joint separation and the shapes of cuffs, boots and straps before adding surface detail. Read cloth as matte, leather as slightly smoother and brass as a controlled highlight. The same silhouette and materials carry across both moments; make the figure readable against both sunlit stone and cold ruins without relying on a heavy outline.

The lantern is the character's strongest warm accent. Its handle, frame and light source should remain identifiable when carried or placed. Show and hide the hand-held model with the existing lantern state. Keep visual geometry separate from the collision body; the interaction rules stay in sections 3.2 and 3.7.

### 6.8 First playable art pass

1. Work on the existing **quay to sea gate** route, including a tight passage, a water edge and the view to the bell tower. Start with sky, fog, directional shadows and ambient fill; then apply the core materials and water. Add district props, plants and character refinements only where needed.
2. Capture paired A/B views from the same third-person camera positions. Check that stone, timber, plaster and roof tiles read at play distance; the route and water edge are clear; the tower holds the skyline; and the character and lantern stand out in both moments. Repeat in greyscale and verify performance on the laptop and desktop before extending the pass to the rest of town.

The first rendering pass implements the gradient sky, ground fill and distance and low mist. Cast shadows, surface materials, water refinements and the paired Windows play-test remain open. Do not rebuild the city or add advanced shadow and post-processing systems before the reference route proves the visual direction.

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
2. Up the market stairs to the market, over the terrace with the lantern, to the west bastion. The bridge to the rock is
   seen: in A it ends at a shut gate on the rock, in B it has fallen.
3. In B down the wreck of the bridge, its spans lying round the gorge like a stair, onto the dry floor of the gorge.
4. The lift of the rock: on the roof of the lift house at the foot (reached in B, switched to A over the moat) its winch
   lowers the cage in A; in B the cage lies at the foot and its frame stands, a ladder of girders up the face (the causal
   rule). Never lowered, the cage fell and tore the frame down with it.
5. On the plaza; over the east bridge to the locks.
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
| Still open | the exact walkable geometry from the northern overlook into the bell tower (section 5) |

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
| Harbour and sea gate | Keep the teaching sequence. Frame the bell tower and the first climb together after the gate. | A freight-yard back door leads to a loading deck and returns by a side stair. It never bypasses the sea gate lesson. |
| West market, gorge and rock | Teach the lantern before the west bridgehead. In A the intact bridge ends at a closed gate; in B the fallen spans become the descent. Use the lift winch in A, then climb the surviving frame in B. | A grate at the west rim previews a maintenance gallery below the market. The lower latch is first reachable from the landing after the wreck descent. The gallery returns to the west rim and then rejoins the same landing; using it is optional. |
| East roofs and theatre | Keep the plank lesson; only then split the route into street and roof paths. Both meet at the theatre backstage. | The street shows intact arcades; the roof previews the backstage and northern gatehouse. A side stair closes the local loop without a required return or pickup. |
| Northern overlook and tower | Frame the tower entrance, first walkable segment and harbour. B exposes the approach; switch on safe ground into A and climb inside. | The warning bell and boats remain the payoff. Optional lamps can affect the visible boat outcome but cannot gate the tower, bell or any ending view. |

#### West loop review and implementation boundary

Keep the existing bridgehead, gorge and lift stations. Review the same west approach in A and B, then inspect the lift-house roof and the view back from the upper station. The first reveal is a change in topology: the intact bridge is not the solution; the wreck becomes the descent. From the upper station, the player should recognise the bridgehead and understand where the market gallery sits relative to the street. Preserve a continuous, safe forward route from the landing to the lift, independent of the gallery.

The gallery in the review is a proposed centerline, not geometry. First test an exposed retaining-wall gallery or a switchback before committing to a tunnel. Its gate is a local B-side latch with no effect in A. The present town's A-only causal objects do not implement it. Begin with a closed passage; wire a local latch only when needed, without changing the causal interaction contract or the lantern rules. Keep the gallery and its upper exit downstream of the lantern lesson; do not expose the lift or the plaza before their intended progression.

Place `west_loop_preview.py` beside `tools/layout/city_model.py` and run `python tools/layout/west_loop_preview.py` from the repository root with NumPy, Matplotlib and Pillow. It runs the existing street checks and writes `build/west_loop_preview.png`. Gold is the route intent; dashed teal is the gallery proposal. The sheet is rendered from the layout model, not the Windows game. Its schematic is not a collision, sight-line or clearance test.


## 8. Puzzles by district (draft)

These are baseline puzzles on the uploaded model, not final layouts for the compact town. Keep the original teaching intent where useful. Give the player a safe lantern demonstration before the west market scaffold; the scaffold is the first required use, not first exposure.

| District | Taught or used | Sketch |
|---|---|---|
| Harbour and lower town | walking; the switch; the causal rule; the switch in the air | **built** (2026-11-25, tests/test_things.cpp): beat 1, the switch: the game starts in B on the quay, a heap of wreck closes the way west in B only; beat 2, the causal rule: the portcullis of the sea gate is shut in both moments; its gold winch raises it in A, where the cargo of the customs fills the passage behind it; in B the cargo is gone and the portcullis is raised where A left it, with its trace; beat 3, the switch in the air: the lower street is closed in A by the barricade of the customs (5 m) and in B by the front of a fallen house (3.4 m over the street); two crates of A climb to 2.4 m; from the top one a jump with a switch at its top lands on the heap of B, whose far side steps down to the street. beat 4 (placed 2026-11-25), the canal of the customs: a crate pushed in A floats to the far quay and lies on the dry bed in B. **Replaced**: the crate puzzle of the customs-house cloister, re-made so that the crate is moved in A and lies in B where A left it, proved again by the solver, at most two switches |
| West: market and well | the lantern (first met here); the orb | **placed** (2026-11-25): the scaffold of A under a shed roof by the terrace of the market; the lantern on it, climbed in B. Earlier sketch: the wooden stair of A kept standing by the lantern in the dry shaft of B; the day bell at the foot |
| Centre: the rock | the causal rule at a large scale | **built** (2026-11-25): the wreck of the west bridge down into the gorge (B), the lift lowered in A and climbed in B (7.4); the gate of the west bridge stays shut (seen, not crossed) |
| East: the locks | water and the crate | **placed** (2026-11-25): the plank laid between two roofs in A holds in B. Still: a crate floated in A to where it is needed lies there on the dry bed in B; the tower lock re-made the same way; the dawn bell in the water tower |
| North: theatre and graveyard | the prism; the switch in the air | **placed** (2026-11-25): the gatehouse of the theatre: a crate pushed under its balcony in A, then a switch in the air onto the heap of B. Still: the prism moves a thing of A into B; the fallen back wall of the theatre passed with a switch in the air; the night bell; the traces of the whole game seen from the cliff |

## 9. Environmental storytelling

The flood premise is in section 5. Main-route play does not depend on knowing the backstory. Keep story beats in places the player already explores rather than in collectible-triggered scenes.

- **Everyday life before the flood:** in A, set a wedding table, hang fishing nets to dry or show the market just after closing. Return to the same place in B to show what survived, what broke or what someone deliberately preserved. Use composition and props rather than explanatory text.
- **Sail and cloth clues:** each highlighted wreck has a recognisable sail colour; somewhere in its owner's district, use matching fabric in an everyday scene. When a boat escapes in A and its matching wreck is missing in B, the player who noticed that clue can recognise whose boat it was. Pair colour with a pattern or silhouette when possible. Clues are not items, a checklist or a condition for saving a boat.
- **Overlooks:** keep a high point in each district that frames a visible next destination and, when the terrain permits, the bell tower. The view should tell the player where a route might run, not just show scenery.
- **Not carried forward:** the player-caused flood twist, graves as the finale payoff, unlockable memory scenes and a story-based true-ending requirement. Further backstory is not needed for the first playable finale.

## 10. Immersion and guidance

- Make the environment explain the next move before adding markers: frame the destination from the entrance, give the next interaction a recognisable silhouette, and make the difference between A and B readable at the obstacle. Teach one new idea at a time and let the player try it somewhere safe before requiring it on the main path.
- Keep the three agreed guidance aids as a fallback. A subtle pillar identifies the current local goal. Moving lights indicate direction, with warm light for A, cool light for B, and a distinct pulse at a suggested switch point; an arc can suggest an aerial switch. Show guidance during the first lesson, then offer it on H or after roughly 30 seconds without meaningful progress. Use brief key prompts where needed, not written walkthroughs. Do not reveal optional routes by default or show the complete answer before the player has tried.
- Give the lantern an early, safe demonstration before the west market scaffold, where it is first required. Show the lantern in hand, placing it in A, the warm patch that retains A inside B, and how to retrieve it. The existing town build already supports placement in A and a patch of A in B; the problem reported in play is that the feature is not apparent. Validate any hint route with the real character and camera, not only the reachability solver.
- Use sound as a second cue: harbour activity and gulls in A; wind, dripping and the far sea in B. Distinguish switch, refusal and lantern placement by sound once audio is added. Warm guidance colours in B must not be confused with the lantern patch or a causal trace.

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
| Push and crate puzzles | the solver: solvable, fewest moves, fewest switches, not solvable in one moment |
| Every room | walked by the real body; the known wrong moves tried |
| Skips | the search of `tests/test_reach.cpp`: no jump ends higher in a room not joined by walking |
| Recovery | every place of return is a place to stand, out of the water, in both moments |
| The ground | under every floor; every walk or jump off an edge ends in the map or is caught as a fall |
| Immersion | every view down meets the map, the land or the sea |

## 12. Order of work and acceptance

1. Prototype third-person movement, sprint, ledge climbing and a visible lantern in one compact section. Tune against a tight alley, a raised ledge and a switch in the air before changing the whole town.
2. Rework the harbour through the first lantern lesson as a reference district. Validate the art on the quay-to-sea-gate route before extending it (section 6.8). Give first-time players a clear destination, one worthwhile detour and a visible reward for changing the past. Ask them to play without verbal coaching; note where they lose the route, cannot read an interaction or expect an unreachable platform to be playable.
3. Prototype the west loop next, then repeat the proven approach in the east and north. In an uncoached west test, check whether players look down from the bridgehead, recognise it again from the upper station and find the lift without taking the optional gallery. Test locked and released gallery states with sprint, mantle and aerial switches. The landing, lift and plaza must stay in order; a viewpoint alone may be reached early. Re-test safe recovery and optional-lamp independence. The old route solver is a baseline, not proof for the new controller.
4. Build the bell tower finale and boat payoff after the movement and exploration loop is stable. Verify the route to the final bell and all ending states, including a run with no optional lamps.

The uploaded town already loads `data/town.dlt`, supports the lantern patch and causal objects, and has a solver for the old route. Keep those results as a baseline only. Before updating the layout broadly, review the first district with a new player: can the player find the route without coaching, explain why a trace appears in B and use the lantern unprompted at the first required obstacle? Test safe camera behaviour on every switch, a refused climb with inadequate clearance, a refused switch into solids or water, and recovery without lantern-dependent support. Test zero, some and all optional lamps; the same main route and ending views must remain available.

## 13. Decisions

- **2026-11-24:** one complete town, A before B, past-to-present cause and effect, one held patch from a placed lantern, no mandatory district-spanning errands and no post-clear shortcut back to the start.
- **2026-11-25:** current town starts in B; the harbour teaches the switch and the one-way causal rule. The lantern receives a safe early demonstration in the next version. The town has a first third-person player pass. A denser town, fallback guidance and optional street lamps remain planned. Use the central warning bell and boats for the finale. See [Changelog](../CHANGELOG.md) for prototype history.

- **2026-11-25, exploration pass:** linear harbour onboarding, a readable main spine and two optional local loops. Review the west bridge reversal first; leave the east loop and northern climb as follow-up prototypes.

## 14. Open

- Tune the camera, sprint speed, ledge height and jump envelope on the reference route.
- Prove the west gallery alignment and lower latch in collision, then finalise playable roofs, interiors and the north-to-tower approach; verify the final bell with actual input, not just route reachability.
- Set optional lamp locations, boat mapping and escape counts; prototype the Past, Present and Both views before choosing their controls.
- Tune guidance thresholds and cue contrast. Holding Tab to preview a layer remains an optional idea, not required for the finale. Audio and further environmental detail can follow after the route works.
