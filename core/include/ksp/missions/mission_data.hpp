// WP-54 slice 2: the mission data classes (Assembly-CSharp/Expansions/Missions): MissionQuaternion, MissionOrbit, Editor/MissionBiome,
// VesselGroundLocation, VesselLocation, LaunchSiteSituation, Crew, SurfaceArea, SurfaceVolume, Asteroid, Comet, MissionKerbal,
// ParamChoices_CelestialBodySurface, ParamChoices_VesselSimpleLocation, MissionCraft, MissionScoreInfo, MissionLocalizer.
//
// Conventions (on top of node_library.hpp):
//   * `[MEGUI_*]` attributes, MEGUIDropDownItem lists (SetCelestialBodies, SetBiomes, SetKerbalList ...), GAP* gizmo / prefab callbacks and
//     the MEGUIParameter controls are editor UI and are not ported; the data, Load / Save, Equals and the text helpers are.
//   * C# `GetHashCode()` overrides exist only for Equals pairing and are not ported.
//   * FlightGlobals.Bodies / GetHomeBody() / GetBodyByName go through NodeLibraryEnvironment (Bodies / HomeBody / BodyByName).
//   * `Waypoint GetWaypoint()` returns a new object in C#: std::unique_ptr<FinePrint::Waypoint> here.
//   * Seams to the world that is not in kspcore: MissionDataEnvironment (below).
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/contracts/waypoints.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/missions/mission_display.hpp"
#include "ksp/missions/mission_types.hpp"
#include "ksp/missions/node_library.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/orbit_snapshot.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/vessel/discovery_info.hpp"

namespace ksp {
class CelestialBody;
}

namespace ksp::Expansions::Missions {

class MENode;
class Mission;

// ---------------------------------------------------------------------------------------------------------------------------
struct MissionDataEnvironment {
    // PSystemSetup.Instance.GetLaunchSiteDisplayName(name) (ParamChoices_CelestialBodySurface.NodeBodyParameterString :launchSiteName).
    // Unset: the name itself.
    static inline std::function<std::string(const std::string&)> launchSiteDisplayName;
    // UnityEngine.Random.Range(0, int.MaxValue) (Asteroid.GetAsteroidSeed / Comet.GetRandomCometSeed). Unset: 0.
    static inline std::function<int32_t()> randomSeed;
    // DiscoverableObjectsUtil.GenerateAsteroidName() / GenerateCometName(). Unset: "Asteroid" / "Comet".
    static inline std::function<std::string()> generateAsteroidName;
    static inline std::function<std::string()> generateCometName;
    // FlightGlobals.GetUniquepersistentId(). Unset: MissionMappedVessel::getUniquePersistentId.
    static uint32_t UniquePersistentId();
    // CometManager.Instance.cometDefinitions (name, displayName) in order; unset: CometManager.Instance == null.
    struct CometDefinition {
        std::string name, displayName;
    };
    static inline std::function<std::optional<std::vector<CometDefinition>>()> cometDefinitions;
    // File system (MissionCraft): File.Exists / ConfigNode.Load / ConfigNode.Save. Unset: the real file system.
    static inline std::function<bool(const std::string& path)> fileExists;
    static inline std::function<ConfigNode::NodePtr(const std::string& path)> loadConfigNode;
    static inline std::function<void(const ConfigNode&, const std::string& path)> saveConfigNode;
    // Directory.Exists(path) / Directory.GetFiles(path, "*.craft") (Mission.ProcessCraftFileFolder) and Path.GetFileName. Unset: std::filesystem (files in
    // ordinal name order, which is the NTFS enumeration order of .NET on Windows).
    static inline std::function<bool(const std::string& path)> directoryExists;
    static inline std::function<std::vector<std::string>(const std::string& path, const std::string& extension)> getFiles;
    // PartLoader.getPartInfoByName(name).title (MissionCraft.GetPartName). Unset: the name.
    static inline std::function<std::string(const std::string& partName)> partTitle;
    // MissionsUtils.UsersMissionsPath (MissionScoreInfo.saveFileName is appended). Unset: "".
    static inline std::function<std::string()> usersMissionsPath;
    static void Reset();
};

// ---------------------------------------------------------------------------------------------------------------------------
// MissionQuaternion.cs:6 (Unity float Vector3 of Euler angles)
class MissionQuaternion {
public:
    MissionQuaternion() = default;                          // :60
    explicit MissionQuaternion(Vector3f newAngles) : angles(newAngles) {}  // :65
    float eulerX() const { return angles.x; }               // :12
    float eulerY() const { return angles.y; }               // :26
    float eulerZ() const { return angles.z; }               // :40
    void set_eulerX(float v) { angles.x = v; }
    void set_eulerY(float v) { angles.y = v; }
    void set_eulerZ(float v) { angles.z = v; }
    Vector3f eulerAngles() const { return angles; }         // :54
    void set_eulerAngles(Vector3f v) { angles = v; }
    // `Equals` is not overridden by the C# (reference equality); VesselGroundLocation.Equals calls it
    bool Equals(const MissionQuaternion& other) const { return this == &other; }

private:
    Vector3f angles;
};

// ---------------------------------------------------------------------------------------------------------------------------
// Editor/MissionBiome.cs:6
namespace Editor {
class MissionBiome {
public:
    CelestialBody* body = nullptr;                          // :9
    std::string biomeName;                                  // :12
    MissionBiome(CelestialBody* celestialBody, std::string biomeName);  // :14
    void Save(ConfigNode& node) const;                      // :21
    void Load(const ConfigNode& node);                      // :30
    bool Equals(const MissionBiome& other) const;           // :71
    std::string GetNodeBodyParameterString() const;         // :90
};
}  // namespace Editor

// ---------------------------------------------------------------------------------------------------------------------------
// MissionOrbit.cs:9 (OrbitSnapshot subclass; Orbit mutated by the property setters)
class MissionOrbit : public OrbitSnapshot {
public:
    explicit MissionOrbit(CelestialBody* bodyRef);          // :180
    explicit MissionOrbit(const Orbit& orbit);              // :188
    explicit MissionOrbit(const ConfigNode& node);          // :196
    MissionOrbit(const MissionOrbit& other);
    MissionOrbit& operator=(const MissionOrbit& other);

    Orbit* orbit() const { return orbit_.get(); }           // :17 Orbit { get; protected set; }
    CelestialBody* Body() const;                            // :20
    void set_Body(CelestialBody* value);
    double SemiMajorAxis() const { return semiMajorAxis; }  // :33
    void set_SemiMajorAxis(double value);
    double Apoapsis() const;                                // :52
    void set_Apoapsis(double value);
    double Periapsis() const;                               // :80
    void set_Periapsis(double value);
    double Eccentricity() const { return eccentricity; }    // :107
    void set_Eccentricity(double value);
    double Inclination() const { return inclination; }      // :125
    void set_Inclination(double value);
    double Lan() const { return LAN; }                      // :140
    void set_Lan(double value);
    double ArgumentOfPeriapsis() const { return argOfPeriapsis; }  // :155
    void set_ArgumentOfPeriapsis(double value);
    double Epoch() const { return epoch; }                  // :170
    void set_Epoch(double value);
    double MeanAnomalyAtEpoch() const { return degreesMNA; }  // :185
    void set_MeanAnomalyAtEpoch(double value);

    Orbit RelativeOrbit(Mission* mission) const;            // :219
    void Reset();                                           // :236
    void Save(ConfigNode& node) const override;             // :246
    void Load(const ConfigNode& node);                      // :252
    bool Equals(const MissionOrbit& other) const;           // :299
    std::string GetNodeBodyParameterString() const;         // :321
    static double GetEditorMaxOrbitRadius(CelestialBody* body);  // :326

private:
    void Construct();
    static double GetUTOrbitRotation(const Orbit& orbit, double UT);  // :228
    std::unique_ptr<Orbit> orbit_;
    double smaCache = 0.0;                                  // :11
    double degreesMNA = 0.0;                                // :13
    double bodyRotationEditor = 0.0;                        // :15
};

// ---------------------------------------------------------------------------------------------------------------------------
// VesselGroundLocation.cs:12
class VesselGroundLocation {
public:
    enum class GizmoIcon { Rocket = 0, AirCraft = 1, Asteroid = 2, Kerbal = 3, LaunchSite = 4, Flag = 5, Comet = 6 };  // :15

    VesselGroundLocation();                                 // :88
    VesselGroundLocation(MENode* node, GizmoIcon icon);     // :93 (KSPRandom: seeded from the tick count)

    GizmoIcon gizmoIcon = GizmoIcon::Flag;                  // :33
    CelestialBody* targetBody = nullptr;                    // :36
    bool splashed = false;                                  // :39
    double latitude = 0.0;                                  // :42
    double longitude = 0.0;                                 // :48
    double altitude = 0.0;                                  // :54
    std::unique_ptr<MissionQuaternion> rotation;            // :57
    double startLatMin = -0.008999999612569809;             // :59
    double startLatMax = 0.15530000627040863;               // :61
    double startLonMin = -74.82420349121094;                // :63
    double startLonMax = -74.51119995117188;                // :65
    std::function<void(GizmoIcon)> OnGizmoIconChange;       // :27

    GizmoIcon GAPGizmoIcon() const { return gizmoIcon; }    // :69
    void set_GAPGizmoIcon(GizmoIcon value);
    bool HasWaypoint() const { return true; }               // :112
    std::unique_ptr<FinePrint::Waypoint> GetWaypoint() const;  // :117
    bool HasNodeLabel() const { return true; }              // :127
    Vector3f GetWorldPosition() const;                      // :132
    std::string GetExtraText() const;                       // :137
    std::string GetNodeBodyParameterString() const;         // :142
    bool Equals(const VesselGroundLocation& other) const;   // :150
    void Load(const ConfigNode& node);                      // :186
    void Save(ConfigNode& node) const;                      // :227
};

// ---------------------------------------------------------------------------------------------------------------------------
// VesselLocation.cs:9
class VesselLocation {
public:
    // MissionSituation.VesselStartSituations (MissionSituation.cs:24)
    enum class VesselStartSituations { LANDED = 1, PRELAUNCH = 4, ORBITING = 0x20 };
    VesselStartSituations situation = VesselStartSituations::PRELAUNCH;  // :12
    EditorFacility facility = EditorFacility::VAB;          // :15
    bool brakesOn = false;                                  // :18
    std::string launchSite;                                 // :21
    std::unique_ptr<MissionOrbit> orbitSnapShot;            // :24
    std::unique_ptr<VesselGroundLocation> vesselGroundLocation;  // :27

    VesselLocation();                                       // :30
    explicit VesselLocation(MENode* node);                  // :36
    void Load(const ConfigNode& node);                      // :70
    void Save(ConfigNode& node);                            // :105
    std::unique_ptr<MissionOrbit> createZeroOrbit() const;  // :56
};

// Crew.cs:8 is declared in ksp/kerbals/crew_manifest.hpp (WP-31); its methods are defined in mission_data.cpp

// ---------------------------------------------------------------------------------------------------------------------------
// LaunchSiteSituation.cs:12 (the Instantiate / PQSCity2 / CommNetHome object creation of AddLaunchSite is the scene's: LaunchSiteEnvironment)
class LaunchSiteSituation {
public:
    std::string launchSiteName;                             // :14 = Localizer.Format("#autoLOC_8100066")
    EditorFacility facility = EditorFacility::VAB;          // :17
    bool showRamp = false;                                  // :20
    std::unique_ptr<VesselGroundLocation> launchSiteGroundLocation;  // :23
    bool splashed = false;                                  // :26
    explicit LaunchSiteSituation(MENode* node);             // :52
    std::string LaunchSiteObjectName() const;               // :40 (launchSiteObjectName cache: private, rebuilt on every call)
    void Load(const ConfigNode& node);                      // :194
    void Save(ConfigNode& node) const;                      // :241
    // :174 RemoveLaunchSite: MakingHistory installed and PSystemSetup.GetLaunchSite(name) != null -> RemoveLaunchSite(name)
    void RemoveLaunchSite();
    // :194 AddLaunchSite(createObject): the decision tree up to the scene calls; `launchSiteObject` is the scene object handle
    struct SceneHooks {
        std::function<bool()> mobileLaunchSitePrefabExists;                 // PSystemSetup.Instance != null && mobileLaunchSitePrefab != null
        std::function<bool(const std::string&)> launchSiteExists;           // PSystemSetup.Instance.GetLaunchSite(name) != null
        // launchSite.pqsCity2 == null -> nullopt, else pqsCity2.celestialBody
        std::function<std::optional<CelestialBody*>(const std::string&)> launchSiteCityBody;
        std::function<void(const std::string&)> removeLaunchSite;           // PSystemSetup.Instance.RemoveLaunchSite
        std::function<void(const std::string& message)> postScreenMessage;  // ScreenMessages.PostScreenMessage
        std::function<bool()> loadedSceneIsMissionBuilder;                  // HighLogic.LoadedSceneIsMissionBuilder
        std::function<void(LaunchSiteSituation&)> createLaunchSiteObject;   // createLaunchSiteObject() coroutine body (scene)
        std::function<void(LaunchSiteSituation&)> createEmptyLaunchSite;    // createEmptyLaunchSite()
    };
    static inline SceneHooks scene;
    bool launchSiteObjectExists = false;                    // :34 launchSiteObject != null
    std::optional<CelestialBody*> launchSiteObjectCityBody; // launchSiteObject.GetComponent<PQSCity2>() -> celestialBody (nullopt = no component)
    void AddLaunchSite(bool createObject);                  // :78 (the coroutine yields are the scene's)
};

// ---------------------------------------------------------------------------------------------------------------------------
// SurfaceArea.cs:13
class SurfaceArea {
public:
    CelestialBody* body = nullptr;                          // :16
    double latitude = 0.0;                                  // :19
    double longitude = 0.0;                                 // :22
    double altitude = 0.0;                                  // :25
    float radius = 0.0f;                                    // :28
    SurfaceArea();                                          // :30
    SurfaceArea(CelestialBody* targetBody, double longitude, double latitude, float radius);  // :35
    bool HasWaypoint() const { return true; }               // :43
    std::unique_ptr<FinePrint::Waypoint> GetWaypoint() const;  // :48
    bool HasNodeLabel() const { return HasWorldPosition(); }  // :60
    bool HasWorldPosition() const { return true; }          // :65
    Vector3f GetWorldPosition() const;                      // :70
    std::string GetExtraText() const;                       // :75
    bool Equals(const SurfaceArea& other) const;            // :80
    void Load(const ConfigNode& node);                      // :96
    void Save(ConfigNode& node) const;                      // :135
    bool IsPointInCircle(double lat, double lon) const;     // :155
    double PointInCircleAccuracy(double lat, double lon) const;  // :160
    std::string GetNodeBodyParameterString() const;         // :166
};

// ---------------------------------------------------------------------------------------------------------------------------
// SurfaceVolume.cs:14
class SurfaceVolume {
public:
    enum class VolumeShape { Cone = 0, Sphere = 1 };        // :17
    CelestialBody* body = nullptr;                          // :27
    double latitude = 0.0;                                  // :30
    double longitude = 0.0;                                 // :33
    double altitude = 0.0;                                  // :36
    VolumeShape shape = VolumeShape::Cone;                  // :39
    float radius = 0.0f;                                    // :42
    float heightSphere = 0.0f;                              // :45
    float heightConeMin = 0.0f;                             // :48
    float heightConeMax = 0.0f;                             // :51
    bool showNodeLabel = false;                             // :54
    SurfaceVolume(CelestialBody* targetBody, double longitude, double latitude, float radius, VolumeShape shape, float heightSphere,
                  float heightConeMin, float heightConeMax);  // :56
    SurfaceVolume();                                        // :68
    void ResetBodyToDefaultValues();                        // :77
    void AjustToBody(float radius, float heightSphere, float heigthConeMin, float heightConeMax, CelestialBody* newBody, CelestialBody* pastBody);  // :85
    bool HasWaypoint() const { return true; }               // :100
    std::unique_ptr<FinePrint::Waypoint> GetWaypoint() const;  // :105
    std::string GetNodeBodyParameterString() const;         // :120
    bool HasNodeLabel() const;                              // :130
    bool HasWorldPosition() const { return true; }          // :139
    Vector3f GetWorldPosition() const;                      // :144
    std::string GetExtraText() const;                       // :159
    bool Equals(const SurfaceVolume& other) const;          // :164
    void Load(const ConfigNode& node);                      // :183
    void Save(ConfigNode& node) const;                      // :250
    bool IsInsideVolume(float targetLatitude, float targetLongitude, float targetAltitude) const;  // :270
};

// ---------------------------------------------------------------------------------------------------------------------------
// Asteroid.cs:11 / Comet.cs:10
class Asteroid {
public:
    enum class AsteroidType { Regular = 0, Glimmeroid = 1 };  // :14
    std::string name = "Asteroid";                          // :21
    uint32_t seed = 0;                                      // :23
    AsteroidType asteroidType = AsteroidType::Regular;      // :26
    UntrackedObjectClass asteroidClass = UntrackedObjectClass::C;  // :29
    uint32_t persistentId = 0;                              // :31
    Asteroid() = default;                                   // :33
    void Randomize();                                       // :42
    void Load(const ConfigNode& node);                      // :49
    void Save(ConfigNode& node) const;                      // :87
    static uint32_t GetAsteroidSeed();                      // :95
};

class Comet {
public:
    std::string name = "Comet";                             // :12
    uint32_t seed = 0;                                      // :15
    float radius = 0.0f;                                    // :17
    std::string cometType;                                  // :20
    std::string cometDisplayType;                           // :22
    UntrackedObjectClass cometClass = UntrackedObjectClass::C;  // :25
    uint32_t persistentId = 0;                              // :27
    Comet();                                                // :29
    void Randomize();                                       // :48
    static uint32_t GetRandomCometSeed();                   // :55
    void OnCometTypeChanged();                              // :70
    void Load(const ConfigNode& node);                      // :111
    void Save(ConfigNode& node) const;                      // :145
};

// ---------------------------------------------------------------------------------------------------------------------------
// MissionKerbal.cs:10
class MissionKerbal {
public:
    using PCM = std::shared_ptr<ProtoCrewMember>;
    using UpdateNodeBodyUI = std::function<void()>;         // :12 delegate
    ProtoCrewMember::RosterStatus statusToShow = ProtoCrewMember::RosterStatus::Available;  // :31
    bool showStrandedOnly = false;                          // :33
    bool showAnyKerbal = false;                             // :35
    bool anyTextIsActiveKerbal = false;                     // :37
    bool showAllRosterStatus = false;                       // :39
    UpdateNodeBodyUI updateNodeBodyUI;                      // :41

    MissionKerbal() = default;                              // :89
    MissionKerbal(PCM kerbal, MENode* node, UpdateNodeBodyUI updateNodeBodyUI,
                  ProtoCrewMember::KerbalType typetoShow = ProtoCrewMember::KerbalType::Crew, bool showAnyKerbal = true,
                  bool anyTextIsActiveKerbal = false);      // :93
    std::string Name() const;                               // :43
    PCM Kerbal() const { return kerbal; }                   // :54
    void set_Kerbal(PCM value);                             // :54
    ProtoCrewMember::KerbalType TypeToShow() const { return typeToShow; }  // :70
    void set_TypeToShow(ProtoCrewMember::KerbalType v) { typeToShow = v; }
    bool AnyValid() const { return kerbal == nullptr; }     // :82
    void set_AnyValid() { kerbal = nullptr; }
    void Initialize(PCM kerbal, MENode* node);              // :104
    bool IsValid(const ProtoCrewMember& targetKerbal) const;  // :111
    void onKerbalStatusChange(const PCM& crew, ProtoCrewMember::RosterStatus prev, ProtoCrewMember::RosterStatus now);  // :132
    void onKerbalAdded(const PCM& crew);                    // :137
    void onKerbalTypeChange(const PCM& crew, ProtoCrewMember::KerbalType oldType, ProtoCrewMember::KerbalType newType);  // :142
    void onKerbalNameChange(const PCM& crew, const std::string& oldName, const std::string& newName);  // :147
    void onKerbalRemoved(const PCM& crew);                  // :152
    void setcurrentKerbalAvailable();                       // :287
    std::string GetNodeBodyParameterString() const;        // :298
    bool Equals(const MissionKerbal& other) const;          // :315
    void Save(ConfigNode& node) const;                      // :331
    void Load(const ConfigNode& node);                      // :341
    PCM kerbal;                                             // :17 (private field `kerbal`; ActionCreateKerbal reads it)

private:
    ProtoCrewMember::KerbalType typeToShow = ProtoCrewMember::KerbalType::Crew;  // :14
    bool settingKerbal = false;                             // :19
    MENode* node = nullptr;                                 // :21
    PCM selectedKerbal;                                     // :23
};

// ---------------------------------------------------------------------------------------------------------------------------
// ParamChoices_CelestialBodySurface.cs:11 / ParamChoices_VesselSimpleLocation.cs:6
class ParamChoices_CelestialBodySurface {
public:
    enum class Choices { anyData = 0, bodyData = 1, biomeData = 2, areaData = 3, launchSiteName = 4 };  // :13
    Choices locationChoice = Choices::bodyData;             // :27
    std::unique_ptr<MissionCelestialBody> bodyData;         // :30
    std::unique_ptr<Editor::MissionBiome> biomeData;        // :33
    std::unique_ptr<SurfaceArea> areaData;                  // :36
    std::string launchSiteName = "LaunchPad";               // :39
    ParamChoices_CelestialBodySurface();
    ~ParamChoices_CelestialBodySurface();
    bool HasWaypoint() const;                               // :41
    std::unique_ptr<FinePrint::Waypoint> GetWaypoint() const;  // :51
    bool HasNodeLabel() const { return HasWorldPosition(); }  // :61
    bool HasWorldPosition() const;                          // :66
    Vector3f GetWorldPosition() const;                      // :76
    std::string GetExtraText() const;                       // :86
    std::string NodeBodyParameterString() const;            // :96
    void Load(const ConfigNode& node);                      // :114
    void Save(ConfigNode& node) const;                      // :134
};

class ParamChoices_VesselSimpleLocation {
public:
    enum class Choices { landed = 0, orbit = 1 };           // :8
    Choices locationChoice = Choices::landed;               // :18
    std::unique_ptr<VesselGroundLocation> landed;           // :21
    std::unique_ptr<MissionOrbit> orbit;                    // :24
    std::string GetNodeBodyParameterString() const;         // :26
    void Load(const ConfigNode& node);                      // :40
    void Save(ConfigNode& node) const;                      // :53
};

// ---------------------------------------------------------------------------------------------------------------------------
// MissionCraft.cs:8
class MissionCraft {
public:
    std::string craftFile;                                  // :11
    std::string craftFolder;                                // :13
    std::string facility;                                   // :15
    bool mappedItem = false;                                // :17
    std::string mappedVesselName;                           // :19
    uint32_t craftPersistentId = 0;                         // :21
    uint32_t rootPartPersistentId = 0;                      // :23
    MissionCraft(const std::string& craftFolder, const std::string& craftFile);  // :71
    ConfigNode* CraftNode();                                // :27
    std::string VesselName();                               // :52
    uint32_t persistentId();                                // :61
    std::string GetPartNameInVessel(uint32_t persistentId); // :87
    uint32_t GetPartPersistentId(ConfigNode* node);         // :107
    void Clear();                                           // :174

private:
    std::string GetPartName(ConfigNode* node);              // :123
    void checkPersistentIds();                              // :137
    ConfigNode::NodePtr configNodeCache;                    // :25
};

// ---------------------------------------------------------------------------------------------------------------------------
// MissionLocalizer.cs:7 / MissionScoreInfo.cs:6 live in mission_scoreinfo.hpp (they need ScoreInfo and Mission)

}  // namespace ksp::Expansions::Missions

template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::VesselLocation::VesselStartSituations> {
    using E = ksp::Expansions::Missions::VesselLocation::VesselStartSituations;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::LANDED, "LANDED", "#autoLOC_8100075"},
                                                {E::PRELAUNCH, "PRELAUNCH", "#autoLOC_8000067"},
                                                {E::ORBITING, "ORBITING", "#autoLOC_8100077"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::VesselGroundLocation::GizmoIcon> {
    using E = ksp::Expansions::Missions::VesselGroundLocation::GizmoIcon;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Rocket, "Rocket", "#autoLOC_8007216"},     {E::AirCraft, "AirCraft", "#autoLOC_8007217"},
                                                {E::Asteroid, "Asteroid", "#autoLOC_8007218"}, {E::Kerbal, "Kerbal", "#autoLOC_8007219"},
                                                {E::LaunchSite, "LaunchSite", "#autoLOC_8000067"}, {E::Flag, "Flag", "#autoLOC_8007221"},
                                                {E::Comet, "Comet", "#autoLOC_8007221"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::SurfaceVolume::VolumeShape> {
    using E = ksp::Expansions::Missions::SurfaceVolume::VolumeShape;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Cone, "Cone", "#autoLOC_8100034"}, {E::Sphere, "Sphere", "#autoLOC_8100031"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Asteroid::AsteroidType> {
    using E = ksp::Expansions::Missions::Asteroid::AsteroidType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Regular, "Regular", "#autoLOC_8100110"}, {E::Glimmeroid, "Glimmeroid", "#autoLOC_8100111"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::ParamChoices_CelestialBodySurface::Choices> {
    using E = ksp::Expansions::Missions::ParamChoices_CelestialBodySurface::Choices;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::anyData, "anyData", "#autoLOC_8000272"},   {E::bodyData, "bodyData", "#autoLOC_8000263"},
                                                {E::biomeData, "biomeData", "#autoLOC_8000136"}, {E::areaData, "areaData", "#autoLOC_8000264"},
                                                {E::launchSiteName, "launchSiteName", "#autoLOC_8000067"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::ParamChoices_VesselSimpleLocation::Choices> {
    using E = ksp::Expansions::Missions::ParamChoices_VesselSimpleLocation::Choices;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::landed, "landed", "#autoLOC_8000201"}, {E::orbit, "orbit", "#autoLOC_8000202"}};
};
