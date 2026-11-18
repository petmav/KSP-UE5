// WP-UE-02: GameProcess, the once-per-process half of the game host (include/ksp/host/game_host.hpp). Host glue with no C# counterpart:
// it stands for KSP's start-up scene (the LOADING scene: GameDatabase.Load, PartLoader, Localizer, PhysicsGlobals) done once before any
// game scene exists. The calls it makes are the same ones tests/vessel_physics_fixture.hpp World and tests/wp19c_fixture.hpp World made.
#include <filesystem>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <algorithm>
#include <fstream>

#include "nlohmann/json.hpp"

#include "ksp/database/game_data.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_model_rig.hpp"
#include "ksp/host/game_host.hpp"
#include "host_internal.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp::host {

namespace {

struct ProcessState {
    GameConfig config;
    bool initialised = false;
    bool databaseLoaded = false;
    std::unique_ptr<GameData> data;
    std::unique_ptr<terrain::MapLibrary> maps;
    std::vector<std::string> modelNames{""};  // id 0 = unknown
    std::map<std::string, uint32_t> modelIds;
    std::map<uint32_t, GameProcess::PartRig> rigs;
    std::map<std::string, std::string> textureFiles;  // url -> file path ("" = none)
    bool reducedRefRead = false;
    nlohmann::json reducedRef;  // <reduced_model_dir>/mucollider_reference.json, null when there is none
};

// preorder ids of a model's nodes, as kspcore_model numbers them (node 0 = the root, a node's children follow it in order)
void PreorderIds(const mu::Node& n, std::map<const mu::Node*, int>& ids) {
    const int id = static_cast<int>(ids.size());
    ids[&n] = id;
    for (const mu::Node& c : n.children) PreorderIds(c, ids);
}

// PartLoader.CompileModel's hierarchy of the prefab (the inputs of MuPartColliders::Shapes), with every rig node tied to its .mu node
GameProcess::PartRig BuildRig(const Part& prefab) {
    GameProcess::PartRig out;
    std::vector<std::shared_ptr<const mu::Model>> models;  // alive while the rig is built
    std::vector<mu::ModelInstance> instances;
    for (const PartModelRef& mr : prefab.models) {
        if (!mr.file) continue;
        std::shared_ptr<const mu::Model> m;
        const std::string file = ProcessModelFile(mr.file->fullPath());
        if (file.empty()) continue;  // no model for this node
        try {
            m = std::make_shared<const mu::Model>(mu::ReadModelFile(file));
        } catch (const std::exception&) {
            continue;
        }
        mu::ModelInstance mi;
        mi.model = m.get();
        mi.fromModelNode = mr.fromModelNode;
        mi.parent = mr.parent;
        if (mr.name.has_value()) mi.name = *mr.name;
        mi.scale = mr.scale;
        mi.position = mr.position;
        mi.rotationEuler = mr.rotation;
        instances.push_back(std::move(mi));
        models.push_back(std::move(m));
        GameProcess::PartRigFile f;
        f.path = file;
        const size_t slash = mr.url.find_last_of('/');
        f.parentUrl = slash == std::string::npos ? std::string() : mr.url.substr(0, slash);
        for (const PartModelRef::TextureReplacement& t : mr.textureReplacements) f.textureReplacements.emplace_back(t.original, t.url);
        out.files.push_back(std::move(f));
    }
    const mu::PartModelRig rig = mu::PartModelRig::Build(prefab.fields.rescaleFactor, instances);
    if (!rig.Valid()) {
        out.files.clear();
        return out;
    }
    // The rig appends its instances in order, each as one preorder block that starts at the instance's .mu root. A part without MODEL
    // nodes uses one instance only (PartLoader.cs:1914-1927): the files the rig does not use stay listed and no node refers to them.
    std::vector<std::map<const mu::Node*, int>> ids(models.size());
    for (size_t k = 0; k < models.size(); ++k) PreorderIds(models[k]->root, ids[k]);
    std::vector<int> fileOfNode(rig.NodeCount(), -1);
    {
        size_t next = 0;
        int current = -1;
        for (size_t i = 0; i < rig.NodeCount(); ++i) {
            const mu::Node* src = rig.At(static_cast<int>(i)).src;
            if (src == nullptr) continue;
            for (size_t k = next; k < models.size(); ++k) {
                if (src == &models[k]->root) {
                    current = static_cast<int>(k);
                    next = k + 1;
                    break;
                }
            }
            fileOfNode[i] = current;
        }
    }
    out.nodes.reserve(rig.NodeCount());
    for (size_t i = 0; i < rig.NodeCount(); ++i) {
        const mu::RigNode& r = rig.At(static_cast<int>(i));
        GameProcess::PartRigNode n;
        n.name = r.name;
        n.parent = r.parent;
        n.position[0] = r.lp.x, n.position[1] = r.lp.y, n.position[2] = r.lp.z;
        n.rotation[0] = r.lr.x, n.rotation[1] = r.lr.y, n.rotation[2] = r.lr.z, n.rotation[3] = r.lr.w;
        n.scale[0] = r.ls.x, n.scale[1] = r.ls.y, n.scale[2] = r.ls.z;
        n.file = fileOfNode[i];
        if (n.file >= 0) n.fileNode = ids[static_cast<size_t>(n.file)].at(r.src);
        out.nodes.push_back(std::move(n));
    }
    return out;
}

ProcessState& State() {
    static ProcessState s;
    return s;
}

}  // namespace

// host glue (no C#): the process-level objects the sims share
GameData& ProcessGameData() { return *State().data; }
// The reduced .mu files of the stock engines (WP-mucol): origin <reduced_model_dir>/mucollider_reference.json, which holds the sha256
// of every original.
std::string ProcessModelFile(const std::string& path) {
    namespace fs = std::filesystem;
    ProcessState& s = State();
    std::error_code ec;
    if (fs::exists(path, ec) && fs::file_size(path, ec) > 0) return path;
    if (s.config.reducedModelDir.empty()) return std::string();
    if (!s.reducedRefRead) {
        s.reducedRefRead = true;
        std::ifstream f(s.config.reducedModelDir + "/mucollider_reference.json", std::ios::binary);
        if (f.good()) s.reducedRef = nlohmann::json::parse(f);
    }
    if (s.reducedRef.is_null()) return std::string();
    std::string p = path;
    std::replace(p.begin(), p.end(), '\\', '/');
    for (auto it = s.reducedRef.at("parts").begin(); it != s.reducedRef.at("parts").end(); ++it) {
        const std::string origin = it.value().at("mu").at("origin").get<std::string>();
        if (p.size() >= origin.size() && p.compare(p.size() - origin.size(), origin.size(), origin) == 0)
            return s.config.reducedModelDir + "/" + it.value().at("reduced").at("file").get<std::string>();
    }
    return std::string();
}

terrain::MapLibrary& ProcessMaps() {
    ProcessState& s = State();
    if (!s.maps) s.maps = std::make_unique<terrain::MapLibrary>(s.config.dataDir + "/terrain");
    return *s.maps;
}

bool GameProcess::Init(const GameConfig& config, std::string& error) {
    ProcessState& s = State();
    if (s.initialised) {
        error = "kspcore_game_init: already initialised";
        return false;
    }
    namespace fs = std::filesystem;
    std::error_code ec;
    if (config.gameRoot.empty() || !fs::exists(fs::path(config.gameRoot) / "GameData", ec)) {
        error = "kspcore_game_init: game_root has no GameData folder: " + config.gameRoot;
        return false;
    }
    if (config.dataDir.empty() || !fs::exists(fs::path(config.dataDir) / "terrain", ec)) {
        error = "kspcore_game_init: data_dir has no terrain folder: " + config.dataDir;
        return false;
    }
    if (config.savesDir.empty()) {
        error = "kspcore_game_init: saves_dir is empty";
        return false;
    }
    fs::create_directories(fs::path(config.savesDir), ec);
    s.config = config;
    if (s.config.physicsCfg.empty()) s.config.physicsCfg = (fs::path(config.gameRoot) / "Physics.cfg").generic_string();
    s.initialised = true;
    return true;
}

void GameProcess::Shutdown() {
    ProcessState& s = State();
    s.maps.reset();
    s.data.reset();
    s.databaseLoaded = false;
    s.initialised = false;
    s.modelNames.assign(1, "");
    s.modelIds.clear();
    s.rigs.clear();
    s.textureFiles.clear();
    s.reducedRefRead = false;
    s.reducedRef = nlohmann::json();
}

bool GameProcess::Initialised() { return State().initialised; }
const GameConfig& GameProcess::Config() { return State().config; }
bool GameProcess::DatabaseLoaded() { return State().databaseLoaded; }

bool GameProcess::LoadDatabase(const ProgressFn& progress, std::string& error) {
    ProcessState& s = State();
    if (!s.initialised) {
        error = "kspcore_game_load_database: kspcore_game_init has not run";
        return false;
    }
    if (s.databaseLoaded) return true;
    auto report = [&](float f, const char* stage) {
        if (progress) progress(f, stage);
    };
    report(0.0f, "GameDatabase");
    // KSPUtil.ApplicationRootPath = Application.dataPath + "/../" (KSPUtil.cs:795): the saves folder's parent is the root, so
    // <saves_dir> = <root>/saves (GamePersistence reads ApplicationRootPath + "saves/")
    {
        namespace fs = std::filesystem;
        const fs::path saves = fs::path(s.config.savesDir).lexically_normal();
        const fs::path root = saves.has_filename() ? saves.parent_path() : saves.parent_path().parent_path();
        fs::create_directories(root / "data");
        KSPUtil::ApplicationDataPath = (root / "data").generic_string();
    }
    s.data = std::make_unique<GameData>();
    GameData::Options o;
    o.gameRoot = s.config.gameRoot;
    s.data->Load(o);  // GameDatabase.Load, PhysicsGlobals / Physics.cfg, resources, PhysicMaterials, PartLoader.CompileAll
    report(0.9f, "PartLoader");
    for (const auto& up : s.data->parts.loadedParts) {
        const AvailablePart* p = up.get();
        if (p == nullptr) continue;
        if (s.modelIds.find(p->name) == s.modelIds.end()) {
            s.modelIds[p->name] = static_cast<uint32_t>(s.modelNames.size());
            s.modelNames.push_back(p->name);
        }
    }
    s.databaseLoaded = true;
    report(1.0f, "done");
    return true;
}

int GameProcess::PartCount() { return State().data ? static_cast<int>(State().data->parts.loadedParts.size()) : 0; }
int GameProcess::ResourceCount() { return State().data ? static_cast<int>(std::distance(State().data->resources.resourceDefinitions.begin(), State().data->resources.resourceDefinitions.end())) : 0; }

uint32_t GameProcess::ModelId(const std::string& partName) {
    ProcessState& s = State();
    const auto it = s.modelIds.find(partName);
    if (it != s.modelIds.end()) return it->second;
    s.modelIds[partName] = static_cast<uint32_t>(s.modelNames.size());
    s.modelNames.push_back(partName);
    return s.modelIds[partName];
}

const char* GameProcess::ModelName(uint32_t id) {
    ProcessState& s = State();
    return id < s.modelNames.size() ? s.modelNames[id].c_str() : "";
}

const GameProcess::PartRig& GameProcess::Rig(uint32_t modelId) {
    ProcessState& s = State();
    static const PartRig none;
    if (!s.databaseLoaded) return none;  // nothing is cached before the parts exist
    const auto it = s.rigs.find(modelId);
    if (it != s.rigs.end()) return it->second;
    PartRig rig;
    if (modelId < s.modelNames.size()) {
        const AvailablePart* ap = s.data->parts.getPartInfoByName(s.modelNames[modelId]);
        if (ap != nullptr && ap->partPrefab) rig = BuildRig(*ap->partPrefab);
    }
    return s.rigs.emplace(modelId, std::move(rig)).first->second;
}

const char* GameProcess::TextureFile(const std::string& url) {
    ProcessState& s = State();
    if (!s.data) return nullptr;
    auto it = s.textureFiles.find(url);
    if (it == s.textureFiles.end()) {
        const io::UrlFile* f = s.data->database.GetTextureFile(&url);
        it = s.textureFiles.emplace(url, f != nullptr ? f->fullPath() : std::string()).first;
    }
    return it->second.empty() ? nullptr : it->second.c_str();
}

}  // namespace ksp::host
