// WP-53: ControlledBase.cs, RoboticCollisions.cs, SerenityUtils.cs (see include/ksp/serenity/serenity_misc.hpp).
#include "ksp/serenity/serenity_misc.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp::Expansions::Serenity {

void SerenityMiscEnvironment::Reset() {
    persistentActionId = nullptr;
    partCollision = nullptr;
    collisionMessages.clear();
    loadAsset = nullptr;
}

namespace {
Part* FindLoadedPart(uint32_t id) {  // FlightGlobals.FindLoadedPart
    SaveEnvironment& env = SaveEnvironment::Current();
    if (id == 0 || !env.persistentIds.fetch) return nullptr;
    const void* p = nullptr;
    if (env.persistentIds.persistentLoadedPartIds.TryGetValue(id, p)) return const_cast<Part*>(static_cast<const Part*>(p));
    return nullptr;
}
}  // namespace

// ControlledBase.cs:66
ControlledBase::ControlledBase(Part* part, PartModule* module, ModuleRoboticController* controller) {
    partId = part->fields.persistentId;
    part_ = part;
    module_ = module;
    if (module != nullptr) {
        moduleId = module->GetPersistentId();
        persistentActionId = SerenityMiscEnvironment::persistentActionId ? SerenityMiscEnvironment::persistentActionId(*module) : 0u;
    }
    Controller = controller;
}

// ControlledBase.cs:34
uint32_t ControlledBase::PartPersistentId() const {
    if (part_ == nullptr) return 0u;
    return part_->fields.persistentId;
}

// ControlledBase.cs:46
std::vector<Part*>* ControlledBase::SymmetryParts() const {
    if (part_ == nullptr) return nullptr;
    return &part_->symmetryCounterparts;
}

// ControlledBase.cs:105
bool ControlledBase::AssignReferenceVars() {
    Part* partout = FindLoadedPart(partId);
    if (partout == nullptr) {
        log::Warning("[ModuleRoboticController]: Unable to find Controlled Part in vessel: " + std::to_string(partId));
        return false;
    }
    part_ = partout;
    if (moduleId != 0) module_ = partout->Modules().ByPersistentId(moduleId);
    if (partNickName.empty()) {
        if (part_->partInfo == nullptr) throw NullReferenceException();
        partNickName = part_->partInfo->title;
    }
    const bool result = OnAssignReferenceVars();
    RebuildSymmetryList();
    return result;
}

// ControlledBase.cs:123
void ControlledBase::RebuildSymmetryList(const std::vector<uint32_t>& excludedPartIds) {
    ClearSymmetryLists();
    std::vector<Part*>* sym = SymmetryParts();
    if (sym == nullptr) return;
    for (size_t i = 0; i < sym->size(); i++)
        if (std::find(excludedPartIds.begin(), excludedPartIds.end(), (*sym)[i]->fields.persistentId) == excludedPartIds.end()) AddSymmetryPart((*sym)[i]);
}

// ControlledBase.cs:140
void ControlledBase::ChangeSymmetryMaster(Part* newPart) {
    uint32_t oldPartId = 0;
    std::vector<Part*>* sym = SymmetryParts();
    if (sym == nullptr) throw NullReferenceException();
    if (std::find(sym->begin(), sym->end(), newPart) == sym->end()) {
        log::Error("[ControlledAxis]: Cannot change Symmetry Master to " + std::to_string(newPart->fields.persistentId) + " as its not listed in the SymmetryParts for " +
                   std::to_string(PartPersistentId()));
    } else if (OnChangeSymmetryMaster(newPart, oldPartId)) {
        part_ = newPart;
        partId = newPart->fields.persistentId;
        RebuildSymmetryList({oldPartId});
    }
}

// ControlledBase.cs:158
void ControlledBase::Load(const ConfigNode& node) {
    node.TryGetValue("persistentId", partId);
    node.TryGetValue("moduleId", moduleId);
    node.TryGetValue("partNickName", partNickName);
    node.TryGetValue("rowIndex", rowIndex);
    OnLoad(node);
}

// ControlledBase.cs:168
void ControlledBase::Save(ConfigNode& node) {
    node.AddValue("persistentId", partId);
    node.AddValue("moduleId", moduleId);
    node.AddValue("partNickName", partNickName);
    node.AddValue("rowIndex", rowIndex);
    std::vector<Part*>* sym = SymmetryParts();
    if (sym != nullptr && !sym->empty()) {
        ConfigNode* configNode = node.AddNode("SYMPARTS");
        for (size_t i = 0; i < sym->size(); i++) configNode->AddValue("symPersistentId", (*sym)[i]->fields.persistentId);
    }
    OnSave(node);
}

// RoboticCollisions.cs:10
void RoboticCollisions::Start() {
    if (!SaveEnvironment::Current().IsExpansionInstalled("Serenity") && SaveEnvironment::Current().loadedSceneIsGame) {
        behaviourEnabled = false;
        destroyed = true;
    }
}

namespace {
void Forward(RoboticCollisions& rc, const char* message) {
    if (rc.part == nullptr) return;
    SerenityMiscEnvironment::collisionMessages.push_back(message);
    if (SerenityMiscEnvironment::partCollision) SerenityMiscEnvironment::partCollision(*rc.part, message);
}
}  // namespace

void RoboticCollisions::OnCollisionEnter() { Forward(*this, "OnCollisionEnter"); }  // :18
void RoboticCollisions::OnCollisionExit() { Forward(*this, "OnCollisionExit"); }    // :26
void RoboticCollisions::OnTriggerEnter() { Forward(*this, "OnTriggerEnter"); }      // :34
void RoboticCollisions::OnTriggerExit() { Forward(*this, "OnTriggerExit"); }        // :42

namespace SerenityUtils {
std::string AssetPath(const std::string& prefabPath) { return "Assets/Expansions/Serenity/" + prefabPath; }
std::shared_ptr<void> SerenityTexture(const std::string& prefabPath) {
    return SerenityMiscEnvironment::loadAsset ? SerenityMiscEnvironment::loadAsset("serenity_assets", AssetPath(prefabPath)) : nullptr;
}
std::shared_ptr<void> SerenityPrefab(const std::string& prefabPath) {
    return SerenityMiscEnvironment::loadAsset ? SerenityMiscEnvironment::loadAsset("serenity_assets", AssetPath(prefabPath)) : nullptr;
}
}  // namespace SerenityUtils

}  // namespace ksp::Expansions::Serenity
