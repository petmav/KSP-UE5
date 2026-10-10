// WP-37: Contracts.TextGen (Assembly-CSharp/Contracts/TextGen.cs).
#include "ksp/contracts/text_gen.hpp"

#include "ksp/core/log.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/unity/random.hpp"

namespace ksp::Contracts {

using KSP::Localization::Localizer;

// TextGen.cs:20
void TextGen::Setup() {
    ConfigNode::NodePtr merged = GameDatabase::Instance()->GetMergedConfigNodes("STORY_DEF", true);
    Setup(*merged);
}

// TextGen.cs:20 (body)
void TextGen::Setup(const ConfigNode& mergedConfigNodes) {
    texts.clear();
    int num = 0;
    for (int i = 0; i < mergedConfigNodes.nodes().Count(); ++i) {
        const ConfigNode& configNode = *mergedConfigNodes.nodes()[i];
        auto dict = std::make_shared<ListDictionary<std::string, std::string>>();
        texts[configNode.name] = dict;
        const auto& vals = configNode.values();
        int count = vals.Count();
        while (count-- > 0) {
            dict->Add(vals[count].name, vals[count].value);
            ++num;
        }
    }
    log::Info("Text Generator Loaded: " + std::to_string(num) + " entries.");
    setup = true;
}

void TextGen::Reset() {
    texts.clear();
    setup = false;
}

// TextGen.cs:41
std::string TextGen::GenerateBackStories(const std::string& contractType, const std::string& agency, const std::string& topic,
                                         const std::string& subject, int seed, bool allowGenericIntroduction, bool allowGenericProblem,
                                         bool allowGenericConclusion) {
    if (!setup) Setup();
    const std::string suffix = "Contract";
    currentContractType = contractType;
    if (contractType.size() >= suffix.size() && contractType.compare(contractType.size() - suffix.size(), suffix.size(), suffix) == 0)
        currentContractType = contractType.substr(0, contractType.rfind(suffix));
    currentAgency = agency;
    currentSubject = subject;
    currentTopic = topic;
    UnityRandom::Global().InitState(seed);
    const std::string s1 = GetSentence("Introduction", allowGenericIntroduction);
    const std::string s2 = GetSentence("Problem", allowGenericProblem);
    const std::string s3 = GetSentence("Conclusion", allowGenericConclusion);
    std::string text = s1 + " " + s2 + " " + s3;
    const int num = 8;
    int num2 = 0;
    while (text.find('[') != std::string::npos && num2 < num) {
        ++num2;
        text = ProcessTextReplacements(text, num2 >= num);
    }
    return Localizer::Format(text, {currentAgency, currentTopic});
}

// TextGen.cs:63
std::string TextGen::GetSentence(const std::string& sentenceType, bool allowGeneric) {
    auto it = texts.find(sentenceType);
    if (it == texts.end()) {
        log::Error("[TextGen]: No loaded texts for '" + sentenceType + "'");
        return std::string();
    }
    ListDictionary<std::string, std::string>& value = *it->second;
    int num = 0;
    ListDictionary<std::string, std::string>::List val;
    if (!currentSubject.empty() && value.TryGetValue(currentSubject, val)) {
        num += static_cast<int>(val->size());
        if (val->empty()) val = nullptr;
    } else {
        val = nullptr;
    }
    ListDictionary<std::string, std::string>::List val2;
    if ((num == 0 || allowGeneric) && value.TryGetValue(currentContractType, val2)) {
        num += static_cast<int>(val2->size());
        if (val2->empty()) val2 = nullptr;
    } else {
        val2 = nullptr;
    }
    ListDictionary<std::string, std::string>::List val3;
    if ((num == 0 || allowGeneric) && value.TryGetValue("Generic", val3)) {
        num += static_cast<int>(val3->size());
        if (val3->empty()) val3 = nullptr;
    } else {
        val3 = nullptr;
    }
    int num2 = UnityRandom::Global().Range(0, num);
    if (val2) {
        if (num2 < static_cast<int>(val2->size())) return (*val2)[static_cast<size_t>(num2)];
        num2 -= static_cast<int>(val2->size());
    }
    if (val) {
        if (num2 < static_cast<int>(val->size())) return (*val)[static_cast<size_t>(num2)];
        num2 -= static_cast<int>(val->size());
    }
    if (val3 && num2 < static_cast<int>(val3->size())) return (*val3)[static_cast<size_t>(num2)];
    log::Warning("[TextGen]: Unable to generate text for " + sentenceType + "!");
    return std::string();
}

// TextGen.cs:123
std::string TextGen::ProcessTextReplacements(std::string src, bool wipeReplacements) {
    (void)wipeReplacements;
    if (src.find('[') != std::string::npos) {
        size_t last0 = src.rfind('[');
        long long num = last0 == std::string::npos ? -1 : static_cast<long long>(last0);
        while (num != -1) {
            const size_t num2p = src.find(']', static_cast<size_t>(num));
            if (num2p != std::string::npos) {
                const long long num2 = static_cast<long long>(num2p);
                const std::string text = src.substr(static_cast<size_t>(num + 1), static_cast<size_t>(num2 - num - 1));
                if (text.find('[') == std::string::npos && text.find(']') == std::string::npos) {
                    bool flag = false;
                    if (num >= 4) {
                        const std::string text2 = src.substr(static_cast<size_t>(num - 4), 4);
                        if (text2[0] == '<' && text2[1] == '<' && text2[3] == ':') flag = true;
                    }
                    src.erase(static_cast<size_t>(num), static_cast<size_t>(num2 - num + 1));
                    std::string value;
                    if (!(text == "Agency")) {
                        if (text == "Topic") value = (!flag) ? "<<2>>" : "2";
                    } else {
                        value = (!flag) ? "<<1>>" : "1";
                    }
                    src.insert(static_cast<size_t>(num), value);
                    const size_t last = src.rfind('[');
                    num = last == std::string::npos ? -1 : static_cast<long long>(last);
                    continue;
                }
                log::Error("TextGenerator Syntax Error: Mismatched brackets on replacement block: " + src +
                           ". nesting is not supported, ensure all opening brackets are closed.");
                return src;
            }
            log::Error("TextGenerator Syntax Error: Missing closing bracket on replacement block: " + src);
            return src;
        }
    }
    return src;
}

}  // namespace ksp::Contracts
