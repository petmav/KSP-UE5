// Native Unity quaternion functions (see include/ksp/unity/quaternion_native.hpp).
#include "ksp/unity/quaternion_native.hpp"

#include <cmath>

#include "ksp/dotnet/ucrt_libm.hpp"

namespace ksp::UnityNative {

// QuaternionScripting::AngleAxis -> AxisAngleToQuaternionSafe(axis, Deg2Rad(angle)) (Runtime/Math/Quaternion.h)
Quaternionf AngleAxis(float angle, const Vector3f& axis) {
    // Unity's Deg2Rad (Runtime/Math/FloatConversion.h) keeps a historical operation order: deg / 360 * 2 * kPI.
    const float rad = angle / 360.0f * 2.0f * 3.14159265358979323846f;
    const float mag = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (mag > 0.000001f) {
        const float halfAngle = rad * 0.5f;
        Quaternionf q;
        q.w = ksp::dotnet::NativeCosf(halfAngle);
        const float s = ksp::dotnet::NativeSinf(halfAngle) / mag;
        q.x = s * axis.x;
        q.y = s * axis.y;
        q.z = s * axis.z;
        return q;
    }
    return Quaternionf::identity();
}

}  // namespace ksp::UnityNative

namespace ksp::UnityNative {

namespace {

// Runtime/Math/Matrix3x3.cpp FromToRotation (Moller & Hughes, "Efficiently building a matrix to rotate one vector to
// another"); mtx[row][col].
void FromToRotationMatrix(const float from[3], const float to[3], float mtx[3][3]) {
    const double kEpsilon = 0.000001;
    float v[3];
    v[0] = from[1] * to[2] - from[2] * to[1];
    v[1] = from[2] * to[0] - from[0] * to[2];
    v[2] = from[0] * to[1] - from[1] * to[0];
    const float e = from[0] * to[0] + from[1] * to[1] + from[2] * to[2];
    if (e > 1.0 - kEpsilon) {
        mtx[0][0] = 1.0f; mtx[0][1] = 0.0f; mtx[0][2] = 0.0f;
        mtx[1][0] = 0.0f; mtx[1][1] = 1.0f; mtx[1][2] = 0.0f;
        mtx[2][0] = 0.0f; mtx[2][1] = 0.0f; mtx[2][2] = 1.0f;
    } else if (e < -1.0 + kEpsilon) {
        float up[3], left[3];
        left[0] = 0.0f;
        left[1] = from[2];
        left[2] = -from[1];
        if (left[0] * left[0] + left[1] * left[1] + left[2] * left[2] < kEpsilon) {
            left[0] = -from[2];
            left[1] = 0.0f;
            left[2] = from[0];
        }
        const float invlen = 1.0f / std::sqrt(left[0] * left[0] + left[1] * left[1] + left[2] * left[2]);
        left[0] *= invlen;
        left[1] *= invlen;
        left[2] *= invlen;
        up[0] = left[1] * from[2] - left[2] * from[1];
        up[1] = left[2] * from[0] - left[0] * from[2];
        up[2] = left[0] * from[1] - left[1] * from[0];
        const float fxx = -from[0] * from[0], fyy = -from[1] * from[1], fzz = -from[2] * from[2];
        const float fxy = -from[0] * from[1], fxz = -from[0] * from[2], fyz = -from[1] * from[2];
        const float uxx = up[0] * up[0], uyy = up[1] * up[1], uzz = up[2] * up[2];
        const float uxy = up[0] * up[1], uxz = up[0] * up[2], uyz = up[1] * up[2];
        const float lxx = -left[0] * left[0], lyy = -left[1] * left[1], lzz = -left[2] * left[2];
        const float lxy = -left[0] * left[1], lxz = -left[0] * left[2], lyz = -left[1] * left[2];
        mtx[0][0] = fxx + uxx + lxx; mtx[0][1] = fxy + uxy + lxy; mtx[0][2] = fxz + uxz + lxz;
        mtx[1][0] = mtx[0][1];       mtx[1][1] = fyy + uyy + lyy; mtx[1][2] = fyz + uyz + lyz;
        mtx[2][0] = mtx[0][2];       mtx[2][1] = mtx[1][2];       mtx[2][2] = fzz + uzz + lzz;
    } else {
        const float h = (1.0f - e) / (v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        const float hvx = h * v[0];
        const float hvz = h * v[2];
        const float hvxy = hvx * v[1];
        const float hvxz = hvx * v[2];
        const float hvyz = hvz * v[1];
        mtx[0][0] = e + hvx * v[0];
        mtx[0][1] = hvxy - v[2];
        mtx[0][2] = hvxz + v[1];
        mtx[1][0] = hvxy + v[2];
        mtx[1][1] = e + h * v[1] * v[1];
        mtx[1][2] = hvyz - v[0];
        mtx[2][0] = hvxz - v[1];
        mtx[2][1] = hvyz + v[0];
        mtx[2][2] = e + hvz * v[2];
    }
}

// Runtime/Math/Quaternion.cpp MatrixToQuaternion (Shoemake), then Normalize (q / |q|)
Quaternionf MatrixToQuaternion(const float m[3][3]) {
    Quaternionf q;
    const float trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0.0f) {
        float root = std::sqrt(trace + 1.0f);
        q.w = 0.5f * root;
        root = 0.5f / root;
        q.x = (m[2][1] - m[1][2]) * root;
        q.y = (m[0][2] - m[2][0]) * root;
        q.z = (m[1][0] - m[0][1]) * root;
    } else {
        const int next[3] = {1, 2, 0};
        int i = 0;
        if (m[1][1] > m[0][0]) i = 1;
        if (m[2][2] > m[i][i]) i = 2;
        const int j = next[i];
        const int k = next[j];
        float root = std::sqrt(m[i][i] - m[j][j] - m[k][k] + 1.0f);
        float* quat[3] = {&q.x, &q.y, &q.z};
        *quat[i] = 0.5f * root;
        root = 0.5f / root;
        q.w = (m[k][j] - m[j][k]) * root;
        *quat[j] = (m[j][i] + m[i][j]) * root;
        *quat[k] = (m[k][i] + m[i][k]) * root;
    }
    const float mag = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    q.x /= mag;
    q.y /= mag;
    q.z /= mag;
    q.w /= mag;
    return q;
}

}  // namespace

// QuaternionScripting::FromToRotation -> FromToQuaternionSafe (Runtime/Math/Quaternion.h)
Quaternionf FromToRotation(const Vector3f& fromDirection, const Vector3f& toDirection) {
    const float lhsMag = std::sqrt(fromDirection.x * fromDirection.x + fromDirection.y * fromDirection.y + fromDirection.z * fromDirection.z);
    const float rhsMag = std::sqrt(toDirection.x * toDirection.x + toDirection.y * toDirection.y + toDirection.z * toDirection.z);
    if (lhsMag < 0.00001f || rhsMag < 0.00001f) return Quaternionf::identity();
    const float from[3] = {fromDirection.x / lhsMag, fromDirection.y / lhsMag, fromDirection.z / lhsMag};
    const float to[3] = {toDirection.x / rhsMag, toDirection.y / rhsMag, toDirection.z / rhsMag};
    float m[3][3];
    FromToRotationMatrix(from, to, m);
    return MatrixToQuaternion(m);
}

// Transform.rotation composition of the native Unity code (UnityPlayer.dll): Quaternionf * Quaternionf with one float32 rounding per
// operation (not the Mono rule of the managed Quaternion.operator*, which rounds a whole expression once). The evaluation order is the
// one that reproduces the in-game stage delta-v (capture.deltav_stages, WP-17b): cross product first, then the two w * v terms,
// w = lw * rw - ((lx * rx + ly * ry) + lz * rz):
//   x = ((ly * rz - lz * ry) + lw * rx) + rw * lx    (y, z cyclic)
// The orders  lw*rx + lx*rw + ly*rz - lz*ry  (the C# source order) and the other permutations of the three terms do not.
// WP-17b: world rotation of a thrust transform = (part chain rotation) * (model chain rotation).
Quaternionf Multiply(const Quaternionf& lhs, const Quaternionf& rhs) {
    const float L[4] = {lhs.x, lhs.y, lhs.z, lhs.w}, R[4] = {rhs.x, rhs.y, rhs.z, rhs.w};
    float out[4];
    for (int c = 0; c < 3; c++) {
        const int j = (c + 1) % 3, k = (c + 2) % 3;
        volatile float m0 = L[j] * R[k], m1 = L[k] * R[j];  // volatile: keep the per-operation rounding (no FMA contraction)
        volatile float cross = m0 - m1;
        volatile float a0 = L[3] * R[c], a1 = R[3] * L[c];
        volatile float acc = cross + a0;
        acc = acc + a1;
        out[c] = acc;
    }
    volatile float d0 = L[0] * R[0], d1 = L[1] * R[1], d2 = L[2] * R[2], dd = d0 + d1;
    dd = dd + d2;
    volatile float ww = L[3] * R[3];
    out[3] = ww - dd;
    return Quaternionf(out[0], out[1], out[2], out[3]);
}

// Runtime/Math/Quaternion.h: Inverse(q) = (-x, -y, -z, w) (for the unit quaternions a Transform stores)
Quaternionf Inverse(const Quaternionf& q) { return Quaternionf(-q.x, -q.y, -q.z, q.w); }

// Runtime/Math/Quaternion.h: Normalize(q) = q / Magnitude(q), Magnitude = sqrt(Dot(q, q)) (float, per operation)
Quaternionf Normalize(const Quaternionf& q) {
    volatile float d0 = q.x * q.x, d1 = q.y * q.y, d2 = q.z * q.z, d3 = q.w * q.w;
    volatile float s = d0 + d1;
    s = s + d2;
    s = s + d3;
    const float mag = std::sqrt(s);
    return Quaternionf(q.x / mag, q.y / mag, q.z / mag, q.w / mag);
}

}  // namespace ksp::UnityNative
