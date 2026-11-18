# WP-UE-10 progress (part rendering)

Branch `wpue10` (started on top of `wpue07`, main `f37c3254` merged). Inline in the orchestrating session, 2026-11-18.
Results and numbers: `STATUS.md` "Part rendering, first half"; core side: `core/STATUS.md` "WP-UE-10, core side".

## First half (this PR, progress 0.6)

- [x] Core: `kspcore_game.h` "part models" (rig, model files, texture replacements, texture file); ctest `capi.part_rig`.
- [x] Facade `KspModel.h` (`FKspModels`): rig, `.mu` render data, texture header, all converted to UE axes and centimetres.
- [x] `UKspPartAssets`: static meshes, textures (DDS / MBM / PNG / TGA), dynamic material instances, part templates.
- [x] `KspCreateMaterials` commandlet and `Scripts/create_materials.ps1`; `Content/Ksp/Materials`.
- [x] `UKspSimSubsystem`: part actors, sun light; the screenshot waits for the shader compiler.
- [x] `KspUe5.Parts.StockSweep` (all 362 parts): 0 unknown shaders, 0 missing textures, winding 99.97 %.
- [x] Screenshot `slice/shots/wpue10_parts_pad.png` (Kerbal X on the pad).

## Second half (not started)

1. Core: a model rig per loaded part in the host (`mu::PartModelRig` exists; the wheel host already keeps one per wheel part), the
   modules' transform seams wired to it (`ModulePartVariants` GAMEOBJECTS, `ModuleJettison` shrouds, `ModuleGimbal` transforms,
   parachutes, animations), and the changed nodes exported: visibility and local pose (`kspcore_node_pose`, reserved in the snapshot).
2. Shell: apply node visibility and poses to the part actors (the template keeps one entry per rig node, so a node index or name maps
   straight to a component); variant texture and material changes.
3. Acceptance left: gimbals visibly deflect; M2 and Kerbal X look as in the game (shrouds, variants); `slice/shots/parts_pad.png`.
4. Smaller: skinned renderers (22 in the stock parts), mip chains for uncompressed textures, part cfg `texture` / `normalmap` overrides,
   `_SpecMap`.

## Things that cost time

- A material whose graph fails to compile draws as the engine's grey default and only logs a warning: read `Saved/Logs/fly.log` for
  `Failed to compile Material` after changing the generator. A collection vector parameter is a float4: mask it before multiplying
  with a float3.
- Shaders of a new master compile in the background on the first run: a screenshot taken too early shows the default material.
- A heredoc passed through the session's Bash tool can fail on long C++ snippets and loses one level of backslashes: write edit scripts
  with the Write tool and run them.
