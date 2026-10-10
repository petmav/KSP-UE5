# PROGRESS wp17 (Delta-v calculation)

- [x] Read spec: VesselDeltaV, DeltaVPartInfo, DeltaVEngineInfo, DeltaVStageInfo, DeltaVEngineStageSet + small files; capture deltav_readout.json tables: craft, totals(68), stages(188), engines(460), parts(1225).
- Plan: include/ksp/vessel/delta_v.hpp + src/vessel/delta_v_*.cpp; runner tools/golden_runner/ds_deltav_stages.cpp; gate capture.deltav_stages (tables stages, totals + parts columns decoupleStage/activationStage/isEngine).
- Design: synchronous RunCalculations (the coroutine yields only wait frames); two runs first (syncListInstances), one per body.
- Thrust direction: no .mu model reader -> runner installs engine thrust transforms (documented deviation).
- gate capture.deltav_stages passes: dV cols <=11 ULP (tolerance rel 2e-6, STATUS), everything else 0 ULP; exclusions: Dynawing/Learstar stage0 (omsEngine thrust direction) + totals rows. Merged main (WP-26) at 47c2cad4. TODO: citations check, unit tests, README/STATUS, fresh runs (Windows + WSL).
