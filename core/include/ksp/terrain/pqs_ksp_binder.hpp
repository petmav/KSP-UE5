// WP-12d: PQS_KSPBinder.cs, the component that binds the game into PQS.GameBindings (ksp/terrain/pqs.hpp PQS_GameBindings).
//
// Awake subscribes one handler to each of the ten PQS_GameBindings events (the C# `+=`: the handler is appended, and because a C#
// multicast Func returns its LAST handler's result, the binder's answer replaces the default while it is subscribed):
//   GetSettingsReady -> GameSettings.Ready, GetPlanetForceShaderModel20 -> GameSettings.UNSUPPORTED_LEGACY_SHADER_TERRAIN,
//   GetUsePlanetScatter -> GameSettings.PLANET_SCATTER, GetPlanetScatterFactor -> GameSettings.PLANET_SCATTER_FACTOR,
//   GetLoadedSceneIsGame -> HighLogic.LoadedSceneIsGame, GetPresetListCompatible -> KSPUtil.CheckVersion vs PQSCache.lastCompatible*,
//   OnPQSCityLoaded / Unloaded -> GameEvents.OnPQSCityLoaded / Unloaded(body of the city's sphere, city name),
//   OnGetPQSCityLoadRange / OnGetPOIRange -> the `load` distance of VesselRanges (PhysicsGlobals.VesselRangesDefault, or the C# defaults
//   without a PhysicsGlobals): flying.load on a body with an atmosphere, subOrbital.load otherwise (also without a body).
// OnDestroy unsubscribes nine of them. sic: OnGetPOIRange is never removed (PQS_KSPBinder.cs:90-103), so after the binder is gone the
// POI range still comes from its handler; the handler here does not use the binder object, so that stays well defined.
//
// The body of a sphere (GetCBForPQS: pqs.gameObject.GetComponentUpwards<CelestialBody>()) is the FlightGlobals body whose pqsController is
// the sphere (the PQS GameObject is a child of its body's in every stock system); `getCBForPQS` replaces the lookup.
#pragma once

#include <functional>
#include <string>

#include "ksp/terrain/pqs.hpp"

namespace ksp {

class CelestialBody;

class PQS_KSPBinder {
public:
    PQS_KSPBinder() = default;
    ~PQS_KSPBinder();   // OnDestroy (the component dies with its GameObject)
    PQS_KSPBinder(const PQS_KSPBinder&) = delete;
    PQS_KSPBinder& operator=(const PQS_KSPBinder&) = delete;

    std::function<CelestialBody*(PQS*)> getCBForPQS;   // unset: FlightGlobals.Bodies, pqsController == pqs

    void Awake();       // PQS_KSPBinder.cs:8
    void OnDestroy();   // :90
    bool Bound() const { return pqsBindings != nullptr; }

    // the handlers (protected / private in C#; public for tests)
    bool pqsBindings_GetPresetListCompatible(const std::string& versionstring);   // :23
    bool pqsBindings_GetLoadedSceneIsGame();                                      // :28
    float pqsBindings_GetPlanetScatterFactor();                                   // :33
    bool pqsBindings_GetUsePlanetScatter();                                       // :38
    bool pqsBindings_GetPlanetForceShaderModel20();                               // :43
    bool pqsBindings_GetSettingsReady();                                          // :48
    void OnPQSCityLoaded(PQSSurfaceObject* city);                                 // :53
    void OnPQSCityUnloaded(PQSSurfaceObject* city);                               // :62
    double GetPQSCityLoadRange(PQSSurfaceObject* city);                           // :71
    double OnGetPOIRange(PQSSurfaceObject* surfaceObject);                        // :78
    CelestialBody* GetCBForPQS(PQS* pqs);                                         // :85

private:
    PQS_GameBindings* pqsBindings = nullptr;   // :6
};

}  // namespace ksp
