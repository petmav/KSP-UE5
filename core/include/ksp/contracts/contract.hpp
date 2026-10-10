// WP-37: Contracts.Contract, ContractParameter, ContractPredicate, IContractParameterHost, ParameterState, BodyLocation
// (Assembly-CSharp/Contracts/*.cs). The contract state machine (Generated / Offered / OfferExpired / Declined / Cancelled / Active /
// Completed / DeadlineExpired / Failed / Withdrawn), deadlines and expiry, the reward / penalty set-up through GameVariables, the
// CONTRACT / PARAM ConfigNode format, the parameter tree and the seeded generation entry point (Contract.Generate).
//
// C# `protected` members are public here (the templates, the system and the tests drive them); every ported function carries the
// declaration line of the C# member. Overridden members keep the C# names. The C# type name of a class (`GetType().Name`) is the
// virtual TypeName().
//
// Seams (ContractsEnvironment): clock, Guid.NewGuid, DateTime ticks, MessageSystem, analytics (dropped), the rich-text of the
// rewards messages (MessageAdvances / MessageRewards / MessageFailurePenalties / MessageCancellationPenalties /
// MissionControlTextRich are mission control GUI text: WP-46; the virtuals return "" until a seam installs them).
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <typeinfo>
#include <vector>

#include "ksp/career/career.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/events/event_data.hpp"
#include "ksp/events/game_events.hpp"

namespace ksp {
class CelestialBody;
class Vessel;
class ProtoVessel;
}  // namespace ksp

namespace ksp::Contracts {

namespace Agents {
class Agent;
}

// ParameterState.cs:3
enum class ParameterState : int { Incomplete, Complete, Failed };
// BodyLocation.cs:3
enum class BodyLocation : int { Space, Surface };

class Contract;
class ContractParameter;
class ContractPredicate;

// IContractParameterHost.cs:5
class IContractParameterHost {
public:
    virtual ~IContractParameterHost() = default;
    virtual std::string Title() = 0;                                  // :7
    virtual IContractParameterHost* Parent() = 0;                      // :9
    virtual Contract* Root() = 0;                                      // :11
    virtual int ParameterCount() const = 0;                            // :13
    // :19 (the host takes ownership)
    virtual ContractParameter* AddParameter(std::unique_ptr<ContractParameter> parameter, const std::string& id = std::string()) = 0;
    virtual ContractParameter* GetParameter(int index) = 0;            // :23
    virtual ContractParameter* GetParameter(const std::string& id) = 0;  // :23
    virtual ContractParameter* GetParameterOfType(const std::string& typeName) = 0;  // (C++ addition) GetParameter(Type)
    virtual void RemoveParameter(int index) = 0;                       // :31
    virtual void RemoveParameter(const std::string& id) = 0;           // :31
    virtual void RemoveParameterOfType(const std::string& typeName) = 0;  // (C++ addition)
    virtual void RemoveParameter(ContractParameter* parameter) = 0;    // :35
    virtual void ParameterStateUpdate(ContractParameter* p) = 0;       // :39
};

// ContractParameter.cs:9
class ContractParameter : public IContractParameterHost {
public:
    ContractParameter();
    ~ContractParameter() override;
    ContractParameter(const ContractParameter&) = delete;
    ContractParameter& operator=(const ContractParameter&) = delete;

    // GetType().Name
    virtual std::string TypeName() const { return "ContractParameter"; }

    double FundsCompletion = 0.0;                 // :37
    double FundsFailure = 0.0;                    // :39
    float ScienceCompletion = 0.0f;               // :41
    float ReputationCompletion = 0.0f;            // :43
    float ReputationFailure = 0.0f;               // :45
    events::EventData<ContractParameter*, ParameterState> OnStateChange{"OnStateChange"};  // :47

    IContractParameterHost* Parent() override { return parent_; }   // :49
    Contract* Root() override { return root_; }                      // :51
    ParameterState State() const { return state; }                   // :53
    const std::string& ID() const { return id; }                     // :55
    void SetID(const std::string& v) { id = v; }
    bool DisableOnStateChange() const { return disableOnStateChange; }  // :67
    void SetDisableOnStateChange(bool v) { disableOnStateChange = v; }
    bool Optional() const { return optional; }                       // :79
    void SetOptional(bool v) { optional = v; }
    bool AllowPartialFailure() const { return allowPartialFailure; } // :91
    void SetAllowPartialFailure(bool v) { allowPartialFailure = v; }
    bool Enabled() const { return enabled; }                          // :103
    void SetEnabled(bool value);                                      // (C++ addition) setter
    int ParameterCount() const override { return static_cast<int>(parameters_.size()); }  // :122
    std::string HashString();                                         // :130
    std::string Title() override { return GetTitle(); }               // :149
    std::string Notes() { return GetNotes(); }                        // :151
    std::string MessageComplete() { return GetMessageComplete(); }    // :153
    std::string MessageFailed() { return GetMessageFailed(); }        // :155
    std::string MessageIncomplete() { return GetMessageIncomplete(); }  // :157
    // :154 AllParameters: this, then the children depth first
    void AllParameters(std::vector<ContractParameter*>& out);

    void NestToParent(IContractParameterHost* parent);                // :179
    void Enable();                                                    // :185
    void Disable(bool recursive = false);                             // :194
    void ParameterStateUpdate(ContractParameter* p) override;         // :212
    ContractParameter* AddParameter(std::unique_ptr<ContractParameter> parameter, const std::string& id = std::string()) override;  // :219
    ContractParameter* GetParameter(int index) override;              // :230
    ContractParameter* GetParameter(const std::string& id) override;  // :230
    ContractParameter* GetParameterOfType(const std::string& typeName) override;  // (C++ addition)
    // :266 GetParameter<T>(id = null): the first child with this id that is a T
    template <class T>
    T* GetParameterT(const std::string& id = std::string()) {
        for (auto& p : parameters_)
            if (p->ID() == id && dynamic_cast<T*>(p.get()) != nullptr) return static_cast<T*>(p.get());
        return nullptr;
    }
    void RemoveParameter(int index) override;                         // :295
    void RemoveParameter(const std::string& id) override;             // :295
    void RemoveParameterOfType(const std::string& typeName) override;  // (C++ addition)
    void RemoveParameter(ContractParameter* parameter) override;      // :340
    virtual void OnParameterStateChange(ContractParameter* p) { (void)p; }  // :345
    bool AllChildParametersComplete();                                // :349
    bool AnyChildParametersFailed();                                  // :363
    void SetFunds(float completion, CelestialBody* body = nullptr);   // :377
    void SetFunds(float completion, float failure, CelestialBody* body = nullptr);  // :377
    void SetReputation(float completion, float failure, CelestialBody* body = nullptr);  // :398
    void SetReputation(float completion, CelestialBody* body = nullptr);  // :398
    void SetScience(float completion, CelestialBody* body = nullptr);  // :419

    virtual void AwardCompletion();                                   // :434
    virtual void PenalizeFailure();                                   // :475
    void Load(ConfigNode& node);                                      // :513
    void Save(ConfigNode& node);                                      // :576
    void Register();                                                  // :612
    void Unregister();                                                // :625
    void Update();                                                    // :635
    void Reset();                                                     // :655
    long long CreateID();                                             // :722

    // overridables (protected virtuals of the C# class)
    virtual std::string GetHashString() { return ""; }                // :668
    virtual std::string GetTitle();                                   // :673
    virtual std::string GetNotes() { return ""; }                     // :678 (null)
    virtual std::string GetMessageComplete();                         // :683
    virtual std::string GetMessageFailed();                           // :688
    virtual std::string GetMessageIncomplete();                       // :693
    virtual void OnLoad(ConfigNode& node) { (void)node; }             // :698
    virtual void OnSave(ConfigNode& node) { (void)node; }             // :702
    virtual void OnRegister() {}                                      // :706
    virtual void OnUnregister() {}                                    // :710
    virtual void OnUpdate() {}                                        // :714
    virtual void OnReset() {}                                         // :718

    void SetComplete();                                               // :744
    void SetIncomplete();                                             // :772
    void SetFailed();                                                 // :791
    void SendStateMessage(const std::string& title, const std::string& message);  // :832

    // protected fields of the C#
    ParameterState state = ParameterState::Incomplete;                // :18
    std::string id;                                                   // :21
    bool disableOnStateChange = true;                                 // :24
    bool optional = false;                                            // :27
    bool allowPartialFailure = false;                                 // :30
    bool enabled = true;                                              // :32

private:
    float GetDestinationWeight(CelestialBody* body);                  // :429
    IContractParameterHost* parent_ = nullptr;                        // (C++ addition)
    Contract* root_ = nullptr;                                        // (C++ addition)
    std::vector<std::unique_ptr<ContractParameter>> parameters_;      // (C++ addition)
};

// ContractPredicate.cs:7
class ContractPredicate {
public:
    explicit ContractPredicate(IContractParameterHost* parent);
    virtual ~ContractPredicate() = default;
    IContractParameterHost* Parent() const { return parent_; }       // :11
    Contract* Root() const { return root_; }                          // :14
    std::string Description() { return GetDescription(); }            // :16
    bool AllowMultiple() { return GetAllowMultiple(); }               // :18
    virtual std::string TypeName() const { return "ContractPredicate"; }
    void Load(ConfigNode& node) { OnLoad(node); }                     // :26
    void Save(ConfigNode& node) { OnSave(node); }                     // :31
    virtual bool Test(Vessel* vessel);                                // :36
    virtual bool Test(ProtoVessel* vessel);                           // :41
    void Update() { OnUpdate(); }                                     // :64
    virtual std::string GetDescription();                             // :46
    virtual bool GetAllowMultiple() { return false; }                 // :51
    virtual void OnLoad(ConfigNode& node) { (void)node; }             // :56 (the C# declares OnLoad / OnSave virtual at :58 / :60)
    virtual void OnSave(ConfigNode& node) { (void)node; }
    virtual void OnUpdate() {}                                        // :69

protected:
    IContractParameterHost* parent_;
    Contract* root_;
};

// Contract.cs:12
class Contract : public IContractParameterHost {
public:
    using ContractPrestige = ksp::ContractPrestige;  // :14 (declared by WP-36 in career_environment.hpp)

    enum class State : int { Generated, Offered, OfferExpired, Declined, Cancelled, Active, Completed, DeadlineExpired, Failed, Withdrawn };  // :24
    enum class Viewed : int { Unseen, Seen, Read };                  // :48
    enum class DeadlineType : int { Fixed, Floating, None };          // :55
    enum class ProgressState : int { Unreached, Reached, Incomplete, Complete };  // :62

    Contract();
    ~Contract() override;
    Contract(const Contract&) = delete;
    Contract& operator=(const Contract&) = delete;

    static inline int contractsInExistance = 0;                       // :139 (private set)

    virtual std::string TypeName() const { return "Contract"; }       // GetType().Name

    double TimeExpiry = 0.0;                                          // :108
    double TimeDeadline = 0.0;                                        // :110
    double FundsAdvance = 0.0;                                        // :112
    double FundsCompletion = 0.0;                                     // :114
    double FundsFailure = 0.0;                                        // :116
    float ScienceCompletion = 0.0f;                                   // :118
    float ReputationCompletion = 0.0f;                                // :120
    float ReputationFailure = 0.0f;                                   // :122
    bool AutoAccept = false;                                          // :124
    bool IgnoresWeight = false;                                       // :126
    events::EventData<State> OnStateChange{"OnStateChange"};          // :135
    events::EventData<Viewed> OnViewedChange{"OnViewedChange"};       // :137

    ContractPrestige Prestige() const { return prestige; }            // :141
    std::string Title() override { return GetTitle(); }               // :143
    std::string Synopsys() { return GetSynopsys(); }                  // :145
    std::string Description() { return GetDescription(); }            // :155
    std::string Notes() { return GetNotes(); }                        // :149
    Agents::Agent* Agent() const { return agent; }                    // :151
    State ContractState() const { return state; }                     // :153
    std::string LocalizedContractState() const;                       // :155
    Viewed ContractViewed() const { return viewed; }                  // :157
    double DateExpire() const { return dateExpire; }                  // :159
    double DateAccepted() const { return dateAccepted; }              // :161
    double DateDeadline() const { return dateDeadline; }              // :163
    double DateFinished() const { return dateFinished; }              // :165
    long long ContractID() const { return contractID; }               // :167
    const Guid& ContractGuid() const { return contractGuid; }         // :169
    int MissionSeed() const { return missionSeed; }                   // :171
    std::vector<std::string>& Keywords() { return keywords; }         // :173
    std::vector<std::string>& KeywordsRequired() { return keywordsRequired; }  // :175
    IContractParameterHost* Parent() override { return nullptr; }     // :177
    Contract* Root() override { return this; }                        // :179
    int ParameterCount() const override { return static_cast<int>(parameters_.size()); }  // :181
    // :199 AllParameters: the parameters' trees depth first (each parameter first, then its children)
    void AllParameters(std::vector<ContractParameter*>& out);
    static double GameTime();                                         // :208

    // :237 / :242 / :247 (UnityEngine.Random draws on the process-wide generator)
    void SetExpiry();
    void SetExpiry(int minDays, int maxDays);
    void SetExpiry(float minDays, float maxDays);
    void SetDeadlineDays(float days, CelestialBody* body = nullptr);   // :245
    void SetDeadlineYears(float years, CelestialBody* body = nullptr); // :255
    void SetFunds(float advance, float completion, CelestialBody* body = nullptr);  // :277
    void SetFunds(float advance, float completion, float failure, CelestialBody* body = nullptr);  // :277
    void SetReputation(float completion, float failure, CelestialBody* body = nullptr);  // :289
    void SetReputation(float completion, CelestialBody* body = nullptr);  // :300
    void SetScience(float completion, CelestialBody* body = nullptr); // :310
    float GetDestinationWeight(CelestialBody* body);                  // :320
    bool AddKeywords(const std::vector<std::string>& keywords);       // :325
    bool AddKeywordsRequired(const std::vector<std::string>& keywords);  // :350

    ContractParameter* AddParameter(std::unique_ptr<ContractParameter> parameter, const std::string& id = std::string()) override;  // :375
    void ParameterStateUpdate(ContractParameter* p) override { OnParameterStateChange(p); }  // :386
    ContractParameter* GetParameter(int index) override;              // :391
    ContractParameter* GetParameter(const std::string& id) override;  // :396
    ContractParameter* GetParameterOfType(const std::string& typeName) override;  // (C++ addition)
    // :432 GetParameter<T>(id = null): id null = the first T
    template <class T>
    T* GetParameterT(const std::optional<std::string>& id = std::nullopt) {
        for (auto& p : parameters_) {
            if (dynamic_cast<T*>(p.get()) != nullptr) {
                if (!id.has_value()) return static_cast<T*>(p.get());
                if (*id == p->ID()) return static_cast<T*>(p.get());
            }
        }
        return nullptr;
    }
    void RemoveParameter(int index) override;                         // :463
    void RemoveParameter(const std::string& id) override;             // :463
    void RemoveParameterOfType(const std::string& typeName) override;  // (C++ addition)
    void RemoveParameter(ContractParameter* parameter) override;      // :489
    virtual void OnParameterStateChange(ContractParameter* p);        // :516
    virtual bool CanBeCancelled() { return true; }                    // :572
    virtual bool CanBeDeclined() { return true; }                     // :577
    virtual bool CanBeFailed() { return true; }                       // :582
    virtual bool MeetRequirements() { return false; }                 // :587
    virtual std::vector<CelestialBody*>* GetWeightBodies() { return nullptr; }  // :592 (a fresh list in the C#; the pointer is owned by the caller: see WeightBodies())
    virtual std::vector<CelestialBody*> WeightBodies() { return {}; }  // GetWeightBodies() result; empty + hasWeightBodies=false = null
    virtual bool HasWeightBodies() { return false; }
    virtual std::string GetSynopsys() { return ""; }                  // :597
    virtual std::string GetDescription() { return ""; }               // :602
    virtual std::string GetTitle() { return ""; }                     // :607 (null)
    virtual std::string GetNotes() { return ""; }                     // :612 (null)
    virtual bool Generate() { return false; }                         // :617
    virtual void OnRegister() {}                                      // :622
    virtual void OnUnregister() {}                                    // :626
    virtual void OnUpdate() {}                                        // :630
    virtual void OnReset() {}                                         // :634
    virtual void OnLoad(ConfigNode& node) { (void)node; }             // :638
    virtual void OnSave(ConfigNode& node) { (void)node; }             // :642
    virtual void AwardAdvance();                                      // :646
    virtual void AwardCompletion();                                   // :658
    virtual void PenalizeCancellation();                              // :686
    virtual void PenalizeFailure();                                   // :714
    virtual void OnOffered() {}                                       // :734
    virtual void OnOfferExpired() {}                                  // :738
    virtual void OnAccepted() {}                                      // :742
    virtual void OnCancelled() {}                                     // :746
    virtual void OnCompleted() {}                                     // :750
    virtual void OnDeclined() {}                                      // :754
    virtual void OnFailed() {}                                        // :758
    virtual void OnDeadlineExpired() {}                               // :762
    virtual void OnFinished() {}                                      // :766
    virtual void OnGenerateFailed() {}                                // :770
    virtual void OnWithdrawn() {}                                     // :774
    virtual void OnSeen() {}                                          // :778
    virtual void OnRead() {}                                          // :782
    virtual std::string GetHashString() { return ""; }                // :786
    virtual std::string MessageOffered();                             // :791
    virtual std::string MessageOfferExpired();                        // :796
    virtual std::string MessageCancelled();                           // :801
    virtual std::string MessageAccepted();                            // :806
    virtual std::string MessageDeadlineExpired();                     // :811
    virtual std::string MessageFailed();                              // :816
    virtual std::string MessageCompleted();                           // :821
    // mission control rich text (WP-46 GUI seam): "" unless installed through ContractRichText
    virtual std::string MessageAdvances();                            // :826
    virtual std::string MessageRewards();                             // :853
    virtual std::string MessageFailurePenalties();                    // :888
    virtual std::string MessageCancellationPenalties(double fundsPenalty, float repPenalty);  // :919
    virtual std::string MissionControlTextRich();                     // :950

    void SendStateMessage(const std::string& title, const std::string& message);  // :1213 (colour / icon are GUI)
    bool Offer();                                                     // :1222
    bool Accept();                                                    // :1240
    bool Decline();                                                   // :1255
    bool Cancel();                                                    // :1270
    void Update();                                                    // :1281
    void Reset();                                                     // :1330
    void Withdraw();                                                  // :1343
    bool Fail();                                                      // :1351
    bool Complete();                                                  // :1362
    void SetState(State newState);                                    // :1373
    bool IsFinished();                                                // :1476
    void Register();                                                  // :1486
    void Unregister();                                                // :1496
    void Kill();                                                      // :1506
    void SetViewed(Viewed viewed);                                    // :1513

    // :1531 Generate(Type, difficulty, seed, state): the contract is created through the ContractSystem type table
    static std::unique_ptr<Contract> Generate(const std::string& contractTypeName, ContractPrestige difficulty, int seed, State state);
    // :1566 Load(contract, node)
    static bool Load(Contract& contract, ConfigNode& node);
    void Save(ConfigNode& node);                                      // :1702
    void GenerateFailed() { OnGenerateFailed(); }                     // :1734

    // :1779-1981 body lists (PSystemManager.Instance.localBodies = SaveEnvironment::Current().localBodies)
    static std::vector<CelestialBody*> GetBodies(bool includeKerbin, bool includeSun);
    static std::vector<CelestialBody*> GetBodies_Reached(bool includeKerbin, bool includeSun);
    static std::vector<CelestialBody*> GetBodies_NotReached(bool includeKerbin, bool includeSun);
    static std::vector<CelestialBody*> GetBodies_InComplete(bool includeKerbin, bool includeSun, const std::string& notComplete);
    static std::vector<CelestialBody*> GetBodies_Complete(bool includeKerbin, bool includeSun, const std::string& complete);
    static int CountBodies_Complete(bool includeKerbin, bool includeSun, const std::string& nodeComplete);
    static int CountBodies_Reached(bool includeKerbin, bool includeSun);
    static std::vector<CelestialBody*> GetBodies(const std::string& nodeName, ProgressState nodeState, const std::function<bool(CelestialBody*)>& where = nullptr);
    static std::vector<CelestialBody*> GetBodies(ProgressState bodyState, const std::function<bool(CelestialBody*)>& where = nullptr);
    static std::vector<CelestialBody*> GetBodies(ProgressState bodyState, const std::string& nodeName, ProgressState nodeState, const std::function<bool(CelestialBody*)>& where = nullptr);
    static std::vector<CelestialBody*> GetBodies_NextUnreached(int depth, const std::function<bool(CelestialBody*)>& where = nullptr);
    // :1997 WeightedBodyChoice(bodies, generator): generator = nullptr -> UnityEngine.Random, else a KSPRandom
    CelestialBody* WeightedBodyChoice(const std::vector<CelestialBody*>& bodies, KSPRandom* generator = nullptr);

    // protected fields of the C#
    ContractPrestige prestige = ContractPrestige::Trivial;           // :71
    DeadlineType expiryType = DeadlineType::Floating;                 // :74
    DeadlineType deadlineType = DeadlineType::Floating;               // :77
    Agents::Agent* agent = nullptr;                                   // :80
    State state = State::Generated;                                   // :83
    Viewed viewed = Viewed::Unseen;                                   // :86
    double dateExpire = 0.0;                                          // :89
    double dateAccepted = 0.0;                                        // :92
    double dateDeadline = 0.0;                                        // :95
    double dateFinished = 0.0;                                        // :98
    long long contractID = 0;                                         // :101
    Guid contractGuid;                                                // :103
    int missionSeed = 0;                                              // :106

private:
    void SetupSeed(int seed = -1);                                    // :1180
    void SetupID();                                                   // :1190
    std::string StateMsgAddition(const std::string& addition);       // :1467
    static void AddBodyNodes(ProgressState nodeState, const std::string& nodeName, std::vector<CelestialBody*>& bodies,
                             KSPAchievements::CelestialBodySubtree* tree, const std::function<bool(CelestialBody*)>& where);
    static void AddBodyTrees(ProgressState bodyState, std::vector<KSPAchievements::CelestialBodySubtree*>& bodies,
                             KSPAchievements::CelestialBodySubtree* tree);
    static void AddBodyTrees(ProgressState bodyState, const std::string& nodeName, ProgressState nodeState,
                             std::vector<KSPAchievements::CelestialBodySubtree*>& bodies, KSPAchievements::CelestialBodySubtree* tree);
    static void AddNextUnreachedBodyTrees(int& depth, std::vector<KSPAchievements::CelestialBodySubtree*>& bodies,
                                          KSPAchievements::CelestialBodySubtree* tree, const std::function<bool(CelestialBody*)>& where);

    std::vector<std::string> keywords;                                // :128
    std::vector<std::string> keywordsRequired;                        // :130
    std::vector<std::unique_ptr<ContractParameter>> parameters_;      // (C++ addition)
};

// Rich-text seam for the mission control texts (WP-46). kind: "advances", "rewards", "failure", "cancellation", "mission".
struct ContractRichText {
    static inline std::function<std::string(Contract&, const std::string& kind, double fundsPenalty, float repPenalty)> provider;
};

}  // namespace ksp::Contracts

template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::ParameterState> {
    static constexpr bool flags = false;
    static constexpr EnumMember<ksp::Contracts::ParameterState> members[] = {
        {ksp::Contracts::ParameterState::Incomplete, "Incomplete", "#autoLOC_7001027"},
        {ksp::Contracts::ParameterState::Complete, "Complete", "#autoLOC_7001028"},
        {ksp::Contracts::ParameterState::Failed, "Failed", "#autoLOC_7001029"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::BodyLocation> {
    static constexpr bool flags = false;
    static constexpr EnumMember<ksp::Contracts::BodyLocation> members[] = {
        {ksp::Contracts::BodyLocation::Space, "Space"}, {ksp::Contracts::BodyLocation::Surface, "Surface"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::ContractPrestige> {
    static constexpr bool flags = false;
    static constexpr EnumMember<ksp::ContractPrestige> members[] = {
        {ksp::ContractPrestige::Trivial, "Trivial", "#autoLOC_7001024"},
        {ksp::ContractPrestige::Significant, "Significant", "#autoLOC_7001025"},
        {ksp::ContractPrestige::Exceptional, "Exceptional", "#autoLOC_7001026"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Contract::State> {
    using E = ksp::Contracts::Contract::State;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        {E::Generated, "Generated", "#autoLOC_900713"},       {E::Offered, "Offered", "#autoLOC_900337"},
        {E::OfferExpired, "OfferExpired", "#autoLOC_900714"}, {E::Declined, "Declined", "#autoLOC_900716"},
        {E::Cancelled, "Cancelled", "#autoLOC_900711"},       {E::Active, "Active", "#autoLOC_900336"},
        {E::Completed, "Completed", "#autoLOC_900710"},       {E::DeadlineExpired, "DeadlineExpired", "#autoLOC_900715"},
        {E::Failed, "Failed", "#autoLOC_900708"},             {E::Withdrawn, "Withdrawn", "#autoLOC_8003155"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Contract::Viewed> {
    using E = ksp::Contracts::Contract::Viewed;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Unseen, "Unseen"}, {E::Seen, "Seen"}, {E::Read, "Read"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Contract::DeadlineType> {
    using E = ksp::Contracts::Contract::DeadlineType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Fixed, "Fixed"}, {E::Floating, "Floating"}, {E::None, "None"}};
};
