// WP-11 + WP-12a: the PQS quad-sphere (Assembly-CSharp/PQS.cs, PQSMod.cs) without Unity objects.
//
// WP-11 (height path): the mod stack as a pure function of a direction.
//   * SetupMods (:2109): the GetChildMods list (prefab order, built by the generated stock data or by the caller) is
//     sorted with List.Sort's unstable introsort on `order` (dotnet::ListSort with the List's capacity as array
//     length), then mods with modEnabled == false, an inactive GameObject or modExpansionDisabled are removed, OnSetup
//     runs on the survivors in order and their requirements are OR-ed;
//   * SetupBuildDelegates (:2177): reqVertexMapCoods (VertexMapCoords, forced by UVSphereCoords), radiusMax/radiusMin =
//     radius + the sums of GetVertexMax/MinHeight in stack order, radiusDelta, halfChord;
//   * GetSurfaceHeight (:3088/:3093), Mod_OnVertexBuildHeight (:1956), BuildVertexMapCoords (:2294).
// State is kept exactly like KSP: one VertexBuildData (vbData) is reused by every call, so a stack that reads
// data.u/v without requesting VertexMapCoords sees the previous call's values; mods keep their scratch fields.
// Not thread-safe (KSP calls this on the main thread); create one PQS per thread for parallel use.
//
// Quad-dependent mods (FlattenArea, FlattenAreaTangential, MapDecal, MapDecalTangent) start in the state of a PQS
// whose quads were never built (MAINMENU): MapDecal(Tangent) apply, FlattenArea applies only with
// overrideQuadBuildCheck, FlattenAreaTangential not at all. OnQuadPreBuild/OnQuadBuilt (a PQ, ksp/terrain/pq.hpp)
// drive that state like the quad builder does.
//
// WP-12a (quadtree and quad build, see "PQS quadtree" below and ksp/terrain/pq.hpp): the sphere lifecycle
// (StartSphere / ResetSphere / Activate / Deactivate, the UpdateSphere coroutine as FrameUpdate()), the six root quads,
// UpdateVisual (thresholds from the target position), UpdateQuadsInit / UpdateQuads, the quad build (BuildQuad: vertex
// directions through the native Matrix4x4.TRS + managed MultiplyPoint3x4, heights through the same Mod_OnVertexBuildHeight,
// positions, sphere or custom normals, UV / colour channels, tangents), the custom-normal edge stitching (UpdateEdges /
// UpdateEdgeNormals), the static mesh cache and the PQSCache quad pool (ksp/terrain/pqs_cache.hpp). Output: every
// built PQ carries a PQMesh (positions, normals, tangents, UVs, colours, triangle set) for the engine to upload.
// Frames: the sphere's local frame is the body-fixed frame (no floating origin); targets are given in it as doubles.
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/unity/animation_curve.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class PQS;
class PQSMod;
class MapSO;
class PQSVertexBuildData;
class PQ;             // ksp/terrain/pq.hpp (included at the end of this header)
class PQSCache;       // ksp/terrain/pqs_cache.hpp
class PQSMeshCache;   // ksp/terrain/pqs_cache.hpp

// One serialized field of a mod, for the bit-exact prefab test (tests/test_terrain.cpp): C# field path (nested
// fields as `a.b`, arrays as `a[i]`) and its value with the C# type kind.
struct SerializedValue {
    enum class Kind { Double, Float, Int, Bool, String, Vector3, Curve, Map, Color };
    Kind kind = Kind::Double;
    double number = 0.0;  // Double/Float/Int/Bool (exact widening)
    std::string text;     // String; Map: map key ("" = null)
    Vector3f vec;
    Color color;
    const AnimationCurve* curve = nullptr;

    static SerializedValue Of(double v) { SerializedValue s; s.kind = Kind::Double; s.number = v; return s; }
    static SerializedValue Of(float v) { SerializedValue s; s.kind = Kind::Float; s.number = v; return s; }
    static SerializedValue Of(int v) { SerializedValue s; s.kind = Kind::Int; s.number = v; return s; }
    static SerializedValue Of(bool v) { SerializedValue s; s.kind = Kind::Bool; s.number = v ? 1.0 : 0.0; return s; }
    static SerializedValue Of(const std::string& v) { SerializedValue s; s.kind = Kind::String; s.text = v; return s; }
    static SerializedValue Of(const Vector3f& v) { SerializedValue s; s.kind = Kind::Vector3; s.vec = v; return s; }
    static SerializedValue Of(const Color& v) { SerializedValue s; s.kind = Kind::Color; s.color = v; return s; }
    static SerializedValue Of(const AnimationCurve& v) { SerializedValue s; s.kind = Kind::Curve; s.curve = &v; return s; }
    static SerializedValue Of(const std::shared_ptr<const MapSO>& v);
};
using SerializedFieldVisitor = std::function<void(const std::string& path, const SerializedValue& value)>;

// PQSMod.cs
class PQSMod {
public:
    virtual ~PQSMod() = default;

    // ---- serialized PQSMod fields (PQSMod.cs:6-18) ----
    PQS* sphere = nullptr;
    int requirements = 0;  // PQS::ModiferRequirements
    bool modEnabled = true;
    int order = 100;
    bool overrideQuadBuildCheck = false;  // internal
    bool modExpansionDisabled = false;    // internal

    // ---- Unity context (the component's class and GameObject) ----
    std::string className = "PQSMod";
    std::string gameObjectPath;     // e.g. "Kerbin/_HeightNoise"
    bool gameObjectActiveSelf = true;

    virtual void OnSetup();                                  // PQSMod.cs:29
    virtual void OnPostSetup() {}                            // PQSMod.cs:38
    virtual void OnSphereReset() {}                          // PQSMod.cs:42
    virtual void OnSphereActive() {}                         // PQSMod.cs:46
    virtual void OnSphereInactive() {}                       // PQSMod.cs:50
    virtual bool OnSphereStart() { return false; }           // PQSMod.cs:54
    virtual void OnSphereStarted() {}                        // PQSMod.cs:59
    virtual void OnSphereTransformUpdate() {}                // PQSMod.cs:63
    virtual void OnPreUpdate() {}                            // PQSMod.cs:67
    virtual void OnUpdateFinished() {}                       // PQSMod.cs:71
    virtual void OnVertexBuild(PQSVertexBuildData&) {}        // PQSMod.cs:75
    virtual void OnVertexBuildHeight(PQSVertexBuildData&) {}  // PQSMod.cs:79
    virtual double GetVertexMaxHeight() { return 0.0; }      // PQSMod.cs:83
    virtual double GetVertexMinHeight() { return 0.0; }      // PQSMod.cs:88
    virtual void OnMeshBuild() {}                            // PQSMod.cs:93
    virtual void OnQuadCreate(PQ&) {}                        // PQSMod.cs:97
    virtual void OnQuadDestroy(PQ&) {}                       // PQSMod.cs:101
    virtual void OnQuadPreBuild(PQ&) {}                      // PQSMod.cs:105
    virtual void OnQuadBuilt(PQ&) {}                         // PQSMod.cs:109
    virtual void OnQuadUpdate(PQ&) {}                        // PQSMod.cs:113
    virtual void OnQuadUpdateNormals(PQ&) {}                 // PQSMod.cs:117

    // True for classes that override OnVertexBuildHeight (the capture's `height_mod`).
    virtual bool AffectsHeight() const { return false; }
    // Every serialized field this port consumes (base fields included), for the prefab test.
    virtual void VisitSerialized(const SerializedFieldVisitor& v) const;
};

// PQS.VertexBuildData (PQS.cs:77). `buildQuad` is a pointer to the quad being built (null in GetSurfaceHeight).
class PQSVertexBuildData {
public:
    PQ* buildQuad = nullptr;
    Vector3d globalV;
    Vector3d directionFromCenter;
    Vector3d directionD;
    Vector3d directionXZ;
    int vertIndex = 0;
    double vertHeight = 0.0;
    Color vertColor;
    double u = 0.0, v = 0.0, u2 = 0.0, v2 = 0.0, u3 = 0.0, v3 = 0.0, u4 = 0.0, v4 = 0.0;
    double gnomonicU = 0.0, gnomonicV = 0.0;
    bool allowScatter = false;
    int gnomonicPlane = 0;  // PQS::QuadPlane
    double latitude = 0.0;
    double longitude = 0.0;

    void Reset() { buildQuad = nullptr; }  // PQS.cs:130
};

// The target / secondary target of a sphere (C#: a Unity Transform): its position in the sphere's local frame
// (body-fixed, double). KSP reads `transform.InverseTransformPoint(target.position)` in float world space; the core
// supplies the body-fixed double directly (no floating-origin rounding). Owned by the host; the PQS keeps a pointer
// and compares identity like the C# compares Transform references.
struct PQSTarget {
    Vector3d position;
};

// PQS_GameBindings.cs: the game-side settings hooks (C# events with these defaults)
struct PQS_GameBindings {
    std::function<bool()> GetSettingsReady = [] { return true; };
    std::function<bool()> GetPlanetForceShaderModel20 = [] { return false; };
    std::function<bool()> GetUsePlanetScatter = [] { return true; };
    std::function<float()> GetPlanetScatterFactor = [] { return 1.0f; };
    std::function<bool()> GetLoadedSceneIsGame = [] { return false; };
    std::function<bool(const std::string&)> GetPresetListCompatible;  // default: KSPUtil.CheckVersion vs 1.4.3 (pqs_cache.cpp)
    bool SettingsReady() const { return GetSettingsReady(); }
    bool PLANET_FORCE_SHADER_MODEL_2_0() const { return GetPlanetForceShaderModel20(); }
    bool PLANET_SCATTER() const { return GetUsePlanetScatter(); }
    float PLANET_SCATTER_FACTOR() const { return GetPlanetScatterFactor(); }
    bool LoadedSceneIsGame() const { return GetLoadedSceneIsGame(); }
    bool TerrainPresetListIsCompatible(const std::string& versionString) const;  // PQS_GameBindings.cs:39
};

class PQS {
public:
    // PQS.cs:20
    enum class QuadEdge : int { North = 0, South = 1, East = 2, West = 3, Null = -1 };
    // PQS.cs:29
    enum class QuadChild : int { SouthWest = 0, SouthEast = 1, NorthWest = 2, NorthEast = 3, Null = -1 };
    // PQS.cs:38 ([Flags])
    enum EdgeState : int { Reset = -1, NoLerps = 0, NorthLerp = 1, SouthLerp = 2, EastLerp = 4, WestLerp = 8 };
    // PQS.cs:49
    enum class QuadPlane : int { XP = 0, XN = 1, YP = 2, YN = 3, ZP = 4, ZN = 5 };
    // PQS.cs:60
    enum ModiferRequirements : int {
        Default = 0,
        UniqueMaterialInstances = 1,
        VertexMapCoords = 2,
        VertexGnomonicMapCoords = 4,
        UVSphereCoords = 8,
        UVQuadCoords = 0x10,
        MeshColorChannel = 0x20,
        MeshCustomNormals = 0x40,
        MeshBuildTangents = 0x80,
        MeshUV2 = 0x100,
        MeshUV3 = 0x200,
        MeshUV4 = 0x400,
        MeshAssignTangents = 0x800
    };
    using VertexBuildData = PQSVertexBuildData;

    PQS() = default;
    ~PQS();
    PQS(const PQS&) = delete;
    PQS& operator=(const PQS&) = delete;

    // ---- statics (PQS.cs:145-151) ----
    static inline bool Global_ForceShaderModel = true;
    static inline bool Global_AllowScatter = true;
    static inline double Global_ScatterFactor = 1.0;
    static PQS_GameBindings& GameBindings();

    // ---- serialized / identity ----
    std::string name;    // GameObject name (= body name for stock bodies)
    double radius = 0.0;
    bool buildTangents = false;
    bool isBuildingMaps = false;
    bool surfaceRelativeQuads = false;

    // ---- derived in setup ----
    double radiusSquared = 0.0;
    double circumference = 0.0;
    double radiusMax = 0.0;
    double radiusMin = 0.0;
    double radiusDelta = 0.0;
    double halfChord = 0.0;
    // PQS.cs:338-339 meshVertMin/meshVertMax: reset by SetupBuildDelegates (PQS.cs:2201-2202), raised by every quad
    // build (PQS.cs:2273). TimeWarp.cs:529 reads max.
    double meshVertMin = 1.7976931348623157e308;
    double meshVertMax = -1.7976931348623157e308;
    int modRequirements = 0;
    bool isFakeBuild = false;
    bool reqVertexMapCoods = false;  // sic (C# name)
    bool reqGnomonicCoords = false;
    bool reqCustomNormals = false;
    bool reqColorChannel = false;
    bool reqBuildTangents = false;
    bool reqAssignTangents = false;
    bool reqUV2 = false, reqUV3 = false, reqUV4 = false;
    bool reqSphereUV = false;
    bool reqUVQuad = false;
    bool useSharedMaterial = true;

    // ---- BuildVertexMapCoords outputs (public fields in C#) ----
    double latitude = 0.0;
    double longitude = 0.0;
    double sx = 0.0;
    double sy = 0.0;
    Vector3d directionXZ;
    int vertexIndex = 0;  // the quad build's loop counter (PQS.cs:2256); GetSurfaceHeight copies it into vbData.vertIndex

    // The PQSMod components under the PQS GameObject in GetChildMods order (PQS.cs:2161), owned here, and the
    // capacity of the C# List after those AddRange calls (List.Sort's introsort depth limit uses the array length;
    // 0 = the list size).
    std::vector<std::unique_ptr<PQSMod>> childMods;
    int childModListCapacity = 0;

    // The evaluated stack after SetupMods (sorted, filtered): PQS.mods / modCount.
    std::vector<PQSMod*> mods;
    int modCount() const { return static_cast<int>(mods.size()); }

    // PQS.cs:758-761 subset: circumference, radiusSquared, SetupMods(), SetupBuildDelegates(false).
    void SetupForHeight();
    void SetupMods();                          // PQS.cs:2109
    void ResetModList() { SetupMods(); }       // PQS.cs:2156
    void SetupBuildDelegates(bool fakeBuild);  // PQS.cs:2177 (flags and radii; the C# keeps no delegates either)

    double GetSurfaceHeight(const Vector3d& radialVector);                               // PQS.cs:3088
    double GetSurfaceHeight(const Vector3d& radialVector, bool overrideQuadBuildCheck);  // PQS.cs:3093 (internal)

    VertexBuildData& vbData() { return vbData_; }

    // Quad-builder hooks (PQS.cs:2077 Mod_OnQuadPreBuild / :2085 Mod_OnQuadBuilt); public for the height-only callers.
    void Mod_OnQuadPreBuild(PQ& quad);
    void Mod_OnQuadBuilt(PQ& quad);

    // =================================================================================================================
    // PQS quadtree (WP-12a)
    // =================================================================================================================

    // ---- serialized LOD settings (PQS.cs:155-260). Defaults are PQS.Reset() (PQS.cs:565); the stock prefab values of
    // the fields WP-11's extraction did not dump are NOT known (terrain::ApplyStockLodSettings, ksp/terrain/pqs_prefab.hpp).
    PQS* parentSphere = nullptr;
    int seed = 1;
    float frameTimeDelta = 0.0f;
    float maxFrameTime = 0.1f;
    int minLevel = 3;
    int maxLevel = 10;
    float maxQuadLenghtsPerFrame = 0.03f;  // sic (C# name)
    int maxLevelAtCurrentTgtSpeed = 10;
    double subdivisionThreshold = 1.0;
    double collapseSeaLevelValue = 2.0;
    double collapseAltitudeValue = 16.0;
    double collapseAltitudeMax = 10000000.0;
    double collapseDelta = 0.0;
    double collapseThreshold = 0.0;
    double visRadSeaLevelValue = 4.0;
    double visRadAltitudeValue = 1.5;
    double visRadAltitudeMax = 100000.0;
    double visRadDelta = 0.0;
    double visRad = 0.0;
    double detailSeaLevelQuads = 3000.0;
    double detailAltitudeQuads = 3000.0;
    double detailAltitudeMax = 100000.0;
    double detailDelta = 0.0;
    double detailRad = 0.0;
    double minDetailDistance = 7.0;
    double maxDetailDistance = 20.0;
    // WP-12a: true when every LOD field above came from the extracted prefab (stock factories); false = PQS.Reset()
    // defaults for the fields data/terrain/pqs_prefab.json lacks (see STATUS WP-12a "Needs data")
    bool lodSettingsFromPrefab = false;

    // ---- state ----
    bool isDisabled = false;
    bool isAlive = false;
    bool isActive = false;
    bool isThinking = false;
    bool isStarted = false;
    const PQSTarget* target = nullptr;
    Vector3d relativeTargetPosition;
    Vector3d relativeTargetPositionNormalized;
    double targetAltitude = 0.0;
    double targetHeight = 0.0;
    Vector3d targetVelocity;
    double targetSpeed = 0.0;
    const PQSTarget* secondaryTarget = nullptr;
    Vector3d relativeSecondaryTargetPosition;
    double targetSecondaryAltitude = 0.0;
    double visibleAltitude = 0.0;
    double visibleRadius = 0.0;
    double horizonDistance = 0.0;
    double horizonAngle = 0.0;
    QuaternionD transformRotation = QuaternionD::identity();
    Vector3d transformPosition;
    bool isSubdivisionEnabled = false;
    std::vector<PQ*> normalUpdateList;  // List<PQ>
    bool quadAllowBuild = false;
    float maxFrameEnd = 0.0f;
    int quadCount = 0;
    std::vector<int> quadCounts;
    bool cancelUpdate = false;
    std::vector<double> subdivisionThresholds;
    std::vector<double> collapseThresholds;
    std::vector<PQ*> quads;  // PQ[6]; empty = null
    PQSCache* cache = nullptr;  // PQSCache.Instance (FindObjectOfType): set by the host before Start()
    std::vector<PQS*> childSpheres;  // ChildSpheres (GetComponentsInChildren<PQS> minus this), set by the host
    // Time.realtimeSinceStartup (frame-budget test of PQ.UpdateSubdivision, fixed-frame timing of UpdateVisual).
    // Unset: always 0 (never out of time; the deterministic default).
    std::function<float()> realtimeSinceStartup;
    // Camera.main.transform for Mod_OnSphereStart's fallback target (unset: no camera)
    std::function<const PQSTarget*()> mainCamera;
    // Called for every quad whose MeshCollider PQSMod_QuadMeshColliders enables / disables (WP-12b seam)
    std::function<void(PQ&)> onQuadColliderEnabled, onQuadColliderDisabled;
    // PQSLandControl / PQSMod_AerialPerspectiveMaterial / PQSMod_CelestialBodyTransform found by SetupMods
    PQSMod* landControl = nullptr;
    PQSMod* aerialPerspectiveMaterial = nullptr;
    PQSMod* modCBTransform = nullptr;

    // ---- lifecycle (PQS.cs) ----
    void Start();                    // :591 (needs `cache`)
    void StartSphere(bool force);    // :717
    void ResetSphere();              // :796
    void RebuildSphere();            // :832
    void ForceStart();               // :838
    void StartUpSphere();            // :847
    void ResetAndWait();             // :852 (+ ResetAndWaitCoroutine :861, resumed by FrameUpdate)
    void SetupExternalRender();      // :872
    void SetupLaunchsitePlacementRender();  // :881
    void ResetLaunchsitePlacementRender();  // :898
    void CloseExternalRender();      // :913
    void EnableSphere();             // :1177
    void ActivateSphere();           // :1183
    void DisableSphere();            // :1216
    void DeactivateSphere();         // :1223
    void SetTarget(const PQSTarget* target);                    // :1261
    void SetSecondaryTarget(const PQSTarget* secondaryTarget);  // :1271
    void SetCache();                 // :1478
    void ClearCache();               // :1491
    void FixedUpdate();              // :1462 (physics tick)
    // One frame of the sphere's coroutines in start order (C#: UpdateSphere :1104, ResetAndWaitCoroutine :861; Unity
    // resumes `yield return null` coroutines once per frame after Update). Call once per frame.
    void FrameUpdate();
    int updateSphereRoutineCount() const {
        int n = 0;
        for (Routine r : routines_) n += r == Routine::UpdateSphere ? 1 : 0;
        return n;
    }

    // ---- quads ----
    PQ* AssignQuad(int subdiv);      // :991
    void QuadCreated(PQ& quad);      // :1026
    void DestroyQuad(PQ& quad);      // :1031
    bool BuildQuad(PQ& quad);        // :2237
    void UpdateVisual();             // :1281
    void UpdateQuadsInit();          // :1388
    void UpdateQuads();              // :1411
    void UpdateEdges();              // :1442
    void UpdateEdgeNormals(PQ& q);   // :2503
    Vector3f GetRightmostCornerNormal(PQ* caller, PQ* nextQuad);  // :2967
    static QuadEdge GetEdgeRotatedClockwise(QuadEdge edge);         // :3008
    static QuadEdge GetEdgeRotatedCounterclockwise(QuadEdge edge);  // :3025
    PQ* buildQuadPtr() const { return buildQuad_; }
    void BuildNormals(PQ& quad);         // :1760 (static in C#)
    void BuildNormalsNoClear(PQ& quad);  // :1798 (static in C#; no caller)
    void BuildTangents(PQ& quad);        // :1619 (static in C#)

    // ---- frame helpers (PQS.cs:3042-3167) ----
    Vector3d GetRelativePosition(const Vector3d& worldPosition) const;   // :3042
    Vector3d GetRelativeDirection(const Vector3d& worldDirection) const; // :3047
    Vector3d GetWorldPosition(const Vector3d& localPosition) const;      // :3052
    double GetAltitude(const Vector3d& worldPosition) const;              // :3103
    static bool LineSphereIntersection(const Vector3d& relPos, const Vector3d& relVel, double radius,
                                       Vector3d& relIntersection);        // :3133

    // The static mesh cache this sphere builds with (cache->meshCache)
    PQSMeshCache& meshCache() const;

    // ---- mod dispatch (PQS.cs:1827-2107) ----
    void Mod_OnPostSetup();          // :1835
    void Mod_OnSphereReset();        // :1843
    void Mod_OnSphereStart();        // :1851
    void Mod_OnSphereStarted();      // :1876
    void Mod_OnSphereTransformUpdate();  // :1884
    void Mod_OnPreUpdate();          // :1912
    void Mod_OnUpdateFinished();     // :1920
    void Mod_OnActive();             // :1940
    void Mod_OnInactive();           // :1948
    void Mod_OnVertexBuild(VertexBuildData& data);  // :1970
    void Mod_OnMeshBuild();          // :2017
    void Mod_OnQuadCreate(PQ& quad);   // :2061
    void Mod_OnQuadDestroy(PQ& quad);  // :2069
    void Mod_OnQuadUpdate(PQ& quad);   // :2093
    void Mod_OnQuadUpdateNormals(PQ& quad);  // :2101

private:
    void Mod_OnSetup();                                                              // PQS.cs:1827
    double Mod_GetVertexMaxHeight();                                                 // PQS.cs:1892
    double Mod_GetVertexMinHeight();                                                 // PQS.cs:1902
    void Mod_OnVertexBuildHeight(VertexBuildData& data, bool overrideQuadBuildCheck);  // PQS.cs:1956
    void BuildVertexMapCoords(VertexBuildData& data);                                // PQS.cs:2294

    // quadtree internals
    void CreateQuads();  // :1055
    void SetupQuad(PQ& quad, const std::string& quadName, QuadPlane plane, PQ* north, PQ* south, PQ* east, PQ* west,
                   const Vector3d& pos);  // :1086
    void SetVisible(bool visible);  // :1238
    void DisableSubdivision();      // :1371
    void EnableSubdivision();       // :1380
    void UpdateSphereIteration();   // one pass of the UpdateSphere loop body (:1108-1117)
    void StartUpdateSphereRoutine();
    void BuildVertexHeight(VertexBuildData& data);          // :2318
    void BuildVertexSurfaceRelative(VertexBuildData& data); // :2324
    void BuildVertexColor(VertexBuildData& data);           // :2332
    void BuildVertexUV2(VertexBuildData& data);             // :2337
    void BuildVertexUV3(VertexBuildData& data);             // :2343
    void BuildVertexUV4(VertexBuildData& data);             // :2349
    void BuildVertexSphereNormal(VertexBuildData& data);    // :2355
    void BuildVertexQuadUV(VertexBuildData& data);          // :2360
    void BuildVertexSphereUV(VertexBuildData& data);        // :2366
    void BuildMeshSphereNormals();   // :2372
    void BuildMeshCustomNormals();   // :2377
    void BuildMeshTangents();        // :2384
    void AssignMeshTangents();       // :2389
    void BuildMeshColorChannel();    // :2394
    void BuildMeshUVChannel();       // :2399
    void BuildMeshUV2Channel();      // :2404
    void BuildMeshUV3Channel();      // :2409
    void BuildMeshUV4Channel();      // :2414
    void BackupEdgeNormals();        // :2471
    void BackupEdgeNormals(std::vector<Vector3f>& edge, int vCount, int localStart, int localDelta);  // :2493
    void CombineEdgeNormals(PQ& q, int vCount, int vertStart, int vertStep, const std::vector<Vector3f>& bkpLocal,
                            int localBackupStart, int localBackupStep, const std::vector<Vector3f>& bkpRemote,
                            int remoteBackupStart, int remoteBackupStep);  // :2951

    VertexBuildData vbData_;
    PQ* buildQuad_ = nullptr;
    // private C# state
    float angularTargetSpeed_ = 0.0f;
    Vector3d targetPosition_;
    Vector3d targetPositionPrev_;
    Vector3d lastRelTgtPosNormalized_;
    double targetDistance_ = 0.0;
    QuaternionD transformRotationInverse_ = QuaternionD::identity();
    int pqID_ = 0;
    int maxLevel1_ = 0;
    int fixedUpdateFrame_ = 0;
    float fixedFrameTime_ = 0.0f;
    float prevFixedFrameTime_ = 0.0f;
    float savemaxFrameTime_ = 0.0f;
    bool hasCache_ = false;
    bool externalRenderScatter_ = false;
    // The sphere's suspended coroutines in resume order (UpdateSphere :1104, ResetAndWaitCoroutine :861); see FrameUpdate
    enum class Routine { UpdateSphere, ResetAndWait };
    std::vector<Routine> routines_;
    std::vector<float> routineResumeAt_;  // WaitForSeconds(frameTimeDelta) target time (UpdateSphere with frameTimeDelta > 0)
    float Now() const { return realtimeSinceStartup ? realtimeSinceStartup() : 0.0f; }
};

}  // namespace ksp

#include "ksp/terrain/pq.hpp"  // NOLINT(misc-header-include-cycle): PQ needs the complete PQS enums above
