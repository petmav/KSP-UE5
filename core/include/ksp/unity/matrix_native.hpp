// WP-12a: the Unity matrix / vector functions the PQS quad build needs.
//
// Native (UnityPlayer.dll 2019.4.18f1, per-operation float32 C++, NOT the Mono float rule):
//   * Matrix4x4.TRS (Matrix4x4.cs:225 -> MatrixScripting::TRS): QuaternionToMatrix (x2 = x * 2, xx = x * x2, xy = x * y2,
//     wx = w * x2 ...; m00 = 1 - (yy + zz), m10 = xy + wz, m20 = xz - wy, m01 = xy - wz, m11 = 1 - (xx + zz),
//     m21 = yz + wx, m02 = xz + wy, m12 = yz - wx, m22 = 1 - (xx + yy)), each column scaled by s, translation = pos.
//     Bit-exact against all 45 rows of harness/captures/out/math_shim_native.json fn = "Matrix4x4.TRS".
//   * Vector3.OrthoNormalize (Vector3.cs:125 -> VectorScripting::OrthoNormalize): u /= |u| (division per component;
//     u = (1, 0, 0) when |u| <= 1e-5), v -= dot(u, v) * u, v = OrthoNormalVectorFast(u) when |v| < 1e-5, else v /= |v|;
//     dot / magnitude as ((x*x + y*y) + z*z) in float. Bit-exact against all 49 captured rows fn = "Vector3.OrthoNormalize"
//     (incl. the zero-tangent cases the PQS tangent build hits); a multiply-by-reciprocal variant misses 22 of them.
// Managed (UnityEngine.CoreModule, Mono rule, ksp/dotnet/mono_float.hpp):
//   * Matrix4x4.MultiplyPoint3x4 (Matrix4x4.cs:424): defined in src/aero/occlusion.cpp (WP-21), declared here and in
//     ksp/aero/occlusion.hpp.
#pragma once

#include "ksp/unity/color.hpp"  // Matrix4x4f
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// Matrix4x4.cs:424 MultiplyPoint3x4 (managed, Mono rule)
Vector3f MultiplyPoint3x4(const Matrix4x4f& m, const Vector3f& point);

}  // namespace ksp

namespace ksp::UnityNative {

// Matrix4x4.TRS(pos, q, s) (native)
Matrix4x4f TRS(const Vector3f& pos, const Quaternionf& q, const Vector3f& s);

// Vector3.OrthoNormalize(ref normal, ref tangent) (native); both arguments are rewritten
void OrthoNormalize(Vector3f& normal, Vector3f& tangent);

// OrthoNormalVectorFast(n) (native helper of OrthoNormalize): a unit vector orthogonal to the unit vector n
Vector3f OrthoNormalVectorFast(const Vector3f& n);

}  // namespace ksp::UnityNative
