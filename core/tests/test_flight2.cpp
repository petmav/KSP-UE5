// WP-19a unit tests, part 2: the FlightGlobals registry and queries, OrbitPhysicsManager, VesselPrecalculate on real vessels (rails, landed,
// easing, drift compensation), the physics_orbit_hold scenario. Part 1 (formulas and decisions against tools/wp19a/flight_reference.py) is
// tests/test_flight.cpp. No KSP game data are needed (the Kerbol system comes from the game's prefab data, Physics.cfg from harness/inputs).
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "doctest/doctest.h"

#include "ksp/localization/localizer.hpp"
#include "ksp/physics/reference_frame.hpp"
#include "ksp/vessel/vessel_types.hpp"
#include "flight_fixture.hpp"

using namespace ksp;
using namespace flighttest;
using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
struct FakeEva final : IEvaController {
    bool ladder = false;
    Part* LadderPart() override { return nullptr; }
    bool OnALadder() override { return ladder; }
};
}  // namespace

// ================================================================================================ the registry (FlightGlobals)

TEST_CASE("WP-19a FlightGlobals registry: AddVessel / RemoveVessel / SortVessel keep the three lists, the persistent ids, the events and the TimeWarp view") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    int added = 0, removed = 0;
    const auto hAdd = decltype(GE::onFlightGlobalsAddVessel)::OnEvent::Lambda(&added, [&](Vessel*) { added++; });
    const auto hRem = decltype(GE::onFlightGlobalsRemoveVessel)::OnEvent::Lambda(&removed, [&](Vessel*) { removed++; });
    GE::onFlightGlobalsAddVessel.Add(hAdd);
    GE::onFlightGlobalsRemoveVessel.Add(hRem);
    Vessel* a = w.MakeVessel("a", kerbin, 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    Vessel* b = w.MakeVessel("b", kerbin, 750000.0, 0.0, false, true);
    CHECK(added == 2);
    CHECK(FlightGlobals::Vessels == std::vector<Vessel*>{a, b});
    CHECK(FlightGlobals::VesselsLoaded == std::vector<Vessel*>{a});
    CHECK(FlightGlobals::VesselsUnloaded == std::vector<Vessel*>{b});
    PersistentIdRegistry& ids = SaveEnvironment::Current().persistentIds;
    CHECK(ids.persistentVesselIds.Contains(a->persistentId));
    CHECK(ids.persistentVesselIds.Contains(b->persistentId));
    Vessel* found = nullptr;
    CHECK(FlightGlobals::FindVessel(b->persistentId, found));
    CHECK(found == b);
    CHECK_FALSE(FlightGlobals::FindVessel(424242u, found));
    CHECK(found == nullptr);
    Guid g;
    g.bytes[0] = 7;
    a->id = g;
    CHECK(FlightGlobals::FindVessel(g) == a);
    CHECK(FlightGlobals::FindVessel(Guid::Empty()) == b);  // searched from the end: b has the empty id
    // the TimeWarp view follows the registry
    CHECK(FlightGlobals::VesselsLoadedView.size() == 1);
    CHECK(FlightGlobals::VesselsLoadedView[0] == &a->FlightView());
    // SortVessel(loaded): b moves to the loaded list (C#: only if it was in the unloaded list)
    b->loaded = true;
    FlightGlobals::SortVessel(b, true);
    CHECK(FlightGlobals::VesselsLoaded == std::vector<Vessel*>{a, b});
    CHECK(FlightGlobals::VesselsUnloaded.empty());
    FlightGlobals::SortVessel(b, true);  // already loaded: nothing
    CHECK(FlightGlobals::VesselsLoaded == std::vector<Vessel*>{a, b});
    CHECK(FlightGlobals::VesselsLoadedView.size() == 2);
    // Unload moves it back through SetLoaded -> SortVessel(false)
    b->Unload();
    CHECK(FlightGlobals::VesselsLoaded == std::vector<Vessel*>{a});
    CHECK(FlightGlobals::VesselsUnloaded == std::vector<Vessel*>{b});
    FlightGlobals::RemoveVessel(a);
    CHECK(removed == 1);
    CHECK(FlightGlobals::Vessels == std::vector<Vessel*>{b});
    CHECK(FlightGlobals::VesselsLoaded.empty());
    CHECK_FALSE(ids.persistentVesselIds.Contains(a->persistentId));
    CHECK(FlightGlobals::VesselsLoadedView.empty());
    FlightGlobals::ClearAllVessels();
    CHECK(FlightGlobals::Vessels.empty());
    CHECK(removed == 2);
    GE::onFlightGlobalsAddVessel.Remove(hAdd);
    GE::onFlightGlobalsRemoveVessel.Remove(hRem);
}

TEST_CASE("WP-19a FlightGlobals.ClearToSave: the C# decision order (dead, ladder, about to crash, atmosphere, acceleration, moving, other kerbals on ladders)") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    Vessel* a = w.MakeVessel("a", kerbin, 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    Vessel* b = w.MakeVessel("b", kerbin, 750000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    FlightGlobals::ActiveVessel = a;
    a->situation = Vessel::Situations::ORBITING;
    a->geeForce = 0.0;
    GameSettings::QUICKSAVE_MINIMUM_ALTITUDE = 0.0f;
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::CLEAR);
    // FLYING near the ground -> about to crash (checked before the atmosphere rule); heightFromTerrain -1 is "unknown"
    a->situation = Vessel::Situations::FLYING;
    a->heightFromTerrain = 0.4f;
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH);
    a->heightFromTerrain = -1.0f;
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::NOT_IN_ATMOSPHERE);
    a->heightFromTerrain = 0.5f;  // max(0.5, QUICKSAVE_MINIMUM_ALTITUDE) is not exceeded by 0.5: not < 0.5
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::NOT_IN_ATMOSPHERE);
    a->situation = Vessel::Situations::SUB_ORBITAL;
    a->heightFromTerrain = 100.0f;
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::CLEAR);
    GameSettings::QUICKSAVE_MINIMUM_ALTITUDE = 500.0f;
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH);
    GameSettings::QUICKSAVE_MINIMUM_ALTITUDE = 0.0f;
    a->geeForce = 0.1;  // > 0.1 is required
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::CLEAR);
    a->geeForce = 0.1000001;
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::NOT_UNDER_ACCELERATION);
    a->Landed = true;                             // landed: g does not matter, the surface speed does
    a->srf_velocity = Vector3d(0.3, 0.0, 0.0);    // 0.3 * 0.3 is below 0.09000000357627869
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::CLEAR);
    a->srf_velocity = Vector3d(0.30000001, 0.0, 0.0);
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::NOT_WHILE_MOVING_OVER_SURFACE);
    a->srf_velocity = Vector3d();
    FakeEva eva;
    eva.ladder = true;
    b->vesselType = VesselType::EVA;
    b->evaController = &eva;
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::NOT_WHILE_ON_A_LADDER);  // another vessel's kerbal on a ladder
    eva.ladder = false;
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::CLEAR);
    a->vesselType = VesselType::EVA;
    a->evaController = &eva;
    eva.ladder = true;
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::NOT_WHILE_ON_A_LADDER);
    a->state = Vessel::State::DEAD;  // a dead active vessel is always clear
    CHECK(FlightGlobals::ClearToSave(false) == ClearToSaveStatus::CLEAR);
    a->state = Vessel::State::ACTIVE;
    a->evaController = nullptr;
    b->evaController = nullptr;
    CHECK(FlightGlobals::GetNotClearToSaveStatusReason(ClearToSaveStatus::NOT_IN_ATMOSPHERE, "x") == Localizer::Format("#autoLOC_7003248", {"x"}));
    CHECK(FlightGlobals::GetNotClearToSaveStatusReason(ClearToSaveStatus::CLEAR, "x") == Localizer::Format("#autoLOC_7003254", {"x"}));
}

TEST_CASE("WP-19a FlightGlobals.SetActiveVessel: refusals, events, packing, the unloaded-vessel path, ForceSetActiveVessel, dead vessels") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    Vessel* a = w.MakeVessel("a", kerbin, 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    Vessel* b = w.MakeVessel("b", kerbin, 750000.0, 0.0, true, true, OrbitDriver::UpdateMode::UPDATE);
    Vessel* far = w.MakeVessel("far", kerbin, 800000.0, 0.0, false, true);
    for (Vessel* v : {a, b, far}) v->situation = Vessel::Situations::ORBITING;
    std::vector<std::string> log;
    const auto hSw = decltype(GE::onVesselSwitching)::OnEvent::Lambda(
        &log, [&](Vessel* f, Vessel* t) { log.push_back("switching " + std::string(f != nullptr ? f->vesselName : "-") + ">" + t->vesselName); });
    const auto hCh = decltype(GE::onVesselChange)::OnEvent::Lambda(&log, [&](Vessel* v) { log.push_back("change " + v->vesselName); });
    const auto hRd = decltype(GE::OnFlightGlobalsReady)::OnEvent::Lambda(&log, [&](bool r) { log.push_back(r ? "ready" : "notready"); });
    const auto hUn = decltype(GE::onVesselSwitchingToUnloaded)::OnEvent::Lambda(
        &log, [&](Vessel* f, Vessel* t) { log.push_back("unloaded " + f->vesselName + ">" + t->vesselName); });
    GE::onVesselSwitching.Add(hSw);
    GE::onVesselChange.Add(hCh);
    GE::OnFlightGlobalsReady.Add(hRd);
    GE::onVesselSwitchingToUnloaded.Add(hUn);
    std::vector<std::string> messages;
    FlightGlobals::env.postScreenMessage = [&](const std::string& m) { messages.push_back(m); };
    std::vector<int> focus;
    FlightGlobals::env.startAndFocusVessel = [&](int i) { focus.push_back(i); };
    int saved = 0;
    FlightGlobals::env.savePersistent = [&] { saved++; };

    CHECK_FALSE(FlightGlobals::SetActiveVessel(nullptr));
    // the first vessel: no ClearToSave check (no active vessel)
    CHECK(FlightGlobals::SetActiveVessel(a));
    CHECK(FlightGlobals::ActiveVessel == a);
    CHECK(FlightGlobals::currentMainBody == kerbin);
    CHECK(a->state == Vessel::State::ACTIVE);
    CHECK(FlightGlobals::ActiveVesselView == &a->FlightView());
    CHECK_FALSE(FlightGlobals::ready);  // "notready" fired; FixedUpdate sets it again
    CHECK(log == std::vector<std::string>{"notready", "switching ->a", "change a"});
    CHECK_FALSE(FlightGlobals::SetActiveVessel(a));  // already active
    log.clear();
    // refused: the active vessel is in an atmosphere
    a->situation = Vessel::Situations::FLYING;
    CHECK_FALSE(FlightGlobals::SetActiveVessel(b));
    CHECK(messages.size() == 1);
    CHECK(messages[0] == Localizer::Format("#autoLOC_6002346"));
    CHECK(FlightGlobals::ActiveVessel == a);
    a->situation = Vessel::Situations::ORBITING;
    messages.clear();
    // refused: the target is not Owned
    b->GetDiscoveryInfo().SetLevel(DiscoveryLevels::Unowned);
    CHECK_FALSE(FlightGlobals::SetActiveVessel(b));
    CHECK(messages.size() == 1);
    CHECK(messages[0] == Localizer::Format("#autoLOC_6002352"));
    b->GetDiscoveryInfo().SetLevel(DiscoveryLevels::Owned);
    log.clear();
    // a packed loaded target: every vessel goes on rails before the switch
    int onRails = 0;
    Vessel::env.precalcGoOnRails = [&](Vessel&) { onRails++; };
    CHECK(FlightGlobals::SetActiveVessel(b));
    CHECK(FlightGlobals::ActiveVessel == b);
    CHECK(a->state == Vessel::State::INACTIVE);
    CHECK(b->state == Vessel::State::ACTIVE);
    CHECK(onRails == 1);  // a was unpacked: GoOnRails; b and far were already packed
    CHECK(a->packed);
    CHECK(log == std::vector<std::string>{"notready", "switching a>b", "change b"});
    // a vessel that is not loaded: the game saves, restarts the flight scene focused on it and does not switch here
    log.clear();
    CHECK(FlightGlobals::SetActiveVessel(far));
    CHECK(FlightGlobals::ActiveVessel == b);
    CHECK(log == std::vector<std::string>{"notready", "unloaded b>far"});
    CHECK(saved == 1);
    CHECK(focus == std::vector<int>{2});
    CHECK(b->state == Vessel::State::INACTIVE);  // MakeInactive ran before the early return
    // force skips the checks
    b->state = Vessel::State::ACTIVE;
    b->situation = Vessel::Situations::FLYING;
    CHECK(FlightGlobals::ForceSetActiveVessel(a));
    CHECK(FlightGlobals::ActiveVessel == a);
    // dead vessels are removed after a switch (ClearDeadVessels)
    far->state = Vessel::State::DEAD;
    b->situation = Vessel::Situations::ORBITING;
    CHECK(FlightGlobals::SetActiveVessel(b));
    CHECK(std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), far) == FlightGlobals::Vessels.end());
    GE::onVesselSwitching.Remove(hSw);
    GE::onVesselChange.Remove(hCh);
    GE::OnFlightGlobalsReady.Remove(hRd);
    GE::onVesselSwitchingToUnloaded.Remove(hUn);
}

TEST_CASE("WP-19a FlightGlobals queries: bodies by name / index, home body, comet perturber, main body by SOI, altitude, pressure, up axis, speed modes") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    CelestialBody* mun = w.Body("Mun");
    CHECK(FlightGlobals::GetBodyByName("Kerbin") == kerbin);
    CHECK(FlightGlobals::GetBodyByName("Kerbin") == kerbin);  // cached
    CHECK(FlightGlobals::GetBodyByName("Nowhere") == nullptr);
    CHECK(FlightGlobals::GetBodyIndex(mun) == static_cast<int>(std::find(FlightGlobals::Bodies().begin(), FlightGlobals::Bodies().end(), mun) - FlightGlobals::Bodies().begin()));
    CHECK(FlightGlobals::GetBodyIndex(nullptr) == -1);
    CHECK(FlightGlobals::GetHomeBody() == kerbin);
    CHECK(FlightGlobals::GetHomeBodyName() == "Kerbin");
    CHECK(FlightGlobals::GetHomeBodyDisplayName() == kerbin->displayName());
    CHECK(FlightGlobals::GetHomeBodyIndex() == FlightGlobals::GetBodyIndex(kerbin));
    CelestialBody* perturber = FlightGlobals::GetCometPerturber();
    CHECK(perturber != nullptr);
    CHECK(perturber->isCometPerturber);
    CHECK(FlightGlobals::GetCometPerturberIndex() == FlightGlobals::GetBodyIndex(perturber));
    CHECK(FlightGlobals::GetCometPerturberName() == perturber->name());

    // main body by SOI: the Sun far away, Kerbin inside its SOI, the Mun inside its own
    CHECK(FlightGlobals::getMainBody(kerbin->position + Vector3d(1000.0, 0.0, 0.0)) == kerbin);
    CHECK(FlightGlobals::getMainBody(mun->position + Vector3d(0.0, 0.0, 500.0)) == mun);
    CHECK(FlightGlobals::getMainBody(kerbin->position + Vector3d(kerbin->sphereOfInfluence * 1.01, 0.0, 0.0)) == w.planetarium.Sun);
    // altitudes, up axis, pressure, temperature, density
    const Vector3d p = kerbin->position + Vector3d(kerbin->Radius + 1000.0, 0.0, 0.0);
    CHECK(FlightGlobals::getAltitudeAtPos(p, kerbin) == Vector3d::Distance(p, kerbin->position) - kerbin->Radius);
    CHECK(FlightGlobals::getAltitudeAtPos(p) == FlightGlobals::getAltitudeAtPos(p, kerbin));
    CHECK(FlightGlobals::getAltitudeAtPos(Vector3f(p.ToVector3f()), kerbin) == static_cast<float>(FlightGlobals::getAltitudeAtPos(Vector3d(p.ToVector3f()), kerbin)));
    CHECK(FlightGlobals::GetSqrAltitude(p, *kerbin) == (p - kerbin->position).sqrMagnitude() - kerbin->Radius * kerbin->Radius);
    CHECK(FlightGlobals::getUpAxis(*kerbin, p).x == doctest::Approx(1.0));
    CHECK(FlightGlobals::getUpAxis(p).x == doctest::Approx(1.0));
    CHECK(FlightGlobals::getStaticPressure(0.0, kerbin) == kerbin->GetPressure(0.0));
    CHECK(FlightGlobals::getStaticPressure(0.0, kerbin) > 100.0);
    CHECK(FlightGlobals::getStaticPressure(p, *kerbin) == kerbin->GetPressure(1000.0));
    CHECK(FlightGlobals::getStaticPressure(p) == kerbin->GetPressure(FlightGlobals::getAltitudeAtPos(p, kerbin)));
    CHECK(FlightGlobals::getStaticPressure(0.0, mun) == 0.0);
    CHECK(FlightGlobals::getExternalTemperature(0.0, kerbin) == kerbin->GetTemperature(0.0));
    CHECK(FlightGlobals::getExternalTemperature(0.0, mun) == 0.0);
    CHECK(FlightGlobals::getExternalTemperature(p, kerbin) == kerbin->GetTemperature(1000.0));
    CHECK(FlightGlobals::getAtmDensity(101.325, 300.0, kerbin) == kerbin->GetDensity(101.325, 300.0));
    CHECK(FlightGlobals::getAtmDensity(0.0, 0.0, mun) == 0.0);
    CHECK(FlightGlobals::getGeeForceAtPosition(p).x < 0.0);  // pulls towards the body
    CHECK(FlightGlobals::vacuumTemperature() == 0.0f);

    // speed modes: Surface -> (Target is skipped without a target) -> Orbit -> Surface ...
    std::vector<int> modes;
    const auto h = decltype(GE::onSetSpeedMode)::OnEvent::Lambda(&modes, [&](FlightGlobals_SpeedDisplayModes m) { modes.push_back(static_cast<int>(m)); });
    GE::onSetSpeedMode.Add(h);
    FlightGlobals::SetSpeedMode(FlightGlobals_SpeedDisplayModes::Surface);
    FlightGlobals::CycleSpeedModes();
    FlightGlobals::CycleSpeedModes();
    FlightGlobals::CycleSpeedModes();
    CHECK(modes == std::vector<int>{1, 0, 1, 0});
    GE::onSetSpeedMode.Remove(h);
    // RefFrameIsRotating reads currentMainBody
    FlightGlobals::currentMainBody = kerbin;
    kerbin->rotates = true;
    kerbin->inverseRotation = true;
    CHECK(FlightGlobals::RefFrameIsRotating());
    kerbin->rotates = false;
    CHECK_FALSE(FlightGlobals::RefFrameIsRotating());
    kerbin->inverseRotation = false;
    kerbin->rotates = true;
}

TEST_CASE("WP-19a FlightGlobals tick: FixedUpdate sets ready and fires OnFlightGlobalsReady; UpdateInformation; the active vessel properties; target delta") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    Vessel* a = w.MakeVessel("a", kerbin, 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    Vessel* b = w.MakeVessel("b", kerbin, 710000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    FlightGlobals::ActiveVessel = a;
    a->situation = Vessel::Situations::ORBITING;
    a->vesselTransform.position = Vector3f(1000.0f, 2000.0f, 3000.0f);
    b->vesselTransform.position = Vector3f(1500.0f, 2500.0f, 2000.0f);
    a->externalTemperature = 300.0;
    a->atmDensity = 0.5;
    a->altitude = 12345.0;
    a->srf_velocity = Vector3d(1.0, 2.0, 2.0);
    a->srfSpeed = 3.0;
    a->staticPressurekPa = 7.5;
    bool ready = false;
    const auto hR = decltype(GE::OnFlightGlobalsReady)::OnEvent::Lambda(&ready, [&](bool r) { ready = r; });
    GE::OnFlightGlobalsReady.Add(hR);
    CHECK_FALSE(FlightGlobals::ready);
    FlightGlobals::FixedUpdate();
    CHECK(FlightGlobals::ready);
    CHECK(ready);
    CHECK(FlightGlobals::currentMainBody == kerbin);
    CHECK(FlightGlobals::ship_temp == 300.0);
    CHECK(FlightGlobals::ship_dns == 0.5);
    CHECK(FlightGlobals::ship_altitude() == 12345.0);
    CHECK(FlightGlobals::ship_srfSpeed() == 3.0);
    CHECK(FlightGlobals::ship_srfVelocity().y == 2.0);
    CHECK(FlightGlobals::ship_stP() == 7.5);
    CHECK(FlightGlobals::getStaticPressure() == 7.5);
    CHECK(FlightGlobals::ship_position().x == 1000.0);
    CHECK(FlightGlobals::activeTarget() == a->rootPart);
    // no camera: the vessel position stands in; camera_altitude is the distance to the body minus its radius
    CHECK(FlightGlobals::camera_altitude == Vector3d::Distance(Vector3d(1000.0, 2000.0, 3000.0), kerbin->position) - kerbin->Radius);
    // a target: the delta is target - active reference position (floats)
    std::vector<std::string> messages;
    FlightGlobals::env.postScreenMessage = [&](const std::string& m) { messages.push_back(m); };
    b->GetDiscoveryInfo().SetLevel(DiscoveryLevels::Owned);
    FlightGlobals::SetVesselTarget(b);
    CHECK(FlightGlobals::VesselTarget() == b);
    CHECK(a->targetObject == b);
    CHECK(messages.size() == 1);
    CHECK(FlightGlobals::vesselTargetMode == VesselTargetModes::DirectionAndVelocity);
    FlightGlobals::UpdateInformation(true);
    CHECK(FlightGlobals::vesselTargetDelta.x == 500.0f);
    CHECK(FlightGlobals::vesselTargetDelta.z == -1000.0f);
    CHECK(FlightGlobals::ship_tgtSpeed >= 0.0);
    // the target is the active vessel itself and not targetable: cleared
    FlightGlobals::SetVesselTarget(a);
    FlightGlobals::UpdateInformation(false);
    CHECK(FlightGlobals::VesselTarget() == nullptr);
    // speed mode: Surface -> Orbit above radius * navballSwitchRadiusMult
    FlightGlobals::SetSpeedMode(FlightGlobals_SpeedDisplayModes::Surface);
    a->altitude = kerbin->Radius * kerbin->navballSwitchRadiusMult * 1.01;
    FlightGlobals::Update();
    CHECK(FlightGlobals::speedDisplayMode() == FlightGlobals_SpeedDisplayModes::Orbit);
    a->altitude = kerbin->Radius * kerbin->navballSwitchRadiusMultLow * 0.99;
    FlightGlobals::Update();
    CHECK(FlightGlobals::speedDisplayMode() == FlightGlobals_SpeedDisplayModes::Surface);
    CHECK(FlightGlobals::GetDisplaySpeed() == 3.0);
    // Start(): overrideOrbit for one second (Invoke), the vessel events become StandardModification
    FlightGlobals::Start();
    CHECK(FlightGlobals::overrideOrbit);
    FlightGlobals::RunDelayedCalls(Time::time + 0.5f);
    CHECK(FlightGlobals::overrideOrbit);
    FlightGlobals::RunDelayedCalls(Time::time + 1.0f);
    CHECK_FALSE(FlightGlobals::overrideOrbit);
    int mods = 0;
    const auto hM = decltype(GE::onVesselStandardModification)::OnEvent::Lambda(&mods, [&](Vessel*) { mods++; });
    GE::onVesselStandardModification.Add(hM);
    GE::onVesselWasModified.Fire(a);
    GE::onVesselPartCountChanged.Fire(b);
    CHECK(mods == 2);
    GE::onVesselStandardModification.Remove(hM);
    GE::OnFlightGlobalsReady.Remove(hR);
}

// ================================================================================================ OrbitPhysicsManager

TEST_CASE("WP-19a OrbitPhysicsManager: singleton, Start waits for FlightGlobals.ready, HoldVesselUnpack counts FixedUpdates, checkReferenceFrame decision chain") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    CelestialBody* mun = w.Body("Mun");
    OrbitPhysicsManager::HoldVesselUnpack(5);  // no instance: nothing
    CHECK(OrbitPhysicsManager::DominantBody() == nullptr);
    OrbitPhysicsManager opm;
    CHECK(OrbitPhysicsManager::Instance() == &opm);
    {
        OrbitPhysicsManager second;  // Awake: a second instance destroys itself
        CHECK_FALSE(second.Alive());
        CHECK(OrbitPhysicsManager::Instance() == &opm);
    }
    CHECK(OrbitPhysicsManager::Instance() == &opm);
    CHECK_FALSE(opm.Start());  // FlightGlobals.ready is false
    FlightGlobals::ready = true;
    FlightGlobals::currentMainBody = kerbin;
    CHECK(opm.Start());
    CHECK(OrbitPhysicsManager::DominantBody() == kerbin);
    // hold: HoldVesselUnpack(3) keeps holdVesselUnpack through 3 decrements and drops it on the next FixedUpdate
    Vessel* a = w.MakeVessel("a", kerbin, 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    FlightGlobals::ActiveVessel = a;
    a->vesselTransform.position = (kerbin->position + Vector3d(kerbin->Radius + 70.0, 0.0, 0.0)).ToVector3f();
    OrbitPhysicsManager::HoldVesselUnpack(3);
    CHECK(opm.holdVesselUnpack);
    CHECK(opm.releaseUnpackIn() == 3);
    int held = 0;
    for (int i = 0; i < 10 && opm.holdVesselUnpack; i++) {
        opm.FixedUpdate();
        held++;
    }
    CHECK(held == 4);
    CHECK_FALSE(opm.holdVesselUnpack);
    // the decision chain: the active vessel is below the rotating-frame threshold -> rotating frame; above -> inertial
    kerbin->rotates = true;
    kerbin->inverseRotation = false;
    kerbin->inverseRotThresholdAltitude = 15000.0f;
    std::vector<std::string> log;
    const auto hF = decltype(GE::onRotatingFrameTransition)::OnEvent::Lambda(&log, [&](GE::HostTargetAction<CelestialBody*, bool> d) {
        log.push_back(std::string("frame ") + d.host->bodyName + (d.target ? " rotating" : " inertial"));
    });
    GE::onRotatingFrameTransition.Add(hF);
    opm.checkReferenceFrame();
    CHECK(kerbin->inverseRotation);
    CHECK(log == std::vector<std::string>{"frame Kerbin rotating"});
    opm.checkReferenceFrame();  // nothing changes
    CHECK(log.size() == 1);
    a->vesselTransform.position = (kerbin->position + Vector3d(kerbin->Radius + 16000.0, 0.0, 0.0)).ToVector3f();
    opm.checkReferenceFrame();
    CHECK_FALSE(kerbin->inverseRotation);
    CHECK(log.size() == 2);
    // a body that does not rotate never changes
    kerbin->rotates = false;
    a->vesselTransform.position = (kerbin->position + Vector3d(kerbin->Radius + 70.0, 0.0, 0.0)).ToVector3f();
    opm.checkReferenceFrame();
    CHECK_FALSE(kerbin->inverseRotation);
    kerbin->rotates = true;
    // the dominant body follows the active vessel's reference body (an SOI change)
    std::vector<std::pair<std::string, std::string>> dom;
    const auto hD = decltype(GE::onDominantBodyChange)::OnEvent::Lambda(&dom, [&](GE::FromToAction<CelestialBody*, CelestialBody*> d) {
        dom.emplace_back(d.from->bodyName, d.to->bodyName);
    });
    GE::onDominantBodyChange.Add(hD);
    a->orbitDriver->orbit->referenceBody = mun;
    mun->rotates = false;
    opm.checkReferenceFrame();
    CHECK(opm.dominantBody == mun);
    CHECK(dom.size() == 1);
    CHECK(dom[0].first == "Mun");  // the C# fires (dominantBody, body) AFTER assigning: both are the new body
    CHECK(dom[0].second == "Mun");
    // FixedUpdate does nothing before FlightGlobals.ready / during overrideOrbit
    FlightGlobals::overrideOrbit = true;
    a->orbitDriver->orbit->referenceBody = kerbin;
    opm.FixedUpdate();
    CHECK(opm.dominantBody == mun);
    FlightGlobals::overrideOrbit = false;
    opm.FixedUpdate();
    CHECK(opm.dominantBody == kerbin);
    // seams (WP-19b) replace the state-only actions
    int rot = 0, domCalls = 0;
    OrbitPhysicsManager::env.setRotatingFrame = [&](OrbitPhysicsManager&, bool) { rot++; };
    OrbitPhysicsManager::env.setDominantBody = [&](OrbitPhysicsManager&, CelestialBody*) { domCalls++; };
    OrbitPhysicsManager::SetRotatingFrame(true);
    OrbitPhysicsManager::SetDominantBody(mun);
    CHECK(rot == 1);
    CHECK(domCalls == 1);
    GE::onRotatingFrameTransition.Remove(hF);
    GE::onDominantBodyChange.Remove(hD);
}

// ================================================================================================ VesselPrecalculate on real vessels

TEST_CASE("WP-19a VesselPrecalculate.MainPhysics on rails: the orbit is advanced to UT, CoMD = body + orbit position, kill checks, flags") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    Vessel* other = w.MakeVessel("pad", kerbin, kerbin->Radius + 70.0, 0.0, true, false, OrbitDriver::UpdateMode::IDLE);
    other->Landed = true;
    FlightGlobals::ActiveVessel = other;
    FlightGlobals::ready = true;
    Vessel* v = w.MakeVessel("probe", kerbin, 800000.0, 0.2, false, true);
    Orbit ref(0.0, 0.2, 800000.0, 0.0, 0.0, 0.0, Planetarium::GetUniversalTime(), kerbin);
    const double ut0 = Planetarium::GetUniversalTime();
    for (int i = 1; i <= 50; i++) {
        w.planetarium.time = ut0 + 0.02 * i;
        v->precalc.FixedUpdate();
        const double ut = Planetarium::GetUniversalTime();
        ref.UpdateFromUT(ut);
        CHECK(v->orbit().pos.x == ref.pos.x);
        CHECK(v->orbit().pos.y == ref.pos.y);
        CHECK(v->orbit().pos.z == ref.pos.z);
        const Vector3d pos = ref.pos.xzy();
        CHECK(v->orbitDriver->pos.x == pos.x);
        CHECK(v->CoMD.x == (kerbin->position + v->orbitDriver->pos).x);
        CHECK(v->CoMD.z == (kerbin->position + v->orbitDriver->pos).z);
        CHECK(v->velocityD.IsZero());  // no rigid body behind an unloaded vessel
    }
    CHECK(v->precalc.packedInFixed);
    CHECK(v->precalc.firstStatsRunComplete);
    CHECK(v->precalc.ranFixedThisFrame);
    CHECK(v->precalc.physStatsNotDoneInUpdate);
    CHECK(v->altitude == doctest::Approx(Vector3d::Distance(v->CoMD, kerbin->position) - kerbin->Radius).epsilon(1e-9));
    CHECK(v->state != Vessel::State::DEAD);
    CHECK(v->GetWorldPos3D().x == v->CoMD.x);
    CHECK(v->graviticAcceleration.IsZero());  // calculateGravity only matters for loaded vessels: gAccel stays zero
    // an on-rails vessel in the atmosphere is destroyed (Vessel.CheckKill): perigee below the atmosphere top
    Vessel* low = w.MakeVessel("low", kerbin, kerbin->Radius + 20000.0, 0.0, false, true);
    low->precalc.FixedUpdate();
    CHECK(low->state == Vessel::State::DEAD);
    CHECK(std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), low) == FlightGlobals::Vessels.end());
}

TEST_CASE("WP-19a VesselPrecalculate.SetLandedPosRot and the packed landed CoM (the WP-14 deviation 1 resolved)") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    Vessel* v = w.MakeVessel("landed", kerbin, kerbin->Radius + 70.0, 0.0, false, true, OrbitDriver::UpdateMode::IDLE);
    v->Landed = true;
    v->latitude = -0.0972;
    v->longitude = -74.5577;
    v->altitude = 70.0;
    v->srfRelRotation = Quaternionf(0.1f, 0.2f, 0.3f, 0.9f);
    v->localCoM = Vector3f(0.0f, 0.5f, 0.0f);
    FlightGlobals::ready = true;
    FlightGlobals::ActiveVessel = v;
    v->precalc.FixedUpdate();
    const QuaternionD wantRot(kerbin->rotation.ToQuaternionf() * v->srfRelRotation);
    const Vector3d wantPos = kerbin->GetWorldSurfacePosition(v->latitude, v->longitude, v->altitude);
    CHECK(v->precalc.worldSurfaceRot.x == wantRot.x);
    CHECK(v->precalc.worldSurfaceRot.w == wantRot.w);
    CHECK(v->precalc.worldSurfacePos.x == wantPos.x);
    CHECK(v->precalc.worldSurfacePos.y == wantPos.y);
    CHECK(v->precalc.worldSurfacePos.z == wantPos.z);
    CHECK(v->vesselTransform.position.x == wantPos.ToVector3f().x);  // Vessel.SetPosition on an unloaded vessel: the float position
    const Vector3d com = wantPos + wantRot * Vector3d(v->localCoM);
    CHECK(v->CoMD.x == com.x);
    CHECK(v->CoMD.y == com.y);
    CHECK(v->CoMD.z == com.z);
    // not landed: the rails branch
    v->Landed = false;
    v->orbitDriver->updateMode = OrbitDriver::UpdateMode::UPDATE;
    v->precalc.railsSetPosRot = true;
    v->precalc.FixedUpdate();
    CHECK(v->CoMD.x == (kerbin->position + v->orbitDriver->pos).x);
    // railsSetPosRot == false: the landed placement is skipped
    v->Landed = true;
    v->orbitDriver->updateMode = OrbitDriver::UpdateMode::IDLE;
    v->vesselTransform.position = Vector3f(1.0f, 2.0f, 3.0f);
    v->precalc.railsSetPosRot = false;
    v->precalc.FixedUpdate();
    CHECK(v->vesselTransform.position.x == 1.0f);
}

TEST_CASE("WP-19a VesselPrecalculate easing: the gravity multiplier ramps 0 -> 1 in 16 ticks after rails -> physics (reference sequence), events, control lock") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    Vessel* v = w.MakeVessel("v", kerbin, 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    FlightGlobals::ActiveVessel = v;
    v->orbitDriver->lastMode = OrbitDriver::UpdateMode::IDLE;  // the previous tick the driver was IDLE: rails -> physics
    v->CoMD = kerbin->position + Vector3d(700000.0, 0.0, 0.0);
    v->velocityD = Vector3d(0.0, 0.0, 2000.0);
    v->precalc.physStatsNotDoneInUpdate = false;  // Update() ran
    int started = 0, stopped = 0;
    const auto hS = decltype(GE::onPhysicsEaseStart)::OnEvent::Lambda(&started, [&](Vessel*) { started++; });
    const auto hE = decltype(GE::onPhysicsEaseStop)::OnEvent::Lambda(&stopped, [&](Vessel*) { stopped++; });
    GE::onPhysicsEaseStart.Add(hS);
    GE::onPhysicsEaseStop.Add(hE);
    const auto& seq = Ref().at("easing").at("sequence");
    CHECK(v->gravityMultiplier == 1.0);
    for (const auto& step : seq) {
        v->precalc.physStatsNotDoneInUpdate = false;
        v->precalc.FixedUpdate();
        v->orbitDriver->lastMode = OrbitDriver::UpdateMode::IDLE;
        CHECK(v->gravityMultiplier == D(step.at("gm")));
        CHECK(v->precalc.isEasingGravity() == step.at("easing").get<bool>());
    }
    CHECK(started == 1);
    CHECK(stopped == 1);
    CHECK(v->gravityMultiplier == 1.0);
    CHECK(v->precalc.easingLockOn);  // the active vessel gets the "PhysicsEase" control lock and the screen message
    CHECK(v->precalc.easingMessageSeconds == static_cast<float>(1.0 / 0.0625 * 0.02 + 0.5));
    // one more tick: no new easing (lastMode is IDLE but easing needs !easing && PHYSICS_EASE: it starts again!)
    // docking completion starts it too (easeDocking), without the lastMode condition
    v->orbitDriver->lastMode = OrbitDriver::UpdateMode::TRACK_Phys;
    v->precalc.easeDocking = true;
    v->precalc.physStatsNotDoneInUpdate = false;
    v->precalc.FixedUpdate();
    CHECK(started == 2);
    CHECK(v->gravityMultiplier == 0.0625);
    // packing during easing stops it (StopEasing, multiplier back to 1)
    v->packed = true;
    v->precalc.CalculateGravity();
    CHECK_FALSE(v->precalc.isEasingGravity());
    CHECK(stopped == 2);
    CHECK(v->gravityMultiplier == 1.0);
    GE::onPhysicsEaseStart.Remove(hS);
    GE::onPhysicsEaseStop.Remove(hE);
}

TEST_CASE("WP-19a VesselRanges: a new Vessel copies PhysicsGlobals.VesselRangesDefault (Physics.cfg); the situation picks the range set (DOCKED and ORBITING share `orbit`)") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    Vessel* a = w.MakeVessel("a", kerbin);
    const VesselRanges& def = PhysicsGlobals::Instance()->VesselRangesDefault;
    CHECK(a->vesselRanges.flying.pack == def.flying.pack);
    CHECK(a->vesselRanges.flying.pack == 25000.0f);  // Physics.cfg VesselRanges (the game's values)
    CHECK(a->vesselRanges.landed.unpack == 200.0f);
    CHECK(a->vesselRanges.subOrbital.unload == 15000.0f);
    // the global default changes new vessels only
    PhysicsGlobals::Instance()->VesselRangesDefault.orbit.pack = 123.0f;
    Vessel* b = w.MakeVessel("b", kerbin);
    CHECK(b->vesselRanges.orbit.pack == 123.0f);
    CHECK(a->vesselRanges.orbit.pack == 350.0f);
    PhysicsGlobals::Instance()->VesselRangesDefault.orbit.pack = 350.0f;
    // GetSituationRanges: every situation but the five named ones (and ESCAPING) uses `orbit`
    CHECK(&a->vesselRanges.GetSituationRanges(Vessel::Situations::DOCKED) == &a->vesselRanges.orbit);
    CHECK(&a->vesselRanges.GetSituationRanges(Vessel::Situations::ORBITING) == &a->vesselRanges.orbit);
    CHECK(&a->vesselRanges.GetSituationRanges(Vessel::Situations::ESCAPING) == &a->vesselRanges.escaping);
    CHECK(&a->vesselRanges.GetSituationRanges(Vessel::Situations::SUB_ORBITAL) == &a->vesselRanges.subOrbital);
    // the obsolete Vessel.distance*Threshold accessors read / write the orbit and landed sets (Vessel.cs:804-868)
    a->set_distancePackThreshold(777.0f);
    CHECK(a->vesselRanges.orbit.pack == 777.0f);
    CHECK(a->distancePackThreshold() == 777.0f);
    a->set_distanceLandedUnpackThreshold(55.0f);
    CHECK(a->vesselRanges.landed.unpack == 55.0f);
}
