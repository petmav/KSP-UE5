// WP-20f: SAS / Vessel.FeedInputFeed in the physics tick, ModuleGimbal actuation, the longer C10 ascent gate.
//
//   * Vessel.FeedInputFeed (Vessel.cs:2035) and FlightInputHandler.FixedUpdate (FlightInputHandler.cs:377) driven by the bubble in the
//     measured order (analysis/physics/raw/run1/run1-L_calltrace.csv: seq 18, before the part modules 22-276 and FlightIntegrator 278);
//   * ModuleGimbal.FixedUpdate (ModuleGimbal.cs:161) on the real gimbal transform chains of the staged engines (core/data, WP-17b): the
//     geometry against an independent double-precision recomposition, the neutral pose bit for bit, the actuation's torque sign against
//     the reaction wheel's for the same command (both KSP's conventions), the one-tick lag of the thrust behind the gimbal;
//   * VesselSAS (WP-24) in the tick: SAS damps a spin in orbit with the probe's reaction wheel, KillAngularVelocity on the rigid bodies;
//   * the C10 ascent of PhysProf-M2 / -L2-AS / -S with SAS on as KSP flew it (harness/captures/out2/aero_forces_vessel.json "states").
//
// Fixture order in a test: World, then the scene (PadScene / OrbitScene), then InputScene, then the vessels (destroyed first).
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "flight_integrator_fixture.hpp"
#include "ksp/input/flight_input_handler.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/modules/module_gimbal.hpp"
#include "ksp/modules/module_reaction_wheel.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/vessel/action_group_list.hpp"
#include "ksp/vessel/vessel_autopilot.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using namespace ksp::physics;
using json = nlohmann::json;

namespace {

// The flight scene's input side as FlightDriver sets it up: GameSettings at their defaults (the bindings FlightInputHandler and the SAS
// input checks read; no input device: nothing pressed), HighLogic.LoadedSceneIsFlight, the scene's FlightInputHandler (Awake / Start /
// OnDestroy here, FixedUpdate by the bubble), a sandbox Game (VesselAutopilot's skill rule reads HighLogic.CurrentGame).
struct InputScene {
    PhysicsBubble* bubble;
    FlightInputHandler fih;
    Game game;
    explicit InputScene(PhysicsBubble& b) : bubble(&b) {
        GameSettings::SetDefaultValues();
        HighLogic::LoadedSceneIsFlight = true;
        FlightDriver::pause = false;
        FlightInputHandler::host = FlightInputHost{};
        Time::deltaTime = Time::fixedDeltaTime;  // Time.deltaTime inside FixedUpdate
        fih.Awake();
        fih.Start();
        game.Mode = Game_Modes::SANDBOX;
        game.CrewRoster = std::make_shared<KerbalRoster>(Game_Modes::SANDBOX);
        SaveEnvironment::Current().currentGame = &game;
        b.SetFlightInputHandler(&fih);
    }
    ~InputScene() {
        bubble->SetFlightInputHandler(nullptr);
        fih.OnDestroy();
        FlightInputHandler::fetch = nullptr;
        FlightInputHandler::state = FlightCtrlState();
        FlightInputHandler::host = FlightInputHost{};
        HighLogic::LoadedSceneIsFlight = false;
        SaveEnvironment::Current().currentGame = nullptr;
    }
};

// the capture's OnFlyByWire callback (HarnessCapture2Flight.cs:332 FullThrottle)
void FullThrottle(FlightCtrlState& s) { s.mainThrottle = 1.0f; }

ModuleGimbal* GimbalOf(Part& p) {
    for (size_t i = 0; i < p.Modules().Count(); ++i)
        if (auto* g = dynamic_cast<ModuleGimbal*>(p.Modules().GetModule(i))) return g;
    return nullptr;
}
ModuleEngines* EngineOf(Part& p) {
    for (size_t i = 0; i < p.Modules().Count(); ++i)
        if (auto* e = dynamic_cast<ModuleEngines*>(p.Modules().GetModule(i))) return e;
    return nullptr;
}
ModuleReactionWheel* WheelOf(Vessel& v) {
    for (Part* p : v.parts)
        for (size_t i = 0; i < p->Modules().Count(); ++i)
            if (auto* w = dynamic_cast<ModuleReactionWheel*>(p->Modules().GetModule(i))) return w;
    return nullptr;
}
Part* FindPartNamed(Vessel& v, const std::string& name) {
    for (Part* p : v.parts)
        if (p->partInfo != nullptr && p->partInfo->name == name) return p;
    return nullptr;
}
double ElectricCharge(Vessel& v) {
    double ec = 0.0;
    for (Part* p : v.parts)
        if (const PartResource* r = p->GetResource("ElectricCharge")) ec += r->amount;
    return ec;
}
double AngleDeg(const Vector3d& a, const Vector3d& b) {
    const double c = Vector3d::Dot(a, b) / (a.magnitude() * b.magnitude());
    return std::acos(std::max(-1.0, std::min(1.0, c))) * 57.29577951308232;
}

// ---- an independent double-precision recomposition of a model transform chain (Hamilton products, v' = q v q*) --------------------
struct Qd {
    double x, y, z, w;
};
Qd Mul(const Qd& a, const Qd& b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
            a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}
Vector3d Rot(const Qd& q, const Vector3d& v) {
    const Qd p{v.x, v.y, v.z, 0.0};
    const Qd r = Mul(Mul(q, p), Qd{-q.x, -q.y, -q.z, q.w});
    return {r.x, r.y, r.z};
}
Qd ToD(const Quaternionf& q) { return {q.x, q.y, q.z, q.w}; }
// angle-axis in degrees, double
Qd AxisAngle(double deg, const Vector3d& axis) {
    const double h = deg * 3.14159265358979323846 / 360.0;
    const Vector3d n = axis.normalized();
    return {n.x * std::sin(h), n.y * std::sin(h), n.z * std::sin(h), std::cos(h)};
}
// part-space position and rotation of the chain's leaf with node k's local rotation replaced by `qk`
void Recompose(const ModelTransformChain& c, size_t k, const Qd& qk, Vector3d& pos, Qd& rot) {
    pos = Vector3d();
    rot = Qd{0, 0, 0, 1};
    Vector3d scl(1, 1, 1);
    for (size_t i = 0; i < c.size(); ++i) {
        const Vector3d lp(c[i].localPosition);
        pos = pos + Rot(rot, Vector3d(scl.x * lp.x, scl.y * lp.y, scl.z * lp.z));
        rot = Mul(rot, i == k ? qk : ToD(c[i].localRotation));
        scl = Vector3d(scl.x * c[i].localScale.x, scl.y * c[i].localScale.y, scl.z * c[i].localScale.z);
    }
}

double HexD(const json& v) {
    const uint64_t b = std::stoull(v.get<std::string>(), nullptr, 16);
    double d;
    std::memcpy(&d, &b, 8);
    return d;
}
float HexF(const json& v) {
    const uint32_t b = static_cast<uint32_t>(std::stoul(v.get<std::string>(), nullptr, 16));
    float f;
    std::memcpy(&f, &b, 4);
    return f;
}

bool SameBits(const Vector3f& a, const Vector3f& b) { return std::memcmp(&a, &b, sizeof(Vector3f)) == 0; }
bool SameBits(const Quaternionf& a, const Quaternionf& b) { return std::memcmp(&a, &b, sizeof(Quaternionf)) == 0; }

}  // namespace

// ===================================================================================================================== FeedInputFeed

TEST_CASE("WP-20f Vessel.FeedInputFeed (Vessel.cs:2035): OnPre / OnAutopilot / OnPost / OnFlyByWire in order on vessel.ctrlState; FlightInputHandler feeds it by default") {
    Vessel v;
    std::vector<std::string> order;
    struct Rec {
        std::vector<std::string>* order;
        void Pre(FlightCtrlState& s) { order->push_back("pre"); s.pitch = 0.5f; }
        void Ap(FlightCtrlState& s) { order->push_back(s.pitch == 0.5f ? "ap(0.5)" : "ap"); s.pitch = 0.25f; }
        void Post(FlightCtrlState& s) { order->push_back(s.pitch == 0.25f ? "post(0.25)" : "post"); }
        void Fbw(FlightCtrlState& s) { order->push_back("fbw"); s.mainThrottle = 1.0f; }
    } rec{&order};
    v.OnFlyByWire += Delegate<void(FlightCtrlState&)>::Bind(&rec, &Rec::Fbw);
    v.OnPostAutopilotUpdate += Delegate<void(FlightCtrlState&)>::Bind(&rec, &Rec::Post);
    v.OnAutopilotUpdate += Delegate<void(FlightCtrlState&)>::Bind(&rec, &Rec::Ap);
    v.OnPreAutopilotUpdate += Delegate<void(FlightCtrlState&)>::Bind(&rec, &Rec::Pre);
    v.FeedInputFeed();  // not loaded: no Part.propagateControlUpdate (rootPart is null here)
    CHECK(order == std::vector<std::string>{"pre", "ap(0.5)", "post(0.25)", "fbw"});
    CHECK(v.ctrlState.pitch == 0.25f);
    CHECK(v.ctrlState.mainThrottle == 1.0f);

    // FlightInputHandler.FixedUpdate's hand-off without a host hook is the real FeedInputFeed (FlightInputHandler.cs:686)
    GameSettings::SetDefaultValues();
    HighLogic::LoadedSceneIsFlight = true;
    FlightDriver::pause = false;
    FlightInputHandler::host = FlightInputHost{};
    FlightRegistry::VesselsLoaded = {&v};
    FlightRegistry::ActiveVessel = &v;
    FlightRegistry::ready = true;
    FlightInputHandler h;
    h.Awake();
    FlightInputHandler::state.mainThrottle = 0.5f;
    v.packed = false;  // FlightInputHandler feeds unpacked vessels only
    order.clear();
    h.FixedUpdate();
    CHECK(order == std::vector<std::string>{"pre", "ap(0.5)", "post(0.25)", "fbw"});
    CHECK(v.ctrlState.mainThrottle == 1.0f);                                 // OnFlyByWire after the copy (0.5) wins
    h.OnDestroy();
    FlightInputHandler::fetch = nullptr;
    FlightInputHandler::state = FlightCtrlState();
    FlightRegistry::VesselsLoaded.clear();
    FlightRegistry::ActiveVessel = nullptr;
    FlightRegistry::ready = false;
    HighLogic::LoadedSceneIsFlight = false;
}

TEST_CASE("WP-20f bubble: FlightInputHandler.FixedUpdate and FeedInputFeed at seq 18, after VesselPrecalculate, before the part modules and FlightIntegrator") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w(true);
    wp20d::PadScene pad(w);
    InputScene in(pad.s.bubble);
    std::vector<std::string> order;
    const std::function<void()> precalc = FlightScene::hooks.afterPrecalc, fi = FlightScene::hooks.afterFlightIntegrator;
    FlightScene::hooks.afterPrecalc = [&] {
        precalc();
        order.push_back("precalc");
    };
    FlightScene::hooks.afterFlightIntegrator = [&] {
        order.push_back("fi");
        fi();
    };
    ReactionWheelEnvironment::addTorque = [&](ModuleReactionWheel&, const Vector3f&) { order.push_back("wheel"); };
    auto v = w.Craft("PhysProf-S");
    struct Rec {
        std::vector<std::string>* order;
        float pitchSeen = -9.0f;
        void Pre(FlightCtrlState& s) {
            order->push_back("pre");
            pitchSeen = s.pitch;
        }
        void Fbw(FlightCtrlState&) { order->push_back("fbw"); }
    } rec{&order};
    v->OnPreAutopilotUpdate += Delegate<void(FlightCtrlState&)>::Bind(&rec, &Rec::Pre);
    v->OnFlyByWire += Delegate<void(FlightCtrlState&)>::Bind(&rec, &Rec::Fbw);
    pad.Place(*v);
    wp20d::StartModules(*v, StartState::PreLaunch);
    pad.s.Start(*v);
    for (int k = 0; k < 3; ++k) pad.s.Tick();
    REQUIRE_FALSE(v->packed);
    wp20d::IgniteFirstStage(*v);
    // the pilot's trim reaches the active vessel through SetControlState before the callbacks (FlightInputHandler.cs:655 / :679)
    FlightInputHandler::state.pitchTrim = 0.25f;
    order.clear();
    pad.s.Tick();
    INFO("order: " << [&] { std::string s; for (auto& o : order) s += o + " "; return s; }());
    // one vessel: precalc, then its feed, then the reaction wheel (part modules), then the FlightIntegrator
    REQUIRE(order.size() == 5);
    CHECK(order == std::vector<std::string>{"precalc", "pre", "fbw", "wheel", "fi"});
    CHECK(rec.pitchSeen == 0.25f);
    CHECK(v->ctrlState.pitch == 0.25f);
    // no FlightInputHandler: no feed
    pad.s.bubble.SetFlightInputHandler(nullptr);
    order.clear();
    pad.s.Tick();
    CHECK(std::count(order.begin(), order.end(), std::string("pre")) == 0);
    pad.s.bubble.SetFlightInputHandler(&in.fih);
    ReactionWheelEnvironment::addTorque = nullptr;
    FlightScene::hooks.afterPrecalc = precalc;
    FlightScene::hooks.afterFlightIntegrator = fi;
}

// ===================================================================================================================== gimbals

TEST_CASE("WP-20f gimbal transform chains (core/data, WP-17b): neutral pose bit for bit, recomposition vs an independent double reference") {
    const wp17b::EngineThrustTransforms data = wp17b::EngineThrustTransforms::Load(wp20d::ThrustJsonPath());
    size_t compared = 0;
    double maxPosErr = 0.0, maxDirErr = 0.0;
    for (const auto& [name, part] : data.Parts())
        for (const auto& t : part.transforms) {
            ModelTransformChain c;
            for (size_t i = 0; i < t.chainNames.size(); ++i)
                c.push_back(ModelTransformNode{t.chainNames[i], t.chainPositions[i], t.chainRotations[i], t.chainScales[i]});
            PartTransform neutral;
            neutral.position = t.partSpacePosition;
            neutral.rotation = wp17b::EngineThrustTransforms::ChainRotation(t);
            // the WP-17b convention, bit for bit
            CHECK(SameBits(ChainPartSpaceRotation(c), neutral.rotation));
            // every node as the gimbal: the neutral local rotation gives the neutral pose bit for bit
            for (size_t k = 0; k < c.size(); ++k) {
                const PartTransform p = ChainPoseWithLocalRotation(neutral, c, k, c[k].localRotation);
                CHECK(SameBits(p.position, neutral.position));
                CHECK(SameBits(p.rotation, neutral.rotation));
            }
            // the last two nodes actuated by ModuleGimbal's composition: initRot * AngleAxis(ax, right) * AngleAxis(ay, up)
            for (size_t k = c.size() >= 2 ? c.size() - 2 : 0; k < c.size(); ++k)
                for (const auto& a : {std::pair<float, float>{2.0f, 0.0f}, {0.0f, -3.0f}, {1.5f, 2.5f}, {-6.0f, 4.0f}}) {
                    const Quaternionf qf = UnityNative::Multiply(UnityNative::Multiply(c[k].localRotation, UnityNative::AngleAxis(a.first, Vector3f::right())),
                                                                 UnityNative::AngleAxis(a.second, Vector3f::up()));
                    const PartTransform p = ChainPoseWithLocalRotation(neutral, c, k, qf);
                    Vector3d pr;
                    Qd rr;
                    Recompose(c, k, Mul(Mul(ToD(c[k].localRotation), AxisAngle(a.first, Vector3d(1, 0, 0))), AxisAngle(a.second, Vector3d(0, 1, 0))), pr, rr);
                    maxPosErr = std::max(maxPosErr, (Vector3d(p.position) - pr).magnitude());
                    maxDirErr = std::max(maxDirErr, AngleDeg(Vector3d(p.rotation * Vector3f::forward()), Rot(rr, Vector3d(0, 0, 1))));
                    ++compared;
                }
        }
    MESSAGE(compared << " actuated poses: largest position error " << maxPosErr << " m, forward direction error " << maxDirErr << " deg");
    CHECK(compared > 50);
    CHECK(maxPosErr < 2e-6);
    CHECK(maxDirErr < 2e-3);  // float vs double over a 4-6 node chain (~1e-5 rad)
}

TEST_CASE("WP-20f ModuleGimbal actuation in the tick: deflection = gimbalRange, torque in the reaction wheel's sense, the thrust lags the gimbal one tick") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w(true);
    wp20d::PadScene pad(w);
    InputScene in(pad.s.bubble);
    auto v = w.Craft("PhysProf-S");
    pad.Place(*v);
    wp20d::StartModules(*v, StartState::PreLaunch);
    pad.s.Start(*v);
    for (int k = 0; k < 100; ++k) pad.s.Tick();
    REQUIRE_FALSE(v->packed);
    Part* mainsail = FindPartNamed(*v, "liquidEngineMainsail.v2");
    Part* lvt45 = FindPartNamed(*v, "liquidEngine2");
    REQUIRE(mainsail != nullptr);
    REQUIRE(lvt45 != nullptr);
    ModuleGimbal* gm = GimbalOf(*mainsail);
    ModuleGimbal* gl = GimbalOf(*lvt45);
    ModuleEngines* em = EngineOf(*mainsail);
    ModuleEngines* el = EngineOf(*lvt45);
    REQUIRE((gm != nullptr && gl != nullptr && em != nullptr && el != nullptr));
    ModuleReactionWheel* wheel = WheelOf(*v);
    REQUIRE(wheel != nullptr);
    // what the engines did this tick, read between the part modules and the physics step (the parts have not moved yet): the thrust
    // axes in their part's frame, the engines' torque about the vessel's centre of mass (Vessel.CoM of this tick's VesselPrecalculate)
    struct Snap {
        Vector3d axisM, axisL, torque;
    } snap;
    const std::function<void()> fiSlot = FlightScene::hooks.afterFlightIntegrator;
    FlightScene::hooks.afterFlightIntegrator = [&] {
        if (!em->thrustTransforms.empty() && !el->thrustTransforms.empty()) {
            snap.axisM = Vector3d(mainsail->partTransform.InverseTransformDirection(em->thrustTransforms[0].forward()));
            snap.axisL = Vector3d(lvt45->partTransform.InverseTransformDirection(el->thrustTransforms[0].forward()));
        }
        snap.torque = Vector3d();
        for (Part* p : v->parts)
            if (ModuleEngines* e = EngineOf(*p))
                for (const EngineForce& f : e->lastThrust.forces) snap.torque += Vector3d::Cross(Vector3d(f.position) - Vector3d(v->CoM), Vector3d(f.force));
        fiSlot();
    };
    FlightInputHandler::state.mainThrottle = 1.0f;
    v->ctrlState.mainThrottle = 1.0f;
    v->OnFlyByWire += Delegate<void(FlightCtrlState&)>::Static(&FullThrottle);
    REQUIRE(wp20d::IgniteFirstStage(*v) == 3);
    for (int k = 0; k < 5; ++k) pad.s.Tick();
    // the gimbals found their transforms through the chains (ModuleGimbal.OnStart on their first active FixedUpdate)
    REQUIRE(gm->gimbalTransforms.size() == 1);
    REQUIRE(gl->gimbalTransforms.size() == 1);
    CHECK(pad.s.bubble.ModuleForces().gimbalUpdates == 3);
    CHECK(pad.s.bubble.ModuleForces().gimbaledThrustTransforms == 0);  // no command: every thrust transform at its neutral pose
    const Vector3d axisM0 = snap.axisM, axisL0 = snap.axisL, t0 = snap.torque;

    struct Cmd {
        float pitch = 0.0f, yaw = 0.0f;
        void Apply(FlightCtrlState& s) {
            s.pitch = pitch;
            s.yaw = yaw;
        }
    } cmd;
    v->OnFlyByWire += Delegate<void(FlightCtrlState&)>::Bind(&cmd, &Cmd::Apply);
    for (const char* axis : {"pitch", "yaw"}) {
        cmd.pitch = std::string(axis) == "pitch" ? 1.0f : 0.0f;
        cmd.yaw = std::string(axis) == "yaw" ? 1.0f : 0.0f;
        pad.s.Tick();
        // the command tick: the wheel torques at once (ModuleReactionWheel.cs:204), the engines still thrust along the neutral axes
        // (ModuleEngines runs before ModuleGimbal on its part, which only now turns the gimbal transforms)
        const Vector3d wheelTorque(wheel->lastTorque);
        const Vector3d lagged = snap.torque - t0;
        CHECK(AngleDeg(snap.axisM, axisM0) < 1e-3);
        pad.s.Tick();
        const Vector3d moved = snap.torque - t0;
        const double c = Vector3d::Dot(moved.normalized(), wheelTorque.normalized());
        MESSAGE(std::string(axis) << ": engines' torque change " << lagged.magnitude() << " kN m on the command tick, " << moved.magnitude()
                     << " kN m one tick later; wheel torque " << wheelTorque.magnitude() << " kN m; cos(gimbal, wheel) " << c);
        CHECK(wheelTorque.magnitude() > 0.01);
        CHECK(moved.magnitude() > 100.0 * lagged.magnitude());
        CHECK(c > 0.9);  // the gimbals turn the vessel the way the reaction wheel does for the same command
        cmd.pitch = cmd.yaw = 0.0f;
        for (int k = 0; k < 3; ++k) pad.s.Tick();
        CHECK(AngleDeg(snap.axisM, axisM0) < 1e-3);  // back at neutral
    }
    // a held pitch: each thrust axis turns by the engine's gimbalRange (limiter 100 %): Mainsail 2 deg, LV-T45 up to 3 deg
    cmd.pitch = 1.0f;
    for (int k = 0; k < 3; ++k) pad.s.Tick();
    const double dM = AngleDeg(snap.axisM, axisM0), dL = AngleDeg(snap.axisL, axisL0);
    MESSAGE("deflection at full pitch: Mainsail " << dM << " deg (gimbalRange " << gm->Get<float>("gimbalRange") << "), LV-T45 " << dL << " deg (gimbalRange "
                                               << gl->Get<float>("gimbalRange") << "); thrust transforms off neutral " << pad.s.bubble.ModuleForces().gimbaledThrustTransforms);
    CHECK(dM == doctest::Approx(static_cast<double>(gm->Get<float>("gimbalRange"))).epsilon(1e-3));
    CHECK(dL <= static_cast<double>(gl->Get<float>("gimbalRange")) * 1.001);
    CHECK(dL > 0.5);  // the radial LV-T45s gimbal too (their pitch share depends on where they sit)
    CHECK(pad.s.bubble.ModuleForces().gimbaledThrustTransforms == 3);
    // gimbal lock (ModuleGimbal.cs:176: t.localRotation = initRots[i] and no actuation): the Mainsail's thrust is neutral one tick later
    gm->Set<bool>("gimbalLock", true);
    pad.s.Tick();
    CHECK(dM == doctest::Approx(AngleDeg(snap.axisM, axisM0)));
    pad.s.Tick();
    CHECK(AngleDeg(snap.axisM, axisM0) < 1e-3);
    CHECK(pad.s.bubble.ModuleForces().gimbaledThrustTransforms == 2);
    FlightScene::hooks.afterFlightIntegrator = fiSlot;
}

// ===================================================================================================================== SAS

namespace {

// a craft in a 120 km circular Kerbin orbit in the bubble (inertial frame, vacuum), engines' models and resource sets, the input scene
struct OrbitScene {
    wp20b::World& w;
    wp19b::BubbleScene s;
    wp20d::EngineModels engines;
    wp20d::ResourceSets resources;
    explicit OrbitScene(wp20b::World& w_) : w(w_), s(w_) {
        wp20d::StartPhysicsGlobals(w);
        SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
        engines.Install(s.bubble.ModuleForces());
    }
    void Place(Vessel& v, double altitude) {
        CelestialBody& kerbin = w.Kerbin();
        Vector3d pos, vel;
        wp19b::CircularStateAt(kerbin, kerbin.Radius + altitude, Vector3d(0.6, 0.48, -0.64), Vector3d(0.3, -0.9, 0.2), pos, vel);
        wp19b::PlaceOnOrbit(v, kerbin, pos, vel, true);
        v.UpdateResourceSets();
        s.Start(v);
    }
};

struct SpinResult {
    double w0 = 0.0, w1 = 0.0;        // |angular velocity| at the kick and at the end (rad/s)
    double drift = 0.0;               // attitude change from the SAS lock to the end (deg)
    bool sas = false, killed = false; // SAS enabled; KillAngularVelocity ran (decay locked)
    double ecUsed = 0.0;
};

SpinResult Spin(bool sasOn, int ticks) {
    SpinResult r;
    wp20b::World w;
    OrbitScene sc(w);
    InputScene in(sc.s.bubble);
    auto v = w.CraftHead("PhysProf-S", 3);  // probe (reaction wheel 1.5 kN m, ModuleSAS), Rockomax X200-32, Mainsail
    sc.Place(*v, 120000.0);
    wp20d::StartModules(*v, StartState::Orbital);
    for (int k = 0; k < 50; ++k) sc.s.Tick();
    REQUIRE_FALSE(v->packed);
    for (Part* p : v->parts)
        if (p->FindModuleOfClass("ModuleCommand") != nullptr) p->fields.isControlSource = static_cast<int32_t>(ControlLevel::FULL);
    v->CheckControllable();
    VesselPhysics* vp = sc.s.bubble.Host().Find(*v);
    REQUIRE(vp != nullptr);
    // a tumble about an axis across the stack (physics axes; the vessel axis is vesselTransform.up)
    const Vector3d axis = Vector3d::Cross(Vector3d(v->vesselTransform.up()), Vector3d(0.3, 0.5, 0.8)).normalized();
    vp->SetPartAngularVelocity(*v->rootPart, axis * 0.05);
    sc.s.Tick();
    r.w0 = Vector3d(v->angularVelocity).magnitude();
    const double ec0 = ElectricCharge(*v);
    if (sasOn) v->ActionGroups().SetGroup(KSPActionGroup::SAS, true);
    sc.s.Tick();
    r.sas = v->Autopilot().Enabled();
    const Quaternionf lock = v->ReferenceTransform().rotation;
    for (int k = 0; k < ticks; ++k) {
        sc.s.Tick();
        r.killed = r.killed || v->Autopilot().SAS().decayLocked();
    }
    r.w1 = Vector3d(v->angularVelocity).magnitude();
    r.drift = AngleDeg(Vector3d(lock * Vector3f::up()), Vector3d(v->ReferenceTransform().rotation * Vector3f::up()));
    r.ecUsed = ec0 - ElectricCharge(*v);
    return r;
}

}  // namespace

TEST_CASE("WP-20f VesselSAS in the tick: SAS (the SAS action group, VesselAutopilot.Update) stops a tumble in orbit with the probe's wheel; without it the tumble stays") {
    if (!wp20b::GameAvailable()) return;
    const SpinResult off = Spin(false, 1000);
    const SpinResult on = Spin(true, 1000);
    MESSAGE("SAS off: |w| " << off.w0 << " -> " << off.w1 << " rad/s, axis turned " << off.drift << " deg, EC used " << off.ecUsed);
    MESSAGE("SAS on:  |w| " << on.w0 << " -> " << on.w1 << " rad/s, axis turned " << on.drift << " deg, EC used " << on.ecUsed
                            << ", KillAngularVelocity (decay lock) " << on.killed);
    CHECK_FALSE(off.sas);
    CHECK(on.sas);
    CHECK(off.w0 == doctest::Approx(0.05).epsilon(0.02));
    CHECK(off.w1 > 0.8 * off.w0);   // free tumble (gravity gradient only)
    CHECK(on.w1 < 0.05 * on.w0);    // damped
    CHECK(on.drift < 20.0);         // brought back towards the locked attitude (the wheel needs a few seconds)
    CHECK(on.ecUsed > off.ecUsed);  // the wheel drew ElectricCharge
}

TEST_CASE("WP-20f module lifecycle on the pad: every part's drag-cube weights equal KSP's live ones (C10 WeightedArea, S / M2 / L2-AS, 0 ULP)") {
    if (!wp20b::GameAvailable()) return;
    // Part.Start's ModulesOnStart / ModulesOnStartFinished and the first ModuleJettison.LateUpdate (wp20d::StartModules) set the cube weights
    // KSP flies with: ModulePartVariants.RefreshVariant (ModulePartVariants.cs:307, Rockomax32.BW's variant cube), ModuleJettison's
    // SetupJettisonState -> SetFairingDragCube (ModuleJettison.cs:542: the Mainsail's variant cube, the LV-T45's Clean cube with nothing on
    // its bottom node). Without them 4 of PhysProf-S's 21 parts fly the default weights (WP-20d's PadScene does not run the lifecycle).
    std::ifstream fs(std::string(KSPCORE_HARNESS_DIR) + "/captures/out2/aero_forces_vessel.json", std::ios::binary);
    REQUIRE(fs.good());
    const json doc = json::parse(fs);
    for (const char* craft : {"PhysProf-S", "PhysProf-M2", "PhysProf-L2-AS"}) {
        std::map<int, std::array<float, 6>> cap;
        std::map<int, std::string> capName;
        for (const json& t : doc.at("tables")) {
            if (t.at("name").get<std::string>() != "parts") continue;
            std::map<std::string, size_t> col;
            size_t i = 0;
            for (const json& c : t.at("columns")) col[c.at("name").get<std::string>()] = i++;
            std::string firstState;
            for (const json& r : t.at("rows")) {
                if (r.at(col.at("craft")).get<std::string>() != craft) continue;
                const std::string st = r.at(col.at("state")).get<std::string>();
                if (firstState.empty()) firstState = st;
                if (st != firstState) continue;
                const int pi = static_cast<int>(r.at(col.at("part_index")).get<int64_t>());
                for (int f = 0; f < 6; ++f) cap[pi][static_cast<size_t>(f)] = HexF(r.at(col.at("WeightedArea_" + std::to_string(f) + "_hex")));
                capName[pi] = r.at(col.at("part")).get<std::string>();
            }
        }
        size_t lifecycle = 0, fixture = 0, n = 0;
        for (int variant = 0; variant < 2; ++variant) {
            wp20b::World w(true);
            wp20d::PadScene pad(w);
            auto v = w.Craft(craft);
            pad.Place(*v);
            if (variant == 1) wp20d::StartModules(*v, StartState::PreLaunch);
            pad.s.Start(*v);
            for (int k = 0; k < 3; ++k) pad.s.Tick();
            REQUIRE(cap.size() == v->parts.size());
            n = v->parts.size();
            for (size_t k = 0; k < v->parts.size(); ++k) {
                const int pi = static_cast<int>(k);
                REQUIRE(capName[pi] == v->parts[k]->partInfo->name);
                const bool same = std::memcmp(v->parts[k]->DragCubes.WeightedArea().data(), cap[pi].data(), sizeof(float) * 6) == 0;
                (variant == 1 ? lifecycle : fixture) += same ? 1 : 0;
            }
        }
        MESSAGE(craft << ": parts whose WeightedArea equals KSP's, bit for bit: " << lifecycle << " / " << n << " with the module lifecycle, " << fixture << " / " << n
                      << " without");
        CHECK(lifecycle == n);
    }
}

// ===================================================================================================================== C10 ascent

namespace {

struct C10State {
    std::string state;
    int frame = 0;
    double altitude = 0.0, spd = 0.0, mach = 0.0, aoa = 0.0;
};

std::vector<C10State> C10States(const std::string& craft) {
    std::ifstream f(std::string(KSPCORE_HARNESS_DIR) + "/captures/out2/aero_forces_vessel.json", std::ios::binary);
    REQUIRE(f.good());
    const json doc = json::parse(f);
    std::vector<C10State> out;
    for (const json& t : doc.at("tables")) {
        if (t.at("name").get<std::string>() != "states") continue;
        std::map<std::string, size_t> col;
        size_t i = 0;
        for (const json& c : t.at("columns")) col[c.at("name").get<std::string>()] = i++;
        for (const json& r : t.at("rows")) {
            if (r.at(col.at("craft")).get<std::string>() != craft) continue;
            C10State s;
            s.state = r.at(col.at("state")).get<std::string>();
            s.frame = static_cast<int>(r.at(col.at("frame")).get<int64_t>());
            s.altitude = HexD(r.at(col.at("altitude_hex")));
            s.spd = HexD(r.at(col.at("spd_hex")));
            s.mach = HexD(r.at(col.at("mach_hex")));
            s.aoa = static_cast<double>(HexF(r.at(col.at("aoa_deg_hex"))));
            out.push_back(s);
        }
    }
    return out;
}

// One launch as C10 flew it (HarnessCapture2Flight.cs:759-764): FlightInputHandler.state.mainThrottle = 1, vessel ctrlState
// mainThrottle = 1, the FullThrottle OnFlyByWire callback, the SAS action group on, the first stage activated (IgniteFirstStage stands
// for StageManager.ActivateNextStage); every tick through the whole bubble with the FlightInputHandler and FeedInputFeed. The parts get
// the module lifecycle on the pad (wp20d::StartModules).
struct Ascent {
    std::vector<double> alt, spd, mach, aoa, vspd;
    std::vector<int> breaks;
    int padBreaks = 0;
    bool sasEnabled = false, controllable = false;
    double minEC = 1e30;
    size_t gimbalUpdates = 0;
    double maxGimbalDeg = 0.0;
};

Ascent Fly(const std::string& craft, bool sas, int frames, int settle = 100) {
    Ascent out;
    wp20b::World w(true);
    wp20d::PadScene pad(w);
    InputScene in(pad.s.bubble);
    auto v = w.Craft(craft);
    pad.Place(*v);
    wp20d::StartModules(*v, StartState::PreLaunch);
    pad.s.Start(*v);
    int nBreaks = 0;
    Part* mainsail = FindPartNamed(*v, "liquidEngineMainsail.v2");
    ModuleEngines* em = mainsail != nullptr ? EngineOf(*mainsail) : nullptr;
    // the Mainsail's thrust axis in its part's frame, read between the part modules and the physics step
    Vector3d axis, axis0;
    const std::function<void()> fiSlot = FlightScene::hooks.afterFlightIntegrator;
    FlightScene::hooks.afterFlightIntegrator = [&] {
        if (em != nullptr && !em->thrustTransforms.empty())
            axis = Vector3d(mainsail->partTransform.InverseTransformDirection(em->thrustTransforms[0].forward()));
        fiSlot();
    };
    for (int k = 0; k < settle + frames; ++k) {
        if (k == settle) {
            out.padBreaks = nBreaks;
            FlightInputHandler::state.mainThrottle = 1.0f;
            v->ctrlState.mainThrottle = 1.0f;
            v->OnFlyByWire += Delegate<void(FlightCtrlState&)>::Static(&FullThrottle);
            if (sas) v->ActionGroups().SetGroup(KSPActionGroup::SAS, true);
            wp20d::IgniteFirstStage(*v);
        }
        pad.s.Tick();
        nBreaks += static_cast<int>(pad.s.bubble.Host().LastBreaks().size());
        if (k == settle) {
            out.sasEnabled = v->Autopilot().Enabled();
            out.controllable = v->IsControllable();
            axis0 = axis;
        }
        if (k < settle) continue;
        const FlightIntegrator* fi = pad.s.bubble.FlightIntegrators().Find(*v);
        out.alt.push_back(pad.RootAltitude(*v));
        out.spd.push_back(fi != nullptr ? fi->atm.spd : 0.0);
        out.mach.push_back(fi != nullptr ? fi->atm.mach : 0.0);
        out.vspd.push_back(v->verticalSpeed);
        const Vector3d up = Vector3d(v->vesselTransform.rotation * Vector3f::up());
        const Vector3d vel = fi != nullptr ? Vector3d(fi->Vel) : Vector3d();
        out.aoa.push_back(vel.magnitude() > 1.0 ? AngleDeg(up, vel) : 0.0);
        out.breaks.push_back(nBreaks - out.padBreaks);
        out.minEC = std::min(out.minEC, ElectricCharge(*v));
        out.gimbalUpdates += pad.s.bubble.ModuleForces().gimbalUpdates;
        if (axis0.magnitude() > 0.5) out.maxGimbalDeg = std::max(out.maxGimbalDeg, AngleDeg(axis, axis0));
    }
    FlightScene::hooks.afterFlightIntegrator = fiSlot;
    return out;
}

}  // namespace

TEST_CASE("WP-20f ascent with SAS from the pad vs KSP's own C10 ascent: M2 to 20 km and its 90 s state at 36.7 km, L2-AS to 10 km and its apex, S to Mach 1") {
    if (!wp20b::GameAvailable()) return;
    // KSP's C10 ascent (harness/captures/out2/aero_forces_vessel.json "states"): first-stage engines at full throttle, vertical, SAS on;
    // a state is the first physics frame at an altitude (1 / 2.5 / 5 / 10 / 20 km) or Mach number (0.5 / 1 / 1.5), plus 'final' (frame
    // 4500, 90 s after the capture's frame 0) or 'apex' (the first frame past 250 with a negative vertical speed). The capture's frame 0 is
    // not our ignition tick (about 25 ticks later: the capture waits 0.5 s after staging, HarnessCapture2Flight.cs:765), so only
    // offset-free quantities are gated: the speed at an altitude, the altitude at a Mach number, the frames between two states (2 s or
    // more apart), the apex altitude; 'final' is compared at our frame the same number of frames after the last state. Tolerances are
    // WP-20d's (not loosened).
    struct Window {
        const char* craft;
        int frames;
        double maxAlt;  // states compared up to this altitude
        bool apex;      // the apex state too
    };
    const double tolSpeed = 0.01, tolAlt = 0.015, tolFrames = 0.025;
    for (const Window wd : {Window{"PhysProf-M2", 4560, 21000.0, false}, Window{"PhysProf-L2-AS", 4400, 10100.0, true},
                            Window{"PhysProf-S", 900, 1600.0, false}}) {
        const std::vector<C10State> ksp = C10States(wd.craft);
        REQUIRE(!ksp.empty());
        const Ascent f = Fly(wd.craft, true, wd.frames);
        CHECK(f.sasEnabled);
        CHECK(f.controllable);
        CHECK(f.padBreaks == 0);
        CHECK(f.minEC > 0.0);
        int prevKsp = -1, prevOurs = -1, n = 0;
        double offset = 0.0;
        for (const C10State& k : ksp) {
            const bool isAlt = k.state.rfind("alt_", 0) == 0, isMach = k.state.rfind("mach_", 0) == 0;
            if (!(isAlt || isMach) || k.altitude > wd.maxAlt) continue;
            const double threshold = isAlt ? std::stod(k.state.substr(4)) : std::stod(k.state.substr(5));
            int i = -1;
            for (size_t j = 0; j < f.alt.size() && i < 0; ++j)
                if ((isAlt ? f.alt[j] : f.mach[j]) >= threshold) i = static_cast<int>(j);
            INFO(wd.craft << " " << k.state);
            REQUIRE(i >= 0);
            const size_t u = static_cast<size_t>(i);
            double maxAoa = 0.0;
            for (size_t j = 0; j <= u; ++j)
                if (f.spd[j] > 30.0) maxAoa = std::max(maxAoa, f.aoa[j]);
            const double dSpd = f.spd[u] / k.spd - 1.0, dAlt = f.alt[u] / k.altitude - 1.0;
            MESSAGE(wd.craft << " " << k.state << ": KSP frame " << k.frame << " alt " << k.altitude << " spd " << k.spd << " mach " << k.mach << " aoa " << k.aoa
                             << " | ours frame " << i << " alt " << f.alt[u] << " spd " << f.spd[u] << " mach " << f.mach[u] << " aoa " << f.aoa[u]
                             << " (max so far " << maxAoa << ") | speed " << dSpd * 100.0 << " %, altitude " << dAlt * 100.0 << " %");
            if (isAlt) CHECK(std::fabs(dSpd) < tolSpeed);
            else CHECK(std::fabs(dAlt) < tolAlt);
            if (prevKsp >= 0 && k.frame - prevKsp >= 100) {
                const double dFrames = static_cast<double>(i - prevOurs) / static_cast<double>(k.frame - prevKsp) - 1.0;
                MESSAGE("    frames since the previous state: KSP " << (k.frame - prevKsp) << ", ours " << (i - prevOurs) << " (" << dFrames * 100.0 << " %)");
                CHECK(std::fabs(dFrames) < tolFrames);
            }
            prevKsp = k.frame;
            prevOurs = i;
            offset += static_cast<double>(i - k.frame);
            ++n;
            CHECK(maxAoa < 3.0);
            CHECK(f.breaks[u] == 0);
        }
        CHECK(n >= 3);
        MESSAGE(std::string(wd.craft) << ": " << n << " states compared, our frames run " << (n > 0 ? offset / n : 0.0) << " behind KSP's; SAS " << f.sasEnabled
                                      << ", min ElectricCharge " << f.minEC << ", gimbal updates " << f.gimbalUpdates << ", largest Mainsail deflection "
                                      << f.maxGimbalDeg << " deg");
        for (const C10State& k : ksp) {
            if (k.state == "final" && prevKsp >= 0) {
                // the 90 s state: ours the same number of frames after the last state compared
                const int j = prevOurs + (k.frame - prevKsp);
                REQUIRE(j >= 0);
                REQUIRE(static_cast<size_t>(j) < f.alt.size());
                const size_t u = static_cast<size_t>(j);
                const double dAlt = f.alt[u] / k.altitude - 1.0, dSpd = f.spd[u] / k.spd - 1.0;
                MESSAGE(std::string(wd.craft) << " final (KSP frame " << k.frame << "): KSP alt " << k.altitude << " spd " << k.spd << " mach " << k.mach << " aoa " << k.aoa
                                              << " | ours frame " << j << " alt " << f.alt[u] << " spd " << f.spd[u] << " mach " << f.mach[u] << " aoa " << f.aoa[u]
                                              << " | altitude " << dAlt * 100.0 << " %, speed " << dSpd * 100.0 << " %");
                CHECK(std::fabs(dAlt) < tolAlt);
                CHECK(std::fabs(dSpd) < tolSpeed);
                CHECK(f.breaks[u] == 0);
            }
            if (k.state == "apex" && wd.apex) {
                int j = -1;
                for (size_t q = 251; q < f.vspd.size() && j < 0; ++q)
                    if (f.vspd[q] < 0.0) j = static_cast<int>(q);
                REQUIRE(j >= 0);
                const size_t u = static_cast<size_t>(j);
                const double dAlt = f.alt[u] / k.altitude - 1.0;
                MESSAGE(std::string(wd.craft) << " apex: KSP frame " << k.frame << " alt " << k.altitude << " | ours frame " << j << " alt " << f.alt[u] << " | altitude "
                                              << dAlt * 100.0 << " %; frames from the last state: KSP " << (k.frame - prevKsp) << ", ours " << (j - prevOurs));
                CHECK(std::fabs(dAlt) < tolAlt);
                CHECK(f.breaks[u] == 0);
            }
        }
        // past the window: what each of KSP's later states looks like in our flight (reported, not gated)
        for (const C10State& k : ksp) {
            const bool isAlt = k.state.rfind("alt_", 0) == 0;
            if (!isAlt || k.altitude <= wd.maxAlt) continue;
            const double threshold = std::stod(k.state.substr(4));
            int i = -1;
            for (size_t j = 0; j < f.alt.size() && i < 0; ++j)
                if (f.alt[j] >= threshold) i = static_cast<int>(j);
            double maxAlt = 0.0;
            for (double a : f.alt) maxAlt = std::max(maxAlt, a);
            if (i < 0) {
                MESSAGE(std::string(wd.craft) << " " << k.state << " (KSP frame " << k.frame << ", aoa " << k.aoa << "): not reached in " << f.alt.size()
                                              << " frames (highest " << maxAlt << " m)");
                continue;
            }
            const size_t u = static_cast<size_t>(i);
            MESSAGE(std::string(wd.craft) << " " << k.state << " (past the window): KSP frame " << k.frame << " spd " << k.spd << " aoa " << k.aoa << " | ours frame " << i
                                          << " spd " << f.spd[u] << " aoa " << f.aoa[u] << " | speed " << (f.spd[u] / k.spd - 1.0) * 100.0 << " %");
        }
    }
}

TEST_CASE("WP-20f cost of the control slot: L2-AS from the pad, ms per tick without / with gimbal actuation and the control modules / with FeedInputFeed and SAS") {
    if (!wp20b::GameAvailable()) return;
    // measured, not gated (timings depend on the machine); the three configurations fly the same craft the same way
    using Clock = std::chrono::steady_clock;
    double ms[3] = {0.0, 0.0, 0.0}, slotMs[3] = {0.0, 0.0, 0.0}, updMs[3] = {0.0, 0.0, 0.0};
    const int settle = 100, frames = 300;
    for (int config = 0; config < 3; ++config) {
        wp20b::World w(true);
        wp20d::PadScene pad(w);
        PartModuleForces& mf = pad.s.bubble.ModuleForces();
        if (config == 0) {  // WP-20d's slot: no chains, no control modules
            mf.hooks.modelTransformChains = nullptr;
            mf.command = mf.sas = mf.gimbals = false;
        }
        std::unique_ptr<InputScene> in;
        if (config == 2) in = std::make_unique<InputScene>(pad.s.bubble);
        auto v = w.Craft("PhysProf-L2-AS");
        pad.Place(*v);
        wp20d::StartModules(*v, StartState::PreLaunch);
        pad.s.Start(*v);
        double t = 0.0, slot = 0.0, upd = 0.0;
        const std::function<void()> before = FlightScene::hooks.beforeFlightIntegrator, bvu = FlightScene::hooks.beforeVesselUpdate;
        bool flying = false;
        FlightScene::hooks.beforeFlightIntegrator = [&] {
            const auto t0 = Clock::now();
            before();
            if (flying) slot += std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
        };
        FlightScene::hooks.beforeVesselUpdate = [&] {
            const auto t0 = Clock::now();
            bvu();
            if (flying) upd += std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
        };
        for (int k = 0; k < settle + frames; ++k) {
            flying = k >= settle;
            if (k == settle) {
                if (config == 2) FlightInputHandler::state.mainThrottle = 1.0f;
                v->ctrlState.mainThrottle = 1.0f;
                v->OnFlyByWire += Delegate<void(FlightCtrlState&)>::Static(&FullThrottle);
                if (config == 2) v->ActionGroups().SetGroup(KSPActionGroup::SAS, true);
                wp20d::IgniteFirstStage(*v);
            }
            const auto t0 = Clock::now();
            pad.s.Tick();
            if (k >= settle) t += std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
        }
        ms[config] = t / frames;
        slotMs[config] = slot / frames;
        updMs[config] = upd / frames;
        FlightScene::hooks.beforeFlightIntegrator = before;
        FlightScene::hooks.beforeVesselUpdate = bvu;
        if (config == 2) CHECK(v->Autopilot().Enabled());
        v.reset();
        in.reset();
    }
    MESSAGE("PhysProf-L2-AS (457 parts, 49 engines) ms per flight tick: WP-20d slot " << ms[0] << ", + gimbal actuation / ModuleCommand / ModuleSAS " << ms[1]
                                                                                    << ", + FlightInputHandler / FeedInputFeed / VesselSAS " << ms[2]
                                                                                    << "; of which the input + module slot " << slotMs[0] << " / " << slotMs[1] << " / " << slotMs[2]
                                                                                    << ", the Update hooks (Part.Update / ModuleRCS.Update / FlightIntegrator.Update) " << updMs[0] << " / "
                                                                                    << updMs[1] << " / " << updMs[2]);
    CHECK(ms[2] > 0.0);
}
