// APSkillExtensions.cs and ModuleExperienceManagement.cs; see include/ksp/kerbals/kerbal_misc.hpp.
#include "ksp/kerbals/kerbal_misc.hpp"

#include <algorithm>
#include <cctype>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_values.hpp"

namespace ksp {

using Localizer = KSP::Localization::Localizer;
using AutopilotMode = VesselAutopilot_AutopilotMode;

namespace {
Game& CurrentGame() {
    Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr) throw NullReferenceException("HighLogic.CurrentGame is null");
    return *g;
}
GameParameters& Params() {
    Game& g = CurrentGame();
    if (!g.Parameters) throw NullReferenceException("HighLogic.CurrentGame.Parameters is null");
    return *g.Parameters;
}
}  // namespace

// ============================================== APSkillExtensions ===================================================

// APSkillExtensions.cs:6
bool APSkillExtensions::AvailableAtLevel(AutopilotMode mode, Vessel& vessel) {
    VesselValues& vesselValues = vessel.GetVesselValues();
    Game& game = CurrentGame();
    const Game_Modes gameMode = game.Mode;
    if (SaveEnvironment::Current().IsExpansionInstalled("MakingHistory") &&
        (gameMode == Game_Modes::MISSION || gameMode == Game_Modes::MISSION_BUILDER)) {
        if (Params().CustomParams<GameParameters::AdvancedParams>().EnableFullSASInMissions) return true;
        return vesselValues.AutopilotSkill.value() >= GetRequiredSkill(mode);
    }
    auto& adv = Params().CustomParams<GameParameters::AdvancedParams>();
    if (gameMode == Game_Modes::CAREER && adv.KerbalExperienceEnabled(gameMode))
        return vesselValues.AutopilotSkill.value() >= GetRequiredSkill(mode);
    if (gameMode == Game_Modes::SANDBOX || gameMode == Game_Modes::SCIENCE_SANDBOX) {
        if (!adv.EnableFullSASInSandbox) {
            if (adv.KerbalExperienceEnabled(gameMode)) return vesselValues.AutopilotSkill.value() >= GetRequiredSkill(mode);
            if (vesselValues.AutopilotKerbalSkill.value() <= -1 && vesselValues.RepairSkill.value() <= -1 &&
                vesselValues.ScienceSkill.value() <= -1)
                return vesselValues.AutopilotSASSkill.value() >= GetRequiredSkill(mode);
            return true;
        }
        if (adv.KerbalExperienceEnabled(gameMode)) {
            if (vesselValues.AutopilotKerbalSkill.value() <= -1 && vesselValues.RepairSkill.value() <= -1 &&
                vesselValues.ScienceSkill.value() <= -1)
                return vesselValues.AutopilotSASSkill.value() >= GetRequiredSkill(mode);
            return vesselValues.AutopilotSkill.value() >= GetRequiredSkill(mode);
        }
    }
    return true;
}

// APSkillExtensions.cs:61
bool APSkillExtensions::AvailableAtLevel(AutopilotMode mode, int skillLvl) {
    if (Params().CustomParams<GameParameters::AdvancedParams>().KerbalExperienceEnabled(CurrentGame().Mode))
        return skillLvl >= GetRequiredSkill(mode);
    return true;
}

// APSkillExtensions.cs:69
int APSkillExtensions::GetRequiredSkill(AutopilotMode mode) {
    switch (mode) {
    case AutopilotMode::Prograde:
    case AutopilotMode::Retrograde: return 1;
    case AutopilotMode::Normal:
    case AutopilotMode::Antinormal:
    case AutopilotMode::RadialIn:
    case AutopilotMode::RadialOut: return 2;
    case AutopilotMode::Target:
    case AutopilotMode::AntiTarget:
    case AutopilotMode::Maneuver: return 3;
    default: return 0;
    }
}

// ========================================= ModuleExperienceManagement ===============================================

namespace {
Vessel* VesselOf(PartModule& m) { return m.part != nullptr ? static_cast<Part*>(m.part)->vessel : nullptr; }
void Post(const std::string& text, float duration) {
    if (ModuleExperienceManagement::postScreenMessage) ModuleExperienceManagement::postScreenMessage(text, duration);
}
}  // namespace

// ModuleExperienceManagement.cs:14
void ModuleExperienceManagement::OnStart(StartState state) {
    PartModule::OnStart(state);
    auto& adv = Params().CustomParams<GameParameters::AdvancedParams>();
    if (!adv.GetEnableKerbalExperience() || adv.ImmediateLevelUp || CurrentGame().Mode == Game_Modes::MISSION) enabled = false;
}

// ModuleExperienceManagement.cs:23
std::string ModuleExperienceManagement::GetInfo() { return Localizer::Format("#autoLOC_230278"); }

// ModuleExperienceManagement.cs:29
void ModuleExperienceManagement::FinalizeExperience() {
    Vessel* vessel = VesselOf(*this);
    if (vessel == nullptr) throw NullReferenceException("ModuleExperienceManagement.vessel is null");
    if (commNetHasNoSignal && commNetHasNoSignal(*vessel)) {
        Post(Localizer::Format("#autoLOC_230288"), 5.0f);
        return;
    }
    int num = 0;
    const std::vector<std::shared_ptr<ProtoCrewMember>> vesselCrew = vessel->GetVesselCrew();
    int count = static_cast<int>(vesselCrew.size());
    const float cost = costPerKerbal();
    while (count-- > 0) {
        const std::shared_ptr<ProtoCrewMember>& protoCrewMember = vesselCrew[static_cast<size_t>(count)];
        if (KerbalRoster::CalculateExperience({&protoCrewMember->careerLog, &protoCrewMember->flightLog}) > protoCrewMember->experience) {
            if (cost != 0.0f && fundingCanAfford && !fundingCanAfford(cost)) {
                Post(Localizer::Format("#autoLOC_230323"), 5.0f);
                break;
            }
            if (cost > 0.0f && fundingAddFunds) fundingAddFunds(0.0f - cost);
            const int experienceLevel = protoCrewMember->experienceLevel;
            protoCrewMember->ArchiveFlightLog();
            std::string text;
            if (protoCrewMember->experienceLevel > experienceLevel) {
                if (!protoCrewMember->experienceTrait) throw NullReferenceException("ProtoCrewMember.experienceTrait is null");
                std::string title = protoCrewMember->experienceTrait->Title();
                std::transform(title.begin(), title.end(), title.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                text = Localizer::Format("#autoLOC_6001016", {protoCrewMember->nameWithGender(),
                                                              dotnet::FormatInt64(protoCrewMember->experienceLevel), title});
            }
            const std::string text2 = Localizer::Format("#autoLOC_7002001");
            Post(Localizer::Format("#autoLOC_6001015", {protoCrewMember->nameWithGender(), text2, text}), 5.0f);
            num++;
            log::Info("Archived flight log for " + protoCrewMember->name().value_or("") + ".");
        }
    }
    if (num == 0) Post(Localizer::Format("#autoLOC_230331"), 5.0f);
}

// ModuleExperienceManagement.cs:71
std::string ModuleExperienceManagement::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003041"); }

void RegisterExperienceManagementModule() {
    ModuleFactory::Register("ModuleExperienceManagement", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<ModuleExperienceManagement>(c));
    });
}

}  // namespace ksp
