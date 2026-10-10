// WP-36: Funding.cs and Reputation.cs, the two currency ScenarioModules (funds as a double, reputation as a float in
// [-RepRange, RepRange] adjusted through the GameVariables reputationAddition / reputationSubtraction curves).
//
// Both create a CurrencyModifierQuery on every transaction: Funding.AddFunds adds `value` to the balance first, then lets the
// strategy effects add their delta through OnCurrencyModifierQuery / OnCurrencyModified (Funding.OnCurrenciesModified applies it).
// ResearchAndDevelopment.AddScience is WP-35's; the seam is CareerEnvironment::rndAddScience.
#pragma once

#include <memory>

#include "ksp/career/career_environment.hpp"
#include "ksp/career/currency.hpp"
#include "ksp/save/game.hpp"

namespace ksp {

class EventReport;
class ShipConstruct;
class ProtoCrewMember;
namespace KSP::UI::Screens {
class MissionRecoveryDialog;
}

// Funding.cs:15 ([KSPScenario(1120, FLIGHT, TRACKSTATION, SPACECENTER, EDITOR)])
class Funding : public ScenarioModule {
public:
    Funding();
    ~Funding() override;
    static inline Funding* Instance = nullptr;            // :17
    double Funds() const { return funds_; }               // :21
    void AddFunds(double value, TransactionReasons reason);  // :23
    void SetFunds(double value, TransactionReasons reason);  // :36
    void OnAwake() override;                              // :46
    void OnDestroy();                                     // :72 (the MonoBehaviour destroy: the destructor calls it)
    void OnLoad(ConfigNode& node) override;              // :186
    void OnSave(ConfigNode& node) override;              // :194
    static bool CanAfford(float cost);                   // :199
    static void Register();                              // ScenarioModuleFactory registration
    // the handlers (private in C#; public here so a test can drive them without the whole event chain)
    void OnCurrenciesModified(CurrencyModifierQuery* query);                                                // :86
    void onVesselRollout(ShipConstruct* ship);                                                              // :96
    void onVesselRecoveryProcessing(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrDialog, float recoveryScore);  // :101
    void onPartPurchased(AvailablePart* aP);                                                                // :165
    void onUpgradePurchased(PartUpgradeHandler_Upgrade* upgrade);                                           // :173
    void onCrewHired(ProtoCrewMember* crew, int crewCount);                                                 // :181

private:
    bool eventsAdded_ = false;
    double funds_ = 0.0;
};

// Reputation.cs:11 ([KSPScenario(96, ...)])
class Reputation : public ScenarioModule {
public:
    Reputation();
    ~Reputation() override;
    static inline Reputation* Instance = nullptr;           // :13
    static inline float RepRange = 1000.0f;                 // :15
    static float CurrentRep();                              // :19
    static float UnitRep();                                 // :31
    float reputation() const { return rep_; }               // :43
    float AddReputation(float r, TransactionReasons reason);  // :45
    void SetReputation(float value, TransactionReasons reason);  // :66
    void addReputation_discrete(float reputation, TransactionReasons reason);  // :72
    void OnAwake() override;                                // :115
    void Start();                                           // :128 (the MonoBehaviour Start: subscribes the events)
    void OnDestroy();                                       // :135
    void OnLoad(ConfigNode& node) override;                // :180
    void OnSave(ConfigNode& node) override;                // :188
    static void Register();
    void OnCurrenciesModified(CurrencyModifierQuery* query);   // :146
    void OnCrewKilled(EventReport* evt);                       // :156
    void onvesselRecoveryProcessing(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrDialog, float recoveryScore);  // :164

private:
    float addReputation_granular(float value);   // :90
    float ModifyReputationDelta(float delta);    // :105
    bool eventsAdded_ = false;
    float rep_ = 0.0f;
};

}  // namespace ksp
