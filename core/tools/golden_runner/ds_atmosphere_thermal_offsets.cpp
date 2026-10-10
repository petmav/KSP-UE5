// atmosphere_thermal_offsets candidate (WP-22, CAPTURE-SPEC C4): CelestialBody.GetAtmoThermalStats / GetFullTemperature /
// GetPressure / GetDensity / GetSolarAtmosphericEffects of kspcore against the in-game capture
// harness/captures/out2/atmosphere_thermal_offsets.json.
//
// Inputs rebuilt from the capture's own input columns (never from the output columns): per epoch the sun's scaled position and
// ScaledSpace.ScaleFactor (`epochs`), per body and epoch the engine transform state (`body_epoch`: bodyTransform.up,
// scaledBody.transform.position, position, rotationPeriod, solarDayLength, and the run-time orbit values GetAtmoThermalStats
// reads), per grid point the engine-built `pos`, `scaledPos`, `sunVector`, `upAxis`, `sunDot`, `lat`, `lon`, `alt`.
// Outputs recomputed: atmosphereTemperatureOffset, bodyEmissiveFlux, bodyAlbedoFlux, bodyTemperature, GetTemperature,
// GetFullTemperature (alt, offset) and (pos), pressure_kPa, density, solarAirMass, solarFluxMultiplier.
// The body prefab data (curves, emissivity, albedo, atmosphere) comes from the game's level data through CelestialEnv.
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "celestial_env.hpp"
#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/orbit/orbit.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using golden::ColType;

namespace {

// The run-time Orbit values of one body at one epoch, as the capture recorded them (ref_* are those of the body referencing the sun).
struct CapturedOrbit final : ICelestialBodyOrbit {
    double ta = 0, rad = 0, pe = 0, ap = 0, ecc = 0, incl = 0;
    Orbit* GetOrbit() override { return nullptr; }
    double period() const override { return 0.0; }
    double trueAnomaly() const override { return ta; }
    double radius() const override { return rad; }
    double PeR() const override { return pe; }
    double ApR() const override { return ap; }
    double eccentricity() const override { return ecc; }
    double inclination() const override { return incl; }
    Vector3d vel() const override { return Vector3d::zero(); }
    Vector3d getRelativePositionAtUT(double) const override { return Vector3d::zero(); }
    Vector3d getOrbitalVelocityAtUT(double) const override { return Vector3d::zero(); }
    void UpdateOrbit() override {}
};

struct TableView {
    const nlohmann::json* t;
    std::map<std::string, size_t> idx;
    explicit TableView(const nlohmann::json& table) : t(&table) {
        size_t i = 0;
        for (const auto& c : table.at("columns")) idx[c.at("name").get<std::string>()] = i++;
    }
    const nlohmann::json& rows() const { return t->at("rows"); }
    double D(const nlohmann::json& r, const char* c) const { return r.at(idx.at(c)).get<double>(); }
    float F(const nlohmann::json& r, const char* c) const { return static_cast<float>(r.at(idx.at(c)).get<double>()); }
    std::string S(const nlohmann::json& r, const char* c) const { return r.at(idx.at(c)).get<std::string>(); }
    int64_t I(const nlohmann::json& r, const char* c) const { return r.at(idx.at(c)).get<int64_t>(); }
};

int RunAtmosphereThermalOffsets(const RunnerOptions& opt) {
    CelestialEnv& env = CelestialEnv::Get(opt);
    nlohmann::json cap;
    {
        std::ifstream f(dotnet::PathFromUtf8(opt.harness + "/captures/out2/atmosphere_thermal_offsets.json"), std::ios::binary);
        if (!f) throw std::runtime_error("atmosphere_thermal_offsets: capture not found under " + opt.harness);
        cap = nlohmann::json::parse(f);
    }
    auto findTable = [&](const char* n) -> const nlohmann::json& {
        for (const auto& t : cap.at("tables"))
            if (t.at("name") == n) return t;
        throw std::runtime_error(std::string("atmosphere_thermal_offsets: no table ") + n);
    };
    const TableView epochs(findTable("epochs")), bodyEpoch(findTable("body_epoch")), samples(findTable("samples"));

    CelestialBody* sun = nullptr;
    for (CelestialBody* b : env.system->SpawnOrder())
        if (!b->orbitDriver) sun = b;
    if (sun == nullptr) throw std::runtime_error("atmosphere_thermal_offsets: no sun");

    golden::File g;
    g.dataset = "atmosphere_thermal_offsets";
    g.description = "kspcore candidate for atmosphere_thermal_offsets (WP-22).";
    golden::Table ts("samples", "Inputs and outputs per grid point.");
    ts.Col("epoch", ColType::I64).Col("body", ColType::Str);
    for (const char* c : {"lat", "lon", "alt", "pos_x", "pos_y", "pos_z", "scaledPos_x", "scaledPos_y", "scaledPos_z", "sunVector_x",
                          "sunVector_y", "sunVector_z", "upAxis_x", "upAxis_y", "upAxis_z", "sunDot", "atmosphereTemperatureOffset",
                          "bodyEmissiveFlux", "bodyAlbedoFlux", "bodyTemperature", "GetTemperature", "GetFullTemperature_alt_offset",
                          "GetFullTemperature_pos", "pressure_kPa", "density", "solarAirMass", "solarFluxMultiplier"})
        ts.Col(c, ColType::F64);
    ts.key = {"epoch", "body", "lat", "lon", "alt"};

    std::map<std::string, std::unique_ptr<CapturedOrbit>> stubs;
    std::map<std::string, ICelestialBodyOrbit*> savedLinks;

    std::map<std::pair<int64_t, std::string>, const nlohmann::json*> beRows;
    for (const auto& r : bodyEpoch.rows()) beRows[{bodyEpoch.I(r, "epoch"), bodyEpoch.S(r, "body")}] = &r;
    std::map<int64_t, const nlohmann::json*> epRows;
    for (const auto& r : epochs.rows()) epRows[epochs.I(r, "epoch")] = &r;

    auto restoreLinks = [&]() {
        for (auto& [name, link] : savedLinks) env.system->GetBodyByName(name)->orbitDriver->link = link;
        savedLinks.clear();
        stubs.clear();
    };
    int64_t curEpoch = -1;
    auto installEpoch = [&](int64_t e) {
        restoreLinks();
        for (const auto& [key, rp] : beRows) {
            if (key.first != e) continue;
            CelestialBody* b = env.system->GetBodyByName(key.second);
            const nlohmann::json& r = *rp;
            auto s = std::make_unique<CapturedOrbit>();
            if (bodyEpoch.S(r, "ref_body") == key.second) {
                s->ta = bodyEpoch.D(r, "ref_trueAnomaly");
                s->rad = bodyEpoch.D(r, "ref_radius");
                s->pe = bodyEpoch.D(r, "ref_PeR");
                s->ap = bodyEpoch.D(r, "ref_ApR");
                s->ecc = bodyEpoch.D(r, "ref_eccentricity");
            }
            s->incl = bodyEpoch.D(r, "orbit_inclination");
            savedLinks[key.second] = b->orbitDriver->link;
            b->orbitDriver->link = s.get();
            stubs[key.second] = std::move(s);
            b->rotationPeriod = bodyEpoch.D(r, "rotationPeriod");
            b->solarDayLength = bodyEpoch.D(r, "solarDayLength");
            b->rotationAngle = bodyEpoch.D(r, "rotationAngle");
            b->position = Vector3d(bodyEpoch.D(r, "position_x"), bodyEpoch.D(r, "position_y"), bodyEpoch.D(r, "position_z"));
        }
        curEpoch = e;
    };

    for (const auto& r : samples.rows()) {
        const int64_t e = samples.I(r, "epoch");
        if (e != curEpoch) installEpoch(e);
        const std::string bn = samples.S(r, "body");
        CelestialBody* b = env.system->GetBodyByName(bn);
        const nlohmann::json& er = *epRows.at(e);
        const nlohmann::json& br = *beRows.at({e, bn});

        CelestialBody::AtmoThermalFrame frame;
        frame.sunScaledPos = Vector3f{epochs.F(er, "sun_scaled_pos_x"), epochs.F(er, "sun_scaled_pos_y"), epochs.F(er, "sun_scaled_pos_z")};
        frame.scaleFactor = epochs.F(er, "ScaledSpace_ScaleFactor");
        frame.bodyUp = Vector3f{bodyEpoch.F(br, "bodyTransform_up_x"), bodyEpoch.F(br, "bodyTransform_up_y"), bodyEpoch.F(br, "bodyTransform_up_z")};
        frame.scaledBodyPos = Vector3f{bodyEpoch.F(br, "scaledBody_pos_x"), bodyEpoch.F(br, "scaledBody_pos_y"), bodyEpoch.F(br, "scaledBody_pos_z")};

        const double alt = samples.D(r, "alt");
        const Vector3d pos(samples.D(r, "pos_x"), samples.D(r, "pos_y"), samples.D(r, "pos_z"));
        const Vector3d scaledPos(samples.D(r, "scaledPos_x"), samples.D(r, "scaledPos_y"), samples.D(r, "scaledPos_z"));
        const Vector3d sunVector(samples.D(r, "sunVector_x"), samples.D(r, "sunVector_y"), samples.D(r, "sunVector_z"));
        const Vector3d upAxis(samples.D(r, "upAxis_x"), samples.D(r, "upAxis_y"), samples.D(r, "upAxis_z"));
        const double sunDot = samples.D(r, "sunDot");

        double off = 0, emis = 0, alb = 0;
        b->GetAtmoThermalStats(true, sun, sunVector, sunDot, upAxis, alt, frame, off, emis, alb);
        const double bodyTemp = b->bodyTemperature;
        const double temp = b->GetTemperature(alt);
        const double full = b->GetFullTemperature(alt, off);
        // GetFullTemperature(Vector3d pos): sun.scaledBody.transform.position - LocalToScaledSpace(pos); FlightGlobals.getAltitudeAtPos
        const Vector3d sunMinusScaled = Vector3d(frame.sunScaledPos) - scaledPos;
        const double altAtPos = Vector3d::Distance(pos, b->position) - b->Radius;
        const double fullPos = b->GetFullTemperature(sun, altAtPos, sunMinusScaled, upAxis, frame);
        const double pres = b->GetPressure(alt);
        const double dens = b->GetDensity(pres, full);
        double airMass = 0, fluxMult = 0;
        b->GetSolarAtmosphericEffects(sunDot, dens, airMass, fluxMult);
        ts.Add({e, bn, samples.D(r, "lat"), samples.D(r, "lon"), alt, pos.x, pos.y, pos.z, scaledPos.x, scaledPos.y, scaledPos.z,
                sunVector.x, sunVector.y, sunVector.z, upAxis.x, upAxis.y, upAxis.z, sunDot, off, emis, alb, bodyTemp, temp, full,
                fullPos, pres, dens, airMass, fluxMult});
    }
    restoreLinks();
    g.tables.push_back(std::move(ts));
    return WriteCandidate(opt, g, "atmosphere_thermal_offsets/atmosphere_thermal_offsets.json");
}

}  // namespace

KSP_GOLDEN_DATASET("atmosphere_thermal_offsets", 80, RunAtmosphereThermalOffsets);
