// WP-21: OcclusionData.cs, OcclusionCone.cs, OcclusionCylinder.cs (see ksp/aero/occlusion.hpp).
#include "ksp/aero/occlusion.hpp"

#include <cfloat>
#include <cmath>

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/bounds.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp {

namespace {
constexpr double kPI = 3.141592653589793;
double Sq(const Vector3f& v) { return static_cast<double>(v.sqrMagnitude()); }
}  // namespace

// Matrix4x4.cs:424 MultiplyPoint3x4
Vector3f MultiplyPoint3x4(const Matrix4x4f& m, const Vector3f& p) {
    Vector3f r;
    r.x = f32store(fx(m.m00) * fx(p.x) + fx(m.m01) * fx(p.y) + fx(m.m02) * fx(p.z) + fx(m.m03));
    r.y = f32store(fx(m.m10) * fx(p.x) + fx(m.m11) * fx(p.y) + fx(m.m12) * fx(p.z) + fx(m.m13));
    r.z = f32store(fx(m.m20) * fx(p.x) + fx(m.m21) * fx(p.y) + fx(m.m22) * fx(p.z) + fx(m.m23));
    return r;
}

// OcclusionCylinder.cs:17
void OcclusionCylinder::Setup(OcclusionData& p) {
    part = &p;
    center = p.projectedCenter;
    radius = p.projectedRadius;
    offset = Vector2f(-p.center.x, -p.center.y);  // -part.center (Vector2 unary minus)
    extents = p.extents;
    cylNoseDot = p.maximumDot - p.maxWidthDepth;
}

// OcclusionCone.cs:60
void OcclusionCone::Setup(OcclusionData& p, double sqrtMach, double sqrtMachAngle, double detachAngle) {
    part = &p;
    center = p.projectedCenter;
    radius = p.projectedRadius;
    offset = Vector2f(-p.center.x, -p.center.y);
    extents = p.extents;
    if (p.ptd == nullptr) throw NullReferenceException();
    double num = p.ptd->convectionTempMultiplier;
    double num2 = p.ptd->convectionCoeffMultiplier;
    cylNoseDot = p.maximumDot - p.maxWidthDepth;
    const double num3 = dotnet::Math::Asin(p.invFineness);
    if (!(p.invFineness >= 1.0) && num3 <= detachAngle) {
        shockNoseDot = p.maximumDot;
        shockAngle = dotnet::Math::Max(num3 * obliqueShockMinAngleMult, sqrtMachAngle * obliqueShockAngleMult + num3 * obliqueShockPartAngleMult);
        occludeConvectionAreaMult = 1.0;
        if (!(num < detachedShockHeatMult - 0.05) && num2 >= detachedShockCoeffMult - 0.05) {
            occludeConvectionTempMult = obliqueShockCylHeatMult;
            shockConvectionTempMult = obliqueShockConeHeatMult;
            occludeConvectionCoeffMult = obliqueShockCylCoeffMult;
            shockConvectionCoeffMult = obliqueShockConeCoeffMult;
        } else {
            double val = dotnet::Math::Max(num, detachedBehindShockHeatMult);
            occludeConvectionTempMult = dotnet::Math::Min(val, obliqueShockCylHeatMult);
            shockConvectionTempMult = dotnet::Math::Min(val, obliqueShockConeHeatMult);
            val = dotnet::Math::Max(num2, detachedBehindShockCoeffMult);
            occludeConvectionCoeffMult = dotnet::Math::Min(val, obliqueShockCylCoeffMult);
            shockConvectionCoeffMult = dotnet::Math::Min(val, obliqueShockConeCoeffMult);
        }
    } else {
        shockNoseDot = p.maximumDot + radius * p.invFineness;
        shockAngle = UtilMath::Lerp(detachedShockEndAngle, detachedShockStartAngle, sqrtMach * detachedShockMachAngleMult);
        occludeConvectionTempMult = 0.0;
        occludeConvectionAreaMult = 0.0;
        if (!(num < detachedShockHeatMult - 0.05) && num2 >= detachedShockCoeffMult - 0.05) {
            num = detachedShockHeatMult;
            num2 = detachedShockCoeffMult;
            shockConvectionTempMult = detachedBehindShockHeatMult;
            shockConvectionCoeffMult = detachedBehindShockCoeffMult;
        } else {
            num = (shockConvectionTempMult = detachedBehindShockHeatMult);
            num2 = (shockConvectionCoeffMult = detachedBehindShockCoeffMult);
        }
    }
    p.ptd->convectionTempMultiplier = num;
    p.ptd->convectionCoeffMultiplier = num2;
}

// OcclusionCone.cs:116
double OcclusionCone::GetShockRadius(double dot) const {
    const double num = shockNoseDot - dot;
    return radius + num * dotnet::Math::Tan(shockAngle);
}

// OcclusionData.cs:48
OcclusionData::OcclusionData(OcclusionThermalRef& data) : part(data.part), ptd(&data) {}

// OcclusionData.cs:57
void OcclusionData::CreateCornerArray() {
    const Bounds bounds(part->DragCubes.WeightedCenter(), part->DragCubes.WeightedSize());
    boundsCenter = bounds.center();
    const Vector3f mn = bounds.min(), mx = bounds.max();
    boundsVertices[0] = mn;
    boundsVertices[1] = mx;
    boundsVertices[2] = Vector3f(mn.x, mn.y, mx.z);
    boundsVertices[3] = Vector3f(mn.x, mx.y, mn.z);
    boundsVertices[4] = Vector3f(mx.x, mn.y, mn.z);
    boundsVertices[5] = Vector3f(mn.x, mx.y, mx.z);
    boundsVertices[6] = Vector3f(mx.x, mn.y, mx.z);
    boundsVertices[7] = Vector3f(mx.x, mx.y, mn.z);
}

// OcclusionData.cs:71
void OcclusionData::Update(const Vector3f& velocity, const Matrix4x4f& localToWorldMatrix, bool useDragArea) {
    if (part == nullptr) return;
    CreateCornerArray();
    const Vector3f vector = MultiplyPoint3x4(localToWorldMatrix, boundsCenter);
    centroidDot = Vector3d::Dot(Vector3d(vector), Vector3d(velocity));
    projectedCenter = (Vector3d(vector) - centroidDot * Vector3d(velocity)).ToVector3f();
    const Quaternionf quaternion = UnityNative::FromToRotation(velocity, Vector3f::up());
    minimumDot = DBL_MAX;
    maximumDot = -DBL_MAX;
    minExtents = Vector2f(FLT_MAX, FLT_MAX);
    maxExtents = Vector2f(-FLT_MAX, -FLT_MAX);
    for (int i = 0; i < 8; i++) {
        const Vector3f vector2 = MultiplyPoint3x4(localToWorldMatrix, boundsVertices[static_cast<size_t>(i)]);
        const double num = Vector3d::Dot(Vector3d(vector2), Vector3d(velocity));
        projectedVertices[static_cast<size_t>(i)] = (Vector3d(vector2) - num * Vector3d(velocity)).ToVector3f();
        const Vector3f vector3 = quaternion * vector2;
        maxExtents.x = dotnet::Math::Max(maxExtents.x, vector3.x);
        maxExtents.y = dotnet::Math::Max(maxExtents.y, vector3.z);
        minExtents.x = dotnet::Math::Min(minExtents.x, vector3.x);
        minExtents.y = dotnet::Math::Min(minExtents.y, vector3.z);
        if (num < minimumDot) minimumDot = num;
        if (num > maximumDot) maximumDot = num;
        projectedDots[static_cast<size_t>(i)] = static_cast<float>(num);
    }
    extents = (maxExtents - minExtents) * 0.5f;
    center = minExtents + extents;
    if (useDragArea) {
        projectedArea = static_cast<double>(part->DragCubes.CrossSectionalArea());
        invFineness = static_cast<double>(part->DragCubes.TaperDot());
        maxWidthDepth = static_cast<double>(part->DragCubes.Depth());
    } else {
        projectedArea = static_cast<double>(part->DragCubes.GetCubeAreaDir(velocity));
        invFineness = static_cast<double>(part->DragCubes.GetCubeCoeffDir(velocity));
        maxWidthDepth = static_cast<double>(part->DragCubes.GetCubeDepthDir(velocity));
    }
    projectedRadius = dotnet::Math::Sqrt(projectedArea / kPI);
}

// OcclusionData.cs:124
double OcclusionData::GetConvectionMultVerts(const OcclusionCone& cone) const {
    double num = 0.0, num2 = 0.0;
    double radius = cone.radius;
    radius *= radius;
    for (int i = 0; i < 8; i++) {
        const double num3 = projectedDots[static_cast<size_t>(i)];
        double shockRadius = cone.GetShockRadius(num3);
        shockRadius *= shockRadius;
        const double num4 = Sq(projectedVertices[static_cast<size_t>(i)] - cone.center);
        if (num4 <= radius && num3 < cone.cylNoseDot) {
            num += 0.125;
            num2 += 0.125;
        } else if (num4 <= shockRadius && num3 < cone.shockNoseDot) {
            num2 += 0.125;
        }
    }
    return 1.0 - num2 + (num2 - num) * cone.shockConvectionTempMult + num * cone.occludeConvectionTempMult;
}

// OcclusionData.cs:149
double OcclusionData::GetShockStats(const OcclusionCone& cone, double& newTempMult, double& newCoeffMult) const {
    const Vector2f vector = cone.offset + minExtents;
    const Vector2f vector2 = cone.offset + maxExtents;
    double num = RectRectIntersection(cone.extents.x, cone.extents.y, vector.x, vector2.x, vector.y, vector2.y);
    if (std::isnan(num)) num = 0.0;
    double num2 = num;
    double num3 = 1.0;
    if (num2 < 0.99) num3 = AreaOfIntersection(cone.GetShockRadius(centroidDot), projectedRadius, Sq(projectedCenter - cone.center));
    else num2 = 1.0;
    const double num4 = 1.0 - num3;
    const double num5 = num3 - num2;
    newTempMult = num4 * newTempMult + num5 * cone.shockConvectionTempMult + num2 * cone.occludeConvectionTempMult;
    newCoeffMult = num4 * newCoeffMult + num5 * cone.shockConvectionCoeffMult + num2 * cone.occludeConvectionCoeffMult;
    return 1.0 - num2 + num2 * cone.occludeConvectionAreaMult;
}

// OcclusionData.cs:179
double OcclusionData::GetCylinderOcclusion(const OcclusionCylinder& cyl) const {
    const Vector2f vector = cyl.offset + minExtents;
    const Vector2f vector2 = cyl.offset + maxExtents;
    double num = RectRectIntersection(cyl.extents.x, cyl.extents.y, vector.x, vector2.x, vector.y, vector2.y);
    if (std::isnan(num)) num = 0.0;
    return num;
}

// OcclusionData.cs:195
double OcclusionData::GetCylinderOcclusionVerts(const OcclusionCylinder& cyl) const {
    double num = 0.0;
    double radius = cyl.radius;
    radius *= radius;
    for (int i = 0; i < 8; i++) {
        const double num2 = projectedDots[static_cast<size_t>(i)];
        if (Sq(projectedVertices[static_cast<size_t>(i)] - cyl.center) <= radius && num2 < cyl.cylNoseDot) num += 0.125;
    }
    return num;
}

// OcclusionData.cs:211
double OcclusionData::sA(double r, double x, double y) {
    if (x < 0.0) return -sA(r, -x, y);
    if (y < 0.0) return -sA(r, x, -y);
    if (x > r) x = r;
    if (y > r) y = r;
    if (x * x + y * y > r * r) {
        const double num = r * r * dotnet::Math::Asin(x / r) + x * dotnet::Math::Sqrt(r * r - x * x) + r * r * dotnet::Math::Asin(y / r) +
                           y * dotnet::Math::Sqrt(r * r - y * y) - r * r * kPI * 0.5;
        return num * 0.5;
    }
    return x * y;
}

// OcclusionData.cs:237
double OcclusionData::RectRectIntersection(double centralExtentX, double centralExtentY, double minX, double maxX, double minY,
                                           double maxY) {
    const double num = (maxX - minX) * (maxY - minY);
    if (!(maxX < -centralExtentX) && !(minX > centralExtentX) && !(maxY < -centralExtentY) && !(minY > centralExtentY) &&
        num != 0.0) {
        return dotnet::Math::Max(0.0, dotnet::Math::Min(centralExtentX, maxX) - dotnet::Math::Max(-centralExtentX, minX)) *
               dotnet::Math::Max(0.0, dotnet::Math::Min(centralExtentY, maxY) - dotnet::Math::Max(-centralExtentY, minY)) /
               ((maxX - minX) * (maxY - minY));
    }
    return 0.0;
}

// OcclusionData.cs:247
double OcclusionData::AreaOfIntersection(double existingConeRadius, double potentialConeRadius, double sqrDistance) {
    const double num = existingConeRadius + potentialConeRadius;
    if (sqrDistance >= num * num) return 0.0;
    if (potentialConeRadius == 0.0) return 1.0;
    if (existingConeRadius == 0.0) return 0.0;
    const double num2 = dotnet::Math::Sqrt(sqrDistance);
    if (existingConeRadius >= num2 + potentialConeRadius) return 1.0;
    const double num3 = potentialConeRadius * potentialConeRadius;
    if (potentialConeRadius >= num2 + existingConeRadius) return UtilMath::Clamp01(existingConeRadius * existingConeRadius / num3);
    double num4 = existingConeRadius;
    double num5 = potentialConeRadius;
    if (num5 < num4) {
        num4 = potentialConeRadius;
        num5 = existingConeRadius;
    }
    const double num6 = num4 * num4;
    const double num7 = num5 * num5;
    const double num8 = num6 * dotnet::Math::Acos((sqrDistance + num6 - num7) / (2.0 * num2 * num4));
    const double num9 = num7 * dotnet::Math::Acos((sqrDistance + num7 - num6) / (2.0 * num2 * num5));
    const double num10 = 0.5 * dotnet::Math::Sqrt((-num2 + num) * (num2 + num4 - num5) * (num2 - num4 + num5) * (num2 + num));
    if (!std::isnan(num8) && !std::isnan(num9) && !std::isnan(num10)) return (num8 + num9 - num10) / (kPI * num3);
    return 0.0;
}

// OcclusionData.cs:307
int OcclusionData::CompareTo(const OcclusionData& b) const {
    if (b.maximumDot < maximumDot) return 1;
    if (b.maximumDot == maximumDot) return 0;
    return -1;
}

}  // namespace ksp
