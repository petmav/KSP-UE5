// WP-23: ModuleEngines.cs, ModuleEnginesFX.cs, MultiModeEngine.cs (see ksp/modules/module_engines.hpp for scope).
#include "ksp/modules/module_engines.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "ksp/aero/float_eval.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/modules/module_gimbal.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/ksp_enums.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using aero::D;
using aero::FE;
using aero::MonoFloat;
using aero::NetFloat;
using aero::S;
using aero::U;

namespace {

// EngineType.cs: Enum.Parse(typeof(EngineType), name)
const EnumTable& EngineTypeTable() {
    static const EnumTable t{"EngineType", false, "EngineType.cs", 1,
                             {{"Generic", 0}, {"SolidBooster", 1}, {"LiquidFuel", 2}, {"Piston", 3}, {"Turbine", 4},
                              {"ScramJet", 5}, {"Electric", 6}, {"Nuclear", 7}, {"MonoProp", 8}}};
    return t;
}

const FloatCurve& EmptyCurve() {
    static const FloatCurve c;
    return c;
}

// OnAwake's `PartResourceLibrary.Instance.GetDefinition(name)`: the game always has a library; without one (a unit test that
// replaced it) the entry is null, as for an unknown resource
const PartResourceDefinition* LookupDefinition(const std::string& name) {
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    return lib ? lib->GetDefinition(std::string_view(name)) : nullptr;
}
PartResourceLibrary& Library() {
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    if (lib == nullptr) throw NullReferenceException();
    return *lib;
}

// the localised status strings (ModuleEngines.CacheLocalStrings, ModuleEnginesFX.CacheLocalStrings)
std::string Loc(const char* key) { return Localizer::Format(key); }

}  // namespace

// ============================================================================================================================
// ModuleEngines
// ============================================================================================================================

Part* ModuleEngines::PartPtr() const { return dynamic_cast<Part*>(part); }

// a KSPField FloatCurve that is still null (OnAwake of the C# creates atmosphereCurve; the others have initialisers) acts as empty
const FloatCurve& ModuleEngines::Curve(std::string_view name) const {
    const ModuleValue* v = Value(name);
    const FloatCurve* c = v ? std::get_if<FloatCurve>(v) : nullptr;
    return c ? *c : EmptyCurve();
}

// Unity Instantiate: the public non-KSPField members the component serializes (ModuleEngines.cs: propellants, mixtureDensity,
// mixtureDensityRecip, ratioSum, minThrust, maxThrust, maxFuelFlow, minFuelFlow, throttleMin, thrustTransformMultipliers,
// resultingThrust, flowMultiplier, requestedMassFlow, requestedThrottle, engineType, g). Protected / private members
// (massFlow, consumedResources, isThrusting, listenersSet) are not serialized: they keep their initialiser value.
void ModuleEngines::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* e = dynamic_cast<const ModuleEngines*>(&from)) {
        propellants = e->propellants;
        mixtureDensity = e->mixtureDensity;
        mixtureDensityRecip = e->mixtureDensityRecip;
        ratioSum = e->ratioSum;
        minThrust = e->minThrust;
        maxThrust = e->maxThrust;
        maxFuelFlow = e->maxFuelFlow;
        minFuelFlow = e->minFuelFlow;
        throttleMin = e->throttleMin;
        thrustTransformMultipliers = e->thrustTransformMultipliers;
        thrustTransforms = e->thrustTransforms;
        resultingThrust = e->resultingThrust;
        flowMultiplier = e->flowMultiplier;
        requestedMassFlow = e->requestedMassFlow;
        requestedThrottle = e->requestedThrottle;
        engineType = e->engineType;
        g = e->g;
    }
}

// ModuleEngines.cs:1609
void ModuleEngines::OnAwake() {
    PartModule::OnAwake();
    // [KSPEvent] / [KSPAction] methods (the delegates Delegate.CreateDelegate made)
    Events().Bind("Activate", BaseEvent::Handler([this] { Activate(); }));
    Events().Bind("Shutdown", BaseEvent::Handler([this] { Shutdown(); }));
    Events().Bind("ToggleIncludeinDV", BaseEvent::Handler([this] { ToggleIncludeinDV(); }));
    Actions().Bind("OnAction", [this](KSPActionParam& p) { OnAction(p); });
    Actions().Bind("ShutdownAction", [this](KSPActionParam& p) { ShutdownAction(p); });
    Actions().Bind("ActivateAction", [this](KSPActionParam& p) { ActivateAction(p); });
    Actions().Bind("ToggleThrottle", [this](KSPActionParam& p) { ToggleThrottle(p); });
    // statusL2Field = base.Fields["statusL2"]; damageLayerMask (Unity layer): not modelled
    if (IsNull("atmosphereCurve")) Set<FloatCurve>("atmosphereCurve", FloatCurve{});  // :1617
    // propellants / thrustTransformMultipliers: `== null` creates the list; the vectors always exist (Instantiate keeps the copy)
    consumedResources.clear();
    for (size_t i = 0; i < propellants.size(); i++) consumedResources.push_back(LookupDefinition(propellants[i].name));
}

// ModuleEngines.cs:598
void ModuleEngines::OnLoad(const ConfigNode& node) {
    Set<std::string>("status", Localizer::Format("#autoLOC_219034"));
    if (node.HasNode("PROPELLANT")) propellants.clear();
    for (ConfigNode* configNode : node.nodes().GetNodes()) {
        if (configNode->name != "PROPELLANT") continue;
        if (!configNode->HasValue("name")) {
            log::Info("Propellant must have value 'name'");
            continue;
        }
        Propellant propellant;
        propellant.Load(*configNode);
        propellants.push_back(std::move(propellant));
    }
    SetupPropellant();
    consumedResources.clear();
    for (size_t j = 0; j < propellants.size(); j++) consumedResources.push_back(LookupDefinition(propellants[j].name));
    const float ispVac = Curve("atmosphereCurve").Evaluate(0.0f);
    if (node.HasValue("maxThrust")) {
        maxThrust = cs::FloatParse(*node.GetValue("maxThrust"));
        maxFuelFlow = FuelFlowFromThrust<MonoFloat>(maxThrust, ispVac, g);
    }
    if (node.HasValue("minThrust")) {
        minThrust = cs::FloatParse(*node.GetValue("minThrust"));
        minFuelFlow = FuelFlowFromThrust<MonoFloat>(minThrust, ispVac, g);
    }
    throttleMin = S(FE<MonoFloat>(minFuelFlow) / FE<MonoFloat>(maxFuelFlow));
    if (Get<bool>("throttleUseAlternate")) {
        if (!Get<bool>("throttleInstant")) {
            if (Get<float>("throttleResponseRate") <= 0.0f) {
                const double mass = part ? static_cast<double>(part->fields.mass) : 0.0;
                Set<float>("throttleResponseRate",
                           static_cast<float>(Get<double>("throttlingBaseRate") /
                                              dotnet::Math::Log(std::max(Get<double>("throttlingBaseClamp"),
                                                                         dotnet::Math::Sqrt(Get<double>("throttlingBaseDivisor") * mass *
                                                                                            static_cast<double>(maxThrust) * static_cast<double>(maxThrust))))));
            }
        } else {
            Set<float>("throttleResponseRate", 1000000.0f);
        }
    }
    if (node.HasValue("EngineType")) engineType = static_cast<EngineType>(EngineTypeTable().Parse(*node.GetValue("EngineType")));
    thrustTransforms.clear();
    if (findModelTransforms) {
        if (Part* p = PartPtr()) thrustTransforms = findModelTransforms(*p, Get<std::string>("thrustVectorTransformName"));
    }
    if (node.HasNode("transformMultipliers")) {
        thrustTransformMultipliers.clear();
        ConfigNode* node2 = node.GetNode("transformMultipliers");
        int num = static_cast<int>(node2->values().Count());
        float num2 = 0.0f;
        if (num == static_cast<int>(thrustTransforms.size()) - 1) num++;
        if (num == static_cast<int>(thrustTransforms.size())) {
            for (int k = 0; k < num - 1; k++) {
                float result = 0.0f;
                if (node2->HasValue("trf" + std::to_string(k)) && cs::FloatTryParse(*node2->GetValue("trf" + std::to_string(k)), result)) {
                    thrustTransformMultipliers.push_back(result);
                    num2 = S(FE<MonoFloat>(num2) + FE<MonoFloat>(result));
                }
            }
            if (static_cast<int>(thrustTransformMultipliers.size()) + 1 == num && num2 <= 1.0f)
                thrustTransformMultipliers.push_back(S(FE<MonoFloat>(1.0f) - FE<MonoFloat>(num2)));
        }
    }
    if (thrustTransformMultipliers.size() != thrustTransforms.size()) {
        const float item = S(FE<MonoFloat>(1.0f) / FE<MonoFloat>(static_cast<float>(thrustTransforms.size())));
        thrustTransformMultipliers.clear();
        size_t count3 = thrustTransforms.size();
        while (count3-- > 0) thrustTransformMultipliers.push_back(item);
    }
}

// ModuleEngines.cs:390 (policy template: `num += definition.density * propellant.ratio` is a float expression)
template <class P>
void ModuleEngines::SetupPropellantT() {
    using E = FE<P>;
    mixtureDensity = 0.0f;
    mixtureDensityRecip = 1.0;
    float num = 0.0f;
    ratioSum = 0.0;
    for (size_t i = 0; i < propellants.size(); i++) {
        Propellant& propellant = propellants[i];
        const PartResourceDefinition* definition = Library().GetDefinition(static_cast<int32_t>(MonoStringHash(propellant.name)));
        if (definition == nullptr) {
            log::Info(propellant.name + " not found in resource database. Propellant Setup has failed.");
            if (Part* pp = PartPtr()) {
                if (pp->partInfo != nullptr) const_cast<AvailablePart*>(pp->partInfo)->amountAvailable = 0;
            }
            continue;
        }
        if (!propellant.ignoreForIsp) {
            num = S(E(num) + E(definition->density()) * E(propellant.ratio));
            ratioSum += static_cast<double>(propellant.ratio);
        }
        propellant.GetFlowMode();
    }
    mixtureDensity = num;
    if (static_cast<double>(mixtureDensity) > 0.0) mixtureDensityRecip = 1.0 / static_cast<double>(mixtureDensity);
}
void ModuleEngines::SetupPropellant() { SetupPropellantT<MonoFloat>(); }
template void ModuleEngines::SetupPropellantT<MonoFloat>();
template void ModuleEngines::SetupPropellantT<NetFloat>();

// ModuleEngines.cs:636-647: `maxThrust / (atmosphereCurve.Evaluate(0f) * g)`
template <class P>
float ModuleEngines::FuelFlowFromThrust(float thrust, float ispVac, float g) {
    using E = FE<P>;
    return S(E(thrust) / (E(ispVac) * E(g)));
}
template float ModuleEngines::FuelFlowFromThrust<MonoFloat>(float, float, float);
template float ModuleEngines::FuelFlowFromThrust<NetFloat>(float, float, float);

// ModuleEngines.cs:421
void ModuleEngines::UpdatePropellantStatus(bool /*doGauge*/) {
    Part* p = PartPtr();
    for (Propellant& propellant : propellants) {
        if (p == nullptr) throw NullReferenceException();
        propellant.UpdateConnectedResources(*p);
        // propellant.drawStackGauge && doGauge -> UpdatePropellantGauge (stage-icon UI)
    }
}

// ModuleEngines.cs:497
void ModuleEngines::Flameout(const std::string& message, bool statusOnly, bool /*showFX*/) {
    if (BaseField* f = Fields()["statusL2"]) f->guiActive = true;
    Set<std::string>("statusL2", message);
    if (!statusOnly) {
        // if (!flameout && showFX) PlayFlameoutFX(flamingOut: true): FX
        Set<bool>("flameout", true);
        if (!Get<bool>("allowRestart")) Shutdown();
        Set<std::string>("status", Loc("#autoLOC_219016"));
    }
    Set<float>("currentThrottle", 0.0f);
}

// ModuleEngines.cs:517
void ModuleEngines::UnFlameout(bool /*showFX*/) {
    // if (flameout && showFX) PlayFlameoutFX(flamingOut: false): FX
    Set<bool>("flameout", false);
    Set<std::string>("status", Loc("#autoLOC_219034"));
    if (BaseField* f = Fields()["statusL2"]) f->guiActive = false;
    // ActivateRunningFX(): FX
}

// ModuleEngines.cs:529
bool ModuleEngines::CheckDeprived(double requiredPropellant, std::string& propName) {
    bool result = false;
    propName = "";
    const float ignitionThreshold = Get<float>("ignitionThreshold");
    for (size_t i = 0; i < propellants.size(); i++) {
        Propellant& propellant = propellants[i];
        propellant.currentRequirement = requiredPropellant * static_cast<double>(propellant.ratio);
        if (propellant.totalResourceAvailable() >= propellant.currentRequirement * static_cast<double>(ignitionThreshold)) {
            propellant.isDeprived = false;
            continue;
        }
        propellant.isDeprived = true;
        result = true;
        propName = propellant.displayName;
    }
    return result;
}

// ModuleEngines.cs:550
double ModuleEngines::RequestPropellant(double mass) {
    const bool clampPropReceived = Get<bool>("clampPropReceived");
    double num = clampPropReceived ? 1.0 : 0.0;
    const double num2 = mass * mixtureDensityRecip;
    double num3 = 0.0;
    if (num2 == 0.0) return 0.0;
    std::string propName;
    if (CheckDeprived(num2, propName)) {
        Flameout(Localizer::Format("#autoLOC_219085", std::vector<std::string>{propName}));
    } else {
        UnFlameout();
        Part* p = PartPtr();
        if (p == nullptr) throw NullReferenceException();
        const int count = static_cast<int>(propellants.size());
        for (int i = 0; i < count; i++) {
            Propellant& propellant = propellants[static_cast<size_t>(i)];
            const ResourceFlowMode mode = static_cast<ResourceFlowMode>(propellant.GetFlowMode());
            if (clampPropReceived) {
                propellant.currentAmount = UtilMath::Clamp(
                    p->RequestResource(propellant.id, std::min(propellant.totalResourceAvailable(), propellant.currentRequirement * num), mode), 0.0,
                    propellant.currentRequirement);
                const double num4 = propellant.currentAmount / propellant.currentRequirement;
                if (num4 < num && num4 < Get<double>("clampPropReceivedMinLowerAmount")) num = num4;
            } else {
                propellant.currentAmount = UtilMath::Clamp(
                    p->RequestResource(propellant.id, std::min(propellant.totalResourceAvailable(), propellant.currentRequirement), mode), 0.0,
                    propellant.currentRequirement);
                num3 += propellant.currentAmount / propellant.currentRequirement;
            }
        }
        if (!clampPropReceived) {
            num = num3 / static_cast<double>(count);
            num = UtilMath::Clamp(num, 0.0, 1.0);
        }
    }
    if (flameout()) return 0.0;
    return num;
}

// ModuleEngines.cs:1651
void ModuleEngines::OnStart(StartState state) {
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    if (p->fields.stagingIcon && p->fields.stagingIcon->empty() && Get<bool>("overrideStagingIconIfBlank"))
        p->fields.stagingIcon = std::string("LIQUID_ENGINE");
    // HighLogic.LoadedSceneIsFlight -> InitializeFX(): FX
    BaseEvent* shutdownEvent = Events()["Shutdown"];
    BaseEvent* activateEvent = Events()["Activate"];
    BaseField* propellantReqMetField = Fields()["propellantReqMet"];
    shutdownEvent->active = false;
    activateEvent->active = false;
    // thrustPercentage.uiControl*.onFieldChanged += thrustPercentChanged, independentThrottle.OnValueModified += throttleModeChanged,
    // independentThrottlePercentField visibility, UpdateIncludeDVUI(): UI
    if (state != StartState::PreLaunch) {
        if (EngineIgnited()) {
            shutdownEvent->active = Get<bool>("allowShutdown");
            activateEvent->active = false;
            propellantReqMetField->guiActive = true;
        } else {
            shutdownEvent->active = false;
            if (!Get<bool>("allowRestart") && Get<bool>("engineShutdown")) activateEvent->active = false;
            else activateEvent->active = true;
            propellantReqMetField->guiActive = false;
        }
    }
    if (Get<bool>("throttleLocked")) {
        Fields()["thrustPercentage"]->guiActive = false;
        Fields()["independentThrottle"]->guiActive = false;
        Fields()["independentThrottlePercentage"]->guiActive = false;
    }
    Fields()["thrustCurveDisplay"]->guiActive = Get<bool>("useThrustCurve");
    // SetListener(): GameEvents.onGamePause / onGameUnpause (audio)
    thrustTransforms.clear();
    if (findModelTransforms) thrustTransforms = findModelTransforms(*p, Get<std::string>("thrustVectorTransformName"));
}

// ModuleEngines.cs:1724
void ModuleEngines::OnActive() {
    if (!EngineIgnited() && !Get<bool>("manuallyOverridden") && Get<bool>("stagingEnabled") && !Get<bool>("staged")) {
        if (!IsEngineDead()) Activate();
        Set<bool>("staged", EngineIgnited());
    }
}

// ModuleEngines.cs:740
void ModuleEngines::Activate() {
    if (!Get<bool>("allowRestart") && Get<bool>("engineShutdown")) return;
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    if (p->ShieldedFromAirstream() && !Get<bool>("shieldedCanActivate")) {
        // ScreenMessages.PostScreenMessage("<color=orange>[" + title + "]: " + #autoLOC_219636 + "</color>", ...): UI
        log::Info("[" + (p->partInfo ? p->partInfo->title : std::string()) + "]: " + Loc("#autoLOC_219636"));
        return;
    }
    // if (!EngineIgnited) PlayEngageFX(): FX
    Set<bool>("EngineIgnited", true);
    Events()["Shutdown"]->active = Get<bool>("allowShutdown");
    Events()["Activate"]->active = false;
    Fields()["propellantReqMet"]->guiActive = true;
    if (!Get<bool>("staged")) {
        PartModuleList& mods = p->Modules();
        size_t count = mods.Count();
        while (count-- > 0) {
            if (auto* gimbal = dynamic_cast<ModuleGimbal*>(mods.GetModule(count))) {
                if (!gimbal->Get<bool>("gimbalActive")) {
                    gimbal->Set<bool>("gimbalActive", true);
                    gimbal->OnStart(StartState::Flying);
                }
            }
        }
    }
    // FXReset(): FX
    GameEvents::onEngineActiveChange.Fire(this);
}

// ModuleEngines.cs:788
void ModuleEngines::Shutdown() {
    if (!Get<bool>("allowShutdown")) return;
    if (!Get<bool>("allowRestart")) {
        Set<bool>("engineShutdown", true);
        Events()["Shutdown"]->active = false;
        Events()["Activate"]->active = false;
    } else {
        Events()["Shutdown"]->active = false;
        Events()["Activate"]->active = true;
    }
    Fields()["propellantReqMet"]->guiActive = false;
    Set<bool>("EngineIgnited", false);
    // PlayShutdownFX(), part.stackIcon.RemoveInfo(PropellantGauges[key]): FX / UI
    GameEvents::onEngineActiveChange.Fire(this);
}

// ModuleEngines.cs:822
void ModuleEngines::ToggleIncludeinDV() {
    Set<bool>("includeinDVCalcs", !Get<bool>("includeinDVCalcs"));
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    for (size_t i = 0; i < p->symmetryCounterparts.size(); i++) {
        Part* sym = p->symmetryCounterparts[i];
        if (sym == nullptr) throw NullReferenceException();
        // GetComponent<ModuleEngines>(): the first ModuleEngines of the counterpart
        PartModule* c = sym->FindModuleImplementing([](PartModule& pm) { return pm.Class().Is("ModuleEngines"); });
        if (auto* component = dynamic_cast<ModuleEngines*>(c)) {
            component->Set<bool>("includeinDVCalcs", Get<bool>("includeinDVCalcs"));
            // component.UpdateIncludeDVUI(): UI
        }
    }
    GameEvents::onChangeEngineDVIncludeState.Fire(this);
}

// ModuleEngines.cs:839
void ModuleEngines::OnAction(KSPActionParam&) {
    if (!EngineIgnited()) Activate();
    else Shutdown();
}
// ModuleEngines.cs:852
void ModuleEngines::ShutdownAction(KSPActionParam&) { Shutdown(); }
// ModuleEngines.cs:858
void ModuleEngines::ActivateAction(KSPActionParam&) { Activate(); }
// ModuleEngines.cs:864 (throttleModeChanged is UI visibility only)
void ModuleEngines::ToggleThrottle(KSPActionParam&) { Set<bool>("independentThrottle", !Get<bool>("independentThrottle")); }
// ModuleEngines.cs:870
void ModuleEngines::thrustPercentChanged() { GameEvents::onEngineThrustPercentageChanged.Fire(this); }

// ---- thrust maths ----------------------------------------------------------------------------------------------------------

// ModuleEngines.cs:2040
float ModuleEngines::getExhaustVelocity(float isp) const { return S(FE<MonoFloat>(isp) * FE<MonoFloat>(g)); }

// ModuleEngines.cs:2045
template <class P>
float ModuleEngines::GetEngineThrustT(float isp, float throttle) const {
    using E = FE<P>;
    return S(E(U<P>::Lerp(minFuelFlow, maxFuelFlow, throttle)) * E(S(E(isp) * E(g))));
}
float ModuleEngines::GetEngineThrust(float isp, float throttle) const { return GetEngineThrustT<MonoFloat>(isp, throttle); }

// ModuleEngines.cs:2050
template <class P>
float ModuleEngines::GetThrottlingMultT(float atm, float throttle) const {
    return U<P>::Lerp(1.0f, Curve("throttleIspCurve").Evaluate(throttle), Curve("throttleIspCurveAtmStrength").Evaluate(atm));
}
float ModuleEngines::GetThrottlingMult(float atm, float throttle) const { return GetThrottlingMultT<MonoFloat>(atm, throttle); }

// ModuleEngines.cs:1833
template <class P>
float ModuleEngines::MaxThrustOutputAtmT(bool runningActive, bool useThrustLimiter, float atmPressure, double /*atmTemp*/, double atmDensity,
                                         double machNumber) const {
    using E = FE<P>;
    float throttle = useThrustLimiter ? S(E(thrustPercentage()) * E(0.01f)) : 1.0f;
    throttle = ApplyThrottleAdjustments(throttle);
    float num = GetEngineThrustT<P>(Curve("atmosphereCurve").Evaluate(atmPressure), throttle);
    if (Get<bool>("useThrottleIspCurve")) num = S(E(num) * E(GetThrottlingMultT<P>(atmPressure, throttle)));
    float num2 = 1.0f;
    if (Get<bool>("atmChangeFlow")) {
        num2 = static_cast<float>(atmDensity * 0.8163265147242933);
        if (Get<bool>("useAtmCurve")) num2 = Curve("atmCurve").Evaluate(num2);
    }
    if (Get<bool>("useVelCurve")) num2 = S(E(num2) * E(Curve("velCurve").Evaluate(static_cast<float>(machNumber))));
    float num3 = 1.0f;
    float num4 = 1.0f;
    if (Get<bool>("useThrustCurve")) {
        size_t count = propellants.size();
        while (count-- > 0) {
            const Propellant& propellant = propellants[count];
            if (!propellant.ignoreForThrustCurve && (runningActive || !(propellant.name == "IntakeAir"))) {
                const float num5 = static_cast<float>(propellant.totalResourceAvailable() / propellant.totalResourceCapacity);
                if (num5 < num4) num4 = num5;
            }
        }
        num3 = Curve("thrustCurve").Evaluate(num4);
        num2 = S(E(num2) * E(num3));
    }
    const float flowMultCap = Get<float>("flowMultCap");
    if (num2 > flowMultCap) {
        const float num6 = S(E(num2) - E(flowMultCap));
        num2 = S(E(flowMultCap) + E(num6) / (E(Get<float>("flowMultCapSharpness")) + E(num6) / E(flowMultCap)));
    }
    if (num2 < Get<float>("CLAMP")) num2 = Get<float>("CLAMP");
    return S(E(num) * E(num2));
}
float ModuleEngines::MaxThrustOutputAtm(bool runningActive, bool useThrustLimiter, float atmPressure, double atmTemp, double atmDensity) const {
    const Part* p = PartPtr();
    return MaxThrustOutputAtmT<MonoFloat>(runningActive, useThrustLimiter, atmPressure, atmTemp, atmDensity,
                                          p ? p->fields.machNumber : 0.0);
}

// ModuleEngines.cs:1887
template <class P>
float ModuleEngines::MaxThrustOutputVacT(bool useThrustLimiter) const {
    using E = FE<P>;
    for (size_t num = 0; num < propellants.size(); num++)
        if (propellants[num].name == "IntakeAir") return 0.0f;
    float throttle = useThrustLimiter ? S(E(thrustPercentage()) * E(0.01f)) : 1.0f;
    throttle = ApplyThrottleAdjustments(throttle);
    float num2 = GetEngineThrustT<P>(Curve("atmosphereCurve").Evaluate(0.0f), throttle);
    if (Get<bool>("useThrottleIspCurve")) num2 = S(E(num2) * E(GetThrottlingMultT<P>(0.0f, 1.0f)));
    return num2;
}
float ModuleEngines::MaxThrustOutputVac(bool useThrustLimiter) const { return MaxThrustOutputVacT<MonoFloat>(useThrustLimiter); }

// ModuleEngines.cs:2029
float ModuleEngines::getMaxFuelFlow(const Propellant& p) const { return getFuelFlow(p, maxFuelFlow); }

// ModuleEngines.cs:2034 (the double chain is exact under both runtimes)
float ModuleEngines::getFuelFlow(const Propellant& p, float fuelFlow) const {
    fuelFlow = Mathf::Clamp(fuelFlow, minFuelFlow, maxFuelFlow);
    return static_cast<float>(static_cast<double>(fuelFlow) * mixtureDensityRecip * static_cast<double>(p.ratio));
}

// ModuleEngines.cs:2055
float ModuleEngines::ModifyFlow() {
    using E = FE<MonoFloat>;
    const Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    float num = 1.0f;
    if (Get<bool>("atmChangeFlow")) {
        num = static_cast<float>(p->fields.atmDensity * 0.8163265147242933);
        if (Get<bool>("useAtmCurve")) num = Curve("atmCurve").Evaluate(num);
    }
    if (Get<bool>("useVelCurve")) num = S(E(num) * E(Curve("velCurve").Evaluate(static_cast<float>(p->fields.machNumber))));
    if (Get<bool>("useThrustCurve")) {
        const float num2 = 1.0f;
        Set<float>("thrustCurveRatio", 1.0f);
        Set<float>("thrustCurveDisplay", num2);
        size_t count = propellants.size();
        while (count-- > 0) {
            const Propellant& propellant = propellants[count];
            if (!propellant.ignoreForThrustCurve) {
                const float num3 = static_cast<float>(propellant.totalResourceAvailable() / propellant.totalResourceCapacity);
                if (num3 < Get<float>("thrustCurveRatio")) Set<float>("thrustCurveRatio", num3);
            }
        }
        Set<float>("thrustCurveDisplay", Curve("thrustCurve").Evaluate(Get<float>("thrustCurveRatio")));
        num = S(E(num) * E(Get<float>("thrustCurveDisplay")));
    }
    const float flowMultCap = Get<float>("flowMultCap");
    if (num > flowMultCap) {
        const float num4 = S(E(num) - E(flowMultCap));
        num = S(E(flowMultCap) + E(num4) / (E(Get<float>("flowMultCapSharpness")) + E(num4) / E(flowMultCap)));
    }
    if (num < Get<float>("CLAMP")) num = Get<float>("CLAMP");
    return num;
}

// ---- flight ----------------------------------------------------------------------------------------------------------------

// ModuleEngines.cs:1352
void ModuleEngines::UpdateThrottle(const EngineFlightInput& in) {
    using E = FE<MonoFloat>;
    Part* p = PartPtr();
    const float thrustPct = thrustPercentage();
    if (Get<bool>("throttleLocked")) {
        requestedThrottle = EngineIgnited() ? S(E(thrustPct) * E(0.01f)) : 0.0f;
    } else if (Get<bool>("independentThrottle")) {
        requestedThrottle = EngineIgnited()
                                ? S(E(Get<float>("independentThrottlePercentage")) * E(0.01f) * (E(thrustPct) * E(0.01f)))
                                : 0.0f;
    } else if (in.flightReady) {
        if (EngineIgnited()) {
            Vessel* v = p ? p->vessel : nullptr;
            if (v == nullptr) throw NullReferenceException();
            if (v->IsControllable()) requestedThrottle = S(E(v->ctrlState.mainThrottle) * (E(thrustPct) * E(0.01f)));
        } else {
            requestedThrottle = 0.0f;
        }
    }
    float currentThrottleV = currentThrottle();
    if (Get<bool>("throttleUseAlternate")) {
        if (requestedThrottle > 0.0f) {
            requestedThrottle = Mathf::Lerp(throttleMin, 1.0f, requestedThrottle);
        } else if (Get<bool>("throttleInstantShutdown")) {
            Set<float>("currentThrottle", 0.0f);
            return;
        }
        if (Get<bool>("throttleInstant")) {
            Set<float>("currentThrottle", requestedThrottle);
            return;
        }
        const float num = S(E(0.01f) * E(Get<float>("throttleIgniteLevelMult")));
        const float fixedDeltaTime = in.fixedDeltaTime;
        float num2 = S(E(requestedThrottle) - E(currentThrottleV));
        if (num2 == 0.0f) return;
        float num3 = S(E(Get<float>("throttleResponseRate")) * E(fixedDeltaTime));
        float num4 = 1.0f;
        if (num2 < 0.0f) {
            num4 = -1.0f;
            num2 = -num2;
            if (requestedThrottle <= num) num3 = S(E(num3) * E(Get<float>("throttleShutdownMult")));
        }
        if (currentThrottleV > num) {
            const float num5 = S(E(1.0f) - E(num2));
            num3 = S(E(num3) * (E(1.0f) - E(num5) * E(num5)) * E(5.0f) * E(Get<float>("throttleStartedMult")));
        } else {
            num3 = S(E(num3) * (E(0.0005f) + E(4.05f) * E(currentThrottleV) * E(Get<float>("throttleStartupMult"))));
        }
        if (num2 > num3 && num2 > 0.005f) Set<float>("currentThrottle", S(E(currentThrottleV) + E(num3) * E(num4)));
        else Set<float>("currentThrottle", requestedThrottle);
        return;
    }
    if (throttleMin != 0.0f) requestedThrottle = Mathf::Lerp(throttleMin, 1.0f, requestedThrottle);
    if (Get<bool>("useEngineResponseTime")) {
        if (Mathf::Abs(S(E(currentThrottleV) - E(requestedThrottle))) <= 0.0001f) {
            Set<float>("currentThrottle", requestedThrottle);
        } else if (currentThrottleV < requestedThrottle) {
            Set<float>("currentThrottle", Mathf::Lerp(currentThrottleV, requestedThrottle, S(E(Get<float>("engineAccelerationSpeed")) * E(in.fixedDeltaTime))));
        } else {
            Set<float>("currentThrottle", Mathf::Lerp(currentThrottleV, requestedThrottle, S(E(Get<float>("engineDecelerationSpeed")) * E(in.fixedDeltaTime))));
        }
    } else {
        Set<float>("currentThrottle", requestedThrottle);
    }
}

// ModuleEngines.cs:1454
double ModuleEngines::RequiredPropellantMass(float throttleAmount, const EngineFlightInput& in) {
    using E = FE<MonoFloat>;
    const Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    const float num = static_cast<float>(p->fields.staticPressureAtm);
    float realIspV = Curve("atmosphereCurve").Evaluate(num);
    if (Get<bool>("useThrottleIspCurve")) realIspV = S(E(realIspV) * E(GetThrottlingMult(num, throttleAmount)));
    requestedMassFlow = S(E(maxFuelFlow) * E(throttleAmount));
    flowMultiplier = ModifyFlow();
    requestedMassFlow = S(E(requestedMassFlow) * (E(flowMultiplier) * E(Get<float>("multFlow"))));
    if (Get<bool>("useAtmCurveIsp")) realIspV = S(E(realIspV) * E(Curve("atmCurveIsp").Evaluate(static_cast<float>(p->fields.atmDensity * 0.8163265147242933))));
    if (Get<bool>("useVelCurveIsp")) realIspV = S(E(realIspV) * E(Curve("velCurveIsp").Evaluate(static_cast<float>(p->fields.machNumber))));
    Set<float>("realIsp", realIspV);
    const float num2 = S(E(realIspV) * E(g) * E(Get<float>("multIsp")));
    resultingThrust = S(E(requestedMassFlow) * E(num2));
    return D(E(requestedMassFlow) * E(in.fixedDeltaTime));
}

// ModuleEngines.cs:1478
float ModuleEngines::CalculateThrust(const EngineFlightInput& in) {
    using E = FE<MonoFloat>;
    massFlow = 0.0;
    double num = 0.0;
    Part* p = PartPtr();
    Vessel* v = p ? p->vessel : nullptr;
    if (v == nullptr) throw NullReferenceException();
    float num2 = v->GetVesselValues().FuelUsage.value();
    if (num2 == 0.0f) num2 = 1.0f;
    massFlow = RequiredPropellantMass(currentThrottle(), in) * static_cast<double>(num2);
    if (flowMultiplier < Get<float>("flameoutBar")) {
        Flameout(Loc("#autoLOC_220370"));
        num = 0.0;
    } else if (Get<bool>("disableUnderwater") && v != nullptr && in.oceanBody && in.underwater) {
        Flameout(Loc("#autoLOC_220377"));
        num = 0.0;
    } else if (ResourceFlowEnvironment::InfinitePropellant) {
        num = 1.0;
        if (currentThrottle() > 0.0f) UnFlameout();
    } else {
        num = RequestPropellant(massFlow);
    }
    Set<float>("propellantReqMet", static_cast<float>(num * 100.0));
    const float result = static_cast<float>(static_cast<double>(resultingThrust) * num);
    Set<float>("fuelFlowGui", S(E(static_cast<float>(massFlow * num * mixtureDensityRecip * ratioSum)) * (E(1.0f) / E(in.fixedDeltaTime))));
    return result;
}

// ModuleEngines.cs:1540
EngineThrustResult ModuleEngines::ThrustUpdate(const EngineFlightInput& in) {
    using E = FE<MonoFloat>;
    EngineThrustResult out;
    isThrusting = false;
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    if (EngineIgnited()) {
        Vessel* v = p->vessel;
        if (v == nullptr) throw NullReferenceException();
        Set<float>("finalThrust", S(E(CalculateThrust(in)) * E(v->GetVesselValues().EnginePower.value())));
        const float ft = finalThrust();
        if (ft > 0.0f) {
            isThrusting = true;
            if (p->hasRigidbody) {
                for (size_t i = 0; i < thrustTransforms.size(); i++) {
                    const Vector3f fwd = thrustTransforms[i].forward();
                    // -transform.forward * finalThrust * thrustTransformMultipliers[i]
                    EngineForce f;
                    f.force = (-fwd * ft) * thrustTransformMultipliers.at(i);
                    f.position = thrustTransforms[i].position;
                    out.forces.push_back(f);
                }
            }
            out.exhaustDamage = true;  // EngineExhaustDamage(): raycast damage, WP-20 / WP-22
        }
        double num = 1.0;
        if (p->fields.machNumber > static_cast<double>(Get<float>("machLimit")))
            num += (p->fields.machNumber - static_cast<double>(Get<float>("machLimit"))) * p->fields.atmDensity * static_cast<double>(Get<float>("machHeatMult"));
        const double kilowatts = num * static_cast<double>(Get<float>("heatProduction")) *
                                 (Get<bool>("normalizeHeatForFlow") ? 1.0 : (1.0 / static_cast<double>(flowMultiplier))) *
                                 D(E(finalThrust()) / E(maxThrust)) * static_cast<double>(v->GetVesselValues().HeatProduction.value()) *
                                 PhysicsGlobals::InternalHeatProductionFactor() * p->fields.thermalMass;
        out.hasThermalFlux = true;
        out.thermalFluxKilowatts = kilowatts;
    } else {
        Events()["Shutdown"]->active = false;
        Events()["Activate"]->active = true;
        Fields()["propellantReqMet"]->guiActive = false;
        Set<float>("fuelFlowGui", 0.0f);
        Set<float>("realIsp", 0.0f);
        Set<float>("finalThrust", 0.0f);
        if (p->ShieldedFromAirstream()) Set<std::string>("status", Loc("#autoLOC_220473"));
        else Set<std::string>("status", Loc("#autoLOC_220477"));
    }
    return out;
}

// ModuleEngines.cs:1587
void ModuleEngines::FixedUpdate() {
    lastThrust = EngineThrustResult{};
    if (warping) {  // TimeWarping() (DeactivateLoopingFX is FX)
        Set<float>("finalThrust", 0.0f);
        return;
    }
    UpdateThrottle(flightInput);
    Set<float>("currentThrottle", ApplyThrottleAdjustments(currentThrottle()));
    if (EngineIgnited()) UpdatePropellantStatus();
    lastThrust = ThrustUpdate(flightInput);
    // FXUpdate(): FX (ModuleEnginesFX::FXUpdate keeps the engineSpool value)
}

// ModuleEngines.cs:2103
void ModuleEngines::OnCenterOfThrustQuery(CenterOfThrustQuery& qry) {
    using E = FE<MonoFloat>;
    if (!Get<bool>("manuallyOverridden")) {
        Vector3f zero = Vector3f::zero();
        Vector3f zero2 = Vector3f::zero();
        for (size_t i = 0; i < thrustTransforms.size(); i++) {
            zero += thrustTransforms[i].position * thrustTransformMultipliers.at(i);
            zero2 += thrustTransforms[i].forward() * thrustTransformMultipliers.at(i);
        }
        qry.pos = zero;
        qry.dir = zero2;
        qry.thrust = S(E(maxThrust) * (E(thrustPercentage()) * E(0.01f)));
    }
}

float ModuleEngines::GetCurrentThrust() { return finalThrust(); }                              // :2127
bool ModuleEngines::isOperational() const { return !flameout() && EngineIgnited(); }            // :90
float ModuleEngines::normalizedOutput() const { return S(FE<MonoFloat>(finalThrust()) / FE<MonoFloat>(maxThrust)); }  // :104
float ModuleEngines::normalizedThrustOutput() const { return normalizedOutput(); }              // :84
float ModuleEngines::throttleSetting() const { return currentThrottle(); }                      // :106
std::string ModuleEngines::engineName() const { return Get<std::string>("engineID"); }          // :108
bool ModuleEngines::getFlameoutState() const { return flameout(); }                             // :86
bool ModuleEngines::getIgnitionState() const { return EngineIgnited(); }                        // :88

// explicit instantiations (the golden gate instantiates NetFloat)
template float ModuleEngines::GetEngineThrustT<MonoFloat>(float, float) const;
template float ModuleEngines::GetEngineThrustT<NetFloat>(float, float) const;
template float ModuleEngines::GetThrottlingMultT<MonoFloat>(float, float) const;
template float ModuleEngines::GetThrottlingMultT<NetFloat>(float, float) const;
template float ModuleEngines::MaxThrustOutputAtmT<MonoFloat>(bool, bool, float, double, double, double) const;
template float ModuleEngines::MaxThrustOutputAtmT<NetFloat>(bool, bool, float, double, double, double) const;
template float ModuleEngines::MaxThrustOutputVacT<MonoFloat>(bool) const;
template float ModuleEngines::MaxThrustOutputVacT<NetFloat>(bool) const;

// ============================================================================================================================
// ModuleEnginesFX
// ============================================================================================================================

// ModuleEnginesFX.cs:162 (the base.part.Effect(..., 0f) calls are FX)
void ModuleEnginesFX::OnAwake() {
    ModuleEngines::OnAwake();
    consumedResources.clear();
    for (size_t i = 0; i < propellants.size(); i++) consumedResources.push_back(LookupDefinition(propellants[i].name));
}

// ModuleEnginesFX.cs:132
bool ModuleEnginesFX::CanStart() {
    if (ModifyFlow() < Get<float>("flameoutBar")) {
        Flameout(Loc("#autoLOC_221082"), true);
        return false;
    }
    if (ResourceFlowEnvironment::InfinitePropellant) return true;
    UpdatePropellantStatus(false);
    float num = 1.0f;
    Part* p = PartPtr();
    if (p != nullptr && p->vessel != nullptr) num = p->vessel->GetVesselValues().FuelUsage.value();
    if (num == 0.0f) num = 1.0f;
    const double requiredPropellant = RequiredPropellantMass(1.0f, flightInput) * mixtureDensityRecip * static_cast<double>(num);
    std::string propName;
    if (!CheckDeprived(requiredPropellant, propName)) return true;
    Flameout(propName + " deprived", true);
    return false;
}

// ModuleEnginesFX.cs:59 FXUpdate: the engineSpool value (the part.Effect calls are FX)
void ModuleEnginesFX::FXUpdate(float fixedDeltaTime) {
    using E = FE<MonoFloat>;
    if (EngineIgnited() && !flameout()) {
        if (Get<bool>("useEngineResponseTime"))
            engineSpool = Mathf::Lerp(engineSpool, Mathf::Max(Get<float>("engineSpoolIdle"), currentThrottle()),
                                      S(E(Get<float>("engineSpoolTime")) * E(fixedDeltaTime)));
        else
            engineSpool = currentThrottle();
    } else {
        if (Get<bool>("useEngineResponseTime"))
            engineSpool = Mathf::Lerp(engineSpool, flameout() ? 0.0f : currentThrottle(), S(E(Get<float>("engineSpoolTime")) * E(fixedDeltaTime)));
        else
            engineSpool = 0.0f;
    }
}

// ============================================================================================================================
// MultiModeEngine
// ============================================================================================================================

namespace {
events::EventData<MultiModeEngine*>& OnAutoSwitchModes() {
    static events::EventData<MultiModeEngine*> evt("onMultiModeEngineAutoSwitch");  // MultiModeEngine.cs:37 (static; created in OnAwake, :281)
    return evt;
}
}  // namespace

// MultiModeEngine.cs:273
void MultiModeEngine::OnAwake() {
    Events().Bind("DisableAutoSwitch", BaseEvent::Handler([this] { DisableAutoSwitch(); }));
    Events().Bind("EnableAutoSwitch", BaseEvent::Handler([this] { EnableAutoSwitch(); }));
    Events().Bind("ModeEvent", BaseEvent::Handler([this] { ModeEvent(); }));
    Actions().Bind("ModeAction", [this](KSPActionParam& p) { ModeAction(p); });
    Actions().Bind("ShutdownAction", [this](KSPActionParam& p) { ShutdownAction(p); });
    Actions().Bind("ActivateAction", [this](KSPActionParam& p) { ActivateAction(p); });
    Actions().Bind("OnAction", [this](KSPActionParam& p) { OnAction(p); });
    if (!registered_) {
        OnAutoSwitchModes().Add(this, &MultiModeEngine::OnAutoSwitch);
        GameEvents::onVesselWasModified.Add(this, &MultiModeEngine::onVesselModified);
        registered_ = true;
    }
}

// MultiModeEngine.cs:303 OnDestroy
MultiModeEngine::~MultiModeEngine() {
    if (registered_) {
        OnAutoSwitchModes().Remove(this, &MultiModeEngine::OnAutoSwitch);
        GameEvents::onVesselWasModified.Remove(this, &MultiModeEngine::onVesselModified);
    }
}

// the autoSwitchAvailable == false block shared by OnLoad (:294) and OnStart (:326)
void MultiModeEngine::ApplyAutoSwitchEvents() {
    Set<bool>("autoSwitch", false);
    BaseEvent* disable = Events()["DisableAutoSwitch"];
    BaseEvent* enable = Events()["EnableAutoSwitch"];
    enable->guiActiveEditor = false;
    enable->guiActive = false;
    enable->active = false;
    disable->guiActiveEditor = false;
    disable->guiActive = false;
    disable->active = false;
}

// MultiModeEngine.cs:283
void MultiModeEngine::OnLoad(const ConfigNode& node) {
    PartModule::OnLoad(node);
    if (!Get<bool>("autoSwitchAvailable")) ApplyAutoSwitchEvents();
}

// MultiModeEngine.cs:89
void MultiModeEngine::OnAutoSwitch(MultiModeEngine* other) {
    Part* mine = dynamic_cast<Part*>(part);
    Part* theirs = dynamic_cast<Part*>(other->part);
    if (mine == nullptr || theirs == nullptr) throw NullReferenceException();
    if (!(theirs->vessel != mine->vessel) && !(theirs->partInfo->name != mine->partInfo->name)) {
        if (leadModule_ == nullptr) leadModule_ = other;
        if (!(other != leadModule_) && autoSwitch() && other->runningPrimary() != runningPrimary()) ToggleMode();
    }
}

// MultiModeEngine.cs:104
void MultiModeEngine::onVesselModified(Vessel* v) {
    Part* mine = dynamic_cast<Part*>(part);
    if (mine != nullptr && v == mine->vessel) leadModule_ = nullptr;
}

// MultiModeEngine.cs:113
void MultiModeEngine::ModeAction(KSPActionParam&) { ModeEvent(); }
// MultiModeEngine.cs:119
void MultiModeEngine::DisableAutoSwitch() {
    Set<bool>("autoSwitch", false);
    Events()["DisableAutoSwitch"]->active = false;
    Events()["EnableAutoSwitch"]->active = true;
}
// MultiModeEngine.cs:127
void MultiModeEngine::EnableAutoSwitch() {
    Set<bool>("autoSwitch", true);
    Events()["DisableAutoSwitch"]->active = true;
    Events()["EnableAutoSwitch"]->active = false;
}
// MultiModeEngine.cs:135
void MultiModeEngine::ModeEvent() {
    DisableAutoSwitch();
    ToggleMode();
}
// MultiModeEngine.cs:141
void MultiModeEngine::ToggleMode() {
    if (!runningPrimary()) SetPrimary(ResourceFlowEnvironment::LoadedSceneIsFlight());
    else SetSecondary(ResourceFlowEnvironment::LoadedSceneIsFlight());
}
// MultiModeEngine.cs:154
void MultiModeEngine::ShutdownAction(KSPActionParam&) {
    if (runningPrimary()) primaryEngine_->Shutdown();
    else secondaryEngine_->Shutdown();
}
// MultiModeEngine.cs:167
void MultiModeEngine::ActivateAction(KSPActionParam&) {
    if (runningPrimary()) primaryEngine_->Activate();
    else secondaryEngine_->Activate();
}
// MultiModeEngine.cs:180
void MultiModeEngine::OnAction(KSPActionParam&) {
    ModuleEnginesFX* e = runningPrimary() ? primaryEngine_ : secondaryEngine_;
    if (e->EngineIgnited()) e->Shutdown();
    else e->Activate();
}

// MultiModeEngine.cs:203
void MultiModeEngine::SetPrimary(bool fireEvents) {
    if (loadFailure_) return;
    if (fireEvents) {
        if (!primaryEngine_->EngineIgnited() && secondaryEngine_->EngineIgnited()) primaryEngine_->Activate();
        if (Get<bool>("carryOverThrottle")) primaryEngine_->Set<float>("currentThrottle", secondaryEngine_->currentThrottle());
        if (secondaryEngine_->EngineIgnited()) secondaryEngine_->Shutdown();
    }
    const std::string primaryName = Get<std::string>("primaryEngineModeDisplayName");
    Set<std::string>("mode", primaryName.empty() ? Get<std::string>("primaryEngineID") : primaryName);
    primaryEngine_->Set<bool>("manuallyOverridden", false);
    primaryEngine_->isEnabled = true;
    secondaryEngine_->Set<bool>("manuallyOverridden", true);
    secondaryEngine_->isEnabled = false;
    Set<bool>("runningPrimary", true);
    GameEvents::onMultiModeEngineSwitchActive.Fire(this);
}

// MultiModeEngine.cs:238
void MultiModeEngine::SetSecondary(bool fireEvents) {
    if (loadFailure_) return;
    if (fireEvents) {
        if (!secondaryEngine_->EngineIgnited() && primaryEngine_->EngineIgnited()) secondaryEngine_->Activate();
        if (Get<bool>("carryOverThrottle")) secondaryEngine_->Set<float>("currentThrottle", primaryEngine_->currentThrottle());
        if (primaryEngine_->EngineIgnited()) primaryEngine_->Shutdown();
    }
    const std::string secondaryName = Get<std::string>("secondaryEngineModeDisplayName");
    Set<std::string>("mode", secondaryName.empty() ? Get<std::string>("secondaryEngineID") : secondaryName);
    primaryEngine_->Set<bool>("manuallyOverridden", true);
    primaryEngine_->isEnabled = false;
    secondaryEngine_->Set<bool>("manuallyOverridden", false);
    secondaryEngine_->isEnabled = true;
    Set<bool>("runningPrimary", false);
    GameEvents::onMultiModeEngineSwitchActive.Fire(this);
}

// MultiModeEngine.cs:309
void MultiModeEngine::OnStart(StartState) {
    Part* p = dynamic_cast<Part*>(part);
    if (p == nullptr) throw NullReferenceException();
    PartModuleList& mods = p->Modules();
    for (size_t i = 0; i < mods.Count(); i++) {
        if (auto* tempEngine = dynamic_cast<ModuleEnginesFX*>(mods.GetModule(i))) {
            if (tempEngine->Get<std::string>("engineID") == Get<std::string>("primaryEngineID")) primaryEngine_ = tempEngine;
            else if (tempEngine->Get<std::string>("engineID") == Get<std::string>("secondaryEngineID")) secondaryEngine_ = tempEngine;
        }
    }
    if (primaryEngine_ != nullptr && secondaryEngine_ != nullptr) {
        // for (j < secondaryEngine.Actions.Count) primaryEngine.Actions[j].active = false (the loop runs twice in C#)
        for (size_t j = 0; j < secondaryEngine_->Actions().Count(); j++) primaryEngine_->Actions()[j]->active = false;
        for (size_t k = 0; k < secondaryEngine_->Actions().Count(); k++) primaryEngine_->Actions()[k]->active = false;
        if (runningPrimary()) SetPrimary(false);
        else SetSecondary(false);
    } else {
        log::Error("Failed to find primary and secondary engine, check engine IDs and module definitions");
        loadFailure_ = true;
    }
    if (!Get<bool>("autoSwitchAvailable")) {
        ApplyAutoSwitchEvents();
    } else if (autoSwitch()) {
        Events()["DisableAutoSwitch"]->active = true;
        Events()["EnableAutoSwitch"]->active = false;
    } else {
        Events()["DisableAutoSwitch"]->active = false;
        Events()["EnableAutoSwitch"]->active = true;
    }
}

// MultiModeEngine.cs:380
void MultiModeEngine::OnUpdate() {
    if (loadFailure_) return;
    if (leadModule_ != nullptr) {
        if (!leadModule_->autoSwitch()) leadModule_ = nullptr;
        else if (leadModule_ != this) return;
    }
    if (!autoSwitch()) return;
    if (runningPrimary()) {
        if (primaryEngine_->flameout() && secondaryEngine_->CanStart()) {
            SetSecondary(true);
            OnAutoSwitchModes().Fire(this);
        }
    } else if (secondaryEngine_->flameout() && primaryEngine_->CanStart()) {
        SetPrimary(true);
        OnAutoSwitchModes().Fire(this);
    }
}

bool MultiModeEngine::isOperational() const { return runningPrimary() ? primaryEngine_->isOperational() : secondaryEngine_->isOperational(); }  // :44
float MultiModeEngine::normalizedOutput() const { return runningPrimary() ? primaryEngine_->normalizedOutput() : secondaryEngine_->normalizedOutput(); }  // :54
float MultiModeEngine::throttleSetting() const { return runningPrimary() ? primaryEngine_->throttleSetting() : secondaryEngine_->throttleSetting(); }  // :64

// ============================================================================================================================
namespace detail {
void RegisterEngineModules() {
    ModuleFactory::Register("ModuleEngines", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleEngines>(c)); });
    ModuleFactory::Register("ModuleEnginesFX", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleEnginesFX>(c)); });
    ModuleFactory::Register("MultiModeEngine", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<MultiModeEngine>(c)); });
}
}  // namespace detail

}  // namespace ksp
