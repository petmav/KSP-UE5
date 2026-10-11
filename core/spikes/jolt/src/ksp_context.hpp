// Jolt spike: the kspcore side of the flight scene: GameData (parts), PhysicsGlobals (Physics.cfg), Planetarium + the Kerbol
// system (WP-07/09), Kerbin's PQS height stack (WP-11), and craft loading through WP-18 ShipConstruct + WP-14 vessel assembly.
// Same setup as tests/test_vessel.cpp World14 (the game data the gates use: testdata/GameData-mirror).
#pragma once

#include <memory>
#include <string>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/vessel/vessel.hpp"

namespace spike {

// The LaunchPad as tests/test_vessel.cpp uses it (World14::PlaceKerbinAtPad)
constexpr double kPadLat = -0.0972077486779459;
constexpr double kPadLon = -74.5576813534722;

class KspContext {
public:
    explicit KspContext(bool loadTerrain = true);
    ~KspContext();
    KspContext(const KspContext&) = delete;
    KspContext& operator=(const KspContext&) = delete;

    // ShipConstruct.LoadShip (file side, WP-04/18) + AssembleVesselFromCraft (WP-14/18: Instantiate every prefab, link the tree,
    // Vessel.Initialize). Parts stand at the craft's pos / rot (Unity frame, +Y up, root at the origin).
    std::unique_ptr<ksp::Vessel> LoadCraft(const std::string& path);

    ksp::CelestialBody& Kerbin() { return *kerbin_; }
    ksp::PQS* KerbinPQS() { return pqs_.get(); }
    // Radial unit vector of (lat, lon) in Kerbin's body-fixed frame (CelestialBody.GetRelSurfaceNVector: Y = polar axis)
    static ksp::Vector3d SurfaceNormal(double latDeg, double lonDeg);
    // PQS.GetSurfaceHeight(dir) = radius + terrain height (m from Kerbin's centre); falls back to the sphere radius without PQS
    double SurfaceRadius(const ksp::Vector3d& dir);

private:
    ksp::GameData data_;
    ksp::PhysicsGlobals pg_;
    ksp::Planetarium planetarium_;
    std::unique_ptr<ksp::CelestialSystem> sys_;
    ksp::CelestialBody* kerbin_ = nullptr;
    std::unique_ptr<ksp::terrain::MapLibrary> maps_;
    std::unique_ptr<ksp::PQS> pqs_;
};

}  // namespace spike
