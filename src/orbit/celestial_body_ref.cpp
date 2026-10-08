// CelestialBody members that Orbit reads (Assembly-CSharp/CelestialBody.cs); see include/ksp/orbit/celestial_body_ref.hpp.
#include "ksp/orbit/celestial_body_ref.hpp"

#include "ksp/orbit/orbit.hpp"

namespace ksp {

// CelestialBody.cs:345
CelestialBodyRef* CelestialBodyRef::referenceBody() {
    if (orbit == nullptr) return this;
    return orbit->referenceBody;
}
const CelestialBodyRef* CelestialBodyRef::referenceBody() const {
    if (orbit == nullptr) return this;
    return orbit->referenceBody;
}

// CelestialBody.cs:610
Vector3d CelestialBodyRef::GetFrameVel() const {
    if (orbit != nullptr && orbitDriverEnabled) {
        return orbit->vel + ((orbit->referenceBody != nullptr) ? orbit->referenceBody->GetFrameVel() : Vector3d::zero());
    }
    return Vector3d::zero();
}

// CelestialBody.cs:619
Vector3d CelestialBodyRef::GetFrameVelAtUT(double UT) const {
    if (orbit != nullptr && orbitDriverEnabled) {
        return orbit->getOrbitalVelocityAtUT(UT) +
               ((orbit->referenceBody != nullptr) ? orbit->referenceBody->GetFrameVelAtUT(UT) : Vector3d::zero());
    }
    return Vector3d::zero();
}

// CelestialBody.cs:628
Vector3d CelestialBodyRef::getRFrmVel(const Vector3d& worldPos) const {
    return Vector3d::Cross(angularVelocity, worldPos - position);
}

// CelestialBody.cs:633
Vector3d CelestialBodyRef::getRFrmVelOrbit(const Orbit& o) const { return Vector3d::Cross(angularVelocity, o.pos.xzy()); }

// CelestialBody.cs:638
Vector3d CelestialBodyRef::getTruePositionAtUT(double UT) const {
    if (orbit != nullptr && orbitDriverEnabled) {
        return orbit->getRelativePositionAtUT(UT).xzy() + orbit->referenceBody->getTruePositionAtUT(UT);
    }
    return position;
}

// CelestialBody.cs:647
Vector3d CelestialBodyRef::getPositionAtUT(double UT) const {
    if (orbit != nullptr && orbitDriverEnabled) {
        return orbit->getRelativePositionAtUT(UT).xzy() + orbit->referenceBody->position;
    }
    return position;
}

// CelestialBody.cs:1037
bool CelestialBodyRef::HasChild(const CelestialBodyRef* body) const {
    for (const CelestialBodyRef* celestialBody : orbitingBodies) {
        if (celestialBody == body) return true;
        if (celestialBody->HasChild(body)) return true;
    }
    return false;
}

}  // namespace ksp
