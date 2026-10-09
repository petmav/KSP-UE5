// WP-13: ModulePartVariants (ModulePartVariants.cs). See module_part_variants.hpp for what is hooked and what is skipped.
#include "ksp/modules/module_part_variants.hpp"

#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp {

using KSP::Localization::Localizer;

namespace {
Part* PartOf(PartModule& m) { return dynamic_cast<Part*>(m.part); }

std::shared_ptr<PartVariant> CloneVariant(const PartVariant& v, const Part* from, Part* to) {
    auto c = std::make_shared<PartVariant>(v);  // C++ copy = the Unity serialization clone
    for (AttachNode& n : c->attachNodes)
        if (n.owner == from) n.owner = to;
    if (c->srfAttachNode) {
        c->srfAttachNode = std::make_shared<AttachNode>(*c->srfAttachNode);
        if (c->srfAttachNode->owner == from) c->srfAttachNode->owner = to;
    }
    return c;
}
}  // namespace

// ModulePartVariants.cs:28
PartVariant* ModulePartVariants::SelectedVariant() {
    return variantList.at(static_cast<size_t>(Get<int32_t>("variantIndex"))).get();
}

// ModulePartVariants.cs:217
int ModulePartVariants::GetVariantIndex(const std::string& variantName) {
    int result = 0;
    for (size_t i = 0; i < variantList.size(); i++)
        if (variantList[i]->name == variantName) {
            result = static_cast<int>(i);
            break;
        }
    return result;
}

// ModulePartVariants.cs:237 (themeName == null compares equal only to a null name; "" is not null)
int ModulePartVariants::GetVariantThemeIndex(const std::string& variantThemeName) {
    int result = 0;
    for (size_t i = 0; i < variantList.size(); i++)
        if (variantList[i]->themeName && *variantList[i]->themeName == variantThemeName) {
            result = static_cast<int>(i);
            break;
        }
    return result;
}

// ModulePartVariants.cs:264
std::vector<std::string> ModulePartVariants::GetVariantNames() {
    std::vector<std::string> list;
    for (const auto& v : variantList) list.push_back(v->name);
    return list;
}

// ModulePartVariants.cs:274
bool ModulePartVariants::HasVariant(const std::string& variantName) {
    const auto names = GetVariantNames();
    return std::find(names.begin(), names.end(), variantName) != names.end();
}

// ModulePartVariants.cs:252
bool ModulePartVariants::SetVariant(const std::string& variantName) {
    if (!HasVariant(variantName)) {
        log::Error("Variant name does not exist to be set. Part:" + (part ? part->gameObjectName : std::string()) + " , VariantName: " + variantName);
        return false;
    }
    Set<int32_t>("variantIndex", GetVariantIndex(variantName));
    RefreshVariant();
    return true;
}

// ModulePartVariants.cs:292 (a null themeName stays null in C#; the list here carries "" for it)
std::vector<std::string> ModulePartVariants::GetVariantThemeNames() {
    std::vector<std::string> list;
    for (const auto& v : variantList) list.push_back(v->themeName.value_or(std::string()));
    return list;
}

// ModulePartVariants.cs:302
bool ModulePartVariants::HasVariantTheme(const std::string& variantThemeName) {
    const auto names = GetVariantThemeNames();
    return std::find(names.begin(), names.end(), variantThemeName) != names.end();
}

// ModulePartVariants.cs:279 (EditorLogic.fetch.SetBackup() is the editor's)
bool ModulePartVariants::SetVariantTheme(const std::string& variantThemeName) {
    if (!HasVariantTheme(variantThemeName)) {
        log::Error("Variant theme does not exist to be set. Part:" + (part ? part->gameObjectName : std::string()) + " , VariantName: " + variantThemeName);
        return false;
    }
    Set<int32_t>("variantIndex", GetVariantThemeIndex(variantThemeName));
    RefreshVariant();
    return true;
}

// ModulePartVariants.cs:307 (sic: `variantIndex > variantList.Count` lets variantIndex == Count through to the indexer)
void ModulePartVariants::RefreshVariant() {
    const int variantIndex = Get<int32_t>("variantIndex");
    if (variantIndex < 0 || variantIndex > static_cast<int>(variantList.size())) return;
    ApplyVariant(PartOf(*this), variantList.at(static_cast<size_t>(variantIndex)).get(), variantIndex);
    // jettison == null, or already jettisoned: the variant's cube is active; otherwise it is not
    if (moduleJettison == nullptr || moduleJettison->Get<bool>("isJettisoned")) SetCubes(true);
    else SetCubes(false);
}

// ModulePartVariants.cs:327
void ModulePartVariants::SetCubes(bool state) {
    Part* p = PartOf(*this);
    if (p == nullptr) throw NullReferenceException();
    if (Get<bool>("useMultipleDragCubes") && !p->DragCubes.Procedural()) {
        const std::string text = std::to_string(Get<int32_t>("variantIndex"));
        auto& cubes = p->DragCubes.Cubes();
        bool found = false;
        for (int i = static_cast<int>(cubes.size()); i-- > 0;)
            if (cubes[static_cast<size_t>(i)].Name() == text) {
                found = true;
                break;
            }
        if (found) {
            for (DragCube& c : cubes) c.SetWeight(0.0f);  // ResetCubeWeights(0f)
            if (state)
                for (DragCube& c : cubes)
                    if (c.Name() == text) c.SetWeight(1.0f);  // SetCubeWeight(text, 1f)
        } else if (SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT) {
            log::Error("[ModulePartVariants] Drag cube named " + text + " not found for multiple drag cubes for part " + p->gameObjectName + " !");
        }
    }
    if (hooks.forceDragCubeUpdate) hooks.forceDragCubeUpdate(*p);  // ForceUpdate(weights: true, occlusion: true, resetProcTiming: true)
}

// ModulePartVariants.cs:177
void ModulePartVariants::UpdateNode(AttachNode& partNode, const AttachNode& variantNode, Part* /*part*/) {
    UpdatePartPosition(partNode, variantNode);
    partNode.originalPosition = variantNode.originalPosition;
    partNode.position = variantNode.position;
    partNode.orientation = variantNode.orientation;
    partNode.originalOrientation = variantNode.originalOrientation;
    partNode.size = variantNode.size;
    // partNode.icon (the node's editor gizmo) is UI
}

// ModulePartVariants.cs:192
void ModulePartVariants::UpdatePartPosition(AttachNode& currentNode, const AttachNode& newNode) {
    if (currentNode.attachedPart == nullptr) return;
    Part* part = (currentNode.owner != nullptr) ? currentNode.owner->potentialParent : nullptr;
    Part* editorRoot = hooks.editorRootPart ? hooks.editorRootPart() : nullptr;
    if (currentNode.attachedPart != editorRoot && currentNode.attachedPart != part) {
        if (currentNode.owner != nullptr) {
            const Vector3f vector = currentNode.owner->partTransform.TransformPoint(newNode.originalPosition);
            const Vector3f vector2 = currentNode.owner->partTransform.TransformPoint(currentNode.originalPosition);
            const Vector3f translation = vector - vector2;
            currentNode.attachedPart->partTransform.position = currentNode.attachedPart->partTransform.position + translation;  // Translate(Space.World)
        }
    } else if (part != nullptr) {
        const Vector3f vector3 = currentNode.owner->partTransform.TransformPoint(newNode.originalPosition);
        const Vector3f translation2 = currentNode.owner->partTransform.TransformPoint(currentNode.originalPosition) - vector3;
        currentNode.owner->partTransform.position = currentNode.owner->partTransform.position + translation2;
    }
}

// ModulePartVariants.cs:48 (the editor-inventory branch, the model, materials and animations are hooks)
void ModulePartVariants::ApplyVariant(Part* part, PartVariant* variant, int variantIndex) {
    if (variant == nullptr) return;
    if (part != nullptr && hooks.applyModel) hooks.applyModel(*part, *variant, variantIndex);
    const GameScenes scene = SaveEnvironment::Current().loadedScene;
    if (scene == GameScenes::EDITOR || scene == GameScenes::LOADING) {
        if (part != nullptr && !variant->attachNodes.empty()) {
            for (size_t k = 0; k < part->attachNodes.size(); k++)
                for (size_t l = 0; l < variant->attachNodes.size(); l++)
                    if (part->attachNodes[k].id == variant->attachNodes[l].id) {
                        UpdateNode(part->attachNodes[k], variant->attachNodes[l], part);
                        break;
                    }
        }
        if (part != nullptr && part->srfAttachNode && variant->srfAttachNode) UpdateNode(*part->srfAttachNode, *variant->srfAttachNode, part);
    }
    if (part != nullptr) {
        if (hooks.applyAnimations) hooks.applyAnimations(*part, *variant);
        if (part->variants != nullptr) {
            ModulePartVariants& pv = *part->variants;
            for (size_t n = 0; n < pv.variantList.size(); n++) {
                for (size_t num = 0; num < pv.variantList[n]->disabledEvents.size(); num++) {
                    const std::vector<std::string> array = dotnet::Split(pv.variantList[n]->disabledEvents[num], ".", false);
                    PartModule* partModule = part->Modules()[array[0]];
                    if (partModule == nullptr) continue;
                    BaseEvent* baseEvent = partModule->Events()[array.at(1)];
                    if (baseEvent == nullptr) continue;
                    if (pv.variantList[n].get() == variant) {
                        if (!baseEvent->EventIsDisabledByVariant()) baseEvent->VariantToggleEventDisabled(true);
                    } else if (baseEvent->EventIsDisabledByVariant()) {
                        baseEvent->VariantToggleEventDisabled(false);
                    }
                }
            }
        }
        if (variant->attachRules) part->fields.attachRules = variant->attachRules;
        else part->fields.attachRules = part->baseVariant ? part->baseVariant->attachRules : std::nullopt;
    }
    GameEvents::onVariantApplied.Fire(part, variant);
    if (scene == GameScenes::EDITOR) GameEvents::onEditorVariantApplied.Fire(part, variant);
}

// ModulePartVariants.cs:348 (the renderer / material copies of the base variant need the model)
void ModulePartVariants::OnAwake() {
    PartModule::OnAwake();
    Part* p = PartOf(*this);
    if (p == nullptr) throw NullReferenceException();
    p->baseVariant = std::make_shared<PartVariant>(Localizer::Format("#autoLOC_8007005"), Localizer::Format("#autoLOC_8007005"),
                                                   p->attachNodes, p->srfAttachNode);
}

// ModulePartVariants.cs:364 (the UI_VariantSelector setup of the editor is UI)
void ModulePartVariants::OnStart(StartState /*state*/) {
    if (part != nullptr) moduleJettison = [&]() -> PartModule* {
        for (size_t i = 0; i < PartOf(*this)->Modules().Count(); i++)
            if (PartOf(*this)->Modules().GetModule(i)->IsA("ModuleJettison")) return PartOf(*this)->Modules().GetModule(i);
        return nullptr;
    }();
    RefreshVariant();
}

// ModulePartVariants.cs:380
void ModulePartVariants::OnLoad(const ConfigNode& node) {
    PartModule::OnLoad(node);
    Part* p = PartOf(*this);
    if (variantListNull) {
        if (p == nullptr || !p->baseVariant) throw NullReferenceException();
        variantListNull = false;
        variantList.clear();
        PartVariant& bv = *p->baseVariant;
        auto nonEmpty = [&](const char* k) -> std::optional<std::string> {
            auto v = node.GetValue(k);
            if (v && !v->empty()) return v;
            return std::nullopt;
        };
        if (auto v = nonEmpty("primaryColor")) bv.primaryColor = *v;
        if (auto v = nonEmpty("secondaryColor")) bv.secondaryColor = *v;
        if (auto v = nonEmpty("sizeGroup")) bv.sizeGroup = *v;
        if (auto v = nonEmpty("baseName")) bv.name = bv.displayName = *v;
        if (auto v = nonEmpty("baseDisplayName")) bv.displayName = *v;
        if (auto v = nonEmpty("baseMass")) {
            float r = 0.0f;
            cs::FloatTryParse(*v, r);
            bv.mass = r;
        }
        if (auto v = nonEmpty("baseCost")) {
            float r = 0.0f;
            cs::FloatTryParse(*v, r);
            bv.cost = r;
        }
        if (auto v = nonEmpty("baseThemeName")) bv.themeName = *v;
        bv.attachRules = p->fields.attachRules;
        for (ConfigNode* vn : node.GetNodes("VARIANT")) {
            auto pv = std::make_shared<PartVariant>(PartVariant::FromBase(*p->baseVariant));
            pv->Load(*vn);
            variantList.push_back(std::move(pv));
        }
        std::string baseVariantName;
        if (node.TryGetValue("baseVariant", baseVariantName)) {
            const int index = GetVariantIndex(baseVariantName);
            std::shared_ptr<PartVariant> partVariant2 = variantList.at(static_cast<size_t>(index));
            if (partVariant2) {
                p->baseVariant = partVariant2;
                Set<int32_t>("variantIndex", index);
            }
        } else {
            variantList.insert(variantList.begin(), p->baseVariant);
        }
    }
    std::string selected;
    node.TryGetValue("selectedVariant", selected);
    if (!selected.empty()) Set<int32_t>("variantIndex", GetVariantIndex(selected));
}

// ModulePartVariants.cs:465
void ModulePartVariants::OnSave(ConfigNode& node) {
    PartModule::OnSave(node);
    const int variantIndex = Get<int32_t>("variantIndex");
    if (variantIndex > 0 && variantIndex < static_cast<int>(variantList.size())) node.AddValue("selectedVariant", SelectedVariant()->name);
}

std::string ModulePartVariants::GetModuleDisplayName() { return Localizer::Format("#autoLOC_8007000"); }  // :474
std::string ModulePartVariants::GetModuleTitle() { return GetTitle(); }                                    // :479

// ModulePartVariants.cs:484
std::string ModulePartVariants::GetInfo() {
    std::string text;
    for (const auto& v : variantList) text += Localizer::Format(v->displayName) + "\n";
    return text;
}

// ModulePartVariants.cs:499
std::string ModulePartVariants::GetPrimaryField() {
    return Localizer::Format("#autoLOC_8007002", std::vector<std::string>{std::to_string(variantList.size())});
}

// ModulePartVariants.cs:509
std::vector<std::string> ModulePartVariants::GetDragCubeNames() {
    std::vector<std::string> array;
    for (size_t i = 0; i < variantList.size(); i++) array.push_back(std::to_string(i));
    return array;
}

// ModulePartVariants.cs:520
void ModulePartVariants::AssumeDragCubePosition(const std::string& name) {
    int32_t result = 0;
    if (cs::IntTryParse(name, result)) {
        Set<int32_t>("variantIndex", result);
        RefreshVariant();
    }
}

// ModulePartVariants.cs:535 (HighLogic.LoadedSceneIsEditor switches useVariantMass on, persistently)
float ModulePartVariants::GetModuleMass(float /*defaultMass*/, ModifierStagingSituation /*sit*/) {
    if (SaveEnvironment::Current().loadedScene == GameScenes::EDITOR) Set<bool>("useVariantMass", true);
    if (Get<bool>("useVariantMass")) return SelectedVariant()->mass;
    return 0.0f;
}

// ModulePartVariants.cs:553
float ModulePartVariants::GetModuleCost(float /*defaultCost*/, ModifierStagingSituation /*sit*/) { return SelectedVariant()->cost; }

// Unity clone: variantList is [SerializeField] public (the PartVariant objects are deep-copied)
void ModulePartVariants::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    const auto* o = dynamic_cast<const ModulePartVariants*>(&from);
    if (o == nullptr) return;
    variantListNull = o->variantListNull;
    variantList.clear();
    for (const auto& v : o->variantList) variantList.push_back(CloneVariant(*v, dynamic_cast<const Part*>(o->part), dynamic_cast<Part*>(part)));
}

namespace detail {
void RegisterModulePartVariants() {
    ModuleFactory::Register("ModulePartVariants", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<ModulePartVariants>(c));
    });
}
}  // namespace detail

}  // namespace ksp
