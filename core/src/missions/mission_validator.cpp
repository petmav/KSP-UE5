// WP-54: MissionEditorValidator.cs, MissionCheckpointValidator.cs. See mission_validator.hpp.
#include "ksp/missions/mission_validator.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission.hpp"

namespace ksp::Expansions::Missions {

namespace Editor {

// MissionEditorValidator.cs:32 Awake
MissionEditorValidator::MissionEditorValidator() {
    if (instance_ != nullptr) {
        destroyed_ = true;  // UnityEngine.Object.DestroyImmediate(this): a second validator is discarded
        return;
    }
    instance_ = this;
    mode = MissionsEnvironment::validatorSetting;
}

MissionEditorValidator::~MissionEditorValidator() {
    if (instance_ == this) instance_ = nullptr;
}

// MissionEditorValidator.cs:22
bool MissionEditorValidator::IsValid() const {
    if (!results_) throw NullReferenceException();
    int num = 0;
    while (true) {
        if (num < static_cast<int>(results_->size())) {
            if ((*results_)[static_cast<size_t>(num)].status != ValidationStatus::Pass) break;
            ++num;
            continue;
        }
        return true;
    }
    return false;
}

// MissionEditorValidator.cs:44
ValidationStatus MissionEditorValidator::Status() const {
    if (!results_) return ValidationStatus::None;
    ValidationStatus result = ValidationStatus::Pass;
    for (size_t i = 0; i < results_->size(); ++i) {
        if ((*results_)[i].status == ValidationStatus::Warn) result = ValidationStatus::Warn;
        if ((*results_)[i].status == ValidationStatus::Fail) {
            result = ValidationStatus::Fail;
            break;
        }
    }
    return result;
}

// MissionEditorValidator.cs:74
ValidatorMode MissionEditorValidator::Mode() {
    if (instance_ != nullptr) return instance_->mode;
    return ValidatorMode::Manual;
}

// MissionEditorValidator.cs:127
void MissionEditorValidator::ResetValidator() {
    if (instance_ != nullptr) {
        instance_->mission_ = nullptr;
        instance_->results_.reset();
    }
}

// MissionEditorValidator.cs:136
void MissionEditorValidator::RunValidationOnParamChange() {
    if (!(instance_ == nullptr) && !(instance_->mission_ == nullptr) && (Mode() == ValidatorMode::AutoAfterRun || Mode() == ValidatorMode::FullAuto) &&
        (Mode() != ValidatorMode::AutoAfterRun || instance_->mission_->hasBeenValidated)) {
        RunValidation(instance_->mission_);
    }
}

// MissionEditorValidator.cs:144
void MissionEditorValidator::RunValidation(Mission* mission) {
    if (instance_ == nullptr) {
        log::Error("[MissionEditorValidator] Insatnce is null - unable to run the validator");
        return;
    }
    instance_->mission_ = mission;
    instance_->results_ = std::make_shared<ResultList>();
    const std::vector<MENode*> list = instance_->mission_->nodes.GetListEnumerator();  // List<MENode>.Enumerator over the node list
    for (size_t i = 0; i < list.size(); ++i) {
        MENode* current = list[i];
        if (current->guiNode != nullptr) current->guiNode->ClearValidityIndicators();
        if (current->isStartNode) {
            instance_->mission_->situation->RunValidationWrapper(instance_);
        } else {
            current->RunValidationWrapper(instance_);
        }
    }
    instance_->mission_->validationResults = instance_->results_;
    instance_->mission_->hasBeenValidated = true;
    if (MissionsEnvironment::updateValidationLed) MissionsEnvironment::updateValidationLed();
}

// MissionEditorValidator.cs:181
void MissionEditorValidator::AddResults(const ResultList& results) { results_->insert(results_->end(), results.begin(), results.end()); }

// MissionEditorValidator.cs:186
void MissionEditorValidator::AddNodePass(MENode* node, const std::string& message) { AddNodeResult(node, ValidationStatus::Pass, message); }

// MissionEditorValidator.cs:191
void MissionEditorValidator::AddNodeWarn(MENode* node, const std::string& message) { AddNodeResult(node, ValidationStatus::Warn, message); }

// MissionEditorValidator.cs:196
void MissionEditorValidator::AddNodeFail(MENode* node, const std::string& message) { AddNodeResult(node, ValidationStatus::Fail, message); }

// MissionEditorValidator.cs:201
void MissionEditorValidator::AddNodeResult(MENode* node, ValidationStatus result, const std::string& message) {
    int num = 0;
    while (true) {
        if (num < static_cast<int>(results_->size())) {
            const MissionValidationTestResult& r = (*results_)[static_cast<size_t>(num)];
            if (!(r.nodeId == node->id()) || r.status != result || !(r.message == message)) {
                ++num;
                continue;
            }
            break;
        }
        MissionValidationTestResult missionValidationTestResult;
        missionValidationTestResult.nodeId = node->id();
        missionValidationTestResult.nodeName = node->Title();
        missionValidationTestResult.status = result;
        missionValidationTestResult.type = KSP::Localization::Localizer::Format("#autoLOC_8006049");
        missionValidationTestResult.message = message;
        results_->push_back(missionValidationTestResult);
        if (highlightGUINodes && result != ValidationStatus::Pass && node->guiNode != nullptr) node->guiNode->SetValidityIndicators(missionValidationTestResult);
        break;
    }
}

// MissionEditorValidator.cs:232
ValidatorMode MissionEditorValidator::NextMode() {
    if (instance_ != nullptr) {
        int num = static_cast<int>(instance_->mode);
        ++num;
        if (num < 0 || num > static_cast<int>(ValidatorMode::FullAuto)) num = 0;  // !Enum.IsDefined(typeof(ValidatorMode), num)
        instance_->mode = static_cast<ValidatorMode>(num);
        return instance_->mode;
    }
    return ValidatorMode::Manual;
}

}  // namespace Editor

// ---- MissionCheckpointValidator ----------------------------------------------------------------------------------------------

// MissionCheckpointValidator.cs:14
bool MissionCheckpointValidator::ValidateCheckpoint(Mission* checkpointMission, Mission* mission) {
    return CheckCheckpointPathRecursive(checkpointMission, mission, checkpointMission->startNode(), mission->startNode());
}

// MissionCheckpointValidator.cs:19
bool MissionCheckpointValidator::CheckCheckpointPathRecursive(Mission* checkpointMission, Mission* mission, MENode* checkpointNode, MENode* missionNode) {
    Flow::MENodePathInfo& mENodePathInfo = *checkpointMission->flow->NodePaths.At(checkpointNode);
    Flow::MENodePathInfo& mENodePathInfo2 = *mission->flow->NodePaths.At(missionNode);
    bool result = false;
    int i = 0;
    for (int count = static_cast<int>(mENodePathInfo.paths.size()); i < count; ++i) {
        int j = 0;
        for (int count2 = static_cast<int>(mENodePathInfo.paths[static_cast<size_t>(i)]->Nodes.size()); j < count2; ++j) {
            MENode* cn = mENodePathInfo.paths[static_cast<size_t>(i)]->Nodes[static_cast<size_t>(j)];
            if (cn->HasBeenActivated()) {
                if (i < static_cast<int>(mENodePathInfo2.paths.size()) && j < static_cast<int>(mENodePathInfo2.paths[static_cast<size_t>(i)]->Nodes.size()) &&
                    !IsCheckpointNodeDirty(cn, mENodePathInfo2.paths[static_cast<size_t>(i)]->Nodes[static_cast<size_t>(j)])) {
                    return CheckCheckpointPathRecursive(checkpointMission, mission, cn, mENodePathInfo2.paths[static_cast<size_t>(i)]->Nodes[static_cast<size_t>(j)]);
                }
                result = true;
            } else {
                result = false;
            }
        }
    }
    return result;
}

// MissionCheckpointValidator.cs:103
bool MissionCheckpointValidator::ModulesAreEqualImpl(const void* a, const void* b, const char* typeNameA, const char* typeNameB) {
    if (std::string(typeNameA) != std::string(typeNameB)) return false;  // checkpointModule.GetType() != missionModule.GetType()
    if (modulesAreEqual) return modulesAreEqual(a, b, typeNameA);
    return true;
}

// MissionCheckpointValidator.cs:53
bool MissionCheckpointValidator::IsCheckpointNodeDirty(MENode* checkpointNode, MENode* missionNode) {
    if (!(checkpointNode->id() == missionNode->id())) return true;
    if (!ModulesAreEqualImpl(checkpointNode, missionNode, "MENode", "MENode")) return true;
    if (checkpointNode->testGroups.size() != missionNode->testGroups.size()) return true;
    if (checkpointNode->actionModules.size() != missionNode->actionModules.size()) return true;
    int num = 0;
    int count = static_cast<int>(missionNode->testGroups.size());
    while (true) {
        if (num < count) {
            if (checkpointNode->testGroups[static_cast<size_t>(num)]->testModules.size() != missionNode->testGroups[static_cast<size_t>(num)]->testModules.size()) break;
            int i = 0;
            for (int count2 = static_cast<int>(missionNode->testGroups[static_cast<size_t>(num)]->testModules.size()); i < count2; ++i) {
                auto* a = static_cast<TestModule*>(checkpointNode->testGroups[static_cast<size_t>(num)]->testModules[static_cast<size_t>(i)].get());
                auto* b = static_cast<TestModule*>(missionNode->testGroups[static_cast<size_t>(num)]->testModules[static_cast<size_t>(i)].get());
                if (!ModulesAreEqualImpl(a, b, a->TypeName().c_str(), b->TypeName().c_str())) return true;
            }
            ++num;
            continue;
        }
        int num2 = 0;
        int count3 = static_cast<int>(missionNode->actionModules.size());
        while (true) {
            if (num2 < count3) {
                IActionModule* a = checkpointNode->actionModules[static_cast<size_t>(num2)].get();
                IActionModule* b = missionNode->actionModules[static_cast<size_t>(num2)].get();
                if (!ModulesAreEqualImpl(a, b, a->TypeName().c_str(), b->TypeName().c_str())) break;
                ++num2;
                continue;
            }
            return false;
        }
        return true;
    }
    return true;
}

}  // namespace ksp::Expansions::Missions
