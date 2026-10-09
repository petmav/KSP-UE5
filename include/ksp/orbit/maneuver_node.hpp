// ManeuverNode (ManeuverNode.cs, 133 lines): a planned burn on a vessel's patched-conic flight plan.
//   DeltaV      the burn in the node frame (x = radial-out... in KSP's gizmo terms: x radial, y normal, z prograde): the
//               solver applies it as QuaternionD.LookRotation(vel, Cross(-pos, vel)) * DeltaV in Unity axes (forward =
//               velocity = prograde, up = Cross(-pos, vel) = orbit normal, right = Cross(up, forward) = radial-out),
//               see PatchedConicSolver::CheckNextManeuver (PatchedConicSolver.cs:259).
//   GetBurnVector  world-axis (Unity xzy) velocity change at UT: nextPatch velocity - current orbit velocity
//   GetPartialDv   the remaining change in the (prograde, normal) LookRotation frame of `patch`
//   nodeRotation   PatchedConicRenderer's gizmo rotation (PatchedConicRenderer.cs:202), UpdateNodeRotation() below
// The ManeuverGizmo (ManeuverGizmo.cs, WP-44 map view) is kept as plain data (ManeuverGizmoState); the UI shell renders
// it and calls OnGizmoUpdated / DetachGizmo / RemoveSelf. Camera and map-object side effects go through the static hooks.
//
// Reference semantics: nodes are shared_ptr owned by PatchedConicSolver::maneuverNodes; patch / nextPatch are
// shared_ptr to the solver's Orbit objects (C# references keep them alive).
#pragma once

#include <functional>
#include <memory>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/unity/quaternion.hpp"

namespace ksp {

class CelestialBody;
class ConfigNode;
class MapObject;
class ManeuverNode;
class Orbit;
class PatchedConicSolver;

// ManeuverGizmo (ManeuverGizmo.cs) as far as ManeuverNode reads and writes it.
struct ManeuverGizmoState {
    Vector3d DeltaV;                   // ManeuverGizmo.DeltaV
    double UT = 0.0;                   // ManeuverGizmo.UT
    std::shared_ptr<Orbit> patchBefore;  // ManeuverGizmo.cs:28
    std::shared_ptr<Orbit> patchAhead;   // ManeuverGizmo.cs:30
    double prevUT = 0.0;               // reset to -1 by SetPatches(reset: true)
    double prevprevUT = 0.0;
    Quaternionf rotation;              // transform.rotation (= node.nodeRotation, PatchedConicRenderer.cs:204)
    bool active = false;               // gameObject.SetActive(true) by AttachGizmo

    void Setup(const ManeuverNode& node);  // ManeuverGizmo.cs:94 (renderer / UpdateOrbitButtons: UI)
    void SetPatches(std::shared_ptr<Orbit> patch, std::shared_ptr<Orbit> nextPatch, bool reset);  // ManeuverGizmo.cs:102
};

class ManeuverNode {
public:
    double UT = 0.0;
    Vector3d DeltaV;
    Quaternionf nodeRotation;
    std::unique_ptr<ManeuverGizmoState> attachedGizmo;  // null = no gizmo
    PatchedConicSolver* solver = nullptr;
    MapObject* scaledSpaceTarget = nullptr;  // map-view object (WP-44), not owned
    std::shared_ptr<Orbit> patch;
    std::shared_ptr<Orbit> nextPatch;
    bool refocusCamera = true;
    double startBurnIn = 0.0;  // auto-property

    void AttachGizmo();                             // ManeuverNode.cs:28 (no prefab: the state object is created)
    void OnGizmoUpdated(Vector3d dV, double ut);    // ManeuverNode.cs:41
    void DetachGizmo();                             // ManeuverNode.cs:72
    void RemoveSelf();                              // ManeuverNode.cs:82
    Vector3d GetBurnVector(const Orbit* currentOrbit) const;  // ManeuverNode.cs:93
    void Load(const ConfigNode& node);              // ManeuverNode.cs:105
    void Save(ConfigNode& node) const;              // ManeuverNode.cs:117
    Vector3d GetPartialDv() const;                  // ManeuverNode.cs:123
    // PatchedConicRenderer.cs:200-202 (LateUpdate, nodes with an attached gizmo): nodeRotation =
    // QuaternionD.LookRotation(vel.xzy, Cross(-pos.xzy, vel.xzy)) of `patch` at UT, converted to Quaternion; the gizmo's
    // rotation follows.
    void UpdateNodeRotation();

    // Side effects KSP performs on Unity objects (no-ops when unset):
    // OnGizmoUpdated: PlanetariumCamera.fetch.SetTarget(patch.referenceBody.MapObject)
    static inline std::function<void(CelestialBody*)> OnRefocusCamera;
    // RemoveSelf: MapView.MapCamera.RemoveTarget(scaledSpaceTarget); scaledSpaceTarget.Terminate()
    static inline std::function<void(MapObject*)> OnRemoveScaledSpaceTarget;
};

}  // namespace ksp
