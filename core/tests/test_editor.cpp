// WP-40 unit tests: the editor core (EditorLogic part-tree operations, symmetry, re-root, gizmos, markers, pre-flight tests).
//
// There is no editor-specific golden or capture (harness/golden/MANIFEST.json has no editor dataset; harness/captures/out*/ none): the C#
// behaviour is covered by tests that (1) run the ported tree operations over every stock craft that loads (harness/inputs/Ships VAB and SPH,
// analysis/physics/craft) and require the original SaveShip bytes back, and (2)-(4) compare against tools/wp40/editor_reference.py (an
// independent numpy re-implementation from the decompiled C#, tests/data/wp40_reference.json, 0 ULP). Every craft / part value is original game data.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "ksp/editor/editor_geometry.hpp"
#include "ksp/editor/editor_gizmos.hpp"
#include "ksp/editor/editor_logic.hpp"
#include "ksp/editor/editor_markers.hpp"
#include "ksp/editor/editor_reroot.hpp"
#include "ksp/editor/preflight_tests.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "nlohmann/json.hpp"
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
        out.push_back({cf, std::move(ship)});
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

TEST_CASE("WP-40 attachPart / detachPart: detaching and re-attaching every part (and its symmetry counterparts) reproduces SaveShip byte-exact") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    EditorScope scope;
    std::vector<LoadedCraft> crafts = LoadAllCrafts();
    REQUIRE(crafts.size() == 57);
    size_t partsDone = 0, symGroups = 0, crafted = 0, localDiffs = 0, restoredLinks = 0;
    for (LoadedCraft& lc : crafts) {
        CAPTURE(lc.file.label);
        std::fprintf(stderr, "craft %s\n", lc.file.label.c_str());
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
            if (std::getenv("WP40_TRACE")) std::fprintf(stderr, "  part %s grp %zu parent %p\n", p->partInfo->name.c_str(), group.size(), (void*)p->parent);
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
            if (std::getenv("WP40_TRACE")) std::fprintf(stderr, "   detaching\n");
            for (Part* q : group) editor.detachPart(q);
            if (std::getenv("WP40_TRACE")) std::fprintf(stderr, "   detached\n");
            REQUIRE(ship.liveParts.size() == countBefore - subtree);
            for (Part* q : group) {
                REQUIRE(q->parent == nullptr);
                REQUIRE_FALSE(ship.Contains(q));
            }
            for (size_t i = 0; i < group.size(); i++) { if (std::getenv("WP40_TRACE")) std::fprintf(stderr, "   attaching %zu\n", i); editor.attachPart(group[i], att[i]); }
            if (std::getenv("WP40_TRACE")) std::fprintf(stderr, "   attached\n");
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
        if (std::getenv("WP40_TRACE") && lc.file.label == "SPH/Dove.craft")
            for (Part* p : ship.liveParts)
                for (size_t m = 0; m < p->Modules().Count(); m++) {
                    if (auto* ee = dynamic_cast<ModuleEngines*>(p->Modules().GetModule(m)))
                        std::fprintf(stderr, "ENGMOD %s %zu maxThrust %g tp %g override %d nT %zu\n", p->partInfo->name.c_str(), m, ee->maxThrust, ee->thrustPercentage(), (int)ee->Get<bool>("manuallyOverridden"), ee->thrustTransforms.size());
                    if (auto* tp = dynamic_cast<IThrustProvider*>(p->Modules().GetModule(m))) {
                        CenterOfThrustQuery q;
                        tp->OnCenterOfThrustQuery(q);
                        std::fprintf(stderr, "ENG %s mod %zu thrust %.9g pos %g %g %g\n", p->partInfo->name.c_str(), m, q.thrust, q.pos.x, q.pos.y, q.pos.z);
                    }
                }
        const EditorRay cot = EditorMarker_CoT::FindCoT(root);
        const json& rc = ref->at("cot");
        CHECK_MESSAGE(SameBits(cot.origin, rc.at("origin")), "CoT origin" << Fmt(cot.origin, rc.at("origin")));
        CHECK_MESSAGE(SameBits(cot.direction, rc.at("direction")), "CoT direction" << Fmt(cot.direction, rc.at("direction")));
        used++;
    }
    CHECK(used >= 30);
}
