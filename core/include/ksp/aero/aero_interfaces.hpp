// WP-21: aerodynamic interfaces and query records (IAirstreamShield.cs, ILiftProvider.cs, IMultipleDragCube.cs,
// CenterOfLiftQuery.cs, CenterOfThrustQuery.cs) and the two data-only modifier modules (ModuleDragModifier.cs,
// ModuleDragAreaModifier.cs: KSPFields only, read by DragCubeSystem when it renders cubes; their fields live in the WP-06
// FieldStore of the generic PartModule, so they need no class of their own).
#pragma once

#include <string>
#include <vector>

#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class Vessel;

// CenterOfLiftQuery.cs
class CenterOfLiftQuery {
public:
    Vector3f refVector{};
    double refAltitude = 0.0;
    double refStaticPressure = 0.0;
    double refAirDensity = 0.0;
    Vector3f pos{};
    Vector3f dir{};
    float lift = 0.0f;
    // CenterOfLiftQuery.cs:19 Reset
    void Reset() {
        refVector = pos = dir = Vector3f::zero();
        refAirDensity = refStaticPressure = refAltitude = 0.0;
        lift = 0.0f;
    }
};

// CenterOfThrustQuery.cs
class CenterOfThrustQuery {
public:
    Vector3f pos{};
    Vector3f dir{};
    float thrust = 0.0f;
    // CenterOfThrustQuery.cs:11 Reset
    void Reset() {
        pos = dir = Vector3f::zero();
        thrust = 0.0f;
    }
};

// IAirstreamShield.cs
class IAirstreamShield {
public:
    virtual ~IAirstreamShield() = default;
    virtual bool ClosedAndLocked() = 0;
    virtual Vessel* GetVessel() = 0;
    virtual Part* GetPart() = 0;
};

// ILiftProvider.cs
class ILiftProvider {
public:
    virtual ~ILiftProvider() = default;
    virtual bool DisableBodyLift() const = 0;
    virtual bool IsLifting() const = 0;
    virtual void OnCenterOfLiftQuery(CenterOfLiftQuery& qry) = 0;
};

// IMultipleDragCube.cs
class IMultipleDragCube {
public:
    virtual ~IMultipleDragCube() = default;
    virtual bool IsMultipleCubesActive() const = 0;
    virtual std::vector<std::string> GetDragCubeNames() const = 0;
    virtual void AssumeDragCubePosition(const std::string& name) = 0;
    virtual bool UsesProceduralDragCubes() const = 0;
};

}  // namespace ksp
