// test_ledger.cpp - tests of the ledger of twelve years (no window, no GPU, no town file): src/scene/Ledger.h, the
// rules of docs/DESIGN.md 3.9.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_ledger.cpp -o test_ledger
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only ledger
// Each rule is checked against an oracle written another way (nested conditions, not a table), on every combination
// of the five facts of a site (32), so that a row that is wrong at one site only is found. What is checked:
//   1. the table of the design has no problem, and its numbers are those of the design
//   2. growth: a seed grows in soil with sun and not under a roof or in the salt of the sea; a vine needs a wall, else
//      it is a tangle; the trees are the sizes of the table
//   3. rot and keeping: plant stuff is gone in the open and stands under a roof; tar keeps timber; stone is never
//      touched
//   4. rust: an iron mechanism is as A left it, at every site
//   5. a ballast crate: the box rots like timber and the stone stays, so a crate and a plank agree site by site
//   6. a prop: a load stands when its support does; water held behind a dam is a bank of silt when the dam holds and
//      a dry bed when it gave way; water that ran is a dry bed
//   7. the sea: it salts the ground and takes a boat, and nothing else; WithSea adds it by height
//   8. keepers: a person takes what his trade wants, keeps it in the place of his trade, and refuses the rest; what he
//      keeps is there in B
//   9. B is a function of A: the same question has the same answer, in any order; a roof never hurts what plant stuff
//      keeps, a fact that a seed needs never hurts it and the sea never helps it
//  10. the trees can be climbed: every rise between two places to stand is a step of docs/DESIGN.md 11
//  11. the table is the rule: changing a row, adding one first or taking one out changes the answer and no code; every
//      kind of wrong table is found by Problems(), by name
// Returns 1 when a check fails.

#include "check.h"
#include "scene/Ledger.h"

#include <algorithm>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

namespace
{

using namespace dl::ledger;

const Table& T()
{
    return DefaultTable();
}

Outcome Of(Stuff s, Sites site, float height = 0.0f)
{
    return Age(T(), Subject{s, height}, site);
}

bool Has(Sites s, Sites flag)
{
    return (s & flag) != 0;
}

constexpr int kSiteCount = 32; // every combination of the five facts

bool IsSeed(Stuff s)
{
    return s == Stuff::OliveSeed || s == Stuff::FigSeed || s == Stuff::VineSeed;
}

bool IsPlant(Stuff s) // what rots in the open
{
    return s == Stuff::RawTimber || s == Stuff::Cordage || s == Stuff::Canvas || s == Stuff::Basket;
}

/// Counts what went wrong in a matrix and says the first few, so that one check can stand for 32 sites of a rule.
struct Matrix
{
    int wrong = 0;
    void Expect(bool ok, const char* rule, Stuff s, Sites site)
    {
        if (!ok && ++wrong <= 5)
        {
            std::printf("    %s: %s at sites %d\n", rule, Name(s), static_cast<int>(site));
        }
    }
};

// ---- 1. the table of the design

void TestTable()
{
    const Table& t = T();
    const std::vector<std::string> problems = Problems(t);
    for (const std::string& p : problems)
    {
        std::printf("    problem: %s\n", p.c_str());
    }
    CHECK(problems.empty());
    CHECK(t.years == 12 && t.maxRise == 1.2f && t.heapShare == 1.0f && t.seaReach == 0.0f);
    CHECK(t.trees.size() == 3 && FindTree(t, Stuff::OliveSeed) != nullptr && FindTree(t, Stuff::Stone) == nullptr);
    // every enumerator has a name, and no two share one
    std::vector<std::string> names;
    for (int i = 0; i < kStuffCount; ++i)
    {
        names.push_back(Name(static_cast<Stuff>(i)));
    }
    std::sort(names.begin(), names.end());
    CHECK(std::adjacent_find(names.begin(), names.end()) == names.end() && names.front() != "?");
    std::vector<std::string> becomes;
    for (int i = 0; i <= static_cast<int>(Becomes::DryBed); ++i)
    {
        becomes.push_back(Name(static_cast<Becomes>(i)));
    }
    std::sort(becomes.begin(), becomes.end());
    CHECK(std::adjacent_find(becomes.begin(), becomes.end()) == becomes.end() && becomes.front() != "?");
}

// ---- 2. growth

Becomes SeedOracle(Stuff seed, Sites s)
{
    if (!Has(s, kSoil) || !Has(s, kSun) || Has(s, kRoof) || Has(s, kSea))
    {
        return Becomes::Nothing;
    }
    if (seed == Stuff::VineSeed && !Has(s, kWall))
    {
        return Becomes::Tangle;
    }
    return Becomes::Tree;
}

void TestGrowth()
{
    Matrix m;
    for (Stuff seed : {Stuff::OliveSeed, Stuff::FigSeed, Stuff::VineSeed})
    {
        for (int i = 0; i < kSiteCount; ++i)
        {
            const Sites s = static_cast<Sites>(i);
            m.Expect(Of(seed, s).becomes == SeedOracle(seed, s), "growth", seed, s);
        }
    }
    CHECK(m.wrong == 0);
    // the plain cases, by name
    const Sites bed = kSoil | kSun;
    CHECK(Of(Stuff::OliveSeed, bed).becomes == Becomes::Tree && Of(Stuff::OliveSeed, bed).tree == TreeKind::Olive);
    CHECK(Of(Stuff::OliveSeed, kSoil).becomes == Becomes::Nothing);        // shade
    CHECK(Of(Stuff::OliveSeed, kSun).becomes == Becomes::Nothing);         // no soil
    CHECK(Of(Stuff::FigSeed, bed | kRoof).becomes == Becomes::Nothing);    // under a roof
    CHECK(Of(Stuff::FigSeed, bed | kSea).becomes == Becomes::Nothing);     // salt
    CHECK(Of(Stuff::VineSeed, bed).becomes == Becomes::Tangle);            // no wall: it grows and gives no way up
    CHECK(Of(Stuff::VineSeed, bed | kWall).becomes == Becomes::Tree);      // the ladder
    CHECK(Of(Stuff::OliveSeed, bed | kWall).becomes == Becomes::Tree);     // a wall is no harm to an olive
    // the sizes of the trees
    CHECK(Of(Stuff::OliveSeed, bed).height == 2.4f && Of(Stuff::FigSeed, bed).height == 4.0f);
    CHECK(Of(Stuff::VineSeed, bed | kWall).height == 6.0f && Of(Stuff::VineSeed, bed | kWall).tree == TreeKind::Vine);
    CHECK(Of(Stuff::FigSeed, bed).tree == TreeKind::Fig);
    // a tree that did not grow has no size and no kind
    CHECK(Of(Stuff::FigSeed, kSoil).tree == TreeKind::None && Of(Stuff::FigSeed, kSoil).height == 0.0f);
}

// ---- 3. rot and keeping

void TestRot()
{
    Matrix m;
    for (int i = 0; i < kSiteCount; ++i)
    {
        const Sites s = static_cast<Sites>(i);
        for (Stuff p : {Stuff::RawTimber, Stuff::Cordage, Stuff::Canvas, Stuff::Basket})
        {
            m.Expect(Of(p, s).becomes == (Has(s, kRoof) ? Becomes::Stands : Becomes::Gone), "rot", p, s);
        }
        m.Expect(Of(Stuff::TarredTimber, s).becomes == Becomes::Stands, "tar", Stuff::TarredTimber, s);
        m.Expect(Of(Stuff::Stone, s).becomes == Becomes::Stands, "stone", Stuff::Stone, s);
        m.Expect(Of(Stuff::Stone, s) == Outcome{}, "stone is untouched", Stuff::Stone, s);
    }
    CHECK(m.wrong == 0);
    CHECK(Of(Stuff::RawTimber, 0).becomes == Becomes::Gone && Of(Stuff::RawTimber, kRoof).becomes == Becomes::Stands);
    CHECK(Of(Stuff::TarredTimber, 0).becomes == Becomes::Stands); // the black piles of the quay
    // a roof over a place that the sea covered keeps the plant stuff: the sea is not a rule of rot
    CHECK(Of(Stuff::Basket, kRoof | kSea).becomes == Becomes::Stands);
    CHECK(Of(Stuff::RawTimber, kSea).becomes == Becomes::Gone);
}

// ---- 4. rust

void TestRust()
{
    Matrix m;
    for (int i = 0; i < kSiteCount; ++i)
    {
        const Sites s = static_cast<Sites>(i);
        m.Expect(Of(Stuff::IronShut, s).becomes == Becomes::StuckShut, "rust shut", Stuff::IronShut, s);
        m.Expect(Of(Stuff::IronOpen, s).becomes == Becomes::StuckOpen, "rust open", Stuff::IronOpen, s);
    }
    CHECK(m.wrong == 0);
}

// ---- 5. a ballast crate

void TestCrate()
{
    Matrix m;
    for (int i = 0; i < kSiteCount; ++i)
    {
        const Sites s = static_cast<Sites>(i);
        const Outcome crate = Of(Stuff::BallastCrate, s, 1.0f);
        const bool boxStands = Of(Stuff::RawTimber, s).becomes == Becomes::Stands;
        const Becomes want = boxStands ? Becomes::Stands : Becomes::Heap;
        m.Expect(crate.becomes == want, "crate is its box", Stuff::BallastCrate, s);
        m.Expect(crate.height == (boxStands ? 0.0f : 1.0f), "heap height", Stuff::BallastCrate, s);
    }
    CHECK(m.wrong == 0);
    // the heap is as high as the crate was: the crates of the town are steps of 1.2 m (tests/test_things.cpp checks
    // that the box of B is as high as this heap)
    CHECK(Of(Stuff::BallastCrate, 0, 1.2f).height == 1.2f && Of(Stuff::BallastCrate, 0, 2.5f).height == 2.5f);
    CHECK(Of(Stuff::BallastCrate, 0, 0.0f).becomes == Becomes::Heap);
}

// ---- 6. a boat, a prop, water

void TestBoat()
{
    Matrix m;
    for (int i = 0; i < kSiteCount; ++i)
    {
        const Sites s = static_cast<Sites>(i);
        const Becomes want = Has(s, kSea) ? Becomes::Gone : Becomes::Stands;
        m.Expect(Of(Stuff::Boat, s).becomes == want, "boat", Stuff::Boat, s);
    }
    CHECK(m.wrong == 0);
}

bool HoldsOracle(Stuff st, Sites s)
{
    switch (st)
    {
    case Stuff::Stone:
    case Stuff::TarredTimber:
    case Stuff::IronShut:
    case Stuff::BallastCrate: // its stone stays, in the open as a heap and under a roof as the crate
        return true;
    case Stuff::RawTimber:
    case Stuff::Cordage:
    case Stuff::Canvas:
    case Stuff::Basket:
        return Has(s, kRoof);
    case Stuff::Boat:
        return !Has(s, kSea);
    default: // seeds and an open iron mechanism hold nothing
        return false;
    }
}

void TestProp()
{
    Matrix m;
    for (int k = 0; k < kStuffCount; ++k)
    {
        const Stuff st = static_cast<Stuff>(k);
        for (int i = 0; i < kSiteCount; ++i)
        {
            const Sites s = static_cast<Sites>(i);
            const Becomes b = AgeProp(T(), st, s).becomes;
            m.Expect(b == (HoldsOracle(st, s) ? Becomes::LoadStands : Becomes::LoadFallen), "prop", st, s);
        }
    }
    CHECK(m.wrong == 0);
    // the lintel of the side arch (docs/DESIGN.md 8.3): a timber prop in the open lets it fall; a stone block does not
    CHECK(AgeProp(T(), Stuff::RawTimber, 0).becomes == Becomes::LoadFallen);
    CHECK(AgeProp(T(), Stuff::Stone, 0).becomes == Becomes::LoadStands);
    CHECK(AgeProp(T(), Stuff::TarredTimber, 0).becomes == Becomes::LoadStands); // tar keeps it
    CHECK(AgeProp(T(), Stuff::RawTimber, kRoof).becomes == Becomes::LoadStands); // so does a roof
}

void TestWater()
{
    Matrix m;
    for (int k = 0; k < kStuffCount; ++k)
    {
        const Stuff st = static_cast<Stuff>(k);
        for (int i = 0; i < kSiteCount; ++i)
        {
            const Sites s = static_cast<Sites>(i);
            const Outcome o = AgeDam(T(), Subject{st, 1.5f}, s);
            const Becomes want = HoldsOracle(st, s) ? Becomes::SiltBank : Becomes::DryBed;
            m.Expect(o.becomes == want && o.height == 1.5f, "dam", st, s);
        }
    }
    CHECK(m.wrong == 0);
    // docs/DESIGN.md 8.4: a lock dammed with timber leaves no step; one dammed with ballast crates leaves a bank
    CHECK(AgeDam(T(), Subject{Stuff::RawTimber, 1.2f}, 0).becomes == Becomes::DryBed);
    CHECK(AgeDam(T(), Subject{Stuff::BallastCrate, 1.2f}, 0).becomes == Becomes::SiltBank);
    CHECK(AgeDam(T(), Subject{Stuff::IronShut, 2.0f}, 0).becomes == Becomes::SiltBank);  // a sluice stuck shut holds
    CHECK(AgeDam(T(), Subject{Stuff::IronOpen, 2.0f}, 0).becomes == Becomes::DryBed);    // one stuck open lets it go
    // water that ran leaves a bed as deep as it was
    const Outcome run = AgeRun(0.7f);
    CHECK(run.becomes == Becomes::DryBed && run.height == 0.7f);
}

// ---- 7. the sea

void TestSea()
{
    const Table& t = T();
    CHECK(WithSea(t, -0.01f, kSoil) == (kSoil | kSea));
    CHECK(WithSea(t, 0.0f, kSoil) == kSoil && WithSea(t, 3.0f, kSoil | kSun) == (kSoil | kSun));
    CHECK(WithSea(t, -6.0f, 0) == kSea && WithSea(t, -6.0f, kSea) == kSea); // the harbour; the flag is set once
    Table far = t;
    far.seaReach = 3.0f; // the sea of another design: the market is under it
    CHECK(WithSea(far, 2.0f, 0) == kSea && WithSea(far, 3.0f, 0) == 0);
    // the quay is under the sea and the first terrace is not: a seed planted at -6 m does not grow, one at +2 m does
    const Sites bed = kSoil | kSun;
    CHECK(Of(Stuff::OliveSeed, WithSea(t, -6.0f, bed)).becomes == Becomes::Nothing);
    CHECK(Of(Stuff::OliveSeed, WithSea(t, 2.0f, bed)).becomes == Becomes::Tree);
    // and it takes nothing else: stone, tar, iron, plant stuff and crates are as without it, in the open and under
    // a roof
    for (Stuff s : {Stuff::Stone, Stuff::TarredTimber, Stuff::IronShut, Stuff::IronOpen, Stuff::RawTimber,
                    Stuff::Basket, Stuff::BallastCrate})
    {
        CHECK(Of(s, kSea, 1.0f) == Of(s, 0, 1.0f));
        CHECK(Of(s, kSea | kRoof, 1.0f) == Of(s, kRoof, 1.0f));
    }
}

// ---- 8. keepers

/// Who takes what: written out, not read from the table.
bool TakesOracle(Trade tr, Stuff st)
{
    switch (tr)
    {
    case Trade::Gardener:
        return IsSeed(st);
    case Trade::Carpenter:
        return st == Stuff::RawTimber || st == Stuff::TarredTimber;
    case Trade::Fisherman:
        return st == Stuff::Boat;
    case Trade::Cooper:
        return st == Stuff::Basket || st == Stuff::RawTimber;
    case Trade::Count:
        break;
    }
    return false;
}

void TestKeepers()
{
    Matrix m;
    int refused = 0;
    for (int a = 0; a < kTradeCount; ++a)
    {
        const Trade tr = static_cast<Trade>(a);
        for (int k = 0; k < kStuffCount; ++k)
        {
            const Stuff st = static_cast<Stuff>(k);
            const Entrusted e = Entrust(T(), tr, Subject{st, 1.0f});
            m.Expect(e.accepted == TakesOracle(tr, st), "who takes what", st, static_cast<Sites>(a));
            if (!e.accepted)
            {
                ++refused;
                // he refuses with a gesture and nothing is lost: no place, nothing in B
                const bool nothing = e.place == 0 && e.outcome.becomes == Becomes::Nothing;
                m.Expect(nothing, "a refusal leaves nothing", st, static_cast<Sites>(a));
            }
            else
            {
                const Outcome inPlace = Age(T(), Subject{st, 1.0f}, e.place);
                m.Expect(e.outcome == inPlace, "kept as the rules say in his place", st, e.place);
                // what a person keeps is there in B (that is what a favour is for): standing, or a tree
                const Becomes kept = e.outcome.becomes;
                m.Expect(kept == Becomes::Stands || kept == Becomes::Tree, "a kept thing is there", st, e.place);
            }
        }
    }
    CHECK(m.wrong == 0);
    CHECK(refused == kTradeCount * kStuffCount - 8); // 8 rows of the table accept
    // the gardener plants in his own walled garden, so the vine he is given is a ladder
    Entrusted v = Entrust(T(), Trade::Gardener, Subject{Stuff::VineSeed, 0.0f});
    CHECK(v.accepted && v.outcome.becomes == Becomes::Tree && v.outcome.tree == TreeKind::Vine && Has(v.place, kWall));
    // a garden that the sea covered grows nothing, and a slip that it covered loses the boat
    CHECK(Entrust(T(), Trade::Gardener, Subject{Stuff::OliveSeed, 0.0f}, kSea).outcome.becomes == Becomes::Nothing);
    CHECK(Entrust(T(), Trade::Fisherman, Subject{Stuff::Boat, 0.0f}, kSea).outcome.becomes == Becomes::Gone);
    CHECK(Entrust(T(), Trade::Fisherman, Subject{Stuff::Boat, 0.0f}).outcome.becomes == Becomes::Stands);
    // the carpenter lays raw timber in his dry shed, where it stands; left in the open it is gone
    CHECK(Entrust(T(), Trade::Carpenter, Subject{Stuff::RawTimber, 0.0f}).outcome.becomes == Becomes::Stands);
    CHECK(Of(Stuff::RawTimber, 0).becomes == Becomes::Gone);
    // wrong trades: the carpenter does not take a seed, the gardener does not take a plank
    CHECK(!Entrust(T(), Trade::Carpenter, Subject{Stuff::OliveSeed, 0.0f}).accepted);
    CHECK(!Entrust(T(), Trade::Gardener, Subject{Stuff::RawTimber, 0.0f}).accepted);
}

// ---- 9. B is a function of A

Sites RandomSites(std::mt19937& rng)
{
    return static_cast<Sites>(rng() % kSiteCount);
}

Subject RandomSubject(std::mt19937& rng)
{
    const Stuff stuff = static_cast<Stuff>(rng() % static_cast<unsigned>(kStuffCount));
    return Subject{stuff, static_cast<float>(rng() % 40u) * 0.1f};
}

int Rank(Becomes b) // for a seed: better and worse
{
    return b == Becomes::Tree ? 2 : (b == Becomes::Tangle ? 1 : 0);
}

void TestFunction()
{
    std::mt19937 rng(2026u);
    // the same question, the same answer
    int differ = 0;
    std::vector<std::pair<Subject, Sites>> entries;
    for (int k = 0; k < 20000; ++k)
    {
        const Subject s = RandomSubject(rng);
        const Sites site = RandomSites(rng);
        differ += Age(T(), s, site) != Age(T(), s, site) ? 1 : 0;
        entries.push_back({s, site});
    }
    CHECK(differ == 0);
    // in any order: the answers of a shuffled list are the answers of the list, entry by entry
    std::vector<Outcome> direct;
    for (const auto& e : entries)
    {
        direct.push_back(Age(T(), e.first, e.second));
    }
    std::vector<std::size_t> order(entries.size());
    for (std::size_t i = 0; i < order.size(); ++i)
    {
        order[i] = i;
    }
    std::shuffle(order.begin(), order.end(), rng);
    int moved = 0;
    for (std::size_t i : order)
    {
        moved += Age(T(), entries[i].first, entries[i].second) != direct[i] ? 1 : 0;
    }
    CHECK(moved == 0);
    // nothing is remembered between questions: a copy of the table gives the same answers as the table itself
    const Table copy = T();
    int copied = 0;
    for (const auto& e : entries)
    {
        copied += Age(copy, e.first, e.second) != Age(T(), e.first, e.second) ? 1 : 0;
    }
    CHECK(copied == 0);
    // what each stuff can become, at any site (a stuff never becomes what is not its own)
    Matrix m;
    for (int k = 0; k < kStuffCount; ++k)
    {
        const Stuff st = static_cast<Stuff>(k);
        for (int i = 0; i < kSiteCount; ++i)
        {
            const Sites s = static_cast<Sites>(i);
            const Becomes b = Of(st, s, 1.0f).becomes;
            bool allowed = false;
            if (IsSeed(st))
            {
                allowed = b == Becomes::Tree || b == Becomes::Tangle || b == Becomes::Nothing;
            }
            else if (IsPlant(st) || st == Stuff::Boat)
            {
                allowed = b == Becomes::Stands || b == Becomes::Gone;
            }
            else if (st == Stuff::BallastCrate)
            {
                allowed = b == Becomes::Stands || b == Becomes::Heap;
            }
            else if (st == Stuff::IronShut)
            {
                allowed = b == Becomes::StuckShut;
            }
            else if (st == Stuff::IronOpen)
            {
                allowed = b == Becomes::StuckOpen;
            }
            else
            {
                allowed = b == Becomes::Stands; // stone, tarred timber
            }
            m.Expect(allowed, "outcome of its own", st, s);
        }
    }
    CHECK(m.wrong == 0);
    // a roof never hurts what plant stuff or a crate keeps, whatever else the place is
    Matrix roof;
    for (int i = 0; i < kSiteCount; ++i)
    {
        const Sites s = static_cast<Sites>(i);
        for (Stuff st : {Stuff::RawTimber, Stuff::Cordage, Stuff::Canvas, Stuff::Basket, Stuff::BallastCrate})
        {
            const bool stands = Of(st, s).becomes == Becomes::Stands;
            const bool still = Of(st, static_cast<Sites>(s | kRoof)).becomes == Becomes::Stands;
            roof.Expect(!stands || still, "a roof never hurts", st, s);
        }
        // a seed: soil, sun and a wall never hurt; a roof or the sea never help
        for (Stuff seed : {Stuff::OliveSeed, Stuff::FigSeed, Stuff::VineSeed})
        {
            const int here = Rank(Of(seed, s).becomes);
            for (Sites good : {kSoil, kSun, kWall})
            {
                const int with = Rank(Of(seed, static_cast<Sites>(s | good)).becomes);
                roof.Expect(with >= here, "what a seed needs never hurts", seed, s);
            }
            for (Sites bad : {kRoof, kSea})
            {
                const int with = Rank(Of(seed, static_cast<Sites>(s | bad)).becomes);
                roof.Expect(with <= here, "a roof and the sea never help", seed, s);
            }
        }
    }
    CHECK(roof.wrong == 0);
}

// ---- 10. the trees can be climbed

bool Climbable(const Table& t, const TreeDef& d)
{
    float level = d.from;
    for (float h : d.stands)
    {
        if (h - level > t.maxRise + 1.0e-4f)
        {
            return false;
        }
        level = h;
    }
    return !d.stands.empty();
}

void TestTrees()
{
    const Table& t = T();
    const TreeDef* olive = FindTree(t, Stuff::OliveSeed);
    const TreeDef* fig = FindTree(t, Stuff::FigSeed);
    const TreeDef* vine = FindTree(t, Stuff::VineSeed);
    CHECK(olive != nullptr && fig != nullptr && vine != nullptr);
    if (olive == nullptr || fig == nullptr || vine == nullptr)
    {
        return;
    }
    CHECK(olive->kind == TreeKind::Olive && fig->kind == TreeKind::Fig && vine->kind == TreeKind::Vine);
    CHECK(olive->top == 2.4f && fig->top == 4.0f && vine->top == 6.0f);
    // the olive is a step: a crown of 2.4 m taken from a ledge of 1.2 m; the fig a stair of three; the vine a ladder
    // to 6 m
    CHECK(olive->from == 1.2f && olive->stands.size() == 1 && olive->stands[0] == 2.4f);
    CHECK(fig->from == 0.0f && fig->stands.size() == 3 && vine->from == 0.0f && vine->stands.size() == 5);
    CHECK(vine->stands.back() == 6.0f && fig->stands.back() <= fig->top && olive->stands.back() <= olive->top);
    // every rise is a step of docs/DESIGN.md 11 (at most 1.2 m on the main path), and each tree can be climbed from its
    // start
    for (const TreeDef& d : t.trees)
    {
        CHECK(LargestRise(d) <= t.maxRise + 1.0e-4f && Climbable(t, d));
    }
    CHECK(std::fabs(LargestRise(*olive) - 1.2f) < 1.0e-4f && std::fabs(LargestRise(*fig) - 1.2f) < 1.0e-4f);
    // a tree that would need a rise of 1.5 m is not climbable and is said to be so
    TreeDef steep = *fig;
    steep.stands = {1.5f, 2.6f, 3.7f};
    CHECK(!Climbable(t, steep) && LargestRise(steep) > t.maxRise);
    // the places to stand rise
    for (const TreeDef& d : t.trees)
    {
        CHECK(std::is_sorted(d.stands.begin(), d.stands.end()) && d.stands.front() > d.from);
    }
}

// ---- 11. the table is the rule

bool Mentions(const std::vector<std::string>& problems, const char* word)
{
    for (const std::string& p : problems)
    {
        if (p.find(word) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

// The rows are found by what they say and not by where they stand, so that a table that is put in another order is not
// a different test.
Rule& RuleFor(Table& t, Stuff s, Becomes b)
{
    for (Rule& r : t.rules)
    {
        if (r.stuff == s && r.becomes == b)
        {
            return r;
        }
    }
    CHECK(false); // the default table has no such row
    static Rule none{};
    return none;
}

TreeDef& TreeOf(Table& t, Stuff seed)
{
    for (TreeDef& d : t.trees)
    {
        if (d.seed == seed)
        {
            return d;
        }
    }
    CHECK(false);
    static TreeDef none{};
    return none;
}

Keeper& KeeperFor(Table& t, Trade trade, Stuff s)
{
    for (Keeper& k : t.keepers)
    {
        if (k.trade == trade && k.stuff == s)
        {
            return k;
        }
    }
    CHECK(false);
    static Keeper none{};
    return none;
}

/// The default table with one defect, and the sentence that must be found for it.
struct Defect
{
    const char* name;
    Table table;
    const char* says;
};

void TestTableIsTheRule()
{
    const Table& base = T();
    const Sites bed = kSoil | kSun;
    // 1. a changed row is a changed answer: the olive of another design needs shade
    {
        Table t = base;
        RuleFor(t, Stuff::OliveSeed, Becomes::Tree) = Rule{Stuff::OliveSeed, kSoil, kSun | kRoof | kSea, Becomes::Tree};
        CHECK(Age(t, Subject{Stuff::OliveSeed, 0.0f}, kSoil).becomes == Becomes::Tree);
        CHECK(Age(t, Subject{Stuff::OliveSeed, 0.0f}, bed).becomes == Becomes::Nothing);
        CHECK(Of(Stuff::OliveSeed, bed).becomes == Becomes::Tree); // the default table is not touched by the copy
    }
    // 2. the first row that matches decides: a row put in front wins
    {
        Table t = base;
        t.rules.insert(t.rules.begin(), Rule{Stuff::RawTimber, 0, 0, Becomes::Stands});
        CHECK(Age(t, Subject{Stuff::RawTimber, 0.0f}, 0).becomes == Becomes::Stands);
        CHECK(Age(t, Subject{Stuff::Cordage, 0.0f}, 0).becomes == Becomes::Gone); // the other rows are as they were
    }
    // 3. what no row touches is as it was: take the rows of the basket away
    {
        Table t = base;
        const auto isBasket = [](const Rule& r) { return r.stuff == Stuff::Basket; };
        t.rules.erase(std::remove_if(t.rules.begin(), t.rules.end(), isBasket), t.rules.end());
        CHECK(Age(t, Subject{Stuff::Basket, 0.0f}, 0) == Outcome{});
        CHECK(Of(Stuff::Basket, 0).becomes == Becomes::Gone);
    }
    // 4. the numbers are data too: a fig of 5 m, a heap of three quarters of the crate, a vine that needs no wall
    {
        Table t = base;
        TreeOf(t, Stuff::FigSeed).top = 5.0f;
        t.heapShare = 0.75f;
        RuleFor(t, Stuff::VineSeed, Becomes::Tree) = Rule{Stuff::VineSeed, kSoil | kSun, kRoof | kSea, Becomes::Tree};
        CHECK(Age(t, Subject{Stuff::FigSeed, 0.0f}, bed).height == 5.0f);
        CHECK(Age(t, Subject{Stuff::BallastCrate, 2.0f}, 0).height == 1.5f);
        CHECK(Age(t, Subject{Stuff::VineSeed, 0.0f}, bed).becomes == Becomes::Tree);
        CHECK(Mentions(Problems(t), "never be reached")); // the row of the tangle is dead now: the one above takes it
    }
    // 5. another keeper: the cooper of another design takes cordage
    {
        Table t = base;
        t.keepers.push_back(Keeper{Trade::Cooper, Stuff::Cordage, kRoof});
        CHECK(Entrust(t, Trade::Cooper, Subject{Stuff::Cordage, 0.0f}).outcome.becomes == Becomes::Stands);
        CHECK(!Entrust(base, Trade::Cooper, Subject{Stuff::Cordage, 0.0f}).accepted);
    }
    // 6. every kind of wrong table is found, by name, and the right table is not blamed
    std::vector<Defect> defects;
    {
        Table t = base;
        RuleFor(t, Stuff::OliveSeed, Becomes::Tree).forbid = kSoil; // needs soil and forbids it
        defects.push_back({"need and forbid overlap", t, "needed and forbidden"});
    }
    {
        Table t = base;
        t.rules.insert(t.rules.begin(), Rule{Stuff::Cordage, 0, 0, Becomes::Stands});
        defects.push_back({"a row that is dead", t, "never be reached"});
    }
    {
        Table t = base;
        t.rules.push_back(Rule{Stuff::Stone, kRoof, 0, Becomes::Gone});
        t.rules.push_back(Rule{Stuff::Stone, kRoof | kWall, 0, Becomes::Stands}); // the row before it has these sites
        defects.push_back({"a narrower row after a wider one", t, "never be reached"});
    }
    {
        Table t = base;
        t.trees.clear();
        defects.push_back({"a tree row with no tree", t, "no tree for this seed"});
    }
    {
        Table t = base;
        TreeOf(t, Stuff::FigSeed).stands = {1.5f, 2.6f, 3.7f};
        defects.push_back({"a rise that cannot be climbed", t, "cannot be climbed"});
    }
    {
        Table t = base;
        TreeOf(t, Stuff::VineSeed).stands = {1.2f, 1.2f, 2.4f};
        defects.push_back({"places to stand that do not rise", t, "do not rise"});
    }
    {
        Table t = base;
        TreeOf(t, Stuff::OliveSeed).stands = {2.5f};
        defects.push_back({"a place above the top of the tree", t, "above the top"});
    }
    {
        Table t = base;
        TreeOf(t, Stuff::OliveSeed).from = 3.0f;
        defects.push_back({"the first place below where the body starts", t, "not above from"});
    }
    {
        Table t = base;
        const TreeDef again = TreeOf(t, Stuff::OliveSeed);
        t.trees.push_back(again);
        defects.push_back({"two trees for one seed", t, "second tree"});
    }
    {
        Table t = base;
        t.years = 0;
        defects.push_back({"no years", t, "years"});
    }
    {
        Table t = base;
        t.heapShare = 1.5f;
        defects.push_back({"a heap taller than its crate", t, "heapShare"});
    }
    {
        Table t = base;
        t.maxRise = 0.0f;
        defects.push_back({"a rise of nothing", t, "maxRise"});
    }
    {
        Table t = base;
        t.rules.push_back(Rule{Stuff::Stone, 0, 0, Becomes::SiltBank});
        defects.push_back({"a derived outcome in a row", t, "from a prop and a dam"});
    }
    {
        Table t = base;
        t.rules.push_back(Rule{Stuff::Stone, 0, 0, Becomes::Tree});
        defects.push_back({"stone that becomes a tree", t, "only a seed"});
    }
    {
        Table t = base;
        t.rules.push_back(Rule{Stuff::Stone, 0x40, 0, Becomes::Gone});
        defects.push_back({"a fact that is not one of the five", t, "not one of the five"});
    }
    {
        Table t = base;
        const Keeper twice = KeeperFor(t, Trade::Gardener, Stuff::OliveSeed);
        t.keepers.push_back(twice);
        defects.push_back({"the same keeper twice", t, "twice"});
    }
    {
        Table t = base;
        KeeperFor(t, Trade::Gardener, Stuff::OliveSeed).place = kRoof | kSun;
        defects.push_back({"a keeper whose place is a roof in the sun", t, "roof over a place"});
    }
    int notFound = 0;
    for (const Defect& d : defects)
    {
        const std::vector<std::string> p = Problems(d.table);
        const bool found = !p.empty() && Mentions(p, d.says);
        if (!found)
        {
            ++notFound;
            std::printf("    not found: %s (looking for \"%s\", %zu problems)\n", d.name, d.says, p.size());
        }
    }
    CHECK(notFound == 0);
    CHECK(defects.size() >= 16);
    CHECK(Problems(base).empty());
}

void TestSites()
{
    // a roof over a place in the sun is a mistake of the model; every other combination of the five facts is a place
    int mistakes = 0;
    for (int i = 0; i < kSiteCount; ++i)
    {
        const Sites s = static_cast<Sites>(i);
        const bool wrong = SiteProblem(s) != nullptr;
        mistakes += wrong ? 1 : 0;
        CHECK(wrong == (Has(s, kRoof) && Has(s, kSun)));
    }
    CHECK(mistakes == 8); // roof and sun, with any of the other three facts
    CHECK(SiteProblem(0x20) != nullptr && SiteProblem(kAllSites & ~kSun) == nullptr);
    CHECK(kAllSites == 31);
}

} // namespace

int main()
{
    TestTable();
    TestGrowth();
    TestRot();
    TestRust();
    TestCrate();
    TestBoat();
    TestProp();
    TestWater();
    TestSea();
    TestKeepers();
    TestFunction();
    TestTrees();
    TestTableIsTheRule();
    TestSites();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
