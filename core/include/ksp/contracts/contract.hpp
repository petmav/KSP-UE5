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
    virtual ContractParameter* GetParameter(int index) = 0;            // :21
    virtual ContractParameter* GetParameter(const std::string& id) = 0;  // :23
    virtual ContractParameter* GetParameterOfType(const std::string& typeName) = 0;  // :25 GetParameter(Type)
    virtual void RemoveParameter(int index) = 0;                       // :29
    virtual void RemoveParameter(const std::string& id) = 0;           // :31
    virtual void RemoveParameterOfType(const std::string& typeName) = 0;  // :33
    virtual void RemoveParameter(ContractParameter* parameter) = 0;    // :35
    virtual void ParameterStateUpdate(ContractParameter* p) = 0;       // :37
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

    double FundsCompletion = 0.0;                 // :31
    double FundsFailure = 0.0;                    // :33
    float ScienceCompletion = 0.0f;               // :35
    float ReputationCompletion = 0.0f;            // :37
    float ReputationFailure = 0.0f;               // :39
    events::EventData<ContractParameter*, ParameterState> OnStateChange{"OnStateChange"};  // :41

    IContractParameterHost* Parent() override { return parent_; }   // :43
    Contract* Root() override { return root_; }                      // :45
    ParameterState State() const { return state; }                   // :47
    const std::string& ID() const { return id; }                     // :49
    void SetID(const std::string& v) { id = v; }
    bool DisableOnStateChange() const { return disableOnStateChange; }  // :63
    void SetDisableOnStateChange(bool v) { disableOnStateChange = v; }
    bool Optional() const { return optional; }                       // :77
    void SetOptional(bool v) { optional = v; }
    bool AllowPartialFailure() const { return allowPartialFailure; } // :91
    void SetAllowPartialFailure(bool v) { allowPartialFailure = v; }
    bool Enabled() const { return enabled; }                          // :105
    void SetEnabled(bool value);                                      // :105 setter
    int ParameterCount() const override { return static_cast<int>(parameters_.size()); }  // :121
    std::string HashString();                                         // :127
    std::string Title() override { return GetTitle(); }               // :144
    std::string Notes() { return GetNotes(); }                        // :146
    std::string MessageComplete() { return GetMessageComplete(); }    // :148
    std::string MessageFailed() { return GetMessageFailed(); }        // :150
    std::string MessageIncomplete() { return GetMessageIncomplete(); }  // :152
    // :154 AllParameters: this, then the children depth first
    void AllParameters(std::vector<ContractParameter*>& out);

    void NestToParent(IContractParameterHost* parent);                // :171
    void Enable();                                                    // :177
    void Disable(bool recursive = false);                             // :187
    void ParameterStateUpdate(ContractParameter* p) override;         // :205
    ContractParameter* AddParameter(std::unique_ptr<ContractParameter> parameter, const std::string& id = std::string()) override;  // :212
    ContractParameter* GetParameter(int index) override;              // :223
    ContractParameter* GetParameter(const std::string& id) override;  // :228
    ContractParameter* GetParameterOfType(const std::string& typeName) override;  // :247
    // :266 GetParameter<T>(id = null): the first child with this id that is a T
    template <class T>
    T* GetParameterT(const std::string& id = std::string()) {
        for (auto& p : parameters_)
            if (p->ID() == id && dynamic_cast<T*>(p.get()) != nullptr) return static_cast<T*>(p.get());
        return nullptr;
    }
    void RemoveParameter(int index) override;                         // :288
    void RemoveParameter(const std::string& id) override;             // :293
    void RemoveParameterOfType(const std::string& typeName) override;  // :312
    void RemoveParameter(ContractParameter* parameter) override;      // :331
    virtual void OnParameterStateChange(ContractParameter* p) { (void)p; }  // :336
    bool AllChildParametersComplete();                                // :340
    bool AnyChildParametersFailed();                                  // :354
    void SetFunds(float completion, CelestialBody* body = nullptr);   // :368
    void SetFunds(float completion, float failure, CelestialBody* body = nullptr);  // :378
    void SetReputation(float completion, float failure, CelestialBody* body = nullptr);  // :389
    void SetReputation(float completion, CelestialBody* body = nullptr);  // :399
    void SetScience(float completion, CelestialBody* body = nullptr);  // :408

    virtual void AwardCompletion();                                   // :423
    virtual void PenalizeFailure();                                   // :466
    void Load(ConfigNode& node);                                      // :512
    void Save(ConfigNode& node);                                      // :578
    void Register();                                                  // :614
    void Unregister();                                                // :627
    void Update();                                                    // :636
    void Reset();                                                     // :656
    long long CreateID();                                             // :784

    // overridables (protected virtuals of the C# class)
    virtual std::string GetHashString() { return ""; }                // :676
    virtual std::string GetTitle();                                   // :681
    virtual std::string GetNotes() { return ""; }                     // :686 (null)
    virtual std::string GetMessageComplete();                         // :691
    virtual std::string GetMessageFailed();                           // :696
    virtual std::string GetMessageIncomplete();                       // :701
    virtual void OnLoad(ConfigNode& node) { (void)node; }             // :706
    virtual void OnSave(ConfigNode& node) { (void)node; }             // :710
    virtual void OnRegister() {}                                      // :714
    virtual void OnUnregister() {}                                    // :718
    virtual void OnUpdate() {}                                        // :722
    virtual void OnReset() {}                                         // :726

    void SetComplete();                                               // :727
    void SetIncomplete();                                             // :746
    void SetFailed();                                                 // :764
    void SendStateMessage(const std::string& title, const std::string& message);  // :810

    // protected fields of the C#
    ParameterState state = ParameterState::Incomplete;                // :14
    std::string id;                                                   // :17
    bool disableOnStateChange = true;                                 // :20
    bool optional = false;                                            // :23
    bool allowPartialFailure = false;                                 // :26
    bool enabled = true;                                              // :28

private:
    float GetDestinationWeight(CelestialBody* body);                  // :418
    IContractParameterHost* parent_ = nullptr;                        // :11
    Contract* root_ = nullptr;                                        // :13
    std::vector<std::unique_ptr<ContractParameter>> parameters_;      // :30
};

// ContractPredicate.cs:7
class ContractPredicate {
public:
    explicit ContractPredicate(IContractParameterHost* parent);
    virtual ~ContractPredicate() = default;
    IContractParameterHost* Parent() const { return parent_; }       // :12
    Contract* Root() const { return root_; }                          // :15
    std::string Description() { return GetDescription(); }            // :17
    bool AllowMultiple() { return GetAllowMultiple(); }               // :19
    virtual std::string TypeName() const { return "ContractPredicate"; }
    void Load(ConfigNode& node) { OnLoad(node); }                     // :28
    void Save(ConfigNode& node) { OnSave(node); }                     // :33
    virtual bool Test(Vessel* vessel);                                // :38
    virtual bool Test(ProtoVessel* vessel);                           // :43
    void Update() { OnUpdate(); }                                     // :62
    virtual std::string GetDescription();                             // :48
    virtual bool GetAllowMultiple() { return false; }                 // :53
    virtual void OnLoad(ConfigNode& node) { (void)node; }             // :58 (the C# declares OnLoad / OnSave virtual at :58 / :60)
    virtual void OnSave(ConfigNode& node) { (void)node; }
    virtual void OnUpdate() {}                                        // :67

protected:
    IContractParameterHost* parent_;
    Contract* root_;
};

// Contract.cs:12
class Contract : public IContractParameterHost {
public:
    using ContractPrestige = ksp::ContractPrestige;  // :14 (declared by WP-36 in career_environment.hpp)

    enum class State : int { Generated, Offered, OfferExpired, Declined, Cancelled, Active, Completed, DeadlineExpired, Failed, Withdrawn };  // :27
    enum class Viewed : int { Unseen, Seen, Read };                  // :52
    enum class DeadlineType : int { Fixed, Floating, None };          // :60
    enum class ProgressState : int { Unreached, Reached, Incomplete, Complete };  // :67

    Contract();
    ~Contract() override;
    Contract(const Contract&) = delete;
    Contract& operator=(const Contract&) = delete;

    static inline int contractsInExistance = 0;                       // :149 (private set)

    virtual std::string TypeName() const { return "Contract"; }       // GetType().Name

    double TimeExpiry = 0.0;                                          // :113
    double TimeDeadline = 0.0;                                        // :115
    double FundsAdvance = 0.0;                                        // :117
    double FundsCompletion = 0.0;                                     // :119
    double FundsFailure = 0.0;                                        // :121
    float ScienceCompletion = 0.0f;                                   // :123
    float ReputationCompletion = 0.0f;                                // :125
    float ReputationFailure = 0.0f;                                   // :127
    bool AutoAccept = false;                                          // :129
    bool IgnoresWeight = false;                                       // :131
    events::EventData<State> OnStateChange{"OnStateChange"};          // :139
    events::EventData<Viewed> OnViewedChange{"OnViewedChange"};       // :141

    ContractPrestige Prestige() const { return prestige; }            // :151
    std::string Title() override { return GetTitle(); }               // :153
    std::string Synopsys() { return GetSynopsys(); }                  // :155
    std::string Description() { return GetDescription(); }            // :157
    std::string Notes() { return GetNotes(); }                        // :159
    Agents::Agent* Agent() const { return agent; }                    // :161
    State ContractState() const { return state; }                     // :163
    std::string LocalizedContractState() const;                       // :165
    Viewed ContractViewed() const { return viewed; }                  // :167
    double DateExpire() const { return dateExpire; }                  // :169
    double DateAccepted() const { return dateAccepted; }              // :171
    double DateDeadline() const { return dateDeadline; }              // :173
    double DateFinished() const { return dateFinished; }              // :175
    long long ContractID() const { return contractID; }               // :177
    const Guid& ContractGuid() const { return contractGuid; }         // :179
    int MissionSeed() const { return missionSeed; }                   // :181
    std::vector<std::string>& Keywords() { return keywords; }         // :183
    std::vector<std::string>& KeywordsRequired() { return keywordsRequired; }  // :185
    IContractParameterHost* Parent() override { return nullptr; }     // :187
    Contract* Root() override { return this; }                        // :189
    int ParameterCount() const override { return static_cast<int>(parameters_.size()); }  // :191
    // :199 AllParameters: the parameters' trees depth first (each parameter first, then its children)
    void AllParameters(std::vector<ContractParameter*>& out);
    static double GameTime();                                         // :218

    // :237 / :242 / :247 (UnityEngine.Random draws on the process-wide generator)
    void SetExpiry();
    void SetExpiry(int minDays, int maxDays);
    void SetExpiry(float minDays, float maxDays);
    void SetDeadlineDays(float days, CelestialBody* body = nullptr);   // :252
    void SetDeadlineYears(float years, CelestialBody* body = nullptr); // :262
    void SetFunds(float advance, float completion, CelestialBody* body = nullptr);  // :272
    void SetFunds(float advance, float completion, float failure, CelestialBody* body = nullptr);  // :283
    void SetReputation(float completion, float failure, CelestialBody* body = nullptr);  // :294
    void SetReputation(float completion, CelestialBody* body = nullptr);  // :304
    void SetScience(float completion, CelestialBody* body = nullptr); // :313
    float GetDestinationWeight(CelestialBody* body);                  // :323
    bool AddKeywords(const std::vector<std::string>& keywords);       // :328
    bool AddKeywordsRequired(const std::vector<std::string>& keywords);  // :353

    ContractParameter* AddParameter(std::unique_ptr<ContractParameter> parameter, const std::string& id = std::string()) override;  // :378
    void ParameterStateUpdate(ContractParameter* p) override { OnParameterStateChange(p); }  // :388
    ContractParameter* GetParameter(int index) override;              // :393
    ContractParameter* GetParameter(const std::string& id) override;  // :398
    ContractParameter* GetParameterOfType(const std::string& typeName) override;  // :415
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
    void RemoveParameter(int index) override;                         // :456
    void RemoveParameter(const std::string& id) override;             // :461
    void RemoveParameterOfType(const std::string& typeName) override;  // :480
    void RemoveParameter(ContractParameter* parameter) override;      // :499
    virtual void OnParameterStateChange(ContractParameter* p);        // :505
    virtual bool CanBeCancelled() { return true; }                    // :554
    virtual bool CanBeDeclined() { return true; }                     // :559
    virtual bool CanBeFailed() { return true; }                       // :564
    virtual bool MeetRequirements() { return false; }                 // :569
    virtual std::vector<CelestialBody*>* GetWeightBodies() { return nullptr; }  // :574 (a fresh list in the C#; the pointer is owned by the caller: see WeightBodies())
    virtual std::vector<CelestialBody*> WeightBodies() { return {}; }  // GetWeightBodies() result; empty + hasWeightBodies=false = null
    virtual bool HasWeightBodies() { return false; }
    virtual std::string GetSynopsys() { return ""; }                  // :579
    virtual std::string GetDescription() { return ""; }               // :584
    virtual std::string GetTitle() { return ""; }                     // :589 (null)
    virtual std::string GetNotes() { return ""; }                     // :594 (null)
    virtual bool Generate() { return false; }                         // :599
    virtual void OnRegister() {}                                      // :604
    virtual void OnUnregister() {}                                    // :608
    virtual void OnUpdate() {}                                        // :612
    virtual void OnReset() {}                                         // :616
    virtual void OnLoad(ConfigNode& node) { (void)node; }             // :620
    virtual void OnSave(ConfigNode& node) { (void)node; }             // :624
    virtual void AwardAdvance();                                      // :628
    virtual void AwardCompletion();                                   // :638
    virtual void PenalizeCancellation();                              // :664
    virtual void PenalizeFailure();                                   // :712
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
    virtual std::string MessageRewards();                             // :857
    virtual std::string MessageFailurePenalties();                    // :907
    virtual std::string MessageCancellationPenalties(double fundsPenalty, float repPenalty);  // :951
    virtual std::string MissionControlTextRich();                     // :993

    void SendStateMessage(const std::string& title, const std::string& message);  // :1181 (colour / icon are GUI)
    bool Offer();                                                     // :1195
    bool Accept();                                                    // :1214
    bool Decline();                                                   // :1229
    bool Cancel();                                                    // :1247
    void Update();                                                    // :1259
    void Reset();                                                     // :1314
    void Withdraw();                                                  // :1330
    bool Fail();                                                      // :1338
    bool Complete();                                                  // :1350
    void SetState(State newState);                                    // :1363
    bool IsFinished();                                                // :1470
    void Register();                                                  // :1477
    void Unregister();                                                // :1487
    void Kill();                                                      // :1497
    void SetViewed(Viewed viewed);                                    // :1505

    // :1531 Generate(Type, difficulty, seed, state): the contract is created through the ContractSystem type table
    static std::unique_ptr<Contract> Generate(const std::string& contractTypeName, ContractPrestige difficulty, int seed, State state);
    // :1566 Load(contract, node)
    static bool Load(Contract& contract, ConfigNode& node);
    void Save(ConfigNode& node);                                      // :1727
    void GenerateFailed() { OnGenerateFailed(); }                     // :1773

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
    ContractPrestige prestige = ContractPrestige::Trivial;           // :80
    DeadlineType expiryType = DeadlineType::Floating;                 // :83
    DeadlineType deadlineType = DeadlineType::Floating;               // :86
    Agents::Agent* agent = nullptr;                                   // :89
    State state = State::Generated;                                   // :92
    Viewed viewed = Viewed::Unseen;                                   // :95
    double dateExpire = 0.0;                                          // :98
    double dateAccepted = 0.0;                                        // :101
    double dateDeadline = 0.0;                                        // :104
    double dateFinished = 0.0;                                        // :107
    long long contractID = 0;                                         // :110
    Guid contractGuid;                                                // :112
    int missionSeed = 0;                                              // :115

private:
    void SetupSeed(int seed = -1);                                    // :1158
    void SetupID();                                                   // :1172
    std::string StateMsgAddition(const std::string& addition);       // :1384
    static void AddBodyNodes(ProgressState nodeState, const std::string& nodeName, std::vector<CelestialBody*>& bodies,
                             KSPAchievements::CelestialBodySubtree* tree, const std::function<bool(CelestialBody*)>& where);
    static void AddBodyTrees(ProgressState bodyState, std::vector<KSPAchievements::CelestialBodySubtree*>& bodies,
                             KSPAchievements::CelestialBodySubtree* tree);
    static void AddBodyTrees(ProgressState bodyState, const std::string& nodeName, ProgressState nodeState,
                             std::vector<KSPAchievements::CelestialBodySubtree*>& bodies, KSPAchievements::CelestialBodySubtree* tree);
    static void AddNextUnreachedBodyTrees(int& depth, std::vector<KSPAchievements::CelestialBodySubtree*>& bodies,
                                          KSPAchievements::CelestialBodySubtree* tree, const std::function<bool(CelestialBody*)>& where);

    std::vector<std::string> keywords;                                // :133
    std::vector<std::string> keywordsRequired;                        // :135
    std::vector<std::unique_ptr<ContractParameter>> parameters_;      // :138
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
