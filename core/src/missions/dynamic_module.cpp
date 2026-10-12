// WP-54: DynamicModule.cs, DynamicModuleList.cs. See dynamic_module.hpp.
#include "ksp/missions/dynamic_module.hpp"

#include <algorithm>
#include <atomic>
#include <map>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission_node.hpp"

namespace ksp::Expansions::Missions {

namespace {
struct RegistryData {
    std::map<std::string, DynamicModuleType> types;
    std::vector<std::string> order;
};
RegistryData& Data() {
    static RegistryData d;
    return d;
}
std::atomic<uint64_t> g_objectIds{0};
}  // namespace

// ---- registry ----------------------------------------------------------------------------------------------------------------

bool DynamicModuleType::IsSubclassOf(const std::string& baseName) const { return std::find(bases.begin(), bases.end(), baseName) != bases.end(); }

void DynamicModuleRegistry::Register(DynamicModuleType type) {
    RegistryData& d = Data();
    if (d.types.find(type.name) == d.types.end()) d.order.push_back(type.name);
    d.types[type.name] = std::move(type);
}

// AssemblyLoader.GetClassByName(typeof(DynamicModule), name)
const DynamicModuleType* DynamicModuleRegistry::GetClassByName(const std::string& name) {
    auto it = Data().types.find(name);
    return it == Data().types.end() ? nullptr : &it->second;
}

std::vector<std::string> DynamicModuleRegistry::SubclassesOf(const std::string& baseName) {
    std::vector<std::string> out;
    for (const std::string& n : Data().order)
        if (Data().types[n].IsSubclassOf(baseName)) out.push_back(n);
    return out;
}

void DynamicModuleRegistry::ResetForTests() {
    Data().types.clear();
    Data().order.clear();
}

// ---- DynamicModule -----------------------------------------------------------------------------------------------------------

// ObjectIDGenerator.GetId(this, out firstTime): ids are handed out 1, 2, 3 ... per newly seen object
uint64_t DynamicModule::NextObjectId() { return ++g_objectIds; }
void DynamicModule::ResetObjectIds() { g_objectIds = 0; }

// DynamicModule.cs:37
std::string DynamicModule::GetModuleInstanceID() { return dotnet::FormatUInt64(NextObjectId()); }

// DynamicModule.cs:24
DynamicModule::DynamicModule(std::string typeName) : name_(std::move(typeName)) {
    id_ = GetModuleInstanceID();
}

// DynamicModule.cs:31
DynamicModule::DynamicModule(std::string typeName, MENode* n) : DynamicModule(std::move(typeName)) { node = n; }

// DynamicModule.cs:43
std::string DynamicModule::GetDisplayName() { return GetName(); }

// DynamicModule.cs:48
std::string DynamicModule::GetDisplayToolTip() { return ""; }

// DynamicModule.cs:53
std::vector<std::string> DynamicModule::GetDefaultPinnedParameters() { return {}; }

// DynamicModule.cs:58
bool DynamicModule::Equals(const DynamicModule& other) const { return name_ == other.name_; }

// DynamicModule.cs:72
std::string DynamicModule::GetName() { return name_ + "_" + id_; }

// DynamicModule.cs:77
void DynamicModule::AddParameterToSAP(const std::string& parameter) {
    if (std::find(parametersDisplayedInSAP.begin(), parametersDisplayedInSAP.end(), parameter) == parametersDisplayedInSAP.end())
        parametersDisplayedInSAP.push_back(parameter);
}

// DynamicModule.cs:82
void DynamicModule::RemoveParameterFromSAP(const std::string& parameter) {
    auto it = std::find(parametersDisplayedInSAP.begin(), parametersDisplayedInSAP.end(), parameter);
    if (it != parametersDisplayedInSAP.end()) parametersDisplayedInSAP.erase(it);
    if (node != nullptr && node->HasNodeBodyParameter(GetName(), parameter)) {
        node->RemoveParameterFromNodeBody(GetName(), parameter);
        UpdateNodeBodyUI();
    }
}

// DynamicModule.cs:92
void DynamicModule::AddParameterToNodeBody(const std::string& parameter) {
    if (node != nullptr) node->AddParameterToNodeBody(GetName(), parameter);
}

// DynamicModule.cs:100
void DynamicModule::AddParameterToNodeBodyAndUpdateUI(const std::string& parameter) {
    if (node != nullptr) {
        node->AddParameterToNodeBody(GetName(), parameter);
        UpdateNodeBodyUI();
    }
}

// DynamicModule.cs:109
void DynamicModule::RemoveParameterFromNodeBody(const std::string& parameter) {
    if (node != nullptr) node->RemoveParameterFromNodeBody(GetName(), parameter);
}

// DynamicModule.cs:117
void DynamicModule::RemoveParameterFromNodeBodyAndUpdateUI(const std::string& parameter) {
    if (node != nullptr) {
        node->RemoveParameterFromNodeBody(GetName(), parameter);
        UpdateNodeBodyUI();
    }
}

// DynamicModule.cs:126
bool DynamicModule::HasNodeBodyParameter(const std::string& parameter) {
    if (node != nullptr) return node->HasNodeBodyParameter(GetName(), parameter);
    return false;
}

// DynamicModule.cs:135
bool DynamicModule::HasSAPParameter(const std::string& parameter) {
    return std::find(parametersDisplayedInSAP.begin(), parametersDisplayedInSAP.end(), parameter) != parametersDisplayedInSAP.end();
}

// DynamicModule.cs:140
std::string DynamicModule::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    std::string text = "None";
    if (field.hasValue) text = field.value;
    return KSP::Localization::Localizer::Format("#autoLOC_8004190", {field.guiName, text});
}

// DynamicModule.cs:150
void DynamicModule::UpdateNodeBodyUI() {
    if (node != nullptr) node->guiNode->DisplayNodeBodyParameters();
}

// DynamicModule.cs:158
std::vector<IMENodeDisplay*> DynamicModule::GetInternalParametersToDisplay() { return {}; }

// DynamicModule.cs:163
MENode* DynamicModule::GetNode() { return node; }

// DynamicModule.cs:168
std::string DynamicModule::GetInfo() { return name_; }

// DynamicModule.cs:173
void DynamicModule::SetNode(MENode* n) { node = n; }

// DynamicModule.cs:178
void DynamicModule::Initialize() {}

// DynamicModule.cs:182
void DynamicModule::Destroy() {}

// DynamicModule.cs:186
void DynamicModule::ParameterSetupComplete() {}

// DynamicModule.cs:190
void DynamicModule::Load(const ConfigNode& cfg) {
    std::string value;
    if (cfg.TryGetValue("id", value)) id_ = value;
}

// DynamicModule.cs:199
void DynamicModule::Save(ConfigNode& cfg) const {
    cfg.AddValue("name", name_);
    cfg.AddValue("id", id_);
}

// ---- DynamicModuleList -------------------------------------------------------------------------------------------------------

// DynamicModuleList.cs:19
DynamicModuleList::DynamicModuleList(MENode* n) : node_(n) {}

// DynamicModuleList.cs:26
std::vector<std::string> DynamicModuleList::GetSupportedTypes() { return supportedTypes; }

// DynamicModuleList.cs:31
std::vector<IMENodeDisplay*> DynamicModuleList::GetInternalParameters() {
    std::vector<IMENodeDisplay*> list;
    for (size_t i = 0; i < activeModules.size(); ++i) list.push_back(activeModules[i].get());
    return list;
}

// DynamicModuleList.cs:42
bool DynamicModuleList::ContainsModule(const std::string& typeName) const {
    int count = static_cast<int>(activeModules.size());
    do {
        if (count-- <= 0) return false;
    } while (!(activeModules[static_cast<size_t>(count)]->TypeName() == typeName));
    return true;
}

// DynamicModuleList.cs:56
void DynamicModuleList::AddDynamicModule(const ConfigNode& moduleNode) {
    std::string value;
    if (!moduleNode.TryGetValue("name", value)) return;
    const DynamicModuleType* classByName = DynamicModuleRegistry::GetClassByName(value);
    if (classByName != nullptr) {
        std::unique_ptr<DynamicModule> dynamicModule = classByName->create(node_);
        dynamicModule->SetNode(node_);
        dynamicModule->Load(moduleNode);
        activeModules.push_back(std::move(dynamicModule));
    } else {
        log::Info("Error " + value + " type was not found in AssemblyLoader");
    }
}

// DynamicModuleList.cs:80
void DynamicModuleList::SetSupportedTypes(std::vector<std::string> supportedModuleTypes) { supportedTypes = std::move(supportedModuleTypes); }

// DynamicModuleList.cs:85
bool DynamicModuleList::Equals(const DynamicModuleList& other) const {
    if (activeModules.size() != other.activeModules.size()) return false;
    int count = static_cast<int>(activeModules.size());
    do {
        if (count-- <= 0) return true;
    } while (activeModules[static_cast<size_t>(count)]->EqualsObject(*other.activeModules[static_cast<size_t>(count)]));
    return false;
}

// DynamicModuleList.cs:112
void DynamicModuleList::Load(const ConfigNode& cfg) {
    if (cfg.HasNode(rootNodeName)) {
        const std::vector<ConfigNode*> nodes = cfg.GetNode(rootNodeName)->GetNodes(moduleNodeName);
        activeModules.clear();
        for (const ConfigNode* moduleNode : nodes) AddDynamicModule(*moduleNode);
    }
}

// DynamicModuleList.cs:126
void DynamicModuleList::Save(ConfigNode& cfg) const {
    ConfigNode* configNode = cfg.AddNode(rootNodeName);
    for (size_t i = 0; i < activeModules.size(); ++i) {
        ConfigNode* configNode2 = configNode->AddNode(moduleNodeName);
        activeModules[i]->Save(*configNode2);
    }
}

}  // namespace ksp::Expansions::Missions
