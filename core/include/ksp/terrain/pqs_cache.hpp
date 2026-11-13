// WP-12a: the PQS static mesh cache (PQS.cs:387-433 statics, CreateCache :1517, CreateIndexCache :1634) and the PQSCache
// quad pool with the terrain detail presets (Assembly-CSharp/PQSCache.cs).
//
// PQSMeshCache = every `static` array of PQS.cs: the template grid (cacheVerts, cacheUVs, cacheUVQuad), the 17 index
// sets (one per EdgeState combination), the template tangents (tan1 / tan2 / cacheTangents) AND the per-build scratch
// arrays the quad build writes and assigns to the quad meshes (verts, normals, uvs, triNormals, cacheColors,
// cacheUV2s..4s; cacheUVs and cacheTangents are rewritten by PQSMod_UVPlanetRelativePosition / BuildTangents). In KSP
// those are process-wide statics shared by every sphere; here they belong to one PQSCache, which every sphere that
// shares the cache (KSP: all of them) builds through. One PQSCache per thread for parallel building.
//
// PQSCache pool semantics (PQSCache.cs:400-468): a Stack<PQ>. IncreasePQCache pushes N fresh quads (only ever called when
// the stack is empty), GetQuad pops, DestroyQuad clears the links / flags listed in the C# and pushes the quad back
// (LIFO), WITHOUT clearing its arrays, mesh, delegates, isCached, edgeState or meshVertMin/Max. Fresh quads are created
// on first use (KSP allocates cachePQInitialCount = 9216 GameObjects up front): identical behaviour, since fresh quads
// are indistinguishable and are always below the reused ones on the stack; each fresh quad's mesh is initialised from
// a snapshot of the static arrays taken when IncreasePQCache ran, as the C# copies them then.
#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class ConfigNode;

class PQSMeshCache {
public:
    // ---- settings statics (PQS.cs:387-393; PQS.LoadCacheSettings :1500 reads them from settings.cfg TERRAIN/PQS) ----
    int cacheSideVertCount = 15;
    float cacheMeshSize = 1.0f;
    float cacheQuadSize = 1.0f;
    float cacheQuadSizeDiv2 = 0.5f;
    // PQS.cs:433 cacheVertCountReciprocal: initialised once from the default 15 (LoadCacheSettings never updates it)
    double cacheVertCountReciprocal = 1.0 / 225.0;

    // ---- derived by CreateCache ----
    bool created = false;  // PQS.cacheMesh != null
    int cacheVertCount = 0;
    int cacheRes = 0;
    int cacheResDiv2 = 0;
    int cacheResDiv2Plus1 = 0;
    int cacheTriIndexCount = 0;
    int cacheTriCount = 0;
    std::array<std::vector<int>, 17> cacheIndices;
    std::vector<Vector3f> cacheVerts;
    std::vector<Vector2f> cacheUVs;      // rewritten by PQSMod_UVPlanetRelativePosition.OnQuadBuilt
    std::vector<Vector3f> cacheNormals;  // zeros
    std::vector<Vector4f> cacheTangents; // rewritten by BuildTangents
    std::vector<Vector3f> tan1, tan2;
    std::vector<Color> cacheColors;      // white, then the per-vertex colour scratch
    std::vector<Vector2f> cacheUV2s, cacheUV3s, cacheUV4s;
    std::vector<Vector2d> cacheUVQuad;
    // template mesh channels (cacheMesh: the grid normals are all up)
    std::vector<Vector3f> cacheMeshNormals;

    // ---- build scratch statics (PQS.cs:511-521) ----
    std::vector<Vector3d> verts;      // planet-relative double vertex positions of the quad being built
    std::vector<Vector2f> uvs;        // written by BuildVertexQuadUV / SphereUV, never assigned to a mesh (sic)
    std::vector<Vector3f> normals;    // sphere normals
    std::vector<Vector3f> triNormals;
    Vector3d vertRel;
    Vector3d planetRel;

    void LoadCacheSettings(const ConfigNode& node);  // PQS.cs:1500
    void CreateCache();                              // PQS.cs:1517
    static int ti(int x, int z, int cacheRes) { return x * 6 + z * cacheRes * 6; }  // PQS.cs:1719
    int ti(int x, int z) const { return ti(x, z, cacheRes); }
    int vi(int x, int z) const { return z * cacheSideVertCount + x; }               // PQS.cs:1724

private:
    void CreateIndexCache();                                                              // :1634
    void CreateIndexState(int state, std::vector<int>& tris, const std::vector<int>& trisBck) const;  // :1666
    void CreateCacheNormals();   // :1729
    void CreateCacheColors();    // :1738
    void CreateCacheUVExtras();  // :1747
};

class PQSCache {
public:
    // ---- presets (PQSCache.cs:9-203) ----
    class PQSSpherePreset {
    public:
        std::string name;
        double minDistance = 0.0;
        int minSubdivision = 0;
        int maxSubdivision = 0;
        PQSSpherePreset() = default;
        PQSSpherePreset(std::string name_, double minDistance_, int minSubdivision_, int maxSubdivision_)
            : name(std::move(name_)), minDistance(minDistance_), minSubdivision(minSubdivision_), maxSubdivision(maxSubdivision_) {}
        void Load(const ConfigNode& node);  // :173
        void Save(ConfigNode& node) const;  // :196
    };
    class PQSPreset {
    public:
        std::string name;
        std::vector<PQSSpherePreset> spherePresets;
        void Load(const ConfigNode& node);  // :120
        void Save(ConfigNode& node) const;  // :140
    };
    class PQSGlobalPresetList {
    public:
        std::vector<PQSPreset> presets;
        std::string preset;
        int presetIndex = 0;
        std::string version;
        PQSGlobalPresetList();                         // :19 (version = VersioningBase.GetVersionString())
        void Load(const ConfigNode& node);             // :25
        void SetPreset(int presetIndex);               // :58
        void SetPreset(const std::string& presetName); // :64 (sic: compares against the current preset)
        void Save(ConfigNode& node) const;             // :77
        const PQSSpherePreset* GetPreset(const std::string& pqsName) const;  // :87
    };

    static inline std::unique_ptr<PQSGlobalPresetList> PresetList;  // PQSCache.PresetList (static)
    static inline int lastCompatibleMajor = 1, lastCompatibleMinor = 4, lastCompatibleRev = 3;
    static void LoadPresetList(const ConfigNode& node, PQSMeshCache* settingsTarget);  // :239 (+ PQS.LoadCacheSettings)
    static void SavePresetList(ConfigNode& node);  // :263
    static void CreateDefaultPresetList();         // :273

    // ---- pool (PQSCache.cs:217-235, Reset :343 defaults) ----
    int cachePQInitialCount = 9216;
    int cachePQIncreasePerFrame = 256;
    bool cacheReady = false;
    int cachePQAssignedCount = 0;
    int cachePQUnassignedCount = 0;
    int cachePQTotalCount = 0;

    PQSCache();
    ~PQSCache();
    PQSCache(const PQSCache&) = delete;
    PQSCache& operator=(const PQSCache&) = delete;

    PQSMeshCache meshCache;

    // WP-12d: the MonoBehaviour lifecycle and the coroutine variant
    static inline PQSCache* Instance = nullptr;   // :207
    bool cachePQCoroutine = false;       // :221 (the field default; Reset, an editor callback, sets true)
    bool cacheBeingCreated = false;      // :223
    std::function<void()> onCacheReady; // :235 (OnEvent)
    int owner = 0;                       // the coroutine owner (StartCoroutine)
    void Reset();                        // :343
    void Awake();                        // :351 (a second instance is "destroyed": Awake returns false)
    bool Destroyed() const { return destroyed_; }
    void OnDestroy();                    // :370
    // :470 ResetCache: ResetSphere on every PQS (FindObjectsOfType: the spheres the host lists)
    void ResetCache(const std::vector<PQS*>& spheres);
    void CreateCache();                  // :378 (cachePQCoroutine: CoroutineCreatePQCache on HighLogic's scheduler)
    Enumerator CoroutineCreatePQCache(); // :479
    void IncreasePQCache(int addCount);  // :400
    PQ* GetQuad();                       // :436
    void DestroyQuad(PQ& quad);          // :448
    int quadsMaterialised() const { return static_cast<int>(all_.size()); }

private:
    struct FreshTemplate {
        std::vector<Vector2f> uv;
        std::vector<Vector4f> tangents;
        std::vector<Color> colors;
    };
    std::vector<std::unique_ptr<PQ>> all_;  // owns every quad ever handed out
    std::vector<PQ*> reused_;               // returned quads, top = back
    int fresh_ = 0;                         // fresh quads below them
    std::shared_ptr<FreshTemplate> freshTemplate_;
    bool destroyed_ = false;
};

}  // namespace ksp
