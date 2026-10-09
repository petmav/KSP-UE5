// WP-11: stock PQS factory glue, MapLibrary and the CelestialBody terrain helpers (see ksp/terrain/pqs_prefab.hpp).
#include "ksp/terrain/pqs_prefab.hpp"

#include <stdexcept>
#include <utility>

#include "ksp/celestial/planetarium.hpp"

namespace ksp::terrain {

MapLibrary::MapLibrary(std::string terrainDataDir) : dir_(std::move(terrainDataDir)) {}

void MapLibrary::Add(const std::string& key, std::shared_ptr<const MapSO> map) { cache_[key] = std::move(map); }

std::shared_ptr<const MapSO> MapLibrary::Get(const std::string& key) {
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second;
    for (const StockMapInfo& m : StockMaps()) {
        if (key != m.key) continue;
        MapSO::Origin origin{m.file, m.pathID, m.objectName, m.key};
        auto map = std::make_shared<const MapSO>(
            MapSO::LoadRaw(dir_ + "/maps/" + key + ".bin", m.mapName, m.width, m.height, m.bpp, std::move(origin)));
        if (static_cast<int64_t>(map->Size()) != m.bytes) throw std::runtime_error("MapLibrary: size mismatch for " + key);
        cache_[key] = map;
        return map;
    }
    throw std::runtime_error("MapLibrary: unknown map key '" + key + "'");
}

const StockPQSInfo* FindStockPQS(const std::string& bodyName) {
    for (const StockPQSInfo& b : StockPQSBodies())
        if (bodyName == b.bodyName) return &b;
    return nullptr;
}

std::unique_ptr<PQS> CreateStockPQS(const std::string& bodyName, MapLibrary& maps, const StockPQSOptions& options) {
    const StockPQSInfo* b = FindStockPQS(bodyName);
    if (!b) throw std::invalid_argument("no stock PQS for body '" + bodyName + "'");
    return b->factory(maps, options);
}

std::unique_ptr<PQS> LoadStockPQS(const std::string& bodyName, MapLibrary& maps, const StockPQSOptions& options) {
    auto pqs = CreateStockPQS(bodyName, maps, options);
    pqs->SetupForHeight();
    return pqs;
}

// CelestialBody.cs:656 (lat *= Math.PI / 180.0)
Vector3d GetRelSurfaceNVector(double latDeg, double lonDeg) {
    const double lat = latDeg * (3.141592653589793 / 180.0);
    const double lon = lonDeg * (3.141592653589793 / 180.0);
    return Planetarium::SphericalVector(lat, lon).xzy();
}

// CelestialBody.cs:831
double TerrainAltitude(PQS* pqsController, double latDeg, double lonDeg, bool allowNegative) {
    if (pqsController == nullptr) return 0.0;
    const Vector3d relSurfaceNVector = GetRelSurfaceNVector(latDeg, lonDeg);
    double num = pqsController->GetSurfaceHeight(relSurfaceNVector) - pqsController->radius;
    if (!allowNegative && num < 0.0) num = 0.0;
    return num;
}

// Vessel.cs:2717
double SurfaceHeightOverride(PQS& pqs, double latDeg, double lonDeg, double bodyRadius) {
    return pqs.GetSurfaceHeight(GetRelSurfaceNVector(latDeg, lonDeg), true) - bodyRadius;
}

}  // namespace ksp::terrain
