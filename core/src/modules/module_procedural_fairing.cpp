// WP-26: ModuleProceduralFairing.cs (see module_procedural_fairing.hpp for the scope).
#include "ksp/modules/module_procedural_fairing.hpp"

#include <algorithm>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_structural.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {
using GE = GameEvents;

// Number.ToString("N<decimals>") of a float: fixed decimals with ',' thousands groups
std::string FormatSingleN(float v, int decimals) {
    std::string s = dotnet::FormatSingleF(v, decimals);
    size_t start = (!s.empty() && s[0] == '-') ? 1 : 0;
    size_t dot = s.find('.');
    if (dot == std::string::npos) dot = s.size();
    for (long i = static_cast<long>(dot) - 3; i > static_cast<long>(start); i -= 3) s.insert(static_cast<size_t>(i), ",");
    return s;
}
}  // namespace

bool ModuleProceduralFairing::loadedSceneIsEditorOrFlight() {
    const GameScenes s = SaveEnvironment::Current().loadedScene;
    return s == GameScenes::EDITOR || s == GameScenes::FLIGHT;
}

ModuleProceduralFairing::ModuleProceduralFairing(const ModuleClassInfo& cls) : PartModule(cls) {}

ModuleProceduralFairing::~ModuleProceduralFairing() {
    // :777 OnDestroy
    if (onVesselModifiedSubscribed_) GE::onVesselWasModified.Remove(this, &ModuleProceduralFairing::OnVesselModified);
}

Part* ModuleProceduralFairing::PartOf() const { return dynamic_cast<Part*>(part); }

// ModuleProceduralFairing.cs:313 (GameEvents.onVariantsAdded: the variant material copy is the model's)
void ModuleProceduralFairing::OnAwake() {
    PartModule::OnAwake();
    fsmStateName_ = "";
    fsmStarted_ = false;
    interstage_ = nullptr;
    if (!Panels.empty()) WipeMesh();
    xSections.clear();
    Panels.clear();
    if (Part* p = PartOf()) {
        originalCoP_ = p->fields.CoPOffset;
        originalCoM_ = p->fields.CoMOffset;
        originalCoL_ = p->fields.CoLOffset;
    }
    Events().Bind("DeleteFairing", BaseEvent::Handler([this] { DeleteFairing(); }));
    Events().Bind("DeployFairing", BaseEvent::Handler([this] { DeployFairing(); }));
    Actions().Bind("DeployFairingAction", [this](KSPActionParam& p) { DeployFairingAction(p); });
}

// ModuleProceduralFairing.cs:330
void ModuleProceduralFairing::OnStart(StartState state) {
    PartModule::OnStart(state);
    Part* p = PartOf();
    if (p == nullptr) throw NullReferenceException();
    const GameScenes scene = SaveEnvironment::Current().loadedScene;
    const bool editor = scene == GameScenes::EDITOR;
    const bool flight = scene == GameScenes::FLIGHT;
    // :349-377 materials and textures: the model's
    for (const char* e : {"DeleteFairing", "EditFairing", "BuildFairing", "DeployFairing"})
        if (BaseEvent* ev = Events()[e]) ev->active = false;
    cargoModule = dynamic_cast<ModuleCargoBay*>(p->FindModuleImplementing([](PartModule& m) { return m.IsA("ModuleCargoBay"); }));
    if (!xSections.empty()) SpawnMeshes(true);
    if (editor || flight) {
        // SetupEditorFSM / SetupFlightFSM + StartFSM: the state machines are state names here; StartFSM enters the start state
        flightFsm_ = flight;
        fsmStateName_ = fsmStateName_.empty() ? "st_idle" : fsmStateName_;
        fsmStarted_ = true;
        if (fsmStateName_ == "st_idle") EnterIdleState();
        else if (fsmStateName_ == "st_flight_deployed") {
            OnStop.Fire(1.0f);
            if (hooks.assumeDeployedDragCube) hooks.assumeDeployedDragCube(*p);
        }
        if (flight) {
            GE::onVesselWasModified.Add(this, &ModuleProceduralFairing::OnVesselModified);
            onVesselModifiedSubscribed_ = true;
        }
    }
    if (Get<uint32_t>("interstageCraftID") != 0) {
        if (editor) {
            if (hooks.findPartWithCraftID) SetInterstage(hooks.findPartWithCraftID(Get<uint32_t>("interstageCraftID")));
        } else if (scene != GameScenes::MISSIONBUILDER && p->vessel != nullptr) {
            Part* found = nullptr;
            for (Part* q : p->vessel->parts) {
                if (q->fields.craftID == Get<uint32_t>("interstageCraftID") && q->fields.missionID == p->fields.missionID) {
                    found = q;
                    break;
                }
            }
            SetInterstage(found);
        }
    }
    // :433 variants (onVariantApplied -> ApplyVariantToPanel: materials) are the model's
    panelMass_ = f32store(fx(GetFairingArea()) * fx(Get<float>("UnitAreaMass")));
    CheckForFairingEdit();
}

// st_editor_idle.OnEnter (:835) / st_flight_idle.OnEnter (:1074)
void ModuleProceduralFairing::EnterIdleState() {
    if (flightFsm_) {
        if (BaseEvent* ev = Events()["DeployFairing"]) ev->active = !xSections.empty();
    } else {
        const bool a = !xSections.empty();
        if (BaseEvent* ev = Events()["DeleteFairing"]) ev->active = a;
        if (BaseEvent* ev = Events()["EditFairing"]) ev->active = a;
    }
    panelMass_ = f32store(fx(GetFairingArea()) * fx(Get<float>("UnitAreaMass")));
}

// :1366
void ModuleProceduralFairing::SpawnMeshes(bool addColliders) {
    nCollidersPerArc = Mathf::Max(1, Mathf::RoundToInt(f32store(fx(static_cast<float>(Get<int32_t>("nCollidersPerXSection"))) / fx(nArcs()))));
    Panels = FairingPanel::SetupPanelArray(Mathf::RoundToInt(nArcs()), xSections, this, Get<float>("capRadius"));
    const int num = Mathf::RoundToInt(nArcs());
    for (int i = 0, count = static_cast<int>(Panels.size()); i < count; i++) {
        FairingPanel& fairingPanel = *Panels[static_cast<size_t>(i)];
        // fairingPanel.Spawn(modelTransform): the GameObject is the model's
        fairingPanel.BuildMesh(true, (i >= num) ? Panels[static_cast<size_t>(i - num)].get() : nullptr);
        // ApplyVariantToPanel / AddPanelColliders: model
    }
    applyVariantRequired_ = false;
    // :1384-1397 colliders (ClosedColliders / SwitchToConvexColliders): model
    if (cargoModule != nullptr) {
        const float num3 = f32store(fx(xSections.back()->h) * 0.5);
        cargoModule->SetLookupCenter(axis() * num3);
        cargoModule->SetLookupRadius(num3);
        if (interstage_ != nullptr && addColliders) cargoModule->AddConnectingPart(interstage_);
    }
    if (addColliders) {
        // AssumeClosedDragCube (:2043): the attach node's overrideDragArea for an interstage, then the procedural drag cube
        Part* p = PartOf();
        if (interstage_ != nullptr || Get<uint32_t>("interstageCraftID") != 0) {
            float n = f32store(fx(xSections.back()->r) * fx(Get<float>("interstageOcclusionFudge")));
            n = f32store(fx(n) * fx(n));
            if (AttachNode* an = p->FindAttachNode(Get<std::string>("fairingNode"))) an->overrideDragArea = f32store(fx(n) * fx(3.14159274f));
        }
        if (hooks.assumeClosedDragCube) hooks.assumeClosedDragCube(*p);
    }
    for (size_t c = Panels.size(); c-- > 0;) Panels[c]->CleanUp();
}

// :1427
void ModuleProceduralFairing::WipeMesh() {
    Part* p = PartOf();
    if (p != nullptr) {
        // flag children (FlagDecalBackground) are deleted by the editor: not modelled
        if (AttachNode* an = p->FindAttachNode(Get<std::string>("fairingNode"))) an->overrideDragArea = -1.0f;
    }
    // Panels[count].Despawn(): GameObject
    Panels.clear();
    if (cargoModule != nullptr) {
        cargoModule->SetLookupRadius(0.0f);
        cargoModule->ClearConnectingParts();
    }
    applyVariantRequired_ = true;
    CheckForFairingEdit();
}

// :1462
void ModuleProceduralFairing::RefreshPanels() {
    WipeMesh();
    SpawnMeshes(false);
}

// :1468
void ModuleProceduralFairing::OnLoad(const ConfigNode& node) {
    if (node.HasNode("XSECTION")) {
        xSections.clear();
        for (ConfigNode* n : node.GetNodes("XSECTION")) {
            auto x = std::make_shared<FairingXSection>();
            x->Load(*n);
            xSections.push_back(x);
        }
        dotnet::ListSort(xSections, [](const std::shared_ptr<FairingXSection>& a, const std::shared_ptr<FairingXSection>& b) { return a->CompareTo(*b); });
        if (xSections.size() >= 2) xSections.back()->isLast = true;
    }
    if (node.HasValue("fsm")) fsmStateName_ = *node.GetValuePtr("fsm");
}

// :1494
void ModuleProceduralFairing::OnSave(ConfigNode& node) {
    if (fsmStarted_) node.AddValue("fsm", fsmStateName_);
    for (size_t i = 0; i < xSections.size(); i++) xSections[i]->Save(*node.AddNode("XSECTION"));
}

// :1534 (the editor state machine reacts to the mouse; its on_Delete event wipes the mesh and the x-sections)
void ModuleProceduralFairing::DeleteFairing() {
    if (flightFsm_ || fsmStateName_ != "st_idle") return;
    xSections.clear();
    WipeMesh();
    panelMass_ = f32store(fx(GetFairingArea()) * fx(Get<float>("UnitAreaMass")));
    EnterIdleState();
}

// :1552 (fsm.RunEvent(on_Deploy): valid from st_flight_idle)
void ModuleProceduralFairing::DeployFairing() {
    Part* p = PartOf();
    if (!flightFsm_ || fsmStateName_ != "st_idle" || p == nullptr) return;
    // on_Deploy.OnEvent (:1103): ReleasePayload, OnMoving, JettisonPanels, onFairingsDeployed; st_flight_idle.OnLeave first
    if (AttachNode* an = p->FindAttachNode(Get<std::string>("fairingNode"))) an->overrideDragArea = -1.0f;  // ReleasePayload (:1999)
    if (hooks.releasePayload) hooks.releasePayload(*p);
    DumpInterstage();
    if (onVesselModifiedSubscribed_) {
        GE::onVesselWasModified.Remove(this, &ModuleProceduralFairing::OnVesselModified);
        onVesselModifiedSubscribed_ = false;
    }
    if (BaseEvent* ev = Events()["DeployFairing"]) ev->active = false;
    fsmStateName_ = "st_flight_deployed";
    OnMoving.Fire(0.0f, 1.0f);
    JettisonPanels(Panels, Get<float>("ejectionForce"));
    GE::onFairingsDeployed.Fire(p);
    // st_flight_deployed.OnEnter (:1093): OnStop(1), AssumeDeployedDragCube
    OnStop.Fire(1.0f);
    if (AttachNode* an = p->FindAttachNode(Get<std::string>("fairingNode"))) an->overrideDragArea = -1.0f;
    if (hooks.assumeDeployedDragCube) hooks.assumeDeployedDragCube(*p);
}

// :1558
void ModuleProceduralFairing::DeployFairingAction(KSPActionParam& p) {
    if (p.type != static_cast<int>(KSPActionType::Deactivate)) {
        if (!IsDeployed()) DeployFairing();
        else JettisonPanels(Panels, Get<float>("ejectionForce"));
    }
}

// :1573
void ModuleProceduralFairing::OnActive() {
    if (StagingEnabled()) DeployFairing();
}

// :1581
float ModuleProceduralFairing::GetFairingArea() const {
    float num = 0.0f;
    for (size_t count = Panels.size(); count-- > 0;) num = f32store(fx(num) + fx(Panels[count]->GetArea()));
    return num;
}

// :1663
float ModuleProceduralFairing::GetModuleMass(float, ModifierStagingSituation sit) {
    if (sit == ModifierStagingSituation::STAGED) return 0.0f;
    return Mathf::Clamp(panelMass_, Mathf::Epsilon, Mathf::Abs(panelMass_));
}

// :1682
float ModuleProceduralFairing::GetModuleCost(float, ModifierStagingSituation sit) {
    if (sit == ModifierStagingSituation::STAGED) return 0.0f;
    return f32store(fx(GetFairingArea()) * fx(Get<float>("UnitAreaCost")));
}

// :1701
Vector3f ModuleProceduralFairing::GetModuleSize(const Vector3f&, ModifierStagingSituation) { return Vector3f::zero(); }

// :274 GetScalar: 1 unless idle (an editor fairing with x-sections reads as 1 too)
float ModuleProceduralFairing::GetScalar() const {
    const bool flightIdle = flightFsm_ && fsmStateName_ == "st_idle";
    const bool editorIdle = !flightFsm_ && fsmStateName_ == "st_idle";
    if (!flightIdle && !(editorIdle && !xSections.empty())) return 1.0f;
    return 0.0f;
}

// :1711 (the collider containers are the model's)
void ModuleProceduralFairing::SetupOcclusionTest(bool) {}

// :1765 (the physics block: panels become physical objects through hooks; the bookkeeping tail is exact)
void ModuleProceduralFairing::JettisonPanels(std::vector<std::shared_ptr<FairingPanel>>& panels, float ejectionForce) {
    Part* p = PartOf();
    if (panels.empty()) return;
    (void)ejectionForce;
    if (hooks.makePanelPhysical && p != nullptr) {
        const float unitAreaMass = Get<float>("UnitAreaMass");
        for (size_t c = panels.size(); c-- > 0;) hooks.makePanelPhysical(*p, *panels[c], unitAreaMass);
    }
    Panels.clear();
    xSections.clear();
    panelMass_ = f32store(fx(GetFairingArea()) * fx(Get<float>("UnitAreaMass")));
    if (p != nullptr) GE::onVesselWasModified.Fire(p->vessel);
}

// :640
void ModuleProceduralFairing::SetInterstage(Part* iStg) {
    if (!(iStg != interstage_)) return;
    if (iStg == nullptr) {
        DumpInterstage();
        return;
    }
    interstage_ = iStg;
    Set<uint32_t>("interstageCraftID", interstage_->fields.craftID);
}

// :673
void ModuleProceduralFairing::DumpInterstage() {
    interstage_ = nullptr;
    Set<uint32_t>("interstageCraftID", 0u);
    if (Part* p = PartOf())
        if (AttachNode* an = p->FindAttachNode(Get<std::string>("fairingNode"))) an->overrideDragArea = -1.0f;
}

// :737 (on_Breakoff_NoJettison: ReleasePayload, OnMoving, panelMass, onVesselWasModified; the target state is st_flight_idle)
void ModuleProceduralFairing::OnVesselModified(Vessel* v) {
    Part* p = PartOf();
    if (p == nullptr) return;
    if (interstage_ != nullptr) {
        if ((v == p->vessel || v == interstage_->vessel) && p->vessel != interstage_->vessel && p->vessel != FlightRegistry::ActiveVessel &&
            fsmStateName_ == "st_idle" && flightFsm_) {
            if (hooks.releasePayload) hooks.releasePayload(*p);
            DumpInterstage();
            OnMoving.Fire(0.0f, 1.0f);
            panelMass_ = f32store(fx(GetFairingArea()) * fx(Get<float>("UnitAreaMass")));
            GE::onVesselWasModified.Fire(p->vessel);
        }
    } else {
        Set<uint32_t>("interstageCraftID", 0u);
    }
}

// :2031
void ModuleProceduralFairing::DecoupleAction(const std::string& nodeName, bool) {
    Part* p = PartOf();
    if (p == nullptr || !hooks.payloadJointFor) return;
    // payloadAttachNode / payloadJoint exist only while the physics package has secured a payload (SecurePayload, :1982)
    Part* payload = hooks.payloadJointFor(*this);
    if (payload == nullptr) return;
    AttachNode* attachNode = p->FindAttachNode(nodeName);
    if (attachNode != nullptr && attachNode->attachedPart != nullptr && attachNode->attachedPart->fields.persistentId == payload->fields.persistentId) {
        // on_Breakoff: ReleasePayload, SwitchToConvexColliders, OnMoving, JettisonPanels(Panels, 0)
        if (hooks.releasePayload) hooks.releasePayload(*p);
        DumpInterstage();
        OnMoving.Fire(0.0f, 1.0f);
        fsmStateName_ = "st_flight_deployed";
        JettisonPanels(Panels, 0.0f);
        OnStop.Fire(1.0f);
    }
}

// :2122 (the editor-only flag bookkeeping)
void ModuleProceduralFairing::OnChildRemove(Part* child) {
    if (SaveEnvironment::Current().loadedScene == GameScenes::EDITOR) {
        uint32_t flagID = 0u;
        if (child->FindModuleImplementing([](PartModule& m) { return m.IsA("FlagDecalBackground"); }) != nullptr) {
            PartModule* fd = child->FindModuleImplementing([](PartModule& m) { return m.IsA("FlagDecalBackground"); });
            const int32_t placementID = fd->Get<int32_t>("placementID");
            flagID = (placementID == 0) ? child->fields.craftID : static_cast<uint32_t>(placementID);
        }
        for (auto& x : xSections) x->RemoveAttachedFlag(flagID);
        for (auto& pn : Panels) pn->RemoveFlag(flagID);
    }
    CheckForFairingEdit();
}

// :2157 (EditFairing / nArcs / editingBlocked PAW flags)
void ModuleProceduralFairing::CheckForFairingEdit() {
    if (SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT) {
        if (BaseField* f = Fields()["editingBlocked"]) f->guiActive = false;
        return;
    }
    bool blocked = false;
    for (size_t n = 0; n < Panels.size(); n++) {
        if (!Panels[n]->attachedFlagPartsNull && !Panels[n]->attachedFlagParts.empty()) {
            blocked = true;
            break;
        }
    }
    if (BaseEvent* e = Events()["EditFairing"]) e->guiActiveEditor = !blocked;
    if (BaseField* f = Fields()["nArcs"]) f->guiActiveEditor = !blocked;
    if (BaseField* f = Fields()["editingBlocked"]) f->guiActiveEditor = blocked;
}

// :2186 / :2206 (cacheAutoLOC_225016)
std::string ModuleProceduralFairing::GetModuleTitle() { return Localizer::Format("#autoLOC_225016"); }
std::string ModuleProceduralFairing::GetModuleDisplayName() { return Localizer::Format("#autoLOC_225016"); }

// :2196
std::string ModuleProceduralFairing::GetPrimaryField() {
    return Localizer::Format("#autoLOC_6001012", std::vector<std::string>{FormatSingleN(Get<float>("maxRadius"), 2)});
}

// :2201
std::string ModuleProceduralFairing::GetInfo() {
    return Localizer::Format("#autoLOC_6001012", std::vector<std::string>{FormatSingleN(Get<float>("maxRadius"), 2)}) +
           Localizer::Format("#autoLOC_6001013", std::vector<std::string>{FormatSingleN(Get<float>("UnitAreaMass"), 3)}) +
           Localizer::Format("#autoLOC_6001014", std::vector<std::string>{FormatSingleN(Get<float>("UnitAreaCost"), 2)});
}

}  // namespace ksp
