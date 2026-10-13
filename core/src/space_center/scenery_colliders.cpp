// WP-mucol: scenery collider data loader. See ksp/space_center/scenery_colliders.hpp.
#include "ksp/space_center/scenery_colliders.hpp"

#include <algorithm>
#include <cmath>
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

SceneryColliderSpec ParseCollider(const json& j) {
    SceneryColliderSpec c;
    const std::string t = j.at("type").get<std::string>();
    c.trigger = j.value("trigger", false);
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
    } else {
        throw std::runtime_error("scenery: unknown collider type '" + t + "'");
    }
    return c;
}

}  // namespace

const SceneryCity* SceneryColliders::Find(std::string_view name) const {
    for (const SceneryCity& c : cities)
        if (c.name == name) return &c;
    return nullptr;
}

SceneryColliders ParseSceneryColliders(std::string_view text) {
    const json doc = json::parse(text.begin(), text.end());
    SceneryColliders out;
    out.schema = doc.at("schema").get<std::string>();
    if (out.schema != "kspcore-scenery-colliders/1") throw std::runtime_error("scenery: unsupported schema '" + out.schema + "'");
    for (const json& jc : doc.at("cities")) {
        SceneryCity city;
        city.name = jc.at("name").get<std::string>();
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
            if (jo.contains("colliders"))
                for (const json& jcol : jo.at("colliders")) o.colliders.push_back(ParseCollider(jcol));
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
        out.cities.push_back(std::move(city));
    }
    return out;
}

SceneryColliders LoadSceneryColliders(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("scenery: cannot open " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ParseSceneryColliders(ss.str());
}

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
        const float ax = std::fabs(w.scl.x), ay = std::fabs(w.scl.y), az = std::fabs(w.scl.z);
        const float a[3] = {ax, ay, az};
        for (const SceneryColliderSpec& c : o.colliders) {
            if (c.trigger) continue;
            CityCollider cc;
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
                    cc.vertices.reserve(c.vertices.size());
                    for (const Vector3f& v : c.vertices) cc.vertices.push_back(place(v));
                    cc.triangles = c.triangles;
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

void ApplySceneryLod(PQSCityColliders& colliders, const SceneryCity& city) {
    const size_t base = colliders.set.ObjectCount();
    AddSceneryToSet(colliders.set, city);
    for (size_t c : city.children) colliders.children.push_back(base + c);
    for (const SceneryLod& l : city.lod) {
        PQSCityColliders::LODRange r;
        r.visibleRange = l.visibleRange;
        for (size_t o : l.objects) r.objects.push_back(base + o);
        colliders.lod.push_back(std::move(r));
    }
}

void ApplySceneryLod(PQSCity2Colliders& colliders, const SceneryCity& city) {
    const size_t base = colliders.set.ObjectCount();
    AddSceneryToSet(colliders.set, city);
    for (const SceneryLod& l : city.lod) {
        PQSCity2Colliders::LodObject o;
        o.visibleRange = l.visibleRange;
        o.KeepActive = l.keepActive;
        for (size_t i : l.objects) o.objects.push_back(base + i);
        colliders.objects.push_back(std::move(o));
    }
}

}  // namespace ksp
