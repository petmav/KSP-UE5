// UrlDir (Assembly-CSharp/UrlDir.cs): KSP's view of the GameData tree. A UrlDir is a directory, a UrlFile a file in it,
// a UrlConfig one top-level node of a .cfg file. Urls are '/'-joined names without extensions, rooted at the configured
// directory's urlRoot ("" for GameData, so "Squad/Parts/Command/mk1pod_v2/mk1Pod_v2/mk1pod_v2" is
// <dir Squad/Parts/Command/mk1pod_v2> / <file mk1Pod_v2.cfg> / <config named mk1pod_v2>).
//
// Faithful to KSP on Windows:
// - Directory listing order is the NTFS index order KSP gets from DirectoryInfo.GetFiles/GetDirectories (Mono calls
//   FindFirstFile/FindNextFile): names compared ordinally after upper-casing each UTF-16 unit. kspcore sorts explicitly,
//   so the order is the same on any file system.
// - Sub-directories named exactly ".svn", "PluginData" or "zDeprecated" are skipped (case-sensitive).
// - Extensions are compared case-sensitively ("cfg" is a config, "CFG" is not); a file's type is the first
//   ConfigFileType whose extension list contains its extension.
// - Every directory is read once at construction; .cfg files are parsed with ksp::ConfigNode::Load at that time.
// Deviations (documented): KSP creates missing configured root directories (Directory.CreateDirectory); kspcore never
// writes and treats a missing root as empty. File times are UTC .NET ticks (see FileTime).
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"

namespace ksp::io {

// UrlDir.cs:9
enum class DirectoryType : int { Parts = 1, Internals = 2, GameData = 3 };
// UrlDir.cs:17
enum class FileType : int { Unknown, Config, Texture, Model, Audio, Assembly, AssetBundle };
const char* FileTypeName(FileType t);

// UrlDir.cs:29 ConfigDirectory (serialized on the GameDatabase prefab)
struct ConfigDirectory {
    std::string urlRoot;
    std::string directory = ".";
    DirectoryType type = DirectoryType::Parts;
};

// UrlDir.cs:52 ConfigFileType
struct ConfigFileType {
    FileType type = FileType::Unknown;
    std::vector<std::string> extensions;
};

// File times as KSP keeps them (FileInfo.LastWriteTime, a local DateTime). Stored as UTC .NET ticks (100 ns since
// 0001-01-01). `valid` is false where C# fell back to new DateTime(1, 1, 1) (UrlDir.cs:294).
struct FileTime {
    int64_t utcTicks = 0;
    bool valid = false;
    // DateTime.ToBinary() of the local-kind LastWriteTime: (UTC ticks) | 0x8000000000000000, as Int64 text
    // (AvailablePart.AddFileTime / the PartDatabase.cfg "timestamps" value).
    std::string ToBinaryString() const;
};

// UrlDir.cs:84
class UrlIdentifier {
public:
    UrlIdentifier() = default;
    explicit UrlIdentifier(std::string_view url);  // UrlDir.cs:128 ConstructURL
    const std::string& url() const { return url_; }
    const std::vector<std::string>& urlSplit() const { return urlSplit_; }
    int urlDepth() const { return urlDepth_; }
    const std::string& operator[](int index) const;  // std::out_of_range like C#'s IndexOutOfRangeException

private:
    std::string url_;
    std::vector<std::string> urlSplit_;
    int urlDepth_ = -1;
};

class UrlDir;
class UrlFile;

// UrlDir.cs:142
class UrlConfig {
public:
    UrlConfig(UrlFile* parent, ConfigNode::NodePtr node);  // UrlDir.cs:177
    const std::string& name() const { return name_; }
    const std::string& type() const { return type_; }
    ConfigNode& config() const { return *config_; }
    const ConfigNode::NodePtr& configPtr() const { return config_; }
    void setConfig(ConfigNode::NodePtr c) { config_ = std::move(c); }
    UrlFile* parent() const { return parent_; }
    std::string url() const;  // parent.url + "/" + name

    // UrlDir.cs:196
    static std::vector<std::unique_ptr<UrlConfig>> CreateNodeList(UrlDir* parentDir, UrlFile* parent);

private:
    std::string name_;
    std::string type_;
    ConfigNode::NodePtr config_;
    UrlFile* parent_ = nullptr;
};

// One directory entry as KSP's FileInfo sees it (name with extension, full path, last write time).
struct DirEntryInfo {
    std::string name;
    std::string fullName;
    FileTime lastWriteTime;
};

// UrlDir.cs:237
class UrlFile {
public:
    UrlFile(UrlDir* parent, const DirEntryInfo& info);  // UrlDir.cs:283
    const std::string& name() const { return name_; }
    FileType fileType() const { return fileType_; }
    const std::string& fullPath() const { return path_; }
    const std::string& fileExtension() const { return fileExtension_; }
    const FileTime& fileTime() const { return fileTime_; }
    UrlDir* root() const { return root_; }
    UrlDir* parent() const { return parent_; }
    const std::vector<std::unique_ptr<UrlConfig>>& configs() const { return configs_; }
    std::string url() const;

    bool Exists(std::string_view url) const;                      // UrlDir.cs:314
    bool Exists(const UrlIdentifier& url, int index) const;       // UrlDir.cs:319
    UrlConfig* GetConfig(std::string_view name) const;            // UrlDir.cs:336 (logs an error when missing)
    bool ContainsConfig(std::string_view name) const;             // UrlDir.cs:357
    void ConfigureFile(const std::vector<ConfigFileType>& fileConfig);  // UrlDir.cs:362
    void AddConfig(ConfigNode::NodePtr newConfig);                // UrlDir.cs:400

private:
    std::string name_;
    FileType fileType_ = FileType::Unknown;
    std::string path_;
    std::string fileExtension_;
    FileTime fileTime_;
    UrlDir* root_ = nullptr;
    UrlDir* parent_ = nullptr;
    std::vector<std::unique_ptr<UrlConfig>> configs_;
};

// UrlDir.cs:7
class UrlDir {
public:
    // UrlDir.cs:576: the root ("root") with one child per ConfigDirectory, resolved against applicationRootPath (KSP:
    // Application.dataPath + "/../"), then every non-config file typed with fileConfig.
    UrlDir(const std::vector<ConfigDirectory>& dirConfig, const std::vector<ConfigFileType>& fileConfig,
           const std::string& applicationRootPath);
    UrlDir(const UrlDir&) = delete;
    UrlDir& operator=(const UrlDir&) = delete;

    const std::string& name() const { return name_; }
    UrlDir* root() const { return root_; }
    UrlDir* parent() const { return parent_; }
    const std::vector<std::unique_ptr<UrlDir>>& children() const { return children_; }
    const std::vector<std::unique_ptr<UrlFile>>& files() const { return files_; }
    const std::string& path() const { return path_; }
    DirectoryType type() const { return type_; }
    std::string url() const;  // UrlDir.cs:472

    // UrlDir.cs:484 AllDirectories (pre-order: child, then its subtree), :504 AllFiles (non-config files),
    // :525 AllConfigFiles, :546 AllConfigs. Materialised as vectors (C# yields lazily; the order is identical).
    std::vector<UrlDir*> AllDirectories() const;
    std::vector<UrlFile*> AllFiles() const;
    std::vector<UrlFile*> AllConfigFiles() const;
    std::vector<UrlConfig*> AllConfigs() const;

    bool DirectoryExists(std::string_view url) const;      // UrlDir.cs:634
    UrlDir* GetDirectory(std::string_view url) const;      // UrlDir.cs:639
    bool FileExists(std::string_view url) const;           // UrlDir.cs:726
    UrlFile* GetFile(std::string_view url) const;          // UrlDir.cs:731
    bool ConfigExists(std::string_view url) const;         // UrlDir.cs:767
    UrlConfig* GetConfig(std::string_view url) const;      // UrlDir.cs:772
    std::vector<UrlFile*> GetFiles(FileType type) const;   // UrlDir.cs:830
    std::vector<UrlConfig*> GetConfigs(std::string_view typeName, bool recursive = true) const;  // UrlDir.cs:841
    std::vector<UrlConfig*> GetConfigs(std::string_view typeName, std::string_view name, bool recursive = true) const;  // :877

    // UrlDir.cs:913/918/923
    static std::string CreateApplicationPath(const std::string& applicationRootPath, const std::string& relativePath);
    static std::string StripExtension(std::string_view filename, std::string_view extension);

    // kspcore: list a directory in KSP's (NTFS) order. Exposed for tests.
    static void ListDirectory(const std::string& path, std::vector<DirEntryInfo>& files, std::vector<DirEntryInfo>& dirs);
    // kspcore: the NTFS collation used for the order (UTF-16 units upper-cased, then compared ordinally).
    static bool NtfsLess(std::string_view a, std::string_view b);

private:
    UrlDir(UrlDir* root, const ConfigDirectory& rootInfo, const std::string& applicationRootPath);  // UrlDir.cs:594
    UrlDir(UrlDir* parent, const DirEntryInfo& info);                                               // UrlDir.cs:602
    void Create(UrlDir* parent, const std::string& fullPath);                                       // UrlDir.cs:609
    UrlDir* GetDirectory(const UrlIdentifier& id, int index) const;
    UrlFile* GetFile(const UrlIdentifier& id, int index) const;
    UrlConfig* GetConfig(const UrlIdentifier& id, int index) const;
    void CollectDirectories(std::vector<UrlDir*>& out) const;

    std::string name_;
    UrlDir* root_ = nullptr;
    UrlDir* parent_ = nullptr;
    std::vector<std::unique_ptr<UrlDir>> children_;
    std::vector<std::unique_ptr<UrlFile>> files_;
    std::string path_;
    DirectoryType type_ = DirectoryType::GameData;
};

}  // namespace ksp::io
