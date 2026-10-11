// WP-53: ControlledBase.cs, RoboticCollisions.cs, SerenityUtils.cs (Assembly-CSharp/Expansions/Serenity).
//
// ControlledBase is the base of the robotics controller's ControlledAxis / ControlledAction rows (WP-52 ports those and ModuleRoboticController);
// it is ported here with the controller as an opaque pointer. RoboticCollisions forwards Unity collision messages to the Part (the Part collision
// handlers are the physics engine's: seams). SerenityUtils loads assets from the "serenity_assets" bundle (BundleLoader: the asset import, WP-48).
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"

namespace ksp {
class Part;
class PartModule;
}  // namespace ksp

namespace ksp::Expansions::Serenity {

class ModuleRoboticController;

struct SerenityMiscEnvironment {
    // PartModule.GetPersistenActiontId() (PartModule.cs:1227; the WP-06 PartModule does not carry persistentActionsId). Unset: 0
    static inline std::function<uint32_t(PartModule&)> persistentActionId;
    // Part.OnCollisionEnter / Exit / OnTriggerEnter / Exit (Part.cs): the engine's contact handling. Calls are recorded as the message name.
    static inline std::function<void(Part&, const std::string& message)> partCollision;
    static inline std::vector<std::string> collisionMessages;
    // BundleLoader.LoadAsset / LoadTextureAsset("serenity_assets", path): the loaded asset (opaque) or nullptr. Unset: nullptr
    static inline std::function<std::shared_ptr<void>(const std::string& bundle, const std::string& assetPath)> loadAsset;
    static void Reset();
};

// ControlledBase.cs:8
class ControlledBase {
public:
    virtual ~ControlledBase() = default;
    uint32_t partId = 0;                       // :11 internal
    std::vector<uint32_t> SymmetryPartIDs;     // :14
    int rowIndex = -1;                         // :20
    uint32_t moduleId = 0;                     // :23
    uint32_t moduleActionsId = 0;              // :26
    uint32_t persistentActionId = 0;           // :28 internal

    Part* PartPtr() const { return part_; }                       // :30 Part
    ModuleRoboticController* Controller = nullptr;                // :32
    uint32_t PartPersistentId() const;                            // :34
    std::vector<Part*>* SymmetryParts() const;                    // :46
    const std::string& PartNickName() const { return partNickName; }  // :57
    PartModule* Module() const { return module_; }                // :60
    uint32_t PersistentActionId() const { return persistentActionId; }  // :62
    virtual std::string BaseName() const = 0;                     // :64

    ControlledBase(Part* part, PartModule* module, ModuleRoboticController* controller);  // :66
    ControlledBase() = default;                                   // :79
    void SetPartNickName(const std::string& newNickName) { partNickName = newNickName; }  // :88
    bool AssignReferenceVars();                                   // :105 internal
    void RebuildSymmetryList(const std::vector<uint32_t>& excludedPartIds = {});  // :123 internal
    void ChangeSymmetryMaster(Part* newPart);                     // :140
    void Load(const ConfigNode& node);                            // :158
    void Save(ConfigNode& node);                                  // :168

    std::string partNickName;                                     // :17 (private in C#; the derived rows read it through PartNickName())

protected:
    virtual bool OnAssignReferenceVars() = 0;                     // :93
    virtual void ClearSymmetryLists() = 0;                        // :95
    virtual void AddSymmetryPart(Part* part) = 0;                 // :97
    virtual bool OnChangeSymmetryMaster(Part* part, uint32_t& oldPartId) = 0;  // :99
    virtual void OnLoad(const ConfigNode& node) = 0;              // :156
    virtual void OnSave(ConfigNode& node) = 0;                    // :158
    Part* part_ = nullptr;
    PartModule* module_ = nullptr;
};

// RoboticCollisions.cs:6
class RoboticCollisions {
public:
    Part* part = nullptr;
    bool behaviourEnabled = true;
    bool destroyed = false;
    void Start();                          // :10
    void OnCollisionEnter();               // :18
    void OnCollisionExit();                // :26
    void OnTriggerEnter();                 // :34
    void OnTriggerExit();                  // :42
};

// SerenityUtils.cs:5
namespace SerenityUtils {
std::string AssetPath(const std::string& prefabPath);                    // "Assets/Expansions/Serenity/" + prefabPath
std::shared_ptr<void> SerenityTexture(const std::string& prefabPath);    // :7 LoadTextureAsset("serenity_assets", ...)
std::shared_ptr<void> SerenityPrefab(const std::string& prefabPath);     // :12 LoadAsset("serenity_assets", ...)
}  // namespace SerenityUtils

}  // namespace ksp::Expansions::Serenity
