// WP-40: EditorGizmos/GizmoOffset.cs and GizmoRotate.cs -- the maths of the editor's offset and rotate gizmos: coordinate system (world / self),
// grid and angle snapping, the drag deltas handed to the editor callbacks. The gizmo prefab (handles, hover / drag state, the camera facing
// culling of GizmoOffset.LateUpdate, screen-space scaling, the flight-scene Update follow) is the engine shell's: a drag is driven through
// OnHandleMoveStart / OnHandleMove / OnHandleMoveEnd and OnHandleRotateStart / OnHandleRotate / OnHandleRotateEnd, which are the C# handler bodies.
#pragma once

#include <functional>

#include "ksp/editor/editor_types.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::EditorGizmos {

// The Transform a gizmo is attached to (the editor passes the selected part's reference transform) and the Transform it is a child of
struct GizmoHost {
    PartTransform* transform = nullptr;   // host
    PartTransform* parent = nullptr;      // host.parent (null: the transform is a root)
    Vector3f localPosition() const;                    // Transform.localPosition
    void set_localPosition(const Vector3f& v) const;   // Transform.localPosition = v
};

// GizmoOffset.cs:6
class GizmoOffset {
public:
    float gridSnapInterval = 0.2f;        // :20
    float gridSnapIntervalFine = 0.1f;    // :22
    bool useGrid = false;                 // :24
    PartTransform transform{};            // the gizmo's own transform
    GizmoHost host;                       // :28
    Quaternionf rotOffset = Quaternionf::identity();   // :30
    std::function<void(const Vector3f&)> onGizmoMove = [](const Vector3f&) {};    // :32
    std::function<void(const Vector3f&)> onGizmoMoved = [](const Vector3f&) {};   // :36
    bool isDragging = false;              // :40
    Vector3f trfPos0{}, offset0{}, offset{};   // :42-46
    static inline UnityEngine::Space coordSpace = UnityEngine::Space::World;   // :48 (`private static Space coordSpace;` = World)

    float SnapInterval() const;                                     // :62
    bool IsDragging() const { return isDragging; }                  // :74
    UnityEngine::Space CoordSpace() const { return coordSpace; }    // :76
    // :78 Attach(host, rotOffset, onMove, onMoved)
    static GizmoOffset Attach(const GizmoHost& host, const Quaternionf& rotOffset, std::function<void(const Vector3f&)> onMove,
                              std::function<void(const Vector3f&)> onMoved);
    void OnHandleMoveStart();                                       // :158
    void OnHandleMove(const Vector3f& axis, float distance);        // :178
    void OnHandleMoveEnd();                                         // :185
    void onEditorSnapChanged(bool useSnap) { useGrid = useSnap; }   // :201
    void SetCoordSystem(UnityEngine::Space space);                  // :206
    void SnapToGrid();                                              // :226
};

// GizmoRotate.cs:6
class GizmoRotate {
public:
    float gridSnapInterval = 15.0f;       // :16
    float gridSnapIntervalFine = 5.0f;    // :18
    bool useAngleSnap = false;            // :20
    PartTransform transform{};            // the gizmo's own transform
    GizmoHost host;                       // :26
    Vector3f pivot{};                     // :24
    Quaternionf rot0 = Quaternionf::identity();      // :26
    Quaternionf hostRot0 = Quaternionf::identity();  // :28
    Quaternionf rotOffset = Quaternionf::identity(); // :30
    std::function<void(const Quaternionf&)> onGizmoRotate = [](const Quaternionf&) {};    // :34
    std::function<void(const Quaternionf&)> onGizmoRotated = [](const Quaternionf&) {};   // :38
    bool isDragging = false;              // :42
    static inline UnityEngine::Space coordSpace = UnityEngine::Space::Self;   // :14

    UnityEngine::Space CoordSpace() const { return coordSpace; }   // :53
    float SnapDegrees() const;                                     // :55
    const Quaternionf& HostRot0() const { return hostRot0; }       // :65
    bool IsDragging() const { return isDragging; }                 // :67
    // :73 Attach(host, pivotPoint, rotOffset, onRotate, onRotated)
    static GizmoRotate Attach(const GizmoHost& host, const Vector3f& pivotPoint, const Quaternionf& rotOffset,
                              std::function<void(const Quaternionf&)> onRotate, std::function<void(const Quaternionf&)> onRotated);
    void Start() { rot0 = transform.rotation; }                    // :94
    void OnHandleRotateStart();                                    // :130
    void OnHandleRotate(const Vector3f& axis, float dAngle);       // :151
    void OnHandleRotateEnd(const Vector3f& axis, float deltaAngle);   // :163
    void onAngleSnapChanged(bool useSnap) { useAngleSnap = useSnap; }   // :180
    void SetCoordSystem(UnityEngine::Space space);                 // :185
    void SnapToGrid();                                             // :203
};

}  // namespace ksp::EditorGizmos
