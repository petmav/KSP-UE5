// WP-00: HierarchyUtil.cs, TransformExtension.cs and GameObjectExtension.cs over the Transform shim (ksp/unity/transform.hpp, where
// the shim stands for Transform and its GameObject at once: name, layer, tag, activeSelf).
//
// Ported: every pure function (depth / ancestor comparisons, CompileID, NestToParent, FindChild / FindParent recursion,
// SetLayerRecursive, SetTagsRecursive, GameObject.GetChild).
// Seams (engine-only, not ported): TransformExtension.SetShader (Shader.Find + MeshRenderer.material), the translucent-shader test
// of SetLayerRecursive(filterTranslucent) (passed in as `isTranslucent`: Renderer.materials[].shader.name.Contains("Translucent")),
// GameObjectExtension.GetComponentCached / AddOrGetComponent / GetComponentOnParent / GetComponentUpwards / GetComponentIndex
// (the component registry lives in the engine shell) and DestroyGameObject / DestroyGameObjectImmediate (Object.Destroy).
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ksp/unity/transform.hpp"

namespace ksp {

class Part;

namespace HierarchyUtil {

using UnityEngine::Transform;

int GetHierarchyDepth(const Part* p);        // HierarchyUtil.cs:6
int GetHierarchyDepth(const Transform* t);   // HierarchyUtil.cs:22

// HierarchyUtil.cs:38 GetHierarchyDepth<T>(T o, Func<T, T> parentAccessor)
template <class T>
int GetHierarchyDepth(T o, const std::function<T(T)>& parentAccessor) {
    if (parentAccessor(o) == nullptr) return 0;
    T val = parentAccessor(o);
    int num = 0;
    while (val != nullptr) {
        num++;
        val = parentAccessor(val);
    }
    return num;
}

int CompareHierarchyDepths(int d1, int d2);                         // HierarchyUtil.cs:54 (Int32.CompareTo: -1 / 0 / 1)
int CompareHierarchyDepths(const Part* p1, const Part* p2);         // HierarchyUtil.cs:59
int CompareHierarchyDepths(const Transform* t1, const Transform* t2);  // HierarchyUtil.cs:64

// HierarchyUtil.cs:69 CompareHierarchyDepths<T>
template <class T>
int CompareHierarchyDepths(T t1, T t2, const std::function<T(T)>& parentAccessor) {
    return CompareHierarchyDepths(GetHierarchyDepth<T>(t1, parentAccessor), GetHierarchyDepth<T>(t2, parentAccessor));
}

bool isAncestor(const Part* p1, const Part* p2);        // HierarchyUtil.cs:74
bool isSameLevel(const Part* p1, const Part* p2);       // HierarchyUtil.cs:79
bool isDescendant(const Part* p1, const Part* p2);      // HierarchyUtil.cs:84
bool isAncestor(const Transform* t1, const Transform* t2);    // HierarchyUtil.cs:89
bool isSameLevel(const Transform* t1, const Transform* t2);   // HierarchyUtil.cs:94
bool isDescendant(const Transform* t1, const Transform* t2);  // HierarchyUtil.cs:99

// HierarchyUtil.cs:104 / 109 / 114 (generic accessor forms)
template <class T>
bool isAncestor(T t1, T t2, const std::function<T(T)>& a) { return CompareHierarchyDepths<T>(t1, t2, a) < 0; }
template <class T>
bool isSameLevel(T t1, T t2, const std::function<T(T)>& a) { return CompareHierarchyDepths<T>(t1, t2, a) == 0; }
template <class T>
bool isDescendant(T t1, T t2, const std::function<T(T)>& a) { return CompareHierarchyDepths<T>(t1, t2, a) > 0; }

std::string CompileID(const Transform* trf, const std::string& rootName);  // HierarchyUtil.cs:119

}  // namespace HierarchyUtil

namespace TransformExtension {

using UnityEngine::Transform;

void NestToParent(Transform* t, Transform* newParent);                       // TransformExtension.cs:5
void NestToParent(Transform* t, Transform* newParent, bool resetParent);     // TransformExtension.cs:18
Transform* FindChild(Transform* t, const std::string& childName);            // TransformExtension.cs:28
Transform* FindChild(Transform* t, const std::string& childName, bool findActiveChild);  // TransformExtension.cs:33
Transform* FindParent(Transform* t, const std::string& parentName, bool findActiveParent);  // TransformExtension.cs:70
void SetLayerRecursive(Transform* root, int layer);                          // TransformExtension.cs:99

}  // namespace TransformExtension

namespace GameObjectExtension {

using UnityEngine::Transform;
using TranslucentTest = std::function<bool(const Transform&)>;

Transform* GetChild(Transform* obj, const std::string& childName);           // GameObjectExtension.cs:82
void SetLayerRecursive(Transform* obj, int layer, int ignoreLayersMask = 0);  // GameObjectExtension.cs:120
// GameObjectExtension.cs:125 (isTranslucent replaces the renderer / material scan, evaluated only when filterTranslucent is set)
void SetLayerRecursive(Transform* obj, int layer, bool filterTranslucent, int ignoreLayersMask = 0, const TranslucentTest& isTranslucent = nullptr);
void SetTagsRecursive(Transform* obj, const std::string& tag, const std::vector<std::string>& ignoreTags = {});  // GameObjectExtension.cs:161

}  // namespace GameObjectExtension

}  // namespace ksp
