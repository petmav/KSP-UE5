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
