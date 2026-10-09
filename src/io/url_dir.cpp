#include "ksp/io/url_dir.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <system_error>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/dotnet/utf16.hpp"

namespace ksp::io {

namespace fs = std::filesystem;

namespace {

constexpr int64_t kUnixEpochTicks = 621355968000000000LL;   // DateTime(1970,1,1).Ticks
constexpr int64_t kFileTimeEpochTicks = 504911232000000000LL;  // DateTime(1601,1,1).Ticks

std::string PathToUtf8(const fs::path& p) {
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

fs::path Utf8ToPath(const std::string& s) {
    return dotnet::PathFromUtf8(s);
}

FileTime LastWriteTime(const fs::path& p) {
    FileTime t;
    std::error_code ec;
    const auto ft = fs::last_write_time(p, ec);
    if (ec) return t;  // UrlDir.cs:294 new DateTime(1, 1, 1)
#ifdef _MSC_VER
    // MSVC's file_clock is FILETIME: 100 ns units since 1601-01-01 UTC (not leap-second adjusted, like .NET).
    t.utcTicks = static_cast<int64_t>(ft.time_since_epoch().count()) + kFileTimeEpochTicks;
#else
    const auto sys = std::chrono::clock_cast<std::chrono::system_clock>(ft);
    t.utcTicks = std::chrono::duration_cast<std::chrono::duration<int64_t, std::ratio<1, 10000000>>>(
                     sys.time_since_epoch()).count() + kUnixEpochTicks;
#endif
    t.valid = true;
    return t;
}

// string.Trim(params char[]) with the given ASCII characters.
std::string_view TrimChars(std::string_view s, std::string_view chars) {
    size_t b = 0, e = s.size();
    while (b < e && chars.find(s[b]) != std::string_view::npos) ++b;
    while (e > b && chars.find(s[e - 1]) != std::string_view::npos) --e;
    return s.substr(b, e - b);
}

// Path.GetExtension / Path.GetFileNameWithoutExtension on a bare file name (Mono's Path.cs).
std::string GetExtension(std::string_view name) {
    const size_t dot = name.rfind('.');
    if (dot == std::string_view::npos || dot == name.size() - 1) return {};
    return std::string(name.substr(dot));
}
std::string GetFileNameWithoutExtension(std::string_view name) {
    const size_t dot = name.rfind('.');
    if (dot == std::string_view::npos) return std::string(name);
    return std::string(name.substr(0, dot));
}

}  // namespace

const char* FileTypeName(FileType t) {
    switch (t) {
        case FileType::Unknown: return "Unknown";
        case FileType::Config: return "Config";
        case FileType::Texture: return "Texture";
        case FileType::Model: return "Model";
        case FileType::Audio: return "Audio";
        case FileType::Assembly: return "Assembly";
        case FileType::AssetBundle: return "AssetBundle";
    }
    return "Unknown";
}

// DateTime.ToBinary (local kind): ticks - utcOffset, i.e. the UTC ticks, with the Local flag bit 63 set.
std::string FileTime::ToBinaryString() const {
    const uint64_t bits = static_cast<uint64_t>(valid ? utcTicks : 0) | 0x8000000000000000ULL;
    return std::to_string(static_cast<int64_t>(bits));
}

// ---------------------------------------------------------------------------------------------------------------------
// UrlDir.cs:128 ConstructURL / :135 UrlSplit
UrlIdentifier::UrlIdentifier(std::string_view url) : url_(url) {
    const std::string_view t = TrimChars(url, "/ \n\r\t");
    urlSplit_ = dotnet::Split(t, "/ .", true);
    urlDepth_ = static_cast<int>(urlSplit_.size()) - 1;
}

const std::string& UrlIdentifier::operator[](int index) const {
    if (index < 0 || index >= static_cast<int>(urlSplit_.size()))
        throw std::out_of_range("Index was outside the bounds of the array.");
    return urlSplit_[static_cast<size_t>(index)];
}

// ---------------------------------------------------------------------------------------------------------------------
// UrlDir.cs:177
UrlConfig::UrlConfig(UrlFile* parent, ConfigNode::NodePtr node) : parent_(parent) {
    type_ = node->name;
    if (node->name.empty()) node->name = parent->name();
    if (node->HasValue("name"))
        name_ = *node->GetValue("name");
    else
        name_ = node->name;
    config_ = std::move(node);
}

std::string UrlConfig::url() const { return parent_->url() + "/" + name_; }

// UrlDir.cs:196
std::vector<std::unique_ptr<UrlConfig>> UrlConfig::CreateNodeList(UrlDir* parentDir, UrlFile* parent) {
    std::vector<std::unique_ptr<UrlConfig>> list;
    ConfigNode::NodePtr configNode = ConfigNode::Load(parent->fullPath());
    if (!configNode) {
        log::Warning("Cannot create config from file '" + parent->fullPath() + "'.");
        return list;
    }
    if (!configNode->HasData()) {
        log::Warning("Config in file '" + parent->fullPath() + "' contains no data.");
        return list;
    }
    if (configNode->HasValue() && parentDir->type() == DirectoryType::Parts) {
        configNode->name = "PART";
        list.push_back(std::make_unique<UrlConfig>(parent, configNode));
    } else {
        const int count = configNode->nodes().Count();
        for (int i = 0; i < count; i++) {
            ConfigNode::NodePtr configNode2 = configNode->nodes().Shared(i);
            if (configNode2->name.empty()) {
                if (parentDir->type() != DirectoryType::Parts) {
                    log::Warning("Config in file '" + parent->fullPath() + "' contains an unnamed node. Skipping.");
                    continue;
                }
                configNode2->name = "PART";
            }
            list.push_back(std::make_unique<UrlConfig>(parent, configNode2));
        }
    }
    return list;
}

// ---------------------------------------------------------------------------------------------------------------------
// UrlDir.cs:283
UrlFile::UrlFile(UrlDir* parent, const DirEntryInfo& info) {
    name_ = GetFileNameWithoutExtension(info.name);
    path_ = info.fullName;
    fileExtension_ = GetExtension(info.name);
    fileTime_ = info.lastWriteTime;
    if (fileExtension_.size() > 1) fileExtension_ = fileExtension_.substr(1);
    parent_ = parent;
    root_ = parent->root();
    if (fileExtension_ == "cfg") {
        fileType_ = FileType::Config;
        configs_ = UrlConfig::CreateNodeList(parent, this);
    } else {
        fileType_ = FileType::Unknown;
    }
}

std::string UrlFile::url() const { return parent_->url() + "/" + name_; }

bool UrlFile::Exists(std::string_view url) const { return Exists(UrlIdentifier(url), 0); }

// UrlDir.cs:319
bool UrlFile::Exists(const UrlIdentifier& url, int index) const {
    if (url.urlDepth() == -1) return false;
    if (index == url.urlDepth()) return GetConfig(url[index]) != nullptr;
    return false;
}

// UrlDir.cs:336
UrlConfig* UrlFile::GetConfig(std::string_view name) const {
    for (const auto& c : configs_)
        if (c->name() == name) return c.get();
    log::Error("Cannot find config in file : " + std::string(name));
    return nullptr;
}

bool UrlFile::ContainsConfig(std::string_view name) const { return GetConfig(name) != nullptr; }

// UrlDir.cs:362
void UrlFile::ConfigureFile(const std::vector<ConfigFileType>& fileConfig) {
    if (fileType_ != FileType::Unknown) return;
    for (const ConfigFileType& t : fileConfig)
        for (const std::string& ext : t.extensions)
            if (ext == fileExtension_) {
                fileType_ = t.type;
                return;
            }
}

// UrlDir.cs:400
void UrlFile::AddConfig(ConfigNode::NodePtr newConfig) {
    if (fileType_ != FileType::Config) {
        log::Info("Cannot add config as file is not of type Config");
        return;
    }
    configs_.push_back(std::make_unique<UrlConfig>(this, std::move(newConfig)));
}

// ---------------------------------------------------------------------------------------------------------------------
// UrlDir.cs:576
UrlDir::UrlDir(const std::vector<ConfigDirectory>& dirConfig, const std::vector<ConfigFileType>& fileConfig,
               const std::string& applicationRootPath) {
    parent_ = nullptr;
    root_ = this;
    name_ = "root";
    for (const ConfigDirectory& d : dirConfig)
        children_.push_back(std::unique_ptr<UrlDir>(new UrlDir(this, d, applicationRootPath)));
    for (UrlFile* f : AllFiles()) f->ConfigureFile(fileConfig);
}

// UrlDir.cs:594
UrlDir::UrlDir(UrlDir* root, const ConfigDirectory& rootInfo, const std::string& applicationRootPath) {
    // Directory.CreateDirectory(CreateApplicationPath(directory)).FullName: the normalised absolute path.
    std::error_code ec;
    fs::path p = Utf8ToPath(CreateApplicationPath(applicationRootPath, rootInfo.directory));
    fs::path full = fs::weakly_canonical(fs::absolute(p, ec), ec);
    if (full.empty()) full = fs::absolute(p, ec);
    full.make_preferred();
    std::string s = PathToUtf8(full);
    while (s.size() > 3 && (s.back() == '\\' || s.back() == '/')) s.pop_back();
    name_ = rootInfo.urlRoot;
    type_ = rootInfo.type;
    Create(root, s);
}

// UrlDir.cs:602
UrlDir::UrlDir(UrlDir* parent, const DirEntryInfo& info) {
    name_ = info.name;
    type_ = parent->type();
    Create(parent, info.fullName);
}

// UrlDir.cs:609
void UrlDir::Create(UrlDir* parent, const std::string& fullPath) {
    path_ = fullPath;
    parent_ = parent;
    root_ = parent->root();
    std::vector<DirEntryInfo> fileInfos, dirInfos;
    ListDirectory(fullPath, fileInfos, dirInfos);
    for (const DirEntryInfo& fi : fileInfos) files_.push_back(std::make_unique<UrlFile>(this, fi));
    for (const DirEntryInfo& di : dirInfos) {
        if (di.name != ".svn" && di.name != "PluginData" && di.name != "zDeprecated")
            children_.push_back(std::unique_ptr<UrlDir>(new UrlDir(this, di)));
    }
}

bool UrlDir::NtfsLess(std::string_view a, std::string_view b) {
    const std::u16string ua = dotnet::ToUtf16(a), ub = dotnet::ToUtf16(b);
    const size_t n = std::min(ua.size(), ub.size());
    for (size_t i = 0; i < n; i++) {
        const char16_t ca = dotnet::CharToUpper(ua[i]), cb = dotnet::CharToUpper(ub[i]);
        if (ca != cb) return ca < cb;
    }
    return ua.size() < ub.size();
}

void UrlDir::ListDirectory(const std::string& path, std::vector<DirEntryInfo>& files, std::vector<DirEntryInfo>& dirs) {
    files.clear();
    dirs.clear();
    std::error_code ec;
    fs::directory_iterator it(Utf8ToPath(path), ec), end;
    if (ec) return;
    for (; it != end; it.increment(ec)) {
        if (ec) break;
        const fs::directory_entry& e = *it;
        DirEntryInfo info;
        info.name = PathToUtf8(e.path().filename());
        fs::path full = e.path();
        full.make_preferred();
        info.fullName = PathToUtf8(full);
        std::error_code ec2;
        if (e.is_directory(ec2)) {
            dirs.push_back(std::move(info));
        } else {
            info.lastWriteTime = LastWriteTime(e.path());
            files.push_back(std::move(info));
        }
    }
    auto less = [](const DirEntryInfo& x, const DirEntryInfo& y) { return NtfsLess(x.name, y.name); };
    std::sort(files.begin(), files.end(), less);
    std::sort(dirs.begin(), dirs.end(), less);
}

// UrlDir.cs:472
std::string UrlDir::url() const {
    if (parent_ != nullptr && parent_ != root_ && !parent_->name().empty()) return parent_->url() + "/" + name_;
    return name_;
}

void UrlDir::CollectDirectories(std::vector<UrlDir*>& out) const {
    for (const auto& c : children_) {
        out.push_back(c.get());
        c->CollectDirectories(out);
    }
}

std::vector<UrlDir*> UrlDir::AllDirectories() const {
    std::vector<UrlDir*> out;
    CollectDirectories(out);
    return out;
}

std::vector<UrlFile*> UrlDir::AllFiles() const {
    std::vector<UrlFile*> out;
    for (UrlDir* d : AllDirectories())
        for (const auto& f : d->files())
            if (f->fileType() != FileType::Config) out.push_back(f.get());
    return out;
}

std::vector<UrlFile*> UrlDir::AllConfigFiles() const {
    std::vector<UrlFile*> out;
    for (UrlDir* d : AllDirectories())
        for (const auto& f : d->files())
            if (f->fileType() == FileType::Config) out.push_back(f.get());
    return out;
}

std::vector<UrlConfig*> UrlDir::AllConfigs() const {
    std::vector<UrlConfig*> out;
    for (UrlFile* f : AllConfigFiles())
        for (const auto& c : f->configs()) out.push_back(c.get());
    return out;
}

bool UrlDir::DirectoryExists(std::string_view url) const { return GetDirectory(url) != nullptr; }

// UrlDir.cs:639
UrlDir* UrlDir::GetDirectory(std::string_view url) const {
    UrlIdentifier id(url);
    if (id.urlDepth() == -1) return nullptr;
    return GetDirectory(id, 0);
}

// UrlDir.cs:648
UrlDir* UrlDir::GetDirectory(const UrlIdentifier& id, int index) const {
    if (index == id.urlDepth()) {
        for (const auto& c : children_)
            if (id[index] == c->name()) return c.get();
    } else {
        for (const auto& c : children_)
            if (id[index] == c->name()) return c->GetDirectory(id, index + 1);
    }
    return nullptr;
}

bool UrlDir::FileExists(std::string_view url) const { return GetFile(url) != nullptr; }

// UrlDir.cs:731
UrlFile* UrlDir::GetFile(std::string_view url) const {
    UrlIdentifier id(url);
    if (id.urlDepth() == -1) return nullptr;
    return GetFile(id, 0);
}

// UrlDir.cs:740
UrlFile* UrlDir::GetFile(const UrlIdentifier& id, int index) const {
    if (index == id.urlDepth() - 1) {
        for (const auto& f : files_)
            if (id[index] == f->name()) return f.get();
    } else {
        for (const auto& c : children_)
            if (id[index] == c->name()) return c->GetFile(id, index + 1);
    }
    return nullptr;
}

bool UrlDir::ConfigExists(std::string_view url) const { return GetConfig(url) != nullptr; }

// UrlDir.cs:772
UrlConfig* UrlDir::GetConfig(std::string_view url) const {
    UrlIdentifier id(url);
    if (id.urlDepth() == -1) {
        log::Error("Invalid url: '" + std::string(url) + "'");
        return nullptr;
    }
    return GetConfig(id, 0);
}

// UrlDir.cs:783
UrlConfig* UrlDir::GetConfig(const UrlIdentifier& id, int index) const {
    if (index == id.urlDepth()) {
        for (const auto& f : files_)
            if (f->fileType() == FileType::Config && f->ContainsConfig(id[index])) return f->GetConfig(id[index]);
    } else {
        for (const auto& c : children_) {
            if (!c->name().empty()) {
                if (id[index] == c->name()) return c->GetConfig(id, index + 1);
                continue;
            }
            if (UrlConfig* config = c->GetConfig(id, index)) return config;
        }
        for (const auto& f : files_)
            if (f->fileType() == FileType::Config && id[index] == f->name()) return f->GetConfig(id[index + 1]);
    }
    return nullptr;
}

// UrlDir.cs:830
std::vector<UrlFile*> UrlDir::GetFiles(FileType type) const {
    std::vector<UrlFile*> out;
    for (UrlFile* f : AllFiles())
        if (f->fileType() == type) out.push_back(f);
    return out;
}

// UrlDir.cs:841
std::vector<UrlConfig*> UrlDir::GetConfigs(std::string_view typeName, bool recursive) const {
    std::vector<UrlConfig*> out;
    if (recursive) {
        for (UrlConfig* c : AllConfigs())
            if (c->type() == typeName) out.push_back(c);
        return out;
    }
    for (const auto& f : files_)
        for (const auto& c : f->configs())
            if (c->type() == typeName) out.push_back(c.get());
    return out;
}

// UrlDir.cs:877
std::vector<UrlConfig*> UrlDir::GetConfigs(std::string_view typeName, std::string_view name, bool recursive) const {
    std::vector<UrlConfig*> out;
    if (recursive) {
        for (UrlConfig* c : AllConfigs())
            if (c->type() == typeName && c->name() == name) out.push_back(c);
        return out;
    }
    for (const auto& f : files_)
        for (const auto& c : f->configs())
            if (c->type() == typeName && c->name() == name) out.push_back(c.get());
    return out;
}

// UrlDir.cs:913 (Path.Combine)
std::string UrlDir::CreateApplicationPath(const std::string& applicationRootPath, const std::string& relativePath) {
    if (relativePath.empty()) return applicationRootPath;
    if (applicationRootPath.empty()) return relativePath;
    const fs::path rel = Utf8ToPath(relativePath);
    if (rel.is_absolute()) return relativePath;
    const char last = applicationRootPath.back();
    if (last == '/' || last == '\\' || last == ':') return applicationRootPath + relativePath;
    return applicationRootPath + "\\" + relativePath;
}

// UrlDir.cs:918
std::string UrlDir::StripExtension(std::string_view filename, std::string_view extension) {
    if (extension.size() > filename.size()) throw std::out_of_range("StripExtension");
    return std::string(filename.substr(0, filename.size() - extension.size()));
}

}  // namespace ksp::io
