// WP-05 module data: the PartModule class table (reflection metadata generated from the decompiled source into
// module_schema.inc by tools/wp05/gen_module_schema.py) and the MODULE records of a part prefab.
//
// kspcore does not instantiate PartModules here (that is WP-06). A PartModuleData is what PartLoader hands to the
// module system: the class AssemblyLoader resolved for the MODULE name, a copy of the MODULE node as the game loaded it
// (after localisation), PartModule.Load's base data (isEnabled), and the values BaseFieldList.Load would assign, already
// typed with BaseField.ReadPvt's rules (BaseField.cs:712): bool/byte/ushort/char/ulong/int/long/float/uint/short/double
// TryParse (failure: logged, field unchanged), Vector2/3/4 and Quaternion split on "(", ",", " ", ")" with
// float.Parse, enums Enum.Parse (case-sensitive), strings verbatim; Color and other types are not read (sic).
// Node-valued fields (IConfigNode types: FloatCurve, DoubleCurve) load from the child node of the same name.
// ModuleEngines / ModuleRCS PROPELLANT nodes are parsed into Propellant records (Propellant.cs).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/curves/double_curve.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/parts/ksp_enums.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// One [KSPField] / [KSPAxisField] member (BaseFieldList.ReflectedData entry).
struct ModuleFieldInfo {
    const char* name;
    const char* csType;       // as declared ("float", "FloatCurve", "ModuleDeployablePart.DeployState")
    const char* typeName;     // FieldType.Name ("Single", "FloatCurve", "DeployState")
    bool isPersistant;
    bool isProperty;
    bool isPrivate;
    const char* initializer;  // C# initializer expression text ("" when none)
    int line;
    const EnumTable* enumTable;  // enum-typed fields
};

struct ModuleClassInfo {
    const char* name;      // Type.Name
    const char* fullName;  // namespace-qualified
    const char* baseName;
    bool isAbstract;
    const char* file;      // decompiled source file (relative to Assembly-CSharp/)
    int line;
    std::vector<ModuleFieldInfo> ownFields;  // declared in this class, declaration order
    const ModuleClassInfo* base = nullptr;   // nullptr above PartModule

    bool Is(std::string_view className) const;  // this class or a base class has this Type.Name
    // BaseFieldList order: this class's fields, then each base class's non-private fields (reflection with
    // FlattenHierarchy does not return private members of base classes).
    std::vector<const ModuleFieldInfo*> Fields() const;
    const ModuleFieldInfo* FindField(std::string_view name) const;  // first match in Fields() order
};

// Every PartModule class of Assembly-CSharp (PartModule itself first), file order.
const std::vector<ModuleClassInfo>& ModuleClasses();
// AssemblyLoader.GetClassByName(typeof(PartModule), name): Type.Name match among PartModule and its subclasses
// (the typesDictionary keeps the last type of a name; kspcore takes the last in file order).
const ModuleClassInfo* GetPartModuleClass(std::string_view name);

// A typed field value as BaseField.ReadPvt would leave it.
struct EnumValue {
    int64_t value = 0;
    const EnumTable* table = nullptr;
};
using ModuleValue = std::variant<std::monostate, bool, uint8_t, uint16_t, char16_t, uint64_t, int32_t, int64_t, float,
                                 uint32_t, int16_t, double, std::string, Vector2f, Vector3f, Vector4f, Quaternionf,
                                 EnumValue, FloatCurve, DoubleCurve>;

struct ModuleFieldValue {
    const ModuleFieldInfo* field = nullptr;
    bool fromNode = false;             // loaded from a child node (IConfigNode field)
    std::string raw;                   // the cfg value text (value-sourced fields)
    ConfigNode::NodePtr node;          // the child node (node-sourced fields)
    ModuleValue value;                 // typed value (monostate when the read failed or the type is not read)
    bool parsed = false;               // false: ReadPvt rejected it (the game logs an error and keeps the default)
};

// Propellant.cs (data)
struct Propellant {
    std::string name;
    std::string displayName;
    int32_t id = 0;
    float ratio = 0.0f;
    double minResToLeave = 0.0;
    bool ignoreForIsp = false;
    bool ignoreForThrustCurve = false;
    bool drawStackGauge = false;
    int32_t flowMode = 8;  // ResourceFlowMode.NULL
    int32_t fMode = 8;
    void Load(const ConfigNode& node);  // Propellant.cs:62
    int32_t GetFlowMode();              // Propellant.cs:114: NULL -> the resource's default flow mode (cached)
};

// One MODULE of a part prefab.
struct PartModuleData {
    std::string moduleName;                     // the MODULE node's `name`
    const ModuleClassInfo* moduleClass = nullptr;
    ConfigNode::NodePtr config;                 // copy of the MODULE node as loaded (nullptr: added in code)
    bool isEnabled = true;                      // PartModule.Load: cfg isEnabled, else true
    bool addedInCode = false;                   // e.g. ModuleTripLogger (Part.AddModule(string), no Load)
    std::vector<ModuleFieldValue> cfgFields;    // values BaseFieldList.Load assigns, node order (values, then nodes)
    std::vector<Propellant> propellants;        // ModuleEngines / ModuleRCS (and subclasses)
    const ModuleFieldValue* FindCfgField(std::string_view name) const;  // last assignment wins (as in C#)
};

// BaseField.ReadPvt (BaseField.cs:712) for a field of `info`'s type. Returns false when C# would log an error.
bool ReadModuleFieldValue(const ModuleFieldInfo& info, std::string_view value, ModuleValue& out);
// True for the IConfigNode field types BaseFieldList.Load fills from child nodes.
bool IsConfigNodeFieldType(const ModuleFieldInfo& info);

// Load a MODULE node into a PartModuleData (PartModule.Load data steps: isEnabled, Fields.Load, propellants). Like the
// game, ModuleRCS without PROPELLANT nodes adds one to `node` itself; `module.config` is a copy taken afterwards.
void LoadModuleData(PartModuleData& module, ConfigNode& node);

}  // namespace ksp
