// WP-17 deltav_stages: the stock delta-v readout (VesselDeltaV in Ship mode, EDITOR) for the 17 crafts of the C5 capture
// (harness/captures/out2/deltav_readout.json: tables `stages`, `totals`, and from `parts` the DeltaVPartInfo columns
// decoupleStage / activationStage / isEngine).
//
// The run replays what the editor does: each craft file goes through the upgrade pipeline and ShipConstruct.LoadShip + the live
// parts (WP-18); the editor starts every part (modules' OnStart, the staging block of Part.Start), StageManager sorts the icons
// (WP-16); then VesselDeltaV.Create(ship) / Start() / the editor's calculation loop (two runs: syncListInstances) and, per situation
// body (Kerbin, Mun, Duna, Eve at altitude 0), DeltaVAppValues.body / altitude are set, SetCalcsDirty(false) and one more run.
//
// Thrust directions: DeltaVEngineInfo.CalculateThrustVector reads the engine's thrust transforms (OnCenterOfThrustQuery: dir = sum
// of transform.forward * multiplier). They are the real ones: tools/wp17/mu_transforms.py reads the transform hierarchy of the engine
// .mu models (PartReader.cs), places it as PartLoader.CompileModel does and writes core/data/engine_thrust_transforms.json;
// tools/support/engine_thrust_transforms.hpp installs them (world rotation = part rotation * part-space chain rotation).
//
// Inputs (original game data only): the craft files of the capture (see ds_resource_mass.cpp), GameData of --gamedata, the bodies of
// WP-09.
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "celestial_env.hpp"
#include "craft_tree.hpp"
#include "datasets.hpp"
#include "editor_staging.hpp"
#include "engine_thrust_transforms.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/parts/compound_part.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/resources/resource_broker.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/delta_v.hpp"

namespace {

using namespace ksp;
using golden::Cell;
using golden::ColType;
namespace fs = std::filesystem;

struct CraftEntry {
    const char* craftId;
    const char* relPath;  // under harness/inputs/Ships, or "!analysis/..." under the repository root
};

// deltav_readout.json `craft` table (craft_id, path)
const CraftEntry kCrafts[] = {
    {"PhysProf-S", "VAB/PhysProf-S.craft"},
    {"PhysProf-M2", "!analysis/physics/craft/PhysProf-M2.craft"},
    {"PhysProf-L2-AS", "!analysis/physics/craft/PhysProf-L2-AS.craft"},
    {"craft_Kerbal_X", "VAB/Kerbal X.craft"},
    {"craft_Jumping_Flea", "VAB/Jumping Flea.craft"},
    {"craft_Kerbal_1", "VAB/Kerbal 1.craft"},
    {"craft_Kerbal_2", "VAB/Kerbal 2.craft"},
    {"craft_Learstar_A1", "VAB/Learstar A1.craft"},
    {"craft_Ion-Powered_Space_Probe", "VAB/Ion-Powered Space Probe.craft"},
    {"craft_Dynawing", "VAB/Dynawing.craft"},
    {"craft_ComSat_Lx", "VAB/ComSat Lx.craft"},
    {"craft_Aeris_3A", "SPH/Aeris 3A.craft"},
    {"craft_Aeris_4A", "SPH/Aeris 4A.craft"},
    {"craft_Gull", "SPH/Gull.craft"},
    {"craft_Stearwing_A300", "SPH/Stearwing A300.craft"},
    {"craft_Bug-E_Buggy", "SPH/Bug-E Buggy.craft"},
    {"craft_Mallard", "SPH/Mallard.craft"},
};
const char* const kBodies[] = {"Kerbin", "Mun", "Duna", "Eve"};

std::string GameRootOf(const std::string& gamedata) {
    const fs::path gd = fs::absolute(dotnet::PathFromUtf8(gamedata)).lexically_normal();
    fs::path p = gd;
    if (!p.has_filename()) p = p.parent_path();
    const std::u8string s = p.parent_path().u8string();
    return std::string(s.begin(), s.end());
}

const CelestialBody* FindBody(CelestialEnv& env, const std::string& name) {
    for (CelestialBody* b : env.system->SpawnOrder())
        if (b->bodyName == name) return b;
    throw std::runtime_error("deltav_stages: no body " + name);
}

// ProtoCrewMember.InventoryMass() (ProtoCrewMember.cs:775) of an editor kerbal = kerbalEVA's ModuleInventoryPart.GetModuleMass(0, CURRENT)
// (ModuleInventoryPart.cs:1928-1959) over the stored parts of its DEFAULTPARTS (kerbalEVA.cfg: evaChute, evaJetpack), one per slot, quantity 1
// (same computation as tests/test_ship_construct.cpp KerbalInventoryMass; every input is the part database):
//   num += snapshot.mass * (float)Math.Max(1, quantity); then the resources last to first: density * (float)amount, times the quantity
float KerbalInventoryMass(GameData& data, const fs::path& gameDataDir) {
    ConfigNode::NodePtr eva = ConfigNode::Load((gameDataDir / "Squad" / "Parts" / "Prebuilt" / "kerbalEVA.cfg").string());
    if (!eva) throw std::runtime_error("deltav_stages: kerbalEVA.cfg");
    ConfigNode* part = eva->GetNode("PART");
    ConfigNode* inv = nullptr;
    if (part != nullptr)
        for (ConfigNode* m : part->GetNodes("MODULE"))
            if (m->GetValuePtr("name") && *m->GetValuePtr("name") == "ModuleInventoryPart") inv = m;
    ConfigNode* defaults = inv != nullptr ? inv->GetNode("DEFAULTPARTS") : nullptr;
    if (defaults == nullptr) throw std::runtime_error("deltav_stages: kerbalEVA DEFAULTPARTS");
    float num = 0.0f;
    for (int i = 0; i < defaults->values().Count(); i++) {
        const AvailablePart* ap = data.parts.getPartInfoByName(defaults->values()[i].value);
        if (ap == nullptr) throw std::runtime_error("deltav_stages: default part " + defaults->values()[i].value);
        const float quantity = 1.0f;
        num = f32store(fx(num) + fx(ap->partPrefab->fields.mass) * fx(quantity));
        float num2 = 0.0f;
        PartResourceList& res = ap->partPrefab->Resources;
        for (int r = res.Count(); r-- > 0;) {
            const PartResource& pr = res[static_cast<size_t>(r)];
            if (pr.info != nullptr) num2 = f32store(fx(num2) + fx(pr.info->density()) * fx(f32store(pr.amount)));
        }
        num = f32store(fx(num) + fx(num2) * fx(quantity));
    }
    return num;
}

// The crew the editor had assigned when the capture ran. KerbalRoster.DefaultCrewForVessel (KerbalRoster.cs:491-615) fills ONE part:
// the first part of the craft (file order = VesselCrewManifest.PartManifests order) that is a control source with crew capacity (or a
// KerbalSeat, which gets nobody); the loop ends with that part (`break`, KerbalRoster.cs:610). Part.FixedUpdate in the editor
// (Part.cs:4094-4143) then does `UpdateMass(); mass += KerbalCrewMass` per crew member, and VesselDeltaV reads that Part.mass
// (DeltaVPartInfo.cs:198). Kerbal resource / inventory mass is 0 for the default roster.
void AssignDefaultCrew(const std::vector<Part*>& parts, float inventoryPerKerbal) {
    for (Part* p : parts) p->UpdateMass();
    for (Part* p : parts) {
        // Part.isControlSource > NONE: ModuleCommand.OnAwake sets FULL on the prefab and the instance (ModuleCommand.cs:378, WP-24), so a part
        // with a ModuleCommand is a control source (the pods and cockpits of the capture crafts all are)
        const bool isControlSource = p->fields.isControlSource > 0;
        const bool flag2 = !isControlSource && p->fields.CrewCapacity > 0 && p->FindModuleOfClass("KerbalSeat") != nullptr;
        if (!((isControlSource && p->fields.CrewCapacity > 0) || flag2)) continue;
        if (!flag2)
            for (int k = 0; k < p->fields.CrewCapacity; k++) {
                p->fields.mass = f32store(fx(p->fields.mass) + fx(PhysicsGlobals::KerbalCrewMass()));  // mass += num (kerbalMass)
                p->fields.mass = f32store(fx(p->fields.mass) + fx(0.0f));                              // + ProtoCrewMember.ResourceMass(): the EVA prefab has no resources
                p->fields.mass = f32store(fx(p->fields.mass) + fx(inventoryPerKerbal));                // + ProtoCrewMember.InventoryMass()
            }
        break;
    }
}

// Part.Start of a CompoundPart (CompoundPart.cs:169 onStartComplete, WP-26): the editor ship's fuel lines find their target by the
// PARTDATA `tgt` craftID (EditorLogic.fetch.ship.parts) and SetTarget -> every CompoundPartModule.OnTargetSet.
void StartCompoundParts(ShipConstruct& ship) {
    CompoundPart::hooks.editorShipParts = [&ship]() { return ship.liveParts; };
    for (Part* p : ship.liveParts)
        if (auto* cp = dynamic_cast<CompoundPart*>(p)) cp->OnStartComplete();
    CompoundPart::hooks.editorShipParts = nullptr;
}

// (ModuleDockingNode.OnLoad sets `part.fuelCrossFeed = crossfeed` (ModuleDockingNode.cs:533): the real module of WP-25 does it while the craft loads;
// a closed docking port in a craft file has `crossfeed = False` and blocks the crossfeed through the part.)

int RunDeltaVStages(const RunnerOptions& opt) {
    SaveEnvironment::Reset();
    CelestialEnv& env = CelestialEnv::Get(opt);
    GameData data;
    GameData::Options go;
    go.gameRoot = GameRootOf(opt.gamedata);
    go.gameDataDir = fs::absolute(dotnet::PathFromUtf8(opt.gamedata)).lexically_normal().string();
    data.Load(go);
    SaveEnvironment& senv = SaveEnvironment::Current();
    senv.getPartInfoByName = [&data](std::string_view n) { return data.parts.getPartInfoByName(n); };
    senv.partExists = [&data](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
    support::InstallUpgradeHooks(data);
    ResourceSetup::database = &data.database;
    ResourceSetup::ResetConfig();

    golden::File g;
    g.dataset = "deltav_stages";
    g.description = "kspcore candidate for the VesselDeltaV readout of the C5 delta-v capture (stages, totals, DeltaVPartInfo staging).";
    golden::Table tt("totals", "VesselDeltaV totals per situation body.");
    tt.Col("craft_id", ColType::Str).Col("body", ColType::Str).Col("altitude", ColType::F64).Col("atmPressure_atm", ColType::F32)
        .Col("atmDensity", ColType::F64).Col("GeeASL", ColType::F64).Col("TotalDeltaVVac", ColType::F64).Col("TotalDeltaVASL", ColType::F64)
        .Col("TotalDeltaVActual", ColType::F64).Col("TotalBurnTime", ColType::F64).Col("IsReady", ColType::Bool).Col("stages", ColType::I64);
    tt.key = {"craft_id", "body"};
    golden::Table ts("stages", "DeltaVStageInfo for each entry of VesselDeltaV.OperatingStageInfo.");
    ts.Col("craft_id", ColType::Str).Col("body", ColType::Str).Col("list_idx", ColType::I64).Col("stage", ColType::I64).Col("separationIndex", ColType::I64)
        .Col("payloadStage", ColType::Bool).Col("deltaVinVac", ColType::F32).Col("deltaVatASL", ColType::F32).Col("deltaVActual", ColType::F32)
        .Col("TWRVac", ColType::F32).Col("TWRASL", ColType::F32).Col("TWRActual", ColType::F32).Col("ispVac", ColType::F64).Col("ispASL", ColType::F64)
        .Col("ispActual", ColType::F64).Col("thrustVac", ColType::F32).Col("thrustASL", ColType::F32).Col("thrustActual", ColType::F32)
        .Col("startMass", ColType::F32).Col("endMass", ColType::F32).Col("dryMass", ColType::F32).Col("fuelMass", ColType::F32)
        .Col("stageMass", ColType::F32).Col("decoupledMass", ColType::F32).Col("stageBurnTime", ColType::F64).Col("engines_in_stage", ColType::I64)
        .Col("engines_active", ColType::I64).Col("parts", ColType::I64);
    ts.key = {"craft_id", "body", "list_idx"};
    golden::Table tp("parts", "DeltaVPartInfo staging values per part (Kerbin situation); launch clamps have no DeltaVPartInfo.");
    tp.Col("craft_id", ColType::Str).Col("part_index", ColType::I64).Col("part", ColType::Str).Col("decoupleStage", ColType::I64)
        .Col("activationStage", ColType::I64).Col("isEngine", ColType::Bool);
    tp.key = {"craft_id", "part_index"};

    const fs::path shipsDir = fs::path(dotnet::PathFromUtf8(opt.harness)) / "inputs" / "Ships";
    const fs::path repoRoot = fs::path(dotnet::PathFromUtf8(opt.harness)).parent_path();
    const CelestialBody* kerbin = FindBody(env, "Kerbin");
    // the real thrust transforms of the staged engine models (WP-17b); an engine that is not staged throws
    const wp17b::EngineThrustTransforms thrust = wp17b::EngineThrustTransforms::Load(repoRoot / "core" / "data" / "engine_thrust_transforms.json");
    const float inventoryPerKerbal = KerbalInventoryMass(data, fs::path(go.gameDataDir));
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;  // HighLogic.LoadedScene
    DeltaVGlobals::homeBody = kerbin;
    size_t stageRows = 0;
    std::vector<std::string> missingEngineParts;
    for (const CraftEntry& c : kCrafts) {
        const std::string rel = c.relPath;
        const fs::path file = rel[0] == '!' ? repoRoot / rel.substr(1) : shipsDir / rel;
        ConfigNode::NodePtr root = ConfigNode::Load(file.string());
        if (!root) throw std::runtime_error(std::string("cannot read ") + c.craftId);
        ConfigNode::NodePtr upgraded;
        const std::string* sn = root->GetValuePtr("ship");
        KSPUpgradePipeline::Process(root, sn ? *sn : std::string(), SaveUpgradePipeline::LoadContext::Craft,
                                    [&](const ConfigNode::NodePtr& n) { upgraded = n; },
                                    [&](KSPUpgradePipeline::UpgradeFailOption, const ConfigNode::NodePtr&) {});
        if (!upgraded) throw std::runtime_error(std::string("upgrade pipeline failed for ") + c.craftId);
        ShipConstruct ship;
        std::string err;
        if (!ship.LoadShipWithParts(*upgraded, 0, true, err)) throw std::runtime_error(std::string("LoadShip ") + c.craftId + ": " + err);

        support::EditorSession editor(ship.liveParts);
        support::EditorSession::StartParts(ship.liveParts);
        editor.Load();
        const wp17b::EditorHierarchy hierarchy = wp17b::EditorHierarchy::Build(ship.liveParts);
        for (Part* p : ship.liveParts) thrust.Install(*p, &missingEngineParts, &hierarchy);
        StartCompoundParts(ship);
        AssignDefaultCrew(ship.liveParts, inventoryPerKerbal);

        DeltaVGlobals::appValues = std::make_unique<DeltaVAppValues>(kerbin);
        std::unique_ptr<VesselDeltaV> dv = VesselDeltaV::Create(&ship);
        dv->Start();
        for (size_t bi = 0; bi < 4; bi++) {
            const char* bodyName = kBodies[bi];
            const CelestialBody* body = FindBody(env, bodyName);
            DeltaVGlobals::appValues->body = body;
            DeltaVGlobals::appValues->altitude = 0.0;
            dv->SetCalcsDirty(false);
            dv->DriveToCompletion();
            const std::vector<DeltaVStageInfoPtr> stages = dv->OperatingStageInfo();
            tt.Add({c.craftId, bodyName, DeltaVGlobals::appValues->altitude, DeltaVGlobals::appValues->atmPressure(), DeltaVGlobals::appValues->atmDensity(),
                    body->GeeASL, dv->TotalDeltaVVac(), dv->TotalDeltaVASL(), dv->TotalDeltaVActual(), dv->TotalBurnTime(), dv->IsReady(),
                    static_cast<int64_t>(stages.size())});
            for (size_t li = 0; li < stages.size(); li++) {
                const DeltaVStageInfo& s = *stages[li];
                ts.Add({c.craftId, bodyName, static_cast<int64_t>(li), static_cast<int64_t>(s.stage), static_cast<int64_t>(s.separationIndex), s.payloadStage,
                        s.deltaVinVac, s.deltaVatASL, s.deltaVActual, s.TWRVac, s.TWRASL, s.TWRActual, s.ispVac, s.ispASL, s.ispActual, s.thrustVac,
                        s.thrustASL, s.thrustActual, s.startMass, s.endMass, s.dryMass, s.fuelMass, s.stageMass, s.decoupledMass, s.stageBurnTime,
                        static_cast<int64_t>(s.enginesInStage.size()), static_cast<int64_t>(s.enginesActiveInStage.size()),
                        static_cast<int64_t>(s.parts.size())});
                stageRows++;
            }
            if (bi == 0) {
                for (size_t pi = 0; pi < ship.liveParts.size(); pi++) {
                    Part* p = ship.liveParts[pi];
                    DeltaVPartInfo* info = deltav_ext::Get(dv->PartInfo(), p);
                    if (info == nullptr) tp.Add({c.craftId, static_cast<int64_t>(pi), p->gameObjectName, Cell(nullptr), Cell(nullptr), Cell(nullptr)});
                    else tp.Add({c.craftId, static_cast<int64_t>(pi), p->gameObjectName, static_cast<int64_t>(info->decoupleStage),
                                 static_cast<int64_t>(info->activationStage), info->isEngine});
                }
            }
        }
        dv.reset();
        DeltaVGlobals::appValues.reset();
    }
    if (!missingEngineParts.empty()) {
        std::sort(missingEngineParts.begin(), missingEngineParts.end());
        missingEngineParts.erase(std::unique(missingEngineParts.begin(), missingEngineParts.end()), missingEngineParts.end());
        std::string list;
        for (const std::string& n : missingEngineParts) list += " " + n;
        throw std::runtime_error("deltav_stages: engine parts without staged .mu thrust transforms:" + list);
    }
    g.tables.push_back(std::move(tt));
    g.tables.push_back(std::move(ts));
    g.tables.push_back(std::move(tp));
    std::cout << "deltav_stages: " << stageRows << " stage rows\n";
    ResourceSetup::database = nullptr;
    ResourceSetup::ResetConfig();
    SaveEnvironment::Reset();
    return WriteCandidate(opt, g, "deltav_stages/deltav_stages.json");
}

}  // namespace

KSP_GOLDEN_DATASET("deltav_stages", 152, RunDeltaVStages);
