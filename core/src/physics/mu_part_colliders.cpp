// WP-mucol: real part colliders from .mu models. See ksp/physics/mu_part_colliders.hpp.
#include "ksp/physics/mu_part_colliders.hpp"

#include <algorithm>
#include <cmath>

#include "ksp/core/log.hpp"
#include "ksp/io/url_dir.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp::physics {

namespace {

constexpr double kPi = 3.14159265358979323846;

// Capsule solid (radius r, cylinder length L = 2 halfCyl along local Y), density 1: volume, inertia about the centre per unit mass.
std::shared_ptr<const ColliderProxy::MassProps> CapsuleMass(double r, double halfCyl) {
    auto m = std::make_shared<ColliderProxy::MassProps>();
    const double L = 2.0 * halfCyl;
    const double mc = kPi * r * r * L;           // cylinder
    const double mh = 2.0 / 3.0 * kPi * r * r * r;  // one hemisphere
    const double total = mc + 2.0 * mh;
    m->volume = total;
    const double iAxis = 0.5 * mc * r * r + 2.0 * (0.4 * mh * r * r);
    const double d = halfCyl + 3.0 * r / 8.0;     // hemisphere centroid from the capsule centre
    const double iHemCom = 0.4 * mh * r * r - mh * (3.0 * r / 8.0) * (3.0 * r / 8.0);
    const double iPerp = mc * (r * r / 4.0 + L * L / 12.0) + 2.0 * (iHemCom + mh * d * d);
    if (total > 0.0) {
        m->inertia[0][0] = iPerp / total;
        m->inertia[1][1] = iAxis / total;
        m->inertia[2][2] = iPerp / total;
    }
    return m;
}

}  // namespace

std::shared_ptr<const ColliderProxy::MassProps> HullMass(const std::vector<Vector3f>& vertices) {
    const HullMassProps h = HullMassProperties(vertices);
    if (!h.valid) return nullptr;
    auto m = std::make_shared<ColliderProxy::MassProps>();
    m->volume = h.volume;
    m->centroid = h.centroid;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) m->inertia[i][j] = h.inertia[i][j];
    return m;
}

std::vector<ColliderProxy> ProxiesFromShapes(const std::vector<mu::PartColliderShape>& shapes) {
    std::vector<ColliderProxy> out;
    for (const mu::PartColliderShape& s : shapes) {
        if (s.trigger || s.kind == mu::ColliderKind::Wheel) continue;
        ColliderProxy p;
        switch (s.kind) {
            case mu::ColliderKind::Box:
                p.kind = ShapeKind::Box;
                p.center = s.center;
                p.halfExtents = s.halfExtents;
                p.rotation = s.nodeRotation;
                break;
            case mu::ColliderKind::Sphere:
                p.kind = ShapeKind::Sphere;
                p.center = s.center;
                p.radius = s.radius;
                p.rotation = s.nodeRotation;
                break;
            case mu::ColliderKind::Capsule: {
                p.kind = ShapeKind::Capsule;
                p.center = s.center;
                p.radius = s.radius;
                p.halfHeight = s.halfCylinder;
                // Jolt's capsule runs along Y: direction 0 (X) is Rz(-90 deg), direction 2 (Z) is Rx(+90 deg)
                const float h = 0.70710678118654752f;
                Quaternionf fix = Quaternionf::identity();
                if (s.axis == 0) fix = Quaternionf(0.0f, 0.0f, -h, h);
                else if (s.axis == 2) fix = Quaternionf(h, 0.0f, 0.0f, h);
                p.rotation = (s.axis == 0 || s.axis == 2) ? UnityNative::Multiply(s.nodeRotation, fix) : s.nodeRotation;
                p.mass = CapsuleMass(p.radius, p.halfHeight);
                break;
            }
            case mu::ColliderKind::Mesh: {
                if (s.vertices.size() < 4) continue;
                p.kind = ShapeKind::ConvexHull;
                p.center = Vector3f();
                p.rotation = Quaternionf::identity();
                p.hull = std::make_shared<const std::vector<Vector3f>>(s.vertices);
                p.mass = HullMass(s.vertices);
                if (!p.mass) {
                    // Jolt cannot build a hull (coplanar points): the bounding box stands in for the inertia (labelled approximation)
                    Vector3f lo(1e30f, 1e30f, 1e30f), hi(-1e30f, -1e30f, -1e30f);
                    for (const Vector3f& v : s.vertices) {
                        lo = Vector3f(std::min(lo.x, v.x), std::min(lo.y, v.y), std::min(lo.z, v.z));
                        hi = Vector3f(std::max(hi.x, v.x), std::max(hi.y, v.y), std::max(hi.z, v.z));
                    }
                    auto m = std::make_shared<ColliderProxy::MassProps>();
                    const double hx = 0.5 * (hi.x - lo.x), hy = 0.5 * (hi.y - lo.y), hz = 0.5 * (hi.z - lo.z);
                    m->volume = 8.0 * hx * hy * hz;
                    m->centroid = Vector3d(0.5 * (hi.x + lo.x), 0.5 * (hi.y + lo.y), 0.5 * (hi.z + lo.z));
                    m->inertia[0][0] = (hy * hy + hz * hz) / 3.0;
                    m->inertia[1][1] = (hx * hx + hz * hz) / 3.0;
                    m->inertia[2][2] = (hx * hx + hy * hy) / 3.0;
                    p.mass = m;
                }
                break;
            }
            default:
                continue;
        }
        out.push_back(std::move(p));
    }
    return out;
}

MuPartColliders::MuPartColliders(ModelLoader loader) : loader_(std::move(loader)) {}

std::shared_ptr<const mu::Model> MuPartColliders::Load(const std::string& path) {
    auto it = cache_.find(path);
    if (it != cache_.end()) return it->second;
    std::shared_ptr<const mu::Model> m;
    try {
        if (loader_) {
            m = loader_(path);
        } else {
            m = std::make_shared<const mu::Model>(mu::ReadModelFile(path));
        }
        if (m) ++stats_.modelsRead;
    } catch (const std::exception& e) {
        ++stats_.modelErrors;
        log::Warning(std::string("MuPartColliders: ") + e.what());
    }
    cache_[path] = m;
    return m;
}

std::vector<mu::PartColliderShape> MuPartColliders::Shapes(const Part& part) {
    std::vector<mu::ModelInstance> inst;
    lastHadModel_ = false;
    for (const PartModelRef& mr : part.models) {
        if (!mr.file) continue;
        auto m = Load(mr.file->fullPath());
        if (!m) continue;
        mu::ModelInstance mi;
        mi.model = m.get();
        mi.fromModelNode = mr.fromModelNode;
        mi.parent = mr.parent;
        if (mr.name.has_value()) mi.name = *mr.name;
        mi.scale = mr.scale;
        mi.position = mr.position;
        mi.rotationEuler = mr.rotation;
        inst.push_back(std::move(mi));
        lastHadModel_ = true;
    }
    return mu::BuildPartColliders(part.fields.rescaleFactor, inst);
}

bool MuPartColliders::Proxies(const Part& part, std::vector<ColliderProxy>& out) {
    const std::vector<mu::PartColliderShape> shapes = Shapes(part);
    if (!lastHadModel_) {
        ++stats_.fixtureNoModel;
        return false;
    }
    out = ProxiesFromShapes(shapes);
    if (out.empty()) {
        ++stats_.fixtureNoShape;
        return false;
    }
    ++stats_.realParts;
    return true;
}

std::function<std::vector<ColliderProxy>(const Part&)> MuPartColliders::Hook() {
    return [this](const Part& part) {
        std::vector<ColliderProxy> out;
        if (Proxies(part, out)) return out;
        return DragCubeColliderProxy(part);  // FIXTURE: no real collider data for this part
    };
}

}  // namespace ksp::physics
