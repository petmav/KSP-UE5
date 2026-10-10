// WP-25: ModuleGrappleNode.cs (the Advanced Grabbing Unit "claw"): KSPFields (schema driven), OnLoad / OnSave (grapple pos / orientation, both
// DockedVesselInfo nodes), OnStart, the KerbalFSM (Ready / Grappled / Grappled (same vessel) / Disengage / Disabled), Grapple (vessel merge through
// Part.Couple), Release (Part.Undock with the saved DockedVesselInfo), GrappleSameVessel, Decouple, the pivot (ActiveJointPivot) mode switching,
// NodeIsTooFar / OtherPartIsLost.
//
// Seams (GrappleEnvironment + DockingEnvironment): the contact Physics.Raycast, the ActiveJointPivot (WP-20: IGrapplePivot), the same-vessel PartJoint,
// KerbalEVA.OnGrapple, FlightInputHandler / FlightCamera, AnalyticsUtil (dropped). Not ported: Missions adjusters (AdjusterGrappleNodeBase: nothing blocks
// grab / release), Debug.DrawRay, analytics.
//
// kspcore deviation: Part.attachNodes is a value vector here (PartPrefab::attachNodes), so the C# reference `grappleNode` is kept in the module until it is
// added to the owner part, after which GrappleNode() finds it by its id "grapple" in that part's list. OnLoad reserves one slot in flight so the pointers
// other code holds into the list stay valid when the node is added.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/fsm/kerbal_fsm.hpp"
#include "ksp/modules/module_docking_node.hpp"
#include "ksp/parts/docked_vessel_info.hpp"
#include "ksp/parts/part_data.hpp"

namespace ksp {

class ModuleGrappleNode;

// ActiveJoint.DriveMode (ActiveJoint.cs:16)
enum class ActiveJointDriveMode : int { NoJoint, Park, Neutral, Drive };

// ActiveJointPivot as ModuleGrappleNode uses it (the joint itself is WP-20's)
class IGrapplePivot {
public:
    virtual ~IGrapplePivot() = default;
    virtual bool IsValid() = 0;                                  // isValid
    virtual ActiveJointDriveMode DriveMode() = 0;                // driveMode
    virtual void SetDriveMode(ActiveJointDriveMode mode) = 0;    // SetDriveMode
    virtual void SetPivotAngleLimit(float degrees) = 0;          // SetPivotAngleLimit
    virtual void Terminate() = 0;                                // Terminate
    virtual Quaternionf GetLocalPivotRotation() = 0;             // GetLocalPivotRotation
};

struct GrappleEnvironment {
    // The contact ray of FindContactParts: Physics.Raycast(nodeTransform.position, nodeTransform.forward * captureRange, out hit, captureRange,
    // LayerUtil.DefaultEquivalent) + FlightGlobals.GetPartUpwardsCached(hit.transform.gameObject). nullopt = nothing hit. Unset: nothing hit.
    struct Hit {
        Part* part = nullptr;
        Vector3f point, normal;
    };
    static inline std::function<std::optional<Hit>(ModuleGrappleNode&, const Vector3f& origin, const Vector3f& direction, float distance)> raycast;
    // ActiveJointPivot.Create(this, grappleNode); unset: no pivot joint
    static inline std::function<std::shared_ptr<IGrapplePivot>(ModuleGrappleNode&, AttachNode*)> createPivot;
    // PartJoint.Create(part, other, grappleNode, null, AttachModes.SRF_ATTACH) / DestroyJoint. Unset: a marker object.
    static inline std::function<std::shared_ptr<void>(ModuleGrappleNode&, Part& other)> createSameVesselJoint;
    static inline std::function<void(ModuleGrappleNode&, std::shared_ptr<void>&)> destroySameVesselJoint;
    // otherPart.GetComponent<KerbalEVA>().OnGrapple() and FlightInputHandler.ResumeVesselCtrlState(vessel), FlightCamera.MinDistanceBackaway()
    static inline std::function<void(Part&)> evaOnGrapple;
    static inline std::function<void(Vessel*)> resumeVesselCtrlState;
    static inline std::function<void()> minDistanceBackaway;
    // nodeTransform.rotation = q (updateGrappleTransform): the node Transform write
    static inline std::function<void(ModuleGrappleNode&, const Quaternionf&)> setNodeRotation;
};

class ModuleGrappleNode : public PartModule {
public:
    explicit ModuleGrappleNode(const ModuleClassInfo& cls);
    ~ModuleGrappleNode() override;

    // ---- non-KSPField state (:41-:92) ----
    std::optional<DockedVesselInfo> vesselInfo;       // :41
    std::optional<DockedVesselInfo> otherVesselInfo;  // :43
    std::string state = "Ready";                      // :45
    uint32_t dockedPartUId = 0;                       // :51
    Part* otherPart = nullptr;                        // :53
    ModuleAnimateGeneric* deployAnimator = nullptr;   // :55
    bool originalDeployAnimatorAllowManualControl = true;
    Vector3f rGrabPoint, grabPos, grabOrt, grabOrt2;  // :63, :69-:73
    GrappleEnvironment::Hit hit;                      // :61
    std::shared_ptr<IGrapplePivot> pivotJoint;        // :75
    std::shared_ptr<void> GrappleJoint;               // :77 (PartJoint)
    bool grappledSameVessel = false;                  // :79
    std::unique_ptr<KerbalFSM> fsm;                   // :81
    std::unique_ptr<KFSMState> st_ready, st_grappled, st_grappled_sameVessel, st_disengage, st_disabled;
    std::unique_ptr<KFSMEvent> on_nodeDistance, on_nodeLost, on_contact, on_undock, on_sameVessel_disconnect, on_disable, on_enable;
    bool nodeTransformPresent = false;
    std::string controlTransformName_;

    AttachNode* GrappleNode();                        // :49 `grappleNode` (see the header comment)
    bool HasGrappleNode() const { return grappleNode_ != nullptr || grappleRegistered_; }
    bool IsDisabled();                                // :110

    void OnAwake() override;
    void OnLoad(const ConfigNode& node) override;     // :122
    void OnSave(ConfigNode& node) override;           // :163
    void OnStart(StartState st) override;             // :184
    void lateFSMStart(StartState st);                 // :248 (after the first yield)
    void SetupFSM();                                  // :258
    void Update();                                    // :483
    void FixedUpdate();                               // :492
    void LateUpdate();                                // :500
    static void TickAll();                            // one Unity frame of the coroutines
    Part* FindContactParts();                         // :512
    bool CheckGrappleContact(const DockPose& nodeT, const GrappleEnvironment::Hit& h, float minDist, float minFwdDot);  // :527
    void Grapple(Part* other, Part* dockerSide);      // :536
    void updateGrappleTransform();                    // :654
    void ReleaseAction(KSPActionParam& param);        // :663
    void Release();                                   // :672
    void WaitAndSwitchFocus();                        // :713
    void GrappleSameVessel(Part* other);              // :722
    void DestroySameVesselJoint();                    // :743
    void ReleaseSameVessel();                         // :757
    void OnOtherNodeSameVesselDisconnect();           // :762
    bool OtherPartIsLost();                           // :767
    bool NodeIsTooFar();                              // :782
    void DecoupleAction(KSPActionParam& param);       // :788
    void Decouple();                                  // :789
    void MakeReferenceToggle(KSPActionParam& act);    // :826
    void MakeReferenceTransform();                    // :832
    void SetLoose();                                  // :839
    void LockPivot();                                 // :848
    bool IsLoose();                                   // :857
    Part* GetHostPart();                              // :866 (IActiveJointHost)
    void OnJointInit(IGrapplePivot* joint);           // :871
    void OnDriveModeChanged(ActiveJointDriveMode mode);  // :883
    bool IsJointUnlocked();                           // :911 (IJointLockState)
    std::string GetContractObjectiveType() { return "Grapple"; }  // :919
    bool CheckContractObjectiveValidity() { return true; }        // :924
    std::string GetInfo() override;                   // :903
    std::string GetModuleDisplayName() override;      // :992
    DockPose NodePose();
    Part* PartOf() const;

private:
    struct Delayed {
        int ticks;
        std::function<void()> fn;
    };
    void StartCoroutine(int ticks, std::function<void()> fn) { delayed_.push_back({ticks, std::move(fn)}); }
    int CountAdjustersBlockingGrappleRelease() { return 0; }   // :970 (no Missions adjusters)
    bool IsAdjusterBlockingGrappleGrab() { return false; }      // :982
    void AddGrappleNodeTo(Part& owner);
    void RemoveGrappleNodeFrom(Part& owner);

    std::unique_ptr<AttachNode> grappleNode_;  // before it is added to a part's attachNodes
    Part* grappleOwner_ = nullptr;             // the part whose attachNodes holds the node (after Add)
    bool grappleRegistered_ = false;
    std::vector<Delayed> delayed_;
    static inline std::vector<ModuleGrappleNode*> live_;
};

}  // namespace ksp
