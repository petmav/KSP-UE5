// ModuleTripLogger.cs, VesselTripLog.cs (see include/ksp/vessel/module_trip_logger.hpp)
#include "ksp/vessel/module_trip_logger.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
Vessel* VesselOf(PartModule& m) { return m.part != nullptr ? static_cast<Part*>(m.part)->vessel : nullptr; }
}  // namespace

ModuleTripLogger::~ModuleTripLogger() {
    if (subscribed_) {
        GE::onVesselSituationChange.Remove(this, &ModuleTripLogger::OnVesselSituationChanged);
        GE::onVesselSOIChanged.Remove(this, &ModuleTripLogger::OnVesselSOIChanged);
    }
}

// ModuleTripLogger.cs:17
void ModuleTripLogger::OnAwake() {
    PartModule::OnAwake();
    Log = FlightLog();
}

// ModuleTripLogger.cs:22
void ModuleTripLogger::OnLoad(const ConfigNode& node) {
    PartModule::OnLoad(node);
    if (node.GetNode("Surfaced")) LoadDeprecated("Surfaced", FlightLog::EntryType::Land, node);
    if (node.GetNode("Flew")) LoadDeprecated("Flew", FlightLog::EntryType::Flight, node);
    if (node.GetNode("FlewBy")) LoadDeprecated("FlewBy", FlightLog::EntryType::Flyby, node);
    if (node.GetNode("Orbited")) LoadDeprecated("Orbited", FlightLog::EntryType::Orbit, node);
    if (node.GetNode("SubOrbited")) LoadDeprecated("SubOrbited", FlightLog::EntryType::Suborbit, node);
    Log.Load(node.GetNode("Log"));
}

// ModuleTripLogger.cs:40
void ModuleTripLogger::LoadDeprecated(const std::string& oldName, FlightLog::EntryType newName, const ConfigNode& node) {
    const std::vector<std::string> values = node.GetNode(oldName)->GetValues("at");
    for (const std::string& v : values) Log.AddEntry(newName, v);
}

// ModuleTripLogger.cs:53
void ModuleTripLogger::OnSave(ConfigNode& node) {
    PartModule::OnSave(node);
    Log.Save(*node.AddNode("Log"));
}

// ModuleTripLogger.cs:58
void ModuleTripLogger::OnStart(StartState state) {
    PartModule::OnStart(state);
    GE::onVesselSituationChange.Add(this, &ModuleTripLogger::OnVesselSituationChanged);
    GE::onVesselSOIChanged.Add(this, &ModuleTripLogger::OnVesselSOIChanged);
    subscribed_ = true;
}

// ModuleTripLogger.cs:74
void ModuleTripLogger::OnVesselSituationChanged(GE::HostedFromToAction<Vessel*, Vessel_Situations> vs) {
    if (!(vs.host != VesselOf(*this))) InvokeLogVessel(0.2f);
}

// ModuleTripLogger.cs:82
void ModuleTripLogger::OnVesselSOIChanged(GE::HostedFromToAction<Vessel*, CelestialBody*> vc) {
    if (!(vc.host != VesselOf(*this))) InvokeLogVessel(0.2f);
}

// ModuleTripLogger.cs:90 (IsInvoking / CancelInvoke / Invoke)
void ModuleTripLogger::InvokeLogVessel(float delay) {
    invokePending_ = true;
    invokeAt_ = Time::time + delay;
}

void ModuleTripLogger::RunDelayedCalls() {
    if (invokePending_ && Time::time >= invokeAt_) {
        invokePending_ = false;
        LogVesselSituation();
    }
}

// ModuleTripLogger.cs:99
void ModuleTripLogger::LogVesselSituation() {
    Vessel* v = VesselOf(*this);
    if (v == nullptr) throw NullReferenceException();
    LogNewSituation(v->situation, v->mainBody());
}

// ModuleTripLogger.cs:104
void ModuleTripLogger::LogNewSituation(Vessel_Situations sit, CelestialBody* where) {
    switch (sit) {
        case Vessel_Situations::FLYING: LogSituation(FlightLog::EntryType::Flight, where->bodyName); break;
        case Vessel_Situations::LANDED:
        case Vessel_Situations::SPLASHED: LogSituation(FlightLog::EntryType::Land, where->bodyName); break;
        case Vessel_Situations::ORBITING: LogSituation(FlightLog::EntryType::Orbit, where->bodyName); break;
        case Vessel_Situations::SUB_ORBITAL: LogSituation(FlightLog::EntryType::Suborbit, where->bodyName); break;
        case Vessel_Situations::ESCAPING: LogSituation(FlightLog::EntryType::Escape, where->bodyName); break;
        default: break;
    }
}

// ModuleTripLogger.cs:127
void ModuleTripLogger::LogSituation(FlightLog::EntryType type, const std::string& target) {
    Log.AddEntryUnique(type, target);
    Part* p = static_cast<Part*>(part);
    for (const auto& c : p->protoModuleCrew) {
        c->flightLog.AddEntryUnique(type, target);
        c->UpdateExperience();
    }
    GE::OnFlightLogRecorded.Fire(VesselOf(*this));
}

// ModuleTripLogger.cs:139
void ModuleTripLogger::ForceVesselLog(Vessel* v, FlightLog::EntryType type, CelestialBody* body) {
    if (v == nullptr || body == nullptr) return;
    if (v->loaded) ForceLogEntry(v, type, body);
    else ForceLogEntry(v->protoVessel, type, body);
    GE::OnFlightLogRecorded.Fire(v);
}

// ModuleTripLogger.cs:155
void ModuleTripLogger::ForceLogEntry(Vessel* v, FlightLog::EntryType type, CelestialBody* body) {
    int count = static_cast<int>(v->Parts().size());
    while (count-- > 0) {
        Part* part = v->Parts()[static_cast<size_t>(count)];
        int count2 = static_cast<int>(part->protoModuleCrew.size());
        while (count2-- > 0) {
            part->protoModuleCrew[static_cast<size_t>(count2)]->flightLog.AddEntryUnique(type, body->bodyName);
            part->protoModuleCrew[static_cast<size_t>(count2)]->UpdateExperience();
        }
        int count3 = static_cast<int>(part->Modules().Count());
        while (count3-- > 0) {
            auto* tl = dynamic_cast<ModuleTripLogger*>(part->Modules()[static_cast<size_t>(count3)]);
            if (tl != nullptr) tl->Log.AddEntryUnique(type, body->bodyName);
        }
    }
}

// ModuleTripLogger.cs:183
void ModuleTripLogger::ForceLogEntry(ProtoVessel* pv, FlightLog::EntryType type, CelestialBody* body) {
    int count = static_cast<int>(pv->protoPartSnapshots.size());
    while (count-- > 0) {
        ProtoPartSnapshot& pps = *pv->protoPartSnapshots[static_cast<size_t>(count)];
        int count2 = static_cast<int>(pps.modules.size());
        while (count2-- > 0) {
            ProtoPartModuleSnapshot& m = *pps.modules[static_cast<size_t>(count2)];
            if (m.moduleName != "ModuleTripLogger") continue;
            FlightLog flightLog;
            flightLog.Load(m.moduleValues->GetNode("Log"));
            m.moduleValues->RemoveNode("Log");
            flightLog.AddEntryUnique(type, body->bodyName);
            flightLog.Save(*m.moduleValues->AddNode("Log"));
            for (const auto& c : pps.protoModuleCrew) {
                c->flightLog.AddEntryUnique(type, body->bodyName);
                c->UpdateExperience();
            }
        }
    }
}

// ModuleTripLogger.cs:208 (cacheAutoLOC_6003061 = Localizer.Format("#autoLoc_6003061"))
std::string ModuleTripLogger::GetModuleDisplayName() { return KSP::Localization::Localizer::Format("#autoLoc_6003061"); }

// ---- VesselTripLog.cs

// VesselTripLog.cs:12
VesselTripLog VesselTripLog::FromVessel(Vessel& v) {
    VesselTripLog t;
    for (Part* part : v.parts) {
        PartModuleList& mods = part->Modules();
        for (size_t i = 0; i < mods.Count(); i++)
            if (mods[i]->ClassName() == "ModuleTripLogger") t.Log.MergeWith(dynamic_cast<ModuleTripLogger*>(mods[i])->Log);
    }
    return t;
}

// VesselTripLog.cs:26
VesselTripLog VesselTripLog::FromProtoVessel(ProtoVessel& pv) {
    VesselTripLog t;
    for (auto& pps : pv.protoPartSnapshots)
        for (auto& m : pps->modules)
            if (m->moduleName == "ModuleTripLogger") {
                VesselTripLog t2;
                t2.Log.Load(m->moduleValues->GetNode("Log"));
                t.Log.MergeWith(t2.Log);
            }
    return t;
}

void RegisterTripLoggerModule() {
    ModuleFactory::Register("ModuleTripLogger", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<ModuleTripLogger>(c));
    });
}

}  // namespace ksp
