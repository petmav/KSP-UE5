// WP-37: Contracts.TextGen (Assembly-CSharp/Contracts/TextGen.cs): the deterministic contract back-story generator. The text tables are the
// STORY_DEF nodes (GameData/Squad/Resources/StoryDefs.cfg, merged by GameDatabase.GetMergedConfigNodes("STORY_DEF", true)); the sentence
// choice draws from UnityEngine.Random seeded with the contract's mission seed.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/util/collections.hpp"

namespace ksp::Contracts {

class TextGen {
public:
    // TextGen.cs:20 Setup(): GameDatabase.Instance.GetMergedConfigNodes("STORY_DEF", true)
    static void Setup();
    // the same over an explicit merged node (tests, tools)
    static void Setup(const ConfigNode& mergedConfigNodes);
    static bool IsSetup() { return setup; }
    static void Reset();
    // TextGen.cs:41
    static std::string GenerateBackStories(const std::string& contractType, const std::string& agency, const std::string& topic,
                                           const std::string& subject, int seed, bool allowGenericIntroduction, bool allowGenericProblem,
                                           bool allowGenericConclusion);

private:
    static std::string GetSentence(const std::string& sentenceType, bool allowGeneric);        // :63
    static std::string ProcessTextReplacements(std::string src, bool wipeReplacements);        // :123
    static inline std::map<std::string, std::shared_ptr<ListDictionary<std::string, std::string>>> texts;  // :11
    static inline bool setup = false;                                                          // :13
    static inline std::string currentContractType, currentAgency, currentTopic, currentSubject;  // :15-:21
};

}  // namespace ksp::Contracts
