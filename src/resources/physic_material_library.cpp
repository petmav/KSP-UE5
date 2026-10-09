#include "ksp/resources/physic_material_library.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/parts/ksp_enums.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace {
PhysicMaterialLibrary* g_pml = nullptr;

// Enum.Parse(typeof(PhysicMaterialCombine), value)
PhysicMaterialCombine ParseCombine(const std::string& v) {
    static const EnumTable t{"PhysicMaterialCombine", false, "Unity/UnityEngine.PhysicsModule/UnityEngine/PhysicMaterialCombine.cs", 3,
                             {{"Average", 0}, {"Minimum", 2}, {"Multiply", 1}, {"Maximum", 3}}};
    return static_cast<PhysicMaterialCombine>(t.Parse(v));
}
const char* CombineName(PhysicMaterialCombine c) {
    switch (c) {
        case PhysicMaterialCombine::Average: return "Average";
        case PhysicMaterialCombine::Multiply: return "Multiply";
        case PhysicMaterialCombine::Minimum: return "Minimum";
        case PhysicMaterialCombine::Maximum: return "Maximum";
    }
    return "Average";
}
}  // namespace

PhysicMaterialDefinition::PhysicMaterialDefinition(std::string_view name, std::string_view displayName,
                                                   float dynamicFriction, float staticFriction, float bounciness,
                                                   PhysicMaterialCombine frictionCombine, PhysicMaterialCombine bounceCombine)
    : name_(name), displayName_(displayName), id_(MonoStringHash(name)), dynamicFriction_(dynamicFriction),
      staticFriction_(staticFriction), bounciness_(bounciness), frictionCombine_(frictionCombine),
      bounceCombine_(bounceCombine) {}

// PhysicMaterialDefinition.cs:58
void PhysicMaterialDefinition::Load(const ConfigNode& node) {
    const auto n = node.GetValue("name");
    if (!n) throw NullReferenceException("Object reference not set to an instance of an object");
    name_ = *n;
    id_ = MonoStringHash(name_);
    displayName_ = node.HasValue("displayName") ? *node.GetValue("displayName") : KSPUtil::PrintModuleName(name_);
    if (node.HasValue("dynamicFriction")) dynamicFriction_ = cs::FloatParse(*node.GetValue("dynamicFriction"));
    if (node.HasValue("staticFriction")) staticFriction_ = cs::FloatParse(*node.GetValue("staticFriction"));
    if (node.HasValue("bounciness")) bounciness_ = cs::FloatParse(*node.GetValue("bounciness"));
    if (node.HasValue("frictionCombine")) frictionCombine_ = ParseCombine(*node.GetValue("frictionCombine"));
    if (node.HasValue("bounceCombine")) bounceCombine_ = ParseCombine(*node.GetValue("bounceCombine"));
    config_ = ConfigNode::Create();
    node.CopyTo(*config_);
}

void PhysicMaterialDefinition::Save(ConfigNode& node) const {
    node.AddValue("name", name_);
    node.AddValue("displayName", displayName_);
    node.AddValue("dynamicFriction", dynamicFriction_);
    node.AddValue("staticFriction", staticFriction_);
    node.AddValue("bounciness", bounciness_);
    node.AddValue("frictionCombine", CombineName(frictionCombine_));
    node.AddValue("bounceCombine", CombineName(bounceCombine_));
}

PhysicMaterialLibrary::PhysicMaterialLibrary() { g_pml = this; }
PhysicMaterialLibrary::~PhysicMaterialLibrary() {
    if (g_pml == this) g_pml = nullptr;
}
PhysicMaterialLibrary* PhysicMaterialLibrary::Instance() { return g_pml; }

// PhysicMaterialLibrary.cs:44
void PhysicMaterialLibrary::LoadDefinitions(const GameDatabase& db) {
    physicMaterials.clear();
    for (ConfigNode* n : db.GetConfigNodes("PHYSICMATERIAL_DEFINITION")) {
        CreatePhysicMaterials(*n);
        log::Info("Resource " + (n->HasValue("name") ? *n->GetValue("name") : n->name) + " added to database");
    }
    auto it = physicMaterials.find("Default");
    if (it != physicMaterials.end()) {
        DefaultMaterial = std::make_unique<PhysicMaterialDefinition>(*it->second);
    } else {
        // PhysicMaterialLibrary.cs:100 CreateDefaultPhysicMaterial; the material is named string.Concat(DefaultMaterial,
        // "PhysicMaterial") = the object's type name + "PhysicMaterial" (sic)
        DefaultMaterial = std::make_unique<PhysicMaterialDefinition>("Default", "#autoLOC_6011085", 0.4f, 0.4f, 0.0f,
                                                                     PhysicMaterialCombine::Average,
                                                                     PhysicMaterialCombine::Average);
        DefaultMaterial->materialName = "PhysicMaterialDefinitionPhysicMaterial";
    }
}

// PhysicMaterialLibrary.cs:70
void PhysicMaterialLibrary::CreatePhysicMaterials(const ConfigNode& node) {
    if (!node.HasValue("name")) {
        log::Warning("Config has no name field");
        return;
    }
    const std::string value = *node.GetValue("name");
    if (physicMaterials.count(value)) {
        log::Warning("physicMaterialsDictionary already contains definition for '" + value + "'");
        return;
    }
    auto def = std::make_unique<PhysicMaterialDefinition>();
    def->Load(node);
    def->materialName = def->name() + "PhysicMaterial";
    physicMaterials.emplace(def->name(), std::move(def));
}

}  // namespace ksp
