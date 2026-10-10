// WP-37: Contracts.ContractParameter and Contracts.ContractPredicate (Assembly-CSharp/Contracts/ContractParameter.cs, ContractPredicate.cs).
#include <algorithm>
#include <cmath>

#include "ksp/career/career.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/contracts/contract.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/misc_util.hpp"

namespace ksp::Contracts {

using GE = events::GameEvents;
using KSP::Localization::Localizer;

namespace {
std::string ToStr(double d) { return dotnet::FormatDoubleG(d); }
std::string ToStr(float f) { return dotnet::FormatSingleG(f); }
std::vector<std::string> SplitCommaSpace(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ',' || c == ' ') {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}
bool ParseBool(const std::string& s) {
    bool b = false;
    if (!dotnet::TryParseBool(s, b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
    return b;
}
void SendParameterMessage(ContractParameter& p, const std::string& title, const std::string& message) {
    ContractsEnvironment& env = ContractsEnvironment::Current();
    const std::string text = "<b>" + p.Root()->Title() + "</b>\n\n" + message;
    if (env.postMessage)
        env.postMessage(title, text);
    else if (CareerEnvironment::Current().postMessage)
        CareerEnvironment::Current().postMessage(title, text);
    log::Info("Contract (" + p.Root()->Title() + "): " + message);
}
}  // namespace

// ---- ContractParameter ------------------------------------------------------------------------------------------------

ContractParameter::ContractParameter() = default;
ContractParameter::~ContractParameter() = default;

// C++ helper (see ContractParameter.cs:105)
void ContractParameter::SetEnabled(bool value) {
    if (value)
        Enable();
    else
        Disable();
}

// ContractParameter.cs:130
std::string ContractParameter::HashString() {
    std::string text;
    const std::string hashString = GetHashString();
    text += hashString;
    for (size_t i = 0; i < parameters_.size(); ++i) text += parameters_[i]->HashString();
    return text;
}

// ContractParameter.cs:159
void ContractParameter::AllParameters(std::vector<ContractParameter*>& out) {
    out.push_back(this);
    for (auto& p : parameters_) p->AllParameters(out);
}

// ContractParameter.cs:179
void ContractParameter::NestToParent(IContractParameterHost* parent) {
    parent_ = parent;
    root_ = parent->Root();
}

// ContractParameter.cs:185
void ContractParameter::Enable() {
    if (!enabled) {
        enabled = true;
        Register();
    }
}

// ContractParameter.cs:194
void ContractParameter::Disable(bool recursive) {
    if (!enabled) return;
    enabled = false;
    Unregister();
    if (recursive) {
        int count = static_cast<int>(parameters_.size());
        while (count-- > 0) parameters_[static_cast<size_t>(count)]->Disable(true);
    }
}

// ContractParameter.cs:212
void ContractParameter::ParameterStateUpdate(ContractParameter* p) {
    GE::Contract::onParameterChange.Fire(root_, p);
    Parent()->ParameterStateUpdate(p);
    OnParameterStateChange(p);
}

// ContractParameter.cs:219
ContractParameter* ContractParameter::AddParameter(std::unique_ptr<ContractParameter> parameter, const std::string& pid) {
    parameter->NestToParent(this);
    if (!pid.empty()) parameter->id = pid;
    parameters_.push_back(std::move(parameter));
    return parameters_.back().get();
}

// ContractParameter.cs:230
ContractParameter* ContractParameter::GetParameter(int index) { return parameters_.at(static_cast<size_t>(index)).get(); }

// ContractParameter.cs:230
ContractParameter* ContractParameter::GetParameter(const std::string& pid) {
    for (auto& p : parameters_)
        if (p->ID() == pid) return p.get();
    return nullptr;
}

// C++ helper (see ContractParameter.cs:247)
ContractParameter* ContractParameter::GetParameterOfType(const std::string& typeName) {
    for (auto& p : parameters_)
        if (p->TypeName() == typeName) return p.get();
    return nullptr;
}

// ContractParameter.cs:295
void ContractParameter::RemoveParameter(int index) { parameters_.erase(parameters_.begin() + index); }

// ContractParameter.cs:295
void ContractParameter::RemoveParameter(const std::string& pid) {
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (parameters_[i]->ID() == pid) {
            parameters_.erase(parameters_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }
}

// C++ helper (see ContractParameter.cs:312)
void ContractParameter::RemoveParameterOfType(const std::string& typeName) {
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (parameters_[i]->TypeName() == typeName) {
            parameters_.erase(parameters_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }
}

// ContractParameter.cs:340
void ContractParameter::RemoveParameter(ContractParameter* parameter) {
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (parameters_[i].get() == parameter) {
            parameters_.erase(parameters_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }
}

// ContractParameter.cs:349
bool ContractParameter::AllChildParametersComplete() {
    int count = static_cast<int>(parameters_.size());
    do {
        if (count-- <= 0) return true;
    } while (parameters_[static_cast<size_t>(count)]->State() == ParameterState::Complete);
    return false;
}

// ContractParameter.cs:363
bool ContractParameter::AnyChildParametersFailed() {
    int count = static_cast<int>(parameters_.size());
    do {
        if (count-- <= 0) return false;
    } while (parameters_[static_cast<size_t>(count)]->State() != ParameterState::Failed);
    return true;
}

// ContractParameter.cs:377
void ContractParameter::SetFunds(float completion, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    FundsCompletion = static_cast<double>(Mathf::Round(f32store(fx(completion) * fx(GameVariables::Instance->GetContractFundsCompletionFactor(Root()->Prestige())) * fx(num))));
}

// ContractParameter.cs:377
void ContractParameter::SetFunds(float completion, float failure, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    GameVariables& gv = *GameVariables::Instance;
    FundsCompletion = static_cast<double>(Mathf::Round(f32store(fx(completion) * fx(gv.GetContractFundsCompletionFactor(Root()->Prestige())) * fx(num))));
    FundsFailure = static_cast<double>(Mathf::Round(f32store(fx(failure) * fx(gv.GetContractFundsFailureFactor(Root()->Prestige())) * fx(num))));
}

// ContractParameter.cs:398
void ContractParameter::SetReputation(float completion, float failure, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    GameVariables& gv = *GameVariables::Instance;
    ReputationCompletion = Mathf::Round(f32store(fx(completion) * fx(gv.GetContractReputationCompletionFactor(Root()->Prestige())) * fx(num)));
    ReputationFailure = Mathf::Round(f32store(fx(failure) * fx(gv.GetContractReputationFailureFactor(Root()->Prestige())) * fx(num)));
}

// ContractParameter.cs:398
void ContractParameter::SetReputation(float completion, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    ReputationCompletion = Mathf::Round(f32store(fx(completion) * fx(GameVariables::Instance->GetContractReputationCompletionFactor(Root()->Prestige())) * fx(num)));
}

// ContractParameter.cs:419
void ContractParameter::SetScience(float completion, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    ScienceCompletion = Mathf::Round(f32store(fx(completion) * fx(GameVariables::Instance->GetContractScienceCompletionFactor(Root()->Prestige())) * fx(num)));
}

// ContractParameter.cs:429
float ContractParameter::GetDestinationWeight(CelestialBody* body) { return GameVariables::Instance->GetContractDestinationWeight(body); }

// ContractParameter.cs:434
void ContractParameter::AwardCompletion() {
    bool any = false;  // string.IsNullOrEmpty(text) of the C# (the sprite markup itself is mission control GUI text)
    CareerEnvironment& career = CareerEnvironment::Current();
    CurrencyModifierQuery q = CurrencyModifierQuery::RunQuery(TransactionReasons::ContractReward, static_cast<float>(FundsCompletion), ScienceCompletion, ReputationCompletion);
    if (FundsCompletion != 0.0) {
        any = true;
        if (Funding::Instance == nullptr) throw NullReferenceException("Object reference not set to an instance of an object");
        Funding::Instance->AddFunds(FundsCompletion, TransactionReasons::ContractReward);
        FundsCompletion = 0.0;
    } else if (q.GetEffectDelta(Currency::Funds) != 0.0f) {
        any = true;
    }
    if (fx(ScienceCompletion) + fx(q.GetEffectDelta(Currency::Science)) != 0.0) {
        any = true;
        if (!(career.rndPresent && career.rndPresent())) throw NullReferenceException("Object reference not set to an instance of an object");
        if (career.rndAddScience) career.rndAddScience(ScienceCompletion, TransactionReasons::ContractReward);
        ScienceCompletion = 0.0f;
    } else if (q.GetEffectDelta(Currency::Science) != 0.0f) {
        any = true;
    }
    if (fx(ReputationCompletion) + fx(q.GetEffectDelta(Currency::Reputation)) != 0.0) {
        any = true;
        if (Reputation::Instance == nullptr) throw NullReferenceException("Object reference not set to an instance of an object");
        Reputation::Instance->AddReputation(ReputationCompletion, TransactionReasons::ContractReward);
        ReputationCompletion = 0.0f;
    } else if (q.GetEffectDelta(Currency::Reputation) != 0.0f) {
        any = true;
    }
    if (any) SendParameterMessage(*this, Localizer::Format("#autoLOC_268165"), MessageComplete() + "\n\n");
}

// ContractParameter.cs:475
void ContractParameter::PenalizeFailure() {
    bool any = false;
    CurrencyModifierQuery q = CurrencyModifierQuery::RunQuery(TransactionReasons::ContractPenalty, static_cast<float>(FundsFailure), 0.0f, ReputationFailure);
    if (FundsFailure + static_cast<double>(q.GetEffectDelta(Currency::Funds)) != 0.0) {
        any = true;
        if (Funding::Instance == nullptr) throw NullReferenceException("Object reference not set to an instance of an object");
        Funding::Instance->AddFunds(0.0 - dotnet::Math::Abs(FundsFailure), TransactionReasons::ContractPenalty);
        FundsFailure = 0.0;
    } else if (q.GetEffectDelta(Currency::Funds) != 0.0f) {
        any = true;
    }
    if (q.GetEffectDelta(Currency::Science) != 0.0f) any = true;
    if (fx(ReputationFailure) + fx(q.GetEffectDelta(Currency::Reputation)) != 0.0) {
        any = true;
        if (Reputation::Instance == nullptr) throw NullReferenceException("Object reference not set to an instance of an object");
        Reputation::Instance->AddReputation(f32store(0.0 - fx(Mathf::Abs(ReputationFailure))), TransactionReasons::ContractPenalty);
        ReputationFailure = 0.0f;
    } else if (q.GetEffectDelta(Currency::Reputation) != 0.0f) {
        any = true;
    }
    // if (string.IsNullOrEmpty(text)) text = MessageFailed; the text is then never empty, so the message is always sent
    if (!any)
        SendParameterMessage(*this, Localizer::Format("#autoLOC_268204"), MessageFailed() + "\n\n" + MessageFailed());
    else
        SendParameterMessage(*this, Localizer::Format("#autoLOC_268204"), MessageFailed() + "\n\n");
}

// ContractParameter.cs:513
void ContractParameter::Load(ConfigNode& node) {
    const std::optional<std::string> value = node.GetValue("state");
    if (value.has_value()) state = dotnet::EnumParseFull<ParameterState>(*value);
    if (node.HasValue("enabled")) enabled = ParseBool(*node.GetValue("enabled"));
    if (node.HasValue("optional")) optional = ParseBool(*node.GetValue("optional"));
    if (node.HasValue("id")) id = *node.GetValue("id");
    if (node.HasValue("disableOnStateChange")) disableOnStateChange = ParseBool(*node.GetValue("disableOnStateChange"));
    if (node.HasValue("allowPartialFailure")) allowPartialFailure = ParseBool(*node.GetValue("allowPartialFailure"));
    if (node.HasValue("values")) {
        const std::vector<std::string> array = SplitCommaSpace(*node.GetValue("values"));
        if (array.size() == 5) {
            FundsCompletion = dotnet::ParseDouble(array[0]);
            FundsFailure = dotnet::ParseDouble(array[1]);
            ScienceCompletion = dotnet::ParseSingle(array[2]);
            ReputationCompletion = dotnet::ParseSingle(array[3]);
            ReputationFailure = dotnet::ParseSingle(array[4]);
        }
    }
    OnLoad(node);
    std::vector<ConfigNode*> nodes = node.GetNodes("PARAM");
    for (size_t i = 0; i < nodes.size(); ++i) {
        ConfigNode* configNode = nodes[i];
        const std::optional<std::string> value2 = configNode->GetValue("name");
        if (!value2.has_value()) {
            log::Error("ContractParameter: Contract parameter config is invalid");
            continue;
        }
        std::unique_ptr<ContractParameter> contractParameter = ContractSystem::CreateParameter(*value2);
        if (!contractParameter) {
            log::Error("ContractParameter: Contract parameter type name '" + *value2 + "' is invalid");
            continue;
        }
        ContractParameter* added = AddParameter(std::move(contractParameter));
        added->Load(*configNode);
    }
}

// ContractParameter.cs:576
void ContractParameter::Save(ConfigNode& node) {
    node.AddValue("name", TypeName());
    if (!id.empty()) node.AddValue("id", id);
    if (!enabled) node.AddValue("enabled", dotnet::FormatBool(enabled));
    if (optional) node.AddValue("optional", dotnet::FormatBool(optional));
    node.AddValue("state", dotnet::EnumToString(state));
    if (!disableOnStateChange) node.AddValue("disableOnStateChange", dotnet::FormatBool(disableOnStateChange));
    if (allowPartialFailure) node.AddValue("allowPartialFailure", allowPartialFailure);
    const std::string value = ToStr(FundsCompletion) + "," + ToStr(FundsFailure) + "," + ToStr(ScienceCompletion) + "," +
                              ToStr(ReputationCompletion) + "," + ToStr(ReputationFailure);
    node.AddValue("values", value);
    OnSave(node);
    for (size_t i = 0; i < parameters_.size(); ++i) {
        ConfigNode* node2 = node.AddNode("PARAM");
        parameters_[i]->Save(*node2);
    }
}

// ContractParameter.cs:612
void ContractParameter::Register() {
    if (enabled) {
        for (size_t i = 0; i < parameters_.size(); ++i) parameters_[i]->Register();
        OnRegister();
    }
}

// ContractParameter.cs:625
void ContractParameter::Unregister() {
    for (size_t i = 0; i < parameters_.size(); ++i) parameters_[i]->Unregister();
    OnUnregister();
}

// ContractParameter.cs:635
void ContractParameter::Update() {
    if (enabled) {
        for (size_t i = 0; i < parameters_.size(); ++i) parameters_[i]->Update();
        try {
            OnUpdate();
        } catch (const std::exception& ex) {
            log::Error(std::string("[ContractParameter] - Call to OnUpdate failed ") + ex.what());
        }
    }
}

// ContractParameter.cs:655
void ContractParameter::Reset() {
    if (enabled) {
        int count = static_cast<int>(parameters_.size());
        while (count-- > 0) parameters_[static_cast<size_t>(count)]->Reset();
        OnReset();
    }
}

// ContractParameter.cs:673
std::string ContractParameter::GetTitle() { return Localizer::Format("#autoLOC_268399"); }
// ContractParameter.cs:683
std::string ContractParameter::GetMessageComplete() { return Localizer::Format("#autoLOC_268411", {Title()}); }
// ContractParameter.cs:688
std::string ContractParameter::GetMessageFailed() { return Localizer::Format("#autoLOC_268417", {Title()}); }
// ContractParameter.cs:693
std::string ContractParameter::GetMessageIncomplete() { return Localizer::Format("#autoLOC_268423", {Title()}); }

// ContractParameter.cs:722
long long ContractParameter::CreateID() {
    long long num = 17;
    num = 391LL + static_cast<long long>(StringExtensions::GetHashCode_Net35(TypeName()));
    std::string hashString = HashString();
    num = num * 23LL + static_cast<long long>(StringExtensions::GetHashCode_Net35(hashString));
    for (size_t i = 0; i < parameters_.size(); ++i) {
        ContractParameter* contractParameter = parameters_[i].get();
        hashString = contractParameter->HashString();
        num = num * 23LL + static_cast<long long>(StringExtensions::GetHashCode_Net35(contractParameter->TypeName() + hashString));
    }
    return num;
}

// ContractParameter.cs:744
void ContractParameter::SetComplete() {
    if (state != ParameterState::Complete) {
        try {
            AwardCompletion();
        } catch (const std::exception& ex) {
            log::Error("[ContractParameter]: " + id + " on Contract " + parent_->Title() + " failed to process Completion Award processing.");
            log::Error(ex.what());
        }
        state = ParameterState::Complete;
        if (disableOnStateChange) Disable();
        OnStateChange.Fire(this, state);
        GE::Contract::onParameterChange.Fire(Root(), this);
        parent_->ParameterStateUpdate(this);
        if (ContractsEnvironment::Current().verboseDebugLog) log::Info("[ContractParameter]: " + id + " on Contract " + parent_->Title() + " set to Complete.");
    }
}

// ContractParameter.cs:772
void ContractParameter::SetIncomplete() {
    if (state != ParameterState::Incomplete) {
        if (disableOnStateChange) Disable();
        state = ParameterState::Incomplete;
        OnStateChange.Fire(this, state);
        GE::Contract::onParameterChange.Fire(Root(), this);
        parent_->ParameterStateUpdate(this);
        if (ContractsEnvironment::Current().verboseDebugLog) log::Info("[ContractParameter]: " + id + " on Contract " + parent_->Title() + " set to Incomplete.");
    }
}

// ContractParameter.cs:791
void ContractParameter::SetFailed() {
    if (state == ParameterState::Failed) return;
    try {
        PenalizeFailure();
    } catch (const std::exception& ex) {
        log::Error("[ContractParameter]: " + id + " on Contract " + parent_->Title() + " failed to process Penalize Failure Award processing.");
        log::Error(ex.what());
    }
    if (disableOnStateChange) Disable();
    state = ParameterState::Failed;
    OnStateChange.Fire(this, state);
    GE::Contract::onParameterChange.Fire(Root(), this);
    parent_->ParameterStateUpdate(this);
    if (allowPartialFailure) {
        int count = static_cast<int>(parameters_.size());
        while (count-- > 0) parameters_[static_cast<size_t>(count)]->Disable(true);
    }
    if (ContractsEnvironment::Current().verboseDebugLog) log::Info("[ContractParameter]: " + id + " on Contract " + parent_->Title() + " set to Failed.");
}

// ContractParameter.cs:832
void ContractParameter::SendStateMessage(const std::string& title, const std::string& message) { SendParameterMessage(*this, title, message); }

// ---- ContractPredicate -----------------------------------------------------------------------------------------------

// ContractPredicate.cs:20
ContractPredicate::ContractPredicate(IContractParameterHost* parent) : parent_(parent), root_(parent->Root()) {}
// ContractPredicate.cs:36
bool ContractPredicate::Test(Vessel*) { return false; }
// ContractPredicate.cs:41
bool ContractPredicate::Test(ProtoVessel*) { return false; }
// ContractPredicate.cs:46
std::string ContractPredicate::GetDescription() { return Localizer::Format("#autoLOC_268399"); }

}  // namespace ksp::Contracts
