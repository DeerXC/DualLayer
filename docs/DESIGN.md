# Design

The next version of DualLayer. Unless identified as a current-build baseline, this document describes the target design, not shipped features. The uploaded baseline is documented in [Architecture](ARCHITECTURE.md) and [Build](BUILD.md).

## 1. The game

A third-person 3D exploration puzzle without combat. One walled harbour town on a hill, at two moments:

- **A, the past**: a warm evening, the town whole and empty, a minute before something ended;
- **B, the present**: the same town much later, in ruins under a cold moon after the flood.

The body carries a lantern. **Tab** moves the whole world between the two moments. The lantern, **set down in the past**,
keeps a circle of the past standing in the present. What is done in the past leaves its trace in the present. The way
through the town is found by reading the present and changing the past.

The game asks one thing, and the player sees it from the first frame: three dark lamps high over the bell tower, on a signal mast.
After the harbour the town opens into three districts. Ring the bell of each, in any order, and the tower opens.

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

- **The three bells and the tower:** each district has one bell, rung where it hangs. Ringing it lights that district's lamp on the signal mast over the bell tower, which can be seen from every district, and changes the town in a way that is seen and never needed elsewhere (the water of A rises, a machine turns, night falls in A: one each, to be set). The tower opens when the three emblems are lit. The three lamps are the only progress mark: large and wordless (section 10.5). The final bell is reachable without lighting any optional street lamp.
- **The links to the rock:** each district ends in a link to the plaza of the rock, opened by that district's last puzzle (section 7.4). A link is a connection, never a requirement: every district is complete from the lower town alone. The player takes the link of the district finished last, so the finale does not start with a long walk back.
- **Anchor lamps:** checkpoints, not collectibles and not optional street lamps. Place them after meaningful progress. A fall or R returns to a safe floor in both moments without depending on the placed lantern. In the uploaded town baseline, this is the last safe standing point; anchor-based recovery and lamp travel belonged to the earlier greybox prototype (git tag `m2-greybox`) and must be rebuilt if wanted.
- **Optional street lamps:** light a lamp in A and it remains lit in B. A lamp can clarify a nearby route or reveal a landmark. Reward exploration with changes visible in the town and with the boat outcome in section 5, not a collection counter. Memory fragments, fragment-based radius upgrades and unlockable story scenes are not part of the new route.

## 5. Bell tower finale and ending views

### 5.1 Final approach (layout to prototype)

The three links meet on the plaza of the rock; the player arrives by the link of the district finished last, and the tower opens when its three emblems are lit. From the plaza, frame the bell tower, a recognisable entrance and the first walkable segment together. Keep the approach local. Prototype a broken approach in B that reveals the route, a safe position to switch to A, and a short climb through the intact tower. Use the established rules for any lantern-held floor or ledge; a new timing mechanic, long backtrack or precise aerial switch is not required. The bell platform must be reachable through an intended path, not by standing on a wall. Leave enough room to read the bell, the interaction and the harbour below with the third-person camera. The layout and whether the final climb requires a lantern placement remain play-test decisions.

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

One sun or moon with soft sky and ground fill, the lamps of the town, and what is added of both to the picture. The numbers are in `[lighting]` of the config ([BUILD.md](BUILD.md)); this is the rule and the reason for each.

- **The sun of A and the moon of B cast shadows.** One shadow map of 2048 x 2048 texels covers 60 m round a point a little ahead of the camera; the box follows the camera on the grid of the texels, so a shadow never crawls. The town, the traveller, the crates and the things that change cast a shadow; water, glow and halos do not. A shadow takes 85% of the direct light and none of the sky light, so a shadow is never black and the detail in it stays. A lamp casts no shadow: its light is a soft pool, not a cut-out.
- **Every lit lantern of the model is a light.** The lamps of the streets, the plaza, the harbour and the lighthouse, the ring of lights on the tower and the lantern on the grave light the surfaces round them in the moments in which they are lit: a warm pool 14 m across for a street lamp. In A the sun is still up, so the lamps are weaker (0.55); in B they are the only light but the moon, and the lit ones along the main path read as a trail of pools (the model keeps every second lamp of the path lit). The renderer lights a surface with the 32 nearest and strongest lamps of the moment, and a lamp fades out with distance, so that none blinks on or off as the camera walks. A flame is never still: a lamp flickers by 3 to 10%, with a phase of its own.
- **What the player did is a light too.** The lantern of the traveller, in the hand or set down, lights its surroundings (13 m) in both moments with the colour of its flame. A lamp of the signal mast, lit by a bell, lights the tower under it. The lamp over the rope of a bell that was rung lights its street in B. The world answers with light.
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

The first rendering pass implements the gradient sky, ground fill and distance and low mist, and since 2026-11-26 the shadows, the lamps, the soft shoulder and the style of the light (6.3). Surface materials, water refinements and the paired Windows play-test remain open. Do not rebuild the city or add post-processing systems before the reference route proves the visual direction.

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
| West: the market and the well | 0 .. 6, the bastion 14 | goes down: the great well to the cistern; the scaffold of the lantern; the day bell; ends at the bastion with the rock in view |
| Centre: the Rock of the Bell | 14 | a round plaza on a rock in a gorge 20 m deep; the church and the bell tower; reached by the link of each district (the lift from the gorge in the west, the east bridge, the cliff way in the north); the finale |
| East: the locks | 3 .. 21 | climbs: five locks to the water tower; the dawn bell; entered by a stair from the lower town (to model: now it is entered from the rock by the east stair) |
| North: the upper street and the theatre | 21 .. 30 | high: the street ends at the top row of the theatre, which faces the rock; entered by its own stair from the lower town (to model: now it starts at the top lock) |
| The cliff: the graveyard | 42 | the view over everything; the night bell; the cliff way to the rock (to design) |

### 7.3 Rules

- **A hub, three ways and a rock.** The lower town is the hub: three stairs part there, one to each district, each marked by its glyph (section 10.5). Each way climbs, drops or crosses in its own manner and ends at its bell with the tower in view. The ways are joined by the rock, not by each other: a way's last puzzle opens its link to the plaza.
- **Equal ceilings.** The three ways have the same number of beats (three, then a bell) and reach the same difficulty at their last beat, because any one of them may be a player's first or last.
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

After the harbour the town opens: three ways, then the rock.

| Stage | Where | What the player does | Ends with | State |
|---|---|---|---|---|
| 1. Teaching ground | the quay, the sea gate, the lower town | the switch, the causal rule, the switch in the air (section 8); the lantern's safe demonstration | the three gates of the lower town open; the tower with its three dark emblems stays in view | built, except the lantern's demonstration |
| 2. West way | up the market stairs, the market, the scaffold, the bastion | the lantern relay; the day bell | the bastion, with the rock framed across the gorge; its link to the rock: the wreck of the west bridge (B), the gorge, the lift | market, scaffold, day bell, wreck and lift built; the return to the lower town is not modelled |
| 3. East way | a stair from the lower town to the first lock, up the locks to the water tower | water and the crate; the plank between two roofs; the dawn bell | the water tower; its link: the east stair and bridge to the rock | the locks and the plank exist; the stair from the lower town is not modelled (the locks are entered from the rock today) |
| 4. North way | a stair from the lower town to the upper street, the theatre, the cliff stair | the aerial switch and the prism; the night bell | the graveyard on the cliff; its link: the cliff way to the rock (to design) | gatehouse crate and night bell exist; the stair from the lower town is not modelled (the street starts at the top lock today) |
| 5. The rock | the link of the district finished last, the plaza | the tower opens when the three emblems are lit; the climb inside; the warning bell | the ending views (section 5) | lift and plaza built; the inside of the tower to design |

Rules of the order:

- The three ways open together, when the teaching ground is done, and are taken in any order. Each is complete from the lower town alone: no puzzle of a way needs anything of another, and nothing done in one changes what another asks.
- The return from a way's bell to the lower town is the one walk back this design accepts. It is not a puzzle, it sends the player to no other district, and it should take about 20 s at most and show a new view or open a shortcut when the bell is rung (a rope, a chute, a lift).
- A link is a connection, not a key. A link opened early may let a player enter another way from the plaza, never past a puzzle of it; a viewpoint alone may be reached early.
- The day bell hangs in the market today, before the scaffold. Whether each bell moves to the end of its way is a play-test question (section 14).

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
| Teaching ground: harbour and sea gate | Keep the teaching sequence. After the gate, frame the bell tower with its three dark emblems and the three stairs of the lower town together. | A freight-yard back door leads to a loading deck and returns by a side stair. It never bypasses the sea gate lesson. |
| West market, gorge and rock | Teach the lantern before the west bridgehead. In A the intact bridge ends at a closed gate; in B the fallen spans become the descent. Use the lift winch in A, then climb the surviving frame in B. | A grate at the west rim previews a maintenance gallery below the market. The lower latch is first reachable from the landing after the wreck descent. The gallery returns to the west rim and then rejoins the same landing; using it is optional. |
| East and north ways: roofs and theatre | Keep the plank lesson; only then split the route into street and roof paths. Both meet at the theatre backstage. | The street shows intact arcades; the roof previews the backstage and northern gatehouse. A side stair closes the local loop without a required return or pickup. |
| The rock and the tower | Frame the tower entrance, first walkable segment and harbour from the plaza, which the player reaches by the link of the last district finished. B exposes the approach; switch on safe ground into A and climb inside. | The warning bell and boats remain the payoff. Optional lamps can affect the visible boat outcome but cannot gate the tower, bell or any ending view. |

#### West loop review and implementation boundary

Keep the existing bridgehead, gorge and lift stations. Review the same west approach in A and B, then inspect the lift-house roof and the view back from the upper station. The first reveal is a change in topology: the intact bridge is not the solution; the wreck becomes the descent. From the upper station, the player should recognise the bridgehead and understand where the market gallery sits relative to the street. Preserve a continuous, safe forward route from the landing to the lift, independent of the gallery.

The gallery in the review is a proposed centerline, not geometry. First test an exposed retaining-wall gallery or a switchback before committing to a tunnel. Its gate is a local B-side latch with no effect in A. The present town's A-only causal objects do not implement it. Begin with a closed passage; wire a local latch only when needed, without changing the causal interaction contract or the lantern rules. Keep the gallery and its upper exit downstream of the lantern lesson; do not expose the lift or the plaza before their intended progression.

Place `west_loop_preview.py` beside `tools/layout/city_model.py` and run `python tools/layout/west_loop_preview.py` from the repository root with NumPy, Matplotlib and Pillow. It runs the existing street checks and writes `build/west_loop_preview.png`. Gold is the route intent; dashed teal is the gallery proposal. The sheet is rendered from the layout model, not the Windows game. Its schematic is not a collision, sight-line or clearance test.


## 8. Puzzles

Every way is built from the same pieces. This section says how a room is made, which templates a room may use, and what each way is built from. It is the baseline on the uploaded model, not the final layout of the compact town.

### 8.1 How a room is made

1. **Three beats.** Each way has three puzzle rooms in this order: **teach** (the rule is shown where nothing can go wrong, and the answer is in view), **twist** (the same rule in a less obvious form), **test** (the rule combined with one idea taught before, in a larger space). The bell closes the way. The teaching ground teaches the rules; no way teaches a rule that another way needs.
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

| Way | Idea | Teach, twist, test, bell | State |
|---|---|---|---|
| Teaching ground: harbour and lower town | walking; the switch; the causal rule; the lantern | **The switch:** the game starts in B on the quay, a heap of wreck closes the way west in B only. **The causal rule:** the portcullis of the sea gate is shut in both moments; its gold winch raises it in A, where the cargo of the customs fills the passage behind it; in B the cargo is gone and the portcullis is raised where A left it, with its trace. **The switch in the air:** the lower street is closed in A by the barricade of the customs (5 m) and in B by the front of a fallen house (3.4 m over the street); two crates of A climb to 2.4 m; from the top one a jump with a switch at its top lands on the heap of B, whose far side steps down to the street. **The canal of the customs:** a crate pushed in A floats to the far quay and lies on the dry bed in B (it replaces the crate puzzle of the customs-house cloister; proved by the solver, at most two switches). **The lantern:** a short safe stretch where the lantern is shown in hand, set down in A, seen as a warm patch in B and taken up again, before the three gates open (section 10.6) | built (2026-11-25, `tests/test_things.cpp`); the lantern's demonstration planned |
| West: market and well | the lantern relay | **Teach:** the scaffold of A under a shed roof by the terrace of the market; the lantern on it, climbed in B. **Twist:** the great well: the wooden stair of A kept standing by the lantern in the dry shaft of B. **Test:** two gaps in a row with one lantern, ending at the bastion. **Bell:** the day bell | scaffold placed (2026-11-25); the well a sketch; the relay and the position of the bell to design |
| East: the locks | water and the crate | **Teach:** a crate floated in A to where it is needed lies there on the dry bed in B. **Twist:** the plank laid between two roofs in A holds in B. **Test:** the tower lock made the same way. **Bell:** the dawn bell in the water tower | plank placed (2026-11-25); the rest to build |
| North: theatre and graveyard | the aerial switch and the prism | **Teach:** the gatehouse of the theatre: a crate pushed under its balcony in A, then a switch in the air onto the heap of B. **Twist:** the prism moves a thing of A into B. **Test:** the fallen back wall of the theatre passed with the lantern and a switch together, not with the switch alone. **Bell:** the night bell on the graveyard; the traces of the whole game are seen from the cliff | gatehouse placed (2026-11-25); the rest to build or design |
| The rock: link of the west way | the causal rule at a large scale | the wreck of the west bridge down into the gorge (B); the lift lowered in A and climbed in B (7.4); the gate of the west bridge stays shut (seen, not crossed) | built (2026-11-25); the tower itself to design |

## 9. Environmental storytelling

The flood premise is in section 5. Main-route play does not depend on knowing the backstory. Keep story beats in places the player already explores rather than in collectible-triggered scenes.

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
| Far | Where does this end? | the three lamps of the signal mast over the bell tower, in sight from the first frame, dark until the bell of their district is rung (a rung bell lights one, in sight of the whole town) | built (2026-11-26) |
| Way | What is in this district? | the three gates of the lower town, each with its glyph; on entering a way, a beacon (10.2) | planned |
| Room | How do I get past this? | the room itself: the end in view, the ruin showing what is missing, A and B readable at the obstacle (3.4, 8.1) | built in part |
| Help | I am stuck | the flame of the lantern and H (10.3), the map (10.4), the lens (3.8) | the flame, H and the map are built; the lens is planned |

### 10.2 Beacons

A beacon is a place the player can see and cannot yet reach, at the start of a way and of a room: a warm window high in the bastion tower, a fall of water at the top lock, a stage lit by one lamp. It is the question; the way to it is the puzzle. It is part of the town, seen from the street, and not a marker. The map shows a beacon the player has seen as a ring that fills when it is reached. First candidates: the bastion tower (west), the top lock (east), the theatre stage (north).

**Why a mast.** The bell tower itself cannot be the far goal from the quay: the sea wall, 8 m in front of the start, hides it in both moments (`tests/test_firstminute.cpp` casts the rays from the camera of the first frame; the tower's south face is blocked in A and in B, and only the top 5 m of the spire of A clears the wall, while the game starts in B). So the three lamps hang on a signal mast that stands over the tower and clears the wall: three round lamps on a yard at 61 m, the mast coming out of the dome of the church and, in B, rising free over the broken shaft. The player sees the lamps from the quay, the sea stair and the sea gate; from the streets of the lower town the houses hide them, and the flame and the map take over.

### 10.3 The lantern as compass

The compass is the lantern, so help is an object of the world and not a layer on the screen. Built (2026-11-26): `scene/Guidance.h`, `scene/Flame.h`.

- **The flame leans (always on).** The flame over the lantern, in the hand and set down, leans 18 degrees toward the goal: the first step of the main path that is not done, and the signal mast when all are done. (The first design said about 10 degrees; that was too small to see at the distance of the camera, so it is a setting, `[guidance] lean_degrees`.) It stands upright within 2 m of the goal.
- **It grows with need.** After 30 s without progress (a step done, a new 6 m cell of ground, or a new closest approach by 4 m) the lean grows to 40 degrees and the flame throws sparks.
- **H, first press:** the lantern is raised (the arm goes up) and a warm beam sweeps over the place for 6 s: it swings wide and settles on the bearing of the goal. It shows the place, not the act.
- **H, second press** during the sweep, and only after 90 s without progress, so the player has tried first: a frame round the thing to use (the winch, the crate, the plank), joined to the lantern by a line of motes, for 12 s. Before the 90 s a second press only puts the sweep away. A press while the frame shows puts it away.
- **Not built:** the third press (the pulse at a suggested switch point, an arc for an aerial switch); the flame standing upright in the lower town, where the three gates are the choice (the main route is one line, not three ways yet: section 7.4).
- **Rules.** The player escalates by pressing and by time without progress, never by distance to an answer. The flame never points at an optional room (there are none yet). With the lantern set down its flame leans where it stands. Warm guidance in B must not be confused with the lantern patch or a causal trace: gold means use, the lantern's colour is the flame.

### 10.4 The map: two sheets of tracing paper

Built (2026-11-26): `scene/MapMemory.h`, `render/MapView.h`, the plan in the town file.

- **M opens it** over the frozen game: time stands still, the clock of the guidance too. Two sheets of tracing paper, the plan of A and the plan of B, laid one over the other. What stands in both moments is drawn once, in a neutral ink; what stands in one only is warm (A) or cool (B); where both have something and it differs, both are drawn and the sheet on top shows over the other. The player reads the rule from the map: a moat drawn warm only is water of the past. Tab turns the other sheet to the top (it does not switch the world); the frame and two chips in a corner say which sheet is on top.
- **The plan** is made by the model: cells of 1 m, one class per cell and moment: water, cliff (a slope of more than 41 degrees), building, street (a floor to walk). A street is drawn over an arch.
- **Only what has been seen is drawn:** the cells within 15 m of the body, and, from an overlook of the model (the two bastions, the plaza of the rock, the top row of the theatre, the graveyard), the town as far as 40 to 80 m. The rest is blank paper. The tower with its three lamps is always drawn: it is the goal.
- **What it holds:** the player (an arrow that points where the camera looks), the lantern when it is set down, the tower with its lit and dark lamps, the markers.
- **Markers:** E with the pointer on the paper places one; E on a marker takes it away. At most five; a sixth takes the oldest place. Each stands in the world as a thin column of light, in both moments. No text, no types.
- **The window** shows what is seen, with a margin, and always the player and the tower; at least 90 m across; north is up. The pointer is free while it is open; M or Esc closes it. The prompt M MAP is offered, dimmed, after 40 s of walking, to a player who has not opened it.
- **What it never holds:** a corner minimap, a task list, the position of an unseen thing, or what to do. The only words are the keys at its foot.
- **Not built:** the beacons seen as rings, the lit street lamps and the glyphs of the ways on their gates (none of them exist yet).

### 10.5 The glyph board

This is the quest system without words. Each way has a glyph, simple enough to read at 64 px in both colours. Built (2026-11-26) as three lamps on the signal mast: a sphere for the well (west, the day bell), a column for the cliff (north, the night bell) and a box for the lock (east, the dawn bell); each is dark until its bell is rung, then warm and glowing, in both moments, and the lit mast is seen from the whole town. The three lamps are the whole quest log: no list, no counter, no marker that names a task. Optional street lamps are not on it. The same three lamps are on the map. Not built: the glyphs carved on the gates of the ways, on the bells and on the map (there are no gates yet: section 7.4).

### 10.6 The lantern's demonstration and the first minute

Give the lantern an early, safe demonstration in the lower town, before the three gates open and before the west scaffold, where it is first required. Show the lantern in hand, placing it in A, the warm patch that retains A inside B, and how to retrieve it. The existing town build already supports placement in A and a patch of A in B; the problem reported in play is that the feature is not apparent. Validate any hint route with the real character and camera, not only the reachability solver. (Not built: KI-14.)

**The first minute, as built (2026-11-26).** The game starts in B on the quay, and the first frame is set by the town file (spawn yaw -0.55, pitch +0.29): the heap of wreck to the left, the winch of the sea gate left of the middle, and the three dark lamps of the mast high in the sky right of the middle. `tests/test_firstminute.cpp` keeps it so for the lenses of 60, 70 and 90 degrees and the screens 16:9, 16:10 and 4:3 (the lamps are at 0.88 of the half height in the narrowest case), and proves with rays that nothing of the town is in the way of any lamp in either moment. The flame of the lantern in the hand leans toward the winch, which is in the picture. After 8 s of walking, to a player who has not switched, the prompt says TAB SWITCH WORLD; after 40 s, M MAP. The first minute is tested uncoached (section 12): the player sees the lamps, goes to the sea gate without being told, and can say what the game asks.

### 10.7 What replaced the fallback

The fallback of 2026-11-26 (a pillar of light over the next step, shown in the first 20 s, on H and after 30 s without progress, with lights running toward it) is gone: a player who never pressed H had no goal, and the pillar was the answer, not a question. What was kept: the measure of progress (a step done, a new 6 m cell, a new closest approach by 4 m), now the clock of the flame; the prompt TAB SWITCH WORLD; and, in the second press of H, a faint frame round the thing to use. The first step of the main path is still the goal of the flame; `steps` becomes `goals` with a way and what each needs when the three ways are modelled (section 7.4, 12).

### 10.8 Sound

Use sound as a second cue: harbour activity and gulls in A; wind, dripping and the far sea in B. Distinguish switch, refusal and lantern placement by sound once audio is added. A bell rung lights its emblem and sounds across the town.

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
| The three ways | `tests/test_route.cpp` (planned): from the lower town with the teaching ground done, each way is solved on its own; its goals are unreachable before their prerequisites; its reachable set is the same with the other two ways done or not; a link opened early reaches no unsolved puzzle of another way; the rock opens only at three bells; in every order of the three ways the tower is reached |
| Guidance | `tests/test_guidance.cpp`: the goal; the flame (it leans toward the goal, restless after 30 s, calm after progress, upright at the goal, the same on the lantern set down and on the avatar); the presses of H and their times (the sweep, the frame only after 90 s, away when a step is done); what is drawn; planned: the flame upright in the lower town, nothing pointing at an optional room |
| The lens | `tests/test_lens.cpp` (planned): the solid does not change while it is open; tap and hold on the ground; the switch at once in the air; it closes when the body leaves the ground |
| The map | `tests/test_map.cpp`: the plan of the model (both moments, the moat in A only, the streets under their middle lines); what is seen (the radius, the overlooks, never more); the markers (five at most); the paper (inside every screen, the player and the tower on it, the window); the ink (neutral, warm, cool, the sheet on top, blank paper, the lamps, the arrow, the flags); the whole town on the map fits the batch |
| The first minute | `tests/test_firstminute.cpp`: rays from the camera of the first frame to every lamp in both moments; the sea wall does hide the tower itself; the lamps, the winch and the heap inside the picture for every lens and screen; three lamps, dark at the start, each lit by its own bell; the compass points into the picture; the heap closes the quay in B only |
| The light | `tests/test_lighting.cpp`: the maths of the shader run as C++ (the fall-off of a lamp, one lamp on a surface, the soft shoulder, the fade at the edge of the shadow map); the choice of at most 32 lamps (the moment, the cap, a far lamp fades out, no tie blinks, the flicker); the shadow box (what is inside it is inside the map, depth runs from the light, it moves on the texel grid, a light overhead); the settings; the lamps of the town file, the lantern, the lamps lit by the bells and the halos |
| The style of the light | `tests/test_style.cpp`: the functions of the style run as C++ (the rings and the warmth of a lamp, the bands of the sun, the clouds: their range, their share of the sky, the drift, the same value along a ray of light; the rim; the cool fill only in the shadow; the painted stone and the damp; the haze; the grade: identity when neutral, the middle grey kept, no negative colour, the corners), the numbers that feed them (the defaults, nonsense pulled in, the config keys, the five registers of `MakeStyle`: plain is all zeros, the shafts need the shadows, A and B differ) and the looks (the cool fill and the tones keep the brightness) |

## 12. Order of work and acceptance

1. Third-person movement, sprint, ledge climbing and a visible lantern: a first pass is built. Tune against a tight alley, a raised ledge and a switch in the air before changing the whole town.
2. **The teaching ground as the reference district, with the new guidance.** Built (2026-11-26): the signal mast with its three dark lamps in view from the quay, the flame that leans, H, the map, the first frame. Not built: the lantern's safe demonstration; the three gates with their glyphs; a beacon at the start of the first way; the lens. Validate the art on the quay-to-sea-gate route before extending it (section 6.8). Give first-time players a clear destination, one worthwhile detour and a visible reward for changing the past. Ask them to play without verbal coaching and measure: within 60 s can they say what the game asks; do they take a stair unprompted; how often do they press H; how many times do 30 s pass without progress; where do they lose the route, cannot read an interaction or expect an unreachable platform to be playable. The light is built (2026-11-26: the shadows, the lamps, the shoulder, the style; section 6.3) and not yet seen in the game: play E-21 to E-25 of [BUILD.md](BUILD.md) first.
3. **Make the order free.** Model the stair from the lower town to the east locks and to the upper street, and the links to the plaza; replace `steps` by `goals` with their ways; extend the solver to the tests of section 11; keep the old line as a baseline until they pass.
4. **The west way first** (it holds the lantern relay and the most built content), then the east and the north, each with three beats and a bell, each tested uncoached. In an uncoached west test, check whether players look down from the bridgehead, recognise it again from the upper station and find the lift without taking the optional gallery. Test locked and released gallery states with sprint, mantle and aerial switches. A viewpoint alone may be reached early. Re-test safe recovery and optional-lamp independence. The old route solver is a baseline, not proof for the new controller.
5. Build the bell tower finale and boat payoff after the movement and exploration loop is stable. Verify the route to the final bell from the link of each district, in each of the six orders, and all ending states, including a run with no optional lamps.

The uploaded town already loads `data/town.dlt`, supports the lantern patch and causal objects, and has a solver for the old route. Keep those results as a baseline only. Before updating the layout broadly, review the first district with a new player: can the player find the route without coaching, explain why a trace appears in B and use the lantern unprompted at the first required obstacle? Test safe camera behaviour on every switch, a refused climb with inadequate clearance, a refused switch into solids or water, and recovery without lantern-dependent support. Test zero, some and all optional lamps; the same main route and ending views must remain available.

## 13. Decisions

- **2026-11-24:** one complete town, A before B, past-to-present cause and effect, one held patch from a placed lantern, no mandatory district-spanning errands and no post-clear shortcut back to the start.
- **2026-11-25:** current town starts in B; the harbour teaches the switch and the one-way causal rule. The lantern receives a safe early demonstration in the next version. The town has a first third-person player pass. A denser town, fallback guidance and optional street lamps remain planned. Use the central warning bell and boats for the finale. See [Changelog](../CHANGELOG.md) for prototype history.
- **2026-11-25, exploration pass:** linear harbour onboarding, a readable main spine and two optional local loops. Review the west bridge reversal first; leave the east loop and northern climb as follow-up prototypes.
- **2026-11-26, guidance and order:** the main route is no longer one line. The harbour and the lower town are the teaching ground; after it the town opens into three ways (west, east, north), done in any order, each of three beats and a bell, and joined by the rock, not by each other. The tower opens when its three emblems are lit; its progress mark is the glyph board and nothing else. Guidance is layered: a goal in sight, a beacon at each way, the lantern's flame as a compass (always on, growing with need), H for more, a map of two sheets of tracing paper with up to five markers, and a lens on holding Tab. Not adopted: a corner minimap, a task list, a compass bar, narration. The pillar of 2026-11-26 is a fallback only.
- **2026-11-26, the first minute and the map:** the bell tower cannot be the far goal (the sea wall hides it from the quay in both moments, measured), so the three emblems hang on a signal mast over it, in sight from the first frame. The guidance of the fallback (the pillar) is replaced by the flame of the lantern, H in two steps and the map on two sheets of tracing paper. The lens, the beacons, the lantern's demonstration and the three ways are still to build.
- **2026-11-26, the light:** the sun and the moon cast shadows; every lit lantern of the model is a light; the lantern of the traveller and the lamps that the bells light are lights of what the player did; the sum goes through a soft shoulder instead of being cut off, and a halo puts a lamp in the air. No lamp is ever needed to read the route (6.3). Lamps cast no shadow; there is no picture in high range and no bloom yet.
- **2026-11-26, the style of the light:** the light is hand-painted, not photographic: soft bands of sun, a cool fill in the shadows, drifting shadows of clouds, a rim, lamps in rings, a painted stone that is damp near the water, rays of light through the shadow map, a haze with a hue and a grade of the picture (6.3). One master and a plain view (the key L) keep it comparable, and no rule of the game is carried by it. It had been "not built" (shafts) and "no post-processing" (6.8): the grade is a step of the model shader, not a pass, so the second still holds.

## 14. Open

- Tune the camera, sprint speed, ledge height and jump envelope on the reference route.
- Model the three ways: the stairs from the lower town to the east locks and to the upper street, the link of each district to the plaza (the cliff way in the north is not designed), and the return from each bell to the lower town (its limit is in 7.4).
- Decide whether each bell moves to the end of its way (the day bell hangs in the market, before the scaffold, today) and what each bell changes in the town.
- Prove the west gallery alignment and lower latch in collision, then finalise playable roofs, interiors and the approach from the plaza into the tower; verify the final bell with actual input, not just route reachability.
- Set optional lamp locations, boat mapping and escape counts; prototype the Past, Present and Both views before choosing their controls.
- Tune the guidance and the map on play: the lean of the flame (18 and 40 degrees), the 30 s and 90 s times, the size and glow of the three lamps against the sky of B and of A, the first frame (yaw and pitch of the town file), the colours of the map, the reveal radius (15 m). If the tap on the ground feels late, move the lens to the right mouse button (3.8). Draw the three glyphs for the gates. Audio and further environmental detail can follow after the route works.
- Tune the light on play (E-21 to E-24): the lamps and the lantern against the moon of B, the weight of the shadows (0.85), the moon (0.8) and the exposure; whether the lit lamps of B read as a trail; whether the shadow box (60 m, 2048 texels) is sharp enough at the quay and soft enough on the far town; the cost on the laptop (the shadow pass draws the town a second time).
