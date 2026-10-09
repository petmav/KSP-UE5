// FlightGlobals (FlightGlobals.cs, 2003 lines; WP-19 "flight scene registry"): only the statics WP-08's OrbitDriver
// reads (getMainBody for CheckDominantBody, overrideOrbit). WP-19 replaces this with the full port; the body list is
// the one CelestialSystem::Spawn builds (PSystemManager.PostSetup fills FlightGlobals.fetch.bodies).
#pragma once

#include <vector>

#include "ksp/math/vector3d.hpp"

namespace ksp {

class CelestialBody;

struct FlightGlobals {
    // FlightGlobals.fetch.bodies; registered by CelestialSystem::Spawn (cleared when that system is destroyed).
    // nullptr = no FlightGlobals instance (getMainBody then throws NullReferenceException, like KSP).
    static inline const std::vector<CelestialBody*>* bodies = nullptr;
    static inline bool overrideOrbit = false;  // FlightGlobals.cs:102

    static CelestialBody* getMainBody(const Vector3d& refPos);  // FlightGlobals.cs:1311

private:
    static CelestialBody* inSOI(const Vector3d& pos, CelestialBody* body);  // FlightGlobals.cs:1316
};

}  // namespace ksp
