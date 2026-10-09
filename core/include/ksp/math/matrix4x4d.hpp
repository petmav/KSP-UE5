// Matrix4x4D (Assembly-CSharp/Matrix4x4D.cs): KSP's double 4x4 matrix. Row-vector convention for TransformVector /
// TransformPoint (v * M, translation in row 3), column-vector convention for MultiplyPoint3x4 (M * v, translation in
// column 3) -- both exactly as in KSP. The builder methods (Translate, Rotate*axis, Scale, SetIdentity, Zero,
// Inverse) overwrite *this and return a copy, like the C# struct methods.
#pragma once

#include "ksp/math/vector3d.hpp"
#include "ksp/unity/color.hpp"

namespace ksp {

struct Matrix4x4D {
    double m00 = 0, m01 = 0, m02 = 0, m03 = 0;
    double m10 = 0, m11 = 0, m12 = 0, m13 = 0;
    double m20 = 0, m21 = 0, m22 = 0, m23 = 0;
    double m30 = 0, m31 = 0, m32 = 0, m33 = 0;

    constexpr Matrix4x4D() = default;
    constexpr Matrix4x4D(double p00, double p01, double p02, double p03, double p10, double p11, double p12, double p13,
                         double p20, double p21, double p22, double p23, double p30, double p31, double p32, double p33)
        : m00(p00), m01(p01), m02(p02), m03(p03), m10(p10), m11(p11), m12(p12), m13(p13),
          m20(p20), m21(p21), m22(p22), m23(p23), m30(p30), m31(p31), m32(p32), m33(p33) {}
    // Matrix4x4D(Matrix4x4) / explicit operator Matrix4x4D(Matrix4x4)
    explicit Matrix4x4D(const Matrix4x4f& m)
        : Matrix4x4D(m.m00, m.m01, m.m02, m.m03, m.m10, m.m11, m.m12, m.m13, m.m20, m.m21, m.m22, m.m23, m.m30, m.m31,
                     m.m32, m.m33) {}

    static constexpr Matrix4x4D Identity() { return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; }

    Matrix4x4D Translate(double dx, double dy, double dz);
    Matrix4x4D RotateXaxis(double angle);
    Matrix4x4D RotateYaxis(double angle);
    Matrix4x4D RotateZaxis(double angle);
    Matrix4x4D RotateXaxis(double sina, double cosa);
    Matrix4x4D RotateYaxis(double sina, double cosa);
    Matrix4x4D RotateZaxis(double sina, double cosa);
    Matrix4x4D Scale(double factor);
    Matrix4x4D Scale(double sx, double sy, double sz);
    Matrix4x4D Zero();
    Matrix4x4D SetIdentity();
    Matrix4x4D Inverse();
    double Det() const;

    Vector3d TransformVector(const Vector3d& v) const;
    Vector3d TransformPoint(const Vector3d& v) const;
    Vector3d MultiplyPoint3x4(const Vector3d& v) const;
};

Matrix4x4D operator+(const Matrix4x4D& a, const Matrix4x4D& b);
Matrix4x4D operator-(const Matrix4x4D& a, const Matrix4x4D& b);
Matrix4x4D operator*(const Matrix4x4D& m1, const Matrix4x4D& m2);

}  // namespace ksp
