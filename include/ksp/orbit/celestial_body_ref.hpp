// The part of CelestialBody (Assembly-CSharp/CelestialBody.cs) that Orbit (Orbit.cs) reads, and nothing else.
//
// Contract for WP-09 (CelestialBody / Planetarium): the full `ksp::CelestialBody` derives publicly from
// `ksp::CelestialBodyRef`, keeps these members (same names as KSP) up to date and sets `orbit` to the body's
// OrbitDriver orbit (WP-10 owns OrbitDriver). Orbit stores `CelestialBodyRef* referenceBody`; code that needs the full
// body casts (`static_cast<CelestialBody*>(orbit.referenceBody)`). No planet data lives here.
//
// Members Orbit reads, by Orbit.cs function:
//   gravParameter        everywhere (Init, UpdateFromFixedVectors, UpdateFromUT, velocities, semiLatusRectum, ...)
//   Radius               UpdateFromFixedVectors/UpdateFromUT (altitude), PeA/ApA, CreateRandomOrbitFlyBy
//   sphereOfInfluence    CreateRandomOrbitAround/FlyBy (PatchedConics, WP-08, reads it too)
//   atmosphereDepth      CreateRandomOrbitAround
//   initialRotation, rotPeriodRecip, inverseRotation, directRotAngle
//                        Planetarium.ZupAtT (GetOrbitalStateVectorsAtTrueAnomaly with worldToLocal)
//   rotates, zUpAngularVelocity           GetRotFrameVel / GetRotFrameVelAtPos
//   rotationPeriod, initialRotation       PhaseAngle
//   angularVelocity      getRFrmVel (GetWorldSpaceVel)
//   position             getPositionAtT / getPositionFromXxx / Separation / OrbitFromStateVectors
//   orbit, orbitDriverEnabled, orbitingBodies
//                        UpdateFromOrbitAtUT (HasChild, toBody.orbit), GetFrameVel(AtUT), getTruePositionAtUT
//   bodyName             logging only
//
// Units as in KSP: metres, seconds, m^3/s^2, degrees for rotation angles.
#pragma once

#include <limits>
#include <string>
#include <vector>

#include "ksp/math/vector3d.hpp"

namespace ksp {

class Orbit;

struct CelestialBodyRef {
    std::string bodyName = "Unnamed";                                     // CelestialBody.cs:12
    double Radius = 0.0;                                                  // CelestialBody.cs:22
    double gravParameter = 0.0;                                           // CelestialBody.cs:30
    double sphereOfInfluence = std::numeric_limits<double>::infinity();   // CelestialBody.cs:32
    double atmosphereDepth = 0.0;                                         // CelestialBody.cs:121
    bool rotates = false;                                                 // CelestialBody.cs:217
    double rotationPeriod = 0.0;                                          // CelestialBody.cs:219
    double rotPeriodRecip = 0.0;                                          // CelestialBody.cs:221
    double initialRotation = 0.0;                                         // CelestialBody.cs:227
    double directRotAngle = 0.0;                                          // CelestialBody.cs:231
    Vector3d angularVelocity;                                             // CelestialBody.cs:233
    Vector3d zUpAngularVelocity;                                          // CelestialBody.cs:235
    bool inverseRotation = false;                                         // CelestialBody.cs:241
    // CelestialBody.position (CelestialBody.cs:304) backing field _position. The property setter's side effects
    // (bodyTransform.position, pqsController.PrecisePosition) belong to WP-09.
    Vector3d position;
    // CelestialBody.orbit (CelestialBody.cs:321) = orbitDriver.orbit; nullptr means "no OrbitDriver" (the Sun).
    Orbit* orbit = nullptr;
    // orbitDriver.enabled, tested together with orbitDriver != null by GetFrameVel/GetFrameVelAtUT/getTruePositionAtUT.
    bool orbitDriverEnabled = true;
    // CelestialBody.orbitingBodies (CelestialBody.cs:251; children in spawn order), used by HasChild.
    std::vector<CelestialBodyRef*> orbitingBodies;

    CelestialBodyRef() = default;
    CelestialBodyRef(const CelestialBodyRef&) = default;
    CelestialBodyRef& operator=(const CelestialBodyRef&) = default;
    virtual ~CelestialBodyRef() = default;

    // CelestialBody.cs:345 referenceBody: the body itself when it has no OrbitDriver, else orbit.referenceBody.
    CelestialBodyRef* referenceBody();
    const CelestialBodyRef* referenceBody() const;
    // CelestialBody.cs:610
    Vector3d GetFrameVel() const;
    // CelestialBody.cs:619
    Vector3d GetFrameVelAtUT(double UT) const;
    // CelestialBody.cs:628
    Vector3d getRFrmVel(const Vector3d& worldPos) const;
    // CelestialBody.cs:633
    Vector3d getRFrmVelOrbit(const Orbit& o) const;
    // CelestialBody.cs:638
    Vector3d getTruePositionAtUT(double UT) const;
    // CelestialBody.cs:647
    Vector3d getPositionAtUT(double UT) const;
    // CelestialBody.cs:1037 (depth-first over orbitingBodies, reference equality)
    bool HasChild(const CelestialBodyRef* body) const;
};

}  // namespace ksp
