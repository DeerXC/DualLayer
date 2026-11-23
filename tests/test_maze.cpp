// test_maze.cpp - proves the block puzzle of the courtyard (src/scene/Courtyard.h) with the solver of src/scene/BlockMaze.h,
// and tests the solver itself on small grids whose answers are known by hand.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_maze.cpp -o test_maze
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only maze
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. the solver: Parse; straight pushes; a block stopped by a wall of either layer; a body that must switch twice to get
//      behind the block; climbing over the block; a dead end; a plate out of reach; the cells the block can be in
//   2. the courtyard: solved with switching in exactly kPushes pushes; NOT solved by a body that stays in A, nor by one that
//      stays in B; the solution that looks right from A (blue walls unseen) is much shorter, and its first push runs into a
//      blue wall; at least one push of the real solution is made by a body standing where a wall of the other layer is; it
//      has dead ends (a careless push costs an R), and most states are alive
// Returns 1 when a check fails.

#include "scene/Courtyard.h"

#include <cstddef>
#include <cstdio>

namespace
{

int g_failed = 0;
int g_passed = 0;

#define CHECK(cond)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (cond)                                                                                                      \
        {                                                                                                              \
            ++g_passed;                                                                                                \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            ++g_failed;                                                                                                \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);                                                        \
        }                                                                                                              \
    } while (0)

using namespace dl::maze;

Grid Make(const char* const* rows, int count)
{
    Grid g;
    const bool ok = Parse(rows, count, g);
    CHECK(ok);
    return g;
}

/// Replays `path` on `g`: every push legal (the next cell free in both layers) and the block ends on the plate.
bool Replays(const Grid& g, const std::vector<Push>& path)
{
    int block = g.block;
    for (const Push& p : path)
    {
        if (p.blockCell != block)
        {
            return false;
        }
        const int x = g.X(block) + p.dx;
        const int z = g.Z(block) + p.dz;
        if (!BlockFree(g, x, z))
        {
            return false;
        }
        block = g.Index(x, z);
    }
    return block == g.plate;
}

void TestParse()
{
    Grid g;
    const char* bad1[] = {"K.P", "S."};
    CHECK(!Parse(bad1, 2, g)); // rows of another width
    const char* bad2[] = {"KXP", "S.."};
    CHECK(!Parse(bad2, 2, g)); // an unknown character
    const char* bad3[] = {"K.P", "..."};
    CHECK(!Parse(bad3, 2, g)); // no start
    const char* good[] = {"K.P", "S.."};
    CHECK(Parse(good, 2, g) && g.w == 3 && g.d == 2);
    CHECK(g.block == g.Index(0, 1) && g.plate == g.Index(2, 1) && g.start == g.Index(0, 0)); // the first row is the north one
}

void TestSolverSmall()
{
    {
        const char* rows[] = {"SKP"};
        const Grid g = Make(rows, 1);
        const Result r = Solve(g);
        CHECK(r.solved && r.pushes == 1 && Replays(g, r.path));
    }
    {
        const char* rows[] = {"SK.P"};
        CHECK(Solve(Make(rows, 1)).pushes == 2);
    }
    {
        // a wall of ONE layer in the way of the block stops it (the block is in both layers)
        const char* red[] = {"SKRP"};
        const char* blue[] = {"SKBP"};
        CHECK(!Solve(Make(red, 1)).solved);
        CHECK(!Solve(Make(blue, 1)).solved);
    }
    {
        // the body must switch twice to get behind the block: a red wall (pass it in B), then a blue wall (stand in it in A)
        const char* rows[] = {"SR.BKP"};
        const Grid g = Make(rows, 1);
        const Result r = Solve(g, Mode::Both);
        CHECK(r.solved && r.pushes == 1);
        CHECK(!Solve(g, Mode::OnlyA).solved); // stays in A: the red wall
        CHECK(!Solve(g, Mode::OnlyB).solved); // stays in B: the blue wall
        CHECK(MinSwitches(g) == 2);           // to B before the red wall, back to A on the blue one
        const char* none[] = {"S.KP"};
        CHECK(MinSwitches(Make(none, 1)) == 0);
        const char* stuck[] = {"S#KP"};
        CHECK(MinSwitches(Make(stuck, 1)) == -1);
    }
    {
        // the body climbs over the block: one row, the body between the block and the plate; the only way to the west side
        // is over the block
        const char* rows[] = {".KSP"};
        const Grid g = Make(rows, 1);
        const Result r = Solve(g, Mode::OnlyA);
        CHECK(r.solved && r.pushes == 2 && Replays(g, r.path));
    }
    {
        // a dead end: pushed into the corner the block can never come back
        const char* rows[] = {
            "...",
            ".K.",
            "SP.",
        };
        const Grid g = Make(rows, 3);
        BlockCells cells;
        const Result r = Analyse(g, &cells);
        CHECK(r.solved && r.pushes == 1);
        CHECK(r.deadStates > 0 && r.states > r.deadStates);
        CHECK(cells.kind[static_cast<std::size_t>(g.Index(1, 1))] == 2); // the start: alive
        CHECK(cells.kind[static_cast<std::size_t>(g.Index(1, 2))] == 1); // pushed north against the edge: dead only
        CHECK(cells.kind[static_cast<std::size_t>(g.Index(0, 2))] == 0); // a corner it can never reach
    }
    {
        // the plate out of reach of every push
        const char* rows[] = {"SK#P"};
        const Result r = Analyse(Make(rows, 1));
        CHECK(!r.solved && r.states == 1 && r.deadStates == 1);
    }
}

void TestCourtyard()
{
    const Grid g = Make(dl::courtyard::kRows, dl::courtyard::kRowCount);
    CHECK(g.w == dl::courtyard::kWidth);
    const Result both = Analyse(g);
    std::printf("  courtyard: %d pushes, %d states, %d dead ends\n", both.pushes, both.states, both.deadStates);
    CHECK(both.solved && both.pushes == dl::courtyard::kPushes);
    // a room asks for few switches (docs/LEVEL_DESIGN.md 5.1): the courtyard is solved with two
    const int switches = MinSwitches(g);
    std::printf("  courtyard: solved with %d switches at the fewest\n", switches);
    CHECK(switches == 2);
    CHECK(Replays(g, both.path));
    CHECK(!Solve(g, Mode::OnlyA).solved); // one layer is never enough
    CHECK(!Solve(g, Mode::OnlyB).solved);
    // the map lies to a player in A who does not read the outlines: it looks much shorter, and its first push hits blue
    const Result naive = Solve(AsSeenFromA(g), Mode::OnlyA);
    CHECK(naive.solved && naive.pushes + 5 <= both.pushes);
    if (!naive.path.empty())
    {
        const Push& first = naive.path.front();
        CHECK(g.At(g.X(first.blockCell) + first.dx, g.Z(first.blockCell) + first.dz) == Cell::BlueWall);
    }
    int fromInsideAWall = 0; // pushes made standing where the other layer has a wall
    for (const Push& p : both.path)
    {
        const Cell behind = g.At(g.X(p.blockCell) - p.dx, g.Z(p.blockCell) - p.dz);
        fromInsideAWall += (behind == Cell::RedWall || behind == Cell::BlueWall) ? 1 : 0;
    }
    CHECK(fromInsideAWall >= 1);
    CHECK(both.deadStates > 0 && both.deadStates * 2 < both.states);
}

} // namespace

int main()
{
    TestParse();
    TestSolverSmall();
    TestCourtyard();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
