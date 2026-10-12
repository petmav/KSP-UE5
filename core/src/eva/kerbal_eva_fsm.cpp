// WP-32: KerbalEVA.SetupFSM (KerbalEVA.cs:3268-4352): the states, their callback lists, the events with their conditions, OnEvent bodies and
// registrations. The order of every statement is the C# order: the per-state event lists (which updateFSM walks by index) depend on it.
#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/eva/kerbal_eva.hpp"
#include "ksp/inventory/construction.hpp"
#include "ksp/modules/parachutes.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp {

using KSP::Localization::Localizer;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
}  // namespace

KFSMState* KerbalEVA::NewState(const std::string& name) {
    ownedStates_.push_back(std::make_unique<KFSMState>(name));
    stateWiring_[name];
    return ownedStates_.back().get();
}
KFSMEvent* KerbalEVA::NewEvent(const std::string& name) {
    ownedEvents_.push_back(std::make_unique<KFSMEvent>(name));
    return ownedEvents_.back().get();
}
KFSMTimedEvent* KerbalEVA::NewTimedEvent(const std::string& name, double time) {
    auto ev = std::make_unique<KFSMTimedEvent>(name, time);
    KFSMTimedEvent* raw = ev.get();
    ownedEvents_.push_back(std::move(ev));
    return raw;
}

void KerbalEVA::HookEnter(KFSMState* st, void (KerbalEVA::*fn)(KFSMState*), const char* name) {
    st->OnEnter = KFSMStateChange::Bind(this, fn);  // `st.OnEnter = method` (assignment, not Combine)
    stateWiring_[st->name].onEnter = name;
}
void KerbalEVA::HookLeave(KFSMState* st, void (KerbalEVA::*fn)(KFSMState*), const char* name) {
    st->OnLeave = KFSMStateChange::Bind(this, fn);
    stateWiring_[st->name].onLeave = name;
}
// `st.OnUpdate = method` is an assignment in the C# when written `OnUpdate = x` and a Delegate.Combine when written Combine; the first
// registration on a fresh state is the same either way, and Combine appends: HookUpdate always appends (assignments only occur on empty lists).
void KerbalEVA::HookUpdate(KFSMState* st, void (KerbalEVA::*fn)(), const char* name) {
    st->OnUpdate = st->OnUpdate + KFSMCallback::Bind(this, fn);
    stateWiring_[st->name].onUpdate.push_back(name);
}
void KerbalEVA::HookFixed(KFSMState* st, void (KerbalEVA::*fn)(), const char* name) {
    st->OnFixedUpdate = st->OnFixedUpdate + KFSMCallback::Bind(this, fn);
    stateWiring_[st->name].onFixedUpdate.push_back(name);
}
void KerbalEVA::HookLate(KFSMState* st, void (KerbalEVA::*fn)(), const char* name) {
    st->OnLateUpdate = st->OnLateUpdate + KFSMCallback::Bind(this, fn);
    stateWiring_[st->name].onLateUpdate.push_back(name);
}

#define ENTER(st, fn) HookEnter(st, &KerbalEVA::fn, #fn)
#define LEAVE(st, fn) HookLeave(st, &KerbalEVA::fn, #fn)
#define UPD(st, fn) HookUpdate(st, &KerbalEVA::fn, #fn)
#define FIX(st, fn) HookFixed(st, &KerbalEVA::fn, #fn)
#define LATE(st, fn) HookLate(st, &KerbalEVA::fn, #fn)

// KerbalEVA.cs:3268
void KerbalEVA::SetupFSM() {
    fsm = std::make_unique<KerbalFSM>();
    ownedStates_.clear();
    ownedEvents_.clear();
    stateWiring_.clear();
    eventWiring_.clear();
    fsm->OnStateChange = fsm->OnStateChange + Callback<KFSMState*, KFSMState*, KFSMEvent*>::Bind(this, &KerbalEVA::OnFSMStateChange);
    fsm->OnEventCalled = fsm->OnEventCalled + Callback<KFSMEvent*>::Bind(this, &KerbalEVA::OnFSMEventCalled);
    SetupFSMStates();
    SetupFSMEvents();
    SetupFSMEvents2();
    RecordEventWiring();
}

// KerbalEVA.cs:3275-3766
void KerbalEVA::SetupFSMStates() {
    const bool serenity = EvaEnvironment::IsInstalled("Serenity");

    st_idle_gr = NewState("Idle (Grounded)");
    ENTER(st_idle_gr, idle_OnEnter);
    FIX(st_idle_gr, correctGroundedRotation);
    FIX(st_idle_gr, UpdateMovement);
    FIX(st_idle_gr, UpdateHeading);
    FIX(st_idle_gr, UpdatePackLinear);
    FIX(st_idle_gr, updateRagdollVelocities);
    FIX(st_idle_gr, CheckLadderTriggers);
    LEAVE(st_idle_gr, idle_OnLeave);
    fsm->AddState(st_idle_gr);

    st_idle_b_gr = NewState("Idle_b (Grounded)");
    ENTER(st_idle_b_gr, idle_b_OnEnter);
    FIX(st_idle_b_gr, correctGroundedRotation);
    FIX(st_idle_b_gr, UpdateMovement);
    FIX(st_idle_b_gr, UpdateHeading);
    FIX(st_idle_b_gr, UpdatePackLinear);
    FIX(st_idle_b_gr, updateRagdollVelocities);
    FIX(st_idle_b_gr, CheckLadderTriggers);
    LEAVE(st_idle_b_gr, idle_b_OnLeave);
    fsm->AddState(st_idle_b_gr);

    On_idle_b_gr = NewEvent("New Idle (Grounded)");
    On_idle_b_gr->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_idle_b_gr->GoToStateOnEvent = st_idle_b_gr;
    fsm->AddEvent(On_idle_b_gr, {st_idle_gr, st_idle_b_gr});
    On_return_idle = NewEvent("Return to Idle (Grounded)");
    On_return_idle->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_return_idle->GoToStateOnEvent = st_idle_gr;
    fsm->AddEvent(On_return_idle, {st_idle_gr, st_idle_b_gr});

    st_walk_acd = NewState("Walk (Arcade)");
    ENTER(st_walk_acd, walk_Acd_OnEnter);
    FIX(st_walk_acd, correctGroundedRotation);
    FIX(st_walk_acd, UpdateMovement);
    FIX(st_walk_acd, UpdateHeading);
    FIX(st_walk_acd, updateRagdollVelocities);
    LEAVE(st_walk_acd, walk_ccd_OnLeave);
    fsm->AddState(st_walk_acd);

    st_walk_fps = NewState("Walk (FPS)");
    UPD(st_walk_fps, walk_fps_OnUpdate);
    FIX(st_walk_fps, correctGroundedRotation);
    FIX(st_walk_fps, UpdateMovement);
    FIX(st_walk_fps, UpdateHeading);
    FIX(st_walk_fps, updateRagdollVelocities);
    LEAVE(st_walk_fps, walk_fps_OnLeave);
    fsm->AddState(st_walk_fps);

    st_heading_acquire = NewState("Turn to Heading");
    ENTER(st_heading_acquire, heading_acquire_OnEnter);
    FIX(st_heading_acquire, correctGroundedRotation);
    FIX(st_heading_acquire, UpdateHeading);
    FIX(st_heading_acquire, updateRagdollVelocities);
    LEAVE(st_heading_acquire, heading_acquire_OnLeave);
    fsm->AddState(st_heading_acquire);

    st_run_acd = NewState("Run (Arcade)");
    ENTER(st_run_acd, run_acd_OnEnter);
    FIX(st_run_acd, correctGroundedRotation);
    FIX(st_run_acd, UpdateMovement);
    FIX(st_run_acd, UpdateHeading);
    FIX(st_run_acd, updateRagdollVelocities);
    LEAVE(st_run_acd, run_acd_OnLeave);
    fsm->AddState(st_run_acd);

    st_run_fps = NewState("Run (FPS)");
    UPD(st_run_fps, run_fps_OnUpdate);
    FIX(st_run_fps, correctGroundedRotation);
    FIX(st_run_fps, UpdateMovement);
    FIX(st_run_fps, UpdateHeading);
    FIX(st_run_fps, updateRagdollVelocities);
    LEAVE(st_run_fps, run_fps_OnLeave);
    fsm->AddState(st_run_fps);

    st_bound_gr_acd = NewState("Low G Bound (Grounded - Arcade)");
    ENTER(st_bound_gr_acd, bound_gr_acd_OnEnter);
    FIX(st_bound_gr_acd, correctGroundedRotation);
    FIX(st_bound_gr_acd, UpdateMovement);
    FIX(st_bound_gr_acd, UpdateHeading);
    FIX(st_bound_gr_acd, updateRagdollVelocities);
    FIX(st_bound_gr_acd, UpdateLowGBodyColliders);
    UPD(st_bound_gr_acd, bound_gr_acd_OnUpdate);
    LEAVE(st_bound_gr_acd, bound_gr_acd_OnLeave);
    fsm->AddState(st_bound_gr_acd);

    st_bound_gr_fps = NewState("Low G Bound (Grounded - FPS)");
    ENTER(st_bound_gr_fps, bound_gr_fps_OnEnter);
    FIX(st_bound_gr_fps, correctGroundedRotation);
    FIX(st_bound_gr_fps, UpdateMovement);
    FIX(st_bound_gr_fps, UpdateHeading);
    FIX(st_bound_gr_fps, updateRagdollVelocities);
    FIX(st_bound_gr_acd, UpdateLowGBodyColliders);  // KerbalEVA.cs:3401 (sic: st_bound_gr_acd gets it twice; st_bound_gr_fps never does)
    UPD(st_bound_gr_fps, bound_gr_fps_OnUpdate);
    LEAVE(st_bound_gr_fps, bound_gr_fps_OnLeave);
    fsm->AddState(st_bound_gr_fps);
    // :3407 lastBoundStep = Animations.walkLowGee.State.length * 0.5f
    set_lastBoundStep(f32store(fx(Animations().walkLowGee.State()->length) * fx(0.5f)));

    st_bound_fl = NewState("Low G Bound (floating)");
    ENTER(st_bound_fl, bound_fl_OnEnter);
    FIX(st_bound_fl, correctGroundedRotation);
    FIX(st_bound_fl, updateRagdollVelocities);
    FIX(st_bound_fl, UpdateLowGBodyColliders);
    LEAVE(st_bound_fl, bound_fl_OnLeave);
    fsm->AddState(st_bound_fl);

    st_ragdoll = NewState("Ragdoll");
    ENTER(st_ragdoll, ragdoll_OnEnter);
    FIX(st_ragdoll, IntegrateRagdollRigidbodyForces);
    LEAVE(st_ragdoll, ragdoll_OnLeave);
    fsm->AddState(st_ragdoll);

    st_recover = NewState("Recover");
    ENTER(st_recover, recover_OnEnter);
    UPD(st_recover, recover_OnUpdate);
    FIX(st_recover, updateRagdollVelocities);
    fsm->AddState(st_recover);

    st_jump = NewState("Jumping");
    ENTER(st_jump, jump_OnEnter);
    UPD(st_jump, correctGroundedRotation);
    UPD(st_jump, UpdateMovement);
    UPD(st_jump, UpdateHeading);
    FIX(st_jump, updateRagdollVelocities);
    fsm->AddState(st_jump);

    st_idle_fl = NewState("Idle (Floating)");
    ENTER(st_idle_fl, idle_fl_OnEnter);
    FIX(st_idle_fl, UpdateOrientationPID);
    FIX(st_idle_fl, UpdatePackAngular);
    FIX(st_idle_fl, UpdatePackLinear);
    FIX(st_idle_fl, updateRagdollVelocities);
    LEAVE(st_idle_fl, idle_fl_OnLeave);
    fsm->AddState(st_idle_fl);

    st_land = NewState("Landing");
    ENTER(st_land, land_OnEnter);
    UPD(st_land, land_OnUpdate);
    FIX(st_land, updateRagdollVelocities);
    fsm->AddState(st_land);

    if (serenity) {
        st_controlPanel_identified = NewState("Control Panel Identified");
        ENTER(st_controlPanel_identified, controlPanel_identified_OnEnter);
        UPD(st_controlPanel_identified, correctGroundedRotation);
        UPD(st_controlPanel_identified, UpdateMovement);
        UPD(st_controlPanel_identified, UpdateHeading);
        UPD(st_controlPanel_identified, ControlPanelInteractionFinished);
        FIX(st_controlPanel_identified, updateRagdollVelocities);
        fsm->AddState(st_controlPanel_identified);
    }

    st_swim_idle = NewState("Swim (Idle)");
    ENTER(st_swim_idle, swim_idle_OnEnter);
    FIX(st_swim_idle, correctGroundedRotation);
    FIX(st_swim_idle, UpdateMovement);
    FIX(st_swim_idle, UpdateHeading);
    FIX(st_swim_idle, updateRagdollVelocities);
    FIX(st_swim_idle, CheckLadderTriggers);
    LEAVE(st_swim_idle, swim_idle_OnLeave);
    fsm->AddState(st_swim_idle);

    st_swim_fwd = NewState("Swim (fwd)");
    ENTER(st_swim_fwd, swim_fwd_OnEnter);
    FIX(st_swim_fwd, correctGroundedRotation);
    FIX(st_swim_fwd, UpdateMovement);
    FIX(st_swim_fwd, UpdateHeading);
    FIX(st_swim_fwd, updateRagdollVelocities);
    LEAVE(st_swim_fwd, swim_fwd_OnLeave);
    fsm->AddState(st_swim_fwd);

    st_ladder_acquire = NewState("Ladder (Acquire)");
    ENTER(st_ladder_acquire, ladder_acquire_OnEnter);
    FIX(st_ladder_acquire, correctLadderPosition);
    FIX(st_ladder_acquire, updateRagdollVelocities);
    LEAVE(st_ladder_acquire, ladder_acquire_OnLeave);
    fsm->AddState(st_ladder_acquire);

    st_ladder_idle = NewState("Ladder (Idle)");
    st_ladder_idle->updateMode = KFSMUpdateMode::UPDATE;
    stateWiring_[st_ladder_idle->name].updateMode = static_cast<int>(KFSMUpdateMode::UPDATE);
    ENTER(st_ladder_idle, ladder_idle_OnEnter);
    FIX(st_ladder_idle, correctLadderRotation);
    FIX(st_ladder_idle, correctLadderPosition);
    FIX(st_ladder_idle, UpdateLadderMovement);
    FIX(st_ladder_idle, updateRagdollVelocities);
    UPD(st_ladder_idle, InterpolateLadders);
    UPD(st_ladder_idle, UpdateCurrentLadderIdle);
    LEAVE(st_ladder_idle, ladder_idle_OnLeave);
    fsm->AddState(st_ladder_idle);

    st_ladder_lean = NewState("Ladder (Lean)");
    ENTER(st_ladder_lean, ladder_lean_OnEnter);
    FIX(st_ladder_lean, correctLadderRotation);
    FIX(st_ladder_lean, correctLadderPosition);
    FIX(st_ladder_lean, updateRagdollVelocities);
    LATE(st_ladder_lean, ladder_lean_OnLateUpdate);
    LEAVE(st_ladder_lean, ladder_lean_OnLeave);
    fsm->AddState(st_ladder_lean);

    st_ladder_climb = NewState("Ladder (Climb)");
    ENTER(st_ladder_climb, ladder_climb_OnEnter);
    FIX(st_ladder_climb, correctLadderRotation);
    FIX(st_ladder_climb, correctLadderPosition);
    FIX(st_ladder_climb, UpdateLadderMovement);
    FIX(st_ladder_climb, updateRagdollVelocities);
    UPD(st_ladder_climb, InterpolateLadders);
    UPD(st_ladder_climb, UpdateCurrentLadder);
    LEAVE(st_ladder_climb, ladder_climb_OnLeave);
    fsm->AddState(st_ladder_climb);

    st_ladder_descend = NewState("Ladder (Descend)");
    ENTER(st_ladder_descend, ladder_descend_OnEnter);
    FIX(st_ladder_descend, correctLadderRotation);
    FIX(st_ladder_descend, correctLadderPosition);
    FIX(st_ladder_descend, UpdateLadderMovement);
    FIX(st_ladder_descend, updateRagdollVelocities);
    UPD(st_ladder_descend, InterpolateLadders);
    UPD(st_ladder_descend, UpdateCurrentLadder);
    LEAVE(st_ladder_descend, ladder_descend_OnLeave);
    fsm->AddState(st_ladder_descend);

    st_ladder_pushoff = NewState("Ladder (Pushoff)");
    FIX(st_ladder_pushoff, correctLadderRotation);
    FIX(st_ladder_pushoff, correctLadderPosition);
    FIX(st_ladder_pushoff, updateRagdollVelocities);
    LEAVE(st_ladder_pushoff, ladder_pushoff_OnLeave);
    fsm->AddState(st_ladder_pushoff);

    st_ladder_end_reached = NewState("Ladder (End Reached)");
    ENTER(st_ladder_end_reached, ladder_end_reached_OnEnter);
    FIX(st_ladder_end_reached, correctLadderRotation);
    FIX(st_ladder_end_reached, correctLadderPosition);
    FIX(st_ladder_end_reached, UpdateLadderMovement);
    FIX(st_ladder_end_reached, updateRagdollVelocities);
    UPD(st_ladder_end_reached, InterpolateLadders);
    UPD(st_ladder_end_reached, UpdateCurrentLadder);
    LEAVE(st_ladder_end_reached, ladder_end_reached_OnLeave);
    fsm->AddState(st_ladder_end_reached);

    st_clamber_acquireP1 = NewState("Clamber (P1)");
    ENTER(st_clamber_acquireP1, clamber_acquireP1_OnEnter);
    FIX(st_clamber_acquireP1, ZeroRBVelocity);
    FIX(st_clamber_acquireP1, correctGroundedRotation);
    FIX(st_clamber_acquireP1, updateRagdollVelocities);
    LEAVE(st_clamber_acquireP1, clamber_acquireP1_OnLeave);
    fsm->AddState(st_clamber_acquireP1);

    st_clamber_acquireP2 = NewState("Clamber (P2)");
    ENTER(st_clamber_acquireP2, clamber_acquireP2_OnEnter);
    FIX(st_clamber_acquireP2, ZeroRBVelocity);
    FIX(st_clamber_acquireP2, correctGroundedRotation);
    FIX(st_clamber_acquireP2, updateRagdollVelocities);
    LEAVE(st_clamber_acquireP2, clamber_acquireP2_OnLeave);
    fsm->AddState(st_clamber_acquireP2);

    st_clamber_acquireP3 = NewState("Clamber (P3)");
    ENTER(st_clamber_acquireP3, clamber_acquireP3_OnEnter);
    FIX(st_clamber_acquireP3, ZeroRBVelocity);
    FIX(st_clamber_acquireP3, correctGroundedRotation);
    FIX(st_clamber_acquireP3, updateRagdollVelocities);
    LEAVE(st_clamber_acquireP3, clamber_acquireP3_OnLeave);
    fsm->AddState(st_clamber_acquireP3);

    st_flagAcquireHeading = NewState("Flag-plant Terrain Acquire");
    ENTER(st_flagAcquireHeading, flagAcquireHeading_OnEnter);
    FIX(st_flagAcquireHeading, correctGroundedRotation);
    FIX(st_flagAcquireHeading, UpdateMovement);
    FIX(st_flagAcquireHeading, UpdateHeading);
    FIX(st_flagAcquireHeading, updateRagdollVelocities);
    LATE(st_flagAcquireHeading, flagAcquireHeading_OnLateUpdate);
    LEAVE(st_flagAcquireHeading, flagAcquireHeading_OnLeave);
    fsm->AddState(st_flagAcquireHeading);

    st_flagPlant = NewState("Planting Flag");
    ENTER(st_flagPlant, flagPlant_OnEnter);
    FIX(st_flagPlant, ZeroRBVelocity);
    UPD(st_flagPlant, correctGroundedRotation);
    UPD(st_flagPlant, UpdateMovement);
    FIX(st_flagPlant, updateRagdollVelocities);
    LEAVE(st_flagPlant, flagPlant_OnLeave);
    fsm->AddState(st_flagPlant);

    st_seated_cmd = NewState("Seated (Command)");
    ENTER(st_seated_cmd, seated_cmd_OnEnter);
    UPD(st_seated_cmd, UpdateParachuteInCommandSeat);
    LEAVE(st_seated_cmd, seated_cmd_OnLeave);
    fsm->AddState(st_seated_cmd);

    st_grappled = NewState("Grappled");
    ENTER(st_grappled, grappled_OnEnter);
    FIX(st_grappled, IntegrateRagdollRigidbodyForces);
    LEAVE(st_grappled, grappled_OnLeave);
    fsm->AddState(st_grappled);

    st_semi_deployed_parachute = NewState("Semi Deployed Parachute");
    ENTER(st_semi_deployed_parachute, OnSemiDeployedParachuteModeEntered);
    FIX(st_semi_deployed_parachute, UpdateSemiDeployedParachuteMovement);
    FIX(st_semi_deployed_parachute, updateRagdollVelocities);
    LEAVE(st_semi_deployed_parachute, OnSemiDeployedParachuteModeLeft);
    fsm->AddState(st_semi_deployed_parachute);

    st_fully_deployed_parachute = NewState("Fully Deployed Parachute");
    ENTER(st_fully_deployed_parachute, OnFullyDeployedParachuteModeEntered);
    FIX(st_fully_deployed_parachute, UpdateFullyDeployedParachuteMovement);
    FIX(st_fully_deployed_parachute, updateRagdollVelocities);
    LEAVE(st_fully_deployed_parachute, OnFullyDeployedParachuteModeLeft);
    fsm->AddState(st_fully_deployed_parachute);

    if (serenity) {
        st_picking_roc_sample = NewState("Picking Roc Sample");
        ENTER(st_picking_roc_sample, picking_roc_sample_OnEnter);
        UPD(st_picking_roc_sample, correctGroundedRotation);
        UPD(st_picking_roc_sample, UpdateMovement);
        UPD(st_picking_roc_sample, UpdateHeading);
        UPD(st_picking_roc_sample, RocSampleStored);
        UPD(st_picking_roc_sample, spawnHammer);
        FIX(st_picking_roc_sample, updateRagdollVelocities);
        fsm->AddState(st_picking_roc_sample);
    }

    st_enteringConstruction = NewState("Entering construction mode");
    ENTER(st_enteringConstruction, enteringConstruction_OnEnter);
    FIX(st_enteringConstruction, enteringExitingConstruction_OnFixedUpdate);
    fsm->AddState(st_enteringConstruction);

    st_exitingConstruction = NewState("Exiting construction mode");
    ENTER(st_exitingConstruction, exitingConstruction_OnEnter);
    FIX(st_exitingConstruction, enteringExitingConstruction_OnFixedUpdate);
    LEAVE(st_exitingConstruction, exitingConstruction_OnLeave);
    fsm->AddState(st_exitingConstruction);

    st_weldAcquireHeading = NewState("Weld Turn to face Part");
    ENTER(st_weldAcquireHeading, weld_acquireHeading_OnEnter);
    FIX(st_weldAcquireHeading, weld_acquireHeading_OnFixedUpdate);
    LATE(st_weldAcquireHeading, weld_acquireHeading_OnLateUpdate);
    LEAVE(st_weldAcquireHeading, weld_acquireHeading_OnLeave);
    fsm->AddState(st_weldAcquireHeading);

    st_weld = NewState("Welding part");
    ENTER(st_weld, weld_OnEnter);
    st_weld->OnFixedUpdate = KFSMCallback::Bind(this, &KerbalEVA::weld_OnFixedUpdate);  // assignment (:3719)
    stateWiring_[st_weld->name].onFixedUpdate.push_back("weld_OnFixedUpdate");
    LEAVE(st_weld, weld_OnLeave);
    fsm->AddState(st_weld);

    st_playing_golf = NewState("Playing Golf");
    ENTER(st_playing_golf, playing_Golf_OnEnter);
    UPD(st_playing_golf, correctGroundedRotation);
    UPD(st_playing_golf, UpdateMovement);
    UPD(st_playing_golf, UpdateHeading);
    UPD(st_playing_golf, PlayingGolfPhysicalBall);
    UPD(st_playing_golf, FinishedPlayingGolf);
    FIX(st_playing_golf, updateRagdollVelocities);
    fsm->AddState(st_playing_golf);

    st_smashing_banana = NewState("Smashing Banana");
    ENTER(st_smashing_banana, smashing_banana_OnEnter);
    UPD(st_smashing_banana, correctGroundedRotation);
    UPD(st_smashing_banana, UpdateMovement);
    UPD(st_smashing_banana, UpdateHeading);
    UPD(st_smashing_banana, SmashingBananaParticles);
    UPD(st_smashing_banana, FinishedSmashingBanana);
    FIX(st_smashing_banana, updateRagdollVelocities);
    fsm->AddState(st_smashing_banana);

    st_spinning_wingnut = NewState("Spinning Wingnut");
    ENTER(st_spinning_wingnut, spinning_Wingnut_OnEnter);
    UPD(st_spinning_wingnut, ApplyWingnutTorque);
    UPD(st_spinning_wingnut, FinishedSpinningWingnut);
    FIX(st_spinning_wingnut, UpdateOrientationPID);
    FIX(st_spinning_wingnut, UpdatePackAngular);
    FIX(st_spinning_wingnut, UpdatePackLinear);
    FIX(st_spinning_wingnut, updateRagdollVelocities);
    fsm->AddState(st_spinning_wingnut);
}

#define COND(body) KFSMEventCondition::Lambda(this, [this](KFSMState* currentState) -> bool { (void)currentState; body })
#define ONEV(body) KFSMCallback::Lambda(this, [this]() { body })

namespace {
// UnityEngine.Vector3 != Vector3.zero (approximate comparison operator)
inline bool NotZero(const Vector3f& v) { return v != Vector3f::zero(); }
inline bool IsZero(const Vector3f& v) { return v == Vector3f::zero(); }
}  // namespace

// KerbalEVA.cs:3767-4147
void KerbalEVA::SetupFSMEvents() {
    On_hdgAcquireStart = NewEvent("Hdg Acquire Start");
    On_hdgAcquireStart->GoToStateOnEvent = st_heading_acquire;
    On_hdgAcquireStart->OnCheckCondition = COND(return fx(Mathf::Abs(deltaHdg)) > fx(60.0f););
    fsm->AddEvent(On_hdgAcquireStart, {st_bound_gr_acd, st_bound_gr_fps, st_walk_acd, st_walk_fps, st_run_acd, st_run_fps});

    On_hdgAcquireComplete = NewEvent("Hdg Acquire Complete");
    On_hdgAcquireComplete->GoToStateOnEvent = st_idle_gr;
    On_hdgAcquireComplete->OnCheckCondition = COND(return fx(Mathf::Abs(deltaHdg)) < fx(30.0f););
    fsm->AddEvent(On_hdgAcquireComplete, {st_heading_acquire});

    On_MoveAcd = NewEvent("Move (Arcade)");
    On_MoveAcd->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_MoveAcd->GoToStateOnEvent = st_walk_acd;
    On_MoveAcd->OnCheckCondition = COND(return NotZero(tgtRpos) && VesselPtr()->mainBody()->GeeASL > fx(minWalkingGee()) && !CharacterFrameMode &&
                                               !EVAConstructionModeController::MovementRestricted() && SurfaceContact(););
    fsm->AddEvent(On_MoveAcd, {st_idle_gr, st_idle_b_gr, st_walk_fps, st_run_fps, st_enteringConstruction, st_exitingConstruction});

    On_MoveFPS = NewEvent("Move (FPS)");
    On_MoveFPS->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_MoveFPS->GoToStateOnEvent = st_walk_fps;
    On_MoveFPS->OnCheckCondition = COND(return NotZero(tgtRpos) && VesselPtr()->mainBody()->GeeASL > fx(minWalkingGee()) && CharacterFrameMode &&
                                               !EVAConstructionModeController::MovementRestricted() && SurfaceContact(););
    fsm->AddEvent(On_MoveFPS, {st_idle_gr, st_idle_b_gr, st_walk_acd, st_run_acd, st_enteringConstruction, st_exitingConstruction});

    On_MoveLowG_Acd = NewEvent("Move Low G (Arcade)");
    On_MoveLowG_Acd->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_MoveLowG_Acd->GoToStateOnEvent = st_bound_gr_acd;
    On_MoveLowG_Acd->OnCheckCondition = COND(return NotZero(tgtRpos) && VesselPtr()->mainBody()->GeeASL <= fx(minWalkingGee()) && !CharacterFrameMode &&
                                                    !EVAConstructionModeController::MovementRestricted(););
    On_MoveLowG_Acd->OnEvent = ONEV();
    fsm->AddEvent(On_MoveLowG_Acd, {st_idle_gr, st_idle_b_gr, st_bound_gr_fps, st_enteringConstruction, st_exitingConstruction});

    On_MoveLowG_fps = NewEvent("Move Low G (FPS)");
    On_MoveLowG_fps->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_MoveLowG_fps->GoToStateOnEvent = st_bound_gr_fps;
    On_MoveLowG_fps->OnCheckCondition = COND(return NotZero(tgtRpos) && VesselPtr()->mainBody()->GeeASL <= fx(minWalkingGee()) && CharacterFrameMode &&
                                                    !EVAConstructionModeController::MovementRestricted(););
    On_MoveLowG_fps->OnEvent = ONEV();
    fsm->AddEvent(On_MoveLowG_fps, {st_idle_gr, st_idle_b_gr, st_bound_gr_acd, st_enteringConstruction, st_exitingConstruction});

    On_bound = NewEvent("Low G bound");
    On_bound->GoToStateOnEvent = st_bound_fl;
    On_bound->updateMode = KFSMUpdateMode::UPDATE;
    On_bound->OnCheckCondition = COND(return SurfaceContact() && NotZero(tgtRpos) && fx(Mathf::Abs(deltaHdg)) < fx(5.0f) && !VesselPtr()->packed &&
                                             fsm->TimeAtCurrentState() > fx(Mathf::Min(boundFrequency(), lastBoundStep())) &&
                                             !EVAConstructionModeController::MovementRestricted(););
    On_bound->OnEvent = ONEV({
        deltaHdg = 0.0f;
        // boundForce = Mathf.Round(base.part.mass * boundForceMassFactor * 100f) / 100f;
        set_boundForce(f32store(fx(Mathf::Round(f32store(fx(PartPtr()->fields.mass) * fx(boundForceMassFactor) * fx(100.0f)))) / fx(100.0f)));
        Vector3f direction = transform().InverseTransformDirection(Engine().RbVelocity());
        direction.y = 0.0f;
        Engine().SetRbVelocity(transform().TransformDirection(direction));
        Vector3f vector = UnityNative::FromToRotation(tgtRpos.normalized(), Engine().RbVelocity().normalized()) *
                          (transform().up() * boundForce() + tgtRpos.normalized() * boundSpeed() * massMultiplier());
        Engine().SetRbAngularVelocity(Vector3f::zero());
        Engine().AddImpulse(vector);
    });
    fsm->AddEvent(On_bound, {st_bound_gr_acd, st_bound_gr_fps});

    // :3817 halfHeight -= Physics.defaultContactOffset * 2f
    halfHeight = f32store(fx(halfHeight) - fx(EvaEnvironment::defaultContactOffset) * 2.0);

    On_bound_land = NewEvent("Low G bound land");
    On_bound_land->updateMode = KFSMUpdateMode::UPDATE;
    On_bound_land->OnCheckCondition = COND(return VesselPtr()->Splashed ||
                                                  (std::fabs(static_cast<double>(HeightFromSurface())) < fx(halfHeight) + fx(boundThreshold()) &&
                                                   fsm->TimeAtCurrentState() > 0.10000000149011612) ||
                                                  (SurfaceContact() && fsm->TimeAtCurrentState() > 0.5););
    On_bound_land->OnEvent = ONEV({
        boundLeftFoot = !boundLeftFoot;
        On_bound_land->GoToStateOnEvent = CharacterFrameMode ? st_bound_gr_fps : st_bound_gr_acd;
    });
    fsm->AddEvent(On_bound_land, {st_bound_fl});

    On_bound_fall = NewEvent("Low G Bound Fall");
    On_bound_fall->GoToStateOnEvent = st_idle_fl;
    On_bound_fall->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_bound_fall->OnCheckCondition = COND(return !SurfaceOrSplashed() && fsm->TimeAtCurrentState() > fx(lastBoundStep()) * fx(boundFallThreshold()) &&
                                                  !EVAConstructionModeController::MovementRestricted(););
    On_bound_fall->OnEvent = ONEV({ ModifyBodyColliderHeight(0.0f); });
    fsm->AddEvent(On_bound_fall, {st_bound_fl});

    On_startRun = NewEvent("Start Run");
    On_startRun->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_startRun->OnCheckCondition = COND(return Key(EvaKey::Run, KeyQuery::Held) && !JetpackDeployed() && VesselUnderControl() &&
                                                !EVAConstructionModeController::MovementRestricted(););
    On_startRun->OnEvent = ONEV({
        if (VesselPtr()->mainBody()->GeeASL > fx(minWalkingGee())) On_startRun->GoToStateOnEvent = CharacterFrameMode ? st_run_fps : st_run_acd;
    });
    fsm->AddEvent(On_startRun, {st_walk_acd, st_walk_fps, st_enteringConstruction, st_exitingConstruction});

    On_endRun = NewEvent("End Run");
    On_endRun->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_endRun->OnCheckCondition = COND(return !Key(EvaKey::Run, KeyQuery::Held););
    On_endRun->OnEvent = ONEV({ On_endRun->GoToStateOnEvent = CharacterFrameMode ? st_walk_fps : st_walk_acd; });
    fsm->AddEvent(On_endRun, {st_run_acd, st_run_fps});

    On_stop = NewEvent("Stop");
    On_stop->GoToStateOnEvent = st_idle_gr;
    On_stop->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_stop->OnCheckCondition = COND(return IsZero(tgtRpos););
    fsm->AddEvent(On_stop, {st_walk_acd, st_run_acd, st_bound_gr_acd, st_bound_gr_fps, st_walk_fps, st_run_fps});

    On_jump_start = NewEvent("Jump Start");
    On_jump_start->GoToStateOnEvent = st_jump;
    On_jump_start->OnCheckCondition = COND(return Key(EvaKey::Jump, KeyQuery::Held) && VesselUnderControl() && !PartPlacementMode() &&
                                                  !EVAConstructionModeController::MovementRestricted(););
    fsm->AddEvent(On_jump_start, {st_idle_gr, st_idle_b_gr, st_walk_acd, st_walk_fps, st_run_acd, st_run_fps, st_bound_gr_acd, st_bound_gr_fps});

    On_jump_complete = NewTimedEvent("Jump Launch", 0.3);
    On_jump_complete->GoToStateOnEvent = st_idle_fl;
    On_jump_complete->OnEvent = ONEV({
        // base.part.AddImpulse(base.transform.up * Mathf.Pow(base.part.mass / PhysicsGlobals.PerCommandSeatReduction, jumpMultiplier) * maxJumpForce
        //                      + base.transform.forward * tgtSpeed * massMultiplier)
        PhysicsGlobals* pg = PhysicsGlobals::Instance();
        if (pg == nullptr) throw Nre();
        const float ratio = f32store(fx(PartPtr()->fields.mass) / fx(pg->perCommandSeatReduction));
        Engine().AddImpulse(transform().up() * Mathf::Pow(ratio, jumpMultiplier()) * maxJumpForce() + transform().forward() * tgtSpeed * massMultiplier());
    });
    fsm->AddEvent(On_jump_complete, {st_jump});

    On_land_start = NewEvent("Landing");
    On_land_start->OnCheckCondition = COND(return SurfaceContact() && fsm->FramesInCurrentState() > 2;);
    On_land_start->OnEvent = ONEV({
        const float st2 = stumbleThreshold();
        if (!(fx(VesselPtr()->rb_velocity.sqrMagnitude()) > fx(st2) * fx(st2)) && fx(Vector3f::Dot(transform().up(), lastCollisionDirection)) <= fx(0.9f)) {
            if (!(fx(Mathf::Abs(Vector3f::Dot(fUp, lastCollisionNormal))) > fx(0.4f)) && UT() - lastCollisionTime <= 0.4) {
                On_land_start->GoToStateOnEvent = st_idle_fl;
            } else if (fx(Vector3f::Dot(transform().up(), fUp)) > fx(0.8f)) {
                On_land_start->GoToStateOnEvent = st_land;
            } else {
                On_land_start->GoToStateOnEvent = st_recover;
            }
        } else {
            On_land_start->GoToStateOnEvent = st_ragdoll;
        }
        cmdDir = Engine().RbVelocity().normalized();
    });
    fsm->AddEvent(On_land_start, {st_idle_fl, st_semi_deployed_parachute, st_fully_deployed_parachute});

    On_land_complete = NewTimedEvent("Landed", 0.10000000149011612);
    On_land_complete->GoToStateOnEvent = st_idle_gr;
    fsm->AddEvent(On_land_complete, {st_land});

    On_fall = NewEvent("Fall");
    On_fall->GoToStateOnEvent = st_idle_fl;
    On_fall->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_fall->OnCheckCondition = COND(return std::fabs(fx(VesselPtr()->heightFromTerrain)) > fx(onFallHeightFromTerrain()) && !SurfaceOrSplashed(););
    fsm->AddEvent(On_fall, {st_idle_gr, st_idle_b_gr, st_walk_acd, st_run_acd, st_swim_idle, st_swim_fwd});

    On_stumble = NewEvent("Stumble");
    On_stumble->GoToStateOnEvent = st_ragdoll;
    On_stumble->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    fsm->AddEventExcluding(On_stumble, {st_ragdoll, st_grappled, st_clamber_acquireP2, st_clamber_acquireP3});

    On_recover_start = NewEvent("Recover Start");
    On_recover_start->GoToStateOnEvent = st_recover;
    On_recover_start->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_recover_start->OnCheckCondition = COND(return CanRecover() && fsm->TimeAtCurrentState() > 0.20000000298023224;);
    fsm->AddEvent(On_recover_start, {st_ragdoll});

    On_recover_complete = NewTimedEvent("Recover End", 0.5);
    On_recover_complete->OnEvent = ONEV({
        isRagdoll = false;
        lastTgtSpeed = 0.0f;
        EvaEnvironment::Effect("CollisionManager.UpdateAllColliders");
        if (SurfaceContact()) On_recover_complete->GoToStateOnEvent = st_idle_gr;
        else if (VesselPtr()->Splashed) On_recover_complete->GoToStateOnEvent = st_swim_idle;
        else On_recover_complete->GoToStateOnEvent = st_idle_fl;
    });
    fsm->AddEvent(On_recover_complete, {st_recover});

    On_packToggle = NewEvent("Pack Toggle");
    On_packToggle->updateMode = KFSMUpdateMode::UPDATE;
    On_packToggle->OnCheckCondition = COND({
        if (Key(EvaKey::TogglePack, KeyQuery::Down) && VesselUnderControl() && !PartPlacementMode()) {
            On_packToggle->GoToStateOnEvent = fsm->CurrentState();
            return true;
        }
        return false;
    });
    On_packToggle->OnEvent = ONEV({ ToggleJetpack(!JetpackDeployed()); });
    fsm->AddEvent(On_packToggle, {st_idle_gr, st_idle_b_gr, st_idle_fl, st_weldAcquireHeading});

    On_feet_wet = NewEvent("Feet Wet");
    On_feet_wet->OnCheckCondition = COND(return VesselPtr()->Splashed && !SurfaceContact(););
    On_feet_wet->OnEvent = ONEV({
        const float st2 = stumbleThreshold();
        if (fx(Engine().RbVelocity().sqrMagnitude()) < fx(st2) * fx(st2)) {
            if (fx(Vector3f::Dot(transform().up(), fUp)) > fx(0.75f)) On_feet_wet->GoToStateOnEvent = st_swim_idle;
            else On_feet_wet->GoToStateOnEvent = st_recover;
        } else {
            On_feet_wet->GoToStateOnEvent = st_ragdoll;
        }
    });
    fsm->AddEvent(On_feet_wet, {st_idle_fl, st_idle_gr, st_idle_b_gr, st_run_acd, st_walk_acd});

    On_feet_dry = NewEvent("Feet Dry");
    On_feet_dry->OnCheckCondition = COND(return SurfaceContact(););
    On_feet_dry->OnEvent = ONEV({
        if (fx(Vector3f::Dot(transform().up(), fUp)) > fx(0.3f)) On_feet_dry->GoToStateOnEvent = st_idle_fl;
        else On_feet_dry->GoToStateOnEvent = st_recover;
    });
    fsm->AddEvent(On_feet_dry, {st_swim_idle, st_swim_fwd});

    On_swim_fwd = NewEvent("Swim Forward");
    On_swim_fwd->GoToStateOnEvent = st_swim_fwd;
    On_swim_fwd->OnCheckCondition = COND(return NotZero(tgtRpos););
    fsm->AddEvent(On_swim_fwd, {st_swim_idle});

    On_swim_stop = NewEvent("Swim Stop");
    On_swim_stop->GoToStateOnEvent = st_swim_idle;
    On_swim_stop->OnCheckCondition = COND(return IsZero(tgtRpos););
    fsm->AddEvent(On_swim_stop, {st_swim_fwd});

    if (EvaEnvironment::IsInstalled("Serenity")) {
        On_control_panel_search = NewEvent("Control Panel Search");
        On_control_panel_search->updateMode = KFSMUpdateMode::FIXEDUPDATE;
        On_control_panel_search->OnCheckCondition = COND({
            if (FindControlPanel(controlPanelStandoff(), controlPanelReach()) && VesselUnderControl() && Key(EvaKey::Use, KeyQuery::Held)) {
                RandomControlPanelAnim();
                return true;
            }
            return false;
        });
        On_control_panel_search->GoToStateOnEvent = st_controlPanel_identified;
        fsm->AddEvent(On_control_panel_search, {st_idle_gr, st_idle_b_gr, st_walk_acd, st_walk_fps, st_heading_acquire, st_run_acd, st_run_fps,
                                                st_bound_gr_acd, st_idle_fl, st_bound_gr_fps, st_ladder_idle, st_controlPanel_identified});
    }
    if (EvaEnvironment::IsInstalled("Serenity")) {
        On_control_panel_interacting = NewEvent("Control Panel Interacting");
        On_control_panel_interacting->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
        On_control_panel_interacting->GoToStateOnEvent = st_idle_gr;
        fsm->AddEvent(On_control_panel_interacting, {st_controlPanel_identified, st_idle_gr});
    }

    On_ladderGrabStart = NewEvent("Ladder Grab Start");
    On_ladderGrabStart->GoToStateOnEvent = st_ladder_acquire;
    On_ladderGrabStart->OnCheckCondition = COND({
        if (!currentLadderTriggers.empty() && VesselUnderControl() && !EVAConstructionModeController::MovementRestricted()) {
            PostInteractionScreenMessage(cacheAutoLOC_114130);
            if (Key(EvaKey::Use, KeyQuery::Down)) return true;
        }
        return false;
    });
    fsm->AddEvent(On_ladderGrabStart, {st_idle_fl, st_idle_gr, st_idle_b_gr, st_swim_idle});

    On_ladderGrabComplete = NewTimedEvent("Ladder Grab Complete", 1.0);
    On_ladderGrabComplete->GoToStateOnEvent = st_ladder_idle;
    fsm->AddEvent(On_ladderGrabComplete, {st_ladder_acquire});

    On_ladderClimb = NewEvent("Ladder Climb");
    On_ladderClimb->GoToStateOnEvent = st_ladder_climb;
    On_ladderClimb->OnCheckCondition = COND(return currentLadder != nullptr && (!GameSettings::EVA_LADDER_CHECK_END || canClimb) &&
                                                   !EVAConstructionModeController::MovementRestricted() &&
                                                   fx(Vector3f::Dot(ladderTgtRPos, currentLadder->pose.up() * (invLadderAxis ? -1.0f : 1.0f))) > fx(0.3f););
    fsm->AddEvent(On_ladderClimb, {st_ladder_idle, st_ladder_descend, st_ladder_end_reached});

    On_ladderDescend = NewEvent("Ladder Descend");
    On_ladderDescend->GoToStateOnEvent = st_ladder_descend;
    On_ladderDescend->OnCheckCondition = COND(return currentLadder != nullptr && (!GameSettings::EVA_LADDER_CHECK_END || canDescend) &&
                                                     !EVAConstructionModeController::MovementRestricted() &&
                                                     fx(Vector3f::Dot(ladderTgtRPos, currentLadder->pose.up() * (invLadderAxis ? -1.0f : 1.0f))) < fx(-0.3f););
    fsm->AddEvent(On_ladderDescend, {st_ladder_idle, st_ladder_climb, st_ladder_end_reached});

    On_ladderStop = NewEvent("Ladder Stop");
    On_ladderStop->GoToStateOnEvent = st_ladder_idle;
    On_ladderStop->OnCheckCondition = COND(return IsZero(ladderTgtRPos););
    fsm->AddEvent(On_ladderStop, {st_ladder_climb, st_ladder_descend});

    On_ladderLetGo = NewEvent("Ladder Let Go");
    On_ladderLetGo->GoToStateOnEvent = st_idle_fl;
    On_ladderLetGo->OnCheckCondition = COND(return Key(EvaKey::Jump, KeyQuery::Down) && VesselUnderControl() && !EVAConstructionModeController::MovementRestricted(););
    On_ladderLetGo->OnEvent = ONEV({
        currentLadderTriggers.clear();
        currentLadder = nullptr;
        if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = false;
        set_currentLadderPart(nullptr);
        VesselPtr()->UpdateLandedSplashed();
        secondaryLadder = nullptr;
    });
    fsm->AddEvent(On_ladderLetGo, {st_ladder_idle, st_ladder_climb, st_ladder_descend, st_ladder_end_reached});

    On_LadderEnd = NewEvent("Ladder End");
    On_LadderEnd->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_LadderEnd->GoToStateOnEvent = st_idle_fl;
    On_LadderEnd->OnCheckCondition = COND(return currentLadder == nullptr || currentLadderTriggers.empty() || !currentLadder->activeInHierarchy || !currentLadder->enabled;);
    On_LadderEnd->OnEvent = ONEV({
        currentLadderTriggers.clear();
        currentLadder = nullptr;
        if (currentLadderPart() != nullptr) currentLadderPart()->fields.hasKerbalOnLadder = false;
        set_currentLadderPart(nullptr);
        VesselPtr()->UpdateLandedSplashed();
        secondaryLadder = nullptr;
    });
    fsm->AddEvent(On_LadderEnd, {st_ladder_idle, st_ladder_climb, st_ladder_descend, st_ladder_acquire, st_ladder_lean, st_ladder_end_reached});

    On_LadderLeanStart = NewEvent("Ladder Lean (Start)");
    On_LadderLeanStart->GoToStateOnEvent = st_ladder_lean;
    On_LadderLeanStart->OnCheckCondition = COND(return Key(EvaKey::Run, KeyQuery::Held) && VesselUnderControl() && !EVAConstructionModeController::MovementRestricted(););
    fsm->AddEvent(On_LadderLeanStart, {st_ladder_idle});

    On_LadderLeanEnd = NewEvent("Ladder Lean (End)");
    On_LadderLeanEnd->GoToStateOnEvent = st_ladder_idle;
    On_LadderLeanEnd->OnCheckCondition = COND(return !Key(EvaKey::Run, KeyQuery::Held););
    fsm->AddEvent(On_LadderLeanEnd, {st_ladder_lean});

    On_LadderPushOff = NewEvent("Ladder Push Off Start");
    On_LadderPushOff->GoToStateOnEvent = st_ladder_pushoff;
    On_LadderPushOff->OnCheckCondition = COND(return Key(EvaKey::Jump, KeyQuery::Held) && VesselUnderControl() && !EVAConstructionModeController::MovementRestricted(););
    fsm->AddEvent(On_LadderPushOff, {st_ladder_lean});

    On_LadderPushoff_complete = NewTimedEvent("Ladder Push Off Complete", 0.3);
    On_LadderPushoff_complete->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_LadderPushoff_complete->GoToStateOnEvent = st_idle_fl;
    On_LadderPushoff_complete->OnEvent = ONEV({
        if (NotZero(ladderTgtRPos)) Engine().AddImpulse(ladderTgtRPos * ladderPushoffForce());
        else Engine().AddImpulse(-transform().forward() * ladderPushoffForce());
    });
    fsm->AddEvent(On_LadderPushoff_complete, {st_ladder_pushoff});

    On_ladderEndReached = NewEvent("Ladder End Reached");
    On_ladderEndReached->GoToStateOnEvent = st_ladder_end_reached;
    On_ladderEndReached->OnCheckCondition = COND(return currentLadder != nullptr && GameSettings::EVA_LADDER_CHECK_END && currentSpd == 0.0f && (!canClimb || !canDescend););
    fsm->AddEvent(On_ladderEndReached, {st_ladder_idle, st_ladder_descend, st_ladder_climb, st_clamber_acquireP1, st_idle_fl});

    On_clamberGrabStart = NewEvent("Clamber Grab Start");
    On_clamberGrabStart->GoToStateOnEvent = st_clamber_acquireP1;
    On_clamberGrabStart->updateMode = KFSMUpdateMode::UPDATE;
    On_clamberGrabStart->OnCheckCondition = COND({
        Vessel* vessel = VesselPtr();
        if (vessel->heightFromTerrain == -1.0f) return false;
        if (((!(vessel->terrainAltitude < 0.0) || !vessel->mainBody()->ocean) ? static_cast<double>(vessel->heightFromTerrain) : vessel->altitude) > fx(clamberMaxAlt())) return false;
        if (PartPlacementMode()) return false;
        if (FindClamberSrf(clamberStandoff(), clamberReach()) && VesselUnderControl() && TestClamberSrf(clamberHitInfo)) {
            if (currentState != st_ladder_idle && currentState != st_ladder_climb && currentState != st_ladder_end_reached)
                PostInteractionScreenMessage(cacheAutoLOC_114297);
            else
                PostInteractionScreenMessage(cacheAutoLOC_114293);
            if (Key(EvaKey::Use, KeyQuery::Held)) {
                clamberPath = GetClamberPath(clamberStandoff(), clamberReach());
                return clamberPath != nullptr;
            }
        }
        return false;
    });
    fsm->AddEvent(On_clamberGrabStart, {st_idle_gr, st_idle_b_gr, st_walk_acd, st_walk_fps, st_run_acd, st_run_fps, st_jump, st_land, st_bound_fl, st_idle_fl,
                                        st_swim_idle, st_swim_fwd, st_ladder_climb, st_ladder_idle, st_ladder_end_reached});

    On_clamberP1 = NewEvent("Clamber Reach P1");
    On_clamberP1->GoToStateOnEvent = st_clamber_acquireP2;
    On_clamberP1->OnCheckCondition = COND(return clamberPath != nullptr && fx(Animations().clamber.State()->normalizedTime) >= fx(0.2f););
    fsm->AddEvent(On_clamberP1, {st_clamber_acquireP1});

    On_clamberP2 = NewEvent("Clamber Reach P2");
    On_clamberP2->GoToStateOnEvent = st_clamber_acquireP3;
    On_clamberP2->OnCheckCondition = COND(return clamberPath != nullptr && fx(Animations().clamber.State()->normalizedTime) >= fx(0.3f););
    fsm->AddEvent(On_clamberP2, {st_clamber_acquireP2});

    On_clamberP3 = NewEvent("Clamber Reach P3");
    On_clamberP3->GoToStateOnEvent = st_idle_gr;
    On_clamberP3->OnCheckCondition = COND(return clamberPath != nullptr && (fx(Animations().clamber.State()->normalizedTime) >= fx(Animations().clamber.end) ||
                                                                           fsm->TimeAtCurrentState() > fx(Animations().clamber.State()->length) * fx(0.5f)););
    fsm->AddEvent(On_clamberP3, {st_clamber_acquireP3});

    On_boardPart = NewEvent("Boarding Part");
    On_boardPart->updateMode = KFSMUpdateMode::UPDATE;
    On_boardPart->OnCheckCondition = COND({
        if (currentAirlockPart != nullptr && VesselUnderControl() && !EVAConstructionModeController::MovementRestricted() &&
            (currentState == st_ladder_idle || currentLadderTriggers.empty())) {
            PostInteractionScreenMessage(cacheAutoLOC_114358);
            if (Key(EvaKey::Board, KeyQuery::Up)) return true;
        }
        return false;
    });
    On_boardPart->OnEvent = ONEV({
        On_boardPart->GoToStateOnEvent = fsm->CurrentState();
        BoardPart(currentAirlockPart);
    });
    fsm->AddEvent(On_boardPart, {st_idle_fl, st_idle_gr, st_idle_b_gr, st_ladder_idle, st_swim_idle});

    On_flagPlantStart = NewEvent("Flag Plant Started");
    On_flagPlantStart->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_flagPlantStart->GoToStateOnEvent = st_flagAcquireHeading;
    fsm->AddEvent(On_flagPlantStart, {st_idle_gr, st_idle_b_gr});

    On_flagPlantHdgAcquire = NewEvent("Flag Plant Heading Acquired");
    On_flagPlantHdgAcquire->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_flagPlantHdgAcquire->GoToStateOnEvent = st_flagPlant;
    On_flagPlantHdgAcquire->OnCheckCondition = COND(return fx(Mathf::Abs(f32store(fx(deltaHdg) + fx(lastDeltaHdg)))) < fx(30.0f););
    fsm->AddEvent(On_flagPlantHdgAcquire, {st_flagAcquireHeading});

    On_flagPlantComplete = NewTimedEvent("Flag Plant Complete", Animations().flagPlant.State()->length);
    On_flagPlantComplete->GoToStateOnEvent = st_idle_gr;
    fsm->AddEvent(On_flagPlantComplete, {st_flagPlant});
}

// KerbalEVA.cs:4190-4352
void KerbalEVA::SetupFSMEvents2() {
    On_seatBoard = NewEvent("Seat Board");
    On_seatBoard->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_seatBoard->GoToStateOnEvent = st_seated_cmd;
    On_seatBoard->OnEvent = ONEV({
        Vessel* vessel = VesselPtr();
        Part* p = PartPtr();
        if (kerbalSeat == nullptr) throw Nre();
        vessel->SetRotation(kerbalSeat->SeatPivotRotation(), false);
        vessel->SetPosition(Vector3d(kerbalSeat->SeatPivotPosition().x, kerbalSeat->SeatPivotPosition().y, kerbalSeat->SeatPivotPosition().z), true);
        vessel->IgnoreGForces(10);
        p->Couple(kerbalSeat->SeatPart());
        vessel = VesselPtr();  // `base.vessel` after Part.Couple: the seat's vessel (the kerbal's own was merged into it)
        FlightGlobals::ForceSetActiveVessel(vessel);
        EvaEnvironment::Effect("FlightInputHandler.ResumeVesselCtrlState");
        if (FlightRegistry::ActiveVessel != nullptr && FlightRegistry::ActiveVessel->id == vessel->id) {
            isActiveVessel = true;
            EvaEnvironment::Effect("portrait:start");
        }
        if (vessel->CurrentControlLevel() == ControlLevel::NONE) MakeReference();
        Engine().SetRbDrag(0.0f);
    });
    fsm->AddEvent(On_seatBoard, {st_idle_fl, st_idle_gr, st_idle_b_gr, st_swim_idle, st_ladder_idle});

    On_seatDeboard = NewEvent("Seat Deboard");
    On_seatDeboard->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_seatDeboard->GoToStateOnEvent = st_idle_fl;
    On_seatDeboard->OnEvent = ONEV({
        EjectFromSeat();
        FlightGlobals::ForceSetActiveVessel(VesselPtr());
        EvaEnvironment::Effect("FlightInputHandler.SetNeutralControls");
        if (evaChute != nullptr && evaChute->deploymentState == ModuleParachute::deploymentStates::SEMIDEPLOYED) On_seatDeboard->GoToStateOnEvent = st_semi_deployed_parachute;
        else if (evaChute != nullptr && evaChute->deploymentState == ModuleParachute::deploymentStates::DEPLOYED) On_seatDeboard->GoToStateOnEvent = st_fully_deployed_parachute;
        else On_seatDeboard->GoToStateOnEvent = st_idle_fl;
    });
    fsm->AddEvent(On_seatDeboard, {st_seated_cmd});

    On_seatEject = NewEvent("Seat Eject");
    On_seatEject->updateMode = KFSMUpdateMode::LATEUPDATE;
    On_seatEject->GoToStateOnEvent = st_ragdoll;
    On_seatEject->OnCheckCondition = COND(return PartPtr()->parent == nullptr;);
    fsm->AddEvent(On_seatEject, {st_seated_cmd});

    On_grapple = NewEvent("Grapple");
    On_grapple->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_grapple->GoToStateOnEvent = st_grappled;
    On_grapple->OnEvent = ONEV();
    fsm->AddEventExcluding(On_grapple, {st_grappled});

    On_grappleRelease = NewEvent("Grapple Release");
    On_grappleRelease->updateMode = KFSMUpdateMode::LATEUPDATE;
    On_grappleRelease->GoToStateOnEvent = st_ragdoll;
    On_grappleRelease->OnCheckCondition = COND(return PartPtr()->parent == nullptr;);
    fsm->AddEvent(On_grappleRelease, {st_grappled});

    On_semi_deploy_parachute = NewEvent("Semi-Deploy Parachute");
    On_semi_deploy_parachute->OnCheckCondition = COND({
        if (evaChute != nullptr && evaChute->isEnabled && evaChute->deploymentState == ModuleParachute::deploymentStates::STOWED && Key(EvaKey::ChuteDeploy, KeyQuery::Held) && VesselUnderControl()) {
            if (JetpackDeployed()) EvaEnvironment::postedMessages.push_back(Localizer::Format("#autoLOC_8004227"));
            else evaChute->Deploy();
        }
        return false;
    });
    On_semi_deploy_parachute->GoToStateOnEvent = st_semi_deployed_parachute;
    fsm->AddEvent(On_semi_deploy_parachute, {st_ragdoll, st_idle_fl});

    On_fully_deploy_parachute = NewEvent("Fully-Deploy Parachute");
    On_fully_deploy_parachute->GoToStateOnEvent = st_fully_deployed_parachute;
    On_grapple->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;  // :4275 (sic: On_grapple again)
    fsm->AddEvent(On_fully_deploy_parachute, {st_semi_deployed_parachute});

    On_parachute_cut = NewEvent("Parachute Cut");
    On_parachute_cut->GoToStateOnEvent = st_idle_fl;
    On_grapple->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;  // :4279 (sic)
    fsm->AddEvent(On_parachute_cut, {st_semi_deployed_parachute, st_fully_deployed_parachute});

    if (EvaEnvironment::IsInstalled("Serenity")) {
        On_chopping_roc = NewEvent("Chopping Roc");
        On_chopping_roc->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
        On_chopping_roc->GoToStateOnEvent = st_picking_roc_sample;
        fsm->AddEvent(On_chopping_roc, {st_idle_gr, st_idle_b_gr, st_walk_acd, st_walk_fps, st_heading_acquire, st_run_acd, st_run_fps, st_bound_gr_acd,
                                        st_idle_fl, st_bound_gr_fps, st_ladder_idle, st_picking_roc_sample});
    }
    if (EvaEnvironment::IsInstalled("Serenity")) {
        On_roc_sample_stored = NewEvent("Roc Sample Stored");
        On_roc_sample_stored->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
        On_roc_sample_stored->GoToStateOnEvent = st_idle_gr;
        fsm->AddEvent(On_roc_sample_stored, {st_idle_gr, st_picking_roc_sample});
    }

    On_constructionModeEnter = NewEvent("Construction Mode Entered");
    On_constructionModeEnter->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_constructionModeEnter->GoToStateOnEvent = st_enteringConstruction;
    On_constructionModeEnter->OnCheckCondition = COND(return ((InConstructionMode && weldFX != nullptr && !weldFX->meshEnabled) || (InConstructionMode && weldFX == nullptr)) ? true : false;);
    fsm->AddEvent(On_constructionModeEnter, {st_exitingConstruction, st_idle_gr, st_idle_b_gr, st_walk_acd, st_walk_fps, st_run_acd, st_run_fps,
                                             st_heading_acquire, st_idle_fl});

    On_constructionModeExit = NewEvent("Construction Mode Exited");
    On_constructionModeExit->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_constructionModeExit->GoToStateOnEvent = st_exitingConstruction;
    fsm->AddEvent(On_constructionModeExit, {st_enteringConstruction, st_idle_gr, st_idle_b_gr, st_walk_acd, st_walk_fps, st_run_acd, st_run_fps,
                                            st_heading_acquire, st_idle_fl});

    On_constructionModeTrigger_gr_Complete = NewEvent("Construction Mode Toggle finished on ground");
    On_constructionModeTrigger_gr_Complete->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_constructionModeTrigger_gr_Complete->GoToStateOnEvent = st_idle_gr;
    fsm->AddEvent(On_constructionModeTrigger_gr_Complete, {st_enteringConstruction, st_exitingConstruction});

    On_constructionModeTrigger_fl_Complete = NewEvent("Construction Mode Toggle finished on space");
    On_constructionModeTrigger_fl_Complete->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_constructionModeTrigger_fl_Complete->GoToStateOnEvent = st_idle_fl;
    fsm->AddEvent(On_constructionModeTrigger_fl_Complete, {st_enteringConstruction, st_exitingConstruction});

    On_weldStart = NewEvent("Weld Started");
    On_weldStart->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_weldStart->GoToStateOnEvent = st_weldAcquireHeading;
    fsm->AddEvent(On_weldStart, {st_idle_gr, st_idle_b_gr, st_idle_fl});

    On_weldHdgAcquired = NewEvent("Weld Heading Acquired");
    On_weldHdgAcquired->updateMode = KFSMUpdateMode::FIXEDUPDATE;
    On_weldHdgAcquired->GoToStateOnEvent = st_weld;
    On_weldHdgAcquired->OnCheckCondition = COND({
        if (constructionTarget == nullptr) throw Nre();
        const Vector3f toTarget = constructionTarget->partTransform.position - transform().position;
        const Vector3f normalized = toTarget.normalized();
        return fx(Vector3f::Angle((normalized - transform().up() * Vector3f::Dot(normalized, transform().up())).normalized(), transform().forward())) < fx(3.0f) ||
               fsm->TimeAtCurrentState() > 2.0;
    });
    fsm->AddEvent(On_weldHdgAcquired, {st_weldAcquireHeading});

    On_weldComplete = NewTimedEvent("Weld Part Complete", Animations().weld.State()->length);
    On_weldComplete->GoToStateOnEvent = st_idle_gr;
    fsm->AddEvent(On_weldComplete, {st_weld});

    On_Playing_Golf = NewEvent("Playing Golf");
    On_Playing_Golf->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_Playing_Golf->GoToStateOnEvent = st_playing_golf;
    fsm->AddEvent(On_Playing_Golf, {st_idle_gr, st_idle_b_gr, st_walk_acd, st_walk_fps, st_heading_acquire, st_run_acd, st_run_fps, st_bound_gr_acd,
                                    st_idle_fl, st_bound_gr_fps, st_ladder_idle, st_playing_golf});
    On_Golf_Complete = NewEvent("Golf Complete");
    On_Golf_Complete->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_Golf_Complete->GoToStateOnEvent = st_idle_gr;
    fsm->AddEvent(On_Golf_Complete, {st_idle_gr, st_playing_golf});

    On_Smashing_Banana = NewEvent("Smashing Banana");
    On_Smashing_Banana->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_Smashing_Banana->GoToStateOnEvent = st_smashing_banana;
    fsm->AddEvent(On_Smashing_Banana, {st_idle_gr, st_idle_b_gr, st_walk_acd, st_walk_fps, st_heading_acquire, st_run_acd, st_run_fps, st_bound_gr_acd,
                                       st_idle_fl, st_bound_gr_fps, st_ladder_idle, st_smashing_banana});
    On_Banana_Complete = NewEvent("Banana Complete");
    On_Banana_Complete->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_Banana_Complete->GoToStateOnEvent = st_idle_gr;
    fsm->AddEvent(On_Banana_Complete, {st_idle_gr, st_smashing_banana});

    On_Spinning_Wingnut = NewEvent("Spinning Wingnut");
    On_Spinning_Wingnut->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_Spinning_Wingnut->GoToStateOnEvent = st_spinning_wingnut;
    fsm->AddEvent(On_Spinning_Wingnut, {st_idle_fl, st_idle_b_gr, st_walk_acd, st_walk_fps, st_heading_acquire, st_run_acd, st_run_fps, st_bound_gr_acd,
                                        st_idle_fl, st_bound_gr_fps, st_ladder_idle, st_spinning_wingnut});
    On_Wingnut_Complete = NewEvent("Wingnut Complete");
    On_Wingnut_Complete->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    On_Wingnut_Complete->GoToStateOnEvent = st_idle_fl;
    fsm->AddEvent(On_Wingnut_Complete, {st_idle_fl, st_spinning_wingnut});
}

// snapshot of the registrations (unit gate)
void KerbalEVA::RecordEventWiring() {
    eventWiring_.clear();
    for (auto& ev : ownedEvents_) {
        EvaEventWiring w;
        w.gotoState = ev->GoToStateOnEvent != nullptr ? ev->GoToStateOnEvent->name : std::string();
        w.updateMode = static_cast<int>(ev->updateMode);
        if (auto* t = dynamic_cast<KFSMTimedEvent*>(ev.get())) {
            w.timed = true;
            w.timerDuration = t->TimerDuration;
        }
        for (KFSMState* st : fsm->GetStates())
            for (KFSMEvent* e : st->StateEvents())
                if (e == ev.get()) w.inStates.push_back(st->name);
        eventWiring_[ev->name] = w;
    }
    for (KFSMState* st : fsm->GetStates()) stateWiring_[st->name].updateMode = static_cast<int>(st->updateMode);
}

}  // namespace ksp
