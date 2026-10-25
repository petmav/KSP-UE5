// WP-38 slice 3: the pre-built craft data model of the contract system (Assembly-CSharp/FinePrint/Utilities/{PreBuiltCraft,PreBuiltCraftDefinition,
// PreBuiltCraftPosition,ConstructionPart,ConstructionParts,ConstructionExtensions}.cs), SystemUtilities.ValidateLoadCraftNode and the validation half of
// KSP.UI.Screens.CraftProfileInfo.LoadDetailsFromCraftFile.
//
// Engine-agnostic subset: the craft files are read as ConfigNodes and validated against the part and module classes of the process (PartLoader /
// AssemblyLoader.GetClassByName); ShipTemplate.LoadShip (part counts, mass, cost, size, stage count) and the .loadmeta cache of CraftProfileInfo are not ported.
// Spawning the vessel (ContractSystem.SpawnPreBuiltShip -> ShipConstruction.LoadShip + ProtoVessel) is ContractsEnvironment::spawnPreBuiltShip.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {
struct AvailablePart;
class CelestialBody;
}

namespace ksp::Contracts::Lite {

// KSP.UI.Screens.CraftProfileInfo (the fields LoadDetailsFromCraftFile fills for the contract validation)
class CraftProfileInfo {
public:
    std::string shipName;                                                  // ShipTemplate.shipName = root.GetValue("ship")
    std::string description;                                               // :15
    std::string version;
    std::string saveMD5;                                                   // :21
    int64_t lastWriteTime = 0;                                             // :23
    int partCount = 0;                                                     // :25 (ShipTemplate.LoadShip: the node count, 0 when a PART node has no `part`)
    int stageCount = 0;                                                    // :27
    float totalCost = 0.0f;                                                // :29
    float totalMass = 0.0f;                                                // :31
    Vector3f shipSize{};                                                   // :33
    EditorFacility shipFacility = EditorFacility::None;                    // :35 (the C# field default is the enum's 0 = None)
    uint64_t steamPublishedFileId = 0;                                     // :37
    bool shipPartsExperimental = false;                                    // :43
    bool duplicatedParts = false;                                          // :49
    VersionCompareResult compatibility = VersionCompareResult::INVALID;    // :17
    std::vector<std::string> partNames;
    std::vector<std::string> partModules;
    std::vector<std::string> UnavailableShipParts;                         // :39
    std::vector<std::string> UnavailableShipPartModules;                   // :41
    std::vector<std::string> BrokenShipParts;                              // :47
    // CraftProfileInfo.cs:76 (internal LoadDetailsFromCraftFile(root, filePath, brokenParts, bypassTechCheck)): the validation half
    CraftProfileInfo& LoadDetailsFromCraftFile(const ConfigNode& root, const std::string& filePath, bool brokenParts, bool bypassTechCheck);
    bool CraftHasErrors() const;                                           // :393
    std::string GetErrorMessage() const;                                   // :375
    bool shipPartsUnlocked() const { return UnavailableShipParts.empty() && !duplicatedParts; }  // :57
    bool shipPartModulesAvailable() const { return UnavailableShipPartModules.empty(); }          // :69
    void Load(const ConfigNode& node);                                     // :172
    void Save(ConfigNode& node) const;                                     // :223
    // (LoadFromMetaFile / SaveToMetaFile / LoadMetaPath / the craft browser's cached load, CraftProfileInfo.cs:236-373, are file-cache UI code: not ported)
};

// PreBuiltCraftPosition.cs:3
struct PreBuiltCraftPosition {
    float CenterLatitude = 3.4028234663852886e38f;                         // :5 float.MaxValue
    float CenterLongitude = 3.4028234663852886e38f;                        // :7
    float SearchRadius = 3.4028234663852886e38f;                           // :9
};

// PreBuiltCraftDefinition.cs:7
class PreBuiltCraftDefinition {
public:
    std::string craftURL;                                                  // :10
    ConfigNode::NodePtr craftNode;                                         // :12
    CraftProfileInfo craftInfo;                                            // :14
    std::vector<std::string> contractTypes;                                // :16
    bool allowGround = true;                                               // :18
    bool allowOrbit = false;                                               // :20
    bool allowWater = false;                                               // :22
    bool usePreBuiltPositions = true;                                      // :24
    std::vector<std::string>& brokenPartNames() { return craftInfo.BrokenShipParts; }   // :26
    bool ContainsBrokenParts() const { return !craftInfo.BrokenShipParts.empty(); }     // :28
    PreBuiltCraftDefinition(const std::string& url, ConfigNode::NodePtr node);          // :30
};

// PreBuiltCraft.cs:8
class PreBuiltCraft {
public:
    using List = std::vector<std::shared_ptr<PreBuiltCraftDefinition>>;
    explicit PreBuiltCraft(List preBuiltDefinitions);                      // :15
    // :22 the list is cached per contract type and handed out by reference (the contracts remove entries from it: sic, the cache changes)
    List& CraftForContractType(const std::string& contractType);
    std::shared_ptr<PreBuiltCraftDefinition> CraftByURL(std::string url);  // :39

private:
    List preBuiltDefinitions_;
    std::map<std::string, List> cachedLists_;
};

// ConstructionPart.cs:5
struct ConstructionPart {
    std::string partName;                                                  // :7
    const AvailablePart* availablePart = nullptr;                          // :9
    std::vector<std::string> contractTypes;                                // :11
    ConstructionPart(const std::string& name, const AvailablePart* aPart, std::vector<std::string> contractTypes_);  // :13
};

// ConstructionParts.cs:7
class ConstructionParts {
public:
    using List = std::vector<std::shared_ptr<ConstructionPart>>;
    explicit ConstructionParts(List constructionPartDefinitions);          // :15
    List& PartsForContractType(const std::string& contractType);           // :21 (cached by reference, as PreBuiltCraft)

private:
    List constructionPartDefinitions_;
    std::map<std::string, List> cachedLists_;
};

// ConstructionExtensions.cs:5 ContainsPart(this List<ConstructionPart>, partName)
bool ContainsPart(const ConstructionParts::List& list, const std::string& partName);

namespace SystemUtilities {
// SystemUtilities.cs:79 / :84: nullptr when the file is missing or the craft is invalid
std::shared_ptr<PreBuiltCraftDefinition> ValidateLoadCraftNode(const std::string& url, const std::string& className, bool brokenParts);
std::shared_ptr<PreBuiltCraftDefinition> ValidateLoadCraftNode(const std::string& url, const std::string& className, bool brokenParts, bool bypassTechCheck);
}  // namespace SystemUtilities

// Expansions.Missions.VesselSituation (the members the three contracts fill) handed to PreBuiltEnvironment::spawnPreBuiltShip
struct PreBuiltVesselSituation {
    enum class StartSituation : int { LANDED, ORBITING };   // MissionSituation.VesselStartSituations
    CelestialBody* targetBody = nullptr;                    // VesselGroundLocation.targetBody
    double latitude = 0.0;                                  // VesselGroundLocation.latitude
    double longitude = 0.0;                                 // VesselGroundLocation.longitude
    bool hasOrbit = false;                                  // orbitSnapShot = new MissionOrbit(orbit)
    Orbit orbit;                                            //   (the generated orbit; for a landed craft the body itself)
    StartSituation situation = StartSituation::LANDED;      // VesselLocation.situation
    bool splashed = false;                                  // VesselGroundLocation.splashed (the C# default of a new VesselGroundLocation)
    bool brakesOn = true;                                   // VesselLocation.brakesOn (VesselLocation() sets true)
    Quaternionf rotation = Quaternionf::identity();         // VesselGroundLocation.rotation.quaternion (new MissionQuaternion: Euler(0, 0, 0))
    bool playerCreated = false;                             // :playerCreated
    std::string craftFile;                                  // :craftFile
    bool focusOnSpawn = false;                              // :focusonSpawn
    std::string vesselName;                                 // :vesselName
};

// Host seams of the pre-built craft loading
struct PreBuiltEnvironment {
    // File.Exists(path) / ConfigNode.Load(path). Unset: the file system as is (case sensitive on Linux; the Squad cfgs write ".../PreBuiltCraft/..." for the folder
    // "PrebuiltCraft", which a case sensitive file system does not find).
    static inline std::function<bool(const std::string& path)> fileExists;
    static inline std::function<ConfigNode::NodePtr(const std::string& path)> loadConfigNode;
    // ResearchAndDevelopment.PartTechAvailable (only read when bypassTechCheck is false). Unset: the part's tech is available.
    static inline std::function<bool(const AvailablePart*)> partTechAvailable;
    // ContractSystem.SpawnPreBuiltShip(craftDef, vesselSituation) (ConstructShip: ShipConstruction.LoadShip + ProtoVessel.Create + FlightGlobals.AddVessel).
    // Unset: nothing is spawned.
    static inline std::function<void(PreBuiltCraftDefinition&, const PreBuiltVesselSituation&)> spawnPreBuiltShip;
};

}  // namespace ksp::Contracts::Lite
