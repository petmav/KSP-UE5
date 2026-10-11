# PROGRESS wp38 gate (gatekeeper)

Branch wp38 at 65459822 (main 13750b33 merged). Verify only; STATUS edits listed below.

- [x] Windows -Fresh: 34/34 ctest; unit 579 cases / 963,143 assertions; 27 compare reports all 0 ULP except capture.deltav_stages (identical to main's build report); capture.part_database identical to main.
- [x] WP-38 13 / 27,984 (test_fineprint.cpp), WP-37 test_contracts.cpp 29 / 29,484 unchanged.
- [x] wp38_reference.json regenerates semantically equal, NOT byte-identical (set order); wp37_reference.json byte-identical.
- [x] Code review: orbit_utilities, fineprint_utilities, waypoint, exploration_contract, ProgressTrackingParameter vs C#: faithful; cites ~50 wrong (tools blind).
- [ ] Linux clang / gcc (WSL, logs /tmp/wp38gate_clang.log, /tmp/wp38gate_gcc.log, units /tmp/wp38gate_*_unit.log)
- [ ] STATUS verification line, commit, push, CI

STATUS edits by gatekeeper: merge note 13750b33, Sun KOLNIYA+TUNDRA ecc wording, cite claim corrected, reference reproducibility and GenerateOrbit-scope and uniqueSeed UB notes.

## Gate 2 (resumed)
- [x] Fresh Windows run (build deleted, CMAKE_BUILD_PARALLEL_LEVEL=4; the default parallelism hit C1060 out of heap): 34/34, unit 579 cases / 963,143 assertions, 28 compare reports all pass, max_ulp 0 except capture.deltav_stages (as main).
- [x] check_cites exact over 4 src files: 43 cites, 0 mismatches. No std:: sin/cos/..., only std::floor/fmod (exact ops).
- [x] wp37 JSON is not touched by wp38; the "monkeypatch" is w37.NODE_ID["CREWTRANSFER"]="CelestialBodyTransfer" inside the wp38 process only (C# KSPAchievements/CelestialBodyTransfer.cs:11).
- [x] GenerateOrbit 0 ULP: not a small change; documented in report.
