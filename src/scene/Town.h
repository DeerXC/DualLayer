// Town.h - the town of the layout model, loaded as data (docs/DESIGN.md 7.1 and 12, step 2).
// File:      data/town.dlt, written by `python tools/layout/city_model.py export` (its docstring of export() is the
//            format): the lights of both moments, the materials (an sRGB colour in A and in B, and whether it shines),
//            the stations of the main path, the streets (their middle lines), the polygons of the town (their moment,
//            their kind and their material), the things that change, the steps of the main path, the three emblems of
//            the signal mast, the overlooks, the lamps of the streets and the plan of both moments for the map. The
//            game and the model are one town: a change of the layout is made in the model and exported again.
// Kinds:     solid (walls, roofs, bridges, trees...), terrain (the ground), overlay (paving and treads laid on the
//            ground: drawn, not solid: the ground under them already has their height), water (in A: drawn as water, a
//            body under its surface is in the water).
// Drawn:     BakeLayer gives the static batch of one moment: every polygon of that moment as a fan of triangles, both
//            sides (the model's polygons have no fixed winding), the terrain and the water from above only. Colours are
//            the model's own (no stone tint), a shining material glows. The stone, the plaster, the tile and the ground
//            (Painted) take the hand-painted look of the style of the light (alpha 1: blotches, tall rain stains, the
//            damp of the foot of the town, a third of the rim); the town's tint is white and its weathering 0, so that
//            nothing else of the stone path of the shader applies. BakeWater gives the water of A.
// Solid:     BuildMesh gives the TriMesh of one moment: its solid and terrain polygons.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_town.cpp).
// UNITS: metres. Space: left-handed, +Y up.
#pragma once

#include "render/StaticBatch.h"
#include "scene/Moments.h"
#include "scene/SceneData.h"
#include "scene/TriMesh.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace dl::town
{

inline constexpr char kPath[] = "data/town.dlt";
inline constexpr std::uint32_t kVersion = 6;

enum class Kind : std::uint8_t
{
    Solid = 0,
    Terrain = 1,
    Overlay = 2,
    Water = 3,
};

struct Light
{
    float toLight[3];
    float color[3];
    float shade[3];
    float skyTop[3];
    float horizon[3];
    float fog = 0.0f;
};

struct Material
{
    std::string name;
    float a[3] = {};
    float b[3] = {};
    bool shinesA = false;
    bool shinesB = false;
};

struct Place
{
    std::string name;
    Vec3 at{};
};

struct Street
{
    std::string name;
    std::uint8_t kind = 0; ///< 0 street, 1 stair, 2 lane
    float width = 0.0f;
    std::vector<Vec3> line; ///< its middle line, on its surface
};

/// One box of a thing that changes (scene/Things.h): solid and drawn in the moments of `worlds` while the thing is in
/// `state`.
struct ThingBox
{
    std::uint8_t state = 0;  ///< 0 shut, 1 open
    std::uint8_t worlds = 0; ///< bit 0: in A, bit 1: in B
    std::uint16_t material = 0;
    Vec3 centre{};
    Vec3 half{};      ///< UNITS: metres: half its size along its own axes
    float yaw = 0.0f; ///< radians round Y (the model's rect turn: x' = x cos - z sin, z' = x sin + z cos)
};

/// What E does to a thing (what its prompt says).
enum class Verb : std::uint8_t
{
    Open = 0,
    Push = 1,
    Lay = 2,
    Ring = 3,
    Lower = 4,
};

/// A thing that changes: E at its handle in A opens it, in A and (by the causal rule) in B.
struct ThingDef
{
    std::string name;
    Verb verb = Verb::Open;
    Vec3 handle{};
    std::vector<ThingBox> boxes;
};

/// One step of the main path (docs/DESIGN.md 7.4, 8), in the order it is played: a thing used at its handle in A, or
/// the lantern set down at a spot. The game guides from the steps (scene/Guidance.h); tests/test_route.cpp plays them.
enum class StepKind : std::uint8_t
{
    Thing = 0,
    Lantern = 1,
};

struct Step
{
    std::string name; ///< the name of the thing (kind Thing), or "lantern"
    StepKind kind = StepKind::Thing;
    Vec3 at{}; ///< the handle of the thing, or the spot of the lantern (the platform the lantern is set on)
};

/// One lamp of the signal mast (scene/Emblems.h): lit when the bell of its district is rung.
struct Emblem
{
    std::string glyph;      ///< "well" (west), "cliff" (north), "lock" (east)
    std::string bell;       ///< the name of the thing whose ringing lights it
    Vec3 at{};              ///< the middle of the lamp
    float size = 3.0f;      ///< UNITS: metres
    std::uint8_t shape = 0; ///< 0 sphere, 1 cylinder, 2 cube
};

/// A place from which the town is seen far: standing there reveals the map round it (scene/MapMemory.h).
struct Overlook
{
    std::string name;
    Vec3 at{};
    float radius = 0.0f; ///< UNITS: metres
};

/// What a lamp is (what the game does with it: scene/Lamps.h).
enum class LampKind : std::uint8_t
{
    Street = 0, ///< a lamp on a post along a street
    Post = 1,   ///< a lamp post of the plaza
    Light = 2,  ///< a lighthouse or the light of the mole: far, strong
    Memory = 3, ///< one of the ring of lights of the bell tower
    Grave = 4,  ///< the lantern on a new grave
};

/// A light of the town that shines where the model has a lantern: where it is, how far it reaches, its colour (sRGB)
/// and strength, how much it flickers, and the moments it shines in.
struct Lamp
{
    Vec3 at{};
    float range = 10.0f;                 ///< UNITS: metres
    float color[3] = {1.0f, 0.7f, 0.4f}; ///< sRGB
    float intensity = 1.0f;              ///< the strength: 1 lights a wall with the light of the sun
    float flicker = 0.0f;                ///< 0 .. 0.4
    std::uint8_t worlds = 3;             ///< bit 0: in A, bit 1: in B
    LampKind kind = LampKind::Street;
};

/// What stands in a cell of the plan.
enum PlanClass : std::uint8_t
{
    kPlanEmpty = 0,
    kPlanWater = 1,
    kPlanCliff = 2,
    kPlanBuilding = 3,
    kPlanStreet = 4,
    kPlanClassCount = 5,
};

/// The plan of both moments for the map: cells of `cell` metres, row by row from the south (z0) up, each row from x0
/// east.
struct Plan
{
    float cell = 2.0f;
    float x0 = 0.0f;
    float z0 = 0.0f;
    int nx = 0;
    int nz = 0;
    std::vector<std::uint8_t> sheet[2]; ///< [0] the past, [1] the present: nx * nz classes each

    bool Valid() const
    {
        const std::size_t n = static_cast<std::size_t>(nx) * static_cast<std::size_t>(nz);
        return nx > 0 && nz > 0 && cell > 0.0f && sheet[0].size() == n && sheet[1].size() == n;
    }
    int CellX(float x) const
    {
        return static_cast<int>(std::floor((x - x0) / cell));
    }
    int CellZ(float z) const
    {
        return static_cast<int>(std::floor((z - z0) / cell));
    }
    bool In(int i, int j) const
    {
        return i >= 0 && j >= 0 && i < nx && j < nz;
    }
    float CentreX(int i) const
    {
        return x0 + (static_cast<float>(i) + 0.5f) * cell;
    }
    float CentreZ(int j) const
    {
        return z0 + (static_cast<float>(j) + 0.5f) * cell;
    }
    /// The class of cell (i, j) in `world` (0 A, 1 B); kPlanEmpty outside the plan.
    std::uint8_t At(int world, int i, int j) const
    {
        if (!In(i, j) || (world != 0 && world != 1) || !Valid())
        {
            return kPlanEmpty;
        }
        return sheet[world][static_cast<std::size_t>(j) * static_cast<std::size_t>(nx) + static_cast<std::size_t>(i)];
    }
};

struct Polygon
{
    std::uint8_t worlds = 0; ///< bit 0: in A, bit 1: in B
    Kind kind = Kind::Solid;
    std::uint16_t material = 0;
    std::uint32_t first = 0; ///< its vertices: vertices[first .. first + count)
    std::uint8_t count = 0;
};

class TownData
{
  public:
    float sea = 0.0f;
    Vec3 spawn{};
    float spawnYaw = 0.0f;
    float spawnPitch = 0.0f; ///< radians, + looks up: the first frame (tests/test_firstminute.cpp)
    int startLayer = 0;      ///< the moment the game starts in (0 A, 1 B)
    Light lights[2] = {};
    std::vector<Material> materials;
    std::vector<Place> places;
    std::vector<Street> streets;
    std::vector<Polygon> polygons;
    std::vector<Vec3> vertices;
    std::vector<std::uint32_t> waters; ///< the polygons of kind water (Submerged looks only at these)
    std::vector<ThingDef> things;      ///< the things that change (scene/Things.h)
    std::vector<Step> steps;           ///< the main path in the order it is played
    std::vector<Emblem> emblems;       ///< the lamps of the signal mast
    std::vector<Overlook> overlooks;   ///< where the map is revealed far round
    std::vector<Lamp> lamps;           ///< the lamps of the streets and the lights of the harbour
    Plan plan;                         ///< the plan of both moments, for the map

    /// Reads the file. False (and `error` says why) when it is missing, of another version, or cut short.
    bool Load(const char* path, std::string& error)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            error = std::string("cannot open ") + path + " (make it with: python tools/layout/city_model.py export)";
            return false;
        }
        // WHY one read and not an iterator over the stream: the town file is 3 MB, and a Debug build of the standard
        // library checks every step of an iterator (seconds at the start of the game)
        in.seekg(0, std::ios::end);
        const std::streamoff size = in.tellg();
        in.seekg(0, std::ios::beg);
        if (size < 0 || !in)
        {
            error = std::string("cannot read ") + path;
            return false;
        }
        std::vector<unsigned char> bytes(static_cast<std::size_t>(size));
        if (size > 0 && !in.read(reinterpret_cast<char*>(bytes.data()), size))
        {
            error = std::string("cannot read ") + path;
            return false;
        }
        return Parse(bytes.data(), bytes.size(), error);
    }

    /// Reads the bytes of a file (Load, and the tests).
    bool Parse(const unsigned char* data, std::size_t size, std::string& error)
    {
        *this = TownData{};
        Reader r{data, size};
        char magic[8] = {};
        if (!r.Bytes(magic, 8) || std::memcmp(magic, "DLTOWN\0\0", 8) != 0)
        {
            error = "not a town file";
            return false;
        }
        std::uint32_t version = 0;
        if (!r.U32(version) || version != kVersion)
        {
            error = "a town file of another version (export it again)";
            return false;
        }
        std::uint8_t start = 0;
        bool ok = r.F32(sea) && r.F32(spawn.x) && r.F32(spawn.y) && r.F32(spawn.z) && r.F32(spawnYaw) &&
                  r.F32(spawnPitch) && r.U8(start) && start <= 1;
        startLayer = start;
        for (Light& l : lights)
        {
            ok = ok && r.Floats(l.toLight, 3) && r.Floats(l.color, 3) && r.Floats(l.shade, 3) &&
                 r.Floats(l.skyTop, 3) && r.Floats(l.horizon, 3) && r.F32(l.fog);
        }
        std::uint32_t n = 0;
        ok = ok && r.U32(n) && n < 1024u;
        for (std::uint32_t i = 0; ok && i < n; ++i)
        {
            Material m;
            std::uint8_t shinesA = 0;
            std::uint8_t shinesB = 0;
            ok = r.Name(m.name) && r.Floats(m.a, 3) && r.Floats(m.b, 3) && r.U8(shinesA) && r.U8(shinesB);
            m.shinesA = shinesA != 0;
            m.shinesB = shinesB != 0;
            materials.push_back(m);
        }
        ok = ok && r.U32(n) && n < 1024u;
        for (std::uint32_t i = 0; ok && i < n; ++i)
        {
            Place p;
            ok = r.Name(p.name) && r.F32(p.at.x) && r.F32(p.at.y) && r.F32(p.at.z);
            places.push_back(p);
        }
        ok = ok && r.U32(n) && n < 1024u;
        for (std::uint32_t i = 0; ok && i < n; ++i)
        {
            Street st;
            std::uint16_t points = 0;
            ok = r.Name(st.name) && r.U8(st.kind) && r.F32(st.width) && r.U16(points) && points >= 2;
            for (std::uint16_t k = 0; ok && k < points; ++k)
            {
                Vec3 v{};
                ok = r.F32(v.x) && r.F32(v.y) && r.F32(v.z);
                st.line.push_back(v);
            }
            streets.push_back(st);
        }
        ok = ok && r.U32(n) && n < 4000000u;
        polygons.reserve(ok ? n : 0u);
        for (std::uint32_t i = 0; ok && i < n; ++i)
        {
            Polygon p;
            std::uint8_t kind = 0;
            ok = r.U8(p.worlds) && r.U8(kind) && r.U16(p.material) && r.U8(p.count) && kind <= 3 && p.worlds >= 1 &&
                 p.worlds <= 3 && p.count >= 3 && p.material < materials.size();
            p.kind = static_cast<Kind>(kind);
            p.first = static_cast<std::uint32_t>(vertices.size());
            for (int k = 0; ok && k < p.count; ++k)
            {
                Vec3 v{};
                ok = r.F32(v.x) && r.F32(v.y) && r.F32(v.z) && std::isfinite(v.x) && std::isfinite(v.y) &&
                     std::isfinite(v.z);
                vertices.push_back(v);
            }
            if (ok && p.kind == Kind::Water)
            {
                waters.push_back(static_cast<std::uint32_t>(polygons.size()));
            }
            polygons.push_back(p);
        }
        ok = ok && r.U32(n) && n < 64u;
        for (std::uint32_t i = 0; ok && i < n; ++i)
        {
            ThingDef t;
            std::uint16_t boxes = 0;
            std::uint8_t verb = 0;
            ok = r.Name(t.name) && r.U8(verb) && verb <= 4 && r.F32(t.handle.x) && r.F32(t.handle.y) &&
                 r.F32(t.handle.z) && r.U16(boxes) && boxes >= 1;
            for (std::uint16_t k = 0; ok && k < boxes; ++k)
            {
                ThingBox b;
                ok = r.U8(b.state) && r.U8(b.worlds) && r.U16(b.material) && r.F32(b.centre.x) && r.F32(b.centre.y) &&
                     r.F32(b.centre.z) && r.F32(b.half.x) && r.F32(b.half.y) && r.F32(b.half.z) && r.F32(b.yaw) &&
                     b.state <= 1 && b.worlds >= 1 && b.worlds <= 3 && b.material < materials.size() &&
                     b.half.x > 0.0f && b.half.y > 0.0f && b.half.z > 0.0f;
                t.boxes.push_back(b);
            }
            t.verb = static_cast<Verb>(verb);
            things.push_back(t);
        }
        ok = ok && r.U32(n) && n < 64u;
        for (std::uint32_t i = 0; ok && i < n; ++i)
        {
            Step st;
            std::uint8_t kind = 0;
            ok = r.Name(st.name) && r.U8(kind) && kind <= 1 && r.F32(st.at.x) && r.F32(st.at.y) && r.F32(st.at.z);
            st.kind = static_cast<StepKind>(kind);
            steps.push_back(st);
        }
        ok = ok && r.U32(n) && n < 16u;
        for (std::uint32_t i = 0; ok && i < n; ++i)
        {
            Emblem e;
            ok = r.Name(e.glyph) && r.Name(e.bell) && r.F32(e.at.x) && r.F32(e.at.y) && r.F32(e.at.z) &&
                 r.F32(e.size) && r.U8(e.shape) && e.shape <= 2 && e.size > 0.0f;
            emblems.push_back(e);
        }
        ok = ok && r.U32(n) && n < 64u;
        for (std::uint32_t i = 0; ok && i < n; ++i)
        {
            Overlook o;
            ok =
                r.Name(o.name) && r.F32(o.at.x) && r.F32(o.at.y) && r.F32(o.at.z) && r.F32(o.radius) && o.radius > 0.0f;
            overlooks.push_back(o);
        }
        ok = ok && r.U32(n) && n < 4096u;
        for (std::uint32_t i = 0; ok && i < n; ++i)
        {
            Lamp lamp;
            std::uint8_t kind = 0;
            ok = r.F32(lamp.at.x) && r.F32(lamp.at.y) && r.F32(lamp.at.z) && r.F32(lamp.range) &&
                 r.Floats(lamp.color, 3) && r.F32(lamp.intensity) && r.F32(lamp.flicker) && r.U8(lamp.worlds) &&
                 r.U8(kind) && lamp.range > 0.0f && lamp.intensity >= 0.0f && lamp.worlds >= 1 && lamp.worlds <= 3 &&
                 kind <= 4 && std::isfinite(lamp.at.x) && std::isfinite(lamp.at.y) && std::isfinite(lamp.at.z);
            lamp.kind = static_cast<LampKind>(kind);
            lamps.push_back(lamp);
        }
        std::uint16_t pnx = 0;
        std::uint16_t pnz = 0;
        ok = ok && r.F32(plan.cell) && r.F32(plan.x0) && r.F32(plan.z0) && r.U16(pnx) && r.U16(pnz) &&
             plan.cell > 0.0f && pnx > 0 && pnz > 0 && pnx <= 1024 && pnz <= 1024;
        if (ok)
        {
            plan.nx = pnx;
            plan.nz = pnz;
            const std::size_t cells = static_cast<std::size_t>(pnx) * static_cast<std::size_t>(pnz);
            for (int w = 0; w < 2 && ok; ++w)
            {
                plan.sheet[w].resize(cells);
                ok = r.Bytes(plan.sheet[w].data(), cells);
                for (std::size_t k = 0; ok && k < cells; ++k)
                {
                    ok = plan.sheet[w][k] < kPlanClassCount;
                }
            }
        }
        if (!ok)
        {
            error = "the town file is cut short or damaged";
            *this = TownData{};
            return false;
        }
        if (r.Left() != 0)
        {
            error = "the town file has bytes after its end";
            *this = TownData{};
            return false;
        }
        return true;
    }

    /// True when polygon p is in `layer` (0 A, 1 B).
    static bool In(const Polygon& p, int layer)
    {
        return (p.worlds & (1u << layer)) != 0u;
    }

    /// The middle of the signal mast (the mean of its lamps); false when the town has none.
    bool MastAt(Vec3& at) const
    {
        if (emblems.empty())
        {
            return false;
        }
        at = {};
        for (const Emblem& e : emblems)
        {
            at.x += e.at.x / static_cast<float>(emblems.size());
            at.y += e.at.y / static_cast<float>(emblems.size());
            at.z += e.at.z / static_cast<float>(emblems.size());
        }
        return true;
    }

    /// The name of the station of the main path nearest to (x, z) on the ground plane, or "" when there is none.
    const char* NearestPlace(float x, float z) const
    {
        const Place* best = nullptr;
        float bestD2 = 0.0f;
        for (const Place& p : places)
        {
            const float d2 = (p.at.x - x) * (p.at.x - x) + (p.at.z - z) * (p.at.z - z);
            if (best == nullptr || d2 < bestD2)
            {
                best = &p;
                bestD2 = d2;
            }
        }
        return best != nullptr ? best->name.c_str() : "";
    }

    /// The solid of `layer`: its solid and terrain polygons, as triangles in a grid.
    TriMesh BuildMesh(int layer) const
    {
        TriMesh mesh;
        for (const Polygon& p : polygons)
        {
            if (!In(p, layer) || (p.kind != Kind::Solid && p.kind != Kind::Terrain))
            {
                continue;
            }
            const Vec3& a = vertices[p.first];
            for (int k = 1; k + 1 < p.count; ++k)
            {
                mesh.Add(a, vertices[p.first + static_cast<std::uint32_t>(k)],
                         vertices[p.first + static_cast<std::uint32_t>(k + 1)]);
            }
        }
        mesh.Build();
        return mesh;
    }

    /// True for the materials that are painted by hand in the style of the light (docs/DESIGN.md 6.9): the stone of the
    /// walls and the streets, the plaster, the clay tile, the ground and the rubble. Wood, cloth, paint, metal, light,
    /// water and the crowns of the trees keep their own colour. Named by prefix, so that a new plaster or a new tile is
    /// painted without a change here.
    static bool Painted(const std::string& name)
    {
        static const char* const kPrefixes[] = {"stone", "rock",    "paving", "sand",  "plaster",
                                                "roof",  "chimney", "grass",  "field", "debris"};
        for (const char* prefix : kPrefixes)
        {
            if (name.rfind(prefix, 0) == 0)
            {
                return true;
            }
        }
        return false;
    }

    /// The drawn town of `layer`: every polygon of that moment but its water, in one batch.
    BatchMesh BakeLayer(int layer) const
    {
        BatchMesh mesh;
        for (const Polygon& p : polygons)
        {
            if (In(p, layer) && p.kind != Kind::Water)
            {
                AddPolygon(mesh, p, layer, false);
            }
        }
        return mesh;
    }

    /// The water of `layer` (in the model: A only), drawn in the water pass of the renderer.
    BatchMesh BakeWater(int layer) const
    {
        BatchMesh mesh;
        for (const Polygon& p : polygons)
        {
            if (In(p, layer) && p.kind == Kind::Water)
            {
                AddPolygon(mesh, p, layer, true);
            }
        }
        return mesh;
    }

    /// True when a body of `layer` with its feet at `feet` is in the water: under a water polygon (seen from above) by
    /// more than `depth` metres.
    bool Submerged(const Vec3& feet, int layer, float depth = 0.3f) const
    {
        return SubmergedIn(feet, layer, depth);
    }

    /// The same in the town with the circle of the lantern: inside it, in B, the water of A is water (scene/Moments.h).
    bool Submerged(const Vec3& feet, int layer, const Circle& circle, float depth = 0.3f) const
    {
        return SubmergedIn(feet, layer == 1 && circle.Contains(feet) ? 0 : layer, depth);
    }

  private:
    bool SubmergedIn(const Vec3& feet, int layer, float depth) const
    {
        for (const std::uint32_t i : waters)
        {
            const Polygon& p = polygons[i];
            if (!In(p, layer))
            {
                continue;
            }
            float top = 0.0f;
            if (Over(p, feet.x, feet.z, top) && feet.y < top - depth && feet.y > top - 30.0f)
            {
                return true;
            }
        }
        return false;
    }

    struct Reader
    {
        const unsigned char* data;
        std::size_t size;
        std::size_t at = 0;

        bool Bytes(void* out, std::size_t n)
        {
            if (size - at < n)
            {
                return false;
            }
            std::memcpy(out, data + at, n);
            at += n;
            return true;
        }
        bool U8(std::uint8_t& v)
        {
            return Bytes(&v, 1);
        }
        bool U16(std::uint16_t& v)
        {
            unsigned char b[2];
            if (!Bytes(b, 2))
            {
                return false;
            }
            v = static_cast<std::uint16_t>(b[0] | (b[1] << 8));
            return true;
        }
        bool U32(std::uint32_t& v)
        {
            unsigned char b[4];
            if (!Bytes(b, 4))
            {
                return false;
            }
            v = static_cast<std::uint32_t>(b[0]) | (static_cast<std::uint32_t>(b[1]) << 8) |
                (static_cast<std::uint32_t>(b[2]) << 16) | (static_cast<std::uint32_t>(b[3]) << 24);
            return true;
        }
        bool F32(float& v)
        {
            std::uint32_t u = 0;
            if (!U32(u))
            {
                return false;
            }
            std::memcpy(&v, &u, 4); // WHY memcpy: the bits of a little-endian float32, without breaking aliasing
            return true;
        }
        bool Floats(float* v, int n)
        {
            for (int i = 0; i < n; ++i)
            {
                if (!F32(v[i]))
                {
                    return false;
                }
            }
            return true;
        }
        bool Name(std::string& s)
        {
            std::uint8_t n = 0;
            if (!U8(n) || size - at < n)
            {
                return false;
            }
            s.assign(reinterpret_cast<const char*>(data + at), n);
            at += n;
            return true;
        }
        std::size_t Left() const
        {
            return size - at;
        }
    };

    /// True when (x, z) lies inside polygon p seen from above; `top` is its height there (a fan from its first vertex).
    bool Over(const Polygon& p, float x, float z, float& top) const
    {
        const Vec3& a = vertices[p.first];
        for (int k = 1; k + 1 < p.count; ++k)
        {
            const Vec3& b = vertices[p.first + static_cast<std::uint32_t>(k)];
            const Vec3& c = vertices[p.first + static_cast<std::uint32_t>(k + 1)];
            const float d = (b.z - c.z) * (a.x - c.x) + (c.x - b.x) * (a.z - c.z);
            if (std::fabs(d) < 1.0e-9f)
            {
                continue;
            }
            const float u = ((b.z - c.z) * (x - c.x) + (c.x - b.x) * (z - c.z)) / d;
            const float v = ((c.z - a.z) * (x - c.x) + (a.x - c.x) * (z - c.z)) / d;
            if (u >= 0.0f && v >= 0.0f && u + v <= 1.0f)
            {
                top = u * a.y + v * b.y + (1.0f - u - v) * c.y;
                return true;
            }
        }
        return false;
    }

    /// The vertices of one side of polygon p, all with the normal n, and its fan of triangles: (0, k, k + 1) when
    /// `forward`, (0, k + 1, k) for the other side.
    void AddSide(BatchMesh& mesh, const Polygon& p, const Vec3& n, bool forward, const float colour[4],
                 const float params[4]) const
    {
        const std::uint32_t first = static_cast<std::uint32_t>(mesh.vertices.size());
        for (int k = 0; k < p.count; ++k)
        {
            const Vec3& q = vertices[p.first + static_cast<std::uint32_t>(k)];
            BatchVertex v{};
            v.position[0] = q.x;
            v.position[1] = q.y;
            v.position[2] = q.z;
            v.normal[0] = n.x;
            v.normal[1] = n.y;
            v.normal[2] = n.z;
            for (int c = 0; c < 4; ++c)
            {
                v.color[c] = colour[c];
                v.params[c] = params[c];
            }
            mesh.vertices.push_back(v);
        }
        for (std::uint32_t k = 1; k + 1 < p.count; ++k)
        {
            mesh.indices.push_back(first);
            mesh.indices.push_back(first + (forward ? k : k + 1u));
            mesh.indices.push_back(first + (forward ? k + 1u : k));
        }
    }

    /// The normal of polygon p that faces the side its fan (0, k, k + 1) shows as front (Newell's method: right for a
    /// polygon that is not quite flat, as a cell of the ground). Zero for a polygon with no area.
    Vec3 FanNormal(const Polygon& p) const
    {
        Vec3 n{0.0f, 0.0f, 0.0f};
        for (int k = 0; k < p.count; ++k)
        {
            const Vec3& a = vertices[p.first + static_cast<std::uint32_t>(k)];
            const Vec3& b = vertices[p.first + static_cast<std::uint32_t>((k + 1) % p.count)];
            n.x += (a.y - b.y) * (a.z + b.z);
            n.y += (a.z - b.z) * (a.x + b.x);
            n.z += (a.x - b.x) * (a.y + b.y);
        }
        return n; // along (b - a) x (c - a) of its fan triangles, the side their order shows as front
                  // (tests/test_town.cpp)
    }

    void AddPolygon(BatchMesh& mesh, const Polygon& p, int layer, bool water) const
    {
        const Material& m = materials[p.material];
        const float* srgb = layer == 0 ? m.a : m.b;
        const bool shines = layer == 0 ? m.shinesA : m.shinesB;
        // its own colour (a = 0: no stone tint; a = 1: painted by hand, see Painted); a shining material glows; water
        // is drawn as water
        const float colour[4] = {batch_detail::SrgbToLinear(srgb[0]), batch_detail::SrgbToLinear(srgb[1]),
                                 batch_detail::SrgbToLinear(srgb[2]), (!water && Painted(m.name)) ? 1.0f : 0.0f};
        float params[4] = {0.0f, 0.04f, 8.0f, shines ? 0.9f : 0.0f};
        if (water)
        {
            params[0] = scene::kMatWater;
            params[1] = 0.9f;
            params[2] = 90.0f;
            params[3] = 0.85f; // (water: how opaque)
        }
        Vec3 n = FanNormal(p);
        const float length = std::sqrt(tri::Dot(n, n));
        if (!(length > 1.0e-9f))
        {
            return;
        }
        n = {n.x / length, n.y / length, n.z / length};
        const Vec3 back{-n.x, -n.y, -n.z};
        if ((water && std::fabs(n.y) > 0.3f) ||
            p.kind == Kind::Terrain) // a surface seen from above only: the side that faces up
        {
            if (n.y >= 0.0f)
            {
                AddSide(mesh, p, n, true, colour, params);
            }
            else
            {
                AddSide(mesh, p, back, false, colour, params);
            }
        }
        else // both sides: the model's polygons have no fixed winding (and a fall of water is seen from both)
        {
            AddSide(mesh, p, n, true, colour, params);
            AddSide(mesh, p, back, false, colour, params);
        }
        ++mesh.objects;
    }
};

} // namespace dl::town
