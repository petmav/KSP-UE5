// WP-29e: the Transform operations of VPWheelCollider.UpdateVisualWheel (VehiclePhysics/VPWheelCollider.cs:241-301) on a part's model rig.
//
// UpdateVisualWheel moves the suspension, caliper (steering pivot) and wheel transforms of the model to the current suspension position every frame
// (Transform.position = m_suspensionPosition; localRotation = Euler(spin, steer, 0)); the colliders below them move with them in KSP (PhysX syncs
// the shape poses from the transforms). The VPWheelCollider port (ksp/wheels/vehicle_physics.hpp) keeps the statements and calls the Transform
// operations through VPWheelCollider::VisualTransformHost; this is the host that answers them on a mu::PartModelRig:
//   setPosition        Transform.position = p            -> PartModelRig::SetPartSpacePosition(node, partTransform.InverseTransformPoint(p))
//   setLocalRotation   Transform.localRotation = q       -> the node's local rotation
//   isChildOf          Transform.IsChildOf               -> PartModelRig::IsChildOf
//   localEulerAnglesX  Transform.localEulerAngles.x      -> UnityNative::EulerAngles of the node's local rotation
//   activeInHierarchy / setActive   the wheel transform's GameObject (hideWheelOnDisable): one flag, `*wheelHidden`
// `nodeOf` maps a NativeTransform handed to the module (the host's pose objects) to its rig node, -1 when unknown.
#pragma once

#include <functional>

#include "ksp/parts/part_model_rig.hpp"
#include "ksp/wheels/vehicle_physics.hpp"

namespace ksp::physics {

VehiclePhysics::VPWheelCollider::VisualTransformHost MakeRigVisualHost(mu::PartModelRig& rig, std::function<VehiclePhysics::NativeTransform()> partPose,
                                                                       std::function<int(const VehiclePhysics::NativeTransform*)> nodeOf, bool* wheelHidden);

}  // namespace ksp::physics
