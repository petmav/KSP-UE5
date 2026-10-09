#include "ksp/physics/vessel_ranges.hpp"

#include <string>

#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/number.hpp"

namespace ksp {

const char* ToString(VesselSituations s) {
    switch (s) {
        case VesselSituations::LANDED: return "LANDED";
        case VesselSituations::SPLASHED: return "SPLASHED";
        case VesselSituations::PRELAUNCH: return "PRELAUNCH";
        case VesselSituations::FLYING: return "FLYING";
        case VesselSituations::SUB_ORBITAL: return "SUB_ORBITAL";
        case VesselSituations::ORBITING: return "ORBITING";
        case VesselSituations::ESCAPING: return "ESCAPING";
        case VesselSituations::DOCKED: return "DOCKED";
    }
    return "";
}

// VesselRanges.cs:33 Situation.Load
void VesselRanges::Situation::Load(const ConfigNode& node) {
    int count = node.values().Count();
    while (count-- > 0) {
        const ConfigNode::Value& value = node.values()[count];
        if (value.name == "unpack") {
            unpack = dotnet::ParseSingle(value.value);
        } else if (value.name == "pack") {
            pack = dotnet::ParseSingle(value.value);
        } else if (value.name == "unload") {
            unload = dotnet::ParseSingle(value.value);
        } else if (value.name == "load") {
            load = dotnet::ParseSingle(value.value);
        }
    }
}

// VesselRanges.cs:57 Situation.Save
void VesselRanges::Situation::Save(ConfigNode& node) const {
    node.SetValue("load", load, true);
    node.SetValue("unload", unload, true);
    node.SetValue("pack", pack, true);
    node.SetValue("unpack", unpack, true);
}

// VesselRanges.cs:95
VesselRanges::Situation& VesselRanges::GetSituationRanges(VesselSituations situation) {
    switch (situation) {
        case VesselSituations::FLYING: return flying;
        case VesselSituations::LANDED: return landed;
        case VesselSituations::SPLASHED: return splashed;
        case VesselSituations::PRELAUNCH: return prelaunch;
        case VesselSituations::ESCAPING: return escaping;
        case VesselSituations::SUB_ORBITAL: return subOrbital;
        default: return orbit;
    }
}
const VesselRanges::Situation& VesselRanges::GetSituationRanges(VesselSituations situation) const {
    return const_cast<VesselRanges*>(this)->GetSituationRanges(situation);
}

// VesselRanges.cs:116
void VesselRanges::Load(const ConfigNode& node) {
    int count = node.nodes().Count();
    while (count-- > 0) {
        const ConfigNode& configNode = *node.nodes()[count];
        const std::string& n = configNode.name;
        if (n == "flying") {
            flying.Load(configNode);
        } else if (n == "escaping") {
            escaping.Load(configNode);
        } else if (n == "subOrbital") {
            subOrbital.Load(configNode);
        } else if (n == "landed") {
            landed.Load(configNode);
        } else if (n == "prelaunch") {
            prelaunch.Load(configNode);
        } else if (n == "orbit") {
            orbit.Load(configNode);
        } else if (n == "splashed") {
            splashed.Load(configNode);
        }
    }
}

// VesselRanges.cs:149
void VesselRanges::Save(ConfigNode& node) const {
    prelaunch.Save(*node.AddNode("prelaunch"));
    landed.Save(*node.AddNode("landed"));
    splashed.Save(*node.AddNode("splashed"));
    flying.Save(*node.AddNode("flying"));
    orbit.Save(*node.AddNode("orbit"));
    subOrbital.Save(*node.AddNode("subOrbital"));
    escaping.Save(*node.AddNode("escaping"));
}

}  // namespace ksp
