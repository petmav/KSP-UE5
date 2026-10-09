// FlightCtrlState.cs (see include/ksp/vessel/flight_ctrl_state.hpp)
#include "ksp/vessel/flight_ctrl_state.hpp"

#include <algorithm>

#include "ksp/dotnet/number.hpp"

namespace ksp {

namespace {
bool Has(KSPAxisGroup mask, KSPAxisGroup bit) { return (static_cast<int>(mask) & static_cast<int>(bit)) != 0; }
}  // namespace

// FlightCtrlState.cs:81
FlightCtrlState::FlightCtrlState() : custom_axes(std::make_shared<std::vector<float>>(static_cast<size_t>(kCustomAxes), 0.0f)) {}

// FlightCtrlState.cs:86
FlightCtrlState::FlightCtrlState(std::shared_ptr<std::vector<float>> axes) : custom_axes(std::move(axes)) {}

// FlightCtrlState.cs:41
bool FlightCtrlState::isIdle() const {
    if (static_cast<double>(roll) == 0.0 && static_cast<double>(pitch) == 0.0 && static_cast<double>(yaw) == 0.0 &&
        static_cast<double>(X) == 0.0 && static_cast<double>(Y) == 0.0 && static_cast<double>(Z) == 0.0)
        return static_cast<double>(wheelSteer) == 0.0;
    return false;
}

// FlightCtrlState.cs:55
bool FlightCtrlState::isNeutral() const {
    if (static_cast<double>(mainThrottle) == 0.0 && static_cast<double>(roll) == 0.0 && static_cast<double>(pitch) == 0.0 &&
        static_cast<double>(yaw) == 0.0 && static_cast<double>(rollTrim) == 0.0 && static_cast<double>(pitchTrim) == 0.0 &&
        static_cast<double>(yawTrim) == 0.0 && static_cast<double>(X) == 0.0 && static_cast<double>(Y) == 0.0 &&
        static_cast<double>(Z) == 0.0 && static_cast<double>(wheelSteer) == 0.0 && static_cast<double>(wheelSteerTrim) == 0.0 &&
        static_cast<double>(wheelThrottle) == 0.0)
        return static_cast<double>(wheelThrottleTrim) == 0.0;
    return false;
}

// FlightCtrlState.cs:91
void FlightCtrlState::CopyFrom(const FlightCtrlState& st) {
    mainThrottle = st.mainThrottle;
    roll = st.roll;
    yaw = st.yaw;
    pitch = st.pitch;
    rollTrim = st.rollTrim;
    yawTrim = st.yawTrim;
    pitchTrim = st.pitchTrim;
    wheelSteer = st.wheelSteer;
    wheelSteerTrim = st.wheelSteerTrim;
    wheelThrottle = st.wheelThrottle;
    wheelThrottleTrim = st.wheelThrottleTrim;
    X = st.X;
    Y = st.Y;
    Z = st.Z;
    killRot = st.killRot;
    gearUp = st.gearUp;
    gearDown = st.gearDown;
    headlight = st.headlight;
    if (custom_axes && st.custom_axes) {
        const size_t n = std::min(custom_axes->size(), st.custom_axes->size());
        for (size_t i = 0; i < n; i++) (*custom_axes)[i] = (*st.custom_axes)[i];
    }
}

// FlightCtrlState.cs:114 (sic: the Y component is copied under TranslateZ, TranslateY is never tested)
void FlightCtrlState::MaskedCopyFrom(const FlightCtrlState& st, KSPAxisGroup mask) {
    if (Has(mask, KSPAxisGroup::MainThrottle)) mainThrottle = st.mainThrottle;
    if (Has(mask, KSPAxisGroup::Roll)) {
        roll = st.roll;
        rollTrim = st.rollTrim;
    }
    if (Has(mask, KSPAxisGroup::Yaw)) {
        yaw = st.yaw;
        yawTrim = st.yawTrim;
    }
    if (Has(mask, KSPAxisGroup::Pitch)) {
        pitch = st.pitch;
        pitchTrim = st.pitchTrim;
    }
    if (Has(mask, KSPAxisGroup::WheelSteer)) {
        wheelSteer = st.wheelSteer;
        wheelSteerTrim = st.wheelSteerTrim;
    }
    if (Has(mask, KSPAxisGroup::WheelThrottle)) {
        wheelThrottle = st.wheelThrottle;
        wheelThrottleTrim = st.wheelThrottleTrim;
    }
    if (Has(mask, KSPAxisGroup::TranslateX)) X = st.X;
    if (Has(mask, KSPAxisGroup::TranslateZ)) Y = st.Y;
    if (Has(mask, KSPAxisGroup::TranslateZ)) Z = st.Z;
}

// FlightCtrlState.cs:158
void FlightCtrlState::Neutralize() {
    mainThrottle = 0.0f;
    roll = 0.0f;
    yaw = 0.0f;
    pitch = 0.0f;
    wheelSteer = 0.0f;
    wheelThrottle = 0.0f;
    X = 0.0f;
    Y = 0.0f;
    Z = 0.0f;
    killRot = false;
    gearUp = false;
    gearDown = false;
    headlight = false;
}

// FlightCtrlState.cs:173
void FlightCtrlState::NeutralizeStick() {
    roll = 0.0f;
    yaw = 0.0f;
    pitch = 0.0f;
    wheelSteer = 0.0f;
}

// FlightCtrlState.cs:181
void FlightCtrlState::ResetTrim() {
    rollTrim = 0.0f;
    yawTrim = 0.0f;
    pitchTrim = 0.0f;
    wheelSteerTrim = 0.0f;
    wheelThrottleTrim = 0.0f;
}

// FlightCtrlState.cs:190
void FlightCtrlState::NeutralizeAll() {
    Neutralize();
    ResetTrim();
}

// FlightCtrlState.cs:196
void FlightCtrlState::Load(const ConfigNode& node) {
    auto f = [&](const char* key, float& dst) {
        if (node.HasValue(key)) dst = dotnet::ParseSingle(*node.GetValue(key));
    };
    f("pitch", pitch);
    f("yaw", yaw);
    f("roll", roll);
    f("trimPitch", pitchTrim);
    f("trimYaw", yawTrim);
    f("trimRoll", rollTrim);
    f("mainThrottle", mainThrottle);
}

// FlightCtrlState.cs:227
void FlightCtrlState::Save(ConfigNode& node) const {
    node.AddValue("pitch", pitch);
    node.AddValue("yaw", yaw);
    node.AddValue("roll", roll);
    node.AddValue("trimPitch", pitchTrim);
    node.AddValue("trimYaw", yawTrim);
    node.AddValue("trimRoll", rollTrim);
    node.AddValue("mainThrottle", mainThrottle);
}

}  // namespace ksp
