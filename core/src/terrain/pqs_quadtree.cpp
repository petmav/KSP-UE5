// WP-12a: PQS sphere lifecycle, target-driven LOD update and mod dispatch (Assembly-CSharp/PQS.cs).
// See include/ksp/terrain/pqs.hpp. Unity-only statements dropped (and noted where they sit): materials / shadows /
// shader quality (ApplyTerrainShaderSettings :656, AssignQuad's renderer setup), LocalSpacePQStorage and the floating-
// origin quad moves (:631-642, :1129-1175), Debug.Log, CreateMaps (:919, editor map baking), the BilinearInterp*
// texture samplers (:2419-2469).
#include <algorithm>
#include <exception>

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/terrain/pqs_mods.hpp"
#include "ksp/terrain/pqs_parallel.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace M = dotnet::Math;

PQS_GameBindings& PQS::GameBindings() {
    static PQS_GameBindings bindings;  // PQS.cs:151 (static)
    return bindings;
}

PQS::~PQS() {
    // PQS.cs:647 OnDestroy -> ResetSphere when a PQSCache exists: return the quads to the pool
    if (cache != nullptr && !quads.empty()) {
        try {
            for (int i = 0; i < static_cast<int>(quads.size()); i++)
                if (quads[static_cast<size_t>(i)] != nullptr) quads[static_cast<size_t>(i)]->Destroy();
        } catch (...) {
        }
        quads.clear();
    }
}

PQSMeshCache& PQS::meshCache() const {
    if (cache == nullptr) throw NullReferenceException("PQS.cache (PQSCache.Instance) is not set");
    return cache->meshCache;
}

// PQS.cs:591 Start
void PQS::Start() {
    if (cache == nullptr) return;  // Debug.Log("No PQS cache found")
    if (!cache->meshCache.created) cache->meshCache.CreateCache();  // if (cacheMesh == null) CreateCache()
    if (!cache->cacheReady) cache->CreateCache();
    if (GameBindings().SettingsReady()) {
        Global_ForceShaderModel = GameBindings().PLANET_FORCE_SHADER_MODEL_2_0();
        Global_AllowScatter = GameBindings().PLANET_SCATTER();
        Global_ScatterFactor = Mathf::Clamp01(GameBindings().PLANET_SCATTER_FACTOR());
    }
    // ApplyTerrainShaderSettings(): material selection (renderer side)
    events::GameEvents::OnPQSStarting.Fire(this);
    if (!isStarted) StartSphere(false);  // else Debug.LogError("... already marked as started ...")
    // LocalSpacePQStorage (floating origin): dropped
    savemaxFrameTime_ = maxFrameTime;
    // GameEvents.OnGameSettingsApplied.Add(OnGameSettingsApplied): shader settings only
}

// PQS.cs:717 StartSphere
void PQS::StartSphere(bool force) {
    if (isStarted && (!force || !GameBindings().LoadedSceneIsGame())) return;
    // cache = FindObjectOfType(PQSCache); parentSphere = GetComponentOnParent<PQS>(): host-set members
    if (parentSphere != nullptr && (!parentSphere->isStarted || !parentSphere->isAlive)) return;
    normalUpdateList.clear();  // new List<PQ>()
    vbData_.Reset();
    targetPositionPrev_ = Vector3d::down();
    pqID_ = 0;
    fixedUpdateFrame_ = 0;
    if (PQSCache::PresetList) {
        const PQSCache::PQSSpherePreset* preset = PQSCache::PresetList->GetPreset(name);
        if (preset != nullptr) {
            minDetailDistance = preset->minDistance;
            minLevel = preset->minSubdivision;
            maxLevel = preset->maxSubdivision;
        }
    }
    minLevel = Mathf::Min(minLevel, maxLevel - 1);
    minLevel = Mathf::Max(minLevel, 2);
    maxLevel = Mathf::Max(maxLevel, minLevel + 1);
    maxLevel1_ = maxLevel + 1;
    maxLevelAtCurrentTgtSpeed = maxLevel;
    subdivisionThresholds.assign(static_cast<size_t>(maxLevel1_), 0.0);
    collapseThresholds.assign(static_cast<size_t>(maxLevel1_), 0.0);
    quadCounts.assign(static_cast<size_t>(maxLevel1_), 0);
    for (int itr = 0; itr < maxLevel1_; itr++)
        subdivisionThresholds[static_cast<size_t>(itr)] =
            radius * minDetailDistance / static_cast<double>(Mathf::Pow(2.0f, static_cast<float>(itr))) * subdivisionThreshold;
    collapseDelta = (collapseAltitudeValue - collapseSeaLevelValue) / collapseAltitudeMax;
    visRadDelta = (visRadAltitudeValue - visRadSeaLevelValue) / visRadAltitudeMax;
    circumference = 6.2831854820251465 * radius;
    radiusSquared = radius * radius;
    SetupMods();
    SetupBuildDelegates(false);
    isActive = false;
    isAlive = false;
    isThinking = false;
    isStarted = true;
    isDisabled = false;
    if (force) {
        isAlive = true;
    } else {
        Mod_OnSphereStart();
    }
    UpdateVisual();
    if (!isAlive || target == nullptr) return;
    UpdateQuadsInit();
    Mod_OnPostSetup();
    Mod_OnSphereStarted();
    int num = static_cast<int>(childSpheres.size());
    while (num-- > 0) {
        PQS* c = childSpheres[static_cast<size_t>(num)];
        if (!c->isStarted || force) c->StartSphere(force);
    }
    // OnReady.Fire(this): EventData<PQS> of this sphere (no listener in the core)
    StartUpdateSphereRoutine();
}

// PQS.cs:796 ResetSphere
void PQS::ResetSphere() {
    if (isAlive || isStarted) {
        if (quads.size() == 6) {
            for (int i = 0; i < 6; i++) {
                if (quads[static_cast<size_t>(i)] != nullptr) {
                    quads[static_cast<size_t>(i)]->Destroy();
                    quads[static_cast<size_t>(i)] = nullptr;
                }
            }
            quads.clear();
        }
        Mod_OnSphereReset();
        normalUpdateList.clear();
        quadCount = 0;
        isAlive = false;
        isStarted = false;
        int num = static_cast<int>(childSpheres.size());
        while (num-- > 0) childSpheres[static_cast<size_t>(num)]->ResetSphere();
    }
    ResetLaunchsitePlacementRender();
    // StopCoroutine("UpdateSphere") / StopCoroutine("ResetAndWaitCoroutine"): the coroutines were started with
    // StartCoroutine(IEnumerator), which StopCoroutine(string) does not stop (Unity's documented rule); a running
    // UpdateSphere loop ends when it next sees isAlive == false. Kept as is (see FrameUpdate).
}

// PQS.cs:832
void PQS::RebuildSphere() {
    ResetSphere();
    StartSphere(false);
}

// PQS.cs:838
void PQS::ForceStart() {
    if (parentSphere == nullptr) StartSphere(true);  // StopCoroutine("ResetAndWaitCoroutine"): no effect (see ResetSphere)
}

// PQS.cs:847
void PQS::StartUpSphere() { StartSphere(true); }

// PQS.cs:852 ResetAndWait + :861 ResetAndWaitCoroutine (its first step runs inside StartCoroutine)
void PQS::ResetAndWait() {
    if (parentSphere != nullptr) return;
    ResetSphere();
    try {
        Mod_OnSphereStart();
        if (!isAlive && !isDisabled) {
            routines_.push_back(Routine::ResetAndWait);
            routineResumeAt_.push_back(0.0f);
        } else {
            StartSphere(false);
        }
    } catch (const std::exception&) {
        // Unity logs a coroutine exception and drops the coroutine
    }
}

// PQS.cs:872
void PQS::SetupExternalRender() {
    externalRenderScatter_ = Global_AllowScatter;
    Global_AllowScatter = false;
    isBuildingMaps = true;
    SetupMods();
    SetupBuildDelegates(true);
}

// PQS.cs:881
void PQS::SetupLaunchsitePlacementRender() {
    SetupMods();
    if (landControl != nullptr) {
        static_cast<PQSLandControl*>(landControl)->createColors = false;
        static_cast<PQSLandControl*>(landControl)->createScatter = false;
    }
    if (aerialPerspectiveMaterial != nullptr) aerialPerspectiveMaterial->modEnabled = false;
    savemaxFrameTime_ = maxFrameTime;
    maxFrameTime = 240.0f;
    SetupBuildDelegates(false);
}

// PQS.cs:898
void PQS::ResetLaunchsitePlacementRender() {
    if (landControl != nullptr) {
        static_cast<PQSLandControl*>(landControl)->createColors = true;
        static_cast<PQSLandControl*>(landControl)->createScatter = true;
    }
    if (aerialPerspectiveMaterial != nullptr) aerialPerspectiveMaterial->modEnabled = true;
    maxFrameTime = savemaxFrameTime_;
    SetupBuildDelegates(false);
}

// PQS.cs:913
void PQS::CloseExternalRender() {
    isBuildingMaps = false;
    Global_AllowScatter = externalRenderScatter_;
}

// PQS.cs:991 AssignQuad (renderer material / shadow / reflection-probe setup dropped)
PQ* PQS::AssignQuad(int subdiv) {
    if (cache == nullptr) throw NullReferenceException("PQS.cache");
    PQ* quad = cache->GetQuad();
    quad->sphereRoot = this;
    quad->subdivision = subdiv;
    quad->subdivideThresholdFactor = 1.0;
    quad->quadRoot = nullptr;
    quad->id = pqID_++;
    quad->isActive = true;
    quad->isForcedInvisible = false;
    return quad;
}

// PQS.cs:1026
void PQS::QuadCreated(PQ& quad) { Mod_OnQuadCreate(quad); }

// PQS.cs:1031 DestroyQuad (renderer material release dropped)
void PQS::DestroyQuad(PQ& quad) {
    Mod_OnQuadDestroy(quad);
    cache->DestroyQuad(quad);
}

// PQS.cs:1055 CreateQuads (QI order xPos, xNeg, yPos, yNeg, zPos, zNeg)
void PQS::CreateQuads() {
    if (quads.size() == 6) {
        for (int i = 0; i < 6; i++)
            if (quads[static_cast<size_t>(i)] != nullptr) quads[static_cast<size_t>(i)]->Destroy();
    }
    quads.assign(6, nullptr);
    for (int num = 5; num >= 0; num--) quads[static_cast<size_t>(num)] = AssignQuad(0);
    PQ* xPos = quads[0];
    PQ* xNeg = quads[1];
    PQ* yPos = quads[2];
    PQ* yNeg = quads[3];
    PQ* zPos = quads[4];
    PQ* zNeg = quads[5];
    SetupQuad(*xPos, name + " Xp", QuadPlane::XP, zPos, zNeg, yNeg, yPos, Vector3d::right());
    SetupQuad(*xNeg, name + " Xn", QuadPlane::XN, zPos, zNeg, yPos, yNeg, Vector3d::left());
    SetupQuad(*yPos, name + " Yp", QuadPlane::YP, zPos, zNeg, xPos, xNeg, Vector3d::up());
    SetupQuad(*yNeg, name + " Yn", QuadPlane::YN, zNeg, zPos, xPos, xNeg, Vector3d::down());
    SetupQuad(*zPos, name + " Zp", QuadPlane::ZP, yNeg, yPos, xPos, xNeg, Vector3d::forward());
    SetupQuad(*zNeg, name + " Zn", QuadPlane::ZN, yPos, yNeg, xPos, xNeg, Vector3d::back());
    quadCount += 6;
    for (int num2 = 5; num2 >= 0; num2--) {
        quads[static_cast<size_t>(num2)]->SetupQuad(nullptr, PQ::QuadChild::Null);
        QuadCreated(*quads[static_cast<size_t>(num2)]);
    }
}

// PQS.cs:1086 SetupQuad (the root quads)
void PQS::SetupQuad(PQ& quad, const std::string& quadName, QuadPlane plane, PQ* north, PQ* south, PQ* east, PQ* west,
                    const Vector3d& pos) {
    quad.scalePlaneRelative = 1.0;
    quad.scalePlanetRelative = radius;
    quad.positionPlanePosition = pos.normalized();
    quad.positionParentRelative.Zero();
    quad.positionPlanetRelative = quad.positionPlanePosition;
    quad.positionPlanet = quad.positionPlanetRelative * radius;
    quad.plane = plane;
    quad.name = quadName;
    quad.north = north;
    quad.south = south;
    quad.east = east;
    quad.west = west;
    quad.uvSW = Vector2f::zero();
    quad.uvDelta = Vector2f(1.0f, 1.0f);
}

// (PQS.cs:1104 UpdateSphere) the loop body
void PQS::UpdateSphereIteration() {
    if (!isDisabled) {
        Mod_OnPreUpdate();
        UpdateVisual();
        if (isAlive && isActive && target != nullptr) {
            UpdateQuads();
            Mod_OnUpdateFinished();
        }
    }
}

// StartCoroutine(UpdateSphere()): `while (isAlive) { ...; yield }` runs its first pass inside StartCoroutine
void PQS::StartUpdateSphereRoutine() {
    if (!isAlive) return;
    try {
        UpdateSphereIteration();
    } catch (const std::exception&) {
        return;  // Unity logs the exception and drops the coroutine
    }
    routines_.push_back(Routine::UpdateSphere);
    routineResumeAt_.push_back(frameTimeDelta > 0.0f ? Now() + frameTimeDelta : 0.0f);
}

// One frame: MonoBehaviour.Update (PQS.cs:1469), then the `yield return null` / WaitForSeconds coroutines in the order
// they yielded (Unity resumes them after Update; a coroutine started during this frame waits for the next one).
void PQS::FrameUpdate() {
    // PQS.cs:1469 Update: a sphere left "thinking" (an exception inside UpdateQuads) is rebuilt
    if (isAlive && isStarted && isThinking) RebuildSphere();  // + Debug.LogError("PQS ...: Restarted")
    std::vector<Routine> current;
    std::vector<float> resumeAt;
    current.swap(routines_);
    resumeAt.swap(routineResumeAt_);
    for (size_t i = 0; i < current.size(); i++) {
        if (current[i] == Routine::UpdateSphere) {
            if (frameTimeDelta > 0.0f && Now() < resumeAt[i]) {  // WaitForSeconds(frameTimeDelta) not elapsed yet
                routines_.push_back(Routine::UpdateSphere);
                routineResumeAt_.push_back(resumeAt[i]);
                continue;
            }
            if (!isAlive) continue;  // while (isAlive): the loop ends
            try {
                UpdateSphereIteration();
            } catch (const std::exception&) {
                continue;  // the coroutine dies (isThinking stays set: the next Update rebuilds the sphere)
            }
            routines_.push_back(Routine::UpdateSphere);
            routineResumeAt_.push_back(frameTimeDelta > 0.0f ? Now() + frameTimeDelta : 0.0f);
        } else {
            try {
                Mod_OnSphereStart();
                if (!isAlive && !isDisabled) {
                    routines_.push_back(Routine::ResetAndWait);
                    routineResumeAt_.push_back(0.0f);
                } else {
                    StartSphere(false);
                }
            } catch (const std::exception&) {
            }
        }
    }
}

// PQS.cs:1177
void PQS::EnableSphere() {
    isDisabled = false;
    ResetAndWait();
}

// PQS.cs:1183 ActivateSphere
void PQS::ActivateSphere() {
    if (isActive) return;
    if (!isAlive) {
        if (!isStarted) {
            StartSphere(false);
            if (!isAlive) {
                UpdateQuadsInit();
                isAlive = true;
            }
        } else {
            UpdateQuadsInit();
            isAlive = true;
        }
    }
    isActive = true;
    int num = static_cast<int>(childSpheres.size());
    while (num-- > 0) childSpheres[static_cast<size_t>(num)]->ActivateSphere();
    Mod_OnActive();
    SetVisible(true);
}

// PQS.cs:1216
void PQS::DisableSphere() {
    isDisabled = true;
    Mod_OnInactive();
    ResetSphere();
}

// PQS.cs:1223
void PQS::DeactivateSphere() {
    if (isActive) {
        isActive = false;
        Mod_OnInactive();
        int num = static_cast<int>(childSpheres.size());
        while (num-- > 0) childSpheres[static_cast<size_t>(num)]->DeactivateSphere();
        SetVisible(false);
    }
}

// PQS.cs:1238 SetVisible (NullReferenceException when the quads were never created, as in C#)
void PQS::SetVisible(bool visible) {
    if (quads.size() != 6) throw NullReferenceException("PQS.quads");
    if (visible) {
        for (int num = 5; num >= 0; num--) quads[static_cast<size_t>(num)]->SetMasterVisible();
    } else {
        for (int num2 = 5; num2 >= 0; num2--) quads[static_cast<size_t>(num2)]->SetMasterInvisible();
    }
    int num3 = static_cast<int>(childSpheres.size());
    while (num3-- > 0) childSpheres[static_cast<size_t>(num3)]->SetVisible(visible);
}

// PQS.cs:1261
void PQS::SetTarget(const PQSTarget* t) {
    target = t;
    int num = static_cast<int>(childSpheres.size());
    while (num-- > 0) childSpheres[static_cast<size_t>(num)]->SetTarget(t);
}

// PQS.cs:1271
void PQS::SetSecondaryTarget(const PQSTarget* t) {
    secondaryTarget = t;
    int num = static_cast<int>(childSpheres.size());
    while (num-- > 0) childSpheres[static_cast<size_t>(num)]->SetSecondaryTarget(t);
}

// PQS.cs:1281 UpdateVisual. relativeTargetPosition = transform.InverseTransformPoint(target.position) in C# (float
// world space); here the target is already in the sphere's body-fixed frame (double).
void PQS::UpdateVisual() {
    if (target == nullptr) return;
    targetPosition_ = target->position;
    relativeTargetPosition = targetPosition_;
    relativeTargetPositionNormalized = relativeTargetPosition.normalized();
    if (fixedUpdateFrame_ != 0) {
        // two reads of Time.realtimeSinceStartup, as in the C#
        fixedFrameTime_ = f32store(fx(Now()) - fx(prevFixedFrameTime_));
        prevFixedFrameTime_ = Now();
        fixedUpdateFrame_ = 0;
        targetVelocity = (relativeTargetPosition - targetPositionPrev_) / static_cast<double>(fixedFrameTime_);
        targetSpeed = targetVelocity.magnitude();
        targetPositionPrev_ = relativeTargetPosition;
        angularTargetSpeed_ = static_cast<float>(M::Acos(Vector3d::Dot(relativeTargetPositionNormalized, lastRelTgtPosNormalized_)));
        lastRelTgtPosNormalized_ = relativeTargetPositionNormalized;
        if (angularTargetSpeed_ != angularTargetSpeed_) angularTargetSpeed_ = 0.0f;  // float.IsNaN
    }
    if (maxQuadLenghtsPerFrame > 0.0f) {
        maxLevelAtCurrentTgtSpeed = minLevel;
        int itr = maxLevel;
        while (itr >= minLevel) {
            // float expression compared in double (Mono rule); Mathf.Pow's float return is a store
            if (fx(angularTargetSpeed_) >=
                fx(1.5707963f) / fx(Mathf::Pow(2.0f, static_cast<float>(itr))) * fx(maxQuadLenghtsPerFrame)) {
                itr--;
                continue;
            }
            maxLevelAtCurrentTgtSpeed = itr;
            break;
        }
    } else {
        maxLevelAtCurrentTgtSpeed = maxLevel;
    }
    targetDistance_ = relativeTargetPosition.magnitude();
    targetAltitude = targetDistance_ - radius;
    visibleAltitude = targetAltitude;
    relativeTargetPosition = relativeTargetPositionNormalized * radius;
    targetHeight = targetDistance_ - GetSurfaceHeight(relativeTargetPositionNormalized);
    if (secondaryTarget != nullptr) {
        relativeSecondaryTargetPosition = secondaryTarget->position;
        targetSecondaryAltitude = relativeSecondaryTargetPosition.magnitude() - radius;
        visibleAltitude = M::Max(targetAltitude, targetSecondaryAltitude);
    }
    const double num = radius + visibleAltitude;
    horizonDistance = M::Sqrt(num * num - radiusMin * radiusMin) + halfChord;
    horizonAngle = M::Pow(M::Cos(radius / (radius + visibleAltitude)), -1.0);
    visRad = visRadSeaLevelValue + visibleAltitude * visRadDelta;
    if (visRad < visRadAltitudeValue) {
        visRad = visRadAltitudeValue;
    } else if (visRad > visRadSeaLevelValue) {
        visRad = visRadSeaLevelValue;
    }
    visibleRadius = horizonDistance * M::Sin(horizonAngle) * visRad;
    for (int itr = 0; itr < maxLevel1_; itr++) {
        collapseThreshold = collapseSeaLevelValue + targetAltitude * collapseDelta;
        if (collapseThreshold < collapseSeaLevelValue) {
            collapseThreshold = collapseSeaLevelValue;
        } else if (collapseThreshold > collapseAltitudeValue) {
            collapseThreshold = collapseAltitudeValue;
        }
        collapseThresholds[static_cast<size_t>(itr)] = subdivisionThresholds[static_cast<size_t>(itr)] * collapseThreshold;
    }
    if (targetAltitude > maxDetailDistance * radius) {
        DisableSubdivision();
    } else {
        EnableSubdivision();
    }
}

// PQS.cs:1371
void PQS::DisableSubdivision() {
    if (isSubdivisionEnabled) {
        UpdateQuads();
        isSubdivisionEnabled = false;
    }
}

// PQS.cs:1380
void PQS::EnableSubdivision() {
    if (!isSubdivisionEnabled) isSubdivisionEnabled = true;
}

// PQS.cs:1388 UpdateQuadsInit (quadCount is only ever 6, so the loop always runs its 10 passes)
void PQS::UpdateQuadsInit() {
    CreateQuads();
    isThinking = true;
    int num = -1;
    quadAllowBuild = false;
    const int num2 = 10;
    for (int num3 = 0; num3 < num2; num3 = (num >= quadCount) ? (num3 + 1) : 0) {
        num = quadCount;
        for (int num4 = 5; num4 >= 0; num4--) quads[static_cast<size_t>(num4)]->UpdateSubdivisionInit();
    }
    if (heightWorkers != nullptr) PrefetchVisibleLeaves();  // WP-12a optimisation
    quadAllowBuild = true;
    for (int num5 = 5; num5 >= 0; num5--) quads[static_cast<size_t>(num5)]->UpdateSubdivision();
    if (heightWorkers != nullptr) heightWorkers->Clear();
    isThinking = false;
}

// WP-12a optimisation (pqs_parallel.hpp): the leaves the building pass of UpdateQuadsInit will build (PQ.UpdateSubdivision
// -> UpdateVisibility for a leaf at or above minLevel with gcd1 < visibleRadius; gcd1 is recomputed from the same inputs)
void PQS::PrefetchVisibleLeaves() {
    std::vector<std::pair<PQ*, Matrix4x4f>> jobs;
    std::vector<PQ*> stack(quads.begin(), quads.end());
    while (!stack.empty()) {
        PQ* q = stack.back();
        stack.pop_back();
        if (q->isSubdivided) {
            for (PQ* c : q->subNodes)
                if (c != nullptr) stack.push_back(c);
            continue;
        }
        if (q->isActive && !q->isBuilt && !q->isCached && q->subdivision >= minLevel && q->gcd1 < visibleRadius)
            jobs.emplace_back(q, q->quadMatrix);
    }
    heightWorkers->Prefetch(jobs, meshCache());
}

// PQS.cs:1411 UpdateQuads: insertion sort of the six roots by distance to the target, then only the first FIVE are
// updated (Mathf.Min(quads.Length, 5)); custom normals are stitched afterwards
void PQS::UpdateQuads() {
    if (quads.empty()) return;
    isThinking = true;
    maxFrameEnd = f32store(fx(Now()) + fx(maxFrameTime));
    const int num = static_cast<int>(quads.size());
    for (int i = 1; i < num; i++) {
        PQ* pQ = quads[static_cast<size_t>(i)];
        int num2 = i - 1;
        while (num2 >= 0 && !((relativeTargetPosition - quads[static_cast<size_t>(num2)]->positionPlanetRelative).sqrMagnitude() <=
                               (relativeTargetPosition - pQ->positionPlanetRelative).sqrMagnitude())) {
            quads[static_cast<size_t>(num2 + 1)] = quads[static_cast<size_t>(num2)];
            num2--;
        }
        quads[static_cast<size_t>(num2 + 1)] = pQ;
    }
    for (int j = 0; j < Mathf::Min(num, 5); j++) quads[static_cast<size_t>(j)]->UpdateSubdivision();
    if (reqCustomNormals) UpdateEdges();
    if (heightWorkers != nullptr) heightWorkers->Clear();
    isThinking = false;
}

// PQS.cs:1442 UpdateEdges (List: processed from the end; UpdateEdgeNormals may append)
void PQS::UpdateEdges() {
    int count;
    while ((count = static_cast<int>(normalUpdateList.size())) > 0) {
        count--;
        PQ* pQ = normalUpdateList[static_cast<size_t>(count)];
        normalUpdateList.erase(normalUpdateList.begin() + count);
        if (pQ != nullptr) {
            if (!pQ->isSubdivided && pQ->isVisible) UpdateEdgeNormals(*pQ);
            pQ->isQueuedForNormalUpdate = false;
            pQ->isQueuedOnlyForCornerNormalUpdate = false;
        }
    }
}

// PQS.cs:1462 FixedUpdate
void PQS::FixedUpdate() {
    fixedUpdateFrame_++;
    Mod_OnSphereTransformUpdate();
    transformRotationInverse_ = QuaternionD::Inverse(transformRotation);
}

// PQS.cs:1478
void PQS::SetCache() {
    if (hasCache_) ClearCache();
    for (int i = 0; i < 6; i++) quads.at(static_cast<size_t>(i))->SetCache();
    hasCache_ = true;
}

// PQS.cs:1491
void PQS::ClearCache() {
    for (int i = 0; i < 6; i++) quads.at(static_cast<size_t>(i))->ClearCache();
    hasCache_ = false;
}

// ---- mod dispatch ------------------------------------------------------------------------------------------------------

// PQS.cs:1835
void PQS::Mod_OnPostSetup() {
    for (PQSMod* m : mods) m->OnPostSetup();
}

// PQS.cs:1843
void PQS::Mod_OnSphereReset() {
    for (PQSMod* m : mods) m->OnSphereReset();
}

// PQS.cs:1851 Mod_OnSphereStart (no mod handles the start: the sphere is alive, and takes its parent's or the main
// camera's target)
void PQS::Mod_OnSphereStart() {
    int num = 0;
    for (size_t i = 0; i < mods.size(); i++)
        if (mods[i]->OnSphereStart()) num++;
    if (num == 0) {
        isAlive = true;
        if (target == nullptr && parentSphere != nullptr && parentSphere->target != nullptr) {
            SetTarget(parentSphere->target);
            SetSecondaryTarget(parentSphere->secondaryTarget);
        }
        if (target == nullptr && mainCamera) {
            const PQSTarget* cam = mainCamera();
            if (cam != nullptr) SetTarget(cam);
        }
    }
}

// PQS.cs:1876
void PQS::Mod_OnSphereStarted() {
    for (PQSMod* m : mods) m->OnSphereStarted();
}

// PQS.cs:1884
void PQS::Mod_OnSphereTransformUpdate() {
    for (PQSMod* m : mods) m->OnSphereTransformUpdate();
}

// PQS.cs:1912
void PQS::Mod_OnPreUpdate() {
    for (size_t i = 0; i < mods.size(); i++) mods[i]->OnPreUpdate();
}

// PQS.cs:1920 (a destroyed mod would trigger ResetModList; mods are never destroyed here)
void PQS::Mod_OnUpdateFinished() {
    for (PQSMod* m : mods) m->OnUpdateFinished();
}

// PQS.cs:1940
void PQS::Mod_OnActive() {
    for (PQSMod* m : mods) m->OnSphereActive();
}

// PQS.cs:1948
void PQS::Mod_OnInactive() {
    for (PQSMod* m : mods) m->OnSphereInactive();
}

// PQS.cs:2061
void PQS::Mod_OnQuadCreate(PQ& quad) {
    for (PQSMod* m : mods) m->OnQuadCreate(quad);
}

// PQS.cs:2069
void PQS::Mod_OnQuadDestroy(PQ& quad) {
    for (PQSMod* m : mods) m->OnQuadDestroy(quad);
}

// PQS.cs:2093
void PQS::Mod_OnQuadUpdate(PQ& quad) {
    for (PQSMod* m : mods) m->OnQuadUpdate(quad);
}

// PQS.cs:2101
void PQS::Mod_OnQuadUpdateNormals(PQ& quad) {
    for (PQSMod* m : mods) m->OnQuadUpdateNormals(quad);
}

// ---- frame helpers -----------------------------------------------------------------------------------------------------

// PQS.cs:3042 (sic: rotation, not its inverse)
Vector3d PQS::GetRelativePosition(const Vector3d& worldPosition) const { return transformRotation * (worldPosition - transformPosition); }

// PQS.cs:3047
Vector3d PQS::GetRelativeDirection(const Vector3d& worldDirection) const { return transformRotationInverse_ * worldDirection; }

// PQS.cs:3052
Vector3d PQS::GetWorldPosition(const Vector3d& localPosition) const { return transformRotationInverse_ * localPosition + transformPosition; }

// PQS.cs:3103 GetAltitude -> GetRelativeDistance (:3057)
double PQS::GetAltitude(const Vector3d& worldPosition) const { return GetRelativePosition(worldPosition).magnitude() - radius; }

// PQS.cs:3133 LineSphereIntersection
bool PQS::LineSphereIntersection(const Vector3d& relPos, const Vector3d& relVel, double radius, Vector3d& relIntersection) {
    const double num = radius * radius;
    if (relPos.sqrMagnitude() <= num) {
        relIntersection = relPos;
        return true;
    }
    const Vector3d normalized = relVel.normalized();
    const double sqrMagnitude = normalized.sqrMagnitude();
    const double num2 = 2.0 * Vector3d::Dot(normalized, relPos);
    const double num3 = relPos.sqrMagnitude() - radius * radius;
    const double num4 = num2 * num2 - 4.0 * sqrMagnitude * num3;
    if (num4 < 0.0) {
        relIntersection = Vector3d::zero();
        return false;
    }
    if (num4 < 1.401298464324817E-45) {
        const double num5 = (0.0 - num2) / (2.0 * sqrMagnitude);
        relIntersection = relPos + num5 * normalized;
        return true;
    }
    const double num6 = M::Sqrt(num4);
    const double num7 = (0.0 - num2 + num6) / (2.0 * sqrMagnitude);
    const double num8 = (0.0 - num2 - num6) / (2.0 * sqrMagnitude);
    if (num7 < num8) {
        relIntersection = relPos + num7 * normalized;
        return true;
    }
    relIntersection = relPos + num8 * normalized;
    return true;
}

}  // namespace ksp
