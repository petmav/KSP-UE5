// WP-20d: the FlightIntegrator tick over the rigid bodies (ksp/physics/flight_integrator.hpp, aero_soa.hpp, part_forces.hpp).
//
// Gates (STATUS.md WP-20d):
//   * an independent Python reference (tools/wp20d/fi_reference.py -> tests/data/wp20d_reference.json, from the C# only) for the
//     FlightIntegrator statements the tick adds: SmoothVelocity with its spike rejection, spd / nVel, IntegratePhysicalObjects' acceleration
//     and drag; 0 ULP;
//   * the part force buffers (Part.cs:8663-8688) and Integrate's force half bit-identical to WP-20b's IntegratePartBuffers;
//   * the SoA aero kernel == the exact WP-21 port, every output of every part: on random states of the real drag cubes (every branch), on
//     whole flights from the pad (S, M2-AS, L2-AS: every part every tick, and the same final Jolt state with either kernel); and the kernel
//     against KSP's own numbers: the in-game capture C10 (harness/captures/out2/aero_forces_vessel.json), 0 ULP;
//   * scenarios against independent references: the ascent of PhysProf-M2 / -S / -L2-AS from the pad against KSP's own ascent (C10's flight
//     states), a vacuum burn against an independent integrator and the rocket equation, an orbit coast against the Kepler orbit.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "flight_integrator_fixture.hpp"
#include "ksp/aero/aero_interfaces.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using namespace ksp::physics;
using json = nlohmann::json;

namespace {

const json& Ref() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp20d_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

double D(const json& j) {
    const uint64_t u = std::stoull(j.get<std::string>(), nullptr, 16);
    double d;
    std::memcpy(&d, &u, sizeof d);
    return d;
}
float Fl(const json& j) {
    const uint32_t u = static_cast<uint32_t>(std::stoul(j.get<std::string>(), nullptr, 16));
    float f;
    std::memcpy(&f, &u, sizeof f);
    return f;
}
Vector3d V3(const json& j) { return {D(j[0]), D(j[1]), D(j[2])}; }
Vector3f V3f(const json& j) { return {Fl(j[0]), Fl(j[1]), Fl(j[2])}; }

using wp19b::SameBits;

Part* FindPart(Vessel& v, const std::string& name) {
    for (Part* p : v.parts)
        if (p->partInfo != nullptr && p->partInfo->name == name) return p;
    return nullptr;
}

}  // namespace

// ===================================================================================================================== references

TEST_CASE("WP-20d reference: SmoothVelocity, spd / nVel (FlightIntegrator.cs:495-616), 40 sequences with spikes, 0 ULP") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w;
    auto v = w.Craft("PhysProf-S");  // any vessel: the FlightIntegrator's owner
    size_t steps = 0, spikes = 0;
    for (const json& c : Ref().at("smooth")) {
        FlightIntegrator fi(*v);
        fi.fDeltaTime = static_cast<double>(Fl(c.at("fdt")));
        fi.VelSmoothLenRecip = 1.0 / static_cast<double>(FlightIntegrator::VelSmoothLen);  // Setup :260
        for (const json& s : c.at("steps")) {
            fi.Veld = V3(s.at("veld"));
            fi.Vel = fi.Veld.ToVector3f();  // FixedUpdate :603
            fi.SmoothVelocity();
            const double spd = static_cast<double>(fi.Vel.magnitude());  // :614
            const Vector3f nVel = spd != 0.0 ? fi.Vel / static_cast<float>(spd) : Vector3f::zero();  // :616
            CHECK(SameBits(fi.Vel, V3f(s.at("vel"))));
            CHECK(SameBits(fi.VelUnsmoothed, V3f(s.at("unsmoothed"))));
            CHECK(fi.VelIndex == s.at("index").get<int>());
            CHECK(fi.VelSpiking == s.at("spiking").get<bool>());
            CHECK(fi.lastVelProvisional == s.at("provisional").get<bool>());
            CHECK(SameBits(spd, D(s.at("spd"))));
            CHECK(SameBits(nVel, V3f(s.at("nvel"))));
            spikes += fi.lastVelProvisional || fi.VelSpiking;
            ++steps;
        }
    }
    MESSAGE(steps << " smoothing steps (" << spikes << " in a provisional / spiking state) bit-identical to the reference");
    CHECK(steps == 3200);
    CHECK(spikes > 100);
}

TEST_CASE("WP-20d reference: IntegratePhysicalObjects' acceleration and drag (FlightIntegrator.cs:747-782), 300 cases, 0 ULP") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w;
    auto v = w.Craft("PhysProf-S");
    CelestialBody& body = w.Kerbin();
    const Vector3d pos0 = body.position, omega0 = body.angularVelocity;
    const double g0 = body.gMagnitudeAtCenter;
    const bool inv0 = body.inverseRotation;
    const bool applyDrag0 = PhysicsGlobals::ApplyDrag();
    const float dragMult0 = PhysicsGlobals::DragMultiplier();
    const double gfm0 = PhysicsGlobals::GraviticForceMultiplier;
    FlightIntegrator fi(*v);
    fi.atm.currentMainBody = &body;
    size_t n = 0, applied = 0;
    for (const json& c : Ref().at("physical_objects")) {
        body.position = V3(c.at("body"));
        body.gMagnitudeAtCenter = D(c.at("g"));
        body.angularVelocity = V3(c.at("omega"));
        body.inverseRotation = c.at("inv").get<bool>();
        PhysicsGlobals::GraviticForceMultiplier = D(c.at("mult"));
        fi.CoMd = V3(c.at("comd"));
        fi.Veld = V3(c.at("veld"));
        CHECK(SameBits(fi.PhysicalObjectGravity(), V3f(c.at("acc"))));
        PhysicsGlobals::SetApplyDrag(c.at("applyDrag").get<bool>());
        PhysicsGlobals::SetDragMultiplier(Fl(c.at("dragMultiplier")));
        fi.atm.pseudoReDragMult = D(c.at("pseudoReDragMult"));
        Vector3f drag;
        const bool a = fi.PhysicalObjectDrag(V3f(c.at("rbv")), V3(c.at("frame")), Fl(c.at("origDrag")), D(c.at("density")), drag);
        CHECK(a == c.at("applied").get<bool>());
        if (a) CHECK(SameBits(drag, V3f(c.at("drag"))));
        applied += a;
        ++n;
    }
    body.position = pos0;
    body.angularVelocity = omega0;
    body.gMagnitudeAtCenter = g0;
    body.inverseRotation = inv0;
    PhysicsGlobals::SetApplyDrag(applyDrag0);
    PhysicsGlobals::SetDragMultiplier(dragMult0);
    PhysicsGlobals::GraviticForceMultiplier = gfm0;
    MESSAGE(n << " cases, " << applied << " with drag: bit-identical to the reference");
    CHECK(n == 300);
    CHECK(applied > 100);
}

// ===================================================================================================================== buffers

TEST_CASE("WP-20d part force buffers (Part.cs:8663-8688) and Integrate's force half == WP-20b IntegratePartBuffers, bit for bit") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w;
    auto v = w.Craft("PhysProf-S");
    Part* cone = FindPart(*v, "pointyNoseConeB");
    REQUIRE(cone != nullptr);
    cone->fields.PhysicsSignificance = 1;  // physicsless: its Part.AddForce* go to its RigidBodyPart (the tank below it)
    StartPartRigidbodies(*v);
    v->UpdateMassStats();
    PhysicsWorld world;
    VesselPhysics vp(world, *v, VesselPose{}, {});
    Part* owner = RigidBodyPart(cone);
    REQUIRE(owner != nullptr);
    REQUIRE(owner != cone);
    // Part.AddForce / AddImpulse / AddForceAtPosition / AddTorque write the RigidBodyPart's buffers
    PartAddForce(*cone, Vector3d(1.0, 2.0, 3.0));
    CHECK(owner->fields.force == Vector3d(1.0, 2.0, 3.0));
    CHECK(cone->fields.force.IsZero());
    PartAddImpulse(*cone, Vector3d(0.02, 0.0, 0.0));
    CHECK(owner->fields.force.x == 1.0 + 0.02 / static_cast<double>(TimeWarp::fixedDeltaTime()));
    PartAddForceAtPosition(*cone, Vector3d(0.0, 5.0, 0.0), Vector3d(1.0, 1.0, 1.0));
    REQUIRE(owner->forces.size() == 1);
    CHECK(owner->forces[0].force == Vector3d(0.0, 5.0, 0.0));
    CHECK(owner->forces[0].pos == Vector3d(1.0, 1.0, 1.0));
    PartAddTorque(*cone, Vector3d(0.0, 0.0, 7.0));
    CHECK(owner->fields.torque == Vector3d(0.0, 0.0, 7.0));
    // the same buffers through WP-20b's IntegratePartBuffers and through FlightIntegrator.Integrate: the per-part accumulation of the
    // binding (force and moment of every part, in the order the C# applies them) is bit-identical
    auto fill = [&v](uint32_t seed) {
        uint32_t s = seed;
        auto rnd = [&s] {
            s = s * 1664525u + 1013904223u;
            return static_cast<double>(s >> 8) / 16777216.0 * 200.0 - 100.0;
        };
        for (Part* p : v->parts) {
            p->forces.clear();
            p->fields.force = Vector3d(rnd(), rnd(), rnd());
            p->fields.torque = Vector3d(rnd(), rnd(), rnd());
            const int k = static_cast<int>(std::fabs(rnd())) % 3;
            for (int i = 0; i < k; ++i)
                p->forces.emplace_back(Vector3d(rnd(), rnd(), rnd()), Vector3d(p->partTransform.position) + Vector3d(rnd(), rnd(), rnd()) * 0.01);
        }
    };
    const Vector3d accel(0.1, -9.81, 0.2);
    fill(7);
    vp.IntegratePartBuffers(accel);
    const std::vector<Vector3d> f1 = vp.PartExternalForces(), m1 = vp.PartExternalMoments();
    vp.Flush(0.0f);
    fill(7);
    for (Part* p : v->parts) p->fields.atmDensity = 0.0;  // no air: UpdateAerodynamics adds no force
    FlightIntegrator fi(*v);
    fi.partRef = v->rootPart;
    fi.vessel->precalc.integrationAccel = accel;
    fi.aero.CacheGlobals();
    fi.aero.currentMainBody = &w.Kerbin();
    fi.Integrate(vp);
    const std::vector<Vector3d> f2 = vp.PartExternalForces(), m2 = vp.PartExternalMoments();
    size_t same = 0;
    for (size_t i = 0; i < f1.size(); ++i) same += SameBits(f1[i], f2[i]) && SameBits(m1[i], m2[i]);
    CHECK(same == f1.size());
    CHECK(fi.IntegrateOrder().size() == v->parts.size());
    for (Part* p : v->parts) {
        CHECK(p->forces.empty());
        CHECK(p->fields.force.IsZero());
        CHECK(p->fields.torque.IsZero());
    }
    MESSAGE(same << " / " << f1.size() << " parts: force and moment bit-identical to IntegratePartBuffers");
}

// ===================================================================================================================== SoA == exact

namespace {

// Every part of a vessel, the same inputs through the exact port and through the SoA kernel; returns the parts compared.
struct SoaCompare {
    size_t compared = 0, mismatches = 0;
    size_t drag = 0, lift = 0, submerged = 0, shielded = 0, nonPartRb = 0, fallback = 0;
    std::string first;
    void Run(const FlightIntegratorAero& fi, const std::vector<Part*>& parts, const std::vector<AeroRigidbody>& rbs, AeroSoaKernel& soa) {
        const size_t n = parts.size();
        std::vector<DragCubeList::CubeData> prev(n);
        std::vector<AeroPartOutputs> exact(n);
        for (size_t i = 0; i < n; ++i) prev[i] = parts[i]->DragCubes.GetCubeData();
        for (size_t i = 0; i < n; ++i) exact[i] = AeroPartOutputs::Capture(*parts[i], fi.UpdateAerodynamics<aero::MonoFloat>(*parts[i], rbs[i]));
        for (size_t i = 0; i < n; ++i) parts[i]->DragCubes.SetCubeData(prev[i]);
        std::vector<AeroForces> out;
        soa.Run(fi, parts, rbs, out);
        fallback += soa.Fallback().size();
        for (size_t i = 0; i < n; ++i) {
            const AeroPartOutputs s = AeroPartOutputs::Capture(*parts[i], out[i]);
            std::string what;
            ++compared;
            if (!exact[i].BitEqual(s, &what)) {
                ++mismatches;
                if (first.empty()) first = parts[i]->partInfo->name + ": " + what;
            }
            drag += s.forces.applyDrag;
            lift += s.forces.applyLift;
            submerged += parts[i]->fields.submergedPortion > 0.0 && s.forces.applyDrag;
            shielded += parts[i]->ShieldedFromAirstream();
            nonPartRb += !rbs[i].isPartRb && s.forces.applyDrag;
        }
    }
};

}  // namespace

TEST_CASE("WP-20d SoA aero kernel == the exact port on random states of the real drag cubes (every branch), 0 mismatches") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w;
    wp20d::StartPhysicsGlobals(w);
    auto v = w.Craft("PhysProf-M2-AS");
    CelestialBody& kerbin = w.Kerbin();
    REQUIRE(kerbin.ocean);
    FlightIntegratorAero fi;
    fi.CacheGlobals();
    fi.currentMainBody = &kerbin;
    // the drag cubes as the flight scene has them: weights and occlusion of the assembled vessel (DragCubeSetupAndPartAeroStats)
    fi.DragCubeSetupAndPartAeroStats<aero::MonoFloat>(v->parts, 9.81, [](const Part&) { return 100.0; });
    const bool atParent0 = PhysicsGlobals::ApplyDragToNonPhysicsPartsAtParentCoM();
    const bool nonPhys0 = PhysicsGlobals::ApplyDragToNonPhysicsParts();
    uint32_t s = 2004;
    auto rnd = [&s] {
        s = s * 1664525u + 1013904223u;
        return static_cast<double>(s >> 8) / 16777216.0;
    };
    auto rdir = [&] {
        Vector3d d(rnd() * 2 - 1, rnd() * 2 - 1, rnd() * 2 - 1);
        return d.magnitude() > 1e-3 ? d.normalized() : Vector3d(0, 1, 0);
    };
    AeroSoaKernel soa;
    SoaCompare cmp;
    const std::vector<Part*>& parts = v->parts;
    for (int round = 0; round < 200; ++round) {
        // vessel-wide: Mach (incl. the curves' key region and supersonic), pseudo-Reynolds multiplier, the PhysicsGlobals switches
        fi.mach = round % 17 == 0 ? 0.0 : rnd() * 4.0;
        fi.density = rnd() * 1.3;
        fi.pseudoReDragMult = 0.8 + rnd() * 0.6;
        fi.cacheDragUsesAcceleration = round % 9 == 0;
        PhysicsGlobals::SetApplyDragToNonPhysicsPartsAtParentCoM(round % 2 == 0);
        fi.cacheApplyDragToNonPhysicsParts = round % 5 != 0;
        if (round % 25 == 13) {  // a cube weight change re-weights a part (the kernel's row must follow)
            Part* p = parts[static_cast<size_t>(round) % parts.size()];
            if (!p->DragCubes.Cubes().empty()) {
                p->DragCubes.Cubes()[0].SetWeight(static_cast<float>(0.25 + rnd()));
                p->DragCubes.ForceUpdate(true, true);
                p->DragCubes.SetDragWeights<aero::MonoFloat>();
                p->DragCubes.SetPartOcclusion<aero::MonoFloat>();
            }
        }
        std::vector<AeroRigidbody> rbs(parts.size());
        for (size_t i = 0; i < parts.size(); ++i) {
            Part& p = *parts[i];
            // a random pose (the drag direction in part space), a random air-relative velocity, random part air state
            const Vector3d axis = rdir();
            const double ang = rnd() * 6.283185307179586;
            p.partTransform.rotation = QuaternionD::AngleAxis(ang * 57.29577951308232, axis).ToQuaternionf();
            p.partTransform.position = (rdir() * (rnd() * 30.0)).ToVector3f();
            const double speed = (round + static_cast<int>(i)) % 23 == 0 ? 0.0 : rnd() * 2200.0;
            AeroRigidbody& rb = rbs[i];
            rb.present = (round + static_cast<int>(i)) % 41 != 0;
            rb.partHasRb = (round + static_cast<int>(i)) % 7 != 0;
            rb.isPartRb = rb.partHasRb;
            rb.velocity = (rdir() * speed).ToVector3f();
            rb.worldCenterOfMass = (rdir() * (rnd() * 30.0)).ToVector3f();
            rb.krakensbaneFrameVelocity = round % 3 == 0 ? rdir() * (rnd() * 2000.0) : Vector3d();
            p.fields.atmDensity = (round + static_cast<int>(i)) % 13 == 0 ? 0.0 : rnd() * 1.3;
            p.fields.staticPressureAtm = rnd() * 1.2;
            const double sub = rnd();
            p.fields.submergedPortion = sub < 0.8 ? 0.0 : (sub < 0.9 ? rnd() : 1.0 + rnd());
            p.fields.angularDragByFI = (round + static_cast<int>(i)) % 11 != 0;
            p.fields.hasLiftModule = (round + static_cast<int>(i)) % 19 == 0;
            p.set_ShieldedFromAirstream((round + static_cast<int>(i)) % 29 == 0);
        }
        cmp.Run(fi, parts, rbs, soa);
    }
    PhysicsGlobals::SetApplyDragToNonPhysicsPartsAtParentCoM(atParent0);
    PhysicsGlobals::SetApplyDragToNonPhysicsParts(nonPhys0);
    MESSAGE(cmp.compared << " part states compared, " << cmp.mismatches << " mismatches" << (cmp.first.empty() ? "" : " (first: " + cmp.first + ")")
                         << "; drag " << cmp.drag << ", body lift " << cmp.lift << ", submerged " << cmp.submerged << ", shielded " << cmp.shielded
                         << ", drag on a parent's Rigidbody " << cmp.nonPartRb << ", exact fallback " << cmp.fallback << ", row rebuilds "
                         << soa.RowRebuilds());
    CHECK(cmp.mismatches == 0);
    CHECK(cmp.compared == 200 * parts.size());
    CHECK(cmp.drag > 10000);
    CHECK(cmp.lift > 1000);
    CHECK(cmp.submerged > 100);
    CHECK(cmp.nonPartRb > 100);
    CHECK(soa.RowRebuilds() > parts.size());
}

namespace {

// harness/captures/out2/aero_forces_vessel.json (C10, in game): a table with a column index by name
struct CaptureTable {
    const json* rows = nullptr;
    std::map<std::string, size_t> col;
    CaptureTable(const json& doc, const std::string& name) {
        for (const json& t : doc.at("tables"))
            if (t.at("name").get<std::string>() == name) {
                rows = &t.at("rows");
                size_t i = 0;
                for (const json& c : t.at("columns")) col[c.at("name").get<std::string>()] = i++;
            }
        REQUIRE(rows != nullptr);
    }
    size_t C(const std::string& n) const { return col.at(n); }
    float F(const json& r, const std::string& n) const { return Fl(r.at(C(n + "_hex"))); }
    double Dd(const json& r, const std::string& n) const { return D(r.at(C(n + "_hex"))); }
    std::string S(const json& r, const std::string& n) const { return r.at(C(n)).get<std::string>(); }
};

}  // namespace

TEST_CASE("WP-20d SoA kernel against KSP's own numbers: the C10 in-game capture (aero_forces_vessel), face loop and drag / lift scalars, 0 ULP") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w;
    wp20d::StartPhysicsGlobals(w);
    std::ifstream f(std::string(KSPCORE_HARNESS_DIR) + "/captures/out2/aero_forces_vessel.json", std::ios::binary);
    REQUIRE(f.good());
    const json doc = json::parse(f);
    const CaptureTable states(doc, "states"), parts(doc, "parts"), sweep(doc, "aoa_sweep");
    // the FlightIntegrator caches of the game (Physics.cfg)
    FlightIntegratorAero fi;
    fi.CacheGlobals();
    fi.currentMainBody = &w.Kerbin();
    std::map<std::string, double> pseudoRe;  // craft|state -> FlightIntegrator.pseudoReDragMult
    for (const json& r : *states.rows) pseudoRe[states.S(r, "craft") + "|" + states.S(r, "state")] = states.Dd(r, "pseudoReDragMult");
    std::map<std::string, AeroFaceRow> rowsByPart;  // craft|state|part_index -> the part's live arrays
    const FloatCurve& liftCurve = PhysicsGlobals::BodyLiftCurve()->liftCurve;
    size_t nParts = 0, nSweep = 0, bad = 0, scalars = 0;
    std::string firstBad;
    auto cmpF = [&](float got, float want, const std::string& what) {
        if (!SameBits(got, want)) {
            ++bad;
            if (firstBad.empty()) firstBad = what;
        }
    };
    auto cmpCube = [&](const DragCubeList::CubeData& c, const CaptureTable& t, const json& r, double calcDrag, const std::string& where) {
        cmpF(c.areaDrag, t.F(r, "AreaDrag"), where + " AreaDrag");
        cmpF(c.area, t.F(r, "Area"), where + " Area");
        cmpF(c.exposedArea, t.F(r, "ExposedArea"), where + " ExposedArea");
        cmpF(c.crossSectionalArea, t.F(r, "CrossSectionalArea"), where + " CrossSectionalArea");
        cmpF(c.depth, t.F(r, "Depth"), where + " Depth");
        cmpF(c.dragCoeff, t.F(r, "DragCoeff"), where + " DragCoeff");
        cmpF(c.taperDot, t.F(r, "TaperDot"), where + " TaperDot");
        cmpF(c.liftForce.x, t.F(r, "LiftForce_x"), where + " LiftForce_x");
        cmpF(c.liftForce.y, t.F(r, "LiftForce_y"), where + " LiftForce_y");
        cmpF(c.liftForce.z, t.F(r, "LiftForce_z"), where + " LiftForce_z");
        if (!SameBits(calcDrag, t.Dd(r, "CalculateDragValue"))) {
            ++bad;
            if (firstBad.empty()) firstBad = where + " CalculateDragValue";
        }
    };
    for (const json& r : *parts.rows) {
        if (r.at(parts.C("dragcubes_none")).get<bool>() || r.at(parts.C("ShieldedFromAirstream")).get<bool>()) continue;
        const std::string key = parts.S(r, "craft") + "|" + parts.S(r, "state") + "|" + std::to_string(r.at(parts.C("part_index")).get<int64_t>());
        // the live per-face arrays of SetDragWeights / SetPartOcclusion, as the game had them
        DragCubeList list;
        list.SetPart(nullptr);
        for (int k = 0; k < 6; ++k) {
            const std::string s = std::to_string(k);
            list.AreaOccluded()[static_cast<size_t>(k)] = parts.F(r, "AreaOccluded_" + s);
            list.WeightedDrag()[static_cast<size_t>(k)] = parts.F(r, "WeightedDrag_" + s);
            list.WeightedDepth()[static_cast<size_t>(k)] = parts.F(r, "WeightedDepth_" + s);
        }
        AeroFaceRow row;
        row.Build(list);
        rowsByPart[key] = row;
        const float mach = static_cast<float>(parts.Dd(r, "machNumber"));
        const AeroMachCurves m = AeroMachCurves::Evaluate(mach);
        const Vector3f dirLocal(parts.F(r, "dragVectorDirLocal_x"), parts.F(r, "dragVectorDirLocal_y"), parts.F(r, "dragVectorDirLocal_z"));
        const DragCubeList::CubeData c = AeroSoaKernel::SurfaceDrag(m, row, liftCurve, dirLocal);
        cmpCube(c, parts, r, aero::D(aero::FE<aero::MonoFloat>(c.areaDrag) * aero::FE<aero::MonoFloat>(fi.cacheDragCubeMultiplier)), key);
        // UpdateAerodynamics' scalars on the part's dynamic pressure (FlightIntegrator.cs:2371-2383)
        fi.pseudoReDragMult = pseudoRe.at(parts.S(r, "craft") + "|" + parts.S(r, "state"));
        const double q = parts.Dd(r, "dynamicPressurekPa");
        double num6 = 0.0;
        const float dragScalar = AeroSoaKernel::DragScalar(fi, c.areaDrag, q, 1.0, num6);
        cmpF(dragScalar, parts.F(r, "dragScalar"), key + " dragScalar");
        const AvailablePart* ap = w.data.parts.getPartInfoByName(parts.S(r, "part"));
        REQUIRE(ap != nullptr);
        cmpF(AeroSoaKernel::BodyLiftScalar(fi, m, ap->partPrefab->fields.bodyLiftMultiplier, q), parts.F(r, "bodyLiftScalar"), key + " bodyLiftScalar");
        scalars += 2;
        ++nParts;
    }
    for (const json& r : *sweep.rows) {
        const std::string key = sweep.S(r, "craft") + "|" + sweep.S(r, "state") + "|" + std::to_string(r.at(sweep.C("part_index")).get<int64_t>());
        const auto it = rowsByPart.find(key);
        if (it == rowsByPart.end()) continue;  // a shielded / cube-less part
        const AeroMachCurves m = AeroMachCurves::Evaluate(sweep.F(r, "mach"));
        const Vector3f flow(sweep.F(r, "flow_local_x"), sweep.F(r, "flow_local_y"), sweep.F(r, "flow_local_z"));
        const DragCubeList::CubeData c = AeroSoaKernel::SurfaceDrag(m, it->second, liftCurve, flow);
        cmpCube(c, sweep, r, aero::D(aero::FE<aero::MonoFloat>(c.areaDrag) * aero::FE<aero::MonoFloat>(fi.cacheDragCubeMultiplier)), key + " aoa");
        ++nSweep;
    }
    MESSAGE("C10: " << nParts << " live part states (SetDrag outputs, CalculateDragValue, dragScalar, bodyLiftScalar) and " << nSweep
                    << " AoA-sweep states through the SoA kernel: " << bad << " values differ from the game" << (firstBad.empty() ? "" : " (first: " + firstBad + ")"));
    CHECK(bad == 0);
    CHECK(nParts > 4000);
    CHECK(nSweep > 24000);
}

// ===================================================================================================================== flights

namespace {

// One launch from the pad through the whole flight tick (bubble + FlightIntegrator + module slot): settle, ignite the first stage at full
// throttle, `frames` ticks of flight. Per frame since ignition (0 = the ignition tick) after the tick: the root part's altitude (Vessel.altitude
// of the capture), the FlightIntegrator's smoothed speed and Mach.
struct Flight {
    std::vector<double> alt, spd, mach, aoa;
    double padAlt = 0.0;  // root altitude at ignition
    uint64_t hash = 0;
    AeroKernelStats stats;
    std::vector<AeroPartOutputs> endAero;
    std::vector<double> endTemp;
    int parts = 0, engines = 0;
    double msTick = 0.0, msFi = 0.0;  // per flight tick: the whole tick, the FlightIntegrator slot
    double massStart = 0.0, massEnd = 0.0;
};

Flight Launch(const std::string& craft, AeroKernel kernel, int frames, int settle = 100) {
    using Clock = std::chrono::steady_clock;
    Flight out;
    wp20b::World w(true);
    wp20d::PadScene pad(w);
    pad.s.bubble.FlightIntegrators().kernel = kernel;
    double fiMs = 0.0;
    const std::function<void()> fiSlot = FlightScene::hooks.afterFlightIntegrator;
    FlightScene::hooks.afterFlightIntegrator = [&fiMs, fiSlot] {
        const auto t0 = Clock::now();
        fiSlot();
        fiMs += std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
    };
    auto v = w.Craft(craft);
    pad.Place(*v);
    pad.s.Start(*v);
    out.parts = static_cast<int>(v->parts.size());
    double tickMs = 0.0;
    for (int k = 0; k < settle + frames; ++k) {
        if (k == settle) {
            out.engines = static_cast<int>(wp20d::IgniteFirstStage(*v));
            out.padAlt = pad.RootAltitude(*v);
            out.massStart = v->totalMass;
            fiMs = 0.0;
        }
        const auto t0 = Clock::now();
        pad.s.Tick();
        if (k >= settle) {
            tickMs += std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
            const FlightIntegrator* fi = pad.s.bubble.FlightIntegrators().Find(*v);
            out.alt.push_back(pad.RootAltitude(*v));
            out.spd.push_back(fi != nullptr ? fi->atm.spd : 0.0);
            out.mach.push_back(fi != nullptr ? fi->atm.mach : 0.0);
            // the angle between the vessel's long axis and its air-relative velocity (deg)
            const Vector3d up = Vector3d(v->vesselTransform.rotation * Vector3f::up());
            const Vector3d vel = fi != nullptr ? Vector3d(fi->Vel) : Vector3d();
            out.aoa.push_back(vel.magnitude() > 1.0 ? std::acos(std::max(-1.0, std::min(1.0, Vector3d::Dot(up, vel) / (up.magnitude() * vel.magnitude())))) * 57.29577951308232 : 0.0);
        }
    }
    out.msTick = tickMs / frames;
    out.msFi = fiMs / frames;
    out.massEnd = v->totalMass;
    out.hash = pad.s.world.StateHash();
    const FlightIntegrator* fi = pad.s.bubble.FlightIntegrators().Find(*v);
    REQUIRE(fi != nullptr);
    out.stats = fi->aeroStats;
    for (Part* p : v->parts) {
        out.endAero.push_back(AeroPartOutputs::Capture(*p, AeroForces{}));
        out.endTemp.push_back(p->fields.temperature);
        out.endTemp.push_back(p->fields.skinTemperature);
    }
    return out;
}

}  // namespace

TEST_CASE("WP-20d SoA == exact over whole flights from the pad: S, M2-AS, L2-AS, every output of every part every tick; the same end state with either kernel") {
    if (!wp20b::GameAvailable()) return;
    uint64_t partTicks = 0;
    for (const char* craft : {"PhysProf-S", "PhysProf-M2-AS", "PhysProf-L2-AS"}) {
        const int frames = 1500;  // 30 s of powered flight (the spike's window)
        const Flight both = Launch(craft, AeroKernel::Both, frames);
        const Flight exact = Launch(craft, AeroKernel::Exact, frames);
        const Flight soa = Launch(craft, AeroKernel::SoA, frames);
        size_t sameParts = 0;
        for (size_t i = 0; i < exact.endAero.size(); ++i) sameParts += exact.endAero[i].BitEqual(soa.endAero[i]);
        bool sameTemps = exact.endTemp.size() == soa.endTemp.size();
        for (size_t i = 0; sameTemps && i < exact.endTemp.size(); ++i) sameTemps = SameBits(exact.endTemp[i], soa.endTemp[i]);
        MESSAGE(std::string(craft) << " (" << both.parts << " parts): Both compared " << both.stats.compared << " part-ticks (of " << both.stats.partTicks
                      << ", SoA-covered " << both.stats.covered << "), mismatches " << both.stats.mismatches
                      << (both.stats.firstMismatch.empty() ? "" : " first " + both.stats.firstMismatch) << "; Exact vs SoA: state hash "
                      << std::string(exact.hash == soa.hash ? "equal" : "DIFFERENT") << ", end aero outputs equal on " << sameParts << " / " << exact.endAero.size()
                      << " parts, temperatures " << std::string(sameTemps ? "equal" : "DIFFERENT") << "; altitude after 30 s " << exact.alt.back()
                      << " m; ms per tick: exact " << exact.msTick << " (FI " << exact.msFi << "), SoA " << soa.msTick << " (FI " << soa.msFi << ")");
        CHECK(both.stats.mismatches == 0);
        CHECK(both.stats.compared == both.stats.partTicks);
        CHECK(both.stats.covered == both.stats.partTicks);  // no part fell back to the exact port
        CHECK(both.stats.partTicks == both.stats.ticks * static_cast<uint64_t>(both.parts));  // every part of the Integrate order, every pass
        CHECK(both.stats.ticks >= static_cast<uint64_t>(frames));
        CHECK(exact.hash == soa.hash);
        CHECK(exact.hash == both.hash);
        CHECK(sameParts == exact.endAero.size());
        CHECK(sameTemps);
        CHECK(exact.alt.back() > exact.padAlt + 1000.0);
        partTicks += both.stats.compared;
    }
    MESSAGE(partTicks << " part-ticks compared in total, 0 mismatches");
}

namespace {

// C10's flight states of one craft: the frame, altitude, FlightIntegrator speed and Mach at each threshold (alt_* / mach_*)
struct C10State {
    std::string state;
    int frame = 0;
    double altitude = 0.0, spd = 0.0, mach = 0.0, aoa = 0.0;
};
std::vector<C10State> C10States(const std::string& craft) {
    std::ifstream f(std::string(KSPCORE_HARNESS_DIR) + "/captures/out2/aero_forces_vessel.json", std::ios::binary);
    REQUIRE(f.good());
    const json doc = json::parse(f);
    const CaptureTable t(doc, "states");
    std::vector<C10State> out;
    for (const json& r : *t.rows) {
        if (t.S(r, "craft") != craft) continue;
        C10State s;
        s.state = t.S(r, "state");
        s.frame = static_cast<int>(r.at(t.C("frame")).get<int64_t>());
        s.altitude = t.Dd(r, "altitude");
        s.spd = t.Dd(r, "spd");
        s.mach = t.Dd(r, "mach");
        s.aoa = static_cast<double>(t.F(r, "aoa_deg"));
        out.push_back(s);
    }
    return out;
}

}  // namespace

TEST_CASE("WP-20d ascent from the pad vs KSP's own ascent (C10 in-game states): speed and Mach over altitude, climb times, up to Mach 1") {
    if (!wp20b::GameAvailable()) return;
    // KSP's C10 ascent (harness/captures/out2/aero_forces_vessel.json "states"): the same crafts, first-stage engines at full throttle,
    // vertical, SAS on; a state is the first physics frame at an altitude (1 / 2.5 / 5 km) or Mach number (0.5 / 1). This tick has no SAS
    // (WP-24's FeedInputFeed is not wired): the comparison stops where KSP's own flight still had AoA < 1 deg and ours flies straight
    // (S and M2 lose their attitude past Mach 1 without SAS; KSP's S did too, with it). The capture's frame 0 is not ours (the offset is
    // reported below), so only offset-free quantities are gated: the speed at an altitude, the altitude at a Mach number, the frames
    // between two states.
    struct Window {
        const char* craft;
        double maxAlt;
        double tolSpeed, tolAlt, tolFrames;
    };
    // windows: as long as our AoA stays small (M2 to Mach 1 / 4.7 km, L2-AS to 5 km, S to 1 km: past them the crafts without SAS turn
    // away, S tumbling at 9 s as in KSP); tolerances on the speed at an altitude, the altitude at a Mach number, the frames between states
    for (const Window wd : {Window{"PhysProf-M2", 4700.0, 0.01, 0.015, 0.025}, Window{"PhysProf-L2-AS", 5100.0, 0.01, 0.015, 0.025},
                            Window{"PhysProf-S", 1100.0, 0.01, 0.015, 0.025}}) {
        const std::vector<C10State> ksp = C10States(wd.craft);
        REQUIRE(!ksp.empty());
        const Flight f = Launch(wd.craft, AeroKernel::SoA, 2200);

        int prevKsp = -1, prevOurs = -1;
        double offset = 0.0;
        int n = 0;
        for (const C10State& k : ksp) {
            const bool isAlt = k.state.rfind("alt_", 0) == 0, isMach = k.state.rfind("mach_", 0) == 0;
            if (!(isAlt || isMach) || k.altitude > wd.maxAlt) continue;
            const double threshold = isAlt ? std::stod(k.state.substr(4)) : std::stod(k.state.substr(5));
            int i = -1;
            for (size_t j = 0; j < f.alt.size() && i < 0; ++j)
                if ((isAlt ? f.alt[j] : f.mach[j]) >= threshold) i = static_cast<int>(j);
            REQUIRE(i >= 0);
            const size_t u = static_cast<size_t>(i);
            double maxAoa = 0.0;  // above 30 m/s (the angle of a slow vessel is the direction of its small lateral speeds)
            for (size_t j = 0; j <= u; ++j)
                if (f.spd[j] > 30.0) maxAoa = std::max(maxAoa, f.aoa[j]);
            const double dSpd = f.spd[u] / k.spd - 1.0;
            const double dAlt = f.alt[u] / k.altitude - 1.0;
            MESSAGE(std::string(wd.craft) << " " << k.state << ": KSP frame " << k.frame << " alt " << k.altitude << " spd " << k.spd << " mach " << k.mach << " aoa "
                             << k.aoa << " | ours frame " << i << " alt " << f.alt[u] << " spd " << f.spd[u] << " mach " << f.mach[u]
                             << " max aoa so far " << maxAoa << " | speed " << dSpd * 100.0 << " %, altitude " << dAlt * 100.0 << " %");
            if (isAlt) CHECK(std::fabs(dSpd) < wd.tolSpeed);
            else CHECK(std::fabs(dAlt) < wd.tolAlt);
            if (prevKsp >= 0 && k.frame - prevKsp >= 100) {  // intervals of 2 s or more (shorter ones are dominated by the 1-frame resolution)
                const double dFrames = static_cast<double>(i - prevOurs) / static_cast<double>(k.frame - prevKsp) - 1.0;
                MESSAGE("    frames since the previous state: KSP " << (k.frame - prevKsp) << ", ours " << (i - prevOurs) << " (" << dFrames * 100.0 << " %)");
                CHECK(std::fabs(dFrames) < wd.tolFrames);
            }
            prevKsp = k.frame;
            prevOurs = i;
            offset += static_cast<double>(i - k.frame);
            ++n;
            CHECK(maxAoa < 3.0);  // ours drifts off the velocity from Mach ~0.9 without SAS (M2: 0.58 deg at 2.5 km, 2.6 deg at Mach 1)
        }
        CHECK(n >= 2);
        MESSAGE(std::string(wd.craft) << ": " << n << " states compared; our frames run " << offset / n
                         << " frames behind KSP's (a constant: the capture's frame 0 is not our ignition tick); root altitude at ignition "
                         << f.padAlt << " m (KSP's pad surface is ~7.6 m above the PQS terrain the craft stands on here: PhysProf-S's root is at 79.29 m in C6)");
    }
}
