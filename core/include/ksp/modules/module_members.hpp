// The attribute data BaseFieldList / BaseEventList / BaseActionList read by reflection ([KSPField(...)], [UI_*(...)],
// [KSPEvent(...)], [KSPAction(...)] and the UI_Control class tree), generated from the decompiled source into
// module_members.inc by tools/wp06/gen_module_members.py. The attribute argument text is kept verbatim and evaluated by
// ksp::cs (cs_expr.hpp); the registry only locates it.
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ksp/parts/part_module_data.hpp"

namespace ksp {

struct UiMemberInfo {
    std::string uiType;  // "UI_FloatRange"
    std::string args;    // "minValue = 0f, maxValue = 100f, stepIncrement = 1f"
};

struct FieldMemberInfo {
    std::string field;
    bool isAxis = false;  // [KSPAxisField]
    std::string args;     // KSPField attribute arguments
    std::vector<UiMemberInfo> ui;
};

struct EventMemberInfo {
    std::string method;
    bool hasDetails = false;  // takes BaseEventDetails (BaseEventDetailsDelegate)
    bool isVirtual = false;
    std::string args;
    int line = 0;
};

struct ActionMemberInfo {
    std::string method;
    bool isVirtual = false;
    std::string args;  // positional guiName, KSPActionGroup, ... + named
    int line = 0;
};

struct ClassMembers {
    std::string name, fullName;
    std::vector<FieldMemberInfo> fields;
    std::vector<EventMemberInfo> events;
    std::vector<ActionMemberInfo> actions;
};

struct UiClassField {
    std::string name, csType, initializer;
};
struct UiClassInfo {
    std::string name, base, file;
    int line = 0;
    std::vector<UiClassField> fields;  // public instance fields declared in this class, declaration order
};

// Members declared in `cls` (matched by Type.Name + namespace-qualified name); nullptr when it declares none.
const ClassMembers* FindClassMembers(const ModuleClassInfo& cls);
// The [KSPField] / [KSPAxisField] attribute record for a field of the schema (info.owner is the declaring class).
const FieldMemberInfo* FindFieldMember(const ModuleFieldInfo& info);
const UiClassInfo* FindUiClass(std::string_view name);
// UI_Scene (UI_Scene.cs)
const EnumTable& UiSceneEnum();

// BaseEventList.ReflectedData: instance methods with [KSPEvent] in reflection order (the class's own, then bases');
// a method name declared by a derived class hides the base declaration.
std::vector<const EventMemberInfo*> ReflectedEvents(const ModuleClassInfo& cls);
// BaseActionList.ReflectedData
std::vector<const ActionMemberInfo*> ReflectedActions(const ModuleClassInfo& cls);

}  // namespace ksp
