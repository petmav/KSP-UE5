// Unloaded-vessel snapshots of the save file (data side; the Load-into-live-Vessel/Part paths are WP-13/WP-14):
// ProtoVessel (ProtoVessel.cs), ProtoPartSnapshot (ProtoPartSnapshot.cs), ProtoPartModuleSnapshot
// (ProtoPartModuleSnapshot.cs), ProtoPartResourceSnapshot (ProtoPartResourceSnapshot.cs), AttachNodeSnapshot
// (AttachNodeSnapshot.cs), VesselNaming (VesselNaming.cs), ProtoTargetInfo (ProtoTargetInfo.cs), ProtoWaypointInfo
// (ProtoWaypointInfo.cs). The orbit block is WP-07's OrbitSnapshot.
//
// Load mirrors the C# constructors field by field (switch over the node's values in file order, unknown values kept
// as KSPParseable), Save writes KSP's fixed order. Things KSP rewrites on every save (see STATUS.md WP-04):
//  * ProtoPartSnapshot: unknown part values are written back with KSPParseable.Save() ("value, TYPE"), so a value
//    without a ", TYPE" suffix gains ", UNKNOWN"; "connected" is read and dropped; integer crew indices of old saves
//    become names; PreFailState only exists when MakingHistory is installed; cData only when non-empty.
//  * ProtoVessel: unknown vessel values are written back as KSPParseable.value (only the text before the first ',');
//    "cln"/"clnRsn" only when autoClean; TARGET only with data, WAYPOINT only with a navigation id; ACTIONGROUPS /
//    DISCOVERY / FLIGHTPLAN / CTRLSTATE / VESSELMODULES always (a missing ACTIONGROUPS node makes Save throw
//    NullReferenceException, as in KSP).
//  * persistentIds are checked against the FlightGlobals registry (SaveEnvironment::persistentIds): a duplicate id is
//    replaced by a new random one.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/config/kspparseable.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/orbit/orbit_snapshot.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Game;
class ProtoVessel;
class Vessel;
class Part;
class PartModule;
struct PartResource;
class PartResourceDefinition;
struct AttachNode;
struct AvailablePart;
class ITargetable;
class FlightState;

// Dictionary<string, KSPParseable> (insertion order; Add of a duplicate key throws ArgumentException)
using KSPParseableDictionary = dotnet::Dictionary<std::string, KSPParseable>;

// ProtoTargetInfo.cs
class ProtoTargetInfo {
public:
    using Type = ProtoTargetInfo_Type;
    Type targetType = Type::Null;
    uint32_t partUID = 0;
    Guid vesselId;
    std::optional<std::string> uniqueName;
    int partModuleIndex = 0;

    ProtoTargetInfo() = default;                     // ProtoTargetInfo.cs:68
    explicit ProtoTargetInfo(const ConfigNode& node);  // :73
    explicit ProtoTargetInfo(ITargetable* tgt);        // :44 (WP-14: vessel / part module / generic targets; implemented in vessel/vessel_transitions.cpp)
    ITargetable* FindTarget() const;                   // :143 (WP-14; vessels and part modules; bodies are not ITargetable in kspcore)
    void Load(const ConfigNode& node);               // :79
    void Save(ConfigNode& node) const;               // :103
};

// ProtoWaypointInfo.cs
class ProtoWaypointInfo {
public:
    using Type = ProtoWaypointInfo_Type;
    std::optional<std::string> name;
    Guid navigationId;
    Type waypointType = Type::Null;  // private in C#

    ProtoWaypointInfo() = default;                       // ProtoWaypointInfo.cs:38
    explicit ProtoWaypointInfo(const ConfigNode& node);  // :43
    void Load(const ConfigNode& node);                   // :49
    void Save(ConfigNode& node) const;                   // :61
};

// AttachNodeSnapshot.cs (data side; the AttachNode constructor needs live parts)
class AttachNodeSnapshot {
public:
    std::string id;
    int partIdx = -1;
    std::optional<std::string> srfAttachMeshName;
    // AttachNodeSnapshot.cs:33 ("id, partIdx[, mesh]"; int.Parse; the mesh is NOT trimmed)
    explicit AttachNodeSnapshot(std::string_view attachNodeString);
    AttachNodeSnapshot() = default;
    // AttachNodeSnapshot.cs:48 ("id, partIdx" + ",mesh" when the mesh is not empty)
    std::string Save() const;
    // AttachNodeSnapshot.cs:7 AttachNodeSnapshot(AttachNode, ProtoVessel): partIdx = the node's attachedPart in the vessel's
    // parts (WP-13; `parts` = pVesselRef.vesselRef.parts)
    AttachNodeSnapshot(const AttachNode& node, const std::vector<Part*>& vesselParts);
    // AttachNodeSnapshot.cs:61 findAttachedPart(vessel): null for partIdx -1 (IndexOutOfRange past the list)
    Part* findAttachedPart(const std::vector<Part*>& vesselParts) const;
};

// VesselNaming.cs (data side)
class VesselNaming {
public:
    std::optional<std::string> vesselName;
    VesselType vesselType = VesselType::Ship;
    int namingPriority = 0;
    VesselNaming() = default;                       // VesselNaming.cs:13
    explicit VesselNaming(const ConfigNode& node);  // :18 (field default Debris, then TryGetEnum default Ship)
    void Load(const ConfigNode& node);              // :23
    void Save(ConfigNode& node) const;              // :30
};

// ProtoPartResourceSnapshot.cs (data side; `definition` needs PartResourceLibrary)
class ProtoPartResourceSnapshot {
public:
    std::optional<std::string> resourceName;
    double amount = 0.0;
    double maxAmount = 0.0;
    bool flowState = false;
    PartResource* resourceRef = nullptr;

    explicit ProtoPartResourceSnapshot(const ConfigNode& node);  // ProtoPartResourceSnapshot.cs:30
    explicit ProtoPartResourceSnapshot(PartResource& resource);  // :14 (WP-13: from a live resource)
    void Load(Part& hostPart);                                   // :73 (WP-13: hostPart.SetResource)
    const PartResourceDefinition* definition = nullptr;          // :11 (PartResourceLibrary.GetDefinition(resourceName))
    void Save(ConfigNode& node) const;                           // :55
    void UpdateConfigNodeAmounts();                              // :63
    const ConfigNode& resourceValues() const { return *resourceValues_; }
    ConfigNode& resourceValues() { return *resourceValues_; }

private:
    ConfigNode::NodePtr resourceValues_;
};

// ProtoPartModuleSnapshot.cs (data side; the PartModule constructor and Load need the module host, WP-06)
class ProtoPartModuleSnapshot {
public:
    ConfigNode::NodePtr moduleValues;
    std::optional<std::string> moduleName;
    PartModule* moduleRef = nullptr;

    explicit ProtoPartModuleSnapshot(const ConfigNode& node);  // ProtoPartModuleSnapshot.cs:34
    explicit ProtoPartModuleSnapshot(PartModule& module);      // :14 (WP-13: from a live module)
    PartModule* Load(Part& hostPart, int& moduleIndex);        // :55 (WP-13: hostPart.LoadModule)
    void Save(ConfigNode& node);                               // :48
    // ProtoPartModuleSnapshot.cs:156 / 197 (MakingHistory adjuster bookkeeping on the module node)
    void RemovePartModuleAdjuster(const Guid& removeAdjusterID);
    void ProtoPartModuleRepair();

private:
    void EnsureModuleValuesIsInitialized();  // :67
    bool hasSaved_ = false;
};

// ProtoPartSnapshot.cs (data side)
class ProtoPartSnapshot {
public:
    using PCM = std::shared_ptr<ProtoCrewMember>;

    KSPParseableDictionary partStateValues;
    std::optional<std::string> partName;
    uint32_t craftID = 0;
    uint32_t flightID = 0;
    uint32_t missionID = 0;
    uint32_t launchID = 0;
    uint32_t persistentId = 0;
    bool sameVesselCollision = false;
    Vector3d position;
    Quaternionf rotation;  // default(Quaternion): (0,0,0,0)
    Vector3f mirror;
    SymmetryMethod symMethod = SymmetryMethod::Radial;
    bool partInfoFound = false;  // PartLoader.getPartInfoByName(partName) != null (SaveEnvironment::partExists)
    const AvailablePart* partInfo = nullptr;  // PartLoader.getPartInfoByName(partName) (SaveEnvironment::getPartInfoByName; WP-13)
    Part* partRef = nullptr;                  // ProtoPartSnapshot.partRef (WP-13)
    std::vector<ProtoPartSnapshot*> symLinks; // :46 (WP-13)
    ProtoPartSnapshot* parent = nullptr;
    int parentIdx = 0;
    std::vector<int> symLinkIdxs;
    std::vector<AttachNodeSnapshot> attachNodes;
    std::optional<AttachNodeSnapshot> srfAttachNode;
    int stageIndex = 0;
    int inverseStageIndex = 0;
    int resourcePriorityOffset = 0;
    int defaultInverseStage = 0;
    int seqOverride = 0;
    int inStageIndex = 0;
    int separationIndex = 0;
    std::string customPartData;
    int attachMode = 0;
    ProtoVessel* pVesselRef = nullptr;
    std::vector<PCM> protoModuleCrew;
    std::vector<int> protoCrewIndicesBackup;
    std::vector<std::string> protoCrewNames;
    float mass = 0.0f;
    double temperature = 0.0;
    double skinTemperature = 0.0;
    double skinUnexposedTemperature = 0.0;
    double staticPressureAtm = 0.0;
    float explosionPotential = 0.0f;
    int state = 0;
    int PreFailState = 0;
    bool shielded = false;
    bool attached = false;
    Part_AutoStrutMode autostrutMode = Part_AutoStrutMode::Off;
    bool rigidAttachment = false;
    std::vector<std::unique_ptr<ProtoPartModuleSnapshot>> modules;
    std::vector<std::unique_ptr<ProtoPartResourceSnapshot>> resources;
    std::vector<std::string> partRendererBoundsIgnore;
    ConfigNode::NodePtr partEvents;
    ConfigNode::NodePtr partActions;
    ConfigNode::NodePtr partEffects;
    ConfigNode::NodePtr partData;
    std::string flagURL;
    std::string refTransformName;
    float moduleCosts = 0.0f;
    float moduleMass = 0.0f;
    std::string moduleVariantName;
    int moduleCargoStackableQuantity = 1;
    std::unique_ptr<VesselNaming> vesselNaming;

    // ProtoPartSnapshot.cs:248 (st = the Game whose roster resolves crew names; nullptr: crew not resolved)
    ProtoPartSnapshot(const ConfigNode& node, ProtoVessel* protoVessel, Game* st);
    ProtoPartSnapshot(const ProtoPartSnapshot&) = delete;
    ProtoPartSnapshot& operator=(const ProtoPartSnapshot&) = delete;
    // ProtoPartSnapshot.cs:525
    void Save(ConfigNode& node);

    // ---- WP-13: the live-Part paths (ProtoPartSnapshot.cs:144, 499, 651, 761); the Vessel-level Load / Init are WP-14 / WP-19
    ProtoPartSnapshot(Part& partRef, ProtoVessel* protoVessel, bool preCreate = false);  // :144 / :149
    void storePartRefs();                       // :499 (parent / symLinks / attach node snapshots from the part tree)
    std::unique_ptr<Part> CreatePart();         // :761 (Instantiate the prefab, then ConfigurePart; nullptr without partInfo)
    void ConfigurePart(Vessel* vesselRef, Part& partToConfigure, const AvailablePart& PartInfo, bool setPosition);  // :651
    // :795 Load(Vessel, loadAsRootPart): Instantiate the part's prefab, link it under vesselParts[parentIdx] (not for the root) and
    // ConfigurePart it. The merge of the root part into the Vessel component and the EVA suit lookup are Unity's / WP-14's; the
    // vessel transform is the identity. The caller owns the returned part.
    std::unique_ptr<Part> Load(Vessel* vesselRef, const std::vector<Part*>& vesselParts, bool loadAsRootPart);
    // :1065 Init(Vessel): symmetry counterparts, srfAttachNode and attachNodes links from vesselParts
    void Init(const std::vector<Part*>& vesselParts);
    bool HasCrew(const std::string& name) const;   // :1131
    PCM GetCrew(const std::string& name) const;    // :1151
    PCM GetCrew(int index) const;                  // :1171
    int GetCrewIndex(const std::string& name) const;  // :1176
    void RemoveCrew(const std::string& name);      // :1196
    void RemoveCrew(int index);                    // :1217
    void RemoveCrew(const PCM& pcm);               // :1223
    ProtoPartModuleSnapshot* FindModule(const std::string& moduleName) const;  // :1244
};

// ProtoVessel.cs (data side)
class ProtoVessel {
public:
    using PCM = std::shared_ptr<ProtoCrewMember>;

    std::vector<std::unique_ptr<ProtoPartSnapshot>> protoPartSnapshots;
    ConfigNode::NodePtr vesselModules;
    KSPParseableDictionary vesselStateValues;
    std::optional<OrbitSnapshot> orbitSnapShot;
    Guid vesselID;
    uint32_t persistentId = 0;
    bool skipGroundPositioning = false;
    bool skipGroundPositioningForDroppedPart = false;
    bool vesselSpawning = false;
    std::string launchedFrom = "";
    bool landed = false;
    std::string landedAt = "";
    std::string displaylandedAt = "";
    bool splashed = false;
    std::string vesselName = "Unnamed Vessel";
    int rootIndex = 0;
    int stage = 0;
    double distanceTraveled = 0.0;
    uint32_t refTransform = 0;
    Vector3d position;
    double altitude = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    float height = 0.0f;
    Vector3f normal;
    Quaternionf rotation;  // default(Quaternion): (0,0,0,0)
    Vector3f CoM;
    Vessel* vesselRef = nullptr;
    VesselSituations situation = static_cast<VesselSituations>(0);
    VesselType vesselType = VesselType::Debris;
    double missionTime = 0.0;
    double launchTime = 0.0;
    double lastUT = -1.0;
    bool autoClean = false;
    std::string autoCleanReason = "";
    bool persistent = false;
    int GroupOverride = 0;
    std::vector<bool> OverrideDefault;
    std::vector<KSPActionGroup> OverrideActionControl;
    std::vector<KSPAxisGroup> OverrideAxisControl;
    std::vector<std::optional<std::string>> OverrideGroupNames;
    ConfigNode::NodePtr actionGroups;
    ConfigNode::NodePtr discoveryInfo;
    ConfigNode::NodePtr flightPlan;
    ConfigNode::NodePtr ctrlState;
    ProtoTargetInfo targetInfo;
    AltimeterDisplayState altimeterDisplayState = AltimeterDisplayState::DEFAULT;
    ProtoWaypointInfo waypointInfo;
    bool wasControllable = false;
    int crewedParts = 0;
    int crewableParts = 0;
    int PQSminLevel = 0;
    int PQSmaxLevel = 0;

    // Vessel.NumOverrideGroups (Vessel.cs:468)
    static inline int NumOverrideGroups = 4;

    // ProtoVessel.cs:254 (st = the Game, may be nullptr)
    ProtoVessel(const ConfigNode& node, Game* st);
    // ProtoVessel.cs:149 / :153 (WP-14, vessel/vessel_transitions.cpp): the snapshot of a live Vessel; sets vesselRef.protoVessel
    explicit ProtoVessel(Vessel& VesselRef);
    ProtoVessel(Vessel& VesselRef, bool preCreate);
    // ProtoVessel.cs:633 / :638 (WP-14): creates the unloaded Vessel (vessel == nullptr: the caller owns the result) or fills `vessel`
    std::unique_ptr<Vessel> Load(FlightState* st, Vessel* vessel = nullptr);
    void ResetProtoPartSnapShots();  // :615
    ProtoVessel(const ProtoVessel&) = delete;
    ProtoVessel& operator=(const ProtoVessel&) = delete;
    // ProtoVessel.cs:513
    void Save(ConfigNode& node);
    // ProtoVessel.cs:597 (no live vessel: only creates the node when it is null)
    void SaveVesselModules();
    void SaveLiveVesselModules();  // WP-14: ProtoVessel.cs:604-611 (vesselRef != null)

    // ProtoVessel.cs:821 LoadObjects (WP-13): the parts of the snapshots, root = protoPartSnapshots[rootIndex]; a snapshot whose
    // part is unknown is skipped (its index shifts the later ones, as in C#). The Vessel (WP-14) takes the parts over.
    struct LoadedObjects {
        std::vector<std::unique_ptr<Part>> parts;
        Part* rootPart = nullptr;
    };
    LoadedObjects LoadObjects(Vessel* vesselRef);

    const std::vector<PCM>& GetVesselCrew();  // ProtoVessel.cs:465
    void RebuildCrewCounts();                 // :471
    void AddCrew(const PCM& pcm);             // :496
    bool RemoveCrew(const PCM& pcm);          // :502
    std::string GetDisplayName() const;       // :508
    std::vector<ProtoPartSnapshot*> GetAllProtoPartsIncludingCargo();  // :1061 (cargo snapshots owned by this vessel)
    std::vector<ProtoPartSnapshot*> GetAllProtoPartsFromCrew();         // :1121

    // ProtoVessel.cs:968 CreateOrbitNode (REF = FlightGlobals.Bodies.IndexOf(orbit.referenceBody))
    static ConfigNode::NodePtr CreateOrbitNode(const Orbit& orbit, const std::vector<CelestialBody*>& bodies);

private:
    void verifyCrew();  // ProtoVessel.cs:453
    std::vector<PCM> crew_;
    std::vector<std::unique_ptr<ProtoPartSnapshot>> cargoSnapshots_;  // keeps GetAllProtoParts* results alive
};

}  // namespace ksp
