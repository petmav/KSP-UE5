// Test hygiene: the global state a test may leave behind and the guards that put it back.
//
// The doctest listener in test_hygiene.cpp runs ResetSharedTestState() before and after every test case, so the suite
// passes in any order (`--order-by=rand -rs=<seed>`, `--order-by=name`, default; ctest unit.random_order). Tests that
// write shared state keep their own RAII guards; the listener is the net for the ones that forgot.
#pragma once

namespace kspcore_tests {

// Restores the process-wide state that the shared read-only fixtures hand out and that tests are known to write:
//  - the position of every body of KerbolSystemPrefab() (CelestialSystem::Spawn copies the prefab, so a moved prefab
//    body moves every later spawned system),
//  - FlightGlobals (ready / ActiveVessel / VesselsLoaded / currentMainBody / overrideOrbit),
//  - Planetarium::Zup (static frame, see planetarium.hpp).
// It never touches singletons that a live object owns (those must be destroyed by the owning test).
void ResetSharedTestState();

}  // namespace kspcore_tests

namespace kspcore_tests {

// The stock en-us dictionary (GameData/Squad/Localization/dictionary.cfg of the game root) as the process-wide Localizer.
// The listener shuts the Localizer down between test cases, so a test that formats #autoLOC tags loads it itself:
//   kspcore_tests::LocalizerScope loc;   // at the top of the test case
void LoadStockLocalizer();
struct LocalizerScope {
    LocalizerScope() { LoadStockLocalizer(); }
    ~LocalizerScope();
    LocalizerScope(const LocalizerScope&) = delete;
    LocalizerScope& operator=(const LocalizerScope&) = delete;
};

}  // namespace kspcore_tests
