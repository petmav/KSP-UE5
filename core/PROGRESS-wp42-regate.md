# PROGRESS wp42 re-gate (gatekeeper), HEAD 68c68e06

- Windows MSVC fresh: 34/34, unit 577 cases / 936,234 assertions (WP-42: 11 / 1,063); 27 compare reports, max_ulp 0 except capture.deltav_stages.
- F1-F7 verified against PQSCity.cs:343, PQSCity2.cs:253/383, SpaceCenterBuilding.cs:728, PSystemSetup.cs:511-518, SpaceCenter.cs: faithful.
- check_cites exact 226/0; no std:: maths in space_center; tolerances untouched.
- STATUS wp42 row (32), unit row, WP-42 test line and Verification updated with measured numbers.
- CI: see final report.
