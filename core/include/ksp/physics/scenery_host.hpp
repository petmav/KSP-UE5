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
class PQSCity;
class PQSCity2;
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

    // WP-12d (documents with a GameObject hierarchy, /2): the set follows Unity's activation (activeSelf / activeInHierarchy, see
    // CityColliderSet::EnableHierarchy), so a building made inactive stays down when a PQS callback re-activates its ancestors, and:
    //   SetFacilityLevel: the UpgradeableObject level of a group (UpgradeLevel.Spawn / Despawn): the objects of the other levels leave
    //   the scene, those of `level` enter it (active as the hierarchy above them is);
    //   SetObjectActiveSelf: GameObject.SetActive of one object (CollapsibleObject's collapse / replacement objects);
    //   SetObjectLocalPose: the object's transform.localPosition / localRotation (the collapse animation): its subtree's colliders follow.
    void SetFacilityLevel(const std::string& group, int level);
    void SetObjectActiveSelf(size_t object, bool active);
    void SetObjectLocalPose(size_t object, const Vector3f& localPosition, const Quaternionf& localRotation);
    const SceneryLevels& levels() const { return levels_; }
    bool Hierarchical() const { return hierarchical_; }

    // WP-12d: link the WP-42 city object of this scenery. PQSCity: its load / POI range events run inside Update (PQSCity.cs:234).
    // PQSCity2: also its setVisibility hook (this host's LOD objects), its re-orientation re-places this host (onOrientated ->
    // PlaceOnBody with the new local pose, when placed on a body), and its 7 km rule measures the city's distance from the physics
    // origin (where the core keeps what the floating origin does in KSP)
    void LinkCity(PQSCity& city);
    void LinkCity(PQSCity2& city);

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
    bool hierarchical_ = false;
    std::vector<SceneryObjectPose> poses_;
};

// PQSCity2.cs:342 OrientateRayCast over the bubble's static colliders, for PQSCity2::orientateRayCast: the city transform at
// surfPosNorm * surfaceHeight (float local position) in `body`'s terrain frame, a ray from 500 m above it straight down to
// maxDistance = (float)|start - sphere centre| - 500, closest hit on Unity layer 15 (mask 32768: the PQ quads and the scenery; hits of
// other layers are passed through), while the city's own colliders (`self`, if given) are moved 5 km up as transform.Translate does;
// returns surfaceHeight + (hit.distance - 500) and sets raycastCompleted on a hit. sic: a collider surface d below the analytic height
// raises the city by d (the correction has the wrong sign; STATUS.md WP-12d).
// Changed: the C# composes the ray in float world space under the floating origin; here the physics frame is double.
std::function<double(const Vector3d& surfPosNorm, double surfaceHeight, bool& raycastCompleted)> MakeOrientateRayCast(
    PhysicsBubble& bubble, const CelestialBody& body, SceneryCityHost* self = nullptr);

}  // namespace ksp::physics
