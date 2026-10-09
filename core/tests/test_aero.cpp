// WP-21 aerodynamics (numeric part): unit tests for what the three gates (golden.aero_forces, capture.aero_forces_mono,
// capture.aero_forces_vessel) do not reach: the float policies against ksp/unity and the native Quaternion.Inverse capture,
// DragCubeList's weight / flag / query API and its occlusion branches, FlightIntegratorAero (drag models, areas,
// UpdateAerodynamics incl. forces, angular drag, ocean, body lift), the lifting / control / aero surface modules and the
// thermal-occlusion geometry. Expected values come from the decompiled C# statements (cited) evaluated by hand or with plain
// IEEE float32 arithmetic where the statement is a single operation.
#include <algorithm>
#include <concepts>
#include <type_traits>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/aero/flight_integrator_aero.hpp"
#include "ksp/aero/float_eval.hpp"
#include "ksp/aero/lifting_surface.hpp"
#include "ksp/aero/occlusion.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using aero::FE;
using aero::MonoFloat;
using aero::NetFloat;

namespace {

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
float FromHex(const nlohmann::json& v) {
    const uint32_t u = static_cast<uint32_t>(std::stoul(v.get<std::string>(), nullptr, 16));
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}
bool Same(const Vector3f& a, const Vector3f& b) { return Bits(a.x) == Bits(b.x) && Bits(a.y) == Bits(b.y) && Bits(a.z) == Bits(b.z); }
bool Same(const Quaternionf& a, const Quaternionf& b) {
    return Bits(a.x) == Bits(b.x) && Bits(a.y) == Bits(b.y) && Bits(a.z) == Bits(b.z) && Bits(a.w) == Bits(b.w);
}

const std::string kHarness = KSPCORE_HARNESS_DIR;

// PhysicsGlobals (harness/inputs/Physics.cfg = the game's Physics.cfg), the Planetarium and the Kerbol system
struct AeroWorld {
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    AeroWorld() {
        pg.physicsDatabaseFilename = kHarness + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        pg.bodyLiftCurve = PhysicsGlobals::GetLiftingSurfaceCurve("BodyLift");  // PhysicsGlobals.Start
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
    }
    ~AeroWorld() {
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
        SaveEnvironment::Reset();
    }
    CelestialBody& Kerbin() { return *sys->GetBodyByName("Kerbin"); }
};

// a cube line: name, 6 x (area, drag, depth), center, size
std::string Cube(const std::string& name, const float a[6], const float d[6], const float dep[6]) {
    std::string s = name;
    for (int i = 0; i < 6; i++)
        s += ", " + dotnet::FormatSingleG(a[i], 0) + "," + dotnet::FormatSingleG(d[i], 0) + "," + dotnet::FormatSingleG(dep[i], 0);
    return s + ", 0,0,0, 2,2,2";
}

std::unique_ptr<Part> CubePart(const std::vector<std::string>& cubes) {
    auto p = std::make_unique<Part>();
    ConfigNode n("DRAG_CUBE");
    for (const std::string& c : cubes) n.AddValue("cube", c);
    REQUIRE(p->DragCubes.LoadCubes(n));
    p->DragCubes.SetPart(p.get());
    return p;
}

uint32_t g_seed = 12345u;
float Rnd(float lo, float hi) {
    g_seed = g_seed * 1664525u + 1013904223u;
    return lo + (hi - lo) * static_cast<float>((g_seed >> 8) & 0xFFFFFF) / 16777216.0f;
}

std::shared_ptr<PartModule> NewModule(const char* cls, Part* part) {
    const ModuleClassInfo* c = GetPartModuleClass(cls);
    REQUIRE(c != nullptr);
    auto m = ModuleFactory::Create(*c);
    m->part = part;
    m->Awake();
    return m;
}

}  // namespace

// ---- float policies ------------------------------------------------------------------------------------------------------

// FE<P> must not mix with a bare float (porting error) or be built from a double
template <class A, class B>
concept Multipliable = requires(A a, B b) { a * b; };
template <class A, class B>
concept Addable = requires(A a, B b) { a + b; };
static_assert(!std::is_constructible_v<FE<MonoFloat>, double>);
static_assert(!Multipliable<FE<MonoFloat>, float>);
static_assert(!Addable<float, FE<NetFloat>>);
static_assert(Multipliable<FE<MonoFloat>, FE<MonoFloat>>);
static_assert(std::is_same_v<decltype(FE<MonoFloat>(1.0f) * 2.0), double>);

TEST_CASE("WP-21 float policies: MonoFloat is ksp::unity's Mono rule, NetFloat rounds every operation") {
    AeroWorld w;
    int netDiffers = 0;
    for (int i = 0; i < 20000; i++) {
        const Vector3f a(Rnd(-10, 10), Rnd(-10, 10), Rnd(-10, 10)), b(Rnd(-10, 10), Rnd(-10, 10), Rnd(-10, 10));
        const float t = Rnd(-0.5f, 1.5f);
        const Quaternionf q = Quaternionf(Rnd(-1, 1), Rnd(-1, 1), Rnd(-1, 1), Rnd(-1, 1)).normalized();
        const Quaternionf r = Quaternionf(Rnd(-1, 1), Rnd(-1, 1), Rnd(-1, 1), Rnd(-1, 1)).normalized();
        using UM = aero::U<MonoFloat>;
        REQUIRE(Bits(UM::Dot(a, b)) == Bits(Vector3f::Dot(a, b)));
        REQUIRE(Bits(UM::SqrMagnitude(a)) == Bits(a.sqrMagnitude()));
        REQUIRE(Bits(UM::Magnitude(a)) == Bits(a.magnitude()));
        REQUIRE(Same(UM::Normalize(a), a.normalized()));
        REQUIRE(Same(UM::ProjectOnPlane(a, b), Vector3f::ProjectOnPlane(a, b)));
        REQUIRE(Bits(UM::Angle(a, b)) == Bits(Vector3f::Angle(a, b)));
        REQUIRE(Bits(UM::Lerp(a.x, b.x, t)) == Bits(Mathf::Lerp(a.x, b.x, t)));
        REQUIRE(Same(UM::Mul(q, r), q * r));
        REQUIRE(Same(UM::Rotate(q, a), q * a));
        // .NET: Vector3.Dot = ((ax*bx + ay*by) + az*bz) with a float32 rounding after each operation
        const float net = ((a.x * b.x) + (a.y * b.y)) + (a.z * b.z);
        REQUIRE(Bits(aero::U<NetFloat>::Dot(a, b)) == Bits(net));
        if (Bits(net) != Bits(UM::Dot(a, b))) ++netDiffers;
        // DragCurveValue under Mono is WP-09's PhysicsGlobals::DragCurveValue (capture.physics_drag_curve)
        const float dn = Rnd(0, 1), mach = Rnd(0, 30);
        REQUIRE(Bits(aero::DragCurveValue<MonoFloat>(PhysicsGlobals::SurfaceCurves, dn, mach)) ==
                Bits(PhysicsGlobals::DragCurveValue(PhysicsGlobals::SurfaceCurves, dn, mach)));
    }
    CHECK(netDiffers > 1000);  // the two runtimes really differ on these inputs
    // FE: float op double promotes to double (both runtimes); exact product under Mono, rounded product under .NET
    const float x = 0.1f, y = 0.3f;
    CHECK(aero::D(FE<MonoFloat>(x) * FE<MonoFloat>(y)) == static_cast<double>(x) * static_cast<double>(y));
    CHECK(aero::D(FE<NetFloat>(x) * FE<NetFloat>(y)) == static_cast<double>(x * y));
    CHECK(Bits(aero::S(-FE<MonoFloat>(0.0f))) == 0x80000000u);  // IL neg
}

TEST_CASE("WP-21 Quaternion.Inverse (native) is the conjugate: 60/60 rows of the in-game capture (C3)") {
    const std::string path = kHarness + "/captures/out/math_shim_native.json";
    const auto text = dotnet::ReadAllText(path);
    REQUIRE_MESSAGE(text.has_value(), "missing " << path);
    const auto j = nlohmann::json::parse(*text);
    int rows = 0, bad = 0;
    for (const auto& t : j.at("tables")) {
        if (t.at("name") != "f32") continue;
        std::vector<std::string> cols;
        for (const auto& c : t.at("columns")) cols.push_back(c.at("name").get<std::string>());
        auto idx = [&](const char* n) { return static_cast<size_t>(std::find(cols.begin(), cols.end(), n) - cols.begin()); };
        const size_t fn = idx("fn");
        const size_t a[4] = {idx("a0_hex"), idx("a1_hex"), idx("a2_hex"), idx("a3_hex")};
        const size_t r[4] = {idx("r0_hex"), idx("r1_hex"), idx("r2_hex"), idx("r3_hex")};
        for (const auto& row : t.at("rows")) {
            if (row.at(fn) != "Quaternion.Inverse") continue;
            ++rows;
            const Quaternionf q(FromHex(row.at(a[0])), FromHex(row.at(a[1])), FromHex(row.at(a[2])), FromHex(row.at(a[3])));
            const Quaternionf want(FromHex(row.at(r[0])), FromHex(row.at(r[1])), FromHex(row.at(r[2])), FromHex(row.at(r[3])));
            if (!Same(aero::U<MonoFloat>::Inverse(q), want)) ++bad;
        }
    }
    CHECK(rows == 60);
    CHECK(bad == 0);
}

// ---- DragCubeList -----------------------------------------------------------------------------------------------------------

TEST_CASE("WP-21 DragCubeList: weights, update flags, cube lookups, copies, procedural cubes") {
    AeroWorld w;
    const float a1[6] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, d1[6] = {0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.2f}, p1[6] = {1, 1, 1, 1, 1, 1};
    const float a2[6] = {2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f}, d2[6] = {0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f}, p2[6] = {2, 2, 2, 2, 2, 2};
    auto part = CubePart({Cube("A", a1, d1, p1), Cube("B", a2, d2, p2)});
    DragCubeList& dcl = part->DragCubes;
    // LoadCubes reads the values last to first (DragCubeList.cs:898-912)
    REQUIRE(dcl.Cubes().size() == 2);
    CHECK(dcl.Cubes()[0].Name() == "B");
    CHECK(dcl.WeightsRequireUpdate());
    dcl.SetDragWeights();
    CHECK_FALSE(dcl.WeightsRequireUpdate());
    // SetDragWeights_List (:234): cubes from the last index down, weights 1 + 1, then * (1/2)
    for (int f = 0; f < 6; f++) {
        const float sum = (0.0f + a1[f] * 1.0f) + a2[f] * 1.0f;  // index 1 (A) first, then index 0 (B)
        CHECK(Bits(dcl.WeightedArea()[f]) == Bits(sum * 0.5f));
    }
    // a cube weight set to its current value does not request an update (:806)
    dcl.SetCubeWeight("A", 1.0f);
    CHECK_FALSE(dcl.WeightsRequireUpdate());
    dcl.SetCubeWeight("A", 0.0f);
    CHECK(dcl.WeightsRequireUpdate());
    CHECK(dcl.OcclusionRequireUpdate());
    dcl.SetDragWeights();
    for (int f = 0; f < 6; f++) CHECK(dcl.WeightedArea()[f] == a2[f]);
    dcl.ResetCubeWeights(0.0f);
    dcl.SetDragWeights();
    for (int f = 0; f < 6; f++) CHECK(dcl.WeightedArea()[f] == 0.0f);  // num == 0 branch (:263)
    dcl.ResetCubeWeights();
    CHECK(dcl.Cubes()[1].Weight() == 1.0f);
    // GetCube returns the last match, LoadCube copies only the named cubes and keeps `procedural`
    CHECK(dcl.GetCube("A") == &dcl.Cubes()[1]);
    CHECK(dcl.GetCube("C") == nullptr);
    DragCubeList other;
    other.SetProcedural(true);
    CHECK(other.LoadCube(dcl, "A"));
    CHECK(other.Cubes().size() == 1);
    CHECK(other.Procedural());
    CHECK_FALSE(other.LoadCube(DragCubeList{}, "A"));
    // Instantiate copies the serialized members only (:66-84)
    dcl.SetDragWeights();
    dcl.SetPartOcclusion();
    dcl.SetDrag(Vector3f(0.0f, -1.0f, 0.0f), 0.5f);
    dcl.SetOcclusionMultiplier(0.5f);
    DragCubeList copy;
    copy.CopySerializedFrom(dcl);
    CHECK(copy.Cubes().size() == 2);
    CHECK(copy.PostOcclusionArea() == dcl.PostOcclusionArea());
    CHECK(Bits(copy.AreaDrag()) == Bits(dcl.AreaDrag()));
    CHECK(copy.GetPart() == nullptr);
    CHECK(copy.WeightsRequireUpdate());
    CHECK(copy.OcclusionMultiplier() == 1.0f);
    CHECK(copy.WeightedArea()[2] == 0.0f);
    CHECK(copy.DragCurveCd == nullptr);
    // Part::Instantiate uses it and Part::Awake calls SetPart (Part.cs:1633)
    auto clone = Part::Instantiate(*part);
    CHECK(clone->DragCubes.GetPart() == clone.get());
    CHECK(clone->DragCubes.Cubes().size() == 2);
    // None: SetDragWeights / SetPartOcclusion / SetDrag do nothing
    DragCubeList none;
    none.SetNone(true);
    none.SetPart(part.get());
    none.SetDragWeights();
    none.SetPartOcclusion();
    none.SetDrag(Vector3f(1.0f, 0.0f, 0.0f), 1.0f);
    CHECK(none.WeightsRequireUpdate());
    CHECK(none.DragVector().Equals(Vector3f::zero()));
    // procedural cubes: without the DragCubeSystem hooks nothing is rendered; with them the cube is rendered once per interval
    auto proc = CubePart({});
    proc->DragCubes.SetProcedural(true);
    proc->DragCubes.SetDragWeights();
    CHECK(proc->DragCubes.WeightsRequireUpdate());
    float now = 1.0f;
    int renders = 0;
    DragCubeList::hooks.realtimeSinceStartup = [&] { return now; };
    DragCubeList::hooks.proceduralDragUpdateInterval = [] { return 0.25f; };
    DragCubeList::hooks.renderProceduralDragCube = [&](Part&) {
        ++renders;
        DragCube c("PROC");
        c.Area().fill(3.0f);
        c.SetSize(Vector3f(1.0f, 2.0f, 3.0f));
        return c;
    };
    proc->DragCubes.SetDragWeights();
    CHECK(renders == 1);
    CHECK(proc->DragCubes.WeightedArea()[4] == 3.0f);
    CHECK(proc->DragCubes.WeightedSize().Equals(Vector3f(1.0f, 2.0f, 3.0f)));
    proc->DragCubes.ForceUpdate(true, true);
    now = 1.1f;  // before proceduralDragNextUpdate (1.25)
    proc->DragCubes.SetDragWeights();
    CHECK(renders == 1);
    CHECK(proc->DragCubes.WeightsRequireUpdate());
    proc->DragCubes.ForceUpdate(true, true, true);  // resetProcTiming: next update at once
    proc->DragCubes.SetDragWeights();
    CHECK(renders == 2);
    DragCubeList::hooks = DragCubeList::Hooks{};
    // faces and per-direction queries (:421-492)
    CHECK(DragCubeList::GetFaceDirection(DragCube::DragFace::ZN).Equals(Vector3f::back()));
    CHECK(DragCubeList::FaceDirections()[3].Equals(Vector3f::down()));
    dcl.ResetCubeWeights(1.0f);
    dcl.SetDragWeights();
    dcl.SetPartOcclusion();
    CHECK(dcl.GetCubeAreaDir(Vector3f::up()) == dcl.AreaOccluded()[2]);
    CHECK(dcl.GetCubeCoeffDir(Vector3f::up()) == dcl.WeightedDrag()[2]);
    CHECK(dcl.GetCubeDepthDir(Vector3f::down()) == dcl.WeightedDepth()[3]);
    CHECK(std::isnan(dcl.GetCubeCoeffDir(Vector3f::zero())));  // 0 / 0 (no face faces the direction)
}

TEST_CASE("WP-21 DragCubeList.SetPartOcclusion over attach nodes (DragCubeList.cs:301, :739, :750)") {
    AeroWorld w;
    const float aTop[6] = {2.0f, 2.0f, 1.5f, 1.25f, 2.0f, 2.0f}, dTop[6] = {0.7f, 0.7f, 0.9f, 0.8f, 0.7f, 0.7f}, p[6] = {1, 1, 1, 1, 1, 1};
    const float aBot[6] = {3.0f, 3.0f, 1.0f, 1.0f, 3.0f, 3.0f}, dBot[6] = {0.6f, 0.6f, 0.95f, 0.95f, 0.6f, 0.6f};
    auto top = CubePart({Cube("Default", aTop, dTop, p)});
    auto bot = CubePart({Cube("Default", aBot, dBot, p)});
    AttachNode nb;
    nb.id = "bottom";
    nb.orientation = Vector3f(0.0f, -1.0f, 0.0f);
    nb.attachedPart = bot.get();
    top->attachNodes.push_back(nb);
    AttachNode nt;
    nt.id = "top";
    nt.orientation = Vector3f(0.0f, 1.0f, 0.0f);
    nt.attachedPart = top.get();
    bot->attachNodes.push_back(nt);
    for (Part* q : {top.get(), bot.get()}) q->DragCubes.SetDragWeights();
    for (Part* q : {top.get(), bot.get()}) q->DragCubes.SetPartOcclusion();
    // top: facing area of the bottom part toward +Y is its YP area (1.0); the top's YN face (1.25) keeps 0.25
    const DragCubeList& t = top->DragCubes;
    CHECK(top->attachNodes[0].contactArea == aBot[2]);
    CHECK(Bits(t.AreaOccluded()[3]) == Bits(1.25f - 1.0f));
    const float occArea = 1.25f - t.AreaOccluded()[3];  // weightedArea - areaOccluded
    CHECK(Bits(t.WeightedDrag()[3]) ==
          Bits(f32store((fx(dTop[3]) * fx(aTop[3]) - fx(occArea)) / fx(t.AreaOccluded()[3]))));
    for (int f : {0, 1, 2, 4, 5}) CHECK(t.AreaOccluded()[f] == aTop[f]);
    CHECK(t.PostOcclusionArea() == 2.0f + 2.0f + 1.5f + 0.25f + 2.0f + 2.0f);
    // bottom: the top's YN area (1.25) covers its whole YP face (1.0): occluded area 0 -> weightedDrag 1e-5
    const DragCubeList& b = bot->DragCubes;
    CHECK(bot->attachNodes[0].contactArea == 1.25f);
    CHECK(b.AreaOccluded()[2] == 0.0f);
    CHECK(b.WeightedDrag()[2] == 1E-05f);
    // overrideDragArea replaces the facing area; occlusionMultiplier scales it (parachutes use 0)
    top->attachNodes[0].overrideDragArea = 0.5f;
    top->DragCubes.ForceUpdate(false, true);
    CHECK(bot->DragCubes.OcclusionRequireUpdate());  // ForceUpdate flags the attached CUBE parts too (:967)
    top->DragCubes.SetPartOcclusion();
    CHECK(top->attachNodes[0].contactArea == 0.5f);
    CHECK(t.AreaOccluded()[3] == 0.75f);
    top->attachNodes[0].overrideDragArea = -1.0f;
    top->DragCubes.SetOcclusionMultiplier(0.0f);
    top->DragCubes.SetPartOcclusion();
    CHECK(t.AreaOccluded()[3] == 1.25f);
    CHECK(Bits(t.WeightedDrag()[3]) == Bits(f32store((fx(dTop[3]) * fx(1.25f) - 0.0) / fx(1.25f))));
    CHECK(top->attachNodes[0].contactArea == 1.0f);  // the facing area itself, before the multiplier
    // a neighbour that is not a CUBE part does not occlude
    bot->fields.dragModel = 3;  // SPHERICAL
    top->DragCubes.SetOcclusionMultiplier(1.0f);
    top->DragCubes.SetPartOcclusion();
    CHECK(t.AreaOccluded()[3] == 1.25f);
    bot->fields.dragModel = 4;
    // a rotated neighbour: the node direction is taken into the neighbour's frame (Inverse(rA) * rB * n)
    bot->partTransform.rotation = UnityNative::AngleAxis(90.0f, Vector3f::forward());  // bottom's +X now points along world +Y
    top->DragCubes.RequestOcclusionUpdate();
    top->DragCubes.SetPartOcclusion();
    const Vector3f vInBot = (aero::U<MonoFloat>::Inverse(bot->partTransform.rotation) * top->partTransform.rotation) * Vector3f::down();
    float facing = 0.0f;
    for (int f = 0; f < 6; f++)
        facing = f32store(fx(facing) + fx(aBot[f]) * fx(Mathf::Clamp01(Vector3f::Dot(-vInBot, DragCubeList::FaceDirections()[f]))));
    CHECK(Bits(top->attachNodes[0].contactArea) == Bits(facing));
    CHECK(facing > 2.9f);  // mostly the bottom's XP face (3.0)
    // surface attachment: the srfAttach node's contact area is the part's own facing area along the node (:344)
    top->srfAttachNode = std::make_shared<AttachNode>();
    top->srfAttachNode->orientation = Vector3f(1.0f, 0.0f, 0.0f);
    top->srfAttachNode->attachedPart = bot.get();
    top->DragCubes.RequestOcclusionUpdate();
    top->DragCubes.SetPartOcclusion();
    CHECK(top->srfAttachNode->contactArea == aTop[0]);
}

TEST_CASE("WP-21 DragCubeList.SetDrag / AddSurfaceDragDirection branches (DragCubeList.cs:352-419)") {
    AeroWorld w;
    const float a[6] = {1.0f, 1.0f, 2.0f, 2.0f, 1.0f, 1.0f}, d[6] = {1.2f, 1.2f, 0.9f, 1.5f, 1.2f, 1.2f}, p[6] = {0.5f, 0.5f, 1.0f, 3.0f, 0.5f, 0.5f};
    auto part = CubePart({Cube("Default", a, d, p)});
    DragCubeList& dcl = part->DragCubes;
    dcl.SetDragWeights();
    dcl.SetPartOcclusion();
    // flow from below (SetDrag(vector) works on -vector): only YP faces the direction (0, 1, 0)
    dcl.SetDrag(Vector3f(0.0f, -1.0f, 0.0f), 0.3f);
    CHECK(dcl.DragVector().Equals(Vector3f(0.0f, 1.0f, 0.0f)));
    CHECK(dcl.CrossSectionalArea() == 2.0f);  // areaOccluded[YP] * Clamp01(1)
    CHECK(dcl.Depth() == 1.0f);               // depth of YP, normalised by the dot sum 1
    CHECK(dcl.TaperDot() == f32store(1.0 / fx(0.9f)));  // weightedDrag 0.9 < 1 and > 0.01 -> 1 / 0.9
    // weightedDrag >= 1 on every face: no Cd curve, no taper division
    const float d2[6] = {1.2f, 1.2f, 1.1f, 1.5f, 1.2f, 1.2f};
    auto blunt = CubePart({Cube("Default", a, d2, p)});
    blunt->DragCubes.SetDragWeights();
    blunt->DragCubes.SetPartOcclusion();
    blunt->DragCubes.SetDrag(Vector3f(0.0f, -1.0f, 0.0f), 0.3f);
    CHECK(blunt->DragCubes.TaperDot() == 1.0f);
    // zero areas: dragCoeff = areaDrag = 0 (:413)
    const float z[6] = {0, 0, 0, 0, 0, 0};
    auto empty = CubePart({Cube("Default", z, d, p)});
    empty->DragCubes.SetDragWeights();
    empty->DragCubes.SetPartOcclusion();
    empty->DragCubes.SetDrag(Vector3f(0.3f, 0.9f, 0.1f).normalized(), 2.0f);
    CHECK(empty->DragCubes.Area() == 0.0f);
    CHECK(empty->DragCubes.AreaDrag() == 0.0f);
    CHECK(empty->DragCubes.DragCoeff() == 0.0f);
    // the drag-vector rotation (SetDragVectorRotation, :824) turns -vector before the face loop
    const Quaternionf rot = UnityNative::AngleAxis(90.0f, Vector3f::right());
    dcl.SetDragVectorRotation(rot);
    CHECK(dcl.RotateDragVector());
    dcl.SetDrag(Vector3f(0.0f, -1.0f, 0.0f), 0.3f);
    CHECK(Same(dcl.DragVector(), rot * Vector3f(0.0f, 1.0f, 0.0f)));
    dcl.SetDragVectorRotation(false);
    dcl.SetDrag(Vector3f(0.0f, -1.0f, 0.0f), 0.3f);
    CHECK(dcl.DragVector().Equals(Vector3f(0.0f, 1.0f, 0.0f)));
    // a part without PhysicsGlobals curves (SetPart before PhysicsGlobals exists) throws like the C# null reference
    DragCubeList bare;
    bare.LoadCubes(dcl);
    CHECK_THROWS_AS(bare.SetDrag(Vector3f::up(), 0.5f), NullReferenceException);
}

// ---- FlightIntegratorAero -----------------------------------------------------------------------------------------------------

TEST_CASE("WP-21 FlightIntegrator drag models and areas (FlightIntegrator.cs:2391-2510)") {
    AeroWorld w;
    const float a[6] = {1, 1, 2, 2, 1, 1}, d[6] = {0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f}, p[6] = {1, 1, 1, 1, 1, 1};
    auto part = CubePart({Cube("Default", a, d, p)});
    part->DragCubes.SetDragWeights();
    part->DragCubes.SetPartOcclusion();
    part->DragCubes.SetDrag(Vector3f::down(), 0.5f);
    FlightIntegratorAero fi;
    fi.CacheGlobals();
    CHECK(fi.cacheDragCubeMultiplier == 0.1f);
    CHECK(fi.cacheDragMultiplier == 8.0f);
    CHECK(fi.cacheBodyLiftMultiplier == 18.0f);
    PartFields& f = part->fields;
    f.maximum_drag = 0.3f;
    f.minimum_drag = 0.1f;
    // CUBE: AreaDrag * cacheDragCubeMultiplier as an exact double product (Mono)
    CHECK(fi.CalculateDragValue(*part) == static_cast<double>(part->DragCubes.AreaDrag()) * static_cast<double>(0.1f));
    CHECK(fi.CalculateDragValue<NetFloat>(*part) == static_cast<double>(part->DragCubes.AreaDrag() * 0.1f));
    f.dragModel = 0;  // DEFAULT: the cube value as well
    CHECK(fi.CalculateDragValue(*part) == fi.CalculateDragValue_Cube(*part));
    f.dragModel = 3;  // SPHERICAL
    CHECK(fi.CalculateDragValue(*part) == static_cast<double>(0.3f));
    f.dragModel = 2;  // CYLINDRICAL: Lerp(min, max, |dot(dragReferenceVector, dragVectorDir)|)
    f.dragVectorDir = Vector3f::up();
    CHECK(fi.CalculateDragValue(*part) == static_cast<double>(0.3f));
    f.dragVectorDir = Vector3f::right();
    CHECK(fi.CalculateDragValue(*part) == static_cast<double>(0.1f));
    f.dragModel = 1;  // CONIC: Lerp(min, max, angle / 180)
    f.dragVectorDir = Vector3f::down();
    CHECK(fi.CalculateDragValue(*part) == static_cast<double>(0.3f));
    f.dragVectorDir = Vector3f::right();
    CHECK(fi.CalculateDragValue(*part) == static_cast<double>(Mathf::Lerp(0.1f, 0.3f, 0.5f)));
    f.dragModel = 5;  // NONE
    CHECK(fi.CalculateDragValue(*part) == 0.0);
    f.dragModel = 4;
    fi.cacheDragCubesUseSpherical = true;
    CHECK(fi.CalculateDragValue(*part) == static_cast<double>(0.3f));
    fi.cacheDragCubesUseSpherical = false;
    CHECK(fi.CalculateAerodynamicArea(*part) == static_cast<double>(part->DragCubes.Area()));
    CHECK(fi.CalculateAreaExposed(*part) == static_cast<double>(part->DragCubes.ExposedArea()));
    CHECK(fi.CalculateAreaRadiative(*part) == static_cast<double>(part->DragCubes.PostOcclusionArea()));
    part->DragCubes.SetNone(true);
    for (int model = 0; model < 6; model++) {
        f.dragModel = model;
        CHECK(fi.CalculateAerodynamicArea(*part) == static_cast<double>(0.3f));
        CHECK(fi.CalculateAreaExposed(*part) == static_cast<double>(0.3f));
        CHECK(fi.CalculateAreaRadiative(*part) == static_cast<double>(0.3f));
    }
}

TEST_CASE("WP-21 FlightIntegrator.UpdateAerodynamics (FlightIntegrator.cs:2260): state, scalars and returned forces") {
    AeroWorld w;
    const float a[6] = {1, 1, 2, 2, 1, 1}, d[6] = {0.8f, 0.8f, 0.6f, 0.9f, 0.8f, 0.8f}, p[6] = {1, 1, 1, 1, 1, 1};
    auto part = CubePart({Cube("Default", a, d, p)});
    part->DragCubes.SetDragWeights();
    part->DragCubes.SetPartOcclusion();
    FlightIntegratorAero fi;
    fi.CacheGlobals();
    fi.currentMainBody = &w.Kerbin();
    fi.mach = 0.3;
    fi.pseudoReDragMult = 0.9;
    PartFields& f = part->fields;
    f.atmDensity = 1.2;
    f.staticPressureAtm = 1.0;
    f.CoPOffset = Vector3f(0.0f, 0.5f, 0.0f);
    part->partTransform.position = Vector3f(10.0f, 0.0f, 0.0f);
    AeroRigidbody rb;
    rb.present = true;
    rb.velocity = Vector3f(0.0f, 100.0f, 0.0f);
    AeroForces out = fi.UpdateAerodynamics(*part, rb);
    CHECK(f.dragVector.Equals(Vector3f(0.0f, 100.0f, 0.0f)));
    CHECK(f.dragVectorSqrMag == 10000.0f);
    CHECK(f.dragVectorMag == 100.0f);
    CHECK(f.dragVectorDir.Equals(Vector3f::up()));
    CHECK(f.dragVectorDirLocal.Equals(Vector3f::down()));
    CHECK(part->DragCubes.DragVector().Equals(Vector3f::up()));  // SetDrag(dragVectorDirLocal) works on its negation
    CHECK(f.dynamicPressurekPa == 1.2 * (0.0005 * 10000.0));     // Kerbin has an ocean: not submerged -> atmDensity * num5
    const double num6 = fi.CalculateDragValue(*part) * 0.9;
    CHECK(Bits(f.dragScalar) == Bits(f32store(fx(static_cast<float>(f.dynamicPressurekPa * num6 * 1.0)) * fx(8.0f))));
    CHECK(out.applyDrag);
    CHECK(Same(out.dragForce, -f.dragVectorDir * f.dragScalar));
    CHECK(Same(out.dragPosition, part->partTransform.TransformPoint(f.CoPOffset)));
    CHECK(out.setAngularDrag);
    CHECK(Bits(out.angularDrag) ==
          Bits(f32store(fx(f.angularDrag) * fx(static_cast<float>(f.dynamicPressurekPa * 0.009869232667160128)) * fx(2.0f))));
    // axial flow: no body lift force from the cube (LiftForce is zero)
    CHECK_FALSE(out.applyLift);
    // oblique flow: body lift from the cube's LiftForce, projected on the plane normal to the drag direction (ApplyAeroLift :2249)
    rb.velocity = Vector3f(30.0f, 100.0f, 0.0f);
    out = fi.UpdateAerodynamics(*part, rb);
    if (out.applyLift) {
        const Vector3f raw = part->partTransform.rotation * (f.bodyLiftScalar * part->DragCubes.LiftForce());
        CHECK(Same(out.liftForce, Vector3f::ProjectOnPlane(raw, -f.dragVectorDir)));
        CHECK(std::fabs(Vector3f::Dot(out.liftForce, f.dragVectorDir)) < 1e-4f);
    }
    // lift providers suppress body lift
    f.hasLiftModule = true;
    f.bodyLiftScalar = 123.0f;
    fi.UpdateAerodynamics(*part, rb);
    CHECK(f.bodyLiftScalar == 123.0f);
    f.hasLiftModule = false;
    // physicsless part on its parent's body: no drag unless ApplyDragToNonPhysicsParts
    AeroRigidbody parentRb = rb;
    parentRb.isPartRb = false;
    parentRb.partHasRb = false;
    fi.cacheApplyDragToNonPhysicsParts = false;
    out = fi.UpdateAerodynamics(*part, parentRb);
    CHECK_FALSE(out.applyDrag);
    CHECK(f.aerodynamicArea == static_cast<double>(part->DragCubes.Area()));
    // half submerged: num2 = 1 - submergedPortion, water pressure from the ocean density (:2306-2329)
    fi.cacheApplyDragToNonPhysicsParts = true;
    f.submergedPortion = 0.5;
    out = fi.UpdateAerodynamics(*part, rb);
    CHECK(f.submergedDynamicPressurekPa == w.Kerbin().oceanDensity * 1000.0 * (0.0005 * static_cast<double>(f.dragVectorSqrMag)));
    const double num6b = fi.CalculateDragValue(*part) * 0.9;
    CHECK(Bits(f.dragScalar) == Bits(f32store(fx(static_cast<float>(f.dynamicPressurekPa * num6b * 0.5)) * fx(8.0f))));
    f.submergedPortion = 0.0;
    // shielded, airless or zero velocity: early returns
    part->set_ShieldedFromAirstream(true);
    part->DragCubes.SetDrag(Vector3f::up(), 0.3f);  // marker
    out = fi.UpdateAerodynamics(*part, rb);
    CHECK(part->DragCubes.DragVector().Equals(Vector3f::down()));  // SetDrag was not called again
    CHECK_FALSE(out.applyDrag);
    part->set_ShieldedFromAirstream(false);
    f.atmDensity = 0.0;
    out = fi.UpdateAerodynamics(*part, rb);
    CHECK(f.aerodynamicArea == 0.0);
    f.atmDensity = 1.2;
    rb.velocity = Vector3f::zero();
    out = fi.UpdateAerodynamics(*part, rb);
    CHECK(f.dragVectorDir.Equals(Vector3f::zero()));
    CHECK_FALSE(out.applyDrag);  // flag3 false
    rb.present = false;
    f.exposedArea = -1.0;
    out = fi.UpdateAerodynamics(*part, rb);
    CHECK(f.exposedArea == -1.0);  // no rigidbody: nothing happens
}

TEST_CASE("WP-21 FlightIntegrator.DragCubeSetupAndPartAeroStats (FlightIntegrator.cs:2512)") {
    AeroWorld w;
    const float a[6] = {1, 1, 2, 2, 1, 1}, d[6] = {0.8f, 0.8f, 0.6f, 0.9f, 0.8f, 0.8f}, p[6] = {1, 1, 1, 1, 1, 1};
    auto part = CubePart({Cube("Default", a, d, p)});
    FlightIntegratorAero fi;
    fi.CacheGlobals();
    fi.currentMainBody = &w.Kerbin();
    fi.mach = 0.7;
    fi.density = 0.9;
    std::vector<Part*> parts = {part.get()};
    fi.DragCubeSetupAndPartAeroStats(parts, 9.81, [](const Part&) { return 1000.0; });
    CHECK_FALSE(part->DragCubes.WeightsRequireUpdate());
    CHECK_FALSE(part->DragCubes.OcclusionRequireUpdate());
    CHECK(part->fields.radiativeArea == static_cast<double>(part->DragCubes.PostOcclusionArea()));
    CHECK(part->fields.staticPressureAtm == w.Kerbin().GetPressure(1000.0) * 0.009869232667160128);
    CHECK(part->fields.atmDensity == 0.9);
    CHECK(part->fields.machNumber == 0.7);
    // below sea level: hydrostatic pressure of the ocean added before the kPa -> atm conversion
    fi.DragCubeSetupAndPartAeroStats(parts, 9.81, [](const Part&) { return -10.0; });
    CHECK(part->fields.staticPressureAtm ==
          (w.Kerbin().GetPressure(-10.0) + 9.81 * 10.0 * w.Kerbin().oceanDensity) * 0.009869232667160128);
}

// ---- lifting / control / aero surfaces ------------------------------------------------------------------------------------------

TEST_CASE("WP-21 ModuleLiftingSurface: OnStart curves, lift / drag vectors, FixedUpdate forces, CoL query") {
    AeroWorld w;
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    SaveEnvironment::Current().loadedSceneIsGame = true;
    Part part;
    part.Awake();
    auto m = NewModule("ModuleLiftingSurface", &part);
    auto* ls = dynamic_cast<ModuleLiftingSurface*>(m.get());
    REQUIRE(ls != nullptr);
    CHECK(ls->Get<float>("deflectionLiftCoeff") == 1.5f);
    ls->OnStart(StartState::Flying);
    const auto def = PhysicsGlobals::GetLiftingSurfaceCurve("Default");
    const aero::LiftingSurfaceParams prm = ls->Params();
    REQUIRE(prm.liftCurve != nullptr);
    CHECK(prm.liftCurve->Evaluate(0.3f) == def->liftCurve.Evaluate(0.3f));
    // GetLiftVector (:232): Sign(liftDot) * liftCurve(absDot) * liftMachCurve(mach) * coeff, times Q * LiftMultiplier
    const Vector3f liftVector = Vector3f::forward();
    const float liftDot = -0.2f, absDot = 0.2f;
    const Vector3f lift = ls->GetLiftVector(liftVector, liftDot, absDot, 5000.0, 0.5f);
    const float s1 = f32store(-1.0 * fx(def->liftCurve.Evaluate(0.2f)) * fx(def->liftMachCurve.Evaluate(0.5f)));
    const float s2 = f32store(fx(s1) * fx(1.5f));
    const float s3 = static_cast<float>(5000.0 * (fx(PhysicsGlobals::LiftMultiplier()) * fx(s2)));
    CHECK(Bits(ls->Get<float>("liftScalar")) == Bits(s3));
    CHECK(Same(lift, -liftVector * s3));
    CHECK(ls->IsLifting());
    // GetDragVector (:262)
    const Vector3f nVel = Vector3f(0.0f, 0.98f, -0.2f).normalized();
    const Vector3f drag = ls->GetDragVector(nVel, absDot, 5000.0, 0.5f);
    const float ds = static_cast<float>(
        5000.0 * (fx(f32store(fx(f32store(fx(def->dragCurve.Evaluate(0.2f)) * fx(def->dragMachCurve.Evaluate(0.5f)))) * fx(1.5f))) *
                  fx(PhysicsGlobals::LiftDragMultiplier())));
    CHECK(Bits(ls->Get<float>("dragScalar")) == Bits(ds));
    CHECK(Same(drag, -nVel * ds));
    // perpendicularOnly: the lift is projected on the plane normal to the velocity and liftScalar becomes its magnitude
    ls->Set<bool>("perpendicularOnly", true);
    ls->nVel = nVel;
    const Vector3f perp = ls->GetLiftVector(liftVector, liftDot, absDot, 5000.0, 0.5f);
    CHECK(std::fabs(Vector3f::Dot(perp, nVel)) < 1e-3f);
    CHECK(ls->Get<float>("liftScalar") == perp.magnitude());
    ls->Set<bool>("perpendicularOnly", false);
    // an attached node switches the surface off (nodeEnabled, :234)
    AttachNode node;
    node.id = "top";
    Part other;
    node.attachedPart = &other;
    part.attachNodes.push_back(node);
    ls->Set<bool>("nodeEnabled", true);
    ls->Set<std::string>("attachNodeName", std::string("top"));
    CHECK(ls->GetLiftVector(liftVector, liftDot, absDot, 5000.0, 0.5f).Equals(Vector3f::zero()));
    CHECK(ls->Get<float>("liftScalar") == 0.0f);
    CHECK_FALSE(ls->IsLifting());
    ls->Set<bool>("nodeEnabled", false);
    // SetupCoefficients (:205): one-sided surfaces clamp the dot
    Vector3f nv, lv;
    float ld = 0.0f, ad = 0.0f;
    LiftingSurfaceFrame frame;
    ls->Set<bool>("omnidirectional", false);
    ls->SetupCoefficients(Vector3f(0.0f, 50.0f, 10.0f), frame, nv, lv, ld, ad);
    CHECK(ld > 0.0f);
    CHECK(ad == ld);
    ls->SetupCoefficients(Vector3f(0.0f, 50.0f, -10.0f), frame, nv, lv, ld, ad);
    CHECK(ad == 0.0f);
    ls->Set<bool>("omnidirectional", true);
    // FixedUpdate (:170): Q in Pa from the part's dynamic pressure, forces at CoL / CoP
    part.fields.dynamicPressurekPa = 2.0;
    part.fields.machNumber = 0.4;
    frame.pointVelocity = Vector3f(0.0f, 100.0f, -17.0f);
    frame.colWorld = Vector3f(1.0f, 2.0f, 3.0f);
    frame.copWorld = Vector3f(4.0f, 5.0f, 6.0f);
    const SurfaceForces sf = ls->FixedUpdateAero(frame);
    CHECK(ls->Qlift == 2000.0);
    CHECK(sf.applied);
    CHECK(sf.applyDrag);
    CHECK(sf.liftPosition.Equals(frame.colWorld));
    CHECK(sf.dragPosition.Equals(frame.copWorld));
    CHECK(sf.liftForce.z > 0.0f);  // flow from +Z side of the wing normal pushes along -liftVector * Sign(dot) = +Z
    part.set_ShieldedFromAirstream(true);
    const SurfaceForces sh = ls->FixedUpdateAero(frame);
    CHECK_FALSE(sh.applied);
    CHECK(ls->Qlift == 0.0);
    part.set_ShieldedFromAirstream(false);
    // OnCenterOfLiftQuery (:369)
    CenterOfLiftQuery q;
    q.refVector = Vector3f(0.0f, 100.0f, -17.0f);
    q.refAirDensity = 1.2;
    q.refStaticPressure = 101.325;
    ls->OnCenterOfLiftQuery(q, frame, 340.0, Vector3f(7.0f, 8.0f, 9.0f));
    CHECK(q.pos.Equals(Vector3f(7.0f, 8.0f, 9.0f)));
    CHECK(q.lift > 0.0f);
    CHECK(std::fabs(q.dir.magnitude() - 1.0f) < 1e-6f);
    q.refVector = Vector3f(0.0f, 100.0f, 0.0f);  // absDot 0 < 0.001
    ls->OnCenterOfLiftQuery(q, frame, 340.0, Vector3f::zero());
    CHECK(q.lift == 1E-05f);
    CHECK(q.dir.Equals(Vector3f::zero()));
    q.Reset();
    CHECK(q.lift == 0.0f);
    CHECK(q.refAirDensity == 0.0);
    CenterOfThrustQuery tq;
    tq.thrust = 5.0f;
    tq.Reset();
    CHECK(tq.thrust == 0.0f);
}

TEST_CASE("WP-21 ModuleControlSurface / ModuleAeroSurface: deflection maths, cube weights, deploy (ModuleControlSurface.cs)") {
    AeroWorld w;
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    SaveEnvironment::Current().loadedSceneIsGame = true;
    const float a[6] = {1, 1, 1, 1, 1, 1}, d[6] = {0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f}, p[6] = {1, 1, 1, 1, 1, 1};
    auto part = CubePart({Cube("neutral", a, d, p), Cube("fullDeflectionPos", a, d, p), Cube("fullDeflectionNeg", a, d, p)});
    part->Awake();
    auto m = NewModule("ModuleControlSurface", part.get());
    auto* cs = dynamic_cast<ModuleControlSurface*>(m.get());
    REQUIRE(cs != nullptr);
    cs->OnStart(StartState::Flying);
    CHECK(cs->Get<float>("deployAngle") == 15.0f);  // NaN -> ctrlSurfaceRange (:276)
    CHECK(cs->deployAngleLimits.x == -22.5f);
    CHECK(cs->deployAngleLimits.y == 22.5f);
    CHECK(cs->Get<float>("authorityLimiterUI") == f32store(15.0 * 100.0 * fx(0.01f)));  // ModifyAuthorityLimiter (:255)
    cs->Set<float>("authorityLimiterUI", 7.5f);
    cs->ModifyAuthorityLimiterUI();
    CHECK(cs->Get<float>("authorityLimiter") == 50.0f);
    cs->Set<float>("authorityLimiter", 100.0f);
    CHECK(cs->originalActuatorSpeed == 25.0f);
    // FixedCtrlSurfaceUpdate (:704): pitch 1, surface hinge along +X, vessel CoM behind the surface (localCoM.y < 0 flips the action)
    ControlSurfaceFrame fr;
    fr.pitch = 1.0f;
    fr.localCoM = Vector3f(0.0f, -1.0f, 0.0f);
    fr.fixedDeltaTime = 0.02f;
    cs->FixedCtrlSurfaceUpdate(fr);
    CHECK(cs->action == -15.0f);
    CHECK(cs->deflection == -0.5f);  // MoveTowards by actuatorSpeed * dt = 0.5
    CHECK(Same(cs->ctrlSurfaceLocalRotation, UnityNative::AngleAxis(-0.5f, Vector3f::right()) * cs->neutral));
    for (int i = 0; i < 100; i++) cs->FixedCtrlSurfaceUpdate(fr);
    CHECK(cs->deflection == -15.0f);
    // deploy with mirror flags: currentDeployAngle = -(signs) * deployAngle added before the clamp to +-1.5 range
    cs->Set<bool>("deploy", true);
    cs->Set<bool>("usesMirrorDeploy", true);
    cs->Set<bool>("mirrorDeploy", true);
    cs->FixedCtrlSurfaceUpdate(fr);
    CHECK(cs->currentDeployAngle == 15.0f);  // -( 1 * 1 * -1) * 15
    CHECK(cs->action == 0.0f);               // -15 + 15
    cs->Set<bool>("deploy", false);
    // ActionSign quirk (:539): the negative test is `action < epsilon`
    CHECK(ModuleControlSurface::ActionSign(0.005f, 0.01f) == -1.0f);
    CHECK(ModuleControlSurface::ActionSign(0.01f, 0.01f) == 0.0f);
    CHECK(ModuleControlSurface::ActionSign(0.02f, 0.01f) == 1.0f);
    // FixedUpdate (:366): forces and the three cube weights from the deflection
    part->fields.dynamicPressurekPa = 3.0;
    part->fields.machNumber = 0.5;
    fr.surface.pointVelocity = Vector3f(0.0f, 100.0f, -10.0f);
    cs->deflection = -15.0f;
    const SurfaceForces sf = cs->FixedUpdateAero(fr);
    CHECK(sf.applied);
    const float num5 = f32store(fx(cs->deflection) / fx(15.0f));
    CHECK(part->DragCubes.GetCube("neutral")->Weight() == f32store(fx(150.0f) * fx(0.01f) - fx(Mathf::Abs(num5))));
    CHECK(part->DragCubes.GetCube("fullDeflectionPos")->Weight() == Mathf::Clamp01(num5));
    CHECK(part->DragCubes.GetCube("fullDeflectionNeg")->Weight() == Mathf::Clamp01(-num5));
    CHECK(part->DragCubes.WeightsRequireUpdate());
    // AssumeDragCubePosition (:896)
    cs->AssumeDragCubePosition("fullDeflectionPos");
    CHECK(cs->action == 1.5f);
    CHECK(Same(cs->ctrlSurfaceLocalRotation, UnityNative::AngleAxis(22.5f, Vector3f::right()) * cs->neutral));
    CHECK(cs->GetDragCubeNames().size() == 3);
    CHECK(cs->IsMultipleCubesActive());
    CHECK_FALSE(cs->UsesProceduralDragCubes());

    // ModuleAeroSurface (airbrake): OnStart overrides, CtrlSurfaceUpdate with deploy and the cased / uncased temperature
    Part brakePart;
    brakePart.Awake();
    auto mb = NewModule("ModuleAeroSurface", &brakePart);
    auto* as = dynamic_cast<ModuleAeroSurface*>(mb.get());
    REQUIRE(as != nullptr);
    as->Set<float>("authorityLimiter", 30.0f);
    as->OnStart(StartState::Flying);
    CHECK(as->Get<bool>("ignoreRoll"));
    CHECK(as->Get<float>("authorityLimiter") == 100.0f);
    CHECK(as->Get<float>("aeroDeployAngle") == as->Get<float>("ctrlSurfaceRange"));
    // sic: the initialiser is Vector2(NaN, 0f) (ModuleAeroSurface.cs:30), so the IsNaN(y) branch of OnStart never fires
    CHECK(as->aeroDeployAngleLimits.x == 0.0f);
    CHECK(as->aeroDeployAngleLimits.y == 0.0f);
    as->Set<bool>("deploy", true);
    as->Set<float>("uncasedTemp", 1200.0f);
    as->Set<float>("casedTemp", 2000.0f);
    ControlSurfaceFrame fb;
    fb.fixedDeltaTime = 0.02f;
    as->CtrlSurfaceUpdate(fb);
    CHECK(as->action == as->Get<float>("aeroDeployAngle"));
    CHECK(brakePart.fields.skinMaxTemp == 1200.0);
    as->Set<bool>("deploy", false);
    as->CtrlSurfaceUpdate(fb);
    CHECK(as->action == 0.0f);
    CHECK(brakePart.fields.skinMaxTemp == 2000.0);
}

// ---- thermal occlusion geometry ---------------------------------------------------------------------------------------------------

TEST_CASE("WP-21 OcclusionData / OcclusionCone / OcclusionCylinder geometry (OcclusionData.cs, OcclusionCone.cs)") {
    AeroWorld w;
    const double kPi = 3.141592653589793;
    // sA (:211), RectRectIntersection (:237), AreaOfIntersection (:247)
    CHECK(OcclusionData::sA(1.0, 0.5, 0.5) == 0.25);
    CHECK(OcclusionData::sA(1.0, -0.5, 0.5) == -0.25);
    CHECK(OcclusionData::sA(1.0, 2.0, 2.0) == doctest::Approx(kPi / 4.0).epsilon(1e-15));
    CHECK(OcclusionData::RectRectIntersection(1.0, 1.0, -0.5, 0.5, -0.5, 0.5) == 1.0);
    CHECK(OcclusionData::RectRectIntersection(1.0, 1.0, 0.0, 2.0, 0.0, 2.0) == 0.25);
    CHECK(OcclusionData::RectRectIntersection(1.0, 1.0, 2.0, 3.0, 0.0, 1.0) == 0.0);
    CHECK(OcclusionData::RectRectIntersection(1.0, 1.0, 0.0, 0.0, 0.0, 1.0) == 0.0);
    CHECK(OcclusionData::AreaOfIntersection(1.0, 1.0, 4.0) == 0.0);
    CHECK(OcclusionData::AreaOfIntersection(1.0, 0.0, 0.25) == 1.0);
    CHECK(OcclusionData::AreaOfIntersection(0.0, 1.0, 0.25) == 0.0);
    CHECK(OcclusionData::AreaOfIntersection(3.0, 1.0, 1.0) == 1.0);
    CHECK(OcclusionData::AreaOfIntersection(1.0, 2.0, 0.25) == 0.25);
    // two unit circles at distance 1: lens area 2*acos(1/2) - sqrt(3)/2, as a share of the potential circle
    const double lens = 2.0 * std::acos(0.5) - std::sqrt(3.0) / 2.0;
    CHECK(OcclusionData::AreaOfIntersection(1.0, 1.0, 1.0) == doctest::Approx(lens / kPi).epsilon(1e-12));
    // Update (:71) on a 2 m cube at the origin, flow along +Y
    const float a[6] = {4, 4, 4, 4, 4, 4}, d[6] = {0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f}, p[6] = {2, 2, 2, 2, 2, 2};
    auto part = CubePart({Cube("Default", a, d, p)});
    part->DragCubes.SetDragWeights();
    part->DragCubes.SetPartOcclusion();
    part->DragCubes.SetDrag(Vector3f::down(), 0.5f);
    OcclusionThermalRef ptd;
    ptd.part = part.get();
    OcclusionData od(ptd);
    od.Update(Vector3f::up(), Matrix4x4f::identity());
    CHECK(od.centroidDot == 0.0);
    CHECK(od.minimumDot == -1.0);
    CHECK(od.maximumDot == 1.0);
    CHECK(od.extents.Equals(Vector2f(1.0f, 1.0f)));
    CHECK(od.center.Equals(Vector2f(0.0f, 0.0f)));
    CHECK(od.projectedArea == static_cast<double>(part->DragCubes.CrossSectionalArea()));
    CHECK(od.projectedRadius == std::sqrt(od.projectedArea / kPi));
    CHECK(od.maxWidthDepth == static_cast<double>(part->DragCubes.Depth()));
    od.Update(Vector3f::up(), Matrix4x4f::identity(), false);
    CHECK(od.projectedArea == static_cast<double>(part->DragCubes.GetCubeAreaDir(Vector3f::up())));
    // MultiplyPoint3x4 with a translation
    Matrix4x4f tr = Matrix4x4f::identity();
    tr.m13 = 5.0f;
    CHECK(MultiplyPoint3x4(tr, Vector3f(1.0f, 2.0f, 3.0f)).Equals(Vector3f(1.0f, 7.0f, 3.0f)));
    od.Update(Vector3f::up(), tr);
    CHECK(od.centroidDot == 5.0);
    // cylinder (:17) and cone (:60)
    OcclusionCylinder cyl;
    cyl.Setup(od);
    CHECK(cyl.radius == od.projectedRadius);
    CHECK(cyl.cylNoseDot == od.maximumDot - od.maxWidthDepth);
    CHECK(cyl.offset.Equals(Vector2f(-od.center.x, -od.center.y)));
    CHECK(od.GetCylinderOcclusionVerts(cyl) >= 0.0);
    CHECK(od.GetCylinderOcclusion(cyl) == 1.0);  // the same rectangle
    OcclusionCone cone;
    od.invFineness = 2.0;  // blunt: detached shock
    cone.Setup(od, 1.0, 0.5, 1.0);
    CHECK(cone.shockNoseDot == od.maximumDot + cone.radius * 2.0);
    CHECK(cone.shockAngle == UtilMath::Lerp(OcclusionCone::detachedShockEndAngle, OcclusionCone::detachedShockStartAngle, 0.05));
    CHECK(ptd.convectionTempMultiplier == 0.5);  // detachedShockHeatMult
    CHECK(cone.shockConvectionTempMult == 0.4);
    CHECK(cone.occludeConvectionAreaMult == 0.0);
    ptd.convectionTempMultiplier = 1.0;
    od.invFineness = 0.2;  // slender: oblique shock
    cone.Setup(od, 1.0, 0.5, 1.0);
    CHECK(cone.shockNoseDot == od.maximumDot);
    const double as = dotnet::Math::Asin(0.2);
    CHECK(cone.shockAngle == std::max(as * 1.05, 0.5 * 0.8 + as * 0.25));
    CHECK(cone.occludeConvectionTempMult == 0.55);
    CHECK(cone.occludeConvectionAreaMult == 1.0);
    CHECK(cone.GetShockRadius(od.maximumDot - 1.0) == cone.radius + 1.0 * dotnet::Math::Tan(cone.shockAngle));
    double tm = 1.0, cm = 1.0;
    CHECK(od.GetShockStats(cone, tm, cm) == doctest::Approx(1.0 - 1.0 + 1.0 * 1.0));  // fully inside the cone's rectangle
    CHECK(od.GetConvectionMultVerts(cone) <= 1.0);
    OcclusionData other(ptd);
    other.maximumDot = od.maximumDot + 4.0;
    CHECK(od.CompareTo(other) == -1);
    CHECK(other.CompareTo(od) == 1);
    CHECK(od.CompareTo(od) == 0);
}
