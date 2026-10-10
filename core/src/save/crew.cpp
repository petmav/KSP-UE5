// FlightLog, experience configs, ProtoCrewMember, KerbalRoster; see include/ksp/save/crew.hpp.
#include "ksp/save/crew.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/misc_util.hpp"
#include "save_util.hpp"

namespace ksp {

using save_detail::EnumName;
using save_detail::ParseBool;
using save_detail::ParseDouble;
using save_detail::ParseEnum;
using save_detail::ParseFloat;
using save_detail::ParseInt;
using Localizer = KSP::Localization::Localizer;
using GE = events::GameEvents;

namespace {
ConfigNode::NodePtr FirstChildShared(const ConfigNode& node, std::string_view name) {
    for (int i = 0; i < node.nodes().Count(); ++i)
        if (node.nodes()[i]->name == name) return node.nodes().Shared(i);
    return nullptr;
}
std::string LocalizedOr(const char* tag, const char* enUs) {
    if (Localizer::Instance() != nullptr) {
        std::string v;
        if (Localizer::TryGetStringByTag(tag, v)) return Localizer::Format(tag);
    }
    return enUs;
}
}  // namespace

// ===================================================== FlightLog ====================================================

// FlightLog.cs:50
FlightLog::Entry::Entry(int flight_, EntryType type_, std::optional<std::string> target_)
    : flight(flight_), type(EnumName(type_)), target(target_ ? std::move(*target_) : std::string()) {}

// FlightLog.cs:82
void FlightLog::Load(ConfigNode* node) {
    if (node == nullptr) return;
    if (node->HasValue("flight")) {
        flight_ = ParseInt(*node->GetValuePtr("flight"));
        node->RemoveValue("flight");
    }
    for (int i = 0; i < node->values().Count(); ++i) {
        const ConfigNode::Value& v = node->values()[i];
        int32_t result = 0;
        if (!dotnet::TryParseInt32(v.name, result)) result = 0;
        const std::vector<std::string> array = dotnet::Split(v.value, ", ", true);
        if (array.size() == 1) entries_.emplace_back(result, array[0]);
        else if (array.size() == 2) entries_.emplace_back(result, array[0], array[1]);
    }
}

// FlightLog.cs:112
void FlightLog::Save(ConfigNode& node) const {
    node.AddValue("flight", static_cast<int32_t>(flight_));
    for (const Entry& entry : entries_)
        node.AddValue(dotnet::FormatInt64(entry.flight),
                      entry.type + (!entry.target.empty() ? ("," + entry.target) : std::string()));
}

// FlightLog.cs:123
FlightLog FlightLog::CreateCopy() const {
    FlightLog log;
    for (const Entry& e : entries_) log.AddEntry(e);
    return log;
}

// FlightLog.cs:134
void FlightLog::MergeWith(const FlightLog& log) {
    for (const Entry& e : log.entries_) entries_.push_back(e);
}

void FlightLog::AddEntry(const Entry& entry) { entries_.emplace_back(entry.flight, entry.type, entry.target); }
void FlightLog::AddEntry(EntryType type, std::optional<std::string> target) {
    entries_.emplace_back(flight_, EnumName(type), std::move(target));
}
void FlightLog::AddEntry(const std::string& type, std::optional<std::string> target) {
    entries_.emplace_back(flight_, type, std::move(target));
}
// FlightLog.cs:162 (entry.target is never null: AddEntryUnique(flight, type, target) with a non-null target)
void FlightLog::AddEntryUnique(const Entry& entry) { AddEntryUnique(entry.flight, entry.type, entry.target); }
void FlightLog::AddEntryUnique(EntryType type, std::optional<std::string> target) {
    AddEntryUnique(flight_, EnumName(type), target);
}
// FlightLog.cs:172
void FlightLog::AddEntryUnique(int flight, const std::string& type, const std::optional<std::string>& target) {
    if (!target.has_value() && !HasEntry(type)) entries_.emplace_back(flight, type, target);
    else if (!HasEntry(type, target)) entries_.emplace_back(flight, type, target);
}

const FlightLog::Entry* FlightLog::Last() const { return entries_.empty() ? nullptr : &entries_.back(); }

// FlightLog.cs:193-231: the loops iterate the freshly created (empty) list, so every overload returns an empty array
std::vector<FlightLog::Entry> FlightLog::GetEntries(EntryType, const std::string&) const { return {}; }
std::vector<FlightLog::Entry> FlightLog::GetEntries(EntryType) const { return {}; }
std::vector<FlightLog::Entry> FlightLog::GetEntries(const std::string&) const { return {}; }
std::vector<FlightLog::Entry> FlightLog::GetEntries(const std::string&, const std::string&) const { return {}; }

bool FlightLog::HasEntry(EntryType type) const { return HasEntry(EnumName(type)); }
bool FlightLog::HasEntry(EntryType type, const std::string& target) const {
    return HasEntry(EnumName(type), std::optional<std::string>(target));
}
// FlightLog.cs:243
bool FlightLog::HasEntry(const std::string& type) const {
    for (const Entry& e : entries_)
        if (e.type == type) return true;
    return false;
}
// FlightLog.cs:263 (a null target never equals a stored target, which is "" at worst)
bool FlightLog::HasEntry(const std::string& type, const std::optional<std::string>& target) const {
    for (const Entry& e : entries_)
        if (e.type == type && target.has_value() && e.target == *target) return true;
    return false;
}

// FlightLog.cs:283
std::vector<std::string> FlightLog::GetDistinctTypes() const {
    std::vector<std::string> list;
    for (const Entry& e : entries_)
        if (std::find(list.begin(), list.end(), e.type) == list.end()) list.push_back(e.type);
    return list;
}
std::vector<std::string> FlightLog::GetDistinctTypes(const std::string& target) const {
    std::vector<std::string> list;
    for (const Entry& e : entries_)
        if (e.target == target && std::find(list.begin(), list.end(), e.type) == list.end()) list.push_back(e.type);
    return list;
}
std::vector<std::string> FlightLog::GetDistinctTargets(EntryType type) const { return GetDistinctTargets(EnumName(type)); }
std::vector<std::string> FlightLog::GetDistinctTargets(const std::string& type) const {
    std::vector<std::string> list;
    for (const Entry& e : entries_)
        if (e.type == type && std::find(list.begin(), list.end(), e.target) == list.end()) list.push_back(e.target);
    return list;
}
std::vector<std::string> FlightLog::GetDistinctTargets() const {
    std::vector<std::string> list;
    for (const Entry& e : entries_)
        if (std::find(list.begin(), list.end(), e.target) == list.end()) list.push_back(e.target);
    return list;
}

// FlightLog.cs:344
std::vector<FlightLog> FlightLog::GetFlights() const {
    std::vector<FlightLog> list;
    if (entries_.empty()) return list;
    list.emplace_back();
    int num = 0;
    for (const Entry& entry : entries_) {
        if (entry.flight != num) {
            num = entry.flight;
            list.emplace_back();
        }
        list.back().AddEntry(entry);
    }
    return list;
}

// ================================================= Experience configs ===============================================

namespace Experience {

// Experience/ExperienceEffectConfig.cs:14
std::shared_ptr<ExperienceEffectConfig> ExperienceEffectConfig::Create(const ConfigNode& node) {
    auto cfg = std::make_shared<ExperienceEffectConfig>();
    const std::string* value = node.GetValuePtr("name");
    if (value == nullptr || value->empty()) return nullptr;
    cfg->name_ = *value;
    cfg->config_ = node.CreateCopy();
    return cfg;
}

// Experience/ExperienceTraitConfig.cs:28
std::shared_ptr<ExperienceTraitConfig> ExperienceTraitConfig::Create(const ConfigNode& node) {
    auto cfg = std::make_shared<ExperienceTraitConfig>();
    const std::string* name = node.GetValuePtr("name");
    if (name == nullptr) {
        log::Error("ExperienceTraitConfig: name cannot be null");
        return nullptr;
    }
    cfg->name_ = *name;
    if (const std::string* v = node.GetValuePtr("title"); v && !v->empty()) cfg->title_ = *v;
    if (const std::string* v = node.GetValuePtr("desc"); v && !v->empty()) cfg->description_ = *v;
    if (const std::string* v = node.GetValuePtr("description"); v && !v->empty()) cfg->description_ = *v;
    if (const std::string* v = node.GetValuePtr("icon"); v && !v->empty()) cfg->iconString_ = *v;
    for (ConfigNode* e : node.GetNodes("EFFECT"))
        if (auto effect = ExperienceEffectConfig::Create(*e)) cfg->effects_.push_back(effect);
    return cfg;
}

// Experience/ExperienceSystemConfig.cs:23
void ExperienceSystemConfig::LoadTraitConfigs(const std::vector<const ConfigNode*>& configNodes) {
    categories_.clear();
    traitNames_.clear();
    traitNamesNoTourist_.clear();
    for (const ConfigNode* node : configNodes) {
        auto cfg = ExperienceTraitConfig::Create(*node);
        if (!cfg) continue;
        ExperienceTraitConfig* existing = GetExperienceTraitConfig(cfg->Name());
        if (existing != nullptr) {
            bool flag = false;
            for (const auto& effect : cfg->Effects()) {
                const std::vector<std::string> traitsWithEffect = GetTraitsWithEffect(effect->Name());
                for (const std::string& t : traitsWithEffect)
                    if (t == cfg->Name()) {
                        log::Error("ExperienceSystemConfig: Experience trait '" + cfg->Name() +
                                   "' already exists, and trying to add a duplicate Effect of '" + effect->Name() + "'");
                        flag = true;
                        break;
                    }
                if (!flag) existing->Effects().push_back(effect);
            }
        } else {
            categories_.push_back(cfg);
            traitNames_.push_back(cfg->Name());
            if (cfg->Name() != KerbalRoster::touristTrait) traitNamesNoTourist_.push_back(cfg->Name());
        }
    }
}

ExperienceTraitConfig* ExperienceSystemConfig::GetExperienceTraitConfig(std::string_view name) const {
    for (const auto& c : categories_)
        if (c->Name() == name) return c.get();
    return nullptr;
}

// ExperienceSystemConfig.cs:90 (returns trait TITLES, which LoadTraitConfigs compares with names -- sic)
std::vector<std::string> ExperienceSystemConfig::GetTraitsWithEffect(std::string_view effectName) const {
    std::vector<std::string> list;
    for (const auto& c : categories_)
        for (const auto& e : c->Effects())
            if (e->Name() == effectName) {
                list.push_back(c->Title().value_or(""));
                break;
            }
    return list;
}

std::vector<std::string> ExperienceSystemConfig::GetTraitsNamesWithEffect(std::string_view effectName) const {
    std::vector<std::string> list;
    for (const auto& c : categories_)
        for (const auto& e : c->Effects())
            if (e->Name() == effectName) {
                list.push_back(c->Name());
                break;
            }
    return list;
}

// GameData/Squad/Experience/Traits.cfg of KSP 1.12.5 (comments stripped as ConfigNode.Load does)
std::shared_ptr<ExperienceSystemConfig> ExperienceSystemConfig::StockBuiltIn() {
    static const char* const kTraits =
        "EXPERIENCE_TRAIT\n{\nname = Pilot\ntitle = #autoLOC_500101\ndesc = #autoLOC_500102\n"
        "EFFECT\n{\nname = AutopilotSkill\n}\nEFFECT\n{\nname = FullVesselControlSkill\n}\n"
        "EFFECT\n{\nname = GeeForceTolerance\nmodifiers = 1.3, 1.45, 1.6, 1.85, 2.0\n}\n"
        "EFFECT\n{\nname = EVAChuteSkill\nlevel = 0\n}\n}\n"
        "EXPERIENCE_TRAIT\n{\nname = Engineer\ntitle = #autoLOC_500103\ndesc = #autoLOC_500104\n"
        "EFFECT\n{\nname = RepairSkill\n}\nEFFECT\n{\nname = ConverterSkill\n}\nEFFECT\n{\nname = DrillSkill\n}\n"
        "EFFECT\n{\nname = FailureRepairSkill\n}\n"
        "EFFECT\n{\nname = GeeForceTolerance\nmodifiers = 1.1, 1.2, 1.3, 1.4, 1.5\n}\n"
        "EFFECT\n{\nname = EVAChuteSkill\nlevel = 1\n}\n}\n"
        "EXPERIENCE_TRAIT\n{\nname = Scientist\ntitle = #autoLOC_500105\ndesc = #autoLOC_500106\n"
        "EFFECT\n{\nname = VesselScienceReturn\nmodifiers = 1.05, 1.1, 1.15, 1.2, 1.25\n}\n"
        "EFFECT\n{\nname = PartScienceReturn\nmodifiers = 1.05, 1.1, 1.15, 1.2, 1.25\n}\n"
        "EFFECT\n{\nname = ScienceSkill\n}\nEFFECT\n{\nname = ScienceResetSkill\n}\n"
        "EFFECT\n{\nname = SpecialExperimentSkill\n}\nEFFECT\n{\nname = ExternalExperimentSkill\n}\n"
        "EFFECT\n{\nname = GeeForceTolerance\nmodifiers = 1.1, 1.2, 1.3, 1.4, 1.5\n}\n"
        "EFFECT\n{\nname = EVAChuteSkill\nlevel = 1\n}\n}\n"
        "EXPERIENCE_TRAIT\n{\nname = Tourist\ntitle = #autoLOC_500107\ndesc = #autoLOC_500108\n"
        "EFFECT\n{\nname = GeeForceTolerance\nmodifiers = 0.75, 0.75, 0.75, 0.75, 0.75\n}\n}\n";
    static const std::shared_ptr<ExperienceSystemConfig> cfg = [] {
        ConfigNode::NodePtr root = ConfigNode::Parse(kTraits);
        std::vector<const ConfigNode*> nodes;
        for (ConfigNode* n : root->GetNodes("EXPERIENCE_TRAIT")) nodes.push_back(n);
        return std::make_shared<ExperienceSystemConfig>(nodes);
    }();
    return cfg;
}

}  // namespace Experience

namespace {
Experience::ExperienceSystemConfig& ExperienceConfigs() {
    SaveEnvironment& env = SaveEnvironment::Current();
    if (!env.experienceConfigs) env.experienceConfigs = Experience::ExperienceSystemConfig::StockBuiltIn();
    return *env.experienceConfigs;
}
}  // namespace

// ================================================== ProtoCrewMember =================================================

// ProtoCrewMember.cs:423
ProtoCrewMember::ProtoCrewMember(KerbalType type) {
    type_ = type;
    completedFirstEVA = false;
    SetDefaultInventory();
}

// ProtoCrewMember.cs:430
ProtoCrewMember::ProtoCrewMember(KerbalType type, std::string newName) {
    type_ = type;
    name_ = std::move(newName);
    completedFirstEVA = false;
    SetDefaultInventory();
}

// ProtoCrewMember.cs:438
ProtoCrewMember::ProtoCrewMember(CopyOf, const ProtoCrewMember& copyOf) {
    name_ = copyOf.name_;
    set_gender(copyOf.gender());
    courage = copyOf.courage;
    stupidity = copyOf.stupidity;
    isBadass = copyOf.isBadass;
    veteran = copyOf.veteran;
    hasToured = copyOf.hasToured;
    rosterStatus_ = copyOf.rosterStatus();
    inactive_ = copyOf.inactive_;
    inactiveTimeEnd = copyOf.inactiveTimeEnd;
    gExperienced = copyOf.gExperienced;
    outDueToG = copyOf.outDueToG;
    type_ = copyOf.type_;
    UTaR = copyOf.UTaR;
    flightLog = copyOf.flightLog.CreateCopy();
    careerLog = copyOf.careerLog.CreateCopy();
    if (SaveEnvironment::Current().IsExpansionAnyKerbalSuitInstalled())
        set_suit(const_cast<ProtoCrewMember&>(copyOf).suit());
    isHero = copyOf.isHero;
    trait = copyOf.trait;
    KerbalRoster::SetExperienceTrait(*this);
    extraExperience_ = copyOf.extraExperience_;
    const Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr) throw NullReferenceException("ProtoCrewMember(copyOf): HighLogic.CurrentGame is null");
    experience = CalculateExperiencePoints(*g);
    experienceLevel = KerbalRoster::CalculateExperienceLevel(experience);
    hasHelmetOn = copyOf.hasHelmetOn;
    hasNeckRingOn = copyOf.hasNeckRingOn;
    hasVisorDown = copyOf.hasVisorDown;
    lightR = copyOf.lightR;
    lightG = copyOf.lightG;
    lightB = copyOf.lightB;
    completedFirstEVA = copyOf.completedFirstEVA;
    SuitTexturePath = copyOf.SuitTexturePath;
    NormalTexturePath = copyOf.NormalTexturePath;
    SpritePath = copyOf.SpritePath;
    ComboId = copyOf.ComboId;
    inventoryNode = copyOf.inventoryNode;
    GE::onProtoCrewMemberLoad.Fire(GE::FromToAction<ProtoCrewMember*, ConfigNode*>(this, nullptr));
}

// ProtoCrewMember.cs:482
ProtoCrewMember::ProtoCrewMember(Game_Modes mode, ConfigNode& node, KerbalType crewType) {
    (void)mode;
    GE::onProtoCrewMemberLoad.Fire(GE::FromToAction<ProtoCrewMember*, ConfigNode*>(this, &node));
    type_ = crewType;
    bool flag = false;
    SaveEnvironment& env = SaveEnvironment::Current();
    for (int i = 0; i < node.values().Count(); ++i) {
        const ConfigNode::Value& value = node.values()[i];
        const std::string& n = value.name;
        const std::string& v = value.value;
        if (n == "lightB") lightB = ParseFloat(v);
        else if (n == "lightG") lightG = ParseFloat(v);
        else if (n == "hero") isHero = ParseBool(v);
        else if (n == "suit") {
            set_suit(ParseEnum<KerbalSuit>(v));
            if (!env.IsExpansionKerbalSuitInstalled(suit())) set_suit(KerbalSuit::Default);
        } else if (n == "hasHelmetOn") hasHelmetOn = ParseBool(v);
        else if (n == "inactiveTimeEnd") inactiveTimeEnd = ParseDouble(v);
        else if (n == "completedFirstEVA") completedFirstEVA = ParseBool(v);
        else if (n == "lightR") lightR = ParseFloat(v);
        else if (n == "tour") hasToured = ParseBool(v);
        else if (n == "spritePath") SpritePath = v;
        else if (n == "outDueToG") outDueToG = ParseBool(v);
        else if (n == "hasNeckRingOn") hasNeckRingOn = ParseBool(v);
        else if (n == "type") type_ = ParseEnum<KerbalType>(v);
        else if (n == "badS") isBadass = ParseBool(v);
        else if (n == "gender") set_gender(ParseEnum<Gender>(v));
        else if (n == "state") {
            int32_t result = -1;
            if (dotnet::TryParseInt32(v, result)) rosterStatus_ = static_cast<RosterStatus>(result);
            else rosterStatus_ = ParseEnum<RosterStatus>(v);
        } else if (n == "hasVisorDown") hasVisorDown = ParseBool(v);
        else if (n == "trait") {
            const std::string* t = node.GetValuePtr("trait");
            trait = t ? std::optional<std::string>(*t) : std::nullopt;
        } else if (n == "idx") seatIdx = ParseInt(v);
        else if (n == "comboId") ComboId = v;
        else if (n == "gExperienced") gExperienced = ParseDouble(v);
        else if (n == "name") name_ = v;
        else if (n == "brave") courage = ParseFloat(v);
        else if (n == "extraXP") extraExperience_ = ParseFloat(v);
        else if (n == "normalTexturePath") NormalTexturePath = v;
        else if (n == "veteran") {
            veteran = ParseBool(v);
            flag = true;
        } else if (n == "suitTexturePath") SuitTexturePath = v;
        else if (n == "dumb") stupidity = ParseFloat(v);
        else if (n == "inactive") inactive_ = ParseBool(v);
        else if (n == "ToD") UTaR = ParseDouble(v);
    }
    if (!flag) {
        if (name_ != "Jebediah Kerman" && name_ != "Bill Kerman" && name_ != "Bob Kerman" && name_ != "Valentina Kerman") {
            veteran = false;
        } else {
            veteran = true;
            isHero = true;
        }
    }
    if (auto chute = FirstChildShared(node, "EVACHUTE")) ChuteNode = chute;
    if (auto inv = FirstChildShared(node, "INVENTORY")) inventoryNode = inv;
    careerLog.Load(node.GetNode("CAREER_LOG"));
    flightLog.Load(node.GetNode("FLIGHT_LOG"));
    KerbalRoster::SetExperienceTrait(*this);
    if (env.currentGame != nullptr) {
        experience = CalculateExperiencePoints(*env.currentGame);
        experienceLevel = KerbalRoster::CalculateExperienceLevel(experience);
    } else {
        GE::onGameStateCreated.Add(this, &ProtoCrewMember::OnGameStateCreated);
        listeningGameStateCreated_ = true;
    }
}

// kspcore: C# keeps a listener alive with its object; C++ must remove it before the object dies
ProtoCrewMember::~ProtoCrewMember() {
    if (listeningGameStateCreated_) GE::onGameStateCreated.Remove(this, &ProtoCrewMember::OnGameStateCreated);
}

// ProtoCrewMember.cs:745
void ProtoCrewMember::OnGameStateCreated(Game* game) {
    if (game == nullptr) throw NullReferenceException("ProtoCrewMember.OnGameStateCreated: game is null");
    experience = CalculateExperiencePoints(*game);
    experienceLevel = KerbalRoster::CalculateExperienceLevel(experience);
    GE::onGameStateCreated.Remove(this, &ProtoCrewMember::OnGameStateCreated);
    listeningGameStateCreated_ = false;
}

// ProtoCrewMember.cs:175
std::string ProtoCrewMember::displayName() const {
    const std::string n = name_.value_or("");
    if (n == "Valentina Kerman") return Localizer::Format("#autoLOC_1100005");
    if (n == "Bob Kerman") return Localizer::Format("#autoLOC_1100004");
    if (n == "Bill Kerman") return Localizer::Format("#autoLOC_1100003");
    if (n == "Jebediah Kerman") return Localizer::Format("#autoLOC_1100002");
    return Localizer::Format(n);
}

// ProtoCrewMember.cs:197
void ProtoCrewMember::set_inactive(bool value) {
    if (inactive_ != value) {
        GE::onKerbalInactiveChange.Fire(this, inactive_, value);
        inactive_ = value;
    }
}

// ProtoCrewMember.cs:215
void ProtoCrewMember::set_rosterStatus(RosterStatus value) {
    GE::onKerbalStatusChange.Fire(this, rosterStatus_, value);
    const RosterStatus data = rosterStatus_;
    rosterStatus_ = value;
    GE::onKerbalStatusChanged.Fire(this, data, value);
}

// ProtoCrewMember.cs:230
void ProtoCrewMember::set_type(KerbalType value) {
    GE::onKerbalTypeChange.Fire(this, type_, value);
    const KerbalType data = type_;
    type_ = value;
    GE::onKerbalTypeChanged.Fire(this, data, value);
}

// ProtoCrewMember.cs:257
ProtoCrewMember::KerbalSuit ProtoCrewMember::suit() {
    if (!SaveEnvironment::Current().IsExpansionKerbalSuitInstalled(suit_)) suit_ = KerbalSuit::Default;
    return suit_;
}
void ProtoCrewMember::set_suit(KerbalSuit value) {
    if (SaveEnvironment::Current().IsExpansionKerbalSuitInstalled(value)) suit_ = value;
}

// ProtoCrewMember.cs:374
ConfigNode::NodePtr ProtoCrewMember::InventoryNode() {
    if (!inventoryNode) SetDefaultInventory();
    return inventoryNode;
}

// ProtoCrewMember.cs:392
void ProtoCrewMember::SetExtraExperience(float value) {
    extraExperience_ = value;
    UpdateExperience();
}

// ProtoCrewMember.cs:405
float ProtoCrewMember::ExperienceLevelDelta() const {
    if (experienceLevel == KerbalRoster::GetExperienceMaxLevel()) return 1.0f;
    if (experience == 0.0f) return 0.0f;
    const float req = KerbalRoster::GetExperienceLevelRequirement(experienceLevel);
    const float req2 = KerbalRoster::GetExperienceLevelRequirement(experienceLevel - 1);
    return f32store((fx(experience) - fx(req2)) / (fx(req) - fx(req2)));
}

// ProtoCrewMember.cs:627
bool ProtoCrewMember::SetInactive(double time, bool additive) {
    const bool result = inactive_;
    if (additive && inactive_) inactiveTimeEnd += time;
    else inactiveTimeEnd = SaveUniversalTime() + time;
    set_inactive(true);
    return result;
}

// ProtoCrewMember.cs:642
bool ProtoCrewMember::CheckActive(double ut) {
    if (inactive_ && ut >= inactiveTimeEnd) {
        set_inactive(false);
        return true;
    }
    return false;
}

// ProtoCrewMember.cs:652
bool ProtoCrewMember::ChangeName(const std::string& newName) {
    Game* g = SaveEnvironment::Current().currentGame;
    if (g != nullptr && g->CrewRoster) {
        if (!g->CrewRoster->ChangeNameCalledFromPCM(shared_from_this(), name_.value_or(""), newName)) return false;
    } else {
        log::Info("[ProtoCrewMember]: Cannot rename kerbal. No loaded Game or CrewRoster.");
    }
    GE::onKerbalNameChange.Fire(this, name_.value_or(""), newName);
    const std::string data = name_.value_or("");
    name_ = newName;
    GE::onKerbalNameChanged.Fire(this, data, newName);
    return true;
}

// ProtoCrewMember.cs:688
void ProtoCrewMember::Save(ConfigNode& node) {
    auto addNullable = [&node](std::string_view key, const std::optional<std::string>& v) {
        if (!v) log::Error("Input is null for field '" + std::string(key) + "' in config node '" + node.name + "'");
        node.AddValue(key, v.value_or(""));
    };
    addNullable("name", name_);
    node.AddValue("gender", EnumName(gender()));
    node.AddValue("type", EnumName(type()));
    addNullable("trait", trait);
    node.AddValue("brave", courage);
    node.AddValue("dumb", stupidity);
    node.AddValue("badS", isBadass);
    node.AddValue("veteran", veteran);
    node.AddValue("tour", hasToured);
    node.AddValue("state", EnumName(rosterStatus()));
    node.AddValue("inactive", inactive());
    node.AddValue("inactiveTimeEnd", dotnet::FormatDoubleG(inactiveTimeEnd, 17));
    node.AddValue("gExperienced", dotnet::FormatDoubleG(gExperienced, 17));
    node.AddValue("outDueToG", outDueToG);
    node.AddValue("ToD", UTaR);
    node.AddValue("idx", static_cast<int32_t>(seatIdx));
    node.AddValue("extraXP", extraExperience_);
    node.AddValue("hasHelmetOn", hasHelmetOn);
    node.AddValue("hasNeckRingOn", hasNeckRingOn);
    node.AddValue("hasVisorDown", hasVisorDown);
    node.AddValue("lightR", lightR);
    node.AddValue("lightG", lightG);
    node.AddValue("lightB", lightB);
    node.AddValue("completedFirstEVA", completedFirstEVA);
    if (ChuteNode) node.AddNode(ChuteNode);
    if (inventoryNode) node.AddNode(inventoryNode);
    node.AddValue("suit", EnumName(suit()));
    node.AddValue("hero", isHero);
    if (SuitTexturePath) node.AddValue("suitTexturePath", *SuitTexturePath);
    if (NormalTexturePath) node.AddValue("normalTexturePath", *NormalTexturePath);
    if (SpritePath) node.AddValue("spritePath", *SpritePath);
    if (ComboId) node.AddValue("comboId", *ComboId);
    careerLog.Save(*node.AddNode("CAREER_LOG"));
    flightLog.Save(*node.AddNode("FLIGHT_LOG"));
    GE::onProtoCrewMemberSave.Fire(GE::FromToAction<ProtoCrewMember*, ConfigNode*>(this, &node));
}

// ProtoCrewMember.cs:810
void ProtoCrewMember::SetDefaultInventory() {
    inventoryNode = std::make_shared<ConfigNode>("INVENTORY");
    if (type() == KerbalType::Tourist) return;
    const SaveEnvironment& env = SaveEnvironment::Current();
    if (env.saveDefaultKerbalInventory) env.saveDefaultKerbalInventory(GetKerbalEVAPartName(), *inventoryNode);
}

// ProtoCrewMember.cs:1008
std::string ProtoCrewMember::GetKerbalEVAPartName() {
    const SaveEnvironment& env = SaveEnvironment::Current();
    if (gender() == Gender::Female) {
        switch (suit()) {
        case KerbalSuit::Vintage:
            if (env.IsExpansionKerbalSuitInstalled(KerbalSuit::Vintage)) return "kerbalEVAfemaleVintage";
            return "kerbalEVAfemale";
        case KerbalSuit::Future:
            if (env.IsExpansionKerbalSuitInstalled(KerbalSuit::Future)) return "kerbalEVAfemaleFuture";
            return "kerbalEVAfemale";
        case KerbalSuit::Slim: return "kerbalEVASlimSuitFemale";
        default: return "kerbalEVAfemale";
        }
    }
    switch (suit()) {
    case KerbalSuit::Vintage:
        if (env.IsExpansionKerbalSuitInstalled(KerbalSuit::Vintage)) return "kerbalEVAVintage";
        return "kerbalEVA";
    case KerbalSuit::Future:
        if (env.IsExpansionKerbalSuitInstalled(KerbalSuit::Future)) return "kerbalEVAFuture";
        return "kerbalEVA";
    case KerbalSuit::Slim: return "kerbalEVASlimSuit";
    default: return "kerbalEVA";
    }
}

// ProtoCrewMember.cs:1205 (GameEvents.onCrewKilled needs an EventReport and KerbalRef.die() the flight scene: not
// fired / not called here)
void ProtoCrewMember::Die() {
    flightLog.AddEntryUnique(FlightLog::EntryType::Die);
    ArchiveFlightLog();
    set_rosterStatus(RosterStatus::Dead);
    if (type() == KerbalType::Tourist) {
        Game* g = SaveEnvironment::Current().currentGame;
        if (g == nullptr || !g->CrewRoster) throw NullReferenceException("ProtoCrewMember.Die: no current game roster");
        g->CrewRoster->Remove(name_.value_or(""));
    }
}

// ProtoCrewMember.cs:1221
void ProtoCrewMember::CheckRespawnTimer(double UT, const GameParameters& gameParameters) {
    if (rosterStatus() == RosterStatus::Missing && UT >= UTaR) {
        if (gameParameters.Difficulty.MissingCrewsRespawn) {
            log::Info("Crewmember " + name_.value_or("") + " has respawned!");
            set_rosterStatus(RosterStatus::Available);
            flightLog.AddEntryUnique(FlightLog::EntryType::Spawn);
        } else {
            log::Info("Crewmember " + name_.value_or("") + " has been missing for too long! Must be dead by now.");
            Die();
        }
    }
}

// ProtoCrewMember.cs:1239
void ProtoCrewMember::StartRespawnPeriod(double timeToRespawn) {
    if (timeToRespawn < 0.0) {
        Game* g = SaveEnvironment::Current().currentGame;
        if (g == nullptr || !g->Parameters) throw NullReferenceException("ProtoCrewMember.StartRespawnPeriod: no game");
        timeToRespawn = g->Parameters->Difficulty.RespawnTimer;
    }
    SetTimeForRespawn(SaveUniversalTime() + timeToRespawn);
}

// ProtoCrewMember.cs:1248
void ProtoCrewMember::SetTimeForRespawn(double UTforRespawn) {
    set_rosterStatus(RosterStatus::Missing);
    UTaR = UTforRespawn;
}

// ProtoCrewMember.cs:1254
void ProtoCrewMember::ArchiveFlightLog() {
    for (int i = 0; i < flightLog.Count(); ++i) careerLog.AddEntry(flightLog[i]);
    careerLog.AddFlight();
    UpdateExperience();
    flightLog.Entries().clear();
    flightLog.AddFlight();
}

// ProtoCrewMember.cs:1267
void ProtoCrewMember::UpdateExperience() {
    const int num = experienceLevel;
    const Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr) throw NullReferenceException("ProtoCrewMember.UpdateExperience: HighLogic.CurrentGame is null");
    experience = CalculateExperiencePoints(*g);
    experienceLevel = KerbalRoster::CalculateExperienceLevel(experience);
    if (num != experienceLevel) GE::onKerbalLevelUp.Fire(this);
}

// ProtoCrewMember.cs:1278
float ProtoCrewMember::CalculateExperiencePoints(const Game& game) const {
    if (!game.Parameters) throw NullReferenceException("ProtoCrewMember.CalculateExperiencePoints: Parameters is null");
    return CalculateExperiencePoints(*game.Parameters, game.Mode);
}

// ProtoCrewMember.cs:1283
float ProtoCrewMember::CalculateExperiencePoints(const GameParameters& parameters, Game_Modes mode) const {
    auto& adv = parameters.CustomParams<GameParameters::AdvancedParams>();
    if (adv.KerbalExperienceEnabled(mode)) {
        if (adv.ImmediateLevelUp) return KerbalRoster::CalculateExperience({&careerLog, &flightLog});
        return KerbalRoster::CalculateExperience({&careerLog});
    }
    return 99999.0f;
}

// =================================================== KerbalRoster ===================================================

namespace {
// KerbalRoster.cs:34-46 (static lists; InitializeExperienceTypes appends on every roster construction)
const float kXpPerLevel[5] = {2.0f, 8.0f, 16.0f, 32.0f, 64.0f};
std::vector<float>& xpHomeValues() { static std::vector<float> v; return v; }
std::vector<std::string>& xpHomeTypes() { static std::vector<std::string> v; return v; }
std::vector<std::string>& xpHomeTypeNames() { static std::vector<std::string> v; return v; }
std::vector<float>& xpNotHomeValues() { static std::vector<float> v; return v; }
std::vector<std::string>& xpNotHomeTypes() { static std::vector<std::string> v; return v; }
std::vector<std::string>& xpNotHomeTypeNames() { static std::vector<std::string> v; return v; }
}  // namespace

// KerbalRoster.cs:146
KerbalRoster::KerbalRoster(Game_Modes mode) : mode_(mode) { InitializeExperienceTypes(); }

// KerbalRoster.cs:154
KerbalRoster::KerbalRoster(ConfigNode& node, Game_Modes mode) : mode_(mode) {
    InitializeExperienceTypes();
    const int count = node.nodes().Count();
    for (int i = 0; i < count; ++i) {
        ConfigNode& child = *node.nodes()[i];
        if (child.name != "KERBAL" && child.name != "CREW") {
            if (child.name != "APPLICANTS") continue;
            const int count2 = child.nodes().Count();
            for (int j = 0; j < count2; ++j)
                if (child.nodes()[j]->name == "RECRUIT") {
                    auto pcm = std::make_shared<ProtoCrewMember>(mode, *child.nodes()[j], ProtoCrewMember::KerbalType::Applicant);
                    if (!pcm->name()) throw ArgumentNullException("key");
                    kerbals_.Add(*pcm->name(), pcm);
                }
        } else {
            auto pcm = std::make_shared<ProtoCrewMember>(mode, child);
            if (!pcm->name()) throw ArgumentNullException("key");
            kerbals_.Add(*pcm->name(), pcm);
        }
    }
}

KerbalRoster::PCM KerbalRoster::operator[](int index) const {
    if (index >= 0 && index < kerbals_.Count() && kerbals_.Count() > 0) return kerbals_.At(index);
    return nullptr;
}
KerbalRoster::PCM KerbalRoster::operator[](const std::string& name) const {
    PCM val;
    kerbals_.TryGetValue(name, val);
    return val;
}

namespace {
std::vector<KerbalRoster::PCM> OfType(const DictionaryValueList<std::string, KerbalRoster::PCM>& k,
                                      ProtoCrewMember::KerbalType t) {
    std::vector<KerbalRoster::PCM> v;
    for (int i = 0; i < k.Count(); ++i)
        if (k.At(i)->type() == t) v.push_back(k.At(i));
    return v;
}
}  // namespace
std::vector<KerbalRoster::PCM> KerbalRoster::Applicants() const { return OfType(kerbals_, ProtoCrewMember::KerbalType::Applicant); }
std::vector<KerbalRoster::PCM> KerbalRoster::Crew() const { return OfType(kerbals_, ProtoCrewMember::KerbalType::Crew); }
std::vector<KerbalRoster::PCM> KerbalRoster::Tourist() const { return OfType(kerbals_, ProtoCrewMember::KerbalType::Tourist); }
std::vector<KerbalRoster::PCM> KerbalRoster::Unowned() const { return OfType(kerbals_, ProtoCrewMember::KerbalType::Unowned); }

// KerbalRoster.cs:188
std::shared_ptr<KerbalRoster> KerbalRoster::GenerateInitialCrewRoster(Game_Modes mode) {
    using KT = ProtoCrewMember::KerbalType;
    using G = ProtoCrewMember::Gender;
    auto make = [](const char* tag, const char* en, G gender, float dumb, float brave, bool badass) {
        auto p = std::make_shared<ProtoCrewMember>(KT::Crew, LocalizedOr(tag, en));
        p->set_gender(gender);
        p->stupidity = dumb;
        p->courage = brave;
        p->isBadass = badass;
        p->seatIdx = -1;
        p->veteran = true;
        p->isHero = true;
        return p;
    };
    auto p1 = make("#autoLOC_20803", "Jebediah Kerman", G::Male, 0.5f, 0.5f, true);
    auto p2 = make("#autoLOC_20811", "Bill Kerman", G::Male, 0.8f, 0.5f, false);
    auto p3 = make("#autoLOC_20819", "Bob Kerman", G::Male, 0.1f, 0.3f, false);
    auto p4 = make("#autoLOC_20827", "Valentina Kerman", G::Female, 0.4f, 0.55f, true);
    auto roster = std::make_shared<KerbalRoster>(mode);
    SetExperienceTrait(*p1, pilotTrait);
    SetExperienceTrait(*p2, engineerTrait);
    SetExperienceTrait(*p3, scientistTrait);
    SetExperienceTrait(*p4, pilotTrait);
    roster->AddCrewMember(p1);
    roster->AddCrewMember(p2);
    roster->AddCrewMember(p3);
    roster->AddCrewMember(p4);
    return roster;
}

// KerbalRoster.cs:250
void KerbalRoster::Init(const Game& st) {
    if (st.flightState && st.Parameters) CheckRosterRespawn(st.UniversalTime(), *st.Parameters);
}

// KerbalRoster.cs:258
void KerbalRoster::CheckRosterRespawn(double universalTime, const GameParameters& parameters) {
    int count = kerbals_.Count();
    while (count-- > 0) kerbals_.At(count)->CheckRespawnTimer(universalTime, parameters);
}

// KerbalRoster.cs:272
void KerbalRoster::Save(ConfigNode& node) const {
    const int count = kerbals_.Count();
    for (int i = 0; i < count; ++i) kerbals_.At(i)->Save(*node.AddNode("KERBAL"));
}

// KerbalRoster.cs:302
KerbalRoster::PCM KerbalRoster::GetNextAvailableKerbal(ProtoCrewMember::KerbalType type) {
    const int count = kerbals_.Count();
    for (int i = 0; i < count; ++i) {
        PCM pcm = kerbals_.At(i);
        if (pcm->type() != type) continue;
        if (pcm->rosterStatus() == ProtoCrewMember::RosterStatus::Missing) {
            Game* g = SaveEnvironment::Current().currentGame;
            if (g == nullptr || !g->Parameters) throw NullReferenceException("GetNextAvailableKerbal: no current game");
            pcm->CheckRespawnTimer(SaveUniversalTime(), *g->Parameters);
        }
        if (pcm->rosterStatus() == ProtoCrewMember::RosterStatus::Available) return pcm;
    }
    return nullptr;
}

// CrewGenerator.cs:138 (Localizer.TryGetStringByTag's out value is null when the tag is missing -- the "Kerman" default
// is overwritten -- sic)
std::optional<std::string> KerbalRoster::CrewGenerator_GetLastName() {
    if (Localizer::Instance() == nullptr) return std::string("Kerman");  // kspcore: no Localizer loaded
    std::string value;
    if (Localizer::TryGetStringByTag("#autoLoc_6003018", value)) return value;
    return std::nullopt;
}

// CrewGenerator.cs:145
std::string KerbalRoster::CrewGenerator_RemoveLastName(const std::string& fullKerbalName) {
    return dotnet::Replace(fullKerbalName, " " + CrewGenerator_GetLastName().value_or(""), "");
}

// KerbalRoster.cs:325
bool KerbalRoster::AddCrewMember(const PCM& crew) {
    const std::string value = CrewGenerator_RemoveLastName(crew->name().value_or(""));
    for (int num = 0; num < kerbals_.Count(); ++num)
        if (kerbals_.KeyAt(num).find(value) != std::string::npos) return false;
    SetExperienceTrait(*crew);
    GE::onKerbalAdded.Fire(crew.get());
    if (!crew->name()) throw ArgumentNullException("key");
    kerbals_.Add(*crew->name(), crew);
    GE::onKerbalAddComplete.Fire(crew.get());
    return true;
}

// KerbalRoster.cs:349
bool KerbalRoster::ChangeNameCalledFromPCM(const PCM& crew, const std::string& oldName, const std::string& newName) {
    if (kerbals_.ContainsKey(newName)) {
        log::Info("[KerbalRoster]: A Kerbal with name:" + newName + " already exists in the roster. Unable to rename " +
                  oldName + " with that new name.");
        return false;
    }
    if (kerbals_.Contains(oldName)) {
        kerbals_.Remove(oldName);
        kerbals_.Add(newName, crew);
        return true;
    }
    log::Info("[KerbalRoster]: A Kerbal with name:" + oldName + " does not exist in the current roster. Unable to rename to " +
              newName);
    return false;
}

// KerbalRoster.cs:366
bool KerbalRoster::ChangeName(const std::string& oldName, const std::string& newName) {
    PCM val;
    if (kerbals_.TryGetValue(oldName, val)) return val->ChangeName(newName);
    log::Info("[KerbalRoster]: A Kerbal with name:" + oldName + " does not exist in the current roster. Unable to rename to " +
              newName);
    return false;
}

bool KerbalRoster::Exists(const std::string& name) const { return kerbals_.Contains(name); }

// KerbalRoster.cs:381
bool KerbalRoster::Remove(const std::string& name) {
    PCM val;
    if (kerbals_.TryGetValue(name, val)) {
        kerbals_.Remove(name);
        GE::onKerbalRemoved.Fire(val.get());
        return true;
    }
    return false;
}
bool KerbalRoster::Remove(const PCM& crew) { return Remove(crew->name().value_or("")); }
void KerbalRoster::Remove(int i) { Remove(kerbals_.At(i)->name().value_or("")); }

// KerbalRoster.cs:403 (SystemUtilities.ExpungeKerbal acts on live vessels: logged, not performed; the mission-builder
// branch needs MissionSystem and is skipped -- no missions are loaded in kspcore yet)
void KerbalRoster::ValidateAssignments(Game& st) {
    if (!st.flightState) return;
    auto& vessels = st.flightState->protoVessels;
    for (auto& protoVessel : vessels) {
        for (auto& pps : protoVessel->protoPartSnapshots) {
            for (size_t k = 0; k < pps->protoModuleCrew.size(); ++k) {
                const std::string crewName = k < pps->protoCrewNames.size() ? pps->protoCrewNames[k] : std::string();
                if (k >= pps->protoCrewNames.size())
                    throw std::out_of_range("KerbalRoster.ValidateAssignments: protoCrewNames index");
                PCM inRoster = (*this)[crewName];
                if (inRoster && pps->protoModuleCrew[k] != inRoster) {
                    log::Warning("[Protocrewmember]: Instance of crewmember " +
                                 (pps->protoModuleCrew[k] ? pps->protoModuleCrew[k]->ToString() : std::string()) +
                                 " in part " + pps->partName.value_or("") + " on " + protoVessel->GetDisplayName() +
                                 " did not match instance of " + inRoster->name().value_or("") +
                                 " on crew roster. (Stored index: " + crewName + ")");
                    pps->protoModuleCrew[k] = inRoster;
                } else if (!inRoster) {
                    log::Warning("[Protocrewmember]: Instance of crewmember " +
                                 (pps->protoModuleCrew[k] ? pps->protoModuleCrew[k]->ToString() : std::string()) +
                                 " in part " + pps->partName.value_or("") + " on " + protoVessel->GetDisplayName() +
                                 " does not exist in the roster");
                    if (pps->protoModuleCrew[k])
                        log::Warning("[KerbalRoster]: SystemUtilities.ExpungeKerbal needs the flight scene (not performed)");
                    else protoVessel->GetVesselCrew();
                }
            }
        }
    }
    for (const PCM& current : Crew()) {
        bool flag = false;
        int count3 = static_cast<int>(vessels.size());
        while (count3-- > 0) {
            const auto& crew = vessels[static_cast<size_t>(count3)]->GetVesselCrew();
            if (std::find(crew.begin(), crew.end(), current) != crew.end()) {
                flag = true;
                break;
            }
        }
        if (current->rosterStatus() == ProtoCrewMember::RosterStatus::Assigned) {
            if (flag) continue;
            log::Warning("[ProtoCrewMember Warning]: Crewmember " + current->name().value_or("") +
                         " found assigned but no vessels reference him. " + current->name().value_or("") +
                         " set as missing.");
            current->StartRespawnPeriod(2000.0);
        } else if (flag) {
            log::Warning("[ProtoCrewMember Warning]: Crewmember " + current->name().value_or("") +
                         " found inside a part but status is set as missing. Vessel must have failed to save earlier. "
                         "Restoring assigned status.");
            current->set_rosterStatus(ProtoCrewMember::RosterStatus::Assigned);
        }
    }
}

int KerbalRoster::IndexOf(const PCM& pcm) const { return kerbals_.IndexOf(pcm); }

// KerbalRoster.cs:621
std::vector<KerbalRoster::PCM> KerbalRoster::Kerbals(ProtoCrewMember::KerbalType type,
                                                     std::initializer_list<ProtoCrewMember::RosterStatus> status) const {
    std::vector<PCM> v;
    for (int i = 0; i < kerbals_.Count(); ++i) {
        const PCM& k = kerbals_.At(i);
        if (k->type() != type) continue;
        if (status.size() != 0) {
            for (auto s : status)
                if (k->rosterStatus() == s) {
                    v.push_back(k);
                    break;
                }
        } else v.push_back(k);
    }
    return v;
}

// KerbalRoster.cs:651
std::vector<KerbalRoster::PCM> KerbalRoster::Kerbals(std::initializer_list<ProtoCrewMember::RosterStatus> status) const {
    std::vector<PCM> v;
    for (int i = 0; i < kerbals_.Count(); ++i) {
        const PCM& k = kerbals_.At(i);
        if (status.size() != 0) {
            for (auto s : status)
                if (k->rosterStatus() == s) {
                    v.push_back(k);
                    break;
                }
        } else v.push_back(k);
    }
    return v;
}

namespace {
template <class Pred>
int CountCrew(const DictionaryValueList<std::string, KerbalRoster::PCM>& k, Pred pred) {
    int num = 0;
    int count = k.Count();
    while (count-- > 0) {
        const auto& p = k.At(count);
        if (p->type() == ProtoCrewMember::KerbalType::Crew && pred(p->rosterStatus())) ++num;
    }
    return num;
}
using RS = ProtoCrewMember::RosterStatus;
}  // namespace
int KerbalRoster::GetActiveCrewCount() const {
    return CountCrew(kerbals_, [](RS s) { return s == RS::Assigned || s == RS::Available || s == RS::Missing; });
}
int KerbalRoster::GetAssignedCrewCount() const { return CountCrew(kerbals_, [](RS s) { return s == RS::Assigned; }); }
int KerbalRoster::GetAvailableCrewCount() const { return CountCrew(kerbals_, [](RS s) { return s == RS::Available; }); }
int KerbalRoster::GetLostCrewCount() const {
    return CountCrew(kerbals_, [](RS s) { return s == RS::Dead || s == RS::Missing; });
}
int KerbalRoster::GetMissingCrewCount() const { return CountCrew(kerbals_, [](RS s) { return s == RS::Missing; }); }
int KerbalRoster::GetKIACrewCount() const { return CountCrew(kerbals_, [](RS s) { return s == RS::Dead; }); }

// KerbalRoster.cs:768
KerbalRoster::PCM KerbalRoster::GetNextApplicant() const {
    for (int i = 0; i < kerbals_.Count(); ++i)
        if (kerbals_.At(i)->type() == ProtoCrewMember::KerbalType::Applicant) return kerbals_.At(i);
    return nullptr;
}

// KerbalRoster.cs:829
void KerbalRoster::UpdateExperience() {
    for (int i = 0; i < Count(); ++i) (*this)[i]->UpdateExperience();
}

// KerbalRoster.cs:851
void KerbalRoster::HireApplicant(const PCM& ap) {
    if (kerbals_.Contains(ap->name().value_or("")) && ap->type() == ProtoCrewMember::KerbalType::Applicant) {
        GE::OnCrewmemberHired.Fire(ap.get(), GetActiveCrewCount());
        ap->set_type(ProtoCrewMember::KerbalType::Crew);
    } else {
        log::Error("[Applicants List Error]: Cannot Hire " + ap->name().value_or("") + ", is not an applicant");
    }
}

// KerbalRoster.cs:864
void KerbalRoster::SackAvailable(const PCM& ap) {
    if (kerbals_.Contains(ap->name().value_or("")) && ap->type() == ProtoCrewMember::KerbalType::Crew &&
        ap->rosterStatus() == RS::Available) {
        ap->set_type(ProtoCrewMember::KerbalType::Applicant);
        GE::OnCrewmemberSacked.Fire(ap.get(), GetActiveCrewCount());
    } else {
        log::Error("[Applicants List Error]: Cannot sack " + ap->name().value_or("") + ", is not part of the available crew");
    }
}

// KerbalRoster.cs:877
void KerbalRoster::RemoveMIA(const PCM& ap) {
    if (kerbals_.Contains(ap->name().value_or("")) && ap->type() == ProtoCrewMember::KerbalType::Crew &&
        ap->rosterStatus() == RS::Missing) {
        ap->set_rosterStatus(RS::Dead);
        GE::OnCrewmemberLeftForDead.Fire(ap.get(), GetActiveCrewCount());
    } else {
        log::Error("[Applicants List Error]: Cannot give up " + ap->name().value_or("") + " for dead, is not MIA");
    }
}

// KerbalRoster.cs:890
void KerbalRoster::RemoveDead(const PCM& ap) {
    if (kerbals_.Contains(ap->name().value_or("")) && ap->type() == ProtoCrewMember::KerbalType::Crew &&
        ap->rosterStatus() == RS::Dead) {
        Remove(ap);
    } else {
        log::Error("[Applicants List Error]: Cannot remove " + ap->name().value_or("") + ", is not dead");
    }
}

// KerbalRoster.cs:982
void KerbalRoster::SetExperienceTrait(ProtoCrewMember& pcm, std::optional<std::string> traitName) {
    Experience::ExperienceSystemConfig& cfg = ExperienceConfigs();
    if (traitName && !traitName->empty()) pcm.trait = *traitName;
    if (pcm.type() == ProtoCrewMember::KerbalType::Tourist && cfg.GetExperienceTraitConfig(touristTrait) != nullptr)
        pcm.trait = touristTrait;
    if (pcm.type() != ProtoCrewMember::KerbalType::Tourist && traitName == touristTrait) pcm.trait = std::nullopt;
    traitName = pcm.trait;
    if (!traitName || traitName->empty()) {
        if (!pcm.name()) throw NullReferenceException("KerbalRoster.SetExperienceTrait: pcm.name is null");
        const int32_t h = StringExtensions::GetHashCode_Net35(*pcm.name());
        if (h == std::numeric_limits<int32_t>::min())
            throw dotnet::OverflowException("Negating the minimum value of a twos complement number is invalid.");
        const int32_t n = static_cast<int32_t>(cfg.TraitNamesNoTourist().size());
        if (n == 0) throw DivideByZeroException();
        traitName = cfg.TraitNamesNoTourist()[static_cast<size_t>(std::abs(h) % n)];
        pcm.trait = traitName;
    }
    // TypeName of the plain ExperienceTrait is Config.Title (the raw "#autoLOC_.." tag), so this early-out only fires for a
    // trait subclass registered under the trait's name (mods) -- stock traits are rebuilt on every call (sic)
    if (pcm.experienceTrait != nullptr && !(pcm.experienceTrait->TypeName() != *traitName)) return;
    Experience::ExperienceTraitConfig* found = nullptr;
    for (const auto& c : cfg.Categories())
        if (*traitName == c->Name()) {
            found = c.get();
            break;
        }
    if (found == nullptr) return;
    pcm.experienceTrait = Experience::ExperienceTrait::Create(*traitName, found, &pcm);
}

// KerbalRoster.cs:1027
bool KerbalRoster::TryGetExperienceTraitConfig(const std::string& traitName, Experience::ExperienceTraitConfig*& traitConfig) {
    for (const auto& c : ExperienceConfigs().Categories())
        if (traitName == c->Name()) {
            traitConfig = c.get();
            return true;
        }
    traitConfig = nullptr;
    return false;
}

// KerbalRoster.cs:1053
void KerbalRoster::SetExperienceLevelImpl(ProtoCrewMember& pcm, int level, const GameParameters& parameters, Game_Modes mode,
                                          const std::function<std::string()>& homeBodyName) {
    if (!parameters.CustomParams<GameParameters::AdvancedParams>().KerbalExperienceEnabled(mode)) level = 5;
    level = std::clamp(level, 0, 5);
    pcm.careerLog.Clear();
    if (level == 0) return;
    FlightLog::EntryType entryType = FlightLog::EntryType::Training1;
    switch (level) {
    case 1: entryType = FlightLog::EntryType::Training1; break;
    case 2: entryType = FlightLog::EntryType::Training2; break;
    case 3: entryType = FlightLog::EntryType::Training3; break;
    case 4: entryType = FlightLog::EntryType::Training4; break;
    default: entryType = FlightLog::EntryType::Training5; break;
    }
    pcm.careerLog.AddEntryUnique(entryType, homeBodyName());
    pcm.experience = pcm.CalculateExperiencePoints(parameters, mode);
    pcm.experienceLevel = CalculateExperienceLevel(pcm.experience);
}

// KerbalRoster.cs:1053 (home body name given)
void KerbalRoster::SetExperienceLevel(ProtoCrewMember& pcm, int level, const GameParameters& parameters, Game_Modes mode,
                                      const std::string& homeBodyName) {
    SetExperienceLevelImpl(pcm, level, parameters, mode, [&] { return homeBodyName; });
}

// KerbalRoster.cs:1053 (FlightGlobals.GetHomeBody().name evaluated when the entry is added)
void KerbalRoster::SetExperienceLevel(ProtoCrewMember& pcm, int level, const GameParameters& parameters, Game_Modes mode) {
    SetExperienceLevelImpl(pcm, level, parameters, mode, [] { return GetHomeBodyName(); });
}

// KerbalRoster.cs:1048
void KerbalRoster::SetExperienceLevel(ProtoCrewMember& pcm, int level) {
    const Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr || !g->Parameters) throw NullReferenceException("KerbalRoster.SetExperienceLevel: HighLogic.CurrentGame is null");
    SetExperienceLevel(pcm, level, *g->Parameters, g->Mode);
}

// FlightGlobals.cs:1797 GetHomeBody().name over PSystemManager.localBodies
std::string KerbalRoster::GetHomeBodyName() {
    for (CelestialBody* body : SaveEnvironment::Current().localBodies)
        if (body != nullptr && body->isHomeWorld) return body->name();
    throw NullReferenceException("FlightGlobals.GetHomeBody() is null");
}

// KerbalRoster.cs:1088
float KerbalRoster::GetExperienceLevelRequirement(int level) {
    if (level < 0) return 0.0f;
    if (level >= 5) return kXpPerLevel[4];
    return kXpPerLevel[level];
}
int KerbalRoster::GetExperienceMaxLevel() { return 5; }

// KerbalRoster.cs:1106 (float comparison in double: Mono rule, same result)
int KerbalRoster::CalculateExperienceLevel(float xp) {
    for (int num = 0; num < 5; ++num)
        if (!(xp >= kXpPerLevel[num])) return num;
    return GetExperienceMaxLevel();
}

// KerbalRoster.cs:1126
float KerbalRoster::CalculateExperience(const std::vector<const FlightLog*>& logs) {
    std::vector<std::string> list, list2;
    std::vector<float> list3;
    const Game* g = SaveEnvironment::Current().currentGame;
    int num = static_cast<int>(logs.size());
    while (num-- > 0) {
        if (g != nullptr && g->Mode == Game_Modes::MISSION && num != 0) continue;
        const std::vector<FlightLog> flights = logs[static_cast<size_t>(num)]->GetFlights();
        for (int i = 0; i < static_cast<int>(flights.size()); ++i)
            if ((g == nullptr || g->Mode != Game_Modes::MISSION || i == 0) &&
                !ExperienceAddFlight(flights[static_cast<size_t>(i)], list, list2, list3)) {
                list.clear();
                list3.clear();
                list2.clear();
            }
    }
    float num2 = 0.0f;
    for (float v : list3) num2 = f32store(fx(num2) + fx(v));
    return num2;
}

// KerbalRoster.cs:1207
std::string KerbalRoster::GetLocalizedExperienceTraitName(const std::optional<std::string>& queriedTrait) {
    const std::string q = queriedTrait.value_or("");
    if (q == "Tourist") return Localizer::Format("#autoLOC_900446");
    if (q == "Scientist") return Localizer::Format("#autoLOC_900442");
    if (q == "Engineer") return Localizer::Format("#autoLOC_900439");
    if (q == "Pilot") return Localizer::Format("#autoLOC_900433");
    return Localizer::Format("#autoLOC_8000022");
}

// KerbalRoster.cs:1225
void KerbalRoster::InitializeExperienceTypes() {
    for (int i = 0; i < 5; ++i)
        AddExperienceType("Training" + dotnet::FormatInt64(i + 1), "#autoLOC_6002265", kXpPerLevel[i], kXpPerLevel[i]);
    AddExperienceType("PlantFlag", "#autoLOC_270135", 2.5f);
    AddExperienceType("Land", "#autoLOC_286262", 2.3f);
    AddExperienceType("Flight", "#autoLOC_6002260", 2.0f, 1.0f);
    AddExperienceType("Suborbit", "#autoLOC_6002261", 0.0f, 1.0f);
    AddExperienceType("Flyby", "#autoLOC_6002262", 1.0f, 1.5f);
    AddExperienceType("Escape", "#autoLOC_6002263", 1.0f);
    AddExperienceType("Orbit", "#autoLOC_6002264", 1.5f, 2.0f);
}

// KerbalRoster.cs:1241
void KerbalRoster::AddExperienceType(const std::string& type, const std::string& typeName, float notHomeValue,
                                     float homeValue) {
    if (homeValue != 0.0f) {
        xpHomeValues().push_back(homeValue);
        xpHomeTypes().push_back(type);
        xpHomeTypeNames().push_back(typeName);
    }
    if (notHomeValue != 0.0f) {
        xpNotHomeValues().push_back(notHomeValue);
        xpNotHomeTypes().push_back(type);
        xpNotHomeTypeNames().push_back(typeName);
    }
}

// KerbalRoster.cs:1257
bool KerbalRoster::ExperienceAddFlight(const FlightLog& flight, std::vector<std::string>& targets,
                                       std::vector<std::string>& types, std::vector<float>& targetXP) {
    const FlightLog::Entry* last = flight.Last();
    if (last == nullptr || last->type == "Die") return false;
    for (int i = 0; i < flight.Count(); ++i) {
        const FlightLog::Entry& entry = flight[i];
        if (entry.target.empty()) continue;
        std::string type;
        float value = 0.0f;
        if (!ExperienceBodyValue(entry.target, entry.type, type, value)) continue;
        const auto it = std::find(targets.begin(), targets.end(), entry.target);
        if (it != targets.end()) {
            const size_t num = static_cast<size_t>(it - targets.begin());
            if (targetXP[num] < value) {
                types[num] = type;
                targetXP[num] = value;
            }
        } else {
            targets.push_back(entry.target);
            types.push_back(type);
            targetXP.push_back(value);
        }
    }
    return true;
}

// KerbalRoster.cs:1297 (PSystemManager.Instance.localBodies = SaveEnvironment::localBodies)
bool KerbalRoster::ExperienceBodyValue(const std::string& bodyName, const std::string& entryType, std::string& type,
                                       float& value) {
    type.clear();
    value = 0.0f;
    if (bodyName.empty()) return false;
    for (CelestialBody* body : SaveEnvironment::Current().localBodies) {
        if (body == nullptr) continue;
        if (body->name() == bodyName || body->displayName() == bodyName) {
            if (body->isHomeWorld)
                return ExperienceHomeTypeValue(entryType, body->scienceValues.RecoveryValue, type, value);
            return ExperienceNotHomeTypeValue(entryType, body->scienceValues.RecoveryValue, type, value);
        }
    }
    return false;
}

// KerbalRoster.cs:1328
bool KerbalRoster::ExperienceHomeTypeValue(const std::string& typeName, float baseValue, std::string& type, float& value) {
    type.clear();
    value = baseValue;
    if (typeName.empty()) return false;
    int count = static_cast<int>(xpHomeTypes().size());
    do {
        if (count-- <= 0) return false;
    } while (typeName != xpHomeTypes()[static_cast<size_t>(count)]);
    value = f32store(fx(value) * fx(xpHomeValues()[static_cast<size_t>(count)]));
    type = xpHomeTypeNames()[static_cast<size_t>(count)];
    return true;
}

// KerbalRoster.cs:1350
bool KerbalRoster::ExperienceNotHomeTypeValue(const std::string& typeName, float baseValue, std::string& type,
                                              float& value) {
    type.clear();
    value = baseValue;
    if (typeName.empty()) return false;
    int count = static_cast<int>(xpNotHomeTypes().size());
    do {
        if (count-- <= 0) return false;
    } while (typeName != xpNotHomeTypes()[static_cast<size_t>(count)]);
    value = f32store(fx(value) * fx(xpNotHomeValues()[static_cast<size_t>(count)]));
    type = xpNotHomeTypeNames()[static_cast<size_t>(count)];
    return true;
}

}  // namespace ksp
