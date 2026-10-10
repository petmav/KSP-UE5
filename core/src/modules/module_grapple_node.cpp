// WP-25: ModuleGrappleNode.cs (see include/ksp/modules/module_grapple_node.hpp). Every function cites ModuleGrappleNode.cs:<line> of
// decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/modules/module_grapple_node.hpp"

#include <algorithm>
#include <unordered_map>

#include "ksp/config/kspparseable.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/animation_modules.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = GameEvents;
using KSP::Localization::Localizer;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }

std::string FormatRange(float v) {
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
void ForceSetActive(Vessel* v) {
    if (ModuleDecouplerBase::forceSetActiveVessel) ModuleDecouplerBase::forceSetActiveVessel(v);
}
void SetNeutral() {
    if (DockingEnvironment::setNeutralControls) DockingEnvironment::setNeutralControls();
}
Vector3f VelocityOf(Part& p) { return DockingEnvironment::partVelocity ? DockingEnvironment::partVelocity(p) : Vector3f(); }
}  // namespace

ModuleGrappleNode::ModuleGrappleNode(const ModuleClassInfo& cls) : PartModule(cls) { live_.push_back(this); }

ModuleGrappleNode::~ModuleGrappleNode() { live_.erase(std::remove(live_.begin(), live_.end(), this), live_.end()); }

Part* ModuleGrappleNode::PartOf() const { return dynamic_cast<Part*>(part); }

void ModuleGrappleNode::TickAll() {
    std::vector<ModuleGrappleNode*> mods(live_);
    for (ModuleGrappleNode* m : mods) {
        if (std::find(live_.begin(), live_.end(), m) == live_.end()) continue;
        std::vector<Delayed> pending;
        pending.swap(m->delayed_);
        for (Delayed& d : pending) {
            if (--d.ticks <= 0) d.fn();
            else m->delayed_.push_back(std::move(d));
        }
    }
}

// the node Transform of FindModelTransform(nodeTransformName) through the seam
DockPose ModuleGrappleNode::NodePose() {
    Part* p = PartOf();
    if (p == nullptr || !DockingEnvironment::modelTransformPose) throw Nre();
    std::optional<DockPose> pose = DockingEnvironment::modelTransformPose(*p, Get<std::string>("nodeTransformName"));
    if (!pose) throw Nre();
    return *pose;
}

// `grappleNode` (a C# reference): the module's own object until it is in a part's attachNodes, then that list's element with the id "grapple"
AttachNode* ModuleGrappleNode::GrappleNode() {
    if (grappleRegistered_ && grappleOwner_ != nullptr) {
        for (AttachNode& n : grappleOwner_->attachNodes)
            if (n.id == "grapple") return &n;
    }
    return grappleNode_.get();
}

void ModuleGrappleNode::AddGrappleNodeTo(Part& owner) {
    AttachNode* n = grappleNode_.get();
    if (n == nullptr) throw Nre();
    owner.attachNodes.push_back(*n);
    grappleOwner_ = &owner;
    grappleRegistered_ = true;
    grappleNode_.reset();
}

void ModuleGrappleNode::RemoveGrappleNodeFrom(Part& owner) {
    // List<AttachNode>.Remove(grappleNode): the element leaves the list, the module keeps the object
    for (auto it = owner.attachNodes.begin(); it != owner.attachNodes.end(); ++it)
        if (it->id == "grapple") {
            grappleNode_ = std::make_unique<AttachNode>(*it);
            owner.attachNodes.erase(it);
            break;
        }
    grappleRegistered_ = false;
    grappleOwner_ = nullptr;
}

// ModuleGrappleNode.cs:110
bool ModuleGrappleNode::IsDisabled() {
    if (fsm != nullptr && fsm->Started()) return fsm->CurrentState() == st_disabled.get();
    return false;
}

void ModuleGrappleNode::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("Release", BaseEvent::Handler([this] { Release(); }));
    Events().Bind("ReleaseSameVessel", BaseEvent::Handler([this] { ReleaseSameVessel(); }));
    Events().Bind("Decouple", BaseEvent::Handler([this] { Decouple(); }));
    Events().Bind("MakeReferenceTransform", BaseEvent::Handler([this] { MakeReferenceTransform(); }));
    Events().Bind("SetLoose", BaseEvent::Handler([this] { SetLoose(); }));
    Events().Bind("LockPivot", BaseEvent::Handler([this] { LockPivot(); }));
    Actions().Bind("ReleaseAction", [this](KSPActionParam& p) { ReleaseAction(p); });
    Actions().Bind("DecoupleAction", [this](KSPActionParam& p) { DecoupleAction(p); });
    Actions().Bind("MakeReferenceToggle", [this](KSPActionParam& p) { MakeReferenceToggle(p); });
}

// ModuleGrappleNode.cs:122
void ModuleGrappleNode::OnLoad(const ConfigNode& node) {
    if (node.HasValue("state")) state = *node.GetValue("state");
    if (node.HasValue("dockUId")) {
        uint32_t v = 0;
        if (!dotnet::TryParseUInt32(*node.GetValue("dockUId"), v)) throw dotnet::FormatException("Input string was not in a correct format.");  // uint.Parse
        dockedPartUId = v;
    }
    if (DockingEnvironment::SceneIsFlight()) {
        grappleNode_ = std::make_unique<AttachNode>();
        grappleNode_->size = 1;
        grappleNode_->id = "grapple";
        if (Part* p = PartOf()) p->attachNodes.reserve(p->attachNodes.size() + 1);  // see the header: keeps the pointers into the list valid
    }
    if (node.HasValue("grapplePos")) grabPos = KSPUtil::ParseVector3(*node.GetValue("grapplePos"));
    if (node.HasValue("grappleOrt")) grabOrt = KSPUtil::ParseVector3(*node.GetValue("grappleOrt"));
    if (node.HasValue("grappleOrt2")) grabOrt2 = KSPUtil::ParseVector3(*node.GetValue("grappleOrt2"));
    node.TryGetValue("grappledSameVessel", grappledSameVessel);
    if (node.HasNode("DOCKEDVESSEL")) {
        vesselInfo = DockedVesselInfo();
        vesselInfo->Load(*node.GetNode("DOCKEDVESSEL"));
    }
    if (node.HasNode("DOCKEDVESSEL_Other")) {
        otherVesselInfo = DockedVesselInfo();
        otherVesselInfo->Load(*node.GetNode("DOCKEDVESSEL_Other"));
    }
}

// ModuleGrappleNode.cs:163
void ModuleGrappleNode::OnSave(ConfigNode& node) {
    node.AddValue("state", (fsm == nullptr || !fsm->Started()) ? std::string("Ready") : fsm->currentStateName);
    node.AddValue("dockUId", dockedPartUId);
    if (AttachNode* g = GrappleNode()) {
        node.AddValue("grappledSameVessel", grappledSameVessel);
        node.AddValue("grapplePos", KSPUtil::WriteVector(g->position));
        node.AddValue("grappleOrt", KSPUtil::WriteVector(g->orientation));
        node.AddValue("grappleOrt2", KSPUtil::WriteVector(g->secondaryAxis));
    }
    if (vesselInfo) vesselInfo->Save(*node.AddNode("DOCKEDVESSEL"));
    if (otherVesselInfo) otherVesselInfo->Save(*node.AddNode("DOCKEDVESSEL_Other"));
}

// ModuleGrappleNode.cs:184
void ModuleGrappleNode::OnStart(StartState st) {
    Events()["Release"]->active = false;
    Events()["ReleaseSameVessel"]->active = false;
    Events()["SetLoose"]->active = false;
    Events()["LockPivot"]->active = false;
    Part* p = PartOf();
    if (p == nullptr) throw Nre();
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
            log::Warning("[Docking Node Module]: WARNING - No control transform found with name " + Get<std::string>("nodeTransformName"));  // sic: the message names nodeTransformName
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
    if (DockingEnvironment::SceneIsFlight() && state.find("Grappled") != std::string::npos) {
        otherPart = p->vessel->byFlightID(dockedPartUId);
        if (!HasGrappleNode()) throw Nre();
        if (otherPart == p->parent) {
            GrappleNode()->attachedPart = otherPart;
            GrappleNode()->owner = p;
            AddGrappleNodeTo(*p);
        } else {
            GrappleNode()->attachedPart = p;
            GrappleNode()->owner = otherPart;
            AddGrappleNodeTo(*otherPart);
        }
        AttachNode* g = GrappleNode();
        g->position = grabPos;
        g->orientation = grabOrt;
        g->secondaryAxis = grabOrt2;
        g->originalPosition = g->position;
        g->originalOrientation = g->orientation;
        g->originalSecondaryAxis = g->secondaryAxis;
        if (GrappleEnvironment::createPivot) {
            pivotJoint = GrappleEnvironment::createPivot(*this, g);
            if (pivotJoint) pivotJoint->SetPivotAngleLimit(Get<float>("pivotRange"));
        }
    }
    if (!(DockingEnvironment::sceneIsMissionBuilder && DockingEnvironment::sceneIsMissionBuilder())) StartCoroutine(1, [this, st] { lateFSMStart(st); });
}

// ModuleGrappleNode.cs:248
void ModuleGrappleNode::lateFSMStart(StartState st) {
    SetupFSM();
    if ((static_cast<int>(st) & static_cast<int>(StartState::Editor)) == 0) fsm->StartFSM(state);
}

// ModuleGrappleNode.cs:258
void ModuleGrappleNode::SetupFSM() {
    using SC = KFSMStateChange;
    using CB = KFSMCallback;
    using CC = KFSMEventCondition;
    fsm = std::make_unique<KerbalFSM>();
    auto toggle = [this](bool active) {
        if (deployAnimator != nullptr) deployAnimator->Events()["Toggle"]->active = active;
    };
    st_ready = std::make_unique<KFSMState>("Ready");
    st_ready->OnEnter = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (deployAnimator) toggle(true);
    });
    st_ready->OnFixedUpdate = CB::Lambda(this, [this] { otherPart = FindContactParts(); });
    st_ready->OnLeave = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (deployAnimator) toggle(false);
    });
    fsm->AddState(st_ready.get());
    st_grappled = std::make_unique<KFSMState>("Grappled");
    st_grappled->OnEnter = SC::Lambda(this, [this](KFSMState*) {
        rGrabPoint = otherPart->partTransform.InverseTransformPoint(NodePose().position);
        Events()["Release"]->active = true;
    });
    st_grappled->OnUpdate = CB::Lambda(this, [this, toggle] {
        const bool flag2 = CountAdjustersBlockingGrappleRelease() > 0;
        Events()["Release"]->active = !flag2;
        if (flag2 && deployAnimator != nullptr) {
            toggle(false);
            deployAnimator->Set<bool>("allowManualControl", false);
        }
    });
    st_grappled->OnLateUpdate = KFSMCallback::Lambda(this, [this] { updateGrappleTransform(); });  // Delegate.Combine onto the empty delegate
    st_grappled->OnLeave = SC::Lambda(this, [this](KFSMState*) {
        Events()["Release"]->active = false;
        if (CountAdjustersBlockingGrappleRelease() > 0 && deployAnimator != nullptr) deployAnimator->Set<bool>("allowManualControl", originalDeployAnimatorAllowManualControl);
    });
    fsm->AddState(st_grappled.get());
    st_grappled_sameVessel = std::make_unique<KFSMState>("Grappled (same vessel)");
    st_grappled_sameVessel->OnEnter = SC::Lambda(this, [this](KFSMState*) {
        Events()["ReleaseSameVessel"]->active = true;
        if (!PartOf()->vessel->packed) GrappleSameVessel(otherPart);
    });
    st_grappled_sameVessel->OnLateUpdate = KFSMCallback::Lambda(this, [this] { updateGrappleTransform(); });
    st_grappled_sameVessel->OnUpdate = CB::Lambda(this, [this, toggle] {
        if (GrappleJoint == nullptr && !PartOf()->vessel->packed) GrappleSameVessel(otherPart);
        const bool flag = CountAdjustersBlockingGrappleRelease() > 0;
        Events()["ReleaseSameVessel"]->active = !flag;
        if (flag && deployAnimator != nullptr) {
            toggle(false);
            deployAnimator->Set<bool>("allowManualControl", false);
        }
    });
    st_grappled_sameVessel->OnLeave = SC::Lambda(this, [this](KFSMState*) {
        Events()["ReleaseSameVessel"]->active = false;
        if (CountAdjustersBlockingGrappleRelease() > 0 && deployAnimator != nullptr) deployAnimator->Set<bool>("allowManualControl", originalDeployAnimatorAllowManualControl);
        DestroySameVesselJoint();
        if (fsm->LastEvent() == on_sameVessel_disconnect.get()) {
            otherPart = nullptr;
            vesselInfo.reset();
            otherVesselInfo.reset();
        }
    });
    fsm->AddState(st_grappled_sameVessel.get());
    st_disengage = std::make_unique<KFSMState>("Disengage");
    st_disengage->OnEnter = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (otherPart != nullptr) rGrabPoint = otherPart->partTransform.InverseTransformPoint(NodePose().position);
        if (deployAnimator) toggle(true);
    });
    st_disengage->OnLeave = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (deployAnimator) toggle(false);
    });
    fsm->AddState(st_disengage.get());
    st_disabled = std::make_unique<KFSMState>("Disabled");
    st_disabled->OnEnter = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (deployAnimator) toggle(true);
    });
    st_disabled->OnLeave = SC::Lambda(this, [this, toggle](KFSMState*) {
        if (deployAnimator) toggle(false);
    });
    fsm->AddState(st_disabled.get());

    on_contact = std::make_unique<KFSMEvent>("Contact");
    on_contact->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    on_contact->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) {
        if (otherPart == nullptr) return false;
        if (IsAdjusterBlockingGrappleGrab()) return false;
        Part* p = PartOf();
        const DockPose pose = NodePose();
        if (otherPart->vessel != p->vessel) {
            if (!CheckGrappleContact(pose, hit, Get<float>("captureRange"), Get<float>("captureMinFwdDot"))) return false;
            const float sqr = (VelocityOf(*p) - VelocityOf(*otherPart)).sqrMagnitude();
            return fx(sqr) < fx(Get<float>("captureMaxRvel")) * fx(Get<float>("captureMaxRvel"));
        }
        return CheckGrappleContact(pose, hit, Get<float>("captureRange"), Get<float>("captureMinFwdDot"));
    });
    on_contact->OnEvent = CB::Lambda(this, [this] {
        Part* p = PartOf();
        dockedPartUId = otherPart->fields.flightID;
        if (otherPart->vessel != p->vessel) {
            on_contact->GoToStateOnEvent = st_grappled.get();
            if (otherPart->vessel->isEVA()) {
                if (GrappleEnvironment::evaOnGrapple) GrappleEnvironment::evaOnGrapple(*otherPart);
                ForceSetActive(p->vessel);
                if (GrappleEnvironment::resumeVesselCtrlState) GrappleEnvironment::resumeVesselCtrlState(p->vessel);
            }
            if (!(Vessel::GetDominantVessel(p->vessel, otherPart->vessel) == p->vessel) && !otherPart->vessel->isEVA()) Grapple(otherPart, p);
            else Grapple(otherPart, otherPart);
        } else {
            on_contact->GoToStateOnEvent = st_grappled_sameVessel.get();
        }
    });
    fsm->AddEvent(on_contact.get(), {st_ready.get()});

    on_nodeLost = std::make_unique<KFSMEvent>("Node Lost");
    on_nodeLost->updateMode = KFSMUpdateMode::LATEUPDATE;
    on_nodeLost->GoToStateOnEvent = st_ready.get();
    on_nodeLost->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) { return OtherPartIsLost() && fsm->FramesInCurrentState() > 2; });
    on_nodeLost->OnEvent = CB::Lambda(this, [this] {
        otherPart = nullptr;
        vesselInfo.reset();
        otherVesselInfo.reset();
    });
    fsm->AddEvent(on_nodeLost.get(), {st_grappled.get()});

    on_nodeDistance = std::make_unique<KFSMEvent>("Node Distanced");
    on_nodeDistance->updateMode = KFSMUpdateMode::LATEUPDATE;
    on_nodeDistance->GoToStateOnEvent = st_ready.get();
    on_nodeDistance->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) { return (OtherPartIsLost() || NodeIsTooFar()) && fsm->FramesInCurrentState() > 2; });
    on_nodeDistance->OnEvent = CB::Lambda(this, [this] {
        otherPart = nullptr;
        vesselInfo.reset();
        otherVesselInfo.reset();
    });
    fsm->AddEvent(on_nodeDistance.get(), {st_disengage.get()});

    on_undock = std::make_unique<KFSMEvent>("Undock");
    on_undock->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_undock->GoToStateOnEvent = st_disengage.get();
    on_undock->OnEvent = CB::Lambda(this, [this] { on_undock->GoToStateOnEvent = otherPart != nullptr ? st_disengage.get() : st_ready.get(); });
    fsm->AddEvent(on_undock.get(), {st_grappled.get(), st_grappled_sameVessel.get()});

    on_sameVessel_disconnect = std::make_unique<KFSMEvent>("Same Vessel Disconnect");
    on_sameVessel_disconnect->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    on_sameVessel_disconnect->GoToStateOnEvent = st_ready.get();
    on_sameVessel_disconnect->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) { return otherPart == nullptr || otherPart->vessel != PartOf()->vessel; });
    on_sameVessel_disconnect->OnEvent = CB::Lambda(this, [] {});
    fsm->AddEvent(on_sameVessel_disconnect.get(), {st_grappled_sameVessel.get()});

    on_disable = std::make_unique<KFSMEvent>("Disable");
    on_disable->updateMode = KFSMUpdateMode::UPDATE;
    on_disable->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) { return deployAnimator != nullptr && deployAnimator->Progress() != 1.0f; });
    on_disable->OnEvent = CB::Lambda(this, [this] {
        AttachNode* g = GrappleNode();
        if (g == nullptr) throw Nre();
        if (g->attachedPart != nullptr && !grappledSameVessel) Release();
    });
    on_disable->GoToStateOnEvent = st_disabled.get();
    fsm->AddEvent(on_disable.get(), {st_ready.get(), st_disengage.get(), st_grappled.get(), st_grappled_sameVessel.get()});

    on_enable = std::make_unique<KFSMEvent>("Enable");
    on_enable->updateMode = KFSMUpdateMode::UPDATE;
    on_enable->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) { return deployAnimator != nullptr && deployAnimator->Progress() == 1.0f; });
    on_enable->GoToStateOnEvent = st_ready.get();
    fsm->AddEvent(on_enable.get(), {st_disabled.get()});
}

// ModuleGrappleNode.cs:483
void ModuleGrappleNode::Update() {
    if (fsm != nullptr && fsm->Started()) {
        fsm->UpdateFSM();
        state = fsm->currentStateName;
    }
}

// ModuleGrappleNode.cs:492
void ModuleGrappleNode::FixedUpdate() {
    if (fsm != nullptr && fsm->Started()) fsm->FixedUpdateFSM();
}

// ModuleGrappleNode.cs:500
void ModuleGrappleNode::LateUpdate() {
    if (fsm != nullptr && fsm->Started()) fsm->LateUpdateFSM();
    // pivotJoint.DrawDebug(): debug drawing
}

// ModuleGrappleNode.cs:512
Part* ModuleGrappleNode::FindContactParts() {
    Part* p = PartOf();
    if (p->fields.packed) return nullptr;
    const DockPose pose = NodePose();
    const float range = Get<float>("captureRange");
    if (GrappleEnvironment::raycast) {
        auto h = GrappleEnvironment::raycast(*this, pose.position, pose.forward * range, range);
        if (h) {
            hit = *h;
            return hit.part;
        }
    }
    return nullptr;
}

// ModuleGrappleNode.cs:527
bool ModuleGrappleNode::CheckGrappleContact(const DockPose& nodeT, const GrappleEnvironment::Hit& h, float minDist, float minFwdDot) {
    if (fx((nodeT.position - h.point).sqrMagnitude()) < fx(minDist) * fx(minDist) && Vector3f::Dot(nodeT.forward, -h.normal) > minFwdDot) return true;
    return false;
}

// ModuleGrappleNode.cs:536
void ModuleGrappleNode::Grapple(Part* other, Part* dockerSide) {
    Part* p = PartOf();
    log::Info("[Grapple Module] Grabbing on to " + Localizer::Format(other->partInfo != nullptr ? other->partInfo->title : std::string()) + " on " +
              Localizer::Format(other->vessel->vesselName));
    vesselInfo = DockedVesselInfo();
    vesselInfo->name = p->vessel->vesselName;
    vesselInfo->vesselType = p->vessel->vesselType;
    vesselInfo->rootPartUId = p->vessel->rootPart->fields.flightID;
    otherVesselInfo = DockedVesselInfo();
    otherVesselInfo->name = other->vessel->vesselName;
    otherVesselInfo->vesselType = other->vessel->vesselType;
    otherVesselInfo->rootPartUId = other->vessel->rootPart->fields.flightID;
    const Vector3f position = other->partTransform.InverseTransformPoint(hit.point);
    const UntrackedObjectClass untrackedObjectSize = other->vessel->GetDiscoveryInfo().objectSize();
    std::unordered_map<Part*, Vector3f> dictionary;
    std::unordered_map<Part*, Quaternionf> dictionary2;
    for (Part* part : p->vessel->parts) {
        if (part->fields.physicalSignificance == 0) {
            dictionary.emplace(part, part->partTransform.position);
            dictionary2.emplace(part, part->partTransform.rotation);
        }
    }
    for (Part* part : otherPart->vessel->parts) {
        if (part->fields.physicalSignificance == 0) {
            if (dictionary.count(part) != 0) throw ArgumentException("An item with the same key has already been added.");  // Dictionary.Add
            dictionary.emplace(part, part->partTransform.position);
            dictionary2.emplace(part, part->partTransform.rotation);
        }
    }
    const DockPose pose = NodePose();
    GE::onActiveJointNeedUpdate.Fire(other->vessel);
    GE::onActiveJointNeedUpdate.Fire(p->vessel);
    other->vessel->SetRotation(other->vessel->vesselTransform.rotation);
    p->vessel->SetRotation(p->vessel->vesselTransform.rotation);
    p->vessel->SetPosition(Vector3d(p->vessel->vesselTransform.position - (pose.position - other->partTransform.TransformPoint(position))), true);
    p->vessel->IgnoreGForces(10);
    AttachNode* g = GrappleNode();
    if (g == nullptr) throw Nre();
    g->position = dockerSide->partTransform.InverseTransformPoint(other->partTransform.TransformPoint(position));
    g->orientation = dockerSide->partTransform.InverseTransformDirection(p->partTransform.TransformDirection(Vector3f(0.0f, 1.0f, 0.0f)));
    g->secondaryAxis = dockerSide->partTransform.InverseTransformDirection(p->partTransform.TransformDirection(Vector3f(0.0f, 0.0f, -1.0f)));
    g->originalPosition = g->position;
    g->originalOrientation = g->orientation;
    g->originalSecondaryAxis = g->secondaryAxis;
    if (DockingEnvironment::vesselTarget) {
        ITargetable* t = DockingEnvironment::vesselTarget();
        if (t != nullptr && t->GetVessel() == other->vessel && DockingEnvironment::setVesselTarget) DockingEnvironment::setVesselTarget(nullptr);
    }
    if (p->vessel->targetObject != nullptr && p->vessel->targetObject->GetVessel() == other->vessel) p->vessel->targetObject = nullptr;
    if (other->vessel->targetObject != nullptr && other->vessel->targetObject->GetVessel() == p->vessel) other->vessel->targetObject = nullptr;
    if (dockerSide == p) {
        g->attachedPart = other;
        g->owner = p;
        AddGrappleNodeTo(*p);
        if (FlightRegistry::ActiveVessel == p->vessel) {
            ForceSetActive(other->vessel);
            SetNeutral();
        }
        p->Couple(other);
    } else {
        g->attachedPart = p;
        g->owner = other;
        AddGrappleNodeTo(*other);
        if (FlightRegistry::ActiveVessel == other->vessel) {
            ForceSetActive(p->vessel);
            SetNeutral();
        } else if (FlightRegistry::ActiveVessel == p->vessel) {
            p->vessel->MakeActive();
            SetNeutral();
        }
        other->Couple(p);
    }
    if (pivotJoint == nullptr || !pivotJoint->IsValid()) {
        if (GrappleEnvironment::createPivot) {
            pivotJoint = GrappleEnvironment::createPivot(*this, GrappleNode());
            if (pivotJoint) pivotJoint->SetPivotAngleLimit(Get<float>("pivotRange"));
        }
    }
    for (Part* part : p->vessel->parts) {
        if (part->fields.physicalSignificance == 0) {
            auto it = dictionary.find(part);
            if (it == dictionary.end()) throw KeyNotFoundException("The given key was not present in the dictionary.");
            part->partTransform.position = it->second;
            part->partTransform.rotation = dictionary2.at(part);
        }
    }
    // ModuleAsteroid / ModuleComet: AnalyticsUtil.LogAsteroidEvent / LogCometEvent are analytics (dropped); the size class is restored
    if (other->FindModuleOfClass("ModuleAsteroid") != nullptr) p->vessel->GetDiscoveryInfo().SetUntrackedObjectSize(untrackedObjectSize);
    if (other->FindModuleOfClass("ModuleComet") != nullptr) p->vessel->GetDiscoveryInfo().SetUntrackedObjectSize(untrackedObjectSize);
    StartCoroutine(1, [this] { WaitAndSwitchFocus(); });
    GE::onVesselWasModified.Fire(p->vessel);
}

// ModuleGrappleNode.cs:654
void ModuleGrappleNode::updateGrappleTransform() {
    if (pivotJoint == nullptr) return;  // (the pivot joint exists whenever the state is entered; the seam may be unset)
    if (pivotJoint->DriveMode() != ActiveJointDriveMode::NoJoint) {
        if (GrappleEnvironment::setNodeRotation) GrappleEnvironment::setNodeRotation(*this, PartOf()->partTransform.rotation * pivotJoint->GetLocalPivotRotation());
    }
}

// ModuleGrappleNode.cs:663
void ModuleGrappleNode::ReleaseAction(KSPActionParam&) {
    if (Events()["Release"]->active) Release();
}

// ModuleGrappleNode.cs:672
void ModuleGrappleNode::Release() {
    Part* p = PartOf();
    Part* parent = p->parent;
    Part* part = otherPart;
    Vessel* vessel = p->vessel;
    const uint32_t referenceTransformId = p->vessel->referenceTransformId;
    const UntrackedObjectClass untrackedObjectSize = p->vessel->GetDiscoveryInfo().objectSize();
    GrappleJoint.reset();
    if (parent != otherPart) {
        otherPart->Undock(otherVesselInfo ? &*otherVesselInfo : nullptr);
        RemoveGrappleNodeFrom(*otherPart);
    } else {
        p->Undock(vesselInfo ? &*vesselInfo : nullptr);
        RemoveGrappleNodeFrom(*p);
    }
    AttachNode* g = GrappleNode();
    if (g != nullptr) {
        g->attachedPart = nullptr;
        g->owner = nullptr;
    }
    const DockPose pose = NodePose();
    const float ejection = Get<float>("undockEjectionForce");
    AddForceOn(*p, pose.forward * f32store((0.0 - fx(ejection)) * 0.5));
    AddForceOn(*part, pose.forward * f32store(fx(ejection) * 0.5));
    if (vessel == FlightRegistry::ActiveVessel && vessel->byFlightID(referenceTransformId) == nullptr) StartCoroutine(1, [this] { WaitAndSwitchFocus(); });
    if (otherPart->FindModuleOfClass("ModuleAsteroid") != nullptr) otherPart->vessel->GetDiscoveryInfo().SetUntrackedObjectSize(untrackedObjectSize);
    if (otherPart->FindModuleOfClass("ModuleComet") != nullptr) otherPart->vessel->GetDiscoveryInfo().SetUntrackedObjectSize(untrackedObjectSize);
    fsm->RunEvent(on_undock.get());
}

// ModuleGrappleNode.cs:713 (after the first yield)
void ModuleGrappleNode::WaitAndSwitchFocus() {
    ForceSetActive(PartOf()->vessel);
    SetNeutral();
    PartOf()->vessel->ResumeStaging();
    if (GrappleEnvironment::minDistanceBackaway) GrappleEnvironment::minDistanceBackaway();
}

// ModuleGrappleNode.cs:722
void ModuleGrappleNode::GrappleSameVessel(Part* other) {
    Part* p = PartOf();
    const Vector3f position = other->partTransform.InverseTransformPoint(hit.point);
    AttachNode* g = GrappleNode();
    if (g == nullptr) throw Nre();
    g->position = p->partTransform.InverseTransformPoint(other->partTransform.TransformPoint(position));
    g->orientation = p->partTransform.InverseTransformDirection(p->partTransform.TransformDirection(Vector3f(0.0f, 1.0f, 0.0f)));
    g->secondaryAxis = p->partTransform.InverseTransformDirection(p->partTransform.TransformDirection(Vector3f(0.0f, 0.0f, -1.0f)));
    g->originalPosition = g->position;
    g->originalOrientation = g->orientation;
    g->originalSecondaryAxis = g->secondaryAxis;
    g->attachedPart = other;
    g->owner = p;
    AddGrappleNodeTo(*p);
    if (GrappleEnvironment::createSameVesselJoint) GrappleJoint = GrappleEnvironment::createSameVesselJoint(*this, *other);
    else GrappleJoint = std::make_shared<int>(0);  // PartJoint.Create: WP-20's
    if (pivotJoint == nullptr || !pivotJoint->IsValid()) {
        if (GrappleEnvironment::createPivot) {
            pivotJoint = GrappleEnvironment::createPivot(*this, GrappleNode());
            if (pivotJoint) pivotJoint->SetPivotAngleLimit(Get<float>("pivotRange"));
        }
    }
    grappledSameVessel = true;
}

// ModuleGrappleNode.cs:743
void ModuleGrappleNode::DestroySameVesselJoint() {
    Part* p = PartOf();
    if (pivotJoint) pivotJoint->Terminate();
    if (GrappleJoint != nullptr) {
        if (GrappleEnvironment::destroySameVesselJoint) GrappleEnvironment::destroySameVesselJoint(*this, GrappleJoint);
        GrappleJoint.reset();
    }
    Events()["SetLoose"]->active = false;
    Events()["LockPivot"]->active = false;
    RemoveGrappleNodeFrom(*p);
    if (AttachNode* g = GrappleNode()) {
        g->owner = nullptr;
        g->attachedPart = nullptr;
    }
    pivotJoint.reset();
    grappledSameVessel = false;
}

// ModuleGrappleNode.cs:757
void ModuleGrappleNode::ReleaseSameVessel() { fsm->RunEvent(on_undock.get()); }

// ModuleGrappleNode.cs:762
void ModuleGrappleNode::OnOtherNodeSameVesselDisconnect() { fsm->RunEvent(on_nodeDistance.get()); }

// ModuleGrappleNode.cs:767
bool ModuleGrappleNode::OtherPartIsLost() {
    if (otherPart == nullptr) {
        log::Warning("[GrappleNode]: other node is null!");
        return true;
    }
    return false;
}

// ModuleGrappleNode.cs:782
bool ModuleGrappleNode::NodeIsTooFar() {
    const float d = Get<float>("minDistanceToReEngage");
    return fx((NodePose().position - otherPart->partTransform.TransformPoint(rGrabPoint)).sqrMagnitude()) > fx(d) * fx(d);
}

// ModuleGrappleNode.cs:788
void ModuleGrappleNode::DecoupleAction(KSPActionParam&) {
    if (Events()["Decouple"]->active) Decouple();
}

// ModuleGrappleNode.cs:789
void ModuleGrappleNode::Decouple() {
    Part* p = PartOf();
    AttachNode* g = GrappleNode();
    if (g == nullptr) throw Nre();
    Part* attachedPart = g->attachedPart;
    const UntrackedObjectClass untrackedObjectSize = p->vessel->GetDiscoveryInfo().objectSize();
    if (g->attachedPart == p->parent) p->decouple();
    else g->attachedPart->decouple();
    if (attachedPart->FindModuleOfClass("ModuleAsteroid") != nullptr) attachedPart->vessel->GetDiscoveryInfo().SetUntrackedObjectSize(untrackedObjectSize);
    if (attachedPart->FindModuleOfClass("ModuleComet") != nullptr) attachedPart->vessel->GetDiscoveryInfo().SetUntrackedObjectSize(untrackedObjectSize);
    const DockPose pose = NodePose();
    const float ejection = Get<float>("undockEjectionForce");
    AddForceOn(*p, pose.forward * f32store((0.0 - fx(ejection)) * 0.5));
    AddForceOn(*attachedPart, pose.forward * f32store(fx(ejection) * 0.5));
    fsm->RunEvent(on_undock.get());
}

// ModuleGrappleNode.cs:826
void ModuleGrappleNode::MakeReferenceToggle(KSPActionParam&) { MakeReferenceTransform(); }

// ModuleGrappleNode.cs:832
void ModuleGrappleNode::MakeReferenceTransform() {
    Part* p = PartOf();
    p->SetReferenceTransformName(controlTransformName_.empty() ? p->gameObjectName : controlTransformName_);  // part.SetReferenceTransform(controlTransform)
    p->vessel->SetReferenceTransform(p);
}

// ModuleGrappleNode.cs:839
void ModuleGrappleNode::SetLoose() {
    if (pivotJoint == nullptr) throw Nre();
    pivotJoint->SetDriveMode(ActiveJointDriveMode::Neutral);
    Events()["SetLoose"]->active = false;
    Events()["LockPivot"]->active = true;
    // vessel.CycleAllAutoStrut(): AutoStrut is the joint package's
}

// ModuleGrappleNode.cs:848
void ModuleGrappleNode::LockPivot() {
    Events()["LockPivot"]->active = false;
    Events()["SetLoose"]->active = true;
    if (pivotJoint == nullptr) throw Nre();
    pivotJoint->SetDriveMode(ActiveJointDriveMode::Park);
}

// ModuleGrappleNode.cs:857
bool ModuleGrappleNode::IsLoose() {
    if (pivotJoint != nullptr) return pivotJoint->DriveMode() == ActiveJointDriveMode::Neutral;
    return true;
}

// ModuleGrappleNode.cs:866
Part* ModuleGrappleNode::GetHostPart() { return PartOf(); }

// ModuleGrappleNode.cs:871
void ModuleGrappleNode::OnJointInit(IGrapplePivot* joint) {
    Events()["SetLoose"]->active = joint != nullptr;
    Events()["LockPivot"]->active = false;
    if (joint != nullptr) log::Info("[Grapple Joint]: joint ready");
    else log::Info("[Grapple Joint]: joint lost");
}

// ModuleGrappleNode.cs:883
void ModuleGrappleNode::OnDriveModeChanged(ActiveJointDriveMode mode) {
    if (mode != ActiveJointDriveMode::NoJoint) {
        Events()["SetLoose"]->active = mode != ActiveJointDriveMode::Neutral;
        Events()["LockPivot"]->active = mode == ActiveJointDriveMode::Neutral;
    } else {
        Events()["SetLoose"]->active = false;
        Events()["LockPivot"]->active = false;
    }
}

// ModuleGrappleNode.cs:911
bool ModuleGrappleNode::IsJointUnlocked() {
    if (pivotJoint != nullptr) return pivotJoint->DriveMode() != ActiveJointDriveMode::Park;
    return false;
}

// ModuleGrappleNode.cs:903
std::string ModuleGrappleNode::GetInfo() { return std::string("") + Localizer::Format("#autoLOC_242893", std::vector<std::string>{FormatRange(Get<float>("captureRange"))}); }

// ModuleGrappleNode.cs:992
std::string ModuleGrappleNode::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003044"); }

namespace detail {
void RegisterGrappleModules() {
    ModuleFactory::Register("ModuleGrappleNode", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleGrappleNode>(c)); });
}
}  // namespace detail

}  // namespace ksp
