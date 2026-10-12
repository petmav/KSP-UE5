// WP-12a: native Unity Matrix4x4.TRS and Vector3.OrthoNormalize (see include/ksp/unity/matrix_native.hpp).
// Plain float32 C++: kspcore builds with /fp:precise (MSVC) / -ffp-contract=off (clang, gcc), SSE2 scalar float, so
// every operation below is one IEEE binary32 rounding, as in UnityPlayer.dll.
#include "ksp/unity/matrix_native.hpp"

#include "ksp/unity/quaternion_native.hpp"  // SqrtF

namespace ksp::UnityNative {

// Matrix4x4.cs:225 TRS -> MatrixScripting::TRS: QuaternionToMatrix, then the 3x3 block scaled column-wise, translation
// in the fourth column (checked bit for bit against the C3 capture, fn = "Matrix4x4.TRS", 45 rows)
Matrix4x4f TRS(const Vector3f& pos, const Quaternionf& q, const Vector3f& s) {
    const float x = q.x * 2.0f;
    const float y = q.y * 2.0f;
    const float z = q.z * 2.0f;
    const float xx = q.x * x;
    const float yy = q.y * y;
    const float zz = q.z * z;
    const float xy = q.x * y;
    const float xz = q.x * z;
    const float yz = q.y * z;
    const float wx = q.w * x;
    const float wy = q.w * y;
    const float wz = q.w * z;
    Matrix4x4f m;
    m.m00 = (1.0f - (yy + zz)) * s.x;
    m.m10 = (xy + wz) * s.x;
    m.m20 = (xz - wy) * s.x;
    m.m30 = 0.0f;
    m.m01 = (xy - wz) * s.y;
    m.m11 = (1.0f - (xx + zz)) * s.y;
    m.m21 = (yz + wx) * s.y;
    m.m31 = 0.0f;
    m.m02 = (xz + wy) * s.z;
    m.m12 = (yz - wx) * s.z;
    m.m22 = (1.0f - (xx + yy)) * s.z;
    m.m32 = 0.0f;
    m.m03 = pos.x;
    m.m13 = pos.y;
    m.m23 = pos.z;
    m.m33 = 1.0f;
    return m;
}

namespace {
float DotF(const Vector3f& a, const Vector3f& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
}  // namespace

// OrthoNormalVectorFast (Runtime/Math/Vector3.h): k1OverSqrt2 = 0.70710678118654752440
Vector3f OrthoNormalVectorFast(const Vector3f& n) {
    constexpr float k1OverSqrt2 = 0.70710678118654752440f;
    Vector3f res;
    if ((n.z < 0.0f ? -n.z : n.z) > k1OverSqrt2) {
        const float a = n.y * n.y + n.z * n.z;
        const float k = 1.0f / SqrtF(a);
        res.x = 0.0f;
        res.y = -n.z * k;
        res.z = n.y * k;
    } else {
        const float a = n.x * n.x + n.y * n.y;
        const float k = 1.0f / SqrtF(a);
        res.x = -n.y * k;
        res.y = n.x * k;
        res.z = 0.0f;
    }
    return res;
}

// Vector3.cs:125 OrthoNormalize -> VectorScripting::OrthoNormalize (checked against the C3 capture, 49 rows)
void OrthoNormalize(Vector3f& u, Vector3f& v) {
    constexpr float kEpsilon = 0.00001f;  // Vector3f::epsilon
    float mag = SqrtF(DotF(u, u));
    if (mag > kEpsilon) {
        u.x = u.x / mag;
        u.y = u.y / mag;
        u.z = u.z / mag;
    } else {
        u = Vector3f(1.0f, 0.0f, 0.0f);
    }
    const float dot0 = DotF(u, v);
    v.x = v.x - dot0 * u.x;
    v.y = v.y - dot0 * u.y;
    v.z = v.z - dot0 * u.z;
    mag = SqrtF(DotF(v, v));
    if (mag < kEpsilon) {
        v = OrthoNormalVectorFast(u);
    } else {
        v.x = v.x / mag;
        v.y = v.y / mag;
        v.z = v.z / mag;
    }
}

}  // namespace ksp::UnityNative
