// SaveEnvironment, the FlightGlobals persistentId registry, ROCManager's removed-ROC list, Versioning strings;
// see include/ksp/save/save_environment.hpp.
#include "ksp/save/save_environment.hpp"

#include <chrono>
#include <random>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/save/game.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

using GE = events::GameEvents;

// =============================================== ROCManagerRemovedROCs =============================================

// ROCManager.cs:359
void ROCManagerRemovedROCs::SaveRemoveROCs(ConfigNode& node) const {
    if (!hasList) return;
    for (int id : removedROCs) node.AddValue("ROCId", static_cast<int32_t>(id));
}

// ROCManager.cs:370
void ROCManagerRemovedROCs::LoadRemovedROCs(const ConfigNode& node) {
    removedROCs.clear();
    hasList = true;
    for (const std::string& s : node.GetValuesList("ROCId")) {
        int32_t result = -1;
        if (dotnet::TryParseInt32(s, result) &&
            std::find(removedROCs.begin(), removedROCs.end(), result) == removedROCs.end())
            removedROCs.push_back(result);
    }
}

// ROCManager.cs:384
bool ROCManagerRemovedROCs::ROCRemoved(int rocId) {
    hasList = true;
    return std::find(removedROCs.begin(), removedROCs.end(), rocId) != removedROCs.end();
}

// =============================================== PersistentIdRegistry ==============================================

// FlightGlobals.cs:473
uint32_t PersistentIdRegistry::GetUniquepersistentId() const {
    const SaveEnvironment& env = SaveEnvironment::Current();
    uint32_t hashCode = static_cast<uint32_t>(env.NewGuidHashCode());
    if (!fetch) return hashCode;
    while (persistentLoadedPartIds.Contains(hashCode) || persistentUnloadedPartIds.Contains(hashCode) ||
           persistentVesselIds.Contains(hashCode) || hashCode == 0)
        hashCode = static_cast<uint32_t>(env.NewGuidHashCode());
    return hashCode;
}

// FlightGlobals.cs:487
uint32_t PersistentIdRegistry::CheckVesselpersistentId(uint32_t persistentId, const void* vessel, bool removeOldId,
                                                       bool addNewId) {
    if (persistentId == 0) {
        persistentId = GetUniquepersistentId();
        if (!fetch) return persistentId;
        if (addNewId) persistentVesselIds.Add(persistentId, vessel);
        return persistentId;
    }
    if (!fetch) throw NullReferenceException("FlightGlobals.FindVessel: no FlightGlobals instance");
    const void* vessel2 = nullptr;
    if (persistentVesselIds.TryGetValue(persistentId, vessel2)) {
        if (vessel2 == nullptr || vessel == nullptr || vessel2 != vessel) {
            const uint32_t num = persistentId;
            persistentId = GetUniquepersistentId();
            if (removeOldId || addNewId) {
                if (removeOldId) persistentVesselIds.Remove(num);
                if (addNewId) persistentVesselIds.Add(persistentId, vessel);
                GE::onVesselPersistentIdChanged.Fire(num, persistentId);
                log::Info("[FlightGlobals]: Vessel persistentId changed from " + dotnet::FormatUInt64(num) + " to " +
                          dotnet::FormatUInt64(persistentId) + ".");
            }
        }
    } else {
        persistentVesselIds.Add(persistentId, vessel);
    }
    return persistentId;
}

// FlightGlobals.cs:604
uint32_t PersistentIdRegistry::CheckProtoPartSnapShotpersistentId(uint32_t persistentId, ProtoPartSnapshot* partSnapshot,
                                                                  bool removeOldId, bool addNewId,
                                                                  uint32_t vesselPersistentId) {
    if (persistentId == 0) {
        persistentId = GetUniquepersistentId();
        if (!fetch) return persistentId;
        if (addNewId) persistentUnloadedPartIds.Add(persistentId, partSnapshot);
        return persistentId;
    }
    if (!fetch) throw NullReferenceException("FlightGlobals.FindUnloadedPart: no FlightGlobals instance");
    ProtoPartSnapshot* partout = nullptr;
    if (persistentUnloadedPartIds.TryGetValue(persistentId, partout)) {
        if (partout == nullptr || partSnapshot == nullptr || partout != partSnapshot) {
            const uint32_t num = persistentId;
            persistentId = GetUniquepersistentId();
            if (removeOldId) persistentUnloadedPartIds.Remove(num);
            if (addNewId) persistentUnloadedPartIds.Add(persistentId, partSnapshot);
            GE::onPartPersistentIdChanged.Fire(vesselPersistentId, num, persistentId);
            log::Info("[FlightGlobals]: ProtoPartSnapShot persistentId changed from " + dotnet::FormatUInt64(num) + " to " +
                      dotnet::FormatUInt64(persistentId) + ". Vessel persistentId " +
                      dotnet::FormatUInt64(vesselPersistentId));
        }
    } else {
        persistentUnloadedPartIds.Add(persistentId, partSnapshot);
    }
    return persistentId;
}

// FlightGlobals.cs:536 (part.vessel == null: the unloaded-part match never counts as the same vessel)
uint32_t PersistentIdRegistry::CheckPartpersistentId(uint32_t persistentId, const void* part, bool removeOldId,
                                                     bool addNewId, uint32_t vesselId) {
    if (persistentId == 0) {
        persistentId = GetUniquepersistentId();
        if (!fetch) return persistentId;
        if (addNewId) persistentLoadedPartIds.Add(persistentId, part);
        return persistentId;
    }
    if (!fetch) throw NullReferenceException("FlightGlobals.FindLoadedPart: no FlightGlobals instance");
    const void* partout = nullptr;
    ProtoPartSnapshot* partout2 = nullptr;
    const bool num = persistentLoadedPartIds.TryGetValue(persistentId, partout);
    const bool flag = persistentUnloadedPartIds.TryGetValue(persistentId, partout2);
    const bool flag2 = false;  // part.vessel != null && ... (no vessel while loading a craft)
    bool flag3 = false;
    if (num && (partout == nullptr || part == nullptr || partout != part)) flag3 = true;
    else if (flag && !flag2) flag3 = true;
    if (flag3) {
        const uint32_t num2 = persistentId;
        persistentId = GetUniquepersistentId();
        if (removeOldId) persistentLoadedPartIds.Remove(num2);
        if (addNewId) persistentLoadedPartIds.Add(persistentId, part);
        GE::onPartPersistentIdChanged.Fire(vesselId, num2, persistentId);
    } else {
        persistentLoadedPartIds.Add(persistentId, part);
    }
    return persistentId;
}

// FlightGlobals.cs:655
bool PersistentIdRegistry::ClearpersistentIdDictionaries() {
    if (!fetch) {
        log::Warning("[FlightGlobals] Warning: Tried to clear persistentId Dictionaries but instance is null");
        return false;
    }
    persistentVesselIds.Clear();
    persistentLoadedPartIds.Clear();
    persistentUnloadedPartIds.Clear();
    return true;
}

// ================================================== SaveEnvironment ================================================

SaveEnvironment& SaveEnvironment::Current() {
    static SaveEnvironment env;
    return env;
}

void SaveEnvironment::Reset() { Current() = SaveEnvironment(); }

// the stock DLC suits (ExpansionsLoader reads them from the expansion metadata: MakingHistory = Vintage,
// Serenity = Future)
void SaveEnvironment::SetStockExpansionInstalled(std::string_view name, bool installed) {
    for (auto& e : expansions)
        if (e.name == name) {
            e.installed = installed;
            return;
        }
    ExpansionInfo info;
    info.name = std::string(name);
    info.installed = installed;
    if (name == "MakingHistory") info.kerbalSuits = {ProtoCrewMember_KerbalSuit::Vintage};
    else if (name == "Serenity") info.kerbalSuits = {ProtoCrewMember_KerbalSuit::Future};
    expansions.push_back(std::move(info));
}

// ExpansionsLoader.cs:328
bool SaveEnvironment::IsExpansionInstalled(std::string_view name) const {
    for (const auto& e : expansions)
        if (e.name == name) return e.installed;
    return false;
}

// ExpansionsLoader.cs:350
bool SaveEnvironment::IsExpansionAnyKerbalSuitInstalled() const {
    for (const auto& e : expansions)
        if (e.installed && !e.kerbalSuits.empty()) return true;
    return false;
}

// ExpansionsLoader.cs:371
bool SaveEnvironment::IsExpansionKerbalSuitInstalled(ProtoCrewMember_KerbalSuit suit) const {
    if (suit == ProtoCrewMember_KerbalSuit::Default || suit == ProtoCrewMember_KerbalSuit::Slim) return true;
    for (const auto& e : expansions) {
        if (!e.installed || e.kerbalSuits.empty()) continue;
        for (auto s : e.kerbalSuits)
            if (s == suit) return true;
    }
    return false;
}

// Guid.NewGuid().GetHashCode() / Environment.TickCount (hooks; default: a process-wide random generator / the clock)
int32_t SaveEnvironment::NewGuidHashCode() const {
    if (newGuidHashCode) return newGuidHashCode();
    static std::mt19937 gen{std::random_device{}()};
    return static_cast<int32_t>(gen());
}

int32_t SaveEnvironment::TickCount() const {
    if (tickCount) return tickCount();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
    return static_cast<int32_t>(static_cast<uint32_t>(ms));
}

// Planetarium.cs:283 GetUniversalTime
double SaveUniversalTime() {
    if (Planetarium::fetch != nullptr) return Planetarium::GetUniversalTime();
    const Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr) throw NullReferenceException("Planetarium.GetUniversalTime: no Planetarium and no current game");
    return g->UniversalTime();
}

// Versioning.cs:123
std::string Versioning_GetVersion() {
    return dotnet::FormatInt64(Versioning::version_major) + "." + dotnet::FormatInt64(Versioning::version_minor) + "." +
           dotnet::FormatInt64(Versioning::Revision);
}

// Versioning.cs:138
std::string Versioning_GetVersionStringFull() {
    const SaveEnvironment& env = SaveEnvironment::Current();
    std::string text = Versioning_GetVersion() + "." + dotnet::FormatInt64(Versioning::BuildID) + " (" +
                       env.applicationPlatform + env.architecture + ")";
    if (env.experimental > 0) text += " x" + dotnet::FormatInt64(env.experimental);
    if (env.beta) text += " BETA";
    if (env.prerelease) text += "-pre";
    return text;
}

}  // namespace ksp
