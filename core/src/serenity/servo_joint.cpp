// WP-52: the joint data model of the robotic servos (see include/ksp/serenity/servo_joint.hpp for the decision and the UNVERIFIED list).
#include "ksp/serenity/servo_joint.hpp"

#include <algorithm>
#include <cmath>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp::Expansions::Serenity {

namespace {
// Matrix4x4f stores m{row}{col}; arrays make the product loops readable
void ToArray(const Matrix4x4f& m, float a[4][4]) {
    a[0][0] = m.m00; a[0][1] = m.m01; a[0][2] = m.m02; a[0][3] = m.m03;
    a[1][0] = m.m10; a[1][1] = m.m11; a[1][2] = m.m12; a[1][3] = m.m13;
    a[2][0] = m.m20; a[2][1] = m.m21; a[2][2] = m.m22; a[2][3] = m.m23;
    a[3][0] = m.m30; a[3][1] = m.m31; a[3][2] = m.m32; a[3][3] = m.m33;
}
Matrix4x4f FromArray(const float a[4][4]) {
    Matrix4x4f m;
    m.m00 = a[0][0]; m.m01 = a[0][1]; m.m02 = a[0][2]; m.m03 = a[0][3];
    m.m10 = a[1][0]; m.m11 = a[1][1]; m.m12 = a[1][2]; m.m13 = a[1][3];
    m.m20 = a[2][0]; m.m21 = a[2][1]; m.m22 = a[2][2]; m.m23 = a[2][3];
    m.m30 = a[3][0]; m.m31 = a[3][1]; m.m32 = a[3][2]; m.m33 = a[3][3];
    return m;
}
}  // namespace

// Matrix4x4.cs:328 operator*(Matrix4x4, Matrix4x4): each element is a float store of the double sum of four float products (Mono rule)
Matrix4x4f MatrixMultiply(const Matrix4x4f& lhs, const Matrix4x4f& rhs) {
    float a[4][4], b[4][4], r[4][4];
    ToArray(lhs, a);
    ToArray(rhs, b);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r[i][j] = f32store(fx(a[i][0]) * fx(b[0][j]) + fx(a[i][1]) * fx(b[1][j]) + fx(a[i][2]) * fx(b[2][j]) + fx(a[i][3]) * fx(b[3][j]));
    return FromArray(r);
}

// Matrix4x4.cs:350 operator*(Matrix4x4, Vector4) with the implicit Vector3 -> Vector4 (w = 0) and Vector4 -> Vector3 (w dropped)
Vector3f MatrixMultiplyDirection(const Matrix4x4f& lhs, const Vector3f& v) {
    return {f32store(fx(lhs.m00) * fx(v.x) + fx(lhs.m01) * fx(v.y) + fx(lhs.m02) * fx(v.z) + fx(lhs.m03) * 0.0),
            f32store(fx(lhs.m10) * fx(v.x) + fx(lhs.m11) * fx(v.y) + fx(lhs.m12) * fx(v.z) + fx(lhs.m13) * 0.0),
            f32store(fx(lhs.m20) * fx(v.x) + fx(lhs.m21) * fx(v.y) + fx(lhs.m22) * fx(v.z) + fx(lhs.m23) * 0.0)};
}

// Matrix4x4.inverse (native): double Gauss-Jordan with partial pivoting, rounded to float; a singular matrix gives zero (Unity's convention)
Matrix4x4f MatrixInverse(const Matrix4x4f& m) {
    float f[4][4];
    ToArray(m, f);
    double a[4][8];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            a[i][j] = f[i][j];
            a[i][4 + j] = (i == j) ? 1.0 : 0.0;
        }
    for (int c = 0; c < 4; c++) {
        int piv = c;
        for (int r = c + 1; r < 4; r++)
            if (std::fabs(a[r][c]) > std::fabs(a[piv][c])) piv = r;
        if (a[piv][c] == 0.0) return Matrix4x4f();
        if (piv != c)
            for (int j = 0; j < 8; j++) std::swap(a[piv][j], a[c][j]);
        const double d = a[c][c];
        for (int j = 0; j < 8; j++) a[c][j] /= d;
        for (int r = 0; r < 4; r++) {
            if (r == c) continue;
            const double k = a[r][c];
            if (k == 0.0) continue;
            for (int j = 0; j < 8; j++) a[r][j] -= k * a[c][j];
        }
    }
    float out[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) out[i][j] = static_cast<float>(a[i][4 + j]);
    return FromArray(out);
}

namespace {
// ConfigurableJointExtensions.cs:23 SetTargetRotationInternal
void SetTargetRotationInternal(ConfigurableJointData& joint, const Quaternionf& targetRotation, const Quaternionf& startRotation, bool worldSpace) {
    const Vector3f axis = joint.axis;                                                    // :25
    const Vector3f normalized = Vector3f::Cross(joint.axis, joint.secondaryAxis).normalized();  // :26
    const Vector3f normalized2 = Vector3f::Cross(normalized, axis).normalized();         // :27
    const Quaternionf quaternion = UnityNative::LookRotation(normalized, normalized2);   // :28
    Quaternionf targetRotation2 = UnityNative::Inverse(quaternion);                      // :29
    if (worldSpace)
        targetRotation2 = targetRotation2 * (startRotation * UnityNative::Inverse(targetRotation));  // :32
    else
        targetRotation2 = targetRotation2 * (UnityNative::Inverse(targetRotation) * startRotation);   // :36
    targetRotation2 = targetRotation2 * quaternion;                                      // :38
    joint.targetRotation = targetRotation2;                                              // :39
}
}  // namespace

// ConfigurableJointExtensions.cs:5
void SetTargetRotationLocal(ConfigurableJointData& joint, const Quaternionf& targetLocalRotation, const Quaternionf& startLocalRotation) {
    if (joint.configuredInWorldSpace)
        log::Error("SetTargetRotationLocal should not be used with joints that are configured in world space. For world space joints, use SetTargetRotation.");
    SetTargetRotationInternal(joint, targetLocalRotation, startLocalRotation, false);
}

// ConfigurableJointExtensions.cs:14
void SetTargetRotationWorld(ConfigurableJointData& joint, const Quaternionf& targetWorldRotation, const Quaternionf& startWorldRotation) {
    if (!joint.configuredInWorldSpace)
        log::Error("SetTargetRotation must be used with joints that are configured in world space. For local space joints, use SetTargetRotationLocal.");
    SetTargetRotationInternal(joint, targetWorldRotation, startWorldRotation, true);
}

// UNVERIFIED native: QuaternionToAxisAngle (angle = 2 acos(w); the xAxis when the angle is ~0; axis = xyz / sqrt(1 - w^2)), then the managed `* 57.29578f`
void ToAngleAxis(const Quaternionf& q, float& angle, Vector3f& axis) {
    float w = q.w;
    // the native asserts a unit quaternion; a non-unit w is clamped so acos stays defined
    if (w > 1.0f) w = 1.0f;
    if (w < -1.0f) w = -1.0f;
    const float a = static_cast<float>(2.0 * std::acos(static_cast<double>(w)));
    if (std::fabs(a - 0.0f) < 1e-6f) {
        axis = Vector3f::right();
    } else {
        const float div = 1.0f / UnityNative::SqrtF(1.0f - w * w);
        axis = Vector3f(q.x * div, q.y * div, q.z * div);
    }
    angle = f32store(fx(a) * fx(57.29578f));
}

// The physical coordinate of the drive. Derivation: SetTargetRotationInternal (local space) builds
//     targetRotation = L^-1 * target^-1 * start * L,   L = LookRotation(normalize(axis x secondary), ...)
// where L maps the joint's X axis onto `axis`. For start = identity and target = AngleAxis(a, axis) that is a rotation by -a about the joint's
// X axis, so the twist of targetRotation about X is -a and the physical angle the extension is built to reach is a = -twist.
double PhysicalTargetAngleDegrees(const ConfigurableJointData& joint) {
    const Quaternionf& q = joint.targetRotation;
    const double twist = 2.0 * std::atan2(static_cast<double>(q.x), static_cast<double>(q.w)) * (180.0 / 3.14159265358979323846);
    double a = -twist;
    while (a > 180.0) a -= 360.0;
    while (a < -180.0) a += 360.0;
    return a;
}

// documented model of the D6 drive (servo_joint.hpp)
void StepDrive1D(ServoPlantState& s, const ServoPlantParams& p, const JointDrive& drive, double target, double targetVelocity, bool limited, double limitLo,
                 double limitHi, double dt) {
    const double k = drive.positionSpring, c = drive.positionDamper, m = p.inertia;
    const double dm = dt / m;
    double v1 = (s.velocity + dm * (k * (target - s.position) + c * targetVelocity)) / (1.0 + dm * (c + dt * k));
    double force = m * (v1 - s.velocity) / dt;
    const double fmax = drive.maximumForce;
    if (std::fabs(force) > fmax) {
        force = force < 0.0 ? -fmax : fmax;
        v1 = s.velocity + dt * force / m;
    }
    if (p.brakeFreeDamping != 0.0) v1 -= v1 * std::min(1.0, p.brakeFreeDamping * dt);
    double x1 = s.position + dt * v1;
    if (limited) {
        if (x1 < limitLo) {
            x1 = limitLo;
            if (v1 < 0.0) v1 = 0.0;
        } else if (x1 > limitHi) {
            x1 = limitHi;
            if (v1 > 0.0) v1 = 0.0;
        }
    }
    s.position = x1;
    s.velocity = v1;
    s.lastForce = force;
}

}  // namespace ksp::Expansions::Serenity
