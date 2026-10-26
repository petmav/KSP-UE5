// WP-29e: see include/ksp/physics/wheel_visual.hpp. C# paths are relative to decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/physics/wheel_visual.hpp"

#include "ksp/unity/quaternion_native.hpp"

namespace ksp::physics {

using namespace VehiclePhysics;

// The Transform setters / getters VPWheelCollider.UpdateVisualWheel uses (VehiclePhysics/VPWheelCollider.cs:198-204, :245-253, :275-300) on the rig
VPWheelCollider::VisualTransformHost MakeRigVisualHost(mu::PartModelRig& rig, std::function<NativeTransform()> partPose, std::function<int(const NativeTransform*)> nodeOf,
                                                       bool* wheelHidden) {
    VPWheelCollider::VisualTransformHost h;
    mu::PartModelRig* r = &rig;
    h.setPosition = [r, partPose, nodeOf](NativeTransform* t, const Vector3f& world) {
        const int node = nodeOf(t);
        if (node < 0) return;
        r->SetPartSpacePosition(node, partPose().InverseTransformPoint(world));
    };
    h.setLocalRotation = [r, nodeOf](NativeTransform* t, const Quaternionf& q) {
        const int node = nodeOf(t);
        if (node >= 0) r->SetLocal(node, r->At(node).lp, q);
    };
    h.isChildOf = [r, nodeOf](NativeTransform* child, NativeTransform* ancestor) {
        const int c = nodeOf(child), a = nodeOf(ancestor);
        return c >= 0 && a >= 0 && r->IsChildOf(c, a);
    };
    h.localEulerAnglesX = [r, nodeOf](NativeTransform* t) {
        const int node = nodeOf(t);
        return node >= 0 ? UnityNative::EulerAngles(r->At(node).lr).x : 0.0f;
    };
    h.activeInHierarchy = [wheelHidden](NativeTransform*) { return wheelHidden == nullptr || !*wheelHidden; };
    h.setActive = [wheelHidden](NativeTransform*, bool active) {
        if (wheelHidden != nullptr) *wheelHidden = !active;
    };
    return h;
}

}  // namespace ksp::physics
