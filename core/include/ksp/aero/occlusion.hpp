// WP-21: the thermal-occlusion geometry FlightIntegrator.UpdateOcclusionConvection / Solar / Body (WP-22) sorts parts with:
// OcclusionData.cs (a part's drag-cube bounds projected along a flow / light direction), OcclusionCone.cs (the shock cone a
// part casts in the airflow), OcclusionCylinder.cs (the shadow cylinder for sun / body flux). Pure maths over the part's
// DragCubeList (WeightedCenter / WeightedSize, CrossSectionalArea, TaperDot, Depth, GetCube*Dir) and its
// partTransform.localToWorldMatrix (an input: Unity's native matrix of the live transform). Managed float code follows the
// Mono rule; Quaternion.FromToRotation is the native port (ksp/unity/quaternion_native.hpp).
//
// PartThermalData (PartThermalData.cs) is WP-22's; the two fields the geometry reads and writes are the OcclusionThermalRef
// seam below. Not ported: the GameSettings.FI_LOG_TEMP_ERROR Debug.Log lines.
#pragma once

#include <array>

#include "ksp/unity/color.hpp"  // Matrix4x4f
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;

// The PartThermalData members OcclusionData / OcclusionCone use (PartThermalData.cs: part, convectionTempMultiplier,
// convectionCoeffMultiplier)
struct OcclusionThermalRef {
    Part* part = nullptr;
    double convectionTempMultiplier = 1.0;
    double convectionCoeffMultiplier = 1.0;
};

// Matrix4x4.MultiplyPoint3x4 (managed Unity, Mono rule)
Vector3f MultiplyPoint3x4(const Matrix4x4f& m, const Vector3f& point);

class OcclusionData;

// OcclusionCylinder.cs
class OcclusionCylinder {
public:
    OcclusionData* part = nullptr;
    Vector3f center{};
    double radius = 0.0;
    Vector2f extents{};
    Vector2f offset{};
    double cylNoseDot = 0.0;
    void Setup(OcclusionData& part);  // OcclusionCylinder.cs:18
};

// OcclusionCone.cs
class OcclusionCone {
public:
    OcclusionData* part = nullptr;
    Vector3f center{};
    Vector2f extents{};
    Vector2f offset{};
    double radius = 0.0;
    double cylNoseDot = 0.0;
    double shockNoseDot = 0.0;
    double shockAngle = 0.0;
    double shockConvectionTempMult = 0.0;
    double shockConvectionCoeffMult = 0.0;
    double occludeConvectionTempMult = 0.0;
    double occludeConvectionCoeffMult = 0.0;
    double occludeConvectionAreaMult = 0.0;
    // OcclusionCone.cs:31-59 statics
    static inline double detachedShockHeatMult = 0.5;
    static inline double detachedShockCoeffMult = 1.0;
    static inline double detachedBehindShockHeatMult = 0.4;
    static inline double detachedBehindShockCoeffMult = 1.0;
    static inline double detachedShockMachAngleMult = 0.05;
    static inline double detachedShockStartAngle = 3.141592653589793 * 49.0 / 100.0;
    static inline double detachedShockEndAngle = 3.141592653589793 * 9.0 / 25.0;
    static inline double obliqueShockAngleMult = 0.8;
    static inline double obliqueShockPartAngleMult = 0.25;
    static inline double obliqueShockMinAngleMult = 1.05;
    static inline double obliqueShockConeHeatMult = 0.75;
    static inline double obliqueShockConeCoeffMult = 1.0;
    static inline double obliqueShockCylHeatMult = 0.55;
    static inline double obliqueShockCylCoeffMult = 1.0;

    void Setup(OcclusionData& part, double sqrtMach, double sqrtMachAngle, double detachAngle);  // OcclusionCone.cs:61
    double GetShockRadius(double dot) const;                                                    // OcclusionCone.cs:115
};

// OcclusionData.cs
class OcclusionData {
public:
    Part* part = nullptr;
    OcclusionThermalRef* ptd = nullptr;
    double projectedArea = 0.0;
    double projectedRadius = 0.0;
    double invFineness = 0.0;
    double minimumDot = 0.0;
    double maximumDot = 0.0;
    double centroidDot = 0.0;
    double maxWidthDepth = 0.0;
    Vector3f boundsCenter{};
    std::array<Vector3f, 8> boundsVertices{};
    Vector3f projectedCenter{};
    std::array<Vector3f, 8> projectedVertices{};
    std::array<float, 8> projectedDots{};
    Vector2f center{};
    Vector2f minExtents{};
    Vector2f maxExtents{};
    Vector2f extents{};
    OcclusionCone convCone;
    OcclusionCylinder sunCyl;
    OcclusionCylinder bodyCyl;

    explicit OcclusionData(OcclusionThermalRef& data);  // OcclusionData.cs:57
    // OcclusionData.cs:73 Update; localToWorldMatrix = part.partTransform.localToWorldMatrix (null part -> no-op)
    void Update(const Vector3f& velocity, const Matrix4x4f& localToWorldMatrix, bool useDragArea = true);
    double GetConvectionMultVerts(const OcclusionCone& cone) const;                                  // :127
    double GetShockStats(const OcclusionCone& cone, double& newTempMult, double& newCoeffMult) const;  // :152
    double GetCylinderOcclusion(const OcclusionCylinder& cyl) const;                                // :180
    double GetCylinderOcclusionVerts(const OcclusionCylinder& cyl) const;                           // :197
    int CompareTo(const OcclusionData& b) const;                                                    // :303 (by maximumDot)
    // protected statics, public for tests
    static double sA(double r, double x, double y);                                                 // :213
    static double RectRectIntersection(double centralExtentX, double centralExtentY, double minX, double maxX, double minY,
                                       double maxY);                                                // :238
    static double AreaOfIntersection(double existingConeRadius, double potentialConeRadius, double sqrDistance);  // :248

protected:
    void CreateCornerArray();  // OcclusionData.cs:62
};

}  // namespace ksp
