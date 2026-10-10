// WP-28: BaseConverter, BaseDrill, ModuleResourceConverter, ModuleResourceHarvester, the asteroid / comet drills and the
// space-object info / resource / analysis modules. See ksp/modules/resource_processing.hpp.
#include "ksp/modules/resource_processing.hpp"

#include <algorithm>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
std::string Fmt(double v, int decimals) { return dotnet::FormatDoubleF(v, decimals); }  // ToString("0.00") on a finite value (UI text)
const PartResourceDefinition* DefByName(const std::string& n) { return PartResourceLibrary::Instance()->GetDefinition(n); }
// PartResourceLibrary.Instance.GetDefinition(string.GetHashCode())
const PartResourceDefinition* DefByHash(const std::string& n) { return PartResourceLibrary::Instance()->GetDefinition(static_cast<int32_t>(MonoStringHash(n))); }
void Post(const std::string& message) { ResourceProcessingEnvironment::screenMessages.push_back(message); }
}  // namespace

// ============================================ ResourceProcessingEnvironment ====================================================

double ResourceProcessingEnvironment::UniversalTime() {
    if (universalTime) return universalTime();
    if (ResourceEnvironment::universalTime) return ResourceEnvironment::universalTime();
    return Planetarium::GetUniversalTime();
}

void ResourceProcessingEnvironment::Reset() {
    vesselOf = nullptr;
    universalTime = nullptr;
    timeSinceLevelLoad = 10.0f;
    flightReady = true;
    coreTemperature = nullptr;
    animationGroupDeployed = nullptr;
    impactRaycast = nullptr;
    intakeForward = nullptr;
    biomeAt = nullptr;
    screenMessages.clear();
    overlay = OverlayGeneratorState{};
}

const ProcessingVessel* ResourceProcessingEnvironment::VesselOf(const Part* p) {
    if (p == nullptr || p->vessel == nullptr || !vesselOf) return nullptr;
    return vesselOf(*p);
}

// the real Vessel as a record (the host installs `vesselOf` with a lookup over this)
ProcessingVessel ResourceProcessingEnvironment::FromVessel(Vessel& v) {
    ProcessingVessel r;
    r.Landed = v.Landed;
    r.Splashed = v.Splashed;
    r.situation = v.situation;
    r.altitude = v.altitude;
    r.radarAltitude = v.radarAltitude;
    r.latitude = v.latitude;
    r.longitude = v.longitude;
    r.atmDensity = v.atmDensity;
    r.srfSpeed = v.srfSpeed;
    r.horizontalSrfSpeed = v.horizontalSrfSpeed;
    r.solarFlux = v.solarFlux;
    r.srf_vel_direction = v.srf_vel_direction;
    r.situation = v.situation;
    r.isActiveVessel = v.isActiveVessel();
    r.controlLevel = v.CurrentControlLevel();
    r.mainThrottle = v.ctrlState.mainThrottle;
    r.parts = v.parts;
    CelestialBody* b = (v.orbitDriver && v.orbitDriver->orbit) ? v.mainBody() : nullptr;  // Vessel.mainBody = orbit.referenceBody
    if (b != nullptr) {
        r.mainBody.flightGlobalsIndex = b->flightGlobalsIndex;
        r.mainBody.Radius = b->Radius;
        r.mainBody.sphereOfInfluence = b->sphereOfInfluence;
        r.mainBody.atmosphere = b->atmosphere;
        r.mainBody.bodyName = b->bodyName;
        r.mainBody.displayName = b->displayName();
    }
    return r;
}

// ============================================== ResourceRatio.Load / Save (ResourceRatio.cs; declared by WP-15) ===============

// ResourceRatio.cs:31 (`Ratio = float.Parse(..)`: a float widened to the double field)
void ResourceRatio::Load(const ConfigNode& node) {
    const auto rn = node.GetValue("ResourceName");
    ResourceName = rn ? *rn : std::string();
    if (node.HasValue("Ratio")) Ratio = static_cast<double>(dotnet::ParseSingle(*node.GetValue("Ratio")));
    if (node.HasValue("DumpExcess")) {
        DumpExcess = cs::BoolParse(*node.GetValue("DumpExcess"));
    }
    if (node.HasValue("FlowMode")) {
        try {
            FlowMode = AsFlowMode(static_cast<int32_t>(kEnum_ResourceFlowMode.Parse(*node.GetValue("FlowMode"))));
        } catch (const std::exception& ex) {
            log::Error("[ResourceRatio]: Error parsing flow mode, given mode: " + *node.GetValue("FlowMode") + ", exception " + ex.what());
            FlowMode = ResourceFlowMode::NULL_;
        }
    }
}

// ResourceRatio.cs:55
void ResourceRatio::Save(ConfigNode& node) const {
    node.AddValue("ResourceName", ResourceName);
    node.AddValue("Ratio", Ratio);
    node.AddValue("DumpExcess", DumpExcess);
    node.AddValue("FlowMode", kEnum_ResourceFlowMode.ToString(static_cast<int32_t>(FlowMode)));
}

// ============================================== BaseConverter ==================================================================

Part* BaseConverter::GetPart() const { return dynamic_cast<Part*>(part); }
const ProcessingVessel* BaseConverter::Vessel() const { return ResourceProcessingEnvironment::VesselOf(GetPart()); }

// ResourceSetup.Instance.ResConfig.ShowDebugOptions (ResConfig is null when the database has no RESOURCE_CONFIGURATION node: NRE)
bool BaseConverter::ShowDebugOptions() {
    ResourceSetup::ResourceConfig* c = ResourceSetup::Instance().ResConfig();
    if (c == nullptr) throw Nre();
    return c->ShowDebugOptions;
}

// BaseConverter.cs:93
ResourceConverter& BaseConverter::ResConverter() {
    if (!_resConverter) _resConverter = std::make_unique<ResourceConverter>(&ResBroker());
    return *_resConverter;
}

// BaseConverter.cs:95
IResourceBroker& BaseConverter::ResBroker() {
    if (_resBroker == nullptr) {
        ownedBroker_ = std::make_unique<ResourceBroker>();
        _resBroker = ownedBroker_.get();
    }
    return *_resBroker;
}

// BaseConverter.cs:97
void BaseConverter::UpdateDebugInfo(const ConverterResults& result, double deltaTime) {
    if (ShowDebugOptions()) {
        Set<std::string>("debugEffBonus", dotnet::FormatSingleG(Get<float>("EfficiencyBonus")));
        Set<std::string>("debugDelta", dotnet::FormatDoubleG(deltaTime));
        Set<std::string>("debugTimeFac", dotnet::FormatDoubleG(result.TimeFactor));
    }
}

// BaseConverter.cs:108 (`float result = SpecialistBonusBase + (1f + (float)HasSpecialist(..)) * SpecialistEfficiencyFactor`: one store)
float BaseConverter::GetCrewEfficiencyBonus() {
    float result = 1.0f;
    if (Get<bool>("UseSpecialistBonus")) {
        const float spec = static_cast<float>(HasSpecialist(Get<std::string>("ExperienceEffect")));
        result = f32store(fx(Get<float>("SpecialistBonusBase")) + (1.0 + fx(spec)) * fx(Get<float>("SpecialistEfficiencyFactor")));
    }
    if (ShowDebugOptions()) Set<std::string>("debugCrewBon", dotnet::FormatSingleG(result));
    return result;
}

// BaseConverter.cs:122
float BaseConverter::GetCrewHeatBonus() {
    float result = 0.0f;
    if (Get<bool>("UseSpecialistHeatBonus")) {
        const float spec = static_cast<float>(HasSpecialist(Get<std::string>("ExperienceEffect")));
        result = f32store((1.0 + fx(spec)) * fx(Get<float>("SpecialistHeatFactor")));
        result = dotnet::Math::Min(result, 1.0f);
    }
    return result;
}

// BaseConverter.cs:134
void BaseConverter::SetupDebugging() {
    const bool show = ShowDebugOptions();
    for (const char* f : {"debugEffBonus", "debugDelta", "debugTimeFac", "debugFinBon", "debugCrewBon"}) Fields()[f]->guiActive = show;
}

// BaseConverter.cs:143
void BaseConverter::EnableModule() {
    isEnabled = true;
    Set<bool>("DirtyFlag", !Get<bool>("IsActivated"));
    UpdateConverterStatus();
}

// BaseConverter.cs:150
void BaseConverter::DisableModule() {
    isEnabled = false;
    Set<bool>("IsActivated", false);
    Set<bool>("DirtyFlag", !Get<bool>("IsActivated"));
    UpdateConverterStatus();
}

// BaseConverter.cs:158
bool BaseConverter::ModuleIsActive() { return Get<bool>("IsActivated"); }

// BaseConverter.cs:163
bool BaseConverter::IsAnimationGroupDeployed() {
    if (hasAnimationGroup && !(ResourceProcessingEnvironment::animationGroupDeployed ? ResourceProcessingEnvironment::animationGroupDeployed(*partAnimationModule) : true))
        return false;
    return true;
}

// BaseConverter.cs:173 (the screen message names the animation group's deploy action: not modelled, the message id is recorded)
void BaseConverter::StartResourceConverter() {
    if (hasAnimationGroup && !IsAnimationGroupDeployed()) {
        Post("#autoLOC_6010007");
    } else {
        Set<bool>("IsActivated", true);
    }
}

// BaseConverter.cs:188
void BaseConverter::StopResourceConverter() {
    Set<bool>("IsActivated", false);
    Set<std::string>("status", Localizer::Format("#autoLOC_257112"));
}

// BaseConverter.cs:196
void BaseConverter::StopResourceConverterAction(KSPActionParam&) { StopResourceConverter(); }

// BaseConverter.cs:202
void BaseConverter::StartResourceConverterAction(KSPActionParam&) { StartResourceConverter(); }

// BaseConverter.cs:208
void BaseConverter::ToggleResourceConverterAction(KSPActionParam&) {
    if (Get<bool>("IsActivated")) StopResourceConverter();
    else StartResourceConverter();
}

// BaseConverter.cs:221
void BaseConverter::SetEfficiencyBonus(float bonus) { Set<float>("EfficiencyBonus", bonus); }

// BaseConverter.cs:226
double BaseConverter::GetDeltaTime() {
    try {
        if (!(ResourceProcessingEnvironment::timeSinceLevelLoad < 1.0f) && ResourceProcessingEnvironment::flightReady) {
            if (dotnet::Math::Abs(lastUpdateTime) < 1E-09) {
                lastUpdateTime = ResourceProcessingEnvironment::UniversalTime();
                return -1.0;
            }
            double num = dotnet::Math::Min(ResourceProcessingEnvironment::UniversalTime() - lastUpdateTime, ResourceUtilities::GetMaxDeltaTime());
            double num2 = dotnet::Math::Min(static_cast<double>(TimeWarp::fixedDeltaTime()), 0.02);
            if (num > num2 && catchupRetries < MAX_CATCHUP_RETRIES) {
                double bestDeltaTime = GetBestDeltaTime(num);
                if (bestDeltaTime < num) {
                    catchupRetries++;
                    num = dotnet::Math::Max(num2, bestDeltaTime);
                }
            }
            lastUpdateTime += num;
            return num;
        }
        return -1.0;
    } catch (const std::exception& ex) {
        log::Error(std::string("[RESOURCES] - Error in - BaseConverter_GetDeltaTime - ") + ex.what());
        return 0.0;
    }
}

// BaseConverter.cs:264
double BaseConverter::GetBestDeltaTime(double deltaTime) {
    ConversionRecipe* conversionRecipe = PrepareRecipe(deltaTime);
    if (conversionRecipe == nullptr) return deltaTime;
    Part* p = GetPart();
    const int count = static_cast<int>(conversionRecipe->Inputs().size());
    double num = deltaTime;
    for (int i = 0; i < count; i++) {
        const ResourceRatio& resourceRatio = conversionRecipe->Inputs()[static_cast<size_t>(i)];
        const PartResourceDefinition* definition = DefByName(resourceRatio.ResourceName);
        if (definition == nullptr) throw Nre();
        double num2 = ResBroker().AmountAvailable(p, definition->id(), deltaTime, AsFlowMode(definition->resourceFlowMode()));
        double num3 = deltaTime * (num2 / (resourceRatio.Ratio * deltaTime));
        if (num3 < num) num = num3;
    }
    const int count2 = static_cast<int>(conversionRecipe->Outputs().size());
    for (int j = 0; j < count2; j++) {
        const ResourceRatio& resourceRatio2 = conversionRecipe->Outputs()[static_cast<size_t>(j)];
        const PartResourceDefinition* definition2 = DefByName(resourceRatio2.ResourceName);
        if (definition2 == nullptr) throw Nre();
        double num4 = ResBroker().StorageAvailable(p, definition2->id(), deltaTime, AsFlowMode(definition2->resourceFlowMode()), 1.0);
        double num5 = deltaTime * (num4 / (resourceRatio2.Ratio * deltaTime));
        if (num5 < num) num = num5;
    }
    return num;
}

// BaseConverter.cs:326 (cacheAutoLOC_257237 is Localizer.Format("#autoLOC_257237") taken once by CacheLocalStrings)
void BaseConverter::UpdateConverterStatus() {
    if (Get<bool>("DirtyFlag") != Get<bool>("IsActivated")) {
        Set<bool>("DirtyFlag", Get<bool>("IsActivated"));
        startEvt->active = !Get<bool>("IsActivated") && isEnabled;
        stopEvt->active = Get<bool>("IsActivated");
        if (Get<bool>("IsActivated")) Set<std::string>("status", Localizer::Format("#autoLOC_257237"));
        // MonoUtilities.RefreshPartContextWindow(part): UI
    }
}

// BaseConverter.cs:342
void BaseConverter::OnLoad(const ConfigNode& node) {
    Set<std::string>("status", Localizer::Format("#autoLOC_257112"));
    if (node.HasNode("INPUT_RESOURCE")) inputList.clear();
    if (node.HasNode("OUTPUT_RESOURCE")) outputList.clear();
    if (node.HasNode("REQUIRED_RESOURCE")) reqList.clear();
    const int countNodes = node.CountNodes();
    for (int i = 0; i < countNodes; i++) {
        ConfigNode* configNode = node.nodes()[i];
        ResourceRatio item;
        item.FlowMode = ResourceFlowMode::NULL_;
        const std::string& nm = configNode->name;
        if (!configNode->HasValue("ResourceName") && nm.size() >= 9 && nm.compare(nm.size() - 9, 9, "_RESOURCE") == 0) {
            log::Info("Resource must have value 'ResourceName'");
            continue;
        }
        item.Load(*configNode);
        if (nm == "REQUIRED_RESOURCE") reqList.push_back(item);
        else if (nm == "OUTPUT_RESOURCE") outputList.push_back(item);
        else if (nm == "INPUT_RESOURCE") inputList.push_back(item);
    }
    if (Vessel() == nullptr) return;
    try {
        PartModule::OnLoad(node);
        lastUpdateTime = ResourceUtilities::GetValue(node, "lastUpdateTime", lastUpdateTime);
        SetupModule();
    } catch (const std::exception& ex) {
        log::Error(std::string("[RESOURCES] - Error in - BaseConverter_OnLoad - ") + ex.what());
    }
}

// BaseConverter.cs:400
void BaseConverter::SetupLabels() {
    startEvt->SetGuiName(Get<std::string>("StartActionName"));
    startEvt->guiActiveEditor = true;
    startEvt->active = !Get<bool>("IsActivated") && isEnabled;
    stopEvt->SetGuiName(Get<std::string>("StopActionName"));
    stopEvt->guiActiveEditor = true;
    stopEvt->active = Get<bool>("IsActivated");
    Actions()["StartResourceConverterAction"]->SetGuiName(Get<std::string>("StartActionName"));
    Actions()["StopResourceConverterAction"]->SetGuiName(Get<std::string>("StopActionName"));
    Actions()["ToggleResourceConverterAction"]->SetGuiName(Get<std::string>("ToggleActionName"));
    const std::string resourceOutputName = Get<std::string>("resourceOutputName");
    if (!resourceOutputName.empty())
        Actions()["ToggleResourceConverterAction"]->SetGuiName(Actions()["ToggleResourceConverterAction"]->guiName() + " (" + resourceOutputName + ")");
    Fields()["status"]->guiName = Get<std::string>("ConverterName");
    Fields()["status"]->guiActiveEditor = true;
}

// BaseConverter.cs:420
void BaseConverter::SetupModule() {
    try {
        Fields()["status"]->guiName = Get<std::string>("ConverterName");
        Part* p = GetPart();
        if (p == nullptr) throw Nre();
        if (!p->Modules().Contains("ModuleAnimationGroup")) EnableModule();
    } catch (const std::exception& ex) {
        log::Info(std::string("[RESOURCES] - Error in - BaseConverter_SetupModule - ") + ex.what());
    }
}

// BaseConverter.cs:437
void BaseConverter::OnStart(StartState state) {
    SetupDebugging();
    SetupLabels();
    catchupRetries = 0;
    EfficiencyModifiers.clear();
    efficiencyModifiersCreated = true;
    TallyEfficiencyModifiers();
    Set<bool>("DirtyFlag", !Get<bool>("IsActivated"));
    if (Vessel() == nullptr) return;
    try {
        PartModule::OnStart(state);
        SetupModule();
        UpdateConverterStatus();
        partAnimationModule = GetPart()->FindModuleOfClass("ModuleAnimationGroup");
        hasAnimationGroup = partAnimationModule != nullptr;
    } catch (const std::exception& ex) {
        log::Error(std::string("[RESOURCES] - Error in - BaseConverter_OnStart - ") + ex.what());
    }
}

// BaseConverter.cs:460 (the curve defaults were WP-06's stand-in; the lists and the two events are added here)
void BaseConverter::OnAwake() {
    listsCreated = true;
    if (IsNull("ThermalEfficiency")) {
        FloatCurve c;
        c.Add(0.0f, 1.0f);
        Set<FloatCurve>("ThermalEfficiency", std::move(c));
    }
    if (IsNull("TemperatureModifier")) {
        FloatCurve c;
        c.Add(0.0f, 100.0f);
        Set<FloatCurve>("TemperatureModifier", std::move(c));
    }
    startEvt = Events()["StartResourceConverter"];
    stopEvt = Events()["StopResourceConverter"];
    Events().Bind("StartResourceConverter", BaseEvent::Handler([this] { StartResourceConverter(); }));
    Events().Bind("StopResourceConverter", BaseEvent::Handler([this] { StopResourceConverter(); }));
    Actions().Bind("StopResourceConverterAction", [this](KSPActionParam& p) { StopResourceConverterAction(p); });
    Actions().Bind("StartResourceConverterAction", [this](KSPActionParam& p) { StartResourceConverterAction(p); });
    Actions().Bind("ToggleResourceConverterAction", [this](KSPActionParam& p) { ToggleResourceConverterAction(p); });
}

// BaseConverter.cs:486
void BaseConverter::OnSave(ConfigNode& node) {
    PartModule::OnSave(node);
    node.AddValue("lastUpdateTime", lastUpdateTime);
}

// Unity Instantiate of the prefab component: `public List<ResourceRatio> inputList / outputList / reqList` are serialised
void BaseConverter::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* f = dynamic_cast<const BaseConverter*>(&from)) {
        inputList = f->inputList;
        outputList = f->outputList;
        reqList = f->reqList;
    }
}

void ModuleResourceHarvester::CopySerializedFrom(const PartModule& from) {
    BaseDrill::CopySerializedFrom(from);
    if (const auto* f = dynamic_cast<const ModuleResourceHarvester*>(&from)) {
        intakeTransformName = f->intakeTransformName;
        eInput = f->eInput;
        hasEInput = f->hasEInput;
    }
}

void ModuleSpaceObjectResource::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* f = dynamic_cast<const ModuleSpaceObjectResource*>(&from)) _flowMode = f->_flowMode;
}

void ModuleSpaceObjectInfo::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* f = dynamic_cast<const ModuleSpaceObjectInfo*>(&from)) {
        totalResourceMass = f->totalResourceMass;
        totalResourcePercent = f->totalResourcePercent;
    }
}

// BaseConverter.cs:493 (EfficiencyBonus * heatThrottle * _totalEfficiencyModifiers * GetCrewEfficiencyBonus(): float operands, double
// result, so one double product with no float store)
double BaseConverter::GetEfficiencyMultiplier() {
    const float heatThrottle = GetHeatThrottle();
    return fx(Get<float>("EfficiencyBonus")) * fx(heatThrottle) * fx(_totalEfficiencyModifiers) * fx(GetCrewEfficiencyBonus());
}

// BaseConverter.cs:500
void BaseConverter::FixedUpdate() {
    if (ResourceScenario::Instance() != nullptr && Vessel() != nullptr) {
        PreProcessing();
        const double deltaTime = GetDeltaTime();
        if (deltaTime < 1E-09 && !ResourceFlowEnvironment::LoadedSceneIsEditor()) return;
        if (Get<bool>("AlwaysActive")) Set<bool>("IsActivated", true);
        UpdateConverterStatus();
        if (Get<bool>("IsActivated")) {
            ConversionRecipe* conversionRecipe = PrepareRecipe(deltaTime);
            if (conversionRecipe != nullptr) {
                conversionRecipe->FillAmount = Get<float>("FillAmount");
                conversionRecipe->TakeAmount = Get<float>("TakeAmount");
                double num = 1.0;
                if (!_preCalculateEfficiency) num = GetEfficiencyMultiplier();
                if (ShowDebugOptions()) Set<std::string>("debugFinBon", dotnet::FormatDoubleG(num));
                if (!ResourceFlowEnvironment::LoadedSceneIsEditor()) {
                    ConverterResults result = ResConverter().ProcessRecipe(deltaTime, *conversionRecipe, GetPart(), this, static_cast<float>(num));
                    PostProcess(result, deltaTime);
                    if (ShowDebugOptions()) UpdateDebugInfo(result, deltaTime);
                    const double num2 = result.TimeFactor / deltaTime;
                    if (dotnet::Math::Abs(num2 - lastTimeFactor) > 1E-09) {
                        lastTimeFactor = num2;
                        GE::OnEfficiencyChange.Fire(this, GetPart(), lastTimeFactor);
                    }
                }
            }
        }
        CheckForShutdown();
        PostUpdateCleanup();
    } else if (ResourceFlowEnvironment::LoadedSceneIsEditor()) {
        UpdateConverterStatus();
    }
}

// BaseConverter.cs:568 (`_totalEfficiencyModifiers *= value`: float store per step)
void BaseConverter::TallyEfficiencyModifiers() {
    _totalEfficiencyModifiers = 1.0f;
    if (!efficiencyModifiersCreated) throw Nre();
    for (const auto& kv : EfficiencyModifiers) _totalEfficiencyModifiers = f32store(fx(_totalEfficiencyModifiers) * fx(kv.second));
}

// BaseConverter.cs:578. KSP quirk kept: a requirement whose density exceeds 1E-09 is added to the OUTPUT list (list2), not to the
// requirement list; only low density requirements stay requirements.
void BaseConverter::ConvertRecipeToUnits(ConversionRecipe& recipe) {
    const int count = static_cast<int>(recipe.Inputs().size());
    const int count2 = static_cast<int>(recipe.Outputs().size());
    const int count3 = static_cast<int>(recipe.Requirements().size());
    std::vector<ResourceRatio> list, list2, list3;
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    auto def = [&](const std::string& n) {
        const PartResourceDefinition* d = lib->resourceDefinitions[n];
        if (d == nullptr) throw Nre();
        return d;
    };
    for (int i = 0; i < count; i++) {
        const ResourceRatio& in = recipe.Inputs()[static_cast<size_t>(i)];
        const PartResourceDefinition* d = def(in.ResourceName);
        if (static_cast<double>(d->density()) > 1E-09) list.push_back(ResourceRatio(in.ResourceName, in.Ratio / static_cast<double>(d->density()), in.DumpExcess, in.FlowMode));
        else list.push_back(in);
    }
    for (int j = 0; j < count2; j++) {
        const ResourceRatio& out = recipe.Outputs()[static_cast<size_t>(j)];
        const PartResourceDefinition* d = def(out.ResourceName);
        if (static_cast<double>(d->density()) > 1E-09) list2.push_back(ResourceRatio(out.ResourceName, out.Ratio / static_cast<double>(d->density()), out.DumpExcess, out.FlowMode));
        else list2.push_back(out);
    }
    for (int k = 0; k < count3; k++) {
        const ResourceRatio& rq = recipe.Requirements()[static_cast<size_t>(k)];
        const PartResourceDefinition* d = def(rq.ResourceName);
        if (static_cast<double>(d->density()) > 1E-09) list2.push_back(ResourceRatio(rq.ResourceName, rq.Ratio / static_cast<double>(d->density()), rq.DumpExcess, rq.FlowMode));  // sic: list2
        else list3.push_back(rq);
    }
    recipe.SetInputs(&list);
    recipe.SetOutputs(&list2);
    recipe.SetRequirements(&list3);
}

// BaseConverter.cs:640
float BaseConverter::GetHeatThrottle() {
    if (!Get<bool>("IsActivated")) return 1.0f;
    Part* p = GetPart();
    if (p == nullptr) throw Nre();
    FloatCurve ThermalEfficiency = Get<FloatCurve>("ThermalEfficiency");
    if (PartModule* core = p->FindModuleOfClass("ModuleCoreHeat")) {
        const double ct = ResourceProcessingEnvironment::coreTemperature ? ResourceProcessingEnvironment::coreTemperature(*core) : 0.0;
        return ThermalEfficiency.Evaluate(static_cast<float>(ct));
    }
    return ThermalEfficiency.Evaluate(static_cast<float>(p->fields.temperature));
}

// BaseConverter.cs:657 (the screen message is recorded)
void BaseConverter::CheckForShutdown() {
    if (Get<bool>("AutoShutdown") && Get<bool>("GeneratesHeat")) {
        Part* p = GetPart();
        double num = p->fields.maxTemp * static_cast<double>(Get<float>("DefaultShutoffTemp"));
        if (p->fields.temperature > num) {
            Post(Localizer::Format("#autoLOC_257466"));
            StopResourceConverter();
        }
    }
}

// BaseConverter.cs:671
int BaseConverter::HasSpecialist(const std::string& effect) {
    int num = -1;
    const ProcessingVessel* v = Vessel();
    if (v == nullptr) throw Nre();
    const auto& vesselCrew = v->crew;
    if (!vesselCrew.empty()) {
        int count = static_cast<int>(vesselCrew.size());
        while (count-- > 0) {
            const ProcessingCrewMember& m = vesselCrew[static_cast<size_t>(count)];
            if (m.effects.count(effect) != 0 && m.experienceLevel > num) num = m.experienceLevel;
            if (num == 5) break;
        }
    }
    return num;
}

// BaseConverter.cs:692
bool BaseConverter::IsOverheating() {
    Part* p = GetPart();
    if (PartModule* core = p->FindModuleOfClass("ModuleCoreHeat")) {
        const double ct = ResourceProcessingEnvironment::coreTemperature ? ResourceProcessingEnvironment::coreTemperature(*core) : 0.0;
        return ct > p->fields.temperature;
    }
    return false;
}

// BaseConverter.cs:711
void BaseConverter::PostProcess(const ConverterResults& result, double deltaTime) {
    if (result.TimeFactor <= 1E-09) {
        Set<std::string>("status", result.Status);
        statusPercent = 0.0;
        return;
    }
    const double num = dotnet::Math::Round(result.TimeFactor / deltaTime * 100.0, 2, dotnet::MidpointRounding::ToEven);
    if (!Mathf::Approximately(static_cast<float>(statusPercent), static_cast<float>(num))) {
        statusPercent = num;
        if (ResourceFlowEnvironment::partActionUIContains && ResourceFlowEnvironment::partActionUIContains(GetPart()))
            Set<std::string>("status", Localizer::Format("#autoLOC_257529", {Fmt(statusPercent, 2)}));
    }
}

// BaseConverter.cs:738
ConversionRecipe* BaseConverter::PrepareRecipe(double) {
    log::Info("[RESOURCES] No Implementation of PrepareRecipe in derived class");
    return nullptr;
}

// BaseConverter.cs:745
bool BaseConverter::IsSituationValid() {
    log::Info("[RESOURCES] No Implementation of IsSituationValid in derived class");
    return true;
}

// BaseConverter.cs:751
void BaseConverter::OnOverheat(double) {}

// BaseConverter.cs:763
bool BaseConverter::DisplayCoreHeat() { return GetPart()->FindModuleOfClass("ModuleCoreHeat") != nullptr; }

// BaseConverter.cs:768
double BaseConverter::GetCoreTemperature() {
    Part* p = GetPart();
    PartModule* core = p->FindModuleOfClass("ModuleCoreHeat");
    if (core == nullptr) return p->fields.temperature;
    return ResourceProcessingEnvironment::coreTemperature ? ResourceProcessingEnvironment::coreTemperature(*core) : 0.0;
}

// BaseConverter.cs:778 (CoreTempGoal + CoreTempGoalAdjustment)
double BaseConverter::GetGoalTemperature() {
    Part* p = GetPart();
    PartModule* core = p->FindModuleOfClass("ModuleCoreHeat");
    if (core == nullptr) return p->fields.temperature;
    return core->Get<double>("CoreTempGoal") + core->Get<double>("CoreTempGoalAdjustment");
}

// BaseConverter.cs:803
std::string BaseConverter::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003020"); }

// BaseConverter.cs:808 (the cached strings are read through Localizer.Format at use: the cache holds the same text)
void BaseConverter::CacheLocalStrings() {}

// BaseDrill.cs:14
std::string BaseDrill::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003021"); }

// ============================================== ModuleResourceConverter ========================================================

// ModuleResourceConverter.cs:13
ConversionRecipe& ModuleResourceConverter::Recipe() {
    if (!_recipe) _recipe = LoadRecipe();
    return *_recipe;
}

// ModuleResourceConverter.cs:15
ConversionRecipe* ModuleResourceConverter::PrepareRecipe(double) {
    UpdateConverterStatus();
    if (!Get<bool>("IsActivated")) return nullptr;
    return &Recipe();
}

// ModuleResourceConverter.cs:25
std::unique_ptr<ConversionRecipe> ModuleResourceConverter::LoadRecipe() {
    auto conversionRecipe = std::make_unique<ConversionRecipe>();
    try {
        for (const auto& r : inputList) conversionRecipe->Inputs().push_back(r);
        for (const auto& r : outputList) conversionRecipe->Outputs().push_back(r);
        for (const auto& r : reqList) conversionRecipe->Requirements().push_back(r);
        if (Get<bool>("ConvertByMass")) ConvertRecipeToUnits(*conversionRecipe);
    } catch (const std::exception&) {
        log::Info("[RESOURCES] Error creating recipe");
    }
    return conversionRecipe;
}

// ModuleResourceConverter.cs:125
std::string ModuleResourceConverter::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003053"); }

// ============================================== ModuleResourceHarvester ========================================================

// ModuleResourceHarvester.cs:154
void ModuleResourceHarvester::OnStart(StartState state) {
    BaseDrill::OnStart(state);
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        _isValidSituation = IsSituationValid();
        if (ResourceFlowEnvironment::LoadedSceneIsFlight()) isEnabled = _isValidSituation;
        const PartResourceDefinition* definition = DefByHash(Get<std::string>("ResourceName"));
        if (definition == nullptr) throw Nre();
        Fields()["ResourceStatus"]->guiName = Localizer::Format("#autoLOC_6001918", {definition->displayName()});
        // impactTransformCache / intakeTransform = part.FindModelTransform(..): the model's transforms (host: hasImpactTransform)
    }
}

// ModuleResourceHarvester.cs:176
ConversionRecipe* ModuleResourceHarvester::PrepareRecipe(double deltaTime) {
    try {
        if (!ResourceFlowEnvironment::LoadedSceneIsFlight()) return nullptr;
        const ProcessingVessel* v = Vessel();
        if (v == nullptr) throw Nre();
        const int HarvesterType = Get<int32_t>("HarvesterType");
        if (HarvesterType == 0 && v->Landed && v->horizontalSrfSpeed > 10.0 && v->isActiveVessel &&
            deltaTime < static_cast<double>(ResourceScenario::Instance()->gameSettings()->MaxDeltaTime)) {
            Set<std::string>("status", Localizer::Format("#autoLOC_259762"));
            Set<bool>("IsActivated", false);
            return nullptr;
        }
        if (HarvesterType == 0 && !v->Landed && v->radarAltitude > 1000.0) {
            Set<std::string>("status", Localizer::Format("#autoLOC_259775"));
            Set<bool>("IsActivated", false);
            return nullptr;
        }
        if (HarvesterType == 1 && !v->Splashed) {
            Set<std::string>("status", Localizer::Format("#autoLOC_259782"));
            Set<bool>("IsActivated", false);
            return nullptr;
        }
        if (!CheckForImpact() && deltaTime / 2.0 < static_cast<double>(TimeWarp::fixedDeltaTime())) {
            Set<std::string>("status", Localizer::Format("#autoLOC_259789"));
            Set<bool>("IsActivated", false);
            return nullptr;
        }
        AbundanceRequest request;
        request.Altitude = v->altitude;
        request.BodyId = v->mainBody.flightGlobalsIndex;  // FlightGlobals.currentMainBody.flightGlobalsIndex
        request.CheckForLock = false;
        request.Latitude = v->latitude;
        request.Longitude = v->longitude;
        request.ResourceType = static_cast<HarvestTypes>(HarvesterType);
        request.ResourceName = Get<std::string>("ResourceName");
        if (ResourceMap::Instance() == nullptr) throw Nre();
        const float abundance = ResourceMap::Instance()->GetAbundance(request);
        if (static_cast<double>(abundance) < 1E-09) {
            Set<std::string>("status", Localizer::Format("#autoLOC_259808"));
            Set<bool>("IsActivated", false);
            return nullptr;
        }
        const float HarvestThreshold = Get<float>("HarvestThreshold");
        if (abundance < HarvestThreshold) {
            Post(Localizer::Format("#autoLOC_6001069", {Fmt(f32store(fx(HarvestThreshold) * 100.0), 1)}));
            Set<std::string>("status", Localizer::Format("#autoLOC_259819"));
            Set<bool>("IsActivated", false);
            return nullptr;
        }
        if (!Get<bool>("IsActivated")) {
            Set<std::string>("status", Localizer::Format("#autoLOC_259826"));
            return nullptr;
        }
        float num = f32store(fx(abundance) * fx(Get<float>("Efficiency")));
        if (HarvesterType == 2) num = f32store(fx(num) * fx(static_cast<float>(GetIntakeMultiplier())));
        _resFlow = num;
        LoadRecipe(num);
        return &recipe;
    } catch (const std::exception& ex) {
        log::Error(std::string("[RESOURCES] - Error in - ModuleCrustalHarvester_ConversionRecipe - ") + ex.what());
        return nullptr;
    }
}

// ModuleResourceHarvester.cs:273 (intakeTransform.forward: the model's transform, host hook; none = multiplier 1)
double ModuleResourceHarvester::GetIntakeMultiplier() {
    const ProcessingVessel* v = Vessel();
    if (v == nullptr) throw Nre();
    double num = v->atmDensity;
    if (Get<int32_t>("HarvesterType") == 3) num = 1.0;
    double num2 = v->srfSpeed + Get<double>("airSpeedStatic");
    if (GetPart()->ShieldedFromAirstream()) return 0.0;
    double num3 = 1.0;
    if (ResourceProcessingEnvironment::intakeForward) {
        if (std::optional<Vector3f> fwd = ResourceProcessingEnvironment::intakeForward(*this)) {
            const Vector3f dir(static_cast<float>(v->srf_vel_direction.x), static_cast<float>(v->srf_vel_direction.y),
                               static_cast<float>(v->srf_vel_direction.z));
            num3 = UtilMath::Clamp01(static_cast<double>(Vector3f::Dot(dir, *fwd)));
        }
    }
    double num4 = num2 * num;
    return num3 * num4;
}

// ModuleResourceHarvester.cs:298
void ModuleResourceHarvester::PostUpdateCleanup() {
    if (Get<bool>("IsActivated")) {
        if (_displayedResFlow != _resFlow) {
            _displayedResFlow = _resFlow;
            if (ResourceFlowEnvironment::partActionUIContains && ResourceFlowEnvironment::partActionUIContains(GetPart()))
                Set<std::string>("ResourceStatus", Localizer::Format("#autoLOC_259857", {Fmt(_displayedResFlow, 6)}));
        }
    } else {
        Set<std::string>("ResourceStatus", Localizer::Format("#autoLOC_259862"));
        _displayedResFlow = -1.0;
    }
}

// ModuleResourceHarvester.cs:321
void ModuleResourceHarvester::LoadRecipe(double harvestRate) {
    recipe.Clear();
    try {
        for (const auto& r : inputList) recipe.Inputs().push_back(r);
        const bool dumpExcess = Get<int32_t>("HarvesterType") == 2;
        ResourceRatio out;
        out.ResourceName = Get<std::string>("ResourceName");
        out.Ratio = harvestRate;
        out.DumpExcess = dumpExcess;
        out.FlowMode = ResourceFlowMode::NULL_;
        recipe.Outputs().push_back(out);
        hasEInput = false;
        int count = static_cast<int>(recipe.Inputs().size());
        do {
            if (count-- <= 0) return;
        } while (!(recipe.Inputs()[static_cast<size_t>(count)].ResourceName == "ElectricCharge"));
        eInput = recipe.Inputs()[static_cast<size_t>(count)];
        hasEInput = true;
    } catch (const std::exception&) {
        log::Info("[RESOURCES] Error preparing recipe");
    }
}

// ModuleResourceHarvester.cs:359
void ModuleResourceHarvester::PostProcess(const ConverterResults& result, double deltaTime) {
    if (Get<bool>("CausesDepletion")) {
        const ProcessingVessel* v = Vessel();
        if (v == nullptr || ResourceMap::Instance() == nullptr) throw Nre();
        const float num = static_cast<float>(dotnet::Math::Min(1.0, result.TimeFactor / deltaTime));
        const Vector2f depletionNode = ResourceMap::Instance()->GetDepletionNode(v->latitude, v->longitude);
        const std::string rn = Get<std::string>("ResourceName");
        const int x = static_cast<int>(depletionNode.x), y = static_cast<int>(depletionNode.y);
        const float depletionNodeValue = ResourceMap::Instance()->GetDepletionNodeValue(v->mainBody.flightGlobalsIndex, rn, x, y);
        const float num2 = f32store(fx(Get<float>("DepletionRate")) * fx(num));
        const float value = f32store(fx(depletionNodeValue) - fx(depletionNodeValue) * fx(num2));
        ResourceMap::Instance()->SetDepletionNodeValue(v->mainBody.flightGlobalsIndex, rn, x, y, value);
    }
    BaseDrill::PostProcess(result, deltaTime);
    if (hasEInput && !(eInput.Ratio < 1E-09)) {
        const double num3 = eInput.Ratio * deltaTime;
        if (_resBroker == nullptr) throw Nre();
        const double num4 = _resBroker->AmountAvailable(GetPart(), std::string("ElectricCharge"), deltaTime, ResourceFlowMode::NULL_);
        if (num3 > num4) {
            Post(Localizer::Format("#autoLOC_259933"));
            StopResourceConverter();
        }
    }
}

// ModuleResourceHarvester.cs:402 (Physics.Raycast(impact ray, ImpactRange, layer 32768): host hook)
bool ModuleResourceHarvester::CheckForImpact() {
    if (!Get<std::string>("ImpactTransform").empty() && hasImpactTransform)
        return ResourceProcessingEnvironment::impactRaycast ? ResourceProcessingEnvironment::impactRaycast(*this) : true;
    return true;
}

// ModuleResourceHarvester.cs:413
bool ModuleResourceHarvester::IsSituationValid() {
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight()) return false;
    const ProcessingVessel* v = Vessel();
    if (v == nullptr) throw Nre();
    const int count = static_cast<int>(v->parts.size());
    if (count != partCountCache) {
        partCountCache = count;
        int index = count;
        while (index-- > 0) {
            Part* p = v->parts[static_cast<size_t>(index)];
            int count2 = static_cast<int>(p->Modules().Count());
            while (count2-- > 0) {
                PartModule* m = p->Modules()[static_cast<size_t>(count2)];
                if (!m->IsA("ModuleAsteroid")) {
                    if (m->IsA("ModuleComet")) {
                        cachedWasNotComet = false;
                        return false;
                    }
                    continue;
                }
                cachedWasNotAsteroid = false;
                return false;
            }
        }
        cachedWasNotAsteroid = true;
        cachedWasNotComet = true;
    } else {
        if (!cachedWasNotAsteroid) return false;
        if (!cachedWasNotComet) return false;
    }
    switch (static_cast<HarvestTypes>(Get<int32_t>("HarvesterType"))) {
        default:
            return false;
        case HarvestTypes::Planetary: {
            bool result;
            if ((result = v->situation == VesselSituations::LANDED)) return result;
            if (v->radarAltitude <= 500.0) result = true;
            return result;
        }
        case HarvestTypes::Oceanic:
            return v->situation == VesselSituations::SPLASHED;
        case HarvestTypes::Atmospheric:
            return v->atmDensity > 1E-09;
        case HarvestTypes::Exospheric:
            return v->situation == VesselSituations::ORBITING;
    }
}

// ModuleResourceHarvester.cs:466 (base.OnUpdate() is the empty PartModule.OnUpdate)
void ModuleResourceHarvester::PreProcessing() {
    const bool flag = isEnabled;
    _isValidSituation = IsSituationValid();
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) isEnabled = _isValidSituation;
    (void)flag;  // MonoUtilities.RefreshPartContextWindow when it changed: UI
    PartModule::OnUpdate();
}

// ModuleResourceHarvester.cs:482
std::vector<const PartResourceDefinition*> ModuleResourceHarvester::GetConsumedResources() { return {DefByName("ElectricCharge")}; }

// ModuleResourceHarvester.cs:487
std::string ModuleResourceHarvester::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003054"); }

void ModuleResourceHarvester::CacheLocalStrings() {}

// ============================================== asteroid / comet drills =========================================================

// ModuleAsteroidDrill.cs:45 / ModuleCometDrill.cs:45
ConversionRecipe* ModuleSpaceObjectDrillBase::PrepareRecipe(double deltaTime) {
    _status = "Connected";
    _potato = GetAttachedPotato();
    _drilledMass = 0.0;
    auto fail = [&](const std::string& text) -> ConversionRecipe* {
        _status = text;
        Set<bool>("IsActivated", false);
        return nullptr;
    };
    if (_potato == nullptr) return fail(NoObjectStatus());
    Part* p = GetPart();
    if (Get<bool>("DirectAttach")) {
        const bool isChild = std::find(p->children.begin(), p->children.end(), _potato) != p->children.end();
        if (!isChild && !(p->parent == _potato)) return fail(NotAttachedStatus());
    }
    if (!Get<bool>("RockOnly") && !_potato->Modules().Contains(ResourceClass())) return fail(Localizer::Format("#autoLOC_258419"));
    std::vector<ModuleSpaceObjectResource*> list;
    for (PartModule* m : _potato->FindModulesImplementing([&](PartModule& pm) { return pm.IsA(ResourceClass()); }))
        list.push_back(static_cast<ModuleSpaceObjectResource*>(m));
    if (!CheckForImpact()) return fail(Localizer::Format("#autoLOC_258428"));
    _info = _potato->FindModuleOfClass(InfoClass());
    if (_info == nullptr) return fail(Localizer::Format("#autoLOC_258436"));
    ModuleSpaceObjectInfo* info = static_cast<ModuleSpaceObjectInfo*>(_info);
    if (info->massThresholdVal() >= info->currentMassVal()) return fail(Localizer::Format("#autoLOC_258443"));
    if (ResBroker().AmountAvailable(p, std::string("ElectricCharge"), deltaTime, ResourceFlowMode::NULL_) <= deltaTime * static_cast<double>(Get<float>("PowerConsumption")))
        return fail(Localizer::Format("#autoLOC_258451"));
    UpdateConverterStatus();
    if (!Get<bool>("IsActivated")) return nullptr;
    const double efficiencyMultiplier = GetEfficiencyMultiplier();
    bool flag = false;
    const int count = static_cast<int>(list.size());
    const double Efficiency = static_cast<double>(Get<float>("Efficiency"));
    for (int i = 0; i < count; i++) {
        ModuleSpaceObjectResource* r = list[static_cast<size_t>(i)];
        double num = ResBroker().StorageAvailable(p, r->Get<std::string>("resourceName"), deltaTime, r->_flowMode, static_cast<double>(Get<float>("FillAmount")));
        double num2 = deltaTime * static_cast<double>(r->Get<float>("abundance")) * Efficiency * efficiencyMultiplier;
        if (!(num <= num2)) {
            flag = true;
            break;
        }
    }
    recipe.Clear();
    if (flag) {
        ResourceRatio ec;
        ec.ResourceName = "ElectricCharge";
        ec.Ratio = static_cast<double>(Get<float>("PowerConsumption"));
        ec.FlowMode = ResourceFlowMode::NULL_;
        recipe.Inputs().push_back(ec);
        for (int j = 0; j < count; j++) {
            ModuleSpaceObjectResource* r = list[static_cast<size_t>(j)];
            if (!(static_cast<double>(r->Get<float>("abundance")) <= 1E-09)) {
                const PartResourceDefinition* definition = DefByName(r->Get<std::string>("resourceName"));
                if (definition == nullptr) throw Nre();
                const double val = deltaTime * static_cast<double>(r->Get<float>("abundance")) * Efficiency * efficiencyMultiplier;
                const double val2 = (info->currentMassVal() - info->massThresholdVal()) / static_cast<double>(definition->density());
                const double num3 = dotnet::Math::Min(val, val2);
                _drilledMass += static_cast<double>(definition->density()) * num3;
                ResourceRatio out;
                out.ResourceName = r->Get<std::string>("resourceName");
                out.Ratio = num3 / deltaTime;
                out.DumpExcess = true;
                out.FlowMode = ResourceFlowMode::NULL_;
                recipe.Outputs().push_back(out);
            }
        }
    } else {
        _status = Localizer::Format("#autoLOC_258501");
    }
    return &recipe;
}

// ModuleAsteroidDrill.cs:170 (the raycast and GetComponentUpwards<ModuleAsteroid>: host hook)
bool ModuleSpaceObjectDrillBase::CheckForImpact() {
    if (!(Get<std::string>("ImpactTransform").empty()) && hasImpactTransform)
        return ResourceProcessingEnvironment::impactRaycast ? ResourceProcessingEnvironment::impactRaycast(*this) : false;
    return true;
}

// ModuleAsteroidDrill.cs:190 (the vessel's parts from the last: the first with the object module)
Part* ModuleSpaceObjectDrillBase::GetAttachedPotato() {
    const ProcessingVessel* v = Vessel();
    if (v == nullptr) throw Nre();
    int count = static_cast<int>(v->parts.size());
    while (count-- > 0) {
        if (PartModule* m = v->parts[static_cast<size_t>(count)]->FindModuleOfClass(ObjectClass())) return dynamic_cast<Part*>(m->part);
    }
    return nullptr;
}

// ModuleAsteroidDrill.cs:214
bool ModuleSpaceObjectDrillBase::IsSituationValid() {
    const ProcessingVessel* v = Vessel();
    if (v == nullptr) throw Nre();
    int count = static_cast<int>(v->parts.size());
    while (count-- > 0) {
        Part* p = v->parts[static_cast<size_t>(count)];
        int count2 = static_cast<int>(p->Modules().Count());
        while (count2-- > 0)
            if (p->Modules()[static_cast<size_t>(count2)]->IsA(ObjectClass())) return true;
    }
    return false;
}

// ModuleAsteroidDrill.cs:234
void ModuleSpaceObjectDrillBase::OnUpdate() {
    bool isValidSituation;
    if ((isValidSituation = IsSituationValid()) != _isValidSituation) {
        _isValidSituation = isValidSituation;
        isEnabled = _isValidSituation;
    }
    if (!_isValidSituation && isEnabled) isEnabled = false;
    BaseDrill::OnUpdate();
}

// ModuleAsteroidDrill.cs:253
void ModuleSpaceObjectDrillBase::OnStart(StartState state) {
    BaseDrill::OnStart(state);
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        _isValidSituation = IsSituationValid();
        isEnabled = _isValidSituation;
        _preCalculateEfficiency = true;
    }
}

// ModuleAsteroidDrill.cs:260 PostProcess
void ModuleSpaceObjectDrillBase::PostProcess(const ConverterResults& result, double deltaTime) {
    BaseDrill::PostProcess(result, deltaTime);
    if (!(_drilledMass < 1E-09) && _potato != nullptr && _info != nullptr) {
        ModuleSpaceObjectInfo* info = static_cast<ModuleSpaceObjectInfo*>(_info);
        const double currentMassVal = info->currentMassVal() - _drilledMass;
        info->set_currentMassVal(currentMassVal);
        Set<std::string>("status", _status);
    }
}

std::string ModuleAsteroidDrill::NoObjectStatus() const { return Localizer::Format("#autoLOC_258405"); }       // :27 cacheAutoLOC_258405
std::string ModuleAsteroidDrill::NotAttachedStatus() const { return Localizer::Format("#autoLOC_258412"); }    // :28
std::string ModuleCometDrill::NoObjectStatus() const { return Localizer::Format("#autoLOC_6002530"); }        // ModuleCometDrill.cs:27
std::string ModuleCometDrill::NotAttachedStatus() const { return Localizer::Format("#autoLOC_6002531"); }     // :28
std::string ModuleAsteroidDrill::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003029"); }     // ModuleAsteroidDrill.cs:272
std::string ModuleCometDrill::GetModuleDisplayName() { return Localizer::Format("#autoLOC_6002533"); }        // ModuleCometDrill.cs:272
void ModuleAsteroidDrill::CacheLocalStrings() {}
void ModuleCometDrill::CacheLocalStrings() {}

// ============================================== space-object resource / info / analysis ========================================

// ModuleSpaceObjectResource.cs:28
void ModuleSpaceObjectResource::OnLoad(const ConfigNode& node) {
    PartModule::OnLoad(node);
    const std::string resourceName = Get<std::string>("resourceName");
    const PartResourceDefinition* definition = DefByName(resourceName);
    _flowMode = ResourceFlowMode::NULL_;
    if (definition != nullptr) _flowMode = AsFlowMode(definition->resourceFlowMode());
    const std::string FlowMode = Get<std::string>("FlowMode");
    if (!FlowMode.empty()) {
        try {
            _flowMode = AsFlowMode(static_cast<int32_t>(kEnum_ResourceFlowMode.Parse(FlowMode)));
        } catch (const std::exception& ex) {
            log::Error("[ModuleSpaceObjectResource]: Error parsing flow mode, given mode: " + FlowMode + ", exception " + ex.what());
        }
    }
}

// ModuleSpaceObjectInfo.cs:30 (double.TryParse: 0 on failure)
double ModuleSpaceObjectInfo::currentMassVal() const {
    double result = 0.0;
    if (!dotnet::TryParseDouble(Get<std::string>("currentMass"), result)) result = 0.0;
    return result;
}
// :41
void ModuleSpaceObjectInfo::set_currentMassVal(double v) { Set<std::string>("currentMass", dotnet::FormatDoubleG(v, 17)); }
// :46
double ModuleSpaceObjectInfo::massThresholdVal() const {
    double result = 0.0;
    if (!dotnet::TryParseDouble(Get<std::string>("massThreshold"), result)) result = 0.0;
    return result;
}
// :57
void ModuleSpaceObjectInfo::set_massThresholdVal(double v) { Set<std::string>("massThreshold", dotnet::FormatDoubleG(v, 17)); }

// ModuleSpaceObjectInfo.cs:62
void ModuleSpaceObjectInfo::SetupSpaceObjectResources(const std::vector<ModuleSpaceObjectResource*>& resInfoList) {
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight()) return;
    KSPRandom kSPRandom(ResourceEnvironment::TickCount());
    struct ResourceDataLocal {
        std::string Name;
        float Mass = 0.0f, Purity = 0.0f;
    };
    std::vector<ResourceDataLocal> list;
    const int count = static_cast<int>(resInfoList.size());
    for (int i = 0; i < count; i++) {
        ModuleSpaceObjectResource* r = resInfoList[static_cast<size_t>(i)];
        if (kSPRandom.Next(100) < r->Get<int32_t>("presenceChance")) {
            const double minValue = static_cast<double>(r->Get<int32_t>("lowRange")) * 0.01;
            const double maxValue = static_cast<double>(r->Get<int32_t>("highRange")) * 0.01;
            double num = kSPRandom.NextDouble(minValue, maxValue);
            if (totalResourcePercent + num > 1.0) num = 1.0 - totalResourcePercent;
            totalResourcePercent += num;
            ResourceDataLocal d;
            d.Name = r->Get<std::string>("resourceName");
            d.Mass = static_cast<float>(currentMassVal() * num);
            d.Purity = f32store(fx(static_cast<float>(kSPRandom.Next(r->Get<int32_t>("lowRange"), r->Get<int32_t>("highRange")))) * fx(0.01f));
            list.push_back(d);
            totalResourceMass = f32store(fx(totalResourceMass) + fx(d.Mass));
        }
    }
    const int count2 = static_cast<int>(list.size());
    for (int j = 0; j < count; j++) {
        ModuleSpaceObjectResource* r = resInfoList[static_cast<size_t>(j)];
        for (int k = 0; k < count2; k++) {
            const ResourceDataLocal& d = list[static_cast<size_t>(k)];
            if (d.Name == r->Get<std::string>("resourceName")) {
                const float val = f32store(fx(d.Mass) / fx(totalResourceMass));
                r->Set<float>("abundance", f32store(fx(dotnet::Math::Max(0.01f, val)) * fx(d.Purity)));
                r->Set<float>("displayAbundance", r->Get<float>("abundance"));
            }
        }
    }
    if (static_cast<double>(totalResourceMass) > currentMassVal()) set_currentMassVal(fx(totalResourceMass) * fx(1.01f));  // float * float into a double argument: no float store
    set_massThresholdVal(currentMassVal() - static_cast<double>(totalResourceMass));
}

// ModuleAsteroidInfo.cs:11 / ModuleCometInfo.cs:11
void ModuleSpaceObjectInfoBase::OnStart(StartState state) {
    Part* p = dynamic_cast<Part*>(part);
    if (p == nullptr) throw Nre();
    baseMod = p->FindModuleOfClass(ObjectClass());
    if (baseMod != nullptr) {
        baseMod->OnStart(state);
        if (currentMassVal() <= 1E-09) set_currentMassVal(static_cast<double>(p->fields.mass));
        if (massThresholdVal() <= 1E-09) SetupAsteroidResources();
        p->force_activate();
        ISpaceObjectMass* sm = dynamic_cast<ISpaceObjectMass*>(baseMod);
        if (sm != nullptr) sm->SetSpaceObjectMass(static_cast<float>(currentMassVal()));
        p->fields.mass = static_cast<float>(currentMassVal());
    }
}

// :31
void ModuleSpaceObjectInfoBase::SetupAsteroidResources() {
    Part* p = dynamic_cast<Part*>(part);
    resInfoList.clear();
    for (PartModule* m : p->FindModulesImplementing([&](PartModule& pm) { return pm.IsA("ModuleSpaceObjectResource"); }))
        resInfoList.push_back(static_cast<ModuleSpaceObjectResource*>(m));
    SetupSpaceObjectResources(resInfoList);
}

// :37 (displayMass / resources are text fields)
void ModuleSpaceObjectInfoBase::Update() {
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        Set<std::string>("displayMass", Fmt(currentMassVal(), 5) + "t");
        if (baseMod != nullptr)
            if (ISpaceObjectMass* sm = dynamic_cast<ISpaceObjectMass*>(baseMod)) sm->SetSpaceObjectMass(static_cast<float>(currentMassVal()));
        Set<std::string>("resources", Localizer::Format("#autoLOC_6001049", {Fmt(currentMassVal() - massThresholdVal(), 5),
                                                                              Fmt((currentMassVal() - massThresholdVal()) / currentMassVal() * 100.0, 3)}));
    }
}

// ModuleAnalysisResource.cs:16
void ModuleAnalysisResource::OnStart(StartState) { Fields()["status"]->guiName = Get<std::string>("resourceName"); }

// :22
void ModuleAnalysisResource::Update() {
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) SetupAnalysis();
}

// :30 (`$"{displayAbundance * 100f:0.0000}%"`: float product, formatted)
void ModuleAnalysisResource::SetupAnalysis() {
    if (static_cast<double>(Get<float>("abundance")) > 1E-09) {
        Fields()["status"]->guiActive = true;
        statusFldGuiActive = true;
        Set<std::string>("status", Fmt(static_cast<double>(f32store(fx(Get<float>("displayAbundance")) * 100.0)), 4) + "%");
    } else {
        Fields()["status"]->guiActive = false;
        statusFldGuiActive = false;
    }
}

// ModuleSpaceObjectAnalysis.cs:7
ModuleAnalysisResource* ModuleSpaceObjectAnalysis::FindMatchingAnalyzer(Part& p, const std::string& rName) {
    auto it = analyzers.find(rName);
    if (it != analyzers.end()) return it->second;
    ModuleAnalysisResource* value = nullptr;
    int count = static_cast<int>(p.Modules().Count());
    while (true) {
        if (count-- > 0) {
            PartModule* m = p.Modules()[static_cast<size_t>(count)];
            if (m->IsA("ModuleAnalysisResource")) {
                value = static_cast<ModuleAnalysisResource*>(m);
                if (value->Get<std::string>("resourceName") == rName) break;
            }
            continue;
        }
        analyzers[rName] = nullptr;
        return nullptr;
    }
    analyzers[rName] = value;
    return value;
}

// ModuleAsteroidAnalysis.cs:5 / ModuleCometAnalysis.cs:5 (GameEvents.onVesselWasModified is wired by the host calling CheckForPotato)
void ModuleSpaceObjectAnalysisBase::OnStart(StartState) {
    GE::onVesselWasModified.Add(this, &ModuleSpaceObjectAnalysisBase::CheckForPotato);
    subscribed_ = true;
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        Part* p = dynamic_cast<Part*>(part);
        CheckForPotato(p != nullptr ? p->vessel : nullptr);
    }
}

// :15
void ModuleSpaceObjectAnalysisBase::OnDestroy() {
    analyzers.clear();
    if (subscribed_) GE::onVesselWasModified.Remove(this, &ModuleSpaceObjectAnalysisBase::CheckForPotato);
    subscribed_ = false;
}

ModuleSpaceObjectAnalysisBase::~ModuleSpaceObjectAnalysisBase() {
    if (subscribed_) GE::onVesselWasModified.Remove(this, &ModuleSpaceObjectAnalysisBase::CheckForPotato);
}

// :21 (v.FindPartModuleImplementing<ModuleAsteroid>(): the vessel's parts through the processing record of the module's part)
void ModuleSpaceObjectAnalysisBase::CheckForPotato(Vessel*) {
    const ProcessingVessel* v = ResourceProcessingEnvironment::VesselOf(dynamic_cast<Part*>(part));
    PartModule* obj = nullptr;
    if (v != nullptr)
        for (Part* p : v->parts)
            if ((obj = p->FindModuleOfClass(ObjectClass())) != nullptr) break;
    if (obj != nullptr) FindAsteroidResources(dynamic_cast<Part*>(obj->part));
    else ClearAsteroidResources();
}

// :34
void ModuleSpaceObjectAnalysisBase::ClearAsteroidResources() {
    Part* p = dynamic_cast<Part*>(part);
    if (potatoPart != nullptr) {
        p = potatoPart;
        potatoPart = nullptr;
        ClearAsteroidResources();
    }
    int count = static_cast<int>(p->Modules().Count());
    while (count-- > 0) {
        PartModule* m = p->Modules()[static_cast<size_t>(count)];
        if (m->IsA("ModuleAnalysisResource")) {
            m->Set<float>("displayAbundance", 0.0f);
            m->Set<float>("abundance", 0.0f);
        }
    }
    analyzers.clear();
}

// :56
void ModuleSpaceObjectAnalysisBase::FindAsteroidResources(Part* pp) {
    if (potatoPart != nullptr && potatoPart != pp) ClearAsteroidResources();
    potatoPart = pp;
    analyzers.clear();
    const int count = static_cast<int>(pp->Modules().Count());
    for (int i = 0; i < count; i++) {
        PartModule* m = pp->Modules()[static_cast<size_t>(i)];
        if (m->IsA(ResourceClass())) {
            ModuleAnalysisResource* a = FindMatchingAnalyzer(*dynamic_cast<Part*>(part), m->Get<std::string>("resourceName"));
            if (a != nullptr) {
                a->Set<float>("abundance", m->Get<float>("abundance"));
                a->Set<float>("displayAbundance", m->Get<float>("displayAbundance"));
            }
        }
    }
}

}  // namespace ksp
