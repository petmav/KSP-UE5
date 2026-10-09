# Embedded third-party code in KSP 1.12.5: what it is for, and what to do with it

Generated 2026-10-08 by the tiering analysis (`analysis/tiering/scripts/`). Paths are relative to
`decompiled/KSP-Decompiled/`. "Reachable" means the file is reachable from KSP-authored code through the
type-reference graph (`work/depgraph.json`, C# namespace visibility applied), which shows how much of each library is used.

These libraries appear in `tiering.csv` with tier `third-party`. They are excluded from the A/B/C totals.

## Summary

| Library | Location | Files | Lines | KSP users (files) | What KSP uses it for | Decision |
|---|---|---|---|---|---|---|
| TextMesh Pro (TMPro) | `Assembly-CSharp/TMPro/` | 121 | 27,094 | 336 | All UI text (TextMeshProUGUI labels, input fields, dropdowns) | **Drop.** UMG/Slate text replaces it |
| Smooth.Foundations | `Assembly-CSharp/Smooth/` | 104 | 11,868 | 4 | `Smooth.Pools` (`Pool<T>`, `ListPool<T>`) and `Smooth.Dispose` for allocation-free temp lists in `Orbit.cs`, `Targeting.cs`, `AudioMultiPooledFX.cs`, `SmoothDisposer.cs` | **Drop.** Use C++ object pools or reused `std::vector` |
| Edy's Vehicle Physics | `Assembly-CSharp/VehiclePhysics/` | 99 | 17,083 | 3 | Wheel simulation: `KSPWheelController : VehicleBase` with `VPWheelCollider` and `TireFriction` | **Port the used subset** into the core wheel model (about 26 files and 5k lines reachable) |
| Edy's Common Tools | `Assembly-CSharp/EdyCommonTools/` | 44 | 6,962 | 1 (+ via VP) | Maths helpers used by VehiclePhysics (`MathUtility`, `BiasedRatio`, `InterpolatedFloat`, `ColliderUtility`); one cheat screen | **Port the 4 helpers inline**, drop the rest |
| LibNoise (C# port) | `Assembly-CSharp/LibNoise/` | 45 | 2,762 | 14 | Coherent noise for PQS terrain height and colour mods | **Port bit-exact** (the 11 reachable files, about 940 lines) |
| Soft Mask for Unity UI | `Assembly-CSharp/SoftMasking/` | 22 | 2,427 | 1 | Soft-edged mask on the navball burn vector (`NavBallBurnVector.cs`) | **Drop.** Use a UMG Retainer Box or material mask |
| UniLinq | `Assembly-CSharp/UniLinq/` | 13 | 3,642 | 11 | AOT-safe LINQ (`ToArray`, `ToList`, `Max`, `Count`, `Where`, `OrderBy`) in `FlightGlobals`, `GameDatabase`, `KSPUtil`, `ContractDefs`, gizmos, `UI_ChooseOption`/`UI_Cycle` | **Drop.** Use C++ algorithms/ranges at each call site |
| DDS headers | `Assembly-CSharp/DDSHeaders/` | 11 | 351 | 1 | DDS header structs for `DatabaseLoaderTexture_DDS.cs` | **Replace** with UE texture import, or DirectXTex if loading the user's KSP install at runtime |
| Highlighting System | `Assembly-CSharp/Highlighting/` | 2 | 1,580 | 16 | Part outline/glow highlighting (`Part.highlighter`, editor, cameras, inventory, settings PPFX) | **Replace** with Custom Depth/Stencil plus a post-process outline material |
| Vectrosity | `Assembly-CSharp-firstpass/Vectrosity/` | 20 | 7,892 | 13 | Screen-space and 3D line drawing: orbit lines (`OrbitRendererBase`, `PatchRendering`, `MapView`), CommNet links, R&D tree connectors, mission editor connectors, fuel-flow overlay, FPS graph | **Replace** with Line Batcher or Procedural Mesh ribbons for 3D lines, and UMG/Slate line drawing for 2D |
| Steamworks.NET | `Assembly-CSharp-firstpass/Steamworks/` | 364 | 18,576 | 13 | Workshop UGC (craft and mission sharing), overlay links, persona name, Steam IDs | **Drop** for the private build. Online Subsystem Steam is available later if wanted |

Notes:
- `INDEX.md` lists `Highlighting/` as embedded third-party but the assignment brief does not. I treated it as third-party.
- `iTween.cs` and `iT.cs` (Bob Berkebile's iTween tweening library, 5,695 lines) sit in the root directory and are third-party too. They stay in the KSP rows of `tiering.csv` (tier C, drop or use UMG animations) because they are not in a separate namespace.
- `Assembly-CSharp-firstpass` root utilities (the `OBJ`/`DAE` importers, `ProgMesh`, `FileIO`, `FileGUI`, the sample camera scripts) look like bundled third-party scripts. They are in `tiering.csv` as tier C (Rendering/Camera, mostly `drop`).
- `Lingoona` (firstpass) is a P/Invoke wrapper over the native `lingoona_grammar_kerbal.dll` grammar engine used by `Localizer`. Decision: drop, and use UE text formatting (FText argument modifiers cover gender and plural) behind a small format shim.

## Per-library detail

### LibNoise: port bit-exact (terrain correctness depends on it)
- Users: `PQSMod_VertexHeightNoise*` (5 variants), `PQSMod_VertexNoise`, `PQSMod_VertexPlanet`, `PQSMod_VertexRidgedAltitudeCurve`, `PQSMod_VertexVoronoi`, `PQSMod_VoronoiCraters`, `PQSMod_VoronoiCraters2`, `PQSMod_VertexColorNoise`, `PQSMod_VertexColorNoiseRGB`, `PQSMod_VertexHeightNoiseHeightMap`.
- Types used: `RidgedMultifractal` (29 references), `Perlin` (20), `Billow` (20), `NoiseQuality` (20), `Voronoi` (10), `IModule`, plus `Select` and `ScaleBiasOutput` from `Modifiers`.
- Reachable files (11): `Billow`, `GradientNoiseBasis`, `IModule`, `Math`, `Modifiers/ScaleBiasOutput`, `Modifiers/Select`, `NoiseQuality`, `Perlin`, `RidgedMultifractal`, `ValueNoiseBasis`, `Voronoi`.
- Why port instead of replacing: every Kerbol-system body's terrain height comes from these functions with config seeds. Any change moves the terrain, which breaks landed saves, KSC placement, PQSCity flattening and scatter positions. The C# code is small and pure (no Unity types), so tier A transpilation is cheap. Keep the C# integer hashing, seed handling and double/float precision exactly. The original C++ libnoise is LGPL and close but not guaranteed identical. Port KSP's version.
- Acceptance: golden heights at a fixed lat/lon grid per body (`terrain_heights` golden, TIERING.md WP-11).
- KSP's own `Simplex.cs` and `SimplexNoise.cs` (root, not LibNoise) drive the `PQSMod_VertexSimplex*` mods. They are KSP code, tier A.

### VehiclePhysics + EdyCommonTools: port the subset that wheels use
- `KSPWheelController.cs` derives from `VehiclePhysics.VehicleBase` and creates a `VPWheelCollider` per wheel with a `TireFriction` model. `ModuleWheelBase.cs` drives it. `Vessel.cs` only checks for `VPWheelCollider` on colliders.
- `VPWheelCollider.SetupWheelCollider()` sets the PhysX `WheelCollider` forward and sideways friction stiffness to 0. It then uses the native WheelCollider only for suspension (raycast, spring/damper, `sprungMass`) and applies its own tyre forces from `TireFriction` (Flat/Lineal/Smooth/Parametric/Pacejka curves).
- Reachable set: 26 files, about 5,000 lines: `VehicleBase`, `VehicleBehaviour`, `Block`, `Solver`, `DataBus`, `VPWheelCollider`, `TireFriction`, `DirectDrive`, `GroundMaterial*`, plus camera/marks/particle helpers pulled in by `VehicleBase` references that KSP does not use at runtime. EdyCommonTools adds `MathUtility`, `BiasedRatio`, `InterpolatedFloat`, `ColliderUtility`.
- Decision: port `TireFriction`, the `VPWheelCollider` force pipeline and the minimal `VehicleBase` solver loop into the core wheel model (tier B work, WP-29 in TIERING.md). Do not use Chaos Vehicles, because its tyre model and suspension differ.
- Fidelity gap: PhysX's WheelCollider internals (suspension raycast, sprung-mass distribution via the PhysX vehicle SDK, contact handling) are native code and not in the decompilation. The core needs its own raycast suspension (spring-damper with `targetPosition`, `sprungMass`, `forceAppPointDistance`), tuned against golden rover/landing-gear scenarios captured from KSP.

### Smooth.Foundations: drop
- 104 files, but only `Smooth.Pools` and `Smooth.Dispose` are reachable (7 files, about 230 lines): `ListPool<Targeting.Sample>` in `Orbit.cs`, pools of `Conic`/`Sample`/`Interval` in `Targeting.cs`, a pooled AudioSource in `AudioMultiPooledFX`.
- `Slinq`, `Algebraics` (Option/Either/Tuple) and the rest are unused by KSP-authored code.

### UniLinq: drop
- A LINQ-to-objects reimplementation (Unity AOT safety). The C++ port replaces each call with an STL algorithm or range. No behaviour to keep beyond ordering stability: `OrderBy` is a stable sort, so use `std::stable_sort`.

### TMPro and SoftMasking: drop
- TMPro is consumed by 336 KSP files, all UI (tier C). UMG text with Slate font rendering covers it. Rich-text tags used in KSP strings (`<color>`, `<b>`, `<sprite>`) need a UMG rich-text decorator set. Note this in the UI work packages.
- SoftMasking is only used for the navball burn-vector mask.

### Highlighting: replace
- `Highlighter` component per part: outline colours for hover, selection, staging, inventory and editor states, plus a camera post-effect (`HighlightingSystem`). In UE5: Custom Depth/Stencil values per state and one outline post-process material. The highlight state machine (which colour when) lives in `Part.cs`/editor code and stays in the shell.

### DDSHeaders: replace
- Only needed if the project loads textures from the user's KSP install at runtime (optional compatibility layer). Otherwise textures are imported offline with the UE editor.

### Vectrosity: replace
- Orbit lines are the main use. The UE5 map view should draw conics from the core's patched-conic data with a camera-relative ribbon mesh (double-precision positions converted per frame). 2D connectors (tech tree, mission editor) become UMG/Slate line drawing.

### Steamworks.NET: drop
- KSP uses UGC (Workshop) for craft and mission sharing, overlay web links, persona name and Steam cache paths. Achievements are not driven through Steamworks here (`KSPAchievements/` is KSP's own progress-tracking). Nothing gameplay-relevant.
