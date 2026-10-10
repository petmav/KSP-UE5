// Native (UnityPlayer.dll) Quaternion functions needed by ported KSP code, implemented as per-operation float32 C++
// (no Mono float rule: this is native code). Spec: Unity 2019.4 Runtime/Math/Quaternion.h AxisAngleToQuaternionSafe,
// checked bit-for-bit against the in-game capture harness/captures/out/math_shim_native.json (CAPTURE-SPEC C3,
// fn = "Quaternion.AngleAxis", 84 rows) by tests/test_orbit.cpp.
#pragma once

#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::UnityNative {

// UnityEngine.Quaternion.AngleAxis(float angle /*degrees*/, Vector3 axis): identity when |axis| <= 1e-6. Unity's
// Deg2Rad is `deg / 360 * 2 * kPI` (not deg * kDeg2Rad); with deg * (pi/180) 18 of the 84 captured rows differ.
Quaternionf AngleAxis(float angle, const Vector3f& axis);

}  // namespace ksp::UnityNative

namespace ksp::UnityNative {

// UnityEngine.Quaternion.FromToRotation(Vector3 from, Vector3 to) (WP-11, used by PQSMod_MapDecal/MapDecalTangent):
// FromToQuaternionSafe (identity if either |v| < 1e-5; both normalised by per-component division), the Moller-Hughes
// Matrix3x3f::SetFromToRotation (identity when dot > 1 - 1e-6, the antiparallel basis construction when dot < -1 + 1e-6,
// otherwise h = (1 - e) / |v|^2 with v = from x to), then MatrixToQuaternion (Shoemake) and Normalize. Float32 per
// operation. Bit-exact against all 49 captured rows (math_shim_native.json fn = "Quaternion.FromToRotation",
// tests/test_terrain.cpp).
Quaternionf FromToRotation(const Vector3f& fromDirection, const Vector3f& toDirection);

}  // namespace ksp::UnityNative

namespace ksp::UnityNative {

// WP-40 (UnityEngine.Quaternion natives the editor maths needs; per-operation float32 like the two above). The ONE implementation in kspcore
// (WP-24 ModuleCommand::DefaultQuaternionEuler and AutopilotEnvironment::DefaultEulerAngles delegate here).
// UNVERIFIED: each of the three is written from Unity's documented formulation (Runtime/Math/Quaternion.cpp), NOT captured against the game
// (CAPTURE-SPEC C3 has no rows for them). tools/wp40/editor_reference.py uses the same formulation, so agreement with it is a consistency
// check, not independent evidence. Needed: an in-game capture of Quaternion.LookRotation / Euler / eulerAngles.
//
// Quaternion.LookRotation(forward, up): LookRotationToMatrix (z = forward / |forward|, x = cross(up, z) / |x|, y = cross(z, x); identity when
// |forward| or |x| < 1e-5 or | |y|^2 - 1 | >= 1e-6) then MatrixToQuaternion.
// UNVERIFIED: from Unity's documented formulation (right-handed basis z/x/y as above), not captured against the game.
Quaternionf LookRotation(const Vector3f& forward, const Vector3f& up);
// Quaternion.Euler(Vector3 euler) (degrees): the managed `euler * ((float)Math.PI / 180f)` (Mono rule), then native EulerToQuaternion
// (q = (qY * qX) * qZ of the half-angle sin / cos).
// UNVERIFIED: from Unity's documented formulation (rotation order Z, X, Y applied i.e. ZXY), not captured against the game.
Quaternionf Euler(const Vector3f& eulerDegrees);
// Quaternion.eulerAngles: native QuaternionToEuler (matrix, YXZ order, asin / atan2), `* 57.29578f`, then the managed Internal_MakePositive.
// UNVERIFIED: from Unity's documented formulation (YXZ extraction convention), not captured against the game.
Vector3f EulerAngles(const Quaternionf& q);

}  // namespace ksp::UnityNative
