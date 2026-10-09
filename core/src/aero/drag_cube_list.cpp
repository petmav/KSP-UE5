// WP-21: the aerodynamics half of DragCubeList (DragCubeList.cs); the data half (LoadCubes / SaveCubes / ClearCubes) is
// WP-05's src/parts/part_data.cpp. Every float expression follows the evaluation policy P (ksp/aero/float_eval.hpp):
// MonoFloat (the game) is the default, NetFloat reproduces the .NET golden harness. Both are instantiated at the end.
//
// Not ported: DragCubeList.cs:494-559 SetFacedDotProducts (x2) / SetFacedDotProductsNormalized / AddFacedDotProducts,
// :579-622 DivideCubeArray (x2) / MultiplyCubeArray(float[], float[]), :650-737 CreateVectorFromArray / AddVectorToArray /
// SubtractVectorFromArray(Clamp): private static helpers nothing in DragCubeList calls; :996-1010 DrawDragGizmos (Unity Gizmos); the Debug.Log lines of debugDragCube.
#include <cmath>

#include "ksp/aero/flight_integrator_aero.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_data.hpp"
#include "ksp/physics/physics_globals.hpp"

namespace ksp {

using aero::FE;
using aero::MonoFloat;
using aero::NetFloat;

// DragCubeList.cs:41 CubeData.Reset
void DragCubeList::CubeData::Reset() {
    dragVector = liftForce = Vector3f::zero();
    taperDot = dragCoeff = exposedArea = crossSectionalArea = depth = areaDrag = area = 0.0f;
}

void DragCubeList::CopySerializedFrom(const DragCubeList& original) {
    // [SerializeField] members (DragCubeList.cs:65-83)
    cubes_ = original.cubes_;
    postOcclusionArea_ = original.postOcclusionArea_;
    none_ = original.none_;
    procedural_ = original.procedural_;
    cubeData_ = original.cubeData_;
    debugDragCube_ = original.debugDragCube_;
    // everything else: the C# field initialisers (DragCubeList.cs:75-126)
    part_ = nullptr;
    weightsRequireUpdate_ = true;
    occlusionRequireUpdate_ = true;
    proceduralDragCube_.reset();
    proceduralDragNextUpdate_ = 0.0f;
    occlusionMultiplier_ = 1.0f;
    weightedArea_ = {};
    weightedDrag_ = {};
    weightedDragOrig_ = {};
    weightedDepth_ = {};
    areaOccluded_ = {};
    weightedCenter_ = Vector3f::zero();
    weightedSize_ = Vector3f::zero();
    rotateDragVector_ = false;
    dragVectorRotation_ = Quaternionf::identity();
    DragCurveCd = DragCurveCdPower = DragCurveMultiplier = nullptr;
    BodyLiftCurve.reset();
    SurfaceCurves = PhysicsGlobals::SurfaceCurvesList{};
}

// DragCubeList.cs:128 faceDirections = {right, left, up, down, forward, back}
const std::array<Vector3f, 6>& DragCubeList::FaceDirections() {
    static const std::array<Vector3f, 6> kFaces = {Vector3f::right(), Vector3f::left(), Vector3f::up(),
                                                   Vector3f::down(),  Vector3f::forward(), Vector3f::back()};
    return kFaces;
}

// DragCubeList.cs:204
void DragCubeList::SetPart(Part* part) {
    part_ = part;
    // KSP's PhysicsGlobals always exists; kspcore hosts that load parts without one (data-only tools) keep null curve
    // references, and SetDrag then throws NullReferenceException like the C# would
    if (PhysicsGlobals::Instance() == nullptr) {
        DragCurveCd = DragCurveCdPower = DragCurveMultiplier = nullptr;
        BodyLiftCurve.reset();
        SurfaceCurves = PhysicsGlobals::SurfaceCurvesList{};
        return;
    }
    DragCurveCd = &PhysicsGlobals::DragCurveCd();
    DragCurveCdPower = &PhysicsGlobals::DragCurveCdPower();
    DragCurveMultiplier = &PhysicsGlobals::DragCurveMultiplier();
    BodyLiftCurve = PhysicsGlobals::BodyLiftCurve();
    SurfaceCurves = PhysicsGlobals::SurfaceCurves;
}

// DragCubeList.cs:214
template <class P>
void DragCubeList::SetDragWeights() {
    if (!weightsRequireUpdate_ || none_) return;
    if (procedural_) {
        if (SetDragWeights_Procedural()) weightsRequireUpdate_ = false;
    } else {
        SetDragWeights_List<P>();
        weightsRequireUpdate_ = false;
    }
}

namespace {
// DragCubeList.cs:561 ResetCubeArray
void ResetCubeArray(std::array<float, 6>& arr) { arr.fill(0.0f); }
// DragCubeList.cs:608 MultiplyCubeArray(float[], float): arr[i] *= multiply (one operation per element)
void MultiplyCubeArray(std::array<float, 6>& arr, float multiply) {
    for (float& a : arr) a = static_cast<float>(static_cast<double>(a) * static_cast<double>(multiply));
}
// DragCubeList.cs:624 AddCubeArray: outputArray[i] += inputArray[i] * multiply
template <class P>
void AddCubeArray(std::array<float, 6>& out, const std::array<float, 6>& in, float multiply) {
    using E = FE<P>;
    for (int i = 0; i < 6; i++) out[i] = aero::S(E(out[i]) + E(in[i]) * E(multiply));
}
// DragCubeList.cs:632 SetCubeArray: outputArray[i] = inputArray[i] * multiply (multiply = 1f: exact)
void SetCubeArray(std::array<float, 6>& out, const std::array<float, 6>& in, float multiply = 1.0f) {
    for (int i = 0; i < 6; i++) out[i] = static_cast<float>(static_cast<double>(in[i]) * static_cast<double>(multiply));
}
// DragCubeList.cs:640 SumCubeArray: num += arr[i] (a float local: one rounding per addition)
float SumCubeArray(const std::array<float, 6>& arr) {
    float num = 0.0f;
    for (float a : arr) num = static_cast<float>(static_cast<double>(num) + static_cast<double>(a));
    return num;
}
// DragCubeList.cs:739 GetFacingAreaSum
template <class P>
float GetFacingAreaSum(const Vector3f& direction, const std::array<float, 6>& faceAreaArray) {
    using E = FE<P>;
    float num = 0.0f;
    for (int i = 0; i < 6; i++) {
        const float num2 = Mathf::Clamp01(aero::U<P>::Dot(direction, DragCubeList::FaceDirections()[i]));
        num = aero::S(E(num) + E(faceAreaArray[i]) * E(num2));
    }
    return num;
}
}  // namespace

// DragCubeList.cs:234
template <class P>
void DragCubeList::SetDragWeights_List() {
    using E = FE<P>;
    ResetCubeArray(weightedArea_);
    ResetCubeArray(weightedDrag_);
    ResetCubeArray(weightedDragOrig_);
    ResetCubeArray(weightedDepth_);
    weightedCenter_ = Vector3f::zero();
    weightedSize_ = Vector3f::zero();
    const int count = static_cast<int>(cubes_.size());
    if (count == 0) return;
    float num = 0.0f;
    int index = count;
    while (index-- > 0) {
        const DragCube& dragCube = cubes_[static_cast<size_t>(index)];
        if (dragCube.Weight() != 0.0f) {
            num = aero::S(E(num) + E(dragCube.Weight()));
            AddCubeArray<P>(weightedArea_, dragCube.Area(), dragCube.Weight());
            AddCubeArray<P>(weightedDrag_, dragCube.Drag(), dragCube.Weight());
            AddCubeArray<P>(weightedDragOrig_, dragCube.Drag(), dragCube.Weight());
            AddCubeArray<P>(weightedDepth_, dragCube.Depth(), dragCube.Weight());
            weightedCenter_ += dragCube.Center() * dragCube.Weight();
            weightedSize_ += dragCube.Size() * dragCube.Weight();
        }
    }
    if (num == 0.0f) {
        ResetCubeArray(weightedArea_);
        ResetCubeArray(weightedDrag_);
        ResetCubeArray(weightedDragOrig_);
        ResetCubeArray(weightedDepth_);
        weightedCenter_ = Vector3f::zero();
        weightedSize_ = Vector3f::zero();
    } else {
        const float num2 = aero::S(E(1.0f) / E(num));
        MultiplyCubeArray(weightedArea_, num2);
        MultiplyCubeArray(weightedDrag_, num2);
        MultiplyCubeArray(weightedDragOrig_, num2);
        MultiplyCubeArray(weightedDepth_, num2);
        weightedCenter_ *= num2;
        weightedSize_ *= num2;
    }
}

// DragCubeList.cs:284
bool DragCubeList::SetDragWeights_Procedural() {
    if (!hooks.realtimeSinceStartup || !hooks.renderProceduralDragCube) return false;
    const float now = hooks.realtimeSinceStartup();
    if (now > proceduralDragNextUpdate_) {
        const float interval = hooks.proceduralDragUpdateInterval ? hooks.proceduralDragUpdateInterval() : 0.0f;
        proceduralDragNextUpdate_ = static_cast<float>(static_cast<double>(now) + static_cast<double>(interval));
        if (part_ == nullptr) throw NullReferenceException();
        proceduralDragCube_ = hooks.renderProceduralDragCube(*part_);
        SetCubeArray(weightedArea_, proceduralDragCube_->Area());
        SetCubeArray(weightedDrag_, proceduralDragCube_->Drag());
        SetCubeArray(weightedDragOrig_, proceduralDragCube_->Drag());
        SetCubeArray(weightedDepth_, proceduralDragCube_->Depth());
        weightedCenter_ = proceduralDragCube_->Center();
        weightedSize_ = proceduralDragCube_->Size();
        return true;
    }
    return false;
}

// DragCubeList.cs:301
template <class P>
void DragCubeList::SetPartOcclusion() {
    using E = FE<P>;
    if (!occlusionRequireUpdate_ || none_) return;
    occlusionRequireUpdate_ = false;
    SetCubeArray(areaOccluded_, weightedArea_);
    SetCubeArray(weightedDrag_, weightedDragOrig_);
    if (part_ == nullptr) throw NullReferenceException();
    Part& part = *part_;
    int count = static_cast<int>(part.attachNodes.size());
    while (count-- > 0) {
        AttachNode& attachNode = part.attachNodes[static_cast<size_t>(count)];
        Part* attachedPart = attachNode.attachedPart;
        if (attachedPart == nullptr) continue;
        // AttachNode attachNode2 = attachedPart.FindAttachNodeByPart(part): only read by the debugDragCube log
        if (attachedPart->fields.dragModel != static_cast<int32_t>(4) /* Part.DragModel.CUBE */) continue;
        const Vector3f normalized = aero::U<P>::Normalize(attachNode.orientation);
        const Vector3f vector = aero::U<P>::Rotate(
            aero::U<P>::Mul(aero::U<P>::Inverse(attachedPart->partTransform.rotation), part.partTransform.rotation), normalized);
        const float num = (attachNode.overrideDragArea >= 0.0f) ? attachNode.overrideDragArea
                                                                : GetFacingAreaSum<P>(-vector, attachedPart->DragCubes.weightedArea_);
        AreaToCubeOperation<P>(normalized, aero::S(E(num) * E(occlusionMultiplier_)));
        attachNode.contactArea = num;
    }
    postOcclusionArea_ = SumCubeArray(areaOccluded_);
    if (part.srfAttachNode != nullptr && part.srfAttachNode->attachedPart != nullptr) {
        AttachNode& attachNode = *part.srfAttachNode;
        const float facingAreaSum = GetFacingAreaSum<P>(aero::U<P>::Normalize(attachNode.orientation), part.DragCubes.weightedArea_);
        attachNode.contactArea = facingAreaSum;
    }
}

// DragCubeList.cs:750 AreaToCubeOperation with SetPartOcclusion's operation (fArea, cArea) => Mathf.Max(0f, cArea - fArea)
template <class P>
void DragCubeList::AreaToCubeOperation(const Vector3f& direction, float area) {
    using E = FE<P>;
    for (int i = 0; i < 6; i++) {
        const float num = Mathf::Clamp01(aero::U<P>::Dot(direction, FaceDirections()[i]));
        if (num > 0.0f) {
            const float fArea = aero::S(E(area) * E(num));  // lambda argument
            areaOccluded_[i] = Mathf::Max(0.0f, aero::S(E(areaOccluded_[i]) - E(fArea)));
            const float num3 = aero::S(E(weightedArea_[i]) - E(areaOccluded_[i]));
            if (areaOccluded_[i] > 0.0f)
                weightedDrag_[i] = Mathf::Max(0.0f, aero::S((E(weightedDragOrig_[i]) * E(weightedArea_[i]) - E(num3)) / E(areaOccluded_[i])));
            else
                weightedDrag_[i] = 1E-05f;
        }
    }
}

// DragCubeList.cs:352
template <class P>
void DragCubeList::SetDrag(const Vector3f& vector, float machNumber) {
    if (none_) return;
    if (rotateDragVector_) AddSurfaceDragDirection<P>(aero::U<P>::Rotate(dragVectorRotation_, -vector), machNumber, cubeData_);
    else AddSurfaceDragDirection<P>(-vector, machNumber, cubeData_);
}

namespace {
// PhysicsGlobals.cs:2692 DragCurveValue under policy P (PhysicsGlobals::DragCurveValue is the MonoFloat instance; the
// equality is checked in tests/test_aero.cpp and by capture.physics_drag_curve)
template <class P>
float DragCurveValueP(const PhysicsGlobals::SurfaceCurvesList& cuves, float dotNormalized, float mach) {
    using E = FE<P>;
    if (dotNormalized <= 0.5f) {
        const float a = cuves.dragCurveTail->Evaluate(mach);
        const float b = cuves.dragCurveSurface->Evaluate(mach);
        const float num = cuves.dragCurveMultiplier->Evaluate(mach);
        return aero::S(E(aero::U<P>::Lerp(a, b, aero::S(E(dotNormalized) * E(2.0f)))) * E(num));
    }
    const float a2 = cuves.dragCurveSurface->Evaluate(mach);
    const float b2 = cuves.dragCurveTip->Evaluate(mach);
    const float num2 = cuves.dragCurveMultiplier->Evaluate(mach);
    return aero::S(E(aero::U<P>::Lerp(a2, b2, aero::S((E(dotNormalized) - E(0.5f)) * E(2.0f)))) * E(num2));
}
}  // namespace

template <class P>
float aero::DragCurveValue(const PhysicsGlobals::SurfaceCurvesList& curves, float dotNormalized, float mach) {
    return DragCurveValueP<P>(curves, dotNormalized, mach);
}

// DragCubeList.cs:367
template <class P>
void DragCubeList::AddSurfaceDragDirection(const Vector3f& direction, float machNumber, CubeData& retData) const {
    using E = FE<P>;
    using aero::S;
    if (DragCurveCd == nullptr || DragCurveCdPower == nullptr || DragCurveMultiplier == nullptr || BodyLiftCurve == nullptr ||
        SurfaceCurves.dragCurveTail == nullptr)
        throw NullReferenceException();
    float num = 0.0f;
    retData.Reset();
    retData.dragVector = direction;
    for (int i = 0; i < 6; i++) {
        const Vector3f& vector = FaceDirections()[i];
        const float num2 = aero::U<P>::Dot(direction, vector);
        const float dotNormalized = S((E(num2) + E(1.0f)) * E(0.5f));
        const float num3 = DragCurveValueP<P>(SurfaceCurves, dotNormalized, machNumber);
        const float num4 = S(E(areaOccluded_[i]) * E(num3));
        float num5 = weightedDrag_[i];
        retData.area = S(E(retData.area) + E(num4));
        float num6 = num5;
        if (num6 < 1.0f) num6 = Mathf::Pow(DragCurveCd->Evaluate(num5), DragCurveCdPower->Evaluate(machNumber));
        retData.areaDrag = S(E(retData.areaDrag) + E(num4) * E(num6));
        retData.crossSectionalArea = S(E(retData.crossSectionalArea) + E(areaOccluded_[i]) * E(Mathf::Clamp01(num2)));
        num5 = (!(num5 < 1.0f) || !(num5 > 0.01f)) ? 1.0f : S(E(1.0f) / E(num5));
        retData.exposedArea = S(E(retData.exposedArea) + E(num4) / E(DragCurveMultiplier->Evaluate(machNumber)) * E(num5));
        if (num2 > 0.0f) {
            num = S(E(num) + E(num2));
            const double num7 = static_cast<double>(BodyLiftCurve->liftCurve.Evaluate(num2));
            if (!std::isnan(num7)) {
                const float scale = S(E(num2) * E(areaOccluded_[i]) * E(weightedDrag_[i]) * E(static_cast<float>(num7)));
                retData.liftForce += -vector * scale;
            }
            retData.depth = S(E(retData.depth) + E(num2) * E(weightedDepth_[i]));
            retData.taperDot = S(E(retData.taperDot) + E(num2) * E(num5));
        }
    }
    if (num > 0.0f) {
        const float num8 = S(E(1.0f) / E(num));
        retData.depth = S(E(retData.depth) * E(num8));
        retData.taperDot = S(E(retData.taperDot) * E(num8));
    }
    if (retData.area > 0.0f) {
        retData.dragCoeff = S(E(retData.areaDrag) / E(retData.area));
        retData.areaDrag = S(E(retData.area) * E(retData.dragCoeff));
    } else {
        retData.areaDrag = 0.0f;
        retData.dragCoeff = 0.0f;
    }
}

// DragCubeList.cs:421
template <class P>
float DragCubeList::GetCubeAreaDir(const Vector3f& dir, const std::array<float, 6>& areas) const {
    using E = FE<P>;
    float num = 0.0f;
    for (int i = 0; i < 6; i++) {
        const float num2 = aero::U<P>::Dot(dir, FaceDirections()[i]);
        if (num2 > 0.0f) num = aero::S(E(num) + E(areas[i]) * E(num2));
    }
    return num;
}

// DragCubeList.cs:441
template <class P>
float DragCubeList::GetCubeCoeffDir(const Vector3f& dir) const {
    using E = FE<P>;
    float num = 0.0f, num2 = 0.0f;
    for (int i = 0; i < 6; i++) {
        const float num3 = aero::U<P>::Dot(dir, FaceDirections()[i]);
        if (num3 > 0.0f) {
            num = aero::S(E(num) + E(weightedDrag_[i]) * E(num3));
            num2 = aero::S(E(num2) + E(num3));
        }
    }
    return aero::S(E(num) / E(num2));
}

// DragCubeList.cs:458
template <class P>
float DragCubeList::GetCubeDepthDir(const Vector3f& dir) const {
    using E = FE<P>;
    float num = 0.0f, num2 = 0.0f;
    for (int i = 0; i < 6; i++) {
        const float num3 = aero::U<P>::Dot(dir, FaceDirections()[i]);
        if (num3 > 0.0f) {
            num = aero::S(E(num) + E(weightedDepth_[i]) * E(num3));
            num2 = aero::S(E(num2) + E(num3));
        }
    }
    return aero::S(E(num) / E(num2));
}

// DragCubeList.cs:475
Vector3f DragCubeList::GetFaceDirection(DragCube::DragFace direction) {
    switch (direction) {
        case DragCube::DragFace::XP: return Vector3f::right();
        case DragCube::DragFace::XN: return Vector3f::left();
        case DragCube::DragFace::YP: return Vector3f::up();
        case DragCube::DragFace::YN: return Vector3f::down();
        case DragCube::DragFace::ZP: return Vector3f::forward();
        default: return Vector3f::back();
    }
}

// DragCubeList.cs:786
void DragCubeList::SetOcclusionMultiplier(float multiplier) {
    occlusionMultiplier_ = multiplier;
    ForceUpdate(false, true);
}

// DragCubeList.cs:792
DragCube* DragCubeList::GetCube(std::string_view name) {
    int count = static_cast<int>(cubes_.size());
    do {
        if (count-- <= 0) return nullptr;
    } while (!(cubes_[static_cast<size_t>(count)].Name() == name));
    return &cubes_[static_cast<size_t>(count)];
}

// DragCubeList.cs:806
void DragCubeList::SetCubeWeight(std::string_view name, float weight) {
    bool flag = false;
    int count = static_cast<int>(cubes_.size());
    while (count-- > 0) {
        DragCube& c = cubes_[static_cast<size_t>(count)];
        if (c.Name() == name && c.Weight() != weight) {
            c.SetWeight(weight);
            flag = true;
        }
    }
    if (flag) ForceUpdate(true, true);
}

// DragCubeList.cs:824
void DragCubeList::SetDragVectorRotation(const Quaternionf& rotation) {
    rotateDragVector_ = true;
    dragVectorRotation_ = rotation;
}

// DragCubeList.cs:830
void DragCubeList::SetDragVectorRotation(bool rotateDragVector) { rotateDragVector_ = rotateDragVector; }

// DragCubeList.cs:835
void DragCubeList::ResetCubeWeights(float newWeight) {
    bool flag = false;
    int count = static_cast<int>(cubes_.size());
    while (count-- > 0) {
        DragCube& c = cubes_[static_cast<size_t>(count)];
        if (c.Weight() != newWeight) {
            c.SetWeight(newWeight);
            flag = true;
        }
    }
    if (flag) ForceUpdate(true, true);
}

// DragCubeList.cs:939
bool DragCubeList::LoadCube(const DragCubeList& cubes, std::string_view whichOne) {
    bool result = false;
    const bool flag = procedural_;
    ClearCubes();
    if (cubes.cubes_.empty()) {
        procedural_ = flag;
        return false;
    }
    for (const DragCube& c : cubes.cubes_) {
        if (c.Name() == whichOne) {
            cubes_.push_back(c);
            result = true;
        }
    }
    procedural_ = flag;
    return result;
}

// DragCubeList.cs:967
void DragCubeList::ForceUpdate(bool weights, bool occlusion, bool resetProcTiming) {
    if (resetProcTiming) proceduralDragNextUpdate_ = -1.0f;
    if (weights) {
        weightsRequireUpdate_ = true;
        occlusionRequireUpdate_ = true;
    } else if (occlusion) {
        occlusionRequireUpdate_ = true;
    }
    if (!occlusionRequireUpdate_) return;
    if (part_ == nullptr) throw NullReferenceException();
    int count = static_cast<int>(part_->attachNodes.size());
    while (count-- > 0) {
        Part* attached = part_->attachNodes[static_cast<size_t>(count)].attachedPart;
        if (attached != nullptr && attached->fields.dragModel == 4 /* CUBE */) attached->DragCubes.occlusionRequireUpdate_ = true;
    }
}

// ---- explicit instantiations: MonoFloat (the game) and NetFloat (the .NET golden harness) ----
#define KSP_DRAGCUBELIST_INSTANTIATE(P)                                                                                   \
    template void DragCubeList::SetDragWeights<P>();                                                                      \
    template void DragCubeList::SetPartOcclusion<P>();                                                                    \
    template void DragCubeList::SetDrag<P>(const Vector3f&, float);                                                       \
    template void DragCubeList::AddSurfaceDragDirection<P>(const Vector3f&, float, CubeData&) const;                      \
    template float DragCubeList::GetCubeAreaDir<P>(const Vector3f&, const std::array<float, 6>&) const;                   \
    template float DragCubeList::GetCubeCoeffDir<P>(const Vector3f&) const;                                               \
    template float DragCubeList::GetCubeDepthDir<P>(const Vector3f&) const;                                               \
    template float aero::DragCurveValue<P>(const PhysicsGlobals::SurfaceCurvesList&, float, float);
KSP_DRAGCUBELIST_INSTANTIATE(MonoFloat)
KSP_DRAGCUBELIST_INSTANTIATE(NetFloat)
#undef KSP_DRAGCUBELIST_INSTANTIATE

}  // namespace ksp
