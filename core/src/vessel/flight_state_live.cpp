// WP-19c: the live half of FlightState (FlightState.cs:33 the capture of the running flight scene, :164 Load) and Vessel.LandedInStockLaunchSite.
// The data half (FlightState(ConfigNode, Game), Save, ContainsFlightID) is WP-04's src/save/game.cpp.
#include <algorithm>
#include <memory>

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/launch_site.hpp"
#include "ksp/space_center/psystem_setup.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {
// FlightState.protoVessels.Add(vessel.BackupVessel()): the list and the vessel share the object. A ProtoVessel that is not shared-owned (a caller's
// own object) cannot be shared: the list gets a copy through its node (same text, same crew from HighLogic.CurrentGame's roster).
std::shared_ptr<ProtoVessel> Shared(ProtoVessel* pv) {
    if (pv == nullptr) return nullptr;
    if (std::shared_ptr<ProtoVessel> s = pv->weak_from_this().lock()) return s;
    ConfigNode node("VESSEL");
    pv->Save(node);
    auto copy = std::make_shared<ProtoVessel>(node, SaveEnvironment::Current().currentGame);
    copy->vesselRef = pv->vesselRef;
    return copy;
}
}  // namespace

// FlightState.cs:33
FlightState::FlightState() {
    const SaveEnvironment& env = SaveEnvironment::Current();
    file_version_major = Versioning::version_major;
    file_version_minor = Versioning::version_minor;
    file_version_revision = Versioning::Revision;
    compatible = true;
    try {
        universalTime = SaveUniversalTime();
    } catch (const NullReferenceException&) {
        universalTime = 0.0;  // no Planetarium and no current game (kspcore tools; the game always has one of them here)
    }
    mapViewFilterState = env.mapViewFilteringVesselTypeFilter;  // MapViewFiltering.GetFilterState()
    std::vector<Vessel*> list;
    int count = static_cast<int>(FlightGlobals::Vessels.size());
    while (count-- > 0) {
        Vessel* vessel = FlightGlobals::Vessels[static_cast<size_t>(count)];
        if (vessel == nullptr) continue;
        if (vessel->state == Vessel::State::DEAD) {
            log::Warning("[FlightPersistence]: Vessel " + vessel->GetDisplayName() + " not saved because it was dead.");
            continue;
        }
        if (GameSettings::DECLUTTER_KSC && !vessel->isCommandable() && !vessel->isPersistent() && vessel->situation == Vessel::Situations::LANDED) {
            if (vessel->LandedInKSC()) vessel->SetAutoClean("it was debris cluttering up KSC");
            if (vessel->LandedInStockLaunchSite()) vessel->SetAutoClean("it was debris cluttering up a LaunchSite");
        }
        list.push_back(vessel);
    }
    if (GameSettings::MAX_VESSELS_BUDGET != -1) {
        int count2 = static_cast<int>(list.size());
        while (count2-- > 0 && static_cast<int>(list.size()) > GameSettings::MAX_VESSELS_BUDGET) {
            Vessel* v = list[static_cast<size_t>(count2)];
            if (!v->isPersistent() && !v->isCommandable()) {
                log::Warning("[Flight Persistence]: Too many vessels in scene - skipping save for " + v->GetDisplayName());
                list.erase(list.begin() + count2);
            }
        }
    }
    std::vector<const ProtoVessel*> backedUp;  // the objects BackupVessel returned (the list may hold copies, see Shared)
    int count3 = static_cast<int>(list.size());
    while (count3-- > 0) {
        ProtoVessel* pv = list[static_cast<size_t>(count3)]->BackupVessel();
        backedUp.push_back(pv);
        protoVessels.push_back(Shared(pv));
    }
    activeVesselIdx = 0;
    if (FlightGlobals::ready) {
        // protoVessels.IndexOf(FlightGlobals.ActiveVessel.protoVessel) (-1 when the active vessel was not captured)
        if (FlightGlobals::ActiveVessel == nullptr) throw NullReferenceException("FlightGlobals.ActiveVessel is null");
        const auto it = std::find(backedUp.begin(), backedUp.end(), FlightGlobals::ActiveVessel->protoVessel);
        activeVesselIdx = it == backedUp.end() ? -1 : static_cast<int>(it - backedUp.begin());
    }
    log::Info("Flight State Captured");
}

// FlightState.cs:164
void FlightState::Load() {
    if (!compatible) return;
    for (size_t i = 0; i < protoVessels.size(); i++) {
        // ProtoVessel.Load(this): `new GameObject().AddComponent<Vessel>()`, the scene keeps the object (Vessel::SpawnedVessels, freed by
        // Vessel::DestroyImmediate like Unity's DestroyImmediate); a vessel with missing parts is not created (nullptr)
        std::unique_ptr<Vessel> created = protoVessels[i]->Load(this, nullptr);
        if (created != nullptr) Vessel::SpawnedVessels().push_back(std::move(created));
    }
    Planetarium::SetUniversalTime(universalTime);
    // MapViewFiltering.cs:138 LoadFilterState: the Serenity bit (0x4000) is dropped without Breaking Ground
    SaveEnvironment& env = SaveEnvironment::Current();
    if ((static_cast<uint32_t>(mapViewFilterState) & 0x4000u) != 0 && !env.IsExpansionInstalled("Serenity"))
        env.mapViewFilteringVesselTypeFilter = mapViewFilterState ^ 0x4000;
    else
        env.mapViewFilteringVesselTypeFilter = mapViewFilterState;
}

// Vessel.cs:617 (PSystemSetup.Instance.StockLaunchSites; a scene without PSystemSetup has none: false, where the C# throws
// NullReferenceException -- the game always has the instance in a flight scene)
bool Vessel::LandedInStockLaunchSite() const {
    const std::string& text = loaded ? landedAt : protoVessel->landedAt;
    if (PSystemSetup::Instance == nullptr) return false;
    for (const auto& site : PSystemSetup::Instance->stocklaunchsites)
        if (site != nullptr && text.find(site->name) != std::string::npos) return true;
    return false;
}

}  // namespace ksp
