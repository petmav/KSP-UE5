// WP-UE-02: the game host (include/ksp/host/game_host.hpp). Host glue: the C# counterpart of each call is cited where there is one;
// the composition itself (which MonoBehaviours exist in the flight scene and in what order they are made and ticked) is the Unity
// scene's, recorded in STATUS.md WP-20g "Host contract" and in tests/{vessel_physics,physics_bubble,flight_integrator,contact}_fixture.hpp,
// tests/test_vab_launch.cpp and tests/test_flight_input_feed.cpp, which this file promotes to product code.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "engine_thrust_transforms.hpp"
#include "host_internal.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/timing_manager.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/flight_input_handler.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_device.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/modules/module_gimbal.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/contact_host.hpp"
#include "ksp/physics/floating_origin.hpp"
#include "ksp/physics/flight_integrator.hpp"
#include "ksp/physics/mu_part_colliders.hpp"
#include "ksp/physics/orbit_physics_manager.hpp"
#include "ksp/physics/part_forces.hpp"
#include "ksp/physics/physics_bubble.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/physics/scenery_host.hpp"
#include "ksp/physics/vessel_physics.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/game_persistence.hpp"
#include "ksp/save/quick_save_load.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/pqs_city.hpp"
#include "ksp/space_center/psystem_setup.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/space_center/scenery_colliders.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/terrain/pqs_mods_quad.hpp"
#include "ksp/terrain/pqs_parallel.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/util/simplex.hpp"
#include "ksp/vessel/action_group_list.hpp"
#include "ksp/vessel/flight_scene.hpp"
#include "ksp/vessel/vessel_autopilot.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_assembly.hpp"
#include "nlohmann/json.hpp"

namespace ksp::host {

namespace fs = std::filesystem;
using namespace ksp::physics;

namespace {

// ---- small helpers -------------------------------------------------------------------------------------------------------------

constexpr int kFrameCountBase = 2000;  // Vessel.env.frameCount at the scene's first frame (>= the physics hold, Vessel.PhysicsHoldExpired)

// Building a PQS reseeds UnityEngine.Random and rewrites the static Simplex permutation tables (Simplex.cs:155, as in KSP): the
// process-wide state later users depend on is restored when the host goes (tests/vessel_physics_fixture.hpp SimplexStateGuard).
struct StaticStateGuard {
    std::array<int, 256> simplexP = Simplex::StaticPermutation();
    std::array<int, 256> simplexNoiseP = SimplexNoise::StaticPermutation();
    UnityRandom::State random = UnityRandom::Global().state();
    ~StaticStateGuard() {
        Simplex::StaticPermutation() = simplexP;
        SimplexNoise::StaticPermutation() = simplexNoiseP;
        UnityRandom::Global().SetState(random);
    }
};

double Hash1(uint64_t& h, const void* p, size_t n) {
    const unsigned char* b = static_cast<const unsigned char*>(p);
    for (size_t i = 0; i < n; ++i) {
        h ^= b[i];
        h *= 1099511628211ull;
    }
    return 0.0;
}

// UnityEngine.Input fed from the shell's kspcore_input_frame (D12: KSP's own bindings read these through ExtendedInput)
class FrameInputDevice final : public IInputDevice {
public:
    void Set(const kspcore_input_frame& f) {
        down_.assign(f.keys_down, f.keys_down + (f.keys_down != nullptr ? f.keys_down_count : 0));
        up_.assign(f.keys_up, f.keys_up + (f.keys_up != nullptr ? f.keys_up_count : 0));
        held_.assign(f.keys_held, f.keys_held + (f.keys_held != nullptr ? f.keys_held_count : 0));
        axes_.clear();
        for (uint32_t i = 0; f.axes != nullptr && i < f.axis_count; ++i)
            if (f.axes[i].name != nullptr) axes_[f.axes[i].name] = f.axes[i].value;
        mouseHeld_ = f.mouse_buttons_held;
        mouseDown_ = f.mouse_buttons_down;
        mouseUp_ = f.mouse_buttons_up;
        mouse_ = Vector3f(static_cast<float>(f.mouse_position[0]), static_cast<float>(f.mouse_position[1]), 0.0f);
        text_ = Utf8ToUtf16(f.text_utf8);
    }
    // keys of one frame only: down / up / typed text are consumed by the frame (Unity resets them every frame)
    void EndFrame() {
        down_.clear();
        up_.clear();
        mouseDown_ = mouseUp_ = 0;
        text_.clear();
    }
    bool GetKey(KeyCode key) const override { return Has(held_, static_cast<int>(key)); }
    bool GetKeyDown(KeyCode key) const override { return Has(down_, static_cast<int>(key)); }
    bool GetKeyUp(KeyCode key) const override { return Has(up_, static_cast<int>(key)); }
    float GetAxis(const std::string& axisName) const override {
        const auto it = axes_.find(axisName);
        return it != axes_.end() ? it->second : 0.0f;
    }
    bool GetMouseButton(int button) const override { return button >= 0 && button < 32 && ((mouseHeld_ >> button) & 1u) != 0; }
    bool GetMouseButtonDown(int button) const override { return button >= 0 && button < 32 && ((mouseDown_ >> button) & 1u) != 0; }
    bool GetMouseButtonUp(int button) const override { return button >= 0 && button < 32 && ((mouseUp_ >> button) & 1u) != 0; }
    Vector3f mousePosition() const override { return mouse_; }
    std::u16string inputString() const override { return text_; }
    std::vector<std::string> GetJoystickNames() const override { return {}; }

private:
    static bool Has(const std::vector<int32_t>& v, int key) { return std::find(v.begin(), v.end(), key) != v.end(); }
    std::vector<int32_t> down_, up_, held_;
    std::map<std::string, float> axes_;
    uint32_t mouseHeld_ = 0, mouseDown_ = 0, mouseUp_ = 0;
    Vector3f mouse_;
    std::u16string text_;
};

// ---- engine models (host glue: the model reader's answer to Part.FindModelTransforms) -----------------------------------------------
// The part-space thrust transforms of the staged stock engines (core/data/engine_thrust_transforms.json, WP-17b: extracted from the
// game's own .mu files by tools/wp17/mu_transforms.py). The shell's model reader (WP-UE-05) replaces this with the .mu node tree later.
struct EngineModels {
    wp17b::EngineThrustTransforms data;
    explicit EngineModels(const std::string& jsonPath) : data(wp17b::EngineThrustTransforms::Load(jsonPath)) {}
    std::vector<PartTransform> Local(const Part& part, const std::string& transformName) const {
        std::vector<PartTransform> out;
        if (part.partInfo == nullptr) return out;
        const auto it = data.Parts().find(part.partInfo->name);
        if (it == data.Parts().end()) return out;
        for (const auto& t : it->second.transforms) {
            if (t.name != transformName) continue;
            PartTransform l;
            l.position = t.partSpacePosition;
            l.rotation = wp17b::EngineThrustTransforms::ChainRotation(t);
            out.push_back(l);
        }
        return out;
    }
    std::vector<ModelTransformChain> Chains(const Part& part, const std::string& name) const {
        std::vector<ModelTransformChain> out;
        if (part.partInfo == nullptr) return out;
        const auto it = data.Parts().find(part.partInfo->name);
        if (it == data.Parts().end()) return out;
        for (const auto& t : it->second.transforms) {
            ModelTransformChain c;
            for (size_t i = 0; i < t.chainNames.size(); ++i) {
                ModelTransformNode n;
                n.name = t.chainNames[i];
                n.localPosition = t.chainPositions[i];
                n.localRotation = t.chainRotations[i];
                n.localScale = t.chainScales[i];
                c.push_back(n);
                if (n.name != name) continue;
                bool seen = false;
                for (const ModelTransformChain& o : out) seen = seen || (o.size() == c.size() && ChainIsAncestorOrSelf(o, c));
                if (!seen) out.push_back(c);
            }
        }
        return out;
    }
    void Install(PartModuleForces& forces) {
        ModuleEngines::findModelTransforms = [this](Part& p, const std::string& name) {
            std::vector<PartTransform> w;
            for (const PartTransform& l : Local(p, name)) w.push_back(ChildTransform(p.partTransform, l));
            return w;
        };
        forces.hooks.engineThrustTransforms = [this](Part& p, const ModuleEngines& e) { return Local(p, e.Get<std::string>("thrustVectorTransformName")); };
        const ModelTransformChains chains = [this](Part& p, const std::string& name) { return Chains(p, name); };
        forces.hooks.modelTransformChains = chains;
        ModuleGimbal::findModelTransforms = GimbalFindModelTransformsHook(chains);
        ModuleGimbal::isThrustTransformInChildren = ThrustBelowGimbalHook(chains);
    }
    ~EngineModels() {
        ModuleEngines::findModelTransforms = nullptr;
        ModuleGimbal::findModelTransforms = nullptr;
        ModuleGimbal::isThrustTransformInChildren = nullptr;
    }
};

// The reduced .mu files of the stock engines behind MuPartColliders (WP-mucol): used for a part whose .mu in the game root is a
// zero-byte stub (the repository's GameData mirror). A real game root reads the .mu itself. Origin: reduced_model_dir/
// mucollider_reference.json (the sha256 of every original is in that file).
std::shared_ptr<const mu::Model> LoadPartModel(const std::string& path, const std::string& reducedDir, const nlohmann::json* ref) {
    std::error_code ec;
    if (fs::exists(path, ec) && fs::file_size(path, ec) > 0) {
        try {
            return std::make_shared<const mu::Model>(mu::ReadModelFile(path));
        } catch (const std::exception&) {
            return nullptr;
        }
    }
    if (ref == nullptr) return nullptr;
    std::string p = path;
    std::replace(p.begin(), p.end(), '\\', '/');
    for (auto it = ref->at("parts").begin(); it != ref->at("parts").end(); ++it) {
        const std::string origin = it.value().at("mu").at("origin").get<std::string>();
        if (p.size() >= origin.size() && p.compare(p.size() - origin.size(), origin.size(), origin) == 0)
            return std::make_shared<const mu::Model>(mu::ReadModelFile(reducedDir + "/" + it.value().at("reduced").at("file").get<std::string>()));
    }
    return nullptr;
}

// ---- the PQS sphere of the dominant body (WP-12a; the logic of kspcore_pqs_* in src/capi/kspcore_c.cpp) ----------------------------------
struct TerrainSphere {
    PQSCache cache;
    std::unique_ptr<PQSHeightWorkers> workers;
    PQSTarget target;
    std::unique_ptr<PQS> sphere;
    PQSMod_CelestialBodyTransform* cbt = nullptr;
    bool started = false;
    std::set<int> known;  // quad ids reported to the shell
    std::map<int, uint32_t> serial;
    uint32_t nextSerial = 1;
    std::vector<PQ*> drawable;

    TerrainSphere(const std::string& body, terrain::MapLibrary& maps, int heightThreads) {
        cache.cachePQInitialCount = 64;
        if (heightThreads > 0) {
            terrain::MapLibrary* m = &maps;
            workers = std::make_unique<PQSHeightWorkers>([m, body] { return terrain::LoadStockPQS(body, *m); }, heightThreads);
        }
        sphere = terrain::CreateStockPQS(body, maps);
        sphere->cache = &cache;
        sphere->heightWorkers = workers.get();
        const PQSTarget* t = &target;
        for (auto& m : sphere->childMods)
            if (auto* cb = dynamic_cast<PQSMod_CelestialBodyTransform*>(m.get())) {
                cb->host.hasBody = [] { return true; };
                cb->host.flightGlobalsExists = [] { return true; };
                cb->host.activeVessel = [t] { return t; };
                cb->host.isCurrentMainBody = [] { return true; };
                cb->host.loadedScene = [] { return static_cast<int>(GameScenes::FLIGHT); };
                cb->host.mapIsEnabled = [] { return false; };
                cbt = cb;
            }
        PQSCache::CreateDefaultPresetList();
        sphere->SetTarget(&target);
    }
    ~TerrainSphere() { PQSCache::PresetList.reset(); }
    void Start() {
        sphere->Start();
        started = true;
    }
    void Update(int fixedUpdates) {
        for (int i = 0; i < fixedUpdates; ++i) sphere->FixedUpdate();
        sphere->FrameUpdate();
        drawable.clear();
        std::vector<PQ*> stack(sphere->quads.begin(), sphere->quads.end());
        while (!stack.empty()) {
            PQ* q = stack.back();
            stack.pop_back();
            if (q->isSubdivided) {
                for (PQ* c : q->subNodes)
                    if (c != nullptr) stack.push_back(c);
            } else if (q->isBuilt && q->rendererEnabled) {
                drawable.push_back(q);
            }
        }
    }
};

}  // namespace

std::u16string Utf8ToUtf16(const char* s) {
    std::u16string out;
    if (s == nullptr) return out;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s);
    while (*p != 0) {
        uint32_t cp = 0;
        int extra = 0;
        if (*p < 0x80) {
            cp = *p;
        } else if ((*p & 0xE0) == 0xC0) {
            cp = *p & 0x1F;
            extra = 1;
        } else if ((*p & 0xF0) == 0xE0) {
            cp = *p & 0x0F;
            extra = 2;
        } else {
            cp = *p & 0x07;
            extra = 3;
        }
        ++p;
        for (int i = 0; i < extra && *p != 0; ++i, ++p) cp = (cp << 6) | (*p & 0x3F);
        if (cp >= 0x10000) {
            cp -= 0x10000;
            out.push_back(static_cast<char16_t>(0xD800 + (cp >> 10)));
            out.push_back(static_cast<char16_t>(0xDC00 + (cp & 0x3FF)));
        } else {
            out.push_back(static_cast<char16_t>(cp));
        }
    }
    return out;
}

// =================================================================================================================================
// the flight scene
// =================================================================================================================================

struct GameHost::Impl;

namespace {

struct SceneEvents;

}  // namespace

struct GameHost::Impl {
    // ---- the Unity objects that outlive a scene: the sim's "process of the game" -------------------------------------------------
    StaticStateGuard guard;  // first member: restored last
    CoroutineScheduler scheduler;
    std::unique_ptr<PlayerLoop> loop;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    HighLogic hl;
    FrameInputDevice device;
    kspcore_input_frame lastInputMeta{};
    bool hasFocus = true;
    std::deque<kspcore_command> commands;
    std::deque<std::string> commandText;
    std::string saveFolder;
    uint32_t guidSeed = 4242u;    // Guid.NewGuid()'s hash source: per sim, so a second sim in the process repeats the first
    int scene = -1;               // the scene core is in (HighLogic.LoadedScene), -1 while a load is pending
    int requestedScene = -1;      // HighLogic.LoadScene recorded this: the old scene goes at the next frame
    bool sceneRunning = false;
    std::string error;
    std::unique_ptr<ScenarioRunner> runner;

    // events (since the last snapshot) and the snapshot arrays
    std::vector<kspcore_event> pendingEvents;
    std::deque<std::string> eventText, snapEventText;
    std::vector<kspcore_event> events;
    std::deque<std::string> snapText;
    std::vector<kspcore_body_state> bodies;
    std::vector<kspcore_vessel_state> vessels;
    std::vector<kspcore_part_state> parts;
    std::vector<kspcore_terrain_quad> quadsAdded;
    std::vector<kspcore_terrain_removed> quadsRemoved;
    kspcore_snapshot_view snapshot{};
    uint64_t frameCount = 0, fixedSteps = 0;
    uint32_t fixedStepsLast = 0, frameChangeSerial = 0;
    int lastFrameKind = KSPCORE_FRAME_ROTATING;

    struct Flight;
    struct Tap;
    std::unique_ptr<Flight> flight;
    std::unique_ptr<Tap> tap;
    VesselCrewManifest launchManifest;  // an unmanned launch: the crew assignment dialog is a UI feature (not in this package)

    Impl();
    ~Impl();
    void PostEvent(int kind, int a = 0, int b = 0, int id = 0, const char* text = nullptr, const double* d = nullptr, int nd = 0);
    CelestialBody& Kerbin() { return *sys->GetBodyByName("Kerbin"); }
    void StartScene(GameScenes scene);
    void EndScene();
    void BuildSnapshot();
    void ApplyCommands();
};

// The flight scene as the Unity scene objects made it, in WP-20g's host-contract order
struct GameHost::Impl::Flight {
    Impl& im;
    const GameConfig& cfg;
    TimeWarp tw;
    TimeWarpEnvironment twEnv;
    SceneryColliders scenery;
    nlohmann::json colliderRef;
    bool haveColliderRef = false;
    std::unique_ptr<MuPartColliders> colliders;
    PhysicsWorldConfig worldCfg;
    std::unique_ptr<PhysicsWorld> world;
    PhysicsBubbleOptions bubbleOptions;
    std::unique_ptr<PhysicsBubble> bubble;
    std::unique_ptr<OrbitPhysicsManager> opm;
    std::unique_ptr<EngineModels> engines;
    FlightInputHandler fih;
    FlightStagingHost stagingHost;
    std::unique_ptr<StageManager> stageManager;
    std::unique_ptr<QuickSaveLoad> quickSave;
    std::unique_ptr<FlightDriver> driver;
    std::unique_ptr<TerrainSphere> sphere;
    PQS cityPqs;
    std::unique_ptr<PQSCity> city;
    std::unique_ptr<SceneryCityHost> kscHost;
    std::unique_ptr<PSystemSetup> setup;
    std::vector<std::unique_ptr<Vessel>> owned;  // vessels the launch path made (the host owns the Vessel objects)
    int spawnIndex = -1;
    std::vector<bool> present;
    std::vector<SceneryObjectPose> poses;
    int levelFrame0 = 0;
    uint64_t sphereFrames = 0;
    bool sasWanted = false;

    Flight(Impl& i, const GameConfig& c) : im(i), cfg(c) {}
    ~Flight();

    void Compose();
    void Awake();
    void LaunchSetup();
    // PlayerLoop scripts
    void FixedUpdateScript();
    void UpdateScript();
    void LateUpdateScript();
    void UpdateTerrain();

    const PQSCityPrefab& KscPrefab() {
        for (const PQSCityPrefab& p : StockPQSCities())
            if (std::string(p.bodyName) == "Kerbin" && std::string(p.name) == "KSC") return p;
        throw std::runtime_error("no stock Kerbin/KSC PQSCity");
    }
    void WorldPose(int i, Vector3d& pos, QuaternionD& rot) {
        CelestialBody& kerbin = im.Kerbin();
        const QuaternionD cityRot(city->localRotation);
        pos = kerbin.position + kerbin.rotation * (Vector3d(city->localPosition) + cityRot * Vector3d(poses[static_cast<size_t>(i)].position));
        rot = kerbin.rotation * cityRot * QuaternionD(poses[static_cast<size_t>(i)].rotation);
    }
    void EnterScene(Vessel& v);
};


// =================================================================================================================================
// Impl: the sim's scene-independent objects
// =================================================================================================================================

namespace {

// Collider.bounds of a part's colliders in its current pose (world AABB): what ShipConstruction.PutShipToGround reads from the part's
// GetComponentsInChildren<Collider>() (ShipConstruction.cs:521); here the bubble's proxies of the part (tests/test_vab_launch.cpp)
std::vector<Bounds> ProxyBounds(const Part& p, const std::vector<ColliderProxy>& proxies) {
    std::vector<Bounds> out;
    for (const ColliderProxy& c : proxies) {
        std::vector<Vector3f> pts;
        auto add = [&](const Vector3f& local) { pts.push_back(c.center + c.rotation * local); };
        switch (c.kind) {
            case ShapeKind::Box:
                for (int k = 0; k < 8; ++k)
                    add(Vector3f((k & 1) ? c.halfExtents.x : -c.halfExtents.x, (k & 2) ? c.halfExtents.y : -c.halfExtents.y, (k & 4) ? c.halfExtents.z : -c.halfExtents.z));
                break;
            case ShapeKind::ConvexHull:
                if (c.hull)
                    for (const Vector3f& h : *c.hull) add(h);
                break;
            default:
                for (int k = 0; k < 8; ++k)
                    add(Vector3f((k & 1) ? c.radius : -c.radius, ((k & 2) ? 1.0f : -1.0f) * (c.halfHeight + c.radius), (k & 4) ? c.radius : -c.radius));
                break;
        }
        if (pts.empty()) continue;
        Vector3f lo(3.4e38f, 3.4e38f, 3.4e38f), hi(-3.4e38f, -3.4e38f, -3.4e38f);
        for (const Vector3f& q : pts) {
            const Vector3f w = p.partTransform.TransformPoint(q);
            lo = Vector3f(std::min(lo.x, w.x), std::min(lo.y, w.y), std::min(lo.z, w.z));
            hi = Vector3f(std::max(hi.x, w.x), std::max(hi.y, w.y), std::max(hi.z, w.z));
        }
        out.push_back(Bounds((lo + hi) * 0.5f, hi - lo));
    }
    return out;
}

}  // namespace

// listeners for the events of the snapshot
struct GameHost::Impl::Tap {
    GameHost::Impl* im;
    explicit Tap(GameHost::Impl* i) : im(i) {
        events::GameEvents::onVesselCreate.Add(this, &Tap::OnCreate);
        events::GameEvents::onVesselDestroy.Add(this, &Tap::OnDestroy);
        events::GameEvents::onVesselChange.Add(this, &Tap::OnChange);
        events::GameEvents::onPartDie.Add(this, &Tap::OnPartDie);
        events::GameEvents::onPartExplode.Add(this, &Tap::OnExplode);
        events::GameEvents::onFloatingOriginShift.Add(this, &Tap::OnShift);
    }
    ~Tap() {
        events::GameEvents::onVesselCreate.Remove(this, &Tap::OnCreate);
        events::GameEvents::onVesselDestroy.Remove(this, &Tap::OnDestroy);
        events::GameEvents::onVesselChange.Remove(this, &Tap::OnChange);
        events::GameEvents::onPartDie.Remove(this, &Tap::OnPartDie);
        events::GameEvents::onPartExplode.Remove(this, &Tap::OnExplode);
        events::GameEvents::onFloatingOriginShift.Remove(this, &Tap::OnShift);
    }
    void OnCreate(Vessel* v) { if (v != nullptr) im->PostEvent(KSPCORE_EVENT_VESSEL_CREATED, 0, 0, static_cast<int>(v->persistentId)); }
    void OnDestroy(Vessel* v) { if (v != nullptr) im->PostEvent(KSPCORE_EVENT_VESSEL_DESTROYED, 0, 0, static_cast<int>(v->persistentId)); }
    void OnChange(Vessel* v) {
        if (v != nullptr) im->PostEvent(KSPCORE_EVENT_VESSEL_SWITCHED, 0, 0, static_cast<int>(v->persistentId));
    }
    void OnPartDie(Part* p) {
        if (p != nullptr) im->PostEvent(KSPCORE_EVENT_PART_DIED, p->vessel != nullptr ? static_cast<int>(p->vessel->persistentId) : 0, 0, static_cast<int>(p->fields.flightID));
    }
    void OnExplode(events::GameEvents::ExplosionReaction r) {
        (void)r;
        im->PostEvent(KSPCORE_EVENT_PART_EXPLODED);
    }
    void OnShift(Vector3d offset, Vector3d offsetNonKrakensbane) {
        (void)offsetNonKrakensbane;
        const double d[3] = {offset.x, offset.y, offset.z};
        im->PostEvent(KSPCORE_EVENT_FLOATING_ORIGIN_REBASE, 0, 0, 0, nullptr, d, 3);
    }
};

GameHost::Impl::Impl() {
    // process start of the game (STATUS.md WP-20g "Host contract"): the World fixture's constructor, over the process-level GameData
    SaveEnvironment::Reset();
    FlightGlobals::Clear();
    GameData& data = ProcessGameData();
    SaveEnvironment& env = SaveEnvironment::Current();
    env.getPartInfoByName = [&data](std::string_view n) { return data.parts.getPartInfoByName(n); };
    env.partExists = [&data](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
    // the v1.8.0 upgrade scripts read the prefab's ctrlSurfaceRange (v180_ModuleControlSurface.cs:52 GetBaseModule): PartLoader's prefab,
    // the LAST module of the class in partPrefab.Modules
    env.prefabModuleFloat = [&data](std::string_view part, std::string_view moduleClass, std::string_view field) -> std::optional<float> {
        const AvailablePart* ap = data.parts.getPartInfoByName(part);
        if (ap == nullptr || !ap->partPrefab) return std::nullopt;
        PartModuleList& mods = ap->partPrefab->Modules();
        for (size_t i = mods.Count(); i-- > 0;) {
            PartModule* m = mods.GetModule(i);
            if (m->ClassName() != moduleClass) continue;
            const ModuleValue v = m->Fields().GetValue(field);
            if (const float* f = std::get_if<float>(&v)) return *f;
            return std::nullopt;
        }
        return std::nullopt;
    };
    // host glue: Guid.NewGuid()'s hash source (deterministic, seeded as the fixtures do)
    env.newGuidHashCode = [this] {
        guidSeed = guidSeed * 1664525u + 1013904223u;
        return static_cast<int32_t>(guidSeed);
    };
    if (data.physics) PhysicsGlobals::SetInstance(data.physics.get());
    planetarium.Awake();
    sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
    env.localBodies = sys->SpawnOrder();
    Vessel::env = VesselEnvironment{};
    VesselModuleManager::CompileModules();
    GameSettings::AwakeOptions so;
    so.gameRoot = GameProcess::Config().gameRoot;
    GameSettings::Awake(so);  // GameSettings.cs:1506
    hl.Awake();               // HighLogic.cs:112
    HighLogic::scheduler = &scheduler;
    HighLogic::sceneLoader = [this](int buildIndex, bool buffered) {
        // host glue: SceneManager.LoadScene(buildIndex): the shell builds the scene's actors and answers kspcore_scene_loaded. A buffered
        // load first goes to the LOADINGBUFFER scene (HighLogic.cs:266), which the shell does not see as a scene of its own.
        if (buffered && buildIndex == static_cast<int>(GameScenes::LOADINGBUFFER)) return;
        requestedScene = buildIndex;
        PostEvent(KSPCORE_EVENT_SCENE_LOAD_REQUESTED, buildIndex);
    };
    loop = std::make_unique<PlayerLoop>(scheduler);
    InputEnvironment::device = &device;
    tap = std::make_unique<Tap>(this);
}

GameHost::Impl::~Impl() {
    tap.reset();
    EndScene();
    runner.reset();
    FlightGlobals::Clear();
    Vessel::SpawnedVessels().clear();
    FlightDriver::ResetStatics();
    QuickSaveLoad::ResetStatics();
    HighLogic::currentGameOwner = nullptr;
    SaveEnvironment::Current().currentGame = nullptr;
    loop.reset();
    sys.reset();
    Planetarium::Zup = Planetarium::AwakeZup(0.0);
    InputEnvironment::device = nullptr;
    InputLockManager::Reset();
    GameSettings::OnDestroy();
    hl.OnDestroy();
    HighLogic::ResetStatics();
    SaveEnvironment::Reset();
    Vessel::env = VesselEnvironment{};
    Time::fixedDeltaTime = 0.02f;
    Time::timeScale = 1.0f;
    Time::deltaTime = 0.02f;
}

void GameHost::Impl::PostEvent(int kind, int a, int b, int id, const char* text, const double* d, int nd) {
    kspcore_event e{};
    e.struct_size = sizeof(kspcore_event);
    e.kind = kind;
    e.a = a;
    e.b = b;
    e.id = id;
    for (int i = 0; i < nd && i < 7; ++i) e.d[i] = d[i];
    if (text != nullptr) {
        eventText.emplace_back(text);
        e.text = eventText.back().c_str();
    }
    pendingEvents.push_back(e);
}

// =================================================================================================================================
// Flight: composition
// =================================================================================================================================

GameHost::Impl::Flight::~Flight() {
    // scene end (STATUS.md WP-20g host contract): the vessels, then the scenery / wheel / servo hosts, then the bubble (Uninstall)
    if (driver) {
        HighLogic::Scheduler().StopAllCoroutines(driver->owner);
        driver->OnDestroy();
    }
    im.loop->RemoveScript("KspHost.Flight");
    FlightGlobals::ready = false;
    FlightGlobals::ActiveVessel = nullptr;
    stageManager.reset();
    quickSave.reset();
    opm.reset();
    FlightGlobals::Clear();
    // the navball speed modes (FlightGlobals.cs:128-130) follow the active vessel's altitude in every FlightGlobals.Update: a flight that
    // reached orbit leaves Orbit; the next scene starts at the Surface default (what the Unity scene's new FlightGlobals object has)
    FlightGlobals::ResetSpeedModesForTests();
    owned.clear();
    Vessel::SpawnedVessels().clear();
    kscHost.reset();
    if (bubble) bubble->Contacts().DropTerrain(im.Kerbin());
    sphere.reset();
    setup.reset();
    space_center::SceneGraph::Reset();
    // (FlightDriver's statics are not reset here: they hold what the NEXT flight scene starts from, set by StartWithNewLaunch /
    // StartAndFocusVessel before HighLogic.LoadScene; the sim's destructor resets them)
    FlightDriver::host = FlightDriver::Host{};
    driver.reset();
    FlightInputHandler::host = FlightInputHost{};
    if (bubble) bubble->SetFlightInputHandler(nullptr);
    fih.OnDestroy();
    FlightInputHandler::fetch = nullptr;
    FlightInputHandler::state = FlightCtrlState();
    HighLogic::LoadedSceneIsFlight = false;
    engines.reset();
    bubble.reset();
    world.reset();
    TimeWarp::fetch = nullptr;
    TimeWarp::environment = nullptr;
    OrbitPhysicsManager::env = OrbitPhysicsManager::Environment{};
    FlightScene::hooks = FlightScene::Hooks{};
    im.Kerbin().inverseRotation = false;
    GameSettings::PHYSICS_EASE = true;
    Vessel::SpawnedVessels().clear();
}

void GameHost::Impl::Flight::Compose() {
    CelestialBody& kerbin = im.Kerbin();
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;  // HighLogic.LoadedSceneIsFlight (module FixedUpdates check it)
    // data: kspcore's derived scenery colliders (WP-12c), the reduced model fallback
    scenery = LoadSceneryColliders(cfg.dataDir + "/scenery/scenery_colliders.json");
    if (!cfg.reducedModelDir.empty()) {
        std::ifstream f(cfg.reducedModelDir + "/mucollider_reference.json", std::ios::binary);
        if (f.good()) {
            colliderRef = nlohmann::json::parse(f);
            haveColliderRef = true;
        }
    }
    // PhysicsGlobals.Start (PhysicsGlobals.cs:2752): bodyLiftCurve = GetLiftingSurfaceCurve("BodyLift")
    PhysicsGlobals::Instance()->bodyLiftCurve = PhysicsGlobals::GetLiftingSurfaceCurve("BodyLift");
    PhysicsGlobals::Instance()->CalculateValues(im.sys->Bodies(), im.planetarium.Sun);
    for (CelestialBody* b : im.sys->Bodies()) b->inverseRotation = false;
    // TimeWarp, the Planetarium clock (tests/physics_bubble_fixture.hpp BubbleScene)
    twEnv.LoadedSceneValue = GameScenes::FLIGHT;
    TimeWarp::environment = &twEnv;
    FlightGlobals::currentMainBody = &kerbin;
    im.planetarium.fixedDeltaTime = 0.02;
    im.planetarium.timeScale = 1.0;
    Time::fixedDeltaTime = 0.02f;
    Time::timeScale = 1.0f;
    // the scene's TimeWarp MonoBehaviour: prefab data, Awake (the singleton), Start (TimeWarp.cs:213: rate index 0 -> curr_rate 1)
    tw.LoadStockPrefab();
    tw.Awake();
    tw.Start();
    GameSettings::ORBIT_DRIFT_COMPENSATION = true;
    Vessel::env.frameCount = kFrameCountBase;
    // the physics world with the game's PhysicMaterials (WP-12c)
    scenery.ApplyTo(worldCfg);
    worldCfg.threads = std::max(1, cfg.physicsThreads + 1);
    world = std::make_unique<PhysicsWorld>(worldCfg);
    // real colliders from the parts' .mu files, drag-cube boxes where there is no readable model (WP-mucol, the FIXTURE of MuPartColliders)
    const std::string root = cfg.gameRoot;
    const std::string reduced = cfg.reducedModelDir;
    const nlohmann::json* ref = haveColliderRef ? &colliderRef : nullptr;
    colliders = std::make_unique<MuPartColliders>([reduced, ref](const std::string& path) { return LoadPartModel(path, reduced, ref); });
    bubbleOptions.vessel.colliders = colliders->Hook();
    bubble = std::make_unique<PhysicsBubble>(*world, bubbleOptions);
    bubble->Install();
    bubble->SetFrameOriginWorld(kerbin.position);  // the physics origin at the dominant body's centre (WP-20b's choice)
    // the rotating frame (OrbitPhysicsManager.checkReferenceFrame below the threshold)
    kerbin.inverseRotation = true;
    im.planetarium.UpdateCBs();
    // the engines with their real thrust transforms (WP-17b data)
    engines = std::make_unique<EngineModels>(cfg.dataDir + "/engine_thrust_transforms.json");
    engines->Install(bubble->ModuleForces());
    // the flight scene's input side as FlightDriver sets it up (tests/test_flight_input_feed.cpp InputScene)
    HighLogic::LoadedSceneIsFlight = true;
    FlightDriver::pause = false;
    FlightInputHandler::host = FlightInputHost{};
    FlightInputHandler::host.postScreenMessage = [this](const std::string& text, float duration) {
        const double d[1] = {static_cast<double>(duration)};
        im.PostEvent(KSPCORE_EVENT_SCREEN_MESSAGE, 0, 0, 0, text.c_str(), d, 1);
    };
    fih.Awake();
    fih.Start();
    bubble->SetFlightInputHandler(&fih);
}

// PQS sphere of Kerbin around the KSC, the KSC colliders at the sandbox facility levels, PSystemSetup's launch sites
void GameHost::Impl::Flight::LaunchSetup() {
    CelestialBody& kerbin = im.Kerbin();
    GameData& data = ProcessGameData();
    (void)data;
    // PQSCity.Orientate (WP-42): the KSC's pose on the body
    cityPqs.radius = kerbin.Radius;
    city = std::make_unique<PQSCity>(KscPrefab(), &kerbin, &cityPqs);
    city->Orientate();
    // Kerbin's PQS sphere as the flight scene has it (PQS.Start, two frames), the target at the KSC until a vessel is active
    terrain::MapLibrary* maps = nullptr;
    maps = &ProcessMaps();
    sphere = std::make_unique<TerrainSphere>("Kerbin", *maps, GameProcess::Config().pqsHeightThreads);
    sphere->target.position = Vector3d(city->localPosition).normalized() * (kerbin.Radius + 70.0);
    sphere->Start();
    for (int f = 0; f < 2; ++f) sphere->Update(1);
    bubble->Contacts().HostTerrain(kerbin, *sphere->sphere);
    // the KSC colliders (WP-12c), every facility at its top level (sandbox)
    const SceneryCity& ksc = *scenery.Find("Kerbin/KSC");
    SceneryLevels lv;
    lv.fallback = -1;
    kscHost = std::make_unique<SceneryCityHost>(*bubble, ksc, 0x5343000000000000ull, lv);
    kscHost->PlaceOnBody(kerbin, Vector3d(city->localPosition), QuaternionD(city->localRotation));
    kscHost->ActivateAll();
    world->OptimizeBroadPhase();
    present = SceneryPresence(ksc, lv);
    poses = SceneryObjectPoses(ksc);
    auto find = [this, &ksc](const std::string& pqsName, const std::string& path) -> std::optional<Vector3d> {
        if (pqsName != "Kerbin") return std::nullopt;
        const int i = SceneryFindObject(ksc, path, present);
        if (i < 0) return std::nullopt;
        Vector3d pos;
        QuaternionD rot;
        WorldPose(i, pos, rot);
        return pos;
    };
    space_center::SceneGraph::FindInPQS = find;
    space_center::SceneGraph::FindChild = [find](const std::string& pqsName, const std::string& parent, const std::string& child) {
        return find(pqsName, parent + "/" + child);
    };
    // PSystemSetup's facility table (data/scenery: WP-12c's extraction of the PSystemSetup prefab)
    setup = std::make_unique<PSystemSetup>();
    setup->Awake();
    setup->pqsArray = {{"Kerbin", sphere->sphere.get()}};
    setup->bodyOfPQS = [&kerbin](const space_center::PQSInfo&) { return &kerbin; };
    for (const SceneryFacility& f : scenery.facilities) {
        PSystemSetup::SpaceCenterFacility fac;
        fac.name = f.name;
        fac.pqsName = f.pqsName;
        fac.facilityTransformName = f.facilityTransformName;
        fac.facilityDisplayName = f.facilityName;
        fac.editorFacility = static_cast<EditorFacility>(f.editorFacility);
        for (const auto& [name, url] : f.spawnPoints) {
            PSystemSetup::SpaceCenterFacility::SpawnPoint sp;
            sp.name = name;
            sp.spawnTransformURL = url;
            fac.spawnPoints.push_back(sp);
        }
        setup->facilities.push_back(std::move(fac));
    }
    setup->SetupFacilities();
    spawnIndex = SceneryFindObject(ksc, "KSC/SpaceCenter/LaunchPad/Facility/LaunchPad_spawn", present);
}

// the vessel enters the flight scene (what Unity runs before its first FixedUpdate: Part.Start with the module lifecycle, the
// Rigidbodies, FlightIntegrator.UpdateMassStats, OrbitDriver.Start; tests/test_vab_launch.cpp)
void GameHost::Impl::Flight::EnterScene(Vessel& v) {
    CelestialBody& kerbin = im.Kerbin();
    StartPartRigidbodies(v);
    v.UpdateMassStats();
    v.loaded = true;
    v.packed = true;
    kerbin.GetLatLonAlt(Vector3d(v.vesselTransform.position), v.latitude, v.longitude, v.altitude);
    v.srfRelRotation = QuaternionExtensions::Inverse(kerbin.rotation.ToQuaternionf()) * v.vesselTransform.rotation;
    {
        // the vessel-frame centre of mass (rb.mass weights over the parts with a Rigidbody)
        Vector3d s;
        double m = 0;
        for (Part* p : v.parts) {
            if (!p->hasRigidbody) continue;
            const double pm = PartRigidbodyMass(*p);
            s += Vector3d(p->fields.orgPos + p->fields.orgRot * p->fields.CoMOffset) * pm;
            m += pm;
        }
        v.localCoM = (s * (1.0 / m)).ToVector3f();
    }
    v.lastBody = &kerbin;
    {
        const double ut = Planetarium::GetUniversalTime();
        const Vector3d rel = Vector3d(v.vesselTransform.position) - kerbin.position;
        auto o = std::make_shared<Orbit>();
        o->UpdateFromStateVectors(rel.xzy(), kerbin.getRFrmVel(Vector3d(v.vesselTransform.position)).xzy(), &kerbin, ut);
        v.orbitDriver->orbit = o;
        v.orbitDriver->updateMode = OrbitDriver::UpdateMode::IDLE;
    }
    if (std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), &v) == FlightGlobals::Vessels.end()) FlightGlobals::AddVessel(&v);
    FlightGlobals::SortVessel(&v, true);
    StartVesselInFlight(v);
    v.UpdateResourceSets();
    v.CoMD = Vector3d(v.vesselTransform.position) + QuaternionD(v.vesselTransform.rotation) * Vector3d(v.localCoM);
    v.orbitDriver->Start();
}

void GameHost::Impl::Flight::Awake() {
    CelestialBody& kerbin = im.Kerbin();
    LaunchSetup();
    // FlightDriver.Awake / Start (FlightDriver.cs:108 / :120)
    driver = std::make_unique<FlightDriver>();
    driver->owner = 1;
    FlightDriver::host = FlightDriver::Host{};
    FlightDriver::host.fileExists = [](const std::string& path) {
        std::error_code ec;
        return fs::exists(fs::path(path), ec);
    };
    FlightDriver::host.frameCount = [] { return Time::frameCount; };
    // FlightDriver.cs:295 FloatingOrigin.SetOffset(spawn position): the spawn transform at the origin of Unity's world
    FlightDriver::host.placeAtSpawn = [](const Vector3d& spawn) { FloatingOrigin::SetOffset(spawn); };
    // FlightDriver.cs:366-369: PutShipToGround on the spawn transform (its Unity world pose after the offset), AssembleForLaunch
    FlightDriver::host.beginNewVessel = [this](ShipConstruct& ship) {
        for (Part* p : ship.liveParts) p->colliderBounds = ProxyBounds(*p, bubbleOptions.vessel.colliders(*p));
        Vector3d spawnPos;
        QuaternionD spawnRot;
        WorldPose(spawnIndex, spawnPos, spawnRot);
        SpawnPoint sp;
        sp.position = spawnPos.ToVector3f();
        sp.rotation = spawnRot.ToQuaternionf();
        ShipConstruction::PutShipToGround(ship, sp);
        Part* root = ship.liveParts[0]->localRoot();
        LaunchPlacement lp;
        lp.rootPosition = root->partTransform.position;
        lp.rootRotation = root->partTransform.rotation;
        Game* game = HighLogic::CurrentGame();
        owned.push_back(ShipConstruction::AssembleForLaunch(ship, FlightDriver::LaunchSiteName, "Launch Pad", FlightDriver::newShipFlagURL, *game, &im.launchManifest, lp));
    };
    ShipConstruction::env.CurrentGame = HighLogic::CurrentGame();
    levelFrame0 = Time::frameCount;
    driver->Awake();
    driver->Start();
    // the new vessel(s) enter the scene; loaded ones (a resumed flight) enter through the bubble's own Vessel.Load
    for (std::unique_ptr<Vessel>& v : owned) EnterScene(*v);
    // StageManager (the flight scene's StageManager MonoBehaviour) and QuickSaveLoad
    stageManager = std::make_unique<StageManager>(&stagingHost);
    stageManager->Awake();
    quickSave = std::make_unique<QuickSaveLoad>();
    QuickSaveLoad::env.postScreenMessage = [this](const std::string& message, float duration) {
        const double d[1] = {static_cast<double>(duration)};
        im.PostEvent(KSPCORE_EVENT_SCREEN_MESSAGE, 0, 0, 0, message.c_str(), d, 1);
    };
    quickSave->start();
    // FlightDriver.Start's focus on a resumed vessel; a new launch: the spawned vessel is the active one
    Vessel* active = !owned.empty() ? owned.front().get() : (FlightGlobals::ActiveVessel != nullptr ? FlightGlobals::ActiveVessel : nullptr);
    if (active == nullptr && !FlightGlobals::Vessels.empty()) active = FlightGlobals::Vessels.front();
    if (active == nullptr) throw std::runtime_error("the flight scene has no vessel");
    // FlightDriver.cs:212-224 (BubbleScene::Start): the world origin moves to the active vessel, FlightGlobals.ready, OrbitPhysicsManager.Start
    FloatingOrigin::SetOffset(Vector3d(active->vesselTransform.position));
    FlightGlobals::ActiveVessel = active;
    FlightGlobals::currentMainBody = active->mainBody();
    FlightGlobals::ready = true;
    Vessel::env.frameCount += 1000;  // every vessel made so far is past its physics hold (Vessel.PhysicsHoldExpired)
    opm = std::make_unique<OrbitPhysicsManager>();
    opm->Start();
    active->MakeActive();
    if (!owned.empty()) {
        active->ResumeStaging();  // Vessel.cs:2140: UpdateStageability(false, false) + CreateIcon
        StageManager::BeginFlight();
        stageManager->FlushSort();
    }
    (void)kerbin;
    Vessel::env.timeSinceLevelLoad = 0.0f;  // Time.timeSinceLevelLoad
}

// ---- PlayerLoop scripts ---------------------------------------------------------------------------------------------------------

// FixedUpdate: Planetarium.FixedUpdate (seq 6) then the bubble's FixedUpdate (seq 7-291); the physics step is inside it
// (tests/physics_bubble_fixture.hpp BubbleScene::Tick)
void GameHost::Impl::Flight::FixedUpdateScript() {
    Vessel::env.frameCount = kFrameCountBase + 1000 + (Time::frameCount - levelFrame0);
    im.planetarium.FixedUpdate();
    tw.FixedUpdate();  // TimeWarp.FixedUpdate (:405), trace seq 19
    bubble->FixedUpdate();
    ++im.fixedSteps;
    ++im.fixedStepsLast;
}

void GameHost::Impl::Flight::UpdateTerrain() {
    // the PQS target is the active vessel's position in the body-fixed frame (PQS.SetTarget: the flight camera / active vessel)
    Vessel* av = FlightGlobals::ActiveVessel;
    CelestialBody& kerbin = im.Kerbin();
    if (av != nullptr && av->mainBody() == &kerbin) {
        const Vector3d rel = Vector3d(av->vesselTransform.position) - kerbin.position;
        sphere->target.position = QuaternionD::Inverse(kerbin.rotation) * rel;
    }
    sphere->Update(1);
    ++sphereFrames;
}

void GameHost::Impl::Flight::UpdateScript() {
    // Time.timeSinceLevelLoad (Vessel.LateUpdate's bounce check at 5 s and updateSituation after 0.5 s read it)
    Vessel::env.timeSinceLevelLoad += Time::deltaTime;
    Vessel::env.deltaTime = Time::deltaTime;
    im.ApplyCommands();
    // TimeWarp.Update (:255): the warp keys of this frame (KeyBinding.GetKeyDown through the input device)
    twEnv.TIME_WARP_INCREASE_down = GameSettings::TIME_WARP_INCREASE->GetKeyDown();
    twEnv.TIME_WARP_DECREASE_down = GameSettings::TIME_WARP_DECREASE->GetKeyDown();
    twEnv.TIME_WARP_STOP_down = GameSettings::TIME_WARP_STOP->GetKeyDown();
    twEnv.MODIFIER_KEY_held = GameSettings::MODIFIER_KEY->GetKey();
    tw.Update();
    // FlightInputHandler.Update (:145: the key toggles); the bubble runs FixedUpdate itself
    fih.Update();
    if (quickSave) {
        quickSave->Update();
        quickSave->RunDelayedCalls(Time::time);
    }
    bubble->Update();
    UpdateTerrain();
}

void GameHost::Impl::Flight::LateUpdateScript() { bubble->LateUpdate(); }

// =================================================================================================================================
// Impl: scenes, commands, snapshot
// =================================================================================================================================

void GameHost::Impl::StartScene(GameScenes s) {
    EndScene();
    // what Unity does on a scene load: the old scene's objects are gone, a new scheduler (Time.time of the scene starts at 0), the scene's
    // flags are set (HighLogic.LoadScene set them already), the ScenarioRunner of the new scene starts
    scheduler = CoroutineScheduler();
    loop = std::make_unique<PlayerLoop>(scheduler);
    HighLogic::scheduler = &scheduler;
    HighLogic::setLevelFlags(s);
    SaveEnvironment::Current().loadedScene = s;
    runner = std::make_unique<ScenarioRunner>();
    scene = static_cast<int>(s);
    requestedScene = -1;
    frameCount = 0;
    fixedSteps = 0;
    if (s == GameScenes::FLIGHT) {
        flight = std::make_unique<Flight>(*this, GameProcess::Config());
        flight->Compose();
        HighLogic::fetch->OnSceneLoaded(static_cast<int>(s), false);
        flight->Awake();
        Flight* f = flight.get();
        loop->AddScript("KspHost.Flight", PlayerLoop::TimingOrderKey(TimingManager::TimingStage::Normal), Delegate<void()>::Bind(f, &Flight::UpdateScript),
                        Delegate<void()>::Bind(f, &Flight::FixedUpdateScript), Delegate<void()>::Bind(f, &Flight::LateUpdateScript));
    } else {
        HighLogic::fetch->OnSceneLoaded(static_cast<int>(s), false);
    }
    sceneRunning = true;
    PostEvent(KSPCORE_EVENT_SCENE_LOADED, static_cast<int>(s));
}

void GameHost::Impl::EndScene() {
    sceneRunning = false;
    flight.reset();
    runner.reset();
}

void GameHost::Impl::ApplyCommands() {
    while (!commands.empty()) {
        const kspcore_command c = commands.front();
        commands.pop_front();
        Vessel* av = FlightGlobals::ActiveVessel;
        switch (c.kind) {
            case KSPCORE_CMD_STAGE:
                if (flight && flight->stageManager) StageManager::ActivateNextStage();
                break;
            case KSPCORE_CMD_THROTTLE:
                FlightInputHandler::state.mainThrottle = static_cast<float>(c.d0);
                break;
            case KSPCORE_CMD_SAS_ENABLE:
                if (av != nullptr) av->ActionGroups().SetGroup(KSPActionGroup::SAS, c.i0 != 0);
                break;
            case KSPCORE_CMD_ACTION_GROUP:
                if (av != nullptr) av->ActionGroups().ToggleGroup(static_cast<KSPActionGroup>(c.i0));
                break;
            case KSPCORE_CMD_TIME_WARP_INDEX:
                if (TimeWarp::fetch != nullptr) {
                    TimeWarp::fetch->Mode = c.i1 != 0 ? TimeWarp::Modes::LOW : TimeWarp::Modes::HIGH;
                    TimeWarp::SetRate(c.i0, false, false);
                }
                break;
            case KSPCORE_CMD_PAUSE:
                FlightDriver::SetPause(c.i0 != 0, false);
                break;
            case KSPCORE_CMD_QUICKSAVE:
                if (flight && flight->quickSave) QuickSaveLoad::QuickSave();
                break;
            case KSPCORE_CMD_QUICKLOAD:
                if (flight && flight->quickSave) flight->quickSave->quickLoad("quicksave", HighLogic::SaveFolder());
                break;
            case KSPCORE_CMD_REVERT_TO_LAUNCH:
                if (flight) FlightDriver::RevertToLaunch();
                break;
            case KSPCORE_CMD_LOAD_SCENE:
                HighLogic::LoadScene(static_cast<GameScenes>(c.i0));
                break;
            default:
                break;  // KSPCORE_CMD_SAS_MODE, SET_ACTIVE_VESSEL, MAP_VIEW, PART_EVENT: WP-UE-04 / later packages
        }
    }
    commandText.clear();
}

// ---- snapshot -------------------------------------------------------------------------------------------------------------------

void GameHost::Impl::BuildSnapshot() {
    snapText.clear();
    bodies.clear();
    vessels.clear();
    parts.clear();
    quadsAdded.clear();
    quadsRemoved.clear();

    kspcore_snapshot_view s{};
    s.struct_size = sizeof(kspcore_snapshot_view);
    kspcore_frame_state& f = s.frame;
    f.struct_size = sizeof(kspcore_frame_state);
    f.ut = Planetarium::fetch != nullptr ? Planetarium::GetUniversalTime() : 0.0;
    f.frame_count = frameCount;
    f.fixed_steps = fixedSteps;
    f.fixed_steps_last = fixedStepsLast;
    f.scene = scene;
    f.dominant_body_id = -1;
    f.frame_kind = lastFrameKind;
    f.frame_change_serial = frameChangeSerial;
    f.fixed_delta_time = Time::fixedDeltaTime;
    f.paused = FlightDriver::pause ? 1 : 0;
    CelestialBody* dominant = nullptr;
    if (flight) {
        dominant = OrbitPhysicsManager::DominantBody();
        if (dominant == nullptr) dominant = FlightGlobals::currentMainBody;
        f.warp_rate = TimeWarp::fetch != nullptr ? TimeWarp::CurrentRate() : 1.0f;
        f.warp_index = TimeWarp::fetch != nullptr ? TimeWarp::CurrentRateIndex() : 0;
        f.warp_mode = TimeWarp::fetch != nullptr && TimeWarp::WarpMode() == TimeWarp::Modes::LOW ? 1 : 0;
        if (dominant != nullptr) {
            f.dominant_body_id = dominant->flightGlobalsIndex;
            const int kind = dominant->inverseRotation ? KSPCORE_FRAME_ROTATING : KSPCORE_FRAME_INERTIAL;
            if (kind != lastFrameKind) {
                ++frameChangeSerial;
                const double d[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
                PostEvent(KSPCORE_EVENT_FRAME_CHANGE, lastFrameKind, kind, 0, nullptr, d, 7);
                lastFrameKind = kind;
            }
            f.frame_kind = kind;
            f.frame_change_serial = frameChangeSerial;
        }
        f.active_vessel_id = FlightGlobals::ActiveVessel != nullptr ? static_cast<int32_t>(FlightGlobals::ActiveVessel->persistentId) : 0;
    } else {
        f.warp_rate = 1.0f;
    }
    s.frame = f;
    // the events of the frame; their texts live in eventText until the next snapshot (a deque swap keeps the element addresses)
    events = pendingEvents;
    pendingEvents.clear();
    snapEventText.clear();
    snapEventText.swap(eventText);
    const Vector3d centre = dominant != nullptr ? dominant->position : Vector3d();
    auto put3 = [](double* out, const Vector3d& v) {
        out[0] = v.x;
        out[1] = v.y;
        out[2] = v.z;
    };
    // bodies
    if (sys && dominant != nullptr) {
        for (CelestialBody* b : sys->Bodies()) {
            kspcore_body_state bs{};
            bs.struct_size = sizeof(kspcore_body_state);
            bs.id = b->flightGlobalsIndex;
            snapText.push_back(b->bodyName);
            bs.name = snapText.back().c_str();
            bs.flags = (b == dominant ? KSPCORE_BODY_DOMINANT : 0u) | KSPCORE_BODY_SCALED_VISIBLE;
            if (b == dominant && flight && flight->sphere && flight->sphere->started && flight->sphere->sphere->isActive) bs.flags |= KSPCORE_BODY_PQS_ACTIVE;
            bs.planet_fade = (b == dominant && flight && flight->sphere && flight->sphere->cbt != nullptr) ? flight->sphere->cbt->planetFade.a : 1.0f;
            put3(bs.position, b->position - centre);
            bs.rotation[0] = b->rotation.x;
            bs.rotation[1] = b->rotation.y;
            bs.rotation[2] = b->rotation.z;
            bs.rotation[3] = b->rotation.w;
            bs.radius = b->Radius;
            bodies.push_back(bs);
        }
        // sun and sky
        s.sun.struct_size = sizeof(kspcore_sun_state);
        if (planetarium.Sun != nullptr) {
            const Vector3d d = (planetarium.Sun->position - centre).normalized();
            put3(s.sun.direction, d);
        }
        const QuaternionD pr = Planetarium::Rotation();
        s.sun.planetarium_rotation[0] = pr.x;
        s.sun.planetarium_rotation[1] = pr.y;
        s.sun.planetarium_rotation[2] = pr.z;
        s.sun.planetarium_rotation[3] = pr.w;
    }
    // vessels and parts (loaded vessels only; an unloaded one has no part transforms)
    if (flight && dominant != nullptr) {
        for (Vessel* v : FlightGlobals::Vessels) {
            if (v == nullptr) continue;
            kspcore_vessel_state vs{};
            vs.struct_size = sizeof(kspcore_vessel_state);
            vs.id = static_cast<int32_t>(v->persistentId);
            snapText.push_back(v->vesselName);
            vs.name = snapText.back().c_str();
            vs.flags = (v->loaded ? KSPCORE_VESSEL_LOADED : 0u) | (v->packed ? KSPCORE_VESSEL_PACKED : 0u) |
                       (v == FlightGlobals::ActiveVessel ? KSPCORE_VESSEL_ACTIVE : 0u) | (v->Landed ? KSPCORE_VESSEL_LANDED : 0u) |
                       (v->Splashed ? KSPCORE_VESSEL_SPLASHED : 0u);
            vs.situation = static_cast<int32_t>(v->situation);
            put3(vs.com, v->CoMD - centre);
            put3(vs.root_position, Vector3d(v->vesselTransform.position) - centre);
            vs.root_rotation[0] = v->vesselTransform.rotation.x;
            vs.root_rotation[1] = v->vesselTransform.rotation.y;
            vs.root_rotation[2] = v->vesselTransform.rotation.z;
            vs.root_rotation[3] = v->vesselTransform.rotation.w;
            vs.first_part = static_cast<uint32_t>(parts.size());
            vs.altitude = (Vector3d(v->vesselTransform.position) - centre).magnitude() - dominant->Radius;  // the root part's altitude (tests: PadScene::RootAltitude)
            vs.vertical_speed = static_cast<float>(v->verticalSpeed);
            if (const FlightIntegrator* fi = flight->bubble->FlightIntegrators().Find(*v)) {
                vs.surface_speed = static_cast<float>(fi->atm.spd);
                vs.mach = static_cast<float>(fi->atm.mach);
            }
            if (v->loaded) {
                for (Part* p : v->parts) {
                    if (p == nullptr) continue;
                    kspcore_part_state ps{};
                    ps.struct_size = sizeof(kspcore_part_state);
                    ps.id = p->fields.flightID;
                    ps.vessel_id = vs.id;
                    ps.model_id = p->partInfo != nullptr ? GameProcess::ModelId(p->partInfo->name) : 0u;
                    put3(ps.position, Vector3d(p->partTransform.position) - centre);
                    ps.rotation[0] = p->partTransform.rotation.x;
                    ps.rotation[1] = p->partTransform.rotation.y;
                    ps.rotation[2] = p->partTransform.rotation.z;
                    ps.rotation[3] = p->partTransform.rotation.w;
                    parts.push_back(ps);
                }
            }
            vs.part_count = static_cast<uint32_t>(parts.size()) - vs.first_part;
            vessels.push_back(vs);
        }
    }
    // terrain: the quads added or rebuilt and removed since the last frame
    if (flight && flight->sphere && dominant != nullptr) {
        TerrainSphere& t = *flight->sphere;
        std::set<int> now;
        for (PQ* q : t.drawable) {
            now.insert(q->id);
            if (t.known.count(q->id) != 0) continue;
            kspcore_terrain_quad tq{};
            kspcore_pqs_quad& out = tq.quad;
            const PQMesh& m = q->mesh;
            const std::vector<int>& idx = t.cache.meshCache.cacheIndices.at(static_cast<size_t>(m.triangleSet));
            auto fp = [](const auto& v) -> const float* { return v.empty() ? nullptr : reinterpret_cast<const float*>(v.data()); };
            out.id = q->id;
            out.subdivision = q->subdivision;
            out.vertex_count = static_cast<int>(m.vertices.size());
            out.index_count = static_cast<int>(idx.size());
            out.origin[0] = q->meshOrigin.x;
            out.origin[1] = q->meshOrigin.y;
            out.origin[2] = q->meshOrigin.z;
            out.positions = fp(m.vertices);
            out.normals = fp(m.normals);
            out.tangents = fp(m.tangents);
            out.uv = fp(m.uv);
            out.uv2 = fp(m.uv2);
            out.uv3 = fp(m.uv3);
            out.uv4 = fp(m.uv4);
            out.colors = fp(m.colors);
            out.indices = idx.data();
            out.collider = q->meshColliderEnabled ? 1 : 0;
            tq.build_serial = t.nextSerial++;
            tq.body_id = dominant->flightGlobalsIndex;
            quadsAdded.push_back(tq);
        }
        for (int id : t.known)
            if (now.count(id) == 0) quadsRemoved.push_back(kspcore_terrain_removed{dominant->flightGlobalsIndex, id});
        t.known = std::move(now);
    }
    s.bodies = bodies.data();
    s.body_count = static_cast<uint32_t>(bodies.size());
    s.vessels = vessels.data();
    s.vessel_count = static_cast<uint32_t>(vessels.size());
    s.parts = parts.data();
    s.part_count = static_cast<uint32_t>(parts.size());
    s.quads_added = quadsAdded.data();
    s.quads_added_count = static_cast<uint32_t>(quadsAdded.size());
    s.quads_removed = quadsRemoved.data();
    s.quads_removed_count = static_cast<uint32_t>(quadsRemoved.size());
    s.events = events.data();
    s.event_count = static_cast<uint32_t>(events.size());
    s.camera.struct_size = sizeof(kspcore_camera_state);
    snapshot = s;
}

uint64_t SnapshotHash(const kspcore_snapshot_view& s) {
    uint64_t h = 14695981039346656037ull;
    Hash1(h, &s.frame.ut, sizeof(double));
    Hash1(h, &s.frame.frame_count, sizeof(uint64_t));
    Hash1(h, &s.frame.fixed_steps, sizeof(uint64_t));
    for (uint32_t i = 0; i < s.body_count; ++i) {
        const kspcore_body_state& b = s.bodies[i];
        Hash1(h, &b.id, sizeof(b.id));
        Hash1(h, b.position, sizeof(b.position));
        Hash1(h, b.rotation, sizeof(b.rotation));
    }
    for (uint32_t i = 0; i < s.vessel_count; ++i) {
        const kspcore_vessel_state& v = s.vessels[i];
        Hash1(h, &v.flags, sizeof(v.flags));
        Hash1(h, &v.situation, sizeof(v.situation));
        Hash1(h, v.com, sizeof(v.com));
        Hash1(h, v.root_position, sizeof(v.root_position));
        Hash1(h, v.root_rotation, sizeof(v.root_rotation));
    }
    for (uint32_t i = 0; i < s.part_count; ++i) {
        const kspcore_part_state& p = s.parts[i];
        Hash1(h, &p.model_id, sizeof(p.model_id));
        Hash1(h, p.position, sizeof(p.position));
        Hash1(h, p.rotation, sizeof(p.rotation));
    }
    return h;
}

// =================================================================================================================================
// GameHost
// =================================================================================================================================

GameHost::GameHost() : impl_(std::make_unique<Impl>()) { impl_->BuildSnapshot(); }
GameHost::~GameHost() = default;

int GameHost::CurrentScene() const { return impl_->scene; }

namespace {

std::string SaveDir(const std::string& saveName) { return (fs::path(GameProcess::Config().savesDir) / saveName).generic_string(); }

}  // namespace

bool GameHost::NewGame(int mode, const std::string& saveName) {
    try {
        if (saveName.empty()) throw std::invalid_argument("save name is empty");
        const Game_Modes m = static_cast<Game_Modes>(mode);
        std::string folder;
        // GamePersistence.CreateNewGame (GamePersistence.cs:304): saves/<name>/{Ships/VAB,Ships/SPH}, persistent.sfs
        std::shared_ptr<GameParameters> params = GameParameters::GetDefaultParameters(m, GameParameters::Preset::Normal);
        // host choice (not KSP's default): CommNet off. A probe core's control level comes from its CommNet connection when CommNet is on
        // (Vessel.GetControlLevel, Vessel.cs:1384), and the network (KSC ground station, link graph) is not part of this package's host:
        // the slice flies with the probe's own control. STATUS.md WP-UE-02 "Not done".
        params->Difficulty.EnableCommNet = false;
        std::unique_ptr<Game> game = GamePersistence::CreateNewGame(
            saveName, m, params, "Squad/Flags/default", GameScenes::SPACECENTER,
            EditorFacility::None, KSPScenarioType::GetAllScenarioTypesInAssemblies(), folder);
        HighLogic::SaveFolder(folder);
        impl_->saveFolder = folder;
        HighLogic::currentGameOwner = std::shared_ptr<Game>(std::move(game));
        HighLogic::CurrentGame(HighLogic::currentGameOwner.get());
        return true;
    } catch (const std::exception& e) {
        error_ = std::string("kspcore_game_new: ") + e.what();
        return false;
    }
}

bool GameHost::LoadGame(const std::string& saveName, const std::string& file) {
    try {
        HighLogic::SaveFolder(saveName);
        std::unique_ptr<Game> game = GamePersistence::LoadGame(file, saveName, true, true);
        if (!game) throw std::runtime_error("no such save or incompatible: " + saveName + "/" + file);
        impl_->saveFolder = saveName;
        HighLogic::currentGameOwner = std::shared_ptr<Game>(std::move(game));
        HighLogic::currentGameOwner->Load();  // Game.cs:539
        return true;
    } catch (const std::exception& e) {
        error_ = std::string("kspcore_game_load: ") + e.what();
        return false;
    }
}

bool GameHost::SaveGame(const std::string& saveName, const std::string& file) {
    try {
        Game* g = HighLogic::CurrentGame();
        if (g == nullptr) throw std::runtime_error("no current game");
        const std::string r = GamePersistence::SaveGame(*g, file, saveName, SaveMode::OVERWRITE);
        return !r.empty();
    } catch (const std::exception& e) {
        error_ = std::string("kspcore_game_save: ") + e.what();
        return false;
    }
}

size_t GameHost::ListSaves(const std::string& saveName, char* buffer, size_t capacity) {
    std::string out;
    std::error_code ec;
    std::vector<std::string> names;
    for (const auto& e : fs::directory_iterator(fs::path(SaveDir(saveName)), ec))
        if (e.is_regular_file() && e.path().extension() == ".sfs") names.push_back(e.path().stem().generic_string());
    std::sort(names.begin(), names.end());
    for (size_t i = 0; i < names.size(); ++i) out += (i != 0 ? "\n" : "") + names[i];
    if (buffer != nullptr && capacity > 0) {
        const size_t n = std::min(out.size(), capacity - 1);
        std::memcpy(buffer, out.data(), n);
        buffer[n] = '\0';
    }
    return out.size();
}

bool GameHost::HasQuickSave() {
    std::error_code ec;
    return fs::exists(fs::path(SaveDir(impl_->saveFolder)) / "quicksave.sfs", ec);
}

bool GameHost::QuickSave() {
    try {
        if (!impl_->flight || !impl_->flight->quickSave) throw std::runtime_error("quicksave needs a running flight scene");
        QuickSaveLoad::QuickSave();  // QuickSaveLoad.cs:189
        return HasQuickSave();
    } catch (const std::exception& e) {
        error_ = std::string("kspcore_game_quicksave: ") + e.what();
        return false;
    }
}

bool GameHost::QuickLoad() {
    try {
        if (!impl_->flight || !impl_->flight->quickSave) throw std::runtime_error("quickload needs a running flight scene");
        if (!HasQuickSave()) throw std::runtime_error("no quicksave in " + impl_->saveFolder);
        impl_->flight->quickSave->quickLoad("quicksave", HighLogic::SaveFolder());  // QuickSaveLoad.cs:252
        return true;
    } catch (const std::exception& e) {
        error_ = std::string("kspcore_game_quickload: ") + e.what();
        return false;
    }
}

bool GameHost::LaunchCraft(const std::string& craftPath, const std::string& launchSite) {
    try {
        if (HighLogic::CurrentGame() == nullptr) throw std::runtime_error("no current game (kspcore_game_new / kspcore_game_load first)");
        std::error_code ec;
        if (!fs::exists(fs::path(craftPath), ec)) throw std::runtime_error("no such craft file: " + craftPath);
        // FlightDriver.StartWithNewLaunch (FlightDriver.cs:461): saves the game as "persistent", then HighLogic.LoadScene(FLIGHT)
        FlightDriver::StartWithNewLaunch(craftPath, "Squad/Flags/default", launchSite, &impl_->launchManifest);
        return true;
    } catch (const std::exception& e) {
        error_ = std::string("kspcore_game_launch_craft: ") + e.what();
        return false;
    }
}

bool GameHost::RevertToLaunch() {
    try {
        if (!impl_->flight || !FlightDriver::CanRevertToPostInit) throw std::runtime_error("nothing to revert to (the flight has not started yet)");
        FlightDriver::RevertToLaunch();  // FlightDriver.cs:475
        return true;
    } catch (const std::exception& e) {
        error_ = std::string("kspcore_game_revert_to_launch: ") + e.what();
        return false;
    }
}

bool GameHost::LoadScene(int scene) {
    try {
        HighLogic::LoadScene(static_cast<GameScenes>(scene));  // HighLogic.cs:211
        return true;
    } catch (const std::exception& e) {
        error_ = std::string("kspcore_game_load_scene: ") + e.what();
        return false;
    }
}

bool GameHost::SceneLoaded(int scene) {
    try {
        impl_->StartScene(static_cast<GameScenes>(scene));
        impl_->BuildSnapshot();
        return true;
    } catch (const std::exception& e) {
        error_ = std::string("kspcore_scene_loaded: ") + e.what();
        impl_->EndScene();
        return false;
    }
}

void GameHost::SetInput(const kspcore_input_frame& in) {
    impl_->device.Set(in);
    impl_->hasFocus = in.has_focus != 0;
    InputEnvironment::screenWidth = in.screen_width > 0 ? in.screen_width : InputEnvironment::screenWidth;
    InputEnvironment::screenHeight = in.screen_height > 0 ? in.screen_height : InputEnvironment::screenHeight;
    InputEnvironment::realtimeSinceStartup = in.real_time_since_startup;
    if (impl_->flight) {
        impl_->flight->fih.OnApplicationFocus(impl_->hasFocus);
        if (in.ui_hover_lock != 0) InputLockManager::SetControlLock(ControlTypes::ALL_SHIP_CONTROLS, "KspUiHover");
        else InputLockManager::RemoveControlLock("KspUiHover");
    }
}

size_t GameHost::Submit(const kspcore_command* commands, size_t count) {
    size_t n = 0;
    for (size_t i = 0; commands != nullptr && i < count; ++i) {
        kspcore_command c = commands[i];
        if (c.text != nullptr) {
            impl_->commandText.emplace_back(c.text);
            c.text = impl_->commandText.back().c_str();
        }
        impl_->commands.push_back(c);
        ++n;
    }
    return n;
}

bool GameHost::Frame(double dtReal) {
    try {
        Impl& im = *impl_;
        // a scene load was requested during the previous frame: the old scene goes now (Unity destroys it at the level switch)
        if (im.requestedScene >= 0 && im.sceneRunning) {
            im.EndScene();
            im.scene = -1;
        }
        im.fixedStepsLast = 0;
        if (im.sceneRunning) {
            im.loop->Frame(static_cast<float>(dtReal));
            ++im.frameCount;
        }
        im.device.EndFrame();
        im.BuildSnapshot();
        return true;
    } catch (const std::exception& e) {
        error_ = std::string("kspcore_frame: ") + e.what();
        return false;
    }
}

const kspcore_snapshot_view& GameHost::Snapshot() { return impl_->snapshot; }

}  // namespace ksp::host
