// PhysicMaterialDefinition.cs / PhysicMaterialLibrary.cs (data side): PHYSICMATERIAL_DEFINITION configs. The Unity
// PhysicMaterial each definition creates is represented by its five values (the physics engine of the shell builds the
// real material from them). Stock GameData defines none, so the library holds only the code-built "Default"
// (0.4 / 0.4 / 0, Average / Average, displayName "#autoLOC_6011085" unlocalised: sic).
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/physics/physics_globals.hpp"  // ksp::PhysicMaterialCombine

namespace ksp {

class GameDatabase;

class PhysicMaterialDefinition {
public:
    PhysicMaterialDefinition() = default;
    PhysicMaterialDefinition(std::string_view name, std::string_view displayName, float dynamicFriction,
                             float staticFriction, float bounciness, PhysicMaterialCombine frictionCombine,
                             PhysicMaterialCombine bounceCombine);
    const std::string& name() const { return name_; }
    const std::string& displayName() const { return displayName_; }
    int32_t id() const { return id_; }
    float dynamicFriction() const { return dynamicFriction_; }
    float staticFriction() const { return staticFriction_; }
    float bounciness() const { return bounciness_; }
    PhysicMaterialCombine frictionCombine() const { return frictionCombine_; }
    PhysicMaterialCombine bounceCombine() const { return bounceCombine_; }
    const ConfigNode* Config() const { return config_.get(); }
    std::string materialName;  // the PhysicMaterial's name: <name> + "PhysicMaterial"
    void Load(const ConfigNode& node);  // PhysicMaterialDefinition.cs:58
    void Save(ConfigNode& node) const;

private:
    std::string name_;
    std::string displayName_;
    int32_t id_ = -1;
    float dynamicFriction_ = 0.0f;
    float staticFriction_ = 0.0f;
    float bounciness_ = 0.0f;
    PhysicMaterialCombine frictionCombine_ = PhysicMaterialCombine::Average;
    PhysicMaterialCombine bounceCombine_ = PhysicMaterialCombine::Average;
    ConfigNode::NodePtr config_;
};

class PhysicMaterialLibrary {
public:
    PhysicMaterialLibrary();
    ~PhysicMaterialLibrary();
    PhysicMaterialLibrary(const PhysicMaterialLibrary&) = delete;
    PhysicMaterialLibrary& operator=(const PhysicMaterialLibrary&) = delete;
    static PhysicMaterialLibrary* Instance();

    // PhysicMaterialLibrary.cs:44: GameDatabase GetConfigNodes("PHYSICMATERIAL_DEFINITION"), then the Default
    void LoadDefinitions(const GameDatabase& db);
    void CreatePhysicMaterials(const ConfigNode& node);  // PhysicMaterialLibrary.cs:70
    std::map<std::string, std::unique_ptr<PhysicMaterialDefinition>, std::less<>> physicMaterials;
    // DefaultMaterial: the "Default" definition when GameData has one (KSP keeps its Unity-serialized object and only
    // swaps the material in), else the code-built default
    std::unique_ptr<PhysicMaterialDefinition> DefaultMaterial;
};

}  // namespace ksp
