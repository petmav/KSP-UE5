// ManeuverNode (ManeuverNode.cs); see include/ksp/orbit/maneuver_node.hpp.
#include "ksp/orbit/maneuver_node.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/patched_conic_solver.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace {
const Orbit& Req(const std::shared_ptr<Orbit>& o, const char* what) {
    if (!o) throw NullReferenceException(what);
    return *o;
}
}  // namespace

// ManeuverGizmo.cs:94 Setup (patch fields; the renderer link and button state are UI)
void ManeuverGizmoState::Setup(const ManeuverNode& node) {
    patchBefore = node.patch;
    patchAhead = node.nextPatch;
}

// ManeuverGizmo.cs:102
void ManeuverGizmoState::SetPatches(std::shared_ptr<Orbit> patch, std::shared_ptr<Orbit> nextPatch, bool reset) {
    patchBefore = std::move(patch);
    patchAhead = std::move(nextPatch);
    if (reset) {
        prevUT = -1.0;
        prevprevUT = -1.0;
    }
}

// ManeuverNode.cs:28 AttachGizmo(gizmoPrefab, renderer): Instantiate, SetActive, name, DeltaV/UT, the three callbacks
// (OnGizmoUpdated, OnMinimize = DetachGizmo, OnDelete = RemoveSelf: the shell calls them), Setup.
void ManeuverNode::AttachGizmo() {
    attachedGizmo = std::make_unique<ManeuverGizmoState>();
    attachedGizmo->active = true;
    attachedGizmo->DeltaV = DeltaV;
    attachedGizmo->UT = UT;
    attachedGizmo->Setup(*this);
}

// ManeuverNode.cs:41
void ManeuverNode::OnGizmoUpdated(Vector3d dV, double ut) {
    const Vector3d deltaV = DeltaV;
    const double uT = UT;
    DeltaV = dV;
    UT = ut;
    if (solver == nullptr) throw NullReferenceException("ManeuverNode.OnGizmoUpdated: solver");
    solver->UpdateFlightPlan();
    if (attachedGizmo == nullptr) return;
    if (patch == nullptr) {
        DeltaV = deltaV;
        UT = uT;
        solver->UpdateFlightPlan();
        return;
    }
    bool reset = false;
    const Orbit& before = Req(attachedGizmo->patchBefore, "ManeuverNode.OnGizmoUpdated: attachedGizmo.patchBefore");
    if (refocusCamera && (before.referenceBody->bodyName != patch->referenceBody->bodyName ||
                          dotnet::Math::Abs(before.StartUT - patch->StartUT) > 100.0)) {
        if (OnRefocusCamera) OnRefocusCamera(patch->referenceBody);
        reset = true;
    }
    attachedGizmo->SetPatches(patch, nextPatch, reset);
}

// ManeuverNode.cs:72
void ManeuverNode::DetachGizmo() {
    if (attachedGizmo != nullptr) {
        attachedGizmo.reset();  // attachedGizmo.Terminate(); attachedGizmo = null
        GameEvents::onManeuverNodeDeselected.Fire();
    }
}

// ManeuverNode.cs:82
void ManeuverNode::RemoveSelf() {
    DetachGizmo();
    if (scaledSpaceTarget != nullptr) {
        if (OnRemoveScaledSpaceTarget) OnRemoveScaledSpaceTarget(scaledSpaceTarget);
    }
    if (solver == nullptr) throw NullReferenceException("ManeuverNode.RemoveSelf: solver");
    solver->RemoveManeuverNode(this);  // may destroy this node (last owner): nothing below touches it
}

// ManeuverNode.cs:93
Vector3d ManeuverNode::GetBurnVector(const Orbit* currentOrbit) const {
    if (currentOrbit != nullptr && nextPatch != nullptr) {
        if (currentOrbit->referenceBody != nextPatch->referenceBody)
            return (nextPatch->getOrbitalVelocityAtUT(UT) - Req(patch, "ManeuverNode.GetBurnVector: patch").getOrbitalVelocityAtUT(UT)).xzy();
        return (nextPatch->getOrbitalVelocityAtUT(UT) - currentOrbit->getOrbitalVelocityAtUT(UT)).xzy();
    }
    return Vector3d::zero();
}

// ManeuverNode.cs:105
void ManeuverNode::Load(const ConfigNode& node) {
    if (node.HasValue("UT")) UT = dotnet::ParseDouble(*node.GetValue("UT"));
    if (node.HasValue("dV")) DeltaV = KSPUtil::ParseVector3d(*node.GetValue("dV"));
}

// ManeuverNode.cs:117
void ManeuverNode::Save(ConfigNode& node) const {
    node.AddValue("UT", UT);
    node.AddValue("dV", KSPUtil::WriteVector(DeltaV));
}

// ManeuverNode.cs:123
Vector3d ManeuverNode::GetPartialDv() const {
    if (patch != nullptr && nextPatch != nullptr) {
        const Vector3d vector3d = nextPatch->getOrbitalVelocityAtUT(UT) - patch->getOrbitalVelocityAtUT(UT);
        const Vector3d normalized = patch->getOrbitalVelocityAtUT(UT).normalized();
        const Vector3d normalized2 = patch->GetOrbitNormal().normalized();
        return QuaternionD::Inverse(QuaternionD::LookRotation(normalized, normalized2)) * vector3d;
    }
    return Vector3d::zero();
}

// PatchedConicRenderer.cs:200-204
void ManeuverNode::UpdateNodeRotation() {
    const Orbit& p = Req(patch, "ManeuverNode.UpdateNodeRotation: patch");
    const Vector3d xzy = p.getRelativePositionAtUT(UT).xzy();
    const Vector3d xzy2 = p.getOrbitalVelocityAtUT(UT).xzy();
    nodeRotation = QuaternionD::LookRotation(xzy2, Vector3d::Cross(-xzy, xzy2)).ToQuaternionf();
    if (attachedGizmo != nullptr) attachedGizmo->rotation = nodeRotation;
}

}  // namespace ksp
