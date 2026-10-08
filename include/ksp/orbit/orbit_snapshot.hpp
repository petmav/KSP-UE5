// OrbitSnapshot (Assembly-CSharp/OrbitSnapshot.cs): the ORBIT block of save files (SMA, ECC, INC, LPE, LAN, MNA, EPH,
// REF). KSP resolves REF through FlightGlobals.Bodies; here the caller passes that list (index = flightGlobalsIndex).
#pragma once

#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/orbit/orbit.hpp"

namespace ksp {

class OrbitSnapshot {
public:
    double semiMajorAxis = 0.0;
    double eccentricity = 0.0;
    double inclination = 0.0;
    double argOfPeriapsis = 0.0;
    double LAN = 0.0;
    double meanAnomalyAtEpoch = 0.0;
    double epoch = 0.0;
    int ReferenceBodyIndex = 0;

    // OrbitSnapshot.cs:19 (REF = FlightGlobals.Bodies.IndexOf(orbit.referenceBody), -1 when absent)
    OrbitSnapshot(const Orbit& orbitRef, const std::vector<CelestialBodyRef*>& bodies);
    // OrbitSnapshot.cs:31 (a low circular orbit at 1.25 radii, REF = bodyRef.flightGlobalsIndex)
    explicit OrbitSnapshot(const CelestialBodyRef& bodyRef);
    // OrbitSnapshot.cs:43 (unknown keys ignored; double.Parse / int.Parse throw dotnet::FormatException)
    explicit OrbitSnapshot(const ConfigNode& node);
    virtual ~OrbitSnapshot() = default;

    // OrbitSnapshot.cs:77 (doubles as "G17", REF as an int)
    virtual void Save(ConfigNode& node) const;
    // OrbitSnapshot.cs:89 (FlightGlobals.Bodies[REF]; std::out_of_range for a bad index, like C#)
    Orbit Load(const std::vector<CelestialBodyRef*>& bodies) const;
};

}  // namespace ksp
