// WP-40: EditorLogic.cs / EditorLogicBase.cs -- see include/ksp/editor/editor_logic.hpp.
#include "ksp/editor/editor_logic.hpp"
#include "ksp/dotnet/math.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/editor/editor_geometry.hpp"
#include "ksp/editor/editor_reroot.hpp"
#include "ksp/editor/preflight_tests.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

using QuaternionExtensions::Inverse;

namespace {

AttachRules RulesOf(const Part& p) { return p.fields.attachRules.value_or(AttachRules{}); }

template <class T>
bool Contains(const std::vector<T>& v, const T& x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}

// Unity's Transform.rotation / localRotation setter: NormalizeSafe (Runtime/Math/Quaternion.h): identity under 1e-5, else q / |q|
Quaternionf NormalizeSafe(const Quaternionf& q) {
    const float mag = UnityNative::SqrtF(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (mag < 0.00001f) return Quaternionf::identity();
    return {q.x / mag, q.y / mag, q.z / mag, q.w / mag};
}

}  // namespace

// ==================================================================================================================================
// EditorLogicBase
// ==================================================================================================================================

// EditorLogicBase.cs:292
std::vector<Part*> EditorLogicBase::FindPartsInChildren(Part* part) {
    std::vector<Part*> parts;
    FindPartsInChildren(parts, part);
    return parts;
}

// EditorLogicBase.cs:292
void EditorLogicBase::FindPartsInChildren(std::vector<Part*>& parts, Part* part) {
    parts.push_back(part);
    for (size_t i = 0, count = part->children.size(); i < count; i++) FindPartsInChildren(parts, part->children[i]);
}

// EditorLogicBase.cs:194
void EditorLogicBase::clearAttachNodes(Part* part, Part* otherPart) {
    if (otherPart == nullptr) return;
    for (size_t i = 0, count = otherPart->attachNodes.size(); i < count; i++) {
        AttachNode& attachNode = otherPart->attachNodes[i];
        if (!(attachNode.attachedPart != part)) {
            attachNode.attachedPart = nullptr;
            break;
        }
    }
    if (otherPart->srfAttachNode->attachedPart == part) otherPart->srfAttachNode->attachedPart = nullptr;
    for (size_t j = 0, count2 = part->attachNodes.size(); j < count2; j++) {
        AttachNode& attachNode = part->attachNodes[j];
        if (!(attachNode.attachedPart != otherPart)) {
            attachNode.attachedPart = nullptr;
            break;
        }
    }
    if (part->srfAttachNode->attachedPart == otherPart) part->srfAttachNode->attachedPart = nullptr;
}

// EditorLogicBase.cs:232
bool EditorLogicBase::PartInHierarchy(Part* part, Part* tgtPart) const {
    if (Contains(part->children, tgtPart)) return true;
    for (size_t num = 0, count = part->children.size(); num < count; num++)
        if (PartInHierarchy(part->children[num], tgtPart)) return true;
    return false;
}

// EditorLogicBase.cs:259
float EditorLogicBase::FindPartSurface(Part& p, const Vector3f& origin, const Vector3f& direction, const Vector3f& fromParent,
                                       const EditorSurfaceHit& hit, Vector3f& srfNormal) const {
    EditorEnvironment& env = EditorEnvironment::Current();
    if (env.findPartSurface) return env.findPartSurface(p, origin, direction, fromParent, hit, srfNormal);
    srfNormal = hit.normal;
    return 1.0f;
}

// EditorLogicBase.cs:75
void EditorLogicBase::partRotationInputUpdate(RotationKey key, bool inRotateTweakStateNow) {
    partTweaked = false;
    EditorEnvironment& env = EditorEnvironment::Current();
    PartFields& f = selectedPart->fields;
    const float step = env.fineTweak ? 5.0f : 90.0f;
    auto axisOf = [&](const Vector3f& dir) { return Inverse(f.initRotation) * dir; };
    if (!inRotateTweakStateNow) {
        switch (key) {
        case RotationKey::YawLeft:
            f.attRotation = UnityNative::AngleAxis(step, axisOf(Vector3f::forward())) * f.attRotation;
            partTweaked = true;
            break;
        case RotationKey::YawRight:
            f.attRotation = UnityNative::AngleAxis(-step, axisOf(Vector3f::forward())) * f.attRotation;
            partTweaked = true;
            break;
        case RotationKey::RollLeft:
            f.attRotation = UnityNative::AngleAxis(step, axisOf(Vector3f::up())) * f.attRotation;
            partTweaked = true;
            break;
        case RotationKey::RollRight:
            f.attRotation = UnityNative::AngleAxis(-step, axisOf(Vector3f::up())) * f.attRotation;
            partTweaked = true;
            break;
        case RotationKey::PitchUp:
            f.attRotation = UnityNative::AngleAxis(-step, axisOf(Vector3f::right())) * f.attRotation;
            partTweaked = true;
            break;
        case RotationKey::PitchDown:
            f.attRotation = UnityNative::AngleAxis(step, axisOf(Vector3f::right())) * f.attRotation;
            partTweaked = true;
            break;
        case RotationKey::None: break;
        }
    }
    partRotationResetUpdate();
    if (partTweaked) {
        GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartRotated, selectedPart);
        partTweaked = false;
    }
}

// EditorLogicBase.cs:126
void EditorLogicBase::displayAttachNodeIcons(const std::vector<Part*>& parts, Part* selected, bool stackNodes, bool srfNodes, bool dockNodes) {
    for (size_t i = 0, count = parts.size(); i < count; i++) handleAttachNodeIcons(parts[i], stackNodes, srfNodes, dockNodes);
    handleAttachNodeIcons(selected, stackNodes, srfNodes, dockNodes);
    handleChildNodeIcons(selected);
}

// EditorLogicBase.cs:137
void EditorLogicBase::handleChildNodeIcons(Part* part) {
    for (size_t i = 0, count = part->children.size(); i < count; i++) {
        Part* part2 = part->children[i];
        handleAttachNodeIcons(part2, false, false, false);
        handleChildNodeIcons(part2);
    }
}

// EditorLogicBase.cs:129
void EditorLogicBase::handleAttachNodeIcons(Part* part, bool stackNodes, bool srfNodes, bool dockNodes) {
    for (size_t i = 0, count = part->attachNodes.size(); i < count; i++) AssignAttachIcon(part, part->attachNodes[i], stackNodes, srfNodes, dockNodes);
}

// EditorLogicBase.cs:157 (the icon object itself is the shell's: iconNodes_ holds the nodes that have one, `onNodeIconPose` places it)
void EditorLogicBase::AssignAttachIcon(Part* part, AttachNode& node, bool stackNodes, bool srfNodes, bool dockNodes) {
    (void)srfNodes;
    if ((stackNodes || node.nodeType != AttachNodeType::Stack) && (dockNodes || node.nodeType != AttachNodeType::Dock) &&
        node.nodeType != AttachNodeType::Surface &&
        (!(node.attachedPart != nullptr) || EditorEnvironment::Current().allowPartClipping || !(part != nullptr))) {
        iconNodes_.insert(&node);   // node.icon = Instantiate(attachNodePrefab) when it is null
        if (onNodeIconPose) onNodeIconPose(*part, node);
    } else if (iconNodes_.count(&node) != 0) {
        iconNodes_.erase(&node);    // node.DestroyNodeIcon(); node.icon = null
    }
}

// ==================================================================================================================================
// EditorLogic: lifetime, transform helpers
// ==================================================================================================================================

EditorLogic::EditorLogic() { fetch = this; }

EditorLogic::~EditorLogic() {
    if (fetch == this) fetch = nullptr;
}

Vector3f EditorLogic::LocalPosition(const Part& p) {
    if (p.parent != nullptr) return p.parent->partTransform.InverseTransformPoint(p.partTransform.position);
    return p.partTransform.position;
}

Quaternionf EditorLogic::LocalRotation(const Part& p) {
    if (p.parent != nullptr) return Inverse(p.parent->partTransform.rotation) * p.partTransform.rotation;
    return p.partTransform.rotation;
}

void EditorLogic::SetLocalPosition(Part& p, const Vector3f& v) {
    p.partTransform.position = p.parent != nullptr ? p.parent->partTransform.TransformPoint(v) : v;
}

void EditorLogic::SetLocalRotation(Part& p, const Quaternionf& q) {
    p.partTransform.rotation = NormalizeSafe(p.parent != nullptr ? p.parent->partTransform.rotation * q : q);
}

void EditorLogic::SetRotation(Part& p, const Quaternionf& q) { p.partTransform.rotation = NormalizeSafe(q); }

PartTransform& EditorLogic::ReferenceTransform(Part& p) {
    EditorEnvironment& env = EditorEnvironment::Current();
    if (env.referenceTransform) {
        if (PartTransform* t = env.referenceTransform(p)) return *t;
    }
    return p.partTransform;
}

// Part.cs:1587 SetSymmetryValues
static void SetSymmetryValues(Part& p, const Vector3f& newPosition, const Quaternionf& newRotation) {
    p.partTransform.position = newPosition;
    EditorLogic::SetRotation(p, newRotation);
}

// ==================================================================================================================================
// symmetry mode / method
// ==================================================================================================================================

// EditorLogic.cs:1002
void EditorLogic::symButton(SymButton button) {
    if (button != SymButton::Left) {
        if (button == SymButton::Middle) {
            RestoreSymmetryState();
            symmetryMethod = symmetryMethod == SymmetryMethod::Mirror ? SymmetryMethod::Radial : SymmetryMethod::Mirror;
            GameEvents::onEditorSymmetryMethodChange.Fire(symmetryMethod);
        } else {
            prevSymMode();
        }
    } else {
        nextSymMode();
    }
}

// EditorLogic.cs:1040
void EditorLogic::nextSymMode() {
    if (symmetryMethod == SymmetryMethod::Radial) {
        symmetryMode++;
        if (symmetryMode == 4) symmetryMode = 5;
        if (symmetryMode == 6) symmetryMode = 7;
        if (symmetryMode >= 8) symmetryMode = 0;
    } else {
        symmetryMode++;
        if (symmetryMode >= 2) symmetryMode = 0;
    }
    GameEvents::onEditorSymmetryModeChange.Fire(symmetryMode);
    SetSymState();
}

// EditorLogic.cs:1070
void EditorLogic::prevSymMode() {
    if (symmetryMethod == SymmetryMethod::Radial) {
        symmetryMode--;
        if (symmetryMode == 4) symmetryMode = 3;
        if (symmetryMode == 6) symmetryMode = 5;
        if (symmetryMode <= -1) symmetryMode = 7;
    } else {
        symmetryMode--;
        if (symmetryMode <= -1) symmetryMode = 1;
    }
    GameEvents::onEditorSymmetryModeChange.Fire(symmetryMode);
    SetSymState();
}

// EditorLogic.cs:1100 (the symmetry / mirror sprite state is the shell's)
void EditorLogic::SetSymState() { RestoreSymmetryState(); }

// symInputUpdate (EditorLogic.cs:1133): the Editor_toggleSymMethod branch
void EditorLogic::ToggleSymMethod() {
    RestoreSymmetryState();
    symmetryMethod = symmetryMethod == SymmetryMethod::Mirror ? SymmetryMethod::Radial : SymmetryMethod::Mirror;
    GameEvents::onEditorSymmetryMethodChange.Fire(symmetryMethod);
}

// symInputUpdate (EditorLogic.cs:1133): the Editor_coordSystem branch
void EditorLogic::ToggleSymmetryCoordSpace() {
    RestoreSymmetryState();
    if (symmetryMethod == SymmetryMethod::Radial) {
        symmetryCoordSpace = symmetryCoordSpace == UnityEngine::Space::Self ? UnityEngine::Space::World : UnityEngine::Space::Self;
        GameEvents::onEditorSymmetryCoordsChange.Fire(symmetryCoordSpace);
    }
}

// EditorLogic.cs:1197
void EditorLogic::onSymMethodChange(SymmetryMethod method) {
    if (method == SymmetryMethod::Mirror) symmetryMode = static_cast<int>(Mathf::Clamp01(static_cast<float>(symmetryMode)));
    SetSymState();
}

// EditorLogic.cs:3140
void EditorLogic::RestoreSymmetryState() {
    if (tmpSymMethodInUse_) {
        tmpSymMethodInUse_ = false;
        symmetryMethod = symmetryMethodTmp;
        if (symmetryMethod == SymmetryMethod::Mirror) symmetryMode = std::min(1, symmetryModeTmp);
        else symmetryMode = symmetryModeTmp;
    }
}

// EditorLogic.cs:3157
void EditorLogic::RestoreSymmetryModeBeforeNodeAttachment() {
    symmetryMode = symmetryModeBeforeNodeAttachment;
    SetSymState();
    symmetryModeBeforeNodeAttachment = -1;
}

// ==================================================================================================================================
// symmetry maths
// ==================================================================================================================================

// EditorLogic.cs:3164
void EditorLogic::createSymmetry(int mode, Attachment& attach) {
    Part* part = selectedPart;
    if (!tmpSymMethodInUse_ &&
        (static_cast<int>(part->symmetryCounterparts.size()) != mode || static_cast<SymmetryMethod>(part->fields.symMethod) != symmetryMethod))
        deleteSymmetryParts();
    part->fields.symMethod = static_cast<int>(symmetryMethod);
    if (mode == 0) {
        if (attach.mode == AttachModes::SRF_ATTACH && symmetryModeBeforeNodeAttachment >= 0 && part->symmetryCounterparts.empty())
            RestoreSymmetryModeBeforeNodeAttachment();
        return;
    }
    if (part->potentialParent->symmetryCounterparts.empty()) {
        if (Vector3f(part->partTransform.position.x, 0.0f, part->partTransform.position.z)
                .Equals(Vector3f(part->potentialParent->partTransform.position.x, 0.0f, part->potentialParent->partTransform.position.z))) {
            deleteSymmetryParts();
            return;
        }
        if (attach.mode == AttachModes::STACK &&
            (part->potentialParent->fields.stackSymmetry != mode ||
             (EditorEnvironment::Current().editorFacility == EditorFacility::VAB && symmetryMethod == SymmetryMethod::Mirror))) {
            symmetryModeBeforeNodeAttachment = symmetryMode;
            symmetryMode = part->potentialParent->fields.stackSymmetry;
            SetSymState();
            return;
        }
    }
    if (part->symmetryCounterparts.empty()) {
        Part* part2 = nullptr;
        for (int i = 0; i < mode; i++) {
            part->OnWillBeCopied(true);
            part2 = DuplicatePart(selectedPart);
            part->OnWasCopied(part2, true);
            part->symmetryCounterparts.push_back(part2);
            part2->symmetryCounterparts.clear();
            part2->symmetryCounterparts.push_back(selectedPart);
            part2->fields.symMethod = static_cast<int>(symmetryMethod);
            part2->OnCopy(part, true);
            if (part->potentialParent->symmetryCounterparts.empty()) {
                part2->potentialParent = part->potentialParent;
                continue;
            }
            if (static_cast<int>(part->potentialParent->symmetryCounterparts.size()) == mode &&
                static_cast<SymmetryMethod>(part->potentialParent->fields.symMethod) == symmetryMethod) {
                part2->potentialParent = part->potentialParent->getSymmetryCounterPart(i + 1);
                continue;
            }
            symmetryMode = static_cast<int>(part->potentialParent->symmetryCounterparts.size());
            if (part->fields.symMethod != part->potentialParent->fields.symMethod) {
                RestoreSymmetryState();
                symmetryMethod = static_cast<SymmetryMethod>(part->potentialParent->fields.symMethod);
                GameEvents::onEditorSymmetryMethodChange.Fire(symmetryMethod);
            }
            SetSymState();
            return;
        }
        for (size_t j = 0, count = part->symmetryCounterparts.size(); j < count; j++) {
            Part* part3 = part->symmetryCounterparts[j];
            for (size_t k = 0, count2 = part->symmetryCounterparts.size(); k < count2; k++) {
                Part* part4 = part->symmetryCounterparts[k];
                if (!(part3 == part4) && !Contains(part3->symmetryCounterparts, part4)) part3->symmetryCounterparts.push_back(part4);
            }
        }
        for (size_t l = 0, count3 = part->symmetryCounterparts.size(); l < count3; l++) {
            Part* part4 = part->symmetryCounterparts[l];
            UpdatePartAndChildren(part, part4, symmetryMethod);
        }
    }
    UpdateSymmetry(part, symmetryMode, attach.potentialParent, attach.callerPartNode);
    if (static_cast<SymmetryMethod>(part->fields.symMethod) != symmetryMethod) {
        symmetryMethodTmp = symmetryMethod;
        symmetryModeTmp = symmetryMode;
        symmetryMethod = static_cast<SymmetryMethod>(part->fields.symMethod);
        tmpSymMethodInUse_ = true;
    }
    for (int num = 0; num < symmetryMode; num++)
        if (part->symmetryCounterparts[static_cast<size_t>(num)]->partTransform.position.Equals(part->partTransform.position)) {
            deleteSymmetryParts();
            return;
        }
}

// EditorLogic.cs:3276
void EditorLogic::UpdatePartAndChildren(Part* SourcePart, Part* UpdatePart, SymmetryMethod SymMethod) {
    if (SourcePart == nullptr || UpdatePart == nullptr || SourcePart == UpdatePart) return;
    if (!Contains(SourcePart->symmetryCounterparts, UpdatePart)) SourcePart->symmetryCounterparts.push_back(UpdatePart);
    for (size_t i = 0, count = SourcePart->symmetryCounterparts.size(); i < count; i++) {
        Part* part = SourcePart->symmetryCounterparts[i];
        part->symmetryCounterparts.clear();
        for (size_t j = i + 1; j < count; j++) part->symmetryCounterparts.push_back(SourcePart->symmetryCounterparts[j]);
        part->symmetryCounterparts.push_back(SourcePart);
        for (size_t k = 0; k < i; k++) part->symmetryCounterparts.push_back(SourcePart->symmetryCounterparts[k]);
    }
    if (static_cast<SymmetryMethod>(SourcePart->fields.symMethod) == SymmetryMethod::Radial && SourcePart->symmetryCounterparts.size() > 1)
        SymMethod = SymmetryMethod::Radial;
    SourcePart->fields.symMethod = static_cast<int>(SymMethod);
    UpdatePart->fields.symMethod = static_cast<int>(SymMethod);
    for (size_t l = 0, count2 = UpdatePart->children.size(); l < count2; l++)
        UpdatePartAndChildren(SourcePart->children[l], UpdatePart->children[l], SymMethod);
}

// EditorLogic.cs:3315
void EditorLogic::UpdateSymmetry(Part* selPart, int symMode, Part* partParent, AttachNode* selPartNode) {
    int num = 1;
    int count = static_cast<int>(selPart->symmetryCounterparts.size());
    while (count-- > 0)
        if (selPart->symmetryCounterparts[static_cast<size_t>(count)]->potentialParent == selPart->potentialParent) num++;
    for (int i = 0; i < symMode; i++) {
        Quaternionf quaternion{};
        Vector3f vector{};
        Quaternionf quaternion2{};
        PartTransform& referenceTransform = ReferenceTransform(*selPart);
        const int num2 = (i + 1) % num;
        Part* ci = selPart->symmetryCounterparts[static_cast<size_t>(i)];
        switch (static_cast<SymmetryMethod>(selPart->fields.symMethod)) {
        case SymmetryMethod::Mirror: {
            Part* part = FirstNonSymmetricalParentFrom(partParent);
            const Vector3f vector2 = referenceTransform.position - part->partTransform.position;
            const Vector3f vector3 = Vector3f::ProjectOnPlane(vector2, rootPart->partTransform.up());
            const Vector3f newPosition =
                part->partTransform.position + (vector2 - vector3) + UnityNative::AngleAxis(180.0f, -rootPart->partTransform.forward()) * vector3;
            Vector3f direction = rootPart->partTransform.InverseTransformDirection(referenceTransform.up());
            Vector3f direction2 = rootPart->partTransform.InverseTransformDirection(referenceTransform.forward());
            direction.x *= -1.0f;
            direction2.x *= -1.0f;
            direction = rootPart->partTransform.TransformDirection(direction);
            direction2 = rootPart->partTransform.TransformDirection(direction2);
            quaternion = UnityNative::LookRotation(direction2, direction);
            const bool flag = ci->OnWillBeMirrored(quaternion, selPartNode, partParent);
            if (selPartNode != nullptr && selPartNode->nodeType == AttachNodeType::Surface && !flag && selPartNode->orientation.x != 0.0f)
                quaternion = UnityNative::AngleAxis(180.0f, direction) * quaternion;
            SetSymmetryValues(*ci, newPosition, quaternion);
            UpdateChildMirrorSymmetry(selPart, part, i);
            break;
        }
        case SymmetryMethod::Radial:
            if (ci->potentialParent == selPart->potentialParent) {
                const float angle = f32store(static_cast<double>(num2) * 360.0 / static_cast<double>(num));
                if (symmetryCoordSpace == UnityEngine::Space::World)
                    quaternion = UnityNative::AngleAxis(angle, rootPart->partTransform.up());
                else if (symmetryCoordSpace == UnityEngine::Space::Self)
                    quaternion = UnityNative::AngleAxis(angle, selPart->potentialParent->partTransform.up());
                else
                    quaternion = UnityNative::AngleAxis(angle, rootPart->partTransform.up());
                vector = quaternion * (referenceTransform.position - selPart->potentialParent->partTransform.position) +
                         ci->potentialParent->partTransform.position;
                quaternion2 = quaternion * referenceTransform.rotation;
                SetSymmetryValues(*ci, vector, quaternion2);
            } else if (ci->potentialParent != nullptr) {
                if (num2 == 0) {
                    vector = ci->potentialParent->partTransform.TransformPoint(
                        selPart->potentialParent->partTransform.InverseTransformPoint(referenceTransform.position));
                    quaternion2 = ci->potentialParent->partTransform.rotation *
                                  (Inverse(ReferenceTransform(*selPart->potentialParent).rotation) * referenceTransform.rotation);
                    SetSymmetryValues(*ci, vector, quaternion2);
                } else {
                    Part* prev = selPart->symmetryCounterparts[static_cast<size_t>(num2 - 1)];
                    vector = ci->potentialParent->partTransform.TransformPoint(
                        prev->potentialParent->partTransform.InverseTransformPoint(prev->partTransform.position));
                    quaternion2 = ci->potentialParent->partTransform.rotation *
                                  (Inverse(prev->potentialParent->partTransform.rotation) * prev->partTransform.rotation);
                    SetSymmetryValues(*ci, vector, quaternion2);
                }
            }
            break;
        }
        if (static_cast<AttachModes>(selPart->fields.attachMode) == AttachModes::SRF_ATTACH)
            ci->srfAttachNode->srfAttachMeshName = selPart->srfAttachNode->srfAttachMeshName;
    }
}

// EditorLogic.cs:3391
void EditorLogic::UpdateChildMirrorSymmetry(Part* part, Part* mirrorRoot, int symIdx) {
    for (size_t i = 0, count = part->children.size(); i < count; i++) {
        Part* part2 = part->children[i];
        if (static_cast<int>(part2->symmetryCounterparts.size()) > symIdx && part2->symmetryCounterparts[static_cast<size_t>(symIdx)] != nullptr &&
            !PartInHierarchy(selectedPart, part2->symmetryCounterparts[static_cast<size_t>(symIdx)])) {
            const Vector3f vector = part2->partTransform.position - mirrorRoot->partTransform.position;
            const Vector3f vector2 = Vector3f::ProjectOnPlane(vector, rootPart->partTransform.up());
            const Vector3f position =
                mirrorRoot->partTransform.position + (vector - vector2) + UnityNative::AngleAxis(180.0f, -rootPart->partTransform.forward()) * vector2;
            Vector3f direction = rootPart->partTransform.InverseTransformDirection(part2->partTransform.up());
            Vector3f direction2 = rootPart->partTransform.InverseTransformDirection(part2->partTransform.forward());
            direction.x *= -1.0f;
            direction2.x *= -1.0f;
            direction = rootPart->partTransform.TransformDirection(direction);
            direction2 = rootPart->partTransform.TransformDirection(direction2);
            Quaternionf rotation = UnityNative::LookRotation(direction2, direction);
            AttachNode* attachNode = part2->FindAttachNodeByPart(part);
            const bool flag = part2->OnWillBeMirrored(rotation, attachNode, part);
            if (attachNode != nullptr && !flag && attachNode->orientation.x != 0.0f) rotation = UnityNative::AngleAxis(180.0f, direction) * rotation;
            Part* counterpart = part2->symmetryCounterparts[static_cast<size_t>(symIdx)];
            counterpart->partTransform.position = position;
            SetRotation(*counterpart, rotation);
            UpdateChildMirrorSymmetry(part2, mirrorRoot, symIdx);
        }
    }
}

// EditorLogic.cs:3422
void EditorLogic::cleanSymmetry() {
    for (size_t i = 0, count = ship->liveParts.size(); i < count; i++) {
        Part* part = ship->liveParts[i];
        for (size_t j = 0, num = part->symmetryCounterparts.size(); j < num; j++) {
            if (IsDestroyed(part->symmetryCounterparts[j])) {
                part->symmetryCounterparts.erase(part->symmetryCounterparts.begin() + static_cast<std::ptrdiff_t>(j));
                j--;
                num--;
            }
        }
    }
}

// EditorLogic.cs:3443
void EditorLogic::CheckCopiedSymmetryPersistentId(Part* selected) {
    if (selected == nullptr) return;
    SaveEnvironment& env = SaveEnvironment::Current();
    std::vector<Part*> list = EditorLogicBase::FindPartsInChildren(selected);
    for (Part* p : list) p->fields.persistentId = env.persistentIds.CheckPartpersistentId(p->fields.persistentId, p, false, true);
    for (size_t j = 0; j < selected->symmetryCounterparts.size(); j++) {
        std::vector<Part*> list2 = EditorLogicBase::FindPartsInChildren(selected->symmetryCounterparts[j]);
        for (Part* p : list2) p->fields.persistentId = env.persistentIds.CheckPartpersistentId(p->fields.persistentId, p, false, true);
    }
}

// EditorLogic.cs:3464
void EditorLogic::wipeSymmetry(Part* selPart) {
    selPart->symmetryCounterparts.clear();
    std::vector<Part*> list = EditorLogicBase::FindPartsInChildren(selPart);
    std::unordered_set<Part*> hashSet(list.begin(), list.end());
    for (size_t i = 0, count = list.size(); i < count; i++) {
        Part* part = list[i];
        for (size_t j = 0, num = part->symmetryCounterparts.size(); j < num; j++) {
            if (hashSet.count(part->symmetryCounterparts[j]) == 0) {
                part->symmetryCounterparts.erase(part->symmetryCounterparts.begin() + static_cast<std::ptrdiff_t>(j));
                j--;
                num--;
            }
        }
    }
}

// EditorLogic.cs:3487
void EditorLogic::deleteSymmetryParts() {
    RestoreSymmetryState();
    if (selectedPart == nullptr) return;
    for (size_t i = 0, count = selectedPart->symmetryCounterparts.size(); i < count; i++) {
        Part* part = selectedPart->symmetryCounterparts[i];
        if (part != nullptr && selectedPart != part) {
            GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartSymmetryDeleted, part);
            detachPart(part);
            deletePart(part, true);
        }
    }
    wipeSymmetry(selectedPart);
}

// EditorLogic.cs:3509
void EditorLogic::deletePartAndSymmetryParts(Part* part, bool symmetry) {
    (void)symmetry;
    if (part != selectedPart) displayAttachNodeIcons(ship->liveParts, selectedPart, false, false, false);
    deleteSymmetryParts();
    deletePart(part, false, true);
}

// EditorLogic.cs:3519
Part* EditorLogic::FirstNonSymmetricalParentFrom(Part* p) {
    if (!p->symmetryCounterparts.empty()) return FirstNonSymmetricalParentFrom(p->parent);
    return p;
}

// ==================================================================================================================================
// attachment search
// ==================================================================================================================================

// EditorLogic.cs:3528
Attachment EditorLogic::checkAttach(Part* selPart) {
    EditorEnvironment& env = EditorEnvironment::Current();
    Attachment attachment;
    attachment.caller = selPart;
    attachment.position = selPart->partTransform.position;
    attachment.rotation = vesselRotation;
    attachment.potentialParent = nullptr;
    bool flag = false;
    bool flag2 = false;
    const AttachRules selRules = RulesOf(*selPart);
    if (selRules.stack && allowNodeAttachment) {
        for (size_t i = 0, count = ship->liveParts.size(); i < count; i++) {
            Part* part = ship->liveParts[i];
            const AttachRules rules = RulesOf(*part);
            if (part == selPart || PartInHierarchy(selPart, part) || (!rules.allowStack && !rules.allowDock)) continue;
            AttachNode* attachNode = nullptr;
            AttachNode* attachNode2 = nullptr;
            for (size_t j = 0, count2 = selPart->attachNodes.size(); j < count2; j++) {
                AttachNode* attachNode3 = &selPart->attachNodes[j];
                if (attachNode3->attachedPart != nullptr || !HasNodeIcon(attachNode3)) continue;
                const Vector3f normalized = selPart->partTransform.TransformDirection(attachNode3->orientation).normalized();
                for (size_t k = 0, count3 = part->attachNodes.size(); k < count3; k++) {
                    AttachNode* attachNode4 = &part->attachNodes[k];
                    if (((!HasNodeIcon(attachNode4) || attachNode4->attachedPart != nullptr) && !env.allowPartClipping) ||
                        attachNode3->nodeType != attachNode4->nodeType ||
                        !(env.nodeIconUnderCursor &&
                          env.nodeIconUnderCursor(*part, *attachNode4, selPart->partTransform.TransformPoint(attachNode3->position))))
                        continue;
                    const Vector3f normalized2 = part->partTransform.TransformDirection(attachNode4->orientation).normalized();
                    const float num = Vector3f::Dot(normalized, normalized2);
                    if (!env.nonStrictAttachmentOrientation) {
                        if (num < -0.7f) {
                            attachNode2 = attachNode4;
                            attachNode = attachNode3;
                            attachment.potentialParent = part;
                            stackRotation = UnityNative::FromToRotation(normalized, -normalized2);
                            attachment.rotation = stackRotation * vesselRotation;
                            attachment.position = part->partTransform.TransformPoint(attachNode2->position + attachNode2->offset) -
                                                  attachment.rotation * selPart->fields.attRotation * attachNode->position;
                            break;
                        }
                    } else if (!(Mathf::Abs(num) <= 0.7f)) {
                        attachNode2 = attachNode4;
                        attachNode = attachNode3;
                        attachment.potentialParent = part;
                        stackRotation = UnityNative::FromToRotation(normalized * Mathf::Sign(num), normalized2);
                        attachment.rotation = stackRotation * vesselRotation;
                        attachment.position = part->partTransform.TransformPoint(attachNode2->position + attachNode2->offset) -
                                              attachment.rotation * selPart->fields.attRotation * attachNode->position;
                        break;
                    }
                }
            }
            if (attachNode != nullptr && attachNode2 != nullptr) {
                flag = true;
                attachment.callerPartNode = attachNode;
                attachment.otherPartNode = attachNode2;
                selPart->fields.attachMode = static_cast<int>(AttachModes::STACK);
            }
        }
    }
    if ((selRules.srfAttach && allowSrfAttachment && !env.modifierKey) || (env.modifierKey && isCurrentPartFlag)) {
        std::optional<EditorSurfaceHit> hitOpt = env.surfaceRaycast ? env.surfaceRaycast() : std::nullopt;
        if (hitOpt) {
            EditorSurfaceHit hit = *hitOpt;
            Part* part2 = hit.part;
            if (!isCurrentPartFlag && hit.fairingPanelCollider) {
                attachment.possible = false;
                return attachment;
            }
            if (part2 != nullptr) {
                if (!ship->Contains(part2)) part2 = nullptr;
                if (hit.noAttachTag) part2 = nullptr;
                if (part2 != nullptr && RulesOf(*part2).allowSrfAttach) {
                    attachment.callerPartNode = selPart->srfAttachNode.get();
                    attachment.otherPartNode = nullptr;
                    selPart->fields.attachMode = static_cast<int>(AttachModes::SRF_ATTACH);
                    selPart->srfAttachNode->srfAttachMeshName = hit.colliderName;
                    attachment.potentialParent = part2;
                    flag2 = true;
                    attachment.rotation = UnityNative::LookRotation(hit.normal, part2->partTransform.rotation * Vector3f::up()) *
                                          UnityNative::LookRotation(selPart->srfAttachNode->orientation, Vector3f::up());
                    attachment.position = hit.point - attachment.rotation * selPart->fields.attRotation * selPart->srfAttachNode->position;
                    if (env.useAngleSnap) {
                        Vector3f vector = hit.point - part2->partTransform.position;
                        const Vector3f rayOrigin = part2->partTransform.position;
                        const Vector3f rayDirection = part2->partTransform.up();   // new Ray(origin, direction): the direction is normalised
                        const Vector3f rayDir = Vector3f::Normalize(rayDirection);
                        const Vector3f vector2 = Vector3f::ProjectOnPlane(vector, rayDir);
                        const Vector3f forward = part2->partTransform.forward();
                        const float num2 = Vector3f::Dot(vector, rayDir);
                        const float num3 = KSPUtil::BearingDegrees(forward, vector2.normalized(), rayDir);
                        const float num4 = env.fineTweak ? srfAttachAngleSnapFine : srfAttachAngleSnap;
                        const float q = f32store(static_cast<double>(num3) / static_cast<double>(num4));
                        float angle;
                        if (!(static_cast<double>(q) - static_cast<double>(Mathf::Floor(q)) > 0.5))
                            angle = f32store(static_cast<double>(Mathf::FloorToInt(q)) * static_cast<double>(num4));
                        else
                            angle = f32store(static_cast<double>(Mathf::CeilToInt(q)) * static_cast<double>(num4));
                        const Vector3f vector3 = UnityNative::AngleAxis(angle, rayDir) * forward;
                        Vector3f vector4 = rayOrigin + rayDir * num2;
                        vector = Vector3f::Normalize(vector);
                        Vector3f srfNormal{};
                        vector4 += vector3 * FindPartSurface(*part2, rayOrigin, rayDir, vector, hit, srfNormal);
                        if (Mathf::Abs(Vector3f::Dot(srfNormal, rayDir)) < 1.0f)
                            srfNormal = UnityNative::FromToRotation(Vector3f::ProjectOnPlane(srfNormal, rayDir).normalized(), vector3) * srfNormal;
                        attachment.rotation = UnityNative::LookRotation(srfNormal, part2->partTransform.rotation * Vector3f::up()) *
                                              UnityNative::LookRotation(selPart->srfAttachNode->orientation, Vector3f::up());
                        attachment.position = vector4 - attachment.rotation * selPart->fields.attRotation * selPart->srfAttachNode->position;
                    }
                }
            }
        }
    }
    attachment.mode = static_cast<AttachModes>(selPart->fields.attachMode);
    attachment.collision = false;
    attachment.possible = (flag2 || flag) && !attachment.collision;
    return attachment;
}

// EditorLogic.cs:3686
bool EditorLogic::checkEditorCollision(Attachment& attach) {
    EditorEnvironment& env = EditorEnvironment::Current();
    if (env.allowPartClipping) return false;
    auto collisionOf = [&](const Part* p) -> Part* {
        auto it = env.editorCollision.find(p);
        return it == env.editorCollision.end() ? nullptr : it->second;
    };
    if (attach.potentialParent != nullptr) {
        if (collisionOf(attach.caller) != nullptr) {
            if (collisionOf(attach.caller) == attach.potentialParent) env.editorCollision[attach.caller] = nullptr;
            if (Contains(attach.caller->symmetryCounterparts, collisionOf(attach.caller))) env.editorCollision[attach.caller] = nullptr;
        }
        if (!RulesOf(*attach.caller).allowCollision && !RulesOf(*attach.potentialParent).allowCollision && collisionOf(attach.caller) != nullptr)
            return !PartInHierarchy(attach.caller, collisionOf(attach.caller));
        return false;
    }
    if (!RulesOf(*attach.caller).allowCollision && collisionOf(attach.caller) != nullptr)
        return !PartInHierarchy(attach.caller, collisionOf(attach.caller));
    return false;
}

// EditorLogic.cs:3718
std::vector<Attachment> EditorLogic::CheckSymPartsAttach(const Attachment& oAttach) {
    std::vector<Attachment> array(selectedPart->symmetryCounterparts.size());
    for (size_t i = 0, count = selectedPart->symmetryCounterparts.size(); i < count; i++) {
        Part* part = selectedPart->symmetryCounterparts[i];
        Attachment attachment;
        switch (oAttach.mode) {
        case AttachModes::SRF_ATTACH:
            attachment.caller = part;
            attachment.position = part->partTransform.position;
            attachment.rotation = part->partTransform.rotation;
            attachment.callerPartNode = part->srfAttachNode.get();
            attachment.potentialParent = part->potentialParent;
            attachment.collision = false;
            attachment.possible = true;
            highlightSelected(attachment.possible, part);
            array[i] = attachment;
            break;
        case AttachModes::STACK:
            attachment.caller = part;
            attachment.collision = false;
            if (oAttach.callerPartNode != nullptr) {
                attachment.callerPartNode = part->FindAttachNode(oAttach.callerPartNode->id);
                AttachNode* attachNode = nullptr;
                float num = 3.40282347E+38f;
                for (size_t j = 0, count2 = ship->liveParts.size(); j < count2; j++) {
                    Part* part2 = ship->liveParts[j];
                    if (!(oAttach.potentialParent == part2) && !Contains(oAttach.potentialParent->symmetryCounterparts, part2)) continue;
                    for (size_t k = 0, count3 = part2->attachNodes.size(); k < count3; k++) {
                        AttachNode* attachNode2 = &part2->attachNodes[k];
                        const float sqrMagnitude =
                            (part->partTransform.TransformPoint(oAttach.callerPartNode->position) - part2->partTransform.TransformPoint(attachNode2->position))
                                .sqrMagnitude();
                        if (sqrMagnitude < num) {
                            attachNode = attachNode2;
                            num = sqrMagnitude;
                            attachment.potentialParent = part2;
                        }
                    }
                }
                AttachNode* attachNode3 = attachment.callerPartNode;
                num = 3.40282347E+38f;
                for (size_t l = 0, count4 = part->attachNodes.size(); l < count4; l++) {
                    AttachNode* attachNode2 = &part->attachNodes[l];
                    const float sqrMagnitude2 = (attachment.potentialParent->partTransform.TransformPoint(attachNode->position) -
                                                 part->partTransform.TransformPoint(attachNode2->position))
                                                    .sqrMagnitude();
                    if (sqrMagnitude2 < num) {
                        num = sqrMagnitude2;
                        attachNode3 = attachNode2;
                    }
                }
                attachment.callerPartNode = attachNode3;
                if (attachNode != nullptr) {
                    attachment.otherPartNode = attachNode;
                    attachment.position =
                        attachment.potentialParent->partTransform.TransformPoint(attachNode->position) - part->partTransform.rotation * attachNode3->position;
                }
                attachment.possible = attachNode != nullptr && !attachment.collision && attachNode3 != nullptr;
                highlightSelected(attachment.possible, part);
                if (attachment.possible) part->partTransform.position = attachment.position;
                array[i] = attachment;
            } else {
                attachment.callerPartNode = nullptr;
                attachment.possible = false;
                attachment.position = part->partTransform.position;
                attachment.rotation = part->partTransform.rotation;
                attachment.potentialParent = nullptr;
                array[i] = attachment;
            }
            break;
        }
    }
    if (oAttach.mode == AttachModes::STACK) {
        for (size_t m = 0, num2 = array.size(); m < num2; m++) {
            Attachment& attachment2 = array[m];
            for (size_t n = 0, num3 = array.size(); n < num3; n++) {
                const Attachment& attachment3 = array[n];
                if (!(attachment2.caller == attachment3.caller) && !(attachment2.potentialParent != attachment3.potentialParent) &&
                    attachment2.otherPartNode == attachment3.otherPartNode)
                    attachment2.possible = false;
            }
            if (!(attachment2.potentialParent != oAttach.potentialParent) && attachment2.otherPartNode == oAttach.otherPartNode) attachment2.possible = false;
        }
    }
    return array;
}

// EditorLogic.cs:3835
void EditorLogic::attachPart(Part* part, Attachment& attach) {
    if (part == rootPart) return;
    part->set_isAttached(true);
    addToShip(part);
    highlightSelected(true, part, true);
    if (attach.potentialParent != nullptr) part->setParent(attach.potentialParent);   // part.transform.parent = potentialParent.transform
    if (attach.callerPartNode != nullptr) attach.callerPartNode->attachedPart = attach.potentialParent;
    if (attach.otherPartNode != nullptr) attach.otherPartNode->attachedPart = part;
    part->fields.attPos0 = LocalPosition(*part);
    part->fields.attRotation0 = LocalRotation(*part);
    part->onAttach(attach.potentialParent);
    // part.Modules[i].enabled = true: PartModule.enabled is the engine's MonoBehaviour flag
    if (part->isCompund()) selectedCompoundPart = static_cast<CompoundPart*>(part);
}

// EditorLogic.cs:3874
void EditorLogic::detachPart(Part* part) {
    if (!(part == rootPart)) {
        if (ship->Contains(part)) removeFromShip(part);
        EditorLogicBase::clearAttachNodes(part, part->parent);
        part->setParent();
        // part.transform.parent = null: the world pose is kept
        part->onDetach();
    }
}

// EditorLogic.cs:3889
void EditorLogic::CopyActionGroups(Part* sPart, Part* cPart) {
    if (sPart == nullptr || cPart == nullptr || sPart == cPart) return;
    int count = static_cast<int>(cPart->Modules().Count());
    while (count-- > 0) {
        BaseActionList& sa = sPart->Modules().GetModule(static_cast<size_t>(count))->Actions();
        BaseActionList& ca = cPart->Modules().GetModule(static_cast<size_t>(count))->Actions();
        if (sa.Count() == ca.Count())
            for (size_t i = 0; i < ca.Count(); i++) ca[i]->actionGroup = sa[i]->actionGroup;
    }
    int count2 = static_cast<int>(sPart->children.size());
    while (count2-- > 0) CopyActionGroups(sPart->children[static_cast<size_t>(count2)], cPart->children[static_cast<size_t>(count2)]);
}

// EditorLogic.cs:3913
bool EditorLogic::attachSymParts(std::vector<Attachment>& cAttaches) {
    for (size_t i = 0, count = selectedPart->symmetryCounterparts.size(); i < count; i++) {
        Part* part = selectedPart->symmetryCounterparts[i];
        attachPart(part, cAttaches[i]);
    }
    return true;
}

// EditorLogic.cs:3925
void EditorLogic::addToShip(Part* part) {
    ship->Add(part);
    std::vector<Part*> list = EditorLogicBase::FindPartsInChildren(part);
    for (size_t i = 0, count = list.size(); i < count; i++) {
        Part* part2 = list[i];
        if (part2 != part) ship->Add(part2);
    }
}

// EditorLogic.cs:3949
void EditorLogic::removeFromShip(Part* part) {
    ship->Remove(part);
    std::vector<Part*> list = EditorLogicBase::FindPartsInChildren(part);
    for (size_t i = 0, count = list.size(); i < count; i++) ship->Remove(list[i]);
}

// EditorLogic.cs:3968 (highlighting is the shell's)
void EditorLogic::highlightSelected(bool attach, Part* part, bool reset) {
    EditorEnvironment& env = EditorEnvironment::Current();
    if (env.onHighlightSelected) env.onHighlightSelected(*part, attach, reset);
}

// ==================================================================================================================================
// create / duplicate / delete
// ==================================================================================================================================

void EditorLogic::RegisterEditorPart(std::unique_ptr<Part> p) { editorParts_.push_back(std::move(p)); }

// UnityEngine.Object.Instantiate(part) of a part with its transform children (the child parts of the hierarchy are cloned and re-linked;
// references to parts inside the cloned hierarchy are remapped to the clones, as Unity does)
Part* EditorLogic::InstantiateCopy(Part* p) {
    std::vector<Part*> orig = EditorLogicBase::FindPartsInChildren(p);
    std::map<const Part*, Part*> map;
    std::vector<Part*> clones;
    for (Part* o : orig) {
        std::unique_ptr<Part> c = Part::Instantiate(*o);
        c->partInfo = o->partInfo;
        c->gameObjectName = o->gameObjectName;
        c->partTransform = o->partTransform;
        c->fields.craftID = o->fields.craftID;
        c->partActionsNode = o->partActionsNode;
        map[o] = c.get();
        clones.push_back(c.get());
        RegisterEditorPart(std::move(c));
    }
    for (size_t i = 0; i < orig.size(); i++) {
        Part* o = orig[i];
        Part* c = clones[i];
        if (o != p && o->parent != nullptr && map.count(o->parent) != 0) c->setParent(map[o->parent]);
        c->symmetryCounterparts.clear();
        for (Part* s : o->symmetryCounterparts) c->symmetryCounterparts.push_back(map.count(s) != 0 ? map[s] : s);
        for (size_t k = 0; k < o->attachNodes.size() && k < c->attachNodes.size(); k++) {
            Part* t = o->attachNodes[k].attachedPart;
            c->attachNodes[k].attachedPart = t != nullptr && map.count(t) != 0 ? map[t] : t;
            c->attachNodes[k].owner = c;
        }
        if (o->srfAttachNode && c->srfAttachNode) {
            Part* t = o->srfAttachNode->attachedPart;
            c->srfAttachNode->attachedPart = t != nullptr && map.count(t) != 0 ? map[t] : t;
            c->srfAttachNode->owner = c;
        }
    }
    return clones[0];
}

// EditorLogic.cs:3028
Part* EditorLogic::DuplicatePart(Part* p) {
    Part* part = InstantiateCopy(p);
    part->gameObjectName = p->partInfo->name;   // part.name = p.partInfo.name
    part->fields.persistentId = SaveEnvironment::Current().persistentIds.GetUniquepersistentId();
    // if (FlightGlobals.fetch) PersistentLoadedPartIds.Add: no FlightGlobals in the editor scene
    part->InitializeModules();
    for (size_t i = 0, count = p->Modules().Count(); i < count; i++) {
        // `part.Actions[baseAction.name].actionGroup = baseAction.actionGroup` is the Part's own action list (the node is kept as loaded);
        // the modules' action groups are copied by Part.OnCopy -> PartModule.Copy
    }
    return part;
}

// EditorLogic.cs:2866
Part* EditorLogic::OnPodSpawn(const AvailablePart& pod) {
    std::unique_ptr<Part> up = Part::Instantiate(*pod.partPrefab);
    rootPart = up.get();
    rootPart->gameObjectName = pod.name;
    rootPart->partInfo = &pod;
    rootPart->fields.persistentId = SaveEnvironment::Current().persistentIds.CheckPartpersistentId(rootPart->fields.persistentId, rootPart, false, true);
    // variants.SetVariant, ModuleInventoryPart.SetInventoryDefaults and PartStatsUpgradeModule are module-side editor steps (WP-35 / WP-36)
    RegisterEditorPart(std::move(up));
    GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartCreated, rootPart);
    return rootPart;
}

// EditorLogic.cs:2906
Part* EditorLogic::SpawnPart(const AvailablePart& partInfo) {
    if (ship->liveParts.empty()) return OnPodSpawn(partInfo);
    std::unique_ptr<Part> up = Part::Instantiate(*partInfo.partPrefab);
    Part* part = up.get();
    part->gameObjectName = partInfo.name;
    part->fields.persistentId = SaveEnvironment::Current().persistentIds.CheckPartpersistentId(part->fields.persistentId, part, false, true);
    part->InitializeModules();
    RegisterEditorPart(std::move(up));
    selectedPart = part;
    SetRotation(*selectedPart, rootPart != nullptr ? rootPart->partTransform.rotation : selectedPart->fields.initRotation);
    // dragOverPlane(out position) -> selectedPart.transform.position = position: the cursor (UpdatePlacement)
    selPartGrabOffset = Vector3f::zero();
    selectedPart->fields.attRotation = Quaternionf::identity();
    selectedPart->fields.attRotation0 = LocalRotation(*selectedPart);
    selectedPart->fields.attPos0 = LocalPosition(*selectedPart);
    selectedPart->partInfo = &partInfo;
    return part;
}

// EditorLogic.cs:2946
void EditorLogic::SetIconAsPart(Part* part) {
    selectedPart = part;
    selPartGrabOffset = Vector3f::zero();
    dragPlaneCenter = Vector3f::zero();
    SetRotation(*selectedPart, rootPart != nullptr ? rootPart->partTransform.rotation : selectedPart->fields.initRotation);
    selectedPart->partTransform.localScale = Vector3f::one();
    selectedPart->fields.attRotation = Quaternionf::identity();
    selectedPart->fields.attRotation0 = LocalRotation(*selectedPart);
    selectedPart->fields.attPos0 = LocalPosition(*selectedPart);
}

// EditorLogic.cs:2996
void EditorLogic::SpawnConstruct(ShipConstruct* construct) {
    if (construct == nullptr) throw ArgumentNullException("construct");
    if (ship->liveParts.empty()) {
        rootPart = construct->liveParts[0]->localRoot();
        return;
    }
    ShipConstruction::SanitizeCraftIDs(construct->liveParts, false);
    selectedPart = construct->liveParts[0]->localRoot();
    SetRotation(*selectedPart, rootPart != nullptr ? rootPart->partTransform.rotation : selectedPart->fields.initRotation);
    selPartGrabOffset = Vector3f::zero();
    selectedPart->fields.attRotation = Quaternionf::identity();
    selectedPart->fields.attRotation0 = LocalRotation(*selectedPart);
    selectedPart->fields.attPos0 = LocalPosition(*selectedPart);
    GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartCreated, selectedPart);
}

// Object.Destroy(part.gameObject): the Part stays allocated (Unity's fake null) and is marked destroyed
void EditorLogic::Destroy(Part* p) { destroyed_.insert(p); }

// EditorLogic.cs:3049
void EditorLogic::DeletePart(Part* part) {
    if (fetch != nullptr) {
        if (part == fetch->selectedPart) fetch->DestroySelectedPart();
        else fetch->deletePartAndSymmetryParts(part);
    }
}

// EditorLogic.cs:3064
void EditorLogic::deletePart(Part* part, bool symmetry, bool printConfirmation) {
    (void)symmetry;
    (void)printConfirmation;
    std::vector<Part*> list = EditorLogicBase::FindPartsInChildren(part);
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        Part* part2 = list[static_cast<size_t>(count)];
        if (ship->Contains(part2)) ship->Remove(part2);
        part2->OnDelete();
        Destroy(part2);
    }
    isCurrentPartFlag = false;
}

// EditorLogic.cs:3101
void EditorLogic::DestroySelectedPart() {
    if (selectedPart != nullptr) {
        if (ship->Contains(selectedPart) && selectedPart != rootPart) {
            detachPart(selectedPart);
            SetBackup();
        }
        GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartDeleted, selectedPart);
        displayAttachNodeIcons(ship->liveParts, selectedPart, false, false, false);
        deletePartAndSymmetryParts(selectedPart);
        selectedPart = nullptr;
        isCurrentPartFlag = false;
    }
}

// ==================================================================================================================================
// composite steps of the placement loop
// ==================================================================================================================================

// st_place.OnUpdate (EditorLogic.cs:1380, the state delegate) after the drag position was applied
void EditorLogic::UpdatePlacement() {
    if (selectedPart == nullptr) return;
    SetRotation(*selectedPart, vesselRotation * selectedPart->fields.initRotation * selectedPart->fields.attRotation);
    attachment = checkAttach(selectedPart);
    selectedPart->fields.attRotation0 = attachment.rotation;
    selectedPart->fields.attPos0 = attachment.position;
    if (attachment.possible) SetRotation(*selectedPart, attachment.rotation * selectedPart->fields.attRotation);
    selectedPart->partTransform.position = attachment.position;
    highlightSelected(attachment.possible, selectedPart);
    if (selectedPart->potentialParent != nullptr && attachment.potentialParent != selectedPart->potentialParent) {
        deleteSymmetryParts();
        wipeSymmetry(selectedPart);
    }
    selectedPart->potentialParent = attachment.potentialParent;
    if (attachment.potentialParent != nullptr) {
        createSymmetry(symmetryMode, attachment);
    } else {
        deleteSymmetryParts();
        if (symmetryModeBeforeNodeAttachment >= 0) RestoreSymmetryModeBeforeNodeAttachment();
    }
    cPartAttachments = CheckSymPartsAttach(attachment);
    const AttachRules r = RulesOf(*selectedPart);
    displayAttachNodeIcons(ship->liveParts, selectedPart, r.stack, r.srfAttach, r.allowDock);
    GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartDragging, selectedPart);
}

// st_place.OnLeave (EditorLogic.cs:1900, the state delegate)
void EditorLogic::LeavePlace(bool toIdle) {
    if (selectedPart != nullptr) displayAttachNodeIcons(ship->liveParts, selectedPart, false, false, false);
    cleanSymmetry();
    CheckCopiedSymmetryPersistentId(selectedPart);
    GameEvents::onEditorPartPlaced.Fire(selectedPart);
    if (toIdle && selectedPart != nullptr) selectedPart = nullptr;
    GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartDropped, selectedPart);
}

// on_partAttached.OnEvent (EditorLogic.cs:1940, the event delegate)
bool EditorLogic::DropSelectedPart() {
    isCurrentPartFlag = false;
    if (!selectedPart->symmetryCounterparts.empty()) {
        RestoreSymmetryState();
        bool flag = true;
        int num3 = static_cast<int>(cPartAttachments.size());
        while (num3-- > 0)
            if (!cPartAttachments[static_cast<size_t>(num3)].possible) {
                flag = false;
                break;
            }
        if (!flag) return false;
        attachPart(selectedPart, attachment);
        attachSymParts(cPartAttachments);
    } else {
        attachPart(selectedPart, attachment);
        if (symmetryModeBeforeNodeAttachment >= 0) RestoreSymmetryModeBeforeNodeAttachment();
    }
    SetBackup();
    return true;
}

// on_partPicked.OnEvent (EditorLogic.cs:1770, the event delegate) for a part that is not the root
void EditorLogic::PickPart(Part* part) {
    selectedPart = part;
    if (selectedPart != selectedPart->localRoot()) {
        const bool num5 = ship->Contains(selectedPart);
        detachPart(selectedPart);
        deleteSymmetryParts();
        if (num5) {
            GameEvents::onEditorPartPicked.Fire(selectedPart);
            SetBackup();
            GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartDetached, selectedPart);
            return;
        }
    } else {
        SetBackup();
    }
}

// ==================================================================================================================================
// gizmo callbacks
// ==================================================================================================================================

// EditorLogic.cs:2445
void EditorLogic::onRotateGizmoUpdate(const Quaternionf& dRot) {
    PartTransform& referenceTransform = ReferenceTransform(*selectedPart);
    switch (gizmoRotate.coordSpace) {
    case UnityEngine::Space::Self:
        referenceTransform.rotation = NormalizeSafe(gizmoRotate.transform.rotation * selectedPart->fields.initRotation);
        break;
    case UnityEngine::Space::World:
        referenceTransform.rotation = NormalizeSafe(dRot * gizmoRotate.hostRot0);
        break;
    }
    selectedPart->fields.attRotation0 = LocalRotation(*selectedPart);
    selectedPart->fields.attRotation = gizmoAttRotate * Inverse(gizmoAttRotate0) * LocalRotation(*selectedPart);
    if (symUpdateAttachNode != nullptr) {
        referenceTransform.position += getPivotOffset(gizmoPivot, referenceTransform.TransformPoint(symUpdateAttachNode->position));
        gizmoRotate.transform.position = gizmoPivot;
    }
    if (symUpdateMode != 0) UpdateSymmetry(selectedPart, symUpdateMode, symUpdateParent, symUpdateAttachNode);
    GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartRotating, selectedPart);
}

// EditorLogic.cs:2471
void EditorLogic::onRotateGizmoUpdated(const Quaternionf& dRot) {
    (void)dRot;
    if (ship->Contains(selectedPart)) SetBackup();
    GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartRotated, selectedPart);
}

// EditorLogic.cs:2480
void EditorLogic::partRotationResetUpdate() {
    srfAttachCursorOffset = Vector3f::zero();
    if (inRotateTweakState) {
        selectedPart->fields.attRotation = gizmoAttRotate;
        SetLocalRotation(*selectedPart, gizmoAttRotate0);
        selectedPart->fields.attRotation0 = gizmoAttRotate0;
        if (symUpdateAttachNode != nullptr) {
            selectedPart->partTransform.position += getPivotOffset(gizmoPivot, selectedPart->partTransform.TransformPoint(symUpdateAttachNode->position));
            gizmoRotate.transform.position = selectedPart->partTransform.position;
        }
    } else {
        selectedPart->fields.attRotation = Quaternionf::identity();
        SetLocalRotation(*selectedPart, selectedPart->fields.attRotation0);
    }
    if (symUpdateMode != 0) UpdateSymmetry(selectedPart, symUpdateMode, symUpdateParent, symUpdateAttachNode);
    partTweaked = true;
}

// EditorLogic.cs:2519
void EditorLogic::onOffsetGizmoUpdate(const Vector3f& dPos) {
    (void)dPos;
    EditorEnvironment& env = EditorEnvironment::Current();
    PartTransform& referenceTransform = ReferenceTransform(*selectedPart);
    referenceTransform.position = gizmoOffset.transform.position;
    if (selectedPart != rootPart) {
        const float threshold = env.fineTweak ? env.fineOffsetThreshold : 0.1f;
        if (env.setSurfaceAttachActive) env.setSurfaceAttachActive(*selectedPart, true);
        Part* referenceParent = env.referenceParent ? env.referenceParent(*selectedPart) : selectedPart->parent;
        Vector3f offsetGap{};
        if (EditorGeometryUtil::TestPartBoundsSeparate(*selectedPart, *referenceParent, threshold, offsetGap)) {
            if (static_cast<AttachModes>(selectedPart->fields.attachMode) == AttachModes::STACK) {
                AttachNode* childToParent = selectedPart->FindAttachNodeByPart(selectedPart->parent);
                AttachNode* parentToChild = selectedPart->parent->FindAttachNodeByPart(selectedPart);
                const Vector3f localPos = &referenceTransform == &selectedPart->partTransform
                                              ? LocalPosition(*selectedPart)
                                              : selectedPart->partTransform.InverseTransformPoint(referenceTransform.position);
                const Vector3f diff = parentToChild->position - (localPos + selectedPart->fields.attRotation * childToParent->position);
                if (static_cast<double>(diff.sqrMagnitude()) > static_cast<double>(threshold) * static_cast<double>(parentToChild->size + 1)) {
                    gizmoOffset.transform.position -= offsetGap;
                    referenceTransform.position = gizmoOffset.transform.position;
                }
            } else {
                gizmoOffset.transform.position -= offsetGap;
                referenceTransform.position = gizmoOffset.transform.position;
            }
        }
        if (env.setSurfaceAttachActive) env.setSurfaceAttachActive(*selectedPart, false);
    }
    const Vector3f localPos = &referenceTransform == &selectedPart->partTransform ? LocalPosition(*selectedPart)
                                                                                   : selectedPart->partTransform.InverseTransformPoint(referenceTransform.position);
    selectedPart->fields.attPos = localPos - selectedPart->fields.attPos0;
    if (symUpdateMode != 0) UpdateSymmetry(selectedPart, symUpdateMode, symUpdateParent, symUpdateAttachNode);
    GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartOffsetting, selectedPart);
}

// EditorLogic.cs:2563
void EditorLogic::onOffsetGizmoUpdated(const Vector3f& dPos) {
    (void)dPos;
    if (ship->Contains(selectedPart)) SetBackup();
    GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartOffset, selectedPart);
}

// EditorLogic.cs:2576
void EditorLogic::partOffsetInputUpdate(bool resetKeyPressed) {
    if (resetKeyPressed && &ReferenceTransform(*selectedPart) == &selectedPart->partTransform) {
        selectedPart->fields.attPos = Vector3f::zero();
        SetLocalPosition(*selectedPart, selectedPart->fields.attPos0);
        srfAttachCursorOffset = Vector3f::zero();
        if (symUpdateMode != 0) UpdateSymmetry(selectedPart, symUpdateMode, symUpdateParent, symUpdateAttachNode);
        GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartOffset, selectedPart);
    }
    for (size_t i = 0, count = selectedPart->symmetryCounterparts.size(); i < count; i++) {
        Part* part = selectedPart->symmetryCounterparts[i];
        part->fields.attPos = LocalPosition(*part) - part->fields.attPos0;
    }
}

// EditorLogic.cs:2602 (the root candidate check and the hierarchy flip; audio and the pick are the shell's)
void EditorLogic::OnNewRootSelect(Part* newRoot) {
    if (EditorReRootUtil::MakeRoot(newRoot, selectedPart)) {
        selectedPart = newRoot;
        if (ship->Contains(newRoot)) rootPart = newRoot;
    }
}

// ==================================================================================================================================
// undo / redo
// ==================================================================================================================================

// EditorLogic.cs:2675
void EditorLogic::SetBackup() {
    if (!ship->liveParts.empty()) {
        if (undoLevel < static_cast<int>(ShipConstruction::backups.size())) {
            ShipConstruction::backups.erase(ShipConstruction::backups.begin() + undoLevel, ShipConstruction::backups.end());
        }
        ship->shipName = shipNameField;
        ship->shipDescription = shipDescriptionField;
        ship->missionFlag = FlagURL;
        if (static_cast<int>(ShipConstruction::backups.size()) >= EditorEnvironment::Current().undoRedoLimit)
            ShipConstruction::ShiftAndCreateBackup(*ship);
        else
            ShipConstruction::CreateBackup(*ship);
        undoLevel = static_cast<int>(ShipConstruction::backups.size());
        GameEvents::onEditorSetBackup.Fire(ship);
        GameEvents::onEditorShipModified.Fire(ship);
    }
}

// EditorLogic.cs:2701
void EditorLogic::ResetBackup() {
    ShipConstruction::backups.pop_back();
    undoLevel = static_cast<int>(ShipConstruction::backups.size());
    SetBackup();
}

ShipConstruct* EditorLogic::AdoptShip(std::unique_ptr<ShipConstruct> s) {
    oldShips_.push_back(std::move(shipOwned_));
    shipOwned_ = std::move(s);
    ship = shipOwned_.get();
    return ship;
}

// EditorLogic.cs:2771
void EditorLogic::RestoreState(int offset, bool inPodSelect) {
    if (offset == 0 || ShipConstruction::backups.empty() ||
        (offset == -1 && ship->liveParts.size() == 1 && ShipConstruction::backups[static_cast<size_t>(undoLevel - 1)]->GetNodes("PART").empty()))
        return;
    const int num = std::clamp(undoLevel + offset, 1, static_cast<int>(ShipConstruction::backups.size()));
    if (num == undoLevel && !inPodSelect) return;
    if (selectedPart != nullptr) displayAttachNodeIcons(ship->liveParts, selectedPart, false, false, false);
    if (!ship->liveParts.empty())
        for (Part* p : EditorLogicBase::FindPartsInChildren(rootPart)) Destroy(p);   // DestroyImmediate(rootPart.gameObject)
    undoLevel = num;
    AdoptShip(ShipConstruction::RestoreBackup(undoLevel - 1));
    rootPart = ship->liveParts[0]->localRoot();
    ShipConstruction::SanitizeCraftIDs(ship->liveParts, false);
    ShipConstruction::ShipConfig = ship->SaveShip();
    GameEvents::onEditorRestoreState.Fire();
    GameEvents::onEditorShipModified.Fire(ship);
    GameEvents::onEditorUndo.Fire(ship);
    if (selectedPart != nullptr) {
        SaveEnvironment& env = SaveEnvironment::Current();
        selectedPart->fields.persistentId = env.persistentIds.CheckPartpersistentId(selectedPart->fields.persistentId, selectedPart, true, true);
        for (Part* c : selectedPart->children) c->fields.persistentId = env.persistentIds.CheckPartpersistentId(c->fields.persistentId, c, true, true);
    }
}

// ==================================================================================================================================
// ship queries
// ==================================================================================================================================

// EditorLogic.cs:5380 (recurseShipList :5386)
std::vector<Part*> EditorLogic::getSortedShipList() {
    std::vector<Part*> list;
    std::function<void(Part*)> recurse = [&](Part* part) {
        if (part == nullptr) return;
        list.push_back(part);
        for (size_t i = 0, count = part->children.size(); i < count; i++) recurse(part->children[i]);
    };
    recurse(rootPart);
    return list;
}

// EditorLogic.cs:464
std::vector<Part*> EditorLogic::SortedShipList() { return fetch->getSortedShipList(); }

// EditorLogic.cs:5407
bool EditorLogic::AreAllPartsConnected() {
    int count = static_cast<int>(Part::allParts.size());
    do {
        if (count-- <= 0) return true;
    } while (ship->Contains(Part::allParts[static_cast<size_t>(count)]));
    return false;
}

// EditorLogic.cs:5421
int EditorLogic::CountAllSceneParts(bool includeSelected) {
    (void)includeSelected;
    if (selectedPart != nullptr) return static_cast<int>(Part::allParts.size()) - 1;
    return static_cast<int>(Part::allParts.size());
}

// EditorLogic.cs:5429
bool EditorLogic::ShipIsValid(const ShipConstruct* s) const { return s != nullptr && !s->liveParts.empty(); }

// EditorLogic.cs:376
Vector3f EditorLogic::initialPodPosition() const {
    if (!EditorBounds::Instance) return Vector3f(0.0f, 5.0f, 0.0f);
    return EditorBounds::Instance->rootPartSpawnPoint;
}

// EditorLogic.cs:402
Bounds EditorLogic::editorBounds() const {
    if (!EditorBounds::Instance) return Bounds(Vector3f::zero(), Vector3f::one() * 3.40282347E+38f);
    return EditorBounds::Instance->constructionBounds;
}

// ==================================================================================================================================
// pre-flight check list
// ==================================================================================================================================

// EditorLogic.cs:4993
std::unique_ptr<PreFlightCheck> EditorLogic::GetStockPreFlightCheck(const std::string& siteName) {
    using namespace PreFlightTests;
    EditorLimits& lim = EditorLimits::Instance();
    auto level = [&](SpaceCenterFacility f) { return lim.facilityLevel ? lim.facilityLevel(f) : 0.0f; };
    auto partLimit = [&](SpaceCenterFacility f, bool isVAB) { return lim.partCountLimit ? lim.partCountLimit(level(f), isVAB) : 2147483647; };
    auto sizeLimit = [&](SpaceCenterFacility f, bool isPad) {
        return lim.craftSizeLimit ? lim.craftSizeLimit(level(f), isPad) : Vector3f(3.40282347E+38f, 3.40282347E+38f, 3.40282347E+38f);
    };
    auto massLimit = [&](SpaceCenterFacility f, bool isPad) { return lim.craftMassLimit ? lim.craftMassLimit(level(f), isPad) : 1.7976931348623157E+308; };
    auto check = std::make_unique<PreFlightCheck>(proceedWithVesselLaunch, abortLaunch);
    switch (EditorEnvironment::Current().editorFacility) {
    case EditorFacility::SPH:
        check->AddTest(std::make_unique<CraftWithinPartCountLimit>(*ship, SpaceCenterFacility::SpaceplaneHangar,
                                                                   partLimit(SpaceCenterFacility::SpaceplaneHangar, false)));
        break;
    case EditorFacility::VAB:
        check->AddTest(std::make_unique<CraftWithinPartCountLimit>(*ship, SpaceCenterFacility::VehicleAssemblyBuilding,
                                                                   partLimit(SpaceCenterFacility::VehicleAssemblyBuilding, true)));
        break;
    default: break;
    }
    if (siteName == "LaunchPad" || siteName == "Woomerang_Launch_Site" || siteName == "Desert_Launch_Site") {
        check->AddTest(std::make_unique<CraftWithinSizeLimits>(*ship, SpaceCenterFacility::LaunchPad, sizeLimit(SpaceCenterFacility::LaunchPad, true)));
        check->AddTest(std::make_unique<CraftWithinMassLimits>(*ship, SpaceCenterFacility::LaunchPad, massLimit(SpaceCenterFacility::LaunchPad, true)));
    }
    if (siteName == "Runway" || siteName == "Island_Airfield" || siteName == "Desert_Airfield") {
        check->AddTest(std::make_unique<CraftWithinSizeLimits>(*ship, SpaceCenterFacility::Runway, sizeLimit(SpaceCenterFacility::Runway, false)));
        check->AddTest(std::make_unique<CraftWithinMassLimits>(*ship, SpaceCenterFacility::Runway, massLimit(SpaceCenterFacility::Runway, false)));
    }
    check->AddTest(std::make_unique<ExperimentalPartsAvailable>(*ship));
    check->AddTest(std::make_unique<CanAffordLaunchTest>(*ship, PreFlightEnvironment::Current().funds));
    if (PreFlightEnvironment::Current().isSpaceCenterFacility && PreFlightEnvironment::Current().isSpaceCenterFacility(siteName))
        check->AddTest(std::make_unique<FacilityOperational>(siteName, siteName));
    check->AddTest(std::make_unique<NoControlSources>(ShipConstruction::ShipManifest));
    if (!siteName.empty()) check->AddTest(std::make_unique<LaunchSiteClear>(siteName, launchSiteDisplayName(siteName)));
    return check;
}

}  // namespace ksp
