---
name: wp42-ksc-lessons
description: WP-42 (KSC / launch sites / scene flow) facts: float32 PQSCity position, x87 Cos vs Python math at poles, duplicate enum, test fixture traps, data gaps
metadata:
  type: project
---

- `PQSCity.Orientate` stores `transform.localPosition` as float32: the KSC lat/lon the game uses comes from `(float)(Vector3.normalized(repositionRadial) * (radius + offset))`, then `SpaceCenter.Start` uses GetLatitudeAndLongitude (const 57.295780181884766), PQSCity.Orientate uses GetLatLonAlt (180/pi). WP-36's old KSC fixture (double-normalised repositionRadial) was ~6 m off; WP-42 regenerated the `recovery` section of wp36_reference.json.
- `ksp::dotnet::Math::Cos/Sin` are x87 fcos/fsin (Mono on x64 Windows): near pi/2 they differ from Python `math.cos` by 1e-4 relative (Moho/Mohole lat 90: 6.1230e-17 vs 6.1232e-17). A Python reference using math.cos cannot be bit-exact at poles; exclude and document, do not loosen.
- `enum class SpaceCenterFacility` was defined in both career/upgradeables.hpp (WP-36) and editor/editor_types.hpp (WP-40): moved to save/save_enums.hpp; check for similar duplicate enums when a new TU includes two packages' headers.
- pqs_prefab.json has duplicate GameObject names (5x Kerbin/CommNetDish): identify cities by `modIndex`, not name.
- Real data available for KSC: the 39 DestructibleBuilding ids in physprofile persistent.sfs ScenarioDestructibles; PQSCity/PQSCity2 serialized fields in pqs_prefab.json (generate C++ tables with tools/wp42/gen_wp42_cities.py). NOT extracted: PSystemSetup.facilities / stocklaunchsites, DestructibleBuilding prefab values (level scene).
- Test traps: PhysicsGlobals.BuildingImpactDamageMinDamageFraction = 100 (AddDamage needs d > 100) and the default sandbox preset has `IndestructibleFacilities = true` (set false in a fixture); `HighLogic::fetch` must exist before any `CurrentGameChecked()` call in SpaceCenterBuilding; Game() already owns `Parameters`.
- Python heredoc in the Bash tool turns `"\n"` inside a replaced source line into a real newline (broke a `.split("\n")`); use the Edit tool for string-escape edits. check_cites exact mode needs the C++ function name to equal the C# member name (rename helpers, e.g. `Start`, `SaveFolder` for a property setter, `UpdateNodeCaption`).
**Why:** each cost a debug cycle in WP-42. **How to apply:** before touching PQSCity / SpaceCenter / facility code or writing a Python reference that uses sin/cos.
