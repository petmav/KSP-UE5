// terrain_height (WP-11): PQS.GetSurfaceHeight for Kerbin and Mun with the stock mod stacks built from the game's
// prefab data, judged against the in-game capture harness/captures/out2/terrain_height.json (batch 2, "Terrain").
//
// Inputs mirror the capture code (decompiled/KSP-Decompiled-profiling/Assembly-CSharp/PhysProf/HarnessCapture2.cs:871
// TerrainCapture and :1020 Sample): per sample GetSurfaceHeight(GetRelSurfaceNVector(lat, lon)) (h_surface), then
// the overrideQuadBuildCheck variant (h_override), terrain_alt = h_surface - pqs.radius.
//   * global_1deg: integer lat -90..90 x lon -180..179 (both bodies);
//   * ksc_0.01deg (Kerbin): lat = -0.6 + i * 0.01, lon = -75.06 + j * 0.01, i, j = 0..100 (double arithmetic);
//   * active_vessel (Kerbin): the vessel's lat/lon on the pad at capture time (provenance active_vessel_lat/lon).
// Scene state: the capture ran in FLIGHT with Kerbin's PQS active (quads built: PQSMod_FlattenArea/MapDecal* have
// seen OnQuadBuilt, quadActive = true) and the Mun's PQS never built (quadActive = false, so the public
// GetSurfaceHeight skips its FlattenArea mods). kspcore reproduces that by calling Mod_OnQuadBuilt on Kerbin only.
// scene_probe's MAINMENU rows use freshly set-up spheres (no quads, as before any game is loaded).
#include <cstdio>
#include <memory>
#include <string>

#include "datasets.hpp"
#include "ksp/terrain/pqs_prefab.hpp"

#ifndef KSPCORE_DATA_DIR
#error "KSPCORE_DATA_DIR must point to core/data"
#endif

namespace {

using ksp::PQS;
using namespace ksp::terrain;

// provenance of harness/captures/out2/terrain_height.json: the active vessel's position (an input of the last sample)
constexpr double kActiveVesselLat = -0.09720779031430374;
constexpr double kActiveVesselLon = -74.55767983607595;

void Sample(golden::Table& t, const std::string& body, PQS& pqs, const char* set, double lat, double lon) {
    const ksp::Vector3d rad = GetRelSurfaceNVector(lat, lon);
    const double h = pqs.GetSurfaceHeight(rad);
    const double ho = pqs.GetSurfaceHeight(rad, true);
    t.Add({body, set, lat, lon, h, ho, h - pqs.radius});
}

int RunTerrainHeight(const RunnerOptions& opt) {
    MapLibrary maps(std::string(KSPCORE_DATA_DIR) + "/terrain");
    golden::File g;
    g.dataset = "terrain_height";
    g.description = "kspcore candidate: PQS.GetSurfaceHeight with the stock PQSMod stacks (prefab data + original height maps).";

    golden::Table tb("bodies", "Per body PQS state after SetupMods/SetupBuildDelegates.");
    tb.Col("body", golden::ColType::Str).Col("Radius", golden::ColType::F64).Col("pqs_radius", golden::ColType::F64);
    tb.Col("radiusMin", golden::ColType::F64).Col("radiusMax", golden::ColType::F64).Col("ocean", golden::ColType::Bool);
    tb.Col("pqs_name", golden::ColType::Str);
    tb.key = {"body"};

    golden::Table tp("scene_probe", "Heights per scene state (MAINMENU: no quads built; FLIGHT: Kerbin quads built).");
    tp.Col("scene", golden::ColType::Str).Col("body", golden::ColType::Str).Col("h_ksc", golden::ColType::F64);
    tp.Col("h_override_ksc", golden::ColType::F64).Col("h_override_0_0", golden::ColType::F64);
    tp.key = {"scene", "body"};

    golden::Table ts("samples", "Height samples.");
    ts.Col("body", golden::ColType::Str).Col("set", golden::ColType::Str).Col("lat", golden::ColType::F64);
    ts.Col("lon", golden::ColType::F64).Col("h_surface", golden::ColType::F64).Col("h_override", golden::ColType::F64);
    ts.Col("terrain_alt", golden::ColType::F64);
    ts.key = {"body", "set", "lat", "lon"};

    for (const char* scene : {"MAINMENU", "FLIGHT"}) {
        for (const char* bn : {"Kerbin", "Mun"}) {
            const std::string body = bn;
            std::unique_ptr<PQS> pqs = LoadStockPQS(body, maps);
            const bool flight = std::string(scene) == "FLIGHT";
            if (flight && body == "Kerbin") pqs->Mod_OnQuadBuilt(ksp::PQ{});  // the active sphere has built quads
            // HarnessCapture2.cs:915-917
            const double ksc = pqs->GetSurfaceHeight(GetRelSurfaceNVector(-0.0972, -74.5577));
            const double ksco = pqs->GetSurfaceHeight(GetRelSurfaceNVector(-0.0972, -74.5577), true);
            const double zero = pqs->GetSurfaceHeight(GetRelSurfaceNVector(0.0, 0.0), true);
            tp.Add({scene, body, ksc, ksco, zero});
            if (!flight) continue;
            const StockPQSInfo* info = FindStockPQS(body);
            tb.Add({body, info->bodyRadius, pqs->radius, pqs->radiusMin, pqs->radiusMax, info->ocean, pqs->name});
            for (int lat = -90; lat <= 90; lat++)
                for (int lon = -180; lon < 180; lon++) Sample(ts, body, *pqs, "global_1deg", lat, lon);
            if (body == "Kerbin") {
                for (int i = 0; i <= 100; i++) {
                    const double la = -0.6 + static_cast<double>(i) * 0.01;
                    for (int j = 0; j <= 100; j++) Sample(ts, body, *pqs, "ksc_0.01deg", la, -75.06 + static_cast<double>(j) * 0.01);
                }
                Sample(ts, body, *pqs, "active_vessel", kActiveVesselLat, kActiveVesselLon);
            }
            if (opt.verbose) std::printf("terrain_height: %s mods %d radiusMin %.17g radiusMax %.17g\n", bn, pqs->modCount(), pqs->radiusMin, pqs->radiusMax);
        }
    }
    g.tables.push_back(std::move(tb));
    g.tables.push_back(std::move(tp));
    g.tables.push_back(std::move(ts));
    return WriteCandidate(opt, g, "terrain_height/terrain_height.json");
}

}  // namespace

KSP_GOLDEN_DATASET("terrain_height", 110, RunTerrainHeight);
