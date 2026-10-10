// WP-24: ControlPoint.cs, SteeringObject.cs (see ksp/modules/control_types.hpp).
#include "ksp/modules/control_types.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

// ControlPoint.cs:21
ControlPoint::ControlPoint(std::string name_, std::string displayName_, const PartTransform* transform_, const Vector3f& orientation_)
    : name(std::move(name_)), orientation(orientation_), displayName(std::move(displayName_)) {
    if (transform_ != nullptr) {
        transform = *transform_;
        hasTransform = true;
    }
}

// ControlPoint.cs:32 (a missing name / displayName is null in C#, kept as ""; KSPUtil.ParseVector3(null) throws)
void ControlPoint::Load(const ConfigNode& node) {
    name = node.GetValue("name").value_or("");
    displayName = node.GetValue("displayName").value_or("");
    const std::optional<std::string> o = node.GetValue("orientation");
    if (!o) throw NullReferenceException();
    orientation = ConfigNode::ParseVector3(*o);
}

// SteeringObject.cs:31
void SteeringObject::FindAlignmentAxis(const PartTransform& referenceTransform, const PartTransform& partTransform) {
    alignmentAxis = AlignmentAxis::None;
    float f = 0.0f;
    float f2 = 0.0f;
    float f3 = 0.0f;
    const Vector3f refForward = referenceTransform.forward(), refRight = referenceTransform.rotation * Vector3f::right(),
                   refUp = referenceTransform.up();
    const Vector3f pForward = partTransform.forward(), pRight = partTransform.rotation * Vector3f::right(), pUp = partTransform.up();
    switch (controlAxis) {
    case ControlAxis::Forward:
        f = Vector3f::Dot(refForward, pForward);
        f2 = Vector3f::Dot(refRight, pForward);
        f3 = Vector3f::Dot(refUp, pForward);
        break;
    case ControlAxis::Right:
        f = Vector3f::Dot(refForward, pRight);
        f2 = Vector3f::Dot(refRight, pRight);
        f3 = Vector3f::Dot(refUp, pRight);
        break;
    case ControlAxis::Up:
        f = Vector3f::Dot(refForward, pUp);
        f2 = Vector3f::Dot(refRight, pUp);
        f3 = Vector3f::Dot(refUp, pUp);
        break;
    }
    if (Mathf::Abs(f) > Mathf::Abs(f3)) {
        if (Mathf::Abs(f) > Mathf::Abs(f2)) {
            alignmentAxis = AlignmentAxis::Forward;
            wheelDriveInvert = f;
        } else {
            alignmentAxis = AlignmentAxis::Right;
            wheelDriveInvert = f2;
        }
    } else if (Mathf::Abs(f3) > Mathf::Abs(f2)) {
        alignmentAxis = AlignmentAxis::Up;
        wheelDriveInvert = f3;
    } else {
        alignmentAxis = AlignmentAxis::Right;
        wheelDriveInvert = f2;
    }
    if (fx(Mathf::Abs(wheelDriveInvert)) >= 0.7) {
        wheelDriveInvert = f32store(fx(wheelDriveInvert) / fx(Mathf::Abs(wheelDriveInvert)));
    } else {
        wheelDriveInvert = 0.0f;
    }
}

}  // namespace ksp
