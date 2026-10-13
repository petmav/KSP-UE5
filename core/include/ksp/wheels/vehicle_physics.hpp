// WP-29a: the reachable part of the VehiclePhysics (VPP, Edy's Vehicle Physics Pro) wheel core and KSP's KSPWheelController, ported 1:1 from
// decompiled/KSP-Decompiled/Assembly-CSharp/:
//   VehiclePhysics/TireFriction.cs   the five tyre curves (Flat, Lineal, Smooth, Parametric, Pacejka) and the contact-patch maths
//   VehiclePhysics/Block.cs, Wheel.cs, DirectDrive.cs, Solver.cs   the wheel-spin block chain and its explicit Euler / RK4 integrator
//   VehiclePhysics/VehicleBase.cs    the per-tick pipeline (suspension travel / forces, downforce, local frame, tyre force), Update
//   VehiclePhysics/VehicleBehaviour.cs, VPWheelCollider.cs, GroundMaterial*.cs, DataBus.cs
//   KSPWheelController.cs, KSPWheelGravity.cs
//   EdyCommonTools/MathUtility.cs (FastAbs, UnclampedLerp, Vector3Equals), InterpolatedFloat.cs
// The native half (UnityEngine.WheelCollider = PhysX, Rigidbody, Transform) is the data seam in wheel_native.hpp; the module layer
// (ModuleWheelBase, ModuleWheels/*) is WP-29b, the native suspension law is WP-29c.
//
// NOT PORTED (unreachable from KSP, or renderer-only), see STATUS.md WP-29a: Engine, Gearbox, Driveline, Differential*, Clutch, Retarder, Brakes,
// Steering, Inertia, VPVehicleController, VPVehicleJoint, VPAudio, VPDamage, VPTelemetry, DataLogger, DataRecorder, the charts, cameras, cargo,
// VPTireEffects, VPGroundMarksRenderer, VPGroundParticleEmitter, VPGroundMaterialManager, EdyCommonTools TextureCanvas / GColor / MiniJSON /
// UdpConnection / CsvFile / Spline / CircularBuffer ... (~20k lines); inside the ported files: VehicleBase.GetSolverState / GetSolverBlockTypes
// (diagnostics), Disable/EnableCollidersRaycast (Rigidbody interpolation only), the Debug.DrawLine gizmos, VPWheelCollider.AdjustToWheelMesh /
// GetScaledBounds / OnValidate / OnDrawGizmosSelected (editor), the Transform writes of UpdateVisualWheel (suspension / caliper / wheel mesh
// placement: the renderer's business), Solver.GetViscousLockingDt (clutches / differentials only).
#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/unity/mathf.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/wheels/wheel_native.hpp"

namespace ksp::VehiclePhysics {

// float * Vector2 (Vector2.cs operator *(float, Vector2)): each component is a float constructor argument
inline Vector2f operator*(float d, const Vector2f& a) { return {f32store(fx(a.x) * fx(d)), f32store(fx(a.y) * fx(d))}; }

// ---- EdyCommonTools ------------------------------------------------------------------------------------------------------------------
struct MathUtility {
    static float FastAbs(float x);                                          // MathUtility.cs:31
    static bool Vector3Equals(const Vector3f& a, const Vector3f& b);        // MathUtility.cs:58
    static float UnclampedLerp(float from, float to, float t);              // MathUtility.cs:107
};

class InterpolatedFloat {  // EdyCommonTools/InterpolatedFloat.cs (the frame ratio is a parameter: Time.fixedTime / time are the host's)
public:
    explicit InterpolatedFloat(float initialValue = 0.0f);
    void Reset(float value);
    void Set(float value);
    float Get() const { return m_value; }
    float GetInterpolated(float t) const;
private:
    float m_prevValue = 0.0f, m_value = 0.0f;
};

// ---- TireFriction.cs -----------------------------------------------------------------------------------------------------------------
class TireFriction {
public:
    enum class Model { Flat, Lineal, Smooth, Parametric, Pacejka };

    struct Settings {
        Vector2f adherent{0.5f, 0.95f};
        Vector2f peak{1.5f, 1.1f};
        Vector2f limit{4.0f, 0.8f};
        float a = 0.0f, b = 0.0f, c = 0.5f, d = 0.0f;
        float A = 0.95f, B = 0.8f, C = 1.5f, D = 1.1f, E = -2.0f;
        void ApplyAPLConstraints();
    };

    struct ContactPatch {
        Vector2f slip = Vector2f::zero();
        float load = 1.0f;
        float groundGrip = 1.0f;
    };

    class CurveBase {
    public:
        explicit CurveBase(std::shared_ptr<Settings> settings) : m_params(std::move(settings)) {}
        virtual ~CurveBase() = default;
        virtual float EvaluateForce(float slip, const ContactPatch& cp);
        virtual float GetAdherentSlip(const ContactPatch& cp);
        virtual float GetPeakSlip(const ContactPatch& cp);
        virtual float GetLimitSlip(const ContactPatch& cp);
    protected:
        std::shared_ptr<Settings> m_params;
    };

    class FlatFriction : public CurveBase {
    public:
        explicit FlatFriction(std::shared_ptr<Settings> settings);
        float EvaluateForce(float s, const ContactPatch& cp) override;
        float GetAdherentSlip(const ContactPatch& cp) override;
        float GetPeakSlip(const ContactPatch& cp) override;
        float GetLimitSlip(const ContactPatch& cp) override;
    };

    class LinealFriction : public CurveBase {
    public:
        explicit LinealFriction(std::shared_ptr<Settings> settings);
        float EvaluateForce(float s, const ContactPatch& cp) override;
        float GetAdherentSlip(const ContactPatch& cp) override;
        float GetPeakSlip(const ContactPatch& cp) override;
        float GetLimitSlip(const ContactPatch& cp) override;
    private:
        static float FastLerp(const Vector2f& P0, const Vector2f& P1, float x);
    };

    class SmoothFriction : public CurveBase {
    public:
        explicit SmoothFriction(std::shared_ptr<Settings> settings);
        float EvaluateForce(float s, const ContactPatch& cp) override;
        float GetAdherentSlip(const ContactPatch& cp) override;
        float GetPeakSlip(const ContactPatch& cp) override;
        float GetLimitSlip(const ContactPatch& cp) override;
    private:
        static float CubicLerp(float x, const Vector2f& P0, const Vector2f& P1);
    };

    class ParametricFriction : public CurveBase {
    public:
        explicit ParametricFriction(std::shared_ptr<Settings> settings);
        float EvaluateForce(float s, const ContactPatch& cp) override;
        float GetAdherentSlip(const ContactPatch& cp) override;
        float GetPeakSlip(const ContactPatch& cp) override;
        float GetLimitSlip(const ContactPatch& cp) override;
    private:
        static float TangentLerp(float x, const Vector2f& P0, const Vector2f& P1, float a, float b);
    };

    class PacejkaFriction : public CurveBase {
    public:
        explicit PacejkaFriction(std::shared_ptr<Settings> settings);
        static void GetPacejkaPoints(float A, float B, float C, float D, float E, Vector2f& adherent, Vector2f& peak, Vector2f& limit);
        static float Pacejka(float slip, float B, float C, float D, float E);
        float EvaluateForce(float s, const ContactPatch& cp) override;
        float GetAdherentSlip(const ContactPatch& cp) override;
        float GetPeakSlip(const ContactPatch& cp) override;
        float GetLimitSlip(const ContactPatch& cp) override;
    private:
        Vector2f m_adherent = Vector2f::zero(), m_peak = Vector2f::zero(), m_limit = Vector2f::zero();
    };

    Model model = Model::Smooth;
    std::shared_ptr<Settings> settings = std::make_shared<Settings>();
    float frictionMultiplier = 1.0f;

    TireFriction();
    void OnBeforeSerialize();
    void OnAfterDeserialize();
    // Builds m_curve from the CURRENT model field. ModuleWheelBase assigns `model = Lineal` after construction and never calls this
    // (ModuleWheelBase.cs:361), so the curve of a stock wheel stays the constructor's SmoothFriction: UNVERIFIED against the game until
    // capture C11 (WHEEL-SPIKE.md Q1).
    void SetupFrictionCurves();
    Vector2f GetForce(const ContactPatch& cp);
    bool IsAdherentSlip(const ContactPatch& cp);
    float GetAdherentSlipForward(const ContactPatch& cp);
    float GetAdherentSlipSideways(const ContactPatch& cp);
    Vector2f GetAdherentSlipBounds(const ContactPatch& cp);
    Vector2f GetPeakSlipBounds(const ContactPatch& cp);
    Vector2f GetLimitSlipBounds(const ContactPatch& cp);
    Vector2f GetAdherentForce(const ContactPatch& cp);
    CurveBase* curve() const { return m_curve.get(); }  // the live m_curve (tests: which class is it?)
private:
    std::shared_ptr<CurveBase> m_curve;
};

// ---- Block.cs / Wheel.cs / DirectDrive.cs / Solver.cs --------------------------------------------------------------------------------
class Block {
public:
    struct Connection {
        float L = 0.0f, I = 0.0f, Tr = 0.0f, outTd = 0.0f;
        Block* input = nullptr;
        int inputSlot = 0;
        Block* output = nullptr;
        int outputSlot = 0;
    };
    struct State {
        float L = 0.0f;
        Vector2f P = Vector2f::zero();
        float Lr = 0.0f;
    };
    struct Derivative {
        float T = 0.0f;
        Vector2f F = Vector2f::zero();
        float Tr = 0.0f;
    };

    static inline const float RpmToW = f32store(fx(3.14159274f) / 30.0);  // (float)Math.PI / 30f
    static inline const float WToRpm = f32store(30.0 / fx(3.14159274f));  // 30f / (float)Math.PI

    Block();
    virtual ~Block() = default;
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    std::vector<std::shared_ptr<Connection>>& inputs() { return m_inputs; }
    std::vector<std::shared_ptr<Connection>>& outputs() { return m_outputs; }
    bool hasInputs() const { return !m_inputs.empty(); }
    bool hasOutputs() const { return !m_outputs.empty(); }
    int connectedInputs() const { return m_connectedInputs; }
    int connectedOutputs() const { return m_connectedOutputs; }

    virtual bool CheckConnections();
    virtual void PreStep();
    virtual void GetState(State& S);
    virtual void SetSubstepState(const State& S);
    virtual void ComputeStateUpstream();
    virtual void EvaluateTorqueDownstream();
    virtual void GetSubstepDerivative(Derivative& D);
    virtual void SetState(const State& S);

    static bool Connect(Block& inputUnit, int inputSlot, Block& outputUnit, int outputSlot);
    static bool Connect(Block& inputUnit, Block& outputUnit);
    bool DisconnectInput(int inputSlot);
    bool DisconnectOutput(int outputSlot);
    void CountConnections();

protected:
    virtual void Initialize();
    void SetInputs(int count);
    void SetOutputs(int count);

private:
    std::vector<std::shared_ptr<Connection>> m_inputs, m_outputs;
    int m_connectedInputs = 0, m_connectedOutputs = 0;
};

class Wheel;

class Solver {
public:
    static constexpr float minInertia = 0.01f;
    static constexpr float minWheelRadius = 0.01f;
    static constexpr float minWheelMass = 0.01f;
    static inline float viscousCouplingRate = 100.0f;
    static inline float viscousLockedRatio = 0.95f;

    static float time() { return s_time; }
    static float deltaTime() { return s_deltaTime; }
    const std::string& resultMessage() const { return m_resultMessage; }

    bool Initialize(const std::vector<Wheel*>& wheels, bool enableRK4);
    void Integrate(float t, float dt, int steps, bool useRK4);
    const std::vector<Block*>& GetBlockArray() const { return m_blocks; }

private:
    using StateVector = std::vector<Block::State>;
    using DerivativeVector = std::vector<Block::Derivative>;
    static void EulerStep(const Block::State& In, const Block::Derivative& Der, Block::State& Out);
    static void EulerStepL(const Block::State& In, const Block::Derivative& Der, Block::State& Out);
    void EulerStep(const StateVector& In, const DerivativeVector& Der, StateVector& Out);
    void IntegrateEuler(StateVector& S, DerivativeVector& D, float t, float dt, int subSteps);
    void EvaluateRK4(StateVector& S, StateVector& Stemp, DerivativeVector& DIn, float t, float dt, DerivativeVector& DOut);
    static void ComputeRK4Derivative(const Block::Derivative& A, const Block::Derivative& B, const Block::Derivative& C, const Block::Derivative& D, Block::Derivative& DOut);
    static void ComputeRK4DerivativeT(const Block::Derivative& A, const Block::Derivative& B, const Block::Derivative& C, const Block::Derivative& D, Block::Derivative& DOut);
    void ComputeRK4Derivative(const DerivativeVector& A, const DerivativeVector& B, const DerivativeVector& C, const DerivativeVector& D, DerivativeVector& DOut);
    void IntegrateRK4(StateVector& S, StateVector& Stemp, DerivativeVector& D, DerivativeVector& Da, DerivativeVector& Db, DerivativeVector& Dc, DerivativeVector& Dd, float t, float dt);
    void ComputeDerivative(StateVector& S, DerivativeVector& Out);

    std::vector<Block*> m_blocks;
    int m_wheelCount = 0;
    std::vector<StateVector> m_states;
    std::vector<DerivativeVector> m_derivatives;
    bool m_RK4enabled = false;
    std::string m_resultMessage;
    static inline float s_time = 0.0f;
    static inline float s_deltaTime = 0.0f;
};

class Wheel : public Block {
public:
    std::shared_ptr<TireFriction> tireFriction;
    float radius = 0.0f;
    float mass = 0.0f;
    float load = 0.0f;
    float grip = 0.0f;
    Vector2f V = Vector2f::zero();
    Vector2f Fext = Vector2f::zero();
    float L = 0.0f;
    Vector2f P = Vector2f::zero();
    float T = 0.0f;
    Vector2f F = Vector2f::zero();
    float Tr = 0.0f;
    bool isResting = false;
    float m_forwardSlip = 0.0f;
    Vector2f m_tireForce = Vector2f::zero();
    float m_Ty = 0.0f;
    bool m_isAdherent = false;
    Vector2f m_adherentSlip = Vector2f::zero();
    Vector2f m_adherentForce = Vector2f::zero();
    float debug1 = 0.0f, debug2 = 0.0f, debug3 = 0.0f;

    Wheel();
    float inertia() const { return I; }
    float angularVelocity() const { return w; }
    float driveTorque() const { return m_inTd; }
    float brakeTorque() const { return m_inTb; }
    TireFriction::ContactPatch& contactPatch() { return m_contact; }

    void RecalculateConstants();
    float AddBrakeTorque(float brakeTorque);
    void ResetBrakeTorque();
    void RecalculateVars();
    void GetState(State& S) override;
    void SetSubstepState(const State& S) override;
    void ComputeStateUpstream() override;
    void EvaluateTorqueDownstream() override;
    void GetSubstepDerivative(Derivative& D) override;
    void SetState(const State& S) override;

protected:
    void Initialize() override;

private:
    static float SolveSymmetricConstraint(float Tsum, float T, float Tmax);
    float I = 0.0f, invI = 0.0f;
    float m_brakeTorque = 0.0f;
    float w = 0.0f;
    float m_inTd = 0.0f, m_inTb = 0.0f;
    TireFriction::ContactPatch m_contact;
};

class DirectDrive : public Block {
public:
    float motorInput = 0.0f;
    float maxMotorTorque = 100.0f;
    float maxRpm = 500.0f;
    float damping = 0.95f;

    DirectDrive();
    float angularVelocity() const;
    float torque() const;
    bool CheckConnections() override;
    void EvaluateTorqueDownstream() override;

protected:
    void Initialize() override;

private:
    Connection* m_output = nullptr;
    std::shared_ptr<Connection> m_outputHold;
};

// ---- Ground material ----------------------------------------------------------------------------------------------------------------
struct GroundMaterial {  // GroundMaterial.cs (the renderer fields marksRenderer / particleEmitter / customData are not modelled)
    enum class SurfaceType { Hard, Soft };
    std::shared_ptr<PhysicMaterial> physicMaterial;
    float grip = 1.0f;
    float drag = 0.0f;
    SurfaceType surfaceType = SurfaceType::Hard;
};

struct GroundMaterialHit {  // GroundMaterialHit.cs
    std::shared_ptr<PhysicMaterial> physicMaterial;
    Collider* collider = nullptr;
    Vector3f hitPoint = Vector3f::zero();
};

class VehicleBase;

class GroundMaterialManagerBase {  // GroundMaterialManagerBase.cs
public:
    virtual ~GroundMaterialManagerBase() = default;
    virtual GroundMaterial* GetGroundMaterial(VehicleBase& vehicle, const GroundMaterialHit& groundHit) = 0;
    virtual void GetGroundMaterialCached(VehicleBase& vehicle, const GroundMaterialHit& groundHit, GroundMaterialHit& cachedGroundHit, GroundMaterial*& groundMaterial);
};

class DataBus {  // DataBus.cs
public:
    DataBus();
    int Get(int idChannel, int idValue) const { return m_data[static_cast<size_t>(idChannel)][static_cast<size_t>(idValue)]; }
    void Set(int idChannel, int idValue, int value) { m_data[static_cast<size_t>(idChannel)][static_cast<size_t>(idValue)] = value; }
    std::vector<int>& Get(int idChannel) { return m_data[static_cast<size_t>(idChannel)]; }
    std::vector<std::vector<int>>& bus() { return m_data; }
private:
    std::vector<std::vector<int>> m_data;
};

// ---- KSPWheelGravity.cs ---------------------------------------------------------------------------------------------------------------
class KSPWheelGravity {
public:
    const Vector3f& Value() const { return gravityValue; }
    void Value(const Vector3f& value);
    float Magnitude();
    Vector3f Up();
    void Refresh();
private:
    Vector3f gravityValue = Vector3f::down() * 9.80665f;
    Vector3f gravityUp = Vector3f::zero();
    bool gravityCached = false;
    float gravityMagnitude = 0.0f;
};

// ---- VehicleBehaviour.cs / VPWheelCollider.cs -------------------------------------------------------------------------------------------
class VehicleBehaviour {
public:
    virtual ~VehicleBehaviour() = default;
    VehicleBase* vehicle() const { return m_vehicle; }
    virtual void OnEnableVehicle();
    virtual void OnDisableVehicle();
    virtual void UpdateVehicle();
    virtual void FixedUpdateVehicle();
    virtual int GetUpdateOrder();
    virtual void UpdateVehicleSuspension();
    virtual void OnEnableComponent();
    virtual void OnDisableComponent();
    virtual void OnReposition();
    virtual void OnEnterPause();
    virtual void OnLeavePause();
    // MonoBehaviour: `enabled` and the GameObject's active state
    bool enabled() const { return m_enabled; }
    bool activeInHierarchy = true;
    bool isActiveAndEnabled() const { return m_enabled && activeInHierarchy; }
    VehicleBase* parentVehicle = nullptr;  // GetComponentInParent<VehicleBase>() (wired by the host / KSPWheelController::Create)
    void SetEnabled(bool value);           // Unity: OnEnable / OnDisable on the transition
    void AddComponentActivate();           // AddComponent<T>(): Awake done, OnEnable now if still enabled
    std::vector<std::string> log;          // Debug.LogWarning / LogError messages
    void DebugLogWarning(const std::string& message);
    void DebugLogError(const std::string& message);
protected:
    virtual VehicleBase* GetVehicle();
    void OnEnable();
    void OnDisable();
private:
    VehicleBase* m_vehicle = nullptr;
    bool m_enabled = true;
    bool m_enableRan = false;
};

class VPWheelCollider : public VehicleBehaviour {
public:
    float mass = 20.0f;
    float radius = 0.3f;
    Vector3f center = Vector3f::zero();
    float suspensionDistance = 0.25f;
    float suspensionAnchor = 0.5f;
    float springRate = 30000.0f;
    float damperRate = 1500.0f;
    float groundPenetration = 0.02f;
    bool disableSuspensionMovement = false;
    bool hideWheelOnDisable = true;
    static inline bool disableSteerAngleFix = false;
    static inline bool disableWheelReferenceFrameFix = false;
    static inline float minSuspensionDistance = 0.01f;
    static inline float scaleFactor = 1.0f;
    int layerMask = -1;
    bool updateSuspension = false, updateCaliper = false, updateWheel = false;
    float suspensionOffset = 0.0f;
    float springSlerpRate = 0.02f;  // internal in C#, set by ModuleWheelBase
    bool hidden = false;

    // the host-owned objects of this component
    NativeTransform* cachedTransform = nullptr;       // the wheel collider's own transform (m_transform)
    NativeTransform* rigidbodyTransform = nullptr;    // the rigidbody's transform (m_rigidbodyTransform)
    NativeWheelCollider wheelColliderData;            // the WheelCollider component the Awake of this component added (SetupWheelCollider)
    // Physics.Raycast result of UpdateVisualWheel (the host's seam): returns the hit distance or a negative value for "no hit"
    std::function<float(const Vector3f& origin, const Vector3f& direction, float maxDistance)> visualRaycast;

    VPWheelCollider();
    bool visualGrounded() const { return m_visualGrounded; }
    float steerAngle() const { return m_steerAngle; }
    void steerAngle(float value);
    float angularVelocity = 0.0f;  // the auto-property
    float angularPosition() const { return m_angularPosition; }
    void angularPosition(float v) { m_angularPosition = v; }
    float contactDistance() const { return m_contactDistance; }
    Vector3f suspensionPosition() const { return m_suspensionPosition; }
    bool canSleep = false;
    float effectiveSpringRate() const { return m_wheelCollider->suspensionSpring.spring; }
    float runtimeSpringRate = 0.0f, runtimeDamperRate = 0.0f, runtimeSuspensionTravel = 0.0f;
    float lastRuntimeSpringRate() const { return m_lastRuntimeSpringRate; }
    float lastRuntimeDamperRate() const { return m_lastRuntimeDamperRate; }
    float lastRuntimeSuspensionTravel() const { return m_lastRuntimeSuspensionTravel; }
    float runtimeExtraDownforce = 0.0f;
    NativeWheelCollider* GetWheelCollider() { return m_wheelCollider; }
    const InterpolatedFloat& visualSteerAngle() const { return m_visualSteerAngle; }

    void Awake();
    void OnEnableVehicle() override;
    void OnDisableVehicle() override;
    void OnDisableComponent() override;
    int GetUpdateOrder() override;
    void FixedUpdateVehicle() override;
    void UpdateVisualWheel(float deltaTime);
    void SetSuspensionForceOffset(float offset);
    bool GetGroundHit(WheelHit& hit);
    float GetContactDepth(const Vector3f& contactPoint, float suspensionTravel);
    Vector3f GetTangentVelocity(const Vector3f& contactPoint, const Vector3f& surfaceNormal, NativeRigidbody* surfaceRigidbody);
    void ApplyForce(const Vector3f& force, const Vector3f& position, NativeRigidbody* otherRb);
    NativeWheelCollider* ResetWheelCollider();

private:
    NativeWheelCollider* SetupWheelCollider();
    void UpdateWheelCollider(NativeWheelCollider& wheelCol);
    float FixSteerAngle(float inputSteerAngle);

    NativeWheelCollider* m_wheelCollider = nullptr;
    NativeRigidbody* m_rigidbody = nullptr;
    float m_steerAngle = 0.0f;
    float m_angularPosition = 0.0f;
    float m_contactDistance = 0.0f;
    Vector3f m_suspensionPosition = Vector3f::zero();
    InterpolatedFloat m_visualSteerAngle;
    bool m_visualGrounded = false;
    float m_2PI = f32store(fx(3.14159274f) * 2.0);  // (float)Math.PI * 2f
    float m_lastRuntimeSpringRate = 0.0f, m_lastRuntimeDamperRate = 0.0f, m_lastRuntimeSuspensionTravel = 0.0f;
};

// ---- VehicleBase.cs -------------------------------------------------------------------------------------------------------------------
struct ContactPoint {  // UnityEngine.ContactPoint (what OnCollision reads)
    Vector3f point = Vector3f::zero();
    Vector3f normal = Vector3f::zero();
    NativeRigidbody* thisRigidbody = nullptr;  // thisCollider.attachedRigidbody
    Collider* otherCollider = nullptr;
};
struct Collision {  // UnityEngine.Collision
    std::vector<ContactPoint> contacts;
    Vector3f relativeVelocity = Vector3f::zero();
};

class VehicleBase {
public:
    enum class VehicleSleepCriteria { Strict, Relaxed };

    class WheelState {
    public:
        VPWheelCollider* wheelCol = nullptr;
        bool steerable = false;
        float steerAngle = 0.0f;
        bool grounded = false;
        WheelHit hit;
        GroundMaterial* groundMaterial = nullptr;
        GroundMaterialHit lastGroundHit;
        float contactDepth = 0.0f;
        float suspensionCompression = 0.0f;
        float downforce = 0.0f;
        float normalizedLoad = 0.0f;
        float contactAngle = 0.0f;
        float contactSpeed = 0.0f;
        float damperForce = 0.0f;
        float lastContactDepth = 0.0f;
        Vector3f wheelVelocity = Vector3f::zero();
        Vector3f surfaceForce = Vector3f::zero();
        Vector2f localWheelVelocity = Vector2f::zero();
        Vector2f localSurfaceForce = Vector2f::zero();
        Vector2f externalTireForce = Vector2f::zero();
        float angularVelocity = 0.0f;
        Vector2f tireForce = Vector2f::zero();
        float reactionTorque = 0.0f;
        Vector2f tireSlip = Vector2f::zero();
        float combinedTireSlip = 0.0f;
        Vector2f lastTireForce = Vector2f::zero();
        float driveTorque = 0.0f;
        float brakeTorque = 0.0f;
    };
    struct VehicleStateVars {
        float time = 0.0f;
        Vector3f lastVelocity = Vector3f::zero();
        float lastImpactTime = 0.0f;
    };
    struct WheelStateVars {
        float L = 0.0f, Tr = 0.0f, contactDepth = 0.0f;
        Vector2f lastTireForce = Vector2f::zero();
    };
    enum class WheelPos { Default = 0, Left = 0, Right = 99 };

    bool tireSideDeflection = false;
    float tireSideDeflectionRate = 10.0f;
    float tireImpulseRatio = 0.5f;
    float wheelSleepVelocity = 0.2f;
    bool advancedSuspensionDamper = false;
    float suspensionDamperLimitFactor = 2.0f;
    bool contactAngleAffectsTireForce = true;
    int integrationSteps = 4;
    bool integrationUseRK4 = false;
    const NativeTransform* centerOfMass = nullptr;
    bool accurateSuspensionForces = true;
    float scaleFactor = 1.0f;
    VehicleSleepCriteria vehicleSleepCriteria = VehicleSleepCriteria::Strict;
    bool inhibitWheelSleep = false;
    bool invertVisualWheelSpinDirection = false;
    std::function<void()> onPreDynamicsStep, onBeforeUpdateBlocks, onBeforeIntegrationStep, onPreVisualUpdate;
    float minimumdownForce = 0.0f;
    KSPWheelGravity gravity;
    bool tireForceSmoothing = true;
    bool suspensionForceSmoothing = true;
    float tireForceSharpness = 5.0f;
    float suspensionForceSharpness = 5.0f;
    DataBus data;
    bool showContactGizmos = true;
    bool disableContactProcessing = false;
    std::function<void()> onImpact, onRawCollision;
    static inline VehicleBase* vehicle = nullptr;  // VehicleBase.vehicle (set only during the onImpact / onRawCollision callbacks)
    static inline const Collision* currentCollision = nullptr;
    static inline bool isCollisionEnter = false;
    float impactThreeshold = 0.6f;
    float impactInterval = 0.2f;
    float impactIntervalRandom = 0.4f;
    float impactMinSpeed = 2.0f;

    // the Unity components this component finds with GetComponent*: wired by the host
    NativeTransform* transform = nullptr;      // m_transform
    NativeRigidbody* rigidbody = nullptr;      // m_rigidbody (GetComponentInParent<Rigidbody>())
    GroundMaterialManagerBase* groundMaterialManager = nullptr;
    std::vector<VehicleBehaviour*> childBehaviours;  // GetComponentsInChildren<VehicleBehaviour>(includeInactive: true)
    std::vector<std::string> log;                    // Debug.LogError / LogWarning

    VehicleBase() = default;
    virtual ~VehicleBase() = default;
    VehicleBase(const VehicleBase&) = delete;
    VehicleBase& operator=(const VehicleBase&) = delete;

    // MonoBehaviour: the component's `enabled` flag; the transition runs OnEnable / OnDisable
    bool enabled() const { return m_enabled; }
    void enabled(bool value);
    void AddComponentActivate();  // AddComponent<T>(): Awake done, OnEnable now if still enabled

    bool initialized() const { return m_initialized; }
    int wheelCount() const { return static_cast<int>(m_wheels.size()); }
    std::vector<std::shared_ptr<WheelState>>& wheelState() { return m_wheelState; }
    const std::vector<std::shared_ptr<WheelState>>& wheelState() const { return m_wheelState; }
    float speed() const { return m_speed; }
    float speedAngle() const { return m_speedAngle; }
    const Vector3f& localAcceleration() const { return m_localAcceleration; }
    float time() const { return m_time; }
    bool paused() const { return m_paused; }
    void paused(bool value);
    const Vector3f& localImpactPosition() const { return m_sumImpactPosition; }
    const Vector3f& localImpactVelocity() const { return m_sumImpactVelocity; }
    bool isHardImpact() const { return m_sumImpactHardness >= 0; }
    const Vector3f& localDragPosition() const { return m_localDragPosition; }
    const Vector3f& localDragVelocity() const { return m_localDragVelocity; }
    bool isHardDrag() const { return m_localDragHardness >= 0; }
    Collider* lastContactedCollider() const { return m_lastContactedCollider; }
    Wheel* wheelAt(int i) const { return m_wheels[static_cast<size_t>(i)].get(); }

    Vector3f GetWheelLocalPosition(VPWheelCollider& wheelCol);
    void SingleStep();
    void Reposition(const Vector3f& position, const Quaternionf& rotation);
    void SetWheelRadius(int wheelIndex, float radius);
    float GetWheelRadius(int wheelIndex);
    void SetWheelTireFriction(int wheelIndex, std::shared_ptr<TireFriction> friction);
    void SetWheelTireFrictionMultiplier(int wheelIndex, float frictionMultiplier);
    std::shared_ptr<TireFriction> GetWheelTireFriction(int wheelIndex);
    float GetWheelAngularVelocityForSlip(int wheelIndex, float slip);
    Vector2f GetWheelPeakSlip(int wheelIndex);
    Vector2f GetWheelAdherentSlip(int wheelIndex);
    void AddWheelBrakeTorque(int wheelIndex, float torque);
    VehicleStateVars GetVehicleStateVars();
    void SetVehicleStateVars(const VehicleStateVars& stateVars);
    std::vector<WheelStateVars> GetWheelStateVars();
    void SetWheelStateVars(const std::vector<WheelStateVars>& stateVars);
    virtual int GetWheelIndex(int axle, WheelPos position = WheelPos::Default);
    virtual int GetAxleCount();
    virtual int GetWheelsInAxle(int axle);

    // MonoBehaviour messages (the host calls them in Unity's order)
    void FixedUpdate();
    void Update();
    void OnCollisionEnter(const Collision& collision);
    void OnCollisionStay(const Collision& collision);
    void OnCollision(const Collision& collision, bool isCollisionEnter);
    void RegisterVehicleBehaviour(VehicleBehaviour* vb);
    void UnregisterVehicleBehaviour(VehicleBehaviour* vb);
    void DebugLogWarning(const std::string& message);
    void DebugLogError(const std::string& message);

protected:
    std::vector<std::shared_ptr<Wheel>>& wheels() { return m_wheels; }
    void SetNumberOfWheels(int numberOfWheels);
    virtual void OnInitialize();
    virtual void OnFinalize();
    virtual void DoUpdateBlocks();
    virtual void DoUpdateData();
    virtual void OnUpdate();

private:
    void ComputeVehicleSpeed();
    void ComputeLocalAcceleration();
    void ConfigureCenterOfMass();
    static float ComputeCombinedSlip(const Vector2f& localVelocity, const Vector2f& tireSlip);
    void CalculateSuspensionTravel(WheelState& wheel);
    void CalculateSuspensionForces(WheelState& wheel);
    void CalculateDownforce(WheelState& wheel, float referenceDownforce);
    void CalculateLocalFrame(WheelState& wheel);
    void ApplyTireForce(WheelState& wheel);
    void OnEnable();
    void OnDisable();
    void EnableVehicleBehaviours();
    void DisableVehicleBehaviours();
    void FixedUpdateVehicleBehaviours();
    void UpdateVehicleBehaviours();
    void UpdateSuspensionBehaviours();
    void AddVehicleBehaviour(VehicleBehaviour* vb);
    void RemoveVehicleBehaviour(VehicleBehaviour* vb);
    void NotifyReposition();
    void NotifyEnterPause();
    void NotifyLeavePause();
    void ClearCollisionData();
    void ProcessImpacts();
    void UpdateDragState(const Vector3f& dragPosition, const Vector3f& dragVelocity, int dragHardness);

    bool m_enabled = true;
    bool m_enableRan = false;
    bool m_initialized = false;
    float m_time = 0.0f;
    std::unique_ptr<Solver> m_solver;
    std::vector<std::shared_ptr<Wheel>> m_wheels;
    std::vector<std::shared_ptr<WheelState>> m_wheelState;
    Vector3f m_localAcceleration = Vector3f::zero();
    Vector3f m_lastVelocity = Vector3f::zero();
    float m_speed = 0.0f;
    float m_speedAngle = 0.0f;
    bool m_paused = false;
    bool m_singleFixedStep = false;
    bool m_singleUpdateStep = false;
    std::vector<VehicleBehaviour*> m_activeBehaviours;
    int m_sumImpactCount = 0;
    Vector3f m_sumImpactPosition = Vector3f::zero();
    Vector3f m_sumImpactVelocity = Vector3f::zero();
    int m_sumImpactHardness = 0;
    float m_lastImpactTime = 0.0f;
    Vector3f m_localDragPosition = Vector3f::zero();
    Vector3f m_localDragVelocity = Vector3f::zero();
    int m_localDragHardness = 0;
    GroundMaterial* m_impactedGroundMaterial = nullptr;
    GroundMaterialHit m_lastImpactedMaterial;
    Collider* m_lastContactedCollider = nullptr;
};

// ---- KSPWheelController.cs --------------------------------------------------------------------------------------------------------------
class KSPWheelController : public VehicleBase {
public:
    VPWheelCollider* wheelCollider = nullptr;
    std::shared_ptr<TireFriction> tireFriction = std::make_shared<TireFriction>();
    float maxDriveTorque = 0.0f;
    float maxBrakeTorque = 0.0f;
    float maxBrakeTorqueRest = 1000.0f;
    float maxSteerAngle = 0.0f;
    float maxRpm = 0.0f;
    float wheelDamping = 0.95f;
    NativeRigidbody* tgtRb = nullptr;
    bool keyboardSteering = false, keyboardDrive = false, keyboardBrakes = false;
    float driveInput = 0.0f, brakeInput = 0.0f, steerInput = 0.0f;
    float steeringResponse = 0.0f, driveResponse = 0.0f, brakeResponse = 0.0f;
    float driveState = 0.0f, steerState = 0.0f, brakeState = 0.0f;
    NativeTransform* wcTransform = nullptr;
    // Input.GetKey(KeyCode) of the keyboard* modes (A / D steer, W / S drive, Space brake); unset: no key pressed
    std::function<bool(int keyCode)> getKey;
    std::function<bool(const WheelState&)> OnShouldIgnoreForces = [](const WheelState&) { return true; };

    // Unity's component construction: AddComponent<KSPWheelController>() on `host` runs Awake, then OnEnable if still enabled.
    // `rb` is the rigidbody of the wheel part, `hostTransform` its transform, `wheelColliderHost` the wheel collider's transform; the
    // controller owns the VPWheelCollider it adds there.
    static std::shared_ptr<KSPWheelController> Create(NativeRigidbody* rb, NativeTransform* hostTransform, NativeTransform* wheelColliderHost);

    NativeRigidbody* rb() const { return rigidbody; }
    bool IsGrounded() const;
    void IsGrounded(bool value);
    Vector3f SuspensionAxis() const;
    NativeRigidbody* RbTgt() const { return tgtRb; }
    WheelState* currentState() const;
    float LegacyWheelLoad();
    Vector3f WheelCenter() const;
    float WheelRadius() const;
    Vector3f WheelDown() const;
    Vector3f WheelBase() const;
    void SetRigidbodyTarget(NativeRigidbody* tgt);
    void Awake();
    void Initialize();
    void SetWheelTransform(const Vector3f& position, const Quaternionf& rotation);

    static constexpr int KeyA = 97, KeyD = 100, KeyS = 115, KeyW = 119, KeySpace = 32;  // UnityEngine.KeyCode

protected:
    void OnInitialize() override;
    void OnUpdate() override;
    void DoUpdateBlocks() override;

private:
    std::unique_ptr<VPWheelCollider> m_ownedWheelCollider;
    std::shared_ptr<DirectDrive> m_directDrive;
    bool init = false;
};

}  // namespace ksp::VehiclePhysics
