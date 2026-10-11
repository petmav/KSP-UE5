# PROGRESS wp42 (scene flow, KSC, facilities, launch sites)

- [x] specs read; ported: pqs_city (+generated pqs_city_data.cpp), space_center, launch_site, psystem_setup (+InstallSeams), high_logic, destructible_building, space_center_building (+9 facility classes, LaunchSiteFacility), scene_drivers (FlightDriver, EditorDriver)
- [x] gate: tools/wp42/ksc_reference.py -> tests/data/wp42_reference.json; tests/test_space_center.cpp (10 cases, 1,044 assertions, 0 ULP)
- [x] WP-36 recovery test on the real seam; wp36 reference recovery section regenerated
- [x] check_cites exact mode 0 mismatches (226)
- [x] STATUS.md (row 32, unit row, WP-42 section), README rows, memory note
- [x] Windows -Fresh: 34/34, unit 576 cases / 936,215 assertions
- [x] WSL clang run 34/34; Linux unit totals measured by the gatekeeper (clang and gcc 576 / 936,199), see PROGRESS-wp42-gate.md
- [x] main 13750b33 merged

## Gate fixes F1-F7 (implementer, after PROGRESS-wp42-gate.md)

- F1 done: psystem_setup.cpp IsFacility cite :693, IsLaunchSite :714 (verified PSystemSetup.cs).
- F2 done: new `SpaceCenterFacilityLink` (hasFacilityTransform / setSpawnPointsLatLonAlt) member `spaceCenterFacility` on PQSCity and PQSCity2; both Orientate call it when the facility transform exists (PQSCity.cs:343, PQSCity2.cs:253). Unset = null facility.
- F3 done: PQSCity2::Orientate no longer writes inVisibleRange (C# only writes it in Reset :383 and setVisibility, which needs the PQS target and is not modelled).
- F4 done: `fx(num) + 0.1f` (SpaceCenterBuilding.cs:728).
- F5 done: new hook `PSystemSetup::prefabHasFlattenArea`; forcePQSPosition only when it returns true (PSystemSetup.cs:511-518).
- F6 done: space_center.hpp cites aligned to SpaceCenter.cs lines (Instance :6, cb :18, Lat/Lon/Srf :34/:36/:38, Awake :40, Start :51, OnDestroy :61, OnGameSceneLoaded :70, GreatCircleDistance :82).
- F7 done: polar Mohole test also asserts Math::Cos(pi/2) bits == libm_mono.json row 9 (3C91A60000000000); the 1e-9 position tolerance stays because the Python reference uses C libm.
- Tests: extended Moho case, new TEST_CASE "WP-42 gate fixes ..." in tests/test_space_center.cpp covering F2/F3/F5.
