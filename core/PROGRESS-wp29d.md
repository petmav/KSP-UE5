# PROGRESS WP-29d (branch wp29d, from main 51530d89)

Plan: (1) local sweeps, (2) real wheel / leg geometry from the .mu models, (3) landing gear + legs, steering circle, runway, brake anchor, placement.

## Step 1: stock-part collider sweep (done)
- `sweep_stock_parts.py --game C:/Projects/KSP-UE5/decompiled/KSP-dev` ran (python -I works on the game dir, Read/ls do not). 383 parts, 12 failures:
  4 trigger-only models (smallCargoContainer, MiniDrill, vernierEngine, largeAdapter: the live part has ONE collider and it is a trigger),
  5 no-model prefab-built parts (flag, 4 kerbalEVA variants), 3 zDeprecated parts whose cfg points at a model file that does not exist
  (solidBooster_sm, ServiceBay_125, ServiceBay_250). Script fixed: live cfgs win the part name over Squad/zDeprecated duplicates.
  Output: core/data/mucollider_stock_counts.json.

## Step 2: geometry (in progress)
- staged 13 wheel / gear / leg parts: core/tools/wp29d/stage_wheel_models.py -> core/data/mu_staging_wheels (gitignored) and reduced committed
  copies core/data/wheel_models/*.mu + MANIFEST.json (origin + sha256).
- mu_model.hpp keeps WheelCollider chunk values + Animation chunk; new PartModelRig (parts/part_model_rig.*).
- WheelModelLibrary (physics/wheel_models.*), WheelPhysicsOptions::models: the host builds a PartModelRig per wheel part; FindModelTransform / animations / collider transform come from it; hub colliders weld into the cluster (suspension / caliper / wheel subtrees and the module-disabled suspensionCollider are left out: they move with the suspension in KSP).
- Rover scenarios on real geometry (test_vessel_wheels.cpp "WP-29d rover*"): rest compression 0.50797 / 0.53108 (proxy 0.50852 / 0.53119), forces = weight; drive v(10/20 s) 19.4123 / 28.3915 and 7.9508 / 9.8942 (proxy 19.4188 / 28.3923, 7.9548 / 9.8945); brake 7.50 m / 8.33 m (proxy 7.38 / 8.30; ref 6.08 / 8.04), tilt 20.1 / 17.5 deg (proxy 11.6 / 13.0); steer yaw -0.1925 vs kinematic -0.1935. Bug-E braked rest drift 54 mm / 20 s (pitch 1.5 deg, proxy 0.3 mm).
- DONE (c7fc4f43): all WP-29d work, from-scratch ctest 50/50. Follow-up: merge origin/main (WP-12c), pr_review.py part_model_rig.cpp:266, runway on real KSC collider if cheap.
