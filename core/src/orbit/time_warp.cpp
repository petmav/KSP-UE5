// TimeWarp (TimeWarp.cs); see include/ksp/orbit/time_warp.hpp.
//
// Float32 code follows the Mono rule (ksp/dotnet/mono_float.hpp): `Time.fixedDeltaTime = 0.02f * Time.timeScale` and the
// Lerp argument are rounded where C# stores them; `double x = float * double` is exact.
#include "ksp/orbit/time_warp.hpp"

#include <iterator>
#include <limits>
#include <string>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace {

namespace M = dotnet::Math;

// The game's own TimeWarp scene-object data, as the running game reports it (reflection dump of TimeWarp.fetch in the
// FLIGHT scene, KSP 1.12.5 build 03190, 19 runs that all agree):
//   analysis/physics/raw/run{1,2,3,4,5}/*_summary.csv rows rt_TimeWarp.{warpRates,physicsWarpRates,altitudeLimits,
//   maxPhysicsRate_index,maxModeSwitchRate_index}
// Cross-checked by the in-game C8 capture (segments.requested_rate for rate indices 0, 4, 5, 6, 7 = 1, 100, 1000, 10000,
// 100000) and by the 16 body prefabs (timeWarpAltitudeLimits = altitudeLimits * Radius, CelestialBody.resetTimeWarpLimits).
constexpr float kWarpRates[8] = {1.0f, 5.0f, 10.0f, 50.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f};
constexpr float kPhysicsWarpRates[4] = {1.0f, 2.0f, 3.0f, 4.0f};
constexpr float kAltitudeLimits[8] = {0.0f, 0.05f, 0.05f, 0.1f, 0.2f, 0.4f, 0.8f, 1.0f};

TimeWarpEnvironment gDefaultEnvironment;

std::string F0(float v) { return dotnet::FormatSingleF(v, 0); }  // float.ToString("0")
std::string F1(float v) { return dotnet::FormatSingleF(v, 1); }  // float.ToString("0.0")
std::string D0(double v) { return dotnet::FormatDoubleF(v, 0); }  // double.ToString("0")
std::string D1(double v) { return dotnet::FormatDoubleF(v, 1); }  // double.ToString("0.0")

// Localizer.Format(tag, args): without a Localizer instance (unit tests) the message is "tag|arg|arg" so the recorded
// text still identifies the message; the numerics never depend on it.
std::string Loc(const std::string& tag, const std::vector<std::string>& args = {}) {
    if (Localizer::Instance() != nullptr) return args.empty() ? Localizer::Format(tag) : Localizer::Format(tag, args);
    std::string s = tag;
    for (const auto& a : args) s += "|" + a;
    return s;
}

std::string PrintTimeText(double t) {
    if (Localizer::Instance() != nullptr) return KSPUtil::PrintTime(t, 3, true);
    return D1(t);
}

}  // namespace

// ---- environment ---------------------------------------------------------------------------------------------------

TimeWarpEnvironment::TimeWarpEnvironment() : LoadedSceneValue(GameScenes::LOADING) {}  // HighLogic.LoadedScene default
bool TimeWarpEnvironment::LoadedSceneIsFlight() const { return LoadedSceneValue == GameScenes::FLIGHT; }  // HighLogic.cs:304
bool TimeWarpEnvironment::LoadedSceneIsMainMenu() const { return LoadedSceneValue == GameScenes::MAINMENU; }

TimeWarpEnvironment& TimeWarp::Env() { return environment != nullptr ? *environment : gDefaultEnvironment; }

// ---- lifetime ------------------------------------------------------------------------------------------------------

// TimeWarp.cs:190 Awake
bool TimeWarp::Awake() {
    if (fetch != nullptr) return false;  // UnityEngine.Object.Destroy(this)
    fetch = this;
    return true;
}

// TimeWarp.cs:699 OnDestroy
TimeWarp::~TimeWarp() {
    Env().controlLocks.erase("TimeWarpLock");  // InputLockManager.RemoveControlLock
    Env().controlLocks.erase("TimeWarpTo");
    if (fetch != nullptr && fetch == this) fetch = nullptr;
}

void TimeWarp::LoadStockPrefab() {
    warpRates.assign(std::begin(kWarpRates), std::end(kWarpRates));
    physicsWarpRates.assign(std::begin(kPhysicsWarpRates), std::end(kPhysicsWarpRates));
    altitudeLimits.assign(std::begin(kAltitudeLimits), std::end(kAltitudeLimits));
    maxPhysicsRate_index = 0;
    maxModeSwitchRate_index = 0;
    Mode = Modes::HIGH;
    textDuration = 3.0f;
}

// ---- properties ----------------------------------------------------------------------------------------------------

// TimeWarp.cs:84
TimeWarp::Modes TimeWarp::WarpMode() { return fetch == nullptr ? Modes::HIGH : fetch->Mode; }

// TimeWarp.cs:98
float TimeWarp::CurrentRate() { return fetch == nullptr ? 1.0f : fetch->curr_rate; }

// TimeWarp.cs:110
int TimeWarp::CurrentRateIndex() { return fetch == nullptr ? 1 : fetch->current_rate_index; }

// TimeWarp.cs:122: curr_rate.Equals(current_rate_index): overload resolution picks Single.Equals(float) (the implicit
// int->float conversion is better than boxing), so the comparison is numeric.
bool TimeWarp::CurrentRateIsTargetRate() {
    if (fetch == nullptr) return false;
    return fetch->curr_rate == static_cast<float>(fetch->current_rate_index);
}

// TimeWarp.cs:134
float TimeWarp::MaxRate() {
    if (fetch == nullptr) return 100000.0f;
    return fetch->warpRates.at(fetch->warpRates.size() - 1);
}

// TimeWarp.cs:146
float TimeWarp::MaxPhysicsRate() {
    if (fetch == nullptr) return 4.0f;
    return fetch->warpRates.at(static_cast<size_t>(fetch->maxPhysicsRate_index));
}

// TimeWarp.cs:158
float TimeWarp::deltaTime() {
    if (fetch != nullptr) {
        if (WarpMode() != Modes::HIGH) return Time::deltaTime;
        return f32store(fx(Time::deltaTime) * fx(fetch->curr_rate));
    }
    return Time::deltaTime;
}

// TimeWarp.cs:174 (managed float code: Mono rule, the product is rounded at the return)
float TimeWarp::fixedDeltaTime() {
    if (fetch != nullptr) {
        if (WarpMode() != Modes::HIGH) return Time::fixedDeltaTime;
        return f32store(fx(Time::fixedDeltaTime) * fx(fetch->curr_rate));
    }
    return Time::fixedDeltaTime;
}

// ---- screen messages -----------------------------------------------------------------------------------------------

// TimeWarp.cs:201 PostScreenMessage(string text = null): `screenText` is one persistent ScreenMessage; a non-null text
// replaces its message, and it is (re)posted either way.
void TimeWarp::PostScreenMessage(const std::string& text) {
    if (!Env().LoadedSceneIsMainMenu()) {
        screenText_ = text;
        Env().screenMessages.push_back(screenText_);
    }
}

void TimeWarp::PostScreenMessage() {
    if (!Env().LoadedSceneIsMainMenu()) Env().screenMessages.push_back(screenText_);
}

// ScreenMessages.PostScreenMessage(<own message>) used by WarpTo / autoWarpTo / CancelAutoWarp
void TimeWarp::PostWarpMessage(const std::string& text) { Env().screenMessages.push_back(text); }

// ---- Start / buttons -----------------------------------------------------------------------------------------------

// TimeWarp.cs:213
void TimeWarp::Start() {
    if (!Env().LoadedSceneIsFlight() || Env().flightStarted) {  // HighLogic.LoadedScene != FLIGHT || flightStarted
        setRate(Mathf::Clamp(current_rate_index, 0, static_cast<int>(warpRates.size())), false);
        curr_rate = tgt_rate_;
        last_rate = tgt_rate_;
        updateRate(curr_rate);
        showPhyWarpWarning = GameSettings::SHOW_PWARP_WARNING;
    }
}

// TimeWarp.cs:225
void TimeWarp::btnSetHighRate(int rate) {
    if (!Env().timewarpInputLocked) {
        if (!Env().CanTimeWarpHigh)
            PostScreenMessage(Loc("#autoLOC_481056"));
        else if (setMode(Modes::HIGH))
            setRate(rate, false);
    }
}

// TimeWarp.cs:240
void TimeWarp::btnSetLowRate(int rate) {
    if (!Env().timewarpInputLocked) {
        if (!Env().CanTimeWarpLow)
            PostScreenMessage(Loc("#autoLOC_481056"));
        else if (setMode(Modes::LOW))
            setRate(rate, false);
    }
}

// ---- Update --------------------------------------------------------------------------------------------------------

// TimeWarp.cs:255
void TimeWarp::Update() {
    TimeWarpEnvironment& env = Env();
    if (setAutoWarp) {
        setAutoWarp = false;
        const double warpDeltaTime = warpToUT - Planetarium::GetUniversalTime();
        int rateIdx = 0;
        getMaxWarpRateForTravel(warpDeltaTime, warpToMaxWarping, warpToMinWarping, rateIdx);
        setRate(Mathf::Min(current_rate_index + 1, rateIdx), false);
        // StartCoroutine(autoWarpTo(...)): runs to the first yield now
        autoWarpTgtUT_ = warpToUT;
        autoWarpRateIdx_ = rateIdx;
        autoWarpState_ = 0;
        autoWarpMoveNext();
        return;
    }
    if (env.timewarpInputLocked) return;
    if (env.LoadedSceneIsFlight() && FlightGlobals::ready && current_rate_index <= maxModeSwitchRate_index && curr_rate == tgt_rate_) {
        IFlightVessel* activeVessel = FlightGlobals::ActiveVessel;
        if (activeVessel == nullptr) throw NullReferenceException("TimeWarp.Update: FlightGlobals.ActiveVessel");
        CelestialBody* mainBody = activeVessel->mainBody();
        if (Mode == Modes::HIGH) {
            if (!activeVessel->LandedOrSplashed()) {
                const double num = FlightGlobals::getAltitudeAtPos(activeVessel->TransformPosition(), mainBody);
                // if (double.IsInfinity(num)) Debug.Break();  (editor pause)
                if (FlightGlobals::getStaticPressure(num, mainBody) > 0.0) setMode(Modes::LOW);
            } else if (FlightGlobals::ship_srfVelocity().sqrMagnitude() > GThreshold) {
                setMode(Modes::LOW);
            }
        }
        if (Mode == Modes::LOW) {
            const double num2 = FlightGlobals::getAltitudeAtPos(activeVessel->TransformPosition(), activeVessel->mainBody());
            const double staticPressure = FlightGlobals::getStaticPressure(num2);
            if ((activeVessel->LandedOrSplashed() && FlightGlobals::ship_srfVelocity().sqrMagnitude() < GThreshold) ||
                staticPressure == 0.0)
                setMode(Modes::HIGH);
        }
    }
    if (env.TIME_WARP_INCREASE_down) {
        if (env.MODIFIER_KEY_held && env.LoadedSceneIsFlight()) {
            if (!env.CanTimeWarpLow) {
                PostScreenMessage(Loc("#autoLOC_481056"));
                return;
            }
            if (setMode(Modes::LOW)) {
                if (showPhyWarpWarning && current_rate_index + 1 == 2)
                    ShowPWarpWarning();
                else
                    setRate(Mathf::Min(current_rate_index + 1, static_cast<int>(physicsWarpRates.size()) - 1), false);
            }
        } else {
            if (!env.CanTimeWarpHigh) {
                PostScreenMessage(Loc("#autoLOC_481056"));
                return;
            }
            if (Mode == Modes::HIGH) setRate(Mathf::Min(current_rate_index + 1, static_cast<int>(warpRates.size()) - 1), false);
            if (Mode == Modes::LOW) {
                if (showPhyWarpWarning && current_rate_index + 1 == 2)
                    ShowPWarpWarning();
                else
                    setRate(Mathf::Min(current_rate_index + 1, static_cast<int>(physicsWarpRates.size()) - 1), false);
            }
        }
    }
    if (env.TIME_WARP_DECREASE_down) {
        if (env.MODIFIER_KEY_held && env.LoadedSceneIsFlight()) {
            if (!env.CanTimeWarpLow) {
                PostScreenMessage(Loc("#autoLOC_481056"));
                return;
            }
            if (setMode(Modes::LOW)) setRate(Mathf::Max(current_rate_index - 1, 0), false);
        } else {
            if (!env.CanTimeWarpHigh) {
                PostScreenMessage(Loc("#autoLOC_481056"));
                return;
            }
            setRate(Mathf::Max(current_rate_index - 1, 0), false);
        }
    }
    if (env.TIME_WARP_STOP_down && current_rate_index != 0) {
        CancelAutoWarp();
        setRate(0, false);
    }
}

// TimeWarp.cs:385 (the dialog is UI: recorded as a flag; the game pauses while it is open)
void TimeWarp::ShowPWarpWarning() {
    Env().paused = true;  // FlightDriver.SetPause(true)
    ++Env().physWarpWarningShown;
}

// TimeWarp.cs:394
void TimeWarp::OnPWarpWarningDismiss() {
    if (saveShowPhysWarp) {
        GameSettings::SHOW_PWARP_WARNING = showPhyWarpWarning;
        Env().settingsSaved = true;  // GameSettings.SaveSettings()
    }
    showPhyWarpWarning = false;
    Env().paused = false;
}

// ---- FixedUpdate ---------------------------------------------------------------------------------------------------

// TimeWarp.cs:405
void TimeWarp::FixedUpdate() {
    IFlightVessel* activeVessel = FlightGlobals::ActiveVessel;
    if (Env().LoadedSceneIsFlight() && Mode == Modes::HIGH &&
        curr_rate > warpRates.at(static_cast<size_t>(maxPhysicsRate_index))) {
        if (activeVessel == nullptr) throw NullReferenceException("TimeWarp.FixedUpdate: FlightGlobals.ActiveVessel");
        if (!activeVessel->LandedOrSplashed()) setRate(current_rate_index, false, true, false);
    }
    if (Planetarium::TimeScale() != static_cast<double>(tgt_rate_)) {
        curr_rate = Mathf::Lerp(last_rate, tgt_rate_, f32store(fx(Time::time) - fx(LerpStartTime)));
        updateRate(curr_rate);
    }
}

// TimeWarp.cs:419
void TimeWarp::updateRate(float r, bool postScreenMessage) {
    if (Planetarium::fetch != nullptr) Planetarium::fetch->timeScale = static_cast<double>(r);  // Planetarium.TimeScale = r
    Time::timeScale = (Mode == Modes::LOW) ? r : 1.0f;
    Time::fixedDeltaTime = f32store(fx(0.02f) * fx(Time::timeScale));
    if (postScreenMessage) PostScreenMessage(Loc("#autoLOC_7003002", {(curr_rate >= 1.0f) ? F0(r) : F1(r)}));
    if (r <= MaxPhysicsRate() && controlsLocked) {
        Env().controlLocks.erase("TimeWarpLock");  // InputLockManager.RemoveControlLock
        controlsLocked = false;
    }
}

// TimeWarp.cs:435
double TimeWarp::GetAltitudeLimit(int i, const CelestialBody& body) const {
    return static_cast<double>(body.timeWarpAltitudeLimits.at(static_cast<size_t>(i)));
}

// TimeWarp.cs:440
void TimeWarp::SetRate(int rate_index, bool instant, bool postScreenMessage) {
    if (fetch != nullptr) {
        rate_index = Mathf::Clamp(rate_index, 0, static_cast<int>(fetch->warpRates.size()));
        fetch->setRate(rate_index, instant, false, true, postScreenMessage);
    }
}

// TimeWarp.cs:449
bool TimeWarp::setRate(int rateIdx, bool instantChange, bool instantIfLower, bool forceSwitch, bool postScreenMessage) {
    int num = rateIdx;
    if (Env().LoadedSceneIsFlight() && Mode == Modes::HIGH && rateIdx > maxPhysicsRate_index) {
        ClearToSaveStatus reason = ClearToSaveStatus::CLEAR;
        num = getMaxOnRailsRateIdx(rateIdx, true, reason);
        if (num < rateIdx && instantIfLower &&
            (reason == ClearToSaveStatus::NOT_IN_ATMOSPHERE || reason == ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH))
            instantChange = true;
        if (postScreenMessage) {
            const std::string maxPhys = F0(warpRates.at(static_cast<size_t>(maxPhysicsRate_index)));
            switch (reason) {
                case ClearToSaveStatus::NOT_IN_ATMOSPHERE: PostScreenMessage(Loc("#autoLOC_6001925", {maxPhys})); break;
                case ClearToSaveStatus::NOT_UNDER_ACCELERATION: PostScreenMessage(Loc("#autoLOC_6001926", {maxPhys})); break;
                case ClearToSaveStatus::NOT_WHILE_MOVING_OVER_SURFACE: PostScreenMessage(Loc("#autoLOC_6001927", {maxPhys})); break;
                case ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH: {
                    CelestialBody* cb = FlightGlobals::currentMainBody;
                    if (cb == nullptr) throw NullReferenceException("TimeWarp.setRate: FlightGlobals.currentMainBody");
                    const std::string limit =
                        D0(GetAltitudeLimit(Mathf::Min(num + 1, static_cast<int>(altitudeLimits.size())), *cb));
                    if (GameSettings::ORBIT_WARP_MAXRATE_MODE == MaxRailsRateMode::VesselAltitude)
                        PostScreenMessage(Loc("#autoLOC_6001928", {F0(warpRates.at(static_cast<size_t>(num))), limit}));
                    else
                        PostScreenMessage(Loc("#autoLOC_8003368", {F0(warpRates.at(static_cast<size_t>(num))), limit,
                                                                  D0(minPeAllowed + GameSettings::ORBIT_WARP_PEMODE_SURFACE_MARGIN)}));
                    break;
                }
                case ClearToSaveStatus::NOT_WHILE_ON_A_LADDER: PostScreenMessage(Loc("#autoLOC_6001929", {maxPhys})); break;
                default: break;
            }
        }
        if (num > maxPhysicsRate_index) {
            Env().controlLocks["TimeWarpLock"] = "ALL_SHIP_CONTROLS_ALLOW_UIMODE";  // InputLockManager.SetControlLock
            controlsLocked = true;
        }
    }
    const Modes mode = Mode;
    num = (mode == Modes::HIGH || mode != Modes::LOW) ? Mathf::Clamp(num, 0, static_cast<int>(warpRates.size()) - 1)
                                                      : Mathf::Clamp(num, 0, static_cast<int>(physicsWarpRates.size()) - 1);
    if (current_rate_index != num || forceSwitch) {
        current_rate_index = num;
        assumeWarpRate((Mode == Modes::HIGH) ? warpRates.at(static_cast<size_t>(num)) : physicsWarpRates.at(static_cast<size_t>(num)),
                       instantChange, postScreenMessage);
    }
    return num == rateIdx;
}

// TimeWarp.cs:503
int TimeWarp::getMaxOnRailsRateIdx(int tgtRateIdx, bool lookAhead, ClearToSaveStatus& reason) {
    reason = ClearToSaveStatus::CLEAR;
    if (tgtRateIdx > 0 && Env().LoadedSceneIsFlight() && FlightGlobals::ActiveVessel != nullptr) {
        for (size_t i = 0; i < FlightGlobals::VesselsLoaded.size(); ++i) {
            IFlightVessel* vessel = FlightGlobals::VesselsLoaded[i];
            if (vessel->isEVA() && vessel->OnALadder()) {
                reason = ClearToSaveStatus::NOT_WHILE_ON_A_LADDER;
                return 0;
            }
        }
        IFlightVessel* activeVessel = FlightGlobals::ActiveVessel;
        CelestialBody* mainBody = activeVessel->mainBody();
        if (!activeVessel->LandedOrSplashed()) {
            // double altitude = FlightGlobals.getAltitudeAtPos(activeVessel.transform.position, mainBody): the Vector3
            // overload returns a float
            double altitude = static_cast<double>(FlightGlobals::getAltitudeAtPos(activeVessel->TransformPosition(), mainBody));
            double staticPressure = FlightGlobals::getStaticPressure(altitude, mainBody);
            double val = 0.0;
            double val2 = 0.0;
            if (mainBody->hasSolidSurface) {
                if (mainBody->pqsController == nullptr)
                    throw NullReferenceException("TimeWarp.getMaxOnRailsRateIdx: mainBody.pqsController");
                val = mainBody->pqsController->radiusMax - mainBody->pqsController->radius;
                val2 = mainBody->pqsController->meshVertMax;
            }
            const double val3 = mainBody->atmosphere ? mainBody->atmosphereDepth : 0.0;
            minPeAllowed = M::Max(val, val3);
            minPeAllowed = M::Max(minPeAllowed, val2);
            if (staticPressure > 0.0) {
                reason = ClearToSaveStatus::NOT_IN_ATMOSPHERE;
                return 0;
            }
            if (activeVessel->geeForce() > GThreshold) {
                reason = ClearToSaveStatus::NOT_UNDER_ACCELERATION;
                return 0;
            }
            if (GameSettings::ORBIT_WARP_MAXRATE_MODE == MaxRailsRateMode::VesselAltitude &&
                activeVessel->altitude() < GetAltitudeLimit(tgtRateIdx, *mainBody)) {
                reason = ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH;
                tgtRateIdx = GetMaxRateForAltitude(activeVessel->altitude(), *mainBody);
            }
            if (lookAhead && tgtRateIdx > maxPhysicsRate_index) {
                if (tgtRateIdx > static_cast<int>(warpRates.size()) - 1) tgtRateIdx = static_cast<int>(warpRates.size()) - 1;
                const double num = static_cast<double>(Time::fixedDeltaTime);
                Orbit* orbit = activeVessel->orbit();
                if (orbit == nullptr) throw NullReferenceException("TimeWarp.getMaxOnRailsRateIdx: activeVessel.orbit");
                const double universalTime = Planetarium::GetUniversalTime();
                const double radius = mainBody->Radius;
                double num2 = num * static_cast<double>(warpRates.at(static_cast<size_t>(tgtRateIdx)));
                double num3 = orbit->timeToPe;
                if (num3 < 0.0) num3 = std::numeric_limits<double>::max();
                const double peA = orbit->PeA();
                if (num2 > num3) {
                    num2 = num3;
                    altitude = peA;
                } else {
                    altitude = orbit->getRelativePositionAtUT(universalTime + num2).magnitude() - radius;
                }
                staticPressure = mainBody->GetPressure(altitude);
                while (tgtRateIdx > maxPhysicsRate_index && (staticPressure > 0.0 || !(altitude >= 0.0))) {
                    tgtRateIdx--;
                    const double num4 = num * static_cast<double>(warpRates.at(static_cast<size_t>(tgtRateIdx)));
                    if (num4 < num3) {
                        num2 = num4;
                        altitude = orbit->getRelativePositionAtUT(universalTime + num2).magnitude() - radius;
                    } else {
                        num2 = num3;
                        altitude = peA;
                    }
                    staticPressure = mainBody->GetPressure(altitude);
                    reason = ClearToSaveStatus::NOT_IN_ATMOSPHERE;
                }
                if (GameSettings::ORBIT_WARP_MAXRATE_MODE != MaxRailsRateMode::PeAltitude ||
                    !(peA > minPeAllowed + GameSettings::ORBIT_WARP_PEMODE_SURFACE_MARGIN)) {
                    while (tgtRateIdx > maxPhysicsRate_index && !(altitude >= GetAltitudeLimit(tgtRateIdx, *mainBody))) {
                        tgtRateIdx--;
                        const double num5 = num * static_cast<double>(warpRates.at(static_cast<size_t>(tgtRateIdx)));
                        if (num5 < num3) {
                            num2 = num5;
                            altitude = orbit->getRelativePositionAtUT(universalTime + num2).magnitude() - radius;
                        } else {
                            num2 = num3;
                            altitude = peA;
                        }
                        reason = ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH;
                    }
                }
            }
        } else if (activeVessel->srf_velocity().sqrMagnitude() > 0.09000000357627869) {
            reason = ClearToSaveStatus::NOT_WHILE_MOVING_OVER_SURFACE;
            return 0;
        }
        if (GameSettings::ORBIT_WARP_DOWN_AT_SOI && activeVessel->PatchedConicsAttached()) {
            Orbit* vesselOrbit = activeVessel->orbit();
            if (vesselOrbit == nullptr) throw NullReferenceException("TimeWarp.getMaxOnRailsRateIdx: activeVessel.orbit");
            if (vesselOrbit->patchEndTransition != Orbit::PatchTransitionType::FINAL) {
                const int num6 = ClampRateToOrbitTransitions(tgtRateIdx, *vesselOrbit, 3, 50);
                if (num6 != tgtRateIdx) {
                    reason = ClearToSaveStatus::ORBIT_EVENT_IMMINENT;
                    log::Info("Orbit event imminent. Dropping TimeWarp to max limit: " + F0(warpRates.at(3)) + "x");
                    return num6;
                }
            }
        }
    }
    return tgtRateIdx;
}

// TimeWarp.cs:633
int TimeWarp::GetMaxRateForAltitude(double altitude, const CelestialBody& cb) const {
    int num = 0;
    const int num2 = static_cast<int>(altitudeLimits.size());
    while (true) {
        if (num < num2) {
            if (GetAltitudeLimit(num, cb) >= altitude) break;
            num++;
            continue;
        }
        return static_cast<int>(warpRates.size()) - 1;
    }
    return Mathf::Max(0, num - 1);
}

// TimeWarp.cs:653
int TimeWarp::ClampRateToOrbitTransitions(int rate, const Orbit& obt, int maxAllowedSOITransitionRate, int secondsBeforeSOItransition) {
    if (obt.patchEndTransition != Orbit::PatchTransitionType::FINAL && rate > maxAllowedSOITransitionRate) {
        const double v = static_cast<double>(warpRates.at(static_cast<size_t>(maxAllowedSOITransitionRate)));
        const double rateChangeTravel = getRateChangeTravel(Planetarium::TimeScale(), v, getWarpAccel(Planetarium::TimeScale(), v, 1.0));
        if (obt.EndUT - Planetarium::GetUniversalTime() < rateChangeTravel + static_cast<double>(secondsBeforeSOItransition))
            return maxAllowedSOITransitionRate;
    }
    return rate;
}

// TimeWarp.cs:666
void TimeWarp::assumeWarpRate(float rate, bool instant, bool postScreenMessage) {
    last_rate = tgt_rate_;
    tgt_rate_ = rate;
    LerpStartTime = Time::time;
    if (instant) {
        curr_rate = rate;
        updateRate(curr_rate, postScreenMessage);
    }
    Env().ignoreGeeForcesSeconds = 1.0f;  // FlightLogger.IgnoreGeeForces(1f)
    GameEvents::onTimeWarpRateChanged.Fire();
    if (postScreenMessage) PostScreenMessage();
}

// TimeWarp.cs:684
bool TimeWarp::setMode(Modes mode) {
    if (Mode != mode) {
        if (current_rate_index > maxModeSwitchRate_index) {
            PostScreenMessage(Loc("#autoLOC_6001930", {F1((Mode == Modes::HIGH) ? warpRates.at(static_cast<size_t>(maxModeSwitchRate_index))
                                                                                  : physicsWarpRates.at(static_cast<size_t>(maxModeSwitchRate_index)))}));
            return false;
        }
        Mode = mode;
    }
    GameEvents::onTimeWarpRateChanged.Fire();
    return true;
}

// ---- warp to a UT --------------------------------------------------------------------------------------------------

// TimeWarp.cs:709
void TimeWarp::WarpTo(double UT, double maxTimeWarping, double minTimeWarping) {
    if (autoWarpEngaged) {
        PostWarpMessage(Loc("#autoLOC_6001931"));  // ScreenMessages.PostScreenMessage(..., 5f, UPPER_CENTER)
        return;
    }
    setAutoWarp = true;
    warpToUT = UT;
    warpToMaxWarping = maxTimeWarping;
    warpToMinWarping = minTimeWarping;
}

// TimeWarp.cs:722
bool TimeWarp::CancelAutoWarp(int rateIdx, bool delay_unlock) {
    if (autoWarpEngaged) {
        PostWarpMessage(Loc("#autoLOC_6001932"));
        if (delay_unlock) {
            delayedUnlockFrames_ = 1;  // StartCoroutine(CallbackUtil.DelayedCallback(1, ...)): parked at its yield
        } else {
            Env().controlLocks.erase("TimeWarpTo");
        }
        autoWarpEngaged = false;
        if (rateIdx >= 0) setRate(rateIdx, false);
        return true;
    }
    return false;
}

// TimeWarp.cs:748 autoWarpTo, as an explicit state machine. The locals `num` (persisting across the do-while) is
// autoWarpNum_. Parked states: 1 = WaitForFixedUpdate at the loop head, 2 = WaitForFixedUpdate in the final
// `while (curr_rate != tgt_rate)`, 3 = `yield return null`.
bool TimeWarp::autoWarpMoveNext() {
    TimeWarpEnvironment& env = Env();
    switch (autoWarpState_) {
        case 0: {
            autoWarpEngaged = true;
            env.controlLocks["TimeWarpTo"] = "WARPTO_LOCK";  // InputLockManager.SetControlLock(WARPTO_LOCK, "TimeWarpTo")
            log::Info("Warping to UT:" + D1(autoWarpTgtUT_) + ". Max Rate Allowed: " + F1(tgt_rate_) + "x.");
            PostWarpMessage(Loc("#autoLOC_6001933", {PrintTimeText(Planetarium::GetUniversalTime() - autoWarpTgtUT_), "/"}));
            autoWarpState_ = 1;
            return true;
        }
        case 1: {
            if (!autoWarpEngaged) {
                autoWarpState_ = 0;
                return false;
            }
            if (current_rate_index < autoWarpRateIdx_) {
                ClearToSaveStatus reason = ClearToSaveStatus::CLEAR;
                const int maxOnRailsRateIdx = getMaxOnRailsRateIdx(autoWarpRateIdx_, true, reason);
                if (maxOnRailsRateIdx <= 0) PostWarpMessage(Loc("#autoLOC_6001934"));
                if (maxOnRailsRateIdx > current_rate_index) setRate(maxOnRailsRateIdx, false);
            }
            const double warpAccel = getWarpAccel(Planetarium::TimeScale(), 1.0, 1.0);
            const double rateChangeTravel = getRateChangeTravel(Planetarium::TimeScale(), 1.0, warpAccel);
            autoWarpNum_ = autoWarpTgtUT_ - rateChangeTravel;
            if (env.TIME_WARP_STOP_down) {
                CancelAutoWarp(0, true);
                autoWarpState_ = 0;
                return false;
            }
            if (env.TIME_WARP_DECREASE_down && current_rate_index > 0 && setRate(current_rate_index - 1, false)) {
                CancelAutoWarp();
                autoWarpState_ = 0;
                return false;
            }
            if (env.TIME_WARP_INCREASE_down && static_cast<float>(current_rate_index) < warpRates.at(warpRates.size() - 1) &&
                setRate(current_rate_index + 1, false)) {
                CancelAutoWarp();
                autoWarpState_ = 0;
                return false;
            }
            if (Planetarium::GetUniversalTime() < autoWarpNum_ && current_rate_index > 0) {
                PostWarpMessage(Loc("#autoLOC_6001933", {PrintTimeText(Planetarium::GetUniversalTime() - autoWarpTgtUT_), "/"}));
                autoWarpState_ = 1;
                return true;
            }
            if (current_rate_index > 0) {
                const double num2 = Planetarium::GetUniversalTime() - autoWarpNum_;
                log::Info("Warping down...  UT: " + D1(Planetarium::GetUniversalTime()) + " DecelStart at " + D1(autoWarpNum_) +
                          " Overshoot is " + D1(num2) + "s.");
                Planetarium::SetUniversalTime(autoWarpNum_ - static_cast<double>(fixedDeltaTime()) * 0.5);
                setRate(0, false);
                last_rate = static_cast<float>(Planetarium::TimeScale());
                if (curr_rate != tgt_rate_) {
                    PostWarpMessage(Loc("#autoLOC_6001935", {PrintTimeText(Planetarium::GetUniversalTime() - autoWarpTgtUT_)}));
                    autoWarpState_ = 2;
                    return true;
                }
                PostWarpMessage(Loc("#autoLOC_6001936"));
            }
            [[fallthrough]];
        }
        case 20: {  // the code after the loops
            const double num3 = Planetarium::GetUniversalTime() - autoWarpTgtUT_;
            log::Info("Real-time resumed. Current UT is: " + D1(Planetarium::GetUniversalTime()) + ". TgtUT Error is " +
                      dotnet::FormatDoubleF(num3, 4) + "s. " + ((num3 > 0.0) ? "(Overshot)" : "(Undershot)"));
            autoWarpState_ = 3;  // yield return null
            return true;
        }
        case 2: {
            if (curr_rate != tgt_rate_) {
                PostWarpMessage(Loc("#autoLOC_6001935", {PrintTimeText(Planetarium::GetUniversalTime() - autoWarpTgtUT_)}));
                autoWarpState_ = 2;
                return true;
            }
            PostWarpMessage(Loc("#autoLOC_6001936"));
            autoWarpState_ = 20;
            return autoWarpMoveNext();
        }
        case 3: {
            env.controlLocks.erase("TimeWarpTo");
            autoWarpEngaged = false;
            autoWarpState_ = 0;
            return false;
        }
        default: return false;
    }
}

// Coroutines parked on `yield return null` (they run after Update in the same frame cycle, i.e. on the next frame).
void TimeWarp::ResumeAfterUpdate() {
    if (autoWarpState_ == 3) autoWarpMoveNext();
    if (delayedUnlockFrames_ == 1) {
        delayedUnlockFrames_ = -1;
        Env().controlLocks.erase("TimeWarpTo");  // CallbackUtil.DelayedCallback(1, RemoveControlLock("TimeWarpTo"))
    }
}

// Coroutines parked on WaitForFixedUpdate (after the physics step of the tick).
void TimeWarp::ResumeAfterFixedUpdate() {
    if (autoWarpState_ == 1 || autoWarpState_ == 2) autoWarpMoveNext();
}

// TimeWarp.cs:818 (the call in Update passes warpToMaxWarping as minTimeInWarp: kept)
double TimeWarp::getMaxWarpRateForTravel(double warpDeltaTime, double minTimeInWarp, double maxTimeInWarp, int& rateIdx) const {
    double num = warpDeltaTime;
    double num2 = 1.0;
    rateIdx = 0;
    if (warpDeltaTime < minTimeInWarp) return 1.0;
    while (!(num <= maxTimeInWarp) && rateIdx < static_cast<int>(warpRates.size()) - 1) {
        num2 = static_cast<double>(warpRates.at(static_cast<size_t>(++rateIdx)));
        num = warpDeltaTime / num2;
    }
    if (num < minTimeInWarp) return static_cast<double>(warpRates.at(static_cast<size_t>(--rateIdx)));
    return num2;
}

// TimeWarp.cs:839
double TimeWarp::getWarpAccel(double v0, double v, double t) {
    if (t == 0.0) log::Error("GetWarpAccel: Time Scale cannot be zero!");
    return (v - v0) / t;
}

// TimeWarp.cs:848
double TimeWarp::getRateChangeTravel(double v0, double v, double a) {
    if (a == 0.0) return 0.0;
    return (v * v - v0 * v0) / (2.0 * a);
}

}  // namespace ksp
