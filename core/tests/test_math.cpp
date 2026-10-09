#include <cmath>

#include "doctest/doctest.h"
#include "ksp/math/util_math.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion.hpp"

using namespace ksp;

TEST_CASE("Mathf rounding and sign follow Unity/.NET") {
    CHECK(Mathf::Round(2.5f) == 2.0f);
    CHECK(Mathf::Round(3.5f) == 4.0f);
    CHECK(Mathf::Round(-2.5f) == -2.0f);
    CHECK(Mathf::RoundToInt(-2.5f) == -2);
    CHECK(Mathf::Sign(0.0f) == 1.0f);
    CHECK(Mathf::Sign(-0.0f) == 1.0f);
    CHECK(Mathf::Repeat(-10.0f, 360.0f) == 350.0f);
    CHECK(Mathf::DeltaAngle(350.0f, 10.0f) == 20.0f);
    CHECK(Mathf::Approximately(1.0f, 1.0f + 1e-7f));
}

TEST_CASE("Vector3 / Quaternion managed ops") {
    const Vector3f v(3, 4, 12);
    CHECK(v.magnitude() == 13.0f);
    CHECK(Vector3f::Angle(Vector3f(1, 0, 0), Vector3f(0, 1, 0)) == doctest::Approx(90.0f));
    CHECK(Vector3f(1, 0, 0) == Vector3f(1.000001f, 0, 0));  // Unity's approximate ==
    CHECK_FALSE(Vector3f(1, 0, 0).Equals(Vector3f(1.000001f, 0, 0)));
    const Quaternionf q(0.70710677f, 0, 0, 0.70710677f);
    const Vector3f r = q * Vector3f(0, 1, 0);
    CHECK(r.z == doctest::Approx(1.0f));
}

TEST_CASE("QuaternionD and UtilMath") {
    const QuaternionD q = QuaternionD::AngleAxis(90.0, Vector3d(0, 0, 1));
    const QuaternionD iq = QuaternionD::Inverse(q);
    CHECK(iq.x == -q.x);
    CHECK(iq.w == q.w);
    CHECK(UtilMath::ClampDegrees180(270.0) == -90.0);
    CHECK(UtilMath::ClampRadiansTwoPI(-1.0) == doctest::Approx(2 * UtilMath::PI - 1.0));
    CHECK(UtilMath::WrapAround(-0.5, -1.0, 3.0) == -0.5);
}

#include <cstring>
#include <limits>

#include "ksp/dotnet/math.hpp"

static bool SameBits(double a, double b) { return std::memcmp(&a, &b, sizeof a) == 0; }

// .NET Framework reference values printed by PowerShell 5.1 (.NET Framework 4.8 x64, "R" format) on this machine;
// Mono values from harness/captures/out/libm_mono.json (CAPTURE-SPEC C7).
TEST_CASE("System.Math runtime semantics (x87 Sin/Cos on both; Tan, Ceiling, Atan2, Pow differ)") {
    using namespace ksp::dotnet;
    REQUIRE(GetRuntime() == Runtime::Mono);
    const double pi = 3.141592653589793;
    // both runtimes
    CHECK(SameBits(Math::Cos(pi / 2), 6.1230317691118863E-17));
    CHECK(SameBits(Math::Sin(pi), 1.2246063538223773E-16));
    CHECK(SameBits(Math::Sin(1e20), 1e20));
    CHECK(SameBits(Math::Cos(1e20), 1e20));
    CHECK(SameBits(Math::Sin(1e10), -0.48750602507627));
    CHECK(SameBits(Math::Sin(-0.0), -0.0));
    CHECK(SameBits(Math::Round(-0.4), -0.0));
    // Mono
    CHECK(SameBits(Math::Tan(pi), -1.2246467991473532e-16));
    CHECK(SameBits(Math::Tan(pi / 2), 1.633123935319537e+16));
    CHECK(SameBits(Math::Ceiling(-0.5), -0.0));
    CHECK(SameBits(Math::Atan2(-0.0, 0.7), 0.0));
    CHECK(SameBits(Math::Pow(-0.0, -3.0), std::numeric_limits<double>::infinity()));
    CHECK(ConvToUInt64(std::nan("")) == 0x8000000000000000ull);
    CHECK(ConvUInt64ToDouble(9223372036854776833ull) == 9223372036854777856.0);
    // .NET Framework
    SetRuntime(Runtime::NetFramework);
    CHECK(SameBits(Math::Tan(pi / 2), 16331778728383844.0));
    CHECK(std::isnan(Math::Tan(1e20)));
    CHECK(SameBits(Math::Tan(-0.0), -0.0));
    CHECK(SameBits(Math::Ceiling(-0.5), 0.0));
    CHECK(SameBits(Math::Ceiling(-0.0), -0.0));
    CHECK(SameBits(Math::Atan2(-0.0, 0.7), -0.0));
    CHECK(SameBits(Math::Pow(-0.0, -3.0), -std::numeric_limits<double>::infinity()));
    CHECK(ConvToUInt64(std::nan("")) == 0ull);
    CHECK(ConvUInt64ToDouble(9223372036854776833ull) == 9223372036854775808.0);
    SetRuntime(Runtime::Mono);
    CHECK(ConvertToInt32(2.5) == 2);
    CHECK(ConvertToInt32(3.5) == 4);
    CHECK_THROWS(ConvertToInt32(2147483647.5));
    CHECK(Math::Round(2.5, MidpointRounding::AwayFromZero) == 3.0);
    CHECK(Math::Round(-2.5, MidpointRounding::AwayFromZero) == -3.0);
}
