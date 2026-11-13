// WP-12c: SceneryCityHost (see ksp/physics/scenery_host.hpp).
#include "ksp/physics/scenery_host.hpp"

#include <algorithm>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/space_center/pqs_city.hpp"

namespace ksp::physics {

SceneryCityHost::SceneryCityHost(PhysicsBubble& bubble, const SceneryCity& city, uint64_t keyBase, const SceneryLevels& levels)
    : host_(bubble.Contacts()), city_(city), keyBase_(keyBase), levels_(levels), hierarchical_(city.hierarchical) {
    CityColliderSet* set = nullptr;
    if (city.className == "PQSCity2") {
        pqsCity2_ = std::make_unique<PQSCity2Colliders>(bubble.World(), keyBase);
        set = &pqsCity2_->set;
        if (hierarchical_) {   // WP-12d: the LOD lists name the switched GameObjects themselves (Unity's SetActive semantics)
            AddSceneryToSet(*set, city);
            for (const SceneryLod& l : city.lod) {
                PQSCity2Colliders::LodObject o;
                o.visibleRange = l.visibleRange;
                o.KeepActive = l.keepActive;
                o.objects = l.objects;
                pqsCity2_->objects.push_back(std::move(o));
            }
        } else {
            ApplySceneryLod(*pqsCity2_, city, levels);
        }
    } else {
        pqsCity_ = std::make_unique<PQSCityColliders>(bubble.World(), keyBase);
        set = &pqsCity_->set;
        if (hierarchical_) {
            AddSceneryToSet(*set, city);
            pqsCity_->children = city.children;
            for (const SceneryLod& l : city.lod) {
                PQSCityColliders::LODRange r;
                r.visibleRange = l.visibleRange;
                r.objects = l.objects;
                pqsCity_->lod.push_back(std::move(r));
            }
        } else {
            ApplySceneryLod(*pqsCity_, city, levels);
        }
    }
    set->onPlaced = [this](uint64_t key, const CityObject& o) { Register(key, o); };
    set->onRemoved = [this](uint64_t key) { host_.UnregisterStatic(key); };
    if (hierarchical_) {
        std::vector<int> parents;
        std::vector<bool> activeSelf;
        for (const SceneryObjectSpec& o : city.objects) {
            parents.push_back(o.parent);
            activeSelf.push_back(o.active);
        }
        set->SetLive(false);   // nothing in the world before the city starts (EnsureStarted)
        set->EnableHierarchy(std::move(parents), std::move(activeSelf), SceneryPresence(city, levels));
        poses_ = SceneryObjectPoses(city);
    } else {
        initiallyActive_ = SceneryInitiallyActive(city, levels);
    }
    hook_ = host_.AddRefreshHook([this] { Refresh(); });
}

SceneryCityHost::~SceneryCityHost() {
    host_.RemoveRefreshHook(hook_);
    // the sets' destructors remove the static bodies; their onRemoved would reach a dying host: detach first
    set().onRemoved = nullptr;
    for (size_t i = 0; i < set().ObjectCount(); ++i)
        for (size_t c = 0; c < set().Object(i).colliders.size(); ++c) host_.UnregisterStatic(set().KeyOf(i, c));
    set().onPlaced = nullptr;
}

void SceneryCityHost::Register(uint64_t key, const CityObject& o) {
    StaticColliderInfo info;
    info.layer = o.layer;
    info.tag = o.tag;
    info.name = o.name;
    info.crashObjectName = o.crashObjectName;
    info.isMobileLaunchPad = o.mobileLaunchPad;
    host_.RegisterStatic(key, info);
}

void SceneryCityHost::SetPose(const Vector3d& physicsPosition, const QuaternionD& physicsRotation) {
    position_ = physicsPosition;
    rotation_ = physicsRotation;
    set().SetPose(position_, rotation_);
}

void SceneryCityHost::PlaceOnBody(const CelestialBody& body, const Vector3d& pqsLocalPosition, const QuaternionD& pqsLocalRotation) {
    body_ = &body;
    localPosition_ = pqsLocalPosition;
    localRotation_ = pqsLocalRotation;
    Refresh();
}

void SceneryCityHost::Refresh() {
    if (body_ == nullptr) return;
    const QuaternionD br = host_.TerrainRotation(*body_);
    SetPose(host_.TerrainOrigin(*body_) + br * localPosition_, br * localRotation_);
}

void SceneryCityHost::EnsureStarted() {
    if (started_) return;
    started_ = true;
    if (pqsCity_) {
        pqsCity_->OnSetup();                // PQSCity.cs:168
        pqsCity_->OnSphereStart(false);     // :179 the sphere is not alive yet: the children are off until a target is found
    } else {
        pqsCity2_->OnSetup();               // PQSCity2.cs:407
    }
    for (size_t i : initiallyActive_) set().SetObjectActive(i, true);
    if (hierarchical_) set().SetLive(true);
}

void SceneryCityHost::Update(const Vector3d& target, bool hasTarget) {
    EnsureStarted();
    if (pqsCity_) pqsCity_->OnUpdateFinished(hasTarget, target);
    else pqsCity2_->OnUpdateFinished(hasTarget, target);
}

void SceneryCityHost::ActivateAll() { Update(position_, true); }

void SceneryCityHost::Deactivate() {
    EnsureStarted();
    if (pqsCity_) pqsCity_->OnSphereInactive();
    else pqsCity2_->OnSphereInactive();
}

// CollapsibleObject.SetDestroyed (DestructibleBuilding.cs, declaration at line 163) applied to a whole building
void SceneryCityHost::SetBuildingDestroyed(size_t index, bool destroyed) {
    const SceneryDestructible& d = city_.destructibles.at(index);
    if (hierarchical_) {   // collapseObject.SetActive(!st), replacementObject.SetActive(st)
        for (const SceneryCollapsible& c : d.collapsibles) {
            set().SetObjectActive(c.collapse, !destroyed);
            if (c.replacement >= 0) set().SetObjectActive(static_cast<size_t>(c.replacement), destroyed);
        }
        return;
    }
    const std::vector<bool> present = SceneryPresence(city_, levels_);
    for (const SceneryCollapsible& c : d.collapsibles) {
        for (size_t i : SceneryActiveSubtree(city_, c.collapse, present)) set().SetObjectActive(i, !destroyed);
        if (c.replacement >= 0)
            for (size_t i : SceneryActiveSubtree(city_, static_cast<size_t>(c.replacement), present)) set().SetObjectActive(i, destroyed);
    }
}

void SceneryCityHost::SetFacilityLevel(const std::string& group, int level) {
    levels_.level[group] = level;
    if (!hierarchical_) return;
    const std::vector<bool> present = SceneryPresence(city_, levels_);
    for (size_t i = 0; i < present.size(); ++i) set().SetPresent(i, present[i]);
}

void SceneryCityHost::SetObjectActiveSelf(size_t object, bool active) { set().SetObjectActive(object, active); }

// the object's city-frame pose with the new local TRS (parent pose composed as SceneryObjectPoses does) as a rigid transform of its
// subtree: T = new * original^-1
void SceneryCityHost::SetObjectLocalPose(size_t object, const Vector3f& localPosition, const Quaternionf& localRotation) {
    if (!hierarchical_) return;
    const SceneryObjectPose& o = poses_.at(object);
    const int parent = city_.objects.at(object).parent;
    const SceneryObjectPose root;
    const SceneryObjectPose& p = parent >= 0 ? poses_.at(static_cast<size_t>(parent)) : root;
    const Vector3d scaled(static_cast<double>(p.scale.x) * localPosition.x, static_cast<double>(p.scale.y) * localPosition.y,
                          static_cast<double>(p.scale.z) * localPosition.z);
    const Vector3d newPos = Vector3d(p.position) + QuaternionD(p.rotation) * scaled;
    const QuaternionD newRot = QuaternionD(p.rotation) * QuaternionD(localRotation);
    const QuaternionD rot = newRot * QuaternionD::Inverse(QuaternionD(o.rotation));
    set().SetObjectTransform(object, newPos - rot * Vector3d(o.position), rot);
}

void SceneryCityHost::LinkCity(PQSCity& c) {
    if (pqsCity_) pqsCity_->city = &c;
}

void SceneryCityHost::LinkCity(PQSCity2& c) {
    if (!pqsCity2_) return;
    pqsCity2_->Link(c);
    c.onOrientated = [this, &c] {
        if (body_ != nullptr) PlaceOnBody(*body_, Vector3d(c.localPosition), QuaternionD(c.localRotation));
    };
    c.distanceFromOrigin = [this] { return Vector3f::Distance(position_.ToVector3f(), Vector3f::zero()); };
}

// PQSCity2.cs:342 (see the header)
std::function<double(const Vector3d&, double, bool&)> MakeOrientateRayCast(PhysicsBubble& bubble, const CelestialBody& body, SceneryCityHost* self) {
    return [&bubble, &body, self](const Vector3d& surfPosNorm, double surfaceHeight, bool& raycastCompleted) {
        ContactHost& ch = bubble.Contacts();
        PhysicsWorld& world = bubble.World();
        double num = 0.0;
        const Vector3f localPosition = (surfPosNorm * surfaceHeight).ToVector3f();   // transform.localPosition = planetRelativePosition
        const Vector3d centre = ch.TerrainOrigin(body);                              // sphere.PrecisePosition
        const Vector3d position = centre + ch.TerrainRotation(body) * Vector3d(localPosition);   // transform.position (PositioningPoint)
        const Vector3d normalized = (position - centre).normalized();
        const float num2 = 500.0f;
        const Vector3d vector = position + normalized * 500.0;
        Vector3d savedPosition;
        QuaternionD savedRotation = QuaternionD::identity();
        if (self != nullptr) {   // base.transform.Translate(normalized * 5000.0, Space.World)
            savedPosition = self->position();
            savedRotation = self->rotation();
            self->SetPose(savedPosition + normalized * 5000.0, savedRotation);
        }
        const float maxDistance = f32store(fx(static_cast<float>((vector - centre).magnitude())) - 500.0);
        // Physics.Raycast(vector, -normalized, out hitInfo, maxDistance, 32768): the closest layer-15 hit
        const Vector3d dir = normalized * -1.0;
        double travelled = 0.0;
        PhysicsWorld::RayHit hit;
        for (int i = 0; i < 64 && travelled < static_cast<double>(maxDistance); ++i) {
            hit = world.RayCast(vector + dir * travelled, dir, static_cast<double>(maxDistance) - travelled, PhysicsWorld::kLayerStatic);
            if (!hit.hit) break;
            if (ch.StaticInfo(hit.userData).layer == 15) {
                hit.distance += travelled;
                break;
            }
            travelled += hit.distance + 1e-3;
            hit.hit = false;
        }
        if (hit.hit) {
            num = static_cast<double>(static_cast<float>(hit.distance)) - static_cast<double>(num2);   // RaycastHit.distance is a float
            raycastCompleted = true;
        }
        if (self != nullptr) self->SetPose(savedPosition, savedRotation);   // base.transform.position = position
        return surfaceHeight + num;
    };
}

}  // namespace ksp::physics
