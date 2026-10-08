// PSystemManager.SpawnBody / PostSetup (PSystemManager.cs:124-249) and the FlightGlobals body list (FlightGlobals.cs).
#include "ksp/celestial/celestial_system.hpp"

#include <algorithm>
#include <limits>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"

namespace ksp {

CelestialSystem::~CelestialSystem() {
    // The Planetarium may outlive the system: drop its references to our bodies.
    if (Planetarium::fetch != nullptr) {
        for (const auto& b : owned_) {
            if (Planetarium::fetch->Sun == b.get()) Planetarium::fetch->Sun = nullptr;
            if (Planetarium::fetch->Home == b.get()) Planetarium::fetch->Home = nullptr;
            if (Planetarium::fetch->CurrentMainBody == b.get()) Planetarium::fetch->CurrentMainBody = nullptr;
        }
    }
}

std::unique_ptr<CelestialSystem> CelestialSystem::Spawn(const PSystemPrefab& prefab, Planetarium& planetarium,
                                                        const OrbitFactory& orbitFactory) {
    std::unique_ptr<CelestialSystem> sys(new CelestialSystem());
    sys->owned_.reserve(prefab.bodies.size());
    // PSystemManager.SetupPSystem: SpawnBody(null, systemPrefab.rootBody)
    for (size_t i = 0; i < prefab.bodies.size(); ++i)
        if (prefab.bodies[i].parent.empty()) sys->SpawnBody(nullptr, prefab, i, planetarium);
    // PostSetup (PSystemManager.cs:236): sort by flightGlobalsIndex (index -1: by name), fill FlightGlobals.bodies
    std::vector<CelestialBody*> localBodies = sys->spawnOrder_;
    std::stable_sort(localBodies.begin(), localBodies.end(), [](const CelestialBody* a, const CelestialBody* b) {
        if (a->flightGlobalsIndex == -1) return a->bodyName < b->bodyName;
        return a->flightGlobalsIndex < b->flightGlobalsIndex;
    });
    sys->maximumSurfaceGeeASL_ = std::numeric_limits<double>::lowest();  // double.MinValue
    for (CelestialBody* b : localBodies) {
        sys->bodies_.push_back(b);
        if (b->hasSolidSurface) sys->maximumSurfaceGeeASL_ = std::max(sys->maximumSurfaceGeeASL_, b->GeeASL);
    }
    // Start(): CelestialBody.Start, then OrbitDriver.Start (orbit.Init(); updateFromParameters())
    for (CelestialBody* b : sys->spawnOrder_) b->Start();
    if (orbitFactory) {
        for (CelestialBody* b : sys->spawnOrder_) {
            if (!b->orbitDriver) continue;
            std::unique_ptr<ICelestialBodyOrbit> o = orbitFactory(*b);
            b->orbitDriver->link = o.get();
            if (o) sys->orbits_.push_back(std::move(o));
        }
    } else {
        log::Warning("CelestialSystem::Spawn: no OrbitFactory; bodies have no run-time Orbit (WP-07 not attached)");
    }
    // first Planetarium.FixedUpdate: UpdateCBsRecursive(FindRootBody(Sun)) at the current UT
    planetarium.UpdateCBs();
    return sys;
}

// PSystemManager.cs:124 SpawnBody (scaled-space objects, PQS, map objects and orbit renderers are the shell's)
void CelestialSystem::SpawnBody(CelestialBody* parent, const PSystemPrefab& prefab, size_t index, Planetarium& planetarium) {
    const CelestialBodyPrefab& p = prefab.bodies[index];
    auto owned = std::make_unique<CelestialBody>(p.body);  // Object.Instantiate(body.celestialBody)
    CelestialBody* celestialBody = owned.get();
    owned_.push_back(std::move(owned));
    celestialBody->orbitDriver = p.orbitDriver;
    celestialBody->Awake();
    celestialBody->orbitingBodies.clear();
    celestialBody->flightGlobalsIndex = p.flightGlobalsIndex;
    if (parent == nullptr)
        planetarium.Sun = celestialBody;
    else
        parent->orbitingBodies.push_back(celestialBody);
    if (celestialBody->isHomeWorld) planetarium.Home = celestialBody;
    if (celestialBody->orbitDriver) celestialBody->orbitDriver->referenceBody = parent;
    if (p.planetariumCameraInitial) planetarium.CurrentMainBody = celestialBody;
    spawnOrder_.push_back(celestialBody);
    // foreach (PSystemBody child in body.children) SpawnBody(celestialBody, child): the children are the following
    // prefab entries whose parent is this body, in prefab (= PSystem children) order
    for (size_t j = index + 1; j < prefab.bodies.size(); ++j)
        if (prefab.bodies[j].parent == p.body.bodyName) SpawnBody(celestialBody, prefab, j, planetarium);
}

// FlightGlobals.cs:1813 GetHomeBody (with FlightGlobals.fetch): the first home world in Bodies, else null
CelestialBody* CelestialSystem::GetHomeBody() const {
    for (CelestialBody* b : bodies_)
        if (b->isHomeWorld) return b;
    return nullptr;
}

// FlightGlobals.cs:1752
CelestialBody* CelestialSystem::GetBodyByName(const std::string& name) const {
    for (CelestialBody* b : bodies_)
        if (b->bodyName == name) return b;
    return nullptr;
}

}  // namespace ksp
