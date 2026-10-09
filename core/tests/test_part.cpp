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
#include "ksp/unity/quaternion_native.hpp"
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

// ================================================= attach nodes and the part tree ===============================================

namespace {
// attach `child`'s node to `parent`'s node (the ShipConstruct link/attN resolution: setParent, nodes' attachedPart both ways)
void Attach(Part& parent, const char* parentNode, Part& child, const char* childNode) {
    AttachNode* pn = parent.FindAttachNode(parentNode);
    AttachNode* cn = child.FindAttachNode(childNode);
    REQUIRE(pn != nullptr);
    REQUIRE(cn != nullptr);
    pn->attachedPart = &child;
    cn->attachedPart = &parent;
    child.setParent(&parent);
}
}  // namespace

TEST_CASE("WP-13 Part attach nodes: lookups, owner links, mirroring, nodes of the C9 attach_nodes table") {
    if (!GameAvailable()) return;
    Game13 g;
    CaptureTable an = Table("attach_nodes");
    const int cPart = an.Col("part"), cIdx = an.Col("idx"), cId = an.Col("id"), cPx = an.Col("position_x"), cPy = an.Col("position_y"),
              cPz = an.Col("position_z"), cSize = an.Col("size");
    std::map<std::string, std::unique_ptr<Part>> clones;
    int nodes = 0;
    for (const auto& row : an.rows) {
        const std::string name = row[cPart];
        const AvailablePart* ap = g.data.parts.getPartInfoByName(name);
        if (ap == nullptr || ap->codeBuilt) continue;
        auto& c = clones[name];
        if (!c) c = g.New(name.c_str());
        CAPTURE(name);
        const int idx = row[cIdx].get<int>();
        const AttachNode* node = idx >= 0 ? &c->attachNodes[static_cast<size_t>(idx)] : c->srfAttachNode.get();
        REQUIRE(node != nullptr);
        CHECK(node->id == row[cId].get<std::string>());
        CHECK(Bits(node->position.x) == Bits(static_cast<float>(row[cPx].get<double>())));
        CHECK(Bits(node->position.y) == Bits(static_cast<float>(row[cPy].get<double>())));
        CHECK(Bits(node->position.z) == Bits(static_cast<float>(row[cPz].get<double>())));
        CHECK(node->size == row[cSize].get<int>());
        if (idx >= 0) {
            // Part.FindAttachNode: the first node with that id
            AttachNode* found = c->FindAttachNode(node->id);
            REQUIRE(found != nullptr);
            CHECK(found->id == node->id);
            CHECK(found <= node);
        }
        nodes++;
    }
    CHECK(nodes == 822);  // the C9 attach_nodes rows of the 357 compiled parts
    Part& tank = *clones.at("fuelTank");
    // FindAttachNodes(partial) returns every node whose id contains the text; none -> empty (C#: null)
    CHECK(tank.FindAttachNodes("o").size() == 2);   // top, bottom
    CHECK(tank.FindAttachNodes("bott").size() == 1);
    CHECK(tank.FindAttachNodes("nothing").empty());
    CHECK(tank.FindAttachNode("nothing") == nullptr);
    CHECK(tank.TopNode() == tank.FindAttachNode("top"));
    CHECK(tank.FindAttachNodeByPart(nullptr) == nullptr);
    CHECK(tank.FindAttachNodeByPart(clones.at("mk1pod.v2").get()) == nullptr);
    // SetupAttachNodes: a node named "attach" becomes the srfAttachNode, renamed "srfAttach" (Part.cs:6077)
    Part syn;
    AttachNode a;
    a.id = "attach";
    a.nodeType = AttachNodeType::Surface;
    a.position = Vector3f(1, 2, 3);
    syn.attachNodes.push_back(a);
    syn.SetupAttachNodes();
    REQUIRE(syn.srfAttachNode);
    CHECK(syn.srfAttachNode->id == "srfAttach");
    CHECK(syn.attachNodes[0].id == "srfAttach");
    // SetMirror: positions and orientations are the originals scaled by the mirror vector
    tank.attachNodes[0].originalPosition = Vector3f(1, 2, 3);
    tank.attachNodes[0].originalOrientation = Vector3f(0, 1, 0);
    tank.SetMirror(Vector3f(-1, 1, 1));
    CHECK(tank.fields.isMirrored);
    CHECK(tank.attachNodes[0].position.Equals(Vector3f(-1, 2, 3)));
    CHECK(tank.attachNodes[0].orientation.Equals(Vector3f(-0.0f, 1, 0)));
    tank.SetMirror(Vector3f::one());
    CHECK_FALSE(tank.fields.isMirrored);
    CHECK(tank.attachNodes[0].position.Equals(Vector3f(1, 2, 3)));
    // Part.AddAttachNode(NODE{name, transform, size, method, crossfeed, rigid}) with a model transform resolver
    auto node = ParseNode("NODE\n{\n name = hatch\n transform = HatchTransform\n size = 2\n method = LOCKED_JOINT\n crossfeed = False\n rigid = True\n}\n");
    AttachNode* added = tank.AddAttachNode(*node, [](const std::string& t) -> std::optional<std::pair<Vector3f, Vector3f>> {
        if (t == "HatchTransform") return std::make_pair(Vector3f(0, 0.5f, 1), Vector3f(0, 0, 1));
        return std::nullopt;
    });
    REQUIRE(added != nullptr);
    CHECK(added->id == "hatch");
    CHECK(added->owner == &tank);
    CHECK(added->size == 2);
    CHECK(added->attachMethod == AttachNodeMethod::LOCKED_JOINT);
    CHECK_FALSE(added->ResourceXFeed);
    CHECK(added->rigid);
    CHECK(added->position.Equals(Vector3f(0, 0.5f, 1)));
    CHECK(added->orientation.Equals(Vector3f(0, 0, 1)));  // transform.forward
    CHECK(added->originalPosition.Equals(added->position));
    CHECK_THROWS_WITH_AS(tank.AddAttachNode(*node, nullptr), "Part: Cannot add attach node. Transform of name 'HatchTransform' not found", Exception);
    auto noName = ParseNode("NODE\n{\n transform = X\n}\n");
    CHECK_THROWS_AS(tank.AddAttachNode(*noName, nullptr), Exception);
    // UpdateAttachNodes: nodes with a model transform follow it in the part's local space
    tank.nodeTransformResolver = [](const std::string&) { return std::make_optional(std::make_pair(Vector3f(5, 0, 0), Vector3f(1, 0, 0))); };
    tank.partTransform.position = Vector3f(4, 0, 0);
    tank.UpdateAttachNodes();
    CHECK(added->position.Equals(Vector3f(1, 0, 0)));  // world (5,0,0) in the local space of a part at (4,0,0)
}

TEST_CASE("WP-13 Part tree: parent / children, indirect queries, symmetry, physics significance, re-rooting") {
    if (!GameAvailable()) return;
    Game13 g;
    // pod (root) -> tank -> engine, a parachute surface-attached to the tank
    auto pod = g.New("mk1pod.v2");
    auto tank = g.New("fuelTank");
    auto engine = g.New("liquidEngine");
    auto chute = g.New("parachuteSingle");
    Attach(*pod, "bottom", *tank, "top");
    Attach(*tank, "bottom", *engine, "top");
    tank->addChild(chute.get());  // surface attach: child of the tank through srfAttach
    chute->parent = tank.get();
    chute->srfAttachNode->attachedPart = tank.get();
    chute->srfAttachNode->owner = chute.get();
    chute->fields.attachMode = static_cast<int>(AttachModes::SRF_ATTACH);

    CHECK(pod->children == std::vector<Part*>{tank.get()});
    CHECK(tank->children == (std::vector<Part*>{engine.get(), chute.get()}));
    CHECK(tank->parent == pod.get());
    CHECK(tank->potentialParent == pod.get());
    CHECK(pod->hasIndirectChild(engine.get()));
    CHECK(pod->hasIndirectChild(pod.get()));  // a part is its own indirect child
    CHECK_FALSE(engine->hasIndirectChild(pod.get()));
    // Part.hasIndirectParent (sic): recurses with the parent itself, so any part with a parent answers true
    CHECK(engine->hasIndirectParent(pod.get()));
    CHECK(engine->hasIndirectParent(chute.get()));
    CHECK_FALSE(pod->hasIndirectParent(engine.get()));
    CHECK(engine->localRoot() == pod.get());
    CHECK(pod->localRoot() == pod.get());
    CHECK(engine->FindChildPart("fuelTank", true) == nullptr);  // gameObject.name is the part name only after ConfigurePart
    tank->gameObjectName = "fuelTank";
    chute->gameObjectName = "parachuteSingle";
    CHECK(pod->FindChildPart("fuelTank") == tank.get());
    CHECK(pod->FindChildPart("parachuteSingle") == nullptr);                     // not a direct child
    CHECK(pod->FindChildPart("parachuteSingle", true) == chute.get());            // recursive
    CHECK(Part::FindChildPart(tank.get(), "parachuteSingle") == chute.get());
    // attach node queries
    CHECK(pod->FindAttachNodeByPart(tank.get()) == pod->FindAttachNode("bottom"));
    CHECK(tank->FindAttachNodeByPart(pod.get()) == tank->FindAttachNode("top"));
    CHECK(chute->FindAttachNodeByPart(tank.get()) == chute->srfAttachNode.get());
    CHECK(tank->FindAttachNode("top")->FindOpposingNode() == pod->FindAttachNode("bottom"));
    CHECK(pod->FindPartThroughNodes(engine.get()) == pod->FindAttachNode("bottom"));  // reached through the tank
    CHECK(pod->FindPartThroughNodes(chute.get()) == nullptr);  // the chute is linked only through ITS srfAttachNode
    CHECK(chute->FindPartThroughNodes(pod.get()) == chute->srfAttachNode.get());
    CHECK(engine->FindPartThroughNodes(pod.get()) == engine->FindAttachNode("top"));
    CHECK(pod->FindPartThroughNodes(tank.get(), tank.get()) != nullptr);
    CHECK(pod->FindPartThroughNodes(engine.get(), pod.get()) == nullptr);  // src == this
    CHECK(pod->FindAttachNode("top")->attachedPart == nullptr);
    CHECK(pod->isAttachable());           // bottom is taken, top is free
    pod->FindAttachNode("top")->attachedPart = tank.get();
    CHECK_FALSE(pod->isAttachable());     // both nodes taken
    pod->FindAttachNode("top")->attachedPart = nullptr;
    CHECK(chute->isAttachable());         // SRF_ATTACH
    // GetPartsOutTo(part, maxLinks): children (last first), then the parent; a HashSet in insertion order
    std::vector<Part*> out;
    Part::GetPartsOutTo(tank.get(), out, 1);
    CHECK(out == (std::vector<Part*>{tank.get(), chute.get(), engine.get(), pod.get()}));
    std::vector<Part*> out0;
    Part::GetPartsOutTo(tank.get(), out0, 0);
    CHECK(out0 == std::vector<Part*>{tank.get()});
    // physical significance: NONE parts are skipped by FindNonPhysicslessChildren / Parent
    chute->fields.physicalSignificance = static_cast<int>(Part_PhysicalSignificance::NONE);
    std::vector<Part*> phys;
    pod->FindNonPhysicslessChildren(phys);
    CHECK(phys == (std::vector<Part*>{tank.get(), engine.get()}));
    CHECK(chute->FindNonPhysicslessParent() == tank.get());
    CHECK(pod->FindNonPhysicslessParent() == pod.get());
    // GetPhysicslessChildMass: children without a Rigidbody: mass + resource mass (float sum of (mass + resource mass))
    chute->fields.mass = 0.25f;
    engine->fields.mass = 1.5f;
    engine->hasRigidbody = true;
    tank->fields.mass = 1.0f;
    CHECK(tank->GetPhysicslessChildMass() == 0.25f);  // the chute: no rigidbody, no resources
    engine->hasRigidbody = false;
    CHECK(tank->GetPhysicslessChildMass() == 1.75f);
    // PromoteToPhysicalPart / DemoteToPhysicslessPart: the physicalSignificance state and rigidbody presence
    chute->PromoteToPhysicalPart();
    CHECK(chute->fields.physicalSignificance == static_cast<int>(Part_PhysicalSignificance::FULL));
    CHECK(chute->hasRigidbody);
    chute->DemoteToPhysicslessPart();
    CHECK(chute->fields.physicalSignificance == static_cast<int>(Part_PhysicalSignificance::NONE));
    CHECK(chute->fields.previousPhysicalSignificance == static_cast<int>(Part_PhysicalSignificance::FULL));
    CHECK_FALSE(chute->hasRigidbody);
    CHECK(Part::allParts.size() >= 4);

    // symmetry: getSymmetryCounterPart wraps around count + 1
    auto t2 = g.New("fuelTank");
    auto t3 = g.New("fuelTank");
    tank->symmetryCounterparts = {t2.get(), t3.get()};
    t2->symmetryCounterparts = {tank.get(), t3.get()};
    t3->symmetryCounterparts = {tank.get(), t2.get()};
    CHECK(tank->getSymmetryCounterPart(0) == tank.get());
    CHECK(tank->getSymmetryCounterPart(1) == t2.get());
    CHECK(tank->getSymmetryCounterPart(2) == t3.get());
    CHECK(tank->getSymmetryCounterPart(3) == tank.get());  // 3 % (2 + 1) == 0
    CHECK(tank->isSymmetryCounterPart(t3.get()));
    CHECK_FALSE(tank->isSymmetryCounterPart(pod.get()));
    t2->CleanSymmetryReferences();  // removes t2 from both counterparts and empties its own list
    CHECK(t2->symmetryCounterparts.empty());
    CHECK(tank->symmetryCounterparts == std::vector<Part*>{t3.get()});
    CHECK(t3->symmetryCounterparts == std::vector<Part*>{tank.get()});

    // resource priority (Part.cs:7744-7777) fires onPartPriorityChanged and follows the symmetry set
    int fired = 0;
    struct L {
        int* n;
        void On(Part*) { (*n)++; }
    } listener{&fired};
    GameEvents::onPartPriorityChanged.Add(&listener, &L::On);
    tank->fields.inverseStage = 3;
    CHECK(tank->GetResourcePriority() == 30);
    tank->ChangeResourcePriority(2);
    CHECK(tank->fields.resourcePriorityOffset == 2);
    CHECK(t3->fields.resourcePriorityOffset == 2);
    CHECK(tank->GetResourcePriority() == 32);
    tank->fields.resourcePriorityUseParentInverseStage = true;
    pod->fields.inverseStage = 5;
    CHECK(tank->GetResourcePriority() == 52);  // the parent's inverse stage
    tank->ResetPri();
    CHECK(t3->fields.resourcePriorityOffset == 0);
    CHECK(fired == 2);
    GameEvents::onPartPriorityChanged.Remove(&listener, &L::On);

    // setParent(null) removes the part from its parent's children and fires onPartRemove
    engine->setParent(nullptr);
    CHECK(engine->parent == nullptr);
    CHECK(tank->children == std::vector<Part*>{chute.get()});
    engine->setParent(tank.get());

    // re-rooting at the tank (the editor's SetHierarchyRoot): the old parent becomes a child, attach modes follow
    engine->fields.attachMode = static_cast<int>(AttachModes::STACK);
    tank->SetHierarchyRoot(tank.get());
    CHECK(tank->parent == nullptr);
    CHECK(pod->parent == tank.get());
    CHECK(std::find(tank->children.begin(), tank->children.end(), pod.get()) != tank->children.end());
    CHECK(pod->children.empty());
    CHECK(pod->fields.attachMode == static_cast<int>(AttachModes::STACK));
}

TEST_CASE("WP-13 AttachNode: surface node inversion on re-rooting (ReverseSrfNodeDirection, ChangeSrfNodePosition)") {
    // part A (root) at the origin with two stack nodes; part B surface-attached to A at A-local (1,0,0); B at world (1,0,0)
    Part a, b;
    AttachNode n0, n1;
    n0.id = "top";
    n0.position = Vector3f(0, 1, 0);
    n0.orientation = Vector3f(0, 1, 0);
    n1.id = "bottom";
    n1.position = Vector3f(0, -1, 0);
    n1.orientation = Vector3f(0, -1, 0);
    n0.owner = n1.owner = &a;
    a.attachNodes = {n0, n1};
    a.srfAttachNode = std::make_shared<AttachNode>();
    a.srfAttachNode->owner = &a;
    b.srfAttachNode = std::make_shared<AttachNode>();
    b.srfAttachNode->owner = &b;
    b.srfAttachNode->nodeType = AttachNodeType::Surface;
    b.srfAttachNode->position = Vector3f(0, 0, 0.5f);
    b.srfAttachNode->attachedPart = &a;
    b.partTransform.position = Vector3f(1, 0, 0);
    b.parent = &a;
    a.children = {&b};
    // ReverseSrfNodeDirection: a's srf node now points at b, positioned where b's node was (in a's space: world (1,0,0.5))
    a.srfAttachNode->ReverseSrfNodeDirection(*b.srfAttachNode);
    CHECK(b.srfAttachNode->attachedPart == nullptr);
    CHECK(a.srfAttachNode->attachedPart == &b);
    CHECK(a.srfAttachNode->position.Equals(Vector3f(1, 0, 0.5f)));  // TransformPoint of (0,0,0.5) in b's frame at (1,0,0), then into a's
    // ChangeSrfNodePosition: a node with an attached part is left alone; a free one takes the first free attach node of the owner
    a.srfAttachNode->attachedPart = nullptr;
    a.attachNodes[0].attachedPart = &b;  // "top" is taken
    a.srfAttachNode->originalOrientation = Vector3f(1, 0, 0);
    a.srfAttachNode->ChangeSrfNodePosition();
    CHECK(a.srfAttachNode->position.Equals(Vector3f(0, -1, 0)));  // "bottom": the first node without an attached part
    CHECK(a.srfAttachNode->orientation.Equals(Vector3f(0, -1, 0)));
    // every owner node taken -> the node is set to the centre of the part (and a warning is logged)
    a.attachNodes[1].attachedPart = &b;
    a.srfAttachNode->attachedPart = nullptr;
    a.srfAttachNode->ChangeSrfNodePosition();
    CHECK(a.srfAttachNode->position.Equals(Vector3f::zero()));
    CHECK(a.srfAttachNode->orientation.Equals(Vector3f(1, 0, 0)));  // originalOrientation
    // FindAttachedPart: GetPartByCraftID, last match, id 0 -> none
    a.fields.craftID = 7;
    b.fields.craftID = 7;
    AttachNode link;
    link.attachedPartId = 7;
    link.FindAttachedPart({&a, &b});
    CHECK(link.attachedPart == &b);
    link.attachedPartId = 0;
    link.FindAttachedPart({&a, &b});
    CHECK(link.attachedPart == nullptr);
}

// ===================================================== modules, lifecycle, events ===============================================

namespace {
// Captures ksp::log output
struct LogCapture {
    std::vector<std::pair<log::Level, std::string>> lines;
    LogCapture() {
        log::SetSink([this](log::Level l, std::string_view m) { lines.emplace_back(l, std::string(m)); });
    }
    ~LogCapture() { log::SetSink(nullptr); }
    bool Has(log::Level l, const std::string& needle) const {
        for (const auto& [lv, m] : lines)
            if (lv == l && m.find(needle) != std::string::npos) return true;
        return false;
    }
};

// A PartModule recording the lifecycle calls Part makes (registered over a stock class nobody else instantiates)
struct LifecycleModule : PartModule, IPartMassModifier {
    static inline std::vector<std::string>* trace = nullptr;
    using PartModule::PartModule;
    void T(const std::string& s) {
        if (trace) trace->push_back(s);
    }
    void OnStart(StartState s) override { T("OnStart:" + std::to_string(static_cast<int>(s))); }
    void OnStartFinished(StartState s) override { T("OnStartFinished:" + std::to_string(static_cast<int>(s))); }
    void OnStartBeforePartAttachJoint(StartState s) override { T("BeforeJoint:" + std::to_string(static_cast<int>(s))); }
    void OnInitialize() override { T("OnInitialize"); }
    void OnActive() override { T("OnActive"); }
    void OnInactive() override { T("OnInactive"); }
    void OnFixedUpdate() override { T("OnFixedUpdate"); }
    void OnUpdate() override {
        T("OnUpdate");
        if (throwInUpdate) throw std::runtime_error("boom");
    }
    void OnWillBeCopied(bool sym) override { T(std::string("WillBeCopied:") + (sym ? "sym" : "")); }
    void OnWasCopied(PartModule* copy, bool) override { T(std::string("WasCopied:") + (copy ? "module" : "null")); }
    void PromoteToPhysicalPart() override { T("Promote"); }
    void DemoteToPhysicslessPart() override { T("Demote"); }
    IPartMassModifier* AsMassModifier() override { return this; }
    float GetModuleMass(float, ModifierStagingSituation) override { return extraMass; }
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }
    float extraMass = 0.5f;
    bool throwInUpdate = false;
};
const char* kLifecycleClass = "ModuleKerbNetAccess";

struct LifecycleRegistration {
    LifecycleRegistration() {
        ModuleFactory::Register(kLifecycleClass, [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<LifecycleModule>(c)); });
    }
    ~LifecycleRegistration() {
        ModuleFactory::Register(kLifecycleClass, [](const ModuleClassInfo& c) { return std::make_shared<PartModule>(c); });
    }
};
}  // namespace

TEST_CASE("WP-13 Part modules: AddModule rules, LoadModule index handling, Find*, RemoveModule") {
    if (!GameAvailable()) return;
    Game13 g;
    LogCapture log;
    Part p;
    p.gameObjectName = "testPart";
    p.Awake();
    CHECK(p.AddModule("NoSuchModule") == nullptr);
    CHECK(log.Has(log::Level::Error, "Cannot find a PartModule of typename 'NoSuchModule'"));

    // ModuleCargoPart / ModuleInventoryPart exclusivity (Part.cs:5787): the cargo module is removed when its packedVolume >= 0
    REQUIRE(p.AddModule("ModuleCargoPart") != nullptr);
    CHECK(p.isCargoPart());
    REQUIRE(p.AddModule("ModuleInventoryPart") != nullptr);
    CHECK(p.Modules().Count() == 1);
    CHECK(p.ModuleData.size() == 1);
    CHECK(p.isInventoryPart());
    CHECK_FALSE(p.isCargoPart());
    CHECK(log.Has(log::Level::Warning, "Cannot have ModuleCargoPart and ModuleInventoryPart on same Part"));
    CHECK(p.AddModule("ModuleCargoPart") == nullptr);  // refused while an inventory module exists
    CHECK(log.Has(log::Level::Warning, "they are mutually exclusive"));
    Part q;
    q.Awake();
    PartModuleData* cargo = q.AddModule("ModuleCargoPart");
    cargo->module->Set<float>("packedVolume", -1.0f);
    REQUIRE(q.AddModule("ModuleInventoryPart") != nullptr);  // packedVolume < 0: the cargo module stays (a warning is still logged)
    CHECK(q.Modules().Count() == 2);

    // AddModule(node): the MODULE node is loaded through PartModule.Load
    Part r;
    r.Awake();
    auto cmd = ParseNode("MODULE\n{\n name = ModuleCommand\n minimumCrew = 2\n}\n");
    PartModuleData* rec = r.AddModule(*cmd);
    REQUIRE(rec != nullptr);
    CHECK(rec->module->Get<int32_t>("minimumCrew") == 2);
    CHECK(rec->moduleName == "ModuleCommand");
    CHECK(rec->FindCfgField("minimumCrew") != nullptr);
    auto noName = ParseNode("MODULE\n{\n isEnabled = True\n}\n");
    CHECK(r.AddModule(*noName) == nullptr);
    CHECK(log.Has(log::Level::Error, "contains no module name"));

    // LoadModule(node, ref moduleIndex): follows the cfg order, warns on a mismatch and searches the other indices
    r.AddModule("ModuleEngines");
    r.AddModule("ModuleEnginesFX");
    int index = 0;
    auto eng = ParseNode("MODULE\n{\n name = ModuleEngines\n exhaustDamageMultiplier = 77\n}\n");
    log.lines.clear();
    PartModule* m = r.LoadModule(*eng, index);  // index 0 is ModuleCommand: mismatch, found at 1
    REQUIRE(m != nullptr);
    CHECK(m == r.Modules().GetModule(1));
    CHECK(index == 1);
    CHECK(m->Get<double>("exhaustDamageMultiplier") == 77.0);
    CHECK(log.Has(log::Level::Warning, "PartModule indexing mismatch"));
    auto fx = ParseNode("MODULE\n{\n name = ModuleEnginesFX\n}\n");
    CHECK(r.LoadModule(*fx, index) == r.Modules().GetModule(2));  // index 1 is ModuleEngines -> mismatch -> found at 2
    CHECK(index == 2);
    auto missing = ParseNode("MODULE\n{\n name = ModuleLight\n}\n");
    CHECK(r.LoadModule(*missing, index) == nullptr);
    CHECK(log.Has(log::Level::Warning, "no ModuleLight module found on part definition"));
    index = 9;
    CHECK(r.LoadModule(*eng, index) == r.Modules().GetModule(1));  // index beyond the list: "index exceeds module count"
    CHECK(log.Has(log::Level::Warning, "index exceeds module count"));

    // type queries: `is T` over the class hierarchy (ModuleEnginesFX derives from ModuleEngines)
    CHECK(r.isEngine());
    CHECK(r.FindModulesImplementing([](PartModule& x) { return x.IsA("ModuleEngines"); }).size() == 2);
    CHECK(r.FindModuleOfClass("ModuleEnginesFX") == r.Modules().GetModule(2));
    CHECK(r.HasModuleImplementing([](PartModule& x) { return x.AsMassModifier() != nullptr; }) == false);
    CHECK_FALSE(r.isDecoupler());
    CHECK_FALSE(r.isAirIntake());
    Part child;
    child.Awake();
    child.parent = &r;
    CHECK(child.FindParentModuleImplementing([](PartModule& x) { return x.IsA("ModuleCommand"); }) == r.Modules().GetModule(0));
    CHECK(child.FindParentModuleImplementing([](PartModule& x) { return x.IsA("ModuleLight"); }) == nullptr);

    // RemoveModule: only a module of this part
    log.lines.clear();
    r.RemoveModule(p.Modules().GetModule(0));
    CHECK(log.Has(log::Level::Error, "Cannot destroy a module which does not belong to this part"));
    CHECK(r.Modules().Count() == 3);
    PartModule* fxm = r.Modules().GetModule(2);
    r.RemoveModule(fxm);
    CHECK(r.Modules().Count() == 2);
    CHECK(r.ModuleData.size() == 2);
    r.RemoveModules();
    CHECK(r.Modules().Count() == 0);
    CHECK(r.ModuleData.empty());
    // the Robotic family needs the Serenity expansion
    CHECK_FALSE(r.isRobotic());
}

TEST_CASE("WP-13 Part module lifecycle: Modules* calls, start states, mass update, copy callbacks, error isolation") {
    if (!GameAvailable()) return;
    Game13 g;
    LogCapture log;
    LifecycleRegistration reg;
    std::vector<std::string> trace;
    LifecycleModule::trace = &trace;
    auto part = g.New("fuelTank");
    PartModuleData* rec = part->AddModule(kLifecycleClass);
    REQUIRE(rec != nullptr);
    auto* lm = dynamic_cast<LifecycleModule*>(rec->module.get());
    REQUIRE(lm != nullptr);

    // GetModuleStartState (Part.cs:5569)
    Vessel* fakeVessel = reinterpret_cast<Vessel*>(part.get());  // only compared with nullptr
    CHECK(part->GetModuleStartState(true, false, std::nullopt) == StartState::Editor);
    CHECK(part->GetModuleStartState(false, false, std::nullopt) == StartState::None);
    CHECK(part->GetModuleStartState(false, true, VesselSituations::ORBITING) == StartState::None);  // no vessel yet
    part->vessel = fakeVessel;
    auto bits = [](StartState s) { return static_cast<int>(s); };
    CHECK(bits(part->GetModuleStartState(false, true, VesselSituations::PRELAUNCH)) == (bits(StartState::PreLaunch) | bits(StartState::Landed)));
    CHECK(part->GetModuleStartState(false, true, VesselSituations::LANDED) == StartState::Landed);
    CHECK(part->GetModuleStartState(false, true, VesselSituations::SPLASHED) == StartState::Splashed);
    CHECK(part->GetModuleStartState(false, true, VesselSituations::FLYING) == StartState::Flying);
    CHECK(part->GetModuleStartState(false, true, VesselSituations::SUB_ORBITAL) == StartState::SubOrbital);
    CHECK(part->GetModuleStartState(false, true, VesselSituations::ESCAPING) == StartState::Orbital);
    CHECK(part->GetModuleStartState(false, true, VesselSituations::DOCKED) == StartState::Docked);
    part->vessel = nullptr;

    // ModulesOnStart: OnStart with the state, mass = prefabMass + sum of mass modifiers (Part.cs:5615)
    const float prefabMass = g.Info("fuelTank").partPrefab->fields.mass;
    part->fields.mass = 7.0f;  // stale: ModulesOnStart takes the prefab's mass as prefabMass
    part->ModulesOnStart(StartState::Editor);
    CHECK(trace.back() == "OnStart:1");
    CHECK(part->fields.prefabMass == prefabMass);
    CHECK(part->fields.mass == f32store(fx(prefabMass) + fx(0.5f)));
    part->ModulesOnStartFinished(StartState::Editor);
    CHECK(trace.back() == "OnStartFinished:1");
    part->ModulesBeforePartAttachJoint(StartState::Editor);
    CHECK(trace.back() == "BeforeJoint:1");
    part->InitializeModules();
    CHECK(trace.back() == "OnInitialize");
    // ModulesOnFixedUpdate: only enabled modules run OnFixedUpdate, the mass modifier still counts
    lm->extraMass = 1.25f;
    lm->isEnabled = false;
    trace.clear();
    part->ModulesOnFixedUpdate();
    CHECK(trace.empty());
    CHECK(part->fields.mass == f32store(fx(prefabMass) + fx(1.25f)));
    lm->isEnabled = true;
    part->ModulesOnFixedUpdate();
    CHECK(trace == std::vector<std::string>{"OnFixedUpdate"});
    // minimum mass clamp
    lm->extraMass = -100.0f;
    part->ModulesOnFixedUpdate();
    CHECK(part->fields.mass == g.Info("fuelTank").MinimumMass());
    lm->extraMass = 0.5f;
    // staging: OnActive only for modules with stagingEnabled, OnInactive always
    trace.clear();
    part->ModulesOnActivate();
    part->ModulesOnDeactivate();
    CHECK(trace == (std::vector<std::string>{"OnActive", "OnInactive"}));
    lm->Set<bool>("stagingEnabled", false);
    trace.clear();
    part->ModulesOnActivate();
    CHECK(trace.empty());
    // a throwing module is logged and the others still run
    lm->throwInUpdate = true;
    log.lines.clear();
    part->ModulesOnUpdate();
    CHECK(log.Has(log::Level::Error, "Module ModuleKerbNetAccess threw during OnUpdate: boom"));
    // physics-significance promotion reaches the modules
    trace.clear();
    part->fields.physicalSignificance = static_cast<int>(Part_PhysicalSignificance::NONE);
    part->PromoteToPhysicalPart();
    CHECK(trace == std::vector<std::string>{"Promote"});
    part->parent = part.get();  // DemoteToPhysicslessPart needs a parent
    part->DemoteToPhysicslessPart();
    CHECK(trace.back() == "Demote");
    part->parent = nullptr;

    // copy callbacks (Part.cs:2102 OnWillBeCopied, :2115 OnWasCopied)
    auto copy = g.New("fuelTank");
    copy->AddModule(kLifecycleClass);
    trace.clear();
    part->OnWillBeCopied(true);
    part->OnWasCopied(copy.get(), false);
    CHECK(trace == (std::vector<std::string>{"WillBeCopied:sym", "WasCopied:module"}));
    // SendEvent reaches the module events of the part and, through the tree, of its neighbours (Part.cs:6207)
    int sent = 0;
    auto other = g.New("mk1pod.v2");
    Attach(*other, "bottom", *part, "top");
    BaseEvent* toggle = other->Modules().GetModule(0)->Events()["ToggleStaging"];
    REQUIRE(toggle != nullptr);
    toggle->onEvent = [&] { sent++; };
    part->SendEvent("ToggleStaging");  // the pod's module is one link away
    CHECK(sent >= 1);
    sent = 0;
    part->SendEvent("ToggleStaging", nullptr, 0);  // maxDepth 0: only this part's modules
    CHECK(sent == 0);
    LifecycleModule::trace = nullptr;
}

TEST_CASE("WP-13 Part resources, crew, state and the Start() data block") {
    if (!GameAvailable()) return;
    Game13 g;
    LogCapture log;
    CaptureTable res = Table("resources");
    const int rPart = res.Col("part"), rName = res.Col("name"), rAmount = res.Col("amount"), rDensity = res.Col("density");
    auto tank = g.New("fuelTank");
    // fuelTank holds LiquidFuel and Oxidizer (C9 resources table): mass = float(sum amount * density) in double
    auto expectedMass = [&](const std::map<std::string, double>& amounts) {
        double sum = 0.0;
        for (const auto& row : res.rows)
            if (row[rPart] == "fuelTank") sum += amounts.at(row[rName]) * static_cast<double>(static_cast<float>(row[rDensity].get<double>()));
        return static_cast<float>(sum);
    };
    std::map<std::string, double> amounts;
    for (const auto& row : res.rows)
        if (row[rPart] == "fuelTank") amounts[row[rName]] = row[rAmount].get<double>();
    REQUIRE(amounts.size() == 2);
    CHECK(Bits(tank->GetResourceMass()) == Bits(expectedMass(amounts)));
    // SetResource: Load into the existing resource; the simulation list is kept in step by AddResource only
    tank->SetResource(*ParseNode("RESOURCE\n{\n name = LiquidFuel\n amount = 100\n maxAmount = 180\n}\n"));
    amounts["LiquidFuel"] = 100.0;
    REQUIRE(tank->GetResource("LiquidFuel") != nullptr);
    CHECK(tank->GetResource("LiquidFuel")->amount == 100.0);
    CHECK(Bits(tank->GetResourceMass()) == Bits(expectedMass(amounts)));
    CHECK(tank->GetResourceMass(false) == tank->GetResourceMass());
    // simulation resources: a copy of the list; ResetSimulationResources re-reads amounts and flow settings
    tank->SetupSimulationResources();
    CHECK(tank->SimulationResources().size() == 2);
    CHECK(tank->SimulationResources()[0].simulationResource);
    tank->GetResource("Oxidizer")->amount = 50.0;
    amounts["Oxidizer"] = 50.0;
    CHECK(tank->GetResourceMass(true) != tank->GetResourceMass(false));
    tank->ResetSimulation();
    CHECK(Bits(tank->GetResourceMass(true)) == Bits(tank->GetResourceMass(false)));
    CHECK(Bits(tank->GetResourceMass(true)) == Bits(expectedMass(amounts)));
    // AddResource: duplicates and unknown definitions are refused with a warning (PartResourceList.Add)
    log.lines.clear();
    CHECK(tank->AddResource(*ParseNode("RESOURCE\n{\n name = Oxidizer\n amount = 1\n maxAmount = 1\n}\n")) == nullptr);
    CHECK(log.Has(log::Level::Warning, "Part already contains  resource 'Oxidizer'"));
    CHECK(tank->AddResource(*ParseNode("RESOURCE\n{\n name = Unobtainium\n amount = 1\n maxAmount = 1\n}\n")) == nullptr);
    CHECK(log.Has(log::Level::Warning, "Could not create PartResource of type 'Unobtainium"));
    PartResource* mono = tank->AddResource(*ParseNode("RESOURCE\n{\n name = MonoPropellant\n amount = 5\n maxAmount = 10\n flowMode = Out\n}\n"));
    REQUIRE(mono != nullptr);
    CHECK(mono->part == tank.get());
    CHECK(mono->flowMode == PartResourceFlowMode::Out);
    CHECK(mono->flowState);  // default true
    CHECK(tank->SimulationResources().size() == 3);  // AddResource adds to the simulation list too
    // RemoveResource by name, id and object; false when absent
    CHECK(tank->RemoveResource("MonoPropellant"));
    CHECK_FALSE(tank->RemoveResource("MonoPropellant"));
    CHECK(tank->SimulationResources().size() == 2);
    CHECK(tank->RemoveResource(MonoStringHash("Oxidizer")));
    PartResource* lf = tank->GetResource("LiquidFuel");
    CHECK(tank->RemoveResource(lf));
    CHECK(tank->Resources.empty());
    CHECK(tank->GetResourceMass() == 0.0f);

    // the thermal overloads: float accumulation (`(float)amount * density`, `thermal += mass * specificHeatCapacity`)
    {
        PartResourceLibrary lib;  // a library with a resource that has a specific heat capacity
        lib.resourceDefinitions.Add(*ParseNode("RESOURCE_DEFINITION\n{\n name = HotFluid\n density = 0.01\n hsp = 4\n unitCost = 1\n}\n"));
        Part p;
        p.Awake();
        PartResource* hot = p.AddResource(*ParseNode("RESOURCE\n{\n name = HotFluid\n amount = 100\n maxAmount = 100\n}\n"));
        REQUIRE(hot != nullptr);
        float thermalF = -1.0f;
        double thermalD = -1.0;
        const float m1 = p.GetResourceMass(thermalF);
        const float m2 = p.GetResourceMass(thermalD);
        const float num2 = f32store(fx(100.0f) * fx(0.01f));
        CHECK(Bits(m1) == Bits(num2));
        CHECK(Bits(m2) == Bits(num2));
        CHECK(Bits(thermalF) == Bits(f32store(fx(num2) * fx(4.0f))));
        CHECK(thermalD == static_cast<double>(num2) * 4.0);
        // Start(): resourceMass / thermalMass / thermalMassReciprocal; the part starts at 288 K when temperature is -1
        p.fields.mass = 0.5f;
        p.fields.thermalMassModifier = 2.0;
        Part::StartContext ctx;
        ctx.flight = true;
        ctx.standardSpecificHeatCapacity = 800.0;
        p.StartData(ctx);
        CHECK(Bits(p.fields.resourceMass) == Bits(num2));
        CHECK(p.fields.resourceThermalMass == thermalD);
        CHECK(p.fields.thermalMass == static_cast<double>(0.5f) * 800.0 * 2.0 + thermalD);
        CHECK(p.fields.thermalMassReciprocal == 1.0 / p.fields.thermalMass);
        CHECK(p.fields.temperature == 288.0);
        CHECK(p.fields.skinTemperature == 288.0);
        CHECK(p.fields.skinUnexposedTemperature == 288.0);
        CHECK(p.fields.started);
    }

    // Start(): skinMaxTemp / absorptiveConstant fallbacks, PhysicsSignificance -> physicalSignificance, editor flag
    auto s = g.New("liquidEngine");
    s->fields.skinMaxTemp = -1.0;
    s->fields.maxTemp = 2000.0;
    s->fields.absorptiveConstant = -1.0;
    s->fields.emissiveConstant = 0.4;
    s->fields.PhysicsSignificance = 1;
    Part::StartContext ectx;
    ectx.editor = true;
    s->StartData(ectx);
    CHECK(s->fields.skinMaxTemp == 2000.0);
    CHECK(s->fields.absorptiveConstant == 0.4);
    CHECK(s->fields.physicalSignificance == 1);
    CHECK(s->fields.previousPhysicalSignificance == 1);
    CHECK(s->fields.editorStarted);
    CHECK_FALSE(s->fields.started);
    auto s2 = g.New("liquidEngine");
    s2->fields.PhysicsSignificance = -1;  // the default: physicalSignificance is left alone
    s2->fields.temperature = -1.0;
    Part::StartContext fctx;
    fctx.flight = true;
    fctx.externalTemperature = 250.5;
    s2->StartData(fctx);
    CHECK(s2->fields.physicalSignificance == 0);
    CHECK(s2->fields.temperature == 250.5);  // (float) of the external temperature while it is still negative

    // crew: capacity, duplicates, status changes (Part.cs:2279 / 2260)
    auto pod = g.New("mk1pod.v2");
    REQUIRE(pod->fields.CrewCapacity == 1);
    auto k1 = std::make_shared<ProtoCrewMember>(Game_Modes::SANDBOX, *ParseNode("KERBAL\n{\n name = Jebediah Kerman\n}\n"));
    auto k2 = std::make_shared<ProtoCrewMember>(Game_Modes::SANDBOX, *ParseNode("KERBAL\n{\n name = Bill Kerman\n}\n"));
    log.lines.clear();
    CHECK(pod->AddCrewmember(k1));
    CHECK(k1->rosterStatus() == ProtoCrewMember::RosterStatus::Assigned);
    CHECK_FALSE(pod->AddCrewmember(k1));
    CHECK(log.Has(log::Level::Warning, "Crewmember is already in this part"));
    CHECK_FALSE(pod->AddCrewmember(k2));
    CHECK(log.Has(log::Level::Warning, "Part Crew Capacity Exceeded"));
    CHECK_FALSE(pod->AddCrewmemberAt(k2, 0));
    CHECK(log.Has(log::Level::Error, "Part Crew Capacity Exceeded"));
    pod->RemoveCrewmember(k1);
    CHECK(k1->rosterStatus() == ProtoCrewMember::RosterStatus::Available);
    CHECK(pod->protoModuleCrew.empty());
    pod->RemoveCrewmember(k1);
    CHECK(log.Has(log::Level::Warning, "Crewmember is not in this part"));
    CHECK(pod->AddCrewmemberAt(k2, 0));
    CHECK(k2->seatIdx == 0);

    // State / CanFail: FAILED needs Making History; the state before the failure is remembered (Part.cs:946)
    auto st = g.New("fuelTank");
    st->set_State(PartStates::ACTIVE);
    st->set_State(PartStates::FAILED);
    CHECK(st->State() == PartStates::ACTIVE);  // CanFail is false without the expansion
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", true);
    st->set_State(PartStates::FAILED);
    CHECK(st->State() == PartStates::FAILED);
    CHECK(st->fields.PreFailState == static_cast<int>(PartStates::IDLE));  // ACTIVE is remembered as IDLE
    st->set_State(PartStates::DEAD);
    st->set_State(PartStates::FAILED);
    CHECK(st->fields.PreFailState == static_cast<int>(PartStates::DEAD));
    st->set_CanFail(false);
    CHECK_FALSE(st->CanFail());
    CHECK(st->localRoot() == st.get());
    // shielding display strings follow the flag
    st->set_ShieldedFromAirstream(true);
    CHECK(st->ShieldedFromAirstream());
}

// ===================================================== variants and copy ========================================================

TEST_CASE("WP-13 PartVariant and ModulePartVariants: lists of the C9 variants table, selection, themes, ApplyVariant, mass and cost") {
    if (!GameAvailable()) return;
    Game13 g;
    LogCapture log;
    CaptureTable vt = Table("variants");
    const int vPart = vt.Col("part"), vIdx = vt.Col("idx"), vName = vt.Col("name"), vDisp = vt.Col("displayName"), vCost = vt.Col("cost"),
              vMass = vt.Col("mass");
    // every variant of every compiled part: names, order, cost and mass (the module owns the list since WP-13)
    std::map<std::string, std::vector<const nlohmann::json*>> byPart;
    for (const auto& row : vt.rows) byPart[row[vPart]].push_back(&row);
    CHECK(byPart.size() == 78);
    for (const auto& [name, rows] : byPart) {
        const AvailablePart* ap = g.data.parts.getPartInfoByName(name);
        if (ap == nullptr || ap->codeBuilt) continue;
        CAPTURE(name);
        REQUIRE(ap->partPrefab->variants != nullptr);
        ModulePartVariants& mv = *ap->partPrefab->variants;
        const auto* list = ap->Variants();
        REQUIRE(list != nullptr);
        REQUIRE(list->size() == rows.size());
        CHECK(&mv.variantList == list);
        for (size_t i = 0; i < rows.size(); i++) {
            const PartVariant& v = *(*list)[i];
            CHECK(v.name == (*rows[i])[vName].get<std::string>());
            CHECK(Bits(v.cost) == Bits(static_cast<float>((*rows[i])[vCost].get<double>())));
            CHECK(Bits(v.mass) == Bits(static_cast<float>((*rows[i])[vMass].get<double>())));
            CHECK(mv.GetVariantIndex(v.name) == static_cast<int>(i));
            CHECK(mv.HasVariant(v.name));
        }
        CHECK(ap->variant == ap->partPrefab->baseVariant);  // CompilePartInfo: newPartInfo.variant = part.baseVariant
        CHECK(mv.GetModuleTitle() == "PartVariants");
        CHECK(mv.GetVariantNames().size() == rows.size());
    }

    // the base variant: with `baseVariant = X` the list is NOT prefixed by the base and variantIndex points at X
    {
        const AvailablePart& lander = g.Info("mk2LanderCabin.v2");
        ModulePartVariants& mv = *lander.partPrefab->variants;
        CHECK(mv.variantList.size() == 2);
        CHECK(mv.variantList[0]->name == "Rover");
        CHECK(lander.partPrefab->baseVariant == mv.variantList[1]);  // the same object
        CHECK(lander.partPrefab->baseVariant->name == "Lander");
        CHECK(mv.GetCurrentVariantIndex() == 1);
        // `baseVariant = Full` names no variant: GetVariantIndex answers 0 (sic), so the first variant (Shroud) is the base
        const AvailablePart& skipper = g.Info("engineLargeSkipper.v2");
        ModulePartVariants& sv = *skipper.partPrefab->variants;
        CHECK(sv.variantList.size() == 3);
        CHECK(sv.variantList[0]->name == "Shroud");
        CHECK(sv.GetCurrentVariantIndex() == 0);
        CHECK(skipper.partPrefab->baseVariant == sv.variantList[0]);
        CHECK(sv.variantList[0]->attachNodes.size() == 2);
        CHECK(sv.variantList[0]->attachNodes[0].id == "top");
        CHECK(sv.variantList[0]->attachNodes[0].position.y == 1.013f);
        CHECK(sv.variantList[0]->srfAttachNode == skipper.partPrefab->srfAttachNode);  // sic: the srf node is shared, not cloned
        REQUIRE(sv.variantList[0]->attachRules);
        CHECK(sv.variantList[0]->attachRules->String() == skipper.partPrefab->fields.attachRules->String());
        CHECK_FALSE(sv.variantList[0]->infoGameObjects.empty());
        CHECK(sv.variantList[1]->GetExtraInfoValue("nothing").empty());
        CHECK(sv.variantList[2]->GetAttachedNodeIndex("top") == 0);
        CHECK(sv.variantList[2]->GetAttachedNodeIndex("bottom") == 1);
        CHECK(sv.variantList[2]->GetAttachedNodeIndex("nope") == -1);
        // without baseVariant the Basic base variant (a copy of the part's nodes) is inserted first; PartVariant.cs:213
        // new PartVariant(baseVariant) is not a memberwise copy, so the base itself has no GAMEOBJECTS
        const AvailablePart& dec = g.Info("Decoupler.1");
        ModulePartVariants& dv = *dec.partPrefab->variants;
        CHECK(dv.variantList[0]->name == "Basic");
        CHECK(dec.partPrefab->baseVariant == dv.variantList[0]);
        CHECK(dv.variantList[0]->infoGameObjects.empty());
        CHECK(dv.variantList[0]->attachNodes.size() == dec.partPrefab->attachNodes.size());
        CHECK_FALSE(dv.variantList[1]->textures.empty());  // the colour variant carries TEXTURE commands
        // the Bare variant's NODES replace the top node's position and size
        CHECK(sv.variantList[2]->attachNodes[0].position.y == 0.583f);
        CHECK(sv.variantList[2]->attachNodes[0].size == 2);
        const auto vis = sv.variantList[2]->ModelVisibility();
        CHECK(vis.size() == 5);
        CHECK(vis[0] == std::make_pair(std::string("COL1"), false));
    }

    // a clone: SetVariant, events, scene-dependent mass, cost
    {
        auto rcs = g.New("RCSBlock.v2");
        REQUIRE(rcs->variants != nullptr);
        ModulePartVariants& mv = *rcs->variants;
        int applied = 0;
        struct L {
            int* n;
            void On(Part*, PartVariant*) { (*n)++; }
        } l{&applied};
        GameEvents::onVariantApplied.Add(&l, &L::On);
        CHECK(mv.GetCurrentVariantIndex() == 0);  // private KSPField: a clone restarts at 0
        CHECK_FALSE(mv.SetVariant("NoSuchVariant"));
        CHECK(log.Has(log::Level::Error, "Variant name does not exist to be set. Part:"));
        CHECK(mv.SetVariant("3Horn"));
        CHECK(mv.GetCurrentVariantIndex() == 3);  // [Angled, 5Horn, 4Horn, 3Horn, 2Horn]
        CHECK(applied == 1);
        CHECK(Bits(mv.GetModuleCost(0.0f, ModifierStagingSituation::CURRENT)) == Bits(-8.0f));
        CHECK(mv.GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == 0.0f);  // useVariantMass is off outside the editor
        CHECK(Bits(rcs->GetModuleCosts(0.0f)) == Bits(-8.0f));
        SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
        CHECK(Bits(mv.GetModuleMass(0.0f, ModifierStagingSituation::CURRENT)) == Bits(-0.01f));
        CHECK(mv.Get<bool>("useVariantMass"));  // HighLogic.LoadedSceneIsEditor sets it, persistently
        SaveEnvironment::Current().loadedScene = GameScenes::MAINMENU;
        CHECK(Bits(mv.GetModuleMass(0.0f, ModifierStagingSituation::CURRENT)) == Bits(-0.01f));
        // OnSave writes selectedVariant for index > 0; OnLoad restores it by name
        ConfigNode saved("MODULE");
        mv.OnSave(saved);
        CHECK(saved.GetValue("selectedVariant") == "3Horn");
        mv.Set<int32_t>("variantIndex", 0);
        ConfigNode none("MODULE");
        mv.OnSave(none);
        CHECK_FALSE(none.HasValue("selectedVariant"));
        mv.OnLoad(saved);
        CHECK(mv.GetCurrentVariantIndex() == 3);
        // module info / drag cube names
        CHECK(mv.GetDragCubeNames() == (std::vector<std::string>{"0", "1", "2", "3", "4"}));
        mv.AssumeDragCubePosition("2");
        CHECK(mv.GetCurrentVariantIndex() == 2);
        mv.AssumeDragCubePosition("x");
        CHECK(mv.GetCurrentVariantIndex() == 2);
        CHECK_FALSE(mv.GetInfo().empty());
        GameEvents::onVariantApplied.Remove(&l, &L::On);
    }

    // themes: largeAdapter's variants carry themeName White / Orange
    {
        auto adapter = g.New("largeAdapter");
        ModulePartVariants& mv = *adapter->variants;
        REQUIRE(mv.HasVariantTheme("Orange"));
        CHECK(mv.SetVariantTheme("Orange"));
        CHECK(mv.variantList[mv.GetCurrentVariantIndex()]->themeName == std::optional<std::string>("Orange"));
        CHECK_FALSE(mv.SetVariantTheme("Magenta"));
        CHECK(log.Has(log::Level::Error, "Variant theme does not exist to be set"));
    }

    // ApplyVariant in the editor scene: the part's nodes take the variant's, the part follows its parent (Part.cs / ModulePartVariants.cs:48)
    {
        SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
        auto tank = g.New("fuelTank");
        auto engine = g.New("engineLargeSkipper.v2");
        Attach(*tank, "bottom", *engine, "top");
        engine->partTransform.position = Vector3f(0, 10, 0);
        ModulePartVariants& mv = *engine->variants;
        REQUIRE(mv.SetVariant("Bare"));
        AttachNode* top = engine->FindAttachNode("top");
        CHECK(top->position.y == 0.583f);
        CHECK(top->originalPosition.y == 0.583f);
        CHECK(top->size == 2);
        // UpdatePartPosition: the attached parent is the owner's potentialParent, so the OWNER moves by current - new
        const float delta = f32store(fx(1.013f) - fx(0.583f));
        CHECK(Bits(engine->partTransform.position.y) == Bits(f32store(10.0 + fx(delta))));
        // the part's attachRules follow the variant (null -> baseVariant.AttachRules)
        REQUIRE(engine->fields.attachRules);
        CHECK(engine->fields.attachRules->String() == mv.variantList[2]->attachRules.value_or(*engine->baseVariant->attachRules).String());
        // the Shroud variant restores the base nodes
        REQUIRE(mv.SetVariant("Shroud"));
        CHECK(engine->FindAttachNode("top")->position.y == 1.013f);
        SaveEnvironment::Current().loadedScene = GameScenes::MAINMENU;
    }

    // disabledEvents toggle the module event (FlagDecal.ToggleFlag) while the variant that lists it is applied
    {
        auto lander = g.New("mk2LanderCabin.v2");
        ModulePartVariants& mv = *lander->variants;
        PartModule* flag = lander->Modules()["FlagDecal"];
        REQUIRE(flag != nullptr);
        BaseEvent* ev = flag->Events()["ToggleFlag"];
        REQUIRE(ev != nullptr);
        CHECK_FALSE(ev->EventIsDisabledByVariant());
        REQUIRE(mv.SetVariant("Lander"));
        CHECK(ev->EventIsDisabledByVariant());
        REQUIRE(mv.SetVariant("Rover"));
        CHECK_FALSE(ev->EventIsDisabledByVariant());
    }
}

TEST_CASE("WP-13 Part copy semantics: Instantiate, OnWillBeCopied / OnCopy / OnWasCopied over a part tree") {
    if (!GameAvailable()) return;
    Game13 g;
    // original: pod -> tank; the tank is half empty, has no oxidizer, a tweaked module and a selected variant on the pod's neighbour
    auto pod = g.New("mk1pod.v2");
    auto tank = g.New("fuelTank");
    Attach(*pod, "bottom", *tank, "top");
    tank->SetResource(*ParseNode("RESOURCE\n{\n name = LiquidFuel\n amount = 33\n maxAmount = 180\n}\n"));
    REQUIRE(tank->RemoveResource("Oxidizer"));
    PartModule* tmod = tank->Modules().GetModule(0);
    tmod->Set<bool>("stagingEnabled", false);
    // the clone path ShipConstruction takes: Instantiate the prefab (or the part) for each part, rebuild the links, OnCopy the roots
    auto pod2 = g.New("mk1pod.v2");
    auto tank2 = g.New("fuelTank");
    Attach(*pod2, "bottom", *tank2, "top");
    CHECK(tank2->Resources.size() == 2);                              // the prefab's resources
    CHECK(tank2->Modules().GetModule(0)->Get<bool>("stagingEnabled"));
    pod->OnWillBeCopied(false);
    pod2->OnCopy(pod.get(), false);
    CHECK(pod2->fields.isClone);
    CHECK(pod2->originalPart == pod.get());
    CHECK(tank2->fields.isClone);
    CHECK(tank2->originalPart == tank.get());
    // resources follow the original: Oxidizer is removed, LiquidFuel takes the amounts, the simulation list is reset
    REQUIRE(tank2->Resources.size() == 1);
    CHECK(tank2->Resources[0].resourceName == "LiquidFuel");
    CHECK(tank2->Resources[0].amount == 33.0);
    CHECK(tank2->Resources[0].maxAmount == 180.0);
    REQUIRE(tank2->SimulationResources().size() == 1);
    CHECK(tank2->SimulationResources()[0].amount == 33.0);
    // module fields are copied (PartModule.Copy -> BaseField.CopyField over every field)
    CHECK_FALSE(tank2->Modules().GetModule(0)->Get<bool>("stagingEnabled"));
    // a resource the original has and the clone lacks is added (with a simulation twin)
    tank->AddResource(*ParseNode("RESOURCE\n{\n name = MonoPropellant\n amount = 4\n maxAmount = 8\n}\n"));
    tank2->OnCopy(tank.get(), false);
    CHECK(tank2->Resources.size() == 2);
    CHECK(tank2->GetResource("MonoPropellant")->amount == 4.0);
    CHECK(tank2->SimulationResources().size() == 2);
    // OnWasCopied hands each module its counterpart in the new part
    struct Probe : PartModule {
        using PartModule::PartModule;
        PartModule* got = nullptr;
        bool called = false;
        void OnWasCopied(PartModule* c, bool) override {
            called = true;
            got = c;
        }
    };
    auto probe = std::make_shared<Probe>(*GetPartModuleClass("ModuleLight"));
    probe->part = pod.get();
    probe->Awake();
    pod->Modules().Add(probe);
    pod2->AddModule("ModuleLight");
    // the probe sits at the end of pod's list; pod2's list is one shorter at that index -> index == Count reads past the end
    // in C# (`newPart.modules.Count >= i`), kspcore indexes with the same guard: the call is made once the lists line up
    pod2->AddModule("ModuleLight");
    pod->OnWasCopied(pod2.get(), false);
    CHECK(probe->called);
    CHECK(probe->got != nullptr);
    CHECK(probe->got->ClassName() == "ModuleLight");

    // variants: OnCopy selects the original's variant by name (Part.cs:2138)
    auto rcs = g.New("RCSBlock.v2");
    auto rcsCopy = g.New("RCSBlock.v2");
    REQUIRE(rcs->variants->SetVariant("2Horn"));
    CHECK(rcsCopy->variants->GetCurrentVariantIndex() == 0);
    rcsCopy->OnCopy(rcs.get(), true);
    CHECK(rcsCopy->variants->GetCurrentVariantIndex() == rcs->variants->GetCurrentVariantIndex());
    CHECK(rcsCopy->variants->SelectedVariant()->name == "2Horn");
    // the clone's variant list is its own deep copy (Unity serializes [Serializable] instances by value)
    CHECK(rcsCopy->variants->variantList[0].get() != rcs->variants->variantList[0].get());
    rcsCopy->variants->variantList[0]->cost = 99.0f;
    CHECK(rcs->variants->variantList[0]->cost == 0.0f);
    CHECK(rcsCopy->baseVariant.get() != rcs->baseVariant.get());

    // Instantiate keeps the data independent: changing the clone's node or resource leaves the original alone
    auto copyA = g.New("fuelTank");
    auto copyB = Part::Instantiate(*copyA);
    copyB->attachNodes[0].position.x = 123.0f;
    copyB->Resources[0].amount = 1.0;
    CHECK(copyA->attachNodes[0].position.x != 123.0f);
    CHECK(copyA->Resources[0].amount != 1.0);
    CHECK(copyB->attachNodes[0].owner == copyB.get());
    CHECK(copyB->partInfo == copyA->partInfo);
}

TEST_CASE("WP-13 structural modules: ModuleMirrorPlacement, ModuleDisplaceTweak, CompoundPartModule, AxisGroupsManager") {
    if (!GameAvailable()) return;
    Game13 g;
    LogCapture log;
    // ModuleMirrorPlacement.OnWillBeMirrored (ModuleMirrorPlacement.cs:26): X and Z by default, Y optional
    {
        Part p;
        p.Awake();
        PartModule* mm = p.AddModule("ModuleMirrorPlacement")->module.get();
        Quaternionf r = Quaternionf::identity();
        const Quaternionf expected = Quaternionf::identity() * UnityNative::AngleAxis(180.0f, Vector3f::right()) * UnityNative::AngleAxis(180.0f, Vector3f::forward());
        CHECK(p.OnWillBeMirrored(r, nullptr, nullptr));
        CHECK(r.Equals(expected));
        mm->Set<bool>("applyMirrorRotationXAxis", false);
        mm->Set<bool>("applyMirrorRotationZAxis", false);
        mm->Set<bool>("applyMirrorRotationYAxis", true);
        mm->Set<float>("mirrorRotationYAxis", 90.0f);
        Quaternionf r2 = Quaternionf::identity();
        CHECK(p.OnWillBeMirrored(r2, nullptr, nullptr));
        CHECK(r2.Equals(Quaternionf::identity() * UnityNative::AngleAxis(90.0f, Vector3f::up())));
        // a surface-attached part with a node pointing along X is left alone unless ignoreMirrorIfSurfaceXOffset is on (sic: the
        // test is inverted -- with the flag OFF the early return happens)
        AttachNode srf;
        srf.nodeType = AttachNodeType::Surface;
        srf.orientation = Vector3f(1, 0, 0);
        Quaternionf r3 = Quaternionf::identity();
        mm->Set<bool>("ignoreMirrorIfSurfaceXOffset", false);
        CHECK_FALSE(p.OnWillBeMirrored(r3, &srf, nullptr));
        CHECK(r3.Equals(Quaternionf::identity()));
        mm->Set<bool>("ignoreMirrorIfSurfaceXOffset", true);
        CHECK(p.OnWillBeMirrored(r3, &srf, nullptr));
        Part none;
        none.Awake();
        Quaternionf r4 = Quaternionf::identity();
        CHECK_FALSE(none.OnWillBeMirrored(r4, nullptr, nullptr));  // no module answers true
    }
    // ModuleDisplaceTweak (ModuleDisplaceTweak.cs): chained displacements of one transform
    {
        Part p, sym;
        p.Awake();
        sym.Awake();
        PartModule* a = p.AddModule("ModuleDisplaceTweak")->module.get();
        PartModule* b = p.AddModule("ModuleDisplaceTweak")->module.get();
        PartModule* sb = sym.AddModule("ModuleDisplaceTweak")->module.get();
        sym.AddModule("ModuleDisplaceTweak");
        p.symmetryCounterparts = {&sym};
        auto* da = dynamic_cast<ModuleDisplaceTweak*>(a);
        auto* db = dynamic_cast<ModuleDisplaceTweak*>(b);
        REQUIRE(da != nullptr);
        REQUIRE(db != nullptr);
        std::vector<Vector3f> written;
        for (auto* d : {da, db}) {
            d->findModelTransform = [](const std::string&) { return std::make_optional(Vector3f(1, 0, 0)); };
            d->setLocalPosition = [&](const Vector3f& v) { written.push_back(v); };
            d->Set<Vector3f>("axis", Vector3f(0, 1, 0));
            d->Set<float>("displaceMax", 2.0f);
            d->Set<float>("displaceMin", -1.0f);
        }
        da->OnStart(StartState::Editor);
        db->OnStart(StartState::Editor);
        CHECK(da->MDprev == nullptr);
        CHECK(da->MDnext == db);
        CHECK(db->MDprev == da);
        CHECK(db->MDnext == nullptr);
        // scalar 0.5: Lerp((0,-1,0), (0,2,0), 0.5) = (0,0.5,0); b adds a's final position
        CHECK(da->GetFinalPosition().Equals(Vector3f(1, 0.5f, 0)));
        CHECK(db->GetFinalPosition().Equals(Vector3f(1, 1.0f, 0)));
        da->UpdateMorph(0.0f);  // a has a successor: nothing is written
        CHECK(written.empty());
        CHECK(da->Get<float>("scalar") == 0.0f);
        db->UpdateMorph(1.0f);  // the last module writes the chain's final position
        REQUIRE(written.size() == 1);
        CHECK(written[0].Equals(Vector3f(1, -1.0f + 2.0f, 0)));  // a: (1,-1,0), b: + (0,2,0)
        // PartTweaked on a symmetry counterpart re-applies that counterpart's scalar (by module index) to this chain
        auto* ds = dynamic_cast<ModuleDisplaceTweak*>(sb);
        ds->Set<float>("scalar", 0.25f);
        written.clear();
        GameEvents::onEditorPartEvent.Fire(static_cast<ConstructionEventType>(14), &sym);
        // mIndex of `db` is 1 -> sym.Modules[1] (the second sym module, default scalar 0.5), b writes the final position
        CHECK(written.size() >= 1);
        // an unrelated part's PartTweaked event is ignored
        written.clear();
        Part stranger;
        GameEvents::onEditorPartEvent.Fire(static_cast<ConstructionEventType>(14), &stranger);
        CHECK(written.empty());
    }
    GameEvents::onEditorPartEvent.Fire(static_cast<ConstructionEventType>(0), nullptr);  // listeners of destroyed modules are gone
    // CompoundPartModule.OnAwake (CompoundPartModule.cs:23): needs a CompoundPart
    {
        struct TestCompound : CompoundPartModule {
            using CompoundPartModule::CompoundPartModule;
            bool awoke = false;
            void OnModuleAwake() override { awoke = true; }
            void OnTargetSet(Part*) override {}
            void OnTargetLost() override {}
        };
        Part comp;
        comp.className = "CompoundPart";
        auto m = std::make_shared<TestCompound>(*GetPartModuleClass("ModuleLight"));
        m->part = &comp;
        m->Awake();
        CHECK(m->compoundPart == &comp);
        CHECK(m->awoke);
        Part plain;
        auto m2 = std::make_shared<TestCompound>(*GetPartModuleClass("ModuleLight"));
        m2->part = &plain;
        log.lines.clear();
        m2->Awake();
        CHECK(m2->compoundPart == nullptr);
        CHECK(log.Has(log::Level::Error, "CompoundPartModule requires a CompoundPart component!"));
        CHECK(m2->awoke);
        comp.className = "CompoundPart";
        m->compoundTarget = [&](const Part&) { return &plain; };
        CHECK(m->target() == &plain);
    }
    // AxisGroupsManager: AXISGROUPS only for modules with a KSPAxisField (AxisGroupsManager.cs:36), loaded back (:19)
    {
        Part p;
        p.Awake();
        PartModule* ax = p.AddModule("AxisTestModule")->module.get();
        BaseAxisField* field = dynamic_cast<BaseAxisField*>(ax->Fields()["TestField"]);
        REQUIRE(field != nullptr);
        AxisGroupsManager::BuildBaseAxisFields(*ax);  // idempotent: the field is already a BaseAxisField
        CHECK(dynamic_cast<BaseAxisField*>(ax->Fields()["TestField"]) == field);
        ConfigNode saved("MODULE");
        AxisGroupsManager::SaveAxisFieldNodes(*ax, saved);
        ConfigNode* groups = saved.GetNode("AXISGROUPS");
        REQUIRE(groups != nullptr);
        REQUIRE(groups->nodes().Count() == 1);
        CHECK(groups->nodes().Shared(0)->name == "TestField");
        CHECK(groups->nodes().Shared(0)->HasValue("axisGroup"));
        ConfigNode load("MODULE");
        ConfigNode* lg = load.AddNode("AXISGROUPS");
        ConfigNode* lf = lg->AddNode("TestField");
        lf->AddValue("axisGroup", "Pitch");
        lf->AddValue("axisSpeedMultiplier", 2.5f);
        AxisGroupsManager::LoadAxisFieldNodes(*ax, load);
        CHECK(field->axisGroup == static_cast<int64_t>(KSPAxisGroup::Pitch));
        CHECK(field->incrementalSpeedMultiplier == 2.5f);
        // a module without axis fields writes nothing
        PartModule* plain = p.AddModule("ModuleCommand")->module.get();
        ConfigNode none("MODULE");
        AxisGroupsManager::SaveAxisFieldNodes(*plain, none);
        CHECK_FALSE(none.HasNode("AXISGROUPS"));
        // PartModule.Save/Load use the manager
        ConfigNode full("MODULE");
        ax->Save(full);
        CHECK(full.HasNode("AXISGROUPS"));
    }
}

// ============================================ proto snapshots of the physprofile save <-> live parts =============================

namespace {
const std::string kSaves13 = std::string(KSPCORE_TESTDATA_DIR) + "/saves";

// value-by-value compare of two nodes' values in order, returning the names that differ
std::vector<std::string> DiffValues(const ConfigNode& a, const ConfigNode& b) {
    std::vector<std::string> d;
    const int n = std::max(a.values().Count(), b.values().Count());
    for (int i = 0; i < n; i++) {
        if (i >= a.values().Count() || i >= b.values().Count()) {
            d.push_back("<count>");
            break;
        }
        if (a.values()[i].name != b.values()[i].name || a.values()[i].value != b.values()[i].value) d.push_back(a.values()[i].name);
    }
    return d;
}
}  // namespace

TEST_CASE("WP-13 physprofile save: ProtoVessel.LoadObjects builds the part tree, ProtoPartSnapshot(Part) writes it back") {
    if (!GameAvailable()) return;
    Game13 g;
    GameParameters::GetDefaultParameters(Game_Modes::CAREER, GameParameters::Preset::Normal);
    ConfigNode::NodePtr root = ConfigNode::Load(kSaves13 + "/physprofile/persistent.sfs");
    REQUIRE(root);
    Game game(root.get());
    REQUIRE(game.flightState);
    LogCapture log;
    size_t vessels = 0, partsTotal = 0;
    std::map<std::string, size_t> keyDiffs;
    size_t modulesTotal = 0, modulesEqual = 0, resourcesTotal = 0, resourcesEqual = 0;
    for (auto& pvp : game.flightState->protoVessels) {
        ProtoVessel& pv = *pvp;
        if (pv.protoPartSnapshots.empty()) continue;
        vessels++;
        CAPTURE(pv.vesselName);
        log.lines.clear();
        ProtoVessel::LoadedObjects lo = pv.LoadObjects(nullptr);
        REQUIRE(lo.parts.size() == pv.protoPartSnapshots.size());
        REQUIRE(lo.rootPart != nullptr);
        CHECK(lo.rootPart == lo.parts[static_cast<size_t>(pv.rootIndex)].get());
        std::vector<Part*> parts;
        for (auto& p : lo.parts) parts.push_back(p.get());
        partsTotal += parts.size();
        // the tree: parent / children / symmetry exactly as the snapshots index them
        std::vector<std::vector<Part*>> expectedChildren(parts.size());
        for (size_t i = 0; i < parts.size(); i++) {
            const ProtoPartSnapshot& s = *pv.protoPartSnapshots[i];
            if (static_cast<int>(i) == pv.rootIndex) {
                CHECK(parts[i]->parent == nullptr);
            } else {
                REQUIRE(parts[i]->parent == parts.at(static_cast<size_t>(s.parentIdx)));
                expectedChildren[static_cast<size_t>(s.parentIdx)].push_back(parts[i]);
            }
            REQUIRE(parts[i]->symmetryCounterparts.size() == s.symLinkIdxs.size());
            for (size_t k = 0; k < s.symLinkIdxs.size(); k++) CHECK(parts[i]->symmetryCounterparts[k] == parts[static_cast<size_t>(s.symLinkIdxs[k])]);
            CHECK(parts[i]->protoPartSnapshot == &s);
            CHECK(parts[i]->fields.craftID == s.craftID);
            CHECK(parts[i]->fields.persistentId == s.persistentId);
            CHECK(parts[i]->partInfo == s.partInfo);
        }
        for (size_t i = 0; i < parts.size(); i++) CHECK(parts[i]->children == expectedChildren[i]);
        // attach nodes: attN / srfN resolved to parts; a child is reachable through the node pair
        for (size_t i = 0; i < parts.size(); i++) {
            Part& p = *parts[i];
            if (p.parent != nullptr) {
                // STACK: a node pair; SRF_ATTACH: the child's srfAttachNode points at the parent (the parent has no node for it)
                if (p.fields.attachMode == static_cast<int>(AttachModes::SRF_ATTACH)) {
                    CHECK(p.srfAttachNode->attachedPart == p.parent);
                } else {
                    CHECK(p.FindAttachNodeByPart(p.parent) != nullptr);
                    CHECK(p.parent->FindAttachNodeByPart(&p) != nullptr);
                }
            }
            for (const AttachNode& n : p.attachNodes) {
                if (n.attachedPart == nullptr) continue;
                CHECK(n.attachedPart->FindAttachNodeByPart(&p) != nullptr);  // stack nodes are mutual
                CHECK(n.attachedPart->hasIndirectParent(&p) == (n.attachedPart->parent != nullptr));  // (sic) quirk of hasIndirectParent
            }
        }
        // snapshot -> part fields, resources, modules (no module of the snapshot was left unmatched)
        CHECK_FALSE(log.Has(log::Level::Warning, "module found on part definition"));
        CHECK_FALSE(log.Has(log::Level::Warning, "indexing mismatch"));
        for (size_t i = 0; i < parts.size(); i++) {
            const ProtoPartSnapshot& s = *pv.protoPartSnapshots[i];
            const Part& p = *parts[i];
            CHECK(Bits(p.fields.mass) == Bits(s.mass));
            CHECK(p.State() == PartStates::IDLE);
            CHECK(p.fields.ResumeState == s.state);
            CHECK(p.Resources.size() >= s.resources.size());
            for (const auto& r : s.resources) {
                const PartResource* pr = const_cast<Part&>(p).GetResource(*r->resourceName);
                REQUIRE(pr != nullptr);
                CHECK(pr->amount == r->amount);
                CHECK(pr->maxAmount == r->maxAmount);
                CHECK(pr->flowState == r->flowState);
            }
            if (!s.refTransformName.empty()) CHECK(p.GetReferenceTransformName() == s.refTransformName);  // asteroid vessels store none
        }
        // Part -> snapshots: a fresh ProtoVessel shell takes the live parts and writes them as PART nodes
        ProtoVessel shell(*ConfigNode::Parse("VESSEL\n{\nname = shell\nORBIT\n{\nREF = 1\n}\nACTIONGROUPS\n{\n}\n}\n")->GetNode("VESSEL"), &game);
        shell.protoPartSnapshots.clear();
        for (Part* p : parts) shell.protoPartSnapshots.push_back(std::make_unique<ProtoPartSnapshot>(*p, &shell));
        for (auto& s : shell.protoPartSnapshots) s->storePartRefs();
        for (size_t i = 0; i < parts.size(); i++) {
            ConfigNode original("PART"), rebuilt("PART");
            pv.protoPartSnapshots[i]->Save(original);
            shell.protoPartSnapshots[i]->Save(rebuilt);
            // the identity and structure of the part is reproduced from the live Part. The asteroid vessel's part is written by
            // ProtoVessel.CreateVesselNode, not from a live part: it stores srfN "None, -1", no rTrf and no MODULE nodes
            const bool handBuilt = original.GetNodes("MODULE").empty();
            for (const char* key : {"name", "cid", "uid", "mid", "persistentId", "launchID", "parent", "position", "rotation", "mirror", "symMethod",
                                    "istg", "resPri", "dstg", "sqor", "sepI", "sidx", "attm", "sameVesselCollision", "srfN", "mass", "temp", "tempExt",
                                    "tempExtUnexp", "staticPressureAtm", "expt", "state", "attached", "autostrutMode", "rigidAttachment", "flag", "rTrf",
                                    "modCost", "modMass", "moduleVariantName", "moduleCargoStackableQuantity"}) {
                CAPTURE(std::string(key));
                CAPTURE(i);
                if (handBuilt && (std::string(key) == "srfN" || std::string(key) == "rTrf" || std::string(key) == "modMass")) continue;
                CHECK(rebuilt.GetValue(key) == original.GetValue(key));
            }
            CHECK(rebuilt.GetValues("sym") == original.GetValues("sym"));
            CHECK(rebuilt.GetValues("attN") == original.GetValues("attN"));
            CHECK(rebuilt.GetValues("crew") == original.GetValues("crew"));
            // RESOURCE nodes, in order
            CHECK(rebuilt.GetNodes("RESOURCE").size() == original.GetNodes("RESOURCE").size());
            for (size_t k = 0; k < original.GetNodes("RESOURCE").size() && k < rebuilt.GetNodes("RESOURCE").size(); k++) {
                resourcesTotal++;
                if (DiffValues(*original.GetNodes("RESOURCE")[k], *rebuilt.GetNodes("RESOURCE")[k]).empty()) resourcesEqual++;
            }
            // MODULE nodes: the live part writes every module it has (a snapshot may carry fewer: asteroids store none); the
            // module host reproduces Save for fields, module OnSave logic belongs to the owning packages
            CHECK(rebuilt.GetNodes("MODULE").size() == parts[i]->Modules().Count());
            CHECK(rebuilt.GetNodes("MODULE").size() >= original.GetNodes("MODULE").size());
            for (size_t k = 0; k < original.GetNodes("MODULE").size() && k < rebuilt.GetNodes("MODULE").size(); k++) {
                modulesTotal++;
                if (original.GetNodes("MODULE")[k]->ToString() == rebuilt.GetNodes("MODULE")[k]->ToString()) modulesEqual++;
            }
            for (const std::string& d : DiffValues(original, rebuilt)) keyDiffs[d]++;
        }
        shell.protoPartSnapshots.clear();
    }
    MESSAGE("physprofile: " << vessels << " vessels, " << partsTotal << " parts; modules equal " << modulesEqual << "/" << modulesTotal
                            << "; resources equal " << resourcesEqual << "/" << resourcesTotal);
    for (const auto& [k, n] : keyDiffs) MESSAGE("  PART value differs: " << k << " x" << n);
    CHECK(vessels == 2);
    CHECK(partsTotal == 458);              // the 457-part PhysProf-L2-AS and the asteroid it is next to
    CHECK(modulesEqual == modulesTotal);   // MODULE nodes after PartModule.Load -> Save are byte-identical to the game's
    CHECK(resourcesEqual == resourcesTotal);
    CHECK(modulesTotal == 1337);
    CHECK(resourcesTotal == 623);
}

// ============================================== part trees of the original crafts (harness/inputs/Ships) =========================

namespace {
// ShipConstruct.LoadShip's part creation (ShipConstruct.cs:378-640) over WP-04's craft form: Instantiate the prefab, assign the
// values of the PART node, load MODULE / RESOURCE / PARTDATA nodes, then link parent / children / symmetry / attach nodes.
// ShipConstruct itself (editor integration, staging, delta-v) is WP-18; this is the part-side of it.
struct CraftParts {
    std::vector<std::unique_ptr<Part>> parts;
    bool ok = false;
    std::string missing;
};

CraftParts BuildCraftParts(Game13& g, const ShipConstruct& ship) {
    CraftParts out;
    for (const auto& d : ship.parts) {
        const AvailablePart* ap = g.data.parts.getPartInfoByName(d.partInfoName);
        if (ap == nullptr || !ap->partPrefab) {
            out.missing = d.partInfoName;
            return out;
        }
    }
    for (const auto& d : ship.parts) {
        const AvailablePart* ap = g.data.parts.getPartInfoByName(d.partInfoName);
        auto part = Part::Instantiate(*ap->partPrefab);
        part->gameObjectName = d.partInfoName;
        part->fields.craftID = d.craftID;
        part->partInfo = ap;
        part->fields.symMethod = static_cast<int>(d.symMethod);
        part->fields.persistentId = d.persistentId;
        part->partTransform.position = d.position;
        part->partTransform.rotation = d.rotation;
        part->fields.attPos = d.attPos;
        part->fields.attPos0 = d.attPos0;
        part->fields.attRotation = d.attRotation;
        part->fields.attRotation0 = d.attRotation0;
        part->SetMirror(d.mirrorVector);
        part->fields.autoStrutMode = static_cast<int>(d.autoStrutMode);
        part->fields.rigidAttachment = d.rigidAttachment;
        part->fields.inverseStage = d.inverseStage;
        part->fields.resourcePriorityOffset = d.resourcePriorityOffset;
        part->fields.defaultInverseStage = d.defaultInverseStage;
        part->fields.inStageIndex = d.inStageIndex;
        part->fields.manualStageOffset = d.manualStageOffset;
        part->fields.separationIndex = d.separationIndex;
        part->fields.attachMode = d.attachMode;
        part->fields.sameVesselCollision = d.sameVesselCollision;
        part->fields.customPartData = d.customPartData;
        if (d.srfAttachNodeInFile) {
            AttachNode& n = *part->srfAttachNode;
            n.attachedPartId = d.srfAttachNode.attachedPartId;
            n.srfAttachMeshName = d.srfAttachNode.srfAttachMeshName.value_or(std::string());
            if (d.srfAttachNode.originalOrientationKnown) {
                n.originalPosition = n.position = d.srfAttachNode.position;
                n.orientation = n.originalOrientation = d.srfAttachNode.orientation;
            }
        }
        for (const auto& a : d.attachNodes) {
            AttachNode* n = part->FindAttachNode(a.id);
            if (n == nullptr) continue;
            n->attachedPartId = a.attachedPartId;
            if (a.positionKnown) n->position = a.position;
            if (a.orientationKnown) n->orientation = a.orientation;
        }
        int moduleIndex = 0;
        for (const auto& m : d.modules) part->LoadModule(*m, moduleIndex);
        for (const auto& r : d.resources) part->SetResource(*r);
        if (d.partData) part->OnLoad(*d.partData);
        out.parts.push_back(std::move(part));
    }
    for (size_t n = 0; n < out.parts.size(); n++) {
        Part* part2 = out.parts[n].get();
        for (int c : ship.parts[n].children) out.parts[static_cast<size_t>(c)]->setParent(part2);
        for (int s : ship.parts[n].symmetryCounterparts) part2->symmetryCounterparts.push_back(s >= 0 ? out.parts[static_cast<size_t>(s)].get() : nullptr);
        std::vector<Part*> all;
        for (auto& p : out.parts) all.push_back(p.get());
        part2->srfAttachNode->owner = part2;
        part2->srfAttachNode->FindAttachedPart(all);
        for (AttachNode& an : part2->attachNodes) {
            an.owner = part2;
            an.FindAttachedPart(all);
        }
    }
    for (auto& p : out.parts) p->InitializeModules();
    out.ok = true;
    return out;
}
}  // namespace

TEST_CASE("WP-13 craft trees: every stock-part craft of harness/inputs/Ships becomes a consistent Part tree") {
    if (!GameAvailable()) return;
    Game13 g;
    LogCapture log;
    size_t crafts = 0, skippedModded = 0, partsTotal = 0, symLinks = 0, srfParts = 0;
    size_t modCostChecked = 0, modCostEqual = 0, modMassChecked = 0, modMassEqual = 0, craftsWithForeignModules = 0, loadFailed = 0;
    std::map<std::string, size_t> costMismatchByClass, massMismatchByClass, oneSided;
    for (const char* dir : {"VAB", "SPH"}) {
        const fs::path d = fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / dir;
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(d))
            if (e.path().extension() == ".craft") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const fs::path& f : files) {
            ConfigNode::NodePtr root = ConfigNode::Load(f.string());
            REQUIRE(root);
            ShipConstruct ship;
            std::string err;
            if (!ship.LoadShip(*root, 0, true, err)) {  // a part missing from the stock database (mods), or a too-old format
                loadFailed++;
                continue;
            }
            CAPTURE(f.filename().string());
            log.lines.clear();
            CraftParts cp = BuildCraftParts(g, ship);
            // MODULE nodes of mods (FAR, ...) match no module class of the stock part: their mass / cost modifiers are missing
            const bool foreignModules = log.Has(log::Level::Warning, "module found on part definition");
            if (foreignModules) craftsWithForeignModules++;
            if (!cp.ok) {
                skippedModded++;  // the craft uses a modded part (not in the stock GameData)
                continue;
            }
            crafts++;
            partsTotal += cp.parts.size();
            REQUIRE(cp.parts.size() == ship.parts.size());
            size_t roots = 0;
            std::vector<Part*> all;
            for (auto& p : cp.parts) all.push_back(p.get());
            for (size_t i = 0; i < cp.parts.size(); i++) {
                Part& p = *cp.parts[i];
                const ShipConstruct::PartData& d = ship.parts[i];
                if (p.parent == nullptr) roots++;
                else CHECK(p.parent == cp.parts[static_cast<size_t>(d.parent)].get());
                REQUIRE(p.children.size() == d.children.size());
                for (size_t k = 0; k < d.children.size(); k++) CHECK(p.children[k] == cp.parts[static_cast<size_t>(d.children[k])].get());
                symLinks += p.symmetryCounterparts.size();
                // attN / srfN resolve to parts of the craft; a stack node is mutual, a surface child points at its parent
                // (CompoundParts -- fuel lines, struts -- attach through CompoundPart.target, not through attach nodes)
                const bool compound = p.className == "CompoundPart" || (p.parent && p.parent->className == "CompoundPart");
                if (p.parent != nullptr && !compound) {
                    if (p.fields.attachMode == static_cast<int>(AttachModes::SRF_ATTACH)) {
                        srfParts++;
                        CHECK(p.srfAttachNode->attachedPart == p.parent);
                    } else {
                        // (a v1.6.0 craft stores the pair one-sided, e.g. a nose cone's attN to a truss that has no node for it)
                        if (p.FindAttachNodeByPart(p.parent) == nullptr || p.parent->FindAttachNodeByPart(&p) == nullptr)
                            oneSided[f.filename().string()]++;
                    }
                }
                for (const AttachNode& n : p.attachNodes)
                    if (n.attachedPart != nullptr && n.attachedPart->className != "CompoundPart" && !compound) {
                        if (n.attachedPart->FindAttachNodeByPart(&p) == nullptr) oneSided[f.filename().string()]++;
                    }
                // the game-written modCost / modMass are GetModuleCosts / GetModuleMass(partInfo.cost) of the live part
                if (d.modCost && !foreignModules) {
                    modCostChecked++;
                    if (Bits(p.GetModuleCosts(p.partInfo->cost)) == Bits(*d.modCost)) modCostEqual++;
                    else costMismatchByClass[d.partInfoName]++;
                }
                if (d.modMass && !foreignModules) {
                    modMassChecked++;
                    if (Bits(p.GetModuleMass(p.partInfo->cost)) == Bits(*d.modMass)) modMassEqual++;
                    else massMismatchByClass[d.partInfoName]++;
                }
                // resources of the file are on the part with their amounts
                for (const auto& r : d.resources) {
                    PartResource* pr = p.GetResource(*r->GetValue("name"));
                    REQUIRE(pr != nullptr);
                    CHECK(pr->amount == dotnet::ParseDouble(*r->GetValue("amount")));
                }
            }
            CHECK(roots == 1);
            CHECK(cp.parts[0]->parent == nullptr);  // the first PART is the root
            // hasIndirectChild from the root reaches every part
            for (Part* p : all) CHECK(all[0]->hasIndirectChild(p));
        }
    }
    MESSAGE("crafts built: " << crafts << " (" << partsTotal << " parts, " << symLinks << " symmetry links, " << srfParts
                             << " surface-attached parts); LoadShip refused (modded parts / old format): " << loadFailed
                             << "; crafts with modded MODULE nodes: " << craftsWithForeignModules);
    MESSAGE("modCost equal " << modCostEqual << "/" << modCostChecked << ", modMass equal " << modMassEqual << "/" << modMassChecked);
    for (const auto& [k, n] : costMismatchByClass) MESSAGE("  modCost differs: " << k << " x" << n);
    for (const auto& [k, n] : massMismatchByClass) MESSAGE("  modMass differs: " << k << " x" << n);
    for (const auto& [k, n] : oneSided) MESSAGE("  one-sided attach node pairs in " << k << ": " << n);
    CHECK(crafts == 33);
    CHECK(partsTotal == 1967);
    CHECK(loadFailed == 53);
    CHECK(craftsWithForeignModules == 18);
    // every attach node pair is mutual except those of the one old craft (3 pairs seen from both sides)
    CHECK(oneSided.size() == 1);
    CHECK(oneSided.count("Rocket-power VTOL.craft") == 1);
    // the game-written modCost / modMass equal the live GetModuleCosts / GetModuleMass(partInfo.cost) except for the two
    // fairings (their mass and cost come from built fairing panels, ModuleProceduralFairing state not in the prefab)
    CHECK(modCostChecked == 1063);
    CHECK(modCostEqual == 1061);
    CHECK(modMassEqual == 1061);
    CHECK(costMismatchByClass.size() == 2);
    CHECK(costMismatchByClass.count("fairingSize1") == 1);
    CHECK(costMismatchByClass.count("fairingSize2") == 1);
    CHECK(massMismatchByClass == costMismatchByClass);
}

TEST_CASE("WP-13 Part activation, detach and delete state machine") {
    if (!GameAvailable()) return;
    Game13 g;
    LogCapture log;
    LifecycleRegistration reg;
    std::vector<std::string> trace;
    LifecycleModule::trace = &trace;
    struct TestPart : Part {
        bool allow = true;
        int detached = 0, deleted = 0;
        bool onPartActivate() override { return allow; }
        void onPartDetach() override { detached++; }
        void onPartDelete() override { deleted++; }
    };
    TestPart p;
    p.gameObjectName = "t";
    p.Awake();
    p.fields.inverseStage = 2;
    p.AddModule(kLifecycleClass);
    Vessel* v = reinterpret_cast<Vessel*>(&p);
    // activate(stage, vessel): the stage must match; a part of another vessel needs ActivatesEvenIfDisconnected
    CHECK_FALSE(p.activate(1, nullptr));
    CHECK(p.State() == PartStates::IDLE);
    p.vessel = v;
    p.fields.ActivatesEvenIfDisconnected = false;
    CHECK_FALSE(p.activate(2, nullptr));  // another vessel is active
    CHECK(p.activate(2, v));
    CHECK(p.State() == PartStates::ACTIVE);
    CHECK(trace.back() == "OnActive");
    p.deactivate();
    CHECK(p.State() == PartStates::DEACTIVATED);
    CHECK(trace.back() == "OnInactive");
    // a refusing part falls back to IDLE and says so
    p.allow = false;
    CHECK_FALSE(p.activate(2, v));
    CHECK(p.State() == PartStates::IDLE);
    p.force_activate();
    CHECK(p.State() == PartStates::IDLE);
    CHECK(log.Has(log::Level::Info, "Did not activate"));
    p.allow = true;
    p.force_activate(false);
    CHECK(p.State() == PartStates::ACTIVE);
    p.set_State(PartStates::DEAD);
    CHECK_FALSE(p.activate(2, v));  // a dead part never activates
    // onDetach walks the children and clears `connected`; OnDelete forgets the persistent id
    TestPart child;
    p.addChild(&child);
    int editorDetach = 0;
    struct Counter {
        int* n;
        void Inc() { (*n)++; }
    } c{&editorDetach};
    child.OnEditorDetach = Delegate<void()>::Bind(&c, &Counter::Inc);
    p.onDetach();
    CHECK(p.detached == 1);
    CHECK(child.detached == 1);
    CHECK(editorDetach == 1);
    SaveEnvironment::Current().persistentIds.persistentLoadedPartIds.Add(777, &p);
    p.fields.persistentId = 777;
    p.OnDelete();
    CHECK(p.deleted == 1);
    CHECK_FALSE(SaveEnvironment::Current().persistentIds.persistentLoadedPartIds.Contains(777));
    LifecycleModule::trace = nullptr;
}
