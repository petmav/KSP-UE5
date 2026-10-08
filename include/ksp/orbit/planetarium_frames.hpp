// Planetarium's static frame state that Orbit reads (Assembly-CSharp/Planetarium.cs), nothing else.
//
//   Planetarium.Zup          (Planetarium.cs:74, static CelestialFrame)  -> PlanetariumFrames::Zup()
//   Planetarium.Rotation     (Planetarium.cs:127, fetch.rotation)        -> PlanetariumFrames::Rotation()
//   Planetarium.InverseRotAngle (Planetarium.cs:139)                     -> PlanetariumFrames::InverseRotAngle()
//   Planetarium.up/forward/right (Planetarium.cs:151-155)                -> up()/forward()/right()
//   Planetarium.ZupAtT       (Planetarium.cs:157)                        -> ZupAtT()
//
// The state is process-wide and mutable, like KSP's statics. Its initial value is what Planetarium.Awake
// (Planetarium.cs:182-191) produces with inverseRotAngle = 0 (the serialized prefab value): Zup = PlanetaryFrame(0, 90,
// 0) (the identity up to ~6e-17 cross terms) and Rotation = Inverse(Zup.Rotation).swizzle. CelestialBody.CBUpdate
// (CelestialBody.cs:562-564) re-derives both when the inverse-rotation frame is active: call SetInverseRotAngle().
// WP-09's Planetarium port should forward to these functions rather than keep a second copy. Not thread-safe.
#pragma once

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/math/vector3d.hpp"

namespace ksp {

struct CelestialBodyRef;

struct PlanetariumFrames {
    static CelestialFrame& Zup();
    static QuaternionD& Rotation();
    static double& InverseRotAngle();

    // Planetarium.Awake (Planetarium.cs:189-190): Zup = PlanetaryFrame(0, 90, inverseRotAngle);
    // rotation = QuaternionD.Inverse(Zup.Rotation).swizzle. Also stores inverseRotAngle.
    static void Awake(double inverseRotAngle);
    // CelestialBody.CBUpdate (CelestialBody.cs:562-564): InverseRotAngle = angle; Zup = PlanetaryFrame(0, 90, angle);
    // Rotation = QuaternionD.Inverse(Zup.Rotation).swizzle.
    static void SetInverseRotAngle(double angle);
    // Restore the Awake(0) state (tests).
    static void Reset() { Awake(0.0); }

    // Planetarium.cs:151 up => Rotation * (0, 1, 0)
    static Vector3d up();
    // Planetarium.cs:153 forward => Rotation * (0, 0, 1)
    static Vector3d forward();
    // Planetarium.cs:155 right => Rotation * (1, 0, 0)
    static Vector3d right();

    // Planetarium.cs:157 ZupAtT: the body's inverse-rotation Zup at UT, or the current Zup.
    static void ZupAtT(double UT, const CelestialBodyRef& body, CelestialFrame& tempZup);
};

}  // namespace ksp
