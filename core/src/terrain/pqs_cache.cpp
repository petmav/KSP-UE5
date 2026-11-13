// WP-12a: PQS static mesh cache (PQS.cs) and the PQSCache quad pool + terrain presets (PQSCache.cs).
// See include/ksp/terrain/pqs_cache.hpp.
#include "ksp/terrain/pqs_cache.hpp"

#include <limits>
#include <stdexcept>

#include "ksp/config/config_node.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/unity/matrix_native.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

// ---- PQS.cs statics --------------------------------------------------------------------------------------------------

// PQS.cs:1500 LoadCacheSettings (int.Parse / float.Parse)
void PQSMeshCache::LoadCacheSettings(const ConfigNode& node) {
    if (node.HasValue("vertCount")) cacheSideVertCount = dotnet::ParseInt32(*node.GetValue("vertCount"));
    if (node.HasValue("cacheMeshSize")) cacheMeshSize = dotnet::ParseSingle(*node.GetValue("cacheMeshSize"));
    if (node.HasValue("cacheQuadSize")) {
        cacheQuadSize = dotnet::ParseSingle(*node.GetValue("cacheQuadSize"));
        cacheQuadSizeDiv2 = f32store(fx(cacheQuadSize) / 2.0);
    }
}

// PQS.cs:1517 CreateCache (managed float code: Mono rule)
void PQSMeshCache::CreateCache() {
    cacheRes = cacheSideVertCount - 1;
    cacheResDiv2 = cacheRes / 2;
    cacheResDiv2Plus1 = cacheResDiv2 + 1;
    cacheVertCount = cacheSideVertCount * cacheSideVertCount;
    cacheTriIndexCount = cacheRes * cacheRes * 6;
    cacheTriCount = cacheTriIndexCount / 3;
    const size_t n = static_cast<size_t>(cacheVertCount);
    cacheVerts.assign(n, Vector3f());
    std::vector<Vector3f> array(n);
    cacheUVs.assign(n, Vector2f());
    cacheUVQuad.assign(n, Vector2d());
    CreateCacheNormals();
    CreateCacheColors();
    CreateCacheUVExtras();
    const float num = f32store(fx(cacheMeshSize) / fx(static_cast<float>(cacheRes)));
    const float num2 = f32store(fx(cacheMeshSize) / fx(static_cast<float>(cacheRes)));
    const float num3 = f32store(1.0 / fx(static_cast<float>(cacheSideVertCount)));
    const float num4 = f32store(1.0 / fx(static_cast<float>(cacheSideVertCount)));
    const float num5 = f32store(fx(cacheMeshSize) / 2.0);
    const float num6 = f32store(fx(cacheMeshSize) / 2.0);
    Vector3f vector(0.0f, 1.0f, 0.0f);
    int i = 0;
    int num7 = 0;
    for (; i < cacheSideVertCount; i++) {
        int num8 = 0;
        while (num8 < cacheSideVertCount) {
            const size_t k = static_cast<size_t>(num7);
            cacheVerts[k] = Vector3f(f32store(fx(num5) - fx(static_cast<float>(num8)) * fx(num)), 0.0f,
                                     f32store(fx(num6) - fx(static_cast<float>(i)) * fx(num2)));
            cacheUVs[k] = Vector2f(f32store(fx(static_cast<float>(num8)) * fx(num3)), f32store(fx(static_cast<float>(i)) * fx(num4)));
            // (double)((float)num8 * num3): a conv.r8 of the float expression, not a store: no float rounding (Mono rule)
            cacheUVQuad[k] = Vector2d(1.0 - fx(static_cast<float>(num8)) * fx(num3), 1.0 - fx(static_cast<float>(i)) * fx(num4));
            array[k] = vector;
            num8++;
            num7++;
        }
    }
    CreateIndexCache();
    cacheTangents.assign(n, Vector4f());
    tan1.assign(n, Vector3f());
    tan2.assign(n, Vector3f());
    int num9 = 0;
    int num10 = 0;
    const std::vector<int>& idx = cacheIndices[0];
    while (num9 < cacheTriCount) {
        const int num11 = idx[static_cast<size_t>(num10)];
        const int num12 = idx[static_cast<size_t>(num10 + 1)];
        const int num13 = idx[static_cast<size_t>(num10 + 2)];
        const Vector3f vector2 = cacheVerts[static_cast<size_t>(num11)];
        const Vector3f vector3 = cacheVerts[static_cast<size_t>(num12)];
        const Vector3f vector4 = cacheVerts[static_cast<size_t>(num13)];
        // Vector2 -> Vector3 implicit conversion (z = 0)
        const Vector3f vector5(cacheUVs[static_cast<size_t>(num11)].x, cacheUVs[static_cast<size_t>(num11)].y, 0.0f);
        const Vector3f vector6(cacheUVs[static_cast<size_t>(num12)].x, cacheUVs[static_cast<size_t>(num12)].y, 0.0f);
        const Vector3f vector7(cacheUVs[static_cast<size_t>(num13)].x, cacheUVs[static_cast<size_t>(num13)].y, 0.0f);
        const float num14 = f32store(fx(vector3.x) - fx(vector2.x));
        const float num15 = f32store(fx(vector4.x) - fx(vector2.x));
        const float num16 = f32store(fx(vector3.y) - fx(vector2.y));
        const float num17 = f32store(fx(vector4.y) - fx(vector2.y));
        const float num18 = f32store(fx(vector3.z) - fx(vector2.z));
        const float num19 = f32store(fx(vector4.z) - fx(vector2.z));
        const float num20 = f32store(fx(vector6.x) - fx(vector5.x));
        const float num21 = f32store(fx(vector7.x) - fx(vector5.x));
        const float num22 = f32store(fx(vector6.y) - fx(vector5.y));
        const float num23 = f32store(fx(vector7.y) - fx(vector5.y));
        const float num24 = f32store(1.0 / (fx(num20) * fx(num23) - fx(num21) * fx(num22)));
        const Vector3f vector8(f32store((fx(num23) * fx(num14) - fx(num22) * fx(num15)) * fx(num24)),
                               f32store((fx(num23) * fx(num16) - fx(num22) * fx(num17)) * fx(num24)),
                               f32store((fx(num23) * fx(num18) - fx(num22) * fx(num19)) * fx(num24)));
        const Vector3f vector9(f32store((fx(num20) * fx(num15) - fx(num21) * fx(num14)) * fx(num24)),
                               f32store((fx(num20) * fx(num17) - fx(num21) * fx(num16)) * fx(num24)),
                               f32store((fx(num20) * fx(num19) - fx(num21) * fx(num18)) * fx(num24)));
        tan1[static_cast<size_t>(num11)] += vector8;
        tan1[static_cast<size_t>(num12)] += vector8;
        tan1[static_cast<size_t>(num13)] += vector8;
        tan2[static_cast<size_t>(num11)] += vector9;
        tan2[static_cast<size_t>(num12)] += vector9;
        tan2[static_cast<size_t>(num13)] += vector9;
        num9++;
        num10 += 3;
    }
    for (num9 = 0; num9 < cacheVertCount; num9++) {
        const size_t k = static_cast<size_t>(num9);
        vector = array[k];
        Vector3f tangent = tan1[k];
        UnityNative::OrthoNormalize(vector, tangent);
        cacheTangents[k].x = tangent.x;
        cacheTangents[k].y = tangent.y;
        cacheTangents[k].z = tangent.z;
        cacheTangents[k].w = (Vector3f::Dot(Vector3f::Cross(vector, tangent), tan2[k]) < 0.0f) ? -1.0f : 1.0f;
    }
    cacheMeshNormals = array;
    verts.assign(n, Vector3d());
    uvs.assign(n, Vector2f());
    normals.assign(n, Vector3f());
    triNormals.assign(static_cast<size_t>(cacheTriCount), Vector3f());
    created = true;
}

// PQS.cs:1634 CreateIndexCache
void PQSMeshCache::CreateIndexCache() {
    cacheIndices[0].assign(static_cast<size_t>(cacheTriIndexCount), 0);
    std::vector<int>& c0 = cacheIndices[0];
    int num = 0;
    int num2 = 0;
    int num3 = 0;
    while (num < cacheRes) {
        int num4 = 0;
        while (num4 < cacheRes) {
            c0[static_cast<size_t>(num3++)] = num2;
            c0[static_cast<size_t>(num3++)] = num2 + cacheSideVertCount;
            c0[static_cast<size_t>(num3++)] = num2 + 1;
            c0[static_cast<size_t>(num3++)] = num2 + 1;
            c0[static_cast<size_t>(num3++)] = num2 + cacheSideVertCount;
            c0[static_cast<size_t>(num3++)] = num2 + cacheSideVertCount + 1;
            num4++;
            num2++;
        }
        num++;
        num2++;
    }
    for (int i = 1; i < 17; i++) {
        cacheIndices[static_cast<size_t>(i)] = cacheIndices[0];
        CreateIndexState(i, cacheIndices[static_cast<size_t>(i)], cacheIndices[0]);
    }
}

// PQS.cs:1666 CreateIndexState (EdgeState flags: North 1, South 2, East 4, West 8)
void PQSMeshCache::CreateIndexState(int state, std::vector<int>& tris, const std::vector<int>& trisBck) const {
    auto at = [](std::vector<int>& v, int i) -> int& { return v[static_cast<size_t>(i)]; };
    auto bk = [&](int i) { return trisBck[static_cast<size_t>(i)]; };
    if ((state & 1) != 0) {
        for (int i = 0; i < cacheRes; i += 2) {
            const int num2 = ti(i, 0);
            const int num3 = ti(i + 1, 0);
            at(tris, num3) = (at(tris, num2 + 3) = bk(num2));
            at(tris, num2) = 0;
            at(tris, num2 + 1) = 0;
            at(tris, num2 + 2) = 0;
        }
    }
    if ((state & 2) != 0) {
        for (int i = 0; i < cacheRes; i += 2) {
            const int num2 = ti(i, cacheRes - 1);
            const int num3 = ti(i + 1, cacheRes - 1);
            at(tris, num3 + 1) = (at(tris, num2 + 5) = bk(num3 + 5));
            at(tris, num3 + 3) = 0;
            at(tris, num3 + 4) = 0;
            at(tris, num3 + 5) = 0;
        }
    }
    if ((state & 4) != 0) {
        for (int i = 0; i < cacheRes; i += 2) {
            const int num2 = ti(0, i);
            const int num3 = ti(0, i + 1);
            at(tris, num3) = (at(tris, num2 + 4) = bk(num2));
            at(tris, num2) = 0;
            at(tris, num2 + 1) = 0;
            at(tris, num2 + 2) = 0;
        }
    }
    if ((state & 8) != 0) {
        for (int i = 0; i < cacheRes; i += 2) {
            const int num2 = ti(cacheRes - 1, i);
            const int num3 = ti(cacheRes - 1, i + 1);
            at(tris, num3 + 2) = (at(tris, num2 + 5) = bk(num3 + 5));
            at(tris, num3 + 3) = 0;
            at(tris, num3 + 4) = 0;
            at(tris, num3 + 5) = 0;
        }
    }
}

// PQS.cs:1729
void PQSMeshCache::CreateCacheNormals() { cacheNormals.assign(static_cast<size_t>(cacheVertCount), Vector3f::zero()); }

// PQS.cs:1738
void PQSMeshCache::CreateCacheColors() { cacheColors.assign(static_cast<size_t>(cacheVertCount), Color::white()); }

// PQS.cs:1747
void PQSMeshCache::CreateCacheUVExtras() {
    cacheUV2s.assign(static_cast<size_t>(cacheVertCount), Vector2f::zero());
    cacheUV3s.assign(static_cast<size_t>(cacheVertCount), Vector2f::zero());
    cacheUV4s.assign(static_cast<size_t>(cacheVertCount), Vector2f::zero());
}

// ---- PQSCache.cs: presets ----------------------------------------------------------------------------------------------

// PQSCache.cs:173
void PQSCache::PQSSpherePreset::Load(const ConfigNode& node) {
    const auto n = node.GetValue("name");
    if (!n) {
        name = "";  // Debug.LogWarning("No preset name found")
        return;
    }
    name = *n;
    if (node.HasValue("minDistance")) minDistance = dotnet::ParseDouble(*node.GetValue("minDistance"));
    if (node.HasValue("minSubdivision")) minSubdivision = dotnet::ParseInt32(*node.GetValue("minSubdivision"));
    if (node.HasValue("maxSubdivision")) maxSubdivision = dotnet::ParseInt32(*node.GetValue("maxSubdivision"));
}

// PQSCache.cs:196
void PQSCache::PQSSpherePreset::Save(ConfigNode& node) const {
    node.AddValue("name", name);
    node.AddValue("minDistance", minDistance);
    node.AddValue("minSubdivision", static_cast<int32_t>(minSubdivision));
    node.AddValue("maxSubdivision", static_cast<int32_t>(maxSubdivision));
}

// PQSCache.cs:120
void PQSCache::PQSPreset::Load(const ConfigNode& node) {
    const auto n = node.GetValue("name");
    if (!n) {
        name = "";  // Debug.LogWarning("No preset name found")
        return;
    }
    name = *n;
    for (const auto& child : node.nodes()) {
        if (child->name == "PLANET") {
            PQSSpherePreset p;
            p.Load(*child);
            spherePresets.push_back(p);
        }
    }
}

// PQSCache.cs:140
void PQSCache::PQSPreset::Save(ConfigNode& node) const {
    node.AddValue("name", name);
    for (const auto& p : spherePresets) p.Save(*node.AddNode("PLANET"));
}

// PQSCache.cs:19
PQSCache::PQSGlobalPresetList::PQSGlobalPresetList() : version(Versioning_GetVersion()) {}

// PQSCache.cs:25
void PQSCache::PQSGlobalPresetList::Load(const ConfigNode& node) {
    const auto p = node.GetValue("preset");
    if (!p) {
        preset = "";  // Debug.LogWarning("No preset name found")
        return;
    }
    preset = *p;
    const auto v = node.GetValue("version");
    version = v ? *v : "0.0.0";
    for (const auto& child : node.nodes()) {
        if (child->name == "PRESET") {
            PQSPreset pr;
            pr.Load(*child);
            presets.push_back(pr);
        }
    }
    for (int i = 0, count = static_cast<int>(presets.size()); i < count; i++)
        if (presets[static_cast<size_t>(i)].name == preset) presetIndex = i;
}

// PQSCache.cs:58
void PQSCache::PQSGlobalPresetList::SetPreset(int index) {
    preset = presets.at(static_cast<size_t>(index)).name;
    presetIndex = index;
}

// PQSCache.cs:64 (sic: tests presets[i].name == preset, the CURRENT preset, not presetName)
void PQSCache::PQSGlobalPresetList::SetPreset(const std::string& presetName) {
    for (int i = 0, count = static_cast<int>(presets.size()); i < count; i++) {
        if (presets[static_cast<size_t>(i)].name == preset) {
            preset = presetName;
            presetIndex = i;
        }
    }
}

// PQSCache.cs:77
void PQSCache::PQSGlobalPresetList::Save(ConfigNode& node) const {
    node.AddValue("preset", preset);
    node.AddValue("version", version);
    for (const auto& p : presets) p.Save(*node.AddNode("PRESET"));
}

// PQSCache.cs:87
const PQSCache::PQSSpherePreset* PQSCache::PQSGlobalPresetList::GetPreset(const std::string& pqsName) const {
    for (const auto& p : presets) {
        if (!(p.name == preset)) continue;
        for (const auto& sp : p.spherePresets)
            if (sp.name == pqsName) return &sp;
    }
    return nullptr;  // Debug.LogWarning("Cannot find preset ...")
}

// PQSCache.cs:239 (+ PQS.LoadCacheSettings :1500 into `settingsTarget`, the static mesh-cache settings)
void PQSCache::LoadPresetList(const ConfigNode& node, PQSMeshCache* settingsTarget) {
    if (node.HasNode("TERRAIN") && node.GetNode("TERRAIN")->nodes().Count() != 0) {
        const ConfigNode* node2 = node.GetNode("TERRAIN");
        PresetList = std::make_unique<PQSGlobalPresetList>();
        PresetList->Load(*node2);
        if (node2->HasNode("PQS") && settingsTarget != nullptr) settingsTarget->LoadCacheSettings(*node2->GetNode("PQS"));
        if (!PQS::GameBindings().TerrainPresetListIsCompatible(PresetList->version)) {
            CreateDefaultPresetList();  // Debug.LogWarning("Terrain Preset list is incompatible. Reverting to Defaults.")
        }
    } else {
        CreateDefaultPresetList();  // Debug.LogWarning("Creating terrain preset list")
    }
}

// PQSCache.cs:263
void PQSCache::SavePresetList(ConfigNode& node) {
    if (!PresetList) CreateDefaultPresetList();
    PresetList->Save(*node.AddNode("TERRAIN"));
}

// PQSCache.cs:273
void PQSCache::CreateDefaultPresetList() {
    PresetList = std::make_unique<PQSGlobalPresetList>();
    PQSPreset low;
    low.name = "Low";
    for (const auto& r : std::initializer_list<PQSSpherePreset>{
             {"Kerbin", 4.0, 1, 8}, {"KerbinOcean", 4.0, 1, 7}, {"Mun", 4.0, 1, 7}, {"Minmus", 4.0, 1, 6},
             {"Bop", 4.0, 1, 6}, {"Duna", 4.0, 1, 7}, {"Eve", 4.0, 1, 7}, {"EveOcean", 4.0, 1, 7}, {"Gilly", 4.0, 1, 6},
             {"Ike", 4.0, 1, 7}, {"Laythe", 4.0, 1, 7}, {"LaytheOcean", 4.0, 1, 7}, {"Moho", 4.0, 1, 7},
             {"Tylo", 4.0, 1, 7}, {"Vall", 4.0, 1, 7}, {"Dres", 4.0, 1, 7}, {"Pol", 4.0, 1, 7}, {"Eeloo", 4.0, 1, 7}})
        low.spherePresets.push_back(r);
    PQSPreset def;
    def.name = "Default";
    for (const auto& r : std::initializer_list<PQSSpherePreset>{
             {"Kerbin", 6.0, 1, 9}, {"KerbinOcean", 6.0, 1, 7}, {"Mun", 6.0, 1, 8}, {"Minmus", 6.0, 1, 6},
             {"Bop", 6.0, 1, 6}, {"Duna", 6.0, 1, 8}, {"Eve", 6.0, 1, 9}, {"EveOcean", 6.0, 1, 7}, {"Gilly", 6.0, 1, 6},
             {"Ike", 6.0, 1, 6}, {"Laythe", 6.0, 1, 9}, {"LaytheOcean", 6.0, 1, 7}, {"Moho", 6.0, 1, 8},
             {"Tylo", 6.0, 1, 8}, {"Vall", 6.0, 1, 8}, {"Dres", 6.0, 1, 8}, {"Pol", 6.0, 1, 8}, {"Eeloo", 6.0, 1, 8}})
        def.spherePresets.push_back(r);
    PQSPreset high;
    high.name = "High";
    for (const auto& r : std::initializer_list<PQSSpherePreset>{
             {"Kerbin", 8.0, 1, 10}, {"KerbinOcean", 8.0, 1, 7}, {"Mun", 8.0, 1, 9}, {"Minmus", 8.0, 1, 7},
             {"Bop", 8.0, 1, 6}, {"Duna", 8.0, 1, 9}, {"Eve", 8.0, 1, 10}, {"EveOcean", 8.0, 1, 7}, {"Gilly", 8.0, 1, 7},
             {"Ike", 8.0, 1, 7}, {"Laythe", 8.0, 1, 10}, {"LaytheOcean", 8.0, 1, 7}, {"Moho", 8.0, 1, 9},
             {"Tylo", 8.0, 1, 9}, {"Vall", 8.0, 1, 9}, {"Dres", 8.0, 1, 9}, {"Pol", 8.0, 1, 9}, {"Eeloo", 8.0, 1, 9}})
        high.spherePresets.push_back(r);
    PresetList->presets.push_back(low);
    PresetList->presets.push_back(def);
    PresetList->presets.push_back(high);
    PresetList->preset = "Default";
    PresetList->presetIndex = 1;
}

namespace {
bool DefaultPresetListCompatible(const std::string& v) {
    return KSPUtil::CheckVersion(v, PQSCache::lastCompatibleMajor, PQSCache::lastCompatibleMinor, PQSCache::lastCompatibleRev) ==
           VersionCompareResult::COMPATIBLE;
}
bool DefaultTrue() { return true; }
bool DefaultFalse() { return false; }
float DefaultOne() { return 1.0f; }
void DefaultCity(PQSSurfaceObject*) {}
double DefaultRange(PQSSurfaceObject*) { return std::numeric_limits<double>::max(); }
}  // namespace

PQS_GameBindings::PQS_GameBindings() { Reset(); }

// the C# field initialisers (PQS_GameBindings.cs:15-37)
void PQS_GameBindings::Reset() {
    GetSettingsReady = Delegate<bool()>::Static(&DefaultTrue);
    GetPlanetForceShaderModel20 = Delegate<bool()>::Static(&DefaultFalse);
    GetUsePlanetScatter = Delegate<bool()>::Static(&DefaultTrue);
    GetPlanetScatterFactor = Delegate<float()>::Static(&DefaultOne);
    GetLoadedSceneIsGame = Delegate<bool()>::Static(&DefaultFalse);
    GetPresetListCompatible = Delegate<bool(const std::string&)>::Static(&DefaultPresetListCompatible);
    OnPQSCityLoaded = Delegate<void(PQSSurfaceObject*)>::Static(&DefaultCity);
    OnPQSCityUnloaded = Delegate<void(PQSSurfaceObject*)>::Static(&DefaultCity);
    OnGetPQSCityLoadRange = Delegate<double(PQSSurfaceObject*)>::Static(&DefaultRange);
    OnGetPOIRange = Delegate<double(PQSSurfaceObject*)>::Static(&DefaultRange);
}

// PQS_GameBindings.cs:39
bool PQS_GameBindings::TerrainPresetListIsCompatible(const std::string& versionString) const { return GetPresetListCompatible(versionString); }

// ---- PQSCache.cs: pool -----------------------------------------------------------------------------------------------

PQSCache::PQSCache() = default;
PQSCache::~PQSCache() { OnDestroy(); }

// PQSCache.cs:378 CreateCache (cachePQCoroutine: the C# spreads IncreasePQCache over frames; synchronous here)
void PQSCache::CreateCache() {
    if (!meshCache.created) meshCache.CreateCache();
    reused_.clear();
    fresh_ = 0;
    cachePQUnassignedCount = 0;  // new Stack<PQ> (the C# drops the old stack's quads)
    cachePQAssignedCount = 0;
    if (cachePQCoroutine) {
        HighLogic::Scheduler().StartCoroutine(CoroutineCreatePQCache(), owner);
        return;
    }
    IncreasePQCache(cachePQInitialCount);
    cacheReady = true;
    if (onCacheReady) onCacheReady();
    onCacheReady = nullptr;
}

// PQSCache.cs:479
Enumerator PQSCache::CoroutineCreatePQCache() {
    cacheBeingCreated = true;
    while (cachePQUnassignedCount < cachePQInitialCount) {
        IncreasePQCache(cachePQIncreasePerFrame);
        co_yield YieldInstruction::Null();
    }
    cacheBeingCreated = false;
    cacheReady = true;
    if (onCacheReady) onCacheReady();
    onCacheReady = nullptr;
}

// PQSCache.cs:343
void PQSCache::Reset() {
    cachePQInitialCount = 9216;
    cachePQCoroutine = true;
    cachePQIncreasePerFrame = 256;
    cacheReady = false;
}

// PQSCache.cs:351 (DontDestroyOnLoad of a root object: engine side)
void PQSCache::Awake() {
    if (Instance != nullptr && Instance != this) {
        log::Warning("PQSCache already exists. Removing");
        destroyed_ = true;   // Object.Destroy(gameObject)
        return;
    }
    Instance = this;
    if (!cacheReady && !cacheBeingCreated) CreateCache();
}

// PQSCache.cs:370
void PQSCache::OnDestroy() {
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

// PQSCache.cs:470
void PQSCache::ResetCache(const std::vector<PQS*>& spheres) {
    for (PQS* s : spheres) s->ResetSphere();
}

// PQSCache.cs:400 IncreasePQCache: fresh quads, materialised on first GetQuad from this snapshot of the statics
void PQSCache::IncreasePQCache(int addCount) {
    auto t = std::make_shared<FreshTemplate>();
    t->uv = meshCache.cacheUVs;
    t->tangents = meshCache.cacheTangents;
    t->colors = meshCache.cacheColors;
    freshTemplate_ = std::move(t);
    fresh_ += addCount;
    cachePQUnassignedCount += addCount;
    cachePQTotalCount += addCount;
}

// PQSCache.cs:436 GetQuad
PQ* PQSCache::GetQuad() {
    if (cachePQUnassignedCount == 0) {
        IncreasePQCache(cachePQIncreasePerFrame);  // print("PQSCache: Increased PQ Cache by ...")
    }
    cachePQAssignedCount++;
    cachePQUnassignedCount--;
    if (!reused_.empty()) {
        PQ* q = reused_.back();
        reused_.pop_back();
        return q;
    }
    if (fresh_ <= 0) throw std::logic_error("PQSCache: pool accounting");
    fresh_--;
    // PQSCache.cs:404-430: the GameObject, its PQ (Awake), MeshFilter / MeshRenderer and the Mesh
    auto q = std::make_unique<PQ>();
    const PQSMeshCache& c = meshCache;
    q->mesh.vertices = c.cacheVerts;
    q->mesh.uv = freshTemplate_->uv;
    q->mesh.normals = c.cacheNormals;
    q->mesh.tangents = freshTemplate_->tangents;
    q->mesh.triangleSet = 0;
    q->mesh.colors = freshTemplate_->colors;
    q->mesh.RecalculateBounds();  // Mesh.vertices assignment recomputes the bounds
    q->rendererEnabled = true;    // a new MeshRenderer is enabled (the GameObject is then deactivated)
    q->verts = c.cacheVerts;
    q->vertNormals = c.cacheNormals;
    for (auto& e : q->edgeNormals) e.assign(static_cast<size_t>(c.cacheSideVertCount), Vector3f());
    q->vertsD.assign(static_cast<size_t>(c.cacheVertCount), Vector3d());
    PQ* raw = q.get();
    all_.push_back(std::move(q));
    return raw;
}

// PQSCache.cs:448 DestroyQuad
void PQSCache::DestroyQuad(PQ& quad) {
    quad.north = nullptr;
    quad.south = nullptr;
    quad.east = nullptr;
    quad.west = nullptr;
    quad.parent = nullptr;
    quad.quadRoot = nullptr;
    quad.sphereRoot = nullptr;
    quad.subdivideThresholdFactor = 1.0;
    quad.isBuilt = false;
    quad.isSubdivided = false;
    quad.isActive = false;
    quad.isVisible = false;
    quad.rendererEnabled = false;
    cachePQAssignedCount--;
    cachePQUnassignedCount++;
    reused_.push_back(&quad);
}

}  // namespace ksp
