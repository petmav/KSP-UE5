// Module behaviours WP-06 ports because the part-database gate observes them: the OnAwake / OnLoad logic of the stock
// modules that changes KSPField values or Part prefab data while the prefab is being compiled (PartLoader.ParsePart ->
// Part.AddModule -> PartModule.Load). Everything else a module does (OnStart ..., events, actions, GetInfo) belongs to the
// package that owns the module (STATUS.md WP-06 lists them); those classes are instantiated as the generic PartModule.
#include "ksp/core/log.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/available_part.hpp"

namespace ksp {
namespace {

// ModuleToggleCrossfeed.cs
class ModuleToggleCrossfeed : public PartModule {
public:
    using PartModule::PartModule;

    // ModuleToggleCrossfeed.cs:116: defaultCrossfeedStatus = part.fuelCrossFeed while the prefab has no partInfo
    void OnAwake() override {
        PartModule::OnAwake();
        if (part) defaultCrossfeedStatus = part->fields.fuelCrossFeed;
    }
    // ModuleToggleCrossfeed.cs:127
    void OnLoad(const ConfigNode& node) override {
        PartModule::OnLoad(node);
        UpdateCrossfeed(false);
    }
    // ModuleToggleCrossfeed.cs:155 (GameEvents.onPartCrossfeedStateChange has no listener while a prefab compiles; the
    // toggle event/action gui names are Events/Actions entries)
    void UpdateCrossfeed(bool /*fireEvent*/) {
        const bool crossfeedStatus = Get<bool>("crossfeedStatus");
        if (part) part->fields.fuelCrossFeed = crossfeedStatus;
        if (BaseEvent* e = Events()["ToggleEvent"]) e->SetGuiName(Get<std::string>(crossfeedStatus ? "disableText" : "enableText"));
        if (BaseAction* a = Actions()["ToggleAction"]) a->SetGuiName(Get<std::string>("toggleText"));
        if (BaseAction* a = Actions()["EnableAction"]) a->SetGuiName(Get<std::string>("enableText"));
        if (BaseAction* a = Actions()["DisableAction"]) a->SetGuiName(Get<std::string>("disableText"));
    }
    bool defaultCrossfeedStatus = false;
};

// ModuleCargoPart.cs:96
class ModuleCargoPart : public PartModule {
public:
    using PartModule::PartModule;
    void OnLoad(const ConfigNode& node) override {
        PartModule::OnLoad(node);
        if (Get<int32_t>("stackableQuantity") > 1 && part && !part->Resources.empty()) {
            log::Info("[ModuleCargoPart]: Setting stackableQuantity to 1 as " + part->gameObjectName + " contains resources");
            Set<int32_t>("stackableQuantity", 1);
        }
    }
};

}  // namespace

namespace detail {
void RegisterBuiltinModules() {
    ModuleFactory::Register("ModuleToggleCrossfeed", [](const ModuleClassInfo& c) { return std::make_shared<ModuleToggleCrossfeed>(c); });
    ModuleFactory::Register("ModuleCargoPart", [](const ModuleClassInfo& c) { return std::make_shared<ModuleCargoPart>(c); });
}
}  // namespace detail

}  // namespace ksp
