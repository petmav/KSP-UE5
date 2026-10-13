// WP-20e: PartBuoyancy (PartBuoyancy.cs), the water model of a physical part (Part.Start adds one to every part with a Rigidbody,
// Part.cs:1885; enabled while the part is not packed, Part.Pack / Unpack :3756 / :3790). One FixedUpdate per part in the module slot of the
// tick (seq 13, after the part's modules; PHYSICS-ENGINE-REVIEW.md 1.2 / 3.6):
//   * the part's displaced volume from its drag cube box (UpdateDisplacement :171: WeightedSize x / y / z scaled by the cube's x / z and y
//     area portions; a named override cube; a PACKED cube for parachutes),
//   * the depth of its centre of displacement and of the 8 corners of its cube bounds below the ocean surface (:243-376; the sea level is
//     the body's radius, plus Vessel.waterOffset + waterLevel),
//   * the submerged portion (:497-514, linear in depth between the shallowest and deepest corner, then mapped through a sine), the centre
//     of buoyancy shifted towards the wet corners (:515-531), the buoyant force -g(CoB) * V * submerged * oceanDensity * BuoyancyScalar *
//     part.buoyancy (ramped in over the first BuoyancyScaleAboveDepth metres) handed to Part.AddForceAtPosition (:533-534, :643),
//   * water drag as PhysX linear damping: rb.drag = 0.5 rho |v| AreaDrag DragCubeMultiplier dragScalar submerged DragMultiplier / rb.mass
//     (or BuoyancyWaterDragSlow at low speed), rb.angularDrag (:539-626), with the splashdown drag / lift scalars easing over time
//     (:381-460) into part.submergedDragScalar / submergedLiftScalar that FlightIntegrator.UpdateAerodynamics reads (WP-21),
//   * the splashdown itself: WaterContact, vessel.Splashed, the splash FX and the crash test max(BuoyancyMinCrashMult |v|, v_down) >
//     crashTolerance * BuoyancyCrashToleranceMult -> onCrashSplashdown and Part.Die (:461-494, :633-640); leaving the water resets it
//     (:646-666, vessel.checkSplashed).
// Every float statement follows the Mono rule (ksp/dotnet/mono_float.hpp); FlightGlobals' altitude / up axis / gravity are the WP-19a ports.
// The Unity state (the Rigidbody, the transform matrix, the camera, FX, Invoke) is `Env`, filled by the contact host from the bubble.
#pragma once

#include <array>
#include <functional>

#include "ksp/math/vector3d.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class CelestialBody;
class DragCube;

class PartBuoyancy {
public:
    struct Env {
        std::function<bool(Part&)> hasRigidbody;                 // part.rb != null
        std::function<Vector3f(Part&)> rbVelocity;               // part.rb.velocity
        std::function<float(Part&)> rbMass;                      // part.rb.mass
        std::function<float(Part&)> rbDrag;                      // part.rb.drag (get)
        std::function<void(Part&, float)> setRbDrag;             // part.rb.drag = (PhysX linear damping)
        std::function<void(Part&, float)> setRbAngularDrag;      // part.rb.angularDrag =
        std::function<Matrix4x4f(Part&)> localToWorld;           // part.partTransform.localToWorldMatrix (native)
        std::function<Vector3d()> frameVelocity;                 // Krakensbane.GetFrameVelocity()
        std::function<Vector3d()> cameraPosition;                // FlightGlobals.camera_position (the splash FX distance test)
        std::function<void(const Vector3f& position, float power)> splashFx;  // FXMonger.Splash
        std::function<double(CelestialBody&)> waterLevelOffset;  // GameSettings.WATERLEVEL_* term of the FX position (num7; unset: 0)
        std::function<void(Part&, const Vector3d& force, const Vector3d& position)> addForceAtPosition;  // Part.AddForceAtPosition
        std::function<Vector3f(Part&)> colliderBoundsSize;       // part.collider.bounds.size (parts without drag cubes; unset: no collider)
    };

    // PartBuoyancy.cs:6-96 fields
    Vector3f centerOfBuoyancy;
    Vector3f centerOfDisplacement;
    Vector3d maxCoords;
    Vector3d minCoords;
    float maxDimension = 0.0f;
    Vector3f boundsCenterLocal, boundsSize;   // `bounds` (center, size)
    std::array<Vector3f, 8> boundCoords{};
    Vector3f boundsCenter;
    std::array<double, 8> depths{};
    bool early = false;
    bool allSplashed = false;
    bool slow = false;
    bool wasSplashed = false;
    bool dead = false;
    static inline bool overrideCubeOnChutesIfUnspecified = true;
    double waterLevel = 0.0;
    CelestialBody* body = nullptr;
    double depth = 0.0;
    double maxDepth = 0.0;
    double minDepth = 0.0;
    double dragScalar = 0.0;
    double splashedCounter = 0.0;
    double liftScalar = 0.0;
    double drag = 0.0;
    double submergedPortion = 0.0;
    Vector3f effectiveForce;
    double buoyantGeeForce = 0.0;
    const DragCube* overrideCube = nullptr;
    double localVelocityMag = 0.0;
    Vector3f lastBuoyantForce;
    Vector3f lastForcePosition;
    bool physWarp = false;
    double xPortion = 0.0, yPortion = 0.0, zPortion = 0.0, xzPortion = 0.0;
    Part* part = nullptr;
    bool splashed = false;
    double displacement = 0.0;
    bool canForceUpdate = false;
    Vector3f pLast;
    Vector3f V;
    Vector3f downAxis;
    float dH = 0.0f;
    bool isKerbal = false;
    bool enabled = true;  // MonoBehaviour.enabled (Part.Pack / Unpack)

    // MonoBehaviour.Invoke("ReportSplashDown", 2f) / IsInvoking / CancelInvoke: game seconds left (< 0: none); TickInvoke runs it
    double invokeIn = -1.0;
    void TickInvoke(double dt);
    // what the last FixedUpdate handed out (the gates)
    bool lastApplied = false;           // AddForceAtPosition was called (or the parent's on death)
    Vector3d lastAppliedForce, lastAppliedPosition;
    bool lastCrash = false;             // onCrashSplashdown fired
    bool lastSplashFx = false;
    Vector3f lastSplashFxPosition;
    float lastSplashFxPower = 0.0f;

    void Start(Part& p, const Env& env);   // :98
    void FixedUpdate(const Env& env);      // :231
    void ReportSplashDown();               // :672
    void FindOverrideCube();               // :147 (private in C#)
    void UpdateDisplacement(const Env& env);  // :171 (private in C#)
};

}  // namespace ksp
