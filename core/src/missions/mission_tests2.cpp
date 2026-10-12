// WP-54 slice 2: Assembly-CSharp/Expansions/Missions/Tests/*.cs (the tests WP-55 deferred), part 1: the vessel / location / orbit / science tests.
// See mission_tests2.hpp. Float rule: managed float expressions are f32store(fx(a) op fx(b)). Cite placeholders `File.cs:@Name[#k]` are resolved by
// tools/wp55/check_cites.py --resolve.
#include "ksp/missions/mission_tests2.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

#include "../save/save_util.hpp"
#include "ksp/career/format.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/utilities.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/modules/misc_modules.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/science/science.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Missions {

using KSP::Localization::Localizer;

void MissionTestsEnvironment::Reset() {
    vesselCrashObjectName = nullptr;
    experimentBiome = nullptr;
    shipObtVelocity = nullptr;
    currentMainBody = nullptr;
    eventReportSender = nullptr;
    persistentLoadedPart = nullptr;
    persistentUnloadedPart = nullptr;
    findUnloadedPartVessel = nullptr;
    partIsAsteroidOrComet = nullptr;
    grappleNodeState = nullptr;
    setupOrbitRenderer = nullptr;
    kerbalEvaCrew = nullptr;
    flagSiteVessel = nullptr;
    vesselAtOrbit = nullptr;
    vesselAtOrbitAccuracy = nullptr;
}

namespace Tests {
namespace {

using Env = MissionTestsEnvironment;

std::string Tag(const char* t) { return Localizer::Format(t); }
std::string D0(double v) { return career::FormatNumber(v, std::string_view("0")); }
std::string D2h(double v) { return career::FormatNumber(v, std::string_view("0.##")); }
std::string N0(double v) { return career::FormatNumber(v, std::string_view("N0")); }
std::string RemoveGender(std::string input) {
    const size_t num = input.rfind('^');
    if (num != std::string::npos) input = input.substr(0, num);
    return input;
}
bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
    return true;
}
// FlightGlobals.PersistentVesselIds.ContainsKey(id) ? [id] : null
Vessel* Persistent(uint32_t id) { return AwardEnvironment::VesselByPersistentId(id); }
// FlightGlobals.PersistentVesselIds[id] (KeyNotFoundException for a missing key)
Vessel* PersistentStrict(uint32_t id) {
    Vessel* v = Persistent(id);
    if (v == nullptr) throw KeyNotFoundException("The given key was not present in the dictionary.");
    return v;
}
CelestialBody* Home() { return NodeLibraryEnvironment::HomeBody(); }
// C# `%` on doubles is fmod (exact; UtilMath.cs:99 uses the same)
double Fmod(double x, double y) { return std::fmod(x, y); }

// ScienceUtil.GetExperimentBiome(body, lat, lon)
std::string ExperimentBiome(CelestialBody* body, double lat, double lon) {
    if (Env::experimentBiome) return Env::experimentBiome(body, lat, lon);
    return ScienceUtil::GetExperimentBiome(*body, lat, lon);
}

// the five `locationSituation.* = ...` lines every Awake of these tests repeats
void InitSurfaceChoices(ParamChoices_CelestialBodySurface& p, CelestialBody* bodyDataBody) {
    p.bodyData = std::make_unique<MissionCelestialBody>(bodyDataBody);
    p.biomeData = std::make_unique<Editor::MissionBiome>(Home(), "");
    p.areaData = std::make_unique<SurfaceArea>(Home(), 0.0, 0.0, 150000.0f);
    p.launchSiteName = "";
}

// the waypoint of a ParamChoices_CelestialBodySurface with `waypoint.name = title`; null when the choice has none
FinePrint::Waypoint* NamedWaypoint(const ParamChoices_CelestialBodySurface& p, std::unique_ptr<FinePrint::Waypoint>& store, const std::string& title) {
    store = p.GetWaypoint();
    if (store == nullptr) return nullptr;
    store->name = title;
    return store.get();
}

// Enum.Parse(typeof(E), text, ignoreCase: true)
template <class E>
E ParseEnumIC(const std::string& text) {
    for (const auto& m : dotnet::EnumTraits<E>::members)
        if (text == m.name) return m.value;
    for (const auto& m : dotnet::EnumTraits<E>::members)
        if (EqualsIgnoreCase(text, m.name)) return m.value;
    return save_detail::ParseEnum<E>(text);  // throws the ArgumentException of Enum.Parse
}
}  // namespace

// ---- TestVesselSituation ---------------------------------------------------------------------------------------------------------

// TestVesselSituation.cs:@Awake
TestVesselSituation::TestVesselSituation(std::string typeName) : TestVessel(std::move(typeName)) {
    title = Tag("#autoLOC_8000118");
    situation = Vessel::Situations::PRELAUNCH;
    useActiveVessel = true;
}

// the Awake lines of the three subclasses that fill `locationSituation`
void TestVesselSituation::InitLocationSituation(ParamChoices_CelestialBodySurface& p) { InitSurfaceChoices(p, nullptr); }

// TestVesselSituation.cs:@Initialized
void TestVesselSituation::Initialized() {
    TestModule::Initialized();
    locationSuccess = false;
    situationSuccess = false;
    speedSuccess = false;
}

// TestVesselSituation.cs:@Test
bool TestVesselSituation::Test() {
    TestVessel::Test();
    situationSuccess = vessel != nullptr && vessel->situation == situation;
    if (!situationSuccess) {
        successStartTime = -1.0;
        return false;
    }
    ParamChoices_CelestialBodySurface* ls = LocationSituation();
    switch (ls->locationChoice) {
        default: locationSuccess = false; break;
        case ParamChoices_CelestialBodySurface::Choices::bodyData: locationSuccess = ls->bodyData->IsValid(vessel->mainBody()); break;
        case ParamChoices_CelestialBodySurface::Choices::biomeData:
            currentBiome = ExperimentBiome(vessel->mainBody(), vessel->latitude, vessel->longitude);
            locationSuccess = ls->biomeData->biomeName.empty() || EqualsIgnoreCase(currentBiome, ls->biomeData->biomeName);
            break;
        case ParamChoices_CelestialBodySurface::Choices::areaData:
            if (ls->areaData->body == vessel->mainBody()) locationSuccess = ls->areaData->IsPointInCircle(vessel->latitude, vessel->longitude);
            break;
        case ParamChoices_CelestialBodySurface::Choices::launchSiteName: {
            const std::optional<std::string> crash = Env::vesselCrashObjectName ? Env::vesselCrashObjectName(vessel) : std::nullopt;
            if (crash.has_value()) {
                locationSuccess = ls->launchSiteName == *crash;
            } else {
                locationSuccess = ls->launchSiteName == Vessel::GetLandedAtString(vessel->landedAt);  // MiniBiome.ConvertTagtoLandedAt
            }
            break;
        }
    }
    if (!locationSuccess) {
        successStartTime = -1.0;
        return false;
    }
    speedSuccess = IgnoreSurfaceVelocity() || vessel->srfSpeed <= SurfaceVelocity() + 1.401298464324817E-45;
    if (!speedSuccess) {
        successStartTime = -1.0;
        return false;
    }
    if (successStartTime < 0.0) successStartTime = MissionsEnvironment::UniversalTime();
    return successStartTime + stabilizationTime < MissionsEnvironment::UniversalTime();
}

// TestVesselSituation.cs:@HasNodeWaypoint
bool TestVesselSituation::HasNodeWaypoint() { return LocationSituation()->HasWaypoint(); }
// TestVesselSituation.cs:@GetNodeWaypoint
FinePrint::Waypoint* TestVesselSituation::GetNodeWaypoint() { return NamedWaypoint(*LocationSituation(), nodeWaypoint_, title); }
// TestVesselSituation.cs:@HasNodeLabel
bool TestVesselSituation::HasNodeLabel() { return LocationSituation()->HasNodeLabel(); }
// TestVesselSituation.cs:@HasWorldPosition
bool TestVesselSituation::HasWorldPosition() { return LocationSituation()->HasWorldPosition(); }
// TestVesselSituation.cs:@GetWorldPosition
Vector3f TestVesselSituation::GetWorldPosition() { return LocationSituation()->GetWorldPosition(); }
// TestVesselSituation.cs:@GetExtraText
std::string TestVesselSituation::GetExtraText() { return LocationSituation()->GetExtraText(); }

// TestVesselSituation.cs:@GetNodeBodyParameterString
std::string TestVesselSituation::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "surfaceVelocity") {
        if (IgnoreSurfaceVelocity()) return Localizer::Format("#autoLOC_8004190", {field.guiName, Tag("#autoLOC_8001004")});
        return field.guiName + ": " + career::FormatNumber(SurfaceVelocity(), std::string_view("0.0")) + Tag("#autoLOC_180095");
    }
    if (field.name == "stabilizationTime") return field.guiName + ": " + D0(stabilizationTime) + Tag("#autoLOC_6002317");
    return TestVessel::GetNodeBodyParameterString(field);
}

// TestVesselSituation.cs:@HasNodeBody
bool TestVesselSituation::HasNodeBody() {
    if (LocationSituation() != nullptr) return LocationSituation()->bodyData->Body() != nullptr;
    return false;
}
// TestVesselSituation.cs:@GetNodeBody
CelestialBody* TestVesselSituation::GetNodeBody() {
    if (LocationSituation() != nullptr && LocationSituation()->bodyData->Body() != nullptr) return LocationSituation()->bodyData->Body();
    return nullptr;
}
// TestVesselSituation.cs:@GetInfo
std::string TestVesselSituation::GetInfo() { return Tag("#autoLOC_8004031"); }
// TestVesselSituation.cs:@Save
void TestVesselSituation::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("stabilizationTime", stabilizationTime);
}
// TestVesselSituation.cs:@Load
void TestVesselSituation::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    node.TryGetValue("stabilizationTime", stabilizationTime);
}
// TestVesselSituation.cs:@GetScoreModifier
ScoreModifier TestVesselSituation::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") {
        r.vessel = ScoreModifierVessel("TestVesselSituation");
    } else if (scoreModule == "ScoreModule_Accuracy") {
        if (LocationSituation()->locationChoice == ParamChoices_CelestialBodySurface::Choices::areaData) {
            r.number = LocationSituation()->areaData->PointInCircleAccuracy(vessel->latitude, vessel->longitude);
        } else {
            r.number = 1.0;
        }
    }
    return r;
}

// ---- TestVesselSituationFlying / Landed / Splashed -------------------------------------------------------------------------------

// TestVesselSituationFlying.cs:@Awake
TestVesselSituationFlying::TestVesselSituationFlying() : TestVesselSituation(kTypeName) {
    situation = Vessel::Situations::FLYING;
    title = Tag("#autoLOC_8000180");
    locationSituation = std::make_unique<ParamChoices_CelestialBodySurface>();
    InitLocationSituation(*locationSituation);
}
// TestVesselSituationFlying.cs:@Save
void TestVesselSituationFlying::Save(ConfigNode& node) const {
    TestVesselSituation::Save(node);
    locationSituation->Save(node);
    node.AddValue("surfaceVelocity", surfaceVelocity);
    node.AddValue("ignoreSurfaceVelocity", ignoreSurfaceVelocity);
}
// TestVesselSituationFlying.cs:@Load
void TestVesselSituationFlying::Load(const ConfigNode& node) {
    TestVesselSituation::Load(node);
    locationSituation->Load(node);
    node.TryGetValue("surfaceVelocity", surfaceVelocity);
    node.TryGetValue("ignoreSurfaceVelocity", ignoreSurfaceVelocity);
}
// TestVesselSituationFlying.cs:@GetInfo
std::string TestVesselSituationFlying::GetInfo() { return Tag("#autoLOC_8004015"); }

// TestVesselSituationLanded.cs:@Awake
TestVesselSituationLanded::TestVesselSituationLanded() : TestVesselSituation(kTypeName) {
    situation = Vessel::Situations::LANDED;
    title = Tag("#autoLOC_8000183");
    locationSituation = std::make_unique<ParamChoices_CelestialBodySurface>();
    InitLocationSituation(*locationSituation);
}
// TestVesselSituationLanded.cs:@Save
void TestVesselSituationLanded::Save(ConfigNode& node) const {
    TestVesselSituation::Save(node);
    locationSituation->Save(node);
    node.AddValue("surfaceVelocity", surfaceVelocity);
    node.AddValue("ignoreSurfaceVelocity", ignoreSurfaceVelocity);
}
// TestVesselSituationLanded.cs:@Load
void TestVesselSituationLanded::Load(const ConfigNode& node) {
    TestVesselSituation::Load(node);
    locationSituation->Load(node);
    node.TryGetValue("surfaceVelocity", surfaceVelocity);
    node.TryGetValue("ignoreSurfaceVelocity", ignoreSurfaceVelocity);
}
// TestVesselSituationLanded.cs:@GetInfo
std::string TestVesselSituationLanded::GetInfo() { return Tag("#autoLOC_8004020"); }

// TestVesselSituationSplashed.cs:@Awake
TestVesselSituationSplashed::TestVesselSituationSplashed() : TestVesselSituation(kTypeName) {
    situation = Vessel::Situations::SPLASHED;
    title = Tag("#autoLOC_8003052");
    locationSituation = std::make_unique<ParamChoices_CelestialBodySurface>();
    InitLocationSituation(*locationSituation);
}
// TestVesselSituationSplashed.cs:@Save
void TestVesselSituationSplashed::Save(ConfigNode& node) const {
    TestVesselSituation::Save(node);
    locationSituation->Save(node);
    node.AddValue("surfaceVelocity", surfaceVelocity);
    node.AddValue("ignoreSurfaceVelocity", ignoreSurfaceVelocity);
}
// TestVesselSituationSplashed.cs:@Load
void TestVesselSituationSplashed::Load(const ConfigNode& node) {
    TestVesselSituation::Load(node);
    locationSituation->Load(node);
    node.TryGetValue("surfaceVelocity", surfaceVelocity);
    node.TryGetValue("ignoreSurfaceVelocity", ignoreSurfaceVelocity);
}

// ---- TestVesselStateCrashed ------------------------------------------------------------------------------------------------------

// TestVesselStateCrashed.cs:@Awake
TestVesselStateCrashed::TestVesselStateCrashed() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000242");
    eventFound = false;
    useActiveVessel = true;
    locationSituation = std::make_unique<ParamChoices_CelestialBodySurface>();
    locationSituation->locationChoice = ParamChoices_CelestialBodySurface::Choices::anyData;
    InitSurfaceChoices(*locationSituation, nullptr);
}
TestVesselStateCrashed::~TestVesselStateCrashed() {
    if (subscribed) GameEvents::onVesselWillDestroy.Remove(this, &TestVesselStateCrashed::OnVesselToBeDestroyed);
}
// TestVesselStateCrashed.cs:@Initialized
void TestVesselStateCrashed::Initialized() {
    eventFound = false;
    GameEvents::onVesselWillDestroy.Add(this, &TestVesselStateCrashed::OnVesselToBeDestroyed);
    subscribed = true;
}
// TestVesselStateCrashed.cs:@Cleared
void TestVesselStateCrashed::Cleared() {
    GameEvents::onVesselWillDestroy.Remove(this, &TestVesselStateCrashed::OnVesselToBeDestroyed);
    subscribed = false;
}
// TestVesselStateCrashed.cs:@OnVesselToBeDestroyed
void TestVesselStateCrashed::OnVesselToBeDestroyed(Vessel* vesselDestroyed) {
    if (!eventFound) {
        TestVessel::Test();
        if (vessel == vesselDestroyed && TestVesselLocation()) eventFound = true;
    }
}
// TestVesselStateCrashed.cs:@Test
bool TestVesselStateCrashed::Test() { return eventFound; }
// TestVesselStateCrashed.cs:@TestVesselLocation
bool TestVesselStateCrashed::TestVesselLocation() {
    bool result = false;
    switch (locationSituation->locationChoice) {
        case ParamChoices_CelestialBodySurface::Choices::anyData: result = true; break;
        case ParamChoices_CelestialBodySurface::Choices::bodyData: result = locationSituation->bodyData->IsValid(vessel->mainBody()); break;
        case ParamChoices_CelestialBodySurface::Choices::biomeData: {
            const std::string experimentBiome = ExperimentBiome(vessel->mainBody(), vessel->latitude, vessel->longitude);
            result = locationSituation->biomeData->biomeName.empty() || EqualsIgnoreCase(experimentBiome, locationSituation->biomeData->biomeName);
            break;
        }
        case ParamChoices_CelestialBodySurface::Choices::areaData:
            if (locationSituation->areaData->body == vessel->mainBody())
                result = locationSituation->areaData->IsPointInCircle(vessel->latitude, vessel->longitude);
            break;
        case ParamChoices_CelestialBodySurface::Choices::launchSiteName: {
            const std::optional<std::string> crash = Env::vesselCrashObjectName ? Env::vesselCrashObjectName(vessel) : std::nullopt;
            result = crash.has_value() ? (locationSituation->launchSiteName == *crash)
                                       : (locationSituation->launchSiteName == Vessel::GetLandedAtString(vessel->landedAt));
            break;
        }
    }
    return result;
}
// TestVesselStateCrashed.cs:@GetInfo
std::string TestVesselStateCrashed::GetInfo() { return Tag("#autoLOC_8004006"); }
// TestVesselStateCrashed.cs:@Save
void TestVesselStateCrashed::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    locationSituation->Save(node);
}
// TestVesselStateCrashed.cs:@Load
void TestVesselStateCrashed::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    locationSituation->Load(node);
}
// TestVesselStateCrashed.cs:@GetScoreModifier
ScoreModifier TestVesselStateCrashed::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestVesselStateCrashed");
    return r;
}

// ---- TestVesselVelocity ----------------------------------------------------------------------------------------------------------

// TestVesselVelocity.cs:@Awake
TestVesselVelocity::TestVesselVelocity() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8004164");
    velocityKerbal = std::make_unique<MissionKerbal>(nullptr, node(), [this] { UpdateNodeBodyUI(); }, ProtoCrewMember::KerbalType::Crew, false);
    useActiveVessel = true;
}
// TestVesselVelocity.cs:@Initialize
void TestVesselVelocity::Initialize(TestGroup* testGroup, const std::string& newTitle) {
    TestVessel::Initialize(testGroup, newTitle);
    velocityKerbal->Initialize(nullptr, testGroup->node());
}
// TestVesselVelocity.cs:@CheckCrew
bool TestVesselVelocity::CheckCrew(Vessel* v, MissionKerbal& missionKerbal) {
    if (v == nullptr) return false;
    const auto& vesselCrew = v->GetVesselCrew();
    int count = static_cast<int>(vesselCrew.size());
    do {
        if (count-- <= 0) return false;
    } while (!missionKerbal.IsValid(*vesselCrew[static_cast<size_t>(count)]));
    return true;
}
// TestVesselVelocity.cs:@Test
bool TestVesselVelocity::Test() {
    TestVessel::Test();
    if (vessel == nullptr) return true;
    double num = 0.0;
    auto speedWrt = [&](Vessel* other) {
        const Vector3d shipVel = Env::shipObtVelocity ? Env::shipObtVelocity() : Vector3d();
        const Vector3f o = other->GetObtVelocity();  // Vector3 (float), widened by the Vector3d subtraction
        return static_cast<double>((shipVel - Vector3d(o.x, o.y, o.z)).ToVector3f().magnitude());  // ((Vector3)(ship_obtVelocity - GetObtVelocity())).magnitude
    };
    switch (referenceBody) {
        default:
            if (speedType == SpeedType::SurfaceVelocity) {
                num = vessel->srfSpeed;
            } else if (speedType == SpeedType::OrbitalVelocity) {
                num = vessel->obt_speed;
            }
            break;
        case VelocityReferenceFrame::Asteroid:
            vesselRefBody = PersistentStrict(velocityAsteroidID);
            if (vesselRefBody != nullptr) num = speedWrt(vesselRefBody);
            break;
        case VelocityReferenceFrame::Comet:
            vesselRefBody = PersistentStrict(velocityCometID);
            if (vesselRefBody != nullptr) num = speedWrt(vesselRefBody);
            break;
        case VelocityReferenceFrame::Kerbal: {
            const std::vector<Vessel*> vessels = AwardEnvironment::Vessels();
            for (size_t i = 0; i < vessels.size(); ++i) {
                if (CheckCrew(vessels[i], *velocityKerbal)) {
                    vesselRefBody = vessels[i];
                    break;
                }
            }
            if (vesselRefBody != nullptr) num = speedWrt(vesselRefBody);
            break;
        }
        case VelocityReferenceFrame::Vessel: {
            const uint32_t key = node()->mission->CurrentVesselID(node(), velocityVesselID);
            vesselRefBody = PersistentStrict(key);
            if (vesselRefBody != nullptr) num = speedWrt(vesselRefBody);
            break;
        }
    }
    if (comparisonOperator == TestComparisonLessGreaterOnly::GreaterThan && !(num < static_cast<double>(velocity))) return true;
    if (comparisonOperator == TestComparisonLessGreaterOnly::LessThan) return num <= static_cast<double>(velocity);
    return false;
}
// TestVesselVelocity.cs:@GetInfo
std::string TestVesselVelocity::GetInfo() { return Tag("#autoLOC_8004165"); }
// TestVesselVelocity.cs:@GetNodeBodyParameterString
std::string TestVesselVelocity::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "velocity") {
        return Localizer::Format("#autoLOC_8100154", {field.guiName, comparisonOperator == TestComparisonLessGreaterOnly::LessThan ? "<" : ">",
                                                      career::FormatNumber(velocity, std::string_view("0"))});
    }
    if (field.name == "referenceBody") {
        std::string text = TestVessel::GetNodeBodyParameterString(field) + "\n";
        switch (referenceBody) {
            case VelocityReferenceFrame::Asteroid: {
                std::string text3;
                Asteroid* asteroidByPersistentID = node()->mission->GetAsteroidByPersistentID(velocityAsteroidID);
                text3 = asteroidByPersistentID != nullptr ? asteroidByPersistentID->name : Tag("#autoLOC_6003000");
                return text + Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8000046"), text3});
            }
            case VelocityReferenceFrame::Comet: {
                std::string text2;
                Comet* cometByPersistentID = node()->mission->GetCometByPersistentID(velocityCometID);
                text2 = cometByPersistentID != nullptr ? Localizer::Format(cometByPersistentID->name) : Tag("#autoLOC_6003000");
                return text + Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_6005065"), text2});
            }
            case VelocityReferenceFrame::Kerbal: return text + velocityKerbal->GetNodeBodyParameterString();
            case VelocityReferenceFrame::Default:
            case VelocityReferenceFrame::Vessel: return text;
        }
    }
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestVesselVelocity.cs:@Save
void TestVesselVelocity::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("velocity", velocity);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
    node.AddValue("speedType", dotnet::EnumToString(speedType));
    node.AddValue("referenceBody", dotnet::EnumToString(referenceBody));
    node.AddValue("velocityVesselID", velocityVesselID);
    node.AddValue("velocityAsteroidID", velocityAsteroidID);
    node.AddValue("velocityCometID", velocityCometID);
    auto configNode = std::make_shared<ConfigNode>("VELOCITYWRTKERBAL");
    velocityKerbal->Save(*configNode);
    node.AddNode(configNode);
}
// TestVesselVelocity.cs:@Load
void TestVesselVelocity::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    node.TryGetValue("velocity", velocity);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterOnly::GreaterThan);
    save_detail::TryGetEnum(node, "speedType", speedType, SpeedType::SurfaceVelocity);
    save_detail::TryGetEnum(node, "referenceBody", referenceBody, VelocityReferenceFrame::Default);
    node.TryGetValue("velocityVesselID", velocityVesselID);
    node.TryGetValue("velocityAsteroidID", velocityAsteroidID);
    node.TryGetValue("velocityCometID", velocityCometID);
    ConfigNode* configNode = nullptr;
    node.TryGetNode("VELOCITYWRTKERBAL", configNode);
    if (configNode != nullptr) velocityKerbal->Load(*configNode);
}

// ---- TestGoTo --------------------------------------------------------------------------------------------------------------------

// TestGoTo.cs:@Awake
TestGoTo::TestGoTo() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000145");
    areaData = std::make_unique<SurfaceArea>(Home(), 0.0, 0.0, 150000.0f);
    passed = false;
    useActiveVessel = true;
}
// TestGoTo.cs:@HasNodeWaypoint
bool TestGoTo::HasNodeWaypoint() { return areaData->HasWaypoint(); }
// TestGoTo.cs:@GetNodeWaypoint
FinePrint::Waypoint* TestGoTo::GetNodeWaypoint() {
    nodeWaypoint_ = areaData->GetWaypoint();
    nodeWaypoint_->name = title;
    return nodeWaypoint_.get();
}
// TestGoTo.cs:@Test
bool TestGoTo::Test() {
    TestVessel::Test();
    speedSuccess = ignoreSurfaceVelocity || (vessel != nullptr && vessel->srfSpeed <= surfaceVelocity + 1.401298464324817E-45);
    if (!speedSuccess) return false;
    if (vessel != nullptr && (vessel->situation == Vessel::Situations::LANDED || vessel->situation == Vessel::Situations::SPLASHED)) {
        if (areaData->IsPointInCircle(static_cast<double>(static_cast<float>(vessel->latitude)), static_cast<double>(static_cast<float>(vessel->longitude)))) passed = true;
        return passed;
    }
    return false;
}
// TestGoTo.cs:@GetNodeBodyParameterString
std::string TestGoTo::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.fieldType == "SurfaceArea") return areaData->GetNodeBodyParameterString();
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestGoTo.cs:@HasNodeLabel
bool TestGoTo::HasNodeLabel() { return areaData->HasNodeLabel(); }
// TestGoTo.cs:@HasWorldPosition
bool TestGoTo::HasWorldPosition() { return areaData->HasWorldPosition(); }
// TestGoTo.cs:@GetWorldPosition
Vector3f TestGoTo::GetWorldPosition() { return areaData->GetWorldPosition(); }
// TestGoTo.cs:@GetExtraText
std::string TestGoTo::GetExtraText() { return areaData->GetExtraText(); }
// TestGoTo.cs:@GetInfo
std::string TestGoTo::GetInfo() { return Tag("#autoLOC_8004035"); }
// TestGoTo.cs:@Save
void TestGoTo::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    areaData->Save(node);
    node.AddValue("surfaceVelocity", surfaceVelocity);
    node.AddValue("ignoreSurfaceVelocity", ignoreSurfaceVelocity);
}
// TestGoTo.cs:@Load
void TestGoTo::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    areaData->Load(node);
    node.TryGetValue("surfaceVelocity", surfaceVelocity);
    node.TryGetValue("ignoreSurfaceVelocity", ignoreSurfaceVelocity);
}
// TestGoTo.cs:@GetScoreModifier
ScoreModifier TestGoTo::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") {
        r.vessel = ScoreModifierVessel("TestGoTo");
    } else if (scoreModule == "ScoreModule_Accuracy") {
        if (vessel == nullptr) throw NullReferenceException();
        r.number = areaData->PointInCircleAccuracy(vessel->latitude, vessel->longitude);
    }
    return r;
}

// ---- TestFlyThrough --------------------------------------------------------------------------------------------------------------

// TestFlyThrough.cs:@Awake
TestFlyThrough::TestFlyThrough() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000133");
    volumeData = std::make_unique<SurfaceVolume>();
    passed = false;
    useActiveVessel = true;
}
// TestFlyThrough.cs:@OnShowLabelChanged
void TestFlyThrough::OnShowLabelChanged(bool) { volumeData->showNodeLabel = showNodeLabel; }
// TestFlyThrough.cs:@HasNodeLabel
bool TestFlyThrough::HasNodeLabel() { return volumeData->HasNodeLabel(); }
// TestFlyThrough.cs:@HasWorldPosition
bool TestFlyThrough::HasWorldPosition() { return volumeData->HasWorldPosition(); }
// TestFlyThrough.cs:@GetWorldPosition
Vector3f TestFlyThrough::GetWorldPosition() { return volumeData->GetWorldPosition(); }
// TestFlyThrough.cs:@GetExtraText
std::string TestFlyThrough::GetExtraText() { return volumeData->GetExtraText(); }
// TestFlyThrough.cs:@HasNodeWaypoint
bool TestFlyThrough::HasNodeWaypoint() { return volumeData->HasWaypoint(); }
// TestFlyThrough.cs:@GetNodeWaypoint
FinePrint::Waypoint* TestFlyThrough::GetNodeWaypoint() {
    if (showMapNode) {
        nodeWaypoint_ = volumeData->GetWaypoint();
        nodeWaypoint_->name = title;
        return nodeWaypoint_.get();
    }
    return nullptr;
}
// TestFlyThrough.cs:@Test
bool TestFlyThrough::Test() {
    TestVessel::Test();
    if (vessel != nullptr) {
        if (volumeData->IsInsideVolume(static_cast<float>(vessel->latitude), static_cast<float>(vessel->longitude), static_cast<float>(vessel->altitude))) passed = true;
        return passed;
    }
    return false;
}
// TestFlyThrough.cs:@GetNodeBodyParameterString
std::string TestFlyThrough::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.fieldType == "SurfaceVolume") return volumeData->GetNodeBodyParameterString();
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestFlyThrough.cs:@GetInfo
std::string TestFlyThrough::GetInfo() { return Tag("#autoLOC_8004034"); }
// TestFlyThrough.cs:@Save
void TestFlyThrough::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    volumeData->Save(node);
    node.AddValue("showNodeLabel", showNodeLabel);
    node.AddValue("showMapNode", showMapNode);
}
// TestFlyThrough.cs:@Load
void TestFlyThrough::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    volumeData->Load(node);
    node.TryGetValue("showNodeLabel", showNodeLabel);
    node.TryGetValue("showMapNode", showMapNode);
    volumeData->showNodeLabel = showNodeLabel;
}
// TestFlyThrough.cs:@GetScoreModifier
ScoreModifier TestFlyThrough::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestFlyThrough");
    return r;
}

// ---- TestPlantFlag ---------------------------------------------------------------------------------------------------------------

// TestPlantFlag.cs:@Awake
TestPlantFlag::TestPlantFlag() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000192");
    eventFound = false;
    TestModule::Initialized();  // base.Initialized()
    situation = std::make_unique<ParamChoices_CelestialBodySurface>();
    InitSurfaceChoices(*situation, Home());
}
TestPlantFlag::~TestPlantFlag() {
    if (subscribed) GameEvents::afterFlagPlanted.Remove(this, &TestPlantFlag::AfterFlagPlanted);
}
// TestPlantFlag.cs:@Initialized
void TestPlantFlag::Initialized() {
    eventFound = false;
    GameEvents::afterFlagPlanted.Add(this, &TestPlantFlag::AfterFlagPlanted);
    subscribed = true;
}
// TestPlantFlag.cs:@Cleared
void TestPlantFlag::Cleared() {
    TestModule::Cleared();
    GameEvents::afterFlagPlanted.Remove(this, &TestPlantFlag::AfterFlagPlanted);
    subscribed = false;
}
// TestPlantFlag.cs:@Test
bool TestPlantFlag::Test() { return eventFound; }
// TestPlantFlag.cs:@AfterFlagPlanted
void TestPlantFlag::AfterFlagPlanted(FlagSite* flagSite) {
    v = Env::flagSiteVessel ? Env::flagSiteVessel(flagSite) : nullptr;
    if (v == nullptr) throw NullReferenceException();
    CelestialBody* mainBody = v->mainBody();
    switch (situation->locationChoice) {
        default: eventFound = true; break;
        case ParamChoices_CelestialBodySurface::Choices::bodyData:
            if (situation->bodyData->IsValid(mainBody)) eventFound = true;
            break;
        case ParamChoices_CelestialBodySurface::Choices::biomeData:
            if (!(mainBody != situation->biomeData->body)) {
                const std::string experimentBiome = ExperimentBiome(mainBody, v->latitude, v->longitude);
                if (situation->biomeData->biomeName.empty() || EqualsIgnoreCase(experimentBiome, situation->biomeData->biomeName)) eventFound = true;
            }
            break;
        case ParamChoices_CelestialBodySurface::Choices::areaData:
            if (!(mainBody != situation->areaData->body)) eventFound = situation->areaData->IsPointInCircle(v->latitude, v->longitude);
            break;
    }
}
// TestPlantFlag.cs:@HasNodeWaypoint
bool TestPlantFlag::HasNodeWaypoint() { return situation->HasWaypoint(); }
// TestPlantFlag.cs:@GetNodeWaypoint
FinePrint::Waypoint* TestPlantFlag::GetNodeWaypoint() { return NamedWaypoint(*situation, nodeWaypoint_, title); }
// TestPlantFlag.cs:@GetNodeBody
CelestialBody* TestPlantFlag::GetNodeBody() { return situation->bodyData->Body(); }
// TestPlantFlag.cs:@HasNodeLabel
bool TestPlantFlag::HasNodeLabel() { return situation->HasNodeLabel(); }
// TestPlantFlag.cs:@HasWorldPosition
bool TestPlantFlag::HasWorldPosition() { return situation->HasWorldPosition(); }
// TestPlantFlag.cs:@GetWorldPosition
Vector3f TestPlantFlag::GetWorldPosition() { return situation->GetWorldPosition(); }
// TestPlantFlag.cs:@GetExtraText
std::string TestPlantFlag::GetExtraText() { return situation->GetExtraText(); }
// TestPlantFlag.cs:@GetInfo
std::string TestPlantFlag::GetInfo() { return Tag("#autoLOC_8004026"); }
// TestPlantFlag.cs:@Save
void TestPlantFlag::Save(ConfigNode& node) const {
    TestModule::Save(node);
    situation->Save(node);
    node.AddValue("eventFound", eventFound);
}
// TestPlantFlag.cs:@Load
void TestPlantFlag::Load(const ConfigNode& node) {
    TestModule::Load(node);
    situation->Load(node);
    node.TryGetValue("eventFound", eventFound);
}
// TestPlantFlag.cs:@GetScoreModifier
ScoreModifier TestPlantFlag::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") {
        r.vessel = AwardEnvironment::ActiveVessel();
    } else if (scoreModule == "ScoreModule_Accuracy") {
        if (situation->locationChoice == ParamChoices_CelestialBodySurface::Choices::areaData) {
            if (v == nullptr) throw NullReferenceException();
            r.number = situation->areaData->PointInCircleAccuracy(v->latitude, v->longitude);
        } else {
            r.number = 1.0;
        }
    }
    return r;
}

// ---- TestOrbit -------------------------------------------------------------------------------------------------------------------

// TestOrbit.cs:@Awake
TestOrbit::TestOrbit() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000158");
    orbitAccuracy = 90.0;
    missionOrbit = std::make_unique<MissionOrbit>(Home());
    useActiveVessel = true;
}
// TestOrbit.cs:@OrbitDeviation
double TestOrbit::OrbitDeviation() const {
    return Mathf::Clamp(f32store(100.0 - fx(static_cast<float>(orbitAccuracy))), 0.0f, 100.0f);
}
// TestOrbit.cs:@Initialized
void TestOrbit::Initialized() {
    TestModule::Initialized();
    relativeOrbit = std::make_unique<Orbit>(missionOrbit->RelativeOrbit(node()->mission));
    // initializeorbitRenderer = StartCoroutine(initializeDrawOrbit()): DrawOrbit() (the renderer is UI)
    DrawOrbit();
}
// TestOrbit.cs:@OnOrbitAccuracyChanged
void TestOrbit::OnOrbitAccuracyChanged(double newValue) {
    orbitAccuracy = Mathf::Clamp(static_cast<float>(newValue), 0.0f, 100.0f);
    UpdateNodeBodyUI();
}
// TestOrbit.cs:@DrawOrbit
void TestOrbit::DrawOrbit() {
    Game* g = HighLogic::CurrentGame();
    if (g != nullptr && HighLogic::LoadedScene() != GameScenes::SPACECENTER && (g->Mode == Game_Modes::MISSION || g->Mode == Game_Modes::MISSION_BUILDER)) {
        // orbitRenderer = CreateOrbitRenderer(): the relative orbit is recomputed (the renderer needs a vessel)
        Vessel* vessel_ = nullptr;
        if (vesselID == 0) {
            const std::vector<Vessel*> vs = AwardEnvironment::Vessels();
            for (size_t i = 0; i < vs.size(); ++i)
                if (vs[i] != nullptr) {
                    vessel_ = vs[i];
                    break;
                }
        } else {
            vessel_ = Persistent(node()->mission->CurrentVesselID(node(), vesselID));
        }
        if (vessel_ != nullptr) {
            relativeOrbit = std::make_unique<Orbit>(missionOrbit->RelativeOrbit(node()->mission));
            if (Env::setupOrbitRenderer) Env::setupOrbitRenderer(node(), vessel_, relativeOrbit.get());
        }
        relativeOrbit = std::make_unique<Orbit>(missionOrbit->RelativeOrbit(node()->mission));
    }
}
// TestOrbit.cs:@Cleared
void TestOrbit::Cleared() { TestModule::Cleared(); }
// TestOrbit.cs:@Test
bool TestOrbit::Test() {
    DrawOrbit();  // `if (orbitRenderer == null) DrawOrbit();` (the renderer handle is never kept here)
    TestVessel::Test();
    situationSuccess = vessel != nullptr && vessel->mainBody() == missionOrbit->Body();
    if (!situationSuccess) {
        successStartTime = -1.0;
        return false;
    }
    if (!underThrust && vessel->geeForce > 0.10000000149011612) {
        successStartTime = -1.0;
        return false;
    }
    orbitSuccess = false;
    if (relativeOrbit != nullptr) orbitSuccess = Env::vesselAtOrbit ? Env::vesselAtOrbit(relativeOrbit.get(), OrbitDeviation(), vessel) : Contracts::Lite::VesselAtOrbit(relativeOrbit.get(), OrbitDeviation(), vessel);
    if (!orbitSuccess) {
        successStartTime = -1.0;
        return false;
    }
    if (successStartTime < 0.0) successStartTime = MissionsEnvironment::UniversalTime();
    return successStartTime + stabilizationTime < MissionsEnvironment::UniversalTime();
}
// TestOrbit.cs:@GetNodeBodyParameterString
std::string TestOrbit::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.fieldType == "MissionOrbit") {
        std::string s = Localizer::Format("#autoLOC_8100316", {field.guiName, RemoveGender(missionOrbit->Body()->displayName())}) + "\n";
        s += Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100058"), N0(missionOrbit->SemiMajorAxis())}) + "\n";
        s += Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100059"), N0(missionOrbit->Apoapsis())}) + "\n";
        s += Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100060"), N0(missionOrbit->Periapsis())}) + "\n";
        s += Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100061"), D2h(missionOrbit->Eccentricity())}) + "\n";
        s += Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100062"), D2h(missionOrbit->Inclination()) + "\xC2\xB0"}) + "\n";
        s += Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100063"), D2h(missionOrbit->LAN)}) + "\n";
        s += Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100064"), N0(missionOrbit->ArgumentOfPeriapsis())}) + "\n";
        s += Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100065"), N0(missionOrbit->MeanAnomalyAtEpoch())});
        return s;
    }
    if (field.name == "orbitAccuracy") return Localizer::Format("#autoLOC_8004190", {field.guiName, D2h(orbitAccuracy) + "%"});
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestOrbit.cs:@GetNodeOrbit
Orbit* TestOrbit::GetNodeOrbit() { return missionOrbit->orbit(); }
// TestOrbit.cs:@GetInfo
std::string TestOrbit::GetInfo() { return Tag("#autoLOC_8004023"); }
// TestOrbit.cs:@Save
void TestOrbit::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("orbitAccuracy", orbitAccuracy);
    node.AddValue("stabilizationTime", stabilizationTime);
    node.AddValue("underThrust", underThrust);
    missionOrbit->Save(*node.AddNode("ORBIT"));
}
// TestOrbit.cs:@Load
void TestOrbit::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    if (!node.TryGetValue("orbitAccuracy", orbitAccuracy)) {
        float value = 0.1f;
        node.TryGetValue("orbitTolerance", value);
        orbitAccuracy = 100.0 - static_cast<double>(f32store(fx(value) * 100.0));
    }
    node.TryGetValue("stabilizationTime", stabilizationTime);
    node.TryGetValue("underThrust", underThrust);
    ConfigNode* configNode = nullptr;
    if (node.TryGetNode("ORBIT", configNode)) {
        missionOrbit = std::make_unique<MissionOrbit>(*configNode);
        if (missionOrbit->orbit() != nullptr) missionOrbit->orbit()->Init();
    }
}
// TestOrbit.cs:@GetScoreModifier
ScoreModifier TestOrbit::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") {
        if (vesselID != 0) {
            r.vessel = PersistentStrict(vesselID);
        } else {
            r.vessel = AwardEnvironment::ActiveVessel();
        }
    } else if (scoreModule == "ScoreModule_Accuracy" && relativeOrbit != nullptr) {
        r.number = Env::vesselAtOrbitAccuracy ? Env::vesselAtOrbitAccuracy(relativeOrbit.get(), OrbitDeviation(), vessel) : Contracts::Lite::VesselAtOrbitAccuracy(relativeOrbit.get(), OrbitDeviation(), vessel);
    }
    return r;
}

// ---- TestOrbitParams -------------------------------------------------------------------------------------------------------------

// TestOrbitParams.cs:@Awake
TestOrbitParams::TestOrbitParams() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8003152");
    body = std::make_unique<MissionCelestialBody>();
    apMinValue = 75000.0;
    apMaxValue = 80000.0;
    peMinValue = 70000.0;
    peMaxValue = 75000.0;
    inclination = 0.0;
    inclinationAccuracy = 90.0;
    LAN = 0.0;
    LANAccuracy = 90.0;
    useActiveVessel = true;
}
// TestOrbitParams.cs:@InclinationDeviation
double TestOrbitParams::InclinationDeviation() const { return Mathf::Clamp(f32store(100.0 - fx(static_cast<float>(inclinationAccuracy))), 0.0f, 100.0f); }
// TestOrbitParams.cs:@LANDeviation
double TestOrbitParams::LANDeviation() const { return Mathf::Clamp(f32store(100.0 - fx(static_cast<float>(LANAccuracy))), 0.0f, 100.0f); }

// TestOrbitParams.cs:@GetMaxAp
double TestOrbitParams::GetMaxAp(CelestialBody* b) {
    if (std::isinf(b->sphereOfInfluence)) {
        double num = 0.0;
        const size_t count = b->orbitingBodies.size();
        for (size_t i = 0; i < count; ++i) {
            if (BodyOrbit(b->orbitingBodies[i])->radius > num) num = BodyOrbit(b->orbitingBodies[i])->radius;
        }
        if (num == 0.0) {
            log::Warning("Unable to find a maxorbit radius - using body radius times 10");
            return b->Radius * 10.0;
        }
        return (num - b->Radius) * 1.2;
    }
    return b->sphereOfInfluence - b->Radius - 1.0;
}

namespace {
// the orbit a vessel is on: vessel.loaded ? vessel.orbit : vessel.protoVessel.orbitSnapShot.Load()
Orbit VesselOrbit(Vessel* vessel) {
    if (vessel->loaded) return vessel->orbit();
    return vessel->protoVessel->orbitSnapShot->Load(NodeLibraryEnvironment::Bodies());
}
Vessel::Situations VesselSit(Vessel* vessel) { return vessel->loaded ? vessel->situation : vessel->protoVessel->situation; }
}  // namespace

// TestOrbitParams.cs:@Test
bool TestOrbitParams::Test() {
    TestVessel::Test();
    situationSuccess = vessel != nullptr;
    if (vessel != nullptr && body != nullptr && body->Body() != nullptr) {
        situationSuccess = situationSuccess && body->Body()->bodyName == vessel->mainBody()->bodyName;
    }
    if (!situationSuccess) {
        successStartTime = -1.0;
        return false;
    }
    if (!underThrust && vessel->geeForce > 0.10000000149011612) {
        successStartTime = -1.0;
        return false;
    }
    Orbit orbit = VesselOrbit(vessel);
    const Vessel::Situations situations = VesselSit(vessel);
    if (situations != Vessel::Situations::ORBITING && situations != Vessel::Situations::ESCAPING && situations != Vessel::Situations::SUB_ORBITAL) return false;
    orbitSuccess = true;
    {
        if (checkApoapsis) orbitSuccess = orbitSuccess && orbit.ApA() >= apMinValue && orbit.ApA() <= apMaxValue;
        if (checkPeriapsis) orbitSuccess = orbitSuccess && orbit.PeA() >= peMinValue && orbit.PeA() <= peMaxValue;
        if (checkInclination)
            orbitSuccess = orbitSuccess && Contracts::Lite::SystemUtilities::WithinDeviationByValue(std::fabs(orbit.inclination), std::fabs(inclination), InclinationDeviation(), 90.0);
        if (checkLAN && orbitSuccess && Fmod(std::fabs(orbit.inclination), 180.0) > 1.0) {
            bool flag = false;
            bool flag2 = false;
            double num = orbit.LAN;
            double num2 = LAN;
            if (orbit.inclination < 0.0) num = Fmod(num + 180.0, 360.0);
            if (checkInclination && inclination < 0.0) num2 = Fmod(num2 + 180.0, 360.0);
            float num3 = f32store(Fmod(std::fabs(num - num2), 360.0)) ;
            // (float)Math.Abs(num - num2) % 360f: the cast binds to Math.Abs(...), the float remainder is taken in double and stored
            num3 = f32store(Fmod(fx(static_cast<float>(std::fabs(num - num2))), 360.0));
            if (num3 > 180.0f) num3 = f32store(360.0 - fx(num3));
            if (static_cast<double>(f32store(fx(num3) / 180.0)) < LANDeviation() / 100.0) flag = true;
            if (checkInclination) {
                float num4 = static_cast<float>(inclination - InclinationDeviation() / 100.0 * 90.0);
                if (num4 < 0.0f) {
                    num4 = Mathf::Abs(num4);
                    if (std::fabs(orbit.inclination) < static_cast<double>(num4)) {
                        num3 = f32store(180.0 - fx(num3));
                        if (static_cast<double>(f32store(fx(num3) / 180.0)) < LANDeviation() / 100.0) flag2 = true;
                    }
                }
            }
            orbitSuccess = flag || flag2;
        }
    }
    if (!orbitSuccess) {
        successStartTime = -1.0;
        return false;
    }
    if (successStartTime < 0.0) successStartTime = MissionsEnvironment::UniversalTime();
    return successStartTime + stabilizationTime < MissionsEnvironment::UniversalTime();
}
// TestOrbitParams.cs:@GetNodeBodyParameterString
std::string TestOrbitParams::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "checkApoapsis") {
        if (checkApoapsis) return Localizer::Format("#autoLOC_8002086", {N0(apMinValue), N0(apMaxValue)});
        return "";
    }
    if (field.name == "checkPeriapsis") {
        if (checkPeriapsis) return Localizer::Format("#autoLOC_8002087", {N0(peMinValue), N0(peMaxValue)});
        return "";
    }
    if (field.name == "checkInclination") {
        if (checkInclination) return Localizer::Format("#autoLOC_8002088", {D2h(inclinationAccuracy), D2h(inclination)});
        return "";
    }
    if (field.name == "checkLAN") {
        if (checkLAN) return Localizer::Format("#autoLOC_8002089", {D2h(LANAccuracy), D2h(LAN)});
        return "";
    }
    if (field.name == "body") {
        const std::string text = (body == nullptr || body->Body() == nullptr) ? Tag("#autoLOC_8000273") : RemoveGender(body->Body()->displayName());
        return Localizer::Format("#autoLOC_8100316", {field.guiName, text});
    }
    if (!(field.name == "apMinValue") && !(field.name == "apMaxValue") && !(field.name == "peMinValue") && !(field.name == "peMaxValue") &&
        !(field.name == "inclination") && !(field.name == "LAN") && !(field.name == "LANAccuracy") && !(field.name == "InclinationAccuracy")) {
        return TestVessel::GetNodeBodyParameterString(field);
    }
    return "";
}
// TestOrbitParams.cs:@GetInfo
std::string TestOrbitParams::GetInfo() { return Tag("#autoLOC_8003153"); }
// TestOrbitParams.cs:@RunValidation
void TestOrbitParams::RunValidation(Editor::MissionEditorValidator* validator) {
    if (checkApoapsis && apMinValue > apMaxValue) validator->AddNodeWarn(node(), Tag("#autoLOC_8002090"));
    if (checkPeriapsis && peMinValue > peMaxValue) validator->AddNodeWarn(node(), Tag("#autoLOC_8002091"));
    if (!checkApoapsis && !checkPeriapsis && !checkInclination && !checkLAN) validator->AddNodeFail(node(), Tag("#autoLOC_8002092"));
}
// TestOrbitParams.cs:@Save
void TestOrbitParams::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("checkApoapsis", checkApoapsis);
    node.AddValue("checkPeriapsis", checkPeriapsis);
    node.AddValue("checkInclination", checkInclination);
    node.AddValue("checkLAN", checkLAN);
    node.AddValue("apMaxValue", apMaxValue);
    node.AddValue("apMinValue", apMinValue);
    node.AddValue("peMaxValue", peMaxValue);
    node.AddValue("peMinValue", peMinValue);
    node.AddValue("inclination", inclination);
    node.AddValue("inclinationAccuracy", inclinationAccuracy);
    node.AddValue("LAN", LAN);
    node.AddValue("LANAccuracy", LANAccuracy);
    node.AddValue("stabilizationTime", stabilizationTime);
    node.AddValue("underThrust", underThrust);
    body->Save(node);
}
// TestOrbitParams.cs:@Load
void TestOrbitParams::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    node.TryGetValue("checkApoapsis", checkApoapsis);
    node.TryGetValue("checkPeriapsis", checkPeriapsis);
    node.TryGetValue("checkInclination", checkInclination);
    node.TryGetValue("checkLAN", checkLAN);
    node.TryGetValue("apMaxValue", apMaxValue);
    node.TryGetValue("apMinValue", apMinValue);
    node.TryGetValue("peMaxValue", peMaxValue);
    node.TryGetValue("peMinValue", peMinValue);
    node.TryGetValue("inclination", inclination);
    node.TryGetValue("inclinationAccuracy", inclinationAccuracy);
    node.TryGetValue("LAN", LAN);
    node.TryGetValue("LANAccuracy", LANAccuracy);
    node.TryGetValue("stabilizationTime", stabilizationTime);
    node.TryGetValue("underThrust", underThrust);
    body->Load(node);
}
// TestOrbitParams.cs:@GetScoreModifier
ScoreModifier TestOrbitParams::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") {
        r.vessel = ScoreModifierVessel("TestOrbitParams");
    } else if (scoreModule == "ScoreModule_Accuracy") {
        int num2 = 0;
        double num3 = 0.0;
        Orbit orbit = VesselOrbit(vessel);
        {
            if (checkApoapsis) {
                ++num2;
                num3 += static_cast<double>(Mathf::Abs(f32store(fx(f32store(orbit.ApA())) / (fx(f32store((fx(static_cast<float>(apMinValue)) + fx(static_cast<float>(apMaxValue))))) / 2.0f) - 1.0)));
            }
            if (checkPeriapsis) {
                ++num2;
                num3 += static_cast<double>(Mathf::Abs(f32store(fx(f32store(orbit.PeA())) / (fx(f32store((fx(static_cast<float>(peMinValue)) + fx(static_cast<float>(peMaxValue))))) / 2.0f) - 1.0)));
            }
            if (checkInclination) {
                ++num2;
                num3 += static_cast<double>(Mathf::Clamp01(static_cast<float>(Contracts::Lite::SystemUtilities::MeasureDeviationByValue(orbit.inclination, inclination, 90.0))));
            }
            if (checkLAN) {
                ++num2;
                if (Fmod(std::fabs(orbit.inclination), 180.0) > 1.0) {
                    double num4 = orbit.LAN;
                    double num5 = LAN;
                    if (orbit.inclination < 0.0) num4 = Fmod(num4 + 180.0, 360.0);
                    if (checkInclination && inclination < 0.0) num5 = Fmod(num5 + 180.0, 360.0);
                    float num6 = f32store(Fmod(fx(static_cast<float>(std::fabs(num4 - num5))), 360.0));
                    if (num6 > 180.0f) num6 = f32store(360.0 - fx(num6));
                    if (static_cast<double>(f32store(fx(num6) / 180.0)) < LANDeviation() / 100.0) num3 += static_cast<double>(Mathf::Clamp01(f32store(fx(num6) / 180.0)));
                    if (checkInclination) {
                        float num7 = static_cast<float>(inclination - InclinationDeviation() / 100.0 * 90.0);
                        if (num7 < 0.0f) {
                            num7 = Mathf::Abs(num7);
                            if (std::fabs(orbit.inclination) < static_cast<double>(num7)) {
                                num6 = f32store(180.0 - fx(num6));
                                if (static_cast<double>(f32store(fx(num6) / 180.0)) < LANDeviation() / 100.0)
                                    num3 += static_cast<double>(Mathf::Clamp01(f32store(fx(num6) / 180.0)));
                            }
                        }
                    }
                } else {
                    num3 += 1.0;
                }
            }
        }
        if (num2 == 0) {
            r.number = 1.0;  // `return 1;` (a boxed int; ScoreModule_Accuracy casts it)
        } else {
            r.number = num3 / static_cast<double>(num2);
        }
    }
    return r;
}

// ---- TestScienceExperiment -------------------------------------------------------------------------------------------------------

// TestScienceExperiment.cs:@Awake
TestScienceExperiment::TestScienceExperiment() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000132");
    testsSuccessful = false;
    experimentID = "";
    experimentSituation = "";
    DeployorReceived = "Deployed";
    biomeData = std::make_unique<Editor::MissionBiome>(Home(), "");
}
TestScienceExperiment::~TestScienceExperiment() {
    if (subscribed) {
        GameEvents::OnScienceRecieved.Remove(this, &TestScienceExperiment::OnScienceReceived);
        GameEvents::OnExperimentStored.Remove(this, &TestScienceExperiment::OnExperimentDeployed);
    }
}
// TestScienceExperiment.cs:@Initialized
void TestScienceExperiment::Initialized() {
    TestModule::Initialized();
    GameEvents::OnScienceRecieved.Add(this, &TestScienceExperiment::OnScienceReceived);
    GameEvents::OnExperimentStored.Add(this, &TestScienceExperiment::OnExperimentDeployed);
    subscribed = true;
}
// TestScienceExperiment.cs:@Cleared
void TestScienceExperiment::Cleared() {
    TestModule::Cleared();
    GameEvents::OnScienceRecieved.Remove(this, &TestScienceExperiment::OnScienceReceived);
    GameEvents::OnExperimentStored.Remove(this, &TestScienceExperiment::OnExperimentDeployed);
    subscribed = false;
}
// TestScienceExperiment.cs:@OnScienceReceived
void TestScienceExperiment::OnScienceReceived(float, ScienceSubject* scienceSubjectIn, ProtoVessel*, bool) {
    if (!(DeployorReceived != "Received")) ConductScienceTest(scienceSubjectIn->id);
}
// TestScienceExperiment.cs:@OnExperimentDeployed
void TestScienceExperiment::OnExperimentDeployed(ScienceData* scienceDataIn) {
    if (!(DeployorReceived != "Deployed")) ConductScienceTest(scienceDataIn->subjectID);
}
// TestScienceExperiment.cs:@Test
bool TestScienceExperiment::Test() { return testsSuccessful; }
// TestScienceExperiment.cs:@ConductScienceTest
void TestScienceExperiment::ConductScienceTest(const std::string& inputExperimentId) {
    passedTests = true;
    const std::string first = inputExperimentId.substr(0, inputExperimentId.find('@'));  // inputExperimentId.Split('@')[0]
    if (!experimentID.empty() && first != experimentID) passedTests = false;
    if (!experimentSituation.empty() && inputExperimentId.find(experimentSituation) == std::string::npos) passedTests = false;
    if (!biomeData->biomeName.empty()) {
        if (biomeData->body != nullptr && inputExperimentId.find(biomeData->body->bodyName) == std::string::npos) passedTests = false;
        if (inputExperimentId.find(biomeData->biomeName) == std::string::npos) passedTests = false;
    }
    testsSuccessful = passedTests;
}
// TestScienceExperiment.cs:@HasNodeBody
bool TestScienceExperiment::HasNodeBody() { return biomeData != nullptr ? biomeData->body != nullptr : false; }
// TestScienceExperiment.cs:@GetNodeBody
CelestialBody* TestScienceExperiment::GetNodeBody() {
    if (biomeData != nullptr && biomeData->body != nullptr) return biomeData->body;
    return nullptr;
}
// TestScienceExperiment.cs:@GetInfo
std::string TestScienceExperiment::GetInfo() { return Tag("#autoLOC_8004030"); }
// TestScienceExperiment.cs:@Save
void TestScienceExperiment::Save(ConfigNode& node) const {
    TestModule::Save(node);
    if (!experimentID.empty()) node.AddValue("experimentID", experimentID);
    if (!experimentSituation.empty()) node.AddValue("experimentSituation", experimentSituation);
    node.AddValue("testsSuccessful", testsSuccessful);
    node.AddValue("DeployorReceived", DeployorReceived);
    if (biomeData != nullptr) biomeData->Save(*node.AddNode("BIOMEDATA"));
}
// TestScienceExperiment.cs:@Load
void TestScienceExperiment::Load(const ConfigNode& node) {
    TestModule::Load(node);
    experimentID = "";
    experimentSituation = "";
    node.TryGetValue("experimentID", experimentID);
    node.TryGetValue("experimentSituation", experimentSituation);
    node.TryGetValue("testsSuccessful", testsSuccessful);
    node.TryGetValue("DeployorReceived", DeployorReceived);
    ConfigNode* configNode = nullptr;
    if (node.TryGetNode("BIOMEDATA", configNode)) biomeData->Load(*configNode);
}
// TestScienceExperiment.cs:@GetNodeBodyParameterString
std::string TestScienceExperiment::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "biomeData") {
        if (!biomeData->biomeName.empty()) {
            return Localizer::Format("#autoLOC_8000255", {biomeData->body->bodyDisplayName, ScienceUtil::GetBiomedisplayName(biomeData->body, biomeData->biomeName)});
        }
        return Localizer::Format("#autoLOC_8000269", {biomeData->body->bodyDisplayName});
    }
    if (field.name == "experimentID") {
        if (!experimentID.empty()) {
            ScienceExperiment* experiment = ResearchAndDevelopment::GetExperiment(experimentID);
            if (experiment != nullptr) return Localizer::Format("#autoLOC_8000256", {experiment->experimentTitle});
        }
        return Localizer::Format("#autoLOC_8000256", {Tag("#autoLOC_8002050")});
    }
    if (field.name == "experimentSituation") {
        if (!experimentSituation.empty()) {
            const ExperimentSituations experimentSituations = save_detail::ParseEnum<ExperimentSituations>(experimentSituation);
            return Localizer::Format("#autoLOC_8000257", {Localizer::Format(EnumDescription(experimentSituations))});
        }
        return Localizer::Format("#autoLOC_8000257", {Tag("#autoLOC_8000258")});
    }
    if (field.name == "DeployorReceived") {
        return Localizer::Format("#autoLOC_8004190", {field.guiName, DeployorReceived == "Deployed" ? Tag("#autoLOC_8001037") : Tag("#autoLOC_8000250")});
    }
    return TestModule::GetNodeBodyParameterString(field);
}
// TestScienceExperiment.cs:@GetAppObjectiveInfo
std::string TestScienceExperiment::GetAppObjectiveInfo() {
    if (biomeData != nullptr && !experimentID.empty()) {
        ScienceExperiment* experiment = ResearchAndDevelopment::GetExperiment(experimentID);
        if (experiment != nullptr) {
            // (ExperimentSituations)(-1) when experimentSituation is empty
            const ExperimentSituations sit = !experimentSituation.empty() ? save_detail::ParseEnum<ExperimentSituations>(experimentSituation) : static_cast<ExperimentSituations>(-1);
            std::string text = ScienceUtil::GenerateScienceSubjectTitle(*experiment, sit, *biomeData->body, biomeData->biomeName);
            text += "\n";
            if (DeployorReceived == "Deployed") return text + Tag("#autoLOC_8002054");
            return text + Tag("#autoLOC_8002055");
        }
    }
    return TestModule::GetAppObjectiveInfo();
}
// TestScienceExperiment.cs:@RunValidation
void TestScienceExperiment::RunValidation(Editor::MissionEditorValidator* validator) {
    if (experimentID.empty()) return;
    ScienceExperiment* experiment = ResearchAndDevelopment::GetExperiment(experimentID);
    if (experiment != nullptr && !experimentSituation.empty()) {
        const ExperimentSituations experimentSituations = ParseEnumIC<ExperimentSituations>(experimentSituation);
        if (!experiment->IsAvailableWhile(experimentSituations, *biomeData->body)) {
            validator->AddNodeFail(node(), Localizer::Format("#autoLOC_8002052", {Localizer::Format(EnumDescription(experimentSituations)), experiment->experimentTitle}));
        }
        if (!biomeData->biomeName.empty() && !experiment->BiomeIsRelevantWhile(experimentSituations)) {
            validator->AddNodeFail(node(), Localizer::Format("#autoLOC_8002053", {ScienceUtil::GetBiomedisplayName(biomeData->body, biomeData->biomeName),
                                                                                   experiment->experimentTitle, biomeData->body->displayName()}));
        }
    }
}
// TestScienceExperiment.cs:@GetScoreModifier
ScoreModifier TestScienceExperiment::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = AwardEnvironment::ActiveVessel();
    return r;
}

}  // namespace Tests
}  // namespace ksp::Expansions::Missions
