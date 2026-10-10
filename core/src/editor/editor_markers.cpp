// WP-40: EditorMarker_CoM.cs / EditorMarker_CoL.cs / EditorMarker_CoT.cs.
#include "ksp/editor/editor_markers.hpp"

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/editor/editor_logic.hpp"
#include "ksp/engines/engine_interfaces.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp {

namespace {

// `EditorLogic.SelectedPart && !EditorLogic.fetch.ship.Contains(SelectedPart) && SelectedPart.potentialParent`: the part being dragged counts
Part* DraggedPart() {
    EditorLogic* e = EditorLogic::fetch;
    if (e == nullptr || e->ship == nullptr) return nullptr;
    Part* sel = e->SelectedPart();
    if (sel != nullptr && !e->ship->Contains(sel) && sel->potentialParent != nullptr) return sel;
    return nullptr;
}

}  // namespace

// ---- EditorMarker_CoM -------------------------------------------------------------------------------------------------------------------

// EditorMarker_CoM.cs:31
Vector3f EditorMarker_CoM::findCenterOfMass(Part* root) {
    if (EditorLogic::fetch == nullptr) return Vector3f::zero();   // EditorMarker_CoM.cs:33 (HighLogic.LoadedSceneIsEditor && EditorLogic.fetch == null)
    Vector3f CoM = Vector3f::zero();
    float m = 0.0f;
    recurseParts(root, CoM, m);
    if (Part* sel = DraggedPart()) {
        recurseParts(sel, CoM, m);
        for (size_t i = 0; i < sel->symmetryCounterparts.size(); i++) recurseParts(sel->symmetryCounterparts[i], CoM, m);
    }
    return CoM / m;
}

// EditorMarker_CoM.cs:51 (`m += part.mass + part.GetResourceMass()` is one double expression rounded at the store)
void EditorMarker_CoM::recurseParts(Part* part, Vector3f& CoM, float& m) {
    auto add = [&](Part& at) {
        const float resMass = part->GetResourceMass();
        CoM += (at.partTransform.position + at.partTransform.rotation * at.fields.CoMOffset) *
               f32store(static_cast<double>(part->fields.mass) + static_cast<double>(resMass));
        m = f32store(static_cast<double>(m) + (static_cast<double>(part->fields.mass) + static_cast<double>(part->GetResourceMass())));
    };
    if (part->fields.physicalSignificance == static_cast<int32_t>(Part_PhysicalSignificance::FULL)) add(*part);
    else if (part->parent != nullptr) add(*part->parent);
    else if (part->potentialParent != nullptr) add(*part->potentialParent);
    for (size_t i = 0; i < part->children.size(); i++) recurseParts(part->children[i], CoM, m);
}

// ---- EditorMarker_CoL -------------------------------------------------------------------------------------------------------------------

// the editor branch of EditorMarker_CoL.UpdatePosition (EditorMarker_CoL.cs:40)
Vector3f EditorMarker_CoL::EditorReferenceVelocity(const Quaternionf& vesselRotation, Part& rootPart, float referencePitch, float referenceSpeed) {
    Vector3f v = vesselRotation * (UnityNative::AngleAxis(referencePitch, rootPart.partTransform.rotation * Vector3f::right()) * Vector3f::up());
    v *= referenceSpeed;
    return v;
}

// EditorMarker_CoL.cs:87
EditorRay EditorMarker_CoL::FindCoL(Part* rootPart, const Vector3f& refVel, double refAlt, double refStp, double refDens) {
    bool noLift = false;
    return FindCoL(rootPart, refVel, refAlt, refStp, refDens, noLift);
}

// EditorMarker_CoL.cs:90
EditorRay EditorMarker_CoL::FindCoL(Part* rootPart, const Vector3f& refVel, double refAlt, double refStp, double refDens, bool& noLift) {
    Vector3f CoL = Vector3f::zero();
    Vector3f DoL = Vector3f::zero();
    float t = 0.0f;
    noLift = true;
    if (rootPart != nullptr) recurseParts(rootPart, refVel, CoL, DoL, t, refAlt, refStp, refDens);
    if (Part* sel = DraggedPart()) {
        recurseParts(sel, refVel, CoL, DoL, t, refAlt, refStp, refDens);
        for (size_t i = 0; i < sel->symmetryCounterparts.size(); i++) recurseParts(sel->symmetryCounterparts[i], refVel, CoL, DoL, t, refAlt, refStp, refDens);
    }
    if (t != 0.0f) {
        const float num = f32store(1.0 / static_cast<double>(t));
        CoL *= num;
        DoL *= num;
        noLift = false;
        return EditorRay(CoL, DoL);
    }
    return EditorRay(Vector3f::zero(), Vector3f::zero());
}

// EditorMarker_CoL.cs:130
void EditorMarker_CoL::recurseParts(Part* part, const Vector3f& refVel, Vector3f& CoL, Vector3f& DoL, float& t, double refAlt, double refStp, double refDens) {
    static CenterOfLiftQuery lQry;
    int count = static_cast<int>(part->Modules().Count());
    while (count-- > 0) {
        if (auto* liftProvider = dynamic_cast<ILiftProvider*>(part->Modules().GetModule(static_cast<size_t>(count)))) {
            lQry.Reset();
            lQry.refVector = refVel;
            lQry.refAltitude = refAlt;
            lQry.refStaticPressure = refStp;
            lQry.refAirDensity = refDens;
            liftProvider->OnCenterOfLiftQuery(lQry);
            CoL += lQry.pos * lQry.lift;
            DoL += lQry.dir * lQry.lift;
            t = f32store(static_cast<double>(t) + static_cast<double>(lQry.lift));
        }
    }
    for (size_t i = 0; i < part->children.size(); i++) recurseParts(part->children[i], refVel, CoL, DoL, t, refAlt, refStp, refDens);
}

// ---- EditorMarker_CoT -------------------------------------------------------------------------------------------------------------------

// EditorMarker_CoT.cs:37
EditorRay EditorMarker_CoT::FindCoT(Part* rootPart) {
    float t = 0.0f;
    Vector3f origin = Vector3f::zero();
    Vector3f direction = Vector3f::zero();
    if (rootPart != nullptr) recurseParts(rootPart, origin, direction, t);
    if (Part* sel = DraggedPart()) {
        recurseParts(sel, origin, direction, t);
        for (size_t i = 0; i < sel->symmetryCounterparts.size(); i++) recurseParts(sel->symmetryCounterparts[i], origin, direction, t);
    }
    if (t != 0.0f) {
        const float num = f32store(1.0 / static_cast<double>(t));
        origin *= num;
        direction *= num;
        CoT = EditorRay(origin, direction);
        return CoT;
    }
    CoT = EditorRay(Vector3f::zero(), Vector3f::zero());
    return CoT;
}

// EditorMarker_CoT.cs:75
void EditorMarker_CoT::recurseParts(Part* part, Vector3f& origin, Vector3f& direction, float& t) {
    static CenterOfThrustQuery tQry;
    int count = static_cast<int>(part->Modules().Count());
    while (count-- > 0) {
        if (auto* thrustProvider = dynamic_cast<IThrustProvider*>(part->Modules().GetModule(static_cast<size_t>(count)))) {
            tQry.Reset();
            thrustProvider->OnCenterOfThrustQuery(tQry);
            origin += tQry.pos * tQry.thrust;
            direction += tQry.dir * tQry.thrust;
            t = f32store(static_cast<double>(t) + static_cast<double>(tQry.thrust));
        }
    }
    for (size_t i = 0; i < part->children.size(); i++) recurseParts(part->children[i], origin, direction, t);
}

}  // namespace ksp
