// Native Unity quaternion functions (see include/ksp/unity/quaternion_native.hpp).
#include "ksp/unity/quaternion_native.hpp"

#include <cmath>

#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/ucrt_libm.hpp"

namespace ksp::UnityNative {

// QuaternionScripting::AngleAxis -> AxisAngleToQuaternionSafe(axis, Deg2Rad(angle)) (Runtime/Math/Quaternion.h)
Quaternionf AngleAxis(float angle, const Vector3f& axis) {
    // Unity's Deg2Rad (Runtime/Math/FloatConversion.h) keeps a historical operation order: deg / 360 * 2 * kPI.
    const float rad = angle / 360.0f * 2.0f * 3.14159265358979323846f;
    const float mag = SqrtF(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
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
        const float invlen = 1.0f / SqrtF(left[0] * left[0] + left[1] * left[1] + left[2] * left[2]);
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
        float root = SqrtF(trace + 1.0f);
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
        float root = SqrtF(m[i][i] - m[j][j] - m[k][k] + 1.0f);
        float* quat[3] = {&q.x, &q.y, &q.z};
        *quat[i] = 0.5f * root;
        root = 0.5f / root;
        q.w = (m[k][j] - m[j][k]) * root;
        *quat[j] = (m[j][i] + m[i][j]) * root;
        *quat[k] = (m[k][i] + m[i][k]) * root;
    }
    const float mag = SqrtF(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    q.x /= mag;
    q.y /= mag;
    q.z /= mag;
    q.w /= mag;
    return q;
}

}  // namespace

// QuaternionScripting::FromToRotation -> FromToQuaternionSafe (Runtime/Math/Quaternion.h)
Quaternionf FromToRotation(const Vector3f& fromDirection, const Vector3f& toDirection) {
    const float lhsMag = SqrtF(fromDirection.x * fromDirection.x + fromDirection.y * fromDirection.y + fromDirection.z * fromDirection.z);
    const float rhsMag = SqrtF(toDirection.x * toDirection.x + toDirection.y * toDirection.y + toDirection.z * toDirection.z);
    if (lhsMag < 0.00001f || rhsMag < 0.00001f) return Quaternionf::identity();
    const float from[3] = {fromDirection.x / lhsMag, fromDirection.y / lhsMag, fromDirection.z / lhsMag};
    const float to[3] = {toDirection.x / rhsMag, toDirection.y / rhsMag, toDirection.z / rhsMag};
    float m[3][3];
    FromToRotationMatrix(from, to, m);
    return MatrixToQuaternion(m);
}

}  // namespace ksp::UnityNative

// ---- WP-40 -------------------------------------------------------------------------------------------------------------------------
namespace ksp::UnityNative {

namespace {

// Runtime/Math/Quaternion.cpp operator*(lhs, rhs): per-operation float32 (native code)
Quaternionf NativeMul(const Quaternionf& lhs, const Quaternionf& rhs) {
    Quaternionf r;
    const float x = lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y;
    const float y = lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z;
    const float z = lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x;
    const float w = lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z;
    r.x = x;
    r.y = y;
    r.z = z;
    r.w = w;
    return r;
}

// the CRT atan2f of native code is IEEE: a zero result keeps the sign of y (the Mono library the dotnet shim follows returns +0)
float NativeAtan2f(float y, float x) {
    double r = ksp::dotnet::Math::Atan2(static_cast<double>(y), static_cast<double>(x));
    if (r == 0.0) r = std::copysign(0.0, static_cast<double>(y));
    return static_cast<float>(r);
}

}  // namespace

// QuaternionScripting::LookRotation -> LookRotationToQuaternion (Runtime/Math/Quaternion.cpp) / LookRotationToMatrix (Matrix3x3.cpp)
Quaternionf LookRotation(const Vector3f& forward, const Vector3f& up) {
    constexpr float kEps = 0.00001f;
    float z[3] = {forward.x, forward.y, forward.z};
    float mag = SqrtF(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
    if (mag < kEps) return Quaternionf::identity();
    z[0] /= mag;
    z[1] /= mag;
    z[2] /= mag;
    float x[3] = {up.y * z[2] - up.z * z[1], up.z * z[0] - up.x * z[2], up.x * z[1] - up.y * z[0]};
    mag = SqrtF(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    if (mag < kEps) return Quaternionf::identity();
    x[0] /= mag;
    x[1] /= mag;
    x[2] /= mag;
    const float y[3] = {z[1] * x[2] - z[2] * x[1], z[2] * x[0] - z[0] * x[2], z[0] * x[1] - z[1] * x[0]};
    const float sqrY = y[0] * y[0] + y[1] * y[1] + y[2] * y[2];
    float dist = sqrY - 1.0f;
    if (dist < 0.0f) dist = -dist;
    if (!(dist < 0.000001f)) return Quaternionf::identity();
    // SetOrthoNormalBasis: the columns are x, y, z
    float m[3][3];
    for (int r = 0; r < 3; r++) {
        m[r][0] = x[r];
        m[r][1] = y[r];
        m[r][2] = z[r];
    }
    return MatrixToQuaternion(m);
}

// EulerToQuaternion (Runtime/Math/Quaternion.cpp), the angles in radians
Quaternionf Euler(const Vector3f& eulerDegrees) {
    // Quaternion.Euler: euler * ((float)Math.PI / 180f), a managed Vector3 * float (each product rounded to float)
    const float deg2rad = 0.0174532924f;
    const float ex = eulerDegrees.x * deg2rad;
    const float ey = eulerDegrees.y * deg2rad;
    const float ez = eulerDegrees.z * deg2rad;
    const float cX = ksp::dotnet::NativeCosf(ex / 2.0f), sX = ksp::dotnet::NativeSinf(ex / 2.0f);
    const float cY = ksp::dotnet::NativeCosf(ey / 2.0f), sY = ksp::dotnet::NativeSinf(ey / 2.0f);
    const float cZ = ksp::dotnet::NativeCosf(ez / 2.0f), sZ = ksp::dotnet::NativeSinf(ez / 2.0f);
    const Quaternionf qX(sX, 0.0f, 0.0f, cX);
    const Quaternionf qY(0.0f, sY, 0.0f, cY);
    const Quaternionf qZ(0.0f, 0.0f, sZ, cZ);
    return NativeMul(NativeMul(qY, qX), qZ);
}

// QuaternionScripting::ToEuler -> QuaternionToEuler: QuaternionToMatrix then MatrixToEuler (the YXZ decomposition of
// geometrictools.com EulerAngles.pdf with the 0.999 fudge); asin / atan2 are the CRT's float functions (here: the double functions
// rounded to float, the same value except in the rare half-way cases).
Vector3f EulerAngles(const Quaternionf& q) {
    const float x2 = q.x * 2.0f, y2 = q.y * 2.0f, z2 = q.z * 2.0f;
    const float xx = q.x * x2, yy = q.y * y2, zz = q.z * z2;
    const float xy = q.x * y2, xz = q.x * z2, yz = q.y * z2;
    const float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;
    // Matrix3x3f::Get(row, col)
    const float m00 = 1.0f - (yy + zz), m01 = xy - wz, m02 = xz + wy;
    const float m10 = xy + wz, m11 = 1.0f - (xx + zz), m12 = yz - wx;
    const float m20 = xz - wy, m21 = yz + wx, m22 = 1.0f - (xx + yy);
    (void)m21;
    constexpr float kPI = 3.14159265358979323846f;
    float vx, vy, vz;
    if (m12 < 0.999f) {
        if (m12 > -0.999f) {
            vx = static_cast<float>(ksp::dotnet::Math::Asin(static_cast<double>(-m12)));
            vy = NativeAtan2f(m02, m22);
            vz = NativeAtan2f(m10, m11);
        } else {
            vx = kPI * 0.5f;
            vy = NativeAtan2f(m01, m00);
            vz = 0.0f;
        }
    } else {
        vx = -kPI * 0.5f;
        vy = NativeAtan2f(-m01, m00);
        vz = 0.0f;
    }
    (void)m20;
    // eulerAngles: Internal_MakePositive(Internal_ToEulerRad(this) * 57.29578f)
    float e[3] = {vx * 57.29578f, vy * 57.29578f, vz * 57.29578f};
    const float lo = -0.005729578f;
    const float hi = 360.0f + lo;
    for (float& c : e) {
        if (c < lo) c += 360.0f;
        else if (c > hi) c -= 360.0f;
    }
    return {e[0], e[1], e[2]};
}

}  // namespace ksp::UnityNative
