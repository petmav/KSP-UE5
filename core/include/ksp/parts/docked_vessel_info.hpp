// DockedVesselInfo.cs (107 lines): what a docking port remembers of the vessel it docked (Part.Undock reads it). Assigned to WP-25
// (docking) by the package list, ported here because Part.Undock (WP-16) takes it; WP-25 only has to use it.
#pragma once

#include <cstdint>
#include <string>

#include "ksp/config/config_node.hpp"
#include "ksp/save/save_enums.hpp"

namespace ksp {

class DockedVesselInfo {
public:
    // DockedVesselInfo.cs:6 (the pre-1.2 numeric VesselType values a save may still hold)
    enum class OldVesselType : int {
        Debris, SpaceObject, Unknown, Probe, Rover, Lander, Ship, Station, Base, EVA, Flag, Plane, Relay, DeployedScienceController,
        DeployedSciencePart
    };

    std::string name;                           // :25
    uint32_t rootPartUId = 0;                   // :27
    VesselType vesselType = VesselType::Debris;  // :29

    void Load(const ConfigNode& node);  // :31
    void Save(ConfigNode& node) const;  // :96
};

}  // namespace ksp
