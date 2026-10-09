// ResourceBroker.cs, ResourceConverter.cs, ResourceSetup.cs (WP-15).
//
// ResourceBroker: IResourceBroker over Part.RequestResource / GetConnectedResourceTotals with the ElectricCharge warp scaling
// (above ResourceSetup.ResConfig.ECMinScale x 0.02 s per step the request is divided by deltaTime / 0.02).
// ResourceConverter.ProcessRecipe: one converter step over a ConversionRecipe (requirements, inputs, outputs, fill/take amounts).
// ResourceSetup: the RESOURCE_CONFIGURATION node (stock: GameData/Squad/Resources/ResourceDefaults.cfg).
#pragma once

#include <memory>
#include <string>

#include "ksp/database/game_database.hpp"
#include "ksp/resources/resource_types.hpp"

namespace ksp {

class Part;
class PartModule;

// ResourceSetup.cs:4 (the GameObject singleton and OnDestroy are the host's; `Instance()` creates the object on first use)
class ResourceSetup {
public:
    // ResourceSetup.ResourceConfig (:8): the four properties LoadNodeProperties<ResourceConfig> fills from the node
    struct ResourceConfig {
        bool HeatEnabled = false;
        int ECMinScale = 0;
        int OverlayStyle = 0;
        bool ShowDebugOptions = false;
    };
    static ResourceSetup& Instance();                       // :20
    static void DestroyInstance();                          // OnDestroy (:26)
    // :22 ResConfig: loaded once from GameDatabase.Instance (null when there is no RESOURCE_CONFIGURATION node)
    ResourceConfig* ResConfig();
    // test/host hook: the GameDatabase to read (default GameDatabase::Instance())
    static inline const GameDatabase* database = nullptr;
    static void ResetConfig();                              // (the C# static _resConfig is never reset; the host does so on a new database)

private:
    ResourceConfig* LoadResourceConfig();                   // :33
    static inline std::unique_ptr<ResourceConfig> resConfig_;
    static inline bool resConfigLoaded_ = false;
    static inline std::unique_ptr<ResourceSetup> instance_;
};

// ResourceBroker.cs:1
class ResourceBroker : public IResourceBroker {
public:
    double AmountAvailable(Part* part, const std::string& resName, double deltaTime, ResourceFlowMode flowMode) override;  // :3
    double AmountAvailable(Part* part, int resID, double deltaTime, ResourceFlowMode flowMode) override;                   // :8
    double RequestResource(Part* part, const std::string& resName, double resAmount, double deltaTime, ResourceFlowMode flowMode) override;  // :23
    double RequestResource(Part* part, int resID, double resAmount, double deltaTime, ResourceFlowMode flowMode) override;                   // :28
    double StorageAvailable(Part* part, const std::string& resName, double deltaTime, ResourceFlowMode flowMode, double FillAmount) override;  // :43
    double StorageAvailable(Part* part, int resID, double deltaTime, ResourceFlowMode flowMode, double FillAmount) override;                   // :48
    double StoreResource(Part* part, const std::string& resName, double resAmount, double deltaTime, ResourceFlowMode flowMode) override;     // :58
    double StoreResource(Part* part, int resID, double resAmount, double deltaTime, ResourceFlowMode flowMode) override;                       // :63
};

// ResourceConverter.cs:5
class ResourceConverter {
public:
    explicit ResourceConverter(IResourceBroker* broker) : broker_(broker) {}   // :13
    ResourceConverter() : owned_(std::make_unique<ResourceBroker>()), broker_(owned_.get()) {}  // :18
    // :23 (resModule may be null; it is only passed to GameEvents.OnResourceConverterOutput)
    ConverterResults ProcessRecipe(double deltaTime, ConversionRecipe& recipe, Part* resPart, PartModule* resModule, float efficiencyBonus);
    static void CacheLocalStrings();                                            // :163

private:
    std::unique_ptr<IResourceBroker> owned_;
    IResourceBroker* broker_;
    static inline std::string cacheAutoLOC_6002353, cacheAutoLOC_261274;
};

}  // namespace ksp
