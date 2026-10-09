// WP-05 part data model, building blocks: AttachRules (AttachRules.cs), the Part field table (every public instance
// field of Part.cs / CompoundPart.cs, generated: part_fields.inc), AttachNode (AttachNode.cs), DragCube (DragCube.cs),
// DragCubeList data side (DragCubeList.cs LoadCubes/SaveCubes/ClearCubes), PartResource (PartResource.cs).
//
// The Part prefab is a plain record here (PartPrefab, available_part.hpp): kspcore loads data, it does not create Unity
// components. Field names, defaults and C# type names follow the decompiled declarations exactly so that ApplyPartValue
// (reflection over Part's public fields in C#) can be reproduced with the generated table.
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/aero/float_eval.hpp"  // WP-21: aero float evaluation policies
#include "ksp/math/vector3d.hpp"
#include "ksp/parts/ksp_enums.hpp"
#include "ksp/physics/physics_globals.hpp"  // WP-21: the curves DragCubeList.SetPart takes
#include "ksp/unity/color.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;

using NullableString = std::optional<std::string>;
using NullableStringList = std::optional<std::vector<std::string>>;

// AttachRules.cs
struct AttachRules {
    bool stack = false;
    bool srfAttach = false;
    bool allowStack = false;
    bool allowSrfAttach = false;
    bool allowCollision = false;
    bool allowDock = false;
    bool allowRotate = false;
    bool allowRoot = false;
    bool StackOrSurfaceAttachable() const { return srfAttach || stack; }
    // AttachRules.cs:34: elements compared to "1" exactly (no trimming); fewer than 5 throws Exception
    static AttachRules Parse(std::string_view value);
    // AttachRules.cs:74: the first 7 flags ("allowRoot" is not written)
    std::string String() const;
};
using NullableAttachRules = std::optional<AttachRules>;

// ---- Part / CompoundPart public instance fields (generated) -----------------------------------------------------------
enum class FieldKind { Single, Double, Int32, UInt32, Boolean, String, Vector3, Vector3d, Quaternion, Color, Enum,
                       AttachRules, StringList, Object };

#define KSP_PF_DECL_Single(n, t, d) t n = d;
#define KSP_PF_DECL_Double(n, t, d) t n = d;
#define KSP_PF_DECL_Int32(n, t, d) t n = d;
#define KSP_PF_DECL_UInt32(n, t, d) t n = d;
#define KSP_PF_DECL_Boolean(n, t, d) t n = d;
#define KSP_PF_DECL_String(n, t, d) t n = d;
#define KSP_PF_DECL_Vector3(n, t, d) t n = d;
#define KSP_PF_DECL_Vector3d(n, t, d) t n = d;
#define KSP_PF_DECL_Quaternion(n, t, d) t n = d;
#define KSP_PF_DECL_Color(n, t, d) t n = d;
#define KSP_PF_DECL_Enum(n, t, d) t n = d;
#define KSP_PF_DECL_AttachRules(n, t, d) t n = d;
#define KSP_PF_DECL_StringList(n, t, d) t n = d;
#define KSP_PF_DECL_Object(n, t, d)

// Part.cs public instance fields with their C# initializers (Object-typed fields have no storage: a fresh prefab holds
// null/empty there and the loader cannot set them).
struct PartFields {
#define KSP_PART_FIELD(name, kind, cppType, csType, enumTable, def, line) KSP_PF_DECL_##kind(name, cppType, def)
#include "ksp/parts/part_fields.inc"
#undef KSP_PART_FIELD
};

// CompoundPart.cs's own public instance fields (`module = CompoundPart`).
struct CompoundPartFields {
#define KSP_COMPOUNDPART_FIELD(name, kind, cppType, csType, enumTable, def, line) KSP_PF_DECL_##kind(name, cppType, def)
#include "ksp/parts/compound_part_fields.inc"
#undef KSP_COMPOUNDPART_FIELD
};

// One reflected field (FieldInfo stand-in).
struct PartFieldInfo {
    const char* name;
    FieldKind kind;
    const char* typeName;       // FieldInfo.FieldType.Name ("Single", "Vector3", "List`1", "AttachRules", ...)
    const EnumTable* enumTable;  // for FieldKind::Enum
    int line;                    // declaration line in Part.cs / CompoundPart.cs
    bool compoundPart;           // declared by CompoundPart
    void* (*address)(PartFields&, CompoundPartFields&);  // nullptr for Object fields
};

// Part's fields in declaration order (Part.cs), then CompoundPart's own fields.
const std::vector<PartFieldInfo>& PartFieldTable();
// Type.GetField(name) on Part (compound = false) or on CompoundPart (its own fields first, then Part's).
const PartFieldInfo* FindPartField(std::string_view name, bool compoundPart);

// ---- AttachNode.cs ---------------------------------------------------------------------------------------------------
struct AttachNode {
    std::string id;
    Vector3f originalPosition{};
    Vector3f originalOrientation{};
    Vector3f originalSecondaryAxis{};
    Vector3f position{0.0f, 0.0f, 0.0f};
    Vector3f orientation{0.0f, 1.0f, 0.0f};
    Vector3f secondaryAxis{0.0f, 0.0f, 0.0f};
    int32_t size = 0;
    float radius = 0.4f;
    float breakingForce = 0.0f;
    float breakingTorque = 0.0f;
    Vector3f offset{0.0f, 0.0f, 0.0f};
    std::string srfAttachMeshName;
    AttachNodeMethod attachMethod = AttachNodeMethod::FIXED_JOINT;
    AttachNodeType nodeType = AttachNodeType::Stack;
    bool requestGate = false;
    bool ResourceXFeed = true;
    bool AllowOneWayXFeed = true;
    bool rigid = false;
    float contactArea = 0.0f;
    float overrideDragArea = -1.0f;
    std::string nodeTransform;  // NODE{transform=} name (Part.AddAttachNode); empty for node_* values
    uint32_t attachedPartId = 0;
    // ---- WP-13 (AttachNode.cs): the live links of a node on an instantiated part
    Part* owner = nullptr;         // AttachNode.owner
    Part* attachedPart = nullptr;  // AttachNode.attachedPart
    float nodeDistEpsilon = 0.001f;  // private, [SerializeField]

    // AttachNode.cs:86 (the transform constructor: position = transform.position, orientation = transform.forward; the
    // originals the same; secondaryAxis zero)
    static AttachNode FromTransform(std::string id, const Vector3f& transformPosition, const Vector3f& transformForward,
                                    int32_t size, AttachNodeMethod attachMethod, bool crossfeed, bool rigid);
    // AttachNode.cs:107 FindAttachedPart: attachedPart = parts.GetPartByCraftID(attachedPartId)
    void FindAttachedPart(const std::vector<Part*>& parts);
    // AttachNode.cs:112 FindOpposingNode
    AttachNode* FindOpposingNode() const;
    // AttachNode.cs:129 Clone (MemberwiseClone: a plain copy)
    static AttachNode Clone(const AttachNode& referenceNode) { return referenceNode; }
    // AttachNode.cs:132 ReverseSrfNodeDirection
    void ReverseSrfNodeDirection(AttachNode& fromNode);
    // AttachNode.cs:143 ChangeSrfNodePosition (internal)
    void ChangeSrfNodePosition();
};

// ---- DragCube.cs -----------------------------------------------------------------------------------------------------
class DragCube {
public:
    // DragCube.cs:6 DragFace (WP-21)
    enum class DragFace : int32_t { XP = 0, XN = 1, YP = 2, YN = 3, ZP = 4, ZN = 5 };
    DragCube() = default;
    explicit DragCube(std::string name) : name_(std::move(name)) {}
    std::array<float, 6>& Area() { return area_; }
    std::array<float, 6>& Drag() { return drag_; }
    std::array<float, 6>& Depth() { return depth_; }
    std::array<float, 6>& DragModifiers() { return dragModifiers_; }
    const std::array<float, 6>& Area() const { return area_; }
    const std::array<float, 6>& Drag() const { return drag_; }
    const std::array<float, 6>& Depth() const { return depth_; }
    const std::array<float, 6>& DragModifiers() const { return dragModifiers_; }
    const Vector3f& Center() const { return center_; }
    const Vector3f& Size() const { return size_; }
    const std::string& Name() const { return name_; }
    float Weight() const { return weight_; }
    void SetCenter(const Vector3f& v) { center_ = v; }
    void SetSize(const Vector3f& v) { size_ = v; }
    void SetName(std::string n) { name_ = std::move(n); }
    void SetWeight(float w) { weight_ = w; }
    // DragCube.cs:106 (24 values: name = data[0] and the last index is out of range -> std::out_of_range, sic)
    bool Load(const std::vector<std::string>& data);
    // DragCube.cs:196 (G4 formatting)
    std::string SaveToString() const;

private:
    std::array<float, 6> area_{};
    std::array<float, 6> drag_{};
    std::array<float, 6> depth_{};
    std::array<float, 6> dragModifiers_{};
    Vector3f center_{};
    Vector3f size_{};
    std::string name_;
    float weight_ = 1.0f;
};

// DragCubeList.cs: the data side (WP-05: LoadCubes / SaveCubes / ClearCubes) and the aerodynamics (WP-21: SetPart, cube
// weights, SetDragWeights, SetPartOcclusion, SetDrag / AddSurfaceDragDirection, the per-direction queries, ForceUpdate).
// The float maths is templated over the evaluation policy (ksp/aero/float_eval.hpp): the default MonoFloat is the game,
// NetFloat reproduces the .NET harness golden; both are explicitly instantiated in src/aero/drag_cube_list.cpp.
class DragCubeList {
public:
    // DragCubeList.cs:8 CubeData (a struct: copied by value)
    struct CubeData {
        Vector3f dragVector{};
        Vector3f liftForce{};
        float area = 0.0f;
        float areaDrag = 0.0f;
        float depth = 0.0f;
        float crossSectionalArea = 0.0f;
        float exposedArea = 0.0f;
        float dragCoeff = 0.0f;
        float taperDot = 0.0f;
        void Reset();  // DragCubeList.cs:41
    };
    // What DragCubeList reads from Unity / DragCubeSystem (procedural cubes are rendered from the model; unset = no
    // procedural cube is ever rendered, SetDragWeights_Procedural returns false)
    struct Hooks {
        std::function<float()> realtimeSinceStartup;          // Time.realtimeSinceStartup
        std::function<float()> proceduralDragUpdateInterval;  // DragCubeSystem.Instance.ProceduralDragUpdateInterval
        std::function<DragCube(Part&)> renderProceduralDragCube;  // DragCubeSystem.Instance.RenderProceduralDragCube(part)
    };
    static inline Hooks hooks;

    DragCubeList() = default;
    // UnityEngine.Object.Instantiate of the owning Part: the [SerializeField] members are copied (cubes deep-copied,
    // postOcclusionArea, none, procedural, cubeData, debugDragCube), every other member gets its C# initialiser
    // (part = null, both update flags true, the weighted arrays zeroed, occlusionMultiplier 1, no drag-vector rotation,
    // the [NonSerialized] curve references null).
    void CopySerializedFrom(const DragCubeList& original);

    // ---- properties (DragCubeList.cs:138-202) ----
    float AreaDrag() const { return cubeData_.areaDrag; }
    float Area() const { return cubeData_.area; }
    float Depth() const { return cubeData_.depth; }
    float CrossSectionalArea() const { return cubeData_.crossSectionalArea; }
    float ExposedArea() const { return cubeData_.exposedArea; }
    float PostOcclusionArea() const { return postOcclusionArea_; }
    const Vector3f& LiftForce() const { return cubeData_.liftForce; }
    float DragCoeff() const { return cubeData_.dragCoeff; }
    float TaperDot() const { return cubeData_.taperDot; }
    const Vector3f& DragVector() const { return cubeData_.dragVector; }
    Part* GetPart() const { return part_; }
    float OcclusionMultiplier() const { return occlusionMultiplier_; }
    std::array<float, 6>& WeightedArea() { return weightedArea_; }
    std::array<float, 6>& WeightedDrag() { return weightedDrag_; }
    std::array<float, 6>& WeightedDepth() { return weightedDepth_; }
    std::array<float, 6>& AreaOccluded() { return areaOccluded_; }
    const std::array<float, 6>& WeightedArea() const { return weightedArea_; }
    const std::array<float, 6>& WeightedDrag() const { return weightedDrag_; }
    const std::array<float, 6>& WeightedDepth() const { return weightedDepth_; }
    const std::array<float, 6>& AreaOccluded() const { return areaOccluded_; }
    const Vector3f& WeightedCenter() const { return weightedCenter_; }
    const Vector3f& WeightedSize() const { return weightedSize_; }
    bool RotateDragVector() const { return rotateDragVector_; }
    const Quaternionf& DragVectorRotation() const { return dragVectorRotation_; }
    const CubeData& GetCubeData() const { return cubeData_; }
    bool WeightsRequireUpdate() const { return weightsRequireUpdate_; }
    bool OcclusionRequireUpdate() const { return occlusionRequireUpdate_; }

    // ---- aerodynamics (WP-21) ----
    void SetPart(Part* part);  // DragCubeList.cs:204
    template <class P = aero::MonoFloat>
    void SetDragWeights();  // DragCubeList.cs:214
    template <class P = aero::MonoFloat>
    void SetPartOcclusion();  // DragCubeList.cs:301
    template <class P = aero::MonoFloat>
    void SetDrag(const Vector3f& vector, float machNumber);  // DragCubeList.cs:352
    template <class P = aero::MonoFloat>
    void AddSurfaceDragDirection(const Vector3f& direction, float machNumber, CubeData& retData) const;  // DragCubeList.cs:367
    template <class P = aero::MonoFloat>
    float GetCubeAreaDir(const Vector3f& dir, const std::array<float, 6>& areas) const;  // DragCubeList.cs:421
    template <class P = aero::MonoFloat>
    float GetCubeAreaDir(const Vector3f& dir) const { return GetCubeAreaDir<P>(dir, areaOccluded_); }  // :436
    template <class P = aero::MonoFloat>
    float GetCubeCoeffDir(const Vector3f& dir) const;  // DragCubeList.cs:441
    template <class P = aero::MonoFloat>
    float GetCubeDepthDir(const Vector3f& dir) const;  // DragCubeList.cs:458
    static Vector3f GetFaceDirection(DragCube::DragFace direction);  // DragCubeList.cs:475
    static const std::array<Vector3f, 6>& FaceDirections();          // DragCubeList.cs:128 faceDirections
    void SetOcclusionMultiplier(float multiplier);   // DragCubeList.cs:786
    DragCube* GetCube(std::string_view name);         // DragCubeList.cs:792 (last match)
    void SetCubeWeight(std::string_view name, float weight);  // DragCubeList.cs:806
    void SetDragVectorRotation(const Quaternionf& rotation);  // DragCubeList.cs:824
    void SetDragVectorRotation(bool rotateDragVector);        // DragCubeList.cs:830
    void ResetCubeWeights(float newWeight);           // DragCubeList.cs:835
    void ResetCubeWeights() { ResetCubeWeights(1.0f); }  // DragCubeList.cs:853
    bool LoadCube(const DragCubeList& cubes, std::string_view whichOne);  // DragCubeList.cs:939
    void RequestOcclusionUpdate() { occlusionRequireUpdate_ = true; }     // DragCubeList.cs:962
    void ForceUpdate(bool weights, bool occlusion, bool resetProcTiming = false);  // DragCubeList.cs:967
    bool DebugDragCube() const { return debugDragCube_; }
    void SetDebugDragCube(bool v) { debugDragCube_ = v; }

    // [NonSerialized] public fields set by SetPart (references to PhysicsGlobals' curves)
    const FloatCurve* DragCurveCd = nullptr;
    const FloatCurve* DragCurveCdPower = nullptr;
    const FloatCurve* DragCurveMultiplier = nullptr;
    std::shared_ptr<PhysicsGlobals::LiftingSurfaceCurve> BodyLiftCurve;
    PhysicsGlobals::SurfaceCurvesList SurfaceCurves{};

    std::vector<DragCube>& Cubes() { return cubes_; }
    const std::vector<DragCube>& Cubes() const { return cubes_; }
    bool None() const { return none_; }
    bool Procedural() const { return procedural_; }
    void SetNone(bool v) { none_ = v; }
    void SetProcedural(bool v) { procedural_ = v; }
    void ClearCubes();                          // DragCubeList.cs:858 (procedural = false; `none` kept: sic)
    ConfigNode::NodePtr SaveCubes() const;      // DragCubeList.cs:864 (cubes written last to first)
    bool LoadCubes(const ConfigNode& node);     // DragCubeList.cs:885 (cube values read last to first)
    bool LoadCubes(const DragCubeList& cubes);  // DragCubeList.cs:919

private:
    std::vector<DragCube> cubes_;
    bool none_ = false;
    bool procedural_ = false;
    // ---- WP-21 (DragCubeList.cs:66-126)
    float postOcclusionArea_ = 0.0f;
    Part* part_ = nullptr;
    CubeData cubeData_{};
    bool debugDragCube_ = false;
    bool weightsRequireUpdate_ = true;
    bool occlusionRequireUpdate_ = true;
    std::optional<DragCube> proceduralDragCube_;  // null until the first procedural render
    float proceduralDragNextUpdate_ = 0.0f;
    float occlusionMultiplier_ = 1.0f;
    std::array<float, 6> weightedArea_{};
    std::array<float, 6> weightedDrag_{};
    std::array<float, 6> weightedDragOrig_{};
    std::array<float, 6> weightedDepth_{};
    std::array<float, 6> areaOccluded_{};
    Vector3f weightedCenter_{};
    Vector3f weightedSize_{};
    bool rotateDragVector_ = false;
    Quaternionf dragVectorRotation_ = Quaternionf::identity();

    template <class P>
    void SetDragWeights_List();      // DragCubeList.cs:234
    bool SetDragWeights_Procedural();  // DragCubeList.cs:284
    template <class P>
    void AreaToCubeOperation(const Vector3f& direction, float area);  // DragCubeList.cs:750 (the only operation: Max(0, c - f))
};

class PartResourceDefinition;

// PartResource.cs
// `flowState` / `flowMode` are the C# backing fields `_flowState` / `_flowMode`; the C# properties of the same names (which
// fire onPartResourceFlowStateChange / onPartResourceFlowModeChange on a change) are SetFlowState / SetFlowMode.
struct PartResource {
    const PartResourceDefinition* info = nullptr;
    std::string resourceName;
    double amount = 0.0;
    double maxAmount = 0.0;
    bool flowState = false;  // _flowState
    bool isTweakable = false;
    bool isVisible = true;
    bool hideFlow = false;
    bool simulationResource = false;
    PartResourceFlowMode flowMode = PartResourceFlowMode::None;  // _flowMode
    Part* part = nullptr;               // PartResource.part (WP-13)

    PartResource() = default;
    explicit PartResource(Part* p) : part(p) {}                                          // PartResource.cs:70
    PartResource(Part* p, bool simulate) : simulationResource(simulate), part(p) {}      // :75
    PartResource(const PartResource& res) = default;                                     // :81 (part = res.part; Copy(res))
    PartResource(const PartResource& res, bool simulate) : PartResource(res) { simulationResource = simulate; }  // :87
    PartResource& operator=(const PartResource&) = default;

    void Load(const ConfigNode& node);  // PartResource.cs:108
    void Copy(const PartResource& res); // PartResource.cs:94
    void Save(ConfigNode& node) const;  // PartResource.cs:160 (name, amount, maxAmount, flowState, isTweakable, hideFlow, isVisible, flowMode)
    void SetFlowState(bool value);                      // PartResource.cs:36 flowState setter (fires the event on a change)
    void SetFlowMode(PartResourceFlowMode value);       // :53 flowMode setter
    void SetInfo(const PartResourceDefinition* i);      // :172
    std::string GetInfo() const;                        // :178
    bool CanProvide(double demand) const;               // :183
    bool CanProvide(bool pulling) const;                // :196
    bool Flowing(bool pulling) const;                   // :213
};

// The node_* attach node grammar shared by PartLoader.ParsePart (PartLoader.cs:740) and PartVariant.ParseNode
// (PartVariant.cs:515). `withOneWay`: ParsePart scales by `scale` and reads an 11th value (AllowOneWayXFeed); PartVariant does
// neither. Returns false (and logs) for fewer than 6 values.
bool ParseNodeValue(const ConfigNode::Value& v, float scale, bool withOneWay, AttachNode& n);

}  // namespace ksp
