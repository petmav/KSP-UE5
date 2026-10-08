// Small KSP float helpers: Mathfx (Mathfx.cs), MinMaxFloat (MinMaxFloat.cs), Vect (Vect.cs), VectorExtensions
// (VectorExtensions.cs), QuaternionExtensions (Assembly-CSharp-firstpass/QuaternionExtensions.cs) and the PID controller
// PIDclamp / IPid (PIDclamp.cs, IPid.cs). Managed float code: Mono's float32 rule (ksp/dotnet/mono_float.hpp).
// Not ported: QuaternionExtensions.QuaternionFromMatrix (needs native Quaternion.LookRotation, CAPTURE-SPEC C3 raw only)
// and RotateInSpecifiedCoordinateSystem (needs a Transform; WP-00).
#pragma once

#include <functional>
#include <string>
#include <string_view>

#include "ksp/math/vector3d.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

namespace Mathfx {
float Hermite(float start, float end, float value);
inline float HermiteIntegral() { return 0.5f; }
float SinerpIntegral();
float CubicHermiteSpline(float start, float end, float v, float easeIn, float easeOut);
float Easerp(float start, float end, float v);
float Easerp(float start, float end, float v, float power);
float Ease(float v, float power);
float Sinerp(float start, float end, float value);
float Coserp(float start, float end, float value);
float XLerp(float start, float end, float value, float power);
float Berp(float start, float end, float value);
float SmoothStep(float x, float min, float max);
float Lerp(float start, float end, float value);
double Lerp(double start, double end, double value);
Vector3f NearestPoint(const Vector3f& lineStart, const Vector3f& lineEnd, const Vector3f& point);
Vector3f NearestPointStrict(const Vector3f& lineStart, const Vector3f& lineEnd, const Vector3f& point);
float Bounce(float x);
bool Approx(float val, float about, float range);
bool Approx(const Vector3f& val, const Vector3f& about, float range);
float Clerp(float start, float end, float value);
Vector3f GetWeightedAvgVector3(int count, const std::function<Vector3f(int)>& getVector,
                               const std::function<float(int)>& getWeight);
}  // namespace Mathfx

// MinMaxFloat.cs
struct MinMaxFloat {
    float min = 0.0f;
    float max = 0.0f;
    float GetLerp(float t) const { return Mathf::Lerp(min, max, t); }
    float GetInverseLerp(float v) const { return Mathf::InverseLerp(min, max, v); }
};

// Vect.cs
namespace Vect {
inline Vector3f r3_half() { return Vector3f::one() * 0.5f; }
inline Vector2f r2_half() { return Vector2f(1.0f, 1.0f) * 0.5f; }
inline Vector4f r4_half() { return Vector4f(1.0f, 1.0f, 1.0f, 1.0f) * 0.5f; }
}  // namespace Vect

// VectorExtensions.cs (Zero(this Vector3 v) modifies a copy in KSP and is therefore a no-op: not provided)
namespace VectorExtensions {
inline bool IsZero(const Vector3f& v) { return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f; }
inline bool IsZero(const Vector2f& v) { return v.x == 0.0f && v.y == 0.0f; }
bool IsInvalid(const Vector3f& v);
bool IsInvalid(const Vector2f& v);
bool IsInvalid(const Vector3d& v);
bool IsSmallerThan(const Vector3f& v, float length = 0.1f);
bool IsSmallerThan(const Vector2f& v, float length = 0.1f);
float HeadingRadians(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis);
float HeadingDegrees(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis);
float BearingRadians(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis);
float BearingDegrees(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis);
}  // namespace VectorExtensions

// QuaternionExtensions.cs
namespace QuaternionExtensions {
// Quaternion.Inverse (native): a plain conjugate, also for non-unit quaternions (CAPTURE-SPEC C3)
inline Quaternionf Inverse(const Quaternionf& q) { return {-q.x, -q.y, -q.z, q.w}; }
float Pitch(const Quaternionf& q);
float Yaw(const Quaternionf& q);
float Roll(const Quaternionf& q);
}  // namespace QuaternionExtensions

// IPid.cs
class IPid {
public:
    virtual ~IPid() = default;
    virtual float Update(float v, float dt) = 0;
};

// PIDclamp.cs
class PIDclamp : public IPid {
public:
    std::string name;
    double kp = 0.0, ki = 0.0, kd = 0.0, clamp = 0.0;
    float tuningScalar = 1.0f, kpScalar = 1.0f, kiScalar = 1.0f, kdScalar = 1.0f, clampScalar = 1.0f;
    bool ignoreIntegral = false;

    PIDclamp(std::string name, float kp, float ki, float kd, float clamp);
    void ClampIntegral(float limit, bool isEnabled);
    void Reinitialize(std::string name, float kp, float ki, float kd);
    void Reinitialize(float kp, float ki, float kd);
    void Reset();
    void ResetIntegral() { integral_ = 0.0; }
    double Update(double error, double dt);
    float Update(float error, float dt) override;
    double Output() const { return output_; }  // implicit operator double
    std::string ToString() const;               // name#kp#ki#kd#clamp (double.ToString())
    static PIDclamp Parse(std::string_view s);  // float.Parse of the '#'-separated fields

private:
    double integral_ = 0.0;
    double integralLast_ = 0.0;
    double derivative_ = 0.0;
    double errorLast_ = 0.0;
    double output_ = 0.0;
    bool clampIntegral_ = false;
    double integralMax_ = 1.0;
};

}  // namespace ksp
