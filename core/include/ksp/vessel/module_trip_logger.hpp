// ModuleTripLogger.cs (216 lines) and VesselTripLog.cs (47 lines).
// Unity's Invoke("LogVesselSituation", 0.2f) is a delayed call: the host (or a test) advances ksp::Time::time and calls
// RunDelayedCalls(), the same convention as OrbitDriver.RunDelayedCalls.
#pragma once

#include <string>

#include "ksp/events/game_events.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/vessel/vessel_types.hpp"

namespace ksp {

class Vessel;
class CelestialBody;

class ModuleTripLogger : public PartModule {
public:
    using PartModule::PartModule;
    ~ModuleTripLogger() override;  // OnDestroy (ModuleTripLogger.cs:64)

    FlightLog Log;  // ModuleTripLogger.cs:5

    void OnAwake() override;                       // :13
    void OnLoad(const ConfigNode& node) override;  // :18
    void OnSave(ConfigNode& node) override;        // :53
    void OnStart(StartState state) override;       // :58
    std::string GetModuleDisplayName() override;   // :207

    // Unity Invoke / IsInvoking / CancelInvoke for the one delayed call this module schedules
    void RunDelayedCalls();
    bool IsInvoking() const { return invokePending_; }

    // ModuleTripLogger.cs:139 ForceVesselLog (+ the two ForceLogEntry overloads, :155 and :179)
    static void ForceVesselLog(Vessel* v, FlightLog::EntryType type, CelestialBody* body);

private:
    void LoadDeprecated(const std::string& oldName, FlightLog::EntryType newName, const ConfigNode& node);  // :43
    void OnVesselSituationChanged(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :74
    void OnVesselSOIChanged(events::GameEvents::HostedFromToAction<Vessel*, CelestialBody*> vc);           // :82
    void InvokeLogVessel(float delay);                                                                      // :90
    void LogVesselSituation();                                                                              // :99
    void LogNewSituation(Vessel_Situations sit, CelestialBody* where);                                      // :104
    void LogSituation(FlightLog::EntryType type, const std::string& target);                                // :127
    static void ForceLogEntry(Vessel* v, FlightLog::EntryType type, CelestialBody* body);
    static void ForceLogEntry(ProtoVessel* pv, FlightLog::EntryType type, CelestialBody* body);
    bool subscribed_ = false;
    bool invokePending_ = false;
    float invokeAt_ = 0.0f;
};

// VesselTripLog.cs
class VesselTripLog {
public:
    FlightLog Log;                                       // :5
    VesselTripLog() = default;                           // :5
    static VesselTripLog FromVessel(Vessel& v);          // :12
    static VesselTripLog FromProtoVessel(ProtoVessel& pv);  // :26
};

void RegisterTripLoggerModule();  // called by RegisterBuiltinModules

}  // namespace ksp
