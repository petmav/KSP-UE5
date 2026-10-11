// WP-53: Breaking Ground ROCs (the scannable / collectable surface features scattered by the PQS): ROCDefinition.cs, RocCBDefinition.cs, ROC.cs, ROCManager.cs,
// ROCScenario.cs, LandClassROC.cs, PQSROCControl.cs (Assembly-CSharp root).
//
// What is ported: the definition/config side (ROC_DEFINITION nodes with their CELESTIALBODY/Biome children), ROCManager (definitions, validation of
// body/biome combinations against the loaded bodies' biome maps, removed-ROC list shared with the save model, enable/disable by the game's ROCSeed,
// ROC stats), the per-quad spawn decisions of LandClassROC (seeding, count per quad, the deterministic placement sequence over UnityEngine.Random:
// positions, orientation, rocID) and PQSROCControl's per-quad biome/area logic.
//
// Seams (documented in RocEnvironment): the PQS quad mesh (vertices, normals, quad rotation/position, mesh bounds are inputs of `RocQuad`), the
// instantiated prefab (renderer bounds, GameObject name), AssetBase.GetPrefab, the PQS sphere flags, physics raycasts of ROC.GetClosestScanPositionWithRaycasts,
// FXMonger.ROCExplode, ROCEmitter / ROCsSFX (WP-48), ArrowPointer debug arrows (WP-43) and the PQSMod_ROCScatterQuad GameObject plumbing (WP-48).
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/util/collections.hpp"
#include "ksp/modules/scenario_runner.hpp"

namespace ksp {

class CelestialBody;
class Vessel;
class ModuleScienceExperiment;
namespace Expansions::Missions {
class Mission;
}
class LandClassROC;
class PQSROCControl;
class ROCManager;

// RocCBDefinition.cs:6
class RocCBDefinition {
public:
    std::string name;
    std::vector<std::string> biomes;
    RocCBDefinition() = default;                                                    // :12
    RocCBDefinition(std::string name_, std::vector<std::string> biomes_) : name(std::move(name_)), biomes(std::move(biomes_)) {}  // :16
    void Load(const ConfigNode& node);                                              // :23
};

// ROCDefinition.cs:6
class ROCDefinition {
public:
    std::string type, displayName, prefabName, modelName;
    bool orientateUp = false;
    float depth = 0.0f;
    bool canBeTaken = false;
    float frequency = 0.0f;
    bool castShadows = true, receiveShadows = true;
    float collisionThreshold = 0.0f;
    bool smallRoc = false, randomDepth = false, randomOrientation = false, randomRotation = false;
    float burstEmitterMinWait = 0.0f, burstEmitterMaxWait = 0.0f, sfxVolume = 0.0f;
    std::string idleClipPath, burstClipPath;
    std::vector<RocCBDefinition> myCelestialBodies;
    std::vector<Vector3f> localSpaceScanPoints;
    float scale = 1.0f;
    float vfxBaseForce = 0.0f;
    std::optional<FloatCurve> vfxCurveForce;  // null until VFX_CURVEFORCE has values
    bool applyForces = false;
    Vector2f vfxForceRadius;
    Vector3f forceDirection, radiusCenter;

    ROCDefinition() = default;                                   // :88 internal
    bool ContainsCBBiome(const std::string& cbName, const std::string& biomeName) const;  // :126
    void Load(const ConfigNode& node);                           // :145
};

// ROC.cs:7 (the MonoBehaviour's data and its engine-independent methods)
class ROC {
public:
    std::string type, displayName, prefabName, modelName;
    bool orientateup = false;
    float depth = 0.0f;
    bool canbetaken = false;
    float frequency = 0.0f;
    std::vector<RocCBDefinition> myCelestialBodies;
    bool castShadows = false, recieveShadows = false;
    float collisionThreshold = 0.0f;
    bool smallROC = false;
    int rocID = 0;
    bool randomDepth = false, randomOrientation = false, randomRotation = false;
    std::vector<Vector3f> localSpaceScanPoints;
    float burstEmitterMinWait = 60.0f, burstEmitterMaxWait = 300.0f;
    float scale = 1.0f;
    float sfxVolume = 0.0f;
    std::string idleClipPath, burstClipPath;
    Vector3f upDirection;
    float vfxBaseForce = 0.0f;
    std::optional<FloatCurve> vfxCurveForce;
    bool applyForces = false;
    Vector2f vfxForceRadius;
    Vector3f forceDirection, radiusCenter;
    PartTransform transform;       // base.transform
    std::string name;              // GameObject.name (Instantiate: prefab name + "(Clone)")
    PartTransform modelTransform;  // modelTransform (Transform.Find(modelName)); `modelTransformFound` = non-null
    bool modelTransformFound = true;
    bool destroyed = false;        // Object.Destroy(gameObject)
    bool started = false;

    // ROC.cs:128 Awake: Destroy(this) without Serenity; upDirection = transform.up when zero
    void Awake();
    // ROC.cs:140 Start: ROCManager stats (flight scene), GameEvents subscriptions of the debug arrows (UI) are not made
    void Start();
    // ROC.cs:165 SetStats
    void SetStats(const std::string& type, const std::string& displayName, const std::string& prefabName, const std::string& modelName, bool orientateup, float depth,
                  bool canbetaken, float frequency, const std::vector<RocCBDefinition>& myCelestialBodies, bool castShadows, bool recieveShadows,
                  float collisionthreshold, bool smallroc, bool randomdepth, bool randomorientation, const std::vector<Vector3f>& localSpaceScanPoints,
                  float burstEmitterMinWait, float burstEmitterMaxWait, bool randomRotation, float scale, float sfxVolume, const std::string& idleClipPath,
                  const std::string& burstClipPath, const std::optional<FloatCurve>& vfxCurveForce, float vfxBaseForce, bool applyForces,
                  const Vector2f& vfxForceRadius, const Vector3f& forceDirection, const Vector3f& radiusCenter);
    void OnDestroyed();                                          // (C++ addition) OnDestroy
    Vector3f GetClosestScanPosition(const Vector3f& testPosition, float& bestDistanceToPoint);  // :211
    // :238 (Physics.Raycast(testPosition, dir, out hit, distance, 32768, Ignore) && hit.collider.CompareTag("ROC") is `RocEnvironment::raycastHitsRoc`)
    Vector3f GetClosestScanPositionWithRaycasts(const Vector3f& testPosition, float& distanceToPoint);
    void CheckCollision(Vessel& vessel);                         // :273
    void PickUpROC();                                            // :287
    bool PerformExperiment(ModuleScienceExperiment* moduleScienceExperiment);  // :305
    float GetVFXForceScale(float burstClipTime) const;           // :426
    void UpdateModelTransform();                                 // :321 protected virtual
};

// Spawn-time quad description: the PQ fields LandClassROC / PQSROCControl read (the quad mesh is WP-12's)
struct RocQuad {
    std::string name;                         // quad.gameObject.name
    int subdivision = 0;
    std::vector<Vector3f> verts, vertNormals; // quad.verts / quad.vertNormals
    Quaternionf quadRotation = Quaternionf::identity();
    Vector3f positionPlanet;                  // quad.positionPlanet
    Vector3d quadTransformPosition;           // quad.quadTransform.position
    bool hasMesh = false;                     // MeshFilter.sharedMesh != null
    Vector3f meshBoundsSize;                  // sharedMesh.bounds.size
    double quadArea = 0.0;                    // quad.quadArea
    PartTransform transform;                  // PQSMod_ROCScatterQuad's transform (TransformDirection of the normal)
};

// PQSROCControl.RocPositionInfo (PQSROCControl.cs:10)
struct RocPositionInfo {
    std::string rocType;
    Vector3f position;
};

// List<RocPositionInfo> is a reference object in C# (shared by the dictionary and LandClassROC)
using RocPositionList = std::shared_ptr<std::vector<std::shared_ptr<RocPositionInfo>>>;

// The sphere (PQS) values LandClassROC reads
struct RocPqsView {
    int maxLevel = 0;                  // PQS.maxLevel
    bool surfaceRelativeQuads = false; // PQS.surfaceRelativeQuads
    bool isActive = false;             // PQS.isActive
    bool isStarted = false;            // PQS.isStarted
};

// PQS statics the spawn maths read (PQS.cacheSideVertCount / cacheRes / vi, PQS.Global_AllowScatter)
struct RocPqsStatics {
    static inline int cacheSideVertCount = 15;       // PQS.cs:387 (the PQS settings node `vertCount` overrides it at load; not in the GameData mirror)
    static int cacheRes() { return cacheSideVertCount - 1; }  // PQS.cs:399
    static int vi(int x, int z) { return z * cacheSideVertCount + x; }  // PQS.cs:1724
    static inline bool Global_AllowScatter = true;   // PQS.cs:147
};

// PQSMod_ROCScatterQuad (WP-48 owns the GameObject plumbing; this is its data: PQSMod_ROCScatterQuad.cs:4)
struct RocScatterQuad {
    bool active = false;       // rocObj.activeSelf
    bool isVisible = false, isBuilt = false;
    int quadSeed = 0;
    RocQuad* quad = nullptr;
    int count = 0;
    LandClassROC* roc = nullptr;
    std::string name = "Unassigned";
    int rocIDForRoc = 0;       // ROC.rocID of the component on the scatter object (PQSMod_ROCScatterQuad.Setup)
    void Setup(RocQuad* q, int quadSeed_, LandClassROC* roc_, int count_, int rocIDCounter);  // (C++ addition)
    void Destroy();                                                                           // (C++ addition)
    std::vector<int> spawnedRocIds;                    // the ROC objects instantiated under it (their rocID)
};

// What CreateScatterMesh instantiated (the engine places the prefab)
struct RocPlacement {
    int rocID = 0;
    Vector3f localPosition;
    Quaternionf localRotation = Quaternionf::identity();
    float scale = 1.0f;
    Vector3f upDirection;
    std::string rocType;
};

// Services of the engine / other packages
struct RocEnvironment {
    // AssetBase.GetPrefab(prefabName) / GetComponent<ROC>(). Unset: 2
    static inline std::function<int(const std::string& prefabName)> prefabState;  // 0 = no prefab, 1 = prefab without a ROC component, 2 = prefab with ROC
    // Object.Instantiate(prefab).name (Unity appends "(Clone)"); the instantiated prefab's renderer bounds (size.y, extents.y) for LandClassROC.
    // Unset: name = prefabName + "(Clone)" (UNVERIFIED: the prefab GameObject's own name needs the asset bundle), bounds (1, 0.5)
    static inline std::function<std::string(const std::string& prefabName)> instantiatedName;
    static inline std::function<std::pair<float, float>(const std::string& rocType)> rendererBoundsY;
    // PSystemManager.Instance.localBodies. Unset: FlightRegistry::Bodies()
    static inline std::function<std::vector<CelestialBody*>()> localBodies;
    // body.gameObject.GetComponentInChildren<PQSROCControl>() and its PQS (PQS values); nullptr = none
    static inline std::function<PQSROCControl*(CelestialBody*)> bodyRocControl;
    static inline std::function<std::optional<RocPqsView>(PQSROCControl*)> pqsOf;
    // Physics.Raycast hit test of ROC.GetClosestScanPositionWithRaycasts (hit collider tagged "ROC"): unset = no hit
    static inline std::function<bool(const Vector3f& origin, const Vector3f& dir, float maxDistance, float& hitDistance)> raycastHitsRoc;
    // FXMonger.ROCExplode(position, 0.3): recorded
    static inline std::vector<std::pair<Vector3f, double>> rocExplosions;
    // ROCManager's debug stat callbacks, ResourceScenario.Instance.gameSettings.ROCMissionSeed: unset = 0
    static inline std::function<int()> rocMissionSeed;
    static inline bool resourceScenarioPresent = false;  // ResourceScenario.Instance != null
    // PSystemManager.OnPSystemReady is fired by the host: ROCManager.GetROCControlFromCB()
    // GameSettings.COLLECT_ROC_STATS (GameSettings.cs); default false
    static inline bool collectRocStats = false;
    // sphere.ResetModList() calls of Disable/EnableAllROCs: recorded
    static inline int resetModListCalls = 0;
    static void Reset();
};

// PQSMod_ROCScatterQuad + the cache of LandClassROC.cs:6
class LandClassROC {
public:
    ROCDefinition* rocType = nullptr;
    std::string rocName;
    int gameSeed = 0;
    int quadSeed = 0;
    int maxCache = 0;
    float cacheMaxSubdivMultiplier = 0.0f;
    float rocFrequency = 0.0f;
    bool castShadows = false, recieveShadows = false;
    int maxROCs = 0, maxROCsMultiplier = 0;
    CelestialBody* celestialBody = nullptr;
    int minLevel = 0;   // private set

    LandClassROC(ROCDefinition* rocDefinition, PQSROCControl* rocControl);  // (C++ addition)
    void Setup(const RocPqsView& sphere);                                  // (C++ addition)
    void SphereActive();                                                   // (C++ addition)
    void SphereInactive();                                                 // (C++ addition)
    void AddScatterMeshController(RocQuad* quad, int rocIDCounter);        // (C++ addition)
    void DestroyQuad(RocScatterQuad* q);                                   // :1031
    // :210 CreateScatterMesh: the placements of q.count ROCs (the engine instantiates them); also fills the control's rocPositionsUsed
    std::vector<RocPlacement> CreateScatterMesh(RocScatterQuad* q);
    void ClearCache();                                                     // :1491

    // observable cache state
    int cacheUnassignedCount = 0, cacheAssignedCount = 0;
    std::string rocObjectName;                       // rocObject.name (the instantiated prefab's name), "" = rocObject == null
    PQSROCControl* rocControl = nullptr;
    int rocN = 0;
    RocScatterQuad* lastAssigned = nullptr;
    std::vector<std::unique_ptr<RocScatterQuad>> allQuads;   // owns every pool entry
    std::vector<RocScatterQuad*> cacheAssigned;              // List
    std::stack<RocScatterQuad*> cacheUnassigned;             // Stack
    bool cacheCreated = false, cacheRecreated = false;
    RocPqsView sphere;
    bool rocParentCreated = false;

private:
    void CreateCache();                        // :1517
    void BuildCache(int countToAdd);           // (C++ addition)
    int rocLoop = 0;
};

// PQSROCControl.cs:6 (a PQSMod)
class PQSROCControl : public PQSMod {
public:
    CelestialBody* celestialBody = nullptr;
    std::string currentCBName;
    std::vector<std::unique_ptr<LandClassROC>> rocs;
    float quadArea = 0.0f;
    DictionaryValueList<int, RocPositionList> rocPositionsUsed;
    RocPqsView sphereView;                  // `sphere` (PQS) flags for the LandClassROCs

    PQSROCControl();
    void Awake();                           // :40 modExpansionDisabled = !Serenity; the dictionaries
    void OnSetup() override;                // :47 (rocMinSubdev from the LandClassROCs)
    void OnQuadPreBuild(const PQ& quad) override;   // :69
    void OnVertexBuild(PQSVertexBuildData& data) override;  // :81
    void OnQuadBuilt(const PQ& quad) override;      // :86 (the quad mesh comes from PQSROCControl::quadInfo)
    void OnSphereStarted();                 // :131
    void OnSphereReset();                   // :140
    void OnQuadDestroy(const PQ& quad);     // :149
    // The ROC view of a PQ (mesh, name, transform position): installed by the quad builder (WP-12) / tests. Unset: nullptr (no mesh data -> the quad is skipped)
    static inline std::function<RocQuad*(const PQ&)> quadInfo;

    int rocMinSubdev = 0;
    int rocCount = 0;
    bool rocsActive = false;
    bool allowROCScatter = false;
    int itr = 0;
    DictionaryValueList<int, std::vector<ROCDefinition*>> quadAppliedROCTypes;
    std::vector<ROCDefinition*> quadROCTypesCache;
    bool hasQuadROCTypesCache = false;
};

// ROCManager.cs:7 (shares the removed-ROC list with the save model: SaveEnvironment::rocManager)
class ROCManager : public ROCManagerRemovedROCs {
public:
    // ROCManager.ROCStats (:9)
    struct ROCStats {
        std::string rocType;
        int activeQuads = 0;
        float activeQuadArea = 0.0f;
        int activeRocCount = 0;
        float rocTypeFrequency = 0.0f;
        float rocCoverage() const;  // :22
        explicit ROCStats(std::string rocType_) : rocType(std::move(rocType_)) {}  // :34
    };

    static inline ROCManager* Instance = nullptr;  // :75
    std::vector<std::unique_ptr<ROCDefinition>> _rocDefinitions;
    std::vector<ROCDefinition*> rocDefinitions() const;           // :79 (pointers in list order)
    std::vector<PQSROCControl*> _pqsRocControls;
    DictionaryValueList<std::string, std::shared_ptr<ROC>> rocTypeObjects;  // :77 (type -> the instantiated, deactivated prefab's ROC component)
    DictionaryValueList<std::string, std::shared_ptr<ROCStats>> rocStats;
    bool rocsEnabledInCurrentGame = false;
    bool debugROCFinder = false, debugROCScanPoints = false, debugROCStats = false;
    std::function<void()> OnStatsChanged;
    bool destroyed = false;

    ROCManager() = default;
    ~ROCManager();
    bool RocsEnabledInCurrentGame() const { return rocsEnabledInCurrentGame; }  // :83
    void Awake();                                                 // :85
    // :106 Start: ROC_DEFINITION nodes -> definitions (prefab check); subscribes the events. `getConfigNodes` is ScienceEnvironment::getConfigNodes.
    void Start();
    void OnGameStateLoad(ConfigNode* node);                       // :169
    void OnGameNewStart();                                        // :181
    void onMissionStarted(Expansions::Missions::Mission* mission); // :186
    void SetROCsStateForGame(bool active);                        // :191
    void GetROCControlFromCB();                                   // :205 (PSystemManager.OnPSystemReady)
    bool PopulateROCControlFromConfig(PQSROCControl* ROCControl); // :219
    void ValidateCBBiomeCombos();                                 // :260
    CelestialBody* ValidCelestialBody(const std::string& bodyName);  // :297
    bool ValidCBBiome(CelestialBody* body, const std::string& biomeName);  // :316
    void DisableAllROCs();                                        // :335
    void EnableAllROCs();                                         // :347
    void SaveRemoveROCs(ConfigNode& node) const { ROCManagerRemovedROCs::SaveRemoveROCs(node); }  // :359
    void LoadRemovedROCs(const ConfigNode& node) { ROCManagerRemovedROCs::LoadRemovedROCs(node); }  // :370
    bool ROCRemoved(int rocId);                                   // :384
    void RemoveROC(int rocId);                                    // :407
    void ClearROCsCache();                                        // :416
    void AddROCStats_Quad(PQSROCControl* control, ROCDefinition* r, const RocQuad& quad);  // :440
    void AddROCStats_ROCs(const std::string& rType, int count, float frequency);  // :455
    void ClearStats();                                            // :469
    void SubtractROCStats_Quad(PQSROCControl* control, ROCDefinition* r);  // :474
    void SubtractROCStats_ROCs(const std::string& rType, int count);  // :487
    std::vector<std::string> rocTypeObjectNames;                  // rocTypeObjects keys in order

private:
    bool subscribed_ = false;
};

// ROCScenario.cs:6 (KSPScenario creation options 1066, scene FLIGHT)
class ROCScenario : public ScenarioModule {
public:
    ROCScenario();
    static inline ROCScenario* Instance = nullptr;  // :12
    std::vector<ROC*> currentROCs;                  // :8
    std::map<std::string, ROC*> destroyedROCs;      // :10
    bool destroyed = false;
    void Start();                                   // :14
    static void Register();
};

namespace detail {
void RegisterRocScenario();
}

}  // namespace ksp
