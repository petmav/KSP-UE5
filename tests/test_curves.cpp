#include <cmath>
#include <limits>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/config/config_node.hpp"
#include "ksp/curves/float_curve.hpp"

using namespace ksp;

TEST_CASE("FloatCurve clamps, steps on +Inf tangents and returns NaN at +Inf") {
    FloatCurve c;
    c.Add(0, 0, 0, std::numeric_limits<float>::infinity());
    c.Add(1, 1, std::numeric_limits<float>::infinity(), 0);
    c.Add(2, 3, 0, 0);
    CHECK(c.Evaluate(-5) == 0.0f);
    CHECK(c.Evaluate(0.5f) == 0.0f);  // stepped segment
    CHECK(c.Evaluate(2.0f) == 3.0f);
    CHECK(c.Evaluate(100.0f) == 3.0f);
    CHECK(std::isnan(c.Evaluate(std::numeric_limits<float>::infinity())));
    CHECK(c.minTime() == 0.0f);
    CHECK(c.maxTime() == 2.0f);
}

TEST_CASE("AddKey rejects duplicate times and smooths 2-value keys") {
    ConfigNode n("CURVE");
    n.AddValue("key", "0 0");
    n.AddValue("key", "1 1");
    n.AddValue("key", "1 5");
    n.AddValue("key", "2 4");
    FloatCurve c;
    c.Load(n);
    REQUIRE(c.Curve().length() == 3);
    CHECK(c.Curve()[1].value == 1.0f);
    // middle key: 0.5 * right slope + 0.5 * left slope = 0.5*3 + 0.5*1
    CHECK(c.Curve()[1].inTangent == 2.0f);
    CHECK(c.Curve()[0].outTangent == 1.0f);
    CHECK(c.Curve()[2].inTangent == 3.0f);
}

TEST_CASE("FloatCurve.Load error behaviour") {
    ConfigNode n("CURVE");
    n.AddValue("key", "5");
    FloatCurve c;
    CHECK_THROWS_AS(c.Load(n), std::out_of_range);
    ConfigNode m("CURVE");
    m.AddValue("key", "0 x");
    CHECK_THROWS(c.Load(m));
}

TEST_CASE("Evaluation is independent of evaluation order") {
    FloatCurve c;
    c.Add(0, 1);
    c.Add(3, 2);
    c.Add(4, -1);
    c.Add(10, 7);
    std::vector<float> fwd, rev;
    for (int i = -10; i <= 120; ++i) fwd.push_back(c.Evaluate(static_cast<float>(i) * 0.1f));
    for (int i = 120; i >= -10; --i) rev.push_back(c.Evaluate(static_cast<float>(i) * 0.1f));
    for (size_t i = 0; i < fwd.size(); ++i) CHECK(fwd[i] == rev[fwd.size() - 1 - i]);
}

TEST_CASE("Curves with duplicate key times use the emulated Unity cache") {
    AnimationCurve a({Keyframe(0, 0, 0, 0), Keyframe(1, 1, 0, 0), Keyframe(1, 2, 0, 0), Keyframe(2, 3, 0, 0)});
    CHECK(a.length() == 4);
    CHECK(a.Evaluate(1.5f) == doctest::Approx(2.5f));
    CHECK(a.Evaluate(3.0f) == 3.0f);
}
