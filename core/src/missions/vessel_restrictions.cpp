// WP-54 slice 2: Assembly-CSharp/Expansions/Missions/VesselRestriction*.cs, MissionPreflightCheck.cs, MissionBlackListPreFlightCheck.cs.
// See vessel_restrictions.hpp. Float rule: C# float expressions follow the Mono rule (fx() / f32store()).
#include "ksp/missions/vessel_restrictions.hpp"

#include <algorithm>
#include <cmath>

#include "../save/save_util.hpp"
#include "ksp/career/format.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/editor/editor_logic.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp::Expansions::Missions {

using KSP::Localization::Localizer;

void RestrictionEnvironment::Reset() {
    shipCost = nullptr;
    shipTotalMass = nullptr;
    shipPartCount = nullptr;
    shipSize = nullptr;
    stageCount = nullptr;
    shipPartNames = nullptr;
    shipResources = nullptr;
    crewManifest = nullptr;
    firstResourceName = nullptr;
    partTitle = nullptr;
    time = nullptr;
    appCurrentVesselSituation = nullptr;
    setVesselItemReadyIcon = nullptr;
}

namespace {
using R = RestrictionEnvironment;

// EditorLogic.fetch.ship (NullReferenceException when there is no editor)
ShipConstruct* Ship() {
    if (EditorLogic::fetch == nullptr || EditorLogic::fetch->ship == nullptr) throw NullReferenceException();
    return EditorLogic::fetch->ship;
}
std::string FloatText(float v) { return dotnet::FormatSingleG(v); }
std::string N1(double v) { return career::FormatNumber(v, std::string_view("N1")); }
std::string N1(float v) { return career::FormatNumber(v, std::string_view("N1")); }
std::string N2(float v) { return career::FormatNumber(v, std::string_view("N2")); }
std::string I(int v) { return dotnet::FormatInt64(v); }

// the three-way switch every GetStateMessage repeats
struct Msg {
    TestComparisonLessGreaterEqual op;
};
}  // namespace

// ---- VesselRestriction.cs --------------------------------------------------------------------------------------------------------

// VesselRestriction.cs:37
std::string VesselRestriction::GetDisplayName() { return Localizer::Format("#autoLOC_8100211"); }

// VesselRestriction.cs:55
void VesselRestriction::UpdateAppEntry() {
    if (UIEntry.present) {
        UIEntry.text = GetStateMessage();
        UIEntry.state = IsComplete() ? 0 : 1;
        if (!IsComplete() && UIEntry.dataIsVesselInfo) {
            UIEntry.vesselSituation->readyToLaunch = false;
            if (R::setVesselItemReadyIcon) R::setVesselItemReadyIcon(UIEntry.vesselSituation);
        }
    }
}

// VesselRestriction.cs:42
void VesselRestriction::AddAppUIReference(bool dataIsVesselInfo, VesselSituation* vesselSituation) {
    UIEntry.present = true;
    UIEntry.dataIsVesselInfo = dataIsVesselInfo;
    UIEntry.vesselSituation = vesselSituation;
    UpdateAppEntry();
}

// VesselRestriction.cs:49
void VesselRestriction::ClearAppUIREference() { UIEntry = UIEntryRef{}; }

// VesselRestriction.cs:70
std::string VesselRestriction::GetStateMessage() {
    return Localizer::Format("#autoLOC_8100212", {FloatText(R::time ? R::time() : 0.0f)});
}

// VesselRestriction.cs:80
bool VesselRestriction::SameComparatorWrapper(VesselRestriction& otherRestriction) {
    if (TypeName() == otherRestriction.TypeName()) return SameComparator(otherRestriction);
    return false;
}

// VesselRestriction.cs:99 (float Mathf.Abs(target - current) argument is stored)
bool VesselRestriction::TestFloat(float currentValue, float targetValue, TestComparisonLessGreaterEqual comparisonOperator) {
    switch (comparisonOperator) {
        default: return true;
        case TestComparisonLessGreaterEqual::LessOrEqual: return currentValue <= targetValue;
        case TestComparisonLessGreaterEqual::Equal: return static_cast<double>(Mathf::Abs(f32store(fx(targetValue) - fx(currentValue)))) < epsilon;
        case TestComparisonLessGreaterEqual::GreaterOrEqual: return currentValue >= targetValue;
    }
}

// VesselRestriction.cs:115
bool VesselRestriction::TestInt(int currentValue, int targetValue, TestComparisonLessGreaterEqual comparisonOperator) {
    switch (comparisonOperator) {
        default: return true;
        case TestComparisonLessGreaterEqual::LessOrEqual: return currentValue <= targetValue;
        case TestComparisonLessGreaterEqual::Equal: return targetValue - currentValue == 0;
        case TestComparisonLessGreaterEqual::GreaterOrEqual: return currentValue >= targetValue;
    }
}

// ---- VesselRestrictionList.cs ----------------------------------------------------------------------------------------------------

// VesselRestrictionList.cs:9
std::vector<VesselRestriction*> VesselRestrictionList::ActiveRestrictions() {
    std::vector<VesselRestriction*> list;
    for (size_t i = 0; i < activeModules.size(); ++i) list.push_back(dynamic_cast<VesselRestriction*>(activeModules[i].get()));
    return list;
}

// VesselRestrictionList.cs:37: every VesselRestriction subclass (assembly order) unless [VesselRestrictionOptions(listedInSAP = false)]
std::vector<std::string> VesselRestrictionList::GetSupportedTypes() {
    std::vector<std::string> out;
    for (const std::string& n : DynamicModuleRegistry::SubclassesOf("VesselRestriction"))
        if (n != VesselRestriction_RequiredParts::kTypeName) out.push_back(n);
    return out;
}

// VesselRestrictionList.cs:60
void VesselRestrictionList::StartAppEvents() {
    std::vector<VesselRestriction*> activeRestrictions = ActiveRestrictions();
    for (size_t i = 0; i < activeRestrictions.size(); ++i) activeRestrictions[i]->SuscribeToEvents();
}

// VesselRestrictionList.cs:70
void VesselRestrictionList::ClearAppEvents() {
    std::vector<VesselRestriction*> activeRestrictions = ActiveRestrictions();
    for (size_t i = 0; i < activeRestrictions.size(); ++i) activeRestrictions[i]->ClearEvents();
}

// DynamicModuleList.cs:140 Clone
std::unique_ptr<DynamicModuleList> VesselRestrictionList::Clone() const {
    auto list = std::make_unique<VesselRestrictionList>(node());
    list->activeModules.clear();
    for (size_t i = 0; i < activeModules.size(); ++i) list->activeModules.push_back(activeModules[i]->Clone());
    list->supportedTypes = supportedTypes;
    return list;
}

// ---- VesselRestriction_Cost.cs ---------------------------------------------------------------------------------------------------

// VesselRestriction_Cost.cs:26
void VesselRestriction_Cost::SuscribeToEvents() {
    GameEvents::onEditorPartPlaced.Add(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorPodPicked.Add(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorStarted.Add(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Add(self(), &VesselRestriction::CbLoad);
}
// :34
void VesselRestriction_Cost::ClearEvents() {
    GameEvents::onEditorPartPlaced.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorPodPicked.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorStarted.Remove(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Remove(self(), &VesselRestriction::CbLoad);
}
// :42 (returns the raw tag)
std::string VesselRestriction_Cost::GetDisplayName() { return "#autoLOC_8100136"; }
// :74
float VesselRestriction_Cost::GetShipCurrentCost() {
    if (R::shipCost) return R::shipCost();
    float dryCost = 0.0f;
    float fuelCost = 0.0f;
    return Ship()->GetShipCosts(dryCost, fuelCost, ShipConstruction::ShipManifest);
}
// :47
std::string VesselRestriction_Cost::GetStateMessage() {
    switch (comparisonOperator) {
        default: return "";
        case TestComparisonLessGreaterEqual::LessOrEqual: return Localizer::Format("#autoLOC_8100138", {FloatText(targetCost), FloatText(GetShipCurrentCost())});
        case TestComparisonLessGreaterEqual::Equal: return Localizer::Format("#autoLOC_8100137", {FloatText(targetCost), FloatText(GetShipCurrentCost())});
        case TestComparisonLessGreaterEqual::GreaterOrEqual: return Localizer::Format("#autoLOC_8100139", {FloatText(targetCost), FloatText(GetShipCurrentCost())});
    }
}
// :64
bool VesselRestriction_Cost::SameComparator(VesselRestriction& other) {
    return comparisonOperator == dynamic_cast<VesselRestriction_Cost&>(other).comparisonOperator;
}
// :69
bool VesselRestriction_Cost::IsComplete() { return VesselRestriction::TestFloat(GetShipCurrentCost(), targetCost, comparisonOperator); }
// :80
void VesselRestriction_Cost::Load(const ConfigNode& node) {
    VesselRestriction::Load(node);
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        if (value.name == "cost") {
            float f = 0.0f;
            if (!dotnet::TryParseSingle(value.value, f)) f = 0.0f;
            targetCost = f;
        } else if (value.name == "comparisonOperator") {
            save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterEqual::GreaterOrEqual);
        }
    }
}
// :101
void VesselRestriction_Cost::Save(ConfigNode& node) const {
    VesselRestriction::Save(node);
    node.AddValue("cost", targetCost);
    node.AddValue("comparisonOperator", save_detail::EnumName(comparisonOperator));
}

// ---- VesselRestriction_Crew.cs ---------------------------------------------------------------------------------------------------

// VesselRestriction_Crew.cs:46
void VesselRestriction_Crew::SuscribeToEvents() {
    R::onCrewDialogChange.Add(self(), &VesselRestriction::CbCrew);
    GameEvents::onEditorStarted.Add(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Add(self(), &VesselRestriction::CbLoad);
}
// :53
void VesselRestriction_Crew::ClearEvents() {
    R::onCrewDialogChange.Remove(self(), &VesselRestriction::CbCrew);
    GameEvents::onEditorStarted.Remove(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Remove(self(), &VesselRestriction::CbLoad);
}
// :60
std::string VesselRestriction_Crew::GetDisplayName() { return Localizer::Format("#autoLOC_8100143"); }
// :65
void VesselRestriction_Crew::Load(const ConfigNode& node) {
    VesselRestriction::Load(node);
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        if (value.name == "crewType") {
            save_detail::TryGetEnum(node, "crewType", crewType, CrewRestrictionType::TotalCrew);
        } else if (value.name == "comparisonOperator") {
            save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterEqual::GreaterOrEqual);
        } else if (value.name == "crewCount") {
            int32_t v = 0;
            if (!dotnet::TryParseInt32(value.value, v)) v = 0;
            targetCrew = v;
        }
    }
}
// :87
void VesselRestriction_Crew::Save(ConfigNode& node) const {
    VesselRestriction::Save(node);
    node.AddValue("crewType", save_detail::EnumName(crewType));
    node.AddValue("comparisonOperator", save_detail::EnumName(comparisonOperator));
    node.AddValue("crewCount", targetCrew);
}
// :95 `((VesselRestriction_Mass)otherRestriction).comparisonOperator`: SameComparatorWrapper only calls it with another Crew -> InvalidCastException
bool VesselRestriction_Crew::SameComparator(VesselRestriction& other) {
    if (dynamic_cast<VesselRestriction_Mass*>(&other) == nullptr) throw InvalidCastException("Unable to cast object to type 'VesselRestriction_Mass'.");
    return comparisonOperator == dynamic_cast<VesselRestriction_Mass&>(other).comparisonOperator;
}
// :100
bool VesselRestriction_Crew::IsComplete() { return VesselRestriction::TestInt(GetCrewCount(), targetCrew, comparisonOperator); }
// :105
int VesselRestriction_Crew::GetCrewCount() {
    int result = 0;
    switch (crewType) {
        case CrewRestrictionType::TotalCrew:
            if (R::crewManifest && R::crewManifest() != nullptr) result = R::crewManifest()->CrewCount();
            break;
        case CrewRestrictionType::Engineers: result = GetCrewTraitCount(KerbalRoster::engineerTrait); break;
        case CrewRestrictionType::Scientists: result = GetCrewTraitCount(KerbalRoster::scientistTrait); break;
        case CrewRestrictionType::Pilots: result = GetCrewTraitCount(KerbalRoster::pilotTrait); break;
    }
    return result;
}
// :123
int VesselRestriction_Crew::GetCrewTraitCount(const std::string& kerbalTrait) {
    int num = 0;
    if (R::crewManifest && R::crewManifest() != nullptr) {
        std::vector<std::shared_ptr<ProtoCrewMember>> allCrew = R::crewManifest()->GetAllCrew(false);
        for (size_t i = 0; i < allCrew.size(); ++i) {
            if (allCrew[i] != nullptr && allCrew[i]->trait == kerbalTrait) ++num;
        }
    }
    return num;
}
// :138
std::string VesselRestriction_Crew::GetStateMessage() {
    switch (comparisonOperator) {
        default: return "";
        case TestComparisonLessGreaterEqual::LessOrEqual:
            return Localizer::Format("#autoLOC_8100145", {I(targetCrew), EnumDescription(crewType), I(GetCrewCount())});
        case TestComparisonLessGreaterEqual::Equal:
            return Localizer::Format("#autoLOC_8100144", {I(targetCrew), EnumDescription(crewType), I(GetCrewCount())});
        case TestComparisonLessGreaterEqual::GreaterOrEqual:
            return Localizer::Format("#autoLOC_8100146", {I(targetCrew), EnumDescription(crewType), I(GetCrewCount())});
    }
}

// ---- VesselRestriction_Mass.cs ---------------------------------------------------------------------------------------------------

// VesselRestriction_Mass.cs:26
void VesselRestriction_Mass::SuscribeToEvents() {
    GameEvents::onEditorPartPlaced.Add(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorShipModified.Add(self(), &VesselRestriction::CbShip);
    GameEvents::onEditorShipCrewModified.Add(self(), &VesselRestriction::CbCrew);
    GameEvents::onEditorStarted.Add(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Add(self(), &VesselRestriction::CbLoad);
}
// :36
void VesselRestriction_Mass::ClearEvents() {
    GameEvents::onEditorPartPlaced.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorShipModified.Remove(self(), &VesselRestriction::CbShip);
    GameEvents::onEditorShipCrewModified.Remove(self(), &VesselRestriction::CbCrew);
    GameEvents::onEditorStarted.Remove(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Remove(self(), &VesselRestriction::CbLoad);
}
// :46
std::string VesselRestriction_Mass::GetDisplayName() { return Localizer::Format("#autoLOC_8100169"); }
// :51
void VesselRestriction_Mass::Load(const ConfigNode& node) {
    VesselRestriction::Load(node);
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        if (value.name == "mass") {
            float f = 0.0f;
            if (!dotnet::TryParseSingle(value.value, f)) f = 0.0f;
            targetMass = f;
        } else if (value.name == "comparisonOperator") {
            save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterEqual::GreaterOrEqual);
        }
    }
}
// :71
void VesselRestriction_Mass::Save(ConfigNode& node) const {
    VesselRestriction::Save(node);
    node.AddValue("mass", targetMass);
    node.AddValue("comparisonOperator", save_detail::EnumName(comparisonOperator));
}
// :78
bool VesselRestriction_Mass::SameComparator(VesselRestriction& other) {
    return comparisonOperator == dynamic_cast<VesselRestriction_Mass&>(other).comparisonOperator;
}
// :83
bool VesselRestriction_Mass::IsComplete() { return VesselRestriction::TestFloat(GetCurrentMass(), targetMass, comparisonOperator); }
// :88
float VesselRestriction_Mass::GetCurrentMass() {
    if (R::shipTotalMass) return R::shipTotalMass();
    return Ship()->GetTotalMass();
}
// :93
std::string VesselRestriction_Mass::GetStateMessage() {
    switch (comparisonOperator) {
        default: return Localizer::Format("#autoLOC_8100173");
        case TestComparisonLessGreaterEqual::LessOrEqual: return Localizer::Format("#autoLOC_8100171", {FloatText(targetMass), FloatText(GetCurrentMass())});
        case TestComparisonLessGreaterEqual::Equal: return Localizer::Format("#autoLOC_8100170", {FloatText(targetMass), FloatText(GetCurrentMass())});
        case TestComparisonLessGreaterEqual::GreaterOrEqual: return Localizer::Format("#autoLOC_8100172", {FloatText(targetMass), FloatText(GetCurrentMass())});
    }
}

// ---- VesselRestriction_PartCount.cs ----------------------------------------------------------------------------------------------

// VesselRestriction_PartCount.cs:24
void VesselRestriction_PartCount::SuscribeToEvents() {
    GameEvents::onEditorPodPicked.Add(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorPartPlaced.Add(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorStarted.Add(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Add(self(), &VesselRestriction::CbLoad);
}
// :32
void VesselRestriction_PartCount::ClearEvents() {
    GameEvents::onEditorPartPlaced.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorPodPicked.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorStarted.Remove(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Remove(self(), &VesselRestriction::CbLoad);
}
// :40
std::string VesselRestriction_PartCount::GetDisplayName() { return Localizer::Format("#autoLOC_8100177"); }
// :45
void VesselRestriction_PartCount::Load(const ConfigNode& node) {
    VesselRestriction::Load(node);
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        if (value.name == "numberOfParts") {
            int32_t v = 0;
            if (!dotnet::TryParseInt32(value.value, v)) v = 0;
            targetParts = v;
        } else if (value.name == "comparisonOperator") {
            save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterEqual::GreaterOrEqual);
        }
    }
}
// :65
void VesselRestriction_PartCount::Save(ConfigNode& node) const {
    VesselRestriction::Save(node);
    node.AddValue("numberOfParts", targetParts);
    node.AddValue("comparisonOperator", save_detail::EnumName(comparisonOperator));
}
// :72
bool VesselRestriction_PartCount::SameComparator(VesselRestriction& other) {
    return comparisonOperator == dynamic_cast<VesselRestriction_PartCount&>(other).comparisonOperator;
}
// :77
bool VesselRestriction_PartCount::IsComplete() { return VesselRestriction::TestInt(GetCurrentParts(), targetParts, comparisonOperator); }
// :82
int VesselRestriction_PartCount::GetCurrentParts() {
    if (R::shipPartCount) return R::shipPartCount();
    return static_cast<int>(Ship()->liveParts.size());
}
// :87
std::string VesselRestriction_PartCount::GetStateMessage() {
    switch (comparisonOperator) {
        default: return Localizer::Format("#autoLOC_8100173");
        case TestComparisonLessGreaterEqual::LessOrEqual: return Localizer::Format("#autoLOC_8100179", {I(targetParts), I(GetCurrentParts())});
        case TestComparisonLessGreaterEqual::Equal: return Localizer::Format("#autoLOC_8100178", {I(targetParts), I(GetCurrentParts())});
        case TestComparisonLessGreaterEqual::GreaterOrEqual: return Localizer::Format("#autoLOC_8100180", {I(targetParts), I(GetCurrentParts())});
    }
}

// ---- VesselRestriction_RequiredParts.cs ------------------------------------------------------------------------------------------

// VesselRestriction_RequiredParts.cs:22
void VesselRestriction_RequiredParts::SuscribeToEvents() {
    GameEvents::onEditorPodPicked.Add(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorPartPlaced.Add(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorStarted.Add(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Add(self(), &VesselRestriction::CbLoad);
}
// :30
void VesselRestriction_RequiredParts::ClearEvents() {
    GameEvents::onEditorPartPlaced.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorPodPicked.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorStarted.Remove(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Remove(self(), &VesselRestriction::CbLoad);
}
// :38 (a literal, not a tag)
std::string VesselRestriction_RequiredParts::GetDisplayName() { return Localizer::Format("Required Parts"); }
// :43 / :47
void VesselRestriction_RequiredParts::Load(const ConfigNode&) {}
void VesselRestriction_RequiredParts::Save(ConfigNode&) const {}
// :51
bool VesselRestriction_RequiredParts::IsComplete() {
    if (!UIEntry.present) throw NullReferenceException();  // UIEntry.Data
    if (UIEntry.dataIsVesselInfo) {
        if (!requiredPartsInUseSet) {
            requiredPartsInUse.clear();
            requiredPartsInUseSet = true;
        } else {
            requiredPartsInUse.clear();
        }
        const std::vector<std::string> names = R::shipPartNames ? R::shipPartNames() : [] {
            std::vector<std::string> n;
            for (Part* p : Ship()->liveParts) n.push_back(p->gameObjectName);
            return n;
        }();
        const std::vector<std::string>& required = UIEntry.vesselSituation->requiredParts;
        for (size_t i = 0; i < names.size(); ++i) {
            if (std::find(required.begin(), required.end(), names[i]) != required.end()) {
                if (std::find(requiredPartsInUse.begin(), requiredPartsInUse.end(), names[i]) == requiredPartsInUse.end())
                    requiredPartsInUse.push_back(names[i]);  // List.AddUnique
            }
        }
        return requiredPartsInUse.size() == required.size();
    }
    return false;
}
// :75
std::string VesselRestriction_RequiredParts::GetStateMessage() {
    VesselSituation* vs = R::appCurrentVesselSituation ? R::appCurrentVesselSituation() : nullptr;
    if (vs != nullptr) {
        std::string text = Localizer::Format("#autoLOC_8005322");
        for (size_t i = 0; i < vs->requiredParts.size(); ++i) {
            const std::string title = R::partTitle ? R::partTitle(vs->requiredParts[i]) : "";
            text = text + "\n- " + (title.empty() ? vs->requiredParts[i] : title);
        }
        return text;
    }
    return "[MissionsApp]: RequiredParts restriction-UIEntry not created yet";
}

// ---- VesselRestriction_Resource.cs -----------------------------------------------------------------------------------------------

// VesselRestriction_Resource.cs:21 / :28: the first resource definition of the library
void VesselRestriction_Resource::Init() {
    if (R::firstResourceName) {
        resourceName = R::firstResourceName();
        return;
    }
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    if (lib == nullptr) throw NullReferenceException();
    auto it = lib->resourceDefinitions.begin();
    if (it == lib->resourceDefinitions.end()) throw NullReferenceException();  // enumerator.Current of an empty list
    resourceName = (*it)->name();
}
// :36
void VesselRestriction_Resource::SuscribeToEvents() {
    GameEvents::onEditorPartPlaced.Add(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorShipModified.Add(self(), &VesselRestriction::CbShip);
    GameEvents::onEditorShipCrewModified.Add(self(), &VesselRestriction::CbCrew);
    GameEvents::onEditorStarted.Add(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Add(self(), &VesselRestriction::CbLoad);
}
// :46
void VesselRestriction_Resource::ClearEvents() {
    GameEvents::onEditorPartPlaced.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorShipModified.Remove(self(), &VesselRestriction::CbShip);
    GameEvents::onEditorShipCrewModified.Remove(self(), &VesselRestriction::CbCrew);
    GameEvents::onEditorStarted.Remove(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Remove(self(), &VesselRestriction::CbLoad);
}
// :56
std::string VesselRestriction_Resource::GetDisplayName() { return Localizer::Format("#autoLOC_8100186"); }
// :61
void VesselRestriction_Resource::Load(const ConfigNode& node) {
    VesselRestriction::Load(node);
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        if (value.name == "comparisonOperator") {
            save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterEqual::GreaterOrEqual);
        } else if (value.name == "resourceName") {
            resourceName = value.value;
        } else if (value.name == "amount") {
            float f = 0.0f;
            if (!dotnet::TryParseSingle(value.value, f)) f = 0.0f;
            targetAmount = f;
        }
    }
}
// :84
void VesselRestriction_Resource::Save(ConfigNode& node) const {
    VesselRestriction::Save(node);
    node.AddValue("amount", targetAmount);
    node.AddValue("resourceName", resourceName);
    node.AddValue("comparisonOperator", save_detail::EnumName(comparisonOperator));
}
// :92
bool VesselRestriction_Resource::SameComparator(VesselRestriction& other) {
    return comparisonOperator == dynamic_cast<VesselRestriction_Resource&>(other).comparisonOperator;
}
// :97
bool VesselRestriction_Resource::IsComplete() {
    return VesselRestriction::TestFloat(static_cast<float>(GetCurrentResourceValue()), targetAmount, comparisonOperator);
}
// :102
double VesselRestriction_Resource::GetCurrentResourceValue() {
    double num = 0.0;
    if (R::shipResources) {
        for (const auto& r : R::shipResources())
            if (r.first == resourceName) num += r.second;
        return num;
    }
    for (Part* p : Ship()->liveParts) {
        PartResourceList& resources = p->Resources;
        for (int j = 0; j < resources.Count(); ++j) {
            if (resources[static_cast<size_t>(j)].resourceName == resourceName) num += resources[static_cast<size_t>(j)].amount;
        }
    }
    return num;
}
// :120
std::string VesselRestriction_Resource::GetStateMessage() {
    switch (comparisonOperator) {
        default: return "";
        case TestComparisonLessGreaterEqual::LessOrEqual:
            return Localizer::Format("#autoLOC_8100188", {N1(targetAmount), resourceName, N1(GetCurrentResourceValue())});
        case TestComparisonLessGreaterEqual::Equal:
            return Localizer::Format("#autoLOC_8100187", {N1(targetAmount), resourceName, N1(GetCurrentResourceValue())});
        case TestComparisonLessGreaterEqual::GreaterOrEqual:
            return Localizer::Format("#autoLOC_8100189", {N1(targetAmount), resourceName, N1(GetCurrentResourceValue())});
    }
}

// ---- VesselRestriction_Size.cs ---------------------------------------------------------------------------------------------------

// VesselRestriction_Size.cs:26 / :31
void VesselRestriction_Size::Init() {
    tempInputX = 0.0f;
    tempInputY = 0.0f;
    tempInputZ = 0.0f;
    targetSize = Vector3f(tempInputX, tempInputY, tempInputZ);
}
// :36
void VesselRestriction_Size::SuscribeToEvents() {
    GameEvents::onEditorPartPlaced.Add(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorPodPicked.Add(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorStarted.Add(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Add(self(), &VesselRestriction::CbLoad);
}
// :44
void VesselRestriction_Size::ClearEvents() {
    GameEvents::onEditorPartPlaced.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorPodPicked.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorStarted.Remove(self(), &VesselRestriction::CbVoid);
    GameEvents::onEditorLoad.Remove(self(), &VesselRestriction::CbLoad);
}
// :52
std::string VesselRestriction_Size::GetDisplayName() { return Localizer::Format("#autoLOC_8100194"); }
// :57
void VesselRestriction_Size::Load(const ConfigNode& node) {
    VesselRestriction::Load(node);
    const auto& values = node.values();
    auto tryFloat = [](const std::string& s, float& out) {
        float f = 0.0f;
        if (!dotnet::TryParseSingle(s, f)) f = 0.0f;
        out = f;
    };
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        if (value.name == "comparisonOperator") {
            save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterEqual::GreaterOrEqual);
        } else if (value.name == "sizeZ") {
            tryFloat(value.value, tempInputZ);
        } else if (value.name == "sizeY") {
            tryFloat(value.value, tempInputY);
        } else if (value.name == "sizeX") {
            tryFloat(value.value, tempInputX);
        }
    }
    targetSize = Vector3f(tempInputX, tempInputY, tempInputZ);
}
// :86
void VesselRestriction_Size::Save(ConfigNode& node) const {
    VesselRestriction::Save(node);
    node.AddValue("sizeX", tempInputX);
    node.AddValue("sizeY", tempInputY);
    node.AddValue("sizeZ", tempInputZ);
    node.AddValue("comparisonOperator", save_detail::EnumName(comparisonOperator));
}
// :95
bool VesselRestriction_Size::SameComparator(VesselRestriction& other) {
    return comparisonOperator == dynamic_cast<VesselRestriction_Size&>(other).comparisonOperator;
}
// :114
Vector3f VesselRestriction_Size::GetShipSize() {
    if (R::shipSize) return R::shipSize();
    return ShipConstruction::CalculateCraftSize(*Ship());
}
// :105
bool VesselRestriction_Size::Check(const Vector3f& currentSize, const Vector3f& target, TestComparisonLessGreaterEqual op) {
    if (VesselRestriction::TestFloat(currentSize.x, target.x, op) && VesselRestriction::TestFloat(currentSize.y, target.y, op))
        return VesselRestriction::TestFloat(currentSize.z, target.z, op);
    return false;
}
// :100
bool VesselRestriction_Size::IsComplete() { return Check(GetShipSize(), targetSize, comparisonOperator); }
// :138 (XKCDColors.HexFormat.KSPBadassGreen = "#b4d455", XKCDColors.cs:34)
std::string VesselRestriction_Size::GetShipDimensionsString(const Vector3f& d) {
    std::string empty;
    empty = empty + "\t<color=#b4d455>" + Localizer::Format("#autoLOC_8100191") + ":</color> " + N2(d.y) + Localizer::Format("#autoLOC_7001411") + "\n";
    empty = empty + "\t<color=#b4d455>" + Localizer::Format("#autoLOC_8100192") + ":</color> " + N2(d.x) + Localizer::Format("#autoLOC_7001411") + "\n";
    return empty + "\t<color=#b4d455>" + Localizer::Format("#autoLOC_8100193") + ":</color> " + N2(d.z) + Localizer::Format("#autoLOC_7001411") + "\n";
}
// :119
std::string VesselRestriction_Size::GetStateMessage() {
    switch (comparisonOperator) {
        default: return Localizer::Format("#autoLOC_8100173");
        case TestComparisonLessGreaterEqual::LessOrEqual:
            return Localizer::Format("#autoLOC_8100196", {GetShipDimensionsString(targetSize), GetShipDimensionsString(GetShipSize())});
        case TestComparisonLessGreaterEqual::Equal:
            return Localizer::Format("#autoLOC_8100195", {GetShipDimensionsString(targetSize), GetShipDimensionsString(GetShipSize())});
        case TestComparisonLessGreaterEqual::GreaterOrEqual:
            return Localizer::Format("#autoLOC_8100197", {GetShipDimensionsString(targetSize), GetShipDimensionsString(GetShipSize())});
    }
}

// ---- VesselRestriction_Stages.cs -------------------------------------------------------------------------------------------------

// VesselRestriction_Stages.cs:24
void VesselRestriction_Stages::SuscribeToEvents() {
    GameEvents::onEditorPartPlaced.Add(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorPartPicked.Add(self(), &VesselRestriction::CbPart);
    GameEvents::StageManager::OnGUIStageSequenceModified.Add(self(), &VesselRestriction::CbVoid);
    GameEvents::StageManager::OnGUIStageAdded.Add(self(), &VesselRestriction::CbInt);
    GameEvents::StageManager::OnGUIStageRemoved.Add(self(), &VesselRestriction::CbInt);
    GameEvents::onEditorStarted.Add(self(), &VesselRestriction::CbVoid);
}
// :35
void VesselRestriction_Stages::ClearEvents() {
    GameEvents::onEditorPartPlaced.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::onEditorPartPicked.Remove(self(), &VesselRestriction::CbPart);
    GameEvents::StageManager::OnGUIStageSequenceModified.Remove(self(), &VesselRestriction::CbVoid);
    GameEvents::StageManager::OnGUIStageAdded.Remove(self(), &VesselRestriction::CbInt);
    GameEvents::StageManager::OnGUIStageRemoved.Remove(self(), &VesselRestriction::CbInt);
    GameEvents::onEditorStarted.Remove(self(), &VesselRestriction::CbVoid);
}
// :46
std::string VesselRestriction_Stages::GetDisplayName() { return Localizer::Format("#autoLOC_8100204"); }
// :51
void VesselRestriction_Stages::Load(const ConfigNode& node) {
    VesselRestriction::Load(node);
    const auto& values = node.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        if (value.name == "stages") {
            int32_t v = 0;
            if (!dotnet::TryParseInt32(value.value, v)) v = 0;
            targetStages = v;
        } else if (value.name == "comparisonOperator") {
            save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterEqual::GreaterOrEqual);
        }
    }
}
// :71
void VesselRestriction_Stages::Save(ConfigNode& node) const {
    VesselRestriction::Save(node);
    node.AddValue("stages", targetStages);
    node.AddValue("comparisonOperator", save_detail::EnumName(comparisonOperator));
}
// :78
bool VesselRestriction_Stages::SameComparator(VesselRestriction& other) {
    return comparisonOperator == dynamic_cast<VesselRestriction_Stages&>(other).comparisonOperator;
}
// :83
bool VesselRestriction_Stages::IsComplete() { return VesselRestriction::TestInt(GetCurrentStageCount(), targetStages, comparisonOperator); }
// :88
int VesselRestriction_Stages::GetCurrentStageCount() { return R::stageCount ? R::stageCount() : StageManager::StageCount(); }
// :93
std::string VesselRestriction_Stages::GetStateMessage() {
    switch (comparisonOperator) {
        default: return "";
        case TestComparisonLessGreaterEqual::LessOrEqual: return Localizer::Format("#autoLOC_8100206", {I(targetStages), I(GetCurrentStageCount())});
        case TestComparisonLessGreaterEqual::Equal: return Localizer::Format("#autoLOC_8100205", {I(targetStages), I(GetCurrentStageCount())});
        case TestComparisonLessGreaterEqual::GreaterOrEqual: return Localizer::Format("#autoLOC_8100207", {I(targetStages), I(GetCurrentStageCount())});
    }
}

// ---- MissionPreflightCheck.cs ----------------------------------------------------------------------------------------------------

// MissionPreflightCheck.cs:15
bool MissionPreflightCheck::Test() { return vesselRestriction->IsComplete(); }
// :20
std::string MissionPreflightCheck::GetWarningTitle() { return Localizer::Format("#autoLOC_8100208"); }
// :25
std::string MissionPreflightCheck::GetWarningDescription() { return Localizer::Format("#autoLOC_8100209"); }
// :30
std::string MissionPreflightCheck::GetProceedOption() { return ""; }
// :35
std::string MissionPreflightCheck::GetAbortOption() { return Localizer::Format("#autoLOC_8100210"); }

// ---- MissionBlackListPreFlightCheck.cs -------------------------------------------------------------------------------------------

// MissionBlackListPreFlightCheck.cs:17
MissionBlackListPreFlightCheck::MissionBlackListPreFlightCheck(VesselSituation* situation, const std::vector<std::string>& shipPartNames,
                                                               MissionSituation* firstMissionSituation)
    : vesselSituation(situation) {
    MissionSituation* missionSituation = vesselSituation == nullptr ? firstMissionSituation : vesselSituation->node->mission->situation.get();
    if (missionSituation != nullptr && missionSituation->partFilter != nullptr) {
        missionExcludedParts = missionSituation->partFilter->GetExcludedParts();
        if (!missionExcludedParts.empty()) missionCheckReqd = true;
    }
    if (vesselSituation != nullptr && vesselSituation->partFilter != nullptr && !vesselSituation->partFilter->GetExcludedParts().empty()) {
        for (const std::string& s : vesselSituation->partFilter->GetExcludedParts())  // AddUniqueRange
            if (std::find(missionExcludedParts.begin(), missionExcludedParts.end(), s) == missionExcludedParts.end()) missionExcludedParts.push_back(s);
        missionCheckReqd = true;
    }
    if (!missionCheckReqd) return;
    int count = static_cast<int>(shipPartNames.size());
    while (count-- > 0) {
        const std::string& name = shipPartNames[static_cast<size_t>(count)];
        if (std::find(missionExcludedParts.begin(), missionExcludedParts.end(), name) != missionExcludedParts.end()) {
            auto it = std::find_if(blackListedPartsOnShip.begin(), blackListedPartsOnShip.end(), [&](const auto& p) { return p.first == name; });
            if (it == blackListedPartsOnShip.end()) {
                blackListedPartsOnShip.emplace_back(name, 1);
            } else {
                it->second += 1;
            }
        }
    }
}
// :70
bool MissionBlackListPreFlightCheck::Test() { return blackListedPartsOnShip.empty(); }
// :79
std::string MissionBlackListPreFlightCheck::GetWarningTitle() { return Localizer::Format("#autoLOC_8100208"); }
// :84
std::string MissionBlackListPreFlightCheck::GetWarningDescription() {
    std::string text = Localizer::Format("#autoLOC_8002063") + "\n\n";
    for (const auto& p : blackListedPartsOnShip) {
        const std::string title = R::partTitle ? R::partTitle(p.first) : "";
        text = text + "<color=orange><b>" + I(p.second) + "x " + (title.empty() ? p.first : title) + "</b></color>\n";
    }
    return text;
}
// :95
std::string MissionBlackListPreFlightCheck::GetProceedOption() { return ""; }
// :100
std::string MissionBlackListPreFlightCheck::GetAbortOption() { return Localizer::Format("#autoLOC_253243"); }

// ---- registration ----------------------------------------------------------------------------------------------------------------

namespace {
template <class T>
void RegisterRestriction() {
    DynamicModuleType t;
    t.name = T::kTypeName;
    t.bases = {"VesselRestriction", "DynamicModule"};
    t.create = [](MENode* node) -> std::unique_ptr<DynamicModule> { return std::make_unique<T>(node); };
    DynamicModuleRegistry::Register(std::move(t));
}
}  // namespace

void RegisterVesselRestrictions() {
    RegisterRestriction<VesselRestriction_Cost>();
    RegisterRestriction<VesselRestriction_Crew>();
    RegisterRestriction<VesselRestriction_Mass>();
    RegisterRestriction<VesselRestriction_PartCount>();
    RegisterRestriction<VesselRestriction_RequiredParts>();
    RegisterRestriction<VesselRestriction_Resource>();
    RegisterRestriction<VesselRestriction_Size>();
    RegisterRestriction<VesselRestriction_Stages>();
}

}  // namespace ksp::Expansions::Missions
