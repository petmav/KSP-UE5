# PROGRESS WP-50 Input, bindings, input locks (branch wp50)

- [x] gen_keycode.py: KeyCode (326) + ControlTypes (89) headers generated from the decompile
- [x] InputLockManager, KeyCodeExtended / InputBinding / KeyBinding / AxisBinding(_Single) / AxisKeyBinding(List) / InputDevices, ExtendedInput
- [x] GameSettings: generator (456 static fields, 446 SetDefaultValues statements), WriteCfg/ParseCfg, layout loader
- [x] FlightInputHandler (Update + FixedUpdate), AxisGroupsInputHandler, SpaceNavigator, Mouse, InputSettings listen session, KSP.UI lock helpers
- [x] Python reference tools/wp50/input_reference.py -> tests/data/wp50_reference.json; 32 unit cases / 47,676 assertions, all 0 ULP
- [x] cites: check_cites.py exact 0 mismatches (193), header_cites.py 0 (269)
- [x] STATUS.md section + summary row 34, README rows
- [x] fresh full verification (34/34 ctest, unit 626 cases / 1,012,303 assertions), branch pushed
