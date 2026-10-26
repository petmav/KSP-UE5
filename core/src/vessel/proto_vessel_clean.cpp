// WP-19c: the ProtoVessel members that act on the running game (ProtoVessel.cs:994 GetSituationString, :1163 Clean, :1215-1240 the
// currencies, :1242 CacheLocalStrings). The data side is WP-04's src/save/proto_vessel.cpp.
#include <algorithm>
#include <cmath>
#include <memory>

#include "ksp/career/format.hpp"
#include "ksp/career/funding.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/science/science.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
std::string L(const char* tag) { return Localizer::Format(tag); }

// FinePrint/Utilities/StringUtilities.cs:467 FormattedCurrencies for the arguments ProtoVessel.Clean passes (symbols false, verbose false,
// TransactionReasons.None, TextStyling.None): no modifier query, no colour markup; one "<N0 value> <currency name> " entry per currency
// whose scenario exists and whose N0 text is not "0"
std::vector<std::string> FormattedCurrenciesPlain(float funds, float science, float reputation) {
    Game* g = HighLogic::CurrentGame();
    if (g == nullptr) throw NullReferenceException("HighLogic.CurrentGame is null");
    if (g->Mode != Game::Modes::CAREER && g->Mode != Game::Modes::SCIENCE_SANDBOX) return {};
    std::vector<std::string> list;
    if (Funding::Instance != nullptr) {
        const std::string text6 = career::FormatNumber(funds + 0.0f, "N0");
        if (text6 != "0") list.push_back(text6 + " " + L("#autoLOC_7001031") + " ");
    }
    if (ResearchAndDevelopment::Instance != nullptr) {
        const std::string text6 = career::FormatNumber(science + 0.0f, "N0");
        if (text6 != "0") list.push_back(text6 + " " + L("#autoLOC_7001032") + " ");
    }
    if (Reputation::Instance != nullptr) {
        const std::string text6 = career::FormatNumber(reputation + 0.0f, "N0");
        if (text6 != "0") list.push_back(text6 + " " + L("#autoLOC_7001033") + " ");
    }
    return list;
}
}  // namespace

// ProtoVessel.cs:994
std::string ProtoVessel::GetSituationString(const ProtoVessel& pv, const std::vector<CelestialBody*>* bodies) {
    if (!pv.orbitSnapShot) throw NullReferenceException("ProtoVessel.orbitSnapShot is null");
    std::string text;
    if (bodies != nullptr && static_cast<int>(bodies->size()) > pv.orbitSnapShot->ReferenceBodyIndex) {
        text = (*bodies)[static_cast<size_t>(pv.orbitSnapShot->ReferenceBodyIndex)]->displayName();
        switch (pv.situation) {
            case Vessel::Situations::SUB_ORBITAL: return L("#autoLOC_145790");
            case Vessel::Situations::FLYING: return L("#autoLOC_145786");
            case Vessel::Situations::LANDED:
                if (pv.landedAt.empty()) return Localizer::Format("#autoLOC_6100012", {text});
                if (pv.displaylandedAt.empty())
                    return Localizer::Format("#autoLOC_6100012",
                                             {ResearchAndDevelopment::GetMiniBiomedisplayNameByScienceID(Vessel::GetLandedAtString(pv.landedAt), true)});
                return Localizer::Format("#autoLOC_6100012", {Localizer::Format(pv.displaylandedAt)});
            case Vessel::Situations::SPLASHED: return L("#autoLOC_145789");
            case Vessel::Situations::PRELAUNCH: return L("#autoLOC_145785");
            default: return L("#autoLOC_145793");
            case Vessel::Situations::DOCKED: return L("#autoLOC_145792");
            case Vessel::Situations::ESCAPING: return Localizer::Format("#autoLOC_145791", {text});
            case Vessel::Situations::ORBITING: return Localizer::Format("#autoLOC_145788", {text});
        }
    }
    switch (pv.situation) {
        case Vessel::Situations::SUB_ORBITAL: return L("#autoLOC_145805");
        case Vessel::Situations::FLYING: return L("#autoLOC_145801");
        case Vessel::Situations::LANDED:
            if (pv.landedAt.empty()) return L("#autoLoc_6002161");  // the C# caches "#autoLoc_6002161" (lower-case 'oc', sic)
            if (pv.displaylandedAt.empty())
                return Localizer::Format("#autoLOC_6100016",
                                         {ResearchAndDevelopment::GetMiniBiomedisplayNameByScienceID(Vessel::GetLandedAtString(pv.landedAt), true)});
            return Localizer::Format("#autoLOC_6100016", {Localizer::Format(pv.displaylandedAt)});
        case Vessel::Situations::SPLASHED: return L("#autoLOC_145804");
        case Vessel::Situations::PRELAUNCH: return L("#autoLOC_145800");
        default: return L("#autoLOC_145808");
        case Vessel::Situations::DOCKED: return L("#autoLOC_145807");
        case Vessel::Situations::ESCAPING: return L("#autoLOC_145806");
        case Vessel::Situations::ORBITING: return L("#autoLOC_145803");
    }
}

// ProtoVessel.cs:1163
void ProtoVessel::Clean(const std::string& reason) {
    Game* game = HighLogic::CurrentGame();
    if (game == nullptr) return;
    // the C# object lives on while referenced; the list below may hold the last shared owner of this snapshot
    const std::shared_ptr<ProtoVessel> keepAlive = weak_from_this().lock();
    if (vesselRef != nullptr && vesselRef->loaded) {
        if (vesselRef != FlightGlobals::ActiveVessel) {
            vesselRef->Unload();
            vesselRef->protoVessel->Clean(reason);
        }
        return;
    }
    if (!orbitSnapShot) throw NullReferenceException("ProtoVessel.orbitSnapShot is null");
    const std::vector<CelestialBody*>& bodies = FlightGlobals::Bodies();
    CelestialBody* celestialBody = bodies.at(static_cast<size_t>(orbitSnapShot->ReferenceBodyIndex));  // FlightGlobals.Bodies[index]
    bool flag = false;
    float funds = 0.0f;
    float science = 0.0f;
    float reputation = 0.0f;
    if (celestialBody->isHomeWorld) {
        funds = CurrentFunds();
        science = CurrentScience();
        reputation = CurrentReputation();
        GE::onVesselRecovered.Fire(this, true);
        flag = true;
        funds = Mathf::Round(CurrentFunds() - funds);
        science = Mathf::Round(CurrentScience() - science);
        reputation = Mathf::Round(CurrentReputation() - reputation);
    }
    flag = flag && (game->Mode == Game::Modes::CAREER || game->Mode == Game::Modes::SCIENCE_SANDBOX);
    if (game->flightState == nullptr) throw NullReferenceException("Game.flightState is null");
    auto& list = game->flightState->protoVessels;
    const auto it = std::find_if(list.begin(), list.end(), [this](const std::shared_ptr<ProtoVessel>& p) { return p.get() == this; });
    if (it != list.end()) list.erase(it);  // protoVessels.Remove(this)
    if (vesselRef != nullptr) Vessel::DestroyImmediate(vesselRef);  // UnityEngine.Object.DestroyImmediate(vesselRef.gameObject)
    std::string text = "Vessel " + (vesselName.empty() ? std::string("was ") : (GetDisplayName() + " was "));
    if (flag) {
        const std::vector<std::string> currencies = FormattedCurrenciesPlain(funds, science, reputation);
        const std::string text2 = Contracts::Lite::StringUtilities::ThisThisAndThat(currencies);
        text += !currencies.empty() ? ("recovered for a refund of " + text2) : "removed from the game";
    } else {
        text += "removed from the game";
    }
    text += reason.empty() ? "." : (": " + reason + ".");
    log::Warning(text);
}

// ProtoVessel.cs:1215
float ProtoVessel::CurrentFunds() const { return Funding::Instance == nullptr ? 0.0f : static_cast<float>(Funding::Instance->Funds()); }

// ProtoVessel.cs:1224
float ProtoVessel::CurrentScience() const { return ResearchAndDevelopment::Instance == nullptr ? 0.0f : ResearchAndDevelopment::Instance->Science(); }

// ProtoVessel.cs:1233
float ProtoVessel::CurrentReputation() const { return Reputation::Instance == nullptr ? 0.0f : Reputation::Instance->reputation(); }

// ProtoVessel.cs:1242 (Localizer._CacheLocalStrings; see GetSituationString)
void ProtoVessel::CacheLocalStrings() {}

}  // namespace ksp
