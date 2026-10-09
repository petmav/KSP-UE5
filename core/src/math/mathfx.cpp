// Port of Assembly-CSharp/Mathfx.cs, VectorExtensions.cs, PIDclamp.cs and Assembly-CSharp-firstpass/
// QuaternionExtensions.cs. Managed float code follows Mono's float rule (ksp/dotnet/mono_float.hpp).
#include "ksp/math/mathfx.hpp"

#include <cmath>

#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace {
constexpr float kPIf = 3.14159274f;  // (float)Math.PI
}

namespace Mathfx {

// Mathfx.cs: Hermite -- Mathf.Lerp(start, end, value * value * (3f - 2f * value))
float Hermite(float start, float end, float value) {
    return Mathf::Lerp(start, end, f32store(fx(value) * fx(value) * (3.0 - 2.0 * fx(value))));
}

// Mathfx.cs: SinerpIntegral -- 2f / (float)Math.PI (compile-time constant)
float SinerpIntegral() { return f32store(2.0 / fx(kPIf)); }

// Mathfx.cs: CubicHermiteSpline
float CubicHermiteSpline(float start, float end, float v, float easeIn, float easeOut) {
    const float num = Mathf::Pow(v, 2.0f);
    const float num2 = Mathf::Pow(v, 3.0f);
    return Mathf::Lerp(start, end,
                       f32store((fx(num2) - 2.0 * fx(num) + fx(v)) * fx(easeIn) + (-2.0 * fx(num2) + 3.0 * fx(num)) +
                                (fx(num2) - fx(num)) * fx(easeOut)));
}

// Mathfx.cs: Easerp(start, end, v)
float Easerp(float start, float end, float v) {
    return Mathf::Lerp(start, end, Mathf::Pow(Mathf::Sin(f32store(fx(v) * fx(1.5707f))), 2.0f));
}

// Mathfx.cs: Ease
float Ease(float v, float power) {
    return f32store((fx(Mathf::Pow(Mathf::Sin(f32store(fx(v) * fx(1.5707f))), power)) +
                     (1.0 - fx(Mathf::Pow(Mathf::Cos(f32store(fx(v) * fx(1.5707f))), power)))) /
                    2.0);
}

// Mathfx.cs: Easerp(start, end, v, power)
float Easerp(float start, float end, float v, float power) { return Mathf::Lerp(start, end, Ease(v, power)); }

// Mathfx.cs: Sinerp -- Mathf.Sin(value * (float)Math.PI * 0.5f)
float Sinerp(float start, float end, float value) {
    return Mathf::Lerp(start, end, Mathf::Sin(f32store(fx(value) * fx(kPIf) * 0.5)));
}

// Mathfx.cs: Coserp
float Coserp(float start, float end, float value) {
    return Mathf::Lerp(start, end, f32store(1.0 - fx(Mathf::Cos(f32store(fx(value) * fx(kPIf) * 0.5)))));
}

// Mathfx.cs: XLerp
float XLerp(float start, float end, float value, float power) {
    return Mathf::Lerp(start, end, Mathf::Pow(Mathf::Clamp01(value), power));
}

// Mathfx.cs: Berp
float Berp(float start, float end, float value) {
    value = Mathf::Clamp01(value);
    const float s = Mathf::Sin(f32store(fx(value) * fx(kPIf) * (fx(0.2f) + 2.5 * fx(value) * fx(value) * fx(value))));
    const float p = Mathf::Pow(f32store(1.0 - fx(value)), 2.2f);
    value = f32store((fx(s) * fx(p) + fx(value)) * (1.0 + fx(1.2f) * (1.0 - fx(value))));
    return f32store(fx(start) + (fx(end) - fx(start)) * fx(value));
}

// Mathfx.cs: SmoothStep(x, min, max)
float SmoothStep(float x, float min, float max) {
    x = Mathf::Clamp(x, min, max);
    const float num = f32store((fx(x) - fx(min)) / (fx(max) - fx(min)));
    const float num2 = f32store((fx(x) - fx(min)) / (fx(max) - fx(min)));
    return f32store(-2.0 * fx(num) * fx(num) * fx(num) + 3.0 * fx(num2) * fx(num2));
}

// Mathfx.cs: Lerp
float Lerp(float start, float end, float value) { return f32store((1.0 - fx(value)) * fx(start) + fx(value) * fx(end)); }
double Lerp(double start, double end, double value) { return (1.0 - value) * start + value * end; }

// Mathfx.cs: NearestPoint
Vector3f NearestPoint(const Vector3f& lineStart, const Vector3f& lineEnd, const Vector3f& point) {
    const Vector3f vector = Vector3f::Normalize(lineEnd - lineStart);
    const float num = f32store(fx(Vector3f::Dot(point - lineStart, vector)) / fx(Vector3f::Dot(vector, vector)));
    return lineStart + num * vector;
}

// Mathfx.cs: NearestPointStrict
Vector3f NearestPointStrict(const Vector3f& lineStart, const Vector3f& lineEnd, const Vector3f& point) {
    const Vector3f vector = lineEnd - lineStart;
    const Vector3f vector2 = Vector3f::Normalize(vector);
    const float value = f32store(fx(Vector3f::Dot(point - lineStart, vector2)) / fx(Vector3f::Dot(vector2, vector2)));
    return lineStart + Mathf::Clamp(value, 0.0f, Vector3f::Magnitude(vector)) * vector2;
}

// Mathfx.cs: Bounce -- Mathf.Abs(Mathf.Sin(6.28f * (x + 1f) * (x + 1f)) * (1f - x))
float Bounce(float x) {
    const float s = Mathf::Sin(f32store(fx(6.28f) * (fx(x) + 1.0) * (fx(x) + 1.0)));
    return Mathf::Abs(f32store(fx(s) * (1.0 - fx(x))));
}

// Mathfx.cs: Approx
bool Approx(float val, float about, float range) { return Mathf::Abs(f32store(fx(val) - fx(about))) < range; }
bool Approx(const Vector3f& val, const Vector3f& about, float range) {
    return fx((val - about).sqrMagnitude()) < fx(range) * fx(range);
}

// Mathfx.cs: Clerp
float Clerp(float start, float end, float value) {
    const float num = 360.0f;
    const float num2 = Mathf::Abs(180.0f);
    float num4;
    if (fx(end) - fx(start) < 0.0 - fx(num2)) {
        num4 = f32store((fx(num) - fx(start) + fx(end)) * fx(value));
        return f32store(fx(start) + fx(num4));
    }
    if (fx(end) - fx(start) > fx(num2)) {
        num4 = f32store((0.0 - (fx(num) - fx(end) + fx(start))) * fx(value));
        return f32store(fx(start) + fx(num4));
    }
    return f32store(fx(start) + (fx(end) - fx(start)) * fx(value));
}

// Mathfx.cs: GetWeightedAvgVector3 (iterates count-1 .. 0)
Vector3f GetWeightedAvgVector3(int count, const std::function<Vector3f(int)>& getVector,
                               const std::function<float(int)>& getWeight) {
    Vector3f zero = Vector3f::zero();
    float num = 0.0f;
    int arg = count;
    while (arg-- > 0) {
        const float num2 = getWeight(arg);
        zero = zero + getVector(arg) * num2;
        num = f32store(fx(num) + fx(num2));
    }
    if (num != 0.0f) return zero / num;
    return zero;
}

}  // namespace Mathfx

namespace VectorExtensions {

// VectorExtensions.cs: IsInvalid -- float f = v.x + v.y + v.z (a float local: one rounding of the double sum)
bool IsInvalid(const Vector3f& v) {
    const float f = f32store(fx(v.x) + fx(v.y) + fx(v.z));
    return std::isnan(f) || std::isinf(f);
}
bool IsInvalid(const Vector2f& v) {
    const float f = f32store(fx(v.x) + fx(v.y));
    return std::isnan(f) || std::isinf(f);
}
bool IsInvalid(const Vector3d& v) {
    const double d = v.x + v.y + v.z;
    return std::isnan(d) || std::isinf(d);
}

// VectorExtensions.cs: IsSmallerThan (comparison of double expressions)
bool IsSmallerThan(const Vector3f& v, float length) {
    return fx(v.x) * fx(v.x) + fx(v.y) * fx(v.y) + fx(v.z) * fx(v.z) < fx(length) * fx(length);
}
bool IsSmallerThan(const Vector2f& v, float length) {
    return fx(v.x) * fx(v.x) + fx(v.y) * fx(v.y) < fx(length) * fx(length);
}

namespace {
float SignedAngleRad(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis) {
    return f32store(fx(Mathf::Acos(Vector3f::Dot(v1, v2))) *
                    fx(Mathf::Sign(Vector3f::Dot(Vector3f::Cross(v1, v2), upAxis))));
}
}  // namespace

// VectorExtensions.cs: HeadingRadians
float HeadingRadians(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis) {
    if (v1 == v2) return 0.0f;
    return UtilMath::WrapAround(SignedAngleRad(v1, v2, upAxis), 0.0f, 6.28318548f);  // (float)Math.PI * 2f
}

// VectorExtensions.cs: HeadingDegrees
float HeadingDegrees(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis) {
    if (v1 == v2) return 0.0f;
    const float a = f32store(fx(Mathf::Acos(Vector3f::Dot(v1, v2))) *
                             fx(Mathf::Sign(Vector3f::Dot(Vector3f::Cross(v1, v2), upAxis))) * fx(57.29578f));
    return UtilMath::WrapAround(a, 0.0f, 360.0f);
}

// VectorExtensions.cs: BearingRadians
float BearingRadians(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis) {
    if (v1 == v2) return 0.0f;
    return SignedAngleRad(v1, v2, upAxis);
}

// VectorExtensions.cs: BearingDegrees
float BearingDegrees(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis) {
    if (v1 == v2) return 0.0f;
    return f32store(fx(Mathf::Acos(Vector3f::Dot(v1, v2))) * fx(Mathf::Sign(Vector3f::Dot(Vector3f::Cross(v1, v2), upAxis))) *
                    fx(57.29578f));
}

}  // namespace VectorExtensions

namespace QuaternionExtensions {

// QuaternionExtensions.cs: Pitch
float Pitch(const Quaternionf& q) {
    return Mathf::Atan2(f32store(2.0 * (fx(q.y) * fx(q.z) + fx(q.w) * fx(q.x))),
                        f32store(fx(q.w) * fx(q.w) - fx(q.x) * fx(q.x) - fx(q.y) * fx(q.y) + fx(q.z) * fx(q.z)));
}

// QuaternionExtensions.cs: Yaw
float Yaw(const Quaternionf& q) { return Mathf::Asin(f32store(-2.0 * (fx(q.x) * fx(q.z) - fx(q.w) * fx(q.y)))); }

// QuaternionExtensions.cs: Roll
float Roll(const Quaternionf& q) {
    return Mathf::Atan2(f32store(2.0 * (fx(q.x) * fx(q.y) + fx(q.w) * fx(q.z))),
                        f32store(fx(q.w) * fx(q.w) + fx(q.x) * fx(q.x) - fx(q.y) * fx(q.y) - fx(q.z) * fx(q.z)));
}

}  // namespace QuaternionExtensions

// ---- PIDclamp.cs ----------------------------------------------------------------------------------------------------

PIDclamp::PIDclamp(std::string n, float p, float i, float d, float c)
    : name(std::move(n)), kp(p), ki(i), kd(d), clamp(c) {}

void PIDclamp::ClampIntegral(float limit, bool isEnabled) {
    clampIntegral_ = isEnabled;
    integralMax_ = limit;
}

void PIDclamp::Reinitialize(std::string n, float p, float i, float d) {
    name = std::move(n);
    Reinitialize(p, i, d);
}

void PIDclamp::Reinitialize(float p, float i, float d) {
    kp = p;
    ki = i;
    kd = d;
    Reset();
}

void PIDclamp::Reset() {
    integral_ = 0.0;
    errorLast_ = 0.0;
}

// PIDclamp.cs: Update(double, double). Quirk kept: with clampIntegral the error term is added twice.
double PIDclamp::Update(double error, double dt) {
    integralLast_ = integral_;
    if (!ignoreIntegral) {
        integral_ += error * dt;
        if (clampIntegral_) {
            integral_ += error * dt;
            integral_ = UtilMath::Clamp(integral_, 0.0 - integralMax_, integralMax_);
        }
    } else {
        integral_ = 0.0;
    }
    derivative_ = (error - errorLast_) / dt;
    errorLast_ = error;
    output_ = kp * static_cast<double>(tuningScalar) * static_cast<double>(kpScalar) * error +
              ki * static_cast<double>(tuningScalar) * static_cast<double>(kiScalar) * integral_ +
              kd * static_cast<double>(tuningScalar) * static_cast<double>(kdScalar) * derivative_;
    const double num = clamp * static_cast<double>(clampScalar);
    if (output_ < 0.0 - num) {
        output_ = 0.0 - num;
        integral_ = integralLast_;
    } else if (output_ > num) {
        output_ = num;
        integral_ = integralLast_;
    }
    return output_;
}

float PIDclamp::Update(float error, float dt) { return f32store(Update(fx(error), fx(dt))); }

std::string PIDclamp::ToString() const {
    using dotnet::FormatDoubleG;
    return name + "#" + FormatDoubleG(kp) + "#" + FormatDoubleG(ki) + "#" + FormatDoubleG(kd) + "#" + FormatDoubleG(clamp);
}

PIDclamp PIDclamp::Parse(std::string_view s) {
    const auto a = dotnet::Split(s, "#", false);
    if (a.size() < 5) throw std::out_of_range("PIDclamp.Parse: Index was outside the bounds of the array.");
    return PIDclamp(a[0], dotnet::ParseSingle(a[1]), dotnet::ParseSingle(a[2]), dotnet::ParseSingle(a[3]),
                    dotnet::ParseSingle(a[4]));
}

}  // namespace ksp
