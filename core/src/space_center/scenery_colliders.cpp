// WP-mucol / WP-12c: scenery collider data loader. See ksp/space_center/scenery_colliders.hpp.
#include "ksp/space_center/scenery_colliders.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "ksp/parts/part_colliders.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "nlohmann/json.hpp"

namespace ksp {

namespace {

using nlohmann::json;

Vector3f Vec3(const json& j) {
    if (!j.is_array() || j.size() != 3) throw std::runtime_error("scenery: expected [x,y,z]");
    return Vector3f(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
}

SceneryMaterial ParseMaterial(const json& j) {
    SceneryMaterial m;
    m.name = j.value("name", std::string());
    m.dynamicFriction = j.at("dynamicFriction").get<float>();
    m.staticFriction = j.at("staticFriction").get<float>();
    m.bounciness = j.at("bounciness").get<float>();
    m.frictionCombine = j.at("frictionCombine").get<int>();
    m.bounceCombine = j.at("bounceCombine").get<int>();
    if (j.contains("origin") && j.at("origin").is_object()) {
        m.originFile = j.at("origin").value("file", std::string());
        m.originPathId = j.at("origin").value("pathID", 0LL);
    }
    return m;
}

SceneryColliderSpec ParseCollider(const json& j, const std::vector<std::shared_ptr<const SceneryMesh>>& meshes,
                                  const std::vector<SceneryMaterial>& materials) {
    SceneryColliderSpec c;
    const std::string t = j.at("type").get<std::string>();
    c.trigger = j.value("trigger", false);
    c.enabled = j.value("enabled", true);
    c.material = j.value("material", -1);
    if (c.material >= 0) {
        if (static_cast<size_t>(c.material) >= materials.size()) throw std::runtime_error("scenery: material index out of range");
        c.materialDesc = std::make_shared<const physics::PhysicMaterialDesc>(materials[static_cast<size_t>(c.material)].Desc());
    }
    if (t == "Box") {
        c.type = SceneryColliderSpec::Type::Box;
        c.center = Vec3(j.at("center"));
        c.size = Vec3(j.at("size"));
    } else if (t == "Sphere") {
        c.type = SceneryColliderSpec::Type::Sphere;
        c.center = Vec3(j.at("center"));
        c.radius = j.at("radius").get<float>();
    } else if (t == "Capsule") {
        c.type = SceneryColliderSpec::Type::Capsule;
        c.center = Vec3(j.at("center"));
        c.radius = j.at("radius").get<float>();
        c.height = j.at("height").get<float>();
        c.direction = j.at("direction").get<int>();
    } else if (t == "Mesh") {
        c.type = SceneryColliderSpec::Type::Mesh;
        c.convex = j.value("convex", false);
        if (j.contains("mesh")) {
            const long mi = j.at("mesh").get<long>();
            if (mi < 0 || static_cast<size_t>(mi) >= meshes.size()) throw std::runtime_error("scenery: mesh index out of range (mesh file missing?)");
            c.mesh = meshes[static_cast<size_t>(mi)];
        } else {
            const json& v = j.at("vertices");
            if (!v.is_array() || v.size() % 3 != 0) throw std::runtime_error("scenery: vertices must be a flat [x,y,z,...] list");
            for (size_t i = 0; i < v.size(); i += 3)
                c.vertices.push_back(Vector3f(v[i].get<float>(), v[i + 1].get<float>(), v[i + 2].get<float>()));
            const json& tr = j.at("triangles");
            if (!tr.is_array() || tr.size() % 3 != 0) throw std::runtime_error("scenery: triangles must be a flat index list (multiple of 3)");
            for (const json& x : tr) {
                const int idx = x.get<int>();
                if (idx < 0 || static_cast<size_t>(idx) >= c.vertices.size()) throw std::runtime_error("scenery: triangle index out of range");
                c.triangles.push_back(idx);
            }
        }
    } else {
        throw std::runtime_error("scenery: unknown collider type '" + t + "'");
    }
    return c;
}

std::vector<std::pair<std::string, std::string>> ParseSpawnPoints(const json& j) {
    std::vector<std::pair<std::string, std::string>> out;
    if (j.contains("spawnPoints"))
        for (const json& s : j.at("spawnPoints")) out.emplace_back(s.at("name").get<std::string>(), s.at("spawnTransformURL").get<std::string>());
    return out;
}

}  // namespace

physics::PhysicMaterialDesc SceneryMaterial::Desc() const {
    physics::PhysicMaterialDesc d;
    d.dynamicFriction = dynamicFriction;
    d.staticFriction = staticFriction;
    d.bounciness = bounciness;
    d.frictionCombine = frictionCombine;
    d.bounceCombine = bounceCombine;
    return d;
}

std::string SceneryObjectSpec::name() const {
    const size_t p = path.rfind('/');
    return p == std::string::npos ? path : path.substr(p + 1);
}

int SceneryCity::FindObject(std::string_view p) const {
    for (size_t i = 0; i < objects.size(); ++i)
        if (objects[i].path == p) return static_cast<int>(i);
    return -1;
}

std::vector<size_t> SceneryCity::FindByTag(std::string_view tag) const {
    std::vector<size_t> out;
    for (size_t i = 0; i < objects.size(); ++i)
        if (objects[i].tag == tag && !objects[i].colliders.empty()) out.push_back(i);
    return out;
}

const SceneryCity* SceneryColliders::Find(std::string_view name) const {
    for (const SceneryCity& c : cities)
        if (c.name == name) return &c;
    return nullptr;
}

std::optional<SceneryMaterial> SceneryColliders::TerrainMaterial(const std::string& body) const {
    auto it = terrainMaterial.find(body);
    if (it != terrainMaterial.end() && it->second >= 0 && static_cast<size_t>(it->second) < materials.size())
        return materials[static_cast<size_t>(it->second)];
    return defaultMaterial;
}

std::shared_ptr<const physics::PhysicMaterialDesc> SceneryColliders::TerrainMaterialDesc(const std::string& body) const {
    const std::optional<SceneryMaterial> m = TerrainMaterial(body);
    return m ? std::make_shared<const physics::PhysicMaterialDesc>(m->Desc()) : nullptr;
}

void SceneryColliders::ApplyTo(physics::PhysicsWorldConfig& config) const {
    if (!defaultMaterial) return;
    config.physxMaterials = true;
    config.defaultMaterial = defaultMaterial->Desc();
    config.minVelocityForRestitution = bounceThreshold;
}

SceneryColliders ParseSceneryColliders(std::string_view text, std::string_view meshBlob) {
    const json doc = json::parse(text.begin(), text.end());
    SceneryColliders out;
    out.schema = doc.at("schema").get<std::string>();
    const bool v2 = out.schema == "kspcore-scenery-colliders/2";
    if (!v2 && out.schema != "kspcore-scenery-colliders/1") throw std::runtime_error("scenery: unsupported schema '" + out.schema + "'");

    if (doc.contains("materials"))
        for (const json& jm : doc.at("materials")) out.materials.push_back(ParseMaterial(jm));
    if (doc.contains("terrainMaterial"))
        for (auto it = doc.at("terrainMaterial").begin(); it != doc.at("terrainMaterial").end(); ++it) out.terrainMaterial[it.key()] = it.value().get<int>();
    if (doc.contains("physicsManager")) {
        const json& pm = doc.at("physicsManager");
        if (pm.contains("defaultMaterial") && pm.at("defaultMaterial").is_object()) out.defaultMaterial = ParseMaterial(pm.at("defaultMaterial"));
        out.bounceThreshold = pm.value("m_BounceThreshold", out.bounceThreshold);
        out.defaultContactOffset = pm.value("m_DefaultContactOffset", out.defaultContactOffset);
    }
    if (doc.contains("tags")) out.tags = doc.at("tags").get<std::vector<std::string>>();
    if (doc.contains("launchSites"))
        for (const json& jl : doc.at("launchSites")) {
            SceneryLaunchSite l;
            l.name = jl.at("name").get<std::string>();
            l.pqsName = jl.value("pqsName", std::string());
            l.launchSiteTransformURL = jl.value("launchSiteTransformURL", std::string());
            l.bundleName = jl.value("BundleName", std::string());
            l.prefabPath = jl.value("prefabPath", std::string());
            if (jl.contains("additionalprefabPaths")) l.additionalPrefabPaths = jl.at("additionalprefabPaths").get<std::vector<std::string>>();
            l.spawnPoints = ParseSpawnPoints(jl);
            out.launchSites.push_back(std::move(l));
        }
    if (doc.contains("facilities"))
        for (const json& jf : doc.at("facilities")) {
            SceneryFacility f;
            f.name = jf.at("name").get<std::string>();
            f.pqsName = jf.value("pqsName", std::string());
            f.facilityTransformName = jf.value("facilityTransformName", std::string());
            f.facilityName = jf.value("facilityName", std::string());
            f.spawnPoints = ParseSpawnPoints(jf);
            out.facilities.push_back(std::move(f));
        }

    // the shared mesh assets
    std::vector<std::shared_ptr<const SceneryMesh>> meshes;
    if (doc.contains("meshes")) {
        for (const json& jm : doc.at("meshes")) {
            const size_t nv = jm.at("vertexCount").get<size_t>(), ni = jm.at("indexCount").get<size_t>(), off = jm.at("offset").get<size_t>();
            if (ni % 3 != 0) throw std::runtime_error("scenery: mesh index count is not a multiple of 3");
            if (off + nv * 12 + ni * 4 > meshBlob.size()) {
                meshes.push_back(nullptr);   // the mesh file is missing: a collider that uses this mesh is refused below
                continue;
            }
            auto mesh = std::make_shared<SceneryMesh>();
            mesh->vertices.resize(nv);
            for (size_t i = 0; i < nv; ++i) {
                float f[3];
                std::memcpy(f, meshBlob.data() + off + i * 12, 12);
                mesh->vertices[i] = Vector3f(f[0], f[1], f[2]);
            }
            mesh->triangles.resize(ni);
            for (size_t i = 0; i < ni; ++i) {
                uint32_t idx;
                std::memcpy(&idx, meshBlob.data() + off + nv * 12 + i * 4, 4);
                if (idx >= nv) throw std::runtime_error("scenery: mesh triangle index out of range");
                mesh->triangles[i] = static_cast<int>(idx);
            }
            meshes.push_back(std::move(mesh));
        }
    }

    for (const json& jc : doc.at("cities")) {
        SceneryCity city;
        city.name = jc.at("name").get<std::string>();
        city.className = jc.value("class", std::string());
        city.additionalPrefabOf = jc.value("additionalPrefabOf", std::string());
        city.gameObjectPathId = jc.value("gameObjectPathID", 0LL);
        city.hierarchical = v2;
        if (jc.contains("placement")) {
            const json& p = jc.at("placement");
            SceneryPlacement2 pl;
            pl.objectName = p.value("objectName", std::string());
            pl.lat = p.value("lat", 0.0);
            pl.lon = p.value("lon", 0.0);
            pl.alt = p.value("alt", 0.0);
            pl.rotation = p.value("rotation", 0.0);
            pl.snapHeightOffset = p.value("snapHeightOffset", 0.0);
            pl.snapToSurface = p.value("snapToSurface", 0) != 0;
            if (p.contains("up")) pl.up = Vector3f(p.at("up").at("x").get<float>(), p.at("up").at("y").get<float>(), p.at("up").at("z").get<float>());
            city.placement = pl;
        }
        for (const json& jo : jc.at("objects")) {
            SceneryObjectSpec o;
            o.path = jo.value("path", std::string());
            o.parent = jo.value("parent", -1);
            if (o.parent >= static_cast<int>(city.objects.size()))
                throw std::runtime_error("scenery: object '" + o.path + "' has a parent that does not precede it");
            o.position = Vec3(jo.at("position"));
            const json& r = jo.at("rotation");
            if (!r.is_array() || r.size() != 4) throw std::runtime_error("scenery: rotation must be [x,y,z,w]");
            o.rotation = Quaternionf(r[0].get<float>(), r[1].get<float>(), r[2].get<float>(), r[3].get<float>());
            o.scale = Vec3(jo.at("scale"));
            o.active = jo.value("active", true);
            o.layer = jo.value("layer", 15);
            o.tag = jo.value("tag", std::string("Untagged"));
            o.crashObjectName = jo.value("crashObjectName", std::string());
            o.mobileLaunchPad = jo.value("mobileLaunchPad", false);
            o.levelGroup = jo.value("levelGroup", std::string());
            o.level = jo.value("level", -1);
            if (jo.contains("colliders"))
                for (const json& jcol : jo.at("colliders")) {
                    SceneryColliderSpec c = ParseCollider(jcol, meshes, out.materials);
                    if (c.type == SceneryColliderSpec::Type::Mesh && !c.mesh && c.vertices.empty()) continue;   // mesh file not loaded
                    o.colliders.push_back(std::move(c));
                }
            city.objects.push_back(std::move(o));
        }
        const size_t n = city.objects.size();
        auto idx = [&](const json& x) {
            const long v = x.get<long>();
            if (v < 0 || static_cast<size_t>(v) >= n) throw std::runtime_error("scenery: object index out of range");
            return static_cast<size_t>(v);
        };
        if (jc.contains("children"))
            for (const json& x : jc.at("children")) city.children.push_back(idx(x));
        if (jc.contains("lod"))
            for (const json& jl : jc.at("lod")) {
                SceneryLod l;
                l.visibleRange = jl.at("visibleRange").get<float>();
                l.keepActive = jl.value("keepActive", false);
                for (const json& x : jl.at("objects")) l.objects.push_back(idx(x));
                city.lod.push_back(std::move(l));
            }
        if (jc.contains("destructibles"))
            for (const json& jd : jc.at("destructibles")) {
                SceneryDestructible d;
                d.object = idx(jd.at("object"));
                d.id = jd.value("id", std::string());
                for (const json& jcl : jd.at("collapsibles")) {
                    SceneryCollapsible c;
                    c.collapse = idx(jcl.at("collapse"));
                    c.replacement = jcl.at("replacement").get<int>();
                    if (c.replacement >= 0) idx(jcl.at("replacement"));
                    if (jcl.contains("collapseOffset")) c.collapseOffset = Vec3(jcl.at("collapseOffset"));
                    c.collapseBehaviour = jcl.value("collapseBehaviour", 0);
                    d.collapsibles.push_back(c);
                }
                city.destructibles.push_back(std::move(d));
            }
        out.cities.push_back(std::move(city));
    }
    return out;
}

SceneryColliders LoadSceneryColliders(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("scenery: cannot open " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    const std::string text = ss.str();
    std::string blob;
    const json head = json::parse(text.begin(), text.end());
    if (head.contains("meshFile")) {
        const size_t slash = path.find_last_of("/\\");
        const std::string dir = slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
        std::ifstream mf(dir + head.at("meshFile").get<std::string>(), std::ios::binary);
        if (!mf) throw std::runtime_error("scenery: cannot open the mesh file " + dir + head.at("meshFile").get<std::string>());
        std::stringstream ms;
        ms << mf.rdbuf();
        blob = ms.str();
    }
    return ParseSceneryColliders(text, blob);
}

// ---- levels, presence, activation (UpgradeableObject.cs:9 / NestedPrefabSpawner.cs / GameObject.SetActive) ----------------------------

std::vector<bool> SceneryPresence(const SceneryCity& city, const SceneryLevels& levels) {
    // highest level per group
    std::map<std::string, int> maxLevel;
    for (const SceneryObjectSpec& o : city.objects)
        if (!o.levelGroup.empty()) maxLevel[o.levelGroup] = std::max(maxLevel[o.levelGroup], o.level);
    std::vector<bool> present(city.objects.size(), true);
    for (size_t i = 0; i < city.objects.size(); ++i) {
        const SceneryObjectSpec& o = city.objects[i];
        bool p = o.parent < 0 ? true : present[static_cast<size_t>(o.parent)];
        if (p && !o.levelGroup.empty()) {
            auto it = levels.level.find(o.levelGroup);
            int want = it != levels.level.end() ? it->second : levels.fallback;
            if (want < 0 || want > maxLevel[o.levelGroup]) want = maxLevel[o.levelGroup];
            p = o.level == want;
        }
        present[i] = p;
    }
    return present;
}

std::vector<size_t> SceneryActiveSubtree(const SceneryCity& city, size_t root, const std::vector<bool>& present) {
    std::vector<size_t> out;
    std::vector<bool> in(city.objects.size(), false);
    if (root >= city.objects.size() || !present[root]) return out;
    in[root] = true;
    out.push_back(root);
    for (size_t i = root + 1; i < city.objects.size(); ++i) {   // parents precede children
        const int par = city.objects[i].parent;
        if (par >= 0 && in[static_cast<size_t>(par)] && city.objects[i].active && present[i]) {
            in[i] = true;
            out.push_back(i);
        }
    }
    return out;
}

std::vector<size_t> SceneryAlwaysActive(const SceneryCity& city, const std::vector<bool>& present, const std::vector<size_t>& switchedRoots) {
    std::vector<bool> switched(city.objects.size(), false);
    for (size_t r : switchedRoots)
        for (size_t i : SceneryActiveSubtree(city, r, present)) switched[i] = true;
    // active in hierarchy from the city root (its activeSelf is not consulted: the PQS keeps the city GameObject on)
    std::vector<size_t> out;
    std::vector<bool> act(city.objects.size(), false);
    for (size_t i = 0; i < city.objects.size(); ++i) {
        const SceneryObjectSpec& o = city.objects[i];
        const bool parentActive = o.parent < 0 ? true : act[static_cast<size_t>(o.parent)];
        act[i] = present[i] && parentActive && (i == 0 || o.active);
        if (act[i] && !switched[i]) out.push_back(i);
    }
    return out;
}

// ---- CityObjects --------------------------------------------------------------------------------------------------------------------

std::vector<CityObject> BuildCityObjects(const SceneryCity& city) {
    struct W {
        Vector3f pos;
        Quaternionf rot = Quaternionf::identity();
        Vector3f scl{1.0f, 1.0f, 1.0f};
    };
    std::vector<W> world(city.objects.size());
    std::vector<CityObject> out(city.objects.size());
    for (size_t i = 0; i < city.objects.size(); ++i) {
        const SceneryObjectSpec& o = city.objects[i];
        W w;  // the root (object 0) is the city frame itself
        if (i != 0 && o.parent >= 0) {
            const W& p = world[static_cast<size_t>(o.parent)];
            const Vector3f scaled(p.scl.x * o.position.x, p.scl.y * o.position.y, p.scl.z * o.position.z);
            const Vector3f r = mu::RotateNative(p.rot, scaled);
            w.pos = Vector3f(p.pos.x + r.x, p.pos.y + r.y, p.pos.z + r.z);
            w.rot = UnityNative::Multiply(p.rot, o.rotation);
            w.scl = Vector3f(p.scl.x * o.scale.x, p.scl.y * o.scale.y, p.scl.z * o.scale.z);
        } else if (i != 0) {  // a parentless object hangs from the city root
            w.pos = o.position;
            w.rot = o.rotation;
            w.scl = o.scale;
        }
        world[i] = w;
        auto place = [&](const Vector3f& v) {
            const Vector3f r = mu::RotateNative(w.rot, Vector3f(w.scl.x * v.x, w.scl.y * v.y, w.scl.z * v.z));
            return Vector3f(w.pos.x + r.x, w.pos.y + r.y, w.pos.z + r.z);
        };
        // GameObject data of the contact records: CrashObjectName is found upwards (Part.GetComponentUpwards), PositionMobileLaunchPad too
        out[i].layer = o.layer;
        out[i].tag = o.tag;
        out[i].name = o.name();
        for (int k = static_cast<int>(i); k >= 0; k = city.objects[static_cast<size_t>(k)].parent) {
            const SceneryObjectSpec& a = city.objects[static_cast<size_t>(k)];
            if (out[i].crashObjectName.empty() && !a.crashObjectName.empty()) out[i].crashObjectName = a.crashObjectName;
            if (a.mobileLaunchPad) out[i].mobileLaunchPad = true;
        }
        const float ax = std::fabs(w.scl.x), ay = std::fabs(w.scl.y), az = std::fabs(w.scl.z);
        const float a[3] = {ax, ay, az};
        for (const SceneryColliderSpec& c : o.colliders) {
            if (c.trigger || !c.enabled) continue;
            CityCollider cc;
            cc.material = c.materialDesc;
            switch (c.type) {
                case SceneryColliderSpec::Type::Box: {
                    cc.kind = CityCollider::Kind::Box;
                    const Vector3f sz(w.scl.x * c.size.x, w.scl.y * c.size.y, w.scl.z * c.size.z);
                    cc.shape = physics::ShapeDesc::Box(Vector3f(std::fabs(sz.x) * 0.5f, std::fabs(sz.y) * 0.5f, std::fabs(sz.z) * 0.5f));
                    cc.localPosition = Vector3d(place(c.center));
                    cc.localRotation = QuaternionD(w.rot);
                    break;
                }
                case SceneryColliderSpec::Type::Sphere: {
                    cc.kind = CityCollider::Kind::Sphere;
                    cc.shape = physics::ShapeDesc::Sphere(c.radius * std::max(ax, std::max(ay, az)));
                    cc.localPosition = Vector3d(place(c.center));
                    cc.localRotation = QuaternionD(w.rot);
                    break;
                }
                case SceneryColliderSpec::Type::Capsule: {
                    cc.kind = CityCollider::Kind::Capsule;
                    const int d = (c.direction < 0 || c.direction > 2) ? 1 : c.direction;
                    float other = 0.0f;
                    for (int k = 0; k < 3; ++k)
                        if (k != d) other = std::max(other, a[k]);
                    const float r = c.radius * other;
                    const float ht = c.height * a[d];
                    cc.shape = physics::ShapeDesc::Capsule(std::max(0.0f, ht * 0.5f - r), r);
                    cc.localPosition = Vector3d(place(c.center));
                    const float h = 0.70710678118654752f;  // Jolt capsules run along Y: X is Rz(-90), Z is Rx(+90)
                    Quaternionf rot = w.rot;
                    if (d == 0) rot = UnityNative::Multiply(w.rot, Quaternionf(0.0f, 0.0f, -h, h));
                    else if (d == 2) rot = UnityNative::Multiply(w.rot, Quaternionf(h, 0.0f, 0.0f, h));
                    cc.localRotation = QuaternionD(rot);
                    break;
                }
                case SceneryColliderSpec::Type::Mesh: {
                    cc.kind = CityCollider::Kind::Mesh;
                    cc.localPosition = Vector3d();
                    cc.localRotation = QuaternionD::identity();
                    const std::vector<Vector3f>& vs = c.mesh ? c.mesh->vertices : c.vertices;
                    cc.vertices.reserve(vs.size());
                    for (const Vector3f& v : vs) cc.vertices.push_back(place(v));
                    cc.triangles = c.mesh ? c.mesh->triangles : c.triangles;
                    break;
                }
            }
            out[i].colliders.push_back(std::move(cc));
        }
    }
    return out;
}

void AddSceneryToSet(CityColliderSet& set, const SceneryCity& city) {
    for (CityObject& o : BuildCityObjects(city)) set.Add(std::move(o));
}

namespace {
std::vector<size_t> Expand(const SceneryCity& city, const std::vector<size_t>& roots, const std::vector<bool>& present) {
    if (!city.hierarchical) return roots;   // /1: the flat lists as given
    std::vector<size_t> out;
    std::vector<bool> seen(city.objects.size(), false);
    for (size_t r : roots)
        for (size_t i : SceneryActiveSubtree(city, r, present))
            if (!seen[i]) {
                seen[i] = true;
                out.push_back(i);
            }
    std::sort(out.begin(), out.end());
    return out;
}
}  // namespace

void ApplySceneryLod(PQSCityColliders& colliders, const SceneryCity& city, const SceneryLevels& levels) {
    const size_t base = colliders.set.ObjectCount();
    AddSceneryToSet(colliders.set, city);
    const std::vector<bool> present = SceneryPresence(city, levels);
    for (size_t c : Expand(city, city.children, present)) colliders.children.push_back(base + c);
    for (const SceneryLod& l : city.lod) {
        PQSCityColliders::LODRange r;
        r.visibleRange = l.visibleRange;
        for (size_t o : Expand(city, l.objects, present)) r.objects.push_back(base + o);
        colliders.lod.push_back(std::move(r));
    }
}

void ApplySceneryLod(PQSCity2Colliders& colliders, const SceneryCity& city, const SceneryLevels& levels) {
    const size_t base = colliders.set.ObjectCount();
    AddSceneryToSet(colliders.set, city);
    const std::vector<bool> present = SceneryPresence(city, levels);
    for (const SceneryLod& l : city.lod) {
        PQSCity2Colliders::LodObject o;
        o.visibleRange = l.visibleRange;
        o.KeepActive = l.keepActive;
        for (size_t i : Expand(city, l.objects, present)) o.objects.push_back(base + i);
        colliders.objects.push_back(std::move(o));
    }
}

std::vector<size_t> SceneryInitiallyActive(const SceneryCity& city, const SceneryLevels& levels) {
    if (!city.hierarchical) return {};
    std::vector<size_t> switched;
    if (city.className != "PQSCity2") switched = city.children;   // PQSCity.transformsActive switches the direct children
    for (const SceneryLod& l : city.lod) switched.insert(switched.end(), l.objects.begin(), l.objects.end());
    return SceneryAlwaysActive(city, SceneryPresence(city, levels), switched);
}

}  // namespace ksp
