// WP-39: the two small CommNet part modules: CommNet/ModuleRelayEnabler.cs and CommNet/ModuleProbeControlPoint.cs.
//
// ModuleRelayEnabler: isRelaying follows the electric charge draw of FixedUpdate (resHandler.UpdateModuleResourceInputs with threshold 0.9),
// CanRelay / CanRelayUnloaded (the `canRelay` value of the proto module), OnSave writes `canRelay`.
// ModuleProbeControlPoint: CanControl / CanControlUnloaded through the Missions adjuster list (IProbeControlPointAdjuster stands for
// AdjusterProbeControlPointBase; the unloaded list comes from CommNetModuleEnvironment::protoProbeAdjusters). The C# loop of
// IsProbeControlPointBroken bounds on the argument list but indexes the module's own cache: kept as written.
//
// Seams: the Unity FixedUpdate message is the public FixedUpdate() (the host calls it once per physics step); GetInfo of ModuleRelayEnabler needs
// ModuleResourceHandler.PrintModuleResources (not modelled by WP-15: returns the empty string).
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ksp/commnet/comm_network.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/resources/resource_types.hpp"

namespace ksp::CommNet {

// Expansions.Missions.Adjusters.AdjusterProbeControlPointBase (MakingHistory; the adjuster classes are not ported)
class IProbeControlPointAdjuster {
public:
    virtual ~IProbeControlPointAdjuster() = default;
    virtual bool IsProbeControlPointBroken() = 0;
};

// Expansions.Missions.Adjusters.AdjusterDataTransmitterBase
class IDataTransmitterAdjuster {
public:
    virtual ~IDataTransmitterAdjuster() = default;
    virtual double ApplyPowerAdjustment(double power) = 0;
    virtual bool IsDataTransmitterBroken() = 0;
};

struct CommNetModuleEnvironment {
    // ProtoPartModuleSnapshot.GetListOfActiveAdjusters<AdjusterProbeControlPointBase / AdjusterDataTransmitterBase>() (the ADJUSTERS node of
    // the snapshot minus ADJUSTERSTOREMOVE). Unset: no adjusters.
    static inline std::function<std::vector<IProbeControlPointAdjuster*>(ProtoPartModuleSnapshot&)> protoProbeAdjusters;
    static inline std::function<std::vector<IDataTransmitterAdjuster*>(ProtoPartModuleSnapshot&)> protoTransmitterAdjusters;
    static void Reset() {
        protoProbeAdjusters = nullptr;
        protoTransmitterAdjusters = nullptr;
    }
};

// ModuleRelayEnabler.cs:6
class ModuleRelayEnabler : public PartModule, public IResourceConsumer, public IRelayEnabler {
public:
    explicit ModuleRelayEnabler(const ModuleClassInfo& cls) : PartModule(cls) {}
    bool isRelaying = true;

    std::vector<const PartResourceDefinition*> GetConsumedResources() override { return consumedResources_; }  // :18
    void OnAwake() override;                       // :23
    void OnSave(ConfigNode& node) override;        // :40
    void OnStart(StartState state) override;       // :46
    virtual void FixedUpdate();                    // :51
    bool CanRelay() override { return isRelaying; }  // :76
    bool CanRelayUnloaded(ProtoPartModuleSnapshot* mSnap) override;  // :81

private:
    std::vector<const PartResourceDefinition*> consumedResources_;
};

// ModuleProbeControlPoint.cs:7
class ModuleProbeControlPoint : public PartModule {
public:
    explicit ModuleProbeControlPoint(const ModuleClassInfo& cls) : PartModule(cls) {}
    int minimumCrew() const { return Get<int>("minimumCrew"); }
    bool multiHop() const { return Get<bool>("multiHop"); }

    bool CanControl();                                         // :23
    bool CanControlUnloaded(ProtoPartModuleSnapshot* mSnap);   // :28
    // OnModuleAdjusterAdded / OnModuleAdjusterRemoved (:41 / :50)
    void OnModuleAdjusterAdded(IProbeControlPointAdjuster* adjuster) { adjusterCache_.push_back(adjuster); }
    void OnModuleAdjusterRemoved(IProbeControlPointAdjuster* adjuster);
    std::string GetInfo() override;                            // :83
    std::string GetModuleDisplayName() override;               // :88

protected:
    bool IsProbeControlPointBroken(const std::vector<IProbeControlPointAdjuster*>& adjusterList);  // :57

private:
    std::vector<IProbeControlPointAdjuster*> adjusterCache_;
};

// the `canRelay` / `canComm` loop of CanRelayUnloaded / CanCommUnloaded: the last entry of that name; "False" = off; absent = true
bool ProtoValueFlag(ProtoPartModuleSnapshot* mSnap, const char* valueName);

}  // namespace ksp::CommNet

namespace ksp::detail {
void RegisterCommNetModules();  // ModuleRelayEnabler, ModuleProbeControlPoint, ModuleDataTransmitter, ModuleKerbNetAccess, CommNetVessel, CommNetScenario
}  // namespace ksp::detail
