// WP-32: KerbalEVA.cs part 2: the FSM state callbacks (OnEnter / OnLeave / OnUpdate bodies, KerbalEVA.cs:1973-3266, 4662-4775, 6226-6607).
// Unity calls (`this.GetComponentCached(ref _animation).X`, `rb.X`, `base.transform.X`) go through Engine() / transform(); see eva_engine.hpp.
#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/eva/kerbal_eva.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/inventory/construction.hpp"
#include "ksp/inventory/inventory.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/modules/parachutes.hpp"
#include "ksp/parts/part_geometry_util.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/unity/random.hpp"

namespace ksp {

using GE = events::GameEvents;
using KSP::Localization::Localizer;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
}  // namespace

// Animation shorthand: `this.GetComponentCached(ref _animation).CrossFade(Animations.x, fade, mode)` etc.
#define A_CF(anim, fade) Engine().AnimCrossFade((anim).GetAnimationString(), (fade), PlayMode::StopSameLayer)
#define A_CFM(anim, fade, mode) Engine().AnimCrossFade((anim).GetAnimationString(), (fade), (mode))
#define A_BL(anim, w, fade) Engine().AnimBlend((anim).GetAnimationString(), (w), (fade))
#define A_PLAY(anim, mode) Engine().AnimPlay((anim).GetAnimationString(), (mode))
#define A_STOP(anim) Engine().AnimStop((anim).GetAnimationString())
#define ANIM Animations()

double KerbalEVA::UT() {
    if (EvaEnvironment::universalTime) return EvaEnvironment::universalTime();
    return Planetarium::GetUniversalTime();
}

void KerbalEVA::LockInput(uint64_t mask, const std::string& id) {
    EvaEnvironment::controlLockLog.push_back("set:" + id);
    InputLockManager::SetControlLock(static_cast<ControlTypes>(mask), id);
}
void KerbalEVA::UnlockInput(const std::string& id) {
    EvaEnvironment::controlLockLog.push_back("remove:" + id);
    InputLockManager::RemoveControlLock(id);
}

// ---- WeldFX (state holder)
void EvaWeldFx::EnableMesh(float delay) {
    meshEnabled = true;
    log.push_back("EnableMesh:" + std::to_string(delay));
}
void EvaWeldFx::DisableMesh(float delay) {
    meshEnabled = false;
    log.push_back("DisableMesh:" + std::to_string(delay));
}
void EvaWeldFx::Play() {
    playing = true;
    log.push_back("Play");
}
void EvaWeldFx::Stop() {
    playing = false;
    log.push_back("Stop");
}

// KerbalEVA.cs:1973
void KerbalEVA::idle_OnEnter(KFSMState*) {
    A_CF(ANIM.idle, 0.2f);
    tgtSpeed = 0.0f;
    ModifyBodyColliderHeight(0.0f);
    CalculateGroundLevelAngle();
    SetPortraitCameraDistance(standardCameraDistance);
    Events()["PlantFlag"]->active = CanPlantFlag();
    if (evaChute != nullptr) evaChute->AllowRepack(true);
    if (!JetpackDeployed() && !PartPlacementMode()) {
        newidleCounter = newidleTime;
        startTimer = true;
    }
}

// KerbalEVA.cs:1992
void KerbalEVA::idle_OnLeave(KFSMState*) {
    newidleCounter = newidleTime;
    startTimer = false;
    lastTgtSpeed = 0.0f;
    Events()["PlantFlag"]->active = false;
}

Enumerator KerbalEVA::ReturnToIdle(float time) {
    co_yield YieldInstruction::WaitForSeconds(time);
    fsm->RunEvent(On_return_idle);
}

// KerbalEVA.cs:2000
void KerbalEVA::idle_b_OnEnter(KFSMState*) {
    idleAnimationsIndex = 0;
    idleAnimationsIndex = UnityRandom::Global().Range(0, static_cast<int>(ANIM.RandomIdleAnims.size()));
    if (!JetpackDeployed() && !alternateIdleDisabled) {
        if ((ANIM.RandomIdleAnims[static_cast<size_t>(idleAnimationsIndex)]->State()->name == "idle_c" ||
             ANIM.RandomIdleAnims[static_cast<size_t>(idleAnimationsIndex)]->State()->name == "idle_c_02") &&
            !isHelmetEnabled()) {
            idleAnimationsIndex++;
        }
        KerbalAnimationState& idleAnim = *ANIM.RandomIdleAnims.at(static_cast<size_t>(idleAnimationsIndex));
        A_CFM(idleAnim, 0.0f, PlayMode::StopAll);
        if (hasExpressionSystem) {
            EvaEnvironment::Effect("expression:idle startExpressionTime=" + std::to_string(idleAnim.startExpressionTime) +
                                   " wheeLevel=" + std::to_string(idleAnim.wheeLevel) + " fearFactor=" + std::to_string(idleAnim.fearFactor));
        }
        tgtSpeed = 0.0f;
        Events()["PlantFlag"]->active = CanPlantFlag();
        if (evaChute != nullptr) evaChute->AllowRepack(true);
        float num = 0.0f;
        num = f32store(fx(idleAnim.State()->length) - fx(idleAnim.CutAnimationTime));
        StopCoroutine(returnIDLE_);
        returnIDLE_ = StartCoroutine(ReturnToIdle(num));
        VesselPtr()->UpdateLandedSplashed();
    } else {
        if (!hasExpressionSystem) throw Nre();
        EvaEnvironment::Effect("expression:useNewIdleExpressions=false");
        fsm->RunEvent(On_return_idle);
    }
}

// KerbalEVA.cs:2043
void KerbalEVA::idle_b_OnLeave(KFSMState*) {
    if (returnIDLE_) StopCoroutine(returnIDLE_);
    if (startExpression_) StopCoroutine(startExpression_);
    if (hasExpressionSystem) EvaEnvironment::Effect("expression:useNewIdleExpressions=false");
    lastTgtSpeed = 0.0f;
    Events()["PlantFlag"]->active = false;
}

// KerbalEVA.cs:2061
void KerbalEVA::walk_Acd_OnEnter(KFSMState*) {
    A_CF(ANIM.walkFwd, 0.2f);
    A_BL(ANIM.walkLowGee, Mathf::InverseLerp(1.0f, minWalkingGee(), static_cast<float>(VesselPtr()->mainBody()->GeeASL)), 0.3f);
    ANIM.walkLowGee.State()->speed = 2.7f;
    tgtSpeed = walkSpeed();
}

// KerbalEVA.cs:2069
void KerbalEVA::walk_ccd_OnLeave(KFSMState*) { lastTgtSpeed = walkSpeed(); }

namespace {
// the FPS blend weights shared by walk_fps_OnUpdate / run_fps_OnUpdate / bound_gr_fps_OnUpdate
struct FpsAxes {
    float fwd, back, right, left;
};
}  // namespace

// KerbalEVA.cs:2074
void KerbalEVA::walk_fps_OnUpdate() {
    float num = Vector3f::Dot(tgtRpos, transform().forward());
    float num2 = Mathf::Clamp01(num);
    float num3 = Mathf::Clamp01(f32store(0.0 - fx(num)));
    float num4 = Vector3f::Dot(tgtRpos, transform().right());
    float num5 = Mathf::Clamp01(num4);
    float num6 = Mathf::Clamp01(f32store(0.0 - fx(num4)));
    tgtSpeed = f32store(fx(walkSpeed()) * (fx(num2) + fx(num3)) + fx(strafeSpeed()) * (fx(num6) + fx(num5)));
    if (fx(num2) > fx(0.01f)) {
        A_CF(ANIM.walkFwd, 0.2f);
        A_BL(ANIM.walkLowGee, Mathf::InverseLerp(1.0f, minWalkingGee(), static_cast<float>(VesselPtr()->mainBody()->GeeASL)), 0.3f);
        ANIM.walkLowGee.State()->speed = 2.7f;
        if (num5 > 0.0f) A_BL(ANIM.strafeRight, num5, 0.3f);
        else if (num6 > 0.0f) A_BL(ANIM.strafeLeft, num6, 0.3f);
    } else if (fx(num3) > fx(0.01f)) {
        A_CF(ANIM.walkBack, 0.2f);
        if (num5 > 0.0f) A_BL(ANIM.strafeRight, num5, 0.3f);
        else if (num6 > 0.0f) A_BL(ANIM.strafeLeft, num6, 0.3f);
    } else if (fx(num5) > fx(0.01f)) {
        A_CF(ANIM.strafeRight, 0.2f);
    } else if (fx(num6) > fx(0.01f)) {
        A_CF(ANIM.strafeLeft, 0.2f);
    }
}

// KerbalEVA.cs:2119
void KerbalEVA::walk_fps_OnLeave(KFSMState*) { lastTgtSpeed = tgtSpeed; }

// KerbalEVA.cs:2124
void KerbalEVA::heading_acquire_OnEnter(KFSMState*) {
    A_CF((deltaHdg > 0.0f) ? ANIM.turnRight : ANIM.turnLeft, 0.3f);
}

// KerbalEVA.cs:2129
void KerbalEVA::heading_acquire_OnLeave(KFSMState*) { lastTgtSpeed = static_cast<float>(VesselPtr()->horizontalSrfSpeed); }

// KerbalEVA.cs:2134
void KerbalEVA::run_acd_OnEnter(KFSMState*) {
    Engine().AnimCrossFade(ANIM.run.GetAnimationString(), 0.3f, PlayMode::StopSameLayer);
    A_BL(ANIM.walkLowGee, Mathf::InverseLerp(1.0f, minWalkingGee(), static_cast<float>(VesselPtr()->mainBody()->GeeASL)), 0.3f);
    ANIM.walkLowGee.State()->speed = 2.7f;
    SetPortraitCameraDistance(runningCameraDistance);
    tgtSpeed = runSpeed();
}

// KerbalEVA.cs:2143
void KerbalEVA::run_acd_OnLeave(KFSMState*) { lastTgtSpeed = runSpeed(); }

// KerbalEVA.cs:2148
void KerbalEVA::run_fps_OnUpdate() {
    float num = Vector3f::Dot(tgtRpos, transform().forward());
    float num2 = Mathf::Clamp01(num);
    float num3 = Mathf::Clamp01(f32store(0.0 - fx(num)));
    float num4 = Vector3f::Dot(tgtRpos, transform().right());
    float num5 = Mathf::Clamp01(num4);
    float num6 = Mathf::Clamp01(f32store(0.0 - fx(num4)));
    tgtSpeed = f32store(fx(runSpeed()) * fx(num2) + fx(walkSpeed()) * fx(num3) + fx(strafeSpeed()) * (fx(num6) + fx(num5)));
    if (fx(num2) > fx(0.01f)) {
        A_CF(ANIM.run, 0.2f);
        A_BL(ANIM.walkLowGee, Mathf::InverseLerp(1.0f, minWalkingGee(), static_cast<float>(VesselPtr()->mainBody()->GeeASL)), 0.3f);
        ANIM.walkLowGee.State()->speed = 2.7f;
        if (num5 > 0.0f) A_BL(ANIM.strafeRight, num5, 0.3f);
        else if (num6 > 0.0f) A_BL(ANIM.strafeLeft, num6, 0.3f);
    } else if (fx(num3) > fx(0.01f)) {
        A_CF(ANIM.walkBack, 0.2f);
        if (num5 > 0.0f) A_BL(ANIM.strafeRight, num5, 0.3f);
        else if (num6 > 0.0f) A_BL(ANIM.strafeLeft, num6, 0.3f);
    } else if (fx(num5) > fx(0.01f)) {
        A_CF(ANIM.strafeRight, 0.2f);
    } else if (fx(num6) > fx(0.01f)) {
        A_CF(ANIM.strafeLeft, 0.2f);
    }
}

// KerbalEVA.cs:2193
void KerbalEVA::run_fps_OnLeave(KFSMState*) { lastTgtSpeed = runSpeed(); }

// KerbalEVA.cs:2198
void KerbalEVA::bound_gr_acd_OnEnter(KFSMState* st) {
    if (st != st_bound_fl) {
        boundColliderModifierCounter = 0.0f;
        tmpBoundState = Engine().AnimCrossFadeQueued(ANIM.walkLowGee.GetAnimationString(), boundAttack(), QueueMode::PlayNow, PlayMode::StopSameLayer);
    }
    tgtSpeed = boundSpeed();
    CalculateGroundLevelAngle();
    set_lastBoundStep(Mathf::Clamp(lastBoundStep(), boundFrequency(), 100.0f));
    boundStepSpeed = Mathf::Clamp(f32store(fx(ANIM.walkLowGee.State()->length) * fx(0.5f) / (fx(lastBoundStep()) + fx(boundFrequency()))), 0.0f, 1.0f);
    if (tmpBoundState != nullptr) tmpBoundState->speed = boundStepSpeed;
    tgtBoundStep = boundLeftFoot ? ANIM.walkLowGee.end : ANIM.walkLowGee.start;
}

// KerbalEVA.cs:2216
void KerbalEVA::bound_gr_acd_OnUpdate() { boundColliderModifierCounter = f32store(fx(boundColliderModifierCounter) + fx(Time::deltaTime)); }

// KerbalEVA.cs:2221
void KerbalEVA::bound_gr_acd_OnLeave(KFSMState*) { lastTgtSpeed = boundSpeed(); }

// KerbalEVA.cs:2226
void KerbalEVA::bound_gr_fps_OnEnter(KFSMState*) {
    set_boundForce(f32store(fx(Mathf::Round(f32store(fx(PartPtr()->fields.mass) * fx(boundForceMassFactor) * fx(100.0f)))) / fx(100.0f)));
    tgtBoundStep = boundLeftFoot ? ANIM.walkLowGee.end : ANIM.walkLowGee.start;
    boundLeftFoot = !boundLeftFoot;
}

// KerbalEVA.cs:2233
void KerbalEVA::bound_gr_fps_OnUpdate() {
    float num = Vector3f::Dot(tgtRpos, transform().forward());
    float num2 = Mathf::Clamp01(num);
    float num3 = Mathf::Clamp01(f32store(0.0 - fx(num)));
    float num4 = Vector3f::Dot(tgtRpos, transform().right());
    float num5 = Mathf::Clamp01(num4);
    float num6 = Mathf::Clamp01(f32store(0.0 - fx(num4)));
    tgtSpeed = f32store(fx(boundSpeed()) * (fx(num2) + fx(num3)) + fx(strafeSpeed()) * (fx(num6) + fx(num5)));
    EvaAnimState* animationState = nullptr;
    EvaAnimState* animationState2 = nullptr;
    if (fx(num2) > fx(0.01f)) {
        animationState = ANIM.walkLowGee.State();
        if (num5 > 0.0f) animationState2 = ANIM.strafeRight.State();
        else if (num6 > 0.0f) animationState2 = ANIM.strafeLeft.State();
    } else if (fx(num3) > fx(0.01f)) {
        animationState = ANIM.walkBack.State();
        if (num5 > 0.0f) animationState2 = ANIM.strafeRight.State();
        else if (num6 > 0.0f) animationState2 = ANIM.strafeLeft.State();
    } else if (fx(num5) > fx(0.01f)) {
        animationState = ANIM.strafeRight.State();
    } else if (fx(num6) > fx(0.01f)) {
        animationState = ANIM.strafeLeft.State();
    }
    if (animationState != nullptr) {
        Engine().AnimCrossFade(animationState->name, f32store(fx(boundAttack()) * fx(0.5f)), PlayMode::StopSameLayer);
        boundStepSpeed = Mathf::Clamp(f32store(fx(animationState->length) * fx(0.5f) / fx(lastBoundStep())), 0.0f, 1.0f);
        animationState->speed = boundStepSpeed;
        if (animationState2 != nullptr) {
            Engine().AnimBlend(animationState2->name, f32store(fx(boundAttack()) * fx(0.5f)), 0.3f);  // Blend(name, targetWeight) -- the second argument is the WEIGHT
            animationState2->normalizedSpeed = animationState->normalizedSpeed;
            animationState2->normalizedTime = animationState->normalizedTime;
        }
    }
}

// KerbalEVA.cs:2290
void KerbalEVA::bound_gr_fps_OnLeave(KFSMState* st) {
    if (st != st_bound_fl) boundLeftFoot = false;
    lastTgtSpeed = boundSpeed();
}

// KerbalEVA.cs:2299
void KerbalEVA::bound_fl_OnEnter(KFSMState* st) {
    if (st == st_bound_gr_fps) {
        A_CF(boundLeftFoot ? ANIM.walkLowGeeSuspendedLeft : ANIM.walkLowGeeSuspendedRight, f32store(fx(lastBoundStep()) * fx(boundRelease())));
    }
}

// KerbalEVA.cs:2307
void KerbalEVA::bound_fl_OnLeave(KFSMState*) {
    lastTgtSpeed = static_cast<float>(VesselPtr()->horizontalSrfSpeed);
    set_lastBoundStep(static_cast<float>(fsm->TimeAtCurrentState()));
}

// KerbalEVA.cs:2313
void KerbalEVA::ragdoll_OnEnter(KFSMState*) {
    if (!VesselPtr()->packed) SetRagdoll(true);
    isRagdoll = true;
    lastCollisionTime = 0.0;
    if (InConstructionMode && EVAConstructionModeController::Instance != nullptr) {
        EVAConstructionModeController::Instance->ClosePanel();
        if (weldFX == nullptr) throw Nre();
        weldFX->Stop();
        alternateIdleDisabled = false;
        ToggleWeldingGun(false);
        UnlockInput("WeldLock_" + VesselPtr()->id.ToString());
    }
    SetPortraitCameraDistance(ragdollCameraDistance);
    currentLadder = nullptr;
    if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = false;
    set_currentLadderPart(nullptr);
    VesselPtr()->UpdateLandedSplashed();
    secondaryLadder = nullptr;
    currentLadderTriggers.clear();
    if (evaChute != nullptr) evaChute->AllowRepack(false);
}

// KerbalEVA.cs:2345
void KerbalEVA::ragdoll_OnLeave(KFSMState*) {
    SetRagdoll(false);
    canRecover = false;
    SetPortraitCameraDistance(standardCameraDistance);
    if (evaChute != nullptr && VesselPtr()->LandedOrSplashed()) evaChute->AllowRepack(true);
}

// KerbalEVA.cs:2356
void KerbalEVA::recover_OnEnter(KFSMState*) {
    if (isRagdoll) EvaEnvironment::Effect("advRagdoll.SynchRagdollOut");
    Vessel* vessel = VesselPtr();
    auto queued = [&](KerbalAnimationState& a) { Engine().AnimCrossFadeQueued(a.GetAnimationString(), 0.5f, QueueMode::PlayNow, PlayMode::StopSameLayer); };
    auto setDur = [&](KerbalAnimationState& a) { On_recover_complete->TimerDuration = f32store(fx(a.State()->length) - fx(0.6f)); };
    if (fx(Vector3f::Dot(transform().forward(), fUp)) > 0.0) {
        if (vessel->Splashed && !SurfaceContact()) {
            queued(ANIM.swimUpFaceUp);
            setDur(ANIM.swimUpFaceUp);
        } else if (SurfaceContact()) {
            queued(ANIM.standUpFaceUp);
            setDur(ANIM.standUpFaceUp);
        } else {
            queued(ANIM.suspendedIdle);
            setDur(ANIM.suspendedIdle);
        }
    } else if (vessel->Splashed && !SurfaceContact()) {
        queued(ANIM.swimUpFaceDown);
        setDur(ANIM.swimUpFaceDown);
    } else if (SurfaceContact()) {
        queued(ANIM.standUpFaceDown);
        setDur(ANIM.standUpFaceDown);
    } else {
        queued(ANIM.suspendedIdle);
        setDur(ANIM.suspendedIdle);
    }
    if (SurfaceOrSplashed()) StartGroundedRotationRecover();
}

// KerbalEVA.cs:2401
void KerbalEVA::recover_OnUpdate() {
    if (SurfaceOrSplashed()) {
        RecoverGroundedRotation(0.5f);
        Engine().SetRbVelocity(Vector3f::zero());
    }
}

// KerbalEVA.cs:2410
void KerbalEVA::controlPanel_identified_OnEnter(KFSMState*) {
    if (EvaEnvironment::IsInstalled("Serenity")) {
        tgtSpeed = 0.0f;
        SetPortraitCameraDistance(standardCameraDistance);
        KerbalAnimationState& a = *ANIM.controlPanelAnims.at(static_cast<size_t>(ANIM.controlPanelAnimSelector));
        a.State()->time = a.start;
        A_CFM(a, 0.2f, PlayMode::StopAll);
        sciencePanelAnimCooldown = a.State()->length;
        LockInput(kAllButUiCamera, "ControlPanelLock_" + VesselPtr()->id.ToString());
        sciencePanelAnimPlaying = true;
        if (sciencePartPresent) EvaEnvironment::Effect("UIPartActionController.SpawnPartActionWindow");
    }
}

// KerbalEVA.cs:2428
void KerbalEVA::picking_roc_sample_OnEnter(KFSMState*) {
    if (EvaEnvironment::IsInstalled("Serenity")) {
        ANIM.rockSample.State()->time = ANIM.rockSample.start;
        A_CFM(ANIM.rockSample, 0.2f, PlayMode::StopAll);
        pickRocSampleAnimCooldown = ANIM.rockSample.State()->length;
        LockInput(kAllButUiCamera, "ControlPanelLock_" + VesselPtr()->id.ToString());
        pickRocSampleAnimPlaying = true;
    }
}

// KerbalEVA.cs:2440 (the hammer prefab and its renderer / animation are the shell's: hammerPresent + the timings)
void KerbalEVA::spawnHammer() {
    if (!EvaEnvironment::IsInstalled("Serenity") || !hammerPrefabPresent) return;
    if (!hammerPresent) {
        hammerPresent = true;
        EvaEnvironment::Effect("hammer:instantiate");
    }
    hammerAnimTimer = f32store(fx(hammerAnimTimer) + fx(Time::deltaTime));
    if (!(fx(hammerAnimTimer) < fx(hammerClipLength) * fx(0.04f)) && fx(hammerAnimTimer) <= fx(hammerClipLength) * fx(0.55f)) hammerMeshEnabled = true;
    else hammerMeshEnabled = false;
    if (!hammerAnimPlaying) {
        hammerAnimPlaying = true;
        EvaEnvironment::Effect("hammer:play");
    }
}

// KerbalEVA.cs:2469
void KerbalEVA::jump_OnEnter(KFSMState*) {
    if (VesselPtr()->horizontalSrfSpeed < 0.20000000298023224) {
        On_jump_complete->TimerDuration = 0.20000000298023224;
        ANIM.JumpStillStart.State()->time = -0.2f;
        A_CFM(ANIM.JumpStillStart, 0.2f, PlayMode::StopAll);
    } else {
        On_jump_complete->TimerDuration = ANIM.JumpFwdStart.end;
        ANIM.JumpFwdStart.State()->time = ANIM.JumpFwdStart.start;
        A_CFM(ANIM.JumpFwdStart, 0.2f, PlayMode::StopAll);
    }
}

// KerbalEVA.cs:2485
void KerbalEVA::idle_fl_OnEnter(KFSMState*) {
    A_CF(ANIM.suspendedIdle, 1.2f);
    tgtSpeed = 0.0f;
    currentSpd = 0.0f;
    ResetOrientationPID();
    if (evaChute != nullptr) evaChute->AllowRepack(false);
}

// KerbalEVA.cs:2497
void KerbalEVA::idle_fl_OnLeave(KFSMState*) {
    if (evaChute != nullptr) evaChute->AllowRepack(true);
}

// KerbalEVA.cs:2505
void KerbalEVA::land_OnEnter(KFSMState*) {
    lastTgtSpeed = static_cast<float>(VesselPtr()->horizontalSrfSpeed);
    StartGroundedRotationRecover();
    if (VesselPtr()->horizontalSrfSpeed < 0.1) {
        ANIM.JumpStillEnd.State()->time = 0.1f;
        A_CF(ANIM.JumpStillEnd, 0.2f);
    } else {
        ANIM.JumpFwdEnd.State()->time = 0.3f;
        A_CF(ANIM.JumpFwdEnd, 0.2f);
    }
}

// KerbalEVA.cs:2521
void KerbalEVA::land_OnUpdate() { RecoverGroundedRotation(0.2f); }

// KerbalEVA.cs:2526
void KerbalEVA::swim_idle_OnEnter(KFSMState*) {
    ToggleJetpack(false);
    SetPortraitCameraDistance(swimmingCameraDistance);
    A_CF(ANIM.swimIdle, 0.5f);
    tgtSpeed = 0.0f;
    if (evaChute != nullptr) evaChute->AllowRepack(true);
    if (partPlacementMode && moduleInventoryPartReference != nullptr) moduleInventoryPartReference->CancelPartPlacementMode();
}

// KerbalEVA.cs:2542
void KerbalEVA::swim_idle_OnLeave(KFSMState*) { lastTgtSpeed = 0.0f; }

// KerbalEVA.cs:2547
void KerbalEVA::swim_fwd_OnEnter(KFSMState*) {
    Engine().AnimCrossFade(ANIM.swimFwd.GetAnimationString(), 0.3f, PlayMode::StopSameLayer);
    tgtSpeed = swimSpeed();
}

// KerbalEVA.cs:2553
void KerbalEVA::swim_fwd_OnLeave(KFSMState*) { lastTgtSpeed = swimSpeed(); }

// KerbalEVA.cs:2558
void KerbalEVA::ladder_acquire_OnEnter(KFSMState*) {
    ToggleJetpack(false);
    currentLadder = currentLadderTriggers.at(0);
    if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = false;
    set_currentLadderPart(currentLadder->part);
    if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = true;
    invLadderAxis = fx(Vector3f::Dot(currentLadder->pose.up(), transform().up())) < 0.0;
    lastTgtSpeed = 0.0f;
    // StartCoroutine(AcquireRotation(Quaternion.LookRotation(currentLadder.transform.forward, currentLadder.transform.up * (invLadderAxis ? -1f : 1f)), 0.5f))
    StartCoroutine(AcquireRotation(UnityNative::LookRotation(currentLadder->pose.forward(), currentLadder->pose.up() * (invLadderAxis ? -1.0f : 1.0f)), 0.5f));
    if (SurfaceOrSplashed()) A_CFM(ANIM.ladderGrabGrounded, 0.2f, PlayMode::StopAll);
    else A_CFM(ANIM.ladderGrabSuspended, 0.2f, PlayMode::StopAll);
    if (partPlacementMode && moduleInventoryPartReference != nullptr) moduleInventoryPartReference->CancelPartPlacementMode();
    if (InConstructionMode) ToggleWeldingGun(false);
}

// KerbalEVA.cs:2592
void KerbalEVA::ladder_acquire_OnLeave(KFSMState*) {
    A_BL(ANIM.ladderGrabGrounded, 0.0f, 2.0f);
    A_BL(ANIM.ladderGrabSuspended, 0.0f, 2.0f);
    VesselPtr()->gravityMultiplier = 1.0;
    onLadder = false;
}

// KerbalEVA.cs:2600
void KerbalEVA::ladder_idle_OnEnter(KFSMState*) {
    A_CFM(ANIM.ladderIdle, 0.2f, PlayMode::StopAll);
    tgtSpeed = 0.0f;
    ladderTransition = false;
    onLadder = true;
    SetLadderHold();
}

// KerbalEVA.cs:2609
void KerbalEVA::ladder_idle_OnLeave(KFSMState*) {
    lastTgtSpeed = 0.0f;
    A_BL(ANIM.ladderIdle, 0.0f, 2.0f);
    VesselPtr()->gravityMultiplier = 1.0;
    onLadder = false;
    ClearLadderHold();
}

// KerbalEVA.cs:2618
void KerbalEVA::ladder_lean_OnEnter(KFSMState*) {
    A_PLAY(ANIM.ladderIdle, PlayMode::StopAll);
    ANIM.ladderLeanFwd.State()->enabled = true;
    ANIM.ladderLeanBack.State()->enabled = true;
    ANIM.ladderLeanLeft.State()->enabled = true;
    ANIM.ladderLeanRight.State()->enabled = true;
    ANIM.ladderPushOff.State()->enabled = true;
    tgtSpeed = 0.0f;
    ladderTransition = false;
    onLadder = true;
}

// KerbalEVA.cs:2631
void KerbalEVA::ladder_lean_OnLateUpdate() {
    float num = Vector3f::Dot(ladderTgtRPos, transform().up());
    A_BL(ANIM.ladderLeanFwd, Mathf::Clamp01(num), 0.3f);
    ANIM.ladderLeanFwd.State()->normalizedTime = Mathf::Lerp(ANIM.ladderLeanFwd.start, ANIM.ladderLeanFwd.end, ANIM.ladderLeanFwd.State()->weight);
    A_BL(ANIM.ladderLeanBack, Mathf::Clamp01(f32store(0.0 - fx(num))), 0.3f);
    ANIM.ladderLeanBack.State()->normalizedTime = Mathf::Lerp(ANIM.ladderLeanBack.start, ANIM.ladderLeanBack.end, ANIM.ladderLeanBack.State()->weight);
    float num2 = Vector3f::Dot(ladderTgtRPos, -transform().right());
    A_BL(ANIM.ladderLeanLeft, Mathf::Clamp01(num2), 0.3f);
    ANIM.ladderLeanLeft.State()->normalizedTime = Mathf::Lerp(ANIM.ladderLeanLeft.start, ANIM.ladderLeanLeft.end, ANIM.ladderLeanLeft.State()->weight);
    A_BL(ANIM.ladderLeanRight, Mathf::Clamp01(f32store(0.0 - fx(num2))), 0.3f);
    ANIM.ladderLeanRight.State()->normalizedTime = Mathf::Lerp(ANIM.ladderLeanRight.start, ANIM.ladderLeanRight.end, ANIM.ladderLeanRight.State()->weight);
    A_BL(ANIM.ladderPushOff, f32store(1.0 - fx(Mathf::Clamp01(f32store(fx(Mathf::Abs(num2)) + fx(Mathf::Abs(num)))))), 0.3f);
    ANIM.ladderPushOff.State()->normalizedTime = Mathf::Lerp(ANIM.ladderPushOff.start, ANIM.ladderPushOff.end, ANIM.ladderPushOff.State()->weight);
}

// KerbalEVA.cs:2647
void KerbalEVA::ladder_lean_OnLeave(KFSMState* st) {
    lastTgtSpeed = 0.0f;
    if (st != st_ladder_pushoff) {
        A_BL(ANIM.ladderLeanFwd, 0.0f, 2.0f);
        A_BL(ANIM.ladderLeanBack, 0.0f, 2.0f);
        A_BL(ANIM.ladderLeanLeft, 0.0f, 2.0f);
        A_BL(ANIM.ladderLeanRight, 0.0f, 2.0f);
        A_BL(ANIM.ladderPushOff, 0.0f, 2.0f);
    }
    VesselPtr()->gravityMultiplier = 1.0;
    onLadder = false;
}

// KerbalEVA.cs:2662
void KerbalEVA::ladder_climb_OnEnter(KFSMState*) {
    A_CFM(ANIM.ladderClimb, 0.2f, PlayMode::StopAll);
    tgtSpeed = ladderClimbSpeed();
    onLadder = true;
}

// KerbalEVA.cs:2669
void KerbalEVA::ladder_climb_OnLeave(KFSMState*) {
    lastTgtSpeed = ladderClimbSpeed();
    VesselPtr()->gravityMultiplier = 1.0;
    onLadder = false;
}

// KerbalEVA.cs:2676
void KerbalEVA::ladder_descend_OnEnter(KFSMState*) {
    A_CFM(ANIM.ladderDescend, 0.2f, PlayMode::StopAll);
    tgtSpeed = f32store(0.0 - fx(ladderClimbSpeed()));
    onLadder = true;
}

// KerbalEVA.cs:2683
void KerbalEVA::ladder_descend_OnLeave(KFSMState*) {
    lastTgtSpeed = f32store(0.0 - fx(ladderClimbSpeed()));
    VesselPtr()->gravityMultiplier = 1.0;
    onLadder = false;
}

// KerbalEVA.cs:2690
void KerbalEVA::ladder_end_reached_OnEnter(KFSMState*) {
    tgtSpeed = 0.0f;
    A_CFM(ANIM.ladderIdle, 1.0f, PlayMode::StopAll);
    onLadder = true;
    SetLadderHold();
}

// KerbalEVA.cs:2698
void KerbalEVA::ladder_end_reached_OnLeave(KFSMState*) {
    A_BL(ANIM.ladderIdle, 0.0f, 1.0f);
    VesselPtr()->gravityMultiplier = 1.0;
    lastTgtSpeed = 0.0f;
    onLadder = false;
    ClearLadderHold();
}

// KerbalEVA.cs:2707
void KerbalEVA::ladder_pushoff_OnLeave(KFSMState*) {
    A_BL(ANIM.ladderLeanFwd, 0.0f, 2.0f);
    A_BL(ANIM.ladderLeanBack, 0.0f, 2.0f);
    A_BL(ANIM.ladderLeanLeft, 0.0f, 2.0f);
    A_BL(ANIM.ladderLeanRight, 0.0f, 2.0f);
    A_BL(ANIM.ladderPushOff, 0.0f, 2.0f);
    VesselPtr()->gravityMultiplier = 1.0;
    onLadder = false;
}

// KerbalEVA.cs:2718
void KerbalEVA::clamber_acquireP1_OnEnter(KFSMState*) {
    currentLadder = nullptr;
    ToggleJetpack(false);
    lastTgtSpeed = 0.0f;
    A_CF(ANIM.clamber, 0.05f);
    ANIM.clamber.State()->normalizedTime = ANIM.clamber.start;
    ANIM.clamber.State()->speed = ANIM.clamber.speedAt1Gee;
    if (!clamberPath) throw Nre();
    const float dur = f32store(fx(ANIM.clamber.State()->length) * fx(0.2f) * fx(ANIM.clamber.speedAt1Gee));
    StartCoroutine(AcquireRotation(UnityNative::LookRotation(-clamberPath->edgeFaceNormal, fUp), dur));
    StartCoroutine(AcquirePosition(clamberPath->p1, dur));
    if (partPlacementMode && moduleInventoryPartReference != nullptr) moduleInventoryPartReference->CancelPartPlacementMode();
}

// KerbalEVA.cs:2734
void KerbalEVA::ZeroRBVelocity() { Engine().SetRbVelocity(Vector3f::zero()); }

// KerbalEVA.cs:2739
void KerbalEVA::clamber_acquireP1_OnLeave(KFSMState* st) {
    if (st != st_clamber_acquireP2) clamberPath.reset();
}

// KerbalEVA.cs:2747
void KerbalEVA::clamber_acquireP2_OnEnter(KFSMState*) {
    if (!clamberPath) throw Nre();
    const float dur = f32store(fx(ANIM.clamber.State()->length) * fx(0.1f) * fx(ANIM.clamber.speedAt1Gee));
    StartCoroutine(AcquireRotation(UnityNative::LookRotation(-clamberPath->edgeFaceNormal, fUp), dur));
    StartCoroutine(AcquirePosition(clamberPath->p2, dur));
}

// KerbalEVA.cs:2753
void KerbalEVA::clamber_acquireP2_OnLeave(KFSMState* st) {
    if (st != st_clamber_acquireP3) clamberPath.reset();
}

// KerbalEVA.cs:2761
void KerbalEVA::clamber_acquireP3_OnEnter(KFSMState*) {
    if (!clamberPath) throw Nre();
    const float dur = f32store(fx(ANIM.clamber.State()->length) * fx(0.2f) * fx(ANIM.clamber.speedAt1Gee));
    StartCoroutine(AcquireRotation(UnityNative::LookRotation(-clamberPath->edgeFaceNormal, fUp), dur));
    StartCoroutine(AcquirePosition(clamberPath->p3, dur));
}

// KerbalEVA.cs:2767
void KerbalEVA::clamber_acquireP3_OnLeave(KFSMState*) {
    A_BL(ANIM.clamber, 0.0f, f32store(1.0 - fx(ANIM.clamber.end)));
    clamberPath.reset();
}

// KerbalEVA.cs:2773
void KerbalEVA::flagAcquireHeading_OnEnter(KFSMState*) {
    tgtSpeed = 0.0f;
    flagSpot = EvaEnvironment::flagScanSurroundingTerrain ? EvaEnvironment::flagScanSurroundingTerrain(transform().position, transform().forward(), flagReach())
                                                          : transform().position;
    ToggleJetpack(false);
    SetWaypoint(flagSpot);
    UpdateHeading();
    A_CF((deltaHdg > 0.0f) ? ANIM.turnRight : ANIM.turnLeft, 0.3f);
    if (partPlacementMode && moduleInventoryPartReference != nullptr) moduleInventoryPartReference->CancelPartPlacementMode();
}

// KerbalEVA.cs:2787
void KerbalEVA::flagAcquireHeading_OnLateUpdate() { SetWaypoint(flagSpot); }

// KerbalEVA.cs:2792
void KerbalEVA::flagAcquireHeading_OnLeave(KFSMState*) { lastTgtSpeed = 0.0f; }

// KerbalEVA.cs:2797
void KerbalEVA::flagPlant_OnEnter(KFSMState*) {
    deltaHdg = 0.0f;
    Engine().SetRbAngularVelocity(Vector3f::zero());
    A_CFM(ANIM.flagPlant, 0.2f, PlayMode::StopAll);
    // Physics.Raycast(transform.position + transform.forward * flagReach + fUp * 10f, -fUp, out hitInfo, 20f, 32768, Ignore); flagSpot = hitInfo.point
    EvaRaycastHit hit;
    Engine().Raycast(transform().position + transform().forward() * flagReach() + fUp * 10.0f, -fUp, 20.0f, 32768, hit);
    flagSpot = hit.point;
    LockInput(kAllButUiCamera, "FlagDeployLock_" + VesselPtr()->id.ToString());
    flag = EvaEnvironment::flagCreate
               ? EvaEnvironment::flagCreate(flagSpot, UnityNative::LookRotation(transform().forward(), transform().up()), *PartPtr())
               : nullptr;
    if (partPlacementMode && moduleInventoryPartReference != nullptr) moduleInventoryPartReference->CancelPartPlacementMode();
}

// KerbalEVA.cs:2812
void KerbalEVA::flagPlant_OnLeave(KFSMState*) {
    lastTgtSpeed = 0.0f;
    UnlockInput("FlagDeployLock_" + VesselPtr()->id.ToString());
    Vessel* vessel0 = VesselPtr();
    if (fsm->LastEvent() != On_flagPlantComplete) {
        if (flag != nullptr) flag->OnPlacementFail();
        else if (EvaEnvironment::flagCreate) throw Nre();
    } else {
        if (flag != nullptr) flag->OnPlacementComplete();
        const std::string& body = vessel0->orbitDriver->orbit->referenceBody->bodyName;
        Part* p = PartPtr();
        p->protoModuleCrew[0]->flightLog.AddEntryUnique(FlightLog::EntryType::PlantFlag, body);
        p->protoModuleCrew[0]->UpdateExperience();
        int count = static_cast<int>(FlightRegistry::VesselsLoaded.size());
        while (count-- > 0) {
            Vessel* vessel = FlightRegistry::VesselsLoaded[static_cast<size_t>(count)];
            if (!(vessel != nullptr) || !vessel->loaded || !(vessel != FlightRegistry::ActiveVessel)) continue;
            if (vessel->vesselType == VesselType::EVA) {
                auto pcm = vessel->GetVesselCrew()[0];
                pcm->flightLog.AddEntryUnique(FlightLog::EntryType::PlantFlag, body);
                pcm->UpdateExperience();
            } else if (vessel->situation == Vessel_Situations::LANDED || vessel->situation == Vessel_Situations::SPLASHED ||
                       vessel->situation == Vessel_Situations::PRELAUNCH) {
                const auto& vesselCrew = vessel->GetVesselCrew();
                int count2 = static_cast<int>(vesselCrew.size());
                while (count2-- > 0) {
                    auto pcm = vesselCrew[static_cast<size_t>(count2)];
                    pcm->flightLog.AddEntryUnique(FlightLog::EntryType::PlantFlag, body);
                    pcm->UpdateExperience();
                }
            }
        }
    }
    flag = nullptr;
}

// KerbalEVA.cs:2855
void KerbalEVA::seated_cmd_OnEnter(KFSMState*) {
    GE::onCommandSeatInteractionEnter.Fire(this, true);
    A_PLAY(ANIM.seatIdle, PlayMode::StopSameLayer);
    Events()["OnDeboardSeat"]->active = true;
    Events()["MakeReference"]->active = true;
    Events()["RenameVessel"]->active = true;
    PartPtr()->fields.isControlSource = static_cast<int32_t>(ControlLevel::FULL);
    if (fsm->LastEvent() != On_seatBoard) SetEjectDirection();
    GE::onCommandSeatInteraction.Fire(this, true);
    if (partPlacementMode && moduleInventoryPartReference != nullptr) moduleInventoryPartReference->CancelPartPlacementMode();
    if (EvaEnvironment::portraitGallery) EvaEnvironment::portraitGallery(*this, true);
}

// KerbalEVA.cs:2878
void KerbalEVA::seated_cmd_OnLeave(KFSMState* st) {
    GE::onCommandSeatInteractionEnter.Fire(this, false);
    Events()["OnDeboardSeat"]->active = false;
    Events()["MakeReference"]->active = false;
    Events()["RenameVessel"]->active = false;
    PartPtr()->fields.isControlSource = static_cast<int32_t>(ControlLevel::NONE);
    if (fsm->LastEvent() != On_seatDeboard) {
        if (VesselPtr()->rootPart != PartPtr()) EjectFromSeat();
        RestoreVesselInfo(0.1f);
        SwitchFocusIfActiveVesselUncontrollable(0.1f);
    }
    if (st == st_ragdoll && evaChute != nullptr &&
        (evaChute->deploymentState == ModuleParachute::deploymentStates::SEMIDEPLOYED || evaChute->deploymentState == ModuleParachute::deploymentStates::DEPLOYED))
        OnParachuteCut();
    if (EvaEnvironment::portraitGallery) EvaEnvironment::portraitGallery(*this, false);
    GE::onCommandSeatInteraction.Fire(this, false);
}

// KerbalEVA.cs:2906
void KerbalEVA::grappled_OnEnter(KFSMState* st) {
    if (st != st_ragdoll) {
        if (!VesselPtr()->packed) SetRagdoll(true);
        isRagdoll = true;
        lastCollisionTime = 0.0;
        if (InConstructionMode && EVAConstructionModeController::Instance != nullptr) {
            EVAConstructionModeController::Instance->ClosePanel();
            if (weldFX == nullptr) throw Nre();
            weldFX->Stop();
            alternateIdleDisabled = false;
            ToggleWeldingGun(false);
            UnlockInput("WeldLock_" + VesselPtr()->id.ToString());
        }
        currentLadder = nullptr;
        if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = false;
        set_currentLadderPart(nullptr);
        VesselPtr()->UpdateLandedSplashed();
        secondaryLadder = nullptr;
        currentLadderTriggers.clear();
        if (partPlacementMode && moduleInventoryPartReference != nullptr) moduleInventoryPartReference->CancelPartPlacementMode();
    }
}

// KerbalEVA.cs:2940
void KerbalEVA::grappled_OnLeave(KFSMState* st) {
    if (st != st_ragdoll) {
        SetRagdoll(false);
        canRecover = false;
    }
    updateRagdollVelocities();
    updateRagdollVelocities();
    RestoreVesselInfo(0.1f);
    SwitchFocusIfActiveVesselUncontrollable(0.1f);
}

// KerbalEVA.cs:2953
void KerbalEVA::EnterConstructionMode() {
    InConstructionMode = true;
    if (!OnALadder_()) fsm->RunEvent(On_constructionModeEnter);
}

// KerbalEVA.cs:2962
void KerbalEVA::ExitConstructionMode() {
    InConstructionMode = false;
    if (!OnALadder_()) {
        InterruptWeld();
        fsm->RunEvent(On_constructionModeExit);
        UnlockInput("WeldLock_" + VesselPtr()->id.ToString());
    }
}

// KerbalEVA.cs:2973
void KerbalEVA::enteringConstruction_OnEnter(KFSMState*) {
    alternateIdleDisabled = true;
    Events()["PlantFlag"]->active = CanPlantFlag();
    if (!weldToolPresent) {
        weldToolPresent = true;
        weldFX = std::make_unique<EvaWeldFx>();
    }
    weldFX->Active = true;
    ToggleWeldingGun(true);
}

// KerbalEVA.cs:2991
void KerbalEVA::exitingConstruction_OnEnter(KFSMState*) {
    alternateIdleDisabled = false;
    Events()["PlantFlag"]->active = CanPlantFlag();
    ToggleWeldingGun(false);
}

// KerbalEVA.cs:2998
void KerbalEVA::enteringExitingConstruction_OnFixedUpdate() {
    if (SurfaceContact()) {
        if ((fsm->CurrentState() == st_enteringConstruction && fsm->TimeAtCurrentState() >= static_cast<double>(ANIM.weldGunDraw.State()->length)) ||
            (fsm->CurrentState() == st_exitingConstruction && fsm->TimeAtCurrentState() >= static_cast<double>(ANIM.weldGunPutAway.State()->length))) {
            fsm->RunEvent(On_constructionModeTrigger_gr_Complete);
            return;
        }
        UpdateMovement();
        correctGroundedRotation();
        if (fsm->LastState() == st_idle_gr || fsm->LastState() == st_idle_b_gr) return;
        UpdateHeading();
        if (!JetpackDeployed()) A_BL(ANIM.weldGunLift, 1.0f, 0.1f);
    } else {
        if ((fsm->CurrentState() == st_enteringConstruction && fsm->TimeAtCurrentState() >= static_cast<double>(ANIM.weldGunDraw.State()->length)) ||
            (fsm->CurrentState() == st_exitingConstruction && fsm->TimeAtCurrentState() >= static_cast<double>(ANIM.weldGunPutAway.State()->length))) {
            fsm->RunEvent(On_constructionModeTrigger_fl_Complete);
            return;
        }
        UpdateOrientationPID();
    }
    updateRagdollVelocities();
}

// KerbalEVA.cs:3031
void KerbalEVA::exitingConstruction_OnLeave(KFSMState*) { A_STOP(ANIM.weldGunLift); }

// KerbalEVA.cs:3036
void KerbalEVA::ToggleWeldingGun(bool toggle) {
    if (toggle) {
        ANIM.weldGunDraw.State()->time = ANIM.weldGunDraw.start;
        A_CF(ANIM.weldGunDraw, 0.2f);
        if (SurfaceContact() && !JetpackDeployed()) A_BL(ANIM.weldGunLift, 1.0f, 0.1f);
        if (weldFX == nullptr) throw Nre();
        weldFX->EnableMesh(0.5f);
        weldFX->IsFloating = !SurfaceContact();
    } else {
        ANIM.weldGunPutAway.State()->time = ANIM.weldGunPutAway.start;
        A_CF(ANIM.weldGunPutAway, 0.2f);
        A_BL(ANIM.weldGunLift, 0.0f, 0.1f);
        if (weldFX != nullptr) weldFX->DisableMesh(f32store(fx(ANIM.weldGunPutAway.State()->length) - fx(1.0f)));
    }
}

// KerbalEVA.cs:3061
void KerbalEVA::weld_acquireHeading_OnEnter(KFSMState*) {
    ZeroRBVelocity();
    tgtRpos = Vector3f::zero();
    packTgtRPos = Vector3f::zero();
    deltaHdg = 0.0f;
    tgtSpeed = 0.0f;
    // ~(ControlTypes.EDITOR_LOCK | ControlTypes.UI)
    LockInput(~(static_cast<uint64_t>(ControlTypes::EDITOR_LOCK) | static_cast<uint64_t>(ControlTypes::UI)), "WeldLock_" + VesselPtr()->id.ToString());
    if (constructionTarget == nullptr) throw Nre();
    SetWaypoint(constructionTarget->partTransform.position);
    UpdateHeading();
    if (SurfaceContact()) {
        A_CF((deltaHdg > 0.0f) ? ANIM.turnRight : ANIM.turnLeft, 0.3f);
        return;
    }
    Engine().SetRbAngularVelocity(Vector3f::zero());
    if (!JetpackDeployed()) {
        jetpackDeployedForWelding = true;
        ToggleJetpack(true);
    } else {
        jetpackDeployedForWelding = false;
    }
}

// KerbalEVA.cs:3088
void KerbalEVA::weld_acquireHeading_OnFixedUpdate() {
    if (SurfaceContact()) correctGroundedRotation();
    else UpdateOrientationPID();
    updateRagdollVelocities();
}

// KerbalEVA.cs:3101
void KerbalEVA::weld_acquireHeading_OnLateUpdate() {
    if (SurfaceContact()) {
        SetWaypoint(constructionTarget->partTransform.position);
        UpdateHeading();
    } else {
        // base.transform.rotation = Quaternion.Slerp(rotation, LookRotation(target - position), Time.deltaTime * 1.5f): Slerp is native (the engine)
        Engine().SetTransformRotationSlerp(UnityNative::LookRotation(constructionTarget->partTransform.position - transform().position, Vector3f::up()),
                                           f32store(fx(Time::deltaTime) * fx(1.5f)));
    }
}

// KerbalEVA.cs:3114
void KerbalEVA::weld_acquireHeading_OnLeave(KFSMState*) { lastTgtSpeed = 0.0f; }

// KerbalEVA.cs:3119
void KerbalEVA::weld_OnEnter(KFSMState*) {
    ANIM.weld.State()->time = ANIM.weld.start;
    ANIM.weldSuspended.State()->time = ANIM.weldSuspended.start;
    if (HasWeldLineOfSight()) {
        if (SurfaceContact()) A_CF(ANIM.weld, 0.2f);
        else A_CF(ANIM.weldSuspended, 0.2f);
        wasVisorEnabledBeforeWelding = visorState == VisorStates::Lowered;
        LowerVisor(true);
        if (weldFX != nullptr) weldFX->Play();
    } else {
        fsm->RunEvent(On_weldComplete);
    }
}

// KerbalEVA.cs:3146
bool KerbalEVA::HasWeldLineOfSight() {
    if (constructionTarget == nullptr) throw Nre();
    Vector3f gunPoint = transform().position + transform().up() * f32store(fx(halfHeight) - fx(0.1f));
    // constructionTarget.gameObject.GetRendererBoundsWithoutParticles(): the part's renderer bounds
    Bounds b = PartGeometryUtil::GetRendererBoundsWithoutParticles(constructionTarget->modelRenderers);
    if (constructionTarget->parent != nullptr) {
        Bounds b2 = PartGeometryUtil::GetRendererBoundsWithoutParticles(constructionTarget->parent->modelRenderers);
        const Vector3f diff = b2.center() - b.center();
        EvaRaycastHit hit;
        if (Engine().Raycast(b.center(), diff.normalized(), diff.magnitude(), kDefaultEquivalent, hit)) {
            if (HasWeldLineOfSight(gunPoint, hit.point)) return true;
        }
    }
    Vector3f partAttachPoint = b.center() - constructionTarget->partTransform.forward() * constructionTargetPivotOffset;
    if (HasWeldLineOfSight(gunPoint, partAttachPoint)) return true;
    partAttachPoint = b.center() - constructionTarget->partTransform.right() * constructionTargetPivotOffset;
    if (HasWeldLineOfSight(gunPoint, partAttachPoint)) return true;
    partAttachPoint = b.center() - constructionTarget->partTransform.up() * constructionTargetPivotOffset;
    if (HasWeldLineOfSight(gunPoint, partAttachPoint)) return true;
    return false;
}

// KerbalEVA.cs:3180
bool KerbalEVA::HasWeldLineOfSight(const Vector3f& gunPoint, const Vector3f& partAttachPoint) {
    EvaRaycastHit hit;
    if (Engine().Raycast(gunPoint, (partAttachPoint - gunPoint).normalized(), f32store(fx(InventoryEnvironment::EVA_CONSTRUCTION_RANGE) * fx(1.2f)),
                         kDefaultEquivalent, hit) &&
        hit.collider != nullptr && hit.collider->part == constructionTarget)
        return true;
    return false;
}

// KerbalEVA.cs:3189
void KerbalEVA::InterruptWeld() {
    if (fsm->CurrentState() == st_weld) {
        if (weldFX == nullptr) throw Nre();
        weldFX->Stop();
        fsm->RunEvent(On_weldComplete);
        UnlockInput("WeldLock_" + VesselPtr()->id.ToString());
    }
}

// KerbalEVA.cs:3199
void KerbalEVA::OnWeldStart(KerbalEVA* kerbalEVA) {
    if (weldingLasersPresent && !weldingLasersPlaying) {
        weldingLasersPlaying = true;
        if (kerbalEVA->weldFX == nullptr) throw Nre();
        EvaEnvironment::Effect("weldingLasers:PlayDelayed " + std::to_string(kerbalEVA->weldFX->StartDelay));
    }
}

// KerbalEVA.cs:3207
void KerbalEVA::OnWeldFinish(KerbalEVA*) {
    if (weldingLasersPresent) {
        weldingLasersPlaying = false;
        EvaEnvironment::Effect("weldingLasers:Stop");
    }
}

// KerbalEVA.cs:3215
void KerbalEVA::weld_OnFixedUpdate() {
    if (SurfaceContact()) {
        correctGroundedRotation();
        ZeroRBVelocity();
        UpdateMovement();
        Engine().SetRbAngularVelocity(Vector3f::zero());
        float value = Vector3f::SignedAngle(constructionTarget->partTransform.position - transform().position, transform().forward(), transform().right());
        value = Mathf::Clamp(value, -20.0f, 45.0f);
        if (value > 0.0f) {
            value = f32store(fx(value) / fx(45.0f));
            A_BL(ANIM.weld_aim_up, value, 0.15f);
            A_BL(ANIM.weld_aim_down, 0.0f, 0.15f);
            A_BL(ANIM.weld, f32store(1.0 - fx(value)), 0.15f);
        } else {
            value = f32store(fx(value) / fx(20.0f));
            A_BL(ANIM.weld_aim_down, f32store(0.0 - fx(value)), 0.15f);
            A_BL(ANIM.weld_aim_up, 0.0f, 0.15f);
            A_BL(ANIM.weld, f32store(1.0 + fx(value)), 0.15f);
        }
    } else {
        UpdateOrientationPID();
        Engine().TransformLookAt(constructionTarget->partTransform.position);
    }
    updateRagdollVelocities();
}

// KerbalEVA.cs:3248
void KerbalEVA::weld_OnLeave(KFSMState*) {
    UnlockInput("WeldLock_" + VesselPtr()->id.ToString());
    if (SurfaceContact()) A_BL(ANIM.weldGunLift, 1.0f, 0.1f);
    A_STOP(ANIM.weld_aim_up);
    A_STOP(ANIM.weld_aim_down);
    A_STOP(ANIM.weld);
    if (!wasVisorEnabledBeforeWelding) RaiseVisor(true);
    if (jetpackDeployedForWelding) ToggleJetpack(false);
}

// KerbalEVA.cs:4662
bool KerbalEVA::IsKerbalInStateAbleToDeployParachute() {
    if (fsm->CurrentState() != st_idle_fl && fsm->CurrentState() != st_ragdoll && fsm->CurrentState() != st_semi_deployed_parachute &&
        fsm->CurrentState() != st_seated_cmd)
        return false;
    return true;
}

// KerbalEVA.cs:4671
void KerbalEVA::UpdateParachuteInCommandSeat() {
    Vessel* vessel = VesselPtr();
    if (evaChute != nullptr && evaChute->isEnabled && evaChute->deploymentState == ModuleParachute::deploymentStates::STOWED &&
        Key(EvaKey::ChuteDeploy, KeyQuery::Held) && VesselUnderControl()) {
        if (evaChute->IsMovingFastEnoughToDeploy()) evaChute->Deploy();
        else EvaEnvironment::postedMessages.push_back(Localizer::Format("#autoLOC_8004228"));
    }
    if (evaChute != nullptr && evaChute->deploymentState == ModuleParachute::deploymentStates::CUT)
        evaChute->AllowRepack(!evaChute->IsMovingFastEnoughToDeploy() && vessel->LandedOrSplashed());
}

// KerbalEVA.cs:4690
void KerbalEVA::OnParachuteSemiDeployed() { fsm->RunEvent(On_semi_deploy_parachute); }

// KerbalEVA.cs:4695
void KerbalEVA::OnSemiDeployedParachuteModeEntered(KFSMState*) {
    evaChute->Deploy();
    SetRagdoll(false);
    A_CF(ANIM.suspendedIdle, 1.2f);
}

// KerbalEVA.cs:4702
void KerbalEVA::UpdateSemiDeployedParachuteMovement() {
    // evaChute.UpdateSemiDeployedParachuteMovement(parachuteInput, rb): the canopy drag / rotation on the rigidbody (engine side; ModuleEvaChute's)
    EvaEnvironment::Effect("evaChute:UpdateSemiDeployedParachuteMovement");
}

// KerbalEVA.cs:4707
void KerbalEVA::OnSemiDeployedParachuteModeLeft(KFSMState* st) {
    if (st != st_fully_deployed_parachute) evaChute->CutParachute();
}

// KerbalEVA.cs:4715
void KerbalEVA::OnParachuteFullyDeployed() {
    fsm->RunEvent(On_fully_deploy_parachute);
    EvaEnvironment::Effect("chuteLift:on");
}

// KerbalEVA.cs:4725
void KerbalEVA::OnFullyDeployedParachuteModeEntered(KFSMState*) {
    A_CFM(ANIM.chuteIdle, 1.2f, PlayMode::StopAll);
    ANIM.chuteLeanForward.State()->enabled = true;
    ANIM.chuteLeanBackward.State()->enabled = true;
    ANIM.chuteLeanLeft.State()->enabled = true;
    ANIM.chuteLeanRight.State()->enabled = true;
}

// KerbalEVA.cs:4734
void KerbalEVA::UpdateFullyDeployedParachuteMovement() {
    EvaEnvironment::Effect("evaChute:UpdateFullyDeployedParachuteMovement");
    A_BL(ANIM.chuteLeanForward, Mathf::Clamp01(parachuteInput.x), 0.3f);
    ANIM.chuteLeanForward.State()->normalizedTime = Mathf::Lerp(ANIM.chuteLeanForward.start, ANIM.chuteLeanForward.end, ANIM.chuteLeanForward.State()->weight);
    A_BL(ANIM.chuteLeanBackward, Mathf::Clamp01(f32store(0.0 - fx(parachuteInput.x))), 0.3f);
    ANIM.chuteLeanBackward.State()->normalizedTime = Mathf::Lerp(ANIM.chuteLeanBackward.start, ANIM.chuteLeanBackward.end, ANIM.chuteLeanBackward.State()->weight);
    A_BL(ANIM.chuteLeanLeft, Mathf::Clamp01(f32store(0.0 - fx(parachuteInput.y))), 0.3f);
    ANIM.chuteLeanLeft.State()->normalizedTime = Mathf::Lerp(ANIM.chuteLeanLeft.start, ANIM.chuteLeanLeft.end, ANIM.chuteLeanLeft.State()->weight);
    A_BL(ANIM.chuteLeanRight, Mathf::Clamp01(parachuteInput.y), 0.3f);
    ANIM.chuteLeanRight.State()->normalizedTime = Mathf::Lerp(ANIM.chuteLeanRight.start, ANIM.chuteLeanRight.end, ANIM.chuteLeanRight.State()->weight);
}

// KerbalEVA.cs:4747
void KerbalEVA::OnFullyDeployedParachuteModeLeft(KFSMState*) {
    evaChute->CutParachute();
    EvaEnvironment::Effect("chuteLift:off");
    A_BL(ANIM.chuteLeanForward, 0.0f, 2.0f);
    A_BL(ANIM.chuteLeanBackward, 0.0f, 2.0f);
    A_BL(ANIM.chuteLeanLeft, 0.0f, 2.0f);
    A_BL(ANIM.chuteLeanRight, 0.0f, 2.0f);
}

// KerbalEVA.cs:4762
void KerbalEVA::OnParachuteCut() {
    if (fsm->currentStateName == fsm->CurrentState()->name) fsm->RunEvent(On_parachute_cut);
    EvaEnvironment::Effect("chuteLift:off");
}

// ---- Serenity props: golf, banana, wingnut (KerbalEVA.cs:6226-6607). The prop prefabs, renderers and particle objects are the shell's:
// the state they gate (cooldowns, sound times, one-shot flags, Random draws, the physical-object spawns) is ported; the spawns are named effects.

// KerbalEVA.cs:6226
void KerbalEVA::PlayGolf(Callback<> afterGolfCallback) {
    afterPlayGolf = afterGolfCallback;
    fsm->RunEvent(On_Playing_Golf);
}

// KerbalEVA.cs:6232
void KerbalEVA::playing_Golf_OnEnter(KFSMState*) {
    ANIM.playingGolf.State()->time = ANIM.playingGolf.start;
    A_CFM(ANIM.playingGolf, 0.2f, PlayMode::StopAll);
    if (!golfSoundFX.empty()) golfSound = golfSoundFX[static_cast<size_t>(UnityRandom::Global().Range(0, static_cast<int>(golfSoundFX.size()) - 1))];
    playingGolfAnimCooldown = ANIM.playingGolf.State()->length;
    LockInput(kAllButUiCamera, "ControlPanelLock_" + VesselPtr()->id.ToString());
    playingGolfAnimPlaying = true;
    replacedGolfBall = false;
    golfSoundPlayed = false;
    SpawnGolf();
}

// KerbalEVA.cs:6248
void KerbalEVA::SpawnGolf() {
    if (!golfClubPresent) return;
    EvaEnvironment::Effect("prop:GolfClub:play");
}

// KerbalEVA.cs:6283
void KerbalEVA::PlayingGolfPhysicalBall() {
    if (!golfSoundPlayed && fx(ANIM.playingGolf.State()->normalizedTime) > fx(golfSoundTime) && !golfSound.empty()) {
        Engine().Effect(golfSound);
        golfSoundPlayed = true;
    }
    if (fx(ANIM.playingGolf.State()->normalizedTime) > fx(ballTime) && !replacedGolfBall) {
        if (golfClubPresent) {
            // the ball: Instantiate(additionalObjectPrefab) + physicalObject.ConvertToPhysicalObject: rb.mass 0.01, drag = ballDrag, AddForce(forward * ballForce)
            ballForceDir = transform().rotation * UnityNative::AngleAxis(ballAngle, Vector3f::right()) * Vector3f::forward();
            EvaEnvironment::Effect("prop:golfBall:spawn");
        }
        replacedGolfBall = true;
    }
}

// KerbalEVA.cs:6314
void KerbalEVA::FinishedPlayingGolf() {
    if (!playingGolfAnimPlaying) return;
    if (playingGolfAnimCooldown > 0.0f) {
        playingGolfAnimCooldown = f32store(fx(playingGolfAnimCooldown) - fx(Time::deltaTime));
        return;
    }
    if (golfClubPresent) EvaEnvironment::Effect("prop:GolfClub:stop");
    playingGolfAnimPlaying = false;
    UnlockInput("ControlPanelLock_" + VesselPtr()->id.ToString());
    fsm->RunEvent(On_Golf_Complete);
    if (afterPlayGolf) afterPlayGolf();
}

// KerbalEVA.cs:6342
void KerbalEVA::Banana(Callback<> afterBananaCallback) {
    afterBanana = afterBananaCallback;
    fsm->RunEvent(On_Smashing_Banana);
}

// KerbalEVA.cs:6348
void KerbalEVA::smashing_banana_OnEnter(KFSMState*) {
    ANIM.smashBanana.State()->time = ANIM.smashBanana.start;
    A_CFM(ANIM.smashBanana, 0.2f, PlayMode::StopAll);
    if (!bananaSoundFX.empty()) bananaSound = bananaSoundFX[static_cast<size_t>(UnityRandom::Global().Range(0, static_cast<int>(bananaSoundFX.size()) - 1))];
    smashingBananaAnimCooldown = ANIM.smashBanana.State()->length;
    LockInput(kAllButUiCamera, "ControlPanelLock_" + VesselPtr()->id.ToString());
    smashingBananaAnimPlaying = true;
    replacedBanana = false;
    bananaSoundPlayed = false;
    SpawnBanana();
}

// KerbalEVA.cs:6364
void KerbalEVA::SpawnBanana() {
    if (!bananaPropPresent) return;
    EvaEnvironment::Effect("prop:Banana:play");
}

// KerbalEVA.cs:6407
void KerbalEVA::SmashingBananaParticles() {
    if (!bananaSoundPlayed && fx(ANIM.smashBanana.State()->normalizedTime) > fx(bananaSoundTime) && !bananaSound.empty()) {
        Engine().Effect(bananaSound);
        bananaSoundPlayed = true;
    }
    if (!(fx(ANIM.smashBanana.State()->normalizedTime) > fx(bananaTime)) || replacedBanana) return;
    if (bananaPropPresent) {
        // the shards: one Random.Range(0, bananaForce) x3 and Random.Range(-3, 3) x3 per renderer child (bananaShardCount of them, shell data)
        for (int i = 0; i < bananaShardCount; i++) {
            UnityRandom& r = UnityRandom::Global();
            Vector3f vector(r.Range(0.0f, bananaForce), r.Range(0.0f, bananaForce), r.Range(0.0f, bananaForce));
            Vector3f vector2(static_cast<float>(r.Range(-3, 3)), static_cast<float>(r.Range(-3, 3)), static_cast<float>(r.Range(-3, 3)));
            bananaShardImpulses.push_back({vector, vector2});
        }
        EvaEnvironment::Effect("prop:bananaShards:spawn");
    }
    replacedBanana = true;
}

// KerbalEVA.cs:6452
void KerbalEVA::FinishedSmashingBanana() {
    if (!smashingBananaAnimPlaying) return;
    if (smashingBananaAnimCooldown > 0.0f) {
        smashingBananaAnimCooldown = f32store(fx(smashingBananaAnimCooldown) - fx(Time::deltaTime));
        return;
    }
    if (bananaPropPresent) EvaEnvironment::Effect("prop:Banana:stop");
    smashingBananaAnimPlaying = false;
    UnlockInput("ControlPanelLock_" + VesselPtr()->id.ToString());
    fsm->RunEvent(On_Banana_Complete);
    if (afterBanana) afterBanana();
}

// KerbalEVA.cs:6484
void KerbalEVA::Dzhanibekov(Callback<> afterWingnutCallback) {
    afterWingnut = afterWingnutCallback;
    fsm->RunEvent(On_Spinning_Wingnut);
}

// KerbalEVA.cs:6490
void KerbalEVA::spinning_Wingnut_OnEnter(KFSMState*) {
    ANIM.spinWingnut.State()->time = ANIM.spinWingnut.start;
    A_CFM(ANIM.spinWingnut, 0.2f, PlayMode::StopAll);
    spinningWingnutAnimCooldown = wingnutTransitionTime;
    LockInput(kAllButUiCamera, "ControlPanelLock_" + VesselPtr()->id.ToString());
    spinningWingnutAnimPlaying = true;
    appliedTorque = false;
    SpawnWingnut();
}

// KerbalEVA.cs:6501
void KerbalEVA::SpawnWingnut() {
    if (!wingnutPropPresent) return;
    EvaEnvironment::Effect(std::string("prop:Wingnut:play") + (JetpackDeployed() ? ":jetpack" : ""));
}

// KerbalEVA.cs:6543
void KerbalEVA::ApplyWingnutTorque() {
    if (fx(ANIM.spinWingnut.State()->normalizedTime) > fx(wingnutTorqueTime) && !appliedTorque) {
        Engine().AddTorque(transform().forward() * wingnutKerbalTorqueForce * 1.0f);
        appliedTorque = true;
    }
}

// KerbalEVA.cs:6553
void KerbalEVA::SpinningWingnutForever() {
    if (!wingnutPropPresent) return;
    EvaEnvironment::Effect("prop:wingnut:spawnForever");
}

// KerbalEVA.cs:6580
void KerbalEVA::FinishedSpinningWingnut() {
    if (!spinningWingnutAnimPlaying) return;
    if (spinningWingnutAnimCooldown > 0.0f) {
        spinningWingnutAnimCooldown = f32store(fx(spinningWingnutAnimCooldown) - fx(Time::deltaTime));
        return;
    }
    if (wingnutPropPresent) {
        EvaEnvironment::Effect("prop:Wingnut:stop");
        SpinningWingnutForever();
    }
    spinningWingnutAnimPlaying = false;
    UnlockInput("ControlPanelLock_" + VesselPtr()->id.ToString());
    fsm->RunEvent(On_Wingnut_Complete);
    if (afterWingnut) afterWingnut();
}

// KerbalEVA.cs:6609
void KerbalEVA::SetupEVAScienceSoundFX() {
    // base.part.Effects.EffectsStartingWith("golf" / "banana"): the part's EFFECTS config names (the effects list is the FX package's)
    if (effectNamesStartingWith) {
        golfSoundFX = effectNamesStartingWith("golf");
        bananaSoundFX = effectNamesStartingWith("banana");
    }
}

}  // namespace ksp
