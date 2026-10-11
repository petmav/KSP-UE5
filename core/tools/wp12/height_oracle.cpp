// WP-12a height oracle for tools/wp12/pqs_reference.py: evaluates the WP-11 terrain height function of a stock sphere
// for the queries the reference needs, using ONLY WP-11 entry points (terrain::LoadStockPQS, PQS::GetSurfaceHeight,
// PQS::Mod_OnQuadPreBuild / Mod_OnQuadBuilt). WP-11's GetSurfaceHeight is gated bit for bit by the in-game capture
// harness/captures/out2/terrain_height.json, which makes these heights game truth up to the quad state:
//   public query (vector, state): PQS.GetSurfaceHeight(vector) on a sphere whose quads were never built ("initial") or
//     after a quad build ("built": every quad-dependent mod's OnQuadBuilt ran), PQS.cs:3088;
//   build query (quad centre, subdivision, the MultiplyPoint3x4 vectors of its 225 vertices): the heights (and the
//     vertex colour the height mods leave in vbData) BuildQuad evaluates (PQS.cs:2249-2262): Mod_OnQuadPreBuild(quad),
//     then for every vertex GetSurfaceHeight(globalV) (= Mod_OnVertexBuildHeight on globalV.normalized with
//     vertHeight = radius), then Mod_OnQuadBuilt(quad).
//
//   wp12_height_oracle <queries.json> <results.json>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using nlohmann::json;

namespace {

double D(const json& h) {
    const uint64_t u = std::stoull(h.get<std::string>(), nullptr, 16);
    double d;
    std::memcpy(&d, &u, 8);
    return d;
}
std::string H(double d) {
    uint64_t u;
    std::memcpy(&u, &d, 8);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(u));
    return buf;
}
std::string HF(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%08x", u);
    return buf;
}
Vector3d V(const json& a) { return Vector3d(D(a[0]), D(a[1]), D(a[2])); }

}  // namespace

int Run(int argc, char** argv);

int main(int argc, char** argv) {
    try {
        return Run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "wp12_height_oracle: " << e.what() << std::endl;
        return 1;
    }
}

int Run(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: wp12_height_oracle <queries.json> <results.json>\n";
        return 2;
    }
    const json q = json::parse(std::ifstream(argv[1]));
    terrain::MapLibrary maps(KSPCORE_DATA_DIR "/terrain");
    json out;
    out["provenance"] = {{"tool", "core/tools/wp12/height_oracle.cpp"},
                         {"api", "terrain::LoadStockPQS, PQS::GetSurfaceHeight, PQS::Mod_OnQuadPreBuild / Mod_OnQuadBuilt (WP-11)"}};
    for (const auto& [body, bq] : q.at("bodies").items()) {
        auto initial = terrain::LoadStockPQS(body, maps);
        auto built = terrain::LoadStockPQS(body, maps);
        PQ dummy;
        built->Mod_OnQuadBuilt(dummy);
        auto builder = terrain::LoadStockPQS(body, maps);
        json res;
        res["public"] = json::array();
        for (const auto& e : bq.at("public")) {
            PQS& p = e[1].get<std::string>() == "built" ? *built : *initial;
            res["public"].push_back(H(p.GetSurfaceHeight(V(e[0]))));
        }
        res["build"] = json::array();
        for (const auto& e : bq.at("build")) {
            PQ quad;
            quad.positionPlanetRelative = V(e.at("centre"));
            quad.subdivision = e.at("sub").get<int>();
            builder->Mod_OnQuadPreBuild(quad);
            json hs = json::array(), cs = json::array();
            for (const auto& g : e.at("gv")) {
                hs.push_back(H(builder->GetSurfaceHeight(V(g))));
                const Color c = builder->vbData().vertColor;
                cs.push_back({HF(c.r), HF(c.g), HF(c.b), HF(c.a)});
            }
            builder->Mod_OnQuadBuilt(quad);
            res["build"].push_back({{"h", hs}, {"c", cs}});
        }
        out["bodies"][body] = res;
        std::cerr << body << ": " << bq.at("public").size() << " public, " << bq.at("build").size() << " build queries\n";
    }
    std::ofstream(argv[2]) << out.dump();
    return 0;
}
