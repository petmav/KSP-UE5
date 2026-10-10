// WP-37: PartTestConstraint.cs and ModuleTestSubject.cs (Assembly-CSharp root): the per-part test conditions of the PartTest contracts and
// the PartModule that marks a part as a test subject (situation mask, staging / event triggered, the CONSTRAINT nodes).
//
// ModuleTestSubject was a schema-only stand-in before this package (module_schema.inc fields, IsStageable = false in module_staging.cpp);
// it is now registered through detail::RegisterContractModules() (module_behaviours.cpp). Seams: ResearchAndDevelopment.PartModelPurchased
// (ContractsEnvironment::partModelPurchased), the Part Action Window events (the RunTestEvent is bound in OnAwake), the 5 frame
// delayedStart coroutine runs on the CareerEnvironment scheduler.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/contracts/contract.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/events/event_data.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

class CelestialBody;
class Part;

// PartTestConstraint.cs:6
class PartTestConstraint {
public:
    enum class ConstraintType : int { SPEED, SPEEDENV, ALTITUDE, ALTITUDEENV, DENSITY, DYNAMICPRESSURE, OXYGEN, ATMOSPHERE, SITUATION, REPEATABILITY };  // :8
    enum class ConstraintTest : int { GT, LT, EQ, NEQ };                                  // :22
    enum class TestRepeatability : int { ALWAYS, BODYANDSITUATION, ONCEPERPART };          // :30

    ConstraintType type = ConstraintType::SPEED;                                          // :37
    ConstraintTest test = ConstraintTest::EQ;                                             // :39
    std::string body = "_Any";                                                            // :41
    std::string prestige = "_Any";                                                        // :43
    ContractPrestige prestigeLevel = ContractPrestige::Trivial;                           // :45
    std::string value = "";                                                               // :47
    double valueD = std::numeric_limits<double>::quiet_NaN();                              // (C++ addition)
    bool valueB = false;                                                                  // :51
    uint32_t valueI = 0;                                                                  // :53
    TestRepeatability valueR = TestRepeatability::ALWAYS;                                 // :55
    uint32_t situationMask = 0xFFFFFFFFu;                                                 // :57
    bool valid = false;                                                                   // :59

    explicit PartTestConstraint(ConfigNode* node = nullptr);                              // :61
    bool SituationAvailable(Vessel::Situations sit) const;                                // :69
    bool BodyAvailable(CelestialBody* cb) const;                                          // :78
    bool PrestigeAvailable(ContractPrestige pres) const;                                  // :107
    bool Test(double input) const;                                                        // :120
    bool Test(int input) const;                                                           // :141
    bool Test(bool input) const;                                                          // :162
    bool Test(const std::string& input) const;                                            // :175
    bool Test(uint32_t sitmask) const;                                                    // :188
    void Load(ConfigNode& node);                                                          // :201
    void Save(ConfigNode& node) {(void)node;}                                             // :298
    static bool SituationAvailable(const std::vector<PartTestConstraint>& constraints, Vessel::Situations sit);  // :302
};

// ModuleTestSubject.cs:8
class ModuleTestSubject : public PartModule {
public:
    explicit ModuleTestSubject(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleTestSubject() override;

    uint32_t situationMask() const { return Get<uint32_t>("situationMask"); }                   // :11
    bool useProgressForBodies() const { return Get<bool>("useProgressForBodies"); }             // :14
    bool usePrestigeForSit() const { return Get<bool>("usePrestigeForSit"); }                   // :17
    bool useStaging() const { return Get<bool>("useStaging"); }                                 // :20
    bool useEvent() const { return Get<bool>("useEvent"); }                                     // :23
    std::string TestNotes() const { return IsNull("TestNotes") ? std::string() : Get<std::string>("TestNotes"); }  // :26
    bool isTestSubject = false;                                                                 // :28
    bool isExperimentalPart = false;                                                            // :30
    std::vector<PartTestConstraint> constraints;                                                // :32
    bool constraintsAssigned = false;                                                           // `constraints != null`
    static inline events::EventData<ModuleTestSubject*> onTestRun{"onPartTestRun"};             // :34
    static inline events::EventData<ModuleTestSubject*> onTestSujectDestroyed{"onTestSujectDestroyed"};  // :36

    std::string GetTestNotes();                           // :44 (empty = null)
    bool HasTestNotes();
    void OnAwake() override;                              // :61
    void OnDestroy();                                     // :69
    void OnLoad(const ConfigNode& node) override;         // :78
    void OnStart(StartState state) override;              // :101
    void CopySerializedFrom(const PartModule& from) override;
    Enumerator delayedStart(int frameDelay);              // :107
    void onPartDie(Part* p);                              // :140
    void RunTest();                                       // :148
    void OnActive() override;                             // :153
    void RunTestEvent();                                  // :162 [KSPEvent]
    void OnContractStateChange(Contracts::Contract* c, Contracts::ContractParameter* p);  // :171
    bool SituationAvailable(Vessel::Situations sit);      // :179
    std::string GetModuleDisplayName() override;          // :184
    static void CacheLocalStrings();                      // :193
    bool IsStageable() override { return false; }         // :200

    int CoroutineOwner() const { return coroutineOwner_; }

private:
    int coroutineOwner_ = 0;
    bool subscribed_ = false;
    static inline std::string cacheAutoLOC_231004, cacheAutoLOC_231008, cacheAutoLOC_6002280;
    static inline bool cacheLoaded_ = false;
};

namespace detail {
void RegisterContractModules();  // module_test_subject.cpp (called by RegisterBuiltinModules)
}

}  // namespace ksp

template <>
struct ksp::dotnet::EnumTraits<ksp::PartTestConstraint::ConstraintType> {
    using E = ksp::PartTestConstraint::ConstraintType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::SPEED, "SPEED"},   {E::SPEEDENV, "SPEEDENV"},     {E::ALTITUDE, "ALTITUDE"},
                                               {E::ALTITUDEENV, "ALTITUDEENV"}, {E::DENSITY, "DENSITY"}, {E::DYNAMICPRESSURE, "DYNAMICPRESSURE"},
                                               {E::OXYGEN, "OXYGEN"}, {E::ATMOSPHERE, "ATMOSPHERE"}, {E::SITUATION, "SITUATION"},
                                               {E::REPEATABILITY, "REPEATABILITY"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::PartTestConstraint::ConstraintTest> {
    using E = ksp::PartTestConstraint::ConstraintTest;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::GT, "GT"}, {E::LT, "LT"}, {E::EQ, "EQ"}, {E::NEQ, "NEQ"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::PartTestConstraint::TestRepeatability> {
    using E = ksp::PartTestConstraint::TestRepeatability;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::ALWAYS, "ALWAYS"}, {E::BODYANDSITUATION, "BODYANDSITUATION"}, {E::ONCEPERPART, "ONCEPERPART"}};
};
