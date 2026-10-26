// WP-29d: the transform hierarchy of a part's model (PartLoader.CompileModel's result below the part transform), with the legacy Animation
// components of the .mu chunks, for the engine-agnostic hosts (the wheel host; any module that looks transforms up by name).
//
// What KSP builds (Assembly-CSharp/PartLoader.cs:1765 CompileModel, PartReader.cs:205 ReadChild / :802 ReadAnimation):
//   part.partTransform
//     "model"  (localScale = rescaleFactor)  -- one MODEL node's .mu root (renamed "model" when the part has no MODEL node)
//        ...the .mu transforms...  each with the local position / rotation / scale of its chunk, and Animation components
// Part.FindModelTransform(name) (Part.cs:5079 / :5084 FindHeirarchyTransform): pre-order search of that tree starting at "model" itself.
// Part.FindModelTransforms(name) (:5147): every match in the same order.  Part.FindModelAnimator(transformName, clipName) (Part.cs):
// the Animation component of the transform with that name that holds the clip.
//
// The pose of a node in PART space (the part transform is the identity in the rig; the host composes the part's own pose) uses Unity's
// native Transform maths: one float32 rounding per operation, quaternions by UnityNative::Multiply, vectors by the Quaternion.cs
// operator* formula (RotateNative), parent scale applied to the child's local position (no shear).
//
// Animation: a legacy `Animation` component samples its enabled states onto the transforms the curves address ("path" relative to the
// owner, properties m_LocalPosition.x/y/z, m_LocalRotation.x/y/z/w, m_LocalScale.x/y/z). `SampleClip` writes the curve values of one clip at
// a time into the local TRS of the addressed nodes (a clip is the only enabled state with weight 1 for gear and legs); the
// rotation is renormalised after the four components are in. WP-29e measured the 13 stock clips (tools/wp29e/anim_norms.py, tests/test_wheel_visual.cpp):
// every STORED quaternion (46 rotation targets, 2,733 key quaternions) is unit to float32 precision, max |1 - norm| 8.4e-8, so at a key time the
// renormalisation is the identity; between keys the four components interpolated on their own are off the unit sphere by up to 9.8e-4, which the
// renormalisation removes (that Unity normalises there is its documented quaternion blend; only the legacy Animation's use of it is not captured).
// ClampForever on both ends (every shipped wheel clip): the curve clamps outside its key range (ksp::AnimationCurve::Evaluate).
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/parts/part_colliders.hpp"
#include "ksp/unity/animation_curve.hpp"

namespace ksp::mu {

struct RigNode {
    std::string name;
    int parent = -1;
    Vector3f lp;                                   // local position
    Quaternionf lr = Quaternionf::identity();      // local rotation
    Vector3f ls{1.0f, 1.0f, 1.0f};                 // local scale
    const Node* src = nullptr;                     // the .mu node (colliders, tag / layer, wheel chunk); null for the synthetic "model"
    std::vector<int> children;
};

// A pose in part space: position, rotation and the scale accumulated down the chain
struct RigPose {
    Vector3f position;
    Quaternionf rotation = Quaternionf::identity();
    Vector3f scale{1.0f, 1.0f, 1.0f};
};

// One Animation component: its owner node and clip table
struct RigAnimation {
    int owner = -1;
    const AnimationChunk* chunk = nullptr;
    struct Target {                 // one curve of a clip, resolved to a node and a component
        int node = -1;
        int component = -1;         // 0..2 position xyz, 3..6 rotation xyzw, 7..9 scale xyz
        AnimationCurve curve;
    };
    std::vector<std::vector<Target>> clipTargets;  // per clip of the chunk
    float Length(const std::string& clip) const;   // AnimationClip.length, 0 for an unknown clip
    int FindClip(const std::string& clip) const;   // index or -1
};

class PartModelRig {
public:
    PartModelRig() = default;
    // The hierarchy of a part: same inputs as BuildPartColliders (models stay owned by the caller and must outlive the rig)
    static PartModelRig Build(float rescaleFactor, const std::vector<ModelInstance>& instances);

    bool Valid() const { return !nodes_.empty(); }
    size_t NodeCount() const { return nodes_.size(); }
    const RigNode& At(int i) const { return nodes_[static_cast<size_t>(i)]; }
    // Part.FindModelTransform / FindModelTransforms: node indices, -1 / empty when absent
    int Find(const std::string& name) const;
    std::vector<int> FindAll(const std::string& name) const;
    // Transform.Find(path) from `from` ('/'-separated, direct children, first match); -1 when absent
    int FindPath(int from, const std::string& path) const;

    // the node's pose in part space, from the CURRENT local transforms
    RigPose PartSpace(int node) const;
    // overwrite one node's local transform (a module moving a transform, or the animation)
    void SetLocal(int node, const Vector3f& position, const Quaternionf& rotation);
    void SetLocalPosition(int node, const Vector3f& position);
    // Transform.position = p for a node, p in part space (WP-29e): the local position under the node's CURRENT parent that puts it there,
    // parent.InverseTransformPoint(p) = (rotate back by the parent's part-space rotation, divide by its accumulated scale). The root has no
    // parent: its local position is p.
    void SetPartSpacePosition(int node, const Vector3f& p);
    // Transform.IsChildOf: true for the node itself and for every node below `ancestor`
    bool IsChildOf(int node, int ancestor) const;
    // back to the prefab transforms
    void ResetLocal();

    // Animation components, in pre-order of their owners
    const std::vector<RigAnimation>& Animations() const { return animations_; }
    // Part.FindModelAnimator(transformName, clipName): index of the component on the (first) transform with that name holding the clip
    int FindAnimator(const std::string& transformName, const std::string& clipName) const;
    // Animation.Sample for one clip at `time` (seconds): writes the addressed local transforms
    void SampleClip(int animation, int clip, float time);

    // The collider shapes of the model in part space at the rig's CURRENT local transforms (pre-order, as BuildPartColliders: at the prefab
    // poses the two agree exactly; after SampleClip the animated colliders of a gear leg are at their sampled poses)
    std::vector<PartColliderShape> Colliders() const;
    const std::vector<RigNode>& Nodes() const { return nodes_; }

private:
    std::vector<RigNode> nodes_;
    std::vector<RigNode> prefab_;  // local transforms as built
    std::vector<RigAnimation> animations_;
    void CollectAnimations(int node);
    void ResolveTargets();
};

}  // namespace ksp::mu
