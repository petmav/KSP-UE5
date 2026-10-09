// WP-05 part_database: GameDatabase + PartLoader results vs the in-game capture C9
// (harness/captures/out2/part_database.json, written by decompiled/KSP-Decompiled-profiling/Assembly-CSharp/PhysProf/
// HarnessCapture2.cs:426 C9). Input: the real GameData (--gamedata, default <harness>/../decompiled/KSP-dev/GameData),
// read-only; the game root (Physics.cfg, PartDatabase.cfg) is its parent directory.
//
// Tables are written with the capture's columns and C9's value formatting (Fmt: float/double "R", bool True/False,
// Vector3 "x,y,z", enums by name, FloatCurve "curve[t v in out; ...]"). module_fields lists every KSPField of every
// module instance (WP-06 module host: initialisers, MODULE-node values and the module's OnAwake/OnLoad results).
// Next to the candidate the runner writes
//   part_database/gamedatabase_load.log -- the GameDatabase load order ("Load(Texture): url", "Config(PART) url", ...)
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include "ksp/dotnet/text.hpp"
#include "datasets.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/utf16.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/physics/physics_globals.hpp"

namespace {

using golden::Cell;
using golden::ColType;
using namespace ksp;

std::string R(float f) { return dotnet::FormatSingleR(f); }
std::string R(double d) { return dotnet::FormatDoubleR(d); }
std::string FmtBool(bool b) { return b ? "True" : "False"; }
std::string FmtV3(const Vector3f& v) { return R(v.x) + "," + R(v.y) + "," + R(v.z); }

std::string CurveString(const AnimationCurve& c) {
    std::string s = "curve[";
    const auto& keys = c.keys();
    for (size_t i = 0; i < keys.size(); i++) {
        if (i) s += "; ";
        s += R(keys[i].time) + " " + R(keys[i].value) + " " + R(keys[i].inTangent) + " " + R(keys[i].outTangent);
    }
    return s + "]";
}

// HarnessCapture2.cs:249 Fmt + :343 Num for module field values
void FmtModuleValue(const ModuleValue& v, std::string& s, Cell& num) {
    num = Cell(nullptr);
    std::visit(
        [&](const auto& x) {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, std::monostate>) s = "null";
            else if constexpr (std::is_same_v<T, bool>) s = FmtBool(x);
            else if constexpr (std::is_same_v<T, float>) { s = R(x); num = Cell(static_cast<double>(x)); }
            else if constexpr (std::is_same_v<T, double>) { s = R(x); num = Cell(x); }
            else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t> || std::is_same_v<T, int16_t>) {
                s = dotnet::FormatInt64(x);
                num = Cell(static_cast<double>(x));
            } else if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, uint8_t>) {
                s = dotnet::FormatUInt64(x);
                num = Cell(static_cast<double>(x));
            } else if constexpr (std::is_same_v<T, uint16_t> || std::is_same_v<T, uint64_t>) s = dotnet::FormatUInt64(x);
            else if constexpr (std::is_same_v<T, char16_t>) s = dotnet::FromUtf16(std::u16string(1, x));
            else if constexpr (std::is_same_v<T, std::string>) s = x;
            else if constexpr (std::is_same_v<T, Vector2f>) s = R(x.x) + "," + R(x.y);
            else if constexpr (std::is_same_v<T, Vector3f>) s = FmtV3(x);
            else if constexpr (std::is_same_v<T, Vector4f>) s = R(x.x) + "," + R(x.y) + "," + R(x.z) + "," + R(x.w);
            else if constexpr (std::is_same_v<T, Quaternionf>) s = R(x.x) + "," + R(x.y) + "," + R(x.z) + "," + R(x.w);
            else if constexpr (std::is_same_v<T, EnumValue>) s = x.table->ToString(x.value);
            else if constexpr (std::is_same_v<T, FloatCurve>) s = CurveString(x.Curve());
            else if constexpr (std::is_same_v<T, DoubleCurve>) s = "DoubleCurve";
        },
        v);
}

// Fmt/Num of a Part field (part_cfg_values.resolved)
void FmtPartField(const PartFieldInfo& f, PartPrefab& p, std::string& s, Cell& num) {
    num = Cell(nullptr);
    void* a = f.address ? f.address(p.fields, p.compoundFields) : nullptr;
    switch (f.kind) {
        case FieldKind::Single: s = R(*static_cast<float*>(a)); num = Cell(static_cast<double>(*static_cast<float*>(a))); return;
        case FieldKind::Double: s = R(*static_cast<double*>(a)); num = Cell(*static_cast<double*>(a)); return;
        case FieldKind::Int32: s = dotnet::FormatInt64(*static_cast<int32_t*>(a)); num = Cell(static_cast<double>(*static_cast<int32_t*>(a))); return;
        case FieldKind::UInt32: s = dotnet::FormatUInt64(*static_cast<uint32_t*>(a)); num = Cell(static_cast<double>(*static_cast<uint32_t*>(a))); return;
        case FieldKind::Boolean: s = FmtBool(*static_cast<bool*>(a)); return;
        case FieldKind::String: { const auto& v = *static_cast<NullableString*>(a); s = v ? *v : "null"; return; }
        case FieldKind::Vector3: s = FmtV3(*static_cast<Vector3f*>(a)); return;
        case FieldKind::Vector3d: { const auto& v = *static_cast<Vector3d*>(a); s = R(v.x) + "," + R(v.y) + "," + R(v.z); return; }
        case FieldKind::Quaternion: { const auto& q = *static_cast<Quaternionf*>(a); s = R(q.x) + "," + R(q.y) + "," + R(q.z) + "," + R(q.w); return; }
        case FieldKind::Color: { const auto& c = *static_cast<Color*>(a); s = R(c.r) + "," + R(c.g) + "," + R(c.b) + "," + R(c.a); return; }
        case FieldKind::Enum: s = f.enumTable->ToString(*static_cast<int32_t*>(a)); return;
        case FieldKind::AttachRules: s = static_cast<NullableAttachRules*>(a)->has_value() ? "AttachRules" : "null"; return;
        case FieldKind::StringList: {
            const auto& l = *static_cast<NullableStringList*>(a);
            if (!l) { s = "null"; return; }
            s = "[";
            for (size_t i = 0; i < l->size() && i < 40; i++) s += (i ? "; " : "") + (*l)[i];
            if (l->size() > 40) s += "; ...";
            s += "]";
            return;
        }
        case FieldKind::Object: s = "null"; return;
    }
}

std::string GameRootOf(const std::string& gamedata) {
    namespace fs = std::filesystem;
    const fs::path gd = fs::absolute(dotnet::PathFromUtf8(gamedata)).lexically_normal();
    fs::path p = gd;
    if (!p.has_filename()) p = p.parent_path();
    const std::u8string s = p.parent_path().u8string();
    return std::string(s.begin(), s.end());
}

int RunPartDatabase(const RunnerOptions& opt) {
    namespace fs = std::filesystem;
    GameData data;
    GameData::Options go;
    go.gameRoot = GameRootOf(opt.gamedata);
    go.gameDataDir = fs::absolute(dotnet::PathFromUtf8(opt.gamedata)).lexically_normal().string();
    data.Load(go);
    const GameDatabase& db = data.database;
    const PartLoader& loader = data.parts;
    const std::string outDir = opt.out + "/part_database";
    golden::MakeDirs(outDir);
    {
        std::ofstream log(outDir + "/gamedatabase_load.log", std::ios::binary);
        for (const std::string& l : db.loadLog) log << l << "\n";
    }

    golden::File g;
    g.dataset = "part_database";
    g.description = "kspcore candidate for the C9 part database capture (GameDatabase + PartLoader data side).";

    golden::Table tp("parts", "One row per AvailablePart.");
    tp.key = {"name"};
    for (auto [n, t] : std::vector<std::pair<const char*, ColType>>{
             {"name", ColType::Str}, {"title", ColType::Str}, {"partUrl", ColType::Str}, {"cfg_file", ColType::Str},
             {"category", ColType::Str}, {"TechRequired", ColType::Str}, {"cost", ColType::F32}, {"entryCost", ColType::I64},
             {"mass", ColType::F32}, {"crashTolerance", ColType::F32}, {"maxTemp", ColType::F64}, {"skinMaxTemp", ColType::F64},
             {"breakingForce", ColType::F32}, {"breakingTorque", ColType::F32}, {"minimum_drag", ColType::F32},
             {"maximum_drag", ColType::F32}, {"angularDrag", ColType::F32}, {"dragModel", ColType::Str},
             {"CoMOffset_x", ColType::F32}, {"CoMOffset_y", ColType::F32}, {"CoMOffset_z", ColType::F32},
             {"CoPOffset_x", ColType::F32}, {"CoPOffset_y", ColType::F32}, {"CoPOffset_z", ColType::F32},
             {"CoLOffset_x", ColType::F32}, {"CoLOffset_y", ColType::F32}, {"CoLOffset_z", ColType::F32},
             {"heatConductivity", ColType::F64}, {"emissiveConstant", ColType::F64}, {"absorptiveConstant", ColType::F64},
             {"thermalMassModifier", ColType::F64}, {"skinMassPerArea", ColType::F64},
             {"skinInternalConductionMult", ColType::F64}, {"radiatorHeadroom", ColType::F64},
             {"explosionPotential", ColType::F32}, {"buoyancy", ColType::F32}, {"fuelCrossFeed", ColType::Bool},
             {"physicalSignificance", ColType::Str}, {"attachRules", ColType::Str}, {"bulkheadProfiles", ColType::Str},
             {"partSize", ColType::F32}, {"scaleFactor", ColType::F32}, {"rescaleFactor", ColType::F32},
             {"stackSymmetry", ColType::I64}, {"CrewCapacity", ColType::I64}, {"module_count", ColType::I64},
             {"resource_count", ColType::I64}, {"attach_node_count", ColType::I64}, {"dragcubes_none", ColType::Bool},
             {"dragcubes_procedural", ColType::Bool}, {"dragcube_count", ColType::I64}, {"resource_mass", ColType::F64},
             {"resource_cost", ColType::F64}, {"module_mass", ColType::F32}, {"module_cost", ColType::F32},
             {"internal", ColType::Str}, {"error", ColType::Str}})
        tp.Col(n, t);

    golden::Table tv("part_cfg_values", "Top-level partConfig values and the field they resolved to.");
    tv.key = {"part", "idx"};
    tv.Col("part", ColType::Str).Col("idx", ColType::I64).Col("key", ColType::Str).Col("cfg_value", ColType::Str)
        .Col("target", ColType::Str).Col("field_type", ColType::Str).Col("resolved", ColType::Str).Col("resolved_num", ColType::F64);
    golden::Table tm("modules", "PartModules on the prefab, in order.");
    tm.key = {"part", "module_index"};
    tm.Col("part", ColType::Str).Col("module_index", ColType::I64).Col("moduleName", ColType::Str).Col("class", ColType::Str)
        .Col("field_count", ColType::I64).Col("isEnabled", ColType::Bool);
    golden::Table tf("module_fields", "Every KSPField of every PartModule on the prefab (BaseFieldList order).");
    tf.key = {"part", "module_index", "field"};
    tf.Col("part", ColType::Str).Col("module_index", ColType::I64).Col("field", ColType::Str).Col("type", ColType::Str)
        .Col("value", ColType::Str).Col("value_num", ColType::F64).Col("isPersistant", ColType::Bool);
    golden::Table tpr("module_propellants", "ModuleEngines / ModuleRCS propellants.");
    tpr.key = {"part", "module_index", "idx"};
    tpr.Col("part", ColType::Str).Col("module_index", ColType::I64).Col("idx", ColType::I64).Col("name", ColType::Str)
        .Col("ratio", ColType::F32).Col("flowMode", ColType::Str).Col("ignoreForIsp", ColType::Bool)
        .Col("ignoreForThrustCurve", ColType::Bool).Col("drawStackGauge", ColType::Bool).Col("resource_density", ColType::F32);
    golden::Table tr("resources", "Prefab PartResources.");
    tr.key = {"part", "idx"};
    tr.Col("part", ColType::Str).Col("idx", ColType::I64).Col("name", ColType::Str).Col("amount", ColType::F64)
        .Col("maxAmount", ColType::F64).Col("flowState", ColType::Bool).Col("isTweakable", ColType::Bool)
        .Col("hideFlow", ColType::Bool).Col("isVisible", ColType::Bool).Col("flowMode", ColType::Str)
        .Col("density", ColType::F32).Col("unitCost", ColType::F32);
    golden::Table ta("attach_nodes", "Prefab attach nodes (idx -1 = srfAttachNode).");
    ta.key = {"part", "idx"};
    ta.Col("part", ColType::Str).Col("idx", ColType::I64).Col("id", ColType::Str).Col("nodeType", ColType::Str);
    for (const char* v : {"position", "orientation", "secondaryAxis"})
        for (const char* c : {"_x", "_y", "_z"}) ta.Col(std::string(v) + c, ColType::F32);
    ta.Col("size", ColType::I64).Col("radius", ColType::F32).Col("attachMethod", ColType::Str).Col("rigid", ColType::Bool)
        .Col("ResourceXFeed", ColType::Bool).Col("breakingForce", ColType::F32).Col("breakingTorque", ColType::F32)
        .Col("contactArea", ColType::F32).Col("overrideDragArea", ColType::F32);
    golden::Table tc("drag_cubes", "Loaded drag cubes.");
    tc.key = {"part", "cube_index"};
    tc.Col("part", ColType::Str).Col("cube_index", ColType::I64).Col("name", ColType::Str).Col("weight", ColType::F32);
    for (const char* v : {"area", "drag", "depth", "dragModifiers"})
        for (int i = 0; i < 6; i++) tc.Col(std::string(v) + "_" + std::to_string(i), ColType::F32);
    for (const char* v : {"center", "size"})
        for (const char* c : {"_x", "_y", "_z"}) tc.Col(std::string(v) + c, ColType::F32);
    tc.Col("saved", ColType::Str);
    golden::Table tvar("variants", "AvailablePart.Variants.");
    tvar.key = {"part", "idx"};
    tvar.Col("part", ColType::Str).Col("idx", ColType::I64).Col("name", ColType::Str).Col("displayName", ColType::Str)
        .Col("cost", ColType::F32).Col("mass", ColType::F32).Col("materials", ColType::Str);
    golden::Table tas("assets", "Files resolved for the part (cfg, models, texture replacements, internal).");
    tas.key = {"part", "idx"};
    tas.Col("part", ColType::Str).Col("idx", ColType::I64).Col("kind", ColType::Str).Col("slot", ColType::Str)
        .Col("url", ColType::Str).Col("file", ColType::Str).Col("exists", ColType::Bool).Col("source", ColType::Str);

    const PartResourceLibrary& lib = data.resources;
    const std::string gameRoot = go.gameRoot;
    size_t nFields = 0;
    for (const auto& apPtr : loader.loadedParts) {
        const AvailablePart& ap = *apPtr;
        const std::string& pn = ap.name;
        PartPrefab& p = *ap.partPrefab;
        if (!ap.loadError.empty()) {
            std::vector<Cell> row(tp.cols.size(), Cell(nullptr));
            row.front() = Cell(pn);
            row.back() = Cell(ap.loadError);
            tp.Add(std::move(row));
            continue;
        }
        // part_cfg_values
        if (ap.partConfig) {
            int vi = 0;
            for (const auto& cv : ap.partConfig->values()) {
                const PartFieldInfo* fi = FindPartField(cv.name, false);
                std::string target = "none", ftype, resolved;
                Cell rnum(nullptr), rcell(nullptr);
                if (fi) {
                    target = "Part";
                    ftype = fi->typeName;
                    FmtPartField(*fi, p, resolved, rnum);
                    rcell = Cell(resolved);
                }
                tv.Add({Cell(pn), Cell(vi++), Cell(cv.name), Cell(cv.value), Cell(target), Cell(ftype), rcell, rnum});
            }
        }
        // modules + fields + propellants
        for (size_t mi = 0; mi < p.Modules.size(); mi++) {
            PartModuleData& m = p.Modules[mi];
            tm.Add({Cell(pn), Cell(mi), Cell(m.moduleName), Cell(m.moduleClass ? m.moduleClass->name : ""),
                    Cell(m.moduleClass ? m.moduleClass->Fields().size() : 0), Cell(m.isEnabled)});
            // every KSPField of the module instance, in BaseFieldList order: cfg-set values and defaults alike
            if (m.module) {
                BaseFieldList& fields = m.module->Fields();
                std::set<std::string> seen;  // C9 lists a field name once (a `new` field hides the base class's: first wins)
                for (size_t k = 0; k < fields.Count(); k++) {
                    const BaseField& bf = *fields[k];
                    if (!seen.insert(bf.name()).second) continue;
                    std::string vs;
                    Cell num;
                    FmtModuleValue(bf.GetValue(), vs, num);
                    tf.Add({Cell(pn), Cell(mi), Cell(bf.name()), Cell(bf.FieldInfo().typeName), Cell(vs), num,
                            Cell(bf.isPersistant)});
                    nFields++;
                }
            }
            for (size_t k = 0; k < m.propellants.size(); k++) {
                Propellant& pr = m.propellants[k];
                const PartResourceDefinition* def = lib.GetDefinition(pr.name);
                const int32_t fm = pr.GetFlowMode();
                tpr.Add({Cell(pn), Cell(mi), Cell(k), Cell(pr.name), Cell(pr.ratio), Cell(kEnum_ResourceFlowMode.ToString(fm)),
                         Cell(pr.ignoreForIsp), Cell(pr.ignoreForThrustCurve), Cell(pr.drawStackGauge),
                         def ? Cell(def->density()) : Cell(nullptr)});
            }
        }
        // resources
        double resMass = 0.0, resCost = 0.0;
        for (size_t k = 0; k < p.Resources.size(); k++) {
            const PartResource& r = p.Resources[k];
            const PartResourceDefinition* def = lib.GetDefinition(r.resourceName);
            tr.Add({Cell(pn), Cell(k), Cell(r.resourceName), Cell(r.amount), Cell(r.maxAmount), Cell(r.flowState),
                    Cell(r.isTweakable), Cell(r.hideFlow), Cell(r.isVisible),
                    Cell(kEnum_PartResource_FlowMode.ToString(static_cast<int64_t>(r.flowMode))),
                    def ? Cell(def->density()) : Cell(nullptr), def ? Cell(def->unitCost()) : Cell(nullptr)});
            if (def) {
                resMass += r.amount * static_cast<double>(def->density());
                resCost += r.amount * static_cast<double>(def->unitCost());
            }
        }
        // attach nodes
        auto addNode = [&](int idx, const AttachNode& n) {
            ta.Add({Cell(pn), Cell(idx), Cell(n.id), Cell(kEnum_AttachNode_NodeType.ToString(static_cast<int64_t>(n.nodeType))),
                    Cell(n.position.x), Cell(n.position.y), Cell(n.position.z), Cell(n.orientation.x), Cell(n.orientation.y),
                    Cell(n.orientation.z), Cell(n.secondaryAxis.x), Cell(n.secondaryAxis.y), Cell(n.secondaryAxis.z),
                    Cell(n.size), Cell(n.radius), Cell(kEnum_AttachNodeMethod.ToString(static_cast<int64_t>(n.attachMethod))),
                    Cell(n.rigid), Cell(n.ResourceXFeed), Cell(n.breakingForce), Cell(n.breakingTorque),
                    Cell(n.contactArea), Cell(n.overrideDragArea)});
        };
        for (size_t k = 0; k < p.attachNodes.size(); k++) addNode(static_cast<int>(k), p.attachNodes[k]);
        if (p.srfAttachNode) addNode(-1, *p.srfAttachNode);
        // drag cubes
        const auto& cubes = p.DragCubes.Cubes();
        for (size_t k = 0; k < cubes.size(); k++) {
            const DragCube& c = cubes[k];
            std::vector<Cell> row{Cell(pn), Cell(k), Cell(c.Name()), Cell(c.Weight())};
            for (const auto* arr : {&c.Area(), &c.Drag(), &c.Depth(), &c.DragModifiers()})
                for (float f : *arr) row.push_back(Cell(f));
            for (const Vector3f* v : {&c.Center(), &c.Size()}) {
                row.push_back(Cell(v->x));
                row.push_back(Cell(v->y));
                row.push_back(Cell(v->z));
            }
            row.push_back(Cell(c.SaveToString()));
            tc.Add(std::move(row));
        }
        // variants
        if (const auto* vars = ap.Variants())
            for (size_t k = 0; k < vars->size(); k++) {
                const PartVariant& v = (*vars)[k];
                tvar.Add({Cell(pn), Cell(k), Cell(v.name), Cell(v.displayName), Cell(v.cost), Cell(v.mass), Cell(nullptr)});
            }
        // assets (texture slots of the model renderers are not produced: they need the .mu contents)
        for (size_t k = 0; k < ap.assets.size(); k++) {
            const PartAssetRef& a = ap.assets[k];
            const bool exists = !a.file.empty() && a.source == "GameData" &&
                                fs::exists(dotnet::PathFromUtf8(gameRoot) /
                                           dotnet::PathFromUtf8(a.file));
            tas.Add({Cell(pn), Cell(k), Cell(a.kind), Cell(a.slot), Cell(a.url),
                     a.file.empty() ? Cell(nullptr) : Cell(a.file), Cell(exists), Cell(a.source)});
        }
        // parts row
        const PartFields& f = p.fields;
        const auto internalName = ap.internalConfig ? ap.internalConfig->GetValue("name") : std::nullopt;
        std::string cfgRel;
        for (const PartAssetRef& a : ap.assets)
            if (a.kind == "cfg") cfgRel = a.file;
        tp.Add({Cell(pn), Cell(ap.title), Cell(ap.partUrl), Cell(cfgRel), Cell(kEnum_PartCategories.ToString(ap.category)),
                Cell(ap.TechRequired), Cell(ap.cost), Cell(ap.entryCost), Cell(f.mass), Cell(f.crashTolerance),
                Cell(f.maxTemp), Cell(f.skinMaxTemp), Cell(f.breakingForce), Cell(f.breakingTorque), Cell(f.minimum_drag),
                Cell(f.maximum_drag), Cell(f.angularDrag), Cell(kEnum_Part_DragModel.ToString(f.dragModel)),
                Cell(f.CoMOffset.x), Cell(f.CoMOffset.y), Cell(f.CoMOffset.z), Cell(f.CoPOffset.x), Cell(f.CoPOffset.y),
                Cell(f.CoPOffset.z), Cell(f.CoLOffset.x), Cell(f.CoLOffset.y), Cell(f.CoLOffset.z),
                Cell(f.heatConductivity), Cell(f.emissiveConstant), Cell(f.absorptiveConstant), Cell(f.thermalMassModifier),
                Cell(f.skinMassPerArea), Cell(f.skinInternalConductionMult), Cell(f.radiatorHeadroom),
                Cell(f.explosionPotential), Cell(f.buoyancy), Cell(f.fuelCrossFeed),
                Cell(kEnum_Part_PhysicalSignificance.ToString(f.physicalSignificance)),
                f.attachRules ? Cell(f.attachRules->String()) : Cell(nullptr),
                ap.bulkheadProfiles ? Cell(*ap.bulkheadProfiles) : Cell(nullptr), Cell(ap.partSize), Cell(f.scaleFactor),
                Cell(f.rescaleFactor), Cell(f.stackSymmetry), Cell(f.CrewCapacity), Cell(p.Modules.size()),
                Cell(p.Resources.size()), Cell(p.attachNodes.size()), Cell(p.DragCubes.None()),
                Cell(p.DragCubes.Procedural()), Cell(cubes.size()), Cell(resMass), Cell(resCost), Cell(0.0f), Cell(0.0f),
                internalName ? Cell(*internalName) : Cell(nullptr), Cell(nullptr)});
    }
    for (golden::Table* t : {&tp, &tv, &tm, &tf, &tpr, &tr, &ta, &tc, &tvar, &tas}) g.tables.push_back(std::move(*t));
    std::cout << "part_database: " << loader.loadedParts.size() << " parts, " << db.databaseTexture.size()
              << " textures, " << db.databaseModel.size() << " models, " << db.databaseAudio.size() << " audio, "
              << db.root()->AllConfigs().size() << " configs, " << nFields << " module fields, "
              << loader.cubesNeedRendering.size() << " parts without cubes\n";
    return WriteCandidate(opt, g, "part_database/part_database.json");
}

}  // namespace

KSP_GOLDEN_DATASET("part_database", 120, RunPartDatabase);
