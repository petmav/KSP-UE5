// WP-36: GameVariables.cs, the table of career/facility formulas the rest of the game asks (part count / craft mass / craft size
// limits of the editors, crew and contract limits of the facilities, recovery value factors, the hire cost of a Kerbal, DSN range,
// science cost limit ...). Every method is virtual in C# (mods derive from it); here too.
//
// The facility levels are normalised levels (0..1) read from ScenarioUpgradeableFacilities.GetFacilityLevel. Float rule: all
// formulas are managed float code (ksp::fx / f32store, Mono).
//
// Prefab values: reputationAddition / reputationSubtraction are Unity-serialized AnimationCurves on the GameVariables prefab and
// are NOT part of any dataset in the repo (harness/golden/serialized has no GameVariables extract): a host installs them with
// SetReputationCurves; the scalar prefab values (reputationKerbalDeath ... mentalityDeadlineExceptional) are the C# field
// initialisers, which equal the serialized prefab values unless the prefab overrides them (unverified, STATUS.md "needs data").
//
// There is no GameVariables.GetActiveVesselMaxReputation in KSP 1.12.5.
#pragma once

#include <string>

#include "ksp/career/career_environment.hpp"
#include "ksp/curves/double_curve.hpp"
#include "ksp/physics/vessel_ranges.hpp"
#include "ksp/unity/animation_curve.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/vessel/discovery_info.hpp"

namespace ksp {

class CelestialBody;
class Vessel;
class ProtoCrewMember;

class GameVariables {
public:
    enum class OrbitDisplayMode : int { None, CelestialBodyOrbits, AllOrbits, PatchedConics };  // :8

    GameVariables();           // Awake (:80): becomes the Instance
    virtual ~GameVariables();  // OnDestroy (:92)
    static inline GameVariables* Instance = nullptr;  // :16

    AnimationCurve reputationAddition;     // :18 (prefab)
    AnimationCurve reputationSubtraction;  // :20 (prefab)
    float reputationKerbalDeath = 10.0f;               // :22
    float reputationKerbalRecovery = 25.0f;            // :24
    float partRecoveryValueFactor = 0.9f;              // :26
    float resourceRecoveryValueFactor = 0.95f;         // :28
    float contractDestinationWeight = 1.0f;            // :30
    float contractPrestigeTrivial = 1.0f;              // :32
    float contractPrestigeSignificant = 1.25f;         // :34
    float contractPrestigeExceptional = 1.5f;          // :36
    float contractFundsAdvanceFactor = 1.0f;           // :38
    float contractFundsCompletionFactor = 1.0f;        // :40
    float contractFundsFailureFactor = 1.0f;           // :42
    float contractReputationCompletionFactor = 1.0f;   // :44
    float contractReputationFailureFactor = 1.0f;      // :46
    float contractScienceCompletionFactor = 1.0f;      // :48
    float mentalityFundsTrivial = 1.1f;                // :50
    float mentalityFundsSignificant = 1.2f;            // :52
    float mentalityFundsExceptional = 1.3f;            // :54
    float mentalityReputationTrivial = 1.1f;           // :56
    float mentalityReputationSignificant = 1.2f;       // :58
    float mentalityReputationExceptional = 1.3f;       // :60
    float mentalityScienceTrivial = 1.1f;              // :62
    float mentalityScienceSignificant = 1.2f;          // :64
    float mentalityScienceExceptional = 1.3f;          // :66
    float mentalityExpiryTrivial = 1.1f;               // :68
    float mentalityExpirySignificant = 1.2f;           // :70
    float mentalityExpiryExceptional = 1.3f;           // :72
    float mentalityDeadlineTrivial = 1.1f;             // :74
    float mentalityDeadlineSignificant = 1.2f;         // :76
    float mentalityDeadlineExceptional = 1.3f;         // :78

    virtual float GetRecoveredPartValue(float pValue);                                  // :100
    virtual float GetRecoveredResourceValue(float rscValue);                            // :105
    virtual float GetContractPrestigeFactor(ContractPrestige prestige);                 // :110
    virtual float GetContractFundsAdvanceFactor(ContractPrestige prestige);             // :123
    virtual float GetContractFundsCompletionFactor(ContractPrestige prestige);          // :128
    virtual float GetContractFundsFailureFactor(ContractPrestige prestige);             // :133
    virtual float GetContractReputationCompletionFactor(ContractPrestige prestige);     // :138
    virtual float GetContractReputationFailureFactor(ContractPrestige prestige);        // :143
    virtual float GetContractScienceCompletionFactor(ContractPrestige prestige);        // :148
    virtual float GetContractDestinationWeight(CelestialBody* body);                    // :153
    virtual float ScoreSituation(VesselSituations sit, CelestialBody* where);           // :158
    virtual float ScoreFlightEnvelope(float altitude, float altEnvelope, float speed, float speedEnvelope);  // :201
    virtual float GetMentalityFundsFactor(float mentalityFactor, ContractPrestige prestige);       // :220
    virtual float GetMentalityReputationFactor(float mentalityFactor, ContractPrestige prestige);  // :233
    virtual float GetMentalityScienceFactor(float mentalityFactor, ContractPrestige prestige);     // :246
    virtual float GetMentalityExpiryFactor(float mentalityFactor, ContractPrestige prestige);      // :259
    virtual float GetMentalityDeadlineFactor(float mentalityFactor, ContractPrestige prestige);    // :272
    virtual int GetPartCountLimit(float editorNormLevel, bool isVAB);                   // :285
    virtual bool UnlockedFuelTransfer(float editorNormLevel);                           // :306
    virtual bool UnlockedActionGroupsStock(float editorNormLevel, bool isVAB);          // :311
    virtual bool UnlockedActionGroupsCustom(float editorNormLevel, bool isVAB);         // :320
    virtual float GetCraftMassLimit(float editorNormLevel, bool isPad);                 // :329
    virtual Vector3f GetCraftSizeLimit(float editorNormLevel, bool isPad);              // :350
    virtual int GetActiveStrategyLimit(float adminNormLevel);                           // :371
    virtual float GetStrategyCommitRange(float adminNormLevel);                         // :388
    virtual float GetStrategyLevelLimit(float adminNormLevel);                          // :401
    virtual int GetActiveContractsLimit(float mCtrlNormLevel);                          // :406
    virtual float GetContractLevelLimit(float mCtrlNormLevel);                          // :423
    virtual bool UnlockedFlightPlanning(float mCtrlNormLevel);                          // :428
    virtual int GetActiveCrewLimit(float astroComplexNormLevel);                        // :433
    virtual float GetCrewLevelLimit(float astroComplexNormLevel);                       // :450
    static float GetRecruitHireCost(int currentActive, float baseCost, float flatRate, float rateModifier);  // :463
    virtual float GetRecruitHireCost(int currentActive);                                // :468
    virtual bool UnlockedEVA(float astroComplexNormLevel);                              // :473
    virtual bool UnlockedEVAFlags(float astroComplexNormLevel);                         // :478
    virtual bool UnlockedEVAClamber(float astroComplexNormLevel);                       // :483
    virtual bool EVAIsPossible(bool evaUnlocked, Vessel* v);                            // :488
    virtual std::string GetEVALockedReason(Vessel* v, ProtoCrewMember* crew);           // :497
    virtual DoubleCurve GetDSNRangeCurve();                                             // :537
    virtual DoubleCurve GetDSNPowerCurve();                                             // :545
    virtual DoubleCurve GetDSNScienceCurve();                                           // :553
    virtual double GetDSNRange(float level);                                            // :562
    virtual OrbitDisplayMode GetOrbitDisplayMode(float tsNormLevel);                    // :569
    virtual int GetTrackedObjectLimit(float tsNormLevel);                               // :578
    virtual int GetPatchesAheadLimit(float tsNormLevel);                                // :595
    virtual bool UnlockedSpaceObjectDiscovery(float tsNormLevel);                       // :612
    virtual UntrackedObjectClass MinTrackedObjectSize(float tsNormLevel);               // :617
    virtual bool ManeuverToolAvailable(float tsNormLevel);                              // :638
    virtual float GetScienceCostLimit(float RnDnormLevel);                              // :643
    virtual float GetDataToScienceRatio(float RnDnormLevel);                            // :660
    virtual float GetExperimentLevel(float RnDnormLevel);                               // :677
};

}  // namespace ksp
