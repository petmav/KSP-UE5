// WP-36: ProgressNode.cs, ProgressTree.cs, ProgressTracking.cs.
#include "ksp/career/format.hpp"
#include "ksp/career/progress.hpp"

#include "ksp/career/funding.hpp"
#include "ksp/career/upgradeables.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;
using namespace KSPAchievements;

namespace achievements_detail {
void BuildTree(ProgressTracking& pt, ProgressTree& tree);  // achievements.cpp: ProgressTracking.generateAchievementsTree
}

namespace {
Game* CurrentGame() { return SaveEnvironment::Current().currentGame; }
}  // namespace

// ---- ProgressNode ---------------------------------------------------------------------------------------------------------

// ProgressNode.cs:41
ProgressNode::ProgressNode(std::string id, bool startReached) : id_(std::move(id)), reached_(startReached), subtree_(std::make_unique<ProgressTree>()) {}

// ProgressNode.cs:47
void ProgressNode::Reach() {
    reached_ = true;
    log::Info("[Progress Node Reached]: " + id_);
    GE::OnProgressReached.Fire(this);
    AchieveDate = SaveUniversalTime();
}

// ProgressNode.cs:55
void ProgressNode::Achieve() { GE::OnProgressAchieved.Fire(this); }

// ProgressNode.cs:60
void ProgressNode::Complete() {
    if (!reached_) Reach();
    complete_ = true;
    log::Info("[Progress Node Complete]: " + id_);
    AchieveDate = SaveUniversalTime();
    GE::OnProgressComplete.Fire(this);
}

// ProgressNode.cs:72
void ProgressNode::CheatComplete() {
    IsCompleteManned_ = true;
    IsCompleteUnmanned_ = true;
    if (!IsComplete()) Complete();
}

// ProgressNode.cs:82
void ProgressNode::Load(const ConfigNode& node) {
    // the event carries the node by pointer (the C# passes the reference; listeners may read it)
    GE::onProgressNodeLoad.Fire(GE::FromToAction<ProgressNode*, ConfigNode*>(this, const_cast<ConfigNode*>(&node)));
    reached_ = true;
    if (node.HasValue("reached")) {
        AchieveDate = dotnet::ParseDouble(*node.GetValue("reached"));
    } else if (node.HasValue("completed")) {
        complete_ = true;
        IsCompleteManned_ = true;
        IsCompleteUnmanned_ = true;
        AchieveDate = dotnet::ParseDouble(*node.GetValue("completed"));
    } else if (node.HasValue("completedManned")) {
        complete_ = true;
        IsCompleteManned_ = true;
        AchieveDate = dotnet::ParseDouble(*node.GetValue("completedManned"));
    } else if (node.HasValue("completedUnmanned")) {
        complete_ = true;
        IsCompleteUnmanned_ = true;
        AchieveDate = dotnet::ParseDouble(*node.GetValue("completedUnmanned"));
    }
    OnLoad(node);
}

// ProgressNode.cs:116
void ProgressNode::Save(ConfigNode& node) {
    if (complete_) {
        if (IsCompleteManned_ && !IsCompleteUnmanned_)
            node.AddValue("completedManned", AchieveDate);
        else if (!IsCompleteManned_ && IsCompleteUnmanned_)
            node.AddValue("completedUnmanned", AchieveDate);
        else
            node.AddValue("completed", AchieveDate);
    } else if (reached_) {
        node.AddValue("reached", AchieveDate);
    }
    OnSave(node);
    GE::onProgressNodeSave.Fire(GE::FromToAction<ProgressNode*, ConfigNode*>(this, &node));
}

// ProgressNode.cs:145
std::string ProgressNode::GetNodeSummary(const std::string& baseID) {
    if (OnGenerateSummary) return OnGenerateSummary(baseID);
    return baseID + Id() + "=" + career::LocalizeNumber(AchieveDate, "0.0");
}

// ProgressNode.cs:154
void ProgressNode::CrewSensitiveComplete(Vessel* v) {
    if (!IsCompleteManned_ || !IsCompleteUnmanned_) {
        const int count = static_cast<int>(v->GetVesselCrew().size());
        if (!IsCompleteManned_ && count > 0) IsCompleteManned_ = true;
        if (!IsCompleteUnmanned_ && count <= 0) IsCompleteUnmanned_ = true;
    }
}

// ProgressNode.cs:170
void ProgressNode::CrewSensitiveComplete(ProtoVessel* pv) {
    if (!IsCompleteManned_ || !IsCompleteUnmanned_) {
        const int count = static_cast<int>(pv->GetVesselCrew().size());
        if (!IsCompleteManned_ && count > 0) IsCompleteManned_ = true;
        if (!IsCompleteUnmanned_ && count <= 0) IsCompleteUnmanned_ = true;
    }
}

// ProgressNode.cs:186
void ProgressNode::AwardProgress(const std::string& description, float funds, float science, float reputation, CelestialBody* body) {
    (void)body;
    if (CurrentGame()->Mode != Game::Modes::CAREER) return;
    CareerEnvironment& env = CareerEnvironment::Current();
    if (funds > 0.0f && Funding::Instance != nullptr) Funding::Instance->AddFunds(static_cast<double>(funds), TransactionReasons::Progression);
    if (science > 0.0f && env.rndPresent && env.rndPresent() && env.rndAddScience) env.rndAddScience(science, TransactionReasons::Progression);
    if (reputation > 0.0f && Reputation::Instance != nullptr) Reputation::Instance->AddReputation(reputation, TransactionReasons::Progression);
    // StringUtilities.FormattedCurrencies + AddOrAppendWorldFirstMessage: GUI (MessageSystem), a seam
    if (env.worldFirstMessage) env.worldFirstMessage(description, funds, science, reputation);
}

// ProgressNode.cs:217 (MessageSystem is a GUI singleton: the host's postMessage seam gets the title and the text)
void ProgressNode::AddOrAppendWorldFirstMessage(const std::string& title, const std::string& body) {
    CareerEnvironment& env = CareerEnvironment::Current();
    if (env.postMessage) env.postMessage(Localizer::Format("#autoLOC_296780"), title + "\n " + body);
}

// ProgressNode.cs:247
void ProgressNode::AwardProgressStandard(const std::string& description, FinePrint::Utilities::ProgressType progress, CelestialBody* body) {
    using FinePrint::Utilities::ProgressRewardType;
    CareerEnvironment& env = CareerEnvironment::Current();
    auto reward = [&](Currency c) {
        return env.worldFirstStandardReward ? env.worldFirstStandardReward(ProgressRewardType::PROGRESS, c, progress, body) : 0.0f;
    };
    const float funds = reward(Currency::Funds);
    const float science = reward(Currency::Science);
    const float reputation = reward(Currency::Reputation);
    AwardProgress(description, funds, science, reputation, body);
}

// ProgressNode.cs:255
void ProgressNode::AwardProgressInterval(const std::string& description, int currentInterval, int totalIntervals,
                                         FinePrint::Utilities::ProgressType progress, CelestialBody* body) {
    using FinePrint::Utilities::ProgressRewardType;
    CareerEnvironment& env = CareerEnvironment::Current();
    auto reward = [&](Currency c) {
        return env.worldFirstIntervalReward ? env.worldFirstIntervalReward(ProgressRewardType::PROGRESS, c, progress, body, currentInterval, totalIntervals)
                                            : 0.0f;
    };
    const float funds = reward(Currency::Funds);
    const float science = reward(Currency::Science);
    const float reputation = reward(Currency::Reputation);
    AwardProgress(description, funds, science, reputation, body);
}

// ProgressNode.cs:263
void ProgressNode::AwardProgressRandomTech(const std::string& description, int seed) {
    CareerEnvironment& env = CareerEnvironment::Current();
    const Game::Modes mode = CurrentGame()->Mode;
    if ((mode == Game::Modes::CAREER || mode == Game::Modes::SCIENCE_SANDBOX) && env.rndPresent && env.rndPresent()) {
        const int count = env.rndNextUnavailableNodeCount ? env.rndNextUnavailableNodeCount() : 0;
        if (count > 0) {
            KSPRandom rnd(seed);
            const int index = rnd.Next(0, count);
            const std::string title = env.rndUnlockNextUnavailableNode ? env.rndUnlockNextUnavailableNode(index) : std::string();
            const std::string body = Localizer::Format("#autoLOC_296834", {"#cc6699" /* XKCDColors.HexFormat.PurplishPink */,
                                                                          "<sprite=\"CurrencySpriteAsset\" name=\"Flask\" tint=1>", Localizer::Format(title)});
            AddOrAppendWorldFirstMessage(description, body);
        }
    }
}

// ---- ProgressTree -----------------------------------------------------------------------------------------------------------

ProgressTree::ProgressTree() = default;
ProgressTree::~ProgressTree() = default;

// ProgressTree.cs:15
ProgressNode* ProgressTree::operator[](const std::string& s) const {
    for (size_t i = 0; i < nodes_.size(); ++i)
        if (nodes_[i]->Id() == s) return nodes_[i].get();
    return nullptr;
}

// ProgressTree.cs:50
bool ProgressTree::Contains(const ProgressNode* node) const {
    for (const auto& n : nodes_)
        if (n.get() == node) return true;
    return false;
}

// ProgressTree.cs:55
bool ProgressTree::AllComplete() const {
    int count = static_cast<int>(nodes_.size());
    while (count-- > 0)
        if (!nodes_[static_cast<size_t>(count)]->IsComplete()) return false;
    return true;
}

// ProgressTree.cs:74
void ProgressTree::Load(const ConfigNode& node) {
    for (size_t i = 0, count = nodes_.size(); i < count; ++i) {
        ProgressNode* n = nodes_[i].get();
        if (node.GetNode(n->Id()) != nullptr) n->Load(*node.GetNode(n->Id()));
    }
}

// ProgressTree.cs:87
void ProgressTree::Save(ConfigNode& node) {
    log::Info("Saving Achievements Tree...");
    for (size_t i = 0, count = nodes_.size(); i < count; ++i) {
        ProgressNode* n = nodes_[i].get();
        if (n->IsReached()) n->Save(*node.AddNode(n->Id()));
    }
}

// ProgressTree.cs:101
void ProgressTree::Deploy() {
    for (size_t i = 0, count = nodes_.size(); i < count; ++i)
        if (nodes_[i]->OnDeploy) nodes_[i]->OnDeploy();
}

// ProgressTree.cs:114
void ProgressTree::IterateVessels(Vessel* v) {
    for (size_t i = 0, count = nodes_.size(); i < count; ++i)
        if (nodes_[i]->OnIterateVessels) nodes_[i]->OnIterateVessels(v);
}

// ProgressTree.cs:127
void ProgressTree::Stow() {
    for (size_t i = 0, count = nodes_.size(); i < count; ++i)
        if (nodes_[i]->OnStow) nodes_[i]->OnStow();
}

// ProgressTree.cs:140
std::string ProgressTree::GetTreeSummary(const std::string& baseID) {
    std::string text;
    const int count = static_cast<int>(nodes_.size());
    for (int i = 0; i < count; ++i) {
        ProgressNode* n = nodes_[static_cast<size_t>(i)].get();
        if (n->IsReached() || n->IsComplete()) {
            text += n->GetNodeSummary(baseID);
            if (i < count - 1) text += ", ";
        }
    }
    return text;
}

// ---- ProgressTracking -------------------------------------------------------------------------------------------------------

ProgressTracking::ProgressTracking() : ScenarioModule("ProgressTracking") {}
ProgressTracking::~ProgressTracking() {
    GE::OnProgressComplete.Remove(this, &ProgressTracking::OnAchievementComplete);
    if (instance_ == this) instance_ = nullptr;
}

// ProgressTracking.cs:150
void ProgressTracking::OnAwake() {
    instance_ = this;
    achievementTree.reset(generateAchievementsTree());
    GE::OnProgressComplete.Add(this, &ProgressTracking::OnAchievementComplete);
}

// ProgressTracking.cs:157
ProgressTree* ProgressTracking::generateAchievementsTree() {
    auto* progressTree = new ProgressTree();
    achievements_detail::BuildTree(*this, *progressTree);
    return progressTree;
}

// ProgressTracking.cs:302
void ProgressTracking::OnLoad(ConfigNode& node) {
    if (node.GetNode("Progress") != nullptr) achievementTree->Load(*node.GetNode("Progress"));
}

// ProgressTracking.cs:310
void ProgressTracking::OnSave(ConfigNode& node) { achievementTree->Save(*node.AddNode("Progress")); }

// ProgressTracking.cs:315
void ProgressTracking::Start() { achievementTree->Deploy(); }

// ProgressTracking.cs:320
void ProgressTracking::Update() {
    if (!(SaveEnvironment::Current().loadedScene == GameScenes::EDITOR)) {
        for (size_t i = 0; i < FlightRegistry::Vessels.size(); ++i) achievementTree->IterateVessels(FlightRegistry::Vessels[i]);
    }
}

// ProgressTracking.cs:331
void ProgressTracking::OnDestroy() {
    achievementTree->Stow();
    GE::OnProgressComplete.Remove(this, &ProgressTracking::OnAchievementComplete);
    if (instance_ != nullptr && instance_ == this) instance_ = nullptr;
}

// ProgressTracking.cs:341
void ProgressTracking::OnAchievementComplete(ProgressNode*) {}

// ProgressTracking.cs:367
std::string ProgressTracking::GenerateSummary() { return achievementTree->GetTreeSummary(""); }

// ProgressTracking.cs:372
bool ProgressTracking::NodeReached(const std::vector<std::string>& search) {
    if (search.empty()) return false;
    ProgressNode* n = FindNode(search);
    return n != nullptr && n->IsReached();
}

// ProgressTracking.cs:381
bool ProgressTracking::NodeComplete(const std::vector<std::string>& search) {
    if (search.empty()) return false;
    ProgressNode* n = FindNode(search);
    return n != nullptr && n->IsComplete();
}

// ProgressTracking.cs:390
bool ProgressTracking::NodeCompleteManned(const std::vector<std::string>& search) {
    if (search.empty()) return false;
    ProgressNode* n = FindNode(search);
    return n != nullptr && n->IsCompleteManned();
}

// ProgressTracking.cs:399
bool ProgressTracking::NodeCompleteUnmanned(const std::vector<std::string>& search) {
    if (search.empty()) return false;
    ProgressNode* n = FindNode(search);
    return n != nullptr && n->IsCompleteUnmanned();
}

// ProgressTracking.cs:408
ProgressNode* ProgressTracking::FindNode(const std::vector<std::string>& search) {
    if (search.empty()) return nullptr;
    return FindNodeRecurse(*achievementTree, 0, search);
}

// ProgressTracking.cs:417
ProgressNode* ProgressTracking::FindNodeRecurse(ProgressTree& tree, int depth, const std::vector<std::string>& search) {
    ProgressNode* progressNode = tree[search[static_cast<size_t>(depth)]];
    if (progressNode == nullptr) return nullptr;
    if (depth + 1 < static_cast<int>(search.size())) {
        if (progressNode->Subtree().Count() > 0) return FindNodeRecurse(progressNode->Subtree(), depth + 1, search);
        return nullptr;
    }
    return progressNode;
}

// ProgressTracking.cs:435
CelestialBodySubtree* ProgressTracking::GetBodyTree(const std::string& bodyName) {
    for (CelestialBodySubtree* t : celestialBodyNodes)
        if (t->Body->name() == bodyName) return t;
    return nullptr;
}

// ProgressTracking.cs:455
CelestialBodySubtree* ProgressTracking::GetBodyTree(CelestialBody* body) {
    for (CelestialBodySubtree* t : celestialBodyNodes)
        if (t->Body == body) return t;
    return nullptr;
}

// ProgressTracking.cs:475
CelestialBodySubtree* ProgressTracking::GetBodyTree() { return celestialBodyHome; }

// ProgressTracking.cs:480
void ProgressTracking::RecurseCheatProgression(ProgressNode* node) {
    if (node == nullptr) return;
    bool flag = false;
    if (auto* poi = dynamic_cast<PointOfInterest*>(node)) {
        for (size_t i = 0; i < nonCheatablePOIs_.size(); ++i)
            if (poi->name() == nonCheatablePOIs_[i]->name() && poi->body() == nonCheatablePOIs_[i]->body()) {
                flag = true;
                break;
            }
    }
    if (flag) return;
    node->CheatComplete();
    int count = node->Subtree().Count();
    while (count-- > 0) RecurseCheatProgression(node->Subtree()[count]);
}

// ProgressTracking.cs:514
void ProgressTracking::CheatProgression() {
    int count = achievementTree->Count();
    while (count-- > 0) RecurseCheatProgression((*achievementTree)[count]);
}

// ProgressTracking.cs:523
void ProgressTracking::CheatEarlyProgression() {
    firstLaunch->CheatComplete();
    reachSpace->CheatComplete();
    CelestialBodySubtree* bodyTree = GetBodyTree(CareerEnvironment::Current().HomeBody());
    ProgressNode* progressNode = bodyTree != nullptr ? bodyTree->orbit : nullptr;
    ProgressNode* progressNode2 = bodyTree != nullptr ? bodyTree->science : nullptr;
    if (progressNode != nullptr) progressNode->CheatComplete();
    if (progressNode2 != nullptr) progressNode2->CheatComplete();
}

void ProgressTracking::Register() {
    ScenarioModuleFactory::Register("ProgressTracking", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new ProgressTracking()); });
}

}  // namespace ksp
