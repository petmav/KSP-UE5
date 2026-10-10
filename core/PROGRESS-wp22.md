# PROGRESS WP-22 (thermal model, numeric part)

Branch wp22 (from main 289d0c98).

## Captures (inspected)
- atmosphere_thermal_offsets.json (C4): tables epochs 4 (UT, sun scaled pos f32, ScaledSpace_ScaleFactor f32), body_epoch 24
  (rotationAngle, rotationPeriod, solarDayLength, bodyTransform_up f32 xyz, scaledBody_pos f32, position f64, ref_* = orbit of the
  body referencing the sun: trueAnomaly/radius/PeR/ApR/eccentricity, orbit_inclination, atmosphere, atmosphereDepth), samples 2688
  (epoch, body, lat, lon, alt; inputs pos, scaledPos, sunVector, upAxis, sunDot; outputs atmosphereTemperatureOffset, bodyEmissiveFlux,
  bodyAlbedoFlux, bodyTemperature, GetTemperature, GetFullTemperature_alt_offset, GetFullTemperature_pos, pressure_kPa, density,
  solarAirMass, solarFluxMultiplier). CelestialBody.GetAtmoThermalStats NOT ported yet (celestial_body.hpp:314).
- thermal_step.json (C6): vessel 3000 (pad 500 / orbit 1000 / reentry 1500; per-frame FlightIntegrator atmosphere+thermal scalars),
  parts 15000 (5 tracked parts of PhysProf-S, 21 parts total, x frames; temps, fluxes, areas, ptd_* intermediates), part_constants 15.
  Only 5 of 21 parts logged -> neighbour temps unknown -> conduction not reproducible from the capture alone.

## Step 1 (done, building): CelestialBody.GetAtmoThermalStats + GetFullTemperature(pos) ported (celestial_body.cpp), ICelestialBodyOrbit gets
eccentricity()/inclination(); runner ds_atmosphere_thermal_offsets.cpp, gate capture.atmosphere_thermal_offsets (TABLES samples).
Plan for thermal_step: gate per-frame vessel scalars with FlightIntegratorAtmosphere (inputs from capture) + per-part one-step recompute
(prev-frame temps from capture row n-1, local fluxes; conduction with unlogged neighbours cannot be reproduced -> take the two
neighbour-dependent fluxes as inputs and document).
