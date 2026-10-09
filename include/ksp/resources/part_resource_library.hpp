// Resource definitions (WP-05): PartResourceDrainDefinition.cs, PartResourceDefinition.cs, PartResourceDefinitionList.cs,
// PartResourceLibrary.cs. Loaded from the RESOURCE_DEFINITION configs of the GameDatabase.
//
// Ids are string.GetHashCode() of the name as Unity's Mono computes it (MonoStringHash: h = h * 31 + c over UTF-16
// code units, two at a time as in mono/mcs/class/corlib/ReferenceSources/String.cs). The ids are only dictionary keys
// inside the game (never saved); the value itself is not verified against a capture.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/unity/color.hpp"

namespace ksp {

// Mono's String.GetHashCode (see the file comment).
int32_t MonoStringHash(std::string_view s);

// PartResourceDrainDefinition.cs
class PartResourceDrainDefinition {
public:
    bool isDrainable() const { return isDrainable_; }
    bool showDrainFX() const { return showDrainFX_; }
    int32_t drainFXPriority() const { return drainFXPriority_; }
    float drainForceISP() const { return drainForceISP_; }
    const std::string& drainFXDefinition() const { return drainFXDefinition_; }
    void Load(const ConfigNode& node);  // PartResourceDrainDefinition.cs:37
    void Save(ConfigNode& node) const;

private:
    bool isDrainable_ = true;
    bool showDrainFX_ = true;
    int32_t drainFXPriority_ = 5;
    float drainForceISP_ = 50.0f;
    std::string drainFXDefinition_ = "gasDraining";
};

// PartResourceDefinition.cs
class PartResourceDefinition {
public:
    PartResourceDefinition() = default;
    explicit PartResourceDefinition(std::string_view name);
    PartResourceDefinition(std::string_view name, const Color& color);

    const std::string& name() const { return name_; }
    const std::string& displayName() const { return displayName_; }
    int32_t id() const { return id_; }
    float density() const { return density_; }
    float volume() const { return volume_; }
    const std::string& abbreviation() const { return abbreviation_; }
    float unitCost() const { return unitCost_; }
    float specificHeatCapacity() const { return specificHeatCapacity_; }
    const ConfigNode* Config() const { return config_.get(); }
    const Color& color() const { return color_; }
    int32_t resourceFlowMode() const { return resourceFlowMode_; }      // ResourceFlowMode (kEnum_ResourceFlowMode)
    int32_t resourceTransferMode() const { return resourceTransferMode_; }  // ResourceTransferMode
    bool isTweakable() const { return isTweakable_; }
    bool isVisible() const { return isVisible_; }
    const PartResourceDrainDefinition& partResourceDrainDefinition() const { return drain_; }

    void Load(const ConfigNode& node);           // PartResourceDefinition.cs:99
    std::string GetShortName(int length = 2) const;  // PartResourceDefinition.cs:163
    void Save(ConfigNode& node) const;           // PartResourceDefinition.cs:172

private:
    std::string name_;
    std::string displayName_;
    int32_t id_ = -1;
    float density_ = 1.0f;
    float volume_ = 5.0f;
    std::string abbreviation_;
    float unitCost_ = 0.0f;
    float specificHeatCapacity_ = 0.0f;
    ConfigNode::NodePtr config_;
    Color color_{1.0f, 1.0f, 1.0f, 1.0f};
    int32_t resourceFlowMode_ = 0;
    int32_t resourceTransferMode_ = 0;
    bool isTweakable_ = false;
    bool isVisible_ = true;
    PartResourceDrainDefinition drain_;
};

// PartResourceDefinitionList.cs: a Dictionary<int, PartResourceDefinition> keyed by the name hash; enumeration in
// insertion order (no removals happen).
class PartResourceDefinitionList {
public:
    const PartResourceDefinition* operator[](std::string_view name) const;
    const PartResourceDefinition* operator[](int32_t id) const;
    int Count() const { return static_cast<int>(defs_.size()); }
    const PartResourceDefinition* Add(const ConfigNode& node);  // PartResourceDefinitionList.cs:45
    bool Contains(std::string_view name) const;
    void Clear() { defs_.clear(); }
    auto begin() const { return defs_.begin(); }
    auto end() const { return defs_.end(); }

private:
    std::vector<std::unique_ptr<PartResourceDefinition>> defs_;
};

class GameDatabase;

// PartResourceLibrary.cs (a scene singleton in KSP: Instance is the most recently constructed live library)
class PartResourceLibrary {
public:
    PartResourceLibrary();
    ~PartResourceLibrary();
    PartResourceLibrary(const PartResourceLibrary&) = delete;
    PartResourceLibrary& operator=(const PartResourceLibrary&) = delete;
    static PartResourceLibrary* Instance();

    static int32_t ElectricityHashcode();
    PartResourceDefinitionList resourceDefinitions;

    // PartResourceLibrary.cs:44: GameDatabase.Instance.GetConfigNodes("RESOURCE_DEFINITION")
    void LoadDefinitions(const GameDatabase& db);
    const PartResourceDefinition* GetDefinition(std::string_view name) const { return resourceDefinitions[name]; }
    const PartResourceDefinition* GetDefinition(int32_t id) const { return resourceDefinitions[id]; }
    // PartResourceLibrary.cs:72/77 (ResourceFlowMode.NULL = 8 when the resource is unknown)
    static int32_t GetDefaultFlowMode(std::string_view resourceName);
    static int32_t GetDefaultFlowMode(int32_t resourceID);
};

}  // namespace ksp
