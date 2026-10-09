#include "ksp/database/game_database.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/localization/localizer.hpp"

namespace ksp {

namespace {

GameDatabase* g_instance = nullptr;

// GameDatabase.cs:60 flagSwaps (a Dictionary: exact, case-sensitive keys)
const std::pair<const char*, const char*> kFlagSwaps[] = {
    {"Squad/Flags/esa_dark_blue", "Squad/FlagsAgency/esa_dark_blue"},
    {"Squad/Flags/NASA", "Squad/FlagsAgency/NASA"},
    {"Squad/Flags/uk_space_agency", "Squad/FlagsAgency/uk_space_agency"},
    {"Squad/Flags/B612_Foundation_flag", "Squad/FlagsOrganization/B612_Foundation_flag"},
    {"Squad/Flags/electron", "Squad/FlagsOrganization/electron"},
    {"Squad/Flags/ESA_Arianespace", "Squad/FlagsOrganization/ESA_Arianespace"},
    {"Squad/Flags/rocketlab", "Squad/FlagsOrganization/rocketlab"},
};

const char* FlagSwap(std::string_view url) {
    for (const auto& [k, v] : kFlagSwaps)
        if (url == k) return v;
    return nullptr;
}

bool EndsWith(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

std::string FileNameWithoutExtension(const io::UrlFile& f) { return f.name(); }

uint32_t Le32(const unsigned char* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

constexpr uint32_t FourCC(const char (&s)[5]) {
    return static_cast<uint32_t>(static_cast<unsigned char>(s[0])) |
           (static_cast<uint32_t>(static_cast<unsigned char>(s[1])) << 8) |
           (static_cast<uint32_t>(static_cast<unsigned char>(s[2])) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(s[3])) << 24);
}

}  // namespace

// ---------------------------------------------------------------------------------------------------------------------
namespace texture_meta {

// DatabaseLoaderTexture_DDS.cs:29 Read (header part) and DDSHeaders/DDSHeader.cs layout
DdsInfo ReadDdsHeader(const std::string& path) {
    DdsInfo info;
    std::ifstream f(dotnet::PathFromUtf8(path), std::ios::binary);
    unsigned char h[128] = {};
    if (!f.read(reinterpret_cast<char*>(h), sizeof(h))) return info;
    if (Le32(h) != FourCC("DDS ")) return info;  // "DDS: File is not a DDS format file!" -> null
    info.ok = true;
    info.height = Le32(h + 12);
    info.width = Le32(h + 16);
    info.pfFlags = Le32(h + 80);
    info.fourCC = Le32(h + 84);
    info.caps = Le32(h + 108);
    info.isNormalMap = (info.pfFlags & 0x80000u) != 0 || (info.pfFlags & 0x80000000u) != 0;
    info.loads = info.fourCC == FourCC("DXT1") || info.fourCC == FourCC("DXT5");
    return info;
}

// MBMReader.cs:25 ReadTexture2D header (BinaryReader.ReadString: 7-bit encoded length prefix)
MbmInfo ReadMbmHeader(const std::string& path) {
    MbmInfo info;
    std::ifstream f(dotnet::PathFromUtf8(path), std::ios::binary);
    if (!f) return info;
    uint32_t len = 0;
    int shift = 0;
    for (;;) {
        const int c = f.get();
        if (c < 0 || shift > 28) return info;
        len |= static_cast<uint32_t>(c & 0x7F) << shift;
        if (!(c & 0x80)) break;
        shift += 7;
    }
    f.seekg(len, std::ios::cur);
    unsigned char b[16];
    if (!f.read(reinterpret_cast<char*>(b), sizeof(b))) return info;
    info.ok = true;
    info.width = static_cast<int32_t>(Le32(b));
    info.height = static_cast<int32_t>(Le32(b + 4));
    info.textureType = static_cast<int32_t>(Le32(b + 8));
    info.bitDepth = static_cast<int32_t>(Le32(b + 12));
    info.isNormalMap = info.textureType == 1;  // PartToolsLib/TextureType.cs: Texture, NormalMap
    return info;
}

}  // namespace texture_meta

// ---------------------------------------------------------------------------------------------------------------------
GameDatabase::GameDatabase() { g_instance = this; }  // GameDatabase.cs:892 Awake

GameDatabase::~GameDatabase() {
    if (g_instance == this) g_instance = nullptr;  // GameDatabase.cs:907 OnDestroy
}

GameDatabase* GameDatabase::Instance() { return g_instance; }

std::vector<io::ConfigDirectory> GameDatabase::DefaultUrlConfig() {
    return {io::ConfigDirectory{"", "GameData", io::DirectoryType::GameData}};
}

// GameDatabase.cs:992 SetupMainLoaders (+ the [DatabaseLoaderAttrib] extension lists of DatabaseLoaderAudio,
// DatabaseLoaderTexture_{DDS,JPEG,MBM,PNG,TGA,TRUECOLOR}, DatabaseLoaderModel_{MU,DAE})
std::vector<io::ConfigFileType> GameDatabase::MainLoaderFileTypes() {
    return {
        {io::FileType::Assembly, {"dll"}},
        {io::FileType::Audio, {"wav", "ogg"}},
        {io::FileType::Texture, {"dds", "jpg", "jpeg", "mbm", "png", "tga", "truecolor"}},
        {io::FileType::Model, {"mu", "dae", "DAE"}},
    };
}

void GameDatabase::Load(const LoadOptions& options) {
    options_ = options;
    databaseAudio.clear();
    databaseAudioFiles.clear();
    databaseTexture.clear();
    databaseModel.clear();
    databaseModelFiles.clear();
    loadLog.clear();
    LoadObjects();
    // GameDatabase.cs:949 "Compiling Configs:"
    if (options_.compileConfigs) {
        for (io::UrlConfig* c : root_->AllConfigs()) {
            loadLog.push_back("Config(" + c->type() + ") " + c->url());
            CompileConfig(c->config());
        }
    }
}

// GameDatabase.cs:1096 LoadObjects (assemblies, add-ons and the progress/frame-time logic are not data)
void GameDatabase::LoadObjects() {
    std::string rootPath = options_.gameRoot;
    if (!rootPath.empty() && rootPath.back() != '/' && rootPath.back() != '\\') rootPath += "\\";
    root_ = std::make_unique<io::UrlDir>(options_.urlConfig, MainLoaderFileTypes(), rootPath);
    gameDataPath_ = io::UrlDir::CreateApplicationPath(rootPath, "GameData");

    // Localizer.Init (Localizer.cs:111) -> SwitchToLanguage -> AddTagValuesForLanguage(GetConfigNodes("Localization"))
    if (options_.translate) {
        using KSP::Localization::Localizer;
        Localizer::Shutdown();
        Localizer& loc = Localizer::Init();
        loc.SetCurrentLanguage(options_.language);
        const std::vector<ConfigNode*> nodes = GetConfigNodes("Localization");
        loc.AddTagValuesForLanguage(nodes, options_.language);
        // GameDatabase.cs:1087 translateLoadedNodes
        for (io::UrlConfig* c : root_->AllConfigs()) Localizer::TranslateBranch(c->config());
    }

    // GameDatabase.cs:1234 audio
    for (io::UrlFile* file : root_->GetFiles(io::FileType::Audio)) {
        const std::string url = file->url();
        if (ExistsAudioClip(url)) {
            // file.fileTime > lastLoadTime (DateTime(1,1,1)) is true on the first load
            loadLog.push_back("Load(Audio): " + url + " OUT OF DATE");
            RemoveAudioClip(url);
        }
        loadLog.push_back("Load(Audio): " + url);
        // DatabaseLoaderAudio {wav, ogg}: assumed to load (the clip itself is not decoded)
        databaseAudio.push_back({url, file});
        databaseAudioFiles.push_back(file);
    }
    // GameDatabase.cs:1271 textures
    for (io::UrlFile* file : root_->GetFiles(io::FileType::Texture)) {
        const std::string url = file->url();
        if (ExistsTexture(&url)) {
            loadLog.push_back("Load(Texture): " + url + " OUT OF DATE");
            RemoveTexture(url);
        }
        loadLog.push_back("Load(Texture): " + url);
        const std::string& ext = file->fileExtension();
        TextureInfo ti;
        ti.name = url;
        ti.file = file;
        bool ok = true;
        const bool nrm = EndsWith(FileNameWithoutExtension(*file), "NRM");
        if (ext == "dds") {
            const texture_meta::DdsInfo d = texture_meta::ReadDdsHeader(file->fullPath());
            ok = d.ok && d.loads;
            ti.loader = "DDS";
            ti.isNormalMap = d.isNormalMap;
            ti.isReadable = false;
            ti.isCompressed = true;
        } else if (ext == "jpg" || ext == "jpeg") {
            ti.loader = "JPEG";
            ti.isNormalMap = nrm;
            ti.isReadable = !nrm;
            ti.isCompressed = true;
        } else if (ext == "mbm") {
            const texture_meta::MbmInfo m = texture_meta::ReadMbmHeader(file->fullPath());
            ok = m.ok;
            ti.loader = "MBM";
            ti.isNormalMap = m.isNormalMap;
            ti.isReadable = true;
            ti.isCompressed = true;
        } else if (ext == "png") {
            ti.loader = "PNG";
            ti.isNormalMap = nrm;
            ti.isReadable = !nrm;
            ti.isCompressed = true;
        } else if (ext == "tga") {
            ti.loader = "TGA";
            ti.isNormalMap = nrm;
            ti.isReadable = !nrm;
            ti.isCompressed = true;
        } else if (ext == "truecolor") {
            ti.loader = "TRUECOLOR";
            ti.isNormalMap = nrm;
            ti.isReadable = false;
            ti.isCompressed = !nrm ? false : true;
        }
        if (!ok) {
            log::Warning("Texture load error in '" + file->fullPath() + "'");
            continue;
        }
        databaseTexture.push_back(std::move(ti));
    }
    // GameDatabase.cs:1317 models
    for (io::UrlFile* file : root_->GetFiles(io::FileType::Model)) {
        const std::string url = file->url();
        if (ExistsModel(url)) {
            loadLog.push_back("Load(Model): " + url + " OUT OF DATE");
            RemoveModel(url);
        }
        loadLog.push_back("Load(Model): " + url);
        // DatabaseLoaderModel_MU (PartReader.Read) / _DAE: assumed to load (the model is not decoded here)
        databaseModel.push_back({url, file});
        databaseModelFiles.push_back(file);
    }
}

// ---- configs ----------------------------------------------------------------------------------------------------------
bool GameDatabase::ExistsConfigNode(std::string_view url) const { return root_->GetConfig(url) != nullptr; }

ConfigNode::NodePtr GameDatabase::GetConfigNode(std::string_view url) const {
    io::UrlConfig* c = root_->GetConfig(url);
    return c ? c->config().CreateCopy() : nullptr;
}

std::vector<ConfigNode*> GameDatabase::GetConfigNodes(std::string_view typeName) const {
    std::vector<ConfigNode*> out;
    for (io::UrlConfig* c : root_->GetConfigs(typeName)) out.push_back(&c->config());
    return out;
}

std::vector<ConfigNode*> GameDatabase::GetConfigNodes(std::string_view baseUrl, std::string_view typeName) const {
    std::vector<ConfigNode*> out;
    io::UrlDir* dir = root_->GetDirectory(baseUrl);
    if (!dir) return out;
    for (io::UrlConfig* c : dir->GetConfigs(typeName)) out.push_back(&c->config());
    return out;
}

// GameDatabase.cs:841
ConfigNode::NodePtr GameDatabase::GetMergedConfigNodes(std::string_view nodeName, bool mergeChildren) const {
    ConfigNode::NodePtr configNode = ConfigNode::Create(nodeName);
    for (io::UrlConfig* urlConfig : root_->GetConfigs(nodeName)) {
        for (ConfigNode* node2 : urlConfig->config().nodes().GetNodes()) {
            ConfigNode* node3 = (!configNode->HasNode(node2->name) || !mergeChildren) ? configNode->AddNode(node2->name)
                                                                                     : configNode->GetNode(node2->name);
            for (const auto& v : node2->values()) node3->AddValue(v.name, v.value);
        }
    }
    return configNode;
}

std::vector<io::UrlConfig*> GameDatabase::GetConfigs(std::string_view typeName) const {
    return root_->GetConfigs(typeName);
}

// GameDatabase.cs:1509 CompileConfigRecursive
void GameDatabase::CompileConfig(ConfigNode& node) {
    if (node.HasValue("config")) {
        ConfigNode configNode(node.name);
        const std::vector<std::string> values = node.GetValues("config");
        node.RemoveValues(std::string_view("config"));
        for (const std::string& text : values) {
            log::Info("CloneURL: System.String[]");
            GameDatabase* db = Instance();
            ConfigNode::NodePtr configNode2 = db ? db->GetConfigNode(text) : nullptr;
            if (!configNode2) {
                log::Error("ConfigNode: Cannot compile from config '" + text + "' as it does not exist");
                continue;
            }
            CompileConfig(*configNode2);
            ConfigNode::Merge(configNode, *configNode2);
            for (ConfigNode* n2 : configNode2->nodes().GetNodes()) CompileConfig(*n2);
        }
        ConfigNode::Merge(node, configNode);
    }
    for (int j = 0; j < node.nodes().Count(); j++) CompileConfig(*node.nodes().Shared(j));
}

// ---- textures ---------------------------------------------------------------------------------------------------------
bool GameDatabase::ResolveTexture(const GameDatabase& db, const std::string* url, size_t& index) {
    std::string u = *url;
    for (int guard = 0; guard < 8; guard++) {
        for (size_t i = 0; i < db.databaseTexture.size(); i++)
            if (dotnet::EqualsOrdinalIgnoreCase(db.databaseTexture[i].name, u)) {
                index = i;
                return true;
            }
        const char* swap = FlagSwap(u);
        if (!swap) return false;
        u = swap;
    }
    return false;
}

// GameDatabase.cs:348
bool GameDatabase::ExistsTexture(const std::string* url) const {
    if (!url) return false;
    size_t i;
    return ResolveTexture(*this, url, i);
}

// GameDatabase.cs:377
io::UrlFile* GameDatabase::GetTextureFile(const std::string* url) const {
    if (!url) return nullptr;
    size_t i;
    return ResolveTexture(*this, url, i) ? databaseTexture[i].file : nullptr;
}

// GameDatabase.cs:407
const GameDatabase::TextureInfo* GameDatabase::GetTextureInfo(const std::string* url) const {
    if (!url) return nullptr;
    size_t i;
    return ResolveTexture(*this, url, i) ? &databaseTexture[i] : nullptr;
}

// GameDatabase.cs:437
const GameDatabase::TextureInfo* GameDatabase::GetTextureInfoIn(const std::string* url, const std::string* textureName) const {
    if (!url || !textureName) return nullptr;
    const size_t slash = url->rfind('/');
    const std::string text = (slash == std::string::npos ? std::string() : url->substr(0, slash + 1)) + *textureName;
    return GetTextureInfo(&text);
}

// GameDatabase.cs:537
bool GameDatabase::RemoveTexture(std::string_view url) {
    for (size_t i = 0; i < databaseTexture.size(); i++)
        if (dotnet::EqualsOrdinalIgnoreCase(databaseTexture[i].name, url)) {
            databaseTexture.erase(databaseTexture.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    return false;
}

// GameDatabase.cs:582
std::vector<const GameDatabase::TextureInfo*> GameDatabase::GetAllTexturesInFolderType(std::string_view folderName,
                                                                                       bool caseInsensitive) const {
    std::vector<const TextureInfo*> list;
    for (const TextureInfo& t : databaseTexture) {
        const size_t s1 = t.name.rfind('/');
        if (s1 == std::string::npos) throw std::out_of_range("Substring length -1");
        std::string text = t.name.substr(0, s1);
        const size_t s2 = text.rfind('/');
        if (s2 != std::string::npos) text = text.substr(s2 + 1);
        if (text == folderName || (caseInsensitive && dotnet::EqualsOrdinalIgnoreCase(text, folderName)))
            list.push_back(&t);
    }
    return list;
}

// GameDatabase.cs:602
std::vector<const GameDatabase::TextureInfo*> GameDatabase::GetAllTexturesInFolder(std::string_view folderURL) const {
    std::vector<const TextureInfo*> list;
    for (const TextureInfo& t : databaseTexture) {
        const size_t s = t.name.rfind('/');
        const std::string prefix = s == std::string::npos ? std::string() : t.name.substr(0, s + 1);
        if (prefix == folderURL) list.push_back(&t);
    }
    return list;
}

// ---- models ----------------------------------------------------------------------------------------------------------
bool GameDatabase::ExistsModel(std::string_view url) const { return GetModelPrefab(url) != nullptr; }

const GameDatabase::ModelInfo* GameDatabase::GetModelPrefab(std::string_view url) const {
    for (const ModelInfo& m : databaseModel)
        if (m.name == url) return &m;
    return nullptr;
}

io::UrlFile* GameDatabase::GetModelFile(std::string_view url) const {
    for (size_t i = 0; i < databaseModel.size(); i++)
        if (databaseModel[i].name == url) return i < databaseModelFiles.size() ? databaseModelFiles[i] : nullptr;
    return nullptr;
}

io::UrlFile* GameDatabase::GetModelFile(const ModelInfo* modelPrefab) const {
    for (size_t i = 0; i < databaseModel.size(); i++)
        if (&databaseModel[i] == modelPrefab) return i < databaseModelFiles.size() ? databaseModelFiles[i] : nullptr;
    return nullptr;
}

// GameDatabase.cs:739
const GameDatabase::ModelInfo* GameDatabase::GetModelPrefabIn(std::string_view url) const {
    for (const ModelInfo& m : databaseModel) {
        const size_t s = m.name.rfind('/');
        if (s == std::string::npos) throw std::out_of_range("Length cannot be less than zero.");
        if (std::string_view(m.name).substr(0, s) == url) return &m;
    }
    return nullptr;
}

bool GameDatabase::RemoveModel(std::string_view url) {
    for (size_t i = 0; i < databaseModel.size(); i++)
        if (databaseModel[i].name == url) {
            databaseModel.erase(databaseModel.begin() + static_cast<std::ptrdiff_t>(i));
            if (i < databaseModelFiles.size())
                databaseModelFiles.erase(databaseModelFiles.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    return false;
}

// ---- audio ------------------------------------------------------------------------------------------------------------
bool GameDatabase::ExistsAudioClip(std::string_view url) const { return GetAudioClip(url) != nullptr; }

const GameDatabase::AudioInfo* GameDatabase::GetAudioClip(std::string_view url) const {
    for (const AudioInfo& a : databaseAudio)
        if (a.name == url) return &a;
    return nullptr;
}

io::UrlFile* GameDatabase::GetAudioFile(std::string_view url) const {
    for (size_t i = 0; i < databaseAudio.size(); i++)
        if (databaseAudio[i].name == url) return i < databaseAudioFiles.size() ? databaseAudioFiles[i] : nullptr;
    return nullptr;
}

// GameDatabase.cs:325 (databaseAudioFiles is not touched: sic)
bool GameDatabase::RemoveAudioClip(std::string_view url) {
    for (size_t i = 0; i < databaseAudio.size(); i++)
        if (databaseAudio[i].name == url) {
            databaseAudio.erase(databaseAudio.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    return false;
}

// GameDatabase.cs:1462
std::string GameDatabase::ToString() const {
    std::string text = "GameDatabase\r\n\r\n";
    for (io::UrlFile* f : root_->GetFiles(io::FileType::Audio)) text += "Audio: " + f->url() + "\r\n";
    for (io::UrlFile* f : root_->GetFiles(io::FileType::Texture)) text += "Texture: " + f->url() + "\r\n";
    for (io::UrlFile* f : root_->GetFiles(io::FileType::Model)) text += "Model: " + f->url() + "\r\n";
    for (const char* t : {"RESOURCE_DEFINITION", "PROP", "INTERNAL", "PART", "PHYSICMATERIAL_DEFINITION",
                          "FIREWORKFX_DEFINITION"})
        for (io::UrlConfig* c : root_->GetConfigs(t)) text += c->type() + " " + c->url() + "\r\n";
    return text;
}

}  // namespace ksp
