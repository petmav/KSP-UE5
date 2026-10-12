// The flight scene tick over the registry; see include/ksp/vessel/flight_scene.hpp (order measured in game).
#include "ksp/vessel/flight_scene.hpp"

#include <algorithm>
#include <vector>

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/physics/orbit_physics_manager.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {
// A vessel destroyed (Die -> RemoveVessel) by an earlier script of the same frame still gets its remaining callbacks only if it is registered
std::vector<Vessel*> Snapshot() { return FlightGlobals::Vessels; }
bool Registered(Vessel* v) { return std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), v) != FlightGlobals::Vessels.end(); }
}  // namespace

void FlightScene::FixedUpdate() {
    const std::vector<Vessel*> vessels = Snapshot();
    for (Vessel* v : vessels)
        if (Registered(v)) v->precalc.FixedUpdate();  // VesselPrecalculate.FixedUpdate (seq 7)
    for (Vessel* v : vessels)
        if (Registered(v) && v->orbitDriver != nullptr) v->orbitDriver->FixedUpdate();  // OrbitDriver.FixedUpdate (seq 9)
    if (hooks.afterPrecalc) hooks.afterPrecalc();
    FlightGlobals::FixedUpdate();  // seq 15
    if (hooks.beforeFlightIntegrator) hooks.beforeFlightIntegrator();
    if (hooks.afterFlightIntegrator) hooks.afterFlightIntegrator();
    if (OrbitPhysicsManager* opm = OrbitPhysicsManager::Instance()) opm->FixedUpdate();  // seq 280
    for (Vessel* v : vessels)
        if (Registered(v)) v->FixedUpdate();  // Vessel.FixedUpdate (seq 281)
}

void FlightScene::Update() {
    const std::vector<Vessel*> vessels = Snapshot();
    for (Vessel* v : vessels)
        if (Registered(v)) v->precalc.Update();  // seq 313
    FlightGlobals::Update();                      // seq 332
    if (hooks.beforeVesselUpdate) hooks.beforeVesselUpdate();
    for (Vessel* v : vessels)
        if (Registered(v)) v->Update();  // seq 578
}

void FlightScene::LateUpdate() {
    const std::vector<Vessel*> vessels = Snapshot();
    if (OrbitPhysicsManager* opm = OrbitPhysicsManager::Instance()) opm->LateUpdate();  // seq 879
    for (Vessel* v : vessels)
        if (Registered(v)) v->LateUpdate();  // seq 880
}

}  // namespace ksp
