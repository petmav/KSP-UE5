// WP-32: RetractableLadder.cs (the retractable ladder module of crew pods / hatches): the four-state KerbalFSM (Retracted / Extended / Extending /
// Retracting) with its editor shortcut events, the actions / events, the drag-cube weights and the inventory hooks.
// Seam (RetractableLadderRig): the Animation component and its clip, the ladder Collider, the model-transform lookups. The shell fills the rig
// (clip length, which lookups succeeded); OnStart takes the same error branches as the C#.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/eva/eva_engine.hpp"
#include "ksp/fsm/kerbal_fsm.hpp"
#include "ksp/modules/part_module.hpp"

namespace ksp {

struct KSPActionParam;

// What RetractableLadder reads and writes of Unity
struct RetractableLadderRig {
    bool rootTransformFound = true;      // part.FindModelTransform(ladderAnimationRootName) != null
    bool animationFound = true;          // transform.GetComponent<Animation>()
    bool clipFound = true;               // anim[ladderRetractAnimationName]
    bool colliderTransformFound = true;  // part.FindModelTransform(ladderColliderName) != null
    bool colliderFound = true;           // transform2.GetComponent<Collider>()
    EvaAnimState clip;                   // anim[ladderRetractAnimationName]: length, speed, normalizedTime, weight, enabled
    int wrapMode = 0;                    // WrapMode (ClampForever = 8)
    bool isPlaying = false;              // anim.isPlaying
    bool colliderEnabled = false;        // ladder.enabled
    std::vector<std::string> log;        // Animation.Play / Sample calls, in order
};

class RetractableLadder : public PartModule, public IMultipleDragCube {
public:
    explicit RetractableLadder(const ModuleClassInfo& cls);
    ~RetractableLadder() override;

    // [KSPField]s
    std::string StateName() const { return Get<std::string>("StateName"); }   // :6
    std::string ladderAnimationRootName() const { return Get<std::string>("ladderAnimationRootName"); }  // :9
    std::string ladderColliderName() const { return Get<std::string>("ladderColliderName"); }            // :12
    std::string ladderRetractAnimationName() const { return Get<std::string>("ladderRetractAnimationName"); }  // :15
    float externalActivationRange() const { return Get<float>("externalActivationRange"); }  // :18

    RetractableLadderRig rig;
    std::unique_ptr<KerbalFSM> ladderFSM;                 // :26
    KFSMState *st_retracted = nullptr, *st_extended = nullptr, *st_retracting = nullptr, *st_extending = nullptr;  // :28-:34
    KFSMEvent *On_retractStart = nullptr, *On_extendStart = nullptr, *On_retractEditor = nullptr, *On_extendEditor = nullptr;  // :36-:46
    KFSMEvent *On_retractEnd = nullptr, *On_extendEnd = nullptr;  // KFSMTimedEvent instances assigned to KFSMEvent fields
    // HighLogic.LoadedSceneIsEditor / LoadedSceneIsMissionBuilder (unset: false)
    static inline std::function<bool()> loadedSceneIsEditor;
    static inline std::function<bool()> loadedSceneIsMissionBuilder;
    std::vector<std::string> errors;                      // Debug.LogError texts, in order

    bool IsMultipleCubesActive() const override { return true; }  // :48
    void OnAwake() override;
    void ToggleAction(KSPActionParam& param);             // :50 [KSPAction]
    void ExtendAction(KSPActionParam& param);             // :62
    void RetractAction(KSPActionParam& param);            // :68
    void Extend();                                        // :75 [KSPEvent]
    void Retract();                                       // :101 [KSPEvent]
    void OnStart(StartState state) override;              // :126
    void OnInventoryModeDisable() override;               // :246
    void OnStoredInInventory(ModuleInventoryPart& moduleInventoryPart) override;  // :262
    void OnPartCreatedFomInventory(ModuleInventoryPart& moduleInventoryPart) override;  // :282
    void DemoteToPhysicslessPart() override;              // :288
    void Update();                                        // :293
    void FixedUpdate();                                   // :302
    void SetDragState(float b);                           // :311
    std::vector<std::string> GetDragCubeNames() const override { return {"A", "B"}; }  // :317
    void AssumeDragCubePosition(const std::string& name) override;  // :322
    bool UsesProceduralDragCubes() const override { return false; }  // :350
    void SetAnimationToRetracted();                       // :355
    Part* PartPtr() const;

private:
    KFSMState* NewState(const std::string& n);
    KFSMEvent* NewEvent(const std::string& n);
    KFSMTimedEvent* NewTimed(const std::string& n, double t);
    std::vector<std::unique_ptr<KFSMState>> states_;
    std::vector<std::unique_ptr<KFSMEvent>> events_;
};

namespace detail {
void RegisterRetractableLadder();  // retractable_ladder.cpp
}

}  // namespace ksp
