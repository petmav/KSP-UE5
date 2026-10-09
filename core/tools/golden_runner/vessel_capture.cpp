// See vessel_capture.hpp. Moved out of ds_vessel_state.cpp (WP-14) by WP-21 so that the aero_forces_vessel dataset shares the
// vessel assembly and the FlightIntegrator atmosphere mirror instead of duplicating them.
#include "vessel_capture.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel_assembly.hpp"

using namespace ksp;

namespace {

namespace fs = std::filesystem;

std::string GameRootOf(const std::string& gamedata) {
    const fs::path gd = fs::absolute(dotnet::PathFromUtf8(gamedata)).lexically_normal();
    fs::path p = gd;
    if (!p.has_filename()) p = p.parent_path();
    const std::u8string s = p.parent_path().u8string();
    return std::string(s.begin(), s.end());
}

double NextUp(double v) { return std::nextafter(v, INFINITY); }
double NextDown(double v) { return std::nextafter(v, -INFINITY); }

// FlightIntegrator.atmosphereTemperatureOffset such that GetFullTemperature(altitude, offset) == T (searched +-512 ulps around
// (T - base) / mult). Returns false if no offset reproduces T.
bool RecoverTemperatureOffset(const CelestialBody& body, double altitude, double T, double& offset) {
    const double mult = static_cast<double>(body.atmosphereTemperatureSunMultCurve.Evaluate(static_cast<float>(altitude)));
    if (body.GetFullTemperature(altitude, 0.0) == T) {
        offset = 0.0;
        return true;
    }
    if (mult == 0.0) return false;
    const double guess = (T - body.GetTemperature(altitude)) / mult;
    double up = guess, down = guess;
    for (int i = 0; i < 512; i++) {
        if (body.GetFullTemperature(altitude, up) == T) {
            offset = up;
            return true;
        }
        if (body.GetFullTemperature(altitude, down) == T) {
            offset = down;
            return true;
        }
        up = NextUp(up);
        down = NextDown(down);
    }
    return false;
}

// FlightIntegrator.altitude (the vessel CoM's altitude, CalculatePressure's input) is not in the capture: the capture's `altitude` column
// is Vessel.altitude of the root part (a 457-part stack differs by ~100 m). The pressure curve is monotone, so the altitude is
// recovered as the lowest one whose GetPressure equals the captured static pressure bit for bit.
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

}  // namespace

VesselCapture::VesselCapture(const RunnerOptions& opt) {
    CelestialEnv& cenv = CelestialEnv::Get(opt);
    for (CelestialBody* b : cenv.system->SpawnOrder())
        if (b->bodyName == "Kerbin") kerbin = b;
    if (kerbin == nullptr) throw std::runtime_error("vessel capture: no Kerbin");

    // the game's part database (GameData mirror or --gamedata), the same loading sequence as part_database
    SaveEnvironment::Reset();
    data = std::make_unique<GameData>();
    GameData::Options go;
    go.gameRoot = GameRootOf(opt.gamedata);
    go.gameDataDir = fs::absolute(dotnet::PathFromUtf8(opt.gamedata)).lexically_normal().string();
    data->Load(go);
    SaveEnvironment& senv = SaveEnvironment::Current();
    GameData* d = data.get();
    senv.getPartInfoByName = [d](std::string_view n) { return d->parts.getPartInfoByName(n); };
    senv.partExists = [d](std::string_view n) { return d->parts.getPartInfoByName(n) != nullptr; };
    senv.newGuidHashCode = [] {
        static uint32_t s = 777u;
        s = s * 1664525u + 1013904223u;
        return static_cast<int32_t>(s);
    };
    senv.localBodies = cenv.system->SpawnOrder();
    FlightRegistry::Clear();
    auto partInfo = [d](const std::string& n) { return d->parts.getPartInfoByName(n); };

    {
        std::ifstream f(dotnet::PathFromUtf8(opt.harness + "/captures/out2/aero_forces_vessel.json"), std::ios::binary);
        if (!f) throw std::runtime_error("vessel capture: aero_forces_vessel.json not found under " + opt.harness);
        cap = nlohmann::json::parse(f);
    }

    // assemble the three vessels (craft name -> Vessel); launch pad of KSC (Kerbin world position at 0 m)
    for (const std::string& name : craftNames) {
        ConfigNode::NodePtr root = ConfigNode::Load(opt.harness + "/../analysis/physics/craft/" + name + ".craft");
        if (!root) throw std::runtime_error("vessel capture: cannot read craft " + name);
        ShipConstruct ship;
        std::string err;
        if (!ship.LoadShip(*root, 0, true, err)) throw std::runtime_error("vessel capture: LoadShip " + name + ": " + err);
        AssembleOptions ao;
        ao.rootPosition = kerbin->GetWorldSurfacePosition(-0.0972077486779459, -74.5576813534722, 75.0).ToVector3f();
        std::unique_ptr<Vessel> v = AssembleVesselFromCraft(ship, partInfo, ao);
        if (!v) throw std::runtime_error("vessel capture: AssembleVesselFromCraft " + name);
        vessels[name] = std::move(v);
    }
}

VesselCapture::~VesselCapture() {
    // vessels reference the part prefabs of `data`: release them before it goes away
    vessels.clear();
    FlightRegistry::Clear();
    SaveEnvironment::Reset();
}

const nlohmann::json& VesselCapture::Table(const char* name) const {
    for (const auto& t : cap.at("tables"))
        if (t.at("name") == name) return t;
    throw std::runtime_error(std::string("vessel capture: no table ") + name);
}

size_t VesselCapture::Col(const nlohmann::json& t, const char* c) {
    size_t i = 0;
    for (const auto& col : t.at("columns")) {
        if (col.at("name") == c) return i;
        i++;
    }
    throw std::runtime_error(std::string("vessel capture: no column ") + c);
}

FlightIntegratorAtmosphere VesselCapture::StateAtmosphere(const nlohmann::json& st, const nlohmann::json& row) const {
    const std::string craft = row[Col(st, "craft")], state = row[Col(st, "state")];
    const double altitude = row[Col(st, "altitude")], spd = row[Col(st, "spd")], T = row[Col(st, "atmosphericTemperature")],
                 P = row[Col(st, "staticPressurekPa")];
    FlightIntegratorAtmosphere fi;
    fi.currentMainBody = kerbin;
    fi.situation = VesselSituations::FLYING;
    fi.altitude = altitude;
    fi.spd = spd;
    double pAlt = altitude;
    if (!RecoverPressureAltitude(*kerbin, P, pAlt))
        std::cerr << "vessel capture: no altitude reproduces the static pressure of " << craft << "/" << state << "\n";
    fi.altitude = pAlt;
    fi.CalculatePressure();
    if (fi.staticPressurekPa > 0.0) {
        double offset = 0.0;
        if (!RecoverTemperatureOffset(*kerbin, pAlt, T, offset))
            std::cerr << "vessel capture: no temperature offset reproduces " << craft << "/" << state << " (T=" << T << ")\n";
        fi.atmosphereTemperatureOffset = offset;
        fi.CalculateConstantsAtmosphere();
    } else {
        fi.CalculateConstantsVacuum();
    }
    return fi;
}
