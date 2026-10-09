// TimeWarp (TimeWarp.cs, 865 lines; WP-10 "Time warp, rails propagation, orbit driver"): the whole MonoBehaviour minus
// the UI (screen messages become strings recorded by the environment, the physics-warp warning dialog is a flag).
//
// Per-frame protocol for a host (Unity's order): Update(), then ResumeAfterUpdate() (coroutines parked on
// `yield return null`); per physics step FixedUpdate(), then ResumeAfterFixedUpdate() (coroutines parked on
// WaitForFixedUpdate: autoWarpTo). Start() once after the object exists; Awake()/destructor are the singleton
// bookkeeping (`fetch`).
//
// What the numerics read and where they come from:
//   * prefab data (warpRates, altitudeLimits, physicsWarpRates, maxPhysicsRate_index, maxModeSwitchRate_index):
//     LoadStockPrefab(), the game's own values (see time_warp.cpp for the provenance);
//   * the flight scene (HighLogic, FlightGlobals.ActiveVessel / VesselsLoaded / ready / getAltitudeAtPos /
//     getStaticPressure, GameSettings.ORBIT_WARP_*): FlightGlobals (IFlightVessel seam, WP-19 replaces it) and
//     GameSettings (ksp_util.hpp);
//   * HighLogic.LoadedScene, FlightDriver.flightStarted, the CanTimeWarp parameters, InputLockManager, key state,
//     ScreenMessages, FlightLogger.IgnoreGeeForces, FlightDriver.SetPause: TimeWarpEnvironment (data + recorded
//     outputs), because they are scene/UI/input state with no numerics.
//
// `curr_rate` and `updateRate` are public although private in KSP: WP-08's tests and the C8 runner drive them.
#pragma once

#include <map>
#include <string>
#include <vector>

namespace ksp {

class CelestialBody;
class Orbit;
enum class GameScenes : int;

// ClearToSaveStatus.cs
enum class ClearToSaveStatus : int {
    CLEAR = 0,
    NOT_IN_ATMOSPHERE,
    NOT_UNDER_ACCELERATION,
    NOT_WHILE_MOVING_OVER_SURFACE,
    NOT_WHILE_ABOUT_TO_CRASH,
    NOT_WHILE_ON_A_LADDER,
    NOT_WHILE_THROTTLED_UP,
    ORBIT_EVENT_IMMINENT
};

// The non-numeric environment TimeWarp.cs reaches through statics. Inputs are plain fields the host sets; outputs are
// recorded for the host (and the tests) to consume.
struct TimeWarpEnvironment {
    // ---- inputs ----
    GameScenes LoadedSceneValue;                  // HighLogic.LoadedScene (LoadedSceneIsFlight is derived)
    bool flightStarted = true;                    // FlightDriver.flightStarted
    bool CanTimeWarpHigh = true;                  // HighLogic.CurrentGame.Parameters.Flight.CanTimeWarpHigh
    bool CanTimeWarpLow = true;                   // ...CanTimeWarpLow
    bool timewarpInputLocked = false;             // InputLockManager.IsLocked(ControlTypes.TIMEWARP)
    // KeyBinding.GetKeyDown() / GetKey() of this frame (the host sets and clears them)
    bool TIME_WARP_INCREASE_down = false;
    bool TIME_WARP_DECREASE_down = false;
    bool TIME_WARP_STOP_down = false;
    bool MODIFIER_KEY_held = false;

    // ---- outputs ----
    std::vector<std::string> screenMessages;          // ScreenMessages.PostScreenMessage texts, in order
    std::map<std::string, std::string> controlLocks;  // InputLockManager: lockID -> ControlTypes name
    int physWarpWarningShown = 0;                     // ShowPWarpWarning calls
    bool paused = false;                              // FlightDriver.SetPause
    float ignoreGeeForcesSeconds = 0.0f;              // last FlightLogger.IgnoreGeeForces(seconds)
    bool settingsSaved = false;                       // GameSettings.SaveSettings called by OnPWarpWarningDismiss

    TimeWarpEnvironment();
    bool LoadedSceneIsFlight() const;
    bool LoadedSceneIsMainMenu() const;
};

class TimeWarp {
public:
    enum class Modes : int { HIGH = 0, LOW = 1 };                          // TimeWarp.cs:8
    enum class MaxRailsRateMode : int { VesselAltitude = 0, PeAltitude = 1 };  // TimeWarp.cs:14

    // ---- serialized fields (TimeWarp.cs:19-32) ----
    std::vector<float> warpRates;
    std::vector<float> altitudeLimits;      // fractions of the body radius (CelestialBody.resetTimeWarpLimits)
    std::vector<float> physicsWarpRates;
    int maxPhysicsRate_index = 0;
    int maxModeSwitchRate_index = 0;
    Modes Mode = Modes::HIGH;
    float textDuration = 3.0f;

    // ---- private state (TimeWarp.cs:34-62), public for the host ----
    float last_rate = 0.0f;
    float curr_rate = 0.0f;
    int current_rate_index = 0;
    float LerpStartTime = 0.0f;
    bool controlsLocked = false;
    bool autoWarpEngaged = false;
    bool showPhyWarpWarning = false;
    static inline bool saveShowPhysWarp = true;
    double shipAltitude = 0.0;  // unused by KSP's code paths
    int soiRate = 0;            // unused
    int cRateIndex = 0;         // unused
    double minPeAllowed = 0.0;
    static inline double GThreshold = 0.1;  // TimeWarp.cs:64
    bool setAutoWarp = false;
    double warpToUT = 0.0;
    double warpToMaxWarping = 0.0;
    double warpToMinWarping = 0.0;

    static inline TimeWarp* fetch = nullptr;
    static inline TimeWarpEnvironment* environment = nullptr;  // null = a process-wide default (scene LOADING)
    static TimeWarpEnvironment& Env();

    TimeWarp() = default;
    TimeWarp(const TimeWarp&) = delete;
    TimeWarp& operator=(const TimeWarp&) = delete;
    ~TimeWarp();  // OnDestroy (TimeWarp.cs:699)

    // The game's scene object data: warpRates 1 5 10 50 100 1000 10000 100000, physicsWarpRates 1 2 3 4,
    // altitudeLimits 0 0.05 0.05 0.1 0.2 0.4 0.8 1, maxPhysicsRate_index 0, maxModeSwitchRate_index 0.
    void LoadStockPrefab();

    // ---- properties (TimeWarp.cs:83-188) ----
    static Modes WarpMode();               // :84
    float tgt_rate() const { return tgt_rate_; }  // :96 (private set)
    static float CurrentRate();            // :98
    static int CurrentRateIndex();         // :111
    static bool CurrentRateIsTargetRate();  // :122
    static float MaxRate();                // :133
    static float MaxPhysicsRate();         // :145
    static float deltaTime();              // :157
    static float fixedDeltaTime();         // :174

    // ---- MonoBehaviour messages ----
    bool Awake();                  // :190 (false: a second instance, KSP destroys it)
    void Start();                  // :213
    void Update();                 // :255
    void FixedUpdate();            // :405
    void ResumeAfterUpdate();      // coroutines parked on `yield return null`
    void ResumeAfterFixedUpdate();  // coroutines parked on WaitForFixedUpdate

    // ---- rate control ----
    void updateRate(float r, bool postScreenMessage = true);  // :419
    double GetAltitudeLimit(int i, const CelestialBody& body) const;  // :435
    static void SetRate(int rate_index, bool instant, bool postScreenMessage = true);  // :440
    bool setRate(int rateIdx, bool instantChange, bool instantIfLower = false, bool forceSwitch = true,
                 bool postScreenMessage = true);  // :449
    int getMaxOnRailsRateIdx(int tgtRateIdx, bool lookAhead, ClearToSaveStatus& reason);  // :503
    int GetMaxRateForAltitude(double altitude, const CelestialBody& cb) const;  // :633
    int ClampRateToOrbitTransitions(int rate, const Orbit& obt, int maxAllowedSOITransitionRate,
                                    int secondsBeforeSOItransition);  // :653
    void assumeWarpRate(float rate, bool instant, bool postScreenMessage = true);  // :666
    bool setMode(Modes mode);  // :684

    // ---- warp to a UT ----
    void WarpTo(double UT, double maxTimeWarping = 8.0, double minTimeWarping = 2.5);  // :709
    bool CancelAutoWarp(int rateIdx = -1, bool delay_unlock = false);  // :722
    // autoWarpTo (:748): the coroutine's MoveNext, one step per call; false once finished. Started by Update().
    bool autoWarpMoveNext();
    bool AutoWarpRunning() const { return autoWarpState_ != 0; }
    double getMaxWarpRateForTravel(double warpDeltaTime, double minTimeInWarp, double maxTimeInWarp, int& rateIdx) const;  // :818
    static double getWarpAccel(double v0, double v, double t);          // :839
    static double getRateChangeTravel(double v0, double v, double a);   // :848

    // The pending physics-warp-warning dialog's dismiss (OnPWarpWarningDismiss, :394).
    void OnPWarpWarningDismiss();

private:
    void PostScreenMessage(const std::string& text);
    void PostScreenMessage();
    void PostWarpMessage(const std::string& text);
    void ShowPWarpWarning();
    void btnSetHighRate(int rate);
    void btnSetLowRate(int rate);

    float tgt_rate_ = 0.0f;
    std::string screenText_;  // the persistent ScreenMessage `screenText` (TimeWarp.cs:36)
    // the autoWarpTo coroutine: 0 not running, 1 parked at the loop's WaitForFixedUpdate, 2 parked at the final
    // WaitForFixedUpdate (curr_rate != tgt_rate), 3 parked at `yield return null`
    int autoWarpState_ = 0;
    double autoWarpTgtUT_ = 0.0;
    double autoWarpNum_ = 0.0;
    int autoWarpRateIdx_ = 0;
    // StartCoroutine(CallbackUtil.DelayedCallback(1, remove "TimeWarpTo")) from CancelAutoWarp(delay_unlock)
    int delayedUnlockFrames_ = -1;
};

}  // namespace ksp
