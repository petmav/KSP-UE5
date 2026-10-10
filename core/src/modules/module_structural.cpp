// WP-26: ModuleCargoBay.cs, ModuleJettison.cs, ModuleServiceModule.cs, LaunchClamp.cs, ModuleStructuralNode.cs,
// ModuleStructuralNodeToggle.cs, ModuleDynamicNodes.cs, DynamicNodeSet.cs (scope: module_structural.hpp).
#include "ksp/modules/module_structural.hpp"
#include "ksp/events/event_data.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/modules/module_part_variants.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {
using GE = GameEvents;
Part* PartOfModule(const PartModule& m) { return dynamic_cast<Part*>(m.part); }
bool SceneIs(GameScenes s) { return SaveEnvironment::Current().loadedScene == s; }
bool SceneEditor() { return SceneIs(GameScenes::EDITOR); }
// a string KSPField without initialiser is null after AddComponent (WP-06): string.IsNullOrEmpty semantics
std::string StrField(const PartModule& m, const char* name) { return m.IsNull(name) ? std::string() : m.Get<std::string>(name); }
bool SceneFlight() { return SceneIs(GameScenes::FLIGHT); }
template <class T>
void AddUnique(std::vector<T>& v, const T& x) {
    if (std::find(v.begin(), v.end(), x) == v.end()) v.push_back(x);
}
template <class T>
bool Contains(const std::vector<T>& v, const T& x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}
}  // namespace

// =============================================================== ModuleCargoBay

Part* ModuleCargoBay::PartOf() const { return PartOfModule(*this); }

ModuleCargoBay::~ModuleCargoBay() {
    // ModuleCargoBay.cs:320 OnDestroy
    GE::onEditorShipModified.Remove(this, &ModuleCargoBay::OnEditorVesselModified);
    GE::onVesselWasModified.Remove(this, &ModuleCargoBay::OnVesselModified);
    GE::onPartDie.Remove(this, &ModuleCargoBay::OnPartDestroyed);
    // ModuleCargoBay.cs:326 (WP-27): deployModule.OnMoving / OnStop; skipped when the deploy module died first
    if (deployModule != nullptr && IScalarModule::IsAlive(deployModule)) {
        if (auto* mv = deployModule->OnMoving()) mv->Remove(this, &ModuleCargoBay::OnCargoBayDoorsMoving);
        if (auto* sp = deployModule->OnStop()) sp->Remove(this, &ModuleCargoBay::OnCargoBayDoorsStopped);
    }
    if (SceneFlight() && PartOf() != nullptr) OnPartDestroyed(PartOf());
}

// ModuleCargoBay.cs:108
void ModuleCargoBay::OnAwake() {
    PartModule::OnAwake();
    partsInCargo.clear();
    ownColliders.clear();
    connectingParts.clear();
    connectedCargoBays.clear();
}

// ModuleCargoBay.cs:117
void ModuleCargoBay::OnStart(StartState state) {
    PartModule::OnStart(state);
    if (SceneEditor() || SceneFlight()) {
        Part* p = PartOf();
        deployModule = FindModule(DeployModuleIndex());
        for (size_t i = 0; i < p->Modules().Count(); i++)
            if (p->Modules().GetModule(i)->IsA("ModuleServiceModule")) {
                serviceModule = p->Modules().GetModule(i);
                break;
            }
        FindAttachNodes();
        ScheduleColliderReset();
        GE::onEditorShipModified.Add(this, &ModuleCargoBay::OnEditorVesselModified);
        GE::onVesselWasModified.Add(this, &ModuleCargoBay::OnVesselModified);
        GE::onPartDie.Add(this, &ModuleCargoBay::OnPartDestroyed);
        // deployModule.OnMoving / OnStop (ModuleCargoBay.cs:134-137; WP-27 modules expose the real events, a module without them returns nullptr)
        if (deployModule != nullptr) {
            if (auto* mv = deployModule->OnMoving()) mv->Add(this, &ModuleCargoBay::OnCargoBayDoorsMoving);
            if (auto* sp = deployModule->OnStop()) sp->Add(this, &ModuleCargoBay::OnCargoBayDoorsStopped);
        }
        // onCommandSeatInteraction (KerbalEVA) is the EVA package's
    }
}

// ModuleCargoBay.cs:142
void ModuleCargoBay::OnStartFinished(StartState) {
    if (!colliderResetCoroutineComplete) {
        enableShieldedVolumeAfterColliderReset = true;
        return;
    }
    ColliderListCleanUp();
    if (SceneIs(GameScenes::MISSIONBUILDER)) return;
    if (serviceModule != nullptr) {
        if (static_cast<ModuleServiceModule*>(serviceModule)->IsShrouded()) EnableShieldedVolume();
        else DisableShieldedVolume();
    } else {
        EnableShieldedVolume();
    }
}

// ModuleCargoBay.cs:171
void ModuleCargoBay::FindAttachNodes() {
    Part* p = PartOf();
    auto find = [&](const char* idField, AttachNode*& node, const char* nodeName) {
        const std::string id = StrField(*this, idField);
        if (!id.empty()) {
            node = p->FindAttachNode(id);
            if (node == nullptr) log::Error("[ModuleCargoBay]: Could not find an attach node with id " + id + " for " + nodeName + ".");
        }
    };
    find("nodeOuterForeID", NodeOuterFore, "NodeOuterFore");
    find("nodeOuterAftID", NodeOuterAft, "NodeOuterAft");
    find("nodeInnerForeID", NodeInnerFore, "NodeInnerFore");
    find("nodeInnerAftID", NodeInnerAft, "NodeInnerAft");
}

// ModuleCargoBay.cs:207
void ModuleCargoBay::OnEditorVesselModified(ShipConstruct*) { ScheduleColliderReset(); }

// ModuleCargoBay.cs:212
void ModuleCargoBay::ScheduleColliderReset() {
    Part* p = PartOf();
    if (serviceModule == nullptr) {
        shouldEnableShieldedVolume = SceneEditor() || (SceneFlight() && !p->vessel->packed);
    } else {
        const bool shrouded = static_cast<ModuleServiceModule*>(serviceModule)->IsShrouded();
        shouldEnableShieldedVolume = (SceneEditor() && shrouded) || (SceneFlight() && !p->vessel->packed && shrouded);
    }
    // StartCoroutine(scheduledColliderResetCoroutine(0.5f, ...)): enableShieldedVolumeAfterColliderReset is set at once, the reset runs
    // when the host calls RunScheduledColliderReset (the coroutine's body after the delay)
    enableShieldedVolumeAfterColliderReset = shouldEnableShieldedVolume;
    colliderResetPending = true;
}

// ModuleCargoBay.cs:225-241 (after the delay)
void ModuleCargoBay::RunScheduledColliderReset() {
    if (!colliderResetPending) return;
    colliderResetPending = false;
    ColliderReset(enableShieldedVolumeAfterColliderReset);
    colliderResetCoroutineComplete = true;
}

// ModuleCargoBay.cs:242
void ModuleCargoBay::ColliderReset(bool enableShieldedVolume) {
    ownColliders.clear();
    connectedCargoBays.clear();
    FindBayColliders(PartOf(), this, ownColliders, connectedCargoBays);
    if (!SceneIs(GameScenes::MISSIONBUILDER) && enableShieldedVolume) EnableShieldedVolume();
}

// ModuleCargoBay.cs:253
void ModuleCargoBay::OnVesselModified(Vessel* v) {
    Part* p = PartOf();
    bool near = false;
    if (v != p->vessel && hooks.vesselPosition) {
        const Vector3f d = hooks.vesselPosition(*v) - p->partTransform.TransformPoint(Get<Vector3f>("lookupCenter"));
        const float r = Get<float>("lookupRadius");
        near = d.sqrMagnitude() < f32store(fx(r) * fx(r));
    }
    if (v == p->vessel || near) {
        ColliderListCleanUp();
        EnableShieldedVolume();
    }
}

// ModuleCargoBay.cs:284
void ModuleCargoBay::OnCargoBayDoorsMoving(float from, float to) {
    if (from != to || from != Get<float>("closedPosition")) NotifyShieldModified(true);
}

// ModuleCargoBay.cs:293
void ModuleCargoBay::OnCargoBayDoorsStopped(float /*at*/) {
    EnableShieldedVolume();
    for (size_t count = connectedCargoBays.size(); count-- > 0;) connectedCargoBays[count]->EnableShieldedVolume();
    NotifyShieldModified(true);
}

// ModuleCargoBay.cs:312
void ModuleCargoBay::OnPartDestroyed(Part* p) {
    if (p == PartOf()) UnShieldEnclosedParts(partsInCargo);
}

// ModuleCargoBay.cs:337
void ModuleCargoBay::ColliderListCleanUp() {
    for (size_t count = ownColliders.size(); count-- > 0;) {
        if (ownColliders[count].collider == nullptr || ownColliders[count].owner == nullptr || ownColliders[count].owner->vessel != PartOf()->vessel)
            ownColliders.erase(ownColliders.begin() + static_cast<std::ptrdiff_t>(count));
    }
}

// ModuleCargoBay.cs:349
IScalarModule* ModuleCargoBay::FindModule(int index) {
    Part* p = PartOf();
    PartModule* m = p->Modules().GetModule(static_cast<size_t>(index));
    if (m->AsScalarModule() != nullptr) return m->AsScalarModule();
    log::Error("[ModuleCargoBay]: Module " + m->moduleName + " is not an IScalarModule");
    return nullptr;
}

// ModuleCargoBay.cs:457
void ModuleCargoBay::EnableShieldedVolume() {
    if (!partsInCargo.empty()) partsInCargoPrev = partsInCargo;
    DisableShieldedVolume();
    SetupDynamicCargoOccluders(true);
    std::vector<Part*> nearby;
    if (hooks.findNearbyParts) nearby = hooks.findNearbyParts(*this, Get<float>("lookupRadius"), Get<Vector3f>("lookupCenter"));
    partsInCargo = hooks.findEnclosedParts ? hooks.findEnclosedParts(*this, nearby, ownColliders) : std::vector<Part*>{};
    SetupDynamicCargoOccluders(false);
    onShdModifiedCallbacks = ShieldEnclosedParts(partsInCargo);
    if (partsInCargoPrev.empty()) return;
    for (size_t count = partsInCargoPrev.size(); count-- > 0;) {
        Part* part = partsInCargoPrev[count];
        if (part != nullptr && !Contains(partsInCargo, part)) part->RemoveShield(this);
    }
}

// ModuleCargoBay.cs:483
void ModuleCargoBay::DisableShieldedVolume() {
    if (!partsInCargo.empty()) {
        UnShieldEnclosedParts(partsInCargo);
        partsInCargo.clear();
        onShdModifiedCallbacks.clear();
    }
}

// ModuleCargoBay.cs:493
void ModuleCargoBay::SetupDynamicCargoOccluders(bool testActive) {
    std::vector<Part*> list;
    bool have = false;
    if (SceneFlight()) {
        list = PartOf()->vessel->parts;
        have = true;
    } else if (SceneEditor() && hooks.editorShipParts) {
        list = hooks.editorShipParts();
        have = true;
    }
    if (!have) return;
    for (Part* part : list)
        for (size_t j = 0; j < part->Modules().Count(); j++)
            if (auto* occ = dynamic_cast<IDynamicCargoOccluder*>(part->Modules().GetModule(j))) occ->SetupOcclusionTest(testActive);
}

// ModuleCargoBay.cs:522 (the returned "callbacks" are Part.OnShieldModified of the enclosed parts: held as the parts)
std::vector<Part*> ModuleCargoBay::ShieldEnclosedParts(std::vector<Part*>& enclosedParts) {
    std::vector<Part*> list;
    for (size_t count = enclosedParts.size(); count-- > 0;) {
        enclosedParts[count]->AddShield(this);
        AddUnique(list, enclosedParts[count]);
    }
    return list;
}

// ModuleCargoBay.cs:533
void ModuleCargoBay::UnShieldEnclosedParts(std::vector<Part*>& enclosedParts) {
    for (size_t count = enclosedParts.size(); count-- > 0;) {
        onShdModifiedCallbacks.erase(onShdModifiedCallbacks.begin() + static_cast<std::ptrdiff_t>(count));  // RemoveAt (throws out_of_range)
        if (enclosedParts[count] != nullptr) enclosedParts[count]->RemoveShield(this);
        enclosedParts.erase(enclosedParts.begin() + static_cast<std::ptrdiff_t>(count));
    }
}

// ModuleCargoBay.cs:551
void ModuleCargoBay::ModifyShieldInEnclosedParts(bool shieldStatus) {
    if (shieldStatus) ShieldEnclosedParts(partsInCargo);
    else UnShieldEnclosedParts(partsInCargo);
}

// ModuleCargoBay.cs:563
void ModuleCargoBay::NotifyShieldModified(bool notifyConnected) {
    for (size_t count = onShdModifiedCallbacks.size(); count-- > 0;) {
        if (partsInCargo.at(count) != nullptr) {
            onShdModifiedCallbacks[count]->OnShieldModified(this);
            continue;
        }
        partsInCargo.erase(partsInCargo.begin() + static_cast<std::ptrdiff_t>(count));
        onShdModifiedCallbacks.erase(onShdModifiedCallbacks.begin() + static_cast<std::ptrdiff_t>(count));
    }
    if (notifyConnected)
        for (size_t count2 = connectedCargoBays.size(); count2-- > 0;) connectedCargoBays[count2]->NotifyShieldModified(false);
}

// ModuleCargoBay.cs:619
void ModuleCargoBay::FindBayColliders(Part* originalPart, ModuleCargoBay* origin, std::vector<PartCollider>& cList, std::vector<ModuleCargoBay*>& cBays) {
    auto addRange = [&](const std::vector<PartCollider>& r) {
        for (const auto& c : r) AddUnique(cList, c);
    };
    auto partColliders = [&](Part* p) { return hooks.findPartColliders ? hooks.findPartColliders(*p) : std::vector<PartCollider>{}; };
    auto bayOf = [](Part* p) {
        return dynamic_cast<ModuleCargoBay*>(p->FindModuleImplementing([](PartModule& m) { return m.IsA("ModuleCargoBay"); }));
    };
    addRange(partColliders(PartOf()));
    for (size_t i = 0, count = connectingParts.size(); i < count; i++) {
        Part* part = connectingParts[i];
        if (part == originalPart) continue;
        if (ModuleCargoBay* bay = bayOf(part)) {
            AddUnique(cBays, bay);
            bay->FindBayColliders(originalPart, this, cList, cBays);
        } else {
            addRange(partColliders(part));
        }
    }
    std::vector<AttachNode*> list = FindConnectingNodes(origin->PartOf());
    for (size_t j = 0, count2 = list.size(); j < count2; j++) {
        AttachNode* attachNode = list[j];
        if (attachNode->attachedPart != nullptr && !Contains(connectingParts, attachNode->attachedPart)) {
            if (ModuleCargoBay* bay = bayOf(attachNode->attachedPart)) {
                AddUnique(cBays, bay);
                bay->FindBayColliders(originalPart, this, cList, cBays);
            } else {
                addRange(partColliders(attachNode->attachedPart));
            }
        }
    }
}

// ModuleCargoBay.cs:661
std::vector<AttachNode*> ModuleCargoBay::FindConnectingNodes(Part* origin) {
    std::vector<AttachNode*> list;
    if (origin != PartOf()) {
        if (AttachNode* opposingNode = GetOpposingNode(origin)) AddUnique(list, opposingNode);
    } else {
        if (NodeOuterAft != nullptr) AddUnique(list, NodeOuterAft);
        if (NodeOuterFore != nullptr) AddUnique(list, NodeOuterFore);
    }
    return list;
}

// ModuleCargoBay.cs:686
AttachNode* ModuleCargoBay::GetOpposingNode(Part* origin) {
    AttachNode* attachNode = PartOf()->FindAttachNodeByPart(origin);
    if (attachNode != nullptr) {
        if (attachNode == NodeOuterAft || attachNode == NodeInnerFore) return NodeOuterFore;
        if (attachNode == NodeInnerAft || attachNode == NodeOuterFore) return NodeOuterAft;
    }
    return nullptr;
}

// ModuleCargoBay.cs:703
void ModuleCargoBay::AddConnectingPart(Part* p) {
    if (!Contains(connectingParts, p)) {
        connectingParts.push_back(p);
        ScheduleColliderResetForAllBays();
    }
}

// ModuleCargoBay.cs:712
void ModuleCargoBay::RemoveConnectingPart(Part* p) {
    auto it = std::find(connectingParts.begin(), connectingParts.end(), p);
    if (it != connectingParts.end()) {
        connectingParts.erase(it);
        ScheduleColliderResetForAllBays();
    }
}

// ModuleCargoBay.cs:721
void ModuleCargoBay::ClearConnectingParts() {
    if (!connectingParts.empty()) {
        connectingParts.clear();
        ScheduleColliderResetForAllBays();
    }
}

// ModuleCargoBay.cs:730
void ModuleCargoBay::ScheduleColliderResetForAllBays() {
    ScheduleColliderReset();
    for (size_t count = connectedCargoBays.size(); count-- > 0;) connectedCargoBays[count]->ScheduleColliderReset();
}

// ModuleCargoBay.cs:740 / :745
void ModuleCargoBay::SetLookupRadius(float radius) { Set<float>("lookupRadius", radius); }
void ModuleCargoBay::SetLookupCenter(const Vector3f& p) { Set<Vector3f>("lookupCenter", p); }

// ModuleCargoBay.cs:750
bool ModuleCargoBay::ClosedAndLocked() {
    if (!SelfClosedAndLocked()) return false;
    for (size_t count = connectedCargoBays.size(); count-- > 0;)
        if (!connectedCargoBays[count]->SelfClosedAndLocked()) return false;
    return true;
}

// ModuleCargoBay.cs:768
bool ModuleCargoBay::SelfClosedAndLocked() {
    return deployModule != nullptr && EndCapped() && deployModule->GetScalar() == Get<float>("closedPosition") && !deployModule->IsMoving();
}

// ModuleCargoBay.cs:777
bool ModuleCargoBay::EndCapped() {
    if (NodeOuterAft != nullptr) {
        if (NodeOuterAft->attachedPart == nullptr) return false;
        if (!TestAttachmentFit(NodeOuterAft)) return false;
    }
    if (NodeOuterFore != nullptr) {
        if (NodeOuterFore->attachedPart == nullptr) return false;
        if (!TestAttachmentFit(NodeOuterFore)) return false;
    }
    return true;
}

// ModuleCargoBay.cs:804
bool ModuleCargoBay::TestAttachmentFit(AttachNode* n) {
    AttachNode* attachNode = n->FindOpposingNode();
    if (attachNode != nullptr) return n->size <= attachNode->size;
    return false;
}

// ModuleCargoBay.cs:814 / :819
Vessel* ModuleCargoBay::GetVessel() { return PartOf()->vessel; }
Part* ModuleCargoBay::GetPart() { return PartOf(); }

// ModuleCargoBay.cs:938
std::string ModuleCargoBay::GetModuleDisplayName() { return Localizer::Format("#autoLOC_8004213"); }

// =============================================================== ModuleJettison

Part* ModuleJettison::PartOf() const { return PartOfModule(*this); }
namespace {
std::vector<const ModuleJettison*>& LiveJettisons() {
    static std::vector<const ModuleJettison*> v;
    return v;
}
}  // namespace

ModuleJettison::ModuleJettison(const ModuleClassInfo& cls) : PartModule(cls) { LiveJettisons().push_back(this); }
bool ModuleJettison::IsAlive(const ModuleJettison* m) { return std::find(LiveJettisons().begin(), LiveJettisons().end(), m) != LiveJettisons().end(); }


ModuleJettison::~ModuleJettison() {
    LiveJettisons().erase(std::remove(LiveJettisons().begin(), LiveJettisons().end(), this), LiveJettisons().end());
    // ModuleJettison.cs:519 OnDestroy
    if (variantSubscribed_) GE::onVariantApplied.Remove(this, &ModuleJettison::OnVariantApplied);
}

void ModuleJettison::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("Jettison", BaseEvent::Handler([this] { Jettison(); }));
    Actions().Bind("JettisonAction", [this](KSPActionParam& p) { JettisonAction(p); });
}

// ModuleJettison.cs:130
void ModuleJettison::JettisonAction(KSPActionParam&) { Jettison(); }

// ModuleJettison.cs:136
void ModuleJettison::Jettison() {
    Part* p = PartOf();
    if (Get<bool>("checkBottomNode")) {
        AttachNode* attachNode = p->FindAttachNode(Get<std::string>("bottomNodeName"));
        if (attachNode != nullptr && attachNode->attachedPart != nullptr) {
            log::Warning("Checked bottom node and found an attached part. Unable to Jettison");
            return;
        }
    }
    if (!Get<bool>("isJettisoned") && jettisonTransformPresent_) {
        if (!p->fields.partRendererBoundsIgnore) p->fields.partRendererBoundsIgnore = std::vector<std::string>();
        p->fields.partRendererBoundsIgnore->push_back(jettisonTransformName_);
        const bool flag = false;  // the transform's parent is a ModuleDockingNode: the model's (docking port shrouds)
        if (!Get<bool>("isFairing") || flag) {
            OnMoving.Fire(0.0f, 1.0f);
            if (hooks.jettisonTransformToPhysical) hooks.jettisonTransformToPhysical(*p, *this);
        }
    }
    Set<bool>("isJettisoned", true);
    SetFairingDragCube(false);
    OnStop.Fire(1.0f);
    if (BaseEvent* ev = Events()["Jettison"]) ev->active = false;
}

// ModuleJettison.cs:233
void ModuleJettison::OnVariantApplied(Part*, PartVariant*) { CheckAttachNode(); }

// ModuleJettison.cs:238
void ModuleJettison::OnLoad(const ConfigNode& node) {
    PartModule::OnLoad(node);
    if (BaseField* f = Fields()["shroudHideOverride"]) f->guiActiveEditor = Get<bool>("allowShroudToggle");
}

// ModuleJettison.cs:244 (the delayed coroutine of the flight scene is run at once)
void ModuleJettison::OnInventoryModeDisable() {
    SetVariants();
    SetupJettisonState(false);
}

// ModuleJettison.cs:261: the first listed transform that exists and is active becomes jettisonTransform (and activejettisonName)
bool ModuleJettison::FetchActiveMeshTransform(const std::string& nameList) {
    Part* p = PartOf();
    const std::vector<std::string> array = dotnet::Split(nameList, ",", false);
    for (const std::string& name : array) {
        const bool exists = StructuralModel().hasModelTransform && StructuralModel().hasModelTransform(*p, name);
        if (exists && StructuralModel().modelTransformActive && StructuralModel().modelTransformActive(*p, name)) {
            Set<std::string>("activejettisonName", name);
            jettisonTransformPresent_ = true;
            jettisonTransformName_ = name;
            return true;
        }
    }
    return jettisonTransformPresent_;  // `return jettisonTransform` (unchanged)
}

// ModuleJettison.cs:285
void ModuleJettison::LoadJettisonMeshState() {
    if (Get<std::string>("activejettisonName").empty()) FetchActiveMeshTransform(Get<std::string>("jettisonName"));
    else FetchActiveMeshTransform(Get<std::string>("activejettisonName"));
}

// ModuleJettison.cs:297
void ModuleJettison::OnStart(StartState state) {
    PartModule::OnStart(state);
    Part* p = PartOf();
    SetVariants();
    GE::onVariantApplied.Add(this, &ModuleJettison::OnVariantApplied);
    variantSubscribed_ = true;
    const std::string menuName = Get<std::string>("menuName");
    if (!menuName.empty()) {
        if (BaseEvent* e = Events()["Jettison"]) e->SetGuiName(menuName);
        if (BaseAction* a = Actions()["JettisonAction"]) a->SetGuiName(menuName);
    }
    if (BaseField* f = Fields()["shroudHideOverride"]) f->guiActiveEditor = Get<bool>("allowShroudToggle");
    Set<bool>("isJettisoned", Get<bool>("isJettisoned") || Get<bool>("shroudHideOverride"));
    if (state == StartState::Editor) {
        if (jettisonTransformPresent_ && !Get<bool>("ignoreNodes") && StructuralModel().setModelTransformActive)
            StructuralModel().setModelTransformActive(*p, jettisonTransformName_, false);
        inEditor_ = true;
    } else {
        inEditor_ = false;
    }
    if (BaseEvent* e = Events()["Jettison"]) e->active = false;
    const Vector3f partVolume = StructuralModel().partColliderBoundsSize ? StructuralModel().partColliderBoundsSize(*p) : Vector3f::zero();
    partDensity_ = f32store(fx(partVolume.x) * fx(partVolume.y) * fx(partVolume.z) / fx(p->fields.mass));
    if (!Get<std::string>("actionSuffix").empty()) SetActionsSuffix();
}

// ModuleJettison.cs:329
void ModuleJettison::SetVariants() {
    Part* p = PartOf();
    if (p->variants != nullptr && static_cast<ModulePartVariants*>(p->variants)->IsMultipleCubesActive()) hasVariantDragCubes_ = true;
}

// ModuleJettison.cs:337
void ModuleJettison::SetActionsSuffix() {
    const std::string suffix = Get<std::string>("actionSuffix");
    if (BaseAction* a = Actions()["JettisonAction"]) a->SetGuiName(a->guiName() + " (" + suffix + ")");
    if (BaseEvent* e = Events()["Jettison"]) e->SetGuiName(e->guiName() + " (" + suffix + ")");
    if (BaseField* f = Fields()["shroudHideOverride"]) f->guiName += " (" + suffix + ")";
}

// ModuleJettison.cs:344 (the collider blocks are the model's)
void ModuleJettison::SetupJettisonState(bool isEditor) {
    Part* p = PartOf();
    LoadJettisonMeshState();
    const bool ignoreNodes = Get<bool>("ignoreNodes");
    auto setActive = [&](bool v) {
        if (StructuralModel().setModelTransformActive) StructuralModel().setModelTransformActive(*p, jettisonTransformName_, v);
    };
    if (!isEditor && !ignoreNodes) {
        AttachNode* an = p->FindAttachNode(Get<std::string>("bottomNodeName"));
        if (an == nullptr) throw NullReferenceException();
        if ((an->attachedPart == nullptr && !ignoreNodes) || Get<bool>("isJettisoned")) {
            if (jettisonTransformPresent_ && !ignoreNodes) setActive(false);
            SetFairingDragCube(false);
        } else if (!Get<bool>("isJettisoned")) {
            if (jettisonTransformPresent_) setActive(true);
            SetFairingDragCube(true);
        }
    }
    if (jettisonTransformPresent_ && Get<bool>("isJettisoned")) setActive(false);
}

// ModuleJettison.cs:451
void ModuleJettison::OnActive() {
    if (!Get<bool>("manualJettison") && !Get<bool>("isJettisoned") && Get<bool>("stagingEnabled")) Jettison();
}

// ModuleJettison.cs:474
void ModuleJettison::LateUpdate() {
    if (!setupComplete_) {
        SetupJettisonState(inEditor_);
        setupComplete_ = true;
    }
    if (inEditor_) CheckAttachNode();
    if (SceneFlight()) {
        BaseEvent* ev = Events()["Jettison"];
        if (ev != nullptr)
            ev->active = !Get<bool>("isJettisoned") && !Get<bool>("shroudHideOverride") && !Get<bool>("hideJettisonMenu") && !Get<bool>("isFairing");
    }
}

// ModuleJettison.cs:498
void ModuleJettison::CheckAttachNode() {
    Part* p = PartOf();
    FetchActiveMeshTransform(Get<std::string>("jettisonName"));
    AttachNode* attachNode = p->FindAttachNode(Get<std::string>("bottomNodeName"));
    const bool ignoreNodes = Get<bool>("ignoreNodes");
    auto setActive = [&](bool v) {
        if (StructuralModel().setModelTransformActive) StructuralModel().setModelTransformActive(*p, jettisonTransformName_, v);
    };
    if ((attachNode != nullptr && attachNode->attachedPart == nullptr && !ignoreNodes) || Get<bool>("shroudHideOverride")) {
        Set<bool>("isJettisoned", true);
        if (jettisonTransformPresent_ && !ignoreNodes) {
            setActive(false);
            SetFairingDragCube(false);
        }
    } else if (jettisonTransformPresent_) {
        Set<bool>("isJettisoned", false);
        setActive(true);
        SetFairingDragCube(true);
    }
}

// ModuleJettison.cs:524
void ModuleJettison::SetFairingDragCube(bool fairingState) {
    if (!IsMultipleCubesActive()) return;
    Part* p = PartOf();
    auto* variants = static_cast<ModulePartVariants*>(p->variants);
    if (fairingState) {
        if (hasVariantDragCubes_) variants->SetCubes(false);
        else p->DragCubes.SetCubeWeight("Clean", 0.0f);
        p->DragCubes.SetCubeWeight("Fairing", 1.0f);
    } else if (hasVariantDragCubes_) {
        variants->SetCubes(true);
    } else {
        p->DragCubes.SetCubeWeight("Fairing", 0.0f);
        p->DragCubes.SetCubeWeight("Clean", 1.0f);
    }
    bool resetProcTiming = true;
    if (SceneEditor()) resetProcTiming = !PhysicsGlobals::AeroDataDisplay();
    p->DragCubes.ForceUpdate(true, true, resetProcTiming);
}

// ModuleJettison.cs:564
void ModuleJettison::AssumeDragCubePosition(const std::string& name) {
    Part* p = PartOf();
    std::string text = Get<std::string>("activejettisonName");
    if (text.empty()) text = Get<std::string>("jettisonName");
    FetchActiveMeshTransform(text);
    if (!jettisonTransformPresent_) return;
    if (name != "Fairing") {
        if (name == "Clean" && StructuralModel().setModelTransformActive) StructuralModel().setModelTransformActive(*p, jettisonTransformName_, false);
    } else if (StructuralModel().setModelTransformActive) {
        StructuralModel().setModelTransformActive(*p, jettisonTransformName_, true);
    }
}

// ModuleJettison.cs:594
void ModuleJettison::DecoupleAction(const std::string& nodeName, bool) {
    if (!Get<bool>("isJettisoned") && !Get<bool>("shroudHideOverride") && Get<bool>("decoupleEnabled") && nodeName == Get<std::string>("bottomNodeName") &&
        !Get<bool>("ignoreNodes"))
        Jettison();
}

// ModuleJettison.cs:602
float ModuleJettison::GetModuleMass(float, ModifierStagingSituation) {
    if (!Get<bool>("modifyJettisonedMass")) return 0.0f;
    if (Get<bool>("isJettisoned")) return Get<bool>("useCalculatedMass") ? -partDensity_ : -Get<float>("jettisonedObjectMass");
    return 0.0f;
}

// ModuleJettison.cs:641
bool ModuleJettison::IsStageable() {
    if (!Get<bool>("stagingEnabled") && !Get<bool>("stagingToggleEnabledEditor")) return Get<bool>("stagingToggleEnabledFlight");
    return true;
}

// ModuleJettison.cs:650 / :659
std::string ModuleJettison::GetStagingEnableText() {
    const std::string t = Get<std::string>("stagingEnableText");
    if (!t.empty()) return t;
    return Localizer::Format("#autoLOC_243358");
}
std::string ModuleJettison::GetStagingDisableText() {
    const std::string t = Get<std::string>("stagingDisableText");
    if (!t.empty()) return t;
    return Localizer::Format("#autoLOC_243359");
}

// =============================================================== ModuleServiceModule

Part* ModuleServiceModule::PartOf() const { return PartOfModule(*this); }

ModuleServiceModule::~ModuleServiceModule() { GE::onVesselSwitching.Remove(this, &ModuleServiceModule::CheckCutaway); }  // :166 OnDestroy

// ModuleServiceModule.cs:72
void ModuleServiceModule::OnStart(StartState state) {
    PartModule::OnStart(state);
    Setup();
}

// ModuleServiceModule.cs:77
void ModuleServiceModule::OnFieldUpdated(const std::string& fieldName) {
    if (fieldName != "showMesh") {
        if (fieldName == "openCutaway" && !Get<bool>("IsDeployed")) ToggleCutaway(Get<bool>("openCutaway"));
    } else {
        Set<bool>("IsDeployed", !Get<bool>("showMesh"));
        ToggleShrouds();
        UpdateStaging();
    }
}

// ModuleServiceModule.cs:95
void ModuleServiceModule::ToggleCutaway(bool state) { ProcessJettisonModules(!state, false); }

// ModuleServiceModule.cs:109 (modAction = UpdateCutaway :100 or UpdateShrouds :154)
void ModuleServiceModule::ProcessJettisonModules(bool state, bool updateShrouds) {
    Part* p = PartOf();
    if (jettisonModulesNull) SetupJettisonModules();
    for (size_t i = 0; i < jettisonModules.size(); i++) {
        ModuleJettison* j = jettisonModules[i];
        if (!j->HasJettisonTransform()) continue;
        auto setActive = [&](bool v) {
            if (StructuralModel().setModelTransformActive) StructuralModel().setModelTransformActive(*p, j->JettisonTransformName(), v);
        };
        if (updateShrouds) {
            setActive(state);
            j->Set<bool>("shroudHideOverride", !state);
        } else if (!j->Get<bool>("isJettisoned") && !j->Get<bool>("shroudHideOverride")) {
            setActive(state);
            // ToggleShellMesh(state): the shell transform is the model's
            const std::string shell = StrField(*this, "ShellMeshName");
            if (!shell.empty() && StructuralModel().setModelTransformActive && StructuralModel().hasModelTransform && StructuralModel().hasModelTransform(*p, shell))
                StructuralModel().setModelTransformActive(*p, shell, state);
        }
    }
    // SetExteriorCollider(state): the exterior collider is the model's
    const std::string ext = StrField(*this, "ExteriorColliderName");
    if (!ext.empty() && StructuralModel().setModelTransformActive && StructuralModel().hasModelTransform && StructuralModel().hasModelTransform(*p, ext))
        StructuralModel().setModelTransformActive(*p, ext, state);
}

// ModuleServiceModule.cs:127
void ModuleServiceModule::ToggleShrouds() {
    Part* p = PartOf();
    bool flag = Get<bool>("showMesh");
    if (Get<bool>("IsDeployed")) flag = false;
    ModuleCargoBay* bay = dynamic_cast<ModuleCargoBay*>(p->FindModuleImplementing([](PartModule& m) { return m.IsA("ModuleCargoBay"); }));
    if (bay != nullptr) bay->ModifyShieldInEnclosedParts(flag);
    ProcessJettisonModules(flag, true);
    if (SceneEditor()) {
        const std::string shell = StrField(*this, "ShellMeshName");
        if (!shell.empty() && StructuralModel().setModelTransformActive && StructuralModel().hasModelTransform && StructuralModel().hasModelTransform(*p, shell))
            StructuralModel().setModelTransformActive(*p, shell, flag);
    }
}

// ModuleServiceModule.cs:160
void ModuleServiceModule::SetupJettisonModules() {
    jettisonModules.clear();
    jettisonModulesNull = false;
    Part* p = PartOf();
    for (size_t i = 0; i < p->Modules().Count(); i++)
        if (auto* j = dynamic_cast<ModuleJettison*>(p->Modules().GetModule(i))) jettisonModules.push_back(j);
}

// ModuleServiceModule.cs:171
void ModuleServiceModule::Setup() {
    Part* p = PartOf();
    GE::onVesselSwitching.Add(this, &ModuleServiceModule::CheckCutaway);
    if (p->fields.stagingIcon && p->fields.stagingIcon->empty() && Get<bool>("overrideStagingIconIfBlank")) p->fields.stagingIcon = std::string("FUEL_TANK");
    if (BaseField* f = Fields()["openCutaway"]) f->guiActive = !Get<bool>("IsDeployed");
    if (SceneEditor() || !Get<bool>("showMesh") || Get<bool>("IsDeployed")) {
        if (BaseField* f = Fields()["openCutaway"]) {
            f->guiActiveEditor = false;
            f->guiActive = false;
        }
    }
    SetupJettisonModules();
    if (!Get<bool>("IsDeployed")) ToggleCutaway(Get<bool>("openCutaway"));
    ToggleShrouds();
    UpdateStaging();
}

// ModuleServiceModule.cs:197
void ModuleServiceModule::CheckCutaway(Vessel* vFrom, Vessel*) {
    if (!(PartOf()->vessel == vFrom) && !Get<bool>("IsDeployed") && Get<bool>("showMesh")) ToggleCutaway(Get<bool>("openCutaway"));
    else ToggleCutaway(!Get<bool>("showMesh") || Get<bool>("IsDeployed"));
}

// ModuleServiceModule.cs:209
void ModuleServiceModule::OnActive() {
    if (Get<bool>("showMesh") && !Get<bool>("IsDeployed")) {
        Set<bool>("openCutaway", false);
        ToggleCutaway(false);
        ShedExternalParts();
        // SetExteriorCollider(false)
        if (Part* p = PartOf()) {
            const std::string ext = StrField(*this, "ExteriorColliderName");
            if (!ext.empty() && StructuralModel().setModelTransformActive && StructuralModel().hasModelTransform && StructuralModel().hasModelTransform(*p, ext))
                StructuralModel().setModelTransformActive(*p, ext, false);
        }
        UpdateStaging(false);
        Set<bool>("IsDeployed", true);
        for (size_t i = 0; i < jettisonModules.size(); i++) jettisonModules[i]->Jettison();
        if (BaseField* f = Fields()["openCutaway"]) f->guiActive = !Get<bool>("IsDeployed");
        OnStop.Fire(1.0f);
    } else {
        ToggleShrouds();
    }
}

// ModuleServiceModule.cs:233
void ModuleServiceModule::ShedExternalParts() {
    BuildJettisonPartsList();
    for (size_t i = 0; i < jettisonParts.size(); i++) jettisonParts[i]->decouple();
}

// ModuleServiceModule.cs:242
void ModuleServiceModule::BuildJettisonPartsList() {
    Part* p = PartOf();
    jettisonParts.clear();
    std::vector<BoxZone> list;
    if (Get<bool>("UseJettisonZones")) {
        for (const std::string& zone : dotnet::Split(StrField(*this, "JettisonZoneNames"), ",", false)) {
            if (p == nullptr || !hooks.findZone) continue;
            if (auto z = hooks.findZone(*p, zone)) list.push_back(*z);
        }
    }
    for (size_t count = p->children.size(); count-- > 0;) {
        Part* part = p->children[count];
        if (!(part->srfAttachNode->attachedPart == p) || part->ShieldedFromAirstream()) continue;
        bool flag = true;
        if (Get<bool>("UseJettisonZones")) {
            bool flag2 = false;
            const Vector3f pos = hooks.partWorldPosition ? hooks.partWorldPosition(*part) : part->partTransform.position;
            for (const BoxZone& box : list) {
                const Vector3f local = box.inverseTransformPoint ? box.inverseTransformPoint(pos) : pos;
                if (PointInBox(local, box.center, box.size)) {
                    flag2 = true;
                    break;
                }
            }
            flag = flag2;
        }
        if (flag) jettisonParts.push_back(part);
    }
}

// ModuleServiceModule.cs:296 (`point` is already box.transform.InverseTransformPoint(point))
bool ModuleServiceModule::PointInBox(const Vector3f& pointLocal, const Vector3f& center, const Vector3f& size) {
    const Vector3f point = pointLocal - center;
    const float num = f32store(fx(size.x) * 0.5);
    const float num2 = f32store(fx(size.y) * 0.5);
    const float num3 = f32store(fx(size.z) * 0.5);
    return point.x < num && point.x > -num && point.y < num2 && point.y > -num2 && point.z < num3 && point.z > -num3;
}

// ModuleServiceModule.cs:317
void ModuleServiceModule::UpdateStaging() {
    bool state = Get<bool>("showMesh");
    if (Get<bool>("IsDeployed")) state = false;
    UpdateStaging(state);
}

// ModuleServiceModule.cs:327
void ModuleServiceModule::UpdateStaging(bool state) {
    Set<bool>("stagingEnabled", state);
    SetModuleAttributesIsStageable(state);
    if (jettisonModulesNull) SetupJettisonModules();
    for (size_t i = 0; i < jettisonModules.size(); i++) {
        jettisonModules[i]->Set<bool>("stagingEnabled", state);
        jettisonModules[i]->SetModuleAttributesIsStageable(state);
    }
    PartOf()->UpdateStageability(true, true);
}

// ModuleServiceModule.cs:345
int ModuleServiceModule::GetStageIndex(int) { return PartOf()->fields.inverseStage; }

// ModuleServiceModule.cs:350
bool ModuleServiceModule::PartDetaches(std::vector<Part*>& decoupledParts) {
    BuildJettisonPartsList();
    decoupledParts = jettisonParts;
    return Get<bool>("partDecoupled");
}

// =============================================================== LaunchClamp

Part* LaunchClamp::PartOf() const { return PartOfModule(*this); }

void LaunchClamp::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("Release", BaseEvent::Handler([this] { Release(); }));
    Actions().Bind("ReleaseClamp", [this](KSPActionParam& p) { ReleaseClamp(p); });
}

// LaunchClamp.cs:60
void LaunchClamp::OnLoad(const ConfigNode& node) {
    if (node.HasValue("towerRot")) towerRotation = ConfigNode::ParseQuaternion(*node.GetValuePtr("towerRot"));
}

// LaunchClamp.cs:68
void LaunchClamp::OnSave(ConfigNode& node) { node.AddValue("towerRot", ConfigNode::WriteQuaternion(towerRotation)); }

// LaunchClamp.cs:95
void LaunchClamp::OnPutToGround(PartHeightQuery& qr) {
    Part* p = PartOf();
    qr.lowestOnParts[p] = 0.0f;
    if (qr.lowestPoint < qr.lowestOnParts[p]) {
        Set<float>("height", f32store(fx(Get<float>("height")) + (fx(qr.lowestOnParts[p]) - fx(qr.lowestPoint))));
        initialHeight = -1.0f;
    } else {
        qr.lowestPoint = qr.lowestOnParts[p];
    }
}

// LaunchClamp.cs:152
void LaunchClamp::OnInitialize() { PartOf()->fields.PermanentGroundContact = true; }

// LaunchClamp.cs:157 (the tower transforms, animation and ground raycast are the model's; the scale math is exact)
void LaunchClamp::OnStart(StartState state) {
    PartModule::OnStart(state);
    Part* p = PartOf();
    if (p->fields.stagingIcon && p->fields.stagingIcon->empty() && Get<bool>("overrideStagingIconIfBlank")) p->fields.stagingIcon = std::string("STRUT");
    p->fields.PermanentGroundContact = true;
    if (BaseEvent* e = Events()["Release"]) e->active = false;
    if (hooks.initialHeightFromModel && initialHeight == -1.0f) {
        if (auto d = hooks.initialHeightFromModel(*p)) {
            initialHeight = *d;
            if (std::isinf(initialHeight)) initialHeight = -1.0f;
        }
        if (initialHeight != -1.0f) Set<float>("scaleFactor", f32store(fx(Get<float>("height")) / fx(initialHeight)));
    } else if (initialHeight != -1.0f && hooks.initialHeightFromModel) {
        Set<float>("scaleFactor", f32store(fx(Get<float>("height")) / fx(initialHeight)));
    }
    if ((static_cast<int>(state) & static_cast<int>(StartState::Landed)) == 0) return;
    if (hooks.setJoint) {
        hooks.setJoint(*p);  // StartCoroutine(SetJointWhenPartStarted)
        jointSet = true;
    }
    if (p->parent != nullptr) {
        if (BaseEvent* e = Events()["Release"]) e->active = true;
    }
    if (!SceneEditor()) p->fields.PermanentGroundContact = true;
}

// LaunchClamp.cs:316
void LaunchClamp::OnActive() {
    if (Get<bool>("stagingEnabled")) Release();
}

// LaunchClamp.cs:324 / :332
void LaunchClamp::OnVesselPack() {
    if (jointSet) {
        if (hooks.unsetJoint) hooks.unsetJoint(*PartOf());
        jointSet = false;
    }
}
void LaunchClamp::OnVesselUnpack() {
    if (!jointSet && hooks.setJoint) {
        hooks.setJoint(*PartOf());
        jointSet = true;
    }
}

// LaunchClamp.cs:341
void LaunchClamp::ReleaseClamp(KSPActionParam&) { Release(); }

// LaunchClamp.cs:347
void LaunchClamp::Release() {
    Part* p = PartOf();
    if (p->parent == nullptr || !SceneFlight()) return;
    Vessel* vessel = p->vessel;
    p->decouple();
    if (vessel != nullptr && vessel->persistentId != p->vessel->persistentId) {
        auto& gc = vessel->GroundContacts;
        gc.erase(std::remove(gc.begin(), gc.end(), p), gc.end());
    }
    // releaseFx.Burst() and the decouple animation: the model's
}

// LaunchClamp.cs:369
int LaunchClamp::GetStageIndex(int) { return PartOf()->fields.inverseStage; }

// =============================================================== ModuleStructuralNode

Part* ModuleStructuralNode::PartOf() const { return PartOfModule(*this); }

// ModuleStructuralNode.cs:29
void ModuleStructuralNode::OnStart(StartState state) {
    PartModule::OnStart(state);
    Part* p = PartOf();
    const std::string root = Get<std::string>("rootObject");
    structTransformPresent = StructuralModel().hasModelTransform && StructuralModel().hasModelTransform(*p, root);
    if (!structTransformPresent) return;
    if (state == StartState::Editor) {
        if (StructuralModel().setModelTransformActive) StructuralModel().setModelTransformActive(*p, root, false);
        structureVisible = false;
    } else {
        CheckDisplay();
    }
}

// ModuleStructuralNode.cs:45
void ModuleStructuralNode::LateUpdate() {
    if (SceneEditor() && structTransformPresent) CheckDisplay();
}

// ModuleStructuralNode.cs:53
void ModuleStructuralNode::CheckDisplayEditor(Part*) {
    if (structTransformPresent) CheckDisplay();
}

// ModuleStructuralNode.cs:71
void ModuleStructuralNode::SetNodeState(bool showNodes) {
    Part* p = PartOf();
    for (const std::string& nodeId : dotnet::Split(Get<std::string>("attachNodeNames"), ",", false)) {
        AttachNode* attachNode = p->FindAttachNode(nodeId);
        if (attachNode == nullptr) continue;
        if (showNodes) {
            attachNode->nodeType = AttachNodeType::Stack;
            attachNode->radius = Get<float>("nodeRadius");
        } else {
            attachNode->nodeType = AttachNodeType::Dock;
            attachNode->radius = 0.001f;
        }
    }
}

// ModuleStructuralNode.cs:93
bool ModuleStructuralNode::CheckDisplay() {
    Part* p = PartOf();
    std::vector<AttachNode*> list;
    for (const std::string& nodeId : dotnet::Split(Get<std::string>("attachNodeNames"), ",", false))
        if (AttachNode* an = p->FindAttachNode(nodeId)) list.push_back(an);
    bool flag = false;
    if (Get<bool>("visibilityState")) {
        if (Get<bool>("spawnManually")) {
            flag = Get<bool>("spawnState");
        } else {
            for (AttachNode* item : list)
                if (item->attachedPart != nullptr) {
                    flag = true;
                    break;
                }
        }
        if (Get<bool>("reverseVisibility")) flag = !flag;
    }
    structureVisible = flag;
    if (StructuralModel().setModelTransformActive) StructuralModel().setModelTransformActive(*p, Get<std::string>("rootObject"), flag);
    return flag;
}

// ModuleStructuralNode.cs:132
void ModuleStructuralNode::DecoupleAction(const std::string&, bool) { CheckDisplay(); }

// =============================================================== ModuleStructuralNodeToggle

// ModuleStructuralNodeToggle.cs:21
void ModuleStructuralNodeToggle::Start() {
    if (!Get<std::string>("MeshMenuName").empty()) {
        if (BaseField* f = Fields()["showMesh"]) f->guiName = Get<std::string>("MeshMenuName");
    }
    if (!Get<std::string>("NodeMenuName").empty()) {
        if (BaseField* f = Fields()["showNodes"]) f->guiName = Get<std::string>("NodeMenuName");
    }
    UpdateNodes();
}

// ModuleStructuralNodeToggle.cs:36
void ModuleStructuralNodeToggle::OnFieldUpdated(const std::string&) { UpdateNodes(); }

// ModuleStructuralNodeToggle.cs:41
void ModuleStructuralNodeToggle::UpdateNodes() {
    Part* p = PartOfModule(*this);
    if (structNodesNull) {
        structNodes.clear();
        structNodesNull = false;
        for (size_t i = 0; i < p->Modules().Count(); i++)
            if (auto* n = dynamic_cast<ModuleStructuralNode*>(p->Modules().GetModule(i))) structNodes.push_back(n);
    }
    for (size_t i = 0; i < structNodes.size(); i++) {
        structNodes[i]->Set<bool>("visibilityState", Get<bool>("showMesh"));
        structNodes[i]->SetNodeState(Get<bool>("showNodes"));
    }
}

// =============================================================== DynamicNodeSet / ModuleDynamicNodes

// DynamicNodeSet.cs:16
void DynamicNodeSet::Load(const ConfigNode& node) {
    auto str = [&](const char* k, std::string& out) {
        if (const std::string* v = node.GetValuePtr(k)) out = *v;
    };
    auto num = [&](const char* k, int& out) {
        int32_t r = 0;
        if (const std::string* v = node.GetValuePtr(k))
            if (dotnet::TryParseInt32(*v, r)) out = r;
    };
    DisplayText = "";
    str("DisplayText", DisplayText);
    MeshTransform = "";
    str("MeshTransform", MeshTransform);
    NodePrefix = "";
    str("NodePrefix", NodePrefix);
    SetCount = 0;
    num("SetCount", SetCount);
    Symmetry = 0;
    num("Symmetry", Symmetry);
}

Part* ModuleDynamicNodes::PartOf() const { return PartOfModule(*this); }

ModuleDynamicNodes::~ModuleDynamicNodes() {
    if (subscribed_) GE::onEditorShipModified.Remove(this, &ModuleDynamicNodes::OnShipModified);  // :163 OnDestroy
}

// ModuleDynamicNodes.cs:126
void ModuleDynamicNodes::OnAwake() {
    PartModule::OnAwake();
    if (setListNull) {
        SetList.clear();
        setListNull = false;
    }
}

// ModuleDynamicNodes.cs:134
void ModuleDynamicNodes::OnLoad(const ConfigNode& node) {
    if (node.HasNode("NODE_SET")) SetList.clear();
    if (node.HasValue("NodeSetIdx")) {
        int32_t r = 0;
        if (dotnet::TryParseInt32(*node.GetValuePtr("NodeSetIdx"), r)) Set<int32_t>("setIndex", r);
    }
    for (ConfigNode* n : node.GetNodes("NODE_SET")) {
        DynamicNodeSet item;
        item.Load(*n);
        SetList.push_back(item);
    }
}

// ModuleDynamicNodes.cs:25
void ModuleDynamicNodes::ChangeNodeSet(int newIndex) {
    ToggleNodeSet(Get<int32_t>("NodeSetIdx"), false);
    Set<int32_t>("NodeSetIdx", newIndex);
    ToggleNodeSet(Get<int32_t>("NodeSetIdx"), true);
    const DynamicNodeSet& s = SetList.at(static_cast<size_t>(Get<int32_t>("NodeSetIdx")));
    PartOf()->fields.stackSymmetry = s.Symmetry;
}

// ModuleDynamicNodes.cs:35
void ModuleDynamicNodes::ToggleNodeSet(int idx, bool showNodes) {
    Part* p = PartOf();
    const DynamicNodeSet& s = SetList.at(static_cast<size_t>(idx));
    if (StructuralModel().hasModelTransform && StructuralModel().setModelTransformActive && StructuralModel().hasModelTransform(*p, s.MeshTransform))
        StructuralModel().setModelTransformActive(*p, s.MeshTransform, showNodes);
    for (int i = 0; i < s.SetCount + 1; i++) {
        AttachNode* attachNode = p->FindAttachNode(s.NodePrefix + std::to_string(i));
        if (attachNode == nullptr) continue;
        if (showNodes) {
            attachNode->nodeType = AttachNodeType::Stack;
            attachNode->radius = 0.4f;
        } else {
            attachNode->nodeType = AttachNodeType::Dock;
            attachNode->radius = 0.0001f;
        }
    }
}

// ModuleDynamicNodes.cs:63
void ModuleDynamicNodes::OnStart(StartState state) {
    PartModule::OnStart(state);
    InitializeNodeSets();
    ChangeNodeSet(Get<int32_t>("NodeSetIdx"));
    GE::onEditorShipModified.Add(this, &ModuleDynamicNodes::OnShipModified);
    subscribed_ = true;
}

// ModuleDynamicNodes.cs:70
void ModuleDynamicNodes::OnStartFinished(StartState) {
    if (Get<bool>("autostrut")) SetupAutoStrut();
}

// ModuleDynamicNodes.cs:78
void ModuleDynamicNodes::OnShipModified(ShipConstruct*) { SetupAutoStrut(); }

// ModuleDynamicNodes.cs:83
void ModuleDynamicNodes::SetupAutoStrut() {
    Part* p = PartOf();
    if (p->parent == nullptr) return;
    for (size_t i = 0, count = p->attachNodes.size(); i < count; i++) {
        AttachNode& attachNode = p->attachNodes[i];
        if (attachNode.nodeType == AttachNodeType::Stack && attachNode.attachedPart != nullptr && attachNode.attachedPart != p &&
            attachNode.attachedPart->fields.persistentId != p->parent->fields.persistentId) {
            const bool servo = p->isRobotic();
            Part* attachedPart = attachNode.attachedPart;
            if (!servo) {
                attachedPart->fields.autoStrutMode = 6 /* Part.AutoStrutMode.ForceGrandparent */;
                // attachedPart.UpdateAutoStrut(): the physics package's (joint)
            }
        }
    }
}

// ModuleDynamicNodes.cs:106
void ModuleDynamicNodes::InitializeNodeSets() {
    const int count = static_cast<int>(SetList.size());
    stateNames.assign(static_cast<size_t>(count), "");
    for (int i = 0; i < count; i++) {
        stateNames[static_cast<size_t>(i)] = SetList[static_cast<size_t>(i)].DisplayText;
        ToggleNodeSet(i, i == Get<int32_t>("NodeSetIdx"));
    }
}

// ModuleDynamicNodes.cs:155
void ModuleDynamicNodes::FixedUpdate() {
    if (SceneEditor() && Get<int32_t>("setIndex") != Get<int32_t>("NodeSetIdx")) ChangeNodeSet(Get<int32_t>("setIndex"));
}

}  // namespace ksp

namespace ksp::detail {
void RegisterWp26Modules() {
    ModuleFactory::Register("ModuleProceduralFairing", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleProceduralFairing>(c)); });
    ModuleFactory::Register("ModuleJettison", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleJettison>(c)); });
    ModuleFactory::Register("ModuleCargoBay", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleCargoBay>(c)); });
    ModuleFactory::Register("ModuleServiceModule", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleServiceModule>(c)); });
    ModuleFactory::Register("LaunchClamp", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<LaunchClamp>(c)); });
    ModuleFactory::Register("ModuleStructuralNode", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleStructuralNode>(c)); });
    ModuleFactory::Register("ModuleStructuralNodeToggle", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleStructuralNodeToggle>(c)); });
    ModuleFactory::Register("ModuleDynamicNodes", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleDynamicNodes>(c)); });
}
}  // namespace ksp::detail
