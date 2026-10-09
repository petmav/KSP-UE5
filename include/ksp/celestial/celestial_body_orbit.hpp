// The run-time orbit of a celestial body (CelestialBody.orbit => orbitDriver.orbit): KSP's OrbitDriver
// (OrbitDriver.cs) as far as a CelestialBody uses it, over WP-07's ksp::Orbit, behind WP-09's ICelestialBodyOrbit link.
// An adapter rather than Orbit itself because Orbit's fields period / radius / vel / trueAnomaly carry the names of the
// interface's methods.
//
//   construction  OrbitDriver.Start in UpdateMode.UPDATE (OrbitDriver.cs:98): the Orbit is the deserialized one (the
//                 seven elements and objectType of body.orbitDriver->orbit, referenceBody = body.orbitDriver->
//                 referenceBody), then orbit.Init(); updateFromParameters().
//   UpdateOrbit   OrbitDriver.UpdateOrbit (OrbitDriver.cs:158) in UPDATE mode without a vessel: updateFromParameters()
//                 (OrbitDriver.cs:272): orbit.UpdateFromUT(Planetarium UT); pos/vel = orbit.pos/vel swizzled; then
//                 body.position = referenceBody.position + pos, or, for a reversed driver (the home body Kerbin, which
//                 KSP keeps still), referenceBody.position = body.position - pos.
// Not here: vessel orbits (TRACK_Phys / IDLE, CheckDominantBody, SOI transitions, the isHyperbolic events) are WP-10's
// OrbitDriver; Planetarium.Orbits registration (read by patched conics) is WP-08's. Every stock body uses UPDATE.
// The reference body is fixed at spawn (KSP never re-parents a planet): orbit.referenceBody == orbitDriver->referenceBody.
#pragma once

#include <memory>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/orbit/orbit.hpp"

namespace ksp {

class CelestialBodyOrbitAdapter final : public ICelestialBodyOrbit {
public:
    // OrbitDriver.Start. `body` needs an orbitDriver whose referenceBody is set, and a Planetarium must be registered
    // (updateFromParameters reads Planetarium.GetUniversalTime()). Throws std::logic_error for other update modes.
    explicit CelestialBodyOrbitAdapter(CelestialBody& body);
    CelestialBodyOrbitAdapter(const CelestialBodyOrbitAdapter&) = delete;
    CelestialBodyOrbitAdapter& operator=(const CelestialBodyOrbitAdapter&) = delete;

    Orbit orbit;            // OrbitDriver.orbit
    Vector3d driverPos;     // OrbitDriver.pos (orbit.pos swizzled to Unity axes); renamed: vel() is an interface method
    Vector3d driverVel;     // OrbitDriver.vel
    double updateUT = 0.0;  // OrbitDriver.updateUT

    // OrbitDriver.updateFromParameters (OrbitDriver.cs:267/272)
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
