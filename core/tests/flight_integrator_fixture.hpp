// WP-20d test support: the flight scene with the physics bubble (WP-19b) and the real FlightIntegrator tick (ksp/physics/flight_integrator.hpp)
// over the PhysProf crafts: a craft on the LaunchPad of Kerbin's real PQS terrain (the spike's 30 x 30 mesh patch, in the world's axes),
// the engines with their real thrust transforms (core/data/engine_thrust_transforms.json, WP-17b), launch at full throttle.
//
// Declaration order in a test (the process rule "fixtures declare vessels last"): World, then PadScene, then the vessels.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "engine_thrust_transforms.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/modules/module_command.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/physics/flight_integrator.hpp"
#include "ksp/physics/part_forces.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "physics_bubble_fixture.hpp"

namespace wp20d {

using namespace ksp;
using namespace ksp::physics;
using wp19b::BubbleScene;
using wp20b::World;

// PhysicsGlobals.Start (PhysicsGlobals.cs:2752): bodyLiftCurve = GetLiftingSurfaceCurve("BodyLift"). The WP-20b World runs Awake and
// LoadDatabase only, so without this every DragCubeList made afterwards holds the empty default curve and body lift is zero (KSP's C10
// capture has it at 52 kPa-scaled units at Mach 0.5). Run before the crafts are made (DragCubeList.SetPart copies the pointer).
inline void StartPhysicsGlobals(World& w) { w.pg.bodyLiftCurve = PhysicsGlobals::GetLiftingSurfaceCurve("BodyLift"); }

inline std::filesystem::path ThrustJsonPath() { return std::filesystem::path(KSPCORE_PROJECT_ROOT) / "core" / "data" / "engine_thrust_transforms.json"; }

// The part-space thrust transforms of the staged stock engines (WP-17b data): the model reader's answer to Part.FindModelTransforms.
struct EngineModels {
    wp17b::EngineThrustTransforms data = wp17b::EngineThrustTransforms::Load(ThrustJsonPath());
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
    // ModuleEngines.findModelTransforms (OnLoad / OnStart) and the tick's re-posing hook
    void Install(PartModuleForces& forces) {
        ModuleEngines::findModelTransforms = [this](Part& p, const std::string& name) {
            std::vector<PartTransform> w;
            for (const PartTransform& l : Local(p, name)) w.push_back(ChildTransform(p.partTransform, l));
            return w;
        };
        forces.hooks.engineThrustTransforms = [this](Part& p, const ModuleEngines& e) {
            return Local(p, e.Get<std::string>("thrustVectorTransformName"));
        };
    }
    ~EngineModels() { ModuleEngines::findModelTransforms = nullptr; }
};

// Vessel.resourcePartSet / crossfeedSets (Vessel.cs:3913 UpdateResourceSets) for every flight vessel: the WP-15 seam a host fills
// (ResourceFlowEnvironment::vesselHost, Vessel::env.updateResourceSets), as tests/test_staging.cpp's ResourceSetsHook
struct ResourceSets {
    std::map<Vessel*, VesselResourceSets> sets;
    struct Host : IResourceVessel {
        Vessel* v;
        VesselResourceSets* s;
        Host(Vessel* vv, VesselResourceSets* ss) : v(vv), s(ss) {}
        const std::vector<Part*>& ResourceParts() const override { return v->parts; }
        double RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) override {
            return s->RequestResource(part, id, demand, usePriority, simulate);
        }
        void GetConnectedResourceTotals(int id, bool simulate, double& a, double& m, bool pulling) override {
            s->GetConnectedResourceTotals(id, simulate, a, m, pulling);
        }
        void GetConnectedResourceTotals(int id, bool simulate, double& a, double& m, double th, bool pulling) override {
            s->GetConnectedResourceTotals(id, simulate, a, m, th, pulling);
        }
    };
    std::map<Vessel*, std::unique_ptr<Host>> hosts;
    ResourceSets() {
        ResourceFlowEnvironment::vesselHost = [this](Vessel* v) -> IResourceVessel* {
            auto it = hosts.find(v);
            if (it == hosts.end()) it = hosts.emplace(v, std::make_unique<Host>(v, &sets[v])).first;
            return it->second.get();
        };
        Vessel::env.updateResourceSets = [this](Vessel& v) { sets[&v].UpdateResourceSets(&v, v.parts); };
    }
    ~ResourceSets() {
        ResourceFlowEnvironment::vesselHost = nullptr;
        Vessel::env.updateResourceSets = nullptr;
    }
};

inline std::vector<ModuleEngines*> EnginesOf(Vessel& v) {
    std::vector<ModuleEngines*> out;
    for (Part* p : v.parts)
        for (size_t i = 0; i < p->Modules().Count(); ++i)
            if (auto* e = dynamic_cast<ModuleEngines*>(p->Modules().GetModule(i))) out.push_back(e);
    return out;
}

// Stage-1 engines at full throttle, a controllable vessel (the probe core as a full control source: ModuleCommand's own state
// machine and its electric charge are WP-24's, not in this tick)
inline size_t IgniteFirstStage(Vessel& v, int stage = 1) {
    size_t n = 0;
    for (Part* p : v.parts) {
        if (p->FindModuleOfClass("ModuleCommand") != nullptr) p->fields.isControlSource = static_cast<int32_t>(ControlLevel::FULL);
        for (size_t i = 0; i < p->Modules().Count(); ++i)
            if (auto* e = dynamic_cast<ModuleEngines*>(p->Modules().GetModule(i))) {
                if (stage >= 0 && p->fields.inverseStage != stage) continue;
                e->Activate();
                ++n;
            }
    }
    v.CheckControllable();
    v.ctrlState.mainThrottle = 1.0f;
    return n;
}

// A craft on the LaunchPad in the bubble: Kerbin in its rotating frame (below 100 km, as in KSP), the terrain patch of the spike (30 x 30
// quads of 50 m from Kerbin's PQS heights) as static meshes in the world's axes, the craft upright (craft X, Y, Z -> east, up, north)
// with its lowest proxy collider corner 5 cm above the pad, landed (PRELAUNCH), packed until FlightDriver.Start's frame.
struct PadScene {
    World& w;
    BubbleScene s;
    EngineModels engines;
    ResourceSets resources;
    Vector3d padUp, padEast, padNorth;  // world axes at the pad
    Vector3d padWorld;                  // the pad surface point (world)
    double padRadius = 0.0;
    explicit PadScene(World& w_, const PhysicsBubbleOptions& o = {}) : w(w_), s(w_, o) {
        StartPhysicsGlobals(w);
        CelestialBody& kerbin = w.Kerbin();
        kerbin.inverseRotation = true;  // the rotating frame (OrbitPhysicsManager.checkReferenceFrame below the threshold)
        w.planetarium.UpdateCBs();
        GameSettings::PHYSICS_EASE = true;  // KSP's default (BubbleScene turns it off for its orbit tests)
        engines.Install(s.bubble.ModuleForces());
        // the pad frame in the WORLD axes (CelestialBody.GetSurfaceNVector) and its body-relative direction for the PQS heights
        const Vector3d rel = terrain::GetRelSurfaceNVector(wp20b::kPadLat, wp20b::kPadLon).normalized();
        padRadius = w.pqs ? w.pqs->GetSurfaceHeight(rel, true) : kerbin.Radius;
        padUp = kerbin.GetSurfaceNVector(wp20b::kPadLat, wp20b::kPadLon).normalized();
        const Vector3d polar = (kerbin.rotation * Vector3d(0.0, 1.0, 0.0)).normalized();  // the body's north pole axis (world)
        padNorth = (polar - padUp * Vector3d::Dot(polar, padUp)).normalized();
        padEast = Vector3d::Cross(padUp, padNorth);
        padWorld = kerbin.position + padUp * padRadius;
        AddTerrain(30);
    }
    ~PadScene() {
        GameSettings::PHYSICS_EASE = true;
        w.Kerbin().inverseRotation = false;
    }
    // body-relative point -> world (CelestialBody.GetWorldSurfacePosition's mapping)
    Vector3d WorldOfRel(const Vector3d& rel) const {
        const CelestialBody& k = w.Kerbin();
        return k.BodyFrame.LocalToWorld(rel.xzy()).xzy() + k.position;
    }
    // the spike's terrain patch (wp20b::AddTerrainPatch) with the vertices mapped into the world's axes, at StaticPhysicsFromWorld
    void AddTerrain(int Q, int V = 15, double quadSize = 50.0) {
        const CelestialBody& k = w.Kerbin();
        const Vector3d relUp = terrain::GetRelSurfaceNVector(wp20b::kPadLat, wp20b::kPadLon).normalized();
        const Vector3d relPolar(0.0, 1.0, 0.0);
        const Vector3d relNorth = (relPolar - relUp * Vector3d::Dot(relPolar, relUp)).normalized();
        const Vector3d relEast = Vector3d::Cross(relUp, relNorth);
        const int G = Q * (V - 1) + 1;
        const double step = quadSize / (V - 1), half = 0.5 * Q * quadSize;
        std::vector<Vector3d> grid(static_cast<size_t>(G) * static_cast<size_t>(G));
        for (int z = 0; z < G; z++)
            for (int x = 0; x < G; x++) {
                const Vector3d p = relUp * padRadius + relEast * (-half + x * step) + relNorth * (-half + z * step);
                const Vector3d dir = p.normalized();
                const double r = w.pqs ? w.pqs->GetSurfaceHeight(dir, true) : k.Radius;
                grid[static_cast<size_t>(z) * G + x] = WorldOfRel(dir * r);
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
                        tris.insert(tris.end(), {a, c, b, a, d, c});
                    }
                s.world.AddStaticMesh(s.bubble.StaticPhysicsFromWorld(centre), verts, tris, 0.6f);
            }
        s.world.OptimizeBroadPhase();
    }
    // the craft on the pad, landed and packed (VesselPrecalculate.SetLandedPosRot keeps it there until GoOffRails)
    void Place(Vessel& v) {
        CelestialBody& kerbin = w.Kerbin();
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
        const QuaternionD rot = wp20b::BasisRotation(padEast, padUp, padNorth);
        v.loaded = true;
        v.packed = true;
        v.SetRotation(rot.ToQuaternionf(), true);
        v.SetPosition(padWorld + padUp * (0.05 - minY), true);
        v.Landed = true;
        v.Splashed = false;
        v.situation = Vessel::Situations::PRELAUNCH;
        kerbin.GetLatLonAlt(Vector3d(v.vesselTransform.position), v.latitude, v.longitude, v.altitude);
        v.srfRelRotation = QuaternionExtensions::Inverse(kerbin.rotation.ToQuaternionf()) * v.vesselTransform.rotation;
        v.localCoM = wp19b::VesselFrameCom(v).ToVector3f();
        v.lastBody = &kerbin;
        // the orbit of a vessel standing on the rotating body (OrbitDriver in IDLE: VesselPrecalculate.SetLandedPosRot places it)
        const double ut = Planetarium::GetUniversalTime();
        const Vector3d rel = Vector3d(v.vesselTransform.position) - kerbin.position;
        auto o = std::make_shared<Orbit>();
        o->UpdateFromStateVectors(rel.xzy(), kerbin.getRFrmVel(Vector3d(v.vesselTransform.position)).xzy(), &kerbin, ut);
        v.orbitDriver->orbit = o;
        v.orbitDriver->updateMode = OrbitDriver::UpdateMode::IDLE;
        if (std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), &v) == FlightGlobals::Vessels.end()) FlightGlobals::AddVessel(&v);
        FlightGlobals::SortVessel(&v, true);
        StartPartsInFlight(v);
        v.UpdateResourceSets();
    }
    // the root part's altitude above sea level (Vessel.altitude of the capture: the vessel transform's)
    double RootAltitude(const Vessel& v) const {
        return (Vector3d(v.vesselTransform.position) - w.Kerbin().position).magnitude() - w.Kerbin().Radius;
    }
};

}  // namespace wp20d
