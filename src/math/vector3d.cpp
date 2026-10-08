#include "ksp/math/vector3d.hpp"

namespace ksp {

// Vector3d.cs: Normalize(Vector3d)
Vector3d Vector3d::Normalize(const Vector3d& value) {
    const double num = Magnitude(value);
    if (num > 0.0) return value / num;
    return zero();
}

// Vector3d.cs: Lerp (t clamped with Math.Max/Math.Min)
Vector3d Vector3d::Lerp(const Vector3d& from, const Vector3d& to, double t) {
    t = std::max(0.0, std::min(t, 1.0));
    return {from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t, from.z + (to.z - from.z) * t};
}

// Vector3d.cs: Reflect = -2.0 * Dot(inNormal, inDirection) * inNormal + inDirection
Vector3d Vector3d::Reflect(const Vector3d& inDirection, const Vector3d& inNormal) {
    return -2.0 * Dot(inNormal, inDirection) * inNormal + inDirection;
}

// Vector3d.cs: Project = onNormal * Dot(vector, onNormal) / num
Vector3d Vector3d::Project(const Vector3d& vector, const Vector3d& onNormal) {
    const double num = Dot(onNormal, onNormal);
    if (num < 1.401298464324817E-45) return zero();
    return onNormal * Dot(vector, onNormal) / num;
}

// Vector3d.cs: Exclude = fromThat - Project(fromThat, excludeThis)
Vector3d Vector3d::Exclude(const Vector3d& excludeThis, const Vector3d& fromThat) {
    return fromThat - Project(fromThat, excludeThis);
}

// Vector3d.cs: Angle (degrees)
double Vector3d::Angle(const Vector3d& from, const Vector3d& to) {
    return std::acos(std::min(std::max(Dot(from.normalized(), to.normalized()), -1.0), 1.0)) * (180.0 / 3.141592653589793);
}

// Vector3d.cs: AngleBetween (radians, obsolete)
double Vector3d::AngleBetween(const Vector3d& from, const Vector3d& to) {
    return std::acos(std::min(std::max(Dot(from.normalized(), to.normalized()), -1.0), 1.0));
}

// Vector3d.cs: Distance
double Vector3d::Distance(const Vector3d& a, const Vector3d& b) {
    const Vector3d v(a.x - b.x, a.y - b.y, a.z - b.z);
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

// Vector3d.cs: Basis = x * X + y * Y + z * Z
Vector3d Vector3d::Basis(const Vector3d& X, const Vector3d& Y, const Vector3d& Z) const { return x * X + y * Y + z * Z; }

// Vector2d.cs: Normalize()
void Vector2d::Normalize() {
    const double num = magnitude();
    if (num > 0.0) *this = *this / num;
    else *this = zero();
}

// Vector2d.cs: Lerp
Vector2d Vector2d::Lerp(const Vector2d& from, const Vector2d& to, double t) {
    t = std::min(1.0, std::max(0.0, t));
    return {from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t};
}

// Vector2d.cs: MoveTowards
Vector2d Vector2d::MoveTowards(const Vector2d& current, const Vector2d& target, double maxDistanceDelta) {
    const Vector2d v = target - current;
    const double num = v.magnitude();
    if (!(num <= maxDistanceDelta) && num != 0.0) return current + v / num * maxDistanceDelta;
    return target;
}

// Vector2d.cs: Angle
double Vector2d::Angle(const Vector2d& from, const Vector2d& to) {
    return std::acos(std::min(std::max(Dot(from.normalized(), to.normalized()), -1.0), 1.0)) * (180.0 / 3.141592653589793);
}

// Vector2d.cs: ClampMagnitude
Vector2d Vector2d::ClampMagnitude(const Vector2d& v, double maxLength) {
    if (v.sqrMagnitude() > maxLength * maxLength) return v.normalized() * maxLength;
    return v;
}

// Vector4d.cs: Normalize(Vector4d)
Vector4d Vector4d::Normalize(const Vector4d& a) {
    const double num = Magnitude(a);
    if (num > 0.0) return a / num;
    return zero();
}

}  // namespace ksp
