// thermal_step candidate (WP-22, CAPTURE-SPEC C6): FlightIntegrator's thermal model of kspcore against the in-game capture
// harness/captures/out2/thermal_step.json (PhysProf-S: pad 500 frames, orbit 1000, reentry 1500; 5 of the 21 parts logged).
//
// What the capture can and cannot feed (only 5 of 21 parts, no rigid body, no part transforms):
//
// vessel   per frame: the FlightIntegratorAtmosphere (WP-09) of the frame is recomputed from altitude, spd, sunDot,
//          atmosphereTemperatureOffset (INPUTS: CalculateSunBodyFlux / GetAtmoThermalStats, the C4 gate) and the previous frame's
//          density (KSP reads it for the solar air mass): pressure, density, dynamic pressure, atmospheric / external temperature,
//          Mach, convective coefficient and Mach lerp, density thermal lerp, solar flux multiplier, background radiation
//          temperatures. `solarFlux` is not written: it is solarFlux_before_attenuation * multiplier and the unattenuated value
//          needs the sun-ray cast distance (engine input).
//
// parts    one FlightIntegrator.UpdateThermodynamics step per tracked part and frame (the real function, on a one-part graph):
//          pre-state = the previous frame's row (temperatures, flux accumulators, exposed flag, stale ptd values), frame inputs =
//          the vessel row, this frame's radiativeArea and the previous frame's exposedArea (UpdateAerodynamics sets it after the
//          thermal step), part constants = the part's prefab. OUTPUTS compared: SetSkinProperties (skinExposedAreaFrac,
//          skinExposedArea, convectionArea, exposed), PrecalcConvection (postShockExtTemp, finalCoeff), UpdateConvection,
//          PrecalcRadiation scalars (emission / absorption, background temperatures, sunFlux, bodyFlux), UpdateRadiation, the
//          skin-local conduction (skin-skin, unexposed-skin-internal, skin-internal), and the integrated temperatures.
//          INPUTS recovered from the capture (engine geometry or neighbours that are not in the file):
//            * ptd_intConductionFlux, ptd_skinConductionFlux: conduction to / from the unlogged neighbours (UpdateConduction's
//              neighbour loops read their temperatures);
//            * the occlusion multipliers (convectionAreaMultiplier / convectionTempMultiplier, UpdateOcclusionConvection): the
//              value m that reproduces the logged convectionArea / postShockExtTemp bit for bit (1.0 when it does);
//            * the sun and body flux entering each skin (geometry: cube area along the sun / up direction, direction cosines to
//              the velocity): the exact double I with I - emission == the logged expFlux / unexpFlux (searched +-4 ulp around
//              logged + emission), so that the emission and everything after it are verified.
//          Recovery is not verification: a nearby double that reproduces the logged value always exists, so recoveryMisses (0) is a
//          sanity check only. The seven columns recovered from their own logged values (ptd_convectionArea, ptd_postShockExtTemp,
//          ptd_brtExposed, ptd_expFlux, ptd_unexpFlux, ptd_sunFlux, ptd_bodyFlux) are inputs: tests/CMakeLists.txt drops them from
//          the comparison.
//          Row frame 0 of each case has no previous row and is not written.
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <string>
#include <vector>

#include "celestial_env.hpp"
#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/thermal/flight_integrator_thermal.hpp"
#include "nlohmann/json.hpp"
#include "vessel_capture.hpp"

using namespace ksp;
using golden::ColType;
namespace M = dotnet::Math;

namespace {

struct TableView {
    const nlohmann::json* t;
    std::map<std::string, size_t> idx;
    explicit TableView(const nlohmann::json& table) : t(&table) {
        size_t i = 0;
        for (const auto& c : table.at("columns")) idx[c.at("name").get<std::string>()] = i++;
    }
    const nlohmann::json& rows() const { return t->at("rows"); }
    double D(const nlohmann::json& r, const char* c) const { return r.at(idx.at(c)).get<double>(); }
    std::string S(const nlohmann::json& r, const char* c) const { return r.at(idx.at(c)).get<std::string>(); }
    int64_t I(const nlohmann::json& r, const char* c) const { return r.at(idx.at(c)).get<int64_t>(); }
    bool B(const nlohmann::json& r, const char* c) const { return r.at(idx.at(c)).get<bool>(); }
};

double NextUp(double x) { return std::nextafter(x, std::numeric_limits<double>::infinity()); }
double NextDown(double x) { return std::nextafter(x, -std::numeric_limits<double>::infinity()); }

// FlightIntegrator.altitude (the CoM's altitude) is not in the capture (`altitude` is Vessel.altitude of the root part): the pressure curve
// is monotone, so it is the lowest altitude whose GetPressure equals the captured static pressure bit for bit (as WP-14's vessel_state).
bool RecoverPressureAltitude(const CelestialBody& body, double P, double& altitude) {
    double lo = 0.0, hi = body.atmosphereDepth;
    if (body.GetPressure(lo) < P) return false;
    for (int i = 0; i < 200; i++) {
        const double mid = 0.5 * (lo + hi);
        if (body.GetPressure(mid) > P) lo = mid;
        else hi = mid;
    }
    altitude = hi;
    return body.GetPressure(hi) == P;
}

// the host of the C6 gate: neighbour conduction and the radiation geometry come from the capture
struct GateFI final : FlightIntegratorThermal {
    double capIntCond = 0.0, capSkinCond = 0.0;
    double incomingExp = 0.0, incomingUnexp = 0.0;
    double loggedSunFlux = 0.0, loggedBodyFlux = 0.0;

    // UpdateConduction with the neighbour terms taken from the capture; the skin-local block is the real function
    void UpdateConduction() override {
        for (int i = 0; i < partThermalDataCount; i++) {
            PartThermalData& p = *partThermalDataList[i];
            p.unexpSkinInternalConductionFlux = 0.0;
            p.skinInteralConductionFlux = 0.0;
            p.skinSkinConductionFlux = 0.0;
            p.localIntConduction = 0.0;
            p.intConductionFlux = 0.0;
            p.localSkinConduction = 0.0;
            p.skinConductionFlux = 0.0;
            p.unifiedTemp = p.GetUnifiedSkinTemp();
        }
        for (int i = 0; i < partThermalDataCount; i++)
            UpdateSkinLocalConduction(*partThermalDataListSkin[i], PhysicsGlobals::SkinInternalConductionFactor());
        PartThermalData& p = *partThermalDataList[0];
        p.intConductionFlux = capIntCond;
        p.skinConductionFlux = capSkinCond;
    }

    // PrecalcRadiation's tail (FlightIntegrator.cs:2138-2180) with the sun / body geometry recovered
    void PrecalcRadiationIncoming(PartThermalData& ptd, double) override {
        if (ptd.part->ShieldedFromAirstream()) return;
        if (loggedSunFlux != 0.0) ptd.sunFlux = ptd.absorbScalar * atm.solarFlux;  // sunArea > 0 and directSunlight
        const double rawBody = ptd.bodyFlux;
        if (rawBody > 0.0 && loggedBodyFlux != rawBody)  // bodyArea > 0: bodyFlux = Lerp(0, bodyFlux, densityThermalLerp) * absorbScalar
            ptd.bodyFlux = UtilMath::Lerp(0.0, ptd.bodyFlux, atm.densityThermalLerp) * ptd.absorbScalar;
        ptd.expFlux = incomingExp;
        ptd.unexpFlux = incomingUnexp;
    }
};

int RunThermalStep(const RunnerOptions& opt) {
    CelestialEnv& env = CelestialEnv::Get(opt);
    nlohmann::json cap;
    {
        std::ifstream f(dotnet::PathFromUtf8(opt.harness + "/captures/out2/thermal_step.json"), std::ios::binary);
        if (!f) throw std::runtime_error("thermal_step: capture not found under " + opt.harness);
        cap = nlohmann::json::parse(f);
    }
    auto findTable = [&](const char* n) -> const nlohmann::json& {
        for (const auto& t : cap.at("tables"))
            if (t.at("name") == n) return t;
        throw std::runtime_error(std::string("thermal_step: no table ") + n);
    };
    const TableView vtab(findTable("vessel")), ptab(findTable("parts")), ctab(findTable("part_constants"));

    VesselCapture vc(opt);  // the PhysProf-S part tree and the part prefab data (GameData mirror)
    Vessel* vessel = vc.vessels.at("PhysProf-S").get();
    CelestialBody* kerbin = vc.kerbin;
    CelestialBody* sunBody = nullptr;
    for (CelestialBody* b : env.system->SpawnOrder())
        if (!b->orbitDriver) sunBody = b;

    golden::File g;
    g.dataset = "thermal_step";
    g.description = "kspcore candidate for thermal_step (WP-22).";

    // ---------------------------------------------------------------- vessel
    golden::Table tv("vessel", "FlightIntegrator atmosphere / thermal scalars per frame.");
    tv.Col("case", ColType::Str).Col("frame", ColType::I64);
    const std::vector<const char*> vcols = {"mach", "density", "staticPressurekPa", "dynamicPressurekPa", "atmosphericTemperature",
                                            "externalTemperature", "convectiveCoefficient", "convectiveMachLerp", "densityThermalLerp",
                                            "solarFluxMultiplier", "backgroundRadiationTemp", "backgroundRadiationTempExposed"};
    for (const char* c : vcols) tv.Col(c, ColType::F64);
    tv.Col("fi_isAnalytical", ColType::Bool);
    tv.key = {"case", "frame"};
    std::map<std::pair<std::string, int64_t>, const nlohmann::json*> vrows;
    for (const auto& r : vtab.rows()) vrows[{vtab.S(r, "case"), vtab.I(r, "frame")}] = &r;
    {
        FlightIntegratorThermal fi;
        fi.vessel = vessel;
        fi.sunBody = sunBody;
        fi.CacheGlobals();
        double prevDensity = 0.0, prevUT = 0.0;
        std::string curCase;
        for (const auto& r : vtab.rows()) {
            const std::string cs = vtab.S(r, "case");
            const int64_t frame = vtab.I(r, "frame");
            const bool first = (cs != curCase);
            curCase = cs;
            FlightIntegratorAtmosphere& a = fi.atm;
            a.currentMainBody = kerbin;
            a.situation = VesselSituations::FLYING;
            a.altitude = vtab.D(r, "altitude");
            if (vtab.D(r, "staticPressurekPa") > 0.0) {
                double pAlt = a.altitude;
                if (RecoverPressureAltitude(*kerbin, vtab.D(r, "staticPressurekPa"), pAlt)) a.altitude = pAlt;
                else std::fprintf(stderr, "thermal_step: no altitude reproduces the static pressure of %s frame %lld\n", cs.c_str(), static_cast<long long>(frame));
            }
            a.spd = vtab.D(r, "spd");
            a.sunDot = vtab.D(r, "sunDot");
            a.atmosphereTemperatureOffset = vtab.D(r, "atmosphereTemperatureOffset");
            a.solarFlux = 0.0;
            a.density = first ? vtab.D(r, "density") : prevDensity;  // KSP's previous-frame density (frame 0: no previous frame)
            a.CalculatePressure();
            if (a.staticPressurekPa > 0.0)
                a.CalculateConstantsAtmosphere();
            else
                a.CalculateConstantsVacuum();
            a.densityThermalLerp = a.CalculateDensityThermalLerp();
            const double brt = a.CalculateBackgroundRadiationTemperature(a.atmosphericTemperature);
            const double brtE = a.CalculateBackgroundRadiationTemperature(a.externalTemperature);
            // UpdateThermodynamics' isAnalytical test: lastUT = the previous frame's UT (frame 0: the capture's flag, no history)
            bool analytical = vtab.B(r, "fi_isAnalytical");
            if (!first) {
                const double ut = vtab.D(r, "UT");
                const double tslu = ut - prevUT;
                analytical = static_cast<double>(1.0f) > PhysicsGlobals::ThermalMaxIntegrationWarp() ||
                             tslu > PhysicsGlobals::ThermalMaxIntegrationWarp() * 0.04;
            }
            tv.Add({cs, frame, a.mach, a.density, a.staticPressurekPa, a.dynamicPressurekPa, a.atmosphericTemperature,
                    a.externalTemperature, a.convectiveCoefficient, a.convectiveMachLerp, a.densityThermalLerp, a.solarFluxMultiplier,
                    brt, brtE, analytical});
            prevDensity = a.density;
            prevUT = vtab.D(r, "UT");
        }
    }
    g.tables.push_back(std::move(tv));

    // ---------------------------------------------------------------- parts
    golden::Table tp("parts", "One thermal step per tracked part and frame.");
    tp.Col("case", ColType::Str).Col("frame", ColType::I64).Col("part", ColType::Str);
    const std::vector<const char*> pcols = {
        "temperature", "skinTemperature", "skinUnexposedTemperature", "thermalInternalFlux", "thermalConductionFlux",
        "thermalConvectionFlux", "thermalRadiationFlux", "thermalSkinFlux", "thermalExposedFlux", "skinExposedAreaFrac",
        "skinExposedArea", "skinThermalMass", "ptd_convectionArea", "ptd_convectionFlux", "ptd_radiationFlux",
        "ptd_unexpRadiationFlux", "ptd_sunFlux", "ptd_bodyFlux", "ptd_expFlux", "ptd_unexpFlux", "ptd_postShockExtTemp",
        "ptd_finalCoeff", "ptd_skinSkinConductionFlux", "ptd_skinInteralConductionFlux", "ptd_brtExposed", "ptd_brtUnexposed"};
    for (const char* c : pcols) tp.Col(c, ColType::F64);
    tp.Col("ptd_exposed", ColType::Bool);
    tp.key = {"case", "frame", "part"};

    // the tracked parts: first part of the craft with that name (the capture tracks the first of each)
    std::map<std::string, Part*> trackedParts;
    for (Part* p : vessel->parts) {
        Part::StartContext ctx;  // Part.Start(): absorptiveConstant := emissiveConstant when unset, skinMaxTemp, thermal mass block
        ctx.flight = true;
        ctx.standardSpecificHeatCapacity = PhysicsGlobals::StandardSpecificHeatCapacity();
        p->StartData(ctx);
        if (trackedParts.find(p->partInfo->name) == trackedParts.end()) trackedParts[p->partInfo->name] = p;
    }

    std::map<std::tuple<std::string, std::string, int64_t>, const nlohmann::json*> prow;
    for (const auto& r : ptab.rows()) prow[{ptab.S(r, "case"), ptab.S(r, "part"), ptab.I(r, "frame")}] = &r;
    std::map<std::pair<std::string, std::string>, const nlohmann::json*> crow;
    for (const auto& r : ctab.rows()) crow[{ctab.S(r, "case"), ctab.S(r, "part")}] = &r;

    GateFI fi;
    fi.vessel = vessel;
    fi.sunBody = sunBody;
    fi.CacheGlobals();
    fi.partCount = 1;
    fi.recreateThermalGraph = false;
    std::map<std::string, std::unique_ptr<PartThermalData>> ptds;
    int recoveryMisses = 0, tracked = 0;
    for (const auto& r : ptab.rows()) {
        const std::string cs = ptab.S(r, "case"), pn = ptab.S(r, "part");
        const int64_t frame = ptab.I(r, "frame");
        if (frame == 0) continue;
        const auto pit = prow.find({cs, pn, frame - 1});
        if (pit == prow.end()) continue;
        const nlohmann::json& pr = *pit->second;
        const nlohmann::json& vr = *vrows.at({cs, frame});
        const nlohmann::json& vprev = *vrows.at({cs, frame - 1});
        const std::string base = pn.substr(0, pn.find('#'));
        Part& part = *trackedParts.at(base);
        auto& up = ptds[pn];
        if (!up) up = std::make_unique<PartThermalData>(part);
        PartThermalData& ptd = *up;
        ++tracked;

        // ---- frame inputs: atmosphere / thermal scalars of the vessel row
        FlightIntegratorAtmosphere& a = fi.atm;
        a.currentMainBody = kerbin;
        a.mach = vtab.D(vr, "mach");
        a.density = vtab.D(vr, "density");
        a.atmosphericTemperature = vtab.D(vr, "atmosphericTemperature");
        a.externalTemperature = vtab.D(vr, "externalTemperature");
        a.convectiveCoefficient = vtab.D(vr, "convectiveCoefficient");
        a.densityThermalLerp = vtab.D(vr, "densityThermalLerp");
        a.solarFlux = vtab.D(vr, "solarFlux");
        a.pseudoReynolds = a.density * vtab.D(vr, "spd");
        a.pseudoReLerpTimeMult = 1.0 / (PhysicsGlobals::TurbulentConvectionEnd() - PhysicsGlobals::TurbulentConvectionStart());
        fi.bodyEmissiveFlux = vtab.D(vr, "bodyEmissiveFlux");
        fi.bodyAlbedoFlux = vtab.D(vr, "bodyAlbedoFlux");
        fi.backgroundRadiationTemp = vtab.D(vr, "backgroundRadiationTemp");
        fi.backgroundRadiationTempExposed = vtab.D(vr, "backgroundRadiationTempExposed");
        vessel->directSunlight = vtab.B(vr, "directSunlight");
        fi.fDeltaTime = vtab.D(vr, "fi_fDeltaTime");
        fi.fTimeSinceThermo = 0.0;
        fi.universalTime = vtab.D(vr, "UT");
        vessel->lastUT = vtab.D(vprev, "UT");
        fi.timeWarpRate = 1.0f;

        // ---- the part: pre-state = previous row; constants = prefab; areas of this step
        PartFields& f = part.fields;
        f.temperature = ptab.D(pr, "temperature");
        f.skinTemperature = ptab.D(pr, "skinTemperature");
        f.skinUnexposedTemperature = ptab.D(pr, "skinUnexposedTemperature");
        f.thermalInternalFlux = ptab.D(pr, "thermalInternalFlux");
        f.thermalSkinFlux = ptab.D(pr, "thermalSkinFlux");
        f.thermalExposedFlux = ptab.D(pr, "thermalExposedFlux");
        f.skinExposedAreaFrac = ptab.D(pr, "skinExposedAreaFrac");
        f.skinExposedArea = ptab.D(pr, "skinExposedArea");
        f.radiativeArea = ptab.D(r, "radiativeArea");  // DragCubeSetupAndPartAeroStats of this frame (before the thermal step)
        f.exposedArea = ptab.D(pr, "exposedArea");     // UpdateAerodynamics of the previous frame (after its thermal step)
        f.atmDensity = a.density;
        f.thermalMass = ptab.D(r, "thermalMass");  // UpdateMassStats: max(total - skin, 0.1) (INPUT: includes the resource thermal mass)
        f.thermalMassReciprocal = 1.0 / f.thermalMass;
        // UpdateMassStats runs at the start of the frame with the previous frame's radiativeArea
        f.radiativeArea = ptab.D(pr, "radiativeArea");
        fi.SetSkinThermalMass(part);
        f.radiativeArea = ptab.D(r, "radiativeArea");
        f.skinExposedMassMult = 1.0;
        f.skinUnexposedMassMult = 0.0;
        ptd.exposed = ptab.B(pr, "ptd_exposed");
        ptd.postShockExtTemp = ptab.D(pr, "ptd_postShockExtTemp");  // stale values survive frames without convection
        ptd.finalCoeff = ptab.D(pr, "ptd_finalCoeff");
        ptd.convectionArea = ptab.D(pr, "ptd_convectionArea");
        ptd.convectionAreaMultiplier = 1.0;
        ptd.convectionTempMultiplier = 1.0;
        ptd.convectionCoeffMultiplier = 1.0;

        // ---- recovered inputs: occlusion multipliers
        {
            const double num = M::Max(f.radiativeArea, 0.001);
            const double lerp = UtilMath::Lerp(num, f.exposedArea,
                                               0.0 - fi.cacheFullConvectionAreaMin +
                                                   (a.mach - fi.cacheFullToCrossSectionLerpStart) /
                                                       (fi.cacheFullToCrossSectionLerpEnd - fi.cacheFullToCrossSectionLerpStart));
            const double target = ptab.D(r, "ptd_convectionArea");
            if (f.atmDensity > 0.0) {
                double m = 1.0;
                if (lerp * m != target) {
                    m = (target == 0.0) ? 0.0 : target / lerp;
                    bool ok = (lerp * m == target);
                    double lo = m, hi = m;
                    for (int k = 0; k < 8 && !ok; k++) {
                        lo = NextDown(lo);
                        hi = NextUp(hi);
                        if (lerp * lo == target) { m = lo; ok = true; }
                        else if (lerp * hi == target) { m = hi; ok = true; }
                    }
                    if (!ok) recoveryMisses++;
                }
                ptd.convectionAreaMultiplier = m;
                const double post = ptab.D(r, "ptd_postShockExtTemp");
                if (target > 0.0) {
                    // convectionTempMultiplier: reproduces postShockExtTemp and brtExposed (both read it) bit for bit; 1.0 when it does
                    const double brtExpLogged = ptab.D(r, "ptd_brtExposed");
                    auto fits = [&](double tm) {
                        return UtilMath::LerpUnclamped(a.atmosphericTemperature, a.externalTemperature, tm) == post &&
                               UtilMath::Lerp(fi.backgroundRadiationTemp, fi.backgroundRadiationTempExposed, tm) == brtExpLogged;
                    };
                    double tm = 1.0;
                    if (!fits(tm)) {
                        const double g0 = (post - a.atmosphericTemperature) / (a.externalTemperature - a.atmosphericTemperature);
                        bool ok = fits(g0);
                        double lo = g0, hi = g0;
                        tm = g0;
                        for (int k = 0; k < 256 && !ok; k++) {
                            lo = NextDown(lo);
                            hi = NextUp(hi);
                            if (fits(lo)) { tm = lo; ok = true; }
                            else if (fits(hi)) { tm = hi; ok = true; }
                        }
                        if (!ok) recoveryMisses++;
                    }
                    ptd.convectionTempMultiplier = tm;
                }
            }
        }

        // ---- recovered inputs: neighbour conduction
        fi.capIntCond = ptab.D(r, "ptd_intConductionFlux");
        fi.capSkinCond = ptab.D(r, "ptd_skinConductionFlux");
        fi.loggedSunFlux = ptab.D(r, "ptd_sunFlux");
        fi.loggedBodyFlux = ptab.D(r, "ptd_bodyFlux");

        // ---- recovered inputs: incoming sun / body radiation (exposed, unexposed skin), see the file header
        {
            // the emission terms of UpdateRadiation, from the pre-state temperatures (the conduction block below does not touch them)
            const double frac = ptab.D(r, "skinExposedAreaFrac");
            const double rad = f.radiativeArea;
            const double areaExp = rad * frac;
            const double areaUnexp = rad * (1.0 - frac);
            const double emiss = (f.emissiveConstant * (fi.cacheRadiationFactor * 0.001));
            auto emission = [&](double T, double brt, double area, bool& used) {
                used = false;
                if (!(T > 0.0 && area > 0.0)) return 0.0;
                used = true;
                double b = brt;
                b *= b;
                b *= b;
                double t = T;
                t *= t;
                t *= t;
                return (t - b) * fi.cacheStefanBoltzmanConstant * emiss * area;
            };
            // temperatures at the moment UpdateRadiation reads them: after SetSkinProperties, before integration
            // -> run the real SetSkinProperties on a scratch copy of the pre-state to get them
            const double saveT = f.skinTemperature, saveU = f.skinUnexposedTemperature, saveFrac = f.skinExposedAreaFrac,
                         saveArea = f.skinExposedArea, saveMe = f.skinExposedMassMult, saveMu = f.skinUnexposedMassMult;
            const bool saveExposed = ptd.exposed;
            const double saveConv = ptd.convectionArea;
            fi.SetSkinProperties(ptd);
            const double skinT = f.skinTemperature, skinU = f.skinUnexposedTemperature;
            const double brtExposed = UtilMath::Lerp(fi.backgroundRadiationTemp, fi.backgroundRadiationTempExposed, ptd.convectionTempMultiplier);
            const double brtUnexposed = fi.backgroundRadiationTemp;
            const double areaExp2 = f.radiativeArea * f.skinExposedAreaFrac;
            const double areaUnexp2 = f.radiativeArea * (1.0 - f.skinExposedAreaFrac);
            (void)areaExp;
            (void)areaUnexp;
            bool usedE = false, usedU = false;
            const double xe = emission(skinT, brtExposed, areaExp2, usedE);
            const double xu = emission(skinU, brtUnexposed, areaUnexp2, usedU);
            f.skinTemperature = saveT;
            f.skinUnexposedTemperature = saveU;
            f.skinExposedAreaFrac = saveFrac;
            f.skinExposedArea = saveArea;
            f.skinExposedMassMult = saveMe;
            f.skinUnexposedMassMult = saveMu;
            ptd.exposed = saveExposed;
            ptd.convectionArea = saveConv;
            auto recover = [&](double L, double X, bool used) {
                if (!used) return L;
                double I = L + X;
                if (I - X == L) return I;
                double lo = I, hi = I;
                for (int k = 0; k < 4; k++) {
                    lo = NextDown(lo);
                    hi = NextUp(hi);
                    if (lo - X == L) return lo;
                    if (hi - X == L) return hi;
                }
                recoveryMisses++;
                return I;
            };
            fi.incomingExp = recover(ptab.D(r, "ptd_expFlux"), xe, usedE);
            fi.incomingUnexp = recover(ptab.D(r, "ptd_unexpFlux"), xu, usedU);
        }

        // ---- the step
        fi.partThermalDataList.assign(1, &ptd);
        fi.partThermalDataListSkin.assign(1, &ptd);
        fi.partThermalDataCount = 1;
        fi.UpdateThermodynamics();

        tp.Add({cs, frame, pn, f.temperature, f.skinTemperature, f.skinUnexposedTemperature, f.thermalInternalFlux,
                f.thermalConductionFlux, f.thermalConvectionFlux, f.thermalRadiationFlux, f.thermalSkinFlux, f.thermalExposedFlux,
                f.skinExposedAreaFrac, f.skinExposedArea, f.skinThermalMass, ptd.convectionArea, ptd.convectionFlux,
                ptd.radiationFlux, ptd.unexpRadiationFlux, ptd.sunFlux, ptd.bodyFlux, ptd.expFlux, ptd.unexpFlux,
                ptd.postShockExtTemp, ptd.finalCoeff, ptd.skinSkinConductionFlux, ptd.skinInteralConductionFlux, ptd.brtExposed,
                ptd.brtUnexposed, ptd.exposed});
    }
    // ---------------------------------------------------------------- part_constants
    // The thermal constants of the tracked parts at flight start (the part prefab after Part.Start) and the thermal graph's link
    // count (FlightIntegrator.UpdateThermalGraph over the whole PhysProf-S tree: every part with a thermal mass is linked to its
    // parent and children). resource_mass is the amount of propellant left after the C10 ascent: not an input of the craft file.
    golden::Table tc("part_constants", "Thermal constants of each tracked part.");
    tc.Col("case", ColType::Str).Col("part", ColType::Str).Col("partName", ColType::Str).Col("mass", ColType::F32);
    for (const char* c : {"maxTemp", "skinMaxTemp", "emissiveConstant", "absorptiveConstant", "heatConductivity", "skinInternalConductionMult",
                          "thermalMassModifier", "skinMassPerArea", "skinThermalMassModifier"})
        tc.Col(c, ColType::F64);
    tc.Col("ptd_thermalLinkCount", ColType::I64);
    tc.key = {"case", "part"};
    {
        GateFI graph;
        graph.vessel = vessel;
        graph.sunBody = sunBody;
        graph.CacheGlobals();
        graph.partCount = static_cast<int>(vessel->parts.size());
        graph.UpdateThermalGraph();
        for (const auto& r : ctab.rows()) {
            const std::string cs = ctab.S(r, "case"), pn = ctab.S(r, "part");
            Part& part = *trackedParts.at(pn.substr(0, pn.find('#')));
            const PartFields& f = part.fields;
            tc.Add({cs, pn, part.partInfo->name, f.mass, f.maxTemp, f.skinMaxTemp, f.emissiveConstant, f.absorptiveConstant,
                    f.heatConductivity, f.skinInternalConductionMult, f.thermalMassModifier, f.skinMassPerArea, f.skinThermalMassModifier,
                    static_cast<int64_t>(part.ptd != nullptr ? part.ptd->thermalLinkCount : -1)});
        }
        if (opt.verbose)
            for (Part* p : vessel->parts) {
                std::fprintf(stderr, "thermal_step: %s tm=%g parent=%s children:", p->partInfo->name.c_str(), p->fields.thermalMass,
                             p->parent ? p->parent->partInfo->name.c_str() : "-");
                for (Part* c : p->children) std::fprintf(stderr, " %s(%g)", c->partInfo->name.c_str(), c->fields.thermalMass);
                std::fprintf(stderr, "\n");
            }
        for (Part* p : vessel->parts) p->ptd = nullptr;  // the graph object dies with `graph`
    }
    g.tables.push_back(std::move(tc));
    if (opt.verbose) std::fprintf(stderr, "thermal_step: %d part rows, %d recovery misses\n", tracked, recoveryMisses);
    g.tables.push_back(std::move(tp));
    return WriteCandidate(opt, g, "thermal_step/thermal_step.json");
}

}  // namespace

KSP_GOLDEN_DATASET("thermal_step", 81, RunThermalStep);
