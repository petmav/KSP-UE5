// WP-mucol / WP-12c: KSC / launch-site / city scenery colliders as DATA and the loader that feeds them to CityColliderSet.
//
// Where the data lives in the game: the city GameObjects of the PQS prefab (PQSCity / PQSCity2 components, e.g. "Kerbin/KSC" with pathID
// 5898 in KSP_x64_Data/sharedassets9.assets, data/terrain/pqs_prefab.json) have a Transform subtree: the buildings, runway, launch pad
// and set dressing. Their colliders are NATIVE Unity components (BoxCollider / SphereCollider / CapsuleCollider / MeshCollider with a
// Mesh asset). tools/mucollider/extract_scenery_colliders.py (UnityPy, schema /2) writes core/data/scenery/scenery_colliders.json and
// scenery_meshes.bin; tools/wp12c/verify_scenery_extraction.py re-extracts them by a second route and compares every value (WP-12c).
//
// What the extraction replays so that the data is the tree the game runs with (Assembly-CSharp):
//   NestedPrefabSpawner.cs  : "Kerbin/KSC/SpaceCenter" is a spawner; the KSC buildings are a prefab Instantiated in its place
//                             (:20 Spawn, layers and tags of the new subtree defaulted by :60 setLayersAndTagsRecursive)
//   UpgradeableObject.cs:9  : every facility level (levelCost, facilityPrefab) is a prefab replacing the "Facility" transform; ALL levels
//                             are in the data, tagged with "levelGroup" (the facility id) and "level"; the host picks one (SceneryLevels)
//   DestructibleBuilding.cs : collapseObject / replacementObject of each CollapsibleObject (intact = collapse on, replacement off;
//                             destroyed = the opposite, :163 SetDestroyed)
//
// Schema "kspcore-scenery-colliders/2" (parsed by ParseSceneryColliders; "/1" is the older inline-mesh form the unit tests use):
//   { "schema": ..., "source": {...provenance...}, "meshFile": "scenery_meshes.bin",
//     "meshes": [ { "name", "origin": {file, pathID}, "vertexCount", "indexCount", "offset", "sha256" } ],     (blob: float32 xyz, uint32 idx)
//     "materials": [ { "name", "dynamicFriction", "staticFriction", "bounciness", "frictionCombine", "bounceCombine", "origin" } ],
//     "terrainMaterial": { "Kerbin": 0, "Mun": -1, ... },       (PQSMod_QuadMeshColliders.physicsMaterial per body; -1 = none)
//     "physicsManager": { "defaultMaterial": {...}, "m_BounceThreshold": ..., ... },    (Physics.defaultMaterial = "Metal" in stock)
//     "tags": [...], "launchSites": [...], "facilities": [...],
//     "cities": [ { "name": "Kerbin/KSC", "class": "PQSCity", "gameObjectPathID": n,
//         "objects": [ { "path": "KSC/SpaceCenter/LaunchPad/Facility/...", "parent": -1, "src": [file, pathID], "active": b, "layer": 15,
//                        "tag": "LaunchPad", "position": [x,y,z], "rotation": [x,y,z,w], "scale": [x,y,z],
//                        "crashObjectName"?, "mobileLaunchPad"?, "levelGroup"?, "level"?, "via"?,
//                        "colliders": [ { "type": "Box", "trigger", "enabled", "material": -1, "center", "size" },
//                                       { "type": "Sphere", ..., "radius" }, { "type": "Capsule", ..., "radius", "height", "direction" },
//                                       { "type": "Mesh", ..., "convex", "mesh": index } ] } ],
//         "children": [ object index ],  "lod": [ { "visibleRange", "keepActive", "objects": [ index ] } ],
//         "destructibles": [ { "object", "id", "collapsibles": [ { "collapse", "replacement", "collapseOffset", "collapseBehaviour" } ] } ] } ] }
//
// objects[0] is the city root; its own TRS is the city pose (CityColliderSet::SetPose) and is ignored here. Every other object's
// colliders are composed into the CITY frame with the same float arithmetic as part colliders (parts/part_colliders.hpp: lossyScale =
// component-wise product, Transform maths native float).  Mesh colliders become static triangle meshes (Jolt MeshShape), convex or not
// (Unity's non-convex mesh colliders collide statically; a convex flag is kept in the data).
//
// Activation (/2 documents): GameObject.SetActive(true) on an object makes its subtree active except the descendants with activeSelf
// false. `children` / `lod[].objects` are the GameObjects the PQSCity / PQSCity2 callbacks switch (PQSCity.cs:41, PQSCity2.cs:40); the
// Apply functions expand them to their subtrees (SceneryActiveSubtree) so that CityColliderSet's flat per-object switch matches the
// hierarchy; objects no callback switches (the root, the objects outside every LOD of a PQSCity2) are active from the start.
#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/physics/physics_world.hpp"
#include "ksp/space_center/city_colliders.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

struct SceneryMesh {
    std::vector<Vector3f> vertices;
    std::vector<int> triangles;
};

struct SceneryMaterial {
    std::string name;
    std::string originFile;
    long long originPathId = 0;
    float dynamicFriction = 0.6f, staticFriction = 0.6f, bounciness = 0.0f;
    int frictionCombine = 0, bounceCombine = 0;   // PhysicMaterialCombine: Average 0, Multiply 1, Minimum 2, Maximum 3
    physics::PhysicMaterialDesc Desc() const;
};

struct SceneryColliderSpec {
    enum class Type { Box, Sphere, Capsule, Mesh };
    Type type = Type::Box;
    bool trigger = false;
    bool enabled = true;       // Collider.enabled
    bool convex = false;
    Vector3f center;
    Vector3f size{1.0f, 1.0f, 1.0f};
    float radius = 0.5f;
    float height = 2.0f;
    int direction = 1;
    std::vector<Vector3f> vertices;                 // /1: inline mesh
    std::vector<int> triangles;
    std::shared_ptr<const SceneryMesh> mesh;        // /2: the shared mesh asset (preferred when set)
    int material = -1;                              // index into SceneryColliders::materials; -1: none (Physics.defaultMaterial)
    std::shared_ptr<const physics::PhysicMaterialDesc> materialDesc;   // resolved at parse
};

struct SceneryObjectSpec {
    std::string path;
    int parent = -1;
    Vector3f position;
    Quaternionf rotation = Quaternionf::identity();
    Vector3f scale{1.0f, 1.0f, 1.0f};
    std::vector<SceneryColliderSpec> colliders;
    // /2
    bool active = true;                 // GameObject.activeSelf
    int layer = 15;                     // GameObject.layer (15 = Local Scenery)
    std::string tag = "Untagged";       // GameObject.tag
    std::string crashObjectName;        // CrashObjectName.objectName on this object ("" none)
    bool mobileLaunchPad = false;       // PositionMobileLaunchPad on this object
    std::string levelGroup;             // root of an UpgradeLevel prefab: the facility id; descendants carry no group
    int level = -1;
    std::string name() const;           // the last path segment
};

struct SceneryLod {
    float visibleRange = 0.0f;
    bool keepActive = false;   // PQSCity2.LodObject.KeepActive (optional in the file)
    std::vector<size_t> objects;
};

struct SceneryCollapsible {
    size_t collapse = 0;
    int replacement = -1;
    Vector3f collapseOffset;
    int collapseBehaviour = 0;
};

struct SceneryDestructible {
    size_t object = 0;
    std::string id;
    std::vector<SceneryCollapsible> collapsibles;
};

// The PQSCity2 fields of a set dressing prefab that is not part of the PQS prefab (PSystemSetup.SpawnAdditionalPrefabs)
struct SceneryPlacement2 {
    std::string objectName;
    double lat = 0.0, lon = 0.0, alt = 0.0, rotation = 0.0, snapHeightOffset = 0.0;
    Vector3f up{0.0f, 1.0f, 0.0f};
    bool snapToSurface = false;
};

struct SceneryCity {
    std::string name;
    std::string className;               // "PQSCity" / "PQSCity2" ("" in /1 documents)
    std::string additionalPrefabOf;      // launch site whose PSystemSetup.additionalprefabPaths entry this is
    long long gameObjectPathId = 0;
    bool hierarchical = false;           // /2: activation follows the GameObject hierarchy
    std::vector<SceneryObjectSpec> objects;
    std::vector<size_t> children;
    std::vector<SceneryLod> lod;
    std::vector<SceneryDestructible> destructibles;
    std::optional<SceneryPlacement2> placement;
    int FindObject(std::string_view path) const;                    // -1 when absent; exact path match (first)
    std::vector<size_t> FindByTag(std::string_view tag) const;      // objects carrying that tag AND at least one collider
};

struct SceneryLaunchSite {
    std::string name, pqsName, launchSiteTransformURL, bundleName, prefabPath;
    std::vector<std::string> additionalPrefabPaths;
    std::vector<std::pair<std::string, std::string>> spawnPoints;   // (name, spawnTransformURL)
};

struct SceneryFacility {
    std::string name, pqsName, facilityTransformName, facilityName;
    int editorFacility = 0;  // PSystemSetup.SpaceCenterFacility.editorFacility (EditorFacility: 0 None, 1 VAB, 2 SPH)
    std::vector<std::pair<std::string, std::string>> spawnPoints;
};

struct SceneryColliders {
    std::string schema;
    std::vector<SceneryMaterial> materials;
    std::map<std::string, int> terrainMaterial;          // body name -> material index (-1: none)
    std::optional<SceneryMaterial> defaultMaterial;      // PhysicsManager.m_DefaultMaterial = Physics.defaultMaterial
    float bounceThreshold = 0.2f;                        // PhysicsManager.m_BounceThreshold = Physics.bounceThreshold
    float defaultContactOffset = 0.01f;
    std::vector<std::string> tags;
    std::vector<SceneryLaunchSite> launchSites;
    std::vector<SceneryFacility> facilities;
    std::vector<SceneryCity> cities;
    const SceneryCity* Find(std::string_view name) const;
    // the PhysicMaterial of a body's terrain colliders (PQSMod_QuadMeshColliders.physicsMaterial): the body's material, else the default
    // material (a collider without a material has Physics.defaultMaterial); nullopt when the document carries neither
    std::optional<SceneryMaterial> TerrainMaterial(const std::string& body) const;
    // PhysX's way of combining a contact pair's materials with the game's Physics.defaultMaterial / bounceThreshold / defaultContactOffset
    // (PhysicsManager): physxMaterials on, defaultMaterial = the document's, minVelocityForRestitution = Physics.bounceThreshold
    void ApplyTo(physics::PhysicsWorldConfig& config) const;
    // TerrainColliders options for a body: opt.material = TerrainMaterial(body) (null when the document has none)
    std::shared_ptr<const physics::PhysicMaterialDesc> TerrainMaterialDesc(const std::string& body) const;
};

// Throws std::runtime_error on a malformed document (wrong schema, bad index, parent after child, ...). `meshBlob` is the content of the
// document's meshFile (/2 documents with "meshes").
SceneryColliders ParseSceneryColliders(std::string_view json, std::string_view meshBlob = {});
// Reads the file and, for /2, the mesh file next to it.
SceneryColliders LoadSceneryColliders(const std::string& path);

// ---- levels, presence and activation ------------------------------------------------------------------------------------------------
// The UpgradeableObject level each facility is shown at (ScenarioUpgradeableFacilities.GetFacilityLevel x MaxLevel); a group not listed
// takes `fallback` (0 = the first level; -1 = the highest level of the group).
struct SceneryLevels {
    std::map<std::string, int> level;
    int fallback = 0;
};
// true for the objects of the scene: not a non-selected level of a facility, nor below one
std::vector<bool> SceneryPresence(const SceneryCity& city, const SceneryLevels& levels = {});
// GameObject.SetActive(true) on `root`: root and the descendants reachable through activeSelf == true objects that are present
std::vector<size_t> SceneryActiveSubtree(const SceneryCity& city, size_t root, const std::vector<bool>& present);
// Objects active-in-hierarchy from the start of the scene and not switched by any callback of `switched` roots
std::vector<size_t> SceneryAlwaysActive(const SceneryCity& city, const std::vector<bool>& present, const std::vector<size_t>& switchedRoots);

// The city-frame pose of every object (index = object index; object 0, the root, is the city frame): Unity's Transform hierarchy composed
// in float as BuildCityObjects places the colliders (position = parent.position + parent.rotation * (parent.lossyScale * localPosition),
// rotation = parent.rotation * localRotation, lossyScale componentwise; no shear).
struct SceneryObjectPose {
    Vector3f position{};
    Quaternionf rotation = Quaternionf::identity();
    Vector3f scale{1.0f, 1.0f, 1.0f};
};
std::vector<SceneryObjectPose> SceneryObjectPoses(const SceneryCity& city);
// WP-20g: Transform.Find by hierarchy path among the objects of the scene: the index of the present object whose path is `path`
// ("KSC/SpaceCenter/LaunchPad/Facility/LaunchPad_spawn"; the facility levels not shown are not present), -1 = none. The PSystemSetup
// spawn-point lookup (PSystemSetup.cs:40 facilityTransform.Find(spawnTransformURL)) of a flight host goes through it.
int SceneryFindObject(const SceneryCity& city, const std::string& path, const std::vector<bool>& present);

// The CityObjects of a city (index = object index; object 0, the root, has the colliders of the root GameObject itself).
// Trigger colliders and disabled colliders are dropped (a trigger has no contact response; a disabled collider none at all).
std::vector<CityObject> BuildCityObjects(const SceneryCity& city);

// Adds every object to `set` (indices coincide with the object indices) and returns nothing else: the PQSCity / PQSCity2 switching
// structures take the indices from `city.children` / `city.lod`.
void AddSceneryToSet(CityColliderSet& set, const SceneryCity& city);
// adds objects, fills children and lod. /2 documents: the lists are hierarchy-expanded (SceneryActiveSubtree) and exclude the objects of a
// non-selected facility level. The objects no callback ever switches are NOT activated here (the caller places the pose first, then
// activates SceneryInitiallyActive(city, levels) with CityColliderSet::SetObjectActive(base + i, true)).
void ApplySceneryLod(PQSCityColliders& colliders, const SceneryCity& city, const SceneryLevels& levels = {});
void ApplySceneryLod(PQSCity2Colliders& colliders, const SceneryCity& city, const SceneryLevels& levels = {});
// /2 documents: the objects (city object indices) that are active from the start and never switched: the root, and for a PQSCity2 every
// object outside its LOD subtrees. /1 documents: none.
std::vector<size_t> SceneryInitiallyActive(const SceneryCity& city, const SceneryLevels& levels = {});

}  // namespace ksp
