// WP-26: ModuleProceduralFairing.cs (+ IDynamicCargoOccluder.cs). The schema (KSPFields, KSPEvent / KSPAction attribute data) comes
// from the generated module_schema.inc / module_members.inc.
//
// Ported: the data model (xSections, Panels), OnAwake, OnStart (the numeric half: cargo module link, interstage lookup, SpawnMeshes
// = nCollidersPerArc + FairingPanel.SetupPanelArray + BuildMesh of every panel, the closed drag-cube bookkeeping, `panelMass`),
// OnLoad / OnSave (XSECTION nodes, `fsm`), GetFairingArea, the IPartMassModifier / IPartCostModifier / size queries, the
// interstage bookkeeping (SetInterstage, DumpInterstage), the flight state machine as a state name (idle / deployed) with
// DeployFairing / JettisonPanels' bookkeeping tail, IScalarModule, IActivateOnDecouple, OnChildRemove, CheckForFairingEdit's field
// flags, GetInfo / titles. NOT ported (see STATUS.md WP-26): the editor state machine driven by the mouse (placing x-sections:
// PlaceNewXSection, UpdateXSectionConeCast, GetMouse*, snapping), materials / textures / shaders, colliders (ColliderSection,
// SwitchToConvexColliders), the physical panels after a jettison (MakePanelPhysical, physicalObject, joints: PartJoint payload),
// PAW / UI, the coroutines of the flag attachment. They go through `hooks` where a physics / rendering package must act.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/events/event_data.hpp"
#include "ksp/modules/fairing_panel.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/staging/staging_types.hpp"

namespace ksp {

class Part;
class ModuleCargoBay;

// IDynamicCargoOccluder.cs
class IDynamicCargoOccluder {
public:
    virtual ~IDynamicCargoOccluder() = default;
    virtual void SetupOcclusionTest(bool testActive) = 0;
};

class ModuleProceduralFairing : public PartModule,
                                public IPartMassModifier,
                                public IPartCostModifier,
                                public IScalarModule,
                                public IModuleInfo,
                                public IDynamicCargoOccluder,
                                public IActivateOnDecouple {
public:
    explicit ModuleProceduralFairing(const ModuleClassInfo& cls);
    ~ModuleProceduralFairing() override;

    using FairingXSection = ProceduralFairings::FairingXSection;
    using FairingPanel = ProceduralFairings::FairingPanel;

    // What the model / physics packages do on behalf of the module (unset: nothing happens)
    struct Hooks {
        // MakePanelPhysical / ConvertToPhysicalObject of one jettisoned panel (:1868); `force` = ProjectOnPlane(...).normalized * ejectionForce
        std::function<void(Part&, FairingPanel&, float unitAreaMass)> makePanelPhysical;
        // SwitchToConvexColliders / ReleasePayload's joint (PartJoint.DestroyJoint)
        std::function<void(Part&)> releasePayload;
        // DragCubeSystem.RenderProceduralDragCube(part): the closed fairing's drag cube (:2043). The model's renderers are WP-21's input.
        std::function<void(Part&)> assumeClosedDragCube;
        // base.part.partInfo.partPrefab.DragCubes.Cubes[0] (:2061)
        std::function<void(Part&)> assumeDeployedDragCube;
        // ShipConstruction.FindPartWithCraftID(id) (editor, :365)
        std::function<Part*(uint32_t)> findPartWithCraftID;
        // payloadAttachNode.attachedPart while the physics package holds a payload joint (SecurePayload :1982): nullptr = none
        std::function<Part*(ModuleProceduralFairing&)> payloadJointFor;
    };
    static inline Hooks hooks;

    // ---- field accessors the panels use (private to the C# panel through `host.`) ----
    int nSides() const { return Get<int32_t>("nSides"); }
    float nArcs() const { return Get<float>("nArcs"); }
    Vector3f axis() const { return Get<Vector3f>("axis"); }
    Vector3f pivot() const { return Get<Vector3f>("pivot"); }
    float edgeSlide() const { return Get<float>("edgeSlide"); }
    float edgeWarp() const { return Get<float>("edgeWarp"); }
    float noseTip() const { return Get<float>("noseTip"); }
    float aberrantNormalLimit() const { return Get<float>("aberrantNormalLimit"); }
    // HighLogic.LoadedSceneIsEditor || HighLogic.LoadedSceneIsFlight (SaveEnvironment::Current().loadedScene)
    static bool loadedSceneIsEditorOrFlight();

    // ---- state (:83-150) ----
    std::vector<std::shared_ptr<FairingXSection>> xSections;  // :115 public
    int nCollidersPerArc = 0;                                 // :117
    std::vector<std::shared_ptr<FairingPanel>> Panels;        // :137
    float PanelMass() const { return panelMass_; }            // :148 private
    void SetPanelMass(float v) { panelMass_ = v; }            // test / host access (a private field in C#)
    const std::string& FsmStateName() const { return fsmStateName_; }
    bool FsmStarted() const { return fsmStarted_; }
    Part* Interstage() const { return interstage_; }

    // ---- PartModule ----
    void OnAwake() override;                                  // :313
    void OnStart(StartState state) override;                  // :330
    void OnLoad(const ConfigNode& node) override;             // :1468
    void OnSave(ConfigNode& node) override;                   // :1494
    void OnActive() override;                                 // :1573
    std::string GetInfo() override;                           // :2201
    std::string GetModuleDisplayName() override;              // :2206

    // ---- methods ----
    void DeleteFairing();                                     // :1534 [KSPEvent] (editor state machine: not ported beyond the state name)
    void DeployFairing();                                     // :1552 [KSPEvent]
    void DeployFairingAction(KSPActionParam& p);              // :1558 [KSPAction]
    float GetFairingArea() const;                             // :1581
    void SpawnMeshes(bool addColliders);                      // :1366
    void WipeMesh();                                          // :1427
    void RefreshPanels();                                     // :1462
    void JettisonPanels(std::vector<std::shared_ptr<FairingPanel>>& panels, float ejectionForce);  // :1765
    void SetInterstage(Part* iStg);                           // :640
    void DumpInterstage();                                    // :673
    void CheckForFairingEdit();                               // :2157 (the PAW flags `guiActiveEditor` of EditFairing / nArcs / editingBlocked)
    void OnChildRemove(Part* child);                          // :2122
    void EnterIdleState();                                    // st_flight_idle / st_editor_idle OnEnter: panelMass (:1074 / :835)
    void OnVesselModified(Vessel* v);                         // :737

    // IPartMassModifier / IPartCostModifier (:1663, :1682); GetModuleSize :1701 is not part of the interface list in C#
    IPartMassModifier* AsMassModifier() override { return this; }
    IPartCostModifier* AsCostModifier() override { return this; }
    IModuleInfo* AsModuleInfo() override { return this; }
    IScalarModule* AsScalarModule() override { return this; }
    float GetModuleMass(float defaultMass, ModifierStagingSituation sit) override;
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::STAGED; }  // :1672
    float GetModuleCost(float defaultCost, ModifierStagingSituation sit) override;
    ModifierChangeWhen GetModuleCostChangeWhen() override { return ModifierChangeWhen::STAGED; }  // :1691
    Vector3f GetModuleSize(const Vector3f& defaultSize, ModifierStagingSituation sit);  // :1701 -> Vector3.zero
    ModifierChangeWhen GetModuleSizeChangeWhen() { return ModifierChangeWhen::STAGED; }  // :1706

    // IScalarModule (:274-300, :2014-2030)
    std::string ScalarModuleID() const override { return Get<std::string>("moduleID"); }
    float GetScalar() const override;
    bool CanMove() const override { return fsmStateName_ == "st_idle" && flightFsm_; }
    void SetScalar(float) override {}
    void SetUIRead(bool) override {}
    void SetUIWrite(bool) override {}
    bool IsMoving() override { return false; }
    events::EventData<float, float> OnMoving{"FairingDeployStart"};   // :264
    events::EventData<float> OnStop{"FairingDeployEnd"};              // :266

    // IModuleInfo
    std::string GetModuleTitle() override;                    // :2186
    std::string GetPrimaryField() override;                   // :2196

    // IDynamicCargoOccluder (:1711): occlusion colliders belong to the model; only the collider-free call is a no-op
    void SetupOcclusionTest(bool testActive) override;
    // IActivateOnDecouple (:2031)
    void DecoupleAction(const std::string& nodeName, bool weDecouple) override;

    // st_flight_deployed
    bool IsDeployed() const { return fsmStateName_ == "st_flight_deployed"; }
    // The scene state machine is created by OnStart (editor: st_idle; flight: st_idle); `flightFsm_` tells which one runs.
    ModuleCargoBay* cargoModule = nullptr;                    // :186 NonSerialized

private:
    Part* PartOf() const;
    float panelMass_ = 0.0f;
    std::string fsmStateName_;
    bool fsmStarted_ = false;
    bool flightFsm_ = false;
    Part* interstage_ = nullptr;
    Vector3f originalCoP_{}, originalCoM_{}, originalCoL_{};
    bool onVesselModifiedSubscribed_ = false;
    bool applyVariantRequired_ = false;
};

}  // namespace ksp
