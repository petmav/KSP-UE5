// WP-54: Expansions/Missions/{ScoreModule,ScoreModule_Completion,ScoreModule_Modifier,ScoreModule_Time,ScoreModule_Accuracy,
// ScoreModule_Resource,ScoreRange,ScoreInfo,MissionScore}.cs. The score arithmetic is `float` managed code: Mono float rule
// (fx()/f32store()).
//
// MEGUI_* attributes (editor widgets), GetNodeBodyParameterString (editor reflection) and MissionCheckpointValidator.CompareObjectLists
// based Equals are the editor's; Equals is ported (it is used by DynamicModuleList.Equals), the widget text is not.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/missions/dynamic_module.hpp"
#include "ksp/missions/mission_node.hpp"

namespace ksp::Expansions::Missions {

class Mission;

// ScoreRange.cs:6
class ScoreRange {
public:
    ScoreRange() = default;                                  // :16
    double minRange = 0.0;                                   // :8
    double maxRange = 1.0;                                   // :10
    float score = 0.0f;                                      // :12
    bool isValueInRange(double value) const;                 // :23
    bool Equals(const ScoreRange& other) const;              // :30
    int32_t GetHashCode() const;                             // :42
    void Load(const ConfigNode& node);                       // :46
    void Save(ConfigNode& node) const;                       // :53
};

// ScoreInfo.cs:6
class ScoreInfo {
public:
    ScoreInfo();                                             // :19
    Guid id;                                                 // :8
    Guid missionId;                                          // :10
    float score = 0.0f;                                      // :12
    double timeTaken = 0.0;                                  // :14
    std::vector<std::string> awardIds;                       // :16
    std::string resultsText;                                 // :18
    std::optional<int64_t> completedAt;                      // :20 (DateTime? as the FILETIME ticks ToFileTimeUtc / FromFileTimeUtc round-trips)
    void Load(const ConfigNode& node);                       // :28
    void Save(ConfigNode& node) const;                       // :43
};

// ScoreModule.cs:6
class ScoreModule : public DynamicModule {
public:
    explicit ScoreModule(std::string typeName) : DynamicModule(std::move(typeName)) {}             // :23
    ScoreModule(std::string typeName, MENode* node) : DynamicModule(std::move(typeName), node) {}  // :27
    virtual float AwardScore(float currentScore);            // :32
    virtual std::string ScoreDescription();                  // :37
    std::string AwarededScoreDescription();                  // :42
    void Load(const ConfigNode& node) override;              // :52
    void Save(ConfigNode& node) const override;              // :58

protected:
    std::string awardedScoreDescription;                     // :8
    IScoreableObjective* scoreableObjective();               // :10 (the first test module of the dock parent, when it is scoreable)
};

// ScoreModule_Completion.cs:8
class ScoreModule_Completion : public DynamicModuleT<ScoreModule_Completion, ScoreModule> {
public:
    static constexpr const char* kTypeName = "ScoreModule_Completion";
    ScoreModule_Completion() : DynamicModuleT(kTypeName) {}                  // :13
    explicit ScoreModule_Completion(MENode* node) : DynamicModuleT(kTypeName, node) {}  // :18
    float bonusScore = 0.0f;                                                  // :11
    std::string GetDisplayName() override;                                    // :24
    std::string GetDisplayToolTip() override;                                 // :29
    float AwardScore(float currentScore) override;                            // :34
    std::string ScoreDescription() override;                                  // :41
    std::vector<std::string> GetDefaultPinnedParameters() override;           // :46
    bool EqualsObject(const DynamicModule& other) const override;             // :51
    std::string GetInfo() override;                                           // :65
    void Load(const ConfigNode& node) override;                               // :70
    void Save(ConfigNode& node) const override;                               // :76
};

// ScoreModule_Modifier.cs:8
class ScoreModule_Modifier : public DynamicModuleT<ScoreModule_Modifier, ScoreModule> {
public:
    static constexpr const char* kTypeName = "ScoreModule_Modifier";
    ScoreModule_Modifier() : DynamicModuleT(kTypeName) {}                    // :16
    explicit ScoreModule_Modifier(MENode* node) : DynamicModuleT(kTypeName, node) {}  // :22
    ScoreModifierType modifierType = ScoreModifierType::Multiply;            // :11
    float modifierScore = 0.0f;                                               // :14
    std::string GetDisplayName() override;                                    // :29
    std::string GetDisplayToolTip() override;                                 // :34
    float AwardScore(float currentScore) override;                            // :39
    std::string ScoreDescription() override;                                  // :67
    std::vector<std::string> GetDefaultPinnedParameters() override;           // :82
    bool EqualsObject(const DynamicModule& other) const override;             // :87
    std::string GetInfo() override;                                           // :101
    void Load(const ConfigNode& node) override;                               // :106
    void Save(ConfigNode& node) const override;                               // :113
};

// ScoreModule_Time.cs:8
class ScoreModule_Time : public DynamicModuleT<ScoreModule_Time, ScoreModule> {
public:
    static constexpr const char* kTypeName = "ScoreModule_Time";
    ScoreModule_Time() : DynamicModuleT(kTypeName) {}                       // :15
    explicit ScoreModule_Time(MENode* node) : DynamicModuleT(kTypeName, node) {}  // :21
    std::vector<ScoreRange> scoreRanges;                                      // :11
    std::string GetDisplayName() override;                                    // :28
    float AwardScore(float currentScore) override;                            // :33
    std::string ScoreDescription() override;                                  // :55
    std::vector<std::string> GetDefaultPinnedParameters() override;           // :67
    bool EqualsObject(const DynamicModule& other) const override;             // :72
    std::string GetInfo() override;                                           // :86
    void Load(const ConfigNode& node) override;                               // :91
    void Save(ConfigNode& node) const override;                               // :107
};

// ScoreModule_Accuracy.cs:8
class ScoreModule_Accuracy : public DynamicModuleT<ScoreModule_Accuracy, ScoreModule> {
public:
    static constexpr const char* kTypeName = "ScoreModule_Accuracy";
    ScoreModule_Accuracy() : DynamicModuleT(kTypeName) {}                   // :13
    explicit ScoreModule_Accuracy(MENode* node) : DynamicModuleT(kTypeName, node) {}  // :18
    std::vector<ScoreRange> scoreRanges;                                      // :11
    std::string GetDisplayName() override;                                    // :24
    float AwardScore(float currentScore) override;                            // :29
    std::string ScoreDescription() override;                                  // :54
    std::vector<std::string> GetDefaultPinnedParameters() override;           // :66
    bool EqualsObject(const DynamicModule& other) const override;             // :71
    std::string GetInfo() override;                                           // :85
    void Load(const ConfigNode& node) override;                               // :90
    void Save(ConfigNode& node) const override;                               // :106
};

// ScoreModule_Resource.cs:8
class ScoreModule_Resource : public DynamicModuleT<ScoreModule_Resource, ScoreModule> {
public:
    static constexpr const char* kTypeName = "ScoreModule_Resource";
    ScoreModule_Resource() : DynamicModuleT(kTypeName) {}                   // :16
    explicit ScoreModule_Resource(MENode* node) : DynamicModuleT(kTypeName, node) {}  // :21
    std::string resourceName = "LiquidFuel";                                  // :11
    std::vector<ScoreRange> scoreRanges;                                      // :14
    std::string GetDisplayName() override;                                    // :27
    float AwardScore(float currentScore) override;                            // :32
    std::string ScoreDescription() override;                                  // :64
    std::vector<std::string> GetDefaultPinnedParameters() override;           // :86
    bool EqualsObject(const DynamicModule& other) const override;             // :91
    std::string GetInfo() override;                                           // :105
    void Load(const ConfigNode& node) override;                               // :110
    void Save(ConfigNode& node) const override;                               // :126
};

// MissionScore.cs:8
class MissionScore : public DynamicModuleList {
public:
    explicit MissionScore(MENode* node);                                       // :27
    // :10 `public static List<Type> globalScoreModules { get; private set; }`: null until SetDefaultGlobalScoreModules
    static const std::vector<std::string>& globalScoreModules();
    // :69 AddGlobalScoreModule(Type): the type must derive from ScoreModule and not be registered yet
    static void AddGlobalScoreModule(const std::string& scoreModuleTypeName);
    static void ResetGlobalScoreModules();                                     // tests: back to "null"
    IScoreableObjective* scoreableObjective();                                 // :12
    void AwardScore(Mission* mission);                                         // :43
    std::string ScoreDescription();                                            // :53
    std::string AwarededScoreDescription();                                    // :64
    std::vector<std::string> GetSupportedTypes() override;                     // :98
    std::unique_ptr<DynamicModuleList> Clone() const override;
    // the custom attribute [MEScoreModule(typeof(...))] of a scoreable objective (MEScoreModule.cs): objective type name -> allowed systems
    static inline std::function<std::vector<std::string>(IScoreableObjective*)> allowedSystemsOf;
};

// Registers the six score modules and the awards types with the registries (call once; idempotent).
void RegisterScoreModules();

}  // namespace ksp::Expansions::Missions
