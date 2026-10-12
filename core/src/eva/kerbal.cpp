// WP-32: Kerbal.cs, CrewTransfer.cs, CrewHatchController.cs (see include/ksp/eva/kerbal.hpp). Every function cites its C# line.
#include "ksp/eva/kerbal.hpp"

#include <algorithm>

#include "ksp/career/game_variables.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/coroutine_host.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/eva/flight_eva.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/science/science.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/misc_util.hpp"

namespace ksp {

using GE = events::GameEvents;
using KSP::Localization::Localizer;

// ============================================================================================ Kerbal

Vessel* Kerbal::InVessel() const { return InPart != nullptr ? InPart->vessel : nullptr; }

// Kerbal.cs:116
Kerbal::Kerbal() {
    running = false;
    GE::onVesselSituationChange.Add(this, &Kerbal::onVesselSitChange);
}

// Kerbal.cs:135 (DestroyAvatarTexture / gallery unregister are the shell's)
Kerbal::~Kerbal() {
    if (KerbalEnvironment::portraitGallery && portraitRegistered) KerbalEnvironment::portraitGallery(*this, false);
    GE::onVesselSituationChange.Remove(this, &Kerbal::onVesselSitChange);
    headRendererEnabled.clear();
}

// Kerbal.cs:153
void Kerbal::Start() {
    running = true;
    state = States::NO_SIGNAL;
    KerbalEnvironment::Effect("avatarTexture:create");
    if (KerbalEnvironment::portraitGallery) {
        KerbalEnvironment::portraitGallery(*this, true);
        portraitRegistered = true;
    }
    SetupKerbalModel();
    if (randomizeOnStartup) {
        UnityRandom::Global().InitState(StringExtensions::GetHashCode_Net35(crewMemberName));
        updateInterval = UnityRandom::Global().Range(0.1f, 0.15f);
        noiseSeed = static_cast<float>(UnityRandom::Global().Range(0, 20));
    }
    if (rosterStatus == ProtoCrewMember::RosterStatus::Dead) state = States::DEAD;
    if (visibleInPortrait) updateCoroutineRunning = true;
    stateTimer = ScienceEnvironment::Scheduler().StartCoroutine(CallbackUtil::DelayedCallback(
                                                                     staticOverlayDuration, Callback<>::Lambda(this, [this] {
                                                                         if (running) state = States::ALIVE;
                                                                     })),
                                                                 static_cast<int>(reinterpret_cast<uintptr_t>(this) >> 4));
}

bool Kerbal::UsesVeteranTexture() const {
    if (protoCrewMember == nullptr) throw NullReferenceException("Object reference not set to an instance of an object");
    return protoCrewMember->veteran;
}

// Kerbal.cs:200
void Kerbal::SetupKerbalModel() {
    int num = textureTargets;
    textureTargetUsesVeteran.assign(static_cast<size_t>(std::max(0, num)), false);
    while (num-- > 0) textureTargetUsesVeteran[static_cast<size_t>(num)] = UsesVeteranTexture();
    if (!capturedInitial) {
        capturedInitial = true;
        eyeInitialPos = eyeLocalPosition;
    }
}

// Kerbal.cs:222
void Kerbal::ShowHelmet(bool show) {
    if (show) {
        showHelmet = true;
        return;
    }
    showHelmet = false;
    if (helmetTransformPresent) helmetDestroyed = true;
}

// Kerbal.cs:240 (the Euler angles of the camera bob; Mathf.PerlinNoise is native and the shell's)
Vector3f Kerbal::SeatCamBobEuler(float perlinX, float perlinY, float perlinZ, double shipAccelerationMagnitude, double shipAngularVelocityMagnitude, float camBobAmount) {
    const float num = f32store(fx(perlinX) - fx(0.5f));
    const float num2 = f32store(fx(perlinY) - fx(0.5f));
    const float num3 = f32store(fx(perlinZ) - fx(0.5f));
    const float x = f32store(fx(num) * fx(camBobAmount) * fx(Mathf::Clamp(static_cast<float>(shipAccelerationMagnitude) * 5.0f, 0.0f, 50.0f)) * fx(0.4f));
    const float y = f32store(fx(num2) * fx(camBobAmount) * fx(Mathf::Clamp(static_cast<float>(shipAccelerationMagnitude) * 5.0f, 0.0f, 50.0f)) * fx(0.4f));
    const float z = f32store(fx(num3) * fx(camBobAmount) * fx(Mathf::Clamp(static_cast<float>(shipAngularVelocityMagnitude), 0.0f, 100.0f)) * fx(0.5f));
    return Vector3f(x, y, z);
}

// Kerbal.cs:269
void Kerbal::die() {
    if (KerbalEnvironment::portraitCrewDie && portraitRegistered) KerbalEnvironment::portraitCrewDie(*this);
    state = States::DEAD;
    rosterStatus = ProtoCrewMember::RosterStatus::Dead;
}

// Kerbal.cs:279
void Kerbal::onVesselSitChange(GE::HostedFromToAction<Vessel*, Vessel_Situations> evt) {
    if (evt.host == FlightRegistry::ActiveVessel) {
        CheckEVAUnlocked();
        protoCrewMember->UpdateExperience();
    }
}

// Kerbal.cs:288
void Kerbal::CheckEVAUnlocked() {
    GameVariables* gv = GameVariables::Instance;
    if (gv == nullptr) throw NullReferenceException("Object reference not set to an instance of an object");
    EVAisUnlocked = gv->UnlockedEVA(astronautComplexLevel ? astronautComplexLevel() : 0.0f);
    EVAisUnlocked = gv->EVAIsPossible(EVAisUnlocked, FlightRegistry::ActiveVessel);
}

// Kerbal.cs:294
void Kerbal::IVAEnable(bool setIVAMode) {
    size_t num = headRendererEnabled.size();
    while (num-- > 0) headRendererEnabled[num] = false;
    if (setIVAMode) ivaMode = true;
}

// Kerbal.cs:308
void Kerbal::IVADisable(bool setIVAMode) {
    size_t num = headRendererEnabled.size();
    while (num-- > 0) headRendererEnabled[num] = true;
    if (setIVAMode) ivaMode = false;
}

// Kerbal.cs:322
void Kerbal::SetVisibleInPortrait(bool visible) {
    if (visible) {
        if (!visibleInPortrait) {
            visibleInPortrait = true;
            if (!updateCoroutineRunning) updateCoroutineRunning = true;
        }
    } else if (visibleInPortrait) {
        visibleInPortrait = false;
        if (updateCoroutineRunning) updateCoroutineRunning = false;
    }
}

// ============================================================================================ CrewTransfer

// CrewTransfer.cs:21
std::unique_ptr<CrewTransfer> CrewTransfer::Create(Part& srcPart, std::shared_ptr<ProtoCrewMember> crewMember,
                                                    std::function<void(DismissAction, Part*)> onDialogDismiss) {
    auto crewTransfer = std::make_unique<CrewTransfer>();
    crewTransfer->crew = crewMember;
    crewTransfer->Setup(srcPart, "#autoLOC_6002372", crewMember->name().value_or(""), Localizer::Format("#autoLOC_111558"), std::move(onDialogDismiss));
    return crewTransfer;
}

CrewTransfer::~CrewTransfer() {
    if (completion && ScienceEnvironment::scheduler != nullptr) ScienceEnvironment::scheduler->StopCoroutine(completion);
}

// CrewTransfer.cs:29 / :35
void CrewTransfer::HookAdditionalEvents() {
    GE::onCrewTransferred.Add(this, &CrewTransfer::onGoForEva);
    GE::onKerbalInactiveChange.Add(this, &CrewTransfer::onChangeInactive);
}
void CrewTransfer::UnhookAdditionalEvents() {
    GE::onCrewTransferred.Remove(this, &CrewTransfer::onGoForEva);
    GE::onKerbalInactiveChange.Remove(this, &CrewTransfer::onChangeInactive);
}

// CrewTransfer.cs:41
bool CrewTransfer::IsSemiValidPart(Part& p) {
    if (p.fields.CrewCapacity > 0 && static_cast<int>(p.protoModuleCrew.size()) >= p.fields.CrewCapacity) return p.fields.crewTransferAvailable;
    return false;
}

// CrewTransfer.cs:49
bool CrewTransfer::IsValidPart(Part& p) {
    if (p.fields.CrewCapacity > 0 && static_cast<int>(p.protoModuleCrew.size()) < p.fields.CrewCapacity) return p.fields.crewTransferAvailable;
    return false;
}

// CrewTransfer.cs:55
void CrewTransfer::AfterPartsFound() {
    GE::onCrewTransferPartListCreated.Fire(GE::HostedFromToAction<Part*, std::vector<Part*>*>(srcPart, &validParts, &semiValidParts));
}

// CrewTransfer.cs:56
void CrewTransfer::OnPartSelect(Part* p) {
    CrewTransferData data;
    data.canTransfer = true;
    data.crewMember = crew;
    data.sourcePart = srcPart;
    data.destPart = p;
    data.transfer = this;
    GE::onCrewTransferSelected.Fire(&data);
    if (data.canTransfer) MoveCrewTo(p);
}

// CrewTransfer.cs:70
void CrewTransfer::onGoForEva(GE::HostedFromToAction<ProtoCrewMember*, Part*> fromto) {
    if (fromto.host == crew.get() && fromto.to != nullptr && KerbalEVA::Of(fromto.to) != nullptr) DismissInterrupted();
}

// CrewTransfer.cs:78
void CrewTransfer::onChangeInactive(ProtoCrewMember* pcm, bool, bool to) {
    if (pcm == crew.get() && to) DismissInterrupted();
}

// CrewTransfer.cs:86
void CrewTransfer::MoveCrewTo(Part* p) {
    srcPart->RemoveCrewmember(crew);
    p->AddCrewmember(crew);
    screenMessages.push_back(Localizer::Format("#autoLOC_111636", std::vector<std::string>{crew->name().value_or(""), p->partInfo != nullptr ? p->partInfo->title : std::string()}));
    GE::onCrewTransferred.Fire(GE::HostedFromToAction<ProtoCrewMember*, Part*>(crew.get(), srcPart, p));
    Vessel::CrewWasModified(srcPart->vessel, p->vessel);
    tgtPart = p;
    if (despawnCrew && FlightRegistry::ActiveVessel != nullptr) despawnCrew(*FlightRegistry::ActiveVessel);
    completion = ScienceEnvironment::Scheduler().StartCoroutine(
        CallbackUtil::DelayedCallback(1, Callback<>::Lambda(this, [this] { waitAndCompleteTransfer(); })), static_cast<int>(reinterpret_cast<uintptr_t>(this) >> 4));
}

// CrewTransfer.cs:104
void CrewTransfer::waitAndCompleteTransfer() {
    if (spawnCrew && FlightRegistry::ActiveVessel != nullptr) spawnCrew(*FlightRegistry::ActiveVessel);
    Dismiss(DismissAction::ItemMoved, tgtPart);
}

// ============================================================================================ CrewHatchController

void CrewHatchEnvironment::Reset() {
    raycast = nullptr; flightDriverPause = nullptr; pointerOverUI = nullptr; cursorLocked = nullptr; mouseLeftClick = nullptr; showTooltip = nullptr;
    spawnDialog = nullptr; terminateDialog = nullptr; dialogPart = nullptr; messages.clear();
}

// CrewHatchController.cs:57
CrewHatchController::CrewHatchController() {
    if (fetch != nullptr) log::Error("[CrewHatchController]: Attempting to create another instance of singleton");
    fetch = this;
    GE::onVesselChange.Add(this, &CrewHatchController::OnVesselSwitch);
    GE::onVesselSituationChange.Add(this, &CrewHatchController::onVesselSitChange);
    GE::onGamePause.Add(this, &CrewHatchController::DespawnUIs);
    GE::onGameUnpause.Add(this, &CrewHatchController::DespawnUIs);
    interfaceEnabled = true;
}

// CrewHatchController.cs:71
CrewHatchController::~CrewHatchController() {
    GE::onGameUnpause.Remove(this, &CrewHatchController::DespawnUIs);
    GE::onGamePause.Remove(this, &CrewHatchController::DespawnUIs);
    GE::onVesselChange.Remove(this, &CrewHatchController::OnVesselSwitch);
    GE::onVesselSituationChange.Remove(this, &CrewHatchController::onVesselSitChange);
    DespawnUIs();
    if (fetch != nullptr && fetch == this) fetch = nullptr;
}

// CrewHatchController.cs:85
void CrewHatchController::LateUpdate() {
    if (CrewHatchEnvironment::flightDriverPause && CrewHatchEnvironment::flightDriverPause()) return;
    const bool pointerOver = CrewHatchEnvironment::pointerOverUI && CrewHatchEnvironment::pointerOverUI();
    const bool locked = CrewHatchEnvironment::cursorLocked && CrewHatchEnvironment::cursorLocked();
    if (!crewDialogOpen && crewTransfer == nullptr && !pointerOver && !locked && interfaceEnabled) {
        hoveredPart = nullptr;
        rayHit = CrewHatchEnvironment::raycast ? CrewHatchEnvironment::raycast() : CrewHatchEnvironment::Hit{};
        if (rayHit.hit && rayHit.airlockTag) hoveredPart = rayHit.part;
        if (hoveredPart != nullptr) {
            ShowTooltip();
            TooltipUpdate(*hoveredPart);
        } else {
            HideTooltip();
        }
    } else {
        HideTooltip();
    }
}

// CrewHatchController.cs:112
void CrewHatchController::ShowTooltip() {
    if (!tooltipVisible) {
        tooltipVisible = true;
        if (CrewHatchEnvironment::showTooltip) CrewHatchEnvironment::showTooltip(true);
    }
}

// CrewHatchController.cs:121
void CrewHatchController::TooltipUpdate(Part& hp) {
    if (CrewHatchEnvironment::mouseLeftClick && CrewHatchEnvironment::mouseLeftClick()) SpawnCrewDialog(hp, true, true);
}

// CrewHatchController.cs:134
void CrewHatchController::HideTooltip() {
    if (tooltipVisible) {
        tooltipVisible = false;
        if (CrewHatchEnvironment::showTooltip) CrewHatchEnvironment::showTooltip(false);
    }
}

// CrewHatchController.cs:143
void CrewHatchController::SpawnCrewDialog(Part& part, bool showEVA, bool showTransfer) {
    dialogPart = &part;
    crewDialogOpen = true;
    if (CrewHatchEnvironment::spawnDialog) CrewHatchEnvironment::spawnDialog(part, showEVA, showTransfer);
}

// CrewHatchController.cs:150
void CrewHatchController::OnEVABtn(std::shared_ptr<ProtoCrewMember> crew) {
    GameVariables* gv = GameVariables::Instance;
    if (gv == nullptr) throw NullReferenceException("Object reference not set to an instance of an object");
    const float level = 0.0f;
    bool evaUnlocked = gv->UnlockedEVA(level);
    if ((evaUnlocked = gv->EVAIsPossible(evaUnlocked, FlightRegistry::ActiveVessel)) && crew->type() != ProtoCrewMember::KerbalType::Tourist && !crew->inactive()) {
        const PartTransform* transform = nullptr;
        if (rayHit.hit && rayHit.colliderTransform != nullptr) transform = rayHit.colliderTransform;
        else if (FlightEvaEnvironment::partAirlock && dialogPart != nullptr) transform = FlightEvaEnvironment::partAirlock(*dialogPart);
        if (transform != nullptr) {
            if (FlightEVA::fetch == nullptr) throw NullReferenceException("Object reference not set to an instance of an object");
            FlightEVA::fetch->spawnEVA(crew, dialogPart, transform);
            DismissDialog();
        } else {
            CrewHatchEnvironment::messages.push_back(gv->GetEVALockedReason(FlightRegistry::ActiveVessel, crew.get()));
        }
    } else {
        CrewHatchEnvironment::messages.push_back(gv->GetEVALockedReason(FlightRegistry::ActiveVessel, crew.get()));
    }
}

// CrewHatchController.cs:174
void CrewHatchController::OnTransferBtn(std::shared_ptr<ProtoCrewMember> crew) {
    overrideTransfer = false;
    GE::onAttemptTransfer.Fire(crew.get(), dialogPart, this);
    if (!overrideTransfer) {
        crewTransfer = CrewTransfer::Create(*dialogPart, crew, [this](PartItemTransfer::DismissAction dma, Part* p) { OnCrewTransferDone(dma, p); });
    }
    DismissDialog();
}

// CrewHatchController.cs:184
void CrewHatchController::DismissDialog() {
    if (crewDialogOpen) {
        crewDialogOpen = false;
        if (CrewHatchEnvironment::terminateDialog) CrewHatchEnvironment::terminateDialog();
    }
}

// CrewHatchController.cs:196
void CrewHatchController::DespawnUIs() {
    DismissDialog();
    HideTooltip();
}

// CrewHatchController.cs:202
void CrewHatchController::OnVesselSwitch(Vessel*) { DespawnUIs(); }

// CrewHatchController.cs:207
void CrewHatchController::onVesselSitChange(GE::HostedFromToAction<Vessel*, Vessel_Situations> evt) {
    if (evt.host == FlightRegistry::ActiveVessel) DismissDialog();
}

// CrewHatchController.cs:215
void CrewHatchController::OnCrewTransferDone(PartItemTransfer::DismissAction, Part*) {
    crewTransfer.reset();
    DismissDialog();
}

// CrewHatchController.cs:222 / :228 / :234
void CrewHatchController::EnableInterface() {
    DismissDialog();
    interfaceEnabled = true;
}
void CrewHatchController::DisableInterface() {
    DismissDialog();
    interfaceEnabled = false;
}
void CrewHatchController::ShowHatchTooltip(bool show) { interfaceEnabled = show; }

}  // namespace ksp
