// WP-37: Contracts.Agents (Assembly-CSharp/Contracts/Agents/*.cs): Agent, AgentList, AgentMentality, AgentStanding, KeywordScore and the 22
// mentality classes (Contracts/Agents/Mentalities/*.cs). Agents are loaded from the AGENT nodes (GameData/Squad/Agencies/Agents.cfg).
//
// Seams: the logo textures (Agent's constructor loads them through GameDatabase.GetTexture and scales them with TextureScale.Bilinear):
// only the logo URLs are kept. AssemblyLoader.loadedAssemblies.TypeOperation (the mentality type table) is the registration table below.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/contracts/contract.hpp"
#include "ksp/dotnet/enum.hpp"

namespace ksp {
class AvailablePart;
}

namespace ksp::Contracts::Agents {

class Agent;
class AgentList;

// KeywordScore.cs:3
enum class KeywordScore : int { Ignore = -2147483647 - 1, NegativeHigh = -2, Negative = -1, None = 0, Positive = 1, PositiveHigh = 2 };

// AgentStanding.cs:5
class AgentStanding {
public:
    AgentStanding(Agent* parentAgent, std::string agentName, float standing);  // :17
    const std::string& AgentName() const { return agentName_; }                // :11
    Agent* ParentAgent() const { return parentAgent_; }                         // :12
    Agent* GetAgent() const { return agent_; }                                  // :13
    float Standing() const { return standing_; }                                // :14
    bool Link();                                                                // :24

private:
    std::string agentName_;
    Agent* parentAgent_;
    Agent* agent_ = nullptr;
    float standing_;
};

// AgentMentality.cs:7
class AgentMentality {
public:
    virtual ~AgentMentality() = default;
    virtual std::string TypeName() const = 0;     // GetType().Name
    Agent* GetAgent() const { return agent; }     // :14
    float Factor() const { return factor; }       // :16
    std::string Description() { return GetDescription(); }   // :18
    std::string DisplayName() { return GetDisplayName(); }   // :20
    virtual std::string GetDescription() { return std::string(); }  // :49 (null)
    virtual std::string GetDisplayName() { return TypeName(); }     // :54
    virtual KeywordScore ScoreKeyword(const std::string& keyword) { (void)keyword; return KeywordScore::None; }  // :59
    virtual bool CanProcessContract(Contract& contract) { (void)contract; return true; }   // :64
    virtual void ProcessContract(Contract& contract) { (void)contract; }                    // :69
    virtual void OnAdded() {}                                                                // :73
    // :77 AddMentality: nullptr when the type name is unknown or the agent already has that type
    static AgentMentality* AddMentality(Agent& agent, const std::string& mentalityName, float factor);
    // the registered mentality type names (AssemblyLoader order is the registration order)
    static const std::vector<std::string>& MentalityTypeNames();

    // :104-:174 the Factor* helpers
    void FactorFundsAdvance(Contract& contract, bool positive);           // :104
    void FactorFundsCompletion(Contract& contract, bool positive);        // :114
    void FactorFundsFailure(Contract& contract, bool positive);           // :124
    void FactorScienceCompletion(Contract& contract, bool positive);      // :134
    void FactorReputationCompletion(Contract& contract, bool positive);   // :144
    void FactorReputationFailure(Contract& contract, bool positive);      // :154
    void FactorDeadline(Contract& contract, bool positive);               // :164
    void FactorExpiry(Contract& contract, bool positive);                 // :174

    Agent* agent = nullptr;  // :12
    float factor = 0.0f;     // :15
};

// Agent.cs:7
class Agent {
public:
    Agent(std::string name, std::string title, std::string logoURL, const char* logoScaledURL);  // :88
    const std::string& Name() const { return name_; }                 // :43
    const std::string& Title() const { return title_; }               // :45
    const std::string& Description() const { return description_; }   // :47
    std::string DescriptionRich();                                    // :49
    const std::string& LogoURL() const { return logoURL_; }           // :78
    std::vector<std::unique_ptr<AgentMentality>>& Mentality() { return mentality_; }  // :84
    std::vector<AgentStanding>& Standings() { return standings_; }    // :86
    bool CanProcessContract(Contract& contract);                      // :127
    int ScoreAgentSuitability(Contract& contract);                    // :165
    bool ProcessContract(Contract& contract);                         // :183
    std::string GetMindsetString();                                   // :201
    // :212 LoadAgent: nullptr when invalid (errors are logged)
    static std::unique_ptr<Agent> LoadAgent(ConfigNode& node);

private:
    bool HasExclusiveKeywords(Contract& contract);                    // :148
    std::string name_;
    std::string title_;
    std::string description_;
    std::string logoURL_;
    std::vector<std::unique_ptr<AgentMentality>> mentality_;
    std::vector<AgentStanding> standings_;
};

// AgentList.cs:7
class AgentList {
public:
    AgentList();                                  // :27 Awake: Instance = this
    ~AgentList();                                 // :37 OnDestroy
    static inline AgentList* Instance = nullptr;  // :21
    int LogoWidth() const { return logoWidth_; }
    int LogoHeight() const { return logoHeight_; }
    std::vector<std::unique_ptr<Agent>>& Agencies() { return agencies_; }  // :23
    void Start();                                 // :32: GameDatabase.Instance.GetConfigNodes("AGENT") -> LoadAgents
    void LoadAgents(const std::vector<ConfigNode*>& nodes);   // :44
    Agent* GetAgent(const std::string& name);                 // :81
    Agent* GetAgentbyTitle(const std::string& title);         // :100
    Agent* GetAgentRandom();                                  // :118
    Agent* GetSuitableAgentForContract(Contract& contract);   // :123
    Agent* GetPartManufacturer(const std::string& manufacturer);  // :152 (AvailablePart.manufacturer)

private:
    int logoWidth_ = 64;
    int logoHeight_ = 40;
    std::vector<std::unique_ptr<Agent>> agencies_;
    static inline int suitabilityFactor = 2;      // :19
};

}  // namespace ksp::Contracts::Agents

template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Agents::KeywordScore> {
    using E = ksp::Contracts::Agents::KeywordScore;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Ignore, "Ignore"},     {E::NegativeHigh, "NegativeHigh"}, {E::Negative, "Negative"},
                                               {E::None, "None"},         {E::Positive, "Positive"},         {E::PositiveHigh, "PositiveHigh"}};
};
