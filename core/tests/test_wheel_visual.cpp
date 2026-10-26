// WP-29e: the Transform writes of VPWheelCollider.UpdateVisualWheel (the suspension, caliper and wheel transforms of the model follow the suspension
// every frame; the colliders below them go with them) against the independent Python reference tools/wp29e/visual_reference.py.
//
// Gate: tests/data/wp29e_visual_reference.json (numpy float32 per operation, over the reduced stock models core/data/wheel_models): per case a part
// (13 stock wheel / gear / leg models with a suspension, caliper or wheel transform), a rescale factor, a random part pose, the update flags and the
// IsChildOf flags, then three consecutive writes on the same rig (random m_suspensionPosition, steer, spin). Expected after each write: the part-space
// pose of the moved transforms and of every transform that carries a collider. The C++ runs VPWheelCollider::WriteVisualTransforms (the ported
// statements, VPWheelCollider.cs:275-300) through MakeRigVisualHost (Transform.position / localRotation / IsChildOf on the PartModelRig).
// Tolerance: the Euler rotations use the UCRT's sinf / cosf (the reference rounds the double functions to float): within 2e-6 on quaternion
// components and 2e-6 on positions (a few ULP of metre-sized values); the moves without a rotation are float-exact.
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>

#include "doctest/doctest.h"
#include "ksp/physics/physics_world.hpp"
#include "ksp/physics/wheel_models.hpp"
#include "ksp/physics/wheel_visual.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using namespace ksp::VehiclePhysics;
using nlohmann::json;

namespace {

const physics::WheelModelLibrary& Lib() {
    static const physics::WheelModelLibrary lib(std::string(KSPCORE_DATA_DIR) + "/wheel_models");
    return lib;
}

const json& Ref() {
    static const json ref = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp29e_visual_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return ref;
}

Vector3f V3(const json& j, size_t o = 0) { return Vector3f(j[o].get<float>(), j[o + 1].get<float>(), j[o + 2].get<float>()); }

}  // namespace

TEST_CASE("WP-29e VPWheelCollider.UpdateVisualWheel Transform writes: the suspension / caliper / wheel transforms and the colliders below them vs the independent Python reference") {
    const json& ref = Ref();
    REQUIRE(ref["cases"].size() > 100);
    double maxPos = 0.0, maxRot = 0.0, maxScale = 0.0, maxMoved = 0.0, maxTurned = 0.0;
    size_t compared = 0, withWheel = 0, withCaliper = 0, withSusp = 0;
    std::map<std::string, int> parts;
    for (const json& c : ref["cases"]) {
        const std::string part = c["part"].get<std::string>();
        parts[part]++;
        std::shared_ptr<mu::PartModelRig> rig = Lib().BuildRig(part, c["rescale"].get<float>());
        REQUIRE(rig != nullptr);
        const json& moved = c["moved"];
        NativeTransform handle[3];
        int nodeOfHandle[3];
        for (int k = 0; k < 3; ++k) nodeOfHandle[k] = moved[k].get<int>();
        auto nodeOf = [&](const NativeTransform* t) {
            for (int k = 0; k < 3; ++k)
                if (t == &handle[k]) return nodeOfHandle[k];
            return -1;
        };
        const json& pp = c["partPose"];
        NativeTransform part_pose;
        part_pose.position = V3(pp);
        part_pose.rotation = Quaternionf(pp[3].get<float>(), pp[4].get<float>(), pp[5].get<float>(), pp[6].get<float>());
        bool hidden = false;
        VPWheelCollider::VisualTransformHost host = physics::MakeRigVisualHost(*rig, [&] { return part_pose; }, nodeOf, &hidden);
        // the IsChildOf flags the host answers must be what the reference computed from the hierarchy (the flags themselves are the case's)
        VPWheelCollider::VisualWriteState st;
        st.suspensionTransform = nodeOfHandle[0] >= 0 ? &handle[0] : nullptr;
        st.caliperTransform = nodeOfHandle[1] >= 0 ? &handle[1] : nullptr;
        st.wheelTransform = nodeOfHandle[2] >= 0 ? &handle[2] : nullptr;
        withSusp += st.suspensionTransform != nullptr;
        withCaliper += st.caliperTransform != nullptr;
        withWheel += st.wheelTransform != nullptr;
        st.updateSuspension = c["update"][0].get<bool>();
        st.updateCaliper = c["update"][1].get<bool>();
        st.updateWheel = c["update"][2].get<bool>();
        st.caliperChildOfSuspension = c["childOf"][0].get<bool>();
        st.wheelChildOfSuspension = c["childOf"][1].get<bool>();
        st.wheelChildOfCaliper = c["childOf"][2].get<bool>();
        if (c["childOf"][0].get<bool>() || c["childOf"][1].get<bool>() || c["childOf"][2].get<bool>()) {
            // the flags of a re-enabled component are what Transform.IsChildOf says on this hierarchy
            if (st.caliperTransform && st.suspensionTransform) CHECK(host.isChildOf(st.caliperTransform, st.suspensionTransform) == st.caliperChildOfSuspension);
            if (st.wheelTransform && st.suspensionTransform) CHECK(host.isChildOf(st.wheelTransform, st.suspensionTransform) == st.wheelChildOfSuspension);
            if (st.wheelTransform && st.caliperTransform) CHECK(host.isChildOf(st.wheelTransform, st.caliperTransform) == st.wheelChildOfCaliper);
        }
        const json& carriers = c["carriers"];
        const mu::PartModelRig prefab = *rig;  // the poses before any write: the writes must have moved things
        for (const json& step : c["steps"]) {
            st.suspensionPosition = V3(step["suspensionPosition"]);
            st.visualSteerAngle = step["steer"].get<float>();
            st.angularPosition = step["spin"].get<float>();
            VPWheelCollider::WriteVisualTransforms(st, host);
            for (size_t k = 0; k < carriers.size(); ++k) {
                const mu::RigPose got = rig->PartSpace(carriers[k].get<int>());
                const json& e = step["poses"][k];
                const Vector3f ep = V3(e, 0);
                const Quaternionf eq(e[3].get<float>(), e[4].get<float>(), e[5].get<float>(), e[6].get<float>());
                const Vector3f es = V3(e, 7);
                maxPos = std::max({maxPos, std::fabs(static_cast<double>(got.position.x - ep.x)), std::fabs(static_cast<double>(got.position.y - ep.y)),
                                   std::fabs(static_cast<double>(got.position.z - ep.z))});
                // a quaternion and its negative are one rotation: compare up to the sign
                const double sgn = (got.rotation.x * eq.x + got.rotation.y * eq.y + got.rotation.z * eq.z + got.rotation.w * eq.w) < 0.0 ? -1.0 : 1.0;
                maxRot = std::max({maxRot, std::fabs(got.rotation.x - sgn * eq.x), std::fabs(got.rotation.y - sgn * eq.y), std::fabs(got.rotation.z - sgn * eq.z),
                                   std::fabs(got.rotation.w - sgn * eq.w)});
                maxScale = std::max({maxScale, std::fabs(static_cast<double>(got.scale.x - es.x)), std::fabs(static_cast<double>(got.scale.y - es.y)),
                                     std::fabs(static_cast<double>(got.scale.z - es.z))});
                ++compared;
                const mu::RigPose p0 = prefab.PartSpace(carriers[k].get<int>());
                maxMoved = std::max({maxMoved, std::fabs(static_cast<double>(got.position.x - p0.position.x)), std::fabs(static_cast<double>(got.position.y - p0.position.y)),
                                     std::fabs(static_cast<double>(got.position.z - p0.position.z))});
                maxTurned = std::max({maxTurned, std::fabs(static_cast<double>(got.rotation.x - p0.rotation.x)), std::fabs(static_cast<double>(got.rotation.y - p0.rotation.y)),
                                      std::fabs(static_cast<double>(got.rotation.z - p0.rotation.z))});
            }
        }
    }
    MESSAGE("compared " << compared << " node poses in " << ref["cases"].size() << " cases of " << parts.size() << " models (suspension transform in " << withSusp
                        << ", caliper in " << withCaliper << ", wheel in " << withWheel << "): max |position| " << maxPos << " m, max |rotation| " << maxRot << ", max |scale| " << maxScale
                        << "; the writes moved a node by up to " << maxMoved << " m and turned one by up to " << maxTurned << " (quaternion component)");
    CHECK(parts.size() >= 11);
    CHECK(withSusp > 40);
    CHECK(withCaliper > 30);
    CHECK(withWheel > 40);
    CHECK(maxPos < 2.0e-6);
    CHECK(maxRot < 2.0e-6);
    CHECK(maxScale < 1.0e-7);
    CHECK(maxMoved > 0.1);   // the gate is not vacuous
    CHECK(maxTurned > 0.1);
}

TEST_CASE("WP-29e VPWheelCollider.UpdateVisualWheel: nothing is written without the host's Transform operations or while the update flags are off") {
    NativeTransform susp, wheel;
    VPWheelCollider::VisualWriteState st;
    st.suspensionTransform = &susp;
    st.wheelTransform = &wheel;
    st.updateSuspension = st.updateWheel = true;
    st.suspensionPosition = Vector3f(1.0f, 2.0f, 3.0f);
    int writes = 0;
    VPWheelCollider::VisualTransformHost none;  // no setPosition / setLocalRotation
    VPWheelCollider::WriteVisualTransforms(st, none);
    CHECK(writes == 0);
    VPWheelCollider::VisualTransformHost host;
    host.setPosition = [&](NativeTransform*, const Vector3f&) { ++writes; };
    host.setLocalRotation = [&](NativeTransform*, const Quaternionf&) { ++writes; };
    st.updateSuspension = st.updateWheel = false;
    VPWheelCollider::WriteVisualTransforms(st, host);
    CHECK(writes == 0);
    // the C# order: with both flags on the suspension is written first (position), then the wheel (position, rotation)
    st.updateSuspension = st.updateWheel = true;
    std::vector<std::pair<NativeTransform*, int>> order;
    host.setPosition = [&](NativeTransform* t, const Vector3f&) { order.emplace_back(t, 0); };
    host.setLocalRotation = [&](NativeTransform* t, const Quaternionf&) { order.emplace_back(t, 1); };
    VPWheelCollider::WriteVisualTransforms(st, host);
    REQUIRE(order.size() == 3);
    CHECK(order[0] == std::make_pair(&susp, 0));
    CHECK(order[1] == std::make_pair(&wheel, 0));
    CHECK(order[2] == std::make_pair(&wheel, 1));
}

// PhysicsWorld::SetCompoundChildPoses (Jolt MutableCompoundShape::ModifyShape + BodyInterface::NotifyShapeChanged): a child that moves moves the contact.
// A box with a sphere hung 0.25 m under it rests on a static floor on the sphere; the sphere is lifted 0.2 m into the box's shadow, and the body drops by the
// same 0.2 m (analytic: the lowest point of the compound is the sphere's bottom, then the box's); a static compound refuses.
TEST_CASE("WP-29e mutable compound: moving a child moves the contact (a sphere under a box, lifted 0.2 m), a static compound refuses") {
    using namespace ksp::physics;
    constexpr double kG = 9.80665;
    auto make = [](PhysicsWorld& w, bool mutableChildren) {
        CompoundBodyDesc d;
        CompoundChild box;
        box.shape = ShapeDesc::Box(Vector3f(0.5f, 0.1f, 0.5f));
        box.userData = 1;
        CompoundChild ball;
        ball.shape = ShapeDesc::Sphere(0.1f);
        ball.position = Vector3f(0.0f, -0.25f, 0.0f);
        ball.userData = 2;
        d.children = {box, ball};
        d.position = Vector3d(1.0e5, 3.0, -2.0e5);
        d.mass = 10.0;
        d.inertia[0][0] = d.inertia[1][1] = d.inertia[2][2] = 1.0;
        d.mutableChildren = mutableChildren;
        return w.CreateCompoundBody(d);
    };
    auto settle = [&](PhysicsWorld& w, BodyId id) {
        for (int i = 0; i < 400; ++i) {
            w.AddForce(id, Vector3d(0, -kG * w.GetMass(id), 0));
            w.Step(0.02f);
        }
        return w.GetPosition(id).y;
    };
    PhysicsWorld w;
    BodyDesc floor;
    floor.shape = ShapeDesc::Box(Vector3f(50, 0.5f, 50));
    floor.motion = MotionType::Static;
    floor.position = Vector3d(1.0e5, -0.5, -2.0e5);
    w.CreateBody(floor);
    const BodyId id = make(w, true);
    const double y0 = settle(w, id);
    CHECK(std::fabs(y0 - 0.35) < 0.03);  // the sphere's bottom, 0.25 + 0.1 under the origin (Jolt's penetration slop is 0.02 m)
    Vector3f p;
    Quaternionf q;
    REQUIRE(w.GetCompoundChildPose(id, 1, p, q));
    CHECK(std::fabs(p.y + 0.25f) < 1e-6f);
    CHECK(w.SetCompoundChildPoses(id, {CompoundChildPose{1, Vector3f(0.0f, -0.05f, 0.0f), Quaternionf::identity()}}));
    REQUIRE(w.GetCompoundChildPose(id, 1, p, q));
    CHECK(std::fabs(p.y + 0.05f) < 1e-6f);
    CHECK(std::fabs(p.x) < 1e-6f);
    const double y1 = settle(w, id);   // the box's bottom (0.1 under the origin) is now the lowest: the sphere's bottom is at -0.15, below it
    // lowest point = min(box bottom -0.1, sphere bottom -0.05 - 0.1 = -0.15) = -0.15: the sphere still reaches below the box by 0.05
    CHECK(std::fabs((y0 - y1) - 0.20) < 0.03);
    // a static compound cannot be modified
    PhysicsWorld w2;
    const BodyId sid = make(w2, false);
    CHECK(!w2.SetCompoundChildPoses(sid, {CompoundChildPose{1, Vector3f(0.0f, -0.05f, 0.0f), Quaternionf::identity()}}));
    Vector3f p2;
    Quaternionf q2;
    CHECK(!w2.GetCompoundChildPose(sid, 1, p2, q2));
}
