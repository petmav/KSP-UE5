// WP-26: CompoundPart.cs (scope and exclusions: include/ksp/parts/compound_part.hpp).
#include "ksp/parts/compound_part.hpp"

#include <algorithm>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/modules/part_structural_modules.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {
using GE = GameEvents;
bool SceneEditor() { return SaveEnvironment::Current().loadedScene == GameScenes::EDITOR; }
bool SceneFlight() { return SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT; }
std::vector<CompoundPartModule*> CmpModules(Part& p) {  // GetComponents<CompoundPartModule>()
    std::vector<CompoundPartModule*> r;
    for (size_t i = 0; i < p.Modules().Count(); i++)
        if (auto* m = dynamic_cast<CompoundPartModule*>(p.Modules().GetModule(i))) r.push_back(m);
    return r;
}
}  // namespace

CompoundPart::CompoundPart() { className = "CompoundPart"; }

CompoundPart::~CompoundPart() {
    if (subscribed_) GE::onEditorPartEvent.Remove(this, &CompoundPart::OnEditorEvent);
}

// CompoundPart.cs:128
void CompoundPart::onPartAwake() {
    hasSaveData_ = false;
    // cmpModules = GetComponents<CompoundPartModule>(): found on demand (CmpModules); the modules read `target` through compoundTarget
    for (CompoundPartModule* m : CmpModules(*this))
        m->compoundTarget = [](const Part& p) { return dynamic_cast<const CompoundPart&>(p).target; };
    if (!subscribed_) {
        GE::onEditorPartEvent.Add(this, &CompoundPart::OnEditorEvent);
        subscribed_ = true;
    }
    set_isCompund(true);
}

// CompoundPart.cs:114
void CompoundPart::onCopy(Part* original, bool asSymCPart) {
    hasSaveData_ = false;
    Part* selected = hooks.editorSelectedPart ? hooks.editorSelectedPart() : nullptr;
    if (std::find(symmetryCounterparts.begin(), symmetryCounterparts.end(), selected) != symmetryCounterparts.end()) {
        compoundFields.direction = Vector3f::zero();
    } else if (asSymCPart && hooks.editorSymmetryIsMirror && hooks.editorSymmetryIsMirror()) {
        original_ = static_cast<CompoundPart*>(original);
        needsDirectionFlip = true;
    }
}

// CompoundPart.cs:145
void CompoundPart::OnSave(ConfigNode& node) {
    node.AddValue("tgt", tgtId_);
    node.AddValue("tpersID", targetPersistentId_);
    node.AddValue("pos", ConfigNode::WriteVector(compoundFields.targetPosition));
    node.AddValue("rot", ConfigNode::WriteQuaternion(compoundFields.targetRotation));
    node.AddValue("dir", ConfigNode::WriteVector(compoundFields.direction));
    if (compoundFields.targetMeshColName && !compoundFields.targetMeshColName->empty()) node.AddValue("col", *compoundFields.targetMeshColName);
}

// CompoundPart.cs:158 (TryGetValue leaves the field unchanged when the key is absent or does not parse)
void CompoundPart::OnLoad(ConfigNode& node) {
    hasSaveData_ = node.HasData();
    node.TryGetValue("tgt", tgtId_);
    node.TryGetValue("tpersID", targetPersistentId_);
    node.TryGetValue("pos", compoundFields.targetPosition);
    node.TryGetValue("dir", compoundFields.direction);
    node.TryGetValue("rot", compoundFields.targetRotation);
    std::string col;
    if (node.TryGetValue("col", col)) compoundFields.targetMeshColName = col;
}

// CompoundPart.cs:234 (the editor / flight input locks are not modelled; the data half)
void CompoundPart::onPartAttach(Part*) {
    if (SceneEditor()) {
        Part* selected = hooks.editorSelectedPart ? hooks.editorSelectedPart() : nullptr;
        if (selected == this) {
            set_attachState(AttachState::Attaching);
        } else {
            set_attachState(AttachState::Detached);
            if (compoundFields.direction != Vector3f::zero()) {
                if (needsDirectionFlip && original_ != nullptr) {
                    Vector3f vector = original_->partTransform.TransformDirection(original_->compoundFields.direction);
                    vector = Vector3f(-vector.x, vector.y, vector.z);
                    compoundFields.direction = partTransform.InverseTransformDirection(vector);
                    needsDirectionFlip = false;
                }
                if (hooks.scheduleRaycast) hooks.scheduleRaycast(*this);
            }
        }
    }
}

// CompoundPart.cs:265
void CompoundPart::onPartDetach() {
    Part* selected = hooks.editorSelectedPart ? hooks.editorSelectedPart() : nullptr;
    if ((SceneEditor() && selected == this) || (target != nullptr && target->localRoot() != localRoot())) {
        DumpTarget();
        set_attachState(AttachState::Detached);
        if (selected == this) compoundFields.direction = Vector3f::zero();
    }
    if (SceneFlight()) DumpTarget();
}

// CompoundPart.cs:283
void CompoundPart::OnPartDestroy() {
    DumpTarget();
    if (subscribed_) {
        GE::onEditorPartEvent.Remove(this, &CompoundPart::OnEditorEvent);
        subscribed_ = false;
    }
}

// CompoundPart.cs:169
void CompoundPart::OnStartComplete() {
    if (fields.customPartData && !fields.customPartData->empty()) {
        ConfigNode upgraded = ParseCustomPartData(*fields.customPartData);
        OnLoad(upgraded);
        fields.customPartData = std::string("");
    }
    set_attachState(AttachState::Detached);
    target = nullptr;
    Part* part = nullptr;
    if (hasSaveData_) {
        if (SceneFlight() && vessel != nullptr) {
            for (size_t i = 0; i < vessel->parts.size(); i++) {
                Part* q = vessel->parts[i];
                if (q->fields.craftID != tgtId_ || q->fields.missionID != fields.missionID) {
                    if (targetPersistentId_ != 0 && q->fields.persistentId == targetPersistentId_) {
                        part = q;
                        break;
                    }
                    continue;
                }
                part = q;
                break;
            }
        }
        if (SceneEditor() && hooks.editorShipParts) {
            for (Part* q : hooks.editorShipParts())
                if (q->fields.craftID == tgtId_) {
                    part = q;
                    break;
                }
        }
        if (part != nullptr) {
            target = part;
            SetTarget(part, compoundFields.targetMeshColName.value_or(""));
            GE::onEditorCompoundPartLinked.Fire(this);
        } else if (SceneEditor()) {
            if (compoundFields.direction != Vector3f::zero()) {
                log::Warning("[CompoundPart]: No target found with craftID " + std::to_string(fields.craftID) + ". Attempting to find it at direction [" +
                             ConfigNode::WriteVector(compoundFields.direction) + "].");
                if (hooks.scheduleRaycast) hooks.scheduleRaycast(*this);
            }
        }
    }
}

// CompoundPart.cs:80 / :86 / :91
void CompoundPart::DisconnectEvent() { DisconnectCompoundPart(); }
void CompoundPart::DisconnectAction(KSPActionParam&) { DisconnectCompoundPart(); }
void CompoundPart::DisconnectCompoundPart() {
    // Effect(disconnectedEffectName, 1f) when attachState != Detached: the part effect system is the FX package's
    DumpTarget();
    set_attachState(AttachState::Detached);
    for (size_t count = symmetryCounterparts.size(); count-- > 0;) {
        Part* sc = symmetryCounterparts[count];
        if (sc->fields.persistentId != fields.persistentId && sc->isCompund()) {
            auto* compoundPart = dynamic_cast<CompoundPart*>(sc);
            if (compoundPart != nullptr && compoundPart->attachState() != AttachState::Detached) {
                compoundPart->DumpTarget();
                compoundPart->set_attachState(AttachState::Detached);
            }
        }
    }
}

// CompoundPart.cs:639
bool CompoundPart::SetTarget(Part* tgt, const std::string& tgtColName) {
    if (target != nullptr) DumpTarget();
    target = tgt;
    compoundFields.targetMeshColName = tgtColName;
    if (target != nullptr) {
        // target.frozen: a Part member of the EVA construction package (not modelled): never frozen here
        // the editor's OnEditorDetach / OnEditorDestroy / OnEditorAttach callbacks (onTargetDetach / Destroy / Reattach) are not wired:
        // the host calls OnTargetDetach / OnTargetDestroy
        tgtId_ = target->fields.craftID;
        targetPersistentId_ = target->fields.persistentId;
        UpdateWorldValues();
        SetLink();
        set_attachState(AttachState::Attached);
        if (SceneFlight() && target->fields.persistentId != fields.persistentId) GE::OnFlightCompoundPartLinked.Fire(this);
        return true;
    }
    return false;
}

// CompoundPart.cs:677 (Quaternion.Inverse is native: a plain conjugate, mathfx.hpp)
void CompoundPart::UpdateWorldValues() {
    wTgtPos_ = target->partTransform.InverseTransformPoint(partTransform.TransformPoint(compoundFields.targetPosition));
    wTgtRot_ = QuaternionExtensions::Inverse(target->partTransform.rotation) * partTransform.rotation * compoundFields.targetRotation;
}

// CompoundPart.cs:683
void CompoundPart::DumpTarget() {
    if (target != nullptr) {
        if (SceneFlight()) GE::OnFlightCompoundPartDetached.Fire(this);
    }
    // onEditorEndTweak(): the tweak mode is the editor's
    wTgtPos_ = Vector3f::zero();
    wTgtRot_ = Quaternionf::identity();
    UnsetLink();
    target = nullptr;
    tgtId_ = 0u;
    targetPersistentId_ = 0u;
    set_attachState(AttachState::Detached);
}

// CompoundPart.cs:708 / :717
void CompoundPart::SetLink() {
    auto mods = CmpModules(*this);
    for (size_t n = mods.size(); n-- > 0;) mods[n]->OnTargetSet(target);
}
void CompoundPart::UnsetLink() {
    auto mods = CmpModules(*this);
    for (size_t n = mods.size(); n-- > 0;) mods[n]->OnTargetLost();
}

// CompoundPart.cs:375 / :384
void CompoundPart::OnTargetDetach() {
    UnsetLink();
    set_attachState(AttachState::Detached);
}
void CompoundPart::OnTargetDestroy() {
    UnsetLink();
    set_attachState(AttachState::Detached);
}

// CompoundPart.cs:800 (`(uint)(evt - 10) <= 3u`: ConstructionEventType 10..13)
void CompoundPart::OnEditorEvent(ConstructionEventType evt, Part*) {
    const unsigned e = static_cast<unsigned>(static_cast<int>(evt)) - 10u;
    if (e <= 3u) UpdateTargetCoords();
}

// CompoundPart.cs:808
void CompoundPart::UpdateTargetCoords() {
    if (target == nullptr) return;
    compoundFields.targetPosition = partTransform.InverseTransformPoint(target->partTransform.TransformPoint(wTgtPos_));
    compoundFields.targetRotation = QuaternionExtensions::Inverse(partTransform.rotation) * target->partTransform.rotation * wTgtRot_;
    compoundFields.direction = compoundFields.targetPosition.normalized();
    if (attachState() == AttachState::Attached) {
        auto mods = CmpModules(*this);
        for (size_t n = mods.size(); n-- > 0;) mods[n]->OnTargetUpdate();
    }
}

// CompoundPart.cs:827
ConfigNode CompoundPart::ParseCustomPartData(const std::string& customPartData) {
    ConfigNode configNode;
    if (!customPartData.empty()) {
        log::Warning("[CompoundPart]: Deprecated 'customPartData' field found. Upgrading to new format...");
        for (const std::string& text : dotnet::Split(customPartData, ";", false)) {
            if (text.find(':') == std::string::npos) continue;
            const std::vector<std::string> kv = dotnet::Split(text, ":", false);
            const std::string text2 = dotnet::Trim(kv[0]);
            const std::string text3 = dotnet::Trim(kv[1]);
            if (text2 == "rot") configNode.AddValue("rot", text3);
            else if (text2 == "pos") configNode.AddValue("pos", text3);
            else if (text2 == "dir") configNode.AddValue("dir", text3);
            else if (text2 == "tgt") {
                if (text3.find('_') != std::string::npos) {
                    const int index = dotnet::ParseInt32(dotnet::Trim(dotnet::Split(text3, "_", false)[1]));
                    uint32_t value = 0u;
                    if (SceneEditor()) {
                        if (hooks.editorSortedShipList) value = hooks.editorSortedShipList().at(static_cast<size_t>(index))->fields.craftID;
                    } else if (SceneFlight()) {
                        value = vessel->parts.at(static_cast<size_t>(index))->fields.craftID;
                    }
                    configNode.AddValue("tgt", value);
                }
            }
        }
    }
    return configNode;
}

}  // namespace ksp
