// WP-40 unit tests: the editor core (EditorLogic part-tree operations, symmetry, re-root, gizmos, markers, pre-flight tests).
//
// There is no editor-specific golden or capture (harness/golden/MANIFEST.json has no editor dataset; harness/captures/out*/ none): the C#
// behaviour is covered by tests that (1) run the ported tree operations over every stock craft that loads (harness/inputs/Ships VAB and SPH,
// analysis/physics/craft) and require the original SaveShip bytes back, and (2)-(4) compare against tools/wp40/editor_reference.py (an
// independent numpy re-implementation from the decompiled C#, tests/data/wp40_reference.json, 0 ULP). Every craft / part value is original game data.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "test_hygiene.hpp"
#include "ksp/editor/editor_geometry.hpp"
#include "ksp/editor/editor_gizmos.hpp"
#include "ksp/editor/editor_logic.hpp"
#include "ksp/editor/editor_markers.hpp"
#include "ksp/editor/editor_reroot.hpp"
#include "ksp/editor/preflight_tests.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/save/game.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "nlohmann/json.hpp"
#include "editor_staging.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::support;
namespace fs = std::filesystem;
using nlohmann::json;

namespace {

struct CraftFile {
    std::string label;
    fs::path path;
};

std::vector<CraftFile> AllCrafts() {
    std::vector<CraftFile> out;
    for (const char* dir : {"VAB", "SPH"}) {
        const fs::path d = fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / dir;
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(d))
            if (e.path().extension() == ".craft") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const auto& f : files) out.push_back({std::string(dir) + "/" + f.filename().string(), f});
    }
    const fs::path physDir = fs::path(KSPCORE_PROJECT_ROOT) / "analysis" / "physics" / "craft";
    if (fs::exists(physDir)) {
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(physDir))
            if (e.path().extension() == ".craft") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const auto& f : files) out.push_back({"physics/" + f.filename().string(), f});
    }
    return out;
}

ConfigNode::NodePtr Upgraded(const fs::path& file) {
    ConfigNode::NodePtr root = ConfigNode::Load(file.string());
    if (!root) return nullptr;
    const std::string* sn = root->GetValuePtr("ship");
    ConfigNode::NodePtr up;
    KSPUpgradePipeline::Process(root, sn ? *sn : std::string(), SaveUpgradePipeline::LoadContext::Craft, [&](const ConfigNode::NodePtr& n) { up = n; },
                                [&](KSPUpgradePipeline::UpgradeFailOption, const ConfigNode::NodePtr&) {});
    return up;
}

void ClearIds() {
    PersistentIdRegistry& ids = SaveEnvironment::Current().persistentIds;
    ids.persistentVesselIds.Clear();
    ids.persistentLoadedPartIds.Clear();
    ids.persistentUnloadedPartIds.Clear();
}

struct EditorScope {
    ShipConstructionEnvironment savedEnv;
    EditorScope() : savedEnv(ShipConstruction::env) {
        ShipConstruction::backups.clear();
        ShipConstruction::ShipConfig = nullptr;
        ShipConstruction::ShipManifest = nullptr;
        EditorEnvironment::Current() = EditorEnvironment{};
        PreFlightTests::PreFlightEnvironment::Current() = PreFlightTests::PreFlightEnvironment{};
        EditorLimits::Instance() = EditorLimits{};
        EditorReRootUtil::context = EditorReRootUtil::Context{};
    }
    ~EditorScope() {
        ShipConstruction::env = savedEnv;
        ShipConstruction::backups.clear();
        ShipConstruction::ShipConfig = nullptr;
        ShipConstruction::ShipManifest = nullptr;
        EditorEnvironment::Current() = EditorEnvironment{};
        PreFlightTests::PreFlightEnvironment::Current() = PreFlightTests::PreFlightEnvironment{};
        EditorReRootUtil::context = EditorReRootUtil::Context{};
    }
};

struct LoadedCraft {
    CraftFile file;
    std::unique_ptr<ShipConstruct> ship;
    ConfigNode::NodePtr node;   // the upgraded craft node LoadShip read
};

std::vector<LoadedCraft> LoadAllCrafts() {
    std::vector<LoadedCraft> out;
    for (const CraftFile& cf : AllCrafts()) {
        ClearIds();
        ConfigNode::NodePtr up = Upgraded(cf.path);
        if (!up) continue;
        auto ship = std::make_unique<ShipConstruct>();
        std::string err;
        if (!ship->LoadShipWithParts(*up, 0, true, err)) continue;
        out.push_back({cf, std::move(ship), up});
    }
    return out;
}

Part* RootOf(ShipConstruct& ship) {
    for (Part* p : ship.liveParts)
        if (p->parent == nullptr) return p;
    return nullptr;
}

std::string Bytes(ShipConstruct& ship) { return ship.SaveShip()->ToString(); }

std::string FirstDiff(const std::string& a, const std::string& b) {
    std::istringstream sa(a), sb(b);
    std::string la, lb;
    int n = 0;
    while (true) {
        const bool ga = static_cast<bool>(std::getline(sa, la));
        const bool gb = static_cast<bool>(std::getline(sb, lb));
        n++;
        if (!ga && !gb) return "";
        if (!ga || !gb || la != lb) return "line " + std::to_string(n) + ": [" + (ga ? la : "<eof>") + "] vs [" + (gb ? lb : "<eof>") + "]";
    }
}

// the Attachment that re-creates the current attachment of `p` (callerPartNode / otherPartNode as the editor's checkAttach finds them)
Attachment AttachmentOf(Part* p) {
    Attachment a;
    a.caller = p;
    a.potentialParent = p->parent;
    a.position = p->partTransform.position;
    a.rotation = p->partTransform.rotation;
    a.mode = static_cast<AttachModes>(p->fields.attachMode);
    a.possible = true;
    if (a.mode == AttachModes::SRF_ATTACH) {
        a.callerPartNode = p->srfAttachNode.get();
        a.otherPartNode = nullptr;
    } else {
        a.callerPartNode = p->FindAttachNodeByPart(p->parent);
        a.otherPartNode = p->parent->FindAttachNodeByPart(p);
    }
    return a;
}

// "srf:<craftID>|id:<craftID>|..." the attachedPart links of every node of a part
std::string NodeLinks(const Part* p) {
    std::string t = "parent:" + std::to_string(p->parent ? p->parent->fields.craftID : 0) + " mode" + std::to_string(p->fields.attachMode) + " srf:" + std::to_string(p->srfAttachNode->attachedPart ? p->srfAttachNode->attachedPart->fields.craftID : 0);
    for (const AttachNode& n : p->attachNodes) t += "|" + n.id + ":" + std::to_string(n.attachedPart ? n.attachedPart->fields.craftID : 0);
    return t;
}

}  // namespace

// ====================================================================================================================================
// (1) part-tree operations over every stock craft
// ====================================================================================================================================

TEST_CASE("WP-40 attachPart / detachPart: detaching and re-attaching every part (and its symmetry counterparts) reproduces SaveShip (all lines except attPos0 / attRot0, which are re-derived from the world pose)") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    EditorScope scope;
    std::vector<LoadedCraft> crafts = LoadAllCrafts();
    REQUIRE(crafts.size() == 57);
    size_t partsDone = 0, symGroups = 0, crafted = 0, localDiffs = 0, restoredLinks = 0;
    for (LoadedCraft& lc : crafts) {
        CAPTURE(lc.file.label);
        ShipConstruct& ship = *lc.ship;
        EditorLogic editor;
        editor.ship = &ship;
        editor.SetRootPart(RootOf(ship));
        const std::string before = Bytes(ship);
        const std::vector<Part*> fileOrder = ship.liveParts;
        std::set<Part*> done;
        // group = the part and its symmetry counterparts (a symmetry set is detached and re-attached together)
        for (Part* p : fileOrder) {
            if (p == editor.RootPart() || done.count(p) != 0) continue;
            std::vector<Part*> group{p};
            for (Part* s : p->symmetryCounterparts)
                if (s != nullptr && s != p && s != editor.RootPart() && std::find(group.begin(), group.end(), s) == group.end()) group.push_back(s);
            std::map<Part*, std::vector<Part*>> linksBefore;   // attachedPart of every node (srf node first)
            for (Part* q : ship.liveParts) {
                std::vector<Part*>& v = linksBefore[q];
                v.push_back(q->srfAttachNode->attachedPart);
                for (const AttachNode& n : q->attachNodes) v.push_back(n.attachedPart);
            }
            std::vector<Attachment> att;
            std::vector<size_t> childIndex;
            std::vector<Part*> parents;
            for (Part* q : group) {
                att.push_back(AttachmentOf(q));
                parents.push_back(q->parent);
                childIndex.push_back(static_cast<size_t>(std::find(q->parent->children.begin(), q->parent->children.end(), q) - q->parent->children.begin()));
            }
            const size_t countBefore = ship.liveParts.size();
            size_t subtree = 0;
            for (Part* q : group) subtree += EditorLogicBase::FindPartsInChildren(q).size();
            for (Part* q : group) editor.detachPart(q);
            REQUIRE(ship.liveParts.size() == countBefore - subtree);
            for (Part* q : group) {
                REQUIRE(q->parent == nullptr);
                REQUIRE_FALSE(ship.Contains(q));
            }
            for (size_t i = 0; i < group.size(); i++) editor.attachPart(group[i], att[i]);
            REQUIRE(ship.liveParts.size() == countBefore);
            // attachPart appends to the parent's child list (the editor drops parts at the end): put the siblings back at their file index
            {
                std::vector<size_t> order(group.size());
                for (size_t i = 0; i < order.size(); i++) order[i] = i;
                std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return childIndex[a] < childIndex[b]; });
                for (size_t i : order) {
                    auto& ch = parents[i]->children;
                    ch.erase(std::find(ch.begin(), ch.end(), group[i]));
                    ch.insert(ch.begin() + static_cast<std::ptrdiff_t>(std::min(childIndex[i], ch.size())), group[i]);
                }
            }
            // Links attachPart cannot re-create: a stale surface link on a stack-attached part (detachPart clears it) and the node of a
            // part that file data links to two others (the strut cubes of Z-MAP link each other). The editor loses them too; restore for the byte test.
            for (Part* q : ship.liveParts) {
                const std::vector<Part*>& v = linksBefore[q];
                auto fix = [&](AttachNode& n, Part* want) {
                    if (n.attachedPart != want) {
                        n.attachedPart = want;
                        restoredLinks++;
                    }
                };
                fix(*q->srfAttachNode, v[0]);
                for (size_t k = 0; k < q->attachNodes.size(); k++) fix(q->attachNodes[k], v[k + 1]);
            }
            for (Part* q : group) done.insert(q);
            partsDone += group.size();
            if (group.size() > 1) symGroups++;
        }
        // attachPart / addToShip append in tree order; the file order of ship.parts is the loader's
        std::vector<Part*> sortedNow = ship.liveParts;
        std::sort(sortedNow.begin(), sortedNow.end());
        std::vector<Part*> sortedFile = fileOrder;
        std::sort(sortedFile.begin(), sortedFile.end());
        REQUIRE(sortedNow == sortedFile);
        ship.liveParts = fileOrder;
        const std::string after = Bytes(ship);
        {
            // every line must match except the two local-pose lines Unity derives from the world pose when the part is re-parented
            std::istringstream sa(before), sb(after);
            std::string la, lb;
            int n = 0, other = 0;
            while (true) {
                const bool ga = static_cast<bool>(std::getline(sa, la)), gb = static_cast<bool>(std::getline(sb, lb));
                n++;
                if (!ga && !gb) break;
                if (!ga || !gb) {
                    FAIL_CHECK("length differs at line " << n);
                    break;
                }
                if (la == lb) continue;
                const bool local = la.find("attPos0 =") != std::string::npos || la.find("attRot0 =") != std::string::npos;
                if (local) localDiffs++;
                else if (other++ < 3) FAIL_CHECK("line " << n << ": [" << la << "] vs [" << lb << "]");
            }
        }
        crafted++;
    }
    CHECK(crafted == 57);
    CHECK(restoredLinks <= 14);   // STATUS: 14 attach-node links (stale surface links, Z-MAP strut cubes)
    MESSAGE("attPos0 / attRot0 lines re-derived from the world pose: " << localDiffs << "; attach node links restored by the test: " << restoredLinks);
    CHECK(partsDone > 1000);
    CHECK(symGroups > 20);
}

TEST_CASE("WP-40 EditorReRootUtil: re-rooting to every part that can be the root and back restores the tree") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    EditorScope scope;
    std::vector<LoadedCraft> crafts = LoadAllCrafts();
    size_t candidates = 0, refused = 0, rerooted = 0;
    for (LoadedCraft& lc : crafts) {
        CAPTURE(lc.file.label);
        ShipConstruct& ship = *lc.ship;
        EditorLogic editor;
        editor.ship = &ship;
        Part* root = RootOf(ship);
        editor.SetRootPart(root);
        EditorReRootUtil::context.shipContains = [&ship](const Part* p) { return ship.Contains(p); };
        auto snapshot = [&]() {
            std::map<const Part*, std::pair<const Part*, std::vector<const Part*>>> m;
            for (Part* p : ship.liveParts) {
                std::vector<const Part*> ch(p->children.begin(), p->children.end());
                std::sort(ch.begin(), ch.end());
                m[p] = {p->parent, ch};
            }
            return m;
        };
        const auto original = snapshot();
        // EditorReRootUtil.GetRootCandidates over the ship with the root selected (Root construction mode)
        EditorReRootUtil::context.selectedPart = root;
        std::vector<Part*> cand = EditorReRootUtil::GetRootCandidates(ship.liveParts);
        for (Part* p : ship.liveParts) {
            if (p == root) continue;
            EditorReRootUtil::context.selectedPart = root;
            const bool isCand = std::find(cand.begin(), cand.end(), p) != cand.end();
            if (!isCand) {
                refused++;
                CHECK_FALSE(EditorReRootUtil::MakeRoot(p, root));
                continue;
            }
            candidates++;
            const AttachModes pm = static_cast<AttachModes>(p->fields.attachMode);
            (void)pm;
            REQUIRE(EditorReRootUtil::MakeRoot(p, root));
            CHECK(p->parent == nullptr);
            CHECK(root->parent != nullptr);
            // the tree is the same undirected tree
            size_t edges = 0;
            for (Part* q : ship.liveParts)
                if (q->parent != nullptr) edges++;
            CHECK(edges == ship.liveParts.size() - 1);
            // and back
            EditorReRootUtil::context.selectedPart = p;
            REQUIRE(EditorReRootUtil::MakeRoot(root, p));
            CHECK(root->parent == nullptr);
            CHECK(snapshot() == original);
            rerooted++;
        }
    }
    CHECK(candidates + refused > 1000);
    MESSAGE("re-root candidates " << candidates << ", refused " << refused << ", flipped and restored " << rerooted);
}

// ====================================================================================================================================
// (2)-(4) against tools/wp40/editor_reference.py (tests/data/wp40_reference.json)
// ====================================================================================================================================

namespace {

const json& Reference() {
    static json ref = [] {
        std::ifstream in(std::string(KSPCORE_TEST_DATA_DIR) + "/wp40_reference.json");
        REQUIRE_MESSAGE(in.good(), "tests/data/wp40_reference.json missing: run tools/wp40/editor_reference.py");
        return json::parse(in);
    }();
    return ref;
}

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
float FromBits(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}
Vector3f V(const json& a) { return {FromBits(a.at(0)), FromBits(a.at(1)), FromBits(a.at(2))}; }

// 0-ULP comparison against the reference bits (a -0 / +0 mismatch is a mismatch too)
bool SameBits(const Vector3f& v, const json& ref) { return Bits(v.x) == ref.at(0) && Bits(v.y) == ref.at(1) && Bits(v.z) == ref.at(2); }
bool SameBits(const Quaternionf& q, const json& ref) { return Bits(q.x) == ref.at(0) && Bits(q.y) == ref.at(1) && Bits(q.z) == ref.at(2) && Bits(q.w) == ref.at(3); }

std::string Fmt(const Vector3f& v, const json& ref) {
    std::ostringstream o;
    o.precision(9);
    o << " got (" << v.x << "," << v.y << "," << v.z << ") want (" << FromBits(ref.at(0)) << "," << FromBits(ref.at(1)) << "," << FromBits(ref.at(2)) << ")";
    return o.str();
}
std::string Fmt(const Quaternionf& v, const json& ref) {
    std::ostringstream o;
    o.precision(9);
    o << " got (" << v.x << "," << v.y << "," << v.z << "," << v.w << ") want (" << FromBits(ref.at(0)) << "," << FromBits(ref.at(1)) << "," << FromBits(ref.at(2)) << "," << FromBits(ref.at(3)) << ")";
    return o.str();
}

// a craft of the reference whose part list the loaded ship matches (the crafts with parts the upgrade pipeline renames are not in the reference)
const json* RefFor(const LoadedCraft& lc) {
    const json& crafts = Reference().at("crafts");
    auto it = crafts.find(lc.file.label);
    if (it == crafts.end() || !it->at("known").get<bool>()) return nullptr;
    const json& names = it->at("names");
    if (names.size() != lc.ship->liveParts.size()) return nullptr;
    for (size_t i = 0; i < names.size(); i++)
        if (lc.ship->liveParts[i]->partInfo->name != names[i].get<std::string>()) return nullptr;
    return &*it;
}

// The upgrade pipeline re-positions some parts (landing gear, old fuel tanks ...): the reference starts from the raw craft file, so a part whose
// loaded pose is not the file pose is outside what the reference can predict
bool PoseIsFile(const json& ref, const std::vector<Part*>& fileOrder, size_t i) {
    return SameBits(fileOrder[i]->partTransform.position, ref.at("pos").at(i)) && SameBits(fileOrder[i]->partTransform.rotation, ref.at("rot").at(i));
}
bool AllPosesAreFile(const json& ref, const std::vector<Part*>& fileOrder) {
    for (size_t i = 0; i < fileOrder.size(); i++)
        if (!PoseIsFile(ref, fileOrder, i)) return false;
    return true;
}

}  // namespace

TEST_CASE("WP-40 reference: attPos0 / attRot0 after attachPart are the local pose of the part, 0 ULP") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    EditorScope scope;
    std::vector<LoadedCraft> crafts = LoadAllCrafts();
    size_t compared = 0, craftsUsed = 0, upgradedSkipped = 0;
    for (LoadedCraft& lc : crafts) {
        const json* ref = RefFor(lc);
        if (ref == nullptr) continue;
        CAPTURE(lc.file.label);
        ShipConstruct& ship = *lc.ship;
        EditorLogic editor;
        editor.ship = &ship;
        editor.SetRootPart(RootOf(ship));
        const std::vector<Part*> fileOrder = ship.liveParts;
        for (size_t i = 0; i < fileOrder.size(); i++) {
            Part* p = fileOrder[i];
            if (p == editor.RootPart()) continue;
            if (!PoseIsFile(*ref, fileOrder, i) || !PoseIsFile(*ref, fileOrder, static_cast<size_t>(ref->at("parent").at(i).get<int>()))) {
                upgradedSkipped++;
                continue;
            }
            Attachment a = AttachmentOf(p);
            editor.detachPart(p);
            editor.attachPart(p, a);
            const json& lp = ref->at("local_pose").at(i);
            REQUIRE_FALSE(lp.is_null());
            CHECK_MESSAGE(SameBits(p->fields.attPos0, lp.at("attPos0")), "part " << i << " " << p->partInfo->name << " attPos0" << Fmt(p->fields.attPos0, lp.at("attPos0")) << " parent C++ " << (std::find(fileOrder.begin(), fileOrder.end(), p->parent) - fileOrder.begin()) << " ref " << ref->at("parent").at(i));
            CHECK_MESSAGE(SameBits(p->fields.attRotation0, lp.at("attRot0")), "part " << i << " " << p->partInfo->name << " attRot0");
            compared++;
        }
        craftsUsed++;
    }
    CHECK(craftsUsed >= 30);
    CHECK(compared > 500);
    MESSAGE(craftsUsed << " crafts, " << compared << " parts compared, " << upgradedSkipped << " skipped (pose changed by the upgrade pipeline)");
}

TEST_CASE("WP-40 reference: UpdateSymmetry (radial, mirror) over the symmetry sets of the stock crafts, 0 ULP") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    EditorScope scope;
    std::vector<LoadedCraft> crafts = LoadAllCrafts();
    size_t cases = 0, placed = 0, radial = 0, mirror = 0;
    for (LoadedCraft& lc : crafts) {
        const json* ref = RefFor(lc);
        if (ref == nullptr) continue;
        CAPTURE(lc.file.label);
        ShipConstruct& ship = *lc.ship;
        EditorLogic editor;
        editor.ship = &ship;
        Part* root = RootOf(ship);
        editor.SetRootPart(root);
        const std::vector<Part*> fileOrder = ship.liveParts;
        if (!AllPosesAreFile(*ref, fileOrder)) continue;
        std::vector<PartTransform> original;
        for (Part* p : fileOrder) original.push_back(p->partTransform);
        for (const json& c : ref->at("symmetry")) {
            const size_t idx = c.at("part");
            Part* sel = fileOrder[idx];
            editor.SetSelectedPart(sel);
            editor.symmetryCoordSpace = c.at("space") == "World" ? UnityEngine::Space::World : UnityEngine::Space::Self;
            if (sel->fields.symMethod != (c.at("method") == "Mirror" ? 1 : 0)) {
                FAIL_CHECK("symMethod of part " << idx << " " << sel->partInfo->name << ": " << sel->fields.symMethod << " vs " << c.at("method"));
                continue;
            }
            REQUIRE(sel->symmetryCounterparts.size() == c.at("mode").get<size_t>());
            editor.UpdateSymmetry(sel, c.at("mode"), sel->parent, sel->fields.attachMode == 1 ? sel->srfAttachNode.get() : nullptr);
            std::set<size_t> expected;
            for (const json& r : c.at("result")) {
                const size_t pi = r.at("part");
                expected.insert(pi);
                const PartTransform& t = fileOrder[pi]->partTransform;
                CHECK_MESSAGE(SameBits(t.position, r.at("pos")), c.at("method") << " " << c.at("space") << " part " << idx << " -> " << pi << " pos" << Fmt(t.position, r.at("pos")));
                CHECK_MESSAGE(SameBits(t.rotation, r.at("rot")), c.at("method") << " " << c.at("space") << " part " << idx << " -> " << pi << " rot" << Fmt(t.rotation, r.at("rot")));
                placed++;
            }
            // nothing else moved
            for (size_t i = 0; i < fileOrder.size(); i++)
                if (expected.count(i) == 0) {
                    CHECK(fileOrder[i]->partTransform.position.Equals(original[i].position));
                    CHECK(fileOrder[i]->partTransform.rotation.Equals(original[i].rotation));
                }
            for (size_t i = 0; i < fileOrder.size(); i++) fileOrder[i]->partTransform = original[i];
            cases++;
            (c.at("method") == "Mirror" ? mirror : radial)++;
        }
    }
    MESSAGE(cases << " cases (" << radial << " radial, " << mirror << " mirror), " << placed << " counterpart poses compared");
    CHECK(radial > 20);
    CHECK(mirror > 5);
}

TEST_CASE("WP-40 reference: EditorGeometryUtil mirror maths, gizmo snapping and drag deltas, 0 ULP") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    EditorScope scope;
    std::vector<LoadedCraft> crafts = LoadAllCrafts();
    size_t mirrorCases = 0, gizmoCases = 0;
    for (LoadedCraft& lc : crafts) {
        const json* ref = RefFor(lc);
        if (ref == nullptr) continue;
        CAPTURE(lc.file.label);
        ShipConstruct& ship = *lc.ship;
        Part* root = RootOf(ship);
        const std::vector<Part*> fileOrder = ship.liveParts;
        const size_t rootIdx = static_cast<size_t>(std::find(fileOrder.begin(), fileOrder.end(), root) - fileOrder.begin());
        for (const json& c : ref->at("mirror_helpers")) {
            if (!PoseIsFile(*ref, fileOrder, c.at("part").get<size_t>()) || !PoseIsFile(*ref, fileOrder, rootIdx)) continue;
            Part* p = fileOrder[c.at("part").get<size_t>()];
            const PartTransform& rt = root->partTransform;
            CHECK_MESSAGE(SameBits(EditorGeometryUtil::MirrorPos(p->partTransform.position, rt, rt), c.at("pos")), "MirrorPos part " << c.at("part") << Fmt(EditorGeometryUtil::MirrorPos(p->partTransform.position, rt, rt), c.at("pos")));
            CHECK_MESSAGE(SameBits(EditorGeometryUtil::MirrorDirection(p->partTransform.position, rt), c.at("dir")), "MirrorDir part " << c.at("part") << Fmt(EditorGeometryUtil::MirrorDirection(p->partTransform.position, rt), c.at("dir")));
            CHECK(SameBits(EditorGeometryUtil::MirrorRotation(p->partTransform.rotation, rt, rt), c.at("rot")));
            mirrorCases++;
        }
        for (const json& c : ref->at("gizmos")) {
            Part* p = fileOrder[c.at("part").get<size_t>()];
            if (!PoseIsFile(*ref, fileOrder, c.at("part").get<size_t>()) || (p->parent != nullptr && !PoseIsFile(*ref, fileOrder, static_cast<size_t>(std::find(fileOrder.begin(), fileOrder.end(), p->parent) - fileOrder.begin())))) continue;
            // GizmoOffset.SnapToGrid
            for (const json& o : c.at("offset")) {
                if (o.contains("space")) {
                    PartTransform host = p->partTransform;
                    PartTransform parentT = p->parent != nullptr ? p->parent->partTransform : PartTransform{};
                    EditorGizmos::GizmoOffset gz;
                    gz.host = {&host, p->parent != nullptr ? &parentT : nullptr};
                    gz.gridSnapInterval = FromBits(o.at("snap"));
                    EditorGizmos::GizmoOffset::coordSpace = o.at("space") == "Self" ? UnityEngine::Space::Self : UnityEngine::Space::World;
                    gz.SnapToGrid();
                    CHECK_MESSAGE(SameBits(host.position, o.at("pos")), "offset snap " << o.at("space") << " part " << c.at("part") << Fmt(host.position, o.at("pos")));
                } else {
                    EditorGizmos::GizmoOffset gz;
                    gz.trfPos0 = p->partTransform.position;
                    gz.offset0 = Vector3f::zero();
                    gz.OnHandleMove(V(o.at("axis")), FromBits(o.at("distance")));
                    CHECK(SameBits(gz.transform.position, o.at("move")));
                }
                gizmoCases++;
            }
            EditorGizmos::GizmoOffset::coordSpace = UnityEngine::Space::World;
            // GizmoRotate
            EditorGizmos::GizmoRotate::coordSpace = UnityEngine::Space::Self;
            for (const json& r : c.at("rotate")) {
                if (r.contains("euler")) {
                    CHECK_MESSAGE(SameBits(UnityNative::EulerAngles(p->partTransform.rotation), r.at("euler")), "eulerAngles part " << c.at("part") << Fmt(UnityNative::EulerAngles(p->partTransform.rotation), r.at("euler")));
                    PartTransform host = p->partTransform;
                    EditorGizmos::GizmoRotate gz;
                    gz.host = {&host, nullptr};
                    gz.gridSnapInterval = FromBits(r.at("snap"));
                    gz.SnapToGrid();
                    CHECK_MESSAGE(SameBits(host.rotation, r.at("rot")), "rotate snap part " << c.at("part"));
                } else {
                    EditorGizmos::GizmoRotate gz;
                    gz.rot0 = p->partTransform.rotation;
                    Quaternionf cb = Quaternionf::identity();
                    gz.onGizmoRotate = [&](const Quaternionf& q) { cb = q; };
                    gz.OnHandleRotate(V(r.at("axis")), FromBits(r.at("dAngle")));
                    CHECK_MESSAGE(SameBits(cb, r.at("aa")), "AngleAxis part " << c.at("part") << " angle " << FromBits(r.at("dAngle")) << Fmt(cb, r.at("aa")));
                    CHECK_MESSAGE(SameBits(gz.transform.rotation, r.at("gizmo")), "gizmo part " << c.at("part") << Fmt(gz.transform.rotation, r.at("gizmo")));
                }
                gizmoCases++;
            }
        }
    }
    MESSAGE(mirrorCases << " mirror cases, " << gizmoCases << " gizmo cases");
    CHECK(mirrorCases > 100);
    CHECK(gizmoCases > 300);
}

// the engine thrust transform stand-in of tools/golden_runner/ds_deltav_stages.cpp InstallThrustTransforms (the .mu reader is not ported)
static void InstallThrustTransforms(Part& part) {
    for (size_t i = 0; i < part.Modules().Count(); i++) {
        auto* engine = dynamic_cast<ModuleEngines*>(part.Modules().GetModule(i));
        if (engine == nullptr) continue;
        PartTransform t;
        t.position = part.partTransform.position;
        t.rotation = Quaternionf::Normalize(part.partTransform.rotation * UnityNative::AngleAxis(90.0f, Vector3f::right()));
        engine->thrustTransforms = {t};
        engine->thrustTransformMultipliers = {1.0f};
    }
}

TEST_CASE("WP-40 reference: centre of mass and centre of thrust of the stock crafts, 0 ULP") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    EditorScope scope;
    std::vector<LoadedCraft> crafts = LoadAllCrafts();
    size_t used = 0;
    for (LoadedCraft& lc : crafts) {
        const json* ref = RefFor(lc);
        if (ref == nullptr) continue;
        CAPTURE(lc.file.label);
        ShipConstruct& ship = *lc.ship;
        EditorLogic editor;
        editor.ship = &ship;
        Part* root = RootOf(ship);
        editor.SetRootPart(root);
        if (!AllPosesAreFile(*ref, ship.liveParts)) continue;
        const Vector3f com = EditorMarker_CoM::findCenterOfMass(root);
        CHECK_MESSAGE(SameBits(com, ref->at("com").at("com")), "CoM" << Fmt(com, ref->at("com").at("com")));
        for (Part* p : ship.liveParts) InstallThrustTransforms(*p);
        const EditorRay cot = EditorMarker_CoT::FindCoT(root);
        const json& rc = ref->at("cot");
        CHECK_MESSAGE(SameBits(cot.origin, rc.at("origin")), "CoT origin" << Fmt(cot.origin, rc.at("origin")));
        CHECK_MESSAGE(SameBits(cot.direction, rc.at("direction")), "CoT direction" << Fmt(cot.direction, rc.at("direction")));
        used++;
    }
    CHECK(used >= 30);
}

// ====================================================================================================================================
// (4) every PreFlightTest over every stock craft; the expected outcomes are derived from the craft contents by the Python side
// ====================================================================================================================================

namespace {

std::vector<size_t> IndicesOf(const std::vector<Part*>& parts, const std::vector<Part*>& all) {
    std::vector<size_t> out;
    for (Part* p : parts) out.push_back(static_cast<size_t>(std::find(all.begin(), all.end(), p) - all.begin()));
    std::sort(out.begin(), out.end());
    return out;
}

}  // namespace

TEST_CASE("WP-40 PreFlightTests: every design concern and pre-flight test over every stock craft") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    EditorScope scope;
    using namespace PreFlightTests;
    std::vector<LoadedCraft> crafts = LoadAllCrafts();
    REQUIRE(crafts.size() == 57);
    Game game;   // HighLogic.CurrentGame: a sandbox game with an empty crew roster
    game.CrewRoster = std::make_shared<KerbalRoster>(Game_Modes::SANDBOX);
    ShipConstruction::env.CurrentGame = &game;
    SaveEnvironment::Current().currentGame = &game;
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    size_t checked = 0, derived = 0, failedParts = 0;
    for (LoadedCraft& lc : crafts) {
        CAPTURE(lc.file.label);
        ShipConstruct& ship = *lc.ship;
        EditorLogic editor;
        editor.ship = &ship;
        Part* root = RootOf(ship);
        editor.SetRootPart(root);
        EditorSession session(editor);   // EditorLogic.Start: the StageManager of the scene (ParachuteOnFirstStage reads StageManager.LastStage)
        EditorSession::StartParts(ship.liveParts);
        session.Load();
        std::shared_ptr<VesselCrewManifest> manifest = VesselCrewManifest::FromConfigNode(*lc.node);
        ShipConstruction::ShipManifest = manifest.get();
        PreFlightEnvironment& env = PreFlightEnvironment::Current();
        env = PreFlightEnvironment{};
        env.commNetEnabled = true;
        env.ship = &ship;
        env.rootPart = root;
        const json* ref = RefFor(lc);
        auto expect = [&](const char* name, bool pass, const std::vector<Part*>* failed) {
            checked++;
            if (ref == nullptr || !ref->at("preflight").contains(name)) return;
            const json& e = ref->at("preflight").at(name);
            derived++;
            CHECK_MESSAGE(pass == e.at("pass").get<bool>(), std::string(name) << " on " << lc.file.label);
            if (failed != nullptr && e.contains("failed")) {
                std::vector<size_t> want;
                for (const json& i : e.at("failed")) want.push_back(i.get<size_t>());
                std::sort(want.begin(), want.end());
                CHECK_MESSAGE(IndicesOf(*failed, ship.liveParts) == want, std::string(name) << " affected parts on " << lc.file.label);
            }
        };
        auto concern = [&](auto&& t, const char* name) {
            const bool pass = t.Test();
            const std::vector<Part*> failed = t.GetAffectedParts();
            for (Part* p : failed) CHECK(std::find(ship.liveParts.begin(), ship.liveParts.end(), p) != ship.liveParts.end());
            failedParts += failed.size();
            CHECK(t.GetPreviousResult() == pass);
            CHECK(t.Test() == pass);   // idempotent
            const DesignConcernSeverity sev = t.GetSeverity();
            CHECK((sev == DesignConcernSeverity::NOTICE || sev == DesignConcernSeverity::WARNING || sev == DesignConcernSeverity::CRITICAL));
            expect(name, pass, &failed);
        };
        concern(AirBreathingEnginesForIntakes(), "AirBreathingEnginesForIntakes");
        concern(IntakesForAirBreathingEngines(), "IntakesForAirBreathingEngines");
        concern(AntennaPresent(), "AntennaPresent");
        concern(ComOffset(), "ComOffset");
        concern(ContractEquipment(ship), "ContractEquipment");
        concern(DecouplerFacing(), "DecouplerFacing");
        concern(DecouplersBeforeClamps(), "DecouplersBeforeClamps");
        concern(DockingPortAsDecoupler(), "DockingPortAsDecoupler");
        concern(DockingPortFacing(), "DockingPortFacing");
        concern(DockingPortRCS(), "DockingPortRCS");
        concern(ElectricBatteryAndNoCharge(), "ElectricBatteryAndNoCharge");
        concern(ElectricChargeAndNoBattery(), "ElectricChargeAndNoBattery");
        concern(ElectricChargeAndNoConsumer(), "ElectricChargeAndNoConsumer");
        concern(ElectricConsumerAndNoCharge(), "ElectricConsumerAndNoCharge");
        concern(EnginesJettisonedBeforeUse(), "EnginesJettisonedBeforeUse");
        concern(HatchObstructed(), "HatchObstructed");
        concern(InputAuthority(ship), "InputAuthority");
        concern(KerbalSeatCollision(), "KerbalSeatCollision");
        concern(LadderPresent(), "LadderPresent");
        concern(LandingGearPresent(), "LandingGearPresent");
        concern(MissingCrew(), "MissingCrew");
        concern(NonRootCmdMissaligned(), "NonRootCmdMissaligned");
        concern(ParachuteOnEngineStage(), "ParachuteOnEngineStage");
        concern(ParachuteOnFirstStage(), "ParachuteOnFirstStage");
        concern(ParachutePresent(), "ParachutePresent");
        concern(RequiredIScienceDataTransmitter(), "RequiredIScienceDataTransmitter");
        concern(StationHubAttachments(), "StationHubAttachments");
        {
            NoControlSources t;
            const bool pass = t.Test();
            expect("NoControlSources", pass, nullptr);
            NoControlSources fixed(manifest.get());
            CHECK(fixed.Test() == pass);
            NoCrewInMannedVessel nc;   // [Obsolete] but ported
            (void)nc.Test();
        }
        // the resource design concerns of the engineer report: every resource that flows and is not ElectricCharge
        PartResourceLibrary* lib = PartResourceLibrary::Instance();
        env.sccFlowGraphUCFinder = std::make_shared<RUI::Algorithms::SCCFlowGraphUCFinder>(ship.liveParts);
        for (const auto& def : lib->resourceDefinitions) {
            if (def->resourceFlowMode() == 0 || def->name() == "ElectricCharge") continue;
            ResourceContainersReachable a(def.get());
            ResourceConsumersReachable b(def.get());
            const bool pa = a.Test(), pb = b.Test();
            for (Part* p : a.GetAffectedParts()) CHECK(std::find(ship.liveParts.begin(), ship.liveParts.end(), p) != ship.liveParts.end());
            for (Part* p : b.GetAffectedParts()) CHECK(std::find(ship.liveParts.begin(), ship.liveParts.end(), p) != ship.liveParts.end());
            CHECK(a.Test() == pa);
            CHECK(b.Test() == pb);
            CHECK_FALSE(a.GetConcernDescription().empty());
            CHECK_FALSE(b.GetConcernTitle().empty());
        }
        // the IPreFlightTest family
        {
            CraftWithinPartCountLimit okCount(ship, SpaceCenterFacility::VehicleAssemblyBuilding, ship.LiveCount());
            CraftWithinPartCountLimit overCount(ship, SpaceCenterFacility::VehicleAssemblyBuilding, ship.LiveCount() - 1);
            CHECK(okCount.Test());
            CHECK_FALSE(overCount.Test());
            const float mass = ship.GetTotalMass();
            CraftWithinMassLimits okMass(ship, SpaceCenterFacility::LaunchPad, static_cast<double>(mass));
            CraftWithinMassLimits overMass(ship, SpaceCenterFacility::LaunchPad, static_cast<double>(mass) * 0.5);
            CHECK(okMass.Test());
            CHECK((mass == 0.0f ? overMass.Test() : !overMass.Test()));
            const Vector3f size = ShipConstruction::CalculateCraftSize(ship);
            CraftWithinSizeLimits sizeTest(ship, SpaceCenterFacility::LaunchPad, Vector3f(size.x + 1.0f, size.y + 1.0f, size.z + 1.0f));
            // a craft without renderer bounds has size zero: the test then refuses it (craftSize != zero is part of the condition)
            CHECK(sizeTest.Test() == (size != Vector3f::zero()));
            CraftWithinSizeLimits tooBig(size, "x", SpaceCenterFacility::LaunchPad, Vector3f(size.x - 1.0f, size.y, size.z));
            CHECK_FALSE(tooBig.Test());
            CanAffordLaunchTest freeLaunch(ship, std::nullopt);
            CHECK(freeLaunch.Test());
            CanAffordLaunchTest poor(ship, 0.0);
            CHECK(poor.Test() == (poor.launchCost <= 0.0f));
            ExperimentalPartsAvailable exp(ship);   // no R&D: always passes
            CHECK(exp.Test());
            FacilityOperational fo("LaunchPad", "LaunchPad");
            CHECK_FALSE(fo.Test());   // no PSystemSetup facility
            WrongVesselTypeForLaunchSite wrong(EditorFacility::VAB, "no/such/file.craft", "Plane", "LaunchPad");
            (void)wrong.Test();
            LaunchSiteClear clear("LaunchPad", "LaunchPad");
            CHECK(clear.Test());   // no flight state: nothing obstructs
        }
    }
    // PreFlightCheck: the list runs in order, stops at the first failing test, Proceed() continues, Abort() reports
    {
        bool completed = false, aborted = false;
        PreFlightCheck check([&] { completed = true; }, [&] { aborted = true; });
        CHECK(check.RunTests());   // no tests: runTestResult = true
        auto* ship = crafts[0].ship.get();
        check.AddTest(std::make_unique<CraftWithinPartCountLimit>(*ship, SpaceCenterFacility::LaunchPad, ship->LiveCount()));
        check.AddTest(std::make_unique<CraftWithinPartCountLimit>(*ship, SpaceCenterFacility::LaunchPad, 0));
        check.AddTest(std::make_unique<CraftWithinPartCountLimit>(*ship, SpaceCenterFacility::LaunchPad, ship->LiveCount()));
        CHECK_FALSE(check.RunTests());
        CHECK(check.failedTest != nullptr);
        CHECK_FALSE(completed);
        CHECK(check.log.size() == 2);
        check.Abort();
        CHECK(aborted);
    }
    SaveEnvironment::Current().currentGame = nullptr;
    MESSAGE(checked << " test evaluations, " << derived << " compared with the Python expectation, " << failedParts << " affected parts reported");
    CHECK(derived > 500);
}

// ====================================================================================================================================
// the placement flow with real parts: spawn, checkAttach (stack and surface), radial symmetry, drop, undo / redo, delete
// ====================================================================================================================================

TEST_CASE("WP-40 EditorLogic placement flow: stack attach, radial symmetry, drop, undo / redo, delete (stock part prefabs)") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    EditorScope scope;
    ClearIds();
    ShipConstruct ship(EditorFacility::VAB);
    EditorLogic editor;
    editor.ship = &ship;
    editor.shipNameField = "WP-40 flow";
    EditorEnvironment& env = EditorEnvironment::Current();
    env.nodeIconUnderCursor = [](Part&, AttachNode&, const Vector3f&) { return true; };

    // the command pod becomes the root (OnPodSpawn, then st_podSelect drops it at initialPodPosition)
    Part* pod = editor.OnPodSpawn(g.Info("mk1pod.v2"));
    REQUIRE(pod != nullptr);
    CHECK(editor.RootPart() == pod);
    pod->partTransform.position = Vector3f(0.0f, 15.0f, 0.0f);
    ship.Add(pod);
    editor.SetBackup();
    CHECK(ShipConstruction::backups.size() == 1);
    CHECK(editor.UndoLevel() == 1);

    // a stack part: the tank is dragged to the pod and its top node meets the pod bottom node
    Part* tank = editor.SpawnPart(g.Info("fuelTankSmallFlat"));
    REQUIRE(tank != nullptr);
    CHECK(editor.SelectedPart() == tank);
    tank->partTransform.position = pod->partTransform.position + Vector3f(0.0f, -1.5f, 0.0f);
    editor.displayAttachNodeIcons(ship.liveParts, tank, true, true, true);
    editor.symmetryMode = 0;
    editor.UpdatePlacement();
    CHECK(editor.CurrentAttachment().possible);
    CHECK(editor.CurrentAttachment().mode == AttachModes::STACK);
    CHECK(editor.CurrentAttachment().potentialParent == pod);
    REQUIRE(editor.CurrentAttachment().callerPartNode != nullptr);
    REQUIRE(editor.CurrentAttachment().otherPartNode != nullptr);
    // the pair is the last one the search loop finds (tank bottom against pod top here: both orientations qualify); either way they oppose
    CHECK(Vector3f::Dot(tank->partTransform.TransformDirection(editor.CurrentAttachment().callerPartNode->orientation).normalized(),
                        pod->partTransform.TransformDirection(editor.CurrentAttachment().otherPartNode->orientation).normalized()) < -0.7f);
    // the two nodes coincide in the world
    const Vector3f a = tank->partTransform.TransformPoint(editor.CurrentAttachment().callerPartNode->position);
    const Vector3f b = pod->partTransform.TransformPoint(editor.CurrentAttachment().otherPartNode->position + editor.CurrentAttachment().otherPartNode->offset);
    CHECK((a - b).magnitude() < 1e-4f);
    REQUIRE(editor.DropSelectedPart());
    CHECK(ship.LiveCount() == 2);
    CHECK(tank->parent == pod);
    CHECK(tank->isAttached());
    CHECK(editor.CurrentAttachment().callerPartNode->attachedPart == pod);
    CHECK(editor.CurrentAttachment().otherPartNode->attachedPart == tank);
    CHECK(ShipConstruction::backups.size() == 2);
    editor.LeavePlace(true);
    CHECK(editor.SelectedPart() == nullptr);

    // a surface part with three-way radial symmetry on the tank: two counterparts are created by DuplicatePart and rotated by 120 / 240 degrees
    Part* battery = editor.SpawnPart(g.Info("radialDecoupler"));
    REQUIRE(battery != nullptr);
    env.surfaceRaycast = [&]() -> std::optional<EditorSurfaceHit> {
        EditorSurfaceHit h;
        h.part = tank;
        h.point = tank->partTransform.position + Vector3f(0.625f, 0.0f, 0.0f);
        h.normal = Vector3f(1.0f, 0.0f, 0.0f);
        h.colliderName = "model";
        return h;
    };
    editor.displayAttachNodeIcons(ship.liveParts, battery, true, true, true);
    editor.symmetryMethod = SymmetryMethod::Radial;
    editor.symmetryMode = 2;
    battery->partTransform.position = tank->partTransform.position + Vector3f(0.7f, 0.0f, 0.0f);
    editor.UpdatePlacement();
    REQUIRE(editor.CurrentAttachment().possible);
    CHECK(editor.CurrentAttachment().mode == AttachModes::SRF_ATTACH);
    CHECK(editor.CurrentAttachment().potentialParent == tank);
    REQUIRE(battery->symmetryCounterparts.size() == 2);
    REQUIRE(editor.PartAttachments().size() == 2);
    for (Part* c : battery->symmetryCounterparts) {
        CHECK(c != battery);
        CHECK(c->symmetryCounterparts.size() == 2);
        CHECK(c->potentialParent == tank);
        CHECK(c->fields.persistentId != battery->fields.persistentId);
    }
    for (const Attachment& at : editor.PartAttachments()) CHECK(at.possible);
    // the radial maths: the three parts sit on a circle around the tank axis, 120 degrees apart
    const Vector3f axisPos = tank->partTransform.position;
    const float r0 = Vector3f(battery->partTransform.position.x - axisPos.x, 0.0f, battery->partTransform.position.z - axisPos.z).magnitude();
    for (Part* c : battery->symmetryCounterparts) {
        const float r = Vector3f(c->partTransform.position.x - axisPos.x, 0.0f, c->partTransform.position.z - axisPos.z).magnitude();
        CHECK(std::fabs(r - r0) < 1e-4f);
        CHECK(std::fabs(c->partTransform.position.y - battery->partTransform.position.y) < 1e-4f);
    }
    const float ang1 = Vector3f::SignedAngle(battery->partTransform.position - axisPos, battery->symmetryCounterparts[0]->partTransform.position - axisPos, Vector3f::up());
    const float ang2 = Vector3f::SignedAngle(battery->partTransform.position - axisPos, battery->symmetryCounterparts[1]->partTransform.position - axisPos, Vector3f::up());
    CHECK(std::fabs(std::fabs(ang1) - 120.0f) < 0.01f);
    CHECK(std::fabs(std::fabs(ang2) - 120.0f) < 0.01f);
    CHECK(ang1 * ang2 < 0.0f);   // one each way round
    REQUIRE(editor.DropSelectedPart());
    CHECK(ship.LiveCount() == 5);
    CHECK(battery->parent == tank);
    for (Part* c : battery->symmetryCounterparts) {
        CHECK(c->parent == tank);
        CHECK(ship.Contains(c));
    }
    CHECK(ShipConstruction::backups.size() == 3);
    editor.LeavePlace(true);
    CHECK(editor.getSortedShipList().size() == 5);
    CHECK(editor.getSortedShipList()[0] == pod);
    CHECK(editor.AreAllPartsConnected() == (Part::allParts.size() == 5));

    // picking one of the three up removes it from the ship and deletes the counterparts (on_partPicked): back to two parts
    const std::vector<Part*> cps = battery->symmetryCounterparts;
    editor.PickPart(battery);
    CHECK(editor.SelectedPart() == battery);
    CHECK_FALSE(ship.Contains(battery));
    CHECK(ship.LiveCount() == 2);
    CHECK(battery->symmetryCounterparts.empty());
    for (Part* c : cps) CHECK(editor.IsDestroyed(c));

    // on_partPicked took a backup of the two-part ship; undo / redo walk the backup stack (4: two parts, 3: five parts, 2: two parts, 1: pod)
    CHECK(editor.UndoLevel() == 4);
    CHECK(ShipConstruction::backups.size() == 4);
    editor.RestoreState(-1);
    CHECK(editor.UndoLevel() == 3);
    CHECK(editor.ship->LiveCount() == 5);
    editor.RestoreState(-1);
    CHECK(editor.UndoLevel() == 2);
    CHECK(editor.ship->LiveCount() == 2);
    editor.RestoreState(-1);
    CHECK(editor.UndoLevel() == 1);
    CHECK(editor.ship->LiveCount() == 1);
    editor.RestoreState(1);
    editor.RestoreState(1);
    CHECK(editor.UndoLevel() == 3);
    CHECK(editor.ship->LiveCount() == 5);
    CHECK(editor.RootPart() == editor.ship->liveParts[0]->localRoot());

    // delete: the selected part and its subtree leave the ship
    Part* extra = editor.SpawnPart(g.Info("fuelTankSmallFlat"));
    CHECK(editor.CountAllSceneParts(true) == static_cast<int>(Part::allParts.size()) - 1);
    editor.DestroySelectedPart();
    CHECK(editor.SelectedPart() == nullptr);
    CHECK(editor.IsDestroyed(extra));
}

// ====================================================================================================================================
// small API tests: symmetry mode state machine, enum conversions, filters, EditorBounds, native quaternion cases
// ====================================================================================================================================

TEST_CASE("WP-40 EditorLogic symmetry mode cycling (nextSymMode / prevSymMode / symButton / onSymMethodChange)") {
    kspcore_tests::LocalizerScope localizer;  // GetSymmetryMethodName formats #autoLOC_6004039 / 6004040
    EditorScope scope;
    EditorLogic editor;
    // Radial: 0 1 2 3 5 7 (4 and 6 are skipped), then wraps
    std::vector<int> seen;
    editor.symmetryMethod = SymmetryMethod::Radial;
    for (int i = 0; i < 7; i++) {
        seen.push_back(editor.symmetryMode);
        editor.symButton(EditorLogic::SymButton::Left);
    }
    CHECK(seen == std::vector<int>{0, 1, 2, 3, 5, 7, 0});
    // previous: 0 -> 7 -> 5 -> 3 ...
    editor.symmetryMode = 0;
    editor.symButton(EditorLogic::SymButton::Right);
    CHECK(editor.symmetryMode == 7);
    editor.symButton(EditorLogic::SymButton::Right);
    CHECK(editor.symmetryMode == 5);
    editor.symButton(EditorLogic::SymButton::Right);
    CHECK(editor.symmetryMode == 3);
    // middle button toggles the method; mirror has two modes and onSymMethodChange clamps the mode
    editor.symmetryMode = 5;
    editor.symButton(EditorLogic::SymButton::Middle);
    CHECK(editor.symmetryMethod == SymmetryMethod::Mirror);
    editor.onSymMethodChange(SymmetryMethod::Mirror);
    CHECK(editor.symmetryMode == 1);
    editor.symButton(EditorLogic::SymButton::Left);
    CHECK(editor.symmetryMode == 0);
    editor.symButton(EditorLogic::SymButton::Right);
    CHECK(editor.symmetryMode == 1);
    editor.ToggleSymmetryCoordSpace();   // Mirror: unchanged
    CHECK(editor.symmetryCoordSpace == UnityEngine::Space::Self);
    editor.ToggleSymMethod();
    CHECK(editor.symmetryMethod == SymmetryMethod::Radial);
    editor.ToggleSymmetryCoordSpace();
    CHECK(editor.symmetryCoordSpace == UnityEngine::Space::World);
    CHECK(EditorEnumExtensions::GetSymmetryMethodName(SymmetryMethod::Mirror) != EditorEnumExtensions::GetSymmetryMethodName(SymmetryMethod::Radial));
}

TEST_CASE("WP-40 EditorEnumExtensions, EditorPartListFilter(List) and EditorBounds") {
    using namespace EditorEnumExtensions;
    CHECK(ToFacility(EditorFacility::VAB) == SpaceCenterFacility::VehicleAssemblyBuilding);
    CHECK(ToFacility(EditorFacility::SPH) == SpaceCenterFacility::SpaceplaneHangar);
    CHECK(ToFacility(EditorFacility::None) == SpaceCenterFacility::SpaceplaneHangar);
    CHECK(ToEditor(SpaceCenterFacility::Runway) == EditorFacility::SPH);
    CHECK(ToEditor(SpaceCenterFacility::SpaceplaneHangar) == EditorFacility::SPH);
    CHECK(ToEditor(SpaceCenterFacility::LaunchPad) == EditorFacility::VAB);
    CHECK(ToEditor(SpaceCenterFacility::VehicleAssemblyBuilding) == EditorFacility::VAB);
    CHECK(ToEditor(SpaceCenterFacility::TrackingStation) == EditorFacility::None);
    CHECK(GetEditorFacility(SpaceCenterFacility::Runway) == SpaceCenterFacility::SpaceplaneHangar);
    CHECK(GetEditorFacility(SpaceCenterFacility::LaunchPad) == SpaceCenterFacility::VehicleAssemblyBuilding);
    CHECK(GetFacility("RnD") == SpaceCenterFacility::ResearchAndDevelopment);
    CHECK(GetFacility("VAB") == SpaceCenterFacility::VehicleAssemblyBuilding);
    CHECK(GetFacility("FlagPole") == SpaceCenterFacility::AstronautComplex);
    CHECK_THROWS(GetFacility("Nope"));

    EditorPartListFilterList<int> filters;
    auto even = std::make_shared<EditorPartListFilter<int>>("even", [](const int& v) { return v % 2 == 0; }, "odd");
    auto big = std::make_shared<EditorPartListFilter<int>>("big", [](const int& v) { return v > 3; });
    filters.AddFilter(even);
    filters.AddFilter(even);   // already there
    filters.AddFilter(big);
    CHECK(filters.Count() == 2);
    CHECK(filters["big"] == big);
    CHECK(filters["none"] == nullptr);
    CHECK(filters.GetFilterKey() == "evenbig");
    CHECK(filters.GetFilterKeySingleOrNothing().empty());
    const std::vector<int> src{1, 2, 3, 4, 5, 6, 7, 8};
    CHECK(filters.GetFilteredList(src) == std::vector<int>{4, 6, 8});
    std::vector<int> inPlace = src;
    filters.FilterList(inPlace);
    CHECK(inPlace == std::vector<int>{4, 6, 8});
    filters.RemoveFilter("big");
    CHECK(filters.GetFilterKeySingleOrNothing() == "even");
    CHECK(filters.ToString("Int32").find("even 'odd'") != std::string::npos);

    // EditorBounds: one instance; ClampCameraDistance without an instance leaves the value
    CHECK(EditorBounds::ClampCameraDistance(123.0f) == 123.0f);
    EditorBounds a, b;
    a.cameraMinDistance = 3.0f;
    a.cameraMaxDistance = 35.0f;
    CHECK(a.Awake());
    CHECK_FALSE(b.Awake());
    CHECK(EditorBounds::ClampCameraDistance(1.0f) == 3.0f);
    CHECK(EditorBounds::ClampCameraDistance(50.0f) == 35.0f);
    CHECK(EditorBounds::ClampCameraDistance(10.0f) == 10.0f);
    a.OnDestroy();
    CHECK(EditorBounds::Instance == nullptr);
}

TEST_CASE("WP-40 native Quaternion.LookRotation / Euler / eulerAngles: identities and degenerate inputs") {
    // LookRotation(forward, up): the result rotates Vector3.forward onto forward and Vector3.up into the up/forward plane
    const Vector3f fwd = Vector3f(1.0f, 2.0f, 3.0f).normalized();
    const Quaternionf q = UnityNative::LookRotation(fwd, Vector3f::up());
    CHECK((q * Vector3f::forward() - fwd).magnitude() < 1e-6f);
    CHECK(Vector3f::Dot(q * Vector3f::right(), fwd) < 1e-6f);
    // degenerate: zero forward, forward parallel to up -> identity
    CHECK(UnityNative::LookRotation(Vector3f::zero(), Vector3f::up()).Equals(Quaternionf::identity()));
    CHECK(UnityNative::LookRotation(Vector3f::up(), Vector3f::up()).Equals(Quaternionf::identity()));
    // Euler and eulerAngles are inverse of each other (away from gimbal lock)
    const Vector3f e(10.0f, 200.0f, 330.0f);
    const Vector3f back = UnityNative::EulerAngles(UnityNative::Euler(e));
    CHECK(std::fabs(back.x - e.x) < 1e-3f);
    CHECK(std::fabs(back.y - e.y) < 1e-3f);
    CHECK(std::fabs(back.z - e.z) < 1e-3f);
    // eulerAngles is made positive: 0..360
    for (float v : {back.x, back.y, back.z}) CHECK((v >= -0.0057f && v <= 360.0f));
    CHECK(UnityNative::Euler(Vector3f::zero()).Equals(Quaternionf::identity()));
}
