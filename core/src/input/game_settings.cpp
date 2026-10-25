// WP-50: GameSettings.cs (everything except the generated SetDefaultValues / Fields in game_settings_defaults.cpp), KeyboardLayout.cs
// and TrackIR.Settings.
#include "ksp/input/game_settings.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <system_error>

#include "binding_util.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/dotnet/version.hpp"
#include "ksp/events/game_events.hpp"

namespace ksp {

using namespace input_detail;

namespace {

bool FloatEquals(float a, float b) { return a == b || (std::isnan(a) && std::isnan(b)); }

// ordinal-ignore-case equality for ASCII names (StringComparison.InvariantCultureIgnoreCase on cfg file names)
std::string AsciiLower(std::string s) {
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return s;
}
bool EqualsIgnoreCase(const std::string& a, const std::string& b) { return AsciiLower(a) == AsciiLower(b); }
bool StartsWithIgnoreCase(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && AsciiLower(s.substr(0, prefix.size())) == AsciiLower(prefix);
}

// the public static field with this name (Type.GetField(name) for the IConfigNode fields; null otherwise)
const SettingsField* FindField(const std::string& name) {
    for (const SettingsField& f : GameSettings::Fields())
        if (name == f.name) return &f;
    return nullptr;
}

// `field.GetValue(this) is IConfigNode c` then c.Load(node): returns true when the value was a (non-null) IConfigNode
bool LoadIntoConfigNodeField(const SettingsField& f, const ConfigNode::NodePtr& node) {
    switch (f.kind) {
        case SettingsField::Kind::KeyBinding: {
            auto& p = *static_cast<std::shared_ptr<KeyBinding>*>(f.address);
            if (p) { p->Load(*node); return true; }
            return false;
        }
        case SettingsField::Kind::AxisBinding: {
            auto& p = *static_cast<std::shared_ptr<AxisBinding>*>(f.address);
            if (p) { p->Load(*node); return true; }
            return false;
        }
        case SettingsField::Kind::AxisKeyBindingList: {
            auto& p = *static_cast<std::shared_ptr<AxisKeyBindingList>*>(f.address);
            if (p) { p->Load(*node); return true; }
            return false;
        }
        case SettingsField::Kind::InputDevices: {
            auto& p = *static_cast<std::shared_ptr<InputDevices>*>(f.address);
            if (p) { p->Load(*node); return true; }
            return false;
        }
        case SettingsField::Kind::TrackIR: {
            auto& p = *static_cast<std::shared_ptr<TrackIRSettings>*>(f.address);
            if (p) { p->Load(node); return true; }
            return false;
        }
        default:
            return false;
    }
}

// Convert.ToString(fieldValue, CultureInfo.InvariantCulture) of a non-IConfigNode field
std::string ConvertToString(const SettingsField& f) {
    switch (f.kind) {
        case SettingsField::Kind::String: return *static_cast<std::string*>(f.address);
        case SettingsField::Kind::Int: return dotnet::FormatInt64(*static_cast<int*>(f.address));
        case SettingsField::Kind::Bool: return dotnet::FormatBool(*static_cast<bool*>(f.address));
        case SettingsField::Kind::Float: return dotnet::FormatSingleG(*static_cast<float*>(f.address));
        case SettingsField::Kind::Double: return dotnet::FormatDoubleG(*static_cast<double*>(f.address));
        case SettingsField::Kind::ValidatorMode:
            return dotnet::EnumToString(*static_cast<Expansions::Missions::Editor::ValidatorMode*>(f.address));
        case SettingsField::Kind::MaxRailsRateMode:
            return dotnet::EnumToString(*static_cast<TimeWarp::MaxRailsRateMode*>(f.address));
        case SettingsField::Kind::FloatList:
            // a List<float> is not IConvertible / IFormattable: obj.ToString() (null -> "")
            return *static_cast<std::shared_ptr<std::vector<float>>*>(f.address) ? "System.Collections.Generic.List`1[System.Single]" : "";
        default:
            return "";  // a null IConfigNode field: Convert.ToString(null) = ""
    }
}

}  // namespace

// ------------------------------------------------------------------------------------------------ KeyboardLayout

// KeyboardLayout.cs:14 (user32.dll GetKeyboardLayout: the host's `provider`; unset = an en-US QWERTY keyboard)
KeyboardLayout KeyboardLayout::GetKeyboardLayout() {
    if (provider) return provider();
    KeyboardLayout k;
    k.LocaleName = "en-US";
    k.TwoLetterISOLanguageName = "en";
    k.Type = KeyboardLayoutType::QWERTY;
    return k;
}

// KeyboardLayout.cs:25
KeyboardLayoutType KeyboardLayout::GetKeyboardLayoutType(int64_t layoutCode) {
    switch (static_cast<int>(layoutCode >> 16)) {
        case 1062: return KeyboardLayoutType::Latvian;
        default: return KeyboardLayoutType::QWERTY;
        case 61460:
        case 66591: return KeyboardLayoutType::TurkishF;
        case 61442:
        case 66569: return KeyboardLayoutType::Dvorak;
        case 1036:
        case 2060: return KeyboardLayoutType::AZERTY;
        case 1031:
        case 2055: return KeyboardLayoutType::QWERTZ;
    }
}

// ------------------------------------------------------------------------------------------------ TrackIR.Settings

// TrackIR.cs:84
void TrackIRSettings::Load(const ConfigNode::NodePtr& node) {
    node_ = node;
    if (Instance != nullptr) Instance->Load(*node);
}

// TrackIR.cs:93
void TrackIRSettings::Save(ConfigNode& node) const {
    if (Instance != nullptr) {
        Instance->Save(node);
    } else if (node_) {
        if (node_->name == "TRACKIR") {
            node_->CopyTo(node);
        } else if (node_->HasNode("TRACKIR")) {
            ConfigNode::NodePtr child;
            for (int i = 0; i < node_->nodes().Count(); ++i)
                if (node_->nodes()[i] == node_->GetNode("TRACKIR")) child = node_->nodes().Shared(i);
            node.SetNode("TRACKIR", child, true);
        }
    }
}

// ------------------------------------------------------------------------------------------------ GameSettings

// GameSettings.cs:1407
void GameSettings::LoadLayoutKeyBindings(const std::string& layoutSettings, bool overrideSettings) {
    if (!layoutConfigNodes.ContainsKey(layoutSettings)) return;
    ConfigNode::NodePtr configNode = layoutConfigNodes.At(layoutSettings);
    int i = 0;
    for (int count = configNode->nodes().Count(); i < count; i++) {
        const ConfigNode::NodePtr& configNode2 = configNode->nodes().Shared(i);
        const SettingsField* field = FindField(configNode2->name);
        if (field != nullptr) LoadIntoConfigNodeField(*field, configNode2);
    }
    if (overrideSettings) CURRENT_LAYOUT_SETTINGS = layoutSettings;
}

// GameSettings.cs:1432
std::string GameSettings::DetectKeyboardLayout() {
    const auto& dictionary = layoutConfigNodes;
    KeyboardLayout currentLayout = KeyboardLayout::GetKeyboardLayout();
    std::string text = dotnet::EnumToString(currentLayout.Type) + "_" + currentLayout.LocaleName + ".cfg";
    if (EqualsIgnoreCase(text, "Qwerty_en-US.cfg")) text = "Qwerty.cfg";
    else if (EqualsIgnoreCase(text, "Azerty_fr-FR.cfg")) text = "Azerty.cfg";
    else if (EqualsIgnoreCase(text, "Qwertz_de-DE.cfg")) text = "Qwertz.cfg";
    if (dictionary.ContainsKey(text)) return text;
    const std::string prefix = dotnet::EnumToString(currentLayout.Type) + "_" + currentLayout.TwoLetterISOLanguageName;
    for (const auto& kv : dictionary)
        if (StartsWithIgnoreCase(kv.first, prefix)) return kv.first;
    const std::string plain = dotnet::EnumToString(currentLayout.Type) + ".cfg";
    for (const auto& kv : dictionary)
        if (EqualsIgnoreCase(kv.first, plain)) return kv.first;
    return "Qwerty.cfg";
}

// GameSettings.cs:1467 (WriteCfg + WriteLauncherCFG; the launcher .bak file is not ported)
void GameSettings::SaveSettings() { WriteCfg(); }

// GameSettings.cs:1473
void GameSettings::ResetSettings() {
    SetDefaultValues();
    LoadLayoutKeyBindings(DetectKeyboardLayout(), true);
    if (host.createDefaultPresetList) host.createDefaultPresetList();  // PQSCache.CreateDefaultPresetList()
    SaveSettings();
    ApplySettings();
}

// GameSettings.cs:1482
void GameSettings::LoadGameSettingsOnly() { ParseCfgFile(); }

namespace {
struct GameSaveListener {
    void OnGameSave(Game*) { GameSettings::OnGameSave(); }
};
GameSaveListener& SaveListener() {
    static GameSaveListener l;
    return l;
}
bool g_fetch = false;  // GameSettings.fetch != null (:947)
}  // namespace

// GameSettings.cs:953
bool GameSettings::Ready() { return g_fetch; }

// GameSettings.cs:1506 (WP-20g: the host's start-up; see the header)
void GameSettings::Awake(const AwakeOptions& options) {
    namespace fs = std::filesystem;
    if (g_fetch) return;                                                 // :1508-1512 another instance: Destroy(this)
    g_fetch = true;                                                      // :1513
    layoutConfigNodes.Clear();                                           // :33 the new instance's field initialiser
    SetDefaultValues();                                                  // :1529
    if (!INPUT_DEVICES) INPUT_DEVICES = std::make_shared<InputDevices>();  // :1534-1537
    const fs::path root(options.gameRoot);
    configFilePath = (root / "settings.cfg").string();                   // :1538
    controlsLayoutPath = (root / options.controlsLayoutPath).string();  // :1540
    std::error_code ec;
    if (fs::is_directory(controlsLayoutPath, ec)) {                      // :1541-1544
        std::vector<std::string> files;
        for (fs::recursive_directory_iterator it(controlsLayoutPath, ec), end; !ec && it != end; it.increment(ec)) {
            if (!it->is_regular_file(ec)) continue;
            std::string ext = it->path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (ext == ".cfg") files.push_back(it->path().string());
        }
        std::sort(files.begin(), files.end());
        ParseLayoutsCfg(files);
    }
    if (fs::exists(configFilePath, ec) && !overrideSettings) {          // :1545-1549
        ParseCfgFile();
        if (options.writeSettingsCfg) WriteCfg();
    } else {                                                             // :1550-1555
        LoadLayoutKeyBindings(DetectKeyboardLayout(), true);
        InitializeDefaultValues();
        if (options.writeSettingsCfg) WriteCfg();
    }
    saveOnGameSave = false;                                              // :1564
    events::GameEvents::onGameStateSaved.Remove(&SaveListener(), &GameSaveListener::OnGameSave);
    events::GameEvents::onGameStateSaved.Add(&SaveListener(), &GameSaveListener::OnGameSave);  // :1565
}

// GameSettings.cs:1573
void GameSettings::OnDestroy() {
    g_fetch = false;  // :1575-1578
    events::GameEvents::onGameStateSaved.Remove(&SaveListener(), &GameSaveListener::OnGameSave);
}

// GameSettings.cs:1487
void GameSettings::SaveGameSettingsOnly() { WriteCfg(); }

// GameSettings.cs:1582
void GameSettings::ParseLayoutsCfg(const std::vector<std::string>& layoutFiles) {
    for (const std::string& file : layoutFiles) {
        // Path.GetFileName
        const size_t cut = file.find_last_of("/\\");
        const std::string fileName = cut == std::string::npos ? file : file.substr(cut + 1);
        layoutConfigNodes.Add(fileName, ParseLayoutRecursive(ConfigNode::Create(), file));
    }
}

// GameSettings.cs:1591
ConfigNode::NodePtr GameSettings::ParseLayoutRecursive(ConfigNode::NodePtr node, const std::string& layoutFile, int depth) {
    ConfigNode::NodePtr configNode = ConfigNode::Load(layoutFile);
    if (!configNode) {
        log::Warning("GameSettings: Cannot find keyboard layout " + layoutFile);
        return node;
    }
    if (!configNode->HasNode("KEYBOARD_LAYOUT")) {
        log::Warning("GameSettings: " + layoutFile + " invalid format, missing KEYBOARD_LAYOUT node!");
        return node;
    }
    ConfigNode* node2 = configNode->GetNode("KEYBOARD_LAYOUT");
    if (node2->HasValue("dependency") && depth < 5) {
        depth++;
        const std::string dependency = *node2->GetValue("dependency");
        node = !layoutConfigNodes.ContainsKey(dependency) ? ParseLayoutRecursive(node, dependency, depth)
                                                          : layoutConfigNodes.At(dependency)->CreateCopy();
    }
    if (configNode->HasNode("KEY_MAP") && node->HasNode("KEY_MAP")) {
        configNode->GetNode("KEY_MAP")->CopyTo(*node->GetNode("KEY_MAP"), true);
        configNode->RemoveNode("KEY_MAP");
    }
    configNode->CopyTo(*node, true);
    return node;
}

// kspcore split of GameSettings.cs:1619 (the ConfigNode WriteCfg builds; WriteCfg then saves it)
ConfigNode::NodePtr GameSettings::BuildCfg() {
    SetCollapsedPawGroupString();
    ConfigNode::NodePtr configNode = ConfigNode::Create();
    for (const SettingsField& f : Fields()) {
        if (f.isConfigNode()) {
            bool present = false;
            switch (f.kind) {
                case SettingsField::Kind::KeyBinding: present = static_cast<bool>(*static_cast<std::shared_ptr<KeyBinding>*>(f.address)); break;
                case SettingsField::Kind::AxisBinding: present = static_cast<bool>(*static_cast<std::shared_ptr<AxisBinding>*>(f.address)); break;
                case SettingsField::Kind::AxisKeyBindingList:
                    present = static_cast<bool>(*static_cast<std::shared_ptr<AxisKeyBindingList>*>(f.address));
                    break;
                case SettingsField::Kind::InputDevices: present = static_cast<bool>(*static_cast<std::shared_ptr<InputDevices>*>(f.address)); break;
                case SettingsField::Kind::TrackIR: present = static_cast<bool>(*static_cast<std::shared_ptr<TrackIRSettings>*>(f.address)); break;
                default: break;
            }
            if (present) {
                ConfigNode* n = configNode->AddNode(f.name);
                switch (f.kind) {
                    case SettingsField::Kind::KeyBinding: (*static_cast<std::shared_ptr<KeyBinding>*>(f.address))->Save(*n); break;
                    case SettingsField::Kind::AxisBinding: (*static_cast<std::shared_ptr<AxisBinding>*>(f.address))->Save(*n); break;
                    case SettingsField::Kind::AxisKeyBindingList: (*static_cast<std::shared_ptr<AxisKeyBindingList>*>(f.address))->Save(*n); break;
                    case SettingsField::Kind::InputDevices: (*static_cast<std::shared_ptr<InputDevices>*>(f.address))->Save(*n); break;
                    case SettingsField::Kind::TrackIR: (*static_cast<std::shared_ptr<TrackIRSettings>*>(f.address))->Save(*n); break;
                    default: break;
                }
                continue;
            }
        }
        configNode->AddValue(f.name, ConvertToString(f));
    }
    if (host.savePresetList) host.savePresetList(*configNode);  // PQSCache.SavePresetList(configNode)
    return configNode;
}

// GameSettings.cs:1619
bool GameSettings::WriteCfg() {
    ConfigNode::NodePtr configNode = BuildCfg();
    bool saved = true;
    if (!configFilePath.empty()) saved = configNode->Save(configFilePath, std::string("KSP Game Settings"));
    events::GameEvents::OnGameSettingsWritten.Fire();
    return saved;
}

// GameSettings.cs:1691
GameSettings::ParseResult GameSettings::ParseCfg(const ConfigNode::NodePtr& configNode) {
    if (!configNode) {
        log::Warning("GameSettings: Cannot find settings");
        return ParseResult::NoFile;
    }
    bool flag = false;
    bool flag2 = false;
    if (configNode->HasValue("SETTINGS_FILE_VERSION")) {
        if (KSPUtil::CheckVersion(*configNode->GetValue("SETTINGS_FILE_VERSION"), 1, 1, 0) != VersionCompareResult::COMPATIBLE) {
            log::Warning("[GAME SETTINGS]: WARNING - settings.cfg file is incompatible with this version");
            return ParseResult::Incompatible;
        }
        SETTINGS_FILE_VERSION = *configNode->GetValue("SETTINGS_FILE_VERSION");
        if (KSPUtil::CheckVersion(*configNode->GetValue("SETTINGS_FILE_VERSION"), 1, 2, 0) != VersionCompareResult::COMPATIBLE) flag = true;
        if (KSPUtil::CheckVersion(*configNode->GetValue("SETTINGS_FILE_VERSION"), 1, 3, 0) != VersionCompareResult::COMPATIBLE) flag2 = true;
        SETTINGS_FILE_VERSION = "1.3.0";
        if (!configNode->HasValue("SHOW_VERSION_WATERMARK")) SHOW_VERSION_WATERMARK = false;
        SHOW_ANALYTICS_DIALOG_SettingExists = configNode->HasValue("SHOW_ANALYTICS_DIALOG");
        SHOW_WHATSNEW_DIALOG_SettingExists = configNode->HasValue("SHOW_WHATSNEW_DIALOG");
        int i = 0;
        for (int count = configNode->nodes().Count(); i < count; i++) {
            const ConfigNode::NodePtr& configNode2 = configNode->nodes().Shared(i);
            const SettingsField* field = FindField(configNode2->name);
            if (field != nullptr) LoadIntoConfigNodeField(*field, configNode2);
        }
        for (const ConfigNode::Value& value : configNode->values()) {
            if (value.name == "SETTINGS_FILE_VERSION") continue;
            const SettingsField* field2 = FindField(value.name);
            if (field2 == nullptr) continue;
            switch (field2->kind) {
                case SettingsField::Kind::String: *static_cast<std::string*>(field2->address) = value.value; break;
                case SettingsField::Kind::Int: *static_cast<int*>(field2->address) = ParseInt(value.value); break;
                case SettingsField::Kind::Float: *static_cast<float*>(field2->address) = ParseFloatInvariant(value.value); break;
                case SettingsField::Kind::Bool: *static_cast<bool*>(field2->address) = ParseBool(value.value); break;
                case SettingsField::Kind::Double: *static_cast<double*>(field2->address) = dotnet::ParseDouble(value.value); break;
                case SettingsField::Kind::ValidatorMode:
                    *static_cast<Expansions::Missions::Editor::ValidatorMode*>(field2->address) =
                        dotnet::EnumParseFull<Expansions::Missions::Editor::ValidatorMode>(value.value);
                    break;
                default: break;  // KeyBinding & co. are nodes; ORBIT_WARP_MAXRATE_MODE / List<float>: no branch in the C#
            }
        }
        if (host.loadPresetList) host.loadPresetList(*configNode);  // PQSCache.LoadPresetList(configNode)
        InitializeDefaultValues();
        bool flag3 = false;
        if (flag) {
            SHADOWS_DEFAULT_PROJECTION = 1;
            SHADOWS_FLIGHT_PROJECTION = 1;
            SHADOWS_KSC_PROJECTION = 1;
            SHADOWS_TRACKING_PROJECTION = 1;
            SHADOWS_EDITORS_PROJECTION = 1;
            flag3 = true;
        }
        if (flag2) {
            SHOW_VESSEL_NAMING_IN_FLIGHT = true;
            flag3 = true;
        }
        if (flag3) WriteCfg();
        return ParseResult::Ok;
    }
    log::Warning("[GAME SETTINGS]: WARNING - settings.cfg file is incompatible with this version");
    return ParseResult::Incompatible;
}

// kspcore split of GameSettings.cs:1691 (ParseCfg of ConfigNode.Load(configFilePath))
GameSettings::ParseResult GameSettings::ParseCfgFile() { return ParseCfg(ConfigNode::Load(configFilePath)); }

// GameSettings.cs:1823
void GameSettings::InitializeDefaultValues() {
    SetCollapsedPawGroupNames();
    SetHighlighterColors();
    SetUITextColors();
    SetRDSearchHighlightColors();
    highlighterColors.HighlighterLimit = PART_HIGHLIGHTER_BRIGHTNESSFACTOR;
    SetCPVisualColors();
    InitializeAxisGroupbindings();
    SetLightPresetColors();
    SetFireworkPresetColors();
}

// GameSettings.cs:1836
void GameSettings::SetCollapsedPawGroupNames() {
    collpasedPAWGroups.clear();
    const std::vector<std::string> array = dotnet::Split(PAW_COLLAPSED_GROUP_NAMES, ",", false);
    for (const std::string& s : array)
        if (!s.empty()) collpasedPAWGroups.push_back(s);
}

// GameSettings.cs:1849
void GameSettings::SetCollapsedPawGroupString() {
    std::string joined;
    for (size_t i = 0; i < collpasedPAWGroups.size(); ++i) {
        if (i != 0) joined += ",";
        joined += collpasedPAWGroups[i];
    }
    PAW_COLLAPSED_GROUP_NAMES = joined;
}

// GameSettings.cs:1854
void GameSettings::SetHighlighterColors() {
    HighlighterColors& h = highlighterColors;
    SetHighlighterColor(h.colorPartHighlightDefault, COLOR_PART_HIGHLIGHT, "COLOR_PART_HIGHLIGHT");
    SetHighlighterColor(h.colorPartEditorAttached, COLOR_PART_EDITORATTACHED, "COLOR_PART_EDITORATTACHED");
    SetHighlighterColor(h.colorPartEditorDetached, COLOR_PART_EDITORDETACHED, "COLOR_PART_EDITORDETACHED");
    SetHighlighterColor(h.colorPartEditorActionSelected, COLOR_PART_ACTIONGROUP_SELECTED, "COLOR_PART_ACTIONGROUP_SELECTED");
    SetHighlighterColor(h.colorPartEditorActionHighlight, COLOR_PART_ACTIONGROUP_HIGHLIGHT, "COLOR_PART_ACTIONGROUP_HIGHLIGHT");
    SetHighlighterColor(h.colorPartRootToolHighlight, COLOR_PART_ROOTTOOL_HIGHLIGHT, "COLOR_PART_ROOTTOOL_HIGHLIGHT");
    SetHighlighterColor(h.colorPartRootToolHighlightEdge, COLOR_PART_ROOTTOOL_HIGHLIGHTEDGE, "COLOR_PART_ROOTTOOL_HIGHLIGHTEDGE");
    SetHighlighterColor(h.colorPartRootToolHover, COLOR_PART_ROOTTOOL_HOVER, "COLOR_PART_ROOTTOOL_HOVER");
    SetHighlighterColor(h.colorPartRootToolHoverEdge, COLOR_PART_ROOTTOOL_HOVEREDGE, "COLOR_PART_ROOTTOOL_HOVEREDGE");
    SetHighlighterColor(h.colorPartEngineerAppHighlight, COLOR_PART_ENGINEERAPP_HIGHLIGHT, "COLOR_PART_ENGINEERAPP_HIGHLIGHT");
    SetHighlighterColor(h.colorPartTransferSourceHighlight, COLOR_PART_TRANSFER_SOURCE_HIGHLIGHT, "COLOR_PART_TRANSFER_SOURCE_HIGHLIGHT");
    SetHighlighterColor(h.colorPartTransferSourceHover, COLOR_PART_TRANSFER_SOURCE_HOVER, "COLOR_PART_TRANSFER_SOURCE_HOVER");
    SetHighlighterColor(h.colorPartTransferDestHighlight, COLOR_PART_TRANSFER_DEST_HIGHLIGHT, "COLOR_PART_TRANSFER_DEST_HIGHLIGHT");
    SetHighlighterColor(h.colorPartTransferDestHover, COLOR_PART_TRANSFER_DEST_HOVER, "COLOR_PART_TRANSFER_DEST_HOVER");
    SetHighlighterColor(h.colorPartInventoryContainer, COLOR_PART_INVENTORY_CONTAINER, "COLOR_PART_INVENTORY_CONTAINER");
    SetHighlighterColor(h.colorPartInventoryUnavailableSpace, COLOR_PART_INVENTORY_NOSPACE, "COLOR_PART_INVENTORY_NOSPACE");
    SetHighlighterColor(h.colorPartConstructionValid, COLOR_PART_CONSTRUCTION_VALID, "COLOR_PART_CONSTRUCTION_VALID");
}

// GameSettings.cs:1875
void GameSettings::SetLightPresetColors() {
    lightPresetColors.clear();
    lightPresetColors.push_back(LoadColor(COLOR_LIGHT_PRESET_1, "COLOR_LIGHT_PRESET_1"));
    lightPresetColors.push_back(LoadColor(COLOR_LIGHT_PRESET_2, "COLOR_LIGHT_PRESET_2"));
    lightPresetColors.push_back(LoadColor(COLOR_LIGHT_PRESET_3, "COLOR_LIGHT_PRESET_3"));
    lightPresetColors.push_back(LoadColor(COLOR_LIGHT_PRESET_4, "COLOR_LIGHT_PRESET_4"));
    lightPresetColors.push_back(LoadColor(COLOR_LIGHT_PRESET_5, "COLOR_LIGHT_PRESET_5"));
}

// GameSettings.cs:1885
void GameSettings::SetFireworkPresetColors() {
    fireworkPresetColors.clear();
    fireworkPresetColors.push_back(LoadColor(COLOR_FIREWORK_PRESET_GREEN, "COLOR_FIREWORK_PRESET_GREEN"));
    fireworkPresetColors.push_back(LoadColor(COLOR_FIREWORK_PRESET_LIGHT_BLUE, "COLOR_FIREWORK_PRESET_LIGHT_BLUE"));
    fireworkPresetColors.push_back(LoadColor(COLOR_FIREWORK_PRESET_BLUE, "COLOR_FIREWORK_PRESET_BLUE"));
    fireworkPresetColors.push_back(LoadColor(COLOR_FIREWORK_PRESET_PURPLE, "COLOR_FIREWORK_PRESET_PURPLE"));
    fireworkPresetColors.push_back(LoadColor(COLOR_FIREWORK_PRESET_PINK, "COLOR_FIREWORK_PRESET_PINK"));
}

// GameSettings.cs:1895
Color GameSettings::LoadColor(const std::string& colorValues, const std::string& name) {
    Color color;
    if (!ConfigNode::CheckAndParseColor(colorValues, color)) log::Warning("Invalid color supplied for " + name + ": " + colorValues);
    return color;
}

// GameSettings.cs:1904
const std::vector<Color>& GameSettings::GetLightPresetColors() { return lightPresetColors; }

// GameSettings.cs:1909
const std::vector<Color>& GameSettings::GetFireworkPresetColors() { return fireworkPresetColors; }

// GameSettings.cs:1914
void GameSettings::SetUITextColors() {
    SetHighlighterColor(highlighterColors.inactiveTextColor, UI_COLOR_INACTIVE_TEXT, "UI_COLOR_DEFAULT_TEXT");
    SetHighlighterColor(highlighterColors.activeTextColor, UI_COLOR_ACTIVE_TEXT, "UI_COLOR_HIGHLIGHT_TEXT");
    SetHighlighterColor(highlighterColors.inactiveTextMiniSettingsColor, UI_COLOR_INACTIVE_MINISETTNIGS_TEXT, "UI_COLOR_HIGHLIGHT_GREEN_TEXT");
}

// GameSettings.cs:1921
void GameSettings::SetRDSearchHighlightColors() {
    SetHighlighterColor(highlighterColors.searchSelectionNodeColor, COLOR_RD_SEARCH_NODE_HIGHLIGHT, "COLOR_RD_SEARCH_NODE_HIGHLIGHT");
    SetHighlighterColor(highlighterColors.searchSelectionPartColor, COLOR_RD_SEARCH_PART_HIGHLIGHT, "COLOR_RD_SEARCH_PART_HIGHLIGHT");
}

// GameSettings.cs:1927
bool GameSettings::SetHighlighterColor(Color& c, const std::string& colorValues, const std::string& name) {
    Color color;
    if (ConfigNode::CheckAndParseColor(colorValues, color)) {
        c = color;
        return true;
    }
    log::Warning("Invalid color supplied for " + name + ": " + colorValues);
    return false;
}

// GameSettings.cs:1938
void GameSettings::SetCPVisualColors() {
    SetHighlighterColor(COLOR_CONTROLPOINT_COLOR_FORWARD, CONTROLPOINT_COLOR_FORWARD, "COLOR_CONTROLPOINT_COLOR_FORWARD");
    SetHighlighterColor(COLOR_CONTROLPOINT_COLOR_UP, CONTROLPOINT_COLOR_UP, "COLOR_CONTROLPOINT_COLOR_UP");
    SetHighlighterColor(COLOR_CONTROLPOINT_COLOR_RIGHT, CONTROLPOINT_COLOR_RIGHT, "COLOR_CONTROLPOINT_COLOR_RIGHT");
}

// GameSettings.cs:1945 (float.TryParse(string, out result): current culture en-US)
void GameSettings::InitializeAxisGroupbindings() {
    AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES = std::make_shared<std::vector<float>>();
    const std::vector<std::string> array = dotnet::Split(AXIS_INCREMENTAL_SPEED_MULTIPLIER_STORAGE, ",", false);
    float result = 0.0f;
    for (const std::string& s : array) {
        if (dotnet::TryParseSingle(s, result)) AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES->push_back(result);
    }
}

// GameSettings.cs:1963
std::vector<float>* GameSettings::GetAxisIncrementalSpeedMultiplierValues() { return AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES.get(); }

// GameSettings.cs:1968
void GameSettings::AddIncrementalSpeedMultiplierValue(float value) {
    if (AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES) {
        AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES->push_back(value);
        return;
    }
    AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES = std::make_shared<std::vector<float>>();
    AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES->push_back(value);
}

// GameSettings.cs:1979
float GameSettings::GetAxisSpeedMultiplier(int index) {
    if (AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES && index < static_cast<int>(AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES->size())) {
        if (index < 0) throw dotnet::ArgumentOutOfRangeException("Index was out of range.");
        return (*AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES)[static_cast<size_t>(index)];
    }
    return AXIS_INCREMENTAL_SPEED_MULTIPLIER_DEFAULT;
}

// GameSettings.cs:1989
int GameSettings::GetAxiSpeedMultiplierIndex(float value) {
    if (!AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES) {
        InitializeAxisGroupbindings();
        return GetAxiSpeedMultiplierIndex(value);
    }
    std::vector<float>& list = *AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES;
    auto contains = [&](float v) { return std::any_of(list.begin(), list.end(), [&](float x) { return FloatEquals(x, v); }); };
    auto indexOf = [&](float v) {
        for (size_t i = 0; i < list.size(); ++i)
            if (FloatEquals(list[i], v)) return static_cast<int>(i);
        return -1;
    };
    if (!contains(value)) {
        if (!contains(AXIS_INCREMENTAL_SPEED_MULTIPLIER_DEFAULT)) {
            list.push_back(AXIS_INCREMENTAL_SPEED_MULTIPLIER_DEFAULT);
            // List<float>.Sort(): float.CompareTo (NaN sorts first)
            std::stable_sort(list.begin(), list.end(), [](float a, float b) {
                if (std::isnan(a)) return !std::isnan(b);
                if (std::isnan(b)) return false;
                return a < b;
            });
        }
        return indexOf(value);
    }
    return indexOf(value);
}

// GameSettings.cs:2008 (QualitySettings / Screen / UIMasterController / AudioListener / HighlightingSystem: the host's)
void GameSettings::ApplySettings() {
    if (host.applySettings) host.applySettings();
}

// GameSettings.cs:2055
void GameSettings::OnGameSave() {
    if (saveOnGameSave) {
        saveOnGameSave = false;
        SaveGameSettingsOnly();
    }
}

// GameSettings.cs:2064
void GameSettings::SaveSettingsOnNextGameSave() { saveOnGameSave = true; }

}  // namespace ksp
