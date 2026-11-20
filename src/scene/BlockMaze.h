// BlockMaze.h - the block puzzle of the courtyard as a grid, and a solver that proves things about it.
// The rule this file is about: the purple block exists in BOTH layers, so it is stopped by the walls of BOTH layers; the
// body is in ONE layer at a time and can switch where it fits. So the block moves through the cells that are free in A AND
// in B (the intersection of the two maps), the body through the cells that are free in A OR in B (their union, by
// switching). Reading the outlines of the other layer is how the player sees the intersection.
// Grid:      W x D cells of 1 m, cell (x, z), z grows to the north. Outside the grid counts as a wall of both layers.
// Cells:     '.' floor   'R' red wall (layer A only)   'B' blue wall (layer B only)   '#' stone (both)   'P' the plate
//            (floor)   'K' where the block starts (floor)   'S' where the body starts (floor, layer A)
// Climbing:  the block is 1 m high and a jump is 2 m, so the body can climb over the block: for the body, the cell of the
//            block is passable (on top of it). The walls of the courtyard are taller than a jump from the top of the block,
//            so the body cannot climb a wall (src/scene/Courtyard.h keeps them so; the low red wall of the finale is the
//            only exception, and in B, where the body could use it, it is not there).
// Solver:    a breadth-first search over (cell of the block, where the body can get to without pushing). One push moves
//            the block one cell; the body steps into the cell the block left, in the layer it pushed from. Walking and
//            switching cost nothing; the count is pushes, the measure that a player feels as "how far the block went".
// Modes:     Both = the game; OnlyA / OnlyB = a body that never switches (it still sees the block stopped by both layers).
// Plain logic, no Windows, no Direct3D, no game state: used by tests/test_maze.cpp and tools/maze_search.cpp.
#pragma once

#include <array>
#include <cstddef>
#include <queue>
#include <vector>

namespace dl::maze
{

inline constexpr int kMaxW = 12;
inline constexpr int kMaxD = 12;
inline constexpr int kMaxCells = kMaxW * kMaxD;

enum class Cell : unsigned char
{
    Floor,
    RedWall,  // solid in layer A only
    BlueWall, // solid in layer B only
    Stone,    // solid in both
};

enum class Mode
{
    Both,  // the body switches layers where it fits
    OnlyA, // the body stays in A
    OnlyB, // the body stays in B
};

struct Grid
{
    int w = 0;
    int d = 0;
    std::array<Cell, kMaxCells> cells{};
    int block = -1; // cell index of the block at the start
    int plate = -1; // cell index of the plate
    int start = -1; // cell index of the body at the start (it starts in layer A)

    int Index(int x, int z) const
    {
        return z * w + x;
    }
    int X(int i) const
    {
        return i % w;
    }
    int Z(int i) const
    {
        return i / w;
    }
    bool Inside(int x, int z) const
    {
        return x >= 0 && z >= 0 && x < w && z < d;
    }
    Cell At(int x, int z) const
    {
        return Inside(x, z) ? cells[static_cast<std::size_t>(Index(x, z))] : Cell::Stone;
    }
};

/// Builds a grid from rows of text, the NORTH row first (as it is read on a map). Returns false when a row has another
/// width, a character is unknown, the size is too large, or 'K', 'P' or 'S' is not there exactly once.
inline bool Parse(const char* const* rows, int rowCount, Grid& out)
{
    out = Grid{};
    if (rows == nullptr || rowCount <= 0 || rowCount > kMaxD)
    {
        return false;
    }
    int width = 0;
    while (rows[0][width] != '\0')
    {
        ++width;
    }
    if (width <= 0 || width > kMaxW)
    {
        return false;
    }
    out.w = width;
    out.d = rowCount;
    int blocks = 0;
    int plates = 0;
    int starts = 0;
    for (int r = 0; r < rowCount; ++r)
    {
        const int z = rowCount - 1 - r; // the first row is the north one
        for (int x = 0; x < width; ++x)
        {
            const char c = rows[r][x];
            if (c == '\0')
            {
                return false; // a short row
            }
            Cell cell = Cell::Floor;
            switch (c)
            {
            case '.':
                break;
            case 'R':
                cell = Cell::RedWall;
                break;
            case 'B':
                cell = Cell::BlueWall;
                break;
            case '#':
                cell = Cell::Stone;
                break;
            case 'P':
                out.plate = out.Index(x, z);
                ++plates;
                break;
            case 'K':
                out.block = out.Index(x, z);
                ++blocks;
                break;
            case 'S':
                out.start = out.Index(x, z);
                ++starts;
                break;
            default:
                return false;
            }
            out.cells[static_cast<std::size_t>(out.Index(x, z))] = cell;
        }
        if (rows[r][width] != '\0')
        {
            return false; // a long row
        }
    }
    return blocks == 1 && plates == 1 && starts == 1;
}

/// True when the body can be in (x, z) in `layer` (0 = A, 1 = B), the block not counted.
inline bool BodyFree(const Grid& g, int x, int z, int layer)
{
    switch (g.At(x, z))
    {
    case Cell::Floor:
        return true;
    case Cell::RedWall:
        return layer == 1;
    case Cell::BlueWall:
        return layer == 0;
    case Cell::Stone:
        return false;
    }
    return false;
}

/// True when the block can be in (x, z): free in both layers.
inline bool BlockFree(const Grid& g, int x, int z)
{
    return g.At(x, z) == Cell::Floor;
}

struct Push
{
    int blockCell; // where the block was before the push
    int dx;
    int dz;
};

struct Result
{
    bool solved = false;
    int pushes = -1;          // the fewest pushes that put the block on the plate
    std::vector<Push> path;   // one shortest solution
    int states = 0;           // (block, body region) states reachable from the start
    int deadStates = 0;       // of these, states from which the plate can no longer be reached
};

namespace detail
{

inline constexpr int kDx[4] = {1, -1, 0, 0};
inline constexpr int kDz[4] = {0, 0, 1, -1};

/// Every (cell, layer) the body can reach from (cell, layer) without moving the block; marks them in `seen` (index
/// cell * 2 + layer). The cell of the block counts as passable: the body climbs over it. The block still matters: it
/// decides which pushes are possible. Returns the smallest index reached: the name of the region.
inline int Flood(const Grid& g, int block, int cell, int layer, Mode mode, std::vector<char>& seen)
{
    (void)block; // the body climbs over the block (see "Climbing" at the top)
    const int n = g.w * g.d;
    seen.assign(static_cast<std::size_t>(n * 2), 0);
    std::vector<int> stack;
    const auto visit = [&](int c, int l) {
        const std::size_t k = static_cast<std::size_t>(c * 2 + l);
        if (seen[k] == 0 && BodyFree(g, g.X(c), g.Z(c), l))
        {
            seen[k] = 1;
            stack.push_back(c * 2 + l);
        }
    };
    visit(cell, layer);
    int smallest = cell * 2 + layer;
    while (!stack.empty())
    {
        const int k = stack.back();
        stack.pop_back();
        smallest = k < smallest ? k : smallest;
        const int c = k / 2;
        const int l = k % 2;
        if (mode == Mode::Both)
        {
            visit(c, 1 - l); // switch, where the body fits
        }
        for (int i = 0; i < 4; ++i)
        {
            const int x = g.X(c) + kDx[i];
            const int z = g.Z(c) + kDz[i];
            if (g.Inside(x, z))
            {
                visit(g.Index(x, z), l);
            }
        }
    }
    return smallest;
}

struct State
{
    int block;
    int bodyCell;
    int bodyLayer;
};

/// Breadth-first search from `from` in pushes. Fills `result` (solved, pushes, path) and, when `count` is set, the number
/// of reachable states. `key` is the visited map, sized by the caller.
inline void Search(const Grid& g, const State& from, Mode mode, Result& result, bool count)
{
    const int n = g.w * g.d;
    std::vector<int> parent(static_cast<std::size_t>(n * n * 2), -2); // key -> key of the state before (-1: the start)
    std::vector<Push> how(static_cast<std::size_t>(n * n * 2));
    std::vector<char> seen;
    const auto keyOf = [&](int block, int region) { return block * n * 2 + region; };

    std::queue<std::pair<State, int>> queue; // state, its key
    const int startRegion = Flood(g, from.block, from.bodyCell, from.bodyLayer, mode, seen);
    if ((seen.empty() || seen[static_cast<std::size_t>(from.bodyCell * 2 + from.bodyLayer)] == 0))
    {
        return; // the body does not fit where it starts
    }
    const int startKey = keyOf(from.block, startRegion);
    parent[static_cast<std::size_t>(startKey)] = -1;
    queue.push({from, startKey});
    int goalKey = -1;
    while (!queue.empty())
    {
        const auto [state, key] = queue.front();
        queue.pop();
        ++result.states;
        if (state.block == g.plate && goalKey < 0)
        {
            goalKey = key;
            if (!count)
            {
                break;
            }
        }
        Flood(g, state.block, state.bodyCell, state.bodyLayer, mode, seen);
        const int bx = g.X(state.block);
        const int bz = g.Z(state.block);
        for (int i = 0; i < 4; ++i)
        {
            // the body stands on the other side of the block, in a layer where it fits, and pushes
            const int px = bx - kDx[i];
            const int pz = bz - kDz[i];
            const int tx = bx + kDx[i];
            const int tz = bz + kDz[i];
            if (!g.Inside(px, pz) || !BlockFree(g, tx, tz))
            {
                continue;
            }
            for (int l = 0; l < 2; ++l)
            {
                if (seen[static_cast<std::size_t>(g.Index(px, pz) * 2 + l)] == 0)
                {
                    continue;
                }
                const State next{g.Index(tx, tz), state.block, l};
                std::vector<char> nextSeen;
                const int region = Flood(g, next.block, next.bodyCell, next.bodyLayer, mode, nextSeen);
                const int nextKey = keyOf(next.block, region);
                if (parent[static_cast<std::size_t>(nextKey)] != -2)
                {
                    continue;
                }
                parent[static_cast<std::size_t>(nextKey)] = key;
                how[static_cast<std::size_t>(nextKey)] = {state.block, kDx[i], kDz[i]};
                queue.push({next, nextKey});
            }
        }
    }
    if (goalKey >= 0)
    {
        result.solved = true;
        for (int k = goalKey; parent[static_cast<std::size_t>(k)] != -1; k = parent[static_cast<std::size_t>(k)])
        {
            result.path.insert(result.path.begin(), how[static_cast<std::size_t>(k)]);
        }
        result.pushes = static_cast<int>(result.path.size());
    }
}

} // namespace detail

/// The fewest pushes that put the block on the plate, with one shortest solution.
inline Result Solve(const Grid& g, Mode mode = Mode::Both)
{
    Result result;
    if (g.block < 0 || g.plate < 0 || g.start < 0)
    {
        return result;
    }
    const int startLayer = mode == Mode::OnlyB ? 1 : 0;
    detail::Search(g, {g.block, g.start, startLayer}, mode, result, false);
    return result;
}

/// Solve, plus how many states the player can get into and how many of them are dead ends (the plate can no longer be
/// reached: the player must press R). A puzzle with dead ends punishes a careless push; one without them cannot be lost.
inline Result Analyse(const Grid& g)
{
    Result result = Solve(g, Mode::Both);
    if (g.block < 0)
    {
        return result;
    }
    // walk every reachable state once more and ask the solver from each one
    const int n = g.w * g.d;
    std::vector<char> visited(static_cast<std::size_t>(n * n * 2), 0);
    std::vector<char> seen;
    std::queue<detail::State> queue;
    const auto keyOf = [&](const detail::State& s) {
        std::vector<char> tmp;
        return s.block * n * 2 + detail::Flood(g, s.block, s.bodyCell, s.bodyLayer, Mode::Both, tmp);
    };
    const detail::State start{g.block, g.start, 0};
    visited[static_cast<std::size_t>(keyOf(start))] = 1;
    queue.push(start);
    int states = 0;
    int dead = 0;
    while (!queue.empty())
    {
        const detail::State s = queue.front();
        queue.pop();
        ++states;
        Result from;
        detail::Search(g, s, Mode::Both, from, false);
        if (!from.solved)
        {
            ++dead;
            continue; // nothing after a dead end matters
        }
        detail::Flood(g, s.block, s.bodyCell, s.bodyLayer, Mode::Both, seen);
        const int bx = g.X(s.block);
        const int bz = g.Z(s.block);
        for (int i = 0; i < 4; ++i)
        {
            const int px = bx - detail::kDx[i];
            const int pz = bz - detail::kDz[i];
            const int tx = bx + detail::kDx[i];
            const int tz = bz + detail::kDz[i];
            if (!g.Inside(px, pz) || !BlockFree(g, tx, tz))
            {
                continue;
            }
            for (int l = 0; l < 2; ++l)
            {
                if (seen[static_cast<std::size_t>(g.Index(px, pz) * 2 + l)] == 0)
                {
                    continue;
                }
                const detail::State next{g.Index(tx, tz), s.block, l};
                const int k = keyOf(next);
                if (visited[static_cast<std::size_t>(k)] == 0)
                {
                    visited[static_cast<std::size_t>(k)] = 1;
                    queue.push(next);
                }
            }
        }
    }
    result.states = states;
    result.deadStates = dead;
    return result;
}

/// The map as a player in layer A sees it without reading the outlines: blue walls are invisible, so they are floor.
/// Solving it gives the solution that LOOKS right from A.
inline Grid AsSeenFromA(const Grid& g)
{
    Grid seen = g;
    for (Cell& c : seen.cells)
    {
        if (c == Cell::BlueWall)
        {
            c = Cell::Floor;
        }
    }
    return seen;
}

} // namespace dl::maze
