// WP-55: Assembly-CSharp/Expansions/Missions/Adjusters/*.cs. See mission_adjusters.hpp and node_library.hpp.
// Float rule (README "Mono float32 rule"): managed float expressions are f32store(fx(a) op fx(b)).
// Generated in part from a table (the ten single-multiplier adjusters); the cites are the C# declaration lines.
#include "ksp/missions/mission_adjusters.hpp"

#include <algorithm>
#include <cmath>

#include "../save/save_util.hpp"
#include "ksp/config/kspparseable.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission_awards.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Missions::Adjusters {

using KSP::Localization::Localizer;

namespace {
// Localizer.Format arg: Mathf.Round(multiplier / 100f) boxed as a float
std::string RoundedPercent(float multiplier) { return dotnet::FormatSingleG(Mathf::Round(f32store(fx(multiplier) / 100.0))); }
// AdjusterEnginesThrottle.Activate: Mathf.Round(multiplier / 10f) / 10f
std::string RoundedTenth(float multiplier) {
    return dotnet::FormatSingleG(f32store(fx(Mathf::Round(f32store(fx(multiplier) / 10.0))) / 10.0));
}
std::string Tag(const char* t) { return Localizer::Format(t); }

// `adjustedModule as <cls>` != null
bool ModuleIs(PartModule* module, const char* cls) {
    if (module == nullptr) return false;
    if (NodeLibraryEnvironment::moduleIsA) return NodeLibraryEnvironment::moduleIsA(module, cls);
    return module->ClassName() == cls;
}
void ModuleCall(PartModule* module, const char* cls, const char* call, int arg = 0) {
    if (NodeLibraryEnvironment::moduleCall) {
        NodeLibraryEnvironment::moduleCall(module, cls, call, arg);
    } else {
        NodeLibraryEnvironment::moduleCalls.push_back(std::string(cls) + "." + call + "(" + std::to_string(arg) + ")");
    }
}
bool ModuleQuery(PartModule* module, const char* cls, const char* query) {
    return NodeLibraryEnvironment::moduleQuery ? NodeLibraryEnvironment::moduleQuery(module, cls, query) : false;
}
constexpr int kActivate = static_cast<int>(KSPActionType::Activate);
constexpr int kDeactivate = static_cast<int>(KSPActionType::Deactivate);

template <class T>
void AddUnique(std::vector<T>& list, const T& item) {
    if (std::find(list.begin(), list.end(), item) == list.end()) list.push_back(item);
}
}  // namespace

// ---- AdjusterPartModuleBase --------------------------------------------------------------------------------------------------

// AdjusterPartModuleBase.cs:85
AdjusterPartModuleBase::AdjusterPartModuleBase(std::string typeName) : DynamicModule(std::move(typeName)) { adjusterID = MissionsEnvironment::NewGuid(); }

// AdjusterPartModuleBase.cs:90
AdjusterPartModuleBase::AdjusterPartModuleBase(std::string typeName, MENode* node_) : DynamicModule(std::move(typeName), node_) {
    adjusterID = MissionsEnvironment::NewGuid();
}

// AdjusterPartModuleBase.cs:96
void AdjusterPartModuleBase::ModularSetup() {
    className = TypeName();
    classID = MonoStringHash(className);
    modularSetupDone = true;
}

// AdjusterPartModuleBase.cs:104
void AdjusterPartModuleBase::Setup(PartModule* newPartModule) {
    adjustedModule = newPartModule;
    if (!modularSetupDone) ModularSetup();
    if (NodeLibraryEnvironment::adjusterAttach) NodeLibraryEnvironment::adjusterAttach(this, adjustedModule, true);
}

// AdjusterPartModuleBase.cs:121
void AdjusterPartModuleBase::CleanUp() {
    if (NodeLibraryEnvironment::adjusterAttach) NodeLibraryEnvironment::adjusterAttach(this, adjustedModule, false);
}

// AdjusterPartModuleBase.cs:137
void AdjusterPartModuleBase::RemoveAdjuster() {
    Vessel* active = AwardEnvironment::ActiveVessel();
    if (active == nullptr) throw NullReferenceException();
    if (active->GetVesselValues().FailureRepairSkill.value() < 1) {
        // ScreenMessages.PostScreenMessage("#autoLOC_8100278")
        NodeLibraryEnvironment::ScreenMessage m;
        m.text = "#autoLOC_8100278";
        m.duration = 2.0f;
        if (NodeLibraryEnvironment::postScreenMessage) {
            NodeLibraryEnvironment::postScreenMessage(m);
        } else {
            NodeLibraryEnvironment::screenMessages.push_back(m);
        }
    } else if (NodeLibraryEnvironment::removePartModuleAdjuster) {
        NodeLibraryEnvironment::removePartModuleAdjuster(adjustedModule, this);
    }
}

// AdjusterPartModuleBase.cs:149
void AdjusterPartModuleBase::UpdateStatusMessage(const std::string& newStatusMessage) { AdjusterState = newStatusMessage; }

// AdjusterPartModuleBase.cs:154
std::vector<std::string> AdjusterPartModuleBase::GetPartModulesThatCanBeAffected() {
    std::vector<std::string> list;
    std::vector<std::string> partModuleTargetList = GetPartModuleTargetList();
    std::string targetPartModule = GetTargetPartModule();
    if (!targetPartModule.empty()) AddUnique(partModuleTargetList, targetPartModule);
    for (size_t i = 0; i < partModuleTargetList.size(); ++i) {
        AddUnique(list, partModuleTargetList[i]);
        std::vector<std::string> subclassesOfParentClass;
        if (NodeLibraryEnvironment::partModuleSubclasses) subclassesOfParentClass = NodeLibraryEnvironment::partModuleSubclasses(partModuleTargetList[i]);
        for (size_t j = 0; j < subclassesOfParentClass.size(); ++j) AddUnique(list, subclassesOfParentClass[j]);
    }
    return list;
}

// AdjusterPartModuleBase.cs:197
std::string AdjusterPartModuleBase::GetDisplayName() { return Localizer::Format(guiName); }

// AdjusterPartModuleBase.cs:202
std::string AdjusterPartModuleBase::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "guiName" && field.hasValue) return field.value;
    return DynamicModule::GetNodeBodyParameterString(field);
}

// AdjusterPartModuleBase.cs:211
std::vector<std::unique_ptr<AdjusterPartModuleBase>> AdjusterPartModuleBase::CreateModuleAdjusterList(const ConfigNode& node) {
    std::vector<std::unique_ptr<AdjusterPartModuleBase>> list;
    for (int i = 0; i < node.nodes().Count(); ++i) {
        if (node.nodes()[i]->name == "ADJUSTERMODULE") {
            std::unique_ptr<AdjusterPartModuleBase> adjusterPartModuleBase = CreateInstanceOfModuleAdjuster(*node.nodes()[i]);
            if (adjusterPartModuleBase != nullptr) list.push_back(std::move(adjusterPartModuleBase));
        }
    }
    return list;
}

// AdjusterPartModuleBase.cs:228
std::unique_ptr<AdjusterPartModuleBase> AdjusterPartModuleBase::CreateInstanceOfModuleAdjuster(const ConfigNode& node) {
    std::unique_ptr<AdjusterPartModuleBase> adjusterPartModuleBase;
    std::string value;
    if (node.TryGetValue("name", value)) {
        adjusterPartModuleBase = CreateInstanceOfModuleAdjuster(value);
        if (adjusterPartModuleBase != nullptr) adjusterPartModuleBase->Load(node);
    }
    return adjusterPartModuleBase;
}

// AdjusterPartModuleBase.cs:240 (Activator.CreateInstance(type, new object[1]) runs the (MENode) constructor with null)
std::unique_ptr<AdjusterPartModuleBase> AdjusterPartModuleBase::CreateInstanceOfModuleAdjuster(const std::string& className_) {
    const DynamicModuleType* type = DynamicModuleRegistry::GetClassByName(className_);
    std::unique_ptr<DynamicModule> obj;
    if (type != nullptr) obj = type->create(nullptr);
    auto* asAdjuster = dynamic_cast<AdjusterPartModuleBase*>(obj.get());
    if (asAdjuster == nullptr) return nullptr;
    obj.release();
    return std::unique_ptr<AdjusterPartModuleBase>(asAdjuster);
}

// AdjusterPartModuleBase.cs:251
void AdjusterPartModuleBase::Save(ConfigNode& node) const {
    DynamicModule::Save(node);
    node.AddValue("canBeRepaired", canBeRepaired);
    node.AddValue("adjusterID", adjusterID.ToString());
}

// AdjusterPartModuleBase.cs:258
void AdjusterPartModuleBase::Load(const ConfigNode& node) {
    DynamicModule::Load(node);
    if (node.HasValue("canBeRepaired")) {
        bool b = false;
        if (!dotnet::TryParseBool(*node.GetValue("canBeRepaired"), b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
        canBeRepaired = b;
    } else {
        canBeRepaired = false;
    }
    if (node.HasValue("adjusterID")) {
        adjusterID = Guid::Parse(*node.GetValue("adjusterID"));
    } else {
        adjusterID = MissionsEnvironment::NewGuid();
    }
}

// ---- the single-multiplier adjusters (generated from a table) ------------------------------------------------------------------

// AdjusterActiveRadiatorMaxEnergyTransfer.cs:12
AdjusterActiveRadiatorMaxEnergyTransfer::AdjusterActiveRadiatorMaxEnergyTransfer() : DynamicModuleT(kTypeName) { guiName = Localizer::Format("#autoLOC_8100214"); }
// AdjusterActiveRadiatorMaxEnergyTransfer.cs:17
AdjusterActiveRadiatorMaxEnergyTransfer::AdjusterActiveRadiatorMaxEnergyTransfer(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = Localizer::Format("#autoLOC_8100214"); }
// AdjusterActiveRadiatorMaxEnergyTransfer.cs:23
void AdjusterActiveRadiatorMaxEnergyTransfer::Activate() {
    AdjusterActiveRadiatorBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100215", {RoundedPercent(radiatorMaxEnergyTransferMultiplier)}));
}
// AdjusterActiveRadiatorMaxEnergyTransfer.cs:29
double AdjusterActiveRadiatorMaxEnergyTransfer::ApplyMaxEnergyTransferAdjustment(double maxEnergyTransfer) { return maxEnergyTransfer * fx(radiatorMaxEnergyTransferMultiplier) / 100.0; }
// AdjusterActiveRadiatorMaxEnergyTransfer.cs:34
std::string AdjusterActiveRadiatorMaxEnergyTransfer::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "radiatorMaxEnergyTransferMultiplier") return Localizer::Format("#autoLOC_8100216", {dotnet::FormatSingleG(radiatorMaxEnergyTransferMultiplier)});
    return AdjusterActiveRadiatorBase::GetNodeBodyParameterString(field);
}
// AdjusterActiveRadiatorMaxEnergyTransfer.cs:43
void AdjusterActiveRadiatorMaxEnergyTransfer::Save(ConfigNode& node) const {
    AdjusterActiveRadiatorBase::Save(node);
    node.AddValue("radiatorMaxEnergyTransferMultiplier", radiatorMaxEnergyTransferMultiplier);
    node.AddValue("radMultiplierIsHundredBased", true);
}
// AdjusterActiveRadiatorMaxEnergyTransfer.cs:50
void AdjusterActiveRadiatorMaxEnergyTransfer::Load(const ConfigNode& node) {
    AdjusterActiveRadiatorBase::Load(node);
    node.TryGetValue("radiatorMaxEnergyTransferMultiplier", radiatorMaxEnergyTransferMultiplier);
    if (!node.HasValue("radMultiplierIsHundredBased")) radiatorMaxEnergyTransferMultiplier = f32store(fx(radiatorMaxEnergyTransferMultiplier) * 100.0);
}

// AdjusterAlternatorOutput.cs:12
AdjusterAlternatorOutput::AdjusterAlternatorOutput() : DynamicModuleT(kTypeName) { guiName = "Alternator output"; }
// AdjusterAlternatorOutput.cs:17
AdjusterAlternatorOutput::AdjusterAlternatorOutput(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = Localizer::Format("#autoLOC_8100150"); }
// AdjusterAlternatorOutput.cs:23
void AdjusterAlternatorOutput::Activate() {
    AdjusterAlternatorBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100152", {RoundedPercent(alternatorOutputMultiplier)}));
}
// AdjusterAlternatorOutput.cs:29
float AdjusterAlternatorOutput::ApplyOutputAdjustment(float output) { return f32store(fx(output) * fx(alternatorOutputMultiplier) / 100.0); }
// AdjusterAlternatorOutput.cs:34
std::string AdjusterAlternatorOutput::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "alternatorOutputMultiplier") return Localizer::Format("#autoLOC_8100153", {dotnet::FormatSingleG(alternatorOutputMultiplier)});
    return AdjusterAlternatorBase::GetNodeBodyParameterString(field);
}
// AdjusterAlternatorOutput.cs:43
void AdjusterAlternatorOutput::Save(ConfigNode& node) const {
    AdjusterAlternatorBase::Save(node);
    node.AddValue("alternatorOutputMultiplier", alternatorOutputMultiplier);
    node.AddValue("alternatorMultiplierIsHundredBased", true);
}
// AdjusterAlternatorOutput.cs:50
void AdjusterAlternatorOutput::Load(const ConfigNode& node) {
    AdjusterAlternatorBase::Load(node);
    node.TryGetValue("alternatorOutputMultiplier", alternatorOutputMultiplier);
    if (!node.HasValue("alternatorMultiplierIsHundredBased")) alternatorOutputMultiplier = f32store(fx(alternatorOutputMultiplier) * 100.0);
}

// AdjusterControlSurfaceActuatorSpeed.cs:12
AdjusterControlSurfaceActuatorSpeed::AdjusterControlSurfaceActuatorSpeed() : DynamicModuleT(kTypeName) { guiName = "Actuator speed"; }
// AdjusterControlSurfaceActuatorSpeed.cs:17
AdjusterControlSurfaceActuatorSpeed::AdjusterControlSurfaceActuatorSpeed(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = "#autoLOC_8100217"; }
// AdjusterControlSurfaceActuatorSpeed.cs:23
void AdjusterControlSurfaceActuatorSpeed::Activate() {
    AdjusterControlSurfaceBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100218", {RoundedPercent(actuatorSpeedMultiplier)}));
}
// AdjusterControlSurfaceActuatorSpeed.cs:29
float AdjusterControlSurfaceActuatorSpeed::ApplyActuatorSpeedAdjustment(float actuatorSpeed) { return f32store(fx(actuatorSpeed) * fx(actuatorSpeedMultiplier) / 100.0); }
// AdjusterControlSurfaceActuatorSpeed.cs:34
std::string AdjusterControlSurfaceActuatorSpeed::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "actuatorSpeedMultiplier") return Localizer::Format("#autoLOC_8100218", {dotnet::FormatSingleG(actuatorSpeedMultiplier)});
    return AdjusterControlSurfaceBase::GetNodeBodyParameterString(field);
}
// AdjusterControlSurfaceActuatorSpeed.cs:43
void AdjusterControlSurfaceActuatorSpeed::Save(ConfigNode& node) const {
    AdjusterControlSurfaceBase::Save(node);
    node.AddValue("actuatorSpeedMultiplier", actuatorSpeedMultiplier);
    node.AddValue("actuatorSpeedMultiplier", true);  // C# bug kept: the flag is written under the value's own key (the load test below never finds its key)
}
// AdjusterControlSurfaceActuatorSpeed.cs:50
void AdjusterControlSurfaceActuatorSpeed::Load(const ConfigNode& node) {
    AdjusterControlSurfaceBase::Load(node);
    node.TryGetValue("actuatorSpeedMultiplier", actuatorSpeedMultiplier);
    if (!node.HasValue("actuatorMultiplierIsHundredBased")) actuatorSpeedMultiplier = f32store(fx(actuatorSpeedMultiplier) * 100.0);
}

// AdjusterDeployableSolarPanelEfficiency.cs:12
AdjusterDeployableSolarPanelEfficiency::AdjusterDeployableSolarPanelEfficiency() : DynamicModuleT(kTypeName) { guiName = "Solar Panel Efficiency"; }
// AdjusterDeployableSolarPanelEfficiency.cs:17
AdjusterDeployableSolarPanelEfficiency::AdjusterDeployableSolarPanelEfficiency(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = "#autoLOC_8100224"; }
// AdjusterDeployableSolarPanelEfficiency.cs:23
void AdjusterDeployableSolarPanelEfficiency::Activate() {
    AdjusterDeployableSolarPanelBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100225", {RoundedPercent(solarPanelEfficiencyMultiplier)}));
}
// AdjusterDeployableSolarPanelEfficiency.cs:29
float AdjusterDeployableSolarPanelEfficiency::ApplyEfficiencyAdjustment(float efficiency) { return f32store(fx(efficiency) * fx(solarPanelEfficiencyMultiplier) / 100.0); }
// AdjusterDeployableSolarPanelEfficiency.cs:34
std::string AdjusterDeployableSolarPanelEfficiency::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "solarPanelEfficiencyMultiplier") return Localizer::Format("#autoLOC_8100226", {dotnet::FormatSingleG(solarPanelEfficiencyMultiplier)});
    return AdjusterDeployableSolarPanelBase::GetNodeBodyParameterString(field);
}
// AdjusterDeployableSolarPanelEfficiency.cs:43
void AdjusterDeployableSolarPanelEfficiency::Save(ConfigNode& node) const {
    AdjusterDeployableSolarPanelBase::Save(node);
    node.AddValue("solarPanelEfficiencyMultiplier", solarPanelEfficiencyMultiplier);
    node.AddValue("solarMultiplierIsHundredBased", true);
}
// AdjusterDeployableSolarPanelEfficiency.cs:50
void AdjusterDeployableSolarPanelEfficiency::Load(const ConfigNode& node) {
    AdjusterDeployableSolarPanelBase::Load(node);
    node.TryGetValue("solarPanelEfficiencyMultiplier", solarPanelEfficiencyMultiplier);
    if (!node.HasValue("solarMultiplierIsHundredBased")) solarPanelEfficiencyMultiplier = f32store(fx(solarPanelEfficiencyMultiplier) * 100.0);
}

// AdjusterEnginesThrottle.cs:12
AdjusterEnginesThrottle::AdjusterEnginesThrottle() : DynamicModuleT(kTypeName) { guiName = "Engine throttle"; }
// AdjusterEnginesThrottle.cs:17
AdjusterEnginesThrottle::AdjusterEnginesThrottle(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = "#autoLOC_8100228"; }
// AdjusterEnginesThrottle.cs:23
void AdjusterEnginesThrottle::Activate() {
    AdjusterEnginesBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100229", {RoundedTenth(engineThrottleMultiplier)}));
}
// AdjusterEnginesThrottle.cs:29
float AdjusterEnginesThrottle::ApplyThrottleAdjustment(float throttle) { return f32store(fx(throttle) * fx(engineThrottleMultiplier) / 100.0); }
// AdjusterEnginesThrottle.cs:34
std::string AdjusterEnginesThrottle::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "engineThrottleMultiplier") return Localizer::Format("#autoLOC_8100230", {dotnet::FormatSingleG(engineThrottleMultiplier)});
    return AdjusterEnginesBase::GetNodeBodyParameterString(field);
}
// AdjusterEnginesThrottle.cs:43
void AdjusterEnginesThrottle::Save(ConfigNode& node) const {
    AdjusterEnginesBase::Save(node);
    node.AddValue("engineThrottleMultiplier", engineThrottleMultiplier);
    node.AddValue("engineMultiplierIsHundredBased", true);
}
// AdjusterEnginesThrottle.cs:50
void AdjusterEnginesThrottle::Load(const ConfigNode& node) {
    AdjusterEnginesBase::Load(node);
    node.TryGetValue("engineThrottleMultiplier", engineThrottleMultiplier);
    if (!node.HasValue("engineMultiplierIsHundredBased")) engineThrottleMultiplier = f32store(fx(engineThrottleMultiplier) * 100.0);
}

// AdjusterGeneratorEfficiency.cs:12
AdjusterGeneratorEfficiency::AdjusterGeneratorEfficiency() : DynamicModuleT(kTypeName) { guiName = "Generator output"; }
// AdjusterGeneratorEfficiency.cs:17
AdjusterGeneratorEfficiency::AdjusterGeneratorEfficiency(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = "#autoLOC_8100232"; }
// AdjusterGeneratorEfficiency.cs:23
void AdjusterGeneratorEfficiency::Activate() {
    AdjusterGeneratorBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100233", {RoundedPercent(generatorOutputMultiplier)}));
}
// AdjusterGeneratorEfficiency.cs:29
float AdjusterGeneratorEfficiency::ApplyEfficiencyAdjustment(float efficiency) { return f32store(fx(efficiency) * fx(generatorOutputMultiplier) / 100.0); }
// AdjusterGeneratorEfficiency.cs:34
std::string AdjusterGeneratorEfficiency::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "generatorOutputMultiplier") return Localizer::Format("#autoLOC_8100234", {dotnet::FormatSingleG(generatorOutputMultiplier)});
    return AdjusterGeneratorBase::GetNodeBodyParameterString(field);
}
// AdjusterGeneratorEfficiency.cs:43
void AdjusterGeneratorEfficiency::Save(ConfigNode& node) const {
    AdjusterGeneratorBase::Save(node);
    node.AddValue("generatorOutputMultiplier", generatorOutputMultiplier);
    node.AddValue("generatorMultiplierIsHundredBased", true);
}
// AdjusterGeneratorEfficiency.cs:50
void AdjusterGeneratorEfficiency::Load(const ConfigNode& node) {
    AdjusterGeneratorBase::Load(node);
    node.TryGetValue("generatorOutputMultiplier", generatorOutputMultiplier);
    if (!node.HasValue("generatorMultiplierIsHundredBased")) generatorOutputMultiplier = f32store(fx(generatorOutputMultiplier) * 100.0);
}

// AdjusterLiftingSurfaceLiftForce.cs:12
AdjusterLiftingSurfaceLiftForce::AdjusterLiftingSurfaceLiftForce() : DynamicModuleT(kTypeName) { guiName = "Lifting surface lift force"; }
// AdjusterLiftingSurfaceLiftForce.cs:17
AdjusterLiftingSurfaceLiftForce::AdjusterLiftingSurfaceLiftForce(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = "#autoLOC_8100244"; }
// AdjusterLiftingSurfaceLiftForce.cs:23
void AdjusterLiftingSurfaceLiftForce::Activate() {
    AdjusterLiftingSurfaceBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100245", {RoundedPercent(liftForceMultiplier)}));
}
// AdjusterLiftingSurfaceLiftForce.cs:29
Vector3f AdjusterLiftingSurfaceLiftForce::ApplyLiftForceAdjustment(Vector3f liftForce) {
    // Vector3 * float then Vector3 / float: each operator stores its components as float
    Vector3f scaled(f32store(fx(liftForce.x) * fx(liftForceMultiplier)), f32store(fx(liftForce.y) * fx(liftForceMultiplier)), f32store(fx(liftForce.z) * fx(liftForceMultiplier)));
    return Vector3f(f32store(fx(scaled.x) / 100.0), f32store(fx(scaled.y) / 100.0), f32store(fx(scaled.z) / 100.0));
}
// AdjusterLiftingSurfaceLiftForce.cs:34
std::string AdjusterLiftingSurfaceLiftForce::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "liftForceMultiplier") return Localizer::Format("#autoLOC_8100246", {dotnet::FormatSingleG(liftForceMultiplier)});
    return AdjusterLiftingSurfaceBase::GetNodeBodyParameterString(field);
}
// AdjusterLiftingSurfaceLiftForce.cs:43
void AdjusterLiftingSurfaceLiftForce::Save(ConfigNode& node) const {
    AdjusterLiftingSurfaceBase::Save(node);
    node.AddValue("liftForceMultiplier", liftForceMultiplier);
    node.AddValue("liftMultiplierIsHundredBased", true);
}
// AdjusterLiftingSurfaceLiftForce.cs:50
void AdjusterLiftingSurfaceLiftForce::Load(const ConfigNode& node) {
    AdjusterLiftingSurfaceBase::Load(node);
    node.TryGetValue("liftForceMultiplier", liftForceMultiplier);
    if (!node.HasValue("liftMultiplierIsHundredBased")) liftForceMultiplier = f32store(fx(liftForceMultiplier) * 100.0);
}

// AdjusterLightIntensity.cs:12
AdjusterLightIntensity::AdjusterLightIntensity() : DynamicModuleT(kTypeName) { guiName = "Light intensity"; }
// AdjusterLightIntensity.cs:17
AdjusterLightIntensity::AdjusterLightIntensity(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = "#autoLOC_8100248"; }
// AdjusterLightIntensity.cs:23
void AdjusterLightIntensity::Activate() {
    AdjusterLightBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100249", {RoundedPercent(lightIntensityMultiplier)}));
}
// AdjusterLightIntensity.cs:29
float AdjusterLightIntensity::ApplyIntensityAdjustment(float intensity) { return f32store(fx(intensity) * fx(lightIntensityMultiplier) / 100.0); }
// AdjusterLightIntensity.cs:34
std::string AdjusterLightIntensity::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "lightIntensityMultiplier") return Localizer::Format("#autoLOC_8100250", {dotnet::FormatSingleG(lightIntensityMultiplier)});
    return AdjusterLightBase::GetNodeBodyParameterString(field);
}
// AdjusterLightIntensity.cs:43
void AdjusterLightIntensity::Save(ConfigNode& node) const {
    AdjusterLightBase::Save(node);
    node.AddValue("lightIntensityMultiplier", lightIntensityMultiplier);
    node.AddValue("lightMultiplierIsHundredBased", true);
}
// AdjusterLightIntensity.cs:50
void AdjusterLightIntensity::Load(const ConfigNode& node) {
    AdjusterLightBase::Load(node);
    node.TryGetValue("lightIntensityMultiplier", lightIntensityMultiplier);
    if (!node.HasValue("lightMultiplierIsHundredBased")) lightIntensityMultiplier = f32store(fx(lightIntensityMultiplier) * 100.0);
}

// AdjusterWheelBrakesTorque.cs:12
AdjusterWheelBrakesTorque::AdjusterWheelBrakesTorque() : DynamicModuleT(kTypeName) { guiName = "Wheel Brakes torque"; }
// AdjusterWheelBrakesTorque.cs:17
AdjusterWheelBrakesTorque::AdjusterWheelBrakesTorque(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = "#autoLOC_8100264"; }
// AdjusterWheelBrakesTorque.cs:23
void AdjusterWheelBrakesTorque::Activate() {
    AdjusterWheelBrakesBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100265", {RoundedPercent(brakeTorqueMultiplier)}));
}
// AdjusterWheelBrakesTorque.cs:29
float AdjusterWheelBrakesTorque::ApplyTorqueAdjustment(float torque) { return f32store(fx(torque) * fx(brakeTorqueMultiplier) / 100.0); }
// AdjusterWheelBrakesTorque.cs:34
std::string AdjusterWheelBrakesTorque::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "brakeTorqueMultiplier") return Localizer::Format("#autoLOC_8100266", {dotnet::FormatSingleG(brakeTorqueMultiplier)});
    return AdjusterWheelBrakesBase::GetNodeBodyParameterString(field);
}
// AdjusterWheelBrakesTorque.cs:43
void AdjusterWheelBrakesTorque::Save(ConfigNode& node) const {
    AdjusterWheelBrakesBase::Save(node);
    node.AddValue("brakeTorqueMultiplier", brakeTorqueMultiplier);
    node.AddValue("brakeMultiplierIsHundredBased", true);
}
// AdjusterWheelBrakesTorque.cs:50
void AdjusterWheelBrakesTorque::Load(const ConfigNode& node) {
    AdjusterWheelBrakesBase::Load(node);
    node.TryGetValue("brakeTorqueMultiplier", brakeTorqueMultiplier);
    if (!node.HasValue("brakeMultiplierIsHundredBased")) brakeTorqueMultiplier = f32store(fx(brakeTorqueMultiplier) * 100.0);
}

// AdjusterWheelMotorTorque.cs:12
AdjusterWheelMotorTorque::AdjusterWheelMotorTorque() : DynamicModuleT(kTypeName) { guiName = "Wheel Motor torque"; }
// AdjusterWheelMotorTorque.cs:17
AdjusterWheelMotorTorque::AdjusterWheelMotorTorque(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = "#autoLOC_8100267"; }
// AdjusterWheelMotorTorque.cs:23
void AdjusterWheelMotorTorque::Activate() {
    AdjusterWheelMotorBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100268", {RoundedPercent(motorTorqueMultiplier)}));
}
// AdjusterWheelMotorTorque.cs:29
float AdjusterWheelMotorTorque::ApplyTorqueAdjustment(float torque) { return f32store(fx(torque) * fx(motorTorqueMultiplier) / 100.0); }
// AdjusterWheelMotorTorque.cs:34
std::string AdjusterWheelMotorTorque::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "motorTorqueMultiplier") return Localizer::Format("#autoLOC_8100269", {dotnet::FormatSingleG(motorTorqueMultiplier)});
    return AdjusterWheelMotorBase::GetNodeBodyParameterString(field);
}
// AdjusterWheelMotorTorque.cs:43
void AdjusterWheelMotorTorque::Save(ConfigNode& node) const {
    AdjusterWheelMotorBase::Save(node);
    node.AddValue("motorTorqueMultiplier", motorTorqueMultiplier);
    node.AddValue("motorMultiplierIsHundredBased", true);
}
// AdjusterWheelMotorTorque.cs:50
void AdjusterWheelMotorTorque::Load(const ConfigNode& node) {
    AdjusterWheelMotorBase::Load(node);
    node.TryGetValue("motorTorqueMultiplier", motorTorqueMultiplier);
    if (!node.HasValue("motorMultiplierIsHundredBased")) motorTorqueMultiplier = f32store(fx(motorTorqueMultiplier) * 100.0);
}

// ---- AdjusterDataTransmitterPower ----------------------------------------------------------------------------------------------

// AdjusterDataTransmitterPower.cs:12
AdjusterDataTransmitterPower::AdjusterDataTransmitterPower() : DynamicModuleT(kTypeName) { guiName = "Transmitter power"; }
// AdjusterDataTransmitterPower.cs:17
AdjusterDataTransmitterPower::AdjusterDataTransmitterPower(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = Localizer::Format("#autoLOC_8100220"); }
// AdjusterDataTransmitterPower.cs:23
void AdjusterDataTransmitterPower::Activate() {
    AdjusterDataTransmitterBase::Activate();
    UpdatePowerValueDisplay();
}
// AdjusterDataTransmitterPower.cs:29
void AdjusterDataTransmitterPower::PowerValueChanged() { UpdatePowerValueDisplay(); }
// AdjusterDataTransmitterPower.cs:34
void AdjusterDataTransmitterPower::UpdatePowerValueDisplay() {
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100221", {RoundedPercent(transmitterPowerMultiplier)}));
    if (ModuleIs(adjustedModule, "ModuleDataTransmitter")) ModuleCall(adjustedModule, "ModuleDataTransmitter", "UpdatePowerText");
}
// AdjusterDataTransmitterPower.cs:44
double AdjusterDataTransmitterPower::ApplyPowerAdjustment(double power) { return power * (fx(transmitterPowerMultiplier) / 100.0); }
// AdjusterDataTransmitterPower.cs:49
std::string AdjusterDataTransmitterPower::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "transmitterPowerMultiplier") return Localizer::Format("#autoLOC_8100222", {dotnet::FormatSingleG(transmitterPowerMultiplier)});
    return AdjusterDataTransmitterBase::GetNodeBodyParameterString(field);
}
// AdjusterDataTransmitterPower.cs:58
void AdjusterDataTransmitterPower::Save(ConfigNode& node) const {
    AdjusterDataTransmitterBase::Save(node);
    node.AddValue("transmitterPowerMultiplier", transmitterPowerMultiplier);
    node.AddValue("transmitterMultiplierIsHundredBased", true);
}
// AdjusterDataTransmitterPower.cs:65
void AdjusterDataTransmitterPower::Load(const ConfigNode& node) {
    AdjusterDataTransmitterBase::Load(node);
    node.TryGetValue("transmitterPowerMultiplier", transmitterPowerMultiplier);
    if (!node.HasValue("transmitterMultiplierIsHundredBased")) transmitterPowerMultiplier = f32store(fx(transmitterPowerMultiplier) * 100.0);
}

// ---- AdjusterGimbalLimitControl ------------------------------------------------------------------------------------------------

// AdjusterGimbalLimitControl.cs:18
AdjusterGimbalLimitControl::AdjusterGimbalLimitControl() : DynamicModuleT(kTypeName) { guiName = "Gimbal limit control"; }
// AdjusterGimbalLimitControl.cs:23
AdjusterGimbalLimitControl::AdjusterGimbalLimitControl(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = "#autoLOC_8100238"; }
// AdjusterGimbalLimitControl.cs:29
void AdjusterGimbalLimitControl::Activate() {
    AdjusterGimbalBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100239"));
}
// AdjusterGimbalLimitControl.cs:35
Vector3f AdjusterGimbalLimitControl::ApplyControlAdjustment(Vector3f control) {
    control.x = f32store(fx(control.x) * (fx(pitchControlMultiplier) / 100.0));
    control.y = f32store(fx(control.y) * (fx(rollControlMultiplier) / 100.0));
    control.z = f32store(fx(control.z) * (fx(yawControlMultiplier) / 100.0));
    return control;
}
// AdjusterGimbalLimitControl.cs:43
std::string AdjusterGimbalLimitControl::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "pitchControlMultiplier") return Localizer::Format("#autoLOC_8100240", {dotnet::FormatSingleG(pitchControlMultiplier)});
    if (field.name == "rollControlMultiplier") return Localizer::Format("#autoLOC_8100241", {dotnet::FormatSingleG(rollControlMultiplier)});
    if (field.name == "yawControlMultiplier") return Localizer::Format("#autoLOC_8100242", {dotnet::FormatSingleG(yawControlMultiplier)});
    return AdjusterGimbalBase::GetNodeBodyParameterString(field);
}
// AdjusterGimbalLimitControl.cs:60
void AdjusterGimbalLimitControl::Save(ConfigNode& node) const {
    AdjusterGimbalBase::Save(node);
    node.AddValue("pitchControlMultiplier", pitchControlMultiplier);
    node.AddValue("rollControlMultiplier", rollControlMultiplier);
    node.AddValue("yawControlMultiplier", yawControlMultiplier);
    node.AddValue("pitchMultiplierIsHundredBased", true);
    node.AddValue("rollMultiplierIsHundredBased", true);
    node.AddValue("yawMultiplierIsHundredBased", true);
}
// AdjusterGimbalLimitControl.cs:71
void AdjusterGimbalLimitControl::Load(const ConfigNode& node) {
    AdjusterGimbalBase::Load(node);
    node.TryGetValue("pitchControlMultiplier", pitchControlMultiplier);
    node.TryGetValue("rollControlMultiplier", rollControlMultiplier);
    node.TryGetValue("yawControlMultiplier", yawControlMultiplier);
    if (!node.HasValue("pitchMultiplierIsHundredBased")) pitchControlMultiplier = f32store(fx(pitchControlMultiplier) * 100.0);
    if (!node.HasValue("rollMultiplierIsHundredBased")) rollControlMultiplier = f32store(fx(rollControlMultiplier) * 100.0);
    if (!node.HasValue("yawMultiplierIsHundredBased")) yawControlMultiplier = f32store(fx(yawControlMultiplier) * 100.0);
}

// ---- AdjusterKerbNetAccessDisabled ---------------------------------------------------------------------------------------------

// AdjusterKerbNetAccessDisabled.cs:8
AdjusterKerbNetAccessDisabled::AdjusterKerbNetAccessDisabled() : DynamicModuleT(kTypeName) {
    disableKSPFields = true;
    disableKSPActions = true;
    disableKSPEvents = true;
    canBeRepaired = true;
    guiName = "#autoLOC_8100318";
}
// AdjusterKerbNetAccessDisabled.cs:17
AdjusterKerbNetAccessDisabled::AdjusterKerbNetAccessDisabled(MENode* node) : DynamicModuleT(kTypeName, node) {
    disableKSPFields = true;
    disableKSPActions = true;
    disableKSPEvents = true;
    canBeRepaired = true;
    guiName = "#autoLOC_8100318";
}
// AdjusterKerbNetAccessDisabled.cs:27
void AdjusterKerbNetAccessDisabled::Activate() {
    AdjusterKerbNetAccessBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100319"));
    if (ModuleIs(adjustedModule, "ModuleKerbNetAccess") && ModuleQuery(adjustedModule, "ModuleKerbNetAccess", "HasThisDialogOpen")) {
        ModuleCall(adjustedModule, "KerbNetDialog", "Close");
    }
}

// ---- AdjusterReactionWheelTorque -----------------------------------------------------------------------------------------------

// AdjusterReactionWheelTorque.cs:18
AdjusterReactionWheelTorque::AdjusterReactionWheelTorque() : DynamicModuleT(kTypeName) { guiName = "Reaction wheel torque"; }
// AdjusterReactionWheelTorque.cs:23
AdjusterReactionWheelTorque::AdjusterReactionWheelTorque(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = Localizer::Format("#autoLOC_8100254"); }
// AdjusterReactionWheelTorque.cs:29
void AdjusterReactionWheelTorque::Activate() {
    AdjusterReactionWheelBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100255"));
}
// AdjusterReactionWheelTorque.cs:35
Vector3f AdjusterReactionWheelTorque::ApplyTorqueAdjustment(Vector3f torque) {
    torque.x = f32store(fx(torque.x) * (fx(pitchTorqueMultiplier) / 100.0));
    torque.y = f32store(fx(torque.y) * (fx(rollTorqueMultiplier) / 100.0));
    torque.z = f32store(fx(torque.z) * (fx(yawTorqueMultiplier) / 100.0));
    return torque;
}
// AdjusterReactionWheelTorque.cs:43
std::string AdjusterReactionWheelTorque::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "pitchTorqueMultiplier") return Localizer::Format("#autoLOC_8100256", {dotnet::FormatSingleG(pitchTorqueMultiplier)});
    if (field.name == "rollTorqueMultiplier") return Localizer::Format("#autoLOC_8100257", {dotnet::FormatSingleG(rollTorqueMultiplier)});
    if (field.name == "yawTorqueMultiplier") return Localizer::Format("#autoLOC_8100258", {dotnet::FormatSingleG(yawTorqueMultiplier)});
    return AdjusterReactionWheelBase::GetNodeBodyParameterString(field);
}
// AdjusterReactionWheelTorque.cs:60
void AdjusterReactionWheelTorque::Save(ConfigNode& node) const {
    AdjusterReactionWheelBase::Save(node);
    node.AddValue("pitchTorqueMultiplier", pitchTorqueMultiplier);
    node.AddValue("rollTorqueMultiplier", rollTorqueMultiplier);
    node.AddValue("yawTorqueMultiplier", yawTorqueMultiplier);
    node.AddValue("pitchMultiplierIsHundredBased", true);
    node.AddValue("rollMultiplierIsHundredBased", true);
    node.AddValue("yawMultiplierIsHundredBased", true);
}
// AdjusterReactionWheelTorque.cs:71
void AdjusterReactionWheelTorque::Load(const ConfigNode& node) {
    AdjusterReactionWheelBase::Load(node);
    node.TryGetValue("pitchTorqueMultiplier", pitchTorqueMultiplier);
    node.TryGetValue("rollTorqueMultiplier", rollTorqueMultiplier);
    node.TryGetValue("yawTorqueMultiplier", yawTorqueMultiplier);
    if (!node.HasValue("pitchMultiplierIsHundredBased")) pitchTorqueMultiplier = f32store(fx(pitchTorqueMultiplier) * 100.0);
    if (!node.HasValue("rollMultiplierIsHundredBased")) rollTorqueMultiplier = f32store(fx(rollTorqueMultiplier) * 100.0);
    if (!node.HasValue("yawMultiplierIsHundredBased")) yawTorqueMultiplier = f32store(fx(yawTorqueMultiplier) * 100.0);
}

// ---- AdjusterSASServiceLevel ---------------------------------------------------------------------------------------------------

// AdjusterSASServiceLevel.cs:11
AdjusterSASServiceLevel::AdjusterSASServiceLevel() : DynamicModuleT(kTypeName) { guiName = "SAS service level"; }
// AdjusterSASServiceLevel.cs:16
AdjusterSASServiceLevel::AdjusterSASServiceLevel(MENode* node) : DynamicModuleT(kTypeName, node) { guiName = "#autoLOC_8100260"; }
// AdjusterSASServiceLevel.cs:22
void AdjusterSASServiceLevel::Activate() {
    AdjusterSASBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100261", {dotnet::FormatInt64(sasServiceLevel)}));
}
// AdjusterSASServiceLevel.cs:28
int AdjusterSASServiceLevel::ApplySASServiceLevelAdjustment(int) { return sasServiceLevel; }
// AdjusterSASServiceLevel.cs:33
std::string AdjusterSASServiceLevel::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "sasServiceLevel") return Localizer::Format("#autoLOC_8100262", {dotnet::FormatInt64(sasServiceLevel)});
    return AdjusterSASBase::GetNodeBodyParameterString(field);
}
// AdjusterSASServiceLevel.cs:42
void AdjusterSASServiceLevel::Save(ConfigNode& node) const {
    AdjusterSASBase::Save(node);
    node.AddValue("sasServiceLevel", sasServiceLevel);
}
// AdjusterSASServiceLevel.cs:48
void AdjusterSASServiceLevel::Load(const ConfigNode& node) {
    AdjusterSASBase::Load(node);
    node.TryGetValue("sasServiceLevel", sasServiceLevel);
}

// ---- Failure*: the two constructors are identical in each class ---------------------------------------------------------------
// FAILURE_CTORS(Name, flags..., guiName expression): X() and X(MENode*) set the same flags and guiName.
#define FAILURE_FLAGS_ALL \
    disableKSPFields = true; \
    disableKSPActions = true; \
    disableKSPEvents = true; \
    canBeRepaired = true;
#define FAILURE_FLAGS_REPAIR_ONLY canBeRepaired = true;
#define FAILURE_FLAGS_NO_FIELDS \
    disableKSPActions = true; \
    disableKSPEvents = true; \
    canBeRepaired = true;

// FailureActiveRadiatorStuck.cs:12
FailureActiveRadiatorStuck::FailureActiveRadiatorStuck() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_NO_FIELDS
    guiName = "#autoLOC_8100096";
}
// FailureActiveRadiatorStuck.cs:20
FailureActiveRadiatorStuck::FailureActiveRadiatorStuck(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_NO_FIELDS
    guiName = "#autoLOC_8100096";
}
// FailureActiveRadiatorStuck.cs:29
void FailureActiveRadiatorStuck::Activate() {
    AdjusterActiveRadiatorBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100096"));
    if (!ModuleIs(adjustedModule, "ModuleActiveRadiator")) return;
    if (stuckState == FailureActivationState::CurrentState) {
        if (ModuleQuery(adjustedModule, "ModuleActiveRadiator", "IsCooling")) {
            stuckState = FailureActivationState::On;
        } else {
            stuckState = FailureActivationState::Off;
        }
    }
    if (stuckState == FailureActivationState::On) {
        ModuleCall(adjustedModule, "ModuleActiveRadiator", "ApplyRadiatorActivation", kActivate);
    } else if (stuckState == FailureActivationState::Off) {
        ModuleCall(adjustedModule, "ModuleActiveRadiator", "ApplyRadiatorActivation", kDeactivate);
    }
}
// FailureActiveRadiatorStuck.cs:59
bool FailureActiveRadiatorStuck::IsBlockingCooling() { return true; }
// FailureActiveRadiatorStuck.cs:64
void FailureActiveRadiatorStuck::Save(ConfigNode& node) const {
    AdjusterActiveRadiatorBase::Save(node);
    node.AddValue("stuckState", dotnet::EnumToString(stuckState));
}
// FailureActiveRadiatorStuck.cs:70
void FailureActiveRadiatorStuck::Load(const ConfigNode& node) {
    AdjusterActiveRadiatorBase::Load(node);
    if (node.HasValue("stuckState")) stuckState = dotnet::EnumParseFull<FailureActivationState>(*node.GetValue("stuckState"));
}

// FailureAnchoredDecouplerCantDecouple.cs:7
FailureAnchoredDecouplerCantDecouple::FailureAnchoredDecouplerCantDecouple() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = Localizer::Format("#autoLOC_8100322");
}
// FailureAnchoredDecouplerCantDecouple.cs:16
FailureAnchoredDecouplerCantDecouple::FailureAnchoredDecouplerCantDecouple(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = Localizer::Format("#autoLOC_8100322");
}
// FailureAnchoredDecouplerCantDecouple.cs:26
void FailureAnchoredDecouplerCantDecouple::Activate() {
    AdjusterAnchoredDecouplerBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100323"));
}
// FailureAnchoredDecouplerCantDecouple.cs:32
bool FailureAnchoredDecouplerCantDecouple::IsBlockingDecouple() { return true; }

// FailureCargoBayStuck.cs:12
FailureCargoBayStuck::FailureCargoBayStuck() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100098";
}
// FailureCargoBayStuck.cs:21
FailureCargoBayStuck::FailureCargoBayStuck(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100098";
}
// FailureCargoBayStuck.cs:31
void FailureCargoBayStuck::Activate() {
    AdjusterCargoBayBase::Activate();
    if (ModuleIs(adjustedModule, "ModuleCargoBay")) {
        ModuleCall(adjustedModule, "ModuleCargoBay", "ForceCargoBayStuck", static_cast<int>(stuckState));
        std::string partTypeName;
        if (NodeLibraryEnvironment::moduleText) partTypeName = NodeLibraryEnvironment::moduleText(adjustedModule, "ModuleCargoBay", "partTypeName");
        UpdateStatusMessage(Localizer::Format("#autoLOC_8100324", {partTypeName}));
    }
}
// FailureCargoBayStuck.cs:42
void FailureCargoBayStuck::Deactivate() {
    AdjusterCargoBayBase::Deactivate();
    if (ModuleIs(adjustedModule, "ModuleCargoBay")) ModuleCall(adjustedModule, "ModuleCargoBay", "RemoveAdjusterForcingCargoBayStuck");
}
// FailureCargoBayStuck.cs:52
bool FailureCargoBayStuck::IsCargoBayStuck() { return true; }
// FailureCargoBayStuck.cs:57
void FailureCargoBayStuck::Save(ConfigNode& node) const {
    AdjusterCargoBayBase::Save(node);
    node.AddValue("stuckState", dotnet::EnumToString(stuckState));
}
// FailureCargoBayStuck.cs:63
void FailureCargoBayStuck::Load(const ConfigNode& node) {
    AdjusterCargoBayBase::Load(node);
    if (node.HasValue("stuckState")) stuckState = dotnet::EnumParseFull<FailureOpenState>(*node.GetValue("stuckState"));
}

// FailureControlSurfaceDisableControl.cs:7
FailureControlSurfaceDisableControl::FailureControlSurfaceDisableControl() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = Localizer::Format("#autoLOC_8100325");
}
// FailureControlSurfaceDisableControl.cs:16
FailureControlSurfaceDisableControl::FailureControlSurfaceDisableControl(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = Localizer::Format("#autoLOC_8100325");
}
// FailureControlSurfaceDisableControl.cs:26
void FailureControlSurfaceDisableControl::Activate() {
    AdjusterControlSurfaceBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100326"));
}
// FailureControlSurfaceDisableControl.cs:32
float FailureControlSurfaceDisableControl::ApplyActuatorSpeedAdjustment(float) { return 0.0f; }

// FailureCrossfeedStuck.cs:12
FailureCrossfeedStuck::FailureCrossfeedStuck() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100106";
}
// FailureCrossfeedStuck.cs:21
FailureCrossfeedStuck::FailureCrossfeedStuck(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100106";
}
// FailureCrossfeedStuck.cs:31
void FailureCrossfeedStuck::Activate() {
    AdjusterToggleCrossfeedBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100351"));
    if (ModuleIs(adjustedModule, "ModuleToggleCrossfeed")) {
        if (stuckState == FailureActivationState::On) {
            ModuleCall(adjustedModule, "ModuleToggleCrossfeed", "ToggleAction", kActivate);
        } else if (stuckState == FailureActivationState::Off) {
            ModuleCall(adjustedModule, "ModuleToggleCrossfeed", "ToggleAction", kDeactivate);
        }
    }
}
// FailureCrossfeedStuck.cs:49
void FailureCrossfeedStuck::Save(ConfigNode& node) const {
    AdjusterToggleCrossfeedBase::Save(node);
    node.AddValue("stuckState", dotnet::EnumToString(stuckState));
}
// FailureCrossfeedStuck.cs:55
void FailureCrossfeedStuck::Load(const ConfigNode& node) {
    AdjusterToggleCrossfeedBase::Load(node);
    if (node.HasValue("stuckState")) stuckState = dotnet::EnumParseFull<FailureActivationState>(*node.GetValue("stuckState"));
}

// FailureDataTransmitterBroken.cs:7
FailureDataTransmitterBroken::FailureDataTransmitterBroken() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100327";
}
// FailureDataTransmitterBroken.cs:16
FailureDataTransmitterBroken::FailureDataTransmitterBroken(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100327";
}
// FailureDataTransmitterBroken.cs:26
void FailureDataTransmitterBroken::Activate() {
    AdjusterDataTransmitterBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100328"));
    if (ModuleIs(adjustedModule, "ModuleDataTransmitter") && ModuleQuery(adjustedModule, "ModuleDataTransmitter", "IsBusy")) {
        ModuleCall(adjustedModule, "ModuleDataTransmitter", "StopTransmission");
    }
}
// FailureDataTransmitterBroken.cs:37
bool FailureDataTransmitterBroken::IsDataTransmitterBroken() { return true; }

// FailureDecoupleCantDecouple.cs:7
FailureDecoupleCantDecouple::FailureDecoupleCantDecouple() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100329";
}
// FailureDecoupleCantDecouple.cs:16
FailureDecoupleCantDecouple::FailureDecoupleCantDecouple(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100329";
}
// FailureDecoupleCantDecouple.cs:26
void FailureDecoupleCantDecouple::Activate() {
    AdjusterDecoupleBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100341"));
}
// FailureDecoupleCantDecouple.cs:32
bool FailureDecoupleCantDecouple::IsBlockingDecouple() { return true; }

// FailureDeployablePartStuck.cs:12
FailureDeployablePartStuck::FailureDeployablePartStuck() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100100";
}
// FailureDeployablePartStuck.cs:21
FailureDeployablePartStuck::FailureDeployablePartStuck(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100100";
}
// FailureDeployablePartStuck.cs:31
void FailureDeployablePartStuck::Activate() {
    AdjusterDeployablePartBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100283"));
    if (ModuleIs(adjustedModule, "ModuleDeployablePart")) {
        if (stuckState == FailureDeploymentState::Deployed) {
            ModuleCall(adjustedModule, "ModuleDeployablePart", "ControlPanelsWithoutUsingSymmetry", kActivate);
        } else if (stuckState == FailureDeploymentState::Retracted) {
            ModuleCall(adjustedModule, "ModuleDeployablePart", "ControlPanelsWithoutUsingSymmetry", kDeactivate);
        }
    }
}
// FailureDeployablePartStuck.cs:49
bool FailureDeployablePartStuck::IsDeployablePartStuck() { return true; }
// FailureDeployablePartStuck.cs:54
void FailureDeployablePartStuck::Save(ConfigNode& node) const {
    AdjusterDeployablePartBase::Save(node);
    node.AddValue("stuckState", dotnet::EnumToString(stuckState));
}
// FailureDeployablePartStuck.cs:60
void FailureDeployablePartStuck::Load(const ConfigNode& node) {
    AdjusterDeployablePartBase::Load(node);
    if (node.HasValue("stuckState")) stuckState = dotnet::EnumParseFull<FailureDeploymentState>(*node.GetValue("stuckState"));
}

// FailureDockingNodeCantUndock.cs:7
FailureDockingNodeCantUndock::FailureDockingNodeCantUndock() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_REPAIR_ONLY
    guiName = "#autoLOC_8100331";
}
// FailureDockingNodeCantUndock.cs:13
FailureDockingNodeCantUndock::FailureDockingNodeCantUndock(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_REPAIR_ONLY
    guiName = "#autoLOC_8100331";
}
// FailureDockingNodeCantUndock.cs:20
void FailureDockingNodeCantUndock::Activate() {
    AdjusterDockingNodeBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100332"));
}
// FailureDockingNodeCantUndock.cs:26
bool FailureDockingNodeCantUndock::IsBlockingUndock() { return true; }

// FailureEnginesDeadEngine.cs:7
FailureEnginesDeadEngine::FailureEnginesDeadEngine() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100333";
}
// FailureEnginesDeadEngine.cs:16
FailureEnginesDeadEngine::FailureEnginesDeadEngine(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100333";
}
// FailureEnginesDeadEngine.cs:26
void FailureEnginesDeadEngine::Activate() {
    AdjusterEnginesBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100334"));
    if (ModuleIs(adjustedModule, "ModuleEngines")) {
        const bool allowShutdown = ModuleQuery(adjustedModule, "ModuleEngines", "allowShutdown");
        ModuleCall(adjustedModule, "ModuleEngines", "set_allowShutdown", 1);
        ModuleCall(adjustedModule, "ModuleEngines", "Shutdown");
        ModuleCall(adjustedModule, "ModuleEngines", "set_allowShutdown", allowShutdown ? 1 : 0);
    }
}
// FailureEnginesDeadEngine.cs:40
bool FailureEnginesDeadEngine::IsEngineDead() { return true; }

// FailureFuelLineBlocked.cs:8
FailureFuelLineBlocked::FailureFuelLineBlocked() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100335";
}
// FailureFuelLineBlocked.cs:17
FailureFuelLineBlocked::FailureFuelLineBlocked(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100335";
}
// FailureFuelLineBlocked.cs:27
void FailureFuelLineBlocked::Activate() {
    AdjusterFuelLineBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100336"));
    if (ModuleIs(adjustedModule, "CModuleFuelLine")) ModuleCall(adjustedModule, "CModuleFuelLine", "CloseFuelLine");
}
// FailureFuelLineBlocked.cs:38
void FailureFuelLineBlocked::Deactivate() {
    AdjusterFuelLineBase::Deactivate();
    if (ModuleIs(adjustedModule, "CModuleFuelLine")) ModuleCall(adjustedModule, "CModuleFuelLine", "OnTargetSet");
}

// FailureGPSNoDisplay.cs:7
FailureGPSNoDisplay::FailureGPSNoDisplay() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = Localizer::Format("#autoLOC_8100337");
}
// FailureGPSNoDisplay.cs:16
FailureGPSNoDisplay::FailureGPSNoDisplay(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = Localizer::Format("#autoLOC_8100337");
}
// FailureGPSNoDisplay.cs:26
void FailureGPSNoDisplay::Activate() {
    AdjusterGPSBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100338"));
}

// FailureGrappleNodeCantGrapple.cs:7
FailureGrappleNodeCantGrapple::FailureGrappleNodeCantGrapple() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_REPAIR_ONLY
    guiName = "#autoLOC_8100339";
}
// FailureGrappleNodeCantGrapple.cs:13
FailureGrappleNodeCantGrapple::FailureGrappleNodeCantGrapple(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_REPAIR_ONLY
    guiName = "#autoLOC_8100339";
}
// FailureGrappleNodeCantGrapple.cs:20
void FailureGrappleNodeCantGrapple::Activate() {
    AdjusterGrappleNodeBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100340"));
}
// FailureGrappleNodeCantGrapple.cs:26
bool FailureGrappleNodeCantGrapple::IsBlockingGrappleGrab() { return true; }

// FailureGrappleNodeCantRelease.cs:7
FailureGrappleNodeCantRelease::FailureGrappleNodeCantRelease() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_REPAIR_ONLY
    guiName = Localizer::Format("#autoLOC_8100320");
}
// FailureGrappleNodeCantRelease.cs:13
FailureGrappleNodeCantRelease::FailureGrappleNodeCantRelease(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_REPAIR_ONLY
    guiName = Localizer::Format("#autoLOC_8100320");
}
// FailureGrappleNodeCantRelease.cs:20
void FailureGrappleNodeCantRelease::Activate() {
    AdjusterGrappleNodeBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100321"));
    if (ModuleIs(adjustedModule, "ModuleGrappleNode")) {
        ModuleCall(adjustedModule, "ModuleGrappleNode", "SetUpAdjusterBlockingGrappleNodeAbilityToRelease");
    }
}
// FailureGrappleNodeCantRelease.cs:31
void FailureGrappleNodeCantRelease::Deactivate() {
    AdjusterGrappleNodeBase::Deactivate();
    if (ModuleIs(adjustedModule, "ModuleGrappleNode")) {
        ModuleCall(adjustedModule, "ModuleGrappleNode", "RemoveAdjusterBlockingGrappleNodeAbilityToRelease");
    }
}
// FailureGrappleNodeCantRelease.cs:41
bool FailureGrappleNodeCantRelease::IsBlockingGrappleRelease() { return true; }

// FailureLightFlickering.cs:16
FailureLightFlickering::FailureLightFlickering() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = Localizer::Format("#autoLOC_8100271");
}
// FailureLightFlickering.cs:25
FailureLightFlickering::FailureLightFlickering(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = Localizer::Format("#autoLOC_8100271");
}
// FailureLightFlickering.cs:35
void FailureLightFlickering::Activate() {
    AdjusterLightBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100272"));
    if (ModuleIs(adjustedModule, "ModuleLight")) {
        ModuleCall(adjustedModule, "ModuleLight", "ToggleLightAction", kActivate);
        UpdateFlickeringData();
    }
}
// FailureLightFlickering.cs:47
void FailureLightFlickering::UpdateFlickeringData() {
    if (currentIncrement < 0.0f || currentIncrement > 1.0f) {
        currentIncrement = Mathf::Clamp01(currentIncrement);
        const float lo = 0.1f;
        const float hi = f32store(fx(maximumFlickerTime) / 100.0);
        const float r = NodeLibraryEnvironment::randomRangeFloat ? NodeLibraryEnvironment::randomRangeFloat(lo, hi) : lo;
        gradient = f32store(-1.0 * fx(Mathf::Sign(gradient)) * (1.0 / fx(r)));
    }
    const float dt = NodeLibraryEnvironment::deltaTime ? NodeLibraryEnvironment::deltaTime() : 0.0f;
    currentIncrement = f32store(fx(currentIncrement) + fx(gradient) * fx(dt));
}
// FailureLightFlickering.cs:57
float FailureLightFlickering::ApplyIntensityAdjustment(float intensity) {
    UpdateFlickeringData();
    return f32store(fx(intensity) * fx(currentIncrement));
}
// FailureLightFlickering.cs:63
std::string FailureLightFlickering::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "maximumFlickerTime") return Localizer::Format("#autoLOC_8100273", {RoundedPercent(maximumFlickerTime)});
    return AdjusterLightBase::GetNodeBodyParameterString(field);
}
// FailureLightFlickering.cs:72
void FailureLightFlickering::Save(ConfigNode& node) const {
    AdjusterLightBase::Save(node);
    node.AddValue("maximumFlickerTime", maximumFlickerTime);
    node.AddValue("timeMultiplierIsHundredBased", true);
}
// FailureLightFlickering.cs:79
void FailureLightFlickering::Load(const ConfigNode& node) {
    AdjusterLightBase::Load(node);
    node.TryGetValue("maximumFlickerTime", maximumFlickerTime);
    if (!node.HasValue("timeMultiplierIsHundredBased")) maximumFlickerTime = f32store(fx(maximumFlickerTime) * 100.0);
}

// FailureLightStuck.cs:12
FailureLightStuck::FailureLightStuck() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100102";
}
// FailureLightStuck.cs:21
FailureLightStuck::FailureLightStuck(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100102";
}
// FailureLightStuck.cs:31
void FailureLightStuck::Activate() {
    AdjusterLightBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100312"));
    if (ModuleIs(adjustedModule, "ModuleLight")) {
        if (stuckState == FailureActivationState::On) {
            ModuleCall(adjustedModule, "ModuleLight", "ToggleLightAction", kActivate);
        } else if (stuckState == FailureActivationState::Off) {
            ModuleCall(adjustedModule, "ModuleLight", "ToggleLightAction", kDeactivate);
        }
    }
}
// FailureLightStuck.cs:49
void FailureLightStuck::Save(ConfigNode& node) const {
    AdjusterLightBase::Save(node);
    node.AddValue("stuckState", dotnet::EnumToString(stuckState));
}
// FailureLightStuck.cs:55
void FailureLightStuck::Load(const ConfigNode& node) {
    AdjusterLightBase::Load(node);
    if (node.HasValue("stuckState")) stuckState = dotnet::EnumParseFull<FailureActivationState>(*node.GetValue("stuckState"));
}

// FailureProbeControlPointBroken.cs:5
FailureProbeControlPointBroken::FailureProbeControlPointBroken() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100342";
}
// FailureProbeControlPointBroken.cs:14
FailureProbeControlPointBroken::FailureProbeControlPointBroken(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100342";
}
// FailureProbeControlPointBroken.cs:24
bool FailureProbeControlPointBroken::IsProbeControlPointBroken() { return true; }

// FailureRCSFireRandomly.cs:8
FailureRCSFireRandomly::FailureRCSFireRandomly() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100343";
}
// FailureRCSFireRandomly.cs:17
FailureRCSFireRandomly::FailureRCSFireRandomly(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100343";
}
// FailureRCSFireRandomly.cs:27
void FailureRCSFireRandomly::Activate() {
    AdjusterRCSBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100344"));
}
// FailureRCSFireRandomly.cs:33
Vector3f FailureRCSFireRandomly::ApplyInputRotationAdjustment(Vector3f) { return Vector3f::zero(); }
// FailureRCSFireRandomly.cs:38
Vector3f FailureRCSFireRandomly::ApplyInputLinearAdjustment(Vector3f) {
    const float x = NodeLibraryEnvironment::randomValue ? NodeLibraryEnvironment::randomValue() : 0.0f;
    const float y = NodeLibraryEnvironment::randomValue ? NodeLibraryEnvironment::randomValue() : 0.0f;
    const float z = NodeLibraryEnvironment::randomValue ? NodeLibraryEnvironment::randomValue() : 0.0f;
    return Vector3f(x, y, z).normalized();
}

// FailureRCSStopWorking.cs:7
FailureRCSStopWorking::FailureRCSStopWorking() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100345";
}
// FailureRCSStopWorking.cs:16
FailureRCSStopWorking::FailureRCSStopWorking(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100345";
}
// FailureRCSStopWorking.cs:26
void FailureRCSStopWorking::Activate() {
    AdjusterRCSBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100346"));
}
// FailureRCSStopWorking.cs:32
bool FailureRCSStopWorking::IsRCSBroken() { return true; }

// FailureReactionWheelBroken.cs:7
FailureReactionWheelBroken::FailureReactionWheelBroken() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100347";
}
// FailureReactionWheelBroken.cs:16
FailureReactionWheelBroken::FailureReactionWheelBroken(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100347";
}
// FailureReactionWheelBroken.cs:26
void FailureReactionWheelBroken::Activate() {
    AdjusterReactionWheelBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100348"));
}
// FailureReactionWheelBroken.cs:32
bool FailureReactionWheelBroken::IsReactionWheelBroken() { return true; }

// FailureReactionWheelStuck.cs:12
FailureReactionWheelStuck::FailureReactionWheelStuck() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100349";
}
// FailureReactionWheelStuck.cs:21
FailureReactionWheelStuck::FailureReactionWheelStuck(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100349";
}
// FailureReactionWheelStuck.cs:31
void FailureReactionWheelStuck::Activate() {
    AdjusterReactionWheelBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100350"));
    if (!loadedFromConfigNode) {
        if (ModuleIs(adjustedModule, "ModuleReactionWheel")) {
            stuckTorque = NodeLibraryEnvironment::reactionWheelAppliedTorque ? NodeLibraryEnvironment::reactionWheelAppliedTorque(adjustedModule)
                                                                             : Vector3f::zero();
        }
    }
}
// FailureReactionWheelStuck.cs:45
Vector3f FailureReactionWheelStuck::ApplyTorqueAdjustment(Vector3f) { return stuckTorque; }
// FailureReactionWheelStuck.cs:50
void FailureReactionWheelStuck::Save(ConfigNode& node) const {
    AdjusterReactionWheelBase::Save(node);
    node.AddValue("stuckTorque", KSPUtil::WriteVector(stuckTorque));
}
// FailureReactionWheelStuck.cs:56
void FailureReactionWheelStuck::Load(const ConfigNode& node) {
    AdjusterReactionWheelBase::Load(node);
    node.TryGetValue("stuckTorque", stuckTorque);
    loadedFromConfigNode = true;
}

// FailureResourceIntakeStuck.cs:12
FailureResourceIntakeStuck::FailureResourceIntakeStuck() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100104";
}
// FailureResourceIntakeStuck.cs:21
FailureResourceIntakeStuck::FailureResourceIntakeStuck(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100104";
}
// FailureResourceIntakeStuck.cs:31
void FailureResourceIntakeStuck::Activate() {
    AdjusterResourceIntakeBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100354"));
    if (ModuleIs(adjustedModule, "ModuleResourceIntake")) {
        if (stuckState == FailureActivationState::On) {
            ModuleCall(adjustedModule, "ModuleResourceIntake", "ToggleAction", kActivate);
        } else if (stuckState == FailureActivationState::Off) {
            ModuleCall(adjustedModule, "ModuleResourceIntake", "ToggleAction", kDeactivate);
        }
    }
}
// FailureResourceIntakeStuck.cs:49
void FailureResourceIntakeStuck::Save(ConfigNode& node) const {
    AdjusterResourceIntakeBase::Save(node);
    node.AddValue("stuckState", dotnet::EnumToString(stuckState));
}
// FailureResourceIntakeStuck.cs:55
void FailureResourceIntakeStuck::Load(const ConfigNode& node) {
    AdjusterResourceIntakeBase::Load(node);
    if (node.HasValue("stuckState")) stuckState = dotnet::EnumParseFull<FailureActivationState>(*node.GetValue("stuckState"));
}

// FailureScienceExperimentDisable.cs:7
FailureScienceExperimentDisable::FailureScienceExperimentDisable() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100355";
}
// FailureScienceExperimentDisable.cs:16
FailureScienceExperimentDisable::FailureScienceExperimentDisable(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100355";
}
// FailureScienceExperimentDisable.cs:26
void FailureScienceExperimentDisable::Activate() {
    AdjusterScienceExperimentBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100356"));
}

// FailureWheelDeploymentStuck.cs:13
FailureWheelDeploymentStuck::FailureWheelDeploymentStuck() : DynamicModuleT(kTypeName) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100352";
}
// FailureWheelDeploymentStuck.cs:22
FailureWheelDeploymentStuck::FailureWheelDeploymentStuck(MENode* node) : DynamicModuleT(kTypeName, node) {
    FAILURE_FLAGS_ALL
    guiName = "#autoLOC_8100352";
}
// FailureWheelDeploymentStuck.cs:32
void FailureWheelDeploymentStuck::Activate() {
    AdjusterWheelDeploymentBase::Activate();
    UpdateStatusMessage(Localizer::Format("#autoLOC_8100353"));
    if (ModuleIs(adjustedModule, "ModuleWheelDeployment")) {
        if (stuckState == FailureDeploymentState::Deployed) {
            ModuleCall(adjustedModule, "ModuleWheelDeployment", "ActionToggle", kActivate);
        } else if (stuckState == FailureDeploymentState::Retracted) {
            ModuleCall(adjustedModule, "ModuleWheelDeployment", "ActionToggle", kDeactivate);
        }
    }
}
// FailureWheelDeploymentStuck.cs:50
void FailureWheelDeploymentStuck::Save(ConfigNode& node) const {
    AdjusterWheelDeploymentBase::Save(node);
    node.AddValue("stuckState", dotnet::EnumToString(stuckState));
}
// FailureWheelDeploymentStuck.cs:56
void FailureWheelDeploymentStuck::Load(const ConfigNode& node) {
    AdjusterWheelDeploymentBase::Load(node);
    if (node.HasValue("stuckState")) stuckState = dotnet::EnumParseFull<FailureDeploymentState>(*node.GetValue("stuckState"));
}

#undef FAILURE_FLAGS_ALL
#undef FAILURE_FLAGS_REPAIR_ONLY
#undef FAILURE_FLAGS_NO_FIELDS

// ---- registration and MissionsUtils.InitialiseAdjusterTypes --------------------------------------------------------------------

AdjusterTypeTables& GetAdjusterTypeTables() {
    static AdjusterTypeTables tables;
    return tables;
}

namespace {
std::vector<std::string>& RegisteredAdjusterNames() {
    static std::vector<std::string> names;
    return names;
}
template <class T>
void RegisterAdjuster(const char* baseName) {
    DynamicModuleType t;
    t.name = T::kTypeName;
    t.bases = {baseName, "AdjusterPartModuleBase", "DynamicModule"};
    t.create = [](MENode* node) -> std::unique_ptr<DynamicModule> { return std::make_unique<T>(node); };
    DynamicModuleRegistry::Register(std::move(t));
    RegisteredAdjusterNames().push_back(T::kTypeName);
}
}  // namespace

void RegisterAdjusters() {
    RegisteredAdjusterNames().clear();
    RegisterAdjuster<AdjusterActiveRadiatorMaxEnergyTransfer>("AdjusterActiveRadiatorBase");
    RegisterAdjuster<AdjusterAlternatorOutput>("AdjusterAlternatorBase");
    RegisterAdjuster<AdjusterControlSurfaceActuatorSpeed>("AdjusterControlSurfaceBase");
    RegisterAdjuster<AdjusterDataTransmitterPower>("AdjusterDataTransmitterBase");
    RegisterAdjuster<AdjusterDeployableSolarPanelEfficiency>("AdjusterDeployableSolarPanelBase");
    RegisterAdjuster<AdjusterEnginesThrottle>("AdjusterEnginesBase");
    RegisterAdjuster<AdjusterGeneratorEfficiency>("AdjusterGeneratorBase");
    RegisterAdjuster<AdjusterGimbalLimitControl>("AdjusterGimbalBase");
    RegisterAdjuster<AdjusterKerbNetAccessDisabled>("AdjusterKerbNetAccessBase");
    RegisterAdjuster<AdjusterLiftingSurfaceLiftForce>("AdjusterLiftingSurfaceBase");
    RegisterAdjuster<AdjusterLightIntensity>("AdjusterLightBase");
    RegisterAdjuster<AdjusterReactionWheelTorque>("AdjusterReactionWheelBase");
    RegisterAdjuster<AdjusterSASServiceLevel>("AdjusterSASBase");
    RegisterAdjuster<AdjusterWheelBrakesTorque>("AdjusterWheelBrakesBase");
    RegisterAdjuster<AdjusterWheelMotorTorque>("AdjusterWheelMotorBase");
    RegisterAdjuster<FailureActiveRadiatorStuck>("AdjusterActiveRadiatorBase");
    RegisterAdjuster<FailureAnchoredDecouplerCantDecouple>("AdjusterAnchoredDecouplerBase");
    RegisterAdjuster<FailureCargoBayStuck>("AdjusterCargoBayBase");
    RegisterAdjuster<FailureControlSurfaceDisableControl>("AdjusterControlSurfaceBase");
    RegisterAdjuster<FailureCrossfeedStuck>("AdjusterToggleCrossfeedBase");
    RegisterAdjuster<FailureDataTransmitterBroken>("AdjusterDataTransmitterBase");
    RegisterAdjuster<FailureDecoupleCantDecouple>("AdjusterDecoupleBase");
    RegisterAdjuster<FailureDeployablePartStuck>("AdjusterDeployablePartBase");
    RegisterAdjuster<FailureDockingNodeCantUndock>("AdjusterDockingNodeBase");
    RegisterAdjuster<FailureEnginesDeadEngine>("AdjusterEnginesBase");
    RegisterAdjuster<FailureFuelLineBlocked>("AdjusterFuelLineBase");
    RegisterAdjuster<FailureGPSNoDisplay>("AdjusterGPSBase");
    RegisterAdjuster<FailureGrappleNodeCantGrapple>("AdjusterGrappleNodeBase");
    RegisterAdjuster<FailureGrappleNodeCantRelease>("AdjusterGrappleNodeBase");
    RegisterAdjuster<FailureLightFlickering>("AdjusterLightBase");
    RegisterAdjuster<FailureLightStuck>("AdjusterLightBase");
    RegisterAdjuster<FailureProbeControlPointBroken>("AdjusterProbeControlPointBase");
    RegisterAdjuster<FailureRCSFireRandomly>("AdjusterRCSBase");
    RegisterAdjuster<FailureRCSStopWorking>("AdjusterRCSBase");
    RegisterAdjuster<FailureReactionWheelBroken>("AdjusterReactionWheelBase");
    RegisterAdjuster<FailureReactionWheelStuck>("AdjusterReactionWheelBase");
    RegisterAdjuster<FailureResourceIntakeStuck>("AdjusterResourceIntakeBase");
    RegisterAdjuster<FailureScienceExperimentDisable>("AdjusterScienceExperimentBase");
    RegisterAdjuster<FailureWheelDeploymentStuck>("AdjusterWheelDeploymentBase");
}

// MissionsUtils.cs:449 (the order of the types is the registration order above: the assembly metadata order is not observable here, UNVERIFIED)
void InitialiseAdjusterTypes() {
    AdjusterTypeTables& tables = GetAdjusterTypeTables();
    tables.adjusterTypesSupportedByPartModule.clear();
    tables.partModuleTypesSupportedByAdjuster.clear();
    auto find = [](std::vector<std::pair<std::string, std::vector<std::string>>>& v, const std::string& key) -> std::vector<std::string>* {
        for (auto& p : v)
            if (p.first == key) return &p.second;
        return nullptr;
    };
    const std::vector<std::string> names = RegisteredAdjusterNames();
    for (size_t i = 0; i < names.size(); ++i) {
        std::unique_ptr<AdjusterPartModuleBase> adjuster = AdjusterPartModuleBase::CreateInstanceOfModuleAdjuster(names[i]);
        if (adjuster == nullptr) continue;
        std::vector<std::string> partModulesThatCanBeAffected = adjuster->GetPartModulesThatCanBeAffected();
        if (std::vector<std::string>* existing = find(tables.partModuleTypesSupportedByAdjuster, names[i])) {
            *existing = partModulesThatCanBeAffected;
        } else {
            tables.partModuleTypesSupportedByAdjuster.emplace_back(names[i], partModulesThatCanBeAffected);
        }
        for (size_t j = 0; j < partModulesThatCanBeAffected.size(); ++j) {
            std::vector<std::string>* list = find(tables.adjusterTypesSupportedByPartModule, partModulesThatCanBeAffected[j]);
            if (list == nullptr) {
                tables.adjusterTypesSupportedByPartModule.emplace_back(partModulesThatCanBeAffected[j], std::vector<std::string>());
                list = &tables.adjusterTypesSupportedByPartModule.back().second;
            }
            AddUnique(*list, names[i]);
        }
    }
}

}  // namespace ksp::Expansions::Missions::Adjusters
