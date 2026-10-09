// ITargetable (ITargetable.cs, 26 lines): what the targeting UI and the navball read from anything that can be a
// target (Vessel, CelestialBody, PositionTarget, DirectionTarget, docking ports ...). KSP's `Transform` is the engine
// transform stand-in of ksp/unity/transform.hpp.
#pragma once

#include <string>

#include "ksp/orbit/targeting_modes.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Orbit;
class OrbitDriver;
class Vessel;
namespace UnityEngine {
class Transform;
}

class ITargetable {
public:
    virtual ~ITargetable() = default;
    virtual UnityEngine::Transform* GetTransform() = 0;   // ITargetable.cs:5
    virtual Vector3f GetObtVelocity() = 0;                // :7
    virtual Vector3f GetSrfVelocity() = 0;                // :9
    virtual Vector3f GetFwdVector() = 0;                  // :11
    virtual Vessel* GetVessel() = 0;                      // :13
    virtual std::string GetName() = 0;                    // :15
    virtual std::string GetDisplayName() = 0;             // :17
    virtual Orbit* GetOrbit() = 0;                        // :19
    virtual OrbitDriver* GetOrbitDriver() = 0;            // :21
    virtual VesselTargetModes GetTargetingMode() = 0;     // :23
    virtual bool GetActiveTargetable() = 0;               // :25
};

}  // namespace ksp
