// WP-12d: PQS_KSPBinder.cs (see ksp/terrain/pqs_ksp_binder.hpp).
#include "ksp/terrain/pqs_ksp_binder.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/physics/vessel_ranges.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace {

using GE = events::GameEvents;

CelestialBody* BodyOfSphere(const std::function<CelestialBody*(PQS*)>& hook, PQS* pqs) {
    if (hook) return hook(pqs);
    for (CelestialBody* b : FlightGlobals::Bodies())
        if (b != nullptr && b->pqsController == pqs) return b;
    return nullptr;
}

// PQS_KSPBinder.cs:71 / :78 (the same body for both handlers)
double LoadRangeOf(CelestialBody* cBForPQS) {
    const VesselRanges vesselRanges = PhysicsGlobals::Instance() != nullptr ? VesselRanges(PhysicsGlobals::Instance()->VesselRangesDefault) : VesselRanges();
    return (cBForPQS == nullptr || !cBForPQS->atmosphere) ? static_cast<double>(vesselRanges.subOrbital.load)
                                                          : static_cast<double>(vesselRanges.flying.load);
}

}  // namespace

PQS_KSPBinder::~PQS_KSPBinder() { OnDestroy(); }

// PQS_KSPBinder.cs:8
void PQS_KSPBinder::Awake() {
    pqsBindings = &PQS::GameBindings();
    pqsBindings->GetSettingsReady += Delegate<bool()>::Bind(this, &PQS_KSPBinder::pqsBindings_GetSettingsReady);
    pqsBindings->GetPlanetForceShaderModel20 += Delegate<bool()>::Bind(this, &PQS_KSPBinder::pqsBindings_GetPlanetForceShaderModel20);
    pqsBindings->GetUsePlanetScatter += Delegate<bool()>::Bind(this, &PQS_KSPBinder::pqsBindings_GetUsePlanetScatter);
    pqsBindings->GetPlanetScatterFactor += Delegate<float()>::Bind(this, &PQS_KSPBinder::pqsBindings_GetPlanetScatterFactor);
    pqsBindings->GetLoadedSceneIsGame += Delegate<bool()>::Bind(this, &PQS_KSPBinder::pqsBindings_GetLoadedSceneIsGame);
    pqsBindings->GetPresetListCompatible += Delegate<bool(const std::string&)>::Bind(this, &PQS_KSPBinder::pqsBindings_GetPresetListCompatible);
    pqsBindings->OnPQSCityLoaded += Delegate<void(PQSSurfaceObject*)>::Bind(this, &PQS_KSPBinder::OnPQSCityLoaded);
    pqsBindings->OnPQSCityUnloaded += Delegate<void(PQSSurfaceObject*)>::Bind(this, &PQS_KSPBinder::OnPQSCityUnloaded);
    pqsBindings->OnGetPQSCityLoadRange += Delegate<double(PQSSurfaceObject*)>::Bind(this, &PQS_KSPBinder::GetPQSCityLoadRange);
    // never removed (OnDestroy): a closure that keeps working after the binder is gone, as the C# delegate on a destroyed MonoBehaviour does
    pqsBindings->OnGetPOIRange += Delegate<double(PQSSurfaceObject*)>::Lambda(this, [hook = getCBForPQS](PQSSurfaceObject* so) {
        return LoadRangeOf(so != nullptr ? BodyOfSphere(hook, so->sphere) : nullptr);
    });
}

// PQS_KSPBinder.cs:23
bool PQS_KSPBinder::pqsBindings_GetPresetListCompatible(const std::string& versionstring) {
    return KSPUtil::CheckVersion(versionstring, PQSCache::lastCompatibleMajor, PQSCache::lastCompatibleMinor, PQSCache::lastCompatibleRev) ==
           VersionCompareResult::COMPATIBLE;
}

// PQS_KSPBinder.cs:28
bool PQS_KSPBinder::pqsBindings_GetLoadedSceneIsGame() { return HighLogic::LoadedSceneIsGame(); }

// PQS_KSPBinder.cs:33
float PQS_KSPBinder::pqsBindings_GetPlanetScatterFactor() { return GameSettings::PLANET_SCATTER_FACTOR; }

// PQS_KSPBinder.cs:38
bool PQS_KSPBinder::pqsBindings_GetUsePlanetScatter() { return GameSettings::PLANET_SCATTER; }

// PQS_KSPBinder.cs:43
bool PQS_KSPBinder::pqsBindings_GetPlanetForceShaderModel20() { return GameSettings::UNSUPPORTED_LEGACY_SHADER_TERRAIN; }

// PQS_KSPBinder.cs:48
bool PQS_KSPBinder::pqsBindings_GetSettingsReady() { return GameSettings::Ready(); }

// PQS_KSPBinder.cs:53
void PQS_KSPBinder::OnPQSCityLoaded(PQSSurfaceObject* city) {
    CelestialBody* cBForPQS = GetCBForPQS(city->sphere);
    if (cBForPQS != nullptr) GE::OnPQSCityLoaded.Fire(cBForPQS, city->name);
}

// PQS_KSPBinder.cs:62
void PQS_KSPBinder::OnPQSCityUnloaded(PQSSurfaceObject* city) {
    CelestialBody* cBForPQS = GetCBForPQS(city->sphere);
    if (cBForPQS != nullptr) GE::OnPQSCityUnloaded.Fire(cBForPQS, city->name);
}

// PQS_KSPBinder.cs:71
double PQS_KSPBinder::GetPQSCityLoadRange(PQSSurfaceObject* city) { return LoadRangeOf(GetCBForPQS(city->sphere)); }

// PQS_KSPBinder.cs:78
double PQS_KSPBinder::OnGetPOIRange(PQSSurfaceObject* surfaceObject) { return LoadRangeOf(GetCBForPQS(surfaceObject->sphere)); }

// PQS_KSPBinder.cs:85
CelestialBody* PQS_KSPBinder::GetCBForPQS(PQS* pqs) { return BodyOfSphere(getCBForPQS, pqs); }

// PQS_KSPBinder.cs:90 (sic: OnGetPOIRange stays subscribed)
void PQS_KSPBinder::OnDestroy() {
    if (pqsBindings != nullptr) {
        pqsBindings->GetSettingsReady -= Delegate<bool()>::Bind(this, &PQS_KSPBinder::pqsBindings_GetSettingsReady);
        pqsBindings->GetPlanetForceShaderModel20 -= Delegate<bool()>::Bind(this, &PQS_KSPBinder::pqsBindings_GetPlanetForceShaderModel20);
        pqsBindings->GetUsePlanetScatter -= Delegate<bool()>::Bind(this, &PQS_KSPBinder::pqsBindings_GetUsePlanetScatter);
        pqsBindings->GetPlanetScatterFactor -= Delegate<float()>::Bind(this, &PQS_KSPBinder::pqsBindings_GetPlanetScatterFactor);
        pqsBindings->GetLoadedSceneIsGame -= Delegate<bool()>::Bind(this, &PQS_KSPBinder::pqsBindings_GetLoadedSceneIsGame);
        pqsBindings->GetPresetListCompatible -= Delegate<bool(const std::string&)>::Bind(this, &PQS_KSPBinder::pqsBindings_GetPresetListCompatible);
        pqsBindings->OnPQSCityLoaded -= Delegate<void(PQSSurfaceObject*)>::Bind(this, &PQS_KSPBinder::OnPQSCityLoaded);
        pqsBindings->OnPQSCityUnloaded -= Delegate<void(PQSSurfaceObject*)>::Bind(this, &PQS_KSPBinder::OnPQSCityUnloaded);
        pqsBindings->OnGetPQSCityLoadRange -= Delegate<double(PQSSurfaceObject*)>::Bind(this, &PQS_KSPBinder::GetPQSCityLoadRange);
    }
    pqsBindings = nullptr;   // (kspcore: the component is gone; a second OnDestroy is a no-op)
}

}  // namespace ksp
