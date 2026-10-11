# PROGRESS wp42 (scene flow, KSC, facilities, launch sites)

- [x] specs read; ported: pqs_city (+generated pqs_city_data.cpp), space_center, launch_site, psystem_setup (+InstallSeams), high_logic, destructible_building, space_center_building (+9 facility classes, LaunchSiteFacility), scene_drivers (FlightDriver, EditorDriver)
- [x] gate: tools/wp42/ksc_reference.py -> tests/data/wp42_reference.json; tests/test_space_center.cpp (10 cases, 1,044 assertions, 0 ULP)
- [x] WP-36 recovery test on the real seam; wp36 reference recovery section regenerated
- [x] check_cites exact mode 0 mismatches (226)
- [x] STATUS.md (row 32, unit row, WP-42 section), README rows, memory note
- [x] Windows -Fresh: 34/34, unit 576 cases / 936,215 assertions
- [x] WSL clang run 34/34; Linux unit totals measured by the gatekeeper (clang and gcc 576 / 936,199), see PROGRESS-wp42-gate.md
- [x] main 13750b33 merged
