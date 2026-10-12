// WP-19a: VesselSwitching (VesselSwitching.cs, 130 lines): the "next / previous vessel" keys of the flight scene. Which loaded vessel comes
// next is pure logic over the FlightGlobals registry; the keys, the map view state, the pause state, the camera mode and the screen messages are the
// host's (`Environment`), the switch itself is FlightGlobals::SetActiveVessel / ForceSetActiveVessel (events, packing, ClearToSave).
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace ksp {

class Vessel;

class VesselSwitching {
public:
    struct Environment {
        std::function<bool()> mapIsEnabled;       // MapView.MapIsEnabled
        std::function<bool()> paused;             // FlightDriver.Pause
        std::function<bool()> focusPrevKeyDown;   // GameSettings.FOCUS_PREV_VESSEL.GetKeyDown()
        std::function<bool()> focusNextKeyDown;   // GameSettings.FOCUS_NEXT_VESSEL.GetKeyDown()
        std::function<bool()> modifierKeyHeld;    // GameSettings.MODIFIER_KEY.GetKey()
        std::function<void()> setCameraFlight;    // CameraManager.Instance.SetCameraFlight()
        std::function<void(const std::string&)> postScreenMessage;  // ScreenMessages.PostScreenMessage(Localizer.Format(key), 5f, UPPER_CENTER); the text
        std::function<void(Vessel&)> resumeVesselCtrlState;         // FlightInputHandler.ResumeVesselCtrlState(vessel) (default: the real function)
        std::function<bool()> loadedSceneIsFlight;                  // HighLogic.LoadedSceneIsFlight (default: true)
    };
    static inline Environment env;

    VesselSwitching();   // Awake (VesselSwitching.cs:15): the singleton `fetch`
    ~VesselSwitching();  // OnDestroy (VesselSwitching.cs:27)
    VesselSwitching(const VesselSwitching&) = delete;
    VesselSwitching& operator=(const VesselSwitching&) = delete;
    bool Alive() const { return alive_; }

    // VesselSwitching.cs:35: the per-frame key handling. Returns the vessel that was made active (nullptr: nothing happened).
    Vessel* Update();

    // The candidate search and the switch of Update() for one key press (direction -1 = previous, +1 = next), without the key / map / pause gates.
    // `flag` of the C# (an uncontrollable vessel was skipped because the modifier key was not held) is reported through `skippedUncontrollable`.
    std::vector<Vessel*> FindCandidates(int direction, bool modifierKey, bool& skippedUncontrollable) const;

private:
    static inline VesselSwitching* fetch_ = nullptr;
    int nextVessel_ = 0;
    std::vector<Vessel*> nextVesselCandidates_;
    bool alive_ = true;
};

}  // namespace ksp
