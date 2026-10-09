// The C10 assembled-vessel capture (harness/captures/out2/aero_forces_vessel.json) as a test environment, shared by the WP-14
// vessel_state and the WP-21 aero_forces_vessel datasets: the game's part database (GameData mirror or --gamedata), the three
// PhysProf crafts (analysis/physics/craft) assembled into Vessels, the capture JSON, and the FlightIntegrator atmosphere state of
// a captured snapshot (WP-14's recovery of FlightIntegrator.altitude and atmosphereTemperatureOffset from the captured static
// pressure and temperature, see ds_vessel_state.cpp).
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "celestial_env.hpp"
#include "datasets.hpp"
#include "ksp/celestial/flight_atmosphere.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/vessel/vessel.hpp"
#include "nlohmann/json.hpp"

struct VesselCapture {
    ksp::CelestialBody* kerbin = nullptr;
    std::unique_ptr<ksp::GameData> data;  // owns the part prefabs the vessels reference: declared before `vessels`
    nlohmann::json cap;
    std::vector<std::string> craftNames = {"PhysProf-S", "PhysProf-M2", "PhysProf-L2-AS"};
    std::map<std::string, std::unique_ptr<ksp::Vessel>> vessels;

    explicit VesselCapture(const RunnerOptions& opt);
    ~VesselCapture();
    VesselCapture(const VesselCapture&) = delete;
    VesselCapture& operator=(const VesselCapture&) = delete;

    const nlohmann::json& Table(const char* name) const;
    static size_t Col(const nlohmann::json& table, const char* column);

    // FlightIntegrator's atmosphere for a `states` row: altitude recovered from the captured static pressure (fallback: the captured
    // root-part altitude), temperature offset from the captured temperature, then CalculatePressure + CalculateConstantsAtmosphere
    // (or CalculateConstantsVacuum). Prints a note when an inversion fails.
    ksp::FlightIntegratorAtmosphere StateAtmosphere(const nlohmann::json& statesTable, const nlohmann::json& row) const;
};
