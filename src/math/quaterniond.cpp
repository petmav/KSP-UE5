#include "ksp/math/quaterniond.hpp"

#include <cmath>

#include "ksp/dotnet/math.hpp"

namespace ksp {

namespace {
constexpr double kPI = 3.141592653589793;  // System.Math.PI
}

// QuaternionD.cs:50 QuaternionD(Vector3d X, Vector3d Y, Vector3d Z)
QuaternionD::QuaternionD(const Vector3d& X, const Vector3d& Y, const Vector3d& Z) {
    const double num = X.x, num2 = X.y, num3 = X.z;
    const double num4 = Y.x, num5 = Y.y, num6 = Y.z;
    const double num7 = Z.x, num8 = Z.y, num9 = Z.z;
    if (num + num5 + num9 >= 0.0) {
        const double num10 = num + num5 + num9 + 1.0;
        const double num11 = 0.5 / std::sqrt(num10);
        w = num10 * num11;
        z = (num2 - num4) * num11;
        y = (num7 - num3) * num11;
        x = (num6 - num8) * num11;
    } else if (num > num5 && num > num9) {
        const double num12 = num - num5 - num9 + 1.0;
        const double num13 = 0.5 / std::sqrt(num12);
        x = num12 * num13;
        y = (num2 + num4) * num13;
        z = (num7 + num3) * num13;
        w = (num6 - num8) * num13;
    } else if (num5 > num9) {
        const double num14 = 0.0 - num + num5 - num9 + 1.0;
        const double num15 = 0.5 / std::sqrt(num14);
        y = num14 * num15;
        x = (num2 + num4) * num15;
        w = (num7 - num3) * num15;
        z = (num6 + num8) * num15;
    } else {
        const double num16 = 0.0 - num - num5 + num9 + 1.0;
        const double num17 = 0.5 / std::sqrt(num16);
        z = num16 * num17;
        w = (num2 - num4) * num17;
        x = (num7 + num3) * num17;
        y = (num6 + num8) * num17;
    }
}

// QuaternionD.cs:88 FrameVectors
void QuaternionD::FrameVectors(Vector3d& frameX, Vector3d& frameY, Vector3d& frameZ) const {
    frameX = Vector3d(1.0 - 2.0 * y * y - 2.0 * z * z, 2.0 * x * y + 2.0 * w * z, 2.0 * x * z - 2.0 * w * y);
    frameY = Vector3d(2.0 * x * y - 2.0 * w * z, 1.0 - 2.0 * x * x - 2.0 * z * z, 2.0 * y * z + 2.0 * w * x);
    frameZ = Vector3d(2.0 * x * z + 2.0 * w * y, 2.0 * y * z - 2.0 * w * x, 1.0 - 2.0 * x * x - 2.0 * y * y);
}

// QuaternionD.cs:98 AngleAxis (degrees)
QuaternionD QuaternionD::AngleAxis(double angle, const Vector3d& axis) {
    const double magnitude = axis.magnitude();
    double num3, num4, num5, num6;
    if (magnitude > 0.0001) {
        const double num = dotnet::Math::Cos(angle * (kPI / 180.0) / 2.0);
        const double num2 = dotnet::Math::Sin(angle * (kPI / 180.0) / 2.0);
        num3 = axis.x / magnitude * num2;
        num4 = axis.y / magnitude * num2;
        num5 = axis.z / magnitude * num2;
        num6 = num;
    } else {
        num6 = 1.0;
        num3 = 0.0;
        num4 = 0.0;
        num5 = 0.0;
    }
    return {num3, num4, num5, num6};
}

// QuaternionD.cs:126 LookRotation(Vector3d forward, Vector3d up)
QuaternionD QuaternionD::LookRotation(Vector3d forward, Vector3d up) {
    QuaternionD result;
    forward = Vector3d::Normalize(forward);
    const Vector3d rhs = Vector3d::Normalize(Vector3d::Cross(up, forward));
    up = Vector3d::Cross(forward, rhs);
    const double num = forward.x, num2 = forward.y, num3 = forward.z;
    const double num4 = up.x, num5 = up.y, num6 = up.z;
    const double num7 = rhs.x, num8 = rhs.y, num9 = rhs.z;
    const double num10 = num7 + num5 + num3;
    if (num10 > 0.0) {
        double num11 = std::sqrt(num10 + 1.0);
        result.w = num11 * 0.5;
        num11 = 0.5 / num11;
        result.x = (num6 - num2) * num11;
        result.y = (num - num9) * num11;
        result.z = (num8 - num4) * num11;
        return result;
    }
    if (num7 >= num5 && num7 >= num3) {
        const double num12 = std::sqrt(1.0 + num7 - num5 - num3);
        const double num13 = 0.5 / num12;
        result.x = 0.5 * num12;
        result.y = (num8 + num4) * num13;
        result.z = (num9 + num) * num13;
        result.w = (num6 - num2) * num13;
        return result;
    }
    if (num5 > num3) {
        const double num14 = std::sqrt(1.0 + num5 - num7 - num3);
        const double num15 = 0.5 / num14;
        result.x = (num4 + num8) * num15;
        result.y = 0.5 * num14;
        result.z = (num2 + num6) * num15;
        result.w = (num - num9) * num15;
        return result;
    }
    const double num16 = std::sqrt(1.0 + num3 - num7 - num5);
    const double num17 = 0.5 / num16;
    result.x = (num + num9) * num17;
    result.y = (num2 + num6) * num17;
    result.z = 0.5 * num16;
    result.w = (num8 - num4) * num17;
    return result;
}

// QuaternionD.cs:203 Angle
double QuaternionD::Angle(const QuaternionD& a, const QuaternionD& b) {
    return dotnet::Math::Acos(std::min(std::fabs(Dot(a, b)), 1.0)) * 2.0 * (180.0 / kPI);
}

// QuaternionD.cs:273 operator*(QuaternionD, QuaternionD)
QuaternionD operator*(const QuaternionD& lhs, const QuaternionD& rhs) {
    return {lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x,
            lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z};
}

// QuaternionD.cs:275 operator*(QuaternionD, Vector3d)
Vector3d operator*(const QuaternionD& rotation, const Vector3d& point) {
    const double num = rotation.x * 2.0;
    const double num2 = rotation.y * 2.0;
    const double num3 = rotation.z * 2.0;
    const double num4 = rotation.x * num;
    const double num5 = rotation.y * num2;
    const double num6 = rotation.z * num3;
    const double num7 = rotation.x * num2;
    const double num8 = rotation.x * num3;
    const double num9 = rotation.y * num3;
    const double num10 = rotation.w * num;
    const double num11 = rotation.w * num2;
    const double num12 = rotation.w * num3;
    Vector3d result;
    result.x = (1.0 - (num5 + num6)) * point.x + (num7 - num12) * point.y + (num8 + num11) * point.z;
    result.y = (num7 + num12) * point.x + (1.0 - (num4 + num6)) * point.y + (num9 - num10) * point.z;
    result.z = (num8 - num11) * point.x + (num9 + num10) * point.y + (1.0 - (num4 + num5)) * point.z;
    return result;
}

}  // namespace ksp
