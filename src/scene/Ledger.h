// Ledger.h - what twelve years do (docs/DESIGN.md 3.9): the present is computed from the past by a table of rules.
// Rule:      B is a function of A. What was left in A (a Subject: what it is), where it was left (a Site: a few facts
//            about the place) and a Table of rules give what stands in B (an Outcome). There is no history, no clock
//            and no growth to watch: the same subject at the same site gives the same outcome, whatever else was done
//            and in whatever order, and what no rule touches is as it was ("a thing that is never touched is the
//            same").
// Table:     the rules are data. A row says: this stuff, at a site that has all of `need` and none of `forbid`,
//            becomes this; the first row that matches is the one. A new puzzle may need a new row, never new code. The
//            trees, the people who keep things and the numbers that tie the rules to the limits of section 11 are in
//            the table too, and Problems() reads a table back for rows that can never match, trees that cannot be
//            climbed and the like.
// Sites:     five facts, set by the model for each place (soil, sun, roof, wall, sea) and never worked out here,
//            except the sea (WithSea).
// Derived:   a prop (a load held by a support), water held behind a dam and what a person keeps are not rules of
//            their own: they are the rows of the table seen through one more question (does the support stand? does
//            the dam hold? what is his place like?).
// Not here:  the circle of the lantern. Inside it the ledger has not run and A is what is seen and solid; that is the
//            work of scene/Moments.h, which swaps triangles, and nothing of this file is asked inside the circle.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_ledger.cpp).
// UNITS: metres, years.
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dl::ledger
{

/// What a thing is made of, as far as twelve years are concerned.
enum class Stuff : std::uint8_t
{
    OliveSeed,
    FigSeed,
    VineSeed,
    RawTimber,    ///< pale: planks, beams, props, posts
    TarredTimber, ///< black: the roofers' planks, the piles of the quay, doors of the harbour
    Cordage,      ///< rope
    Canvas,
    Basket,
    Stone, ///< dressed or heaped: no rule touches it
    IronShut, ///< an iron mechanism (a gate, a winch, a lever, a sluice) left shut
    IronOpen, ///< ... left open
    BallastCrate, ///< timber bound with iron and full of stone (docs/DESIGN.md 3.9)
    Boat,         ///< a tarred hull
    Count
};

inline constexpr int kStuffCount = static_cast<int>(Stuff::Count);

/// Facts about a place, as bit flags. The model sets them; nothing here works them out, except WithSea.
using Sites = std::uint8_t;
inline constexpr Sites kSoil = 1u << 0; ///< earth that takes a seed: a bed, a planter, a terrace of earth
inline constexpr Sites kSun = 1u << 1;  ///< open to the sun for most of the day: no roof, no wall or tree shades it
inline constexpr Sites kRoof = 1u << 2; ///< a roof over it, or a closed room: sheltered from the weather
inline constexpr Sites kWall = 1u << 3; ///< a wall at the foot that a vine can climb
inline constexpr Sites kSea = 1u << 4;  ///< covered by the sea in the storm (below `seaReach`): the ground is salted
inline constexpr Sites kAllSites = kSoil | kSun | kRoof | kWall | kSea;

/// What stands in B.
enum class Becomes : std::uint8_t
{
    Stands,     ///< still there, grey and sound enough to hold (and what no rule touches)
    Gone,       ///< not there: a stain, a stub, a line of rusted nails
    Nothing,    ///< an empty spot: nothing grew
    Tree,       ///< a tree: see Outcome::tree
    Tangle,     ///< a vine with no wall to climb: it grew, and gives no way up
    StuckShut,  ///< an iron mechanism, as A left it
    StuckOpen,  ///< ... open
    Heap,       ///< the stone of a ballast crate whose box has rotted
    LoadStands, ///< a load held by a support that stood
    LoadFallen, ///< a load whose support has rotted
    SiltBank,   ///< where water was held behind a dam that still holds: a bank as high as the dam
    DryBed,     ///< where water ran, or was held behind a dam that gave way: a bed, scoured and dry
};

enum class TreeKind : std::uint8_t
{
    None,
    Olive,
    Fig,
    Vine
};

struct Outcome
{
    Becomes becomes = Becomes::Stands;
    TreeKind tree = TreeKind::None; ///< when `becomes` is Tree
    float height = 0.0f;            ///< UNITS: metres: the top of a tree, a heap or a bank; the depth of a dry bed

    friend bool operator==(const Outcome& a, const Outcome& b)
    {
        return a.becomes == b.becomes && a.tree == b.tree && a.height == b.height;
    }
    friend bool operator!=(const Outcome& a, const Outcome& b)
    {
        return !(a == b);
    }
};

/// What was left in A.
struct Subject
{
    Stuff stuff = Stuff::Stone;
    float height = 0.0f; ///< UNITS: metres: the height of a crate or a dam, the depth of water that ran
};

/// One row: `stuff`, at a site that has all the flags of `need` and none of `forbid`, becomes `becomes`.
struct Rule
{
    Stuff stuff;
    Sites need;
    Sites forbid;
    Becomes becomes;
};

/// A tree, as a seed grows into it, and where the body can stand on it.
struct TreeDef
{
    TreeKind kind;
    Stuff seed;
    float top;                 ///< UNITS: metres: the height of the whole tree
    float from;                ///< UNITS: metres: the height the body must already stand at to take the first place
    std::vector<float> stands; ///< UNITS: metres: the places to stand on, from the foot of the tree, ascending
};

enum class Trade : std::uint8_t
{
    Gardener,
    Carpenter,
    Fisherman,
    Cooper,
    Count
};

inline constexpr int kTradeCount = static_cast<int>(Trade::Count);

/// A person of this trade takes this stuff and puts it in a place with these sites; anything else he refuses.
struct Keeper
{
    Trade trade;
    Stuff stuff;
    Sites place;
};

struct Table
{
    int years = 12;         ///< how long "now" is after the evening: also how much older the people are
    float maxRise = 1.2f;   ///< UNITS: metres: the largest rise between two places to stand (docs/DESIGN.md 11)
    /// The height of the heap of a ballast crate as a share of the crate. The stone of a crate is packed tight and
    /// keeps its height, so the built crates (steps of 1.2 m, proved by tests/test_route.cpp) do not change.
    float heapShare = 1.0f;
    float seaReach = 0.0f;  ///< UNITS: metres: the sea covered what is lower than this (docs/DESIGN.md 1.1)
    std::vector<Rule> rules;
    std::vector<TreeDef> trees;
    std::vector<Keeper> keepers;
};

/// The table of docs/DESIGN.md 3.9.
inline Table MakeTable()
{
    constexpr Sites kBad = kRoof | kSea; // where no seed grows: in the dark, or in salt
    Table t;
    t.rules = {
        // Growth: a seed planted in soil that has sun is a tree; a vine needs a wall (else it is a tangle)
        {Stuff::OliveSeed, kSoil | kSun, kBad, Becomes::Tree},
        {Stuff::OliveSeed, 0, 0, Becomes::Nothing},
        {Stuff::FigSeed, kSoil | kSun, kBad, Becomes::Tree},
        {Stuff::FigSeed, 0, 0, Becomes::Nothing},
        {Stuff::VineSeed, kSoil | kSun | kWall, kBad, Becomes::Tree},
        {Stuff::VineSeed, kSoil | kSun, kBad, Becomes::Tangle},
        {Stuff::VineSeed, 0, 0, Becomes::Nothing},
        // Rot and keeping: what is made of plant is gone in the open and stays under a roof; tar keeps timber
        {Stuff::TarredTimber, 0, 0, Becomes::Stands},
        {Stuff::RawTimber, kRoof, 0, Becomes::Stands},
        {Stuff::RawTimber, 0, 0, Becomes::Gone},
        {Stuff::Cordage, kRoof, 0, Becomes::Stands},
        {Stuff::Cordage, 0, 0, Becomes::Gone},
        {Stuff::Canvas, kRoof, 0, Becomes::Stands},
        {Stuff::Canvas, 0, 0, Becomes::Gone},
        {Stuff::Basket, kRoof, 0, Becomes::Stands},
        {Stuff::Basket, 0, 0, Becomes::Gone},
        // Rust: an iron mechanism is as A left it
        {Stuff::IronShut, 0, 0, Becomes::StuckShut},
        {Stuff::IronOpen, 0, 0, Becomes::StuckOpen},
        // A ballast crate: the box rots like any timber, and the stone stays
        {Stuff::BallastCrate, kRoof, 0, Becomes::Stands},
        {Stuff::BallastCrate, 0, 0, Becomes::Heap},
        // The sea: a boat below its reach is lost (a hull above it is tarred and stays)
        {Stuff::Boat, kSea, 0, Becomes::Gone},
        {Stuff::Boat, 0, 0, Becomes::Stands},
        // Stone is not in the table: nothing touches it
    };
    // The numbers of the trees: a rise between two places to stand is a step of docs/DESIGN.md 11 (at most `maxRise`)
    t.trees = {
        // olive: a crown to step onto from a ledge of 1.2 m; fig: limbs, a stair of three; vine: footholds, a ladder
        {TreeKind::Olive, Stuff::OliveSeed, 2.4f, 1.2f, {2.4f}},
        {TreeKind::Fig, Stuff::FigSeed, 4.0f, 0.0f, {1.2f, 2.3f, 3.4f}},
        {TreeKind::Vine, Stuff::VineSeed, 6.0f, 0.0f, {1.2f, 2.4f, 3.6f, 4.8f, 6.0f}},
    };
    // Who keeps what, and where: the gardener in his walled garden, the carpenter and the cooper in a dry shed and a
    // cellar (a roof), the fisherman on the slip above the tide line
    t.keepers = {
        {Trade::Gardener, Stuff::OliveSeed, kSoil | kSun | kWall},
        {Trade::Gardener, Stuff::FigSeed, kSoil | kSun | kWall},
        {Trade::Gardener, Stuff::VineSeed, kSoil | kSun | kWall},
        {Trade::Carpenter, Stuff::RawTimber, kRoof},
        {Trade::Carpenter, Stuff::TarredTimber, kRoof},
        {Trade::Fisherman, Stuff::Boat, kSun},
        {Trade::Cooper, Stuff::Basket, kRoof},
        {Trade::Cooper, Stuff::RawTimber, kRoof},
    };
    return t;
}

inline const Table& DefaultTable()
{
    static const Table table = MakeTable();
    return table;
}

/// The tree that grows from `seed`, or null.
inline const TreeDef* FindTree(const Table& t, Stuff seed)
{
    for (const TreeDef& d : t.trees)
    {
        if (d.seed == seed)
        {
            return &d;
        }
    }
    return nullptr;
}

/// The facts of a place at height `y` with the sea added when the sea covered it (a place lower than `seaReach`).
inline Sites WithSea(const Table& t, float y, Sites sites)
{
    return y < t.seaReach ? static_cast<Sites>(sites | kSea) : sites;
}

/// What `s` left at a place with these sites becomes in B. The first row of the table that matches decides; nothing
/// that matches leaves it standing.
inline Outcome Age(const Table& t, const Subject& s, Sites sites)
{
    for (const Rule& r : t.rules)
    {
        if (r.stuff != s.stuff || (sites & r.need) != r.need || (sites & r.forbid) != 0)
        {
            continue;
        }
        Outcome o;
        o.becomes = r.becomes;
        if (r.becomes == Becomes::Tree)
        {
            if (const TreeDef* d = FindTree(t, s.stuff))
            {
                o.tree = d->kind;
                o.height = d->top;
            }
        }
        else if (r.becomes == Becomes::Heap)
        {
            o.height = s.height * t.heapShare;
        }
        return o;
    }
    return Outcome{};
}

/// True for what is still there to hold something up or hold water back: what stands, a heap of stone, a sluice stuck
/// shut.
inline bool Holds(Becomes b)
{
    return b == Becomes::Stands || b == Becomes::Heap || b == Becomes::StuckShut;
}

/// A load held up by `support`: it stands when the support is still there, and has fallen when the support is gone. A
/// stone held up by timber falls when the timber rots (in the open, and not tarred); a stone held up by stone stands.
inline Outcome AgeProp(const Table& t, Stuff support, Sites sites)
{
    Outcome o;
    o.becomes = Holds(Age(t, Subject{support, 0.0f}, sites).becomes) ? Becomes::LoadStands : Becomes::LoadFallen;
    return o;
}

/// Water held behind `dam` (its height is the height of the dam): where it stood, a bank of silt as high as the dam, if
/// the dam still holds; if it gave way, the silt went with it and the bed is dry.
inline Outcome AgeDam(const Table& t, const Subject& dam, Sites sites)
{
    Outcome o;
    o.height = dam.height;
    o.becomes = Holds(Age(t, dam, sites).becomes) ? Becomes::SiltBank : Becomes::DryBed;
    return o;
}

/// Water that ran, `depth` deep (a channel, a flow let go): a bed scoured deep and dry.
inline Outcome AgeRun(float depth)
{
    Outcome o;
    o.becomes = Becomes::DryBed;
    o.height = depth;
    return o;
}

struct Entrusted
{
    bool accepted = false; ///< false: he refused, with a gesture, and the thing stays with the traveller
    Sites place = 0;       ///< the place where he keeps it
    Outcome outcome{Becomes::Nothing, TreeKind::None, 0.0f}; ///< what it is in B (only when accepted)
};

/// A small thing given to a person of `trade`: he takes it if his trade wants it and keeps it in his place (`extra`
/// adds facts that the place of this person has, such as the sea); otherwise he refuses and nothing is lost.
inline Entrusted Entrust(const Table& t, Trade trade, const Subject& s, Sites extra = 0)
{
    for (const Keeper& k : t.keepers)
    {
        if (k.trade == trade && k.stuff == s.stuff)
        {
            Entrusted e;
            e.accepted = true;
            e.place = static_cast<Sites>(k.place | extra);
            e.outcome = Age(t, s, e.place);
            return e;
        }
    }
    return Entrusted{};
}

/// The largest rise on the way up a tree: from `from` to the first place to stand, and from each to the next.
inline float LargestRise(const TreeDef& d)
{
    float level = d.from;
    float most = 0.0f;
    for (float h : d.stands)
    {
        most = h - level > most ? h - level : most;
        level = h;
    }
    return most;
}

/// A fact about a site that cannot be: a roof over a place that is in the sun, or a fact that does not exist.
inline const char* SiteProblem(Sites s)
{
    if ((s & kRoof) != 0 && (s & kSun) != 0)
    {
        return "a roof over a place that is in the sun";
    }
    if ((s & ~kAllSites) != 0)
    {
        return "a fact that is not one of the five";
    }
    return nullptr;
}

/// Reads a table back. Every problem is a sentence; an empty list is a table that can be used. WHY: the table is data,
/// so a wrong row must be found by a test and by the exporter, not by a player who plants a seed that can never grow.
inline std::vector<std::string> Problems(const Table& t)
{
    std::vector<std::string> out;
    const auto say = [&](const std::string& s) { out.push_back(s); };
    if (t.years <= 0)
    {
        say("years is not positive");
    }
    if (!(t.maxRise > 0.0f))
    {
        say("maxRise is not positive");
    }
    if (!(t.heapShare > 0.0f && t.heapShare <= 1.0f))
    {
        say("heapShare is not in (0, 1]");
    }
    for (std::size_t i = 0; i < t.rules.size(); ++i)
    {
        const Rule& r = t.rules[i];
        const std::string row = "rule " + std::to_string(i) + ": ";
        if ((r.need & r.forbid) != 0)
        {
            say(row + "a fact that is needed and forbidden at once: it can never match");
        }
        if (((r.need | r.forbid) & ~kAllSites) != 0)
        {
            say(row + "a fact that is not one of the five");
        }
        if (static_cast<int>(r.stuff) >= kStuffCount)
        {
            say(row + "a stuff that does not exist");
        }
        const bool derived = r.becomes == Becomes::LoadStands || r.becomes == Becomes::LoadFallen ||
                             r.becomes == Becomes::SiltBank || r.becomes == Becomes::DryBed;
        if (derived)
        {
            say(row + "LoadStands, LoadFallen, SiltBank and DryBed come from a prop and a dam, not from a row");
        }
        const bool seed = r.stuff == Stuff::OliveSeed || r.stuff == Stuff::FigSeed || r.stuff == Stuff::VineSeed;
        if ((r.becomes == Becomes::Tree || r.becomes == Becomes::Tangle) && !seed)
        {
            say(row + "only a seed becomes a tree or a tangle");
        }
        if (r.becomes == Becomes::Tree && FindTree(t, r.stuff) == nullptr)
        {
            say(row + "it makes a tree and the table has no tree for this seed");
        }
        // a row is dead when an earlier row of the same stuff matches wherever it matches
        for (std::size_t j = 0; j < i; ++j)
        {
            const Rule& e = t.rules[j];
            if (e.stuff == r.stuff && (e.need & r.need) == e.need && (e.forbid & r.forbid) == e.forbid &&
                (e.need & e.forbid) == 0)
            {
                say(row + "it can never be reached: rule " + std::to_string(j) + " matches wherever this one does");
                break;
            }
        }
    }
    for (std::size_t i = 0; i < t.trees.size(); ++i)
    {
        const TreeDef& d = t.trees[i];
        const std::string row = "tree " + std::to_string(i) + ": ";
        if (d.kind == TreeKind::None)
        {
            say(row + "its kind is None");
        }
        if (d.stands.empty())
        {
            say(row + "it has no place to stand");
            continue;
        }
        if (d.from < 0.0f)
        {
            say(row + "from is below the ground");
        }
        for (std::size_t k = 1; k < d.stands.size(); ++k)
        {
            if (!(d.stands[k] > d.stands[k - 1]))
            {
                say(row + "the places to stand do not rise");
            }
        }
        if (!(d.stands.front() > d.from))
        {
            say(row + "the first place to stand is not above from");
        }
        if (d.stands.back() > d.top + 1.0e-4f)
        {
            say(row + "a place to stand is above the top of the tree");
        }
        if (LargestRise(d) > t.maxRise + 1.0e-4f)
        {
            say(row + "a rise of " + std::to_string(LargestRise(d)) + " m is more than maxRise: it cannot be climbed");
        }
        for (std::size_t j = 0; j < i; ++j)
        {
            if (t.trees[j].seed == d.seed)
            {
                say(row + "a second tree for the same seed");
            }
        }
    }
    for (std::size_t i = 0; i < t.keepers.size(); ++i)
    {
        const Keeper& k = t.keepers[i];
        const std::string row = "keeper " + std::to_string(i) + ": ";
        if (const char* p = SiteProblem(k.place))
        {
            say(row + "his place is " + p);
        }
        if (static_cast<int>(k.trade) >= kTradeCount || static_cast<int>(k.stuff) >= kStuffCount)
        {
            say(row + "a trade or a stuff that does not exist");
        }
        for (std::size_t j = 0; j < i; ++j)
        {
            if (t.keepers[j].trade == k.trade && t.keepers[j].stuff == k.stuff)
            {
                say(row + "the same trade and the same stuff twice");
            }
        }
    }
    return out;
}

inline const char* Name(Stuff s)
{
    switch (s)
    {
    case Stuff::OliveSeed:
        return "olive seed";
    case Stuff::FigSeed:
        return "fig seed";
    case Stuff::VineSeed:
        return "vine seed";
    case Stuff::RawTimber:
        return "raw timber";
    case Stuff::TarredTimber:
        return "tarred timber";
    case Stuff::Cordage:
        return "cordage";
    case Stuff::Canvas:
        return "canvas";
    case Stuff::Basket:
        return "basket";
    case Stuff::Stone:
        return "stone";
    case Stuff::IronShut:
        return "iron (shut)";
    case Stuff::IronOpen:
        return "iron (open)";
    case Stuff::BallastCrate:
        return "ballast crate";
    case Stuff::Boat:
        return "boat";
    case Stuff::Count:
        break;
    }
    return "?";
}

inline const char* Name(Becomes b)
{
    switch (b)
    {
    case Becomes::Stands:
        return "stands";
    case Becomes::Gone:
        return "gone";
    case Becomes::Nothing:
        return "nothing";
    case Becomes::Tree:
        return "tree";
    case Becomes::Tangle:
        return "tangle";
    case Becomes::StuckShut:
        return "stuck shut";
    case Becomes::StuckOpen:
        return "stuck open";
    case Becomes::Heap:
        return "heap";
    case Becomes::LoadStands:
        return "load stands";
    case Becomes::LoadFallen:
        return "load fallen";
    case Becomes::SiltBank:
        return "silt bank";
    case Becomes::DryBed:
        return "dry bed";
    }
    return "?";
}

} // namespace dl::ledger
