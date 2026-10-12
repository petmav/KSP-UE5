// WP-32: RetractableLadder.cs (see include/ksp/eva/retractable_ladder.hpp). Every function cites RetractableLadder.cs:<line>.
#include "ksp/eva/retractable_ladder.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
}  // namespace

RetractableLadder::RetractableLadder(const ModuleClassInfo& cls) : PartModule(cls) {}
RetractableLadder::~RetractableLadder() = default;

Part* RetractableLadder::PartPtr() const { return dynamic_cast<Part*>(part); }

KFSMState* RetractableLadder::NewState(const std::string& n) {
    states_.push_back(std::make_unique<KFSMState>(n));
    return states_.back().get();
}
KFSMEvent* RetractableLadder::NewEvent(const std::string& n) {
    events_.push_back(std::make_unique<KFSMEvent>(n));
    return events_.back().get();
}
KFSMTimedEvent* RetractableLadder::NewTimed(const std::string& n, double t) {
    auto e = std::make_unique<KFSMTimedEvent>(n, t);
    KFSMTimedEvent* raw = e.get();
    events_.push_back(std::move(e));
    return raw;
}

void RetractableLadder::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("Extend", BaseEvent::Handler([this] { Extend(); }));
    Events().Bind("Retract", BaseEvent::Handler([this] { Retract(); }));
    Actions().Bind("ToggleAction", [this](KSPActionParam& p) { ToggleAction(p); });
    Actions().Bind("ExtendAction", [this](KSPActionParam& p) { ExtendAction(p); });
    Actions().Bind("RetractAction", [this](KSPActionParam& p) { RetractAction(p); });
}

// RetractableLadder.cs:49 `param.type != 0 && (param.type != Toggle || ladderFSM == null || ladderFSM.CurrentState != st_retracted)` -> Retract
void RetractableLadder::ToggleAction(KSPActionParam& param) {
    if (param.type != 0 && (param.type != static_cast<int>(KSPActionType::Toggle) || ladderFSM == nullptr || ladderFSM->CurrentState() != st_retracted))
        Retract();
    else
        Extend();
}

void RetractableLadder::ExtendAction(KSPActionParam&) { Extend(); }     // :62
void RetractableLadder::RetractAction(KSPActionParam&) { Retract(); }   // :68

// RetractableLadder.cs:74
void RetractableLadder::Extend() {
    if (ladderFSM == nullptr || !ladderFSM->Started() || ladderFSM->CurrentState() != st_retracted) return;
    const bool editor = loadedSceneIsEditor ? loadedSceneIsEditor() : ResourceFlowEnvironment::LoadedSceneIsEditor();
    if (editor) {
        if (!rig.isPlaying) {
            rig.isPlaying = true;
            rig.log.push_back("Play");
        }
        ladderFSM->RunEvent(On_extendEditor);
        rig.log.push_back("Sample");
        Events()["Extend"]->active = false;
        Events()["Retract"]->active = true;
    } else {
        ladderFSM->RunEvent(On_extendStart);
    }
}

// RetractableLadder.cs:98
void RetractableLadder::Retract() {
    if (ladderFSM == nullptr || !ladderFSM->Started() || ladderFSM->CurrentState() != st_extended) return;
    const bool editor = loadedSceneIsEditor ? loadedSceneIsEditor() : ResourceFlowEnvironment::LoadedSceneIsEditor();
    if (editor) {
        if (!rig.isPlaying) {
            rig.isPlaying = true;
            rig.log.push_back("Play");
        }
        ladderFSM->RunEvent(On_retractEditor);
        rig.log.push_back("Sample");
        Events()["Extend"]->active = true;
        Events()["Retract"]->active = false;
    } else {
        ladderFSM->RunEvent(On_retractStart);
    }
}

// RetractableLadder.cs:121
void RetractableLadder::OnStart(StartState state) {
    const bool missionBuilder = loadedSceneIsMissionBuilder ? loadedSceneIsMissionBuilder() : false;
    if (missionBuilder) {
        if (rig.rootTransformFound && rig.animationFound && rig.clipFound) {
            rig.clip.enabled = true;
            rig.clip.speed = 3.4028235e38f;  // float.MaxValue
            rig.log.push_back("Play:" + ladderRetractAnimationName());
        }
        return;
    }
    if (state == StartState::None) return;
    Events()["Extend"]->active = false;
    Events()["Retract"]->active = false;
    if (!rig.rootTransformFound) {
        errors.push_back("RetractableLadder: Cannot find ladder animation root '" + ladderAnimationRootName() + "'");
        return;
    }
    if (!rig.animationFound) {
        errors.push_back("RetractableLadder: No animation component found for ladder module.");
        return;
    }
    if (!rig.clipFound) {
        errors.push_back("RetractableLadder: Retract animation clip name not found on ladder module animation.");
        return;
    }
    if (!rig.colliderTransformFound) {
        // `if (transform == null)` tests the animation root again (sic), so a missing collider transform falls through to GetComponent and throws
        throw Nre();
    }
    if (!rig.colliderFound) {
        errors.push_back("RetractableLadder: No ladder collider found for ladder module.");
        return;
    }
    rig.wrapMode = 8;  // WrapMode.ClampForever
    rig.clip.enabled = true;
    rig.clip.weight = 1.0f;
    ladderFSM = std::make_unique<KerbalFSM>();
    states_.clear();
    events_.clear();
    st_retracted = NewState("Retracted");
    st_retracted->OnEnter = KFSMStateChange::Lambda(this, [this](KFSMState*) {
        rig.clip.speed = 0.0f;
        rig.clip.normalizedTime = 1.0f;
        SetDragState(1.0f);
        rig.colliderEnabled = false;
        Events()["Extend"]->active = true;
        Set<std::string>("StateName", "Retracted");
    });
    ladderFSM->AddState(st_retracted);
    st_extended = NewState("Extended");
    st_extended->OnEnter = KFSMStateChange::Lambda(this, [this](KFSMState*) {
        rig.clip.speed = 0.0f;
        rig.clip.normalizedTime = 0.0f;
        rig.colliderEnabled = true;
        SetDragState(0.0f);
        Events()["Retract"]->active = true;
        Set<std::string>("StateName", "Extended");
    });
    ladderFSM->AddState(st_extended);
    st_extending = NewState("Extending");
    st_extending->OnEnter = KFSMStateChange::Lambda(this, [this](KFSMState*) {
        rig.clip.speed = -1.0f;
        rig.isPlaying = true;
        rig.log.push_back("Play");
        rig.colliderEnabled = false;
        Events()["Extend"]->active = false;
    });
    ladderFSM->AddState(st_extending);
    st_retracting = NewState("Retracting");
    st_retracting->OnEnter = KFSMStateChange::Lambda(this, [this](KFSMState*) {
        rig.clip.speed = 1.0f;
        rig.isPlaying = true;
        rig.log.push_back("Play");
        rig.colliderEnabled = false;
        Events()["Retract"]->active = false;
    });
    ladderFSM->AddState(st_retracting);
    On_extendEditor = NewEvent("Extend Editor");
    On_extendEditor->GoToStateOnEvent = st_extended;
    ladderFSM->AddEvent(On_extendEditor, {st_retracted});
    On_retractEditor = NewEvent("Retract Editor");
    On_retractEditor->GoToStateOnEvent = st_retracted;
    ladderFSM->AddEvent(On_retractEditor, {st_extended});
    On_extendStart = NewEvent("Extend Start");
    On_extendStart->GoToStateOnEvent = st_extending;
    On_extendStart->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    ladderFSM->AddEvent(On_extendStart, {st_retracted});
    On_extendEnd = NewTimed("Extend End", rig.clip.length);
    On_extendEnd->GoToStateOnEvent = st_extended;
    ladderFSM->AddEvent(On_extendEnd, {st_extending});
    On_retractStart = NewEvent("Retract Start");
    On_retractStart->GoToStateOnEvent = st_retracting;
    On_retractStart->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    ladderFSM->AddEvent(On_retractStart, {st_extended});
    On_retractEnd = NewTimed("Retract End", rig.clip.length);
    On_retractEnd->GoToStateOnEvent = st_retracted;
    ladderFSM->AddEvent(On_retractEnd, {st_retracting});
    ladderFSM->StartFSM(StateName());
}

// RetractableLadder.cs:242
void RetractableLadder::OnInventoryModeDisable() {
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        Part* p = PartPtr();
        if (p == nullptr || p->protoPartSnapshot == nullptr) throw Nre();
        ProtoPartModuleSnapshot* snap = p->protoPartSnapshot->FindModule("RetractableLadder");
        if (snap != nullptr && snap->moduleValues != nullptr) snap->moduleValues->SetValue("StateName", StateName());
        if (StateName() == "Retracted") SetAnimationToRetracted();
    }
}

// RetractableLadder.cs:258
void RetractableLadder::OnStoredInInventory(ModuleInventoryPart& moduleInventoryPart) {
    PartModule::OnStoredInInventory(moduleInventoryPart);
    Part* p = PartPtr();
    if (p == nullptr || p->protoPartSnapshot == nullptr) return;
    ProtoPartModuleSnapshot* snap = p->protoPartSnapshot->FindModule("RetractableLadder");
    Set<std::string>("StateName", "Retracted");
    if (snap != nullptr && snap->moduleValues != nullptr) {
        // `if (TryGetValue("StateName", ref value)) { value = "Retracted"; SetValue("StateName", value); }`
        if (snap->moduleValues->HasValue("StateName")) snap->moduleValues->SetValue("StateName", "Retracted");
    }
}

// RetractableLadder.cs:278
void RetractableLadder::OnPartCreatedFomInventory(ModuleInventoryPart&) { SetAnimationToRetracted(); }

// RetractableLadder.cs:283
void RetractableLadder::DemoteToPhysicslessPart() { PartModule::DemoteToPhysicslessPart(); }

// RetractableLadder.cs:288
void RetractableLadder::Update() {
    if (ladderFSM != nullptr) ladderFSM->UpdateFSM();
}

// RetractableLadder.cs:296
void RetractableLadder::FixedUpdate() {
    if (ladderFSM != nullptr) {
        ladderFSM->FixedUpdateFSM();
        SetDragState(rig.clip.normalizedTime);
    }
}

// RetractableLadder.cs:305
void RetractableLadder::SetDragState(float b) {
    Part* p = PartPtr();
    if (p == nullptr) throw Nre();
    p->DragCubes.SetCubeWeight("A", b);
    p->DragCubes.SetCubeWeight("B", f32store(1.0 - fx(b)));
}

// RetractableLadder.cs:316
void RetractableLadder::AssumeDragCubePosition(const std::string& name) {
    if (!rig.animationFound) {
        errors.push_back("RetractableLadder: No animation component found for ladder module.");
        return;
    }
    if (!rig.clipFound) {
        errors.push_back("RetractableLadder: Retract animation clip name not found on ladder module animation.");
        return;
    }
    rig.clip.speed = 0.0f;
    rig.clip.enabled = true;
    rig.clip.weight = 1.0f;
    if (!(name == "A")) {
        if (name == "B") rig.clip.normalizedTime = 0.0f;
    } else {
        rig.clip.normalizedTime = 1.0f;
    }
}

// RetractableLadder.cs:350
void RetractableLadder::SetAnimationToRetracted() {
    if (rig.rootTransformFound && rig.animationFound && rig.clipFound) {
        rig.clip.enabled = true;
        rig.clip.weight = 1.0f;
        rig.clip.speed = 0.0f;
        rig.clip.normalizedTime = 1.0f;
    }
}

namespace detail {
void RegisterRetractableLadder() {
    ModuleFactory::Register("RetractableLadder",
                            [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<RetractableLadder>(c)); });
}
}  // namespace detail

}  // namespace ksp
