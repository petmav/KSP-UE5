// WP-37: Contracts.Agents (Assembly-CSharp/Contracts/Agents/*.cs and Contracts/Agents/Mentalities/*.cs).
#include "ksp/contracts/agents.hpp"

#include <algorithm>
#include <functional>
#include <map>

#include "ksp/contracts/contract.hpp"
#include "ksp/core/log.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"

namespace ksp::Contracts::Agents {

using KSP::Localization::Localizer;

namespace {
std::string Trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\n' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\n' || s[b - 1] == '\r')) --b;
    return s.substr(a, b - a);
}
std::vector<std::string> SplitRemoveEmpty(const std::string& s, const std::string& seps) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (seps.find(c) != std::string::npos) {
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

// ---- AgentStanding ---------------------------------------------------------------------------------------------------

// AgentStanding.cs:27
AgentStanding::AgentStanding(Agent* parentAgent, std::string agentName, float standing)
    : agentName_(std::move(agentName)), parentAgent_(parentAgent), standing_(standing) {}

// AgentStanding.cs:34
bool AgentStanding::Link() {
    agent_ = AgentList::Instance->GetAgent(agentName_);
    if (agent_ == parentAgent_) agent_ = nullptr;
    return agent_ != nullptr;
}

// ---- AgentMentality (base) -------------------------------------------------------------------------------------------

namespace {

// AgentMentality.cs:104-174: the factor of a Factor* helper
float FundsFactor(Contract& c, float factor, bool positive) {
    float num = GameVariables::Instance->GetMentalityFundsFactor(factor, c.Prestige());
    if (!positive) num = f32store(1.0 / fx(num));
    return num;
}

}  // namespace

// AgentMentality.cs:122
void AgentMentality::FactorFundsAdvance(Contract& contract, bool positive) {
    const float num = FundsFactor(contract, Factor(), positive);
    contract.FundsAdvance *= static_cast<double>(num);
}
// AgentMentality.cs:132
void AgentMentality::FactorFundsCompletion(Contract& contract, bool positive) {
    const float num = FundsFactor(contract, Factor(), positive);
    contract.FundsCompletion *= static_cast<double>(num);
}
// AgentMentality.cs:142
void AgentMentality::FactorFundsFailure(Contract& contract, bool positive) {
    const float num = FundsFactor(contract, Factor(), positive);
    contract.FundsFailure *= static_cast<double>(num);
}
// AgentMentality.cs:152
void AgentMentality::FactorScienceCompletion(Contract& contract, bool positive) {
    float num = GameVariables::Instance->GetMentalityScienceFactor(Factor(), contract.Prestige());
    if (!positive) num = f32store(1.0 / fx(num));
    contract.ScienceCompletion = f32store(fx(contract.ScienceCompletion) * fx(num));
}
// AgentMentality.cs:162
void AgentMentality::FactorReputationCompletion(Contract& contract, bool positive) {
    float num = GameVariables::Instance->GetMentalityReputationFactor(Factor(), contract.Prestige());
    if (!positive) num = f32store(1.0 / fx(num));
    contract.ReputationCompletion = f32store(fx(contract.ReputationCompletion) * fx(num));
}
// AgentMentality.cs:172
void AgentMentality::FactorReputationFailure(Contract& contract, bool positive) {
    float num = GameVariables::Instance->GetMentalityReputationFactor(Factor(), contract.Prestige());
    if (!positive) num = f32store(1.0 / fx(num));
    contract.ReputationFailure = f32store(fx(contract.ReputationFailure) * fx(num));
}
// AgentMentality.cs:182
void AgentMentality::FactorDeadline(Contract& contract, bool positive) {
    float num = GameVariables::Instance->GetMentalityDeadlineFactor(Factor(), contract.Prestige());
    if (!positive) num = f32store(1.0 / fx(num));
    contract.TimeDeadline *= static_cast<double>(num);
}
// AgentMentality.cs:192
void AgentMentality::FactorExpiry(Contract& contract, bool positive) {
    float num = GameVariables::Instance->GetMentalityExpiryFactor(Factor(), contract.Prestige());
    if (!positive) num = f32store(1.0 / fx(num));
    contract.TimeExpiry *= static_cast<double>(num);
}

// ---- the mentalities (Contracts/Agents/Mentalities/*.cs) ---------------------------------------------------------------

namespace {

// the shared shape: display / description tags and the "Commercial" keyword score
struct Tagged : AgentMentality {
    const char* name;
    const char* displayTag;
    const char* descTag;
    Tagged(const char* n, const char* d, const char* desc) : name(n), displayTag(d), descTag(desc) {}
    std::string TypeName() const override { return name; }
    std::string GetDisplayName() override { return Localizer::Format(displayTag); }
    std::string GetDescription() override { return descTag != nullptr ? Localizer::Format(descTag) : std::string(); }
};
struct CommercialKw : Tagged {
    using Tagged::Tagged;
    KeywordScore ScoreKeyword(const std::string& keyword) override { return keyword == "Commercial" ? KeywordScore::Positive : KeywordScore::None; }
};

// Commercial.cs:7
struct Commercial : CommercialKw {
    Commercial() : CommercialKw("Commercial", "#autoLOC_7001036", "#autoLOC_264842") {}
    void ProcessContract(Contract& c) override {  // Commercial.cs:27
        FactorFundsCompletion(c, true);
        FactorFundsFailure(c, true);
        FactorReputationCompletion(c, false);
    }
};
struct Competitive : CommercialKw {  // Competitive.cs:7
    Competitive() : CommercialKw("Competitive", "#autoLOC_7001037", nullptr) {}
};
struct Conglomerate : Tagged {  // Conglomerate.cs:7
    Conglomerate() : Tagged("Conglomerate", "#autoLOC_7001038", "#autoLOC_264908") {}
    KeywordScore ScoreKeyword(const std::string& keyword) override { return keyword == "Commercial" ? KeywordScore::PositiveHigh : KeywordScore::None; }
    void ProcessContract(Contract& c) override {
        FactorExpiry(c, false);
        FactorDeadline(c, false);
        FactorFundsAdvance(c, true);
        FactorFundsCompletion(c, true);
    }
};
struct Cooperative : CommercialKw {  // Cooperative.cs:7
    Cooperative() : CommercialKw("Cooperative", "#autoLOC_7001039", nullptr) {}
};
struct EasyGoing : CommercialKw {  // EasyGoing.cs:7
    EasyGoing() : CommercialKw("EasyGoing", "#autoLOC_7001040", "#autoLOC_264975") {}
    void ProcessContract(Contract& c) override {
        FactorReputationFailure(c, false);
        FactorReputationCompletion(c, false);
    }
};
struct Ecomental : CommercialKw {  // Ecomental.cs:7
    Ecomental() : CommercialKw("Ecomental", "#autoLOC_7001041", "#autoLOC_265008") {}
};
struct Economic : CommercialKw {  // Economic.cs:7
    Economic() : CommercialKw("Economic", "#autoLOC_7001042", "#autoLOC_265039") {}
    void ProcessContract(Contract& c) override {
        FactorFundsAdvance(c, true);
        FactorFundsFailure(c, true);
        FactorFundsCompletion(c, true);
        FactorReputationCompletion(c, false);
    }
};
struct Hasty : CommercialKw {  // Hasty.cs:7
    Hasty() : CommercialKw("Hasty", "#autoLOC_7001043", "#autoLOC_265075") {}
    void ProcessContract(Contract& c) override {
        FactorDeadline(c, false);
        FactorExpiry(c, false);
        FactorReputationCompletion(c, true);
    }
};
struct Industrial : CommercialKw {  // Industrial.cs:7
    Industrial() : CommercialKw("Industrial", "#autoLOC_7001044", "#autoLOC_265110") {}
};
struct Kerbalminded : CommercialKw {  // Kerbalminded.cs:7
    Kerbalminded() : CommercialKw("Kerbalminded", "#autoLOC_7001045", "#autoLOC_265141") {}
};
struct Moral : Tagged {  // Moral.cs:7
    Moral() : Tagged("Moral", "#autoLOC_7001046", "#autoLOC_265172") {}
    void ProcessContract(Contract& c) override {
        FactorFundsAdvance(c, false);
        FactorFundsCompletion(c, false);
        FactorReputationCompletion(c, true);
    }
};
struct NoRandomContractMentality : Tagged {  // NoRandomContractMentality.cs:7
    NoRandomContractMentality() : Tagged("NoRandomContractMentality", "#autoLOC_7001047", nullptr) {}
    bool CanProcessContract(Contract&) override { return false; }
    KeywordScore ScoreKeyword(const std::string&) override { return KeywordScore::NegativeHigh; }
    void OnAdded() override;
};
struct Patient : CommercialKw {  // Patient.cs:7
    Patient() : CommercialKw("Patient", "#autoLOC_7001048", "#autoLOC_265230") {}
    void ProcessContract(Contract& c) override {
        FactorDeadline(c, true);
        FactorExpiry(c, true);
        FactorReputationCompletion(c, false);
        FactorReputationFailure(c, false);
    }
};
struct Perfectionist : CommercialKw {  // Perfectionist.cs:7
    Perfectionist() : CommercialKw("Perfectionist", "#autoLOC_7001049", "#autoLOC_265266") {}
};
struct Pioneer : CommercialKw {  // Pioneer.cs:7
    Pioneer() : CommercialKw("Pioneer", "#autoLOC_7001050", "#autoLOC_265297") {}
};
struct Record : Tagged {  // Record.cs:7
    Record() : Tagged("Record", "#autoLOC_7001051", "#autoLOC_265328") {}
    bool CanProcessContract(Contract& c) override {
        int count = static_cast<int>(c.KeywordsRequired().size());
        do {
            if (count-- <= 0) return false;
        } while (!(c.KeywordsRequired()[static_cast<size_t>(count)] == "Record"));
        return true;
    }
    KeywordScore ScoreKeyword(const std::string& keyword) override { return keyword == "Record" ? KeywordScore::Positive : KeywordScore::NegativeHigh; }
};
struct Scientific : Tagged {  // Scientific.cs:7
    Scientific() : Tagged("Scientific", "#autoLOC_7001052", "#autoLOC_265367") {}
    KeywordScore ScoreKeyword(const std::string& keyword) override { return keyword == "Scientific" ? KeywordScore::Positive : KeywordScore::None; }
    bool CanProcessContract(Contract& c) override {
        const auto& k = c.Keywords();
        return std::find(k.begin(), k.end(), "Record") == k.end();
    }
};
struct Startup : CommercialKw {  // Startup.cs:7
    Startup() : CommercialKw("Startup", "#autoLOC_7001053", "#autoLOC_265398") {}
    void ProcessContract(Contract& c) override {
        FactorFundsAdvance(c, false);
        FactorFundsCompletion(c, false);
        FactorExpiry(c, false);
        FactorDeadline(c, false);
    }
};
struct Stern : CommercialKw {  // Stern.cs:7
    Stern() : CommercialKw("Stern", "#autoLOC_7001054", "#autoLOC_265434") {}
    void ProcessContract(Contract& c) override {
        FactorReputationFailure(c, true);
        FactorReputationCompletion(c, true);
    }
};

// NoRandomContractMentality.cs:25 OnAdded
void NoRandomContractMentality::OnAdded() {
    auto& list = GetAgent()->Mentality();
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        if (list[static_cast<size_t>(count)].get() != this) list.erase(list.begin() + count);
    }
    AgentMentality::OnAdded();
}

using Creator = std::function<std::unique_ptr<AgentMentality>()>;
const std::vector<std::pair<std::string, Creator>>& Registry() {
    static const std::vector<std::pair<std::string, Creator>> r = {
        {"Commercial", [] { return std::make_unique<Commercial>(); }},
        {"Competitive", [] { return std::make_unique<Competitive>(); }},
        {"Conglomerate", [] { return std::make_unique<Conglomerate>(); }},
        {"Cooperative", [] { return std::make_unique<Cooperative>(); }},
        {"EasyGoing", [] { return std::make_unique<EasyGoing>(); }},
        {"Ecomental", [] { return std::make_unique<Ecomental>(); }},
        {"Economic", [] { return std::make_unique<Economic>(); }},
        {"Hasty", [] { return std::make_unique<Hasty>(); }},
        {"Industrial", [] { return std::make_unique<Industrial>(); }},
        {"Kerbalminded", [] { return std::make_unique<Kerbalminded>(); }},
        {"Moral", [] { return std::make_unique<Moral>(); }},
        {"NoRandomContractMentality", [] { return std::make_unique<NoRandomContractMentality>(); }},
        {"Patient", [] { return std::make_unique<Patient>(); }},
        {"Perfectionist", [] { return std::make_unique<Perfectionist>(); }},
        {"Pioneer", [] { return std::make_unique<Pioneer>(); }},
        {"Record", [] { return std::make_unique<Record>(); }},
        {"Scientific", [] { return std::make_unique<Scientific>(); }},
        {"Startup", [] { return std::make_unique<Startup>(); }},
        {"Stern", [] { return std::make_unique<Stern>(); }}};
    return r;
}

}  // namespace

// AgentMentality.cs:90
AgentMentality* AgentMentality::AddMentality(Agent& agent, const std::string& mentalityName, float factor) {
    const Creator* creator = nullptr;
    for (const auto& kv : Registry())
        if (kv.first == mentalityName) creator = &kv.second;
    if (creator == nullptr) {
        log::Error("[Agent]: Cannot load mentality, no mentality with name '" + mentalityName + "'.");
        return nullptr;
    }
    for (size_t num = 0; num < agent.Mentality().size(); ++num) {
        if (agent.Mentality()[num]->TypeName() == mentalityName) {
            log::Error("[Agent]: Cannot load mentality of name '" + mentalityName + "'. Agent already has a mentality of that type.");
            return nullptr;
        }
    }
    std::unique_ptr<AgentMentality> m = (*creator)();
    AgentMentality* raw = m.get();
    raw->agent = &agent;
    raw->factor = Mathf::Clamp(factor, 0.0f, 1.0f);
    agent.Mentality().push_back(std::move(m));
    raw->OnAdded();
    return raw;
}

const std::vector<std::string>& AgentMentality::MentalityTypeNames() {
    static const std::vector<std::string> names = [] {
        std::vector<std::string> n;
        for (const auto& kv : Registry()) n.push_back(kv.first);
        return n;
    }();
    return names;
}

// ---- Agent -----------------------------------------------------------------------------------------------------------

// Agent.cs:88
Agent::Agent(std::string name, std::string title, std::string logoURL, const char* logoScaledURL)
    : name_(std::move(name)), title_(std::move(title)), logoURL_(std::move(logoURL)) {
    (void)logoScaledURL;  // the textures are a seam (header)
}

// Agent.cs:40
std::string Agent::DescriptionRich() {
    std::string text;
    text += Localizer::Format("#autoLOC_264161", {title_});
    text += Localizer::Format("#autoLOC_264163");
    if (!description_.empty()) text = text + "<color=#CCCCCC>  " + description_ + "</color>\n\n\n";
    if (!mentality_.empty()) {
        text += Localizer::Format("#autoLOC_264172");
        for (size_t i = 0; i < mentality_.size(); ++i) {
            AgentMentality& m = *mentality_[i];
            if (!m.Description().empty()) {
                text = text + "<b>" + m.DisplayName() + "</b>\n";
                text = text + "<color=#CCCCCC>  " + m.Description() + "</color>\n\n";
            }
        }
    }
    return text;
}

// Agent.cs:121
bool Agent::CanProcessContract(Contract& contract) {
    if (!contract.KeywordsRequired().empty() && !HasExclusiveKeywords(contract)) return false;
    size_t num = 0;
    while (true) {
        if (num < mentality_.size()) {
            if (mentality_[num]->CanProcessContract(contract)) break;
            ++num;
            continue;
        }
        return false;
    }
    return true;
}

// Agent.cs:145
bool Agent::HasExclusiveKeywords(Contract& contract) {
    if (contract.KeywordsRequired().empty()) return true;
    for (size_t i = 0; i < mentality_.size(); ++i)
        for (size_t j = 0; j < contract.KeywordsRequired().size(); ++j)
            if (static_cast<int>(mentality_[i]->ScoreKeyword(contract.KeywordsRequired()[j])) > static_cast<int>(KeywordScore::None)) return true;
    return false;
}

// Agent.cs:165
int Agent::ScoreAgentSuitability(Contract& contract) {
    int num = 1;
    for (size_t i = 0; i < mentality_.size(); ++i) {
        if (mentality_[i]->CanProcessContract(contract)) {
            for (size_t j = 0; j < contract.Keywords().size(); ++j)
                num = static_cast<int>(num + static_cast<int>(mentality_[i]->ScoreKeyword(contract.Keywords()[j])));
        }
    }
    return num;
}

// Agent.cs:183
bool Agent::ProcessContract(Contract& contract) {
    if (ContractsEnvironment::Current().ignoreAgencyMindsetOnContracts) return true;
    for (size_t i = 0; i < mentality_.size(); ++i)
        if (mentality_[i]->CanProcessContract(contract)) mentality_[i]->ProcessContract(contract);
    return true;
}

// Agent.cs:201
std::string Agent::GetMindsetString() {
    std::string text;
    for (size_t i = 0; i < mentality_.size(); ++i) text += mentality_[i]->TypeName();
    return text;
}

// Agent.cs:212
std::unique_ptr<Agent> Agent::LoadAgent(ConfigNode& node) {
    const std::optional<std::string> value = node.GetValue("name");
    const std::optional<std::string> value2 = node.GetValue("logoURL");
    if (value.has_value() && value2.has_value()) {
        if (AgentList::Instance->GetAgent(*value) != nullptr) {
            log::Error("[Agent]: Cannot load, node of name '" + *value + "' as there is already an agent with that name");
            return nullptr;
        }
        std::optional<std::string> value3 = node.GetValue("title");
        if (!value3.has_value()) log::Error("[Agent]: Is missing its 'title' value");
        const std::optional<std::string> scaled = node.GetValue("logoScaledURL");
        std::unique_ptr<Agent> agent = std::make_unique<Agent>(*value, value3.value_or(std::string()), *value2, scaled.has_value() ? scaled->c_str() : nullptr);
        if (node.HasValue("description")) agent->description_ = *node.GetValue("description");
        AgentMentality::AddMentality(*agent, "Cooperative", 0.5f);
        AgentMentality::AddMentality(*agent, "Competitive", 0.5f);
        const std::vector<std::string> values = node.GetValues("mentality");
        for (size_t i = 0; i < values.size(); ++i) {
            const std::vector<std::string> array = SplitRemoveEmpty(values[i], ", ");
            if (array.size() >= 1 && array.size() <= 2) {
                const std::string mentalityName = Trim(array[0]);
                float result = 0.5f;
                if (array.size() == 2 && !dotnet::TryParseSingle(array[1], result)) {
                    log::Error("[Agent]: Mentality of agent '" + *value + "' has a float value which could not be parsed");
                } else {
                    AgentMentality::AddMentality(*agent, mentalityName, result);
                }
            } else {
                log::Error("[Agent]: Mentality of agent '" + *value + "' should consist a mentalityName and an optional float factor");
            }
        }
        const std::vector<std::string> values2 = node.GetValues("standing");
        for (size_t j = 0; j < values2.size(); ++j) {
            const std::vector<std::string> array2 = SplitRemoveEmpty(values2[j], ",");
            if (array2.size() != 2) {
                log::Error("[Agent]: Standing of agent '" + *value + "' should consist of two comma seperated values; agentName, standingFloat");
                continue;
            }
            const std::string agentName = Trim(array2[0]);
            float result2 = 0.0f;
            if (!dotnet::TryParseSingle(array2[1], result2)) {
                log::Error("[Agent]: Standings of agent '" + *value + "' has a float value which could not be parsed");
            } else {
                agent->standings_.emplace_back(agent.get(), agentName, result2);
            }
        }
        return agent;
    }
    if (!value.has_value()) log::Error("[Agent]: Cannot load, node which does not have a valid 'name' value");
    if (!value2.has_value()) log::Error("[Agent]: Cannot load, node which does not have a valid 'logoURL' value");
    return nullptr;
}

// ---- AgentList -------------------------------------------------------------------------------------------------------

// AgentList.cs:24
AgentList::AgentList() { Instance = this; }
// C++ helper (see AgentList.cs:37)
AgentList::~AgentList() {
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

// AgentList.cs:32
void AgentList::Start() {
    std::vector<ConfigNode*> configNodes = GameDatabase::Instance()->GetConfigNodes("AGENT");
    LoadAgents(configNodes);
}

// AgentList.cs:47
void AgentList::LoadAgents(const std::vector<ConfigNode*>& nodes) {
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::unique_ptr<Agent> agent = Agent::LoadAgent(*nodes[i]);
        if (agent) agencies_.push_back(std::move(agent));
    }
    for (size_t j = 0; j < agencies_.size(); ++j) {
        int count2 = static_cast<int>(agencies_[j]->Standings().size());
        while (count2-- > 0) {
            if (!agencies_[j]->Standings()[static_cast<size_t>(count2)].Link())
                agencies_[j]->Standings().erase(agencies_[j]->Standings().begin() + count2);
        }
    }
    if (agencies_.empty()) agencies_.push_back(std::make_unique<Agent>("Kerbal Space Program", "Kerbal Space Program", "", ""));
    log::Info("[AgentList]: " + std::to_string(agencies_.size()) + " agents parsed and loaded.");
}

// AgentList.cs:77
Agent* AgentList::GetAgent(const std::string& name) {
    for (auto& a : agencies_)
        if (a->Name() == name) return a.get();
    return nullptr;
}

// AgentList.cs:97
Agent* AgentList::GetAgentbyTitle(const std::string& title) {
    for (auto& a : agencies_)
        if (Localizer::Format(a->Title()) == title) return a.get();
    return nullptr;
}

// AgentList.cs:118
Agent* AgentList::GetAgentRandom() {
    return agencies_[static_cast<size_t>(UnityRandom::Global().Range(0, static_cast<int32_t>(agencies_.size())))].get();
}

// AgentList.cs:123
Agent* AgentList::GetSuitableAgentForContract(Contract& contract) {
    std::vector<Agent*> list;
    for (size_t i = 0; i < agencies_.size(); ++i) {
        if (!agencies_[i]->CanProcessContract(contract)) continue;
        const int num = agencies_[i]->ScoreAgentSuitability(contract) * suitabilityFactor;
        if (num > 0)
            for (int j = 0; j < num; ++j) list.push_back(agencies_[i].get());
    }
    if (list.empty()) return nullptr;
    return list[static_cast<size_t>(UnityRandom::Global().Range(0, static_cast<int32_t>(list.size())))];
}

// AgentList.cs:148
Agent* AgentList::GetPartManufacturer(const std::string& manufacturer) {
    if (!manufacturer.empty()) {
        for (size_t i = 0; i < agencies_.size(); ++i) {
            const std::string& n = agencies_[i]->Name();
            if (n == manufacturer || n.find(manufacturer) != std::string::npos || manufacturer.find(n) != std::string::npos) return agencies_[i].get();
        }
    }
    return GetAgent(Localizer::Format("#autoLOC_6002413"));
}

}  // namespace ksp::Contracts::Agents
