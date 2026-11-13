// Test hygiene: the global state a test may leave behind and the guards that put it back.
//
// The doctest listener in test_hygiene.cpp runs ResetSharedTestState() before and after every test case, so the suite
// passes in any order (`--order-by=rand -rs=<seed>`, `--order-by=name`, default; ctest unit.random_order). Tests that
// write shared state keep their own RAII guards; the listener is the net for the ones that forgot.
#pragma once

#include <cstddef>
#include <map>
#include <string>

namespace kspcore_tests {

// Restores the process-wide state that the shared read-only fixtures hand out and that tests are known to write:
//  - the position of every body of KerbolSystemPrefab() (CelestialSystem::Spawn copies the prefab, so a moved prefab
//    body moves every later spawned system),
//  - FlightGlobals (ready / ActiveVessel / VesselsLoaded / currentMainBody / overrideOrbit),
//  - Planetarium::Zup (static frame, see planetarium.hpp).
// It never touches singletons that a live object owns (those must be destroyed by the owning test).
void ResetSharedTestState();

// WP-UE-02: the same checks, counted: runs the listener's Restore (which also puts everything back) and returns the number of leaks it
// found (a body moved, FlightGlobals / InputLockManager state, GameEvents listeners above the process baseline, ...). 0 = clean. A kspcore_sim
// destroyed through the C ABI must leave 0 (ue5/DESIGN.md 1.3 "Lifetime", R1).
size_t HygieneLeaks(const std::string& who);

// WP-UE-02: a read-only version for objects that own the process state they set up (kspcore_sim_destroy): take the probe before the sim is
// made, ask Leaks() after it is destroyed. Same checks as the listener's Restore (bodies of the Kerbol prefab, FlightGlobals, FlightIntegrator,
// InputLockManager, GameSettings' instance, GameEvents listeners above the count at the probe), nothing is put back. 0 = clean.
class HygieneProbe {
public:
    HygieneProbe();
    size_t Leaks(std::string* what = nullptr) const;

private:
    std::map<std::string, size_t> listeners_;
};

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
