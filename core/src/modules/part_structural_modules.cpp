// WP-13: ModuleMirrorPlacement, ModuleDisplaceTweak, CompoundPartModule (see the header).
#include "ksp/modules/part_structural_modules.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp {

namespace {
constexpr int kPartTweaked = 14;  // ConstructionEventType.PartTweaked
Part* PartOf(PartModule& m) { return dynamic_cast<Part*>(m.part); }
}  // namespace

// ModuleMirrorPlacement.cs:26
bool ModuleMirrorPlacement::OnWillBeMirrored(Quaternionf& rotation, AttachNode* selPartNode, Part* /*partParent*/) {
    if (!Get<bool>("ignoreMirrorIfSurfaceXOffset") && selPartNode != nullptr && selPartNode->nodeType == AttachNodeType::Surface &&
        selPartNode->orientation.x != 0.0f)
        return false;
    if (Get<bool>("applyMirrorRotationXAxis")) rotation = rotation * UnityNative::AngleAxis(Get<float>("mirrorRotationXAxis"), Vector3f::right());
    if (Get<bool>("applyMirrorRotationYAxis")) rotation = rotation * UnityNative::AngleAxis(Get<float>("mirrorRotationYAxis"), Vector3f::up());
    if (Get<bool>("applyMirrorRotationZAxis")) rotation = rotation * UnityNative::AngleAxis(Get<float>("mirrorRotationZAxis"), Vector3f::forward());
    return true;
}

// ModuleDisplaceTweak.cs:39
void ModuleDisplaceTweak::OnStart(StartState /*state*/) {
    const auto pos = findModelTransform ? findModelTransform(Get<std::string>("tgtTransformName")) : std::nullopt;
    if (!pos) return;
    tgtFound_ = true;
    p0_ = *pos;
    const Vector3f axis = Get<Vector3f>("axis");
    pMax_ = axis * Get<float>("displaceMax");
    pMin_ = axis * Get<float>("displaceMin");
    GameEvents::onEditorPartEvent.Add(this, &ModuleDisplaceTweak::OnPartEvent);
    subscribed_ = true;
    MDprev = nullptr;
    MDnext = nullptr;
    Part* p = PartOf(*this);
    mIndex_ = p->Modules().IndexOf(this);
    int num = 0;
    while (true) {
        if (num < static_cast<int>(p->Modules().Count())) {
            auto* m = dynamic_cast<ModuleDisplaceTweak*>(p->Modules().GetModule(static_cast<size_t>(num)));
            if (num < mIndex_ && m != nullptr) MDprev = m;
            if (num != mIndex_ && num > mIndex_ && m != nullptr) break;
            num++;
            continue;
        }
        return;
    }
    MDnext = dynamic_cast<ModuleDisplaceTweak*>(p->Modules().GetModule(static_cast<size_t>(num)));
}

// ModuleDisplaceTweak.cs:74
ModuleDisplaceTweak::~ModuleDisplaceTweak() {
    if (subscribed_) GameEvents::onEditorPartEvent.Remove(this, &ModuleDisplaceTweak::OnPartEvent);
}

// ModuleDisplaceTweak.cs:79
void ModuleDisplaceTweak::OnPartEvent(ConstructionEventType constructionEventType, Part* p) {
    Part* self = PartOf(*this);
    if (static_cast<int>(constructionEventType) == kPartTweaked &&
        (p == self || std::find(self->symmetryCounterparts.begin(), self->symmetryCounterparts.end(), p) != self->symmetryCounterparts.end())) {
        mD_ = (p != self) ? dynamic_cast<ModuleDisplaceTweak*>(p->Modules().GetModule(static_cast<size_t>(mIndex_))) : this;
        UpdateMorph(mD_->Get<float>("scalar"));
    }
}

// ModuleDisplaceTweak.cs:88
void ModuleDisplaceTweak::UpdateMorph(float t) {
    Set<float>("scalar", t);
    if (MDnext == nullptr && setLocalPosition) setLocalPosition(GetFinalPosition());
}

// ModuleDisplaceTweak.cs:97
Vector3f ModuleDisplaceTweak::GetFinalPosition() const {
    return ((MDprev != nullptr) ? MDprev->GetFinalPosition() : p0_) + Vector3f::Lerp(pMin_, pMax_, Get<float>("scalar"));
}

// CompoundParts/CompoundPartModule.cs:21 (CompoundPart.target is read through the CompoundPart implementation)
Part* CompoundPartModule::target() const {
    if (compoundPart == nullptr || !compoundTarget) throw NullReferenceException();
    return compoundTarget(*compoundPart);
}

// CompoundParts/CompoundPartModule.cs:23
void CompoundPartModule::OnAwake() {
    Part* p = PartOf(*this);
    compoundPart = (p != nullptr && p->className == "CompoundPart") ? p : nullptr;
    if (compoundPart == nullptr) log::Error("[CompoundPartModule]: CompoundPartModule requires a CompoundPart component!");
    OnModuleAwake();
}

namespace detail {
void RegisterStructuralModules() {
    ModuleFactory::Register("ModuleMirrorPlacement", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<ModuleMirrorPlacement>(c));
    });
    ModuleFactory::Register("ModuleDisplaceTweak", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<ModuleDisplaceTweak>(c));
    });
}
}  // namespace detail

}  // namespace ksp
