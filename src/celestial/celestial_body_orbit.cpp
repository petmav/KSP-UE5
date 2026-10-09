// OrbitDriver (OrbitDriver.cs) for celestial bodies; see include/ksp/celestial/celestial_body_orbit.hpp.
#include "ksp/celestial/celestial_body_orbit.hpp"

#include <stdexcept>

#include "ksp/celestial/planetarium.hpp"

namespace ksp {

namespace {
constexpr int32_t kUpdateModeUPDATE = 1;  // OrbitDriver.UpdateMode { TRACK_Phys, UPDATE, IDLE } (OrbitDriver.cs:6)
}

// CelestialBody.Start (CelestialBody.cs:483) sets orbitDriver.QueuedUpdate = true (CBUpdate drives the body's orbit);
// then OrbitDriver.Start (OrbitDriver.cs:98), case UpdateMode.UPDATE: orbit.Init(); updateFromParameters(); ready;
// Planetarium.Orbits.Add(this). The Orbit object is the one Unity deserialized from the PSystem prefab (fields only, no
// constructor -> no Init before Start).
CelestialBodyOrbitAdapter::CelestialBodyOrbitAdapter(CelestialBody& body) : orbit(*driver.orbit), body_(&body) {
    if (!body.orbitDriver) throw std::logic_error("CelestialBodyOrbitAdapter: " + body.bodyName + " has no OrbitDriver");
    const CelestialBodyOrbitDriver& od = *body.orbitDriver;
    if (od.referenceBody == nullptr) throw std::logic_error("CelestialBodyOrbitAdapter: " + body.bodyName + " has no reference body");
    if (od.updateMode != kUpdateModeUPDATE)
        throw std::logic_error("CelestialBodyOrbitAdapter: " + body.bodyName +
                               ": only OrbitDriver.UpdateMode.UPDATE is supported for celestial bodies (TRACK_Phys/IDLE: WP-10)");
    const CelestialOrbitElements& e = od.orbit;
    orbit.inclination = e.inclination;
    orbit.eccentricity = e.eccentricity;
    orbit.semiMajorAxis = e.semiMajorAxis;
    orbit.LAN = e.LAN;
    orbit.argumentOfPeriapsis = e.argumentOfPeriapsis;
    orbit.meanAnomalyAtEpoch = e.meanAnomalyAtEpoch;
    orbit.epoch = e.epoch;
    orbit.objectType = static_cast<Orbit::ObjectType>(e.objectType);
    orbit.referenceBody = od.referenceBody;  // OrbitDriver.referenceBody => orbit.referenceBody
    driver.celestialBody = &body;            // OrbitDriver.Awake: GetComponent<CelestialBody>()
    driver.name = body.bodyName;
    driver.reverse = od.reverse;
    driver.updateMode = OrbitDriver::UpdateMode::UPDATE;
    driver.QueuedUpdate = true;  // CelestialBody.cs:483
    driver.Start();
}

// OrbitDriver.cs:256 updateFromParameters(setPosition) (reverse mirrored from the body's scene state)
void CelestialBodyOrbitAdapter::updateFromParameters(bool setPosition) {
    driver.reverse = body_->orbitDriver->reverse;
    driver.updateFromParameters(setPosition);
}

// OrbitDriver.cs:150 UpdateOrbit(offset = true), UPDATE mode, no vessel: updateFromParameters(); the fdtLast and
// isHyperbolic bookkeeping only feed vessel events.
void CelestialBodyOrbitAdapter::UpdateOrbit() {
    driver.reverse = body_->orbitDriver->reverse;
    driver.UpdateOrbit();
}

std::unique_ptr<ICelestialBodyOrbit> MakeCelestialBodyOrbit(CelestialBody& body) {
    return std::make_unique<CelestialBodyOrbitAdapter>(body);
}

}  // namespace ksp
