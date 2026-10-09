// OrbitDriver (OrbitDriver.cs) for celestial bodies; see include/ksp/celestial/celestial_body_orbit.hpp.
#include "ksp/celestial/celestial_body_orbit.hpp"

#include <cmath>
#include <stdexcept>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"

namespace ksp {

namespace {
constexpr int32_t kUpdateModeUPDATE = 1;  // OrbitDriver.UpdateMode { TRACK_Phys, UPDATE, IDLE } (OrbitDriver.cs:6)
}

// OrbitDriver.Start (OrbitDriver.cs:98), case UpdateMode.UPDATE: orbit.Init(); updateFromParameters(). The Orbit
// object is the one Unity deserialized from the PSystem prefab (fields only, no constructor -> no Init before Start).
CelestialBodyOrbitAdapter::CelestialBodyOrbitAdapter(CelestialBody& body) : body_(&body) {
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
    orbit.Init();
    updateFromParameters();
}

// OrbitDriver.cs:272 updateFromParameters(setPosition) (the vessel branches do not apply to a body)
void CelestialBodyOrbitAdapter::updateFromParameters(bool setPosition) {
    updateUT = Planetarium::GetUniversalTime();
    orbit.UpdateFromUT(updateUT);
    driverPos = orbit.pos;
    driverVel = orbit.vel;
    driverPos.Swizzle();
    driverVel.Swizzle();
    if (std::isnan(driverPos.x)) {
        // MonoBehaviour.print of the orbit state, without the vel/AN vector texts (the vessel removal that follows in KSP
        // needs a vessel)
        log::Info("ObT : " + dotnet::FormatDoubleG(orbit.ObT) + "\nM : " + dotnet::FormatDoubleG(orbit.meanAnomaly) +
                  "\nE : " + dotnet::FormatDoubleG(orbit.eccentricAnomaly) + "\nV : " + dotnet::FormatDoubleG(orbit.trueAnomaly) +
                  "\nRadius: " + dotnet::FormatDoubleG(orbit.radius) + "\nperiod: " + dotnet::FormatDoubleG(orbit.period) + "\n");
    }
    if (!setPosition) return;
    CelestialBody* referenceBody = orbit.referenceBody;
    if (!body_->orbitDriver->reverse)
        body_->position = referenceBody->position + driverPos;
    else
        referenceBody->position = body_->position - driverPos;
}

// OrbitDriver.cs:158 UpdateOrbit(offset = true), UPDATE mode, no vessel: updateFromParameters(). (fdtLast and the
// isHyperbolic bookkeeping only feed vessel events; DrawOrbit is a gizmo.)
void CelestialBodyOrbitAdapter::UpdateOrbit() { updateFromParameters(); }

std::unique_ptr<ICelestialBodyOrbit> MakeCelestialBodyOrbit(CelestialBody& body) {
    return std::make_unique<CelestialBodyOrbitAdapter>(body);
}

}  // namespace ksp
