// WP-50: FlightInputHandler (Assembly-CSharp/FlightInputHandler.cs), FlightInputCallback.cs and AxisGroupsInputHandler.cs.
// The per-frame flight control mapping: key toggles (Update), then the axis / trim / throttle / precision-mode maths and the
// per-vessel control-state hand-off (FixedUpdate). All input comes through GameSettings bindings -> ExtendedInput -> IInputDevice;
// Time.deltaTime is ksp::Time::deltaTime.
//
// Seams (FlightInputHandler::host): ScreenMessages.PostScreenMessage, StageManager.ActivateNextStage (default: the ported
// StageManager), Vessel.FeedInputFeed (not in the vessel model), the ActionGroupsApp UI instance.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ksp/core/delegate.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_binding.hpp"
#include "ksp/vessel/flight_ctrl_state.hpp"

namespace ksp {

class Vessel;
class AxisGroupsModule;

// FlightInputCallback.cs:1 (public delegate void FlightInputCallback(FlightCtrlState st)): the state is a reference in C#
using FlightInputCallback = Delegate<void(FlightCtrlState&)>;

// the host seams of FlightInputHandler
struct FlightInputHost {
    std::function<void(const std::string& text, float duration)> postScreenMessage;  // ScreenMessages.PostScreenMessage(.., LOWER_CENTER)
    std::function<void()> activateNextStage;           // unset: StageManager.ActivateNextStage()
    std::function<void(Vessel*)> feedInputFeed;        // Vessel.FeedInputFeed()
    bool actionGroupsAppPresent = false;               // ActionGroupsApp.Instance != null
    std::function<void()> actionGroupsSelectNext;      // ActionGroupsApp.Instance.SelectNext()
    std::function<void()> actionGroupsSelectPrev;      // ActionGroupsApp.Instance.SelectPrev()
};

class FlightInputHandler {
public:
    static void NoCallback(FlightCtrlState&) {}           // `delegate { }` (:11, :15)
    static inline FlightCtrlState state;                  // :8
    // OnFlyByWire (:11, [Obsolete]) is never invoked by this class; OnRawAxisInput (:15) is invoked by FixedUpdate
    static inline FlightInputCallback OnRawAxisInput = FlightInputCallback::Static(&FlightInputHandler::NoCallback);

    float throttleResponsiveness = 2.0f;                  // :19
    float rcsDeadZone = 0.001f;                           // :21

    static inline FlightInputHandler* fetch = nullptr;    // :47
    static inline int currentTarget = 0;                  // :49

    bool stageLock = false;                               // :53
    bool rcslock = true;                                  // :55
    bool precisionMode = false;                           // :57
    bool hasFocus = true;                                 // :59
    static inline bool SPACENAV_USE_AS_FLIGHT_CONTROL = false;  // :61

    static bool RCSLock() { return fetch != nullptr ? fetch->rcslock : false; }  // :71

    // host seams (FlightInputHost: a namespace-level type, so its default member initializers are complete here)
    using Host = FlightInputHost;
    static inline Host host;

    void Awake();                                         // :83
    void Start();                                         // :89
    void OnVesselOverrideGroupChanged(Vessel* v);         // :96
    static void SetNeutralControls();                     // :104
    static void ResumeVesselCtrlState(Vessel* v);         // :115
    static void SetLaunchCtrlState();                     // :126
    void OnApplicationFocus(bool focus);                  // :136
    void Update();                                        // :145
    void ProcessAxis(AxisBinding& axisBinding, KeyBinding& plusKeyBinding, KeyBinding& minusKeyBinding, float& axisValue,
                     float& precisionAxis);               // :362
    void FixedUpdate();                                   // :377
    void OnDestroy();                                     // :695

    // private state (tests read it)
    float throttle = 0.0f;                                // :23
    float axisThrottle = 0.0f;                            // :25
    float lastAxisThrottle = 0.0f;                        // :27
    float precisionPitch = 0.0f, precisionRoll = 0.0f, precisionYaw = 0.0f;   // :29-33
    float precisionX = 0.0f, precisionY = 0.0f, precisionZ = 0.0f;            // :35-39
    float precisionWheelSteer = 0.0f, precisionWheelThrottle = 0.0f;          // :41-43
    std::vector<float> precision_custom_axis;             // :45 (null = empty)
    bool hasSpaceNavDevice = false;                       // :63
    bool linRotSwitchHold = false;                        // :65
    bool throttleFocus = false;                           // :67
    bool refocusThrottle = false;                         // :69

};

// AxisGroupsInputHandler.cs:4
class AxisGroupsInputHandler {
public:
    AxisGroupsModule* VesselAxisGroups = nullptr;         // :6

    void Awake();                                         // :8
    void OnDestroy();                                     // :14
    void onVesselChange(Vessel* vessel);                  // :20
    void AxisInputHandler(FlightCtrlState& ctrlState);    // :38
};

}  // namespace ksp
