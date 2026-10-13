#include "scenarios.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <stdexcept>

#include "ksp/config/config_node.hpp"
#include "ksp/save/upgrade_pipeline.hpp"
#include "ksp_context.hpp"

namespace wspike {

using namespace JPH;
namespace fs = std::filesystem;

// ---- args / craft --------------------------------------------------------------------------------------------------------
Args::Args(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        std::string a = argv[i];
        if (a.rfind("--", 0) == 0) {
            const std::string k = a.substr(2);
            if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) opts[k] = argv[++i];
            else opts[k] = "1";
        } else {
            positional.push_back(a);
        }
    }
}
std::string Args::Str(const std::string& k, const std::string& def) const {
    auto it = opts.find(k);
    return it == opts.end() ? def : it->second;
}
double Args::Num(const std::string& k, double def) const {
    auto it = opts.find(k);
    return it == opts.end() ? def : std::stod(it->second);
}

std::string CraftPath(const std::string& n) {
    static const std::map<std::string, std::string> alias = {
        {"bug-e", "SPH/Bug-E Buggy.craft"},          {"prospector", "SPH/Prospector Rover.craft"},
        {"crawler", "SPH/Crater Crawler.craft"},     {"twostage", "VAB/Two-Stage Lander.craft"},
        {"munsplorer", "VAB/PT Series Munsplorer.craft"}, {"skycrane", "VAB/Rover + Skycrane.craft"},
    };
    auto it = alias.find(n);
    if (it != alias.end()) return std::string(KSPCORE_HARNESS_DIR) + "/inputs/Ships/" + it->second;
    return n;
}

namespace {
std::map<const spike::CraftModel*, Vec3> g_craftUp;
}

Vec3 CraftUpOf(const spike::CraftModel& m) {
    auto it = g_craftUp.find(&m);
    return it == g_craftUp.end() ? Vec3::sAxisY() : it->second;
}

std::unique_ptr<spike::CraftModel> LoadCraft(spike::KspContext& ctx, const std::string& name) {
    const std::string path = CraftPath(name);
    ksp::ConfigNode::NodePtr root = ksp::ConfigNode::Load(path);
    if (!root) throw std::runtime_error("cannot read " + path);
    ksp::ConfigNode::NodePtr upgraded;
    const std::string* sn = root->GetValuePtr("ship");
    ksp::KSPUpgradePipeline::Process(root, sn ? *sn : std::string(), ksp::SaveUpgradePipeline::LoadContext::Craft,
                                     [&](const ksp::ConfigNode::NodePtr& n) { upgraded = n; },
                                     [&](ksp::KSPUpgradePipeline::UpgradeFailOption, const ksp::ConfigNode::NodePtr&) {});
    if (!upgraded) throw std::runtime_error("upgrade pipeline failed for " + path);
    fs::create_directories(WHEEL_SPIKE_TMP_DIR);
    std::string safe = name;
    for (char& ch : safe)
        if (!std::isalnum(static_cast<unsigned char>(ch))) ch = '_';
    const std::string tmp = std::string(WHEEL_SPIKE_TMP_DIR) + "/" + safe + ".craft";
    if (!upgraded->Save(tmp)) throw std::runtime_error("cannot write " + tmp);
    auto m = std::make_unique<spike::CraftModel>(spike::BuildCraftModel(ctx, tmp, name));
    // the editor's up in the assembled (root-relative) frame: inverse(root part's file rotation) * Y
    Vec3 up = Vec3::sAxisY();
    if (ksp::ConfigNode* part = upgraded->nodes().GetNode("PART"))
        if (const std::string* rs = part->GetValuePtr("rot")) {
            float q[4] = {0, 0, 0, 1};
            std::stringstream ss(*rs);
            std::string tok;
            for (int k = 0; k < 4 && std::getline(ss, tok, ','); k++) q[k] = std::stof(tok);
            const Quat qr = Quat(q[0], q[1], q[2], q[3]).Normalized();
            up = (qr.Conjugated() * Vec3::sAxisY()).Normalized();
        }
    g_craftUp[m.get()] = up;
    return m;
}

namespace {

bool Bad(const Vec3& v) { return !std::isfinite(v.GetX()) || !std::isfinite(v.GetY()) || !std::isfinite(v.GetZ()) || v.Length() > 1000.0f; }
double Deg(double r) { return r * 180.0 / 3.14159265358979; }

// ---- the C# motor / brake models without tyre slip, aero or suspension (the "analytic" references) ----------------------
struct MotorWheelRef {
    ksp::FloatCurve curve;
    float rCfg = 0, rTrue = 0, driveResponse = 0, tcs = 1, maxBrake = 0, brakeResponse = 0, brakeTweakable = 50;
    bool autoTorque = true, brakes = false, motor = false;
    float driveState = 0, brakeState = 0;
};

std::vector<MotorWheelRef> MotorRefs(const Rover& rv) {
    std::vector<MotorWheelRef> out;
    for (int i = 0; i < rv.WheelCount(); i++) {
        const KspWheel& w = rv.Wheel(i);
        MotorWheelRef r;
        r.motor = w.cfg.hasMotor && w.cfg.motorEnabled;
        r.curve = w.cfg.torqueCurve;
        r.rCfg = w.cfg.radius;
        r.rTrue = w.wcRadius;
        r.driveResponse = w.cfg.driveResponse;
        r.tcs = w.cfg.tractionControlScale;
        r.autoTorque = w.cfg.autoTorque;
        r.brakes = w.cfg.hasBrakes;
        r.maxBrake = w.cfg.maxBrakeTorque;
        r.brakeResponse = w.cfg.brakeResponse;
        r.brakeTweakable = w.cfg.brakeTweakable;
        out.push_back(r);
    }
    return out;
}

// one tick of the reference: returns the propulsive (+) / braking (-) force (kN) at speed v
double RefForce(std::vector<MotorWheelRef>& refs, double v, float throttle, float brake, float dt, double geeASL) {
    double f = 0.0;
    for (MotorWheelRef& r : refs) {
        const float w = static_cast<float>(v) / r.rTrue;  // rolling without slip
        if (r.motor) {
            float maxT = r.curve.Evaluate(std::fabs(w * r.rCfg));
            maxT = static_cast<float>(std::nearbyint(static_cast<double>(maxT * 100.0f))) / 100.0f;
            float di = throttle;
            if (r.autoTorque) {
                const float slipDispY = (w * r.rCfg - static_cast<float>(v)) * dt;  // ModuleWheelBase.slipDisplacement.y
                di *= 1.0f - std::fabs(slipDispY * r.tcs);
                di *= mathf::Lerp(1.0f, static_cast<float>(geeASL), mathf::Clamp01(r.tcs / 2.0f));
            }
            r.driveState = mathf::MoveTowards(r.driveState, di, r.driveResponse * dt);
            f += static_cast<double>(r.driveState * maxT) / r.rTrue;
        }
        if (r.brakes) {
            const float num = std::min(1.0f, mathf::Lerp(1.0f, static_cast<float>(geeASL), std::fabs(w)));
            const float bi = brake * r.brakeTweakable * 0.01f * num;
            r.brakeState = mathf::MoveTowards(r.brakeState, bi, (bi != 0.0f ? r.brakeResponse : 100.0f) * dt);
            const float tb = r.brakeState >= 1.0f ? r.brakeState * 1000.0f : r.brakeState * r.maxBrake;
            if (v > 1e-4) f -= static_cast<double>(tb) / r.rTrue;
        }
    }
    return f;
}

// The vessel mass plus the wheels' rotational inertia seen at the contact (sum of I / r^2, VPP Wheel.RecalculateConstants:
// I = max(0.01, m r^2 / 2): Solver.minInertia floors the small stock wheels' inertia, Wheel.cs:76-84)
double EffectiveMass(const Rover& rv) {
    double m = rv.Mass();
    for (int i = 0; i < rv.WheelCount(); i++) {
        const KspWheel& w = rv.Wheel(i);
        m += static_cast<double>(w.block.I) / (static_cast<double>(w.block.radius) * w.block.radius);
    }
    return m;
}

double SlopeAt(const Rover& rv) {
    for (int i = 0; i < rv.WheelCount(); i++)
        if (rv.Wheel(i).hit.grounded) {
            const Vec3 n = rv.Wheel(i).hit.normal;
            const DVec3 up = rv.Up();
            return Deg(std::acos(std::clamp(static_cast<double>(n.GetX()) * up.GetX() + n.GetY() * up.GetY() + n.GetZ() * up.GetZ(), -1.0, 1.0)));
        }
    return 0.0;
}

}  // namespace

void PrintCraftInfo(spike::KspContext& ctx, spike::CraftModel& craft, const RoverConfig& cfg) {
    Rover rv(ctx, craft, cfg);
    std::printf("%s: parts %zu (physical %d), vessel mass %.3f t, rb mass sum %.3f t, wheels %d, spawn terrain height %.2f m, "
                "patch heights %.2f..%.2f m\n",
                craft.name.c_str(), craft.parts.size(), craft.physicalCount, craft.totalMass, rv.Mass(), rv.WheelCount(),
                rv.SpawnTerrainHeight(), rv.TerrainMinHeight(), rv.TerrainMaxHeight());
    // lowest collider proxies (drag-cube boxes) along the craft's up axis, relative to the CoM
    const Vec3 up = CraftUpOf(craft);
    auto P = [](const spike::PartSpec& s) {
        return Vec3(static_cast<float>(s.pos.GetX()), static_cast<float>(s.pos.GetY()), static_cast<float>(s.pos.GetZ()));
    };
    Vec3 com = Vec3::sZero();
    {
        double m = 0;
        for (const spike::PartSpec& s : craft.parts)
            if (s.physical) {
                com += (P(s) + s.rot * s.comLocal) * s.rbMass;
                m += s.rbMass;
            }
        com = com / static_cast<float>(m);
        std::vector<std::pair<float, std::string>> low;
        for (const spike::PartSpec& s : craft.parts) {
            float mn = 1e30f;
            for (int k = 0; k < 8; k++) {
                const Vec3 corner = s.boxCenter + Vec3((k & 1) ? s.boxHalf.GetX() : -s.boxHalf.GetX(), (k & 2) ? s.boxHalf.GetY() : -s.boxHalf.GetY(),
                                                       (k & 4) ? s.boxHalf.GetZ() : -s.boxHalf.GetZ());
                const Vec3 p = Vec3(static_cast<float>(s.pos.GetX()), static_cast<float>(s.pos.GetY()), static_cast<float>(s.pos.GetZ())) +
                               s.rot * corner;
                mn = std::min(mn, (p - com).Dot(up));
            }
            low.push_back({mn, s.name});
        }
        std::sort(low.begin(), low.end());
        std::printf("  lowest drag-cube boxes below the CoM (m):");
        for (size_t k = 0; k < std::min<size_t>(6, low.size()); k++) std::printf(" %s %.3f,", low[k].second.c_str(), low[k].first);
        std::printf("\n");
    }
    for (int i = 0; i < rv.WheelCount(); i++) {
        const KspWheel& w = rv.Wheel(i);
        const WheelGeom& g = rv.Geom(i);
        const WheelConfig& c = w.cfg;
        const Vec3 cl = w.UnityCenterLocal();
        const float foot = (g.transformLocal + g.upLocal * cl.GetY()).Dot(g.upLocal) - w.suspensionDistance - w.wcRadius;
        std::printf("  [%d] tyre bottom at full extension %.3f m below the CoM (part origin %.3f below)\n", i, -foot,
                    -(P(rv.Craft().parts[g.part]) - com).Dot(up));
        std::printf("  [%d] %-14s type %d rescale %.2f r %.3f (cfg %.3f) D %.3f anchor %.2f springRatio %.0f damperRatio %.1f distrib %d "
                    "maxLoad %.0f | motor %d T %.2f resp %.1f | brakes %d T %.2f resp %.1f tweak %.0f | lock %d %.0f | minDown %.1f "
                    "fm-> adh %.2f peak %.2f lim %.2f | rbMass %.3f | transform (body) %.2f %.2f %.2f\n",
                    i, c.partName.c_str(), c.wheelType, c.rescaleFactor, w.wcRadius, c.radius, w.suspensionDistance, w.suspensionAnchor,
                    c.springRatio, c.damperRatio, c.useDistributedMass ? 1 : 0, c.maximumLoad, c.hasMotor ? 1 : 0, w.motorMaxTorque,
                    c.driveResponse, c.hasBrakes ? 1 : 0, w.maxBrakeTorque, w.brakeResponse, c.brakeTweakable, c.hasLock ? 1 : 0,
                    c.lockTorque, w.minimumdownForce, w.tire.adherent.y, w.tire.peak.y, w.tire.limit.y, g.partRbMass,
                    g.transformLocal.GetX(), g.transformLocal.GetY(), g.transformLocal.GetZ());
    }
}

// ---- rest -------------------------------------------------------------------------------------------------------------------
RestResult RunRest(spike::KspContext& ctx, spike::CraftModel& craft, RoverConfig cfg, double seconds, bool brakes, double warp,
                   double window0) {
    cfg.dt = 0.02 * warp;
    Rover rv(ctx, craft, cfg);
    RestResult r;
    r.craft = craft.name;
    r.native = NativeName(cfg.native);
    r.warp = warp;
    r.seconds = seconds;
    r.brakes = brakes;
    r.wheels = rv.WheelCount();
    const int ticks = static_cast<int>(std::lround(seconds / cfg.dt));
    const int w0 = static_cast<int>(std::lround(window0 / cfg.dt));
    const DVec3 up = rv.Up();
    const DVec3 spawn = rv.Com();
    DVec3 start = spawn;
    double sumVz2 = 0.0, tsum = 0.0;
    int n = 0;
    Controls c;
    c.brake = brakes ? 1.0f : 0.0f;
    for (int t = 0; t < ticks; t++) {
        rv.Tick(c);
        tsum += rv.LastTimes().total;
        const Vec3 v = rv.Velocity();
        if (Bad(v)) {
            r.exploded = true;
            break;
        }
        if (rv.BottomedCount() > 0) r.bottomedTicks++;
        if (t == w0) start = rv.Com();
        if (t > w0) {
            const double vz = static_cast<double>(v.GetX()) * up.GetX() + v.GetY() * up.GetY() + v.GetZ() * up.GetZ();
            sumVz2 += vz * vz;
            n++;
            r.maxSpeed = std::max(r.maxSpeed, static_cast<double>(v.Length()));
            r.maxAngVel = std::max(r.maxAngVel, static_cast<double>(rv.AngularVelocity().Length()));
            r.bodyTouch = r.bodyTouch || rv.BodyTouchesGround();
        }
    }
    const DVec3 d = rv.Com() - start;
    r.vertDrift = d.Dot(up);
    r.horizDrift = (d - up * r.vertDrift).Length();
    r.settleDrop = (spawn - start).Dot(up);
    r.rmsVertSpeed = n > 0 ? std::sqrt(sumVz2 / n) : 0.0;
    for (int i = 0; i < rv.WheelCount(); i++) {
        const KspWheel& w = rv.Wheel(i);
        const double f = w.hit.grounded ? w.hit.compression / w.nativeSpring.suspensionDistance : 0.0;
        r.meanCompression += f / std::max(1, rv.WheelCount());
        r.maxCompression = std::max(r.maxCompression, f);
        r.grounded += w.hit.grounded ? 1 : 0;
    }
    r.slopeDeg = SlopeAt(rv);
    r.tickMs = tsum / std::max(1, ticks);
    return r;
}

// ---- drive (motor torque on flat ground) ---------------------------------------------------------------------------------
DriveResult RunDrive(spike::KspContext& ctx, spike::CraftModel& craft, RoverConfig cfg, double seconds) {
    Rover rv(ctx, craft, cfg);
    DriveResult r;
    r.craft = craft.name;
    r.native = NativeName(cfg.native);
    r.friction = cfg.friction == TireFriction::Model::Smooth ? "smooth" : "lineal";
    r.mass = rv.Mass();
    r.massEff = EffectiveMass(rv);
    auto refs = MotorRefs(rv);
    for (const auto& m : refs) r.motors += m.motor ? 1 : 0;
    // analytic top speed: the torque curve's last key (value 0), converted from the module's (unscaled) radius
    for (const auto& m : refs)
        if (m.motor) r.vTopAnalytic = m.curve.maxTime() * m.rTrue / m.rCfg;
    const float dt = static_cast<float>(cfg.dt);
    const double settle = 2.0;
    const int ticks = static_cast<int>(std::lround((settle + seconds) / cfg.dt));
    double va = 0.0, xa = 0.0;
    DVec3 x0 = rv.Com(), f0 = rv.Forward();
    double tsum = 0.0;
    int nt = 0;
    const double marks[4] = {10.0, 20.0, 30.0, 40.0};
    for (int t = 0; t < ticks; t++) {
        const double time = t * cfg.dt;
        const bool on = time >= settle;
        if (t == static_cast<int>(std::lround(settle / cfg.dt))) {
            x0 = rv.Com();
            f0 = rv.Forward();
        }
        Controls c;
        c.wheelThrottle = on ? 1.0f : 0.0f;
        rv.Tick(c);
        tsum += rv.LastTimes().total;
        nt++;
        const Vec3 v = rv.Velocity();
        if (Bad(v)) {
            r.exploded = true;
            break;
        }
        if (!on) continue;
        const double ts = time + cfg.dt - settle;  // time since throttle-up
        const DVec3 fw = rv.Forward();
        const double vf = static_cast<double>(v.GetX()) * fw.GetX() + v.GetY() * fw.GetY() + v.GetZ() * fw.GetZ();
        const double xf = (rv.Com() - x0).Dot(f0);
        // reference: same tick, same ramp, no slip (m dv/dt = sum T / r)
        const double fa = RefForce(refs, va, 1.0f, 0.0f, dt, rv.Config().gravityScale);
        va += fa / r.massEff * cfg.dt;
        xa += va * cfg.dt;
        if (r.t5 < 0 && vf >= 5.0) r.t5 = ts;
        if (r.t5a < 0 && va >= 5.0) r.t5a = ts;
        if (r.t10 < 0 && vf >= 10.0) r.t10 = ts;
        if (r.t10a < 0 && va >= 10.0) r.t10a = ts;
        for (int k = 0; k < 4; k++)
            if (std::fabs(ts - marks[k]) < 0.5 * cfg.dt) {
                r.vAt[k] = vf;
                r.vAtA[k] = va;
            }
        r.vMax = std::max(r.vMax, vf);
        r.vMaxA = std::max(r.vMaxA, va);
        for (int i = 0; i < rv.WheelCount(); i++) r.maxSlip = std::max(r.maxSlip, static_cast<double>(std::fabs(rv.Wheel(i).tireSlip.y)));
        r.maxPitchDeg = std::max(r.maxPitchDeg, rv.TiltDeg());
        if (t % static_cast<int>(std::lround(0.5 / cfg.dt)) == 0) r.samples.push_back({ts, vf, va, xf, xa});
    }
    r.tickMs = tsum / std::max(1, nt);
    return r;
}

// ---- brake --------------------------------------------------------------------------------------------------------------------
BrakeResult RunBrake(spike::KspContext& ctx, spike::CraftModel& craft, RoverConfig cfg, double v0) {
    Rover rv(ctx, craft, cfg);
    BrakeResult r;
    r.craft = craft.name;
    r.native = NativeName(cfg.native);
    r.friction = cfg.friction == TireFriction::Model::Smooth ? "smooth" : "lineal";
    const float dt = static_cast<float>(cfg.dt);
    auto vf = [&]() {
        const Vec3 v = rv.Velocity();
        const DVec3 fw = rv.Forward();
        return static_cast<double>(v.GetX()) * fw.GetX() + v.GetY() * fw.GetY() + v.GetZ() * fw.GetZ();
    };
    // accelerate (after a 2 s settle) until v0 or 60 s
    int t = 0;
    const int settle = static_cast<int>(std::lround(2.0 / cfg.dt));
    for (; t < settle + static_cast<int>(60.0 / cfg.dt); t++) {
        Controls c;
        c.wheelThrottle = t >= settle ? 1.0f : 0.0f;
        rv.Tick(c);
        if (Bad(rv.Velocity())) {
            r.exploded = true;
            return r;
        }
        if (t >= settle && vf() >= v0) break;
    }
    r.v0 = vf();
    const DVec3 x0 = rv.Com(), f0 = rv.Forward();
    double vPrev = r.v0;
    for (int k = 0; k < static_cast<int>(60.0 / cfg.dt); k++) {
        Controls c;
        c.brake = 1.0f;
        rv.Tick(c);
        if (Bad(rv.Velocity())) {
            r.exploded = true;
            return r;
        }
        const Vec3 vv = rv.Velocity();
        const double v = static_cast<double>(vv.GetX()) * f0.GetX() + vv.GetY() * f0.GetY() + vv.GetZ() * f0.GetZ();  // fixed axis
        if (v > 1.0) r.maxDecel = std::max(r.maxDecel, (vPrev - v) / cfg.dt);  // excluding the final stop transient
        r.maxTiltDeg = std::max(r.maxTiltDeg, rv.TiltDeg());
        vPrev = v;
        if (v < 0.05) {
            r.time = (k + 1) * cfg.dt;
            r.distance = (rv.Com() - x0).Dot(f0);
            break;
        }
    }
    // reference: KSP's brake torque (ramp at brakeResponse, then maxBrakeTorqueRest once brakeState >= 1) through rolling wheels,
    // capped by the tyre curve of locked wheels sliding at slip = v (VPP: force = curve(|slip|) * load, sum of loads = m g; no
    // load transfer, no suspension)
    auto refs = MotorRefs(rv);
    const TireFriction& tf = rv.Wheel(0).tire;
    const double g = rv.Config().gravityScale * 9.81;
    double va = r.v0, xa = 0.0;
    for (int k = 0; k < static_cast<int>(60.0 / cfg.dt) && va > 0.05; k++) {
        const double fa = -RefForce(refs, va, 0.0f, 1.0f, dt, rv.Config().gravityScale);  // brake force (kN, positive)
        const double fSlide = tf.EvaluateForce(static_cast<float>(va), 1.0f, 1.0f) * rv.Mass() * g;
        if (fa < fSlide) va -= fa / EffectiveMass(rv) * cfg.dt;  // rolling: the wheels' spin inertia is braked too
        else va -= fSlide / rv.Mass() * cfg.dt;                  // locked: sliding on the tyre curve
        xa += std::max(0.0, va) * cfg.dt;
        r.timeA = (k + 1) * cfg.dt;
    }
    r.distanceA = xa;
    // reference: wheels locked from the first tick, sliding with the tyre curve at slip = v
    double vl = r.v0, xl = 0.0;
    for (int k = 0; k < 6000 && vl > 0.05; k++) {
        vl -= tf.EvaluateForce(static_cast<float>(vl), 1.0f, 1.0f) * g * cfg.dt;
        xl += std::max(0.0, vl) * cfg.dt;
    }
    r.distanceLocked = xl;
    return r;
}

// ---- touchdown ----------------------------------------------------------------------------------------------------------------
TouchdownResult RunTouchdown(spike::KspContext& ctx, spike::CraftModel& craft, RoverConfig cfg, double v0, double seconds) {
    cfg.spawnClearance = 0.02f;
    cfg.spawnVerticalSpeed = -v0;
    cfg.gravityEase = false;  // an unpacked vessel in flight: no easing
    Rover rv(ctx, craft, cfg);
    TouchdownResult r;
    r.craft = craft.name;
    r.native = NativeName(cfg.native);
    r.ground = cfg.ground == Ground::PQS ? "PQS" : "flat";
    r.gScale = cfg.gravityScale;
    r.v0 = v0;
    const DVec3 up = rv.Up();
    const int ticks = static_cast<int>(std::lround(seconds / cfg.dt));
    int first = -1, calm = 0, airborneRun = 0;
    double altFirst = 0.0, altMin = 1e30;
    bool pastMin = false;
    for (int t = 0; t < ticks; t++) {
        rv.Tick(Controls{});
        const Vec3 v = rv.Velocity();
        if (Bad(v)) {
            r.exploded = true;
            break;
        }
        const double vz = static_cast<double>(v.GetX()) * up.GetX() + v.GetY() * up.GetY() + v.GetZ() * up.GetZ();
        const double alt = rv.Altitude();
        const int gnd = rv.GroundedCount();
        if (rv.BottomedCount() > 0) r.bottomedTicks++;
        r.bodyTouch = r.bodyTouch || rv.BodyTouchesGround();
        for (int i = 0; i < rv.WheelCount(); i++) {
            const KspWheel& w = rv.Wheel(i);
            if (!w.hit.grounded) continue;
            r.maxCompressionM = std::max(r.maxCompressionM, static_cast<double>(w.hit.compression));
            r.maxCompression = std::max(r.maxCompression, static_cast<double>(w.hit.compression / w.nativeSpring.suspensionDistance));
        }
        if (first < 0 && gnd > 0) {
            first = t;
            altFirst = alt;
        }
        if (first < 0) continue;
        if (alt < altMin) altMin = alt;
        else if (vz > 0.0) pastMin = true;
        if (pastMin) r.reboundHeight = std::max(r.reboundHeight, alt - altFirst);
        r.maxUpSpeed = std::max(r.maxUpSpeed, vz);
        if (gnd == 0) {
            r.airborneTicksAfterContact++;
            airborneRun++;
            if (airborneRun >= 3 && vz > 0.3) r.bounceLaunch = true;
        } else {
            airborneRun = 0;
        }
        if (v.Length() < 0.05f) {
            if (++calm >= static_cast<int>(1.0 / cfg.dt) && r.settleTime < 0) r.settleTime = (t + 1 - calm) * cfg.dt;
        } else {
            calm = 0;
        }
    }
    r.minClearance = altMin - altFirst;  // CoM travel below the first-contact height (m, negative)
    for (int i = 0; i < rv.WheelCount(); i++) {
        r.maxImpactRatio = std::max(r.maxImpactRatio, static_cast<double>(rv.Wheel(i).maxImpact));
        r.maxStressRatio = std::max(r.maxStressRatio, static_cast<double>(rv.Wheel(i).maxStressRatio));
    }
    return r;
}

// ---- benches --------------------------------------------------------------------------------------------------------------
namespace {

struct Out {
    std::string name;
    std::ostringstream md, csv;
    explicit Out(std::string n) : name(std::move(n)) {}
    void Write() {
        fs::create_directories(WHEEL_SPIKE_RESULTS_DIR);
        std::ofstream(std::string(WHEEL_SPIKE_RESULTS_DIR) + "/" + name + ".md") << md.str();
        std::ofstream(std::string(WHEEL_SPIKE_RESULTS_DIR) + "/" + name + ".csv") << csv.str();
        std::fputs(md.str().c_str(), stdout);
    }
};

std::string F(double v, int p = 3) {
    char b[64];
    std::snprintf(b, sizeof b, "%.*f", p, v);
    return b;
}

std::vector<std::string> Crafts(const Args& a, const std::vector<std::string>& def) {
    if (!a.Has("crafts")) return def;
    std::vector<std::string> out;
    std::stringstream ss(a.Str("crafts", ""));
    std::string s;
    while (std::getline(ss, s, ',')) out.push_back(s);
    return out;
}
std::vector<NativeModel> Natives(const Args& a) {
    const std::string s = a.Str("native", "both");
    if (s == "explicit") return {NativeModel::Explicit};
    if (s == "jolt-vc") return {NativeModel::JoltVehicle};
    return {NativeModel::Explicit, NativeModel::JoltVehicle};
}
RoverConfig BaseConfig(const Args& a) {
    RoverConfig c;
    c.ground = a.Str("ground", "pqs") == "flat" ? Ground::Flat : Ground::PQS;
    c.friction = a.Str("friction", "smooth") == "lineal" ? TireFriction::Model::Lineal : TireFriction::Model::Smooth;
    c.headingDeg = a.Num("heading", 90.0);
    c.startOffsetEast = a.Num("east", 0.0);
    c.startOffsetNorth = a.Num("north", 0.0);
    c.inertiaFloor = !a.Has("no-inertia-floor");
    c.legGroundOffset = a.Has("leg-offset");
    c.gravityScale = a.Num("g", 1.0);
    return c;
}

int BenchRest(spike::KspContext& ctx, const Args& a) {
    Out o(a.Str("out", "rest"));
    const double secs = a.Num("seconds", 60.0);
    o.md << "# rest: pad rest stability, " << secs << " s, CoM drift measured from t = 5 s (warp 1x / 4x = dt 0.02 / 0.08)\n\n";
    o.md << "| craft | native | warp | brakes | horiz drift (m) | vert drift (m) | max speed (m/s) | rms vert speed (m/s) | max ang vel (rad/s) | "
            "settle drop (m) | compression mean / max | grounded | bottomed ticks | body touch | slope (deg) | tick (ms) |\n";
    o.md << "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";
    o.csv << "craft,native,warp,brakes,horiz_drift,vert_drift,max_speed,rms_vz,max_angvel,settle_drop,comp_mean,comp_max,grounded,wheels,bottomed_ticks,body_touch,slope_deg,tick_ms,exploded\n";
    std::vector<double> warps;
    {
        std::stringstream ss(a.Str("warps", "1,4"));
        std::string s;
        while (std::getline(ss, s, ',')) warps.push_back(std::stod(s));
    }
    for (const std::string& cn : Crafts(a, {"bug-e", "prospector", "twostage"})) {
        auto craft = LoadCraft(ctx, cn);
        for (NativeModel nm : Natives(a))
            for (double warp : warps)
                for (bool brakes : {true, false}) {
                    RoverConfig cfg = BaseConfig(a);
                    cfg.native = nm;
                    const RestResult r = RunRest(ctx, *craft, cfg, secs, brakes, warp);
                    o.md << "| " << r.craft << " | " << r.native << " | " << r.warp << "x | " << (brakes ? "on" : "off") << " | "
                         << F(r.horizDrift, 4) << " | " << F(r.vertDrift, 4) << " | " << F(r.maxSpeed, 4) << " | " << F(r.rmsVertSpeed, 5)
                         << " | " << F(r.maxAngVel, 4) << " | " << F(r.settleDrop, 3) << " | " << F(r.meanCompression, 2) << " / "
                         << F(r.maxCompression, 2) << " | " << r.grounded << "/" << r.wheels << " | " << r.bottomedTicks << " | "
                         << (r.bodyTouch ? "yes" : "no") << " | " << F(r.slopeDeg, 2) << " | " << F(r.tickMs, 4)
                         << (r.exploded ? " | EXPLODED" : "") << " |\n";
                    o.csv << r.craft << "," << r.native << "," << r.warp << "," << brakes << "," << r.horizDrift << "," << r.vertDrift << ","
                          << r.maxSpeed << "," << r.rmsVertSpeed << "," << r.maxAngVel << "," << r.settleDrop << "," << r.meanCompression << ","
                          << r.maxCompression << "," << r.grounded << "," << r.wheels << "," << r.bottomedTicks << "," << r.bodyTouch << ","
                          << r.slopeDeg << "," << r.tickMs << "," << r.exploded << "\n";
                    std::fflush(stdout);
                }
    }
    o.Write();
    return 0;
}

int BenchDrive(spike::KspContext& ctx, const Args& a) {
    Out o(a.Str("out", "drive"));
    const double secs = a.Num("seconds", a.Str("ground", "pqs") == "flat" ? 40.0 : 30.0);
    const std::vector<double> warps = a.Has("warp4") ? std::vector<double>{1.0, 4.0} : std::vector<double>{1.0};
    o.md << "# drive: full wheel throttle on " << a.Str("ground", "pqs") << " ground, " << secs
         << " s after a 2 s settle; reference = the C# motor model without tyre slip (same tick, drive ramp, torque curve, traction control, motors enabled per the craft) accelerating the vessel mass plus the wheels' spin inertia (sum I / r^2, VPP floors I at 0.01)\n\n";
    o.md << "| craft | native | friction | warp | mass / with wheel inertia (t) | motors | t to 5 m/s sim / ref (s) | t to 10 m/s | v at 10 s | v at 20 s | v at 30 s | v at 40 s | "
            "v max sim / ref | ref asymptote | max fwd slip (m/s) | tick (ms) |\n";
    o.md << "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";
    o.csv << "craft,native,friction,warp,t,v,v_ref,x,x_ref\n";
    for (const std::string& cn : Crafts(a, {"bug-e", "prospector"})) {
        auto craft = LoadCraft(ctx, cn);
        for (NativeModel nm : Natives(a))
            for (double warp : warps) {
                RoverConfig cfg = BaseConfig(a);
                cfg.native = nm;
                cfg.dt = 0.02 * warp;
                const DriveResult r = RunDrive(ctx, *craft, cfg, secs);
                o.md << "| " << r.craft << " | " << r.native << " | " << r.friction << " | " << warp << "x | " << F(r.mass, 3) << " / " << F(r.massEff, 3) << " | " << r.motors
                     << " | " << F(r.t5, 2) << " / " << F(r.t5a, 2) << " | " << F(r.t10, 2) << " / " << F(r.t10a, 2);
                for (int k = 0; k < 4; k++) o.md << " | " << F(r.vAt[k], 2) << " / " << F(r.vAtA[k], 2);
                o.md << " | " << F(r.vMax, 2) << " / " << F(r.vMaxA, 2) << " | " << F(r.vTopAnalytic, 2) << " | " << F(r.maxSlip, 2) << " | "
                     << F(r.tickMs, 4) << (r.exploded ? " EXPLODED" : "") << " |\n";
                for (const DriveSample& s : r.samples)
                    o.csv << r.craft << "," << r.native << "," << r.friction << "," << warp << "," << s.t << "," << s.v << "," << s.vAnalytic << ","
                          << s.x << "," << s.xAnalytic << "\n";
                std::fflush(stdout);
            }
    }
    o.Write();
    return 0;
}

int BenchBrake(spike::KspContext& ctx, const Args& a) {
    Out o(a.Str("out", "brake"));
    o.md << "# brake: accelerate to v0, then wheel throttle 0 and the Brakes action group (brakeTweakable from the craft); "
         << a.Str("ground", "pqs") << " ground. Reference = KSP's brake torque (brakeResponse ramp; maxBrakeTorqueRest = 1000 kN m once "
            "brakeState >= 1, i.e. brakeTweakable >= 100 %) through rolling wheels, capped by locked wheels sliding on the tyre curve "
            "(no load transfer); 'locked' = wheels locked from the first tick\n\n";
    o.md << "| craft | native | friction | v0 (m/s) | stop distance sim (m) | stop time sim (s) | ref: distance / time | "
            "ref locked: distance | max decel above 1 m/s (m/s2) | max tilt (deg) |\n";
    o.md << "|---|---|---|---|---|---|---|---|---|---|\n";
    o.csv << "craft,native,friction,v0,distance,time,distance_ref,time_ref,distance_locked,max_decel,max_tilt,exploded\n";
    const std::map<std::string, double> v0s = {{"bug-e", 8.0}, {"prospector", 20.0}, {"crawler", 10.0}};
    for (const std::string& cn : Crafts(a, {"bug-e", "prospector"})) {
        auto craft = LoadCraft(ctx, cn);
        const double v0 = a.Num("v0", v0s.count(cn) ? v0s.at(cn) : 10.0);
        for (NativeModel nm : Natives(a)) {
            RoverConfig cfg = BaseConfig(a);
            cfg.native = nm;
            const BrakeResult r = RunBrake(ctx, *craft, cfg, v0);
            o.md << "| " << r.craft << " | " << r.native << " | " << r.friction << " | " << F(r.v0, 2) << " | " << F(r.distance, 2) << " | "
                 << F(r.time, 2) << " | " << F(r.distanceA, 2) << " / " << F(r.timeA, 2) << " | " << F(r.distanceLocked, 2) << " | "
                 << F(r.maxDecel, 2) << " | " << F(r.maxTiltDeg, 1) << (r.exploded ? " EXPLODED" : "") << " |\n";
            o.csv << r.craft << "," << r.native << "," << r.friction << "," << r.v0 << "," << r.distance << "," << r.time << "," << r.distanceA
                  << "," << r.timeA << "," << r.distanceLocked << "," << r.maxDecel << "," << r.maxTiltDeg << "," << r.exploded << "\n";
            std::fflush(stdout);
        }
    }
    o.Write();
    return 0;
}

int BenchTouchdown(spike::KspContext& ctx, const Args& a) {
    Out o(a.Str("out", "touchdown"));
    const double v0 = a.Num("v0", 3.0);
    o.md << "# touchdown: landing legs deployed, vertical descent at " << v0
         << " m/s onto the ground, engines off, 20 s. Bounce = no leg on the ground after the first contact\n\n";
    o.md << "| craft | native | ground | g | max compression (travel / m) | CoM travel below first contact (m) | max up speed (m/s) | "
            "rebound above first contact (m) | airborne ticks | bounce-launch | settle (s) | bottomed ticks | body touch | "
            "damage: impact / stress (x tolerance) |\n";
    o.md << "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";
    o.csv << "craft,native,ground,g,max_comp,max_comp_m,com_below_contact,max_up,rebound,airborne_ticks,bounce_launch,settle,bottomed,body_touch,impact,stress,exploded\n";
    for (const std::string& cn : Crafts(a, {"twostage"})) {
        auto craft = LoadCraft(ctx, cn);
        for (NativeModel nm : Natives(a))
            for (Ground gr : {Ground::Flat, Ground::PQS})
                for (double gs : {1.0, 0.166}) {
                    RoverConfig cfg = BaseConfig(a);
                    cfg.native = nm;
                    cfg.ground = gr;
                    cfg.gravityScale = gs;
                    const TouchdownResult r = RunTouchdown(ctx, *craft, cfg, v0, a.Num("seconds", 20.0));
                    o.md << "| " << r.craft << " | " << r.native << " | " << r.ground << " | " << (gs == 1.0 ? "Kerbin" : "0.166 (Mun)") << " | "
                         << F(r.maxCompression, 2) << " / " << F(r.maxCompressionM, 3) << " | " << F(r.minClearance, 3) << " | "
                         << F(r.maxUpSpeed, 3) << " | " << F(r.reboundHeight, 3) << " | " << r.airborneTicksAfterContact << " | "
                         << (r.bounceLaunch ? "YES" : "no") << " | " << F(r.settleTime, 2) << " | " << r.bottomedTicks << " | "
                         << (r.bodyTouch ? "yes" : "no") << " | " << F(r.maxImpactRatio, 2) << " / " << F(r.maxStressRatio, 2)
                         << (r.exploded ? " EXPLODED" : "") << " |\n";
                    o.csv << r.craft << "," << r.native << "," << r.ground << "," << gs << "," << r.maxCompression << "," << r.maxCompressionM << ","
                          << r.minClearance << "," << r.maxUpSpeed << "," << r.reboundHeight << "," << r.airborneTicksAfterContact << ","
                          << r.bounceLaunch << "," << r.settleTime << "," << r.bottomedTicks << "," << r.bodyTouch << "," << r.maxImpactRatio
                          << "," << r.maxStressRatio << "," << r.exploded << "\n";
                    std::fflush(stdout);
                }
    }
    o.Write();
    return 0;
}

int BenchCost(spike::KspContext& ctx, const Args& a) {
    Out o(a.Str("out", "cost"));
    const int repeat = static_cast<int>(a.Num("repeat", 3));
    const double secs = a.Num("seconds", 20.0);
    o.md << "# cost: per-tick phase times (ms), mean over " << secs << " s of game time, best of " << repeat
         << " runs; rovers at full throttle, the lander at rest. managed = the ported C# of every wheel (8 VPP substeps: active vessel); "
            "native = rays + PhysX-law forces + limit PGS (explicit) or VehicleConstraint parameter update + readback (jolt-vc); "
            "the jolt-vc rays and spring rows run inside the Jolt step\n\n";
    o.md << "| craft | native | wheels | substeps | precalc | managed | native pre | Jolt step | native post | tick total | managed per wheel (us) | "
            "native per wheel (us) | wheel total per wheel (us) |\n";
    o.md << "|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";
    o.csv << "craft,native,wheels,substeps,precalc,managed,native_pre,step,native_post,total,managed_per_wheel_us,native_per_wheel_us\n";
    for (const std::string& cn : Crafts(a, {"bug-e", "prospector", "crawler", "twostage"})) {
        auto craft = LoadCraft(ctx, cn);
        const bool lander = cn == "twostage" || cn == "munsplorer";
        for (NativeModel nm : Natives(a))
            for (int sub : {8, 4}) {
                PhaseTimes best;
                best.total = 1e30;
                int wheels = 0;
                for (int rep = 0; rep < repeat; rep++) {
                    RoverConfig cfg = BaseConfig(a);
                    cfg.native = nm;
                    Rover rv(ctx, *craft, cfg);
                    for (int i = 0; i < rv.WheelCount(); i++) rv.Wheel(i).integrationSteps = sub;
                    wheels = rv.WheelCount();
                    const int ticks = static_cast<int>(secs / cfg.dt);
                    PhaseTimes s;
                    int n = 0;
                    for (int t = 0; t < ticks; t++) {
                        Controls c;
                        c.wheelThrottle = lander ? 0.0f : 1.0f;
                        rv.Tick(c);
                        if (t < 50) continue;  // warm-up
                        const PhaseTimes& p = rv.LastTimes();
                        s.precalc += p.precalc;
                        s.managed += p.managed;
                        s.nativePre += p.nativePre;
                        s.step += p.step;
                        s.nativePost += p.nativePost;
                        s.total += p.total;
                        n++;
                    }
                    s.precalc /= n;
                    s.managed /= n;
                    s.nativePre /= n;
                    s.step /= n;
                    s.nativePost /= n;
                    s.total /= n;
                    if (s.total < best.total) best = s;
                }
                const double mpw = wheels ? best.managed / wheels * 1000.0 : 0.0;
                const double npw = wheels ? (best.nativePre + best.nativePost) / wheels * 1000.0 : 0.0;
                o.md << "| " << craft->name << " | " << NativeName(nm) << " | " << wheels << " | " << sub << " | " << F(best.precalc, 4) << " | "
                     << F(best.managed, 4) << " | " << F(best.nativePre, 4) << " | " << F(best.step, 4) << " | " << F(best.nativePost, 4) << " | "
                     << F(best.total, 4) << " | " << F(mpw, 2) << " | " << F(npw, 2) << " | " << F(mpw + npw, 2) << " |\n";
                o.csv << craft->name << "," << NativeName(nm) << "," << wheels << "," << sub << "," << best.precalc << "," << best.managed << ","
                      << best.nativePre << "," << best.step << "," << best.nativePost << "," << best.total << "," << mpw << "," << npw << "\n";
                std::fflush(stdout);
            }
    }
    o.Write();
    return 0;
}

}  // namespace

int RunBench(spike::KspContext& ctx, const std::string& set, const Args& a) {
    if (set == "rest") return BenchRest(ctx, a);
    if (set == "drive") return BenchDrive(ctx, a);
    if (set == "brake") return BenchBrake(ctx, a);
    if (set == "touchdown") return BenchTouchdown(ctx, a);
    if (set == "cost") return BenchCost(ctx, a);
    throw std::runtime_error("unknown bench " + set);
}

}  // namespace wspike
