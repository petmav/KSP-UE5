// WP-40: EditorBounds.cs, EditorGeometryUtil.cs, EditorEnumExtensions.cs, EditorPartListFilter(.List).cs -- the editor's bounds, the part-bounds
// separation tests and the mirror maths, the facility enum conversions and the part-list filters.
#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/editor/editor_types.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/bounds.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// EditorBounds.cs:3 (the MonoBehaviour's data; the gizmo drawing is the engine's)
class EditorBounds {
public:
    static inline EditorBounds* Instance = nullptr;   // :6
    Bounds constructionBounds;                        // :8
    float cameraStartDistance = 30.0f;                // :10
    float cameraMaxDistance = 35.0f;                  // :12
    float cameraMinDistance = 3.0f;                   // :14
    Bounds cameraOffsetBounds;                        // :16
    Vector3f rootPartSpawnPoint{};                    // :18
    Vector3f sceneryCenter{};                         // sceneryCenter.position (:20)

    bool Awake();                                     // :31 (false: a second instance, the C# destroys it)
    void OnDestroy();                                 // :44
    static Vector3f ClampToCameraBounds(Vector3f pos, const Vector3f& camFwd, float& clampHeight);   // :60
    static float ClampCameraDistance(float dist);     // :80
    static void CenterSceneryOrigin(Vector3f& sceneryRootPosition);   // :89
    void SetExtents(const Vector3f& extents) { cameraOffsetBounds.set_extents(extents); }   // :102
};

// EditorGeometryUtil.cs:3
class EditorGeometryUtil {
public:
    static bool TestPartBoundsIntersect(Part& p1, Part& p2);                                    // :5
    static bool TestPartBoundsSeparate(Part& p1, Part& p2, float threshold, Vector3f& gap);     // :10
    static Vector3f GetPartBoundsSeparation(Part& p1, Part& p2);                                // :17
    static Quaternionf MirrorRotation(const Quaternionf& input, const PartTransform& mirrorRoot, const PartTransform& setRoot);   // :72
    static Vector3f MirrorPos(const Vector3f& input, const PartTransform& mirrorRoot, const PartTransform& setRoot);              // :84
    static Vector3f MirrorDirection(const Vector3f& input, const PartTransform& setRoot);                                          // :91
    static float GetPixelDistance(const Vector3f& sPos1, const Vector3f& sPos2);                // :104
    // Collider.ClosestPointOnBounds(point) of a box collider: the point of the bounds nearest to `point`
    static Vector3f ClosestPointOnBounds(const Bounds& b, const Vector3f& point);
};

// EditorEnumExtensions.cs:5
namespace EditorEnumExtensions {
SpaceCenterFacility ToFacility(EditorFacility editor);                 // :7
EditorFacility ToEditor(SpaceCenterFacility facility);                 // :17
SpaceCenterFacility GetEditorFacility(SpaceCenterFacility launchSite); // :32
// PSystemSetup.SpaceCenterFacility.name -> SpaceCenterFacility (:43); throws ArgumentNullException for an unknown name
SpaceCenterFacility GetFacility(const std::string& pSystemFacilityName);
std::string GetSymmetryMethodName(SymmetryMethod method);              // :81 (localised: #autoLOC_6004040 Mirror, #autoLOC_6004039 Radial)
}  // namespace EditorEnumExtensions

// EditorPartListFilter.cs:4
template <class T>
class EditorPartListFilter {
public:
    EditorPartListFilter(std::string filterID, std::function<bool(const T&)> criteria, std::string failMessage = "")   // :17
        : criteriaFailMessage(std::move(failMessage)), id_(std::move(filterID)), filterCriteria_(std::move(criteria)) {}
    const std::function<bool(const T&)>& FilterCriteria() const { return filterCriteria_; }   // :13
    const std::string& ID() const { return id_; }                                              // :15
    std::string criteriaFailMessage;                                                           // :11
    // :24 (typeName: typeof(T).Name)
    std::string ToString(const std::string& typeName) const { return "EditorPartListFilter(" + typeName + "): " + id_ + " '" + criteriaFailMessage + "'"; }
    // :29 FilterList: remove (from the back) every element the filter rejects
    static void FilterList(std::vector<T>& parts, const std::function<bool(const T&)>& filter) {
        int count = static_cast<int>(parts.size());
        while (count-- > 0)
            if (!filter(parts[static_cast<size_t>(count)])) parts.erase(parts.begin() + count);
    }

private:
    std::string id_;
    std::function<bool(const T&)> filterCriteria_;
};

// EditorPartListFilterList.cs:4
template <class T>
class EditorPartListFilterList {
public:
    using FilterPtr = std::shared_ptr<EditorPartListFilter<T>>;
    int Count() const { return static_cast<int>(filters_.size()); }   // :9
    FilterPtr operator[](int index) const { return filters_.at(static_cast<size_t>(index)); }   // :11
    FilterPtr operator[](const std::string& id) const {             // :18 (null when no filter has the id)
        for (const FilterPtr& f : filters_)
            if (f->ID() == id) return f;
        return nullptr;
    }
    void AddFilter(const FilterPtr& filter) {                       // :41
        if (std::find(filters_.begin(), filters_.end(), filter) == filters_.end()) filters_.push_back(filter);
    }
    void RemoveFilter(const FilterPtr& filter) {                    // :57
        auto it = std::find(filters_.begin(), filters_.end(), filter);
        if (it != filters_.end()) filters_.erase(it);
    }
    void RemoveFilter(const std::string& filterID) { RemoveFilter((*this)[filterID]); }   // :57
    std::vector<T> GetFilteredList(const std::vector<T>& list) const {   // :67
        std::vector<T> list2 = list;
        for (const FilterPtr& f : filters_) EditorPartListFilter<T>::FilterList(list2, f->FilterCriteria());
        return list2;
    }
    void FilterList(std::vector<T>& list) const {                   // :78
        for (const FilterPtr& f : filters_) EditorPartListFilter<T>::FilterList(list, f->FilterCriteria());
    }
    std::string GetFilterKey() const {                              // :98
        std::string text;
        for (const FilterPtr& f : filters_) text += f->ID();
        return text;
    }
    std::string GetFilterKeySingleOrNothing() const { return filters_.size() == 1 ? filters_[0]->ID() : std::string(); }   // :99
    std::string ToString(const std::string& typeName) const {       // :116
        std::string text = "EditorPartListFilterList(" + typeName + "): " + std::to_string(filters_.size());
        for (const FilterPtr& f : filters_) text += "\n  " + f->ToString(typeName);
        return text;
    }

private:
    std::vector<FilterPtr> filters_;
};

}  // namespace ksp
