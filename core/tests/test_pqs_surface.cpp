// WP-12d: the PQS close-out ports (PQS_Collider / PQS_ColliderAdvanced / PQS_PartCollider / PQSMod_CreateSphereCollider, ...).
//
// Gate of the collider family: tests/data/wp12d_colliders_reference.json from tools/wp12d/surface_colliders_reference.py (an
// independent Python port of the three C# files over the WP-12a numerics of tools/wp12/pqs_ref_math.py; terrain heights = WP-11's
// PQS.GetSurfaceHeight of the stock sphere through the WP-12a height oracle). Every box pose, flag and owner state of every step and the
// IgnoreCollision call log are compared bit for bit.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/physics/physics_world.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/terrain/pqs_surface_colliders.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using nlohmann::json;

namespace {

const std::string kData = KSPCORE_DATA_DIR;
const std::string kTestData = KSPCORE_TEST_DATA_DIR;

terrain::MapLibrary& Maps() {
    static terrain::MapLibrary maps(kData + "/terrain");
    return maps;
}

float F(const json& h) {
    const uint32_t u = static_cast<uint32_t>(std::stoul(h.get<std::string>(), nullptr, 16));
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}
double D(const json& h) {
    const uint64_t u = std::stoull(h.get<std::string>(), nullptr, 16);
    double d;
    std::memcpy(&d, &u, 8);
    return d;
}
std::string HF(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    char b[16];
    std::snprintf(b, sizeof b, "%08x", u);
    return b;
}
Vector3f FV(const json& a) { return Vector3f(F(a[0]), F(a[1]), F(a[2])); }
json JV(const Vector3f& v) { return json::array({HF(v.x), HF(v.y), HF(v.z)}); }
json JQ(const Quaternionf& q) { return json::array({HF(q.x), HF(q.y), HF(q.z), HF(q.w)}); }

const char* ParentName(PQSBoxCollider::Parent p) {
    switch (p) {
        case PQSBoxCollider::Parent::Sphere: return "Sphere";
        case PQSBoxCollider::Parent::Owner: return "Owner";
        default: return "Root";
    }
}

struct Body {
    Vector3f pos, vel;
};

// one object of a scenario: the collider component and its owner state
struct Obj {
    std::string name, kind;
    Body body;
    std::unique_ptr<PQS_Collider> col;
    std::unique_ptr<PQS_ColliderAdvanced> adv;
    std::unique_ptr<PQS_PartCollider> part;
    std::vector<int> ownIds;
};

void Wire(SurfaceColliderOwner& o, Obj& obj) {
    o.name = obj.name;
    o.position = [&obj] { return obj.body.pos; };
    o.setPosition = [&obj](const Vector3f& v) { obj.body.pos = v; };
    o.velocity = [&obj] { return obj.body.vel; };
    o.setVelocity = [&obj](const Vector3f& v) { obj.body.vel = v; };
}

json Box(const PQSBoxCollider& b) {
    return {{"name", b.name}, {"parent", ParentName(b.parent)}, {"active", b.activeSelf}, {"scale", JV(b.localScale)},
            {"localPosition", JV(b.localPosition)}, {"position", JV(b.position)}, {"rotation", JQ(b.rotation)}};
}

}  // namespace

TEST_CASE("WP-12d PQS_Collider / PQS_ColliderAdvanced / PQS_PartCollider vs tools/wp12d/surface_colliders_reference.py (bit for bit)") {
    const json ref = json::parse(std::ifstream(kTestData + "/wp12d_colliders_reference.json"));
    REQUIRE(ref.at("schema") == "kspcore-wp12d-colliders/1");
    size_t rows = 0, mismatches = 0, boxes = 0, calls = 0;
    std::map<std::string, std::unique_ptr<PQS>> spheres;
    for (const json& sc : ref.at("scenarios")) {
        const std::string body = sc.at("body");
        if (!spheres.count(body)) spheres[body] = terrain::LoadStockPQS(body, Maps());
        PQS* sphere = spheres[body].get();
        PQS_ColliderAdvanced::ClearStatics();
        PQS_PartCollider::ClearStatics();
        PhysicsIgnoreCollisions().Clear();
        std::map<int, std::string> idName;
        std::vector<std::unique_ptr<Obj>> objs;
        for (const json& oj : sc.at("objects")) {
            auto o = std::make_unique<Obj>();
            o->name = oj.at("name");
            o->kind = oj.at("kind");
            o->body.pos = FV(oj.at("position"));
            o->body.vel = FV(oj.at("velocity"));
            if (o->kind == "collider") {
                o->col = std::make_unique<PQS_Collider>();
                o->col->sphere = sphere;
                Wire(o->col->owner, *o);
            } else if (o->kind == "advanced" || o->kind == "advanced_grav") {
                o->adv = std::make_unique<PQS_ColliderAdvanced>();
                o->adv->sphere = sphere;
                o->adv->useGravityCollider = true;
                o->adv->useVelocityCollider = o->kind == "advanced";
                Wire(o->adv->owner, *o);
                o->adv->owner.colliderId = NewSurfaceColliderId();
                idName[o->adv->owner.colliderId] = o->name + ".own";
            } else {
                o->part = std::make_unique<PQS_PartCollider>();
                o->part->sphere = sphere;
                Wire(o->part->owner, *o);
                for (const json& c : oj.at("colliders")) {
                    const int id = NewSurfaceColliderId();
                    o->part->owner.colliderIds.push_back(id);
                    idName[id] = c.get<std::string>();
                }
                o->part->Awake();
            }
            objs.push_back(std::move(o));
        }
        for (auto& o : objs) {
            if (o->col) o->col->Start();
            if (o->adv) o->adv->Start();
            if (o->part) o->part->Start();
        }
        auto nameBoxes = [&](const Obj& o) {
            if (o.col) idName[o.col->colliderObject.id] = o.col->colliderObject.name;
            if (o.adv) {
                if (o.adv->hasGravityCollider) idName[o.adv->gravityCollider.id] = o.adv->gravityCollider.name;
                if (o.adv->hasVelocityCollider) {
                    idName[o.adv->velocityCollider.id] = o.adv->velocityCollider.name;
                    idName[o.adv->velocityCliffCollider.id] = o.adv->velocityCliffCollider.name;
                }
            }
            if (o.part) {
                if (o.part->hasGravityCollider) idName[o.part->gravityCollider.id] = o.part->gravityCollider.name;
                if (o.part->hasVelocityCollider) {
                    idName[o.part->velocityCollider.id] = o.part->velocityCollider.name;
                    idName[o.part->velocityCliffCollider.id] = o.part->velocityCliffCollider.name;
                }
            }
        };
        for (auto& o : objs) nameBoxes(*o);
        const double dt = D(sc.at("dt"));
        const json& events = sc.at("events");
        const int steps = sc.at("steps").get<int>();
        for (int k = 0; k < steps; ++k) {
            const std::string key = std::to_string(k);
            if (events.contains(key)) {
                PQS* target = events.at(key) == "null" ? nullptr : sphere;
                for (auto& o : objs) {
                    if (o->col) o->col->SetTargetSphere(target);
                    if (o->adv) o->adv->SetTargetSphere(target);
                    if (o->part) o->part->SetTargetSphere(target);
                }
            }
            for (auto& o : objs) {
                if (o->col) o->col->FixedUpdate();
                if (o->adv) o->adv->FixedUpdate();
                if (o->part) o->part->FixedUpdate();
            }
            const json& want = sc.at("rows").at(static_cast<size_t>(k));
            for (size_t i = 0; i < objs.size(); ++i) {
                const Obj& o = *objs[i];
                json got;
                json bs = json::array();
                if (o.col) {
                    got["flags"] = json::array({o.col->IsColliderActive()});
                    bs.push_back(Box(o.col->colliderObject));
                } else if (o.adv) {
                    got["flags"] = json::array({o.adv->IsGravityColliderActive(), o.adv->IsVelocityColliderActive()});
                    bs.push_back(Box(o.adv->gravityCollider));
                    if (o.adv->useVelocityCollider) {
                        bs.push_back(Box(o.adv->velocityCollider));
                        bs.push_back(Box(o.adv->velocityCliffCollider));
                    }
                } else {
                    got["flags"] = json::array({o.part->isGravityColliderActive, o.part->isVelocityColliderActive});
                    bs.push_back(Box(o.part->gravityCollider));
                    bs.push_back(Box(o.part->velocityCollider));
                    bs.push_back(Box(o.part->velocityCliffCollider));
                }
                got["boxes"] = bs;
                got["ownerPosition"] = JV(o.body.pos);
                got["ownerVelocity"] = JV(o.body.vel);
                boxes += bs.size();
                if (got != want.at("objects").at(i)) {
                    if (mismatches < 5)
                        MESSAGE(sc.at("name").get<std::string>() << " step " << k << " " << o.name << ":\n got  " << got.dump() << "\n want "
                                                                 << want.at("objects").at(i).dump());
                    ++mismatches;
                }
            }
            ++rows;
            for (auto& o : objs) {
                const Vector3f p = o->body.pos, v = o->body.vel;
                o->body.pos = Vector3f(static_cast<float>(fx(p.x) + fx(v.x) * dt), static_cast<float>(fx(p.y) + fx(v.y) * dt),
                                       static_cast<float>(fx(p.z) + fx(v.z) * dt));
            }
        }
        // the IgnoreCollision log by collider name
        json log = json::array();
        for (const IgnoreCollisionLog::Call& c : PhysicsIgnoreCollisions().Calls()) log.push_back({idName[c.a], idName[c.b], c.ignore});
        calls += log.size();
        if (log != sc.at("ignore")) {
            MESSAGE(sc.at("name").get<std::string>() << " ignore log:\n got  " << log.dump() << "\n want " << sc.at("ignore").dump());
            ++mismatches;
        }
        objs.clear();
    }
    MESSAGE("WP-12d colliders: " << rows << " steps, " << boxes << " box records, " << calls << " IgnoreCollision calls, " << mismatches << " mismatches");
    CHECK(rows == 420);
    CHECK(mismatches == 0);
    PQS_ColliderAdvanced::ClearStatics();
    PQS_PartCollider::ClearStatics();
    PhysicsIgnoreCollisions().Clear();
}

TEST_CASE("WP-12d surface colliders: C# rules the reference scenarios do not reach") {
    auto sphere = terrain::LoadStockPQS("Kerbin", Maps());
    PQS_ColliderAdvanced::ClearStatics();
    PQS_PartCollider::ClearStatics();
    PhysicsIgnoreCollisions().Clear();
    SUBCASE("PQS_ColliderAdvanced.SetTargetSphere before Start dereferences the missing boxes (NullReferenceException)") {
        PQS_ColliderAdvanced a;
        a.useGravityCollider = true;
        CHECK_THROWS_AS(a.SetTargetSphere(sphere.get()), NullReferenceException);
    }
    SUBCASE("PQS_PartCollider.SetTargetSphere before Start returns early; FixedUpdate skips a kinematic / packed / landed owner") {
        PQS_PartCollider p;
        p.Awake();
        p.SetTargetSphere(sphere.get());
        CHECK(p.sphere == sphere.get());
        CHECK(!p.collidersSet);
        Vector3f pos(0.0f, 600100.0f, 0.0f), vel(0.0f, -50.0f, 0.0f);
        p.owner.position = [&] { return pos; };
        p.owner.velocity = [&] { return vel; };
        bool kinematic = true;
        p.owner.isKinematic = [&] { return kinematic; };
        p.sphere = sphere.get();
        p.Start();
        const int writes = p.gravityCollider.writes;
        p.FixedUpdate();
        CHECK(p.gravityCollider.writes == writes);   // kinematic: nothing updated
        kinematic = false;
        bool packed = true, landed = false;
        p.partPresent = true;
        p.part.packed = [&] { return packed; };
        p.part.hasVessel = [] { return true; };
        p.part.vesselLanded = [&] { return landed; };
        p.part.referenceBodySphere = [&] { return sphere.get(); };
        p.FixedUpdate();
        CHECK(p.gravityCollider.writes == writes);
        packed = false;
        landed = true;
        p.FixedUpdate();
        CHECK(p.gravityCollider.writes == writes);
        landed = false;
        p.FixedUpdate();
        CHECK(p.gravityCollider.writes > writes);
        // the reference body's sphere changes: SetTargetSphere parks the boxes at the safe spot, then the update places them again
        PQS* other = nullptr;
        p.part.referenceBodySphere = [&] { return other; };
        p.FixedUpdate();
        CHECK(p.sphere == nullptr);
        CHECK(p.gravityCollider.position.x == PQS_PartCollider::SafeSpot().x);
        CHECK(!p.isGravityColliderActive);
        p.part.hasVessel = [] { return false; };
        p.part.vesselLanded = [] { return false; };
        CHECK_THROWS_AS(p.FixedUpdate(), NullReferenceException);   // part.vessel.orbitDriver without a vessel
    }
    SUBCASE("BoxWorldPose: the gravity box's top face lies on the terrain under the owner; it is a placeable static box") {
        PQS_ColliderAdvanced a;
        a.sphere = sphere.get();
        a.useGravityCollider = true;
        const Vector3d dir = Vector3d(0.3, 0.1, -0.95).normalized();
        const double h = sphere->GetSurfaceHeight(dir);
        Vector3f pos = (dir * (h + 30.0)).ToVector3f(), vel(0.0f, 0.0f, 0.0f);
        a.owner.position = [&] { return pos; };
        a.owner.velocity = [&] { return vel; };
        a.owner.colliderId = NewSurfaceColliderId();
        a.Start();
        REQUIRE(a.IsGravityColliderActive());
        const PQSBoxWorldPose w = BoxWorldPose(a.gravityCollider, sphere.get(), Vector3d(pos));
        CHECK(w.halfExtents.y == 100.0f);
        // the top face centre: centre + rotation * (0, halfExtents.y, 0) = the terrain point under the owner (normal * 100 below it)
        const Vector3d top = w.centre + w.rotation * Vector3d(0.0, w.halfExtents.y, 0.0);
        const Vector3d surf = dir * h;
        MESSAGE("gravity box top vs terrain point: " << (top - surf).magnitude() << " m");
        CHECK((top - surf).magnitude() < 0.1);   // float box pose (local position at 600 km: 1/16 m) and the normal-vs-up rotation
        physics::PhysicsWorld world;
        const physics::BodyId id = world.AddStaticShape(1, physics::ShapeDesc::Box(w.halfExtents), w.centre, w.rotation);
        REQUIRE(id != physics::kInvalidBody);
        const physics::PhysicsWorld::RayHit hit = world.RayCast(Vector3d(pos), dir * -1.0, 200.0, physics::PhysicsWorld::kLayerStatic);
        REQUIRE(hit.hit);
        MESSAGE("ray from the owner meets the box " << hit.distance << " m down (terrain " << (Vector3d(pos).magnitude() - h) << " m)");
        CHECK(std::fabs(hit.distance - (Vector3d(pos).magnitude() - h)) < 0.2);
    }
    SUBCASE("PQSMod_CreateSphereCollider: radius = (float)(sphere.radius + radiusOffset)") {
        PQSMod_CreateSphereCollider m;
        m.sphere = sphere.get();
        m.radiusOffset = -25.5f;
        m.OnSetup();
        CHECK(m.hasSphereCollider);
        CHECK(m.sphereColliderRadius == static_cast<float>(600000.0 - 25.5));
    }
    PQS_ColliderAdvanced::ClearStatics();
    PQS_PartCollider::ClearStatics();
    PhysicsIgnoreCollisions().Clear();
}
