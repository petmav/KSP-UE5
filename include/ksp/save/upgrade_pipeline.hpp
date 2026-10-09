// The save/craft upgrade pipeline: namespace SaveUpgradePipeline (Assembly-CSharp/SaveUpgradePipeline/*.cs, all 51
// files: the framework and the 40 stock upgrade scripts) and KSPUpgradePipeline.cs (the runner the game calls).
//
// The pipeline repeatedly tests every script against the file's version and its PART nodes, upgrades the copy of the
// tree with every Upgradeable script, stamps the lowest version among the scripts involved (GAME/version for .sfs, the
// root "version" for .craft) and iterates until every script passes. Scripts are tested and run from the END of the
// list, which is sorted (stable) by TargetVersion, then EarliestCompatibleVersion, starting from the discovery order
// (Mono Assembly.GetTypes = TypeDef order of the shipped Assembly-CSharp.dll, see src/save/upgrade_scripts_data.inc).
// Every upgraded script appends `upgraded = <Name> // <Description>` to the ROOT node (not to GAME), and the RecurseNodes
// walk visits nodes in reverse order. The Unity float maths of the part offsets (Vector3 / Quaternion operators,
// managed) follow the Mono float32 rule through ksp/unity; Quaternion.AngleAxis is the native port.
//
// Inputs from other systems (PartLoader): SaveEnvironment::prefabModuleFloat (v180 scripts read the prefab's
// ctrlSurfaceRange) and SaveEnvironment::partReplacements (PartReplace.OnInit's PartLoader.AddPartReplacement).
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/version.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::SaveUpgradePipeline {

using dotnet::Version;

// LoadContext.cs ([Flags])
enum class LoadContext : int { None = 0, SFS = 1, Craft = 2, Any = -1 };
inline LoadContext operator|(LoadContext a, LoadContext b) {
    return static_cast<LoadContext>(static_cast<int>(a) | static_cast<int>(b));
}
inline LoadContext operator&(LoadContext a, LoadContext b) {
    return static_cast<LoadContext>(static_cast<int>(a) & static_cast<int>(b));
}
std::string ToString(LoadContext c);  // Enum.ToString ([Flags])

// TestResult.cs / IterationResult.cs
enum class TestResult : int { Failed, TooEarly, Upgradeable, Pass };
enum class IterationResult : int { Fail, Continue, Pass };
std::string ToString(TestResult r);

// LogEntry.cs
class LogEntry {
public:
    TestResult testResult;
    bool upgraded;
    LogEntry(TestResult testResult_, bool upgraded_) : testResult(testResult_), upgraded(upgraded_) {}
    std::string ToString() const;  // LogEntry.cs:17 (rich-text colour tags)
};

// UpgradeModule.cs (the attribute's data)
struct UpgradeModule {
    LoadContext loadContext = LoadContext::None;
    std::optional<std::string> sfsNodeUrl;
    std::optional<std::string> craftNodeUrl;
};

// NodeUtil.cs
namespace NodeUtil {
Version GetCfgVersion(const ConfigNode& n, LoadContext loadContext);                          // NodeUtil.cs:9
std::optional<std::string> GetPartNodeNameValue(const ConfigNode& node, LoadContext loadContext);  // :18
std::vector<ConfigNode*> GetModuleNodesNamed(const ConfigNode& node, std::string_view moduleName);  // :31
void SetPartNodeNameValue(ConfigNode& node, LoadContext loadContext, const std::string& newNameValue);  // :44
std::optional<std::string> GetPartPosition(const ConfigNode& node, LoadContext loadContext);       // :57
void SetPartPosition(ConfigNode& node, LoadContext loadContext, const Vector3f& newPos);            // :70
std::optional<std::string> GetPartRotation(const ConfigNode& node, LoadContext loadContext);       // :83
void SetPartRotation(ConfigNode& node, LoadContext loadContext, const Quaternionf& newRot);        // :96
}  // namespace NodeUtil

// UpgradeScript.cs
class UpgradeScript {
public:
    virtual ~UpgradeScript() = default;
    LoadContext ContextMask = LoadContext::None;

    virtual std::string Name() const = 0;
    virtual std::string Description() const = 0;
    virtual Version EarliestCompatibleVersion() const = 0;
    virtual Version TargetVersion() const = 0;
    // the C# class name (Type.Name)
    virtual std::string TypeName() const = 0;

    void Init(LoadContext contextMask, std::optional<std::string> nodeUrlCraft, std::optional<std::string> nodeUrlSFS);  // :25
    bool AppliesInContext(LoadContext ctx) const { return (ctx & ContextMask) != LoadContext::None; }  // :33
    virtual TestResult OnTest(ConfigNode& node, LoadContext loadContext, std::string& nodeName) = 0;
    virtual void OnUpgrade(ConfigNode& node, LoadContext loadContext, ConfigNode* parentNode) = 0;
    virtual TestResult Test(ConfigNode& n, LoadContext loadContext);  // :46
    virtual void Upgrade(ConfigNode& n, LoadContext loadContext);     // :86
    std::string ToString() const;                                     // :178

protected:
    virtual void OnInit() {}
    virtual bool CheckMinVersion(const Version& v) const { return v >= EarliestCompatibleVersion(); }
    virtual bool CheckMaxVersion(const Version& v) const { return v < TargetVersion(); }
    virtual Version GetCfgNodeVersion(const ConfigNode& n, LoadContext loadContext) const {
        return NodeUtil::GetCfgVersion(n, loadContext);
    }
    virtual TestResult VersionTest(const Version& v) const;  // :127
    std::optional<std::string> GetNodeURL(LoadContext loadContext) const;  // :141
    // :154 (nodes of each level visited from the last to the first)
    void RecurseNodes(ConfigNode& node, const std::vector<std::string>& urlNodes, int level,
                      const std::function<void(ConfigNode&, ConfigNode*)>& cb, ConfigNode* parent = nullptr);
    void LogTestResults(const std::string& nodeName, TestResult test) const;  // :169

    std::optional<std::string> nodeUrlCraft;
    std::optional<std::string> nodeUrlSFS;
};

// PartOffset.cs
class PartOffset : public UpgradeScript {
public:
    TestResult OnTest(ConfigNode& node, LoadContext loadContext, std::string& nodeName) override;  // :28
    void OnUpgrade(ConfigNode& node, LoadContext loadContext, ConfigNode* parentNode) override;     // :42

protected:
    void OnInit() override;  // :21 -> Setup
    virtual void Setup(std::string& pName, Vector3f& posOffset, Quaternionf& rotOffset) = 0;
    std::string partName;
    Vector3f positionOffset;
    Quaternionf rotationOffset;
    Quaternionf rotation0 = Quaternionf::identity();
    Quaternionf rotation;
    Vector3f position0;
    Vector3f position;
};

// PartReplace.cs
class PartReplace : public PartOffset {
public:
    struct attachNodeOffset {
        std::string id;
        Vector3f offset;
    };
    TestResult OnTest(ConfigNode& node, LoadContext loadContext, std::string& nodeName) override;  // :33
    void OnUpgrade(ConfigNode& node, LoadContext loadContext, ConfigNode* parentNode) override;     // :38

protected:
    void OnInit() override;  // :24 (Setup + PartLoader.Instance.AddPartReplacement)
    void Setup(std::string& pName, Vector3f& posOffset, Quaternionf& rotOffset) override;  // :29 (throws)
    virtual void Setup(std::string& pName, std::string& replacementPartName, std::string& refTransformName,
                       Vector3f& posOffset, Quaternionf& rotOffset, Vector3f& childPosOffset, Vector3f& att0Offset,
                       std::vector<attachNodeOffset>& attNOffsets) = 0;

    std::string replacementPartName;
    std::string referenceTransformName;
    Vector3f childPositionOffset;
    Vector3f attach0Offset;
    std::vector<attachNodeOffset> attachNodeOffsets;

private:
    void AdjustCraftChildPartsPos(ConfigNode& node, ConfigNode* parent, LoadContext loadContext,
                                  const std::vector<ConfigNode*>& partNodes, const Vector3f& offset);  // :173
    void AdjustSFSChildPartsPos(ConfigNode& node, LoadContext loadContext, const std::vector<ConfigNode*>& partNodes,
                                const Vector3f& offset);  // :210
    void AdjustSFSRootChildPartsPos(ConfigNode& node, LoadContext loadContext, const std::vector<ConfigNode*>& partNodes,
                                    const Vector3f& offsetTop, const Vector3f& offsetBottom);  // :227
    void AdjustPartByOffset(ConfigNode& part, LoadContext loadContext, const Vector3f& offset);  // :255
};

// ModuleReplace.cs
class ModuleReplace : public UpgradeScript {
public:
    TestResult OnTest(ConfigNode& node, LoadContext loadContext, std::string& nodeName) override;  // :22
    void OnUpgrade(ConfigNode& node, LoadContext loadContext, ConfigNode* parentNode) override;     // :33

protected:
    void OnInit() override;  // :14
    virtual void Setup(std::string& moduleName, std::string& newModuleName) = 0;
    std::string moduleName;
    std::string newModuleName;

private:
    std::vector<ConfigNode*> moduleNodes;
};

// v110_WheelModuleOverhaul.cs
class v110_WheelModuleOverhaul : public UpgradeScript {
public:
    enum class OldGearDeploymentStates : int { RETRACTED, DEPLOYED, DEPLOYING, RETRACTING, UNDEFINED };
    enum class OldLegDeploymentStates : int { RETRACTED, RETRACTING, DEPLOYING, DEPLOYED, BROKEN, REPAIRING };
    std::string Name() const override { return "1.1 Wheel Module Overhaul"; }
    std::string Description() const override;
    Version EarliestCompatibleVersion() const override { return Version(0, 21, 0); }
    Version TargetVersion() const override { return Version(1, 1, 0); }
    std::string TypeName() const override { return "v110_WheelModuleOverhaul"; }
    TestResult OnTest(ConfigNode& node, LoadContext loadContext, std::string& nodeName) override;  // :36
    void OnUpgrade(ConfigNode& node, LoadContext loadContext, ConfigNode* parentNode) override;     // :57
    static void SalvageBrakes(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext loadContext);  // :87
    void SalvageSteering(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext loadContext);       // :110
    static void SalvageMotor(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext loadContext);   // :133
    static void SalvageDamage(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext loadContext);  // :147
    static void SalvageGearDeployment(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext loadContext);  // :161
    static void SalvageLegDeployment(ConfigNode& dstNode, const ConfigNode& mNode, LoadContext loadContext);   // :191
    static std::string DeployStateUpgrade(OldGearDeploymentStates st);  // :221
    static std::string DeployStateUpgrade(OldLegDeploymentStates st);   // :233
};

// v180_ModuleAeroSurface.cs / v180_ModuleControlSurface.cs (the same script for two module classes; the prefab's
// ctrlSurfaceRange comes from SaveEnvironment::prefabModuleFloat)
class v180_ControlAuthority : public UpgradeScript {
public:
    // aero: ModuleAeroSurface / aeroAuthorityLimiter -> aeroDeployAngle; control: ModuleControlSurface /
    // authorityLimiter -> deployAngle
    explicit v180_ControlAuthority(bool aero) : aero_(aero) {}
    std::string Name() const override { return aero_ ? "1.8 ModuleAeroSurface" : "1.8 ModuleControlSurface"; }
    std::string Description() const override { return "Upgrades control authority settings for control surfaces."; }
    Version EarliestCompatibleVersion() const override { return Version(0, 21, 0); }
    Version TargetVersion() const override { return Version(1, 8, 0); }
    std::string TypeName() const override { return aero_ ? "v180_ModuleAeroSurface" : "v180_ModuleControlSurface"; }
    TestResult OnTest(ConfigNode& node, LoadContext loadContext, std::string& nodeName) override;  // :17
    void OnUpgrade(ConfigNode& node, LoadContext loadContext, ConfigNode* parentNode) override;     // :36

private:
    float GetBaseModuleCtrlSurfaceRange(const ConfigNode& node, LoadContext loadContext) const;  // :52
    bool aero_;
};

// SaveUpgradePipeline.cs
class SaveUpgradePipeline {
public:
    using Row = std::vector<std::pair<UpgradeScript*, LogEntry>>;  // Dictionary<UpgradeScript, LogEntry> (insertion order)
    std::vector<std::unique_ptr<UpgradeScript>> upgradeScripts;
    std::vector<Row> runLog;
    bool initialized = false;
    // event OnSetCfgNodeVersion (Init installs OnSetCfgNodeVersion_Default)
    std::function<void(ConfigNode&, LoadContext, const Version&)> OnSetCfgNodeVersion;

    // SaveUpgradePipeline.cs:31 (scripts = the attributed UpgradeScript types in discovery order; each is Init'ed with
    // its UpgradeModule attribute and the list is stable-sorted by TargetVersion, EarliestCompatibleVersion)
    void Init(std::vector<std::pair<std::unique_ptr<UpgradeScript>, UpgradeModule>> scripts);
    // :50 / 56 (returns the upgraded copy, or `node` when nothing was upgraded)
    ConfigNode::NodePtr Run(const ConfigNode::NodePtr& node, LoadContext ctx, const Version& AppVersion, bool& runSuccess,
                            std::string& runInfo);
    ConfigNode::NodePtr Run(const ConfigNode::NodePtr& node, LoadContext ctx, const Version& AppVersion, bool& runSuccess,
                            std::string& runInfo, std::vector<Row>& log);
    TestResult RunTest(UpgradeScript& uSc, ConfigNode* node, LoadContext loadContext);  // :187
    ConfigNode* RunUpgrade(UpgradeScript& uSc, ConfigNode* node, LoadContext loadContext);  // :204
    void SetCfgNodeVersion(ConfigNode& node, LoadContext ctx, const Version& version);  // :222
    std::string PrintLog(const std::vector<Row>& log, const std::string& cfgName, const std::string& runInfo) const;  // :227

    static bool SanityCheck(const UpgradeScript& uSC, const Version& AppVersion);  // :148
    static bool TestExceptionCases(const std::vector<Row>& log);                   // :162
    static void OnSetCfgNodeVersion_Default(ConfigNode& n, LoadContext loadContext, const Version& version);  // :176

private:
    IterationResult RunIteration(ConfigNode& srcNode, ConfigNode::NodePtr& node, LoadContext ctx,
                                 const std::vector<UpgradeScript*>& scripts, std::vector<Row>& log);  // :100
    Version lowestVersion_;
};

// The 40 stock scripts with their [UpgradeModule] attributes, in discovery order
std::vector<std::pair<std::unique_ptr<UpgradeScript>, UpgradeModule>> CreateStockUpgradeScripts();

}  // namespace ksp::SaveUpgradePipeline

namespace ksp {

// KSPUpgradePipeline.cs (static class)
class KSPUpgradePipeline {
public:
    enum class UpgradeFailOption : int { Cancel, LoadAnyway };
    static inline bool Enabled = true;
    static inline int FailDialogIndex = 0;

    // KSPUpgradePipeline.cs:24 (the pipeline with the stock scripts, created on first use)
    static SaveUpgradePipeline::SaveUpgradePipeline& Pipeline();
    static bool Init() { return Pipeline().initialized; }
    // KSPUpgradePipeline.cs:35. On failure KSP shows a dialog with the run log ("Cancel" / "Attempt to Load Anyway");
    // here `failDialog` answers it (default: Cancel) and onFail receives (option, LoadAnyway ? n : nullptr).
    static void Process(const ConfigNode::NodePtr& n, const std::string& saveName, SaveUpgradePipeline::LoadContext loadContext,
                        const std::function<void(const ConfigNode::NodePtr&)>& onSucceed,
                        const std::function<void(UpgradeFailOption, const ConfigNode::NodePtr&)>& onFail,
                        const std::function<UpgradeFailOption(const std::string& runInfo)>& failDialog = nullptr);
    // kspcore: drop the cached pipeline (tests)
    static void Reset();
};

}  // namespace ksp
