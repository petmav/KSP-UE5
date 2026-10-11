// WP-55: Making History node library, shared pieces. The Tests.*, Actions.* and Adjusters.* classes live in mission_tests.hpp,
// mission_actions.hpp and mission_adjusters.hpp; RegisterNodeLibrary() registers all of them in the WP-54 registries.
//
// Spec: Assembly-CSharp/Expansions/Missions/{Tests,Actions,Adjusters}/*, MissionCelestialBody.cs, MEScoreModule.cs, MissionsUtils.cs.
//
// C# -> C++ conventions of this package (on top of the WP-54 header comment in mission_node.hpp):
//   * `public override void Awake() { base.Awake(); ... }` is the constructor body; the base constructor runs first, like base.Awake().
//   * `GetType().Name` / `is T` of the node library classes: TypeName() / IsA(); every class that other code casts to (TestVessel,
//     TestVesselSituation, ActionVessel, ActionMissionScore) adds its name to IsA.
//   * `[MEScoreModule(...)]` (MEScoreModule.cs, read with inherit: false by MissionScore.GetSupportedTypes): a per-class table
//     (ScoreModuleAttributes) installed into MissionScore::allowedSystemsOf by RegisterNodeLibrary().
//   * `[MEGUI_*]` field attributes are editor reflection (BaseAPFieldList) and are not ported; GetNodeBodyParameterString takes the
//     BaseAPField stand-in (name, guiName, ToString of the value).
//   * `Debug.Log*` is dropped (the C# log lines are not state).
//   * Seams to the flight scene / UI: NodeLibraryEnvironment (below) and the WP-54 MissionsEnvironment / AwardEnvironment.
#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/missions/mission_display.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission_types.hpp"
#include "ksp/vessel/vessel_types.hpp"

namespace ksp {
class CelestialBody;
class ProtoVessel;
class Part;
class Vessel;
}  // namespace ksp

namespace ksp::Expansions::Missions {

// ScreenMessageStyle.cs:3 (the value ActionMessage stores)
enum class ScreenMessageStyle { UPPER_CENTER = 0, UPPER_LEFT = 1, UPPER_RIGHT = 2, LOWER_CENTER = 3, KERBAL_EVA = 4 };

// MissionCelestialBody.cs:6 (slice-2 class pulled in by TestSOILeft / TestSOIReached)
class MissionCelestialBody {
public:
    MissionCelestialBody();                                  // :64 body = FlightGlobals.GetHomeBody()
    explicit MissionCelestialBody(CelestialBody* body);      // :69
    CelestialBody* Body() const { return body_; }            // :35
    void SetBody(CelestialBody* b) { body_ = b; }            // :39
    bool AnyValid() const { return body_ == nullptr; }       // :45
    void SetAnyValid() { body_ = nullptr; }                  // :49 (setter: body = null)
    std::string DisplayName() const;                         // :13
    std::string Name() const;                                // :24
    bool IsValid(CelestialBody* targetbody) const;           // :74
    std::string GetNodeBodyParameterString() const;          // :94
    void Save(ConfigNode& node) const;                       // :99
    void Load(const ConfigNode& node);                       // :104

private:
    CelestialBody* body_ = nullptr;
};

namespace Library {
// MissionsUtils.GetFieldValueForDisplay (MissionsUtils.cs:565) for a bool field: Localizer.Format of #autoLOC_439839 (True) / #autoLOC_439840
std::string BoolFieldText(bool v);
// ... for an enum field: Enum.displayDescription() = Localizer.Format(e.Description()) (EnumExtensions.cs:54); the BaseAPField `value`
template <class E>
std::string EnumFieldText(E v) {
    return KSP::Localization::Localizer::Format(EnumDescription(v));
}
}  // namespace Library

// The vessel world / UI seams used by the node library, on top of MissionsEnvironment and AwardEnvironment (unset = the stated default).
struct NodeLibraryEnvironment {
    // FlightGlobals.GetHomeBody(). Unset: the first body with isHomeWorld in SaveEnvironment::Current().localBodies.
    static inline std::function<CelestialBody*()> homeBody;
    // FlightGlobals.GetBodyByName(name). Unset: AwardEnvironment::getBodyByName, else SaveEnvironment::Current().localBodies by bodyName.
    static inline std::function<CelestialBody*(const std::string&)> bodyByName;
    // KerbalRoster.GetLocalizedExperienceTraitName(traitName) (TestVesselCrewCount :96). Unset: the trait name itself.
    static inline std::function<std::string(const std::string&)> localizedTraitName;
    // ScreenMessages.PostScreenMessage(text, duration, style, persist). Unset: recorded in screenMessages.
    struct ScreenMessage {
        std::string text;
        float duration = 0.0f;
        ScreenMessageStyle style = ScreenMessageStyle::UPPER_CENTER;
        bool persist = false;
    };
    static inline std::function<void(const ScreenMessage&)> postScreenMessage;
    static inline std::vector<ScreenMessage> screenMessages;
    // MessageNodeDialog.Spawn(...) (ActionDialogMessage.Fire, UI): receives the arguments, `dialogClosed` is the callback the dialog calls.
    struct DialogMessage {
        std::string heading, message, instructorName, buttonLabel, instructorName2;
        int textAreaSize = 0;
        int screenArea = 0;  // ActionDialogMessage.DialogMessageArea
        bool autoClose = false;
        int autoCloseTimeOut = 0;
        bool autoGrowDialogHeight = false;
        std::function<void()> dialogClosed;
    };
    static inline std::function<void(DialogMessage&&)> spawnMessageDialog;
    static inline std::vector<DialogMessage> dialogMessages;  // unset: recorded
    // MessageSystem.Instance.AddMessage(new Message(title, message, color, icon), animate: false). Unset: recorded as (title, message).
    static inline std::function<void(const std::string& title, const std::string& message)> addMessageSystemMessage;
    static inline std::vector<std::pair<std::string, std::string>> systemMessages;
    // FlightGlobals.FindLoadedPart(partId, out part) (ActionPartExplode.Fire). Unset: nullptr (not found).
    static inline std::function<Part*(uint32_t partPersistentId)> findLoadedPart;
    // FlightGlobals.FindNearestControllableVessel(v) / ForceSetActiveVessel(v) + FlightInputHandler.ResumeVesselCtrlState(v)
    static inline std::function<Vessel*(Vessel*)> findNearestControllableVessel;
    static inline std::function<void(Vessel*)> forceSetActiveVessel;
    // UnityEngine.Random.Range(min, max) (int, max exclusive) (ActionPartExplode.Fire). Unset: min.
    static inline std::function<int(int, int)> randomRange;
    // MissionMessages / editor: MissionEditorLogic.LoadVessel(craft).parts persistentIds of the craft of vessel `vesselId` (VesselPartIDPair.
    // ValidatePartAgainstCraft). Unset: nullopt = no craft (the validation returns).
    static inline std::function<std::optional<std::vector<uint32_t>>(MENode*, uint32_t vesselId)> craftPartIds;
    // Part.explode() (ActionVesselExplode.Fire). Unset: recorded in explodedParts.
    static inline std::function<void(Part*)> explodePart;
    static inline std::vector<Part*> explodedParts;
    // Vessel.RequestResource(part, id, demand, usePriority) (Vessel.cs:3975; resource flow, WP-15; ActionPartResourceAmount.Fire).
    // Unset: nothing is drained or added (returns 0).
    static inline std::function<double(Vessel*, Part*, int resourceId, double demand, bool usePriority)> vesselRequestResource;
    // StageManager.ActivateNextStage() (ActionVesselStage.Fire). Unset: counted.
    static inline std::function<void()> activateNextStage;
    static inline int stagesActivated = 0;
    // Funding.Instance (ActionModifierFunds, TestMissionFunds): the live Funding scenario module (Funding::Instance). No seam needed.
    // ModuleAsteroid / ModuleComet part modules of a vessel (TestVesselActiveCount): found by class name through Vessel.
    static void Reset();

    static CelestialBody* HomeBody();
    static CelestialBody* BodyByName(const std::string& name);
};

// MEScoreModule attribute table (own attribute only: MissionScore.GetSupportedTypes reads it with inherit: false)
const std::map<std::string, std::vector<std::string>>& ScoreModuleAttributes();

// Registers every Tests.* module in TestModuleRegistry, Actions.* in ActionModuleRegistry, the Adjusters.* in DynamicModuleRegistry and
// installs MissionScore::allowedSystemsOf. Registration order is the order of the lists in node_library.cpp (the assembly scan order
// is not observable: lookup is by name).
void RegisterNodeLibrary();

}  // namespace ksp::Expansions::Missions
