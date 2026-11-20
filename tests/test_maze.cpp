// test_maze.cpp - proves the block puzzle of the courtyard (src/scene/Courtyard.h) with the solver of src/scene/BlockMaze.h,
// and tests the solver itself on small grids whose answers are known by hand.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_maze.cpp -o test_maze
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. the solver: Parse; straight pushes; a block stopped by a wall of either layer; a body that must switch to get behind
//      the block; a body that stays in one layer; a dead end; the path it returns replays to the plate
//   2. the courtyard: solved with switching in exactly kCourtyardPushes pushes; NOT solved by a body that stays in A, nor by
//      one that stays in B; the solution that looks right from A (blue walls unseen) is much shorter, and its first push runs
//      into a blue wall; at least one push of the real solution is made by a body standing where a wall of the other layer
//      is; it has dead ends (a careless push costs an R)
//   3. the frame that the finale needs: the red wall runs north from beside the plate to the north edge
// Returns 1 when a check fails.

#include "scene/Courtyard.h"

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

/// Replays `path` on `g` and returns true when every push is legal (the block's next cell free in both layers) and the
/// block ends on the plate. The body is not replayed here: the solver's reachability is tested by the small grids.
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
        const char* rows[] = {"SKP"}; // two cells east... one push: block from x = 1 to x = 2
        const Grid g = Make(rows, 1);
        const Result r = Solve(g);
        CHECK(r.solved && r.pushes == 1 && Replays(g, r.path));
    }
    {
        const char* rows[] = {"SK.P"}; // two pushes
        const Result r = Solve(Make(rows, 1));
        CHECK(r.solved && r.pushes == 2);
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
        CHECK(Solve(g, Mode::Both).solved && Solve(g, Mode::Both).pushes == 1);
        CHECK(!Solve(g, Mode::OnlyA).solved); // stays in A: the red wall
        CHECK(!Solve(g, Mode::OnlyB).solved); // stays in B: the blue wall
    }
    {
        // the body climbs over the block: with the body east of the block, the west side is still reachable
        const char* rows[] = {".KSP"};
        // one row: the body starts between the block and the plate; the only way to the west side is over the block
        const Grid g = Make(rows, 1);
        const Result r = Solve(g, Mode::OnlyA);
        CHECK(r.solved && r.pushes == 2 && Replays(g, r.path));
    }
    {
        // a dead end: pushed north into the corner the block can never come back
        const char* rows[] = {
            "...",
            ".K.",
            "SP.",
        };
        const Grid g = Make(rows, 3);
        const Result r = Analyse(g);
        CHECK(r.solved && r.pushes == 1);
        CHECK(r.deadStates > 0 && r.states > r.deadStates);
    }
    {
        // the plate outside every cell the block can reach
        const char* rows[] = {"SK#P"};
        const Result r = Analyse(Make(rows, 1));
        CHECK(!r.solved && r.states == 1 && r.deadStates == 1);
    }
}

void TestCourtyard()
{
    const Grid g = Make(dl::courtyard::kRows, dl::courtyard::kRowCount);
    const Result both = Analyse(g);
    std::printf("  courtyard: %d pushes, %d states, %d dead ends\n", both.pushes, both.states, both.deadStates);
    CHECK(both.solved && both.pushes == dl::courtyard::kPushes);
    CHECK(Replays(g, both.path));
    // one layer is never enough: the puzzle is about the two together
    CHECK(!Solve(g, Mode::OnlyA).solved);
    CHECK(!Solve(g, Mode::OnlyB).solved);
    // the map lies to a player in A who does not read the outlines: it looks much shorter, and its first push hits blue
    const Result naive = Solve(AsSeenFromA(g), Mode::OnlyA);
    CHECK(naive.solved && naive.pushes + 5 <= both.pushes);
    if (!naive.path.empty())
    {
        const Push& first = naive.path.front();
        CHECK(g.At(g.X(first.blockCell) + first.dx, g.Z(first.blockCell) + first.dz) == Cell::BlueWall);
    }
    // at least one push of the solution is made standing where the other layer has a wall
    int fromInsideAWall = 0;
    for (const Push& p : both.path)
    {
        const Cell behind = g.At(g.X(p.blockCell) - p.dx, g.Z(p.blockCell) - p.dz);
        fromInsideAWall += (behind == Cell::RedWall || behind == Cell::BlueWall) ? 1 : 0;
    }
    std::printf("  courtyard: %d push(es) made from inside a wall of the other layer\n", fromInsideAWall);
    CHECK(fromInsideAWall >= 1);
    // careless pushes are possible, and most states are still alive
    CHECK(both.deadStates > 0 && both.deadStates * 2 < both.states);
    // the finale: the plate is beside the south end of a red wall that runs north to the north edge, under the doorway
    const int px = g.X(g.plate);
    const int pz = g.Z(g.plate);
    const int wx = dl::courtyard::kFinaleWallX;
    CHECK(wx == px + 1 || wx == px - 1);
    for (int z = pz; z < g.d; ++z)
    {
        CHECK(g.At(wx, z) == Cell::RedWall);
    }
    CHECK(g.At(wx, pz - 1) != Cell::RedWall); // it starts beside the plate, not before
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
