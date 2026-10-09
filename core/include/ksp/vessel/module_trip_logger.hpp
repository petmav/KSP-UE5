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
    ~ModuleTripLogger() override;  // OnDestroy (ModuleTripLogger.cs:69)

    FlightLog Log;  // ModuleTripLogger.cs:5

    void OnAwake() override;                       // :17
    void OnLoad(const ConfigNode& node) override;  // :22
    void OnSave(ConfigNode& node) override;        // :59
    void OnStart(StartState state) override;       // :64
    std::string GetModuleDisplayName() override;   // :208

    // Unity Invoke / IsInvoking / CancelInvoke for the one delayed call this module schedules
    void RunDelayedCalls();
    bool IsInvoking() const { return invokePending_; }

    // ModuleTripLogger.cs:144 ForceVesselLog (+ the two ForceLogEntry overloads, :160 and :183)
    static void ForceVesselLog(Vessel* v, FlightLog::EntryType type, CelestialBody* body);

private:
    void LoadDeprecated(const std::string& oldName, FlightLog::EntryType newName, const ConfigNode& node);  // :40
    void OnVesselSituationChanged(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :84
    void OnVesselSOIChanged(events::GameEvents::HostedFromToAction<Vessel*, CelestialBody*> vc);           // :93
    void InvokeLogVessel(float delay);                                                                      // :101
    void LogVesselSituation();                                                                              // :112
    void LogNewSituation(Vessel_Situations sit, CelestialBody* where);                                      // :117
    void LogSituation(FlightLog::EntryType type, const std::string& target);                                // :145
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
    VesselTripLog() = default;                           // :7
    static VesselTripLog FromVessel(Vessel& v);          // :12
    static VesselTripLog FromProtoVessel(ProtoVessel& pv);  // :26
};

void RegisterTripLoggerModule();  // called by RegisterBuiltinModules

}  // namespace ksp
