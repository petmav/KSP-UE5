// WP-13: the live-Part paths of ProtoPartSnapshot / ProtoPartModuleSnapshot / ProtoPartResourceSnapshot
// (ProtoPartSnapshot.cs:144, 499, 651, 761; ProtoPartModuleSnapshot.cs:14, 55, 67; ProtoPartResourceSnapshot.cs:14, 73).
// The WP-04 file (proto_vessel.cpp) keeps the file-form halves. Vessel-level paths (ProtoPartSnapshot.Load(Vessel, ...), Init)
// belong to WP-14 / WP-19.
#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/module_part_variants.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp {

using GE = events::GameEvents;

// ---- ProtoPartResourceSnapshot -------------------------------------------------------------------------------------------
// ProtoPartResourceSnapshot.cs:14
ProtoPartResourceSnapshot::ProtoPartResourceSnapshot(PartResource& resource) {
    resourceRef = &resource;
    resourceName = resource.info->name();
    resourceValues_ = std::make_shared<ConfigNode>("RESOURCE");
    resource.Save(*resourceValues_);
    amount = resource.amount;
    maxAmount = resource.maxAmount;
    flowState = resource.flowState;
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    definition = lib ? lib->GetDefinition(*resourceName) : nullptr;
}

// ProtoPartResourceSnapshot.cs:73
void ProtoPartResourceSnapshot::Load(Part& hostPart) {
    resourceValues_->SetValue("flowState", flowState, true);
    resourceValues_->SetValue("amount", amount, true);
    resourceValues_->SetValue("maxAmount", maxAmount, true);
    hostPart.SetResource(*resourceValues_);
}

// ---- ProtoPartModuleSnapshot -----------------------------------------------------------------------------------------------
// ProtoPartModuleSnapshot.cs:14 (the module's vessel is the part's: the values are saved now for cargo parts and for vessels
// that are not loaded / are unloading / backing up -- the Vessel hook decides)
ProtoPartModuleSnapshot::ProtoPartModuleSnapshot(PartModule& module) {
    moduleRef = &module;
    moduleName = module.ClassName();
    moduleValues = std::make_shared<ConfigNode>("MODULE");
    Part* part = dynamic_cast<Part*>(module.part);
    bool saveNow = false;
    if (part != nullptr && part->isCargoPart()) saveNow = true;
    if (part != nullptr && part->vessel != nullptr && Part::vesselHooks.snapshotSavesModules && Part::vesselHooks.snapshotSavesModules(*part->vessel))
        saveNow = true;
    if (saveNow) {
        module.Save(*moduleValues);
        hasSaved_ = true;
    }
    module.snapshot = this;
    if (hasSaved_) GE::onProtoPartModuleSnapshotSave.Fire(GE::FromToAction<ProtoPartModuleSnapshot*, ConfigNode*>(this, nullptr));
}

// ProtoPartModuleSnapshot.cs:55
PartModule* ProtoPartModuleSnapshot::Load(Part& hostPart, int& moduleIndex) {
    GE::onProtoPartModuleSnapshotLoad.Fire(GE::FromToAction<ProtoPartModuleSnapshot*, ConfigNode*>(this, nullptr));
    PartModule* partModule = hostPart.LoadModule(*moduleValues, moduleIndex);
    if (partModule == nullptr) {
        log::Warning("PartModule is null.");
        return nullptr;
    }
    partModule->snapshot = this;
    moduleRef = partModule;
    return partModule;
}

// ProtoPartModuleSnapshot.cs:67
void ProtoPartModuleSnapshot::EnsureModuleValuesIsInitialized() {
    if (!hasSaved_ && moduleRef != nullptr) {
        moduleRef->Save(*moduleValues);
        hasSaved_ = true;
    }
}

// ---- ProtoPartSnapshot -------------------------------------------------------------------------------------------------------
// ProtoPartSnapshot.cs:149 (the three-argument form; :144 passes preCreate = false)
ProtoPartSnapshot::ProtoPartSnapshot(Part& PartRef, ProtoVessel* protoVessel, bool /*preCreate*/) {
    SaveEnvironment& env = SaveEnvironment::Current();
    pVesselRef = protoVessel;
    partRef = &PartRef;
    partRef->onBackup();
    if (PartRef.partInfo == nullptr) throw NullReferenceException();
    partName = PartRef.partInfo->name;
    craftID = PartRef.fields.craftID;
    flightID = PartRef.fields.flightID;
    missionID = PartRef.fields.missionID;
    launchID = PartRef.fields.launchID;
    persistentId = PartRef.fields.persistentId;
    sameVesselCollision = PartRef.fields.sameVesselCollision;
    partInfo = env.getPartInfoByName ? env.getPartInfoByName(*partName) : nullptr;
    partInfoFound = partInfo != nullptr;
    mirror = PartRef.fields.mirrorVector;
    symMethod = static_cast<SymmetryMethod>(PartRef.fields.symMethod);
    inverseStageIndex = PartRef.fields.inverseStage;
    resourcePriorityOffset = PartRef.fields.resourcePriorityOffset;
    defaultInverseStage = PartRef.fields.defaultInverseStage;
    seqOverride = PartRef.fields.manualStageOffset;
    inStageIndex = PartRef.fields.inStageIndex;
    separationIndex = PartRef.fields.separationIndex;
    customPartData = PartRef.fields.customPartData.value_or(std::string());
    attachMode = PartRef.fields.attachMode;
    attached = PartRef.isAttached();
    autostrutMode = static_cast<Part_AutoStrutMode>(PartRef.fields.autoStrutMode);
    rigidAttachment = PartRef.fields.rigidAttachment;
    mass = PartRef.fields.mass;
    shielded = PartRef.ShieldedFromAirstream();
    temperature = PartRef.fields.temperature;
    skinTemperature = PartRef.fields.skinTemperature;
    skinUnexposedTemperature = PartRef.fields.skinUnexposedTemperature;
    staticPressureAtm = PartRef.fields.staticPressureAtm;
    explosionPotential = PartRef.fields.explosionPotential;
    state = static_cast<int>(PartRef.State());
    PreFailState = PartRef.fields.PreFailState;
    moduleCosts = PartRef.GetModuleCosts(PartRef.partInfo->cost);
    moduleMass = PartRef.GetModuleMass(PartRef.partInfo->cost);  // sic: the part COST is passed as the default mass
    moduleVariantName = "";
    if (PartRef.variants != nullptr && PartRef.variants->SelectedVariant() != nullptr) moduleVariantName = PartRef.variants->SelectedVariant()->name;
    moduleCargoStackableQuantity = 1;
    if (PartRef.isCargoPart()) moduleCargoStackableQuantity = PartRef.FindModuleOfClass("ModuleCargoPart")->Get<int32_t>("stackableQuantity");
    PartRef.onFlightStateSave(partStateValues);
    PartRef.protoPartSnapshot = this;
    for (const auto& pcm : PartRef.protoModuleCrew) {
        protoModuleCrew.push_back(pcm);
        protoCrewNames.push_back(pcm->name().value_or(std::string()));
        protoVessel->AddCrew(pcm);
    }
    for (size_t j = 0; j < PartRef.Modules().Count(); j++) modules.push_back(std::make_unique<ProtoPartModuleSnapshot>(*PartRef.Modules().GetModule(j)));
    position = Vector3d(PartRef.fields.orgPos.x, PartRef.fields.orgPos.y, PartRef.fields.orgPos.z);
    rotation = PartRef.fields.orgRot;
    for (size_t k = 0; k < PartRef.Resources.size(); k++) resources.push_back(std::make_unique<ProtoPartResourceSnapshot>(PartRef.Resources[k]));
    if (PartRef.fields.partRendererBoundsIgnore) partRendererBoundsIgnore = *PartRef.fields.partRendererBoundsIgnore;
    // partRef.Events.OnSave / Actions.OnSave: the part's own lists are kept as the nodes they were loaded from
    partEvents = PartRef.partEventsNode ? PartRef.partEventsNode->CreateCopy() : std::make_shared<ConfigNode>("EVENTS");
    partActions = PartRef.partActionsNode ? PartRef.partActionsNode->CreateCopy() : std::make_shared<ConfigNode>("ACTIONS");
    partData = std::make_shared<ConfigNode>("PARTDATA");
    PartRef.OnSave(*partData);
    flagURL = PartRef.fields.flagURL.value_or(std::string());
    refTransformName = PartRef.GetReferenceTransformName();
    if (PartRef.vesselNaming) vesselNaming = std::make_unique<VesselNaming>(*PartRef.vesselNaming);
    GE::onProtoPartSnapshotSave.Fire(GE::FromToAction<ProtoPartSnapshot*, ConfigNode*>(this, nullptr));
}

// ProtoPartSnapshot.cs:499
void ProtoPartSnapshot::storePartRefs() {
    symLinks.clear();
    symLinkIdxs.clear();
    for (size_t i = 0; i < pVesselRef->protoPartSnapshots.size(); i++) {
        ProtoPartSnapshot* other = pVesselRef->protoPartSnapshots[i].get();
        if (other->partRef == partRef->parent) {
            parent = other;
            parentIdx = static_cast<int>(i);
        }
        if (std::find(partRef->symmetryCounterparts.begin(), partRef->symmetryCounterparts.end(), other->partRef) !=
            partRef->symmetryCounterparts.end()) {
            symLinks.push_back(other);
            symLinkIdxs.push_back(static_cast<int>(i));
        }
    }
    attachNodes.clear();
    auto snapshotOf = [&](const AttachNode& node) {
        // AttachNodeSnapshot.cs:7 (AttachNode, ProtoVessel): the index of node.attachedPart in vessel.parts (-1 when none)
        AttachNodeSnapshot s;
        s.id = node.id;
        s.srfAttachMeshName = node.srfAttachMeshName;
        s.partIdx = -1;
        if (node.attachedPart != nullptr)
            for (size_t i = 0; i < pVesselRef->protoPartSnapshots.size(); i++)
                if (pVesselRef->protoPartSnapshots[i]->partRef == node.attachedPart) {
                    s.partIdx = static_cast<int>(i);
                    break;
                }
        return s;
    };
    srfAttachNode = snapshotOf(*partRef->srfAttachNode);
    for (const AttachNode& n : partRef->attachNodes) attachNodes.push_back(snapshotOf(n));
}

// ProtoPartSnapshot.cs:651
void ProtoPartSnapshot::ConfigurePart(Vessel* vesselRef, Part& partToConfigure, const AvailablePart& PartInfo, bool setPosition) {
    SaveEnvironment& env = SaveEnvironment::Current();
    partToConfigure.protoPartSnapshot = this;
    if (setPosition) {
        partToConfigure.partTransform.position = position.ToVector3f();
        partToConfigure.partTransform.rotation = rotation;
        partToConfigure.fields.orgPos = position.ToVector3f();
        partToConfigure.fields.orgRot = rotation;
    }
    partToConfigure.gameObjectName = partName.value_or(std::string());
    partToConfigure.fields.craftID = craftID;
    partToConfigure.fields.flightID = flightID;
    partToConfigure.fields.missionID = missionID;
    partToConfigure.fields.launchID = launchID;
    partToConfigure.partInfo = &PartInfo;
    partToConfigure.vessel = vesselRef;
    partToConfigure.fields.packed = true;
    if (env.persistentIds.fetch) env.persistentIds.persistentLoadedPartIds.Remove(partToConfigure.fields.persistentId);
    partToConfigure.fields.persistentId = persistentId;
    partToConfigure.fields.persistentId = env.persistentIds.CheckPartpersistentId(persistentId, partRef, true, true);
    if (persistentId != partToConfigure.fields.persistentId) persistentId = partToConfigure.fields.persistentId;
    partToConfigure.fields.inverseStage = inverseStageIndex;
    partToConfigure.fields.resourcePriorityOffset = resourcePriorityOffset;
    partToConfigure.fields.defaultInverseStage = defaultInverseStage;
    partToConfigure.fields.separationIndex = separationIndex;
    partToConfigure.fields.inStageIndex = inStageIndex;
    partToConfigure.fields.manualStageOffset = seqOverride;
    partToConfigure.fields.attachMode = attachMode;
    partToConfigure.set_isAttached(attached);
    partToConfigure.fields.autoStrutMode = static_cast<int32_t>(autostrutMode);
    partToConfigure.fields.rigidAttachment = rigidAttachment;
    partToConfigure.fields.customPartData = customPartData;
    partToConfigure.fields.sameVesselCollision = sameVesselCollision;
    partToConfigure.fields.mass = mass;
    partToConfigure.fields.explosionPotential = explosionPotential;
    partToConfigure.fields.temperature = temperature;
    partToConfigure.fields.skinTemperature = skinTemperature;
    partToConfigure.fields.skinUnexposedTemperature = skinUnexposedTemperature;
    partToConfigure.fields.staticPressureAtm = staticPressureAtm;
    partToConfigure.fields.ResumeState = state;
    partToConfigure.fields.PreFailState = PreFailState;
    if (!partRendererBoundsIgnore.empty()) {
        partToConfigure.fields.partRendererBoundsIgnore = partRendererBoundsIgnore;
    } else if (PartInfo.partPrefab && PartInfo.partPrefab->fields.partRendererBoundsIgnore &&
               !PartInfo.partPrefab->fields.partRendererBoundsIgnore->empty()) {
        partToConfigure.fields.partRendererBoundsIgnore = PartInfo.partPrefab->fields.partRendererBoundsIgnore;
    }
    partToConfigure.fields.flagURL = flagURL;
    // reference transform: the model's transform by name (FindModelTransform), else the part's own transform
    partToConfigure.SetReferenceTransformName(refTransformName.empty() ? partToConfigure.gameObjectName : refTransformName);
    partToConfigure.protoModuleCrew = protoModuleCrew;
    partToConfigure.SetMirror(mirror);
    partToConfigure.fields.symMethod = static_cast<int32_t>(symMethod);
    if (partActions) partToConfigure.partActionsNode = partActions->CreateCopy();
    if (partEvents) partToConfigure.partEventsNode = partEvents->CreateCopy();
    if (partEffects) partToConfigure.LoadEffects(*partEffects);
    if (partData) partToConfigure.OnLoad(*partData);
    int moduleIndex = 0;
    for (size_t i = 0; i < modules.size(); i++) modules[i]->Load(partToConfigure, moduleIndex);
    for (size_t j = 0; j < resources.size(); j++) resources[j]->Load(partToConfigure);
    if (vesselNaming) partToConfigure.vesselNaming = std::make_shared<VesselNaming>(*vesselNaming);
}

// ProtoPartSnapshot.cs:761
std::unique_ptr<Part> ProtoPartSnapshot::CreatePart() {
    GE::onProtoPartSnapshotLoad.Fire(GE::FromToAction<ProtoPartSnapshot*, ConfigNode*>(this, nullptr));
    PartVariant* partVariant = nullptr;
    size_t count = modules.size();
    while (count-- > 0) {
        if (modules[count]->moduleName == "ModulePartVariants" && modules[count]->moduleRef != nullptr)
            partVariant = static_cast<ModulePartVariants*>(modules[count]->moduleRef)->SelectedVariant();
    }
    if (partInfo == nullptr) return nullptr;
    AvailablePart& availablePart = const_cast<AvailablePart&>(*partInfo);  // partInfo.variant is mutated in C#
    if (partVariant != nullptr) {
        // partInfo.variant = partVariant (a reference to the selected variant of the live module)
        availablePart.variant = std::shared_ptr<PartVariant>(std::shared_ptr<PartVariant>{}, partVariant);
    }
    if (!availablePart.partPrefab) return nullptr;
    std::unique_ptr<Part> created = Part::Instantiate(*availablePart.partPrefab);
    partRef = created.get();
    ConfigurePart(nullptr, *partRef, availablePart, true);
    if (partRef->variants != nullptr && availablePart.variant != nullptr) partRef->variants->SetVariant(availablePart.variant->name);
    return created;
}

}  // namespace ksp
