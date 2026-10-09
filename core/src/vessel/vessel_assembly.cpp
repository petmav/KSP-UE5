// See include/ksp/vessel/vessel_assembly.hpp
#include "ksp/vessel/vessel_assembly.hpp"

#include "ksp/save/save_environment.hpp"

namespace ksp {

// ShipConstruct.cs:378-640 (the part creation of LoadShip)
CraftParts BuildCraftParts(const ShipConstruct& ship, const std::function<const AvailablePart*(const std::string&)>& partInfoByName) {
    CraftParts out;
    for (const auto& d : ship.parts) {
        const AvailablePart* ap = partInfoByName(d.partInfoName);
        if (ap == nullptr || !ap->partPrefab) {
            out.missing = d.partInfoName;
            return out;
        }
    }
    for (const auto& d : ship.parts) {
        const AvailablePart* ap = partInfoByName(d.partInfoName);
        auto part = Part::Instantiate(*ap->partPrefab);
        part->gameObjectName = d.partInfoName;
        part->fields.craftID = d.craftID;
        part->partInfo = ap;
        part->fields.symMethod = static_cast<int>(d.symMethod);
        part->fields.persistentId = d.persistentId;
        part->partTransform.position = d.position;
        part->partTransform.rotation = d.rotation;
        part->fields.attPos = d.attPos;
        part->fields.attPos0 = d.attPos0;
        part->fields.attRotation = d.attRotation;
        part->fields.attRotation0 = d.attRotation0;
        part->SetMirror(d.mirrorVector);
        part->fields.autoStrutMode = static_cast<int>(d.autoStrutMode);
        part->fields.rigidAttachment = d.rigidAttachment;
        part->fields.inverseStage = d.inverseStage;
        part->fields.resourcePriorityOffset = d.resourcePriorityOffset;
        part->fields.defaultInverseStage = d.defaultInverseStage;
        part->fields.inStageIndex = d.inStageIndex;
        part->fields.manualStageOffset = d.manualStageOffset;
        part->fields.separationIndex = d.separationIndex;
        part->fields.attachMode = d.attachMode;
        part->fields.sameVesselCollision = d.sameVesselCollision;
        part->fields.customPartData = d.customPartData;
        if (d.srfAttachNodeInFile) {
            AttachNode& n = *part->srfAttachNode;
            n.attachedPartId = d.srfAttachNode.attachedPartId;
            n.srfAttachMeshName = d.srfAttachNode.srfAttachMeshName.value_or(std::string());
            if (d.srfAttachNode.originalOrientationKnown) {
                n.originalPosition = n.position = d.srfAttachNode.position;
                n.orientation = n.originalOrientation = d.srfAttachNode.orientation;
            }
        }
        for (const auto& a : d.attachNodes) {
            AttachNode* n = part->FindAttachNode(a.id);
            if (n == nullptr) continue;
            n->attachedPartId = a.attachedPartId;
            if (a.positionKnown) n->position = a.position;
            if (a.orientationKnown) n->orientation = a.orientation;
        }
        int moduleIndex = 0;
        for (const auto& m : d.modules) part->LoadModule(*m, moduleIndex);
        for (const auto& r : d.resources) part->SetResource(*r);
        if (d.partData) part->OnLoad(*d.partData);
        out.parts.push_back(std::move(part));
    }
    std::vector<Part*> all;
    for (auto& p : out.parts) all.push_back(p.get());
    for (size_t n = 0; n < out.parts.size(); n++) {
        Part* part2 = out.parts[n].get();
        for (int c : ship.parts[n].children) out.parts[static_cast<size_t>(c)]->setParent(part2);
        for (int s : ship.parts[n].symmetryCounterparts)
            part2->symmetryCounterparts.push_back(s >= 0 ? out.parts[static_cast<size_t>(s)].get() : nullptr);
        part2->srfAttachNode->owner = part2;
        part2->srfAttachNode->FindAttachedPart(all);
        for (AttachNode& an : part2->attachNodes) {
            an.owner = part2;
            an.FindAttachedPart(all);
        }
    }
    for (auto& p : out.parts) p->InitializeModules();
    out.ok = true;
    return out;
}

namespace {
uint32_t UniqueHash() { return static_cast<uint32_t>(SaveEnvironment::Current().NewGuidHashCode()); }
Guid RandomGuid() {
    Guid g;
    for (int i = 0; i < 4; i++) {
        const uint32_t r = UniqueHash();
        for (int k = 0; k < 4; k++) g.bytes[i * 4 + k] = static_cast<uint8_t>(r >> (8 * k));
    }
    g.bytes[7] = static_cast<uint8_t>((g.bytes[7] & 0x0F) | 0x40);
    g.bytes[8] = static_cast<uint8_t>((g.bytes[8] & 0x3F) | 0x80);
    return g;
}
// ShipConstruction.cs:463 findFirstCrewablePart
Part* FindFirstCrewablePart(Part* part) {
    if (part->fields.CrewCapacity > 0 && !part->protoModuleCrew.empty() && part->fields.isControlSource > 0) return part;
    for (Part* c : part->children)
        if (Part* p = FindFirstCrewablePart(c)) return p;
    return nullptr;
}
// ShipConstruction.cs:482 findFirstControlSource
Part* FindFirstControlSource(Vessel& v) {
    for (Part* p : v.parts)
        if (p->fields.isControlSource > 0) return p;
    return nullptr;
}
}  // namespace

// ShipConstruction.cs:296
std::unique_ptr<Vessel> AssembleVesselFromCraft(const ShipConstruct& ship,
                                                const std::function<const AvailablePart*(const std::string&)>& partInfoByName,
                                                const AssembleOptions& options) {
    CraftParts cp = BuildCraftParts(ship, partInfoByName);
    if (!cp.ok) return nullptr;
    auto vessel = std::make_unique<Vessel>();
    Part* localRoot = cp.parts[0]->localRoot();
    localRoot->partTransform.position = options.rootPosition;
    localRoot->partTransform.rotation = options.rootRotation;
    vessel->vesselTransform.position = options.rootPosition;
    vessel->vesselTransform.rotation = options.rootRotation;
    std::vector<std::unique_ptr<Part>> owned = std::move(cp.parts);
    vessel->AdoptParts(std::move(owned), localRoot);
    vessel->id = RandomGuid();
    vessel->vesselName = ship.shipName;
    vessel->persistentId = ship.persistentId;
    if (!ship.OverrideDefault.empty()) vessel->OverrideDefault = ship.OverrideDefault;
    if (!ship.OverrideActionControl.empty()) vessel->OverrideActionControl = ship.OverrideActionControl;
    if (!ship.OverrideAxisControl.empty()) vessel->OverrideAxisControl = ship.OverrideAxisControl;
    if (!ship.OverrideGroupNames.empty()) vessel->OverrideGroupNames = ship.OverrideGroupNames;
    vessel->Initialize(true, false, false, options.setActiveVessel);
    vessel->UpdateVesselNaming();
    vessel->IgnoreGForces(10);
    if (!options.landedAt.empty()) {
        vessel->vesselSpawning = true;
        vessel->launchedFrom = options.landedAt;
    }
    if (options.isLanded) {
        if (!options.isSplashed) {
            vessel->Landed = true;
            vessel->SetLandedAt(options.landedAt, "", options.displaylandedAt);
            vessel->skipGroundPositioning = false;
        } else {
            vessel->Landed = false;
            vessel->Splashed = true;
            vessel->skipGroundPositioning = true;
        }
    } else {
        vessel->Landed = false;
    }
    const uint32_t hashCode = UniqueHash();
    for (Part* part : vessel->parts) {
        part->fields.flightID = UniqueHash();  // ShipConstruction.GetUniqueFlightID
        part->fields.missionID = hashCode;
        part->fields.launchID = 0;
        part->fields.flagURL = options.flagURL;
    }
    vessel->RebuildCrewList();
    if (options.setActiveVessel) FlightRegistry::ActiveVessel = vessel.get();
    if (localRoot->fields.isControlSource == 0) {
        Part* part2 = FindFirstCrewablePart(ship.parts.empty() ? localRoot : vessel->parts[0]);
        Part* ref = part2 != nullptr ? part2 : FindFirstControlSource(*vessel);
        vessel->SetReferenceTransform(ref != nullptr ? ref : localRoot);
    } else {
        vessel->SetReferenceTransform(localRoot);
    }
    return vessel;
}

}  // namespace ksp
