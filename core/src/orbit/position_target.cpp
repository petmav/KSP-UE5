// PositionTarget (PositionTarget.cs); see include/ksp/orbit/position_target.hpp.
#include "ksp/orbit/position_target.hpp"

#include "ksp/celestial/celestial_body.hpp"

namespace ksp {

// PositionTarget.cs:11 / :16 (displayName stays null = "" when not given)
PositionTarget::PositionTarget(const std::string& name) : g_(std::make_unique<UnityEngine::Transform>()), name_(name) {}
PositionTarget::PositionTarget(const std::string& name, const std::string& displayName)
    : g_(std::make_unique<UnityEngine::Transform>()), name_(name), displayName_(displayName) {}

// PositionTarget.cs:22
void PositionTarget::Update(CelestialBody& body, double latitude, double longitude) {
    const double altitude = body.TerrainAltitude(latitude, longitude);
    Update(body, latitude, longitude, altitude);
}

// PositionTarget.cs:28
void PositionTarget::Update(CelestialBody& body, double latitude, double longitude, double altitude) {
    Update(body.GetWorldSurfacePosition(latitude, longitude, altitude));
}

// PositionTarget.cs:33: g.transform.position = position (implicit Vector3d -> Vector3 conversion rounds each component)
void PositionTarget::Update(const Vector3d& position) { g_->position = position.ToVector3f(); }

// PositionTarget.cs:38: `return Vector3d.up;` converted to Vector3
Vector3f PositionTarget::GetFwdVector() { return Vector3d::up().ToVector3f(); }

// PositionTarget.cs:43
std::string PositionTarget::GetName() { return name_; }

// PositionTarget.cs:48
std::string PositionTarget::GetDisplayName() { return displayName_; }

// PositionTarget.cs:53
Vector3f PositionTarget::GetObtVelocity() { return Vector3f::zero(); }

// PositionTarget.cs:58
Orbit* PositionTarget::GetOrbit() { return nullptr; }

// PositionTarget.cs:63
OrbitDriver* PositionTarget::GetOrbitDriver() { return nullptr; }

// PositionTarget.cs:68
Vector3f PositionTarget::GetSrfVelocity() { return Vector3f::zero(); }

// PositionTarget.cs:73
UnityEngine::Transform* PositionTarget::GetTransform() { return g_ ? g_.get() : nullptr; }

// PositionTarget.cs:82
Vessel* PositionTarget::GetVessel() { return nullptr; }

// PositionTarget.cs:87
VesselTargetModes PositionTarget::GetTargetingMode() { return VesselTargetModes::Direction; }

// PositionTarget.cs:92
bool PositionTarget::GetActiveTargetable() { return false; }

}  // namespace ksp
