// WP-53: ROCDefinition.cs, RocCBDefinition.cs, ROC.cs, ROCManager.cs, ROCScenario.cs, LandClassROC.cs, PQSROCControl.cs
// (see include/ksp/serenity/roc.hpp).
#include "ksp/serenity/roc.hpp"

#include <algorithm>
#include <limits>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/science_modules.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/game.hpp"
#include "ksp/science/science.hpp"
#include "ksp/serenity/serenity_env.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/misc_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {
namespace {

using GE = events::GameEvents;
bool SerenityInstalled() { return SaveEnvironment::Current().IsExpansionInstalled("Serenity"); }

std::vector<CelestialBody*> LocalBodies() {
    if (RocEnvironment::localBodies) return RocEnvironment::localBodies();
    return FlightRegistry::Bodies();
}

// unchecked int arithmetic (C# wraps)
int Wrap3(int a, int b, int c = 0, int d = 0) {
    return static_cast<int>(static_cast<uint32_t>(a) + static_cast<uint32_t>(b) + static_cast<uint32_t>(c) + static_cast<uint32_t>(d));
}

template <class T>
void AddUnique(std::vector<T>& v, const T& x) {
    if (std::find(v.begin(), v.end(), x) == v.end()) v.push_back(x);
}

}  // namespace

void RocEnvironment::Reset() {
    prefabState = nullptr;
    instantiatedName = nullptr;
    rendererBoundsY = nullptr;
    localBodies = nullptr;
    bodyRocControl = nullptr;
    pqsOf = nullptr;
    raycastHitsRoc = nullptr;
    rocExplosions.clear();
    rocMissionSeed = nullptr;
    resourceScenarioPresent = false;
    collectRocStats = false;
    resetModListCalls = 0;
}

// =============================================================================================================================
// RocCBDefinition / ROCDefinition

// RocCBDefinition.cs:23
void RocCBDefinition::Load(const ConfigNode& node) {
    name = node.GetValue("Name").value_or("");  // sic: GetValue returns null when absent
    biomes = node.GetValuesList("Biome");
}

// ROCDefinition.cs:126
bool ROCDefinition::ContainsCBBiome(const std::string& cbName, const std::string& biomeName) const {
    for (size_t i = 0; i < myCelestialBodies.size(); i++) {
        if (myCelestialBodies[i].name != cbName) continue;
        for (size_t j = 0; j < myCelestialBodies[i].biomes.size(); j++)
            if (myCelestialBodies[i].biomes[j] == biomeName) return true;
    }
    return false;
}

// ROCDefinition.cs:145
void ROCDefinition::Load(const ConfigNode& node) {
    node.TryGetValue("Type", type);
    node.TryGetValue("displayName", displayName);
    node.TryGetValue("prefabName", prefabName);
    node.TryGetValue("modelName", modelName);
    node.TryGetValue("OrientateUp", orientateUp);
    node.TryGetValue("Depth", depth);
    node.TryGetValue("CanBeTaken", canBeTaken);
    node.TryGetValue("Frequency", frequency);
    node.TryGetValue("CastShadows", castShadows);
    node.TryGetValue("ReceiveShadows", receiveShadows);
    node.TryGetValue("CollisionThreshold", collisionThreshold);
    node.TryGetValue("SmallROC", smallRoc);
    node.TryGetValue("RandomDepth", randomDepth);
    node.TryGetValue("RandomOrientation", randomOrientation);
    node.TryGetValue("RandomRotation", randomRotation);
    node.TryGetValue("burstEmitterMinWait", burstEmitterMinWait);
    node.TryGetValue("burstEmitterMaxWait", burstEmitterMaxWait);
    node.TryGetValue("sfxVolume", sfxVolume);
    node.TryGetValue("idleClip", idleClipPath);
    node.TryGetValue("burstClip", burstClipPath);
    node.TryGetValue("vfxBaseForce", vfxBaseForce);
    ConfigNode* node2 = nullptr;
    if (node.TryGetNode("VFX_CURVEFORCE", node2) && node2->CountValues() > 0) {
        vfxCurveForce = FloatCurve();
        vfxCurveForce->Load(*node2);
    }
    std::vector<std::string> values = node.GetValues("localSpaceScanPoints");
    for (size_t i = 0; i < values.size(); i++) localSpaceScanPoints.push_back(ConfigNode::ParseVector3(values[i]));
    node.TryGetValue("applyForces", applyForces);
    node.TryGetValue("vfxForceRadius", vfxForceRadius);
    node.TryGetValue("forceDirection", forceDirection);
    node.TryGetValue("vfxRadiusCenter", radiusCenter);
    node.TryGetValue("Scale", scale);
}

// =============================================================================================================================
// ROC

// ROC.cs:118
void ROC::Awake() {
    if (!SerenityInstalled()) destroyed = true;
    if (upDirection.x == 0.0f && upDirection.y == 0.0f && upDirection.z == 0.0f) upDirection = transform.up();
}

// ROC.cs:130
void ROC::Start() {
    started = true;
    if (SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT) {
        if (ROCManager::Instance == nullptr) throw NullReferenceException();
        ROCManager::Instance->AddROCStats_ROCs(type, 1, frequency);
    }
    if (ROCManager::Instance == nullptr) throw NullReferenceException();
}

// ROC.cs:168
void ROC::SetStats(const std::string& type_, const std::string& displayName_, const std::string& prefabName_, const std::string& modelName_, bool orientateup_,
                   float depth_, bool canbetaken_, float frequency_, const std::vector<RocCBDefinition>& myCelestialBodies_, bool castShadows_, bool recieveShadows_,
                   float collisionthreshold, bool smallroc, bool randomdepth, bool randomorientation, const std::vector<Vector3f>& localSpaceScanPoints_,
                   float burstEmitterMinWait_, float burstEmitterMaxWait_, bool randomRotation_, float scale_, float sfxVolume_, const std::string& idleClipPath_,
                   const std::string& burstClipPath_, const std::optional<FloatCurve>& vfxCurveForce_, float vfxBaseForce_, bool applyForces_,
                   const Vector2f& vfxForceRadius_, const Vector3f& forceDirection_, const Vector3f& radiusCenter_) {
    type = type_;
    displayName = displayName_;
    prefabName = prefabName_;
    modelName = modelName_;
    orientateup = orientateup_;
    depth = depth_;
    canbetaken = canbetaken_;
    frequency = frequency_;
    myCelestialBodies = myCelestialBodies_;
    castShadows = castShadows_;
    recieveShadows = recieveShadows_;
    collisionThreshold = collisionthreshold;
    smallROC = smallroc;
    randomDepth = randomdepth;
    randomOrientation = randomorientation;
    localSpaceScanPoints = localSpaceScanPoints_;
    burstEmitterMinWait = burstEmitterMinWait_;
    burstEmitterMaxWait = burstEmitterMaxWait_;
    randomRotation = randomRotation_;
    sfxVolume = sfxVolume_;
    idleClipPath = idleClipPath_;
    burstClipPath = burstClipPath_;
    scale = scale_;
    vfxBaseForce = vfxBaseForce_;
    vfxCurveForce = vfxCurveForce_;
    applyForces = applyForces_;
    vfxForceRadius = vfxForceRadius_;
    forceDirection = forceDirection_;
    radiusCenter = radiusCenter_;
}

// ROC.cs:201
void ROC::OnDestroyed() {
    if (SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT) {
        if (ROCManager::Instance == nullptr) throw NullReferenceException();
        ROCManager::Instance->SubtractROCStats_ROCs(type, 1);
    }
}

// ROC.cs:321
void ROC::UpdateModelTransform() {
    // modelTransform = transform.Find(modelName): the child lookup is the engine's (`modelTransform` / `modelTransformFound` are set by the host)
}

// ROC.cs:211
Vector3f ROC::GetClosestScanPosition(const Vector3f& testPosition, float& bestDistanceToPoint) {
    UpdateModelTransform();
    if (!modelTransformFound) throw NullReferenceException();
    Vector3f result = modelTransform.position;
    bestDistanceToPoint = std::numeric_limits<float>::max();
    bool flag = false;
    for (size_t i = 0; i < localSpaceScanPoints.size(); i++) {
        const Vector3f zero = modelTransform.TransformPoint(localSpaceScanPoints[i]);
        const float magnitude = (zero - testPosition).magnitude();
        if (bestDistanceToPoint > magnitude) {
            result = zero;
            bestDistanceToPoint = magnitude;
            flag = true;
        }
    }
    if (!flag) bestDistanceToPoint = (modelTransform.position - testPosition).magnitude();
    return result;
}

// ROC.cs:236
Vector3f ROC::GetClosestScanPositionWithRaycasts(const Vector3f& testPosition, float& distanceToPoint) {
    UpdateModelTransform();
    if (!modelTransformFound) throw NullReferenceException();
    distanceToPoint = std::numeric_limits<float>::max();
    struct ScanPoint {
        Vector3f point, traceDirection;
        float distance = 0.0f;
    };
    std::vector<ScanPoint> list;
    for (size_t i = 0; i < localSpaceScanPoints.size(); i++) {
        ScanPoint sp;
        sp.point = modelTransform.TransformPoint(localSpaceScanPoints[i]);
        sp.traceDirection = sp.point - testPosition;
        sp.distance = sp.traceDirection.magnitude();
        list.push_back(sp);
    }
    // List.Sort() with ScanPoint.CompareTo (distance); .NET's unstable introsort: for the <= 16 elements ROC definitions have it is an insertion sort
    for (size_t i = 1; i < list.size(); i++) {
        ScanPoint key = list[i];
        size_t j = i;
        while (j > 0 && (key.distance < list[j - 1].distance)) {
            list[j] = list[j - 1];
            j--;
        }
        list[j] = key;
    }
    for (size_t num = 0; num < list.size(); num++) {
        float hit = 0.0f;
        if (RocEnvironment::raycastHitsRoc && RocEnvironment::raycastHitsRoc(testPosition, list[num].traceDirection.normalized(), list[num].distance, hit)) {
            distanceToPoint = hit;
            return list[num].point;
        }
    }
    return GetClosestScanPosition(testPosition, distanceToPoint);
}

// ROC.cs:273
void ROC::CheckCollision(Vessel& vessel) {
    if (vessel.speed > static_cast<double>(collisionThreshold) && smallROC) {
        if (ROCManager::Instance != nullptr) ROCManager::Instance->RemoveROC(rocID);
        UpdateModelTransform();
        RocEnvironment::rocExplosions.push_back({modelTransform.position, 0.30000001192092896});  // FXMonger.ROCExplode(position, 0.3)
        destroyed = true;
    }
}

// ROC.cs:287 (the collision bookkeeping of the parts that touched the collider is the engine's)
void ROC::PickUpROC() {
    if (ROCManager::Instance != nullptr) ROCManager::Instance->RemoveROC(rocID);
    destroyed = true;
}

// ROC.cs:305
bool ROC::PerformExperiment(ModuleScienceExperiment* moduleScienceExperiment) {
    if (moduleScienceExperiment == nullptr) {
        log::Warning("[ROC]: Unable to run Experiment on ROC '" + type + "' ModuleScienceExperiment PartModule is not defined in this Parts cfg file.");
        throw NullReferenceException();  // moduleScienceExperiment.experimentID
    }
    const std::string experimentID = moduleScienceExperiment->Get<std::string>("experimentID") + "_" + type;
    moduleScienceExperiment->experiment = ResearchAndDevelopment::GetExperiment(experimentID);
    if (moduleScienceExperiment->experiment != nullptr) {
        moduleScienceExperiment->DeployExperiment();
        return true;
    }
    return false;
}

// ROC.cs:426
float ROC::GetVFXForceScale(float burstClipTime) const {
    if (!vfxCurveForce) throw NullReferenceException();
    return vfxCurveForce->Evaluate(burstClipTime);
}

// =============================================================================================================================
// PQSMod_ROCScatterQuad (data only)

// PQSMod_ROCScatterQuad.cs:22 (the PQ delegate registration and the transform placement are WP-48's)
void RocScatterQuad::Setup(RocQuad* q, int quadSeed_, LandClassROC* roc_, int count_, int rocIDCounter) {
    quad = q;
    quadSeed = quadSeed_;
    count = count_;
    roc = roc_;
    name = "_" + roc_->rocName;
    rocIDForRoc = Wrap3(quadSeed_, StringExtensions::GetHashCode_Net35(roc_->rocName), rocIDCounter);
    isVisible = true;
    isBuilt = false;
}

// PQSMod_ROCScatterQuad.cs:81
void RocScatterQuad::Destroy() {
    spawnedRocIds.clear();
    quad = nullptr;
    isBuilt = false;
    isVisible = false;
}

// =============================================================================================================================
// LandClassROC

// LandClassROC.cs:74
LandClassROC::LandClassROC(ROCDefinition* rocDefinition, PQSROCControl* control) {
    rocType = rocDefinition;
    rocName = rocType->type;
    rocControl = control;
    const std::vector<CelestialBody*> bodies = LocalBodies();
    for (size_t i = 0; i < bodies.size(); i++)
        if (bodies[i]->name() == control->currentCBName) celestialBody = bodies[i];
    cacheMaxSubdivMultiplier = 20.0f;
    maxROCsMultiplier = 5;
    rocFrequency = rocDefinition->frequency;
    castShadows = rocDefinition->castShadows;
    recieveShadows = rocDefinition->receiveShadows;
    if (ROCManager::Instance != nullptr) {
        rocObjectName.clear();
        std::shared_ptr<ROC> obj;
        if (ROCManager::Instance->rocTypeObjects.TryGetValue(rocType->type, obj) && obj) rocObjectName = obj->name;
    }
}

// LandClassROC.cs:98
void LandClassROC::Setup(const RocPqsView& s) {
    sphere = s;
    minLevel = sphere.maxLevel;
    maxCache = dotnet::ConvToInt32(fx((rocFrequency < 1.0f) ? 1.0f : rocFrequency) * fx(static_cast<float>(sphere.maxLevel)) * fx(cacheMaxSubdivMultiplier));
    maxROCs = maxROCsMultiplier;
    CreateCache();
}

// LandClassROC.cs:107
void LandClassROC::CreateCache() {
    if (!cacheCreated) {
        cacheAssigned.clear();
        cacheAssignedCount = 0;
        cacheUnassigned = std::stack<RocScatterQuad*>();
        cacheUnassignedCount = 0;
        cacheCreated = true;
    }
}

// LandClassROC.cs:119
void LandClassROC::SphereActive() {
    if (RocPqsStatics::Global_AllowScatter && cacheUnassignedCount == 0) BuildCache(maxCache);
}

// LandClassROC.cs:127
void LandClassROC::SphereInactive() { ClearCache(); }

// LandClassROC.cs:132
void LandClassROC::AddScatterMeshController(RocQuad* quad, int rocIDCounter) {
    SaveEnvironment& env = SaveEnvironment::Current();
    if (env.currentGame != nullptr && env.currentGame->Mode == Game_Modes::MISSION && RocEnvironment::resourceScenarioPresent) {
        const int seed = RocEnvironment::rocMissionSeed ? RocEnvironment::rocMissionSeed() : 0;
        env.currentGame->ROCSeed = seed;
        gameSeed = seed;
    } else if (env.currentGame != nullptr && env.currentGame->ROCSeed > -1) {
        gameSeed = env.currentGame->ROCSeed;
    } else {
        gameSeed = 0;
    }
    quadSeed = Wrap3(gameSeed, StringExtensions::GetHashCode_Net35(quad->name), StringExtensions::GetHashCode_Net35(rocName));
    UnityRandom& rnd = UnityRandom::Global();
    rnd.InitState(quadSeed);
    const float num = f32store(fx(rocFrequency) * fx(rocControl->quadArea));
    const float num2 = Mathf::Round(f32store(1.0 / fx(num)));
    rocN = dotnet::ConvToInt32(fx(num));
    const int num3 = rnd.Range(1, dotnet::ConvToInt32(fx(num2)));
    if (num < 1.0f && num > 0.0f && num3 == 1) rocN = 1;
    maxROCs = rocN * maxROCsMultiplier;
    if (rocN >= 1) {
        if (!cacheRecreated) {
            cacheCreated = false;
            CreateCache();
            cacheRecreated = true;
        }
        if (cacheUnassignedCount == 0) BuildCache(maxCache);
        if (cacheUnassigned.empty()) throw Exception("InvalidOperationException: Stack empty.");
        RocScatterQuad* qc = cacheUnassigned.top();
        cacheUnassigned.pop();
        cacheAssigned.push_back(qc);
        cacheUnassignedCount--;
        cacheAssignedCount++;
        lastAssigned = qc;
        qc->Setup(quad, quadSeed, this, rocN, rocIDCounter);
    }
}

// LandClassROC.cs:181
void LandClassROC::BuildCache(int countToAdd) {
    rocParentCreated = true;
    for (int i = 0; i < countToAdd; i++) {
        allQuads.push_back(std::make_unique<RocScatterQuad>());
        RocScatterQuad* q = allQuads.back().get();
        q->name = "Unassigned";
        q->active = false;
        cacheUnassigned.push(q);
    }
    cacheUnassignedCount += countToAdd;
}

// LandClassROC.cs:209
void LandClassROC::DestroyQuad(RocScatterQuad* q) {
    if (rocControl != nullptr && q->quad != nullptr && rocControl->rocPositionsUsed.ContainsKey(StringExtensions::GetHashCode_Net35(q->quad->name))) {
        RocPositionList list = rocControl->rocPositionsUsed[StringExtensions::GetHashCode_Net35(q->quad->name)];
        int count = static_cast<int>(list->size());
        while (count-- > 0)
            if ((*list)[static_cast<size_t>(count)]->rocType == rocName) list->erase(list->begin() + count);
    }
    q->Destroy();
    auto it = std::find(cacheAssigned.begin(), cacheAssigned.end(), q);
    if (it != cacheAssigned.end()) {
        cacheAssigned.erase(it);
        cacheUnassigned.push(q);
        cacheAssignedCount--;
        cacheUnassignedCount++;
    }
    q->name = "Unassigned";
    q->active = false;
}

// LandClassROC.cs:235
std::vector<RocPlacement> LandClassROC::CreateScatterMesh(RocScatterQuad* q) {
    std::vector<RocPlacement> placements;
    int num = 0;
    UnityRandom& rnd = UnityRandom::Global();
    rnd.InitState(q->quadSeed);
    if (rocControl != nullptr) {
        const int key = StringExtensions::GetHashCode_Net35(q->quad->name);
        if (!rocControl->rocPositionsUsed.ContainsKey(key)) rocControl->rocPositionsUsed.Add(key, std::make_shared<std::vector<std::shared_ptr<RocPositionInfo>>>());
        RocPositionList listRef = rocControl->rocPositionsUsed[key];
        std::vector<std::shared_ptr<RocPositionInfo>>& list = *listRef;
        RocQuad& quad = *q->quad;
        for (rocLoop = 0; rocLoop < q->count; rocLoop++) {
            int num2 = -1;
            int num3 = -1;
            bool flag = false;
            const std::pair<float, float> bounds = RocEnvironment::rendererBoundsY ? RocEnvironment::rendererBoundsY(rocType->type) : std::make_pair(1.0f, 0.5f);
            const float num4 = f32store(fx(q->roc->rocType->depth) * fx(bounds.first));
            const float num5 = f32store(fx(rnd.value()) * 360.0);
            const Quaternionf rotation = rnd.rotation();
            const float num6 = rnd.Range(0.1f, num4);
            while (num3 == num2) {
                const int num7 = rnd.Range(1, RocPqsStatics::cacheRes() + 1);
                const int num8 = rnd.Range(1, RocPqsStatics::cacheRes() + 1);
                const int x = num7 + rnd.Range(-1, 1);
                const int z = num8 + rnd.Range(-1, 1);
                num3 = RocPqsStatics::vi(num7, num8);
                num2 = RocPqsStatics::vi(x, z);
            }
            Vector3f rocPOS, quadNormal;
            if (rnd.Range(0, 1) == 0) {
                rocPOS = quad.verts.at(static_cast<size_t>(num3));
                quadNormal = quad.vertNormals.at(static_cast<size_t>(num3));
            } else {
                rocPOS = quad.verts.at(static_cast<size_t>(num2));
                quadNormal = quad.vertNormals.at(static_cast<size_t>(num2));
            }
            for (size_t i = 0; i < list.size(); i++) {
                if (fx(list[i]->position.x) == fx(Mathf::Round(rocPOS.x)) && fx(list[i]->position.y) == fx(Mathf::Round(rocPOS.y)) &&
                    fx(list[i]->position.z) == fx(Mathf::Round(rocPOS.z)))
                    flag = true;
            }
            if (!flag) {
                auto info = std::make_shared<RocPositionInfo>();
                info->rocType = rocName;
                info->position = Vector3f(Mathf::Round(rocPOS.x), Mathf::Round(rocPOS.y), Mathf::Round(rocPOS.z));
                list.push_back(info);  // AddUnique over reference objects: always added
                Vector3f rocUp;
                if (sphere.surfaceRelativeQuads)
                    rocUp = (rocPOS + quad.positionPlanet).normalized();
                else
                    rocUp = rocPOS.normalized();
                Quaternionf quaternion = quad.quadRotation;
                if (!q->roc->rocType->orientateUp) quaternion = UnityNative::FromToRotation(Vector3f(0.0f, 1.0f, 0.0f), quadNormal);
                Quaternionf rocRot;
                if (q->roc->rocType->randomRotation) {
                    rocRot = rotation * quaternion;
                } else if (q->roc->rocType->randomOrientation) {
                    rocRot = quaternion * UnityNative::AngleAxis(num5, Vector3f(0.0f, 1.0f, 0.0f));
                } else {
                    rocRot = UnityNative::AngleAxis(0.0f, rocUp) * quaternion;
                }
                // `if (componentInChildren != null)`: the prefab's Renderer exists (bounds are supplied)
                rocPOS += rocUp * bounds.second;
                if (!q->roc->rocType->randomDepth)
                    rocPOS += -rocUp * num4;
                else
                    rocPOS += -rocUp * num6;
                const int num9 = Wrap3(gameSeed, StringExtensions::GetHashCode_Net35(rocObjectName), rocLoop, dotnet::ConvToInt32(fx(rocPOS.magnitude())));
                const bool removed = !rocObjectName.empty() && ROCManager::Instance != nullptr && ROCManager::Instance->ROCRemoved(num9);
                if (!rocObjectName.empty() && ROCManager::Instance != nullptr && !removed) {
                    RocPlacement pl;
                    pl.rocID = num9;
                    pl.localPosition = rocPOS;
                    pl.localRotation = rocRot;
                    pl.scale = q->roc->rocType->scale;
                    pl.upDirection = quad.transform.TransformDirection(quadNormal);
                    pl.rocType = rocName;
                    placements.push_back(pl);
                    q->spawnedRocIds.push_back(num9);
                }
                if (!rocObjectName.empty() && ROCManager::Instance != nullptr && removed)
                    log::Info("[LandClassROC]: Skipped ROC " + std::to_string(num9) + " as it has been removed from this game.");
                else
                    num++;
            }
        }
        rocControl->rocPositionsUsed.Set(key, listRef);
        q->active = true;
        q->isBuilt = true;
    } else {
        log::Error("[LandClassROC]: Unable to CreateScatterMesh for Quad " + q->quad->name + " Roc Type " + rocName + " as Roc Control reference is not set!");
    }
    return placements;
}

// LandClassROC.cs:368
void LandClassROC::ClearCache() {
    if (cacheCreated) {
        std::vector<RocScatterQuad*> list = cacheAssigned;
        for (int num = static_cast<int>(list.size()) - 1; num >= 0; num--) DestroyQuad(list[static_cast<size_t>(num)]);
    }
    cacheAssignedCount = 0;
    for (int i = 0; i < cacheUnassignedCount; i++) {
        if (!cacheUnassigned.empty()) cacheUnassigned.pop();
    }
    cacheUnassignedCount = 0;
    cacheCreated = false;
    if (ROCManager::Instance != nullptr) ROCManager::Instance->ClearStats();
}

// =============================================================================================================================
// PQSROCControl

PQSROCControl::PQSROCControl() { className = "PQSROCControl"; }

// PQSROCControl.cs:40
void PQSROCControl::Awake() {
    modExpansionDisabled = !SerenityInstalled();
    rocPositionsUsed = DictionaryValueList<int, RocPositionList>();
    quadAppliedROCTypes = DictionaryValueList<int, std::vector<ROCDefinition*>>();
}

// PQSROCControl.cs:47
void PQSROCControl::OnSetup() {
    requirements = PQS::VertexMapCoords | PQS::MeshColorChannel;
    if (RocEnvironment::pqsOf) {
        if (auto v = RocEnvironment::pqsOf(this)) sphereView = *v;
    }
    if (ROCManager::Instance == nullptr) return;
    rocCount = static_cast<int>(rocs.size());
    rocMinSubdev = std::numeric_limits<int>::max();
    for (size_t i = 0; i < rocs.size(); i++) {
        rocs[i]->Setup(sphereView);
        if (rocs[i]->minLevel < rocMinSubdev) rocMinSubdev = rocs[i]->minLevel;
    }
}

// PQSROCControl.cs:69
void PQSROCControl::OnQuadPreBuild(PQ& quad) {
    rocsActive = quad.subdivision >= rocMinSubdev && ROCManager::Instance != nullptr && celestialBody != nullptr;
}

// PQSROCControl.cs:81
void PQSROCControl::OnVertexBuild(PQSVertexBuildData& data) { allowROCScatter = data.allowScatter; }

// PQSROCControl.cs:86
void PQSROCControl::OnQuadBuilt(PQ& pq) {
    int num = 0;
    if (!rocsActive || rocCount <= 0 || !allowROCScatter) return;
    num++;
    RocQuad* quad = quadInfo ? quadInfo(pq) : nullptr;
    if (quad == nullptr) return;  // no mesh data for the quad (the quad builder is WP-12)
    const Vector2d latitudeAndLongitude = celestialBody->GetLatitudeAndLongitude(quad->quadTransformPosition);
    const std::string experimentBiome = ScienceUtil::GetExperimentBiome(*celestialBody, latitudeAndLongitude.x, latitudeAndLongitude.y);
    hasQuadROCTypesCache = false;
    quadROCTypesCache.clear();
    if (quad->hasMesh) {
        quadArea = f32store(fx(quad->meshBoundsSize.x) / 1000.0 * fx(quad->meshBoundsSize.z) / 1000.0);
    } else {
        quadArea = f32store(quad->quadArea / 10000000000.0);
        log::Error("[PQSROCControl]: Failed to calculate Quad Area. Quad:" + quad->name);
    }
    for (itr = 0; itr < rocCount; itr++) {
        if (rocs[static_cast<size_t>(itr)]->rocType->ContainsCBBiome(celestialBody->name(), experimentBiome)) {
            rocs[static_cast<size_t>(itr)]->AddScatterMeshController(quad, num);
            if (!hasQuadROCTypesCache) {
                hasQuadROCTypesCache = true;
                quadROCTypesCache.clear();
            }
            if (ROCManager::Instance != nullptr && ROCManager::Instance->debugROCStats) {
                quadROCTypesCache.push_back(rocs[static_cast<size_t>(itr)]->rocType);
                ROCManager::Instance->AddROCStats_Quad(this, rocs[static_cast<size_t>(itr)]->rocType, *quad);
            }
        }
    }
    if (hasQuadROCTypesCache && ROCManager::Instance != nullptr && ROCManager::Instance->debugROCStats)
        quadAppliedROCTypes.Add(StringExtensions::GetHashCode_Net35(quad->name), quadROCTypesCache);
    rocsActive = false;
}

// PQSROCControl.cs:131
void PQSROCControl::OnSphereStarted() {
    for (size_t i = 0; i < rocs.size(); i++) rocs[i]->SphereActive();
}

// PQSROCControl.cs:140
void PQSROCControl::OnSphereReset() {
    for (size_t i = 0; i < rocs.size(); i++) rocs[i]->SphereInactive();
}

// PQSROCControl.cs:149
void PQSROCControl::OnQuadDestroy(PQ& pq) {
    RocQuad* quad = quadInfo ? quadInfo(pq) : nullptr;
    if (quad == nullptr) return;
    const int key = StringExtensions::GetHashCode_Net35(quad->name);
    if (ROCManager::Instance != nullptr && ROCManager::Instance->debugROCStats && quadAppliedROCTypes.ContainsKey(key)) {
        std::vector<ROCDefinition*> types = quadAppliedROCTypes[key];
        for (size_t i = 0; i < types.size(); i++) ROCManager::Instance->SubtractROCStats_Quad(this, types[i]);
        quadAppliedROCTypes.Remove(key);
    }
}

// =============================================================================================================================
// ROCManager

// ROCManager.cs:22
float ROCManager::ROCStats::rocCoverage() const {
    if (activeQuadArea <= 0.0f) return 0.0f;
    return f32store(fx(static_cast<float>(activeRocCount)) / fx(activeQuadArea));
}

ROCManager::~ROCManager() {
    if (subscribed_) {
        GE::onGameStateLoad.Remove(this, &ROCManager::OnGameStateLoad);
        GE::onGameNewStart.Remove(this, &ROCManager::OnGameNewStart);
        GE::Mission::onStarted.Remove(this, &ROCManager::onMissionStarted);
    }
    if (Instance == this) Instance = nullptr;
    if (SaveEnvironment::Current().rocManager == this) SaveEnvironment::Current().rocManager = nullptr;
}

std::vector<ROCDefinition*> ROCManager::rocDefinitions() const {
    std::vector<ROCDefinition*> r;
    for (const auto& d : _rocDefinitions) r.push_back(d.get());
    return r;
}

// ROCManager.cs:85
void ROCManager::Awake() {
    if (Instance != nullptr) {
        destroyed = true;  // DestroyImmediate(gameObject); Instance = null
        Instance = nullptr;
        return;
    }
    if (!SerenityInstalled()) {
        destroyed = true;
        Instance = nullptr;
        return;
    }
    Instance = this;
    SaveEnvironment::Current().rocManager = this;
    debugROCStats = RocEnvironment::collectRocStats;
    _pqsRocControls.clear();
    _rocDefinitions.clear();
    rocTypeObjects = DictionaryValueList<std::string, std::shared_ptr<ROC>>();
    rocStats = DictionaryValueList<std::string, std::shared_ptr<ROCStats>>();
}

// ROCManager.cs:113
void ROCManager::Start() {
    std::vector<ConfigNode*> rocDefs = ScienceEnvironment::getConfigNodes ? ScienceEnvironment::getConfigNodes("ROC_DEFINITION") : std::vector<ConfigNode*>();
    for (size_t i = 0; i < rocDefs.size(); i++) {
        auto def = std::make_unique<ROCDefinition>();
        def->Load(*rocDefs[i]);
        std::vector<ConfigNode*> cbDefs = rocDefs[i]->GetNodes("CELESTIALBODY");
        std::vector<RocCBDefinition> list;
        for (size_t j = 0; j < cbDefs.size(); j++) {
            RocCBDefinition cb;
            cb.Load(*cbDefs[j]);
            list.push_back(cb);
        }
        def->myCelestialBodies = list;
        _rocDefinitions.push_back(std::move(def));
    }
    for (int num = static_cast<int>(_rocDefinitions.size()) - 1; num >= 0; num--) {
        ROCDefinition& d = *_rocDefinitions[static_cast<size_t>(num)];
        int state = 0;
        if (!d.prefabName.empty()) state = RocEnvironment::prefabState ? RocEnvironment::prefabState(d.prefabName) : 2;
        if (state == 0) {
            log::Warning("[ROCManager]: Unable to instantiate prefab for ROC type " + d.type + " with prefab name " + d.prefabName);
        } else if (state == 2) {
            auto roc = std::make_shared<ROC>();
            roc->name = RocEnvironment::instantiatedName ? RocEnvironment::instantiatedName(d.prefabName) : d.prefabName + "(Clone)";
            roc->SetStats(d.type, d.displayName, d.prefabName, d.modelName, d.orientateUp, d.depth, d.canBeTaken, d.frequency, d.myCelestialBodies, d.castShadows,
                          d.receiveShadows, d.collisionThreshold, d.smallRoc, d.randomDepth, d.randomOrientation, d.localSpaceScanPoints, d.burstEmitterMinWait,
                          d.burstEmitterMaxWait, d.randomRotation, d.scale, d.sfxVolume, d.idleClipPath, d.burstClipPath, d.vfxCurveForce, d.vfxBaseForce,
                          d.applyForces, d.vfxForceRadius, d.forceDirection, d.radiusCenter);
            rocTypeObjects.Add(d.type, roc);
        } else {
            _rocDefinitions.erase(_rocDefinitions.begin() + num);
        }
    }
    // PSystemManager.Instance.OnPSystemReady.Add(GetROCControlFromCB): the host calls GetROCControlFromCB()
    GE::onGameStateLoad.Add(this, &ROCManager::OnGameStateLoad);
    GE::onGameNewStart.Add(this, &ROCManager::OnGameNewStart);
    GE::Mission::onStarted.Add(this, &ROCManager::onMissionStarted);
    subscribed_ = true;
}

// ROCManager.cs:169
void ROCManager::OnGameStateLoad(ConfigNode*) {
    Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr) throw NullReferenceException();
    SetROCsStateForGame(g->ROCSeed > -1);
}

// ROCManager.cs:181
void ROCManager::OnGameNewStart() { SetROCsStateForGame(true); }

// ROCManager.cs:186
void ROCManager::onMissionStarted(Expansions::Missions::Mission*) { SetROCsStateForGame(true); }

// ROCManager.cs:191
void ROCManager::SetROCsStateForGame(bool active) {
    if (active) {
        EnableAllROCs();
        rocsEnabledInCurrentGame = true;
    } else {
        DisableAllROCs();
        rocsEnabledInCurrentGame = false;
    }
}

// ROCManager.cs:205
void ROCManager::GetROCControlFromCB() {
    ValidateCBBiomeCombos();
    const std::vector<CelestialBody*> localBodies = LocalBodies();
    for (size_t i = 0; i < localBodies.size(); i++) {
        PQSROCControl* control = RocEnvironment::bodyRocControl ? RocEnvironment::bodyRocControl(localBodies[i]) : nullptr;
        if (control != nullptr && PopulateROCControlFromConfig(control)) _pqsRocControls.push_back(control);
    }
}

// ROCManager.cs:219
bool ROCManager::PopulateROCControlFromConfig(PQSROCControl* ROCControl) {
    bool result = true;
    for (size_t i = 0; i < _rocDefinitions.size(); i++) {
        ROCDefinition* def = _rocDefinitions[i].get();
        for (size_t j = 0; j < def->myCelestialBodies.size(); j++) {
            if (def->myCelestialBodies[j].name != ROCControl->currentCBName) continue;
            auto landClassROC = std::make_unique<LandClassROC>(def, ROCControl);
            std::optional<RocPqsView> pqs = RocEnvironment::pqsOf ? RocEnvironment::pqsOf(ROCControl) : std::optional<RocPqsView>(ROCControl->sphereView);
            if (pqs) {
                landClassROC->Setup(*pqs);
                if (pqs->isActive) landClassROC->SphereActive();
                if (landClassROC->celestialBody == nullptr) {
                    log::Warning("[ROCManager]: Invalid CelestialBody Name Defined for ROC " + def->type + " BodyName: " + ROCControl->currentCBName);
                    result = false;
                } else {
                    ROCControl->rocs.push_back(std::move(landClassROC));
                }
            } else {
                log::Warning("[ROCManager]: Unable to find PQS in parent for ROC " + def->type + " on " + ROCControl->currentCBName);
                result = false;
            }
        }
    }
    return result;
}

// ROCManager.cs:260
void ROCManager::ValidateCBBiomeCombos() {
    for (int num = static_cast<int>(_rocDefinitions.size()) - 1; num >= 0; num--) {
        ROCDefinition& d = *_rocDefinitions[static_cast<size_t>(num)];
        for (int num2 = static_cast<int>(d.myCelestialBodies.size()) - 1; num2 >= 0; num2--) {
            CelestialBody* celestialBody = ValidCelestialBody(d.myCelestialBodies[static_cast<size_t>(num2)].name);
            if (celestialBody == nullptr) {
                log::Warning("[ROCManager]: Invalid CelestialBody Name " + d.myCelestialBodies[static_cast<size_t>(num2)].name + " on ROC Definition " + d.type + ". Removed entry.");
                d.myCelestialBodies.erase(d.myCelestialBodies.begin() + num2);
            } else {
                RocCBDefinition& cb = d.myCelestialBodies[static_cast<size_t>(num2)];
                for (int num3 = static_cast<int>(cb.biomes.size()) - 1; num3 >= 0; num3--) {
                    if (!ValidCBBiome(celestialBody, cb.biomes[static_cast<size_t>(num3)])) {
                        log::Warning("[ROCManager]: Invalid Biome Name " + cb.biomes[static_cast<size_t>(num3)] + " for Celestial Body " + cb.name +
                                     " on ROC Definition " + d.type + ". Removed entry.");
                        cb.biomes.erase(cb.biomes.begin() + num3);
                    }
                }
            }
            if (d.myCelestialBodies.at(static_cast<size_t>(num2)).biomes.empty()) {
                log::Warning("[ROCManager]: No Valid Biomes for Celestial Body " + d.myCelestialBodies[static_cast<size_t>(num2)].name + " on ROC Definition " + d.type +
                             ". Removed entry.");
                d.myCelestialBodies.erase(d.myCelestialBodies.begin() + num2);
            }
        }
        if (d.myCelestialBodies.empty()) {
            log::Warning("[ROCManager]: No Valid Celestial Bodies on ROC Definition " + d.type + ". Removed entry.");
            _rocDefinitions.erase(_rocDefinitions.begin() + num);
        }
    }
}

// ROCManager.cs:297
CelestialBody* ROCManager::ValidCelestialBody(const std::string& bodyName) {
    const std::vector<CelestialBody*> bodies = LocalBodies();
    for (size_t num = 0; num < bodies.size(); num++)
        if (bodies[num]->name() == bodyName) return bodies[num];
    return nullptr;
}

// ROCManager.cs:316
bool ROCManager::ValidCBBiome(CelestialBody* body, const std::string& biomeName) {
    if (!body->BiomeMap) throw NullReferenceException();
    for (size_t num = 0; num < body->BiomeMap->Attributes.size(); num++)
        if (body->BiomeMap->Attributes[num].name == biomeName) return true;
    return false;
}

// ROCManager.cs:335
void ROCManager::DisableAllROCs() {
    for (size_t i = 0; i < _pqsRocControls.size(); i++) {
        _pqsRocControls[i]->modEnabled = false;
        if (_pqsRocControls[i]->sphere != nullptr) ++RocEnvironment::resetModListCalls;
    }
}

// ROCManager.cs:347
void ROCManager::EnableAllROCs() {
    for (size_t i = 0; i < _pqsRocControls.size(); i++) {
        _pqsRocControls[i]->modEnabled = true;
        if (_pqsRocControls[i]->sphere != nullptr && _pqsRocControls[i]->sphereView.isStarted) ++RocEnvironment::resetModListCalls;
    }
}

// ROCManager.cs:384
bool ROCManager::ROCRemoved(int rocId) { return ROCManagerRemovedROCs::ROCRemoved(rocId); }

// ROCManager.cs:407
void ROCManager::RemoveROC(int rocId) {
    if (!hasList) {
        removedROCs.clear();
        hasList = true;
    }
    AddUnique(removedROCs, rocId);
}

// ROCManager.cs:416
void ROCManager::ClearROCsCache() {
    for (size_t i = 0; i < _pqsRocControls.size(); i++)
        for (size_t j = 0; j < _pqsRocControls[i]->rocs.size(); j++) _pqsRocControls[i]->rocs[j]->ClearCache();
}

// ROCManager.cs:440
void ROCManager::AddROCStats_Quad(PQSROCControl* control, ROCDefinition* r, const RocQuad&) {
    if (!rocStats.ContainsKey(r->type)) {
        rocStats.Add(r->type, std::make_shared<ROCStats>(r->type));
        rocStats[r->type]->rocTypeFrequency = r->frequency;
    }
    rocStats[r->type]->activeQuads++;
    rocStats[r->type]->activeQuadArea = f32store(fx(rocStats[r->type]->activeQuadArea) + fx(control->quadArea));
    if (OnStatsChanged) OnStatsChanged();
}

// ROCManager.cs:455
void ROCManager::AddROCStats_ROCs(const std::string& rType, int count, float frequency) {
    if (!rocStats.ContainsKey(rType)) {
        rocStats.Add(rType, std::make_shared<ROCStats>(rType));
        rocStats[rType]->rocTypeFrequency = frequency;
    }
    rocStats[rType]->activeRocCount += count;
    if (OnStatsChanged) OnStatsChanged();
}

// ROCManager.cs:469
void ROCManager::ClearStats() { rocStats.Clear(); }

// ROCManager.cs:474
void ROCManager::SubtractROCStats_Quad(PQSROCControl* control, ROCDefinition* r) {
    if (rocStats.ContainsKey(r->type)) {
        rocStats[r->type]->activeQuads--;
        rocStats[r->type]->activeQuadArea = f32store(fx(rocStats[r->type]->activeQuadArea) - fx(control->quadArea));
        if (OnStatsChanged) OnStatsChanged();
    }
}

// ROCManager.cs:487
void ROCManager::SubtractROCStats_ROCs(const std::string& rType, int count) {
    if (rocStats.ContainsKey(rType)) {
        rocStats[rType]->activeRocCount -= count;
        if (OnStatsChanged) OnStatsChanged();
    }
}

// =============================================================================================================================
// ROCScenario

ROCScenario::ROCScenario() : ScenarioModule("ROCScenario") {}

// ROCScenario.cs:14 (sic: Instance is only ever assigned null)
void ROCScenario::Start() {
    if (!SerenityInstalled()) {
        destroyed = true;
        Instance = nullptr;
    }
}

void ROCScenario::Register() {
    ScenarioModuleFactory::Register("ROCScenario", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new ROCScenario()); });
}

}  // namespace ksp
