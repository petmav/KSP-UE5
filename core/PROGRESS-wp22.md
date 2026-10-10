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

## Step 1 DONE: capture.atmosphere_thermal_offsets passes, samples 2688 rows, 0 ULP all columns (commit on wp22).


## Step 2: thermal port written (include/ksp/thermal/{thermal_types,flight_integrator_thermal}.hpp, src/thermal/flight_integrator_thermal.cpp), builds on Windows.
Part::ptd member + PartModule::AsAnalytic*() added. Next: gate ds_thermal_step.cpp (vessel table: FlightIntegratorAtmosphere with capture inputs; parts table:
per-part one-step recompute, prev-row temps as inputs; neighbour conduction fluxes + recovered radiation geometry as inputs), unit tests, modules.


## Step 3 DONE: capture.thermal_step passes 0 ULP (vessel 2994 rows after 6 documented exclusions, parts 14,985 rows x 27 columns).
Runner ds_thermal_step.cpp: GateFI (UpdateConduction neighbour terms + PrecalcRadiationIncoming geometry from capture; everything else the
real UpdateThermodynamics on a one-part graph). Recovered inputs: occlusion multipliers, incoming sun/body flux, conduction to unlogged
neighbours, FI.altitude from static pressure. Part::StartData supplies absorptiveConstant etc. Exclusions: tests/data/thermal_step_exclusions.json.
Remaining: part_constants table gate, modules (TemperatureModel, ModuleConductionMultiplier, ModuleCoreHeat, ModuleActiveRadiator, ModuleAblator),
tests/test_thermal.cpp, STATUS/README, full runs Windows + WSL.

## Step 4: thermal_modules (TemperatureModel, RadiatorUtilities, ModuleConductionMultiplier/Ablator/CoreHeat/ActiveRadiator) written + registered (stand-in ModuleCoreHeat removed from module_behaviours.cpp), Part::AddThermalFlux/AddSkinThermalFlux/AddExposedThermalFlux added. Builds. Next: tests/test_thermal.cpp, README/STATUS, full runs.

## Step 5 DONE: tests/test_thermal.cpp (7 cases, 112 assertions) pass; part_constants table added to capture.thermal_step (15 rows, 0 ULP).
Next: README rows (tests table, layout, golden gate options), STATUS WP-22 section + header counts, full Windows -Fresh run, WSL run, commit.

## Step 6: README/STATUS committed; Windows -Fresh run: 30/30 ctest pass, 299 unit cases / 685,133 assertions. WSL linux-rel run in progress (/tmp/wp22_wsl.log).
