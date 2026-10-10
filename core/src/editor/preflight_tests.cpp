// WP-40: PreFlightCheck.cs and PreFlightTests/*.cs, see include/ksp/editor/preflight_tests.hpp.
#include "ksp/editor/preflight_tests.hpp"

#include <algorithm>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/editor/editor_geometry.hpp"
#include "ksp/editor/editor_logic.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/modules/resource_processing.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/staging/staging_types.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

PreFlightCheck::PreFlightCheck(std::function<void()> onComplete, std::function<void()> onAbort)
    : OnComplete(std::move(onComplete)), OnAbort(std::move(onAbort)) {}

namespace PreFlightTests {

namespace {

bool HasModule(Part& p, std::string_view cls) { return p.FindModuleOfClass(cls) != nullptr; }

std::vector<PartModule*> ModulesOf(Part& p, std::string_view cls) {
    return p.FindModulesImplementing([cls](PartModule& m) { return m.IsA(cls); });
}

bool Contains(const std::vector<Part*>& v, const Part* x) { return std::find(v.begin(), v.end(), x) != v.end(); }

bool IsStageSeparator(Part& p) {
    return p.FindModuleImplementing([](PartModule& m) { return dynamic_cast<IStageSeparator*>(&m) != nullptr; }) != nullptr;
}

// part.isGenerator(out generator) and generator.resHandler.outputResources.Any(name == "ElectricCharge")
bool GeneratorOutputsElectricCharge(Part& part) {
    PartModule* m = part.FindModuleOfClass("ModuleGenerator");
    if (m == nullptr) return false;
    for (const auto& r : m->resHandler.outputResources)
        if (r->name == "ElectricCharge") return true;
    return false;
}

// part.HasModuleImplementing<ModuleDeployableSolarPanel>() || (part.name != "launchClamp1" && generator that outputs ElectricCharge)
bool IsElectricChargeProducer(Part& part) {
    return HasModule(part, "ModuleDeployableSolarPanel") || (part.gameObjectName != "launchClamp1" && GeneratorOutputsElectricCharge(part));
}

bool ConsumesResource(Part& part, const PartResourceDefinition* def) {
    for (PartModule* m : part.Modules().GetModules([](PartModule& pm) { return dynamic_cast<IResourceConsumer*>(&pm) != nullptr; })) {
        auto* c = dynamic_cast<IResourceConsumer*>(m);
        for (const PartResourceDefinition* d : c->GetConsumedResources())
            if (d == def) return true;
    }
    return false;
}

std::string FacilityName(SpaceCenterFacility f) {
    static const char* names[] = {"Administration", "AstronautComplex", "LaunchPad", "MissionControl", "ResearchAndDevelopment",
                                  "Runway", "TrackingStation", "SpaceplaneHangar", "VehicleAssemblyBuilding"};
    return names[static_cast<int>(f)];
}

// double.ToString("0.##"): two decimals with the trailing zeros (and a bare point) removed
std::string Format0hh(double v) {
    std::string t = dotnet::FormatDoubleF(v, 2);
    if (t.find('.') != std::string::npos) {
        while (!t.empty() && t.back() == '0') t.pop_back();
        if (!t.empty() && t.back() == '.') t.pop_back();
    }
    if (t == "-0") t = "0";
    return t;
}

std::string LocalizedFacilityName(SpaceCenterFacility f) { return Localizer::Format(KSPUtil::PrintLocalizedModuleName(FacilityName(f))); }

}  // namespace

ShipConstruct& PreFlightEnvironment::Ship() const { return ship != nullptr ? *ship : *EditorLogic::fetch->ship; }
Part* PreFlightEnvironment::Root() const { return rootPart != nullptr ? rootPart : EditorLogic::fetch->RootPart(); }

#define SHIP() (PreFlightEnvironment::Current().Ship())

// ---- DesignConcernBase.cs --------------------------------------------------------------------------------------------------------------

std::string DesignConcernBase::GetConcernTitle() {
    const std::string t = ConcernTitleTag();
    return t.empty() ? t : Localizer::Format(t);
}
std::string DesignConcernBase::GetConcernDescription() {
    const std::string t = ConcernDescriptionTag();
    return t.empty() ? t : Localizer::Format(t);
}

// DesignConcernBase.cs:51
bool DesignConcernBase::Test() {
    const bool flag = TestCondition();
    if (firstTest) {
        for (auto& cb : TestResultChanged) cb(flag);
        firstTest = false;
        lastPassed = flag;
    } else if (flag) {
        if (!lastPassed)
            for (auto& cb : TestResultChanged) cb(true);
        lastPassed = true;
    } else {
        if (lastPassed)
            for (auto& cb : TestResultChanged) cb(false);
        lastPassed = false;
    }
    return lastPassed;
}

// ---- AirBreathingEnginesForIntakes.cs / IntakesForAirBreathingEngines.cs -------------------------------------------------------------------

namespace {
bool HasIntakeAirIntake(Part& part) {
    for (PartModule* m : ModulesOf(part, "ModuleResourceIntake"))
        if (m->Get<std::string>("resourceName") == "IntakeAir") return true;
    return false;
}
bool HasIntakeAirEngine(Part& part) {
    for (PartModule* m : ModulesOf(part, "ModuleEngines"))
        if (auto* e = dynamic_cast<ModuleEngines*>(m))
            for (const Propellant& p : e->propellants)
                if (p.name == "IntakeAir") return true;
    return false;
}
}  // namespace

// AirBreathingEnginesForIntakes.cs:25
bool AirBreathingEnginesForIntakes::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (HasIntakeAirIntake(*part)) failedParts.push_back(part);
    }
    if (!failedParts.empty()) {
        for (int j = 0; j < count; j++) {
            Part* part = ship[static_cast<size_t>(j)];
            if (HasIntakeAirEngine(*part)) {
                failedParts.clear();
                return true;
            }
        }
    }
    return failedParts.empty();
}
DesignConcernSeverity AirBreathingEnginesForIntakes::GetSeverity() { return DesignConcernSeverity::WARNING; }   // :53
std::string AirBreathingEnginesForIntakes::ConcernTitleTag() { return "#autoLOC_250906"; }
std::string AirBreathingEnginesForIntakes::ConcernDescriptionTag() { return "#autoLOC_250911"; }

// IntakesForAirBreathingEngines.cs:25
bool IntakesForAirBreathingEngines::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (HasIntakeAirEngine(*part)) failedParts.push_back(part);
    }
    if (!failedParts.empty()) {
        for (int j = 0; j < count; j++) {
            Part* part = ship[static_cast<size_t>(j)];
            if (HasIntakeAirIntake(*part)) {
                failedParts.clear();
                return true;
            }
        }
    }
    return failedParts.empty();
}
DesignConcernSeverity IntakesForAirBreathingEngines::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string IntakesForAirBreathingEngines::ConcernTitleTag() { return "#autoLOC_252317"; }
std::string IntakesForAirBreathingEngines::ConcernDescriptionTag() { return "#autoLOC_252322"; }

// ---- AntennaPresent.cs ---------------------------------------------------------------------------------------------------------------------

// AntennaPresent.cs:74
bool AntennaPresent::PartIsCrewControlled(Part& p, VesselCrewManifest* manifest) {
    if (manifest == nullptr) return false;
    auto pcm = manifest->GetPartCrewManifest(p.fields.craftID);
    if (pcm != nullptr && pcm->PartInfo()->partPrefab->fields.CrewCapacity > 0 && p.fields.CrewCapacity > 0 && !pcm->AllSeatsEmpty()) {
        if (pcm->PartInfo()->partPrefab->fields.isControlSource <= 0) return false;
        PartModule* moduleCommand = pcm->PartInfo()->partPrefab->FindModuleOfClass("ModuleCommand");
        if (moduleCommand != nullptr) {
            if (moduleCommand->Get<int32_t>("minimumCrew") <= pcm->CountCrewNotType(ProtoCrewMember_KerbalType::Tourist)) return true;
            return false;
        }
        return true;
    }
    return false;
}

// AntennaPresent.cs:32 (ICommAntenna implementers: ModuleDataTransmitter and ModuleGroundCommsPart, both with `antennaType`; INTERNAL = 0)
bool AntennaPresent::TestCondition() {
    if (!PreFlightEnvironment::Current().commNetEnabled) return true;
    ShipConstruct& ship = SHIP();
    VesselCrewManifest* shipManifest = ShipConstruction::ShipManifest;
    foundShortRanged = false;
    foundLongRanged = false;
    foundCrewedPart = false;
    for (int i = 0, count = ship.LiveCount(); i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (!foundCrewedPart && PartIsCrewControlled(*part, shipManifest)) foundCrewedPart = true;
        if (!foundShortRanged || !foundLongRanged) {
            std::vector<PartModule*> list = part->FindModulesImplementing([](PartModule& m) { return m.IsA("ModuleDataTransmitter") || m.IsA("ModuleGroundCommsPart"); });
            int num = static_cast<int>(list.size()) - 1;
            while (num >= 0 && (!foundShortRanged || !foundLongRanged)) {
                if (list[static_cast<size_t>(num)]->Get<int32_t>("antennaType") == 0) foundShortRanged = true;
                else foundLongRanged = true;
                num--;
            }
        }
        if (foundShortRanged && foundLongRanged && foundCrewedPart) break;
    }
    if (foundCrewedPart && !foundLongRanged) return false;
    if (!foundCrewedPart && (!foundShortRanged || !foundLongRanged)) return false;
    return true;
}
// AntennaPresent.cs:138
std::string AntennaPresent::ConcernTitleTag() {
    if (foundCrewedPart) {
        if (!foundLongRanged) return "#autoLOC_251018";
    } else {
        if (!foundShortRanged) return "#autoLOC_251023";
        if (!foundLongRanged) return "#autoLOC_251025";
    }
    return "#autoLOC_251028";
}
std::string AntennaPresent::ConcernDescriptionTag() {
    if (foundCrewedPart) {
        if (!foundLongRanged) return "#autoLOC_251036";
    } else {
        if (!foundShortRanged) return "#autoLOC_251041";
        if (!foundLongRanged) return "#autoLOC_251043";
    }
    return "#autoLOC_251046";
}
DesignConcernSeverity AntennaPresent::GetSeverity() {
    if (foundCrewedPart) return DesignConcernSeverity::NOTICE;
    if (!foundShortRanged) return DesignConcernSeverity::CRITICAL;
    if (!foundLongRanged) return DesignConcernSeverity::WARNING;
    return DesignConcernSeverity::NOTICE;
}

// ---- CanAffordLaunchTest.cs ----------------------------------------------------------------------------------------------------------------

CanAffordLaunchTest::CanAffordLaunchTest(ShipConstruct& ship, const std::optional<double>& funds) {
    if (funds) {
        float dry = 0.0f, fuel = 0.0f;
        launchCost = ship.GetShipCosts(dry, fuel, ShipConstruction::ShipManifest);
        fundsAvailable = *funds;
    } else {
        launchCost = 0.0f;
        fundsAvailable = 3.4028234663852886E+38;
    }
}
CanAffordLaunchTest::CanAffordLaunchTest(ShipTemplate& tmpl, const std::optional<double>& funds) {
    if (funds) {
        launchCost = tmpl.totalCost;
        fundsAvailable = *funds;
    } else {
        launchCost = 0.0f;
        fundsAvailable = 3.4028234663852886E+38;
    }
}
// CanAffordLaunchTest.cs:47
bool CanAffordLaunchTest::Test() { return static_cast<double>(launchCost) <= fundsAvailable; }
std::string CanAffordLaunchTest::GetWarningTitle() { return Localizer::Format("#autoLOC_250625"); }
std::string CanAffordLaunchTest::GetWarningDescription() { return Localizer::Format("#autoLOC_250630"); }
std::string CanAffordLaunchTest::GetAbortOption() { return Localizer::Format("#autoLOC_250640"); }

// ---- ComOffset.cs / InputAuthority.cs / ContractEquipment.cs ---------------------------------------------------------------------------------

bool ComOffset::TestCondition() { return true; }   // ComOffset.cs:15
DesignConcernSeverity ComOffset::GetSeverity() { return DesignConcernSeverity::CRITICAL; }
std::string ComOffset::ConcernTitleTag() { return "#autoLOC_251096"; }
std::string ComOffset::ConcernDescriptionTag() { return "#autoLOC_251101"; }

bool InputAuthority::TestCondition() { return true; }   // InputAuthority.cs:16 (`_ = ship.parts.Count`)
DesignConcernSeverity InputAuthority::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string InputAuthority::ConcernTitleTag() { return "#autoLOC_252231"; }
std::string InputAuthority::ConcernDescriptionTag() { return "#autoLOC_252236"; }

// ContractEquipment.cs:19 (career: failedParts is cleared, nothing fills it in the stock game)
bool ContractEquipment::TestCondition() {
    if (ship != nullptr && ship->LiveCount() > 0 && PreFlightEnvironment::Current().careerMode) failedParts.clear();
    return failedParts.empty();
}
DesignConcernSeverity ContractEquipment::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string ContractEquipment::ConcernTitleTag() { return "#autoLOC_251140"; }
std::string ContractEquipment::ConcernDescriptionTag() { return "#autoLOC_251145"; }

// ---- CraftWithin*Limits.cs -----------------------------------------------------------------------------------------------------------------

CraftWithinMassLimits::CraftWithinMassLimits(ShipConstruct& ship, SpaceCenterFacility facility, double maxMass_)
    : craftMass(static_cast<double>(ship.GetTotalMass())), maxMass(maxMass_), facilityName(LocalizedFacilityName(facility)), shipName(ship.shipName) {}
CraftWithinMassLimits::CraftWithinMassLimits(ShipTemplate& tmpl, SpaceCenterFacility facility, double maxMass_)
    : craftMass(static_cast<double>(tmpl.totalMass)), maxMass(maxMass_), facilityName(LocalizedFacilityName(facility)), shipName(tmpl.shipName) {}
CraftWithinMassLimits::CraftWithinMassLimits(float mass, const std::string& name, SpaceCenterFacility facility, double maxMass_)
    : craftMass(static_cast<double>(mass)), maxMass(maxMass_), facilityName(LocalizedFacilityName(facility)), shipName(name) {}
std::string CraftWithinMassLimits::GetWarningTitle() { return Localizer::Format("#autoLOC_250677"); }
std::string CraftWithinMassLimits::GetWarningDescription() {
    return Localizer::Format("#autoLOC_250682", {facilityName, Format0hh(maxMass), shipName, Format0hh(craftMass)});
}
std::string CraftWithinMassLimits::GetAbortOption() { return Localizer::Format("#autoLOC_250692"); }

CraftWithinPartCountLimit::CraftWithinPartCountLimit(ShipConstruct& ship, SpaceCenterFacility facility, int partLimit)
    : partCount(ship.LiveCount()), maxParts(partLimit), facilityName(LocalizedFacilityName(facility)) {}
CraftWithinPartCountLimit::CraftWithinPartCountLimit(ShipTemplate& tmpl, SpaceCenterFacility facility, int partLimit)
    : partCount(tmpl.partCount), maxParts(partLimit), facilityName(LocalizedFacilityName(facility)) {}
CraftWithinPartCountLimit::CraftWithinPartCountLimit(int count, SpaceCenterFacility facility, int partLimit)
    : partCount(count), maxParts(partLimit), facilityName(LocalizedFacilityName(facility)) {}
std::string CraftWithinPartCountLimit::GetWarningTitle() { return Localizer::Format("#autoLOC_250727"); }
std::string CraftWithinPartCountLimit::GetWarningDescription() { return Localizer::Format("#autoLOC_250732", {facilityName, std::to_string(maxParts)}); }
std::string CraftWithinPartCountLimit::GetAbortOption() { return Localizer::Format("#autoLOC_250742"); }

CraftWithinSizeLimits::CraftWithinSizeLimits(ShipConstruct& ship, SpaceCenterFacility facility_, const Vector3f& maxSize)
    : craftSize(ShipConstruction::CalculateCraftSize(ship)), maxSizeAllowed(maxSize), facilityName(LocalizedFacilityName(facility_)), shipName(ship.shipName),
      facility(facility_) {}
CraftWithinSizeLimits::CraftWithinSizeLimits(ShipTemplate& tmpl, SpaceCenterFacility facility_, const Vector3f& maxSize)
    : craftSize(ShipConstruction::CalculateCraftSize(tmpl)), maxSizeAllowed(maxSize), facilityName(LocalizedFacilityName(facility_)), shipName(tmpl.shipName),
      facility(facility_) {}
CraftWithinSizeLimits::CraftWithinSizeLimits(const Vector3f& shipSize, const std::string& name, SpaceCenterFacility facility_, const Vector3f& maxSize)
    : craftSize(shipSize), maxSizeAllowed(maxSize), facilityName(LocalizedFacilityName(facility_)), shipName(name), facility(facility_) {}
// CraftWithinSizeLimits.cs:67
bool CraftWithinSizeLimits::Test() {
    if (craftSize.x <= maxSizeAllowed.x && craftSize.y <= maxSizeAllowed.y && craftSize.z <= maxSizeAllowed.z) return craftSize != Vector3f::zero();
    return false;
}
std::string CraftWithinSizeLimits::GetWarningTitle() {
    if (craftSize == Vector3f::zero()) return Localizer::Format("#autoLOC_250789");
    return Localizer::Format("#autoLOC_250793", {facilityName});
}
std::string CraftWithinSizeLimits::GetWarningDescription() {
    auto f = [](float v) { return Format0hh(static_cast<double>(v)); };
    if (craftSize == Vector3f::zero())
        return Localizer::Format("#autoLOC_250801", {shipName, KSPUtil::PrintModuleName(FacilityName(EditorEnumExtensions::GetEditorFacility(facility)))});
    const bool padOrVab = facility == SpaceCenterFacility::LaunchPad || facility == SpaceCenterFacility::VehicleAssemblyBuilding;
    const bool useVab = isSCFacility ? padOrVab : padOrVab;
    // the launch-site variant picks the VAB wording by launchsite.editorFacility; the SC facility variant by the facility
    return Localizer::Format("#autoLOC_250811", {shipName}) + "\n" + Localizer::Format(useVab ? "#autoLOC_250820" : "#autoLOC_250812", {f(craftSize.y), f(maxSizeAllowed.y)}) + "\n" +
           Localizer::Format(useVab ? "#autoLOC_250821" : "#autoLOC_250813", {f(craftSize.x), f(maxSizeAllowed.x)}) + "\n" +
           Localizer::Format(useVab ? "#autoLOC_250822" : "#autoLOC_250814", {f(craftSize.z), f(maxSizeAllowed.z)}) + "\n";
}
std::string CraftWithinSizeLimits::GetAbortOption() { return Localizer::Format("#autoLOC_250834"); }

// ---- DecouplerFacing.cs ----------------------------------------------------------------------------------------------------------------------

// DecouplerFacing.cs:21
bool DecouplerFacing::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    std::vector<Part*> list;
    for (Part* a : ship.liveParts)
        if (HasModule(*a, "ModuleDecouple")) list.push_back(a);
    for (Part* part : list) {
        PartModule* moduleDecoupler = part->FindModuleOfClass("ModuleDecouple");   // part.isDecoupler(out moduleDecoupler)
        AttachNode* attachNode = part->FindAttachNode(moduleDecoupler->Get<std::string>("explosiveNodeID"));
        if (!moduleDecoupler->Get<bool>("isOmniDecoupler") && attachNode != nullptr && attachNode->attachedPart != part->parent) failedParts.push_back(part);
    }
    return failedParts.empty();
}
DesignConcernSeverity DecouplerFacing::GetSeverity() { return DesignConcernSeverity::CRITICAL; }
std::string DecouplerFacing::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_251200" : "#autoLOC_251201"; }
std::string DecouplerFacing::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_251207" : "#autoLOC_251208"; }

// ---- DecouplersBeforeClamps.cs / DockingPortRCS.cs ---------------------------------------------------------------------------------------

// DecouplersBeforeClamps.cs:21
bool DecouplersBeforeClamps::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts = StageManager::FindPartsWithModuleSeparatingBeforeOtherPartsWithModule(
        ship.liveParts, [](Part& p) { return HasModule(p, "ModuleDecouple"); }, [](Part& p) { return HasModule(p, "LaunchClamp"); });
    return failedParts.empty();
}
DesignConcernSeverity DecouplersBeforeClamps::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string DecouplersBeforeClamps::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_251251" : "#autoLOC_251252"; }
std::string DecouplersBeforeClamps::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_251258" : "#autoLOC_251259"; }

// DockingPortRCS.cs:17
bool DockingPortRCS::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts = StageManager::FindPartsWithModuleSeparatingBeforeOtherPartsWithModule(
        ship.liveParts, [](Part& p) { return HasModule(p, "ModuleDockingNode"); }, [](Part& p) { return HasModule(p, "ModuleRCS"); });
    return failedParts.empty();
}
DesignConcernSeverity DockingPortRCS::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string DockingPortRCS::ConcernTitleTag() { return "#autoLOC_251727"; }
std::string DockingPortRCS::ConcernDescriptionTag() { return "#autoLOC_251732"; }

// ---- DockingPortAsDecoupler.cs / DockingPortFacing.cs -----------------------------------------------------------------------------------------

namespace {
std::string ReferenceNodeOf(PartModule* dockingPort) { return dockingPort->Get<std::string>("referenceAttachNode"); }
}  // namespace

// DockingPortAsDecoupler.cs:18
bool DockingPortAsDecoupler::TestCondition() {
    failedParts.clear();
    if (PreFlightEnvironment::Current().Root() != nullptr) RecurseHierarchy(PreFlightEnvironment::Current().Root());
    return failedParts.empty();
}
// DockingPortAsDecoupler.cs:27
bool DockingPortAsDecoupler::RecurseHierarchy(Part* part) {
    bool flag = false;
    for (size_t i = 0; i < part->children.size(); i++)
        if (RecurseHierarchy(part->children[i])) flag = true;
    bool flag2 = false;
    PartModule* dockingPort = part->FindModuleOfClass("ModuleDockingNode");   // part.isDockingPort(out dockingPort)
    PartModule* moduleCommand = part->FindModuleOfClass("ModuleCommand");
    if (flag || moduleCommand != nullptr) flag2 = true;
    if (dockingPort != nullptr) {
        AttachNode* attachNode = part->FindAttachNode(ReferenceNodeOf(dockingPort));
        if (attachNode != nullptr && attachNode->attachedPart == part->parent) {
            if (part->children.size() == 1) {
                if (IsStageSeparator(*part->children[0])) {
                    PartModule* dockingPort2 = part->children[0]->FindModuleOfClass("ModuleDockingNode");
                    if (!(dockingPort2 != nullptr)) return flag2;
                    if (part->children[0]->FindAttachNode(ReferenceNodeOf(dockingPort2)) != nullptr) return flag2;
                }
                if (!flag2) {
                    AddToFailedParts(part, "---DockingAsDecoupler  : 1");
                    return false;
                }
            }
        } else {
            if (part->parent != nullptr && IsStageSeparator(*part->parent)) {
                PartModule* dockingPort3 = part->parent->FindModuleOfClass("ModuleDockingNode");
                if (dockingPort3 != nullptr) {
                    if (part->parent->FindAttachNode(ReferenceNodeOf(dockingPort3)) != nullptr && attachNode != nullptr) return flag2;
                } else if (attachNode != nullptr) {
                    return flag2;
                }
            }
            if (attachNode != nullptr && attachNode->attachedPart != nullptr && !flag2) {
                AddToFailedParts(part, "---DockingAsDecoupler  : 2");
                return false;
            }
        }
    }
    return flag2;
}
// DockingPortAsDecoupler.cs:84
void DockingPortAsDecoupler::AddToFailedParts(Part* part, const std::string&) {
    if (!Contains(failedParts, part)) failedParts.push_back(part);
}
DesignConcernSeverity DockingPortAsDecoupler::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string DockingPortAsDecoupler::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_251475" : "#autoLOC_251476"; }
std::string DockingPortAsDecoupler::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_251482" : "#autoLOC_251483"; }

// DockingPortFacing.cs:18
bool DockingPortFacing::TestCondition() {
    failedParts.clear();
    if (PreFlightEnvironment::Current().Root() != nullptr) RecurseHierarchy(PreFlightEnvironment::Current().Root());
    return failedParts.empty();
}
// DockingPortFacing.cs:27
bool DockingPortFacing::RecurseHierarchy(Part* part) {
    bool flag = false;
    for (size_t i = 0; i < part->children.size(); i++)
        if (RecurseHierarchy(part->children[i])) flag = true;
    bool result = false;
    PartModule* dockingPort = part->FindModuleOfClass("ModuleDockingNode");
    PartModule* moduleCommand = part->FindModuleOfClass("ModuleCommand");
    if (flag || moduleCommand != nullptr) result = true;
    if (dockingPort != nullptr) {
        AttachNode* attachNode = part->FindAttachNode(ReferenceNodeOf(dockingPort));
        if (attachNode != nullptr && attachNode->attachedPart == part->parent) {
            if (part->children.empty()) {
                AddToFailedParts(part, "---Docking  : 1");
                return false;
            }
            if (part->children.size() == 1 && IsStageSeparator(*part->children[0])) {
                PartModule* dockingPort2 = part->children[0]->FindModuleOfClass("ModuleDockingNode");
                if (!(dockingPort2 != nullptr)) {
                    AddToFailedParts(part, "---Docking  : 2b");
                    return false;
                }
                if (part->children[0]->FindAttachNode(ReferenceNodeOf(dockingPort2)) != nullptr) {
                    AddToFailedParts(part, "---Docking  : 2");
                    return false;
                }
            }
        } else {
            if (part->parent != nullptr && IsStageSeparator(*part->parent)) {
                PartModule* dockingPort3 = part->parent->FindModuleOfClass("ModuleDockingNode");
                if (dockingPort3 != nullptr) {
                    if (part->parent->FindAttachNode(ReferenceNodeOf(dockingPort3)) != nullptr && attachNode != nullptr) {
                        AddToFailedParts(part, "---Docking  : 3");
                        return false;
                    }
                } else if (attachNode != nullptr) {
                    AddToFailedParts(part, "---Docking  : 3b");
                    return false;
                }
            }
            if (part->srfAttachNode != nullptr && part->srfAttachNode->attachedPart != nullptr && attachNode != nullptr) {
                const Vector3f normalized = (part->srfAttachNode->attachedPart->partTransform.position - part->partTransform.position).normalized();
                const Vector3f normalized2 = part->partTransform.TransformDirection(attachNode->orientation).normalized();
                if (Vector3f::Angle(normalized, normalized2) < 100.0f) {
                    AddToFailedParts(part, "---Docking  : 4");
                    return false;
                }
            }
        }
    }
    return result;
}
// DockingPortFacing.cs:110
void DockingPortFacing::AddToFailedParts(Part* part, const std::string&) {
    if (!Contains(failedParts, part)) failedParts.push_back(part);
}
DesignConcernSeverity DockingPortFacing::GetSeverity() { return DesignConcernSeverity::CRITICAL; }
std::string DockingPortFacing::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_251638" : "#autoLOC_251639"; }
std::string DockingPortFacing::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_251645" : "#autoLOC_251646"; }

// ---- Electric*.cs -------------------------------------------------------------------------------------------------------------------------

// ElectricBatteryAndNoCharge.cs:18
bool ElectricBatteryAndNoCharge::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    alternatorException = false;
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (part->Resources.Contains("ElectricCharge") && !HasModule(*part, "ModuleCommand") && !HasModule(*part, "ModuleAlternator")) failedParts.push_back(part);
    }
    if (!failedParts.empty()) {
        for (int j = 0; j < count; j++) {
            Part* part = ship[static_cast<size_t>(j)];
            if (!HasModule(*part, "ModuleDeployableSolarPanel") && (!(part->gameObjectName != "launchClamp1") || !HasModule(*part, "ModuleGenerator") || !GeneratorOutputsElectricCharge(*part))) {
                if (!alternatorException && HasModule(*part, "ModuleAlternator")) alternatorException = true;
                continue;
            }
            failedParts.clear();
            return true;
        }
    }
    return failedParts.empty();
}
DesignConcernSeverity ElectricBatteryAndNoCharge::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string ElectricBatteryAndNoCharge::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_251805" : "#autoLOC_251806"; }
std::string ElectricBatteryAndNoCharge::ConcernDescriptionTag() {
    if (alternatorException) return failedParts.size() == 1 ? "#autoLOC_251814" : "#autoLOC_251815";
    return failedParts.size() == 1 ? "#autoLOC_251820" : "#autoLOC_251821";
}

// ElectricChargeAndNoBattery.cs:15
bool ElectricChargeAndNoBattery::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (IsElectricChargeProducer(*part)) failedParts.push_back(part);
    }
    if (!failedParts.empty()) {
        for (int j = 0; j < count; j++) {
            Part* part = ship[static_cast<size_t>(j)];
            if (part->Resources.Contains("ElectricCharge") && !HasModule(*part, "ModuleCommand") && !HasModule(*part, "ModuleAlternator")) {
                failedParts.clear();
                return true;
            }
        }
    }
    return failedParts.empty();
}
DesignConcernSeverity ElectricChargeAndNoBattery::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string ElectricChargeAndNoBattery::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_251890" : "#autoLOC_251891"; }
std::string ElectricChargeAndNoBattery::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_251897" : "#autoLOC_251898"; }

// ElectricChargeAndNoConsumer.cs:16
ElectricChargeAndNoConsumer::ElectricChargeAndNoConsumer() {
    resourceDefinition = PartResourceLibrary::Instance()->GetDefinition("ElectricCharge");
}
// ElectricChargeAndNoConsumer.cs:22
bool ElectricChargeAndNoConsumer::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (IsElectricChargeProducer(*part)) failedParts.push_back(part);
    }
    if (!failedParts.empty()) {
        for (int j = 0; j < count; j++) {
            Part* part = ship[static_cast<size_t>(j)];
            if (!HasModule(*part, "ModuleCommand") && ConsumesResource(*part, resourceDefinition)) {
                failedParts.clear();
                return true;
            }
        }
    }
    return failedParts.empty();
}
DesignConcernSeverity ElectricChargeAndNoConsumer::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string ElectricChargeAndNoConsumer::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_251969" : "#autoLOC_251970"; }
std::string ElectricChargeAndNoConsumer::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_251976" : "#autoLOC_251977"; }

// ElectricConsumerAndNoCharge.cs:18
ElectricConsumerAndNoCharge::ElectricConsumerAndNoCharge() {
    resourceDefinition = PartResourceLibrary::Instance()->GetDefinition(Localizer::Format("#autoLOC_252004"));
}
// ElectricConsumerAndNoCharge.cs:24
bool ElectricConsumerAndNoCharge::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    alternatorException = false;
    const int count = ship.LiveCount();
    for (int num = 0; num < count; num++) {
        Part* part = ship[static_cast<size_t>(num)];
        if (IsElectricChargeProducer(*part)) return true;
    }
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (!HasModule(*part, "ModuleCommand")) {
            if (ConsumesResource(*part, resourceDefinition)) failedParts.push_back(part);
            if (!alternatorException && HasModule(*part, "ModuleAlternator")) alternatorException = true;
        }
    }
    return failedParts.empty();
}
DesignConcernSeverity ElectricConsumerAndNoCharge::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string ElectricConsumerAndNoCharge::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_252048" : "#autoLOC_252049"; }
std::string ElectricConsumerAndNoCharge::ConcernDescriptionTag() {
    if (alternatorException) return failedParts.size() == 1 ? "#autoLOC_252057" : "#autoLOC_252058";
    return "#autoLOC_252063";
}

// ---- EnginesJettisonedBeforeUse.cs ---------------------------------------------------------------------------------------------------------

// EnginesJettisonedBeforeUse.cs:19
bool EnginesJettisonedBeforeUse::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if ((part->isEngine() || HasModule(*part, "ModuleEnginesFX")) && RecurseHierarchy(part, part) != nullptr) failedParts.push_back(part);
    }
    return failedParts.empty();
}
// EnginesJettisonedBeforeUse.cs:35
Part* EnginesJettisonedBeforeUse::RecurseHierarchy(Part* p, Part* orig) {
    if (p->parent == nullptr) return nullptr;
    if (HasModule(*p->parent, "ModuleDecouple") && p->parent->fields.inStageIndex != -1 && p->parent->fields.inverseStage >= orig->fields.inverseStage) return p;
    return RecurseHierarchy(p->parent, orig);
}
DesignConcernSeverity EnginesJettisonedBeforeUse::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string EnginesJettisonedBeforeUse::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_252125" : "#autoLOC_252126"; }
std::string EnginesJettisonedBeforeUse::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_252132" : "#autoLOC_252133"; }

// ---- ExperimentalPartsAvailable.cs ----------------------------------------------------------------------------------------------------------

namespace {
void AddExperimental(std::vector<std::pair<const AvailablePart*, int>>& v, const AvailablePart* ap) {
    for (auto& e : v)
        if (e.first == ap) {
            e.second += 1;
            return;
        }
    v.emplace_back(ap, 1);
}
bool NotPurchased(const AvailablePart& info) {
    PreFlightEnvironment& env = PreFlightEnvironment::Current();
    std::optional<PreFlightEnvironment::TechState> ts = env.techState ? env.techState(info.TechRequired) : std::nullopt;
    if (!ts || !ts->available) return true;
    return std::find(ts->partsPurchased.begin(), ts->partsPurchased.end(), info.name) == ts->partsPurchased.end();
}
}  // namespace

// ExperimentalPartsAvailable.cs:14
ExperimentalPartsAvailable::ExperimentalPartsAvailable(ShipConstruct& ship) {
    if (!PreFlightEnvironment::Current().researchAndDevelopmentPresent) return;
    hasDictionary = true;
    int i = ship.LiveCount();
    while (i-- > 0) {
        const AvailablePart& info = *ship.liveParts[static_cast<size_t>(i)]->partInfo;
        if (NotPurchased(info)) AddExperimental(expPartsOnShip, &info);
    }
}
// ExperimentalPartsAvailable.cs:39
ExperimentalPartsAvailable::ExperimentalPartsAvailable(VesselCrewManifest& manifest) {
    if (!PreFlightEnvironment::Current().researchAndDevelopmentPresent) return;
    hasDictionary = true;
    int count = static_cast<int>(manifest.PartManifests().size());
    while (count-- > 0) {
        const AvailablePart& info = *manifest.PartManifests()[static_cast<size_t>(count)]->PartInfo();
        if (NotPurchased(info)) AddExperimental(expPartsOnShip, &info);
    }
}
// ExperimentalPartsAvailable.cs:64
bool ExperimentalPartsAvailable::Test() {
    PreFlightEnvironment& env = PreFlightEnvironment::Current();
    if (env.researchAndDevelopmentPresent) {
        for (const auto& e : expPartsOnShip)
            if (!(env.isExperimentalPart && env.isExperimentalPart(*e.first))) return false;
        return true;
    }
    return true;
}
std::string ExperimentalPartsAvailable::GetWarningTitle() { return Localizer::Format("#autoLOC_253217"); }
std::string ExperimentalPartsAvailable::GetWarningDescription() {
    PreFlightEnvironment& env = PreFlightEnvironment::Current();
    std::string text = Localizer::Format("#autoLOC_253222");
    for (const auto& e : expPartsOnShip)
        if (!(env.isExperimentalPart && env.isExperimentalPart(*e.first))) text += "<color=orange><b>" + std::to_string(e.second) + "x " + e.first->title + "</b></color>\n";
    return text;
}
std::string ExperimentalPartsAvailable::GetAbortOption() { return Localizer::Format("#autoLOC_253243"); }

// ---- FacilityOperational.cs -----------------------------------------------------------------------------------------------------------------

// FacilityOperational.cs:12
FacilityOperational::FacilityOperational(const std::string& FacilityName_, const std::string& FacilityTitle) : facilityName(FacilityName_), facilityTitle(FacilityTitle) {
    PreFlightEnvironment& env = PreFlightEnvironment::Current();
    std::optional<float> damage = env.facilityDamage ? env.facilityDamage(facilityName) : std::nullopt;
    if (damage) {
        operational = *damage < 100.0f;
        return;
    }
    if (env.isLaunchSite && env.isLaunchSite(FacilityName_)) {
        operational = true;
        return;
    }
    operational = false;   // Debug.LogError("[ConstructionFacilityOperational]: Invalid Test Parameters! ...")
}
std::string FacilityOperational::GetWarningTitle() { return Localizer::Format("#autoLOC_253284", {facilityTitle}); }
std::string FacilityOperational::GetWarningDescription() { return Localizer::Format("#autoLOC_253289", {facilityTitle}); }
std::string FacilityOperational::GetAbortOption() { return Localizer::Format("#autoLOC_253299"); }

// ---- HatchObstructed.cs ---------------------------------------------------------------------------------------------------------------------

// HatchObstructed.cs:17
bool HatchObstructed::TestCondition() {
    ShipConstruct& ship = SHIP();
    PreFlightEnvironment& env = PreFlightEnvironment::Current();
    failedParts.clear();
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (part->fields.CrewCapacity <= 0 || !(env.hasAirlock && env.hasAirlock(*part)) || !(env.hatchIsObstructedMore && env.hatchIsObstructedMore(*part))) continue;
        if (part->partInfo->name == "MK1CrewCabin") {
            AttachNode* attachNode = part->FindAttachNode("top");
            if (attachNode != nullptr && attachNode->attachedPart != nullptr && attachNode->attachedPart->fields.CrewCapacity > 0) continue;
        }
        failedParts.push_back(part);
    }
    return failedParts.empty();
}
DesignConcernSeverity HatchObstructed::GetSeverity() { return DesignConcernSeverity::CRITICAL; }
std::string HatchObstructed::ConcernTitleTag() { return "#autoLOC_252190"; }
std::string HatchObstructed::ConcernDescriptionTag() { return "#autoLOC_252195"; }

// ---- KerbalSeatCollision.cs -----------------------------------------------------------------------------------------------------------------

// KerbalSeatCollision.cs:12
bool KerbalSeatCollision::TestCondition() {
    VesselCrewManifest* shipManifest = ShipConstruction::ShipManifest;
    if (shipManifest == nullptr) return true;
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    PreFlightEnvironment& env = PreFlightEnvironment::Current();
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (part->fields.CrewCapacity <= 0) continue;
        auto pcm = shipManifest->GetPartCrewManifest(part->fields.craftID);
        if (pcm != nullptr && pcm->AnySeatTaken()) {
            PartModule* kerbalSeat = part->FindModuleOfClass("KerbalSeat");
            if (kerbalSeat != nullptr && !(env.editorSeatBoundsAreClear && env.editorSeatBoundsAreClear(*part))) failedParts.push_back(part);
        }
    }
    return failedParts.empty();
}
DesignConcernSeverity KerbalSeatCollision::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string KerbalSeatCollision::ConcernTitleTag() { return "#autoLOC_8004252"; }
std::string KerbalSeatCollision::ConcernDescriptionTag() { return "#autoLOC_8004253"; }

// ---- LadderPresent.cs / LandingGearPresent.cs ------------------------------------------------------------------------------------------------

// LadderPresent.cs:11
bool LadderPresent::TestCondition() {
    ShipConstruct& ship = SHIP();
    const int count = ship.LiveCount();
    bool flag = false;
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (HasModule(*part, "ModuleWheelBase")) {
            flag = true;
            break;
        }
    }
    if (flag) {
        for (int num = 0; num < count; num++) {
            Part* part = ship[static_cast<size_t>(num)];
            std::string lower = part->gameObjectName;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (HasModule(*part, "RetractableLadder") || lower.find("ladder") != std::string::npos) return true;
        }
        return false;
    }
    return true;
}
DesignConcernSeverity LadderPresent::GetSeverity() { return DesignConcernSeverity::NOTICE; }
std::string LadderPresent::ConcernTitleTag() { return "#autoLOC_252377"; }
std::string LadderPresent::ConcernDescriptionTag() { return "#autoLOC_252382"; }

// LandingGearPresent.cs:14 (WheelType: FREE = 0, MOTORIZED = 1, LEG = 2)
bool LandingGearPresent::TestCondition() {
    ShipConstruct& ship = SHIP();
    roverWheelsPresent = false;
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        PartModule* wheel = ship[static_cast<size_t>(i)]->FindModuleOfClass("ModuleWheelBase");
        if (wheel != nullptr) {
            switch (wheel->Get<int32_t>("wheelType")) {
            case 1: roverWheelsPresent = true; break;
            case 0: return true;
            default: break;
            }
        }
    }
    return false;
}
DesignConcernSeverity LandingGearPresent::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string LandingGearPresent::ConcernTitleTag() { return roverWheelsPresent ? "#autoLOC_252437" : "#autoLOC_252438"; }
std::string LandingGearPresent::ConcernDescriptionTag() { return roverWheelsPresent ? "#autoLOC_252444" : "#autoLOC_252445"; }

// ---- LaunchSiteClear.cs ----------------------------------------------------------------------------------------------------------------------

LaunchSiteClear::LaunchSiteClear(const std::string& SiteName, const std::string& SiteDisplayName)
    : siteName(SiteName), siteDisplayName(Localizer::Format(SiteDisplayName)) {}
// LaunchSiteClear.cs:34 (the Game is PreFlightEnvironment: flightState null / not compatible -> true)
bool LaunchSiteClear::Test() {
    stValid = false;
    PreFlightEnvironment& env = PreFlightEnvironment::Current();
    if (env.flightState == nullptr) return true;
    if (!env.gameCompatible) return true;
    stValid = true;
    int count = 0;
    VesselType vType = VesselType::Debris;
    ShipConstruction::FindVesselsLandedAt(env.flightState, siteName, count, obstructingVesselName, obstructingVesselIndex, vType);
    if (count == 0) return true;
    if (vType == VesselType::Debris) return true;
    return false;
}
std::string LaunchSiteClear::GetWarningTitle() { return Localizer::Format("#autoLOC_253369", {siteDisplayName}); }
std::string LaunchSiteClear::GetWarningDescription() { return Localizer::Format("#autoLOC_253374", {obstructingVesselName, siteDisplayName}); }
std::optional<std::string> LaunchSiteClear::GetProceedOption() { return Localizer::Format("#autoLOC_253379", {siteDisplayName, obstructingVesselName}); }
std::string LaunchSiteClear::GetAbortOption() { return Localizer::Format("#autoLOC_253384"); }

// ---- MissingCrew.cs --------------------------------------------------------------------------------------------------------------------------

// MissingCrew.cs:20
bool MissingCrew::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    VesselCrewManifest* manifest = ShipConstruction::ShipManifest;
    if (manifest == nullptr) return false;
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        auto pcm = manifest->GetPartCrewManifest(part->fields.craftID);
        if (pcm != nullptr && pcm->PartInfo()->partPrefab->fields.CrewCapacity > 0 && part->fields.CrewCapacity > 0 && pcm->AllSeatsEmpty()) failedParts.push_back(part);
    }
    return failedParts.empty();
}
DesignConcernSeverity MissingCrew::GetSeverity() { return DesignConcernSeverity::NOTICE; }
std::string MissingCrew::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_252504" : "#autoLOC_252505"; }
std::string MissingCrew::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_252511" : "#autoLOC_252512"; }

// ---- NoControlSources.cs / NoCrewInMannedVessel.cs --------------------------------------------------------------------------------------------

NoControlSources::NoControlSources() : manifest(ShipConstruction::ShipManifest), updateManifest(true) {}
NoControlSources::NoControlSources(VesselCrewManifest* crewManifest) : manifest(crewManifest) {}
NoControlSources::NoControlSources(VesselCrewManifest* crewManifest, KerbalRoster* r) : manifest(crewManifest), roster(r) {}
// NoControlSources.cs:35
bool NoControlSources::TestCondition() {
    if (updateManifest) manifest = ShipConstruction::ShipManifest;
    if (manifest == nullptr) return false;
    KerbalRoster* useRoster = roster != nullptr ? roster : PreFlightEnvironment::Current().roster;
    int count = static_cast<int>(manifest->PartManifests().size());
    while (true) {
        if (count-- > 0) {
            auto pcm = manifest->PartManifests()[static_cast<size_t>(count)];
            auto crewNotTourist = [&]() {
                return useRoster != nullptr ? pcm->CountCrewNotType(ProtoCrewMember_KerbalType::Tourist, *useRoster) : pcm->CountCrewNotType(ProtoCrewMember_KerbalType::Tourist);
            };
            if (pcm->PartInfo()->partPrefab->fields.isControlSource > 0) {
                PartModule* moduleCommand = pcm->PartInfo()->partPrefab->FindModuleOfClass("ModuleCommand");
                if (!(moduleCommand != nullptr)) return true;
                if (moduleCommand->Get<int32_t>("minimumCrew") <= crewNotTourist()) return true;
            } else if (!pcm->NoSeats() && crewNotTourist() > 0 && pcm->PartInfo()->partPrefab->FindModuleOfClass("KerbalSeat") != nullptr) {
                break;
            }
            continue;
        }
        return false;
    }
    return true;
}
std::string NoControlSources::GetWarningTitle() { return Localizer::Format("#autoLOC_253486"); }
std::string NoControlSources::GetWarningDescription() { return Localizer::Format("#autoLOC_253491"); }
std::optional<std::string> NoControlSources::GetProceedOption() { return Localizer::Format("#autoLOC_253496"); }
std::string NoControlSources::GetAbortOption() { return Localizer::Format("#autoLOC_253501"); }

NoCrewInMannedVessel::NoCrewInMannedVessel() : manifest(ShipConstruction::ShipManifest), updateManifest(true) {}
NoCrewInMannedVessel::NoCrewInMannedVessel(VesselCrewManifest* crewManifest) : manifest(crewManifest) {}
// NoCrewInMannedVessel.cs:36
bool NoCrewInMannedVessel::TestCondition() {
    if (updateManifest) manifest = ShipConstruction::ShipManifest;
    bool flag = false;
    if (manifest == nullptr) return false;
    int count = static_cast<int>(manifest->PartManifests().size());
    while (true) {
        if (count-- > 0) {
            auto pcm = manifest->PartManifests()[static_cast<size_t>(count)];
            if (!pcm->AnySeatTaken()) {
                if (pcm->PartInfo()->partPrefab->fields.isControlSource > 0) {
                    if (pcm->PartInfo()->partPrefab->fields.CrewCapacity == 0) break;
                    flag = true;
                }
                continue;
            }
            return true;
        }
        if (flag) return false;
        return true;
    }
    return true;
}
std::string NoCrewInMannedVessel::GetWarningTitle() { return Localizer::Format("#autoLOC_253598"); }
std::string NoCrewInMannedVessel::GetWarningDescription() { return Localizer::Format("#autoLOC_253603"); }
std::optional<std::string> NoCrewInMannedVessel::GetProceedOption() { return Localizer::Format("#autoLOC_253608"); }
std::string NoCrewInMannedVessel::GetAbortOption() { return Localizer::Format("#autoLOC_253613"); }
std::string NoCrewInMannedVessel::GetTestName() { return Localizer::Format("#autoLOC_253618"); }

// ---- NonRootCmdMissaligned.cs -------------------------------------------------------------------------------------------------------------------

// NonRootCmdMissaligned.cs:21
bool NonRootCmdMissaligned::TestCondition() {
    ShipConstruct& ship = SHIP();
    Part* root = PreFlightEnvironment::Current().Root();
    failedParts.clear();
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (part != root && HasModule(*part, "ModuleCommand") &&
            (Vector3f::Angle(part->partTransform.rotation * Vector3f::up(), root->partTransform.rotation * Vector3f::up()) > 5.0f ||
             Vector3f::Angle(part->partTransform.rotation * Vector3f::forward(), root->partTransform.rotation * Vector3f::forward()) > 5.0f))
            failedParts.push_back(part);
    }
    return failedParts.empty();
}
DesignConcernSeverity NonRootCmdMissaligned::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string NonRootCmdMissaligned::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_252566" : "#autoLOC_252567"; }
std::string NonRootCmdMissaligned::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_252573" : "#autoLOC_252574"; }

// ---- Parachute*.cs -----------------------------------------------------------------------------------------------------------------------------

// ParachuteOnEngineStage.cs:19
bool ParachuteOnEngineStage::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    std::vector<Part*> list, engines;
    for (Part* a : ship.liveParts) {
        if (HasModule(*a, "ModuleParachute")) list.push_back(a);
        if (HasModule(*a, "ModuleEngines") || HasModule(*a, "ModuleEnginesFX")) engines.push_back(a);
    }
    for (Part* a : list) {
        bool any = false;
        for (Part* b : engines)
            if (a->fields.inverseStage == b->fields.inverseStage) any = true;
        if (any) failedParts.push_back(a);
    }
    return failedParts.empty();
}
DesignConcernSeverity ParachuteOnEngineStage::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string ParachuteOnEngineStage::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_252616" : "#autoLOC_252617"; }
std::string ParachuteOnEngineStage::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_252623" : "#autoLOC_252624"; }

// ParachuteOnFirstStage.cs:17
bool ParachuteOnFirstStage::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (part->isParachute() && part->fields.inverseStage == StageManager::LastStage()) failedParts.push_back(part);
    }
    return failedParts.empty();
}
DesignConcernSeverity ParachuteOnFirstStage::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string ParachuteOnFirstStage::ConcernTitleTag() { return failedParts.size() == 1 ? "#autoLOC_252675" : "#autoLOC_252676"; }
std::string ParachuteOnFirstStage::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_252682" : "#autoLOC_252683"; }

// ParachutePresent.cs:22
bool ParachutePresent::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    VesselCrewManifest* manifest = ShipConstruction::ShipManifest;
    chutePresentInSameStage = false;
    if (manifest == nullptr) return false;
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* p = ship[static_cast<size_t>(i)];
        auto pcm = manifest->GetPartCrewManifest(p->fields.craftID);
        bool childChute = false;
        for (Part* a : p->children)
            if (HasModule(*a, "ModuleParachute")) childChute = true;
        if (pcm != nullptr && pcm->PartInfo()->partPrefab->fields.isControlSource > 0 && p != nullptr && p->fields.CrewCapacity > 0 && HasModule(*p, "ModuleCommand") &&
            pcm->AnySeatTaken() && (!(p->parent != nullptr) || !p->parent->isParachute()) && !childChute) {
            for (Part* a : ship.liveParts)
                if (a->fields.separationIndex == p->fields.separationIndex && HasModule(*a, "ModuleParachute")) {
                    chutePresentInSameStage = true;
                    break;
                }
            failedParts.push_back(p);
        }
    }
    return failedParts.empty();
}
DesignConcernSeverity ParachutePresent::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string ParachutePresent::ConcernTitleTag() { return chutePresentInSameStage ? "#autoLOC_252764" : "#autoLOC_252765"; }
std::string ParachutePresent::ConcernDescriptionTag() {
    if (chutePresentInSameStage) return failedParts.size() == 1 ? "#autoLOC_252773" : "#autoLOC_252774";
    return failedParts.size() == 1 ? "#autoLOC_252779" : "#autoLOC_252780";
}

// ---- RequiredIScienceDataTransmitter.cs ---------------------------------------------------------------------------------------------------------

// RequiredIScienceDataTransmitter.cs:15 (IScienceDataTransmitter implementers: ModuleDataTransmitter, ModuleRTAntenna-style mods are not stock)
bool RequiredIScienceDataTransmitter::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    bool any = false;
    for (Part* a : ship.liveParts)
        if (HasModule(*a, "ModuleDataTransmitter")) any = true;
    if (!any)
        for (Part* a : ship.liveParts)
            if (HasModule(*a, "ModuleOrbitalSurveyor")) failedParts.push_back(a);
    return failedParts.empty();
}
DesignConcernSeverity RequiredIScienceDataTransmitter::GetSeverity() { return DesignConcernSeverity::CRITICAL; }
std::string RequiredIScienceDataTransmitter::ConcernTitleTag() { return "#autoLOC_252827"; }
std::string RequiredIScienceDataTransmitter::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_252834" : "#autoLOC_252835"; }

// ---- ResourceConsumersReachable.cs / ResourceContainersReachable.cs ---------------------------------------------------------------------------

namespace {
bool IsStackFlow(const PartResourceDefinition* d) {
    const auto m = static_cast<ResourceFlowMode>(d->resourceFlowMode());
    return m == ResourceFlowMode::STACK_PRIORITY_SEARCH || m == ResourceFlowMode::STAGE_STACK_FLOW || m == ResourceFlowMode::STAGE_STACK_FLOW_BALANCE;
}
std::vector<Part*> ToVector(const PartHashSetPtr& set) { return set ? set->ToList() : std::vector<Part*>{}; }
}  // namespace

ResourceConsumersReachable::ResourceConsumersReachable(const PartResourceDefinition* def) : resourceDefinition(def) {}
// ResourceConsumersReachable.cs:32
bool ResourceConsumersReachable::TestCondition() {
    ShipConstruct& ship = SHIP();
    exceptionCommandPod = false;
    failedParts.clear();
    if (!IsStackFlow(resourceDefinition)) {
        const int count = ship.LiveCount();
        for (int i = 0; i < count; i++) {
            Part* part = ship[static_cast<size_t>(i)];
            if (part->Resources.Contains(resourceDefinition->id()) && part->Resources.Get(resourceDefinition->id())->amount > 0.0) {
                if (!(resourceDefinition->name() == "MonoPropellant") || !HasModule(*part, "ModuleCommand")) {
                    failedParts.clear();
                    return true;
                }
                exceptionCommandPod = true;
            }
            if (ConsumesResource(*part, resourceDefinition)) failedParts.push_back(part);
        }
    } else {
        auto& finder = PreFlightEnvironment::Current().sccFlowGraphUCFinder;
        failedParts = finder ? ToVector(finder->GetUnreachableFuelRequests(resourceDefinition->id())) : std::vector<Part*>{};
    }
    return failedParts.empty();
}
DesignConcernSeverity ResourceConsumersReachable::GetSeverity() { return DesignConcernSeverity::WARNING; }
std::string ResourceConsumersReachable::ConcernTitleTag() { return ""; }
std::string ResourceConsumersReachable::ConcernDescriptionTag() { return ""; }
std::string ResourceConsumersReachable::GetConcernTitle() {
    const std::string dn = resourceDefinition->displayName();
    if (exceptionCommandPod) return Localizer::Format(failedParts.size() == 1 ? "#autoLOC_252942" : "#autoLOC_252943", {dn});
    return Localizer::Format(failedParts.size() == 1 ? "#autoLOC_252948" : "#autoLOC_252949", {dn});
}
std::string ResourceConsumersReachable::GetConcernDescription() {
    const std::string dn = resourceDefinition->displayName();
    if (exceptionCommandPod) {
        if (failedParts.size() == 1) return Localizer::Format("#autoLOC_252958", {dn, dn});
        return Localizer::Format("#autoLOC_252959", {std::to_string(failedParts.size()), dn, dn});
    }
    if (failedParts.size() == 1) return Localizer::Format("#autoLOC_252964", {dn});
    return Localizer::Format("#autoLOC_252965", {std::to_string(failedParts.size()), dn});
}

ResourceContainersReachable::ResourceContainersReachable(const PartResourceDefinition* def) : resourceDefinition(def) {}
// ResourceContainersReachable.cs:23
bool ResourceContainersReachable::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    if (!IsStackFlow(resourceDefinition)) {
        const int count = ship.LiveCount();
        for (int i = 0; i < count; i++) {
            Part* part = ship[static_cast<size_t>(i)];
            if (part->Resources.Contains(resourceDefinition->id()) && part->Resources.Get(resourceDefinition->id())->amount > 0.0) failedParts.push_back(part);
            if (ConsumesResource(*part, resourceDefinition)) {
                failedParts.clear();
                return true;
            }
        }
    } else {
        auto& finder = PreFlightEnvironment::Current().sccFlowGraphUCFinder;
        failedParts = finder ? ToVector(finder->GetUnreachableFuelDeliveries(resourceDefinition->id())) : std::vector<Part*>{};
    }
    return failedParts.empty();
}
DesignConcernSeverity ResourceContainersReachable::GetSeverity() { return DesignConcernSeverity::NOTICE; }
std::string ResourceContainersReachable::ConcernTitleTag() { return ""; }
std::string ResourceContainersReachable::ConcernDescriptionTag() { return ""; }
std::string ResourceContainersReachable::GetConcernTitle() {
    return Localizer::Format("#autoLOC_6001044", {resourceDefinition->displayName(), std::to_string(failedParts.size())});
}
std::string ResourceContainersReachable::GetConcernDescription() {
    const std::string dn = resourceDefinition->displayName();
    if (failedParts.size() == 1) return Localizer::Format("#autoLOC_253062", {dn, dn});
    return Localizer::Format("#autoLOC_253063", {std::to_string(failedParts.size()), dn, dn});
}

// ---- StationHubAttachments.cs -----------------------------------------------------------------------------------------------------------------

// StationHubAttachments.cs:15
bool StationHubAttachments::TestCondition() {
    ShipConstruct& ship = SHIP();
    failedParts.clear();
    const int count = ship.LiveCount();
    for (int i = 0; i < count; i++) {
        Part* part = ship[static_cast<size_t>(i)];
        if (part->gameObjectName == "stationHub") {
            bool anyFree = false;
            for (const AttachNode& b : part->attachNodes)
                if (b.attachedPart == nullptr) anyFree = true;
            if (anyFree) failedParts.push_back(part);
        }
    }
    return failedParts.empty();
}
DesignConcernSeverity StationHubAttachments::GetSeverity() { return DesignConcernSeverity::NOTICE; }
std::string StationHubAttachments::ConcernTitleTag() { return "#autoLOC_253109"; }
std::string StationHubAttachments::ConcernDescriptionTag() { return failedParts.size() == 1 ? "#autoLOC_253115" : "#autoLOC_253116"; }

// ---- WrongVesselTypeForLaunchSite.cs ----------------------------------------------------------------------------------------------------------

WrongVesselTypeForLaunchSite::WrongVesselTypeForLaunchSite(EditorFacility CorrectType, const std::string& CraftFileName, const std::string& VesselTypeName,
                                                           const std::string& LaunchSiteName)
    : correctType(CorrectType), craftFile(CraftFileName), vesselType(VesselTypeName), launchSiteName(LaunchSiteName) {}
// WrongVesselTypeForLaunchSite.cs:23
bool WrongVesselTypeForLaunchSite::Test() { return ShipConstruction::CheckCraftFileType(craftFile) == correctType; }
std::string WrongVesselTypeForLaunchSite::GetWarningTitle() { return Localizer::Format("#autoLOC_253804"); }
std::string WrongVesselTypeForLaunchSite::GetWarningDescription() { return Localizer::Format("#autoLOC_253809", {vesselType, launchSiteName}); }
std::optional<std::string> WrongVesselTypeForLaunchSite::GetProceedOption() { return Localizer::Format("#autoLOC_253814"); }
std::string WrongVesselTypeForLaunchSite::GetAbortOption() { return Localizer::Format("#autoLOC_253819"); }

}  // namespace PreFlightTests

// ==================================================================================================================================
// PreFlightCheck.cs
// ==================================================================================================================================

// PreFlightCheck.cs:29
bool PreFlightCheck::RunTests() {
    currentTest = -1;
    failedTest = nullptr;
    runTestResult = tests.empty();
    if (!tests.empty()) runNextTest();
    return runTestResult;
}

// PreFlightCheck.cs:43
void PreFlightCheck::runNextTest() {
    currentTest++;
    if (currentTest < static_cast<int>(tests.size())) {
        PreFlightTests::IPreFlightTest& t = *tests[static_cast<size_t>(currentTest)];
        const std::string name = t.TypeName();
        const bool flag = name == "WrongVesselTypeForLaunchSite";
        if (t.Test()) {
            log.push_back("[Pre-Flight Check]: Checking for " + name + ": PASS!");
            runNextTest();
            return;
        }
        const bool show = showWrongVesselTypeConfirmation != nullptr ? *showWrongVesselTypeConfirmation
                                                                       : PreFlightTests::PreFlightEnvironment::Current().showWrongVesselTypeConfirmation;
        if (flag && !show) {
            log.push_back("[Pre-Flight Check]: Checking for " + name + ": SKIPPED WARNING!");
            runNextTest();
            return;
        }
        failedTest = &t;   // the warning dialog (GetWarningTitle / Description, proceed and abort options) is the shell's
        log.push_back("[Pre-Flight Check]: Checking for " + name + ": FAIL!");
    } else {
        Complete();
        runTestResult = true;
    }
}

// PreFlightCheck.cs:127
void PreFlightCheck::Complete() {
    log.push_back("[Pre-Flight Check]: All Checks Complete. Go for Launch!");
    OnComplete();
}

// PreFlightCheck.cs:135
void PreFlightCheck::Abort() {
    log.push_back("[Pre-Flight Check]: Launch Aborted.");
    OnAbort();
}

}  // namespace ksp
