// FlightLogger.cs (see include/ksp/vessel/flight_logger.hpp)
#include "ksp/vessel/flight_logger.hpp"

#include <algorithm>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
std::string L(const char* key) { return Localizer::Format(key); }
std::string L(const char* key, std::vector<std::string> args) { return Localizer::Format(key, args); }

// double.ToString("N0"): F0 with the group separator of the invariant/en-US culture the game runs under
std::string N0(double v) {
    std::string s = dotnet::FormatDoubleF(v, 0);
    const bool neg = !s.empty() && s[0] == '-';
    if (neg) s.erase(0, 1);
    for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) s.insert(static_cast<size_t>(i), ",");
    return (neg ? "-" : "") + s;
}
}  // namespace

// FlightLogger.cs:111 OnDestroy
FlightLogger::~FlightLogger() {
    if (subscribed_) {
        GE::onJointBreak.Remove(this, &FlightLogger::onJointBreak);
        GE::onCrash.Remove(this, &FlightLogger::onCrash);
        GE::onCrashSplashdown.Remove(this, &FlightLogger::onCrashSplashdown);
        GE::onCollision.Remove(this, &FlightLogger::onCollision);
        GE::onOverheat.Remove(this, &FlightLogger::onOverheat);
        GE::onOverPressure.Remove(this, &FlightLogger::onOverPressure);
        GE::onOverG.Remove(this, &FlightLogger::onOverG);
        GE::onStageSeparation.Remove(this, &FlightLogger::onStageSeparation);
        GE::onCrewOnEva.Remove(this, &FlightLogger::onCrewOnEva);
        GE::onCrewKilled.Remove(this, &FlightLogger::onCrewKilled);
        GE::onCrewBoardVessel.Remove(this, &FlightLogger::onCrewBoardVessel);
        GE::onLaunch.Remove(this, &FlightLogger::onLaunch);
        GE::onUndock.Remove(this, &FlightLogger::onUndock);
        GE::onSplashDamage.Remove(this, &FlightLogger::onSplashDamage);
    }
    lastReport_ = "";
    if (fetch == this) fetch = nullptr;
}

// FlightLogger.cs:73
void FlightLogger::Start() {
    if (fetch != nullptr) {
        destroyed_ = true;  // UnityEngine.Object.Destroy(this)
        return;
    }
    fetch = this;
    endMessages_[FlightEndModes::ABORTED] = L("#autoLOC_135142");
    endMessages_[FlightEndModes::CATASTROPHIC_FAILURE] = L("#autoLOC_135143");
    endMessages_[FlightEndModes::SUCCESS] = L("#autoLOC_135144");
    endMessages_[FlightEndModes::SUCCESSFUL_FAILURE] = L("#autoLOC_135145");
    eventLog.clear();
    highestAltitude_ = 0.0;
    groundDistance_ = 0.0;
    totalDistance_ = 0.0;
    highestGee_ = 0.0;
    highestSpeed_ = 0.0;
    highestSpeedOverLand_ = 0.0;
    okToSetSpeed_ = false;
    liftOff_ = false;
    missionEnd_ = false;
    GE::onJointBreak.Add(this, &FlightLogger::onJointBreak);
    GE::onCrash.Add(this, &FlightLogger::onCrash);
    GE::onCrashSplashdown.Add(this, &FlightLogger::onCrashSplashdown);
    GE::onCollision.Add(this, &FlightLogger::onCollision);
    GE::onOverheat.Add(this, &FlightLogger::onOverheat);
    GE::onOverPressure.Add(this, &FlightLogger::onOverPressure);
    GE::onOverG.Add(this, &FlightLogger::onOverG);
    GE::onStageSeparation.Add(this, &FlightLogger::onStageSeparation);
    GE::onCrewOnEva.Add(this, &FlightLogger::onCrewOnEva);
    GE::onCrewKilled.Add(this, &FlightLogger::onCrewKilled);
    GE::onCrewBoardVessel.Add(this, &FlightLogger::onCrewBoardVessel);
    GE::onLaunch.Add(this, &FlightLogger::onLaunch);
    GE::onUndock.Add(this, &FlightLogger::onUndock);
    GE::onSplashDamage.Add(this, &FlightLogger::onSplashDamage);
    subscribed_ = true;
}

// FlightLogger.cs:69 (KSPUtil.PrintTimeStamp(missionTime))
std::string FlightLogger::missionTimestamp() const { return KSPUtil::PrintTimeStamp(missionTime); }

// FlightLogger.cs:153
void FlightLogger::LateUpdate() {
    Vessel* activeVessel = FlightRegistry::ActiveVessel;
    if (activeVessel == nullptr || activeVessel->state == VesselState::DEAD || Planetarium::Pause() || !FlightRegistry::ready) return;
    missionTime = activeVessel->missionTime;
    highestAltitude_ = dotnet::Math::Max(activeVessel->altitude, highestAltitude_);  // FlightGlobals.ship_altitude
    highestSpeed_ = dotnet::Math::Max(activeVessel->srfSpeed, highestSpeed_);
    if (LogGees) {
        instantGee_ = activeVessel->geeForce;  // FlightGlobals.ship_geeForce
        highestGee_ = dotnet::Math::Max(highestGee_, instantGee_);
    }
    totalDistance_ += activeVessel->srfSpeed * static_cast<double>(TimeWarp::fixedDeltaTime());
    groundDistance_ += activeVessel->horizontalSrfSpeed * static_cast<double>(TimeWarp::fixedDeltaTime());
}

// FlightLogger.cs:171
void FlightLogger::FixedUpdate() {
    const bool flag = okToSetSpeed_;
    okToSetSpeed_ = false;
    Vessel* activeVessel = FlightRegistry::ActiveVessel;
    if (activeVessel == nullptr || activeVessel->state == VesselState::DEAD || Planetarium::Pause() || !FlightRegistry::ready) return;
    okToSetSpeed_ = flag;
    const double num = static_cast<double>(TimeWarp::fixedDeltaTime());
    totalDistance_ += activeVessel->srfSpeed * num;
    groundDistance_ += activeVessel->horizontalSrfSpeed * num;
    if (!okToSetSpeed_) okToSetSpeed_ = true;
    else highestSpeedOverLand_ = dotnet::Math::Max(activeVessel->horizontalSrfSpeed, highestSpeedOverLand_);
}

// FlightLogger.cs:193 (IsInvoking / CancelInvoke / Invoke are one pending time)
void FlightLogger::IgnoreGeeForces(float duration) {
    if (fetch == nullptr) return;
    LogGees = false;
    resumeGeeAt = Time::time + (duration > 0.01f ? duration : 0.01f);
}

// FlightLogger.cs:211
std::string FlightLogger::getMissionStats() {
    std::string s;
    s += L("#autoLOC_135285", {KSPUtil::PrintTimeLong(fetch->missionTime)});
    s += L("#autoLOC_135287", {N0(fetch->highestAltitude_)});
    s += L("#autoLOC_135288", {N0(fetch->highestSpeed_)});
    s += L("#autoLOC_135289", {N0(fetch->highestSpeedOverLand_)});
    s += L("#autoLOC_135291", {N0(fetch->groundDistance_)});
    s += L("#autoLOC_135292", {N0(fetch->totalDistance_)});
    s += L("#autoLOC_135294", {dotnet::FormatDoubleF(fetch->highestGee_, 1)});
    return s;
}

// FlightLogger.cs:213
void FlightLogger::onJointBreak(EventReport* report) { LogEvent(L("#autoLOC_135304", {report->sender, report->other}), report); }

// FlightLogger.cs:218
void FlightLogger::onCrash(EventReport* report) {
    fetch->checkMissionEnd(report->origin, FlightEndModes::CATASTROPHIC_FAILURE);
    fetch->partsLost_++;
    LogEvent(L("#autoLOC_135311", {report->sender, report->other}), report);
}

// FlightLogger.cs:225
void FlightLogger::onCrashSplashdown(EventReport* report) {
    fetch->checkMissionEnd(report->origin, FlightEndModes::CATASTROPHIC_FAILURE);
    fetch->partsLost_++;
    LogEvent(L("#autoLOC_135318", {report->sender}), report);
}

// FlightLogger.cs:232
void FlightLogger::onCollision(EventReport* report) {
    fetch->checkMissionEnd(report->origin, FlightEndModes::CATASTROPHIC_FAILURE);
    fetch->partsLost_++;
    LogEvent(L("#autoLOC_135325", {report->sender, report->other}), report);
}

// FlightLogger.cs:239
void FlightLogger::onOverheat(EventReport* report) {
    fetch->checkMissionEnd(report->origin, FlightEndModes::CATASTROPHIC_FAILURE);
    fetch->partsLost_++;
    LogEvent(L("#autoLOC_135332", {report->sender, report->msg}), report);
}

// FlightLogger.cs:246
void FlightLogger::onOverPressure(EventReport* report) {
    fetch->checkMissionEnd(report->origin, FlightEndModes::CATASTROPHIC_FAILURE);
    fetch->partsLost_++;
    LogEvent(L("#autoLOC_135339", {report->sender, report->msg}), report);
}

// FlightLogger.cs:253
void FlightLogger::onOverG(EventReport* report) {
    fetch->checkMissionEnd(report->origin, FlightEndModes::CATASTROPHIC_FAILURE);
    fetch->partsLost_++;
    LogEvent(L("#autoLOC_135346", {report->sender, report->msg}), report);
}

// FlightLogger.cs:260
void FlightLogger::onStageSeparation(EventReport* report) { LogEvent(L("#autoLOC_135351", {std::to_string(report->stage)}), report); }

// FlightLogger.cs:265
void FlightLogger::onCrewOnEva(GE::FromToAction<Part*, Part*> fv) {
    const bool female = fv.to->protoModuleCrew.at(0)->gender() == ProtoCrewMember_Gender::Female;
    const std::string toName = fv.to->vessel->vesselName;
    if (female) {
        if (fv.from != nullptr) {
            EventReport r(FlightEvents::CREW_ON_EVA, fv.to, toName, fv.from->vessel->GetDisplayName());
            LogEvent(L("#autoLOC_135358", {toName + " ^F", fv.from->vessel->GetDisplayName()}), &r);
        } else {
            EventReport r(FlightEvents::CREW_ON_EVA, fv.to, toName);
            LogEvent(L("#autoLOC_135358", {toName + "^F"}), &r);
        }
    } else if (fv.from != nullptr) {
        EventReport r(FlightEvents::CREW_ON_EVA, fv.to, toName, fv.from->vessel->GetDisplayName());
        LogEvent(L("#autoLOC_135358", {toName, fv.from->vessel->GetDisplayName()}), &r);
    } else {
        EventReport r(FlightEvents::CREW_ON_EVA, fv.to, toName);
        LogEvent(L("#autoLOC_135358", {toName}), &r);
    }
}

// FlightLogger.cs:287
void FlightLogger::onCrewKilled(EventReport* report) {
    fetch->kerbalsKilled_++;
    LogEvent(L("#autoLOC_135371", {report->sender}), report);
}

// FlightLogger.cs:293
void FlightLogger::onCrewBoardVessel(GE::FromToAction<Part*, Part*> fp) {
    EventReport r(FlightEvents::CREW_BOARD_VESSEL, fp.to, fp.from->vessel->vesselName, fp.to->vessel->GetDisplayName());
    LogEvent(L("#autoLOC_135376", {fp.from->vessel->vesselName, fp.to->partInfo->title, fp.to->vessel->GetDisplayName()}), &r);
}

// FlightLogger.cs:298
void FlightLogger::onLaunch(EventReport* report) {
    if (!fetch->liftOff_) {
        fetch->liftOff_ = true;
        LogEvent(L("#autoLOC_135384"), report);
    }
}

// FlightLogger.cs:309
void FlightLogger::onUndock(EventReport* report) { LogEvent(L("#autoLOC_135389", {report->sender, report->other}), report); }

// FlightLogger.cs:314
void FlightLogger::onSplashDamage(EventReport* report) { LogEvent(L("#autoLOC_135394", {report->sender, report->other}), report); }

// FlightLogger.cs:335 (the verbose Debug.Log line is log output only)
void FlightLogger::LogEvent(const std::string& message, const EventReport* /*report*/) { LogEvent(message); }

// FlightLogger.cs:335
void FlightLogger::LogEvent(const std::string& message) {
    const std::string text = "[" + missionTimestamp() + "]: " + message;
    if (!(text == lastReport_)) {
        lastReport_ = text;
        eventLog.push_back(text);
    }
}

// FlightLogger.cs:349
bool FlightLogger::checkMissionEnd(Part* p, FlightEndModes endMode) {
    if (FlightRegistry::ActiveVessel != nullptr && p == FlightRegistry::ActiveVessel->rootPart) {
        if (!switchPending_) {
            switchPending_ = true;
            switchAt_ = Time::time + attemptSwitchWait;
        }
        if (!missionEnd_) {
            endWaits_.push_back({p->vessel, endMode, Time::time + endFlightWait});
            missionEnd_ = true;
        }
        return true;
    }
    return false;
}

// Invoke("ResumeGForceReading"), Invoke("AttemptVesselSwitch") and the waitAndEndFlight coroutine (FlightLogger.cs:367, :376-:401)
void FlightLogger::RunDelayedCalls(float time, float /*realtimeSinceStartup*/, bool anyInput) {
    if (resumeGeeAt >= 0.0f && time >= resumeGeeAt) {
        resumeGeeAt = -1.0f;
        LogGees = true;  // ResumeGForceReading
    }
    if (switchPending_ && time >= switchAt_) {
        switchPending_ = false;
        if (host.attemptVesselSwitch && host.attemptVesselSwitch()) missionEnd_ = false;  // FlightLogger.cs:367
    }
    for (size_t i = 0; i < endWaits_.size();) {
        EndWait w = endWaits_[i];
        if (FlightRegistry::ActiveVessel != w.v) {  // the coroutine ends when the active vessel changed
            endWaits_.erase(endWaits_.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }
        if (time >= w.deadline || anyInput) {
            endWaits_.erase(endWaits_.begin() + static_cast<std::ptrdiff_t>(i));
            if (host.endFlight) host.endFlight(w.v, w.mode);
            continue;
        }
        i++;
    }
}

}  // namespace ksp
