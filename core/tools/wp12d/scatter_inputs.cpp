// WP-12d: the inputs of tools/wp12d/scatter_reference.py: the stock Kerbin sphere around two surface points (the KSC pad and a forested
// plain at lat -15, lon 100) built as the flight scene builds it (tools/wp12d/scatter_scene.hpp), and for the first quads each stock scatter
// was assigned to there: the quad as CreateScatterMesh reads it (name, vertices, positionPlanet, quadRotation, quadArea) and the density
// PQSLandControl.OnQuadBuilt handed to AddScatterMeshController, plus the sphere / scatter constants. No output of the scatter code itself
// (count, seed, placement) is written: the reference computes those.
//
//   wp12d_scatter_inputs <out.json>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "ksp/terrain/pqs_prefab.hpp"
#include "nlohmann/json.hpp"
#include "scatter_scene.hpp"

using nlohmann::json;
using namespace ksp;

namespace {
std::string HD(double d) {
    uint64_t u;
    std::memcpy(&u, &d, 8);
    char b[24];
    std::snprintf(b, sizeof b, "%016llx", static_cast<unsigned long long>(u));
    return b;
}
std::string HF(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    char b[16];
    std::snprintf(b, sizeof b, "%08x", u);
    return b;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: wp12d_scatter_inputs <out.json>\n";
        return 2;
    }
    terrain::MapLibrary maps(KSPCORE_DATA_DIR "/terrain");
    json out;
    out["provenance"] = {{"tool", "core/tools/wp12d/scatter_inputs.cpp"},
                         {"scene", "core/tools/wp12d/scatter_scene.hpp (stock Kerbin, default preset, 3 frames)"}};
    const double points[][2] = {{wp12d::kPadLat, wp12d::kPadLon}, {-15.0, 100.0}};
    for (const auto& pt : points) {
        wp12d::ScatterScene sc(maps, 3, 0, pt[0], pt[1]);
        const PQSLandControl* lc = sc.LandControl();
        const PQSMeshCache& cache = sc.sphere->meshCache();
        json scene;
        scene["lat"] = HD(pt[0]);
        scene["lon"] = HD(pt[1]);
        scene["radius"] = HD(sc.sphere->radius);
        scene["surfaceRelativeQuads"] = sc.sphere->surfaceRelativeQuads;
        scene["cacheRes"] = cache.cacheRes;
        scene["cacheSideVertCount"] = cache.cacheSideVertCount;
        scene["scatters"] = json::array();
        for (size_t s = 0; s < lc->scatters.size(); ++s) {
            const PQSLandControl::LandClassScatter& scat = lc->scatters[s];
            json js = {{"index", s}, {"scatterName", scat.scatterName}, {"seed", scat.seed}, {"maxScatter", scat.maxScatter},
                       {"densityFactor", HD(scat.densityFactor)}, {"verticalOffset", HF(scat.verticalOffset)}, {"minScale", HF(scat.minScale)},
                       {"maxScale", HF(scat.maxScale)}, {"hasBaseMesh", scat.hasBaseMesh}, {"quads", json::array()}};
            int n = 0;
            for (const auto* q : scat.cacheAssigned) {
                if (n >= 8 || q->quad == nullptr) continue;
                const PQ& quad = *q->quad;
                json jq = {{"name", quad.name}, {"quadArea", HD(quad.quadArea)}, {"density", HD(q->density)},
                           {"positionPlanet", {HD(quad.positionPlanet.x), HD(quad.positionPlanet.y), HD(quad.positionPlanet.z)}},
                           {"quadRotation", {HD(quad.quadRotation.x), HD(quad.quadRotation.y), HD(quad.quadRotation.z), HD(quad.quadRotation.w)}},
                           {"verts", json::array()}};
                for (const Vector3f& v : quad.verts) jq["verts"].push_back({HF(v.x), HF(v.y), HF(v.z)});
                js["quads"].push_back(jq);
                ++n;
            }
            scene["scatters"].push_back(js);
        }
        out["scenes"].push_back(scene);
    }
    std::ofstream(argv[1]) << out.dump();
    return 0;
}
