// WP-12d: the scatter half of PQSLandControl.cs (LandClassScatter, OnSetup's scatter matching, OnQuadPreBuild, the scatter branch of
// OnVertexBuild, OnQuadBuilt, OnSphereStarted / Reset, ApplyCastShadowsSetting) and the scheduling of PQSMod_LandClassScatterQuad.cs.
// See ksp/terrain/pqs_mods.hpp.
#include <algorithm>

#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/terrain/pqs_mods.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/misc_util.hpp"

namespace ksp {

namespace {
using GE = events::GameEvents;
using LCS = PQSLandControl::LandClassScatter;
using LCSQ = PQSLandControl::LandClassScatterQuad;
size_t Z(int i) { return static_cast<size_t>(i); }
}  // namespace

// ---- PQSMod_LandClassScatterQuad.cs ----------------------------------------------------------------------------------------------

// PQSMod_LandClassScatterQuad.cs:26 (Delegate.Combine of a null delegate is the handler itself: `+=`)
void LCSQ::Setup(PQ& q, int seed_, LandClassScatter* scatter_, int count_) {
    quad = &q;
    seed = seed_;
    count = count_;
    scatter = scatter_;
    name = "_" + q.name;
    isVisible = true;
    isBuilt = false;
    q.onVisible += PQ::QuadDelegate::Bind(this, &LCSQ::OnQuadVisible);
    q.onUpdate += PQ::QuadDelegate::Bind(this, &LCSQ::OnQuadUpdate);
    q.onInvisible += PQ::QuadDelegate::Bind(this, &LCSQ::OnQuadInvisible);
    q.onDestroy += PQ::QuadDelegate::Bind(this, &LCSQ::OnQuadDestroy);
    localPosition = q.sphereRoot != nullptr && q.sphereRoot->surfaceRelativeQuads ? q.positionPlanet : Vector3d::zero();
}

// PQSMod_LandClassScatterQuad.cs:79 (sic: isBuilt is only cleared when the object was visible)
void LCSQ::Destroy() {
    if (isVisible) {
        isVisible = false;
        if (isBuilt) {
            objActive = false;
            isBuilt = false;
        }
    }
    if (quad != nullptr) {
        quad->onVisible -= PQ::QuadDelegate::Bind(this, &LCSQ::OnQuadVisible);
        quad->onInvisible -= PQ::QuadDelegate::Bind(this, &LCSQ::OnQuadInvisible);
        quad->onDestroy -= PQ::QuadDelegate::Bind(this, &LCSQ::OnQuadDestroy);
        quad->onUpdate -= PQ::QuadDelegate::Bind(this, &LCSQ::OnQuadUpdate);
        quad = nullptr;
    }
}

// PQSMod_LandClassScatterQuad.cs:104
void LCSQ::OnQuadVisible(PQ&) {
    if (!isVisible) {
        isVisible = true;
        if (isBuilt) objActive = true;
    }
}

// PQSMod_LandClassScatterQuad.cs:116
void LCSQ::OnQuadInvisible(PQ&) {
    if (isVisible) {
        isVisible = false;
        if (isBuilt) objActive = false;
    }
}

// PQSMod_LandClassScatterQuad.cs:128
void LCSQ::OnQuadDestroy(PQ&) { scatter->DestroyQuad(*this); }

// PQSMod_LandClassScatterQuad.cs:133
void LCSQ::OnQuadUpdate(PQ& q) {
    if (!isBuilt && isVisible && q.sphereRoot->quadAllowBuild && q.sphereRoot->targetSpeed < scatter->maxSpeed) {
        scatter->CreateScatterMesh(*this);
        isBuilt = true;
        objActive = true;
        q.onUpdate -= PQ::QuadDelegate::Bind(this, &LCSQ::OnQuadUpdate);
    }
}

// ---- PQSLandControl.LandClassScatter ---------------------------------------------------------------------------------------------

// PQSLandControl.cs:246
void LCS::Setup(PQS* sphere_) {
    sphere = sphere_;
    minLevel_ = Mathf::Abs(maxLevelOffset) + sphere->maxLevel;
    if (!cacheCreated) {
        if (!hasBaseMesh)
            BuildCacheQuad();
        else
            BuildCacheMesh(baseMeshVertices, baseMeshUV, baseMeshNormals, baseMeshTriangles);
        cacheAssigned.clear();
        cacheAssigned.reserve(Z(std::max(maxCache, 0)));
        cacheAssignedCount = 0;
        cacheUnassigned.clear();
        cacheUnassignedCount = 0;
        cacheTotalCount = 0;
        cacheCreated = true;
    }
}

// PQSLandControl.cs:269
void LCS::SphereActive() {
    if (PQS::Global_AllowScatter && cacheUnassignedCount == 0) BuildCache(maxCacheDelta);
}

// PQSLandControl.cs:277 (sic: cacheAssigned keeps its destroyed entries; only the count is reset)
void LCS::SphereInactive() {
    while (cacheUnassignedCount > 0) {
        cacheUnassigned.back()->destroyed = true;
        cacheUnassigned.pop_back();
        cacheUnassignedCount--;
    }
    int index = cacheAssignedCount;
    while (index-- > 0) cacheAssigned.at(Z(index))->destroyed = true;
    cacheAssignedCount = 0;
}

// PQSLandControl.cs:292 ((int) of a double: conv.i4)
void LCS::AddScatterMeshController(PQ& quad, double density) {
    if (cacheUnassignedCount == 0) BuildCache(maxCacheDelta);
    scatterN = dotnet::ConvToInt32(density * densityFactor * (quad.quadArea / sphere->radius / 1000.0) * static_cast<double>(maxScatter));
    if (scatterN >= 1) {
        if (scatterN > maxScatter) scatterN = maxScatter;
        LandClassScatterQuad* qc = cacheUnassigned.back();
        cacheUnassigned.pop_back();
        cacheAssigned.push_back(qc);
        cacheUnassignedCount--;
        cacheAssignedCount++;
        const int hash = StringExtensions::GetHashCode_Net35(quad.name);
        qc->Setup(quad, static_cast<int>(static_cast<uint32_t>(seed) + static_cast<uint32_t>(hash)), this, scatterN);
        qc->density = density;
    }
}

// PQSLandControl.cs:313
void LCS::BuildCacheMesh(const std::vector<Vector3f>& vertBase, const std::vector<Vector2f>& uvBase, const std::vector<Vector3f>& normBase,
                         const std::vector<int>& triBase) {
    vertStride = static_cast<int>(vertBase.size());
    triStride = static_cast<int>(triBase.size());
    vertsUntrans = vertBase;
    normUntrans = normBase;
    normUntrans.resize(Z(vertStride));
    uvUntrans = uvBase;
    uvUntrans.resize(Z(vertStride));
    trisUntrans = triBase;
    std::vector<Vector2f> list;
    std::vector<int> list2;
    int num = 0;
    for (int i = 0; i < maxScatter; i++) {
        list.insert(list.end(), uvUntrans.begin(), uvUntrans.end());
        for (int j = 0; j < triStride; j++) list2.push_back(trisUntrans[Z(j)] + num);
        num += vertStride;
    }
    uvs = std::move(list);
    tris = std::move(list2);
    vertsEmpty.assign(Z(vertStride * maxScatter), Vector3f::zero());
    vertsTransformed.assign(Z(vertStride * maxScatter), Vector3f::zero());
    normalsEmpty.assign(Z(vertStride * maxScatter), Vector3f::zero());
    normalsTransformed.assign(Z(vertStride * maxScatter), Vector3f::zero());
}

// PQSLandControl.cs:345 (a unit quad, both faces)
void LCS::BuildCacheQuad() {
    const std::vector<Vector3f> vertBase = {Vector3f::zero(), Vector3f::right(), Vector3f::up(), Vector3f(1.0f, 1.0f, 0.0f),
                                            Vector3f::zero(), Vector3f::right(), Vector3f::up(), Vector3f(1.0f, 1.0f, 0.0f)};
    const std::vector<Vector2f> uvBase = {Vector2f(0.0f, 0.0f), Vector2f(1.0f, 0.0f), Vector2f(0.0f, 1.0f), Vector2f(1.0f, 1.0f),
                                          Vector2f(0.0f, 0.0f), Vector2f(1.0f, 0.0f), Vector2f(0.0f, 1.0f), Vector2f(1.0f, 1.0f)};
    std::vector<Vector3f> array(8);
    for (int i = 0; i < 4; i++) array[Z(i)] = Vector3f::forward();
    for (int i = 4; i < 8; i++) array[Z(i)] = Vector3f::back();
    BuildCacheMesh(vertBase, uvBase, array, {0, 1, 2, 2, 1, 3, 4, 6, 5, 6, 7, 5});
}

// PQSLandControl.cs:385 (the GameObjects, MeshFilter / MeshRenderer and the scatterParent transform are the engine's)
void LCS::BuildCache(int countToAdd) {
    scatterParentCreated = true;   // scatterParent "Scatter " + scatterName under sphere.transform
    for (int i = 0; i < countToAdd; i++) {
        auto q = std::make_unique<LandClassScatterQuad>();
        q->layer = 15;
        q->name = "Unass";
        q->mesh.vertices = vertsEmpty;
        q->mesh.normals = normalsEmpty;
        q->mesh.uv = uvs;
        q->triangles = tris;
        q->objActive = false;
        cacheUnassigned.push_back(q.get());
        objects.push_back(std::move(q));
    }
    cacheTotalCount += countToAdd;
    cacheUnassignedCount += countToAdd;
}

// PQSLandControl.cs:432
void LCS::DestroyQuad(LandClassScatterQuad& q) {
    q.Destroy();
    const auto it = std::find(cacheAssigned.begin(), cacheAssigned.end(), &q);
    if (it != cacheAssigned.end()) cacheAssigned.erase(it);
    cacheUnassigned.push_back(&q);
    q.name = "Unass";
    cacheAssignedCount--;
    cacheUnassignedCount++;
}

// PQSLandControl.cs:442 (managed Unity maths under the Mono float rule; Quaternion.AngleAxis native; the instances are recorded too)
void LCS::CreateScatterMesh(LandClassScatterQuad& q) {
    const PQSMeshCache& cache = sphere->meshCache();
    UnityRandom& random = UnityRandom::Global();
    int num = 0;
    random.InitState(q.seed);
    q.instances.clear();
    int scatterLoop = 0;
    for (scatterLoop = 0; scatterLoop < q.count; scatterLoop++) {
        int num2 = -1;
        int num3 = -1;
        while (num3 == num2) {
            const int num4 = random.Range(1, cache.cacheRes + 1);
            const int num5 = random.Range(1, cache.cacheRes + 1);
            const int x = num4 + random.Range(-1, 1);
            const int z = num5 + random.Range(-1, 1);
            num3 = cache.vi(num4, num5);
            num2 = cache.vi(x, z);
        }
        const Vector3f& a = q.quad->verts.at(Z(num3));
        const Vector3f& b = q.quad->verts.at(Z(num2));
        Vector3f scatterPos = Vector3f::Lerp(a, b, random.value());
        Vector3f scatterUp;
        if (sphere->surfaceRelativeQuads)
            scatterUp = (Vector3d(scatterPos) + q.quad->positionPlanet).normalized().ToVector3f();
        else
            scatterUp = scatterPos.normalized();
        scatterPos = scatterPos + scatterUp * verticalOffset;
        const float scatterAngle = f32store(fx(random.value()) * 360.0);
        const Quaternionf scatterRot = UnityNative::AngleAxis(scatterAngle, scatterUp) * q.quad->quadRotation.ToQuaternionf();
        const float scatterScale = random.Range(minScale, maxScale);
        q.instances.push_back({scatterPos, scatterUp, scatterAngle, scatterRot, scatterScale, num3, num2});
        int num6 = 0;
        while (num6 < vertStride) {
            vertsTransformed[Z(num)] = scatterPos + scatterRot * (vertsUntrans[Z(num6)] * scatterScale);
            normalsTransformed[Z(num)] = scatterRot * normUntrans[Z(num6)];
            num6++;
            num++;
        }
    }
    while (scatterLoop < maxScatter) {
        int num6 = 0;
        while (num6 < vertStride) {
            vertsTransformed[Z(num)] = Vector3f::zero();
            num6++;
            num++;
        }
        scatterLoop++;
    }
    q.mesh.vertices = vertsTransformed;
    q.mesh.normals = normalsTransformed;
    q.mesh.RecalculateBounds();
    q.objActive = true;
    q.isBuilt = true;
}

// PQSLandControl.cs:499
Vector3f LCS::RandomRange(const Vector3f& min, const Vector3f& max) {
    UnityRandom& r = UnityRandom::Global();
    const float x = r.Range(min.x, max.x);
    const float y = r.Range(min.y, max.y);
    const float z = r.Range(min.z, max.z);
    return Vector3f(x, y, z);
}

// PQSLandControl.cs:504
void LCS::ClearAll() {
    vertsTransformed.clear();
    vertsEmpty.clear();
    normalsTransformed.clear();
    normalsEmpty.clear();
    vertsUntrans.clear();
    uvUntrans.clear();
    trisUntrans.clear();
    normUntrans.clear();
    cacheUnassigned.clear();
    scatterParentCreated = false;   // Object.Destroy(scatterParent)
}

// ---- PQSLandControl ----------------------------------------------------------------------------------------------------------------

PQSLandControl::PQSLandControl() { className = "PQSLandControl"; }
PQSLandControl::~PQSLandControl() { OnDestroy(); }

// PQSLandControl.cs:617
void PQSLandControl::Start() {
    if (!listening_) GE::OnGameSettingsApplied.Add(this, &PQSLandControl::ApplyCastShadowsSetting);
    listening_ = true;
}

// PQSLandControl.cs:622
void PQSLandControl::OnDestroy() {
    if (listening_) GE::OnGameSettingsApplied.Remove(this, &PQSLandControl::ApplyCastShadowsSetting);
    listening_ = false;
}

// PQSLandControl.cs:627
void PQSLandControl::ApplyCastShadowsSetting() {
    if (scatters.empty()) return;
    for (LandClassScatter& s : scatters) {
        s.castShadows = GameSettings::CELESTIAL_BODIES_CAST_SHADOWS;
        s.recieveShadows = GameSettings::CELESTIAL_BODIES_CAST_SHADOWS;
    }
}

// PQSLandControl.cs:653
void PQSLandControl::ClearAll() {
    for (LandClassScatter& s : scatters) s.ClearAll();
}

// PQSLandControl.cs:726
void PQSLandControl::OnQuadPreBuild(PQ& quad) {
    scatterActive = quad.subdivision >= scatterMinSubdiv && createScatter && PQS::Global_AllowScatter;
}

// PQSLandControl.cs:809 (the scatter statements; the createColors statements are rendering and not ported)
void PQSLandControl::OnVertexBuild(PQSVertexBuildData& data) {
    for (int itr = 0; itr < lcListCount; itr++) {
        LandClass& lc = *lcList[Z(itr)];
        if (lc.delta > 0.0) {
            if (data.allowScatter && scatterActive && lc.delta > 0.05) {
                for (const LandClassScatterAmount& amount : lc.scatter) {
                    if (data.buildQuad->subdivision >= scatterMinSubdiv) {
                        lcScatterList.at(Z(amount.scatterIndex)) +=
                            amount.density * lc.delta * sphere->meshCache().cacheVertCountReciprocal * PQS::Global_ScatterFactor;
                        scatterInstCount++;
                    }
                }
            }
        }
    }
}

// PQSLandControl.cs:850
void PQSLandControl::OnQuadBuilt(PQ& quad) {
    if (!scatterActive || scatterInstCount <= 0) return;
    for (int itr = 0; itr < scatterCount; itr++) {
        if (lcScatterList[Z(itr)] > 0.0) {
            scatters[Z(itr)].AddScatterMeshController(quad, lcScatterList[Z(itr)]);
            lcScatterList[Z(itr)] = 0.0;
        }
    }
    scatterActive = false;
}

// PQSLandControl.cs:867
void PQSLandControl::OnSphereStarted() {
    for (LandClassScatter& s : scatters) s.SphereActive();
}

// PQSLandControl.cs:876
void PQSLandControl::OnSphereReset() {
    for (LandClassScatter& s : scatters) s.SphereInactive();
}

// PQSLandControl.cs:890
double PQSLandControl::CubicHermite(double start, double end, double startTangent, double endTangent, double t) {
    ct2 = t * t;
    ct3 = ct2 * t;
    return start * (2.0 * ct3 - 3.0 * ct2 + 1.0) + startTangent * (ct3 - 2.0 * ct2 + t) + end * (-2.0 * ct3 + 3.0 * ct2) + endTangent * (ct3 - ct2);
}

// PQSLandControl.cs:897
double PQSLandControl::Clamp(double v, double low, double high) {
    if (v < low) return low;
    if (v > high) return high;
    return v;
}

// PQSLandControl.cs:910
double PQSLandControl::DoubleLerp(double startStart, double startEnd, double endStart, double endEnd, double point) {
    if (!(point <= startStart) && point < endEnd) {
        if (point < startEnd) return (point - startStart) / (startEnd - startStart);
        if (point <= endStart) return 1.0;
        if (point < endEnd) return (point - endStart) / (endEnd - endStart);
        return 0.0;
    }
    return 0.0;
}

void PQSLandControl::CaptureBuildState(BuildState& out) const {
    out.clear();
    for (int i = 0; i < lcListCount; i++) {
        const LandClass* lc = lcList[Z(i)];
        out.emplace_back(static_cast<int>(lc - landClasses.data()), lc->delta);
    }
}

void PQSLandControl::RestoreBuildState(const BuildState& in) {
    lcList.clear();
    for (const auto& [index, delta] : in) {
        LandClass& lc = landClasses[Z(index)];
        lc.delta = delta;
        lcList.push_back(&lc);
    }
    lcListCount = static_cast<int>(lcList.size());
}

}  // namespace ksp
