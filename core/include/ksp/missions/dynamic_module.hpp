// WP-54: DynamicModule.cs and DynamicModuleList.cs (the base of the score and award module families), plus the type registry that stands
// for `AssemblyLoader.GetClassByName(typeof(DynamicModule), name)` and the reflection scans (`IsSubclassOf`).
//
// C# -> C++:
//   * `GetType().Name` is the registered type name: every concrete class derives from DynamicModuleT<Self, Base> (CRTP) which supplies
//     `TypeName()`, the clone and the registration. The base constructor takes the name (a base class constructor cannot see the
//     most derived type).
//   * `ObjectIDGenerator.GetId(this)` (a per-process counter, 1-based, one id per newly seen object) is `NextObjectId()`; a Clone() creates
//     a new instance (consumes one id) and then copies every field, id included, as the reflection loop does.
//   * Activator.CreateInstance(type, node[, definition]) is the factory registered with the type.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/missions/mission_display.hpp"
#include "ksp/missions/mission_types.hpp"

namespace ksp::Expansions::Missions {

class DynamicModule;
class AwardDefinition;

// What AssemblyLoader.GetClassByName(typeof(DynamicModule), name) finds: a type with its base-type chain (for IsSubclassOf) and the two
// constructors Activator.CreateInstance is used with.
struct DynamicModuleType {
    std::string name;
    std::vector<std::string> bases;  // nearest base first, ending with "DynamicModule"
    std::function<std::unique_ptr<DynamicModule>(MENode*)> create;                              // new T(node)
    std::function<std::unique_ptr<DynamicModule>(MENode*, AwardDefinition*)> createWithDefinition;  // new T(node, definition), may be empty
    bool IsSubclassOf(const std::string& baseName) const;
};

class DynamicModuleRegistry {
public:
    static void Register(DynamicModuleType type);
    // AssemblyLoader.GetClassByName(typeof(DynamicModule), name): nullptr when unknown
    static const DynamicModuleType* GetClassByName(const std::string& name);
    // every registered type below `baseName`, in registration order (the assembly scan order of GetSupportedTypes)
    static std::vector<std::string> SubclassesOf(const std::string& baseName);
    static void ResetForTests();
};

// DynamicModule.cs:9
class DynamicModule : public IMENodeDisplay {
public:
    DynamicModule(std::string typeName);                    // DynamicModule.cs:21
    DynamicModule(std::string typeName, MENode* node);      // :28
    ~DynamicModule() override = default;

    MENode* node = nullptr;                                 // :11 (protected in C#)
    std::vector<std::string> parametersDisplayedInSAP;      // :13

    const std::string& name() const { return name_; }       // :17
    const std::string& id() const { return id_; }           // :19
    virtual bool canBeDisplayedInEditor() const { return true; }  // :21

    std::string TypeName() const { return name_; }          // GetType().Name

    // IMENodeDisplay
    virtual std::string GetDisplayName() override;                                // :44
    virtual std::string GetDisplayToolTip();                                      // :49
    virtual std::vector<std::string> GetDefaultPinnedParameters();                // :54
    bool Equals(const DynamicModule& other) const;                                // :59 (name equality; subclasses add fields)
    virtual bool EqualsObject(const DynamicModule& other) const { return Equals(other); }
    std::string GetName() override;                                               // :74
    void AddParameterToSAP(const std::string& parameter) override;                // :79
    void RemoveParameterFromSAP(const std::string& parameter) override;           // :84
    void AddParameterToNodeBody(const std::string& parameter) override;           // :94
    void AddParameterToNodeBodyAndUpdateUI(const std::string& parameter) override;  // :102
    void RemoveParameterFromNodeBody(const std::string& parameter) override;      // :111
    void RemoveParameterFromNodeBodyAndUpdateUI(const std::string& parameter) override;  // :119
    bool HasNodeBodyParameter(const std::string& parameter) override;             // :128
    bool HasSAPParameter(const std::string& parameter) override;                  // :137
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :142 (editor reflection: not ported, "")
    void UpdateNodeBodyUI() override;                                             // :152
    std::vector<IMENodeDisplay*> GetInternalParametersToDisplay() override;       // :160
    MENode* GetNode() override;                                                   // :165
    virtual std::string GetInfo() override;                                       // :170
    void SetNode(MENode* n);                                                      // :175
    virtual void Initialize();                                                    // :180
    virtual void Destroy();                                                       // :184
    virtual void ParameterSetupComplete() override;                               // :188
    virtual void Load(const ConfigNode& node);                                    // :192
    virtual void Save(ConfigNode& node) const;                                    // :201
    // :208 Clone: a new instance of the same type (consumes an object id) with every field copied
    virtual std::unique_ptr<DynamicModule> Clone() const = 0;

    // ObjectIDGenerator ids handed out so far (tests / reset)
    static uint64_t NextObjectId();
    static void ResetObjectIds();

protected:
    DynamicModule(const DynamicModule&) = default;
    DynamicModule& operator=(const DynamicModule&) = default;

    std::string name_;
    std::string id_;

private:
    static std::string GetModuleInstanceID();  // :37
};

// CRTP: the type name, registration and Clone() of a concrete module. `Self` must provide Self(MENode*) (and Self(MENode*, AwardDefinition*) for
// the award family; pass hasDefinition = true).
template <class Self, class Base>
class DynamicModuleT : public Base {
public:
    using Base::Base;
    std::unique_ptr<DynamicModule> Clone() const override {
        auto m = std::make_unique<Self>(this->node);  // Activator.CreateInstance(GetType(), node): consumes an id
        static_cast<Self&>(*m) = static_cast<const Self&>(*this);  // fields[i].SetValue(clone, fields[i].GetValue(this)) incl. id
        return m;
    }
};

// DynamicModuleList.cs:7
class DynamicModuleList {
public:
    explicit DynamicModuleList(MENode* node);                           // :23
    virtual ~DynamicModuleList() = default;
    DynamicModuleList(const DynamicModuleList&) = delete;
    DynamicModuleList& operator=(const DynamicModuleList&) = delete;

    std::vector<std::unique_ptr<DynamicModule>> activeModules;          // :9
    MENode* node() const { return node_; }                              // :19

    virtual std::vector<std::string> GetSupportedTypes();               // :30
    std::vector<IMENodeDisplay*> GetInternalParameters();               // :35
    bool ContainsModule(const std::string& typeName) const;             // :46 (GetType() == module)
    void SetSupportedTypes(std::vector<std::string> supportedModuleTypes);  // :84
    bool Equals(const DynamicModuleList& other) const;                  // :89
    void Load(const ConfigNode& node);                                  // :112
    void Save(ConfigNode& node) const;                                  // :126
    // :140 Clone: Activator.CreateInstance(GetType(), node) + a Clone() of every module + a copy of supportedTypes
    // (WP-54c: the base class is concrete in C# - ActionPartFailure keeps a plain DynamicModuleList - so the base Clone is real; subclasses override)
    virtual std::unique_ptr<DynamicModuleList> Clone() const;

protected:
    void AddDynamicModule(const ConfigNode& moduleNode);                // :60
    std::vector<std::string> supportedTypes;                            // :15
    std::string rootNodeName = "MODULES";                               // :17
    std::string moduleNodeName = "MODULE";                              // :19

private:
    MENode* node_ = nullptr;
};

}  // namespace ksp::Expansions::Missions
