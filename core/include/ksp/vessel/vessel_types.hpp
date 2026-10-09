// WP-14 small vessel-side types: FlightEvents.cs, FlightEndModes.cs, VesselActuatorMode.cs, VesselTargetModes.cs, EventReport.cs,
// IVesselAutoRename.cs, VesselEngiUtil.cs, and the Vessel.State / Vessel.ControlLevel enums (Vessel.cs:12, 45).
#pragma once

#include <string>

#include "ksp/orbit/targetable.hpp"
#include "ksp/physics/physics_globals.hpp"  // VesselTargetModes
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class Vessel;
class Orbit;
class OrbitDriver;
enum class VesselType : int;

// Vessel.State (Vessel.cs:12)
enum class VesselState : int { INACTIVE, ACTIVE, DEAD };

// Vessel.ControlLevel (Vessel.cs:45)
enum class ControlLevel : int { NONE, PARTIAL_UNMANNED, PARTIAL_MANNED, FULL };

// FlightEvents.cs
enum class FlightEvents : int {
    LAUNCH, CRASH, SPLASHDOWN, SPLASHDOWN_CRASH, COLLISION, OVERHEAT, SPLASHDAMAGE, OUTOFFUEL, STAGESEPARATION, TOUCHDOWN,
    JOINTBREAK, CREW_KILLED, CREW_ON_EVA, CREW_BOARD_VESSEL, REENTRY_BURNUP, UNDOCK, OVERPRESSURE, OVERG, CUSTOM
};

// FlightEndModes.cs
enum class FlightEndModes : int { CATASTROPHIC_FAILURE, SUCCESSFUL_FAILURE, SUCCESS, ABORTED };

// VesselActuatorMode.cs
enum class VesselActuatorMode : int { Normal, SAS, Pilot };

// VesselTargetModes.cs
// VesselTargetModes.cs: defined with PhysicsGlobals (physics_globals.hpp)

// EventReport.cs:1 (the payload of GameEvents.onJointBreak / onCrash / ...; `origin` is a Part)
class EventReport {
public:
    FlightEvents eventType;
    Part* origin;
    std::string sender;
    std::string other;
    int stage;
    std::string msg;
    float param;

    // EventReport.cs:15
    EventReport(FlightEvents type, Part* eventCreator, std::string name = "an unidentified object",
                std::string otherName = "an unidentified object", int stageNumber = 0, std::string customMsg = "",
                float param_ = 0.0f)
        : eventType(type), origin(eventCreator), sender(std::move(name)), other(std::move(otherName)), stage(stageNumber),
          msg(std::move(customMsg)), param(param_) {}
};

// IVesselAutoRename.cs (a PartModule that renames the vessel it is on)
class IVesselAutoRename {
public:
    virtual ~IVesselAutoRename() = default;
    virtual VesselType GetVesselType() = 0;
    virtual std::string GetVesselName() = 0;
};

// ITargetable.cs is ksp/orbit/targetable.hpp (WP-10; GetTransform() hands out the ksp/unity/transform.hpp stand-in).

// VesselEngiUtil.cs:3 (extension method, returns 0)
inline double VesselEngiUtil_GetDeltaV(const Vessel&) { return 0.0; }

}  // namespace ksp
