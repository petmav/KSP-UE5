# WP-UE-08 progress (input device)

Branch `wpue08` (on top of `wpue11`). Inline in the orchestrating session, 2026-11-18. Results: `STATUS.md` "Input device".

- [x] `FKspInputDevice`: Slate input pre-processor, key table, per-frame `UnityEngine.Input`-shaped state.
- [x] `UKspSimSubsystem` owns and registers it; `DefaultInput` takes its frame (viewport size, cursor, focus, UI hover).
- [x] Tests `KspUe5.Input.KeyCodeTable`, `DeviceFrames`, `DeviceParity` (hash `2c4ae66445a41c81` with keys through the device).
- [x] `-KspKeys` / `fly.ps1 -Keys`: scripted key presses as real Slate events; liftoff screenshot `slice/shots/wpue08_liftoff.png`.
- [ ] `Input.inputString`; input-manager axes (needs the game's InputManager asset); joystick axes; a test of the UI hover lock (needs UI).

Notes for the next package:
- The pure-C twin's input frame is not what a device produces (it lists Z as held with T and Space only "down", and never sends a
  key-up). The device run taps T and Space and releases Z a frame later; core's flight state is the same, so key-up lists and the held
  set of that frame do not matter to those three bindings. A binding that polls `GetKey` or `GetKeyUp` would need its own parity row.
- A new `.ps1` written by the session is LF; the old ones are CRLF in the working tree. Edit scripts must match the file they patch.
