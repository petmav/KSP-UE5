// WP-25: CompoundParts/CModuleStrut.cs (see include/ksp/modules/module_strut.hpp). Every function cites CModuleStrut.cs:<line>.
#include "ksp/modules/module_strut.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_docking_node.hpp"
#include "ksp/parts/compound_part.hpp"
#include "ksp/parts/part.hpp"

namespace ksp {

using KSP::Localization::Localizer;

namespace {
Part* PartOfModule(PartModule& m) { return dynamic_cast<Part*>(m.part); }
}  // namespace

// Part.cs:1049 Rigidbody
Part* CModuleStrut::Rigidbody(Part* p) {
    while (p != nullptr && !p->hasRigidbody) p = p->parent;
    return p;
}

// CModuleStrut.cs:26
void CModuleStrut::OnTargetSet(Part* target) {
    Part* p = PartOfModule(*this);
    if (p == nullptr) throw NullReferenceException();
    if (Rigidbody(p) == nullptr || !DockingEnvironment::SceneIsFlight()) return;
    auto* cp = dynamic_cast<CompoundPart*>(p);
    if (cp == nullptr) throw NullReferenceException();
    jointTarget = Rigidbody(target);
    if (jointTarget == nullptr) throw NullReferenceException();  // target.Rigidbody.GetComponent<Part>() on a null Rigidbody
    jointRoot = Rigidbody(p);
    if (jointTarget != jointRoot) {
        targetNode_ = std::make_unique<AttachNode>();
        targetNode_->id = "Strut";
        targetNode_->attachedPart = jointTarget;
        targetNode_->srfAttachMeshName = cp->compoundFields.targetMeshColName.value_or("");  // null string -> ""
        targetNode_->nodeType = AttachNodeType::Surface;
        targetNode_->attachMethod = AttachNodeMethod::FIXED_JOINT;
        targetNode_->breakingForce = Get<float>("linearStrength");
        targetNode_->breakingTorque = Get<float>("angularStrength");
        const Vector3f vector = p->partTransform.TransformPoint(cp->compoundFields.targetPosition);
        targetNode_->position = jointTarget->partTransform.InverseTransformPoint(vector);
        targetNode_->orientation = jointTarget->partTransform.InverseTransformDirection((p->partTransform.position - vector).normalized());
        targetNode_->size = 1;
        targetNode_->ResourceXFeed = false;
        targetNode_->owner = jointRoot;
        strutJoint = StrutEnvironment::createJoint ? StrutEnvironment::createJoint(*jointRoot, *jointTarget, *targetNode_) : std::make_shared<int>(0);
    } else {
        log::Warning("[StrutConnector]: Both sides of the strut share the same parent rigidbody. No actual joint was created.");
        strutJoint = nullptr;
        targetNode_.reset();
    }
}

// CModuleStrut.cs:57
void CModuleStrut::OnTargetLost() {
    if (strutJoint != nullptr) {
        if (StrutEnvironment::destroyJoint) StrutEnvironment::destroyJoint(strutJoint);
        if (targetNode_ != nullptr) targetNode_->owner = nullptr;
    }
}

// CModuleStrut.cs:69
void CModuleStrut::OnStartFinished(StartState) {
    // base.OnStartFinished is empty
    Part* p = PartOfModule(*this);
    if (DockingEnvironment::SceneIsFlight() && StrutEnvironment::evaConstructionOpen && StrutEnvironment::evaConstructionOpen() && p != nullptr)
        if (auto* cp = dynamic_cast<CompoundPart*>(p)) cp->set_attachState(CompoundPart::AttachState::Attaching);
}

// CModuleStrut.cs:77 (float.ToString(): the default format "G")
std::string CModuleStrut::GetInfo() {
    Part* p = PartOfModule(*this);
    auto* cp = p != nullptr ? dynamic_cast<CompoundPart*>(p) : nullptr;
    if (cp == nullptr) throw NullReferenceException();
    return Localizer::Format("#autoLOC_216749", std::vector<std::string>{dotnet::FormatSingleF(cp->compoundFields.maxLength, 1)}) +
           Localizer::Format("#autoLOC_216999", std::vector<std::string>{dotnet::FormatSingleG(Get<float>("linearStrength")),
                                                                        dotnet::FormatSingleG(Get<float>("angularStrength"))});
}

// CModuleStrut.cs:97
std::string CModuleStrut::GetModuleDisplayName() { return cacheAutoLOC_217004; }

// CModuleStrut.cs:102
void CModuleStrut::CacheLocalStrings() { cacheAutoLOC_217004 = Localizer::Format("#autoLOC_217004"); }

namespace detail {
void RegisterStrutModules() {
    ModuleFactory::Register("CModuleStrut", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<CModuleStrut>(c)); });
}
}  // namespace detail

}  // namespace ksp
