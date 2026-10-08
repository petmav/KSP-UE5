// WP-09 unit tests: Kerbol-system catalog integrity (against the game's serialized prefab, bit for bit), spawn/
// hierarchy, atmosphere edge cases, FlightIntegrator atmosphere edge cases, PhysicsGlobals load/save/enums/properties,
// biome-map metadata and asset origins.
#include <cmath>
#include <cstring>
#include <filesystem>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <string>

#include "doctest/doctest.h"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/flight_atmosphere.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;

namespace {

const std::string kHarness = KSPCORE_HARNESS_DIR;
const std::string kProjectRoot = KSPCORE_PROJECT_ROOT;

bool SameBits(double a, double b) { return std::memcmp(&a, &b, sizeof a) == 0; }

// The in-game start-up order (as the golden runner's CelestialEnv, without a run-time Orbit unless given).
struct Env {
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    explicit Env(const CelestialSystem::OrbitFactory& factory = nullptr) {
        pg.physicsDatabaseFilename = kHarness + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        pg.bodyLiftCurve = PhysicsGlobals::GetLiftingSurfaceCurve("BodyLift");
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium, factory);
        pg.CalculateValues(sys->Bodies(), planetarium.Sun);
    }
    ~Env() { sys.reset(); }
    CelestialBody& Body(const char* name) const { return *sys->GetBodyByName(name); }
};

// Orbit.period as Orbit.Init computes it (Orbit.cs:334/370) - test stand-in for WP-07's Orbit.
struct PeriodOnlyOrbit : ICelestialBodyOrbit {
    double p;
    explicit PeriodOnlyOrbit(const CelestialBody& b) {
        const double a = std::fabs(b.orbitDriver->orbit.semiMajorAxis);
        p = UtilMath::PI * 2.0 / std::sqrt(b.orbitDriver->referenceBody->gravParameter / (a * a * a));
    }
    double period() const override { return p; }
    double trueAnomaly() const override { return 0; }
    double radius() const override { return 0; }
    double PeR() const override { return 0; }
    double ApR() const override { return 0; }
    Vector3d vel() const override { return {}; }
    Vector3d getRelativePositionAtUT(double) const override { return {}; }
    Vector3d getOrbitalVelocityAtUT(double) const override { return {}; }
    void UpdateOrbit() override {}
};

}  // namespace

TEST_CASE("Kerbol catalog: 17 bodies, PSystem tree, home world") {
    const PSystemPrefab& s = KerbolSystemPrefab();
    REQUIRE(s.bodies.size() == 17);
    CHECK(s.bodies[0].body.bodyName == "Sun");
    CHECK(s.bodies[0].parent.empty());
    CHECK_FALSE(s.bodies[0].orbitDriver.has_value());
    std::set<int> indices;
    std::set<std::string> seen;
    int homes = 0;
    for (size_t i = 0; i < s.bodies.size(); ++i) {
        const CelestialBodyPrefab& p = s.bodies[i];
        CHECK(p.spawnOrder == static_cast<int>(i));
        indices.insert(p.flightGlobalsIndex);
        if (i > 0) {
            CHECK(seen.count(p.parent) == 1);  // depth-first: the parent was spawned earlier
            CHECK(p.orbitDriver.has_value());
        }
        seen.insert(p.body.bodyName);
        homes += p.body.isHomeWorld ? 1 : 0;
    }
    CHECK(indices.size() == 17);
    CHECK(*indices.begin() == 0);
    CHECK(*indices.rbegin() == 16);
    CHECK(homes == 1);
    const std::map<std::string, std::string> parents = {{"Kerbin", "Sun"}, {"Mun", "Kerbin"}, {"Minmus", "Kerbin"},
                                                        {"Gilly", "Eve"},  {"Ike", "Duna"},    {"Laythe", "Jool"},
                                                        {"Pol", "Jool"},   {"Eeloo", "Sun"}};
    for (const auto& p : s.bodies)
        if (parents.count(p.body.bodyName)) CHECK(parents.at(p.body.bodyName) == p.parent);
}

TEST_CASE("Kerbol catalog equals the serialized PSystem prefab bit for bit") {
    // Every scalar of every CelestialBody / OrbitDriver in harness/golden/serialized/celestial_bodies.serialized.json
    // (extracted from decompiled/KSP-dev/KSP_x64_Data/sharedassets9.assets) must be in the catalog unchanged.
    const auto text = dotnet::ReadAllText(kHarness + "/golden/serialized/celestial_bodies.serialized.json");
    REQUIRE(text.has_value());
    const nlohmann::json root = nlohmann::json::parse(*text);
    using Get = std::function<double(const CelestialBody&)>;
#define F(f) {#f, [](const CelestialBody& b) { return static_cast<double>(b.f); }}
    const std::map<std::string, Get> scalars = {
        F(GeeASL), F(Radius), F(Mass), F(Density), F(SurfaceArea), F(gravParameter), F(sphereOfInfluence), F(hillSphere),
        F(gMagnitudeAtCenter), F(atmDensityASL), F(atmPressureASL), F(scaledEllipsoid), F(scaledRadiusHorizonMultiplier),
        F(navballSwitchRadiusMult), F(navballSwitchRadiusMultLow), F(use_The_InName), F(isHomeWorld), F(isCometPerturber),
        F(ocean), F(oceanUseFog), F(oceanFogPQSDepth), F(oceanFogPQSDepthRecip), F(oceanFogDensityStart), F(oceanFogDensityEnd),
        F(oceanFogDensityPQSMult), F(oceanFogDensityAltScalar), F(oceanFogDensityExponent), F(oceanFogDawnFactor),
        F(oceanSkyColorMult), F(oceanSkyColorOpacityBase), F(oceanSkyColorOpacityAltMult), F(oceanDensity), F(oceanAFGBase),
        F(oceanAFGAltMult), F(oceanAFGMin), F(oceanSunBase), F(oceanSunAltMult), F(oceanSunMin), F(oceanAFGLerp),
        F(oceanMinAlphaFogDistance), F(oceanMaxAlbedoFog), F(oceanMaxAlphaFog), F(oceanAlbedoDistanceScalar),
        F(oceanAlphaDistanceScalar), F(minOrbitalDistance), F(atmosphere), F(atmosphereContainsOxygen), F(atmosphereDepth),
        F(atmosphereTemperatureSeaLevel), F(atmospherePressureSeaLevel), F(atmosphereMolarMass), F(atmosphereAdiabaticIndex),
        F(atmosphereTemperatureLapseRate), F(atmosphereGasMassLapseRate), F(atmosphereUseTemperatureCurve),
        F(atmosphereTemperatureCurveIsNormalized), F(maxAxialDot), F(albedo), F(emissivity), F(coreTemperatureOffset),
        F(convectionMultiplier), F(shockTemperatureMultiplier), F(bodyTemperature), F(atmosphereUsePressureCurve),
        F(atmospherePressureCurveIsNormalized), F(radiusAtmoFactor), F(hasSolidSurface), F(isStar), F(rotates),
        F(rotationPeriod), F(rotPeriodRecip), F(solarDayLength), F(solarRotationPeriod), F(initialRotation), F(rotationAngle),
        F(directRotAngle), F(tidallyLocked), F(clampInverseRotThreshold), F(inverseRotation), F(inverseRotThresholdAltitude),
        F(angularV)};
#undef F
    const std::set<std::string> nonScalar = {"m_Enabled", "_pathID", "bodyType"};
    const std::map<std::string, const FloatCurve CelestialBody::*> curves = {
        {"atmosphereTemperatureCurve", &CelestialBody::atmosphereTemperatureCurve},
        {"latitudeTemperatureBiasCurve", &CelestialBody::latitudeTemperatureBiasCurve},
        {"latitudeTemperatureSunMultCurve", &CelestialBody::latitudeTemperatureSunMultCurve},
        {"axialTemperatureSunMultCurve", &CelestialBody::axialTemperatureSunMultCurve},
        {"axialTemperatureSunBiasCurve", &CelestialBody::axialTemperatureSunBiasCurve},
        {"atmosphereTemperatureSunMultCurve", &CelestialBody::atmosphereTemperatureSunMultCurve},
        {"eccentricityTemperatureBiasCurve", &CelestialBody::eccentricityTemperatureBiasCurve},
        {"atmospherePressureCurve", &CelestialBody::atmospherePressureCurve}};
    const PSystemPrefab& s = KerbolSystemPrefab();
    REQUIRE(root.at("bodies").size() == s.bodies.size());
    size_t compared = 0;
    for (size_t i = 0; i < s.bodies.size(); ++i) {
        const nlohmann::json& e = root.at("bodies")[i];
        const nlohmann::json& cb = e.at("celestialBody");
        const CelestialBody& b = s.bodies[i].body;
        INFO(b.bodyName);
        CHECK(cb.at("bodyName").get<std::string>() == b.bodyName);
        CHECK(e.at("flightGlobalsIndex").get<int>() == s.bodies[i].flightGlobalsIndex);
        for (const auto& [k, v] : cb.items()) {
            if (v.is_number() || v.is_boolean()) {
                if (nonScalar.count(k)) continue;
                INFO(k);
                REQUIRE(scalars.count(k) == 1);
                CHECK(SameBits(scalars.at(k)(b), v.is_boolean() ? (v.get<bool>() ? 1.0 : 0.0) : v.get<double>()));
                ++compared;
            } else if (curves.count(k)) {
                INFO(k);
                const FloatCurve& fc = b.*curves.at(k);
                const auto& keys = fc.Curve().keys();
                const nlohmann::json& jk = v.at("fCurve").at("m_Curve");
                REQUIRE(keys.size() == jk.size());
                for (size_t j = 0; j < keys.size(); ++j) {
                    CHECK(SameBits(keys[j].time, jk[j].at("time").get<double>()));
                    CHECK(SameBits(keys[j].value, jk[j].at("value").get<double>()));
                    CHECK(SameBits(keys[j].inTangent, jk[j].at("inSlope").get<double>()));
                    CHECK(SameBits(keys[j].outTangent, jk[j].at("outSlope").get<double>()));
                    CHECK(SameBits(keys[j].inWeight, jk[j].at("inWeight").get<double>()));
                    CHECK(SameBits(keys[j].outWeight, jk[j].at("outWeight").get<double>()));
                }
                CHECK(SameBits(fc.minTime(), v.at("_minTime").get<double>()));
                CHECK(SameBits(fc.maxTime(), v.at("_maxTime").get<double>()));
                compared += keys.size();
            }
        }
        const auto& tw = cb.at("timeWarpAltitudeLimits");
        REQUIRE(tw.size() == b.timeWarpAltitudeLimits.size());
        for (size_t j = 0; j < tw.size(); ++j) CHECK(SameBits(b.timeWarpAltitudeLimits[j], tw[j].get<double>()));
        const auto& sv = cb.at("scienceValues");
        CHECK(SameBits(b.scienceValues.LandedDataValue, sv.at("LandedDataValue").get<double>()));
        CHECK(SameBits(b.scienceValues.RecoveryValue, sv.at("RecoveryValue").get<double>()));
        CHECK(SameBits(b.scienceValues.spaceAltitudeThreshold, sv.at("spaceAltitudeThreshold").get<double>()));
        CHECK(SameBits(b.atmosphericAmbientColor.g, cb.at("atmosphericAmbientColor").at("g").get<double>()));
        if (e.at("orbitDriver").is_object()) {
            const auto& o = e.at("orbitDriver").at("orbit");
            const CelestialOrbitElements& el = s.bodies[i].orbitDriver->orbit;
            CHECK(SameBits(el.semiMajorAxis, o.at("semiMajorAxis").get<double>()));
            CHECK(SameBits(el.eccentricity, o.at("eccentricity").get<double>()));
            CHECK(SameBits(el.inclination, o.at("inclination").get<double>()));
            CHECK(SameBits(el.LAN, o.at("LAN").get<double>()));
            CHECK(SameBits(el.argumentOfPeriapsis, o.at("argumentOfPeriapsis").get<double>()));
            CHECK(SameBits(el.meanAnomalyAtEpoch, o.at("meanAnomalyAtEpoch").get<double>()));
            CHECK(SameBits(el.epoch, o.at("epoch").get<double>()));
            CHECK(s.bodies[i].orbitDriver->reverse == (e.at("orbitDriver").at("reverse").get<int>() != 0));
        }
    }
    CHECK(compared > 17 * 80);
}

TEST_CASE("spawned system: hierarchy, SOI ordering, home body, Kerbin constants") {
    Env env([](CelestialBody& b) { return std::make_unique<PeriodOnlyOrbit>(b); });
    REQUIRE(env.sys->Bodies().size() == 17);
    for (size_t i = 0; i < env.sys->Bodies().size(); ++i) CHECK(env.sys->Bodies()[i]->flightGlobalsIndex == static_cast<int>(i));
    CelestialBody& sun = env.Body("Sun");
    CelestialBody& kerbin = env.Body("Kerbin");
    CelestialBody& mun = env.Body("Mun");
    CHECK(env.sys->GetHomeBody() == &kerbin);
    CHECK(env.planetarium.Home == &kerbin);
    CHECK(env.planetarium.Sun == &sun);
    CHECK(env.planetarium.CurrentMainBody == &kerbin);  // FindRootBody: Kerbin's OrbitDriver is reversed
    CHECK(env.planetarium.FindRootBody(&sun) == &kerbin);
    CHECK(sun.referenceBody() == &sun);
    CHECK(mun.referenceBody() == &kerbin);
    CHECK(mun.HasParent(&sun));
    CHECK_FALSE(sun.HasParent(&mun));
    CHECK(sun.HasChild(&mun));
    CHECK(CelestialBody::GetBodyReferencing(&mun, &sun) == &kerbin);
    CHECK(std::isinf(sun.sphereOfInfluence));
    for (CelestialBody* b : env.sys->Bodies()) {
        if (!b->orbitDriver) continue;
        INFO(b->bodyName);
        const CelestialBody* parent = b->orbitDriver->referenceBody;
        CHECK(b->sphereOfInfluence < parent->sphereOfInfluence);
        CHECK(b->sphereOfInfluence < b->orbitDriver->orbit.semiMajorAxis);
        CHECK(b->Mass < parent->Mass);
        CHECK(b->GetParent() == parent);
    }
    // float-derived constants as the game holds them (harness README / NEXT-STEPS "must-know quirks")
    CHECK(kerbin.gravParameter == 3531599999999.9995);
    CHECK(kerbin.rotationPeriod == 21549.425183089825);  // solar 21600 s -> sidereal (CelestialBody.Start)
    CHECK(kerbin.atmosphereAdiabaticIndex == 1.39999997615814);
    CHECK(mun.rotationPeriod == mun.orbit()->period());   // tidally locked (CBUpdate)
    CHECK(kerbin.GetGravParameter() == kerbin.gravParameter);
    CHECK(kerbin.GetOrbitElements()->semiMajorAxis == 13599840256.0);
    CHECK(Planetarium::FrameIsRotating());  // Mun and Minmus rotate with inverseRotation
    double maxGee = -1.0;
    for (const CelestialBody* b : env.sys->Bodies())
        if (b->hasSolidSurface && b->GeeASL > maxGee) maxGee = b->GeeASL;
    CHECK(env.sys->maximumSurfaceGeeASL() == maxGee);
    CHECK(maxGee == env.Body("Eve").GeeASL);
}

TEST_CASE("spawn without a run-time Orbit takes KSP's orbit == null branches") {
    Env env;  // no OrbitFactory (WP-07 not attached)
    CelestialBody& mun = env.Body("Mun");
    CelestialBody& kerbin = env.Body("Kerbin");
    CHECK(mun.orbit() == nullptr);
    CHECK(mun.rotationPeriod == KerbolSystemPrefab().bodies[5].body.rotationPeriod);  // tidal lock skipped
    CHECK(kerbin.solarDayLength == 1.0);
    CHECK(kerbin.sphereOfInfluence > 8.4e7);  // SOI/Hill need only the serialized elements
    CHECK_THROWS_AS(mun.getPositionAtUT(0.0), NullReferenceException);
}

TEST_CASE("atmosphere edge cases") {
    Env env;
    const CelestialBody& kerbin = env.Body("Kerbin");
    const CelestialBody& mun = env.Body("Mun");
    // sea level: the pressure curve's first key (float 101.325)
    CHECK(kerbin.GetPressure(0.0) == static_cast<double>(101.325f));
    CHECK(kerbin.GetTemperature(0.0) == static_cast<double>(288.15f));
    CHECK(kerbin.atmPressureASL == static_cast<double>(101.325f) * 0.009869232667160128);
    CHECK(kerbin.GetDensity(kerbin.GetPressure(0.0), kerbin.GetTemperature(0.0)) == kerbin.atmDensityASL);
    // at and above the atmosphere depth: vacuum
    CHECK(kerbin.GetPressure(70000.0) == 0.0);
    CHECK(kerbin.GetPressure(70000.5) == 0.0);
    CHECK(kerbin.GetTemperature(70000.0) == 0.0);
    CHECK(kerbin.GetPressure(69999.0) > 0.0);
    CHECK(kerbin.GetDensity(0.0, 250.0) == 0.0);
    CHECK(kerbin.GetDensity(1.0, 0.0) == 0.0);
    CHECK(kerbin.GetSpeedOfSound(0.0, 1.0) == 0.0);
    CHECK(kerbin.GetSpeedOfSound(1.0, 0.0) == 0.0);
    // negative altitude: the curves clamp to their first key (Unity ClampForever)
    CHECK(kerbin.GetPressure(-100.0) == kerbin.GetPressure(0.0));
    CHECK(kerbin.GetTemperature(-100.0) == kerbin.GetTemperature(0.0));
    // airless body: no pressure; GetTemperature has no `atmosphere` check (KSP quirk): below 0 m it returns
    // atmosphereTemperatureSeaLevel - lapse * altitude with the serialized values
    CHECK(mun.GetPressure(0.0) == 0.0);
    CHECK(mun.GetPressure(-50.0) == 0.0);
    CHECK(mun.GetTemperature(0.0) == 0.0);
    CHECK(mun.GetTemperature(-1.0) == mun.atmosphereTemperatureSeaLevel - mun.atmosphereTemperatureLapseRate * -1.0);
    // inverseRotThresholdAltitude is raised to the atmosphere depth (SetupConstants)
    CHECK(env.Body("Sun").inverseRotThresholdAltitude == static_cast<float>(env.Body("Sun").atmosphereDepth));  // 1e5 -> 6e5
    CHECK(env.Body("Jool").inverseRotThresholdAltitude == 220000.0f);  // already above its 200 km atmosphere
    // solar attenuation: overhead sun at sea level vs. sun below the horizon
    double am = 0, fm = 0;
    kerbin.GetSolarAtmosphericEffects(1.0, kerbin.atmDensityASL, am, fm);
    CHECK(am == doctest::Approx(1.0));
    CHECK(fm == doctest::Approx(1.0 - PhysicsGlobals::SolarInsolationAtHome()));
    kerbin.GetSolarAtmosphericEffects(-1.0, 0.0, am, fm);
    CHECK(am == std::sqrt(2.0 * kerbin.radiusAtmoFactor + 1.0));
    CHECK(fm == 1.0);

    FlightIntegratorAtmosphere fi;
    fi.currentMainBody = &kerbin;
    fi.altitude = 70001.0;
    fi.CalculatePressure();
    CHECK(fi.staticPressurekPa == 0.0);
    fi.altitude = 70000.0;  // `<=` depth enters the branch, GetPressure (`<` depth) returns 0
    fi.CalculatePressure();
    CHECK(fi.staticPressureAtm == 0.0);
    fi.altitude = 0.0;
    fi.spd = 300.0;
    fi.CalculatePressure();
    fi.CalculateConstantsAtmosphere();
    const double rho1 = fi.solarAirMass;
    fi.CalculatePressure();
    fi.CalculateConstantsAtmosphere();  // solar air mass now uses the previous call's density (KSP quirk)
    CHECK(fi.mach == doctest::Approx(300.0 / fi.speedOfSound));
    CHECK(fi.externalTemperature >= fi.atmosphericTemperature);
    CHECK(fi.dynamicPressurekPa == 0.0005 * fi.density * 300.0 * 300.0);
    CHECK(rho1 > 0.0);
    fi.CalculateConstantsVacuum();
    CHECK(fi.atmosphericTemperature == PhysicsGlobals::SpaceTemperature());
    CHECK(fi.pseudoReDragMult == 1.0);
}

TEST_CASE("surface coordinates round trip") {
    Env env;
    const CelestialBody& kerbin = env.Body("Kerbin");
    const Vector3d n = kerbin.GetRelSurfaceNVector(0.0, 0.0);
    CHECK(n.x == 1.0);
    CHECK(n.y == 0.0);
    const Vector3d w = kerbin.GetWorldSurfacePosition(-0.0972, -74.5577, 75.0);  // KSC
    double lat = 0, lon = 0, alt = 0;
    kerbin.GetLatLonAlt(w, lat, lon, alt);
    CHECK(lat == doctest::Approx(-0.0972).epsilon(1e-9));
    CHECK(lon == doctest::Approx(-74.5577).epsilon(1e-9));
    CHECK(alt == doctest::Approx(75.0).epsilon(1e-6));
    CHECK(kerbin.GetAltitude(w) == doctest::Approx(75.0).epsilon(1e-6));
}

TEST_CASE("PhysicsGlobals: Physics.cfg values, enums, properties, save/load") {
    Env env;
    const PhysicsGlobals& pg = env.pg;
    CHECK(PhysicsGlobals::Instance() == &env.pg);
    CHECK(pg.VesselRangesDefault.flying.unload == 22500.0f);
    CHECK(pg.constructionWeightLimit == static_cast<double>(588.399f));  // double field read with float.Parse (PhysicsGlobals.cs:3330)
    CHECK(pg.constructionWeightLimitPerKerbalCombine == 588.399);        // not in Physics.cfg: prefab double kept
    CHECK(pg.VesselRangesDefault.GetSituationRanges(VesselSituations::DOCKED).load == pg.VesselRangesDefault.orbit.load);
    CHECK(pg.showRigidJointTweakable == ShowRigidAttachmentOption::Editor);
    CHECK(pg.liftingSurfaceCurves.Count() == 4);
    CHECK(PhysicsGlobals::BodyLiftCurve() == PhysicsGlobals::GetLiftingSurfaceCurve("BodyLift"));
    CHECK(PhysicsGlobals::SurfaceCurves.dragCurveTip == &env.pg.dragCurveTip);
    CHECK(PhysicsGlobals::KpaToAtmospheres() == 1.0 / env.Body("Kerbin").atmospherePressureSeaLevel);
    CHECK(PhysicsGlobals::JointForce() == pg.jointForce);
    CHECK(PhysicsGlobals::SolarLuminosity() ==
          dotnet::Math::Pow(env.Body("Kerbin").orbitDriver->orbit.semiMajorAxis, 2.0) * 4.0 * UtilMath::PI * 1360.0);
    CHECK(EnumParse<ShowRigidAttachmentOption>(" Always ") == ShowRigidAttachmentOption::Always);
    CHECK(EnumParse<VesselTargetModes>("1") == VesselTargetModes::Direction);
    CHECK_THROWS(EnumParse<PhysicMaterialCombine>("average"));
    CHECK(std::string(ToString(PhysicMaterialCombine::Maximum)) == "Maximum");

    // SaveDatabase -> LoadDatabase reproduces every loaded double/int/bool/string/enum field
    ConfigNode saved("PHYSICS_DATABASE");
    pg.SaveDatabase(saved);
    CHECK(saved.GetValue("kerbalGOffset").value_or("") == "900");  // typed SetValue(double): G17
    CHECK(saved.HasNode("VesselRanges"));
    CHECK(saved.GetNode("LIFTING_SURFACE_CURVES")->GetNodes("LIFTING_SURFACE").size() == 4);
    PhysicsGlobals copy;
    PhysicsGlobals::SetInstance(&env.pg);  // keep env.pg as the singleton
    copy.LoadDatabase(saved);
    for (const auto& f : PhysicsGlobals::Fields()) {
        if (f.isStatic || !saved.HasValue(f.name)) continue;
        INFO(f.name);
        switch (f.kind) {
            case PhysicsGlobals::FieldKind::Double:
                // 1e-7: constructionWeightLimit* are doubles that LoadDatabase reads with float.Parse (KSP quirk)
                CHECK(*static_cast<const double*>(f.get(copy)) == doctest::Approx(*static_cast<const double*>(f.get(pg))).epsilon(1e-7));
                break;
            case PhysicsGlobals::FieldKind::Float:
                CHECK(*static_cast<const float*>(f.get(copy)) == doctest::Approx(*static_cast<const float*>(f.get(pg))).epsilon(1e-6));
                break;
            case PhysicsGlobals::FieldKind::Int: CHECK(*static_cast<const int32_t*>(f.get(copy)) == *static_cast<const int32_t*>(f.get(pg))); break;
            case PhysicsGlobals::FieldKind::Bool: CHECK(*static_cast<const bool*>(f.get(copy)) == *static_cast<const bool*>(f.get(pg))); break;
            case PhysicsGlobals::FieldKind::String:
                CHECK(*static_cast<const std::string*>(f.get(copy)) == *static_cast<const std::string*>(f.get(pg)));
                break;
            case PhysicsGlobals::FieldKind::Enum: CHECK(PhysicsGlobals::EnumValue(f, copy) == PhysicsGlobals::EnumValue(f, pg)); break;
            default: break;
        }
    }
    // a bad bool aborts the load like C#'s bool.Parse
    ConfigNode bad("PHYSICS_DATABASE");
    bad.AddValue("thermalIntegrationAlwaysRK2", "maybe");
    CHECK_THROWS_AS(copy.LoadDatabase(bad), dotnet::FormatException);
}

TEST_CASE("biome maps, mini-biomes and asset origins come from the game's files") {
    const PSystemPrefab& s = KerbolSystemPrefab();
    const CelestialBody& kerbin = s.bodies[4].body;
    REQUIRE(kerbin.bodyName == "Kerbin");
    REQUIRE(kerbin.BiomeMap.has_value());
    const CBAttributeMapSO& bm = *kerbin.BiomeMap;
    CHECK(bm.mapName == "kerbin_biomes");
    CHECK(bm.width == 4096);
    CHECK(bm.height == 2048);
    CHECK(bm.bpp == 3);
    CHECK(bm.origin.file == "KSP_x64_Data/sharedassets9.assets");
    CHECK(bm.origin.streamSize == 4096LL * 2048 * 3);
    CHECK(bm.Attributes.size() == 11);
    CHECK(bm.Attributes[0].name == "Water");
    CHECK(kerbin.MiniBiomes.size() == 49);
    CHECK_FALSE(s.bodies[0].body.BiomeMap.has_value());  // Sun
    CHECK(CBAttributeMapSO::RGBColorSqrMag(Color{1, 0, 0, 1}, Color{0, 0, 0, 1}) == 1.0f);
    bool heightMap = false, scaledMain = false;
    for (const GameAssetRef& a : kerbin.assets) {
        heightMap |= a.role.find("PQSMod_VertexHeightMap.heightMap") != std::string::npos && a.name == "kerbin_height_new";
        scaledMain |= a.role.find("._MainTex") != std::string::npos && a.type == "Texture2D";
    }
    CHECK(heightMap);
    CHECK(scaledMain);
    // every referenced asset file exists in the dev copy of the game (when present)
    const std::filesystem::path game = std::filesystem::path(dotnet::Utf8ToWide(kProjectRoot)) / "decompiled" / "KSP-dev";
    if (std::filesystem::exists(game)) {
        for (const auto& p : s.bodies) {
            for (const GameAssetRef& a : p.body.assets) {
                INFO(p.body.bodyName << " " << a.role << " " << a.file);
                CHECK(std::filesystem::exists(game / a.file));
                if (!a.stream.empty()) CHECK(std::filesystem::exists(game / a.stream));
            }
        }
    }
}
