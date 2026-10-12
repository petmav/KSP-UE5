// Wheel spike: scenarios, analytic references and the measurement sets (core/WHEEL-SPIKE.md section 3-4).
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "craft_model.hpp"
#include "rover.hpp"

namespace spike {
class KspContext;
}

namespace wspike {

struct Args {
    std::vector<std::string> positional;
    std::map<std::string, std::string> opts;
    Args(int argc, char** argv);
    bool Has(const std::string& k) const { return opts.count(k) != 0; }
    std::string Str(const std::string& k, const std::string& def) const;
    double Num(const std::string& k, double def) const;
};

// craft aliases (stock craft shipped with KSP 1.12.5, copies in harness/inputs/Ships) or a path
std::string CraftPath(const std::string& nameOrPath);
// ShipConstruct load with KSPUpgradePipeline.Process first (as the WP-18 gate does), then the Jolt spike's craft model
std::unique_ptr<spike::CraftModel> LoadCraft(spike::KspContext& ctx, const std::string& nameOrPath);
// the craft's up axis in the assembled frame (RoverConfig::craftUp), recorded by LoadCraft
JPH::Vec3 CraftUpOf(const spike::CraftModel& m);

void PrintCraftInfo(spike::KspContext& ctx, spike::CraftModel& craft, const RoverConfig& cfg);

// ---- scenarios ----
struct RestResult {
    std::string craft, native;
    double warp = 1, seconds = 0;
    bool brakes = false;
    double horizDrift = 0, vertDrift = 0;     // CoM displacement over the window (m)
    double maxSpeed = 0, rmsVertSpeed = 0, maxAngVel = 0;
    double settleDrop = 0;                    // CoM drop from spawn to the window start (m)
    double meanCompression = 0, maxCompression = 0;  // fraction of the suspension travel, end of run
    int grounded = 0, wheels = 0, bottomedTicks = 0;
    bool bodyTouch = false, exploded = false;
    double slopeDeg = 0;
    double tickMs = 0;
};
RestResult RunRest(spike::KspContext& ctx, spike::CraftModel& craft, RoverConfig cfg, double seconds, bool brakes, double warp,
                   double window0 = 5.0);

struct DriveSample {
    double t, v, vAnalytic, x, xAnalytic;
};
struct DriveResult {
    std::string craft, native, friction;
    std::vector<DriveSample> samples;
    double t5 = -1, t5a = -1, t10 = -1, t10a = -1;  // time to 5 / 10 m/s (sim, analytic)
    double vAt[4] = {}, vAtA[4] = {};               // speed at 10, 20, 30, 40 s
    double vMax = 0, vMaxA = 0, vTopAnalytic = 0;   // max reached; analytic asymptote (torque curve zero)
    double maxSlip = 0, maxPitchDeg = 0;
    double mass = 0;
    int motors = 0;
    bool exploded = false;
    double tickMs = 0;
};
DriveResult RunDrive(spike::KspContext& ctx, spike::CraftModel& craft, RoverConfig cfg, double seconds);

struct BrakeResult {
    std::string craft, native, friction;
    double v0 = 0, distance = 0, time = 0, distanceA = 0, timeA = 0;
    double distanceLocked = 0;  // analytic, wheels locked (sliding at the friction limit)
    double maxDecel = 0;
    bool exploded = false;
};
BrakeResult RunBrake(spike::KspContext& ctx, spike::CraftModel& craft, RoverConfig cfg, double v0);

struct TouchdownResult {
    std::string craft, native, ground;
    double gScale = 1, v0 = 0;
    double maxCompression = 0, maxCompressionM = 0;  // fraction / metres (any leg)
    double minClearance = 0;                         // lowest non-leg collider above the ground (m, min over the run)
    double maxUpSpeed = 0, reboundHeight = 0;        // after first contact
    int airborneTicksAfterContact = 0;               // ticks with no leg on the ground after the first contact
    bool bounceLaunch = false;                       // left the ground with upward speed > 0.5 m/s
    double settleTime = -1;
    bool bodyTouch = false, exploded = false;
    double maxImpactRatio = 0, maxStressRatio = 0;   // ModuleWheelDamage diagnostics (>= 1: KSP damages / explodes the leg)
    int bottomedTicks = 0;
};
TouchdownResult RunTouchdown(spike::KspContext& ctx, spike::CraftModel& craft, RoverConfig cfg, double v0, double seconds);

// measurement sets -> results/<set>.md|csv
int RunBench(spike::KspContext& ctx, const std::string& set, const Args& args);

}  // namespace wspike
