// WP-16: Part.cs tree operations that split and join vessels (decouple, Couple, Undock, disconnect, SetVessel), the staging block of
// Part.Start, CheckBodyLiftAttachment, and DockedVesselInfo.cs; WP-20c added OnPartJointBreak and the data side of Die (the physics host,
// ksp/physics/vessel_physics.hpp, reacts to their GameEvents). Every function cites Part.cs:<line> (decompiled/KSP-Decompiled/
// Assembly-CSharp/) unless it names another file. Not ported (physics / render / FX packages): attachJoint.DestroyJoint /
// CreateAttachJoint / ResetJoints (CycleAutoStrut), SetCollisionIgnores, CreateRendererLists, ReleaseAutoStruts, the map object.
#include <algorithm>
#include <cctype>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/docked_vessel_info.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = GameEvents;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }

// Guid.NewGuid() (version 4), drawn from the save environment's randomness like Vessel.cs's id (vessel_transitions.cpp)
Guid NewGuid() {
    Guid g;
    for (int i = 0; i < 4; i++) {
        const uint32_t r = static_cast<uint32_t>(SaveEnvironment::Current().NewGuidHashCode());
        for (int k = 0; k < 4; k++) g.bytes[i * 4 + k] = static_cast<uint8_t>(r >> (8 * k));
    }
    g.bytes[7] = static_cast<uint8_t>((g.bytes[7] & 0x0F) | 0x40);
    g.bytes[8] = static_cast<uint8_t>((g.bytes[8] & 0x3F) | 0x80);
    return g;
}

// Enum.Parse(typeof(DefaultIcons), s): a member name, or an integer
bool ParsesAsDefaultIcon(const std::string& s) {
    if (IsDefaultIconName(s)) return true;
    int32_t v;
    return !s.empty() && dotnet::TryParseInt32(s, v);
}
}  // namespace

// Part.cs:1727-1747 (the staging block of Start; the modules' OnStart ran before it at :1727)
void Part::StartStaging() {
    if (fields.stagingIcon != std::string()) {
        if (!fields.stagingIcon || !ParsesAsDefaultIcon(*fields.stagingIcon)) {
            log::Error(fields.stagingIcon ? "Requested value '" + *fields.stagingIcon + "' was not found." : "Value cannot be null.");
            fields.stagingIcon = std::string();
        }
    }
    UpdateStageability(false, true);
    if (fields.stagingOn) CreateStageIcon();
}

// Part.cs:1964
void Part::CheckBodyLiftAttachment() {
    fields.bodyLiftOnlyUnattachedLiftActual = false;
    if (!fields.bodyLiftOnlyUnattachedLift || !fields.bodyLiftOnlyAttachName || fields.bodyLiftOnlyAttachName->empty()) return;
    AttachNode* node = FindAttachNode(*fields.bodyLiftOnlyAttachName);
    if (node == nullptr) throw Nre();
    Part* attachedPart = node->attachedPart;
    if (attachedPart == nullptr) return;
    // bodyLiftOnlyProvider = null; the first ILiftProvider module of the attached part becomes the provider (aero package: WP-21)
    fields.bodyLiftOnlyUnattachedLiftActual = false;
}

// Part.cs:3263
void Part::SetVessel(Vessel* v) {
    v->parts.push_back(this);
    vessel = v;
    for (size_t i = 0, count = children.size(); i < count; i++) children[i]->SetVessel(v);
}

// Part.cs:3084
void Part::disconnect(bool controlledSeparation) {
    connected_ = false;
    onDisconnect();
    if (vessel == nullptr) throw Nre();
    auto it = std::find(vessel->parts.begin(), vessel->parts.end(), this);
    if (it != vessel->parts.end()) vessel->parts.erase(it);
    vessel->RemoveCrewList(protoModuleCrew, true);
    for (size_t i = 0, count = children.size(); i < count; i++) children[i]->disconnect(controlledSeparation);
    if (controlledSeparation) RemoveStageIcon();
}

// Part.cs:2840
void Part::Decoupling(float breakForce) {
    GE::onPartDeCouple.Fire(this);
    if (fields.physicalSignificance != static_cast<int32_t>(Part_PhysicalSignificance::FULL)) PromoteToPhysicalPart();
    onDecouple(breakForce);
    // if (attachJoint) attachJoint.DestroyJoint(): joints are the physics package's
    disconnect(breakForce == 0.0f);
    GE::onVesselPartCountChanged.Fire(vessel);
    attached_ = false;
    AttachNode* attachNode = FindAttachNodeByPart(parent);
    if (attachNode != nullptr) {
        attachNode->attachedPart = nullptr;
        for (size_t i = 0, count = modules_.Count(); i < count; i++)
            if (auto* a = dynamic_cast<IActivateOnDecouple*>(modules_[i])) a->DecoupleAction(attachNode->id, true);
    }
    if (parent == nullptr) return;
    AttachNode* attachNode2 = parent->FindAttachNodeByPart(this);
    if (attachNode2 == nullptr) return;
    for (size_t j = 0, count2 = parent->modules_.Count(); j < count2; j++)
        if (auto* a = dynamic_cast<IActivateOnDecouple*>(parent->modules_[j])) a->DecoupleAction(attachNode2->id, false);
    attachNode2->attachedPart = nullptr;
}

// Part.cs:2804. The new Vessel takes the unique_ptr ownership of the parts that moved (Vessel::SpawnedVessels owns the Vessel).
Vessel* Part::decouple(float breakForce) {
    Decoupling(breakForce);
    Vessel* oldVessel = vessel;
    oldVessel->initialPosVelSet = false;
    const std::string vesselName = oldVessel->vesselName;
    Vessel* vessel2 = Vessel::SpawnVessel();  // base.gameObject.AddComponent<Vessel>()
    vessel2->id = NewGuid();
    vessel2->rootPart = this;  // Vessel.Initialize: rootPart = GetComponent<Part>() (Vessel.cs:1661)
    if (vessel2->Initialize()) {
        vessel2->AcceptOwnedParts(oldVessel->ReleaseOwnedParts(vessel2->parts));
        if (!vessel2->UpdateVesselNaming()) vessel2->vesselName = Vessel::AutoRename(*vessel2, vesselName);
        vessel2->IgnoreGForces(10);
        vessel2->ctrlState.CopyFrom(oldVessel->ctrlState);
        vessel2->ActionGroups().CopyFrom(oldVessel->ActionGroups());
        vessel2->CopyOverrides(*oldVessel);
        vessel2->currentStage = StageManager::RecalculateVesselStaging(vessel2);
        vessel2->launchedFrom = oldVessel->launchedFrom;
        vessel2->landedAt = oldVessel->landedAt;
        setParent();
    }
    oldVessel->checkLanded();
    oldVessel->FallBackReferenceTransform();
    CleanSymmetryVesselReferencesRecursively();
    vessel2->GetDiscoveryInfo().SetUntrackedObjectSize(oldVessel->GetDiscoveryInfo().objectSize());
    GE::onVesselWasModified.Fire(oldVessel);
    GE::onPartDeCoupleComplete.Fire(this);
    GE::onPartDeCoupleNewVesselComplete.Fire(oldVessel, vessel2);
    // CreateRendererLists(): render
    CheckBodyLiftAttachment();
    oldVessel->CheckAirstreamShields();
    vessel2->CheckAirstreamShields();
    return vessel2;
}

// Part.cs:3159 (WP-20c: called by the physics host for a broken attach joint, PartJoint.cs:408)
void Part::OnPartJointBreak(float breakForce) {
    if (vessel == nullptr) throw Nre();
    if (vessel->rootPart == this) return;
    if (breakForce > 0.0f && parent != nullptr) {
        const std::string title = partInfo != nullptr ? partInfo->title : std::string();
        const std::string parentTitle = parent->partInfo != nullptr ? parent->partInfo->title : std::string();
        EventReport report(FlightEvents::JOINTBREAK, this, title, parentTitle);
        GE::onJointBreak.Fire(&report);
    }
    decouple(breakForce);
}

// Part.cs:4409 (WP-20c; the data side, see part.hpp)
void Part::Die() {
    if (state_ == PartStates::DEAD) return;
    GE::onPartWillDie.Fire(this);
    const std::vector<Part*> array(children);  // children.ToArray()
    for (size_t i = 0, num = array.size(); i < num; i++) {
        Part* part = array[i];
        if (part->fields.physicalSignificance == static_cast<int32_t>(Part_PhysicalSignificance::FULL)) {
            if (part->State() != PartStates::DEAD) part->decouple();
        } else {
            part->Die();
        }
    }
    disconnect();
    GE::onVesselPartCountChanged.Fire(vessel);
    if (FindAttachNodeByPart(parent) != nullptr) FindAttachNodeByPart(parent)->attachedPart = nullptr;
    if (parent != nullptr && parent->FindAttachNodeByPart(this) != nullptr) parent->FindAttachNodeByPart(this)->attachedPart = nullptr;
    vessel->checkLanded();
    vessel->FallBackReferenceTransform();
    setParent();
    CleanSymmetryReferences();
    deactivate();
    attached_ = false;
    OnJustAboutToDie();
    // BroadcastMessage("OnPartDie") (KerbalEVA.OnPartDie): the engine's components
    GE::onVesselWasModified.Fire(vessel);
    if (vessel->rootPart == this) vessel->Die();
    // FlightCamera.ClearTarget, gameObject.SetActive(false), onPartExplode(), stackIcon.ClearInfoBoxes: engine / UI
    RemoveStageIcon();  // stackIcon.RemoveIcon()
    GE::onPartDie.Fire(this);
    // FULL: CollisionEnhancer / terrainCollider / PartBuoyancy destroyed, attachJoint.DestroyJoint(): the physics host's (onPartDie)
    if (!protoModuleCrew.empty()) {
        Game* game = HighLogic::CurrentGame();
        for (size_t j = 0, count = protoModuleCrew.size(); j < count; j++) {
            ProtoCrewMember& protoCrewMember = *protoModuleCrew[j];
            protoCrewMember.UnregisterExperienceTraits(*this);
            protoCrewMember.Die();
            if (game == nullptr || game->Parameters == nullptr) throw Nre();
            if (game->Parameters->Difficulty.MissingCrewsRespawn) {
                if (fields.crewRespawnTime > 0.0) protoCrewMember.StartRespawnPeriod(fields.crewRespawnTime);
                else if (fields.crewRespawnTime == 0.0) protoCrewMember.StartRespawnPeriod();
            }
        }
        Vessel::CrewWasModified(vessel);
    }
    // DespawnIVA(), IOUtils.RemoveFlightData(vessel.id), Object.Destroy(gameObject) (not the root): engine
    state_ = PartStates::DEAD;
}

// Part.cs:3171
void Part::Couple(Part* tgtPart) {
    GE::onPartCouple.Fire(GE::FromToAction<Part*, Part*>(this, tgtPart));
    Vessel* srcVessel = vessel;
    srcVessel->ClearStaging();
    tgtPart->vessel->ClearStaging();
    const std::vector<Part*> moved(vessel->parts);
    for (size_t i = 0, count = moved.size(); i < count; i++) {
        Part* part = moved[i];
        part->attached_ = true;
        part->connected_ = true;
        part->UpdateOrgPosAndRot(tgtPart->vessel->rootPart);
    }
    for (size_t j = 0, count2 = tgtPart->vessel->parts.size(); j < count2; j++) {
        Part* part2 = tgtPart->vessel->parts[j];
        part2->attached_ = true;
        part2->connected_ = true;
    }
    SetHierarchyRoot(this);
    setParent(tgtPart);
    // CreateAttachJoint(tgtPart.attachMode); ResetJoints(): joints
    SetVessel(tgtPart->vessel);
    tgtPart->vessel->AcceptOwnedParts(srcVessel->ReleaseOwnedParts(moved));
    GE::onVesselPartCountChanged.Fire(tgtPart->vessel);
    vessel->vesselType = vessel->FindDefaultVesselType();
    srcVessel->OnJustAboutToBeDestroyed();
    GE::onVesselPersistentIdChanged.Fire(srcVessel->persistentId, tgtPart->vessel->persistentId);
    Vessel::DestroyImmediate(srcVessel);
    vessel->checkLanded();
    // SetCollisionIgnores() of every part: physics
    vessel->currentStage = StageManager::RecalculateVesselStaging(vessel) + 1;
    GE::onVesselWasModified.Fire(vessel);
    GE::onPartCoupleComplete.Fire(GE::FromToAction<Part*, Part*>(this, tgtPart));
    CheckBodyLiftAttachment();
}

// Part.cs:3274
Vessel* Part::Undock(const DockedVesselInfo* newVesselInfo) {
    GE::onPartUndock.Fire(this);
    // if (attachJoint) attachJoint.DestroyJoint(): joints
    Vessel* oldVessel = vessel;
    oldVessel->ClearStaging();
    Part* part = newVesselInfo != nullptr ? oldVessel->byFlightID(newVesselInfo->rootPartUId) : nullptr;
    setParent();
    bool flag;
    if ((flag = part != nullptr && hasIndirectChild(part))) {
        part->SetHierarchyRoot(part);
    } else {
        part = this;
    }
    Vessel* vessel2 = Vessel::SpawnVessel();
    vessel2->id = NewGuid();
    vessel2->rootPart = part;
    vessel2->Initialize();
    vessel2->AcceptOwnedParts(oldVessel->ReleaseOwnedParts(vessel2->parts));
    vessel2->orbit().referenceBody = FlightGlobals::getMainBody(Vector3d(part->partTransform.position));
    if (!vessel2->UpdateVesselNaming()) {
        if (!flag && (newVesselInfo == nullptr || newVesselInfo->vesselType != VesselType::EVA)) {
            vessel2->vesselName = Vessel::AutoRename(*vessel2, oldVessel->vesselName);
        } else {
            if (newVesselInfo == nullptr) throw Nre();
            vessel2->vesselName = newVesselInfo->name;
            vessel2->vesselType = newVesselInfo->vesselType;
            if (static_cast<int>(vessel2->vesselType) <= static_cast<int>(VesselType::Unknown)) {
                VesselType vesselType = VesselType::Debris;
                int count = static_cast<int>(vessel2->parts.size());
                while (count-- > 0) {
                    const VesselType pt = static_cast<VesselType>(vessel2->parts[static_cast<size_t>(count)]->fields.vesselType);
                    if (static_cast<int>(pt) > static_cast<int>(vesselType)) vesselType = pt;
                }
                if (static_cast<int>(vesselType) > static_cast<int>(vessel2->vesselType)) vessel2->vesselType = vesselType;
            }
        }
    }
    vessel2->IgnoreGForces(10);
    vessel2->currentStage = StageManager::RecalculateVesselStaging(vessel2) + 1;
    vessel2->launchedFrom = oldVessel->launchedFrom;
    for (size_t i = 0, count2 = vessel2->parts.size(); i < count2; i++) {
        auto it = std::find(oldVessel->parts.begin(), oldVessel->parts.end(), vessel2->parts[i]);
        if (it != oldVessel->parts.end()) oldVessel->parts.erase(it);
    }
    GE::onVesselPartCountChanged.Fire(oldVessel);
    // SetCollisionIgnores() of every new part: physics
    oldVessel->delayedFrameCalls.push_back([oldVessel] { oldVessel->ResumeStaging(); });  // CallbackUtil.DelayedCallback(1, ...)
    oldVessel->checkLanded();
    oldVessel->FallBackReferenceTransform();
    GE::onVesselWasModified.Fire(oldVessel);
    GE::onPartUndockComplete.Fire(this);
    GE::onVesselsUndocking.Fire(oldVessel, vessel2);
    return vessel2;
}

// ---- DockedVesselInfo.cs ------------------------------------------------------------------------------------------------------

// DockedVesselInfo.cs:31
void DockedVesselInfo::Load(const ConfigNode& node) {
    if (node.HasValue("vesselName")) name = *node.GetValue("vesselName");
    if (node.HasValue("rootUId")) {
        uint32_t v = 0;
        if (!dotnet::TryParseUInt32(*node.GetValue("rootUId"), v)) throw dotnet::FormatException("Input string was not in a correct format.");  // uint.Parse
        rootPartUId = v;
    }
    if (!node.HasValue("vesselType")) return;
    int32_t result;
    if (dotnet::TryParseInt32(*node.GetValue("vesselType"), result)) {
        switch (static_cast<OldVesselType>(result)) {
            case OldVesselType::Debris: vesselType = VesselType::Debris; break;
            case OldVesselType::SpaceObject: vesselType = VesselType::SpaceObject; break;
            case OldVesselType::Unknown: vesselType = VesselType::Unknown; break;
            case OldVesselType::Probe: vesselType = VesselType::Probe; break;
            case OldVesselType::Rover: vesselType = VesselType::Rover; break;
            case OldVesselType::Lander: vesselType = VesselType::Lander; break;
            default: vesselType = VesselType::Ship; break;
            case OldVesselType::Station: vesselType = VesselType::Station; break;
            case OldVesselType::Base: vesselType = VesselType::Base; break;
            case OldVesselType::EVA: vesselType = VesselType::EVA; break;
            case OldVesselType::Flag: vesselType = VesselType::Flag; break;
            case OldVesselType::Plane: vesselType = VesselType::Plane; break;
            case OldVesselType::Relay: vesselType = VesselType::Relay; break;
            case OldVesselType::DeployedScienceController: vesselType = VesselType::DeployedScienceController; break;
            case OldVesselType::DeployedSciencePart: vesselType = VesselType::DeployedSciencePart; break;
        }
    } else {
        vesselType = dotnet::EnumParse<VesselType>(*node.GetValue("vesselType"));
    }
}

// DockedVesselInfo.cs:96
void DockedVesselInfo::Save(ConfigNode& node) const {
    node.AddValue("vesselName", name);
    node.AddValue("vesselType", dotnet::EnumToString(vesselType));
    node.AddValue("rootUId", dotnet::FormatUInt64(rootPartUId));
}

}  // namespace ksp
