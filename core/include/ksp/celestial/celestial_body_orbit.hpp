// The run-time orbit of a celestial body (CelestialBody.orbit => orbitDriver.orbit): the body's ksp::OrbitDriver
// (OrbitDriver.cs, include/ksp/orbit/orbit_driver.hpp) over WP-07's ksp::Orbit, behind WP-09's ICelestialBodyOrbit link.
// An adapter rather than OrbitDriver/Orbit itself because Orbit's fields period / radius / vel / trueAnomaly carry the
// names of the interface's methods.
//
//   construction  CelestialBody.Start's `orbitDriver.QueuedUpdate = true` (CelestialBody.cs:483), then OrbitDriver.Start
//                 in UpdateMode.UPDATE (OrbitDriver.cs:98): the Orbit is the deserialized one (the seven elements and
//                 objectType of body.orbitDriver->orbit, referenceBody = body.orbitDriver->referenceBody), orbit.Init(),
//                 updateFromParameters(), Planetarium.Orbits.Add(driver) (WP-08: the list patched conics iterate).
//   UpdateOrbit   OrbitDriver.UpdateOrbit (OrbitDriver.cs:150) in UPDATE mode without a vessel: updateFromParameters()
//                 (orbit.UpdateFromUT(Planetarium UT); pos/vel = orbit.pos/vel swizzled; body.position =
//                 referenceBody.position + pos, or, for a reversed driver (the home body Kerbin, which KSP keeps still),
//                 referenceBody.position = body.position - pos).
// `reverse` lives in the body's CelestialBodyOrbitDriver (the scene state Planetarium.FindRootBody reads and
// PSystemSetup / OrbitPhysicsManager write); the adapter copies it into the run-time driver before every update.
// The reference body is fixed at spawn (KSP never re-parents a planet): orbit.referenceBody == orbitDriver->referenceBody.
#pragma once

#include <memory>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/orbit_driver.hpp"

namespace ksp {

class CelestialBodyOrbitAdapter final : public ICelestialBodyOrbit {
public:
    // CelestialBody.Start's QueuedUpdate + OrbitDriver.Start. `body` needs an orbitDriver whose referenceBody is set, and
    // a Planetarium must be registered (updateFromParameters reads Planetarium.GetUniversalTime(), Start appends the
    // driver to Planetarium.Orbits). Throws std::logic_error for other update modes.
    explicit CelestialBodyOrbitAdapter(CelestialBody& body);
    CelestialBodyOrbitAdapter(const CelestialBodyOrbitAdapter&) = delete;
    CelestialBodyOrbitAdapter& operator=(const CelestialBodyOrbitAdapter&) = delete;

    OrbitDriver driver;  // the body's OrbitDriver (celestialBody = &body; registered in Planetarium.Orbits)
    Orbit& orbit;        // *driver.orbit

    // OrbitDriver.updateFromParameters (OrbitDriver.cs:251/256)
    void updateFromParameters(bool setPosition = true);

    // ---- ICelestialBodyOrbit ----
    Orbit* GetOrbit() override { return &orbit; }
    double period() const override { return orbit.period; }
    double trueAnomaly() const override { return orbit.trueAnomaly; }
    double radius() const override { return orbit.radius; }
    double PeR() const override { return orbit.PeR(); }
    double ApR() const override { return orbit.ApR(); }
    Vector3d vel() const override { return orbit.vel; }
    Vector3d getRelativePositionAtUT(double UT) const override { return orbit.getRelativePositionAtUT(UT); }
    Vector3d getOrbitalVelocityAtUT(double UT) const override { return orbit.getOrbitalVelocityAtUT(UT); }
    void UpdateOrbit() override;  // OrbitDriver.UpdateOrbit(offset: true), UPDATE mode

    CelestialBody& body() const { return *body_; }  // OrbitDriver.celestialBody

private:
    CelestialBody* body_;
};

// The CelestialSystem::OrbitFactory that attaches the real run-time Orbit (CelestialSystem::Spawn's default).
std::unique_ptr<ICelestialBodyOrbit> MakeCelestialBodyOrbit(CelestialBody& body);

}  // namespace ksp
