// WP-37: everything the contract system reads from systems that are other work packages, GUI or the wall clock. Each member is a
// seam: the owning package (or a test) installs it; the default is documented next to the member. Same pattern as
// DeployablesEnvironment (modules/deployables.hpp) and CareerEnvironment (career/career_environment.hpp).
//
// Owners:
//   WP-38  FinePrint (ContractDefs, ProgressUtilities, StringUtilities, OrbitUtilities, CelestialUtilities, WaypointManager, Waypoint,
//          the FinePrint contract types, ExplorationContract): the small stateless helpers WP-37's files call are ported next to the
//          templates (contracts/fineprint_lite.hpp) and replaceable here; the stateful ones are seams
//   WP-46  UI (MessageSystem, ScreenMessages, mission control), WP-59 analytics
//   WP-19  flight scene (FlightGlobals.ActiveVessel / ready are read through FlightGlobals / FlightRegistry)
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"

namespace ksp {

class CelestialBody;
class Vessel;
class ProtoVessel;
class ConfigNode;

namespace Contracts {

class Contract;
class ContractParameter;

// FinePrint.ContractDefs (the part WP-37 reads): the "Contracts" node of GameData/Squad/Contracts/Contracts.cfg. ContractDefs.cs:1562
// CreateDefaultConfig writes the node from the static defaults and :2072 LoadConfig reads it back into the static members, so the
// loaded value of every key is the cfg value. Section / sub-node names are the cfg names (C# `Research` is the cfg node `Science`,
// C# `Expire` is `Expiration`); the call sites cite the C# member.
class ContractDefsLite {
public:
    // The root node ("Contracts" node itself, or its parent). Replaces the loaded data (ContractDefs.Instance != null afterwards).
    static void Load(const ConfigNode& contractsNode);
    static void Load(const std::string& contractsCfgPath);
    static void Clear();
    static bool Loaded();
    // section / sub / key, e.g. ("Flag", "Expiration", "MinimumExpireDays") or ("", "", "WeightDefault") for a top-level key.
    // Missing key or unparsable value: throws KeyNotFoundException / FormatException (the C# keeps its static initialiser; the stock
    // cfg carries every key).
    static float F(const std::string& section, const std::string& sub, const std::string& key);
    static double D(const std::string& section, const std::string& sub, const std::string& key);
    static int I(const std::string& section, const std::string& sub, const std::string& key);
    static bool B(const std::string& section, const std::string& sub, const std::string& key);
    static std::string S(const std::string& section, const std::string& sub, const std::string& key);
    static bool Has(const std::string& section, const std::string& sub, const std::string& key);
    // the sub node itself (PART_REQUEST etc. are WP-38)
    static ConfigNode* Node(const std::string& section, const std::string& sub = "");
};

struct ContractsEnvironment {
    // Planetarium.fetch.time (Contract.GameTime). Unset: Planetarium::fetch ? time : 0.
    std::function<double()> gameTime;
    // Guid.NewGuid() (Contract.Generate, ContractSystem; Contract guid is part of SuperSeed). Unset: a process-wide counter based guid
    // (deterministic, unique per call); tests install their own.
    std::function<Guid()> newGuid;
    // KSPUtil.SystemDateTime.DateTimeNow().Ticks (SetupSeed(-1), RefreshContracts). Unset: steady clock based.
    std::function<int64_t()> nowTicks;
    // new KSPRandom() (parameterless: seeded from Environment.TickCount; SystemUtilities.CoinFlip(null)). Unset: tick count.
    std::function<int32_t()> newRandomSeed;
    // CheatOptions.IgnoreAgencyMindsetOnContracts (Agent.ProcessContract). Unset: false.
    bool ignoreAgencyMindsetOnContracts = false;
    // MessageSystem.Instance != null and AddMessage(title, message): reuses CareerEnvironment::postMessage when unset (dropped there).
    std::function<void(const std::string& title, const std::string& message)> postMessage;
    // GameSettings.VERBOSE_DEBUG_LOG (log lines only).
    bool verboseDebugLog = false;
    // TimeWarp.CurrentRate (ContractSystem.UpdateDaemon). Unset: 1.
    std::function<float()> timeWarpRate;
    // ProgressTracking present is read through ProgressTracking::Instance(); the AstronautComplex facility level through
    // ScenarioUpgradeableFacilities (WP-36).
    // PartLoader.getPartInfoByName(name) -> AvailablePart (unset: the PartLoader of the process: PartLoader::getPartInfoByName)
    // ResearchAndDevelopment.PartModelPurchased(partInfo) (ModuleTestSubject; unset: true)
    std::function<bool(const std::string& partName)> partModelPurchased;
    // CompatibilityUtilities.OldCareerSave(Game.Modes.CAREER, version) / UpdateCareerSave(gameNode) (ContractSystem.OnLoadRoutine, WP-38).
    // Unset: not an old save / nothing updated.
    std::function<bool(const std::string& versionString)> oldCareerSave;
    std::function<void(ConfigNode& gameNode)> updateCareerSave;
    // VersioningBase.GetVersionString() (the version written back after UpdateCareerSave)
    std::string versionString = "1.12.5";
    // CelestialBody.BiomeMap.GetAtt(latitude, longitude).name (the biome bitmap decode is a later package). Unset: "" (no biome map).
    std::function<std::string(CelestialBody*, double latitude, double longitude)> biomeNameAt;
    // ContractDefs.LoadContractCraftDefs() + LoadConstructionPartsList() (ContractSystem.OnAwake, ContractSystem.cs:219; FinePrint::InstallAll connects them). Unset: nothing.
    std::function<void()> loadContractDefsData;
    // ContractSystem.SpawnPreBuiltShip / ConstructShip (ship construction + vessel spawn; WP-19 / WP-38 / WP-40). Unset: nothing.
    std::function<void(const std::string& craftUrl)> spawnPreBuiltShip;

    static ContractsEnvironment& Current();
    static void Reset();

    // helpers over the seams (the defaults)
    double GameTime() const;
    Guid NewGuid();
    int64_t NowTicks() const;
    int32_t NewRandomSeed() const;

private:
    uint64_t guidCounter_ = 0;
};

}  // namespace Contracts
}  // namespace ksp
