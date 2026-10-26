// WP-29b: ModuleWheelBase.cs, ModuleWheels/{WheelSubsystem, WheelSubsystems, WheelUtil}.cs, SuspensionLoadBalancer.cs (see
// ksp/wheels/wheel_modules.hpp for the scope and the seams). Every function cites File.cs:line of decompiled/KSP-Decompiled/Assembly-CSharp
// (tools/wp28/check_cites.py verifies the citations). Float32 follows the Mono rule (ksp/dotnet/mono_float.hpp).
#include "ksp/wheels/wheel_modules.hpp"

#include <algorithm>
#include <cmath>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/wheels/vehicle_physics.hpp"

namespace ksp {

using namespace VehiclePhysics;

void WheelModuleEnvironment::Reset() {
    scheduler = nullptr;
    maximumSurfaceGeeASL = nullptr;
    timeSinceLevelLoad = nullptr;
    trace = nullptr;
    describeCollider = nullptr;
    findModelTransform = nullptr;
    findModelAnimator = nullptr;
    sampleAnimation = nullptr;
    asStatusLight = nullptr;
    findLightModule = nullptr;
    noCrashDamage = false;
    suspensionColliderEnable = nullptr;
    setRigidbodyFrozen = nullptr;
    wheelColliderCreated = nullptr;
    isActiveVessel = nullptr;
    shieldedFromAirstream = nullptr;
}

// ---- WheelSubsystems -----------------------------------------------------------------------------------------------------------------------
namespace ModuleWheels {

// WheelSubsystems.cs:22
void WheelSubsystems::AddSubsystem(WheelSubsystem* d) {
    if (std::find(subs_.begin(), subs_.end(), d) == subs_.end()) subs_.push_back(d);
    types_ |= d->type;
    Fire();
}

// WheelSubsystems.cs:29
void WheelSubsystems::RemoveSubsystem(WheelSubsystem* d) {
    auto it = std::find(subs_.begin(), subs_.end(), d);
    if (it != subs_.end()) subs_.erase(it);
    ResetFlags();
    Fire();
}

// WheelSubsystems.cs:36
void WheelSubsystems::ResetFlags() {
    types_ = WheelSubsystem::None;
    int count = static_cast<int>(subs_.size());
    while (count-- > 0) types_ |= subs_[static_cast<size_t>(count)]->type;
}

// WheelSubsystems.cs:51
bool WheelSubsystems::Contains(const WheelSubsystem* s) const { return std::find(subs_.begin(), subs_.end(), s) != subs_.end(); }

// WheelSubsystems.cs:56
void WheelSubsystems::GetReasons(std::string& o, const std::string& separator) const {
    for (size_t i = 0, count = subs_.size(); i < count; i++) {
        o += subs_[i]->reason;
        if (i > 0) o += separator;
    }
}

// OnModified(this) (the multicast invocation: every entry in order; a listener may add or remove entries while it runs, the C# delegate list is
// immutable, so a snapshot is walked)
void WheelSubsystems::Fire() {
    const auto snapshot = onModified_;
    for (const auto& e : snapshot) e.second(this);
}

void WheelSubsystems::SetOnModified(const void* owner, Callback cb) {
    onModified_.clear();
    onModified_.emplace_back(owner, std::move(cb));
}

void WheelSubsystems::CombineOnModified(const void* owner, Callback cb) { onModified_.emplace_back(owner, std::move(cb)); }

// Delegate.Remove(OnModified, new Callback(OnSubsystemsModified)): removes the last occurrence of the owner's entry
void WheelSubsystems::RemoveOnModified(const void* owner) {
    for (size_t i = onModified_.size(); i-- > 0;) {
        if (onModified_[i].first == owner) {
            onModified_.erase(onModified_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }
}

// ---- WheelUtil.cs --------------------------------------------------------------------------------------------------------------------------
// WheelUtil.cs:7
float WheelUtil::RpmToDegsSec(float rpm) { return f32store(fx(rpm) * 360.0 * 60.0); }
// WheelUtil.cs:12  (float)Math.PI * 2f * wheelRadius * (rpm / 60f)
float WheelUtil::RpmToSpeed(float rpm, float wheelRadius) { return f32store(fx(Mathf::PI) * 2.0 * fx(wheelRadius) * (fx(rpm) / 60.0)); }
// WheelUtil.cs:17  speed * 30f / (wheelRadius * (float)Math.PI)
float WheelUtil::SpeedToRpm(float speed, float wheelRadius) { return f32store(fx(speed) * 30.0 / (fx(wheelRadius) * fx(Mathf::PI))); }

}  // namespace ModuleWheels

// ---- SuspensionLoadBalancer ----------------------------------------------------------------------------------------------------------------

// SuspensionLoadBalancer.cs:24 (OnStart: UpdateSuspensionModules, subscribe onVesselPartCountChanged -- the host calls VesselChanged)
void SuspensionLoadBalancer::OnStart() {
    UpdateSuspensionModules();
    isReady_ = true;
}

// SuspensionLoadBalancer.cs:31
void SuspensionLoadBalancer::FixedUpdate() {
    if (!isReady_ || suspensionModules_.empty()) return;
    double num = static_cast<double>(vessel->GetTotalMass());
    if (vessel->Landed) {
        num = dotnet::Math::Round(num * vessel->gravityMultiplier * vessel->mainBody()->GeeASL * 10.0) / 10.0;
    }
    int num2 = 0;
    for (size_t i = 0; i < suspensionModules_.size(); i++) {
        if (suspensionModules_[i]->wheel != nullptr && suspensionModules_[i]->wheel->IsGrounded()) num2++;
    }
    for (size_t j = 0; j < suspensionModules_.size(); j++) {
        float num3 = 0.0f;
        if (suspensionModules_[j]->wheel != nullptr) {
            num3 = f32store(fx(Mathf::Round(f32store(100.0 / (static_cast<float>(num2) > 0.0f ? fx(static_cast<float>(num2)) : 1.0)))) / 100.0);
            suspensionModules_[j]->sprungMassGravity = Mathf::Max(static_cast<float>(num * fx(num3) * fx(multiplier)), minMass);
        }
    }
}

// SuspensionLoadBalancer.cs:66
void SuspensionLoadBalancer::VesselChanged(Vessel* vsl) {
    if (vsl->persistentId == vessel->persistentId) UpdateSuspensionModules();
}

// SuspensionLoadBalancer.cs:74
void SuspensionLoadBalancer::UpdateSuspensionModules() {
    suspensionModules_.clear();
    for (size_t i = 0; i < vessel->parts.size(); i++) {
        PartModule* m = vessel->parts[i]->FindModuleImplementing([](PartModule& pm) { return dynamic_cast<ModuleWheels::ModuleWheelSuspension*>(&pm) != nullptr; });
        if (auto* s = dynamic_cast<ModuleWheels::ModuleWheelSuspension*>(m)) suspensionModules_.push_back(s);
    }
}

}  // namespace ksp
