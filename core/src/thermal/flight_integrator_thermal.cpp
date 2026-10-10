// FlightIntegrator.cs thermal members, operation for operation (see include/ksp/thermal/flight_integrator_thermal.hpp for the
// line map). All thermal arithmetic is C# `double`; the only float code is the Vector3 products (Vector3.Dot, managed UnityEngine
// code under the Mono rule: ksp/unity/vector.hpp) and the native FloatCurve evaluation, both already ported. Runtime maths go
// through ksp::dotnet::Math. Not ported: the GameSettings.FI_LOG_TEMP_ERROR / FI_LOG_OVERTEMP Debug.Log blocks.
#include "ksp/thermal/flight_integrator_thermal.hpp"

#include <cmath>
#include <limits>
#include <map>

#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace M = dotnet::Math;

namespace {
inline bool IsNaN(double d) { return d != d; }
}  // namespace

// ---------------------------------------------------------------------------------------------------------------------
// ThermalLink.cs:21
ThermalLink::ThermalLink(Part& part, Part& remotePart_) {
    remotePart = &remotePart_;
    AttachNode* attachNode = remotePart_.FindAttachNodeByPart(&part);
    if (attachNode != nullptr) {
        if (attachNode->contactArea > 0.0f) {
            contactArea = static_cast<double>(attachNode->contactArea);
            AttachNode* attachNode2 = part.FindAttachNodeByPart(&remotePart_);
            if (attachNode2 != nullptr) {
                partToNode = (attachNode->position - part.partTransform.position).normalized();
                remoteToNode = (attachNode2->position - remotePart_.partTransform.position).normalized();
                useOrientation = true;
            }
        }
    } else {
        attachNode = part.FindAttachNodeByPart(&remotePart_);
        if (attachNode != nullptr && attachNode->contactArea > 0.0f) {
            contactArea = static_cast<double>(attachNode->contactArea);
            AttachNode* attachNode3 = remotePart_.FindAttachNodeByPart(&part);
            if (attachNode3 != nullptr) {
                partToNode = (attachNode3->position - part.partTransform.position).normalized();
                remoteToNode = (attachNode->position - remotePart_.partTransform.position).normalized();
                useOrientation = true;
            }
        }
    }
    contactArea = M::Max(contactArea, 0.01);
    contactAreaSqrt = M::Sqrt(contactArea);
    if (IsNaN(contactAreaSqrt)) contactAreaSqrt = 1.0;
}

// PartThermalData.cs:104
PartThermalData::PartThermalData(Part& part_)
    : OcclusionThermalRef{&part_}, convectionData(*this), sunData(*this), bodyData(*this) {
    thermalLinkCount = 0;
    const size_t count = part_.Modules().Count();
    for (size_t i = 0; i < count; i++) {
        PartModule* partModule = part_.Modules()[i];
        if (IAnalyticOverheatModule* m = partModule->AsAnalyticOverheatModule()) overheatModules.push_back(m);
        if (IAnalyticPreview* m = partModule->AsAnalyticPreview()) analyticPreviewModules.push_back(m);
        if (IAnalyticTemperatureModifier* m = partModule->AsAnalyticTemperatureModifier()) {
            if (analyticModifier == nullptr) analyticModifier = m;  // else: Debug.LogWarning("... multiple IAnalyticTemperatureModifier!")
        }
        if (partModule->IsA("KerbalEVA")) isEVA = true;
    }
}

// PartThermalData.cs:146
double PartThermalData::GetUnifiedSkinTemp() const {
    const Part& p = *part;
    if (exposed) {
        return p.fields.skinExposedAreaFrac * p.fields.skinTemperature +
               (1.0 - p.fields.skinExposedAreaFrac) * p.fields.skinUnexposedTemperature;
    }
    return p.fields.skinTemperature;
}

// PartThermalDataIntComparer.cs:7 / PartThermalDataSkinComparer.cs:7
int PartThermalData::IntCompare(const PartThermalData* a, const PartThermalData* b) {
    if (a->part->fields.temperature < b->part->fields.temperature) return -1;
    if (a->part->fields.temperature == b->part->fields.temperature) return 0;
    return 1;
}
int PartThermalData::SkinCompare(const PartThermalData* a, const PartThermalData* b) {
    if (a->unifiedTemp < b->unifiedTemp) return -1;
    if (a->unifiedTemp == b->unifiedTemp) return 0;
    return 1;
}

// ---------------------------------------------------------------------------------------------------------------------
// FlightIntegrator.cs:573-593 (thermal subset)
void FlightIntegratorThermal::CacheGlobals() {
    cacheRadiationFactor = PhysicsGlobals::RadiationFactor();
    cacheSpaceTemperature = PhysicsGlobals::SpaceTemperature();
    cacheFullConvectionAreaMin = PhysicsGlobals::FullConvectionAreaMin();
    cacheFullToCrossSectionLerpStart = PhysicsGlobals::FullToCrossSectionLerpStart();
    cacheFullToCrossSectionLerpEnd = PhysicsGlobals::FullToCrossSectionLerpEnd();
    cacheStefanBoltzmanConstant = PhysicsGlobals::StefanBoltzmanConstant();
    cacheSkinSkinConductionFactor = PhysicsGlobals::SkinSkinConductionFactor();
    cacheConductionFactor = PhysicsGlobals::ConductionFactor();
    cacheTurbulentConvectionStart = PhysicsGlobals::TurbulentConvectionStart();
    cacheTurbulentConvectionEnd = PhysicsGlobals::TurbulentConvectionEnd();
    cacheTurbulentConvectionMult = PhysicsGlobals::TurbulentConvectionMult();
    cacheStandardSpecificHeatCapacity = PhysicsGlobals::StandardSpecificHeatCapacity();
}

// FlightIntegrator.cs:452-465 (the first loop of UpdateMassStats; the rigidbody mass loop is WP-20's)
void FlightIntegratorThermal::UpdateThermalMassStats() {
    int index = partCount;
    while (index-- > 0) {
        Part& part = *vessel->parts[index];
        part.fields.resourceMass = part.GetResourceMass(part.fields.resourceThermalMass);
        part.fields.thermalMass = static_cast<double>(part.fields.mass) * cacheStandardSpecificHeatCapacity *
                                      part.fields.thermalMassModifier +
                                  part.fields.resourceThermalMass;
        SetSkinThermalMass(part);
        part.fields.thermalMass = M::Max(part.fields.thermalMass - part.fields.skinThermalMass, 0.1);
        part.fields.thermalMassReciprocal = 1.0 / part.fields.thermalMass;
    }
}

// FlightIntegrator.cs:672-697 (Update): ThermoPrecalculate; the occlusion timer is UpdateThermal's
void FlightIntegratorThermal::ThermoPrecalculate(const SunBodyFluxInput& in) { CalculateSunBodyFlux(in); }

// FlightIntegrator.cs:798
void FlightIntegratorThermal::CalculateSunBodyFlux(const SunBodyFluxInput& in) {
    if (vessel == nullptr || vessel->state == VesselState::DEAD || atm.currentMainBody == nullptr) return;
    vessel->directSunlight = false;
    const Vector3d vector3d = in.integratorScaledPos;
    const Vector3d vector3d2 = Vector3d(in.frame.sunScaledPos);
    const Vector3d vector3d3 = vector3d2 - vector3d;
    double num = (vector3d2 - vector3d).magnitude();
    if (num == 0.0) num = 1.0;
    sunVector = (vector3d3 / num).ToVector3f();
    atm.solarFlux = 0.0;
    atm.sunDot = Vector3d::Dot(Vector3d(sunVector), vessel->upAxis);
    // currentMainBody.GetAtmoThermalStats(getBodyFlux: true, sunBody, sunVector, sunDot, vessel.upAxis, altitude, ...)
    const_cast<CelestialBody*>(atm.currentMainBody)
        ->GetAtmoThermalStats(true, sunBody, Vector3d(sunVector), atm.sunDot, vessel->upAxis, atm.altitude, in.frame,
                              atm.atmosphereTemperatureOffset, bodyEmissiveFlux, bodyAlbedoFlux);
    if (in.raycastHit) {
        if (in.hitIsSunBody) {
            realDistanceToSun = static_cast<double>(in.frame.scaleFactor) * static_cast<double>(in.hitDistance);
            vessel->directSunlight = true;
        }
    } else {
        vessel->directSunlight = true;
        realDistanceToSun = num * static_cast<double>(in.frame.scaleFactor) - sunBody->Radius;
    }
    if (vessel->directSunlight) {
        atm.solarFlux = PhysicsGlobals::SolarLuminosity() / (UtilMath::PI * 4.0 * realDistanceToSun * realDistanceToSun);
    }
}

// FlightIntegrator.cs:593-668, the thermal statements after CalculateConstantsAtmosphere / Vacuum
void FlightIntegratorThermal::FixedUpdateThermal() {
    atm.densityThermalLerp = atm.CalculateDensityThermalLerp();
    backgroundRadiationTemp = atm.CalculateBackgroundRadiationTemperature(atm.atmosphericTemperature);
    backgroundRadiationTempExposed = atm.CalculateBackgroundRadiationTemperature(atm.externalTemperature);
    CheckThermalGraph();
    if (needOcclusion) UpdateOcclusion(true);
    UpdateThermodynamics();
}

// FlightIntegrator.cs:963
void FlightIntegratorThermal::CheckThermalGraph() {
    if (recreateThermalGraph) {
        UpdateThermalGraph();
        needOcclusion = true;
        recreateThermalGraph = false;
    }
    UpdateCompoundParts();
}

// FlightIntegrator.cs:974
void FlightIntegratorThermal::UpdateThermodynamics() {
    fTimeSinceThermo += fDeltaTime;
    if (!(fTimeSinceThermo > PhysicsGlobals::ThermalIntegrationMinStep())) return;
    const double universalTimeNow = universalTime;
    timeSinceLastUpdate = universalTimeNow - vessel->lastUT;
    if (vessel->lastUT >= 0.0) {
        if (timeSinceLastUpdate < 0.0) timeSinceLastUpdate = fTimeSinceThermo;
    } else {
        timeSinceLastUpdate = std::numeric_limits<double>::max();
    }
    vessel->lastUT = universalTimeNow;
    isAnalytical = static_cast<double>(timeWarpRate) > PhysicsGlobals::ThermalMaxIntegrationWarp() ||
                   timeSinceLastUpdate > PhysicsGlobals::ThermalMaxIntegrationWarp() * 0.04;
    fTimeSinceThermoRecip = 1.0 / fTimeSinceThermo;
    double num = 0.0;
    bool flag = true;
    if (isAnalytical) {
        num = CalculateAnalyticTemperature();
        double num2 = fTimeSinceThermo;
        if (timeSinceLastUpdate > fTimeSinceThermo * 1.1) num2 = timeSinceLastUpdate;  // + MonoBehaviour.print
        const double num3 = PhysicsGlobals::AnalyticLerpRateInternal() * num2;
        const double num4 = PhysicsGlobals::AnalyticLerpRateSkin() * num2;
        if (!IsNaN(num)) {
            int index = partThermalDataCount;
            while (index-- > 0) {
                PartThermalData& partThermalData = *partThermalDataList[index];
                Part& part = *partThermalDataList[index]->part;
                double num5 = num4 * part.fields.analyticSkinInsulationFactor;
                double num6 = num3 * part.fields.analyticInternalInsulationFactor;
                if (num5 > 1.0) num5 = 1.0;
                if (num6 > 1.0) num6 = 1.0;
                const double num7 = UtilMath::LerpUnclamped(part.fields.skinTemperature, num, num5);
                const double num8 = UtilMath::LerpUnclamped(part.fields.temperature, num, num6);
                if (partThermalData.analyticModifier != nullptr) {
                    IAnalyticTemperatureModifier* analyticModifier = partThermalData.analyticModifier;
                    analyticModifier->SetAnalyticTemperature(*this, num, num8, num7);
                    bool lerp = false;
                    const double internalTemperature = analyticModifier->GetInternalTemperature(lerp);
                    bool lerp2 = false;
                    const double skinTemperature = analyticModifier->GetSkinTemperature(lerp2);
                    if (lerp)
                        part.fields.temperature = UtilMath::LerpUnclamped(part.fields.temperature, internalTemperature, num6);
                    else
                        part.fields.temperature = internalTemperature;
                    if (lerp2)
                        part.fields.skinTemperature = UtilMath::LerpUnclamped(part.fields.skinTemperature, skinTemperature, num5);
                    else
                        part.fields.skinTemperature = skinTemperature;
                } else {
                    part.fields.skinTemperature = num7;
                    part.fields.temperature = num8;
                }
            }
        }
        // else if (GameSettings.FI_LOG_TEMP_ERROR) Debug.LogError(...)
    } else {
        int index2 = partThermalDataCount;
        while (index2-- > 0) {
            PartThermalData& partThermalData2 = *partThermalDataList[index2];
            Part& part2 = *partThermalData2.part;
            part2.fields.thermalConductionFlux = 0.0;
            part2.fields.skinToInternalFlux = 0.0;
            part2.fields.thermalConvectionFlux = 0.0;
            part2.fields.thermalRadiationFlux = 0.0;
            partThermalData2.previousTemperature = partThermalData2.part->fields.temperature;
            partThermalData2.previousSkinTemperature = partThermalData2.part->fields.skinTemperature;
            partThermalData2.previousSkinUnexposedTemperature = partThermalData2.part->fields.skinUnexposedTemperature;
            if (part2.fields.temperature < cacheSpaceTemperature || part2.fields.skinTemperature < cacheSpaceTemperature) {
                if (flag) {
                    num = CalculateAnalyticTemperature();
                    flag = false;
                }
                if (!IsNaN(num)) {
                    part2.fields.skinUnexposedTemperature = num;
                    part2.fields.skinTemperature = num;
                    part2.fields.temperature = num;
                }
            }
            SetSkinProperties(partThermalData2);
            PrecalcConduction(partThermalData2);
            PrecalcConvection(partThermalData2);
            PrecalcRadiation(partThermalData2);
            part2.fields.thermalInternalFluxPrevious = part2.fields.thermalInternalFlux * fTimeSinceThermoRecip;
            part2.fields.thermalInternalFlux = 0.0;
            part2.fields.thermalSkinFluxPrevious = part2.fields.thermalSkinFlux * fTimeSinceThermoRecip;
            part2.fields.thermalSkinFlux = 0.0;
            part2.fields.thermalExposedFluxPrevious = part2.fields.thermalExposedFlux * fTimeSinceThermoRecip;
            part2.fields.thermalExposedFlux = 0.0;
        }
        if (!(timeWarpRate > 1.0f) && !PhysicsGlobals::ThermalIntegrationAlwaysRK2()) {
            timeFactor = fTimeSinceThermo;
            ThermalIntegrationPass(false);
        } else if (fTimeSinceThermo > PhysicsGlobals::ThermalIntegrationMaxTimeOnePass()) {
            double num9 = M::Sqrt(fTimeSinceThermo * 50.0 / PhysicsGlobals::ThermalMaxIntegrationWarp());
            num9 = M::Sqrt(num9) * num9;
            passes = dotnet::ConvToInt32(M::Round(num9 * static_cast<double>(PhysicsGlobals::ThermalIntegrationHighMaxPasses())));
            if (passes < PhysicsGlobals::ThermalIntegrationHighMinPasses()) passes = PhysicsGlobals::ThermalIntegrationHighMinPasses();
            passesRecip = 1.0 / static_cast<double>(passes);
            timeFactor = fTimeSinceThermo * passesRecip;
            int num10 = passes;
            while (num10-- > 0) {
                ThermalIntegrationPass(false);
                ThermalIntegrationPass(true);
            }
        } else {
            passes = 1;
            timeFactor = fTimeSinceThermo;
            ThermalIntegrationPass(false);
            ThermalIntegrationPass(true);
        }
    }
    fTimeSinceThermo = 0.0;
}

// FlightIntegrator.cs:1137
void FlightIntegratorThermal::ThermalIntegrationPass(bool averageWithPrevious) {
    if (PhysicsGlobals::ThermalConductionEnabled()) UpdateConduction();
    const double thermalConvergenceFactor = PhysicsGlobals::ThermalConvergenceFactor();
    const bool thermalConvectionEnabled = PhysicsGlobals::ThermalConvectionEnabled();
    const bool thermalRadiationEnabled = PhysicsGlobals::ThermalRadiationEnabled();
    int index = partThermalDataCount;
    while (index-- > 0) {
        PartThermalData& partThermalData = *partThermalDataList[index];
        Part& part = *partThermalData.part;
        if (!averageWithPrevious) {
            partThermalData.previousTemperature = part.fields.temperature;
            partThermalData.previousSkinTemperature = part.fields.skinTemperature;
            partThermalData.previousSkinUnexposedTemperature = part.fields.skinUnexposedTemperature;
        }
        double num = 0.0;
        partThermalData.convectionFlux = 0.0;
        const double unexpRadiationFlux = num;
        num = 0.0;
        partThermalData.unexpRadiationFlux = unexpRadiationFlux;
        partThermalData.radiationFlux = num;
        if (thermalConvectionEnabled) UpdateConvection(partThermalData);
        if (thermalRadiationEnabled) UpdateRadiation(partThermalData);
        const double num2 = part.fields.skinThermalMassRecip * part.fields.skinExposedMassMult * timeFactor;
        double num3 = 0.0;
        if (part.fields.skinExposedAreaFrac > 0.0) num3 = part.fields.skinThermalMassRecip * part.fields.skinUnexposedMassMult * timeFactor;
        const double num4 = part.fields.thermalMassReciprocal * timeFactor;
        part.fields.temperature += part.fields.thermalInternalFluxPrevious * num4;
        const double num5 = part.fields.thermalSkinFluxPrevious * part.fields.skinThermalMassRecip * timeFactor;
        if (part.fields.skinExposedAreaFrac > 0.0) part.fields.skinTemperature += num5;
        if (part.fields.skinExposedAreaFrac < 1.0) part.fields.skinUnexposedTemperature += num5;
        if (part.fields.skinExposedAreaFrac > 0.0)
            part.fields.skinTemperature += part.fields.thermalExposedFluxPrevious * num2;
        else
            part.fields.skinUnexposedTemperature += part.fields.thermalExposedFluxPrevious * num3;
        part.fields.skinToInternalFlux =
            (partThermalData.skinInteralConductionFlux + partThermalData.unexpSkinInternalConductionFlux) * thermalConvergenceFactor;
        part.fields.thermalConductionFlux = partThermalData.intConductionFlux * thermalConvergenceFactor;
        part.fields.temperature += (part.fields.thermalConductionFlux + part.fields.skinToInternalFlux) * num4;
        part.fields.skinTemperature += (partThermalData.skinConductionFlux * part.fields.skinExposedAreaFrac -
                                        partThermalData.skinSkinConductionFlux - partThermalData.skinInteralConductionFlux) *
                                       num2 * thermalConvergenceFactor;
        part.fields.skinUnexposedTemperature +=
            (partThermalData.skinConductionFlux * (1.0 - part.fields.skinExposedAreaFrac) + partThermalData.skinSkinConductionFlux -
             partThermalData.unexpSkinInternalConductionFlux) *
            num3 * thermalConvergenceFactor;
        part.fields.thermalConvectionFlux = partThermalData.convectionFlux * thermalConvergenceFactor;
        part.fields.skinTemperature += part.fields.thermalConvectionFlux * num2;
        part.fields.skinTemperature += partThermalData.radiationFlux * num2;
        part.fields.skinUnexposedTemperature += partThermalData.unexpRadiationFlux * num3;
        part.fields.thermalRadiationFlux = partThermalData.radiationFlux + partThermalData.unexpRadiationFlux;
        if (IsNaN(part.fields.temperature)) part.fields.temperature = partThermalData.previousTemperature;
        if (IsNaN(part.fields.skinTemperature)) part.fields.skinTemperature = partThermalData.previousSkinTemperature;
        if (IsNaN(part.fields.skinUnexposedTemperature))
            part.fields.skinUnexposedTemperature = partThermalData.previousSkinUnexposedTemperature;
        if (averageWithPrevious) {
            part.fields.temperature = (partThermalData.previousTemperature + part.fields.temperature) * 0.5;
            part.fields.skinTemperature = (part.fields.skinTemperature + partThermalData.previousSkinTemperature) * 0.5;
            part.fields.skinUnexposedTemperature =
                (part.fields.skinUnexposedTemperature + partThermalData.previousSkinUnexposedTemperature) * 0.5;
        }
        if (part.fields.temperature < cacheSpaceTemperature) part.fields.temperature = cacheSpaceTemperature;
        if (part.fields.skinTemperature < cacheSpaceTemperature) part.fields.skinTemperature = cacheSpaceTemperature;
        if (part.fields.skinUnexposedTemperature < cacheSpaceTemperature) part.fields.skinUnexposedTemperature = cacheSpaceTemperature;
        // if (GameSettings.FI_LOG_OVERTEMP && over max temp) MonoBehaviour.print(...)
    }
}

// FlightIntegrator.cs:1308
double FlightIntegratorThermal::CalculateAnalyticTemperature() {
    const double num = 1.0 / fTimeSinceThermo;
    double num2 = 0.0;
    double num3 = 0.0;
    double num4 = 0.0;
    double num5 = 0.0;
    double num6 = 0.0;
    double num7 = 0.0;
    double num8 = 0.0;
    double num9 = std::numeric_limits<double>::max();
    double num10 = 0.0;
    double num11 = 0.0;
    if (partThermalDataCount == 0) UpdateThermalGraph();
    for (int i = 0; i < partThermalDataCount; i++) {
        Part& part = *partThermalDataList[i]->part;
        if (IsNaN(part.fields.temperature)) part.fields.temperature = cacheSpaceTemperature;
        if (IsNaN(part.fields.skinTemperature)) part.fields.skinTemperature = cacheSpaceTemperature;
        num11 = M::Max(num11, M::Max(part.fields.skinTemperature, part.fields.temperature));
        part.fields.thermalInternalFluxPrevious = part.fields.thermalInternalFlux * num;
        num2 += part.fields.thermalInternalFluxPrevious;
        part.fields.thermalInternalFlux = 0.0;
        part.fields.thermalSkinFluxPrevious = part.fields.thermalSkinFlux * num;
        num2 += part.fields.thermalSkinFluxPrevious;
        part.fields.thermalSkinFlux = 0.0;
        part.fields.thermalExposedFluxPrevious = part.fields.thermalExposedFlux * num;
        num2 += part.fields.thermalExposedFluxPrevious;
        part.fields.thermalExposedFlux = 0.0;
        double num12 = 0.0;
        part.fields.skinToInternalFlux = 0.0;
        const double thermalRadiationFlux = num12;
        num12 = 0.0;
        part.fields.thermalRadiationFlux = thermalRadiationFlux;
        const double thermalConvectionFlux = num12;
        num12 = 0.0;
        part.fields.thermalConvectionFlux = thermalConvectionFlux;
        part.fields.thermalConductionFlux = num12;
        if (!part.ShieldedFromAirstream()) {
            num3 += GetSunArea(*partThermalDataList[i]) * part.fields.absorptiveConstant;
            num4 += GetBodyArea(*partThermalDataList[i]) * part.fields.absorptiveConstant;
            num5 += part.fields.radiativeArea;
            num6 += part.fields.radiativeArea * part.fields.emissiveConstant;
            num7 += part.fields.radiativeArea * part.fields.absorptiveConstant;
        }
        num8 += part.fields.thermalMass + part.fields.skinThermalMass;
        if (part.fields.maxTemp < num9) num9 = part.fields.maxTemp;
    }
    if (num6 <= 0.0) num6 = 0.01;
    if (num7 <= 0.0) num7 = 0.01;
    if (num5 < num6) num5 = num6;
    const double num13 = num7 / num5;
    double num14 = 0.0;
    double num15 = backgroundRadiationTemp;
    num15 *= num15;
    num15 *= num15;
    num15 *= num13;
    double num16 = 0.0;
    if (vessel->directSunlight) num16 = num3 * atm.solarFlux * cacheRadiationFactor;
    double num17 = bodyEmissiveFlux + bodyAlbedoFlux;
    if (num17 > 0.0) num17 = num4 * UtilMath::Lerp(0.0, num17, atm.densityThermalLerp) * PhysicsGlobals::RadiationFactor();
    const double num18 = num16 + num17;
    int count = static_cast<int>(overheatModules.size());
    while (count-- > 0) num10 += overheatModules[count]->GetFlux();
    double num19 = 0.0;
    int count2 = static_cast<int>(previewModules.size());
    while (count2-- > 0) {
        IAnalyticPreview* analyticPreview = previewModules[count2];
        analyticPreview->AnalyticInfo(*this, num18, backgroundRadiationTemp, num6, num13, num2, atm.convectiveCoefficient,
                                      atm.atmosphereTemperatureOffset, num11);
        num19 += analyticPreview->InternalFluxAdjust();
    }
    num2 += num19;
    const double num20 = 1.0 / (cacheStefanBoltzmanConstant * num6 * cacheRadiationFactor);
    const double num21 = (num18 + num2 * 1000.0) * num20 + num15;
    double num22 = cacheSpaceTemperature;
    num22 *= num22;
    num22 *= num22;
    num14 = (!(num21 > num22)) ? cacheSpaceTemperature : M::Sqrt(M::Sqrt(num21));
    double t = 1.0;
    if (atm.density > 0.0) {
        const double num23 = num5 / num8;
        t = 1.0 / ((PhysicsGlobals::AnalyticConvectionSensitivityBase() + num23) * PhysicsGlobals::AnalyticConvectionSensitivityFinal() *
                   atm.convectiveCoefficient);
        num14 = UtilMath::Lerp(atm.atmosphericTemperature, num14, t);
    }
    if (num14 > num9 - 3.0) {
        double num24 = M::Sqrt(M::Sqrt(num21 - num10));
        if (atm.density > 0.0) num24 = UtilMath::Lerp(atm.atmosphericTemperature, num24, t);
        if (num24 < num9 - 10.0) {
            double num25 = num9 - 10.0;
            num25 *= num25;
            num25 *= num25;
            num25 = num21 - num25;
            num25 /= num10;
            const int count3 = static_cast<int>(overheatModules.size());
            for (int j = 0; j < count3; j++) overheatModules[j]->OnOverheat(num25);
            num14 = num9 - 10.0;
        }
    }
    return num14;
}

// FlightIntegrator.cs:1450
void FlightIntegratorThermal::PrecalcConduction(PartThermalData& ptd) {
    Part& part = *ptd.part;
    ptd.conductionMult = cacheConductionFactor * 10.0 * part.fields.heatConductivity;
    ptd.skinSkinTransfer = part.fields.skinSkinConductionMult * cacheSkinSkinConductionFactor *
                           M::Min(part.fields.skinExposedAreaFrac, 1.0 - part.fields.skinExposedAreaFrac) * 2.0 *
                           M::Sqrt(part.fields.radiativeArea);
}

// FlightIntegrator.cs:1631-1738 (the per-part block of the skin loop: no neighbour is read)
void FlightIntegratorThermal::UpdateSkinLocalConduction(PartThermalData& partThermalData, double skinInternalConductionFactor) {
    Part& part = *partThermalData.part;
    if (!(part.fields.radiativeArea > 0.0)) return;
    const double num14 = partThermalData.conductionMult * 0.5;
    const double num15 = part.fields.temperature * part.fields.thermalMass;
    double num16;
    double num22;
    if (partThermalData.exposed && part.fields.skinExposedAreaFrac < 1.0) {
        num16 = part.fields.skinTemperature - part.fields.skinUnexposedTemperature;
        if (num16 > 0.001 || num16 < -0.001) {
            const double num17 = num14 * partThermalData.skinSkinTransfer;
            double val = 1.0;
            double val2 = 1.0;
            const double num18 = (0.0 - num16) * num17 * part.fields.skinThermalMassRecip * part.fields.skinExposedMassMult;
            const double num19 = num16 * num17 * part.fields.skinThermalMassRecip * part.fields.skinUnexposedMassMult;
            const double num20 = part.fields.skinTemperature + num18 - partThermalData.unifiedTemp;
            const double num21 = part.fields.skinUnexposedTemperature + num19 - partThermalData.unifiedTemp;
            if (num16 > 0.0) {
                if (num20 < 0.0) val = (num18 - num20) / num18;
                if (num21 > 0.0) val2 = (num19 - num21) / num19;
            } else {
                if (num20 > 0.0) val = (num18 - num20) / num18;
                if (num21 < 0.0) val2 = (num19 - num21) / num19;
            }
            num22 = M::Max(0.0, M::Min(val, val2));
            partThermalData.skinSkinConductionFlux = num17 * num22 * num16;
        }
        num16 = part.fields.skinUnexposedTemperature - part.fields.temperature;
        if (num16 > 0.001 || num16 < -0.001) {
            const double num17 = part.fields.radiativeArea * (1.0 - part.fields.skinExposedAreaFrac) * num14 * skinInternalConductionFactor *
                                 part.fields.skinInternalConductionMult;
            const double num23 = part.fields.skinThermalMass * (1.0 - part.fields.skinExposedAreaFrac);
            const double num24 = (num15 + part.fields.skinUnexposedTemperature * num23) / (part.fields.thermalMass + num23);
            const double num25 = (0.0 - num16) * num17 * part.fields.skinThermalMassRecip * part.fields.skinUnexposedMassMult;
            const double num26 = num16 * num17 * part.fields.thermalMassReciprocal;
            const double num27 = part.fields.skinUnexposedTemperature + num25 - num24;
            const double num28 = part.fields.temperature + num26 - num24;
            double val3 = 1.0;
            double val4 = 1.0;
            if (num16 > 0.0) {
                if (num27 < 0.0) val3 = (num25 - num27) / num25;
                if (num28 > 0.0) val4 = (num26 - num28) / num26;
            } else {
                if (num27 > 0.0) val3 = (num25 - num27) / num25;
                if (num28 < 0.0) val4 = (num26 - num28) / num26;
            }
            num22 = M::Max(0.0, M::Min(val4, val3));
            partThermalData.unexpSkinInternalConductionFlux = num17 * num22 * num16;
        }
    }
    num16 = part.fields.skinTemperature - part.fields.temperature;
    if (num16 > 0.001 || num16 < -0.001) {
        const double num17 = part.fields.skinExposedAreaFrac * part.fields.radiativeArea * num14 * skinInternalConductionFactor *
                             part.fields.skinInternalConductionMult;
        const double num29 = part.fields.skinThermalMass * part.fields.skinExposedAreaFrac;
        const double num30 = (num15 + part.fields.skinTemperature * num29) / (part.fields.thermalMass + num29);
        const double num25 = (0.0 - num16) * num17 * part.fields.skinThermalMassRecip * part.fields.skinExposedMassMult;
        const double num26 = num16 * num17 * part.fields.thermalMassReciprocal;
        const double num27 = part.fields.skinTemperature + num25 - num30;
        const double num28 = part.fields.temperature + num26 - num30;
        double val3 = 1.0;
        double val4 = 1.0;
        if (num16 > 0.0) {
            if (num27 < 0.0) val3 = (num25 - num27) / num25;
            if (num28 > 0.0) val4 = (num26 - num28) / num26;
        } else {
            if (num27 > 0.0) val3 = (num25 - num27) / num25;
            if (num28 < 0.0) val4 = (num26 - num28) / num26;
        }
        num22 = M::Max(0.0, M::Min(val4, val3));
        partThermalData.skinInteralConductionFlux = num17 * num22 * num16;
    }
}

// FlightIntegrator.cs:1457
void FlightIntegratorThermal::UpdateConduction() {
    bool flag = false;
    bool flag2 = false;
    int num = partThermalDataCount - 1;
    while (true) {
        PartThermalData& partThermalData = *partThermalDataList[num];
        partThermalData.unexpSkinInternalConductionFlux = 0.0;
        partThermalData.skinInteralConductionFlux = 0.0;
        partThermalData.skinSkinConductionFlux = 0.0;
        partThermalData.localIntConduction = 0.0;
        partThermalData.intConductionFlux = 0.0;
        partThermalData.localSkinConduction = 0.0;
        partThermalData.skinConductionFlux = 0.0;
        partThermalData.unifiedTemp = partThermalData.GetUnifiedSkinTemp();
        num--;
        if (num < 0) break;
        if (flag || partThermalData.part->fields.temperature < partThermalDataList[num]->part->fields.temperature) flag = true;
        if (flag2 || partThermalDataListSkin[num + 1]->unifiedTemp < partThermalDataListSkin[num]->unifiedTemp) flag2 = true;
    }
    if (flag) {
        dotnet::ListSort(partThermalDataList, PartThermalData::IntCompare);
    }
    for (int num3 = partThermalDataCount - 2; num3 >= 0; num3--) {
        PartThermalData& partThermalData = *partThermalDataList[num3];
        Part& part = *partThermalData.part;
        double num4 = part.fields.thermalMass * part.fields.temperature;
        double num5 = part.fields.thermalMass;
        int thermalLinkCount = partThermalData.thermalLinkCount;
        while (thermalLinkCount-- > 0) {
            ThermalLink& thermalLink = *partThermalData.thermalLinks[thermalLinkCount];
            PartThermalData* ptd = thermalLink.remotePart->ptd;
            if (ptd == nullptr) continue;
            thermalLink.temperatureDelta = thermalLink.remotePart->fields.temperature - part.fields.temperature;
            if (thermalLink.temperatureDelta > 0.0) {
                num4 += thermalLink.remotePart->fields.thermalMass * thermalLink.remotePart->fields.temperature;
                num5 += thermalLink.remotePart->fields.thermalMass;
                double num6 = partThermalData.conductionMult * thermalLink.remotePart->fields.heatConductivity;
                if (part.ShieldedFromAirstream() != thermalLink.remotePart->ShieldedFromAirstream()) {
                    num6 *= PhysicsGlobals::ShieldedConductionFactor();
                }
                const double num7 = thermalLink.contactArea * num6 * thermalLink.temperatureDelta;
                ptd->localIntConduction -= num7;
                partThermalData.localIntConduction += num7;
            }
        }
        if (partThermalData.localIntConduction != 0.0) {
            const double num8 = num4 / num5;
            double num9 = 1.0;
            double num10 = partThermalData.localIntConduction * part.fields.thermalMassReciprocal;
            double num11 = part.fields.temperature + num10 - num8;
            if (num11 > num10) {
                num9 = 0.0;
            } else if (num11 > 0.0 && num10 > 0.0) {
                num9 = (num10 - num11) / num10;
                partThermalData.localIntConduction *= num9;
            }
            int thermalLinkCount2 = partThermalData.thermalLinkCount;
            while (thermalLinkCount2-- > 0) {
                ThermalLink& thermalLink = *partThermalData.thermalLinks[thermalLinkCount2];
                PartThermalData* ptd = thermalLink.remotePart->ptd;
                if (ptd == nullptr) continue;
                ptd->localIntConduction *= num9;
                if (ptd->localIntConduction < 0.0) {
                    num10 = ptd->localIntConduction * thermalLink.remotePart->fields.thermalMassReciprocal;
                    num11 = thermalLink.remotePart->fields.temperature + num10 - num8;
                    if (num11 < 0.0 && num10 < 0.0) {
                        const double num12 = num11 * thermalLink.remotePart->fields.thermalMass;
                        if (ptd->localIntConduction < num12) {
                            partThermalData.localIntConduction += num12;
                            ptd->localIntConduction -= num12;
                        } else {
                            partThermalData.localIntConduction += ptd->localIntConduction;
                            ptd->localIntConduction = 0.0;
                        }
                    }
                }
                ptd->intConductionFlux += ptd->localIntConduction;
                ptd->localIntConduction = 0.0;
            }
            if (partThermalData.localIntConduction > 0.0) partThermalData.intConductionFlux += partThermalData.localIntConduction;
            partThermalData.localIntConduction = 0.0;
        }
    }
    if (flag2) {
        dotnet::ListSort(partThermalDataListSkin, PartThermalData::SkinCompare);
    }
    const double skinInternalConductionFactor = PhysicsGlobals::SkinInternalConductionFactor();
    int num13 = partThermalDataCount;
    while (num13-- > 0) {
        PartThermalData& partThermalData = *partThermalDataListSkin[num13];
        Part& part = *partThermalData.part;
        if (!(part.fields.radiativeArea > 0.0)) continue;
        const double num14 = partThermalData.conductionMult * 0.5;
        UpdateSkinLocalConduction(partThermalData, skinInternalConductionFactor);
        if (num13 >= partThermalDataCount - 1) continue;
        double num31 = part.fields.skinThermalMass * partThermalData.unifiedTemp;
        double num32 = part.fields.skinThermalMass;
        int thermalLinkCount3 = partThermalData.thermalLinkCount;
        while (thermalLinkCount3-- > 0) {
            ThermalLink& thermalLink = *partThermalData.thermalLinks[thermalLinkCount3];
            PartThermalData* ptd = thermalLink.remotePart->ptd;
            if (ptd == nullptr) continue;
            if (part.ShieldedFromAirstream() != thermalLink.remotePart->ShieldedFromAirstream()) continue;
            const double num33 = ptd->unifiedTemp - partThermalData.unifiedTemp;
            if (!(num33 <= 0.0) &&
                !(part.fields.skinTemperature >= M::Max(ptd->part->fields.skinTemperature, ptd->part->fields.skinUnexposedTemperature))) {
                num31 += thermalLink.remotePart->fields.skinThermalMass * ptd->unifiedTemp;
                num32 += thermalLink.remotePart->fields.skinThermalMass;
                const double num34 = num14 * thermalLink.remotePart->fields.heatConductivity * part.fields.skinSkinConductionMult *
                                     thermalLink.remotePart->fields.skinSkinConductionMult * cacheSkinSkinConductionFactor *
                                     thermalLink.contactAreaSqrt * 200.0 * num33;
                ptd->localSkinConduction = 0.0 - num34;
                partThermalData.localSkinConduction += num34;
            }
        }
        if (partThermalData.localSkinConduction == 0.0) continue;
        const double num35 = num31 / num32;
        double num22 = 1.0;
        double num36 = partThermalData.localSkinConduction * part.fields.skinThermalMassRecip;
        double num37 = partThermalData.unifiedTemp + num36 - num35;
        if (num37 > num36) {
            num22 = 0.0;
        } else if (num37 > 0.0 && num36 > 0.0) {
            num22 = (num36 - num37) / num36;
            partThermalData.localSkinConduction *= num22;
        }
        int thermalLinkCount4 = partThermalData.thermalLinkCount;
        while (thermalLinkCount4-- > 0) {
            ThermalLink& thermalLink = *partThermalData.thermalLinks[thermalLinkCount4];
            PartThermalData* ptd = thermalLink.remotePart->ptd;
            if (ptd == nullptr) continue;
            ptd->localSkinConduction *= num22;
            if (ptd->localSkinConduction < 0.0) {
                num36 = ptd->localSkinConduction * thermalLink.remotePart->fields.skinThermalMassRecip;
                num37 = ptd->unifiedTemp + num36 - num35;
                if (num37 < 0.0 && num36 < 0.0) {
                    const double num38 = num37 * thermalLink.remotePart->fields.skinThermalMass;
                    if (ptd->localSkinConduction < num38) {
                        partThermalData.localSkinConduction += num38;
                        ptd->localSkinConduction -= num38;
                    } else {
                        partThermalData.localSkinConduction += ptd->localSkinConduction;
                        ptd->localSkinConduction = 0.0;
                    }
                }
            }
            ptd->skinConductionFlux += ptd->localSkinConduction;
            ptd->localSkinConduction = 0.0;
        }
        if (partThermalData.localSkinConduction > 0.0) partThermalData.skinConductionFlux += partThermalData.localSkinConduction;
        partThermalData.localSkinConduction = 0.0;
    }
}

// FlightIntegrator.cs:1849
void FlightIntegratorThermal::UnifySkinTemp(PartThermalData& ptd) {
    Part& part = *ptd.part;
    part.fields.skinTemperature = ptd.GetUnifiedSkinTemp();
    ptd.exposed = false;
    part.fields.skinExposedArea = part.fields.radiativeArea;
    double skinUnexposedMassMult = 0.0;
    part.fields.skinUnexposedTemperature = 0.0;
    part.fields.skinUnexposedMassMult = skinUnexposedMassMult;
    skinUnexposedMassMult = 1.0;
    part.fields.skinExposedMassMult = 1.0;
    part.fields.skinExposedAreaFrac = skinUnexposedMassMult;
}

// FlightIntegrator.cs:1863
void FlightIntegratorThermal::UpdateThermalGraph() {
    std::map<const Part*, double> dictionary;
    int index = partThermalDataCount;
    while (index-- > 0) {
        PartThermalData& partThermalData = *partThermalDataList[index];
        Part& part = *partThermalData.part;
        const double value = (!partThermalData.exposed) ? 0.0 : part.fields.skinExposedArea;
        dictionary[&part] = value;
    }
    partThermalDataList.clear();
    partThermalDataListSkin.clear();
    partThermalDataCount = 0;
    occlusionConv.clear();
    occlusionSun.clear();
    occlusionBody.clear();
    overheatModules.clear();
    previewModules.clear();
    compoundParts.clear();
    ptdStore.clear();  // the old PartThermalData objects are garbage in the C# once the lists are cleared (part.ptd is overwritten below)
    for (int i = 0; i < partCount; i++) {
        Part& part2 = *vessel->parts[i];
        part2.DragCubes.RequestOcclusionUpdate();
        if (part2.fields.thermalMass == 0.0) continue;
        ptdStore.push_back(std::make_unique<PartThermalData>(part2));
        PartThermalData* partThermalData2 = ptdStore.back().get();
        part2.ptd = partThermalData2;
        overheatModules.insert(overheatModules.end(), partThermalData2->overheatModules.begin(), partThermalData2->overheatModules.end());
        previewModules.insert(previewModules.end(), partThermalData2->analyticPreviewModules.begin(),
                              partThermalData2->analyticPreviewModules.end());
        auto it = dictionary.find(&part2);
        part2.fields.skinExposedArea = (it != dictionary.end()) ? it->second : 0.0;  // TryGetValue(part2, out part2.skinExposedArea)
        if (part2.fields.skinExposedArea > 0.0) partThermalData2->exposed = true;
        if (part2.fields.skinTemperature < PhysicsGlobals::SpaceTemperature()) part2.fields.skinTemperature = part2.fields.temperature;
        if (part2.parent != nullptr && part2.parent->fields.thermalMass != 0.0) {
            partThermalData2->thermalLinks.push_back(std::make_unique<ThermalLink>(part2, *part2.parent));
            partThermalData2->thermalLinkCount++;
        }
        const int count = static_cast<int>(part2.children.size());
        for (int j = 0; j < count; j++) {
            Part* part3 = part2.children[j];
            if (part3->fields.thermalMass != 0.0) {
                partThermalData2->thermalLinks.push_back(std::make_unique<ThermalLink>(part2, *part3));
                partThermalData2->thermalLinkCount++;
            }
        }
        if (part2.isCompund()) compoundParts.push_back(&part2);
        partThermalDataList.push_back(partThermalData2);
        partThermalDataListSkin.push_back(partThermalData2);
        occlusionConv.push_back(&partThermalData2->convectionData);
        occlusionSun.push_back(&partThermalData2->sunData);
        occlusionBody.push_back(&partThermalData2->bodyData);
        partThermalDataCount++;
    }
    hasCompoundParts = !compoundParts.empty();
    if (!hasCompoundParts) compoundParts.clear();
    occludersConvection.assign(static_cast<size_t>(partThermalDataCount), nullptr);
    occludersSun.assign(static_cast<size_t>(partThermalDataCount), nullptr);
    occludersBody.assign(static_cast<size_t>(partThermalDataCount), nullptr);
}

// FlightIntegrator.cs:1958
void FlightIntegratorThermal::UpdateCompoundParts() {
    if (!hasCompoundParts) return;
    compoundPartsTimer++;
    if (compoundPartsTimer <= 2) return;
    int count = static_cast<int>(compoundParts.size());
    while (count-- > 0) {
        Part* compoundPart = compoundParts[count];
        Part& part = *compoundPart;
        PartThermalData* ptd = part.ptd;
        if (ptd == nullptr) continue;
        Part* target = compoundTarget ? compoundTarget(part) : nullptr;
        if (target != nullptr) {
            PartThermalData* ptd2 = target->ptd;
            if (ptd2 != nullptr) {
                ptd2->thermalLinks.push_back(std::make_unique<ThermalLink>(*target, part));
                ptd2->thermalLinkCount++;
                ptd->thermalLinks.push_back(std::make_unique<ThermalLink>(part, *target));
                ptd->thermalLinkCount++;
            }
        }
    }
    compoundPartsTimer = 0;
    compoundParts.clear();
    hasCompoundParts = false;
}

// FlightIntegrator.cs:1996
void FlightIntegratorThermal::SetSkinProperties(PartThermalData& ptd) {
    Part& part = *ptd.part;
    if (part.fields.skinUnexposedTemperature < cacheSpaceTemperature) part.fields.skinUnexposedTemperature = part.fields.skinTemperature;
    const double num = M::Max(part.fields.radiativeArea, 0.001);
    ptd.radAreaRecip = 1.0 / num;
    if (num == 0.0) ptd.radAreaRecip = 1.0;
    bool flag = false;
    double num2 = 1.0;
    if (!part.ShieldedFromAirstream() && part.fields.atmDensity > 0.0) {
        ptd.convectionArea =
            UtilMath::Lerp(num, part.fields.exposedArea,
                           0.0 - cacheFullConvectionAreaMin + (atm.mach - cacheFullToCrossSectionLerpStart) /
                                                                  (cacheFullToCrossSectionLerpEnd - cacheFullToCrossSectionLerpStart)) *
            ptd.convectionAreaMultiplier;
        const double num3 = ptd.convectionArea * ptd.radAreaRecip;
        if (num3 < 0.999 && !IsNaN(num3)) {
            if (num3 > 0.001) {
                num2 = num3;
                flag = true;
            } else {
                num2 = 0.0;
            }
        } else {
            num2 = 1.0;
            flag = true;
        }
    }
    if (flag) {
        if (!ptd.exposed || num2 == 1.0) {
            part.fields.skinUnexposedTemperature = part.fields.skinTemperature;
            ptd.exposed = true;
        }
        part.fields.skinExposedMassMult = 1.0 / num2;
        if (num2 < 1.0)
            part.fields.skinUnexposedMassMult = 1.0 / (1.0 - num2);
        else
            part.fields.skinUnexposedMassMult = 0.0;
        if (part.fields.skinExposedAreaFrac != num2 && part.fields.skinUnexposedTemperature != part.fields.skinTemperature &&
            part.fields.skinExposedAreaFrac > 0.0 && part.fields.skinExposedAreaFrac < 1.0 && num2 < 1.0) {
            const double num4 = 1.0 - part.fields.skinExposedAreaFrac;
            const double num5 = part.fields.skinTemperature * part.fields.skinExposedAreaFrac * part.fields.skinThermalMass;
            const double num6 = part.fields.skinUnexposedTemperature * num4 * part.fields.skinThermalMass;
            const double num7 = num2 - part.fields.skinExposedAreaFrac;
            double num8 = 0.0;
            num8 = (!(num7 > 0.0)) ? (num7 / part.fields.skinExposedAreaFrac * num5) : (num7 / num4 * num6);
            part.fields.skinTemperature = (num5 + num8) * part.fields.skinExposedMassMult * part.fields.skinThermalMassRecip;
            part.fields.skinUnexposedTemperature = (num6 - num8) * part.fields.skinUnexposedMassMult * part.fields.skinThermalMassRecip;
        }
        part.fields.skinExposedAreaFrac = num2;
        part.fields.skinExposedArea = num2 * num;
    } else {
        if (ptd.exposed) UnifySkinTemp(ptd);
        ptd.exposed = false;
        double num9 = 0.0;
        part.fields.skinUnexposedMassMult = 0.0;
        const double skinExposedArea = num9;
        num9 = 0.0;
        part.fields.skinExposedArea = skinExposedArea;
        ptd.convectionArea = num9;
        num9 = 1.0;
        part.fields.skinExposedMassMult = 1.0;
        part.fields.skinExposedAreaFrac = num9;
    }
}

// FlightIntegrator.cs:2090
void FlightIntegratorThermal::PrecalcConvection(PartThermalData& ptd) {
    if (ptd.convectionArea <= 0.0) return;
    Part& part = *ptd.part;
    ptd.postShockExtTemp = UtilMath::LerpUnclamped(atm.atmosphericTemperature, atm.externalTemperature, ptd.convectionTempMultiplier);
    ptd.finalCoeff = atm.convectiveCoefficient * ptd.convectionArea * 0.001 * part.fields.heatConvectiveConstant * ptd.convectionCoeffMultiplier;
    ptd.partPseudoRe = atm.pseudoReynolds * static_cast<double>(part.DragCubes.DragCoeff());
    if (ptd.partPseudoRe > cacheTurbulentConvectionStart) {
        if (ptd.partPseudoRe > cacheTurbulentConvectionEnd)
            ptd.finalCoeff *= cacheTurbulentConvectionMult;
        else
            ptd.finalCoeff *= UtilMath::LerpUnclamped(1.0, cacheTurbulentConvectionMult,
                                                      (ptd.partPseudoRe - cacheTurbulentConvectionStart) * atm.pseudoReLerpTimeMult);
    }
    ptd.finalCoeff = M::Min(ptd.finalCoeff, part.fields.skinThermalMass * part.fields.skinExposedAreaFrac);
}

// FlightIntegrator.cs:2114
void FlightIntegratorThermal::UpdateConvection(PartThermalData& ptd) {
    if (!(ptd.convectionArea <= 0.0)) ptd.convectionFlux = (ptd.postShockExtTemp - ptd.part->fields.skinTemperature) * ptd.finalCoeff;
}

// FlightIntegrator.cs:2122
void FlightIntegratorThermal::PrecalcRadiation(PartThermalData& ptd) {
    Part& part = *ptd.part;
    const double num = cacheRadiationFactor * 0.001;
    ptd.emissScalar = part.fields.emissiveConstant * num;
    ptd.absorbScalar = part.fields.absorptiveConstant * num;
    ptd.sunFlux = 0.0;
    ptd.bodyFlux = bodyEmissiveFlux + bodyAlbedoFlux;
    const double num2 = part.fields.radiativeArea * (1.0 - part.fields.skinExposedAreaFrac);
    ptd.expFlux = 0.0;
    ptd.unexpFlux = 0.0;
    ptd.brtUnexposed = backgroundRadiationTemp;
    ptd.brtExposed = UtilMath::Lerp(backgroundRadiationTemp, backgroundRadiationTempExposed, ptd.convectionTempMultiplier);
    PrecalcRadiationIncoming(ptd, num2);
}

// FlightIntegrator.cs:2138-2180 (the tail of PrecalcRadiation)
void FlightIntegratorThermal::PrecalcRadiationIncoming(PartThermalData& ptd, double num2) {
    Part& part = *ptd.part;
    if (part.ShieldedFromAirstream()) return;
    if (vessel->directSunlight && !part.ShieldedFromAirstream()) {
        const double sunArea = GetSunArea(ptd);
        if (sunArea > 0.0) {
            ptd.sunFlux = ptd.absorbScalar * atm.solarFlux;
            if (ptd.exposed) {
                const double num3 = (static_cast<double>(Vector3f::Dot(sunVector, nVel)) + 1.0) * 0.5;
                const double num4 = M::Min(sunArea, part.fields.skinExposedArea * num3);
                const double num5 = M::Min(sunArea - num4, num2 * (1.0 - num3));
                ptd.expFlux += ptd.sunFlux * num4;
                ptd.unexpFlux += ptd.sunFlux * num5;
            } else {
                ptd.expFlux += ptd.sunFlux * sunArea;
            }
        }
    }
    if (!(ptd.bodyFlux > 0.0) || part.ShieldedFromAirstream()) return;
    const double bodyArea = GetBodyArea(ptd);
    if (bodyArea > 0.0) {
        ptd.bodyFlux = UtilMath::Lerp(0.0, ptd.bodyFlux, atm.densityThermalLerp) * ptd.absorbScalar;
        if (ptd.exposed) {
            // double num6 = (Vector3.Dot(-vessel.upAxis, nVel) + 1f) * 0.5f: float operands, but the expression is widened to
            // double without a float store (Mono rule: no rounding point), Dot itself returns a float
            const float dotUp = Vector3f::Dot((-vessel->upAxis).ToVector3f(), nVel);
            const double num6 = (fx(dotUp) + fx(1.0f)) * fx(0.5f);
            const double num7 = M::Min(bodyArea, part.fields.skinExposedArea * num6);
            const double num8 = M::Min(bodyArea - num7, num2 * (1.0 - num6));
            ptd.expFlux += ptd.bodyFlux * num7;
            ptd.unexpFlux += ptd.bodyFlux * num8;
        } else {
            ptd.expFlux += ptd.bodyFlux * bodyArea;
        }
    }
}

// FlightIntegrator.cs:2182
void FlightIntegratorThermal::UpdateRadiation(PartThermalData& ptd) {
    Part& part = *ptd.part;
    double skinTemperature = part.fields.skinTemperature;
    double skinUnexposedTemperature = part.fields.skinUnexposedTemperature;
    const double num = part.fields.radiativeArea * part.fields.skinExposedAreaFrac;
    const double num2 = part.fields.radiativeArea * (1.0 - part.fields.skinExposedAreaFrac);
    if (!part.ShieldedFromAirstream()) {
        if (skinUnexposedTemperature > 0.0 && num2 > 0.0) {
            double brtUnexposed = ptd.brtUnexposed;
            brtUnexposed *= brtUnexposed;
            brtUnexposed *= brtUnexposed;
            skinUnexposedTemperature *= skinUnexposedTemperature;
            skinUnexposedTemperature *= skinUnexposedTemperature;
            ptd.unexpFlux -= (skinUnexposedTemperature - brtUnexposed) * cacheStefanBoltzmanConstant * ptd.emissScalar * num2;
        }
        if (skinTemperature > 0.0 && num > 0.0) {
            double brtExposed = ptd.brtExposed;
            brtExposed *= brtExposed;
            brtExposed *= brtExposed;
            skinTemperature *= skinTemperature;
            skinTemperature *= skinTemperature;
            ptd.expFlux -= (skinTemperature - brtExposed) * cacheStefanBoltzmanConstant * ptd.emissScalar * num;
        }
        ptd.radiationFlux += ptd.expFlux;
        ptd.unexpRadiationFlux += ptd.unexpFlux;
    }
}

// FlightIntegrator.cs:2214
double FlightIntegratorThermal::GetSunArea(PartThermalData& ptd) {
    Part& part = *ptd.part;
    if (part.DragCubes.None()) return 0.0;
    const Vector3f dir = part.partTransform.InverseTransformDirection(sunVector);
    return static_cast<double>(part.DragCubes.GetCubeAreaDir(dir)) * ptd.sunAreaMultiplier;
}

// FlightIntegrator.cs:2225
double FlightIntegratorThermal::GetBodyArea(PartThermalData& ptd) {
    Part& part = *ptd.part;
    if (part.DragCubes.None()) return 0.0;
    const Vector3f dir = part.partTransform.InverseTransformDirection((-vessel->upAxis).ToVector3f());
    return static_cast<double>(part.DragCubes.GetCubeAreaDir(dir)) * ptd.bodyAreaMultiplier;
}

// FlightIntegrator.cs:2236
void FlightIntegratorThermal::SetSkinThermalMass(Part& part) {
    const double num = cacheStandardSpecificHeatCapacity * part.fields.thermalMassModifier;
    part.fields.skinThermalMass =
        M::Max(0.1, M::Min(0.001 * part.fields.skinMassPerArea * part.fields.skinThermalMassModifier * part.fields.radiativeArea * num,
                           static_cast<double>(part.fields.mass) * num * 0.5));
    part.fields.skinThermalMassRecip = 1.0 / part.fields.skinThermalMass;
}

// ---------------------------------------------------------------------------------------------------------------------
// FlightIntegrator.cs:2545
void FlightIntegratorThermal::UpdateOcclusion(bool all) {
    if (all) {
        UpdateOcclusionConvection();
        UpdateOcclusionSolar();
        UpdateOcclusionBody();
        needOcclusion = false;
        return;
    }
    switch (occlusionCounter) {
        default:
            UpdateOcclusionBody();
            occlusionCounter = 0;
            break;
        case 1:
            UpdateOcclusionSolar();
            occlusionCounter++;
            break;
        case 0:
            UpdateOcclusionConvection();
            occlusionCounter++;
            break;
    }
}

namespace {
Matrix4x4f LocalToWorldOf(const FlightIntegratorThermal& fi, const Part& part) {
    return fi.localToWorld ? fi.localToWorld(part) : Matrix4x4f{};
}
}  // namespace

// FlightIntegrator.cs:2572
void FlightIntegratorThermal::UpdateOcclusionConvection() {
    bool flag = true;
    if (atm.mach > 1.0) flag = false;
    double num;
    if (flag) {
        if (wasMachConvectionEnabled) {
            for (int i = 0; i < partThermalDataCount; i++) {
                PartThermalData& a = *partThermalDataList[i];
                num = 1.0;
                a.convectionCoeffMultiplier = 1.0;
                const double convectionAreaMultiplier = num;
                num = 1.0;
                a.convectionAreaMultiplier = convectionAreaMultiplier;
                a.convectionTempMultiplier = num;
            }
            wasMachConvectionEnabled = false;
        }
        return;
    }
    bool flag2 = false;
    if (partThermalDataCount != static_cast<int>(partThermalDataList.size())) {
        recreateThermalGraph = true;
        CheckThermalGraph();
    }
    const int num2 = partThermalDataCount - 1;
    int num3 = partThermalDataCount;
    while (num3-- > 0) {
        occlusionConv[num3]->Update(Vector3f(nVel), LocalToWorldOf(*this, *occlusionConv[num3]->part));
        if (num3 < num2 && !flag2 && occlusionConv[num3 + 1]->maximumDot < occlusionConv[num3]->maximumDot) flag2 = true;
    }
    if (flag2) {
        dotnet::ListSort(occlusionConv, [](OcclusionData* a, OcclusionData* b) { return a->CompareTo(*b); });
    }
    const double num4 = M::Sqrt(atm.mach);
    const double num5 = M::Asin(1.0 / num4);
    const double num6 = 0.7957 * (1.0 - 1.0 / (atm.mach * num4));
    OcclusionData* occlusionData = occlusionConv[num2];
    {
        OcclusionThermalRef* ptd = occlusionData->ptd;
        PartThermalData* p = static_cast<PartThermalData*>(ptd);
        num = 1.0;
        p->convectionCoeffMultiplier = 1.0;
        const double convectionAreaMultiplier2 = num;
        num = 1.0;
        p->convectionAreaMultiplier = convectionAreaMultiplier2;
        p->convectionTempMultiplier = num;
    }
    occlusionData->convCone.Setup(*occlusionData, num4, num5, num6);
    occludersConvection[0] = &occlusionData->convCone;
    occludersConvectionCount = 1;
    // FXCamera.Instance.ApplyObliqueness((float)occludersConvection[0].shockAngle): camera effect, no thermal state
    int index = num2;
    while (index-- > 0) {
        occlusionData = occlusionConv[index];
        PartThermalData* p = static_cast<PartThermalData*>(occlusionData->ptd);
        num = 1.0;
        p->convectionCoeffMultiplier = 1.0;
        const double convectionAreaMultiplier3 = num;
        num = 1.0;
        p->convectionAreaMultiplier = convectionAreaMultiplier3;
        p->convectionTempMultiplier = num;
        for (int j = 0; j < occludersConvectionCount; j++) {
            const double shockStats =
                occlusionData->GetShockStats(*occludersConvection[j], p->convectionTempMultiplier, p->convectionCoeffMultiplier);
            p->convectionAreaMultiplier *= shockStats;
            if (!(p->convectionAreaMultiplier > 0.001)) {
                p->convectionAreaMultiplier = 0.0;
                break;
            }
        }
        if (p->convectionAreaMultiplier > 0.0) {
            const double num7 = 1.0;
            occlusionData->convCone.Setup(*occlusionData, num4 * num7, num5 * num7, num6 * num7);
            occludersConvection[occludersConvectionCount] = &occlusionData->convCone;
            occludersConvectionCount++;
        }
    }
}

// FlightIntegrator.cs:2670
void FlightIntegratorThermal::UpdateOcclusionSolar() {
    bool flag = false;
    if (partThermalDataCount != static_cast<int>(partThermalDataList.size())) {
        recreateThermalGraph = true;
        CheckThermalGraph();
    }
    const int num = partThermalDataCount - 1;
    int num2 = partThermalDataCount;
    while (num2-- > 0) {
        occlusionSun[num2]->Update(sunVector, LocalToWorldOf(*this, *occlusionSun[num2]->part));
        if (num2 < num && !flag && occlusionSun[num2 + 1]->maximumDot < occlusionSun[num2]->maximumDot) flag = true;
    }
    if (flag) {
        dotnet::ListSort(occlusionSun, [](OcclusionData* a, OcclusionData* b) { return a->CompareTo(*b); });
    }
    OcclusionData* occlusionData = occlusionSun[num];
    static_cast<PartThermalData*>(occlusionData->ptd)->sunAreaMultiplier = 1.0;
    occlusionData->sunCyl.Setup(*occlusionData);
    occludersSun[0] = &occlusionData->sunCyl;
    occludersSunCount = 1;
    int index = num;
    while (index-- > 0) {
        occlusionData = occlusionSun[index];
        PartThermalData* p = static_cast<PartThermalData*>(occlusionData->ptd);
        p->sunAreaMultiplier = 1.0;
        for (int i = 0; i < occludersSunCount; i++) {
            p->sunAreaMultiplier -= occlusionData->GetCylinderOcclusion(*occludersSun[i]);
            if (!(p->sunAreaMultiplier > 0.001)) {
                p->sunAreaMultiplier = 0.0;
                break;
            }
        }
        if (p->sunAreaMultiplier > 0.0) {
            occlusionData->sunCyl.Setup(*occlusionData);
            occludersSun[occludersSunCount] = &occlusionData->sunCyl;
            occludersSunCount++;
        }
    }
}

// FlightIntegrator.cs:2720
void FlightIntegratorThermal::UpdateOcclusionBody() {
    const Vector3f velocity = (-vessel->upAxis).ToVector3f();
    bool flag = false;
    if (partThermalDataCount != static_cast<int>(partThermalDataList.size())) {
        recreateThermalGraph = true;
        CheckThermalGraph();
    }
    const int num = partThermalDataCount - 1;
    int num2 = partThermalDataCount;
    while (num2-- > 0) {
        occlusionBody[num2]->Update(velocity, LocalToWorldOf(*this, *occlusionBody[num2]->part));
        if (num2 < num && !flag && occlusionBody[num2 + 1]->maximumDot < occlusionBody[num2]->maximumDot) flag = true;
    }
    if (flag) {
        dotnet::ListSort(occlusionBody, [](OcclusionData* a, OcclusionData* b) { return a->CompareTo(*b); });
    }
    OcclusionData* occlusionData = occlusionBody[num];
    static_cast<PartThermalData*>(occlusionData->ptd)->bodyAreaMultiplier = 1.0;
    occlusionData->bodyCyl.Setup(*occlusionData);
    occludersBody[0] = &occlusionData->bodyCyl;
    occludersBodyCount = 1;
    int index = num;
    while (index-- > 0) {
        occlusionData = occlusionBody[index];
        PartThermalData* p = static_cast<PartThermalData*>(occlusionData->ptd);
        p->bodyAreaMultiplier = 1.0;
        for (int i = 0; i < occludersBodyCount; i++) {
            p->bodyAreaMultiplier -= occlusionData->GetCylinderOcclusion(*occludersBody[i]);
            if (!(p->bodyAreaMultiplier > 0.001)) {
                p->bodyAreaMultiplier = 0.0;
                break;
            }
        }
        if (p->bodyAreaMultiplier > 0.0) {
            occlusionData->bodyCyl.Setup(*occlusionData);
            occludersBody[occludersBodyCount] = &occlusionData->bodyCyl;
            occludersBodyCount++;
        }
    }
}

}  // namespace ksp
