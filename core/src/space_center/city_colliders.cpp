// WP-12b: collider activation of PQSCity / PQSCity2 objects (see ksp/space_center/city_colliders.hpp).
#include "ksp/space_center/city_colliders.hpp"

#include <algorithm>

#include "ksp/dotnet/math.hpp"

namespace ksp {

// ---- CityColliderSet: GameObject.SetActive for colliders (engine side, not KSP code) -----------------------------------------------

CityColliderSet::~CityColliderSet() {
    for (size_t i = 0; i < objects_.size(); ++i) Remove(i);
}

size_t CityColliderSet::Add(CityObject object) {
    Slot s;
    s.object = std::move(object);
    objects_.push_back(std::move(s));
    return objects_.size() - 1;
}

void CityColliderSet::Place(size_t index) {
    Slot& s = objects_[index];
    for (size_t c = 0; c < s.object.colliders.size(); ++c) {
        const CityCollider& col = s.object.colliders[c];
        const uint64_t key = KeyOf(index, c);
        const Vector3d pos = position_ + rotation_ * col.localPosition;
        const QuaternionD rot = rotation_ * col.localRotation;
        if (col.kind == CityCollider::Kind::Mesh) {
            // static meshes are placed with an identity rotation: rotate the vertices into the physics frame
            std::vector<Vector3f> v;
            v.reserve(col.vertices.size());
            for (const Vector3f& p : col.vertices) {
                const Vector3d r = rot * Vector3d(p);
                v.push_back(Vector3f(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.z)));
            }
            world_.AddStaticMesh(key, pos, v, col.triangles, col.friction);
        } else {
            world_.AddStaticShape(key, col.shape, pos, rot, col.friction);
        }
    }
}

void CityColliderSet::Remove(size_t index) {
    Slot& s = objects_[index];
    for (size_t c = 0; c < s.object.colliders.size(); ++c) world_.RemoveStatic(KeyOf(index, c));
}

void CityColliderSet::SetPose(const Vector3d& position, const QuaternionD& rotation) {
    position_ = position;
    rotation_ = rotation;
    for (size_t i = 0; i < objects_.size(); ++i)
        if (objects_[i].active) Place(i);
}

void CityColliderSet::SetObjectActive(size_t index, bool active) {
    Slot& s = objects_[index];
    if (s.active == active) return;
    s.active = active;
    if (active) {
        Place(index);
    } else {
        Remove(index);
    }
}

size_t CityColliderSet::ActiveObjectCount() const {
    size_t n = 0;
    for (const Slot& s : objects_) n += s.active ? 1 : 0;
    return n;
}

size_t CityColliderSet::BodyCount() const {
    size_t n = 0;
    for (size_t i = 0; i < objects_.size(); ++i)
        for (size_t c = 0; c < objects_[i].object.colliders.size(); ++c)
            n += world_.StaticBody(KeyOf(i, c)) != physics::kInvalidBody ? 1 : 0;
    return n;
}

// ---- PQSCity.cs ------------------------------------------------------------------------------------------------------------

// PQSCity.cs:22 (LODRange.Setup; the renderer list is the host's)
void PQSCityColliders::Setup(LODRange& r) { r.isActive = true; }

// PQSCity.cs:41 (LODRange.SetActive; renderers are the host's)
void PQSCityColliders::SetActive(LODRange& r, bool active) {
    if (active != r.isActive) {
        for (size_t o : r.objects) set.SetObjectActive(o, active);
    }
    r.isActive = active;
}

void PQSCityColliders::SetChildrenActive(bool active) {
    for (size_t o : children) set.SetObjectActive(o, active);
}

// PQSCity.cs:168
void PQSCityColliders::OnSetup() {
    std::stable_sort(lod.begin(), lod.end(), [](const LODRange& a, const LODRange& b) { return a.visibleRange < b.visibleRange; });
    size_t num = lod.size();
    while (num-- > 0) {
        Setup(lod[num]);
        SetActive(lod[num], false);
    }
}

// PQSCity.cs:179
bool PQSCityColliders::OnSphereStart(bool sphereIsAlive) {
    if (!sphereIsAlive && transformsActive) {
        SetChildrenActive(false);
        transformsActive = false;
    }
    return false;
}

// PQSCity.cs:208
void PQSCityColliders::OnSphereReset() {
    if (transformsActive) {
        SetChildrenActive(false);
        transformsActive = false;
    }
}

// PQSCity.cs:220
void PQSCityColliders::OnSphereActive(bool hasTarget, const Vector3d& targetPosition) { OnUpdateFinished(hasTarget, targetPosition); }

// PQSCity.cs:225
void PQSCityColliders::OnSphereInactive() {
    size_t num = lod.size();
    while (num-- > 0) SetActive(lod[num], false);
}

// PQSCity.cs:234 (the load / POI range events are WP-42's)
void PQSCityColliders::OnUpdateFinished(bool hasTarget, const Vector3d& targetPosition) {
    if (!hasTarget) {
        size_t n = lod.size();
        while (n-- > 0) SetActive(lod[n], false);
        return;
    }
    if (!transformsActive) {
        SetChildrenActive(true);
        transformsActive = true;
    }
    const Vector3d d = targetPosition - set.position();
    const float num2 = static_cast<float>(dotnet::Math::Sqrt(d.x * d.x + d.y * d.y + d.z * d.z));  // Vector3.Distance (float)
    size_t num3 = lod.size();
    while (num3-- > 0) SetActive(lod[num3], num2 < lod[num3].visibleRange);
}

// ---- PQSCity2.cs -----------------------------------------------------------------------------------------------------------------

// PQSCity2.cs:35
void PQSCity2Colliders::SetRange(LodObject& o) { o.visibleRangeSqr = o.visibleRange * o.visibleRange; }

// PQSCity2.cs:24
void PQSCity2Colliders::Setup(LodObject& o) {
    SetRange(o);
    size_t num = o.objects.size();
    while (num-- > 0) set.SetObjectActive(o.objects[num], false);
    o.isActive = false;
}

// PQSCity2.cs:40
void PQSCity2Colliders::SetActive(LodObject& o, bool active) {
    if (o.KeepActive) active = true;
    if (active != o.isActive) {
        size_t num = o.objects.size();
        while (num-- > 0) set.SetObjectActive(o.objects[num], active);
    }
    o.isActive = active;
}

// PQSCity2.cs:367
void PQSCity2Colliders::SetInactive() {
    size_t num = objects.size();
    while (num-- > 0) SetActive(objects[num], false);
}

// PQSCity2.cs:407
void PQSCity2Colliders::OnSetup() {
    size_t num = objects.size();
    while (num-- > 0) Setup(objects[num]);
    SetInactive();
}

// PQSCity2.cs:425
bool PQSCity2Colliders::OnSphereStart(bool sphereIsAlive) {
    if (!sphereIsAlive) SetInactive();
    return false;
}

// PQSCity2.cs:443
void PQSCity2Colliders::OnSphereReset() { SetInactive(); }

// PQSCity2.cs:448
void PQSCity2Colliders::OnSphereActive(bool hasTarget, const Vector3d& targetPosition) { OnUpdateFinished(hasTarget, targetPosition); }

// PQSCity2.cs:453
void PQSCity2Colliders::OnSphereInactive() { SetInactive(); }

// PQSCity2.cs:458 (positioning / raycast / POI range work is WP-42's)
void PQSCity2Colliders::OnUpdateFinished(bool hasTarget, const Vector3d& targetPosition) {
    if (!hasTarget) {
        SetInactive();
        return;
    }
    setVisibility(targetPosition);
}

// PQSCity2.cs:491
void PQSCity2Colliders::setVisibility(const Vector3d& targetPosition) {
    const Vector3d d = targetPosition - set.position();
    const float num = static_cast<float>(d.x * d.x + d.y * d.y + d.z * d.z);  // Vector3.SqrMagnitude (float)
    size_t num2 = objects.size();
    while (num2-- > 0) {
        if (objects[num2].visibleRangeSqr == 0.0f) SetRange(objects[num2]);
        SetActive(objects[num2], num < objects[num2].visibleRangeSqr);
        inVisibleRange = inVisibleRange || num < objects[num2].visibleRangeSqr;
    }
}

}  // namespace ksp
