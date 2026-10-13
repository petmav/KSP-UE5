// WP-mucol: KSC / city scenery colliders as DATA (the seam WP-12b left open) and the loader that feeds them to CityColliderSet.
//
// Where the data lives in the game: the city GameObjects of the PQS prefab (PQSCity / PQSCity2 components, e.g. "Kerbin/KSC" with pathID
// 20638 in KSP_x64_Data/sharedassets9.assets, data/terrain/pqs_prefab.json) have a Transform subtree: the buildings, runway, launch pad
// and set dressing. Their colliders are NATIVE Unity components (BoxCollider / SphereCollider / CapsuleCollider / MeshCollider with a
// Mesh asset), so UnityPy reads them without the managed type trees. tools/mucollider/extract_scenery_colliders.py writes them to
// core/data/scenery/scenery_colliders.json in the format below (documented here, parsed by ParseSceneryColliders):
//
//   { "schema": "kspcore-scenery-colliders/1",
//     "source": { "dataDir": "...", "files": { "<asset file>": "<sha256>" } },      (provenance, free-form)
//     "cities": [ { "name": "Kerbin/KSC",                                          (PQSCity gameObject path)
//         "objects": [ { "path": "KSC/Runway", "parent": -1,                       (index into this array; parents precede children)
//                        "position": [x,y,z], "rotation": [x,y,z,w], "scale": [x,y,z],   (Transform local TRS, Unity left-handed axes)
//                        "colliders": [ { "type": "Box", "trigger": false, "center": [x,y,z], "size": [x,y,z] },
//                                       { "type": "Sphere", "trigger": false, "center": [x,y,z], "radius": r },
//                                       { "type": "Capsule", "trigger": false, "center": [x,y,z], "radius": r, "height": h, "direction": 0|1|2 },
//                                       { "type": "Mesh", "trigger": false, "convex": b, "vertices": [x,y,z,...], "triangles": [i,...] } ] } ],
//         "children": [ object index, ... ],                                       (direct children of the city transform: PQSCity.transformsActive)
//         "lod": [ { "visibleRange": f, "objects": [ object index, ... ] } ] } ] }  (PQSCity.lod / PQSCity2.objects, in serialized order)
//
// objects[0] is the city root; its own TRS is the city pose (CityColliderSet::SetPose) and is ignored here. Every other object's
// colliders are composed into the CITY frame with the same float arithmetic as part colliders (parts/part_colliders.hpp: lossyScale =
// component-wise product, Transform maths native float).  Mesh colliders become static triangle meshes (Jolt MeshShape), convex or not.
//
// STATUS: the loader is exercised by a synthetic-format test only. The extraction itself needs the game's asset files, which the session
// that wrote this could not read (STATUS.md "WP-mucol", next step 1).
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ksp/space_center/city_colliders.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

struct SceneryColliderSpec {
    enum class Type { Box, Sphere, Capsule, Mesh };
    Type type = Type::Box;
    bool trigger = false;
    bool convex = false;
    Vector3f center;
    Vector3f size{1.0f, 1.0f, 1.0f};
    float radius = 0.5f;
    float height = 2.0f;
    int direction = 1;
    std::vector<Vector3f> vertices;
    std::vector<int> triangles;
};

struct SceneryObjectSpec {
    std::string path;
    int parent = -1;
    Vector3f position;
    Quaternionf rotation = Quaternionf::identity();
    Vector3f scale{1.0f, 1.0f, 1.0f};
    std::vector<SceneryColliderSpec> colliders;
};

struct SceneryLod {
    float visibleRange = 0.0f;
    bool keepActive = false;   // PQSCity2.LodObject.KeepActive (optional in the file)
    std::vector<size_t> objects;
};

struct SceneryCity {
    std::string name;
    std::vector<SceneryObjectSpec> objects;
    std::vector<size_t> children;
    std::vector<SceneryLod> lod;
};

struct SceneryColliders {
    std::string schema;
    std::vector<SceneryCity> cities;
    const SceneryCity* Find(std::string_view name) const;
};

// Throws std::runtime_error on a malformed document (wrong schema, bad index, parent after child, ...).
SceneryColliders ParseSceneryColliders(std::string_view json);
SceneryColliders LoadSceneryColliders(const std::string& path);

// The CityObjects of a city (index = object index; object 0, the root, has the colliders of the root GameObject itself).
// Trigger colliders are dropped (a trigger has no contact response).
std::vector<CityObject> BuildCityObjects(const SceneryCity& city);

// Adds every object to `set` (indices coincide with the object indices) and returns nothing else: the PQSCity / PQSCity2 switching
// structures take the indices from `city.children` / `city.lod`.
void AddSceneryToSet(CityColliderSet& set, const SceneryCity& city);
void ApplySceneryLod(PQSCityColliders& colliders, const SceneryCity& city);   // adds objects, fills children and lod
void ApplySceneryLod(PQSCity2Colliders& colliders, const SceneryCity& city);  // adds objects and LodObjects

}  // namespace ksp
