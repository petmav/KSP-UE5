// WP-14 vessel_state: the assembled-vessel capture C10 (harness/captures/out2/aero_forces_vessel.json, written by
// decompiled/KSP-Decompiled-profiling/.../HarnessCapture2Flight.cs C10) gated with kspcore's Vessel.
//
// structure  The three crafts (PhysProf-S, -M2, -L2-AS: analysis/physics/craft/*.craft, the game's own craft files that C10 launched)
//            are loaded (ShipConstruct.LoadShip) and assembled into a Vessel (BuildCraftParts + Vessel.Initialize, the part
//            half of ShipConstruction.AssembleForLaunch). Gated per part: vessel.parts order (findVesselParts DFS), part name,
//            parent_index, attach_node (the node of the part that holds its parent; "srfAttach" for surface attachment),
//            attachMode, dragModel. NOT gated here (documented in STATUS.md): cube_names (cube weights are set by module OnStart
//            logic and DragCubeList.ForceUpdate: WP-21 / the module packages) and node_contact_areas (DragCubeList.SetPartOcclusion:
//            WP-21); tests/test_vessel.cpp checks the cube NAMES of the cube_names column.
// states     Vessel-level values of the 20 flight snapshots. Inputs read from the capture: altitude, spd, srf_velocity and the
//            atmosphericTemperature (GetAtmoThermalStats / the sun direction are not ported, so FlightIntegrator's
//            atmosphereTemperatureOffset is recovered from the captured temperature and must reproduce it bit for bit
//            before it is used; the temperature column is therefore a consistency check, not an independent output).
//            Outputs: srfSpeed (Vessel.UpdateSurfaceSpeeds), the vessel's mirrored FlightIntegrator values (speed, density,
//            static / dynamic pressure, mach, pseudoReynolds, pseudoReDragMult) through FlightIntegratorAtmosphere, the cache
//            of PhysicsGlobals.DragCubeMultiplier and vessel.parts.Count. Not gated: frame, UT, altitude, aoa_deg, srf_velocity_*,
//            ref_up_* (inputs or engine state).
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <filesystem>
#include <fstream>

#include "celestial_env.hpp"
#include "datasets.hpp"
#include "game_root.hpp"
#include "golden_json.hpp"
#include "ksp/celestial/flight_atmosphere.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel_assembly.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using golden::Cell;
using golden::ColType;

namespace {

namespace fs = std::filesystem;

std::string GameRootOf(const std::string& gamedata) {
    const fs::path gd = fs::absolute(dotnet::PathFromUtf8(gamedata)).lexically_normal();
    fs::path p = gd;
    if (!p.has_filename()) p = p.parent_path();
    const std::u8string s = p.parent_path().u8string();
    return std::string(s.begin(), s.end());
}

// Part.DragModel (Part.cs:29)
const char* const kDragModelNames[] = {"DEFAULT", "CONIC", "CYLINDRICAL", "SPHERICAL", "CUBE", "NONE"};

double NextUp(double v) { return std::nextafter(v, INFINITY); }
double NextDown(double v) { return std::nextafter(v, -INFINITY); }

// FlightIntegrator.atmosphereTemperatureOffset such that GetFullTemperature(altitude, offset) == T (searched +-512 ulps around
// (T - base) / mult). Returns false if no offset reproduces T.
bool RecoverTemperatureOffset(const CelestialBody& body, double altitude, double T, double& offset) {
    const double mult = static_cast<double>(body.atmosphereTemperatureSunMultCurve.Evaluate(static_cast<float>(altitude)));
    if (body.GetFullTemperature(altitude, 0.0) == T) {
        offset = 0.0;
        return true;
    }
    if (mult == 0.0) return false;
    const double guess = (T - body.GetTemperature(altitude)) / mult;
    double up = guess, down = guess;
    for (int i = 0; i < 512; i++) {
        if (body.GetFullTemperature(altitude, up) == T) {
            offset = up;
            return true;
        }
        if (body.GetFullTemperature(altitude, down) == T) {
            offset = down;
            return true;
        }
        up = NextUp(up);
        down = NextDown(down);
    }
    return false;
}

// FlightIntegrator.altitude (the vessel CoM's altitude, CalculatePressure's input) is not in the capture: the capture's `altitude` column
// is Vessel.altitude of the root part (a 457-part stack differs by ~100 m). The pressure curve is monotone, so the altitude is
// recovered as the lowest one whose GetPressure equals the captured static pressure bit for bit.
bool RecoverPressureAltitude(const CelestialBody& body, double P, double& altitude) {
    double lo = 0.0, hi = body.atmosphereDepth;
    if (body.GetPressure(lo) < P) return false;
    for (int i = 0; i < 200; i++) {
        const double mid = 0.5 * (lo + hi);
        if (body.GetPressure(mid) > P) lo = mid;
        else hi = mid;
    }
    altitude = hi;
    return body.GetPressure(hi) == P;
}

int RunVesselState(const RunnerOptions& opt) {
    CelestialEnv& cenv = CelestialEnv::Get(opt);
    CelestialBody* kerbin = nullptr;
    for (CelestialBody* b : cenv.system->SpawnOrder())
        if (b->bodyName == "Kerbin") kerbin = b;
    if (kerbin == nullptr) throw std::runtime_error("vessel_state: no Kerbin");

    // the game's part database (GameData mirror or --gamedata), the same loading sequence as part_database
    SaveEnvironment::Reset();
    GameData data;
    GameData::Options go;
    go.gameRoot = GameRootOf(opt.gamedata);
    go.gameDataDir = fs::absolute(dotnet::PathFromUtf8(opt.gamedata)).lexically_normal().string();
    data.Load(go);
    SaveEnvironment& senv = SaveEnvironment::Current();
    senv.getPartInfoByName = [&data](std::string_view n) { return data.parts.getPartInfoByName(n); };
    senv.partExists = [&data](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
    senv.newGuidHashCode = [] {
        static uint32_t s = 777u;
        s = s * 1664525u + 1013904223u;
        return static_cast<int32_t>(s);
    };
    senv.localBodies = cenv.system->SpawnOrder();
    FlightRegistry::Clear();
    auto partInfo = [&data](const std::string& n) { return data.parts.getPartInfoByName(n); };

    // The capture (inputs of the states table, names of the crafts)
    nlohmann::json cap;
    {
        std::ifstream f(opt.harness + "/captures/out2/aero_forces_vessel.json", std::ios::binary);
        if (!f) throw std::runtime_error("vessel_state: capture aero_forces_vessel.json not found under " + opt.harness);
        cap = nlohmann::json::parse(f);
    }
    auto findTable = [&](const char* name) -> const nlohmann::json& {
        for (const auto& t : cap.at("tables"))
            if (t.at("name") == name) return t;
        throw std::runtime_error(std::string("vessel_state: no table ") + name);
    };
    auto colIndex = [](const nlohmann::json& t, const char* c) -> size_t {
        size_t i = 0;
        for (const auto& col : t.at("columns")) {
            if (col.at("name") == c) return i;
            i++;
        }
        throw std::runtime_error(std::string("vessel_state: no column ") + c);
    };

    // assemble the three vessels (craft name -> Vessel); launch pad of KSC (Kerbin world position at 0 m)
    std::map<std::string, std::unique_ptr<Vessel>> vessels;
    std::vector<std::string> craftNames = {"PhysProf-S", "PhysProf-M2", "PhysProf-L2-AS"};
    for (const std::string& name : craftNames) {
        ConfigNode::NodePtr root = ConfigNode::Load(opt.harness + "/../analysis/physics/craft/" + name + ".craft");
        if (!root) throw std::runtime_error("vessel_state: cannot read craft " + name);
        ShipConstruct ship;
        std::string err;
        if (!ship.LoadShip(*root, 0, true, err)) throw std::runtime_error("vessel_state: LoadShip " + name + ": " + err);
        AssembleOptions ao;
        ao.rootPosition = kerbin->GetWorldSurfacePosition(-0.0972077486779459, -74.5576813534722, 75.0).ToVector3f();
        std::unique_ptr<Vessel> v = AssembleVesselFromCraft(ship, partInfo, ao);
        if (!v) throw std::runtime_error("vessel_state: AssembleVesselFromCraft " + name);
        vessels[name] = std::move(v);
    }

    golden::File g;
    g.dataset = "vessel_state";
    g.description = "kspcore candidate for the C10 assembled-vessel capture (Vessel part tree, vessel-level atmosphere values).";

    // ---- structure ---------------------------------------------------------------------------------------------------
    golden::Table ts("structure", "Vessel structure at the first state: parent part and the attach node used.");
    ts.Col("craft", ColType::Str).Col("part_index", ColType::I64).Col("part", ColType::Str).Col("parent_index", ColType::I64);
    ts.Col("attach_node", ColType::Str).Col("attachMode", ColType::Str).Col("dragModel", ColType::Str);
    ts.key = {"craft", "part_index"};
    for (const std::string& name : craftNames) {
        Vessel& v = *vessels[name];
        for (size_t i = 0; i < v.parts.size(); i++) {
            Part* p = v.parts[i];
            Cell parentIdx(nullptr), attachNode(nullptr);
            if (p->parent != nullptr) {
                const auto it = std::find(v.parts.begin(), v.parts.end(), p->parent);
                parentIdx = Cell(static_cast<int64_t>(it - v.parts.begin()));
                if (p->fields.attachMode == static_cast<int>(AttachModes::SRF_ATTACH)) {
                    attachNode = Cell(p->srfAttachNode->id);
                } else if (AttachNode* n = p->FindAttachNodeByPart(p->parent)) {
                    attachNode = Cell(n->id);
                }
            }
            ts.Add({name, static_cast<int64_t>(i), p->partInfo->name, parentIdx, attachNode,
                    p->fields.attachMode == static_cast<int>(AttachModes::SRF_ATTACH) ? "SRF_ATTACH" : "STACK",
                    kDragModelNames[p->fields.dragModel]});
        }
    }
    g.tables.push_back(std::move(ts));

    // ---- states ------------------------------------------------------------------------------------------------------
    golden::Table tt("states", "Vessel-level state per snapshot (atmosphere-derived values through Vessel / FlightIntegratorAtmosphere).");
    tt.Col("craft", ColType::Str).Col("state", ColType::Str);
    for (const char* c : {"srfSpeed", "spd", "mach", "density", "staticPressurekPa", "staticPressureAtm", "dynamicPressurekPa",
                          "atmosphericTemperature", "pseudoReynolds", "pseudoReDragMult"})
        tt.Col(c, ColType::F64);
    tt.Col("cacheDragCubeMultiplier", ColType::F32).Col("parts", ColType::I64);
    tt.key = {"craft", "state"};
    const nlohmann::json& st = findTable("states");
    const size_t cCraft = colIndex(st, "craft"), cState = colIndex(st, "state"), cAlt = colIndex(st, "altitude"),
                 cSpd = colIndex(st, "spd"), cT = colIndex(st, "atmosphericTemperature"), cP = colIndex(st, "staticPressurekPa"), cVx = colIndex(st, "srf_velocity_x"),
                 cVy = colIndex(st, "srf_velocity_y"), cVz = colIndex(st, "srf_velocity_z");
    for (const auto& row : st.at("rows")) {
        const std::string craft = row[cCraft], state = row[cState];
        Vessel& v = *vessels.at(craft);
        const double altitude = row[cAlt], spd = row[cSpd], T = row[cT], P = row[cP];
        FlightIntegratorAtmosphere fi;
        fi.currentMainBody = kerbin;
        fi.situation = VesselSituations::FLYING;
        fi.altitude = altitude;
        fi.spd = spd;
        double pAlt = altitude;
        if (!RecoverPressureAltitude(*kerbin, P, pAlt))
            std::cerr << "vessel_state: no altitude reproduces the static pressure of " << craft << "/" << state << "\n";
        fi.altitude = pAlt;
        fi.CalculatePressure();
        if (fi.staticPressurekPa > 0.0) {
            double offset = 0.0;
            if (!RecoverTemperatureOffset(*kerbin, pAlt, T, offset))
                std::cerr << "vessel_state: no temperature offset reproduces " << craft << "/" << state << " (T=" << T << ")\n";
            fi.atmosphereTemperatureOffset = offset;
            fi.CalculateConstantsAtmosphere();
        } else {
            fi.CalculateConstantsVacuum();
        }
        v.ApplyFlightIntegratorAtmosphere(fi);
        v.srf_velocity = Vector3d(row[cVx], row[cVy], row[cVz]);
        v.verticalSpeed = 0.0;
        v.UpdateSurfaceSpeeds();
        tt.Add({craft, state, v.srfSpeed, v.speed, v.mach, v.atmDensity, v.staticPressurekPa, fi.staticPressureAtm, v.dynamicPressurekPa,
                v.atmosphericTemperature, fi.pseudoReynolds, fi.pseudoReDragMult, PhysicsGlobals::DragCubeMultiplier(),
                static_cast<int64_t>(v.parts.size())});
    }
    g.tables.push_back(std::move(tt));

    // vessels reference the part prefabs of `data`: release them before it goes away
    vessels.clear();
    FlightRegistry::Clear();
    SaveEnvironment::Reset();
    return WriteCandidate(opt, g, "vessel_state/vessel_state.json");
}

}  // namespace

KSP_GOLDEN_DATASET("vessel_state", 130, RunVesselState);
