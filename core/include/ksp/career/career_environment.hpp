// WP-36: everything the funds / reputation / facilities / strategies / progress / recovery code reads from systems that are
// other work packages or GUI. Each member is a seam: the owning package (or a test) installs it; the default is documented
// next to the member. Same pattern as DeployablesEnvironment (modules/deployables.hpp) and SaveEnvironment (save/).
//
// Owners:
//   WP-35  ResearchAndDevelopment (Instance, Science, AddScience, CanAfford, tech tree, experiments)
//   WP-38  FinePrint.Utilities.ProgressUtilities / ContractDefs.Progression / CelestialUtilities, Contract prestige
//   WP-40  editor (reads GameVariables)
//   WP-42  SpaceCenter / PSystemSetup launch sites / DestructibleBuilding / LaunchSite
//   WP-46  UI (MissionRecoveryDialog widgets, MessageSystem, ScreenMessages)
//   WP-59  AnalyticsUtil (every call is dropped)
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ksp/career/currency.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/vessel/discovery_info.hpp"

namespace ksp {

class CelestialBody;
class Vessel;
class ProtoVessel;
class Part;
class PartUpgradeHandler_Upgrade;
class ProtoCrewMember;

// FinePrint/Utilities/ProgressType.cs:3 and ProgressRewardType.cs:3 (WP-38 owns these; they are declared here because the
// achievement nodes pass them to the reward seam)
namespace FinePrint::Utilities {
enum class ProgressType : int {
    NULL_, ALTITUDERECORD, BASECONSTRUCTION, CREWTRANSFER, CREWRECOVERY, DEPTHRECORD, DISTANCERECORD, DOCKING, ESCAPE,
    FIRSTLAUNCH, FLAGPLANT, FLIGHT, FLYBY, FLYBYRETURN, LANDING, LANDINGRETURN, ORBIT, ORBITRETURN, POINTOFINTEREST,
    REACHSPACE, RENDEZVOUS, SCIENCE, SPACEWALK, SPEEDRECORD, SPLASHDOWN, STATIONCONSTRUCTION, STUNT, SUBORBIT, SURFACEEVA
};
enum class ProgressRewardType : int { CONTRACT, PROGRESS };
}  // namespace FinePrint::Utilities

// Contracts.Contract.ContractPrestige (Contract.cs: Trivial, Significant, Exceptional; WP-38 owns the enum)
enum class ContractPrestige : int { Trivial, Significant, Exceptional };

struct CareerEnvironment {
    // ---- WP-35 ResearchAndDevelopment ---------------------------------------------------------------------------------
    std::function<bool()> rndPresent;                          // ResearchAndDevelopment.Instance != null (unset: false)
    std::function<float()> rndScience;                         // ResearchAndDevelopment.Instance.Science (unset: 0)
    std::function<void(float, TransactionReasons)> rndAddScience;  // Instance.AddScience(value, reason) (unset: dropped)
    std::function<bool(float)> rndCanAfford;                   // ResearchAndDevelopment.CanAfford(float) (unset: true)
    // ResearchAndDevelopment.GetExperiment("surfaceSample").requiredExperimentLevel; nullopt-like: return a negative value for
    // "no such experiment" (KSCFacilityLevelText "EVASrfSample"); unset: no experiment
    std::function<bool(float& requiredExperimentLevel)> rndSurfaceSampleExperiment;
    // AwardProgressRandomTech: AssetBase.RnDTechTree.GetNextUnavailableNodes().Count and UnlockProtoTechNode(nodes[index]);
    // rndUnlockNext returns the technology title (localised), unset: no unavailable nodes
    std::function<int()> rndNextUnavailableNodeCount;
    std::function<std::string(int index)> rndUnlockNextUnavailableNode;

    // ---- WP-38 ProgressUtilities / ContractDefs ------------------------------------------------------------------------
    using ProgressType = FinePrint::Utilities::ProgressType;
    using ProgressRewardType = FinePrint::Utilities::ProgressRewardType;
    // ProgressUtilities.WorldFirstStandardReward(reward, currency, progress, body) (unset: 0)
    std::function<float(ProgressRewardType, Currency, ProgressType, CelestialBody*)> worldFirstStandardReward;
    // ProgressUtilities.WorldFirstIntervalReward(reward, currency, progress, body, currentInterval, totalIntervals) (unset: 0)
    std::function<float(ProgressRewardType, Currency, ProgressType, CelestialBody*, int, int)> worldFirstIntervalReward;
    // ProgressUtilities.FindNextRecord(current, maximum, roundValue, ref interval) (unset: returns maximum, interval unchanged)
    std::function<double(double, double, double, int&)> findNextRecord;
    // ContractDefs.Instance != null / ContractDefs.Progression.{RecordSplit, MaxDepthRecord, MaxDistanceRecord, MaxSpeedRecord}
    // (unset: no ContractDefs, the achievement nodes use their literal fallbacks 750 / 100000 / 2500)
    std::function<bool()> contractDefsPresent;
    std::function<int()> progressionRecordSplit;
    std::function<double()> progressionMaxDepthRecord;
    std::function<double()> progressionMaxDistanceRecord;
    std::function<double()> progressionMaxSpeedRecord;
    // StringUtilities.ValueAndUnits(RecordTrackType, value) (RecordTrackType: NULL, ALTITUDE, DEPTH, DISTANCE, SPEED as ints; unset:
    // the plain number)
    std::function<std::string(int recordTrackType, double value)> valueAndUnits;
    // CelestialUtilities.GreatCircleDistance(body, lat1, lon1, lat2, lon2) in metres (unset: 0)
    std::function<double(CelestialBody*, double, double, double, double)> greatCircleDistance;

    // ---- WP-42 SpaceCenter / launch sites -------------------------------------------------------------------------------
    // SpaceCenter.Instance != null and SpaceCenter.Instance.cb (the home body); nullptr = no SpaceCenter / no home
    std::function<CelestialBody*()> spaceCenterBody;
    // SpaceCenter.Instance.GreatCircleDistance(cb.GetRelSurfaceNVector(lat, lon)) in metres (unset: 0)
    std::function<double(double lat, double lon)> spaceCenterGreatCircleDistance;
    // PSystemSetup.Instance.LaunchSites: names, requiresPOIVisit (VesselRecovery.checkLaunchSites, PointOfInterest)
    struct LaunchSiteInfo {
        std::string name;
        bool requiresPOIVisit = false;
        CelestialBody* body = nullptr;
        std::string launchSiteName;          // LaunchSite.launchSiteName (a localisation tag)
        std::string editorFacilityDisplay;   // LaunchSite.editorFacility.displayDescription()
    };
    // GameEvents.LaunchSiteFound.Fire(launchSite) (the LaunchSite object is WP-42's; unset: nothing)
    std::function<void(const std::string& launchSiteName)> launchSiteFound;
    std::function<std::vector<LaunchSiteInfo>()> launchSites;
    // PSystemSetup.Instance.IsStockLaunchSite(name) and GetLaunchSite(name) (RecordsDistance; unset: false / no site)
    std::function<bool(const std::string&)> isStockLaunchSite;
    std::function<bool(const std::string& name, CelestialBody*& siteBody, double lat, double lon, double& distance)> launchSiteDistance;
    // ExpansionsLoader.IsExpansionInstalled comes from SaveEnvironment::Current().
    // PSystemSetup.Instance.SpaceCenterFacilities (ScenarioUpgradeableFacilities.getInitialState / getInitialMissionState): per
    // facility the compiled id of its UpgradeableObject in parent ("" = none: HierarchyUtil.CompileID(transform, "SpaceCenter")),
    // the compiled id of its UpgradeableFacility on the facility transform itself ("" = none) and that facility's MaxLevel.
    struct SpaceCenterFacilityInit {
        std::string facilityName;
        std::string upgradeableObjectId;
        std::string upgradeableFacilityId;
        int maxLevel = 0;
    };
    std::function<std::vector<SpaceCenterFacilityInit>()> spaceCenterFacilities;  // unset: none
    // Expansions.Missions.MissionsUtils.GetFacilityLimit(facilityName, maxLevel) (unset: maxLevel)
    std::function<float(const std::string& facilityName, int maxLevel)> missionsFacilityLimit;

    // ---- WP-46 UI --------------------------------------------------------------------------------------------------------
    // ProgressNode.AwardProgress: the "World First" message built from StringUtilities.FormattedCurrencies (MessageSystem is a
    // GUI singleton; unset: no message). Called after the currencies were paid.
    std::function<void(const std::string& description, float funds, float science, float reputation)> worldFirstMessage;
    // MessageSystem.Instance.AddMessage(title, message) used by PointOfInterest / Strategy.SendStateMessage (unset: dropped)
    std::function<void(const std::string& title, const std::string& message)> postMessage;

    // KSP.UI.Screens.Administration.Instance (Strategy.CanBeActivated / CurrencyExchanger read its counters; the real object
    // computes them from the Administration facility level in Awake and counts the active strategies it lists)
    struct AdministrationState {
        bool present = false;                 // Administration.Instance != null (false: CanBeActivated throws NullReferenceException)
        int ActiveStrategyCount = 0;          // Administration.cs:106
        int MaxActiveStrategies = 5;          // :108
        float MaxStrategyCommitLevel = 1.0f;  // :110
        std::function<void()> RedrawPanels;   // :408 (unset: nothing)
    };
    AdministrationState administration;

    // ---- WP-59 / misc -----------------------------------------------------------------------------------------------------
    // PartUpgradeHandler.Upgrade.entryCost (Funding.onUpgradePurchased; unset: 0)
    std::function<float(PartUpgradeHandler_Upgrade*)> upgradeEntryCost;
    // FlightGlobals.GetHomeBody() / GetHomeBodyName(): unset = the first body of FlightGlobals::bodies with isHomeWorld
    std::function<CelestialBody*()> getHomeBody;
    // VesselUtilities.ActualCrewCapacity() (ScenarioAchievements.OnOrbit; unset: 0)
    std::function<int()> actualCrewCapacity;
    // ResearchAndDevelopment.GetMiniBiomedisplayNameByScienceID(scienceId, formatted) (VesselRecovery location text; unset: the id)
    std::function<std::string(const std::string&, bool)> rndMiniBiomeDisplayName;
    // GameVariables.GetEVALockedReason: the live Kerbal branch (crew.KerbalRef.state == DEAD / InPart.NoAutoEVA): returns the
    // localisation tag of the reason, "" when there is none (unset: none)
    std::function<std::string(ProtoCrewMember*)> kerbalRefLockReason;
    // ScreenMessages.PostScreenMessage / Debug text of GUI-only paths are dropped
    // The scheduler StartCoroutine goes to (Vessel.StartCoroutine / MonoBehaviour.StartCoroutine); unset: an internal one that the
    // caller advances with CareerEnvironment::Scheduler()
    CoroutineScheduler* scheduler = nullptr;
    CoroutineScheduler& Scheduler();

    // GameVariables.GetEVALockedReason etc. need nothing else. The process-wide environment (like the C# statics).
    static CareerEnvironment& Current();
    static void Reset();  // restore the defaults (tests)

    // FlightGlobals.GetHomeBody() through the seam
    CelestialBody* HomeBody();

private:
    CoroutineScheduler own_;
};

}  // namespace ksp
