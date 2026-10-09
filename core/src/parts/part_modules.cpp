// WP-13: the module-facing half of Part (Part.cs): AddModule / RemoveModule / FindModuleImplementing, the Modules* lifecycle
// calls, copy semantics (Instantiate, OnWillBeCopied / OnCopy / OnWasCopied) and SendEvent. Spec: Part.cs line numbers in the
// comments; the module host is WP-06's (modules/part_module.hpp).
#include <algorithm>
#include <exception>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/module_part_variants.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"

namespace ksp {

// ---- modules -----------------------------------------------------------------------------------------------------------------
// Part.cs:5787 AddModule(string): class lookup, ModuleCargoPart / ModuleInventoryPart exclusivity, AddComponent (the
// constructor chain runs the [KSPField] initialisers), Awake (Unity runs Awake inside AddComponent on an active object; the
// prefab parts PartLoader compiles are active, the code-built ones are Awake'd by PartLoader.cs:511/:551 -- once either way)
PartModuleData* Part::AddModule(std::string_view moduleName, bool /*forceAwake*/) {
    const ModuleClassInfo* cls = GetPartModuleClass(moduleName);
    if (!cls) {
        log::Error("Cannot find a PartModule of typename '" + std::string(moduleName) + "'");
        return nullptr;
    }
    if (std::string_view(cls->name) == "ModuleCargoPart" && modules_.Contains("ModuleInventoryPart")) {
        log::Warning("[Part]: Cannot add " + std::string(moduleName) + " as this Part '" + gameObjectName +
                     "' already has a ModuleInventoryPart PartModule and they are mutually exclusive.");
        return nullptr;
    }
    if (std::string_view(cls->name) == "ModuleInventoryPart" && modules_.Contains("ModuleCargoPart")) {
        PartModule* cargo = FindModuleOfClass("ModuleCargoPart");
        if (cargo != nullptr && cargo->Get<float>("packedVolume") >= 0.0f) RemoveModule(modules_["ModuleCargoPart"]);
        log::Warning("[Part]: Cannot have ModuleCargoPart and ModuleInventoryPart on same Part [" + gameObjectName +
                     "'. Removed ModuleCargoPart");
    }
    PartModuleData m;
    m.moduleName = cls->name;  // PartModule.moduleName = ClassName
    m.moduleClass = cls;
    std::shared_ptr<PartModule> module = ModuleFactory::Create(*cls);
    module->part = this;
    module->Awake();
    modules_.Add(module);
    m.module = std::move(module);
    ModuleData.push_back(std::move(m));
    return &ModuleData.back();
}

// Part.cs:5823 AddModule(ConfigNode)
PartModuleData* Part::AddModule(ConfigNode& node, bool forceAwake) {
    const auto name = node.GetValue("name");
    if (!name) {
        log::Error("Cannot add a PartModule because the ConfigNode contains no module name");
        return nullptr;
    }
    PartModuleData* m = AddModule(*name, forceAwake);
    if (!m) return nullptr;
    m->moduleName = *name;
    LoadModuleData(*m, node, this);  // PartModule.Load: Fields.Load ... OnLoad
    return m;
}

// Part.cs:5843
PartModule* Part::LoadModule(ConfigNode& node, int& moduleIndex) {
    const auto name = node.GetValue("name");
    if (!name) {
        log::Error("Cannot load PartModule #" + std::to_string(moduleIndex) + " because ConfigNode contains no module name");
        return nullptr;
    }
    const std::string partName = partInfo ? partInfo->name : gameObjectName;
    PartModule* partModule = nullptr;
    if (moduleIndex >= static_cast<int>(modules_.Count())) {
        log::Warning("[Part]: PartModule " + *name + " at " + partName + ", index " + std::to_string(moduleIndex) +
                     ": index exceeds module count as defined in cfg.\nLooking for " + *name + " in other indices...");
    } else {
        partModule = modules_.GetModule(static_cast<size_t>(moduleIndex));
        if (partModule->moduleName != *name) {
            log::Warning("[Part]: PartModule indexing mismatch at " + partName + ", index " + std::to_string(moduleIndex) +
                         ".\nNode '" + *name + "' found in loaded data, but '" + partModule->moduleName + "' is defined in cfg.\nLooking for " +
                         *name + " in other indices...");
            partModule = nullptr;
        }
    }
    if (partModule == nullptr) {
        int num = -1;
        for (size_t i = 0; i < modules_.Count(); i++)
            if (modules_.GetModule(i)->moduleName == *name) {
                num = static_cast<int>(i);
                break;
            }
        if (num != -1) {
            log::Warning("..." + *name + " module found at index " + std::to_string(num) + ".");
            partModule = modules_.GetModule(static_cast<size_t>(num));
            moduleIndex++;
        } else {
            log::Warning("...no " + *name + " module found on part definition. Skipping...");
        }
    } else {
        moduleIndex++;
    }
    if (partModule == nullptr) return nullptr;
    partModule->Load(node);
    return partModule;
}

// Part.cs:5901
void Part::RemoveModule(PartModule* module) {
    if (module == nullptr || module->part != this) {
        log::Error("Cannot destroy a module which does not belong to this part");
        return;
    }
    for (auto it = ModuleData.begin(); it != ModuleData.end(); ++it)
        if (it->module.get() == module) {
            std::shared_ptr<PartModule> keepAlive = it->module;  // DestroyImmediate: the object dies after the lists drop it
            modules_.Remove(module);
            ModuleData.erase(it);
            return;
        }
    modules_.Remove(module);
}

// Part.cs:5913
void Part::RemoveModules() {
    modules_.Clear();
    ModuleData.clear();
}

// Part.cs:5930 FindModuleImplementing<T>() (the first module that `is T`; the C# type cache only memoises the answer)
PartModule* Part::FindModuleImplementing(const std::function<bool(PartModule&)>& is) {
    for (size_t i = 0; i < modules_.Count(); i++) {
        PartModule* m = modules_.GetModule(i);
        if (is(*m)) return m;
    }
    return nullptr;
}

PartModule* Part::FindModuleOfClass(std::string_view className) {
    return FindModuleImplementing([&](PartModule& m) { return m.IsA(className); });
}

// Part.cs:5957
bool Part::HasModuleImplementing(const std::function<bool(PartModule&)>& is) { return FindModuleImplementing(is) != nullptr; }

// Part.cs:5962
PartModule* Part::FindParentModuleImplementing(const std::function<bool(PartModule&)>& is) {
    if (parent != nullptr) {
        PartModule* val = parent->FindModuleImplementing(is);
        if (val != nullptr) return val;
        return parent->FindParentModuleImplementing(is);
    }
    return nullptr;
}

// Part.cs:5976
std::vector<PartModule*> Part::FindModulesImplementing(const std::function<bool(PartModule&)>& is) {
    std::vector<PartModule*> list;
    for (size_t j = 0; j < modules_.Count(); j++) {
        PartModule* m = modules_.GetModule(j);
        if (is(*m)) list.push_back(m);
    }
    return list;
}

// Part.cs:5569 (HighLogic scene flags and Vessel.situation are passed in)
StartState Part::GetModuleStartState(bool editor, bool flight, std::optional<VesselSituations> situation) const {
    auto bits = [](StartState s) { return static_cast<int>(s); };
    int startState = bits(StartState::None);
    if (editor) {
        startState |= bits(StartState::Editor);
    } else if (flight) {
        if (vessel == nullptr || !situation) return static_cast<StartState>(startState);
        const VesselSituations s = *situation;
        if (s == VesselSituations::PRELAUNCH) {
            startState |= bits(StartState::PreLaunch);
            startState |= bits(StartState::Landed);
        }
        if (s == VesselSituations::DOCKED) startState |= bits(StartState::Docked);
        if (s == VesselSituations::ORBITING || s == VesselSituations::ESCAPING) startState |= bits(StartState::Orbital);
        if (s == VesselSituations::SUB_ORBITAL) startState |= bits(StartState::SubOrbital);
        if (s == VesselSituations::SPLASHED) startState |= bits(StartState::Splashed);
        if (s == VesselSituations::FLYING) startState |= bits(StartState::Flying);
        if (s == VesselSituations::LANDED) startState |= bits(StartState::Landed);
    }
    return static_cast<StartState>(startState);
}

// Part.cs:5534
void Part::ModulesOnActivate() {
    for (size_t i = 0; i < modules_.Count(); i++) {
        PartModule* m = modules_.GetModule(i);
        try {
            if (m->StagingEnabled()) m->OnActive();
        } catch (const std::exception& ex) {
            log::Error("Module " + m->moduleName + " threw during OnActive " + ex.what());
        }
    }
}

// Part.cs:5553
void Part::ModulesOnDeactivate() {
    for (size_t i = 0; i < modules_.Count(); i++) {
        PartModule* m = modules_.GetModule(i);
        try {
            m->OnInactive();
        } catch (const std::exception& ex) {
            log::Error("Module " + m->moduleName + " threw during OnInactive: " + ex.what());
        }
    }
}

namespace {
// the `needPrefabMass` block shared by UpdateMass, ModulesOnStart and ModulesOnFixedUpdate (Part.cs:4034, 5633, 5724)
void InitPrefabMass(Part& p) {
    if (p.fields.needPrefabMass) {
        p.fields.prefabMass = p.fields.mass;
        if (p.partInfo != nullptr && p.partInfo->partPrefab != nullptr) p.fields.prefabMass = p.partInfo->partPrefab->fields.mass;
        p.fields.needPrefabMass = false;
    }
}
// `mass = prefabMass + num; if (mass < partInfo.MinimumMass) mass = partInfo.MinimumMass`
void ApplyMass(Part& p, float num) {
    p.fields.mass = f32store(fx(p.fields.prefabMass) + fx(num));
    if (p.partInfo == nullptr) throw NullReferenceException();
    if (p.fields.mass < p.partInfo->MinimumMass()) p.fields.mass = p.partInfo->MinimumMass();
}
}  // namespace

// Part.cs:5615 (ApplyUpgrades / AppliedUpgrades and the ShowUpgradeStats event are the part-upgrade system's, WP-32)
void Part::ModulesOnStart(StartState moduleStartState) {
    float num = 0.0f;
    InitPrefabMass(*this);
    for (size_t i = 0; i < modules_.Count(); i++) {
        PartModule* partModule = modules_.GetModule(i);
        try {
            partModule->OnStart(moduleStartState);
        } catch (const std::exception& ex) {
            log::Error("Module " + partModule->moduleName + " threw during OnStart: " + ex.what());
        }
        partModule->UpdateStagingToggle();
        if (IPartMassModifier* mm = partModule->AsMassModifier())
            num = f32store(fx(num) + fx(mm->GetModuleMass(fields.prefabMass, ModifierStagingSituation::CURRENT)));
    }
    ApplyMass(*this, num);
}

// Part.cs:5674
void Part::ModulesOnStartFinished(StartState moduleStartState) {
    for (size_t i = 0; i < modules_.Count(); i++) {
        PartModule* partModule = modules_.GetModule(i);
        try {
            partModule->OnStartFinished(moduleStartState);
        } catch (const std::exception& ex) {
            log::Error("Module " + partModule->moduleName + " threw during OnStartFinished: " + ex.what());
        }
    }
}

// Part.cs:5694
void Part::ModulesBeforePartAttachJoint(StartState moduleStartState) {
    for (size_t i = 0; i < modules_.Count(); i++) {
        PartModule* partModule = modules_.GetModule(i);
        try {
            partModule->OnStartBeforePartAttachJoint(moduleStartState);
        } catch (const std::exception& ex) {
            log::Error("Module " + partModule->moduleName + " threw during ModulesBeforePartAttachJoint: " + ex.what());
        }
    }
}

// Part.cs:5712
void Part::ModulesOnFixedUpdate() {
    float num = 0.0f;
    InitPrefabMass(*this);
    for (size_t i = 0; i < modules_.Count(); i++) {
        try {
            PartModule* partModule = modules_.GetModule(i);
            if (partModule->isEnabled) partModule->OnFixedUpdate();
            if (IPartMassModifier* mm = partModule->AsMassModifier())
                num = f32store(fx(num) + fx(mm->GetModuleMass(fields.prefabMass, ModifierStagingSituation::CURRENT)));
        } catch (const std::exception& ex) {
            log::Error("Module " + modules_.GetModule(i)->moduleName + " threw during OnFixedUpdate: " + ex.what());
        }
    }
    ApplyMass(*this, num);
}

// Part.cs:5751
void Part::ModulesOnUpdate() {
    for (size_t i = 0; i < modules_.Count(); i++) {
        try {
            if (modules_.GetModule(i)->isEnabled) modules_.GetModule(i)->OnUpdate();
        } catch (const std::exception& ex) {
            log::Error("Module " + modules_.GetModule(i)->moduleName + " threw during OnUpdate: " + ex.what());
        }
    }
}

// Part.cs:5770
void Part::InitializeModules() {
    for (size_t i = 0; i < modules_.Count(); i++) {
        try {
            modules_.GetModule(i)->OnInitialize();
            modules_.GetModule(i)->UpdateStagingToggle();
        } catch (const std::exception& ex) {
            log::Error("Module " + modules_.GetModule(i)->moduleName + " threw during OnInitialize: " + ex.what());
        }
    }
}

// ---- copy -----------------------------------------------------------------------------------------------------------------------
// UnityEngine.Object.Instantiate(original). Unity clones the GameObject: the serialized (public / [SerializeField]) fields of
// the Part and of every module, then calls Awake on the new components. Parent / children / symmetry links are runtime links
// and start empty here (a part with a child hierarchy would clone the hierarchy: not modelled; the editor's copy path in
// ShipConstruction builds the clone's links itself).
std::unique_ptr<Part> Part::Instantiate(const Part& original) {
    auto p = std::make_unique<Part>();
    p->className = original.className;
    p->gameObjectName = original.gameObjectName;
    p->fields = original.fields;
    p->compoundFields = original.compoundFields;
    p->DragCubes = original.DragCubes;
    p->partInfo = original.partInfo;
    p->vessel = original.vessel;
    p->fxGroupNames = original.fxGroupNames;
    p->effects.clear();
    for (const auto& e : original.effects) p->effects.push_back(e->CreateCopy());
    p->fxDefinitions = original.fxDefinitions;
    p->soundDefinitions = original.soundDefinitions;
    p->models = original.models;
    p->materialOverrides = original.materialOverrides;
    p->effectNodes_.clear();
    for (const auto& e : original.effectNodes_) p->effectNodes_.push_back(e->CreateCopy());
    p->partTransform = original.partTransform;
    p->state_ = original.state_;
    p->canFail_ = original.canFail_;
    p->modelRenderers = original.modelRenderers;
    p->colliderBounds = original.colliderBounds;
    p->hasRigidbody = original.hasRigidbody;
    // attach nodes: cloned; the owner link points at the new part, the attached part is kept (an external reference)
    for (const AttachNode& n : original.attachNodes) {
        AttachNode c = AttachNode::Clone(n);
        if (c.owner == &original) c.owner = p.get();
        if (c.attachedPart == &original) c.attachedPart = p.get();
        p->attachNodes.push_back(std::move(c));
    }
    if (original.srfAttachNode) {
        p->srfAttachNode = std::make_shared<AttachNode>(AttachNode::Clone(*original.srfAttachNode));
        if (p->srfAttachNode->owner == &original) p->srfAttachNode->owner = p.get();
    }
    // resources (PartResourceList is [Serializable]: the clone gets its own entries)
    p->Resources.CloneFrom(original.Resources, p.get());
    p->simulationResources_.CloneFrom(original.simulationResources_, p.get());
    // Part.baseVariant is a [Serializable] class instance: deep copied
    if (original.baseVariant) {
        p->baseVariant = std::make_shared<PartVariant>(*original.baseVariant);
        for (AttachNode& n : p->baseVariant->attachNodes)
            if (n.owner == &original) n.owner = p.get();
        if (p->baseVariant->srfAttachNode) {
            p->baseVariant->srfAttachNode = std::make_shared<AttachNode>(*p->baseVariant->srfAttachNode);
            if (p->baseVariant->srfAttachNode->owner == &original) p->baseVariant->srfAttachNode->owner = p.get();
        }
    }
    // modules: one component per prefab module, the serialized state copied (CopySerializedFrom), then the module's Awake
    for (size_t i = 0; i < original.ModuleData.size(); i++) {
        const PartModuleData& src = original.ModuleData[i];
        PartModuleData m;
        m.moduleName = src.moduleName;
        m.moduleClass = src.moduleClass;
        m.config = src.config ? src.config->CreateCopy() : nullptr;
        m.isEnabled = src.isEnabled;
        m.addedInCode = src.addedInCode;
        m.cfgFields = src.cfgFields;
        m.propellants = src.propellants;
        if (src.module && src.moduleClass) {
            m.module = ModuleFactory::Create(*src.moduleClass);
            m.module->part = p.get();
            m.module->CopySerializedFrom(*src.module);
            p->modules_.Add(m.module);
        }
        p->ModuleData.push_back(std::move(m));
    }
    // the Part's own Awake (PartModuleList ctor links the modules), then the modules' Awake in component order
    p->Awake();
    for (size_t i = 0; i < p->ModuleData.size(); i++) {
        if (p->ModuleData[i].module) p->ModuleData[i].module->Awake();
    }
    // the `variants` reference is remapped to the clone's module
    if (original.variants != nullptr)
        for (size_t i = 0; i < original.modules_.Count(); i++)
            if (static_cast<PartModule*>(original.variants) == original.modules_.GetModule(i) && i < p->modules_.Count())
                p->variants = dynamic_cast<ModulePartVariants*>(p->modules_.GetModule(i));
    return p;
}

// Part.cs:2102
void Part::OnWillBeCopied(bool asSymCounterpart) {
    onWillBeCopied(asSymCounterpart);
    for (size_t i = 0; i < modules_.Count(); i++) modules_.GetModule(i)->OnWillBeCopied(asSymCounterpart);
    for (size_t j = 0; j < children.size(); j++) children[j]->OnWillBeCopied(asSymCounterpart);
}

// Part.cs:2115 (sic: `newPart.modules.Count >= i` is always true for a module list of the same size, and reads past the end
// when it is shorter -- ArgumentOutOfRange in C#)
void Part::OnWasCopied(Part* newPart, bool asSymCounterpart) {
    onWasCopied(newPart, asSymCounterpart);
    for (size_t i = 0; i < modules_.Count(); i++) {
        PartModule* partModule = modules_.GetModule(i);
        PartModule* partModule2 = nullptr;
        if (newPart->modules_.Count() >= i) {
            partModule2 = newPart->modules_.GetModule(i);
            if (partModule->ClassName() != partModule2->ClassName()) partModule2 = nullptr;
        }
        modules_.GetModule(i)->OnWasCopied(partModule2, asSymCounterpart);
    }
    for (size_t j = 0; j < children.size(); j++) children[j]->OnWasCopied(newPart->children[j], asSymCounterpart);
}

// Part.cs:2138
void Part::OnCopy(Part* original, bool asSymCounterpart) {
    fields.isClone = true;
    originalPart = original;
    int count = Resources.Count();
    while (count-- > 0) {
        if (!original->Resources.Contains(Resources.At(count).info->id())) Resources.Remove(Resources.At(count).info->id());
    }
    bool flag = false;
    for (int i = 0; i < original->Resources.Count(); i++) {
        PartResource* partResource = Resources.Get(original->Resources.At(i).info->id());
        if (partResource == nullptr) {
            Resources.Add(original->Resources.At(i));
            simulationResources_.Add(original->simulationResources_.At(i));
        } else {
            partResource->amount = original->Resources.At(i).amount;
            partResource->maxAmount = original->Resources.At(i).maxAmount;
            flag = true;
        }
    }
    if (flag) ResetSimulationResources();
    int count2 = static_cast<int>(original->modules_.Count());
    while (count2-- > 0) modules_.GetModule(static_cast<size_t>(count2))->Copy(*original->modules_.GetModule(static_cast<size_t>(count2)));
    if (original->variants != nullptr && original->variants->SelectedVariant() != nullptr && variants != nullptr)
        variants->SetVariant(original->variants->SelectedVariant()->name);
    onCopy(original, asSymCounterpart);
    for (size_t j = 0; j < children.size(); j++) children[j]->OnCopy(original->children[j], asSymCounterpart);
}

// Part.cs:2187
bool Part::OnWillBeMirrored(Quaternionf& rotation, AttachNode* selPartNode, Part* partParent) {
    bool flag = false;
    for (size_t i = 0; i < modules_.Count(); i++) flag = flag || modules_.GetModule(i)->OnWillBeMirrored(rotation, selPartNode, partParent);
    return flag;
}

// ---- events ---------------------------------------------------------------------------------------------------------------------
// Part.cs:6207 / 6217 / 6227 / 6234 (the Part's own BaseEventList is not modelled: only module events are collected)
void Part::SendEvent(std::string_view eventName, BaseEventDetails* data, int maxDepth) {
    std::vector<BaseEvent*> list;
    CreateEventListRecursive(MonoStringHash(eventName), NewRequestID(), list, 0, maxDepth);
    for (BaseEvent* e : list) {
        if (data) e->Invoke(data);
        else e->Invoke();
    }
}

void Part::CreateEventListRecursive(int eventID, int requestID, std::vector<BaseEvent*>& eventList, int depth, int maxDepth) {
    if (AlreadyProcessedRequest(requestID)) return;
    for (size_t i = 0; i < modules_.Count(); i++) {
        BaseEvent* baseEvent = modules_.GetModule(i)->Events()[eventID];
        if (baseEvent != nullptr && baseEvent->active) eventList.push_back(baseEvent);
    }
    if (depth == maxDepth) return;
    depth++;
    if (parent != nullptr) parent->CreateEventListRecursive(eventID, requestID, eventList, depth, maxDepth);
    for (size_t j = 0; j < children.size(); j++) children[j]->CreateEventListRecursive(eventID, requestID, eventList, depth, maxDepth);
    for (size_t k = 0; k < fuelLookupTargets.size(); k++) fuelLookupTargets[k]->CreateEventListRecursive(eventID, requestID, eventList, depth, maxDepth);
    for (size_t l = 0; l < attachNodes.size(); l++) {
        const AttachNode& an = attachNodes[l];
        if (an.attachedPart != nullptr) an.attachedPart->CreateEventListRecursive(eventID, requestID, eventList, depth, maxDepth);
    }
}

}  // namespace ksp
