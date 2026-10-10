// WP-22: the thermal members of FlightIntegrator (FlightIntegrator.cs) over the WP-14 Vessel, the WP-13 Part and WP-09's
// FlightIntegratorAtmosphere (the atmosphere half of the integrator: pressure, density, Mach, shock temperature, convective
// coefficient, density thermal lerp, background radiation temperature are NOT duplicated: `atm`). WP-21's aero half is
// FlightIntegratorAero. The engine seams are the plain input structs below and std::function hooks (named in the comments).
//
// Ported (FlightIntegrator.cs):
//   :452-465   UpdateMassStats, thermal half (thermal mass, skin thermal mass)          -> UpdateThermalMassStats
//   :573-593   FixedUpdate cache block, thermal subset                                  -> CacheGlobals
//   :593-668   FixedUpdate, thermal sequence after the atmosphere (densityThermalLerp, background radiation, CheckThermalGraph,
//              UpdateOcclusion(all), UpdateThermodynamics)                              -> FixedUpdateThermal
//   :672-697   Update / ThermoPrecalculate                                              -> UpdateThermal / ThermoPrecalculate
//   :798-835   CalculateSunBodyFlux (the Physics.Raycast result is an input)            -> CalculateSunBodyFlux
//   :963-1135  CheckThermalGraph, UpdateThermodynamics                                  -> same names
//   :1137-1306 ThermalIntegrationPass                                                   -> same name
//   :1308-1448 CalculateAnalyticTemperature                                             -> same name
//   :1450-1847 PrecalcConduction, UpdateConduction (the skin-local block is UpdateSkinLocalConduction), UnifySkinTemp
//   :1863-1994 UpdateThermalGraph, UpdateCompoundParts
//   :1996-2241 SetSkinProperties, PrecalcConvection, UpdateConvection, PrecalcRadiation, UpdateRadiation, GetSunArea,
//              GetBodyArea, SetSkinThermalMass
//   :2545-2769 UpdateOcclusion, UpdateOcclusionConvection / Solar / Body (the geometry is WP-21's OcclusionData; the part's
//              localToWorldMatrix is the `localToWorld` hook; FXCamera.ApplyObliqueness is rendering and not called)
// Not ported: GameSettings.FI_LOG_* Debug.Log lines and MonoBehaviour.print messages (no behavioural effect); Setup / events /
// SmoothVelocity / Integrate (physics tick: WP-20); the aero members (WP-21).
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "ksp/aero/occlusion.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_atmosphere.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/thermal/thermal_types.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class Vessel;

// What FlightIntegrator reads from the engine in CalculateSunBodyFlux (:798)
struct SunBodyFluxInput {
    Vector3d integratorScaledPos;  // ScaledSpace.LocalToScaledSpace(integratorTransform.position)
    CelestialBody::AtmoThermalFrame frame;  // sunBody.scaledBody.transform.position, scaledBody position, ScaleFactor, bodyTransform.up
    bool raycastHit = false;           // Physics.Raycast(ray, out sunBodyFluxHit, float.MaxValue, sunLayerMask)
    bool hitIsSunBody = false;         // sunBodyFluxHit.transform.GetComponent<ScaledMovement>().celestialBody == sunBody
    float hitDistance = 0.0f;          // sunBodyFluxHit.distance
};

class FlightIntegratorThermal {
public:
    // ---- environment (statics and singletons of the C#) ----
    Vessel* vessel = nullptr;
    const CelestialBody* sunBody = nullptr;  // FlightIntegrator.sunBody (static)
    double universalTime = 0.0;              // Planetarium.GetUniversalTime()
    float timeWarpRate = 1.0f;               // TimeWarp.CurrentRate
    // CompoundPart.target of a compound part (strut / fuel line); unset: no compound links (WP-?? CompoundPart)
    std::function<Part*(Part&)> compoundTarget;
    // part.partTransform.localToWorldMatrix (Unity native): used by the occlusion passes
    std::function<Matrix4x4f(const Part&)> localToWorld;

    // ---- FlightIntegrator fields (names as in the C#) ----
    FlightIntegratorAtmosphere atm;  // density, mach, atmosphericTemperature, externalTemperature, convectiveCoefficient, ...
    double backgroundRadiationTemp = 0.0;
    double backgroundRadiationTempExposed = 0.0;
    double bodyAlbedoFlux = 0.0;
    double bodyEmissiveFlux = 0.0;
    double realDistanceToSun = 0.0;
    Vector3f sunVector{};
    Vector3f nVel{};
    double timeSinceLastUpdate = 0.0;
    bool firstFrame = true;
    bool needOcclusion = true;
    bool isAnalytical = false;
    double fDeltaTime = 0.0;
    double deltaTime = 0.0;
    double fTimeSinceThermo = 0.0;
    double fTimeSinceThermoRecip = 0.0;
    double timeFactor = 0.02;
    double passesRecip = 1.0;
    int passes = 0;
    bool wasMachConvectionEnabled = false;
    int occlusionCounter = 0;
    int partCount = 0;
    bool recreateThermalGraph = true;
    int partThermalDataCount = 0;
    int compoundPartsTimer = 0;
    std::vector<PartThermalData*> partThermalDataList;
    std::vector<PartThermalData*> partThermalDataListSkin;
    std::vector<Part*> compoundParts;
    bool hasCompoundParts = false;  // compoundParts != null
    std::vector<OcclusionData*> occlusionConv, occlusionSun, occlusionBody;
    std::vector<OcclusionCone*> occludersConvection;
    int occludersConvectionCount = 0;
    std::vector<OcclusionCylinder*> occludersSun, occludersBody;
    int occludersSunCount = 0, occludersBodyCount = 0;
    std::vector<IAnalyticOverheatModule*> overheatModules;
    std::vector<IAnalyticPreview*> previewModules;

    // caches (FixedUpdate :573-593, thermal subset)
    double cacheRadiationFactor = 0.0;
    double cacheSpaceTemperature = 0.0;
    double cacheFullConvectionAreaMin = 0.0;
    double cacheFullToCrossSectionLerpStart = 0.0;
    double cacheFullToCrossSectionLerpEnd = 0.0;
    double cacheStefanBoltzmanConstant = 0.0;
    double cacheSkinSkinConductionFactor = 0.0;
    double cacheConductionFactor = 0.0;
    double cacheTurbulentConvectionStart = 0.0;
    double cacheTurbulentConvectionEnd = 0.0;
    double cacheTurbulentConvectionMult = 0.0;
    double cacheStandardSpecificHeatCapacity = 0.0;

    virtual ~FlightIntegratorThermal() = default;

    void CacheGlobals();  // :573-593 (thermal subset)
    // :452-465
    void UpdateThermalMassStats();
    // :672-697
    virtual void ThermoPrecalculate(const SunBodyFluxInput& in);
    // :798 (writes vessel.directSunlight, sunVector, atm.solarFlux / sunDot / atmosphereTemperatureOffset, bodyEmissiveFlux, bodyAlbedoFlux)
    virtual void CalculateSunBodyFlux(const SunBodyFluxInput& in);
    // :593-668 after the atmosphere: `atm` already holds this frame's pressure / constants (CalculatePressure and
    // CalculateConstantsAtmosphere / Vacuum); DragCubeSetupAndPartAeroStats (WP-21) is run by the caller before this.
    virtual void FixedUpdateThermal();
    double CalculateDensityThermalLerp() const { return atm.CalculateDensityThermalLerp(); }  // :837

    void CheckThermalGraph();               // :963
    virtual void UpdateThermodynamics();    // :974
    virtual void ThermalIntegrationPass(bool averageWithPrevious);  // :1137
    virtual double CalculateAnalyticTemperature();  // :1308
    virtual void PrecalcConduction(PartThermalData& ptd);  // :1450
    virtual void UpdateConduction();        // :1457
    // The per-part skin / internal conduction block of UpdateConduction (:1631-1738: skin-skin, unexposed-skin-internal and
    // skin-internal fluxes of one part). It reads only that part's own temperatures: no neighbour is involved.
    void UpdateSkinLocalConduction(PartThermalData& ptd, double skinInternalConductionFactor);
    virtual void UnifySkinTemp(PartThermalData& ptd);  // :1849
    virtual void UpdateThermalGraph();      // :1863
    virtual void UpdateCompoundParts();     // :1958
    virtual void SetSkinProperties(PartThermalData& ptd);  // :1996
    virtual void PrecalcConvection(PartThermalData& ptd);  // :2090
    virtual void UpdateConvection(PartThermalData& ptd);   // :2114
    virtual void PrecalcRadiation(PartThermalData& ptd);   // :2122
    // The tail of PrecalcRadiation (:2138-2180: sun and body flux over the occluded cube areas, split between the exposed and
    // unexposed skin). It is the one place where the part's world orientation enters; a virtual of its own so a host that has
    // no transforms can supply the geometry (the C6 gate recovers it from the capture). `unexposedArea` = radiativeArea *
    // (1 - skinExposedAreaFrac) (the C# local num2).
    virtual void PrecalcRadiationIncoming(PartThermalData& ptd, double unexposedArea);
    virtual void UpdateRadiation(PartThermalData& ptd);    // :2182
    virtual double GetSunArea(PartThermalData& ptd);       // :2214
    virtual double GetBodyArea(PartThermalData& ptd);      // :2225
    virtual void SetSkinThermalMass(Part& part);           // :2236

    void UpdateOcclusion(bool all);  // :2545
    void UpdateOcclusionConvection();  // :2572
    void UpdateOcclusionSolar();       // :2670
    void UpdateOcclusionBody();        // :2720

    // The thermal data of this integrator's graph: owner of every PartThermalData (the C# lets the GC do it)
    std::vector<std::unique_ptr<PartThermalData>> ptdStore;

private:
    // The first half of UpdateThermodynamics' non-analytical branch for one part (:1067-1100)
    void ThermodynamicsPrecalc(PartThermalData& ptd, double& analyticTemp, bool& needAnalytic);
};

}  // namespace ksp
