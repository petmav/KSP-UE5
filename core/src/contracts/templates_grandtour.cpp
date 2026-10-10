// WP-37: Contracts.Templates.GrandTour (Assembly-CSharp/Contracts/Templates/GrandTour.cs).
//
// The parameters the template builds (VesselSystemsParameter, VesselDestinationParameter, LocationAndSituationParameter) are WP-38's: they come
// from TemplateEnvironment::grandTour* factories (unset: Generate returns false after the draws, like a missing parameter class would stop it).
#include <algorithm>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/contracts/text_gen.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp::Contracts::Templates {

using KSP::Localization::Localizer;
namespace PU = Lite::ProgressUtilities;
namespace CU = Lite::CelestialUtilities;
using Lite::ProgressType;
using Sit = Vessel::Situations;

namespace {
float Def(const char* sub, const char* key) { return ContractDefsLite::F("Grand", sub, key); }
CelestialBody* Home() { return Planetarium::fetch->Home; }
template <class T>
bool Has(const std::vector<T>& v, const T& x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}
template <class T>
void RemoveOne(std::vector<T>& v, const T& x) {
    auto it = std::find(v.begin(), v.end(), x);
    if (it != v.end()) v.erase(it);
}
}  // namespace

// GrandTour.cs:30
int GrandTour::TotalBodies() const { return !finalDestination ? static_cast<int>(targetBodies.size()) : static_cast<int>(targetBodies.size()) + 1; }

// GrandTour.cs:44
std::string GrandTour::FormattedDestinations() const {
    std::vector<std::string> list;
    for (size_t i = 0; i < targetBodies.size(); ++i) list.push_back(Localizer::Format("#autoLOC_7001301", {targetBodies[i]->displayName()}));
    return Lite::StringUtilities::ThisThisAndThat(list);
}

// GrandTour.cs:58
bool GrandTour::MeetRequirements() {
    return fx(static_cast<float>(Lite::HashNumber(static_cast<uint32_t>(MissionSeed())) % 100u)) <= fx(Def("", "Rarity"));
}

// GrandTour.cs:68
std::string GrandTour::GetDescription() {
    const std::string text = focused ? focusBody->displayName() : ((targetBodies.size() <= 0) ? CareerEnvironment::Current().HomeBody()->displayName() : targetBodies[0]->displayName());
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), text, text, MissionSeed(), true, false, true);
}

// GrandTour.cs:75
std::string GrandTour::GetSynopsys() {
    std::string text, text2, text3, text4, text5;
    text = (progressType != ProgressType::LANDING) ? Localizer::Format("#autoLOC_6200001", {FormattedDestinations()}) : Localizer::Format("#autoLOC_6200000", {FormattedDestinations()});
    if (finalDestination) {
        text2 = Localizer::Format("#autoLOC_272642");
        switch (finalSituation) {
            case Sit::SPLASHED: text3 = Localizer::Format("#autoLOC_272656"); break;
            case Sit::LANDED: text3 = Localizer::Format("#autoLOC_272650"); break;
            case Sit::ORBITING: text3 = Localizer::Format("#autoLOC_272653"); break;
            case Sit::SUB_ORBITAL: text3 = Localizer::Format("#autoLOC_272659"); break;
            case Sit::FLYING: text3 = Localizer::Format("#autoLOC_272647"); break;
            default: break;
        }
        text4 = finalBody->displayName();
    }
    text5 = Localizer::Format("#autoLOC_272666");
    return Localizer::Format("#autoLOC_6002377", {text, text2, text3, text4, text5});
}

// GrandTour.cs:110
std::string GrandTour::GetTitle() {
    KSPRandom kSPRandom(MissionSeed());
    std::string text, tmpl, text2, text3;
    text = Localizer::Format("#autoLOC_272675");
    if (finalDestination) {
        switch (prestige) {
            case ContractPrestige::Trivial: tmpl = Localizer::Format("#autoLOC_272682"); break;
            case ContractPrestige::Significant: tmpl = Localizer::Format("#autoLOC_272685"); break;
            case ContractPrestige::Exceptional: tmpl = Localizer::Format("#autoLOC_272688"); break;
        }
    }
    const std::string total = dotnet::FormatInt64(TotalBodies());
    if (focused)
        text2 = (focusBody == Planetarium::fetch->Sun) ? std::string() : (focusBody->bodyAdjectiveDisplayName + " " + total + " ");
    else
        text2 = targetBodies[static_cast<size_t>(kSPRandom.Next(0, static_cast<int32_t>(targetBodies.size())))]->bodyAdjectiveDisplayName + " " + total + " ";
    if (!focused) {
        if (progressType == ProgressType::FLYBY) {
            switch (prestige) {
                case ContractPrestige::Trivial: text3 = Localizer::Format("#autoLOC_272705"); break;
                case ContractPrestige::Significant: text3 = Localizer::Format("#autoLOC_272708"); break;
                case ContractPrestige::Exceptional: text3 = Localizer::Format("#autoLOC_272711"); break;
            }
        } else {
            switch (prestige) {
                case ContractPrestige::Trivial: text3 = Localizer::Format("#autoLOC_272720"); break;
                case ContractPrestige::Significant: text3 = Localizer::Format("#autoLOC_272723"); break;
                case ContractPrestige::Exceptional: text3 = Localizer::Format("#autoLOC_272726"); break;
            }
        }
    } else {
        text3 = Localizer::Format("#autoLOC_272732");
    }
    tmpl = Localizer::Format(tmpl, {text3});
    return Localizer::Format("#autoLOC_6002376", {text, tmpl, text2, text3});
}

// GrandTour.cs:175
std::string GrandTour::MessageCompleted() { return Localizer::Format("#autoLOC_272741", {dotnet::FormatInt64(TotalBodies())}); }

// GrandTour.cs:180
void GrandTour::OnAccepted() {
    if (TemplateEnvironment::grandTourAccepted) TemplateEnvironment::grandTourAccepted(*this, SaveEnvironment::Current().currentGame->launchID);
}

// GrandTour.cs:189
std::string GrandTour::GetNotes() {
    if (finalDestination && CU::IsGasGiant(finalBody)) {
        if (finalSituation != Sit::FLYING && finalSituation != Sit::SUB_ORBITAL) return std::string();
        std::string text = Localizer::Format("#autoLOC_272752");
        if (SaveEnvironment::Current().loadedScene == GameScenes::SPACECENTER) text += "\n";
        return text;
    }
    return std::string();
}

// GrandTour.cs:208
void GrandTour::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "Template.GrandTour", "progressType", progressType, ProgressType::FLYBY);
    Lite::LoadNode(node, "Template.GrandTour", "finalDestination", finalDestination, false);
    Lite::LoadNode(node, "Template.GrandTour", "focused", focused, false);
    if (node.HasValue("body")) {
        targetBodies.clear();
        for (const auto& v : node.GetValues("body")) targetBodies.push_back(FlightRegistry::Bodies().at(static_cast<size_t>(dotnet::ParseInt32(v))));
    }
    const auto loadBody = [&](const char* key, CelestialBody*& dst, CelestialBody* dflt) {
        dst = dflt;
        if (node.HasValue(key)) {
            int idx = 0;
            if (dotnet::TryParseInt32(*node.GetValue(key), idx)) dst = FlightRegistry::Bodies().at(static_cast<size_t>(idx));
        }
    };
    if (focused) loadBody("focusBody", focusBody, Planetarium::fetch->Home);
    if (finalDestination) {
        loadBody("finalBody", finalBody, Planetarium::fetch->Home);
        Lite::LoadNode(node, "Template.GrandTour", "finalSituation", finalSituation, Sit::ORBITING);
    }
}

// GrandTour.cs:234
void GrandTour::OnSave(ConfigNode& node) {
    node.AddValue("progressType", static_cast<int32_t>(progressType));
    node.AddValue("finalDestination", finalDestination);
    node.AddValue("focused", focused);
    for (size_t i = 0; i < targetBodies.size(); ++i) node.AddValue("body", static_cast<int32_t>(targetBodies[i]->flightGlobalsIndex));
    if (focused) node.AddValue("focusBody", static_cast<int32_t>(focusBody->flightGlobalsIndex));
    if (finalDestination) {
        node.AddValue("finalBody", static_cast<int32_t>(finalBody->flightGlobalsIndex));
        node.AddValue("finalSituation", static_cast<int32_t>(finalSituation));
    }
}

// GrandTour.cs:255
bool GrandTour::Generate() {
    KSPRandom generator(MissionSeed());
    targetBodies.clear();
    focused = false;
    focusBody = nullptr;
    if (Prestige() == ContractPrestige::Exceptional && Lite::CoinFlip(&generator)) AttemptFocusedTour(&generator);
    if (!focused) AttemptRandomTour(&generator);
    AttemptFinalDestination(&generator);
    if (targetBodies.size() <= 0) return false;
    const std::string text = Localizer::Format("#autoLOC_7001016");
    if (!TemplateEnvironment::makeVesselSystemsParameter || !TemplateEnvironment::makeVesselDestinationParameter ||
        !TemplateEnvironment::makeLocationAndSituationParameter)
        return false;
    AddParameter(TemplateEnvironment::makeVesselSystemsParameter(text));
    int count = static_cast<int>(targetBodies.size());
    for (int i = 0; i < count; ++i) {
        const int targetType = (progressType == ProgressType::FLYBY) ? 7 /* FlightLog.EntryType.Flyby */ : 6 /* FlightLog.EntryType.Land */;
        AddParameter(TemplateEnvironment::makeVesselDestinationParameter(targetBodies[static_cast<size_t>(i)], targetType));
    }
    if (finalDestination) AddParameter(TemplateEnvironment::makeLocationAndSituationParameter(finalBody, finalSituation, text, true));
    switch (prestige) {
        default:
            if (!focused) {
                AddKeywords({"Pioneer", "Commercial", "Scientific"});
            } else {
                AddKeywords({"Record"});
            }
            break;
        case ContractPrestige::Significant: AddKeywords({"Commercial", "Scientific"}); break;
        case ContractPrestige::Trivial: AddKeywords({"Commercial"}); break;
    }
    float num = 0.0f, num2 = 0.0f, num3 = 0.0f, num4 = 0.0f, num5 = 0.0f, num6 = 0.0f, num7 = 0.0f;
    count = static_cast<int>(targetBodies.size());
    for (int j = 0; j < count; ++j) {
        CelestialBody* celestialBody = targetBodies[static_cast<size_t>(j)];
        const float num8 = PU::ScoreProgressType(progressType, celestialBody);
        const float recoveryValue = celestialBody->scienceValues.RecoveryValue;
        num = f32store(fx(num) + fx(Def("Funds", "BaseAdvance")) * fx(num8) * fx(recoveryValue));
        num2 = f32store(fx(num2) + fx(Def("Funds", "BaseReward")) * fx(num8) * fx(recoveryValue));
        num3 = f32store(fx(num3) + fx(Def("Funds", "BaseFailure")) * fx(num8) * fx(recoveryValue));
        num4 = f32store(fx(num4) + fx(Def("Science", "BaseReward")) * fx(num8));
        num5 = f32store(fx(num5) + fx(Def("Reputation", "BaseReward")) * fx(num8));
        num6 = f32store(fx(num6) + fx(Def("Reputation", "BaseFailure")) * fx(num8));
        num7 = f32store(fx(num7) + fx(static_cast<float>(ContractDefsLite::I("Grand", "Expiration", "DeadlineDays"))) * fx(recoveryValue));
    }
    SetExpiry(ContractDefsLite::I("Grand", "Expiration", "MinimumExpireDays"), ContractDefsLite::I("Grand", "Expiration", "MaximumExpireDays"));
    SetDeadlineDays(num7);
    SetFunds(num, num2, num3);
    SetScience(num4);
    SetReputation(num5, num6);
    return true;
}

// GrandTour.cs:334
void GrandTour::AttemptRandomTour(KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(MissionSeed());
        generator = own.get();
    }
    std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesProgressNoReach(ProgressType::FLYBY, true, [](CelestialBody* cb) { return cb != Home() && cb != Planetarium::fetch->Sun; });
    std::vector<CelestialBody*> bodiesProgress2 = PU::GetBodiesProgressNoReach(
        ProgressType::LANDING, true, [](CelestialBody* cb) { return cb != Home() && cb != Planetarium::fetch->Sun && cb->hasSolidSurface; });
    if (prestige == ContractPrestige::Trivial) bodiesProgress2.clear();
    int num = 3;
    if (prestige == ContractPrestige::Significant)
        num = 4;
    else if (prestige == ContractPrestige::Exceptional)
        num = 6;
    if (static_cast<int>(bodiesProgress.size()) < num && static_cast<int>(bodiesProgress2.size()) < num) return;
    std::vector<CelestialBody*> list;
    if (static_cast<int>(bodiesProgress2.size()) >= num && static_cast<int>(bodiesProgress.size()) < num) {
        progressType = ProgressType::LANDING;
        list.insert(list.end(), bodiesProgress2.begin(), bodiesProgress2.end());
    }
    if (static_cast<int>(bodiesProgress2.size()) < num && static_cast<int>(bodiesProgress.size()) >= num) {
        progressType = ProgressType::FLYBY;
        list.insert(list.end(), bodiesProgress.begin(), bodiesProgress.end());
    }
    if (static_cast<int>(bodiesProgress2.size()) >= num && static_cast<int>(bodiesProgress.size()) >= num) {
        progressType = Lite::CoinFlip(generator) ? ProgressType::FLYBY : ProgressType::LANDING;
        const auto& src = (progressType == ProgressType::LANDING) ? bodiesProgress2 : bodiesProgress;
        list.insert(list.end(), src.begin(), src.end());
    }
    for (int i = 0; i < num; ++i) {
        CelestialBody* item = WeightedBodyChoice(list, generator);
        RemoveOne(list, item);
        if (!Has(targetBodies, item)) targetBodies.push_back(item);
    }
}

// GrandTour.cs:386
void GrandTour::AttemptFocusedTour(KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(MissionSeed());
        generator = own.get();
    }
    std::vector<CelestialBody*> focusedChallenges = GetFocusedChallenges(ProgressType::LANDING);
    std::vector<CelestialBody*> focusedChallenges2 = GetFocusedChallenges(ProgressType::FLYBY);
    if (prestige == ContractPrestige::Trivial) focusedChallenges.clear();
    if (focusedChallenges.size() <= 0 && focusedChallenges2.size() <= 0) return;
    if (focusedChallenges.size() > 0 && focusedChallenges2.size() <= 0) progressType = ProgressType::LANDING;
    if (focusedChallenges.size() <= 0 && focusedChallenges2.size() > 0) progressType = ProgressType::FLYBY;
    if (focusedChallenges.size() > 0 && focusedChallenges2.size() > 0) progressType = Lite::CoinFlip(generator) ? ProgressType::FLYBY : ProgressType::LANDING;
    if (progressType == ProgressType::FLYBY) {
        focused = true;
        focusBody = WeightedBodyChoice(focusedChallenges2, generator);
        targetBodies.push_back(focusBody);
        std::vector<CelestialBody*> children = CU::ChildrenOf(focusBody);
        targetBodies.insert(targetBodies.end(), children.begin(), children.end());
    }
    if (progressType != ProgressType::LANDING) return;
    focused = true;
    focusBody = WeightedBodyChoice(focusedChallenges, generator);
    if (!CU::IsGasGiant(focusBody)) targetBodies.push_back(focusBody);
    std::vector<CelestialBody*> list = CU::ChildrenOf(focusBody);
    for (size_t i = 0; i < list.size(); ++i)
        if (!CU::IsGasGiant(list[i])) targetBodies.push_back(list[i]);
}

// GrandTour.cs:443
std::vector<CelestialBody*> GrandTour::GetFocusedChallenges(ProgressType type) {
    std::vector<CelestialBody*> list;
    const auto& all = FlightRegistry::Bodies();
    for (size_t i = 0; i < all.size(); ++i)
        if (CU::ChildrenOf(all[i]).size() > 1) list.push_back(all[i]);
    std::vector<CelestialBody*> list2;
    Lite::ProgressMilestone progressMilestone(Planetarium::fetch->Home, type);
    for (size_t j = 0; j < list.size(); ++j) {
        CelestialBody* celestialBody = list[j];
        std::vector<CelestialBody*> list3 = CU::ChildrenOf(celestialBody);
        list3.push_back(celestialBody);
        int num = 0;
        bool flag = true;
        for (size_t k = 0; k < list3.size(); ++k) {
            progressMilestone.body = list3[k];
            if (progressMilestone.possible()) {
                num++;
                if (!progressMilestone.complete()) {
                    flag = false;
                    break;
                }
            }
        }
        if (num > 0 && flag) list2.push_back(celestialBody);
    }
    return list2;
}

// GrandTour.cs:485
void GrandTour::AttemptFinalDestination(KSPRandom* generator) {
    finalDestination = false;
    finalBody = Planetarium::fetch->Home;
    finalSituation = Sit::ORBITING;
    if (prestige == ContractPrestige::Trivial || targetBodies.size() < 2) return;
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(MissionSeed());
        generator = own.get();
    }
    finalDestination = Has(targetBodies, Planetarium::fetch->Home) || Lite::CoinFlip(generator);
    if (!finalDestination) return;
    if (focused) {
        finalBody = Has(targetBodies, Planetarium::fetch->Home) ? Planetarium::fetch->Home : targetBodies[static_cast<size_t>(generator->Next(0, static_cast<int32_t>(targetBodies.size())))];
        targetBodies.erase(std::remove(targetBodies.begin(), targetBodies.end(), finalBody), targetBodies.end());
    } else {
        finalBody = targetBodies[static_cast<size_t>(generator->Next(0, static_cast<int32_t>(targetBodies.size())))];
        targetBodies.erase(std::remove(targetBodies.begin(), targetBodies.end(), finalBody), targetBodies.end());
        if (Lite::CoinFlip(generator)) finalBody = Planetarium::fetch->Home;
    }
    std::vector<Sit> list = {Sit::ORBITING, Sit::SUB_ORBITAL};
    if (CU::IsFlyablePlanet(finalBody)) list.push_back(Sit::FLYING);
    if (!CU::IsGasGiant(finalBody)) {
        if (PU::GetBodyProgress(ProgressType::LANDING, finalBody)) list.push_back(Sit::LANDED);
        if (finalBody->ocean && PU::GetBodyProgress(ProgressType::SPLASHDOWN, finalBody)) list.push_back(Sit::SPLASHED);
    }
    if (finalBody == Planetarium::fetch->Home) {
        list.clear();
        list.push_back((progressType == ProgressType::LANDING) ? Sit::LANDED : Sit::SUB_ORBITAL);
    }
    finalSituation = list[static_cast<size_t>(generator->Next(0, static_cast<int32_t>(list.size())))];
}

// GrandTour.cs:547
std::string GrandTour::GetHashString() { return dotnet::FormatInt64(Lite::SuperSeed(*this)); }

}  // namespace ksp::Contracts::Templates
