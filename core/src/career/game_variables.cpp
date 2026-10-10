// WP-36: GameVariables.cs. Managed float code: every expression is evaluated in double and rounded where C# stores, passes or
// returns it (ksp::fx / f32store).
#include "ksp/career/game_variables.hpp"

#include <limits>

#include "ksp/career/upgradeables.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using Localizer = KSP::Localization::Localizer;

namespace {
GameParameters& Params() { return *SaveEnvironment::Current().currentGame->Parameters; }
constexpr int kIntMax = std::numeric_limits<int>::max();
constexpr float kFloatMax = std::numeric_limits<float>::max();
}  // namespace

// GameVariables.cs:80 Awake
GameVariables::GameVariables() {
    if (Instance != nullptr) {
        log::Error("[GameVariables]: Instance already exists!");
    } else {
        Instance = this;
    }
}

// GameVariables.cs:92 OnDestroy
GameVariables::~GameVariables() {
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

// GameVariables.cs:100
float GameVariables::GetRecoveredPartValue(float pValue) { return f32store(fx(pValue) * fx(partRecoveryValueFactor)); }

// GameVariables.cs:105
float GameVariables::GetRecoveredResourceValue(float rscValue) { return f32store(fx(rscValue) * fx(resourceRecoveryValueFactor)); }

// GameVariables.cs:110
float GameVariables::GetContractPrestigeFactor(ContractPrestige prestige) {
    switch (prestige) {
        case ContractPrestige::Significant: return contractPrestigeSignificant;
        case ContractPrestige::Trivial: return contractPrestigeTrivial;
        default: return contractPrestigeExceptional;
    }
}

// GameVariables.cs:123
float GameVariables::GetContractFundsAdvanceFactor(ContractPrestige prestige) {
    return f32store(fx(GetContractPrestigeFactor(prestige)) * fx(contractFundsAdvanceFactor) * fx(Params().Career.FundsGainMultiplier));
}

// GameVariables.cs:128
float GameVariables::GetContractFundsCompletionFactor(ContractPrestige prestige) {
    return f32store(fx(GetContractPrestigeFactor(prestige)) * fx(contractFundsCompletionFactor) * fx(Params().Career.FundsGainMultiplier));
}

// GameVariables.cs:133
float GameVariables::GetContractFundsFailureFactor(ContractPrestige prestige) {
    return f32store(fx(GetContractPrestigeFactor(prestige)) * fx(contractFundsFailureFactor) * fx(Params().Career.FundsLossMultiplier));
}

// GameVariables.cs:138
float GameVariables::GetContractReputationCompletionFactor(ContractPrestige prestige) {
    return f32store(fx(GetContractPrestigeFactor(prestige)) * fx(contractReputationCompletionFactor) * fx(Params().Career.RepGainMultiplier));
}

// GameVariables.cs:143
float GameVariables::GetContractReputationFailureFactor(ContractPrestige prestige) {
    return f32store(fx(GetContractPrestigeFactor(prestige)) * fx(contractReputationFailureFactor) * fx(Params().Career.RepLossMultiplier));
}

// GameVariables.cs:148
float GameVariables::GetContractScienceCompletionFactor(ContractPrestige prestige) {
    return f32store(fx(GetContractPrestigeFactor(prestige)) * fx(contractScienceCompletionFactor) * fx(Params().Career.ScienceGainMultiplier));
}

// GameVariables.cs:153
float GameVariables::GetContractDestinationWeight(CelestialBody* body) {
    return f32store(fx(body->scienceValues.RecoveryValue) * fx(contractDestinationWeight));
}

// GameVariables.cs:158
float GameVariables::ScoreSituation(VesselSituations sit, CelestialBody* where) {
    if (where == Planetarium::fetch->Home) {
        switch (sit) {
            case VesselSituations::SUB_ORBITAL: return 0.8f;
            case VesselSituations::FLYING: return 0.5f;
            case VesselSituations::SPLASHED: return 0.3f;
            case VesselSituations::LANDED:
            case VesselSituations::PRELAUNCH: return 0.1f;
            case VesselSituations::ESCAPING: return 1.1f;
            case VesselSituations::ORBITING: return 1.0f;
            default: return 0.1f;
        }
    }
    switch (sit) {
        case VesselSituations::SUB_ORBITAL: return 1.0f;
        case VesselSituations::FLYING: return 1.2f;
        case VesselSituations::LANDED: return 1.5f;
        case VesselSituations::SPLASHED: return 1.5f;
        case VesselSituations::ESCAPING: return 0.6f;
        case VesselSituations::ORBITING: return 0.8f;
        default: return 0.1f;
    }
}

// GameVariables.cs:201
float GameVariables::ScoreFlightEnvelope(float altitude, float altEnvelope, float speed, float speedEnvelope) {
    float num = 0.0f;
    float num2 = 0.0f;
    if (altEnvelope != 0.0f) {
        const float num3 = f32store(fx(Mathf::Floor(f32store(fx(altitude) / fx(10000.0f) * fx(1000.0f)))) / fx(1000.0f));
        const float num4 = Mathf::InverseLerp(0.0f, altitude, altEnvelope);
        num = Mathf::Max(f32store(fx(num3) - fx(num4)), 0.0f);
    }
    if (speedEnvelope != 0.0f) {
        const float num5 = f32store(fx(Mathf::Floor(f32store(fx(speed) / fx(1000.0f) * fx(100.0f)))) / fx(100.0f));
        const float num6 = Mathf::InverseLerp(0.0f, speed, speedEnvelope);
        num2 = Mathf::Max(f32store(fx(num5) - fx(num6)), 0.0f);
    }
    return f32store(fx(num) + fx(num2));
}

// GameVariables.cs:220
float GameVariables::GetMentalityFundsFactor(float mentalityFactor, ContractPrestige prestige) {
    switch (prestige) {
        case ContractPrestige::Significant: return Mathf::Lerp(1.0f, mentalityFundsSignificant, mentalityFactor);
        case ContractPrestige::Trivial: return Mathf::Lerp(1.0f, mentalityFundsTrivial, mentalityFactor);
        default: return Mathf::Lerp(1.0f, mentalityFundsExceptional, mentalityFactor);
    }
}

// GameVariables.cs:233
float GameVariables::GetMentalityReputationFactor(float mentalityFactor, ContractPrestige prestige) {
    switch (prestige) {
        case ContractPrestige::Significant: return Mathf::Lerp(1.0f, mentalityReputationSignificant, mentalityFactor);
        case ContractPrestige::Trivial: return Mathf::Lerp(1.0f, mentalityReputationTrivial, mentalityFactor);
        default: return Mathf::Lerp(1.0f, mentalityReputationExceptional, mentalityFactor);
    }
}

// GameVariables.cs:246
float GameVariables::GetMentalityScienceFactor(float mentalityFactor, ContractPrestige prestige) {
    switch (prestige) {
        case ContractPrestige::Significant: return Mathf::Lerp(1.0f, mentalityScienceSignificant, mentalityFactor);
        case ContractPrestige::Trivial: return Mathf::Lerp(1.0f, mentalityScienceTrivial, mentalityFactor);
        default: return Mathf::Lerp(1.0f, mentalityScienceExceptional, mentalityFactor);
    }
}

// GameVariables.cs:259
float GameVariables::GetMentalityExpiryFactor(float mentalityFactor, ContractPrestige prestige) {
    switch (prestige) {
        case ContractPrestige::Significant: return Mathf::Lerp(1.0f, mentalityExpirySignificant, mentalityFactor);
        case ContractPrestige::Trivial: return Mathf::Lerp(1.0f, mentalityExpiryTrivial, mentalityFactor);
        default: return Mathf::Lerp(1.0f, mentalityExpiryExceptional, mentalityFactor);
    }
}

// GameVariables.cs:272
float GameVariables::GetMentalityDeadlineFactor(float mentalityFactor, ContractPrestige prestige) {
    switch (prestige) {
        case ContractPrestige::Significant: return Mathf::Lerp(1.0f, mentalityDeadlineSignificant, mentalityFactor);
        case ContractPrestige::Trivial: return Mathf::Lerp(1.0f, mentalityDeadlineTrivial, mentalityFactor);
        default: return Mathf::Lerp(1.0f, mentalityDeadlineExceptional, mentalityFactor);
    }
}

// GameVariables.cs:285
int GameVariables::GetPartCountLimit(float editorNormLevel, bool isVAB) {
    (void)isVAB;
    if (editorNormLevel < 0.2f) return 30;
    if (editorNormLevel < 0.4f) return 110;
    if (editorNormLevel < 0.6f) return 255;
    if (editorNormLevel < 0.8f) return 450;
    return kIntMax;
}

// GameVariables.cs:306
bool GameVariables::UnlockedFuelTransfer(float editorNormLevel) { return editorNormLevel > 0.2f; }

// GameVariables.cs:311
bool GameVariables::UnlockedActionGroupsStock(float editorNormLevel, bool isVAB) {
    (void)isVAB;
    Game* g = SaveEnvironment::Current().currentGame;
    if (g != nullptr && g->Parameters->CustomParams<GameParameters::AdvancedParams>().ActionGroupsAlways) return true;
    return editorNormLevel > 0.4f;
}

// GameVariables.cs:320
bool GameVariables::UnlockedActionGroupsCustom(float editorNormLevel, bool isVAB) {
    (void)isVAB;
    Game* g = SaveEnvironment::Current().currentGame;
    if (g != nullptr && g->Parameters->CustomParams<GameParameters::AdvancedParams>().ActionGroupsAlways) return true;
    return editorNormLevel > 0.6f;
}

// GameVariables.cs:329
float GameVariables::GetCraftMassLimit(float editorNormLevel, bool isPad) {
    (void)isPad;
    if (editorNormLevel < 0.2f) return 18.0f;
    if (editorNormLevel < 0.4f) return 32.0f;
    if (editorNormLevel < 0.6f) return 140.0f;
    if (editorNormLevel < 0.8f) return 300.0f;
    return kFloatMax;
}

// GameVariables.cs:350
Vector3f GameVariables::GetCraftSizeLimit(float editorNormLevel, bool isPad) {
    (void)isPad;
    if (editorNormLevel < 0.2f) return Vector3f(15.0f, 20.0f, 15.0f);
    if (editorNormLevel < 0.4f) return Vector3f(20.0f, 28.0f, 20.0f);
    if (editorNormLevel < 0.6f) return Vector3f(28.0f, 36.0f, 28.0f);
    if (editorNormLevel < 0.8f) return Vector3f(36.0f, 50.0f, 36.0f);
    return Vector3f(kFloatMax, kFloatMax, kFloatMax);
}

// GameVariables.cs:371
int GameVariables::GetActiveStrategyLimit(float adminNormLevel) {
    if (adminNormLevel < 0.25f) return 1;
    if (adminNormLevel < 0.5f) return 2;
    if (adminNormLevel < 0.75f) return 3;
    return 5;
}

// GameVariables.cs:388
float GameVariables::GetStrategyCommitRange(float adminNormLevel) {
    if (adminNormLevel < 0.3f) return 0.25f;
    if (adminNormLevel < 0.6f) return 0.6f;
    return 1.0f;
}

// GameVariables.cs:401
float GameVariables::GetStrategyLevelLimit(float adminNormLevel) { return adminNormLevel; }

// GameVariables.cs:406
int GameVariables::GetActiveContractsLimit(float mCtrlNormLevel) {
    if (mCtrlNormLevel < 0.25f) return 2;
    if (mCtrlNormLevel < 0.5f) return 4;
    if (mCtrlNormLevel < 0.75f) return 7;
    return kIntMax;
}

// GameVariables.cs:423
float GameVariables::GetContractLevelLimit(float mCtrlNormLevel) { return mCtrlNormLevel; }

// GameVariables.cs:428
bool GameVariables::UnlockedFlightPlanning(float mCtrlNormLevel) { return mCtrlNormLevel > 0.4f; }

// GameVariables.cs:433
int GameVariables::GetActiveCrewLimit(float astroComplexNormLevel) {
    if (astroComplexNormLevel < 0.25f) return 5;
    if (astroComplexNormLevel < 0.5f) return 8;
    if (astroComplexNormLevel < 0.75f) return 12;
    return kIntMax;
}

// GameVariables.cs:450
float GameVariables::GetCrewLevelLimit(float astroComplexNormLevel) {
    if (astroComplexNormLevel < 0.33f) return 1.0f;
    if (astroComplexNormLevel < 0.66f) return 3.0f;
    return kFloatMax;
}

// GameVariables.cs:463
float GameVariables::GetRecruitHireCost(int currentActive, float baseCost, float flatRate, float rateModifier) {
    (void)flatRate;
    (void)rateModifier;
    const float lg = Mathf::Log(f32store(0.25 * static_cast<double>(static_cast<float>(currentActive)) + fx(1.12f)));
    const float rounded = Mathf::Round(f32store(fx(baseCost) + fx(80000.0f) * (fx(lg) / fx(500.0f)) * fx(500.0f)));
    return f32store(fx(rounded) * fx(Params().Career.FundsLossMultiplier));
}

// GameVariables.cs:468
float GameVariables::GetRecruitHireCost(int currentActive) { return GetRecruitHireCost(currentActive, 2000.0f, 1.25f, 0.015f); }

// GameVariables.cs:473
bool GameVariables::UnlockedEVA(float astroComplexNormLevel) { return astroComplexNormLevel > 0.2f; }

// GameVariables.cs:478
bool GameVariables::UnlockedEVAFlags(float astroComplexNormLevel) { return astroComplexNormLevel >= 0.4f; }

// GameVariables.cs:483
bool GameVariables::UnlockedEVAClamber(float astroComplexNormLevel) { return astroComplexNormLevel > 0.6f; }

// GameVariables.cs:488
bool GameVariables::EVAIsPossible(bool evaUnlocked, Vessel* v) {
    if (!evaUnlocked && (!(v->mainBody() == Planetarium::fetch->Home) || !v->LandedOrSplashed())) return false;
    return TimeWarp::CurrentRate() <= 1.0f;
}

// GameVariables.cs:497 (the Kerbal / InPart branch reads the live Kerbal: CareerEnvironment::kerbalRefLockReason returns its
// localisation tag, "" when KerbalRef is null or neither dead nor NoAutoEVA)
std::string GameVariables::GetEVALockedReason(Vessel* v, ProtoCrewMember* crew) {
    if (crew->type() == ProtoCrewMember::KerbalType::Tourist) return Localizer::Format("#autoLOC_294604");
    if (crew->inactive()) return Localizer::Format("#autoLOC_294609");
    CareerEnvironment& env = CareerEnvironment::Current();
    if (env.kerbalRefLockReason) {
        const std::string tag = env.kerbalRefLockReason(crew);
        if (!tag.empty()) return Localizer::Format(tag);
    }
    if (!SaveEnvironment::Current().currentGame->Parameters->Flight.CanEVA) return Localizer::Format("#autoLOC_294628");
    if (!UnlockedEVA(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::AstronautComplex)))
        return Localizer::Format("#autoLOC_294633");
    if (crew->rosterStatus() != ProtoCrewMember::RosterStatus::Assigned) return Localizer::Format("#autoLOC_294639");
    if (v->packed) return Localizer::Format("#autoLOC_294644");
    return Localizer::Format("#autoLOC_294648");
}

// GameVariables.cs:537
DoubleCurve GameVariables::GetDSNRangeCurve() {
    DoubleCurve c;
    c.Add(0.0, 0.0, 0.0, 0.0);
    c.Add(1.0, 1.0, 0.0, 0.0);
    return c;
}

// GameVariables.cs:545
DoubleCurve GameVariables::GetDSNPowerCurve() {
    DoubleCurve c;
    c.Add(0.0, 1.0, 0.0, 0.0);
    c.Add(1.0, 0.10000000149011612, 0.0, 0.0);
    return c;
}

// GameVariables.cs:553
DoubleCurve GameVariables::GetDSNScienceCurve() {
    DoubleCurve c;
    c.Add(0.0, 0.0, 0.0, 0.0);
    c.Add(0.5, 0.10000000149011612, 0.0, 0.0);
    c.Add(1.0, 0.75, 0.0, 0.0);
    return c;
}

// GameVariables.cs:562
double GameVariables::GetDSNRange(float level) {
    double num = (static_cast<double>(level) > 0.500000001) ? 250000000000.0
                                                              : (!(static_cast<double>(level) > 1E-09) ? 2000000000.0 : 50000000000.0);
    Game* g = SaveEnvironment::Current().currentGame;
    return num * ((g != nullptr) ? static_cast<double>(g->Parameters->CustomParams<CommNet::CommNetParams>().DSNModifier) : 1.0);
}

// GameVariables.cs:569
GameVariables::OrbitDisplayMode GameVariables::GetOrbitDisplayMode(float tsNormLevel) {
    if (tsNormLevel < 0.2f) return OrbitDisplayMode::AllOrbits;
    return OrbitDisplayMode::PatchedConics;
}

// GameVariables.cs:578
int GameVariables::GetTrackedObjectLimit(float tsNormLevel) {
    if (tsNormLevel < 0.25f) return 0;
    if (tsNormLevel < 0.5f) return 3;
    if (tsNormLevel < 0.75f) return 8;
    return kIntMax;
}

// GameVariables.cs:595
int GameVariables::GetPatchesAheadLimit(float tsNormLevel) {
    if (tsNormLevel < 0.25f) return 0;
    if (tsNormLevel < 0.5f) return 1;
    if (tsNormLevel < 0.75f) return 2;
    return GameSettings::CONIC_PATCH_LIMIT;
}

// GameVariables.cs:612
bool GameVariables::UnlockedSpaceObjectDiscovery(float tsNormLevel) { return tsNormLevel > 0.6f; }

// GameVariables.cs:617
UntrackedObjectClass GameVariables::MinTrackedObjectSize(float tsNormLevel) {
    if (tsNormLevel < 0.2f) return UntrackedObjectClass::E;
    if (tsNormLevel < 0.4f) return UntrackedObjectClass::D;
    if (tsNormLevel < 0.6f) return UntrackedObjectClass::C;
    if (tsNormLevel < 0.8f) return UntrackedObjectClass::B;
    return UntrackedObjectClass::A;
}

// GameVariables.cs:638
bool GameVariables::ManeuverToolAvailable(float tsNormLevel) { return tsNormLevel > 0.75f; }

// GameVariables.cs:643
float GameVariables::GetScienceCostLimit(float RnDnormLevel) {
    if (RnDnormLevel < 0.25f) return 100.0f;
    if (RnDnormLevel < 0.5f) return 300.0f;
    if (RnDnormLevel < 0.75f) return 500.0f;
    return kFloatMax;
}

// GameVariables.cs:660
float GameVariables::GetDataToScienceRatio(float RnDnormLevel) {
    if (RnDnormLevel < 0.25f) return 0.4f;
    if (RnDnormLevel < 0.5f) return 0.6f;
    if (RnDnormLevel < 0.75f) return 0.8f;
    return 1.0f;
}

// GameVariables.cs:677
float GameVariables::GetExperimentLevel(float RnDnormLevel) { return RnDnormLevel; }

}  // namespace ksp
