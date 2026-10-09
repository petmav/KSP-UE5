// FlightLogger.cs (401 lines): the per-flight statistics and the mission event log.
//
// Ported: Start / OnDestroy (the GameEvents subscriptions), LateUpdate / FixedUpdate (statistics), IgnoreGeeForces /
// ResumeGForceReading, getMissionStats, every event handler, LogEvent, checkMissionEnd. The Unity pieces become host
// callbacks: `FlightLogger::host` (AttemptVesselSwitch -> FlightGlobals.FindNearestControllableVessel / ForceSetActiveVessel,
// waitAndEndFlight's save and results dialog) and RunDelayedCalls(realtime) for the Invoke / coroutine timing. Not ported:
// Update (key toggle of FlightResultsDialog, UI) and the dialog text.
#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "ksp/events/game_events.hpp"
#include "ksp/vessel/vessel_types.hpp"

namespace ksp {

class Part;
class Vessel;

class FlightLogger {
public:
    // What the MonoBehaviour does through scene statics when a mission ends
    struct Host {
        std::function<bool()> attemptVesselSwitch;  // FlightLogger.AttemptVesselSwitch (:367); true when a vessel was found and made active
        std::function<void(Vessel*, FlightEndModes)> endFlight;          // waitAndEndFlight after the wait (:378-:395)
    };
    static inline FlightLogger* fetch = nullptr;  // :43
    static inline std::vector<std::string> eventLog;  // :41
    static inline bool LogGees = false;           // :47
    static inline Host host;

    double missionTime = 0.0;       // :27
    float endFlightWait = 4.0f;     // :51
    float attemptSwitchWait = 0.5f;  // :53

    FlightLogger() = default;
    ~FlightLogger();  // OnDestroy (:111)
    FlightLogger(const FlightLogger&) = delete;
    FlightLogger& operator=(const FlightLogger&) = delete;

    static bool LiftOff() { return fetch != nullptr && fetch->liftOff_; }  // :55
    static double met() { return fetch->missionTime; }                      // :67

    void Start();         // :73 (a second instance destroys itself: `destroyed()` is true)
    bool destroyed() const { return destroyed_; }
    void LateUpdate();    // :153
    void FixedUpdate();   // :171
    static void IgnoreGeeForces(float duration);  // :193 (Invoke("ResumeGForceReading", max(duration, 0.01)))
    static std::string getMissionStats();         // :211
    void LogEvent(const std::string& message, const EventReport* report);  // :335
    void LogEvent(const std::string& message);                              // :335
    // The delayed calls: ResumeGForceReading (Invoke), AttemptVesselSwitch (Invoke) and the waitAndEndFlight coroutine, driven by
    // Time.time / Time.realtimeSinceStartup (`anyInput` = mouse button / Escape / Space of the coroutine's exit test)
    void RunDelayedCalls(float time, float realtimeSinceStartup, bool anyInput = false);
    static inline float resumeGeeAt = -1.0f;

    // statistics (private in C#)
    double highestAltitude() const { return highestAltitude_; }
    double groundDistance() const { return groundDistance_; }
    double totalDistance() const { return totalDistance_; }
    double highestGee() const { return highestGee_; }
    double highestSpeed() const { return highestSpeed_; }
    double highestSpeedOverLand() const { return highestSpeedOverLand_; }
    int partsLost() const { return partsLost_; }
    int kerbalsKilled() const { return kerbalsKilled_; }
    bool missionEnd() const { return missionEnd_; }
    const std::map<FlightEndModes, std::string>& endMessages() const { return endMessages_; }

private:
    void onJointBreak(EventReport* report);       // :216
    void onCrash(EventReport* report);            // :221
    void onCrashSplashdown(EventReport* report);  // :228
    void onCollision(EventReport* report);        // :235
    void onOverheat(EventReport* report);         // :242
    void onOverPressure(EventReport* report);     // :249
    void onOverG(EventReport* report);            // :256
    void onStageSeparation(EventReport* report);  // :263
    void onCrewKilled(EventReport* report);       // :291
    void onLaunch(EventReport* report);           // :302
    void onUndock(EventReport* report);           // :311
    void onSplashDamage(EventReport* report);     // :316
    void onCrewOnEva(events::GameEvents::FromToAction<Part*, Part*> fv);       // :268
    void onCrewBoardVessel(events::GameEvents::FromToAction<Part*, Part*> fp);  // :297
    bool checkMissionEnd(Part* p, FlightEndModes endMode);                      // :349
    std::string missionTimestamp() const;                                       // :69

    double highestAltitude_ = 0.0, groundDistance_ = 0.0, totalDistance_ = 0.0, highestGee_ = 0.0, instantGee_ = 0.0, sustainedGee_ = 0.0;
    double highestSpeed_ = 0.0, highestSpeedOverLand_ = 0.0;
    bool liftOff_ = false, missionEnd_ = false, okToSetSpeed_ = false, destroyed_ = false, subscribed_ = false;
    int partsLost_ = 0, kerbalsKilled_ = 0;
    std::map<FlightEndModes, std::string> endMessages_;
    static inline std::string lastReport_ = "";
    bool switchPending_ = false;
    float switchAt_ = 0.0f;
    struct EndWait {
        Vessel* v;
        FlightEndModes mode;
        float deadline;
    };
    std::vector<EndWait> endWaits_;
};

}  // namespace ksp
