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

// FlightGlobals.cs:196
Vector3d FlightGlobals::ship_srfVelocity() {
    if (ActiveVessel == nullptr) throw NullReferenceException("FlightGlobals.ship_srfVelocity: no ActiveVessel");
    return ActiveVessel->srf_velocity();
}

// FlightGlobals.cs:1471
double FlightGlobals::getAltitudeAtPos(const Vector3d& position, CelestialBody* body) {
    return Vector3d::Distance(position, body->position) - body->Radius;
}

// FlightGlobals.cs:1466: `return (float)getAltitudeAtPos((Vector3d)position, body);`
float FlightGlobals::getAltitudeAtPos(const Vector3f& position, CelestialBody* body) {
    return static_cast<float>(getAltitudeAtPos(Vector3d(position), body));
}

// FlightGlobals.cs:1406
double FlightGlobals::getStaticPressure(double altitude, CelestialBody* body) {
    if (body == nullptr) body = currentMainBody;
    if (body == nullptr) throw NullReferenceException("FlightGlobals.getStaticPressure: no currentMainBody");
    if (!body->atmosphere) return 0.0;
    return body->GetPressure(altitude);
}

}  // namespace ksp
