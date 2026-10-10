// CrewGenerator.cs; see include/ksp/kerbals/crew_generator.hpp.
#include "ksp/kerbals/crew_generator.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/misc_util.hpp"

namespace ksp {

using Localizer = KSP::Localization::Localizer;
using Gender = ProtoCrewMember_Gender;

namespace {
std::optional<std::vector<std::string>> g_kerbalNamesMale, g_kerbalNamesFemale, g_specialNamesMale, g_specialNamesFemale;
std::optional<std::string> g_crewLastName;  // CrewGenerator.crewLastName (null before Initialize)

std::string GenderName(Gender g) { return g == Gender::Male ? "Male" : "Female"; }

// Localizer.GetLanguageIdFromFile()
std::string LanguageId() {
    if (CrewGenerator::languageIdFromFile) return CrewGenerator::languageIdFromFile();
    if (Localizer::Instance() == nullptr) return "en-us";
    const std::string lang = Localizer::CurrentLanguage();
    return lang.empty() ? std::string("en-us") : lang;
}
}  // namespace

int CrewGenerator::specialNameChance = 20;

const std::optional<std::vector<std::string>>& CrewGenerator::KerbalNamesMale() { return g_kerbalNamesMale; }
const std::optional<std::vector<std::string>>& CrewGenerator::KerbalNamesFemale() { return g_kerbalNamesFemale; }
const std::optional<std::vector<std::string>>& CrewGenerator::SpecialNamesMale() { return g_specialNamesMale; }
const std::optional<std::vector<std::string>>& CrewGenerator::SpecialNamesFemale() { return g_specialNamesFemale; }

// CrewGenerator.cs:28
void CrewGenerator::Initialize() {
    LoadNameArray(g_kerbalNamesMale, "KerbalNames", Gender::Male);
    LoadNameArray(g_kerbalNamesFemale, "KerbalNames", Gender::Female);
    LoadNameArray(g_specialNamesMale, "KerbalSpecialNames", Gender::Male);
    LoadNameArray(g_specialNamesFemale, "KerbalSpecialNames", Gender::Female);
    g_crewLastName = Localizer::Format("#autoLoc_6003018");
}

void CrewGenerator::SetNameTables(std::vector<std::string> male, std::vector<std::string> female,
                                  std::vector<std::string> specialMale, std::vector<std::string> specialFemale,
                                  const std::string& lastName) {
    g_kerbalNamesMale = std::move(male);
    g_kerbalNamesFemale = std::move(female);
    g_specialNamesMale = std::move(specialMale);
    g_specialNamesFemale = std::move(specialFemale);
    g_crewLastName = lastName;
}

void CrewGenerator::Reset() {
    g_kerbalNamesMale.reset();
    g_kerbalNamesFemale.reset();
    g_specialNamesMale.reset();
    g_specialNamesFemale.reset();
    g_crewLastName.reset();
}

// CrewGenerator.cs:37
std::shared_ptr<ProtoCrewMember> CrewGenerator::RandomCrewMemberPrototype(ProtoCrewMember_KerbalType type) {
    const SaveEnvironment& env = SaveEnvironment::Current();
    // new KSPRandom(Environment.TickCount ^ Guid.NewGuid().GetHashCode()): TickCount is evaluated first
    const int32_t tick = env.TickCount();
    const int32_t guid = env.NewGuidHashCode();
    return RandomCrewMemberPrototypeSeeded(tick ^ guid, type);
}

// CrewGenerator.cs:37
std::shared_ptr<ProtoCrewMember> CrewGenerator::RandomCrewMemberPrototypeSeeded(int32_t seed,
                                                                                ProtoCrewMember_KerbalType type) {
    KSPRandom kSPRandom(seed);
    const Gender gender = kSPRandom.Next(2) > 0 ? Gender::Female : Gender::Male;
    auto protoCrewMember = std::make_shared<ProtoCrewMember>(type, GetRandomName(gender, &kSPRandom));
    protoCrewMember->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
    kSPRandom = KSPRandom(StringExtensions::GetHashCode_Net35(protoCrewMember->name().value_or("")));
    protoCrewMember->courage = static_cast<float>(kSPRandom.NextDouble());
    protoCrewMember->stupidity = static_cast<float>(kSPRandom.NextDouble());
    protoCrewMember->isBadass = kSPRandom.Next(10) == 0;
    protoCrewMember->hasToured = false;
    protoCrewMember->set_gender(gender);
    KerbalRoster::SetExperienceTrait(*protoCrewMember);
    GameParameters* missionParams = missionEditorParameters ? missionEditorParameters() : nullptr;
    if (missionParams != nullptr) {  // MissionEditorLogic.Instance != null
        int maxValue = 6;
        auto& general = missionParams->CustomParams<Expansions::Missions::MissionParamsGeneral>();
        if (general.enableKerbalLevels) {
            const std::string t = protoCrewMember->trait.value_or("");
            if (t == "Scientist") maxValue = general.kerbalLevelScientist;
            else if (t == "Engineer") maxValue = general.kerbalLevelEngineer;
            else if (t == "Pilot") maxValue = general.kerbalLevelPilot;
        }
        protoCrewMember->experienceLevel = kSPRandom.Next(maxValue);
        KerbalRoster::SetExperienceLevel(*protoCrewMember, protoCrewMember->experienceLevel, *missionParams,
                                         Game_Modes::MISSION);
    }
    return protoCrewMember;
}

// CrewGenerator.cs:74 (the generator is a KSPRandom: `kSPRandom?.Next(n) ?? generator.Next(n)` takes the first arm; a null
// generator makes `new KSPRandom()` seeded with Environment.TickCount)
std::string CrewGenerator::GetRandomName(Gender g, KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(SaveEnvironment::Current().TickCount());
        generator = own.get();
    }
    return GetRandomNameImpl(g, [generator](int32_t n) { return generator->Next(n); });
}

// CrewGenerator.cs:74 (generator is a plain System.Random: `generator as KSPRandom` is null, generator.Next is used)
std::string CrewGenerator::GetRandomName(Gender g, dotnet::SystemRandom& generator) {
    return GetRandomNameImpl(g, [&generator](int32_t n) { return generator.Next(n); });
}

std::string CrewGenerator::GetRandomNameImpl(Gender g, const std::function<int32_t(int32_t)>& next) {
    const std::optional<std::vector<std::string>>& array = g == Gender::Female ? g_specialNamesFemale : g_specialNamesMale;
    const std::optional<std::vector<std::string>>& array2 = g == Gender::Female ? g_kerbalNamesFemale : g_kerbalNamesMale;
    std::string empty;
    std::string fullName;
    bool flag;
    do {
        if (next(specialNameChance) != 0) {
            if (!array2) throw NullReferenceException("CrewGenerator.kerbalNames is null");
            empty = (*array2)[static_cast<size_t>(next(static_cast<int32_t>(array2->size())))];
        } else {
            if (!array) throw NullReferenceException("CrewGenerator.specialNames is null");
            empty = (*array)[static_cast<size_t>(next(static_cast<int32_t>(array->size())))];
        }
        fullName = GetFullName(empty, g_crewLastName.value_or(""));
        Game* game = SaveEnvironment::Current().currentGame;
        if (game == nullptr || !game->CrewRoster) break;
        KerbalRoster& crewRoster = *game->CrewRoster;
        flag = false;
        for (int i = 0; i < crewRoster.Count(); ++i) {
            const std::string name = crewRoster[i]->name().value_or("");
            if (name.find(empty) != std::string::npos) flag = true;
        }
    } while (flag);
    return fullName;
}

// CrewGenerator.cs:114
std::string CrewGenerator::GetFullName(const std::string& name, const std::string& lastName) {
    const std::string languageIdFromFile = LanguageId();
    (void)lastName;
    if (languageIdFromFile == "ja") return name + g_crewLastName.value_or("");
    return name + " " + g_crewLastName.value_or("");
}

// CrewGenerator.cs:124
bool CrewGenerator::LoadNameArray(std::optional<std::vector<std::string>>& nameArray, const std::string& filePrefix,
                                  Gender gender) {
    if (!loadTextAsset) throw NullReferenceException("CrewGenerator: no TextAsset loader (Resources.Load)");
    const std::optional<std::string> text = loadTextAsset("TextAssets/" + GetFileName(filePrefix, gender));
    if (!text) throw NullReferenceException("TextAsset not found");
    ConfigNode::NodePtr configNode = ConfigNode::Parse(*text);
    ConfigNode* node = configNode->GetNode(filePrefix + GenderName(gender));
    if (node == nullptr) throw NullReferenceException("CrewGenerator: name node missing");
    nameArray = node->GetValues("Element");
    configNode->ClearData();
    return true;
}

// CrewGenerator.cs:132
std::string CrewGenerator::GetFileName(const std::string& filePrefix, Gender gender) {
    return "CrewGenerator/" + LanguageId() + "/" + filePrefix + GenderName(gender);
}

// CrewGenerator.cs:138 (WP-04 port: KerbalRoster::CrewGenerator_GetLastName)
std::string CrewGenerator::GetLastName() { return KerbalRoster::CrewGenerator_GetLastName().value_or(""); }

// CrewGenerator.cs:145
std::string CrewGenerator::RemoveLastName(const std::string& fullKerbalName) {
    return KerbalRoster::CrewGenerator_RemoveLastName(fullKerbalName);
}

}  // namespace ksp
