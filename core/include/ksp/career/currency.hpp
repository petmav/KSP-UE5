// WP-36: Currency.cs, TransactionReasons.cs, CurrencyModifierQuery.cs, ValueModifierQuery.cs (the currency modifier pipeline
// every career transaction runs through: Funding.AddFunds / Reputation.AddReputation / ResearchAndDevelopment.AddScience build a
// CurrencyModifierQuery, fire GameEvents.Modifiers.OnCurrencyModifierQuery so the active strategy effects add their deltas, then
// OnCurrencyModified so the scenario modules apply them).
//
// Float rule: all fields are C# `float`; every `+=` stores a float, every sum that is returned or passed on is rounded
// (ksp::fx / ksp::f32store, Mono semantics).
#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "ksp/dotnet/enum.hpp"

namespace ksp {

// Currency.cs:1 (the [Description]s are the localisation tags of the currency names)
enum class Currency : int { Funds, Science, Reputation };

// TransactionReasons.cs:2 ([Flags])
enum class TransactionReasons : int {
    None = 0,
    ContractAdvance = 2,
    ContractReward = 4,
    ContractPenalty = 8,
    Contracts = 0x4000E,
    VesselRollout = 0x10,
    VesselRecovery = 0x20,
    VesselLoss = 0x40,
    Vessels = 0x70,
    StrategyInput = 0x80,
    StrategyOutput = 0x100,
    StrategySetup = 0x200,
    Strategies = 0x380,
    ScienceTransmission = 0x400,
    StructureRepair = 0x800,
    StructureCollapse = 0x1000,
    StructureConstruction = 0x2000,
    Structures = 0x3800,
    RnDTechResearch = 0x4000,
    RnDPartPurchase = 0x8000,
    RnDs = 0xC000,
    Cheating = 0x10000,
    CrewRecruited = 0x20000,
    ContractDecline = 0x40000,
    Progression = 0x80000,
    Mission = 0x100000,
    Any = -1
};
constexpr TransactionReasons operator|(TransactionReasons a, TransactionReasons b) {
    return static_cast<TransactionReasons>(static_cast<int>(a) | static_cast<int>(b));
}
constexpr TransactionReasons operator&(TransactionReasons a, TransactionReasons b) {
    return static_cast<TransactionReasons>(static_cast<int>(a) & static_cast<int>(b));
}
constexpr TransactionReasons& operator|=(TransactionReasons& a, TransactionReasons b) { return a = a | b; }

}  // namespace ksp

template <>
struct ksp::dotnet::EnumTraits<ksp::Currency> {
    static constexpr bool flags = false;
    static constexpr EnumMember<ksp::Currency> members[] = {
        {ksp::Currency::Funds, "Funds", "#autoLOC_7001031"},
        {ksp::Currency::Science, "Science", "#autoLOC_7001032"},
        {ksp::Currency::Reputation, "Reputation", "#autoLOC_7001033"}};
};

template <>
struct ksp::dotnet::EnumTraits<ksp::TransactionReasons> {
    static constexpr bool flags = true;
    static constexpr EnumMember<ksp::TransactionReasons> members[] = {
        {ksp::TransactionReasons::None, "None"},
        {ksp::TransactionReasons::ContractAdvance, "ContractAdvance"},
        {ksp::TransactionReasons::ContractReward, "ContractReward"},
        {ksp::TransactionReasons::ContractPenalty, "ContractPenalty"},
        {ksp::TransactionReasons::Contracts, "Contracts"},
        {ksp::TransactionReasons::VesselRollout, "VesselRollout"},
        {ksp::TransactionReasons::VesselRecovery, "VesselRecovery"},
        {ksp::TransactionReasons::VesselLoss, "VesselLoss"},
        {ksp::TransactionReasons::Vessels, "Vessels"},
        {ksp::TransactionReasons::StrategyInput, "StrategyInput"},
        {ksp::TransactionReasons::StrategyOutput, "StrategyOutput"},
        {ksp::TransactionReasons::StrategySetup, "StrategySetup"},
        {ksp::TransactionReasons::Strategies, "Strategies"},
        {ksp::TransactionReasons::ScienceTransmission, "ScienceTransmission"},
        {ksp::TransactionReasons::StructureRepair, "StructureRepair"},
        {ksp::TransactionReasons::StructureCollapse, "StructureCollapse"},
        {ksp::TransactionReasons::StructureConstruction, "StructureConstruction"},
        {ksp::TransactionReasons::Structures, "Structures"},
        {ksp::TransactionReasons::RnDTechResearch, "RnDTechResearch"},
        {ksp::TransactionReasons::RnDPartPurchase, "RnDPartPurchase"},
        {ksp::TransactionReasons::RnDs, "RnDs"},
        {ksp::TransactionReasons::Cheating, "Cheating"},
        {ksp::TransactionReasons::CrewRecruited, "CrewRecruited"},
        {ksp::TransactionReasons::ContractDecline, "ContractDecline"},
        {ksp::TransactionReasons::Progression, "Progression"},
        {ksp::TransactionReasons::Mission, "Mission"},
        {ksp::TransactionReasons::Any, "Any"}};
};

namespace ksp {

// ValueModifierQuery.cs:1
class ValueModifierQuery {
public:
    enum class TextStyling { None, OnGUI, EzGUIRichText, OnGUI_LessIsGood, EzGUIRichText_LessIsGood };
    ValueModifierQuery(std::string valueId, float v0);          // :20
    const std::string& ValueId() const { return valueId_; }     // :18
    float GetInput() const { return input_; }                   // :26
    void AddDelta(float val);                                   // :30
    float GetEffectDelta() const { return delta_; }             // :35
    std::string GetEffectDeltaText(const std::string& format, TextStyling textStyle = TextStyling::None) const;       // :40
    std::string GetEffectPercentageText(const std::string& format, TextStyling textStyle = TextStyling::None) const;  // :64
    // :88: runs GameEvents.Modifiers.onValueModifierQuery and returns the query (the caller owns it)
    static ValueModifierQuery RunQuery(const std::string& valueId, float input);

private:
    std::string valueId_;
    float input_;
    float delta_;
};

// CurrencyModifierQuery.cs:4
class CurrencyModifierQuery {
public:
    using TextStyling = ValueModifierQuery::TextStyling;  // CurrencyModifierQuery.TextStyling has the same members
    TransactionReasons reason;                             // :15
    CurrencyModifierQuery(TransactionReasons reason, float f0, float s0, float r0);  // :29
    float GetInput(Currency c) const;                      // :40
    void AddDelta(Currency c, float val);                  // :55
    float GetEffectDelta(Currency c) const;                // :71
    float GetTotal(Currency c) const;                      // :86
    // :91 (the sprite/colour markup is part of the text; KSPUtil.LocalizeNumber is the invariant-culture formatter)
    std::string GetCostLine(bool displayInverted = true, bool useCurrencyColors = false, bool useInsufficientCurrencyColors = true,
                            bool includePercentage = false, const std::string& seperator = ", ") const;
    // :144 (onInsufficientCurrency is called with the first currency that is short)
    bool CanAfford(const std::function<void(Currency)>& onInsufficientCurrency = nullptr) const;
    bool CanAfford(Currency c) const;                       // :167
    std::string GetEffectDeltaText(Currency c, const std::string& format, TextStyling textStyle = TextStyling::None) const;       // :195
    std::string GetEffectPercentageText(Currency c, const std::string& format, TextStyling textStyle = TextStyling::None) const;  // :231
    // :275 (the caller owns the returned query)
    static CurrencyModifierQuery RunQuery(TransactionReasons reason, float f0, float s0, float r0);

private:
    float inputFunds_, inputScience_, inputRep_;
    float deltaFunds_, deltaScience_, deltaRep_;
};

}  // namespace ksp
