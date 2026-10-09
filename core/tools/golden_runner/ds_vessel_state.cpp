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
// states     Vessel-level values of the 20 flight snapshots. Inputs read from the capture: spd, srf_velocity and the static
//            pressure / atmosphericTemperature. The capture's `altitude` is the root part's, not FlightIntegrator's CoM altitude, so
//            it is only the fallback when the inversion fails: the altitude is recovered by inverting the captured static pressure,
//            and (GetAtmoThermalStats / the sun direction are not ported) FlightIntegrator's atmosphereTemperatureOffset by inverting
//            the captured temperature; each must reproduce its capture value bit for bit before it is used.
//            Outputs: srfSpeed (Vessel.UpdateSurfaceSpeeds); density, mach, dynamicPressurekPa, pseudoReynolds, pseudoReDragMult
//            (functions of the captured P, T and spd, mirrored into the Vessel by FlightIntegratorAtmosphere); the cache of
//            PhysicsGlobals.DragCubeMultiplier and vessel.parts.Count. staticPressurekPa, staticPressureAtm and
//            atmosphericTemperature are consistency checks of the inversion, not independent gates (the altitude-to-pressure and
//            altitude-to-temperature models are gated by golden.atmosphere_curves). `spd` is emitted but dropped by the gate (a
//            pass-through of the input). Not gated: frame, UT, altitude, aoa_deg, srf_velocity_*, ref_up_* (inputs or engine state).
//
// WP-21 moved the part database loading, the vessel assembly and the atmosphere recovery into vessel_capture.{hpp,cpp} (shared with
// ds_aero_forces_vessel.cpp); the tables and their values are unchanged.
#include <algorithm>
#include <map>

#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/celestial/flight_atmosphere.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "vessel_capture.hpp"

using namespace ksp;
using golden::Cell;
using golden::ColType;

namespace {

// Part.DragModel (Part.cs:29)
const char* const kDragModelNames[] = {"DEFAULT", "CONIC", "CYLINDRICAL", "SPHERICAL", "CUBE", "NONE"};

int RunVesselState(const RunnerOptions& opt) {
    VesselCapture vc(opt);
    const std::vector<std::string>& craftNames = vc.craftNames;
    auto& vessels = vc.vessels;

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
    const nlohmann::json& st = vc.Table("states");
    const size_t cCraft = VesselCapture::Col(st, "craft"), cState = VesselCapture::Col(st, "state"), cVx = VesselCapture::Col(st, "srf_velocity_x"),
                 cVy = VesselCapture::Col(st, "srf_velocity_y"), cVz = VesselCapture::Col(st, "srf_velocity_z");
    for (const auto& row : st.at("rows")) {
        const std::string craft = row[cCraft], state = row[cState];
        Vessel& v = *vessels.at(craft);
        const FlightIntegratorAtmosphere fi = vc.StateAtmosphere(st, row);
        v.ApplyFlightIntegratorAtmosphere(fi);
        v.srf_velocity = Vector3d(row[cVx], row[cVy], row[cVz]);
        v.verticalSpeed = 0.0;
        v.UpdateSurfaceSpeeds();
        tt.Add({craft, state, v.srfSpeed, v.speed, v.mach, v.atmDensity, v.staticPressurekPa, fi.staticPressureAtm, v.dynamicPressurekPa,
                v.atmosphericTemperature, fi.pseudoReynolds, fi.pseudoReDragMult, PhysicsGlobals::DragCubeMultiplier(),
                static_cast<int64_t>(v.parts.size())});
    }
    g.tables.push_back(std::move(tt));

    return WriteCandidate(opt, g, "vessel_state/vessel_state.json");
}

}  // namespace

KSP_GOLDEN_DATASET("vessel_state", 130, RunVesselState);
