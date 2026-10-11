// WP-38: FinePrint (Assembly-CSharp/FinePrint/**): OrbitUtilities, the ProgressTrackingParameter and the ExplorationContract (the mandatory
// World First contract). The stateless utilities that WP-37 started live in contracts/fineprint_lite.hpp (namespace
// ksp::Contracts::Lite, extended in place by WP-38: ProgressUtilities, CelestialUtilities, SystemUtilities, StringUtilities); the
// Waypoint / WaypointManager data side is in contracts/waypoints.hpp.
//
// Seams: none beyond the ones WP-36 / WP-37 own (CareerEnvironment, ContractsEnvironment, TemplateEnvironment). `FinePrint::Install()` wires the
// WP-36 CareerEnvironment hooks (WorldFirstStandardReward, WorldFirstIntervalReward, FindNextRecord, ContractDefs.Progression record values,
// GreatCircleDistance) to the real functions and registers the FinePrint contract and parameter types.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/contracts/contract.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/orbit/orbit.hpp"

namespace ksp::FinePrint {

// FinePrint/Utilities/OrbitType.cs:3
enum class OrbitType : int { SYNCHRONOUS, STATIONARY, POLAR, EQUATORIAL, KOLNIYA, TUNDRA, RANDOM };

namespace Utilities::OrbitUtilities {
// OrbitUtilities.cs:79 / :84
Vector3d PositionOfApoapsis(const Orbit& o);
Vector3d PositionOfPeriapsis(const Orbit& o);
// :89 / :102
double AngleOfAscendingNode(const Orbit& currentOrbit, const Orbit& targetOrbit);
double AngleOfDescendingNode(const Orbit& currentOrbit, const Orbit& targetOrbit);
// :107 (Vector3 = float: GetOrbitNormal().normalized is a Vector3d -> implicit Vector3 in the C#, see the .cpp)
double GetRelativeInclination(const Orbit& a, const Orbit& b);
// :112 IdentifyOrbit
OrbitType IdentifyOrbit(const Orbit& o);
// :154 GenerateOrbit(seed, targetBody, orbitType, altitudeDifficulty, inclinationDifficulty, eccentricityOverride)
Orbit GenerateOrbit(int seed, CelestialBody* targetBody, OrbitType orbitType, double altitudeDifficulty, double inclinationDifficulty,
                    double eccentricityOverride = 0.0);
// :172 ValidateOrbit (the generator is KSPRandom(seed))
bool ValidateOrbit(int seed, Orbit& orbit, OrbitType orbitType, double altitudeDifficulty, double inclinationDifficulty, const std::string& source = "");
}  // namespace Utilities::OrbitUtilities

}  // namespace ksp::FinePrint

namespace ksp::Contracts::Parameters {

// FinePrint/Contracts/Parameters/ProgressTrackingParameter.cs:9
class ProgressTrackingParameter : public ContractParameter {
public:
    Lite::ProgressMilestone milestone;                                    // :11
    ProgressTrackingParameter();                                          // :164
    explicit ProgressTrackingParameter(const Lite::ProgressMilestone& milestone);  // :164
    std::string TypeName() const override { return "ProgressTrackingParameter"; }
    bool IsTutorial();                                                    // :15
    std::string InstructionalNote();                                      // :36
    std::string FlavorNote();                                             // :96
    std::string GetHashString() override;                                 // :174
    std::string GetTitle() override;                                      // :179
    std::string GetNotes() override;                                      // :232
    std::string GetMessageComplete() override;                            // :247
    void OnRegister() override;                                           // :312
    void OnUnregister() override;                                         // :323
    void OnSave(ConfigNode& node) override;                               // :339
    void OnLoad(ConfigNode& node) override;                               // :345
    void ProgressCheck(ProgressNode* node);                               // :331

private:
    bool eventsAdded = false;                                             // :13
};

}  // namespace ksp::Contracts::Parameters

namespace ksp::FinePrint::Contracts {

namespace Lite = ::ksp::Contracts::Lite;

// FinePrint/Contracts/ExplorationContract.cs:12
class ExplorationContract : public ksp::Contracts::Contract {
public:
    std::string TypeName() const override { return "ExplorationContract"; }
    bool IsTutorial();                                                    // :59
    std::vector<ksp::Contracts::Parameters::ProgressTrackingParameter*>& TrackingParameters();  // :71
    bool Generate() override;                                             // :95
    bool CanBeCancelled() override { return false; }                      // :121
    bool CanBeDeclined() override { return false; }                       // :126
    bool CanBeFailed() override { return false; }                         // :131
    std::string GetTitle() override;                                      // :136
    std::string GetDescription() override;                                // :156
    std::string GetSynopsys() override;                                   // :175
    std::string MessageCompleted() override;                              // :192
    void OnSave(ConfigNode& node) override;                               // :219
    void OnLoad(ConfigNode& node) override;                               // :225
    bool MeetRequirements() override;                                     // :231
    CelestialBody* TargetBody() const { return targetBody; }
    float BodyRatio() const { return bodyRatio; }

private:
    // MilestoneComparer (ExplorationContract.cs:15) is a free function in the .cpp
    int AllowableContracts(const std::vector<Lite::ProgressMilestone>& milestones);                 // :247
    void SetLocals();                                                     // :281
    void SetPrestige();                                                   // :303
    void SetRewards();                                                    // :340
    std::vector<Lite::ProgressMilestone> GetMissionMilestones();          // :367
    std::vector<Lite::ProgressMilestone> GetSeedMilestones(KSPRandom* generator = nullptr);  // :389
    std::vector<Lite::ProgressMilestone> GetThemedMilestones(Lite::ProgressMilestone& seed);  // :686

    std::vector<ksp::Contracts::Parameters::ProgressTrackingParameter*> trackingParameters;  // :45
    bool trackingParametersBuilt = false;
    std::vector<ksp::Contracts::Parameters::ProgressTrackingParameter*> emptyList_;  // the fresh empty list of the C# getter
    CelestialBody* targetBody = nullptr;                                  // :47
    float bodyRatio = 0.0f;                                               // :49
    bool rendezvousUnlocked = false;                                      // :51
    bool dockUnlocked = false;                                            // :53
    bool evaUnlocked = false;                                             // :55
    bool flagsUnlocked = false;                                           // :57
};

}  // namespace ksp::FinePrint::Contracts

namespace ksp::FinePrint {
// Registers ExplorationContract / ProgressTrackingParameter in the ContractSystem type tables and installs the CareerEnvironment hooks
// that ProgressUtilities / StringUtilities / CelestialUtilities own (see the header comment). Idempotent.
void Install();
}  // namespace ksp::FinePrint
