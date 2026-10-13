# PROGRESS WP-19b: frames and the physics bubble (branch `wp19b`, from local `wp20b` 950c1d94)

Implementer package. Scope: the WP-20b "Deferred to WP-19b" list (STATUS.md WP-20b "Seams"): frame-origin policy (the
floatOrigin-in-CalculatePhysicsStats frame mixing risk R1), the rotating <-> inertial switch of the bodies, Vessel.ChangeWorldVelocity
incl. krakensbaneAcc, the Krakensbane host, FloatingOrigin consistent with the reduced model (double positions, no per-tick world
shift), several vessels in one bubble (pack / unpack distances, rails transitions), vessel lifecycle of the bindings.
Not touched: WP-20c's files (load graph, Cut / Merge internals). Local commits only, no push.

## Plan
1. Frame policy (decided): ONE world frame for every float and double field (KSP's convention, floatOrigin = 0), its origin kept near
   the active vessel by FloatingOrigin's own threshold rule applied to the NON-physics state (bodies, vessels' transforms / CoMD) and to
   one double `frameOriginWorld` of the bubble; Jolt bodies never move for it. Krakensbane's frame drift stays FrameOffset (WP-20a) and
   is folded into the dynamic bodies when the frame velocity returns to zero; static geometry does not collide while FrameOffset != 0
   (it is in the body frame, the dynamic bodies are not). Physics-origin rebase (Jolt bodies) only past 1,000 km (PHYSICS-SPIKE R3).
2. PhysicsWorld: TranslateBodies (moving / static), static-collision gate. FloatingOrigin: threshold / continuous rule + host hooks.
3. Vessel::ChangeWorldVelocity / SetWorldVelocity (Vessel.cs:3197 / 3219) with krakensbaneAcc; env.setWorldVelocity seam.
4. PhysicsBubble (new, `physics/physics_bubble.hpp`): owns the VesselPhysicsHost wiring + Krakensbane + FloatingOrigin, the
   OrbitPhysicsManager seams (setRotatingFrame / setDominantBody full ports with the rigid bodies), physicalObject, lifecycle
   (unload / registry removal drop the binding), the per-tick order (FlightScene, Krakensbane, FloatingOrigin, step, fold).
5. Gates: independent Python reference (tools/wp19b) for the bit-exact parts; scenarios frame_switch, soi_change, two_vessels, world
   shift / rebase invariance, fold; random order 11 / 27; ASan / UBSan.
6. STATUS row 51 + section, README, check_cites exact.

## Log
- start: read CLAUDE.md, README, PHYSICS-SPIKE.md, STATUS WP-20a / WP-19a / WP-20b, PROGRESS-wp20b / wp19a; C# Krakensbane.cs,
  FloatingOrigin.cs, OrbitPhysicsManager.cs, Vessel.cs (ChangeWorldVelocity, SetWorldVelocity, UpdateAcceleration, SetPosition, Unload),
  physicalObject.cs, Orbit.GetVel. Worktree created, baseline build green (239 s).
- finding: kspcore's world has Kerbin at the origin (reverse orbit driver, position 0): float fields of a vessel at 680 km have a 6 cm
  ulp; KSP keeps the active vessel near the origin with FloatingOrigin. That is the R1 risk in numbers.
- finding: static meshes are in the body frame, dynamic bodies in the Galilean frame (FrameOffset): without a fold / gate a vessel
  could touch terrain that is FrameOffset away from the real terrain.
- step 1-4 done (first version): PhysicsWorld TranslateBodies / static-collision gate; FloatingOrigin threshold / continuous rule with
  the shiftWorld / activeVesselPosition / continuousCondition hooks (unset = WP-20a's behaviour); Vessel::ChangeWorldVelocity /
  SetWorldVelocity (+ env.setWorldVelocity); physicalObject; PhysicsBubble (physics_bubble.hpp/.cpp): install, frames, ShiftWorld,
  FoldFrameOffset, RebasePhysicsOrigin, SetRotatingFrame / SetDominantBody ports, Krakensbane host, lifecycle listeners, tick.
- scenario tests green (test_physics_bubble.cpp, physics_bubble_fixture.hpp): frame policy (localCoM 1.3 cm far / 1e-7 m after the
  rebase), in-tick rebases (50 in 12 s, transform <= 532 m, trajectory unchanged 1.2e-6 m), two_vessels (load 2249.9 m, unpack 199.8 m,
  pack 350.0 m, relative error 0.65 mm, rails continuation 2e-10 m), frame_switch both ways, SOI change into the Mun (+ physics-origin
  rebase at 9,600 km).
- finding: KSP's two clocks: UT += Planetarium.fixedDeltaTime 0.02 (double), physics steps Time.fixedDeltaTime 0.02f; a vessel in
  physics runs on the physics clock (1 um per tick behind the UT-clocked rails position at 2.2 km/s, 1.7 cm in 335 s).
- finding: KSP's tick order makes the frame changes of OrbitPhysicsManager.FixedUpdate (seq 280) take effect for velocities in the
  same tick but for the body / Planetarium frame only from the next Planetarium.FixedUpdate (seq 6): the rotating <-> inertial switch
  displaces the trajectory by -/+ (omega x r) dt once (3.3-4.0 m at 100 km, matched within 0.3 mm), the SOI change by -V_Mun dt
  (10.8 m, matched within 5e-6 m); before and after, the vessel follows its Kepler orbit (1e-6 .. 7e-5 m over 16 s).
- finding: the first orbitDriftFramesToWait (5) ticks after an unpack in the rotating frame run without drift compensation: 1.2 mm per
  tick of Euler error (7.5 mm total), then held.
- independent reference tools/wp19b/frames_reference.py -> tests/data/wp19b_reference.json (setRotatingFrame 300, ChangeWorldVelocity 300,
  Krakensbane through the host 400, FloatingOrigin 300 scenes): 0 ULP on the first run; setDominantBody reverse chain for every body.
- tests added: fold / static gate (6e-7 m at rest in the true frame while drifting; without the gate 20.5 m), physics-origin rebase invariance
  (1.4e-7 m), lifecycle (unload / die / destruction, reload), physicalObject.
- check_cites exact: 29 cites over physics_bubble / physical_object / floating_origin / vessel_flight / krakensbane / physics_world, 0 mismatches
  (aliases ShiftWorld, DefaultFlightIntegrator, physicalObject ctor / dtor, UpdateWantsDestroy; the bubble's ports keep the C# names
  setRotatingFrame / setDominantBody); the WP-20b files still 57 / 0.
- docs: header comments (vessel_physics.hpp frame paragraph: the decided policy; krakensbane / reference_frame / orbit_physics_manager /
  flight_globals pointers), README (bubble paragraph, unit row, layout rows), STATUS row 51 + WP-19b section.
- verification: fresh MSVC build (rm -rf core/build, run_tests.ps1 -Fresh): configure 3.3 s, build 289.5 s, ctest 38/38 (508 s); unit random
  order seeds 11 and 27: 840/840, 1,385,155 assertions each; WSL g++ 13.3 ASan+UBSan+LSan whole unit binary: 840/840, 1,385,139 assertions,
  no report, no warning in the new files. Done.
