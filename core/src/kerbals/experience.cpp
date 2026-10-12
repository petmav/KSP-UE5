// Experience/ExperienceEffect.cs, ExperienceTrait.cs and Effects/*.cs; see include/ksp/kerbals/experience.hpp.
#include "ksp/kerbals/experience.hpp"

#include <algorithm>
#include <map>
#include <typeinfo>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp::Experience {

using Localizer = KSP::Localization::Localizer;


// ================================================== ExperienceEffect ================================================

// ExperienceEffect.cs:35
ExperienceEffect::ExperienceEffect(ExperienceTrait*, const std::vector<float>&) {
    // modifiers.CopyTo(LevelModifiers, 0) with LevelModifiers == null
    throw ArgumentNullException("array");
}

// ExperienceEffect.cs:35
ExperienceEffect::ExperienceEffect(ExperienceTrait*, int level) : level_(level) {}

// ExperienceEffect.cs:19
const std::string& ExperienceEffect::Name() {
    if (!haveName_) {
        name_ = ClassName();
        haveName_ = true;
    }
    return name_;
}

// ExperienceEffect.cs:31
const std::vector<float>& ExperienceEffect::LevelModifiers() const {
    if (!levelModifiers_) throw NullReferenceException("ExperienceEffect.LevelModifiers is null");
    return *levelModifiers_;
}

float ExperienceEffect::LevelModifier(int index) const {
    const std::vector<float>& m = LevelModifiers();
    if (index < 0 || index >= static_cast<int>(m.size())) throw std::out_of_range("Index was outside the bounds of the array.");
    return m[static_cast<size_t>(index)];
}

// ExperienceEffect.cs:60
void ExperienceEffect::LoadFromConfig(const ConfigNode& node) {
    if (const std::string* value = node.GetValuePtr("modifiers")) {
        const std::vector<std::string> array = dotnet::Split(*value, ", ", true);
        std::vector<float> mods(array.size() + 1);
        mods[0] = GetDefaultValue();
        for (size_t i = 0; i < array.size(); ++i) mods[i + 1] = dotnet::ParseSingle(array[i]);
        levelModifiers_ = std::move(mods);
    }
    if (const std::string* value2 = node.GetValuePtr("level")) level_ = dotnet::ParseInt32(*value2);
    OnLoadFromConfig(node);
}

// ================================================== ExperienceTrait =================================================

namespace {
std::map<std::string, ExperienceTrait::TraitFactory>& TraitTypes() {
    static std::map<std::string, ExperienceTrait::TraitFactory> m;
    return m;
}
}  // namespace

void ExperienceTrait::RegisterTraitType(const std::string& typeName, TraitFactory factory) {
    TraitTypes()[typeName] = std::move(factory);
}
bool ExperienceTrait::HasTraitType(const std::string& typeName) { return TraitTypes().count(typeName) != 0; }

// ExperienceTrait.cs:18 (the plain ExperienceTrait returns Config.Title -- the raw localization tag of the cfg, sic)
std::string ExperienceTrait::TypeName() const {
    if (config_ == nullptr) throw NullReferenceException("ExperienceTrait.Config is null");
    return config_->Title().value_or("");
}

// ExperienceTrait.cs:31
std::string ExperienceTrait::Title() const {
    if (config_ == nullptr) throw NullReferenceException("ExperienceTrait.Config is null");
    return config_->Title().value_or("");
}

// ExperienceTrait.cs:33
std::string ExperienceTrait::Description() const {
    if (config_ == nullptr) throw NullReferenceException("ExperienceTrait.Config is null");
    return config_->Description().value_or("");
}

// ExperienceTrait.cs:35
std::string ExperienceTrait::DescriptionEffects() {
    std::string text;
    for (size_t i = 0; i < effects_.size(); ++i) {
        const std::string description = effects_[i]->Description();
        if (!description.empty()) {
            if (!text.empty()) text += "\n";
            text += description;
        }
    }
    return text;
}

// ExperienceTrait.cs:58
std::shared_ptr<ExperienceTrait> ExperienceTrait::Create(const std::string& typeName, ExperienceTraitConfig* config,
                                                         ProtoCrewMember* pcm) {
    if (config == nullptr) {
        log::Error("ExperienceTrait: Config cannot be null");
        return nullptr;
    }
    std::shared_ptr<ExperienceTrait> obj;
    if (!typeName.empty()) {
        auto it = TraitTypes().find(typeName);
        if (it != TraitTypes().end()) obj = it->second();
    }
    if (!obj) obj = std::make_shared<ExperienceTrait>();
    obj->SetupConfig(config);
    obj->protoCrewMember_ = pcm;
    return obj;
}

// ExperienceTrait.cs:74
void ExperienceTrait::SetupConfig(ExperienceTraitConfig* cfg) {
    config_ = cfg;
    for (size_t i = 0; i < cfg->Effects().size(); ++i) AddEffect(*cfg->Effects()[i]);
}

// ExperienceTrait.cs:83
void ExperienceTrait::AddEffect(ExperienceEffectConfig& cfg) {
    std::unique_ptr<ExperienceEffect> effect = ExperienceEffectFactory::Create(cfg.Name(), this);
    if (!effect) {
        log::Error("ExperienceTrait: Cannot add effect '" + cfg.Name() + "' as it does not exist.");
        return;
    }
    effect->LoadFromConfig(*cfg.Config());
    effects_.push_back(std::move(effect));
}

// ExperienceTrait.cs:101
int ExperienceTrait::CrewMemberExperienceLevel(int levelCap) const {
    if (protoCrewMember_ == nullptr) throw NullReferenceException("ExperienceTrait.CrewMember is null");
    const int level = std::max(protoCrewMember_->experienceLevel, 0);
    if (levelCap == -1) return level;
    return std::min(level, levelCap - 1);
}

// ExperienceTrait.cs:134
void ExperienceTrait::Register(Part& part) {
    OnRegister(part);
    for (size_t i = 0; i < effects_.size(); ++i) effects_[i]->Register(part);
}

// ExperienceTrait.cs:144
void ExperienceTrait::Unregister(Part& part) {
    OnUnregister(part);
    for (size_t i = 0; i < effects_.size(); ++i) effects_[i]->Unregister(part);
}

// ExperienceTrait.cs:146 (MessageSystem is UI)
void ExperienceTrait::SendStateMessage(const std::string& /*title*/, const std::string& message) {
    log::Info("Experience (" + Title() + "): " + message);
}

}  // namespace ksp::Experience

// ===================================================== effects ======================================================

namespace ksp::Experience::Effects {

using Localizer = KSP::Localization::Localizer;
using ksp::Experience::ExperienceTrait;

namespace {
bool SerenityInstalled() { return SaveEnvironment::Current().IsExpansionInstalled("Serenity"); }
std::string Percent2(float v) {
    const float p = f32store(fx(v) * fx(100.0f));
    return dotnet::FormatSingleF(p, 2);
}
GameParameters& CurrentParameters() {
    Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr || !g->Parameters) throw NullReferenceException("HighLogic.CurrentGame.Parameters is null");
    return *g->Parameters;
}
std::string Fmt(const char* tag) { return Localizer::Format(tag); }
std::string Fmt1(const char* tag, const std::string& a) { return Localizer::Format(tag, {a}); }
}  // namespace

// ---- AutopilotSkill.cs ----

// AutopilotSkill.cs:20
// (the static list is an optional so that ReadableSkillsCache::ResetForTests can forget it, like a fresh process)
static std::optional<std::vector<std::string>> g_autopilotReadable;
std::vector<std::string>& AutopilotSkill::SkillsReadable() {
    if (!g_autopilotReadable)
        g_autopilotReadable = std::vector<std::string>{Fmt("#autoLOC_18284"), Fmt("#autoLOC_18285"), Fmt("#autoLOC_18286"), Fmt("#autoLOC_18287")};
    return *g_autopilotReadable;
}
AutopilotSkill::AutopilotSkill(ExperienceTrait* parent) : ExperienceEffect(parent) { SkillsReadable(); }
// AutopilotSkill.cs:44
std::string AutopilotSkill::GetDescription() {
    const int num = Parent()->CrewMemberExperienceLevel();
    std::string text;
    const std::vector<std::string>& readable = SkillsReadable();
    for (int i = 0; i < std::min(num + 1, static_cast<int>(readable.size())); ++i) {
        if (i != 0) text += "\n";
        text += Localizer::Format(readable[static_cast<size_t>(i)]);
    }
    return text;
}
// AutopilotSkill.cs:60
void AutopilotSkill::OnRegister(Part& part) {
    part.partValues.AutopilotSkill.Add(this, &AutopilotSkill::GetValue);
    part.partValues.AutopilotKerbalSkill.Add(this, &AutopilotSkill::GetValue);
}
// AutopilotSkill.cs:66
void AutopilotSkill::OnUnregister(Part& part) {
    part.partValues.AutopilotSkill.Remove(this, &AutopilotSkill::GetValue);
    part.partValues.AutopilotKerbalSkill.Remove(this, &AutopilotSkill::GetValue);
}
// AutopilotSkill.cs:68
int AutopilotSkill::GetValue() { return Parent()->CrewMemberExperienceLevel(); }

// ---- ConverterSkill.cs:17 / DrillSkill.cs:17 / ExternalExperimentSkill.cs:17 / ScienceResetSkill.cs:17 ----
std::string ConverterSkill::GetDescription() { return Fmt("#autoLOC_18401"); }
std::string DrillSkill::GetDescription() { return Fmt("#autoLOC_18426"); }
std::string ExternalExperimentSkill::GetDescription() { return Fmt("#autoLOC_18491"); }
std::string ScienceResetSkill::GetDescription() { return Fmt("#autoLOC_18818"); }

// ---- DeployedScienceExpSkill.cs ----
// DeployedScienceExpSkill.cs:17
float DeployedScienceExpSkill::GetDefaultValue() { return SerenityInstalled() ? 0.25f : 0.0f; }
// DeployedScienceExpSkill.cs:26
std::string DeployedScienceExpSkill::GetDescription() {
    if (SerenityInstalled()) {
        const std::string text = Percent2(GetValue());
        return Fmt1("#autoLOC_8002228", text);
    }
    return "";
}
// DeployedScienceExpSkill.cs:36
float DeployedScienceExpSkill::GetValue() {
    if (SerenityInstalled()) return LevelModifier(Parent()->CrewMemberExperienceLevel(static_cast<int>(LevelModifiers().size())));
    return 0.0f;
}

// ---- DeployedSciencePowerSkill.cs ----
// DeployedSciencePowerSkill.cs:17
float DeployedSciencePowerSkill::GetDefaultValue() { return SerenityInstalled() ? 1.0f : 0.0f; }
// DeployedSciencePowerSkill.cs:26
std::string DeployedSciencePowerSkill::GetDescription() {
    if (SerenityInstalled()) return Fmt1("#autoLOC_8002229", dotnet::FormatSingleG(GetValue()));
    return "";
}
// DeployedSciencePowerSkill.cs:36
float DeployedSciencePowerSkill::GetValue() {
    if (SerenityInstalled()) {
        const int lvl = Parent()->CrewMemberExperienceLevel(static_cast<int>(LevelModifiers().size()));
        if (lvl == 0) return GetDefaultValue();
        return f32store(fx(GetDefaultValue()) + fx(LevelModifier(lvl)));
    }
    return 0.0f;
}
// DeployedSciencePowerSkill.cs:49
float DeployedSciencePowerSkill::GetTraitBonus(int level) {
    if (level < static_cast<int>(LevelModifiers().size())) return f32store(fx(1.0f) + fx(LevelModifier(level)));
    return 1.0f;
}

// ---- EnginePower.cs / FuelUsage.cs / HeatProduction.cs / MaxThrottle.cs / PartScienceReturn.cs / VesselScienceReturn.cs ----
// EnginePower.cs:21
std::string EnginePower::GetDescription() { return Fmt1("#autoLOC_18451", Percent2(GetValue()) + "%"); }
// EnginePower.cs:27
void EnginePower::OnRegister(Part& part) { part.partValues.EnginePower.Add(this, &EnginePower::GetValue); }
// EnginePower.cs:32
void EnginePower::OnUnregister(Part& part) { part.partValues.EnginePower.Remove(this, &EnginePower::GetValue); }
// EnginePower.cs:37
float EnginePower::GetValue() { return LevelModifier(Parent()->CrewMemberExperienceLevel(static_cast<int>(LevelModifiers().size()))); }

// FuelUsage.cs:21
std::string FuelUsage::GetDescription() { return Fmt1("#autoLOC_18516", Percent2(GetValue()) + "%"); }
// FuelUsage.cs:27
void FuelUsage::OnRegister(Part& part) { part.partValues.FuelUsage.Add(this, &FuelUsage::GetValue); }
// FuelUsage.cs:32
void FuelUsage::OnUnregister(Part& part) { part.partValues.FuelUsage.Remove(this, &FuelUsage::GetValue); }
// FuelUsage.cs:37
float FuelUsage::GetValue() { return LevelModifier(Parent()->CrewMemberExperienceLevel(static_cast<int>(LevelModifiers().size()))); }

// HeatProduction.cs:21
std::string HeatProduction::GetDescription() { return Fmt1("#autoLOC_18624", Percent2(GetValue())); }
// HeatProduction.cs:27
void HeatProduction::OnRegister(Part& part) { part.partValues.HeatProduction.Add(this, &HeatProduction::GetValue); }
// HeatProduction.cs:32
void HeatProduction::OnUnregister(Part& part) { part.partValues.HeatProduction.Remove(this, &HeatProduction::GetValue); }
// HeatProduction.cs:37
float HeatProduction::GetValue() { return LevelModifier(Parent()->CrewMemberExperienceLevel(static_cast<int>(LevelModifiers().size()))); }

// MaxThrottle.cs:21
std::string MaxThrottle::GetDescription() { return Fmt1("#autoLOC_7001012", Percent2(GetValue())); }
// MaxThrottle.cs:27
void MaxThrottle::OnRegister(Part& part) { part.partValues.MaxThrottle.Add(this, &MaxThrottle::GetValue); }
// MaxThrottle.cs:32
void MaxThrottle::OnUnregister(Part& part) { part.partValues.MaxThrottle.Remove(this, &MaxThrottle::GetValue); }
// MaxThrottle.cs:37
float MaxThrottle::GetValue() { return LevelModifier(Parent()->CrewMemberExperienceLevel(static_cast<int>(LevelModifiers().size()))); }

// PartScienceReturn.cs:21
std::string PartScienceReturn::GetDescription() { return Fmt1("#autoLOC_18706", Percent2(GetValue())); }
// PartScienceReturn.cs:27
void PartScienceReturn::OnRegister(Part& part) { part.partValues.ScienceReturnSum.Add(this, &PartScienceReturn::GetValue); }
// PartScienceReturn.cs:32
void PartScienceReturn::OnUnregister(Part& part) { part.partValues.ScienceReturnSum.Remove(this, &PartScienceReturn::GetValue); }
// PartScienceReturn.cs:37
float PartScienceReturn::GetValue() { return LevelModifier(Parent()->CrewMemberExperienceLevel(static_cast<int>(LevelModifiers().size()))); }

// VesselScienceReturn.cs:21
std::string VesselScienceReturn::GetDescription() { return Fmt1("#autoLOC_18908", Percent2(GetValue())); }
// VesselScienceReturn.cs:27
void VesselScienceReturn::OnRegister(Part& part) { part.partValues.ScienceReturnMax.Add(this, &VesselScienceReturn::GetValue); }
// VesselScienceReturn.cs:32
void VesselScienceReturn::OnUnregister(Part& part) { part.partValues.ScienceReturnMax.Remove(this, &VesselScienceReturn::GetValue); }
// VesselScienceReturn.cs:37
float VesselScienceReturn::GetValue() { return LevelModifier(Parent()->CrewMemberExperienceLevel(static_cast<int>(LevelModifiers().size()))); }

// ---- EVAChuteSkill.cs ----
// EVAChuteSkill.cs:15
void EVAChuteSkill::OnRegister(Part& part) { part.partValues.EVAChuteSkill.Add(this, &EVAChuteSkill::GetValue); }
// EVAChuteSkill.cs:20
void EVAChuteSkill::OnUnregister(Part& part) { part.partValues.EVAChuteSkill.Remove(this, &EVAChuteSkill::GetValue); }
// EVAChuteSkill.cs:18 (the first effect of the same C# type in the parent's list, which may be another instance)
int EVAChuteSkill::GetValue() {
    for (size_t num = 0; num < Parent()->Effects().size(); ++num)
        if (typeid(*Parent()->Effects()[num]) == typeid(*this)) return Parent()->Effects()[num]->Level();
    return Level();
}

// ---- FailureRepairSkill.cs ----
// FailureRepairSkill.cs:12
std::vector<std::string>& FailureRepairSkill::SkillsReadable() {
    static std::vector<std::string> v = {"Repair Failures"};
    return v;
}
// FailureRepairSkill.cs:30 (Enum.GetNames / GetValues are sorted by value: the single member Failures = 1)
std::string FailureRepairSkill::GetDescription() {
    const int num = Parent()->CrewMemberExperienceLevel();
    if (num == 0) return "";
    std::string text = Fmt("#autoLOC_8003452");
    const int values[1] = {Failures};
    for (int i = 0; i < 1; ++i)
        if (num >= values[i]) text = text + "\n" + Localizer::Format(SkillsReadable()[static_cast<size_t>(i)]);
    return text;
}
// FailureRepairSkill.cs:51
void FailureRepairSkill::OnRegister(Part& part) { part.partValues.FailureRepairSkill.Add(this, &FailureRepairSkill::GetValue); }
// FailureRepairSkill.cs:56
void FailureRepairSkill::OnUnregister(Part& part) { part.partValues.FailureRepairSkill.Remove(this, &FailureRepairSkill::GetValue); }
// FailureRepairSkill.cs:56
int FailureRepairSkill::GetValue() { return Parent()->CrewMemberExperienceLevel(); }

// ---- FullVesselControlSkill.cs ----
// FullVesselControlSkill.cs:22
std::string FullVesselControlSkill::GetDescription() {
    const std::string result = Fmt("#autoLOC_18556");
    if (!CurrentParameters().Difficulty.EnableCommNet) return "";
    return result;
}

// ---- GeeForceTolerance.cs ----
// GeeForceTolerance.cs:22 (C# returns null when GKerbalLimits is off; "" here, ExperienceTrait.DescriptionEffects treats both alike)
std::string GeeForceTolerance::GetDescription() {
    if (CurrentParameters().CustomParams<GameParameters::AdvancedParams>().GKerbalLimits) {
        const std::string text = dotnet::FormatSingleF(GeeTolerance(), 2);
        return Fmt1("#autoLOC_18586", text);
    }
    return "";
}
// GeeForceTolerance.cs:26
float GeeForceTolerance::GeeTolerance() {
    if (Parent()->CrewMember() == nullptr) throw NullReferenceException("ExperienceTrait.CrewMember is null");
    if (Parent()->CrewMember()->experienceLevel == 0)
        return f32store(fx(LevelModifier(1)) - (fx(LevelModifier(2)) - fx(LevelModifier(1))));
    return LevelModifier(Parent()->CrewMemberExperienceLevel(static_cast<int>(LevelModifiers().size())));
}

// ---- RepairSkill.cs ----
// RepairSkill.cs:16
static std::optional<std::vector<std::string>> g_repairReadable;
std::vector<std::string>& RepairSkill::SkillsReadable() {
    if (!g_repairReadable) g_repairReadable = std::vector<std::string>{Fmt("#autoLOC_18738"), Fmt("#autoLOC_18739"), Fmt("#autoLOC_18740")};
    return *g_repairReadable;
}

// C++ only (tests)
void ReadableSkillsCache::ResetForTests() {
    g_autopilotReadable.reset();
    g_repairReadable.reset();
}
RepairSkill::RepairSkill(ExperienceTrait* parent) : ExperienceEffect(parent) { SkillsReadable(); }
// RepairSkill.cs:36 (Enum.GetNames / GetValues sort by value: Parachutes 1, LandingLegs 2, Wheels 3; SkillsReadable is indexed
// by that sorted position)
std::string RepairSkill::GetDescription() {
    const int num = Parent()->CrewMemberExperienceLevel();
    if (num == 0) return "";
    std::string text = Fmt("#autoLOC_18766");
    const int values[3] = {Parachutes, LandingLegs, Wheels};
    for (int i = 0; i < 3; ++i)
        if (num >= values[i]) text = text + "\n" + Localizer::Format(SkillsReadable()[static_cast<size_t>(i)]);
    return text;
}
// RepairSkill.cs:58
void RepairSkill::OnRegister(Part& part) { part.partValues.RepairSkill.Add(this, &RepairSkill::GetValue); }
// RepairSkill.cs:63
void RepairSkill::OnUnregister(Part& part) { part.partValues.RepairSkill.Remove(this, &RepairSkill::GetValue); }
// RepairSkill.cs:63
int RepairSkill::GetValue() { return Parent()->CrewMemberExperienceLevel(); }

// ---- ScienceSkill.cs ----
// ScienceSkill.cs:25
void ScienceSkill::OnRegister(Part& part) { part.partValues.ScienceSkill.Add(this, &ScienceSkill::GetValue); }
// ScienceSkill.cs:30
void ScienceSkill::OnUnregister(Part& part) { part.partValues.ScienceSkill.Remove(this, &ScienceSkill::GetValue); }
// ScienceSkill.cs:27
int ScienceSkill::GetValue() { return Parent()->CrewMemberExperienceLevel(); }

}  // namespace ksp::Experience::Effects

namespace ksp::Experience {

namespace {
using namespace ksp::Experience::Effects;
template <class T>
std::unique_ptr<ExperienceEffect> Make(ExperienceTrait* p) { return std::make_unique<T>(p); }
struct EffectEntry {
    const char* name;
    std::unique_ptr<ExperienceEffect> (*make)(ExperienceTrait*);
};
// KerbalRoster.GenerateExperienceEffectTypes: every non-abstract subclass of ExperienceEffect in the loaded assemblies
const std::vector<EffectEntry>& Entries() {
    static const std::vector<EffectEntry> v = {
        {"AutopilotSkill", Make<AutopilotSkill>},
        {"ConverterSkill", Make<ConverterSkill>},
        {"DeployedScienceExpSkill", Make<DeployedScienceExpSkill>},
        {"DeployedSciencePowerSkill", Make<DeployedSciencePowerSkill>},
        {"DrillSkill", Make<DrillSkill>},
        {"EnginePower", Make<EnginePower>},
        {"EVAChuteSkill", Make<EVAChuteSkill>},
        {"ExternalExperimentSkill", Make<ExternalExperimentSkill>},
        {"FailureRepairSkill", Make<FailureRepairSkill>},
        {"FuelUsage", Make<FuelUsage>},
        {"FullVesselControlSkill", Make<FullVesselControlSkill>},
        {"GeeForceTolerance", Make<GeeForceTolerance>},
        {"HeatProduction", Make<HeatProduction>},
        {"MaxThrottle", Make<MaxThrottle>},
        {"PartScienceReturn", Make<PartScienceReturn>},
        {"RepairSkill", Make<RepairSkill>},
        {"ScienceResetSkill", Make<ScienceResetSkill>},
        {"ScienceSkill", Make<ScienceSkill>},
        {"SpecialExperimentSkill", Make<SpecialExperimentSkill>},
        {"VesselScienceReturn", Make<VesselScienceReturn>},
    };
    return v;
}
}  // namespace

std::unique_ptr<ExperienceEffect> ExperienceEffectFactory::Create(const std::string& typeName, ExperienceTrait* parent) {
    for (const EffectEntry& e : Entries())
        if (typeName == e.name) return e.make(parent);
    return nullptr;
}
bool ExperienceEffectFactory::Exists(const std::string& typeName) {
    for (const EffectEntry& e : Entries())
        if (typeName == e.name) return true;
    return false;
}
const std::vector<std::string>& ExperienceEffectFactory::TypeNames() {
    static const std::vector<std::string> v = [] {
        std::vector<std::string> n;
        for (const EffectEntry& e : Entries()) n.push_back(e.name);
        return n;
    }();
    return v;
}

}  // namespace ksp::Experience
