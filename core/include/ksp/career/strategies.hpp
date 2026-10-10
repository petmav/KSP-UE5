// WP-36: the strategy system. Strategies/Strategy.cs, StrategyConfig.cs, StrategyEffect.cs, StrategyEffectConfig.cs,
// DepartmentConfig.cs, StrategySystemConfig.cs, StrategySystem.cs, CurrencyExchanger.cs and Effects/{ValueModifier,
// CurrencyOperation, CurrencyConverter}.cs.
//
// Configs come from the GameDatabase (`STRATEGY_DEPARTMENT` and `STRATEGY` nodes: GameData/Squad/Strategies/Departments.cfg and
// Strategies.cfg). A strategy effect registers on GameEvents.Modifiers.OnCurrencyModifierQuery / onValueModifierQuery while its
// strategy is active; its delta is what the Funding / Reputation / R&D modules apply (see career/funding.hpp).
//
// Not ported (GUI): Strategy.GetText / GetEffectText / Text / Effect, StrategyEffect.Description / GetDescription, the cached
// localised strings, MessageSystem (CareerEnvironment::postMessage gets the title and text). The reflection that finds the
// Strategy / StrategyEffect subclasses (AssemblyLoader.loadedAssemblies) is a registry: the four stock effects are registered,
// there is no stock Strategy subclass.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/career/career_environment.hpp"
#include "ksp/career/currency.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/save/game.hpp"

namespace ksp::Strategies {

class Strategy;
class StrategyConfig;

// StrategyEffectConfig.cs:6
class StrategyEffectConfig {
public:
    const std::string& Name() const { return name_; }          // :12
    const ConfigNode& Config() const { return *config_; }      // :14
    static std::unique_ptr<StrategyEffectConfig> Create(const ConfigNode& node);  // :16 (nullptr when `name` is empty)

private:
    std::string name_;
    ConfigNode::NodePtr config_;
};

// DepartmentConfig.cs:8 (avatar prefab, head image texture: asset handles are not loaded; their strings are kept)
class DepartmentConfig {
public:
    const std::string& Name() const { return name_; }               // :28
    const std::string& Title() const { return title_; }             // :30
    const std::string& Description() const { return description_; } // :32
    const std::string& AvatarString() const { return avatar_; }     // :34 (AvatarPrefab: AssetBase.GetPrefab(value) ?? GetModel(value))
    const std::string& HeadName() const { return headName_; }       // :36
    const std::string& HeadImageString() const { return headImageString_; }  // :38
    const std::string& ColorString() const { return color_; }       // :42 (ConfigNode.ParseColor32 of it is the Color)
    std::vector<StrategyConfig*>& Strategies() { return strategies_; }  // :44
    static std::unique_ptr<DepartmentConfig> Create(const ConfigNode& node);  // :46

private:
    std::string name_, title_, description_, avatar_, headName_, headImageString_, color_;
    std::vector<StrategyConfig*> strategies_;
};

// StrategyConfig.cs:8
class StrategyConfig {
public:
    const std::string& Name() const { return name_; }                    // :58
    const std::string& DepartmentName() const { return departmentName_; }  // :60
    DepartmentConfig* Department() const { return department_; }         // :62
    const std::string& Title() const { return title_; }                  // :64
    const std::string& Description() const { return description_; }      // :66
    const std::string& IconString() const { return iconString_; }        // :68
    const std::vector<std::string>& GroupTags() const { return groupTags_; }  // :72
    const std::vector<std::unique_ptr<StrategyEffectConfig>>& Effects() const { return effects_; }  // :74
    float InitialCostFundsMin() const { return initialCostFundsMin_; }   // :76
    float InitialCostScienceMin() const { return initialCostScienceMin_; }  // :78
    float InitialCostReputationMin() const { return initialCostReputationMin_; }  // :80
    float InitialCostFundsMax() const { return initialCostFundsMax_; }   // :82
    float InitialCostScienceMax() const { return initialCostScienceMax_; }  // :84
    float InitialCostReputationMax() const { return initialCostReputationMax_; }  // :86
    float RequiredReputationMin() const { return requiredReputationMin_; }  // :88
    float RequiredReputationMax() const { return requiredReputationMax_; }  // :90
    double MinLeastDuration() const { return minLeastDuration_; }        // :92
    double MaxLeastDuration() const { return maxLeastDuration_; }        // :94
    double MaxLongestDuration() const { return maxLongestDuration_; }    // :96
    double MinLongestDuration() const { return minLongestDuration_; }    // :98
    bool HasFactorSlider() const { return hasFactorSlider_; }            // :100
    float FactorSliderDefault() const { return factorSliderDefault_; }   // :102
    int FactorSliderSteps() const { return factorSliderSteps_; }         // :104
    // :106 (nullptr when name / department are missing or unknown; departments keep a back pointer)
    static std::unique_ptr<StrategyConfig> Create(const ConfigNode& node, const std::vector<std::unique_ptr<DepartmentConfig>>& departments);

private:
    static DepartmentConfig* GetDepartmentConfig(const std::string& name, const std::vector<std::unique_ptr<DepartmentConfig>>& departments);  // :265
    std::string name_, departmentName_, title_, description_, iconString_;
    DepartmentConfig* department_ = nullptr;
    std::vector<std::string> groupTags_;
    std::vector<std::unique_ptr<StrategyEffectConfig>> effects_;
    float initialCostFundsMin_ = 0, initialCostScienceMin_ = 0, initialCostReputationMin_ = 0;
    float initialCostFundsMax_ = 0, initialCostScienceMax_ = 0, initialCostReputationMax_ = 0;
    float requiredReputationMin_ = 0, requiredReputationMax_ = 0;
    double minLeastDuration_ = 0, maxLeastDuration_ = 0, maxLongestDuration_ = 0, minLongestDuration_ = 0;
    bool hasFactorSlider_ = false;
    float factorSliderDefault_ = 0;
    int factorSliderSteps_ = 0;
};

// StrategySystemConfig.cs:8
class StrategySystemConfig {
public:
    StrategySystemConfig();  // :18 (reads GameDatabase.Instance)
    std::vector<std::unique_ptr<DepartmentConfig>>& Departments() { return departments_; }  // :14
    std::vector<std::unique_ptr<StrategyConfig>>& Strategies() { return strategies_; }      // :16
    StrategyConfig* GetStrategyConfig(const std::string& name);       // :68
    DepartmentConfig* GetDepartmentConfig(const std::string& name);   // :88

private:
    void LoadDepartmentConfigs();  // :24
    void LoadStrategyConfigs();    // :46
    std::vector<std::unique_ptr<DepartmentConfig>> departments_;
    std::vector<std::unique_ptr<StrategyConfig>> strategies_;
    std::vector<std::unique_ptr<StrategyConfig>> discarded_;  // duplicates the C# drops but its departments still point at
};

// StrategyEffect.cs:5
class StrategyEffect {
public:
    explicit StrategyEffect(Strategy* parent) : parent_(parent) {}  // :13
    virtual ~StrategyEffect() = default;
    Strategy* Parent() const { return parent_; }          // :9
    virtual const char* TypeName() const { return "StrategyEffect"; }  // GetType().Name
    void Register() { OnRegister(); }                     // :18
    void Unregister() { OnUnregister(); }                 // :23
    void Load(const ConfigNode& node) { OnLoad(node); }   // :28
    void Save(ConfigNode& node);                          // :33
    void LoadFromConfig(const ConfigNode& node) { OnLoadFromConfig(node); }  // :39
    void Update() { OnUpdate(); }                         // :44
    virtual bool CanActivate(std::string& reason) { (void)reason; return true; }  // :78
    float ParentLerp(float minValue, float maxValue) const;  // :89

protected:
    virtual void OnRegister() {}                          // :54
    virtual void OnUnregister() {}                        // :58
    virtual void OnLoad(const ConfigNode&) {}             // :62
    virtual void OnSave(ConfigNode&) {}                   // :66
    virtual void OnLoadFromConfig(const ConfigNode&) {}   // :70
    virtual void OnUpdate() {}                            // :74

private:
    Strategy* parent_;
};

// Strategy.cs:10
class Strategy {
public:
    Strategy() = default;
    virtual ~Strategy();
    bool IsActive() const { return isActive_; }                       // :86
    StrategyConfig* Config() const { return config_; }                // :88
    const std::string& DepartmentName() const;                        // :90
    DepartmentConfig* Department() const;                             // :92
    const std::string& Title() const;                                 // :94
    const std::string& Description() const;                           // :96
    const std::vector<std::string>& GroupTags() const;                // :98
    float Factor() const { return factor_; }                          // :104
    void SetFactor(float f) { factor_ = f; }
    double DateActivated() const { return dateActivated_; }           // :116
    std::vector<std::unique_ptr<StrategyEffect>>& Effects() { return effects_; }  // :118
    float RequiredReputationMin() const;   // :120
    float RequiredReputationMax() const;   // :132
    float RequiredReputation() const;      // :144
    float InitialCostFunds() const;        // :146
    float InitialCostScience() const;      // :148
    float InitialCostReputation() const;   // :150
    float InitialCostFundsMin() const;     // :152
    float InitialCostScienceMin() const;   // :164
    float InitialCostReputationMin() const;  // :176
    float InitialCostFundsMax() const;     // :188
    float InitialCostScienceMax() const;   // :200
    float InitialCostReputationMax() const;  // :212
    double MinLeastDuration() const;       // :224
    double MaxLeastDuration() const;       // :236
    double MinLongestDuration() const;     // :248
    double MaxLongestDuration() const;     // :260
    double LeastDuration() const;          // :272
    double LongestDuration() const;        // :274 (sic: lerps the LEAST duration fields)
    bool NoDuration() const;               // :276
    bool HasFactorSlider() const;          // :288
    float FactorSliderDefault() const;     // :300
    int FactorSliderSteps() const;         // :312

    // :324 Create(type, config): typeName selects a registered Strategy subclass (empty / unknown: Strategy)
    static std::unique_ptr<Strategy> Create(const std::string& typeName, StrategyConfig* config);
    // the registry of Strategy subclasses (StrategySystem.GenerateStrategyTypes); stock KSP has none
    static std::map<std::string, std::function<std::unique_ptr<Strategy>()>>& StrategyTypes();
    void Load(const ConfigNode& node);     // :522
    void Save(ConfigNode& node);           // :560
    void Register();                       // :573
    void Unregister();                     // :583
    bool Activate();                       // :593
    bool Deactivate();                     // :617
    bool CanBeActivated(std::string& reason);    // :628
    bool CanBeDeactivated(std::string& reason);  // :682
    void Update();                         // :697

protected:
    float FactorLerp(float minValue, float maxValue) const;     // :369
    double FactorLerp(double minValue, double maxValue) const;  // :374
    virtual void OnLoad(const ConfigNode&) {}      // :379
    virtual void OnSave(ConfigNode&) {}            // :383
    virtual void OnRegister() {}                   // :387
    virtual void OnUnregister() {}                 // :391
    virtual void OnUpdate() {}                     // :395
    virtual bool CanActivate(std::string& reason) { (void)reason; return true; }    // :512
    virtual bool CanDeactivate(std::string& reason) { (void)reason; return true; }  // :517
    virtual std::string MessageDeactivatedMaxTime() { return ""; }                  // :399
    void SendStateMessage(const std::string& title, const std::string& message);   // :713
    float factor_ = 0.0f;                          // :14
    float requiredReputationMin_ = 0, requiredReputationMax_ = 0;  // :20, :22
    float initialCostFundsMin_ = 0, initialCostScienceMin_ = 0, initialCostReputationMin_ = 0;   // :24-:28
    float initialCostFundsMax_ = 0, initialCostScienceMax_ = 0, initialCostReputationMax_ = 0;   // :30-:34
    double minLeastDuration_ = 0, maxLeastDuration_ = 0, minLongestDuration_ = 0, maxLongestDuration_ = 0;  // :36-:42
    bool hasFactorSlider_ = false;                 // :44
    float factorSliderDefault_ = 0.0f;             // :46
    int factorSliderSteps_ = 0;                    // :48

private:
    void SetupConfig(StrategyConfig* cfg);         // :344
    void AddEffect(const StrategyEffectConfig& cfg);  // :353
    bool isActive_ = false;                        // :12
    double dateActivated_ = 0.0;                   // :16
    std::vector<std::unique_ptr<StrategyEffect>> effects_;  // :18
    StrategyConfig* config_ = nullptr;
};

// Effects/ValueModifier.cs:7
class ValueModifier : public StrategyEffect {
public:
    explicit ValueModifier(Strategy* parent) : StrategyEffect(parent) {}  // :17
    ValueModifier(Strategy* parent, std::string valueId, std::string description, float minValue, float maxValue);  // :22
    ~ValueModifier() override;
    const char* TypeName() const override { return "ValueModifier"; }
    void OnValueModifierQuery(ValueModifierQuery* q);   // :68
protected:
    void OnLoadFromConfig(const ConfigNode& node) override;  // :36
    void OnRegister() override;    // :58
    void OnUnregister() override;  // :63
private:
    float minValue_ = 0, maxValue_ = 0;
    std::string valueId_, effectDescription_;
    bool registered_ = false;
};

// Effects/CurrencyOperation.cs:7
class CurrencyOperation : public StrategyEffect {
public:
    enum class Operator : int { Add, Multiply, Set };  // :9
    explicit CurrencyOperation(Strategy* parent) : StrategyEffect(parent) {}  // :28
    CurrencyOperation(Strategy* parent, float minValue, float maxValue, Currency currency, Operator op,
                      TransactionReasons AffectReasons, std::string description);  // :33
    ~CurrencyOperation() override;
    const char* TypeName() const override { return "CurrencyOperation"; }
    virtual void OnEffectQuery(CurrencyModifierQuery* qry);  // :104
protected:
    void OnLoadFromConfig(const ConfigNode& node) override;  // :59
    void OnRegister() override;    // :94
    void OnUnregister() override;  // :99
private:
    float minValue_ = 0, maxValue_ = 0;
    Currency currency_ = Currency::Funds;
    TransactionReasons AffectReasons_ = TransactionReasons::None;
    std::string effectDescription_;
    Operator op_ = Operator::Add;
    bool registered_ = false;
};

// Effects/CurrencyConverter.cs:7
class CurrencyConverter : public StrategyEffect {
public:
    CurrencyConverter(Strategy* parent, float minRate, float maxRate, float minShare, float maxShare, Currency input,
                      Currency output, TransactionReasons affectReasons, std::string effectDescription);  // :25
    explicit CurrencyConverter(Strategy* parent) : StrategyEffect(parent) {}  // :42
    ~CurrencyConverter() override;
    const char* TypeName() const override { return "CurrencyConverter"; }
    void OnEffectQuery(CurrencyModifierQuery* qry);        // :111
    bool CanActivate(std::string& reason) override;        // :122
protected:
    void OnLoadFromConfig(const ConfigNode& node) override;  // :47
    void OnRegister() override;    // :101
    void OnUnregister() override;  // :106
private:
    float minRate_ = 0, maxRate_ = 0, minShare_ = 0, maxShare_ = 0;
    Currency input_ = Currency::Funds, output_ = Currency::Funds;
    TransactionReasons AffectReasons_ = TransactionReasons::None;
    std::string effectDescription_;
    bool registered_ = false;
};

// CurrencyExchanger.cs:8
class CurrencyExchanger : public StrategyEffect {
public:
    CurrencyExchanger(Strategy* parent, float minRate, float maxRate, float minShare, float maxShare, Currency input, Currency output);  // :24
    explicit CurrencyExchanger(Strategy* parent) : StrategyEffect(parent) {}  // :39
    const char* TypeName() const override { return "CurrencyExchanger"; }
    bool CanActivate(std::string& reason) override;  // :144
protected:
    void OnLoadFromConfig(const ConfigNode& node) override;  // :44
    void OnRegister() override;    // :105
    void OnUnregister() override;  // :140
private:
    double GetTotalInput(float shareLerp);                    // :82
    double GetTotalOutput(double input, double rateLerp);     // :100
    float minRate_ = 0, maxRate_ = 0, minShare_ = 0, maxShare_ = 0;
    Currency input_ = Currency::Funds, output_ = Currency::Funds;
    std::string effectDescription_;
};

// StrategySystem.cs:15 ([KSPScenario(96, FLIGHT, TRACKSTATION, SPACECENTER, EDITOR)])
class StrategySystem : public ScenarioModule {
public:
    StrategySystem();
    ~StrategySystem() override;
    static StrategySystem* Instance() { return instance_; }    // :25
    StrategySystemConfig* SystemConfig() { return systemConfig_.get(); }  // :27
    std::vector<std::unique_ptr<Strategy>>& Strategies() { return strategies_; }  // :29
    std::vector<Strategy*> StrategiesActive();                  // :31
    std::vector<Strategy*> StrategiesInactive();                // :50
    std::vector<std::unique_ptr<DepartmentConfig>>& Departments() { return systemConfig_->Departments(); }  // :69
    // :131 the registry of StrategyEffect subclasses (GetStrategyEffectType); the four stock effects are registered by
    // RegisterStockTypes (done in OnAwake, GenerateTypes)
    using EffectFactory = std::function<std::unique_ptr<StrategyEffect>(Strategy*)>;
    static EffectFactory GetStrategyEffectType(const std::string& typeName);
    static void RegisterEffectType(const std::string& typeName, EffectFactory factory);
    static bool HasStrategyType(const std::string& typeName);   // :94 GetStrategyType != null
    void OnAwake() override;                       // :151
    void OnLoad(ConfigNode& gameNode) override;    // :176 (the coroutine's `yield return null` is a frame delay: LoadNow runs the rest)
    void OnSave(ConfigNode& gameNode) override;    // :226
    void LoadNow(ConfigNode& gameNode);            // :181 OnLoadRoutine after its yield
    void Update();                                 // :241
    void OnDestroy();                              // :158
    bool HasActiveStrategy(const std::string& department);                // :253
    bool HasConflictingActiveStrategies(const std::vector<std::string>& groupTags);  // :273
    std::vector<Strategy*> GetStrategies(const std::string& department);  // :344
    static void Register();

private:
    void Unregister();                             // :167
    void LoadStrategies(std::vector<ConfigNode*> stratNodes);  // :196
    Enumerator OnLoadRoutine(ConfigNode::NodePtr gameNode);    // :181
    static inline StrategySystem* instance_ = nullptr;
    std::unique_ptr<StrategySystemConfig> systemConfig_;
    std::vector<std::unique_ptr<Strategy>> strategies_;
};

}  // namespace ksp::Strategies

template <>
struct ksp::dotnet::EnumTraits<ksp::Strategies::CurrencyOperation::Operator> {
    static constexpr bool flags = false;
    static constexpr EnumMember<ksp::Strategies::CurrencyOperation::Operator> members[] = {
        {ksp::Strategies::CurrencyOperation::Operator::Add, "Add"},
        {ksp::Strategies::CurrencyOperation::Operator::Multiply, "Multiply"},
        {ksp::Strategies::CurrencyOperation::Operator::Set, "Set"}};
};
