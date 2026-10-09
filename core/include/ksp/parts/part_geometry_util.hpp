// WP-13: PartGeometryUtil (PartGeometryUtil.cs): bounds maths over the part model. The renderer / collider bounds themselves
// are mesh data (Unity computes them from the .mu geometry and the part transform): the model reader (UE import) fills
// Part::modelRenderers / Part::colliderBounds, and everything here works from those stored bounds.
//   ported      MergeBounds (3 overloads), FindBoundsCentroid, GetPartRendererBounds (variant / ignore-list filtering),
//               GetPartColliderBounds, GetRendererBounds / GetColliderBounds / GetRendererBoundsWithoutParticles over bounds
//               lists, GetPartRendererBound, and PartLoader's `partSize` formula (PartLoader.cs:937)
//   deferred    computing the bounds from a mesh (Renderer.bounds, Collider.bounds): needs the model
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "ksp/unity/bounds.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
struct PartTransform;
struct PartModelRenderer;

namespace PartGeometryUtil {

// PartGeometryUtil.cs:9 PosRot
struct PosRot {
    Vector3f vector{};
    Quaternionf quaternion = Quaternionf::identity();
    PosRot(const Vector3f& v, const Quaternionf& q) : vector(v), quaternion(q) {}
};

// PartGeometryUtil.cs:23 GetRendererBounds(GameObject): the first renderer's bounds, the others encapsulated
Bounds GetRendererBounds(const std::vector<PartModelRenderer>& renderers);
// :54 GetRendererBoundsWithoutParticles (ParticleSystemRenderer entries are skipped)
Bounds GetRendererBoundsWithoutParticles(const std::vector<PartModelRenderer>& renderers);
// :85 GetColliderBounds(GameObject)
Bounds GetColliderBounds(const std::vector<Bounds>& colliders);
// :107 GetPartRendererBound(Part)
Bounds GetPartRendererBound(Part& p);
// :129 GetRendererBounds(Part) -> GetPartRendererBounds
std::vector<Bounds> GetRendererBounds(Part& p);
// :134 GetColliderBounds(Part) -> GetPartColliderBounds
std::vector<Bounds> GetColliderBounds(Part& p);
// :139 GetPartRendererBounds(Part): renderers named in partRendererBoundsIgnore or switched off by the selected variant are
// dropped (from the part's cached renderer lists, as in C#); mesh renderers first, then skinned ones
std::vector<Bounds> GetPartRendererBounds(Part& p);
// :190 GetPartColliderBounds(Part)
std::vector<Bounds> GetPartColliderBounds(Part& p);
// :200 MergeBounds(Bounds[], Transform relativeTo): centered on the transform's position
Bounds MergeBounds(const std::vector<Bounds>& bounds, const Vector3f& relativeToPosition);
// :214 MergeBounds(Bounds[]) (internal): centered on the first bounds
Bounds MergeBounds(const std::vector<Bounds>& bounds);
// :234 MergeBounds(Dictionary<Bounds[], PosRot>, PosRot) (dictionary in insertion order)
Bounds MergeBounds(const std::vector<std::pair<std::vector<Bounds>, PosRot>>& centeredBounds, const PosRot& relativeTo);
// :245 FindBoundsCentroid(Bounds[], Transform localTo); localTo == nullptr: world-space centres
Vector3f FindBoundsCentroid(const std::vector<Bounds>& bounds, const PartTransform* localTo);

// PartLoader.cs:937: availablePart.partSize = MergeBounds(partPrefab.GetRendererBounds(), transform).size.magnitude
float PartSize(Part& p);

}  // namespace PartGeometryUtil
}  // namespace ksp
