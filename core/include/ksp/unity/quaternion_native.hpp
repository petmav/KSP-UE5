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

// UnityEngine.Transform.rotation composition (native, UnityPlayer.dll): Quaternionf operator* with one float32 rounding per operation
// (see quaternion_native.cpp). WP-17b: world rotation of a thrust transform = partRotation * partSpaceChainRotation.
Quaternionf Multiply(const Quaternionf& lhs, const Quaternionf& rhs);
Quaternionf Inverse(const Quaternionf& q);    // conjugate
Quaternionf Normalize(const Quaternionf& q);  // q / sqrt(dot(q, q)), float per operation

}  // namespace ksp::UnityNative
