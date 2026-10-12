// WP-20b test support: the flight-scene data of the PhysProf crafts (GameData mirror, Physics.cfg, the Kerbol system, Kerbin's PQS
// height stack) and the spike's ascent scenario (core/spikes/jolt/src/flight.cpp, part_pipeline.cpp) on the PRODUCTION binding
// (ksp/physics/vessel_physics.hpp). Shared by tests/test_vessel_physics.cpp and the bench tools/wp20b/bench.cpp.
//
// The scenario is the spike's, statement for statement where it matters for the trajectory: the craft is placed 5 cm above the
// pad (craft X, Y, Z -> east, up, north), gravity mu r / r^3 at the vessel CoM with the body-fixed frame's Coriolis and
// centrifugal terms and KSP's 16-tick easing, the WP-09 atmosphere at the CoM altitude, the WP-21 exact aero port per part,
// engine thrust maxThrust * throttle along each engine part's +Y at the part origin (the spike's stand-in for WP-23
// ThrustUpdate), 100 ticks of settle with the engines off, then full throttle. Differences to the spike (documented in STATUS):
// the ground is one static box at the PQS pad height instead of 900 PQS mesh quads (PhysicsWorld has no mesh API before WP-12b),
// the vessel state is VesselPhysics::ReadState (KSP's CalculatePhysicsStats sums, physicsMass weights), angular drag is the
// per-part torque of VesselPhysics::Flush instead of the spike's mass-weighted cluster damping.
#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "game_root.hpp"
#include "ksp/aero/flight_integrator_aero.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/flight_atmosphere.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/physics/physics_world.hpp"
#include "ksp/physics/vessel_physics.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_assembly.hpp"

namespace wp20b {

using namespace ksp;
using namespace ksp::physics;

inline const std::string kHarness = KSPCORE_HARNESS_DIR;
inline std::string CraftDir() { return kHarness + "/../analysis/physics/craft"; }
inline bool GameAvailable() { return std::filesystem::exists(std::filesystem::path(ksp::testing::GameRoot()) / "GameData" / "Squad"); }

// The LaunchPad (tests/test_vessel.cpp World14::PlaceKerbinAtPad, the spike's kPadLat / kPadLon)
constexpr double kPadLat = -0.0972077486779459;
constexpr double kPadLon = -74.5576813534722;

// Game data + PhysicsGlobals + Planetarium + the Kerbol system (as tests/test_vessel.cpp World14), Kerbin's PQS on demand.
struct World {
    GameData data;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    std::unique_ptr<terrain::MapLibrary> maps;
    std::unique_ptr<PQS> pqs;
    std::map<int, std::vector<Vector3d>> terrainGrids;  // the terrain patch heights per patch size (the spike's static cache)
    explicit World(bool loadPqs = false) {
        SaveEnvironment::Reset();
        FlightGlobals::Clear();
        GameData::Options o;
        o.gameRoot = ksp::testing::GameRoot();
        data.Load(o);
        SaveEnvironment& env = SaveEnvironment::Current();
        env.getPartInfoByName = [this](std::string_view n) { return data.parts.getPartInfoByName(n); };
        env.partExists = [this](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
        env.newGuidHashCode = [] {
            static uint32_t s = 4242u;
            s = s * 1664525u + 1013904223u;
            return static_cast<int32_t>(s);
        };
        pg.physicsDatabaseFilename = kHarness + "/inputs/Physics.cfg";
        pg.Awake("");
        if (!pg.LoadDatabase()) throw std::runtime_error("Physics.cfg");
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        Vessel::env = VesselEnvironment{};
        VesselModuleManager::CompileModules();
        if (loadPqs) {
            maps = std::make_unique<terrain::MapLibrary>(std::string(KSPCORE_DATA_DIR) + "/terrain");
            pqs = terrain::LoadStockPQS("Kerbin", *maps);
        }
    }
    ~World() {
        FlightGlobals::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
        SaveEnvironment::Reset();
        Vessel::env = VesselEnvironment{};
    }
    CelestialBody& Kerbin() { return *sys->GetBodyByName("Kerbin"); }
    // ShipConstruct.LoadShip + AssembleVesselFromCraft (WP-04 / 14 / 18) with the root at the origin, then Part.Start's rigid bodies
    std::unique_ptr<Vessel> Craft(const std::string& nameOrPath) {
        std::string path = nameOrPath;
        if (!std::filesystem::exists(path)) path = CraftDir() + "/" + nameOrPath + ".craft";
        ConfigNode::NodePtr root = ConfigNode::Load(path);
        if (!root) throw std::runtime_error("cannot load " + path);
        return Assemble(*root, path);
    }
    // the first `count` PART nodes of a craft file (links to dropped parts removed): small test vessels made of real parts
    std::unique_ptr<Vessel> CraftHead(const std::string& name, size_t count) {
        std::ifstream f(CraftDir() + "/" + name + ".craft", std::ios::binary);
        std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        std::vector<std::string> lines, kept;
        std::string line;
        for (char ch : text) {
            if (ch == '\n') {
                lines.push_back(line);
                line.clear();
            } else if (ch != '\r') {
                line += ch;
            }
        }
        if (!line.empty()) lines.push_back(line);
        std::vector<std::string> names;  // part names (part = x) of the kept PART nodes
        size_t parts = 0;
        for (const std::string& l : lines) {
            const std::string t = l.substr(l.find_first_not_of(" \t") == std::string::npos ? l.size() : l.find_first_not_of(" \t"));
            if (t == "PART" && ++parts > count) break;
            if (t.rfind("part = ", 0) == 0) names.push_back(t.substr(7));
            kept.push_back(l);
        }
        std::vector<std::string> filtered;
        for (const std::string& l : kept) {
            const size_t p = l.find("link = ");
            if (p != std::string::npos && std::find(names.begin(), names.end(), l.substr(p + 7)) == names.end()) continue;
            const size_t q = l.find("attN = ");
            if (q != std::string::npos) {
                const std::string rest = l.substr(l.find(',', q) + 1);
                const std::string target = rest.substr(0, rest.find('|'));
                bool known = false;
                for (const std::string& n : names)
                    if (target.rfind(n, 0) == 0) known = true;
                if (!known) continue;
            }
            filtered.push_back(l);
        }
        std::string joined;
        for (const std::string& l : filtered) joined += l + "\n";
        ConfigNode::NodePtr root = ConfigNode::Parse(joined);
        if (!root) throw std::runtime_error("cannot parse the head of " + name);
        return Assemble(*root, name);
    }
    std::unique_ptr<Vessel> Assemble(ConfigNode& root, const std::string& what) {
        const std::string path = what;
        ShipConstruct ship;
        std::string err;
        if (!ship.LoadShip(root, 0, true, err)) throw std::runtime_error("LoadShip: " + err);
        AssembleOptions ao;
        ao.isLanded = true;
        auto v = AssembleVesselFromCraft(ship, [this](const std::string& n) { return data.parts.getPartInfoByName(n); }, ao);
        if (!v) throw std::runtime_error("AssembleVesselFromCraft " + path);
        StartPartRigidbodies(*v);
        v->UpdateMassStats();  // FlightIntegrator.UpdateMassStats: physicsMass (rb.mass follows from it)
        return v;
    }
};

// Pad frame (the spike's PadFrame): up / east / north at the pad, PQS surface radius
struct PadFrame {
    Vector3d up, east, north;
    double radius = 0.0;
};
inline PadFrame MakePadFrame(World& w) {
    PadFrame f;
    f.up = terrain::GetRelSurfaceNVector(kPadLat, kPadLon).normalized();
    const Vector3d polar(0.0, 1.0, 0.0);
    f.north = (polar - f.up * Vector3d::Dot(polar, f.up)).normalized();
    f.east = Vector3d::Cross(f.up, f.north);
    f.radius = w.pqs ? w.pqs->GetSurfaceHeight(f.up, true) : w.Kerbin().Radius;
    return f;
}

// Rotation taking craft axes (X, Y, Z) to (east, up, north): columns of the basis
inline QuaternionD BasisRotation(const Vector3d& x, const Vector3d& y, const Vector3d& z) {
    const double m00 = x.x, m01 = y.x, m02 = z.x, m10 = x.y, m11 = y.y, m12 = z.y, m20 = x.z, m21 = y.z, m22 = z.z;
    const double tr = m00 + m11 + m22;
    QuaternionD q;
    if (tr > 0) {
        const double s = std::sqrt(tr + 1.0) * 2;
        q = {(m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, 0.25 * s};
    } else if (m00 > m11 && m00 > m22) {
        const double s = std::sqrt(1.0 + m00 - m11 - m22) * 2;
        q = {0.25 * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s};
    } else if (m11 > m22) {
        const double s = std::sqrt(1.0 + m11 - m00 - m22) * 2;
        q = {(m01 + m10) / s, 0.25 * s, (m12 + m21) / s, (m02 - m20) / s};
    } else {
        const double s = std::sqrt(1.0 + m22 - m00 - m11) * 2;
        q = {(m02 + m20) / s, (m12 + m21) / s, 0.25 * s, (m10 - m01) / s};
    }
    return q;
}

// The spike's BuildTerrainPatch: Q x Q quads of 50 m with 15 x 15 vertices, centred on the pad, heights from Kerbin's PQS, each
// quad a static mesh at its centre vertex (physics frame = Kerbin's body-fixed frame, origin at the centre).
inline void AddTerrainPatch(World& w, PhysicsWorld& world, const PadFrame& f, int Q, int V = 15, double quadSize = 50.0) {
    const int G = Q * (V - 1) + 1;
    const double step = quadSize / (V - 1);
    const double half = 0.5 * Q * quadSize;
    const Vector3d padPoint = f.up * f.radius;
    std::vector<Vector3d>& grid = w.terrainGrids[Q];
    if (grid.empty()) {
        grid.resize(static_cast<size_t>(G) * static_cast<size_t>(G));
        for (int z = 0; z < G; z++)
            for (int x = 0; x < G; x++) {
                const Vector3d p = padPoint + f.east * (-half + x * step) + f.north * (-half + z * step);
                const Vector3d dir = p.normalized();
                const double r = w.pqs ? w.pqs->GetSurfaceHeight(dir, true) : w.Kerbin().Radius;
                grid[static_cast<size_t>(z) * G + x] = dir * r;
            }
    }
    for (int qz = 0; qz < Q; qz++)
        for (int qx = 0; qx < Q; qx++) {
            const int cx = qx * (V - 1) + (V - 1) / 2, cz = qz * (V - 1) + (V - 1) / 2;
            const Vector3d centre = grid[static_cast<size_t>(cz) * G + cx];
            std::vector<Vector3f> verts;
            std::vector<uint32_t> tris;
            for (int z = 0; z < V; z++)
                for (int x = 0; x < V; x++) verts.push_back((grid[static_cast<size_t>(qz * (V - 1) + z) * G + (qx * (V - 1) + x)] - centre).ToVector3f());
            for (int z = 0; z < V - 1; z++)
                for (int x = 0; x < V - 1; x++) {
                    const uint32_t a = static_cast<uint32_t>(z * V + x), b = a + 1, c = a + 1 + static_cast<uint32_t>(V), d = a + static_cast<uint32_t>(V);
                    // winding: normals pointing away from the body centre (the spike's Triangle(a, c, b), Triangle(a, d, c))
                    tris.insert(tris.end(), {a, c, b, a, d, c});
                }
            world.AddStaticMesh(centre, verts, tris, 0.6f);
        }
}

struct Percentiles {
    double mean = 0, p50 = 0, p95 = 0, max = 0;
};
inline Percentiles Stats(std::vector<double> v) {
    Percentiles p;
    if (v.empty()) return p;
    double s = 0;
    for (double x : v) s += x;
    p.mean = s / static_cast<double>(v.size());
    std::sort(v.begin(), v.end());
    p.p50 = v[v.size() / 2];
    p.p95 = v[std::min(v.size() - 1, static_cast<size_t>(0.95 * static_cast<double>(v.size())))];
    p.max = v.back();
    return p;
}

struct AscentConfig {
    int ticks = 1500;        // 30 s at 0.02 s
    int settleTicks = 100;   // engines off on the pad first
    double dt = 0.02;
    float throttle = 1.0f;
    bool aero = true;
    bool gravityEase = true;
    ClusterPolicy policy = ClusterPolicy::Whole;
    AngularDragModel angularDrag = AngularDragModel::PerPart;
    float defaultAngularDrag = 0.05f;  // rb.angularDrag before FlightIntegrator sets it (Unity's Rigidbody default)
    int threads = 1;
    int sampleEvery = 10;
    // quads per side of the terrain patch: the spike's 30 (1.5 km). The size matters beyond the craft's footprint: Jolt orders its
    // contact constraints by body id, and on a statically indeterminate support (9-49 engine bells) a different order can leave the
    // contact solver in a small limit cycle (M2-AS on a 4 x 4 patch: 7 cm/s, 9 mm) instead of exact rest (30 x 30, as the spike)
    int terrainQuads = 30;
};

struct AscentSample {
    int tick = 0;
    double altitude = 0, speed = 0;
    Vector3d com;  // body-fixed physics frame
};

struct AscentResult {
    int parts = 0, bodies = 0, constraints = 0;
    double altitudeEnd = 0, speedEnd = 0;
    Vector3d comEnd;
    uint64_t stateHash = 0;
    bool exploded = false;
    // ms per tick over the measured ticks (after the settle): whole tick, ReadState + gravity, per-part pipeline, Flush, Jolt step, AfterStep
    Percentiles total, precalc, pipeline, flush, step, after;
    double maxSpeedAfterSettle = 0, maxDriftAfterSettle = 0;  // pad runs: physics-frame speed / CoM displacement after the settle
    // ms per tick inside the pipeline (measured ticks): transforms + masses, DragCubeSetupAndPartAeroStats, the per-part
    // Rigidbody reads, UpdateAerodynamics, applying the aero forces
    double tTransforms = 0, tAeroSetup = 0, tRbRead = 0, tAero = 0, tApply = 0;
    std::vector<AscentSample> samples;
};

// The spike's flight scenario (pad -> settle -> full-thrust ascent) on VesselPhysics.
inline AscentResult RunAscent(World& w, Vessel& v, const AscentConfig& cfg) {
    using Clock = std::chrono::steady_clock;
    auto ms = [](Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
    AscentResult res;
    CelestialBody& kerbin = w.Kerbin();
    const Vector3d bodyPos = kerbin.position;  // world = bodyPos + physics (frameOriginWorld)
    const double mu = kerbin.gMagnitudeAtCenter;
    const Vector3d omega(0.0, -2.0 * 3.14159265358979323846 / kerbin.rotationPeriod, 0.0);
    const PadFrame pad = MakePadFrame(w);
    // ---- placement: the lowest proxy corner of the FULL parts 5 cm above the pad (craft frame = vessel frame: orgPos / orgRot)
    double minY = 1e30;
    for (Part* p : v.parts) {
        if (!p->hasRigidbody) continue;
        for (const ColliderProxy& b : DragCubeColliderProxy(*p))
            for (int c = 0; c < 8; ++c) {
                const Vector3f corner = b.center + Vector3f((c & 1) ? b.halfExtents.x : -b.halfExtents.x, (c & 2) ? b.halfExtents.y : -b.halfExtents.y,
                                                            (c & 4) ? b.halfExtents.z : -b.halfExtents.z);
                minY = std::min(minY, static_cast<double>(p->fields.orgPos.y) + static_cast<double>((p->fields.orgRot * corner).y));
            }
    }
    VesselPose pose;
    pose.rotation = BasisRotation(pad.east, pad.up, pad.north);
    pose.position = pad.up * (pad.radius + 0.05 - minY);
    PhysicsWorldConfig wc;
    wc.threads = cfg.threads;
    PhysicsWorld world(wc);
    // the ground: the spike's terrain patch (core/spikes/jolt/src/terrain_patch.cpp) around the pad, Kerbin's PQS heights
    // (GetSurfaceHeight with overrideQuadBuildCheck) at 15 x 15 vertices per 50 m quad, one static mesh per quad
    AddTerrainPatch(w, world, pad, cfg.terrainQuads);
    VesselPhysicsOptions opt;
    opt.policy = cfg.policy;
    opt.angularDragModel = cfg.angularDrag;
    opt.defaultAngularDrag = cfg.defaultAngularDrag;
    VesselPhysics vp(world, v, pose, opt);
    vp.frameOriginWorld = bodyPos;
    world.OptimizeBroadPhase();  // as the spike after its build
    res.parts = static_cast<int>(v.parts.size());
    res.bodies = static_cast<int>(vp.ClusterCount());
    res.constraints = static_cast<int>(vp.ConstraintCount());
    // ---- the per-part pipeline (spike PartPipeline, exact kernel)
    FlightIntegratorAtmosphere atm;
    atm.currentMainBody = &kerbin;
    atm.situation = VesselSituations::FLYING;
    FlightIntegratorAero fi;
    fi.CacheGlobals();
    fi.currentMainBody = &kerbin;
    std::vector<Part*> engines;
    std::vector<float> maxThrust;
    for (Part* p : v.parts)
        if (auto* e = dynamic_cast<ModuleEngines*>(p->FindModuleImplementing([](PartModule& m) { return dynamic_cast<ModuleEngines*>(&m) != nullptr; }))) {
            engines.push_back(p);
            maxThrust.push_back(e->maxThrust);
        }
    std::vector<double> tTotal, tPre, tPipe, tFlush, tStep, tAfter;
    const float dt = static_cast<float>(cfg.dt);
    Vector3d settleCom;
    for (int tick = 0; tick < cfg.ticks; ++tick) {
        const auto t0 = Clock::now();
        // ---- VesselPrecalculate: the state (CalculatePhysicsStats sums) and gravity at the CoM
        const VesselPhysicsState st = vp.ReadState();
        const Vector3d com = st.CoMD - bodyPos;  // body-fixed physics frame
        const Vector3d comVel = st.rb_velocityD;
        const double rl = com.magnitude();
        Vector3d g = com * (-mu / (rl * rl * rl));
        g += Vector3d::Cross(omega, comVel) * -2.0;
        g += Vector3d::Cross(omega, Vector3d::Cross(omega, com * -1.0));
        const double ease = cfg.gravityEase ? std::min(1.0, 0.0625 * (tick + 1)) : 1.0;
        g = g * ease;
        const auto t1 = Clock::now();
        // ---- per-part work: transforms in the vessel-local float frame, masses, atmosphere, aero, engines
        vp.floatOrigin = st.CoMD;
        vp.SyncPartTransforms();
        v.UpdateMassStats();
        vp.UpdateMassProperties();
        const auto ta = Clock::now();
        double rbRead = 0, aeroT = 0, apply = 0;
        auto tb = ta;
        const double R = kerbin.Radius;
        const double altitude = rl - R;
        atm.altitude = altitude;
        atm.spd = comVel.magnitude();
        atm.CalculatePressure();
        if (atm.staticPressurekPa > 0.0) atm.CalculateConstantsAtmosphere();
        else atm.CalculateConstantsVacuum();
        fi.mach = atm.mach;
        fi.density = atm.density;
        fi.pseudoReDragMult = atm.pseudoReDragMult;
        if (cfg.aero) {
            const Vector3d ref = st.CoMD - bodyPos;
            fi.DragCubeSetupAndPartAeroStats<aero::MonoFloat>(v.parts, mu / (rl * rl), [&](const Part& p) {
                const Vector3f lp = p.partTransform.position;
                return (ref + Vector3d(lp)).magnitude() - R;
            });
            tb = Clock::now();
            for (Part* p : v.parts) {
                const auto u0 = Clock::now();
                Part* owner = RigidBodyPart(p);
                if (owner == nullptr) continue;
                const PartRigidbodyState ors = vp.RigidbodyState(*owner);
                AeroRigidbody rb;
                rb.present = true;
                rb.isPartRb = p->hasRigidbody;
                rb.partHasRb = p->hasRigidbody;
                rb.velocity = ors.velocity;
                rb.worldCenterOfMass = ors.worldCenterOfMass;
                const auto u1 = Clock::now();
                const AeroForces af = fi.UpdateAerodynamics<aero::MonoFloat>(*p, rb);
                const auto u2 = Clock::now();
                ApplyAeroForces(vp, *p, af);
                const auto u3 = Clock::now();
                rbRead += ms(u0, u1);
                aeroT += ms(u1, u2);
                apply += ms(u2, u3);
            }
        }
        if (tick >= cfg.settleTicks && cfg.throttle > 0.0f)
            for (size_t e = 0; e < engines.size(); ++e) {
                const PartPhysicsState ps = vp.PartState(*engines[e]);
                vp.AddForceAtPhysicsPoint(*engines[e], ps.rotation * Vector3d(0, 1, 0) * static_cast<double>(maxThrust[e] * cfg.throttle), ps.position);
            }
        const auto t2 = Clock::now();
        // ---- FlightIntegrator.Integrate: gravity and the buffered forces into the solver
        vp.AddUniformAcceleration(g);
        vp.Flush(dt);
        const auto t3 = Clock::now();
        world.Step(dt);
        const auto t4 = Clock::now();
        vp.AfterStep();
        const auto t5 = Clock::now();
        if (tick >= cfg.settleTicks) {
            const double nm = static_cast<double>(cfg.ticks - cfg.settleTicks);
            res.tTransforms += ms(t1, ta) / nm;
            res.tAeroSetup += ms(ta, tb) / nm;
            res.tRbRead += rbRead / nm;
            res.tAero += aeroT / nm;
            res.tApply += apply / nm;
            tTotal.push_back(ms(t0, t5));
            tPre.push_back(ms(t0, t1));
            tPipe.push_back(ms(t1, t2));
            tFlush.push_back(ms(t2, t3));
            tStep.push_back(ms(t3, t4));
            tAfter.push_back(ms(t4, t5));
        }
        if (tick == cfg.settleTicks) settleCom = com;
        if (tick > cfg.settleTicks) {
            res.maxSpeedAfterSettle = std::max(res.maxSpeedAfterSettle, comVel.magnitude());
            res.maxDriftAfterSettle = std::max(res.maxDriftAfterSettle, (com - settleCom).magnitude());
        }
        if (tick % cfg.sampleEvery == 0 || tick == cfg.ticks - 1) {
            AscentSample s;
            s.tick = tick;
            s.altitude = altitude;
            s.speed = comVel.magnitude();
            s.com = com;
            res.samples.push_back(s);
            if (!std::isfinite(s.altitude) || !std::isfinite(s.speed) || s.speed > 5.0e4) {
                res.exploded = true;
                break;
            }
        }
    }
    const VesselPhysicsState st = vp.ReadState();
    res.comEnd = st.CoMD - bodyPos;
    res.altitudeEnd = res.comEnd.magnitude() - kerbin.Radius;
    res.speedEnd = st.rb_velocityD.magnitude();
    res.stateHash = world.StateHash();
    res.total = Stats(tTotal);
    res.precalc = Stats(tPre);
    res.pipeline = Stats(tPipe);
    res.flush = Stats(tFlush);
    res.step = Stats(tStep);
    res.after = Stats(tAfter);
    return res;
}

}  // namespace wp20b
