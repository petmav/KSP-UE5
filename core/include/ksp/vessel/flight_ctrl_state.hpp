// FlightCtrlState.cs (250 lines). WP-24 (control / SAS) owns the class long term; WP-14 needs it whole because Vessel.ctrlState,
// Vessel.setControlStates and ProtoVessel's CTRLSTATE node are part of the vessel model.
#pragma once

#include <memory>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class FlightCtrlState {
public:
    // GameSettings.AXIS_CUSTOM.Length (GameSettings.cs:1346 `new AxisKeyBindingList(4)`)
    static constexpr int kCustomAxes = 4;

    float mainThrottle = 0.0f;
    float roll = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float rollTrim = 0.0f;
    float yawTrim = 0.0f;
    float pitchTrim = 0.0f;
    float wheelSteer = 0.0f;
    float wheelSteerTrim = 0.0f;
    float wheelThrottle = 0.0f;
    float wheelThrottleTrim = 0.0f;
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
    bool killRot = false;
    bool gearUp = false;
    bool gearDown = false;
    bool headlight = false;
    // float[] custom_axes: a reference array, shared by the states of one vessel (Vessel.Awake: new FlightCtrlState(ctrlState.custom_axes))
    std::shared_ptr<std::vector<float>> custom_axes;

    FlightCtrlState();                                                      // FlightCtrlState.cs:81
    explicit FlightCtrlState(std::shared_ptr<std::vector<float>> axes);     // FlightCtrlState.cs:86

    bool isIdle() const;     // FlightCtrlState.cs:41
    bool isNeutral() const;  // FlightCtrlState.cs:55

    void CopyFrom(const FlightCtrlState& st);                           // FlightCtrlState.cs:91
    void MaskedCopyFrom(const FlightCtrlState& st, KSPAxisGroup mask);  // FlightCtrlState.cs:114
    void Neutralize();                                                  // FlightCtrlState.cs:158
    void NeutralizeStick();                                             // FlightCtrlState.cs:173
    void ResetTrim();                                                   // FlightCtrlState.cs:181
    void NeutralizeAll();                                               // FlightCtrlState.cs:190
    void Load(const ConfigNode& node);                                  // FlightCtrlState.cs:196
    void Save(ConfigNode& node) const;                                  // FlightCtrlState.cs:227
    Vector3f GetPYR() const { return Vector3f(pitch, roll, yaw); }      // FlightCtrlState.cs:238
    Vector3f GetXYZ() const { return Vector3f(X, Y, Z); }               // FlightCtrlState.cs:243
};

}  // namespace ksp
