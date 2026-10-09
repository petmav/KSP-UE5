// FlightIntegrator's per-frame atmospheric state and the protected helpers that compute it (FlightIntegrator.cs:808-985):
// static pressure, temperature with the latitude/sun offset, density, dynamic pressure, speed of sound, Mach, solar
// attenuation, shock temperature, convective coefficients, pseudo-Reynolds drag multiplier, density thermal lerp and
// background radiation temperature.
//
// The full FlightIntegrator (thermal graph, aero, buoyancy) is a later package; it will own one of these structs. The
// field names are FlightIntegrator's; values the C# also mirrors onto the Vessel (vessel.atmDensity, vessel.mach, ...)
// are the same fields here (vessel.speedOfSound only exists on the Vessel: `speedOfSound`).
#pragma once

#include "ksp/physics/vessel_ranges.hpp"  // VesselSituations

namespace ksp {

class CelestialBody;

struct FlightIntegratorAtmosphere {
    // ---- inputs (set by the integrator every frame) ----
    const CelestialBody* currentMainBody = nullptr;
    double altitude = 0.0;                     // m above sea level
    double spd = 0.0;                          // air-relative speed, m/s
    double sunDot = 0.0;                       // dot(sun direction, local up)
    double atmosphereTemperatureOffset = 0.0;  // CelestialBody.GetAtmoThermalStats output (CalculateSunBodyFlux)
    double solarFlux = 0.0;                    // W/m^2 before atmospheric attenuation (CalculateSunBodyFlux)
    VesselSituations situation = VesselSituations::FLYING;  // vessel.situation
    float reentryHeatScale = 1.0f;  // HighLogic.CurrentGame.Parameters.Difficulty.ReentryHeatScale (GameParameters.cs:218)

    // ---- state / outputs ----
    double staticPressurekPa = 0.0;
    double staticPressureAtm = 0.0;
    double atmosphericTemperature = 0.0;
    double externalTemperature = 0.0;
    double density = 0.0;  // read by the next frame's GetSolarAtmosphericEffects (KSP's previous-frame quirk)
    double dynamicPressurekPa = 0.0;
    double speedOfSound = 0.0;  // vessel.speedOfSound
    double mach = 0.0;
    double solarAirMass = 0.0;
    double solarFluxMultiplier = 0.0;
    double convectiveMachLerp = 0.0;
    double convectiveCoefficient = 0.0;
    double convectiveMachFlux = 0.0;
    double pseudoReynolds = 0.0;
    double pseudoReLerpTimeMult = 0.0;
    double pseudoReDragMult = 0.0;
    double densityThermalLerp = 0.0;

    void CalculatePressure();             // FlightIntegrator.cs:808
    void CalculateConstantsVacuum();      // FlightIntegrator.cs:885
    // FlightIntegrator.cs:917. Uses `density` as left by the previous call (previous physics frame) for the solar air
    // mass, exactly like KSP; call twice with the same inputs for the steady state.
    void CalculateConstantsAtmosphere();
    double CalculateShockTemperature() const;                          // FlightIntegrator.cs:944
    double CalculateAtmosphericDensity(double pres, double temp) const;  // FlightIntegrator.cs:955
    double CalculateConvectiveCoefficient() const;                     // FlightIntegrator.cs:965 (situation)
    double CalculateConvectiveCoefficient(VesselSituations situation) const;  // FlightIntegrator.cs:970
    double CalculateConvectiveCoefficientNewtonian() const;            // FlightIntegrator.cs:977
    double CalculateConvectiveCoefficientMach() const;                 // FlightIntegrator.cs:982
    double CalculateDensityThermalLerp() const;                        // FlightIntegrator.cs:861
    double CalculateBackgroundRadiationTemperature(double ambientTemp) const;  // FlightIntegrator.cs:880
};

}  // namespace ksp
