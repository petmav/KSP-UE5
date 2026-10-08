// OrbitSnapshot (Assembly-CSharp/OrbitSnapshot.cs); see include/ksp/orbit/orbit_snapshot.hpp.
#include "ksp/orbit/orbit_snapshot.hpp"

#include <algorithm>

#include "ksp/dotnet/number.hpp"

namespace ksp {

// OrbitSnapshot.cs:19
OrbitSnapshot::OrbitSnapshot(const Orbit& orbitRef, const std::vector<CelestialBodyRef*>& bodies) {
    semiMajorAxis = orbitRef.semiMajorAxis;
    eccentricity = orbitRef.eccentricity;
    inclination = orbitRef.inclination;
    argOfPeriapsis = orbitRef.argumentOfPeriapsis;
    LAN = orbitRef.LAN;
    meanAnomalyAtEpoch = orbitRef.meanAnomalyAtEpoch;
    epoch = orbitRef.epoch;
    const auto it = std::find(bodies.begin(), bodies.end(), orbitRef.referenceBody);
    ReferenceBodyIndex = it == bodies.end() ? -1 : static_cast<int>(it - bodies.begin());
}

// OrbitSnapshot.cs:31
OrbitSnapshot::OrbitSnapshot(const CelestialBodyRef& bodyRef) {
    eccentricity = 0.0;
    semiMajorAxis = bodyRef.Radius + bodyRef.Radius * 0.25;
    inclination = 0.0001;
    LAN = 0.0;
    argOfPeriapsis = 0.0;
    meanAnomalyAtEpoch = 0.0;
    epoch = 0.0;
    ReferenceBodyIndex = bodyRef.flightGlobalsIndex;
}

// OrbitSnapshot.cs:43
OrbitSnapshot::OrbitSnapshot(const ConfigNode& node) {
    for (const ConfigNode::Value& value : node.values()) {
        const std::string& name = value.name;
        if (name == "MNA") meanAnomalyAtEpoch = dotnet::ParseDouble(value.value);
        else if (name == "ECC") eccentricity = dotnet::ParseDouble(value.value);
        else if (name == "REF") ReferenceBodyIndex = dotnet::ParseInt32(value.value);
        else if (name == "SMA") semiMajorAxis = dotnet::ParseDouble(value.value);
        else if (name == "LPE") argOfPeriapsis = dotnet::ParseDouble(value.value);
        else if (name == "LAN") LAN = dotnet::ParseDouble(value.value);
        else if (name == "EPH") epoch = dotnet::ParseDouble(value.value);
        else if (name == "INC") inclination = dotnet::ParseDouble(value.value);
    }
}

// OrbitSnapshot.cs:77
void OrbitSnapshot::Save(ConfigNode& node) const {
    node.AddValue("SMA", semiMajorAxis);
    node.AddValue("ECC", eccentricity);
    node.AddValue("INC", inclination);
    node.AddValue("LPE", argOfPeriapsis);
    node.AddValue("LAN", LAN);
    node.AddValue("MNA", meanAnomalyAtEpoch);
    node.AddValue("EPH", epoch);
    node.AddValue("REF", static_cast<int32_t>(ReferenceBodyIndex));
}

// OrbitSnapshot.cs:89
Orbit OrbitSnapshot::Load(const std::vector<CelestialBodyRef*>& bodies) const {
    CelestialBodyRef* body = bodies.at(static_cast<size_t>(ReferenceBodyIndex < 0 ? bodies.size() : static_cast<size_t>(ReferenceBodyIndex)));
    return Orbit(inclination, eccentricity, semiMajorAxis, LAN, argOfPeriapsis, meanAnomalyAtEpoch, epoch, body);
}

}  // namespace ksp
