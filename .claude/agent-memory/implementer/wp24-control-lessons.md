---
name: wp24-control-lessons
description: Facts learned porting command / SAS / reaction wheels (WP-24): prefab-vs-instance Awake order, real Vessel resource host in tests, check_cites --fix, hook pattern
metadata:
  type: project
---

- `tools/wp28/check_cites.py --fix` repairs wrong `// File.cs:N` numbers; a comment that must not be checked (helper without a C# twin) should not start with `// File.cs:N`.
- PartModule.Awake runs before Load on the PREFAB, so `consumedResources` / ModuleCommand `controlPoints` are empty there; instances get the serialized state first. Test instance parts (`Game15::New`), not prefabs.
- A real `Vessel` has no resource host: tests need an `IResourceVessel` shim set through `ResourceFlowEnvironment::vesselHost` (see `TestVesselHost` in `tests/test_command.cpp`); `support::TestVessel` is a fake `Vessel*` and cannot reach `Autopilot()`.
- `FlightCtrlState` was already complete from WP-14. `Vessel::Autopilot()` / `OnAutopilotUpdate` came with WP-24.
- Unity natives (Quaternion.eulerAngles / Euler) have no capture; they are hook-overridable defaults. Ask for a C3-style capture before claiming player parity.
- Existing tests that assumed "no ModuleCommand => isControlSource NONE" had to be adapted when the real class landed (test_vessel, test_ship_construct, test_kerbals).
- Python reference: `tools/wp24/sas_reference.py` replays SAS from recorded traces stored in the JSON; PartSet drains `(-demand/amount)*amount`, not `demand`.
