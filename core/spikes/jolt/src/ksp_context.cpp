#include "ksp_context.hpp"

#include <cstdint>
#include <cstdlib>
#include <stdexcept>

#include "ksp/config/config_node.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/vessel/vessel_assembly.hpp"

namespace spike {

namespace {
std::string GameRoot() {
    const char* e = std::getenv("KSP_GAMEDATA_ROOT");
    if (e && *e) return e;
    return KSPCORE_DEFAULT_GAME_ROOT;
}
}  // namespace

KspContext::KspContext(bool loadTerrain) {
    using namespace ksp;
    SaveEnvironment::Reset();
    FlightRegistry::Clear();
    GameData::Options o;
    o.gameRoot = GameRoot();
    data_.Load(o);
    SaveEnvironment& env = SaveEnvironment::Current();
    env.getPartInfoByName = [this](std::string_view n) { return data_.parts.getPartInfoByName(n); };
    env.partExists = [this](std::string_view n) { return data_.parts.getPartInfoByName(n) != nullptr; };
    env.newGuidHashCode = [] {
        static uint32_t s = 4242u;
        s = s * 1664525u + 1013904223u;
        return static_cast<int32_t>(s);
    };
    pg_.physicsDatabaseFilename = std::string(KSPCORE_HARNESS_DIR) + "/inputs/Physics.cfg";
    pg_.Awake("");
    if (!pg_.LoadDatabase()) throw std::runtime_error("PhysicsGlobals.LoadDatabase failed");
    planetarium_.Awake();
    sys_ = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium_);
    env.localBodies = sys_->SpawnOrder();
    Vessel::env = VesselEnvironment{};
    VesselModuleManager::CompileModules();
    kerbin_ = sys_->GetBodyByName("Kerbin");
    if (kerbin_ == nullptr) throw std::runtime_error("no Kerbin");
    if (loadTerrain) {
        maps_ = std::make_unique<terrain::MapLibrary>(std::string(KSPCORE_DATA_DIR) + "/terrain");
        pqs_ = terrain::LoadStockPQS("Kerbin", *maps_);
    }
}

KspContext::~KspContext() {
    ksp::FlightRegistry::Clear();
    sys_.reset();
    ksp::SaveEnvironment::Reset();
    ksp::Vessel::env = ksp::VesselEnvironment{};
}

std::unique_ptr<ksp::Vessel> KspContext::LoadCraft(const std::string& path) {
    using namespace ksp;
    ConfigNode::NodePtr root = ConfigNode::Load(path);
    if (!root) throw std::runtime_error("cannot load craft " + path);
    ShipConstruct ship;
    std::string err;
    if (!ship.LoadShip(*root, 0, true, err)) throw std::runtime_error("LoadShip " + path + ": " + err);
    AssembleOptions ao;
    ao.rootPosition = Vector3f();
    ao.isLanded = true;
    auto v = AssembleVesselFromCraft(ship, [this](const std::string& n) { return data_.parts.getPartInfoByName(n); }, ao);
    if (!v) throw std::runtime_error("AssembleVesselFromCraft " + path);
    return v;
}

ksp::Vector3d KspContext::SurfaceNormal(double latDeg, double lonDeg) { return ksp::terrain::GetRelSurfaceNVector(latDeg, lonDeg); }

double KspContext::SurfaceRadius(const ksp::Vector3d& dir) {
    if (pqs_) return pqs_->GetSurfaceHeight(dir, true);
    return kerbin_->Radius;
}

}  // namespace spike
