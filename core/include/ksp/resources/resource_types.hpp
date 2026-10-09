// WP-15 resource flow: the small value types and interfaces of the resource system.
//   ResourceFlowMode.cs, ResourceTransferMode.cs, ResourceRatio.cs, ConversionRecipe.cs, ConverterResults.cs,
//   IResourceBroker.cs, IResourceConsumer.cs, IToggleCrossfeed.cs, HarvestTypes.cs, AbundanceRequest.cs,
//   DepletionData.cs, DepletionNode.cs, BiomeLockData.cs, PlanetScanData.cs, PlanetaryResource.cs, DistributionData.cs,
//   ResourceData.cs.
// (ConversionRecipe / ConverterResults / BiomeLockData are not in the WP-15 file list of work_packages.csv but are
// ResourceConverter's / ResourceMap's data types, so they are ported with them.)
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"

namespace ksp {

class Part;
class PartResourceDefinition;

// ResourceFlowMode.cs:1 (NULL is a macro in C++: the member is NULL_)
enum class ResourceFlowMode : int32_t {
    NO_FLOW = 0,
    ALL_VESSEL = 1,
    STAGE_PRIORITY_FLOW = 2,
    STACK_PRIORITY_SEARCH = 3,
    ALL_VESSEL_BALANCE = 4,
    STAGE_PRIORITY_FLOW_BALANCE = 5,
    STAGE_STACK_FLOW = 6,
    STAGE_STACK_FLOW_BALANCE = 7,
    NULL_ = 8
};

// ResourceTransferMode.cs:1
enum class ResourceTransferMode : int32_t { NONE = 0, DIRECT = 1, PUMP = 2 };

// HarvestTypes.cs:4
enum class HarvestTypes : int32_t { Planetary = 0, Oceanic = 1, Atmospheric = 2, Exospheric = 3 };

inline ResourceFlowMode AsFlowMode(int32_t v) { return static_cast<ResourceFlowMode>(v); }

// ResourceRatio.cs:5 (a struct: default(ResourceRatio) has FlowMode 0 = NO_FLOW, the constructors without a mode use NULL)
struct ResourceRatio {
    std::string ResourceName;  // C# string (null when never set)
    double Ratio = 0.0;
    bool DumpExcess = false;
    ResourceFlowMode FlowMode = ResourceFlowMode::NO_FLOW;

    ResourceRatio() = default;
    ResourceRatio(std::string resourceName, double ratio, bool dumpExcess)  // :15
        : ResourceName(std::move(resourceName)), Ratio(ratio), DumpExcess(dumpExcess), FlowMode(ResourceFlowMode::NULL_) {}
    ResourceRatio(std::string resourceName, double ratio, bool dumpExcess, ResourceFlowMode flowMode)  // :23
        : ResourceName(std::move(resourceName)), Ratio(ratio), DumpExcess(dumpExcess), FlowMode(flowMode) {}
    void Load(const ConfigNode& node);  // :31
    void Save(ConfigNode& node) const;  // :55
};

// ConverterResults.cs
struct ConverterResults {
    std::string Status;  // null in C# until a status is set
    double TimeFactor = 0.0;
};

// ConversionRecipe.cs:3
class ConversionRecipe {
public:
    ConversionRecipe() { Clear(); }
    std::vector<ResourceRatio>& Inputs() { return inputs_; }
    std::vector<ResourceRatio>& Outputs() { return outputs_; }
    std::vector<ResourceRatio>& Requirements() { return reqs_; }
    float FillAmount = 1.0f;
    float TakeAmount = 1.0f;
    void SetInputs(const std::vector<ResourceRatio>* value) {  // :27 (null leaves the list alone)
        if (value) inputs_ = *value;
    }
    void SetOutputs(const std::vector<ResourceRatio>* value) {  // :35
        if (value) outputs_ = *value;
    }
    void SetRequirements(const std::vector<ResourceRatio>* value) {  // :43
        if (value) reqs_ = *value;
    }
    void Clear() {  // :51
        inputs_.clear();
        outputs_.clear();
        reqs_.clear();
        FillAmount = 1.0f;
        TakeAmount = 1.0f;
    }

private:
    std::vector<ResourceRatio> inputs_, outputs_, reqs_;
};

// IResourceBroker.cs
class IResourceBroker {
public:
    virtual ~IResourceBroker() = default;
    virtual double AmountAvailable(Part* part, const std::string& resName, double deltaTime, ResourceFlowMode flowMode) = 0;
    virtual double RequestResource(Part* part, const std::string& resName, double resAmount, double deltaTime, ResourceFlowMode flowMode) = 0;
    virtual double StorageAvailable(Part* part, const std::string& resName, double deltaTime, ResourceFlowMode flowMode, double FillAmount) = 0;
    virtual double StoreResource(Part* part, const std::string& resName, double resAmount, double deltaTime, ResourceFlowMode flowMode) = 0;
    virtual double AmountAvailable(Part* part, int resID, double deltaTime, ResourceFlowMode flowMode) = 0;
    virtual double RequestResource(Part* part, int resID, double resAmount, double deltaTime, ResourceFlowMode flowMode) = 0;
    virtual double StorageAvailable(Part* part, int resID, double deltaTime, ResourceFlowMode flowMode, double FillAmount) = 0;
    virtual double StoreResource(Part* part, int resID, double resAmount, double deltaTime, ResourceFlowMode flowMode) = 0;
};

// IResourceConsumer.cs: `List<PartResourceDefinition> GetConsumedResources()`
class IResourceConsumer {
public:
    virtual ~IResourceConsumer() = default;
    virtual std::vector<const PartResourceDefinition*> GetConsumedResources() = 0;
};

// IToggleCrossfeed.cs
class IToggleCrossfeed {
public:
    virtual ~IToggleCrossfeed() = default;
    virtual bool CrossfeedToggleableEditor() = 0;
    virtual bool CrossfeedToggleableFlight() = 0;
    virtual bool CrossfeedRequiresTech() = 0;
    virtual std::string CrossfeedTech() = 0;
    virtual bool CrossfeedHasTech() = 0;
};

// ---- planetary resource records (the data classes of the resource map) --------------------------------------------------
// DistributionData.cs:3
struct DistributionData {
    float PresenceChance = 0, MinAbundance = 0, MaxAbundance = 0, Variance = 0, Dispersal = 0;
    float MinAltitude = 0, MaxAltitude = 0, MinRange = 0, MaxRange = 0;
    bool HasVariableAltitude() const { return static_cast<double>(MinAltitude) > 1E-09 || static_cast<double>(MaxAltitude) > 1E-09; }  // :27
};

// ResourceData.cs:1
struct ResourceData {
    std::string ResourceName, PlanetName, BiomeName;
    int ResourceType = 0;
    std::shared_ptr<DistributionData> Distribution;
};

// DepletionNode.cs:1 / DepletionData.cs:3 / BiomeLockData.cs / PlanetScanData.cs / PlanetaryResource.cs
struct DepletionNode {
    int X = 0, Y = 0;
    float Value = 0;
    double LastUpdate = 0;
};
struct DepletionData {
    int PlanetId = 0;
    std::string ResourceName;
    std::vector<std::shared_ptr<DepletionNode>> DepletionNodes;
};
struct BiomeLockData {
    int PlanetId = 0;
    std::string BiomeName;
};
struct PlanetScanData {
    int PlanetId = 0;
};
struct PlanetaryResource {
    float fraction = 0;
    int resourceID = 0;
    std::string resourceName;
};

// AbundanceRequest.cs:1 (a struct)
struct AbundanceRequest {
    double Latitude = 0, Longitude = 0;
    int BodyId = 0;
    std::string ResourceName;
    HarvestTypes ResourceType = HarvestTypes::Planetary;
    double Altitude = 0;
    bool CheckForLock = false;
    std::string BiomeName;
    bool ExcludeVariance = false;
};

}  // namespace ksp
