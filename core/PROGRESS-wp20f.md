# PROGRESS WP-20f: SAS / Vessel.FeedInputFeed in the physics tick, gimbal actuation, longer ascent gate

Branch `wp20f` (worktree `.worktrees/wp20f`, from main 51530d89). Optimizer (Opus). Log names contain `wp20f`.

## Findings from reading (2026-10-20)

- Measured order (analysis/physics/raw/run1/run1-L_calltrace.csv): seq 18 FlightInputHandler.FixedUpdate (NOT 11 as the bubble
  comment said), 19 TimeWarp, 22 Part.FixedUpdate, 23 ModuleCommand, 24 ModuleReactionWheel, 25 ModuleSAS, then per engine part
  ModuleEngines (27) -> ModuleGimbal (28): the engine reads the gimbal pose of the PREVIOUS tick.
- Vessel.cs:2035 FeedInputFeed: OnPre / OnAutopilot / OnPost / OnFlyByWire, then rootPart.propagateControlUpdate if loaded && !packed
  && !physicsHoldLock && isControllable. Part.onCtrlUpd (Part.cs:4626) is empty and no Assembly-CSharp class overrides it.
- FlightInputHandler.FixedUpdate (FlightInputHandler.cs:377) copies `state` into the active vessel (SetControlState) every tick, then
  FeedInputFeed per unpacked loaded vessel. WP-50 port has the hand-off; feedInputFeed was a host hook (unset = nothing).
- VesselSAS.KillAngularVelocity (VesselAutopilot.cs:672) zeroes every part rb.angularVelocity when the decay locks: hook
  AutopilotEnvironment::killAngularVelocity must be wired by the bubble.
- C10 (harness/captures/out2/aero_forces_vessel.json): SAS on via the SAS action group, FlightInputHandler.state.mainThrottle = 1, a
  FullThrottle OnFlyByWire callback, StageManager.ActivateNextStage. M2 reaches alt_10000 f2088, alt_20000 f3019, final f4500 at 36.7 km;
  L2-AS alt_10000 f3193 (v 140, burning out), apex 11.5 km; S tumbles at Mach 1 (KSP too).
- Engines in the crafts: liquidEngineMainsail.v2 (gimbalTransformName `gimbal`, parent of thrustTransform) and liquidEngine2 (LV-T45
  v1, gimbalTransformName = thrustTransform itself). Both chains are in core/data/engine_thrust_transforms.json.

## Log

1. Ported: Vessel.FeedInputFeed + OnPre/OnPost/OnFlyByWire delegates, Part.propagateControlUpdate/onCtrlUpd, FlightInputHandler's
   default feed = Vessel::FeedInputFeed, AutopilotPitch/Roll/YawInput (VesselSAS.Check*Input bodies). Bubble: SetFlightInputHandler
   (seq 18 slot), KillAngularVelocity / activeVesselPYR / pitch-roll-yaw input / precisionMode hooks while a FIH is set,
   VesselPhysics::SetPartAngularVelocity. Module slot: ModuleCommand, ModuleSAS, ModuleGimbal (model transform chains, thrust follows
   the gimbal one tick later, exact at neutral), chain cache per AvailablePart.
2. SAS would not engage: ModuleSAS.OnStart (PartValues registration), Part.Start's overrideSkillUpdate (:1915) and Part.Update's
   PartValues refresh (:4328) were missing. Added overrideSkillUpdate to StartPartsInFlight and PhysicsBubble::PartValuesUpdate (Update,
   before Vessel.Update). The module lifecycle (ModulesOnStart / OnStartFinished, first ModuleJettison.LateUpdate) is the host's:
   test helper wp20d::StartModules.
3. FINDING: with the module lifecycle the drag-cube weights equal C10's live WeightedArea for 21/21, 153/153, 457/457 parts (S, M2,
   L2-AS); without it (WP-20d's PadScene) 17/21, 129/153, 385/457 (Rockomax32.BW variant cube, Mainsail variant cube via ModuleJettison,
   LV-T45 Clean cube). With it the ascent matches C10 to 0.1 % up to 20 km and the 90 s state.
4. Gates (WP-20d tolerances, unchanged): M2 8 states to alt_20000 + final 36.7 km (alt +0.042 %, speed +0.084 %), L2-AS 5 states to
   alt_10000 + apex (-0.16 %), S to Mach 1 (alt +1.40 %); past Mach 1 S tumbles in both (KSP aoa 172.7 deg at 2.5 km, ours 57.7 deg).
5. PERF: UsePortable() called getenv on every NativeSinf/Cosf/Powf (Windows): AngleAxis 2.3 us -> 0.007 us; cached once per process.
   SAS path on L2-AS ~0.24 ms per tick (UpdateVesselTorque 0.155, PoseGimbalTransforms 0.08); gimbal re-posing gated to FBW vessels.
6. Next: ctest (running), random orders, WSL syntax check, merge main, from-scratch build, docs (STATUS WP-20f, README rows), commit.
