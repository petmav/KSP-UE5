// CrewGenerator.cs (WP-31): random kerbal names and prototypes.
//
// Game data: the four name tables are Unity TextAssets in KSP_x64_Data (Resources "TextAssets/CrewGenerator/<lang>/
// KerbalNamesMale|KerbalNamesFemale|KerbalSpecialNamesMale|KerbalSpecialNamesFemale", ConfigNode text with a
// `KerbalNamesMale { Element = ... }` node). They are not part of the GameData mirror: a host supplies them through
// `CrewGenerator::loadTextAsset` (Resources.Load(path).text) or `SetNameTables` (tools/wp31/extract_crew_names.py writes
// the text assets from a KSP install). Without tables GetRandomName throws NullReferenceException, as the C# does before
// Initialize().
//
// Randomness: every draw goes through ksp::KSPRandom exactly as in CrewGenerator.cs; the two seed sources
// (Environment.TickCount, Guid.NewGuid().GetHashCode()) are SaveEnvironment hooks so a test can pin the seed.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/dotnet/random.hpp"
#include "ksp/save/crew.hpp"

namespace ksp {

class CrewGenerator {
public:
    // Resources.Load(resourcePath) as TextAsset -> .text (nullopt: asset missing; the C# throws NullReferenceException)
    static inline std::function<std::optional<std::string>(const std::string& resourcePath)> loadTextAsset;
    // Localizer.GetLanguageIdFromFile() (buildID64.txt `language = xx`); unset: the localizer's current language
    static inline std::function<std::string()> languageIdFromFile;
    // MissionEditorLogic.Instance.EditorMission.situation.gameParameters; unset or returning nullptr: no mission editor
    static inline std::function<GameParameters*()> missionEditorParameters;

    // CrewGenerator.cs:7-15 internal accessors (nullopt until Initialize / SetNameTables)
    static const std::optional<std::vector<std::string>>& KerbalNamesMale();
    static const std::optional<std::vector<std::string>>& KerbalNamesFemale();
    static const std::optional<std::vector<std::string>>& SpecialNamesMale();
    static const std::optional<std::vector<std::string>>& SpecialNamesFemale();
    static int specialNameChance;  // :14

    // :24 (needs loadTextAsset)
    static void Initialize();
    // kspcore: the effect of Initialize with the given tables (the lists the four text assets contain) and last name
    static void SetNameTables(std::vector<std::string> male, std::vector<std::string> female,
                              std::vector<std::string> specialMale, std::vector<std::string> specialFemale,
                              const std::string& lastName);
    static void Reset();  // tests: forget the tables

    // :34 (seeds: Environment.TickCount ^ Guid.NewGuid().GetHashCode() through SaveEnvironment)
    static std::shared_ptr<ProtoCrewMember> RandomCrewMemberPrototype(
        ProtoCrewMember_KerbalType type = ProtoCrewMember_KerbalType::Crew);
    // :34 with an explicit seed for the first generator (what the two hooks would have produced)
    static std::shared_ptr<ProtoCrewMember> RandomCrewMemberPrototypeSeeded(int32_t seed, ProtoCrewMember_KerbalType type);

    // :77 GetRandomName(Gender, Random generator = null). `generator` is a KSPRandom, a System.Random, or null (new KSPRandom()).
    static std::string GetRandomName(ProtoCrewMember_Gender g, KSPRandom* generator = nullptr);
    static std::string GetRandomName(ProtoCrewMember_Gender g, dotnet::SystemRandom& generator);
    static std::string GetFullName(const std::string& name, const std::string& lastName);  // :122
    static std::string GetLastName();                                                       // :138
    static std::string RemoveLastName(const std::string& fullKerbalName);                   // :145

private:
    static std::string GetRandomNameImpl(ProtoCrewMember_Gender g, const std::function<int32_t(int32_t)>& next);
    static bool LoadNameArray(std::optional<std::vector<std::string>>& nameArray, const std::string& filePrefix,
                              ProtoCrewMember_Gender gender);  // :131
    static std::string GetFileName(const std::string& filePrefix, ProtoCrewMember_Gender gender);  // :131
};

}  // namespace ksp
