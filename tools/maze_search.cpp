// maze_search.cpp - finds a layout for the block puzzle of the courtyard with the solver of src/scene/BlockMaze.h.
// Not part of the game and not part of the tests: it was run once to choose the layout in src/scene/Stage.h, and it is kept
// so that the choice can be repeated or changed. It prints the best layouts it finds.
//   g++ -std=c++20 -O2 -I src tools/maze_search.cpp -o maze_search && ./maze_search [seed] [tries]
// The frame is fixed by the stage: 7 x 8 cells, the body enters at (3, 0) in the south; a red wall runs from (5, 5) north to
// (5, 7) under the doorway, and the plate is at (4, 5), beside its south end (the finale climbs that wall from the block).
// A layout is kept when:
//   - it is solved with switching, in 9 .. 14 pushes
//   - it is NOT solved by a body that stays in A, nor by one that stays in B
//   - the solution that looks right from A (blue walls unseen) is shorter, and one of its first two pushes runs into a
//     blue wall: the player learns at once that the block obeys the other layer too
//   - it has dead ends (a careless push costs an R), but not too many
// Among those it prefers few walls (few objects), and a length near 11 pushes.
#include "scene/BlockMaze.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace dl::maze;

namespace
{

unsigned g_state = 1u;
int Rand(int n)
{
    g_state = g_state * 1664525u + 1013904223u;
    return static_cast<int>((g_state >> 8) % static_cast<unsigned>(n));
}

constexpr int kW = 7;
constexpr int kD = 8;

void Print(const Grid& g)
{
    for (int z = g.d - 1; z >= 0; --z)
    {
        std::string row;
        for (int x = 0; x < g.w; ++x)
        {
            const int i = g.Index(x, z);
            char c = '.';
            switch (g.At(x, z))
            {
            case Cell::RedWall:
                c = 'R';
                break;
            case Cell::BlueWall:
                c = 'B';
                break;
            case Cell::Stone:
                c = '#';
                break;
            default:
                break;
            }
            if (i == g.plate)
            {
                c = 'P';
            }
            if (i == g.block)
            {
                c = 'K';
            }
            if (i == g.start)
            {
                c = 'S';
            }
            row += c;
        }
        std::printf("    \"%s\",\n", row.c_str());
    }
}

} // namespace

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    g_state = argc > 1 ? static_cast<unsigned>(std::atoi(argv[1])) : 7u;
    const int tries = argc > 2 ? std::atoi(argv[2]) : 200000;
    int bestScore = -1000000;
    for (int t = 0; t < tries; ++t)
    {
        Grid g;
        g.w = kW;
        g.d = kD;
        for (Cell& c : g.cells)
        {
            c = Cell::Floor;
        }
        g.cells[static_cast<std::size_t>(g.Index(5, 5))] = Cell::RedWall;
        g.cells[static_cast<std::size_t>(g.Index(5, 6))] = Cell::RedWall;
        g.cells[static_cast<std::size_t>(g.Index(5, 7))] = Cell::RedWall;
        g.plate = g.Index(4, 5);
        g.start = g.Index(3, 0);
        const auto isFixed = [&](int i) { return i == g.plate || i == g.start || g.cells[static_cast<std::size_t>(i)] != Cell::Floor; };
        const int walls = 4 + Rand(6);
        int placed = 0;
        for (int k = 0; k < 200 && placed < walls; ++k)
        {
            const int i = Rand(kW * kD);
            if (isFixed(i) || g.Z(i) == 0)
            {
                continue; // the south row stays free: the body lands there from the entrance
            }
            g.cells[static_cast<std::size_t>(i)] = Rand(2) == 0 ? Cell::RedWall : Cell::BlueWall;
            ++placed;
        }
        do
        {
            g.block = Rand(kW * kD);
        } while (isFixed(g.block) || g.Z(g.block) == 0);

        const Result both = Solve(g, Mode::Both);
        if (!both.solved || both.pushes < 9 || both.pushes > 14)
        {
            continue;
        }
        if (Solve(g, Mode::OnlyA).solved || Solve(g, Mode::OnlyB).solved)
        {
            continue;
        }
        const Result naive = Solve(AsSeenFromA(g), Mode::OnlyA);
        if (!naive.solved || naive.pushes >= both.pushes)
        {
            continue;
        }
        bool lesson = false;
        for (std::size_t k = 0; k < naive.path.size() && k < 2; ++k)
        {
            const Push& p = naive.path[k];
            lesson = lesson || g.At(g.X(p.blockCell) + p.dx, g.Z(p.blockCell) + p.dz) == Cell::BlueWall;
        }
        if (!lesson)
        {
            continue;
        }
        const Result a = Analyse(g);
        if (a.deadStates == 0 || a.deadStates * 2 > a.states)
        {
            continue;
        }
        const int score = 40 - 4 * placed - 3 * std::abs(both.pushes - 11) + 2 * (both.pushes - naive.pushes) + (a.deadStates > 3 ? 4 : 0);
        if (score > bestScore)
        {
            bestScore = score;
            std::printf("score %d: %d pushes (from A it looks like %d), %d walls, %d states, %d dead ends\n", score, both.pushes,
                        naive.pushes, placed, a.states, a.deadStates);
            Print(g);
        }
    }
    return 0;
}
