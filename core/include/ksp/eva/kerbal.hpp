// WP-32: Kerbal.cs (the IVA / portrait kerbal component: state, avatar update bookkeeping, helmet, IVA renderer toggling, EVA-unlock checks),
// CrewTransfer.cs (the crew transfer PartItemTransfer) and CrewHatchController.cs (the hatch click -> EVA / transfer dialog rules).
// Rendering (RenderTexture, cameras, tooltips, dialogs, Perlin camera bob) is the shell's: seams KerbalEnvironment and CrewHatchEnvironment.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/eva/eva_engine.hpp"
#include "ksp/inventory/construction.hpp"
#include "ksp/save/crew.hpp"

namespace ksp {

class CrewHatchController;
class Kerbal;

// Kerbal.States (Kerbal.cs:9)
enum class KerbalStates : int { BAILED_OUT, DEAD, NO_SIGNAL, ALIVE };

struct KerbalEnvironment {
    // KerbalPortraitGallery.RegisterActiveCrew / UnregisterActiveCrew (by kerbal); OnCrewDie
    static inline std::function<void(Kerbal&, bool /*register*/)> portraitGallery;
    static inline std::function<void(Kerbal&)> portraitCrewDie;
    // the camera render calls of kerbalAvatarUpdate / kerbalSeatCamUpdate (named effects: "render:avatar" / "render:seat")
    static inline std::function<void(const std::string&)> effect;
    static inline std::vector<std::string> effects;
    static void Reset() {
        portraitGallery = nullptr; portraitCrewDie = nullptr; effect = nullptr; effects.clear();
    }
    static void Effect(const std::string& s) {
        effects.push_back(s);
        if (effect) effect(s);
    }
};

class Kerbal {
public:
    using States = KerbalStates;
    std::string crewMemberName;            // :14
    float courage = 0.0f, stupidity = 0.0f;  // :16-:18
    bool isBadass = false, veteran = false;  // :20-:22
    ProtoCrewMember::RosterStatus rosterStatus{};  // :24
    Part* InPart = nullptr;                // :26
    std::shared_ptr<ProtoCrewMember> protoCrewMember;  // :28
    float noiseSeed = 0.0f;                // :30
    float camBobAmount = 10.0f;            // :32
    States state = States::BAILED_OUT;     // :44
    float staticOverlayDuration = 3.0f;    // :46
    float updateInterval = 0.0f;           // :48
    bool running = false;                  // :50
    bool EVAisUnlocked = false;            // :56
    bool visibleInPortrait = false;        // :70
    bool showHelmet = false;               // :74
    bool ivaMode = false;                  // :76
    bool randomizeOnStartup = true;        // :82
    Vector3f eyeInitialPos;                // :84
    Vector3f eyeLocalPosition;             // eyeTransform.localPosition (the shell keeps it current)
    bool capturedInitial = false;          // :86
    std::vector<bool> headRendererEnabled; // headRenderers[i].enabled
    bool helmetDestroyed = false;          // Object.Destroy(helmetTransform.gameObject)
    bool helmetTransformPresent = true;
    int textureTargets = 0;                // textureTargets.Length
    std::vector<bool> textureTargetUsesVeteran;  // the texture choice made for each target by SetupKerbalModel
    bool portraitRegistered = false;
    std::function<float()> astronautComplexLevel;  // ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.AstronautComplex)
    bool updateCoroutineRunning = false;
    std::shared_ptr<Coroutine> stateTimer;

    Vessel* InVessel() const;                                  // :98
    Kerbal();                                                  // Awake (:116)
    virtual ~Kerbal();                                         // OnDestroy (:135)
    virtual void Start();                                      // :153
    virtual void SetupKerbalModel();                           // :200
    virtual void ShowHelmet(bool show);                        // :222
    virtual void die();                                        // :269
    virtual void onVesselSitChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> evt);  // :279
    virtual void CheckEVAUnlocked();                           // :288
    virtual void IVAEnable(bool setIVAMode = true);            // :294
    virtual void IVADisable(bool setIVAMode = true);           // :308
    virtual void SetVisibleInPortrait(bool visible);           // :322
    // kerbalSeatCamUpdate (:240): the camera bob Euler angles from the three Perlin samples and the ship's accelerations
    static Vector3f SeatCamBobEuler(float perlinX, float perlinY, float perlinZ, double shipAccelerationMagnitude, double shipAngularVelocityMagnitude,
                                    float camBobAmount);
    // SetupKerbalModel texture rule per target (`protoCrewMember.veteran ? textureVeteran : textureStandard`)
    bool UsesVeteranTexture() const;
};

// CrewTransfer.CrewTransferData (CrewTransfer.cs:7)
class CrewTransfer_CrewTransferData {
public:
    bool canTransfer = true;
    Part* sourcePart = nullptr;
    Part* destPart = nullptr;
    std::shared_ptr<ProtoCrewMember> crewMember;
    class CrewTransfer* transfer = nullptr;
};

// CrewTransfer.cs:5
class CrewTransfer : public PartItemTransfer {
public:
    using CrewTransferData = CrewTransfer_CrewTransferData;
    std::shared_ptr<ProtoCrewMember> crew;   // :17
    Part* tgtPart = nullptr;                 // :19
    static inline std::function<void(Vessel&)> despawnCrew;  // Vessel.DespawnCrew
    static inline std::function<void(Vessel&)> spawnCrew;    // Vessel.SpawnCrew
    std::shared_ptr<Coroutine> completion;

    static std::unique_ptr<CrewTransfer> Create(Part& srcPart, std::shared_ptr<ProtoCrewMember> crewMember,
                                                std::function<void(DismissAction, Part*)> onDialogDismiss);  // :21
    ~CrewTransfer() override;
    void OnPartSelect(Part* p) override;                // :56
    virtual void MoveCrewTo(Part* p);                   // :86
    virtual void waitAndCompleteTransfer();             // :104
    void onGoForEva(events::GameEvents::HostedFromToAction<ProtoCrewMember*, Part*> fromto);  // :70
    void onChangeInactive(ProtoCrewMember* pcm, bool from, bool to);  // :78

protected:
    void HookAdditionalEvents() override;               // :29
    void UnhookAdditionalEvents() override;             // :35
    bool IsSemiValidPart(Part& p) override;             // :41
    bool IsValidPart(Part& p) override;                 // :49
    void AfterPartsFound() override;                    // :55
};

struct CrewHatchEnvironment {
    // Physics.Raycast(FlightCamera.mainCamera.ScreenPointToRay(Input.mousePosition), out rayHit, 300f, 2097152) and the hit collider's tag / transform / part
    struct Hit {
        bool hit = false;
        bool airlockTag = false;
        Part* part = nullptr;                 // FlightGlobals.GetPartUpwardsCached(collider.gameObject)
        const PartTransform* colliderTransform = nullptr;
    };
    static inline std::function<Hit()> raycast;
    static inline std::function<bool()> flightDriverPause;        // FlightDriver.Pause
    static inline std::function<bool()> pointerOverUI;            // EventSystem.current.IsPointerOverGameObject()
    static inline std::function<bool()> cursorLocked;             // Cursor.lockState == Locked
    static inline std::function<bool()> mouseLeftClick;           // Mouse.Left.GetClick()
    static inline std::function<void(bool)> showTooltip;          // crewHatchTooltip.gameObject.SetActive
    static inline std::function<void(Part&, bool showEVA, bool showTransfer)> spawnDialog;  // CrewHatchDialog.Spawn
    static inline std::function<void()> terminateDialog;          // crewDialog.Terminate()
    static inline std::function<Part*()> dialogPart;              // crewDialog.Part
    static inline std::vector<std::string> messages;              // ScreenMessages
    static void Reset();
};

// CrewHatchController.cs:7
class CrewHatchController {
public:
    static inline CrewHatchController* fetch = nullptr;   // :18
    bool overrideTransfer = false;                        // :28
    Vector2f anchorOffset{0.5f, -0.5f};                   // :26
    bool crewDialogOpen = false;                          // crewDialog != null
    std::unique_ptr<CrewTransfer> crewTransfer;           // :24
    CrewHatchEnvironment::Hit rayHit;                     // :14
    Part* hoveredPart = nullptr;                          // :16
    bool tooltipVisible = false;
    bool interfaceEnabled = false;                        // :12

    CrewHatchController();                                // Awake (:57)
    ~CrewHatchController();                               // OnDestroy (:71)
    bool Active() const { return crewDialogOpen || crewTransfer != nullptr; }  // :48
    void LateUpdate();                                    // :85
    void SpawnCrewDialog(Part& part, bool showEVA, bool showTransfer);  // :143
    void OnEVABtn(std::shared_ptr<ProtoCrewMember> crew); // :150
    void OnTransferBtn(std::shared_ptr<ProtoCrewMember> crew);  // :174
    void DismissDialog();                                 // :184
    void OnCrewDialogDismiss() {}                         // :192
    void DespawnUIs();                                    // :196
    void OnVesselSwitch(Vessel* v);                       // :202
    void onVesselSitChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> evt);  // :207
    void OnCrewTransferDone(PartItemTransfer::DismissAction dma, Part* p);  // :215
    void EnableInterface();                               // :222
    void DisableInterface();                              // :228
    void ShowHatchTooltip(bool show);                     // :234
    void ShowTooltip();                                   // :112
    void HideTooltip();                                   // :134
    void TooltipUpdate(Part& hp);                         // :121
    Part* dialogPart = nullptr;                           // crewDialog.Part
};

}  // namespace ksp
