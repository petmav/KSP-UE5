// PositionTarget (PositionTarget.cs, 101 lines): a fixed point in the world used as a target (landing-site markers,
// ground targets). The GameObject is a ksp::UnityEngine::Transform stand-in; the finalizer's Object.Destroy(g) is the
// destructor.
#pragma once

#include <memory>
#include <string>

#include "ksp/math/vector3d.hpp"
#include "ksp/orbit/targetable.hpp"
#include "ksp/unity/transform.hpp"

namespace ksp {

class CelestialBody;

class PositionTarget : public ITargetable {
public:
    explicit PositionTarget(const std::string& name);                         // PositionTarget.cs:11
    PositionTarget(const std::string& name, const std::string& displayName);  // :16

    void Update(CelestialBody& body, double latitude, double longitude);                  // :22 (terrain altitude via PQS)
    void Update(CelestialBody& body, double latitude, double longitude, double altitude);  // :28
    void Update(const Vector3d& position);                                                // :33

    UnityEngine::Transform* GetTransform() override;   // :73 (null once destroyed)
    Vector3f GetObtVelocity() override;                // :53
    Vector3f GetSrfVelocity() override;                // :68
    Vector3f GetFwdVector() override;                  // :38
    Vessel* GetVessel() override;                      // :82
    std::string GetName() override;                    // :43
    std::string GetDisplayName() override;             // :48
    Orbit* GetOrbit() override;                        // :58
    OrbitDriver* GetOrbitDriver() override;            // :63
    VesselTargetModes GetTargetingMode() override;     // :87
    bool GetActiveTargetable() override;               // :92

private:
    std::unique_ptr<UnityEngine::Transform> g_;  // GameObject g = new GameObject() (its transform)
    std::string name_;
    std::string displayName_;
};

}  // namespace ksp
