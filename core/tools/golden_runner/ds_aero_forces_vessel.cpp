// WP-21 aero_forces_vessel: the drag-cube pipeline of the C10 assembled-vessel capture (harness/captures/out2/aero_forces_vessel.json)
// recomputed with kspcore (aero::MonoFloat, the game's float policy) on the three PhysProf crafts assembled into Vessels
// (vessel_capture.hpp, shared with WP-14's vessel_state).
//
// Flight-start drag state (what KSP has done before the first captured frame):
//   cube weights   INPUT: structure.cube_names (the weights ModuleJettison.SetFairingDragCube (WP-26, needs the model's jettison
//                  transform) and ModulePartVariants.SetCubes (WP-13) left at flight start), applied through
//                  DragCubeList.SetCubeWeight in list order.
//   weights + occlusion  OUTPUT: FlightIntegrator.DragCubeSetupAndPartAeroStats (FlightIntegrator.cs:2512): SetDragWeights of every
//                  part, then SetPartOcclusion of every part, over the assembled part tree (attach-node links and orientations from
//                  the part cfgs and the craft, part rotations from the craft: SetPartOcclusion only reads relative rotations).
// structure  OUTPUT node_contact_areas: AttachNode.contactArea written by SetPartOcclusion, formatted as the capture does
//            ("<node id>-><attached part index>:<contactArea G4>", attach nodes in order, then the surface node).
// parts      per part and state. INPUTS from the capture row: dragVectorDirLocal_xyz (rigidbody velocity in part space),
//            dynamicPressurekPa (part.atmDensity * 0.0005 * |rigidbody velocity|^2). OUTPUTS: machNumber / atmDensity (the
//            FlightIntegrator mirror of the state, WP-14's recovery), AreaOccluded / WeightedArea / WeightedDrag / WeightedDepth per
//            face, OcclusionMultiplier, PostOcclusionArea, the SetDrag outputs (AreaDrag ... LiftForce, DragVector),
//            CalculateDragValue, dragScalar and bodyLiftScalar (UpdateAerodynamics' statements with the part's q), aerodynamicArea
//            (CalculateAerodynamicArea), radiativeArea (CalculateAreaRadiative), ShieldedFromAirstream, dragcubes_none.
//            Written but not gated (tests/CMakeLists.txt DROP_COLUMNS): the inputs, dragVectorMag (input), staticPressureAtm (needs the
//            part's world altitude: getAltitudeAtPos(partTransform.position), WP-19/20), exposedArea (UpdateAerodynamics reads it
//            BEFORE this frame's SetDrag: the previous physics frame's ExposedArea).
// aoa_sweep  SetDrag(flow_local, (float)fi.mach) on the same occluded cubes; INPUT flow_local_xyz (R(aoa) * dragVectorDirLocal, a
//            native rotation in game), OUTPUTS mach and every SetDrag / CalculateDragValue column.
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/aero/flight_integrator_aero.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "vessel_capture.hpp"

using namespace ksp;
using golden::Cell;
using golden::ColType;

namespace {

using P = aero::MonoFloat;

// exact float32 / float64 inputs from the capture's `<col>_hex` companions
float HexF(const nlohmann::json& row, size_t col) {
    const uint32_t u = static_cast<uint32_t>(std::stoul(row[col].get<std::string>(), nullptr, 16));
    float f;
    std::memcpy(&f, &u, sizeof f);
    return f;
}
double HexD(const nlohmann::json& row, size_t col) {
    const uint64_t u = std::stoull(row[col].get<std::string>(), nullptr, 16);
    double d;
    std::memcpy(&d, &u, sizeof d);
    return d;
}

int IndexOf(const Vessel& v, const Part* p) {
    for (size_t i = 0; i < v.parts.size(); i++)
        if (v.parts[i] == p) return static_cast<int>(i);
    return -1;
}

std::string ContactAreas(const Vessel& v, const Part& p) {
    std::string s;
    auto add = [&](const AttachNode& n) {
        if (n.attachedPart == nullptr) return;
        if (!s.empty()) s += ",";
        s += n.id + "->" + std::to_string(IndexOf(v, n.attachedPart)) + ":" +
             dotnet::FormatSingleG(n.contactArea, 4, dotnet::Runtime::Mono);
    };
    for (const AttachNode& n : p.attachNodes) add(n);
    if (p.srfAttachNode) add(*p.srfAttachNode);
    return s;
}

int RunAeroForcesVessel(const RunnerOptions& opt) {
    VesselCapture vc(opt);
    golden::File g;
    g.dataset = "aero_forces_vessel";
    g.description = "kspcore candidate for the C10 drag-cube pipeline (DragCubeList, FlightIntegrator aero members, Mono float rule).";

    // ---- flight-start drag state ---------------------------------------------------------------------------------------
    const nlohmann::json& st = vc.Table("structure");
    const size_t sCraft = VesselCapture::Col(st, "craft"), sIdx = VesselCapture::Col(st, "part_index"),
                 sCubes = VesselCapture::Col(st, "cube_names");
    for (const auto& row : st.at("rows")) {
        Vessel& v = *vc.vessels.at(row[sCraft].get<std::string>());
        Part& p = *v.parts.at(row[sIdx].get<size_t>());
        if (row[sCubes].is_null()) continue;
        for (const std::string& item : dotnet::Split(row[sCubes].get<std::string>(), ",", true)) {
            const size_t eq = item.rfind('=');
            if (eq == std::string::npos) throw std::runtime_error("aero_forces_vessel: bad cube weight " + item);
            p.DragCubes.SetCubeWeight(item.substr(0, eq), dotnet::ParseSingle(item.substr(eq + 1)));
        }
    }
    std::map<std::string, FlightIntegratorAero> fis;
    for (auto& [name, v] : vc.vessels) {
        FlightIntegratorAero& fi = fis[name];
        fi.CacheGlobals();
        fi.currentMainBody = vc.kerbin;
        // the first FixedUpdate: DragCubeSetupAndPartAeroStats; the part altitude hook stands in the vessel's (staticPressureAtm
        // is not gated)
        fi.DragCubeSetupAndPartAeroStats<P>(v->parts, 0.0, [](const Part&) { return 0.0; });
    }

    // ---- structure: node_contact_areas -------------------------------------------------------------------------------
    golden::Table ts("structure", "AttachNode.contactArea per part after DragCubeList.SetPartOcclusion.");
    ts.Col("craft", ColType::Str).Col("part_index", ColType::I64).Col("part", ColType::Str).Col("node_contact_areas", ColType::Str);
    ts.key = {"craft", "part_index"};
    for (const std::string& name : vc.craftNames) {
        const Vessel& v = *vc.vessels.at(name);
        for (size_t i = 0; i < v.parts.size(); i++)
            ts.Add({name, static_cast<int64_t>(i), v.parts[i]->partInfo->name, ContactAreas(v, *v.parts[i])});
    }
    g.tables.push_back(std::move(ts));

    // ---- per-state FlightIntegrator atmosphere ------------------------------------------------------------------------
    const nlohmann::json& states = vc.Table("states");
    std::map<std::pair<std::string, std::string>, FlightIntegratorAtmosphere> atm;
    for (const auto& row : states.at("rows"))
        atm[{row[VesselCapture::Col(states, "craft")].get<std::string>(), row[VesselCapture::Col(states, "state")].get<std::string>()}] =
            vc.StateAtmosphere(states, row);

    // ---- parts ---------------------------------------------------------------------------------------------------------
    const nlohmann::json& pt = vc.Table("parts");
    auto pc = [&](const char* c) { return VesselCapture::Col(pt, c); };
    const size_t cCraft = pc("craft"), cState = pc("state"), cIdx = pc("part_index"), cDx = pc("dragVectorDirLocal_x_hex"),
                 cDy = pc("dragVectorDirLocal_y_hex"), cDz = pc("dragVectorDirLocal_z_hex"), cMag = pc("dragVectorMag_hex"),
                 cQ = pc("dynamicPressurekPa_hex"), cPatm = pc("staticPressureAtm_hex"), cExp = pc("exposedArea_hex");
    golden::Table tp("parts", "Per part and snapshot (faces XP,XN,YP,YN,ZP,ZN).");
    tp.Col("craft", ColType::Str).Col("state", ColType::Str).Col("part_index", ColType::I64).Col("part", ColType::Str);
    tp.Col("ShieldedFromAirstream", ColType::Bool).Col("dragcubes_none", ColType::Bool);
    for (const char* c : {"dragVectorDirLocal_x", "dragVectorDirLocal_y", "dragVectorDirLocal_z", "dragVectorMag"}) tp.Col(c, ColType::F32);
    for (const char* c : {"machNumber", "dynamicPressurekPa", "staticPressureAtm", "atmDensity"}) tp.Col(c, ColType::F64);
    for (const char* arr : {"AreaOccluded_", "WeightedArea_", "WeightedDrag_", "WeightedDepth_"})
        for (int f = 0; f < 6; f++) tp.Col(arr + std::to_string(f), ColType::F32);
    for (const char* c : {"OcclusionMultiplier", "PostOcclusionArea", "AreaDrag", "Area", "ExposedArea", "CrossSectionalArea", "Depth",
                          "DragCoeff", "TaperDot", "LiftForce_x", "LiftForce_y", "LiftForce_z", "DragVector_x", "DragVector_y",
                          "DragVector_z", "dragScalar", "bodyLiftScalar"})
        tp.Col(c, ColType::F32);
    for (const char* c : {"aerodynamicArea", "exposedArea", "radiativeArea", "CalculateDragValue"}) tp.Col(c, ColType::F64);
    tp.key = {"craft", "state", "part_index"};
    for (const auto& row : pt.at("rows")) {
        const std::string craft = row[cCraft], state = row[cState];
        Vessel& v = *vc.vessels.at(craft);
        Part& part = *v.parts.at(row[cIdx].get<size_t>());
        const FlightIntegratorAtmosphere& a = atm.at({craft, state});
        FlightIntegratorAero& fi = fis.at(craft);
        fi.mach = a.mach;
        fi.density = a.density;
        fi.pseudoReDragMult = a.pseudoReDragMult;
        // DragCubeSetupAndPartAeroStats of this frame (weights / occlusion are up to date: both return at once)
        fi.DragCubeSetupAndPartAeroStats<P>(v.parts, 0.0, [](const Part&) { return 0.0; });
        PartFields& f = part.fields;
        // UpdateAerodynamics (FlightIntegrator.cs:2260) with the captured rigidbody-derived inputs
        f.dragVectorDirLocal = Vector3f(HexF(row, cDx), HexF(row, cDy), HexF(row, cDz));
        f.dynamicPressurekPa = HexD(row, cQ);
        f.aerodynamicArea = 0.0;
        if (!part.ShieldedFromAirstream() && (f.atmDensity > 0.0 || f.submergedPortion > 0.0)) {
            if (!part.DragCubes.None()) part.DragCubes.SetDrag<P>(f.dragVectorDirLocal, static_cast<float>(fi.mach));
            f.aerodynamicArea = fi.CalculateAerodynamicArea(part);
            f.dragScalar = fi.DragScalar<P>(part, f.dynamicPressurekPa, 1.0);
            if (!f.hasLiftModule &&
                (!f.bodyLiftOnlyUnattachedLiftActual || part.bodyLiftOnlyProvider == nullptr || !part.bodyLiftOnlyProvider->IsLifting()))
                f.bodyLiftScalar = fi.BodyLiftScalar<P>(part, f.dynamicPressurekPa);
        }
        const DragCubeList& d = part.DragCubes;
        std::vector<Cell> out = {craft, state, row[cIdx].get<int64_t>(), part.partInfo->name, part.ShieldedFromAirstream(), d.None(),
                                 f.dragVectorDirLocal.x, f.dragVectorDirLocal.y, f.dragVectorDirLocal.z, HexF(row, cMag),
                                 f.machNumber, f.dynamicPressurekPa, HexD(row, cPatm), f.atmDensity};
        for (const std::array<float, 6>* arr : {&d.AreaOccluded(), &d.WeightedArea(), &d.WeightedDrag(), &d.WeightedDepth()})
            for (float x : *arr) out.emplace_back(x);
        for (float x : {d.OcclusionMultiplier(), d.PostOcclusionArea(), d.AreaDrag(), d.Area(), d.ExposedArea(), d.CrossSectionalArea(),
                        d.Depth(), d.DragCoeff(), d.TaperDot(), d.LiftForce().x, d.LiftForce().y, d.LiftForce().z, d.DragVector().x,
                        d.DragVector().y, d.DragVector().z, f.dragScalar, f.bodyLiftScalar})
            out.emplace_back(x);
        for (double x : {f.aerodynamicArea, HexD(row, cExp), fi.CalculateAreaRadiative(part), fi.CalculateDragValue<P>(part)})
            out.emplace_back(x);
        tp.Add(std::move(out));
    }
    g.tables.push_back(std::move(tp));

    // ---- aoa_sweep -----------------------------------------------------------------------------------------------------
    const nlohmann::json& sw = vc.Table("aoa_sweep");
    auto wc = [&](const char* c) { return VesselCapture::Col(sw, c); };
    const size_t wCraft = wc("craft"), wState = wc("state"), wIdx = wc("part_index"), wAoa = wc("aoa_deg_hex"), wFx = wc("flow_local_x_hex"),
                 wFy = wc("flow_local_y_hex"), wFz = wc("flow_local_z_hex");
    golden::Table ta("aoa_sweep", "Synthetic angle-of-attack sweep on the live cubes.");
    ta.Col("craft", ColType::Str).Col("state", ColType::Str).Col("part_index", ColType::I64);
    for (const char* c : {"aoa_deg", "flow_local_x", "flow_local_y", "flow_local_z", "mach", "AreaDrag", "Area", "ExposedArea",
                          "CrossSectionalArea", "Depth", "DragCoeff", "TaperDot", "LiftForce_x", "LiftForce_y", "LiftForce_z"})
        ta.Col(c, ColType::F32);
    ta.Col("CalculateDragValue", ColType::F64);
    ta.key = {"craft", "state", "part_index", "aoa_deg"};
    for (const auto& row : sw.at("rows")) {
        const std::string craft = row[wCraft], state = row[wState];
        Vessel& v = *vc.vessels.at(craft);
        Part& part = *v.parts.at(row[wIdx].get<size_t>());
        const float mach = static_cast<float>(atm.at({craft, state}).mach);
        const Vector3f flow(HexF(row, wFx), HexF(row, wFy), HexF(row, wFz));
        part.DragCubes.SetDrag<P>(flow, mach);
        const DragCubeList& d = part.DragCubes;
        ta.Add({craft, state, row[wIdx].get<int64_t>(), HexF(row, wAoa), flow.x, flow.y, flow.z, mach, d.AreaDrag(), d.Area(),
                d.ExposedArea(), d.CrossSectionalArea(), d.Depth(), d.DragCoeff(), d.TaperDot(), d.LiftForce().x, d.LiftForce().y,
                d.LiftForce().z, fis.at(craft).CalculateDragValue<P>(part)});
    }
    g.tables.push_back(std::move(ta));
    return WriteCandidate(opt, g, "aero_forces_vessel/aero_forces_vessel.json");
}

}  // namespace

KSP_GOLDEN_DATASET("aero_forces_vessel", 142, RunAeroForcesVessel);
