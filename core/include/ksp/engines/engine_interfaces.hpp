// WP-23: the small engine-side declarations of Assembly-CSharp: EngineType.cs, IEngineStatus.cs, IThrustProvider.cs,
// IThrottleControl.cs.
#pragma once

#include <string>

#include "ksp/aero/aero_interfaces.hpp"

namespace ksp {

// EngineType.cs:1 (the order is the C# enum order: Enum.Parse of the cfg `EngineType` value maps the name to the index)
enum class EngineType : int32_t { Generic, SolidBooster, LiquidFuel, Piston, Turbine, ScramJet, Electric, Nuclear, MonoProp };

// IEngineStatus.cs:1
class IEngineStatus {
public:
    virtual ~IEngineStatus() = default;
    virtual bool isOperational() const = 0;
    virtual float normalizedOutput() const = 0;
    virtual float throttleSetting() const = 0;
    virtual std::string engineName() const = 0;
};

// IThrustProvider.cs:1
class IThrustProvider {
public:
    virtual ~IThrustProvider() = default;
    virtual void OnCenterOfThrustQuery(CenterOfThrustQuery& qry) = 0;
    virtual float GetMaxThrust() = 0;
    virtual float GetCurrentThrust() = 0;
    virtual EngineType GetEngineType() = 0;
};

// IThrottleControl.cs:1 (implemented by the vessel-level throttle owners of the flight scene, not by a module of this package)
class IThrottleControl {
public:
    virtual ~IThrottleControl() = default;
    virtual void SetThrottle() = 0;
    virtual void SetThrottle(float throttle) = 0;
};

}  // namespace ksp
