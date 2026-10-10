// WP-24: ModuleReactionWheel.cs (see ksp/modules/module_reaction_wheel.hpp for the scope).
#include "ksp/modules/module_reaction_wheel.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/flight_ctrl_state.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_autopilot.hpp"

namespace ksp {

namespace {
const PartResourceDefinition* LookupDefinition(const std::string& name) {
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    return lib ? lib->GetDefinition(std::string_view(name)) : nullptr;
}
// EnumExtensions.displayDescription over the [Description] attributes of WheelState (ModuleReactionWheel.cs:12-17)
const char* WheelStateTag(ModuleReactionWheel::WheelState s) {
    switch (s) {
    case ModuleReactionWheel::WheelState::Active: return "#autoLOC_218513";
    case ModuleReactionWheel::WheelState::Disabled: return "#autoLOC_6003062";
    default: return "#autoLOC_6003063";
    }
}
}  // namespace

Vessel* ModuleReactionWheel::VesselPtr() const {
    Part* p = dynamic_cast<Part*>(part);
    return p ? p->vessel : nullptr;
}

const char* ModuleReactionWheel::WheelStateName(WheelState s) {
    switch (s) {
    case WheelState::Active: return "Active";
    case WheelState::Disabled: return "Disabled";
    default: return "Broken";
    }
}

// the State setter, ModuleReactionWheel.cs:77 (the State setter)
void ModuleReactionWheel::SetState(WheelState value) {
    wheelState = value;
    Set<std::string>("stateString", Localizer::Format(WheelStateTag(wheelState)));
}

// Unity Instantiate: the public non-KSPField members (ModuleReactionWheel.cs:30-45)
void ModuleReactionWheel::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* r = dynamic_cast<const ModuleReactionWheel*>(&from)) {
        wheelState = r->wheelState;
        operational = r->operational;
        inputVector = r->inputVector;
        inputSum = r->inputSum;
    }
}

// ModuleReactionWheel.cs:88
void ModuleReactionWheel::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("OnToggle", BaseEvent::Handler([this] { OnToggle(); }));
    Actions().Bind("CycleAction", [this](KSPActionParam& p) { CycleAction(p); });
    Actions().Bind("Activate", [this](KSPActionParam& p) { Activate(p); });
    Actions().Bind("Deactivate", [this](KSPActionParam& p) { Deactivate(p); });
    Actions().Bind("Toggle", [this](KSPActionParam& p) { Toggle(p); });
    consumedResources.clear();
    for (size_t i = 0; i < resHandler.inputResources.size(); i++) consumedResources.push_back(LookupDefinition(resHandler.inputResources[i]->name));
}

// ModuleReactionWheel.cs:105
void ModuleReactionWheel::OnStart(StartState) {
    Actions()["Toggle"]->SetGuiName(Get<std::string>("actionGUIName"));
    const bool moduleIsEnabled = Get<bool>("moduleIsEnabled");
    for (const char* n : {"actuatorModeCycle", "authorityLimiter", "stateString"}) {
        BaseField* f = Fields()[n];
        f->guiActiveEditor = moduleIsEnabled;
        f->guiActive = moduleIsEnabled;
    }
    Events()["OnToggle"]->guiActiveEditor = moduleIsEnabled;
    Events()["OnToggle"]->guiActive = moduleIsEnabled;
    Actions()["CycleAction"]->active = moduleIsEnabled;
    Actions()["Activate"]->active = moduleIsEnabled;
    Actions()["Deactivate"]->active = moduleIsEnabled;
    Actions()["Toggle"]->active = moduleIsEnabled;
}

// ModuleReactionWheel.cs:124 (Enum.Parse: ArgumentException for an unknown name)
void ModuleReactionWheel::OnLoad(const ConfigNode& node) {
    if (node.HasValue("WheelState")) {
        const std::string v = node.GetValue("WheelState").value_or("");
        if (v == "Active") SetState(WheelState::Active);
        else if (v == "Disabled") SetState(WheelState::Disabled);
        else if (v == "Broken") SetState(WheelState::Broken);
        else throw ArgumentException("Requested value was not found: " + v);
    }
}

// ModuleReactionWheel.cs:132
void ModuleReactionWheel::OnSave(ConfigNode& node) { node.AddValue("WheelState", WheelStateName(State())); }

// ModuleReactionWheel.cs:137
void ModuleReactionWheel::FixedUpdate() {
    Part* p = dynamic_cast<Part*>(part);
    if (ResourceFlowEnvironment::LoadedSceneIsFlight() && p != nullptr && p->fields.started && Get<bool>("moduleIsEnabled") &&
        !IsAdjusterBreakingReactionWheel()) {
        switch (State()) {
        case WheelState::Active: ActiveUpdate(static_cast<VesselActuatorMode>(Get<int>("actuatorModeCycle"))); break;
        case WheelState::Disabled:
        case WheelState::Broken: break;
        }
    }
}

// ModuleReactionWheel.cs:158
void ModuleReactionWheel::ActiveUpdate(VesselActuatorMode mode) {
    Vessel* vessel = VesselPtr();
    if (!(vessel == nullptr) && (vessel->Autopilot().Enabled() || mode != VesselActuatorMode::SAS)) {
        const float PitchTorque = Get<float>("PitchTorque"), YawTorque = Get<float>("YawTorque"), RollTorque = Get<float>("RollTorque");
        const float authorityLimiter = Get<float>("authorityLimiter");
        inputVectorTemp_.x = vessel->ctrlState.pitch;
        inputVectorTemp_.y = vessel->ctrlState.roll;
        inputVectorTemp_.z = vessel->ctrlState.yaw;
        cmdInput_ = (vessel == FlightRegistry::ActiveVessel)
                        ? (ReactionWheelEnvironment::activeVesselPYR ? ReactionWheelEnvironment::activeVesselPYR() : Vector3f::zero())
                        : Vector3f::zero();
        switch (mode) {
        case VesselActuatorMode::Pilot: inputVectorTemp_ = cmdInput_; break;
        case VesselActuatorMode::SAS: inputVectorTemp_ -= cmdInput_; break;
        default: break;
        }
        inputVectorTemp_.x = f32store(fx(inputVectorTemp_.x) * fx(PitchTorque));
        inputVectorTemp_.y = f32store(fx(inputVectorTemp_.y) * fx(RollTorque));
        inputVectorTemp_.z = f32store(fx(inputVectorTemp_.z) * fx(YawTorque));
        inputVectorTemp_ *= f32store(fx(authorityLimiter) * fx(0.01f));
        const Vector3f vector = ApplyTorqueAdjustments(inputVectorTemp_);
        const bool num = vector != inputVectorTemp_;
        inputVectorTemp_ = vector;
        inputVector = Vector3f::Lerp(inputVector, inputVectorTemp_, f32store(fx(Get<float>("torqueResponseSpeed")) * fx(TimeWarp::fixedDeltaTime())));
        inputSum = f32store((fx(Mathf::Abs(vessel->ctrlState.pitch)) + fx(Mathf::Abs(vessel->ctrlState.roll)) + fx(Mathf::Abs(vessel->ctrlState.yaw))) *
                            (fx(authorityLimiter) * fx(0.01f)));
        if (num && inputSum == 0.0f) {
            Vector3f vector2 = inputVectorTemp_;
            vector2.x = f32store(fx(vector2.x) / fx(PitchTorque));
            vector2.y = f32store(fx(vector2.y) / fx(RollTorque));
            vector2.z = f32store(fx(vector2.z) / fx(YawTorque));
            inputSum = f32store(fx(Mathf::Abs(vector2.x)) + fx(Mathf::Abs(vector2.y)) + fx(Mathf::Abs(vector2.z)));
        }
        std::string stateString = Localizer::Format("#autoLOC_236147");
        if (inputSum > 0.0f) {
            operational = resHandler.UpdateModuleResourceInputs(stateString, static_cast<double>(inputSum), 0.9, false);
        } else {
            operational = false;
        }
        torqueApplied = false;
        if (operational) {
            stateString = Localizer::Format("#autoLOC_7001223");
            lastTorque = vessel->ReferenceTransform().rotation * -inputVector;
            torqueApplied = true;
            if (ReactionWheelEnvironment::addTorque) ReactionWheelEnvironment::addTorque(*this, lastTorque);
        }
        Set<std::string>("stateString", stateString);
    }
}

// ModuleReactionWheel.cs:210
void ModuleReactionWheel::CycleAction(KSPActionParam&) {
    int v = Get<int>("actuatorModeCycle") + 1;
    if (v > 2) v = 0;
    Set<int>("actuatorModeCycle", v);
}

// ModuleReactionWheel.cs:220
void ModuleReactionWheel::Activate(KSPActionParam&) {
    switch (State()) {
    case WheelState::Disabled: SetState(WheelState::Active); break;
    case WheelState::Active:
    case WheelState::Broken: break;
    }
}

// ModuleReactionWheel.cs:234
void ModuleReactionWheel::Deactivate(KSPActionParam&) {
    switch (State()) {
    case WheelState::Active: SetState(WheelState::Disabled); break;
    case WheelState::Disabled:
    case WheelState::Broken: break;
    }
}

// ModuleReactionWheel.cs:248
void ModuleReactionWheel::Toggle(KSPActionParam&) { OnToggle(); }

// ModuleReactionWheel.cs:254
void ModuleReactionWheel::OnToggle() {
    switch (State()) {
    case WheelState::Active: SetState(WheelState::Disabled); break;
    case WheelState::Disabled: SetState(WheelState::Active); break;
    case WheelState::Broken: break;
    }
}

// ModuleReactionWheel.cs:287
void ModuleReactionWheel::GetPotentialTorque(Vector3f& pos, Vector3f& neg) {
    pos = neg = Vector3f::zero();
    if (Get<bool>("moduleIsEnabled") && wheelState == WheelState::Active && Get<int>("actuatorModeCycle") != 2) {
        neg.x = pos.x = Get<float>("PitchTorque");
        neg.y = pos.y = Get<float>("RollTorque");
        neg.z = pos.z = Get<float>("YawTorque");
    }
}

// OnModuleAdjusterRemoved, ModuleReactionWheel.cs:329 (OnModuleAdjusterRemoved: List.Remove)
void ModuleReactionWheel::RemoveAdjuster(IReactionWheelAdjuster* a) {
    auto it = std::find(adjusterCache_.begin(), adjusterCache_.end(), a);
    if (it != adjusterCache_.end()) adjusterCache_.erase(it);
}

// ModuleReactionWheel.cs:314
bool ModuleReactionWheel::IsAdjusterBreakingReactionWheel() {
    for (size_t i = 0; i < adjusterCache_.size(); i++)
        if (adjusterCache_[i]->IsReactionWheelBroken()) return true;
    return false;
}

// ModuleReactionWheel.cs:333
Vector3f ModuleReactionWheel::ApplyTorqueAdjustments(Vector3f torque) {
    for (size_t i = 0; i < adjusterCache_.size(); i++) torque = adjusterCache_[i]->ApplyTorqueAdjustment(torque);
    return torque;
}

}  // namespace ksp
