// WP-00: HierarchyUtil.cs, TransformExtension.cs, GameObjectExtension.cs (see ksp/util/hierarchy_util.hpp for scope and seams).
#include "ksp/util/hierarchy_util.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/parts/part.hpp"

namespace ksp {

namespace HierarchyUtil {

// HierarchyUtil.cs:6 GetHierarchyDepth
int GetHierarchyDepth(const Part* p) {
    if (p->parent == nullptr) return 0;
    const Part* parent = p->parent;
    int num = 0;
    while (parent != nullptr) {
        num++;
        parent = parent->parent;
    }
    return num;
}

// HierarchyUtil.cs:22 GetHierarchyDepth
int GetHierarchyDepth(const Transform* t) {
    if (t->parent() == nullptr) return 0;
    const Transform* parent = t->parent();
    int num = 0;
    while (parent != nullptr) {
        num++;
        parent = parent->parent();
    }
    return num;
}

// HierarchyUtil.cs:54 CompareHierarchyDepths
int CompareHierarchyDepths(int d1, int d2) { return d1 < d2 ? -1 : (d1 > d2 ? 1 : 0); }

// HierarchyUtil.cs:59 CompareHierarchyDepths
int CompareHierarchyDepths(const Part* p1, const Part* p2) { return CompareHierarchyDepths(GetHierarchyDepth(p1), GetHierarchyDepth(p2)); }

// HierarchyUtil.cs:64 CompareHierarchyDepths
int CompareHierarchyDepths(const Transform* t1, const Transform* t2) { return CompareHierarchyDepths(GetHierarchyDepth(t1), GetHierarchyDepth(t2)); }

// HierarchyUtil.cs:74 isAncestor
bool isAncestor(const Part* p1, const Part* p2) { return CompareHierarchyDepths(p1, p2) < 0; }

// HierarchyUtil.cs:79 isSameLevel
bool isSameLevel(const Part* p1, const Part* p2) { return CompareHierarchyDepths(p1, p2) == 0; }

// HierarchyUtil.cs:84 isDescendant
bool isDescendant(const Part* p1, const Part* p2) { return CompareHierarchyDepths(p1, p2) > 0; }

// HierarchyUtil.cs:89 isAncestor
bool isAncestor(const Transform* t1, const Transform* t2) { return CompareHierarchyDepths(t1, t2) < 0; }

// HierarchyUtil.cs:94 isSameLevel
bool isSameLevel(const Transform* t1, const Transform* t2) { return CompareHierarchyDepths(t1, t2) == 0; }

// HierarchyUtil.cs:99 isDescendant
bool isDescendant(const Transform* t1, const Transform* t2) { return CompareHierarchyDepths(t1, t2) > 0; }

namespace {
// HierarchyUtil.cs:125 compileID
std::string compileID(const Transform* trf, std::string tId, const std::string& rootName) {
    if (trf->name != rootName) {
        if (trf->parent() != nullptr) {
            tId = tId + compileID(trf->parent(), tId, rootName) + "/";
        } else if (!rootName.empty()) {
            log::Error("[HierarchyUtil]: CompileID failed because it reached the scene root without finding a 'SpaceCenter' game object");
        }
    }
    tId += trf->name;
    return tId;
}
}  // namespace

// HierarchyUtil.cs:119 CompileID (string.Replace(" ", ""))
std::string CompileID(const Transform* trf, const std::string& rootName) {
    std::string s = compileID(trf, "", rootName);
    std::string out;
    out.reserve(s.size());
    for (char c : s)
        if (c != ' ') out.push_back(c);
    return out;
}

}  // namespace HierarchyUtil

namespace TransformExtension {

namespace {
// TransformExtension.cs:38 FindChildRecursive
Transform* FindChildRecursive(Transform* parent, const std::string& childName, bool findActiveChild) {
    if (findActiveChild) {
        if (parent != nullptr && parent->activeInHierarchy() && parent->name == childName) return parent;
    } else {
        if (parent == nullptr) throw NullReferenceException();
        if (parent->name == childName) return parent;
    }
    for (int num = 0; num < parent->childCount(); num++) {
        Transform* t = FindChildRecursive(parent->GetChild(num), childName, findActiveChild);
        if (t != nullptr) return t;
    }
    return nullptr;
}

// TransformExtension.cs:75 FindParentRecursive
Transform* FindParentRecursive(Transform* parent, const std::string& parentName, bool findActiveParent) {
    if (findActiveParent) {
        if (parent != nullptr && parent->activeInHierarchy() && parent->name == parentName) return parent;
    } else {
        if (parent == nullptr) throw NullReferenceException();
        if (parent->name == parentName) return parent;
    }
    if (parent->parent() != nullptr) {
        Transform* t = FindParentRecursive(parent->parent(), parentName, findActiveParent);
        if (t != nullptr) return t;
    }
    return nullptr;
}
}  // namespace

// TransformExtension.cs:5 NestToParent
void NestToParent(Transform* t, Transform* newParent) {
    if (newParent != t) {
        t->SetParent(newParent);
        if (newParent != nullptr) {
            t->localPosition = Vector3f::zero();
            t->localRotation = Quaternionf::identity();
        }
    }
}

// TransformExtension.cs:18 NestToParent
void NestToParent(Transform* t, Transform* newParent, bool resetParent) {
    if (resetParent) {
        t->SetParent(nullptr);
        t->localScale = Vector3f::one();
    }
    NestToParent(t, newParent);
}

// TransformExtension.cs:28 FindChild
Transform* FindChild(Transform* t, const std::string& childName) { return FindChildRecursive(t, childName, false); }

// TransformExtension.cs:33 FindChild
Transform* FindChild(Transform* t, const std::string& childName, bool findActiveChild) { return FindChildRecursive(t, childName, findActiveChild); }

// TransformExtension.cs:70 FindParent
Transform* FindParent(Transform* t, const std::string& parentName, bool findActiveParent) { return FindParentRecursive(t, parentName, findActiveParent); }

// TransformExtension.cs:99 SetLayerRecursive
void SetLayerRecursive(Transform* root, int layer) {
    root->layer = layer;
    for (int i = 0; i < root->childCount(); i++) SetLayerRecursive(root->GetChild(i), layer);
}

}  // namespace TransformExtension

namespace GameObjectExtension {

// GameObjectExtension.cs:82 GetChild: direct children first, then each child's subtree
Transform* GetChild(Transform* obj, const std::string& childName) {
    for (int num = 0; num < obj->childCount(); num++) {
        if (obj->GetChild(num)->name == childName) return obj->GetChild(num);
    }
    for (int num2 = 0; num2 < obj->childCount(); num2++) {
        Transform* child2 = GetChild(obj->GetChild(num2), childName);
        if (child2 != nullptr) return child2;
    }
    return nullptr;
}

// GameObjectExtension.cs:120 SetLayerRecursive
void SetLayerRecursive(Transform* obj, int layer, int ignoreLayersMask) { SetLayerRecursive(obj, layer, false, ignoreLayersMask, nullptr); }

// GameObjectExtension.cs:125 SetLayerRecursive
void SetLayerRecursive(Transform* obj, int layer, bool filterTranslucent, int ignoreLayersMask, const TranslucentTest& isTranslucent) {
    if (((1 << obj->layer) & ignoreLayersMask) == 0) {
        bool flag = false;
        if (filterTranslucent && isTranslucent) flag = isTranslucent(*obj);
        if (!flag) obj->layer = layer;
    }
    for (int k = 0; k < obj->childCount(); k++) SetLayerRecursive(obj->GetChild(k), layer, filterTranslucent, ignoreLayersMask, isTranslucent);
}

// GameObjectExtension.cs:161 SetTagsRecursive
void SetTagsRecursive(Transform* obj, const std::string& tag, const std::vector<std::string>& ignoreTags) {
    bool flag = true;
    for (size_t num = ignoreTags.size(); num-- > 0;) {
        if (ignoreTags[num] == obj->tag) {
            flag = false;
            break;
        }
    }
    if (flag) obj->tag = tag;
    for (int i = 0; i < obj->childCount(); i++) SetTagsRecursive(obj->GetChild(i), tag, ignoreTags);
}

}  // namespace GameObjectExtension

}  // namespace ksp
