// WP-40: PreFlightCheck.cs and PreFlightTests/*.cs -- the launch pre-flight tests (IPreFlightTest: abort-or-proceed dialogs) and the
// engineer's-report design concerns (IDesignConcern). Every test condition is the C# condition; the dialogs, the popup and the localised
// text layout are the shell's (PreFlightCheck runs the tests and reports which one failed; the dialog choice is the host callback).
//
// The text of a concern is selected by the same branches as the C# (the #autoLOC tag chosen is exposed as ConcernTitleTag() /
// ConcernDescriptionTag() and localised through Localizer.Format).
//
// Seams (PreFlightEnvironment): CommNetScenario.CommNetEnabled, Funding, the crew roster, the Game's FlightState, R&D, PSystemSetup,
// FlightEVA.HatchIsObstructedMore, KerbalSeat.EditorSeatBoundsAreClear and the SCCFlowGraphUCFinder of the engineer's report.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/editor/editor_types.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/flow_graph.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/ship/ship_template.hpp"

namespace ksp {

class FlightState;
class KerbalRoster;
class ShipTemplate;

namespace PreFlightTests {

// DesignConcernSeverity.cs:3
enum class DesignConcernSeverity : int { NOTICE, WARNING, CRITICAL };

// What the tests read from the game outside the ship (documented seam; defaults = a sandbox game without CommNet / career / R&D)
struct PreFlightEnvironment {
    ShipConstruct* ship = nullptr;       // EditorLogic.fetch.ship (unset: EditorLogic::fetch->ship)
    Part* rootPart = nullptr;            // EditorLogic.RootPart (unset: EditorLogic::fetch->RootPart())
    bool commNetEnabled = false;         // CommNetScenario.CommNetEnabled
    std::optional<double> funds;         // Funding.Instance.Funds; nullopt = Funding.Instance == null (no funding system: free)
    bool careerMode = false;             // HighLogic.CurrentGame.Mode == Game.Modes.CAREER
    KerbalRoster* roster = nullptr;      // HighLogic.CurrentGame.CrewRoster
    const FlightState* flightState = nullptr;   // HighLogic.CurrentGame.flightState (LaunchSiteClear); nullptr = game or state null
    bool gameCompatible = true;          // game.compatible
    bool showWrongVesselTypeConfirmation = true;   // GameSettings.SHOW_WRONG_VESSEL_TYPE_CONFIRMATION
    // PSystemSetup
    std::function<bool(const std::string&)> isSpaceCenterFacility;         // GetSpaceCenterFacility(name) != null
    std::function<std::optional<float>(const std::string&)> facilityDamage;   // SpaceCenterFacility.GetFacilityDamage(); nullopt: not a facility
    std::function<bool(const std::string&)> isLaunchSite;                  // GetLaunchSite(name) != null
    // ResearchAndDevelopment (ResearchAndDevelopment.Instance == null: researchAndDevelopment unset)
    bool researchAndDevelopmentPresent = false;
    struct TechState {
        bool available = false;          // techState.state == RDTech.State.Available
        std::vector<std::string> partsPurchased;   // names of techState.partsPurchased
    };
    std::function<std::optional<TechState>(const std::string& techId)> techState;   // GetTechState(id) (nullopt: null)
    std::function<bool(const AvailablePart&)> isExperimentalPart;          // ResearchAndDevelopment.IsExperimentalPart
    // FlightEVA.HatchIsObstructedMore(part, part.airlock) and `part.airlock != null`
    std::function<bool(Part&)> hasAirlock;
    std::function<bool(Part&)> hatchIsObstructedMore;
    std::function<bool(Part&)> editorSeatBoundsAreClear;                    // KerbalSeat.EditorSeatBoundsAreClear
    std::shared_ptr<RUI::Algorithms::SCCFlowGraphUCFinder> sccFlowGraphUCFinder;   // EngineersReport.sccFlowGraphUCFinder

    static inline PreFlightEnvironment& Current() {
        static PreFlightEnvironment v;
        return v;
    }
    ShipConstruct& Ship() const;
    Part* Root() const;
};

// IPreFlightTest.cs:3
class IPreFlightTest {
public:
    virtual ~IPreFlightTest() = default;
    virtual bool Test() = 0;
    virtual std::string GetWarningTitle() = 0;
    virtual std::string GetWarningDescription() = 0;
    virtual std::optional<std::string> GetProceedOption() = 0;   // null: no proceed button
    virtual std::string GetAbortOption() = 0;
    virtual std::string TypeName() const = 0;                    // GetType().Name
};

// IDesignConcern.cs:5
class IDesignConcern {
public:
    virtual ~IDesignConcern() = default;
    virtual bool Test() = 0;
    virtual std::vector<Part*> GetAffectedParts() = 0;
    virtual std::string GetConcernTitle() = 0;
    virtual std::string GetConcernDescription() = 0;
    virtual bool GetPreviousResult() = 0;
    virtual EditorFacilities GetEditorFacilities() = 0;
    virtual DesignConcernSeverity GetSeverity() = 0;
    virtual std::string TypeName() const = 0;
};

// DesignConcernBase.cs:5
class DesignConcernBase : public IDesignConcern {
public:
    std::vector<std::function<void(bool)>> TestResultChanged;   // :14 (event Callback<bool>)
    virtual bool TestCondition() = 0;                           // :25
    virtual std::string ConcernTitleTag() { return ""; }
    virtual std::string ConcernDescriptionTag() { return ""; }
    std::string GetConcernTitle() override;
    std::string GetConcernDescription() override;
    EditorFacilities GetEditorFacilities() override { return EditorFacilities::ALL; }   // :33
    std::vector<Part*> GetAffectedParts() override { return {}; }                          // :38 (null)
    bool GetPreviousResult() override { return lastPassed; }                               // :43
    bool Test() override;                                                                  // :51

private:
    bool lastPassed = false;
    bool firstTest = true;
};

// ---- the tests ----------------------------------------------------------------------------------------------------------------------

#define KSP_CONCERN_HEAD(Name)                                      \
    std::string TypeName() const override { return #Name; }         \
    bool TestCondition() override;                                  \
    DesignConcernSeverity GetSeverity() override;                   \
    std::string ConcernTitleTag() override;                         \
    std::string ConcernDescriptionTag() override;

// AirBreathingEnginesForIntakes.cs:7
class AirBreathingEnginesForIntakes : public DesignConcernBase {
public:
    AirBreathingEnginesForIntakes() = default;
    AirBreathingEnginesForIntakes(ShipConstruct&, EditorFacility) {}   // :16
    KSP_CONCERN_HEAD(AirBreathingEnginesForIntakes)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// AntennaPresent.cs:7
class AntennaPresent : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(AntennaPresent)
    bool foundShortRanged = false, foundLongRanged = false, foundCrewedPart = false;

private:
    bool PartIsCrewControlled(Part& p, VesselCrewManifest* manifest);   // :85
};

// CanAffordLaunchTest.cs:5
class CanAffordLaunchTest : public IPreFlightTest {
public:
    CanAffordLaunchTest(ShipConstruct& ship, const std::optional<double>& funds);   // :19 (funding != null: ship costs vs Funds)
    CanAffordLaunchTest(ShipTemplate& tmpl, const std::optional<double>& funds);    // :33
    bool Test() override;                                                           // :47
    std::string GetWarningTitle() override;
    std::string GetWarningDescription() override;
    std::optional<std::string> GetProceedOption() override { return std::nullopt; }
    std::string GetAbortOption() override;
    std::string TypeName() const override { return "CanAffordLaunchTest"; }
    float launchCost = 0.0f;
    double fundsAvailable = 0.0;
};

// ComOffset.cs:5
class ComOffset : public DesignConcernBase {
public:
    ComOffset() = default;
    explicit ComOffset(ShipConstruct&) {}
    KSP_CONCERN_HEAD(ComOffset)
    EditorFacilities GetEditorFacilities() override { return EditorFacilities::VAB; }
};

// ContractEquipment.cs:7
class ContractEquipment : public DesignConcernBase {
public:
    ContractEquipment() = default;
    explicit ContractEquipment(ShipConstruct& s) : ship(&s) {}
    KSP_CONCERN_HEAD(ContractEquipment)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    ShipConstruct* ship = nullptr;
    std::vector<Part*> failedParts;
};

// CraftWithinMassLimits.cs:7
class CraftWithinMassLimits : public IPreFlightTest {
public:
    CraftWithinMassLimits(ShipConstruct& ship, SpaceCenterFacility facility, double maxMass);   // :19
    CraftWithinMassLimits(ShipTemplate& tmpl, SpaceCenterFacility facility, double maxMass);    // :28
    CraftWithinMassLimits(float mass, const std::string& name, SpaceCenterFacility facility, double maxMass);   // :37
    bool Test() override { return craftMass <= maxMass; }                                       // :59
    std::string GetWarningTitle() override;
    std::string GetWarningDescription() override;
    std::optional<std::string> GetProceedOption() override { return std::nullopt; }
    std::string GetAbortOption() override;
    std::string TypeName() const override { return "CraftWithinMassLimits"; }
    double craftMass = 0.0, maxMass = 0.0;
    std::string facilityName, shipName;
};

// CraftWithinPartCountLimit.cs:6
class CraftWithinPartCountLimit : public IPreFlightTest {
public:
    CraftWithinPartCountLimit(ShipConstruct& ship, SpaceCenterFacility facility, int partLimit);   // :16
    CraftWithinPartCountLimit(ShipTemplate& tmpl, SpaceCenterFacility facility, int partLimit);    // :23
    CraftWithinPartCountLimit(int count, SpaceCenterFacility facility, int partLimit);             // :30
    bool Test() override { return partCount <= maxParts; }                                         // :37
    std::string GetWarningTitle() override;
    std::string GetWarningDescription() override;
    std::optional<std::string> GetProceedOption() override { return std::nullopt; }
    std::string GetAbortOption() override;
    std::string TypeName() const override { return "CraftWithinPartCountLimit"; }
    int partCount = 0, maxParts = 0;
    std::string facilityName;
};

// CraftWithinSizeLimits.cs:7
class CraftWithinSizeLimits : public IPreFlightTest {
public:
    CraftWithinSizeLimits(ShipConstruct& ship, SpaceCenterFacility facility, const Vector3f& maxSize);   // :25
    CraftWithinSizeLimits(ShipTemplate& tmpl, SpaceCenterFacility facility, const Vector3f& maxSize);    // :35
    CraftWithinSizeLimits(const Vector3f& shipSize, const std::string& shipName, SpaceCenterFacility facility, const Vector3f& maxSize);   // :45
    bool Test() override;                                                                                   // :76
    std::string GetWarningTitle() override;
    std::string GetWarningDescription() override;
    std::optional<std::string> GetProceedOption() override { return std::nullopt; }
    std::string GetAbortOption() override;
    std::string TypeName() const override { return "CraftWithinSizeLimits"; }
    Vector3f craftSize{}, maxSizeAllowed{};
    std::string facilityName, shipName;
    SpaceCenterFacility facility = SpaceCenterFacility::LaunchPad;
    bool isSCFacility = true;
};

// DecouplerFacing.cs:6
class DecouplerFacing : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(DecouplerFacing)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// DecouplersBeforeClamps.cs:6
class DecouplersBeforeClamps : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(DecouplersBeforeClamps)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// DockingPortAsDecoupler.cs:7
class DockingPortAsDecoupler : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(DockingPortAsDecoupler)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;

private:
    bool RecurseHierarchy(Part* part);                       // :27
    void AddToFailedParts(Part* part, const std::string&);   // :99
};

// DockingPortFacing.cs:7
class DockingPortFacing : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(DockingPortFacing)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;

private:
    bool RecurseHierarchy(Part* part);                       // :27
    void AddToFailedParts(Part* part, const std::string&);   // :105
};

// DockingPortRCS.cs:6
class DockingPortRCS : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(DockingPortRCS)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// ElectricBatteryAndNoCharge.cs:7
class ElectricBatteryAndNoCharge : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(ElectricBatteryAndNoCharge)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
    bool alternatorException = false;
};

// ElectricChargeAndNoBattery.cs:7
class ElectricChargeAndNoBattery : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(ElectricChargeAndNoBattery)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// ElectricChargeAndNoConsumer.cs:7
class ElectricChargeAndNoConsumer : public DesignConcernBase {
public:
    ElectricChargeAndNoConsumer();   // :22
    KSP_CONCERN_HEAD(ElectricChargeAndNoConsumer)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
    const PartResourceDefinition* resourceDefinition = nullptr;
};

// ElectricConsumerAndNoCharge.cs:8
class ElectricConsumerAndNoCharge : public DesignConcernBase {
public:
    ElectricConsumerAndNoCharge();   // :26
    KSP_CONCERN_HEAD(ElectricConsumerAndNoCharge)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
    bool alternatorException = false;
    const PartResourceDefinition* resourceDefinition = nullptr;
};

// EnginesJettisonedBeforeUse.cs:6
class EnginesJettisonedBeforeUse : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(EnginesJettisonedBeforeUse)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;

private:
    Part* RecurseHierarchy(Part* p, Part* orig);   // :35
};

// ExperimentalPartsAvailable.cs:10
class ExperimentalPartsAvailable : public IPreFlightTest {
public:
    explicit ExperimentalPartsAvailable(ShipConstruct& ship);       // :10
    explicit ExperimentalPartsAvailable(VesselCrewManifest& manifest);   // :36
    bool Test() override;                                            // :64
    std::string GetWarningTitle() override;
    std::string GetWarningDescription() override;
    std::optional<std::string> GetProceedOption() override { return std::nullopt; }
    std::string GetAbortOption() override;
    std::string TypeName() const override { return "ExperimentalPartsAvailable"; }
    // expPartsOnShip (insertion ordered: the Dictionary enumerates in insertion order when nothing was removed)
    std::vector<std::pair<const AvailablePart*, int>> expPartsOnShip;
    bool hasDictionary = false;
};

// FacilityOperational.cs:6
class FacilityOperational : public IPreFlightTest {
public:
    FacilityOperational(const std::string& FacilityName, const std::string& FacilityTitle);   // :12
    bool Test() override { return operational; }
    std::string GetWarningTitle() override;
    std::string GetWarningDescription() override;
    std::optional<std::string> GetProceedOption() override { return std::nullopt; }
    std::string GetAbortOption() override;
    std::string TypeName() const override { return "FacilityOperational"; }
    std::string facilityName, facilityTitle;
    bool operational = false;
};

// HatchObstructed.cs:6
class HatchObstructed : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(HatchObstructed)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// InputAuthority.cs:5
class InputAuthority : public DesignConcernBase {
public:
    explicit InputAuthority(ShipConstruct& s) : ship(&s) {}   // :13
    KSP_CONCERN_HEAD(InputAuthority)
    ShipConstruct* ship;
};

// IntakesForAirBreathingEngines.cs:7
class IntakesForAirBreathingEngines : public DesignConcernBase {
public:
    IntakesForAirBreathingEngines() = default;
    IntakesForAirBreathingEngines(ShipConstruct&, EditorFacility) {}
    KSP_CONCERN_HEAD(IntakesForAirBreathingEngines)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// KerbalSeatCollision.cs:7
class KerbalSeatCollision : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(KerbalSeatCollision)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// LadderPresent.cs:5
class LadderPresent : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(LadderPresent)
};

// LandingGearPresent.cs:5
class LandingGearPresent : public DesignConcernBase {
public:
    LandingGearPresent() = default;
    explicit LandingGearPresent(EditorFacility) {}
    KSP_CONCERN_HEAD(LandingGearPresent)
    EditorFacilities GetEditorFacilities() override { return EditorFacilities::SPH; }
    bool roverWheelsPresent = false;
};

// LaunchSiteClear.cs:8
class LaunchSiteClear : public IPreFlightTest {
public:
    LaunchSiteClear(const std::string& SiteName, const std::string& SiteDisplayName);   // :19
    bool Test() override;                                                                // :34
    std::string GetWarningTitle() override;
    std::string GetWarningDescription() override;
    std::optional<std::string> GetProceedOption() override;
    std::string GetAbortOption() override;
    std::string TypeName() const override { return "LaunchSiteClear"; }
    const std::string& GetObstructingVesselName() const { return obstructingVesselName; }   // :91
    int GetObstructingVesselIndex() const { return obstructingVesselIndex; }                // :96
    std::string siteName, siteDisplayName, obstructingVesselName;
    int obstructingVesselIndex = 0;
    bool stValid = false;
};

// MissingCrew.cs:6
class MissingCrew : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(MissingCrew)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// NoControlSources.cs:7
class NoControlSources : public DesignConcernBase, public IPreFlightTest {
public:
    NoControlSources();   // :20 (the editor ship manifest, refreshed on every test)
    explicit NoControlSources(VesselCrewManifest* crewManifest);   // :14
    NoControlSources(VesselCrewManifest* crewManifest, KerbalRoster* roster);   // :19
    std::string TypeName() const override { return "NoControlSources"; }
    bool TestCondition() override;
    bool Test() override { return DesignConcernBase::Test(); }
    DesignConcernSeverity GetSeverity() override { return DesignConcernSeverity::CRITICAL; }
    std::string ConcernTitleTag() override { return "#autoLOC_253474"; }
    std::string ConcernDescriptionTag() override { return "#autoLOC_253479"; }
    std::string GetWarningTitle() override;
    std::string GetWarningDescription() override;
    std::optional<std::string> GetProceedOption() override;
    std::string GetAbortOption() override;
    std::string GetTestName() { return "No Control Sources"; }   // :104
    VesselCrewManifest* manifest = nullptr;
    bool updateManifest = false;
    KerbalRoster* roster = nullptr;
};

// NoCrewInMannedVessel.cs:8 [Obsolete]
class NoCrewInMannedVessel : public DesignConcernBase, public IPreFlightTest {
public:
    NoCrewInMannedVessel();   // :20
    explicit NoCrewInMannedVessel(VesselCrewManifest* crewManifest);   // :20
    std::string TypeName() const override { return "NoCrewInMannedVessel"; }
    bool TestCondition() override;
    bool Test() override { return DesignConcernBase::Test(); }
    DesignConcernSeverity GetSeverity() override { return DesignConcernSeverity::CRITICAL; }
    std::string ConcernTitleTag() override { return "#autoLOC_253586"; }
    std::string ConcernDescriptionTag() override { return "#autoLOC_253591"; }
    std::string GetWarningTitle() override;
    std::string GetWarningDescription() override;
    std::optional<std::string> GetProceedOption() override;
    std::string GetAbortOption() override;
    std::string GetTestName();   // :101
    VesselCrewManifest* manifest = nullptr;
    bool updateManifest = false;
};

// NonRootCmdMissaligned.cs:7
class NonRootCmdMissaligned : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(NonRootCmdMissaligned)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// ParachuteOnEngineStage.cs:7
class ParachuteOnEngineStage : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(ParachuteOnEngineStage)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// ParachuteOnFirstStage.cs:6
class ParachuteOnFirstStage : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(ParachuteOnFirstStage)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// ParachutePresent.cs:7
class ParachutePresent : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(ParachutePresent)
    EditorFacilities GetEditorFacilities() override { return EditorFacilities::VAB; }
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
    bool chutePresentInSameStage = false;
};

// RequiredIScienceDataTransmitter.cs:6
class RequiredIScienceDataTransmitter : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(RequiredIScienceDataTransmitter)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// ResourceConsumersReachable.cs:8
class ResourceConsumersReachable : public DesignConcernBase {
public:
    explicit ResourceConsumersReachable(const PartResourceDefinition* resourceDefinition);   // :20
    KSP_CONCERN_HEAD(ResourceConsumersReachable)
    std::string GetConcernTitle() override;
    std::string GetConcernDescription() override;
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    const PartResourceDefinition* resourceDefinition;
    std::vector<Part*> failedParts;
    bool exceptionCommandPod = false;
};

// ResourceContainersReachable.cs:8
class ResourceContainersReachable : public DesignConcernBase {
public:
    explicit ResourceContainersReachable(const PartResourceDefinition* resourceDefinition);   // :15
    KSP_CONCERN_HEAD(ResourceContainersReachable)
    std::string GetConcernTitle() override;
    std::string GetConcernDescription() override;
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    const PartResourceDefinition* resourceDefinition;
    std::vector<Part*> failedParts;
};

// StationHubAttachments.cs:6
class StationHubAttachments : public DesignConcernBase {
public:
    KSP_CONCERN_HEAD(StationHubAttachments)
    std::vector<Part*> GetAffectedParts() override { return failedParts; }
    std::vector<Part*> failedParts;
};

// WrongVesselTypeForLaunchSite.cs:5
class WrongVesselTypeForLaunchSite : public IPreFlightTest {
public:
    WrongVesselTypeForLaunchSite(EditorFacility CorrectType, const std::string& CraftFileName, const std::string& VesselTypeName,
                                 const std::string& LaunchSiteName);   // :13
    bool Test() override;                                               // :23
    std::string GetWarningTitle() override;
    std::string GetWarningDescription() override;
    std::optional<std::string> GetProceedOption() override;
    std::string GetAbortOption() override;
    std::string TypeName() const override { return "WrongVesselTypeForLaunchSite"; }
    EditorFacility correctType;
    std::string craftFile, vesselType, launchSiteName;
};

}  // namespace PreFlightTests

// PreFlightCheck.cs:7 (the popup dialogs are the shell's: RunTests stops at the first failing test and hands it to `onFailure`;
// the host calls Proceed() / Abort() for the user's choice, Proceed continues with the next test)
class PreFlightCheck {
public:
    PreFlightCheck(std::function<void()> onComplete, std::function<void()> onAbort);   // :22
    void AddTest(std::unique_ptr<PreFlightTests::IPreFlightTest> test) { tests.push_back(std::move(test)); }   // :30
    // :29 RunTests: true when every test passed (or there is none)
    bool RunTests();
    // the dialog choices (runNextTest after the proceed button, Abort())
    void Proceed() { runNextTest(); }     // :58 (the proceed delegate)
    void Abort();                         // :111
    // observation: the test that is showing its warning (nullptr when none) and the log of "[Pre-Flight Check]" results
    PreFlightTests::IPreFlightTest* failedTest = nullptr;
    std::vector<std::string> log;
    // GameSettings.SHOW_WRONG_VESSEL_TYPE_CONFIRMATION
    bool* showWrongVesselTypeConfirmation = nullptr;
    std::vector<std::unique_ptr<PreFlightTests::IPreFlightTest>> tests;

private:
    void runNextTest();   // :43
    void Complete();      // :105
    std::function<void()> OnComplete, OnAbort;
    int currentTest = -1;
    bool runTestResult = false;
};

}  // namespace ksp
