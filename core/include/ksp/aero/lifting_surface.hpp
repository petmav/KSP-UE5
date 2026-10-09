// WP-21: lifting and control surfaces (ModuleLiftingSurface.cs, ModuleControlSurface.cs, ModuleAeroSurface.cs).
//
// The force maths is a set of pure functions over a LiftingSurfaceParams view of the module (templated over the float
// evaluation policy, ksp/aero/float_eval.hpp); the PartModule classes read their KSPFields from the WP-06 FieldStore,
// call them and hand the resulting forces to the physics engine (WP-20) instead of Part.AddForceAtPosition.
// Unity transforms (baseTransform, ctrlSurface, the vessel ReferenceTransform) and rigidbody point velocities are inputs
// (LiftingSurfaceFrame / ControlSurfaceFrame).
//
// Not ported: ModuleLiftingSurface.cs:280-362 UpdateAeroDisplay / DestroyLiftAndDragArrows (ArrowPointer rendering),
// :364-367 GetInfo, :392-426 module adjusters (Missions adjusters: ApplyLiftForceAdjustments is the identity without them) and
// display-name localisation; ModuleControlSurface.cs:198-253 OnDestroy / editor / PAW / variant events, :360-364
// onVesselReferenceTransformSwitch, :382-393 and :451-462 the displaceVelocity (rotor blade) branches and blade info display,
// :464-467 CalcAngleOfAttack, :469-524 GetPotentialLift / GetPotentialTorque (ITorqueProvider for SAS: WP-24), :526-532
// LateUpdate, :544-702 RotatingCtrlSurfaceUpdate and :971-993 CalculateCoM (robotic rotor blades: the Serenity robotics
// package), :757-801 CtrlSurfaceEditorUpdate (EditorLogic), :802-889 KSPActions (they only flip the bool fields), :933-962
// adjusters / GetInfo / display names; ModuleAeroSurface.cs:40-43 Start (action activity), :112-151 inventory callbacks,
// :153-163 ActionToggleBrakes, :165-173 GetInfo, :205-234 CtrlSurfaceEditorUpdate / display name.
#pragma once

#include <functional>
#include <limits>
#include <string>
#include <vector>

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/aero/float_eval.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class FlightCtrlState;
class Part;

namespace aero {

// The module state GetLiftVector / GetDragVector read (ModuleLiftingSurface fields).
struct LiftingSurfaceParams {
    const FloatCurve* liftCurve = nullptr;
    const FloatCurve* liftMachCurve = nullptr;
    const FloatCurve* dragCurve = nullptr;
    const FloatCurve* dragMachCurve = nullptr;
    float deflectionLiftCoeff = 1.5f;
    bool perpendicularOnly = false;
    bool nodeBlocked = false;  // nodeEnabled && attachNode.attachedPart != null
    Vector3f nVel{};           // the module's nVel field (read by the perpendicularOnly branch)
};

// ModuleLiftingSurface.cs:205 SetupCoefficients. `axis` = baseTransform.forward / up / right per transformDir.
struct LiftingSurfaceCoefficients {
    Vector3f nVel{};
    Vector3f liftVector{};
    float liftDot = 0.0f;
    float absDot = 0.0f;
};
template <class P = MonoFloat>
LiftingSurfaceCoefficients SetupCoefficients(const Vector3f& pointVelocity, const Vector3f& axis, float transformSign,
                                             bool omnidirectional);
// ModuleLiftingSurface.cs:232 GetLiftVector; writes the module's liftScalar
template <class P = MonoFloat>
Vector3f GetLiftVector(const LiftingSurfaceParams& m, const Vector3f& liftVector, float liftDot, float absDot, double Q,
                       float mach, float& liftScalar);
// ModuleLiftingSurface.cs:262 GetDragVector(nVel, absDot, Q, mach); writes the module's dragScalar
template <class P = MonoFloat>
Vector3f GetDragVector(const LiftingSurfaceParams& m, const Vector3f& nVel, float absDot, double Q, float mach,
                       float& dragScalar);

}  // namespace aero

// The Unity state a lifting surface's FixedUpdate reads.
struct LiftingSurfaceFrame {
    Vector3f pointVelocity{};  // Rigidbody.GetPointVelocity(baseTransform position / velocityOffset) + Krakensbane.GetFrameVelocityV3f()
    Vector3f baseForward{0.0f, 0.0f, 1.0f}, baseUp{0.0f, 1.0f, 0.0f}, baseRight{1.0f, 0.0f, 0.0f};  // baseTransform axes
    Quaternionf baseRotation = Quaternionf::identity();  // baseTransform.rotation
    Vector3f colWorld{};       // part.partTransform.TransformPoint(part.CoLOffset)
    Vector3f copWorld{};       // part.partTransform.TransformPoint(part.CoPOffset)
    bool hasRigidbody = true;  // part.Rigidbody != null
};

// Forces a surface asks the physics engine to apply (Part.AddForceAtPosition).
struct SurfaceForces {
    bool applied = false;
    Vector3f liftForce{}, liftPosition{};
    bool applyDrag = false;
    Vector3f dragForce{}, dragPosition{};
};

// ModuleLiftingSurface.cs
class ModuleLiftingSurface : public PartModule, public ILiftProvider {
public:
    enum class TransformDir : int32_t { X = 0, Y = 1, Z = 2 };  // :8
    explicit ModuleLiftingSurface(const ModuleClassInfo& cls) : PartModule(cls) {}

    // non-KSPField state (:68-111)
    Vector3f liftForce{}, dragForce{};
    float liftDot = 0.0f;
    Vector3f nVel{}, liftVector{}, pointVelocity{};
    float absDot = 0.0f;
    double Qlift = 0.0, Qdrag = 0.0;

    void OnStart(StartState state) override;  // :128 (curves of a non-"Custom" liftingSurfaceCurve from PhysicsGlobals)
    // :170 FixedUpdate (flight scene); returns the forces instead of Part.AddForceAtPosition
    SurfaceForces FixedUpdateAero(const LiftingSurfaceFrame& frame);
    // :205 SetupCoefficients (writes nVel / liftVector / liftDot / absDot through the out parameters like C#)
    void SetupCoefficients(const Vector3f& pointVelocity, const LiftingSurfaceFrame& frame, Vector3f& nVel, Vector3f& liftVector,
                           float& liftDot, float& absDot);
    Vector3f GetLiftVector(const Vector3f& liftVector, float liftDot, float absDot, double Q, float mach);  // :232
    Vector3f GetDragVector(const Vector3f& nVel, float absDot, double Q);              // :257 (mach = part.machNumber)
    Vector3f GetDragVector(const Vector3f& nVel, float absDot, double Q, float mach);  // :262
    // ILiftProvider (:117-119, :369). speedOfSound = mainBody.GetSpeedOfSound(qry.refStaticPressure, qry.refAirDensity)
    bool DisableBodyLift() const override { return Get<bool>("disableBodyLift"); }
    bool IsLifting() const override { return Get<float>("liftScalar") != 0.0f; }
    void OnCenterOfLiftQuery(CenterOfLiftQuery& qry) override;
    // OnCenterOfLiftQuery with the frame (baseTransform) and the body's speed of sound given
    void OnCenterOfLiftQuery(CenterOfLiftQuery& qry, const LiftingSurfaceFrame& frame, double speedOfSound,
                             const Vector3f& colWorldFromBase);
    aero::LiftingSurfaceParams Params() const;
    // Unset: OnCenterOfLiftQuery(qry) uses the identity frame and Kerbin-less speed of sound 0 (no query in that case)
    struct Hooks {
        std::function<double(double staticPressure, double density)> speedOfSound;  // FlightGlobals body GetSpeedOfSound
    };
    static inline Hooks hooks;
    Vector3f AxisOf(const LiftingSurfaceFrame& frame) const;  // baseTransform.forward / up / right per transformDir
};

// The extra Unity state ModuleControlSurface reads.
struct ControlSurfaceFrame {
    LiftingSurfaceFrame surface;
    bool hasCtrlSurface = true;             // ctrlSurface != null
    bool ctrlSurfaceAtNeutral = true;       // ctrlSurface.localRotation == neutral (Unity's approximate ==)
    Vector3f localCoM{};                    // baseTransform.InverseTransformPoint(vessel.CurrentCoM)
    Vector3f currentCoM{};                  // vessel.CurrentCoM
    Vector3f basePosition{};                // baseTransform.position
    Quaternionf referenceRotation = Quaternionf::identity();  // vessel.ReferenceTransform.rotation
    Vector3f referenceUp{0.0f, 1.0f, 0.0f};                    // vessel.ReferenceTransform.up
    float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;               // vessel.ctrlState
    float fixedDeltaTime = 0.02f;                              // TimeWarp.fixedDeltaTime
};

// ModuleControlSurface.cs
class ModuleControlSurface : public ModuleLiftingSurface, public IMultipleDragCube {
public:
    explicit ModuleControlSurface(const ModuleClassInfo& cls) : ModuleLiftingSurface(cls) {}

    float originalActuatorSpeed = 0.0f;   // :23
    float maxAuthority = 150.0f;          // :88
    Vector3f inputVector{};
    Vector3f baseLiftForce{};
    float action = 0.0f, deflection = 0.0f, roll = 0.0f, deflectionDirection = 1.0f;  // :98-104
    Quaternionf neutral = Quaternionf::identity();      // :111 ctrlSurface's neutral localRotation
    Quaternionf ctrlSurfaceLocalRotation = Quaternionf::identity();  // ctrlSurface.localRotation (the output)
    Quaternionf airflowIncidence = Quaternionf::identity();
    float currentDeployAngle = 0.0f;
    Vector2f deployAngleLimits{std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()};  // :79

    void OnAwake() override;                   // :170 (originalActuatorSpeed)
    void OnStart(StartState state) override;   // :273 (NaN defaults, ModifyAuthorityLimiter)
    void ModifyAuthorityLimiter();             // :255
    void ModifyAuthorityLimiterUI();           // :268
    // :366 FixedUpdate (the force part and the cube weights; transforms in `frame`)
    SurfaceForces FixedUpdateAero(const ControlSurfaceFrame& frame);
    // :704 FixedCtrlSurfaceUpdate (deflection maths; ctrlSurfaceLocalRotation = AngleAxis(deflection, right) * neutral)
    virtual void CtrlSurfaceUpdate(const ControlSurfaceFrame& frame);  // :745 (displaceVelocity -> rotating update: not ported)
    void FixedCtrlSurfaceUpdate(const ControlSurfaceFrame& frame);
    // IMultipleDragCube (:891-931)
    bool IsMultipleCubesActive() const override { return true; }
    std::vector<std::string> GetDragCubeNames() const override { return {"neutral", "fullDeflectionPos", "fullDeflectionNeg"}; }
    void AssumeDragCubePosition(const std::string& name) override;  // :896 (the ctrlSurface localRotation it would set)
    bool UsesProceduralDragCubes() const override { return false; }
    static float ActionSign(float action, float epsilon);  // :539 (sic: `action < epsilon`, not `< -epsilon`)
};

// ModuleAeroSurface.cs (airbrakes)
class ModuleAeroSurface : public ModuleControlSurface {
public:
    explicit ModuleAeroSurface(const ModuleClassInfo& cls) : ModuleControlSurface(cls) {}
    Vector2f aeroDeployAngleLimits{std::numeric_limits<float>::quiet_NaN(), 0.0f};  // :30
    void OnAwake() override;                   // :34
    void OnStart(StartState state) override;   // :63
    void ModifyAeroAuthorityLimiter();         // :45
    void ModifyAeroAuthorityLimiterUI();       // :58
    void CtrlSurfaceUpdate(const ControlSurfaceFrame& frame) override;  // :175 (skinMaxTemp switch included)
};

// Registers ModuleLiftingSurface, ModuleControlSurface, ModuleAeroSurface with the ModuleFactory (called by the built-in
// module registration of src/modules/module_behaviours.cpp)
void RegisterAeroModules();

}  // namespace ksp
