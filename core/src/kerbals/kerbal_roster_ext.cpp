// KerbalRoster.cs members beyond WP-04's data side (WP-31): generated kerbals and applicants, Update, experience log,
// CheatExperience. The data side is src/save/crew.cpp.
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/kerbals/crew_generator.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/unity/random.hpp"

namespace ksp {

using Localizer = KSP::Localization::Localizer;

namespace {
constexpr int applicantGroupSizeTarget = 10;    // KerbalRoster.cs:19
constexpr int applicantGroupSizeVariance = 3;   // KerbalRoster.cs:21
}  // namespace

// KerbalRoster.cs:234
void KerbalRoster::RepopulateCrewRoster(int rosterSize) {
    int num = 0;
    for (int i = 0; i < Count(); ++i)
        if ((*this)[i]->rosterStatus() == ProtoCrewMember::RosterStatus::Available) num++;
    for (int j = num; j < rosterSize; ++j) GetNewKerbal();
}

// KerbalRoster.cs:267
void KerbalRoster::CheckRosterRespawn() {
    Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr || !g->Parameters) throw NullReferenceException("KerbalRoster.CheckRosterRespawn: HighLogic.CurrentGame is null");
    CheckRosterRespawn(SaveUniversalTime(), *g->Parameters);
}

// KerbalRoster.cs:281
KerbalRoster::PCM KerbalRoster::GetNextOrNewKerbal(ProtoCrewMember_KerbalType type) {
    PCM nextAvailableKerbal = GetNextAvailableKerbal(type);
    if (nextAvailableKerbal == nullptr) return GetNewKerbal(type);
    return nextAvailableKerbal;
}

// KerbalRoster.cs:291
KerbalRoster::PCM KerbalRoster::GetNewKerbal(ProtoCrewMember_KerbalType type) {
    PCM protoCrewMember;
    do {
        protoCrewMember = CrewGenerator::RandomCrewMemberPrototype(type);
    } while (!AddCrewMember(protoCrewMember));
    return protoCrewMember;
}

// KerbalRoster.cs:788
void KerbalRoster::PCMUpdate(double UT) {
    for (int i = 0, count = Count(); i < count; ++i) (*this)[i]->Update(UT);
}

// KerbalRoster.cs:797
void KerbalRoster::Update(double UT) {
    int num = 0;
    bool flag = false;
    int count = Count();
    while (count-- > 0) {
        PCM protoCrewMember = (*this)[count];
        if (protoCrewMember->type() == ProtoCrewMember::KerbalType::Applicant) {
            if (protoCrewMember->UTaR > 0.0 && protoCrewMember->UTaR < UT) {
                flag = true;
                Remove(protoCrewMember);
            } else {
                num++;
            }
        }
    }
    if (flag || num == 0) {
        const int b = applicantGroupSizeTarget + UnityRandom::Global().Range(-applicantGroupSizeVariance, applicantGroupSizeVariance);
        int num2 = (num > b ? num : b) - num;  // Mathf.Max(num, b)
        while (num2-- > 0) AddApplicant(UT);
    }
}

// KerbalRoster.cs:837
void KerbalRoster::AddApplicant(double UT) {
    PCM protoCrewMember;
    do {
        protoCrewMember = CrewGenerator::RandomCrewMemberPrototype(ProtoCrewMember::KerbalType::Applicant);
    } while (Exists(protoCrewMember->name().value_or("")));
    protoCrewMember->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
    const float days = f32store(fx(UnityRandom::Global().Range(1.0f, 3.0f)) * fx(86400.0f));
    protoCrewMember->UTaR = UT + static_cast<double>(days);
    protoCrewMember = std::make_shared<ProtoCrewMember>(ProtoCrewMember::CopyOf{}, *protoCrewMember);
    AddCrewMember(protoCrewMember);
}

// KerbalRoster.cs:1159
std::string KerbalRoster::GenerateExperienceLog(const FlightLog& log) {
    std::vector<std::string> list;
    std::vector<std::string> list2;
    std::vector<float> list3;
    const std::vector<FlightLog> flights = log.GetFlights();
    for (size_t i = 0; i < flights.size(); ++i) {
        if (!ExperienceAddFlight(flights[i], list, list2, list3)) {
            list.clear();
            list3.clear();
            list2.clear();
        }
    }
    std::string text;
    for (size_t j = 0; j < list.size(); ++j) {
        if (!text.empty()) text += "\n";
        // gettargetdisplayName (:1188): the display name of the body whose bodyName matches, else the name itself
        std::string target = list[j];
        for (CelestialBody* body : SaveEnvironment::Current().localBodies)
            if (body != nullptr && body->name() == list[j]) {
                target = body->displayName();
                break;
            }
        text = text + Localizer::Format(list2[j], {target}) + " = " + dotnet::FormatSingleF(list3[j], 0);
    }
    return text;
}

// KerbalRoster.cs:1372 (the CrewListItem refresh at the end is UI)
void KerbalRoster::CheatExperience() {
    Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr || !g->CrewRoster || !g->Parameters ||
        !g->Parameters->CustomParams<GameParameters::AdvancedParams>().KerbalExperienceEnabled(g->Mode))
        return;
    for (const PCM& item : g->CrewRoster->Crew()) SetExperienceLevel(*item, GetExperienceMaxLevel());
}

}  // namespace ksp
