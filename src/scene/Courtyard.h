// Courtyard.h - the block puzzle of the courtyard: the layout, and what the solver has proved about it.
// The map, NORTH at the top, 1 m cells (x 0 .. 6 from west to east, z 0 .. 7 from south to north):
//
//     z7   . . . . . R .        R  red wall: solid in layer A only
//     z6   . . B . . R .        B  blue wall: solid in layer B only
//     z5   . . B . P R .        P  the red plate (under it, north of the red wall, the doorway of the finale)
//     z4   . . . . B K .        K  the purple block (solid in both layers)
//     z3   . . . B . . .        S  where the body comes down from the entrance
//     z2   . . . . . . .
//     z1   . . . . . . .
//     z0   . . . S . . .
//
// Seen from A, the block is one west push and one north push from the plate. It is not: the cell west of the block is a
// blue wall, and the block obeys both layers. The real way is ten pushes round the blue walls, and four of them are made by
// a body standing where a blue wall is, which it can only do in layer A. A body that never switches cannot solve it in
// either layer (tests/test_maze.cpp proves all of this with src/scene/BlockMaze.h).
// The red wall from the plate to the north edge is the finale: with the block on the plate, the body stands on the block,
// climbs the low red wall in A, walks along its top to the doorway and switches to B in the air, where the door is open.
// The layout was chosen by tools/maze_search.cpp (seed 73): fewest walls among layouts of 9 .. 14 pushes with these proofs.
#pragma once

#include "scene/BlockMaze.h"

namespace dl::courtyard
{

inline constexpr const char* kRows[] = {
    ".....R.",
    "..B..R.",
    "..B.PR.",
    "....BK.",
    "...B...",
    ".......",
    ".......",
    "...S...",
};
inline constexpr int kRowCount = static_cast<int>(sizeof(kRows) / sizeof(kRows[0]));

inline constexpr int kPushes = 10;     ///< the fewest pushes (tests/test_maze.cpp checks it)
inline constexpr int kFinaleWallX = 5; ///< the x of the low red wall that runs from beside the plate to the north edge

} // namespace dl::courtyard
