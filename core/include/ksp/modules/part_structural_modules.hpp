// WP-13: the small structural PartModules of the part package: ModuleMirrorPlacement (ModuleMirrorPlacement.cs),
// ModuleDisplaceTweak (ModuleDisplaceTweak.cs), CompoundPartModule (CompoundParts/CompoundPartModule.cs) and
// ICompoundPartAnchorMethod (CompoundParts/ICompoundPartAnchorMethod.cs). CompoundPart itself (CompoundPart.cs) is WP-26.
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "ksp/modules/part_module.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
enum class ConstructionEventType : int;  // ConstructionEventType.cs (PartTweaked = 14)

// ModuleMirrorPlacement.cs
class ModuleMirrorPlacement : public PartModule {
public:
    using PartModule::PartModule;
    // :26 OnWillBeMirrored: the mirrored copy is rotated by the configured angles about the part's X / Y / Z axes
    bool OnWillBeMirrored(Quaternionf& rotation, AttachNode* selPartNode, Part* partParent) override;
};

// ModuleDisplaceTweak.cs: moves a model transform along `axis` between displaceMin and displaceMax. The transform is the
// model's: `findModelTransform` returns its local position (nullopt: not found) and `setLocalPosition` writes it.
class ModuleDisplaceTweak : public PartModule {
public:
    using PartModule::PartModule;
    std::function<std::optional<Vector3f>(const std::string&)> findModelTransform;  // Part.FindModelTransform(name).localPosition
    std::function<void(const Vector3f&)> setLocalPosition;                         // tgtTrf.localPosition = ...
    void OnStart(StartState state) override;                                       // :39
    ~ModuleDisplaceTweak() override;                                               // :74 OnDestroy
    // :79 OnPartEvent (GameEvents.onEditorPartEvent)
    void OnPartEvent(ConstructionEventType constructionEventType, Part* p);
    void UpdateMorph(float t);                                                     // :88
    Vector3f GetFinalPosition() const;                                             // :97
    ModuleDisplaceTweak* MDprev = nullptr;                                         // :33 private
    ModuleDisplaceTweak* MDnext = nullptr;

private:
    bool tgtFound_ = false;
    Vector3f p0_{}, pMax_{}, pMin_{};
    int mIndex_ = 0;
    ModuleDisplaceTweak* mD_ = nullptr;
    bool subscribed_ = false;
};

// ICompoundPartAnchorMethod.cs
class ICompoundPartAnchorMethod {
public:
    virtual ~ICompoundPartAnchorMethod() = default;
    virtual void OnAttachmentUpdate() = 0;
};

// CompoundPartModule.cs (abstract): OnAwake is sealed and finds the CompoundPart the module sits on
class CompoundPartModule : public PartModule {
public:
    using PartModule::PartModule;
    Part* compoundPart = nullptr;                    // :9 (null unless the part is a CompoundPart)
    Part* target() const;                            // :21 _compoundPart.target (CompoundPart.target: WP-26)
    void OnAwake() final;                            // :23
    virtual void OnModuleAwake() {}                  // :33
    virtual void OnTargetSet(Part* target) = 0;      // :37
    virtual void OnTargetLost() = 0;                 // :39
    virtual void OnPreviewAttachment(const Vector3f&, const Vector3f&, const Quaternionf&) {}  // :41
    virtual void OnPreviewEnd() {}                   // :45
    virtual void OnTargetUpdate() {}                 // :49
    // CompoundPart.target (CompoundPart.cs): set by the CompoundPart implementation
    std::function<Part*(const Part&)> compoundTarget;
};

namespace detail {
void RegisterStructuralModules();  // called by RegisterBuiltinModules
}

}  // namespace ksp
