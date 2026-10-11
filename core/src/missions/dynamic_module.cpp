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

// DynamicModule.cs:21
DynamicModule::DynamicModule(std::string typeName) : name_(std::move(typeName)) {
    id_ = GetModuleInstanceID();
}

// DynamicModule.cs:28
DynamicModule::DynamicModule(std::string typeName, MENode* n) : DynamicModule(std::move(typeName)) { node = n; }

// DynamicModule.cs:44
std::string DynamicModule::GetDisplayName() { return GetName(); }

// DynamicModule.cs:49
std::string DynamicModule::GetDisplayToolTip() { return ""; }

// DynamicModule.cs:54
std::vector<std::string> DynamicModule::GetDefaultPinnedParameters() { return {}; }

// DynamicModule.cs:59
bool DynamicModule::Equals(const DynamicModule& other) const { return name_ == other.name_; }

// DynamicModule.cs:74
std::string DynamicModule::GetName() { return name_ + "_" + id_; }

// DynamicModule.cs:79
void DynamicModule::AddParameterToSAP(const std::string& parameter) {
    if (std::find(parametersDisplayedInSAP.begin(), parametersDisplayedInSAP.end(), parameter) == parametersDisplayedInSAP.end())
        parametersDisplayedInSAP.push_back(parameter);
}

// DynamicModule.cs:84
void DynamicModule::RemoveParameterFromSAP(const std::string& parameter) {
    auto it = std::find(parametersDisplayedInSAP.begin(), parametersDisplayedInSAP.end(), parameter);
    if (it != parametersDisplayedInSAP.end()) parametersDisplayedInSAP.erase(it);
    if (node != nullptr && node->HasNodeBodyParameter(GetName(), parameter)) {
        node->RemoveParameterFromNodeBody(GetName(), parameter);
        UpdateNodeBodyUI();
    }
}

// DynamicModule.cs:94
void DynamicModule::AddParameterToNodeBody(const std::string& parameter) {
    if (node != nullptr) node->AddParameterToNodeBody(GetName(), parameter);
}

// DynamicModule.cs:102
void DynamicModule::AddParameterToNodeBodyAndUpdateUI(const std::string& parameter) {
    if (node != nullptr) {
        node->AddParameterToNodeBody(GetName(), parameter);
        UpdateNodeBodyUI();
    }
}

// DynamicModule.cs:111
void DynamicModule::RemoveParameterFromNodeBody(const std::string& parameter) {
    if (node != nullptr) node->RemoveParameterFromNodeBody(GetName(), parameter);
}

// DynamicModule.cs:119
void DynamicModule::RemoveParameterFromNodeBodyAndUpdateUI(const std::string& parameter) {
    if (node != nullptr) {
        node->RemoveParameterFromNodeBody(GetName(), parameter);
        UpdateNodeBodyUI();
    }
}

// DynamicModule.cs:128
bool DynamicModule::HasNodeBodyParameter(const std::string& parameter) {
    if (node != nullptr) return node->HasNodeBodyParameter(GetName(), parameter);
    return false;
}

// DynamicModule.cs:137
bool DynamicModule::HasSAPParameter(const std::string& parameter) {
    return std::find(parametersDisplayedInSAP.begin(), parametersDisplayedInSAP.end(), parameter) != parametersDisplayedInSAP.end();
}

// DynamicModule.cs:142
std::string DynamicModule::GetNodeBodyParameterString(const Editor::BaseAPField&) { return ""; }

// DynamicModule.cs:152
void DynamicModule::UpdateNodeBodyUI() {
    if (node != nullptr) node->guiNode->DisplayNodeBodyParameters();
}

// DynamicModule.cs:160
std::vector<IMENodeDisplay*> DynamicModule::GetInternalParametersToDisplay() { return {}; }

// DynamicModule.cs:165
MENode* DynamicModule::GetNode() { return node; }

// DynamicModule.cs:170
std::string DynamicModule::GetInfo() { return name_; }

// DynamicModule.cs:175
void DynamicModule::SetNode(MENode* n) { node = n; }

// DynamicModule.cs:180
void DynamicModule::Initialize() {}

// DynamicModule.cs:184
void DynamicModule::Destroy() {}

// DynamicModule.cs:188
void DynamicModule::ParameterSetupComplete() {}

// DynamicModule.cs:192
void DynamicModule::Load(const ConfigNode& cfg) {
    std::string value;
    if (cfg.TryGetValue("id", value)) id_ = value;
}

// DynamicModule.cs:201
void DynamicModule::Save(ConfigNode& cfg) const {
    cfg.AddValue("name", name_);
    cfg.AddValue("id", id_);
}

// ---- DynamicModuleList -------------------------------------------------------------------------------------------------------

// DynamicModuleList.cs:23
DynamicModuleList::DynamicModuleList(MENode* n) : node_(n) {}

// DynamicModuleList.cs:30
std::vector<std::string> DynamicModuleList::GetSupportedTypes() { return supportedTypes; }

// DynamicModuleList.cs:35
std::vector<IMENodeDisplay*> DynamicModuleList::GetInternalParameters() {
    std::vector<IMENodeDisplay*> list;
    for (size_t i = 0; i < activeModules.size(); ++i) list.push_back(activeModules[i].get());
    return list;
}

// DynamicModuleList.cs:46
bool DynamicModuleList::ContainsModule(const std::string& typeName) const {
    int count = static_cast<int>(activeModules.size());
    do {
        if (count-- <= 0) return false;
    } while (!(activeModules[static_cast<size_t>(count)]->TypeName() == typeName));
    return true;
}

// DynamicModuleList.cs:60
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

// DynamicModuleList.cs:84
void DynamicModuleList::SetSupportedTypes(std::vector<std::string> supportedModuleTypes) { supportedTypes = std::move(supportedModuleTypes); }

// DynamicModuleList.cs:89
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
