// Port of Assembly-CSharp/Matrix4x4D.cs
#include "ksp/math/matrix4x4d.hpp"

#include "ksp/dotnet/math.hpp"

namespace ksp {

namespace {
// Matrix4x4D.cs: det2x2 / det3x3
inline double det2x2(double a, double b, double c, double d) { return a * d - b * c; }
inline double det3x3(double a1, double a2, double a3, double b1, double b2, double b3, double c1, double c2, double c3) {
    return a1 * det2x2(b2, b3, c2, c3) - b1 * det2x2(a2, a3, c2, c3) + c1 * det2x2(a2, a3, b2, b3);
}
}  // namespace

Matrix4x4D Matrix4x4D::Translate(double dx, double dy, double dz) {
    SetIdentity();
    m30 = dx;
    m31 = dy;
    m32 = dz;
    return *this;
}

Matrix4x4D Matrix4x4D::RotateXaxis(double angle) { return RotateXaxis(dotnet::Math::Sin(angle), dotnet::Math::Cos(angle)); }
Matrix4x4D Matrix4x4D::RotateYaxis(double angle) { return RotateYaxis(dotnet::Math::Sin(angle), dotnet::Math::Cos(angle)); }
Matrix4x4D Matrix4x4D::RotateZaxis(double angle) { return RotateZaxis(dotnet::Math::Sin(angle), dotnet::Math::Cos(angle)); }

Matrix4x4D Matrix4x4D::RotateXaxis(double sina, double cosa) {
    SetIdentity();
    m11 = cosa;
    m12 = sina;
    m21 = 0.0 - sina;
    m22 = cosa;
    return *this;
}

Matrix4x4D Matrix4x4D::RotateYaxis(double sina, double cosa) {
    SetIdentity();
    m00 = cosa;
    m02 = 0.0 - sina;
    m20 = sina;
    m22 = cosa;
    return *this;
}

Matrix4x4D Matrix4x4D::RotateZaxis(double sina, double cosa) {
    SetIdentity();
    m00 = cosa;
    m01 = sina;
    m10 = 0.0 - sina;
    m11 = cosa;
    return *this;
}

Matrix4x4D Matrix4x4D::Scale(double factor) {
    SetIdentity();
    m00 = factor;
    m11 = factor;
    m22 = factor;
    return *this;
}

Matrix4x4D Matrix4x4D::Scale(double sx, double sy, double sz) {
    SetIdentity();
    m00 = sx;
    m11 = sy;
    m22 = sz;
    return *this;
}

Matrix4x4D Matrix4x4D::Zero() {
    *this = Matrix4x4D();
    return *this;
}

Matrix4x4D Matrix4x4D::SetIdentity() {
    *this = Identity();
    return *this;
}

// Matrix4x4D.cs: Inverse (cofactors / determinant, column-major cofactor layout as in KSP)
Matrix4x4D Matrix4x4D::Inverse() {
    const double num = Det();
    const double a = m00, num2 = m10, num3 = m20, c = m30;
    const double num4 = m01, num5 = m11, num6 = m21, num7 = m31;
    const double num8 = m02, num9 = m12, num10 = m22, num11 = m32;
    const double a2 = m03, num12 = m13, num13 = m23, c2 = m33;
    m00 = det3x3(num5, num9, num12, num6, num10, num13, num7, num11, c2) / num;
    m01 = (0.0 - det3x3(num4, num8, a2, num6, num10, num13, num7, num11, c2)) / num;
    m02 = det3x3(num4, num8, a2, num5, num9, num12, num7, num11, c2) / num;
    m03 = (0.0 - det3x3(num4, num8, a2, num5, num9, num12, num6, num10, num13)) / num;
    m10 = (0.0 - det3x3(num2, num9, num12, num3, num10, num13, c, num11, c2)) / num;
    m11 = det3x3(a, num8, a2, num3, num10, num13, c, num11, c2) / num;
    m12 = (0.0 - det3x3(a, num8, a2, num2, num9, num12, c, num11, c2)) / num;
    m13 = det3x3(a, num8, a2, num2, num9, num12, num3, num10, num13) / num;
    m20 = det3x3(num2, num5, num12, num3, num6, num13, c, num7, c2) / num;
    m21 = (0.0 - det3x3(a, num4, a2, num3, num6, num13, c, num7, c2)) / num;
    m22 = det3x3(a, num4, a2, num2, num5, num12, c, num7, c2) / num;
    m23 = (0.0 - det3x3(a, num4, a2, num2, num5, num12, num3, num6, num13)) / num;
    m30 = (0.0 - det3x3(num2, num5, num9, num3, num6, num10, c, num7, num11)) / num;
    m31 = det3x3(a, num4, num8, num3, num6, num10, c, num7, num11) / num;
    m32 = (0.0 - det3x3(a, num4, num8, num2, num5, num9, c, num7, num11)) / num;
    m33 = det3x3(a, num4, num8, num2, num5, num9, num3, num6, num10) / num;
    return *this;
}

// Matrix4x4D.cs: Det
double Matrix4x4D::Det() const {
    const double num = m00, num2 = m10, num3 = m20, num4 = m30;
    const double a = m01, num5 = m11, num6 = m21, c = m31;
    const double a2 = m02, num7 = m12, num8 = m22, c2 = m32;
    const double a3 = m03, num9 = m13, num10 = m23, c3 = m33;
    return num * det3x3(num5, num7, num9, num6, num8, num10, c, c2, c3) - num2 * det3x3(a, a2, a3, num6, num8, num10, c, c2, c3) +
           num3 * det3x3(a, a2, a3, num5, num7, num9, c, c2, c3) - num4 * det3x3(a, a2, a3, num5, num7, num9, num6, num8, num10);
}

Vector3d Matrix4x4D::TransformVector(const Vector3d& v) const {
    return {v.x * m00 + v.y * m10 + v.z * m20, v.x * m01 + v.y * m11 + v.z * m21, v.x * m02 + v.y * m12 + v.z * m22};
}

Vector3d Matrix4x4D::TransformPoint(const Vector3d& v) const {
    return {v.x * m00 + v.y * m10 + v.z * m20 + m30, v.x * m01 + v.y * m11 + v.z * m21 + m31,
            v.x * m02 + v.y * m12 + v.z * m22 + m32};
}

Vector3d Matrix4x4D::MultiplyPoint3x4(const Vector3d& v) const {
    Vector3d result;
    result.x = m00 * v.x + m01 * v.y + m02 * v.z + m03;
    result.y = m10 * v.x + m11 * v.y + m12 * v.z + m13;
    result.z = m20 * v.x + m21 * v.y + m22 * v.z + m23;
    return result;
}

Matrix4x4D operator+(const Matrix4x4D& a, const Matrix4x4D& b) {
    return {a.m00 + b.m00, a.m01 + b.m01, a.m02 + b.m02, a.m03 + b.m03, a.m10 + b.m10, a.m11 + b.m11,
            a.m12 + b.m12, a.m13 + b.m13, a.m20 + b.m20, a.m21 + b.m21, a.m22 + b.m22, a.m23 + b.m23,
            a.m30 + b.m30, a.m31 + b.m31, a.m32 + b.m32, a.m33 + b.m33};
}

Matrix4x4D operator-(const Matrix4x4D& a, const Matrix4x4D& b) {
    return {a.m00 - b.m00, a.m01 - b.m01, a.m02 - b.m02, a.m03 - b.m03, a.m10 - b.m10, a.m11 - b.m11,
            a.m12 - b.m12, a.m13 - b.m13, a.m20 - b.m20, a.m21 - b.m21, a.m22 - b.m22, a.m23 - b.m23,
            a.m30 - b.m30, a.m31 - b.m31, a.m32 - b.m32, a.m33 - b.m33};
}

// Matrix4x4D.cs: operator*
Matrix4x4D operator*(const Matrix4x4D& m1, const Matrix4x4D& m2) {
    Matrix4x4D r;
    r.m00 = m1.m00 * m2.m00 + m1.m01 * m2.m10 + m1.m02 * m2.m20 + m1.m03 * m2.m30;
    r.m01 = m1.m00 * m2.m01 + m1.m01 * m2.m11 + m1.m02 * m2.m21 + m1.m03 * m2.m31;
    r.m02 = m1.m00 * m2.m02 + m1.m01 * m2.m12 + m1.m02 * m2.m22 + m1.m03 * m2.m32;
    r.m03 = m1.m00 * m2.m03 + m1.m01 * m2.m13 + m1.m02 * m2.m23 + m1.m03 * m2.m33;
    r.m10 = m1.m10 * m2.m00 + m1.m11 * m2.m10 + m1.m12 * m2.m20 + m1.m13 * m2.m30;
    r.m11 = m1.m10 * m2.m01 + m1.m11 * m2.m11 + m1.m12 * m2.m21 + m1.m13 * m2.m31;
    r.m12 = m1.m10 * m2.m02 + m1.m11 * m2.m12 + m1.m12 * m2.m22 + m1.m13 * m2.m32;
    r.m13 = m1.m10 * m2.m03 + m1.m11 * m2.m13 + m1.m12 * m2.m23 + m1.m13 * m2.m33;
    r.m20 = m1.m20 * m2.m00 + m1.m21 * m2.m10 + m1.m22 * m2.m20 + m1.m23 * m2.m30;
    r.m21 = m1.m20 * m2.m01 + m1.m21 * m2.m11 + m1.m22 * m2.m21 + m1.m23 * m2.m31;
    r.m22 = m1.m20 * m2.m02 + m1.m21 * m2.m12 + m1.m22 * m2.m22 + m1.m23 * m2.m32;
    r.m23 = m1.m20 * m2.m03 + m1.m21 * m2.m13 + m1.m22 * m2.m23 + m1.m23 * m2.m33;
    r.m30 = m1.m30 * m2.m00 + m1.m31 * m2.m10 + m1.m32 * m2.m20 + m1.m33 * m2.m30;
    r.m31 = m1.m30 * m2.m01 + m1.m31 * m2.m11 + m1.m32 * m2.m21 + m1.m33 * m2.m31;
    r.m32 = m1.m30 * m2.m02 + m1.m31 * m2.m12 + m1.m32 * m2.m22 + m1.m33 * m2.m32;
    r.m33 = m1.m30 * m2.m03 + m1.m31 * m2.m13 + m1.m32 * m2.m23 + m1.m33 * m2.m33;
    return r;
}

}  // namespace ksp
