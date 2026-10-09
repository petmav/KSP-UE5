// See include/ksp/vessel/vessel_assembly.hpp
#include "ksp/vessel/vessel_assembly.hpp"

#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/ship/ship_construction.hpp"

namespace ksp {

// ShipConstruct.cs:378-640 (the part creation of LoadShip)
CraftParts BuildCraftParts(const ShipConstruct& ship, const std::function<const AvailablePart*(const std::string&)>& partInfoByName,
                           ShipConstruct* shipOwner, const std::function<void(Part&, size_t)>& onPartLoaded) {
    CraftParts out;
    for (const auto& d : ship.parts) {
        const AvailablePart* ap = partInfoByName(d.partInfoName);
        if (ap == nullptr || !ap->partPrefab) {
            out.missing = d.partInfoName;
            return out;
        }
    }
    size_t partIndex = 0;
    for (const auto& d : ship.parts) {
        const AvailablePart* ap = partInfoByName(d.partInfoName);
        auto part = Part::Instantiate(*ap->partPrefab);
        part->gameObjectName = d.partInfoName;
        part->fields.craftID = d.craftID;
        part->partInfo = ap;
        part->ship = shipOwner;  // ShipConstruct.cs:443 part.ship = this
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
        // ShipConstruct.cs:600-620: ACTIONS / EVENTS (Part.Events / Actions are kept as nodes, WP-13), EFFECTS, VESSELNAMING
        if (d.actions) part->partActionsNode = d.actions->CreateCopy();
        if (d.events) part->partEventsNode = d.events->CreateCopy();
        if (d.effects) part->LoadEffects(*d.effects);
        if (d.vesselNaming) part->vesselNaming = std::make_shared<VesselNaming>(*d.vesselNaming);
        if (onPartLoaded) onPartLoaded(*part, partIndex);
        partIndex++;
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
    // ShipConstruct.cs:670-678: orgPos / orgRot relative to the transform root (the root part), packed = true, then InitializeModules
    Part* rootPart = out.parts.empty() ? nullptr : out.parts[0]->localRoot();
    for (auto& p : out.parts) {
        const PartTransform& rt = rootPart->partTransform;
        p->fields.orgPos = rt.InverseTransformPoint(p->partTransform.position);
        const Quaternionf rInv(0.0f - rt.rotation.x, 0.0f - rt.rotation.y, 0.0f - rt.rotation.z, rt.rotation.w);  // Quaternion.Inverse of a unit rotation
        p->fields.orgRot = rInv * p->partTransform.rotation;
        p->fields.packed = true;
        p->InitializeModules();
    }
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
}  // namespace

// ShipConstruction.cs:296 (AssembleForLaunch over the parts of a ship; the launch-site placement moves the whole ship)
std::unique_ptr<Vessel> AssembleVesselFromParts(std::vector<std::unique_ptr<Part>> cpParts, const ShipConstruct& ship,
                                                const AssembleOptions& options) {
    if (cpParts.empty()) return nullptr;
    auto vessel = std::make_unique<Vessel>();
    Part* firstPart = cpParts[0].get();
    Part* localRoot = firstPart->localRoot();  // :298 ship.parts[0].localRoot
    for (auto& up : cpParts) {
        up->partTransform.position = options.rootPosition + options.rootRotation * up->fields.orgPos;
        up->partTransform.rotation = options.rootRotation * up->fields.orgRot;
    }
    vessel->vesselTransform.position = options.rootPosition;
    vessel->vesselTransform.rotation = options.rootRotation;
    if (options.orbit) {  // :349 (vessel.gameObject.GetComponent<OrbitDriver>() ?? AddComponent<OrbitDriver>()).orbit = orbit
        vessel->orbitDriver = std::make_unique<OrbitDriver>();
        vessel->orbitDriver->vessel = &vessel->OrbitDriverView();
        vessel->orbitDriver->orbit = options.orbit;
    }
    vessel->AdoptParts(std::move(cpParts), localRoot);
    vessel->id = RandomGuid();
    vessel->vesselName = ship.shipName;
    vessel->persistentId = ship.persistentId;
    // :304-348 the override arrays are copied when the ship has them (else new arrays of Vessel.NumOverrideGroups)
    if (!ship.OverrideDefault.empty()) vessel->OverrideDefault = ship.OverrideDefault;
    if (!ship.OverrideActionControl.empty()) vessel->OverrideActionControl = ship.OverrideActionControl;
    if (!ship.OverrideAxisControl.empty()) vessel->OverrideAxisControl = ship.OverrideAxisControl;
    if (!ship.OverrideGroupNames.empty()) vessel->OverrideGroupNames = ship.OverrideGroupNames;
    vessel->Initialize(options.fromShipAssembly, options.preCreate, options.orbiting, options.setActiveVessel);
    vessel->UpdateVesselNaming();
    vessel->IgnoreGForces(10);
    if (options.fromShipAssembly && !options.landedAt.empty()) {
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
        // ShipConstruction.GetUniqueFlightID(sceneState.flightState)
        uint32_t flightID;
        do {
            flightID = UniqueHash();
        } while (options.flightState != nullptr && options.flightState->ContainsFlightID(flightID) && flightID != 0);
        part->fields.flightID = flightID;
        part->fields.missionID = hashCode;
        part->fields.launchID = options.launchID;
        part->fields.flagURL = options.flagURL;
    }
    if (options.crewManifestAssign) options.crewManifestAssign();  // crewManifest.AssignCrewToVessel(ship)
    vessel->RebuildCrewList();
    if (options.preCreate && vessel->protoVessel != nullptr) {  // :386-399
        for (auto& pps : vessel->protoVessel->protoPartSnapshots) {
            for (const auto& pcm : pps->partRef->protoModuleCrew) {
                pps->protoModuleCrew.push_back(pcm);
                pps->protoCrewNames.push_back(pcm->name().value_or(std::string()));
                vessel->protoVessel->AddCrew(pcm);
            }
        }
        vessel->protoVessel->RebuildCrewCounts();
    }
    if (options.setActiveVessel) FlightRegistry::ActiveVessel = vessel.get();
    if (localRoot->fields.isControlSource == 0) {
        Part* part2 = ShipConstruction::findFirstCrewablePart(firstPart);
        Part* ref = part2 != nullptr ? part2 : ShipConstruction::findFirstControlSource(*vessel);
        vessel->SetReferenceTransform(ref != nullptr ? ref : localRoot);
    } else {
        vessel->SetReferenceTransform(localRoot);
    }
    return vessel;
}

// ShipConstruction.cs:296 (AssembleForLaunch with fromShipAssembly = true over a craft file)
std::unique_ptr<Vessel> AssembleVesselFromCraft(const ShipConstruct& ship,
                                                const std::function<const AvailablePart*(const std::string&)>& partInfoByName,
                                                const AssembleOptions& options) {
    CraftParts cp = BuildCraftParts(ship, partInfoByName);
    if (!cp.ok) return nullptr;
    return AssembleVesselFromParts(std::move(cp.parts), ship, options);
}

}  // namespace ksp
