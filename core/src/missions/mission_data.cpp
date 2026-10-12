// WP-54 slice 2: the mission data classes. See mission_data.hpp. Every function carries the C# file:line of its source.
// Float rule: C# `float` expressions follow the Mono rule (fx() / f32store()); the double maths is plain double arithmetic.
#include "ksp/missions/mission_data.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>

#include "../save/save_util.hpp"
#include "ksp/career/format.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/config/kspparseable.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/fineprint/sentinel.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/science/science.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp::Expansions::Missions {

using KSP::Localization::Localizer;

namespace {
constexpr double kPi = 3.14159265358979323846;

std::string FormatD(double v, const char* f) { return career::FormatNumber(v, std::string_view(f)); }
std::string FormatF(float v, const char* f) { return career::FormatNumber(v, std::string_view(f)); }
// double.Equals (NaN equals NaN)
bool DEq(double a, double b) { return a == b || (std::isnan(a) && std::isnan(b)); }
// Vector3 v = (Vector3)Vector3d
Vector3f ToV3(const Vector3d& v) { return v.ToVector3f(); }
std::string BodyDisplayName(const CelestialBody* b) {
    if (b == nullptr) throw NullReferenceException();
    return b->displayName();
}
// LingoonaGrammarExtensions.cs:35 LocalizeRemoveGender: cut at the last '^'
std::string RemoveGender(std::string input) {
    const size_t num = input.rfind('^');
    if (num != std::string::npos) input = input.substr(0, num);
    return input;
}
CelestialBody* Need(CelestialBody* b) {
    if (b == nullptr) throw NullReferenceException();
    return b;
}
}  // namespace

// ---- environment --------------------------------------------------------------------------------------------------------------

void MissionDataEnvironment::Reset() {
    launchSiteDisplayName = nullptr;
    randomSeed = nullptr;
    generateAsteroidName = nullptr;
    generateCometName = nullptr;
    cometDefinitions = nullptr;
    fileExists = nullptr;
    loadConfigNode = nullptr;
    saveConfigNode = nullptr;
    partTitle = nullptr;
    usersMissionsPath = nullptr;
    LaunchSiteSituation::scene = LaunchSiteSituation::SceneHooks{};
}

// FlightGlobals.GetUniquepersistentId()
uint32_t MissionDataEnvironment::UniquePersistentId() {
    if (MissionMappedVessel::getUniquePersistentId) return MissionMappedVessel::getUniquePersistentId();
    return SaveEnvironment::Current().persistentIds.GetUniquepersistentId();
}

// FlightGlobals.Bodies
std::vector<CelestialBody*> NodeLibraryEnvironment::Bodies() {
    if (bodies) return bodies();
    return SaveEnvironment::Current().localBodies;
}

// ---- Editor/MissionBiome.cs ------------------------------------------------------------------------------------------------------

namespace Editor {

// MissionBiome.cs:14
MissionBiome::MissionBiome(CelestialBody* celestialBody, std::string name) : body(celestialBody), biomeName(std::move(name)) {}

// MissionBiome.cs:20
void MissionBiome::Save(ConfigNode& node) const {
    node.AddValue("bodyName", Need(body)->name());
    if (!biomeName.empty()) node.AddValue("biomeName", biomeName);
}

// MissionBiome.cs:29
void MissionBiome::Load(const ConfigNode& node) {
    std::string value;
    std::string value2;
    node.TryGetValue("bodyName", value);
    node.TryGetValue("biomeName", value2);
    if (value != "") body = NodeLibraryEnvironment::BodyByName(value);
    if (value2 != "") biomeName = value2;
}

// MissionBiome.cs:70
bool MissionBiome::Equals(const MissionBiome& other) const { return biomeName == other.biomeName && body == other.body; }

// MissionBiome.cs:88
std::string MissionBiome::GetNodeBodyParameterString() const {
    const std::string text = biomeName.empty() ? Localizer::Format("#autoLOC_8002051") : ScienceUtil::GetBiomedisplayName(body, biomeName);
    return Localizer::Format("#autoLOC_8000268", {BodyDisplayName(body), text});
}

}  // namespace Editor

// ---- MissionOrbit.cs -------------------------------------------------------------------------------------------------------------

// helper of the MissionOrbit constructors (MissionOrbit.cs lines 180 / 188 / 196): `Orbit = Load(); degreesMNA = ...; bodyRotationEditor = ...`
void MissionOrbit::Construct() {
    orbit_ = std::make_unique<Orbit>(OrbitSnapshot::Load(NodeLibraryEnvironment::Bodies()));
    degreesMNA = meanAnomalyAtEpoch * (180.0 / kPi);
    bodyRotationEditor = orbit_->referenceBody->rotationAngle - orbit_->referenceBody->directRotAngle;  // GAPCelestialBodyState_Orbit.cs:261
}
MissionOrbit::MissionOrbit(CelestialBody* bodyRef) : OrbitSnapshot(*Need(bodyRef)) { Construct(); }
MissionOrbit::MissionOrbit(const Orbit& orbit) : OrbitSnapshot(orbit, NodeLibraryEnvironment::Bodies()) { Construct(); }
MissionOrbit::MissionOrbit(const ConfigNode& node) : OrbitSnapshot(node) { Construct(); }
MissionOrbit::MissionOrbit(const MissionOrbit& other)
    : OrbitSnapshot(other), orbit_(std::make_unique<Orbit>(*other.orbit_)), smaCache(other.smaCache), degreesMNA(other.degreesMNA),
      bodyRotationEditor(other.bodyRotationEditor) {}
MissionOrbit& MissionOrbit::operator=(const MissionOrbit& other) {
    if (this == &other) return *this;
    OrbitSnapshot::operator=(other);
    orbit_ = std::make_unique<Orbit>(*other.orbit_);
    smaCache = other.smaCache;
    degreesMNA = other.degreesMNA;
    bodyRotationEditor = other.bodyRotationEditor;
    return *this;
}

// MissionOrbit.cs:21
CelestialBody* MissionOrbit::Body() const { return orbit_->referenceBody; }
void MissionOrbit::set_Body(CelestialBody* value) {
    orbit_->referenceBody = value;
    const std::vector<CelestialBody*> bodies = NodeLibraryEnvironment::Bodies();
    const auto it = std::find(bodies.begin(), bodies.end(), value);
    ReferenceBodyIndex = it == bodies.end() ? -1 : static_cast<int>(it - bodies.begin());
}

// MissionOrbit.cs:35
void MissionOrbit::set_SemiMajorAxis(double value) {
    if ((1.0 + eccentricity) * value - Body()->Radius <= Body()->sphereOfInfluence && value >= Body()->Radius / 2.0) {
        semiMajorAxis = value;
        orbit_->semiMajorAxis = value;
    }
}

// MissionOrbit.cs:52
double MissionOrbit::Apoapsis() const { return (1.0 + eccentricity) * semiMajorAxis - Body()->Radius; }
void MissionOrbit::set_Apoapsis(double value) {
    if (value < 0.0) value = 0.0;
    const double editorMaxOrbitRadius = GetEditorMaxOrbitRadius(Body());
    const double periapsis = Periapsis();
    if (value < periapsis) {
        set_Apoapsis(periapsis);
        return;
    }
    if (value > editorMaxOrbitRadius) {
        set_Eccentricity(0.9999);
        return;
    }
    set_SemiMajorAxis((value + Periapsis()) / 2.0 + Body()->Radius);
    set_Eccentricity((value - semiMajorAxis + Body()->Radius) / semiMajorAxis);
}

// MissionOrbit.cs:82
double MissionOrbit::Periapsis() const { return (1.0 - eccentricity) * semiMajorAxis - Body()->Radius; }
void MissionOrbit::set_Periapsis(double value) {
    const double num = 0.0 - Body()->Radius;
    const double apoapsis = Apoapsis();
    if (value > apoapsis) {
        set_Periapsis(apoapsis);
        return;
    }
    if (value <= num) {
        set_Eccentricity(0.9999);
        return;
    }
    set_SemiMajorAxis((Apoapsis() + value) / 2.0 + Body()->Radius);
    set_Eccentricity((semiMajorAxis - Body()->Radius - value) / semiMajorAxis);
}

// MissionOrbit.cs:108
void MissionOrbit::set_Eccentricity(double value) {
    if ((1.0 + value) * semiMajorAxis - Body()->Radius <= Body()->sphereOfInfluence) {
        eccentricity = value;
        orbit_->eccentricity = value;
    }
}
// MissionOrbit.cs:125
void MissionOrbit::set_Inclination(double value) {
    inclination = value;
    orbit_->inclination = value;
}
// MissionOrbit.cs:139
void MissionOrbit::set_Lan(double value) {
    LAN = value;
    orbit_->LAN = value;
}
// MissionOrbit.cs:153
void MissionOrbit::set_ArgumentOfPeriapsis(double value) {
    argOfPeriapsis = value;
    orbit_->argumentOfPeriapsis = value;
}
// MissionOrbit.cs:166
void MissionOrbit::set_Epoch(double value) {
    epoch = value;
    orbit_->epoch = value;
}
// MissionOrbit.cs:180
void MissionOrbit::set_MeanAnomalyAtEpoch(double value) {
    degreesMNA = value;
    meanAnomalyAtEpoch = value * (kPi / 180.0);
    orbit_->meanAnomalyAtEpoch = value * (kPi / 180.0);
}

// MissionOrbit.cs:227
double MissionOrbit::GetUTOrbitRotation(const Orbit& orbit, double UT) { return UT * 360.0 / orbit.period; }

// MissionOrbit.cs:218
Orbit MissionOrbit::RelativeOrbit(Mission* mission) const {
    Orbit orbit = Orbit::CopyOf(*orbit_);
    const double lAN = LAN;
    orbit.LAN = lAN;
    orbit.meanAnomalyAtEpoch -= GetUTOrbitRotation(*orbit_, mission != nullptr ? mission->situation->startUT : MissionsEnvironment::UniversalTime()) * (kPi / 180.0);
    return orbit;
}

// MissionOrbit.cs:232
void MissionOrbit::Reset() {
    set_Eccentricity(0.0);
    set_SemiMajorAxis(Body()->Radius + 100000.0);
    set_Inclination(0.001);
    set_Lan(0.0);
    set_ArgumentOfPeriapsis(0.0);
    set_MeanAnomalyAtEpoch(0.0);
    set_Epoch(0.0);
}

// MissionOrbit.cs:243
void MissionOrbit::Save(ConfigNode& node) const {
    OrbitSnapshot::Save(node);
    node.AddValue("editorRotation", bodyRotationEditor);
}

// MissionOrbit.cs:249
void MissionOrbit::Load(const ConfigNode& node) {
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        const std::string& n = value.name;
        if (n == "MNA") {
            set_MeanAnomalyAtEpoch(save_detail::ParseDouble(value.value) * (180.0 / kPi));
        } else if (n == "ECC") {
            set_Eccentricity(save_detail::ParseDouble(value.value));
        } else if (n == "editorRotation") {
            bodyRotationEditor = save_detail::ParseDouble(value.value);
        } else if (n == "SMA") {
            smaCache = save_detail::ParseDouble(value.value);
        } else if (n == "LAN") {
            set_Lan(save_detail::ParseDouble(value.value));
        } else if (n == "REF") {
            const std::vector<CelestialBody*> bodies = NodeLibraryEnvironment::Bodies();
            const int index = save_detail::ParseInt(value.value);
            if (index < 0 || static_cast<size_t>(index) >= bodies.size()) throw std::out_of_range("Index was out of range.");
            set_Body(bodies[static_cast<size_t>(index)]);
            set_SemiMajorAxis(smaCache);
        } else if (n == "EPH") {
            set_Epoch(save_detail::ParseDouble(value.value));
        } else if (n == "INC") {
            set_Inclination(save_detail::ParseDouble(value.value));
        } else if (n == "LPE") {
            set_ArgumentOfPeriapsis(save_detail::ParseDouble(value.value));
        }
    }
    orbit_->Init();
}

// MissionOrbit.cs:298
bool MissionOrbit::Equals(const MissionOrbit& o) const {
    return ReferenceBodyIndex == o.ReferenceBodyIndex && UtilMath::Approximately(semiMajorAxis, o.semiMajorAxis) &&
           UtilMath::Approximately(eccentricity, o.eccentricity) && UtilMath::Approximately(inclination, o.inclination) &&
           UtilMath::Approximately(LAN, o.LAN) && UtilMath::Approximately(argOfPeriapsis, o.argOfPeriapsis) &&
           UtilMath::Approximately(epoch, o.epoch) && UtilMath::Approximately(meanAnomalyAtEpoch, o.meanAnomalyAtEpoch);
}

// MissionOrbit.cs:313
std::string MissionOrbit::GetNodeBodyParameterString() const { return Localizer::Format("#autoLOC_8004192", {RemoveGender(BodyDisplayName(Body()))}); }

// MissionOrbit.cs:318
double MissionOrbit::GetEditorMaxOrbitRadius(CelestialBody* body) {
    if (std::isinf(body->sphereOfInfluence)) {
        double num = 0.0;
        const size_t count = body->orbitingBodies.size();
        for (size_t i = 0; i < count; ++i) {
            if (BodyOrbit(body->orbitingBodies[i])->radius > num) num = BodyOrbit(body->orbitingBodies[i])->radius;
        }
        if (num == 0.0) {
            log::Warning("Unable to find a maxorbit radius - using body radius times 10");
            return body->Radius * 10.0;
        }
        return num * 1.2;
    }
    return body->sphereOfInfluence - 1.0;
}

// ---- VesselGroundLocation.cs -----------------------------------------------------------------------------------------------------

// VesselGroundLocation.cs:86
VesselGroundLocation::VesselGroundLocation() : rotation(std::make_unique<MissionQuaternion>()) {}

// VesselGroundLocation.cs:86
VesselGroundLocation::VesselGroundLocation(MENode* node, GizmoIcon icon) : VesselGroundLocation() {
    (void)node;
    targetBody = NodeLibraryEnvironment::HomeBody();
    rotation = std::make_unique<MissionQuaternion>();
    KSPRandom kSPRandom(ResourceEnvironment::TickCount());  // new KSPRandom(): Environment.TickCount
    latitude = startLatMin + kSPRandom.NextDouble() * (startLatMax - startLatMin);
    longitude = startLonMin + kSPRandom.NextDouble() * (startLonMax - startLonMin);
    gizmoIcon = icon;
}

// VesselGroundLocation.cs:65
void VesselGroundLocation::set_GAPGizmoIcon(GizmoIcon value) {
    gizmoIcon = value;
    if (OnGizmoIconChange) OnGizmoIconChange(value);
}

// VesselGroundLocation.cs:115
std::unique_ptr<FinePrint::Waypoint> VesselGroundLocation::GetWaypoint() const {
    auto w = std::make_unique<FinePrint::Waypoint>();
    w->celestialName = Need(targetBody)->name();
    w->latitude = latitude;
    w->longitude = longitude;
    w->nodeCaption1 = Localizer::Format("#autoLOC_8002000");
    return w;
}

// VesselGroundLocation.cs:131
Vector3f VesselGroundLocation::GetWorldPosition() const { return ToV3(Need(targetBody)->GetWorldSurfacePosition(latitude, longitude, 0.0)); }

// VesselGroundLocation.cs:136
std::string VesselGroundLocation::GetExtraText() const { return Localizer::Format("#autoLOC_8002000"); }

// VesselGroundLocation.cs:141
std::string VesselGroundLocation::GetNodeBodyParameterString() const {
    const std::string text = FormatD(longitude, "0.##");
    const std::string text2 = FormatD(latitude, "0.##");
    return Localizer::Format("#autoLOC_8004191", {BodyDisplayName(targetBody), text, text2});
}

// VesselGroundLocation.cs:148
bool VesselGroundLocation::Equals(const VesselGroundLocation& o) const {
    if (targetBody == o.targetBody && DEq(altitude, o.altitude) && DEq(latitude, o.latitude) && DEq(longitude, o.longitude)) {
        return rotation->Equals(*o.rotation);
    }
    return false;
}

// VesselGroundLocation.cs:186
void VesselGroundLocation::Load(const ConfigNode& node) {
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        const std::string& n = value.name;
        if (n == "lon") {
            longitude = save_detail::ParseDouble(value.value);
        } else if (n == "gizmoIcon") {
            gizmoIcon = save_detail::ParseEnum<GizmoIcon>(value.value);
        } else if (n == "alt") {
            altitude = save_detail::ParseDouble(value.value);
        } else if (n == "lat") {
            latitude = save_detail::ParseDouble(value.value);
        } else if (n == "rot") {
            rotation = std::make_unique<MissionQuaternion>(KSPUtil::ParseVector3(value.value));
        } else if (n == "splashed") {
            splashed = save_detail::ParseBool(value.value);
        } else if (n == "bodyName") {
            if (!value.value.empty()) targetBody = NodeLibraryEnvironment::BodyByName(value.value);
        }
    }
    if (targetBody == nullptr) targetBody = NodeLibraryEnvironment::HomeBody();
}

// VesselGroundLocation.cs:225
void VesselGroundLocation::Save(ConfigNode& node) const {
    if (targetBody == nullptr) {
        node.AddValue("bodyName", "");
    } else {
        node.AddValue("bodyName", targetBody->name());
    }
    node.AddValue("gizmoIcon", save_detail::EnumName(gizmoIcon));
    node.AddValue("lat", latitude);
    node.AddValue("lon", longitude);
    node.AddValue("alt", altitude);
    node.AddValue("rot", KSPUtil::WriteVector(rotation->eulerAngles()));
    node.AddValue("splashed", splashed);
}

// ---- VesselLocation.cs -----------------------------------------------------------------------------------------------------------

// VesselLocation.cs:28
VesselLocation::VesselLocation() {
    launchSite = "LaunchPad";
    brakesOn = true;
}

// VesselLocation.cs:44
std::unique_ptr<MissionOrbit> VesselLocation::createZeroOrbit() const { return std::make_unique<MissionOrbit>(NodeLibraryEnvironment::HomeBody()); }

// VesselLocation.cs:34
VesselLocation::VesselLocation(MENode* node) : VesselLocation() {
    orbitSnapShot = createZeroOrbit();
    vesselGroundLocation = std::make_unique<VesselGroundLocation>(node, VesselGroundLocation::GizmoIcon::Rocket);
    launchSite = "LaunchPad";
    brakesOn = true;
    vesselGroundLocation->splashed = true;
}

// VesselLocation.cs:71
void VesselLocation::Load(const ConfigNode& node) {
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        const std::string& n = value.name;
        if (n == "brakesOn") {
            brakesOn = save_detail::ParseBool(value.value);
        } else if (n == "facility") {
            save_detail::TryGetEnum(node, "facility", facility, EditorFacility::None);
        } else if (n == "launchSite") {
            node.TryGetValue("launchSite", launchSite);
        } else if (n == "sit") {
            save_detail::TryGetEnum(node, "sit", situation, VesselStartSituations::PRELAUNCH);
        }
    }
    orbitSnapShot = createZeroOrbit();
    vesselGroundLocation = std::make_unique<VesselGroundLocation>();
    vesselGroundLocation->targetBody = NodeLibraryEnvironment::HomeBody();
    const auto& nodes = node.nodes();
    for (int j = 0; j < nodes.Count(); ++j) {
        const ConfigNode* configNode = nodes[j];
        const std::string& name = configNode->name;
        if (name == "ORBIT") {
            orbitSnapShot = std::make_unique<MissionOrbit>(*configNode);
        } else if (name == "GROUNDPOINT") {
            vesselGroundLocation->Load(*configNode);
        }
    }
}

// VesselLocation.cs:113
void VesselLocation::Save(ConfigNode& node) {
    node.AddValue("sit", save_detail::EnumName(situation));
    node.AddValue("launchSite", launchSite);
    node.AddValue("facility", save_detail::EnumName(facility));
    node.AddValue("brakesOn", brakesOn);
    if (vesselGroundLocation != nullptr) vesselGroundLocation->Save(*node.AddNode("GROUNDPOINT"));
    if (orbitSnapShot == nullptr) orbitSnapShot = createZeroOrbit();
    orbitSnapShot->Save(*node.AddNode("ORBIT"));
}

// ---- Crew.cs ---------------------------------------------------------------------------------------------------------------------

// Crew.cs:16
void Crew::Load(const ConfigNode& node) {
    crewNames.clear();
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        const std::string& n = value.name;
        if (n == "crew") {
            crewNames.push_back(value.value);
        } else if (n == "vesselPersistentId") {
            vesselPersistentID = save_detail::ParseUInt(value.value);
        } else if (n == "partPersistentId") {
            partPersistentID = save_detail::ParseUInt(value.value);
        }
    }
}

// Crew.cs:37
std::string Crew::LocalizeCrewMember(int nameIndex) const {
    const std::string& n = crewNames.at(static_cast<size_t>(nameIndex));
    if (n == "Valentina Kerman") return Localizer::Format("#autoLOC_1100005");
    if (n == "Bob Kerman") return Localizer::Format("#autoLOC_1100004");
    if (n == "Bill Kerman") return Localizer::Format("#autoLOC_1100003");
    if (n == "Jebediah Kerman") return Localizer::Format("#autoLOC_1100002");
    return n;
}

// Crew.cs:55
void Crew::Save(ConfigNode& node) const {
    node.AddValue("partPersistentId", partPersistentID);
    node.AddValue("vesselPersistentId", vesselPersistentID);
    for (size_t i = 0; i < crewNames.size(); ++i) node.AddValue("crew", crewNames[i]);
}

// ---- LaunchSiteSituation.cs ------------------------------------------------------------------------------------------------------

// LaunchSiteSituation.cs:46
LaunchSiteSituation::LaunchSiteSituation(MENode* node)
    : launchSiteName(Localizer::Format("#autoLOC_8100066")),
      launchSiteGroundLocation(std::make_unique<VesselGroundLocation>(node, VesselGroundLocation::GizmoIcon::LaunchSite)) {}

// LaunchSiteSituation.cs:36
std::string LaunchSiteSituation::LaunchSiteObjectName() const {
    std::string name = KSPUtil::SanitizeString(launchSiteName, '_', false);
    std::replace(name.begin(), name.end(), ' ', '_');
    return name;
}

// LaunchSiteSituation.cs:51 (the coroutine: yields are the scene's; the decision tree is kept)
void LaunchSiteSituation::AddLaunchSite(bool createObject) {
    const SceneHooks& s = scene;
    if (!(MissionsEnvironment::MakingHistoryInstalled() && s.mobileLaunchSitePrefabExists && s.mobileLaunchSitePrefabExists())) return;
    if (!launchSiteObjectExists) {
        const std::string objectName = LaunchSiteObjectName();
        if (s.launchSiteExists && s.launchSiteExists(objectName)) {
            const std::optional<CelestialBody*> cityBody = s.launchSiteCityBody ? s.launchSiteCityBody(objectName) : std::nullopt;
            // !(launchSite.pqsCity2 == null) && (!(pqsCity2 != null) || !(pqsCity2.celestialBody != target))
            if (cityBody.has_value() && *cityBody == launchSiteGroundLocation->targetBody) {
                if (s.loadedSceneIsMissionBuilder && s.loadedSceneIsMissionBuilder() && s.postScreenMessage)
                    s.postScreenMessage(Localizer::Format("#autoLOC_8100287", {launchSiteName}));
                log::Info("A LaunchSite named '" + launchSiteName + "' already exists.");
                return;
            }
            if (s.removeLaunchSite) s.removeLaunchSite(objectName);
        }
        if (createObject) {
            if (s.createLaunchSiteObject) s.createLaunchSiteObject(*this);
        } else if (s.createEmptyLaunchSite) {
            s.createEmptyLaunchSite(*this);
        }
    } else if (launchSiteObjectCityBody.has_value() && *launchSiteObjectCityBody != launchSiteGroundLocation->targetBody) {
        if (s.removeLaunchSite) s.removeLaunchSite(LaunchSiteObjectName());
        if (createObject) {
            if (s.createLaunchSiteObject) s.createLaunchSiteObject(*this);
        } else if (s.createEmptyLaunchSite) {
            s.createEmptyLaunchSite(*this);
        }
    }
}

// LaunchSiteSituation.cs:198
void LaunchSiteSituation::RemoveLaunchSite() {
    if (MissionsEnvironment::MakingHistoryInstalled() && scene.launchSiteExists && scene.launchSiteExists(LaunchSiteObjectName())) {
        if (scene.removeLaunchSite) scene.removeLaunchSite(LaunchSiteObjectName());
    }
}

// LaunchSiteSituation.cs:206
void LaunchSiteSituation::Load(const ConfigNode& node) {
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        const std::string& n = value.name;
        if (n == "splashed") {
            splashed = save_detail::ParseBool(value.value);
        } else if (n == "showRamp") {
            showRamp = save_detail::ParseBool(value.value);
        } else if (n == "facility") {
            save_detail::TryGetEnum(node, "facility", facility, EditorFacility::None);
        } else if (n == "launchSiteName") {
            launchSiteName = value.value;
        }
    }
    const auto& nodes = node.nodes();
    for (int j = 0; j < nodes.Count(); ++j) {
        const ConfigNode* configNode = nodes[j];
        if (configNode->name != "GROUNDPOINT") continue;
        launchSiteGroundLocation->Load(*configNode);
        if (!node.HasValue("alignedNorth")) {
            launchSiteGroundLocation->rotation->set_eulerZ(f32store(fx(launchSiteGroundLocation->rotation->eulerZ()) + 90.0));
            if (launchSiteGroundLocation->rotation->eulerZ() > 360.0f)
                launchSiteGroundLocation->rotation->set_eulerZ(f32store(fx(launchSiteGroundLocation->rotation->eulerZ()) - 360.0));
        }
    }
}

// LaunchSiteSituation.cs:247
void LaunchSiteSituation::Save(ConfigNode& node) const {
    ConfigNode* configNode = node.AddNode("LAUNCHSITESITUATION");
    configNode->AddValue("launchSiteName", launchSiteName);
    configNode->AddValue("facility", save_detail::EnumName(facility));
    configNode->AddValue("showRamp", showRamp);
    configNode->AddValue("splashed", splashed);
    if (launchSiteGroundLocation != nullptr) launchSiteGroundLocation->Save(*configNode->AddNode("GROUNDPOINT"));
    configNode->AddValue("alignedNorth", true);
}

// ---- SurfaceArea.cs --------------------------------------------------------------------------------------------------------------

// SurfaceArea.cs:28
SurfaceArea::SurfaceArea() : body(NodeLibraryEnvironment::HomeBody()) {}
// SurfaceArea.cs:33
SurfaceArea::SurfaceArea(CelestialBody* targetBody, double lon, double lat, float r) : body(targetBody), latitude(lat), longitude(lon), radius(r) {}

// SurfaceArea.cs:46
std::unique_ptr<FinePrint::Waypoint> SurfaceArea::GetWaypoint() const {
    auto w = std::make_unique<FinePrint::Waypoint>();
    w->celestialName = Need(body)->name();
    w->latitude = latitude;
    w->longitude = longitude;
    w->altitude = altitude;
    w->radius = radius;
    w->nodeCaption1 = Localizer::Format("#autoLOC_8000274", {dotnet::FormatSingleG(radius)});
    return w;
}

// SurfaceArea.cs:68
Vector3f SurfaceArea::GetWorldPosition() const { return ToV3(Need(body)->GetWorldSurfacePosition(latitude, longitude, altitude)); }

// SurfaceArea.cs:73
std::string SurfaceArea::GetExtraText() const { return Localizer::Format("#autoLOC_8000274", {dotnet::FormatSingleG(radius)}); }

// SurfaceArea.cs:78
bool SurfaceArea::Equals(const SurfaceArea& o) const {
    if (body->name() == o.body->name() && UtilMath::Approximately(latitude, o.latitude) && UtilMath::Approximately(longitude, o.longitude))
        return Mathf::Approximately(radius, o.radius);
    return false;
}

// SurfaceArea.cs:96
void SurfaceArea::Load(const ConfigNode& node) {
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        const std::string& n = value.name;
        if (n == "radius") {
            radius = save_detail::ParseFloat(value.value);
        } else if (n == "alt") {
            altitude = save_detail::ParseDouble(value.value);
        } else if (n == "lon") {
            longitude = save_detail::ParseDouble(value.value);
        } else if (n == "lat") {
            latitude = save_detail::ParseDouble(value.value);
        } else if (n == "bodyName") {
            if (!value.value.empty()) body = NodeLibraryEnvironment::BodyByName(value.value);
        }
    }
    if (body == nullptr) body = NodeLibraryEnvironment::HomeBody();
}

// SurfaceArea.cs:129
void SurfaceArea::Save(ConfigNode& node) const {
    if (body == nullptr) {
        node.AddValue("bodyName", "");
    } else {
        node.AddValue("bodyName", body->name());
    }
    node.AddValue("lat", latitude);
    node.AddValue("lon", longitude);
    node.AddValue("alt", altitude);
    node.AddValue("radius", radius);
}

// SurfaceArea.cs:145
bool SurfaceArea::IsPointInCircle(double lat, double lon) const {
    return Contracts::Lite::CelestialUtilities::GreatCircleDistance(body, lat, lon, latitude, longitude) <= static_cast<double>(radius);
}

// SurfaceArea.cs:150
double SurfaceArea::PointInCircleAccuracy(double lat, double lon) const {
    const double num = Contracts::Lite::CelestialUtilities::GreatCircleDistance(body, lat, lon, latitude, longitude);
    return 1.0 - UtilMath::Clamp(num / static_cast<double>(radius), 0.0, 1.0);
}

// SurfaceArea.cs:156
std::string SurfaceArea::GetNodeBodyParameterString() const {
    const std::string text = FormatD(longitude, "0.##");
    const std::string text2 = FormatD(latitude, "0.##");
    const std::string text3 = FormatF(f32store(fx(radius) / fx(1000.0f)), "0.##");
    return Localizer::Format("#autoLOC_8007312", {BodyDisplayName(body), text, text2, text3});
}

// ---- SurfaceVolume.cs ------------------------------------------------------------------------------------------------------------

// SurfaceVolume.cs:51
SurfaceVolume::SurfaceVolume(CelestialBody* targetBody, double lon, double lat, float r, VolumeShape s, float hSphere, float hConeMin, float hConeMax)
    : body(targetBody), latitude(lat), longitude(lon), shape(s), radius(r), heightSphere(hSphere), heightConeMin(hConeMin), heightConeMax(hConeMax) {}

// SurfaceVolume.cs:63
SurfaceVolume::SurfaceVolume() {
    body = NodeLibraryEnvironment::HomeBody();
    longitude = 0.0;
    latitude = 0.0;
    shape = VolumeShape::Cone;
    ResetBodyToDefaultValues();
}

// SurfaceVolume.cs:73
void SurfaceVolume::ResetBodyToDefaultValues() {
    heightSphere = 0.0f;
    radius = f32store(fx(f32store(body->Radius)) / 4.0);
    heightConeMax = f32store(fx(f32store(body->Radius)) * 0.25);
    heightConeMin = 0.0f;
}

// SurfaceVolume.cs:81
void SurfaceVolume::AjustToBody(float r, float hSphere, float hConeMin, float hConeMax, CelestialBody* newBody, CelestialBody* pastBody) {
    const double num = static_cast<double>(r) / pastBody->Radius;
    radius = static_cast<float>(newBody->Radius * num);
    const double num2 = static_cast<double>(hSphere) / pastBody->Radius;
    heightSphere = static_cast<float>(newBody->Radius * num2);
    const double num3 = static_cast<double>(hConeMin) / pastBody->Radius;
    heightConeMin = static_cast<float>(newBody->Radius * num3);
    const double num4 = static_cast<double>(hConeMax) / pastBody->Radius;
    heightConeMax = static_cast<float>(newBody->Radius * num4);
}

// SurfaceVolume.cs:98
std::unique_ptr<FinePrint::Waypoint> SurfaceVolume::GetWaypoint() const {
    auto w = std::make_unique<FinePrint::Waypoint>();
    w->celestialName = Need(body)->name();
    w->latitude = latitude;
    w->longitude = longitude;
    w->altitude = altitude;
    w->radius = radius;
    w->nodeCaption1 = Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8200035"), dotnet::FormatSingleG(radius) + Localizer::Format("#autoLOC_289929")});
    w->nodeCaption2 = (shape == VolumeShape::Cone ? Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8200036"), dotnet::FormatSingleG(heightConeMin)})
                                                   : Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8200038"), dotnet::FormatSingleG(heightSphere)})) +
                      Localizer::Format("#autoLOC_289929");
    w->nodeCaption3 = shape == VolumeShape::Cone ? (Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8200037"), dotnet::FormatSingleG(heightConeMax)}) +
                                                    Localizer::Format("#autoLOC_289929"))
                                                 : "";
    return w;
}

// SurfaceVolume.cs:112
std::string SurfaceVolume::GetNodeBodyParameterString() const {
    const std::string text = FormatD(longitude, "0.##");
    const std::string text2 = FormatD(latitude, "0.##");
    const std::string text3 = FormatF(f32store(fx(radius) / fx(1000.0f)), "0.##");
    if (shape == VolumeShape::Sphere) {
        return Localizer::Format("#autoLOC_8007313", {BodyDisplayName(body), text, text2, text3, FormatF(f32store(fx(heightSphere) / fx(1000.0f)), "0.##")});
    }
    return Localizer::Format("#autoLOC_8007314", {BodyDisplayName(body), text, text2, text3, FormatF(f32store(fx(heightConeMin) / fx(1000.0f)), "0.##"),
                                                  FormatF(f32store(fx(heightConeMax) / fx(1000.0f)), "0.##")});
}

// SurfaceVolume.cs:124
bool SurfaceVolume::HasNodeLabel() const { return showNodeLabel ? HasWorldPosition() : false; }

// SurfaceVolume.cs:138
Vector3f SurfaceVolume::GetWorldPosition() const { return ToV3(Need(body)->GetWorldSurfacePosition(latitude, longitude, altitude)); }

// SurfaceVolume.cs:151
std::string SurfaceVolume::GetExtraText() const { return Localizer::Format("#autoLOC_8000274", {dotnet::FormatSingleG(radius)}); }

// SurfaceVolume.cs:162
bool SurfaceVolume::Equals(const SurfaceVolume& o) const {
    return body == o.body && UtilMath::Approximately(latitude, o.latitude) && UtilMath::Approximately(longitude, o.longitude) &&
           Mathf::Approximately(radius, o.radius) && shape == o.shape && Mathf::Approximately(heightSphere, o.heightSphere) &&
           Mathf::Approximately(heightConeMin, o.heightConeMin) && Mathf::Approximately(heightConeMax, o.heightConeMax);
}

// SurfaceVolume.cs:174
void SurfaceVolume::Load(const ConfigNode& node) {
    bool flag = false;
    bool flag2 = false;
    bool flag3 = false;
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        const std::string& n = value.name;
        if (n == "hConeMax") {
            flag3 = true;
            heightConeMax = save_detail::ParseFloat(value.value);
        } else if (n == "alt") {
            altitude = save_detail::ParseDouble(value.value);
        } else if (n == "hConeMin") {
            flag2 = true;
            heightConeMin = save_detail::ParseFloat(value.value);
        } else if (n == "hSphere") {
            flag = true;
            heightSphere = save_detail::ParseFloat(value.value);
        } else if (n == "lat") {
            latitude = save_detail::ParseDouble(value.value);
        } else if (n == "lon") {
            longitude = save_detail::ParseDouble(value.value);
        } else if (n == "shape") {
            shape = save_detail::ParseEnum<VolumeShape>(value.value);
        } else if (n == "targetBody") {
            if (!value.value.empty()) body = NodeLibraryEnvironment::BodyByName(value.value);
        } else if (n == "rad") {
            radius = save_detail::ParseFloat(value.value);
        }
    }
    switch (shape) {
        case VolumeShape::Sphere:
            if (flag) altitude = heightSphere;
            break;
        case VolumeShape::Cone:
            if (flag2 && flag3) altitude = f32store((fx(heightConeMin) + fx(heightConeMax)) / 2.0);
            break;
    }
    if (body == nullptr) body = NodeLibraryEnvironment::HomeBody();
}

// SurfaceVolume.cs:240
void SurfaceVolume::Save(ConfigNode& node) const {
    if (body == nullptr) {
        node.AddValue("targetBody", "");
    } else {
        node.AddValue("targetBody", body->name());
    }
    node.AddValue("lat", latitude);
    node.AddValue("lon", longitude);
    node.AddValue("alt", altitude);
    node.AddValue("rad", radius);
    node.AddValue("shape", save_detail::EnumName(shape));
    node.AddValue("hSphere", heightSphere);
    node.AddValue("hConeMin", heightConeMin);
    node.AddValue("hConeMax", heightConeMax);
}

// SurfaceVolume.cs:260
bool SurfaceVolume::IsInsideVolume(float targetLatitude, float targetLongitude, float targetAltitude) const {
    bool result = false;
    switch (shape) {
        case VolumeShape::Sphere: {
            const Vector3d relSurfacePosition = body->GetRelSurfacePosition(targetLatitude, targetLongitude, targetAltitude);
            const Vector3d relSurfacePosition2 = body->GetRelSurfacePosition(latitude, longitude, heightSphere);
            result = dotnet::Math::Pow(relSurfacePosition.x - relSurfacePosition2.x, 2.0) + dotnet::Math::Pow(relSurfacePosition.y - relSurfacePosition2.y, 2.0) +
                         dotnet::Math::Pow(relSurfacePosition.z - relSurfacePosition2.z, 2.0) <
                     dotnet::Math::Pow(static_cast<double>(radius), 2.0);
            break;
        }
        case VolumeShape::Cone:
            if (targetAltitude < heightConeMax && targetAltitude > heightConeMin) {
                result = Contracts::Lite::CelestialUtilities::GreatCircleDistance(body, targetLatitude, targetLongitude, latitude, longitude) <=
                         static_cast<double>(radius);
            }
            break;
    }
    return result;
}

// ---- Asteroid.cs / Comet.cs -----------------------------------------------------------------------------------------------------

namespace {
// uint.TryParse(text, out value): value = 0 when it fails
void TryParseUIntOut(const std::string& text, uint32_t& out) {
    uint32_t v = 0;
    if (!dotnet::TryParseUInt32(text, v)) v = 0;
    out = v;
}
}  // namespace

// Asteroid.cs:93 (UnityEngine.Random.Range(0, int.MaxValue))
uint32_t Asteroid::GetAsteroidSeed() {
    return static_cast<uint32_t>(MissionDataEnvironment::randomSeed ? MissionDataEnvironment::randomSeed() : 0);
}

// Asteroid.cs:41
void Asteroid::Randomize() {
    name = MissionDataEnvironment::generateAsteroidName ? MissionDataEnvironment::generateAsteroidName() : "Asteroid";
    persistentId = MissionDataEnvironment::UniquePersistentId();
    seed = GetAsteroidSeed();
}

// Asteroid.cs:48
void Asteroid::Load(const ConfigNode& node) {
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        const std::string& n = value.name;
        if (n == "persistentId") {
            TryParseUIntOut(value.value, persistentId);
        } else if (n == "asteroidType") {
            asteroidType = save_detail::ParseEnum<AsteroidType>(value.value);
        } else if (n == "asteroidClass") {
            asteroidClass = save_detail::ParseEnum<UntrackedObjectClass>(value.value);
        } else if (n == "asteroidSeed") {
            TryParseUIntOut(value.value, seed);
        } else if (n == "asteroidName") {
            name = value.value;
        }
    }
}

// Asteroid.cs:84
void Asteroid::Save(ConfigNode& node) const {
    node.AddValue("asteroidName", name);
    node.AddValue("asteroidSeed", seed);
    node.AddValue("asteroidClass", save_detail::EnumName(asteroidClass));
    node.AddValue("asteroidType", save_detail::EnumName(asteroidType));
    node.AddValue("persistentId", persistentId);
}

// Comet.cs:28
Comet::Comet() {
    const auto defs = MissionDataEnvironment::cometDefinitions ? MissionDataEnvironment::cometDefinitions() : std::nullopt;
    if (defs.has_value() && !defs->empty()) {
        cometType = (*defs)[0].name;
        cometDisplayType = (*defs)[0].displayName;
    } else {
        cometDisplayType = cometType = "";
    }
}

// Comet.cs:53
uint32_t Comet::GetRandomCometSeed() { return static_cast<uint32_t>(MissionDataEnvironment::randomSeed ? MissionDataEnvironment::randomSeed() : 0); }

// Comet.cs:46
void Comet::Randomize() {
    name = MissionDataEnvironment::generateCometName ? MissionDataEnvironment::generateCometName() : "Comet";
    persistentId = MissionDataEnvironment::UniquePersistentId();
    seed = GetRandomCometSeed();
}

// Comet.cs:68
void Comet::OnCometTypeChanged() {
    if (!MissionDataEnvironment::cometDefinitions) return;
    const auto defs = MissionDataEnvironment::cometDefinitions();
    if (!defs.has_value()) return;
    cometDisplayType = cometType;
    for (size_t num = 0; num < defs->size(); ++num) {
        if (cometType == (*defs)[num].name) {
            cometDisplayType = (*defs)[num].displayName;
            return;
        }
    }
}

// Comet.cs:106
void Comet::Load(const ConfigNode& node) {
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        const std::string& n = value.name;
        if (n == "persistentId") {
            TryParseUIntOut(value.value, persistentId);
        } else if (n == "cometClass") {
            cometClass = save_detail::ParseEnum<UntrackedObjectClass>(value.value);
        } else if (n == "cometType") {
            cometType = value.value;
            OnCometTypeChanged();
        } else if (n == "cometradius") {
            float f = 0.0f;
            if (!dotnet::TryParseSingle(value.value, f)) f = 0.0f;
            radius = f;
        } else if (n == "cometSeed") {
            TryParseUIntOut(value.value, seed);
        } else if (n == "cometName") {
            name = value.value;
        }
    }
}

// Comet.cs:136
void Comet::Save(ConfigNode& node) const {
    node.AddValue("cometName", name);
    node.AddValue("cometSeed", seed);
    node.AddValue("cometradius", radius);
    node.AddValue("cometType", cometType);
    node.AddValue("cometClass", save_detail::EnumName(cometClass));
    node.AddValue("persistentId", persistentId);
}

// ---- MissionKerbal.cs ------------------------------------------------------------------------------------------------------------

// MissionKerbal.cs:93
MissionKerbal::MissionKerbal(PCM k, MENode* n, UpdateNodeBodyUI update, ProtoCrewMember::KerbalType typetoShow, bool showAny, bool anyTextIsActive)
    : updateNodeBodyUI(std::move(update)), kerbal(k), typeToShow(typetoShow), node(n), selectedKerbal(k) {
    showAnyKerbal = showAny;
    anyTextIsActiveKerbal = anyTextIsActive;
}

// MissionKerbal.cs:41
std::string MissionKerbal::Name() const {
    if (kerbal != nullptr) return kerbal->name().value_or("");
    return Localizer::Format("#autoLOC_8000152");
}

// MissionKerbal.cs:53
void MissionKerbal::set_Kerbal(PCM value) {
    kerbal = std::move(value);
    if (updateNodeBodyUI) updateNodeBodyUI();
}

// MissionKerbal.cs:108
void MissionKerbal::Initialize(PCM k, MENode* n) {
    kerbal = k;
    selectedKerbal = k;
    node = n;
}

// MissionKerbal.cs:115
bool MissionKerbal::IsValid(const ProtoCrewMember& targetKerbal) const {
    if (AnyValid()) {
        if (!showAnyKerbal) return false;
        return targetKerbal.type() == typeToShow;
    }
    return kerbal->name() == targetKerbal.name() && kerbal->type() == targetKerbal.type();
}

// MissionKerbal.cs:134 / :137 / :142 / :147 (RebuildDropDownOnChange is the dropdown's: UI)
void MissionKerbal::onKerbalStatusChange(const PCM&, ProtoCrewMember::RosterStatus, ProtoCrewMember::RosterStatus) {}
void MissionKerbal::onKerbalAdded(const PCM&) {}
void MissionKerbal::onKerbalTypeChange(const PCM&, ProtoCrewMember::KerbalType, ProtoCrewMember::KerbalType) {}
void MissionKerbal::onKerbalNameChange(const PCM&, const std::string&, const std::string&) {}
// MissionKerbal.cs:154
void MissionKerbal::onKerbalRemoved(const PCM& crew) {
    if (selectedKerbal != nullptr && selectedKerbal->name() == crew->name()) selectedKerbal = nullptr;
}

// MissionKerbal.cs:296
void MissionKerbal::setcurrentKerbalAvailable() {
    if (selectedKerbal != nullptr) {
        selectedKerbal->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
        selectedKerbal = nullptr;
    }
}

// MissionKerbal.cs:305
std::string MissionKerbal::GetNodeBodyParameterString() const {
    if (kerbal != nullptr) return Localizer::Format("#autoLOC_8002022", {kerbal->displayName()});
    if (typeToShow == ProtoCrewMember::KerbalType::Tourist) return Localizer::Format("#autoLOC_8002022", {Localizer::Format("#autoLOC_8002021")});
    return Localizer::Format("#autoLOC_8002022", {Localizer::Format("#autoLOC_8000152")});
}

// MissionKerbal.cs:318
bool MissionKerbal::Equals(const MissionKerbal& o) const {
    if (typeToShow == o.typeToShow) {
        if (kerbal == nullptr) throw NullReferenceException();
        return kerbal == o.kerbal;
    }
    return false;
}

// MissionKerbal.cs:336
void MissionKerbal::Save(ConfigNode& n) const {
    if (kerbal != nullptr) n.AddValue("kerbalName", kerbal->name().value_or(""));
    n.AddValue("kerbalType", save_detail::EnumName(typeToShow));
}

// MissionKerbal.cs:345
void MissionKerbal::Load(const ConfigNode& n) {
    std::string value;
    kerbal = nullptr;
    selectedKerbal = nullptr;
    n.TryGetValue("kerbalName", value);
    if (!value.empty()) {
        if (node == nullptr) throw NullReferenceException();
        PCM protoCrewMember = (*node->mission->situation->crewRoster)[value];
        if (protoCrewMember != nullptr) {
            kerbal = protoCrewMember;
            selectedKerbal = kerbal;
        } else {
            log::Info("Unable to find kerbal (" + value + ") in mission roster, setting to Any Kerbal");
        }
    }
    save_detail::TryGetEnum(n, "kerbalType", typeToShow, ProtoCrewMember::KerbalType::Crew);
}

// ---- ParamChoices_CelestialBodySurface.cs / ParamChoices_VesselSimpleLocation.cs ------------------------------------------------

ParamChoices_CelestialBodySurface::ParamChoices_CelestialBodySurface() = default;
ParamChoices_CelestialBodySurface::~ParamChoices_CelestialBodySurface() = default;

// ParamChoices_CelestialBodySurface.cs:40
bool ParamChoices_CelestialBodySurface::HasWaypoint() const { return locationChoice == Choices::areaData ? areaData->HasWaypoint() : false; }
// ParamChoices_CelestialBodySurface.cs:50
std::unique_ptr<FinePrint::Waypoint> ParamChoices_CelestialBodySurface::GetWaypoint() const {
    if (locationChoice == Choices::areaData) return areaData->GetWaypoint();
    return nullptr;
}
// ParamChoices_CelestialBodySurface.cs:65
bool ParamChoices_CelestialBodySurface::HasWorldPosition() const { return locationChoice == Choices::areaData ? areaData->HasNodeLabel() : false; }
// ParamChoices_CelestialBodySurface.cs:75
Vector3f ParamChoices_CelestialBodySurface::GetWorldPosition() const {
    if (locationChoice == Choices::areaData) return areaData->GetWorldPosition();
    return Vector3f();
}
// ParamChoices_CelestialBodySurface.cs:85
std::string ParamChoices_CelestialBodySurface::GetExtraText() const {
    if (locationChoice == Choices::areaData) return areaData->GetExtraText();
    return "";
}
// ParamChoices_CelestialBodySurface.cs:95
std::string ParamChoices_CelestialBodySurface::NodeBodyParameterString() const {
    switch (locationChoice) {
        default: return Localizer::Format("#autoLOC_8000272");
        case Choices::bodyData: return bodyData->GetNodeBodyParameterString();
        case Choices::biomeData: return biomeData->GetNodeBodyParameterString();
        case Choices::areaData: return areaData->GetNodeBodyParameterString();
        case Choices::launchSiteName:
            return Localizer::Format("#autoLOC_8004190",
                                     {Localizer::Format("#autoLOC_8000067"),
                                      MissionDataEnvironment::launchSiteDisplayName ? MissionDataEnvironment::launchSiteDisplayName(launchSiteName) : launchSiteName});
    }
}
// ParamChoices_CelestialBodySurface.cs:112
void ParamChoices_CelestialBodySurface::Load(const ConfigNode& node) {
    save_detail::TryGetEnum(node, "locationChoice", locationChoice, Choices::bodyData);
    ConfigNode* node2 = nullptr;
    if (node.TryGetNode("BODYDATA", node2)) {
        if (bodyData == nullptr) throw NullReferenceException();
        bodyData->Load(*node2);
    }
    if (node.TryGetNode("BIOMEDATA", node2)) {
        if (biomeData == nullptr) throw NullReferenceException();
        biomeData->Load(*node2);
    }
    if (node.TryGetNode("GROUNDAREA", node2)) {
        if (areaData == nullptr) throw NullReferenceException();
        areaData->Load(*node2);
    }
    node.TryGetValue("LAUNCHSITEDATA", launchSiteName);
}
// ParamChoices_CelestialBodySurface.cs:131
void ParamChoices_CelestialBodySurface::Save(ConfigNode& node) const {
    node.AddValue("locationChoice", save_detail::EnumName(locationChoice));
    if (bodyData != nullptr) bodyData->Save(*node.AddNode("BODYDATA"));
    if (biomeData != nullptr) biomeData->Save(*node.AddNode("BIOMEDATA"));
    if (areaData != nullptr) areaData->Save(*node.AddNode("GROUNDAREA"));
    node.AddValue("LAUNCHSITEDATA", launchSiteName);
}

// ParamChoices_VesselSimpleLocation.cs:25
std::string ParamChoices_VesselSimpleLocation::GetNodeBodyParameterString() const {
    switch (locationChoice) {
        default: return "";
        case Choices::orbit: return orbit->GetNodeBodyParameterString();
        case Choices::landed: return landed->GetNodeBodyParameterString();
    }
}
// ParamChoices_VesselSimpleLocation.cs:38
void ParamChoices_VesselSimpleLocation::Load(const ConfigNode& node) {
    save_detail::TryGetEnum(node, "locationChoice", locationChoice, Choices::orbit);
    ConfigNode* node2 = nullptr;
    if (node.TryGetNode("LANDEDDATA", node2)) {
        if (landed == nullptr) throw NullReferenceException();
        landed->Load(*node2);
    }
    if (node.TryGetNode("ORBITDATA", node2)) {
        if (orbit == nullptr) throw NullReferenceException();
        orbit->Load(*node2);
    }
}
// ParamChoices_VesselSimpleLocation.cs:52
void ParamChoices_VesselSimpleLocation::Save(ConfigNode& node) const {
    node.AddValue("locationChoice", save_detail::EnumName(locationChoice));
    if (landed != nullptr) landed->Save(*node.AddNode("LANDEDDATA"));
    if (orbit != nullptr) orbit->Save(*node.AddNode("ORBITDATA"));
}

// ---- MissionCraft.cs -------------------------------------------------------------------------------------------------------------

namespace {
bool FileExists(const std::string& p) {
    if (MissionDataEnvironment::fileExists) return MissionDataEnvironment::fileExists(p);
    std::ifstream f(p, std::ios::binary);
    return f.good();
}
ConfigNode::NodePtr LoadCfg(const std::string& p) {
    if (MissionDataEnvironment::loadConfigNode) return MissionDataEnvironment::loadConfigNode(p);
    return ConfigNode::Load(p);
}
std::string TrimEndSlash(const std::string& s) {
    size_t e = s.size();
    while (e > 0 && s[e - 1] == '/') --e;
    return s.substr(0, e);
}
}  // namespace

// MissionCraft.cs:73
MissionCraft::MissionCraft(const std::string& folder, const std::string& file) : craftFile(file), craftFolder(folder), facility("") {
    configNodeCache = nullptr;
    const std::string trimmed = TrimEndSlash(folder);
    const size_t slash = trimmed.rfind('/');
    const std::string last = slash == std::string::npos ? trimmed : trimmed.substr(slash + 1);
    if (last == "VAB") {
        facility = "VAB";
    } else if (last == "SPH") {
        facility = "SPH";
    }
    checkPersistentIds();
}

// MissionCraft.cs:25
ConfigNode* MissionCraft::CraftNode() {
    if (configNodeCache == nullptr) {
        const std::string baseFolder = TrimEndSlash(craftFolder);
        if (FileExists(baseFolder + "/" + craftFile)) {
            configNodeCache = LoadCfg(baseFolder + "/" + craftFile);
        } else if (FileExists(baseFolder + "/VAB/" + craftFile)) {
            configNodeCache = LoadCfg(baseFolder + "/VAB/" + craftFile);
            facility = "VAB";
        } else if (FileExists(baseFolder + "/SPH/" + craftFile)) {
            configNodeCache = LoadCfg(baseFolder + "/SPH/" + craftFile);
            facility = "SPH";
        }
    }
    return configNodeCache.get();
}

// MissionCraft.cs:50
std::string MissionCraft::VesselName() {
    std::string value;
    if (CraftNode() != nullptr) CraftNode()->TryGetValue("ship", value);
    return value;
}

// MissionCraft.cs:63
uint32_t MissionCraft::persistentId() {
    uint32_t value = 0;
    if (CraftNode() == nullptr) throw NullReferenceException();
    CraftNode()->TryGetValue("persistentId", value);
    return value;
}

// MissionCraft.cs:90
std::string MissionCraft::GetPartNameInVessel(uint32_t id) {
    if (CraftNode() == nullptr) return "No part selected";
    const std::vector<ConfigNode*> nodes = CraftNode()->GetNodes("PART");
    int num = static_cast<int>(nodes.size());
    do {
        if (num-- <= 0) return "No part selected";
    } while (id != GetPartPersistentId(nodes[static_cast<size_t>(num)]));
    return GetPartName(nodes[static_cast<size_t>(num)]);
}

// MissionCraft.cs:109
uint32_t MissionCraft::GetPartPersistentId(ConfigNode* node) {
    if (node == nullptr) return 0;
    std::string value;
    if (node->TryGetValue("persistentId", value)) return save_detail::ParseUInt(value);
    const uint32_t uniquepersistentId = MissionDataEnvironment::UniquePersistentId();
    node->AddValue("persistentId", dotnet::FormatUInt64(uniquepersistentId));
    return uniquepersistentId;
}

// MissionCraft.cs:125
std::string MissionCraft::GetPartName(ConfigNode* node) {
    if (node == nullptr) return "";
    std::string value;
    if (node->TryGetValue("part", value)) {
        value = KSPUtil::GetPartName(value);
        value = MissionDataEnvironment::partTitle ? MissionDataEnvironment::partTitle(value) : value;
    }
    return value;
}

// MissionCraft.cs:140
void MissionCraft::checkPersistentIds() {
    if (CraftNode() == nullptr) return;
    bool flag = false;
    ConfigNode::NodePtr configNode = configNodeCache->CreateCopy();
    if (!configNode->HasValue("persistentId")) {
        configNode->AddValue("persistentId", MissionDataEnvironment::UniquePersistentId());
        flag = true;
    }
    configNode->TryGetValue("persistentId", craftPersistentId);
    const std::vector<ConfigNode*> nodes = configNode->GetNodes("PART");
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!nodes[i]->HasValue("persistentId")) {
            nodes[i]->AddValue("persistentId", MissionDataEnvironment::UniquePersistentId());
            flag = true;
        }
        if (i == 0) nodes[i]->TryGetValue("persistentId", rootPartPersistentId);
    }
    if (flag) {
        if (MissionDataEnvironment::saveConfigNode) {
            MissionDataEnvironment::saveConfigNode(*configNode, craftFolder + craftFile);
        } else {
            configNode->Save(craftFolder + craftFile);
        }
        configNodeCache = nullptr;
    }
}

// MissionCraft.cs:174
void MissionCraft::Clear() {
    if (configNodeCache != nullptr) {
        configNodeCache->ClearData();
        configNodeCache = nullptr;
    }
}

}  // namespace ksp::Expansions::Missions
