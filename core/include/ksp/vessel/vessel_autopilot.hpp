// WP-24: VesselAutopilot.cs (VesselAutopilot and its nested VesselSAS: the stability assist PID controllers, the mode / lock / target logic).
//
// Ported: every float and double store of the controller (Mono float rule), VesselAutopilot.Enable / Disable / SetMode / CanSetMode /
// the three SetAutopilot* switches / Update (the SAS action group), VesselSAS.ControlUpdate (rotation delta, coasting, decay, auto-tune
// scalars, the three PIDclamp updates, damping, the clamp to FlightCtrlState), GetTotalVesselTorque over the ITorqueProvider modules,
// ConnectFlyByWire / DisconnectFlyByWire (the vessel OnAutopilotUpdate multicast delegate), the lock rotation bookkeeping.
//
// Seams (Unity / other packages), all in AutopilotEnvironment, each documented with what unset means:
//   * Quaternion.eulerAngles is native Unity code (not ported): `eulerAngles` hook, default = the Unity formulation in single precision
//     (see vessel_autopilot.cpp; not verified against the native build);
//   * the input devices of CheckPitch/Roll/YawInput (GameSettings axes, SpaceNavigator, key state): `pitchInput` / `rollInput` /
//     `yawInput` hooks, unset = no input;
//   * FlightGlobals.speedDisplayMode / GetDisplayVelocity, ScreenMessages, FlightInputHandler.precisionMode, rb.angularVelocity = zero.
//
// Not ported: the GameEvents.onVesselWasModified subscription of VesselSAS.Start (VesselModified is public; the host calls it).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/kerbals/experience.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Vessel;
class FlightCtrlState;
class PatchedConicSolver;
class PartModule;

// FlightGlobals.SpeedDisplayModes (FlightGlobals.cs:12)
enum class AutopilotSpeedMode : int { Orbit, Surface, Target };

struct AutopilotEnvironment {
    // Quaternion.eulerAngles (degrees, 0..360). Unset: AutopilotEnvironment::DefaultEulerAngles.
    static inline std::function<Vector3f(const Quaternionf&)> eulerAngles;
    static Vector3f DefaultEulerAngles(const Quaternionf& q);
    // CheckPitchInput / CheckRollInput / CheckYawInput without the ActiveVessel test: true when the axis, space navigator or a key is active
    // (`controlDetectionThreshold` is passed in). Unset: no input.
    static inline std::function<bool(float controlDetectionThreshold)> pitchInput, rollInput, yawInput;
    // FlightGlobals.speedDisplayMode. Unset: Orbit.
    static inline std::function<AutopilotSpeedMode()> speedDisplayMode;
    // FlightGlobals.GetDisplayVelocity().sqrMagnitude. Unset: the vessel obt_velocity / srf_velocity per the speed mode.
    static inline std::function<double(Vessel&)> displayVelocitySqrMagnitude;
    // ScreenMessages.PostScreenMessage texts, in order (the tests inspect them).
    static inline std::vector<std::string> screenMessages;
    // KillAngularVelocity: rb.angularVelocity = Vector3.zero on every part that is not a robotic rotor / a control surface child of one.
    // Unset: nothing happens. Called once per KillAngularVelocity.
    static inline std::function<void(Vessel&)> killAngularVelocity;
    // FlightInputHandler.fetch.precisionMode (ModuleRCS GetPotentialTorque). Unset: true (the game default).
    static inline std::function<bool()> precisionMode;
    // GetPotentialTorque of an ITorqueProvider whose signature needs scene inputs and that kspcore does not model (ModuleControlSurface):
    // returns true when it handled the module. Unset / false: PartModule::AsTorqueProvider, then the ModuleRCS and ModuleGimbal
    // overloads of WP-23, otherwise the module contributes nothing.
    static inline std::function<bool(Vessel&, PartModule&, Vector3f& pos, Vector3f& neg)> torqueProviderHook;
    static void Reset();
};

class VesselAutopilot {
public:
    using AutopilotMode = VesselAutopilot_AutopilotMode;

    // VesselAutopilot.cs:8
    class VesselSAS {
    public:
        PIDclamp pidLockedPitch{"Torque Pitch", 15000.0f, 0.0f, 2000.0f, 1500.0f};
        PIDclamp pidLockedRoll{"Torque Roll", 9000.0f, 0.0f, 600.0f, 2000.0f};
        PIDclamp pidLockedYaw{"Torque Yaw", 15000.0f, 0.0f, 2000.0f, 1500.0f};
        bool dampingMode = false;
        bool lockedMode = true;
        float overrideMinimumMagnitude = 0.1f;
        float controlDetectionThreshold = 0.05f;
        Quaternionf lockedRotation{};  // default(Quaternion): all zero until LockRotation
        bool autoTune = true;
        Vector3f autoScalarMin{0.5f, 0.5f, 0.5f};
        Vector3f autoScalarMax{10.0f, 10.0f, 10.0f};
        Vector3f scalarFactor{-0.3f, -0.3f, -0.3f};
        Vector3f scalarIntx{2.0f, 2.0f, 2.0f};
        float scalarRate = 0.9f;
        Vector3f posTorque{}, negTorque{};
        Vector3f targetOrientation{};
        float stopScalar = 1.0f;
        float coastScalar = 2.0f;
        bool useDecay = true;
        Vector3f decayResponseThreshold{0.02f, 0.02f, 0.02f};
        Vector3f decayDeltaThreshold{0.1f, 0.1f, 0.1f};
        Vector3f decayRate{0.005f, 0.005f, 0.005f};
        Vector3f decayMin{0.1f, 0.1f, 0.1f};
        int dampingCooldown = 20;

        explicit VesselSAS(Vessel& v);
        void SetLockYawPID(float Kp, float Ki, float Kd, float clamp);
        void SetLockRollPID(float Kp, float Ki, float Kd, float clamp);
        void SetLockPitchPID(float Kp, float Ki, float Kd, float clamp);
        void ResetAllPIDS();
        void ResetTuningScalars();
        void TuneScalars(float scalar);
        void SetTuningScalar(float scalar);
        bool CanEngageSAS();
        void LockRotation(const Quaternionf& newRotation);
        void Start();
        void Update();
        void AutoTuneScalar();
        void AutoTuneReset();
        void SetDampingMode(bool isEnabled);
        void ConnectFlyByWire(bool reset);
        void DisconnectFlyByWire();
        void SetTargetOrientation(const Vector3f& tgtOrientation, bool reset);
        // the OnAutopilotUpdate callbacks (FlightInputCallback(FlightCtrlState)); public so that Delegate::Bind can name them
        void ControlUpdate(FlightCtrlState& s);
        void UpdateVesselTorque(FlightCtrlState& s);
        void Destroy();
        void VesselModified(Vessel& v);
        void ModuleSetup();

        // private state the tests read (VesselAutopilot.cs:19-103)
        bool FBWconnected() const { return FBWconnected_; }
        const Vector3d& sasResponse() const { return sasResponse_; }
        const Vector3f& rotationDelta() const { return rotationDelta_; }
        const Vector3d& angularDeltaRad() const { return angularDeltaRad_; }
        const Vector3f& angularAccelerationMax() const { return angularAccelerationMax_; }
        const Vector3f& decayScalar() const { return decayScalar_; }
        const Vector3f& torqueVector() const { return torqueVector_; }
        const Quaternionf& currentRotation() const { return currentRotation_; }
        const Quaternionf& lastRotation() const { return lastRotation_; }
        float sasTuningScalar() const { return sasTuningScalar_; }
        int dampingCooldownTimer() const { return dampingCooldownTimer_; }
        bool decayLocked() const { return decayLocked_; }
        const PartTransform* storedTransform() const { return storedTransform_; }
        bool sasEquippedVessel() const { return sasEquippedVessel_; }

    private:
        Vessel* vessel_ = nullptr;
        float sasTuningScalar_ = 1.0f;
        bool sasEquippedVessel_ = false;
        bool FBWconnected_ = false;
        bool isStarted_ = false;
        Quaternionf currentRotation_ = Quaternionf::identity();
        Quaternionf lastRotation_ = Quaternionf::identity();
        Vector3f rotationDelta_{};
        Vector3d angularDelta_{};
        Vector3d angularDeltaRad_{};
        Vector3d sasResponse_{};
        const PartTransform* storedTransform_ = nullptr;  // Transform identity = address (the vessel transform or a part transform)
        Vector3f autoScalar_{};
        Vector3f torqueVector_{};
        Vector3f angularAccelerationMax_{};
        float neededPitch_ = 0.0f;
        float neededYaw_ = 0.0f;
        bool decayLocked_ = false;
        Vector3f decayScalar_{1.0f, 1.0f, 1.0f};
        int dampingCooldownTimer_ = 20;
        bool pitchInput_ = false, rollInput_ = false, yawInput_ = false;

        void SetVessel(Vessel& v);
        void SASUpdate();
        void CheckStoredTransform();
        bool CheckYawInput();
        bool CheckRollInput();
        bool CheckPitchInput();
        bool UpdatePitchInput();
        bool UpdateRollInput();
        bool UpdateYawInput();
        float EvaluateScalar(float ratio);
        Vector3f GetTotalVesselTorque(Vessel& v);
        void UpdateMaximumAcceleration();
        Vector3f PitchYawAngle(const PartTransform& t, const Vector3f& v2);
        void PitchYawAngle(const PartTransform& t, const Vector3f& v2, float& pitch, float& yaw);
        void CheckCoasting();
        void StabilityDecay();
        void KillAngularVelocity();
        void ResetDecay();
        void CheckDamping();
        Quaternionf GetRotationDelta();
        static float AngularTrim(float angle);
    };

    explicit VesselAutopilot(Vessel& vessel);
    ~VesselAutopilot();

    VesselSAS& SAS() { return *sas_; }
    Vessel& GetVessel() { return *vessel_; }
    AutopilotMode Mode() const { return mode_; }
    bool Enabled() const { return enabled_; }
    void SetEnabled(bool value);  // the Enabled setter
    void Destroy();
    void SetupModules();
    void Update();
    bool Enable();
    bool Enable(AutopilotMode mode);
    bool Disable();
    bool SetMode(AutopilotMode mode);
    bool CanSetMode(AutopilotMode mode);

private:
    std::unique_ptr<VesselSAS> sas_;
    Vessel* vessel_ = nullptr;
    AutopilotMode mode_ = AutopilotMode::StabilityAssist;
    bool enabled_ = false;

    bool SetAutopilot(bool initialize);
    bool SetAutopilotOrbit(bool initialize);
    bool SetAutopilotSurface(bool initialize);
    bool SetAutopilotTarget(bool initialize);
    bool VectorLockInvalid(Vessel& v, float threshold);
    bool TargetLockInvalid(Vessel& v);
    bool ManeuverLockInvalid(PatchedConicSolver* pcs);
};

}  // namespace ksp
