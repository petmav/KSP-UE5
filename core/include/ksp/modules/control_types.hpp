// WP-24: ControlPoint.cs, SteeringObject.cs, ISteerable.cs, ITorqueProvider.cs (the small control types).
//
// Unity glue: the Transform members are PartTransform values (position / rotation / scale). ControlPoint.transform is the part's own
// transform for the "_default" point and a child GameObject (local rotation = Quaternion.Euler(orientation)) for the others; the module
// that creates them (ModuleCommand.OnStart) keeps the rotation in `localRotation` and the host resolves the world transform.
#pragma once

#include <memory>
#include <string>

#include "ksp/config/config_node.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// ControlPoint.cs:4
class ControlPoint {
public:
    std::string name;
    Vector3f orientation{};
    PartTransform transform{};         // ControlPoint.transform (world transform; the host refreshes it from the part)
    bool hasTransform = false;         // transform != null
    bool isPartTransform = false;      // the transform is the part's own (the "_default" point)
    Quaternionf localRotation = Quaternionf::identity();  // the child GameObject's localRotation (OnStart), identity otherwise
    std::string displayName;

    ControlPoint() = default;                                                                           // :17
    ControlPoint(std::string name, std::string displayName, const PartTransform* transform, const Vector3f& orientation);  // :21
    void Load(const ConfigNode& node);                                                                  // :32
};

// ISteerable.cs:1
class ISteerable {
public:
    virtual ~ISteerable() = default;
    virtual bool isSteerable() const = 0;
};

// ITorqueProvider.cs:3 (PartModule::AsTorqueProvider answers `module is ITorqueProvider` for the implementations that fit this signature;
// ModuleRCS / ModuleGimbal / the control surfaces take scene inputs and are queried through VesselSAS::TorqueProviderHook)
class ITorqueProvider {
public:
    virtual ~ITorqueProvider() = default;
    virtual void GetPotentialTorque(Vector3f& pos, Vector3f& neg) = 0;
};

// SteeringObject.cs:4
class SteeringObject {
public:
    enum class ControlAxis { Forward, Right, Up };
    enum class AlignmentAxis { Forward, Up, Right, None };

    PartTransform pivot{};
    Quaternionf neutralAngle = Quaternionf::identity();
    ControlAxis controlAxis = ControlAxis::Forward;
    AlignmentAxis alignmentAxis = AlignmentAxis::None;
    float wheelDriveInvert = 1.0f;

    // :31 (referenceTransform / partTransform are the world transforms of the two objects)
    void FindAlignmentAxis(const PartTransform& referenceTransform, const PartTransform& partTransform);
};

}  // namespace ksp
