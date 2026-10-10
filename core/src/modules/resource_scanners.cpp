// WP-28: ModuleResourceScanner, ModuleOrbitalScanner, ModuleOrbitalSurveyor, ModuleBiomeScanner and the registration of every WP-28
// module. See ksp/modules/resource_processing.hpp.
#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/resource_processing.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/resources/resource_flow_host.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
Part* PartOf(PartModule& m) { return dynamic_cast<Part*>(m.part); }
std::string Fmt(double v, int decimals) { return dotnet::FormatDoubleF(v, decimals); }
void Post(const std::string& message) { ResourceProcessingEnvironment::screenMessages.push_back(message); }
const ProcessingVessel& V(PartModule& m) {
    const ProcessingVessel* v = ResourceProcessingEnvironment::VesselOf(PartOf(m));
    if (v == nullptr) throw Nre();
    return *v;
}
OverlayGeneratorState& Ov() { return ResourceProcessingEnvironment::overlay; }
}  // namespace

// ================================================== ModuleResourceScanner ======================================================

// ModuleResourceScanner.cs:53
std::string ModuleResourceScanner::GetDisplayFormat() { return ResourceDisplayName + "[" + GetScanningMode(true) + "]"; }

// :58 (Convert.ToInt32(bool) = 0 / 1)
std::string ModuleResourceScanner::GetScanningMode(bool abbrevation) {
    std::string result;
    const std::string abbr = abbrevation ? "1" : "0";
    switch (Get<int32_t>("ScannerType")) {
        case 0: result = Localizer::Format("#autoLOC_6001082", {abbr}); break;
        case 1: result = Localizer::Format("#autoLOC_260065"); break;
        case 2: result = Localizer::Format("#autoLOC_6001083", {abbr}); break;
        case 3: result = Localizer::Format("#autoLOC_6001084", {abbr}); break;
    }
    return result;
}

// :82
void ModuleResourceScanner::OnAwake() {}

// :87
void ModuleResourceScanner::OnStart(StartState) {
    Fields()["abundanceDisplay"]->guiName = GetDisplayFormat();
    const PartResourceDefinition* definition = PartResourceLibrary::Instance()->GetDefinition(Get<std::string>("ResourceName"));
    ResourceDisplayName = definition != nullptr ? definition->displayName() : Get<std::string>("ResourceName");
}

// :101 (FlightGlobals.currentMainBody is the vessel's main body in flight)
void ModuleResourceScanner::Update() {
    const ProcessingVessel* v = ResourceProcessingEnvironment::VesselOf(PartOf(*this));
    if (ResourceFlowEnvironment::LoadedSceneIsFlight() && v != nullptr) {
        if (ResourceFlowEnvironment::partActionUIContains && ResourceFlowEnvironment::partActionUIContains(PartOf(*this))) CheckAbundanceDisplay();
        aDispFldGuiActive = IsSituationValid();
        Fields()["abundanceDisplay"]->guiActive = aDispFldGuiActive;
        AbundanceRequest request;
        request.Altitude = v->altitude;
        request.BodyId = v->mainBody.flightGlobalsIndex;
        request.CheckForLock = Get<bool>("RequiresUnlock");
        request.Latitude = v->latitude;
        request.Longitude = v->longitude;
        request.ResourceType = static_cast<HarvestTypes>(Get<int32_t>("ScannerType"));
        request.ResourceName = Get<std::string>("ResourceName");
        if (ResourceMap::Instance() == nullptr) throw Nre();
        abundanceValue = static_cast<double>(ResourceMap::Instance()->GetAbundance(request));
    }
}

// :128
void ModuleResourceScanner::CheckAbundanceDisplay() {
    const ProcessingVessel& v = V(*this);
    const int type = Get<int32_t>("ScannerType");
    if (v.radarAltitude > static_cast<double>(Get<float>("MaxAbundanceAltitude")) && !v.Landed && type == 0)
        Set<std::string>("abundanceDisplay", Localizer::Format("#autoLOC_260116"));
    else if (!v.Splashed && type == 1)
        Set<std::string>("abundanceDisplay", Localizer::Format("#autoLOC_260121"));
    else if (!v.mainBody.atmosphere && type == 2)
        Set<std::string>("abundanceDisplay", Localizer::Format("#autoLOC_260125"));
    else
        DisplayAbundance();
}

// :146
void ModuleResourceScanner::DisplayAbundance() {
    const ProcessingVessel& v = V(*this);
    const double lat = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLat(v.latitude));
    const double lon = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLon(v.longitude));
    std::optional<ResourceProcessingEnvironment::Biome> biome;
    if (ResourceProcessingEnvironment::biomeAt) biome = ResourceProcessingEnvironment::biomeAt(lat, lon, *PartOf(*this));
    const int type = Get<int32_t>("ScannerType");
    std::string defaultSituation = ResourceMap::GetDefaultSituation(static_cast<HarvestTypes>(type));
    if (biome) defaultSituation = biome->name;
    bool num;
    std::string text;
    if (type == 0) {
        num = ResourceMap::Instance()->IsBiomeUnlocked(v.mainBody.flightGlobalsIndex, defaultSituation);
        text = !num ? Localizer::Format("#autoLOC_260145") : std::string("0%");
    } else {
        num = true;
        text = "0%";
    }
    if (abundanceValue > 0.01) text = Fmt(abundanceValue * 100.0, 2) + "%";
    else if (abundanceValue > 1E-09) text = Fmt(abundanceValue * 100.0, 4) + "%";
    if (num) {
        Set<std::string>("abundanceDisplay", text);
        return;
    }
    Set<std::string>("abundanceDisplay", Localizer::Format("#autoLOC_6002267", {text}));
}

// :194
void ModuleResourceScanner::ToggleEvent(const std::string& name, bool status) {
    Events()[name]->active = status;
    Events()[name]->guiActive = status;
}

// :200
bool ModuleResourceScanner::IsSituationValid() {
    const ProcessingVessel& v = V(*this);
    const int count = static_cast<int>(v.parts.size());
    if (count == partCacheCount) return wasValid;
    partCacheCount = count;
    int index = count;
    while (index-- > 0) {
        Part* p = v.parts[static_cast<size_t>(index)];
        int count2 = static_cast<int>(p->Modules().Count());
        while (count2-- > 0) {
            PartModule* m = p->Modules()[static_cast<size_t>(count2)];
            if (!m->IsA("ModuleAsteroid")) {
                if (m->IsA("ModuleComet")) {
                    wasValid = false;
                    return false;
                }
                continue;
            }
            wasValid = false;
            return false;
        }
    }
    wasValid = true;
    return true;
}

// :251
std::string ModuleResourceScanner::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003056"); }

// ================================================== ModuleOrbitalScanner =======================================================

void ModuleOrbitalScanner::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("SwitchResource", BaseEvent::Handler([this] { SwitchResource(); }));
    Events().Bind("ToggleOverlay", BaseEvent::Handler([this] { ToggleOverlay(); }));
    Events().Bind("SetColor", BaseEvent::Handler([this] { SetColor(); }));
    Events().Bind("IncreaseCutoff", BaseEvent::Handler([this] { IncreaseCutoff(); }));
    Events().Bind("DecreaseCutoff", BaseEvent::Handler([this] { DecreaseCutoff(); }));
    Events().Bind("ActivateScanner", BaseEvent::Handler([this] { ActivateScanner(); }));
}

ModuleOrbitalScanner::~ModuleOrbitalScanner() {
    if (subscribed_) GE::OnResourceMapLoaded.Remove(this, &ModuleOrbitalScanner::OnResourceMapLoaded);
}

// ModuleOrbitalScanner.cs:21 (OverlayGenerator.FetchNextResource advances DisplayResource through ResourceList: the generator's own
// cycle; here the next list entry, wrapping)
void ModuleOrbitalScanner::SwitchResource() {
    Ov().fetchNextResourceCalls++;
    if (!Ov().ResourceList.empty()) Ov().DisplayResource = (Ov().DisplayResource + 1) % static_cast<int>(Ov().ResourceList.size());
    ShowResourceName();
    if (Ov().IsActive) GenerateOverlay();
}

// :34
void ModuleOrbitalScanner::ToggleOverlay() {
    if (Ov().IsActive) {
        Ov().IsActive = false;
        HideOverlay();
    } else if (ResourceMap::Instance()->IsPlanetScanned(Ov().DisplayBody)) {
        Ov().IsActive = true;
        GenerateOverlay();
    }
}

// :52
void ModuleOrbitalScanner::GenerateOverlay() {
    UpdateGUI();
    Ov().generateOverlayCalls++;
    Ov().lastCheckForLock = Get<bool>("CheckForLock");
}

// :57
void ModuleOrbitalScanner::HideOverlay() { Ov().clearDisplayCalls++; }

// :64 (`displayMode++; if (displayMode == 4) displayMode = 0`)
void ModuleOrbitalScanner::SetColor() {
    int displayMode = Ov().DisplayMode;
    displayMode++;
    if (displayMode == 4) displayMode = 0;
    Ov().DisplayMode = displayMode;
    setCol->SetGuiName(Localizer::Format("#autoLOC_6001482", {GetOverlayColor(Ov().DisplayMode)}));
    GenerateOverlay();
}

// :79 (MapDisplayTypes: Monochrome, Inverse, HeatMapGreen, HeatMapBlue)
std::string ModuleOrbitalScanner::GetOverlayColor(int dispType) {
    switch (dispType) {
        case 0: return Localizer::Format("#autoLOC_7003278");
        case 1: return Localizer::Format("#autoLOC_7003277");
        case 2: return Localizer::Format("#autoLOC_7003276");
        case 3: return Localizer::Format("#autoLOC_6002358");
        default: return std::to_string(dispType);
    }
}

// :99
void ModuleOrbitalScanner::IncreaseCutoff() {
    Ov().Cutoff += 10;
    if (Ov().Cutoff > 100) Ov().Cutoff = 100;
    if (Ov().IsActive) GenerateOverlay();
}

// :113
void ModuleOrbitalScanner::DecreaseCutoff() {
    Ov().Cutoff -= 10;
    if (Ov().Cutoff < 0) Ov().Cutoff = 0;
    if (Ov().IsActive) GenerateOverlay();
}

// :127
void ModuleOrbitalScanner::ActivateScanner() { SetupMap(); }

// :132
void ModuleOrbitalScanner::ToggleSurvey(bool state) {
    if (_ShowSurveyMenu != state) {
        switchR->active = state;
        toggleO->active = state;
        _ShowSurveyMenu = state;
    }
}

// :144
void ModuleOrbitalScanner::ToggleMenu(bool state) {
    if (_ShowOverlayMenu != state) {
        setCol->active = state;
        Events()["IncreaseCutoff"]->active = state;
        Events()["DecreaseCutoff"]->active = state;
        UpdateGUI();
        _ShowOverlayMenu = state;
    }
}

// :158
void ModuleOrbitalScanner::OnActive() {
    UpdateGUI();
    if (Ov().IsActive) GenerateOverlay();
}

// :166 (cacheAutoLOC_6001054 = "#autoLOC_6001054"; the part-window test is the UI hook)
void ModuleOrbitalScanner::UpdateGUI() {
    if (ResourceFlowEnvironment::partActionUIContains && ResourceFlowEnvironment::partActionUIContains(PartOf(*this))) {
        const std::string text = Localizer::Format(Localizer::Format("#autoLOC_6001054"), {GetOverlayColor(Ov().DisplayMode), std::to_string(Ov().Cutoff)});
        if (setCol->guiName() != text) setCol->SetGuiName(text);
    }
}

// :180
void ModuleOrbitalScanner::FixedUpdate() {
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        const ProcessingVessel& v = V(*this);
        const bool state = ResourceMap::Instance()->IsPlanetScanned(v.mainBody.flightGlobalsIndex);
        const bool isActive = Ov().IsActive;
        if ((Ov().DisplayBody < 0 || v.mainBody.flightGlobalsIndex != Ov().DisplayBody) && !Ov().IsMapView) {
            Ov().clearDisplayCalls++;  // ClearDisplay()
            Ov().DisplayBody = v.mainBody.flightGlobalsIndex;
        }
        ToggleSurvey(state);
        ToggleMenu(isActive);
        UpdateGUI();
    }
}

// :201
void ModuleOrbitalScanner::SetupMap() {
    if (Ov().IsActive) Ov().clearDisplayCalls++;
    Ov().DisplayBody = V(*this).mainBody.flightGlobalsIndex;
    Ov().generateOverlayCalls++;
    Ov().lastCheckForLock = Get<bool>("CheckForLock");
    Ov().IsActive = true;
    ShowResourceName();
}

// :213
void ModuleOrbitalScanner::Start() {
    toggleO = Events()["ToggleOverlay"];
    switchR = Events()["SwitchResource"];
    setCol = Events()["SetColor"];
    if (ResourceFlowEnvironment::LoadedSceneIsFlight() && !ResourceMap::Initialized()) {
        GE::OnResourceMapLoaded.Add(this, &ModuleOrbitalScanner::OnResourceMapLoaded);
        subscribed_ = true;
    }
}

// :225 (`$"{(double)(num2 + num) / 0.02:0.00}%"`: float sum, cast, divided in double)
void ModuleOrbitalScanner::ShowResourceName() {
    if (Ov().DisplayResource < 0 && !Ov().ResourceList.empty()) Ov().DisplayResource = 0;
    Ov().DisplayBody = V(*this).mainBody.flightGlobalsIndex;  // FlightGlobals.currentMainBody
    float num = 0.0f;
    float num2 = 1.0f;
    const auto& cache = ResourceCache::Instance().AbundanceCache();
    const OverlayGeneratorState::ResourceItem& disp = Ov().ResourceList.at(static_cast<size_t>(Ov().DisplayResource));
    for (const auto& s : cache) {
        if (s->BodyId == Ov().DisplayBody && !(s->ResourceName != disp.name) && s->HarvestType == Ov().DisplayResourceType) {
            if (s->Abundance > num) num = s->Abundance;
            if (s->Abundance < num2) num2 = s->Abundance;
        }
    }
    const std::string text = Fmt(static_cast<double>(f32store(fx(num2) + fx(num))) / 0.02, 2) + "%";
    switchR->SetGuiName(disp.displayName + Localizer::Format("#autoLOC_6002267", {text}));
}

// :251
void ModuleOrbitalScanner::EnableModule() { isEnabled = true; }

// :256
void ModuleOrbitalScanner::DisableModule() {
    if (isEnabled) Ov().clearDisplayCalls++;
    isEnabled = false;
}

// :265
bool ModuleOrbitalScanner::ModuleIsActive() { return Ov().IsActive; }

// :270 (vessel.mainBody.BiomeMap == null: a message and false)
bool ModuleOrbitalScanner::IsSituationValid() {
    const ProcessingVessel& v = V(*this);
    if (!v.mainBody.hasBiomeMap) {
        Post(Localizer::Format("#autoLOC_6001055", {v.mainBody.displayName}));
        return false;
    }
    return true;
}

// :281
void ModuleOrbitalScanner::OnDestroy() {
    if (subscribed_) {
        GE::OnResourceMapLoaded.Remove(this, &ModuleOrbitalScanner::OnResourceMapLoaded);
        subscribed_ = false;
    }
}

// :286
void ModuleOrbitalScanner::OnResourceMapLoaded() {
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        ShowResourceName();
        _ShowSurveyMenu = true;
        _ShowOverlayMenu = true;
        ToggleMenu(false);
        ToggleSurvey(false);
    }
}

// ================================================== ModuleOrbitalSurveyor ======================================================

void ModuleOrbitalSurveyor::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("PerformSurvey", BaseEvent::Handler([this] { PerformSurvey(); }));
}

ModuleOrbitalSurveyor::~ModuleOrbitalSurveyor() = default;

// ModuleOrbitalSurveyor.cs:17 (minThreshold / maxThreshold are fields)
void ModuleOrbitalSurveyor::PerformSurvey() {
    const ProcessingVessel& v = V(*this);
    if (v.mainBody.Radius < Get<double>("minThreshold") && Get<bool>("useCustomThresholds")) rescaleThresholds();
    const double num = dotnet::Math::Max(v.mainBody.Radius / 10.0, Get<double>("minThreshold"));
    const double num2 = dotnet::Math::Min(v.mainBody.Radius * 5.0, Get<double>("maxThreshold"));
    if (v.PeA > num && v.ApA < num2 && v.situation == VesselSituations::ORBITING && v.inclination >= 80.0) {
        sendDataToComms();
        return;
    }
    Post(Localizer::Format("#autoLOC_6001056", {Fmt(num / 1000.0, 0), Fmt(num2 / 1000.0, 0)}));
}

// :34 (GameEvents.OnTriggeredDataTransmission.Add(CompleteSurvey): the host forwards the event to CompleteSurvey)
void ModuleOrbitalSurveyor::OnStart(StartState) {
    perfSurv = Events()["PerformSurvey"];
    if (ResourceFlowEnvironment::LoadedSceneIsFlight() && V(*this).mainBody.Radius < Get<double>("minThreshold") && Get<bool>("useCustomThresholds")) rescaleThresholds();
    subscribed_ = true;
}

// :44
void ModuleOrbitalSurveyor::rescaleThresholds() {
    const ProcessingVessel& v = V(*this);
    const double num = v.mainBody.sphereOfInfluence - v.mainBody.Radius;
    double num2 = 1.0;
    double maxThreshold = Get<double>("maxThreshold");
    double minThreshold = Get<double>("minThreshold");
    if (maxThreshold > num) {
        num2 = dotnet::Math::Sqrt(num / maxThreshold);
        maxThreshold = num;
    }
    if (minThreshold > maxThreshold) minThreshold = maxThreshold;
    minThreshold = dotnet::Math::Max(3000.0, minThreshold * num2);
    const double num3 = 0.95 * maxThreshold;
    if (minThreshold > num3) minThreshold = num3;
    Set<double>("maxThreshold", maxThreshold);
    Set<double>("minThreshold", minThreshold);
}

// :64
void ModuleOrbitalSurveyor::OnDestroy() { subscribed_ = false; }

// :69 (`(float)Radius * 0.001f / (float)ScanTime`; the transmitter is the science package's)
void ModuleOrbitalSurveyor::sendDataToComms() {
    const ProcessingVessel& v = V(*this);
    if (transmit) {
        const float dataAmount = f32store(fx(static_cast<float>(v.mainBody.Radius)) * fx(0.001f) / fx(static_cast<float>(Get<int32_t>("ScanTime"))));
        if (transmit(dataAmount, "survey@" + v.mainBody.bodyName)) return;
    }
    Post(Localizer::Format("#autoLOC_259348"));
}

// :86
void ModuleOrbitalSurveyor::CompleteSurvey(uint32_t container, const std::string& subjectID, bool xmitAborted) {
    if (!(container != flightID || subjectID.rfind("survey@", 0) != 0 || xmitAborted)) {
        const ProcessingVessel& v = V(*this);
        std::string message = Localizer::Format("#autoLOC_259361", {v.mainBody.displayName});
        if (addScience) {
            const float num = f32store(fx(static_cast<float>(Get<int32_t>("SciBonus"))) * fx(v.mainBody.recoveryValue));
            addScience(num);
        }
        Post(message);
        ResourceMap::Instance()->UnlockPlanet(v.mainBody.flightGlobalsIndex);
        Events()["PerformSurvey"]->active = false;
        Ov().DisplayBody = v.mainBody.flightGlobalsIndex;
        Ov().generateOverlayCalls++;
        Ov().lastCheckForLock = true;
        surveysCompleted++;  // GameEvents.OnOrbitalSurveyCompleted.Fire(vessel, mainBody)
    }
}

// :119
void ModuleOrbitalSurveyor::CheckForScanData() {
    bool active;
    if (perfSurv->active != (active = !ResourceMap::Instance()->IsPlanetScanned(V(*this).mainBody.flightGlobalsIndex))) perfSurv->active = active;
}

// :130
void ModuleOrbitalSurveyor::Update() {
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) CheckForScanData();
}

// :165
std::string ModuleOrbitalSurveyor::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003047"); }

// ================================================== ModuleBiomeScanner =========================================================

void ModuleBiomeScanner::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("RunAnalysis", BaseEvent::Handler([this] { RunAnalysis(); }));
}

// ModuleBiomeScanner.cs:6
void ModuleBiomeScanner::RunAnalysis() {
    const ProcessingVessel& v = V(*this);
    if (v.controlLevel <= ControlLevel::NONE) {
        Post(Localizer::Format("#autoLOC_258793"));
        return;
    }
    const double lat = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLat(v.latitude));
    const double lon = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLon(v.longitude));
    std::optional<ResourceProcessingEnvironment::Biome> biome;
    if (ResourceProcessingEnvironment::biomeAt) biome = ResourceProcessingEnvironment::biomeAt(lat, lon, *PartOf(*this));
    std::string situationString = v.SituationString;
    std::string text = situationString;
    if (biome) {
        situationString = biome->name;
        text = biome->displayname;
    }
    Post(Localizer::Format("#autoLOC_6001050", {v.mainBody.displayName, text}));
    ResourceMap::Instance()->UnlockBiome(v.mainBody.flightGlobalsIndex, situationString);
}

// :32
void ModuleBiomeScanner::FixedUpdate() {
    const ProcessingVessel* v = ResourceProcessingEnvironment::VesselOf(PartOf(*this));
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight() || v == nullptr) return;
    bool flag;
    if ((flag = v->LandedOrSplashed())) {
        const double lat = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLat(v->latitude));
        const double lon = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLon(v->longitude));
        std::optional<ResourceProcessingEnvironment::Biome> biome;
        if (ResourceProcessingEnvironment::biomeAt) biome = ResourceProcessingEnvironment::biomeAt(lat, lon, *PartOf(*this));
        std::string situationString = v->SituationString;
        if (biome) situationString = biome->name;
        if (ResourceMap::Instance()->IsBiomeUnlocked(v->mainBody.flightGlobalsIndex, situationString)) flag = false;
    }
    if (Events()["RunAnalysis"]->active != flag) Events()["RunAnalysis"]->active = flag;
}

// ================================================== registration ===============================================================

namespace detail {
namespace {
template <class T>
void Reg(std::initializer_list<const char*> names) {
    for (const char* n : names)
        ModuleFactory::Register(n, [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<T>(c)); });
}
}  // namespace

void RegisterResourceProcessingModules() {
    // ModuleScienceConverter (science package) is a BaseConverter subclass: its BaseConverter behaviour (OnAwake / OnLoad / FixedUpdate)
    // is registered under that name, as WP-06's stand-in did
    Reg<BaseConverter>({"BaseConverter", "ModuleScienceConverter"});
    Reg<ModuleResourceConverter>({"ModuleResourceConverter"});
    Reg<BaseDrill>({"BaseDrill"});
    Reg<ModuleResourceHarvester>({"ModuleResourceHarvester"});
    Reg<ModuleAsteroidDrill>({"ModuleAsteroidDrill"});
    Reg<ModuleCometDrill>({"ModuleCometDrill"});
    Reg<ModuleSpaceObjectResource>({"ModuleSpaceObjectResource", "ModuleAsteroidResource", "ModuleCometResource"});
    Reg<ModuleSpaceObjectInfo>({"ModuleSpaceObjectInfo"});
    Reg<ModuleAsteroidInfo>({"ModuleAsteroidInfo"});
    Reg<ModuleCometInfo>({"ModuleCometInfo"});
    Reg<ModuleAnalysisResource>({"ModuleAnalysisResource"});
    Reg<ModuleSpaceObjectAnalysis>({"ModuleSpaceObjectAnalysis"});
    Reg<ModuleAsteroidAnalysis>({"ModuleAsteroidAnalysis"});
    Reg<ModuleCometAnalysis>({"ModuleCometAnalysis"});
    Reg<ModuleGenerator>({"ModuleGenerator"});
    Reg<ModuleAlternator>({"ModuleAlternator"});
    Reg<ModuleFuelJettison>({"ModuleFuelJettison"});
    Reg<ModuleResourceDrain>({"ModuleResourceDrain"});
    Reg<ModuleResourceScanner>({"ModuleResourceScanner"});
    Reg<ModuleOrbitalScanner>({"ModuleOrbitalScanner"});
    Reg<ModuleOrbitalSurveyor>({"ModuleOrbitalSurveyor"});
    Reg<ModuleBiomeScanner>({"ModuleBiomeScanner"});
}
}  // namespace detail

}  // namespace ksp
