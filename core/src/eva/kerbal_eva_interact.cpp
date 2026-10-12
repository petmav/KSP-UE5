// WP-32: KerbalEVA.cs part 4: EVA propellant, pack models, boarding, flag planting, seats, grapple, construction weld entry points, visor, helmet and
// pressure / temperature safety, physic material, part placement mode, and the host hooks that connect WP-33 / WP-53 / WP-25 to the real controller
// (KerbalEVA.cs:1237-1414, 6127-6225, 6615-6858, 6898-6905, 7001-7343).
#include <algorithm>
#include <cmath>

#include "ksp/career/game_variables.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/eva/kerbal_eva.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/fineprint/utilities.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/inventory/construction.hpp"
#include "ksp/inventory/inventory.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_grapple_node.hpp"
#include "ksp/modules/parachutes.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

using GE = events::GameEvents;
using KSP::Localization::Localizer;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
void Post(const std::string& text, float duration) {
    EvaEnvironment::postedMessages.push_back(text);
    if (EvaEnvironment::postScreenMessage) EvaEnvironment::postScreenMessage(text, duration, 1);
}
}  // namespace

// ============================================================================================ EVA propellant / pack models

// KerbalEVA.cs:1237
void KerbalEVA::OnModuleInventoryChanged(ModuleInventoryPart* moduleInventory) {
    if (moduleInventory != nullptr && moduleInventory == moduleInventoryPartReference) {
        UpdatePackModels();
        if (evaChute != nullptr && evaChute->deploymentState != ModuleParachute::deploymentStates::STOWED && !moduleInventory->ContainsPart("evaChute"))
            evaChute->CutParachute();
    }
}

// KerbalEVA.cs:1249
void KerbalEVA::UpdatePackModels() {
    auto set = [this](const char* name, bool on) { packModelActive[name] = on; };
    set("Backpack", false);
    set("BackpackSt", false);
    set("Storage", false);
    set("StorageSlim", false);
    set("ChuteJetpack", false);
    set("ChuteSt", false);
    set("ChuteContainer", false);
    bool flag = false;
    bool flag2 = false;
    bool flag3 = evaChute != nullptr && evaChute->CanCrewMemberUseParachute();
    ModuleInventoryPart* inv = moduleInventoryPartReference;
    if (inv == nullptr) throw Nre();
    if (inv->InventoryItemCount() > 0) {
        for (size_t i = 0; i < static_cast<size_t>(inv->storedParts.Count()); i++) {
            std::shared_ptr<StoredPart> storedPart = inv->storedParts.ValuesList()[i];
            if (storedPart != nullptr) {
                if (storedPart->partName == "evaJetpack") flag = true;
                else if (storedPart->partName == "evaChute") flag2 = true;
            }
        }
        if (inv->InventoryItemCount() == 1) {
            if (flag2) {
                if (flag3) {
                    set("ChuteSt", true);
                    if (evaChute != nullptr) EvaEnvironment::Effect("evaChute.SetCanopy:ChuteSt");
                } else {
                    set("BackpackSt", true);
                }
            } else {
                set("BackpackSt", !flag);
            }
        } else if (flag) {
            if (flag2 && flag3) {
                set("ChuteJetpack", true);
                if (evaChute != nullptr) EvaEnvironment::Effect("evaChute.SetCanopy:ChuteJetpack");
            } else {
                set("Backpack", true);
            }
        } else if (flag2 && flag3) {
            set("StorageSlim", true);
            set("ChuteContainer", true);
            if (evaChute != nullptr) EvaEnvironment::Effect("evaChute.SetCanopy:ChuteContainer");
        } else {
            set("Storage", true);
        }
    }
    ProcessEVAFuel();
    set("Jetpack", flag);
    HasJetpack = flag;
    if (!flag) ToggleJetpack(false);
    HasParachute = flag2;
    if (evaChute != nullptr) evaChute->SetEVAChuteActive(flag2 && flag3);
    Fields()["thrustPercentage"]->guiActive = flag;
}

// KerbalEVA.cs:1344
void KerbalEVA::SetupEVAFuel() {
    Part* p = PartPtr();
    const std::string name = propellantResourceName();
    if (p->Resources.Contains(name)) {
        if (moduleInventoryPartReference != nullptr && moduleInventoryPartReference->InventoryItemCount() > 0) {
            for (size_t i = 0; i < static_cast<size_t>(moduleInventoryPartReference->storedParts.Count()); i++) {
                std::shared_ptr<StoredPart> storedPart = moduleInventoryPartReference->storedParts.ValuesList()[i];
                if (storedPart == nullptr || storedPart->partName != "evaJetpack") continue;
                for (size_t j = 0; j < storedPart->snapshot->resources.size(); j++) {
                    if (storedPart->snapshot->resources[j]->resourceName == name) {
                        storedPart->snapshot->resources[j]->amount = p->Resources.Get(name)->amount;
                        break;
                    }
                }
            }
        }
        p->Resources.Remove(name);
    }
    const PartResourceDefinition* definition = PartResourceLibrary::Instance()->GetDefinition(name);
    if (definition != nullptr) {
        PartResource* added = p->AddResource(*definition->Config());
        // `base.part.Resources.Remove(name)` drops the list entry but the C# reference lives on: keep an owned copy
        propellantOwned_ = std::make_unique<PartResource>(*added);
        p->Resources.Remove(name);
        propellantResource = propellantOwned_.get();
    }
    if (propellantResource == nullptr) throw Nre();
    propellantResource->amount = 0.0;
    propellantResource->maxAmount = 0.0;
}

// KerbalEVA.cs:1379
void KerbalEVA::ProcessEVAFuel() {
    if (propellantResource == nullptr) SetupEVAFuel();
    inventoryPropellantResources.clear();
    propellantResource->amount = 0.0;
    propellantResource->maxAmount = 0.0;
    ModuleInventoryPart* inv = moduleInventoryPartReference;
    if (inv == nullptr) throw Nre();
    const std::string name = propellantResourceName();
    if (inv->InventoryItemCount() > 0) {
        for (size_t i = 0; i < static_cast<size_t>(inv->storedParts.Count()); i++) {
            std::shared_ptr<StoredPart> storedPart = inv->storedParts.ValuesList()[i];
            if (storedPart == nullptr) continue;
            for (size_t j = 0; j < storedPart->snapshot->resources.size(); j++) {
                if (storedPart->snapshot->resources[j]->resourceName == name) {
                    ProtoPartResourceSnapshot* snap = storedPart->snapshot->resources[j].get();
                    propellantResource->maxAmount += snap->maxAmount;
                    propellantResource->amount += snap->amount;
                    EvaResourceListItem item;
                    item.pPResourceSnapshot = snap;
                    item.priority = storedPart->snapshot->resourcePriorityOffset;
                    inventoryPropellantResources.push_back(item);
                }
            }
        }
    }
    // inventoryPropellantResources.Sort((x, y) => x.priority.CompareTo(y.priority)): the runtime's (unstable) introspective sort
    dotnet::ListSort(inventoryPropellantResources, [](const EvaResourceListItem& x, const EvaResourceListItem& y) {
        return x.priority < y.priority ? -1 : (x.priority > y.priority ? 1 : 0);
    });
}

// ============================================================================================ weld / visor

// KerbalEVA.cs:6127
void KerbalEVA::Weld(Part* targetPart) {
    if (EVAConstructionModeController::Instance != nullptr) {
        if (Contracts::Lite::VesselCrewWithTraitCount("Engineer", VesselPtr()) <= 0) {
            log::Error("Kerbal is not an engineer!");
            return;
        }
        constructionTarget = targetPart;
        fsm->RunEvent(On_weldStart);
    }
}

// KerbalEVA.cs:6141
void KerbalEVA::LowerVisor(bool forceHelmet) {
    if (isHelmetEnabled() || forceHelmet) {
        wasHelmetEnabledBeforeWelding = isHelmetEnabled();
        if (forceHelmet && !isHelmetEnabled()) ToggleHelmet(true);
        visorState = VisorStates::Lowering;
        UpdateVisorEventStates();
        GE::OnVisorLowering.Fire(this);
    }
}

// KerbalEVA.cs:6156
void KerbalEVA::RaiseVisor(bool restoreHelmet) {
    removeHelmetAfterRaisingVisor = !wasHelmetEnabledBeforeWelding && restoreHelmet;
    visorState = VisorStates::Raising;
    UpdateVisorEventStates();
    GE::OnVisorRaising.Fire(this);
}

// KerbalEVA.cs:6164
void KerbalEVA::UpdateVisorPosition() {
    if (visorState == VisorStates::Lowered || visorState == VisorStates::Raised) return;
    visorCurrentOffset = visorTextureOffsetY;
    visorTargetOffset = (visorState == VisorStates::Lowering) ? visorLoweredTargetOffset : visorRaisedTargetOffset;
    visorCurrentOffset = Mathf::Lerp(visorCurrentOffset, visorTargetOffset, f32store(fx(Time::deltaTime) * fx(VisorAnimationSpeed())));
    visorTextureOffsetY = visorCurrentOffset;
    if (fx(Mathf::Abs(f32store(fx(visorTargetOffset) - fx(visorCurrentOffset)))) < fx(0.001f)) {
        if (visorState == VisorStates::Lowering) {
            visorState = VisorStates::Lowered;
            set_isVisorEnabled(true);
            GE::OnVisorLowered.Fire(this);
        } else {
            visorState = VisorStates::Raised;
            set_isVisorEnabled(false);
            GE::OnVisorRaised.Fire(this);
        }
        Part* p = PartPtr();
        if (!p->protoModuleCrew.empty() && p->protoModuleCrew[0] != nullptr) p->protoModuleCrew[0]->hasVisorDown = isVisorEnabled();
    }
    UpdateVisorEventStates();
}

// KerbalEVA.cs:6199
void KerbalEVA::OnVisorRaised(KerbalEVA* kerbal) {
    if (!(kerbal != this) && removeHelmetAfterRaisingVisor) {
        removeHelmetAfterRaisingVisor = false;
        ToggleHelmet(false);
    }
}

// KerbalEVA.cs:6208
void KerbalEVA::UpdateVisorEventStates() {
    Events()["LowerVisor"]->guiActive = isHelmetEnabled() && (visorState == VisorStates::Raising || visorState == VisorStates::Raised);
    Events()["RaiseVisor"]->guiActive = isHelmetEnabled() && (visorState == VisorStates::Lowering || visorState == VisorStates::Lowered);
}

// ============================================================================================ boarding / flag / seat

// KerbalEVA.cs:6615
void KerbalEVA::BoardPart(Part* p) {
    const bool canBoard = EvaEnvironment::canBoard ? EvaEnvironment::canBoard() : true;
    if (!canBoard) {
        Post(Localizer::Format("#autoLOC_115948"), 5.0f);
    } else if (static_cast<int>(p->protoModuleCrew.size()) >= p->fields.CrewCapacity) {
        Post(Localizer::Format("#autoLOC_115954"), 5.0f);
    } else if (!processInventory(p, false)) {
        Post(Localizer::Format("#autoLOC_8002271"), 5.0f);
    } else if (checkExperiments(p)) {
        proceedAndBoard(p);
    }
}

// KerbalEVA.cs:6635
bool KerbalEVA::processInventory(Part* p, bool storeParts) {
    (void)storeParts;
    if (moduleInventoryPartReference == nullptr) return true;
    moduleInventoryPartReference->RefillEVAPropellantOnBoarding(*p);
    PartPtr()->protoModuleCrew[0]->SaveInventory(moduleInventoryPartReference);
    return true;
}

// KerbalEVA.cs:6646
bool KerbalEVA::checkExperiments(Part* p) {
    Vessel* vessel = VesselPtr();
    bool flag = false;
    int count = static_cast<int>(vessel->parts.size());
    while (count-- > 0) {
        Part* part = vessel->parts[static_cast<size_t>(count)];
        // for every module that is an IScienceDataContainer: GetScienceCount() > 0 (the container modules answer through scienceDataCount)
        if (scienceDataCount && scienceDataCount(*part) > 0) flag = true;
    }
    if (flag) {
        // ModuleScienceContainer.StoreData(list, dumpRepeats: false) on the target part's container; else the "store experiments" popup
        if (storeScienceInContainer && storeScienceInContainer(*p, *PartPtr())) return true;
        boardingPopupShown = true;
        if (popupStoreExperimentsIssue) popupStoreExperimentsIssue(p->partInfo != nullptr ? p->partInfo->title : std::string(), [this, p] { proceedAndBoard(p); });
        return false;
    }
    return true;
}

// KerbalEVA.cs:6698
void KerbalEVA::proceedAndBoard(Part* p) {
    Part* self = PartPtr();
    Vessel* vessel = VesselPtr();
    if (!self->protoModuleCrew.empty()) {
        processInventory(p, true);
        std::shared_ptr<ProtoCrewMember> pcm = self->protoModuleCrew[0];
        pcm->flightLog.AddEntryUnique(FlightLog::EntryType::BoardVessel, vessel->orbitDriver->orbit->referenceBody->bodyName);
        EvaEnvironment::Effect("SaveEVAChute");
        self->RemoveCrewmember(pcm);
        p->AddCrewmember(pcm);
        pcm->persistentID = InventoryEnvironment::uniquePersistentId ? InventoryEnvironment::uniquePersistentId() : pcm->persistentID;
        GE::onCrewBoardVessel.Fire(GE::FromToAction<Part*, Part*>(self, p));
        GE::onCrewTransferred.Fire(GE::HostedFromToAction<ProtoCrewMember*, Part*>(pcm.get(), self, p));
        Vessel::CrewWasModified(self->vessel, p->vessel);
    }
    if (p->vessel->targetObject != nullptr && p->vessel->targetObject->GetVessel() == vessel) p->vessel->targetObject = nullptr;
    FlightGlobals::ForceSetActiveVessel(p->vessel);
    EvaEnvironment::Effect("FlightInputHandler.ResumeVesselCtrlState");
    vessel->Die();
}

// KerbalEVA.cs:6723
void KerbalEVA::PlantFlag() {
    set_flagItems(flagItems() - 1);
    fsm->RunEvent(On_flagPlantStart);
}

// KerbalEVA.cs:6729
bool KerbalEVA::CanPlantFlag() {
    Vessel* vessel = VesselPtr();
    Part* p = PartPtr();
    const bool unlocked = GameVariables::Instance != nullptr
                              ? GameVariables::Instance->UnlockedEVAFlags(astronautComplexLevel ? astronautComplexLevel() : 0.0f)
                              : true;
    if (vessel->state == Vessel::State::ACTIVE && p->fields.GroundContact && flagItems() > 0 && !isRagdoll && unlocked) return !InConstructionMode;
    return false;
}

// KerbalEVA.cs:6738
void KerbalEVA::AddFlag(int flagCount) {
    set_flagItems(flagItems() + flagCount);
    Events()["PlantFlag"]->active = CanPlantFlag();
}

// KerbalEVA.cs:6745
void KerbalEVA::MakeReference() {
    if (hasReferenceTransform) VesselPtr()->SetReferenceTransform(PartPtr());
    else log::Error("[KerbalEVA Error]: No referenceTransform reference defined!");
}

// KerbalEVA.cs:6757
bool KerbalEVA::BoardSeat(IEvaSeat* seat) {
    kerbalSeat = seat;
    fsm->RunEvent(On_seatBoard);
    if (fsm->LastEvent() == On_seatBoard) {
        ejectDirection = (seat != nullptr) ? seat->EjectDirection() : Vector3f::up();
        return true;
    }
    Post(Localizer::Format("#autoLOC_116092"), 2.0f);
    return false;
}

// KerbalEVA.cs:6770
bool KerbalEVA::IsSeated() { return fsm->CurrentState() == st_seated_cmd; }

// KerbalEVA.cs:6775
void KerbalEVA::OnGrapple() { fsm->RunEvent(On_grapple); }

// KerbalEVA.cs:6781
void KerbalEVA::OnDeboardSeat() {
    if (PartPtr()->protoModuleCrew[0]->type() != ProtoCrewMember::KerbalType::Tourist) fsm->RunEvent(On_seatDeboard);
    else Post(Localizer::Format("#autoLOC_8012052"), 5.0f);
}

// KerbalEVA.cs:6793
void KerbalEVA::RestoreVesselInfo(float delay) { StartCoroutine(restoreVesselInfo_afterWait(delay)); }

// KerbalEVA.cs:6798
Enumerator KerbalEVA::restoreVesselInfo_afterWait(float delay) {
    co_yield YieldInstruction::WaitForSeconds(delay);
    Vessel* vessel = VesselPtr();
    if (vessel->rootPart == PartPtr()) {
        vessel->vesselName = kerbalVesselInfo->name;
        vessel->vesselType = VesselType::EVA;
    } else {
        log::Error("KerbalEVA Error: Trying to restore EVA vessel data, but EVA does not own the vessel");
    }
}

// KerbalEVA.cs:6812
void KerbalEVA::SwitchFocusIfActiveVesselUncontrollable(float delay) { StartCoroutine(swichFocusIfActiveVesselUncontrollable_delay(delay)); }

// KerbalEVA.cs:6817
Enumerator KerbalEVA::swichFocusIfActiveVesselUncontrollable_delay(float delay) {
    co_yield YieldInstruction::WaitForSeconds(delay);
    if (!FlightRegistry::ActiveVessel->IsControllable()) {
        FlightGlobals::ForceSetActiveVessel(VesselPtr());
        EvaEnvironment::Effect("FlightInputHandler.ResumeVesselCtrlState");
    }
}

// KerbalEVA.cs:6827
Vector3f KerbalEVA::GetEjectPoint(const Vector3f& ejectDirection_, float maxDist, float capsuleRadius, float capsuleHeight) {
    float num = f32store(fx(capsuleHeight) * fx(0.5f));
    Vector3f vector = transform().rotation * ejectDirection_ * maxDist;
    Vector3f vector2 = transform().position + vector + transform().up() * num;
    Vector3f vector3 = transform().position + vector - transform().up() * num;
    Vector3f vector4 = transform().position;
    std::vector<EvaRaycastHit> array = Engine().CapsuleCastAllIgnoreSelf(vector2, vector3, capsuleRadius, -vector.normalized(), maxDist, kDefaultEquivalent);
    if (!array.empty()) vector4 = transform().position + vector - vector.normalized() * array[0].distance;
    Vector3f vector5 = vector4;
    const Vector3f up = VesselPtr()->upAxis.ToVector3f();
    std::vector<EvaRaycastHit> array2 =
        Engine().RaycastAllIgnoreSelf(vector4 + up * maxDist, -up, f32store(fx(maxDist) + fx(num)), kDefaultEquivalent | 0x8000 | 0x80000);
    if (!array2.empty()) vector5 = array2[0].point + up * num;
    return vector5;
}

// KerbalEVA.cs:6855
void KerbalEVA::RenameVessel() { EvaEnvironment::Effect("vessel.RenameVessel"); }

// KerbalEVA.cs:6898
void KerbalEVA::PostInteractionScreenMessage(const std::string& message, float delay) {
    if (EvaEnvironment::loadedSceneIsFlight && !EvaEnvironment::mapIsEnabled && VesselPtr() == FlightRegistry::ActiveVessel) {
        EvaEnvironment::postedMessages.push_back(message);
        if (EvaEnvironment::postScreenMessage) EvaEnvironment::postScreenMessage(message, delay, 9 /*KERBAL_EVA*/);
    }
}

// ============================================================================================ helmet

// KerbalEVA.cs:7001
void KerbalEVA::InitHelmetSetup() {
    neckRingTransformExists = neckRingTransformPresent;
    helmetTransformExists = helmetTransformPresent;
    Part* p = PartPtr();
    std::shared_ptr<ProtoCrewMember> pcm = p->protoModuleCrew.empty() ? nullptr : p->protoModuleCrew[0];
    if (pcm != nullptr) {
        if (!pcm->completedFirstEVA) {
            pcm->completedFirstEVA = true;
            pcm->hasHelmetOn = GameSettings::EVA_DEFAULT_HELMET_ON;
            pcm->hasNeckRingOn = GameSettings::EVA_DEFAULT_NECKRING_ON;
            pcm->hasVisorDown = false;
        }
        set_isHelmetEnabled(pcm->hasHelmetOn);
        set_isNeckRingEnabled(pcm->hasNeckRingOn);
        set_isVisorEnabled(pcm->hasVisorDown);
        if (!CanEVAWithoutHelmet()) set_isHelmetEnabled(true);
        ToggleHelmetAndNeckRing(isHelmetEnabled(), isNeckRingEnabled(), false, true);
        OnHelmetChanged(this, isHelmetEnabled(), isNeckRingEnabled());
        visorState = IsVisorEnabled() ? VisorStates::Lowered : VisorStates::Raised;
        visorTargetOffset = (visorState == VisorStates::Lowered) ? visorLoweredTargetOffset : visorRaisedTargetOffset;
        visorTextureOffsetY = visorTargetOffset;
        UpdateVisorEventStates();
    } else {
        log::Error("Unable to get the Proto Crew Member on Kerbal Eva (InitHelmetSetup)");
    }
}

// KerbalEVA.cs:7037
void KerbalEVA::ToggleHelmet(bool enableHelmet) { ToggleHelmetAndNeckRing(enableHelmet, isNeckRingEnabled()); }

// KerbalEVA.cs:7042
void KerbalEVA::ToggleNeckRing(bool enableNeckRing) { ToggleHelmetAndNeckRing(isHelmetEnabled(), enableNeckRing); }

// KerbalEVA.cs:7047
void KerbalEVA::ToggleHelmetAndNeckRing(bool enableHelmet, bool enableNeckRing, bool storeToPCM, bool suppressTransformMessages) {
    if (Ready && FlightRegistry::ActiveVessel != VesselPtr()) return;
    if (!helmetTransformExists) {
        if (!suppressTransformMessages) log::Error("Unable to set Helmet visibility as transform is missing");
        return;
    }
    if (!enableHelmet && isHelmetEnabled() && !CanSafelyRemoveHelmet()) {
        Post(Localizer::Format("#autoLOC_8003205", std::vector<std::string>{HelmetUnsafeReason()}), 5.0f);
        return;
    }
    bool flag = false;
    bool flag2 = false;
    if (helmetActiveSelf != enableHelmet) {
        helmetActiveSelf = enableHelmet;
        if (helmetColliderPresent) {
            if (enableHelmet) {
                helmetColliderRadius = helmetColliderSetup.helmetOnRadius;
                helmetColliderCenter = helmetColliderSetup.helmetOnCenter;
            } else {
                helmetColliderRadius = helmetColliderSetup.helmetOffRadius;
                helmetColliderCenter = helmetColliderSetup.helmetOffCenter;
            }
        }
        flag = true;
    }
    if (neckRingTransformPresent && neckRingActiveSelf != (enableNeckRing || enableHelmet)) {
        neckRingActiveSelf = enableNeckRing || enableHelmet;
        flag2 = true;
    }
    set_isHelmetEnabled(enableHelmet);
    set_isNeckRingEnabled(enableNeckRing);
    if (storeToPCM) {
        Part* p = PartPtr();
        if (!p->protoModuleCrew.empty() && p->protoModuleCrew[0] != nullptr) {
            p->protoModuleCrew[0]->hasHelmetOn = isHelmetEnabled();
            p->protoModuleCrew[0]->hasNeckRingOn = isNeckRingEnabled();
        }
    }
    if (flag || flag2) {
        GE::OnHelmetChanged.Fire(this, isHelmetEnabled(), isNeckRingEnabled());
        UpdateVisorEventStates();
    }
}

// KerbalEVA.cs:7110
void KerbalEVA::ChangeHelmet() { ToggleHelmet(!isHelmetEnabled()); }

// KerbalEVA.cs:7116
void KerbalEVA::ChangeNeckRing() {
    if (!neckRingTransformPresent) return;
    ToggleNeckRing(!isNeckRingEnabled() || isHelmetEnabled());
}

// KerbalEVA.cs:7124
void KerbalEVA::OnHelmetChanged(KerbalEVA* changedKerbal, bool helmetVisible, bool neckRingVisible) {
    if (this != changedKerbal) return;
    if (!helmetTransformExists) {
        Events()["ChangeHelmet"]->guiActive = false;
        Events()["ChangeNeckRing"]->guiActive = false;
        return;
    }
    if (!neckRingTransformExists) Events()["ChangeNeckRing"]->guiActive = false;
    if (helmetVisible) {
        Events()["ChangeHelmet"]->SetGuiName("#autoLOC_6010003");
        Events()["ChangeNeckRing"]->guiActive = false;
        return;
    }
    Events()["ChangeHelmet"]->SetGuiName("#autoLOC_6010004");
    Events()["ChangeNeckRing"]->guiActive = neckRingTransformPresent;
    if (neckRingVisible) Events()["ChangeNeckRing"]->SetGuiName("#autoLOC_6010005");
    else Events()["ChangeNeckRing"]->SetGuiName("#autoLOC_6010006");
}

// KerbalEVA.cs:7158
bool KerbalEVA::CheckHelmetOffSafe(bool includeSafetyMargins, bool startEVAChecks) {
    if (!startEVAChecks && loadingBufferMaskActive && loadingBufferMaskActive()) return true;
    Vessel* vessel = VesselPtr();
    Part* p = PartPtr();
    CelestialBody* mainBody = vessel->mainBody();
    atmosExistence = mainBody->atmosphere && vessel->altitude < mainBody->atmosphereDepth;
    if (!atmosExistence) {
        helmetUnsafeReason = cacheAutoLOC_6010008;
        return false;
    }
    if (!mainBody->atmosphereContainsOxygen) {
        helmetUnsafeReason = cacheAutoLOC_8003204;
        return false;
    }
    kerbalStaticPressureAtm = (!startEVAChecks) ? p->fields.staticPressureAtm : GetPreLoadPressure(p->fields.staticPressureAtm, *mainBody);
    const double m1 = includeSafetyMargins ? fx(helmetOffMinSafePressureAtmMargin()) : 0.0;
    if (kerbalStaticPressureAtm < fx(helmetOffMinSafePressureAtm()) + m1) {
        helmetUnsafeReason = cacheAutoLOC_6010009;
        return false;
    }
    const double m2 = includeSafetyMargins ? fx(helmetOffMaxSafePressureAtmMargin()) : 0.0;
    if (kerbalStaticPressureAtm > fx(helmetOffMaxSafePressureAtm()) - m2) {
        helmetUnsafeReason = cacheAutoLOC_6006049;
        return false;
    }
    const double m3 = includeSafetyMargins ? fx(helmetOffMaxOceanPressureAtmMargin()) : 0.0;
    if ((vessel->Splashed || (startEVAChecks && vessel->altitude < 0.0 && mainBody->ocean)) && kerbalStaticPressureAtm > fx(helmetOffMaxOceanPressureAtm()) - m3) {
        helmetUnsafeReason = cacheAutoLOC_6010015;
        return false;
    }
    kerbalSkinTemp = (!startEVAChecks) ? p->fields.skinTemperature : mainBody->GetTemperature(vessel->altitude);
    kerbalInternalTemp = (!startEVAChecks) ? p->fields.temperature : mainBody->GetTemperature(vessel->altitude);
    const double t1 = includeSafetyMargins ? fx(helmetOffMinSafeTempKMargin()) : 0.0;
    const double t2 = includeSafetyMargins ? fx(helmetOffMaxSafeTempKMargin()) : 0.0;
    if (!(kerbalSkinTemp < fx(helmetOffMinSafeTempK()) + t1) && kerbalInternalTemp >= fx(helmetOffMinSafeTempK())) {
        if (!(kerbalSkinTemp > fx(helmetOffMaxSafeTempK()) - t2) && kerbalInternalTemp <= fx(helmetOffMaxSafeTempK())) {
            helmetUnsafeReason = "";
            return true;
        }
        helmetUnsafeReason = cacheAutoLOC_6010011;
        return false;
    }
    helmetUnsafeReason = cacheAutoLOC_6010010;
    return false;
}

// KerbalEVA.cs:7208
double KerbalEVA::GetPreLoadPressure(double loadValue, CelestialBody& vesselBody) {
    if (loadValue != 0.0) return loadValue;
    Vessel* vessel = VesselPtr();
    double pressureAtm = vesselBody.GetPressureAtm(vessel->altitude);
    if (!(vessel->altitude >= 0.0) && vesselBody.ocean) {
        double num = vesselBody.GeeASL * PhysicsGlobals::GravitationalAcceleration * vesselBody.Radius * vesselBody.Radius /
                     ((vesselBody.Radius + vessel->altitude) * (vesselBody.Radius + vessel->altitude));
        return pressureAtm + num * (0.0 - vessel->altitude) * vesselBody.oceanDensity * 0.009869232667160128;
    }
    return pressureAtm;
}

// KerbalEVA.cs:7223 / :7228 / :7233
bool KerbalEVA::CanEVAWithoutHelmet() { return CheckHelmetOffSafe(true, true); }
bool KerbalEVA::CanSafelyRemoveHelmet() { return CheckHelmetOffSafe(); }
bool KerbalEVA::WillDieWithoutHelmet() { return !CheckHelmetOffSafe(false); }

// KerbalEVA.cs:7238
void KerbalEVA::UpdateHelmetOffChecks() {
    Part* p = PartPtr();
    if (GameSettings::EVA_DIES_WHEN_UNSAFE_HELMET && p->State() != PartStates::DEAD && Ready) {
        if (framesDelayForHelmetDeathCounter < framesDelayForHelmetDeathCheck) {
            framesDelayForHelmetDeathCounter++;
        } else if (!isHelmetEnabled() && WillDieWithoutHelmet()) {
            Post(Localizer::Format("#autoLOC_6010013", std::vector<std::string>{p->protoModuleCrew[0]->displayName(), helmetUnsafeReason}), 5.0f);
            if (partExplode) partExplode(*p);
            else EvaEnvironment::Effect("part.explode");
        }
    }
}

// ============================================================================================ physic material / placement / colour

// KerbalEVA.cs:7254
void KerbalEVA::SetPhysicMaterial(const PhysicMaterialData& newPhysicMaterial) {
    physicMaterial = newPhysicMaterial;
    Engine().Named("physicMaterial:set");
    set_useGlobalPhysicMaterial(false);
}

// KerbalEVA.cs:7272
void KerbalEVA::ResetPhysicMaterial() {
    physicMaterial.reset();
    Engine().Named("physicMaterial:reset");
    set_useGlobalPhysicMaterial(true);
}

// KerbalEVA.cs:7290: see kerbal_eva_states.cpp (ReturnToIdle)

// KerbalEVA.cs:7296
bool KerbalEVA::SetPartPlacementMode(bool mode, ModuleInventoryPart* moduleInventoryPartReference_) {
    if (mode) {
        Vessel* v = VesselPtr();
        if (!isRagdoll && VesselUnderControl() && !OnALadder_() && (!(v != nullptr) || v->Landed)) {
            if (moduleInventoryPartReference_ != nullptr) moduleInventoryPartReference = moduleInventoryPartReference_;
            partPlacementMode = true;
            return true;
        }
        return false;
    }
    partPlacementMode = false;
    return true;
}

// KerbalEVA.cs:7315
Color KerbalEVA::GetCurrentColor_() { return Color(lightR(), lightG(), lightB()); }

// KerbalEVA.cs:7330
void KerbalEVA::OnColorChanged(const Color& color) {
    Part* p = PartPtr();
    if (!p->protoModuleCrew.empty() && p->protoModuleCrew[0] != nullptr) {
        p->protoModuleCrew[0]->lightR = color.r;
        p->protoModuleCrew[0]->lightG = color.g;
        p->protoModuleCrew[0]->lightB = color.b;
    }
    set_lightR(color.r);
    set_lightG(color.g);
    set_lightB(color.b);
    UpdateSuitColors();
}

// KerbalEVA.cs:4393
void KerbalEVA::UpdateSuitColors() {
    Part* p = PartPtr();
    std::shared_ptr<ProtoCrewMember> pcm = p->protoModuleCrew.empty() ? nullptr : p->protoModuleCrew[0];
    if (pcm == nullptr) return;
    if (suitColorChanged) {
        if (!pcm->completedFirstEVA) suitColorChanged(lightR(), lightG(), lightB());
        else suitColorChanged(pcm->lightR, pcm->lightG, pcm->lightB);
    }
    pcm->lightR = lightR();
    pcm->lightG = lightG();
    pcm->lightB = lightB();
}

// ============================================================================================ host hooks

KerbalEVA* KerbalEVA::Of(Vessel* v) {
    if (v == nullptr) return nullptr;
    if (auto* k = dynamic_cast<KerbalEVA*>(v->evaController)) return k;
    for (KerbalEVA* e : live_)
        if (e->VesselPtr() == v) return e;
    return nullptr;
}

KerbalEVA* KerbalEVA::Of(Part* p) {
    if (p == nullptr) return nullptr;
    for (KerbalEVA* e : live_)
        if (e->PartPtr() == p) return e;
    return nullptr;
}

void KerbalEVA::InstallHostHooks() {
    InventoryEnvironment::setPartPlacementMode = [](bool mode, ModuleInventoryPart* host) {
        KerbalEVA* e = host != nullptr ? Of(dynamic_cast<Part*>(host->part)) : nullptr;
        if (e == nullptr) e = Of(FlightRegistry::ActiveVessel);
        return e != nullptr ? e->SetPartPlacementMode(mode, host) : false;
    };
    InventoryEnvironment::jetpackDeployed = [] {
        KerbalEVA* e = Of(FlightRegistry::ActiveVessel);
        return e != nullptr && e->JetpackDeployed();
    };
    InventoryEnvironment::toggleJetpack = [] {
        if (KerbalEVA* e = Of(FlightRegistry::ActiveVessel)) e->ToggleJetpack();
    };
    InventoryEnvironment::alwaysShowInventory = alwaysShowInventory;
    InventoryEnvironment::weldStartDelay = [](KerbalEVA* e) { return (e != nullptr && e->weldFX != nullptr) ? e->weldFX->StartDelay : 0.0f; };
    EVAConstructionEnvironment::interruptWeld = [] {
        if (KerbalEVA* e = Of(FlightRegistry::ActiveVessel)) e->InterruptWeld();
    };
    EVAConstructionEnvironment::weld = [](Part& part2) {
        if (KerbalEVA* e = Of(FlightRegistry::ActiveVessel)) e->Weld(&part2);
    };
    EVAConstructionEnvironment::isSeated = [](Vessel& v) {
        KerbalEVA* e = Of(&v);
        return e != nullptr && e->IsSeated();
    };
    EVAConstructionEnvironment::engineerCount = [](Vessel& v) { return Contracts::Lite::VesselCrewWithTraitCount("Engineer", &v); };
    EVAConstructionModeController::hooks.evaEnterExitConstruction = [](bool enter) {
        if (KerbalEVA* e = Of(FlightRegistry::ActiveVessel)) {
            if (enter) e->EnterConstructionMode();
            else e->ExitConstructionMode();
        }
    };
    EVAConstructionModeController::hooks.activeEvaInConstructionMode = [] {
        KerbalEVA* e = Of(FlightRegistry::ActiveVessel);
        return e != nullptr && e->InConstructionMode;
    };
    EVAConstructionModeController::hooks.hasEvaController = [](Vessel& v) { return v.evaController != nullptr; };
    GrappleEnvironment::evaOnGrapple = [](Part& other) {
        if (KerbalEVA* e = Of(&other)) e->OnGrapple();
    };
}

void KerbalEVA::UninstallHostHooks() {
    InventoryEnvironment::setPartPlacementMode = nullptr;
    InventoryEnvironment::jetpackDeployed = nullptr;
    InventoryEnvironment::toggleJetpack = nullptr;
    InventoryEnvironment::weldStartDelay = nullptr;
    EVAConstructionEnvironment::interruptWeld = nullptr;
    EVAConstructionEnvironment::weld = nullptr;
    EVAConstructionEnvironment::isSeated = nullptr;
    EVAConstructionEnvironment::engineerCount = nullptr;
    EVAConstructionModeController::hooks.evaEnterExitConstruction = nullptr;
    EVAConstructionModeController::hooks.activeEvaInConstructionMode = nullptr;
    EVAConstructionModeController::hooks.hasEvaController = nullptr;
    GrappleEnvironment::evaOnGrapple = nullptr;
}

namespace detail {
void RegisterEvaModules();
}

namespace detail {
void RegisterEvaModules() {
    ModuleFactory::Register("KerbalEVA", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<KerbalEVA>(c)); });
}
}  // namespace detail

}  // namespace ksp
