// WP-40: EditorMarker.cs, EditorMarker_CoM.cs, EditorMarker_CoL.cs, EditorMarker_CoT.cs -- the centre of mass / lift / thrust of a craft in the
// editor. The MonoBehaviour shells (marker GameObjects, Update, Awake) are the engine's; the static maths is here.
//
// Reuses WP-14 mass (Part.mass + Part.GetResourceMass), WP-21 ILiftProvider.OnCenterOfLiftQuery and WP-23 IThrustProvider.OnCenterOfThrustQuery.
// The thrust transforms of an engine come from the part model (.mu reader, not ported): ModuleEngines::thrustTransforms are installed by the host
// (see STATUS.md: tools/golden_runner/ds_deltav_stages.cpp installs one transform along the part's -Y per engine).
#pragma once

#include "ksp/parts/part.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// UnityEngine.Ray: Ray(origin, direction) stores direction.normalized
struct EditorRay {
    Vector3f origin{};
    Vector3f direction{};
    EditorRay() = default;
    EditorRay(const Vector3f& o, const Vector3f& d) : origin(o), direction(d.normalized()) {}
};

// EditorMarker_CoM.cs:3
class EditorMarker_CoM {
public:
    static inline Vector3f CraftCoM{};                                // :5
    static Vector3f findCenterOfMass(Part* root);                     // :35
private:
    static void recurseParts(Part* part, Vector3f& CoM, float& m);    // :55
};

// EditorMarker_CoL.cs:4
class EditorMarker_CoL {
public:
    // :40 the editor's reference conditions: referenceVelocity = VesselRotation * (AngleAxis(referencePitch, root.transform.right) * up) * referenceSpeed
    static Vector3f EditorReferenceVelocity(const Quaternionf& vesselRotation, Part& rootPart, float referencePitch, float referenceSpeed);
    static EditorRay FindCoL(Part* rootPart, const Vector3f& refVel, double refAlt, double refStp, double refDens);                   // :87
    static EditorRay FindCoL(Part* rootPart, const Vector3f& refVel, double refAlt, double refStp, double refDens, bool& noLift);    // :93
private:
    static void recurseParts(Part* part, const Vector3f& refVel, Vector3f& CoL, Vector3f& DoL, float& t, double refAlt, double refStp, double refDens);   // :130
};

// EditorMarker_CoT.cs:3
class EditorMarker_CoT {
public:
    static EditorRay FindCoT(Part* rootPart);                         // :42
    static inline EditorRay CoT{};                                    // :5
private:
    static void recurseParts(Part* part, Vector3f& origin, Vector3f& direction, float& t);   // :75
};

}  // namespace ksp
