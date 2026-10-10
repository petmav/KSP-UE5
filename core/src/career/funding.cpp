// WP-36: Funding.cs and Reputation.cs.
#include "ksp/career/funding.hpp"

#include "ksp/career/game_variables.hpp"
#include "ksp/career/scenario_misc.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel_types.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
Game* CurrentGame() { return SaveEnvironment::Current().currentGame; }
// HighLogic.CurrentGame.Parameters.CustomParams<GameParameters.AdvancedParams>().AllowNegativeCurrency
bool AllowNegativeCurrency() { return CurrentGame()->Parameters->CustomParams<GameParameters::AdvancedParams>().AllowNegativeCurrency; }
}  // namespace

// ---- Funding ------------------------------------------------------------------------------------------------------------

Funding::Funding() : ScenarioModule("Funding") {}
Funding::~Funding() { OnDestroy(); }

// Funding.cs:23
void Funding::AddFunds(double value, TransactionReasons reason) {
    funds_ += value;
    CurrencyModifierQuery data(reason, static_cast<float>(value), 0.0f, 0.0f);
    GE::Modifiers.OnCurrencyModifierQuery.Fire(&data);
    GE::Modifiers.OnCurrencyModified.Fire(&data);
    if (!AllowNegativeCurrency()) funds_ = dotnet::Math::Max(0.0, funds_);
    GE::OnFundsChanged.Fire(funds_, reason);
}

// Funding.cs:36
void Funding::SetFunds(double value, TransactionReasons reason) {
    funds_ = value;
    if (!AllowNegativeCurrency()) funds_ = dotnet::Math::Max(0.0, funds_);
    GE::OnFundsChanged.Fire(funds_, reason);
}

// Funding.cs:46
void Funding::OnAwake() {
    if (Instance != nullptr) log::Error("[Funding Module]: Instance already exists!");
    Instance = this;
    if (CurrentGame() != nullptr) {
        Game* g = CurrentGame();
        if (g->Mode != Game::Modes::MISSION)
            funds_ = g->Parameters->Career.StartingFunds;
        else
            funds_ = g->Parameters->CustomParams<Expansions::Missions::MissionParamsGeneral>().startingFunds;
    }
    GE::OnVesselRollout.Add(this, &Funding::onVesselRollout);
    GE::onVesselRecoveryProcessing.Add(this, &Funding::onVesselRecoveryProcessing);
    GE::OnPartPurchased.Add(this, &Funding::onPartPurchased);
    GE::OnPartUpgradePurchased.Add(this, &Funding::onUpgradePurchased);
    GE::OnCrewmemberHired.Add(this, &Funding::onCrewHired);
    GE::Modifiers.OnCurrencyModified.Add(this, &Funding::OnCurrenciesModified);
    eventsAdded_ = true;
}

// Funding.cs:72
void Funding::OnDestroy() {
    if (eventsAdded_) {
        GE::OnVesselRollout.Remove(this, &Funding::onVesselRollout);
        GE::onVesselRecoveryProcessing.Remove(this, &Funding::onVesselRecoveryProcessing);
        GE::OnPartPurchased.Remove(this, &Funding::onPartPurchased);
        GE::OnPartUpgradePurchased.Remove(this, &Funding::onUpgradePurchased);
        GE::OnCrewmemberHired.Remove(this, &Funding::onCrewHired);
        GE::Modifiers.OnCurrencyModified.Remove(this, &Funding::OnCurrenciesModified);
        eventsAdded_ = false;
    }
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

// Funding.cs:86
void Funding::OnCurrenciesModified(CurrencyModifierQuery* query) {
    const float effectDelta = query->GetEffectDelta(Currency::Funds);
    funds_ += static_cast<double>(effectDelta);
    if (effectDelta != 0.0f) GE::OnFundsChanged.Fire(funds_, query->reason);
}

// Funding.cs:96
void Funding::onVesselRollout(ShipConstruct* ship) {
    float dry = 0.0f, fuel = 0.0f;
    const float cost = ship->GetShipCosts(dry, fuel, ShipConstruction::ShipManifest);
    AddFunds(static_cast<double>(-cost), TransactionReasons::VesselRollout);
}

// Funding.cs:101
void Funding::onVesselRecoveryProcessing(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrDialog, float recoveryScore) {
    if (pv == nullptr) return;
    const bool flag = mrDialog != nullptr;
    double num = 0.0;
    std::vector<ProtoPartSnapshot*> parts = pv->GetAllProtoPartsIncludingCargo();
    for (size_t i = 0; i < parts.size(); ++i) {
        ProtoPartSnapshot& snap = *parts[i];
        const AvailablePart* availablePart = nullptr;
        if (snap.partInfo == nullptr) {
            if (snap.partName && !snap.partName->empty()) {
                SaveEnvironment& env = SaveEnvironment::Current();
                if (env.getPartInfoByName) availablePart = env.getPartInfoByName(*snap.partName);
            }
        } else {
            availablePart = snap.partInfo;
        }
        if (availablePart != nullptr) {
            float dryCost = 0.0f, fuelCost = 0.0f;
            ShipConstruction::GetPartCosts(snap, false, availablePart, dryCost, fuelCost);
            dryCost = f32store(fx(dryCost) * fx(recoveryScore));
            fuelCost = f32store(fx(fuelCost) * fx(recoveryScore));
            num += fx(dryCost) + fx(fuelCost);  // (double)(dryCost + fuelCost): the sum is not rounded to float (Mono)
            if (!flag) continue;
            if (availablePart->name != "kerbalEVA")
                mrDialog->AddPartWidget({availablePart->name, dryCost, fuelCost});
            for (size_t j = 0; j < snap.resources.size(); ++j) {
                ProtoPartResourceSnapshot& res = *snap.resources[j];
                const PartResourceDefinition* definition = PartResourceLibrary::Instance()->GetDefinition(res.resourceName.value_or(std::string()));
                if (definition != nullptr)
                    mrDialog->AddResourceWidget({res.resourceName.value_or(std::string()), static_cast<float>(res.amount),
                                                 f32store(fx(definition->unitCost()) * fx(recoveryScore))});
                else
                    log::Error("[ShipTemplate]: No Resource definition found for " + res.resourceName.value_or(std::string()));
            }
        } else if (flag) {
            log::Info("[Funding]: Cannot recover " + snap.partName.value_or(std::string()) +
                      ". Part has no entry in PartLoader catalog. That is only OK if the part is an EVA.");
        }
    }
    if (flag) mrDialog->fundsEarned = num;
    AddFunds(num, TransactionReasons::VesselRecovery);
}

// Funding.cs:165
void Funding::onPartPurchased(AvailablePart* aP) {
    if (!CurrentGame()->Parameters->Difficulty.BypassEntryPurchaseAfterResearch && aP->costsFunds)
        AddFunds(static_cast<double>(-static_cast<double>(aP->entryCost)), TransactionReasons::RnDPartPurchase);
}

// Funding.cs:173
void Funding::onUpgradePurchased(PartUpgradeHandler_Upgrade* upgrade) {
    CareerEnvironment& env = CareerEnvironment::Current();
    const float entryCost = env.upgradeEntryCost ? env.upgradeEntryCost(upgrade) : 0.0f;
    if (!CurrentGame()->Parameters->Difficulty.BypassEntryPurchaseAfterResearch && entryCost > 0.0f)
        AddFunds(static_cast<double>(-entryCost), TransactionReasons::RnDPartPurchase);
}

// Funding.cs:181
void Funding::onCrewHired(ProtoCrewMember* crew, int crewCount) {
    (void)crew;
    AddFunds(static_cast<double>(-GameVariables::Instance->GetRecruitHireCost(crewCount)), TransactionReasons::CrewRecruited);
}

// Funding.cs:186
void Funding::OnLoad(ConfigNode& node) {
    if (node.HasValue("funds")) funds_ = dotnet::ParseDouble(*node.GetValue("funds"));
}

// Funding.cs:194
void Funding::OnSave(ConfigNode& node) { node.AddValue("funds", funds_); }

// Funding.cs:199
bool Funding::CanAfford(float cost) {
    if (Instance == nullptr) return true;
    return static_cast<double>(cost) <= Instance->Funds();
}

void Funding::Register() {
    ScenarioModuleFactory::Register("Funding", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new Funding()); });
}

// ---- Reputation ----------------------------------------------------------------------------------------------------------

Reputation::Reputation() : ScenarioModule("Reputation") {}
Reputation::~Reputation() { OnDestroy(); }

// Reputation.cs:19
float Reputation::CurrentRep() { return Instance != nullptr ? Instance->rep_ : 0.0f; }

// Reputation.cs:31
float Reputation::UnitRep() { return Instance != nullptr ? f32store(fx(Instance->rep_) / fx(RepRange)) : 0.0f; }

// Reputation.cs:45
float Reputation::AddReputation(float r, TransactionReasons reason) {
    const float num = addReputation_granular(r);
    CurrencyModifierQuery data(reason, 0.0f, 0.0f, r);
    GE::Modifiers.OnCurrencyModifierQuery.Fire(&data);
    GE::Modifiers.OnCurrencyModified.Fire(&data);
    if (reason == TransactionReasons::None)
        log::Info("Added " + dotnet::FormatSingleG(num) + " (" + dotnet::FormatSingleG(r) + ") reputation. Total Rep: " + dotnet::FormatSingleG(reputation()));
    else
        log::Info("Added " + dotnet::FormatSingleG(num) + " (" + dotnet::FormatSingleG(r) + ") reputation: '" + dotnet::EnumToString(reason) + "'.");
    if (num != 0.0f) GE::OnReputationChanged.Fire(rep_, reason);
    return num;
}

// Reputation.cs:66
void Reputation::SetReputation(float value, TransactionReasons reason) {
    rep_ = Mathf::Clamp(value, -RepRange, RepRange);
    GE::OnReputationChanged.Fire(rep_, reason);
}

// Reputation.cs:72
void Reputation::addReputation_discrete(float reputation_, TransactionReasons reason) {
    const float num = ModifyReputationDelta(reputation_);
    if (reason == TransactionReasons::None)
        log::Info("Adding " + dotnet::FormatSingleG(num) + " (" + dotnet::FormatSingleG(reputation_) + ") reputation.");
    else
        log::Info("Adding " + dotnet::FormatSingleG(num) + " (" + dotnet::FormatSingleG(reputation_) + ") reputation: '" +
                  dotnet::EnumToString(reason) + "'.");
    if (num != 0.0f) {
        rep_ = f32store(fx(rep_) + fx(num));
        GE::OnReputationChanged.Fire(rep_, reason);
    }
}

// Reputation.cs:90
float Reputation::addReputation_granular(float value) {
    const int num = dotnet::ConvToInt32(static_cast<double>(Mathf::Abs(value)));
    const float delta = f32store(1.0 * fx(Mathf::Sign(value)));
    float num2 = 0.0f;
    float num3 = 0.0f;
    for (int i = 0; i <= num; ++i) {
        num3 = (i == num) ? ModifyReputationDelta(f32store(fx(value) - fx(num2))) : ModifyReputationDelta(delta);
        rep_ = f32store(fx(rep_) + fx(num3));
        num2 = f32store(fx(num2) + fx(num3));
    }
    return num2;
}

// Reputation.cs:105
float Reputation::ModifyReputationDelta(float delta) {
    const float time = f32store(fx(rep_) / fx(RepRange));
    if (delta < 0.0f) return f32store(fx(delta) * fx(GameVariables::Instance->reputationSubtraction.Evaluate(time)));
    return f32store(fx(delta) * fx(GameVariables::Instance->reputationAddition.Evaluate(time)));
}

// Reputation.cs:115
void Reputation::OnAwake() {
    if (Instance != nullptr) log::Error("[Reputation Module]: Instance already exists!");
    if (CurrentGame() != nullptr) rep_ = CurrentGame()->Parameters->Career.StartingReputation;
    Instance = this;
}

// Reputation.cs:128
void Reputation::Start() {
    GE::onCrewKilled.Add(this, &Reputation::OnCrewKilled);
    GE::onVesselRecoveryProcessing.Add(this, &Reputation::onvesselRecoveryProcessing);
    GE::Modifiers.OnCurrencyModified.Add(this, &Reputation::OnCurrenciesModified);
    eventsAdded_ = true;
}

// Reputation.cs:135
void Reputation::OnDestroy() {
    if (eventsAdded_) {
        GE::onCrewKilled.Remove(this, &Reputation::OnCrewKilled);
        GE::onVesselRecoveryProcessing.Remove(this, &Reputation::onvesselRecoveryProcessing);
        GE::Modifiers.OnCurrencyModified.Remove(this, &Reputation::OnCurrenciesModified);
        eventsAdded_ = false;
    }
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

// Reputation.cs:146
void Reputation::OnCurrenciesModified(CurrencyModifierQuery* query) {
    const float effectDelta = query->GetEffectDelta(Currency::Reputation);
    addReputation_granular(effectDelta);
    if (effectDelta != 0.0f) GE::OnReputationChanged.Fire(rep_, query->reason);
}

// Reputation.cs:156
void Reputation::OnCrewKilled(EventReport* evt) {
    if (evt->eventType == FlightEvents::CREW_KILLED)
        AddReputation(f32store(fx(GameVariables::Instance->reputationKerbalDeath) * fx(CurrentGame()->Parameters->Career.RepLossMultiplier)),
                      TransactionReasons::VesselLoss);
}

// Reputation.cs:164
void Reputation::onvesselRecoveryProcessing(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrDialog, float recoveryScore) {
    (void)recoveryScore;
    if (pv != nullptr) {
        float num = 0.0f;
        for (int num2 = static_cast<int>(pv->GetVesselCrew().size()); num2 > 0; --num2)
            num = f32store(fx(num) + fx(AddReputation(f32store(fx(GameVariables::Instance->reputationKerbalRecovery) *
                                                                    fx(CurrentGame()->Parameters->Career.RepGainMultiplier)),
                                                         TransactionReasons::VesselRecovery)));
        if (mrDialog != nullptr) mrDialog->reputationEarned = num;
    }
}

// Reputation.cs:180
void Reputation::OnLoad(ConfigNode& node) {
    if (node.HasValue("rep")) rep_ = dotnet::ParseSingle(*node.GetValue("rep"));
}

// Reputation.cs:188
void Reputation::OnSave(ConfigNode& node) { node.AddValue("rep", rep_); }

void Reputation::Register() {
    ScenarioModuleFactory::Register("Reputation", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new Reputation()); });
}

}  // namespace ksp
