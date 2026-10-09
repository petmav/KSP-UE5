// WP-16: the staging MODEL of KSP/UI/Screens/StageManager.cs (+ the data side of StageGroup.cs, StageIcon.cs, ProtoStageIcon.cs).
//
// StageManager is a MonoBehaviour of the flight and editor UI; kspcore keeps what decides the staging sequence: the list of
// StageGroups (each a list of StageIcons = one per staged Part), the inverse-stage / separation-index assignment
// (FindStageIndices, GenerateStagingSequence, SetSeparationIndices, RecalculateVesselStaging, UpdateStageGroups), activation
// (ActivateStage / ActivateNextStage / BeginFlight / ResumeFlight), the SortIcons pipeline (held icons -> stage groups, empty
// stages deleted), symmetry grouping of icons, and the GameEvents it listens to and fires. StageGroup / StageIcon / the icon of
// ProtoStageIcon are plain data records (WP-43 owns the UI classes); Unity UI (layout, tweens, drag and drop, hover, sounds, the
// delta-v headings, ScreenMessages) is not ported.
//
// The scene the manager works in (editor: EditorLogic.RootPart / SelectedPart, flight: FlightGlobals.ActiveVessel) is the
// IStagingHost seam below; a ShipConstruct (WP-18) / an editor implements it, flight uses FlightStagingHost over the Vessel.
//
// Coroutines: `SortIcons(instant: false)` starts a 3-frame coroutine in KSP; here it is pending state advanced by Tick() (one Unity
// frame) or FlushSort(). `cooldownSeparation` is `canSeparate` re-enabled by Tick() after PhysicsGlobals.StagingCooldownTimer.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ksp/events/game_event_types.hpp"
#include "ksp/staging/staging_types.hpp"

namespace ksp {

class Part;
class Vessel;
class StageGroup;
class StageManager;

// StackIconGrouping.cs
enum class StackIconGrouping : int { SAME_MODULE, SAME_TYPE, SAME_HIERARCHY, SYM_COUNTERPARTS, NONE };

// What StageManager reads from its scene (HighLogic.LoadedSceneIsFlight / LoadedSceneIsEditor, EditorLogic.RootPart /
// SelectedPart, FlightGlobals.ActiveVessel). A ShipConstruct-based editor (WP-18) implements this; `FlightStagingHost` and
// `EditorStagingHost` below are the two ready-made ones.
class IStagingHost {
public:
    virtual ~IStagingHost() = default;
    virtual bool LoadedSceneIsFlight() const = 0;
    virtual bool LoadedSceneIsEditor() const = 0;
    virtual Part* RootPart() = 0;                      // EditorLogic.RootPart / FlightGlobals.ActiveVessel.rootPart
    virtual Vessel* ActiveVessel() { return nullptr; }  // FlightGlobals.ActiveVessel (flight scene)
    virtual Part* SelectedPart() { return nullptr; }    // EditorLogic.SelectedPart
};

// Flight scene over FlightRegistry::ActiveVessel
class FlightStagingHost : public IStagingHost {
public:
    bool LoadedSceneIsFlight() const override { return true; }
    bool LoadedSceneIsEditor() const override { return false; }
    Part* RootPart() override;
    Vessel* ActiveVessel() override;
};

// Editor scene: the ship's root part (and the part held by the cursor) are set by whoever owns the construct
class EditorStagingHost : public IStagingHost {
public:
    Part* root = nullptr;
    Part* selected = nullptr;
    bool LoadedSceneIsFlight() const override { return false; }
    bool LoadedSceneIsEditor() const override { return true; }
    Part* RootPart() override { return root; }
    Part* SelectedPart() override { return selected; }
};

// KSP/UI/Screens/StageIcon.cs, data side (one per staged Part; `protoIcon.Part` is `part`)
class StageIcon {
public:
    explicit StageIcon(Part* p) : part(p) {}
    Part* part = nullptr;               // ProtoIcon.Part
    StageGroup* stage = nullptr;        // :54 / Stage property (:120)
    int SiblingIndex = -1;              // :58
    bool frozen = false;                // :62
    bool grouped = false;               // :64
    bool expanded = false;              // :66
    std::vector<StageIcon*> groupedIcons;  // :68
    StageIcon* groupLead = nullptr;     // :70
    bool selected = false;              // :88
    bool active = true;                 // gameObject.activeSelf

    bool isMainIcon() const;                   // :136
    bool isSymmetryCounterPart() const;        // :152 (symmetryCounterparts.Count > 0)
    int InverseStage() const;                  // :164
    int InStageIndex() const;                  // :176
    int DefaultSequenceIndex() const;          // :188
    int OriginalStageIndex() const;            // :200
    StackIconGrouping IconGroupingRule() const;  // :236 (Part.stackIconGrouping: SYM_COUNTERPARTS, SAME_TYPE for ModuleRCS parts, ModuleRCS.cs:436)

    void SetInverseSequenceIndex(int inverseIndex, int inStageIndex, bool seqOverride = true);  // :771
    void SetManualStageOffset(int inverseIndex);                                                // :810
    void SetInStageIndex(int index);                                                            // :832
    void SetInStageIndexOFGroupedIcons(bool setSiblingIndex = true);                            // :837
    void SortGroupedIcons();                                                                    // :857 (order by the parent's children)
    void Reset();                                                                               // :889
    bool ResetAvailable() const;                                                                // :961
    void AddToGroup(StageIcon* icon, bool setParent = true);                                    // :1000
    void RemoveFromGroup(StageIcon* icon, bool resetGroupLeadIfempty = true);                   // :1028
    bool RemoveFromGroupAndReshuffle(StageGroup*& foundInGroup);                                // :1049
    void HoldGroupedIcons(bool alertStagingSequencer = true);                                   // :1100
    void ExpandGroup();                                                                         // :1145
    void CollapseGroup();                                                                       // :1195
    void DisableGroup(StageIcon* iconToDisable);                                                // :1222
    void ConsolidateMembers();                                                                  // :1264

private:
    void ResetStageGroup();            // :904
    bool ResetStageGroupAvailable() const;  // :978
};

// KSP/UI/Screens/StageGroup.cs, data side
class StageGroup {
public:
    int defaultStage = -1;       // :68
    int inverseStageIndex = 0;   // :70
    bool manualOverride = false; // :72
    bool IsBeingDragged = false;
    std::vector<StageIcon*>& Icons() { return icons; }              // :110
    const std::vector<StageIcon*>& Icons() const { return icons; }

    void AddIcons(const std::vector<StageIcon*>& iconsToAdd);       // :542
    void AddIcon(StageIcon* icon, bool setParent = true);           // :551
    void AddIconAt(StageIcon* icon, int index, int forcedSiblingIndex, bool setParent = true);  // :570
    void RemoveIcon(StageIcon* icon, bool changeParents = true);    // :602
    void Delete();                                                  // :619 (the stage's delete button)
    void AddStageAfter();                                           // :632 (the stage's add button)
    void UpdateInStageIndexes();                                    // :642
    void SetInverseStageIndex(int index) { inverseStageIndex = index; }  // :652
    void SetPartIndices(bool seqOverride = true);                   // :657
    void SetManualStageOffset();                                    // :666
    void SetSiblingIndexes();                                       // :675
    void Reset();                                                   // :694
    bool ResetAvailable() const;                                    // :705
    StageIcon* FindSymmetryGroupleader(StageIcon* icon);            // :793
    void MoveAllIconsInto(StageGroup* moveInto);                    // :817
    void MoveIconsInto(StageGroup* oldStageGroup, StageIcon* selectedLeader, std::vector<StageIcon*> selectedIcons,
                       StageIcon* unSelectedLeader, std::vector<StageIcon*> unSelectedIcons, int insertAt, int modifiedSiblingIndex);  // :841

private:
    std::vector<StageIcon*> icons;
};

class StageManager {
public:
    // ---- statics ---------------------------------------------------------------------------------------------------------
    static StageManager* Instance;                       // :142 (null until Awake)
    static inline bool EnableDeleteEmptyStages = true;   // :115
    static int LastStage();                              // :146
    static int StageCount();                             // :158
    static int CurrentStage();                           // :160
    static std::vector<StageIcon*>& Selection();         // :162 (a fresh empty list while there is no Instance)
    static bool CanSeparate();                           // :174
    static double StagingCooldownTimer();                // PhysicsGlobals.StagingCooldownTimer (0.2 s default of the prefab)

    explicit StageManager(IStagingHost* host);
    ~StageManager();                                     // OnDestroy (:230)
    StageManager(const StageManager&) = delete;
    StageManager& operator=(const StageManager&) = delete;
    // Awake (:184): becomes the Instance (a second manager is refused: `usurper`) and subscribes to the GameEvents
    bool Awake();
    bool Visible = true;                                 // :176
    IStagingHost& Host() { return *host_; }

    std::vector<StageGroup*> Stages();                   // :144
    StageGroup* StageAt(int i) { return stages_.at(static_cast<size_t>(i)).get(); }

    // ---- GameEvents handlers (:273-358) -----------------------------------------------------------------------------------------
    void OnVesselClearStaging(Vessel* vessel);           // :281
    void OnVesselResumeStaging(Vessel* vessel);          // :286
    void OnPartUpdateStageability(Part* data);           // :294
    void OnEditorStarted();                              // :301
    void OnEditorPodSelected(Part* rootPart);            // :307
    void OnEditorPodDeleted();                           // :313
    void OnEditorPartPicked(Part* part);                 // :318
    void OnEditorPartPlaced(Part* part);                 // :328
    void OnEditorPartDeleted(Part* part);                // :333
    void OnEditorRestoreState();                         // :338
    void OnVesselSwitching(Vessel* from, Vessel* to);    // :344
    void OnEditorLoad();                                 // :352 (the ShipConstruct and LoadType arguments are not read)
    void OnEditorLoadEvent(ShipConstruct*, KSP::UI::Screens::CraftBrowserDialog_LoadType) { OnEditorLoad(); }  // GameEvents.onEditorLoad handler
    void SortIconsEvent(bool instant) { SortIcons(instant); }  // GameEvents.StageManager.SortIcons handler (:1121)

    // ---- icons (:822-947) ------------------------------------------------------------------------------------------------------
    void HoldIcon(StageIcon* stageIcon, bool removeIcon = true, bool alertStagingSequencer = true);  // :822
    void UnHoldIcon(StageIcon* stageIcon, bool changeParents = false);                                // :835
    static StageIcon* CreateIcon(Part& part, bool alertStagingSequencer = true);                      // :843 (ProtoStageIcon.CreateIcon)
    static void RemoveIcon(StageIcon* stageIcon, bool destroyIcon = true, bool removeSelection = true, bool alertStagingSequencer = true);  // :881
    static void DisableIcon(StageIcon* iconToDisable);                                                // :932
    StageIcon* IconOf(const Part* part) const;           // part.stackIcon.StageIcon
    const std::vector<StageIcon*>& IconsInHold() const { return iconsInHold; }

    // ---- stage groups (:949-1061) ----------------------------------------------------------------------------------------------
    void AddStageAt(int index);                          // :949
    void InsertStageAt(StageGroup* stageGroup, int index, int forcedSiblingIndex = -1);  // :958 (takes a group out of the graveyard)
    void DeleteStage(StageGroup* stageToDelete, bool tweenOut = false);                   // :972
    void DeleteEmptyStages();                            // :1005
    void DeleteEmptyStages(bool deleteManualStages);     // :1010
    void RemoveStage(StageGroup* stageGroup);            // :1032
    static int GetStageCount(const std::vector<Part*>& parts);   // :1038
    static void SetStageCount(const std::vector<Part*>& ship);   // :1049 (computes the maximum and discards it, sic)
    // :1063 FindPartsWithModuleSeparatingBeforeOtherPartsWithModule<Before, After>: `isBefore` / `isAfter` are the `FindModuleImplementing<T>() != null` tests
    static std::vector<Part*> FindPartsWithModuleSeparatingBeforeOtherPartsWithModule(const std::vector<Part*>& parts,
                                                                                   const std::function<bool(Part&)>& isBefore,
                                                                                   const std::function<bool(Part&)>& isAfter);
    void SetManualStageOffset(int startIndex);           // :1104

    // ---- the SortIcons pipeline (:1116-1207) -----------------------------------------------------------------------------------
    void SortIcons(bool instant);                        // :1121 (also the GameEvents.StageManager.SortIcons handler)
    void SortIcons(bool instant, bool deleteManualStages);  // :1126
    void SortIconsDefault();                             // :1116 [ContextMenu] SortIcons(instant: true, null, false, false)
    void SortIcons(bool instant, Part* p, bool switchingVessels, bool deleteManualStages);  // :1131
    void UpdateStageGroups(bool seqOverride = true);     // :1193
    // Unity frame / coroutine stand-ins: one frame of `dt` seconds advances the SortIcons coroutine (3 frames after the last request)
    // and the cooldownSeparation wait (:1308)
    void Tick(double dt = 0.0);
    void FlushSort();                                    // Tick() until the pending SortIcons ran
    bool SortPending() const { return sortRoutine_; }

    // ---- flight (:1209-1357) ---------------------------------------------------------------------------------------------------
    static void BeginFlight();                           // :1209
    static void ResumeFlight(int currStage);             // :1227
    static void ActivateNextStage();                     // :1235
    static void ActivateStage(int stage);                // :1244 (AudioSource clips and ResourceDisplay are UI)
    void IncrementCurrentStage();                        // :1314
    void DecrementCurrentStage();                        // :1323
    static void StepBackCurrentStage();                  // :1347

    // ---- the staging sequence (:1402-1577) -------------------------------------------------------------------------------------
    static void GenerateStagingSequence(Part* root);     // :1402
    static void FindStageIndices(Part* part, int thisStage, int carriedInverseStage);  // :1408
    static int RecalculateVesselStaging(Vessel* v);      // :1477
    static void SetSeparationIndices();                  // :1494
    static void SetSeparationIndices(Part* p, int sepIndex);  // :1506
    void Reset();                                        // :1579 (the editor's reset button; no ScreenMessages)
    bool CheckNullrefsInStageIconList() const;           // :1631

    // Test / host access to the private state the C# keeps in fields
    int CurrentStageValue() const { return currentStage_; }
    void SetCurrentStage(int cStage);                    // :1335 private
    bool rebuildIndexesValue() const { return rebuildIndexes_; }
    bool canSeparateValue() const { return canSeparate_; }

private:
    friend class StageGroup;
    friend class StageIcon;
    static void setSeparationIndices(Part* part, int sepIndex, int carriedInverseStage);  // :1512
    void AddNewStageGroupsIfNeeded(Part* selectedPart);  // :373
    void AddHeldIconsToStages(Part* selectedPart, bool switchingVessels);  // :416
    int GetParachuteStageIndex(int currentIndex);        // :637
    void SortIconsSequence(Part* p, bool switchingVessels, bool deleteManualStages);  // :1160
    void UpdateStageGroup(StageGroup* group, int index, bool seqOverride = true);     // :1202
    std::vector<StageIcon*> AddPartsWithStageiconsToListRecursive(Part* part);        // :1376
    void AddPartsWithStageiconsToListRecursive(std::vector<StageIcon*>& partsWithIcons, Part* part);  // :1383
    StageGroup* NewStageGroup();
    void InstallPartHooks();
    void RemovePartHooks();

    IStagingHost* host_;
    bool awake_ = false;
    std::vector<std::unique_ptr<StageGroup>> stages_;       // :94 `stages`
    std::vector<std::unique_ptr<StageGroup>> deletedGroups_;  // Destroy(stageGroup.gameObject): kept alive so stale pointers stay valid
    int currentStage_ = 0;                                  // :97 `_currentStage`
    std::vector<StageIcon*> iconsInHold;                    // :99
    std::vector<StageIcon*> selection;                      // :101
    std::vector<std::unique_ptr<StageIcon>> iconCache_;     // every icon ever created (the C# pools and reuses them)
    std::unordered_map<const Part*, StageIcon*> byPart_;    // part.stackIcon.StageIcon
    bool canSeparate_ = false;                              // :105
    bool holdEditorSelectedIcons = false;                   // :113
    bool rebuildIndexes_ = false;                           // :117
    bool sortRoutine_ = false;                              // :123 (coroutine running)
    int coroutineFramesToWait_ = 0;                         // :125
    Part* pendingPart_ = nullptr;                           // coroutine arguments
    bool pendingSwitching_ = false, pendingDeleteManual_ = false;
    double cooldown_ = 0.0;
    std::vector<std::function<void()>> subscriptions_;       // removal closures run by ~StageManager
    friend class FlightStagingHost;
};

}  // namespace ksp
