# Level Design

The design of the map: what each part teaches, how it is proved, and in which order it is built. The code points here by
section number.

## 0. Summary

One walled city in two layers round a hub. Three wings each teach one new object; at the far end of a wing wait a seal and a
one-way shortcut back to the hub. The three seals open the tower in the middle of the hub; the tower combines the three
objects, and its top is the goal.

| Item | First stage | This design |
|---|---|---|
| Shape | one line north, each part used once | hub, three wings in any order, tower; every shortcut returns to the hub |
| Difficulty | aiming jumps (the hands) | reading the two layers and planning (the head); main-path jumps are generous |
| Rule | the layer switch | still only the layer switch; new objects react to it |
| Length | about 1 to 2 minutes | main path about 10 to 12 minutes, everything about 20 |
| Hidden lights | 3 | 12, each a second use of something the map teaches |

## 1. Why the first stage was too simple

1. One line: no branch, no return, so no "back where I started, from a new side".
2. Each mechanic used once, never combined, never turned round.
3. The difficulty was in the hands (jump distance, the moment of a switch in the air), not in the head; the tower could not
   be entered by the intended path.
4. No reason to come back. A shortcut means nothing on a line; round a hub that is passed again and again, every shortcut
   makes the next trip shorter.

## 2. Goals and hard limits

| Goal | How it is judged |
|---|---|
| One rule | the body has one ability, the layer switch; new objects react to it, no new key |
| Hard in the head, not the hands | every main-path jump within the limits of section 10; precise jumps only for optional lights |
| One "of course!" per room | each room names its moment in one sentence; a room without one is cut |
| Teach, then turn round | the first room of an object has no dead end; later rooms use it the other way |
| The wish to explore | the goal always in sight; a landmark per wing seen from the hub; 12 hidden lights |
| The wish to finish | three seal lamps on the tower door; shortcuts bring the hub nearer |
| Proved | block puzzles by the solver; every room walked by the real body; known ways round tried |

## 3. References

| Work | Borrowed | Where |
|---|---|---|
| Stormveil Castle (Elden Ring) | layered space, seen before reached, one-way doors and shortcuts, coming back to a known place | the hub; three shortcuts; the west wing under the terrace; the east wing on the courtyard wall; the view back from the tower |
| Shrines (Zelda: Breath of the Wild) | one mechanic per room, safe first, then combined; objects carried; room state changed | the orb (carried), the prism (room state), no dead end in the first room of a wing |
| Super Mario Odyssey | many uses of one mechanic, dense collectibles, some appear only after others | 12 lights; the hub light after all three shortcuts |
| Portal | one thing at a time, then turned round | the last room of each wing turns its object round; the tower combines them |

## 4. Objects

Each object answers one question: what happens to it when the body switches layers?

| Object | Rule | Taught | Turned round |
|---|---|---|---|
| Layer wall / floor | exists in one layer | prologue, the gate | north wing: the prism moves it to the other layer |
| Purple block | in both layers, so it obeys the walls of both | courtyard | east wing: with red and blue blocks |
| Plate and door | plate in one layer, door in the other; the door is open while the plate is pressed | courtyard | west wing: the orb holds the plate while the body is in the other layer |
| Orb | carried, it switches with the body; put down one step ahead, it stays in that layer; with no ground in its layer it falls straight down to the first surface of its layer; on a socket it works the socket; it presses a plate | west 1 | west 3: dropped through a floor of the other layer; tower: the prism takes its stand away |
| Single-layer block | a red block exists only in A and obeys only red walls; blue only in B | east 1 | east 2: pushed into a channel it fills it, in its own layer only |
| Channel | a 1 m deep trench; a block pushed in becomes floor of its layer; a red and a blue block fill it in both layers; the purple block crosses only a channel filled in both | east 2 | east 3: one channel filled twice so that the purple block passes |
| Prism switch | stepped on, the objects linked to it swap red and blue, and stay so | north 1 | north 2: one switch moves two things; tower: it drops the orb |
| One-way door | opens from one side only and stays open | west exit | the hub: all three open reveal a light |

Not used on purpose: moving platforms, throwing, timers. They would move the difficulty back to the hands.

## 5. Structure

| Area | Teaches | Main path | Height | Shortcut to the hub |
|---|---|---|---|---|
| Prologue: gate, gap, courtyard | the switch; the switch in the air; the purple block | 2 min | terrace down to the courtyard | - |
| Hub | the goal and the three ways | 0.5 min | ground | - |
| West wing, the archive | the orb | 3 min | down, under the terrace | a one-way door |
| East wing, the water garden | single-layer blocks, channels | 3 min | up 6 m, onto the courtyard wall | a drop onto the gallery of the hub |
| North wing, the bell tower | the prism switch | 3 min | up, 7 m | a ledge through a window of the hub wall, a drop |
| Tower | the three combined | 2 min | up to the balcony, 12 m | - |

The wings can be played in any order: each uses only the prologue and its own object.

## 6. Rooms

Each room states its purpose, layout, solution, its "of course!", failure and reset, and its light.

### 6.1 Prologue

- Gate: a red wall of A across the terrace; in B it is not there. A hidden light is buried in its top.
- Gap: a blue stone of B, 3 m deep and 1 m from the terrace (a jump onto it needs no aim); the red strip of A 1.5 m beyond
  it; a blue curtain of B at the end of the strip. The way: onto the stone in B, jump, switch to A in the air. Neither layer
  alone crosses (the strip is out of reach from the terrace; in B the curtain stops the flight over the strip). It is the
  one place where the switch in the air is taught.
- Courtyard: the proved puzzle (10 pushes). The block on the plate opens the door in the north wall into the hub. The tops
  of the puzzle walls (2.5 m) can be stood on from the strip; the walls round the courtyard (5 m) are out of reach from them.

### 6.2 Hub

A walled square, 10 m high, round the tower. The tower door carries three seal lamps. The openings of the three wings and
the far ends of their shortcuts are in its walls; the ends are closed and promise "this will open". A gallery runs along
its south-east corner, 2.4 m up: out of reach from the floor, it is reached only from the east wing, and turns the hub
from a place to pass through into a place seen from above.

### 6.3 West wing: the archive (the orb)

Theme: one body cannot be in both worlds; a thing can. The wing goes down. Built in step 2 (`scene/WestWing.h`).

| Room | Layout | Moment |
|---|---|---|
| W1 hand it over | the hall is cut by a trench 5 m wide (more than a running jump); an orb of A on the north floor, a socket of B by the trench | the orb and the socket have the colours of their worlds (red-gold, blue); in A the orb is not placed on the blue socket and TAB is offered; switched with the body, the orb turns blue and fits: it lays a bridge of B. "The thing in the hand went with the body" |
| W2 leave it | an orb and a plate of A on the south floor; a door of B at the foot of the ramp down, a red panel in the doorway in A | the orb on the plate holds the door open for B, but the orb is needed beyond the door; a lever just inside keeps the door open for good, then the orb is fetched |
| W3 let it fall | a mezzanine 3 m over the lower room with a hole of 0.6 m (the body is 0.7 m across), closed by a red lid in A; under it, behind bars, a socket of B | in A the orb lies on the lid; in B it falls through onto the socket, and the bars open for good |
| W4 the seal | the vault under the mezzanine; a ramp up from the lower room to the door in the hub wall | the seal opens that door: the closed door seen from the hub |

The orbs start in the hall; R there puts them back. The stairs down under the gap, with the view up at the stone, are
left for the polish of step 6.

### 6.4 East wing: the water garden (single-layer blocks, channels)

Theme: a block obeys only its own world. The wing goes up and ends on the top of the courtyard wall. Every push puzzle is
proved by the solver.

| Room | Moment |
|---|---|
| E1 a red block ignores blue walls | where the purple block is stopped, the red one passes; no dead end (checked) |
| E2 fill the channel | the same trench is a path in one world and a trench in the other; the wrong block in the wrong channel is a dead end, and one dead end hides a light |
| E3 three blocks | to let a block of both layers pass, both layers need a path first |
| E4 the wall top | the wall that held the courtyard shut is now the path; the seal; a drop onto the gallery, then into the hub |

### 6.5 North wing: the bell tower (the prism)

Theme: not changing worlds, changing colours. Ramps and stairs, no jump needed.

| Room | Moment |
|---|---|
| N1 change a colour | two half-bridges of different colours meet with no common floor; the prism makes them one colour |
| N2 two at once | one prism moves the bridge and a wall; the body switches round the wall |
| N3 the bell tower | three floors of prisms; the top window looks at the seal lamps on the tower |
| N4 seal and ledge | a ledge through a window of the hub wall, the way down into the hub |

### 6.6 Tower

The landmark of the map: 13 m high, gold pinnacles on its corners, the beacon over it. Its door opens with the three seals,
on the hub floor, with no jump. Inside, ramps wind round its walls to a balcony 12 m above the hub that looks back over the
whole map; the goal is there. The rooms T1 .. T3 combine the objects: orb and prism, block and prism, then orb, purple block
and one switch in the air over a net.

## 7. Views and turns

| Moment | What is seen | Reference |
|---|---|---|
| First step | the beacon over the tower | the goal seen first |
| End of the strip | the courtyard from above | seen before reached |
| Entering the hub | the tower, the openings of the wings, closed doors | seen before reached |
| West wing, going down | the vault and its bars from the mezzanine; later the underside of the terrace (step 6) | layered space |
| West exit | the closed door of the hub, from the other side | one-way door |
| East exit | the top of the wall that closed the courtyard, then the hub from its gallery | a known place from a new side |
| Bell tower window | the seal lamps already lit | progress in sight |
| Tower balcony | the whole map | the end |

## 8. Hidden lights

Twelve, each a second use of something taught or a view back: in the gate; the purple block as a step; the orb on another
socket; the orb dropped elsewhere; under the gap; a dead end of E2; the wall top; the outside of the bell tower; a precise
jump at the bell tower; the orb dropped by the prism in another order; the hub after all three shortcuts; the tower
parapet. With all twelve the goal light changes colour.

## 9. Guidance without words

- Outlines: a single-layer object leaves an outline in the other layer; the same for orbs, single-layer blocks and the
  objects linked to a prism.
- Colours: red = A, blue = B, purple = both, gold = goals and seals, cyan-white = hidden lights; objects linked to one
  prism share one pattern.
- Readable failure: a refused push outlines its blocker in both layers; a refused switch turns the ripple back; a falling
  orb leaves a trail.
- Room reset: R puts the body at the entry of the room it is in and puts back the objects of that room. Lights, seals and
  shortcuts stay.
- Landmarks: one per wing, seen over the hub walls: the archive (a wall of shelves with a gold crown, west), the water
  ring (a turning ring, east), the bell tower (north). They stand outside the map and are drawn only.

## 10. Jump limits

The body walks at 5.5 m/s and jumps 2 m high; a running jump covers about 3.6 m.

| Item | Main path | Optional lights |
|---|---|---|
| Horizontal distance | 1.8 m | 2.5 m |
| Step up | 1.2 m | 1.8 m |
| Moments that work for a switch in the air | most of the tried moments | a few |
| Landing area | at least 2 x 2 m | at least 1 x 1 m |
| Cost of a miss | a net, or back to the room entry | back to the room entry |
| Switches in the air required | two: the gap, the tower | - |

Walls that must not be climbed are out of reach of a jump from every place a body can stand next to them: a surface that
must not be reached is at least 2.3 m over every floor in reach of it, and a parapet round a high floor is 2.6 m. The
search for skips checks this for the whole map.

## 11. Proof

| Target | Method |
|---|---|
| Push rooms | the solver: solvable, fewest pushes, not solvable in one layer, the naive solution from A blocked, dead ends (none in a teaching room); extended to several blocks, single-layer blocks, channels and prism states |
| Orb rooms | a walk with the real body and the orb; the known wrong moves tried |
| Prism rooms | all prism states with the layer of the body searched |
| The map | a walk from the start to the top; every room entry standable in both layers; the jumps that would skip a part tried |
| Skips | a search: from every place to stand that something higher is in reach of, jumps in 16 directions, with and without a run-up and a switch at the top; a jump that ends higher in a room not joined by walking fails the test (`tests/test_reach.cpp`) |

## 12. Build order

Every step ends with a playable build and a green `tools/check.ps1`.

| Step | Content | State |
|---|---|---|
| 1 skeleton | prologue, hub, the shapes of the wings and the tower, rooms and room reset, seals and doors; the gallery, the landmarks, the search for skips | done, play-tested once |
| 2 west wing | the orb, sockets, plates, levers, gates, W1 .. W4 | done |
| 3 east wing | single-layer blocks, channels, the solver extended, E1 .. E4 | next |
| 4 north wing | the prism and its links, N1 .. N4 | |
| 5 tower | T1 .. T3, an end screen | |
| 6 lights and polish | the 12 lights, play-test changes | |

### 12.1 Risks

| Risk | Handling | Why |
|---|---|---|
| Size (five times the first stage) | adding a room is data: a room table (bounds, entry, reset) and box helpers; push boards stay rows of text; geometry, outlines, solids and tests come from the same data. Grey boxes first, each wing played before polish | the cost of a room is its design; the data-driven pipeline is itself a point of the portfolio |
| Solver state explosion | the bottleneck is the dead-end count, which searches again from every state (quadratic); build the state graph once and search backwards from the goal states (linear); prune cells a block never leaves; encode states as integers in a hash table | about a hundred thousand states for 8 x 8, three blocks, two prisms; linear is enough for 12 x 12; the solver runs only in tests and tools |
| Orb physics | no free physics: put down one step ahead, falls straight down to the first surface of its layer, no roll, no bounce | one sentence of rules, deterministic, easy to test |
| Draw calls above 200 | static batching: fixed boxes merged per room and layer into one vertex buffer; only moving objects drawn one by one | from hundreds of draws to tens; the ripple draws both layers, so it matters twice; measured before and after |
| Fun cannot be proved | each wing played as grey boxes before polish; where a player stops and where a room is seen through at once decide the changes | fun is checked at every step, not at the end |

### 12.2 Room size

The size limit is for push boards (about 8 x 8), not for rooms. Corridors, heights, windows and views need no solver and
are proved by walks. A large room on several levels can hold a 6 x 6 board. Small boards also help: every block and wall is
seen at once, so the player thinks instead of searching.

## 13. Decisions

Agreed on 2026-11-22: the structure (hub, three wings, tower); the four new objects; the prologue changes; the jump limits;
R resets the current room.

Decided on 2026-11-22, after playing the west wing (the next design; it replaces the hub, the wings and the tower of this
document as they are built, step by step):

| Item | Decision |
|---|---|
| Two layers | one city in two states: A whole (warm evening light), B ruined by a storm (cold light, fog). In A the structure is the way (bridges, floors, doors); in B the breaches are (walls broken open), but bridges and floors are broken too. Red and blue outlines stay only on the key objects of one layer |
| Structure | a sunken square as the hub; four districts in a ring, each joined to two others without the square (a sky bridge of A, a cellar broken open in B); a cathedral over the square, the highest point; at least three levels in each district |
| Districts | west: market and sewers (the orb, going down); east: canal and locks (single-layer blocks, water); north-west: theatre and roofs (the prism as a change of scenery); north-east: graveyard and bell tower (prism and orb) |
| New objects | water, only in A: a wall, not swimming (a body that falls in goes back to the room entry); things float on it. Lifts: shortcuts only, entered standing still |
| Guidance | five levels: landmarks (far), light (middle), one look for everything that can be used (near), a safe first use of every object, and key prompts on the screen |
| Order | step 0, the guidance (done: the prompts, the columns of light, the previews of bridges); step 1, the identity of the two layers (done: the themes, the prologue gate; the world colours of the orbs); step 2, the vocabulary of shapes (done: stairs, arcades, railings, roofs, broken bridges, breaches, water, lifts, with the static batch; used so far: stairs, water, an arcade and roofs); then the new skeleton, the districts, the cathedral |


