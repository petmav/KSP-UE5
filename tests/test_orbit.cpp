// WP-07 Orbit: API behaviour the orbit_propagation golden does not cover (frame conventions, degenerate inputs, the
// CelestialBodyRef / PlanetariumFrames contract, static solvers) and the native Quaternion.AngleAxis that Orbit uses,
// checked bit for bit against the in-game capture (CAPTURE-SPEC C3).
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/targeting.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;

namespace {

constexpr double kPI = 3.141592653589793;
constexpr double kInf = std::numeric_limits<double>::infinity();

// Kerbin as the harness builds it: Mass = R*R*(GeeASL*9.80665)/G, mu = Mass*G (CelestialBody.Awake).
CelestialBodyRef MakeBody(const char* name, double radius, double geeASL) {
    CelestialBodyRef b;
    b.bodyName = name;
    b.Radius = radius;
    const double mass = radius * radius * (geeASL * 9.80665) / 6.67408E-11;
    b.gravParameter = mass * 6.67408E-11;
    return b;
}
CelestialBodyRef MakeKerbin() { return MakeBody("Kerbin", 600000.0, 1.0003416049313476); }

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
float FromHex(const std::string& h) {
    const uint32_t u = static_cast<uint32_t>(std::stoul(h, nullptr, 16));
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

bool Near(const Vector3d& a, const Vector3d& b, double tol) {
    return std::fabs(a.x - b.x) <= tol && std::fabs(a.y - b.y) <= tol && std::fabs(a.z - b.z) <= tol;
}

struct FramesGuard {
    FramesGuard() { PlanetariumFrames::Reset(); }
    ~FramesGuard() { PlanetariumFrames::Reset(); }
};

}  // namespace

TEST_CASE("UnityNative::AngleAxis is bit-exact against the in-game capture (C3)") {
    const std::string path = std::string(KSPCORE_HARNESS_DIR) + "/captures/out/math_shim_native.json";
    const auto text = dotnet::ReadAllText(path);
    REQUIRE_MESSAGE(text.has_value(), "missing " << path);
    const auto j = nlohmann::json::parse(*text);
    int rows = 0, bad = 0;
    for (const auto& t : j.at("tables")) {
        if (t.at("name") != "f32") continue;
        std::vector<std::string> cols;
        for (const auto& c : t.at("columns")) cols.push_back(c.at("name").get<std::string>());
        auto idx = [&](const char* n) { return static_cast<size_t>(std::find(cols.begin(), cols.end(), n) - cols.begin()); };
        const size_t fn = idx("fn"), a0 = idx("a0_hex"), a1 = idx("a1_hex"), a2 = idx("a2_hex"), a3 = idx("a3_hex");
        const size_t r0 = idx("r0_hex"), r1 = idx("r1_hex"), r2 = idx("r2_hex"), r3 = idx("r3_hex");
        for (const auto& r : t.at("rows")) {
            if (r.at(fn) != "Quaternion.AngleAxis") continue;
            ++rows;
            const Quaternionf q = UnityNative::AngleAxis(
                FromHex(r.at(a0)), Vector3f(FromHex(r.at(a1)), FromHex(r.at(a2)), FromHex(r.at(a3))));
            const bool ok = Bits(q.x) == Bits(FromHex(r.at(r0))) && Bits(q.y) == Bits(FromHex(r.at(r1))) &&
                            Bits(q.z) == Bits(FromHex(r.at(r2))) && Bits(q.w) == Bits(FromHex(r.at(r3)));
            if (!ok) {
                ++bad;
                if (bad <= 5) {
                    MESSAGE("row " << r.at(1) << " angle " << r.at(2) << " got " << q.x << " " << q.y << " " << q.z << " "
                                   << q.w << " want " << r.at(r0 - 1) << " " << r.at(r1 - 1) << " " << r.at(r2 - 1) << " "
                                   << r.at(r3 - 1));
                }
            }
        }
    }
    CHECK(rows == 84);
    CHECK(bad == 0);
}

TEST_CASE("PlanetariumFrames: Awake(0) frames, up/right/forward, SetInverseRotAngle, ZupAtT") {
    FramesGuard guard;
    const CelestialFrame& zup = PlanetariumFrames::Zup();
    CHECK(Near(zup.X, {1, 0, 0}, 1e-15));
    CHECK(Near(zup.Y, {0, 1, 0}, 1e-15));
    CHECK(Near(zup.Z, {0, 0, 1}, 1e-15));
    CHECK(Near(PlanetariumFrames::up(), {0, 1, 0}, 1e-15));  // Unity world: y up
    CHECK(Near(PlanetariumFrames::right(), {1, 0, 0}, 1e-15));
    CHECK(Near(PlanetariumFrames::forward(), {0, 0, 1}, 1e-15));
    CHECK(PlanetariumFrames::InverseRotAngle() == 0.0);

    // CBUpdate's inverse-rotation update: Zup turns about z by the angle; Reset restores Awake(0) bit for bit.
    const Vector3d x0 = zup.X;
    PlanetariumFrames::SetInverseRotAngle(90.0);
    CHECK(PlanetariumFrames::InverseRotAngle() == 90.0);
    CHECK(Near(PlanetariumFrames::Zup().X, {0, 1, 0}, 1e-15));
    PlanetariumFrames::Reset();
    CHECK(PlanetariumFrames::Zup().X.Equals(x0));

    // ZupAtT: non-inverse-rotation bodies get the current Zup; inverse-rotation bodies their own frame at UT.
    CelestialBodyRef body = MakeKerbin();
    CelestialFrame f;
    PlanetariumFrames::ZupAtT(1234.5, body, f);
    CHECK((f.X.Equals(zup.X) && f.Y.Equals(zup.Y) && f.Z.Equals(zup.Z)));
    body.inverseRotation = true;
    body.initialRotation = 90.0;
    body.rotPeriodRecip = 1.0 / 21600.0;
    body.directRotAngle = 0.0;
    PlanetariumFrames::ZupAtT(5400.0, body, f);  // (90 + 360 * 5400 / 21600) % 360 = 180 degrees
    CelestialFrame expect;
    CelestialFrame::PlanetaryFrame(0.0, 90.0, std::fmod(std::fmod(90.0 + 360.0 * body.rotPeriodRecip * 5400.0, 360.0), 360.0),
                                   expect);
    CHECK((f.X.Equals(expect.X) && f.Y.Equals(expect.Y) && f.Z.Equals(expect.Z)));
    CHECK(Near(f.X, {-1, 0, 0}, 1e-15));
}

TEST_CASE("Orbit: equatorial prograde circular orbit, frame conventions") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    kerbin.position = Vector3d(1.0, 2.0, 3.0);
    const double a = 700000.0;
    Orbit o(0.0, 0.0, a, 0.0, 0.0, 0.0, 0.0, &kerbin);
    // i = 0: Cross(forward, Z) = 0 -> an = right; e = 0 -> eccVec = 0
    CHECK(o.an.Equals(Vector3d::right()));
    CHECK(o.eccVec.sqrMagnitude() == 0.0);
    CHECK(Near(o.GetOrbitNormal(), {0, 0, 1}, 1e-15));  // zup local: z = north
    CHECK(o.h.z > 0.0);
    const double n = std::sqrt(kerbin.gravParameter / (a * a * a));
    CHECK(o.meanMotion == doctest::Approx(n).epsilon(1e-15));
    CHECK(o.period == 2.0 * kPI / o.meanMotion);
    CHECK(o.ObTAtEpoch == 0.0);
    // periapsis direction = OrbitFrame.X; prograde = counter-clockwise seen from +z
    const Vector3d p0 = o.getRelativePositionAtUT(0.0);
    const Vector3d v0 = o.getOrbitalVelocityAtUT(0.0);
    CHECK(Near(p0, {a, 0, 0}, 1e-9));
    CHECK(Near(v0, {0, std::sqrt(kerbin.gravParameter / a), 0}, 1e-9));
    const Vector3d pq = o.getRelativePositionAtUT(o.period / 4.0);
    CHECK(Near(pq, {0, a, 0}, 1e-6));
    // world position = body.position + local.xzy (Unity y up)
    const double t = 777.0;
    CHECK(o.getPositionAtUT(t).Equals(kerbin.position + o.getRelativePositionAtUT(t).xzy()));
    CHECK(o.getPositionAtT(o.getObtAtUT(t)).Equals(o.getPositionAtUT(t)));
    CHECK(o.Up(t).Equals(o.getRelativePositionAtUT(t).xzy().normalized()));
    CHECK(Near(o.Normal(t), {0, -1, 0}, 1e-15));  // -orbit normal in Unity coordinates
    // UpdateFromUT
    o.UpdateFromUT(t);
    CHECK(o.radius == doctest::Approx(a).epsilon(1e-15));
    CHECK(o.altitude == o.radius - kerbin.Radius);
    CHECK(o.orbitalSpeed == doctest::Approx(std::sqrt(kerbin.gravParameter / a)).epsilon(1e-14));
    CHECK(o.orbitalEnergy == doctest::Approx(-kerbin.gravParameter / (2.0 * a)).epsilon(1e-13));
    CHECK(o.pos.Equals(o.getRelativePositionAtUT(t)));
    CHECK(o.GetRelativeVel().Equals(o.vel.xzy()));
    CHECK(o.GetRadiusAtUT(t) == doctest::Approx(a).epsilon(1e-15));
    CHECK(o.PeR() == a);
    CHECK(o.ApR() == a);
    CHECK(o.PeA() == a - kerbin.Radius);
    CHECK(o.semiMinorAxis() == a);
    CHECK(o.semiLatusRectum() == doctest::Approx(a).epsilon(1e-15));
}

TEST_CASE("Orbit: UpdateFromStateVectors inverts the propagation (inclined ellipse, retrograde, hyperbola)") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    struct El {
        double inc, e, sma, lan, argPe, mEp;
    };
    for (const El& el : {El{28.5, 0.3, 2.0e6, 77.7, 40.0, 1.0}, El{135.0, 0.05, 1.0e6, 300.0, 200.0, -2.0},
                         El{60.0, 1.4, -3.0e6, 10.0, 250.0, 0.7}}) {
        Orbit o(el.inc, el.e, el.sma, el.lan, el.argPe, el.mEp, 100.0, &kerbin);
        const double ut = 4321.0;
        Orbit inv;
        inv.UpdateFromStateVectors(o.getRelativePositionAtUT(ut), o.getOrbitalVelocityAtUT(ut), &kerbin, ut);
        CHECK(inv.referenceBody == &kerbin);
        CHECK(inv.epoch == ut);
        CHECK(inv.inclination == doctest::Approx(el.inc).epsilon(1e-10));
        CHECK(inv.eccentricity == doctest::Approx(el.e).epsilon(1e-10));
        CHECK(inv.semiMajorAxis == doctest::Approx(el.sma).epsilon(1e-10));
        CHECK(inv.LAN == doctest::Approx(el.lan).epsilon(1e-10));
        CHECK(inv.argumentOfPeriapsis == doctest::Approx(el.argPe).epsilon(1e-9));
        for (double t : {ut, ut + 1000.0, ut - 5000.0}) {
            const Vector3d a = o.getRelativePositionAtUT(t), b = inv.getRelativePositionAtUT(t);
            CHECK((a - b).magnitude() <= 1e-6 * a.magnitude());
        }
    }
    // pure retrograde equatorial: h along -z -> inclination 180, an = right, LAN 0
    const double vc = std::sqrt(kerbin.gravParameter / 700000.0);
    Orbit r;
    r.UpdateFromStateVectors({700000, 0, 0}, {0, -vc, 0}, &kerbin, 0.0);
    CHECK(r.inclination == 180.0);
    CHECK(r.LAN == 0.0);
    CHECK(r.an.Equals(Vector3d::right()));
    CHECK(r.h.z < 0.0);
}

TEST_CASE("Orbit: degenerate state vectors (radial, zero velocity, overflow -> NaN argPe fallback)") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    // zero velocity: h = 0 -> inclination from pos.z, an = Cross(pos, forward)
    Orbit z;
    z.UpdateFromStateVectors({700000, 0, 0}, {0, 0, 0}, &kerbin, 1000.0);
    CHECK(z.inclination == 90.0);
    CHECK(z.eccentricity == doctest::Approx(1.0).epsilon(1e-15));
    CHECK(z.epoch == 1000.0);
    // radial only: h = 0 as well
    Orbit r;
    r.UpdateFromStateVectors({700000, 0, 0}, {1000, 0, 0}, &kerbin, 0.0);
    CHECK(r.h.sqrMagnitude() == 0.0);
    CHECK(r.inclination == 90.0);
    // overflow, NaN eccentricity vector: Vector3d.Normalize returns zero for a NaN magnitude, so argPe = 0
    Orbit o;
    o.UpdateFromStateVectors({700000, 0, 0}, {0, 1e200, 0}, &kerbin, 0.0);
    CHECK(std::isnan(o.eccentricity));
    CHECK(o.argumentOfPeriapsis == 0.0);
    // overflow, infinite eccentricity vector: OrbitFrame.X = inf/inf = NaN, AngleBetween is NaN and KSP's
    // Quaternion.AngleAxis fallback (Orbit.cs:427) runs; it stores Acos(NaN) = NaN (radians in a degrees field, sic)
    Orbit f;
    f.UpdateFromStateVectors({4e5, 4e5, 4e5}, {1e200, -1e200, 0}, &kerbin, 0.0);
    CHECK(std::isinf(f.eccentricity));
    CHECK(std::isnan(f.argumentOfPeriapsis));
}

TEST_CASE("Orbit: e = 1 boundary and hyperbolic orbits (negative semi-major axis)") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    // e = 1: h = 0, period infinite, eccentric-anomaly positions are the zero vector (neither branch)
    Orbit p(10.0, 1.0, 1.0e6, 30.0, 60.0, 0.0, 0.0, &kerbin);
    CHECK(p.period == kInf);
    CHECK(p.h.sqrMagnitude() == 0.0);
    CHECK(p.getRelativePositionFromEccAnomaly(0.5).Equals(Vector3d::zero()));
    CHECK(p.GetNextApoapsisTime(42.0) == 42.0);
    CHECK(std::isinf(p.getObtAtUT(kInf)));

    Orbit h(10.0, 1.2, -4.0e6, 30.0, 60.0, 0.0, 500.0, &kerbin);
    CHECK(h.period == kInf);
    CHECK(h.orbitPercent == 0.0);
    CHECK(h.meanMotion == std::sqrt(kerbin.gravParameter / (4.0e6 * 4.0e6 * 4.0e6)));
    CHECK(h.PeR() == doctest::Approx(0.8e6).epsilon(1e-15));
    CHECK(h.ApR() < 0.0);
    CHECK(h.semiMinorAxis() < 0.0);
    CHECK(h.GetNextPeriapsisTime(500.0) == 500.0);  // M(epoch) = 0
    CHECK(h.GetNextApoapsisTime(900.0) == 900.0);
    h.UpdateFromUT(2000.0);
    CHECK(h.timeToAp == kInf);
    CHECK(h.timeToPe == 0.0 - h.ObT);
    CHECK(h.ObT == 1500.0);
    CHECK(h.GetTimeToPeriapsis() == h.timeToPe);  // the quirk only applies to e < 1
    CHECK(h.orbitalEnergy > 0.0);
    // asymptote: true anomaly tends to acos(-1/e)
    CHECK(h.GetTrueAnomaly(kInf) == dotnet::Math::Acos(-1.0 / 1.2));
    CHECK(h.GetTrueAnomaly(-kInf) == 0.0 - dotnet::Math::Acos(-1.0 / 1.2));
    CHECK(h.GetEccentricAnomaly(dotnet::Math::Acos(-1.0 / 1.2) + 0.01) == kInf);
    CHECK(h.GetMeanAnomaly(kInf) == kInf);
    // DT to a true anomaly beyond the asymptote is infinite
    CHECK(std::isinf(h.GetDTforTrueAnomalyAtUT(2.6, 2000.0)));
}

TEST_CASE("Orbit: Kepler solvers (field eccentricity, NaN handling, accuracy)") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    const Orbit low(0, 0.3, 1.0e6, 0, 0, 0, 0, &kerbin);
    const Orbit high(0, 0.9, 1.0e6, 0, 0, 0, 0, &kerbin);
    const Orbit hyp(0, 2.0, -1.0e6, 0, 0, 0, 0, &kerbin);
    // KSP quirk: the ecc argument is ignored
    CHECK(low.solveEccentricAnomaly(1.0, 0.9) == low.solveEccentricAnomaly(1.0, 0.3));
    CHECK(low.solveEccentricAnomaly(1.0, 0.9) != high.solveEccentricAnomaly(1.0, 0.9));
    // Kepler's equation holds (M = E - e sin E; M = e sinh H - H)
    for (double M : {0.1, 1.0, 3.0, 5.0}) {
        const double E = low.solveEccentricAnomaly(M, 0.3);
        CHECK(E - 0.3 * std::sin(E) == doctest::Approx(M).epsilon(1e-12));
        const double E2 = high.solveEccentricAnomaly(M, 0.9);
        CHECK(E2 - 0.9 * std::sin(E2) == doctest::Approx(M).epsilon(1e-9));
        const double H = hyp.solveEccentricAnomaly(M, 2.0);
        CHECK(2.0 * std::sinh(H) - H == doctest::Approx(M).epsilon(1e-12));
    }
    const double nan = std::numeric_limits<double>::quiet_NaN();
    CHECK(std::isnan(low.solveEccentricAnomaly(nan, 0.3)));
    CHECK(std::isnan(high.solveEccentricAnomaly(nan, 0.9)));   // Math.Sign(NaN) throws -> caught -> NaN
    CHECK(std::isnan(high.solveEccentricAnomaly(kInf, 0.9)));  // Sin(inf) = NaN
    CHECK(hyp.solveEccentricAnomaly(kInf, 2.0) == kInf);
    CHECK(hyp.solveEccentricAnomaly(-kInf, 2.0) == -kInf);
    CHECK(std::isnan(low.TrueAnomalyAtT(nan)));
    // anomaly round trips (2 * Atan2 keeps the half-angle branch, so angles in (pi, 2pi) come back unchanged)
    for (double tA : {0.3, 2.0, 3.0, 5.5, -1.0}) {
        CHECK(low.GetTrueAnomaly(low.GetEccentricAnomaly(tA)) == doctest::Approx(tA).epsilon(1e-12));
    }
    // getObtAtUT(infinity) on an ellipse logs and returns NaN
    CHECK(std::isnan(low.getObtAtUT(kInf)));
}

TEST_CASE("Orbit: GetTimeToPeriapsis quirk, time-of-anomaly helpers, radius helpers") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    Orbit o(5, 0.5, 6.0e6, 10, 270, 1.0, 0.0, &kerbin);
    o.UpdateFromUT(1000.0);
    REQUIRE(o.timeToPe > 0.0);
    // fresh orbit: patchEndTransition INITIAL, StartUT = EndUT = 0 -> timeToPe - period
    CHECK(o.GetTimeToPeriapsis() == o.timeToPe - o.period);
    o.patchEndTransition = Orbit::PatchTransitionType::FINAL;
    CHECK(o.GetTimeToPeriapsis() == o.timeToPe);
    o.patchEndTransition = Orbit::PatchTransitionType::ESCAPE;
    o.EndUT = 1.0e12;
    CHECK(o.GetTimeToPeriapsis() == o.timeToPe);

    const double ut = 1000.0;
    for (double tA : {0.0, 1.0, kPI, 4.0}) {
        const double t = o.TimeOfTrueAnomaly(tA, ut);
        CHECK(t >= ut);
        CHECK(t < ut + o.period);
        CHECK(std::cos(o.TrueAnomalyAtUT(t)) == doctest::Approx(std::cos(tA)).epsilon(1e-9));
        CHECK(std::sin(o.TrueAnomalyAtUT(t)) == doctest::Approx(std::sin(tA)).epsilon(1e-9).scale(1.0));
    }
    CHECK(o.GetNextPeriapsisTime(ut) == o.TimeOfTrueAnomaly(0.0, ut));
    CHECK(o.GetNextApoapsisTime(ut) == o.TimeOfTrueAnomaly(kPI, ut));
    CHECK(o.GetUTforTrueAnomaly(2.0, 0.0) == o.epoch + (o.ObT - o.ObTAtEpoch) + o.GetDTforTrueAnomaly(2.0, 0.0));
    // radius helpers
    for (double tA : {0.0, 1.0, 2.5}) {
        CHECK(o.RadiusAtTrueAnomaly(tA) ==
              doctest::Approx(o.getRelativePositionFromTrueAnomaly(tA).magnitude()).epsilon(1e-12));
    }
    const double R = 0.5 * (o.PeR() + o.ApR());
    CHECK(o.RadiusAtTrueAnomaly(o.TrueAnomalyAtRadius(R)) == doctest::Approx(R).epsilon(1e-9));
    // R is clamped to [PeR, ApR]; at the clamped ends KSP's acos argument is -1 or 1 up to rounding, so the result is
    // 0 / pi or NaN when the rounding leaves [-1, 1] (here: NaN at ApR)
    const double atPe = o.TrueAnomalyAtRadius(1.0);
    CHECK((std::isnan(atPe) || std::fabs(atPe) < 1e-6));
    const double atAp = o.TrueAnomalyAtRadius(1.0e12);
    CHECK((std::isnan(atAp) || std::fabs(atAp - kPI) < 1e-6));
    CHECK(o.TrueAnomalyAtRadius(0.999 * o.ApR()) == doctest::Approx(kPI).epsilon(0.05));
    const double tR = o.GetNextTimeOfRadius(ut, R);
    CHECK(tR >= ut);
    CHECK(o.GetRadiusAtUT(tR) == doctest::Approx(R).epsilon(1e-6));
    CHECK(o.GetNextTimeOfRadius(ut, 1.0) == ut);  // below periapsis: UT
    CHECK(o.getOrbitalSpeedAtDistance(o.PeR()) > o.getOrbitalSpeedAtDistance(o.ApR()));
    CHECK(o.GetOrbitalCurvatureAtTrueAnomaly(0.0) > 0.0);
}

TEST_CASE("Orbit: state vectors with acceleration and jerk (world-like frame)") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    const Orbit o(20, 0.2, 2.0e6, 50, 80, 0.5, 0.0, &kerbin);
    Orbit::State s;
    const double r = o.GetOrbitalStateVectorsAtUT(300.0, s);
    CHECK(r == doctest::Approx(s.pos.magnitude()).epsilon(1e-14));
    // acc = -mu r / |r|^3
    const Vector3d expectAcc = s.pos * (-kerbin.gravParameter / (r * r * r));
    CHECK((s.acc - expectAcc).magnitude() <= 1e-12 * expectAcc.magnitude());
    // world-like = Zup.LocalToWorld(zup local) ~ zup local (Zup is the identity up to 6e-17)
    CHECK(Near(s.pos, o.getRelativePositionAtUT(300.0), 1e-6));
    Vector3d p, v;
    CHECK(o.GetOrbitalStateVectorsAtUT(300.0, p, v) == r);
    CHECK(Near(v, o.getOrbitalVelocityAtUT(300.0), 1e-9));
    // NaN true anomaly -> default state and NaN
    Orbit::State bad;
    bad.w = 5.0;
    CHECK(std::isnan(o.GetOrbitalStateVectorsAtObT(std::numeric_limits<double>::quiet_NaN(), bad)));
    CHECK(bad.w == 0.0);
}

TEST_CASE("CelestialBodyRef contract: referenceBody, HasChild, frame velocity and true position chains") {
    FramesGuard guard;
    CelestialBodyRef sun = MakeBody("Sun", 261600000.0, 1.7468465610013681);
    CelestialBodyRef kerbin = MakeKerbin();
    CelestialBodyRef mun = MakeBody("Mun", 200000.0, 0.1660567000983534);
    sun.position = Vector3d(10.0, 20.0, 30.0);
    Orbit kerbinOrbit(0, 0, 13599840256.0, 0, 0, 3.14000010490417, 0, &sun);
    Orbit munOrbit(0, 0, 12000000.0, 0, 0, 1.70000004768372, 0, &kerbin);
    kerbin.orbit = &kerbinOrbit;
    mun.orbit = &munOrbit;
    sun.orbitingBodies.push_back(&kerbin);
    kerbin.orbitingBodies.push_back(&mun);

    CHECK(sun.referenceBody() == &sun);
    CHECK(kerbin.referenceBody() == &sun);
    CHECK(mun.referenceBody() == &kerbin);
    CHECK(sun.HasChild(&mun));
    CHECK(sun.HasChild(&kerbin));
    CHECK_FALSE(kerbin.HasChild(&sun));
    CHECK_FALSE(mun.HasChild(&kerbin));
    CHECK_FALSE(mun.HasChild(&mun));

    const double t = 5000.0;
    kerbinOrbit.UpdateFromUT(t);
    munOrbit.UpdateFromUT(t);
    CHECK(sun.GetFrameVel().Equals(Vector3d::zero()));
    CHECK(mun.GetFrameVel().Equals(munOrbit.vel + (kerbinOrbit.vel + Vector3d::zero())));
    CHECK(mun.GetFrameVelAtUT(t).Equals(munOrbit.getOrbitalVelocityAtUT(t) +
                                        (kerbinOrbit.getOrbitalVelocityAtUT(t) + Vector3d::zero())));
    CHECK(mun.getTruePositionAtUT(t).Equals(munOrbit.getRelativePositionAtUT(t).xzy() +
                                            (kerbinOrbit.getRelativePositionAtUT(t).xzy() + sun.position)));
    kerbin.position = Vector3d(-1.0, -2.0, -3.0);
    CHECK(mun.getPositionAtUT(t).Equals(munOrbit.getRelativePositionAtUT(t).xzy() + kerbin.position));
    CHECK(munOrbit.getTruePositionAtUT(t).Equals(mun.getTruePositionAtUT(t)));
    CHECK(munOrbit.GetFrameVel().Equals(munOrbit.vel + kerbin.GetFrameVel()));
    // a disabled OrbitDriver contributes nothing
    kerbin.orbitDriverEnabled = false;
    CHECK(kerbin.GetFrameVel().Equals(Vector3d::zero()));
    CHECK(kerbin.getTruePositionAtUT(t).Equals(kerbin.position));
    kerbin.orbitDriverEnabled = true;
    // GetVel subtracts the active vessel's reference-body frame velocity and swizzles
    const Vector3d gv = munOrbit.GetVel(&kerbin);
    const Vector3d d = munOrbit.GetFrameVel() - kerbin.GetFrameVel();
    CHECK(gv.Equals(Vector3d(d.x, d.z, d.y)));
    CHECK(munOrbit.GetVel().Equals(munOrbit.GetFrameVel().xzy()));
}

TEST_CASE("Orbit::UpdateFromOrbitAtUT: parent -> child, child -> parent and sibling transitions") {
    FramesGuard guard;
    CelestialBodyRef sun = MakeBody("Sun", 261600000.0, 1.7468465610013681);
    CelestialBodyRef kerbin = MakeKerbin();
    CelestialBodyRef mun = MakeBody("Mun", 200000.0, 0.1660567000983534);
    CelestialBodyRef minmus = MakeBody("Minmus", 60000.0, 0.05001708099187999);
    Orbit kerbinOrbit(0, 0, 13599840256.0, 0, 0, 3.14000010490417, 0, &sun);
    Orbit munOrbit(0, 0, 12000000.0, 0, 0, 1.70000004768372, 0, &kerbin);
    Orbit minmusOrbit(6, 0, 47000000.0, 78, 38, 0.899999976158142, 0, &kerbin);
    kerbin.orbit = &kerbinOrbit;
    mun.orbit = &munOrbit;
    minmus.orbit = &minmusOrbit;
    sun.orbitingBodies = {&kerbin};
    kerbin.orbitingBodies = {&mun, &minmus};

    const double t = 2.0e5;
    const Orbit vessel(1, 0.6, 7.0e6, 20, 30, 0.4, 0, &kerbin);
    // Kerbin -> Mun (orbit.referenceBody.HasChild(toBody))
    Orbit atMun;
    atMun.UpdateFromOrbitAtUT(vessel, t, &mun);
    CHECK(atMun.referenceBody == &mun);
    CHECK(atMun.epoch == t);
    const Vector3d rel = vessel.getRelativePositionAtUT(t) - munOrbit.getRelativePositionAtUT(t);
    CHECK((atMun.getRelativePositionAtUT(t) - rel).magnitude() <= 1e-6 * rel.magnitude());
    const Vector3d relV = vessel.getOrbitalVelocityAtUT(t) - munOrbit.getOrbitalVelocityAtUT(t);
    CHECK((atMun.getOrbitalVelocityAtUT(t) - relV).magnitude() <= 1e-9 * relV.magnitude());
    // Mun -> Kerbin (toBody.HasChild(orbit.referenceBody)) recovers the vessel orbit
    Orbit back;
    back.UpdateFromOrbitAtUT(atMun, t, &kerbin);
    CHECK(back.semiMajorAxis == doctest::Approx(vessel.semiMajorAxis).epsilon(1e-9));
    CHECK(back.eccentricity == doctest::Approx(vessel.eccentricity).epsilon(1e-9));
    CHECK((back.getRelativePositionAtUT(t + 500.0) - vessel.getRelativePositionAtUT(t + 500.0)).magnitude() <= 1e-3);
    // Mun -> Minmus (siblings: true positions and frame velocities)
    Orbit atMinmus;
    atMinmus.UpdateFromOrbitAtUT(atMun, t, &minmus);
    CHECK(atMinmus.referenceBody == &minmus);
    const Vector3d relM = vessel.getRelativePositionAtUT(t) - minmusOrbit.getRelativePositionAtUT(t);
    CHECK((atMinmus.getRelativePositionAtUT(t) - relM).magnitude() <= 1e-6 * relM.magnitude());
}

TEST_CASE("Orbit static helpers: nodes, inclinations, synodic period, Pe/Ap test, close approach") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    const Orbit p(0, 0.1, 2.0e6, 0, 30, 0, 0, &kerbin);
    const Orbit s(25, 0.05, 3.0e6, 60, 10, 1, 0, &kerbin);
    CHECK(p.GetRelativeInclination(s) == doctest::Approx(25.0).epsilon(1e-12));
    CHECK(Orbit::RelativeInclination(p, s) == doctest::Approx(25.0).epsilon(1e-12));
    // the node line lies in both planes
    const double an = Orbit::AscendingNodeTrueAnomaly(p, s);
    const double dn = Orbit::DescendingNodeTrueAnomaly(p, s);
    CHECK(dn == std::fmod(an + kPI, 2 * kPI));
    const Vector3d nodeDir = p.getRelativePositionFromTrueAnomaly(an).normalized();
    CHECK(std::fabs(Vector3d::Dot(nodeDir, s.GetOrbitNormal())) <= 1e-12);
    CHECK(Orbit::SynodicPeriod(p, s) == s.period * p.period / (s.period - p.period));
    const Orbit h(0, 1.5, -1.0e6, 0, 0, 0, 0, &kerbin);
    CHECK(Orbit::SynodicPeriod(p, h) == p.period);
    CHECK(Orbit::SynodicPeriod(h, p) == 0.0 - p.period);
    CHECK(std::isnan(Orbit::SynodicPeriod(h, h)));
    // Pe/Ap overlap: p spans 1.8e6..2.2e6, s spans 2.85e6..3.15e6
    CHECK_FALSE(Orbit::PeApIntersects(p, s, 0.0));
    CHECK(Orbit::PeApIntersects(p, s, 7.0e5));
    CHECK(Orbit::PeApIntersects(h, s, 0.0));  // hyperbolic primary: PeR < secondary ApR + threshold
    // close approach: the returned time lies within one period of UT
    const double tc = Orbit::NextCloseApproachTime(p, s, 0.0);
    CHECK(tc >= 0.0);
    CHECK(tc <= p.period);
    CHECK(Orbit::Swizzle({1, 2, 3}).Equals({1, 3, 2}));
}

TEST_CASE("Orbit: TrueAnomalyFromVector (Unity AngleAxis path) and GetTrueAnomalyOfZupVector") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    const Orbit o(0, 0.1, 2.0e6, 0, 30, 0, 0, &kerbin);
    for (double tA : {0.5, 2.0, 4.0}) {
        const Vector3d local = o.getRelativePositionFromTrueAnomaly(tA);
        CHECK(o.GetTrueAnomalyOfZupVector(local) == doctest::Approx(tA > kPI ? tA - 2 * kPI : tA).epsilon(1e-12));
        CHECK(o.TrueAnomalyFromVector(local.xzy()) == doctest::Approx(tA).epsilon(1e-6));
    }
}

TEST_CASE("Orbit: copies, enum names, delegates") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    Orbit o(5, 0.2, 3.0e6, 10, 20, 0.3, 50, &kerbin);
    Orbit next;
    o.nextPatch = &next;
    o.UpdateFromUT(400.0);
    const Orbit full = o;  // member-wise (C# MemberwiseClone semantics)
    CHECK(full.nextPatch == &next);
    CHECK(full.radius == o.radius);
    const Orbit elems = Orbit::CopyOf(o);  // `new Orbit(orbit)`: elements + Init only
    CHECK(elems.nextPatch == nullptr);
    CHECK(elems.radius == 0.0);
    CHECK(elems.meanMotion == o.meanMotion);
    CHECK(elems.referenceBody == &kerbin);
    CHECK(std::string(ToString(Orbit::PatchTransitionType::ENCOUNTER)) == "ENCOUNTER");
    CHECK(std::string(ToString(Orbit::EncounterSolutionLevel::SOI_INTERSECT_1)) == "SOI_INTERSECT_1");
    CHECK(std::string(ToString(Orbit::ObjectType::UNKNOWN_MISC)) == "UNKNOWN_MISC");
    CHECK(Orbit::SolveSOI == &Orbit::SolveSOIDefault);
    CHECK(Orbit::SolveClosestApproach == &Orbit::SolveClosestApproachDefault);
    double CD = 0, CCD = 0, FFp = 0, FFs = 0, SFp = 0, SFs = 0;
    int it = 0;
    const Orbit s(30, 0.1, 3.5e6, 40, 0, 0, 0, &kerbin);
    const int found = Orbit::FindClosestPoints(o, s, CD, CCD, FFp, FFs, SFp, SFs, 0.0001, 20, it);
    CHECK(found >= 0);
    CHECK(found <= 2);
    Orbit::LegacyOrbitTargeting = true;
    CHECK(Orbit::FindClosestPoints(o, s, CD, CCD, FFp, FFs, SFp, SFs, 0.0001, 20, it) == 2);
    Orbit::LegacyOrbitTargeting = false;
    CHECK(CD >= 0.0);
    CHECK(CCD >= 0.0);
}

TEST_CASE("Targeting::Conic: geometry matches Orbit, line intercepts, inside/real tests") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    const Orbit o(20, 0.3, 2.0e6, 40, 60, 0, 0, &kerbin);
    const Targeting::Conic c = Targeting::Conic::Borrow(o);
    CHECK(c.e == 0.3);
    CHECK(c.l == o.semiLatusRectum());
    CHECK(c.a == doctest::Approx(2.0e6).epsilon(1e-12));
    CHECK(c.pe == doctest::Approx(o.PeR()).epsilon(1e-12));
    CHECK(c.ap == doctest::Approx(o.ApR()).epsilon(1e-12));
    CHECK(c.max_v == kPI);
    CHECK(c.min_v == -kPI);
    for (double v : {0.0, 1.0, 2.5, -2.0}) {
        CHECK(c.Point(v).Equals(o.getPositionFromTrueAnomaly(v, false)));  // same expression, world-like frame
        CHECK(c.Radius(v) == doctest::Approx(o.RadiusAtTrueAnomaly(v)).epsilon(1e-14));
        CHECK(c.TrueAnomaly(c.Point(v)) == doctest::Approx(v).epsilon(1e-12));
        // the frame's tangent is along the velocity, the normal is perpendicular to it
        const Targeting::Frame f = c.GetFrame(c.Point(v));
        const Vector3d vel = o.getOrbitalVelocityAtTrueAnomaly(v, false);
        CHECK(Vector3d::Dot(f.t.normalized(), vel.normalized()) == doctest::Approx(1.0).epsilon(1e-12));
        CHECK(std::fabs(Vector3d::Dot(f.t, f.n)) <= 1e-12 * f.t.sqrMagnitude());
    }
    CHECK(c.point_inside(c.center));
    CHECK_FALSE(c.point_inside(c.Point(1.0) * 1.01));
    CHECK(c.point_inside(c.Point(1.0) * 0.99));
    CHECK(c.point_is_real(c.Point(1.0)));
    // a line through the centre along X meets the ellipse at the periapsis and apoapsis points
    Vector3d i1, i2;
    REQUIRE(c.Intercepts(Targeting::Ray(c.center, c.X), i1, i2) == 2);
    CHECK((i1 - c.pe_point).magnitude() <= 1e-6);
    CHECK((i2 - c.ap_point).magnitude() <= 1e-6);
    // a line outside the ellipse misses it
    CHECK(c.Intercepts(Targeting::Ray(c.center + c.Y * (3.0 * c.b), c.X), i1, i2) == 0);
    // hyperbola: asymptote limits, only the near branch is real
    const Orbit h(0, 1.5, -1.0e6, 0, 0, 0, 0, &kerbin);
    const Targeting::Conic ch = Targeting::Conic::Borrow(h);
    CHECK(ch.max_v == dotnet::Math::Acos(-1.0 / 1.5));
    CHECK(ch.ap == kInf);
    CHECK(ch.point_is_real(ch.Point(0.5)));
    CHECK_FALSE(ch.point_is_real(ch.center + ch.X * 1.0e9));
    // rays and planes
    const Targeting::Ray r({0, 0, 0}, {1, 0, 0});
    CHECK(r.ClosestPoint(Vector3d(5, 3, 0)).Equals(Vector3d(5, 0, 0)));
    CHECK(r.ClosestPoint(Vector3d(-5, 3, 0), true).Equals(Vector3d(0, 0, 0)));
    CHECK(Targeting::Plane({0, 0, 2}, {0, 0, 1}).Intersept(Targeting::Ray({1, 1, 0}, {0, 0, 1})).Equals(Vector3d(1, 1, 2)));
    CHECK(r.ClosestPoint(Targeting::Ray({3, 1, 5}, {0, 0, 1})).Equals(Vector3d(3, 0, 0)));
}

TEST_CASE("Orbit::FindClosestPoints (Targeting.Intercepts) finds the true minimum distance between two conics") {
    FramesGuard guard;
    CelestialBodyRef kerbin = MakeKerbin();
    struct Pair {
        Orbit p, s;
    };
    const Pair pairs[] = {
        {Orbit(0, 0.6, 7.0e6, 0, 30, 0, 0, &kerbin), Orbit(0, 0, 1.2e7, 0, 0, 1.7, 0, &kerbin)},     // Mun-like crossing
        {Orbit(10, 0.2, 3.0e6, 20, 40, 0, 0, &kerbin), Orbit(35, 0.1, 3.6e6, 80, 10, 0, 0, &kerbin)},  // inclined
        {Orbit(4, 0.95, 4.0e7, 70, 10, 0, 0, &kerbin), Orbit(6, 0, 4.7e7, 78, 38, 0.9, 0, &kerbin)},  // Minmus-like
        {Orbit(2, 1.6, -2.0e6, 40, 75, 0, 0, &kerbin), Orbit(0, 0, 1.2e7, 0, 0, 1.7, 0, &kerbin)},   // escape
    };
    for (const Pair& pr : pairs) {
        double CD = 0, CCD = 0, FFp = 0, FFs = 0, SFp = 0, SFs = 0;
        int it = 0;
        const int n = Orbit::FindClosestPoints(pr.p, pr.s, CD, CCD, FFp, FFs, SFp, SFs, 0.0001, 20, it);
        REQUIRE(n >= 1);
        // the reported anomalies reproduce the reported distances
        const double dFirst = (pr.p.getPositionFromTrueAnomaly(FFp, false) - pr.s.getPositionFromTrueAnomaly(FFs, false)).magnitude();
        CHECK(dFirst == doctest::Approx(CD).epsilon(1e-6));
        const double dSecond = (pr.p.getPositionFromTrueAnomaly(SFp, false) - pr.s.getPositionFromTrueAnomaly(SFs, false)).magnitude();
        CHECK(dSecond == doctest::Approx(CCD).epsilon(1e-6));
        // brute force over both anomalies (1,000 x 1,000 grid, then zoomed grids) agrees with the best minimum
        const Targeting::Conic cp = Targeting::Conic::Borrow(pr.p), cs = Targeting::Conic::Borrow(pr.s);
        double best = std::numeric_limits<double>::max(), bv1 = 0, bv2 = 0;
        const int N = 1000;
        std::vector<Vector3d> sPts(N);
        for (int b = 0; b < N; ++b) sPts[static_cast<size_t>(b)] = cs.Point(cs.min_v + (cs.max_v - cs.min_v) * (b + 0.5) / N);
        for (int a = 0; a < N; ++a) {
            const double v1 = cp.min_v + (cp.max_v - cp.min_v) * (a + 0.5) / N;
            const Vector3d P = cp.Point(v1);
            for (int b = 0; b < N; ++b) {
                const double d = (P - sPts[static_cast<size_t>(b)]).sqrMagnitude();
                if (d < best) {
                    best = d;
                    bv1 = v1;
                    bv2 = cs.min_v + (cs.max_v - cs.min_v) * (b + 0.5) / N;
                }
            }
        }
        // zoom: 81 x 81 grids over +-2 cells of the previous level, eight levels
        double h1 = (cp.max_v - cp.min_v) / N, h2 = (cs.max_v - cs.min_v) / N;
        for (int level = 0; level < 8; ++level) {
            const double c1 = bv1, c2 = bv2;
            std::vector<Vector3d> zPts;
            for (int b = -40; b <= 40; ++b) zPts.push_back(cs.Point(c2 + h2 * 2.0 * b / 40.0));
            for (int a = -40; a <= 40; ++a) {
                const double v1 = c1 + h1 * 2.0 * a / 40.0;
                const Vector3d P = cp.Point(v1);
                for (int b = -40; b <= 40; ++b) {
                    const double v2 = c2 + h2 * 2.0 * b / 40.0;
                    const double d = (P - zPts[static_cast<size_t>(b + 40)]).sqrMagnitude();
                    if (d < best) {
                        best = d;
                        bv1 = v1;
                        bv2 = v2;
                    }
                }
            }
            h1 *= 2.0 / 40.0;
            h2 *= 2.0 / 40.0;
        }
        CHECK(std::min(CD, CCD) == doctest::Approx(std::sqrt(best)).epsilon(1e-6));
    }
}
