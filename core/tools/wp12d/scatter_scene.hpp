// WP-12d: the stock Kerbin sphere built around the KSC launch pad as the flight scene builds it (PQS.Start with the default terrain preset,
// the flight host of PQSMod_CelestialBodyTransform, `frames` FixedUpdate + Update), shared by tools/wp12d/scatter_inputs.cpp (the inputs
// of tools/wp12d/scatter_reference.py) and tests/test_pqs_surface.cpp (the gate). The PQSLandControl scatter of this sphere runs as in
// KSP: AddScatterMeshController on the built maximum-level quads, CreateScatterMesh on their first OnQuadUpdate.
#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "ksp/save/save_enums.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/terrain/pqs_mods.hpp"
#include "ksp/terrain/pqs_mods_quad.hpp"
#include "ksp/terrain/pqs_parallel.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/simplex.hpp"

namespace wp12d {

// Building a stock sphere constructs Simplex objects (they reseed UnityEngine.Random and rewrite the static permutation tables, as in KSP)
// and the scatter reseeds UnityEngine.Random: a test case restores that process-wide state for the cases after it (as WP-12a's guard)
struct StateGuard {
    std::array<int, 256> simplexP = ksp::Simplex::StaticPermutation();
    std::array<int, 256> simplexNoiseP = ksp::SimplexNoise::StaticPermutation();
    ksp::UnityRandom::State random = ksp::UnityRandom::Global().state();
    ~StateGuard() {
        ksp::Simplex::StaticPermutation() = simplexP;
        ksp::SimplexNoise::StaticPermutation() = simplexNoiseP;
        ksp::UnityRandom::Global().SetState(random);
    }
};

// the WP-20b / WP-12a pad coordinates (LaunchPad spawn point)
constexpr double kPadLat = -0.0972, kPadLon = -74.5577;

struct ScatterScene {
    ksp::PQSCache cache;
    std::unique_ptr<ksp::PQSHeightWorkers> workers;   // outlives the sphere
    std::unique_ptr<ksp::PQS> sphere;
    ksp::PQSTarget target;
    ScatterScene(ksp::terrain::MapLibrary& maps, int frames, int workerThreads = 0, double lat = kPadLat, double lon = kPadLon) {
        using namespace ksp;
        cache.cachePQInitialCount = 64;
        sphere = terrain::CreateStockPQS("Kerbin", maps);
        for (auto& m : sphere->childMods)
            if (auto* cb = dynamic_cast<PQSMod_CelestialBodyTransform*>(m.get())) {
                cb->host.hasBody = [] { return true; };
                cb->host.flightGlobalsExists = [] { return true; };
                cb->host.activeVessel = [this] { return &target; };
                cb->host.isCurrentMainBody = [] { return true; };
                cb->host.loadedScene = [] { return static_cast<int>(GameScenes::FLIGHT); };
                cb->host.mapIsEnabled = [] { return false; };
            }
        PQSCache::CreateDefaultPresetList();
        sphere->cache = &cache;
        if (workerThreads > 0) {
            workers = std::make_unique<PQSHeightWorkers>([&maps] { return terrain::LoadStockPQS("Kerbin", maps); }, workerThreads);
            sphere->heightWorkers = workers.get();
        }
        target.position = terrain::GetRelSurfaceNVector(lat, lon).normalized() * (600000.0 + 70.0);
        sphere->SetTarget(&target);
        sphere->Start();
        for (int f = 0; f < frames; ++f) {
            sphere->FixedUpdate();
            sphere->FrameUpdate();
        }
    }
    ~ScatterScene() {
        sphere.reset();
        ksp::PQSCache::PresetList.reset();
    }
    ksp::PQSLandControl* LandControl() const { return dynamic_cast<ksp::PQSLandControl*>(sphere->landControl); }
    // the scatter quads in use (assigned to a quad), in assignment order per scatter
    std::vector<const ksp::PQSLandControl::LandClassScatterQuad*> Assigned(int scatterIndex) const {
        std::vector<const ksp::PQSLandControl::LandClassScatterQuad*> out;
        for (const auto* q : LandControl()->scatters.at(static_cast<size_t>(scatterIndex)).cacheAssigned) out.push_back(q);
        return out;
    }
};

}  // namespace wp12d
