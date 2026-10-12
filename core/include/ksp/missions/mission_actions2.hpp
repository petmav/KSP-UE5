// WP-54 slice 2: the create actions WP-55 deferred (Expansions/Missions/Actions/ActionCreate{Vessel,Kerbal,Asteroid,Comet,Flag,LaunchSite}.cs).
// Conventions: see node_library.hpp. The proto-vessel node builders (ProtoVessel.CreateVesselNode / CreatePartNode / CreateDiscoveryNode),
// HighLogic.CurrentGame.AddVessel, the PQS surface height and the comet definition are the seams of ActionCreateEnvironment; the Unity
// Quaternion natives go through the functions of quaternion_native.hpp. ActionPartFailure / ActionPartRepair / ActionPartResourceDrain
// and the scene body of ActionCreateVessel.Fire (MissionSystem.ConstructShip ...) are not ported.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/missions/mission_data.hpp"
#include "ksp/missions/mission_node.hpp"
#include "ksp/missions/node_library.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {
class ProtoVessel;
}

namespace ksp::Expansions::Missions::Actions {
class ActionCreateVessel;
}

namespace ksp::Expansions::Missions {

struct ActionCreateEnvironment {
    using PCM = std::shared_ptr<ProtoCrewMember>;
    // ProtoVessel.CreateVesselNode(name, type, orbit, rootPartIndex, partNodes, additionalNodes). Unset: ProtoVesselNodes::CreateVesselNode with the
    // additional nodes appended to the part nodes (the same node order).
    static inline std::function<ConfigNode::NodePtr(const std::string& name, VesselType, const Orbit&, int rootPartIndex,
                                                    const std::vector<ConfigNode::NodePtr>& partNodes, const std::vector<ConfigNode::NodePtr>& additionalNodes)> createVesselNode;
    // ProtoVessel.CreatePartNode(partName, id, crew...). Unset: throws (the part database is the host's).
    static inline std::function<ConfigNode::NodePtr(const std::string& partName, uint32_t id, const std::vector<PCM>& crew)> createPartNode;
    // ProtoVessel.CreateDiscoveryNode(level, size, lifeTime, maxLifeTime). Unset: a DISCOVERY node with those four numbers.
    static inline std::function<ConfigNode::NodePtr(DiscoveryLevels, UntrackedObjectClass, double, double)> createDiscoveryNode;
    // HighLogic.CurrentGame.AddVessel(node) -> protoVessel.vesselRef. Unset: the node is recorded in addedVesselNodes and null is returned.
    static inline std::function<Vessel*(const ConfigNode&)> addVessel;
    static inline std::vector<ConfigNode::NodePtr> addedVesselNodes;
    // pqsController != null / pqsController.GetSurfaceHeight(radialVector, overrideQuadBuildCheck: true). Unset: body->pqsController.
    static inline std::function<bool(CelestialBody*)> hasPQS;
    static inline std::function<double(CelestialBody*, const Vector3d&)> surfaceHeight;
    // ShipConstruction.GetUniqueFlightID(HighLogic.CurrentGame.flightState). Unset: ShipConstruction::GetUniqueFlightID(flightState).
    static inline std::function<uint32_t()> uniqueFlightID;
    // Orbit.CreateRandomOrbitAround(body). Unset: Orbit::CreateRandomOrbitAround.
    static inline std::function<Orbit(CelestialBody*)> createRandomOrbitAround;
    // The Unity native quaternion operations. Unset: quaternion_native.hpp.
    static inline std::function<Quaternionf(const Vector3f&, const Vector3f&)> fromToRotation;
    static inline std::function<Quaternionf(float, const Vector3f&)> angleAxis;
    static inline std::function<Quaternionf(const Vector3f&, const Vector3f&)> lookRotation;
    // CometManager.GenerateDefinition(GetCometOrbitType(type), class, seed).CreateVesselNode(optimizedCollider: false, 0, hasName: true) (the
    // VESSELMODULES child of ActionCreateComet). Unset: an empty "COMET" node.
    static inline std::function<ConfigNode::NodePtr(const std::string& cometType, UntrackedObjectClass, int seed)> cometVesselNode;
    // GameEvents.onAsteroidSpawned / onCometSpawned fire with protoVessel.vesselRef (null without a scene).
    // PSystemSetup.Instance.GetSpaceCenterFacility(name).facilityTransform.position or GetLaunchSite(name).GetWorldPos().position (ActionCreateVessel.ActionLocation)
    static inline std::function<std::optional<Vector3f>(const std::string& launchSite)> launchSiteWorldPosition;
    // MissionSystem.Instance.ConstructShip(...) etc. (ActionCreateVessel.Fire craft branch): called once, synchronously, after the 1 s wait
    static inline std::function<void(Actions::ActionCreateVessel&)> spawnCraftVessel;
    // CreatePlaceholderCraft (file / ShipConstruct work)
    static inline std::function<void(Actions::ActionCreateVessel&)> createPlaceholderCraft;
    // MissionEditorLogic: the incompatible craft of the vessel situation (GetNodeBodyParameterString), HighLogic.LoadedSceneIsMissionBuilder
    static inline std::function<bool(VesselSituation*)> craftIsIncompatible;
    static inline std::function<bool()> loadedSceneIsMissionBuilder;
    // RunValidation's craft checks (MissionEditorLogic.LoadVessel, NoControlSources, MissionCheckLaunchClamps): receives the action and the validator
    static inline std::function<void(Actions::ActionCreateVessel&, Editor::MissionEditorValidator*)> validateCraft;
    // GameSettings.EVA_DEFAULT_HELMET_ON / EVA_DEFAULT_NECKRING_ON (game_settings.hpp redeclares Editor::ValidatorMode, so it cannot be included next to
    // mission_types.hpp). Unset: false.
    static inline std::function<bool()> evaDefaultHelmetOn;
    static inline std::function<bool()> evaDefaultNeckRingOn;
    // MissionsApp.UpdateLauncherButtonPlayAnim()
    static inline std::function<void()> updateLauncherButtonPlayAnim;
    static void Reset();
};

}  // namespace ksp::Expansions::Missions

namespace ksp::Expansions::Missions::Actions {

// ActionCreateVessel.cs:15
class ActionCreateVessel : public ActionModule, public INodeOrbit, public IActionCreateVessel {
public:
    static constexpr const char* kTypeName = "ActionCreateVessel";
    ActionCreateVessel();                                    // :23 Awake
    std::unique_ptr<VesselSituation> vesselSituation;        // :19
    bool screenMessageActive = false;                        // activeMessage != null && activeMessage.textInstance != null
    VesselSituation* VesselToBuild() { return vesselSituation.get(); }  // :21
    VesselSituation* GetVesselSituation() override { return vesselSituation.get(); }
    void Initialize(MENode* node) override;                  // :29
    Enumerator Fire() override;                              // :35
    void CreateVesselScreenMessage();                        // :148
    Vector3f ActionLocation() override;                      // :165
    void OnCloned(ActionModule*& actionModuleBase) override;  // :199
    bool HasNodeOrbit() override { return true; }            // :206
    Orbit* GetNodeOrbit() override;                          // :211
    void CreatePlaceholderCraft();                           // :228
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :262
    std::string GetInfo() override;                          // :349
    void RunValidation(Editor::MissionEditorValidator* validator) override;  // :354
    void Save(ConfigNode& node) const override;              // :509
    void Load(const ConfigNode& node) override;              // :515
};

// ActionCreateKerbal.cs:11
class ActionCreateKerbal : public ActionModule, public INodeOrbit, public IMissionKerbal, public IActionCreateKerbal {
public:
    static constexpr const char* kTypeName = "ActionCreateKerbal";
    ActionCreateKerbal();                                    // :44 Awake
    std::unique_ptr<MissionKerbal> missionKerbal;            // :14
    bool isHelmetEnabled = false;                            // :17 = GameSettings.EVA_DEFAULT_HELMET_ON
    bool isNeckRingEnabled = false;                          // :20 = GameSettings.EVA_DEFAULT_NECKRING_ON
    bool isStranded = false;                                 // :23
    std::unique_ptr<ParamChoices_VesselSimpleLocation> location;  // :26
    uint32_t persistentId = 0;                               // :28
    uint32_t GetPersistentId() override { return persistentId; }
    void Initialize(MENode* node) override;                  // :57
    Enumerator Fire() override;                              // :63
    ConfigNode::NodePtr SpawnKerbal(bool addtoCurrentGame = true);  // :93
    ConfigNode::NodePtr GetProtoVesselNode();                // :135
    ConfigNode::NodePtr GetProtoVesselNode(float feetToPivotDistance);  // :140
    Vector3f ActionLocation() override;                      // :249
    void OnCloned(ActionModule*& actionModuleBase) override;  // :263
    bool HasNodeOrbit() override { return true; }            // :270
    Orbit* GetNodeOrbit() override;                          // :275
    std::string GetInfo() override;                          // :313
    void NodeDeleted() override;                             // :318
    void KerbalRosterStatusChange(const std::shared_ptr<ProtoCrewMember>& kerbal, ProtoCrewMember::RosterStatus oldStatus, ProtoCrewMember::RosterStatus newStatus);  // :326
    void KerbalAdded(const std::shared_ptr<ProtoCrewMember>& kerbal);   // :334
    void KerbalTypeChange(const std::shared_ptr<ProtoCrewMember>& kerbal, ProtoCrewMember::KerbalType oldType, ProtoCrewMember::KerbalType newType);  // :342
    void KerbalNameChange(const std::shared_ptr<ProtoCrewMember>& kerbal, const std::string& oldName, const std::string& newName);  // :350
    void KerbalRemoved(const std::shared_ptr<ProtoCrewMember>& kerbal);  // :358
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :366
    void Save(ConfigNode& node) const override;              // :380
    void Load(const ConfigNode& node) override;              // :392

private:
    ConfigNode::NodePtr CreateKerbalPartNode(uint32_t id, const std::shared_ptr<ProtoCrewMember>& crew);  // :230
    std::string kerbalName;                                  // :30
    double TimeDeadline = 1.7976931348623157e308;            // :32
    float kerbalRotationOffset = -90.0f;                     // :34
    float defaultKerbalFeetToPivotDist = 0.25f;              // :38
};

// ActionCreateAsteroid.cs:10
class ActionCreateAsteroid : public ActionModule, public INodeOrbit, public IActionCreateAsteroid {
public:
    static constexpr const char* kTypeName = "ActionCreateAsteroid";
    ActionCreateAsteroid();                                  // :21 Awake
    std::unique_ptr<Asteroid> asteroid;                      // :13
    std::unique_ptr<ParamChoices_VesselSimpleLocation> location;  // :16
    uint32_t PersistentID() const { return asteroid->persistentId; }  // :18
    uint32_t GetPersistentID() override { return asteroid->persistentId; }
    Asteroid* GetAsteroid() override { return asteroid.get(); }
    Enumerator Fire() override;                              // :31
    Vector3f ActionLocation() override;                      // :37
    void OnCloned(ActionModule*& actionModuleBase) override;  // :51
    bool HasNodeOrbit() override { return true; }            // :58
    Orbit* GetNodeOrbit() override;                          // :63
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :76
    std::string GetInfo() override;                          // :90
    void Save(ConfigNode& node) const override;              // :95
    void Load(const ConfigNode& node) override;              // :102
    void SpawnAsteroid();                                    // :109
    ConfigNode::NodePtr GetProtoVesselNode();                // :117
};

// ActionCreateComet.cs:10
class ActionCreateComet : public ActionModule, public INodeOrbit, public IActionCreateComet {
public:
    static constexpr const char* kTypeName = "ActionCreateComet";
    ActionCreateComet();                                     // :21 Awake
    std::unique_ptr<Comet> comet;                            // :13
    std::unique_ptr<ParamChoices_VesselSimpleLocation> location;  // :16
    uint32_t PersistentID() const { return comet->persistentId; }  // :18
    uint32_t GetPersistentID() override { return comet->persistentId; }
    Comet* GetComet() override { return comet.get(); }
    Enumerator Fire() override;                              // :31
    Vector3f ActionLocation() override;                      // :37
    void OnCloned(ActionModule*& actionModuleBase) override;  // :51
    bool HasNodeOrbit() override { return true; }            // :58
    Orbit* GetNodeOrbit() override;                          // :63
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :76
    std::string GetInfo() override;                          // :90
    void Save(ConfigNode& node) const override;              // :95
    void Load(const ConfigNode& node) override;              // :102
    void SpawnComet();                                       // :109
    ConfigNode::NodePtr GetProtoVesselNode();                // :117
};

// ActionCreateFlag.cs:9
class ActionCreateFlag : public ActionModule, public IActionCreateFlag {
public:
    static constexpr const char* kTypeName = "ActionCreateFlag";
    ActionCreateFlag();                                      // :24 Awake
    std::string siteName;                                    // :12
    std::string plaqueText;                                  // :15
    std::unique_ptr<VesselGroundLocation> location;          // :18
    uint32_t persistentID = 0;                               // :22
    uint32_t GetPersistentID() override { return persistentID; }
    Enumerator Fire() override;                              // :33
    ConfigNode::NodePtr GetProtoVesselNode();                // :39
    Vector3f ActionLocation() override;                      // :81
    void OnCloned(ActionModule*& actionModuleBase) override;  // :86
    std::string GetInfo() override;                          // :92
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :97
    void Save(ConfigNode& node) const override;              // :107
    void Load(const ConfigNode& node) override;              // :116

private:
    ConfigNode::NodePtr CreateFlagPartNode(uint32_t id);     // :68
    float flagRotationOffset = -30.0f;                       // :20
};

// ActionCreateLaunchSite.cs:8
class ActionCreateLaunchSite : public ActionModule, public IActionCreateLaunchSite {
public:
    static constexpr const char* kTypeName = "ActionCreateLaunchSite";
    ActionCreateLaunchSite();                                // :14 Awake
    std::unique_ptr<LaunchSiteSituation> launchSiteSituation;  // :11
    std::string GetLaunchSiteName() override { return launchSiteSituation->launchSiteName; }
    void Initialize(MENode* node) override;                  // :20
    Enumerator Fire() override;                              // :26
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :35
    void RunValidation(Editor::MissionEditorValidator* validator) override;  // :52
    std::string GetInfo() override;                          // :86
    void Save(ConfigNode& node) const override;              // :91
    void Load(const ConfigNode& node) override;              // :97
};

}  // namespace ksp::Expansions::Missions::Actions

namespace ksp::Expansions::Missions {
// Registers the six create actions in ActionModuleRegistry (called by RegisterNodeLibrary()).
void RegisterNodeLibrarySlice2Actions();
}  // namespace ksp::Expansions::Missions
