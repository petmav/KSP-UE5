// WP-40: EditorGizmos/GizmoOffset.cs and GizmoRotate.cs.
#include "ksp/editor/editor_gizmos.hpp"

#include <cmath>

#include "ksp/dotnet/math.hpp"

#include "ksp/editor/editor_logic.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp::EditorGizmos {

using QuaternionExtensions::Inverse;

namespace {
// float sqrt through ksp::dotnet (IEEE sqrt is correctly rounded: the double result rounded to float equals the float sqrt)
static inline float SqrtF(float x) { return static_cast<float>(dotnet::Math::Sqrt(static_cast<double>(x))); }
// Transform.rotation = q (Unity normalises on set)
void SetRotation(PartTransform& t, const Quaternionf& q) {
    const float mag = SqrtF(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    t.rotation = mag < 0.00001f ? Quaternionf::identity() : Quaternionf(q.x / mag, q.y / mag, q.z / mag, q.w / mag);
}
}  // namespace

Vector3f GizmoHost::localPosition() const { return parent != nullptr ? parent->InverseTransformPoint(transform->position) : transform->position; }
void GizmoHost::set_localPosition(const Vector3f& v) const { transform->position = parent != nullptr ? parent->TransformPoint(v) : v; }

// ---- GizmoOffset --------------------------------------------------------------------------------------------------------------------------

// GizmoOffset.cs:51
float GizmoOffset::SnapInterval() const { return EditorEnvironment::Current().fineTweak ? gridSnapIntervalFine : gridSnapInterval; }

// GizmoOffset.cs:67
GizmoOffset GizmoOffset::Attach(const GizmoHost& host, const Quaternionf& rotOffset, std::function<void(const Vector3f&)> onMove,
                                std::function<void(const Vector3f&)> onMoved) {
    GizmoOffset component;
    component.transform.position = host.transform->position;
    switch (coordSpace) {
    case UnityEngine::Space::Self: SetRotation(component.transform, host.transform->rotation * Inverse(rotOffset)); break;
    case UnityEngine::Space::World: component.transform.rotation = Quaternionf::identity(); break;
    }
    component.host = host;
    component.rotOffset = rotOffset;
    component.onGizmoMove = std::move(onMove);
    component.onGizmoMoved = std::move(onMoved);
    component.useGrid = EditorEnvironment::Current().useAngleSnap;
    return component;
}

// GizmoOffset.cs:146
void GizmoOffset::OnHandleMoveStart() {
    if (useGrid && coordSpace == UnityEngine::Space::World) SnapToGrid();
    offset0 = Vector3f::zero();
    trfPos0 = transform.position;
    isDragging = true;
}

// GizmoOffset.cs:166
void GizmoOffset::OnHandleMove(const Vector3f& axis, float distance) {
    offset = offset0 + axis * distance;
    transform.position = trfPos0 + offset;
    onGizmoMove(offset);
}

// GizmoOffset.cs:173
void GizmoOffset::OnHandleMoveEnd() {
    isDragging = false;
    onGizmoMoved(offset);
}

// GizmoOffset.cs:193
void GizmoOffset::SetCoordSystem(UnityEngine::Space space) {
    if (space != coordSpace) {
        switch (space) {
        case UnityEngine::Space::Self: SetRotation(transform, host.transform->rotation * Inverse(rotOffset)); break;
        case UnityEngine::Space::World: transform.rotation = Quaternionf::identity(); break;
        }
        offset0 = Vector3f::zero();
        trfPos0 = transform.position;
        coordSpace = space;
    }
}

// GizmoOffset.cs:212
void GizmoOffset::SnapToGrid() {
    const float snap = SnapInterval();
    auto round = [&](float v) { return Mathf::Round(f32store(static_cast<double>(v) / static_cast<double>(snap))) * snap; };
    if (coordSpace == UnityEngine::Space::Self) {
        const Vector3f localPosition = host.localPosition();
        const Vector3f localPosition2(round(localPosition.x), round(localPosition.y), round(localPosition.z));
        host.set_localPosition(localPosition2);
        transform.position = host.transform->position;
    } else {
        const Vector3f localPosition = host.parent != nullptr ? host.transform->position - host.parent->position : host.transform->position;
        const Vector3f localPosition2(round(localPosition.x), round(localPosition.y), round(localPosition.z));
        host.transform->position = host.parent != nullptr ? localPosition2 + host.parent->position : localPosition2;
        transform.position = host.transform->position;
    }
}

// ---- GizmoRotate --------------------------------------------------------------------------------------------------------------------------

// GizmoRotate.cs:46
float GizmoRotate::SnapDegrees() const { return EditorEnvironment::Current().fineTweak ? gridSnapIntervalFine : gridSnapInterval; }

// GizmoRotate.cs:66
GizmoRotate GizmoRotate::Attach(const GizmoHost& host, const Vector3f& pivotPoint, const Quaternionf& rotOffset,
                                std::function<void(const Quaternionf&)> onRotate, std::function<void(const Quaternionf&)> onRotated) {
    GizmoRotate component;
    component.transform.position = pivotPoint;
    component.host = host;
    component.onGizmoRotate = std::move(onRotate);
    component.onGizmoRotated = std::move(onRotated);
    component.useAngleSnap = EditorEnvironment::Current().useAngleSnap;
    component.hostRot0 = host.transform->rotation;
    component.rotOffset = rotOffset;
    switch (coordSpace) {
    case UnityEngine::Space::Self: SetRotation(component.transform, host.transform->rotation * Inverse(rotOffset)); break;
    case UnityEngine::Space::World: component.transform.rotation = Quaternionf::identity(); break;
    }
    return component;
}

// GizmoRotate.cs:130
void GizmoRotate::OnHandleRotateStart() {
    if (useAngleSnap && coordSpace == UnityEngine::Space::World) SnapToGrid();
    rot0 = transform.rotation;
    isDragging = true;
    if (host.transform != nullptr) hostRot0 = host.transform->rotation;
}

// GizmoRotate.cs:151
void GizmoRotate::OnHandleRotate(const Vector3f& axis, float dAngle) {
    if (!std::isnan(dAngle)) {
        if (coordSpace == UnityEngine::Space::Self) SetRotation(transform, UnityNative::AngleAxis(dAngle, axis) * rot0);
        onGizmoRotate(UnityNative::AngleAxis(dAngle, axis));
    }
}

// GizmoRotate.cs:163
void GizmoRotate::OnHandleRotateEnd(const Vector3f& axis, float deltaAngle) {
    isDragging = false;
    onGizmoRotated(UnityNative::AngleAxis(deltaAngle, axis));
}

// GizmoRotate.cs:185
void GizmoRotate::SetCoordSystem(UnityEngine::Space space) {
    if (space != coordSpace) {
        switch (space) {
        case UnityEngine::Space::Self: SetRotation(transform, host.transform->rotation * Inverse(rotOffset)); break;
        case UnityEngine::Space::World: transform.rotation = Quaternionf::identity(); break;
        }
        rot0 = transform.rotation;
        coordSpace = space;
    }
}

// GizmoRotate.cs:203
void GizmoRotate::SnapToGrid() {
    const Vector3f eulerAngles = UnityNative::EulerAngles(host.transform->rotation);
    const float snap = SnapDegrees();
    auto round = [&](float v) { return Mathf::Round(f32store(static_cast<double>(v) / static_cast<double>(snap))) * snap; };
    const Vector3f euler(round(eulerAngles.x), round(eulerAngles.y), round(eulerAngles.z));
    SetRotation(*host.transform, UnityNative::Euler(euler));
}

}  // namespace ksp::EditorGizmos
