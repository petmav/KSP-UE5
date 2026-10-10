# PROGRESS wp42 (scene flow, KSC, facilities, launch sites)

- [x] read specs; design: namespace ksp, include/ksp/space_center/*.hpp, src/space_center/*.cpp
- [x] written (uncompiled as of this entry): pqs_city (+generated pqs_city_data.cpp via tools/wp42/gen_wp42_cities.py), space_center, launch_site, psystem_setup (+InstallSeams), high_logic, destructible_building, space_center_building (+facility subclasses, LaunchSiteFacility), scene_drivers (FlightDriver, EditorDriver)
- [ ] build + fix compile errors
- [ ] tools/wp42/ksc_reference.py -> tests/data/wp42_reference.json; tests/test_space_center.cpp
- [ ] switch WP-36 recovery test to the real seam
- [ ] check_cites, STATUS.md, README, full verification (Windows + WSL clang), merge main
