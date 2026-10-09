// WP-21 aero_forces: the stock drag-cube / drag-chain / lifting-surface tables of harness/src/KspGolden/Generators/AeroGen.cs,
// recomputed with kspcore's DragCubeList (src/aero/drag_cube_list.cpp), FlightIntegratorAero (src/aero/flight_integrator_aero.cpp)
// and the lifting-surface maths (src/aero/lifting_surface.cpp).
//
// Two datasets, one templated port:
//   aero_forces       the harness golden harness/golden/aero_forces/aero_forces.json (KSP's IL on .NET Framework 4.8): every
//                     float expression is evaluated with aero::NetFloat (per-operation float32 rounding, RyuJIT SSE2). Tables
//                     drag_curve_value, cube_inputs, cube_weighted, cube_drag, drag_chain_kerbin, lifting_surface.
//   aero_forces_mono  the in-game Mono capture harness/captures/out/aero_forces_mono.json (C-series, the same AeroGen tables
//                     re-run inside KSP): aero::MonoFloat, the production policy. Tables cube_inputs, cube_weighted, cube_drag
//                     (drag_curve_value of that file is WP-09's capture.physics_drag_curve; drag_chain_kerbin and
//                     lifting_surface were not re-run in game).
// Inputs (never golden outputs): harness/inputs/PartDatabase.cfg (the harness's copy, sha256 0e876e05... = the capture's
// provenance `partdatabase_sha256`), harness/inputs/Physics.cfg through CelestialEnv, the generator's literal grids
// (AeroGen.cs:13-46, 72-78, 165-200), and for the Mono dataset the flow / Mach rows of harness/captures/in/aero_in.tsv (the
// capture's own input file: the .NET-normalised directions it was fed).
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "celestial_env.hpp"
#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/aero/flight_integrator_aero.hpp"
#include "ksp/aero/lifting_surface.hpp"
#include "ksp/celestial/flight_atmosphere.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"

using namespace ksp;
using golden::ColType;

namespace {

// AeroGen.cs:13
const char* const kPartPicks[] = {
    "aerodynamicNoseCone/noseCone", "mk1Pod_v2/mk1pod_v2", "fuelTankT400/fuelTank", "Rockomax32/Rockomax32_BW", "connector1/wingConnector",
    "wings/delta/deltaWing", "HeatShield1/HeatShield1", "liquidEngineLV-T45_v2/liquidEngine2_v2", "liquidEngineMainsail_v2/liquidEngineMainsail_v2",
    "parachuteMk1/parachuteSingle", "Airbrake/airbrake1", "structuralPanel1x1/structuralPanel1", "bicoupler/mk2_1m_Bicoupler",
    "decouplerRadialTT-38K/radialDecoupler", "LFO_short/mk2FuselageShortLFO", "Size1_Tanks/fuelTankT800/fuelTank_long",
};
// AeroGen.cs:46
const float kMachs[] = {0.0f, 0.3f, 0.6f, 0.85f, 0.95f, 1.0f, 1.1f, 1.5f, 2.0f, 3.0f, 5.0f, 10.0f, 25.0f};

struct Dir {
    std::string id;
    Vector3f dir;
};

// AeroGen.cs:20 Directions(): N() = new Vector3(x, y, z).normalized (managed Unity under policy P)
template <class P>
std::vector<Dir> Directions() {
    auto N = [](float x, float y, float z) { return aero::U<P>::Normalize(Vector3f(x, y, z)); };
    std::vector<Dir> d = {{"+X", Vector3f::right()}, {"-X", Vector3f::left()}, {"+Y", Vector3f::up()},
                          {"-Y", Vector3f::down()},  {"+Z", Vector3f::forward()}, {"-Z", Vector3f::back()}};
    const int s[] = {1, -1};
    auto sg = [](int v) { return std::string(v > 0 ? "+1" : "-1"); };  // {a:+0;-0}
    for (int a : s)
        for (int b : s) {
            d.push_back({"xy" + sg(a) + sg(b), N(static_cast<float>(a), static_cast<float>(b), 0.0f)});
            d.push_back({"yz" + sg(a) + sg(b), N(0.0f, static_cast<float>(a), static_cast<float>(b))});
            d.push_back({"xz" + sg(a) + sg(b), N(static_cast<float>(a), 0.0f, static_cast<float>(b))});
            for (int c : s)
                d.push_back({"xyz" + sg(a) + sg(b) + sg(c), N(static_cast<float>(a), static_cast<float>(b), static_cast<float>(c))});
        }
    for (int aoa : {2, 5, 10, 15, 20, 30, 45, 60, 75, 89, 120, 150, 175}) {
        const double r = static_cast<double>(aoa) * 3.141592653589793 / 180.0;
        d.push_back({"aoa" + std::to_string(aoa),
                     Vector3f(0.0f, static_cast<float>(dotnet::Math::Cos(r)), static_cast<float>(dotnet::Math::Sin(r)))});
    }
    d.push_back({"odd1", N(0.3f, 0.9f, -0.2f)});
    d.push_back({"odd2", N(-0.7f, 0.1f, 0.65f)});
    return d;
}

struct Case {
    std::string id;
    std::unique_ptr<Part> part;  // AeroGen.HostPart(): no attach nodes, no surface node, dragModel CUBE
};

bool EndsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// AeroGen.cs:82-127: the cube cases (one per part; per named cube as well when a part has several) and the cube_inputs /
// cube_weighted tables.
template <class P>
std::vector<Case> BuildCases(const RunnerOptions& opt, golden::Table& tcubes, golden::Table& tw) {
    ConfigNode::NodePtr db = ConfigNode::Load(opt.harness + "/inputs/PartDatabase.cfg");
    if (!db) throw std::runtime_error("aero_forces: cannot read harness/inputs/PartDatabase.cfg");
    const std::vector<ConfigNode*> partNodes = db->GetNodes("PART");
    std::vector<Case> cases;
    for (const char* pick : kPartPicks) {
        const ConfigNode* node = nullptr;
        for (const ConfigNode* n : partNodes) {
            const std::string* u = n->GetValuePtr("url");
            if (EndsWith(u ? *u : std::string(), std::string("/") + pick)) {
                node = n;
                break;
            }
        }
        if (node == nullptr) continue;  // KspEnv.Notes "aero: no PartDatabase entry matching ..." (none for the stock database)
        const ConfigNode* cubeNode = node->GetNode("DRAG_CUBE");
        const std::string url = *node->GetValuePtr("url");
        const std::string partName = url.substr(url.rfind('/') + 1);
        DragCubeList probe;
        probe.LoadCubes(*cubeNode);
        std::vector<std::string> names;
        for (const DragCube& c : probe.Cubes()) names.push_back(c.Name());
        std::vector<const std::string*> variants = {nullptr};
        if (names.size() > 1)
            for (const std::string& n : names) variants.push_back(&n);
        std::string cubesText;
        {
            const std::vector<std::string> vals = cubeNode->GetValues("cube");
            for (size_t i = 0; i < vals.size(); i++) cubesText += (i ? " | " : "") + vals[i];
        }
        for (const std::string* only : variants) {
            auto part = std::make_unique<Part>();
            part->fields.dragModel = 4;  // Part.DragModel.CUBE
            DragCubeList& dcl = part->DragCubes;
            dcl.LoadCubes(*cubeNode);
            dcl.SetPart(part.get());
            if (only != nullptr)
                for (const std::string& n : names) dcl.SetCubeWeight(n, n == *only ? 1.0f : 0.0f);
            dcl.SetDragWeights<P>();
            dcl.SetPartOcclusion<P>();
            const std::string id = partName + (only == nullptr ? "" : "@" + *only);
            std::string weights;
            for (size_t i = 0; i < dcl.Cubes().size(); i++)
                weights += (i ? " " : "") + dcl.Cubes()[i].Name() + "=" + ExactFormatFloat(dcl.Cubes()[i].Weight());
            tcubes.Add({id, url, cubesText, weights, dcl.PostOcclusionArea()});
            for (int f = 0; f < 6; f++)
                tw.Add({id, static_cast<int64_t>(f), dcl.WeightedArea()[f], dcl.WeightedDrag()[f], dcl.WeightedDepth()[f],
                        dcl.AreaOccluded()[f]});
            cases.push_back({id, std::move(part)});
        }
    }
    return cases;
}

void CubeTables(golden::Table& tcubes, golden::Table& tw, golden::Table& td) {
    tcubes.Col("case_id", ColType::Str).Col("url", ColType::Str).Col("cubes", ColType::Str).Col("weights", ColType::Str);
    tcubes.Col("postOcclusionArea", ColType::F32);
    tcubes.key = {"case_id"};
    tw.Col("case_id", ColType::Str).Col("face", ColType::I64).Col("weightedArea", ColType::F32).Col("weightedDrag", ColType::F32);
    tw.Col("weightedDepth", ColType::F32).Col("areaOccluded", ColType::F32);
    tw.key = {"case_id", "face"};
    td.Col("case_id", ColType::Str).Col("dir_id", ColType::Str).Col("flow_x", ColType::F32).Col("flow_y", ColType::F32);
    td.Col("flow_z", ColType::F32).Col("mach", ColType::F32);
    for (const char* c : {"Area", "AreaDrag", "DragCoeff", "CrossSectionalArea", "ExposedArea", "Depth", "TaperDot", "LiftForce_x",
                          "LiftForce_y", "LiftForce_z"})
        td.Col(c, ColType::F32);
    td.Col("CalculateDragValue", ColType::F64);
    td.key = {"case_id", "dir_id", "mach"};
}

// AeroGen.cs:94-98 HostFlightIntegrator + the four caches
FlightIntegratorAero HostAero() {
    FlightIntegratorAero fi;
    fi.cacheDragCubeMultiplier = PhysicsGlobals::DragCubeMultiplier();
    fi.cacheDragMultiplier = PhysicsGlobals::DragMultiplier();
    fi.cacheBodyLiftMultiplier = PhysicsGlobals::BodyLiftMultiplier();
    fi.cacheDragCubesUseSpherical = PhysicsGlobals::DragCubesUseSpherical();
    return fi;
}

template <class P>
void AddCubeDragRow(golden::Table& td, const std::string& id, const std::string& dirId, const Vector3f& dir, float m, Part& part,
                    const FlightIntegratorAero& fi) {
    DragCubeList& dcl = part.DragCubes;
    dcl.SetDrag<P>(-dir, m);
    const double dv = fi.CalculateDragValue<P>(part);
    const Vector3f lf = dcl.LiftForce();
    td.Add({id, dirId, dir.x, dir.y, dir.z, m, dcl.Area(), dcl.AreaDrag(), dcl.DragCoeff(), dcl.CrossSectionalArea(), dcl.ExposedArea(),
            dcl.Depth(), dcl.TaperDot(), lf.x, lf.y, lf.z, dv});
}

const CelestialBody* FindKerbin(CelestialEnv& env) {
    for (CelestialBody* b : env.system->SpawnOrder())
        if (b->bodyName == "Kerbin") return b;
    throw std::runtime_error("aero_forces: no Kerbin");
}

// ---- aero_forces: the .NET harness golden (NetFloat) --------------------------------------------------------------------
int RunAeroForces(const RunnerOptions& opt) {
    using P = aero::NetFloat;
    CelestialEnv& env = CelestialEnv::Get(opt);
    golden::File g;
    g.dataset = "aero_forces";
    g.description = "kspcore candidate for the AeroGen tables under the .NET Framework float policy (aero::NetFloat).";

    // drag_curve_value (AeroGen.cs:68-78)
    golden::Table tdc("drag_curve_value", "PhysicsGlobals.DragCurveValue(SurfaceCurves, dotNormalized, mach).");
    tdc.Col("dotNormalized", ColType::F32).Col("mach", ColType::F32).Col("value", ColType::F32);
    tdc.key = {"dotNormalized", "mach"};
    std::vector<float> machGrid;
    for (int i = 0; i < 61; ++i) machGrid.push_back(static_cast<float>(i) * 0.05f);
    for (float m : {3.5f, 4.0f, 5.0f, 7.0f, 10.0f, 15.0f, 20.0f, 25.0f, 30.0f}) machGrid.push_back(m);
    for (int i = 0; i <= 40; ++i)
        for (float m : machGrid) {
            const float dn = static_cast<float>(i) / 40.0f;
            tdc.Add({dn, m, aero::DragCurveValue<P>(PhysicsGlobals::SurfaceCurves, dn, m)});
        }

    golden::Table tcubes("cube_inputs", "Drag cube definitions used and the weights applied.");
    golden::Table tw("cube_weighted", "After SetDragWeights/SetPartOcclusion: per-face arrays (order XP,XN,YP,YN,ZP,ZN).");
    golden::Table td("cube_drag", "DragCubeList.SetDrag(-flow_dir, mach) outputs plus FlightIntegrator drag value.");
    CubeTables(tcubes, tw, td);
    std::vector<Case> cases = BuildCases<P>(opt, tcubes, tw);
    const FlightIntegratorAero fi = HostAero();
    const std::vector<Dir> dirs = Directions<P>();
    for (Case& c : cases)
        for (const Dir& d : dirs)
            for (float m : kMachs) AddCubeDragRow<P>(td, c.id, d.id, d.dir, m, *c.part, fi);

    // drag_chain_kerbin (AeroGen.cs:131-162)
    golden::Table tchain("drag_chain_kerbin", "Kerbin flight condition -> FlightIntegrator atmosphere state -> SetDrag -> force scalars.");
    tchain.Col("case_id", ColType::Str).Col("dir_id", ColType::Str);
    for (const char* c : {"altitude", "speed", "mach", "dynamicPressurekPa", "pseudoReDragMult", "dragValue"}) tchain.Col(c, ColType::F64);
    for (const char* c : {"dragScalar", "bodyLiftScalar", "bodyLift_x", "bodyLift_y", "bodyLift_z"}) tchain.Col(c, ColType::F32);
    tchain.key = {"case_id", "dir_id", "altitude", "speed"};
    FlightIntegratorAtmosphere atm;  // AtmosphereGen.HostFlightIntegrator(Kerbin): situation FLYING
    atm.currentMainBody = FindKerbin(env);
    atm.situation = VesselSituations::FLYING;
    FlightIntegratorAero chainFi = HostAero();
    chainFi.currentMainBody = atm.currentMainBody;
    for (Case& c : cases) {
        if (c.id.find('@') != std::string::npos) continue;
        for (const Dir& d : dirs) {
            if (d.id != "+Y" && d.id != "aoa10" && d.id != "aoa45" && d.id != "+Z") continue;
            for (double alt : {0.0, 5000.0, 15000.0, 30000.0, 50000.0, 69000.0})
                for (double spd : {50.0, 250.0, 340.0, 600.0, 1200.0, 2300.0}) {
                    atm.altitude = alt;
                    atm.spd = spd;
                    atm.sunDot = 1.0;
                    atm.density = 0.0;
                    atm.atmosphereTemperatureOffset = 0.0;
                    atm.CalculatePressure();
                    atm.CalculateConstantsAtmosphere();
                    atm.CalculatePressure();
                    atm.CalculateConstantsAtmosphere();
                    const double q = atm.dynamicPressurekPa, mach = atm.mach, pRe = atm.pseudoReDragMult;
                    chainFi.mach = mach;
                    chainFi.pseudoReDragMult = pRe;
                    c.part->DragCubes.SetDrag<P>(-d.dir, static_cast<float>(mach));
                    // FlightIntegrator.cs:2353-2370 (not submerged: num2 = 1; the generator writes the literal 1f for
                    // part.bodyLiftMultiplier, the C++ part has its C# default 1f)
                    const double num6 = chainFi.CalculateDragValue<P>(*c.part) * pRe;
                    const float dragScalar = chainFi.DragScalar<P>(*c.part, q, 1.0);
                    const float bodyLiftScalar = chainFi.BodyLiftScalar<P>(*c.part, q);
                    const Vector3f bl = bodyLiftScalar * c.part->DragCubes.LiftForce();
                    tchain.Add({c.id, d.id, alt, spd, mach, q, pRe, num6, dragScalar, bodyLiftScalar, bl.x, bl.y, bl.z});
                }
        }
    }

    // lifting_surface (AeroGen.cs:165-203)
    golden::Table tl("lifting_surface", "ModuleLiftingSurface.GetLiftVector/GetDragVector for the stock 'Default' curve set.");
    tl.Col("deflectionLiftCoeff", ColType::F32).Col("aoa_deg", ColType::F64).Col("mach", ColType::F32).Col("Q", ColType::F64);
    for (const char* c : {"liftDot", "absDot", "liftScalar", "lift_x", "lift_y", "lift_z", "dragScalar", "drag_x", "drag_y", "drag_z"})
        tl.Col(c, ColType::F32);
    tl.key = {"deflectionLiftCoeff", "aoa_deg", "mach", "Q"};
    const std::shared_ptr<PhysicsGlobals::LiftingSurfaceCurve> def = PhysicsGlobals::GetLiftingSurfaceCurve("Default");
    if (!def) throw std::runtime_error("aero_forces: no Default lifting surface curve");
    aero::LiftingSurfaceParams mls;
    mls.liftCurve = &def->liftCurve;
    mls.liftMachCurve = &def->liftMachCurve;
    mls.dragCurve = &def->dragCurve;
    mls.dragMachCurve = &def->dragMachCurve;
    mls.perpendicularOnly = false;
    mls.nodeBlocked = false;  // nodeEnabled = false
    const Vector3f liftVector(0.0f, 0.0f, 1.0f);
    std::vector<double> aoas;
    for (int i = -36; i <= 36; i++) aoas.push_back(i * 5.0);
    for (double a : {1.0, 2.0, 3.0, 7.5, 12.0, 17.0, 22.0}) aoas.push_back(a);
    for (float coeff : {1.5f, 0.25f, 2.0f})
        for (double aoa : aoas)
            for (float mach : {0.0f, 0.3f, 0.6f, 0.9f, 1.0f, 1.2f, 2.0f, 5.0f, 25.0f}) {
                const double q = 10000.0;
                mls.deflectionLiftCoeff = coeff;
                const double r = aoa * 3.141592653589793 / 180.0;
                const Vector3f pointVelocity =
                    Vector3f(0.0f, static_cast<float>(dotnet::Math::Cos(r)), static_cast<float>(-dotnet::Math::Sin(r))) * 100.0f;
                // SetupCoefficients (ModuleLiftingSurface.cs:205) with liftVector given instead of a Transform axis
                const Vector3f nVel = aero::U<P>::Normalize(pointVelocity);
                const float liftDot = aero::U<P>::Dot(nVel, liftVector);
                const float absDot = Mathf::Abs(liftDot);
                mls.nVel = nVel;
                float liftScalar = 0.0f, dragScalar = 0.0f;
                const Vector3f lift = aero::GetLiftVector<P>(mls, liftVector, liftDot, absDot, q, mach, liftScalar);
                const Vector3f drag = aero::GetDragVector<P>(mls, nVel, absDot, q, mach, dragScalar);
                tl.Add({coeff, aoa, mach, q, liftDot, absDot, liftScalar, lift.x, lift.y, lift.z, dragScalar, drag.x, drag.y, drag.z});
            }

    g.tables.push_back(std::move(tdc));
    g.tables.push_back(std::move(tcubes));
    g.tables.push_back(std::move(tw));
    g.tables.push_back(std::move(td));
    g.tables.push_back(std::move(tchain));
    g.tables.push_back(std::move(tl));
    return WriteCandidate(opt, g, "aero_forces/aero_forces.json");
}

// ---- aero_forces_mono: the in-game Mono capture (MonoFloat) -------------------------------------------------------------
uint32_t Hex32(const std::string& s) { return static_cast<uint32_t>(std::stoul(s, nullptr, 16)); }
float HexFloat(const std::string& s) {
    const uint32_t u = Hex32(s);
    float f;
    std::memcpy(&f, &u, sizeof f);
    return f;
}

int RunAeroForcesMono(const RunnerOptions& opt) {
    using P = aero::MonoFloat;
    CelestialEnv::Get(opt);
    golden::File g;
    g.dataset = "aero_forces";
    g.description = "kspcore candidate for the AeroGen drag-cube tables under Mono's float rule (aero::MonoFloat).";
    golden::Table tcubes("cube_inputs", "Drag cube definitions used and the weights applied.");
    golden::Table tw("cube_weighted", "After SetDragWeights/SetPartOcclusion: per-face arrays (order XP,XN,YP,YN,ZP,ZN).");
    golden::Table td("cube_drag", "DragCubeList.SetDrag(-flow_dir, mach) outputs plus FlightIntegrator drag value.");
    CubeTables(tcubes, tw, td);
    std::vector<Case> cases = BuildCases<P>(opt, tcubes, tw);
    std::map<std::string, Part*> byId;
    for (Case& c : cases) byId[c.id] = c.part.get();
    const FlightIntegratorAero fi = HostAero();
    // R <case_id> <dir_id> <flow_x> <flow_y> <flow_z> <mach> (float32 hex), the capture's input rows
    std::ifstream in(dotnet::PathFromUtf8(opt.harness + "/captures/in/aero_in.tsv"), std::ios::binary);
    if (!in) throw std::runtime_error("aero_forces_mono: harness/captures/in/aero_in.tsv not found");
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] != 'R') continue;
        const std::vector<std::string> f = dotnet::Split(line, "\t", true);
        if (f.size() != 7) throw std::runtime_error("aero_forces_mono: bad row " + line);
        auto it = byId.find(f[1]);
        if (it == byId.end()) throw std::runtime_error("aero_forces_mono: unknown case " + f[1]);
        AddCubeDragRow<P>(td, f[1], f[2], Vector3f(HexFloat(f[3]), HexFloat(f[4]), HexFloat(f[5])), HexFloat(f[6]), *it->second, fi);
    }
    g.tables.push_back(std::move(tcubes));
    g.tables.push_back(std::move(tw));
    g.tables.push_back(std::move(td));
    return WriteCandidate(opt, g, "aero_forces_mono/aero_forces_mono.json");
}

}  // namespace

KSP_GOLDEN_DATASET("aero_forces", 140, RunAeroForces);
KSP_GOLDEN_DATASET("aero_forces_mono", 141, RunAeroForcesMono);
