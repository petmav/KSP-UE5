// WP-20b / WP-20c: the records of VesselPhysics shared by its translation units (vessel_physics.cpp, vessel_physics_loads.cpp).
// Engine glue in double (no ported KSP statements here).
#pragma once

#include <cmath>
#include <vector>

#include "ksp/physics/vessel_physics.hpp"

namespace ksp::physics::detail {

// ---- 3x3 double algebra ----------------------------------------------------------------------------------------------------
struct Mat3 {
    double m[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    static Mat3 Zero() { return {}; }
    static Mat3 Rotation(const QuaternionD& q) {
        const double x = q.x, y = q.y, z = q.z, w = q.w;
        Mat3 r;
        r.m[0][0] = 1.0 - 2.0 * (y * y + z * z);
        r.m[0][1] = 2.0 * (x * y - z * w);
        r.m[0][2] = 2.0 * (x * z + y * w);
        r.m[1][0] = 2.0 * (x * y + z * w);
        r.m[1][1] = 1.0 - 2.0 * (x * x + z * z);
        r.m[1][2] = 2.0 * (y * z - x * w);
        r.m[2][0] = 2.0 * (x * z - y * w);
        r.m[2][1] = 2.0 * (y * z + x * w);
        r.m[2][2] = 1.0 - 2.0 * (x * x + y * y);
        return r;
    }
    // R diag(d) R^T
    static Mat3 RotatedDiagonal(const Mat3& R, const Vector3d& d) {
        Mat3 o;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) o.m[i][j] = R.m[i][0] * d.x * R.m[j][0] + R.m[i][1] * d.y * R.m[j][1] + R.m[i][2] * d.z * R.m[j][2];
        return o;
    }
    // m (|r|^2 E - r r^T): the parallel-axis term
    static Mat3 ParallelAxis(double mass, const Vector3d& r) {
        const double v[3] = {r.x, r.y, r.z};
        const double r2 = r.x * r.x + r.y * r.y + r.z * r.z;
        Mat3 o;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) o.m[i][j] = mass * ((i == j ? r2 : 0.0) - v[i] * v[j]);
        return o;
    }
    Mat3& operator+=(const Mat3& b) {
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) m[i][j] += b.m[i][j];
        return *this;
    }
    Mat3 operator*(double s) const {
        Mat3 o;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) o.m[i][j] = m[i][j] * s;
        return o;
    }
    Vector3d operator*(const Vector3d& v) const {
        return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z, m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z};
    }
    // R A R^T
    Mat3 Rotated(const Mat3& R) const {
        Mat3 t;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) t.m[i][j] = m[i][0] * R.m[j][0] + m[i][1] * R.m[j][1] + m[i][2] * R.m[j][2];  // A R^T
        Mat3 o;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) o.m[i][j] = R.m[i][0] * t.m[0][j] + R.m[i][1] * t.m[1][j] + R.m[i][2] * t.m[2][j];
        return o;
    }
    // A^-1 b for a symmetric positive (semi)definite A (cofactor inverse); 0 when A is singular
    Vector3d Solve(const Vector3d& b) const {
        const double a = m[0][0], bb = m[0][1], c = m[0][2], d = m[1][0], e = m[1][1], f = m[1][2], g = m[2][0], h = m[2][1], k = m[2][2];
        const double A = e * k - f * h, B = -(d * k - f * g), C = d * h - e * g;
        const double det = a * A + bb * B + c * C;
        if (!(std::fabs(det) > 0.0)) return {};
        const double D = -(bb * k - c * h), E = a * k - c * g, F = -(a * h - bb * g);
        const double G = bb * f - c * e, H = -(a * f - c * d), K = a * e - bb * d;
        const double inv = 1.0 / det;
        return {(A * b.x + D * b.y + G * b.z) * inv, (B * b.x + E * b.y + H * b.z) * inv, (C * b.x + F * b.y + K * b.z) * inv};
    }
};

inline Vector3d Cross(const Vector3d& a, const Vector3d& b) { return Vector3d::Cross(a, b); }

inline QuaternionD Normalized(const QuaternionD& q) {
    const double n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (n == 1.0 || !(n > 0.0)) return q;
    return {q.x / n, q.y / n, q.z / n, q.w / n};
}

}  // namespace ksp::physics::detail

namespace ksp::physics {

struct VesselPhysics::PartRec {
    Part* part = nullptr;
    int owner = -1;               // RigidBodyPart (Part.cs:1184) index, -1 without one
    bool rb = false;              // part.rb != null
    float rbMass = 0.0f;          // rb.mass
    Vector3f inertiaPerUnit;      // Part.inertiaTensor (per unit mass)
    Vector3f inertia;             // rb.inertiaTensor
    float angularDrag = 0.05f;    // rb.angularDrag
    bool sameVesselCollision = false;
    std::vector<ColliderProxy> colliders;
    int cluster = -1;
    // reference pose at the cluster's creation (physics frame; the body axes are the physics axes then)
    Vector3d pos0, com0;
    QuaternionD rot0 = QuaternionD::identity();
    Vector3d localPos, localCom;  // pos0 / com0 relative to the body origin, body space
    detail::Mat3 ownBody;         // rot0 diag(inertia) rot0^T: the part's own tensor in body space
    // the pose now (refreshed after every step / rebuild: RefreshPartPoses)
    Vector3d posNow, comNow;
    QuaternionD rotNow = QuaternionD::identity();
    // WP-20c: the vessel-frame Transform (orgPos / orgRot) the PartJoint data is measured in
    PartTransform vesselFrame;
};

struct VesselPhysics::ClusterRec {
    BodyId body = kInvalidBody;
    std::vector<int> parts;  // vessel part order
    Vector3d origin0;        // body origin at creation (physics frame)
    double mass = 0.0;
    Vector3d comBody;        // centre of mass relative to the origin, body space (0 at creation)
    detail::Mat3 inertia;    // about the centre of mass, body axes
    // cached after each step
    Vector3d origin, com, v, w;
    QuaternionD q = QuaternionD::identity();
    // accumulated this tick: force and moment about `com`
    Vector3d force, moment;
};

}  // namespace ksp::physics
