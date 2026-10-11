// WP-37: PartTestConstraint.cs and ModuleTestSubject.cs.
#include "ksp/contracts/module_test_subject.hpp"

#include <cmath>
#include <limits>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/parameters.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/modules/module_members.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/science/science.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp {

using GE = events::GameEvents;
using KSP::Localization::Localizer;

// ---- PartTestConstraint ----------------------------------------------------------------------------------------------------

// PartTestConstraint.cs:61
PartTestConstraint::PartTestConstraint(ConfigNode* node) {
    if (node != nullptr) Load(*node);
}

// PartTestConstraint.cs:69
bool PartTestConstraint::SituationAvailable(Vessel::Situations sit) const {
    if (!valid) return false;
    return (static_cast<uint32_t>(sit) & situationMask) != 0;
}

// PartTestConstraint.cs:78
bool PartTestConstraint::BodyAvailable(CelestialBody* cb) const {
    if (body == "_Any") return true;
    if (body == "_Home" && cb->isHomeWorld) return true;
    if (body == "_NotHome" && !cb->isHomeWorld) return true;
    if (body == "_Sun" && cb == Planetarium::fetch->Sun) return true;
    if (body == "_NotSun" && cb != Planetarium::fetch->Sun) return true;
    if (body == cb->bodyName) return true;
    return false;
}

// PartTestConstraint.cs:107
bool PartTestConstraint::PrestigeAvailable(ContractPrestige pres) const {
    if (prestige == "_Any") return true;
    if (pres == prestigeLevel) return true;
    return false;
}

// PartTestConstraint.cs:120
bool PartTestConstraint::Test(double input) const {
    if (!valid) return false;
    switch (test) {
        default: return false;
        case ConstraintTest::GT: return input > valueD;
        case ConstraintTest::LT: return input < valueD;
        case ConstraintTest::EQ: return input == valueD;
        case ConstraintTest::NEQ: return input != valueD;
    }
}

// PartTestConstraint.cs:141 (int input vs uint valueI: promoted to long)
bool PartTestConstraint::Test(int input) const {
    if (!valid) return false;
    const int64_t a = input;
    const int64_t b = valueI;
    switch (test) {
        default: return false;
        case ConstraintTest::GT: return a > b;
        case ConstraintTest::LT: return a < b;
        case ConstraintTest::EQ: return a == b;
        case ConstraintTest::NEQ: return a != b;
    }
}

// PartTestConstraint.cs:162
bool PartTestConstraint::Test(bool input) const {
    if (!valid) return false;
    if (test == ConstraintTest::EQ) return input == valueB;
    return input != valueB;
}

// PartTestConstraint.cs:175
bool PartTestConstraint::Test(const std::string& input) const {
    if (!valid) return false;
    if (test == ConstraintTest::EQ) return input == value;
    return input != value;
}

// PartTestConstraint.cs:188
bool PartTestConstraint::Test(uint32_t sitmask) const {
    if (!valid) return false;
    if (test == ConstraintTest::EQ) return (sitmask & valueI) != 0;
    return (sitmask & valueI) == 0;
}

// PartTestConstraint.cs:201
void PartTestConstraint::Load(ConfigNode& node) {
    valid = true;
    if (node.HasValue("type")) {
        try {
            type = dotnet::EnumParseFull<ConstraintType>(*node.GetValue("type"));
        } catch (...) {
            log::Error("[PartTestConstraint]: Error parsing type " + *node.GetValue("type") + " in constraint for ModuleTestSubject.");
            valid = false;
        }
    } else {
        valid = false;
    }
    if (node.HasValue("test")) {
        try {
            test = dotnet::EnumParseFull<ConstraintTest>(*node.GetValue("test"));
        } catch (...) {
            log::Error("[PartTestConstraint]: Error parsing type " + node.GetValue("type").value_or(std::string()) + " in constraint for ModuleTestSubject.");
            valid = false;
        }
    }
    if (node.HasValue("value")) {
        value = *node.GetValue("value");
        double result = 0.0;
        if (dotnet::TryParseDouble(value, result)) valueD = result;
        const bool flag = dotnet::TryParseBool(value, valueB);
        uint32_t result2 = 0;
        if (dotnet::TryParseUInt32(value, result2)) valueI = result2;
        if (type != ConstraintType::ALTITUDE && type != ConstraintType::DENSITY && type != ConstraintType::DYNAMICPRESSURE && type != ConstraintType::SPEED) {
            if (type != ConstraintType::ATMOSPHERE && type != ConstraintType::OXYGEN) {
                if (type == ConstraintType::REPEATABILITY) {
                    try {
                        valueR = dotnet::EnumParseFull<TestRepeatability>(value);
                    } catch (...) {
                        log::Error("[PartTestConstraint]: Error parsing value of type repeatability, value =  " + value + " in constraint for ModuleTestSubject.");
                        valid = false;
                    }
                }
            } else if (!flag) {
                valid = false;
            }
        } else if (std::isnan(valueD)) {
            valid = false;
        }
    }
    if (node.HasValue("body")) body = *node.GetValue("body");
    uint32_t result3 = 0;
    if (node.HasValue("situationMask") && dotnet::TryParseUInt32(*node.GetValue("situationMask"), result3)) situationMask = result3;
    if (!node.HasValue("prestige")) return;
    prestige = *node.GetValue("prestige");
    if (prestige != "_Any") {
        try {
            prestigeLevel = dotnet::EnumParseFull<ContractPrestige>(prestige);
        } catch (...) {
            log::Error("[PartTestConstraint]: Error parsing prestige level " + prestige + " in constraint for ModuleTestSubject.");
            valid = false;
        }
    }
}

// PartTestConstraint.cs:302
bool PartTestConstraint::SituationAvailable(const std::vector<PartTestConstraint>& constraints, Vessel::Situations sit) {
    size_t num = 0;
    const size_t count = constraints.size();
    while (true) {
        if (num < count) {
            const PartTestConstraint& c = constraints[num];
            if (c.type == ConstraintType::SITUATION && !c.Test(static_cast<uint32_t>(sit))) break;
            ++num;
            continue;
        }
        return true;
    }
    return false;
}

// ---- ModuleTestSubject ------------------------------------------------------------------------------------------------------

namespace {
int NextTestOwner() {
    static int next = 9000;
    return ++next;
}
}  // namespace

ModuleTestSubject::~ModuleTestSubject() {
    CareerEnvironment::Current().Scheduler().StopAllCoroutines(coroutineOwner_);
    OnDestroy();
}

// ModuleTestSubject.cs:44
std::string ModuleTestSubject::GetTestNotes() {
    const std::string notes = TestNotes();
    if (!notes.empty()) return notes;
    if (!cacheLoaded_) CacheLocalStrings();
    if (useStaging()) return cacheAutoLOC_231004;
    if (useEvent()) return cacheAutoLOC_231008;
    return std::string();
}

bool ModuleTestSubject::HasTestNotes() {
    if (!TestNotes().empty()) return true;
    return useStaging() || useEvent();
}

// ModuleTestSubject.cs:61
void ModuleTestSubject::OnAwake() {
    if (coroutineOwner_ == 0) coroutineOwner_ = NextTestOwner();
    if (!constraintsAssigned) {
        constraints.clear();
        constraintsAssigned = true;
    }
    Events().Bind("RunTestEvent", BaseEvent::Handler([this] { RunTestEvent(); }));
}

// ModuleTestSubject.cs:69
void ModuleTestSubject::OnDestroy() {
    if (isTestSubject && subscribed_) {
        GE::onPartDie.Remove(this, &ModuleTestSubject::onPartDie);
        GE::Contract::onParameterChange.Remove(this, &ModuleTestSubject::OnContractStateChange);
        subscribed_ = false;
    }
}

// ModuleTestSubject.cs:78
void ModuleTestSubject::OnLoad(const ConfigNode& node) {
    if (!constraintsAssigned) {
        constraints.clear();
        constraintsAssigned = true;
    }
    if (node.HasNode("CONSTRAINT")) constraints.clear();
    std::vector<ConfigNode*> nodes = node.GetNodes("CONSTRAINT");
    for (size_t i = 0; i < nodes.size(); ++i) {
        PartTestConstraint partTestConstraint(nodes[i]);
        if (partTestConstraint.valid) constraints.push_back(partTestConstraint);
    }
}

// Unity Instantiate: isTestSubject / isExperimentalPart / constraints are public (serialized)
void ModuleTestSubject::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* r = dynamic_cast<const ModuleTestSubject*>(&from)) {
        isTestSubject = r->isTestSubject;
        isExperimentalPart = r->isExperimentalPart;
        constraints = r->constraints;
        constraintsAssigned = r->constraintsAssigned;
    }
}

// ModuleTestSubject.cs:101
void ModuleTestSubject::OnStart(StartState) {
    Events()["RunTestEvent"]->active = false;
    CareerEnvironment::Current().Scheduler().StartCoroutine(delayedStart(5), coroutineOwner_);
}

// ModuleTestSubject.cs:107
Enumerator ModuleTestSubject::delayedStart(int frameDelay) {
    for (int i = 0; i < frameDelay; ++i) co_yield YieldInstruction::Null();
    Part* p = dynamic_cast<Part*>(part);
    {
        const Contracts::ContractsEnvironment& env = Contracts::ContractsEnvironment::Current();
        isExperimentalPart = !(env.partModelPurchased ? env.partModelPurchased(p->partInfo->name) : ResearchAndDevelopment::PartModelPurchased(p->partInfo));
    }
    if (SaveEnvironment::Current().loadedScene != GameScenes::FLIGHT || Contracts::ContractSystem::Instance == nullptr) co_return;
    std::vector<Contracts::Templates::PartTest*> currentActiveContracts = Contracts::ContractSystem::Instance->GetCurrentActiveContracts<Contracts::Templates::PartTest>();
    int num = static_cast<int>(currentActiveContracts.size());
    Contracts::Templates::PartTest* partTest = nullptr;
    do {
        if (num-- > 0) {
            partTest = currentActiveContracts[static_cast<size_t>(num)];
            continue;
        }
        co_return;
    } while (partTest->PartName != p->partInfo->title);
    isTestSubject = true;
    if (useEvent() && !partTest->Hauled()) Events()["RunTestEvent"]->active = true;
    GE::onPartDie.Add(this, &ModuleTestSubject::onPartDie);
    GE::Contract::onParameterChange.Add(this, &ModuleTestSubject::OnContractStateChange);
    subscribed_ = true;
}

// ModuleTestSubject.cs:140
void ModuleTestSubject::onPartDie(Part* p) {
    if (p == dynamic_cast<Part*>(part)) onTestSujectDestroyed.Fire(this);
}

// ModuleTestSubject.cs:148
void ModuleTestSubject::RunTest() { onTestRun.Fire(this); }

// ModuleTestSubject.cs:153
void ModuleTestSubject::OnActive() {
    Part* p = dynamic_cast<Part*>(part);
    if (useStaging() && p->fields.stagingOn) RunTest();
}

// ModuleTestSubject.cs:162
void ModuleTestSubject::RunTestEvent() {
    RunTest();
    Part* p = dynamic_cast<Part*>(part);
    if (p->hasStagingIcon() && p->State() == PartStates::IDLE) p->force_activate();
}

// ModuleTestSubject.cs:171
void ModuleTestSubject::OnContractStateChange(Contracts::Contract* c, Contracts::ContractParameter* p) {
    if (c != nullptr && p != nullptr && p->State() == Contracts::ParameterState::Complete) {
        auto* partTest = dynamic_cast<Contracts::Templates::PartTest*>(c);
        Part* thePart = dynamic_cast<Part*>(part);
        if (partTest != nullptr && partTest->PartName == thePart->partInfo->title && dynamic_cast<Contracts::Parameters::PartTest*>(p) != nullptr)
            Events()["RunTestEvent"]->active = false;
    }
}

// ModuleTestSubject.cs:179
bool ModuleTestSubject::SituationAvailable(Vessel::Situations sit) { return (static_cast<uint32_t>(sit) & situationMask()) != 0; }

// ModuleTestSubject.cs:184 (sic: returns the cache when it is null, the localized text otherwise is inverted)
std::string ModuleTestSubject::GetModuleDisplayName() {
    if (!cacheAutoLOC_6002280.empty()) return Localizer::Format("#autoLOC_6002280");
    return cacheAutoLOC_6002280;
}

// ModuleTestSubject.cs:193
void ModuleTestSubject::CacheLocalStrings() {
    cacheAutoLOC_231004 = Localizer::Format("#autoLOC_231004");
    cacheAutoLOC_231008 = Localizer::Format("#autoLOC_231008");
    cacheAutoLOC_6002280 = Localizer::Format("#autoLOC_6002280");
    cacheLoaded_ = true;
}

namespace detail {
void RegisterContractModules() {
    ModuleFactory::Register("ModuleTestSubject", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleTestSubject>(c)); });
}
}  // namespace detail

}  // namespace ksp
