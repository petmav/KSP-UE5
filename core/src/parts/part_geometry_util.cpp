// WP-13: PartGeometryUtil (PartGeometryUtil.cs) over stored renderer / collider bounds. See the header for what is deferred.
#include "ksp/parts/part_geometry_util.hpp"

#include <algorithm>

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/modules/module_part_variants.hpp"
#include "ksp/parts/part.hpp"

namespace ksp {

// Part.cs:5353 / 5378 / 5403
std::vector<PartModelRenderer> Part::FindModelMeshRenderersCached() const {
    std::vector<PartModelRenderer> r;
    for (const auto& m : modelRenderers)
        if (!m.skinned) r.push_back(m);
    return r;
}
std::vector<PartModelRenderer> Part::FindModelSkinnedMeshRenderersCached() const {
    std::vector<PartModelRenderer> r;
    for (const auto& m : modelRenderers)
        if (m.skinned) r.push_back(m);
    return r;
}
std::vector<PartModelRenderer> Part::FindModelRenderersCached() const { return modelRenderers; }

namespace PartGeometryUtil {

// PartGeometryUtil.cs:23
Bounds GetRendererBounds(const std::vector<PartModelRenderer>& renderers) {
    Bounds result;
    bool flag = false;
    for (const PartModelRenderer& r : renderers) {
        if (flag) {
            result.Encapsulate(r.bounds);
            continue;
        }
        result = r.bounds;
        flag = true;
    }
    return result;
}

// PartGeometryUtil.cs:54
Bounds GetRendererBoundsWithoutParticles(const std::vector<PartModelRenderer>& renderers) {
    Bounds result;
    bool flag = false;
    for (const PartModelRenderer& r : renderers) {
        if (r.particleSystem) continue;
        if (flag) {
            result.Encapsulate(r.bounds);
            continue;
        }
        result = r.bounds;
        flag = true;
    }
    return result;
}

// PartGeometryUtil.cs:85
Bounds GetColliderBounds(const std::vector<Bounds>& colliders) {
    Bounds result;
    bool flag = false;
    for (const Bounds& b : colliders) {
        if (flag) {
            result.Encapsulate(b);
            continue;
        }
        result = b;
        flag = true;
    }
    return result;
}

// PartGeometryUtil.cs:107
Bounds GetPartRendererBound(Part& p) {
    const std::vector<PartModelRenderer> list = p.FindModelRenderersCached();  // FindModelComponents<Renderer>()
    return GetRendererBounds(list);
}

// PartGeometryUtil.cs:129 / 134
std::vector<Bounds> GetRendererBounds(Part& p) { return GetPartRendererBounds(p); }
std::vector<Bounds> GetColliderBounds(Part& p) { return GetPartColliderBounds(p); }

// PartGeometryUtil.cs:139
std::vector<Bounds> GetPartRendererBounds(Part& p) {
    std::vector<std::string> disabledVariantGOs;
    if (p.variants != nullptr) {
        PartVariant* selectedVariant = p.variants->SelectedVariant();
        for (const PartGameObjectInfo& g : selectedVariant->infoGameObjects)
            if (!g.Status()) disabledVariantGOs.push_back(g.Name());
    }
    std::vector<PartModelRenderer> list = p.FindModelMeshRenderersCached();
    std::vector<PartModelRenderer> list2 = p.FindModelSkinnedMeshRenderersCached();
    auto filter = [&](std::vector<PartModelRenderer>& l) {
        int count = static_cast<int>(l.size());
        while (count-- > 0) {
            const std::string& name = l[static_cast<size_t>(count)].name;
            const auto& ignore = p.fields.partRendererBoundsIgnore;
            if (ignore && std::find(ignore->begin(), ignore->end(), name) != ignore->end()) l.erase(l.begin() + count);
            else if (std::find(disabledVariantGOs.begin(), disabledVariantGOs.end(), name) != disabledVariantGOs.end()) l.erase(l.begin() + count);
        }
    };
    filter(list);
    filter(list2);
    std::vector<Bounds> array;
    for (const auto& r : list) array.push_back(r.bounds);
    for (const auto& r : list2) array.push_back(r.bounds);
    return array;
}

// PartGeometryUtil.cs:190
std::vector<Bounds> GetPartColliderBounds(Part& p) { return p.colliderBounds; }

// PartGeometryUtil.cs:200
Bounds MergeBounds(const std::vector<Bounds>& bounds, const Vector3f& relativeToPosition) {
    Bounds result;
    result.set_center(relativeToPosition);
    int num = static_cast<int>(bounds.size());
    while (num-- > 0) {
        result.Encapsulate(bounds[static_cast<size_t>(num)].max());
        result.Encapsulate(bounds[static_cast<size_t>(num)].min());
    }
    return result;
}

// PartGeometryUtil.cs:214
Bounds MergeBounds(const std::vector<Bounds>& bounds) {
    Bounds result;
    if (!bounds.empty()) {
        result.set_center(bounds[0].center());
        int num = static_cast<int>(bounds.size());
        while (num-- > 0) {
            result.Encapsulate(bounds[static_cast<size_t>(num)].max());
            result.Encapsulate(bounds[static_cast<size_t>(num)].min());
        }
        return result;
    }
    return result;
}

// PartGeometryUtil.cs:234
Bounds MergeBounds(const std::vector<std::pair<std::vector<Bounds>, PosRot>>& centeredBounds, const PosRot& relativeTo) {
    Bounds result;
    result.set_center(relativeTo.vector);
    for (const auto& [key, value] : centeredBounds) {
        const int num = static_cast<int>(key.size());
        for (int i = 0; i < num; i++) {
            result.Encapsulate(relativeTo.quaternion * value.vector + value.quaternion * key[static_cast<size_t>(i)].max());
            result.Encapsulate(relativeTo.quaternion * value.vector + value.quaternion * key[static_cast<size_t>(i)].min());
        }
    }
    return result;
}

// PartGeometryUtil.cs:245
Vector3f FindBoundsCentroid(const std::vector<Bounds>& bounds, const PartTransform* localTo) {
    Vector3f zero = Vector3f::zero();
    float num = 0.0f;
    int num2 = static_cast<int>(bounds.size());
    while (num2-- > 0) {
        const Bounds bounds2 = bounds[static_cast<size_t>(num2)];
        const float magnitude = bounds2.size().magnitude();
        if (localTo != nullptr) zero = zero + localTo->InverseTransformPoint(bounds2.center()) * magnitude;
        else zero = zero + bounds2.center() * magnitude;
        num = f32store(fx(num) + fx(magnitude));
    }
    return zero / num;
}

// PartLoader.cs:937
float PartSize(Part& p) { return MergeBounds(GetRendererBounds(p), p.partTransform.position).size().magnitude(); }

}  // namespace PartGeometryUtil
}  // namespace ksp
