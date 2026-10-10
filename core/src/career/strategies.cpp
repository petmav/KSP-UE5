// WP-36: Strategies/*.cs (Strategy, StrategyConfig, StrategyEffect, StrategyEffectConfig, DepartmentConfig, StrategySystemConfig,
// StrategySystem, CurrencyExchanger) and Strategies/Effects/*.cs (ValueModifier, CurrencyOperation, CurrencyConverter).
#include "ksp/career/strategies.hpp"

#include <cmath>

#include "ksp/career/format.hpp"
#include "ksp/career/funding.hpp"
#include "ksp/core/coroutine_host.hpp"
#include "ksp/core/log.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/enum_extensions.hpp"

namespace ksp::Strategies {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
CareerEnvironment& Env() { return CareerEnvironment::Current(); }
std::string Loc1(const char* tag, const std::string& a) { return Localizer::Format(tag, {a}); }
}  // namespace

// ---- StrategyEffectConfig -------------------------------------------------------------------------------------------------

// StrategyEffectConfig.cs:16
std::unique_ptr<StrategyEffectConfig> StrategyEffectConfig::Create(const ConfigNode& node) {
    auto cfg = std::make_unique<StrategyEffectConfig>();
    const auto value = node.GetValue("name");
    if (!value || value->empty()) return nullptr;
    cfg->name_ = *value;
    cfg->config_ = node.CreateCopy();
    return cfg;
}

// ---- DepartmentConfig -------------------------------------------------------------------------------------------------------

// DepartmentConfig.cs:46
std::unique_ptr<DepartmentConfig> DepartmentConfig::Create(const ConfigNode& node) {
    auto d = std::make_unique<DepartmentConfig>();
    auto value = node.GetValue("name");
    if (!value || value->empty()) return nullptr;
    d->name_ = *value;
    value = node.GetValue("title");
    if (value && !value->empty()) d->title_ = *value;
    value = node.GetValue("desc");
    if (value && !value->empty()) d->description_ = *value;
    value = node.GetValue("description");
    if (value && !value->empty()) d->description_ = *value;
    value = node.GetValue("avatar");
    if (value && !value->empty()) d->avatar_ = *value;
    value = node.GetValue("headName");
    if (value && !value->empty()) d->headName_ = *value;
    value = node.GetValue("headImage");
    if (value && !value->empty()) d->headImageString_ = *value;
    value = node.GetValue("color");
    if (value && !value->empty()) d->color_ = *value;
    return d;
}

// ---- StrategyConfig ----------------------------------------------------------------------------------------------------------

// StrategyConfig.cs:265
DepartmentConfig* StrategyConfig::GetDepartmentConfig(const std::string& name, const std::vector<std::unique_ptr<DepartmentConfig>>& departments) {
    for (const auto& d : departments)
        if (d->Name() == name) return d.get();
    return nullptr;
}

// StrategyConfig.cs:106
std::unique_ptr<StrategyConfig> StrategyConfig::Create(const ConfigNode& node, const std::vector<std::unique_ptr<DepartmentConfig>>& departments) {
    auto c = std::make_unique<StrategyConfig>();
    auto nameValue = node.GetValue("name");
    if (!nameValue) {
        log::Error("StrategyConfig: name cannot be null");
        return nullptr;
    }
    c->name_ = *nameValue;
    auto value = node.GetValue("department");
    if (!value || value->empty()) return nullptr;
    c->departmentName_ = *value;
    c->department_ = GetDepartmentConfig(c->departmentName_, departments);
    if (c->department_ == nullptr) return nullptr;
    c->department_->Strategies().push_back(c.get());
    value = node.GetValue("title");
    if (value && !value->empty()) c->title_ = *value;
    value = node.GetValue("desc");
    if (value && !value->empty()) c->description_ = *value;
    value = node.GetValue("description");
    if (value && !value->empty()) c->description_ = *value;
    value = node.GetValue("icon");
    if (value && !value->empty()) c->iconString_ = *value;
    value = node.GetValue("groupTag");
    if (value && !value->empty()) {
        c->groupTags_ = dotnet::Split(*value, ",", false);
        int num = static_cast<int>(c->groupTags_.size());
        while (num-- > 0) c->groupTags_[static_cast<size_t>(num)] = dotnet::Trim(c->groupTags_[static_cast<size_t>(num)]);
    }
    for (ConfigNode* n : node.GetNodes("EFFECT")) {
        auto e = StrategyEffectConfig::Create(*n);
        if (e) c->effects_.push_back(std::move(e));
    }
    auto num = [&](const char* key, float& a, float& b) {
        auto v = node.GetValue(key);
        if (v && !v->empty()) a = b = dotnet::ParseSingle(*v);
    };
    num("initialCostFunds", c->initialCostFundsMin_, c->initialCostFundsMax_);
    num("initialCostReputation", c->initialCostReputationMin_, c->initialCostReputationMax_);
    num("initialCostScience", c->initialCostScienceMin_, c->initialCostScienceMax_);
    auto single = [&](const char* key, float& a) {
        if (node.HasValue(key)) a = dotnet::ParseSingle(*node.GetValue(key));
    };
    single("initialCostFundsMin", c->initialCostFundsMin_);
    single("initialCostFundsMax", c->initialCostFundsMax_);
    single("initialCostScienceMin", c->initialCostScienceMin_);
    single("initialCostScienceMax", c->initialCostScienceMax_);
    single("initialCostReputationMin", c->initialCostReputationMin_);
    single("initialCostReputationMax", c->initialCostReputationMax_);
    single("requiredReputationMin", c->requiredReputationMin_);
    single("requiredReputationMax", c->requiredReputationMax_);
    value = node.GetValue("minDuration");
    if (value && !value->empty()) c->minLeastDuration_ = c->maxLeastDuration_ = dotnet::ParseDouble(*value);
    value = node.GetValue("maxDuration");
    if (value && !value->empty()) c->minLongestDuration_ = c->maxLongestDuration_ = dotnet::ParseDouble(*value);
    auto dbl = [&](const char* key, double& a) {
        if (node.HasValue(key)) a = dotnet::ParseDouble(*node.GetValue(key));
    };
    dbl("minLeastDuration", c->minLeastDuration_);
    dbl("maxLeastDuration", c->maxLeastDuration_);
    dbl("minLongestDuration", c->minLongestDuration_);
    dbl("maxLongestDuration", c->maxLongestDuration_);
    value = node.GetValue("hasFactorSlider");
    if (value && !value->empty()) {
        bool b = false;
        if (!dotnet::TryParseBool(*value, b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
        c->hasFactorSlider_ = b;
    }
    value = node.GetValue("factorSliderDefault");
    if (value && !value->empty()) c->factorSliderDefault_ = dotnet::ParseSingle(*value);
    value = node.GetValue("factorSliderSteps");
    if (value && !value->empty()) c->factorSliderSteps_ = dotnet::ParseInt32(*value);
    return c;
}

// ---- StrategySystemConfig ----------------------------------------------------------------------------------------------------

// StrategySystemConfig.cs:18
StrategySystemConfig::StrategySystemConfig() {
    LoadDepartmentConfigs();
    LoadStrategyConfigs();
}

// StrategySystemConfig.cs:24
void StrategySystemConfig::LoadDepartmentConfigs() {
    departments_.clear();
    for (ConfigNode* n : GameDatabase::Instance()->GetConfigNodes("STRATEGY_DEPARTMENT")) {
        auto d = DepartmentConfig::Create(*n);
        if (d) {
            if (GetDepartmentConfig(d->Name()) != nullptr)
                log::Error("StrategyConfig: Department '" + d->Name() + "' already exists");
            else
                departments_.push_back(std::move(d));
        }
    }
}

// StrategySystemConfig.cs:46 (a rejected duplicate stays registered in its department's strategy list in C#; it is kept alive here)
void StrategySystemConfig::LoadStrategyConfigs() {
    strategies_.clear();
    for (ConfigNode* n : GameDatabase::Instance()->GetConfigNodes("STRATEGY")) {
        auto s = StrategyConfig::Create(*n, departments_);
        if (s) {
            if (GetStrategyConfig(s->Name()) != nullptr) {
                log::Error("StrategyConfig: Strategy '" + s->Name() + "' already exists");
                discarded_.push_back(std::move(s));
            } else {
                strategies_.push_back(std::move(s));
            }
        }
    }
}

// StrategySystemConfig.cs:68
StrategyConfig* StrategySystemConfig::GetStrategyConfig(const std::string& name) {
    for (auto& s : strategies_)
        if (s->Name() == name) return s.get();
    return nullptr;
}

// StrategySystemConfig.cs:88
DepartmentConfig* StrategySystemConfig::GetDepartmentConfig(const std::string& name) {
    for (auto& d : departments_)
        if (d->Name() == name) return d.get();
    return nullptr;
}

// ---- StrategyEffect -----------------------------------------------------------------------------------------------------------

// StrategyEffect.cs:33
void StrategyEffect::Save(ConfigNode& node) {
    node.AddValue("name", std::string(TypeName()));
    OnSave(node);
}

// StrategyEffect.cs:89
float StrategyEffect::ParentLerp(float minValue, float maxValue) const { return Mathf::Lerp(minValue, maxValue, Parent()->Factor()); }

// ---- Strategy -------------------------------------------------------------------------------------------------------------------

Strategy::~Strategy() = default;

const std::string& Strategy::DepartmentName() const { return config_->DepartmentName(); }                // Strategy.cs:90
DepartmentConfig* Strategy::Department() const { return config_->Department(); }                         // Strategy.cs:92
const std::string& Strategy::Title() const { return config_->Title(); }                                  // Strategy.cs:94
const std::string& Strategy::Description() const { return config_->Description(); }                      // Strategy.cs:96
const std::vector<std::string>& Strategy::GroupTags() const { return config_->GroupTags(); }              // Strategy.cs:98

// Strategy.cs:120
float Strategy::RequiredReputationMin() const { return requiredReputationMin_ == 0.0f ? config_->RequiredReputationMin() : requiredReputationMin_; }
// Strategy.cs:132
float Strategy::RequiredReputationMax() const { return requiredReputationMax_ == 0.0f ? config_->RequiredReputationMax() : requiredReputationMax_; }
// Strategy.cs:144
float Strategy::RequiredReputation() const { return FactorLerp(RequiredReputationMin(), RequiredReputationMax()); }
// Strategy.cs:146
float Strategy::InitialCostFunds() const { return FactorLerp(InitialCostFundsMin(), InitialCostFundsMax()); }
// Strategy.cs:148
float Strategy::InitialCostScience() const { return FactorLerp(InitialCostScienceMin(), InitialCostScienceMax()); }
// Strategy.cs:150
float Strategy::InitialCostReputation() const { return FactorLerp(InitialCostReputationMin(), InitialCostReputationMax()); }
// Strategy.cs:152
float Strategy::InitialCostFundsMin() const { return initialCostFundsMin_ == 0.0f ? config_->InitialCostFundsMin() : initialCostFundsMin_; }
// Strategy.cs:164
float Strategy::InitialCostScienceMin() const { return initialCostScienceMin_ == 0.0f ? config_->InitialCostScienceMin() : initialCostScienceMin_; }
// Strategy.cs:176
float Strategy::InitialCostReputationMin() const { return initialCostReputationMin_ == 0.0f ? config_->InitialCostReputationMin() : initialCostReputationMin_; }
// Strategy.cs:188
float Strategy::InitialCostFundsMax() const { return initialCostFundsMax_ == 0.0f ? config_->InitialCostFundsMax() : initialCostFundsMax_; }
// Strategy.cs:200
float Strategy::InitialCostScienceMax() const { return initialCostScienceMax_ == 0.0f ? config_->InitialCostScienceMax() : initialCostScienceMax_; }
// Strategy.cs:212
float Strategy::InitialCostReputationMax() const { return initialCostReputationMax_ == 0.0f ? config_->InitialCostReputationMax() : initialCostReputationMax_; }
// Strategy.cs:224
double Strategy::MinLeastDuration() const { return minLeastDuration_ == 0.0 ? config_->MinLeastDuration() : minLeastDuration_; }
// Strategy.cs:236
double Strategy::MaxLeastDuration() const { return maxLeastDuration_ == 0.0 ? config_->MaxLeastDuration() : maxLeastDuration_; }
// Strategy.cs:248
double Strategy::MinLongestDuration() const { return minLongestDuration_ == 0.0 ? config_->MinLongestDuration() : minLongestDuration_; }
// Strategy.cs:260
double Strategy::MaxLongestDuration() const { return maxLongestDuration_ == 0.0 ? config_->MaxLongestDuration() : maxLongestDuration_; }
// Strategy.cs:272 (the protected fields, not the properties: always 0 in stock KSP)
double Strategy::LeastDuration() const { return FactorLerp(minLeastDuration_, maxLeastDuration_); }
// Strategy.cs:274 (sic: lerps the LEAST duration fields)
double Strategy::LongestDuration() const { return FactorLerp(minLeastDuration_, maxLeastDuration_); }
// Strategy.cs:276
bool Strategy::NoDuration() const {
    if (config_->MinLeastDuration() == 0.0 && config_->MaxLeastDuration() == 0.0 && config_->MinLongestDuration() == 0.0) return config_->MaxLongestDuration() == 0.0;
    return false;
}
// Strategy.cs:288
bool Strategy::HasFactorSlider() const { return !hasFactorSlider_ ? config_->HasFactorSlider() : hasFactorSlider_; }
// Strategy.cs:300
float Strategy::FactorSliderDefault() const { return factorSliderDefault_ == 0.0f ? config_->FactorSliderDefault() : factorSliderDefault_; }
// Strategy.cs:312
int Strategy::FactorSliderSteps() const { return factorSliderSteps_ == 0 ? config_->FactorSliderSteps() : factorSliderSteps_; }

std::map<std::string, std::function<std::unique_ptr<Strategy>()>>& Strategy::StrategyTypes() {
    static std::map<std::string, std::function<std::unique_ptr<Strategy>()>> m;
    return m;
}

// Strategy.cs:324
std::unique_ptr<Strategy> Strategy::Create(const std::string& typeName, StrategyConfig* config) {
    if (config == nullptr) {
        log::Error("Strategy: Config cannot be null");
        return nullptr;
    }
    std::unique_ptr<Strategy> strategy;
    auto it = StrategyTypes().find(typeName);
    if (it != StrategyTypes().end())
        strategy = it->second();
    else
        strategy = std::make_unique<Strategy>();
    strategy->SetupConfig(config);
    if (strategy->FactorSliderDefault() != 0.0f && strategy->Factor() == 0.0f) strategy->factor_ = strategy->FactorSliderDefault();
    return strategy;
}

// Strategy.cs:344
void Strategy::SetupConfig(StrategyConfig* cfg) {
    config_ = cfg;
    for (size_t i = 0; i < cfg->Effects().size(); ++i) AddEffect(*cfg->Effects()[i]);
}

// Strategy.cs:353
void Strategy::AddEffect(const StrategyEffectConfig& cfg) {
    StrategySystem::EffectFactory factory = StrategySystem::GetStrategyEffectType(cfg.Name());
    if (!factory) {
        log::Error("Strategy: Cannot add effect '" + cfg.Name() + "' as it does not exist.");
        return;
    }
    std::unique_ptr<StrategyEffect> effect = factory(this);
    if (effect) {
        effect->LoadFromConfig(cfg.Config());
        effects_.push_back(std::move(effect));
    }
}

// Strategy.cs:369
float Strategy::FactorLerp(float minValue, float maxValue) const { return Mathf::Lerp(minValue, maxValue, Factor()); }

// Strategy.cs:374 (Mathfx.Lerp(double, double, double): unclamped)
double Strategy::FactorLerp(double minValue, double maxValue) const { return Mathfx::Lerp(minValue, maxValue, static_cast<double>(Factor())); }

// Strategy.cs:522
void Strategy::Load(const ConfigNode& node) {
    auto value = node.GetValue("date");
    if (value) dateActivated_ = dotnet::ParseDouble(*value);
    value = node.GetValue("factor");
    if (value) factor_ = dotnet::ParseSingle(*value);
    OnLoad(node);
    std::vector<ConfigNode*> list = node.GetNodes("EFFECT");
    for (size_t i = 0, count = effects_.size(); i < count; ++i) {
        ConfigNode* configNode = nullptr;
        const std::string name = effects_[i]->TypeName();
        int count2 = static_cast<int>(list.size());
        while (count2-- > 0) {
            const auto n = list[static_cast<size_t>(count2)]->GetValue("name");
            if (n && *n == name) {
                configNode = list[static_cast<size_t>(count2)];
                list.erase(list.begin() + count2);
                break;
            }
        }
        if (configNode != nullptr) effects_[i]->Load(*configNode);
    }
    isActive_ = true;
    Register();
}

// Strategy.cs:560
void Strategy::Save(ConfigNode& node) {
    node.AddValue("name", config_->Name());
    node.AddValue("date", dotnet::FormatDoubleG(dateActivated_, 17));
    node.AddValue("factor", dotnet::FormatSingleG(factor_, 9));
    OnSave(node);
    for (size_t i = 0, count = effects_.size(); i < count; ++i) effects_[i]->Save(*node.AddNode("EFFECT"));
}

// Strategy.cs:573
void Strategy::Register() {
    OnRegister();
    for (size_t i = 0, count = effects_.size(); i < count; ++i) effects_[i]->Register();
}

// Strategy.cs:583
void Strategy::Unregister() {
    OnUnregister();
    for (size_t i = 0, count = effects_.size(); i < count; ++i) effects_[i]->Unregister();
}

// Strategy.cs:593
bool Strategy::Activate() {
    std::string reason;
    if (CanBeActivated(reason)) {
        isActive_ = true;
        Register();
        dateActivated_ = Planetarium::fetch->time;
        if (InitialCostFunds() != 0.0f) Funding::Instance->AddFunds(static_cast<double>(0.0f - Mathf::Abs(InitialCostFunds())), TransactionReasons::StrategySetup);
        if (InitialCostReputation() != 0.0f)
            Reputation::Instance->AddReputation(f32store(0.0 - fx(Mathf::Abs(InitialCostReputation()))), TransactionReasons::StrategySetup);
        if (InitialCostScience() != 0.0f && Env().rndAddScience)
            Env().rndAddScience(f32store(0.0 - fx(Mathf::Abs(InitialCostScience()))), TransactionReasons::StrategySetup);
        return true;
    }
    return false;
}

// Strategy.cs:617
bool Strategy::Deactivate() {
    std::string reason;
    if (CanBeDeactivated(reason)) {
        isActive_ = false;
        Unregister();
        return true;
    }
    return false;
}

// Strategy.cs:628
bool Strategy::CanBeActivated(std::string& reason) {
    reason = "";
    CareerEnvironment::AdministrationState& admin = Env().administration;
    if (!admin.present) throw NullReferenceException("Administration.Instance is null");
    if (admin.ActiveStrategyCount >= admin.MaxActiveStrategies) {
        reason = Localizer::Format("#autoLOC_304820", {dotnet::FormatInt64(admin.MaxActiveStrategies)});
        return false;
    }
    if (StrategySystem::Instance()->HasConflictingActiveStrategies(GroupTags())) {
        reason = Localizer::Format("#autoLOC_304827");
        return false;
    }
    if (Factor() > admin.MaxStrategyCommitLevel) {
        reason = Localizer::Format("#autoLOC_304834", {career::FormatNumber(f32store(fx(admin.MaxStrategyCommitLevel) * 100.0), "G")});
        return false;
    }
    if (static_cast<double>(InitialCostFunds()) != 0.0 && Funding::Instance != nullptr && Funding::Instance->Funds() < static_cast<double>(InitialCostFunds())) {
        reason = Localizer::Format("#autoLOC_304841");
        return false;
    }
    if (InitialCostReputation() != 0.0f && Reputation::Instance != nullptr && Reputation::Instance->reputation() < InitialCostReputation()) {
        reason = Localizer::Format("#autoLOC_304848");
        return false;
    }
    if (InitialCostScience() != 0.0f && Env().rndPresent && Env().rndPresent() && Env().rndCanAfford && !Env().rndCanAfford(InitialCostScience())) {
        reason = Localizer::Format("#autoLOC_304855");
        return false;
    }
    if (RequiredReputation() != 0.0f && Mathf::Ceil(Reputation::CurrentRep()) < Mathf::Floor(RequiredReputation())) {
        reason = Localizer::Format("#autoLOC_304862", {career::FormatNumber(RequiredReputation(), "N0"), career::FormatNumber(Reputation::CurrentRep(), "N0")});
        return false;
    }
    if (!CanActivate(reason)) return false;
    int count = static_cast<int>(effects_.size());
    do {
        if (count-- <= 0) return true;
    } while (effects_[static_cast<size_t>(count)]->CanActivate(reason));
    return false;
}

// Strategy.cs:682
bool Strategy::CanBeDeactivated(std::string& reason) {
    reason = Localizer::Format("#autoLOC_304887");
    if (LeastDuration() != 0.0 && dateActivated_ + LeastDuration() < Planetarium::fetch->time) {
        reason = Localizer::Format("#autoLOC_304890");
        return false;
    }
    if (!CanDeactivate(reason)) return false;
    return true;
}

// Strategy.cs:697
void Strategy::Update() {
    if (LongestDuration() != 0.0 && dateActivated_ + LongestDuration() <= Planetarium::fetch->time) {
        SendStateMessage(Localizer::Format("#autoLOC_304909"), MessageDeactivatedMaxTime());
        Deactivate();
        return;
    }
    OnUpdate();
    for (size_t i = 0, count = effects_.size(); i < count; ++i) effects_[i]->Update();
}

// Strategy.cs:713 (MessageSystem is a GUI seam)
void Strategy::SendStateMessage(const std::string& title, const std::string& message) {
    if (Env().postMessage) Env().postMessage(title, "<b>" + Title() + "</>\n\n" + message);
    log::Info("Strategy (" + Title() + "): " + message);
}

// ---- Effects/ValueModifier -------------------------------------------------------------------------------------------------------

// Effects/ValueModifier.cs:22
ValueModifier::ValueModifier(Strategy* parent, std::string valueId, std::string description, float minValue, float maxValue)
    : StrategyEffect(parent), minValue_(minValue), maxValue_(maxValue), valueId_(std::move(valueId)), effectDescription_(std::move(description)) {}

ValueModifier::~ValueModifier() {
    if (registered_) OnUnregister();
}

// Effects/ValueModifier.cs:36
void ValueModifier::OnLoadFromConfig(const ConfigNode& node) {
    auto value = node.GetValue("minValue");
    if (value) minValue_ = dotnet::ParseSingle(*value);
    value = node.GetValue("maxValue");
    if (value) maxValue_ = dotnet::ParseSingle(*value);
    if (node.HasValue("valueId")) valueId_ = *node.GetValue("valueId");
    if (node.HasValue("effectDescription")) effectDescription_ = *node.GetValue("effectDescription");
}

// Effects/ValueModifier.cs:58
void ValueModifier::OnRegister() {
    GE::Modifiers.onValueModifierQuery.Add(this, &ValueModifier::OnValueModifierQuery);
    registered_ = true;
}

// Effects/ValueModifier.cs:63
void ValueModifier::OnUnregister() {
    GE::Modifiers.onValueModifierQuery.Remove(this, &ValueModifier::OnValueModifierQuery);
    registered_ = false;
}

// Effects/ValueModifier.cs:68
void ValueModifier::OnValueModifierQuery(ValueModifierQuery* q) {
    if (q->ValueId() == valueId_) q->AddDelta(f32store(fx(q->GetInput()) * fx(ParentLerp(minValue_, maxValue_)) - fx(q->GetInput())));
}

// ---- Effects/CurrencyOperation -----------------------------------------------------------------------------------------------------

// Effects/CurrencyOperation.cs:33
CurrencyOperation::CurrencyOperation(Strategy* parent, float minValue, float maxValue, Currency currency, Operator op, TransactionReasons AffectReasons,
                                     std::string description)
    : StrategyEffect(parent), minValue_(minValue), maxValue_(maxValue), currency_(currency), AffectReasons_(AffectReasons),
      effectDescription_(std::move(description)), op_(op) {}

CurrencyOperation::~CurrencyOperation() {
    if (registered_) OnUnregister();
}

namespace {
template <class E>
E ParseEnum(const std::string& s) {
    E e{};
    if (!dotnet::EnumTryParseFull(s, e)) throw ArgumentException("Requested value '" + s + "' was not found.");
    return e;
}
}  // namespace

// Effects/CurrencyOperation.cs:59
void CurrencyOperation::OnLoadFromConfig(const ConfigNode& node) {
    auto value = node.GetValue("minValue");
    if (value) minValue_ = dotnet::ParseSingle(*value);
    value = node.GetValue("maxValue");
    if (value) maxValue_ = dotnet::ParseSingle(*value);
    if (node.HasValue("currency")) currency_ = ParseEnum<Currency>(*node.GetValue("currency"));
    if (node.HasValue("operation")) op_ = ParseEnum<Operator>(*node.GetValue("operation"));
    if (node.HasValue("AffectReasons")) {
        const std::vector<std::string> array = dotnet::Split(*node.GetValue("AffectReasons"), ",", false);
        for (size_t i = 0; i < array.size(); ++i) AffectReasons_ |= ParseEnum<TransactionReasons>(dotnet::Trim(array[i]));
    }
    if (node.HasValue("effectDescription")) effectDescription_ = *node.GetValue("effectDescription");
}

// Effects/CurrencyOperation.cs:94
void CurrencyOperation::OnRegister() {
    GE::Modifiers.OnCurrencyModifierQuery.Add(this, &CurrencyOperation::OnEffectQuery);
    registered_ = true;
}

// Effects/CurrencyOperation.cs:99
void CurrencyOperation::OnUnregister() {
    GE::Modifiers.OnCurrencyModifierQuery.Remove(this, &CurrencyOperation::OnEffectQuery);
    registered_ = false;
}

// Effects/CurrencyOperation.cs:104
void CurrencyOperation::OnEffectQuery(CurrencyModifierQuery* qry) {
    if (static_cast<int>(qry->reason & AffectReasons_) != 0) {
        switch (op_) {
            case Operator::Add: qry->AddDelta(currency_, ParentLerp(minValue_, maxValue_)); break;
            case Operator::Multiply:
                qry->AddDelta(currency_, f32store(fx(ParentLerp(minValue_, maxValue_)) * fx(qry->GetInput(currency_)) - fx(qry->GetInput(currency_))));
                break;
            case Operator::Set: qry->AddDelta(currency_, f32store(fx(ParentLerp(minValue_, maxValue_)) - fx(qry->GetInput(currency_)))); break;
        }
    }
}

// ---- Effects/CurrencyConverter -----------------------------------------------------------------------------------------------------

// Effects/CurrencyConverter.cs:25
CurrencyConverter::CurrencyConverter(Strategy* parent, float minRate, float maxRate, float minShare, float maxShare, Currency input, Currency output,
                                     TransactionReasons affectReasons, std::string effectDescription)
    : StrategyEffect(parent), minRate_(minRate), maxRate_(maxRate), minShare_(minShare), maxShare_(maxShare), input_(input), output_(output),
      AffectReasons_(affectReasons), effectDescription_(std::move(effectDescription)) {
    if (input == output) log::Error("[CurrencyConverter]: Input and Output currencies must not be the same!");
}

CurrencyConverter::~CurrencyConverter() {
    if (registered_) OnUnregister();
}

// Effects/CurrencyConverter.cs:47
void CurrencyConverter::OnLoadFromConfig(const ConfigNode& node) {
    if (node.HasValue("input")) input_ = ParseEnum<Currency>(*node.GetValue("input"));
    if (node.HasValue("output")) output_ = ParseEnum<Currency>(*node.GetValue("output"));
    if (node.HasValue("maxRate")) maxRate_ = dotnet::ParseSingle(*node.GetValue("maxRate"));
    if (node.HasValue("minRate")) minRate_ = dotnet::ParseSingle(*node.GetValue("minRate"));
    if (node.HasValue("maxShare")) maxShare_ = dotnet::ParseSingle(*node.GetValue("maxShare"));
    if (node.HasValue("minShare")) minShare_ = dotnet::ParseSingle(*node.GetValue("minShare"));
    if (node.HasValue("AffectReasons")) {
        const std::vector<std::string> array = dotnet::Split(*node.GetValue("AffectReasons"), ",", false);
        for (size_t i = 0; i < array.size(); ++i) AffectReasons_ |= ParseEnum<TransactionReasons>(dotnet::Trim(array[i]));
    }
    if (node.HasValue("effectDescription")) effectDescription_ = *node.GetValue("effectDescription");
    if (input_ == output_) log::Error("[CurrencyConverter]: Input and Output currencies must not be the same!");
}

// Effects/CurrencyConverter.cs:101
void CurrencyConverter::OnRegister() {
    GE::Modifiers.OnCurrencyModifierQuery.Add(this, &CurrencyConverter::OnEffectQuery);
    registered_ = true;
}

// Effects/CurrencyConverter.cs:106
void CurrencyConverter::OnUnregister() {
    GE::Modifiers.OnCurrencyModifierQuery.Remove(this, &CurrencyConverter::OnEffectQuery);
    registered_ = false;
}

// Effects/CurrencyConverter.cs:111
void CurrencyConverter::OnEffectQuery(CurrencyModifierQuery* qry) {
    if (static_cast<int>(qry->reason & AffectReasons_) != 0) {
        const float num = f32store(fx(qry->GetInput(input_)) * fx(ParentLerp(minShare_, maxShare_)));
        qry->AddDelta(input_, f32store(0.0 - fx(num)));
        qry->AddDelta(output_, f32store(fx(ParentLerp(minRate_, maxRate_)) * fx(num)));
        log::Info("[CurrencyConverter for " + Parent()->Title() + "]: " + dotnet::FormatSingleG(num) + " " + dotnet::EnumToString(input_) + " taken, yields " +
                  dotnet::FormatSingleG(f32store(fx(ParentLerp(minRate_, maxRate_)) * fx(num))) + " " + dotnet::EnumToString(output_));
    }
}

// Effects/CurrencyConverter.cs:122
bool CurrencyConverter::CanActivate(std::string& reason) {
    if (StrategySystem::Instance() != nullptr) {
        float num = ParentLerp(minShare_, maxShare_);
        const int count = static_cast<int>(StrategySystem::Instance()->Strategies().size());
        for (int i = 0; i < count; ++i) {
            Strategy* strategy = StrategySystem::Instance()->Strategies()[static_cast<size_t>(i)].get();
            if (!strategy->IsActive()) continue;
            const int count2 = static_cast<int>(strategy->Effects().size());
            for (int j = 0; j < count2; ++j) {
                auto* cc = dynamic_cast<CurrencyConverter*>(strategy->Effects()[static_cast<size_t>(j)].get());
                if (cc != nullptr && cc->input_ == input_) num = f32store(fx(num) + fx(cc->ParentLerp(cc->minShare_, cc->maxShare_)));
            }
        }
        if (num <= 1.0f) return true;
        reason = Localizer::Format("#autoLOC_303729", {util::displayDescription(input_)});
        return false;
    }
    return true;
}

// ---- CurrencyExchanger --------------------------------------------------------------------------------------------------------------

// CurrencyExchanger.cs:24
CurrencyExchanger::CurrencyExchanger(Strategy* parent, float minRate, float maxRate, float minShare, float maxShare, Currency input, Currency output)
    : StrategyEffect(parent), minRate_(minRate), maxRate_(maxRate), minShare_(minShare), maxShare_(maxShare), input_(input), output_(output) {
    if (input == output) log::Error("[CurrencyConverter]: Input and Output currencies must not be the same!");
}

// CurrencyExchanger.cs:44
void CurrencyExchanger::OnLoadFromConfig(const ConfigNode& node) {
    if (node.HasValue("input")) input_ = ParseEnum<Currency>(*node.GetValue("input"));
    if (node.HasValue("output")) output_ = ParseEnum<Currency>(*node.GetValue("output"));
    if (node.HasValue("maxRate")) maxRate_ = dotnet::ParseSingle(*node.GetValue("maxRate"));
    if (node.HasValue("minRate")) minRate_ = dotnet::ParseSingle(*node.GetValue("minRate"));
    if (node.HasValue("maxShare")) maxShare_ = dotnet::ParseSingle(*node.GetValue("maxShare"));
    if (node.HasValue("minShare")) minShare_ = dotnet::ParseSingle(*node.GetValue("minShare"));
    if (input_ == output_) log::Error("[CurrencyExchanger]: Input and Output currencies must not be the same!");
}

// CurrencyExchanger.cs:82
double CurrencyExchanger::GetTotalInput(float shareLerp) {
    double result = 0.0;
    switch (input_) {
        case Currency::Funds: result = Funding::Instance->Funds() * static_cast<double>(shareLerp); break;
        case Currency::Science: result = fx(Env().rndScience ? Env().rndScience() : 0.0f) * fx(shareLerp); break;
        case Currency::Reputation: result = (fx(Reputation::Instance->reputation()) + 1000.0) * fx(shareLerp); break;
    }
    return result;
}

// CurrencyExchanger.cs:100
double CurrencyExchanger::GetTotalOutput(double input, double rateLerp) { return input * rateLerp; }

// CurrencyExchanger.cs:105
void CurrencyExchanger::OnRegister() {
    const double totalInput = GetTotalInput(ParentLerp(minShare_, maxShare_));
    const double totalOutput = GetTotalOutput(totalInput, static_cast<double>(ParentLerp(minRate_, maxRate_)));
    switch (input_) {
        case Currency::Funds: Funding::Instance->AddFunds(0.0 - totalInput, TransactionReasons::StrategyInput); break;
        case Currency::Science:
            if (Env().rndAddScience) Env().rndAddScience(static_cast<float>(0.0 - totalInput), TransactionReasons::StrategyInput);
            break;
        case Currency::Reputation: Reputation::Instance->AddReputation(static_cast<float>(0.0 - totalInput), TransactionReasons::StrategyInput); break;
    }
    switch (output_) {
        case Currency::Funds: Funding::Instance->AddFunds(totalOutput, TransactionReasons::StrategyOutput); break;
        case Currency::Science:
            if (Env().rndAddScience) Env().rndAddScience(static_cast<float>(totalOutput), TransactionReasons::StrategyOutput);
            break;
        case Currency::Reputation: Reputation::Instance->AddReputation(static_cast<float>(totalOutput), TransactionReasons::StrategyOutput); break;
    }
    Env().Scheduler().StartCoroutine(CallbackUtil::DelayedCallback(
        1, Delegate<void()>::Lambda(this, [this] {
            Parent()->Deactivate();
            if (Env().administration.RedrawPanels) Env().administration.RedrawPanels();
        })));
}

// CurrencyExchanger.cs:140
void CurrencyExchanger::OnUnregister() {}

// CurrencyExchanger.cs:144
bool CurrencyExchanger::CanActivate(std::string& reason) {
    (void)reason;
    return true;
}

// ---- StrategySystem -----------------------------------------------------------------------------------------------------------------

namespace {
std::map<std::string, StrategySystem::EffectFactory>& EffectTypes() {
    static std::map<std::string, StrategySystem::EffectFactory> m;
    return m;
}
void EnsureStockEffectTypes() {
    auto& m = EffectTypes();
    if (!m.empty()) return;
    m["CurrencyConverter"] = [](Strategy* p) { return std::unique_ptr<StrategyEffect>(new CurrencyConverter(p)); };
    m["CurrencyOperation"] = [](Strategy* p) { return std::unique_ptr<StrategyEffect>(new CurrencyOperation(p)); };
    m["ValueModifier"] = [](Strategy* p) { return std::unique_ptr<StrategyEffect>(new ValueModifier(p)); };
    m["CurrencyExchanger"] = [](Strategy* p) { return std::unique_ptr<StrategyEffect>(new CurrencyExchanger(p)); };
}
}  // namespace

StrategySystem::StrategySystem() : ScenarioModule("StrategySystem") {}
StrategySystem::~StrategySystem() { OnDestroy(); }

// StrategySystem.cs:131
StrategySystem::EffectFactory StrategySystem::GetStrategyEffectType(const std::string& typeName) {
    EnsureStockEffectTypes();
    auto it = EffectTypes().find(typeName);
    return it == EffectTypes().end() ? EffectFactory() : it->second;
}

void StrategySystem::RegisterEffectType(const std::string& typeName, EffectFactory factory) {
    EnsureStockEffectTypes();
    EffectTypes()[typeName] = std::move(factory);
}

// StrategySystem.cs:94
bool StrategySystem::HasStrategyType(const std::string& typeName) { return Strategy::StrategyTypes().count(typeName) != 0; }

// StrategySystem.cs:31
std::vector<Strategy*> StrategySystem::StrategiesActive() {
    std::vector<Strategy*> r;
    for (auto& s : strategies_)
        if (s->IsActive()) r.push_back(s.get());
    return r;
}

// StrategySystem.cs:50
std::vector<Strategy*> StrategySystem::StrategiesInactive() {
    std::vector<Strategy*> r;
    for (auto& s : strategies_)
        if (!s->IsActive()) r.push_back(s.get());
    return r;
}

// StrategySystem.cs:151
void StrategySystem::OnAwake() {
    instance_ = this;
    EnsureStockEffectTypes();  // GenerateTypes
    log::Info("[StrategySystem]: Found " + std::to_string(Strategy::StrategyTypes().size()) + " strategy types");
    log::Info("[StrategySystem]: Found " + std::to_string(EffectTypes().size()) + " effect types");
    systemConfig_ = std::make_unique<StrategySystemConfig>();
}

// StrategySystem.cs:158 (the C# OnDestroy is the Unity destroy; the destructor runs it)
void StrategySystem::OnDestroy() {
    Unregister();
    if (instance_ != nullptr && instance_ == this) instance_ = nullptr;
}

// StrategySystem.cs:167
void StrategySystem::Unregister() {
    for (size_t i = 0, count = strategies_.size(); i < count; ++i) strategies_[i]->Unregister();
}

// StrategySystem.cs:176 (the node is copied: the coroutine outlives the caller's reference)
void StrategySystem::OnLoad(ConfigNode& gameNode) {
    ConfigNode::NodePtr copy = gameNode.CreateCopy();
    Env().Scheduler().StartCoroutine(OnLoadRoutine(copy));
}

// StrategySystem.cs:181
Enumerator StrategySystem::OnLoadRoutine(ConfigNode::NodePtr gameNode) {
    co_yield YieldInstruction::Null();
    LoadNow(*gameNode);
}

// StrategySystem.cs:181 (after the yield)
void StrategySystem::LoadNow(ConfigNode& gameNode) {
    strategies_.clear();
    ConfigNode* node = gameNode.GetNode("STRATEGIES");
    if (node == nullptr)
        LoadStrategies({});
    else
        LoadStrategies(node->GetNodes("STRATEGY"));
}

// StrategySystem.cs:196
void StrategySystem::LoadStrategies(std::vector<ConfigNode*> stratNodes) {
    for (size_t i = 0; i < systemConfig_->Strategies().size(); ++i) {
        StrategyConfig* strategyConfig = systemConfig_->Strategies()[i].get();
        strategies_.push_back(Strategy::Create(strategyConfig->Name(), strategyConfig));
    }
    for (size_t j = 0, count = strategies_.size(); j < count; ++j) {
        Strategy* strategy = strategies_[j].get();
        int count2 = static_cast<int>(stratNodes.size());
        while (count2-- > 0) {
            const auto n = stratNodes[static_cast<size_t>(count2)]->GetValue("name");
            if (n && *n == strategy->Config()->Name()) {
                ConfigNode* node = stratNodes[static_cast<size_t>(count2)];
                stratNodes.erase(stratNodes.begin() + count2);
                strategy->Load(*node);
                break;
            }
        }
    }
}

// StrategySystem.cs:226
void StrategySystem::OnSave(ConfigNode& gameNode) {
    ConfigNode* configNode = gameNode.AddNode("STRATEGIES");
    for (size_t i = 0, count = strategies_.size(); i < count; ++i) {
        Strategy* strategy = strategies_[i].get();
        if (strategy->IsActive()) strategy->Save(*configNode->AddNode("STRATEGY"));
    }
}

// StrategySystem.cs:241
void StrategySystem::Update() {
    for (size_t i = 0, count = strategies_.size(); i < count; ++i)
        if (strategies_[i]->IsActive()) strategies_[i]->Update();
}

// StrategySystem.cs:253
bool StrategySystem::HasActiveStrategy(const std::string& department) {
    for (auto& s : strategies_)
        if (s->IsActive() && s->DepartmentName() == department) return true;
    return false;
}

// StrategySystem.cs:273
bool StrategySystem::HasConflictingActiveStrategies(const std::vector<std::string>& groupTags) {
    std::vector<Strategy*> list;
    int count = static_cast<int>(Strategies().size());
    while (count-- > 0) {
        Strategy* strategy = Strategies()[static_cast<size_t>(count)].get();
        if (strategy->IsActive()) list.push_back(strategy);
    }
    int num = 0;
    int count2 = static_cast<int>(list.size());
    while (count2-- > 0) {
        bool flag = false;
        Strategy* strategy = list[static_cast<size_t>(count2)];
        int num2 = static_cast<int>(strategy->GroupTags().size());
        while (num2-- > 0) {
            int num3 = static_cast<int>(groupTags.size());
            while (num3-- > 0) {
                if (!(strategy->GroupTags()[static_cast<size_t>(num2)] != groupTags[static_cast<size_t>(num3)])) {
                    flag = true;
                    break;
                }
            }
            if (flag) break;
        }
        if (flag) num++;
        if (num >= 2) break;
    }
    const int num4 = 3 - num;
    int count3 = static_cast<int>(list.size());
    while (count3-- > 0) {
        Strategy* strategy = strategies_[static_cast<size_t>(count3)].get();  // sic: indexes `strategies`, not `list`
        num = 0;
        int num5 = static_cast<int>(groupTags.size());
        while (num5-- > 0) {
            int num6 = static_cast<int>(strategy->GroupTags().size());
            while (num6-- > 0) {
                if (strategy->GroupTags()[static_cast<size_t>(num6)] == groupTags[static_cast<size_t>(num5)]) {
                    num++;
                    break;
                }
            }
            if (num >= num4) return true;
        }
    }
    return false;
}

// StrategySystem.cs:344
std::vector<Strategy*> StrategySystem::GetStrategies(const std::string& department) {
    std::vector<Strategy*> list;
    for (size_t i = 0; i < strategies_.size(); ++i)
        if (strategies_[i]->DepartmentName() == department) list.push_back(strategies_[i].get());
    return list;
}

void StrategySystem::Register() {
    ScenarioModuleFactory::Register("StrategySystem", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new StrategySystem()); });
}

}  // namespace ksp::Strategies
