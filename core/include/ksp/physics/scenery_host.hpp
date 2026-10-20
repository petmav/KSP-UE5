// WP-12c: a PQSCity / PQSCity2 scenery object (the KSC, the island airfield, the stock launch sites, set dressing) hosted in the physics
// bubble: its colliders (data/scenery, ksp/space_center/scenery_colliders.hpp) are static Jolt bodies placed at the city's pose, registered
// with the ContactHost with the GameObject's layer, tag and name, so that Part.OnCollisionEnter / LandedCollisionChecks read what KSP reads:
// `c.gameObject.layer == 15`, `c.gameObject.tag` ("LaunchPad", "Runway", "KSC_Runway_09", ...) -> Vessel.SetLandedAt(tag), CrashObjectName
// upwards, PositionMobileLaunchPad in the parents.
//
// Usage (the API wheels / vessel placement use):
//
//   SceneryColliders doc = LoadSceneryColliders(".../core/data/scenery/scenery_colliders.json");
//   SceneryCityHost ksc(bubble, *doc.Find("Kerbin/KSC"), /*keyBase*/ 0x5343000000000000ull);
//   ksc.PlaceOnBody(kerbin, Vector3d(pqsCity.localPosition), QuaternionD(pqsCity.localRotation));   // WP-42 PQSCity::Orientate output
//   ksc.ActivateAll();                       // the PQS sphere is live with a target at the city (all LOD ranges in range)
//   // or drive PQSCity.OnUpdateFinished: ksc.Update(targetPositionInPhysicsFrame);
//
// The pose is valid in the body's rotating frame like WP-12b's terrain colliders (ContactHost::TerrainOrigin / TerrainRotation); the host
// re-places itself on ContactHost::RefreshTerrain (frame changes, physics origin rebases).
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/physics/contact_host.hpp"
#include "ksp/physics/physics_bubble.hpp"
#include "ksp/space_center/city_colliders.hpp"
#include "ksp/space_center/scenery_colliders.hpp"

namespace ksp {
class CelestialBody;
}

namespace ksp::physics {

class SceneryCityHost {
public:
    // `keyBase`: the static-collider key range of this city (CityColliderSet::KeyOf: keyBase + object << 16 + collider); distinct per city
    SceneryCityHost(PhysicsBubble& bubble, const SceneryCity& city, uint64_t keyBase, const SceneryLevels& levels = {});
    ~SceneryCityHost();
    SceneryCityHost(const SceneryCityHost&) = delete;
    SceneryCityHost& operator=(const SceneryCityHost&) = delete;

    // The city transform in the body's PQS-local frame (PQSCity.localPosition / localRotation: WP-42 PQSCity::Orientate, PQSCity2::Orientate)
    // -> physics frame through the host's terrain frame of the body: origin + rotation * local. Remembered for Refresh().
    void PlaceOnBody(const CelestialBody& body, const Vector3d& pqsLocalPosition, const QuaternionD& pqsLocalRotation);
    // The city transform directly in the physics frame (no body)
    void SetPose(const Vector3d& physicsPosition, const QuaternionD& physicsRotation);
    // Re-places everything from the remembered body frame (ContactHost::RefreshTerrain calls it)
    void Refresh();

    // PQSCity.OnSetup (once), OnSphereStart, OnSphereActive, OnUpdateFinished with the target at `target` (physics frame), or the sphere
    // without a target. The objects no callback switches are activated on the first call.
    void Update(const Vector3d& target, bool hasTarget = true);
    // The target at the city: every LOD range is in range
    void ActivateAll();
    // The sphere is gone (OnSphereInactive): every switched object is off (the always-active ones stay)
    void Deactivate();

    // DestructibleBuilding.CollapsibleObject.SetDestroyed (DestructibleBuilding.cs:163) on the destructible `index` of the city: the collapse
    // subtrees off and the replacement subtrees on (st) or back. Immediate; PQS callbacks that switch the same objects later win.
    void SetBuildingDestroyed(size_t index, bool destroyed);

    const SceneryCity& city() const { return city_; }
    CityColliderSet& set() { return pqsCity_ ? pqsCity_->set : pqsCity2_->set; }
    const CityColliderSet& set() const { return pqsCity_ ? pqsCity_->set : pqsCity2_->set; }
    PQSCityColliders* pqsCity() { return pqsCity_.get(); }
    PQSCity2Colliders* pqsCity2() { return pqsCity2_.get(); }
    uint64_t keyBase() const { return keyBase_; }
    uint64_t KeyOf(size_t objectIndex, size_t colliderIndex) const { return set().KeyOf(objectIndex, colliderIndex); }
    size_t BodyCount() const { return set().BodyCount(); }
    const Vector3d& position() const { return position_; }
    const QuaternionD& rotation() const { return rotation_; }

private:
    void Register(uint64_t key, const CityObject& object);
    void Apply();
    void EnsureStarted();

    ContactHost& host_;
    const SceneryCity& city_;
    uint64_t keyBase_;
    SceneryLevels levels_;
    std::unique_ptr<PQSCityColliders> pqsCity_;
    std::unique_ptr<PQSCity2Colliders> pqsCity2_;
    const CelestialBody* body_ = nullptr;
    Vector3d localPosition_;
    QuaternionD localRotation_ = QuaternionD::identity();
    Vector3d position_;
    QuaternionD rotation_ = QuaternionD::identity();
    uint64_t hook_ = 0;
    bool started_ = false;
    std::vector<size_t> initiallyActive_;
};

}  // namespace ksp::physics
