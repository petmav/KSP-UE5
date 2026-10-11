// WP-01 helpers without golden data (verified by inspection against the C#): DoubleCurve, Vector3Curve, Matrix4x4D,
// Mathfx, VectorExtensions, QuaternionExtensions, PIDclamp, KSPRandom, UnityRandom.
#include <cmath>

#include "doctest/doctest.h"
#include "ksp/config/config_node.hpp"
#include "ksp/curves/double_curve.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/math/matrix4x4d.hpp"
#include "ksp/unity/random.hpp"

using namespace ksp;

TEST_CASE("DoubleCurve: Hermite evaluation, clamping and KSP's tangent quirks") {
    DoubleCurve c;
    c.Add(0.0, 0.0, 0.0, 1.0);
    c.Add(2.0, 2.0, 1.0, 0.0);
    CHECK(c.Evaluate(-1.0) == 0.0);
    CHECK(c.Evaluate(3.0) == 2.0);
    CHECK(c.Evaluate(2.0) == 2.0);
    // the first Add zeroes the single key's tangents even when explicit (RecomputeTangents, count == 1): out0 = 0
    CHECK(c.keys[0].outTangent == 0.0);
    CHECK(c.Evaluate(1.0) == doctest::Approx(0.75));
    // auto tangents: first key uses a one-sided slope; last key divides by (time - previous VALUE) (sic)
    DoubleCurve a;
    a.Add(0.0, 10.0);
    a.Add(5.0, 20.0);
    CHECK(a.keys[0].outTangent == 2.0);
    CHECK(a.keys[1].inTangent == (20.0 - 10.0) / (5.0 - 10.0));
    // middle auto keys are never updated (struct copy in KSP)
    a.Add(2.0, 12.0);
    CHECK(a.keys[1].time == 2.0);
    CHECK(a.keys[1].inTangent == 0.0);
    CHECK(a.keys[1].outTangent == 0.0);

    ConfigNode n("C");
    n.AddValue("key", "0 1");
    n.AddValue("key", "1 2 0.5 0.5");
    DoubleCurve l;
    l.Load(n);
    REQUIRE(l.keys.size() == 2);
    ConfigNode out("C");
    l.Save(out);
    CHECK(out.GetValues("key")[1] == "1 2 0.5 0.5");
}

TEST_CASE("Vector3Curve uses Unity smooth tangents per component") {
    Vector3Curve v;
    v.Add(0.0f, Vector3f(0, 0, 0));
    v.Add(1.0f, Vector3f(1, 2, 3));
    const Vector3f m = v.EvaluateVector(1.0f);
    CHECK(m.Equals(Vector3f(1, 2, 3)));
    CHECK(v.minTime() == 0.0f);
    CHECK(v.maxTime() == 1.0f);
}

TEST_CASE("Matrix4x4D: inverse, transforms") {
    Matrix4x4D t;
    t.Translate(1, 2, 3);
    CHECK(t.TransformPoint(Vector3d(1, 1, 1)).Equals(Vector3d(2, 3, 4)));
    CHECK(t.TransformVector(Vector3d(1, 1, 1)).Equals(Vector3d(1, 1, 1)));
    Matrix4x4D r;
    r.RotateZaxis(0.5);
    Matrix4x4D m = r * t;
    Matrix4x4D inv = m;
    inv.Inverse();
    const Matrix4x4D id = m * inv;
    CHECK(id.m00 == doctest::Approx(1.0));
    CHECK(id.m11 == doctest::Approx(1.0));
    CHECK(id.m33 == doctest::Approx(1.0));
    CHECK(std::fabs(id.m01) < 1e-12);
    CHECK(Matrix4x4D::Identity().Det() == 1.0);
}

TEST_CASE("Mathfx / extensions / PIDclamp") {
    CHECK(Mathfx::Hermite(0.0f, 10.0f, 0.5f) == 5.0f);
    CHECK(Mathfx::Clerp(350.0f, 10.0f, 0.5f) == 360.0f);
    CHECK(Mathfx::SmoothStep(0.5f, 0.0f, 1.0f) == 0.5f);
    CHECK(VectorExtensions::IsInvalid(Vector3f(1, std::nanf(""), 0)));
    CHECK_FALSE(VectorExtensions::IsInvalid(Vector3f(3e38f, 3e38f, -3e38f)));  // double sum under Mono: no overflow
    CHECK(VectorExtensions::IsSmallerThan(Vector3f(0.05f, 0, 0)));
    CHECK(QuaternionExtensions::Inverse(Quaternionf(0.1f, -0.2f, 0.3f, 0.9f)).Equals(Quaternionf(-0.1f, 0.2f, -0.3f, 0.9f)));
    CHECK(QuaternionExtensions::Pitch(Quaternionf(0, 0, 0, 1)) == 0.0f);

    PIDclamp pid("p", 1.0f, 0.5f, 0.0f, 10.0f);
    CHECK(pid.Update(2.0, 0.5) == doctest::Approx(2.0 + 0.5 * 1.0));
    pid.ClampIntegral(100.0f, true);
    pid.Reset();
    pid.Update(1.0, 1.0);  // integral += 1 twice (KSP quirk)
    CHECK(pid.Output() == doctest::Approx(1.0 + 0.5 * 2.0));
    CHECK(PIDclamp::Parse("x#1#2#3#4").kd == 3.0);
    CHECK(PIDclamp("n", 1.5f, 0, 0, 2).ToString() == "n#1.5#0#0#2");
}

TEST_CASE("KSPRandom and UnityRandom basics") {
    KSPRandom r(42);
    const double d = r.NextDouble();
    CHECK(d >= 0.0);
    CHECK(d < 1.0);
    CHECK_THROWS(KSPRandom(INT32_MIN));
    UnityRandom u(0);
    const UnityRandom::State s = u.state();
    CHECK(s.s0 == 0u);
    CHECK(s.s1 == 1u);
    CHECK(s.s2 == 1812433254u);
    CHECK(u.Range(5, 5) == 5);
    CHECK(u.state().s3 == s.s3);  // Range(min, min) consumes no draw
    const float v = u.value();
    CHECK(v >= 0.0f);
    CHECK(v <= 1.0f);
}

// wpfix7: seeding wraps in unchecked int arithmetic (Random.cs ctor, KSPRandom.cs ctor); the expected values come from an independent Python
// implementation of the same algorithm with explicit 32-bit wrap (INT32_MIN / INT32_MAX / negative seeds overflow the subtraction in the seeding loops)
TEST_CASE("SystemRandom / KSPRandom seeding wrap-around (Random.cs, KSPRandom.cs) matches an independent reference") {
    struct Row { int32_t seed; std::vector<int32_t> sys; std::vector<int32_t> ksp; };
    const std::vector<Row> rows = {
        {INT32_MIN, {1559595546, 1755192844, 1649316172, 1198642031, 442452829, 1200195955, 1945678308, 949569752}, {}},
        {2147483647, {1559595546, 1755192844, 1649316172, 1198642031, 442452829, 1200195955, 1945678308, 949569752}, {1976681210, 551155468, 2145952471, 324791282, 261666074, 1334032169, 1682463122, 1484985026}},
        {-2147483647, {1559595546, 1755192844, 1649316172, 1198642031, 442452829, 1200195955, 1945678308, 949569752}, {1976681210, 551155468, 2145952471, 324791282, 261666074, 1334032169, 1682463122, 1484985026}},
        {-1, {534011718, 237820880, 1002897798, 1657007234, 1412011072, 929393559, 760389092, 2026928803}, {787814235, 446536433, 2047446447, 542114668, 1948695961, 780511039, 1618872875, 1295684380}},
        {0, {1559595546, 1755192844, 1649316166, 1198642031, 442452829, 1200195957, 1945678308, 949569752}, {1976681210, 551155468, 2145952487, 324791282, 261666074, 1334032169, 1682463122, 1484985026}},
        {1, {534011718, 237820880, 1002897798, 1657007234, 1412011072, 929393559, 760389092, 2026928803}, {787814235, 446536433, 2047446447, 542114668, 1948695961, 780511039, 1618872875, 1295684380}},
        {42, {1434747710, 302596119, 269548474, 1122627734, 361709742, 563913476, 1555655117, 1101493307}, {1436392141, 452123292, 156182454, 862438906, 249960977, 1708464826, 1159156395, 2124292482}},
        {-123456789, {1091672793, 381850644, 1335622286, 865414785, 1968738143, 1473299219, 172313993, 1943666776}, {296843298, 1294992356, 1298113573, 1229663878, 268041490, 400193693, 138777313, 1924177290}},
        {1999999999, {1250015411, 1511885116, 51968186, 1107261761, 612990673, 107674095, 1363332312, 1224778866}, {1988932029, 493944146, 460553517, 1112323671, 1894071188, 1596095727, 491811073, 1478458027}},
    };
    for (const Row& row : rows) {
        dotnet::SystemRandom s(row.seed);
        for (int32_t e : row.sys) CHECK(s.Next() == e);
        if (row.ksp.empty()) continue;
        KSPRandom k(row.seed);
        for (int32_t e : row.ksp) CHECK(k.Next() == e);
    }
}
