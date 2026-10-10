// WP-26: CompoundPart.cs -- the Part subclass of struts and fuel lines (`module = CompoundPart`): the target data (tgt / tpersID / pos /
// rot / dir / col persistence), attach state, target link bookkeeping (SetTarget / DumpTarget / UpdateWorldValues / UpdateTargetCoords),
// the CompoundPartModule notifications and the legacy `customPartData` upgrade (ParseCustomPartData).
//
// CompoundPart's own public fields live in `compoundFields` (WP-05: direction, targetPosition, targetRotation, targetMeshColName,
// maxLength, attachState, disconnectedEffectName, disconnectAction); `target` (a Part) is a member here. Not ported (editor / physics /
// UI, see STATUS.md WP-26): the mouse-driven target selection and cone casts (onAttachUpdate, raycastTarget, findTargetDirection,
// getAnchorRot), the editor tweak mode (onEditorStartTweak / EndTweak, ICMTweakTarget), the input locks (lockEditor / LockEVAEditor),
// the part effect `Effect(disconnectedEffectName)`, GetReferenceTransform / GetPartColliders, the delayed raycast coroutine.
// `Part::Instantiate` and PartLoader create a CompoundPart for className "CompoundPart". The host calls OnStartComplete (Part.Start's
// tail: onStartComplete) and OnPartDestroy (Part.OnDestroy: onPartDestroy), which Part does not model.
#pragma once

#include <functional>
#include <string>

#include "ksp/parts/part.hpp"

namespace ksp {

enum class ConstructionEventType : int;  // ConstructionEventType.cs

class CompoundPart : public Part {
public:
    enum class AttachState : int { Detached = 0, Attaching = 1, Attached = 2 };  // CompoundPart.cs:7

    CompoundPart();
    ~CompoundPart() override;

    // What CompoundPart reads from the editor / flight scene (EditorLogic.*, HighLogic.*): unset = no editor
    struct Hooks {
        std::function<Part*()> editorSelectedPart;                       // EditorLogic.SelectedPart
        std::function<bool()> editorSymmetryIsMirror;                    // EditorLogic.fetch.symmetryMethod == SymmetryMethod.Mirror
        std::function<std::vector<Part*>()> editorShipParts;             // EditorLogic.fetch.ship.parts
        std::function<std::vector<Part*>()> editorSortedShipList;        // EditorLogic.SortedShipList
        std::function<void(CompoundPart&)> scheduleRaycast;              // the delayed schedule_raycast coroutine
    };
    static inline Hooks hooks;

    Part* target = nullptr;                                              // :20
    uint32_t tgtId() const { return tgtId_; }                            // :32 private
    uint32_t targetPersistentId() const { return targetPersistentId_; }  // :34 private
    bool hasSaveData() const { return hasSaveData_; }                    // :30
    Vector3f wTgtPos() const { return wTgtPos_; }                        // :46
    Quaternionf wTgtRot() const { return wTgtRot_; }                     // :48
    AttachState attachState() const { return static_cast<AttachState>(compoundFields.attachState); }
    void set_attachState(AttachState s) { compoundFields.attachState = static_cast<int32_t>(s); }
    bool needsDirectionFlip = false;                                     // :36 private

    void onPartAwake() override;                                         // :128
    void onCopy(Part* original, bool asSymCPart) override;               // :114
    void OnSave(ConfigNode& node) override;                              // :145
    void OnLoad(ConfigNode& node) override;                              // :158
    void onPartAttach(Part* parent) override;                            // :234 (the data half)
    void onPartDetach() override;                                        // :265
    void OnStartComplete();                                              // :169 onStartComplete
    void OnPartDestroy();                                                // :283 onPartDestroy
    void DisconnectEvent();                                              // :80 [KSPEvent]
    void DisconnectAction(KSPActionParam& param);                        // :86 [KSPAction]
    void DisconnectCompoundPart();                                       // :91
    bool SetTarget(Part* tgt, const std::string& tgtColName);            // :639
    void UpdateWorldValues();                                            // :677
    void DumpTarget();                                                   // :683
    void UpdateTargetCoords();                                           // :808
    void OnEditorEvent(ConstructionEventType evt, Part* selPart);        // :800
    void OnTargetDetach();                                               // :375 (the target left the ship)
    void OnTargetDestroy();                                              // :384
    ConfigNode ParseCustomPartData(const std::string& customPartData);   // :827

private:
    bool hasSaveData_ = false;
    uint32_t tgtId_ = 0;
    uint32_t targetPersistentId_ = 0;
    CompoundPart* original_ = nullptr;
    Vector3f wTgtPos_{};
    Quaternionf wTgtRot_ = Quaternionf::identity();
    bool subscribed_ = false;
    void SetLink();                                                      // :708
    void UnsetLink();                                                    // :717
};

}  // namespace ksp
