// Planetarium's static frames (Assembly-CSharp/Planetarium.cs); see include/ksp/orbit/planetarium_frames.hpp.
#include "ksp/orbit/planetarium_frames.hpp"

#include <cmath>

#include "ksp/orbit/celestial_body_ref.hpp"

namespace ksp {

namespace {

struct FrameState {
    CelestialFrame zup;
    QuaternionD rotation;
    double inverseRotAngle = 0.0;
    FrameState() { PlanetariumAwakeFrames(0.0, zup, rotation); }  // Planetarium.Awake with the prefab's 0
};

FrameState& State() {
    static FrameState s;
    return s;
}

}  // namespace

CelestialFrame& PlanetariumFrames::Zup() { return State().zup; }
QuaternionD& PlanetariumFrames::Rotation() { return State().rotation; }
double& PlanetariumFrames::InverseRotAngle() { return State().inverseRotAngle; }

// Planetarium.cs:182 Awake (frame statements, :189-190)
void PlanetariumFrames::Awake(double inverseRotAngle) {
    FrameState& s = State();
    s.inverseRotAngle = inverseRotAngle;
    PlanetariumAwakeFrames(inverseRotAngle, s.zup, s.rotation);
}

// CelestialBody.cs:562-564 (CBUpdate, inverse rotation active)
void PlanetariumFrames::SetInverseRotAngle(double angle) {
    FrameState& s = State();
    s.inverseRotAngle = angle;
    CelestialFrame::PlanetaryFrame(0.0, 90.0, s.inverseRotAngle, s.zup);
    s.rotation = QuaternionD::Inverse(s.zup.Rotation()).swizzle();
}

// Planetarium.cs:151
Vector3d PlanetariumFrames::up() { return Rotation() * Vector3d(0.0, 1.0, 0.0); }
// Planetarium.cs:153
Vector3d PlanetariumFrames::forward() { return Rotation() * Vector3d(0.0, 0.0, 1.0); }
// Planetarium.cs:155
Vector3d PlanetariumFrames::right() { return Rotation() * Vector3d(1.0, 0.0, 0.0); }

// Planetarium.cs:157
void PlanetariumFrames::ZupAtT(double UT, const CelestialBodyRef& body, CelestialFrame& tempZup) {
    const double num = std::fmod(body.initialRotation + 360.0 * body.rotPeriodRecip * UT, 360.0);
    if (body.inverseRotation) {
        const double rot = std::fmod(num - body.directRotAngle, 360.0);
        CelestialFrame::PlanetaryFrame(0.0, 90.0, rot, tempZup);
    } else {
        const CelestialFrame& zup = Zup();
        tempZup.X = zup.X;
        tempZup.Y = zup.Y;
        tempZup.Z = zup.Z;
    }
}

}  // namespace ksp
