# PROGRESS WP-21 (numeric aerodynamics), branch wp21

## Plan
1. `include/ksp/aero/float_eval.hpp`: evaluation policy (MonoFloat = game, NetFloat = harness .NET oracle) so one templated
   port reproduces both the in-game captures and the .NET golden bit for bit.
2. DragCubeList aero half (part_data.hpp additive + src/aero/drag_cube_list.cpp): SetPart, SetDragWeights, SetPartOcclusion,
   SetDrag/AddSurfaceDragDirection, weights, ForceUpdate, helper maths.
3. FlightIntegrator aero members (src/aero/flight_integrator_aero.cpp): CalculateDragValue*, areas, UpdateAerodynamics scalars,
   DragCubeSetupAndPartAeroStats, no rigidbody.
4. ModuleLiftingSurface / ControlSurface / AeroSurface data side; OcclusionData/Cone/Cylinder; queries; interfaces.
5. Gates: golden.aero_forces (NetFloat), capture.aero_forces_mono (MonoFloat, captures/out), capture.aero_forces_vessel
   (C10 parts + aoa_sweep), vessel_state un-drops cube_names / node_contact_areas.

## Findings so far
- Mono capture vs .NET golden differ in float32 tables (cube_drag up to 37 ULP) -> NetFloat policy for the .NET gate.
- SetPartOcclusion needs only node orientations + part rotations (no .mu bounds). conj(r)*r has exactly zero imaginary parts
  under Mono (double eval), so equal-rotation stacks give vector == node orientation exactly.
- OcclusionData is thermal occlusion (UpdateOcclusionConvection/Solar/Body), not SetPartOcclusion; needs part bounds via
  DragCubes.WeightedCenter/Size + localToWorldMatrix: unit-tested only.
- Cube weights at flight start: ModuleJettison.SetFairingDragCube (WP-26 file) + ModulePartVariants.SetCubes (WP-13).

## Status
- [x] 1 float_eval.hpp (MonoFloat / NetFloat, FE<P>, U<P> Dot/Magnitude/Normalize/ProjectOnPlane/Angle/Lerp/quaternion ops)
- [x] 2 DragCubeList aero (src/aero/drag_cube_list.cpp), Part::Awake SetPart, Instantiate CopySerializedFrom
- [x] 3 FlightIntegratorAero (src/aero/flight_integrator_aero.cpp)
- [x] 4 lifting / control / aero surface modules (registered), occlusion.cpp, aero_interfaces.hpp
- [x] gates golden.aero_forces (6 tables, 0 ULP incl. signed zeros) and capture.aero_forces_mono (3 tables, 0 ULP)
- [x] gate capture.aero_forces_vessel (parts 4,224 x 46, aoa_sweep 25,344 x 12, structure.node_contact_areas 631): 0 ULP
- [x] unit tests tests/test_aero.cpp (11 cases, 220,304 assertions)
- [x] STATUS "WP-21 (numeric part)", README rows; citations checked against the decompile by script
- [x] fresh ./run_tests.sh -f (clang 18) and -p linux-gcc-rel -f (GCC 13): 26/26 each, every gate 0 ULP

## Step log
- Signed-zero finding: the .NET golden's lifting_surface lift_x = -0.0 proves Vector3 unary minus is IL `neg` (ILSpy prints it
  as `0f - a.x`). Fixed ksp::unity Vector3f unary minus. Gatekeeper review: Vector3d / Vector2d (KSP Assembly-CSharp) reverted to
  the printed `0.0 - a.x`, and every Assembly-CSharp `0f - x` site in src/aero follows the decompile text; all gates still 0 ULP.
- Gatekeeper F2/F3: the vessel runner no longer writes dropped columns; DragVector_xyz (negated input) dropped: parts 46 columns.
- vessel_capture.{hpp,cpp}: moved WP-14's craft assembly + atmosphere recovery out of ds_vessel_state.cpp for sharing.

