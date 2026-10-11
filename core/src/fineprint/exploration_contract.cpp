// WP-38: FinePrint/Contracts/Parameters/ProgressTrackingParameter.cs and FinePrint/Contracts/ExplorationContract.cs (the mandatory World First
// contract), plus FinePrint::Install().
#include <algorithm>

#include "ksp/career/career.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/contracts/text_gen.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/fineprint/fineprint.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp::Contracts::Parameters {

using KSP::Localization::Localizer;
using Lite::MannedStatus;
using Lite::ProgressType;
using GE = events::GameEvents;

namespace {
// Convert.ToDecimal(Math.Round(x)).ToString("#,###"): the rounded integer with thousands separators ("" for zero)
std::string GroupThousands(double x) {
    const double r = dotnet::Math::Round(x);
    long long v = static_cast<long long>(r < 0 ? -r : r);
    if (v == 0) return "";
    std::string digits = std::to_string(v);
    std::string out;
    int n = static_cast<int>(digits.size());
    for (int i = 0; i < n; ++i) {
        out += digits[static_cast<size_t>(i)];
        if ((n - 1 - i) % 3 == 0 && i != n - 1) out += ',';
    }
    return (r < 0 ? "-" : "") + out;
}
}  // namespace

// ProgressTrackingParameter.cs:164
ProgressTrackingParameter::ProgressTrackingParameter() : milestone() {}

// ProgressTrackingParameter.cs:164
ProgressTrackingParameter::ProgressTrackingParameter(const Lite::ProgressMilestone& m) : milestone(m.body, m.type) {}

// ProgressTrackingParameter.cs:15
bool ProgressTrackingParameter::IsTutorial() {
    if (!(milestone.body == nullptr) && !(milestone.body != Planetarium::fetch->Home)) {
        switch (milestone.type) {
            default: return false;
            case ProgressType::FIRSTLAUNCH:
            case ProgressType::ORBIT:
            case ProgressType::REACHSPACE:
            case ProgressType::SCIENCE: return true;
        }
    }
    return false;
}

// ProgressTrackingParameter.cs:36
std::string ProgressTrackingParameter::InstructionalNote() {
    const std::string bn = milestone.body != nullptr ? milestone.body->displayName() : std::string();
    const std::string hn = Planetarium::fetch->Home->displayName();
    (void)bn;
    (void)hn;
    switch (milestone.type) {
        case ProgressType::BASECONSTRUCTION:
        return Localizer::Format("#autoLOC_284365", {bn});
        case ProgressType::CREWTRANSFER:
        return Localizer::Format("#autoLOC_284367", {bn});
        case ProgressType::DOCKING:
        return Localizer::Format("#autoLOC_284369", {bn});
        case ProgressType::ESCAPE:
        return Localizer::Format("#autoLOC_284371", {bn});
        case ProgressType::FIRSTLAUNCH:
        return Localizer::Format("#autoLOC_284373");
        case ProgressType::FLAGPLANT:
        return Localizer::Format("#autoLOC_284375", {bn});
        case ProgressType::FLIGHT:
        return Localizer::Format("#autoLOC_284377", {bn});
        case ProgressType::FLYBY:
        return Localizer::Format("#autoLOC_284379", {bn});
        case ProgressType::FLYBYRETURN:
        return Localizer::Format("#autoLOC_284381", {bn, hn});
        case ProgressType::LANDING:
        return Localizer::Format(milestone.body->ocean ? "#autoLOC_6001960" : "#autoLOC_6001961", {bn});
        case ProgressType::LANDINGRETURN:
        return Localizer::Format("#autoLOC_284385", {bn, hn});
        case ProgressType::ORBIT:
        if (milestone.body == Planetarium::fetch->Home)
        return Localizer::Format("#autoLOC_284388");
        return Localizer::Format("#autoLOC_284390", {bn});
        case ProgressType::ORBITRETURN:
        if (milestone.body == Planetarium::fetch->Home)
        return Localizer::Format("#autoLOC_284393");
        return Localizer::Format("#autoLOC_284395", {bn, hn});
        case ProgressType::REACHSPACE:
        return Localizer::Format("#autoLOC_284397", {GroupThousands(Planetarium::fetch->Home->atmosphereDepth)});
        case ProgressType::RENDEZVOUS:
        return Localizer::Format("#autoLOC_284399", {bn});
        case ProgressType::SCIENCE:
        return Localizer::Format("#autoLOC_284401", {bn});
        case ProgressType::SPACEWALK:
        return Localizer::Format("#autoLOC_284403", {bn});
        case ProgressType::SPLASHDOWN:
        return Localizer::Format("#autoLOC_284405", {bn});
        case ProgressType::STATIONCONSTRUCTION:
        return Localizer::Format("#autoLOC_284407", {bn});
        default:
        return Localizer::Format("#autoLOC_284411");
        case ProgressType::SURFACEEVA:
        return Localizer::Format("#autoLOC_284409", {bn});
    }
    return "";
}

// ProgressTrackingParameter.cs:96
std::string ProgressTrackingParameter::FlavorNote() {
    const std::string bn = milestone.body != nullptr ? milestone.body->displayName() : std::string();
    const std::string hn = Planetarium::fetch->Home->displayName();
    (void)bn;
    (void)hn;
    switch (milestone.type) {
        case ProgressType::BASECONSTRUCTION:
        return Localizer::Format("#autoLOC_284423", {bn});
        case ProgressType::CREWTRANSFER:
        return Localizer::Format("#autoLOC_284425", {bn});
        case ProgressType::DOCKING:
        return Localizer::Format("#autoLOC_284427", {bn});
        case ProgressType::ESCAPE:
        return Localizer::Format("#autoLOC_284429", {bn});
        case ProgressType::FIRSTLAUNCH:
        return Localizer::Format("#autoLOC_284431");
        case ProgressType::FLAGPLANT:
        return Localizer::Format("#autoLOC_284433", {bn});
        case ProgressType::FLIGHT:
        return Localizer::Format("#autoLOC_284435", {bn, hn});
        case ProgressType::FLYBY:
        return Localizer::Format("#autoLOC_284437", {bn});
        case ProgressType::FLYBYRETURN:
        return Localizer::Format("#autoLOC_284439", {bn, hn});
        case ProgressType::LANDING:
        return Localizer::Format("#autoLOC_284441", {bn});
        case ProgressType::LANDINGRETURN:
        return Localizer::Format("#autoLOC_284443", {hn, bn});
        case ProgressType::ORBIT:
        if (milestone.body == Planetarium::fetch->Home)
        return Localizer::Format("#autoLOC_284446", {hn});
        return Localizer::Format("#autoLOC_284448", {bn});
        case ProgressType::ORBITRETURN:
        if (milestone.body == Planetarium::fetch->Home)
        return Localizer::Format("#autoLOC_284451");
        return Localizer::Format("#autoLOC_284453", {bn, hn});
        case ProgressType::REACHSPACE:
        return Localizer::Format("#autoLOC_284455");
        case ProgressType::RENDEZVOUS:
        return Localizer::Format("#autoLOC_284457", {bn});
        case ProgressType::SCIENCE:
        if (milestone.body == Planetarium::fetch->Home)
        return Localizer::Format("#autoLOC_284460");
        return Localizer::Format("#autoLOC_284462", {bn});
        case ProgressType::SPACEWALK:
        if (milestone.body == Planetarium::fetch->Home)
        return Localizer::Format("#autoLOC_6001963", {bn});
        return Localizer::Format("#autoLOC_6001962");
        case ProgressType::SPLASHDOWN:
        return Localizer::Format("#autoLOC_284466", {bn});
        case ProgressType::STATIONCONSTRUCTION:
        return Localizer::Format("#autoLOC_284468", {bn});
        default:
        return Localizer::Format("#autoLOC_284472");
        case ProgressType::SURFACEEVA:
        return Localizer::Format("#autoLOC_284470", {bn});
    }
    return "";
}

// ProgressTrackingParameter.cs:174
std::string ProgressTrackingParameter::GetHashString() { return std::to_string(Lite::SuperSeed(*Root())) + ID(); }

// ProgressTrackingParameter.cs:179
std::string ProgressTrackingParameter::GetTitle() {
    const std::string bn = milestone.body != nullptr ? milestone.body->displayName() : std::string();
    const std::string hn = Planetarium::fetch->Home->displayName();
    (void)bn;
    (void)hn;
    if (IsTutorial()) return Localizer::Format("#autoLOC_6001924");
    switch (milestone.type) {
        case ProgressType::BASECONSTRUCTION:
        return Localizer::Format("#autoLOC_284203", {bn});
        case ProgressType::CREWTRANSFER:
        return Localizer::Format("#autoLOC_284205", {bn});
        case ProgressType::DOCKING:
        return Localizer::Format("#autoLOC_284207", {bn});
        case ProgressType::ESCAPE:
        return Localizer::Format("#autoLOC_284209", {bn});
        case ProgressType::FIRSTLAUNCH:
        return Localizer::Format("#autoLOC_284211");
        case ProgressType::FLAGPLANT:
        return Localizer::Format("#autoLOC_284213", {bn});
        case ProgressType::FLIGHT:
        return Localizer::Format("#autoLOC_284215", {bn});
        case ProgressType::FLYBY:
        return Localizer::Format("#autoLOC_284217", {bn});
        case ProgressType::FLYBYRETURN:
        return Localizer::Format("#autoLOC_284219", {hn, bn});
        case ProgressType::LANDING:
        return Localizer::Format(milestone.body->ocean ? "#autoLOC_6001964" : "#autoLOC_269752", {bn});
        case ProgressType::LANDINGRETURN:
        return Localizer::Format("#autoLOC_284223", {hn, bn});
        case ProgressType::ORBIT:
        return Localizer::Format("#autoLOC_284225", {bn});
        case ProgressType::ORBITRETURN:
        return Localizer::Format((milestone.body == Planetarium::fetch->Home) ? "#autoLOC_6001965" : "#autoLOC_6001966", {hn, bn});
        case ProgressType::REACHSPACE:
        return Localizer::Format("#autoLOC_284229");
        case ProgressType::RENDEZVOUS:
        return Localizer::Format("#autoLOC_284231", {bn});
        case ProgressType::SCIENCE:
        return Localizer::Format("#autoLOC_284233", {bn});
        case ProgressType::SPACEWALK:
        return Localizer::Format("#autoLOC_284235", {bn});
        case ProgressType::SPLASHDOWN:
        return Localizer::Format("#autoLOC_284237", {bn});
        case ProgressType::STATIONCONSTRUCTION:
        return Localizer::Format("#autoLOC_284239", {bn});
        default:
        return Localizer::Format("#autoLOC_284243");
        case ProgressType::SURFACEEVA:
        return Localizer::Format("#autoLOC_284241", {bn});
    }
    return "";
}

// ProgressTrackingParameter.cs:232
std::string ProgressTrackingParameter::GetNotes() {
    if (SaveEnvironment::Current().loadedScene == GameScenes::SPACECENTER && !IsTutorial()) {
        std::string text = FlavorNote();
        const int parameterCount = Root()->ParameterCount();
        if (parameterCount > 0 && this != Root()->GetParameter(parameterCount - 1)) text += "\n";
        return text;
    }
    return InstructionalNote();
}

// ProgressTrackingParameter.cs:247
std::string ProgressTrackingParameter::GetMessageComplete() {
    const std::string bn = milestone.body != nullptr ? milestone.body->displayName() : std::string();
    const std::string hn = Planetarium::fetch->Home->displayName();
    (void)bn;
    (void)hn;
    if (IsTutorial()) return Localizer::Format("#autoLOC_284264", {Title()});
    switch (milestone.type) {
        case ProgressType::BASECONSTRUCTION:
        return Localizer::Format("#autoLOC_284269", {bn});
        case ProgressType::CREWTRANSFER:
        return Localizer::Format("#autoLOC_284271");
        case ProgressType::DOCKING:
        return Localizer::Format("#autoLOC_284273");
        case ProgressType::ESCAPE:
        return Localizer::Format("#autoLOC_284275");
        case ProgressType::FIRSTLAUNCH:
        return Localizer::Format("#autoLOC_284277");
        case ProgressType::FLAGPLANT:
        return Localizer::Format("#autoLOC_284279");
        case ProgressType::FLIGHT:
        return Localizer::Format("#autoLOC_284281");
        case ProgressType::FLYBY:
        return Localizer::Format("#autoLOC_284283");
        case ProgressType::FLYBYRETURN:
        return Localizer::Format("#autoLOC_284285", {bn});
        case ProgressType::LANDING:
        return Localizer::Format("#autoLOC_284287", {bn});
        case ProgressType::LANDINGRETURN:
        return Localizer::Format("#autoLOC_284289", {bn});
        case ProgressType::ORBIT:
        if (milestone.body == Planetarium::fetch->Home)
        return Localizer::Format("#autoLOC_284292");
        return Localizer::Format("#autoLOC_284294");
        case ProgressType::ORBITRETURN:
        if (milestone.body == Planetarium::fetch->Home)
        return Localizer::Format("#autoLOC_284297");
        return Localizer::Format("#autoLOC_284299", {bn});
        case ProgressType::REACHSPACE:
        return Localizer::Format("#autoLOC_284301");
        case ProgressType::RENDEZVOUS:
        return Localizer::Format("#autoLOC_284303");
        case ProgressType::SCIENCE:
        if (milestone.body == Planetarium::fetch->Home)
        return Localizer::Format("#autoLOC_284306");
        return Localizer::Format("#autoLOC_284308", {bn});
        case ProgressType::SPACEWALK:
        return Localizer::Format("#autoLOC_284310");
        case ProgressType::SPLASHDOWN:
        return Localizer::Format("#autoLOC_284312");
        case ProgressType::STATIONCONSTRUCTION:
        return Localizer::Format("#autoLOC_284314", {bn});
        default:
        return Localizer::Format("#autoLOC_284318");
        case ProgressType::SURFACEEVA:
        return Localizer::Format("#autoLOC_284316", {hn});
    }
    return "";
}

// ProgressTrackingParameter.cs:312
void ProgressTrackingParameter::OnRegister() {
    SetDisableOnStateChange(true);
    eventsAdded = false;
    if (Root()->ContractState() == Contract::State::Active) {
        GE::OnProgressComplete.Add(this, &ProgressTrackingParameter::ProgressCheck);
        eventsAdded = true;
    }
}

// ProgressTrackingParameter.cs:323
void ProgressTrackingParameter::OnUnregister() {
    if (eventsAdded) GE::OnProgressComplete.Remove(this, &ProgressTrackingParameter::ProgressCheck);
}

// ProgressTrackingParameter.cs:331
void ProgressTrackingParameter::ProgressCheck(ProgressNode*) {
    if (milestone.complete()) SetComplete();
}

// ProgressTrackingParameter.cs:339
void ProgressTrackingParameter::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(milestone.body->flightGlobalsIndex));
    node.AddValue("targetType", dotnet::EnumToString(milestone.type));
}

// ProgressTrackingParameter.cs:345
void ProgressTrackingParameter::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "ProgressTrackingParameter", "targetBody", milestone.body, Planetarium::fetch->Home);
    Lite::LoadNode(node, "ProgressTrackingParameter", "targetType", milestone.type, ProgressType::FIRSTLAUNCH);
}

}  // namespace ksp::Contracts::Parameters

namespace ksp::FinePrint::Contracts {

using KSP::Localization::Localizer;
using Lite::MannedStatus;
using Lite::ProgressType;
namespace PU = ksp::Contracts::Lite::ProgressUtilities;
namespace CU = ksp::Contracts::Lite::CelestialUtilities;
using ksp::Contracts::ContractDefsLite;
using ksp::Contracts::TextGen;
using ksp::Contracts::Parameters::ProgressTrackingParameter;
using Lite::ProgressMilestone;

namespace {
CelestialBody* Home() { return Planetarium::fetch->Home; }

// ExplorationContract.cs:17 ProgressTypeValue / :15 MilestoneComparer.Compare (int.CompareTo)
int ProgressTypeValue(const ProgressMilestone& milestone) {
    switch (milestone.type) {
        default: return 3;
        case ProgressType::FLYBY: return 0;
        case ProgressType::FLYBYRETURN: return 6;
        case ProgressType::LANDING: return 2;
        case ProgressType::LANDINGRETURN: return 4;
        case ProgressType::ORBIT: return 1;
        case ProgressType::ORBITRETURN: return 5;
    }
}
int MilestoneCompare(const ProgressMilestone& first, const ProgressMilestone& second) {
    const int num = ProgressTypeValue(first);
    const int value = ProgressTypeValue(second);
    return num < value ? -1 : (num > value ? 1 : 0);
}
}  // namespace

// ExplorationContract.cs:59
bool ExplorationContract::IsTutorial() {
    if (TrackingParameters().size() == 1) return TrackingParameters()[0]->IsTutorial();
    return false;
}

// ExplorationContract.cs:71
std::vector<ProgressTrackingParameter*>& ExplorationContract::TrackingParameters() {
    if (trackingParametersBuilt) return trackingParameters;
    if (ParameterCount() < 1) {
        emptyList_.clear();
        return emptyList_;
    }
    trackingParameters.clear();
    trackingParametersBuilt = true;
    for (int i = 0; i < ParameterCount(); i++) {
        if (auto* item = dynamic_cast<ProgressTrackingParameter*>(GetParameter(i))) trackingParameters.push_back(item);
    }
    return trackingParameters;
}

// ExplorationContract.cs:95
bool ExplorationContract::Generate() {
    IgnoresWeight = true;
    std::vector<ProgressMilestone> missionMilestones = GetMissionMilestones();
    if (missionMilestones.size() >= 1) {
        if (static_cast<int>(ksp::Contracts::ContractSystem::Instance->GetCurrentContracts<ExplorationContract>().size()) >= AllowableContracts(missionMilestones))
            return false;
        int i = 0;
        for (int count = static_cast<int>(missionMilestones.size()); i < count; i++)
            AddParameter(std::make_unique<ProgressTrackingParameter>(missionMilestones[static_cast<size_t>(i)]));
        SetLocals();
        SetPrestige();
        SetRewards();
        SetExpiry();
        AddKeywordsRequired({"Record"});
        deadlineType = DeadlineType::None;
        return true;
    }
    return false;
}

// ExplorationContract.cs:136
std::string ExplorationContract::GetTitle() {
    if (IsTutorial()) {
        ProgressMilestone& milestone = TrackingParameters()[0]->milestone;
        switch (milestone.type) {
            case ProgressType::ORBIT: return Localizer::Format("#autoLOC_278641", {milestone.body->displayName()});
            case ProgressType::FIRSTLAUNCH: return Localizer::Format("#autoLOC_278637");
            case ProgressType::SCIENCE: return Localizer::Format("#autoLOC_278643", {milestone.body->displayName()});
            case ProgressType::REACHSPACE: return Localizer::Format("#autoLOC_278639");
            default: break;
        }
    }
    return Localizer::Format("#autoLOC_278647", {targetBody->displayName()});
}

// ExplorationContract.cs:156
std::string ExplorationContract::GetDescription() {
    if (IsTutorial()) {
        switch (TrackingParameters()[0]->milestone.type) {
            case ProgressType::ORBIT: return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), "Orbit", "Orbit", MissionSeed(), false, false, false);
            case ProgressType::FIRSTLAUNCH: return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), "Launch", "Launch", MissionSeed(), false, false, false);
            case ProgressType::SCIENCE: return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), "Science", "Science", MissionSeed(), false, false, false);
            case ProgressType::REACHSPACE: return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), "Space", "Space", MissionSeed(), false, false, false);
            default: break;
        }
    }
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), targetBody->displayName(), targetBody->displayName(), MissionSeed(), true, true, true);
}

// ExplorationContract.cs:175
std::string ExplorationContract::GetSynopsys() {
    if (IsTutorial()) return TrackingParameters()[0]->FlavorNote();
    if (fx(bodyRatio) < fx(0.33f)) return Localizer::Format("#autoLOC_278678", {targetBody->displayName()});
    if (fx(bodyRatio) > fx(0.66f)) return Localizer::Format("#autoLOC_278680", {targetBody->displayName()});
    return Localizer::Format("#autoLOC_278682", {targetBody->displayName()});
}

// ExplorationContract.cs:192
std::string ExplorationContract::MessageCompleted() {
    if (IsTutorial()) {
        switch (TrackingParameters()[0]->milestone.type) {
            case ProgressType::ORBIT: return Localizer::Format("#autoLOC_278698");
            case ProgressType::FIRSTLAUNCH: return Localizer::Format("#autoLOC_278694");
            case ProgressType::SCIENCE: return Localizer::Format("#autoLOC_278700");
            case ProgressType::REACHSPACE: return Localizer::Format("#autoLOC_278696");
            default: break;
        }
    }
    if (fx(bodyRatio) < fx(0.33f)) return Localizer::Format("#autoLOC_278705", {targetBody->displayName()});
    if (fx(bodyRatio) > fx(0.66f)) return Localizer::Format("#autoLOC_278707", {targetBody->displayName()});
    return Localizer::Format("#autoLOC_278709", {targetBody->displayName()});
}

// ExplorationContract.cs:219
void ExplorationContract::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("bodyRatio", bodyRatio);
}

// ExplorationContract.cs:225
void ExplorationContract::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "ExplorationContract", "targetBody", targetBody, Planetarium::fetch->Sun);
    Lite::LoadNode(node, "ExplorationContract", "bodyRatio", bodyRatio, 0.5f);
}

// ExplorationContract.cs:231
bool ExplorationContract::MeetRequirements() {
    if (ContractState() == State::Generated || ContractState() == State::Offered) {
        int count = static_cast<int>(TrackingParameters().size());
        while (count-- > 0) {
            if (TrackingParameters()[static_cast<size_t>(count)]->milestone.complete()) return false;
        }
    }
    return true;
}

// ExplorationContract.cs:247
int ExplorationContract::AllowableContracts(const std::vector<ProgressMilestone>& milestones) {
    if (milestones.size() != 1) return 1;
    int num = 0;
    const ProgressMilestone& progressMilestone = milestones[0];
    ProgressTracking* pt = ProgressTracking::Instance();
    if (progressMilestone.type != ProgressType::FIRSTLAUNCH && (progressMilestone.type != ProgressType::SCIENCE || !(progressMilestone.body == Home())) &&
        progressMilestone.type != ProgressType::REACHSPACE && (progressMilestone.type != ProgressType::ORBIT || !(progressMilestone.body == Home()))) {
        num = 1;
    } else {
        if (!pt->NodeComplete({"FirstLaunch"})) num++;
        if (!pt->NodeComplete({Home()->name(), "Science"})) num++;
        if (!pt->NodeComplete({"ReachedSpace"})) num++;
        if (!pt->NodeComplete({Home()->name(), "Orbit"})) num++;
    }
    return num;
}

// ExplorationContract.cs:281
void ExplorationContract::SetLocals() {
    if (TrackingParameters().size() < 1) {
        targetBody = Home();
        bodyRatio = 0.5f;
        return;
    }
    targetBody = TrackingParameters()[0]->milestone.body;
    ProgressTree& subtree = targetBody->progressTree->Subtree();
    bodyRatio = 0.0f;
    int count = subtree.Count();
    while (count-- > 0) {
        if (subtree[count]->IsComplete()) bodyRatio = f32store(fx(bodyRatio) + 1.0);
    }
    bodyRatio = f32store(fx(bodyRatio) / fx(static_cast<float>(subtree.Count())));
}

// ExplorationContract.cs:303
void ExplorationContract::SetPrestige() {
    if (TrackingParameters().size() < 1) {
        prestige = ContractPrestige::Trivial;
        return;
    }
    if (IsTutorial()) {
        switch (TrackingParameters()[0]->milestone.type) {
            case ProgressType::ORBIT: prestige = ContractPrestige::Exceptional; break;
            case ProgressType::FIRSTLAUNCH:
            case ProgressType::SCIENCE: prestige = ContractPrestige::Trivial; break;
            case ProgressType::REACHSPACE: prestige = ContractPrestige::Significant; break;
            default: break;
        }
        return;
    }
    int count = static_cast<int>(TrackingParameters().size());
    do {
        if (count-- <= 0) {
            prestige = static_cast<ContractPrestige>(static_cast<int>(TrackingParameters().size()) - 1);
            return;
        }
    } while (!PU::OutlierWorldFirstContract(TrackingParameters()[static_cast<size_t>(count)]->milestone));
    prestige = ContractPrestige::Exceptional;
}

// ExplorationContract.cs:340
void ExplorationContract::SetRewards() {
    float num = 0.0f;
    float num2 = 0.0f;
    float num3 = 0.0f;
    int count = static_cast<int>(TrackingParameters().size());
    while (count-- > 0) {
        ProgressTrackingParameter* progressTrackingParameter = TrackingParameters()[static_cast<size_t>(count)];
        ProgressMilestone& milestone = progressTrackingParameter->milestone;
        const float num4 = Mathf::Ceil(f32store(fx(PU::WorldFirstStandardReward(Lite::ProgressRewardType::CONTRACT, Currency::Funds, milestone.type, milestone.body)) / 2.0));
        const float num5 = Mathf::Ceil(f32store(fx(PU::WorldFirstStandardReward(Lite::ProgressRewardType::CONTRACT, Currency::Science, milestone.type, milestone.body)) / 2.0));
        const float num6 = Mathf::Ceil(f32store(fx(PU::WorldFirstStandardReward(Lite::ProgressRewardType::CONTRACT, Currency::Reputation, milestone.type, milestone.body)) / 2.0));
        progressTrackingParameter->SetFunds(num4, targetBody);
        progressTrackingParameter->SetScience(num5);
        progressTrackingParameter->SetReputation(num6);
        num = f32store(fx(num) + fx(num4));
        num2 = f32store(fx(num2) + fx(num5));
        num3 = f32store(fx(num3) + fx(num6));
    }
    const float num7 = f32store(fx(num) * fx(0.35f));
    num = f32store(fx(num) - fx(num7));
    SetFunds(num7, num, targetBody);
    SetScience(num2);
    SetReputation(num3);
}

// ExplorationContract.cs:367
std::vector<ProgressMilestone> ExplorationContract::GetMissionMilestones() {
    KSPRandom kSPRandom(MissionSeed());
    rendezvousUnlocked = PU::GetProgressLevel() >= 4;
    dockUnlocked = rendezvousUnlocked && ksp::Contracts::Templates::TemplateEnvironment::researchedValidContractObjectives &&
                   ksp::Contracts::Templates::TemplateEnvironment::researchedValidContractObjectives("Dock");
    evaUnlocked = GameVariables::Instance->UnlockedEVA(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::AstronautComplex));
    flagsUnlocked = GameVariables::Instance->UnlockedEVAFlags(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::AstronautComplex));
    std::vector<ProgressMilestone> list = GetSeedMilestones(&kSPRandom);
    if (list.size() <= 0) return list;
    const size_t seedIndex = static_cast<size_t>(kSPRandom.Next(static_cast<int>(list.size())));
    ProgressMilestone progressMilestone = list[seedIndex];
    list.erase(list.begin() + static_cast<std::ptrdiff_t>(seedIndex));  // list.Remove(progressMilestone): reference equality, the element itself
    std::vector<ProgressMilestone> themed = GetThemedMilestones(progressMilestone);
    list.insert(list.end(), themed.begin(), themed.end());
    Lite::SystemUtilities::ShuffleList(list, &kSPRandom);
    list.insert(list.begin(), progressMilestone);
    const int take = std::min(kSPRandom.Next(3) + 1, static_cast<int>(list.size()));
    list.resize(static_cast<size_t>(take));  // list.GetRange(0, take)
    dotnet::ListSort(list, MilestoneCompare);
    return list;
}

// ExplorationContract.cs:389
std::vector<ProgressMilestone> ExplorationContract::GetSeedMilestones(KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(MissionSeed());
        generator = own.get();
    }
    ProgressTracking* pt = ProgressTracking::Instance();
    if (pt == nullptr) return {};
    const bool flag = pt->NodeComplete({"FirstLaunch"});
    const bool flag2 = pt->NodeComplete({"ReachedSpace"});
    const bool flag3 = pt->NodeComplete({Home()->name(), "Orbit"});
    const bool flag4 = pt->NodeComplete({Home()->name(), "Science"});
    CelestialBody* homeBody = CareerEnvironment::Current().HomeBody();
    if (!ContractDefsLite::B("Progression", "", "DisableTutorialContracts") && (!flag || !flag4 || !flag2 || !flag3)) {
        bool flag5 = false;
        bool flag6 = false;
        bool flag7 = false;
        bool flag8 = false;
        std::vector<ExplorationContract*> currentContracts = ksp::Contracts::ContractSystem::Instance->GetCurrentContracts<ExplorationContract>();
        int num = static_cast<int>(currentContracts.size());
        while (num-- > 0) {
            ExplorationContract* explorationContract = currentContracts[static_cast<size_t>(num)];
            if (explorationContract->TrackingParameters().size() < 1) continue;
            int count = static_cast<int>(explorationContract->TrackingParameters().size());
            while (count-- > 0) {
                ProgressMilestone& milestone = explorationContract->TrackingParameters()[static_cast<size_t>(count)]->milestone;
                switch (milestone.type) {
                    case ProgressType::ORBIT:
                        if (milestone.body == Home()) flag8 = true;
                        break;
                    case ProgressType::FIRSTLAUNCH: flag5 = true; break;
                    case ProgressType::SCIENCE:
                        if (milestone.body == Home()) flag6 = true;
                        break;
                    case ProgressType::REACHSPACE: flag7 = true; break;
                    default: break;
                }
            }
        }
        if (!flag && !flag5) return {ProgressMilestone(Home(), ProgressType::FIRSTLAUNCH)};
        if (!flag4 && !flag6) return {ProgressMilestone(Home(), ProgressType::SCIENCE)};
        if (!flag2 && !flag7) return {ProgressMilestone(Home(), ProgressType::REACHSPACE)};
        if (!flag3 && !flag8) return {ProgressMilestone(Home(), ProgressType::ORBIT)};
    }
    if (ContractDefsLite::B("Progression", "", "DisableProgressionContracts")) return {};
    std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesReached(true);
    std::vector<ProgressMilestone> list;
    std::vector<ProgressMilestone> list2;
    const bool flag9 = static_cast<int>(bodiesProgress.size()) >= static_cast<int>(FlightRegistry::Bodies().size()) / 2;
    if (!flag9) bodiesProgress.erase(std::remove(bodiesProgress.begin(), bodiesProgress.end(), Planetarium::fetch->Sun), bodiesProgress.end());
    std::vector<ProgressMilestone> list3;
    int count2 = static_cast<int>(bodiesProgress.size());
    while (count2-- > 0) {
        list3.clear();
        CelestialBody* celestialBody = bodiesProgress[static_cast<size_t>(count2)];
        KSPAchievements::CelestialBodySubtree* progressTree = celestialBody->progressTree;
        if (progressTree == nullptr) {
            list.insert(list.end(), list3.begin(), list3.end());
            continue;
        }
        const bool anyBodyProgress = PU::GetAnyBodyProgress(celestialBody, MannedStatus::MANNED);
        if (celestialBody->isHomeWorld) {
            if (list3.size() <= 0 && !progressTree->orbit->IsComplete()) list3.emplace_back(celestialBody, ProgressType::ORBIT);
            if (list3.size() <= 0 && evaUnlocked && anyBodyProgress && !progressTree->spacewalk->IsComplete()) list3.emplace_back(celestialBody, ProgressType::SPACEWALK);
            if (list3.size() <= 0 && !progressTree->returnFromOrbit->IsComplete()) list3.emplace_back(celestialBody, ProgressType::ORBITRETURN);
        } else {
            if (list3.size() <= 0 && !progressTree->flyBy->IsComplete()) list3.emplace_back(celestialBody, ProgressType::FLYBY);
            if (list3.size() <= 0 && !progressTree->orbit->IsComplete()) list3.emplace_back(celestialBody, ProgressType::ORBIT);
            if (list3.size() <= 0) {
                if (!progressTree->science->IsComplete()) list3.emplace_back(celestialBody, ProgressType::SCIENCE);
                if (evaUnlocked && anyBodyProgress && !progressTree->spacewalk->IsComplete()) list3.emplace_back(celestialBody, ProgressType::SPACEWALK);
                if (CU::IsFlyablePlanet(celestialBody) && !progressTree->flight->IsComplete()) list3.emplace_back(celestialBody, ProgressType::FLIGHT);
            }
            if (list3.size() <= 0) {
                if (!progressTree->returnFromFlyby->IsComplete()) list3.emplace_back(celestialBody, ProgressType::FLYBYRETURN);
                if (!progressTree->returnFromOrbit->IsComplete()) list3.emplace_back(celestialBody, ProgressType::ORBITRETURN);
                if (!CU::IsGasGiant(celestialBody)) {
                    if (!progressTree->landing->IsComplete()) list3.emplace_back(celestialBody, ProgressType::LANDING);
                    if (celestialBody->ocean && !progressTree->splashdown->IsComplete()) list3.emplace_back(celestialBody, ProgressType::SPLASHDOWN);
                }
            }
        }
        if (list3.size() > 0) {
            list.insert(list.end(), list3.begin(), list3.end());
            continue;
        }
        if (celestialBody != Home() && !CU::IsGasGiant(celestialBody)) {
            if (!progressTree->returnFromSurface->IsComplete()) {
                if (celestialBody->atmosphere)
                    list2.emplace_back(celestialBody, ProgressType::LANDINGRETURN);
                else
                    list3.emplace_back(celestialBody, ProgressType::LANDINGRETURN);
            }
            if (evaUnlocked && anyBodyProgress && !progressTree->surfaceEVA->IsComplete()) list3.emplace_back(celestialBody, ProgressType::SURFACEEVA);
            if (evaUnlocked && flagsUnlocked && anyBodyProgress && !progressTree->flagPlant->IsComplete()) list3.emplace_back(celestialBody, ProgressType::FLAGPLANT);
            if (dockUnlocked && anyBodyProgress && !progressTree->baseConstruction->IsComplete()) list2.emplace_back(celestialBody, ProgressType::BASECONSTRUCTION);
        }
        if (celestialBody->isHomeWorld && !progressTree->escape->IsComplete() && PU::ReachedHomeBodies()) list3.emplace_back(celestialBody, ProgressType::ESCAPE);
        if (rendezvousUnlocked) {
            if (!progressTree->rendezvous->IsComplete())
                list3.emplace_back(celestialBody, ProgressType::RENDEZVOUS);
            else if (anyBodyProgress && !progressTree->crewTransfer->IsComplete())
                list3.emplace_back(celestialBody, ProgressType::CREWTRANSFER);
        }
        if (dockUnlocked && !progressTree->docking->IsComplete()) list3.emplace_back(celestialBody, ProgressType::DOCKING);
        if (dockUnlocked && anyBodyProgress && !progressTree->stationConstruction->IsComplete()) list2.emplace_back(celestialBody, ProgressType::STATIONCONSTRUCTION);
        list.insert(list.end(), list3.begin(), list3.end());
    }
    bool flag10 = true;
    int count3 = static_cast<int>(list.size());
    while (count3-- > 0) {
        ProgressMilestone& progressMilestone = list[static_cast<size_t>(count3)];
        switch (progressMilestone.type) {
            case ProgressType::FLYBYRETURN:
            case ProgressType::ORBITRETURN:
                if (progressMilestone.body == homeBody || progressMilestone.body->referenceBody() == homeBody) flag10 = false;
                break;
            case ProgressType::FLIGHT:
            case ProgressType::FLYBY:
            case ProgressType::LANDING:
            case ProgressType::ORBIT:
            case ProgressType::SCIENCE:
            case ProgressType::SPACEWALK:
            case ProgressType::SPLASHDOWN: flag10 = false; break;
            default: break;
        }
    }
    if (flag10) {
        std::vector<CelestialBody*> list4 = flag9 ? PU::GetNextUnreached(1) : PU::GetNextUnreached(1, [](CelestialBody* body) { return body != Planetarium::fetch->Sun; });
        int count4 = static_cast<int>(list4.size());
        while (count4-- > 0) list.emplace_back(list4[static_cast<size_t>(count4)], ProgressType::FLYBY);
    }
    if (list.size() < 1) list = list2;
    if (list.size() > 0) {
        std::vector<CelestialBody*> list5;
        int count5 = static_cast<int>(list.size());
        while (count5-- > 0) {
            CelestialBody* body2 = list[static_cast<size_t>(count5)].body;
            if (body2 != nullptr && std::find(list5.begin(), list5.end(), body2) == list5.end()) list5.push_back(body2);
        }
        CelestialBody* celestialBody2 = WeightedBodyChoice(list5, generator);
        int count6 = static_cast<int>(list.size());
        while (count6-- > 0) {
            if (list[static_cast<size_t>(count6)].body != celestialBody2) list.erase(list.begin() + count6);  // list.Remove(list[count6]): that element
        }
    }
    return list;
}

// ExplorationContract.cs:686
std::vector<ProgressMilestone> ExplorationContract::GetThemedMilestones(ProgressMilestone& seed) {
    std::vector<ProgressMilestone> list;
    CelestialBody* body = seed.body;
    KSPAchievements::CelestialBodySubtree* progressTree = body->progressTree;
    if (progressTree == nullptr) return list;
    const bool anyBodyProgress = PU::GetAnyBodyProgress(body, MannedStatus::MANNED);
    const bool hasSolidSurface = body->hasSolidSurface;
    const bool flag = hasSolidSurface && body->ocean;
    const auto add = [&](ProgressType t) { list.emplace_back(body, t); };
    if (!body->isHomeWorld) {
        switch (seed.type) {
            case ProgressType::FLIGHT:
                if (hasSolidSurface && !progressTree->landing->IsComplete()) add(ProgressType::LANDING);
                if (hasSolidSurface && flag && !progressTree->splashdown->IsComplete()) add(ProgressType::SPLASHDOWN);
                break;
            case ProgressType::FLYBY:
                if (!progressTree->returnFromFlyby->IsComplete()) add(ProgressType::FLYBYRETURN);
                if (!progressTree->science->IsComplete()) add(ProgressType::SCIENCE);
                break;
            case ProgressType::LANDING:
                if (!progressTree->returnFromSurface->IsComplete()) add(ProgressType::LANDINGRETURN);
                if (hasSolidSurface && evaUnlocked && anyBodyProgress && !progressTree->surfaceEVA->IsComplete()) add(ProgressType::SURFACEEVA);
                if (hasSolidSurface && evaUnlocked && flagsUnlocked && anyBodyProgress && !progressTree->flagPlant->IsComplete()) add(ProgressType::FLAGPLANT);
                break;
            case ProgressType::ORBIT:
                if (!progressTree->returnFromOrbit->IsComplete()) add(ProgressType::ORBITRETURN);
                if (!progressTree->science->IsComplete()) add(ProgressType::SCIENCE);
                if (evaUnlocked && anyBodyProgress && !progressTree->spacewalk->IsComplete()) add(ProgressType::SPACEWALK);
                break;
            case ProgressType::FLYBYRETURN:
            case ProgressType::ORBITRETURN:
                if (rendezvousUnlocked && !progressTree->rendezvous->IsComplete()) add(ProgressType::RENDEZVOUS);
                if (dockUnlocked && !progressTree->docking->IsComplete()) add(ProgressType::DOCKING);
                if (rendezvousUnlocked && anyBodyProgress && !progressTree->crewTransfer->IsComplete()) add(ProgressType::CREWTRANSFER);
                break;
            case ProgressType::SCIENCE:
                if (!progressTree->returnFromFlyby->IsComplete()) add(ProgressType::FLYBYRETURN);
                if (!progressTree->returnFromOrbit->IsComplete()) add(ProgressType::ORBITRETURN);
                if (hasSolidSurface && evaUnlocked && anyBodyProgress && !progressTree->surfaceEVA->IsComplete()) add(ProgressType::SURFACEEVA);
                break;
            case ProgressType::SPACEWALK:
                if (!progressTree->returnFromFlyby->IsComplete()) add(ProgressType::FLYBYRETURN);
                if (!progressTree->returnFromOrbit->IsComplete()) add(ProgressType::ORBITRETURN);
                if (rendezvousUnlocked && !progressTree->rendezvous->IsComplete()) add(ProgressType::RENDEZVOUS);
                if (rendezvousUnlocked && anyBodyProgress && !progressTree->crewTransfer->IsComplete()) add(ProgressType::CREWTRANSFER);
                break;
            case ProgressType::SPLASHDOWN:
                if (!progressTree->returnFromSurface->IsComplete()) add(ProgressType::LANDINGRETURN);
                if (hasSolidSurface && evaUnlocked && anyBodyProgress && !progressTree->surfaceEVA->IsComplete()) add(ProgressType::SURFACEEVA);
                break;
            default: break;
        }
    } else {
        switch (seed.type) {
            case ProgressType::SPACEWALK:
                if (!progressTree->returnFromOrbit->IsComplete()) add(ProgressType::ORBITRETURN);
                if (rendezvousUnlocked && !progressTree->rendezvous->IsComplete()) add(ProgressType::RENDEZVOUS);
                if (rendezvousUnlocked && anyBodyProgress && !progressTree->crewTransfer->IsComplete()) add(ProgressType::CREWTRANSFER);
                break;
            case ProgressType::ORBITRETURN:
                if (rendezvousUnlocked && !progressTree->rendezvous->IsComplete()) add(ProgressType::RENDEZVOUS);
                if (dockUnlocked && !progressTree->docking->IsComplete()) add(ProgressType::DOCKING);
                if (rendezvousUnlocked && anyBodyProgress && !progressTree->crewTransfer->IsComplete()) add(ProgressType::CREWTRANSFER);
                break;
            default: break;
        }
    }
    return list;
}

}  // namespace ksp::FinePrint::Contracts

namespace ksp::FinePrint {

// WP-38 registration of the FinePrint types and the WP-36 CareerEnvironment hooks (see fineprint.hpp)
void Install() {
    using ksp::Contracts::ContractSystem;
    namespace L = ksp::Contracts::Lite;
    using ksp::Contracts::ContractDefsLite;
    ContractSystem::RegisterContractType("ExplorationContract", [] { return std::make_unique<Contracts::ExplorationContract>(); });
    ContractSystem::RegisterParameterType("ProgressTrackingParameter", [] { return std::make_unique<ksp::Contracts::Parameters::ProgressTrackingParameter>(); });
    CareerEnvironment& env = CareerEnvironment::Current();
    env.worldFirstStandardReward = [](FinePrint::Utilities::ProgressRewardType r, Currency c, FinePrint::Utilities::ProgressType p, CelestialBody* b) {
        return L::ProgressUtilities::WorldFirstStandardReward(r, c, p, b);
    };
    env.worldFirstIntervalReward = [](FinePrint::Utilities::ProgressRewardType r, Currency c, FinePrint::Utilities::ProgressType p, CelestialBody* b, int i, int n) {
        return L::ProgressUtilities::WorldFirstIntervalReward(r, c, p, b, i, n);
    };
    env.findNextRecord = [](double cur, double max, double round, int& interval) { return L::ProgressUtilities::FindNextRecord(cur, max, round, interval); };
    env.contractDefsPresent = [] { return ContractDefsLite::Loaded(); };
    env.progressionRecordSplit = [] { return ContractDefsLite::I("Progression", "", "RecordSplit"); };
    env.progressionMaxDepthRecord = [] { return ContractDefsLite::D("Progression", "", "MaxDepthRecord"); };
    env.progressionMaxDistanceRecord = [] { return ContractDefsLite::D("Progression", "", "MaxDistanceRecord"); };
    env.progressionMaxSpeedRecord = [] { return ContractDefsLite::D("Progression", "", "MaxSpeedRecord"); };
    env.greatCircleDistance = [](CelestialBody* body, double la1, double lo1, double la2, double lo2) {
        return L::CelestialUtilities::GreatCircleDistance(body, la1, lo1, la2, lo2);
    };
}

}  // namespace ksp::FinePrint
