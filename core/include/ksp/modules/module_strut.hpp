// WP-25: CompoundParts/CModuleStrut.cs -- the strut connector module of a CompoundPart. KSPFields linearStrength / angularStrength (schema driven),
// OnTargetSet (target resolution: the rigidbody owners of both ends, the "Strut" AttachNode, PartJoint.Create), OnTargetLost, OnStartFinished, GetInfo.
//
// Seam (StrutEnvironment): the PartJoint (WP-20) and the EVA construction controller. The rigidbody owner walk (Part.Rigidbody -> parent chain) and the
// AttachNode the joint is built from (position / orientation in the target's frame, breaking force / torque from the two strengths) are ported, so the
// joint package only has to turn the node into a joint.
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "ksp/modules/part_structural_modules.hpp"
#include "ksp/parts/part_data.hpp"

namespace ksp {

struct StrutEnvironment {
    // PartJoint.Create(jointRoot, jointTarget, targetNode, null, AttachModes.SRF_ATTACH); DestroyJoint. Unset: a marker object / nothing.
    static inline std::function<std::shared_ptr<void>(Part& jointRoot, Part& jointTarget, AttachNode& node)> createJoint;
    static inline std::function<void(std::shared_ptr<void>&)> destroyJoint;
    // EVAConstructionModeController.Instance.IsOpen. Unset: false.
    static inline std::function<bool()> evaConstructionOpen;
};

// CModuleStrut.cs:6
class CModuleStrut : public CompoundPartModule, public IModuleInfo {
public:
    using CompoundPartModule::CompoundPartModule;
    std::shared_ptr<void> strutJoint;         // :14 PartJoint
    Part* jointRoot = nullptr;                // :16
    Part* jointTarget = nullptr;              // :18
    const AttachNode* TargetNode() const { return targetNode_.get(); }  // :20 private

    void OnTargetSet(Part* target) override;  // :26
    void OnTargetLost() override;             // :57
    void OnStartFinished(StartState state) override;  // :69
    std::string GetInfo() override;           // :77
    std::string GetModuleTitle() override { return "Strut Connector"; }  // :82
    std::string GetPrimaryField() override { return ""; }                // :92
    std::string GetModuleDisplayName() override;                         // :97
    IModuleInfo* AsModuleInfo() override { return this; }
    static void CacheLocalStrings();                                     // :102
    // Part.Rigidbody (Part.cs:1049): the part itself when it has a rigidbody, else the nearest ancestor with one (nullptr = no rigidbody)
    static Part* Rigidbody(Part* p);

private:
    std::unique_ptr<AttachNode> targetNode_;
    static inline std::string cacheAutoLOC_217004;
};

namespace detail {
void RegisterDockingModules();   // ModuleDockingNode
void RegisterGrappleModules();   // ModuleGrappleNode
void RegisterStrutModules();     // CModuleStrut
}  // namespace detail

}  // namespace ksp
