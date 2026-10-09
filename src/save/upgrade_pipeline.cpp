// SaveUpgradePipeline (framework + every stock upgrade script) and KSPUpgradePipeline; see
// include/ksp/save/upgrade_pipeline.hpp.
#include "ksp/save/upgrade_pipeline.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <stdexcept>

#include "ksp/config/kspparseable.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/util/ksp_util.hpp"
#include "save_util.hpp"

namespace ksp::dotnet {
template <>
struct EnumTraits<ksp::SaveUpgradePipeline::v110_WheelModuleOverhaul::OldGearDeploymentStates> {
    using E = ksp::SaveUpgradePipeline::v110_WheelModuleOverhaul::OldGearDeploymentStates;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::RETRACTED, "RETRACTED"}, {E::DEPLOYED, "DEPLOYED"},
                                                {E::DEPLOYING, "DEPLOYING"}, {E::RETRACTING, "RETRACTING"},
                                                {E::UNDEFINED, "UNDEFINED"}};
};
template <>
struct EnumTraits<ksp::SaveUpgradePipeline::v110_WheelModuleOverhaul::OldLegDeploymentStates> {
    using E = ksp::SaveUpgradePipeline::v110_WheelModuleOverhaul::OldLegDeploymentStates;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::RETRACTED, "RETRACTED"}, {E::RETRACTING, "RETRACTING"},
                                                {E::DEPLOYING, "DEPLOYING"}, {E::DEPLOYED, "DEPLOYED"},
                                                {E::BROKEN, "BROKEN"},       {E::REPAIRING, "REPAIRING"}};
};
}  // namespace ksp::dotnet

namespace ksp::SaveUpgradePipeline {

namespace {
std::string Req(const std::optional<std::string>& s, const char* what) {
    if (!s) throw NullReferenceException(std::string("SaveUpgradePipeline: ") + what + " is null");
    return *s;
}
}  // namespace

std::string ToString(LoadContext c) {
    switch (c) {
    case LoadContext::None: return "None";
    case LoadContext::SFS: return "SFS";
    case LoadContext::Craft: return "Craft";
    case LoadContext::Any: return "Any";
    default: break;
    }
    if (c == (LoadContext::SFS | LoadContext::Craft)) return "SFS, Craft";
    return dotnet::FormatInt64(static_cast<int>(c));
}

std::string ToString(TestResult r) {
    switch (r) {
    case TestResult::Failed: return "Failed";
    case TestResult::TooEarly: return "TooEarly";
    case TestResult::Upgradeable: return "Upgradeable";
    default: return "Pass";
    }
}

// LogEntry.cs:17
std::string LogEntry::ToString() const {
    const std::string tr = ::ksp::SaveUpgradePipeline::ToString(testResult);
    switch (testResult) {
    case TestResult::Failed:
        return "<color=red>" + tr + "</color> \t\t| " + (upgraded ? "Upgraded" : "Not Upgraded");
    case TestResult::TooEarly:
        return "<color=yellow>" + tr + "</color> \t\t| " + (upgraded ? "Upgraded" : "Not Upgraded");
    case TestResult::Pass:
        return "<color=green>" + tr + "</color> \t\t| " + (upgraded ? "<color=green>Upgraded</color>" : "Not Upgraded");
    default:
        return tr + " \t| " + (upgraded ? "<color=green>Upgraded</color>" : "<color=yellow>Not Upgraded</color>");
    }
}

// ===================================================== NodeUtil =====================================================

namespace NodeUtil {

// NodeUtil.cs:9
Version GetCfgVersion(const ConfigNode& n, LoadContext loadContext) {
    const ConfigNode* src = &n;
    if (loadContext == LoadContext::SFS) {
        src = n.GetNode("GAME");
        if (src == nullptr) throw NullReferenceException("NodeUtil.GetCfgVersion: no GAME node");
    }
    const std::string* v = src->GetValuePtr("version");
    if (v == nullptr) throw ArgumentNullException("version");
    return Version(*v);
}

// NodeUtil.cs:18
std::optional<std::string> GetPartNodeNameValue(const ConfigNode& node, LoadContext loadContext) {
    switch (loadContext) {
    case LoadContext::Craft: return node.GetValue("part");
    case LoadContext::SFS: return node.GetValue("name");
    default: return std::string();
    }
}

// NodeUtil.cs:31
std::vector<ConfigNode*> GetModuleNodesNamed(const ConfigNode& node, std::string_view moduleName) {
    std::vector<ConfigNode*> list;
    for (ConfigNode* m : node.GetNodes("MODULE")) {
        const std::string* n = m->GetValuePtr("name");
        if (n != nullptr && *n == moduleName) list.push_back(m);
    }
    return list;
}

// NodeUtil.cs:44
void SetPartNodeNameValue(ConfigNode& node, LoadContext loadContext, const std::string& newNameValue) {
    if (loadContext == LoadContext::Craft) node.SetValue("part", newNameValue);
    else if (loadContext == LoadContext::SFS) node.SetValue("name", newNameValue);
}

// NodeUtil.cs:57
std::optional<std::string> GetPartPosition(const ConfigNode& node, LoadContext loadContext) {
    switch (loadContext) {
    case LoadContext::Craft: return node.GetValue("pos");
    case LoadContext::SFS: return node.GetValue("position");
    default: return std::string();
    }
}

// NodeUtil.cs:70
void SetPartPosition(ConfigNode& node, LoadContext loadContext, const Vector3f& newPos) {
    if (loadContext == LoadContext::Craft) node.SetValue("pos", KSPUtil::WriteVector(newPos));
    else if (loadContext == LoadContext::SFS) node.SetValue("position", KSPUtil::WriteVector(newPos));
}

// NodeUtil.cs:83
std::optional<std::string> GetPartRotation(const ConfigNode& node, LoadContext loadContext) {
    switch (loadContext) {
    case LoadContext::Craft: return node.GetValue("rot");
    case LoadContext::SFS: return node.GetValue("rotation");
    default: return std::string();
    }
}

// NodeUtil.cs:96 (ConfigNode.SetValue(string, Quaternion): ConfigNode.WriteQuaternion)
void SetPartRotation(ConfigNode& node, LoadContext loadContext, const Quaternionf& newRot) {
    if (loadContext == LoadContext::Craft) node.SetValue("rot", newRot);
    else if (loadContext == LoadContext::SFS) node.SetValue("rotation", newRot);
}

}  // namespace NodeUtil

// =================================================== UpgradeScript ==================================================

// UpgradeScript.cs:25
void UpgradeScript::Init(LoadContext contextMask, std::optional<std::string> nodeUrlCraft_,
                         std::optional<std::string> nodeUrlSFS_) {
    nodeUrlCraft = std::move(nodeUrlCraft_);
    nodeUrlSFS = std::move(nodeUrlSFS_);
    ContextMask = contextMask;
    OnInit();
}

// UpgradeScript.cs:46
TestResult UpgradeScript::Test(ConfigNode& n, LoadContext loadContext) {
    TestResult tRst = VersionTest(GetCfgNodeVersion(n, loadContext));
    if (tRst != TestResult::Upgradeable) return tRst;
    std::string nodeName;
    const std::optional<std::string> nodeURL = GetNodeURL(loadContext);
    if (!nodeURL || nodeURL->empty()) {
        tRst = OnTest(n, loadContext, nodeName);
        LogTestResults(nodeName, tRst);
        return tRst;
    }
    tRst = TestResult::Pass;
    RecurseNodes(n, dotnet::Split(*nodeURL, "/", false), 0, [&](ConfigNode& node, ConfigNode*) {
        nodeName.clear();
        const TestResult testResult = OnTest(node, loadContext, nodeName);
        LogTestResults(nodeName, testResult);
        switch (testResult) {
        case TestResult::TooEarly:
            throw std::logic_error("InvalidOperationException: Script-Level testing shouldn't return TooEarly. This value "
                                   "is only meaningful for Version testing. Override VersionTest if necessary.");
        case TestResult::Upgradeable:
            if (tRst != TestResult::Pass) break;
            tRst = testResult;
            break;
        case TestResult::Failed: tRst = testResult; break;
        default: break;
        }
    });
    return tRst;
}

// UpgradeScript.cs:86
void UpgradeScript::Upgrade(ConfigNode& n, LoadContext loadContext) {
    std::string nodeName;
    const std::optional<std::string> nodeURL = GetNodeURL(loadContext);
    if (!nodeURL || nodeURL->empty()) {
        if (OnTest(n, loadContext, nodeName) == TestResult::Upgradeable) OnUpgrade(n, loadContext, &n);
    } else {
        RecurseNodes(n, dotnet::Split(*nodeURL, "/", false), 0, [&](ConfigNode& node, ConfigNode* parent) {
            nodeName.clear();
            if (OnTest(node, loadContext, nodeName) == TestResult::Upgradeable) OnUpgrade(node, loadContext, parent);
        });
    }
    n.AddValue("upgraded", Name(), std::optional<std::string>(Description()));
}

// UpgradeScript.cs:127
TestResult UpgradeScript::VersionTest(const Version& v) const {
    if (!CheckMinVersion(v)) {
        log::Warning("[UpgradeScript]: " + Name() + " supports files from " + EarliestCompatibleVersion().ToString() +
                     " and higher only!");
        return TestResult::TooEarly;
    }
    if (!CheckMaxVersion(v)) return TestResult::Pass;
    return TestResult::Upgradeable;
}

// UpgradeScript.cs:141
std::optional<std::string> UpgradeScript::GetNodeURL(LoadContext loadContext) const {
    switch (loadContext) {
    case LoadContext::Craft: return nodeUrlCraft;
    case LoadContext::SFS: return nodeUrlSFS;
    default: return std::nullopt;
    }
}

// UpgradeScript.cs:154
void UpgradeScript::RecurseNodes(ConfigNode& node, const std::vector<std::string>& urlNodes, int level,
                                 const std::function<void(ConfigNode&, ConfigNode*)>& cb, ConfigNode* parent) {
    if (level < static_cast<int>(urlNodes.size())) {
        int num = static_cast<int>(node.GetNodes(urlNodes[static_cast<size_t>(level)]).size());
        while (num-- > 0)
            RecurseNodes(*node.GetNodes(urlNodes[static_cast<size_t>(level)]).at(static_cast<size_t>(num)), urlNodes,
                         level + 1, cb, &node);
    } else {
        cb(node, parent);
    }
}

// UpgradeScript.cs:169
void UpgradeScript::LogTestResults(const std::string& nodeName, TestResult test) const {
    switch (test) {
    case TestResult::Failed: log::Warning("[UpgradeScript]: Module " + Name() + " Failed for " + nodeName + "!"); break;
    case TestResult::Upgradeable:
    case TestResult::Pass: break;
    default: log::Error("[UpgradeScript]: Module " + Name() + " test was undefined  for " + nodeName + "!"); break;
    }
}

// UpgradeScript.cs:178
std::string UpgradeScript::ToString() const {
    return Name() + " [" + EarliestCompatibleVersion().ToString() + " > " + TargetVersion().ToString() + "]";
}

// ===================================================== PartOffset ===================================================

void PartOffset::OnInit() { Setup(partName, positionOffset, rotationOffset); }

// PartOffset.cs:28
TestResult PartOffset::OnTest(ConfigNode& node, LoadContext loadContext, std::string& nodeName) {
    const std::optional<std::string> name = NodeUtil::GetPartNodeNameValue(node, loadContext);
    if (loadContext == LoadContext::Craft) nodeName = dotnet::Split(Req(name, "part name"), "_", false)[0];
    else nodeName = name.value_or("");
    if (name.has_value() && nodeName == partName) return TestResult::Upgradeable;
    return TestResult::Pass;
}

// PartOffset.cs:42 (rotation0 keeps the previous part's value when a part has no rotation -- sic)
void PartOffset::OnUpgrade(ConfigNode& node, LoadContext loadContext, ConfigNode*) {
    const char* name;
    const char* name2;
    if (loadContext != LoadContext::SFS && loadContext == LoadContext::Craft) {
        name = "rot";
        name2 = "pos";
    } else {
        name = "rotation";
        name2 = "position";
    }
    if (node.HasValue(name)) {
        rotation0 = KSPUtil::ParseQuaternion(*node.GetValuePtr(name));
        rotation = rotation0 * rotationOffset;
        node.SetValue(name, KSPUtil::WriteQuaternion(rotation));
    }
    if (node.HasValue(name2)) {
        position0 = KSPUtil::ParseVector3(*node.GetValuePtr(name2));
        const Vector3f vector = position0 + rotation0 * positionOffset;
        node.SetValue(name2, KSPUtil::WriteVector(vector));
    }
}

// ==================================================== PartReplace ===================================================

// PartReplace.cs:24
void PartReplace::OnInit() {
    Setup(partName, replacementPartName, referenceTransformName, positionOffset, rotationOffset, childPositionOffset,
          attach0Offset, attachNodeOffsets);
    SaveEnvironment::Current().partReplacements.emplace_back(partName, replacementPartName);
}

// PartReplace.cs:29
void PartReplace::Setup(std::string&, Vector3f&, Quaternionf&) {
    log::Error("Abstract Setup method called on PartReplace upgradeScript - shouldnt be called");
    throw std::logic_error("NotImplementedException");
}

TestResult PartReplace::OnTest(ConfigNode& node, LoadContext loadContext, std::string& nodeName) {
    return PartOffset::OnTest(node, loadContext, nodeName);
}

// PartReplace.cs:38
void PartReplace::OnUpgrade(ConfigNode& node, LoadContext loadContext, ConfigNode* parentNode) {
    const std::string partNodeNameValue = Req(NodeUtil::GetPartNodeNameValue(node, loadContext), "part name");
    const size_t us = partNodeNameValue.rfind('_');
    const std::string text = us != std::string::npos ? partNodeNameValue.substr(us) : std::string();
    const std::string text2 = replacementPartName + text;
    NodeUtil::SetPartNodeNameValue(node, loadContext, text2);
    if (parentNode == nullptr) throw NullReferenceException("PartReplace.OnUpgrade: parentNode is null");
    const std::vector<ConfigNode*> nodes = parentNode->GetNodes(Req(nodeUrlCraft, "nodeUrlCraft"));
    ConfigNode* configNode = nullptr;
    if (loadContext == LoadContext::SFS) {
        int32_t value = -1;
        if (node.TryGetValue("parent", value)) configNode = nodes.at(static_cast<size_t>(value));
        if (configNode == &node) configNode = nullptr;
    } else if (loadContext == LoadContext::Craft) {
        for (ConfigNode* configNode2 : nodes) {
            if (configNode == nullptr) {
                std::vector<std::string> valuesList = configNode2->GetValuesList("link");
                for (std::string& link : valuesList)
                    if (partNodeNameValue == link) {
                        configNode = configNode2;
                        link = text2;
                        break;
                    }
                if (configNode != nullptr) {
                    configNode2->RemoveValues(std::string_view("link"));
                    for (const std::string& l : valuesList) configNode2->AddValue("link", l);
                }
            }
            const std::vector<std::string> valuesList2 = configNode2->GetValuesList("attN");
            bool flag = false;
            for (const std::string& a : valuesList2)
                if (a.find(partNodeNameValue) != std::string::npos) {
                    flag = true;
                    break;
                }
            if (flag) {
                configNode2->RemoveValues(std::string_view("attN"));
                for (const std::string& a : valuesList2) configNode2->AddValue("attN", dotnet::Replace(a, partNodeNameValue, text2));
            }
        }
    }
    Vector3f vector = positionOffset;
    Vector3f vector2 = childPositionOffset;
    position0 = KSPUtil::ParseVector3(Req(NodeUtil::GetPartPosition(node, loadContext), "part position"));
    rotation0 = KSPUtil::ParseQuaternion(Req(NodeUtil::GetPartRotation(node, loadContext), "part rotation"));
    if (configNode != nullptr) {
        const Vector3f rhs =
            KSPUtil::ParseVector3(Req(NodeUtil::GetPartPosition(*configNode, loadContext), "parent position")) - position0;
        if (Vector3f::Dot(rotation0 * Vector3f::up(), rhs) < 0.0f) std::swap(vector, vector2);
    }
    if (loadContext == LoadContext::Craft || configNode != nullptr) {
        const Vector3f newPos = position0 + rotation0 * vector;
        NodeUtil::SetPartPosition(node, loadContext, newPos);
        const Quaternionf newRot = rotation0 * rotationOffset;
        NodeUtil::SetPartRotation(node, loadContext, newRot);
    }
    if (loadContext == LoadContext::SFS && node.HasValue("rTrf")) node.SetValue("rTrf", referenceTransformName);
    if (loadContext == LoadContext::Craft) {
        if (node.HasValue("attPos0")) {
            Vector3f v4 = KSPUtil::ParseVector3(*node.GetValuePtr("attPos0"));
            v4 = v4 + rotation0 * attach0Offset;
            node.SetValue("attPos0", KSPUtil::WriteVector(v4));
        }
        const std::vector<std::string> valuesList3 = node.GetValuesList("attN");
        node.RemoveValues(std::string_view("attN"));
        for (const std::string& a : valuesList3) {
            const size_t comma = a.find(',');
            const bool hasTwo = comma != std::string::npos;
            const std::string second = hasTwo ? a.substr(comma + 1) : std::string();
            if (hasTwo && dotnet::Split(second, "_", false).size() > 2) {
                const std::string text3 = a.substr(0, comma);
                const size_t num = a.rfind('_');
                Vector3f v5 = KSPUtil::ParseVector3(a.substr(num + 1), '|');
                for (const auto& o : attachNodeOffsets)
                    if (text3 == o.id) v5 = v5 + o.offset;
                node.AddValue("attN", a.substr(0, num + 1) + KSPUtil::WriteVector(v5, "|"));
            } else {
                node.AddValue("attN", a);
            }
        }
    }
    if (loadContext == LoadContext::Craft) {
        AdjustCraftChildPartsPos(node, configNode, loadContext, nodes, vector - vector2);
    } else if (loadContext == LoadContext::SFS) {
        if (configNode != nullptr) AdjustSFSChildPartsPos(node, loadContext, nodes, vector - vector2);
        else AdjustSFSRootChildPartsPos(node, loadContext, nodes, vector, vector2);
    }
}

// PartReplace.cs:173
void PartReplace::AdjustCraftChildPartsPos(ConfigNode& node, ConfigNode* parent, LoadContext loadContext,
                                           const std::vector<ConfigNode*>& partNodes, const Vector3f& offset) {
    std::vector<std::string> valuesList = node.GetValuesList("link");
    if (parent == nullptr) {
        const std::vector<std::string> valuesList2 = node.GetValuesList("attN");
        for (const std::string& a : valuesList2) {
            const size_t comma = a.find(',');
            if ((comma == std::string::npos ? a : a.substr(0, comma)) != "top") continue;
            int count = static_cast<int>(valuesList.size());
            while (count-- > 0)
                if (a.find(valuesList[static_cast<size_t>(count)]) != std::string::npos) {
                    valuesList.erase(valuesList.begin() + count);
                    break;
                }
            break;
        }
    }
    for (const std::string& link : valuesList)
        for (ConfigNode* configNode : partNodes)
            if (NodeUtil::GetPartNodeNameValue(*configNode, loadContext) == link) {
                AdjustPartByOffset(*configNode, loadContext, offset);
                AdjustCraftChildPartsPos(*configNode, &node, loadContext, partNodes, offset);
            }
}

// PartReplace.cs:210
void PartReplace::AdjustSFSChildPartsPos(ConfigNode& node, LoadContext loadContext,
                                         const std::vector<ConfigNode*>& partNodes, const Vector3f& offset) {
    const auto it = std::find(partNodes.begin(), partNodes.end(), &node);
    const int num = it == partNodes.end() ? -1 : static_cast<int>(it - partNodes.begin());
    for (int i = 0; i < static_cast<int>(partNodes.size()); ++i) {
        if (num == i) continue;
        ConfigNode* configNode = partNodes[static_cast<size_t>(i)];
        if (configNode->GetValue("parent") == dotnet::FormatInt64(num)) {
            AdjustPartByOffset(*configNode, loadContext, offset);
            AdjustSFSChildPartsPos(*configNode, loadContext, partNodes, offset);
        }
    }
}

// PartReplace.cs:227
void PartReplace::AdjustSFSRootChildPartsPos(ConfigNode& node, LoadContext loadContext,
                                             const std::vector<ConfigNode*>& partNodes, const Vector3f& offsetTop,
                                             const Vector3f& offsetBottom) {
    const auto it = std::find(partNodes.begin(), partNodes.end(), &node);
    const int num = it == partNodes.end() ? -1 : static_cast<int>(it - partNodes.begin());
    for (int i = 0; i < static_cast<int>(partNodes.size()); ++i) {
        if (num == i) continue;
        ConfigNode* configNode = partNodes[static_cast<size_t>(i)];
        if (configNode->GetValue("parent") == dotnet::FormatInt64(num)) {
            if (KSPUtil::ParseVector3(Req(NodeUtil::GetPartPosition(*configNode, loadContext), "part position")).y > 0.0f) {
                AdjustPartByOffset(*configNode, loadContext, -offsetTop);
                AdjustSFSChildPartsPos(*configNode, loadContext, partNodes, -offsetTop);
            } else {
                AdjustPartByOffset(*configNode, loadContext, -offsetBottom);
                AdjustSFSChildPartsPos(*configNode, loadContext, partNodes, -offsetBottom);
            }
        }
    }
}

// PartReplace.cs:255
void PartReplace::AdjustPartByOffset(ConfigNode& part, LoadContext loadContext, const Vector3f& offset) {
    Vector3f newPos = KSPUtil::ParseVector3(Req(NodeUtil::GetPartPosition(part, loadContext), "part position"));
    newPos = newPos + rotation0 * offset;
    NodeUtil::SetPartPosition(part, loadContext, newPos);
}

// =================================================== ModuleReplace ==================================================

void ModuleReplace::OnInit() {
    moduleNodes.clear();
    Setup(moduleName, newModuleName);
}

// ModuleReplace.cs:22
TestResult ModuleReplace::OnTest(ConfigNode& node, LoadContext, std::string& nodeName) {
    nodeName = moduleName;
    moduleNodes = NodeUtil::GetModuleNodesNamed(node, moduleName);
    return moduleNodes.empty() ? TestResult::Pass : TestResult::Upgradeable;
}

// ModuleReplace.cs:33
void ModuleReplace::OnUpgrade(ConfigNode&, LoadContext, ConfigNode*) {
    for (ConfigNode* m : moduleNodes)
        if (m->HasValue("name")) m->SetValue("name", newModuleName);
}

// ============================================== v110_WheelModuleOverhaul ============================================

std::string v110_WheelModuleOverhaul::Description() const {
    return "Upgrades obsolete ModuleWheel, ModuleLandingGear, ModuleLandingLeg data into the new ModuleWheelBase and "
           "WheelSubmodules scheme.";
}

// v110_WheelModuleOverhaul.cs:36
TestResult v110_WheelModuleOverhaul::OnTest(ConfigNode& node, LoadContext loadContext, std::string& nodeName) {
    nodeName = NodeUtil::GetPartNodeNameValue(node, loadContext).value_or("");
    const std::vector<ConfigNode*> nodes = node.GetNodes("MODULE");
    int num = static_cast<int>(nodes.size());
    while (num-- > 0) {
        const std::string* n = nodes[static_cast<size_t>(num)]->GetValuePtr("name");
        if (n == nullptr) continue;
        if (*n == "ModuleWheel" || *n == "ModuleLandingGear" || *n == "ModuleLandingGearFixed" ||
            *n == "ModuleLandingLeg" || *n == "ModuleSteering")
            return TestResult::Upgradeable;
    }
    return TestResult::Pass;
}

// v110_WheelModuleOverhaul.cs:57
void v110_WheelModuleOverhaul::OnUpgrade(ConfigNode& node, LoadContext loadContext, ConfigNode*) {
    const std::vector<ConfigNode*> nodes = node.GetNodes("MODULE");
    int num = static_cast<int>(nodes.size());
    while (num-- > 0) {
        const ConfigNode& m = *nodes[static_cast<size_t>(num)];
        const std::string* n = m.GetValuePtr("name");
        if (n == nullptr) continue;
        if (*n == "ModuleSteering") SalvageSteering(node, m, loadContext);
        else if (*n == "ModuleLandingLeg") SalvageLegDeployment(node, m, loadContext);
        else if (*n == "ModuleLandingGearFixed") SalvageBrakes(node, m, loadContext);
        else if (*n == "ModuleLandingGear") {
            SalvageBrakes(node, m, loadContext);
            SalvageGearDeployment(node, m, loadContext);
        } else if (*n == "ModuleWheel") {
            SalvageBrakes(node, m, loadContext);
            SalvageSteering(node, m, loadContext);
            SalvageMotor(node, m, loadContext);
        }
    }
}

// v110_WheelModuleOverhaul.cs:87
void v110_WheelModuleOverhaul::SalvageBrakes(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext loadContext) {
    auto configNode = std::make_shared<ConfigNode>("MODULE");
    configNode->AddValue("name", "ModuleWheelBrakes");
    if (loadContext == LoadContext::SFS && mNode.HasValue("brakesEngaged")) {
        const bool flag = save_detail::ParseBool(*mNode.GetValuePtr("brakesEngaged"));
        configNode->AddValue("brakeInput", flag ? 1.0f : 0.0f);
    }
    if (mNode.HasValue("brakesEnabled")) {
        const bool flag2 = save_detail::ParseBool(*mNode.GetValuePtr("brakesEnabled"));
        configNode->AddValue("brakeTweak", flag2 ? 100.0f : 0.0f);
    }
    if (configNode->values().Count() > 1) dstNode.AddNode(configNode);
}

// v110_WheelModuleOverhaul.cs:110
void v110_WheelModuleOverhaul::SalvageSteering(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext) {
    auto configNode = std::make_shared<ConfigNode>("MODULE");
    configNode->AddValue("name", "ModuleWheelSteering");
    if (mNode.HasValue("steeringLocked")) {
        const bool flag = save_detail::ParseBool(*mNode.GetValuePtr("steeringLocked"));
        configNode->AddValue("steeringEnabled", !flag);
    }
    if (mNode.HasValue("invertSteering")) {
        const bool value = save_detail::ParseBool(*mNode.GetValuePtr("invertSteering"));
        configNode->AddValue("steeringInvert", value);
    }
    if (configNode->values().Count() > 1) {
        dstNode.AddNode(configNode);
        ConfigNode::NodePtr configNode2 = configNode->CreateCopy();
        configNode2->SetValue("name", "ModuleWheelMotorSteering");
        dstNode.AddNode(configNode2);
    }
}

// v110_WheelModuleOverhaul.cs:133
void v110_WheelModuleOverhaul::SalvageMotor(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext) {
    auto configNode = std::make_shared<ConfigNode>("MODULE");
    configNode->AddValue("name", "ModuleWheelMotor");
    if (mNode.HasValue("motorEnabled"))
        configNode->AddValue("motorEnabled", save_detail::ParseBool(*mNode.GetValuePtr("motorEnabled")));
    if (configNode->values().Count() > 1) dstNode.AddNode(configNode);
}

// v110_WheelModuleOverhaul.cs:147
void v110_WheelModuleOverhaul::SalvageDamage(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext) {
    auto configNode = std::make_shared<ConfigNode>("MODULE");
    configNode->AddValue("name", "ModuleWheelDamage");
    if (mNode.HasValue("isDamaged"))
        configNode->AddValue("isDamaged", save_detail::ParseBool(*mNode.GetValuePtr("isDamaged")));
    if (configNode->values().Count() > 1) dstNode.AddNode(configNode);
}

// v110_WheelModuleOverhaul.cs:161
void v110_WheelModuleOverhaul::SalvageGearDeployment(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext loadContext) {
    auto configNode = std::make_shared<ConfigNode>("MODULE");
    configNode->AddValue("name", "ModuleWheelDeployment");
    if (mNode.HasValue("storedAnimationTime"))
        configNode->AddValue("position", save_detail::ParseFloat(*mNode.GetValuePtr("storedAnimationTime")));
    if (loadContext == LoadContext::Craft) {
        if (mNode.HasValue("StartDeployed")) {
            const bool flag = save_detail::ParseBool(*mNode.GetValuePtr("StartDeployed"));
            configNode->AddValue("stateString", flag ? "Deployed" : "Retracted");
        }
    } else if (loadContext == LoadContext::SFS) {
        if (mNode.HasValue("storedGearState")) {
            const auto st = save_detail::ParseEnum<OldGearDeploymentStates>(*mNode.GetValuePtr("storedGearState"));
            configNode->AddValue("stateString", DeployStateUpgrade(st));
        }
    }
    if (configNode->values().Count() > 1) dstNode.AddNode(configNode);
}

// v110_WheelModuleOverhaul.cs:191
void v110_WheelModuleOverhaul::SalvageLegDeployment(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext loadContext) {
    auto configNode = std::make_shared<ConfigNode>("MODULE");
    configNode->AddValue("name", "ModuleWheelDeployment");
    if (mNode.HasValue("savedAnimationTime"))
        configNode->AddValue("position", save_detail::ParseFloat(*mNode.GetValuePtr("savedAnimationTime")));
    if (loadContext == LoadContext::Craft) {
        if (mNode.HasValue("StartDeployed")) {
            const bool flag = save_detail::ParseBool(*mNode.GetValuePtr("StartDeployed"));
            configNode->AddValue("stateString", flag ? "Deployed" : "Retracted");
        }
    } else if (loadContext == LoadContext::SFS) {
        if (mNode.HasValue("savedLegState")) {
            const auto st = save_detail::ParseEnum<OldLegDeploymentStates>(*mNode.GetValuePtr("savedLegState"));
            configNode->AddValue("stateString", DeployStateUpgrade(st));
        }
    }
    if (configNode->values().Count() > 1) dstNode.AddNode(configNode);
}

// v110_WheelModuleOverhaul.cs:221
std::string v110_WheelModuleOverhaul::DeployStateUpgrade(OldGearDeploymentStates st) {
    switch (st) {
    case OldGearDeploymentStates::DEPLOYED: return "Deployed";
    case OldGearDeploymentStates::RETRACTED: return "Retracted";
    default: return "";
    }
}

// v110_WheelModuleOverhaul.cs:233
std::string v110_WheelModuleOverhaul::DeployStateUpgrade(OldLegDeploymentStates st) {
    switch (st) {
    case OldLegDeploymentStates::DEPLOYED: return "Deployed";
    case OldLegDeploymentStates::RETRACTED: return "Retracted";
    default: return "";
    }
}

// =============================================== v180 control surfaces ==============================================

// v180_ModuleAeroSurface.cs:17 / v180_ModuleControlSurface.cs:17
TestResult v180_ControlAuthority::OnTest(ConfigNode& node, LoadContext loadContext, std::string& nodeName) {
    nodeName = NodeUtil::GetPartNodeNameValue(node, loadContext).value_or("");
    const std::string moduleName = aero_ ? "ModuleAeroSurface" : "ModuleControlSurface";
    const std::vector<ConfigNode*> nodes = node.GetNodes("MODULE");
    int num = static_cast<int>(nodes.size());
    while (num-- > 0) {
        const std::string* n = nodes[static_cast<size_t>(num)]->GetValuePtr("name");
        if (n != nullptr && *n == moduleName) return TestResult::Upgradeable;
    }
    return TestResult::Pass;
}

// v180_ModuleAeroSurface.cs:52 GetBaseModule(...).ctrlSurfaceRange (PartLoader prefab: SaveEnvironment hook;
// a missing part or module is the NullReferenceException KSP throws)
float v180_ControlAuthority::GetBaseModuleCtrlSurfaceRange(const ConfigNode& node, LoadContext loadContext) const {
    std::string value;
    if (loadContext == LoadContext::Craft) {
        value = Req(node.GetValue("part"), "part");
        const size_t us = value.find('_');
        if (us == std::string::npos) throw std::out_of_range("ArgumentOutOfRangeException: Substring length");
        value = value.substr(0, us);
    } else {
        value = Req(node.GetValue("name"), "name");
    }
    const SaveEnvironment& env = SaveEnvironment::Current();
    std::optional<float> r;
    if (env.prefabModuleFloat)
        r = env.prefabModuleFloat(value, aero_ ? "ModuleAeroSurface" : "ModuleControlSurface", "ctrlSurfaceRange");
    if (!r) throw NullReferenceException("v180 upgrade: no prefab " + std::string(aero_ ? "ModuleAeroSurface" : "ModuleControlSurface") +
                                         " for part " + value);
    return *r;
}

// v180_ModuleAeroSurface.cs:36 (ConvertAeroAuthority / ConvertControlAuthority + the AXISGROUPS rename)
void v180_ControlAuthority::OnUpgrade(ConfigNode& node, LoadContext loadContext, ConfigNode*) {
    const std::string moduleName = aero_ ? "ModuleAeroSurface" : "ModuleControlSurface";
    const char* limiter = aero_ ? "aeroAuthorityLimiter" : "authorityLimiter";
    const char* deploy = aero_ ? "aeroDeployAngle" : "deployAngle";
    const std::vector<ConfigNode*> nodes = node.GetNodes("MODULE");
    int num = static_cast<int>(nodes.size());
    while (num-- > 0) {
        ConfigNode& mNode = *nodes[static_cast<size_t>(num)];
        const std::string* n = mNode.GetValuePtr("name");
        if (n == nullptr || *n != moduleName) continue;
        const float ctrlSurfaceRange = GetBaseModuleCtrlSurfaceRange(node, loadContext);
        // ConvertAeroAuthority: float.TryParse(GetValue(limiter)) (null -> 0); value = range * result * 0.01f
        float result = 0.0f;
        if (const std::string* t = mNode.GetValuePtr(limiter)) dotnet::TryParseSingle(*t, result);
        const float value = f32store(fx(ctrlSurfaceRange) * fx(result) * fx(0.01f));
        mNode.AddValue(deploy, value);
        // ConvertAeroAuthorityAxisGroup
        ConfigNode* axis = mNode.GetNode("AXISGROUPS");
        if (axis == nullptr) continue;
        int count = axis->nodes().Count();
        while (count-- > 0) {
            ConfigNode& c = *axis->nodes()[count];
            if (c.name == limiter) c.name = deploy;
        }
    }
}

// ================================================ SaveUpgradePipeline ===============================================

// SaveUpgradePipeline.cs:31 / 38 (OrderBy TargetVersion, ThenBy EarliestCompatibleVersion: stable)
void SaveUpgradePipeline::Init(std::vector<std::pair<std::unique_ptr<UpgradeScript>, UpgradeModule>> scripts) {
    OnSetCfgNodeVersion = OnSetCfgNodeVersion_Default;
    upgradeScripts.clear();
    for (auto& [script, attr] : scripts) {
        script->Init(attr.loadContext, attr.craftNodeUrl, attr.sfsNodeUrl);
        upgradeScripts.push_back(std::move(script));
    }
    std::stable_sort(upgradeScripts.begin(), upgradeScripts.end(),
                     [](const std::unique_ptr<UpgradeScript>& a, const std::unique_ptr<UpgradeScript>& b) {
                         const int t = a->TargetVersion().CompareTo(b->TargetVersion());
                         if (t != 0) return t < 0;
                         return a->EarliestCompatibleVersion() < b->EarliestCompatibleVersion();
                     });
    initialized = true;
}

// SaveUpgradePipeline.cs:50
ConfigNode::NodePtr SaveUpgradePipeline::Run(const ConfigNode::NodePtr& node, LoadContext ctx, const Version& AppVersion,
                                             bool& runSuccess, std::string& runInfo) {
    runLog.clear();
    return Run(node, ctx, AppVersion, runSuccess, runInfo, runLog);
}

// SaveUpgradePipeline.cs:56
ConfigNode::NodePtr SaveUpgradePipeline::Run(const ConfigNode::NodePtr& node, LoadContext ctx, const Version& AppVersion,
                                             bool& runSuccess, std::string& runInfo, std::vector<Row>& log) {
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<UpgradeScript*> scripts;
    for (auto& s : upgradeScripts)
        if (SanityCheck(*s, AppVersion)) scripts.push_back(s.get());
    ConfigNode::NodePtr node2;
    const Version cfgVersion = NodeUtil::GetCfgVersion(*node, ctx);
    runInfo = "Original File Version: " + cfgVersion.ToString(3) + "\n";
    int num = 0;
    IterationResult iterationResult = IterationResult::Continue;
    while (iterationResult == IterationResult::Continue) {
        ++num;
        iterationResult = RunIteration(*node, node2, ctx, scripts, log);
    }
    const long long ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    switch (iterationResult) {
    case IterationResult::Fail:
        runSuccess = false;
        runInfo += "Upgrades Failed. " + dotnet::FormatInt64(num) + " iterations. Runtime: " + dotnet::FormatInt64(ms) + "ms";
        log::Warning("[SaveUpgradePipeline]: " + runInfo);
        break;
    case IterationResult::Pass:
        runSuccess = true;
        runInfo += "Upgrades Completed. " + dotnet::FormatInt64(num) + " iterations. Runtime: " + dotnet::FormatInt64(ms) + "ms";
        break;
    default:
        runSuccess = false;
        runInfo += "Upgrades Finished with result Continue! This is madness!" + dotnet::FormatInt64(num) +
                   " iterations.Runtime: " + dotnet::FormatInt64(ms) + "ms";
        log::Error("[SaveUpgradePipeline]: " + runInfo);
        break;
    }
    return node2 ? node2 : node;
}

// SaveUpgradePipeline.cs:100
IterationResult SaveUpgradePipeline::RunIteration(ConfigNode& srcNode, ConfigNode::NodePtr& node, LoadContext ctx,
                                                  const std::vector<UpgradeScript*>& scripts, std::vector<Row>& log) {
    const bool hasPrevious = !log.empty();
    log.emplace_back();
    const size_t rowIndex = log.size() - 1;
    auto rowOf = [&](size_t idx) -> Row& { return log[idx]; };
    auto find = [](Row& row, UpgradeScript* s) -> LogEntry& {
        for (auto& [k, v] : row)
            if (k == s) return v;
        throw KeyNotFoundException("The given key was not present in the dictionary.");
    };
    ConfigNode* node2 = node ? node.get() : &srcNode;
    int count = static_cast<int>(scripts.size());
    while (count-- > 0) {
        const TestResult testResult = RunTest(*scripts[static_cast<size_t>(count)], node2, ctx);
        rowOf(rowIndex).emplace_back(scripts[static_cast<size_t>(count)], LogEntry(testResult, false));
        if (testResult == TestResult::Failed) return IterationResult::Fail;
    }
    Row& row = rowOf(rowIndex);
    if (std::all_of(row.begin(), row.end(), [](const auto& e) { return e.second.testResult == TestResult::Pass; }))
        return IterationResult::Pass;
    if (std::all_of(row.begin(), row.end(), [](const auto& e) { return e.second.testResult == TestResult::TooEarly; }))
        return IterationResult::Fail;
    if (!TestExceptionCases(log)) return IterationResult::Fail;
    if (!node) node = srcNode.CreateCopy();
    int count2 = static_cast<int>(scripts.size());
    while (count2-- > 0) {
        UpgradeScript* s = scripts[static_cast<size_t>(count2)];
        LogEntry& entry = find(rowOf(rowIndex), s);
        if (entry.testResult != TestResult::Upgradeable) continue;
        if (hasPrevious && find(rowOf(rowIndex - 1), s).upgraded) {
            entry.testResult = TestResult::Pass;
            entry.upgraded = true;
        } else {
            ConfigNode* r = RunUpgrade(*s, node.get(), ctx);
            if (r == nullptr) node.reset();  // C#: node = RunUpgrade(...) (null for a script outside its context)
            find(rowOf(rowIndex), s).upgraded = true;
            if (!node) throw NullReferenceException("SaveUpgradePipeline.RunIteration: RunUpgrade returned null");
        }
    }
    lowestVersion_ = Version(std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
                             std::numeric_limits<int>::max());
    std::vector<std::pair<UpgradeScript*, LogEntry*>> array;
    for (auto& [k, v] : rowOf(rowIndex))
        if (v.testResult == TestResult::Upgradeable || v.testResult == TestResult::TooEarly || v.upgraded)
            array.emplace_back(k, &v);
    int num = static_cast<int>(array.size());
    while (num-- > 0) {
        const auto& [k, v] = array[static_cast<size_t>(num)];
        const Version version = v->testResult != TestResult::TooEarly ? k->TargetVersion() : k->EarliestCompatibleVersion();
        if (version < lowestVersion_) lowestVersion_ = version;
    }
    OnSetCfgNodeVersion(*node, ctx, lowestVersion_);
    return IterationResult::Continue;
}

// SaveUpgradePipeline.cs:148
bool SaveUpgradePipeline::SanityCheck(const UpgradeScript& uSC, const Version& AppVersion) {
    if (uSC.TargetVersion() <= uSC.EarliestCompatibleVersion()) {
        log::Error("[SaveUpgradePipeline]: A script's target version should never be LEqual to its earliest-compat "
                   "version. " + uSC.Name() + " will be skipped.");
        return false;
    }
    if (!(uSC.TargetVersion() > AppVersion) && !(uSC.EarliestCompatibleVersion() > AppVersion)) return true;
    log::Error("[SaveUpgradePipeline]: A script's versions should never exceed the current application version " +
               uSC.Name() + " will be skipped.");
    return false;
}

// SaveUpgradePipeline.cs:162
bool SaveUpgradePipeline::TestExceptionCases(const std::vector<Row>& log) {
    if (log.size() > 1) {
        const Row& previous = log[log.size() - 2];
        const Row& last = log[log.size() - 1];
        for (const auto& [key, entry] : last) {
            const LogEntry* prev = nullptr;
            for (const auto& [k2, e2] : previous)
                if (k2 == key) prev = &e2;
            if (prev == nullptr) throw KeyNotFoundException("The given key was not present in the dictionary.");
            if (static_cast<int>(entry.testResult) <= static_cast<int>(TestResult::Upgradeable) &&
                prev->testResult == TestResult::Pass && !prev->upgraded)
                throw std::logic_error("InvalidOperationException: [SaveUpgradePipeline] " + key->Name() + "returned " +
                                       ToString(entry.testResult) +
                                       " after having passed in a previous iteration. this doesn't make sense.");
        }
    }
    return true;
}

// SaveUpgradePipeline.cs:176
void SaveUpgradePipeline::OnSetCfgNodeVersion_Default(ConfigNode& n, LoadContext loadContext, const Version& version) {
    if (loadContext == LoadContext::SFS) {
        ConfigNode* game = n.GetNode("GAME");
        if (game == nullptr) throw NullReferenceException("OnSetCfgNodeVersion_Default: no GAME node");
        game->SetValue("version", version.ToString(3), true);
    } else {
        n.SetValue("version", version.ToString(3), true);
    }
}

// SaveUpgradePipeline.cs:187
TestResult SaveUpgradePipeline::RunTest(UpgradeScript& uSc, ConfigNode* node, LoadContext loadContext) {
    if (!UtilMath::IsPowerOfTwo(static_cast<int>(loadContext)))
        throw ArgumentException("Multiple LoadContexts cannot be used as a parameter in this method.");
    if (uSc.AppliesInContext(loadContext)) {
        if (node != nullptr) return uSc.Test(*node, loadContext);
        throw ArgumentException("Cannot Run tests, given node is null!");
    }
    log::Error("[SaveUpgradePipeline]: Test Module " + uSc.Name() + " does not apply for " + ToString(loadContext) +
               " loading!");
    return TestResult::Failed;
}

// SaveUpgradePipeline.cs:204
ConfigNode* SaveUpgradePipeline::RunUpgrade(UpgradeScript& uSc, ConfigNode* node, LoadContext loadContext) {
    if (!UtilMath::IsPowerOfTwo(static_cast<int>(loadContext)))
        throw ArgumentException("Multiple LoadContexts cannot be used as a parameter in this method.");
    if (uSc.AppliesInContext(loadContext)) {
        if (node != nullptr) {
            uSc.Upgrade(*node, loadContext);
            return node;
        }
        throw ArgumentException("Cannot Run upgrade, given node is null!");
    }
    log::Error("[SaveUpgradePipeline]: Test Module " + uSc.Name() + " does not apply for " + ToString(loadContext) +
               " loading!");
    return nullptr;
}

void SaveUpgradePipeline::SetCfgNodeVersion(ConfigNode& node, LoadContext ctx, const Version& version) {
    OnSetCfgNodeVersion(node, ctx, version);
}

// SaveUpgradePipeline.cs:227
std::string SaveUpgradePipeline::PrintLog(const std::vector<Row>& log, const std::string& cfgName,
                                          const std::string& runInfo) const {
    std::string text = "<b>Upgrade Log for " + cfgName + ":</b>\n";
    for (size_t i = 0; i < log.size(); ++i) {
        std::string text2 = ">> #" + dotnet::FormatInt64(static_cast<int64_t>(i)) + "\n";
        for (const auto& [key, entry] : log[i]) text2 += "<b>" + key->ToString() + "</b>\t | " + entry.ToString() + "\n";
        text += text2;
    }
    return text + runInfo;
}

// ================================================== stock scripts ===================================================

namespace {

struct ScriptDescriptor {
    char kind;  // R = PartReplace, O = PartOffset, M = ModuleReplace, C = custom class
    const char* typeName;
    const char* source;
    LoadContext context;
    const char* sfsUrl;
    const char* craftUrl;
    const char* name;
    const char* description;
    Version earliest;
    Version target;
    const char* a;  // pName / oldName
    const char* b;  // replacementPartName / newName
    const char* c;  // refTransformName
    Vector3f posOffset;
    Quaternionf rotOffset;
    Vector3f childPos;
    Vector3f att0;
    const char* unused;
    std::vector<PartReplace::attachNodeOffset> attN;
};

class StockPartOffset final : public PartOffset {
public:
    explicit StockPartOffset(const ScriptDescriptor& d) : d_(d) {}
    std::string Name() const override { return d_.name; }
    std::string Description() const override { return d_.description; }
    Version EarliestCompatibleVersion() const override { return d_.earliest; }
    Version TargetVersion() const override { return d_.target; }
    std::string TypeName() const override { return d_.typeName; }

protected:
    void Setup(std::string& pName, Vector3f& posOffset, Quaternionf& rotOffset) override {
        pName = d_.a;
        posOffset = d_.posOffset;
        rotOffset = d_.rotOffset;
    }

private:
    const ScriptDescriptor& d_;
};

class StockPartReplace final : public PartReplace {
public:
    explicit StockPartReplace(const ScriptDescriptor& d) : d_(d) {}
    std::string Name() const override { return d_.name; }
    std::string Description() const override { return d_.description; }
    Version EarliestCompatibleVersion() const override { return d_.earliest; }
    Version TargetVersion() const override { return d_.target; }
    std::string TypeName() const override { return d_.typeName; }

protected:
    using PartReplace::Setup;
    void Setup(std::string& pName, std::string& replacementPartName_, std::string& refTransformName, Vector3f& posOffset,
               Quaternionf& rotOffset, Vector3f& childPosOffset, Vector3f& att0Offset,
               std::vector<attachNodeOffset>& attNOffsets) override {
        pName = d_.a;
        replacementPartName_ = d_.b;
        refTransformName = d_.c;
        posOffset = d_.posOffset;
        rotOffset = d_.rotOffset;
        att0Offset = d_.att0;
        childPosOffset = d_.childPos;
        attNOffsets = d_.attN;
    }

private:
    const ScriptDescriptor& d_;
};

class StockModuleReplace final : public ModuleReplace {
public:
    explicit StockModuleReplace(const ScriptDescriptor& d) : d_(d) {}
    std::string Name() const override { return d_.name; }
    std::string Description() const override { return d_.description; }
    Version EarliestCompatibleVersion() const override { return d_.earliest; }
    Version TargetVersion() const override { return d_.target; }
    std::string TypeName() const override { return d_.typeName; }

protected:
    void Setup(std::string& oldName, std::string& newName) override {
        oldName = d_.a;
        newName = d_.b;
    }

private:
    const ScriptDescriptor& d_;
};

Vector3f V3(float x, float y, float z) { return Vector3f{x, y, z}; }
PartReplace::attachNodeOffset AN(const char* id, const Vector3f& offset) { return {id, offset}; }

const std::vector<ScriptDescriptor>& StockDescriptors() {
    static const std::vector<ScriptDescriptor> d = {
#define KSP_UPGRADE_SCRIPT(kind, cls, src, ctx, sfs, craft, name, desc, earliest, target, a, b, c, pos, rot, child, att0, \
                           unused, ...)                                                                                     \
    ScriptDescriptor{#kind[0], #cls, src, ctx, sfs, craft, name, desc, earliest, target, a, b, c, pos, rot, child, att0,     \
                     unused, std::vector<PartReplace::attachNodeOffset>{__VA_ARGS__}},
#include "upgrade_scripts_data.inc"
#undef KSP_UPGRADE_SCRIPT
    };
    return d;
}

}  // namespace

std::vector<std::pair<std::unique_ptr<UpgradeScript>, UpgradeModule>> CreateStockUpgradeScripts() {
    std::vector<std::pair<std::unique_ptr<UpgradeScript>, UpgradeModule>> list;
    for (const ScriptDescriptor& d : StockDescriptors()) {
        std::unique_ptr<UpgradeScript> s;
        switch (d.kind) {
        case 'R': s = std::make_unique<StockPartReplace>(d); break;
        case 'O': s = std::make_unique<StockPartOffset>(d); break;
        case 'M': s = std::make_unique<StockModuleReplace>(d); break;
        default:
            if (std::string_view(d.typeName) == "v110_WheelModuleOverhaul") s = std::make_unique<v110_WheelModuleOverhaul>();
            else if (std::string_view(d.typeName) == "v180_ModuleAeroSurface") s = std::make_unique<v180_ControlAuthority>(true);
            else if (std::string_view(d.typeName) == "v180_ModuleControlSurface")
                s = std::make_unique<v180_ControlAuthority>(false);
            else throw std::logic_error(std::string("unknown custom upgrade script ") + d.typeName);
            break;
        }
        UpgradeModule attr;
        attr.loadContext = d.context;
        if (d.sfsUrl) attr.sfsNodeUrl = std::string(d.sfsUrl);
        if (d.craftUrl) attr.craftNodeUrl = std::string(d.craftUrl);
        list.emplace_back(std::move(s), std::move(attr));
    }
    return list;
}

}  // namespace ksp::SaveUpgradePipeline

namespace ksp {

namespace {
std::unique_ptr<SaveUpgradePipeline::SaveUpgradePipeline>& PipelineSlot() {
    static std::unique_ptr<SaveUpgradePipeline::SaveUpgradePipeline> p;
    return p;
}
}  // namespace

// KSPUpgradePipeline.cs:24
SaveUpgradePipeline::SaveUpgradePipeline& KSPUpgradePipeline::Pipeline() {
    auto& slot = PipelineSlot();
    if (!slot) {
        slot = std::make_unique<SaveUpgradePipeline::SaveUpgradePipeline>();
        slot->Init(SaveUpgradePipeline::CreateStockUpgradeScripts());
    }
    return *slot;
}

void KSPUpgradePipeline::Reset() { PipelineSlot().reset(); }

// KSPUpgradePipeline.cs:35
void KSPUpgradePipeline::Process(const ConfigNode::NodePtr& n, const std::string& saveName,
                                 SaveUpgradePipeline::LoadContext loadContext,
                                 const std::function<void(const ConfigNode::NodePtr&)>& onSucceed,
                                 const std::function<void(UpgradeFailOption, const ConfigNode::NodePtr&)>& onFail,
                                 const std::function<UpgradeFailOption(const std::string&)>& failDialog) {
    if (!Enabled) {
        onSucceed(n);
        return;
    }
    if (!n) {
        onFail(UpgradeFailOption::Cancel, n);
        return;
    }
    const dotnet::Version cfgVersion = SaveUpgradePipeline::NodeUtil::GetCfgVersion(*n, loadContext);
    const dotnet::Version version(Versioning::version_major, Versioning::version_minor, Versioning::Revision);
    bool runSuccess = false;
    std::string runInfo;
    ConfigNode::NodePtr arg = Pipeline().Run(n, loadContext, version, runSuccess, runInfo);
    if (runSuccess) {
        if (cfgVersion != version)
            log::Info("[KSPUpgradePipeline]: " + saveName + " updated from " + cfgVersion.ToString() + " to " +
                      version.ToString() + ".");
        else
            log::Info("[KSPUpgradePipeline]: " + saveName + " (" + cfgVersion.ToString() + ") is up to date.");
        onSucceed(arg);
    } else {
        const UpgradeFailOption opt = failDialog ? failDialog(runInfo) : UpgradeFailOption::Cancel;
        onFail(opt, opt == UpgradeFailOption::LoadAnyway ? n : nullptr);
    }
}

}  // namespace ksp
