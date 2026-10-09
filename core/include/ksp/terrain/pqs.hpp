// WP-11: PQS height evaluation (Assembly-CSharp/PQS.cs, PQSMod.cs): the quad-sphere's mod stack as a pure function of
// a direction, without quads, meshes or Unity objects (mesh/collider generation is WP-12).
//
// What is ported (PQS.cs):
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
// overrideQuadBuildCheck, FlattenAreaTangential not at all. OnQuadPreBuild/OnQuadBuilt with the PQ stub below drive
// that state like the quad builder does.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/math/vector3d.hpp"
#include "ksp/unity/animation_curve.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class PQS;
class PQSMod;
class MapSO;
class PQSVertexBuildData;

// The parts of PQ (a quad) the height mods read in OnQuadPreBuild/OnQuadBuilt (WP-12 replaces this with the real PQ).
struct PQ {
    Vector3d positionPlanetRelative;
    int subdivision = 0;
};

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
    virtual void OnVertexBuild(PQSVertexBuildData&) {}        // PQSMod.cs:75
    virtual void OnVertexBuildHeight(PQSVertexBuildData&) {}  // PQSMod.cs:79
    virtual double GetVertexMaxHeight() { return 0.0; }      // PQSMod.cs:83
    virtual double GetVertexMinHeight() { return 0.0; }      // PQSMod.cs:88
    virtual void OnQuadPreBuild(const PQ&) {}                // PQSMod.cs:105
    virtual void OnQuadBuilt(const PQ&) {}                   // PQSMod.cs:109

    // True for classes that override OnVertexBuildHeight (the capture's `height_mod`).
    virtual bool AffectsHeight() const { return false; }
    // Every serialized field this port consumes (base fields included), for the prefab test.
    virtual void VisitSerialized(const SerializedFieldVisitor& v) const;
};

// PQS.VertexBuildData (PQS.cs:77). `buildQuad` is a pointer to the quad being built (null in GetSurfaceHeight).
class PQSVertexBuildData {
public:
    const PQ* buildQuad = nullptr;
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
    int gnomonicPlane = 0;
    double latitude = 0.0;
    double longitude = 0.0;

    void Reset() { buildQuad = nullptr; }  // PQS.cs:130
};

class PQS {
public:
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
    PQS(const PQS&) = delete;
    PQS& operator=(const PQS&) = delete;

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
    // build (PQS.cs:2273; the quad builder is WP-12, so they stay at the reset values here). TimeWarp.cs:529 reads max.
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
    int vertexIndex = 0;

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
    void SetupBuildDelegates(bool fakeBuild);  // PQS.cs:2177 (flags and radii; no delegates)

    double GetSurfaceHeight(const Vector3d& radialVector);                               // PQS.cs:3088
    double GetSurfaceHeight(const Vector3d& radialVector, bool overrideQuadBuildCheck);  // PQS.cs:3093 (internal)

    VertexBuildData& vbData() { return vbData_; }

    // Quad-builder hooks for quad-dependent mods (PQS.cs Mod_OnQuadPreBuild / Mod_OnQuadBuilt).
    void Mod_OnQuadPreBuild(const PQ& quad);
    void Mod_OnQuadBuilt(const PQ& quad);

private:
    void Mod_OnSetup();                                                              // PQS.cs:1827
    double Mod_GetVertexMaxHeight();                                                 // PQS.cs:1892
    double Mod_GetVertexMinHeight();                                                 // PQS.cs:1902
    void Mod_OnVertexBuildHeight(VertexBuildData& data, bool overrideQuadBuildCheck);  // PQS.cs:1956
    void BuildVertexMapCoords(VertexBuildData& data);                                // PQS.cs:2294

    VertexBuildData vbData_;
};

}  // namespace ksp
