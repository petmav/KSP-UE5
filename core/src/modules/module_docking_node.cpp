// WP-25: ModuleDockingNode.cs (see include/ksp/modules/module_docking_node.hpp). Every function cites ModuleDockingNode.cs:<line> of
// decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/modules/module_docking_node.hpp"

#include <algorithm>
#include <stdexcept>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/modules/animation_modules.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = GameEvents;
using KSP::Localization::Localizer;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }

// float * float stored as a float
float MulF(float a, float b) { return f32store(fx(a) * fx(b)); }

// float.ToString("0.0###"): at least one and at most four decimals
std::string FormatForce(float v) {
    std::string s = dotnet::FormatSingleF(v, 4);
    const size_t dot = s.find('.');
    if (dot != std::string::npos) {
        while (s.size() > dot + 2 && s.back() == '0') s.pop_back();
    }
    return s;
}

void AddForceOn(Part& p, const Vector3f& f) {
    if (ModuleDecouplerBase::addForce) ModuleDecouplerBase::addForce(p, f);
}
void AddForceAtPositionOn(Part& p, const Vector3f& f, const Vector3f& pos) {
    if (ModuleDecouplerBase::addForceAtPosition) ModuleDecouplerBase::addForceAtPosition(p, f, pos);
}
void AddTorqueOn(Part& p, const Vector3f& t) {
    if (DockingEnvironment::addTorque) DockingEnvironment::addTorque(p, t);
}
Vector3f VelocityOf(Part& p) { return DockingEnvironment::partVelocity ? DockingEnvironment::partVelocity(p) : Vector3f(); }
void ForceSetActive(Vessel* v) {
    if (ModuleDecouplerBase::forceSetActiveVessel) ModuleDecouplerBase::forceSetActiveVessel(v);
}
void SetNeutral() {
    if (DockingEnvironment::setNeutralControls) DockingEnvironment::setNeutralControls();
}
}  // namespace

bool DockingEnvironment::SceneIsFlight() { return ResourceFlowEnvironment::LoadedSceneIsFlight(); }
bool DockingEnvironment::SceneIsEditor() { return ResourceFlowEnvironment::LoadedSceneIsEditor(); }

// ModuleDockingNode.cs:8 (the constructor chain: the [KSPField] initialisers run in PartModule's schema; kspcore also tracks the live nodes)
ModuleDockingNode::ModuleDockingNode(const ModuleClassInfo& cls) : PartModule(cls) { live_.push_back(this); }

ModuleDockingNode::~ModuleDockingNode() {
    live_.erase(std::remove(live_.begin(), live_.end(), this), live_.end());
    if (subscribed_) {
        GE::onPartActionUIShown.Remove(this, &ModuleDockingNode::OnPartMenuOpen);
        GE::onPartActionUIDismiss.Remove(this, &ModuleDockingNode::OnPartMenuClose);
    }
}

Part* ModuleDockingNode::PartOf() const { return dynamic_cast<Part*>(part); }

// one frame of the coroutines (lateFSMStart, WaitAndSwitchFocus), in module creation order
void ModuleDockingNode::TickAll() {
    std::vector<ModuleDockingNode*> mods(live_);
    for (ModuleDockingNode* m : mods) {
        if (std::find(live_.begin(), live_.end(), m) == live_.end()) continue;
        std::vector<Delayed> pending;
        pending.swap(m->delayed_);
        for (Delayed& d : pending) {
            if (--d.ticks <= 0) d.fn();
            else m->delayed_.push_back(std::move(d));
        }
    }
}

// nodeTransform.position / forward / up / right: Part.FindModelTransform(nodeTransformName) through the seam (NullReferenceException when the
// transform does not exist, like the C# access to a destroyed / null Transform)
DockPose ModuleDockingNode::NodePose() {
    Part* p = PartOf();
    if (p == nullptr || !DockingEnvironment::modelTransformPose) throw Nre();
    std::optional<DockPose> pose = DockingEnvironment::modelTransformPose(*p, Get<std::string>("nodeTransformName"));
    if (!pose) throw Nre();
    return *pose;
}

// ModuleDockingNode.cs:297
float ModuleDockingNode::JointTargetAngle() {
    float workingJointTargetAngle = Get<float>("targetAngle");
    if (Get<bool>("inverted")) workingJointTargetAngle = f32store(0.0 - fx(workingJointTargetAngle));
    if (otherNode != nullptr) {
        if (otherNode->Get<bool>("inverted")) workingJointTargetAngle = f32store(fx(workingJointTargetAngle) + (0.0 - fx(otherNode->Get<float>("targetAngle"))));
        else workingJointTargetAngle = f32store(fx(workingJointTargetAngle) + fx(otherNode->Get<float>("targetAngle")));
    }
    return workingJointTargetAngle;
}

// ModuleDockingNode.cs:321
float ModuleDockingNode::VisualTargetAngle() {
    float num = 0.0f;
    const void* rj = RotationJoint();
    const void* own = DockingEnvironment::attachJointOf ? DockingEnvironment::attachJointOf(*PartOf()) : nullptr;
    if (rj != nullptr && rj == own) {
        num = f32store(0.0 - fx(Get<float>("targetAngle")));
        if (Get<bool>("inverted")) num = f32store(0.0 - fx(num));
    } else {
        num = Get<float>("targetAngle");
        if (Get<bool>("inverted")) num = f32store(0.0 - fx(num));
    }
    return num;
}

// ModuleDockingNode.cs:346
const void* ModuleDockingNode::RotationJoint() {
    Part* p = PartOf();
    auto joint = [](Part* x) -> const void* { return (x != nullptr && DockingEnvironment::attachJointOf) ? DockingEnvironment::attachJointOf(*x) : nullptr; };
    if (fsm && fsm->Started() && fsm->CurrentState() == st_docked_docker.get()) {
        if (const void* j = joint(p)) return j;
    } else if (fsm && fsm->Started() && fsm->CurrentState() == st_docked_dockee.get()) {
        if (otherNode != nullptr)
            if (const void* j = joint(otherNode->PartOf())) return j;
    } else if (fsm && fsm->Started() && fsm->CurrentState() == st_preattached.get() && joint(p) != nullptr && otherNode != nullptr) {
        // base.part.attachJoint.Parent: the part the joint hangs on = the part's parent
        if (p->parent != nullptr && p->parent->FindModuleOfClass("ModuleDockingNode") != nullptr) return joint(p);
        if (const void* j = joint(otherNode->PartOf())) return j;
    }
    return nullptr;
}

// ModuleDockingNode.cs:391
bool ModuleDockingNode::IsDisabled() {
    if (fsm != nullptr && fsm->Started()) return fsm->CurrentState() == st_disabled.get();
    return false;
}

// ModuleDockingNode.cs:404
void ModuleDockingNode::ToggleLockedAction(KSPActionParam&) {
    if (Get<bool>("nodeIsLocked")) {
        DisengageNodeLock();
    } else if (!EngageNodeLock()) {
        DockingEnvironment::screenMessages.push_back(Localizer::Format("Cannot Lock Docking part, Docking Node is moving."));
    }
}

// ModuleDockingNode.cs:417
void ModuleDockingNode::ServoEngageLockAction(KSPActionParam&) {
    if (!EngageNodeLock()) DockingEnvironment::screenMessages.push_back(Localizer::Format("Cannot Lock Docking part, Servo is moving."));
}

// ModuleDockingNode.cs:426
void ModuleDockingNode::ServoDisgageLockAction(KSPActionParam&) { DisengageNodeLock(); }

// ModuleDockingNode.cs:431
bool ModuleDockingNode::EngageNodeLock() {
    if (Get<bool>("nodeIsLocked")) return true;
    if (!Get<bool>("nodeIsLocked") && !IsRotating) {
        Fields()["nodeIsLocked"]->SetValue(ModuleValue(true));
        ModifyLocked(ModuleValue());
        return true;
    }
    return false;
}

// ModuleDockingNode.cs:446
void ModuleDockingNode::DisengageNodeLock() {
    if (Get<bool>("nodeIsLocked")) {
        Fields()["nodeIsLocked"]->SetValue(ModuleValue(false));
        ModifyLocked(ModuleValue());
    }
}

// ModuleDockingNode.cs:455 (CycleAllAutoStrut / CycleAutoStrut: AutoStrut is the joint package's)
void ModuleDockingNode::ModifyLocked(const ModuleValue&) {
    Fields()["targetAngle"]->guiInteractable = !Get<bool>("nodeIsLocked");
    Fields()["inverted"]->guiInteractable = !Get<bool>("nodeIsLocked");
    Part* p = PartOf();
    if (Get<bool>("nodeIsLocked") && (DockingEnvironment::SceneIsFlight() || DockingEnvironment::SceneIsEditor())) {
        if (p == nullptr) throw Nre();
        Part* root = nullptr;
        if (DockingEnvironment::SceneIsFlight()) root = p->vessel->rootPart;
        else if (p->ship != nullptr && !p->ship->liveParts.empty()) root = p->ship->liveParts[0];
        RecurseCoordUpdate(p, root);
    }
}

// ModuleDockingNode.cs:479
void ModuleDockingNode::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("MakePrimary", BaseEvent::Handler([this] { MakePrimary(); }));
    Events().Bind("Undock", BaseEvent::Handler([this] { Undock(); }));
    Events().Bind("UndockSameVessel", BaseEvent::Handler([this] { UndockSameVessel(); }));
    Events().Bind("Decouple", BaseEvent::Handler([this] { Decouple(); }));
    Events().Bind("SetAsTarget", BaseEvent::Handler([this] { SetAsTarget(); }));
    Events().Bind("UnsetTarget", BaseEvent::Handler([this] { UnsetTarget(); }));
    Events().Bind("EnableXFeed", BaseEvent::Handler([this] { EnableXFeed(); }));
    Events().Bind("DisableXFeed", BaseEvent::Handler([this] { DisableXFeed(); }));
    Events().Bind("MakeReferenceTransform", BaseEvent::Handler([this] { MakeReferenceTransform(); }));
    Actions().Bind("ToggleLockedAction", [this](KSPActionParam& p) { ToggleLockedAction(p); });
    Actions().Bind("ServoEngageLockAction", [this](KSPActionParam& p) { ServoEngageLockAction(p); });
    Actions().Bind("ServoDisgageLockAction", [this](KSPActionParam& p) { ServoDisgageLockAction(p); });
    Actions().Bind("UndockAction", [this](KSPActionParam& p) { UndockAction(p); });
    Actions().Bind("DecoupleAction", [this](KSPActionParam& p) { DecoupleAction(p); });
    Actions().Bind("MakeReferenceToggle", [this](KSPActionParam& p) { MakeReferenceToggle(p); });
    Actions().Bind("EnableXFeedAction", [this](KSPActionParam& p) { EnableXFeedAction(p); });
    Actions().Bind("DisableXFeedAction", [this](KSPActionParam& p) { DisableXFeedAction(p); });
    Actions().Bind("ToggleXFeedAction", [this](KSPActionParam& p) { ToggleXFeedAction(p); });
    nodeTypes.clear();
    if (setStagingState) {
        setStagingState = false;
        Set<bool>("stagingEnabled", false);
    }
    // the prefab path of LoadModuleData runs Awake before it assigns `part`: the part-side statements wait for OnStart then
    if (Part* p = PartOf()) {
        if (std::find(p->dockingPorts.begin(), p->dockingPorts.end(), this) == p->dockingPorts.end()) p->dockingPorts.push_back(this);  // AddUnique
    }
    evtSetAsTarget = Events()["SetAsTarget"];
    evtUnsetTarget = Events()["UnsetTarget"];
    nodeTransformPresent = false;
    if (Part* p = PartOf())
        if (DockingEnvironment::modelTransformPose) nodeTransformPresent = DockingEnvironment::modelTransformPose(*p, Get<std::string>("nodeTransformName")).has_value();
    consumedResources_.clear();
    consumedResourcesSet_ = true;
    const float num = f32store(fx(Get<float>("baseResourceConsumptionRate")) * fx(Get<float>("maxMotorOutput")) *
                               (fx(Get<float>("traverseVelocity")) / fx(Get<float>("referenceConsumptionVelocity"))));
    motorRate = f32store(fx(num) / fx(Get<float>("efficiency")));
    for (size_t i = 0, count = resHandler.inputResources.size(); i < count; i++) {
        resHandler.inputResources[i]->startUpHandler = true;
        resHandler.inputResources[i]->startUpAmount = motorRate;
        PartResourceLibrary* lib = PartResourceLibrary::Instance();
        consumedResources_.push_back(lib != nullptr ? lib->GetDefinition(resHandler.inputResources[i]->name) : nullptr);
    }
}

// ModuleDockingNode.cs:510
void ModuleDockingNode::OnLoad(const ConfigNode& node) {
    if (node.HasValue("state")) state = *node.GetValue("state");
    if (node.HasValue("dockUId")) {
        uint32_t v = 0;
        if (!dotnet::TryParseUInt32(*node.GetValue("dockUId"), v)) throw dotnet::FormatException("Input string was not in a correct format.");  // uint.Parse
        dockedPartUId = v;
    }
    if (node.HasValue("dockNodeIdx")) {
        int32_t v = 0;
        if (!dotnet::TryParseInt32(*node.GetValue("dockNodeIdx"), v)) throw dotnet::FormatException("Input string was not in a correct format.");  // int.Parse
        dockingNodeModuleIndex = v;
    }
    if (node.HasNode("DOCKEDVESSEL")) {
        vesselInfo = DockedVesselInfo();
        vesselInfo->Load(*node.GetNode("DOCKEDVESSEL"));
    }
    Part* p = PartOf();
    if (!Get<std::string>("referenceAttachNode").empty()) {
        if (p == nullptr) throw Nre();
        referenceNode = p->FindAttachNode(Get<std::string>("referenceAttachNode"));
    }
    if (p == nullptr) throw Nre();
    p->fields.fuelCrossFeed = Get<bool>("crossfeed");
    Events()["EnableXFeed"]->active = !Get<bool>("crossfeed");
    Events()["DisableXFeed"]->active = Get<bool>("crossfeed");
    // nodeType.Split(new char[] { ',', ' ' }, StringSplitOptions.RemoveEmptyEntries)
    {
        const std::string nt = Get<std::string>("nodeType");
        std::string cur;
        for (char ch : nt) {
            if (ch == ',' || ch == ' ') {
                if (!cur.empty()) nodeTypes.insert(cur);
                cur.clear();
            } else {
                cur.push_back(ch);
            }
        }
        if (!cur.empty()) nodeTypes.insert(cur);
    }
    Vector2f lim = Get<Vector2f>("hardMinMaxLimits");
    lim.x = Mathf::Clamp(lim.x, -86.0f, 0.0f);
    lim.y = Mathf::Clamp(lim.y, 0.0f, 86.0f);
    Set<Vector2f>("hardMinMaxLimits", lim);
}

// ModuleDockingNode.cs:546
void ModuleDockingNode::OnSave(ConfigNode& node) {
    node.AddValue("state", (fsm == nullptr || !fsm->Started()) ? std::string("Ready") : fsm->currentStateName);
    node.AddValue("dockUId", dockedPartUId);
    node.AddValue("dockNodeIdx", static_cast<int32_t>(dockingNodeModuleIndex));
    if (vesselInfo) vesselInfo->Save(*node.AddNode("DOCKEDVESSEL"));
    Part* p = PartOf();
    if (DockingEnvironment::SceneIsFlight() && p != nullptr && p->vessel != nullptr && p->vessel->loaded && Get<bool>("canRotate") &&
        !Get<bool>("nodeIsLocked") && otherNode != nullptr)
        ApplyCoordsUpdate();
}

// ModuleDockingNode.cs:561
void ModuleDockingNode::OnStart(StartState st) {
    Events()["Undock"]->active = false;
    Events()["UndockSameVessel"]->active = false;
    Events()["MakePrimary"]->active = false;
    Part* p = PartOf();
    if (p == nullptr) throw Nre();
    if (std::find(p->dockingPorts.begin(), p->dockingPorts.end(), this) == p->dockingPorts.end()) p->dockingPorts.push_back(this);  // OnAwake's AddUnique (prefab path)
    nodeTransformPresent = DockingEnvironment::modelTransformPose && DockingEnvironment::modelTransformPose(*p, Get<std::string>("nodeTransformName")).has_value();
    if (!nodeTransformPresent) {
        log::Warning("[Docking Node Module]: WARNING - No node transform found with name " + Get<std::string>("nodeTransformName"));
        return;
    }
    if (Get<std::string>("controlTransformName").empty()) {
        controlTransformName_.clear();
    } else {
        const bool found = DockingEnvironment::modelTransformPose && DockingEnvironment::modelTransformPose(*p, Get<std::string>("controlTransformName")).has_value();
        if (!found) {
            log::Warning("[Docking Node Module]: WARNING - No control transform found with name " + Get<std::string>("controlTransformName"));
            controlTransformName_.clear();
        } else {
            controlTransformName_ = Get<std::string>("controlTransformName");
        }
    }
    if (p->fields.physicalSignificance != 0) {
        log::Warning("[Docking Node Module]: WARNING - The part for a docking node module cannot be physicsless!");
        p->fields.physicalSignificance = 0;
    }
    if (Get<int32_t>("deployAnimationController") != -1) {
        const int32_t idx = Get<int32_t>("deployAnimationController");
        deployAnimator = (idx >= 0 && static_cast<size_t>(idx) < p->Modules().Count()) ? dynamic_cast<ModuleAnimateGeneric*>(p->Modules().GetModule(static_cast<size_t>(idx))) : nullptr;
    }
    p->fields.fuelCrossFeed = Get<bool>("crossfeed");
    Events()["EnableXFeed"]->active = !Get<bool>("crossfeed");
    Events()["DisableXFeed"]->active = Get<bool>("crossfeed");
    StartCoroutine(1, [this, st] { lateFSMStart(st); });
    // `rotationTransformName != string.Empty`: the field has no initialiser (null), so a port without the line still looks the transform up
    if (IsNull("rotationTransformName") || !Get<std::string>("rotationTransformName").empty()) {
        const std::string rn = IsNull("rotationTransformName") ? std::string() : Get<std::string>("rotationTransformName");
        rotationTransformPresent_ = DockingEnvironment::modelTransformPose && DockingEnvironment::modelTransformPose(*p, rn).has_value();
        if (!rotationTransformPresent_) {
            log::Warning("[Docking Node Module]: WARNING - No rotation transform found with name " + rn);
            Set<bool>("canRotate", false);
        }
    }
    if (!subscribed_) {
        GE::onPartActionUIShown.Add(this, &ModuleDockingNode::OnPartMenuOpen);
        GE::onPartActionUIDismiss.Add(this, &ModuleDockingNode::OnPartMenuClose);
        subscribed_ = true;
    }
    if (!(DockingEnvironment::sceneIsMissionBuilder && DockingEnvironment::sceneIsMissionBuilder()) && Get<bool>("canRotate")) {
        if (!rotationTransformPresent_ || !DockingEnvironment::rotationTransformEuler) throw Nre();
        initialRotation = DockingEnvironment::rotationTransformEuler(*this);
        const Vector2f lim = Get<Vector2f>("hardMinMaxLimits");
        BaseField* f = Fields()["targetAngle"];
        if (f->uiControlFlight) {
            // targetAngleUIField.minValue / maxValue: the UI_FloatRange params
            for (auto& kv : f->uiControlFlight->params) {
                if (kv.first == "minValue") { kv.second.kind = cs::CsValue::Kind::Float; kv.second.f = lim.x; }
                if (kv.first == "maxValue") { kv.second.kind = cs::CsValue::Kind::Float; kv.second.f = lim.y; }
            }
        }
        if (auto* ax = dynamic_cast<BaseAxisField*>(f)) {
            ax->minValue = lim.x;
            ax->maxValue = lim.y;
        }
    }
    Fields()["targetAngle"]->guiActive = false;
    Fields()["inverted"]->guiActive = false;
    Fields()["targetAngle"]->guiActiveEditor = false;
    Fields()["inverted"]->guiActiveEditor = false;
    if ((Get<bool>("canRotate") && otherNode == nullptr && sameVesselDockJoint == nullptr) || !Get<bool>("canRotate")) Set<bool>("nodeIsLocked", true);
    Fields()["nodeIsLocked"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyLocked(v); });
    Fields()["targetAngle"]->guiInteractable = !Get<bool>("nodeIsLocked");
}

// ModuleDockingNode.cs:631
void ModuleDockingNode::OnStartFinished(StartState) {
    if (!DockingEnvironment::SceneIsFlight()) return;
    Part* p = PartOf();
    if (otherNode != nullptr) {
        const void* joint = RotationJoint();
        const void* own = DockingEnvironment::attachJointOf ? DockingEnvironment::attachJointOf(*p) : nullptr;
        if (Get<bool>("canRotate") && joint != nullptr && own != nullptr && rotationTransformPresent_) {
            if (DockingEnvironment::limitAngularXMotion) DockingEnvironment::limitAngularXMotion(joint);
            driveTargetAngle = JointTargetAngle();
            if (DockingEnvironment::rotationTransformEuler) initialRotation = DockingEnvironment::rotationTransformEuler(*this);
            cachedInitialAngle = JointTargetAngle();
            if (joint == own) {
                const Quaternionf targetLocalRotation = SetTargetRotation(Quaternionf::identity(), f32store(fx(JointTargetAngle()) - fx(cachedInitialAngle)), true, Vector3f(0.0f, 1.0f, 0.0f));
                if (DockingEnvironment::setTargetRotationLocal) DockingEnvironment::setTargetRotationLocal(joint, targetLocalRotation);
            }
            if (joint == own) targetRotation = SetTargetRotation(Quaternionf::identity(), VisualTargetAngle(), false);
            else targetRotation = SetTargetRotation(Quaternionf::identity(), f32store(0.0 - fx(VisualTargetAngle())), false);
            if (DockingEnvironment::setRotationTransformLocalRotation) DockingEnvironment::setRotationTransformLocalRotation(*this, initialRotation, targetRotation);
            visualTargetAngle = VisualTargetAngle();
            rotationInitComplete = true;
            if (otherNode != nullptr && !otherNode->rotationInitComplete) {
                otherNode->targetRotation = SetTargetRotation(Quaternionf::identity(), f32store(0.0 - fx(otherNode->VisualTargetAngle())), false);
                if (DockingEnvironment::setRotationTransformLocalRotation) DockingEnvironment::setRotationTransformLocalRotation(*otherNode, otherNode->initialRotation, otherNode->targetRotation);
                otherNode->visualTargetAngle = otherNode->VisualTargetAngle();
                otherNode->rotationInitComplete = true;
            }
        }
    } else {
        rotationInitComplete = true;
    }
}

// ModuleDockingNode.cs:681
void ModuleDockingNode::OnPartPack() {
    if (Get<bool>("canRotate") && !Get<bool>("nodeIsLocked") && otherNode != nullptr) ApplyCoordsUpdate();
}

// ModuleDockingNode.cs:689
void ModuleDockingNode::OnPartUnpack() {
    if (!Get<bool>("canRotate") || otherNode == nullptr) return;
    if (!IsRotating) {
        driveTargetAngle = JointTargetAngle();
        visualTargetAngle = VisualTargetAngle();
        const void* joint = RotationJoint();
        const void* own = DockingEnvironment::attachJointOf ? DockingEnvironment::attachJointOf(*PartOf()) : nullptr;
        if (joint != nullptr && own != nullptr && joint == own) {
            const Quaternionf targetLocalRotation = SetTargetRotation(Quaternionf::identity(), f32store(fx(JointTargetAngle()) - fx(cachedInitialAngle)), true, Vector3f(0.0f, 1.0f, 0.0f));
            if (DockingEnvironment::setTargetRotationLocal) DockingEnvironment::setTargetRotationLocal(joint, targetLocalRotation);
        }
    }
    SetJointHighLowLimits();
}

// ModuleDockingNode.cs:708
void ModuleDockingNode::OnInventoryModeEnable() {
    Part* p = PartOf();
    if (p == nullptr) throw Nre();
    if (!Get<std::string>("referenceAttachNode").empty()) referenceNode = p->FindAttachNode(Get<std::string>("referenceAttachNode"));
    nodeTransformPresent = DockingEnvironment::modelTransformPose && DockingEnvironment::modelTransformPose(*p, Get<std::string>("nodeTransformName")).has_value();
    if (referenceNode == nullptr || referenceNode->attachedPart == nullptr) return;
    ModuleDockingNode* found = nullptr;
    for (size_t num = 0, count = referenceNode->attachedPart->Modules().Count(); num < count; num++) {
        auto* m = dynamic_cast<ModuleDockingNode*>(referenceNode->attachedPart->Modules().GetModule(num));
        if (m != nullptr && m->referenceNode != nullptr && m->referenceNode->attachedPart == p) {
            found = m;
            break;
        }
    }
    if (found == nullptr) return;
    otherNode = found;
    if (fsm->Started()) fsm->RunEvent(on_construction_Attach.get());
    otherNode->otherNode = this;
    otherNode->OnConstructionAttach();
}

// ModuleDockingNode.cs:745
void ModuleDockingNode::OnInventoryModeDisable() {
    Part* p = PartOf();
    if (p == nullptr) throw Nre();
    if (!Get<std::string>("referenceAttachNode").empty()) referenceNode = p->FindAttachNode(Get<std::string>("referenceAttachNode"));
    nodeTransformPresent = DockingEnvironment::modelTransformPose && DockingEnvironment::modelTransformPose(*p, Get<std::string>("nodeTransformName")).has_value();
    if (fsm->Started()) fsm->RunEvent(on_construction_Detach.get());
}

// ModuleDockingNode.cs:759
void ModuleDockingNode::DemoteToPhysicslessPart() { physicsLessMode = true; }

// ModuleDockingNode.cs:764
void ModuleDockingNode::PromoteToPhysicalPart() { physicsLessMode = false; }

// ModuleDockingNode.cs:769
void ModuleDockingNode::OnPartMenuOpen(UIPartActionWindow*, Part* inpPart) {
    Part* p = PartOf();
    if (p != nullptr && inpPart != nullptr && inpPart->fields.persistentId == p->fields.persistentId) {
        partActionMenuOpen = true;
        UpdatePAWUI();
    }
}

// ModuleDockingNode.cs:779
void ModuleDockingNode::OnPartMenuClose(Part* inpPart) {
    Part* p = PartOf();
    if (p != nullptr && inpPart != nullptr && inpPart->fields.persistentId == p->fields.persistentId) partActionMenuOpen = false;
}

// ModuleDockingNode.cs:785
ModuleDockingNode* ModuleDockingNode::FindOtherNode() {
    Part* p = FlightRegistry::FindPartByID(dockedPartUId);
    if (p != nullptr) {
        const size_t idx = static_cast<size_t>(dockingNodeModuleIndex);
        if (dockingNodeModuleIndex < 0 || idx >= p->Modules().Count()) throw std::out_of_range("Index was out of range");  // List<T>.get_Item: ArgumentOutOfRangeException
        return dynamic_cast<ModuleDockingNode*>(p->Modules().GetModule(idx));
    }
    return nullptr;
}

// ModuleDockingNode.cs:797 (the part of the coroutine after `yield return null`)
void ModuleDockingNode::lateFSMStart(StartState st) {
    SetupFSM();
    if ((static_cast<int>(st) & static_cast<int>(StartState::Editor)) != 0) return;
    Part* p = PartOf();
    if (state.find("Docked") != std::string::npos) {
        otherNode = FindOtherNode();
        if (otherNode == nullptr) {
            state = "Ready";
        } else if (state.find("(dockee)") != std::string::npos) {
            if (otherNode->state.find("Acquire") != std::string::npos) {
                state = st_acquire_dockee->name;
            } else if (otherNode->state.find("Docked") == std::string::npos ||
                       (otherNode->PartOf()->parent != p && p->parent != otherNode->PartOf())) {
                state = "Ready";
            }
        }
    }
    if (otherNode == nullptr && (state.find("Acquire") != std::string::npos || state.find("Disengage") != std::string::npos)) {
        otherNode = FindOtherNode();
        if (otherNode == nullptr) state = "Ready";
    }
    if (referenceNode != nullptr && referenceNode->attachedPart != nullptr) {
        for (size_t i = 0, count = referenceNode->attachedPart->Modules().Count(); i < count; i++) {
            auto* m = dynamic_cast<ModuleDockingNode*>(referenceNode->attachedPart->Modules().GetModule(i));
            if (m != nullptr && m->referenceNode != nullptr && m->referenceNode->attachedPart == p) otherNode = m;
        }
        fsm->StartFSM(st_preattached.get());
    } else {
        fsm->StartFSM(state);
    }
}

// ModuleDockingNode.cs:849
void ModuleDockingNode::SetupFSM() {
    using SC = KFSMStateChange;
    using CB = KFSMCallback;
    using CC = KFSMEventCondition;
    fsm = std::make_unique<KerbalFSM>();
    fsm->OnStateChange = Callback<KFSMState*, KFSMState*, KFSMEvent*>::Bind(this, &ModuleDockingNode::OnFSMStateChange);  // Delegate.Combine onto the empty delegate
    fsm->OnEventCalled = Callback<KFSMEvent*>::Bind(this, &ModuleDockingNode::OnFSMEventCalled);
    auto toggle = [this](bool active) {
        if (deployAnimator != nullptr) deployAnimator->Events()["Toggle"]->active = active;
    };
    st_ready = std::make_unique<KFSMState>("Ready");
    st_ready->OnEnter = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (Get<bool>("animReadyEnter") && deployAnimator) toggle(true);
    });
    st_ready->OnFixedUpdate = CB::Lambda(this, [this] { otherNode = FindNodeApproaches(); });
    st_ready->OnLeave = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (Get<bool>("animReadyExit") && deployAnimator) toggle(false);
    });
    fsm->AddState(st_ready.get());
    st_acquire = std::make_unique<KFSMState>("Acquire");
    st_acquire->OnFixedUpdate = CB::Lambda(this, [this] {
        const DockPose a = NodePose();
        const DockPose b = otherNode->NodePose();
        const Vector3f vector = b.position - a.position;
        const Vector3f vector2 = Vector3f::Cross(a.forward, -b.forward);
        const Vector3f vector3 = Vector3f::Cross(a.up, -b.up);
        const float num = f32store(1.0 / fx(Mathf::Max(vector.sqrMagnitude(), 0.05f)));
        Part* p = PartOf();
        const float tweak = MulF(Get<float>("acquireForceTweak"), 0.01f);
        AddForceAtPositionOn(*p, vector * num * Get<float>("acquireForce") * 0.5f * tweak, a.position);
        AddTorqueOn(*p, vector2 * num * Get<float>("acquireTorque") * 0.5f * tweak);
        AddTorqueOn(*p, vector3 * num * Get<float>("acquireTorqueRoll") * 0.5f * tweak);
        Part* op = otherNode->PartOf();
        const float otweak = MulF(otherNode->Get<float>("acquireForceTweak"), 0.01f);
        AddForceAtPositionOn(*op, -(vector * num) * otherNode->Get<float>("acquireForce") * 0.5f * otweak, b.position);
        AddTorqueOn(*op, -(vector2 * num) * otherNode->Get<float>("acquireTorque") * 0.5f * otweak);
        AddTorqueOn(*op, -(vector3 * num) * otherNode->Get<float>("acquireTorqueRoll") * 0.5f * otweak);
    });
    fsm->AddState(st_acquire.get());
    st_acquire_dockee = std::make_unique<KFSMState>("Acquire (dockee)");
    fsm->AddState(st_acquire_dockee.get());
    st_docked_dockee = std::make_unique<KFSMState>("Docked (dockee)");
    st_docked_dockee->OnEnter = SC::Lambda(this, [this](KFSMState*) {
        if (sameVesselUndockRedock && fsm->LastState() == st_docker_sameVessel.get()) {
            DestroySameVesselJoint();
            otherNode->DockToVessel(this);
            otherNode->fsm->RunEvent(otherNode->on_capture_docker.get());
        }
        if (deployAnimator != nullptr && Get<bool>("setAnimWrite")) deployAnimator->SetUIWrite(false);
        if (fsm->LastState() != st_ready.get()) Events()["MakePrimary"]->active = true;
        sameVesselUndockRedock = false;
    });
    fsm->AddState(st_docked_dockee.get());
    st_docked_docker = std::make_unique<KFSMState>("Docked (docker)");
    st_docked_docker->OnEnter = SC::Lambda(this, [this](KFSMState*) {
        if (sameVesselUndockRedock && fsm->LastState() == st_docker_sameVessel.get()) {
            DestroySameVesselJoint();
            DockToVessel(otherNode);
            otherNode->fsm->RunEvent(otherNode->on_capture_dockee.get());
        }
        Events()["Undock"]->active = true;
        Events()["MakePrimary"]->active = false;
        Part* p = PartOf();
        p->fuelLookupTargets.push_back(otherNode->PartOf());
        otherNode->PartOf()->fuelLookupTargets.push_back(p);
        GE::onPartFuelLookupStateChange.Fire(GE::HostedFromToAction<bool, Part*>(true, otherNode->PartOf(), p));
        if (deployAnimator != nullptr && Get<bool>("setAnimWrite")) deployAnimator->SetUIWrite(false);
        sameVesselUndockRedock = false;
    });
    st_docked_docker->OnUpdate = CB::Lambda(this, [this] {
        Events()["Undock"]->active = !IsAdjusterBlockingUndock() && !otherNode->IsAdjusterBlockingUndock();
    });
    st_docked_docker->OnLeave = SC::Lambda(this, [this](KFSMState*) {
        Events()["Undock"]->active = false;
        Part* p = PartOf();
        auto removeFirst = [](std::vector<Part*>& v, Part* x) {  // List.Remove: the first occurrence
            auto it = std::find(v.begin(), v.end(), x);
            if (it != v.end()) v.erase(it);
        };
        removeFirst(p->fuelLookupTargets, otherNode->PartOf());
        removeFirst(otherNode->PartOf()->fuelLookupTargets, p);
        GE::onPartFuelLookupStateChange.Fire(GE::HostedFromToAction<bool, Part*>(true, p, otherNode->PartOf()));
    });
    fsm->AddState(st_docked_docker.get());
    st_docker_sameVessel = std::make_unique<KFSMState>("Docked (same vessel)");
    st_docker_sameVessel->OnEnter = SC::Lambda(this, [this](KFSMState*) {
        Events()["UndockSameVessel"]->active = true;
        Events()["MakePrimary"]->active = false;
        if (!PartOf()->vessel->packed) DockToSameVessel(otherNode);
        sameVesselUndockRedock = false;
    });
    st_docker_sameVessel->OnUpdate = CB::Lambda(this, [this] {
        if (sameVesselDockJoint == nullptr && !PartOf()->vessel->packed) DockToSameVessel(otherNode);
        Events()["UndockSameVessel"]->active = !IsAdjusterBlockingUndock() && !otherNode->IsAdjusterBlockingUndock();
    });
    st_docker_sameVessel->OnLeave = SC::Lambda(this, [this, toggle](KFSMState*) {
        Events()["UndockSameVessel"]->active = false;
        if (sameVesselUndockRedock) {
            if (Get<bool>("animCaptureOff") && deployAnimator) toggle(false);
        } else {
            DestroySameVesselJoint();
            if (fsm->LastEvent() == on_sameVessel_disconnect.get()) {
                otherNode = nullptr;
                vesselInfo.reset();
            }
            sameVesselUndockNode = nullptr;
            sameVesselUndockOtherNode = nullptr;
        }
    });
    fsm->AddState(st_docker_sameVessel.get());
    st_disengage = std::make_unique<KFSMState>("Disengage");
    st_disengage->OnEnter = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (Get<bool>("animDisengageEnter") && deployAnimator) toggle(true);
    });
    st_disengage->OnLeave = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (Get<bool>("animDisengageExit") && deployAnimator) toggle(false);
    });
    fsm->AddState(st_disengage.get());
    st_disabled = std::make_unique<KFSMState>("Disabled");
    st_disabled->OnEnter = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (Get<bool>("animDisabledEnter") && deployAnimator) toggle(true);
    });
    st_disabled->OnLeave = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (Get<bool>("animDisabledExit") && deployAnimator) toggle(false);
    });
    fsm->AddState(st_disabled.get());
    st_preattached = std::make_unique<KFSMState>("PreAttached");
    st_preattached->OnEnter = SC::Lambda(this, [this](KFSMState*) {
        Events()["Undock"]->active = true;
        undockPreAttached = true;
    });
    st_preattached->OnLeave = SC::Lambda(this, [this](KFSMState*) { undockPreAttached = false; });
    fsm->AddState(st_preattached.get());

    on_nodeApproach = std::make_unique<KFSMEvent>("Node Approach");
    on_nodeApproach->updateMode = KFSMUpdateMode::UPDATE;
    on_nodeApproach->GoToStateOnEvent = st_acquire.get();
    on_nodeApproach->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) { return otherNode != nullptr; });
    on_nodeApproach->OnEvent = CB::Lambda(this, [this] {
        Part* p = PartOf();
        dockedPartUId = otherNode->PartOf()->fields.flightID;
        dockingNodeModuleIndex = p == nullptr ? 0 : otherNode->PartOf()->Modules().IndexOf(otherNode);
        if (otherNode->PartOf()->vessel != p->vessel) {
            if (Vessel::GetDominantVessel(p->vessel, otherNode->PartOf()->vessel) == p->vessel) on_nodeApproach->GoToStateOnEvent = st_acquire_dockee.get();
            else on_nodeApproach->GoToStateOnEvent = st_acquire.get();
        } else if (GetDominantNode(this, otherNode) == this) {
            on_nodeApproach->GoToStateOnEvent = st_docked_dockee.get();
        } else {
            on_nodeApproach->GoToStateOnEvent = st_acquire.get();
        }
    });
    fsm->AddEvent(on_nodeApproach.get(), {st_ready.get()});

    on_capture = std::make_unique<KFSMEvent>("Capture");
    on_capture->updateMode = KFSMUpdateMode::UPDATE;
    on_capture->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) {
        Part* p = PartOf();
        Part* op = otherNode->PartOf();
        if (op->vessel != p->vessel) {
            if (!CheckDockContact(this, otherNode, Get<float>("captureRange"), Get<float>("captureMinFwdDot"), Get<float>("captureMinRollDot"))) return false;
            const float sqr = (VelocityOf(*p) - VelocityOf(*op)).sqrMagnitude();
            return fx(sqr) <= fx(Get<float>("captureMaxRvel")) * fx(Get<float>("captureMaxRvel"));
        }
        return CheckDockContact(this, otherNode, Get<float>("captureRange"), Get<float>("captureMinFwdDot"), Get<float>("captureMinRollDot"));
    });
    on_capture->OnEvent = CB::Lambda(this, [this, toggle] {
        Part* p = PartOf();
        Part* op = otherNode->PartOf();
        if (op->vessel != p->vessel) {
            if (Vessel::GetDominantVessel(p->vessel, op->vessel) == p->vessel) {
                on_capture->GoToStateOnEvent = st_docked_dockee.get();
                otherNode->DockToVessel(this);
                otherNode->fsm->RunEvent(otherNode->on_capture_docker.get());
                return;
            }
            on_capture->GoToStateOnEvent = st_docked_docker.get();
            DockToVessel(otherNode);
            otherNode->fsm->RunEvent(otherNode->on_capture_dockee.get());
        } else {
            if (GetDominantNode(this, otherNode) == this) {
                on_capture->GoToStateOnEvent = st_docked_dockee.get();
                otherNode->fsm->RunEvent(otherNode->on_capture_docker_sameVessel.get());
                return;
            }
            on_capture->GoToStateOnEvent = st_docker_sameVessel.get();
            otherNode->fsm->RunEvent(otherNode->on_capture_dockee.get());
        }
        if (Get<bool>("animCaptureOff") && deployAnimator) toggle(false);
    });
    fsm->AddEvent(on_capture.get(), {st_acquire.get()});

    on_capture_dockee = std::make_unique<KFSMEvent>("Capture (dockee)");
    on_capture_dockee->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_capture_dockee->GoToStateOnEvent = st_docked_dockee.get();
    fsm->AddEvent(on_capture_dockee.get(), {st_acquire_dockee.get()});
    fsm->AddEvent(on_capture_dockee.get(), {st_docker_sameVessel.get()});
    fsm->AddEvent(on_capture_dockee.get(), {st_disengage.get()});

    on_capture_docker = std::make_unique<KFSMEvent>("Capture (docker)");
    on_capture_docker->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_capture_docker->OnEvent = CB::Lambda(this, [this, toggle] {
        if (Get<bool>("animCaptureOff") && deployAnimator) toggle(false);
    });
    on_capture_docker->GoToStateOnEvent = st_docked_docker.get();
    fsm->AddEvent(on_capture_docker.get(), {st_acquire.get()});
    fsm->AddEvent(on_capture_docker.get(), {st_docker_sameVessel.get()});
    fsm->AddEvent(on_capture_docker.get(), {st_ready.get()});

    on_capture_docker_sameVessel = std::make_unique<KFSMEvent>("Capture (docker same vessel)");
    on_capture_docker_sameVessel->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_capture_docker_sameVessel->OnEvent = CB::Lambda(this, [this, toggle] {
        if (Get<bool>("animCaptureOff") && deployAnimator) toggle(false);
    });
    on_capture_docker_sameVessel->GoToStateOnEvent = st_docker_sameVessel.get();
    fsm->AddEvent(on_capture_docker_sameVessel.get(), {st_acquire.get()});
    fsm->AddEvent(on_capture_docker_sameVessel.get(), {st_acquire_dockee.get()});

    on_nodeDistance = std::make_unique<KFSMEvent>("Node Distanced");
    on_nodeDistance->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    on_nodeDistance->GoToStateOnEvent = st_ready.get();
    on_nodeDistance->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) { return NodeIsTooFar(); });
    on_nodeDistance->OnEvent = CB::Lambda(this, [this] {
        otherNode = nullptr;
        vesselInfo.reset();
    });
    fsm->AddEvent(on_nodeDistance.get(), {st_acquire.get(), st_acquire_dockee.get(), st_disengage.get(), st_docked_dockee.get()});

    on_undock = std::make_unique<KFSMEvent>("Undock");
    on_undock->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_undock->GoToStateOnEvent = st_disengage.get();
    on_undock->OnEvent = CB::Lambda(this, [this, toggle] {
        if (deployAnimator != nullptr && Get<bool>("setAnimWrite")) deployAnimator->SetUIWrite(true);
        if (Get<bool>("animUndockOn") && deployAnimator) toggle(true);
        on_undock->GoToStateOnEvent = otherNode != nullptr ? st_disengage.get() : st_ready.get();
        Events()["Undock"]->active = false;
    });
    fsm->AddEvent(on_undock.get(), {st_docked_docker.get(), st_docked_dockee.get(), st_preattached.get(), st_docker_sameVessel.get()});

    on_sameVessel_disconnect = std::make_unique<KFSMEvent>("Same Vessel Disconnect");
    on_sameVessel_disconnect->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    on_sameVessel_disconnect->GoToStateOnEvent = st_ready.get();
    on_sameVessel_disconnect->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) {
        Part* p = PartOf();
        if (otherNode->PartOf()->vessel != p->vessel &&
            ((sameVesselUndockNode == nullptr && sameVesselUndockOtherNode == nullptr) || (sameVesselUndockNode != this && sameVesselUndockOtherNode != this))) {
            sameVesselUndockRedock = true;
            otherNode->sameVesselUndockRedock = true;
            if (Vessel::GetDominantVessel(p->vessel, otherNode->PartOf()->vessel) == p->vessel) on_sameVessel_disconnect->GoToStateOnEvent = st_docked_dockee.get();
            else on_sameVessel_disconnect->GoToStateOnEvent = st_docked_docker.get();
            return true;
        }
        sameVesselUndockNode = nullptr;
        sameVesselUndockOtherNode = nullptr;
        on_sameVessel_disconnect->GoToStateOnEvent = st_ready.get();
        return otherNode == nullptr || otherNode->PartOf()->vessel != p->vessel;
    });
    on_sameVessel_disconnect->OnEvent = CB::Lambda(this, [this] {
        if (otherNode != nullptr && !sameVesselUndockRedock) otherNode->OnOtherNodeSameVesselDisconnect();
    });
    fsm->AddEvent(on_sameVessel_disconnect.get(), {st_docker_sameVessel.get()});

    on_disable = std::make_unique<KFSMEvent>("Disable");
    on_disable->updateMode = KFSMUpdateMode::UPDATE;
    on_disable->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) { return deployAnimator != nullptr && Get<bool>("animDisableIfNot1") && deployAnimator->Progress() != 1.0f; });
    on_disable->GoToStateOnEvent = st_disabled.get();
    fsm->AddEvent(on_disable.get(), {st_ready.get(), st_disengage.get()});

    on_enable = std::make_unique<KFSMEvent>("Enable");
    on_enable->updateMode = KFSMUpdateMode::UPDATE;
    on_enable->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) { return deployAnimator != nullptr && Get<bool>("animEnableIf1") && deployAnimator->Progress() == 1.0f; });
    on_enable->GoToStateOnEvent = st_ready.get();
    fsm->AddEvent(on_enable.get(), {st_disabled.get()});

    on_preattachedDecouple = std::make_unique<KFSMEvent>("Pre-Attached Part Decoupled");
    on_preattachedDecouple->updateMode = KFSMUpdateMode::UPDATE;
    on_preattachedDecouple->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) { return referenceNode->attachedPart == nullptr; });
    on_preattachedDecouple->GoToStateOnEvent = st_ready.get();
    fsm->AddEvent(on_preattachedDecouple.get(), {st_preattached.get()});

    on_swapPrimary = std::make_unique<KFSMEvent>("SwapPrimary");
    on_swapPrimary->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_swapPrimary->GoToStateOnEvent = st_docked_docker.get();
    fsm->AddEvent(on_swapPrimary.get(), {st_docked_dockee.get()});

    on_swapSecondary = std::make_unique<KFSMEvent>("SwapSecondary");
    on_swapSecondary->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_swapSecondary->GoToStateOnEvent = st_docked_dockee.get();
    fsm->AddEvent(on_swapSecondary.get(), {st_docked_docker.get()});

    const std::vector<KFSMState*> allStates = {st_ready.get(), st_acquire.get(), st_acquire_dockee.get(), st_docked_dockee.get(), st_docked_docker.get(),
                                               st_docker_sameVessel.get(), st_disengage.get(), st_disabled.get(), st_preattached.get()};
    on_construction_Attach = std::make_unique<KFSMEvent>("OnConstructionAttach");
    on_construction_Attach->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_construction_Attach->GoToStateOnEvent = st_preattached.get();
    on_construction_Attach->OnEvent = CB::Lambda(this, [] {});
    fsm->AddEvent(on_construction_Attach.get(), allStates);

    on_construction_Detach = std::make_unique<KFSMEvent>("OnConstructionDetach");
    on_construction_Detach->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_construction_Detach->GoToStateOnEvent = st_ready.get();
    on_construction_Detach->OnEvent = CB::Lambda(this, [] {});
    fsm->AddEvent(on_construction_Detach.get(), allStates);
}

// ModuleDockingNode.cs:1223
void ModuleDockingNode::OnFSMStateChange(KFSMState* oldState, KFSMState* newState, KFSMEvent* fsmEvent) {
    if (DebugFSMState) {
        Part* p = PartOf();
        log::Info("[ModuleDockingNode]: Part:" + (p != nullptr && p->partInfo != nullptr ? p->partInfo->title : std::string()) + "-" +
                  std::to_string(p != nullptr ? p->fields.persistentId : 0u) + " FSM State Changed, Old State:" + oldState->name + " New State:" + newState->name +
                  " Event:" + fsmEvent->name);
    }
}

// ModuleDockingNode.cs:1231
void ModuleDockingNode::OnFSMEventCalled(KFSMEvent* fsmEvent) {
    if (DebugFSMState) {
        Part* p = PartOf();
        log::Info("[ModuleDockingNode]: Part:" + (p != nullptr && p->partInfo != nullptr ? p->partInfo->title : std::string()) + "-" +
                  std::to_string(p != nullptr ? p->fields.persistentId : 0u) + " FSM Event Called, Event:" + fsmEvent->name);
    }
}

// ModuleDockingNode.cs:1239
void ModuleDockingNode::Update() {
    if (physicsLessMode) return;
    if (fsm != nullptr && fsm->Started()) {
        fsm->UpdateFSM();
        state = fsm->currentStateName;
    }
    if (DockingEnvironment::SceneIsFlight()) {
        ITargetable* target = DockingEnvironment::vesselTarget ? DockingEnvironment::vesselTarget() : nullptr;
        if (target == static_cast<ITargetable*>(this)) {
            evtSetAsTarget->active = false;
            evtUnsetTarget->active = true;
            Part* p = PartOf();
            if (FlightRegistry::ActiveVessel == p->vessel) {
                if (DockingEnvironment::setVesselTarget) DockingEnvironment::setVesselTarget(nullptr);
            } else if (FlightRegistry::ActiveVessel != nullptr &&
                       fx((FlightRegistry::ActiveVessel->vesselTransform.position - NodePose().position).sqrMagnitude()) > 40000.0) {
                if (DockingEnvironment::setVesselTarget) DockingEnvironment::setVesselTarget(p->vessel);
            }
        } else {
            evtSetAsTarget->active = true;
            evtUnsetTarget->active = false;
        }
    }
    if (partActionMenuOpen) UpdatePAWUI();
}

// ModuleDockingNode.cs:1277
void ModuleDockingNode::FixedUpdate() {
    if (physicsLessMode) return;
    if (fsm != nullptr && fsm->Started()) fsm->FixedUpdateFSM();
    Part* p = PartOf();
    if (Get<bool>("canRotate") && DockingEnvironment::SceneIsFlight() && p != nullptr && p->vessel != nullptr && !p->vessel->packed) {
        UpdateAlignmentRotation();
        if (IsRotating && hasEnoughResources) CalculateResourceDrain();
        else if (!hasEnoughResources && resHandler.HasEnoughResourcesToAutoStart()) hasEnoughResources = true;
    }
}

// ModuleDockingNode.cs:1301
void ModuleDockingNode::LateUpdate() {
    if (!physicsLessMode && fsm != nullptr && fsm->Started()) fsm->LateUpdateFSM();
}

// ModuleDockingNode.cs:1316
void ModuleDockingNode::UpdatePAWUI() {
    const bool linked = otherNode != nullptr && sameVesselDockJoint == nullptr;
    if (DockingEnvironment::SceneIsFlight() && Get<bool>("canRotate")) {
        Fields()["targetAngle"]->guiActive = linked;
        Fields()["inverted"]->guiActive = linked;
        Fields()["nodeIsLocked"]->guiActive = linked;
        Fields()["targetAngle"]->guiActiveEditor = linked;
        Fields()["inverted"]->guiActiveEditor = linked;
        Fields()["nodeIsLocked"]->guiActiveEditor = linked;
    } else {
        Fields()["targetAngle"]->guiActive = false;
        Fields()["inverted"]->guiActive = false;
        Fields()["nodeIsLocked"]->guiActive = false;
        Fields()["targetAngle"]->guiActiveEditor = false;
        Fields()["inverted"]->guiActiveEditor = false;
        Fields()["nodeIsLocked"]->guiActiveEditor = linked;
    }
}

// ModuleDockingNode.cs:1338
ModuleDockingNode* ModuleDockingNode::FindNodeApproaches() {
    Part* p = PartOf();
    if (p->fields.packed) return nullptr;
    int count = static_cast<int>(FlightRegistry::VesselsLoaded.size());
    while (count-- > 0) {
        Vessel* vessel = FlightRegistry::VesselsLoaded[static_cast<size_t>(count)];
        if (vessel->packed) continue;
        const int count2 = static_cast<int>(vessel->dockingPorts.size());
        if (count2 == 0) continue;
        int index = count2;
        while (index-- > 0) {
            PartModule* partModule = vessel->dockingPorts[static_cast<size_t>(index)];
            auto* other = dynamic_cast<ModuleDockingNode*>(partModule);
            if (partModule->part == part || partModule->part == nullptr) continue;
            Part* otherPart = dynamic_cast<Part*>(partModule->part);
            if (otherPart == nullptr || otherPart->State() == PartStates::DEAD || other == nullptr) continue;
            if (other->state != st_ready->name) continue;
            if (CanDockWith(*other)) {
                if (CheckDockContact(this, other, Get<float>("acquireRange"), Get<float>("acquireMinFwdDot"), Get<float>("acquireMinRollDot"))) return other;
            }
        }
    }
    return nullptr;
}

// kspcore helper: the matching expression inside FindNodeApproaches (ModuleDockingNode.cs lines 1257-1270)
bool ModuleDockingNode::CanDockWith(ModuleDockingNode& other) {
    bool flag = true;
    for (const std::string& t : nodeTypes) flag &= other.nodeTypes.count(t) == 0;
    const bool gendered = Get<bool>("gendered");
    const bool snapRotation = Get<bool>("snapRotation");
    return !flag && other.Get<bool>("gendered") == gendered && (!gendered || other.Get<bool>("genderFemale") != Get<bool>("genderFemale")) &&
           other.Get<bool>("snapRotation") == snapRotation && (!snapRotation || other.Get<float>("snapOffset") == Get<float>("snapOffset"));
}

// kspcore helper: the pose maths of CheckDockContact (ModuleDockingNode.cs line 1390), with the intermediate roll dot reported
bool ModuleDockingNode::DockContact(const DockPose& p1, const DockPose& p2, bool snapRotation, float snapOffset, float minDist, float minFwdDot, float minRollDot,
                                    float* rollDot) {
    if (rollDot != nullptr) *rollDot = 0.0f;
    if (fx((p1.position - p2.position).sqrMagnitude()) < fx(minDist) * fx(minDist) && Vector3f::Dot(p1.forward, -p2.forward) > minFwdDot) {
        float num = Vector3f::Dot(p1.up, p2.up);
        if (snapRotation) {
            double num2 = dotnet::Math::Acos(static_cast<double>(num));
            if (num2 != num2) {  // double.IsNaN
                num2 = !(num > 0.0f) ? UtilMath::PI : 0.0;
            } else if (Vector3f::Dot(p1.up, p2.right) > 0.0f) {
                num2 = UtilMath::PI * 2.0 - num2;
            }
            const double num3 = UtilMath::DegreesToRadians(static_cast<double>(snapOffset));
            if (num3 > 0.0) {
                while (num2 > num3) num2 -= num3;
            }
            num = f32store(dotnet::Math::Cos(num2));
        }
        if (rollDot != nullptr) *rollDot = num;
        if (num > minRollDot) return true;
    }
    return false;
}

// ModuleDockingNode.cs:1390
bool ModuleDockingNode::CheckDockContact(ModuleDockingNode* m1, ModuleDockingNode* m2, float minDist, float minFwdDot, float minRollDot) {
    return DockContact(m1->NodePose(), m2->NodePose(), m1->Get<bool>("snapRotation"), m1->Get<float>("snapOffset"), minDist, minFwdDot, minRollDot);
}

// ModuleDockingNode.cs:1424
void ModuleDockingNode::DockToVessel(ModuleDockingNode* node) {
    Part* p = PartOf();
    Part* np = node->PartOf();
    log::Info("Docking to vessel " + np->vessel->GetDisplayName());
    vesselInfo = DockedVesselInfo();
    vesselInfo->name = p->vessel->vesselName;
    vesselInfo->vesselType = p->vessel->vesselType;
    vesselInfo->rootPartUId = p->vessel->rootPart->fields.flightID;
    node->vesselInfo = DockedVesselInfo();
    node->vesselInfo->name = np->vessel->vesselName;
    node->vesselInfo->vesselType = np->vessel->vesselType;
    node->vesselInfo->rootPartUId = np->vessel->rootPart->fields.flightID;
    Vessel* vessel = p->vessel;
    const bool vesselWasActive = vessel == FlightRegistry::ActiveVessel;  // the comparison C# makes after Couple, on the (then destroyed) old vessel
    const uint32_t data = p->vessel->persistentId;
    const uint32_t data2 = np->vessel->persistentId;
    GE::onVesselDocking.Fire(data, data2);
    GE::onActiveJointNeedUpdate.Fire(np->vessel);
    GE::onActiveJointNeedUpdate.Fire(p->vessel);
    const DockPose mine = NodePose();
    const DockPose theirs = node->NodePose();
    np->vessel->SetRotation(np->vessel->vesselTransform.rotation);
    p->vessel->SetRotation(UnityNative::FromToRotation(mine.forward, -theirs.forward) * p->vessel->vesselTransform.rotation);
    p->vessel->SetPosition(Vector3d(p->vessel->vesselTransform.position - (mine.position - theirs.position)), true);
    p->vessel->IgnoreGForces(10);
    p->Couple(np);
    GE::onVesselPersistentIdChanged.Fire(data, data2);
    if (vesselWasActive) {
        ForceSetActive(p->vessel);
        SetNeutral();
    } else if (p->vessel == FlightRegistry::ActiveVessel) {
        p->vessel->MakeActive();
        SetNeutral();
    }
    // FlightGlobals.PersistentLoadedPartIds / PersistentUnloadedPartIds registration of the old vessel's parts: no such registry in kspcore
    GE::onVesselWasModified.Fire(p->vessel);
    GE::onDockingComplete.Fire(GE::FromToAction<Part*, Part*>(p, np));
}

// ModuleDockingNode.cs:1470
void ModuleDockingNode::MakePrimary() {
    if (otherNode != nullptr) {
        otherNode->fsm->RunEvent(otherNode->on_swapSecondary.get());
        fsm->RunEvent(on_swapPrimary.get());
    }
}

// ModuleDockingNode.cs:1480
void ModuleDockingNode::UndockAction(KSPActionParam&) {
    if (Events()["Undock"]->active) Undock();
    if (Events()["UndockSameVessel"]->active) UndockSameVessel();
}

// ModuleDockingNode.cs:1493
void ModuleDockingNode::Undock() {
    Part* p = PartOf();
    Part* parent = p->parent;
    Vessel* vessel = p->vessel;
    const uint32_t referenceTransformId = p->vessel->referenceTransformId;
    sameVesselUndockNode = this;
    sameVesselUndockOtherNode = otherNode;
    if (undockPreAttached) {
        Decouple();
        fsm->RunEvent(on_undock.get());
        if (otherNode != nullptr) otherNode->OnOtherNodeUndock();
        undockPreAttached = false;
        return;
    }
    if (otherNode == nullptr) throw Nre();
    if (parent != otherNode->PartOf()) {
        otherNode->Undock();
        return;
    }
    const DockPose pose = NodePose();
    const float ejection = Get<float>("undockEjectionForce");
    p->Undock(vesselInfo ? &*vesselInfo : nullptr);
    AddForceOn(*p, pose.forward * f32store((0.0 - fx(ejection)) * 0.5));
    AddForceOn(*parent, pose.forward * f32store(fx(ejection) * 0.5));
    if (vessel == FlightRegistry::ActiveVessel && vessel->byFlightID(referenceTransformId) == nullptr) StartCoroutine(1, [this] { WaitAndSwitchFocus(); });
    fsm->RunEvent(on_undock.get());
    otherNode->OnOtherNodeUndock();
}

// ModuleDockingNode.cs:1527 (after the first yield)
void ModuleDockingNode::WaitAndSwitchFocus() {
    ForceSetActive(PartOf()->vessel);
    SetNeutral();
}

// ModuleDockingNode.cs:1534
void ModuleDockingNode::OnOtherNodeUndock() { fsm->RunEvent(on_undock.get()); }

// ModuleDockingNode.cs:1539
void ModuleDockingNode::UpdateAlignmentRotation() {
    const Vector2f lim = Get<Vector2f>("hardMinMaxLimits");
    const float targetAngle = Get<float>("targetAngle");
    const void* joint = RotationJoint();
    if (!Get<bool>("canRotate") || !hasEnoughResources || !rotationInitComplete || joint == nullptr || targetAngle < lim.x || !(targetAngle <= lim.y)) return;
    maxAnglePerFrame = MulF(Get<float>("traverseVelocity"), DockingEnvironment::fixedDeltaTime);
    if (!(driveTargetAngle == JointTargetAngle())) {
        if (!IsRotating) {
            IsRotating = true;
            SetJointHighLowLimits();
        }
        driveTargetAngle = Mathf::MoveTowards(driveTargetAngle, JointTargetAngle(), maxAnglePerFrame);
        visualTargetAngle = Mathf::MoveTowards(visualTargetAngle, VisualTargetAngle(), maxAnglePerFrame);
        const void* own = DockingEnvironment::attachJointOf ? DockingEnvironment::attachJointOf(*PartOf()) : nullptr;
        if (joint == own) {
            if (!partJointUnbreakable) {
                if (DockingEnvironment::scaleBreakForces) DockingEnvironment::scaleBreakForces(joint, 10.0f, false);
                partJointUnbreakable = true;
            }
            const Quaternionf targetLocalRotation = SetTargetRotation(Quaternionf::identity(), f32store(fx(driveTargetAngle) - fx(cachedInitialAngle)), true, Vector3f(0.0f, 1.0f, 0.0f));
            if (DockingEnvironment::setTargetRotationLocal) DockingEnvironment::setTargetRotationLocal(joint, targetLocalRotation);
            ApplyCoordsUpdate();
            targetRotation = SetTargetRotation(Quaternionf::identity(), visualTargetAngle, false);
            if (DockingEnvironment::setRotationTransformLocalRotation) DockingEnvironment::setRotationTransformLocalRotation(*this, initialRotation, targetRotation);
        } else {
            targetRotation = SetTargetRotation(Quaternionf::identity(), f32store(0.0 - fx(visualTargetAngle)), false);
            if (DockingEnvironment::setRotationTransformLocalRotation) DockingEnvironment::setRotationTransformLocalRotation(*this, initialRotation, targetRotation);
        }
    } else {
        IsRotating = false;
        if (partJointUnbreakable) {
            if (DockingEnvironment::scaleBreakForces) DockingEnvironment::scaleBreakForces(joint, 10.0f, true);
            partJointUnbreakable = false;
        }
    }
}

// ModuleDockingNode.cs:1592
Quaternionf ModuleDockingNode::SetTargetRotation(const Quaternionf& startingRotation, float rotationAngle, bool setRotation) {
    return SetTargetRotation(startingRotation, rotationAngle, setRotation, GetRotationAxis());
}

// ModuleDockingNode.cs:1592
Quaternionf ModuleDockingNode::SetTargetRotation(const Quaternionf& startingRotation, float rotationAngle, bool setRotation, const Vector3f& rotationAxis) {
    const Quaternionf quaternion = startingRotation;
    if (setRotation) return UnityNative::AngleAxis(rotationAngle, rotationAxis);
    return quaternion * UnityNative::AngleAxis(rotationAngle, rotationAxis);
}

// ModuleDockingNode.cs:1602
Vector3f ModuleDockingNode::GetRotationAxis() {
    const std::string ax = Get<std::string>("rotationAxis");
    if (ax == "Z-") return Vector3f(0.0f, 0.0f, -1.0f);
    if (ax == "Y") return Vector3f(0.0f, 1.0f, 0.0f);
    if (ax == "Y-") return Vector3f(0.0f, -1.0f, 0.0f);
    if (ax == "X") return Vector3f(1.0f, 0.0f, 0.0f);
    if (ax == "X-") return Vector3f(-1.0f, 0.0f, 0.0f);
    return Vector3f(0.0f, 0.0f, 1.0f);
}

// ModuleDockingNode.cs:1622
void ModuleDockingNode::CalculateResourceDrain() {
    if (DockingEnvironment::SceneIsEditor()) return;
    float num = motorRate;
    if (std::abs(static_cast<double>(num)) < 0.001) num = 0.0f;
    double rateMultiplier, rateMultiplier2;
    if (num >= 0.0f) {
        rateMultiplier = num;
        rateMultiplier2 = 0.0;
    } else {
        rateMultiplier = 0.0;
        rateMultiplier2 = 0.0 - static_cast<double>(num);
    }
    std::string error;
    hasEnoughResources = resHandler.UpdateModuleResourceInputs(error, true, rateMultiplier, 0.999, true, false);
    resHandler.UpdateModuleResourceOutputs(rateMultiplier2);
    if (!hasEnoughResources) {
        num = 0.0f;
        IsRotating = false;
    }
}

// ModuleDockingNode.cs:1654
void ModuleDockingNode::SetJointHighLowLimits() {
    if (const void* joint = RotationJoint()) {
        const Vector2f lim = Get<Vector2f>("hardMinMaxLimits");
        if (DockingEnvironment::limitAngularXMotion) DockingEnvironment::limitAngularXMotion(joint);
        lowerRotationLimit = f32store(fx(lim.x) * 2.0 - 5.0);
        upperRotationLimit = f32store(fx(lim.y) * 2.0 + 5.0);
        if (DockingEnvironment::setAngularXLimits) DockingEnvironment::setAngularXLimits(joint, lowerRotationLimit, upperRotationLimit);
    }
}

// ModuleDockingNode.cs:1669
void ModuleDockingNode::ApplyCoordsUpdate() {
    Part* p = PartOf();
    if (DockingEnvironment::SceneIsFlight() && Get<bool>("canRotate") && !Get<bool>("nodeIsLocked") && otherNode != nullptr && p != nullptr && p->vessel != nullptr)
        RecurseCoordUpdate(p->vessel->rootPart, p->vessel->rootPart);
}

// ModuleDockingNode.cs:1677
void ModuleDockingNode::RecurseCoordUpdate(Part* p, Part* rootPart) {
    if (p != nullptr && rootPart != nullptr) {
        p->UpdateOrgPosAndRot(rootPart);
        for (size_t i = 0; i < p->children.size(); i++) RecurseCoordUpdate(p->children[i], rootPart);
    }
}

// ModuleDockingNode.cs:1694
ModuleDockingNode* ModuleDockingNode::GetDominantNode(ModuleDockingNode* m1, ModuleDockingNode* m2) {
    const std::vector<Part*>& parts = PartOf()->vessel->parts;
    auto indexOf = [&parts](Part* x) {
        auto it = std::find(parts.begin(), parts.end(), x);
        return it == parts.end() ? -1 : static_cast<int>(it - parts.begin());
    };
    if (indexOf(m1->PartOf()) > indexOf(m2->PartOf())) return m1;
    return m2;
}

// ModuleDockingNode.cs:1703
void ModuleDockingNode::DockToSameVessel(ModuleDockingNode* node) {
    if (DockingEnvironment::createSameVesselJoint) sameVesselDockJoint = DockingEnvironment::createSameVesselJoint(*this, *node);
    else sameVesselDockJoint = std::make_shared<int>(0);  // PartJoint.Create: the joint itself is WP-20's
    GE::onSameVesselDock.Fire(GE::FromToAction<ModuleDockingNode*, ModuleDockingNode*>(this, node));
}

// ModuleDockingNode.cs:1709
void ModuleDockingNode::DestroySameVesselJoint() {
    if (sameVesselDockJoint != nullptr) {
        if (DockingEnvironment::destroySameVesselJoint) DockingEnvironment::destroySameVesselJoint(*this, sameVesselDockJoint);
        sameVesselDockJoint.reset();  // PartJoint.DestroyJoint (the C# field keeps the destroyed object; the "== null" tests are Unity's destroyed-object test)
    }
    if (!sameVesselUndockRedock) {
        otherNode->OnOtherNodeUndock();
        GE::onSameVesselUndock.Fire(GE::FromToAction<ModuleDockingNode*, ModuleDockingNode*>(this, otherNode));
    }
}

// ModuleDockingNode.cs:1723
void ModuleDockingNode::UndockSameVessel() {
    fsm->RunEvent(on_undock.get());
    otherNode->OnOtherNodeUndock();
}

// ModuleDockingNode.cs:1729
void ModuleDockingNode::OnOtherNodeSameVesselDisconnect() { fsm->RunEvent(on_nodeDistance.get()); }

// ModuleDockingNode.cs:1734
bool ModuleDockingNode::NodeIsTooFar() {
    if (otherNode == nullptr) {
        log::Warning("[DockingNode]: other node is null!");
        return true;
    }
    if (!otherNode->nodeTransformPresent) {
        log::Warning("[DockingNode]: other node transform is null!");
        return true;
    }
    const float d = Get<float>("minDistanceToReEngage");
    return fx((NodePose().position - otherNode->NodePose().position).sqrMagnitude()) > fx(d) * fx(d);
}

// ModuleDockingNode.cs:1751
void ModuleDockingNode::DecoupleAction(KSPActionParam&) {
    if (Events()["Decouple"]->active) Decouple();
}

// ModuleDockingNode.cs:1760
void ModuleDockingNode::Decouple() {
    if (referenceNode == nullptr) throw Nre();
    Part* attachedPart = referenceNode->attachedPart;
    if (attachedPart == nullptr) return;
    Part* p = PartOf();
    sameVesselUndockNode = this;
    sameVesselUndockOtherNode = otherNode;
    if (referenceNode->attachedPart == p->parent) p->decouple();
    else referenceNode->attachedPart->decouple();
    const DockPose pose = NodePose();
    const float ejection = Get<float>("undockEjectionForce");
    AddForceOn(*p, pose.forward * f32store((0.0 - fx(ejection)) * 0.5));
    AddForceOn(*attachedPart, pose.forward * f32store(fx(ejection) * 0.5));
    fsm->RunEvent(on_undock.get());
    if (otherNode != nullptr) otherNode->OnOtherNodeUndock();
    if (Get<bool>("stagingEnabled")) SetStaging(false);
}

// ModuleDockingNode.cs:1788
void ModuleDockingNode::OnActive() {
    if (Get<bool>("staged") && Get<bool>("stagingEnabled")) Decouple();
}

// ModuleDockingNode.cs:1796
void ModuleDockingNode::OnConstructionAttach() {
    if (fsm->Started()) fsm->RunEvent(on_construction_Attach.get());
}

// ModuleDockingNode.cs:1804 GetTransform (the node transform's pose mirror)
UnityEngine::Transform* ModuleDockingNode::GetTransform() {
    nodeTransformMirror.name = Get<std::string>("nodeTransformName");
    if (nodeTransformPresent) nodeTransformMirror.position = NodePose().position;
    return &nodeTransformMirror;
}

// ModuleDockingNode.cs:1809
Vector3f ModuleDockingNode::GetObtVelocity() { return PartOf()->vessel->obt_velocity.ToVector3f(); }

// ModuleDockingNode.cs:1814
Vector3f ModuleDockingNode::GetSrfVelocity() { return PartOf()->vessel->srf_velocity.ToVector3f(); }

// ModuleDockingNode.cs:1819
Vector3f ModuleDockingNode::GetFwdVector() { return NodePose().forward; }

// ModuleDockingNode.cs:1824
Vessel* ModuleDockingNode::GetVessel() { return PartOf() != nullptr ? PartOf()->vessel : nullptr; }

// ModuleDockingNode.cs:1829
std::string ModuleDockingNode::GetName() { return Localizer::Format("#autoLOC_241797", std::vector<std::string>{PartOf()->vessel->vesselName}); }

// ModuleDockingNode.cs:1834
std::string ModuleDockingNode::GetDisplayName() { return GetName(); }

// ModuleDockingNode.cs:1839
Orbit* ModuleDockingNode::GetOrbit() { return &PartOf()->vessel->orbit(); }

// ModuleDockingNode.cs:1844
OrbitDriver* ModuleDockingNode::GetOrbitDriver() { return PartOf()->vessel->orbitDriver.get(); }

// ModuleDockingNode.cs:1860
void ModuleDockingNode::SetAsTarget() {
    if (DockingEnvironment::setVesselTarget) DockingEnvironment::setVesselTarget(this);
}

// ModuleDockingNode.cs:1866
void ModuleDockingNode::UnsetTarget() {
    if (DockingEnvironment::setVesselTarget) DockingEnvironment::setVesselTarget(nullptr);
}

// ModuleDockingNode.cs:1872
void ModuleDockingNode::EnableXFeed() {
    Events()["EnableXFeed"]->active = false;
    Events()["DisableXFeed"]->active = true;
    Part* p = PartOf();
    const bool fuelCrossFeed = p->fields.fuelCrossFeed;
    Set<bool>("crossfeed", true);
    p->fields.fuelCrossFeed = true;
    if (fuelCrossFeed != Get<bool>("crossfeed")) GE::onPartCrossfeedStateChange.Fire(p);
}

// ModuleDockingNode.cs:1887
void ModuleDockingNode::DisableXFeed() {
    Events()["EnableXFeed"]->active = true;
    Events()["DisableXFeed"]->active = false;
    Part* p = PartOf();
    const bool fuelCrossFeed = p->fields.fuelCrossFeed;
    Set<bool>("crossfeed", false);
    p->fields.fuelCrossFeed = false;
    if (fuelCrossFeed != Get<bool>("crossfeed")) GE::onPartCrossfeedStateChange.Fire(p);
}

// ModuleDockingNode.cs:1902
void ModuleDockingNode::MakeReferenceToggle(KSPActionParam&) { MakeReferenceTransform(); }

// ModuleDockingNode.cs:1908
void ModuleDockingNode::MakeReferenceTransform() {
    Part* p = PartOf();
    p->SetReferenceTransformName(controlTransformName_.empty() ? p->gameObjectName : controlTransformName_);  // part.SetReferenceTransform(controlTransform)
    p->vessel->SetReferenceTransform(p);
}

// ModuleDockingNode.cs:1915
void ModuleDockingNode::EnableXFeedAction(KSPActionParam&) { EnableXFeed(); }

// ModuleDockingNode.cs:1921
void ModuleDockingNode::DisableXFeedAction(KSPActionParam&) { DisableXFeed(); }

// ModuleDockingNode.cs:1927
void ModuleDockingNode::ToggleXFeedAction(KSPActionParam&) {
    if (Get<bool>("crossfeed")) DisableXFeed();
    else EnableXFeed();
}

// ModuleDockingNode.cs:1939
std::string ModuleDockingNode::GetInfo() {
    return std::string("") + Localizer::Format("#autoLOC_241887", std::vector<std::string>{FormatForce(Get<float>("captureRange"))}) +
           Localizer::Format("#autoLOC_241888", std::vector<std::string>{FormatForce(Get<float>("undockEjectionForce"))});
}

// ModuleDockingNode.cs:1949
bool ModuleDockingNode::IsStageable() { return Get<bool>("staged"); }

// ModuleDockingNode.cs:1954
bool ModuleDockingNode::StagingEnabled() { return Get<bool>("stagingEnabled") ? Get<bool>("staged") : false; }

// ModuleDockingNode.cs:1963
bool ModuleDockingNode::StagingToggleEnabledEditor() { return Get<bool>("staged"); }

// ModuleDockingNode.cs:1968
bool ModuleDockingNode::StagingToggleEnabledFlight() { return Get<bool>("staged"); }

// ModuleDockingNode.cs:1973
std::string ModuleDockingNode::GetStagingEnableText() {
    const std::string t = Get<std::string>("stagingEnableText");
    if (!t.empty()) return t;
    return Localizer::Format("#autoLOC_241913");
}

// ModuleDockingNode.cs:1981
std::string ModuleDockingNode::GetStagingDisableText() {
    const std::string t = Get<std::string>("stagingDisableText");
    if (!t.empty()) return t;
    return Localizer::Format("#autoLOC_241914");
}

// ModuleDockingNode.cs:2044
std::string ModuleDockingNode::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003039"); }

// ModuleDockingNode.cs:2051
bool ModuleDockingNode::IsJointUnlocked() {
    if ((otherNode == nullptr && sameVesselDockJoint == nullptr) || !Get<bool>("canRotate")) return false;
    return !Get<bool>("nodeIsLocked");
}

namespace detail {
void RegisterDockingModules() {
    ModuleFactory::Register("ModuleDockingNode", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleDockingNode>(c)); });
}
}  // namespace detail

}  // namespace ksp
