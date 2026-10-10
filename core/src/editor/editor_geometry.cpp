// WP-40: EditorBounds.cs, EditorGeometryUtil.cs, EditorEnumExtensions.cs, EditorReRootUtil.cs.
#include "ksp/editor/editor_geometry.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/editor/editor_logic.hpp"
#include "ksp/editor/editor_reroot.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/util/hierarchy_util.hpp"

namespace ksp {

// ---- EditorBounds ---------------------------------------------------------------------------------------------------------------------

// EditorBounds.cs:31
bool EditorBounds::Awake() {
    if (Instance != nullptr) return false;
    Instance = this;
    return true;
}

// EditorBounds.cs:44
void EditorBounds::OnDestroy() {
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

// EditorBounds.cs:60
Vector3f EditorBounds::ClampToCameraBounds(Vector3f pos, const Vector3f& camFwd, float& clampHeight) {
    if (Instance != nullptr) {
        const Bounds& b = Instance->cameraOffsetBounds;
        pos.x = Mathf::Clamp(pos.x, 0.0f - b.extents().x, b.extents().x);
        pos.z = Mathf::Clamp(pos.z, 0.0f - b.extents().z, b.extents().z);
        const float num = f32store(static_cast<double>(Instance->cameraMaxDistance) * static_cast<double>(Mathf::Clamp01(1.0f - Vector3f::Dot(camFwd, Vector3f::up()))));
        const float num2 = f32store(static_cast<double>(Instance->cameraMaxDistance) * static_cast<double>(Mathf::Clamp01(1.0f - Vector3f::Dot(camFwd, Vector3f::down()))));
        const double lo = static_cast<double>(b.center().y) - static_cast<double>(b.extents().y) - static_cast<double>(num);
        const double hi = static_cast<double>(b.center().y) + static_cast<double>(b.extents().y) + static_cast<double>(num2);
        if (static_cast<double>(pos.y) < lo) clampHeight = f32store(static_cast<double>(clampHeight) - (static_cast<double>(pos.y) - lo));
        if (static_cast<double>(pos.y) > hi) clampHeight = f32store(static_cast<double>(clampHeight) - (static_cast<double>(pos.y) - hi));
        pos.y = Mathf::Clamp(pos.y, f32store(lo), f32store(hi));
    }
    return pos;
}

// EditorBounds.cs:80
float EditorBounds::ClampCameraDistance(float dist) {
    if (Instance != nullptr) dist = Mathf::Clamp(dist, Instance->cameraMinDistance, Instance->cameraMaxDistance);
    return dist;
}

// EditorBounds.cs:89 (centerSceneryOrigin :102: sceneryRoot.position = sceneryCenter.position)
void EditorBounds::CenterSceneryOrigin(Vector3f& sceneryRootPosition) {
    if (Instance != nullptr) sceneryRootPosition = Instance->sceneryCenter;
}

// ---- EditorGeometryUtil -----------------------------------------------------------------------------------------------------------------

Vector3f EditorGeometryUtil::ClosestPointOnBounds(const Bounds& b, const Vector3f& point) {
    const Vector3f mn = b.min(), mx = b.max();
    return {Mathf::Clamp(point.x, mn.x, mx.x), Mathf::Clamp(point.y, mn.y, mx.y), Mathf::Clamp(point.z, mn.z, mx.z)};
}

// EditorGeometryUtil.cs:5
bool EditorGeometryUtil::TestPartBoundsIntersect(Part& p1, Part& p2) { return GetPartBoundsSeparation(p1, p2).sqrMagnitude() < 0.1f; }

// EditorGeometryUtil.cs:10
bool EditorGeometryUtil::TestPartBoundsSeparate(Part& p1, Part& p2, float threshold, Vector3f& gap) {
    const Vector3f partBoundsSeparation = GetPartBoundsSeparation(p1, p2);
    gap = partBoundsSeparation - partBoundsSeparation.normalized() * threshold;
    return static_cast<double>(partBoundsSeparation.sqrMagnitude()) > static_cast<double>(threshold) * static_cast<double>(threshold);
}

// EditorGeometryUtil.cs:17 (the colliders are the parts' world-space collider bounds; Debug.DrawLine / DebugDrawUtil are not ported)
Vector3f EditorGeometryUtil::GetPartBoundsSeparation(Part& p1, Part& p2) {
    EditorEnvironment& env = EditorEnvironment::Current();
    float num = 3.40282347E+38f;
    Vector3f vector = Vector3f::zero();
    Vector3f vector2 = Vector3f::zero();
    auto valid = [&](Part& p, size_t i) { return !env.isValidPartCollider || env.isValidPartCollider(p, i); };
    int num2 = static_cast<int>(p1.colliderBounds.size());
    while (num2-- > 0) {
        const Bounds& collider = p1.colliderBounds[static_cast<size_t>(num2)];
        if (!valid(p1, static_cast<size_t>(num2))) continue;
        for (int num3 = static_cast<int>(p2.colliderBounds.size()) - 1; num3 >= 0; num3--) {
            const Bounds& collider2 = p2.colliderBounds[static_cast<size_t>(num3)];
            const bool same = &p1 == &p2 && num2 == num3;
            if (!same || valid(p2, static_cast<size_t>(num3))) {
                const Vector3f vector3 = ClosestPointOnBounds(collider2, EditorLogic::ReferenceTransform(p1).position);
                const Vector3f vector4 = ClosestPointOnBounds(collider, vector3);
                const float sqrMagnitude = (vector4 - vector3).sqrMagnitude();
                if (sqrMagnitude < num) {
                    num = sqrMagnitude;
                    vector = vector4;
                    vector2 = vector3;
                }
            }
        }
    }
    return vector - vector2;
}

// EditorGeometryUtil.cs:72
Quaternionf EditorGeometryUtil::MirrorRotation(const Quaternionf& input, const PartTransform& mirrorRoot, const PartTransform& setRoot) {
    (void)mirrorRoot;
    Vector3f direction = setRoot.InverseTransformDirection(input * Vector3f::up());
    Vector3f direction2 = setRoot.InverseTransformDirection(input * Vector3f::forward());
    direction.x *= -1.0f;
    direction2.x *= -1.0f;
    direction = setRoot.TransformDirection(direction);
    direction2 = setRoot.TransformDirection(direction2);
    const Quaternionf quaternion = UnityNative::LookRotation(direction2, direction);
    return UnityNative::AngleAxis(180.0f, direction) * quaternion;
}

// EditorGeometryUtil.cs:84
Vector3f EditorGeometryUtil::MirrorPos(const Vector3f& input, const PartTransform& mirrorRoot, const PartTransform& setRoot) {
    const Vector3f vector = input - mirrorRoot.position;
    const Vector3f vector2 = Vector3f::ProjectOnPlane(vector, setRoot.up() * Vector3f::Dot(setRoot.up(), vector));
    return mirrorRoot.position + (vector - vector2) + UnityNative::AngleAxis(180.0f, -setRoot.forward()) * vector2;
}

// EditorGeometryUtil.cs:91
Vector3f EditorGeometryUtil::MirrorDirection(const Vector3f& input, const PartTransform& setRoot) {
    const Vector3f vector = Vector3f::ProjectOnPlane(input, setRoot.up() * Vector3f::Dot(setRoot.up(), input));
    return input - vector + UnityNative::AngleAxis(180.0f, -setRoot.forward()) * vector;
}

// EditorGeometryUtil.cs:104
float EditorGeometryUtil::GetPixelDistance(const Vector3f& sPos1, const Vector3f& sPos2) { return Mathf::Sqrt((sPos1 - sPos2).sqrMagnitude()); }

// ---- EditorEnumExtensions -----------------------------------------------------------------------------------------------------------------

namespace EditorEnumExtensions {

// EditorEnumExtensions.cs:7
SpaceCenterFacility ToFacility(EditorFacility editor) {
    if (editor == EditorFacility::VAB) return SpaceCenterFacility::VehicleAssemblyBuilding;
    return SpaceCenterFacility::SpaceplaneHangar;
}

// EditorEnumExtensions.cs:17
EditorFacility ToEditor(SpaceCenterFacility facility) {
    switch (facility) {
    case SpaceCenterFacility::Runway:
    case SpaceCenterFacility::SpaceplaneHangar: return EditorFacility::SPH;
    case SpaceCenterFacility::LaunchPad:
    case SpaceCenterFacility::VehicleAssemblyBuilding: return EditorFacility::VAB;
    default: return EditorFacility::None;
    }
}

// EditorEnumExtensions.cs:32
SpaceCenterFacility GetEditorFacility(SpaceCenterFacility launchSite) {
    switch (launchSite) {
    case SpaceCenterFacility::Runway:
    case SpaceCenterFacility::SpaceplaneHangar: return SpaceCenterFacility::SpaceplaneHangar;
    default: return SpaceCenterFacility::VehicleAssemblyBuilding;
    }
}

// EditorEnumExtensions.cs:43
SpaceCenterFacility GetFacility(const std::string& name) {
    if (name == "LaunchPad") return SpaceCenterFacility::LaunchPad;
    if (name == "ResearchAndDevelopment" || name == "RnD") return SpaceCenterFacility::ResearchAndDevelopment;
    if (name == "TrackingStation") return SpaceCenterFacility::TrackingStation;
    if (name == "Runway") return SpaceCenterFacility::Runway;
    if (name == "VAB" || name == "VehicleAssemblyBuilding") return SpaceCenterFacility::VehicleAssemblyBuilding;
    if (name == "Administration") return SpaceCenterFacility::Administration;
    if (name == "FlagPole") return SpaceCenterFacility::AstronautComplex;   // Debug.LogError("FlagPole is not an actual Space Center Facility!")
    if (name == "SPH" || name == "SpaceplaneHangar") return SpaceCenterFacility::SpaceplaneHangar;
    if (name == "MissionControl") return SpaceCenterFacility::MissionControl;
    throw ArgumentNullException("pSystemFacility: " + name + " is not a valid Space Center Facility name!");
}

// EditorEnumExtensions.cs:81
std::string GetSymmetryMethodName(SymmetryMethod method) {
    switch (method) {
    case SymmetryMethod::Mirror: return Localizer::Format("#autoLOC_6004040");
    case SymmetryMethod::Radial: return Localizer::Format("#autoLOC_6004039");
    }
    return "Unlocalised symmetry method!";
}

}  // namespace EditorEnumExtensions

// ---- EditorReRootUtil ----------------------------------------------------------------------------------------------------------------------

// EditorReRootUtil.cs:8
std::vector<Part*> EditorReRootUtil::GetRootCandidates(const std::vector<Part*>& fromSet) {
    std::vector<Part*> list(fromSet);
    int count = static_cast<int>(list.size());
    while (count-- > 0)
        if (!isRootCandidate(list[static_cast<size_t>(count)])) list.erase(list.begin() + count);
    return list;
}

// EditorReRootUtil.cs:20
bool EditorReRootUtil::isRootCandidate(Part* p) {
    if (!(p == p->localRoot()) && !(p == context.selectedPart)) {
        const AttachRules rules = p->fields.attachRules.value_or(AttachRules{});
        if (!rules.allowRoot) return false;
        if (!rules.allowSrfAttach && !rules.allowStack) return false;
        if (rules.srfAttach && p->srfAttachNode != nullptr && p->srfAttachNode->attachedPart == nullptr) return true;
        if (rules.stack) {
            if (context.shipContains && context.shipContains(p)) return !p->attachNodes.empty();
            int count = static_cast<int>(p->attachNodes.size());
            do {
                if (count-- <= 0) return false;
            } while (!(p->attachNodes[static_cast<size_t>(count)].attachedPart == nullptr));
            return true;
        }
        return false;
    }
    return false;
}

// EditorReRootUtil.cs:58
bool EditorReRootUtil::MakeRoot(Part* newRoot, Part* oldRoot) {
    if (!isRootCandidate(newRoot)) return false;
    if (!HierarchyUtil::isDescendant(newRoot, oldRoot)) return false;
    newRoot->SetHierarchyRoot(newRoot);
    StripInvalidSymmetrySets(newRoot);
    return true;
}

// EditorReRootUtil.cs:75
void EditorReRootUtil::StripInvalidSymmetrySets(Part* setRoot) { StripInvalidSymmetrySets_recurseChildren(setRoot); }

// EditorReRootUtil.cs:80
void EditorReRootUtil::StripInvalidSymmetrySets_recurseChildren(Part* p) {
    if (!SymmetrySetIsValid(p)) {
        int count = static_cast<int>(p->symmetryCounterparts.size());
        while (count-- > 0) p->symmetryCounterparts[static_cast<size_t>(count)]->symmetryCounterparts.clear();
        p->symmetryCounterparts.clear();
    }
    int count2 = static_cast<int>(p->children.size());
    while (count2-- > 0) StripInvalidSymmetrySets_recurseChildren(p->children[static_cast<size_t>(count2)]);
}

// EditorReRootUtil.cs:82
bool EditorReRootUtil::SymmetrySetIsValid(Part* p) {
    int num = static_cast<int>(p->symmetryCounterparts.size()) - 1;
    if (num >= 0) {
        const int hierarchyDepth = HierarchyUtil::GetHierarchyDepth(p);
        int num2 = num;
        while (num2 >= 0) {
            if (HierarchyUtil::GetHierarchyDepth(p->symmetryCounterparts[static_cast<size_t>(num2)]) == hierarchyDepth) {
                num2--;
                continue;
            }
            return false;
        }
    }
    return true;
}

}  // namespace ksp
