// FlightGlobals (FlightGlobals.cs, 2003 lines; WP-19 "flight scene registry"): only the statics WP-08's OrbitDriver
// reads (getMainBody for CheckDominantBody, overrideOrbit). WP-19 replaces this with the full port; the body list is
// the one CelestialSystem::Spawn builds (PSystemManager.PostSetup fills FlightGlobals.fetch.bodies).
#pragma once

#include <vector>

#include "ksp/math/vector3d.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class CelestialBody;
class Orbit;

// The members of Vessel that TimeWarp.cs reads through FlightGlobals.ActiveVessel / FlightGlobals.VesselsLoaded
// (WP-10). The Vessel package (WP-14/WP-19) implements it; KSP's `activeVessel != null` is `!= nullptr`.
class IFlightVessel {
public:
    virtual ~IFlightVessel() = default;
    virtual bool LandedOrSplashed() const = 0;           // Vessel.LandedOrSplashed
    virtual CelestialBody* mainBody() const = 0;          // Vessel.mainBody
    virtual Vector3f TransformPosition() const = 0;       // vessel.transform.position (Unity float)
    virtual Orbit* orbit() const = 0;                     // Vessel.orbit
    virtual double altitude() const = 0;                  // Vessel.altitude
    virtual double geeForce() const = 0;                  // Vessel.geeForce
    virtual Vector3d srf_velocity() const = 0;            // Vessel.srf_velocity
    virtual bool PatchedConicsAttached() const = 0;       // Vessel.PatchedConicsAttached
    virtual bool isEVA() const = 0;                       // Vessel.isEVA
    virtual bool OnALadder() const = 0;                   // vessel.GetComponent<KerbalEVA>().OnALadder (EVA vessels)
};

struct FlightGlobals {
    // FlightGlobals.fetch.bodies; registered by CelestialSystem::Spawn (cleared when that system is destroyed).
    // nullptr = no FlightGlobals instance (getMainBody then throws NullReferenceException, like KSP).
    static inline const std::vector<CelestialBody*>* bodies = nullptr;
    static inline bool overrideOrbit = false;  // FlightGlobals.cs:102

    // WP-10 additions (TimeWarp.cs): the flight-scene statics TimeWarp reads. WP-19 replaces them with the real fields.
    static inline bool ready = false;                             // FlightGlobals.ready
    static inline IFlightVessel* ActiveVessel = nullptr;          // FlightGlobals.ActiveVessel
    static inline std::vector<IFlightVessel*> VesselsLoaded;      // FlightGlobals.VesselsLoaded
    static inline CelestialBody* currentMainBody = nullptr;       // FlightGlobals.cs:68

    static Vector3d ship_srfVelocity();                                                  // FlightGlobals.cs:196
    static double getAltitudeAtPos(const Vector3d& position, CelestialBody* body);       // FlightGlobals.cs:1471
    static float getAltitudeAtPos(const Vector3f& position, CelestialBody* body);        // FlightGlobals.cs:1466 (float overload)
    static double getStaticPressure(double altitude, CelestialBody* body = nullptr);     // FlightGlobals.cs:1406

    static CelestialBody* getMainBody(const Vector3d& refPos);  // FlightGlobals.cs:1311

private:
    static CelestialBody* inSOI(const Vector3d& pos, CelestialBody* body);  // FlightGlobals.cs:1316
};

}  // namespace ksp
