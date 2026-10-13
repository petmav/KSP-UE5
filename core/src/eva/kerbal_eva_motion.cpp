// WP-32: KerbalEVA.cs part 3: input, movement, heading, orientation PID, jetpack, collisions, ragdoll, situation / frame events, ladders, clamber,
// control panels, triggers (KerbalEVA.cs:4502-4660, 4776-5100, 5148-6125, 6860-6905). Every function cites its C# line.
// Float arithmetic follows the Mono rule (ksp/dotnet/mono_float.hpp): fx() widens, f32store() rounds where the C# stores / passes / returns a float.
#include <algorithm>
#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/eva/kerbal_eva.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/flight_input_handler.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/inventory/construction.hpp"
#include "ksp/inventory/inventory.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/science/science.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/unity/random.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }

// Quaternion.Lerp (native, UnityPlayer.dll): t clamped by the managed wrapper, then the shortest-arc normalised lerp, one float32 rounding per
// operation. UNVERIFIED: written from Unity's documented formulation (no in-game capture of Quaternion.Lerp yet).
Quaternionf QuatLerp(const Quaternionf& q1, const Quaternionf& q2, float t) {
    t = Mathf::Clamp01(t);
    Quaternionf r;
    if (Quaternionf::Dot(q1, q2) < 0.0f) {
        r = {q1.x + t * (-q2.x - q1.x), q1.y + t * (-q2.y - q1.y), q1.z + t * (-q2.z - q1.z), q1.w + t * (-q2.w - q1.w)};
    } else {
        r = {q1.x + t * (q2.x - q1.x), q1.y + t * (q2.y - q1.y), q1.z + t * (q2.z - q1.z), q1.w + t * (q2.w - q1.w)};
    }
    return UnityNative::Normalize(r);
}

inline bool NotZero(const Vector3f& v) { return v != Vector3f::zero(); }
inline Vector3d W(const Vector3f& v) { return Vector3d(v.x, v.y, v.z); }
inline float ClampMag(float v) { return v; }
}  // namespace

// ============================================================================================ input

// KerbalEVA.cs:4502
void KerbalEVA::HandleMovementInput() {
    if (!VesselUnderControl() || (!SurfaceOrSplashed() && JetpackDeployed() && EVAConstructionModeController::MovementRestricted())) return;
    if (Key(EvaKey::ToggleMovementMode, KeyQuery::Down) && !EVAConstructionModeController::MovementRestricted()) CharacterFrameModeToggle = !CharacterFrameModeToggle;
    CharacterFrameMode = JetpackDeployed() || CharacterFrameModeToggle;
    parachuteInput = Vector3f::zero();
    if (Key(EvaKey::Forward, KeyQuery::Held)) {
        tgtRpos += (CharacterFrameMode ? transform().forward() : fFwd);
        ladderTgtRPos += transform().up();
    }
    if (Key(EvaKey::Back, KeyQuery::Held)) {
        tgtRpos -= (CharacterFrameMode ? transform().forward() : fFwd);
        ladderTgtRPos -= transform().up();
    }
    if (Key(EvaKey::Left, KeyQuery::Held)) {
        tgtRpos -= (CharacterFrameMode ? transform().right() : fRgt);
        ladderTgtRPos -= transform().right();
    }
    if (Key(EvaKey::Right, KeyQuery::Held)) {
        tgtRpos += (CharacterFrameMode ? transform().right() : fRgt);
        ladderTgtRPos += transform().right();
    }
    tgtRpos = Vector3f::Normalize(tgtRpos);  // Vector3.Normalize(): in place
    packTgtRPos += transform().forward() * Axis(EvaAxis::TranslateZ);
    packTgtRPos += transform().right() * Axis(EvaAxis::TranslateX);
    packTgtRPos += transform().up() * Axis(EvaAxis::TranslateY);
    packRRot += transform().right() * Axis(EvaAxis::Pitch);
    packRRot += transform().up() * Axis(EvaAxis::Yaw);
    packRRot += transform().forward() * Axis(EvaAxis::Roll);
    if (FlightInputHandler::SPACENAV_USE_AS_FLIGHT_CONTROL) {
        const Vector3f trans = EvaEnvironment::spaceNavTranslation ? EvaEnvironment::spaceNavTranslation() : Vector3f::zero();
        if (CharacterFrameMode) {
            tgtRpos += (transform().rotation * trans).normalized();
        } else {
            tgtRpos += UnityNative::LookRotation(fFwd, fUp) * trans;
        }
        if (SurfaceOrSplashed()) {
            if (CharacterFrameMode) {
                tgtFwd = fFwd;
                tgtUp = fUp;
            } else {
                tgtFwd = tgtRpos;
                tgtUp = fUp;
            }
        }
        ladderTgtRPos += UnityNative::LookRotation(fFwd, fUp) * trans;
        packTgtRPos += (UnityNative::LookRotation(fFwd, fUp) * trans) * GameSettings::SPACENAV_FLIGHT_SENS_LIN;
    }
    if (packRRot != Vector3f::zero()) {
        manualAxisControl = true;
        cmdRot = packRRot;
    }
    if (Key(EvaKey::PackForward, KeyQuery::Held)) {
        packTgtRPos += transform().forward();
        parachuteInput.x = f32store(fx(parachuteInput.x) + 1.0);
    }
    if (Key(EvaKey::PackBack, KeyQuery::Held)) {
        packTgtRPos -= transform().forward();
        parachuteInput.x = f32store(fx(parachuteInput.x) - 1.0);
    }
    if (Key(EvaKey::PackLeft, KeyQuery::Held)) {
        packTgtRPos -= transform().right();
        parachuteInput.y = f32store(fx(parachuteInput.y) - 1.0);
    }
    if (Key(EvaKey::PackRight, KeyQuery::Held)) {
        packTgtRPos += transform().right();
        parachuteInput.y = f32store(fx(parachuteInput.y) + 1.0);
    }
    if (Key(EvaKey::PackUp, KeyQuery::Held)) packTgtRPos += transform().up();
    if (Key(EvaKey::PackDown, KeyQuery::Held)) packTgtRPos -= transform().up();
    manualRotation = Quaternionf::identity();
    parachuteInput.x = Mathf::Max(Mathf::Min(f32store(fx(parachuteInput.x) - fx(Axis(EvaAxis::Pitch))), 1.0f), -1.0f);
    parachuteInput.y = Mathf::Max(Mathf::Min(f32store(fx(Axis(EvaAxis::Yaw)) + fx(parachuteInput.y)), 1.0f), -1.0f);
    if (EvaEnvironment::mouseButtonDown && EvaEnvironment::mouseButtonDown()) {
        const bool dragging = EvaEnvironment::partActionWindowDragging && EvaEnvironment::partActionWindowDragging(*PartPtr());
        if (!dragging) {
            const float mx = EvaEnvironment::mouseAxis ? EvaEnvironment::mouseAxis(true) : 0.0f;
            const float my = EvaEnvironment::mouseAxis ? EvaEnvironment::mouseAxis(false) : 0.0f;
            manualRotation = UnityNative::AngleAxis(f32store(fx(mx) * fx(57.29578f) * fx(Time::deltaTime)), -transform().forward()) *
                             UnityNative::AngleAxis(f32store(fx(my) * fx(57.29578f) * fx(Time::deltaTime)), transform().right());
        }
    }
    if (!SurfaceOrSplashed() && JetpackDeployed()) {
        if (Key(EvaKey::YawLeft, KeyQuery::Held))
            manualRotation = UnityNative::AngleAxis(f32store(fx(f32store(0.0 - fx(turnRate()))) * fx(57.29578f) * fx(Time::deltaTime)), transform().up());
        if (Key(EvaKey::YawRight, KeyQuery::Held))
            manualRotation = UnityNative::AngleAxis(f32store(fx(turnRate()) * fx(57.29578f) * fx(Time::deltaTime)), transform().up());
        if (EvaEnvironment::spaceNavActive && EvaEnvironment::spaceNavActive() && FlightInputHandler::SPACENAV_USE_AS_FLIGHT_CONTROL) {
            const Vector3f pyr = EvaEnvironment::spaceNavRotationPitchYawRoll ? EvaEnvironment::spaceNavRotationPitchYawRoll() : Vector3f::zero();
            const float sens = GameSettings::SPACENAV_FLIGHT_SENS_ROT;
            manualRotation = UnityNative::AngleAxis(f32store(fx(pyr.x) * fx(sens)), fRgt) * UnityNative::AngleAxis(f32store(fx(pyr.y) * fx(sens)), fUp) *
                             UnityNative::AngleAxis(f32store(fx(pyr.z) * fx(sens)), fFwd);
        }
    }
    if (manualRotation != Quaternionf::identity()) {
        if (fx(Mathf::Acos(Vector3f::Dot(tgtFwd, transform().forward()))) < fx(Mathf::PI) / 2.0 &&
            fx(Mathf::Acos(Vector3f::Dot(tgtUp, transform().up()))) < fx(Mathf::PI) / 2.0) {
            tgtUp = manualRotation * tgtUp;
            tgtFwd = manualRotation * tgtFwd;
        } else {
            tgtUp = transform().up();
            tgtFwd = transform().forward();
        }
    }
    if ((!manualAxisControl && (GameSettings::EVA_ROTATE_ON_MOVE || SurfaceOrSplashed()) &&
         (Key(EvaKey::Forward, KeyQuery::Held) || Key(EvaKey::Back, KeyQuery::Held) || Key(EvaKey::Left, KeyQuery::Held) || Key(EvaKey::Right, KeyQuery::Held))) ||
        Key(EvaKey::Orient, KeyQuery::Held)) {
        manualAxisControl = false;
        if (CharacterFrameMode) {
            tgtFwd = fFwd;
            tgtUp = fUp;
        } else {
            tgtFwd = tgtRpos;
            tgtUp = fUp;
        }
    }
}

// KerbalEVA.cs:4655
void KerbalEVA::SetWaypoint(const Vector3f& tgtPos) {
    tgtRpos = (tgtPos - transform().position).normalized();
    tgtFwd = tgtRpos;
    tgtUp = fUp;
}

// ============================================================================================ movement

// KerbalEVA.cs:4776
void KerbalEVA::UpdateMovement() {
    Vessel* vessel = VesselPtr();
    if (SurfaceOrSplashed() && !vessel->packed) {
        float num = static_cast<float>(fsm->TimeAtCurrentState());
        num = (num >= 0.3f) ? 1.0f : ((!(num > 0.0f)) ? 0.0f : f32store(fx(num) * fx(3.3333333f)));
        currentSpd = Mathf::Lerp(lastTgtSpeed, tgtSpeed, num);
        if (NotZero(tgtRpos)) cmdDir = Vector3f::Lerp(cmdDir, CharacterFrameMode ? tgtRpos : transform().forward(), num);
        const Vector3f up = EvaEnvironment::upAxis ? EvaEnvironment::upAxis() : FlightGlobals::getUpAxis().ToVector3f();
        slopeRotation = UnityNative::FromToRotation(up, heightFromSurfaceHitNormal);
        Engine().SetRbVelocity(cmdDir * currentSpd);
        const Vector3f v = Engine().RbVelocity();
        Engine().SetRbVelocity(slopeRotation * v + fUp * Vector3f::Dot(v, fUp));
    }
}

// KerbalEVA.cs:4793
void KerbalEVA::UpdateHeading() {
    if (VesselPtr()->packed) return;
    if (NotZero(tgtRpos)) {
        float num = Vector3f::Dot(transform().forward(), tgtFwd);
        if (num <= -1.0f || !(num < 1.0f)) return;
        deltaHdg = f32store(fx(Mathf::Acos(num)) * fx(57.29578f));
    }
    float num2 = Mathf::Sign((UnityNative::Inverse(transform().rotation) * tgtFwd).x);
    deltaHdg = f32store(fx(deltaHdg) * fx(num2));
    if (fx(Mathf::Abs(deltaHdg)) < fx(turnRate()) * 2.0) {
        Engine().SetRbAngularVelocity(f32store(fx(deltaHdg) * fx(0.5f)) * fUp);
    } else {
        Engine().SetRbAngularVelocity(f32store(fx(turnRate()) * fx(num2)) * fUp);
    }
}

// KerbalEVA.cs:4820
void KerbalEVA::UpdateLowGBodyColliders() {
    if (!(boundColliderModifierCounter < boundColliderModifierThresholdTime) && tmpBoundState != nullptr) {
        // normalizedBoundTime = tmpBoundState.normalizedTime % 1f
        normalizedBoundTime = static_cast<float>(std::fmod(fx(tmpBoundState->normalizedTime), 1.0));
        const double range = fx(boundColliderYOffsetY) - fx(boundColliderYOffsetX);
        if (normalizedBoundTime >= 0.0f && fx(normalizedBoundTime) < fx(0.25f)) {
            colliderHeight = f32store(range * (-4.0 * fx(normalizedBoundTime) + 1.0) + fx(boundColliderYOffsetX));
        } else if (fx(normalizedBoundTime) >= fx(0.25f) && fx(normalizedBoundTime) < fx(0.5f)) {
            colliderHeight = f32store(range * (4.0 * fx(normalizedBoundTime) - 1.0) + fx(boundColliderYOffsetX));
        } else if (fx(normalizedBoundTime) >= fx(0.5f) && fx(normalizedBoundTime) < fx(0.75f)) {
            colliderHeight = f32store(range * (-4.0 * fx(normalizedBoundTime) + 3.0) + fx(boundColliderYOffsetX));
        } else {
            colliderHeight = f32store(range * (4.0 * fx(normalizedBoundTime) - 3.0) + fx(boundColliderYOffsetX));
        }
        ModifyBodyColliderHeight(colliderHeight);
    }
}

// KerbalEVA.cs:4845
void KerbalEVA::ResetOrientationPID() {
    tgtFwd = transform().forward();
    tgtUp = fUp;
    integral = Vector3f::zero();
    prev_error = error;
}

// KerbalEVA.cs:4853
void KerbalEVA::UpdateOrientationPID() {
    if (!manualAxisControl) {
        if (thrustPercentagePIDBoost && thrustPercentage() > pidBoostThreshold) {
            PIDBoost = 1.0f;
        } else {
            const float p = Mathf::Pow(f32store(fx(pidBoostThreshold) - fx(thrustPercentage())), pidBoostExponent);
            PIDBoost = f32store(1.0 + fx(p) * fx(pidBoostMultiplier));
        }
        error = Vector3f::ClampMagnitude(Vector3f::Cross(transform().forward(), tgtFwd), 0.5f) + Vector3f::Cross(transform().up(), tgtUp);
        integral = Vector3f::ClampMagnitude(integral + error * Time::fixedDeltaTime, 1.0f);
        derivative = (error - prev_error) / Time::fixedDeltaTime;
        cmdRot = (Kp() * error + Ki() * integral + Kd() * derivative) * PIDBoost;
        prev_error = error;
    }
}

// KerbalEVA.cs:4873
void KerbalEVA::UpdatePackAngular() {
    Vessel* vessel = VesselPtr();
    if (JetpackDeployed() && !(vessel == nullptr) && !vessel->packed && NotZero(cmdRot) && Fuel() > 0.0) {
        Engine().AddTorque(cmdRot * massMultiplier() * rotPower() * f32store(fx(thrustPercentage()) * fx(0.01f)));
        fuelFlowRate = f32store(fx(fuelFlowRate) + fx(cmdRot.magnitude()) * fx(Time::fixedDeltaTime));
    }
}

// KerbalEVA.cs:4882
void KerbalEVA::correctGroundedRotation() {
    if (!VesselPtr()->packed) {
        Quaternionf quaternion = UnityNative::FromToRotation(transform().up(), fUp);
        if (tgtSpeed != 0.0f) {
            const Vector3f foot = Engine().FootPivotPosition();
            transform().position += transform().position - foot - quaternion * (transform().position - foot);
        }
        transform().rotation = quaternion * transform().rotation;
    }
}

// KerbalEVA.cs:4895
void KerbalEVA::StartGroundedRotationRecover() {
    rd_rot = transform().rotation;
    rd_tgtRot = UnityNative::FromToRotation(transform().up(), fUp) * transform().rotation;
}

// KerbalEVA.cs:4901
void KerbalEVA::RecoverGroundedRotation(float duration) {
    if (!VesselPtr()->packed) {
        if ((rd_tgtRot.w == 0.0f && rd_tgtRot.x == 0.0f && rd_tgtRot.y == 0.0f && rd_tgtRot.z == 0.0f) ||
            std::isnan(f32store(fx(rd_tgtRot.w) + fx(rd_tgtRot.x) + fx(rd_tgtRot.y) + fx(rd_tgtRot.z)))) {
            Quaternionf quaternion = rd_rot;
            StartGroundedRotationRecover();
            rd_rot = quaternion;
        }
        transform().rotation = QuatLerp(rd_rot, rd_tgtRot, Mathf::InverseLerp(0.0f, duration, static_cast<float>(std::max(0.001, fsm->TimeAtCurrentState()))));
    }
}

// ============================================================================================ jetpack

// KerbalEVA.cs:4915
void KerbalEVA::SetupJetpackEffects() {
    // PitchPos = base.part.findFxGroup("Pitch+") ... (the FXGroup objects are the shell's; the latch calls are reported per group name)
    SetFXGroupMinMaxPower();
}

// KerbalEVA.cs:4932
void KerbalEVA::SetFXGroupMinMaxPower() {
    // SetVisualMinMax(rotFXMinPower, rotFXMaxPower) on six rotational and (lin...) on six linear groups: pure FX parameters
}

namespace {
const char* kFxGroups[12] = {"Pitch+", "Pitch-", "Yaw+", "Yaw-", "Roll+", "Roll-", "X+", "X-", "Y+", "Y-", "Z+", "Z-"};
}

// KerbalEVA.cs:4948
void KerbalEVA::updateJetpackEffects() {
    Part* p = PartPtr();
    if (p == nullptr) return;
    SetFXGroupMinMaxPower();
    // base.part.fxGroups[i].Unlatch() for every group
    for (const char* g : kFxGroups) Engine().FxGroupSet(g, false, 0.0f);
    if (JetpackDeployed() && !VesselPtr()->packed && !isRagdoll && Fuel() > 0.0) {
        const Quaternionf inv = UnityNative::Inverse(transform().rotation);
        Vector3f vector = inv * packLinear;
        auto set = [&](const char* g, bool latch, float power) { Engine().FxGroupSet(g, latch, power); };
        set("X+", fx(vector.x) > fx(linFXLatch), vector.x);
        set("X-", fx(vector.x) < fx(f32store(0.0 - fx(linFXLatch))), f32store(0.0 - fx(vector.x)));
        set("Y+", fx(vector.y) > fx(linFXLatch), vector.y);
        set("Y-", fx(vector.y) < fx(f32store(0.0 - fx(linFXLatch))), f32store(0.0 - fx(vector.y)));
        set("Z+", fx(vector.z) > fx(linFXLatch), vector.z);
        set("Z-", fx(vector.z) < fx(f32store(0.0 - fx(linFXLatch))), f32store(0.0 - fx(vector.z)));
        Vector3f vector2 = inv * Vector3f::ClampMagnitude(cmdRot * f32store(fx(thrustPercentage()) * fx(0.01f)), 1.0f);
        set("Pitch+", fx(vector2.x) > fx(rotFXLatch), vector2.x);
        set("Pitch-", fx(vector2.x) < fx(f32store(0.0 - fx(rotFXLatch))), f32store(0.0 - fx(vector2.x)));
        set("Yaw+", fx(vector2.y) > fx(rotFXLatch), vector2.y);
        set("Yaw-", fx(vector2.y) < fx(f32store(0.0 - fx(rotFXLatch))), f32store(0.0 - fx(vector2.y)));
        set("Roll+", fx(vector2.z) > fx(rotFXLatch), vector2.z);
        set("Roll-", fx(vector2.z) < fx(f32store(0.0 - fx(rotFXLatch))), f32store(0.0 - fx(vector2.z)));
    }
}

// KerbalEVA.cs:4990
void KerbalEVA::ToggleLamp() {
    if (FlightRegistry::ActiveVessel == VesselPtr()) {
        if (lampOn()) {
            set_lampOn(false);
            EvaEnvironment::Effect("headLamp:off");
        } else {
            set_lampOn(true);
            EvaEnvironment::Effect("headLamp:on");
        }
        EvaEnvironment::Effect(std::string("suitColorChanger:ToggleAction light ") + (lampOn() ? "Activate" : "Deactivate"));
    }
}

// KerbalEVA.cs:5012
void KerbalEVA::ToggleJetpack() {
    if (HasJetpack && FlightRegistry::ActiveVessel == VesselPtr() && VesselUnderControl()) {
        On_packToggle->GoToStateOnEvent = fsm->CurrentState();
        fsm->RunEvent(On_packToggle);
    }
}

// KerbalEVA.cs:5021
void KerbalEVA::ToggleJetpack(bool packState) {
    if (!HasJetpack && packState) {
        set_JetpackDeployed(false);
        return;
    }
    if (packState && !JetpackDeployed()) {
        Animations().packExtend.State()->time = 0.1f;
        Engine().AnimPlay(Animations().packExtend.GetAnimationString(), PlayMode::StopAll);
        Engine().AnimBlend(Animations().packExtend.GetAnimationString(), 1.0f, 0.5f);
    }
    if (!packState && JetpackDeployed()) {
        Animations().packStow.State()->time = 0.1f;
        Engine().AnimPlay(Animations().packStow.GetAnimationString(), PlayMode::StopAll);
        Engine().AnimBlend(Animations().packStow.GetAnimationString(), 1.0f, 0.5f);
    }
    set_JetpackDeployed(packState);
    if (InConstructionMode && !JetpackDeployed() && SurfaceContact())
        Engine().AnimBlend(Animations().weldGunLift.GetAnimationString(), 1.0f, 0.1f);
}

// KerbalEVA.cs:5047
void KerbalEVA::UpdatePackLinear() {
    if (JetpackDeployed() && !VesselPtr()->packed && !isRagdoll && !EVAConstructionModeController::MovementRestricted()) {
        packLinear = packTgtRPos * f32store(fx(thrustPercentage()) * fx(0.01f));
        if (NotZero(packLinear) && Fuel() > 0.0) {
            Engine().AddForce(packLinear * linPower());
            fuelFlowRate = f32store(fx(fuelFlowRate) + fx(packLinear.magnitude()) * fx(Time::fixedDeltaTime));
        }
    }
}

// KerbalEVA.cs:5060
void KerbalEVA::UpdatePackFuel() {
    if (ResourceFlowEnvironment::InfinitePropellant) {
        fuelFlowRate = 0.0f;
        return;
    }
    if (propellantResource == nullptr) return;
    double num = fx(PropellantConsumption()) * fx(fuelFlowRate);  // `double num = float * float` is the exact product
    Part* p = PartPtr();
    p->TransferResource(*propellantResource, 0.0 - num, p);
    for (size_t i = 0; i < inventoryPropellantResources.size(); i++) {
        ProtoPartResourceSnapshot* snap = inventoryPropellantResources[i].pPResourceSnapshot;
        if (snap->amount <= num) {
            num -= snap->amount;
            snap->amount = 0.0;
        } else {
            snap->amount -= num;
            num = 0.0;
        }
        snap->UpdateConfigNodeAmounts();
        if (num <= 0.0) break;
    }
}

// KerbalEVA.cs:5095
void KerbalEVA::getCoordinateFrame() {
    fUp = (FlightGlobals::ready && EvaEnvironment::cameraReferenceUp) ? EvaEnvironment::cameraReferenceUp() : Vector3f::up();
    const Vector3f camFwd = EvaEnvironment::cameraForward ? EvaEnvironment::cameraForward() : Vector3f::forward();
    fFwd = Vector3f::ProjectOnPlane(camFwd, fUp).normalized();
    fRgt = Vector3f::Cross(fUp, fFwd);
}

// ============================================================================================ ROC

// KerbalEVA.cs:5112
void KerbalEVA::PickUpROC() {
    if (EvaEnvironment::IsInstalled("Serenity") && Animations().rockSample.State() != nullptr) {
        if (availableROCPresent) {
            experimentROCPresent = true;
            fsm->RunEvent(On_chopping_roc);
            hammerAnimTimer = 0.0f;
            Events()["PickUpROC"]->guiActive = false;
        } else {
            Events()["PickUpROC"]->guiActive = false;
        }
    }
}

// KerbalEVA.cs:5130
void KerbalEVA::OnROCExperimentFinished(ScienceData* experimentData) {
    if (experimentROCPresent && availableROCSmall && experimentData->subjectID.find("ROCScience") != std::string::npos) {
        EvaEnvironment::Effect("ROC:PickUpROC");
        Events()["PickUpROC"]->guiActive = false;
        experimentROCPresent = false;
    }
}

// KerbalEVA.cs:5140
void KerbalEVA::OnROCExperimentReset(ScienceData* experimentData) {
    if (experimentROCPresent && availableROCSmall && experimentData->subjectID.find("ROCScience") != std::string::npos) Events()["PickUpROC"]->guiActive = true;
}

// ============================================================================================ collisions

// KerbalEVA.cs:5148
void KerbalEVA::OnCollisionEnter(EvaCollision& c) {
    Vessel* vessel = VesselPtr();
    if (vessel->packed || !Ready) return;
    EvaEnvironment::Effect("part.HandleCollision");
    RemoveRBAnchor();
    if (c.layer == 0 || c.layer == 15 || c.layer == 19) {
        // Array.Sort(c.contacts, CompareContactsByNormalToSurface): the runtime's introspective sort (the comparer never returns 0)
        std::vector<EvaContact>& cs = c.contacts;
        dotnet::ListSort(cs, [this](const EvaContact& a, const EvaContact& b) { return CompareContactsByNormalToSurface(a, b); });
        lastCollisionDirection = (cs.at(0).point - transform().position).normalized();
        lastCollisionNormal = cs.at(0).normal;
        if (!isSelfCollision(c)) lastCollisionTime = UT();
        CalculateGroundLevelAngle();
    }
    const float st = stumbleThreshold();
    if (PartPtr()->State() != PartStates::DEAD && fx(c.relativeVelocitySqrMagnitude) > fx(st) * fx(st)) fsm->RunEvent(On_stumble);
    if (c.isRoc) {
        availableROCPresent = true;
        if (availableROCSmall) {
            Events()["PickUpROC"]->guiActive = true;
            Events()["PickUpROC"]->SetGuiName(Localizer::Format("#autoLOC_6011014", std::vector<std::string>{rocDisplayName}));
        }
    }
}

// KerbalEVA.cs:5185
int KerbalEVA::CompareContactsByNormalToSurface(const EvaContact& c1, const EvaContact& c2) {
    Vector3f up = VesselPtr()->upAxis.ToVector3f();
    if (fx(Mathf::Abs(Vector3f::Dot(c1.normal, up))) > fx(Mathf::Abs(Vector3f::Dot(c2.normal, up)))) return -1;
    return 1;
}

// KerbalEVA.cs:5194
void KerbalEVA::OnCollisionStay(EvaCollision&) { EvaEnvironment::Effect("part.LandedCollisionChecks"); }

// KerbalEVA.cs:5199
void KerbalEVA::OnCollisionExit(EvaCollision& c) {
    EvaEnvironment::Effect("part.OnCollisionExit");
    if (c.isRoc) {
        Events()["PickUpROC"]->guiActive = false;
        availableROCPresent = false;
    }
}

// KerbalEVA.cs:5209
bool KerbalEVA::isSelfCollision(const EvaCollision& c) {
    size_t num = ragdollNodes.size();
    while (num-- > 0) {
        if (num < ragdollNodeColliders.size() && c.collider == ragdollNodeColliders[num]) return true;
    }
    size_t num2 = characterColliders.size();
    while (num2-- > 0) {
        if (characterColliders[num2] == c.collider) return true;
    }
    size_t num3 = otherRagdollColliders.size();
    while (num3-- > 0) {
        if (otherRagdollColliders[num3] == c.collider) return true;
    }
    return false;
}

// KerbalEVA.cs:5243
bool KerbalEVA::SurfaceContact() {
    if (!VesselPtr()->Landed) return GetL19Contact();
    return true;
}

// KerbalEVA.cs:5252
bool KerbalEVA::SurfaceOrSplashed() {
    if (!VesselPtr()->LandedOrSplashed()) return GetL19Contact();
    return true;
}

// KerbalEVA.cs:5261
bool KerbalEVA::GetL19Contact() {
    Vessel* v = VesselPtr();
    if (!v->Splashed && v->heightFromTerrain < 1000.0f && v->srfSpeed < 10.0) {
        size_t count = currentCollisionLayers.size();
        while (count-- > 0) {
            if (currentCollisionLayers[count] == 19) return true;
        }
    }
    return false;
}

// KerbalEVA.cs:5277
void KerbalEVA::AnchorUpdate() {
    Vessel* vessel = VesselPtr();
    KFSMState* cs = fsm->CurrentState();
    if (vessel->loaded && !vessel->packed && vessel->Landed && fx(vessel->rb_velocity.sqrMagnitude()) < fx(0.1f) &&
        (cs == st_idle_gr || cs == st_idle_b_gr || cs == st_enteringConstruction || cs == st_exitingConstruction || cs == st_weld)) {
        if (kerbalAnchorTimeCounter < kerbalAnchorTimeThreshold) {
            kerbalAnchorTimeCounter = f32store(fx(kerbalAnchorTimeCounter) + fx(Time::deltaTime));
        } else if (heightFromSurfaceHitDistance != 0.0f && fx(heightFromSurfaceHitDistance) > fx(onFallHeightFromTerrain())) {
            RemoveRBAnchor();
        } else if (heightFromSurfaceHitDistance != 0.0f && !vesselSurfaceAnchored) {
            RemoveRBAnchor();
        } else if (JetpackDeployed() && JetpackIsThrusting) {
            RemoveRBAnchor();
        } else if (vessel->GroundLevelAngle() == -1.0f) {
            CalculateGroundLevelAngle();
            RemoveRBAnchor();
        } else if (fx(vessel->GroundLevelAngle()) < fx(GameSettings::EVA_MAX_SLOPE_ANGLE)) {
            AddRBAnchor();
        } else {
            RemoveRBAnchor();
        }
    } else {
        RemoveRBAnchor();
    }
}

// KerbalEVA.cs:5317
void KerbalEVA::AddRBAnchor() {
    if (!isAnchored) {
        isAnchored = true;
        Engine().SetFixedJointAnchor(true);
    }
}

// KerbalEVA.cs:5326
void KerbalEVA::RemoveRBAnchor() {
    if (isAnchored) {
        isAnchored = false;
        Engine().SetFixedJointAnchor(false);
        kerbalAnchorTimeCounter = 0.0f;
    }
}

// ============================================================================================ ragdoll

// KerbalEVA.cs:5336
void KerbalEVA::SetupRagdoll(Part& part) {
    size_t num = ragdollNodes.size();
    for (size_t i = 0; i < num; i++) {
        // obj.rb.mass *= massMultiplier; obj.rb.useGravity = false; CharacterJoint.enablePreprocessing = false: per-node rigid body data (shell)
        Engine().Named("ragdollNode:" + std::to_string(i) + ":setup");
    }
    part.fields.mass = f32store(fx(initialMass()) * fx(massMultiplier()));
    part.fields.prefabMass = part.fields.mass;
    part.fields.needPrefabMass = false;
    Engine().SetRbMass(part.fields.mass);
}

// KerbalEVA.cs:5361
void KerbalEVA::EnableCharacterAndLadderColliders(bool enable) {
    Engine().SetCharacterCollidersEnabled(enable);
    if (HasColliderLadder) Engine().SetLadderColliderEnabled(enable);
}

// KerbalEVA.cs:5374
void KerbalEVA::SetRagdoll(bool ragDoll, bool preservePose) {
    if (ragDoll && Engine().AnimEnabled()) Engine().SetAnimEnabled(false);
    size_t num = ragdollNodes.size();
    while (num-- > 0) {
        KerbalRagdollNode& node = ragdollNodes[num];
        node.goActive = ragDoll;
        Engine().NodeSetActive(static_cast<int>(num), ragDoll);
        if (ragDoll) Engine().NodeSetVelocity(static_cast<int>(num), node.velocity);
        else node.velocity = Engine().RbVelocity();
    }
    Engine().SetCharacterCollidersEnabled(!ragDoll);
    Engine().SetOtherRagdollCollidersEnabled(ragDoll);
    if (!ragDoll && !Engine().AnimEnabled()) Engine().SetAnimEnabled(!preservePose);
    if (ragDoll && partPlacementMode && moduleInventoryPartReference != nullptr) moduleInventoryPartReference->CancelPartPlacementMode();
    if (ragDoll && (sciencePanelAnimPlaying || playingGolfAnimPlaying || smashingBananaAnimPlaying || spinningWingnutAnimPlaying)) {
        sciencePanelAnimPlaying = false;
        playingGolfAnimPlaying = false;
        smashingBananaAnimPlaying = false;
        spinningWingnutAnimPlaying = false;
        UnlockInput("ControlPanelLock_" + VesselPtr()->id.ToString());
    }
}

// KerbalEVA.cs:5422
void KerbalEVA::IntegrateRagdollRigidbodyForces() {
    Vessel* vessel = VesselPtr();
    if (vessel->packed) return;
    if (referenceFrameChanged_rdPhysHold != 0) {
        updateRagdollVelocities();
        referenceFrameChanged_rdPhysHold--;
        return;
    }
    Part* p = PartPtr();
    CelestialBody* refBody = vessel->orbitDriver->orbit->referenceBody;
    const double grav = PhysicsGlobals::GraviticForceMultiplier;
    geeForce = FlightGlobals::getGeeForceAtPosition(W(transform().position)) * grav;
    centrifugalForce = FlightGlobals::getCentrifugalAcc(W(transform().position), *refBody) * grav;
    coriolisForce = FlightGlobals::getCoriolisAcc(W(vessel->rb_velocity + Krakensbane::GetFrameVelocityV3f()), *refBody) * grav;
    size_t num = ragdollNodes.size();
    for (size_t i = 0; i < num; i++) {
        KerbalRagdollNode& node = ragdollNodes[i];
        const int idx = static_cast<int>(i);
        Engine().NodeSetDrag(idx, Engine().RbDrag(), Engine().RbAngularDrag());
        Engine().NodeAddForce(idx, geeForce.ToVector3f(), ForceMode::Acceleration);
        Engine().NodeAddForce(idx, centrifugalForce.ToVector3f(), ForceMode::Acceleration);
        Engine().NodeAddForce(idx, coriolisForce.ToVector3f(), ForceMode::Acceleration);
        if (!ragdollNodeIsRoot(i)) Engine().NodeAddForce(idx, Krakensbane::GetLastCorrection().ToVector3f(), ForceMode::VelocityChange);
        if (vessel->Splashed && partBuoyancyDisplacement) {
            const double depthOfNode = static_cast<double>(FlightGlobals::getAltitudeAtPos(W(node.goPosition), vessel->mainBody()));
            const float num2 = Mathf::Max(0.0f, f32store(0.0 - fx(static_cast<float>(depthOfNode))));
            const double disp = partBuoyancyDisplacement();
            PhysicsGlobals* pg = PhysicsGlobals::Instance();
            if (pg == nullptr) throw Nre();
            Vector3d g = FlightGlobals::getGeeForceAtPosition(W(p->partTransform.position));
            const double scale = (disp != 0.0 ? disp : pg->buoyancyDefaultVolume) * vessel->mainBody()->oceanDensity *
                                 ((static_cast<double>(num2) < pg->buoyancyScaleAboveDepth) ? (static_cast<double>(num2) / pg->buoyancyScaleAboveDepth) : 1.0) *
                                 pg->buoyancyScalar * pg->buoyancyKerbalsRagdoll;
            Vector3d f(-g.x * scale, -g.y * scale, -g.z * scale);
            Engine().NodeAddForce(idx, f.ToVector3f(), ForceMode::Acceleration);
        }
    }
}

// KerbalEVA.cs:5459
void KerbalEVA::updateRagdollVelocities() {
    if (!VesselPtr()->packed) {
        size_t num = ragdollNodes.size();
        const Vector3d rbv = W(Engine().RbVelocity());
        const float fdtRecip = f32store(1.0 / fx(Time::fixedDeltaTime));
        while (num-- > 0) ragdollNodes[num].updateVelocity(transform().position, rbv, fdtRecip);
    }
}

// KerbalEVA.cs:5471
void KerbalEVA::CalculateGroundLevelAngle() {
    if (vesselGetGroundLevelAngle) vesselGetGroundLevelAngle(*VesselPtr());
    const Vector3f up = EvaEnvironment::upAxis ? EvaEnvironment::upAxis() : FlightGlobals::getUpAxis().ToVector3f();
    Vector3f origin = transform().position - up * halfHeight;
    EvaRaycastHit hitInfo, hitInfo2;
    const int mask = kDefaultEquivalent | 0x8000 | 0x80000;
    bool flag = Engine().Raycast(origin, transform().forward(), 0.25f, mask, hitInfo);
    bool flag2 = Engine().Raycast(origin, -transform().forward(), 0.25f, mask, hitInfo2);
    if (flag && !flag2) slopeMovementDirection = 1;
    else if (!flag && flag2) slopeMovementDirection = -1;
    else if (flag && flag2) slopeMovementDirection = -1;
    else slopeMovementDirection = 0;
}

// KerbalEVA.cs:5498
void KerbalEVA::OnVesselSituationChange(HostedSituation vcs) {
    if (vcs.host == VesselPtr() && fsm->CurrentState() == st_idle_gr) Events()["PlantFlag"]->active = CanPlantFlag();
}

// KerbalEVA.cs:5506
void KerbalEVA::onReferencebodyChanged(BodyChange) {
    referenceFrameChanged_rdPhysHold = 2;
    Vessel* vessel = VesselPtr();
    Vector3d vel = vessel->orbitDriver->orbit->GetVel();
    const Vector3d fv = Krakensbane::GetFrameVelocity();
    if (!(fv.x == 0.0 && fv.y == 0.0 && fv.z == 0.0) && !(vel.sqrMagnitude() < Krakensbane::SqrThreshold())) return;
    vel = vel - fv;
    size_t num = ragdollNodes.size();
    while (num-- > 0) {
        if (!ragdollNodeIsRoot(num)) {
            const Vector3f nv = Engine().NodeVelocity(static_cast<int>(num));
            Engine().NodeAddForce(static_cast<int>(num), Vector3d(vel.x - nv.x, vel.y - nv.y, vel.z - nv.z).ToVector3f(), ForceMode::VelocityChange);
        }
    }
}

// KerbalEVA.cs:5526
void KerbalEVA::onRotatingFrameChanged(FrameChange frm) {
    referenceFrameChanged_rdPhysHold = 2;
    const Vector3d rfv = frm.host->getRFrmVel(W(PartPtr()->partTransform.position));
    const Vector3d fv = Krakensbane::GetFrameVelocity();
    const double sgn = frm.target ? -1.0 : 1.0;
    const Vector3f rbv = Engine().RbVelocity();
    Vector3d vector3d(rbv.x + rfv.x * sgn + fv.x, rbv.y + rfv.y * sgn + fv.y, rbv.z + rfv.z * sgn + fv.z);
    if (!(fv.x == 0.0 && fv.y == 0.0 && fv.z == 0.0) && !(vector3d.sqrMagnitude() < Krakensbane::SqrThreshold())) return;
    size_t num = ragdollNodes.size();
    while (num-- > 0) {
        if (!ragdollNodeIsRoot(num)) {
            const Vector3f nv = Engine().NodeVelocity(static_cast<int>(num));
            const Vector3d r = frm.host->getRFrmVel(W(PartPtr()->partTransform.position));
            if (frm.target) Engine().NodeSetVelocity(static_cast<int>(num), Vector3d(nv.x - r.x, nv.y - r.y, nv.z - r.z).ToVector3f());
            else Engine().NodeSetVelocity(static_cast<int>(num), Vector3d(nv.x + r.x, nv.y + r.y, nv.z + r.z).ToVector3f());
        }
    }
}

// KerbalEVA.cs:5552
void KerbalEVA::onFrameVelocityChange(const Vector3d& velOffset) {
    referenceFrameChanged_rdPhysHold = 2;
    size_t num = ragdollNodes.size();
    while (num-- > 0) {
        if (!ragdollNodeIsRoot(num)) Engine().NodeAddForce(static_cast<int>(num), velOffset.ToVector3f(), ForceMode::VelocityChange);
    }
}

// KerbalEVA.cs:5566
void KerbalEVA::OnVesselGoOnRails(Vessel* v) {
    if (v == VesselPtr()) {
        if (fsm->currentStateName.find("Ladder") != std::string::npos) fsm->RunEvent(On_ladderLetGo);
        if (isRagdoll) SetRagdoll(false, true);
    }
}

// KerbalEVA.cs:5581
void KerbalEVA::OnVesselGoOffRails(Vessel* v) {
    if (v == VesselPtr()) {
        StartCoroutine(waitAndHandleRagdollTimeWarp(3));
        if (fsm->CurrentState() == st_idle_gr) Events()["PlantFlag"]->active = CanPlantFlag();
    }
}

// KerbalEVA.cs:5593
Enumerator KerbalEVA::waitAndHandleRagdollTimeWarp(int waitFrames) {
    int i = 0;
    while (i < waitFrames) {
        co_yield YieldInstruction::Null();
        int num = i + 1;
        i = num;
    }
    log::Info("Unpacking kerbalEVA. Vel: (" + std::to_string(Engine().RbVelocity().x) + ", " + std::to_string(Engine().RbVelocity().y) + ", " + std::to_string(Engine().RbVelocity().z) + ")");
    if (isRagdoll) {
        size_t num2 = ragdollNodes.size();
        while (num2-- > 0) ragdollNodes[num2].velocity = Engine().RbVelocity();
        SetRagdoll(true);
        if (InConstructionMode && EVAConstructionModeController::Instance != nullptr) {
            EVAConstructionModeController::Instance->ClosePanel();
            if (weldFX == nullptr) throw Nre();
            weldFX->Stop();
            alternateIdleDisabled = false;
            ToggleWeldingGun(false);
            UnlockInput("WeldLock_" + VesselPtr()->id.ToString());
        }
    } else {
        SetRagdoll(false);
    }
}

// KerbalEVA.cs:5626
void KerbalEVA::ResetRagdollLinks() {
    size_t num = ragdollNodes.size();
    while (num-- > 0) {
        ragdollNodes[num].velocity = Vector3f::zero();
        Engine().Named("ragdollNode:" + std::to_string(num) + ":connectedBody=rb");
    }
}

// KerbalEVA.cs:5641
void KerbalEVA::OnPartDie() { EvaEnvironment::Effect("RDPartCollisionHandler:destroyAll"); }

// KerbalEVA.cs:5651
void KerbalEVA::Splat(const Vector3f& point, const Vector3f& normal) {
    Quaternionf rotation = UnityNative::FromToRotation(Vector3f::up(), normal);
    (void)rotation;
    EvaEnvironment::Effect("splat:" + std::to_string(point.x + normal.x * 0.1f));
}

// KerbalEVA.cs:5657
bool KerbalEVA::CanRecover() {
    Vessel* vessel = VesselPtr();
    const float rt = recoverThreshold();
    canRecover = fx(Engine().RbVelocity().sqrMagnitude()) < fx(rt) * fx(rt) || (!vessel->LandedOrSplashed() && UT() > lastCollisionTime + recoverTime());
    if (canRecover) {
        if (!(FlightRegistry::ActiveVessel == vessel)) return true;
        return NotZero(tgtRpos);
    }
    return false;
}

// KerbalEVA.cs:5671
void KerbalEVA::SetEjectDirection() {
    ejectDirection = Vector3f::up();
    Part* p = PartPtr();
    if (p->parent == nullptr) throw Nre();
    // for each module of the parent: `partModule is KerbalSeat` with Occupant == base.part: the seat is the seat package's (kerbalSeat set by the host)
    if (kerbalSeat != nullptr && kerbalSeat->Occupant() == p) ejectDirection = kerbalSeat->EjectDirection();
}

// KerbalEVA.cs:5696
void KerbalEVA::EjectFromSeat() {
    Part* p = PartPtr();
    Vessel* vessel = VesselPtr();
    Vector3f ejectPoint = GetEjectPoint(ejectDirection, 50.0f, halfHeight, halfHeight);
    p->Undock(kerbalVesselInfo.get());
    vessel->SetPosition(Vector3d(ejectPoint.x, ejectPoint.y, ejectPoint.z), true);
    vessel->IgnoreGForces(1);
    if (kerbalSeat != nullptr && kerbalSeat->EjectionForce() > 0.0f) {
        // base.part.rb.AddForce(partTransform.TransformVector(ejectionForceDirection.normalized * EjectionForce))
        Engine().AddForce(p->partTransform.TransformDirection(kerbalSeat->EjectionForceDirection().normalized() * kerbalSeat->EjectionForce()));
    }
    if (EvaEnvironment::portraitGallery) EvaEnvironment::portraitGallery(*this, false);
}

// KerbalEVA.cs:5717
Enumerator KerbalEVA::AcquireRotation(Quaternionf tgtRot, float duration) {
    if (!VesselPtr()->packed) {
        Engine().SetRbAngularVelocity(Vector3f::zero());
        Quaternionf iRot = transform().rotation;
        float startTime = Time::time;
        float endTime = f32store(fx(Time::time) + fx(duration));
        while (Time::time < endTime) {
            transform().rotation = QuatLerp(iRot, tgtRot, Mathf::InverseLerp(startTime, endTime, Time::time));
            co_yield YieldInstruction::Null();
        }
    }
}

// KerbalEVA.cs:5733
Enumerator KerbalEVA::AcquirePosition(Vector3f tgtPos, float duration) {
    if (!VesselPtr()->packed) {
        Engine().SetRbVelocity(Vector3f::zero());
        Vector3f iPos = transform().position;
        float startTime = Time::time;
        float endTime = f32store(fx(Time::time) + fx(duration));
        while (Time::time < endTime) {
            transform().position = Vector3f::Lerp(iPos, tgtPos, Mathf::InverseLerp(startTime, endTime, Time::time));
            co_yield YieldInstruction::Null();
        }
    }
}

// KerbalEVA.cs:7345
void KerbalEVA::ModifyBodyColliderHeight(float newHeight) { Engine().SetBodyColliderCenterY(newHeight); }

// ============================================================================================ ladders

// KerbalEVA.cs:5749
void KerbalEVA::correctLadderPosition() {
    Vessel* vessel = VesselPtr();
    if (vessel->packed || currentLadder == nullptr) return;
    if (currentLadder->hasRigidbody) {
        // currentLadder.attachedRigidbody.velocity + Vector3.Cross(angularVelocity, rb.worldCenterOfMass - attachedRigidbody.worldCenterOfMass)
        Vtgt = currentLadder->rbVelocity + Vector3f::Cross(currentLadder->rbAngularVelocity, Engine().RbWorldCenterOfMass() - currentLadder->rbWorldCenterOfMass);
    } else {
        Vtgt = Vector3f::zero();
    }
    Engine().SetRbVelocity(Vtgt);
    vessel->gravityMultiplier = 1.0;
    onLadder = true;
    Vector3f zero = Vector3f::zero();
    if (isLadderJointed) {
        zero = (currentLadderPart() != nullptr) ? currentLadderPart()->partTransform.TransformPoint(ladderPosition) : currentLadder->pose.TransformPoint(ladderPosition);
        if (currentLadderPart() != nullptr && currentLadderPart()->vessel->perturbation_immediate.magnitude() > GameSettings::EVA_LADDER_JOINT_BREAK_ACCELERATION &&
            currentLadderPart()->vessel->velocityD.magnitude() > GameSettings::EVA_LADDER_JOINT_BREAK_VELOCITY)
            ClearLadderHold();
    } else {
        ladderPos = Vector3f::ProjectOnPlane(currentLadder->pose.position - Engine().LadderPivotPosition(), currentLadder->pose.up());
        zero = Vector3f::Lerp(transform().position, ladderPos + transform().position, f32store(fx(10.0f) * fx(Time::deltaTime)));
    }
    transform().position = zero;
}

// KerbalEVA.cs:5776
void KerbalEVA::correctLadderRotation() {
    if (!VesselPtr()->packed && !(currentLadder == nullptr)) {
        ladderUp = currentLadder->pose.up() * (invLadderAxis ? -1.0f : 1.0f);
        ladderFwd = currentLadder->pose.forward();
        transform().rotation = QuatLerp(transform().rotation, UnityNative::FromToRotation(transform().up(), ladderUp) * transform().rotation,
                                        f32store(fx(10.0f) * fx(Time::deltaTime)));
        transform().rotation = QuatLerp(transform().rotation, UnityNative::FromToRotation(transform().forward(), ladderFwd) * transform().rotation,
                                        f32store(fx(5.0f) * fx(Time::deltaTime)));
    }
}

// KerbalEVA.cs:5787
void KerbalEVA::UpdateLadderMovement() {
    if (!VesselPtr()->packed && !(currentLadder == nullptr)) {
        float num = static_cast<float>(fsm->TimeAtCurrentState());
        num = (num >= 0.3f) ? 1.0f : ((!(num > 0.0f)) ? 0.0f : f32store(fx(num) * fx(3.3333333f)));
        currentSpd = Mathf::Lerp(lastTgtSpeed, tgtSpeed, num);
        if (((currentSpd < 0.0f && !canDescend) || (currentSpd > 0.0f && !canClimb)) && GameSettings::EVA_LADDER_CHECK_END) currentSpd = 0.0f;
        Vector3f lad = Vector3f::zero();
        if (currentLadder->hasRigidbody)
            lad = currentLadder->rbVelocity + Vector3f::Cross(currentLadder->rbAngularVelocity, Engine().RbWorldCenterOfMass() - currentLadder->rbWorldCenterOfMass);
        Engine().SetRbVelocity(lad + ladderUp * currentSpd);
    }
}

// KerbalEVA.cs:5802
void KerbalEVA::InterpolateLadders() {
    if (currentLadderTriggers.size() > 1 && currentLadder != nullptr && !ladderTransition &&
        fx(Vector3f::Dot(currentLadder->pose.forward(), currentLadderTriggers[1]->pose.forward())) > fx(MinLadderForwardDot) &&
        fx(Mathf::Abs(Vector3f::Dot(currentLadder->pose.right(), currentLadderTriggers[1]->pose.right()))) > fx(MinLadderRightDot)) {
        secondaryLadder = currentLadderTriggers[1];
    } else if (NotZero(ladderTgtRPos)) {
        secondaryLadder = nullptr;
    }
}

// KerbalEVA.cs:5814
void KerbalEVA::AutoTransition() {
    if (!(secondaryLadder != nullptr) || ladderTransition) return;
    Vector3f lhs = currentLadder->pose.position - secondaryLadder->pose.position;
    float num = Vector3f::Dot(lhs, secondaryLadder->pose.forward());
    float num2 = Vector3f::Dot(lhs, currentLadder->pose.forward());
    const Vector3f pivot = Engine().LadderPivotPosition();
    if (num > 0.0f && num2 < 0.0f) {
        if (Vector3f::Dot(pivot - secondaryLadder->pose.position, secondaryLadder->pose.forward()) < 0.0f) {
            if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = false;
            currentLadder = secondaryLadder;
            set_currentLadderPart(currentLadder->part);
            if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = true;
            secondaryLadder = nullptr;
            ladderTransition = true;
        }
    } else if (num < 0.0f && num2 > 0.0f && Vector3f::Dot(pivot - secondaryLadder->pose.position, secondaryLadder->pose.forward()) > 0.0f) {
        if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = false;
        currentLadder = secondaryLadder;
        set_currentLadderPart(currentLadder->part);
        if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = true;
        secondaryLadder = nullptr;
        ladderTransition = true;
    }
}



// KerbalEVA.cs:5858
void KerbalEVA::UpdateCurrentLadder() {
    if (VesselPtr()->packed || currentLadderTriggers.size() < 1) return;
    PostInteractionScreenMessage(cacheAutoLOC_115662);
    AutoTransition();
    if (NotZero(ladderTgtRPos) && !ladderTransition) {
        dotnet::ListSort(currentLadderTriggers, [this](EvaCollider* a, EvaCollider* b) { return SortTriggersByAlignment(a, b); });
        EvaCollider* collider = currentLadderTriggers[0];
        if (currentLadder != collider && fx(Mathf::Abs(Vector3f::Dot(ladderTgtRPos, collider->pose.up()))) > fx(0.3f)) {
            currentLadder = collider;
            if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = false;
            set_currentLadderPart(currentLadder->part);
            if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = true;
            invLadderAxis = fx(Vector3f::Dot(ladderTgtRPos.normalized(), currentLadder->pose.up())) * (tgtSpeed != 0.0f ? fx(Mathf::Sign(tgtSpeed)) : 1.0) < 0.0;
            if (Key(EvaKey::Left, KeyQuery::Held) || Key(EvaKey::Right, KeyQuery::Held)) ladderTransition = true;
        }
    }
    if (Key(EvaKey::Left, KeyQuery::Up) || Key(EvaKey::Right, KeyQuery::Up) || currentLadderTriggers.size() == 1) ladderTransition = false;
}

// KerbalEVA.cs:5895
void KerbalEVA::UpdateCurrentLadderIdle() {
    if (VesselPtr()->packed || currentLadderTriggers.size() < 1) return;
    PostInteractionScreenMessage(cacheAutoLOC_115694);
    if (currentLadderTriggers.size() == 1) {
        ladderTransition = false;
        return;
    }
    AutoTransition();
    if (fx(Mathf::Abs(Vector3f::Dot(ladderTgtRPos, transform().right()))) > fx(0.9f) && !ladderTransition) {
        ladderTgtRPos = Vector3f::ProjectOnPlane(ladderTgtRPos, transform().up());
        dotnet::ListSort(currentLadderTriggers, [this](EvaCollider* a, EvaCollider* b) { return SortTriggersByAlignment(a, b); });
        EvaCollider* collider = currentLadderTriggers[0];
        if (currentLadder != collider && fx(Mathf::Abs(Vector3f::Dot(ladderTgtRPos, collider->pose.up()))) > fx(0.3f)) {
            currentLadder = collider;
            if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = false;
            set_currentLadderPart(currentLadder->part);
            if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = true;
            invLadderAxis = fx(Vector3f::Dot(ladderTgtRPos.normalized(), currentLadder->pose.up())) < 0.0;
            ladderTransition = true;
        }
    }
    if (Key(EvaKey::Left, KeyQuery::Up) || Key(EvaKey::Right, KeyQuery::Up)) ladderTransition = false;
}

// KerbalEVA.cs:5935
void KerbalEVA::CheckLadderTriggers() {
    if (currentLadderTriggers.size() <= 0) return;
    std::vector<EvaCollider*> list;
    for (size_t i = 0; i < currentLadderTriggers.size(); i++) {
        if (currentLadderTriggers[i] != nullptr && !currentLadderTriggers[i]->enabled) list.push_back(currentLadderTriggers[i]);
    }
    for (size_t j = 0; j < list.size(); j++) {
        auto it = std::find(currentLadderTriggers.begin(), currentLadderTriggers.end(), list[j]);
        if (it != currentLadderTriggers.end()) currentLadderTriggers.erase(it);
    }
}

// KerbalEVA.cs:5955
int KerbalEVA::SortTriggersByDistance(EvaCollider* c1, EvaCollider* c2) {
    const Vector3f pivot = Engine().LadderPivotPosition();
    Vector3f vector = Engine().ColliderClosestPoint(*c1, pivot);
    Vector3f vector2 = Engine().ColliderClosestPoint(*c2, pivot);
    if (fx((vector - pivot).sqrMagnitude()) <= fx((vector2 - pivot).sqrMagnitude())) return -1;
    return 1;
}

// KerbalEVA.cs:5966
int KerbalEVA::SortTriggersByAlignment(EvaCollider* c1, EvaCollider* c2) {
    if (fx(Mathf::Abs(Vector3f::Dot(ladderTgtRPos.normalized(), c1->pose.up()))) >=
        fx(Mathf::Abs(Vector3f::Dot(ladderTgtRPos.normalized(), c2->pose.up()))) - fx(0.001f))
        return -1;
    return 1;
}

// KerbalEVA.cs:5975
Enumerator KerbalEVA::StartNonCollidePeriod(float duration, float standoff, Part* fromPart, const PartTransform* airlockTrf) {
    float T0 = realtimeSinceStartup ? realtimeSinceStartup() : Time::time;
    Engine().SetCharacterCollidersTrigger(true);
    while (fx(T0) + fx(duration) > fx(realtimeSinceStartup ? realtimeSinceStartup() : Time::time)) {
        if (airlockTrf != nullptr && fromPart != nullptr) {
            transform().rotation = airlockTrf->rotation;
            transform().position = airlockTrf->position + airlockTrf->rotation * (Vector3f::back() * standoff);
            Vessel* fv = fromPart->vessel;
            const Vector3f fpv = fromPartVelocity ? fromPartVelocity(*fromPart) : Vector3f::zero();
            Vector3f av = fv != nullptr ? fv->angularVelocity : Vector3f::zero();
            Vector3f com = fv != nullptr ? fv->CurrentCoM() : Vector3f::zero();
            Engine().SetRbVelocity(fpv + Vector3f::Cross(transform().position - com, av));
            Engine().SetRbAngularVelocity(av);
            co_yield YieldInstruction::WaitForFixedUpdate();
        }
        Engine().SetCharacterCollidersTrigger(false);
        if (airlockTrf == nullptr || fromPart == nullptr) break;  // (the C# loop spins on the clock; without an airlock it would never yield)
    }
}

// KerbalEVA.cs:7354
void KerbalEVA::SetLadderHold() {
    if (GameSettings::EVA_LADDER_JOINT_WHEN_IDLE && onLadder && !isLadderJointed) {
        ladderPosition = (currentLadderPart() != nullptr) ? currentLadderPart()->partTransform.InverseTransformPoint(transform().position)
                                                          : currentLadder->pose.InverseTransformPoint(transform().position);
        isLadderJointed = true;
    }
}

// KerbalEVA.cs:7364
void KerbalEVA::ClearLadderHold() { isLadderJointed = false; }

// KerbalEVA.cs:6860
void KerbalEVA::OnTriggerStay(EvaCollider* o) {
    if (o->tag == "Ladder") {
        Part* partUpwardsCached = o->part;
        if ((partUpwardsCached == nullptr || o->partIsAttached) && std::find(currentLadderTriggers.begin(), currentLadderTriggers.end(), o) == currentLadderTriggers.end()) {
            currentLadderTriggers.push_back(o);
            dotnet::ListSort(currentLadderTriggers, [this](EvaCollider* a, EvaCollider* b) { return SortTriggersByDistance(a, b); });
        }
    }
    if (o->tag == "Airlock") {
        if (o != currentAirlockTrigger) currentAirlockPart = o->part;
        currentAirlockTrigger = o;
    }
}

// KerbalEVA.cs:6881
void KerbalEVA::OnTriggerExit(EvaCollider* o) {
    if (o->tag == "Ladder") {
        auto it = std::find(currentLadderTriggers.begin(), currentLadderTriggers.end(), o);
        if (it != currentLadderTriggers.end()) currentLadderTriggers.erase(it);
        if (currentLadderTriggers.size() > 0) dotnet::ListSort(currentLadderTriggers, [this](EvaCollider* a, EvaCollider* b) { return SortTriggersByDistance(a, b); });
    }
    if (o->tag == "Airlock" && o == currentAirlockTrigger) {
        currentAirlockTrigger = nullptr;
        currentAirlockPart = nullptr;
    }
}

// ============================================================================================ clamber / control panel

// KerbalEVA.cs:6001
bool KerbalEVA::FindClamberSrf(float fwdOffset, float reach) {
    clamberOrigin = transform().position + transform().forward() * fwdOffset + fUp * reach;
    clamberTarget = transform().position + transform().forward() * fwdOffset;
    const int mask = kDefaultEquivalent | 0x8000 | 0x80000;
    if (Engine().Raycast(clamberOrigin, (clamberTarget - clamberOrigin).normalized(), reach, mask, clamberHitInfo)) {
        EvaRaycastHit up;
        if (Engine().Raycast(transform().position + fUp * halfHeight, fUp, f32store(fx(reach) - fx(clamberHitInfo.distance) + fx(halfHeight) * 3.0), -1, up)) return false;
        return true;
    }
    return false;
}

// KerbalEVA.cs:6016
bool KerbalEVA::TestClamberSrf(EvaRaycastHit& hit) {
    if (hit.collider != nullptr && hit.collider->hasRigidbody) {
        sciencePartPresent = false;
        if (groundSciencePartAt) sciencePartPresent = groundSciencePartAt(*hit.collider, nullptr);
        if (sciencePartPresent) return false;
        if (fx(hit.collider->rbVelocity.sqrMagnitude()) < fx(0.01f)) return fx(hit.collider->rbAngularVelocity.sqrMagnitude()) < fx(0.01f);
        return false;
    }
    return true;
}

// KerbalEVA.cs:6034
std::unique_ptr<ClamberPath> KerbalEVA::GetClamberPath(float fwdOffset, float reach) {
    Vector3f zero = Vector3f::zero(), zero2 = Vector3f::zero(), zero3 = Vector3f::zero(), zero4 = Vector3f::zero(), zero5 = Vector3f::zero();
    clamberOrigin = transform().position + transform().forward() * fwdOffset + fUp * reach;
    clamberTarget = transform().position + transform().forward() * fwdOffset;
    Vector3f forward = transform().forward();
    float num = 0.0f;
    std::unique_ptr<ClamberPath> path;
    const int mask = kDefaultEquivalent | 0x8000 | 0x80000;
    if (Engine().Raycast(clamberOrigin, (clamberTarget - clamberOrigin).normalized(), reach, mask, clamberHitInfo)) {
        zero3 = clamberHitInfo.point;
        zero4 = clamberHitInfo.normal;
        num = Vector3f::Dot(zero3 - transform().position, fUp);
        EvaCollider* col = clamberHitInfo.collider;
        if (col != nullptr && Engine().ColliderRaycast(*col, transform().position + fUp * f32store(fx(num) - fx(0.01f)), forward, reach, clamberHitInfo)) {
            zero2 = clamberHitInfo.point;
            zero5 = Vector3f::ProjectOnPlane(clamberHitInfo.normal, fUp).normalized();
            zero = zero2 + UnityNative::AngleAxis(90.0f, Vector3f::Cross(zero5, zero4)) * (zero3 - zero2);
            path = std::make_unique<ClamberPath>(zero5, zero4, zero, zero2, zero3, num, halfHeight);
        }
    }
    return path;
}

// KerbalEVA.cs:6066
bool KerbalEVA::FindControlPanel(float fwdOffset, float reach) {
    if (!EvaEnvironment::IsInstalled("Serenity")) return false;
    controlOrigin = transform().position + transform().forward() * fwdOffset + fUp * reach;
    controlTarget = transform().position + transform().forward() * fwdOffset;
    const int mask = kDefaultEquivalent | 0x8000 | 0x80000;
    if (Engine().Raycast(controlOrigin, (controlTarget - controlOrigin).normalized(), reach, mask, controlHitInfo)) {
        bool deployed = false;
        sciencePartPresent = false;
        if (groundSciencePartAt && controlHitInfo.collider != nullptr) sciencePartPresent = groundSciencePartAt(*controlHitInfo.collider, &deployed);
        if (sciencePartPresent && deployed) {
            PostInteractionScreenMessage(cacheAutoLOC_6012032);
            return true;
        }
        return false;
    }
    return false;
}

// KerbalEVA.cs:6087
void KerbalEVA::ControlPanelInteractionFinished() {
    if (EvaEnvironment::IsInstalled("Serenity") && sciencePanelAnimPlaying) {
        if (sciencePanelAnimCooldown > 0.0f) {
            sciencePanelAnimCooldown = f32store(fx(sciencePanelAnimCooldown) - fx(Time::deltaTime));
            return;
        }
        sciencePanelAnimPlaying = false;
        UnlockInput("ControlPanelLock_" + VesselPtr()->id.ToString());
        fsm->RunEvent(On_control_panel_interacting);
    }
}

// KerbalEVA.cs:6102
void KerbalEVA::RandomControlPanelAnim() {
    Animations().controlPanelAnimSelector = UnityRandom::Global().Range(0, static_cast<int>(Animations().controlPanelAnims.size()));
}

// KerbalEVA.cs:6107
void KerbalEVA::RocSampleStored() {
    if (!EvaEnvironment::IsInstalled("Serenity") || !pickRocSampleAnimPlaying) return;
    if (pickRocSampleAnimCooldown > 0.0f) {
        pickRocSampleAnimCooldown = f32store(fx(pickRocSampleAnimCooldown) - fx(Time::deltaTime));
        return;
    }
    pickRocSampleAnimPlaying = false;
    UnlockInput("ControlPanelLock_" + VesselPtr()->id.ToString());
    fsm->RunEvent(On_roc_sample_stored);
    if (experimentROCPresent) EvaEnvironment::Effect("ROC:PerformExperiment");
}

}  // namespace ksp

namespace ksp {
// vessel.GetHeightFromSurface() (Vessel.cs:2675): the downward ray from the vessel transform; the vessel's own ray is the physics bubble's
float KerbalEVA::HeightFromSurface() {
    if (vesselGetHeightFromSurface) return vesselGetHeightFromSurface();
    return VesselPtr()->heightFromSurface;
}
}  // namespace ksp
