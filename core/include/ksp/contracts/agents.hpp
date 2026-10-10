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
    AgentStanding(Agent* parentAgent, std::string agentName, float standing);  // :27
    const std::string& AgentName() const { return agentName_; }                // :19
    Agent* ParentAgent() const { return parentAgent_; }                         // :21
    Agent* GetAgent() const { return agent_; }                                  // (C++ addition)
    float Standing() const { return standing_; }                                // :25
    bool Link();                                                                // :34

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
    Agent* GetAgent() const { return agent; }     // (C++ addition)
    float Factor() const { return factor; }       // :19
    std::string Description() { return GetDescription(); }   // :21
    std::string DisplayName() { return GetDisplayName(); }   // :23
    virtual std::string GetDescription() { return std::string(); }  // :62 (null)
    virtual std::string GetDisplayName() { return TypeName(); }     // :67
    virtual KeywordScore ScoreKeyword(const std::string& keyword) { (void)keyword; return KeywordScore::None; }  // :72
    virtual bool CanProcessContract(Contract& contract) { (void)contract; return true; }   // :77
    virtual void ProcessContract(Contract& contract) { (void)contract; }                    // :82
    virtual void OnAdded() {}                                                                // :86
    // :77 AddMentality: nullptr when the type name is unknown or the agent already has that type
    static AgentMentality* AddMentality(Agent& agent, const std::string& mentalityName, float factor);
    // the registered mentality type names (AssemblyLoader order is the registration order)
    static const std::vector<std::string>& MentalityTypeNames();

    // :104-:174 the Factor* helpers
    void FactorFundsAdvance(Contract& contract, bool positive);           // :122
    void FactorFundsCompletion(Contract& contract, bool positive);        // :132
    void FactorFundsFailure(Contract& contract, bool positive);           // :142
    void FactorScienceCompletion(Contract& contract, bool positive);      // :152
    void FactorReputationCompletion(Contract& contract, bool positive);   // :162
    void FactorReputationFailure(Contract& contract, bool positive);      // :172
    void FactorDeadline(Contract& contract, bool positive);               // :182
    void FactorExpiry(Contract& contract, bool positive);                 // :192

    Agent* agent = nullptr;  // :12
    float factor = 0.0f;     // :15
};

// Agent.cs:7
class Agent {
public:
    Agent(std::string name, std::string title, std::string logoURL, const char* logoScaledURL);  // :79
    const std::string& Name() const { return name_; }                 // :34
    const std::string& Title() const { return title_; }               // :36
    const std::string& Description() const { return description_; }   // :38
    std::string DescriptionRich();                                    // :40
    const std::string& LogoURL() const { return logoURL_; }           // :69
    std::vector<std::unique_ptr<AgentMentality>>& Mentality() { return mentality_; }  // :75
    std::vector<AgentStanding>& Standings() { return standings_; }    // :77
    bool CanProcessContract(Contract& contract);                      // :121
    int ScoreAgentSuitability(Contract& contract);                    // :165
    bool ProcessContract(Contract& contract);                         // :182
    std::string GetMindsetString();                                   // :199
    // :212 LoadAgent: nullptr when invalid (errors are logged)
    static std::unique_ptr<Agent> LoadAgent(ConfigNode& node);

private:
    bool HasExclusiveKeywords(Contract& contract);                    // :145
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
    AgentList();                                  // (C++ addition) Awake: Instance = this
    ~AgentList();                                 // (C++ addition) OnDestroy
    static inline AgentList* Instance = nullptr;  // :24
    int LogoWidth() const { return logoWidth_; }
    int LogoHeight() const { return logoHeight_; }
    std::vector<std::unique_ptr<Agent>>& Agencies() { return agencies_; }  // :26
    void Start();                                 // :33: GameDatabase.Instance.GetConfigNodes("AGENT") -> LoadAgents
    void LoadAgents(const std::vector<ConfigNode*>& nodes);   // :47
    Agent* GetAgent(const std::string& name);                 // :77
    Agent* GetAgentbyTitle(const std::string& title);         // :97
    Agent* GetAgentRandom();                                  // :117
    Agent* GetSuitableAgentForContract(Contract& contract);   // :122
    Agent* GetPartManufacturer(const std::string& manufacturer);  // :148 (AvailablePart.manufacturer)

private:
    int logoWidth_ = 64;
    int logoHeight_ = 40;
    std::vector<std::unique_ptr<Agent>> agencies_;
    static inline int suitabilityFactor = 2;      // :18
};

}  // namespace ksp::Contracts::Agents

template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Agents::KeywordScore> {
    using E = ksp::Contracts::Agents::KeywordScore;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Ignore, "Ignore"},     {E::NegativeHigh, "NegativeHigh"}, {E::Negative, "Negative"},
                                               {E::None, "None"},         {E::Positive, "Positive"},         {E::PositiveHigh, "PositiveHigh"}};
};
