// WP-13: ModulePartVariants (ModulePartVariants.cs) -- the part-variant module: variant list loading, selection by name /
// theme, ApplyVariant (attach nodes, attach rules, disabled events, GameEvents), the mass and cost modifiers, module info.
//
// Model-bound steps (variant GameObject visibility, material copies, the animation toggle of ModuleAnimateGeneric, the drag
// cube refresh) go through hooks the model / aero packages install (`ApplyVariantHooks`); without them they are skipped.
// The editor UI (UI_VariantSelector callbacks, EditorLogic.SetBackup) is not ported.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/modules/part_module.hpp"
#include "ksp/parts/available_part.hpp"

namespace ksp {

class Part;

class ModulePartVariants : public PartModule, public IPartMassModifier, public IPartCostModifier, public IModuleInfo {
public:
    explicit ModulePartVariants(const ModuleClassInfo& cls) : PartModule(cls) {}

    // ModulePartVariants.cs:9 variantList (null until OnLoad builds it: `variantListNull`)
    std::vector<std::shared_ptr<PartVariant>> variantList;
    bool variantListNull = true;
    PartModule* moduleJettison = nullptr;  // :26 internal ModuleJettison

    // What ApplyVariant does to the model and to modules outside this package
    struct ApplyVariantHooks {
        // variant.UpdateModel(meshRoot) + the Materials copy (CopyPropertiesFromMaterial): needs the model
        std::function<void(Part&, const PartVariant&, int variantIndex)> applyModel;
        // ModuleAnimateGeneric.VariantToggleAnimationDisabled for the variant's DisabledAnimations (WP-27)
        std::function<void(Part&, const PartVariant&)> applyAnimations;
        // DragCubeList.ForceUpdate(weights: true, occlusion: true, resetProcTiming: true) after SetCubes (WP-21)
        std::function<void(Part&)> forceDragCubeUpdate;
        // EditorLogic.RootPart (UpdatePartPosition)
        std::function<Part*()> editorRootPart;
    };
    static inline ApplyVariantHooks hooks;

    // ---- selection ----
    PartVariant* SelectedVariant();                        // :28 variantList[variantIndex] (std::out_of_range like C#)
    bool IsMultipleCubesActive() const { return Get<bool>("useMultipleDragCubes"); }  // :30
    int GetVariantIndex(const std::string& variantName);   // :217 (0 when absent)
    int GetCurrentVariantIndex() const { return Get<int32_t>("variantIndex"); }  // :232
    int GetVariantThemeIndex(const std::string& variantThemeName);  // :237 (private)
    bool SetVariant(const std::string& variantName);       // :252
    std::vector<std::string> GetVariantNames();            // :264
    bool HasVariant(const std::string& variantName);       // :274
    bool SetVariantTheme(const std::string& variantThemeName);  // :279
    std::vector<std::string> GetVariantThemeNames();       // :292 (null theme names are written as empty: NullableString)
    bool HasVariantTheme(const std::string& variantThemeName);  // :302
    void RefreshVariant();                                 // :307 (private)
    void SetCubes(bool state);                             // :327

    // ApplyVariant (:43 / :48). `part` may be null; the model parameters are the hook's. In the editor / loading scene the part's
    // attach nodes take the variant's.
    static void ApplyVariant(Part* part, PartVariant* variant, int variantIndex = -1);
    static void UpdatePartPosition(AttachNode& currentNode, const AttachNode& newNode);  // :192
    static void UpdateNode(AttachNode& partNode, const AttachNode& variantNode, Part* part);  // :177 (private)

    // ---- PartModule overrides ----
    void OnAwake() override;                               // :348
    void OnStart(StartState state) override;               // :364
    void OnLoad(const ConfigNode& node) override;          // :380
    void OnSave(ConfigNode& node) override;                // :465
    std::string GetModuleDisplayName() override;           // :474
    std::string GetModuleTitle() override;                 // :479
    std::string GetInfo() override;                        // :484
    std::string GetPrimaryField() override;                // :499
    static std::string GetTitle() { return "PartVariants"; }  // :504
    std::vector<std::string> GetDragCubeNames();           // :509 (IMultipleDragCube)
    void AssumeDragCubePosition(const std::string& name);  // :520
    bool UsesProceduralDragCubes() const { return Get<bool>("useProceduralDragCubes"); }  // :530

    // IPartMassModifier / IPartCostModifier (:535-562)
    IPartMassModifier* AsMassModifier() override { return this; }
    IPartCostModifier* AsCostModifier() override { return this; }
    IModuleInfo* AsModuleInfo() override { return this; }
    float GetModuleMass(float defaultMass, ModifierStagingSituation sit) override;
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::FIXED; }
    float GetModuleCost(float defaultCost, ModifierStagingSituation sit) override;
    ModifierChangeWhen GetModuleCostChangeWhen() override { return ModifierChangeWhen::FIXED; }

    void CopySerializedFrom(const PartModule& from) override;  // Unity clone: the [SerializeField] variantList is deep-copied
};

namespace detail {
void RegisterModulePartVariants();  // called by RegisterBuiltinModules
}

}  // namespace ksp
