// WP-13 unit tests: the Part model (Part.cs), PartVariant / ModulePartVariants, PartGeometryUtil, the small structural modules,
// the live-Part paths of the proto snapshots, and part trees of the original crafts.
//
// There is no golden for the numerics (no craft_tree capture exists): the expectations are derived from the decompiled source
// or taken from original game data -- the C9 part database capture (harness/captures/out2/part_database.json), the physprofile
// save (testdata/saves) and the stock crafts of harness/inputs/Ships. Parts come from the GameData mirror (testdata) through the
// WP-05 loader and the WP-06 module host.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/axis_groups_manager.hpp"
#include "ksp/modules/module_part_variants.hpp"
#include "ksp/modules/part_structural_modules.hpp"
#include "ksp/parts/available_variant_theme.hpp"
#include "ksp/parts/flag_name_manager.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_geometry_util.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/unity/bounds.hpp"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;
using namespace ksp;

namespace {

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}

// The stock game: the GameData mirror (or KSP_GAMEDATA_ROOT) loaded through GameData. One per test case (the singletons the
// loaders install -- PartLoader, PartResourceLibrary, Localizer -- belong to the most recent one).
struct Game13 {
    GameData data;
    Game13() {
        SaveEnvironment::Reset();
        GameData::Options o;
        o.gameRoot = ksp::testing::GameRoot();
        o.translate = false;
        data.Load(o);
        SaveEnvironment& env = SaveEnvironment::Current();
        env.getPartInfoByName = [this](std::string_view n) { return data.parts.getPartInfoByName(n); };
        env.partExists = [this](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
        env.newGuidHashCode = [] {
            static uint32_t s = 777u;
            s = s * 1664525u + 1013904223u;
            return static_cast<int32_t>(s);
        };
    }
    ~Game13() { SaveEnvironment::Reset(); }
    const AvailablePart& Info(const char* name) {
        const AvailablePart* p = data.parts.getPartInfoByName(name);
        REQUIRE_MESSAGE(p != nullptr, name);
        return *p;
    }
    // ShipConstruction.CreatePart-like: Instantiate(prefab) + partInfo + name
    std::unique_ptr<Part> New(const char* name) {
        const AvailablePart& ap = Info(name);
        auto p = Part::Instantiate(*ap.partPrefab);
        p->partInfo = &ap;
        p->gameObjectName = ap.name;
        return p;
    }
};

bool GameAvailable() { return fs::exists(fs::path(ksp::testing::GameRoot()) / "GameData" / "Squad"); }

// The C9 part database capture as rows keyed by column name
struct CaptureTable {
    std::vector<std::string> cols;
    nlohmann::json rows;
    int Col(const std::string& c) const {
        for (size_t i = 0; i < cols.size(); i++)
            if (cols[i] == c) return static_cast<int>(i);
        FAIL("column " << c);
        return -1;
    }
};

const nlohmann::json& Capture() {
    static const nlohmann::json j = [] {
        std::ifstream f(std::string(KSPCORE_HARNESS_DIR) + "/captures/out2/part_database.json", std::ios::binary);
        REQUIRE(f.good());
        return nlohmann::json::parse(f);
    }();
    return j;
}

CaptureTable Table(const std::string& name) {
    for (const auto& t : Capture().at("tables"))
        if (t.at("name") == name) {
            CaptureTable ct;
            for (const auto& c : t.at("columns")) ct.cols.push_back(c.at("name"));
            ct.rows = t.at("rows");
            return ct;
        }
    FAIL("table " << name);
    return {};
}

ConfigNode::NodePtr ParseNode(const char* text) {
    auto n = ConfigNode::Parse(text);
    REQUIRE(n);
    REQUIRE(n->nodes().Count() == 1);
    return n->nodes().Shared(0);
}

}  // namespace

// ================================================ small records and enums ===================================================

TEST_CASE("WP-13 enums and records: values as declared, PartValues defaults and extremum rules") {
    // AttachModes.cs, PartStates.cs, Part.PhysicalSignificance: the typed mirrors agree with the generated member tables
    CHECK(static_cast<int>(AttachModes::SRF_ATTACH) == kEnum_AttachModes.Parse("SRF_ATTACH"));
    for (const auto& [n, v] : kEnum_PartStates.members) {
        CAPTURE(n);
        CHECK(kEnum_PartStates.ToString(v) == n);
    }
    CHECK(static_cast<int>(PartStates::CARGO) == 6);
    CHECK(static_cast<int>(PartStates::FAILED) == kEnum_PartStates.Parse("FAILED"));
    CHECK(static_cast<int>(Part_PhysicalSignificance::NONE) == kEnum_Part_PhysicalSignificance.Parse("NONE"));
    CHECK(static_cast<int>(Part_ShowRigidAttachmentOption::Always) == kEnum_Part_ShowRigidAttachmentOption.Parse("Always"));
    CHECK(static_cast<int>(PartSpaceMode::Current) == 1);
    CHECK(static_cast<int>(KSPAxisMode::Incremental) == 1);
    CHECK(static_cast<int>(AttachNodeMethod::NONE) == 5);

    // PartValues.cs: defaults and the comparison direction of each member
    PartValues pv;
    CHECK(pv.MaxThrottle.value() == 1.0f);
    CHECK(pv.AutopilotSkill.value() == -1);
    CHECK(pv.EVAChuteSkill.value() == 0);
    CHECK(pv.ScienceReturnSum.value() == 1.0f);
    struct Src {
        float a = 0.5f;
        float f() { return a; }
        int32_t g() { return 3; }
    } s1, s2;
    s2.a = 0.25f;
    pv.HeatProduction.Add(&s1, &Src::f);  // HeatProduction keeps the smaller value (a < b)
    pv.HeatProduction.Add(&s2, &Src::f);
    pv.MaxThrottle.Add(&s1, &Src::f);     // MaxThrottle keeps the larger value (a > b), starting from 1
    pv.AutopilotSkill.Add(&s1, &Src::g);
    pv.ScienceReturnSum.Add(&s1, &Src::f);
    pv.ScienceReturnSum.Add(&s2, &Src::f);
    pv.Update();  // EventValueWrapper.Update: every member's Update
    CHECK(pv.HeatProduction.value() == 0.25f);
    CHECK(pv.MaxThrottle.value() == 1.0f);   // 0.5 is not > 1
    CHECK(pv.AutopilotSkill.value() == 3);
    CHECK(pv.ScienceReturnSum.value() == 1.75f);  // 1 + 0.5 + 0.25 (operation, not comparison)

    // PartHeightQuery / PartPointer / PartRawInfo / PartGameObjectInfo
    PartHeightQuery q(-2.5f);
    CHECK(q.lowestPoint == -2.5f);
    CHECK(q.lowestOnParts.empty());
    PartRawInfo ri("_Cutoff", "0.5");
    CHECK(ri.Name() == "_Cutoff");
    CHECK(ri.Value() == "0.5");
    PartGameObjectInfo gi("Cap", false);
    CHECK(gi.Name() == "Cap");
    CHECK_FALSE(gi.Status());
    PartPointer pp;
    CHECK(pp.PartReference() == nullptr);
    Part part;
    pp.SetPart(&part);
    CHECK(pp.PartReference() == &part);
    CHECK(pp.GetPart() == &part);
    KSPModelEvent me("start", "finish", true);
    CHECK(me.startEvent == "start");
    CHECK(me.allowMultiple);
    KSPPart kp;
    CHECK(kp.stageIcon == 0);  // DefaultIcons.CUSTOM
}

TEST_CASE("WP-13 FlagNameManager and AvailableVariantTheme") {
    FlagMesh a{"Small", 0, 0, FlagOrientation::LANDSCAPE, "Small Flag", "", 0, 0, ""};
    FlagMesh b{"Medium", 1, 0, FlagOrientation::LANDSCAPE, "Medium Flag", "", 0, 0, ""};
    FlagMesh c{"Strut", 2, FlagNameManager::STRUCTURAL_PART, FlagOrientation::PORTRAIT, "Strut Flag", "", 0, 0, ""};
    FlagNameManager empty;
    CHECK(empty.SizeNames(FlagOrientation::LANDSCAPE, 0).empty());
    CHECK_FALSE(empty.HasSizeNames(FlagOrientation::LANDSCAPE, 0));
    FlagNameManager m(std::vector<FlagMesh>{a, b, c});
    CHECK(m.SizeNames(FlagOrientation::LANDSCAPE, FlagNameManager::NON_STRUCTURAL_PART) == std::vector<std::string>{"Small", "Medium"});
    CHECK(m.DisplayNames(FlagOrientation::LANDSCAPE, 0) == std::vector<std::string>{"Small Flag", "Medium Flag"});
    CHECK(m.HasSizeNames(FlagOrientation::PORTRAIT, FlagNameManager::STRUCTURAL_PART));
    CHECK_FALSE(m.HasSizeNames(FlagOrientation::PORTRAIT, 0));
    CHECK(m.SizeNames(FlagOrientation::PORTRAIT, 7).empty());

    Localizer::Shutdown();
    Localizer::InitWithBuiltInEnglish();
    AvailableVariantTheme t = AvailableVariantTheme::CreateVariantTheme();
    CHECK(t.primaryColor == "#ffffff");
    CHECK(t.secondaryColor == "#f0f0f0");
    CHECK(t.parts.empty());
    ConfigNode n("VARIANTTHEME");
    n.AddValue("name", "Orange");
    n.AddValue("displayName", "Orange Theme");
    n.AddValue("primaryColor", "#ff8000");
    t.Load(n);
    CHECK(t.name == "Orange");
    CHECK(t.displayName == "Orange Theme");
    CHECK(t.primaryColor == "#ff8000");
    CHECK(t.secondaryColor == "#f0f0f0");  // not in the node: unchanged
    ConfigNode out("X");
    t.Save(out);
    CHECK(out.values().Count() == 5);
    CHECK(out.GetValue("name") == "Orange");
    CHECK(out.values()[4].name == "secondaryColor");
    Localizer::Shutdown();
}

// ================================================= Bounds, PartGeometryUtil ===================================================

TEST_CASE("WP-13 Bounds and PartGeometryUtil: merge, centroid, filtering by variant GameObjects and the ignore list") {
    // Bounds.cs: Encapsulate(point) = SetMinMax(Min(min, p), Max(max, p)); all values below are exact binary fractions
    Bounds b(Vector3f(1.0f, 2.0f, 3.0f), Vector3f(2.0f, 4.0f, 6.0f));  // extents (1,2,3)
    CHECK(b.min().Equals(Vector3f(0.0f, 0.0f, 0.0f)));
    CHECK(b.max().Equals(Vector3f(2.0f, 4.0f, 6.0f)));
    CHECK(b.size().Equals(Vector3f(2.0f, 4.0f, 6.0f)));
    b.Encapsulate(Vector3f(-2.0f, 1.0f, 8.0f));
    CHECK(b.min().Equals(Vector3f(-2.0f, 0.0f, 0.0f)));
    CHECK(b.max().Equals(Vector3f(2.0f, 4.0f, 8.0f)));
    CHECK(b.center().Equals(Vector3f(0.0f, 2.0f, 4.0f)));
    Bounds e;
    e.Encapsulate(Bounds(Vector3f(10.0f, 0.0f, 0.0f), Vector3f(2.0f, 2.0f, 2.0f)));  // from a zero-size box at the origin
    CHECK(e.min().Equals(Vector3f(0.0f, -1.0f, -1.0f)));
    CHECK(e.max().Equals(Vector3f(11.0f, 1.0f, 1.0f)));
    Bounds x(Vector3f(0, 0, 0), Vector3f(2, 2, 2));
    x.Expand(2.0f);
    CHECK(x.extents().Equals(Vector3f(2, 2, 2)));
    x.Expand(Vector3f(2, 4, 0));
    CHECK(x.extents().Equals(Vector3f(3, 4, 2)));

    // PartGeometryUtil.MergeBounds(Bounds[], Transform): the merged box starts as a zero box at the transform position
    const std::vector<Bounds> list{Bounds(Vector3f(1, 0, 0), Vector3f(2, 2, 2)), Bounds(Vector3f(-4, 0, 0), Vector3f(2, 2, 2))};
    Bounds merged = PartGeometryUtil::MergeBounds(list, Vector3f(0, 0, 0));
    CHECK(merged.min().Equals(Vector3f(-5, -1, -1)));
    CHECK(merged.max().Equals(Vector3f(2, 1, 1)));
    // a transform far away is part of the merged box (sic: the zero box at its position is never removed)
    Bounds mergedFar = PartGeometryUtil::MergeBounds(list, Vector3f(0, 10, 0));
    CHECK(mergedFar.max().y == 10.0f);
    Bounds mergedFirst = PartGeometryUtil::MergeBounds(list);  // centred on the first bounds
    CHECK(mergedFirst.min().Equals(Vector3f(-5, -1, -1)));
    CHECK(PartGeometryUtil::MergeBounds(std::vector<Bounds>{}).size().Equals(Vector3f::zero()));
    // the Dictionary<Bounds[], PosRot> overload: rotate by relativeTo, offset by each entry
    const Quaternionf qid = Quaternionf::identity();
    std::vector<std::pair<std::vector<Bounds>, PartGeometryUtil::PosRot>> centered;
    centered.emplace_back(std::vector<Bounds>{Bounds(Vector3f(0, 0, 0), Vector3f(2, 2, 2))}, PartGeometryUtil::PosRot(Vector3f(3, 0, 0), qid));
    Bounds mc = PartGeometryUtil::MergeBounds(centered, PartGeometryUtil::PosRot(Vector3f(0, 0, 0), qid));
    CHECK(mc.min().Equals(Vector3f(0, -1, -1)));
    CHECK(mc.max().Equals(Vector3f(4, 1, 1)));
    // FindBoundsCentroid: centres weighted by the box diagonal; two equal boxes -> the midpoint
    const std::vector<Bounds> two{Bounds(Vector3f(0, 0, 0), Vector3f(2, 2, 2)), Bounds(Vector3f(4, 0, 0), Vector3f(2, 2, 2))};
    CHECK(PartGeometryUtil::FindBoundsCentroid(two, nullptr).Equals(Vector3f(2, 0, 0)));
    PartTransform local;
    local.position = Vector3f(2, 0, 0);
    CHECK(PartGeometryUtil::FindBoundsCentroid(two, &local).Equals(Vector3f(0, 0, 0)));

    // GetPartRendererBounds: mesh renderers then skinned ones; names in partRendererBoundsIgnore and the selected variant's
    // switched-off GameObjects are dropped
    Part part;
    part.modelRenderers = {{"a", false, false, Bounds(Vector3f(0, 0, 0), Vector3f(2, 2, 2))},
                           {"ignored", false, false, Bounds(Vector3f(50, 0, 0), Vector3f(2, 2, 2))},
                           {"skin", true, false, Bounds(Vector3f(0, 8, 0), Vector3f(2, 2, 2))},
                           {"fx", false, true, Bounds(Vector3f(0, 0, 40), Vector3f(2, 2, 2))}};
    part.fields.partRendererBoundsIgnore = std::vector<std::string>{"ignored"};
    std::vector<Bounds> pb = PartGeometryUtil::GetPartRendererBounds(part);
    REQUIRE(pb.size() == 3);  // a, fx (mesh renderers, in order), then skin
    CHECK(pb[0].center().Equals(Vector3f(0, 0, 0)));
    CHECK(pb[1].center().Equals(Vector3f(0, 0, 40)));
    CHECK(pb[2].center().Equals(Vector3f(0, 8, 0)));
    // GetRendererBounds / WithoutParticles over renderer lists
    CHECK(PartGeometryUtil::GetRendererBounds(part.modelRenderers).max().x == 51.0f);
    CHECK(PartGeometryUtil::GetRendererBoundsWithoutParticles(part.modelRenderers).max().z == 1.0f);
    CHECK(PartGeometryUtil::GetPartRendererBound(part).max().z == 41.0f);
    part.colliderBounds = {Bounds(Vector3f(1, 1, 1), Vector3f(2, 2, 2))};
    CHECK(PartGeometryUtil::GetColliderBounds(part).size() == 1);
    CHECK(PartGeometryUtil::GetColliderBounds(part.colliderBounds).center().Equals(Vector3f(1, 1, 1)));
    // PartLoader.cs:937: partSize = |merged size| with the box centred on the part transform
    part.partTransform.position = Vector3f(0, 0, 0);
    part.modelRenderers = {{"m", false, false, Bounds(Vector3f(0, 0, 0), Vector3f(2, 2, 2))}};
    part.fields.partRendererBoundsIgnore.reset();
    CHECK(PartGeometryUtil::PartSize(part) == Vector3f(2, 2, 2).magnitude());
}

// ================================================ prefabs of the stock parts (C9) ==============================================

TEST_CASE("WP-13 Part prefabs: the Part model agrees with the C9 part database on counts, resource mass, module mass and cost") {
    if (!GameAvailable()) return;
    Game13 g;
    CaptureTable parts = Table("parts");
    const int cName = parts.Col("name"), cErr = parts.Col("error"), cMods = parts.Col("module_count"), cRes = parts.Col("resource_count"),
              cNodes = parts.Col("attach_node_count"), cCrew = parts.Col("CrewCapacity"), cSym = parts.Col("stackSymmetry"),
              cResMass = parts.Col("resource_mass"), cModMass = parts.Col("module_mass"), cModCost = parts.Col("module_cost"),
              cMass = parts.Col("mass"), cCost = parts.Col("cost"), cDrag = parts.Col("dragcube_count");
    int checked = 0, withResources = 0, withModuleMass = 0;
    for (const auto& row : parts.rows) {
        if (!row[cErr].is_null()) continue;
        const std::string name = row[cName];
        const AvailablePart* ap = g.data.parts.getPartInfoByName(name);
        REQUIRE_MESSAGE(ap != nullptr, name);
        if (ap->codeBuilt) continue;  // Unity-serialized prefab parts (EVA kerbals, flag): not part of the gate
        CAPTURE(name);
        Part& p = *ap->partPrefab;
        // Part.AddComponent / Awake / OnLoad ran for the prefab, partInfo was linked at the end of ParsePart
        CHECK(p.partInfo == ap);
        CHECK((p.className == "Part" || p.className == "CompoundPart"));  // fuelLine, strutConnector
        CHECK(p.fields.partName.value_or("") == p.className);              // Part.AttributeSetup: the class name
        CHECK(p.isAttached());
        CHECK(p.fxGroupNames == std::vector<std::string>{"prelaunch", "activate", "active", "deactivate"});
        CHECK(p.Modules().Count() == p.ModuleData.size());
        for (size_t i = 0; i < p.Modules().Count(); i++) {
            CHECK(p.Modules().GetModule(i)->part == &p);
            CHECK(p.Modules().GetModule(i) == p.ModuleData[i].module.get());
        }
        for (const AttachNode& n : p.attachNodes) CHECK(n.owner == &p);
        // counts and the Part's own crew / symmetry fields
        CHECK(static_cast<int64_t>(p.Modules().Count()) == row[cMods].get<int64_t>());
        CHECK(static_cast<int64_t>(p.Resources.size()) == row[cRes].get<int64_t>());
        CHECK(static_cast<int64_t>(p.attachNodes.size()) == row[cNodes].get<int64_t>());
        CHECK(static_cast<int64_t>(p.DragCubes.Cubes().size()) == row[cDrag].get<int64_t>());
        CHECK(p.fields.CrewCapacity == row[cCrew].get<int>());
        CHECK(p.fields.stackSymmetry == row[cSym].get<int>());
        // Part.GetResourceMass: the double sum of amount * density rounded once to float (C9's column is the double sum)
        const float rm = static_cast<float>(row[cResMass].get<double>());
        CHECK(Bits(p.GetResourceMass()) == Bits(rm));
        if (!p.Resources.empty()) withResources++;
        // Part.GetModuleMass(partInfo.cost-free default) / GetModuleCosts over the module host
        const float mm = static_cast<float>(row[cModMass].get<double>());
        const float mc = static_cast<float>(row[cModCost].get<double>());
        CHECK(Bits(p.GetModuleMass(p.fields.mass)) == Bits(mm));
        CHECK(Bits(p.GetModuleCosts(ap->cost)) == Bits(mc));
        if (mm != 0.0f) withModuleMass++;
        CHECK(Bits(p.fields.mass) == Bits(static_cast<float>(row[cMass].get<double>())));
        CHECK(Bits(ap->cost) == Bits(static_cast<float>(row[cCost].get<double>())));
        checked++;
    }
    CHECK(checked == 353);  // 362 captured - 4 DLC-suit error rows - 5 Unity prefab parts (kerbalEVA, kerbalEVAfemale, flag, the 2 slim suits)
    CHECK(withResources > 100);
    CHECK(withModuleMass == 7);  // fairingSize1..3, PotatoComet, PotatoRoid, fireworksLauncherBig / Small (STATUS WP-06)
}

TEST_CASE("WP-13 Part.Instantiate: a clone of every stock prefab, UpdateMass against the C9 mass + module mass") {
    if (!GameAvailable()) return;
    Game13 g;
    CaptureTable parts = Table("parts");
    const int cName = parts.Col("name"), cErr = parts.Col("error"), cMass = parts.Col("mass"), cModMass = parts.Col("module_mass"),
              cModCost = parts.Col("module_cost");
    int cloned = 0, variantParts = 0;
    for (const auto& row : parts.rows) {
        if (!row[cErr].is_null()) continue;
        const std::string name = row[cName];
        const AvailablePart* ap = g.data.parts.getPartInfoByName(name);
        if (ap->codeBuilt) continue;
        CAPTURE(name);
        const Part& proto = *ap->partPrefab;
        std::unique_ptr<Part> c = g.New(name.c_str());
        // the Unity clone: same serialized data, an independent object graph
        CHECK(c.get() != &proto);
        CHECK(c->className == proto.className);
        CHECK(c->Modules().Count() == proto.Modules().Count());
        CHECK(c->attachNodes.size() == proto.attachNodes.size());
        CHECK(c->Resources.size() == proto.Resources.size());
        CHECK(c->parent == nullptr);
        CHECK(c->children.empty());
        CHECK(c->fields.craftID == static_cast<uint32_t>(c->instanceID));  // Part.Awake: craftID = GetInstanceID()
        for (size_t i = 0; i < c->attachNodes.size(); i++) {
            CHECK(c->attachNodes[i].owner == c.get());
            CHECK(c->attachNodes[i].id == proto.attachNodes[i].id);
            CHECK(c->attachNodes[i].position.Equals(proto.attachNodes[i].position));
        }
        for (size_t i = 0; i < c->Modules().Count(); i++) {
            PartModule& m = *c->Modules().GetModule(i);
            const PartModule& pm = *proto.Modules().GetModule(i);
            CHECK(&m != &pm);
            CHECK(m.part == c.get());
            CHECK(m.ClassName() == pm.ClassName());
            // every non-private KSPField value is the prefab's (the private ones restart from their initialiser)
            for (size_t f = 0; f < m.FieldCount(); f++) {
                if (m.FieldInfos()[f]->isPrivate) continue;
                CAPTURE(m.FieldInfos()[f]->name);
                CHECK(m.ValueAt(f).index() == pm.ValueAt(f).index());
            }
        }
        // UpdateMass: mass = prefabMass + sum of the IPartMassModifier modules, clamped to partInfo.MinimumMass (Part.cs:4026)
        if (c->variants) {  // ShipConstruction.CreatePart: SetVariant(partInfo.variant.Name) selects the base variant
            variantParts++;
            REQUIRE(ap->variant);
            CHECK(c->variants->SetVariant(ap->variant->name));
        }
        c->UpdateMass();
        const float prefabMass = static_cast<float>(row[cMass].get<double>());
        const float moduleMass = static_cast<float>(row[cModMass].get<double>());
        float expected = f32store(fx(prefabMass) + fx(moduleMass));
        if (expected < ap->MinimumMass()) expected = ap->MinimumMass();
        CHECK(Bits(c->fields.mass) == Bits(expected));
        CHECK(Bits(c->fields.prefabMass) == Bits(prefabMass));
        CHECK_FALSE(c->fields.needPrefabMass);
        CHECK(Bits(c->GetModuleCosts(ap->cost)) == Bits(static_cast<float>(row[cModCost].get<double>())));
        cloned++;
    }
    CHECK(cloned == 353);
    CHECK(variantParts > 50);
}
