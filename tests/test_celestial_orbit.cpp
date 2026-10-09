// Integration WP-07/WP-09: the Kerbol system spawned with its real run-time Orbits (CelestialSystem::Spawn's default
// factory, CelestialBodyOrbitAdapter over ksp::Orbit) against harness/golden/body_catalog: orbit states, tidal lock,
// solar day, world positions and the position / frame-velocity chain Mun -> Kerbin -> Sun.
//
// Frames: the PSystem prefab ships the Mun and Minmus with inverseRotation = 1, so the first CBUpdate pass (KSP code,
// ported faithfully) sets Planetarium.InverseRotAngle to 230 and turns Planetarium.Zup; the harness's CBUpdate skips that
// write, so its orbit states are in the Awake frame (inverseRotAngle 0). The first test checks the faithful state
// (frame-independent quantities bit-exact, x/y components equal after undoing the frame), the second restores the harness
// frame exactly like the golden runner (CelestialEnv::UseHarnessFrame) and compares bit for bit.
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/celestial/celestial_body_orbit.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;

namespace {

const std::string kHarness = KSPCORE_HARNESS_DIR;

bool SameBits(double a, double b) { return std::memcmp(&a, &b, sizeof a) == 0; }
bool SameBits(const Vector3d& a, const Vector3d& b) { return SameBits(a.x, b.x) && SameBits(a.y, b.y) && SameBits(a.z, b.z); }

// One table of the body_catalog golden, rows by body name.
struct GoldenTable {
    std::vector<std::string> cols;
    std::map<std::string, std::vector<nlohmann::json>> rows;
    double D(const std::string& body, const std::string& col) const {
        const auto& r = rows.at(body);
        for (size_t i = 0; i < cols.size(); ++i) {
            if (cols[i] != col) continue;
            if (r[i].is_string()) {  // non-finite values are written as strings
                const std::string s = r[i].get<std::string>();
                if (s == "Infinity") return HUGE_VAL;
                if (s == "-Infinity") return -HUGE_VAL;
                return std::nan("");
            }
            return r[i].get<double>();
        }
        throw std::out_of_range("no column " + col);
    }
    Vector3d V(const std::string& body, const char* prefix) const {
        const std::string p = prefix;
        return {D(body, p + "_x"), D(body, p + "_y"), D(body, p + "_z")};
    }
};

const nlohmann::json& GoldenFile() {
    static const nlohmann::json j = [] {
        const auto text = dotnet::ReadAllText(kHarness + "/golden/body_catalog/body_catalog.json");
        REQUIRE(text.has_value());
        return nlohmann::json::parse(*text);
    }();
    return j;
}

GoldenTable Golden(const char* table) {
    GoldenTable g;
    for (const auto& t : GoldenFile().at("tables")) {
        if (t.at("name") != table) continue;
        for (const auto& c : t.at("columns")) g.cols.push_back(c.at("name").get<std::string>());
        for (const auto& r : t.at("rows")) g.rows[r.at(0).get<std::string>()] = r.get<std::vector<nlohmann::json>>();
    }
    REQUIRE(!g.rows.empty());
    return g;
}

// The in-game start-up: PhysicsGlobals (Physics.cfg), an Awake Planetarium, the spawned system with real orbits.
struct Kerbol {
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    Kerbol() {
        pg.physicsDatabaseFilename = kHarness + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
    }
    ~Kerbol() {
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);  // static: do not leak the turned frame into other tests
    }
    CelestialBody& Body(const char* name) const { return *sys->GetBodyByName(name); }
    const Orbit& O(const char* name) const { return *BodyOrbit(sys->GetBodyByName(name)); }

    // As the golden runner's CelestialEnv::UseHarnessFrame: Awake frame for inverseRotAngle 0, then OrbitDriver.UpdateOrbit
    // for every body in Planetarium.UpdateCBsRecursive order.
    void UseHarnessFrame() {
        planetarium.inverseRotAngle = 0.0;
        PlanetariumAwakeFrames(0.0, Planetarium::Zup, planetarium.rotation);
        struct Pass {
            static void Run(CelestialBody* cb) {
                if (cb->orbitDriver && cb->orbitDriver->link != nullptr) cb->orbitDriver->link->UpdateOrbit();
                for (CelestialBody* c : cb->orbitingBodies)
                    if (!c->orbitDriver || !c->orbitDriver->reverse) Run(c);
                if (cb->orbitDriver && cb->orbitDriver->reverse) Run(cb->referenceBody());
            }
        };
        Pass::Run(planetarium.FindRootBody(planetarium.Sun));
    }
};

const char* const kFrameFree[] = {"inclination", "eccentricity", "semiMajorAxis", "LAN", "argumentOfPeriapsis",
                                  "meanAnomalyAtEpoch", "epoch", "meanMotion", "period", "PeR", "ApR", "ObT_UT0",
                                  "meanAnomaly_UT0", "eccentricAnomaly_UT0", "trueAnomaly_UT0", "radius_UT0",
                                  "orbitalSpeed_UT0", "pos_z", "vel_z"};

std::vector<double> OrbitColumns(const Orbit& o) {
    return {o.inclination, o.eccentricity, o.semiMajorAxis, o.LAN, o.argumentOfPeriapsis, o.meanAnomalyAtEpoch, o.epoch,
            o.meanMotion, o.period, o.PeR(), o.ApR(), o.ObT, o.meanAnomaly, o.eccentricAnomaly, o.trueAnomaly, o.radius,
            o.orbitalSpeed, o.pos.z, o.vel.z};
}

}  // namespace

TEST_CASE("Kerbol system with real orbits: links, faithful first CBUpdate pass, tidal lock, position chains") {
    Kerbol k;
    const GoldenTable orbits = Golden("orbits");
    const GoldenTable bodies = Golden("bodies");
    REQUIRE(k.sys->Bodies().size() == 17);

    // every body with an OrbitDriver has its own ksp::Orbit behind the link, parented like the driver
    for (CelestialBody* b : k.sys->Bodies()) {
        INFO(b->bodyName);
        if (!b->orbitDriver) {
            CHECK(b->orbit() == nullptr);
            CHECK(BodyOrbit(b) == nullptr);
            continue;
        }
        REQUIRE(BodyOrbit(b) != nullptr);
        CHECK(b->orbit()->GetOrbit() == BodyOrbit(b));
        CHECK(BodyOrbit(b)->referenceBody == b->referenceBody());
        CHECK(dynamic_cast<CelestialBodyOrbitAdapter*>(b->orbit()) != nullptr);
    }

    // KSP's CBUpdate inverse-rotation branch ran for the Mun and Minmus (prefab inverseRotation = 1): the frame turned by
    // their rotation angle at UT 0 (initialRotation 230, directRotAngle 0)
    CHECK(k.Body("Mun").inverseRotation);
    CHECK(Planetarium::InverseRotAngle() == 230.0);
    const CelestialFrame zup0 = Planetarium::AwakeZup(0.0);
    CelestialFrame zup230;
    CelestialFrame::PlanetaryFrame(0.0, 90.0, 230.0, zup230);
    CHECK(SameBits(Planetarium::Zup.X, zup230.X));
    CHECK(SameBits(Planetarium::Zup.Z, zup230.Z));

    for (CelestialBody* b : k.sys->Bodies()) {
        if (!b->orbitDriver) continue;
        INFO(b->bodyName);
        const Orbit& o = *BodyOrbit(b);
        // frame-independent orbit state: bit-exact even in the turned frame, except orbitalSpeed = |vel| (its
        // components are rotated, so the last bit may differ)
        const std::vector<double> got = OrbitColumns(o);
        for (size_t i = 0; i < got.size(); ++i) {
            const std::string col = kFrameFree[i];
            INFO(col);
            const double want = orbits.D(b->bodyName, col);
            if (col == "orbitalSpeed_UT0")
                CHECK(std::fabs(got[i] - want) <= 4e-16 * want);
            else
                CHECK(SameBits(got[i], want));
        }
        // pos/vel: Kerbin, updated in the CBUpdate pass before the Mun turned the frame, is the golden bit for bit; the
        // others equal the golden after undoing the frame (Zup230.LocalToWorld vs Zup0.LocalToWorld)
        const Vector3d gPos = orbits.V(b->bodyName, "pos"), gVel = orbits.V(b->bodyName, "vel");
        if (b->bodyName == "Kerbin") {
            CHECK((SameBits(o.pos, gPos) && SameBits(o.vel, gVel)));
        } else {
            const Vector3d wPos = Planetarium::Zup.LocalToWorld(o.pos), wVel = Planetarium::Zup.LocalToWorld(o.vel);
            CHECK((wPos - zup0.LocalToWorld(gPos)).magnitude() <= 1e-15 * gPos.magnitude() * 8);
            CHECK((wVel - zup0.LocalToWorld(gVel)).magnitude() <= 1e-15 * gVel.magnitude() * 8);
        }
        // CBUpdate with the real Orbit: tidal lock and solar day length equal the golden bit for bit
        CHECK(SameBits(b->rotationPeriod, bodies.D(b->bodyName, "rotationPeriod")));
        CHECK(SameBits(b->rotPeriodRecip, bodies.D(b->bodyName, "rotPeriodRecip")));
        CHECK(SameBits(b->solarDayLength, bodies.D(b->bodyName, "solarDayLength")));
        if (b->tidallyLocked) {
            CHECK(b->rotationPeriod == o.period);
            CHECK(b->rotationPeriod == orbits.D(b->bodyName, "period"));
            CHECK(b->rotPeriodRecip == 1.0 / o.period);
        }
    }
    int locked = 0;
    for (const CelestialBody* b : k.sys->Bodies()) locked += b->tidallyLocked ? 1 : 0;
    CHECK(locked == 7);  // Mun, Ike, Laythe, Vall, Tylo, Bop, Pol

    // OrbitDriver.updateFromParameters moved every body: Kerbin (reversed driver, root of the CBUpdate pass) stays at
    // the origin and moves the Sun; everything else sits at parent.position + orbit.pos.xzy
    const CelestialBody& sun = k.Body("Sun");
    const CelestialBody& kerbin = k.Body("Kerbin");
    const CelestialBody& mun = k.Body("Mun");
    CHECK(kerbin.orbitDriver->reverse);
    CHECK(kerbin.position.Equals(Vector3d::zero()));
    CHECK(SameBits(sun.position, kerbin.position - k.O("Kerbin").pos.xzy()));
    for (CelestialBody* b : k.sys->Bodies()) {
        if (!b->orbitDriver || b->orbitDriver->reverse) continue;
        INFO(b->bodyName);
        CHECK(SameBits(b->position, b->referenceBody()->position + BodyOrbit(b)->pos.xzy()));
    }

    // getPositionAtUT / getTruePositionAtUT / GetFrameVelAtUT chain through the parents (CelestialBody.cs:610-650)
    const Orbit& munO = k.O("Mun");
    const Orbit& kerbinO = k.O("Kerbin");
    for (double ut : {0.0, 1234.5, 0.5 * munO.period, 3.0e6, 1.0e8}) {
        INFO("UT " << ut);
        CHECK(SameBits(mun.getPositionAtUT(ut), munO.getRelativePositionAtUT(ut).xzy() + kerbin.position));
        CHECK(SameBits(mun.GetPositionAtUT(ut), mun.getPositionAtUT(ut)));
        CHECK(SameBits(mun.getTruePositionAtUT(ut),
                       munO.getRelativePositionAtUT(ut).xzy() + (kerbinO.getRelativePositionAtUT(ut).xzy() + sun.position)));
        CHECK(SameBits(munO.getTruePositionAtUT(ut), mun.getTruePositionAtUT(ut)));
        CHECK(SameBits(mun.GetFrameVelAtUT(ut),
                       munO.getOrbitalVelocityAtUT(ut) + (kerbinO.getOrbitalVelocityAtUT(ut) + Vector3d::zero())));
    }
    CHECK(SameBits(mun.GetFrameVel(), munO.vel + (kerbinO.vel + Vector3d::zero())));
    CHECK(sun.GetFrameVel().Equals(Vector3d::zero()));

    // Planetarium.FixedUpdate advances time and every orbit with it
    k.planetarium.FixedUpdate();
    CHECK(k.planetarium.time == 0.02);
    CHECK(munO.ObT == munO.getObtAtUT(0.02));
    CHECK(SameBits(mun.position, kerbin.position + munO.pos.xzy()));
}

TEST_CASE("Kerbol system with real orbits in the harness frame: body_catalog orbits and world positions bit for bit") {
    Kerbol k;
    k.UseHarnessFrame();
    const GoldenTable orbits = Golden("orbits");
    REQUIRE(orbits.rows.size() == 16);

    // every column of the orbits table, every body
    for (const auto& [name, row] : orbits.rows) {
        INFO(name);
        const CelestialBody* b = k.sys->GetBodyByName(name);
        REQUIRE(b != nullptr);
        const Orbit& o = *BodyOrbit(b);
        CHECK(o.referenceBody->bodyName == row.at(1).get<std::string>());
        CHECK(b->orbitDriver->reverse == row.at(2).get<bool>());
        const std::vector<double> got = OrbitColumns(o);
        for (size_t i = 0; i < got.size(); ++i) {
            INFO(std::string(kFrameFree[i]));
            CHECK(SameBits(got[i], orbits.D(name, kFrameFree[i])));
        }
        CHECK(SameBits(o.pos, orbits.V(name, "pos")));
        CHECK(SameBits(o.vel, orbits.V(name, "vel")));
    }

    // world positions the golden implies at UT 0 (Unity axes = zup.xzy): Kerbin at the origin, the Sun at -Kerbin's
    // orbit position, the Mun and Minmus at Kerbin + their orbit positions, the planets at Sun + theirs
    const CelestialBody& sun = k.Body("Sun");
    const CelestialBody& kerbin = k.Body("Kerbin");
    const CelestialBody& mun = k.Body("Mun");
    const Vector3d gKerbin = orbits.V("Kerbin", "pos").xzy();
    const Vector3d gMun = orbits.V("Mun", "pos").xzy();
    const Vector3d expectSun = Vector3d::zero() - gKerbin;
    CHECK(kerbin.position.Equals(Vector3d::zero()));
    CHECK(SameBits(sun.position, expectSun));
    CHECK(SameBits(mun.position, Vector3d::zero() + gMun));
    CHECK(SameBits(k.Body("Minmus").position, Vector3d::zero() + orbits.V("Minmus", "pos").xzy()));
    CHECK(SameBits(k.Body("Duna").position, expectSun + orbits.V("Duna", "pos").xzy()));
    CHECK(SameBits(k.Body("Ike").position, (expectSun + orbits.V("Duna", "pos").xzy()) + orbits.V("Ike", "pos").xzy()));
    CHECK(SameBits(k.Body("Laythe").position, (expectSun + orbits.V("Jool", "pos").xzy()) + orbits.V("Laythe", "pos").xzy()));

    // the Mun at UT 0 through CelestialBody's API: relative position at UT (getRelativePositionAtUT, a separate code
    // path from UpdateFromUT) and the chain Mun -> Kerbin -> Sun
    const Orbit& munO = k.O("Mun");
    const double r = orbits.D("Mun", "radius_UT0");
    CHECK((mun.getPositionAtUT(0.0) - gMun).magnitude() <= 1e-15 * r * 4);
    CHECK((mun.getTruePositionAtUT(0.0) - (gMun + (gKerbin + expectSun))).magnitude() <= 1e-15 * r * 4);
    CHECK(SameBits(mun.getTruePositionAtUT(0.0),
                   munO.getRelativePositionAtUT(0.0).xzy() + (k.O("Kerbin").getRelativePositionAtUT(0.0).xzy() + sun.position)));

    // the Mun at later UTs (circular equatorial orbit, period from the golden): back at the UT-0 position after whole
    // periods, opposite after half a period, a quarter turn prograde (+90 degrees about +z, zup frame) after a quarter
    const double P = orbits.D("Mun", "period");
    const Vector3d g0 = orbits.V("Mun", "pos");
    for (double n : {1.0, 2.0, 10.0}) {
        INFO("periods " << n);
        CHECK((munO.getRelativePositionAtUT(n * P) - g0).magnitude() <= 1e-9 * r);
        CHECK((mun.getPositionAtUT(n * P) - (kerbin.position + g0.xzy())).magnitude() <= 1e-9 * r);
    }
    CHECK((munO.getRelativePositionAtUT(0.5 * P) + g0).magnitude() <= 1e-9 * r);
    CHECK((munO.getRelativePositionAtUT(0.25 * P) - Vector3d(-g0.y, g0.x, g0.z)).magnitude() <= 1e-9 * r);
    CHECK(std::fabs(munO.getRelativePositionAtUT(0.3 * P).magnitude() - r) <= 1e-9 * r);
}
