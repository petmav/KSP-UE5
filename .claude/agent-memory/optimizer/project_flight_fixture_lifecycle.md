---
name: flight-fixture-lifecycle
description: WP-20d PadScene skips the module lifecycle; without wp20d::StartModules drag-cube weights differ from KSP and SAS cannot engage
metadata:
  type: project
---

`tests/flight_integrator_fixture.hpp` PadScene places crafts without Part.Start's ModulesOnStart / OnStartFinished or the first
ModuleJettison.LateUpdate. WP-20f added `wp20d::StartModules(v, state)` (opt-in): with it every part's WeightedArea equals C10's live
values bit for bit (S 21/21, M2 153/153, L2-AS 457/457) and the C10 ascent matches to ~0.1 % up to 36.7 km; without it 4 / 24 / 72 parts
fly default weights and ModuleSAS never registers, so CanEngageSAS is false.
**Why:** WP-20d's gates were frozen on the default weights (their numbers must not move), so the helper was not folded into PadScene.
**How to apply:** any new flight scenario that compares with captures should call StartModules after Place; folding it into PadScene
(and re-baselining WP-20d's messages) is an open follow-up. See [[shell-quirks-wp20f]] for measuring on this laptop.
