// WP-26 unit tests: ModuleProceduralFairing + FairingPanel / FairingXSection, ModuleCargoBay, ModuleJettison, LaunchClamp,
// ModuleStructuralNode, Part.AddShield. The numbers that exist in a capture are gated by `capture.ship_construct` (the fairing's
// panelMass of craft_ComSat_Lx at 0 ULP); everything else is derived from the decompiled control flow (the derivation is in the comment
// above each check) or taken from the game's own data: the part cfgs of the GameData mirror and the XSECTION values the original
// ComSat Lx.craft stores. No number is invented.
#include <cmath>
#include <fstream>
#include <map>

#include "nlohmann/json.hpp"

#include "resource_fixture.hpp"

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/modules/module_procedural_fairing.hpp"
#include "ksp/modules/module_structural.hpp"
#include "ksp/parts/compound_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/save/save_environment.hpp"

using namespace ksp;
using namespace ksp::rtest;
using ksp::ProceduralFairings::FairingPanel;
using ksp::ProceduralFairings::FairingXSection;

namespace {

const std::string kHarness26 = KSPCORE_HARNESS_DIR;

// The first MODULE node named `module` of the PART `partPrefix*` of a craft file (the original game's own ship data)
struct CraftModule {
    ConfigNode::NodePtr root;
    const ConfigNode* node = nullptr;
};
CraftModule FindCraftModule(const std::string& craft, const std::string& partPrefix, const std::string& module) {
    CraftModule r;
    r.root = ConfigNode::Load(kHarness26 + "/inputs/Ships/VAB/" + craft + ".craft");
    REQUIRE(r.root != nullptr);
    for (ConfigNode* p : r.root->GetNodes("PART")) {
        const std::string* name = p->GetValuePtr("part");
        if (name == nullptr || name->rfind(partPrefix, 0) != 0) continue;
        for (ConfigNode* m : p->GetNodes("MODULE"))
            if (const std::string* n = m->GetValuePtr("name"))
                if (*n == module) {
                    r.node = m;
                    return r;
                }
    }
    FAIL("module not found in " << craft);
    return r;
}

// The C5 capture's `module_mass` of the fairing part of craft_ComSat_Lx (the editor's ModuleProceduralFairing.GetModuleMass)
float CapturedFairingModuleMass() {
    std::ifstream f(kHarness26 + "/captures/out2/deltav_readout.json", std::ios::binary);
    REQUIRE(f.good());
    const nlohmann::json j = nlohmann::json::parse(f);
    for (const auto& t : j.at("tables")) {
        if (t.at("name") != "parts") continue;
        int cCraft = -1, cIdx = -1, cMod = -1;
        for (size_t i = 0; i < t.at("columns").size(); i++) {
            const std::string n = t.at("columns")[i].at("name");
            if (n == "craft_id") cCraft = static_cast<int>(i);
            if (n == "part_index") cIdx = static_cast<int>(i);
            if (n == "module_mass") cMod = static_cast<int>(i);
        }
        REQUIRE(cCraft >= 0);
        REQUIRE(cMod >= 0);
        for (const auto& row : t.at("rows"))
            if (row[cCraft] == "craft_ComSat_Lx" && row[cIdx] == 10) return row[cMod].get<float>();
    }
    FAIL("row not found");
    return 0.0f;
}

// A fairingSize1 with the ComSat Lx craft's XSECTION data, started like the editor starts it
struct Fairing {
    Game15 g;
    CraftModule cm;
    std::unique_ptr<Part> part;
    ModuleProceduralFairing* m = nullptr;
    explicit Fairing(GameScenes scene = GameScenes::EDITOR) {
        SaveEnvironment::Current().loadedScene = scene;
        cm = FindCraftModule("ComSat Lx", "fairingSize1", "ModuleProceduralFairing");
        part = g.New("fairingSize1");
        m = dynamic_cast<ModuleProceduralFairing*>(part->FindModuleOfClass("ModuleProceduralFairing"));
        REQUIRE(m != nullptr);
        m->Load(*cm.node);  // PartModule.Load: the persistent fields (nArcs, ejectionForce, ...), then OnLoad (XSECTION, fsm)
    }
};

}  // namespace

// ModuleProceduralFairing.cs:1468 OnLoad: XSECTION nodes -> FairingXSection.Load, List.Sort by h, the last one isLast
TEST_CASE("fairing: OnLoad reads the XSECTION nodes of the original craft, sorted by height, last section flagged") {
    Fairing f;
    const std::vector<ConfigNode*> nodes = f.cm.node->GetNodes("XSECTION");
    REQUIRE(f.m->xSections.size() == nodes.size());
    REQUIRE(f.m->xSections.size() >= 2);
    for (size_t i = 0; i < nodes.size(); i++) {
        // float.Parse of the saved text (the file lists them in ascending height already)
        CHECK(f.m->xSections[i]->h == dotnet::ParseSingle(*nodes[i]->GetValuePtr("h")));
        CHECK(f.m->xSections[i]->r == dotnet::ParseSingle(*nodes[i]->GetValuePtr("r")));
        CHECK(f.m->xSections[i]->isLast == (i + 1 == nodes.size()));
        if (i > 0) CHECK(f.m->xSections[i - 1]->CompareTo(*f.m->xSections[i]) < 0);
    }
    CHECK(f.m->FsmStateName() == "st_idle");  // `fsm = st_idle` in the craft's module node
}

// FairingXSection.cs:276 float.CompareTo (NaN sorts first, equal 0)
TEST_CASE("fairing: FairingXSection.CompareTo is float.CompareTo") {
    FairingXSection a, b;
    a.h = 1.0f;
    b.h = 2.0f;
    CHECK(a.CompareTo(b) == -1);
    CHECK(b.CompareTo(a) == 1);
    CHECK(a.CompareTo(a) == 0);
    a.h = std::nanf("");
    CHECK(a.CompareTo(b) == -1);
    b.h = std::nanf("");
    CHECK(a.CompareTo(b) == 0);
}

// FairingXSection.cs:138 Load / :183 Save: h and r round-trip, ATTACHEDFLAG nodes carry panelIndex and the part ids
TEST_CASE("fairing: FairingXSection Load / Save round trip including ATTACHEDFLAG") {
    ConfigNode n("XSECTION");
    n.AddValue("h", "1.1307373");
    n.AddValue("r", "0.519284248");
    ConfigNode* fl = n.AddNode("ATTACHEDFLAG");
    fl->AddValue("attachedFlagPartIDs", "7,9,bad,11");  // uint.TryParse skips the bad entry
    fl->AddValue("panelIndex", "3");
    FairingXSection x;
    x.Load(n);
    CHECK(x.h == 1.1307373f);
    CHECK(x.r == 0.519284248f);
    REQUIRE(x.fairingPanelFlags.size() == 1);
    CHECK(x.fairingPanelFlags[0].panelIndex == 3);
    CHECK(x.fairingPanelFlags[0].attachedFlagsPartIds == std::vector<uint32_t>{7, 9, 11});
    CHECK(x.GetPanelIndexFromFlag(9, 0) == 3);
    CHECK(x.GetPanelIndexFromFlag(5, 11) == 3);  // placementID also matches
    CHECK(x.GetPanelIndexFromFlag(5, 6) == -1);
    ConfigNode out("XSECTION");
    x.Save(out);
    CHECK(*out.GetValuePtr("h") == "1.1307373");
    CHECK(*out.GetValuePtr("r") == "0.519284248");
    ConfigNode* sf = out.GetNode("ATTACHEDFLAG");
    REQUIRE(sf != nullptr);
    CHECK(*sf->GetValuePtr("attachedFlagPartIDs") == "7,9,11");
    CHECK(*sf->GetValuePtr("panelIndex") == "3");
    x.RemoveAttachedFlag(9);
    CHECK(x.fairingPanelFlags[0].attachedFlagsPartIds == std::vector<uint32_t>{7, 11});
}

// ModuleProceduralFairing.cs:330 OnStart -> :1366 SpawnMeshes -> FairingPanel.SetupPanelArray (:152): nArcs panels per section pair, then
// the cap panels when the last section's radius equals capRadius
TEST_CASE("fairing: OnStart builds nArcs panels per section pair plus the nose cap, linked bottom to top") {
    Fairing f;
    f.part->ModulesOnStart(StartState::Editor);
    const int nArcs = Mathf::RoundToInt(f.m->nArcs());
    CHECK(nArcs == 2);  // `nArcs = 2` in the craft
    const int sections = static_cast<int>(f.m->xSections.size());
    REQUIRE(f.m->xSections.back()->r == f.m->Get<float>("capRadius"));  // 0.200000003 == 0.2f: the fairing is capped
    REQUIRE(static_cast<int>(f.m->Panels.size()) == nArcs * (sections - 1) + nArcs);
    for (size_t i = 0; i < f.m->Panels.size(); i++) CHECK(f.m->Panels[i]->panelIndex == static_cast<int>(i));
    for (size_t i = static_cast<size_t>(nArcs * (sections - 1)); i < f.m->Panels.size(); i++) {
        CHECK(f.m->Panels[i]->isCap);
        CHECK(f.m->Panels[i]->ArcCount() == 2);  // {last section, cap section}
    }
    CHECK(f.m->Panels[0]->ArcCount() == 4);  // {bottom, lerp1, lerp2, top}
    // nCollidersPerArc = Max(1, RoundToInt(nCollidersPerXSection / nArcs)) = Max(1, 6 / 2)
    CHECK(f.m->nCollidersPerArc == 3);
    // the cap section (SetupPanelArray :178): r = 0, h = last.h + last.r * Tan(slope angle) * noseTip
    const FairingPanel& cap = *f.m->Panels.back();
    const auto& last = *f.m->xSections.back();
    const auto& prev = *f.m->xSections[f.m->xSections.size() - 2];
    const double slope = std::atan((double(last.h) - double(prev.h)) / (double(prev.r) - double(last.r)));
    const double capH = double(last.h) + double(last.r) * std::tan(slope) * double(f.m->Get<float>("noseTip"));
    CHECK(double(cap.Arc(1).xSection->h) == doctest::Approx(capH).epsilon(1e-6));
    CHECK(cap.Arc(1).xSection->r == 0.0f);
}

// FairingPanel.cs:624 GetArea: pi * (r0 + r1) * sqrt((r0 - r1)^2 + dh^2) * ((arcTo - arcFrom) / 360), in float32 per Mono store points
TEST_CASE("fairing: panel areas follow the lateral-surface-of-a-frustum formula and sum to GetFairingArea") {
    Fairing f;
    f.part->ModulesOnStart(StartState::Editor);
    const int nArcs = Mathf::RoundToInt(f.m->nArcs());
    double sum = 0.0;
    for (const auto& pn : f.m->Panels) {
        const double r0 = pn->Arc(0).xSection->r, r1 = pn->Arc(pn->ArcCount() - 1).xSection->r;
        const double dh = double(pn->Arc(pn->ArcCount() - 1).xSection->h) - double(pn->Arc(0).xSection->h);
        const double expect = 3.14159265358979323846 * (r0 + r1) * std::sqrt((r0 - r1) * (r0 - r1) + dh * dh) / double(nArcs);
        CHECK(double(pn->GetArea()) == doctest::Approx(expect).epsilon(1e-5));
        sum += expect;
    }
    CHECK(double(f.m->GetFairingArea()) == doctest::Approx(sum).epsilon(1e-5));
}

// ModuleProceduralFairing.cs:443 + :1663 + the C5 capture: the module mass of the original editor's fairingSize1 (0 ULP); :1682 cost
TEST_CASE("fairing: panelMass = GetFairingArea * UnitAreaMass equals the C5 capture's module mass; cost and STAGED rules") {
    Fairing f;
    f.part->ModulesOnStart(StartState::Editor);
    const float unit = f.m->Get<float>("UnitAreaMass");  // 0.01 in fairingSize1.cfg
    CHECK(unit == 0.01f);
    CHECK(f.m->PanelMass() == f32store(fx(f.m->GetFairingArea()) * fx(unit)));
    CHECK(f.m->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == CapturedFairingModuleMass());  // bit-exact
    CHECK(f.m->GetModuleMass(0.0f, ModifierStagingSituation::UNSTAGED) == CapturedFairingModuleMass());
    CHECK(f.m->GetModuleMass(0.0f, ModifierStagingSituation::STAGED) == 0.0f);  // :1665
    CHECK(f.m->GetModuleMassChangeWhen() == ModifierChangeWhen::STAGED);
    CHECK(f.m->GetModuleCost(0.0f, ModifierStagingSituation::CURRENT) == f32store(fx(f.m->GetFairingArea()) * fx(f.m->Get<float>("UnitAreaCost"))));
    CHECK(f.m->GetModuleCost(0.0f, ModifierStagingSituation::STAGED) == 0.0f);
    CHECK(f.m->GetModuleSize(Vector3f::one(), ModifierStagingSituation::CURRENT) == Vector3f::zero());
    // Part.GetModuleMass sums it: the part's modifier total is the module's mass
    CHECK(PartGetModuleMass(f.part->Modules(), f.part->partInfo->partPrefab->fields.mass) == CapturedFairingModuleMass());
}

// :1663: before any panel exists (a prefab, or after the jettison) panelMass is 0: Mathf.Clamp(0, Mathf.Epsilon, Mathf.Abs(0)) = Mathf.Epsilon
TEST_CASE("fairing: an unbuilt fairing has module mass Mathf.Epsilon") {
    Fairing f;
    CHECK(f.m->PanelMass() == 0.0f);
    CHECK(f.m->GetFairingArea() == 0.0f);
    CHECK(f.m->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == Mathf::Epsilon);
}

// FairingPanel.cs:274 BuildMesh / :432 SetArcPoints: every outer point of arc k lies on the circle of radius r_k about the axis (the inner at
// r_k - wallThickness 0.1); the arcs sweep arcFrom..arcTo; point counts and indices follow pCount / SetPanelMeshPoints; the triangle index
// count is 6 per quad as TriangulatePanelMesh adds them
TEST_CASE("fairing: BuildMesh vertices lie on the section circles; triangle and index counts follow TriangulatePanelMesh") {
    Fairing f;
    f.part->ModulesOnStart(StartState::Editor);
    const FairingPanel& p = *f.m->Panels[0];  // bottom panel of the first section pair (arcs 0..180 degrees)
    const int n = p.nSides();
    CHECK(n == 15);  // (int)(30 * (180 / 360)) with nSides = 30
    REQUIRE(p.ArcCount() == 4);
    const Vector3f pivot = f.m->pivot();
    for (size_t k = 0; k < p.ArcCount(); k++) {
        const auto& arc = p.Arc(k);
        REQUIRE(static_cast<int>(arc.outer.size()) == n);
        REQUIRE(static_cast<int>(arc.inner.size()) == n);
        for (int i = 0; i < n; i++) {
            const Vector3f o = arc.outer[static_cast<size_t>(i)]->vert - pivot;
            const Vector3f in = arc.inner[static_cast<size_t>(i)]->vert - pivot;
            // axis = up: the height is y, the radius is the xz distance (the lerp arcs carry the slid section values, tolerance of float32)
            if (k == 0 || k == p.ArcCount() - 1) {
                CHECK(std::hypot(double(o.x), double(o.z)) == doctest::Approx(double(arc.xSection->r)).epsilon(1e-5));
                CHECK(std::hypot(double(in.x), double(in.z)) == doctest::Approx(double(arc.xSection->r) - 0.1).epsilon(1e-4));
                CHECK(double(o.y) == doctest::Approx(double(arc.xSection->h)).epsilon(1e-5));
            }
        }
    }
    // pCount = 4 arcs * (n outer + n inner); every point of pts has its own index 0..pCount-1 (SetPanelMeshPoints)
    CHECK(static_cast<int>(p.Pts().size()) == 0);  // pts is released by CleanUp at the end of SpawnMeshes (:1417)
    CHECK(p.mesh.vertices.size() == static_cast<size_t>(4 * 2 * n));
    CHECK(p.mesh.normals.size() == p.mesh.vertices.size());
    CHECK(p.mesh.uv.size() == p.mesh.vertices.size());
    // quads of the three TriangulatePanelMesh calls (i = 1, 2, 3): per call 2 (n - 1) + 2, plus (n - 1) for the bottom cap (i == 1) and
    // the top cap (i == 3); 6 indices each
    const size_t quads = 3 * (2 * (static_cast<size_t>(n) - 1) + 2) + 2 * (static_cast<size_t>(n) - 1);
    CHECK(p.mesh.triangles.size() == 6 * quads);
    for (int t : p.mesh.triangles) CHECK((t >= 0 && t < static_cast<int>(p.mesh.vertices.size())));
    // the panel's own offset: AssignMesh subtracts the barycentre; vertices + pivotOffset average back to it
    Vector3d sum;
    for (const Vector3f& v : p.mesh.vertices) sum = sum + Vector3d(v.x, v.y, v.z);
    sum = sum / static_cast<double>(p.mesh.vertices.size());
    CHECK(sum.x == doctest::Approx(0.0).epsilon(1e-4).scale(1.0));
    CHECK(sum.y == doctest::Approx(0.0).epsilon(1e-4).scale(1.0));
    CHECK(sum.z == doctest::Approx(0.0).epsilon(1e-4).scale(1.0));
}

// FairingPanel.cs:531 TriangulateConePanel / :565 TriangulateTriangleMesh: the cap panel's fan (n outer points to the single apex)
TEST_CASE("fairing: the cap panel is a triangle fan to its apex") {
    Fairing f;
    f.part->ModulesOnStart(StartState::Editor);
    const FairingPanel& cap = *f.m->Panels.back();
    const int n = cap.nSides();
    CHECK(cap.Arc(1).outer.size() == 1);  // the cap section has one point per side
    // TriangulateTriangleMesh x2: (n - 1) triangles each (outer, inner), capBottom: (n - 1) quads, 2 side quads; 3 resp. 6 indices
    const size_t idx = 2 * 3 * (static_cast<size_t>(n) - 1) + 6 * (static_cast<size_t>(n) - 1) + 2 * 6;
    CHECK(cap.mesh.triangles.size() == idx);
    // the apex (SetNoseConePoints :522): pivot + axis * radius with the cap section's own r = 0, so on the axis at the pivot height
    const Vector3f apex = cap.Arc(1).outer[0]->vert;
    CHECK(apex.x == f.m->pivot().x);
    CHECK(apex.z == f.m->pivot().z);
}

// ModuleProceduralFairing.cs:1494 OnSave / :1468 OnLoad: fsm and the XSECTION nodes round-trip; Save writes `h` / `r` like float.ToString("R")
TEST_CASE("fairing: OnSave writes fsm and one XSECTION node per section") {
    Fairing f;
    f.part->ModulesOnStart(StartState::Editor);
    ConfigNode out("MODULE");
    f.m->OnSave(out);
    CHECK(*out.GetValuePtr("fsm") == "st_idle");
    const auto xs = out.GetNodes("XSECTION");
    const auto orig = f.cm.node->GetNodes("XSECTION");
    REQUIRE(xs.size() == orig.size());
    for (size_t i = 0; i < xs.size(); i++) {
        CHECK(dotnet::ParseSingle(*xs[i]->GetValuePtr("h")) == dotnet::ParseSingle(*orig[i]->GetValuePtr("h")));
        CHECK(dotnet::ParseSingle(*xs[i]->GetValuePtr("r")) == dotnet::ParseSingle(*orig[i]->GetValuePtr("r")));
    }
}

// ModuleProceduralFairing.cs:1558 DeployFairingAction -> :1103 on_Deploy -> :1765 JettisonPanels (tail :1860-1864): the panels and x-sections are
// cleared, panelMass becomes GetFairingArea() * UnitAreaMass = 0, the part's modifier falls back to Mathf.Epsilon; events fire; OnStop(1)
TEST_CASE("fairing: DeployFairing in flight clears the panels, zeroes panelMass and fires the deploy events") {
    Fairing f(GameScenes::FLIGHT);
    f.m->OnStart(StartState::Landed);  // only the fairing's own OnStart (no vessel exists in this test)
    CHECK(f.m->FsmStateName() == "st_idle");
    CHECK(f.m->CanMove());
    CHECK(f.m->GetScalar() == 0.0f);
    CHECK(f.m->PanelMass() > 0.0f);
    struct Spy {
        int deployed = 0, moving = 0, stopped = 0;
        float stopAt = -1.0f;
        void OnDeployed(Part*) { deployed++; }
        void OnMoving(float, float) { moving++; }
        void OnStop(float v) {
            stopped++;
            stopAt = v;
        }
    } spy;
    GameEvents::onFairingsDeployed.Add(&spy, &Spy::OnDeployed);
    f.m->OnMoving.Add(&spy, &Spy::OnMoving);
    f.m->OnStop.Add(&spy, &Spy::OnStop);
    int jettisoned = 0;
    ModuleProceduralFairing::hooks.makePanelPhysical = [&](Part&, FairingPanel&, float unit) {
        jettisoned++;
        CHECK(unit == 0.01f);
    };
    const size_t panels = f.m->Panels.size();
    f.m->DeployFairing();
    ModuleProceduralFairing::hooks = {};
    GameEvents::onFairingsDeployed.Remove(&spy, &Spy::OnDeployed);
    CHECK(f.m->FsmStateName() == "st_flight_deployed");
    CHECK(static_cast<size_t>(jettisoned) == panels);  // every panel became a physical object
    CHECK(f.m->Panels.empty());
    CHECK(f.m->xSections.empty());
    CHECK(f.m->PanelMass() == 0.0f);
    CHECK(f.m->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == Mathf::Epsilon);
    CHECK(spy.deployed == 1);
    CHECK(spy.moving == 1);
    CHECK(spy.stopped == 1);
    CHECK(spy.stopAt == 1.0f);
    CHECK(f.m->GetScalar() == 1.0f);
    CHECK_FALSE(f.m->CanMove());
    // a second DeployFairingAction while deployed only re-runs JettisonPanels (:1566): nothing left to do
    KSPActionParam param;
    f.m->DeployFairingAction(param);
    CHECK(f.m->Panels.empty());
}

// ModuleProceduralFairing.cs:2157 CheckForFairingEdit (editor): editing is blocked while a flag part is attached to a panel
TEST_CASE("fairing: CheckForFairingEdit blocks editing while flags are attached") {
    Fairing f;
    f.part->ModulesOnStart(StartState::Editor);
    CHECK(f.m->Events()["EditFairing"]->guiActiveEditor);
    CHECK(f.m->Fields()["nArcs"]->guiActiveEditor);
    CHECK_FALSE(f.m->Fields()["editingBlocked"]->guiActiveEditor);
    f.m->xSections[0]->AddNewFairingPanel(0);
    f.m->xSections[0]->AddAttachedFlag(0, 77u);
    f.m->Panels[0]->attachedFlagPartsNull = false;
    f.m->Panels[0]->attachedFlagParts.push_back({77u});
    f.m->CheckForFairingEdit();
    CHECK_FALSE(f.m->Events()["EditFairing"]->guiActiveEditor);
    CHECK_FALSE(f.m->Fields()["nArcs"]->guiActiveEditor);
    CHECK(f.m->Fields()["editingBlocked"]->guiActiveEditor);
    f.m->Panels[0]->RemoveFlag(77u);
    f.m->CheckForFairingEdit();
    CHECK(f.m->Events()["EditFairing"]->guiActiveEditor);
}

// ModuleProceduralFairing.cs:640 SetInterstage / :673 DumpInterstage: the craft id of the interstage part is mirrored in the persistent field
TEST_CASE("fairing: SetInterstage stores the craft id; DumpInterstage clears it and the node's drag area override") {
    Fairing f;
    auto other = f.g.New("fairingSize1");
    other->fields.craftID = 4242u;
    f.m->SetInterstage(other.get());
    CHECK(f.m->Interstage() == other.get());
    CHECK(f.m->Get<uint32_t>("interstageCraftID") == 4242u);
    f.part->FindAttachNode("top")->overrideDragArea = 3.0f;
    f.m->DumpInterstage();
    CHECK(f.m->Interstage() == nullptr);
    CHECK(f.m->Get<uint32_t>("interstageCraftID") == 0u);
    CHECK(f.part->FindAttachNode("top")->overrideDragArea == -1.0f);
}

// ModuleProceduralFairing.cs:2201 GetInfo / :2196 GetPrimaryField (Number format "N2" / "N3" of the cfg's maxRadius, UnitAreaMass)
TEST_CASE("fairing: module info lists maxRadius, UnitAreaMass and UnitAreaCost from the cfg") {
    Fairing f;
    const std::string info = f.m->GetInfo();
    CHECK(info.find("1.50") != std::string::npos);  // maxRadius = 1.5
    CHECK(info.find("0.010") != std::string::npos);  // UnitAreaMass = 0.01
    CHECK(info.find("6.00") != std::string::npos);   // UnitAreaCost = 6
    CHECK(f.m->GetPrimaryField().find("1.50") != std::string::npos);
}

// ---------------------------------------------------------------------------------------------------------------- ModuleJettison

// ModuleJettison.cs:602 GetModuleMass: only when modifyJettisonedMass; -partDensity (useCalculatedMass) or -jettisonedObjectMass once jettisoned
TEST_CASE("jettison: Jettison sets isJettisoned, fires OnStop(1), drops the event; mass modifier follows modifyJettisonedMass") {
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    auto p = g.New("liquidEngineMini.v2");  // cfg: ModuleJettison jettisonName = Shroud, bottomNodeName = bottom, isFairing = True, jettisonedObjectMass = 0.1
    auto* j = dynamic_cast<ModuleJettison*>(p->FindModuleOfClass("ModuleJettison"));
    REQUIRE(j != nullptr);
    CHECK(j->Get<std::string>("jettisonName") == "Shroud");
    CHECK(j->Get<float>("jettisonedObjectMass") == 0.1f);
    CHECK(j->Get<bool>("isFairing"));
    CHECK(j->GetScalar() == 0.0f);
    CHECK(j->CanMove());
    CHECK(j->GetDragCubeNames() == std::vector<std::string>{"Fairing", "Clean"});
    float stopped = -1.0f;
    struct Spy {
        float* v;
        void OnStop(float x) { *v = x; }
    } spy{&stopped};
    j->OnStop.Add(&spy, &Spy::OnStop);
    j->Jettison();
    j->OnStop.Remove(&spy, &Spy::OnStop);
    CHECK(j->Get<bool>("isJettisoned"));
    CHECK(stopped == 1.0f);
    CHECK(j->GetScalar() == 1.0f);
    CHECK_FALSE(j->CanMove());
    CHECK_FALSE(j->Events()["Jettison"]->active);
    // modifyJettisonedMass is false in the cfg: no mass change
    CHECK(j->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == 0.0f);
    j->Set<bool>("modifyJettisonedMass", true);
    CHECK(j->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == -0.1f);
    j->Set<bool>("useCalculatedMass", true);
    CHECK(j->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == -j->PartDensity());
    j->Set<bool>("isJettisoned", false);
    CHECK(j->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == 0.0f);
    CHECK(j->GetModuleMassChangeWhen() == ModifierChangeWhen::STAGED);
}

// ModuleJettison.cs:136 Jettison (checkBottomNode) and :594 DecoupleAction (only the bottom node, only when not yet jettisoned)
TEST_CASE("jettison: checkBottomNode refuses while a part hangs on the bottom node; DecoupleAction jettisons on that node only") {
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    auto p = g.New("liquidEngineMini.v2");
    auto below = g.New("fairingSize1");
    auto* j = dynamic_cast<ModuleJettison*>(p->FindModuleOfClass("ModuleJettison"));
    REQUIRE(j != nullptr);
    j->Set<bool>("checkBottomNode", true);
    p->FindAttachNode("bottom")->attachedPart = below.get();
    j->Jettison();
    CHECK_FALSE(j->Get<bool>("isJettisoned"));  // "Unable to Jettison"
    p->FindAttachNode("bottom")->attachedPart = nullptr;
    j->DecoupleAction("top", true);
    CHECK_FALSE(j->Get<bool>("isJettisoned"));  // another node
    j->DecoupleAction("bottom", true);
    CHECK(j->Get<bool>("isJettisoned"));
    // ModuleJettison.cs:451 OnActive: staged jettison only when not manual and not yet jettisoned
    j->Set<bool>("isJettisoned", false);
    j->Set<bool>("manualJettison", true);
    j->Set<bool>("stagingEnabled", true);
    j->OnActive();
    CHECK_FALSE(j->Get<bool>("isJettisoned"));
    j->Set<bool>("manualJettison", false);
    j->OnActive();
    CHECK(j->Get<bool>("isJettisoned"));
}

// ModuleJettison.cs:641 / :650 staging queries
TEST_CASE("jettison: IsStageable and the staging texts") {
    Game15 g;
    auto p = g.New("liquidEngineMini.v2");
    auto* j = dynamic_cast<ModuleJettison*>(p->FindModuleOfClass("ModuleJettison"));
    REQUIRE(j != nullptr);
    j->Set<bool>("stagingEnabled", false);
    j->Set<bool>("stagingToggleEnabledEditor", false);
    j->Set<bool>("stagingToggleEnabledFlight", false);
    CHECK_FALSE(j->IsStageable());
    j->Set<bool>("stagingToggleEnabledFlight", true);
    CHECK(j->IsStageable());
    j->Set<bool>("stagingToggleEnabledFlight", false);
    j->Set<bool>("stagingEnabled", true);
    CHECK(j->IsStageable());
    CHECK(j->GetStagingEnableText() == Localizer::Format("#autoLOC_243358"));
    CHECK(j->GetStagingDisableText() == Localizer::Format("#autoLOC_243359"));
    j->Set<std::string>("stagingEnableText", "On");
    CHECK(j->GetStagingEnableText() == "On");
}

// ModuleJettison.cs:524 SetFairingDragCube: weights "Clean" / "Fairing" of the DragCubeList (the WP-21 hook this package owns)
TEST_CASE("jettison: SetFairingDragCube writes the Clean / Fairing cube weights") {
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    auto p = g.New("liquidEngineMini.v2");
    auto* j = dynamic_cast<ModuleJettison*>(p->FindModuleOfClass("ModuleJettison"));
    REQUIRE(j != nullptr);
    REQUIRE(j->IsMultipleCubesActive());  // useMultipleDragCubes defaults to true
    // the prefab's cubes: the engine's cfg defines "Fairing" and "Clean" cubes
    auto weights = [&](const char* name) {
        for (const auto& c : p->DragCubes.Cubes())
            if (c.Name() == name) return c.Weight();
        return -1.0f;
    };
    j->SetFairingDragCube(true);
    CHECK(weights("Clean") == 0.0f);
    CHECK(weights("Fairing") == 1.0f);
    j->SetFairingDragCube(false);
    CHECK(weights("Fairing") == 0.0f);
    CHECK(weights("Clean") == 1.0f);
}

// ------------------------------------------------------------------------------------------------------------- ModuleCargoBay / Part.AddShield

namespace {
// IScalarModule stand-in for the bay's animation (WP-27's ModuleAnimateGeneric): scalar, moving
struct Door : IScalarModule {
    float scalar = 1.0f;
    bool moving = false;
    std::string ScalarModuleID() const override { return "door"; }
    float GetScalar() const override { return scalar; }
    bool CanMove() const override { return true; }
    void SetScalar(float t) override { scalar = t; }
    void SetUIRead(bool) override {}
    void SetUIWrite(bool) override {}
    bool IsMoving() override { return moving; }
};
struct Shield : IAirstreamShield {
    bool closed = false;
    Vessel* vessel = nullptr;
    Part* part = nullptr;
    bool ClosedAndLocked() override { return closed; }
    Vessel* GetVessel() override { return vessel; }
    Part* GetPart() override { return part; }
};
}  // namespace

// Part.cs:7627 AddShield (AddUnique) / :7634 RemoveShield / :7641 OnShieldModified: the part is shielded when at least one shield reports
// ClosedAndLocked; the display string follows the change
TEST_CASE("shielding: Part.AddShield / RemoveShield / OnShieldModified") {
    Game15 g;
    auto p = g.New("mk3CargoBayS");
    Shield a, b;
    p->AddShield(&a);
    CHECK_FALSE(p->ShieldedFromAirstream());  // a shield exists but is not closed
    p->AddShield(&a);                          // AddUnique
    CHECK(p->airstreamShields.size() == 1);
    p->AddShield(&b);
    CHECK(p->airstreamShields.size() == 2);
    b.closed = true;
    p->OnShieldModified(&b);
    CHECK(p->ShieldedFromAirstream());
    CHECK(p->fields.isShieldedDisplay == Localizer::Format("#autoLOC_439839"));
    p->RemoveShield(&b);
    CHECK_FALSE(p->ShieldedFromAirstream());
    p->RemoveShield(&a);
    CHECK(p->airstreamShields.empty());
    CHECK_FALSE(p->ShieldedFromAirstream());
}

// ModuleCargoBay.cs:171 FindAttachNodes, :777 EndCapped, :804 TestAttachmentFit, :750 ClosedAndLocked
TEST_CASE("cargo bay: end caps and size fit decide ClosedAndLocked (mk3CargoBayS)") {
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    auto bay = g.New("mk3CargoBayS");
    auto* cb = dynamic_cast<ModuleCargoBay*>(bay->FindModuleOfClass("ModuleCargoBay"));
    REQUIRE(cb != nullptr);
    CHECK(cb->Get<std::string>("nodeOuterAftID") == "bottom");  // cfg
    CHECK(cb->Get<float>("closedPosition") == 1.0f);
    cb->FindAttachNodes();
    CHECK(cb->NodeOuterFore == bay->FindAttachNode("top"));
    CHECK(cb->NodeOuterAft == bay->FindAttachNode("bottom"));
    CHECK(cb->NodeInnerFore == bay->FindAttachNode("top2"));
    CHECK(cb->NodeInnerAft == bay->FindAttachNode("bottom2"));
    Door door;
    cb->deployModule = &door;
    CHECK_FALSE(cb->SelfClosedAndLocked());  // nothing on the outer nodes: not end-capped
    auto up = g.New("mk3CargoBayS");
    auto down = g.New("mk3CargoBayS");
    AttachNode* top = bay->FindAttachNode("top");
    AttachNode* bottom = bay->FindAttachNode("bottom");
    top->attachedPart = up.get();
    CHECK_FALSE(cb->EndCapped());
    bottom->attachedPart = down.get();
    CHECK_FALSE(cb->EndCapped());  // TestAttachmentFit: no opposing node in a bare test tree
    // an opposing node of the same or larger size makes the node fit (AttachNode.FindOpposingNode: the neighbour's node holding `bay`)
    up->FindAttachNode("bottom")->attachedPart = bay.get();
    down->FindAttachNode("top")->attachedPart = bay.get();
    CHECK(cb->TestAttachmentFit(top));
    CHECK(cb->TestAttachmentFit(bottom));
    CHECK(cb->EndCapped());
    door.scalar = 1.0f;  // closedPosition
    CHECK(cb->SelfClosedAndLocked());
    CHECK(cb->ClosedAndLocked());
    door.moving = true;
    CHECK_FALSE(cb->ClosedAndLocked());
    door.moving = false;
    door.scalar = 0.0f;
    CHECK_FALSE(cb->ClosedAndLocked());
    top->attachedPart = nullptr;
    up->FindAttachNode("bottom")->attachedPart = nullptr;
    door.scalar = 1.0f;
    CHECK_FALSE(cb->ClosedAndLocked());
    cb->deployModule = nullptr;
}

// ModuleCargoBay.cs:703-745 connecting parts, :686 GetOpposingNode, :661 FindConnectingNodes, :619 FindBayColliders (the bay graph)
TEST_CASE("cargo bay: connecting nodes, opposing node and the collider graph walk over joined bays") {
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    auto a = g.New("mk3CargoBayS");
    auto b = g.New("mk3CargoBayS");
    auto* ca = dynamic_cast<ModuleCargoBay*>(a->FindModuleOfClass("ModuleCargoBay"));
    auto* cbb = dynamic_cast<ModuleCargoBay*>(b->FindModuleOfClass("ModuleCargoBay"));
    REQUIRE(ca != nullptr);
    REQUIRE(cbb != nullptr);
    ca->FindAttachNodes();
    cbb->FindAttachNodes();
    // own part as origin: the two outer nodes (aft first)
    const auto own = ca->FindConnectingNodes(a.get());
    REQUIRE(own.size() == 2);
    CHECK(own[0] == a->FindAttachNode("bottom"));
    CHECK(own[1] == a->FindAttachNode("top"));
    // b hangs under a: a's outer-aft node ("bottom") holds b; its opposing node is the outer-fore ("top")
    a->FindAttachNode("bottom")->attachedPart = b.get();
    CHECK(ca->GetOpposingNode(b.get()) == a->FindAttachNode("top"));
    a->FindAttachNode("bottom")->attachedPart = nullptr;
    a->FindAttachNode("top")->attachedPart = b.get();
    CHECK(ca->GetOpposingNode(b.get()) == a->FindAttachNode("bottom"));
    a->FindAttachNode("top")->attachedPart = nullptr;
    CHECK(ca->GetOpposingNode(b.get()) == nullptr);
    // the walk: a - b joined at a's bottom; colliders come from the hook, one per part
    int c1 = 0, c2 = 0;
    ModuleCargoBay::hooks.findPartColliders = [&](Part& p) {
        return std::vector<ModuleCargoBay::PartCollider>{{&p, &p == a.get() ? static_cast<const void*>(&c1) : static_cast<const void*>(&c2)}};
    };
    a->FindAttachNode("bottom")->attachedPart = b.get();
    std::vector<ModuleCargoBay::PartCollider> list;
    std::vector<ModuleCargoBay*> bays;
    ca->FindBayColliders(a.get(), ca, list, bays);
    ModuleCargoBay::hooks.findPartColliders = nullptr;
    // a's own collider; a's opposing node of `origin` (a itself: the outer nodes) leads to b, a cargo bay: added to the bay list
    CHECK(list.size() == 2);
    CHECK(bays.size() == 1);
    CHECK(bays[0] == cbb);
    // connecting parts (:703): added once, cleared (:721)
    auto ext = g.New("mk3CargoBayS");
    ca->AddConnectingPart(ext.get());
    ca->AddConnectingPart(ext.get());
    CHECK(ca->connectingParts.size() == 1);
    ca->RemoveConnectingPart(ext.get());
    CHECK(ca->connectingParts.empty());
    ca->AddConnectingPart(ext.get());
    ca->ClearConnectingParts();
    CHECK(ca->connectingParts.empty());
    ca->SetLookupRadius(3.5f);
    ca->SetLookupCenter(Vector3f(0.0f, 1.0f, 0.0f));
    CHECK(ca->Get<float>("lookupRadius") == 3.5f);
    CHECK(ca->Get<Vector3f>("lookupCenter") == Vector3f(0.0f, 1.0f, 0.0f));
}

// ModuleCargoBay.cs:457 EnableShieldedVolume / :483 DisableShieldedVolume / :522-562: the enclosed parts get this bay as a shield; the part is
// shielded only while the bay is closed and locked; moving doors notify the parts
TEST_CASE("cargo bay: EnableShieldedVolume shields the enclosed parts while the doors are closed") {
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    auto bay = g.New("mk3CargoBayS");
    auto cargo = g.New("fairingSize1");
    auto* cb = dynamic_cast<ModuleCargoBay*>(bay->FindModuleOfClass("ModuleCargoBay"));
    REQUIRE(cb != nullptr);
    cb->FindAttachNodes();
    Door door;
    cb->deployModule = &door;
    auto up = g.New("mk3CargoBayS");
    auto down = g.New("mk3CargoBayS");
    bay->FindAttachNode("top")->attachedPart = up.get();
    bay->FindAttachNode("bottom")->attachedPart = down.get();
    up->FindAttachNode("bottom")->attachedPart = bay.get();
    down->FindAttachNode("top")->attachedPart = bay.get();
    ModuleCargoBay::hooks.findNearbyParts = [&](ModuleCargoBay&, float radius, const Vector3f&) {
        CHECK(radius == 2.0f);  // lookupRadius = 2 in the cfg
        return std::vector<Part*>{cargo.get()};
    };
    ModuleCargoBay::hooks.findEnclosedParts = [&](ModuleCargoBay&, const std::vector<Part*>& nearby, const std::vector<ModuleCargoBay::PartCollider>&) { return nearby; };
    cb->EnableShieldedVolume();
    REQUIRE(cb->partsInCargo.size() == 1);
    CHECK(cb->partsInCargo[0] == cargo.get());
    CHECK(cargo->airstreamShields.size() == 1);
    CHECK(cargo->ShieldedFromAirstream());  // closed (closedPosition = 1, scalar 1) and end-capped
    door.moving = true;
    cb->OnCargoBayDoorsMoving(1.0f, 0.5f);  // from != to: parts are notified
    CHECK_FALSE(cargo->ShieldedFromAirstream());
    door.moving = false;
    cb->OnCargoBayDoorsStopped(1.0f);  // EnableShieldedVolume again, then notify
    CHECK(cargo->ShieldedFromAirstream());
    cb->ModifyShieldInEnclosedParts(false);  // UnShieldEnclosedParts empties partsInCargo
    CHECK(cb->partsInCargo.empty());
    CHECK(cargo->airstreamShields.empty());
    CHECK_FALSE(cargo->ShieldedFromAirstream());
    ModuleCargoBay::hooks = {};
    cb->deployModule = nullptr;
}

// ------------------------------------------------------------------------------------------------------- LaunchClamp / structural nodes

// LaunchClamp.cs:60 OnLoad / :68 OnSave (towerRot), :95 OnPutToGround, :369 GetStageIndex, :347 Release
TEST_CASE("launch clamp: towerRot round trip, OnPutToGround height, stage index, release decouples in flight only") {
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    auto clamp = g.New("launchClamp1");
    auto* lc = dynamic_cast<LaunchClamp*>(clamp->FindModuleOfClass("LaunchClamp"));
    REQUIRE(lc != nullptr);
    ConfigNode in("MODULE");
    in.AddValue("towerRot", ConfigNode::WriteQuaternion(Quaternionf(0.0f, 0.70710677f, 0.0f, 0.70710677f)));
    lc->OnLoad(in);
    ConfigNode out("MODULE");
    lc->OnSave(out);
    CHECK(*out.GetValuePtr("towerRot") == *in.GetValuePtr("towerRot"));
    CHECK(lc->towerRotation.Equals(Quaternionf(0.0f, 0.70710677f, 0.0f, 0.70710677f)));
    // OnPutToGround (:95): lowestOnParts[part] = 0; a lowest point below 0 lifts `height` by (0 - lowest) and resets initialHeight;
    // otherwise the query's lowest point becomes 0
    lc->Set<float>("height", 1.0f);
    lc->initialHeight = 5.0f;
    PartHeightQuery below(-2.0f);
    lc->OnPutToGround(below);
    CHECK(below.lowestPoint == -2.0f);
    CHECK(lc->Get<float>("height") == 1.0f + (0.0f - (-2.0f)));
    CHECK(lc->initialHeight == -1.0f);
    lc->Set<float>("height", 1.0f);
    lc->initialHeight = 5.0f;
    PartHeightQuery above(3.0f);
    lc->OnPutToGround(above);
    CHECK(above.lowestPoint == 0.0f);  // `qr.lowestPoint = qr.lowestOnParts[part]`
    CHECK(lc->Get<float>("height") == 1.0f);
    CHECK(lc->initialHeight == 5.0f);
    clamp->fields.inverseStage = 4;
    CHECK(lc->GetStageIndex(0) == 4);
    // OnStart: the strut icon default and PermanentGroundContact (LaunchClamp.cs:159 / :163); Release needs a parent and the flight scene
    lc->OnStart(StartState::PreLaunch);
    CHECK(clamp->fields.PermanentGroundContact);
    CHECK(clamp->fields.stagingIcon.value_or("") == "STRUT");
    CHECK_FALSE(lc->Events()["Release"]->active);
    lc->Release();  // no parent: nothing happens (no crash)
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    lc->Release();
}

// ModuleStructuralNode.cs:71 SetNodeState / :93 CheckDisplay (interstage caps of fairingSize1: attachNodeNames = interstage01a,interstage01b)
TEST_CASE("structural node: SetNodeState switches the interstage nodes between Stack radius and Dock 0.001; CheckDisplay follows attachments") {
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    auto p = g.New("fairingSize1");
    ModuleStructuralNode* sn = nullptr;
    for (size_t i = 0; i < p->Modules().Count(); i++)
        if (auto* m = dynamic_cast<ModuleStructuralNode*>(p->Modules().GetModule(i))) {
            sn = m;
            break;
        }
    REQUIRE(sn != nullptr);
    CHECK(sn->Get<std::string>("attachNodeNames") == "interstage01a,interstage01b");
    AttachNode* na = p->FindAttachNode("interstage01a");
    AttachNode* nb = p->FindAttachNode("interstage01b");
    REQUIRE(na != nullptr);
    REQUIRE(nb != nullptr);
    sn->SetNodeState(false);
    CHECK(na->nodeType == AttachNodeType::Dock);
    CHECK(na->radius == 0.001f);
    sn->SetNodeState(true);
    CHECK(na->nodeType == AttachNodeType::Stack);
    CHECK(nb->radius == sn->Get<float>("nodeRadius"));
    // CheckDisplay: visible when visibilityState and any listed node has an attached part (spawnManually false), reversed by reverseVisibility
    CHECK_FALSE(sn->CheckDisplay());
    auto other = g.New("fairingSize1");
    nb->attachedPart = other.get();
    CHECK(sn->CheckDisplay());
    sn->Set<bool>("reverseVisibility", true);
    CHECK_FALSE(sn->CheckDisplay());
    sn->Set<bool>("reverseVisibility", false);
    sn->Set<bool>("visibilityState", false);
    CHECK_FALSE(sn->CheckDisplay());
    sn->Set<bool>("visibilityState", true);
    sn->Set<bool>("spawnManually", true);
    CHECK_FALSE(sn->CheckDisplay());  // spawnState false
    sn->SpawnStructure();
    CHECK(sn->CheckDisplay());
    sn->DespawnStructure();
    CHECK_FALSE(sn->CheckDisplay());
    nb->attachedPart = nullptr;
}

// ModuleStructuralNodeToggle.cs:41 UpdateNodes: pushes showMesh / showNodes to every ModuleStructuralNode of the part
TEST_CASE("structural node toggle: UpdateNodes sets visibilityState and the node types of all ModuleStructuralNodes") {
    Game15 g;
    auto p = g.New("fairingSize1");
    ModuleStructuralNodeToggle* t = nullptr;
    for (size_t i = 0; i < p->Modules().Count(); i++)
        if (auto* m = dynamic_cast<ModuleStructuralNodeToggle*>(p->Modules().GetModule(i))) t = m;
    REQUIRE(t != nullptr);
    t->Set<bool>("showMesh", false);
    t->Set<bool>("showNodes", false);
    t->OnFieldUpdated("showMesh");
    size_t expectNodes = 0;  // every ModuleStructuralNode of the part (the cfg lists one per cap)
    for (size_t i = 0; i < p->Modules().Count(); i++) expectNodes += dynamic_cast<ModuleStructuralNode*>(p->Modules().GetModule(i)) != nullptr;
    CHECK(t->structNodes.size() == expectNodes);
    CHECK(expectNodes > 0);
    for (auto* n : t->structNodes) CHECK_FALSE(n->Get<bool>("visibilityState"));
    CHECK(p->FindAttachNode("interstage03a")->nodeType == AttachNodeType::Dock);
    t->Set<bool>("showNodes", true);
    t->OnFieldUpdated("showNodes");
    CHECK(p->FindAttachNode("interstage03a")->nodeType == AttachNodeType::Stack);
}

// DynamicNodeSet.cs:16 Load, ModuleDynamicNodes.cs:134 OnLoad (NODE_SET list, NodeSetIdx), :35 ToggleNodeSet, :25 ChangeNodeSet
TEST_CASE("dynamic nodes: NODE_SET loading and node-set switching") {
    Game15 g;
    auto p = g.New("fairingSize1");  // any part hosts the module in this test: it owns attach nodes interstage0Na / b
    PartModuleData* pmd = p->AddModule("ModuleDynamicNodes", true);
    REQUIRE(pmd != nullptr);
    auto* holder = dynamic_cast<ModuleDynamicNodes*>(pmd->module.get());
    REQUIRE(holder != nullptr);
    ConfigNode node("MODULE");
    node.AddValue("NodeSetIdx", "1");
    ConfigNode* s0 = node.AddNode("NODE_SET");
    s0->AddValue("DisplayText", "Two");
    s0->AddValue("MeshTransform", "Two");
    s0->AddValue("NodePrefix", "interstage01");
    s0->AddValue("SetCount", "1");
    s0->AddValue("Symmetry", "2");
    ConfigNode* s1 = node.AddNode("NODE_SET");
    s1->AddValue("DisplayText", "Three");
    s1->AddValue("NodePrefix", "interstage02");
    s1->AddValue("SetCount", "0");
    holder->OnLoad(node);
    REQUIRE(holder->SetList.size() == 2);
    CHECK(holder->SetList[0].DisplayText == "Two");
    CHECK(holder->SetList[0].SetCount == 1);
    CHECK(holder->SetList[0].Symmetry == 2);
    CHECK(holder->SetList[1].MeshTransform == "");   // absent value: ""
    CHECK(holder->SetList[1].Symmetry == 0);
    CHECK(holder->Get<int32_t>("setIndex") == 1);    // `node.TryGetValue("NodeSetIdx", ref setIndex)` (sic: the field setIndex)
    holder->ToggleNodeSet(0, true);
    // NodePrefix + i for i in 0..SetCount: interstage010 / interstage011 do not exist on this part: no node changes, no failure
    holder->ToggleNodeSet(1, false);
    holder->InitializeNodeSets();
    CHECK(holder->stateNames == std::vector<std::string>{"Two", "Three"});
    holder->ChangeNodeSet(0);
    CHECK(p->fields.stackSymmetry == 2);
}

// ModuleServiceModule.cs:296 PointInBox (strict inequalities against the half sizes), :317 / :327 UpdateStaging, :350 PartDetaches (the
// surface-attached, unshielded children), :345 GetStageIndex. No stock part carries the module (STATUS.md WP-26): it is added to a part.
TEST_CASE("service module: PointInBox, UpdateStaging propagation and the jettison part list") {
    CHECK(ModuleServiceModule::PointInBox(Vector3f(0.4f, 0.0f, 0.0f), Vector3f::zero(), Vector3f(1.0f, 1.0f, 1.0f)));
    CHECK_FALSE(ModuleServiceModule::PointInBox(Vector3f(0.5f, 0.0f, 0.0f), Vector3f::zero(), Vector3f(1.0f, 1.0f, 1.0f)));   // x < 0.5 is strict
    CHECK_FALSE(ModuleServiceModule::PointInBox(Vector3f(-0.5f, 0.0f, 0.0f), Vector3f::zero(), Vector3f(1.0f, 1.0f, 1.0f)));
    CHECK(ModuleServiceModule::PointInBox(Vector3f(1.2f, 2.0f, 3.0f), Vector3f(1.0f, 2.0f, 3.0f), Vector3f(1.0f, 1.0f, 1.0f)));  // the box centre is subtracted
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    auto p = g.New("fairingSize1");
    PartModuleData* sd = p->AddModule("ModuleServiceModule", true);
    PartModuleData* jd = p->AddModule("ModuleJettison", true);
    REQUIRE(sd != nullptr);
    REQUIRE(jd != nullptr);
    auto* sm = dynamic_cast<ModuleServiceModule*>(p->FindModuleOfClass("ModuleServiceModule"));  // (ModuleData may reallocate on AddModule: no stored pointers)
    auto* jm = dynamic_cast<ModuleJettison*>(p->FindModuleOfClass("ModuleJettison"));
    REQUIRE(sm != nullptr);
    REQUIRE(jm != nullptr);
    CHECK(sm->GetModuleDisplayName() == "ServiceModule");
    CHECK_FALSE(sm->IsEnginePlate());
    p->fields.inverseStage = 3;
    CHECK(sm->GetStageIndex(0) == 3);
    // children: one surface-attached and open, one surface-attached and shielded, one on a stack node (srfAttachNode points elsewhere)
    auto open = g.New("fairingSize1");
    auto shielded = g.New("fairingSize1");
    auto stacked = g.New("fairingSize1");
    for (Part* c : {open.get(), shielded.get(), stacked.get()}) {
        c->setParent(p.get());
        if (c->srfAttachNode == nullptr) c->srfAttachNode = std::make_shared<AttachNode>();
    }
    open->srfAttachNode->attachedPart = p.get();
    shielded->srfAttachNode->attachedPart = p.get();
    shielded->set_ShieldedFromAirstream(true);
    stacked->srfAttachNode->attachedPart = nullptr;
    std::vector<Part*> detaches;
    // ModuleServiceModule.cs:248: UseJettisonZones with a missing JettisonZoneNames throws (NullReferenceException on .Split)
    sm->Set<bool>("UseJettisonZones", true);
    REQUIRE(sm->IsNull("JettisonZoneNames"));  // no default in the C# (public string JettisonZoneNames;)
    CHECK_THROWS_AS(sm->BuildJettisonPartsList(), NullReferenceException);
    sm->Set<std::string>("JettisonZoneNames", "");
    CHECK_NOTHROW(sm->BuildJettisonPartsList());
    sm->Set<bool>("UseJettisonZones", false);
    sm->Set<bool>("partDecoupled", true);
    CHECK(sm->PartDetaches(detaches));
    REQUIRE(detaches.size() == 1);
    CHECK(detaches[0] == open.get());
    sm->Set<bool>("partDecoupled", false);
    CHECK_FALSE(sm->PartDetaches(detaches));
    // UpdateStaging(state): its own stagingEnabled and every ModuleJettison's
    sm->SetupJettisonModules();
    REQUIRE(sm->jettisonModules.size() >= 1);
    sm->UpdateStaging(false);
    CHECK_FALSE(sm->Get<bool>("stagingEnabled"));
    CHECK_FALSE(jm->Get<bool>("stagingEnabled"));
    sm->UpdateStaging(true);
    CHECK(sm->Get<bool>("stagingEnabled"));
    CHECK(jm->Get<bool>("stagingEnabled"));
    // OnFieldUpdated("showMesh"): IsDeployed = !showMesh, staging follows
    sm->Set<bool>("showMesh", false);
    sm->OnFieldUpdated("showMesh");
    CHECK(sm->Get<bool>("IsDeployed"));
    CHECK_FALSE(sm->Get<bool>("stagingEnabled"));
    CHECK(sm->GetScalar() == 1.0f);
    CHECK_FALSE(sm->CanMove());
}

// ---------------------------------------------------------------------------------------------------------------------- CompoundPart

// CompoundPart.cs:128 onPartAwake, :145 OnSave / :158 OnLoad, :639 SetTarget, :683 DumpTarget, :677 UpdateWorldValues, :808 UpdateTargetCoords,
// :91 DisconnectCompoundPart, :827 ParseCustomPartData; the cfg values of fuelLine.cfg (maxLength = 10, disconnectAction = true)
TEST_CASE("compound part: fuelLine is a CompoundPart with its cfg fields; target bookkeeping, persistence, disconnect") {
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    auto p = g.New("fuelLine");
    auto* cp = dynamic_cast<CompoundPart*>(p.get());
    REQUIRE(cp != nullptr);  // Part.Instantiate keeps the CompoundPart class
    CHECK(cp->className == "CompoundPart");
    CHECK(cp->compoundFields.maxLength == 10.0f);
    CHECK(cp->compoundFields.disconnectAction);
    CHECK(cp->attachState() == CompoundPart::AttachState::Detached);
    cp->onPartAwake();
    CHECK(cp->isCompund());
    // OnLoad: absent / unparsable keys leave the fields unchanged; HasData sets hasSaveData
    ConfigNode in("PARTDATA");
    in.AddValue("tgt", "17");
    in.AddValue("tpersID", "99");
    in.AddValue("pos", "1,2,3");
    in.AddValue("dir", "0,1,0");
    in.AddValue("rot", "0,0,0,1");
    in.AddValue("col", "meshCol");
    cp->OnLoad(in);
    CHECK(cp->hasSaveData());
    CHECK(cp->tgtId() == 17u);
    CHECK(cp->targetPersistentId() == 99u);
    CHECK(cp->compoundFields.targetPosition == Vector3f(1.0f, 2.0f, 3.0f));
    CHECK(cp->compoundFields.targetMeshColName.value_or("") == "meshCol");
    ConfigNode out("PARTDATA");
    cp->OnSave(out);
    CHECK(*out.GetValuePtr("tgt") == "17");
    CHECK(*out.GetValuePtr("tpersID") == "99");
    CHECK(*out.GetValuePtr("pos") == "1,2,3");
    CHECK(*out.GetValuePtr("col") == "meshCol");
    ConfigNode empty("PARTDATA");
    cp->OnLoad(empty);
    CHECK_FALSE(cp->hasSaveData());
    CHECK(cp->tgtId() == 17u);  // unchanged
    // SetTarget: ids from the target, link, Attached; the world values invert the transforms
    auto tgt = g.New("fairingSize1");
    tgt->fields.craftID = 31u;
    tgt->fields.persistentId = 77u;
    tgt->partTransform.position = Vector3f(0.0f, 5.0f, 0.0f);
    cp->compoundFields.targetPosition = Vector3f(0.0f, 5.0f, 0.0f);
    CHECK(cp->SetTarget(tgt.get(), "col2"));
    CHECK(cp->target == tgt.get());
    CHECK(cp->tgtId() == 31u);
    CHECK(cp->targetPersistentId() == 77u);
    CHECK(cp->attachState() == CompoundPart::AttachState::Attached);
    CHECK(cp->wTgtPos() == Vector3f::zero());  // target.InverseTransformPoint(this.TransformPoint(targetPosition)) with both at their places
    // UpdateTargetCoords: the target moved 2 up in the world; targetPosition follows, direction = its normal
    tgt->partTransform.position = Vector3f(0.0f, 7.0f, 0.0f);
    cp->UpdateTargetCoords();
    CHECK(cp->compoundFields.targetPosition == Vector3f(0.0f, 7.0f, 0.0f));
    CHECK(cp->compoundFields.direction == Vector3f(0.0f, 1.0f, 0.0f));
    // DisconnectCompoundPart: dumps the target and detaches
    cp->DisconnectCompoundPart();
    CHECK(cp->target == nullptr);
    CHECK(cp->tgtId() == 0u);
    CHECK(cp->targetPersistentId() == 0u);
    CHECK(cp->attachState() == CompoundPart::AttachState::Detached);
    CHECK(cp->SetTarget(nullptr, "") == false);
    cp->OnPartDestroy();
}

// CompoundPart.cs:827 ParseCustomPartData: the deprecated `customPartData` string ("rot: ..; pos: ..; dir: ..; tgt: part_N")
TEST_CASE("compound part: ParseCustomPartData upgrades the legacy string; tgt index resolves through the editor's sorted list") {
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    auto p = g.New("strutConnector");
    auto* cp = dynamic_cast<CompoundPart*>(p.get());
    REQUIRE(cp != nullptr);
    auto other = g.New("fairingSize1");
    other->fields.craftID = 555u;
    CompoundPart::hooks.editorSortedShipList = [&] { return std::vector<Part*>{p.get(), other.get()}; };
    ConfigNode n = cp->ParseCustomPartData("rot:0,0,0,1;pos:1,0,0;dir:0,0,1;tgt:part_1;junk");
    CompoundPart::hooks = {};
    CHECK(*n.GetValuePtr("rot") == "0,0,0,1");
    CHECK(*n.GetValuePtr("pos") == "1,0,0");
    CHECK(*n.GetValuePtr("dir") == "0,0,1");
    CHECK(*n.GetValuePtr("tgt") == "555");
    ConfigNode none = cp->ParseCustomPartData("");
    CHECK_FALSE(none.HasData());
}
