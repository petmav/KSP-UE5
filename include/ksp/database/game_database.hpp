// GameDatabase, data side (Assembly-CSharp/GameDatabase.cs): the GameData tree (UrlDir), the config queries, and the
// texture / model / audio tables as METADATA (url, file, loader flags). Nothing is decoded here: a texture entry says
// which file the game would have loaded under which url, a model entry which .mu file backs a model url. Decoding is
// the Unreal import pipeline's job (it reads TextureInfo::file->fullPath() / ModelInfo::file->fullPath()).
//
// Load order is KSP's (GameDatabase.cs:942 CreateDatabase / :1096 LoadObjects):
//   1. UrlDir over the configured directories (GameData, urlRoot ""), every .cfg parsed;
//   2. Localizer: the "Localization" configs of the language (default en-us) are loaded into the tag table, then every
//      config tree is translated in place (translateLoadedNodes -> Localizer.TranslateBranch);
//   3. audio, then texture, then model files, each in UrlDir.GetFiles order ("Load(Audio|Texture|Model): <url>");
//      a url seen again replaces the earlier entry at the end of the table (the first-load "OUT OF DATE" path);
//   4. asset bundles (*.ksp): NOT read (Unity asset bundles; recorded as files of type Unknown);
//   5. CompileConfig on every config ("config = <url>" clone values).
// The resource / physic-material / upgrade libraries and the part compiler run after this; ksp::GameData
// (game_data.hpp) runs the whole sequence.
//
// GameData root convention: `gameRoot` is the KSP install directory (KSP: Application.dataPath + "/../"); GameData is
// `gameRoot`/GameData and PartDatabase.cfg is `gameRoot`/PartDatabase.cfg. kspcore only ever reads below it.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/io/url_dir.hpp"

namespace ksp {

class GameDatabase {
public:
    // GameDatabase.cs:17 TextureInfo (the Texture2D is replaced by metadata)
    struct TextureInfo {
        std::string name;               // the url (GameDatabase.cs:1297 loader.obj.name = file.url)
        io::UrlFile* file = nullptr;    // null for asset-bundle textures (not read by kspcore)
        bool isNormalMap = false;       // loader rule: *NRM file names (png/jpg/tga/truecolor), DDS pixel-format flags
                                        // 0x80000 | 0x80000000, MBM texture type NormalMap
        bool isReadable = false;
        bool isCompressed = false;
        std::string loader;             // DatabaseLoaderTexture_<X> that produced it ("DDS", "PNG", ...)
    };
    // A databaseModel entry: GameObject named after the url (GameDatabase.cs:1342), backed by databaseModelFiles[i].
    struct ModelInfo {
        std::string name;
        io::UrlFile* file = nullptr;
    };
    struct AudioInfo {
        std::string name;
        io::UrlFile* file = nullptr;
    };

    struct LoadOptions {
        // KSP install directory (contains GameData/). Required.
        std::string gameRoot;
        // GameDatabase.urlConfig (serialized on the loading-scene prefab): GameData with urlRoot "".
        std::vector<io::ConfigDirectory> urlConfig = DefaultUrlConfig();
        // Localizer language (KSP reads it from buildID64.txt; en-us for this install).
        std::string language = "en-us";
        // translateLoadedNodes (GameDatabase.cs:1087)
        bool translate = true;
        // CompileConfig over AllConfigs (GameDatabase.cs:950)
        bool compileConfigs = true;
    };

    static std::vector<io::ConfigDirectory> DefaultUrlConfig();
    // GameDatabase.cs:992 SetupMainLoaders: Assembly {dll}, Audio {wav, ogg}, Texture {dds, jpg, jpeg, mbm, png, tga,
    // truecolor}, Model {mu, dae, DAE}.
    static std::vector<io::ConfigFileType> MainLoaderFileTypes();

    GameDatabase();
    ~GameDatabase();
    GameDatabase(const GameDatabase&) = delete;
    GameDatabase& operator=(const GameDatabase&) = delete;

    // GameDatabase.Instance: the most recently constructed live database (KSP's singleton).
    static GameDatabase* Instance();

    void Load(const LoadOptions& options);
    const LoadOptions& options() const { return options_; }
    io::UrlDir* root() const { return root_.get(); }
    const std::string& gameDataPath() const { return gameDataPath_; }

    // ---- configs (GameDatabase.cs:802-871) ----
    bool ExistsConfigNode(std::string_view url) const;
    ConfigNode::NodePtr GetConfigNode(std::string_view url) const;  // a deep copy (CreateCopy)
    std::vector<ConfigNode*> GetConfigNodes(std::string_view typeName) const;
    std::vector<ConfigNode*> GetConfigNodes(std::string_view baseUrl, std::string_view typeName) const;
    ConfigNode::NodePtr GetMergedConfigNodes(std::string_view nodeName, bool mergeChildren = false) const;
    std::vector<io::UrlConfig*> GetConfigs(std::string_view typeName) const;
    // GameDatabase.cs:1504
    static void CompileConfig(ConfigNode& node);

    // ---- textures (GameDatabase.cs:348-614; url comparisons are OrdinalIgnoreCase, plus the flag swaps) ----
    bool ExistsTexture(const std::string* url) const;
    io::UrlFile* GetTextureFile(const std::string* url) const;
    const TextureInfo* GetTextureInfo(const std::string* url) const;
    const TextureInfo* GetTextureInfoIn(const std::string* url, const std::string* textureName) const;
    bool RemoveTexture(std::string_view url);
    std::vector<const TextureInfo*> GetAllTexturesInFolderType(std::string_view folderName, bool caseInsensitive = false) const;
    std::vector<const TextureInfo*> GetAllTexturesInFolder(std::string_view folderURL) const;
    bool ExistsTexture(std::string_view url) const { const std::string s(url); return ExistsTexture(&s); }
    const TextureInfo* GetTextureInfo(std::string_view url) const { const std::string s(url); return GetTextureInfo(&s); }

    // ---- models (GameDatabase.cs:640-800; url comparisons are ordinal) ----
    bool ExistsModel(std::string_view url) const;
    const ModelInfo* GetModelPrefab(std::string_view url) const;
    io::UrlFile* GetModelFile(std::string_view url) const;
    io::UrlFile* GetModelFile(const ModelInfo* modelPrefab) const;
    // GameDatabase.cs:739: first model whose name up to its last '/' equals url (std::out_of_range for a name
    // without '/', like C#'s Substring(0, -1)).
    const ModelInfo* GetModelPrefabIn(std::string_view url) const;
    bool RemoveModel(std::string_view url);

    // ---- audio (GameDatabase.cs:250-346) ----
    bool ExistsAudioClip(std::string_view url) const;
    const AudioInfo* GetAudioClip(std::string_view url) const;
    io::UrlFile* GetAudioFile(std::string_view url) const;
    bool RemoveAudioClip(std::string_view url);

    // GameDatabase.cs:1462 ToString
    std::string ToString() const;

    std::vector<AudioInfo> databaseAudio;
    std::vector<io::UrlFile*> databaseAudioFiles;
    std::vector<TextureInfo> databaseTexture;
    std::vector<ModelInfo> databaseModel;
    std::vector<io::UrlFile*> databaseModelFiles;
    // Log of the load, in order: "Load(Audio): url", "Load(Texture): url", "Load(Model): url", "Config(TYPE) url"
    // (the lines KSP writes to KSP.log; used by tests to compare the order with a real game log).
    std::vector<std::string> loadLog;

private:
    void LoadObjects();
    static bool ResolveTexture(const GameDatabase& db, const std::string* url, size_t& index);

    LoadOptions options_;
    std::string gameDataPath_;
    std::unique_ptr<io::UrlDir> root_;
};

// Texture-file metadata readers used for the loader flags (no pixel decoding).
namespace texture_meta {
// DatabaseLoaderTexture_DDS.cs:29 Read: DXT1/DXT5 load; DXT2/3/4, DX10 and uncompressed formats fail (null).
struct DdsInfo {
    bool ok = false;          // header readable
    bool loads = false;       // the loader would return a texture
    bool isNormalMap = false;
    uint32_t width = 0, height = 0, fourCC = 0, pfFlags = 0, caps = 0;
};
DdsInfo ReadDdsHeader(const std::string& path);
// MBMReader.cs:25: BinaryReader string, width, height, TextureType (NormalMap = 1), bit depth.
struct MbmInfo {
    bool ok = false;
    int32_t width = 0, height = 0, textureType = 0, bitDepth = 0;
    bool isNormalMap = false;
};
MbmInfo ReadMbmHeader(const std::string& path);
}  // namespace texture_meta

}  // namespace ksp
