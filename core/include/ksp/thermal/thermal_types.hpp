// WP-22: the data and interface types of KSP's thermal model.
//   IAnalyticOverheatModule.cs, IAnalyticPreview.cs, IAnalyticTemperatureModifier.cs, IOverheatDisplay.cs   (interfaces)
//   Radiators/RadiatorData.cs                                                                              (struct)
//   ThermalLink.cs                                                                                         (class)
//   PartThermalData.cs, PartThermalDataIntComparer.cs, PartThermalDataSkinComparer.cs                      (class)
// A module that implements an interface exposes it through the PartModule::As...() query (the C# `module is IAnalytic...`).
#pragma once

#include <memory>
#include <vector>

#include "ksp/aero/occlusion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class FlightIntegratorThermal;

// IAnalyticOverheatModule.cs
class IAnalyticOverheatModule {
public:
    virtual ~IAnalyticOverheatModule() = default;
    virtual void OnOverheat(double amout = 1.0) = 0;
    virtual double GetFlux() = 0;
};

// IAnalyticPreview.cs
class IAnalyticPreview {
public:
    virtual ~IAnalyticPreview() = default;
    virtual void AnalyticInfo(FlightIntegratorThermal& fi, double sunAndBodyIn, double backgroundRadiation, double radArea,
                              double absEmissRatio, double internalFlux, double convCoeff, double ambientTemp,
                              double maxPartTemp) = 0;
    virtual double InternalFluxAdjust() = 0;
};

// IAnalyticTemperatureModifier.cs
class IAnalyticTemperatureModifier {
public:
    virtual ~IAnalyticTemperatureModifier() = default;
    virtual void SetAnalyticTemperature(FlightIntegratorThermal& fi, double analyticTemp, double toBeInternal, double toBeSkin) = 0;
    virtual double GetSkinTemperature(bool& lerp) = 0;
    virtual double GetInternalTemperature(bool& lerp) = 0;
};

// IOverheatDisplay.cs
class IOverheatDisplay {
public:
    virtual ~IOverheatDisplay() = default;
    virtual float GetHeatThrottle() = 0;
    virtual bool ModuleIsActive() = 0;
    virtual bool IsOverheating() = 0;
    virtual double GetCoreTemperature() = 0;
    virtual double GetGoalTemperature() = 0;
    virtual bool DisplayCoreHeat() = 0;
};

namespace Radiators {
// Radiators/RadiatorData.cs
struct RadiatorData {
    double Energy = 0.0;
    double MaxEnergy = 0.0;
    double EnergyCap = 0.0;
    Part* part = nullptr;  // RadiatorData.Part
};
}  // namespace Radiators

// ThermalLink.cs: the conduction link between two parts. The constructor reads the attach nodes of both parts
// (contactArea) and their world positions (partToNode / remoteToNode, normalised float vectors).
class ThermalLink {
public:
    Part* remotePart = nullptr;
    double contactArea = 0.0;
    double contactAreaSqrt = 0.0;
    double temperatureDelta = 0.0;
    Vector3f partToNode{};
    Vector3f remoteToNode{};
    bool useOrientation = false;

    ThermalLink(Part& part, Part& remotePart);  // ThermalLink.cs:21
};

// PartThermalData.cs. Derives from OcclusionThermalRef (WP-21's seam: part, convectionTempMultiplier, convectionCoeffMultiplier).
class PartThermalData : public OcclusionThermalRef {
public:
    int thermalLinkCount = 0;
    double bodyAreaMultiplier = 1.0;
    double sunAreaMultiplier = 1.0;
    double convectionAreaMultiplier = 1.0;
    // convectionTempMultiplier / convectionCoeffMultiplier: OcclusionThermalRef (initial 1.0)
    std::vector<std::unique_ptr<ThermalLink>> thermalLinks;
    std::vector<IAnalyticOverheatModule*> overheatModules;
    std::vector<IAnalyticPreview*> analyticPreviewModules;
    IAnalyticTemperatureModifier* analyticModifier = nullptr;
    OcclusionData convectionData;
    OcclusionData sunData;
    OcclusionData bodyData;
    double previousTemperature = 0.0;
    double previousSkinTemperature = 0.0;
    double previousSkinUnexposedTemperature = 0.0;
    bool exposed = false;
    double radAreaRecip = 1.0;
    double convectionArea = 0.0;
    double intConductionFlux = 0.0;
    double skinConductionFlux = 0.0;
    double localIntConduction = 0.0;
    double localSkinConduction = 0.0;
    double skinSkinConductionFlux = 0.0;
    double skinInteralConductionFlux = 0.0;
    double unexpSkinInternalConductionFlux = 0.0;
    double radiationFlux = 0.0;
    double unexpRadiationFlux = 0.0;
    double convectionFlux = 0.0;
    bool isEVA = false;
    double postShockExtTemp = 0.0;
    double finalCoeff = 0.0;
    double partPseudoRe = 0.0;
    double emissScalar = 0.0;
    double absorbScalar = 0.0;
    double absEmissRatio = 0.0;
    double sunFlux = 0.0;
    double bodyFlux = 0.0;
    double expFlux = 0.0;
    double unexpFlux = 0.0;
    double brtUnexposed = 0.0;
    double brtExposed = 0.0;
    int sCount = 0;
    int realSCount = 0;
    double sDivisor = 0.0;
    double conductionMult = 0.0;
    double skinSkinTransfer = 0.0;
    double unifiedTemp = 0.0;

    explicit PartThermalData(Part& part);  // PartThermalData.cs:104 (collects the thermal interfaces of the part's modules)
    PartThermalData(const PartThermalData&) = delete;
    PartThermalData& operator=(const PartThermalData&) = delete;

    double GetUnifiedSkinTemp() const;  // PartThermalData.cs:146

    // PartThermalDataIntComparer.cs / PartThermalDataSkinComparer.cs (the static comparer instances of the C# class)
    static int IntCompare(const PartThermalData* a, const PartThermalData* b);
    static int SkinCompare(const PartThermalData* a, const PartThermalData* b);
};

}  // namespace ksp
