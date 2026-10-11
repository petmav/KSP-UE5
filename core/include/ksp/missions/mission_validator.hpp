// WP-54: Editor/MissionEditorValidator.cs (the validation run over a mission and its result list), MissionCheckpointValidator.cs.
//
// MissionEditorValidator is a MonoBehaviour singleton of the mission builder scene. The data side is ported (Instance, Results, IsValid,
// Status, RunValidation, AddNode*, NextMode, the dedupe of AddNodeResult); the colours (GetStatusColor / GetValidationColor read the
// prefab's serialized ValidationColors / MEValidityHighlighColor lists) are editor UI and are not.
//
// MissionCheckpointValidator: the path recursion and the dirty-node tests are ported; `ModulesAreEqual` walks the [MEGUI_*] fields of the
// two modules by reflection (BaseAPFieldList, checkpointValidation = Equals / CustomMethod / Controls) and is the seam
// `MissionCheckpointValidator::modulesAreEqual` (editor reflection, WP-56); unset it compares the runtime type names only.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/missions/mission_node.hpp"
#include "ksp/missions/mission_types.hpp"

namespace ksp::Expansions::Missions {

class Mission;

namespace Editor {

// MissionEditorValidator.cs:8
class MissionEditorValidator {
public:
    using ResultList = std::vector<MissionValidationTestResult>;

    MissionEditorValidator();  // :32 Awake
    ~MissionEditorValidator();
    MissionEditorValidator(const MissionEditorValidator&) = delete;
    MissionEditorValidator& operator=(const MissionEditorValidator&) = delete;

    static MissionEditorValidator* Instance() { return instance_; }  // :20
    ValidatorMode mode = ValidatorMode::AutoAfterRun;                // :16 (internal)
    bool highlightGUINodes = true;                                   // :18
    bool IsValid() const;                                            // :22
    ValidationStatus Status() const;                                 // :44
    Mission* GetMission() const { return mission_; }                 // :70
    std::shared_ptr<ResultList> Results() const { return results_; }  // :72 (null before the first run)
    static ValidatorMode Mode();                                     // :74
    static void ResetValidator();                                    // :127
    static void RunValidationOnParamChange();                        // :136
    static void RunValidation(Mission* mission);                     // :144
    void AddResults(const ResultList& results);                      // :181
    void AddNodePass(MENode* node, const std::string& message);      // :186
    void AddNodeWarn(MENode* node, const std::string& message);      // :191
    void AddNodeFail(MENode* node, const std::string& message);      // :196
    void AddNodeResult(MENode* node, ValidationStatus result, const std::string& message);  // :201
    static ValidatorMode NextMode();                                 // :232

private:
    static inline MissionEditorValidator* instance_ = nullptr;
    bool destroyed_ = false;
    Mission* mission_ = nullptr;
    std::shared_ptr<ResultList> results_;
};

}  // namespace Editor

// MissionCheckpointValidator.cs:9
class MissionCheckpointValidator {
public:
    static bool ValidateCheckpoint(Mission* checkpointMission, Mission* mission);  // :14
    // :158/:164/:170 CompareStructLists / CompareObjectLists / CompareAnyLists: list1[i].Equals(list2[i]) over two lists of the same length
    template <class T>
    static bool CompareStructLists(const std::vector<T>& list1, const std::vector<T>& list2) {
        return CompareAnyLists(list1, list2);
    }
    template <class T>
    static bool CompareObjectLists(const std::vector<T>* list1, const std::vector<T>* list2) {
        if (list1 != nullptr && list2 != nullptr) return CompareAnyLists(*list1, *list2);
        return false;
    }
    template <class T>
    static bool CompareAnyLists(const std::vector<T>& list1, const std::vector<T>& list2) {
        if (list1.size() != list2.size()) return false;
        for (size_t num = 0; num < list1.size(); ++num)
            if (!(list1[num] == list2[num])) return false;
        return true;
    }
    // ModulesAreEqual(object, object) (:103): see the file comment. Arguments are the two nodes / test modules / action modules.
    static inline std::function<bool(const void* checkpointModule, const void* missionModule, const char* typeName)> modulesAreEqual;

private:
    static bool CheckCheckpointPathRecursive(Mission* checkpointMission, Mission* mission, MENode* checkpointNode, MENode* missionNode);  // :19
    static bool IsCheckpointNodeDirty(MENode* checkpointNode, MENode* missionNode);                                                     // :53
    static bool ModulesAreEqualImpl(const void* a, const void* b, const char* typeNameA, const char* typeNameB);                        // :103
};

}  // namespace ksp::Expansions::Missions
