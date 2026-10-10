// WP-22: the thermal part modules and helpers.
//   TemperatureModel.cs (internal class, no caller in the game: ported whole)
//   Radiators/RadiatorUtilities.cs
//   ModuleConductionMultiplier.cs
//   ModuleAblator.cs
//   ModuleCoreHeat.cs
//   ModuleActiveRadiator.cs
// KSPFields live in the WP-06 FieldStore (schema generated from the decompile): `Get<double>("name")`. Engine seams (named in
// each class): HighLogic.LoadedSceneIsFlight is ThermalModuleEnvironment::loadedSceneIsFlight, TimeWarp.fixedDeltaTime is
// ksp::TimeWarp::fixedDeltaTime(), Planetarium.GetUniversalTime() / Time.timeSinceLevelLoad / FlightGlobals.ready are the
// ThermalModuleEnvironment members; BaseConverter (ModuleCoreHeat's converters), ModuleDeployableRadiator (WP-27) and the
// ModuleResourceHandler of the radiator are hooks. Not ported: the debug-field GUI strings (D_*), PAW / localisation text
// (GetInfo, GetModuleDisplayName, status text beyond the percentage), adjusters (WP-55) and GameEvents wiring of the part caches
// (the cache is rebuilt when `partCachesDirty` is set).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/curves/float_curve.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/thermal/thermal_types.hpp"

namespace ksp {

class Part;
struct AttachNode;
class PartResourceDefinition;

// The statics the thermal modules read (HighLogic / Planetarium / Time / FlightGlobals)
struct ThermalModuleEnvironment {
    static inline bool loadedSceneIsFlight = true;  // HighLogic.LoadedSceneIsFlight
    static inline bool flightGlobalsReady = true;   // FlightGlobals.ready
    static inline double universalTime = 0.0;       // Planetarium.GetUniversalTime()
    static inline float timeSinceLevelLoad = 10.0f; // Time.timeSinceLevelLoad
};

// Radiators/RadiatorUtilities.cs
struct RadiatorUtilities {
    static Radiators::RadiatorData GetThermalData(Part& part);                                    // :7
    static void TransferEnergy(const Radiators::RadiatorData& src, const Radiators::RadiatorData& tgt);  // :17
};

// TemperatureModel.cs (float arithmetic under the Mono rule)
class TemperatureModel {
public:
    enum class TemperatureModels { LINEAR, QUADRATIC, CONSTANT, STEPPED };

    class Template {
    public:
        virtual ~Template() = default;
        virtual float getTemperatureAtAltitude(float) { return 0.0f; }
    };
    class Linear : public Template {
    public:
        Linear(float m_, float b_) : m(m_), b(b_) {}
        Linear(float temp1, float alt1, float temp2, float alt2);
        float getTemperatureAtAltitude(float altitude) override;
    private:
        float m, b;
    };
    class Quadratic : public Template {
    public:
        Quadratic(float A_, float B_, float C_) : A(A_), B(B_), C(C_) {}
        float getTemperatureAtAltitude(float altitude) override;
    private:
        float A, B, C;
    };
    class Constant : public Template {
    public:
        explicit Constant(float k) : K(k) {}
        float getTemperatureAtAltitude(float) override { return K; }
    private:
        float K;
    };
    class Stepped : public Template {
    public:
        struct TempRange {
            float minAlt = 0.0f, maxAlt = 0.0f;
            TemperatureModels modelName = TemperatureModels::LINEAR;
            std::shared_ptr<Template> modelData;
        };
        bool AddInitialLinearStepEquation(float min, float max, float m, float b);
        bool AddInitialLinearStep(float temp1, float alt1, float temp2, float alt2);
        bool AddInitialQuadraticStep(float min, float max, float A, float B, float C);
        bool AddInitialConstantStep(float min, float max, float K);
        bool AddConstantStep(float limit, float K);
        bool AddLinearStep(float limit, float temp2);
        bool AddQuadraticStep(float limit, float A, float B, float C);
        float getTemperatureAtAltitude(float altitude) override;
        void clearIntervals() { Steps.clear(); }
        const std::vector<TempRange>& steps() const { return Steps; }
    private:
        std::vector<TempRange> Steps;
    };

    TemperatureModel() = default;
    bool AddInitialConstantStep(float min, float max, float K) { return Intervals.AddInitialConstantStep(min, max, K); }
    bool AddInitialLinearStep(float t1, float a1, float t2, float a2) { return Intervals.AddInitialLinearStep(t1, a1, t2, a2); }
    bool AddInitialLinearStepEquation(float min, float max, float m, float b) { return Intervals.AddInitialLinearStepEquation(min, max, m, b); }
    bool AddInitialQuadraticStep(float min, float max, float A, float B, float C) { return Intervals.AddInitialQuadraticStep(min, max, A, B, C); }
    bool AddConstantStep(float limit, float K) { return Intervals.AddConstantStep(limit, K); }
    bool AddLinearStep(float limit, float temp2) { return Intervals.AddLinearStep(limit, temp2); }
    bool AddQuadraticStep(float limit, float A, float B, float C) { return Intervals.AddQuadraticStep(limit, A, B, C); }
    float getTemperatureAtAltitude(double altitude) { return Intervals.getTemperatureAtAltitude(static_cast<float>(altitude)); }  // :313 (private)

private:
    Stepped Intervals;
};

// ModuleConductionMultiplier.cs
class ModuleConductionMultiplier : public PartModule {
public:
    using PartModule::PartModule;
    void OnAwake() override;  // :14
    void Start();             // :19
    void FixedUpdate();       // :24
    double originalConduction = 0.0;
    double currentConduction = 0.0;
    double threshRecip = 0.0;
};

// ModuleAblator.cs
class ModuleAblator : public PartModule {
public:
    using PartModule::PartModule;
    void Start();         // :95
    void FixedUpdate();   // :166 (heat conductivity switch, ablation loss, resource request, exposed flux)
    void UpdateColor();   // :248 (the char scalar is returned: IScalarModule.SetScalar is the shell's)
    float CharScalar() const;
    bool useAblator = true;
    bool useOutput = false;
    int ablativeID = 0;
    int outputID = 0;
    double ablativeAmount = 0.0;
    double ablativeMaxAmount = 0.0;
    int ablativeFlowMode = 0;
    int outputFlowMode = 0;
    double pyrolysisLoss = 0.0;
    double origConductivity = 0.0;
    int downDir = 3;
    double density = 1.0;
    AttachNode* occNode = nullptr;
    bool charModuleFound = false;  // IScalarModule charModule != null (the shell resolves it)
    Part* PartOf() const;
};

// ModuleActiveRadiator.cs (the radiator side of the core-heat loop)
class ModuleActiveRadiator : public PartModule {
public:
    using PartModule::PartModule;
    // seams: ModuleDeployableRadiator.deployState == EXTENDED (null: not deployable), PartModule resource inputs (CheckResources)
    std::function<bool()> deployableExtended;  // unset: no ModuleDeployableRadiator
    std::function<bool()> checkResources;      // unset: resources fine
    double originalMaxEnergyTranfer = 100000.0;
    double statusValue = -1.0;
    bool partCachesDirty = true;
    std::vector<Part*> activeRadiatorParts, nonRadiatorParts;
    std::vector<Radiators::RadiatorData> coolParts;
    std::vector<Part*> hotParts;

    void OnAwake() override;                                      // :206
    void Start();                                                 // :152 (the deployable seam replaces CheckIfDeployable)
    void FixedUpdate();                                           // :216
    void InternalCooling(Radiators::RadiatorData& rad, int radCount);  // :276
    bool IsSibling(Part& targetPart);                             // :353
    void CheckPartCaches();                                       // :485
    // `part.isRadiator(out radiator)` of a part: the deploy state of its ModuleDeployableRadiator (seam, default: not a radiator)
    static inline std::function<bool(Part&, bool& extended)> isRadiator;
    Part* PartOf() const;
};

// ModuleCoreHeat.cs. BaseConverter (the part's resource converters) is the `converters` seam.
class ModuleCoreHeat : public PartModule {
public:
    using PartModule::PartModule;
    struct ConverterState {  // the BaseConverter members ModuleCoreHeat reads / writes
        bool isActivated = false;
        bool generatesHeat = false;
        double lastTimeFactor = 0.0;
        const FloatCurve* temperatureModifier = nullptr;
        double crewHeatBonus = 0.0;  // GetCrewHeatBonus()
        double lastHeatFlux = 0.0;   // written
        bool autoShutdown = true;
        std::function<void()> stopResourceConverter;
    };
    std::vector<ConverterState*> converters;  // part.Modules as BaseConverter (shell fills)
    std::vector<Part*> vRadList, pRadList, activeRadiatorParts, coreHeatParts;
    bool partCachesDirty = true;
    double lastFlux = 0.0, lastUpdateTime = 0.0, lastCoreTemp = 0.0, energyTemp = 0.0;

    void OnAwake() override;                       // :416 (PassiveEnergy default curve)
    double CoreTemperature() const;                // CoreThermalEnergy / part.thermalMass
    void AddEnergyToCore(double energy) { energyTemp += energy; }  // :245
    double GetLastFlux() const { return lastFlux; }                 // :250
    void CheckCoreShutdown();                      // :255 (screen message omitted)
    void FixedUpdate();                            // :273
    void ResolveNetEnergy();                       // :353
    void ResolveConverterEnergy(double deltaTime); // :359
    bool HasActiveConverters() const;              // :382
    bool HasEnabledConverters() const;             // :399
    void MoveCoreEnergyToRadiators(double excess, double deltaTime);  // :431
    void AddFluxToRadiator(Part& rad, double xferToPart, double deltatime);  // :554
    void CheckStartingTemperature();               // :562
    double GetTransferAmount(double energy, double multiplier) const { return energy * multiplier; }  // :574
    double GetDeltaTime();                         // :579
    void CheckPartCaches();                        // :677
    Part* PartOf() const;
};

// Registers the thermal modules with the ModuleFactory (called from RegisterBuiltinModules)
namespace detail {
void RegisterThermalModules();
}

}  // namespace ksp
