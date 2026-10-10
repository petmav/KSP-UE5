// UnityEngine.Transform / GameObject as far as engine-free KSP code needs them: the GameObject name, a world position and
// rotation (float). The real transforms live in the engine shell; this is the stand-in for objects the core creates itself
// (PositionTarget's `new GameObject()`, FlightCoMTracker's marker) and for the transform an ITargetable hands out
// (Vessel.GetTransform mirrors its ReferenceTransform here). Completes the opaque forward declaration in
// ksp/events/game_event_types.hpp.
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::UnityEngine {

class Transform {
public:
    std::string name;                                   // Object.name (the GameObject's name)
    Vector3f position;                                  // Transform.position (world space)
    Quaternionf rotation = Quaternionf::identity();     // Transform.rotation (world space)

    // WP-00: the hierarchy half (parent / children) and the GameObject fields HierarchyUtil, TransformExtension and
    // GameObjectExtension read. The local* fields are plain data: the shim does not derive them from / into the world
    // position (that is the engine's transform hierarchy, a seam); SetParent therefore keeps every transform field as is.
    Vector3f localPosition = Vector3f::zero();
    Quaternionf localRotation = Quaternionf::identity();
    Vector3f localScale = Vector3f::one();
    int layer = 0;                                      // GameObject.layer
    std::string tag = "Untagged";                       // GameObject.tag
    bool activeSelf = true;                             // GameObject.activeSelf

    Transform* parent() const { return parent_; }       // Transform.parent
    int childCount() const { return static_cast<int>(children_.size()); }  // Transform.childCount
    Transform* GetChild(int index) const { return children_.at(static_cast<size_t>(index)); }  // Transform.GetChild
    // Transform.SetParent(parent) (worldPositionStays = true): re-links the hierarchy only.
    void SetParent(Transform* newParent) {
        if (parent_ == newParent) return;
        if (parent_ != nullptr) parent_->children_.erase(std::remove(parent_->children_.begin(), parent_->children_.end(), this), parent_->children_.end());
        parent_ = newParent;
        if (parent_ != nullptr) parent_->children_.push_back(this);
    }
    // GameObject.activeInHierarchy: this object and every ancestor is active.
    bool activeInHierarchy() const {
        for (const Transform* t = this; t != nullptr; t = t->parent_)
            if (!t->activeSelf) return false;
        return true;
    }

private:
    Transform* parent_ = nullptr;
    std::vector<Transform*> children_;
};

}  // namespace ksp::UnityEngine
