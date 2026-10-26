# PROGRESS-wp29e (WP-29e: wheel package leftovers; branch wp29e from main 839f2f5f)

- [x] Baseline: fresh build + ctest 52/52 on main (build 413 s, test 166 s).
- [x] Item 2: retracted-gear stand-in sphere contact (tests/test_scenery_scenarios.cpp, "WP-29e Aeris 3A gear up ..." x2). Aeris 3A as a gear-up save
      (LoadRover gearRetracted), set down 2 cm above the KSC runway: nose bay rests on its stand-in (sphere bottom -2.4 mm, nearest contact 0.37 mm);
      without the stand-in the sphere would be 2.56 cm inside the runway; nose pitch change 0.3563 deg observed vs 0.3584 predicted; deploying on the
      ground: stand-in gone at Position >= TsubSys (1.0), wheels on the next tick, rest within 3.4 mm of an aircraft deployed all along.
- [x] Item 3: colliders under the suspension / caliper / wheel transforms follow the suspension. Port of the Transform writes of
      VPWheelCollider.UpdateVisualWheel (cs:275-300) + mutable Jolt compound (PhysicsWorld::SetCompoundChildPoses, VesselPhysics::SetPartColliderPoses),
      host UpdateColliders per physics step (replaces the 5 cm stand-in Rebuild steps). Gates: tools/wp29e/visual_reference.py -> tests/data/wp29e_visual_reference.json
      (2,040 poses, 130 cases, 13 models: 0 mismatches), rover rest test (8 followed colliders = m_suspensionPosition - prefab pivot to 2.5e-6 m), mutable
      compound unit test. Every WP-29c/d/21b scenario number is bit-identical to main (no following collider touches anything in them). Finding: LY-01 /
      LY-05 / LY-10 bays have NO colliders under SuspensionPivot/SteeringPivot/WheelPivot (WP-21b deferred item 4 resolves to: nothing follows; the
      WheelRetractCollider rides the animation clip, now exactly); the LT-1/2/5 legs only have the disabled piston_collider below the suspension.
      Inertia: following colliders are shapes but not in the part inertia estimate (noInertia), else Bug-E drive/rest numbers shift (v(10 s) 7.90 -> 8.04, drift 13 -> 71 mm).
- [ ] Item 4: getLowestPoint wheel-radius corrections and the bounds centre from the model data.
- [ ] Item 5: brake freeze: cluster member vs cluster weld (evaluate, document).
- [ ] Item 6: animation rotation renormalisation: measure the stored quaternions of the 13 stock clips.
- [ ] STATUS WP-29e, README, progress.csv, final verification, PR.
