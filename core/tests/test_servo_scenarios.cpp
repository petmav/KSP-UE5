// WP-52b gate, part 2: the servos closed loop on the reduced rigid-body model (VesselPhysics + the Jolt backend, servo_backend.hpp) against
// the reference plant (HeadlessServoPlant, the documented 1-DOF D6 drive) with the same effective inertia, against closed forms of the drive
// law, and the structural loads across the servo joint. The craft is the first two parts of PhysProf-S (a probeStackLarge root with a
// Rockomax32 under it); the servo MODULE on the root is a labelled SYNTHETIC fixture (no SquadExpansion part exists locally), its moving
// side the root's "bottom" node. Sign conventions and the limit / spring behaviour of the drive are UNVERIFIED (no capture).
#include "robotics_fixture.hpp"
#include "flight_integrator_fixture.hpp"
#include "vessel_physics_fixture.hpp"

#include "ksp/physics/physics_world.hpp"
#include "ksp/serenity/servo_backend.hpp"

using namespace wp52t;

namespace {

constexpr double kPi = 3.14159265358979323846;

// the resources of several test vessels (TestVesselHost serves one)
struct MultiHost {
    struct One final : IResourceVessel {
        Vessel* v = nullptr;
        VesselResourceSets sets;
        const std::vector<Part*>& ResourceParts() const override { return v->parts; }
        double RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) override {
            return sets.RequestResource(part, id, demand, usePriority, simulate);
        }
        void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling) override {
            sets.GetConnectedResourceTotals(id, simulate, amount, maxAmount, pulling);
        }
        void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, double threshold, bool pulling) override {
            sets.GetConnectedResourceTotals(id, simulate, amount, maxAmount, threshold, pulling);
        }
    };
    std::vector<std::unique_ptr<One>> hosts;
    MultiHost() {
        ResourceFlowEnvironment::vesselHost = [this](Vessel* x) -> IResourceVessel* {
            for (auto& h : hosts)
                if (h->v == x) return h.get();
            return nullptr;
        };
    }
    ~MultiHost() {
        ResourceFlowEnvironment::vesselHost = nullptr;
        for (auto& h : hosts)
            for (Part* p : h->v->parts) {
                p->crossfeedPartSet.reset();
                p->simulationCrossfeedPartSet.reset();
            }
    }
    void Remove(Vessel* v) {
        for (size_t i = 0; i < hosts.size(); ++i)
            if (hosts[i]->v == v) {
                for (Part* p : v->parts) {
                    p->crossfeedPartSet.reset();
                    p->simulationCrossfeedPartSet.reset();
                }
                hosts.erase(hosts.begin() + static_cast<long>(i));
                return;
            }
    }
    void Add(Vessel* v) {
        auto h = std::make_unique<One>();
        h->v = v;
        h->sets.UpdateResourceSets(v, v->parts);
        hosts.push_back(std::move(h));
    }
};

// the electric charge of the root (SYNTHETIC: a big store so the servo never runs out)
void Charge(Part& p) {
    PartResource* r = p.GetResource("ElectricCharge");
    if (r == nullptr) {
        ConfigNode rn("RESOURCE");
        rn.AddValue("name", "ElectricCharge");
        rn.AddValue("amount", "0");
        rn.AddValue("maxAmount", "100");
        p.AddResource(rn);
        r = p.GetResource("ElectricCharge");
    }
    r->maxAmount = 1.0e6;
    r->amount = 1.0e6;
    p.ResetSimulationResources();
}

struct Scene {
    wp20b::World w;
    SceneScope flight{GameScenes::FLIGHT};
    EnvReset reset;
    TimeWarp tw;
    TimeWarpEnvironment twEnv;
    MultiHost host;
    Scene() {
        SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", true);
        twEnv.LoadedSceneValue = GameScenes::FLIGHT;
        TimeWarp::environment = &twEnv;
        TimeWarp::fetch = &tw;
        tw.warpRates = {1.0f, 5.0f, 10.0f, 50.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f};
        tw.physicsWarpRates = {1.0f, 2.0f, 3.0f, 4.0f};
        tw.maxPhysicsRate_index = 3;
        tw.Mode = TimeWarp::Modes::HIGH;
        tw.curr_rate = 1.0f;
        tw.current_rate_index = 0;
        Time::fixedDeltaTime = 0.02f;
        Time::deltaTime = 0.02f;
    }
    ~Scene() {
        TimeWarp::fetch = nullptr;
        TimeWarp::environment = nullptr;
    }
};

// one servo craft: the vessel, the servo on its root, optionally its binding with the root's cluster welded to the world
struct Craft {
    Scene* scene = nullptr;
    std::unique_ptr<Vessel> v;
    Part* root = nullptr;
    Part* tank = nullptr;
    BaseServo* servo = nullptr;
    std::unique_ptr<physics::PhysicsWorld> world;
    std::unique_ptr<physics::VesselPhysics> vp;
    physics::ConstraintId anchor = physics::kInvalidConstraint;
    template <class T>
    T* Make(Scene& s, const char* cls, const std::function<void(T&)>& configure) {
        scene = &s;
        v = s.w.CraftHead("PhysProf-S", 2);
        root = v->rootPart;
        REQUIRE(root != nullptr);
        REQUIRE(v->parts.size() == 2);
        tank = v->parts[1];
        Charge(*root);
        s.host.Add(v.get());
        T* t = AddServo<T>(*root, cls, 0.1);
        t->template Set<std::string>("servoAttachNodes", "bottom");
        configure(*t);
        WireRig(*t);
        t->OnStart(StartState::None);
        t->OnStartBeforePartAttachJoint(StartState::None);
        servo = t;
        return t;
    }
    // servoSteps < 0: VesselPhysicsOptions' default solver iterations of a servo's island
    void Bind(bool anchored, int threads = 1, physics::LoadSolve solve = physics::LoadSolve::Auto, int servoSteps = -1, float angularDrag = -1.0f) {
        physics::PhysicsWorldConfig cfg;
        cfg.threads = threads;
        world = std::make_unique<physics::PhysicsWorld>(cfg);
        physics::VesselPhysicsOptions opt;
        UseServoBackend(opt);
        opt.loadSolve = solve;
        if (servoSteps >= 0) opt.servoVelocitySteps = servoSteps;
        if (angularDrag >= 0.0f) opt.defaultAngularDrag = angularDrag;  // rb.angularDrag (and servoRb's) until a FlightIntegrator sets it
        vp = std::make_unique<physics::VesselPhysics>(*world, *v, physics::VesselPose{}, opt);
        if (anchored) anchor = world->CreateFixedToWorldConstraint(vp->ClusterBody(static_cast<size_t>(vp->ClusterOf(*root))), vp->PartState(*root).position);
    }
    // one fixed tick: the servo's FixedUpdate (the module slot), the physics step, its Update (the frame)
    void Tick() {
        servo->FixedUpdate();
        vp->Flush(0.02f);
        world->Step(0.02f);
        vp->AfterStep();
        servo->Update();
    }
    // the moving side's effective inertia about the servo axis through the anchor (the base held): n.I.n + M d^2
    double EffectiveInertia() const {
        const size_t c = static_cast<size_t>(vp->ServoCluster(*root));
        double I[3][3];
        vp->ClusterInertia(c, I);
        physics::ServoBodySpec spec;
        REQUIRE(ServoBodySpecOf(*root, spec));
        const physics::PartPhysicsState rs = vp->PartState(*root);
        const Vector3d n = (rs.rotation * spec.axis).normalized();
        const Vector3d p = rs.position + rs.rotation * spec.anchor;
        double nIn = 0.0;
        const double nv[3] = {n.x, n.y, n.z};
        for (int a = 0; a < 3; ++a)
            for (int b = 0; b < 3; ++b) nIn += nv[a] * I[a][b] * nv[b];
        const Vector3d r = vp->ClusterCenterOfMass(c) - p;
        const Vector3d perp = r - n * Vector3d::Dot(r, n);
        return nIn + vp->ClusterMass(c) * perp.sqrMagnitude();
    }
    void Done() {
        if (servo != nullptr) servo->OnDestroy();
        servo = nullptr;
        vp.reset();
        world.reset();
        if (scene != nullptr && v) scene->host.Remove(v.get());  // the part sets point into the host's resource sets
        scene = nullptr;
    }
    ~Craft() { Done(); }
};

void PlantTick(BaseServo& s, HeadlessServoPlant& plant) {
    s.FixedUpdate();
    plant.Step(0.02);
    s.Update();
}

}  // namespace

TEST_CASE("WP-52b servo split: the servo body is a cluster of its own side, rb and servoRb share the rb mass, one articulated joint") {
    if (!wp20b::GameAvailable()) return;
    Scene s;
    Craft c;
    c.Make<ModuleRoboticServoHinge>(s, "ModuleRoboticServoHinge", [](ModuleRoboticServoHinge&) {});
    c.Bind(false);
    physics::VesselPhysics& vp = *c.vp;
    CHECK(vp.ClusterCount() == 2);
    CHECK(vp.ConstraintCount() == 1);
    CHECK(vp.ServoCount() == 1);
    CHECK(vp.PartHasServoBody(*c.root));
    CHECK(!vp.PartHasServoBody(*c.tank));
    CHECK(vp.ServoJoint(*c.root) != physics::kInvalidConstraint);
    CHECK(c.world->IsArticulated(vp.ServoJoint(*c.root)));
    // the tank rides on the servo body (AttachServoRigidBody: the "bottom" node is on the moving side), the root on its own cluster
    CHECK(vp.ServoCluster(*c.root) == vp.ClusterOf(*c.tank));
    CHECK(vp.ServoCluster(*c.root) != vp.ClusterOf(*c.root));
    // rb.mass = servoRb.mass = num2 / 2f (FlightIntegrator.cs:476-480)
    const float num2 = physics::PartRigidbodyMass(*c.root);
    CHECK(vp.PartMass(*c.root) == num2 / 2.0f);
    CHECK(vp.ServoBodyMass(*c.root) == num2 / 2.0f);
    const double total = vp.ClusterMass(0) + vp.ClusterMass(1);
    CHECK(total == doctest::Approx(static_cast<double>(num2) + static_cast<double>(physics::PartRigidbodyMass(*c.tank))).epsilon(1e-12));
    // the servo body and the tank do not move apart; the joint reads the moving transform's coordinate (0)
    CHECK(vp.ServoCoordinate(*c.root) == 0.0);
    // without the backend's hooks (isArticulation only, WP-52) the same craft is the welded split: 2 clusters, one fixed constraint
    physics::PhysicsWorld w2;
    physics::VesselPhysicsOptions o2;
    o2.isArticulation = [](const physics::PartJointSpec& j) { return ServoArticulation(j); };
    physics::VesselPhysics vp2(w2, *c.v, physics::VesselPose{}, o2);
    CHECK(vp2.ClusterCount() == 2);
    CHECK(vp2.ConstraintCount() == 1);
    CHECK(vp2.ServoCount() == 0);
    CHECK(vp2.PartMass(*c.root) == num2);
    c.Done();
}

TEST_CASE("WP-52b hinge to a target angle: the Jolt joint follows the reference plant of the same inertia, settles on the target, lag c/k*rate") {
    if (!wp20b::GameAvailable()) return;
    auto configure = [](ModuleRoboticServoHinge& h) {
        h.Set<Vector2f>("softMinMaxAngles", Vector2f(-90.0f, 90.0f));
        h.Set<Vector2f>("traverseVelocityLimits", Vector2f(0.1f, 180.0f));
        h.Set<float>("traverseVelocity", 10.0f);
    };
    // the production solver iterations of a servo's island, then 200: the Gauss-Seidel error with the arm's lever converges away
    for (int steps : {-1, 200}) {
        INFO("servo island velocity steps " << (steps < 0 ? physics::VesselPhysicsOptions{}.servoVelocitySteps : steps));
        Scene s;
        Craft c, ref;
        auto* h = c.Make<ModuleRoboticServoHinge>(s, "ModuleRoboticServoHinge", configure);
        auto* hr = ref.Make<ModuleRoboticServoHinge>(s, "ModuleRoboticServoHinge", configure);
        c.Bind(true, 1, physics::LoadSolve::Auto, steps);
        const double I = c.EffectiveInertia();
        ServoPlantParams pp;
        pp.inertia = I;  // the plant's SYNTHETIC inertia set to the moving side's
        HeadlessServoPlant plant(*hr, pp);
        plant.SetInitial(0.0);
        h->Fields()["targetAngle"]->SetValue(ModuleValue(40.0f));
        hr->Fields()["targetAngle"]->SetValue(ModuleValue(40.0f));
        const JointDrive d = h->servoJoint->angularXDrive;
        INFO("I_eff " << I << " t m^2, k " << d.positionSpring << ", c " << d.positionDamper << ", F " << d.maximumForce);
        double worst = 0.0, lagEnd = 0.0;
        for (int t = 1; t <= 600; ++t) {
            c.Tick();
            PlantTick(*hr, plant);
            const double a = c.vp->ServoCoordinate(*c.root);
            worst = std::max(worst, std::fabs(a - plant.state.position));
            if (t == 150) lagEnd = h->driveTargetAngle * kPi / 180.0 - a;  // 40 deg at 10 deg/s: the ramp ends at t = 200
            // the moving transform the module reads is the joint's coordinate
            REQUIRE(std::fabs(h->currentTransformAngle() - a * 180.0 / kPi) < 1e-3);
        }
        MESSAGE("hinge: worst |Jolt - plant| " << worst << " rad, lag on the ramp " << lagEnd << " rad, final " << c.vp->ServoCoordinate(*c.root));
        CHECK(worst < (steps < 0 ? 1e-3 : 1e-4));
        CHECK(c.vp->ServoCoordinate(*c.root) == doctest::Approx(40.0 * kPi / 180.0).epsilon(1e-3));
        // the overdamped drive's steady lag behind a ramp is c / k * rate
        const double rate = 10.0 * kPi / 180.0;
        CHECK(lagEnd == doctest::Approx(static_cast<double>(d.positionDamper) / static_cast<double>(d.positionSpring) * rate).epsilon(0.02));
    }
}

TEST_CASE("WP-52b piston extension: the Jolt slider follows the reference plant of the same mass and reaches the target extension") {
    if (!wp20b::GameAvailable()) return;
    auto configure = [](ModuleRoboticServoPiston& p) {
        p.Set<Vector2f>("softMinMaxExtension", Vector2f(0.0f, 2.0f));
        p.Set<Vector2f>("hardMinMaxLimits", Vector2f(0.0f, 2.0f));
        p.Set<Vector2f>("traverseVelocityLimits", Vector2f(0.05f, 5.0f));
        p.Set<float>("traverseVelocity", 0.5f);
    };
    for (int steps : {-1, 200}) {
        INFO("servo island velocity steps " << (steps < 0 ? physics::VesselPhysicsOptions{}.servoVelocitySteps : steps));
        Scene s;
        Craft c, ref;
        auto* p = c.Make<ModuleRoboticServoPiston>(s, "ModuleRoboticServoPiston", configure);
        auto* pr = ref.Make<ModuleRoboticServoPiston>(s, "ModuleRoboticServoPiston", configure);
        c.Bind(true, 1, physics::LoadSolve::Auto, steps);
        const double M = c.vp->ClusterMass(static_cast<size_t>(c.vp->ServoCluster(*c.root)));
        ServoPlantParams pp;
        pp.inertia = M;
        HeadlessServoPlant plant(*pr, pp);
        plant.SetInitial(-1.0);  // the extension starts at 0, the joint's coordinate is measured from the middle of the travel
        CHECK(c.vp->ServoCoordinate(*c.root) == doctest::Approx(-1.0));
        const Vector3d tank0 = c.vp->PartState(*c.tank).position;
        p->Fields()["targetExtension"]->SetValue(ModuleValue(1.5f));
        pr->Fields()["targetExtension"]->SetValue(ModuleValue(1.5f));
        double worst = 0.0;
        for (int t = 1; t <= 400; ++t) {
            c.Tick();
            PlantTick(*pr, plant);
            worst = std::max(worst, std::fabs(c.vp->ServoCoordinate(*c.root) - plant.state.position));
        }
        MESSAGE("piston: M " << M << " t, worst |Jolt - plant| " << worst << " m, extension " << p->currentTransformPosition());
        CHECK(worst < (steps < 0 ? 6e-3 : 1e-3));
        CHECK(p->currentTransformPosition() == doctest::Approx(1.5f).epsilon(1e-3));
        // the tank moved 1.5 m along the main axis in part space (the servo's axis through the root)
        physics::ServoBodySpec spec;
        REQUIRE(ServoBodySpecOf(*c.root, spec));
        const Vector3d moved = c.vp->PartState(*c.tank).position - tank0;
        const Vector3d axis = (c.vp->PartState(*c.root).rotation * spec.axis).normalized();
        CHECK(Vector3d::Dot(moved, axis) == doctest::Approx(1.5).epsilon(1e-3));
        CHECK((moved - axis * Vector3d::Dot(moved, axis)).magnitude() < 1e-3);
    }
}

TEST_CASE("WP-52b rotor at rpm under its torque limit: the spool-up is torque-limited (dw/dt = tau / I), the rate holds rpmLimit, momentum is kept") {
    if (!wp20b::GameAvailable()) return;
    Scene s;
    auto configure = [](ModuleRoboticServoRotor& r) {
        r.Set<std::string>("mainAxis", "Y");             // SYNTHETIC: the stack axis, through the tank's centre of mass
        r.Set<Vector2f>("traverseVelocityLimits", Vector2f(0.1f, 460.0f));
        r.Set<float>("rpmLimit", 30.0f);
        r.Set<float>("maxTorque", 40.0f);
        r.Set<float>("angularPositionDamper", 1.0e4f);  // SYNTHETIC: a stiff velocity drive, so the torque limit binds
        r.Set<float>("rotorSpoolTime", 1000.0f);        // SYNTHETIC: the drive target jumps to workingRPM at once
    };
    // {anchored, angular drag}: the base welded to the world without rb.angularDrag (the plant has none), with Unity's default 0.05 on rb and
    // servoRb (the steady rate then balances the drive against the drag of the spinning bodies), free floating without drag (momentum)
    const struct {
        bool anchored;
        float drag;
    } runs[3] = {{true, 0.0f}, {true, 0.05f}, {false, 0.0f}};
    for (const auto& run : runs) {
        INFO("anchored " << run.anchored << ", angular drag " << run.drag);
        Craft c, ref;
        auto* r = c.Make<ModuleRoboticServoRotor>(s, "ModuleRoboticServoRotor", configure);
        auto* rr = ref.Make<ModuleRoboticServoRotor>(s, "ModuleRoboticServoRotor", configure);
        c.Bind(run.anchored, 1, physics::LoadSolve::Auto, -1, run.drag);
        r->Fields()["servoMotorLimit"]->SetValue(ModuleValue(100.0f));
        rr->Fields()["servoMotorLimit"]->SetValue(ModuleValue(100.0f));
        const double tau = static_cast<double>(r->totalTorque);
        const double want = static_cast<double>(r->workingRPM);  // rad/s (rpm * RPM2RadPerSec)
        const double I = c.EffectiveInertia();
        ServoPlantParams pp;
        pp.inertia = I;
        HeadlessServoPlant plant(*rr, pp);
        double worst = 0.0, rateAt02 = 0.0;
        const int ticks = static_cast<int>(std::ceil(want * I / tau / 0.02)) + 150;
        for (int t = 1; t <= ticks; ++t) {
            c.Tick();
            PlantTick(*rr, plant);
            worst = std::max(worst, std::fabs(c.vp->ServoRate(*c.root) - plant.state.velocity));
            if (t == 10) rateAt02 = c.vp->ServoRate(*c.root);
        }
        const double rate = c.vp->ServoRate(*c.root);
        MESSAGE("rotor: tau " << tau << " kN m, I_eff " << I << " t m^2, target " << want << " rad/s, rate " << rate << ", worst |Jolt - plant| " << worst);
        if (run.anchored && run.drag == 0.0f) {
            CHECK(worst < 2e-3 * want);  // the corner where the torque limit lets go: a fraction of one tick's 0.1 rad/s
            if (tau / I * 0.2 < want) CHECK(rateAt02 == doctest::Approx(tau / I * 0.2).epsilon(0.005));  // torque-limited: 0.2 s at tau / I
            CHECK(rate == doctest::Approx(want).epsilon(1e-4));
            // the rate of motion the module measures from the Transform is rpmLimit
            CHECK(r->transformRateOfMotion == doctest::Approx(30.0f).epsilon(0.02));
            CHECK(r->IsMoving());
        } else if (run.anchored) {
            // rb.angularDrag and servoRb.angularDrag (0.05 each: PhysX's per-body damping w *= 1 - dt d, as the torque d I_own w on the cluster) on
            // the spinning parts' own tensors: the drive's damper balances it, c (vt - v) = d I_own v  ->  v = vt c / (c + d I_own)
            const double cdr = static_cast<double>(r->servoJoint->angularXDrive.positionDamper);
            const double d = 0.05;
            const double own = static_cast<double>(c.vp->PartInertia(*c.tank).y) + static_cast<double>(c.vp->PartInertia(*c.root).y);
            const double expect = want * cdr / (cdr + d * own);
            MESSAGE("drag: own inertia " << own << " t m^2, expected rate " << expect);
            CHECK(rate == doctest::Approx(expect).epsilon(2e-5));
            CHECK(rate < want);
        } else {
            // free floating: the reaction spins the base the other way; the total angular momentum stays 0 (no external torque)
            Vector3d L;
            for (size_t k = 0; k < c.vp->ClusterCount(); ++k) {
                double Iw[3][3];
                c.vp->ClusterInertia(k, Iw);
                const Vector3d w = c.vp->ClusterAngularVelocity(k);
                const Vector3d com = c.vp->ClusterCenterOfMass(k);
                const Vector3d vv = c.vp->ClusterVelocity(k);
                L += Vector3d(Iw[0][0] * w.x + Iw[0][1] * w.y + Iw[0][2] * w.z, Iw[1][0] * w.x + Iw[1][1] * w.y + Iw[1][2] * w.z,
                              Iw[2][0] * w.x + Iw[2][1] * w.y + Iw[2][2] * w.z) +
                     Vector3d::Cross(com, vv * c.vp->ClusterMass(k));
            }
            const double Lscale = I * std::fabs(rate);
            MESSAGE("free rotor: |L| " << L.magnitude() << " of I w " << Lscale);
            CHECK(L.magnitude() < 1e-4 * Lscale);
            CHECK(rate == doctest::Approx(want).epsilon(1e-4));  // the relative rate is the drive's target
            CHECK(Vector3d::Dot(c.vp->ClusterAngularVelocity(static_cast<size_t>(c.vp->ClusterOf(*c.root))),
                                c.vp->ClusterAngularVelocity(static_cast<size_t>(c.vp->ServoCluster(*c.root)))) < 0.0);
        }
    }
}

TEST_CASE("WP-52b locked servo: the drive holds the lock angle against a load (offset tau / k), the joint loads carry the load across the servo") {
    if (!wp20b::GameAvailable()) return;
    Scene s;
    Craft c;
    auto* h = c.Make<ModuleRoboticServoHinge>(s, "ModuleRoboticServoHinge", [](ModuleRoboticServoHinge& x) {
        x.Set<Vector2f>("softMinMaxAngles", Vector2f(-90.0f, 90.0f));
        x.Set<Vector2f>("traverseVelocityLimits", Vector2f(0.1f, 180.0f));
        x.Set<float>("traverseVelocity", 30.0f);
    });
    c.Bind(true, 1, physics::LoadSolve::Tree);
    for (int t = 0; t < 100; ++t) c.Tick();  // at rest at 0
    REQUIRE(h->EngageServoLock());
    CHECK(h->Get<bool>("servoIsLocked"));
    const JointDrive d = h->servoJoint->angularXDrive;
    REQUIRE(d.positionSpring > 0.0f);
    // a constant force on the tank's centre of mass across the hinge axis (SYNTHETIC load); its moment about the axis turns the joint
    physics::ServoBodySpec spec;
    REQUIRE(ServoBodySpecOf(*c.root, spec));
    const physics::PartPhysicsState rs = c.vp->PartState(*c.root);
    const Vector3d n = (rs.rotation * spec.axis).normalized();
    const Vector3d pivot = rs.position + rs.rotation * spec.anchor;
    const Vector3d arm = c.vp->PartState(*c.tank).centerOfMass - pivot;
    const Vector3d dir = Vector3d::Cross(n, arm).normalized();
    const double tau = std::min(0.5 * static_cast<double>(d.maximumForce), 0.02 * static_cast<double>(d.positionSpring));  // in the linear range
    const double F = tau / (arm - n * Vector3d::Dot(arm, n)).magnitude();
    const double lock = h->driveTargetAngle;
    for (int t = 0; t < 1500; ++t) {
        c.vp->AddForceAtPhysicsPoint(*c.tank, dir * F, c.vp->PartState(*c.tank).centerOfMass);
        c.Tick();
    }
    const double offset = c.vp->ServoCoordinate(*c.root) - lock * kPi / 180.0;
    MESSAGE("locked: tau " << tau << " kN m, k " << d.positionSpring << ", offset " << offset << " rad (tau / k " << tau / d.positionSpring << ")");
    CHECK(h->Get<bool>("servoIsLocked"));
    CHECK(offset == doctest::Approx(tau / static_cast<double>(d.positionSpring)).epsilon(0.02));
    // the structural loads at rest: the tank's attach joint (to the servo body) carries the load, the servo joint the same force and the
    // load's moment about the anchor, the root's anchor nothing it does not see (WP-20c's PartJoint capacity rule on the tank's joint)
    const auto& joints = c.vp->Joints();
    int tankJoint = -1;
    for (size_t j = 0; j < joints.size(); ++j)
        if (joints[j].child == c.tank) tankJoint = static_cast<int>(j);
    REQUIRE(tankJoint >= 0);
    const physics::VesselPhysics::JointLoad& L = c.vp->JointLoads()[static_cast<size_t>(tankJoint)];
    REQUIRE(L.valid);
    const physics::VesselPhysics::ServoLoad SL = c.vp->ServoJointLoad(*c.root);
    REQUIRE(SL.valid);
    MESSAGE("tank joint |F| " << L.force.magnitude() << " kN, servo joint |F| " << SL.force.magnitude() << " kN, load " << F << " kN");
    CHECK(L.force.magnitude() == doctest::Approx(F).epsilon(0.02));
    CHECK(SL.force.magnitude() == doctest::Approx(F).epsilon(0.02));
    // the servo joint's force on the part points along the load (what the moving side pushes into the base)
    CHECK(Vector3d::Dot(SL.force.normalized(), dir) > 0.99);
    CHECK(L.ratio > 0.0f);
    c.Done();
}

TEST_CASE("WP-52b determinism: the hinge closed loop is bit-identical at 1 and 4 Jolt threads") {
    if (!wp20b::GameAvailable()) return;
    uint64_t hashes[2] = {0, 0};
    double coords[2] = {0, 0};
    for (int k = 0; k < 2; ++k) {
        Scene s;
        Craft c;
        auto* h = c.Make<ModuleRoboticServoHinge>(s, "ModuleRoboticServoHinge", [](ModuleRoboticServoHinge& x) {
            x.Set<Vector2f>("softMinMaxAngles", Vector2f(-90.0f, 90.0f));
            x.Set<Vector2f>("traverseVelocityLimits", Vector2f(0.1f, 180.0f));
            x.Set<float>("traverseVelocity", 20.0f);
        });
        c.Bind(false, k == 0 ? 1 : 4);
        h->Fields()["targetAngle"]->SetValue(ModuleValue(-60.0f));
        for (int t = 0; t < 300; ++t) c.Tick();
        hashes[k] = c.world->StateHash();
        coords[k] = c.vp->ServoCoordinate(*c.root);
        c.Done();
    }
    CHECK(hashes[0] == hashes[1]);
    CHECK(coords[0] == coords[1]);
}

TEST_CASE("WP-52b in the physics bubble: FlightIntegrator sees servoRb (angular drag mirrored, integrationAccel on the servo body), the hinge holds in orbit") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w;
    SceneScope flight{GameScenes::FLIGHT};
    EnvReset reset;
    MultiHost host;
    wp20d::StartPhysicsGlobals(w);
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", true);
    physics::PhysicsBubbleOptions opts;
    UseServoBackend(opts.vessel);  // the bubble's bindings get the servo bodies and joints
    wp19b::BubbleScene s(w, opts);
    s.tw.current_rate_index = 0;
    auto v = w.CraftHead("PhysProf-S", 2);
    Part* root = v->rootPart;
    Charge(*root);
    host.Add(v.get());
    auto* h = AddServo<ModuleRoboticServoHinge>(*root, "ModuleRoboticServoHinge", 0.1);
    h->Set<std::string>("servoAttachNodes", "bottom");
    h->Set<Vector2f>("softMinMaxAngles", Vector2f(-90.0f, 90.0f));
    WireRig(*h);
    h->OnStart(StartState::None);
    h->OnStartBeforePartAttachJoint(StartState::None);
    CelestialBody& kerbin = w.Kerbin();
    Vector3d pos, vel;
    wp19b::CircularStateAt(kerbin, kerbin.Radius + 120000.0, Vector3d(0.6, 0.48, -0.64), Vector3d(0.3, -0.9, 0.2), pos, vel);
    wp19b::PlaceOnOrbit(*v, kerbin, pos, vel, true);
    s.Start(*v);
    double worstCoord = 0.0, worstLoad = 0.0;
    int validLoads = 0;
    physics::VesselPhysics* vp = nullptr;
    for (int t = 0; t < 150; ++t) {
        h->FixedUpdate();  // the servo's FixedUpdate in the module slot (the host's wiring)
        s.Tick();
        h->Update();
        vp = s.bubble.Host().Find(*v);
        if (vp == nullptr || t < 20) continue;
        worstCoord = std::max(worstCoord, std::fabs(vp->ServoCoordinate(*root)));
        const physics::VesselPhysics::ServoLoad L = vp->ServoJointLoad(*root);
        if (L.valid) {
            ++validLoads;
            worstLoad = std::max(worstLoad, L.force.magnitude());
        }
    }
    REQUIRE(vp != nullptr);
    CHECK(vp->PartHasServoBody(*root));
    CHECK(vp->ClusterCount() == 2);
    // FlightIntegrator.UpdateAerodynamics (FlightIntegrator.cs:2276-2281 / :2366-2369): servoRb.angularDrag = rb.angularDrag (no longer
    // the Rigidbody default 0.05 once the integrator has set it)
    CHECK(vp->ServoAngularDrag(*root) == vp->AngularDrag(*root));
    CHECK(vp->AngularDrag(*root) != 0.05f);
    // FlightIntegrator.Integrate (FlightIntegrator.cs:728-731): servoRb.AddForce(integrationAccel, Acceleration): the servo body falls with
    // the rest, so the joint carries no gravity in orbit (without it, servoRb.mass * g = ~2 kN) and the hinge stays at its angle
    const double g = kerbin.gravParameter / ((kerbin.Radius + 120000.0) * (kerbin.Radius + 120000.0));
    MESSAGE("orbit: worst servo coordinate " << worstCoord << " rad, worst servo joint force " << worstLoad << " kN (servoRb.mass g = "
                                              << vp->ServoBodyMass(*root) * g << " kN)");
    CHECK(validLoads > 100);  // the load solve ran every tick
    CHECK(worstCoord < 1e-5);
    CHECK(worstLoad < 0.01 * vp->ServoBodyMass(*root) * g);
    h->OnDestroy();
    host.Remove(v.get());
}
