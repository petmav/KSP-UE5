// FlightGlobals subset for WP-08; see include/ksp/celestial/flight_globals.hpp.
#include "ksp/celestial/flight_globals.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"

namespace ksp {

// FlightGlobals.cs:1311 getMainBody(refPos) => inSOI(refPos, fetch.bodies[0])
CelestialBody* FlightGlobals::getMainBody(const Vector3d& refPos) {
    if (bodies == nullptr) throw NullReferenceException("FlightGlobals.getMainBody: no FlightGlobals instance");
    if (bodies->empty()) throw std::out_of_range("FlightGlobals.getMainBody: bodies is empty");
    return inSOI(refPos, (*bodies)[0]);
}

// FlightGlobals.cs:1316
CelestialBody* FlightGlobals::inSOI(const Vector3d& pos, CelestialBody* body) {
    const size_t count = body->orbitingBodies.size();
    if (count > 0) {
        for (size_t i = 0; i < count; ++i) {
            CelestialBody* celestialBody = body->orbitingBodies[i];
            if (!((pos - celestialBody->position).sqrMagnitude() >=
                  celestialBody->sphereOfInfluence * celestialBody->sphereOfInfluence))
                return inSOI(pos, celestialBody);
        }
    }
    return body;
}

}  // namespace ksp
