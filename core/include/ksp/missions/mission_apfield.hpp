// WP-54c: Assembly-CSharp/Expansions/Missions/Editor/BaseAPFieldList.cs and BaseFieldList.cs (the generic `BaseFieldList<R, K>` it instantiates). The real BaseAPField
// itself lives in mission_display.hpp (the WP-55 stand-in members stay: the ported node library never has reflection data).
//
// C# builds the list by reflecting over the host's fields and properties (BaseFieldList.ReflectedData); C++ has no reflection, so a host type registers its members
// (RegisterReflectedType: the fields in reflection order, then the properties, each with its MEGUI_Control attribute) and the list is built from that record.
#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/missions/mission_display.hpp"

namespace ksp::Expansions::Missions::Editor {

class BaseAPFieldList {
public:
    struct ReflectedMember {
        MEGUI_Control attribute;
        MemberInfo member;
    };
    // BaseFieldList.ReflectedData(type, ignoreUIControl): the members carrying the attribute, fields first (in reflection order), then properties. An unregistered type has
    // no members (the cache is per type, whatever ignoreUIControl was passed first).
    static void RegisterReflectedType(const std::string& typeName, std::vector<ReflectedMember> fields, std::vector<ReflectedMember> properties);
    static void ResetReflectedTypes();

    BaseAPFieldList();                                                         // BaseFieldList.cs:@BaseFieldList
    // BaseAPFieldList(object host) : base(host, ignoreUIControl: true); host == nullptr logs and leaves the list null (Count throws)
    BaseAPFieldList(const std::string& hostTypeName, IAPFieldHost* hostType, void* host);  // BaseAPFieldList.cs:@BaseAPFieldList#2

    int Count() const;                                                         // BaseFieldList.cs:@Count
    BaseAPField* operator[](const std::string& fieldName);                     // :@this[]  (null when absent)
    BaseAPField* operator[](int index);                                        // :@this[]#2 (ArgumentOutOfRange when out of range)
    void SetAt(int index, std::unique_ptr<BaseAPField> value);                 // :@this[]#2 setter
    // GetValue / SetValue by name go through BaseField<K>.GetValue(host) / SetValue(newValue, host): NOT the BaseAPField overloads (no onValueChange, no validation)
    Boxed GetValue(const std::string& fieldName);                              // :@GetValue
    bool SetValue(const std::string& fieldName, const Boxed& value);           // :@SetValue
    void SetOriginalValue();                                                   // :@SetOriginalValue
    void Add(std::unique_ptr<BaseAPField> element);                            // :@Add
    const std::string& ReflectedType() const { return typeName_; }             // :@ReflectedType

private:
    void CreateList(const std::string& typeName, IAPFieldHost* hostType, void* instance);  // :@CreateList
    std::optional<std::vector<std::unique_ptr<BaseAPField>>> fields_;
    void* host_ = nullptr;
    std::string typeName_;
};

}  // namespace ksp::Expansions::Missions::Editor
