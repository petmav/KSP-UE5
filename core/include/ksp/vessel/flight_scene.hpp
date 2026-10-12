// WP-19a: the flight scene's per-frame tick over the vessel registry, in the order Unity ran the scripts in KSP 1.12.5.
//
// The order is MEASURED (analysis/physics/raw/run1/run1-L_calltrace.csv, one frame of PhysProf-L on the pad; `seq` below), not in the C#:
//
//   FixedUpdate  seq 5-285
//     6   Planetarium.FixedUpdate                 host (WP-07/09: Planetarium::FixedUpdate)
//     7   VesselPrecalculate.FixedUpdate          every vessel, loaded or not          <- FlightScene::FixedUpdate step 1
//     9   OrbitDriver.FixedUpdate                 every vessel's driver (QueueOnce makes it a no-op after MainPhysics) <- step 2
//     15  FlightGlobals.FixedUpdate               <- step 3 (UpdateInformation, `ready`)
//     19  TimeWarp.FixedUpdate                    host
//     22-276 Part / module FixedUpdate           host (WP-13 .. WP-28)
//     278 FlightIntegrator.FixedUpdate            host (WP-20d)
//     280 OrbitPhysicsManager.FixedUpdate         <- step 4
//     281 Vessel.FixedUpdate                      <- step 5
//     284 Krakensbane.FixedUpdate, 285 FloatingOrigin.FixedUpdate   host (WP-20a; ORDER CONSTRAINT: they come after VesselPrecalculate, which reads
//                                                  the true positions for gravity; PHYSICS-SPIKE.md WP-19a, STATUS.md WP-20a deviation 4)
//     291 PhysicsFixedUpdate                      PhysicsWorld::FixedStep (WP-20a)
//   Update       VesselPrecalculate.Update (313) -> FlightGlobals.Update (332) -> Vessel.Update (578)
//   LateUpdate   OrbitPhysicsManager.LateUpdate (879) -> Vessel.LateUpdate (880)
//
// Steps of other packages belong between the calls; `FlightScene::Hooks` lets a host interleave them without reading this order from the trace.
#pragma once

#include <functional>

namespace ksp {

struct FlightScene {
    struct Hooks {
        std::function<void()> afterPrecalc;        // between seq 7-9 and 15 (nothing of the 19a set runs there)
        std::function<void()> beforeFlightIntegrator;  // seq 15-278: Part / module FixedUpdate, FlightInputHandler, TimeWarp
        std::function<void()> afterFlightIntegrator;   // seq 278: FlightIntegrator.FixedUpdate (WP-20d)
    };
    static inline Hooks hooks;

    static void FixedUpdate();  // steps 1-5 above (Planetarium, Krakensbane, FloatingOrigin and the physics step are the host's)
    static void Update();       // VesselPrecalculate.Update, FlightGlobals.Update, Vessel.Update for every vessel
    static void LateUpdate();   // OrbitPhysicsManager.LateUpdate, Vessel.LateUpdate for every vessel
};

}  // namespace ksp
