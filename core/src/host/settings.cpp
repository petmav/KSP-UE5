// WP-UE-02: the settings surface of the C ABI (kspcore_settings_*): GameSettings (GameSettings.cs) by field name. Host glue over the
// WP-50 port: Load = ConfigNode.Load + ParseCfg (:1691), Save = BuildCfg (:1619 WriteCfg minus the write) + ConfigNode.Save, get / set
// over the reflection table GameSettings::Fields() (Type.GetFields() / GetField(name), the way ParseCfg / WriteCfg read it). Only the scalar
// kinds (string, int, float, double, bool) are exposed by name; bindings are set by the controls layout files and settings.cfg.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "ksp/config/config_node.hpp"
#include "ksp/host/game_host.hpp"
#include "ksp/input/game_settings.hpp"

namespace ksp::host {

namespace {

const SettingsField* Find(const std::string& name) {
    for (const SettingsField& f : GameSettings::Fields())
        if (name == f.name) return &f;
    return nullptr;
}

}  // namespace

bool SettingsLoad(const std::string& path) {
    ConfigNode::NodePtr root = ConfigNode::Load(path);
    if (!root) return false;
    return GameSettings::ParseCfg(root) == GameSettings::ParseResult::Ok;
}

bool SettingsSave(const std::string& path) {
    ConfigNode::NodePtr node = GameSettings::BuildCfg();
    if (!node) return false;
    return node->Save(path);
}

bool SettingsGet(const std::string& name, std::string& value) {
    const SettingsField* f = Find(name);
    if (f == nullptr || f->address == nullptr) return false;
    char b[64];
    switch (f->kind) {
        case SettingsField::Kind::String: value = *static_cast<std::string*>(f->address); return true;
        case SettingsField::Kind::Int: value = std::to_string(*static_cast<int*>(f->address)); return true;
        case SettingsField::Kind::Float: std::snprintf(b, sizeof b, "%.9g", static_cast<double>(*static_cast<float*>(f->address))); value = b; return true;
        case SettingsField::Kind::Double: std::snprintf(b, sizeof b, "%.17g", *static_cast<double*>(f->address)); value = b; return true;
        case SettingsField::Kind::Bool: value = *static_cast<bool*>(f->address) ? "True" : "False"; return true;
        default: return false;
    }
}

bool SettingsSet(const std::string& name, const std::string& value) {
    const SettingsField* f = Find(name);
    if (f == nullptr || f->address == nullptr) return false;
    char* end = nullptr;
    switch (f->kind) {
        case SettingsField::Kind::String: *static_cast<std::string*>(f->address) = value; return true;
        case SettingsField::Kind::Int: {
            const long v = std::strtol(value.c_str(), &end, 10);
            if (end == value.c_str() || *end != '\0') return false;
            *static_cast<int*>(f->address) = static_cast<int>(v);
            return true;
        }
        case SettingsField::Kind::Float: {
            const float v = std::strtof(value.c_str(), &end);
            if (end == value.c_str() || *end != '\0') return false;
            *static_cast<float*>(f->address) = v;
            return true;
        }
        case SettingsField::Kind::Double: {
            const double v = std::strtod(value.c_str(), &end);
            if (end == value.c_str() || *end != '\0') return false;
            *static_cast<double*>(f->address) = v;
            return true;
        }
        case SettingsField::Kind::Bool:
            if (value == "True" || value == "true" || value == "1") *static_cast<bool*>(f->address) = true;
            else if (value == "False" || value == "false" || value == "0") *static_cast<bool*>(f->address) = false;
            else return false;
            return true;
        default: return false;
    }
}

}  // namespace ksp::host
