// ShipConstruct over live Part objects (WP-18); see include/ksp/save/ship_construct.hpp. The file-side load (WP-04) is
// src/save/ship_construct.cpp; the editor / UI parts of ShipConstruct.cs are not ported.
#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/part_set.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel_assembly.hpp"
#include "save_util.hpp"

namespace ksp {

using save_detail::EnumName;
using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

// ---- ownership ------------------------------------------------------------------------------------------------------------------

// The parts the ship owns must not keep a pointer to a destroyed ship; parts it merely lists (Add) belong to someone else
ShipConstruct::~ShipConstruct() {
    for (auto& p : ownedParts)
        if (p->ship == this) p->ship = nullptr;
}

// ResourceFlowEnvironment::shipHost: Part.ship -> its IResourceVessel (the ship itself), installed once
void ShipConstruct::RegisterResourceHost() {
    if (!ResourceFlowEnvironment::shipHost)
        ResourceFlowEnvironment::shipHost = [](ShipConstruct* s) { return static_cast<IResourceVessel*>(s); };
}

// ---- constructors (ShipConstruct.cs:89-150) -----------------------------------------------------------------------------------

namespace {
void InitOverrides(ShipConstruct& s) {
    ShipConstruct::RegisterResourceHost();
    s.OverrideDefault.assign(4, false);  // Vessel.NumOverrideGroups
    s.OverrideActionControl.assign(4, KSPActionGroup::None);
    s.OverrideAxisControl.assign(4, KSPAxisGroup::None);
    s.OverrideGroupNames.assign(4, std::nullopt);
}
}  // namespace

// ShipConstruct.cs:89 (the constructor does not set the identity rotation; the field default of Quaternion is (0,0,0,0))
ShipConstruct::ShipConstruct(EditorFacility facility) {
    shipFacility = facility;
    shipSize = Vector3f{0.0f, 0.0f, 0.0f};
    persistentId = SaveEnvironment::Current().persistentIds.GetUniquepersistentId();
    rotation = Quaternionf{0.0f, 0.0f, 0.0f, 0.0f};
    InitOverrides(*this);
}

// ShipConstruct.cs:102
ShipConstruct::ShipConstruct(const std::string& name, EditorFacility facility, std::vector<Part*> p) {
    shipName = name;
    shipFacility = facility;
    liveParts = std::move(p);
    for (Part* part : liveParts) part->ship = this;
    shipSize = ShipConstruction::CalculateCraftSize(*this);
    persistentId = SaveEnvironment::Current().persistentIds.GetUniquepersistentId();
    rotation = Quaternionf{0.0f, 0.0f, 0.0f, 0.0f};
    InitOverrides(*this);
}

// ShipConstruct.cs:120
ShipConstruct::ShipConstruct(const std::string& name, const std::string& description, Part* rootPart) {
    shipName = name;
    shipDescription = description;
    AddToConstruct(rootPart);
    shipSize = ShipConstruction::CalculateCraftSize(*this);
    persistentId = SaveEnvironment::Current().persistentIds.GetUniquepersistentId();
    rotation = Quaternionf{0.0f, 0.0f, 0.0f, 0.0f};
    InitOverrides(*this);
}

// ShipConstruct.cs:135
ShipConstruct::ShipConstruct(const std::string& name, const std::string& description, std::vector<Part*> p) {
    shipName = name;
    shipDescription = description;
    liveParts = std::move(p);
    for (Part* part : liveParts) part->ship = this;
    shipSize = ShipConstruction::CalculateCraftSize(*this);
    persistentId = SaveEnvironment::Current().persistentIds.GetUniquepersistentId();
    rotation = Quaternionf{0.0f, 0.0f, 0.0f, 0.0f};
    InitOverrides(*this);
}

// ---- list operations (ShipConstruct.cs:742-784) ---------------------------------------------------------------------------------

// ShipConstruct.cs:752 (List.Remove: the first match)
void ShipConstruct::Remove(Part* p) {
    auto it = std::find(liveParts.begin(), liveParts.end(), p);
    if (it != liveParts.end()) liveParts.erase(it);
}

// ShipConstruct.cs:757
bool ShipConstruct::Contains(const Part* p) const { return std::find(liveParts.begin(), liveParts.end(), p) != liveParts.end(); }

// ShipConstruct.cs:772
void ShipConstruct::AddToConstruct(Part* rootPart) {
    if (!Contains(rootPart)) {
        liveParts.push_back(rootPart);
        rootPart->ship = this;
    }
    const size_t count = rootPart->children.size();
    for (size_t i = 0; i < count; i++) AddToConstruct(rootPart->children[i]);
}

// ShipConstruct.cs:709
bool ShipConstruct::AreAllPartsConnected() const {
    int count = static_cast<int>(liveParts.size());
    while (count-- > 0) {
        const Part* part = liveParts[static_cast<size_t>(count)];
        if (part->parent == nullptr && part->children.empty()) return false;
    }
    return true;
}

// ShipConstruct.cs:727 ([Obsolete])
bool ShipConstruct::isControllable() const {
    for (const Part* p : liveParts)
        if (p->fields.isControlSource > 0) return true;
    return false;
}

// ---- LoadShip: the part-creating half (ShipConstruct.cs:378-700) -----------------------------------------------------------------

// ShipConstruct.cs:378-700 over the PART records the file-side LoadShip (ship_construct.cpp) produced
bool ShipConstruct::InstantiateParts(bool returnErrors, std::string& errorString) {
    SaveEnvironment& env = SaveEnvironment::Current();
    auto lookup = [&env](const std::string& n) -> const AvailablePart* { return env.getPartInfoByName ? env.getPartInfoByName(n) : nullptr; };
    liveParts.clear();
    ownedParts.clear();
    CraftParts cp = BuildCraftParts(*this, lookup, this);
    if (!cp.ok) {
        // :455-466 "case part": the part has no AvailablePart / prefab
        const std::string msg = Localizer::Format("#autoLOC_6002427", {cp.missing});
        if (returnErrors) errorString = msg;
        else log::Error("Loading Failed: " + msg);
        return false;
    }
    ownedParts = std::move(cp.parts);
    for (auto& up : ownedParts) {
        liveParts.push_back(up.get());
        // :446 if (!ResearchAndDevelopment.PartTechAvailable(availablePart)) shipPartsUnlocked = false;
        if (ShipConstruct::partTechAvailable && up->partInfo != nullptr && !ShipConstruct::partTechAvailable(*up->partInfo)) shipPartsUnlocked = false;
    }
    // :672-686 IShipConstructIDChanges.UpdatePersistentIDs(dictionary)
    for (Part* part2 : liveParts) {
        for (PartModule* m : part2->FindModulesImplementing([](PartModule& pm) { return dynamic_cast<IShipConstructIDChanges*>(&pm) != nullptr; }))
            dynamic_cast<IShipConstructIDChanges*>(m)->UpdatePersistentIDs(changedPersistentIds_);
    }
    // :688 StageManager.SetStageCount(parts): computes the highest inverseStage and discards it (no side effect in the C#)
    UpdateVesselNaming();  // :689
    if (ResourceFlowEnvironment::LoadedSceneIsEditor() && ShipConstruct::onEditorShipLoaded) ShipConstruct::onEditorShipLoaded(*this);  // :690-700
    log::Info(Localizer::Format(shipName) + " loaded!");  // :702
    return true;
}

// ShipConstruct.cs:301 (the whole of LoadShip in a scene with a part database)
bool ShipConstruct::LoadShipWithParts(const ConfigNode& root, uint32_t persistentID, bool returnErrors, std::string& errorString) {
    if (!LoadShip(root, persistentID, returnErrors, errorString)) return false;
    return InstantiateParts(returnErrors, errorString);
}

// ShipConstruct.cs:295 / :704
bool ShipConstruct::LoadShipWithParts(const ConfigNode& root, uint32_t persistentID) {
    std::string errorString;
    return LoadShipWithParts(root, persistentID, false, errorString);
}

// ---- SaveShip over live parts (ShipConstruct.cs:153-293) ---------------------------------------------------------------------------

// ShipConstruct.cs:153
ConfigNode::NodePtr ShipConstruct::SaveShipFromParts() {
    GE::onAboutToSaveShip.Fire(this);
    auto configNode = std::make_shared<ConfigNode>();
    configNode->AddValue("ship", shipName);
    configNode->AddValue("version", Versioning_GetVersion());
    configNode->AddValue("description", dotnet::Replace(shipDescription, "\n", "\xC2\xA8"));
    configNode->AddValue("type", EnumName(shipFacility));
    // shipSize = ShipConstruction.CalculateCraftSize(this): zero when no part has renderer bounds (no model reader); then the
    // size the ship carries (from the file) is kept
    {
        const Vector3f size = ShipConstruction::CalculateCraftSize(*this);
        bool anyBounds = false;
        for (const Part* p : liveParts)
            if (!p->modelRenderers.empty()) anyBounds = true;
        if (anyBounds || liveParts.empty()) shipSize = size;
    }
    configNode->AddValue("size", KSPUtil::WriteVector(shipSize));
    configNode->AddValue("steamPublishedFileId", steamPublishedFileId);
    SaveEnvironment::Current().persistentIds.CheckVesselpersistentId(persistentId, nullptr, false, false);
    configNode->AddValue("persistentId", persistentId);
    configNode->AddValue("rot", rotation);
    configNode->AddValue("missionFlag", missionFlag);
    configNode->AddValue("vesselType", EnumName(vesselType));
    configNode->AddValue("OverrideDefault", save_detail::WriteBoolArray(OverrideDefault));
    configNode->AddValue("OverrideActionControl", save_detail::WriteEnumIntArray(OverrideActionControl));
    configNode->AddValue("OverrideAxisControl", save_detail::WriteEnumIntArray(OverrideAxisControl));
    configNode->AddValue("OverrideGroupNames", save_detail::WriteStringArray(OverrideGroupNames));
    auto nameOf = [](const Part* p) { return p->partInfo->name + "_" + dotnet::FormatUInt64(p->fields.craftID); };
    for (Part* part : liveParts) {
        part->onBackup();
        ConfigNode& n2 = *configNode->AddNode("PART");
        n2.AddValue("part", nameOf(part));
        save_detail::AddNullableValue(n2, "partName", part->fields.partName);
        n2.AddValue("persistentId", part->fields.persistentId);
        n2.AddValue("pos", KSPUtil::WriteVector(part->partTransform.position));
        n2.AddValue("attPos", KSPUtil::WriteVector(part->fields.attPos));
        n2.AddValue("attPos0", KSPUtil::WriteVector(part->fields.attPos0));
        n2.AddValue("rot", KSPUtil::WriteQuaternion(part->partTransform.rotation));
        n2.AddValue("attRot", KSPUtil::WriteQuaternion(part->fields.attRotation));
        n2.AddValue("attRot0", KSPUtil::WriteQuaternion(part->fields.attRotation0));
        n2.AddValue("mir", KSPUtil::WriteVector(part->fields.mirrorVector));
        n2.AddValue("symMethod", EnumName(static_cast<SymmetryMethod>(part->fields.symMethod)));
        n2.AddValue("autostrutMode", EnumName(static_cast<Part_AutoStrutMode>(part->fields.autoStrutMode)));
        n2.AddValue("rigidAttachment", dotnet::FormatBool(part->fields.rigidAttachment));
        n2.AddValue("istg", static_cast<int32_t>(part->fields.inverseStage));
        n2.AddValue("resPri", static_cast<int32_t>(part->fields.resourcePriorityOffset));
        n2.AddValue("dstg", static_cast<int32_t>(part->fields.defaultInverseStage));
        n2.AddValue("sidx", static_cast<int32_t>(part->fields.inStageIndex));
        n2.AddValue("sqor", static_cast<int32_t>(part->fields.manualStageOffset));
        n2.AddValue("sepI", static_cast<int32_t>(part->fields.separationIndex));
        n2.AddValue("attm", static_cast<int32_t>(part->fields.attachMode));
        n2.AddValue("sameVesselCollision", part->fields.sameVesselCollision);
        n2.AddValue("modCost", part->GetModuleCosts(part->partInfo->cost));
        n2.AddValue("modMass", part->GetModuleMass(part->partInfo->partPrefab->fields.mass));
        n2.AddValue("modSize", part->GetModuleSize(part->fields.prefabSize));
        if (part->fields.customPartData.value_or(std::string()) != "") n2.AddValue("cData", *part->fields.customPartData);
        for (const Part* c : part->children) n2.AddValue("link", nameOf(c));
        for (const Part* s : part->symmetryCounterparts) {
            if (s == nullptr) throw NullReferenceException("ShipConstruct.SaveShip: null symmetry counterpart");
            n2.AddValue("sym", nameOf(s));
        }
        const AttachNode& srf = *part->srfAttachNode;
        if (srf.attachedPart != nullptr) {
            std::string s = srf.id + "," + nameOf(srf.attachedPart);
            if (!srf.srfAttachMeshName.empty()) s += "," + srf.srfAttachMeshName;
            else s += ",";
            s += "," + KSPUtil::WriteVector(srf.position, "|");
            s += "," + KSPUtil::WriteVector(srf.orientation, "|");
            s += "," + KSPUtil::WriteVector(srf.position, "|");
            n2.AddValue("srfN", s);
        }
        for (const AttachNode& a : part->attachNodes) {
            const std::string value5 = a.attachedPart != nullptr ? a.attachedPart->partInfo->name : "Null";
            const std::string value6 = a.attachedPart != nullptr ? dotnet::FormatUInt64(a.attachedPart->fields.craftID) : "0";
            n2.AddValue("attN", a.id + "," + value5 + "_" + value6 + "_" + KSPUtil::WriteVector(a.originalPosition, "|") + "_" +
                                    KSPUtil::WriteVector(a.originalOrientation, "|") + "_" + KSPUtil::WriteVector(a.position, "|") + "_" +
                                    KSPUtil::WriteVector(a.orientation, "|"));
        }
        // part.Events.OnSave / Actions.OnSave: the part's lists are the nodes they were loaded from (Part's PAW lists are not modelled)
        (part->partEventsNode ? part->partEventsNode->CopyTo(*n2.AddNode("EVENTS")) : (void)n2.AddNode("EVENTS"));
        (part->partActionsNode ? part->partActionsNode->CopyTo(*n2.AddNode("ACTIONS")) : (void)n2.AddNode("ACTIONS"));
        part->OnSave(*n2.AddNode("PARTDATA"));
        for (size_t m = 0; m < part->Modules().Count(); m++) part->Modules().GetModule(m)->Save(*n2.AddNode("MODULE"));
        for (int r = 0; r < part->Resources.Count(); r++) part->Resources[static_cast<size_t>(r)].Save(*n2.AddNode("RESOURCE"));
        if (part->vesselNaming) part->vesselNaming->Save(*n2.AddNode("VESSELNAMING"));
    }
    return configNode;
}

// ---- costs, masses (ShipConstruct.cs:786-864) ---------------------------------------------------------------------------------------

// ShipConstruct.cs:791 (float locals and out parameters store at every assignment: Mono rule)
float ShipConstruct::GetShipCosts(float& dryCost, float& fuelCost, VesselCrewManifest* vcm) {
    float num = 0.0f;
    dryCost = 0.0f;
    fuelCost = 0.0f;
    int count = static_cast<int>(liveParts.size());
    while (count-- > 0) {
        Part* part = liveParts[static_cast<size_t>(count)];
        const AvailablePart* partInfo = part->partInfo;
        float num2 = f32store(fx(partInfo->cost) + fx(part->GetModuleCosts(partInfo->cost)));
        float num3 = 0.0f;
        int count2 = part->Resources.Count();
        while (count2-- > 0) {
            PartResource& partResource = part->Resources[static_cast<size_t>(count2)];
            const PartResourceDefinition* info = partResource.info;
            if (info == nullptr) throw NullReferenceException();
            num2 = f32store(fx(num2) - fx(info->unitCost()) * fx(f32store(partResource.maxAmount)));
            num3 = f32store(fx(num3) + fx(info->unitCost()) * fx(f32store(partResource.amount)));
        }
        dryCost = f32store(fx(dryCost) + fx(num2));
        fuelCost = f32store(fx(fuelCost) + fx(num3));
    }
    num = f32store(fx(num) + (fx(dryCost) + fx(fuelCost)));
    if (vcm != nullptr) num = f32store(fx(num) + fx(vcm->GetCrewInventoryCosts()));
    return num;
}

// ShipConstruct.cs:827
float ShipConstruct::GetShipMass(float& dryMass, float& fuelMass, VesselCrewManifest* vcm) {
    float num = 0.0f;
    dryMass = 0.0f;
    fuelMass = 0.0f;
    int count = static_cast<int>(liveParts.size());
    while (count-- > 0) {
        Part* part = liveParts[static_cast<size_t>(count)];
        part->UpdateMass();
        const float mass = part->fields.mass;
        float num2 = 0.0f;
        int count2 = part->Resources.Count();
        while (count2-- > 0) {
            PartResource& partResource = part->Resources[static_cast<size_t>(count2)];
            const PartResourceDefinition* info = partResource.info;
            if (info == nullptr) throw NullReferenceException();
            num2 = f32store(fx(num2) + fx(info->density()) * fx(f32store(partResource.amount)));
        }
        dryMass = f32store(fx(dryMass) + fx(mass));
        fuelMass = f32store(fx(fuelMass) + fx(num2));
    }
    num = f32store(fx(num) + (fx(dryMass) + fx(fuelMass)));
    if (vcm != nullptr) {
        num = f32store(fx(num) + fx(static_cast<float>(vcm->CrewCount())) * fx(PhysicsGlobals::KerbalCrewMass()));
        num = f32store(fx(num) + fx(vcm->GetCrewInventoryMass()));
        num = f32store(fx(num) + fx(vcm->GetCrewResourceMass()));
    }
    return num;
}

// ShipConstruct.cs:859
float ShipConstruct::GetTotalMass() {
    float dryMass = 0.0f, fuelMass = 0.0f;
    return GetShipMass(dryMass, fuelMass, ShipConstruction::ShipManifest);
}

// ---- vessel naming (ShipConstruct.cs:866-914) ---------------------------------------------------------------------------------------

// ShipConstruct.cs:866 (VesselNaming.FindPriorityNamePart(ShipConstruct), VesselNaming.cs:51)
bool ShipConstruct::UpdateVesselNaming(bool noGameEvent) {
    Part* part = nullptr;
    int num = 0;
    for (Part* p : liveParts) {
        const VesselNaming* vn = p->vesselNaming.get();
        if (vn != nullptr && vn->namingPriority > num) {
            num = vn->namingPriority;
            part = p;
        }
    }
    if (part == nullptr) {
        vesselNamedBy = nullptr;
        return false;
    }
    RunVesselNamingUpdates(part, noGameEvent);
    return true;
}

// ShipConstruct.cs:878
void ShipConstruct::RunVesselNamingUpdates(Part* pNewName, bool noGameEvent) {
    const std::string from = shipName;
    bool flag = false;
    if (ResourceFlowEnvironment::LoadedSceneIsEditor() && vesselNamedBy == nullptr && ShipConstruct::editorRootPart)
        vesselNamedBy = ShipConstruct::editorRootPart();  // EditorLogic.RootPart
    if (vesselNamedBy == nullptr) vesselNamedBy = pNewName;
    if (vesselNamedBy->fields.persistentId != pNewName->fields.persistentId) {
        highestPriorityName = pNewName->vesselNaming->vesselName.value_or(std::string());
        vesselType = pNewName->vesselNaming->vesselType;
        vesselNamedBy = pNewName;
        flag = true;
    } else if (vesselNamedBy->vesselNaming->vesselName != highestPriorityName) {
        highestPriorityName = pNewName->vesselNaming->vesselName.value_or(std::string());
        vesselType = pNewName->vesselNaming->vesselType;
        flag = true;
    } else if (vesselType != vesselNamedBy->vesselNaming->vesselType) {
        vesselType = pNewName->vesselNaming->vesselType;
        flag = true;
    }
    if (flag && !noGameEvent) {
        if (ShipConstruct::postScreenMessage) ShipConstruct::postScreenMessage(pNewName->vesselNaming->vesselName.value_or(std::string()));
        GE::onEditorVesselNamingChanged.Fire(GE::HostedFromToAction<ShipConstruct*, std::string>(this, from, shipName));
    }
}

// ---- resource sets (ShipConstruct.cs:916-970, WP-15) ------------------------------------------------------------------------------------

// ShipConstruct.cs:927
double ShipConstruct::RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) {
    return resourceSets.RequestResource(part, id, demand, usePriority, simulate);
}

// ShipConstruct.cs:916
void ShipConstruct::GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling) {
    resourceSets.GetConnectedResourceTotals(id, simulate, amount, maxAmount, pulling);
}

// The threshold variant exists on Vessel only (Vessel.cs:4022); a ShipConstruct has no vessel to ask: no totals
void ShipConstruct::GetConnectedResourceTotals(int, bool, double& amount, double& maxAmount, double, bool) {
    amount = 0.0;
    maxAmount = 0.0;
}

}  // namespace ksp
