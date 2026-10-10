// WP-26: ProceduralFairings/FairingXSection.cs, FairingPanel.cs, FairingPanelFlags.cs, MeshArc.cs, MeshPoint.cs.
//
// The panel geometry is computed as numbers: SetupPanelArray builds the panels of a fairing from its cross sections, BuildMesh
// computes the arc points (vertices, normals, uv, colours), the triangle index list and the barycentre, GetArea is the panel area
// that ModuleProceduralFairing turns into `panelMass`. Not ported (Unity objects, see STATUS.md WP-26): the GameObject / Mesh /
// MeshRenderer / material of Spawn / Despawn (AssignMesh fills the plain arrays of `PanelMesh`), the colliders
// (GeneratePanelColliders, ColliderChunk, ColliderSection), the opacity / exploded-view camera maths, the flag Part reparenting
// (AddFlag, ReadyFlagsForJettison), FairingXSection.CircleCast / ConeCast (Physics.Raycast).
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class ModuleProceduralFairing;

namespace ProceduralFairings {

// FairingPanelFlags.cs
struct FairingPanelFlags {
    std::vector<uint32_t> attachedFlagsPartIds;
    int panelIndex = -1;
};

// FairingXSection.cs (a [Serializable] class that panels share by reference: held in shared_ptr)
class FairingXSection {
public:
    float h = 0.0f;
    float r = 0.0f;
    bool isLast = false;
    bool isCap = false;
    bool isValid = false;
    Color color;
    std::vector<FairingPanelFlags> fairingPanelFlags;
    bool fairingPanelFlagsNull = true;  // the C# List is null until Load / AddNewFairingPanel / AddAttachedFlag

    FairingXSection() = default;                                  // :34
    explicit FairingXSection(bool isCap_) : isCap(isCap_) {}      // :39
    FairingXSection(std::shared_ptr<FairingXSection> from, std::shared_ptr<FairingXSection> to);  // :44
    bool IsLerp() const { return isLerp_; }                       // :32

    bool AddAttachedFlag(int panelIndex, uint32_t flagPartID);    // :64
    void AddNewFairingPanel(int panelIndex);                      // :83
    int GetPanelIndexFromFlag(uint32_t FlagID, uint32_t placementID);  // :104
    void RemoveAttachedFlag(uint32_t FlagID);                     // :127
    void Load(const ConfigNode& node);                            // :138
    void Save(ConfigNode& node) const;                            // :183
    void UpdateLerp(float t, float y);                            // :210
    static float GetSlopeAngle(const FairingXSection& from, const FairingXSection& to);  // :220
    int CompareTo(const FairingXSection& b) const;                // :276 (float.CompareTo)

private:
    std::shared_ptr<FairingXSection> lerp1_, lerp2_;
    bool isLerp_ = false;
};

// MeshPoint.cs (a class: arcs and the panel's point array share the objects)
struct MeshPoint {
    Vector3f vert, normal;
    Vector2f uv1, uv2;
    Color color;
    int index = 0;
    void Clear();                                                 // :19
};
using MeshPointPtr = std::shared_ptr<MeshPoint>;
using MeshPointArray = std::vector<MeshPointPtr>;                 // a null C# array is the empty vector

// MeshArc.cs
struct MeshArc {
    MeshPointArray inner, outer;
    std::shared_ptr<FairingXSection> xSection;
    std::vector<float> hOffsetOuter, hOffsetInner;                // empty until SetArcPoints (null in C#)
    Vector3f pivot;
    MeshArc(std::shared_ptr<FairingXSection> xs, MeshPointArray in, MeshPointArray out)
        : inner(std::move(in)), outer(std::move(out)), xSection(std::move(xs)) {}
};

// The arrays AssignMesh (FairingPanel.cs:386) gives the Unity Mesh
struct PanelMesh {
    std::vector<Vector3f> vertices, normals;
    std::vector<Vector2f> uv, uv2;
    std::vector<Color> colors;
    std::vector<int> triangles;
};

class FairingPanel {
public:
    int panelIndex = 0;
    bool isCap = false;
    bool isLast = false;
    std::shared_ptr<FairingXSection> bottom, top, persistentXSection;
    PanelMesh mesh;
    Vector3f pivotOffset;  // :64 (the panel GameObject's localPosition after BuildMesh)
    Vector3f explodedPos;  // :66

    // :93. `host` supplies nSides, axis, pivot, nArcs, edgeSlide, edgeWarp, aberrantNormalLimit
    FairingPanel(const std::vector<std::shared_ptr<FairingXSection>>& xSections, float arcFrom, float arcTo, ModuleProceduralFairing* host);
    bool ContainsSection(const FairingXSection* sec) const;       // :138
    // :152 (the materials are rendering state)
    static std::vector<std::shared_ptr<FairingPanel>> SetupPanelArray(int nSegments, std::vector<std::shared_ptr<FairingXSection>>& xSections,
                                                                    ModuleProceduralFairing* host, float capRadius);
    void CleanUp();                                               // :255
    float GetSlope() const;                                       // :269
    void BuildMesh(bool triangulate, const FairingPanel* prevPanel);  // :274
    Vector3f GetMidPoint() const;                                 // :427
    float GetArea() const;                                        // :624
    static Vector3f GetBarycenter(const MeshPointArray& pts);     // :375
    void RemoveFlag(uint32_t FlagID);                             // :830
    struct AttachedFlagParts { uint32_t flagVesselID = 0; };      // :11 (the Part / renderer members are the model's)
    std::vector<AttachedFlagParts> attachedFlagParts;
    bool attachedFlagPartsNull = true;
    const MeshArc& Arc(size_t i) const { return *Arcs_[i]; }
    size_t ArcCount() const { return Arcs_.size(); }
    int nSides() const { return nSides_; }
    const std::vector<int>& Tris() const { return tris_; }
    const MeshPointArray& Pts() const { return pts_; }

    // internal static helpers
    static std::vector<float> SetArcPoints(const Vector3f& pivot, const Vector3f& axis, float nArcs, const Vector3f& v0, const Vector3f& n0,
                                           const Quaternionf& slope, float fromAngle, float toAngle, float radius, float h, float arcOffset,
                                           float uOffset, float uTiling, const Color& vColor, MeshPointArray& arcPoints,
                                           const std::vector<float>* prevH, const MeshPointArray* prevPoints);  // :432
    static int SetPanelMeshPoints(MeshPointArray& pts, MeshPointArray& arc, int ptsOffset, bool aberrantNormals);  // :451
    static void TriangulatePanelMesh(const MeshPointArray& bottomArcOuter, const MeshPointArray& topArcOuter, const MeshPointArray& bottomArcInner,
                                     const MeshPointArray& topArcInner, int nSides, bool capTop, bool capBottom, std::vector<int>& tris);  // :468
    static void TriangulateQuad(int v00, int v01, int v0, int v1, std::vector<int>& tris);  // :512
    static void SetNoseConePoints(const Vector3f& pivot, const Vector3f& axis, const Vector3f& n0, float radius, float uOffset, MeshPointArray& pts);  // :522
    static void TriangulateConePanel(const MeshPointArray& bottomArcOuter, const MeshPointArray& topArcOuter, const MeshPointArray& bottomArcInner,
                                     const MeshPointArray& topArcInner, bool capTop, bool capBottom, std::vector<int>& tris);  // :531
    static void TriangulateTriangleMesh(const MeshPointArray& arcOuter, const MeshPointArray& arcInner, int nSidesOuter, int nSidesInner,
                                        std::vector<int>& tris, bool flip);  // :565
    static void TriangulateTri(int v00, int v0, int v1, std::vector<int>& tris, bool flip);  // :587
    static void LazyInitMeshArray(MeshPointArray& mArray, int length);  // :603

private:
    float wallThickness_ = 0.1f;
    float arcFrom_ = 0.0f, arcTo_ = 0.0f;
    ModuleProceduralFairing* host_ = nullptr;
    int nSides_ = 0;
    int pCount_ = 0;
    MeshPointArray pts_;
    std::vector<int> tris_;
    Quaternionf slope_ = Quaternionf::identity();
    std::vector<std::shared_ptr<MeshArc>> Arcs_;
    FairingPanel* topNeighbour_ = nullptr;
    FairingPanel* bottomNeighbour_ = nullptr;
    bool ptsNull_ = true, trisNull_ = true;
    void UpdateInterpolations();                                  // :261
    void SetArcPivot(MeshArc& arc);                               // :400
    void GetPanelSlope(const MeshArc& bottom, const MeshArc& top, Quaternionf& slope);  // :405
    float GetPanelHeight() const;                                 // :417
    float GetBaseHeight() const;                                  // :422
    void FetchExistingFlagReferences(const std::vector<std::shared_ptr<FairingXSection>>& xSections, int index);  // :212
};

}  // namespace ProceduralFairings
}  // namespace ksp
