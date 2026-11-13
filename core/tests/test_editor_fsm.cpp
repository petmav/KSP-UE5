// WP-40b unit tests: the editor placement loop (EditorLogic KFSM, EditorLogic.cs:1236-2366) driven headless.
//
// There is no in-game editor capture (harness/captures/out*/ has none; harness/golden/MANIFEST.json no editor dataset), so the gate is
// tools/wp40/session_reference.py -> tests/data/wp40b_reference.json: an independent Python simulator of the whole loop written from the decompiled
// C# (no kspcore code) runs scripted sessions on stock part prefabs and stock crafts. The test replays every session's inputs (mouse, buttons, keys,
// host commands) through the real EditorLogic, answers the scene-seam calls (camera ray, Physics.Raycast, attach node icon bounds, FindPartSurface)
// from the log the simulator recorded (asserting that the port asks the same question, in the same order), and compares for every frame the ordered
// trace (FSM events, state changes, GameEvents, audio clips, highlight and tweak hooks) and an FNV-1a hash of the canonical snapshot of every part
// (position, rotation, attPos / attPos0 / attRotation / attRotation0 bits, tree, symmetry sets, attach node links, frozen flag, ship membership).
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <map>
#include <sstream>

#include "test_hygiene.hpp"
#include "input_test_support.hpp"
#include "ksp/editor/editor_logic.hpp"
#include "ksp/editor/editor_reroot.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/mouse.hpp"
#include "ksp/math/mathfx.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using nlohmann::json;
using events::GameEvents;
namespace fs = std::filesystem;

namespace {

float FromHex(const std::string& h) {
    const uint32_t u = static_cast<uint32_t>(std::stoul(h, nullptr, 16));
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

std::string Hex(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    char b[16];
    std::snprintf(b, sizeof b, "%08x", u);
    return b;
}

Vector3f V3(const json& j) { return Vector3f(FromHex(j[0].get<std::string>()), FromHex(j[1].get<std::string>()), FromHex(j[2].get<std::string>())); }
std::string HexV(const Vector3f& v) { return Hex(v.x) + " " + Hex(v.y) + " " + Hex(v.z); }
std::string HexQ(const Quaternionf& q) { return Hex(q.x) + " " + Hex(q.y) + " " + Hex(q.z) + " " + Hex(q.w); }

std::string Fnv1a64(const std::string& s) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 0x100000001b3ULL;
    }
    char b[24];
    std::snprintf(b, sizeof b, "%016llx", static_cast<unsigned long long>(h));
    return b;
}

const json& Ref() {
    static const json j = [] {
        std::ifstream in(std::string(KSPCORE_TEST_DATA_DIR) + "/wp40b_reference.json", std::ios::binary);
        REQUIRE_MESSAGE(in.good(), "tests/data/wp40b_reference.json missing: run tools/wp40/session_reference.py");
        return json::parse(in);
    }();
    return j;
}

const char* kEventNames[] = {"Unknown", "PartCreated", "PartDropped", "PartPicked", "PartDragging", "PartAttached", "PartDetached", "PartDeleted",
                             "PartCopied", "PartRootSelected", "PartOffsetting", "PartOffset", "PartRotating", "PartRotated", "PartTweaked",
                             "PartSymmetryDeleted", "PartOverInventoryGrid", "PartPickedInInventoryGrid", "PartDroppedInInventoryGrid"};
const char* kClipNames[] = {"PartGrab", "PartRelease", "Attach", "CannotPlace", "TweakGrab", "TweakRelease", "DeletePart", "ReRoot"};

// the observer registered after EditorLogic.Awake: GameEvents fire from the last registered listener to the first, so it sees every event
// before the editor's own listeners react (the simulator logs an event before it calls the listeners)
struct Observer {
    std::vector<std::string>* tr = nullptr;
    void Add(const std::string& s) { tr->push_back(s); }
    void PartEvent(ConstructionEventType t, Part*) { Add(std::string("ge:onEditorPartEvent(") + kEventNames[static_cast<int>(t)] + ")"); }
    void Placed(Part*) { Add("ge:onEditorPartPlaced"); }
    void Picked(Part*) { Add("ge:onEditorPartPicked"); }
    void PodPicked(Part*) { Add("ge:onEditorPodPicked"); }
    void PodSelected(Part*) { Add("ge:onEditorPodSelected"); }
    void PodDeleted() { Add("ge:onEditorPodDeleted"); }
    void PartDeleted(Part*) { Add("ge:onEditorPartDeleted"); }
    void RestoreState() { Add("ge:onEditorRestoreState"); }
    void ShipModified(ShipConstruct*) { Add("ge:onEditorShipModified"); }
    void Undo(ShipConstruct*) { Add("ge:onEditorUndo"); }
    void SetBackup(ShipConstruct*) { Add("ge:onEditorSetBackup"); }
    void SymMethod(SymmetryMethod m) { Add("ge:onEditorSymmetryMethodChange(" + std::to_string(static_cast<int>(m)) + ")"); }
    void SymMode(int m) { Add("ge:onEditorSymmetryModeChange(" + std::to_string(m) + ")"); }
    void SymCoords(UnityEngine::Space s) { Add("ge:onEditorSymmetryCoordsChange(" + std::to_string(static_cast<int>(s)) + ")"); }
    void Snap(bool b) { Add("ge:onEditorSnapModeChange(" + std::to_string(b ? 1 : 0) + ")"); }
    void ConstructionMode_(ConstructionMode m) { Add("ge:onEditorConstructionModeChange(" + std::to_string(static_cast<int>(m)) + ")"); }
    void Restart() { Add("ge:onEditorRestart"); }
    void Load(ShipConstruct*, KSP::UI::Screens::CraftBrowserDialog_LoadType) { Add("ge:onEditorLoad"); }
    void Started() { Add("ge:onEditorStarted"); }
    void Dismiss() { Add("ge:onEditorNewShipDialogDismiss"); }

    void Register() {
        GameEvents::onEditorPartEvent.Add(this, &Observer::PartEvent);
        GameEvents::onEditorPartPlaced.Add(this, &Observer::Placed);
        GameEvents::onEditorPartPicked.Add(this, &Observer::Picked);
        GameEvents::onEditorPodPicked.Add(this, &Observer::PodPicked);
        GameEvents::onEditorPodSelected.Add(this, &Observer::PodSelected);
        GameEvents::onEditorPodDeleted.Add(this, &Observer::PodDeleted);
        GameEvents::onEditorPartDeleted.Add(this, &Observer::PartDeleted);
        GameEvents::onEditorRestoreState.Add(this, &Observer::RestoreState);
        GameEvents::onEditorShipModified.Add(this, &Observer::ShipModified);
        GameEvents::onEditorUndo.Add(this, &Observer::Undo);
        GameEvents::onEditorSetBackup.Add(this, &Observer::SetBackup);
        GameEvents::onEditorSymmetryMethodChange.Add(this, &Observer::SymMethod);
        GameEvents::onEditorSymmetryModeChange.Add(this, &Observer::SymMode);
        GameEvents::onEditorSymmetryCoordsChange.Add(this, &Observer::SymCoords);
        GameEvents::onEditorSnapModeChange.Add(this, &Observer::Snap);
        GameEvents::onEditorConstructionModeChange.Add(this, &Observer::ConstructionMode_);
        GameEvents::onEditorRestart.Add(this, &Observer::Restart);
        GameEvents::onEditorLoad.Add(this, &Observer::Load);
        GameEvents::onEditorStarted.Add(this, &Observer::Started);
        GameEvents::onEditorNewShipDialogDismiss.Add(this, &Observer::Dismiss);
    }
    void Unregister() {
        GameEvents::onEditorPartEvent.Remove(this, &Observer::PartEvent);
        GameEvents::onEditorPartPlaced.Remove(this, &Observer::Placed);
        GameEvents::onEditorPartPicked.Remove(this, &Observer::Picked);
        GameEvents::onEditorPodPicked.Remove(this, &Observer::PodPicked);
        GameEvents::onEditorPodSelected.Remove(this, &Observer::PodSelected);
        GameEvents::onEditorPodDeleted.Remove(this, &Observer::PodDeleted);
        GameEvents::onEditorPartDeleted.Remove(this, &Observer::PartDeleted);
        GameEvents::onEditorRestoreState.Remove(this, &Observer::RestoreState);
        GameEvents::onEditorShipModified.Remove(this, &Observer::ShipModified);
        GameEvents::onEditorUndo.Remove(this, &Observer::Undo);
        GameEvents::onEditorSetBackup.Remove(this, &Observer::SetBackup);
        GameEvents::onEditorSymmetryMethodChange.Remove(this, &Observer::SymMethod);
        GameEvents::onEditorSymmetryModeChange.Remove(this, &Observer::SymMode);
        GameEvents::onEditorSymmetryCoordsChange.Remove(this, &Observer::SymCoords);
        GameEvents::onEditorSnapModeChange.Remove(this, &Observer::Snap);
        GameEvents::onEditorConstructionModeChange.Remove(this, &Observer::ConstructionMode_);
        GameEvents::onEditorRestart.Remove(this, &Observer::Restart);
        GameEvents::onEditorLoad.Remove(this, &Observer::Load);
        GameEvents::onEditorStarted.Remove(this, &Observer::Started);
        GameEvents::onEditorNewShipDialogDismiss.Remove(this, &Observer::Dismiss);
    }
};

struct Session {
    Game15& g;
    EditorLogic& editor;
    const json& s;
    std::vector<std::string> trace;
    size_t seamIdx = 0;
    std::string seamError;
    bool uiOver = false;

    Session(Game15& g_, EditorLogic& e, const json& s_) : g(g_), editor(e), s(s_) {}

    int IdOf(const Part* p) const {
        if (p == nullptr) return 0;
        const std::vector<Part*> all = editor.SceneParts();
        for (size_t i = 0; i < all.size(); i++)
            if (all[i] == p) return static_cast<int>(i) + 1;
        return -1;
    }
    std::string Pid(const Part* p) const { return p == nullptr ? "-" : std::to_string(IdOf(p)); }
    Part* PartById(int id) const {
        const std::vector<Part*> all = editor.SceneParts();
        return id >= 1 && static_cast<size_t>(id) <= all.size() ? all[static_cast<size_t>(id) - 1] : nullptr;
    }

    const json* Next(const char* kind) {
        const json& seam = s["seam"];
        if (seamIdx >= seam.size()) {
            if (seamError.empty()) seamError = std::string("seam log exhausted at ") + kind;
            return nullptr;
        }
        const json& e = seam[seamIdx++];
        if (e[0].get<std::string>() != kind) {
            if (seamError.empty()) seamError = std::string("seam call #") + std::to_string(seamIdx - 1) + ": port asked '" + kind + "', reference asked '" + e[0].get<std::string>() + "'";
            return nullptr;
        }
        return &e;
    }
    void Mismatch(const char* kind, const char* what) {
        if (seamError.empty()) seamError = std::string("seam call #") + std::to_string(seamIdx - 1) + " (" + kind + "): " + what + " differs from the reference";
    }
    bool SameV(const json& j, const Vector3f& v) const { return Hex(v.x) == j[0].get<std::string>() && Hex(v.y) == j[1].get<std::string>() && Hex(v.z) == j[2].get<std::string>(); }

    // the canonical snapshot (the same text as session_ref_lib.Sim.snapshot_string)
    std::string Snapshot(size_t* nvals) {
        std::ostringstream o;
        auto ids = [&](const std::vector<Part*>& v) {
            std::string r;
            for (size_t i = 0; i < v.size(); i++) r += (i ? "," : "") + std::to_string(IdOf(v[i]));
            return r;
        };
        o << "S|" << editor.fsm.currentStateName << "|" << Pid(editor.SelectedPart()) << "|" << Pid(editor.RootPart()) << "|" << editor.UndoLevel() << "|"
          << ShipConstruction::backups.size() << "|" << editor.symmetryMode << "|" << static_cast<int>(editor.symmetryMethod) << "|"
          << static_cast<int>(editor.symmetryCoordSpace) << "|" << ids(editor.ship->liveParts);
        for (Part* p : editor.SceneParts()) {
            o << "\n";
            const std::string name = p->partInfo != nullptr ? p->partInfo->name : p->gameObjectName;
            if (editor.IsDestroyed(p)) {
                o << "D|" << IdOf(p) << "|" << name;
                continue;
            }
            const PartFields& f = p->fields;
            *nvals += 3 + 4 + 3 + 3 + 4 + 4;
            std::string nodes;
            for (size_t i = 0; i < p->attachNodes.size(); i++)
                nodes += (i ? ";" : "") + p->attachNodes[i].id + ":" + Pid(p->attachNodes[i].attachedPart);
            o << "P|" << IdOf(p) << "|" << name << "|" << Pid(p->parent) << "|" << HexV(p->partTransform.position) << "|" << HexQ(p->partTransform.rotation) << "|"
              << HexV(f.attPos) << "|" << HexV(f.attPos0) << "|" << HexQ(f.attRotation) << "|" << HexQ(f.attRotation0) << "|" << f.attachMode << "|" << f.symMethod << "|"
              << ids(p->children) << "|" << ids(p->symmetryCounterparts) << "|" << nodes << "|"
              << (p->srfAttachNode ? Pid(p->srfAttachNode->attachedPart) : std::string("-")) << "|"
              << (p->srfAttachNode ? p->srfAttachNode->srfAttachMeshName : std::string()) << "|" << (editor.IsFrozen(p) ? 1 : 0) << "|"
              << (editor.ship->Contains(p) ? 1 : 0);
        }
        return o.str();
    }
};

struct EditorWorld {
    ShipConstructionEnvironment savedEnv;
    EditorWorld() : savedEnv(ShipConstruction::env) {
        ShipConstruction::backups.clear();
        ShipConstruction::ShipConfig = nullptr;
        ShipConstruction::ShipManifest = nullptr;
        EditorEnvironment::Current() = EditorEnvironment{};
        EditorLimits::Instance() = EditorLimits{};
        EditorReRootUtil::context = EditorReRootUtil::Context{};
        Mouse::ResetStatics();
        Time::frameCount = 0;
        Time::time = 0.0f;
        Time::deltaTime = 0.02f;
        GizmoOffsetSpaceReset();
    }
    ~EditorWorld() {
        ShipConstruction::env = savedEnv;
        ShipConstruction::backups.clear();
        ShipConstruction::ShipConfig = nullptr;
        ShipConstruction::ShipManifest = nullptr;
        EditorEnvironment::Current() = EditorEnvironment{};
        EditorReRootUtil::context = EditorReRootUtil::Context{};
        Mouse::ResetStatics();
        InputLockManager::Reset();
        GizmoOffsetSpaceReset();
    }
    // GizmoOffset.coordSpace / GizmoRotate.coordSpace are process-wide statics of the C# (defaults World / Self)
    static void GizmoOffsetSpaceReset() {
        EditorGizmos::GizmoOffset::coordSpace = UnityEngine::Space::World;
        EditorGizmos::GizmoRotate::coordSpace = UnityEngine::Space::Self;
    }
};

void ClearIds() {
    PersistentIdRegistry& ids = SaveEnvironment::Current().persistentIds;
    ids.persistentVesselIds.Clear();
    ids.persistentLoadedPartIds.Clear();
    ids.persistentUnloadedPartIds.Clear();
}

std::string CraftPath(const std::string& key) {
    if (key == "flea") return std::string(KSPCORE_HARNESS_DIR) + "/inputs/Ships/VAB/Jumping Flea.craft";
    if (key == "ion") return std::string(KSPCORE_HARNESS_DIR) + "/inputs/Ships/VAB/Ion-Powered Space Probe.craft";
    return key;
}

struct SessionResult {
    size_t frames = 0, traceRows = 0, values = 0, seamCalls = 0;
    std::string failure;
};

SessionResult RunSession(Game15& g, const json& s, const Vector3f& camera) {
    SessionResult res;
    wp50test::InputFixture input;   // the scripted UnityEngine.Input, GameSettings defaults, no input locks
    EditorWorld world;
    ClearIds();
    EditorEnvironment& env = EditorEnvironment::Current();
    env.editorFacility = s["facility"].get<std::string>() == "SPH" ? EditorFacility::SPH : EditorFacility::VAB;
    EditorLogic editor;
    Session S(g, editor, s);
    Observer obs;
    obs.tr = &S.trace;

    env.cameraPosition = [camera] { return camera; };
    env.screenPointToRay = [&S](const Vector3f& sp) {
        EditorRay r;
        const json* e = S.Next("ray");
        if (e == nullptr) return r;
        if (!S.SameV((*e)[1], sp)) S.Mismatch("ray", "the screen point");
        r.origin = V3((*e)[2]);
        r.direction = V3((*e)[3]);
        return r;
    };
    env.worldToScreenPoint = [](const Vector3f& p) { return Vector3f(f32store(fx(p.x) * 100.0), f32store(fx(p.y) * 100.0), p.z); };
    env.raycast = [&S](const EditorRay& ray, float maxd, int mask) -> std::optional<EditorSurfaceHit> {
        const json* e = S.Next("raycast");
        if (e == nullptr) return std::nullopt;
        if (!S.SameV((*e)[1], ray.origin)) S.Mismatch("raycast", "the ray origin");
        if (!S.SameV((*e)[2], ray.direction)) S.Mismatch("raycast", "the ray direction");
        if ((*e)[3].get<int>() != mask) S.Mismatch("raycast", "the layer mask");
        (void)maxd;
        if ((*e)[4].is_null()) return std::nullopt;
        EditorSurfaceHit h;
        h.part = S.PartById((*e)[4][0].get<int>());
        h.point = V3((*e)[4][1]);
        h.normal = V3((*e)[4][2]);
        h.colliderName = "model";
        return h;
    };
    env.nodeIconUnderCursor = [&S](Part& p, AttachNode& n, const Vector3f& wp) {
        const json* e = S.Next("icon");
        if (e == nullptr) return false;
        if ((*e)[1].get<int>() != S.IdOf(&p) || (*e)[2].get<std::string>() != n.id) S.Mismatch("icon", "the node");
        if (!S.SameV((*e)[3], wp)) S.Mismatch("icon", "the point");
        return (*e)[4].get<bool>();
    };
    env.findPartSurface = [&S](Part& p, const Vector3f& origin, const Vector3f& direction, const Vector3f& fromParent, const EditorSurfaceHit& hit, Vector3f& srfNormal) {
        const json* e = S.Next("surface");
        srfNormal = hit.normal;
        if (e == nullptr) return 1.0f;
        if ((*e)[1].get<int>() != S.IdOf(&p) || !S.SameV((*e)[2], origin) || !S.SameV((*e)[3], direction) || !S.SameV((*e)[4], fromParent) ||
            !S.SameV((*e)[5], hit.normal))
            S.Mismatch("surface", "the query");
        srfNormal = V3((*e)[7]);
        return FromHex((*e)[6].get<std::string>());
    };
    env.pointerOverUI = [&S] { return S.uiOver; };
    env.playOneShot = [&S](EditorClip c) { S.trace.push_back(std::string("clip:") + kClipNames[static_cast<int>(c)]); };
    env.onHighlightSelected = [&S](Part& p, bool attach, bool reset) {
        S.trace.push_back("hl:" + S.Pid(&p) + "," + (attach ? "1" : "0") + "," + (reset ? "1" : "0"));
    };
    env.onEditorStartTweak = [&S](Part&) { S.trace.push_back("tw:start"); };
    env.onEditorEndTweak = [&S](Part&) { S.trace.push_back("tw:end"); };
    env.revealPart = [&S](const AvailablePart& ap) { S.trace.push_back("reveal:" + ap.name); };
    env.newShipDialog = [&S] {
        S.trace.push_back("dialog:newShip");
        return true;
    };

    editor.fsm.OnEventCalled = Callback<KFSMEvent*>::Lambda(&editor, [&S](KFSMEvent* ev) { S.trace.push_back("ev:" + ev->name); });
    editor.fsm.OnStateChange = Callback<KFSMState*, KFSMState*, KFSMEvent*>::Lambda(
        &editor, [&S](KFSMState* a, KFSMState* b, KFSMEvent*) { S.trace.push_back("st:" + a->name + ">" + b->name); });

    editor.Awake();
    obs.Register();
    editor.Start();

    auto compare = [&](const json& expectTrace, const std::string& expectHash, size_t idx, const char* what) {
        if (!res.failure.empty()) return;
        std::ostringstream err;
        if (S.trace.size() != expectTrace.size()) {
            err << what << " " << idx << ": trace length " << S.trace.size() << " vs " << expectTrace.size() << "\n  port:";
            for (const std::string& t : S.trace) err << " [" << t << "]";
            err << "\n  reference:";
            for (const auto& t : expectTrace) err << " [" << t.get<std::string>() << "]";
        } else {
            for (size_t i = 0; i < S.trace.size(); i++)
                if (S.trace[i] != expectTrace[i].get<std::string>()) {
                    err << what << " " << idx << ": trace row " << i << " '" << S.trace[i] << "' vs reference '" << expectTrace[i].get<std::string>() << "'";
                    break;
                }
        }
        res.traceRows += S.trace.size();
        size_t nvals = 0;
        const std::string snap = S.Snapshot(&nvals);
        res.values += nvals;
        if (err.str().empty() && Fnv1a64(snap) != expectHash) err << what << " " << idx << ": snapshot hash differs\n" << snap;
        if (err.str().empty() && !S.seamError.empty()) err << what << " " << idx << ": " << S.seamError;
        res.failure = err.str();
    };
    compare(s["start"]["trace"], s["start"]["hash"].get<std::string>(), 0, "start");

    for (size_t k = 0; k < s["frames"].size() && res.failure.empty(); k++) {
        const json& fr = s["frames"][k];
        S.trace.clear();
        Time::frameCount++;
        Time::deltaTime = FromHex(fr["dt"].get<std::string>());
        Time::time = f32store(fx(Time::time) + fx(Time::deltaTime));
        wp50test::ScriptedDevice& dev = input.dev;
        dev.mouse = Vector3f(FromHex(fr["mouse"][0].get<std::string>()), FromHex(fr["mouse"][1].get<std::string>()), FromHex(fr["mouse"][2].get<std::string>()));
        for (int b = 0; b < 3; b++) {
            dev.mouseDown[b] = fr["md"][static_cast<size_t>(b)].get<bool>();
            dev.mouseUp[b] = fr["mu"][static_cast<size_t>(b)].get<bool>();
            dev.mouseHeld[b] = fr["mh"][static_cast<size_t>(b)].get<bool>();
        }
        dev.down.clear();
        dev.up.clear();
        dev.held.clear();
        for (const auto& x : fr["kd"]) dev.down.insert(x.get<std::string>());
        for (const auto& x : fr["ku"]) dev.up.insert(x.get<std::string>());
        for (const auto& x : fr["kh"]) dev.held.insert(x.get<std::string>());
        S.uiOver = fr.value("ui", false);
        S.trace.push_back("frame");
        for (const json& c : fr["cmds"]) {
            const std::string k2 = c[0].get<std::string>();
            S.trace.push_back("cmd:" + k2);
            if (k2 == "spawn") editor.OnPartListIconTap(g.Info(c[1].get<std::string>().c_str()));
            else if (k2 == "mode") editor.toolsSetMode(static_cast<ConstructionMode>(c[1].get<int>()));
            else if (k2 == "new") editor.NewShip();
            else if (k2 == "load") editor.ShipToLoadSelected(CraftPath(c[1].get<std::string>()), KSP::UI::Screens::CraftBrowserDialog_LoadType::Normal);
            else if (k2 == "inventory") editor.ReleasePartToIcon();
            else if (k2 == "allow") {
                editor.allowSrfAttachment = c[1].get<int>() != 0;
                editor.allowNodeAttachment = c[2].get<int>() != 0;
            } else if (k2 == "destroyHeld") editor.deletePart(editor.SelectedPart());
            else if (k2 == "rootSelect") editor.OnNewRootSelect(S.PartById(c[1].get<int>()));
            else if (k2 == "offsetStart") editor.OffsetGizmo()->OnHandleMoveStart();
            else if (k2 == "offsetMove") editor.OffsetGizmo()->OnHandleMove(V3(c[1]) , FromHex(c[2].get<std::string>()));
            else if (k2 == "offsetEnd") editor.OffsetGizmo()->OnHandleMoveEnd();
            else if (k2 == "rotateStart") editor.RotateGizmo()->OnHandleRotateStart();
            else if (k2 == "rotateMove") editor.RotateGizmo()->OnHandleRotate(V3(c[1]), FromHex(c[2].get<std::string>()));
            else if (k2 == "rotateEnd") editor.RotateGizmo()->OnHandleRotateEnd(V3(c[1]), FromHex(c[2].get<std::string>()));
        }
        Mouse::Update();
        editor.Update();
        editor.RunCoroutines();
        editor.LateUpdate();
        compare(fr["trace"], fr["hash"].get<std::string>(), k + 1, "frame");
        res.frames++;
    }
    if (res.failure.empty() && S.seamIdx != s["seam"].size()) res.failure = "seam log not consumed: " + std::to_string(S.seamIdx) + " of " + std::to_string(s["seam"].size());
    res.seamCalls = S.seamIdx;
    obs.Unregister();
    editor.OnDestroy();
    return res;
}

}  // namespace

TEST_CASE("WP-40b EditorLogic placement loop: scripted sessions against the independent Python simulator") {
    if (!GameAvailable()) return;
    kspcore_tests::LocalizerScope localizer;
    Game15 g;
    LogCapture quiet;
    const json& ref = Ref();
    const Vector3f camera = V3(ref["camera"]);
    size_t sessions = 0, frames = 0, rows = 0, values = 0, seam = 0, failed = 0;
    const char* only = std::getenv("WP40B_ONLY");
    for (const json& s : ref["sessions"]) {
        if (only != nullptr && s["name"].get<std::string>().find(only) == std::string::npos) continue;
        const SessionResult r = RunSession(g, s, camera);
        sessions++;
        frames += r.frames;
        rows += r.traceRows;
        values += r.values;
        seam += r.seamCalls;
        if (!r.failure.empty()) {
            failed++;
            FAIL_CHECK("session " << s["name"].get<std::string>() << ": " << r.failure);
        }
    }
    MESSAGE(sessions << " sessions, " << frames << " frames, " << rows << " trace rows, " << values << " part values (bit-exact), " << seam << " seam calls");
    CHECK(failed == 0);
    CHECK(ref["coverage"]["statesMissing"].empty());
    CHECK(ref["coverage"]["eventsMissing"].empty());
}

// ====================================================================================================================================
// build a stock craft step by step through the editor loop and compare the saved part tree with the original
// ====================================================================================================================================
//
// The driver is a host: it spawns each part from the part list (OnPartListIconTap), aims the cursor (camera rays through the dragOverPlane of the editor
// itself), presses the symmetry keys and clicks. The scene seams are analytic fixtures over the live ship (a node icon is a 0.35 sphere around the node,
// the surface ray hit is the point and normal at which the ORIGINAL pose of the part sits on its parent: test inputs, not part data). Compared: the saved
// craft part tree as a canonical string (part name, attach mode, symmetry set size, symmetry method, linked attach nodes by id and partner name, surface
// link partner name, children as a sorted multiset). NOT compared: craftIDs / persistentIds (re-issued by the editor), pos / rot / attPos0 / attRot0
// (reported as the largest deviation of a part position relative to the root: the editor places by node alignment, the original poses carry the cursor
// path of the player), mir, stage numbers and module data (the editor does not touch them).

namespace {

struct Canon {
    std::string name;
    int attm = 0, symMethod = 0;
    std::vector<int> links, syms;
    std::vector<std::pair<std::string, std::string>> attN;   // node id, partner part name
    std::string srf;
};

std::vector<Canon> ParseSaved(const ConfigNode& root) {
    std::vector<Canon> out;
    std::map<std::string, int> indexOf;
    std::vector<ConfigNode*> parts = root.GetNodes("PART");
    for (size_t i = 0; i < parts.size(); i++) {
        const std::string* pn = parts[i]->GetValuePtr("part");
        indexOf[pn ? *pn : std::string()] = static_cast<int>(i);
    }
    for (ConfigNode* n : parts) {
        Canon c;
        const std::string* pn = n->GetValuePtr("part");
        c.name = pn ? pn->substr(0, pn->rfind('_')) : std::string();
        const std::string* attm = n->GetValuePtr("attm");
        c.attm = attm ? std::stoi(*attm) : 0;
        const std::string* sm = n->GetValuePtr("symMethod");
        c.symMethod = sm && *sm == "Mirror" ? 1 : 0;
        for (const std::string& l : n->GetValues("link")) c.links.push_back(indexOf.count(l) ? indexOf[l] : -1);
        for (const std::string& l : n->GetValues("sym")) c.syms.push_back(indexOf.count(l) ? indexOf[l] : -1);
        for (const std::string& a : n->GetValues("attN")) {
            const size_t comma = a.find(',');
            const std::string rest = a.substr(comma + 1);
            const size_t u1 = rest.find('_'), u2 = rest.find('_', u1 + 1);
            c.attN.push_back({a.substr(0, comma), rest.substr(0, u1)});
            (void)u2;
        }
        const std::string* srf = n->GetValuePtr("srfN");
        if (srf != nullptr) {
            const size_t comma = srf->find(',');
            if (comma != std::string::npos && comma + 1 < srf->size()) {
                const std::string partner = srf->substr(comma + 1);
                c.srf = partner.substr(0, partner.rfind('_'));
            }
        }
        out.push_back(c);
    }
    return out;
}

std::string CanonOf(const std::vector<Canon>& v, int i) {
    const Canon& c = v[static_cast<size_t>(i)];
    std::vector<std::string> nodes;
    for (const auto& a : c.attN) nodes.push_back(a.first + ">" + a.second);
    std::sort(nodes.begin(), nodes.end());
    std::vector<std::string> kids;
    for (int k : c.links) kids.push_back(k >= 0 ? CanonOf(v, k) : std::string("?"));
    std::sort(kids.begin(), kids.end());
    std::string s = c.name + "[" + std::to_string(c.attm) + "," + std::to_string(c.syms.size()) + "," + std::to_string(c.symMethod) + "]{";
    for (const std::string& n : nodes) s += n + ";";
    s += "srf>" + c.srf + "}(";
    for (const std::string& k : kids) s += k + ",";
    return s + ")";
}

std::string CanonTree(const ConfigNode& saved, int* partCount) {
    std::vector<Canon> v = ParseSaved(saved);
    *partCount = static_cast<int>(v.size());
    std::vector<bool> isChild(v.size(), false);
    for (const Canon& c : v)
        for (int k : c.links)
            if (k >= 0) isChild[static_cast<size_t>(k)] = true;
    for (size_t i = 0; i < v.size(); i++)
        if (!isChild[i]) return CanonOf(v, static_cast<int>(i));
    return std::string();
}

struct Rig {
    Game15& g;
    wp50test::InputFixture input;
    EditorWorld world;
    std::unique_ptr<EditorLogic> editor;
    std::map<float, Vector3f> tokens;
    float nextToken = 700.0f;
    std::optional<EditorSurfaceHit> srfHit;
    Vector3f camera{-12.0f, 9.0f, -14.0f};
    Vector3f mouse{640.0f, 360.0f, 0.0f};

    explicit Rig(Game15& g_) : g(g_) {
        ClearIds();
        EditorEnvironment& env = EditorEnvironment::Current();
        editor = std::make_unique<EditorLogic>();
        env.cameraPosition = [this] { return camera; };
        env.screenPointToRay = [this](const Vector3f& sp) {
            EditorRay r;
            r.origin = camera;
            auto it = tokens.find(sp.x);
            const Vector3f tgt = it != tokens.end() ? it->second : Vector3f(0.0f, 0.0f, 0.0f);
            r.direction = (tgt - camera).normalized();
            return r;
        };
        env.worldToScreenPoint = [](const Vector3f& p) { return Vector3f(p.x * 100.0f, p.y * 100.0f, p.z); };
        env.raycast = [this](const EditorRay&, float, int mask) -> std::optional<EditorSurfaceHit> {
            if (mask == 8519681) return srfHit;
            return std::nullopt;
        };
        env.nodeIconUnderCursor = [](Part& p, AttachNode& n, const Vector3f& wp) {
            return (wp - p.partTransform.TransformPoint(n.position)).magnitude() <= 0.35f;
        };
        editor->Awake();
        editor->Start();
    }
    ~Rig() { editor->OnDestroy(); }

    void Frame(bool down = false, bool up = false, const std::vector<std::string>& keysDown = {}, const std::vector<std::string>& keysUp = {}) {
        Time::frameCount++;
        Time::deltaTime = 5.0f;
        Time::time = f32store(fx(Time::time) + 5.0);
        wp50test::ScriptedDevice& dev = input.dev;
        dev.mouse = mouse;
        dev.mouseDown[0] = down;
        dev.mouseHeld[0] = down;
        dev.mouseUp[0] = up;
        dev.down.clear();
        dev.up.clear();
        dev.held.clear();
        for (const auto& k : keysDown) {
            dev.down.insert(k);
            dev.held.insert(k);
        }
        for (const auto& k : keysUp) dev.up.insert(k);
        Mouse::Update();
        editor->Update();
        editor->RunCoroutines();
        editor->LateUpdate();
    }
    void Settle(int n) {
        for (int i = 0; i < n; i++) Frame();
    }
    // the cursor goes where the dragged part lands at `target`: fixed point of the aim point through dragOverPlane
    void AimForPosition(const Vector3f& target) {
        Vector3f aimPt = target;
        for (int it = 0; it < 12; it++) {
            tokens[nextToken] = aimPt;
            mouse = Vector3f(nextToken, 360.0f, 0.0f);
            input.dev.mouse = mouse;
            Vector3f pos;
            editor->dragOverPlane(pos);
            const Vector3f err = target - pos;
            if (err.magnitude() < 1e-4f) break;
            aimPt = aimPt + err;
        }
        nextToken += 3.0f;
    }
    void PressKey(const char* k) { Frame(false, false, {k}); }
    void ReleaseKey(const char* k) { Frame(false, false, {}, {k}); }
};

}  // namespace

TEST_CASE("WP-40b EditorLogic: stock crafts rebuilt part by part through the placement loop save the original part tree") {
    if (!GameAvailable()) return;
    kspcore_tests::LocalizerScope localizer;
    Game15 g;
    LogCapture quiet;
    std::vector<std::string> crafts;
    for (const char* dir : {"VAB", "SPH"}) {
        const fs::path d = fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / dir;
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(d))
            if (e.path().extension() == ".craft") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const auto& f : files) crafts.push_back(f.string());
    }
    const fs::path physDir = fs::path(KSPCORE_PROJECT_ROOT) / "analysis" / "physics" / "craft";
    if (fs::exists(physDir)) {
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(physDir))
            if (e.path().extension() == ".craft") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const auto& f : files) crafts.push_back(f.string());
    }
    // the crafts the rebuild is pinned for (a bounded number of others is attempted and reported)
    const std::set<std::string> pinned = {"PhysProf-S.craft", "PhysProf-M.craft", "PhysProf-L.craft", "Jumping Flea.craft", "Ion-Powered Space Probe.craft"};
    int attempted = 0, equal = 0;
    std::set<std::string> pinnedOkNames;
    std::ostringstream report;
    for (const std::string& path : crafts) {
        const std::string base = fs::path(path).filename().string();
        ClearIds();
        std::unique_ptr<ShipConstruct> orig = ShipConstruction::LoadShip(path);
        if (!orig || orig->liveParts.empty()) continue;
        bool known = true;
        for (Part* p : orig->liveParts)
            if (p->partInfo == nullptr || g.data.parts.getPartInfoByName(p->partInfo->name) == nullptr) known = false;
        if (!known) continue;
        const bool isPinned = pinned.count(base) != 0;
        if (!isPinned && attempted >= 12) continue;
        attempted++;
        std::string wantSaved;
        {
            ConfigNode::NodePtr n = orig->SaveShip();
            int c = 0;
            wantSaved = CanonTree(*n, &c);
        }
        const int origCount = static_cast<int>(orig->liveParts.size());

        std::string failure;
        std::string gotCanon;
        float maxDev = 0.0f;
        {
            Rig rig(g);
            EditorLogic& ed = *rig.editor;
            ed.shipNameField = "rebuild";
            Part* oroot = orig->liveParts[0]->localRoot();
            std::map<Part*, Part*> made;   // original part -> the editor part
            std::set<Part*> skip;          // counterparts (created by symmetry) and their subtrees
            const std::vector<Part*> order = EditorLogicBase::FindPartsInChildren(oroot);
            rig.Frame();
            ed.OnPartListIconTap(*oroot->partInfo);
            rig.Settle(3);
            made[oroot] = ed.RootPart();
            for (Part* p : order) {
                if (!failure.empty()) break;
                if (p == oroot || skip.count(p) != 0) continue;
                Part* oparent = p->parent;
                if (made.count(oparent) == 0) {
                    failure = "parent of " + p->partInfo->name + " was not built";
                    break;
                }
                Part* np = made[oparent];
                // symmetry: this part leads its set; the counterparts are created with it
                const size_t counterparts = p->symmetryCounterparts.size();
                for (Part* c : p->symmetryCounterparts)
                    for (Part* d : EditorLogicBase::FindPartsInChildren(c)) skip.insert(d);
                const SymmetryMethod wantMethod = static_cast<SymmetryMethod>(p->fields.symMethod);
                if (counterparts > 0 && ed.symmetryMethod != wantMethod) rig.ReleaseKey("R");
                for (int tries = 0; tries < 9 && ed.symmetryMode != static_cast<int>(counterparts); tries++) rig.PressKey("X");
                if (ed.symmetryMode != static_cast<int>(counterparts)) {
                    failure = "symmetry mode " + std::to_string(counterparts) + " not reachable for " + p->partInfo->name;
                    break;
                }
                ed.OnPartListIconTap(*p->partInfo);
                Part* held = ed.SelectedPart();
                if (held == nullptr) {
                    failure = "spawn failed for " + p->partInfo->name;
                    break;
                }
                const AttachModes mode = static_cast<AttachModes>(p->fields.attachMode);
                if (mode == AttachModes::STACK) {
                    AttachNode* a = nullptr;
                    AttachNode* b = nullptr;
                    for (AttachNode& n : p->attachNodes)
                        if (n.attachedPart == oparent) a = &n;
                    for (AttachNode& n : oparent->attachNodes)
                        if (n.attachedPart == p) b = &n;
                    if (a == nullptr || b == nullptr) {
                        failure = "no node link between " + p->partInfo->name + " and its parent";
                        break;
                    }
                    AttachNode* na = held->FindAttachNode(a->id);
                    AttachNode* nb = np->FindAttachNode(b->id);
                    if (na == nullptr || nb == nullptr) {
                        failure = "node id missing on the rebuilt parts";
                        break;
                    }
                    ed.allowSrfAttachment = false;
                    rig.srfHit = std::nullopt;
                    const Vector3f target = np->partTransform.TransformPoint(nb->position + nb->offset) - held->partTransform.rotation * na->position;
                    rig.AimForPosition(target);
                } else {
                    ed.allowSrfAttachment = true;
                    // the original pose of the part relative to its parent, replayed onto the rebuilt parent
                    const Vector3f lp = oparent->partTransform.InverseTransformPoint(p->partTransform.position);
                    const Quaternionf lq = QuaternionExtensions::Inverse(oparent->partTransform.rotation) * p->partTransform.rotation;
                    const Vector3f wp = np->partTransform.TransformPoint(lp);
                    const Quaternionf wq = np->partTransform.rotation * lq;
                    AttachNode* srf = p->srfAttachNode.get();
                    const Quaternionf q1 = wq * QuaternionExtensions::Inverse(UnityNative::LookRotation(srf->orientation, Vector3f::up()));
                    EditorSurfaceHit h;
                    h.part = np;
                    h.point = wp + wq * srf->position;
                    h.normal = q1 * Vector3f::forward();
                    h.colliderName = "model";
                    rig.srfHit = h;
                    rig.AimForPosition(h.point);
                }
                rig.Settle(3);
                rig.Frame(true);
                rig.Frame(false, true);
                rig.Settle(2);
                if (!ed.ship->Contains(held)) {
                    failure = "the drop of " + p->partInfo->name + " was refused";
                    break;
                }
                made[p] = held;
                if (held->symmetryCounterparts.size() != counterparts) {
                    failure = "symmetry set size of " + p->partInfo->name + " differs";
                    break;
                }
                const Vector3f origRel = oroot->partTransform.InverseTransformPoint(p->partTransform.position);
                const Vector3f newRel = ed.RootPart()->partTransform.InverseTransformPoint(held->partTransform.position);
                maxDev = std::max(maxDev, (origRel - newRel).magnitude());
            }
            if (failure.empty()) {
                ConfigNode::NodePtr saved = ed.ship->SaveShip();
                int count = 0;
                gotCanon = CanonTree(*saved, &count);
                if (count != origCount) failure = "part count " + std::to_string(count) + " vs " + std::to_string(origCount);
            }
        }
        const bool same = failure.empty() && gotCanon == wantSaved;
        if (same) equal++;
        if (isPinned && same) pinnedOkNames.insert(base);
        report << "\n  " << base << ": " << (same ? "EQUAL" : ("differs: " + (failure.empty() ? std::string("tree") : failure))) << " (" << origCount
               << " parts, max part position deviation from the original " << maxDev << ")";
        if (isPinned) CHECK_MESSAGE(same, base << ": " << (failure.empty() ? std::string("saved part tree differs from the original") : failure));
    }
    MESSAGE(attempted << " crafts rebuilt through the editor loop, " << equal << " saved a part tree equal to the original (" << pinnedOkNames.size() << " of " << pinned.size()
                      << " pinned)" << report.str());
    CHECK(pinnedOkNames.size() == pinned.size());
}
