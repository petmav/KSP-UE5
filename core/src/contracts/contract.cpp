// WP-37: Contracts.Contract (Assembly-CSharp/Contracts/Contract.cs).
#include <algorithm>
#include <cmath>

#include "ksp/career/career.hpp"
#include "ksp/career/format.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/enum_extensions.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/util/misc_util.hpp"

namespace ksp::Contracts {

using GE = events::GameEvents;
using KSP::Localization::Localizer;

namespace {
Game* CurrentGame() { return SaveEnvironment::Current().currentGame; }
std::string ToStr(double d) { return dotnet::FormatDoubleG(d); }
std::string ToStr(float f) { return dotnet::FormatSingleG(f); }
std::string Loc(const char* tag) { return Localizer::Format(tag); }
// string.Split(new char[]{',', ' '}, RemoveEmptyEntries)
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
}  // namespace

// ---- Contract ---------------------------------------------------------------------------------------------------------

// Contract.cs:220
Contract::Contract() { ++contractsInExistance; }
// C++ helper (see Contract.cs:156)
Contract::~Contract() { --contractsInExistance; }

// Contract.cs:208
double Contract::GameTime() { return ContractsEnvironment::Current().GameTime(); }

// Contract.cs:155
std::string Contract::LocalizedContractState() const { return Localizer::Format(util::Description(state)); }

// Contract.cs:189
void Contract::AllParameters(std::vector<ContractParameter*>& out) {
    for (auto& p : parameters_) p->AllParameters(out);
}

// Contract.cs:235
void Contract::SetExpiry() { SetExpiry(1, 7); }

// Contract.cs:240
void Contract::SetExpiry(int minDays, int maxDays) {
    const float range = static_cast<float>(UnityRandom::Global().Range(minDays, maxDays));
    const float day = static_cast<float>(KSPUtil::dateTimeFormatter().Day());
    const float r = Mathf::Round(f32store(fx(range) * fx(day) / 60.0));
    TimeExpiry = fx(r) * 60.0;  // float * float stored into a double: the exact product (C7)
}

// Contract.cs:240
void Contract::SetExpiry(float minDays, float maxDays) {
    const float range = UnityRandom::Global().Range(minDays, maxDays);
    const float day = static_cast<float>(KSPUtil::dateTimeFormatter().Day());
    const float r = Mathf::Round(f32store(fx(range) * fx(day) / 60.0));
    TimeExpiry = fx(r) * 60.0;  // float * float stored into a double: the exact product (C7)
}

// Contract.cs:245
void Contract::SetDeadlineDays(float days, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    const float day = static_cast<float>(KSPUtil::dateTimeFormatter().Day());
    TimeDeadline = fx(days) * fx(day) * fx(num);  // stored into a double: no float rounding (C7)
}

// Contract.cs:255
void Contract::SetDeadlineYears(float years, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    const float year = static_cast<float>(KSPUtil::dateTimeFormatter().Year());
    TimeDeadline = fx(years) * fx(year) * fx(num);  // stored into a double: no float rounding (C7)
}

// Contract.cs:277
void Contract::SetFunds(float advance, float completion, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    GameVariables& gv = *GameVariables::Instance;
    FundsAdvance = static_cast<double>(Mathf::Round(f32store(fx(advance) * fx(gv.GetContractFundsAdvanceFactor(prestige)) * fx(num))));
    FundsCompletion = static_cast<double>(Mathf::Round(f32store(fx(completion) * fx(gv.GetContractFundsCompletionFactor(prestige)) * fx(num))));
    FundsFailure = FundsAdvance;
}

// Contract.cs:277
void Contract::SetFunds(float advance, float completion, float failure, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    GameVariables& gv = *GameVariables::Instance;
    FundsAdvance = static_cast<double>(Mathf::Round(f32store(fx(advance) * fx(gv.GetContractFundsAdvanceFactor(prestige)) * fx(num))));
    FundsCompletion = static_cast<double>(Mathf::Round(f32store(fx(completion) * fx(gv.GetContractFundsCompletionFactor(prestige)) * fx(num))));
    FundsFailure = dotnet::Math::Max(FundsAdvance, static_cast<double>(Mathf::Round(f32store(fx(failure) * fx(gv.GetContractFundsFailureFactor(prestige)) * fx(num)))));
}

// Contract.cs:289
void Contract::SetReputation(float completion, float failure, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    GameVariables& gv = *GameVariables::Instance;
    ReputationCompletion = Mathf::Round(f32store(fx(completion) * fx(gv.GetContractReputationCompletionFactor(prestige)) * fx(num)));
    ReputationFailure = Mathf::Round(f32store(fx(failure) * fx(gv.GetContractReputationFailureFactor(prestige)) * fx(num)));
}

// Contract.cs:300
void Contract::SetReputation(float completion, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    ReputationCompletion = Mathf::Round(f32store(fx(completion) * fx(GameVariables::Instance->GetContractReputationCompletionFactor(prestige)) * fx(num)));
}

// Contract.cs:310
void Contract::SetScience(float completion, CelestialBody* body) {
    float num = 1.0f;
    if (body != nullptr) num = GetDestinationWeight(body);
    ScienceCompletion = Mathf::Round(f32store(fx(completion) * fx(GameVariables::Instance->GetContractScienceCompletionFactor(prestige)) * fx(num)));
}

// Contract.cs:320
float Contract::GetDestinationWeight(CelestialBody* body) { return GameVariables::Instance->GetContractDestinationWeight(body); }

// Contract.cs:325
bool Contract::AddKeywords(const std::vector<std::string>& kw) {
    if (!kw.empty()) {
        size_t num = 0;
        while (true) {
            if (num < kw.size()) {
                if (std::find(keywords.begin(), keywords.end(), kw[num]) == keywords.end()) break;
                ++num;
                continue;
            }
            return false;
        }
        keywords.push_back(kw[num]);
        return true;
    }
    return false;
}

// Contract.cs:350
bool Contract::AddKeywordsRequired(const std::vector<std::string>& kw) {
    if (!kw.empty()) {
        size_t num = 0;
        while (true) {
            if (num < kw.size()) {
                if (std::find(keywordsRequired.begin(), keywordsRequired.end(), kw[num]) == keywordsRequired.end()) break;
                ++num;
                continue;
            }
            return false;
        }
        keywordsRequired.push_back(kw[num]);
        return true;
    }
    return false;
}

// Contract.cs:375
ContractParameter* Contract::AddParameter(std::unique_ptr<ContractParameter> parameter, const std::string& id) {
    parameter->NestToParent(this);
    if (!id.empty()) parameter->SetID(id);
    parameters_.push_back(std::move(parameter));
    return parameters_.back().get();
}

// Contract.cs:391
ContractParameter* Contract::GetParameter(int index) { return parameters_.at(static_cast<size_t>(index)).get(); }

// Contract.cs:396
ContractParameter* Contract::GetParameter(const std::string& id) {
    for (auto& p : parameters_)
        if (p->ID() == id) return p.get();
    return nullptr;
}

// C++ helper (see Contract.cs:415)
ContractParameter* Contract::GetParameterOfType(const std::string& typeName) {
    for (auto& p : parameters_)
        if (p->TypeName() == typeName) return p.get();
    return nullptr;
}

// Contract.cs:463
void Contract::RemoveParameter(int index) { parameters_.erase(parameters_.begin() + index); }

// Contract.cs:463
void Contract::RemoveParameter(const std::string& id) {
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (parameters_[i]->ID() == id) {
            parameters_[i]->Unregister();
            parameters_.erase(parameters_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }
}

// C++ helper (see Contract.cs:480)
void Contract::RemoveParameterOfType(const std::string& typeName) {
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (parameters_[i]->TypeName() == typeName) {
            parameters_[i]->Unregister();
            parameters_.erase(parameters_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }
}

// Contract.cs:489
void Contract::RemoveParameter(ContractParameter* parameter) {
    parameter->Unregister();
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (parameters_[i].get() == parameter) {
            parameters_.erase(parameters_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }
}

// Contract.cs:516
void Contract::OnParameterStateChange(ContractParameter* p) {
    switch (p->State()) {
        case ParameterState::Failed: {
            if (!p->AllowPartialFailure()) {
                SetState(State::Failed);
                break;
            }
            bool flag = false;
            bool flag2 = false;
            int count2 = static_cast<int>(parameters_.size());
            while (count2-- > 0) {
                if (parameters_[static_cast<size_t>(count2)]->State() != ParameterState::Incomplete) {
                    if (parameters_[static_cast<size_t>(count2)]->State() == ParameterState::Complete) flag2 = true;
                    continue;
                }
                flag = true;
                break;
            }
            if (!flag) {
                if (flag2)
                    SetState(State::Completed);
                else
                    SetState(State::Failed);
            }
            break;
        }
        case ParameterState::Complete: {
            for (size_t i = 0; i < parameters_.size(); ++i)
                if (parameters_[i]->State() != ParameterState::Complete) return;
            SetState(State::Completed);
            break;
        }
        default: break;
    }
}

// Contract.cs:646
void Contract::AwardAdvance() {
    if (FundsAdvance != 0.0) {
        log::Info("Awarding " + ToStr(FundsAdvance) + " funds to player for contract advance");
        if (Funding::Instance != nullptr) Funding::Instance->AddFunds(FundsAdvance, TransactionReasons::ContractAdvance);
    }
}

// Contract.cs:658
void Contract::AwardCompletion() {
    CareerEnvironment& career = CareerEnvironment::Current();
    if (FundsCompletion != 0.0) {
        log::Info("Awarding " + ToStr(FundsCompletion) + " funds to player for contract completion");
        if (Funding::Instance != nullptr) Funding::Instance->AddFunds(FundsCompletion, TransactionReasons::ContractReward);
    }
    if (ScienceCompletion != 0.0f) {
        log::Info("Awarding " + ToStr(ScienceCompletion) + " science to player for contract completion");
        if (career.rndPresent && career.rndPresent() && career.rndAddScience) career.rndAddScience(ScienceCompletion, TransactionReasons::ContractReward);
    }
    if (ReputationCompletion != 0.0f) {
        log::Info("Awarding " + ToStr(ReputationCompletion) + " reputation to player for contract completion");
        if (Reputation::Instance != nullptr) Reputation::Instance->AddReputation(ReputationCompletion, TransactionReasons::ContractReward);
    }
}

// Contract.cs:686
void Contract::PenalizeCancellation() {
    const float num = Mathf::InverseLerp(static_cast<float>(dateAccepted), static_cast<float>(dateDeadline), static_cast<float>(dateFinished));
    const float repLossDeclined = CurrentGame()->Parameters->Career.RepLossDeclined;
    const double num2 = Mathfx::Lerp(FundsAdvance, FundsFailure, static_cast<double>(num));
    const float num3 = Mathfx::Lerp(repLossDeclined, ReputationFailure, num);
    if (num2 != 0.0) {
        log::Info("Deducting " + career::FormatNumber(num2, "N0") + " funds for contract cancellation");
        if (Funding::Instance != nullptr) Funding::Instance->AddFunds(0.0 - dotnet::Math::Abs(num2), TransactionReasons::ContractPenalty);
    }
    if (num3 != 0.0f) {
        log::Info("Deducting " + career::FormatNumber(static_cast<double>(num3), "N0") + " reputation for contract cancellation");
        if (Reputation::Instance != nullptr) Reputation::Instance->AddReputation(f32store(0.0 - fx(Mathf::Abs(num3))), TransactionReasons::ContractPenalty);
    }
    if (num > 0.01f) {
        SendStateMessage(Loc("#autoLOC_266201"), MessageCancelled() + StateMsgAddition(MessageCancellationPenalties(num2, num3)));
    }
}

// Contract.cs:714
void Contract::PenalizeFailure() {
    if (FundsFailure != 0.0) {
        log::Info("Penalizing " + ToStr(FundsFailure) + " funds from player for contract failure");
        if (Funding::Instance != nullptr) Funding::Instance->AddFunds(0.0 - dotnet::Math::Abs(FundsFailure), TransactionReasons::ContractPenalty);
    }
    if (ReputationFailure != 0.0f) {
        log::Info("Penalizing " + ToStr(ReputationFailure) + " reputation from player for contract failure");
        if (Reputation::Instance != nullptr) Reputation::Instance->AddReputation(f32store(0.0 - fx(Mathf::Abs(ReputationFailure))), TransactionReasons::ContractPenalty);
    }
}

// Contract.cs:791
std::string Contract::MessageOffered() { return Localizer::Format("#autoLOC_6002414", {Agent()->Title(), GetTitle()}); }
// Contract.cs:796
std::string Contract::MessageOfferExpired() { return Localizer::Format("#autoLOC_6002415", {Agent()->Title(), GetTitle()}); }
// Contract.cs:801
std::string Contract::MessageCancelled() { return Localizer::Format("#autoLOC_6002416", {Agent()->Title(), GetTitle()}); }
// Contract.cs:806
std::string Contract::MessageAccepted() { return Localizer::Format("#autoLOC_266321", {Agent()->Title()}) + ": " + GetTitle(); }
// Contract.cs:811
std::string Contract::MessageDeadlineExpired() { return Localizer::Format("#autoLOC_266330", {Agent()->Title()}) + ": " + GetTitle(); }
// Contract.cs:816
std::string Contract::MessageFailed() { return Localizer::Format("#autoLOC_266339", {Agent()->Title()}) + ": " + GetTitle(); }
// Contract.cs:821
std::string Contract::MessageCompleted() { return Localizer::Format("#autoLOC_266348", {Agent()->Title()}) + ": " + GetTitle(); }

// Contract.cs:826
std::string Contract::MessageAdvances() {
    return ContractRichText::provider ? ContractRichText::provider(*this, "advances", 0.0, 0.0f) : std::string();
}
// Contract.cs:853
std::string Contract::MessageRewards() {
    return ContractRichText::provider ? ContractRichText::provider(*this, "rewards", 0.0, 0.0f) : std::string();
}
// Contract.cs:888
std::string Contract::MessageFailurePenalties() {
    return ContractRichText::provider ? ContractRichText::provider(*this, "failure", 0.0, 0.0f) : std::string();
}
// Contract.cs:919
std::string Contract::MessageCancellationPenalties(double fundsPenalty, float repPenalty) {
    return ContractRichText::provider ? ContractRichText::provider(*this, "cancellation", fundsPenalty, repPenalty) : std::string();
}
// Contract.cs:950
std::string Contract::MissionControlTextRich() {
    return ContractRichText::provider ? ContractRichText::provider(*this, "mission", 0.0, 0.0f) : std::string();
}

// Contract.cs:1180
void Contract::SetupSeed(int seed) {
    if (seed == -1) seed = static_cast<int>(ContractsEnvironment::Current().NowTicks() & 0x7FFFFFFFLL);
    missionSeed = seed;
    UnityRandom::Global().InitState(missionSeed);
}

// Contract.cs:1190
void Contract::SetupID() {
    long long num = 17;
    num = 391LL + static_cast<long long>(StringExtensions::GetHashCode_Net35(TypeName()));
    const std::string hashString = GetHashString();
    if (!hashString.empty()) num = num * 23LL + static_cast<long long>(StringExtensions::GetHashCode_Net35(GetHashString()));
    for (size_t i = 0; i < parameters_.size(); ++i) {
        ContractParameter* contractParameter = parameters_[i].get();
        const std::string hs = contractParameter->HashString();
        num = num * 23LL + static_cast<long long>(StringExtensions::GetHashCode_Net35(contractParameter->TypeName()));
        if (!hs.empty()) num = num * 23LL + static_cast<long long>(StringExtensions::GetHashCode_Net35(hs));
    }
    contractID = num;
}

// Contract.cs:1213
void Contract::SendStateMessage(const std::string& title, const std::string& message) {
    const std::string text = "<b>" + GetTitle() + "</b>\n\n" + message;
    ContractsEnvironment& env = ContractsEnvironment::Current();
    if (env.postMessage)
        env.postMessage(title, text);
    else if (CareerEnvironment::Current().postMessage)
        CareerEnvironment::Current().postMessage(title, text);
    log::Info("Contract (" + GetTitle() + "): " + message);
}

// Contract.cs:1222
bool Contract::Offer() {
    if (state == State::Generated) {
        if (expiryType == DeadlineType::Floating) dateExpire = GameTime() + TimeExpiry;
        if (deadlineType == DeadlineType::Fixed) dateDeadline = GameTime() + TimeDeadline;
        SetState(State::Offered);
        return true;
    }
    return false;
}

// Contract.cs:1240
bool Contract::Accept() {
    if (state == State::Offered) {
        dateAccepted = GameTime();
        if (deadlineType == DeadlineType::Floating) dateDeadline = GameTime() + TimeDeadline;
        SetState(State::Active);
        return true;
    }
    return false;
}

// Contract.cs:1255
bool Contract::Decline() {
    if (state == State::Offered) {
        dateFinished = GameTime();
        SetState(State::Declined);
        if (Reputation::Instance != nullptr && CurrentGame()->Parameters->Career.RepLossDeclined > 0.0f)
            Reputation::Instance->AddReputation(f32store(0.0 - fx(CurrentGame()->Parameters->Career.RepLossDeclined)), TransactionReasons::ContractDecline);
        return true;
    }
    return false;
}

// Contract.cs:1270
bool Contract::Cancel() {
    if (state == State::Active) {
        dateFinished = GameTime();
        SetState(State::Cancelled);
        return true;
    }
    return false;
}

// Contract.cs:1281
void Contract::Update() {
    switch (state) {
        case State::Active: {
            if (deadlineType != DeadlineType::None && GameTime() >= dateDeadline) {
                dateFinished = dateDeadline;
                SetState(State::DeadlineExpired);
                break;
            }
            if (!MeetRequirements()) {
                dateFinished = dateExpire;
                SetState(State::OfferExpired);
                break;
            }
            for (size_t i = 0; i < parameters_.size(); ++i) parameters_[i]->Update();
            try {
                OnUpdate();
            } catch (const std::exception& ex) {
                log::Error(std::string("[Contract] - Call to OnUpdate failed ") + ex.what());
            }
            break;
        }
        case State::Offered:
            if (expiryType != DeadlineType::None && GameTime() >= dateExpire) {
                dateFinished = dateExpire;
                SetState(State::OfferExpired);
            } else if (!MeetRequirements()) {
                dateFinished = dateExpire;
                SetState(State::OfferExpired);
            }
            break;
        default: break;
    }
}

// Contract.cs:1330
void Contract::Reset() {
    if (state == State::Active) {
        int count = static_cast<int>(parameters_.size());
        while (count-- > 0) parameters_[static_cast<size_t>(count)]->Reset();
        OnReset();
    }
}

// Contract.cs:1343
void Contract::Withdraw() {
    if (state != State::Withdrawn) SetState(State::Withdrawn);
}

// Contract.cs:1351
bool Contract::Fail() {
    if (state == State::Active) {
        dateFinished = GameTime();
        SetState(State::Failed);
        return true;
    }
    return false;
}

// Contract.cs:1362
bool Contract::Complete() {
    if (state == State::Active) {
        dateFinished = GameTime();
        SetState(State::Completed);
        return true;
    }
    return false;
}

// Contract.cs:1373
void Contract::SetState(State newState) {
    if (newState == state) return;
    if (newState == State::Active)
        Register();
    else
        Unregister();
    state = newState;
    OnStateChange.Fire(state);
    if (!IgnoresWeight && !AutoAccept) {
        ContractSystem::AdjustWeight(TypeName(), this);
        if (HasWeightBodies()) {
            std::vector<CelestialBody*> weightBodies = WeightBodies();
            int count = static_cast<int>(weightBodies.size());
            while (count-- > 0) {
                if (weightBodies[static_cast<size_t>(count)] != nullptr)
                    ContractSystem::AdjustWeight(weightBodies[static_cast<size_t>(count)]->name(), this);
            }
        }
    }
    switch (state) {
        case State::Offered:
            OnOffered();
            GE::Contract::onOffered.Fire(this);
            break;
        case State::OfferExpired:
            OnOfferExpired();
            GE::Contract::onFinished.Fire(this);
            break;
        case State::Declined:
            OnDeclined();
            GE::Contract::onDeclined.Fire(this);
            break;
        case State::Cancelled:
            PenalizeCancellation();
            OnCancelled();
            OnFinished();
            GE::Contract::onCancelled.Fire(this);
            GE::Contract::onFailed.Fire(this);
            GE::Contract::onFinished.Fire(this);
            break;
        case State::Active:
            AwardAdvance();
            OnAccepted();
            GE::Contract::onAccepted.Fire(this);
            break;
        case State::Completed:
            AwardCompletion();
            OnCompleted();
            OnFinished();
            SendStateMessage(Loc("#autoLOC_267072"), MessageCompleted() + StateMsgAddition(MessageRewards()));
            GE::Contract::onCompleted.Fire(this);
            GE::Contract::onFinished.Fire(this);
            break;
        case State::DeadlineExpired:
            PenalizeFailure();
            OnDeadlineExpired();
            OnFinished();
            SendStateMessage(Loc("#autoLOC_267084"), MessageDeadlineExpired() + StateMsgAddition(MessageFailurePenalties()));
            GE::Contract::onFailed.Fire(this);
            GE::Contract::onFinished.Fire(this);
            break;
        case State::Failed:
            PenalizeFailure();
            OnFailed();
            OnFinished();
            SendStateMessage(Loc("#autoLOC_267092"), MessageFailed() + StateMsgAddition(MessageFailurePenalties()));
            GE::Contract::onFailed.Fire(this);
            GE::Contract::onFinished.Fire(this);
            break;
        case State::Withdrawn:
            OnWithdrawn();
            OnFinished();
            GE::Contract::onFinished.Fire(this);
            break;
        default: break;
    }
}

// Contract.cs:1467
std::string Contract::StateMsgAddition(const std::string& addition) {
    if (addition.empty()) return std::string();
    return "\n\n" + addition;
}

// Contract.cs:1476
bool Contract::IsFinished() {
    const unsigned s = static_cast<unsigned>(state);
    if ((s - 2u) > 2u && (s - 6u) > 3u) return false;
    return true;
}

// Contract.cs:1486
void Contract::Register() {
    for (size_t i = 0; i < parameters_.size(); ++i) parameters_[i]->Register();
    OnRegister();
}

// Contract.cs:1496
void Contract::Unregister() {
    for (size_t i = 0; i < parameters_.size(); ++i) parameters_[i]->Unregister();
    OnUnregister();
}

// Contract.cs:1506
void Contract::Kill() {
    OnFinished();
    GE::Contract::onFinished.Fire(this);
    Unregister();
}

// Contract.cs:1513
void Contract::SetViewed(Viewed v) {
    if (static_cast<int>(v) > static_cast<int>(viewed)) {
        viewed = v;
        OnViewedChange.Fire(v);
        switch (v) {
            case Viewed::Read:
                OnRead();
                GE::Contract::onRead.Fire(this);
                break;
            case Viewed::Seen:
                OnSeen();
                GE::Contract::onSeen.Fire(this);
                break;
            default: break;
        }
    }
}

// Contract.cs:1533
std::unique_ptr<Contract> Contract::Generate(const std::string& contractTypeName, ContractPrestige difficulty, int seed, State state) {
    std::unique_ptr<Contract> contract = ContractSystem::CreateContract(contractTypeName);
    contract->contractGuid = ContractsEnvironment::Current().NewGuid();
    contract->SetupSeed(seed);
    contract->prestige = difficulty;
    contract->state = state;
    if (!contract->MeetRequirements()) return nullptr;
    if (!contract->Generate()) return nullptr;
    if (contract->agent == nullptr) contract->agent = Agents::AgentList::Instance->GetSuitableAgentForContract(*contract);
    if (contract->agent == nullptr) {
        contract->OnGenerateFailed();
        return nullptr;
    }
    if (!contract->agent->ProcessContract(*contract)) {
        contract->OnGenerateFailed();
        return nullptr;
    }
    contract->FundsFailure = dotnet::Math::Max(contract->FundsFailure, contract->FundsAdvance);
    contract->SetupID();
    return contract;
}

// Contract.cs:1567
bool Contract::Load(Contract& contract, ConfigNode& node) {
    if (node.HasValue("guid"))
        contract.contractGuid = Guid::Parse(*node.GetValue("guid"));
    else
        contract.contractGuid = ContractsEnvironment::Current().NewGuid();
    std::optional<std::string> text = node.GetValue("prestige");
    if (!text.has_value()) {
        text = node.GetValue("diff");
        if (text == std::optional<std::string>("Easy")) text = "0";
        if (text == std::optional<std::string>("Medium")) text = "1";
        if (text == std::optional<std::string>("Hard")) text = "2";
    }
    const std::optional<std::string> value = node.GetValue("seed");
    const std::optional<std::string> value2 = node.GetValue("state");
    const std::optional<std::string> value3 = node.GetValue("viewed");
    const std::optional<std::string> value4 = node.GetValue("values");
    const std::optional<std::string> value5 = node.GetValue("agent");
    std::string name;
    bool flag = node.HasValue("agentName");
    if (flag) name = *node.GetValue("agentName");
    if (text.has_value() && value.has_value() && value2.has_value() && value4.has_value() && value5.has_value()) {
        contract.SetupSeed(dotnet::ParseInt32(*value));
        contract.prestige = static_cast<ContractPrestige>(dotnet::ParseInt32(*text));
        contract.state = dotnet::EnumParseFull<State>(*value2);
        contract.viewed = value3.has_value() ? dotnet::EnumParseFull<Viewed>(*value3) : Viewed::Unseen;
        if (flag)
            contract.agent = Agents::AgentList::Instance->GetAgent(name);
        else
            contract.agent = Agents::AgentList::Instance->GetAgentbyTitle(*value5);
        if (contract.agent == nullptr) {
            log::Error("Contract: Contract agent does not exist, randomizing agent");
            contract.agent = Agents::AgentList::Instance->GetAgentRandom();
        }
        if (node.HasValue("deadlineType")) contract.deadlineType = dotnet::EnumParseFull<DeadlineType>(*node.GetValue("deadlineType"));
        if (node.HasValue("expiryType")) contract.expiryType = dotnet::EnumParseFull<DeadlineType>(*node.GetValue("expiryType"));
        bool b = false;
        if (node.HasValue("autoAccept")) {
            if (!dotnet::TryParseBool(*node.GetValue("autoAccept"), b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
            contract.AutoAccept = b;
        }
        if (node.HasValue("ignoresWeight")) {
            if (!dotnet::TryParseBool(*node.GetValue("ignoresWeight"), b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
            contract.IgnoresWeight = b;
        }
        const std::vector<std::string> array = SplitCommaSpace(*value4);
        if (array.size() != 12) {
            log::Error("Contract: Contract config values are invalid");
            return false;
        }
        contract.TimeExpiry = dotnet::ParseDouble(array[0]);
        contract.TimeDeadline = dotnet::ParseDouble(array[1]);
        contract.FundsAdvance = dotnet::ParseDouble(array[2]);
        contract.FundsCompletion = dotnet::ParseDouble(array[3]);
        contract.FundsFailure = dotnet::ParseDouble(array[4]);
        contract.ScienceCompletion = dotnet::ParseSingle(array[5]);
        contract.ReputationCompletion = dotnet::ParseSingle(array[6]);
        contract.ReputationFailure = dotnet::ParseSingle(array[7]);
        contract.dateExpire = dotnet::ParseDouble(array[8]);
        contract.dateAccepted = dotnet::ParseDouble(array[9]);
        contract.dateDeadline = dotnet::ParseDouble(array[10]);
        contract.dateFinished = dotnet::ParseDouble(array[11]);
        std::vector<ConfigNode*> nodes = node.GetNodes("PARAM");
        for (size_t num = 0; num < nodes.size(); ++num) {
            ConfigNode* configNode = nodes[num];
            const std::optional<std::string> value6 = configNode->GetValue("name");
            if (!value6.has_value()) {
                log::Error("Contract: Contract parameter config is invalid");
                return false;
            }
            std::unique_ptr<ContractParameter> contractParameter = ContractSystem::CreateParameter(*value6);
            if (!contractParameter) {
                log::Error("Contract: Contract parameter type name '" + *value6 + "' is invalid");
                return false;
            }
            ContractParameter* added = contract.AddParameter(std::move(contractParameter));
            added->Load(*configNode);
        }
        try {
            contract.OnLoad(node);
            contract.SetupID();
        } catch (const std::exception& ex) {
            log::Error("Contract: Exception while OnLoad() or SetupID() on contract " + contract.Title() + ", exception " + ex.what());
        }
        return true;
    }
    log::Error("Contract: Contract config is invalid");
    return false;
}

// Contract.cs:1702
void Contract::Save(ConfigNode& node) {
    node.AddValue("guid", contractGuid.ToString());
    node.AddValue("type", TypeName());
    node.AddValue("prestige", dotnet::FormatInt64(static_cast<int>(Prestige())));
    node.AddValue("seed", static_cast<int32_t>(MissionSeed()));
    node.AddValue("state", dotnet::EnumToString(ContractState()));
    node.AddValue("viewed", dotnet::EnumToString(ContractViewed()));
    node.AddValue("agent", agent->Title());
    node.AddValue("agentName", agent->Name());
    node.AddValue("deadlineType", dotnet::EnumToString(deadlineType));
    node.AddValue("expiryType", dotnet::EnumToString(expiryType));
    if (AutoAccept) node.AddValue("autoAccept", dotnet::FormatBool(AutoAccept));
    if (IgnoresWeight) node.AddValue("ignoresWeight", dotnet::FormatBool(IgnoresWeight));
    const std::string value = ToStr(TimeExpiry) + "," + ToStr(TimeDeadline) + "," + ToStr(FundsAdvance) + "," + ToStr(FundsCompletion) + "," +
                              ToStr(FundsFailure) + "," + ToStr(ScienceCompletion) + "," + ToStr(ReputationCompletion) + "," +
                              ToStr(ReputationFailure) + "," + ToStr(dateExpire) + "," + ToStr(dateAccepted) + "," + ToStr(dateDeadline) + "," +
                              ToStr(dateFinished);
    node.AddValue("values", value);
    OnSave(node);
    for (size_t i = 0; i < parameters_.size(); ++i) {
        ConfigNode* node2 = node.AddNode("PARAM");
        parameters_[i]->Save(*node2);
    }
}

namespace {
std::vector<CelestialBody*>& LocalBodies() { return SaveEnvironment::Current().localBodies; }
// List.Sort(b1.scienceValues.RecoveryValue.CompareTo(b2...)): the unstable Mono introsort
void SortByRecovery(std::vector<CelestialBody*>& list) {
    dotnet::ListSort(list, [](CelestialBody* b1, CelestialBody* b2) {
        const float a = b1->scienceValues.RecoveryValue;
        const float b = b2->scienceValues.RecoveryValue;
        return a < b ? -1 : (a > b ? 1 : (a == b ? 0 : (std::isnan(a) ? (std::isnan(b) ? 0 : -1) : 1)));
    });
}
}  // namespace

// Contract.cs:1739
std::vector<CelestialBody*> Contract::GetBodies(bool includeKerbin, bool includeSun) {
    std::vector<CelestialBody*> list;
    for (size_t i = 0; i < LocalBodies().size(); ++i)
        if ((i != 0 || includeSun) && (i != 1 || includeKerbin)) list.push_back(LocalBodies()[i]);
    SortByRecovery(list);
    return list;
}

// Contract.cs:1753
std::vector<CelestialBody*> Contract::GetBodies_Reached(bool includeKerbin, bool includeSun) {
    std::vector<CelestialBody*> list;
    for (size_t i = 0; i < LocalBodies().size(); ++i)
        if ((i != 0 || includeSun) && (i != 1 || includeKerbin) && ProgressTracking::Instance()->NodeReached({LocalBodies()[i]->name()}))
            list.push_back(LocalBodies()[i]);
    SortByRecovery(list);
    return list;
}

// Contract.cs:1767
std::vector<CelestialBody*> Contract::GetBodies_NotReached(bool includeKerbin, bool includeSun) {
    std::vector<CelestialBody*> list;
    for (size_t i = 0; i < LocalBodies().size(); ++i)
        if ((i != 0 || includeSun) && (i != 1 || includeKerbin) && !ProgressTracking::Instance()->NodeReached({LocalBodies()[i]->name()}))
            list.push_back(LocalBodies()[i]);
    SortByRecovery(list);
    return list;
}

// Contract.cs:1781
std::vector<CelestialBody*> Contract::GetBodies_InComplete(bool includeKerbin, bool includeSun, const std::string& notComplete) {
    std::vector<CelestialBody*> list;
    for (size_t i = 0; i < LocalBodies().size(); ++i)
        if ((i != 0 || includeSun) && (i != 1 || includeKerbin) &&
            !ProgressTracking::Instance()->NodeComplete({LocalBodies()[i]->name(), notComplete}))
            list.push_back(LocalBodies()[i]);
    SortByRecovery(list);
    return list;
}

// Contract.cs:1795
std::vector<CelestialBody*> Contract::GetBodies_Complete(bool includeKerbin, bool includeSun, const std::string& complete) {
    std::vector<CelestialBody*> list;
    for (size_t i = 0; i < LocalBodies().size(); ++i)
        if ((i != 0 || includeSun) && (i != 1 || includeKerbin) &&
            ProgressTracking::Instance()->NodeComplete({LocalBodies()[i]->name(), complete}))
            list.push_back(LocalBodies()[i]);
    SortByRecovery(list);
    return list;
}

// Contract.cs:1809
int Contract::CountBodies_Complete(bool includeKerbin, bool includeSun, const std::string& nodeComplete) {
    int num = 0;
    for (size_t i = 0; i < LocalBodies().size(); ++i)
        if ((i != 0 || includeSun) && (i != 1 || includeKerbin) &&
            ProgressTracking::Instance()->NodeComplete({LocalBodies()[i]->name(), nodeComplete}))
            ++num;
    return num;
}

// Contract.cs:1822
int Contract::CountBodies_Reached(bool includeKerbin, bool includeSun) {
    int num = 0;
    for (size_t i = 0; i < LocalBodies().size(); ++i)
        if ((i != 0 || includeSun) && (i != 1 || includeKerbin) && ProgressTracking::Instance()->NodeReached({LocalBodies()[i]->name()})) ++num;
    return num;
}

// Contract.cs:1875
std::vector<CelestialBody*> Contract::GetBodies(const std::string& nodeName, ProgressState nodeState, const std::function<bool(CelestialBody*)>& where) {
    std::vector<CelestialBody*> list;
    AddBodyNodes(nodeState, nodeName, list, ProgressTracking::Instance()->GetBodyTree(), where);
    return list;
}

// Contract.cs:1842
void Contract::AddBodyNodes(ProgressState nodeState, const std::string& nodeName, std::vector<CelestialBody*>& bodies,
                            KSPAchievements::CelestialBodySubtree* tree, const std::function<bool(CelestialBody*)>& where) {
    ProgressNode* progressNode = tree->Subtree()[nodeName];
    if (progressNode != nullptr) {
        switch (nodeState) {
            case ProgressState::Unreached:
                if (!progressNode->IsReached() && (!where || where(tree->Body))) bodies.push_back(tree->Body);
                break;
            case ProgressState::Reached:
                if (progressNode->IsReached() && (!where || where(tree->Body))) bodies.push_back(tree->Body);
                break;
            case ProgressState::Complete:
                if (progressNode->IsComplete() && (!where || where(tree->Body))) bodies.push_back(tree->Body);
                break;
            default: break;
        }
    }
    for (size_t i = 0; i < tree->childTrees.size(); ++i) AddBodyNodes(nodeState, nodeName, bodies, tree->childTrees[i], nullptr);
}

// Contract.cs:1920
std::vector<CelestialBody*> Contract::GetBodies(ProgressState bodyState, const std::function<bool(CelestialBody*)>& where) {
    std::vector<KSPAchievements::CelestialBodySubtree*> list;
    AddBodyTrees(bodyState, list, ProgressTracking::Instance()->GetBodyTree());
    std::vector<CelestialBody*> list2;
    for (size_t i = 0; i < list.size(); ++i)
        if (!where || where(list[i]->Body)) list2.push_back(list[i]->Body);
    return list2;
}

// Contract.cs:1936
void Contract::AddBodyTrees(ProgressState bodyState, std::vector<KSPAchievements::CelestialBodySubtree*>& bodies,
                            KSPAchievements::CelestialBodySubtree* tree) {
    switch (bodyState) {
        case ProgressState::Unreached:
            if (!tree->IsReached()) bodies.push_back(tree);
            break;
        case ProgressState::Reached:
            if (tree->IsReached()) bodies.push_back(tree);
            break;
        case ProgressState::Complete:
            if (tree->IsComplete()) bodies.push_back(tree);
            break;
        default: break;
    }
    for (size_t i = 0; i < tree->childTrees.size(); ++i) AddBodyTrees(bodyState, bodies, tree->childTrees[i]);
}

// Contract.cs:1920
std::vector<CelestialBody*> Contract::GetBodies(ProgressState bodyState, const std::string& nodeName, ProgressState nodeState,
                                                const std::function<bool(CelestialBody*)>& where) {
    std::vector<KSPAchievements::CelestialBodySubtree*> list;
    AddBodyTrees(bodyState, nodeName, nodeState, list, ProgressTracking::Instance()->GetBodyTree());
    std::vector<CelestialBody*> list2;
    for (size_t i = 0; i < list.size(); ++i)
        if (!where || where(list[i]->Body)) list2.push_back(list[i]->Body);
    return list2;
}

// Contract.cs:1936
void Contract::AddBodyTrees(ProgressState bodyState, const std::string& nodeName, ProgressState nodeState,
                            std::vector<KSPAchievements::CelestialBodySubtree*>& bodies, KSPAchievements::CelestialBodySubtree* tree) {
    if ((bodyState == ProgressState::Reached && tree->IsReached()) || (bodyState == ProgressState::Complete && tree->IsComplete())) {
        ProgressNode* progressNode = tree->Subtree()[nodeName];
        if (progressNode != nullptr) {
            switch (nodeState) {
                case ProgressState::Unreached:
                    if (!progressNode->IsReached()) bodies.push_back(tree);
                    break;
                case ProgressState::Reached:
                    if (progressNode->IsReached()) bodies.push_back(tree);
                    break;
                case ProgressState::Complete:
                    if (progressNode->IsComplete()) bodies.push_back(tree);
                    break;
                default: break;
            }
        }
    }
    for (size_t i = 0; i < tree->childTrees.size(); ++i) AddBodyTrees(bodyState, nodeName, nodeState, bodies, tree->childTrees[i]);
}

// Contract.cs:1972 (the C# declares GetBodies_NextUnreached before WeightedBodyChoice; line order: :1996 / :2003)
std::vector<CelestialBody*> Contract::GetBodies_NextUnreached(int depth, const std::function<bool(CelestialBody*)>& where) {
    std::vector<KSPAchievements::CelestialBodySubtree*> list;
    AddNextUnreachedBodyTrees(depth, list, ProgressTracking::Instance()->GetBodyTree(), where);
    std::vector<CelestialBody*> list2;
    for (size_t i = 0; i < list.size(); ++i) list2.push_back(list[i]->Body);
    return list2;
}

// Contract.cs:1985
CelestialBody* Contract::WeightedBodyChoice(const std::vector<CelestialBody*>& bodies, KSPRandom* generator) {
    if (IgnoresWeight) {
        if (generator != nullptr) return bodies[static_cast<size_t>(generator->Next(0, static_cast<int32_t>(bodies.size())))];
        return bodies[static_cast<size_t>(UnityRandom::Global().Range(0, static_cast<int32_t>(bodies.size())))];
    }
    return ContractSystem::WeightedBodyChoice(bodies, generator);
}

// Contract.cs:2003
void Contract::AddNextUnreachedBodyTrees(int& depth, std::vector<KSPAchievements::CelestialBodySubtree*>& bodies,
                                         KSPAchievements::CelestialBodySubtree* tree, const std::function<bool(CelestialBody*)>& where) {
    if (!tree->IsReached() && (!where || where(tree->Body))) {
        bodies.push_back(tree);
        --depth;
    }
    for (size_t i = 0; i < tree->childTrees.size(); ++i) {
        if (depth <= 0) break;
        AddNextUnreachedBodyTrees(depth, bodies, tree->childTrees[i], where);
    }
}

}  // namespace ksp::Contracts
