// WP-UE-16a (private): ConfigNode.LoadObjectFromConfig / CreateConfigFromObject for the [Persistent] fields of the effect classes.
// ConfigNode.cs:2666 ReadObject (values in node order by field name; ReadValue :2803: TryParse forms leave the field on failure, enums go
// through Enum.Parse), ConfigNode.cs:2352 WriteObject / :2413 WriteValue.
#pragma once

#include <string>
#include <string_view>

#include "ksp/config/config_node.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/parts/cs_parse.hpp"

namespace ksp::persist {

inline void Read(const ConfigNode& n, std::string_view name, std::string& f) {
    for (const auto& v : n.values())
        if (v.name == name) f = v.value;
}
inline void Read(const ConfigNode& n, std::string_view name, bool& f) {
    for (const auto& v : n.values())
        if (v.name == name) {
            bool r = false;
            if (cs::BoolTryParse(v.value, r)) f = r;
        }
}
inline void Read(const ConfigNode& n, std::string_view name, int32_t& f) {
    for (const auto& v : n.values())
        if (v.name == name) {
            int32_t r = 0;
            if (cs::IntTryParse(v.value, r)) f = r;
        }
}
inline void Read(const ConfigNode& n, std::string_view name, float& f) {
    for (const auto& v : n.values())
        if (v.name == name) {
            float r = 0.0f;
            if (cs::FloatTryParse(v.value, r)) f = r;
        }
}
inline void Read(const ConfigNode& n, std::string_view name, Vector3f& f) {
    for (const auto& v : n.values())
        if (v.name == name) f = ConfigNode::ParseVector3(v.value);
}
inline void Read(const ConfigNode& n, std::string_view name, Vector4f& f) {
    for (const auto& v : n.values())
        if (v.name == name) f = ConfigNode::ParseVector4(v.value);
}
// Enum.Parse(type, value): trimmed; a defined name (case-sensitive) or an integer
template <class E, size_t N>
inline void ReadEnum(const ConfigNode& n, std::string_view name, E& f, const char* const (&names)[N]) {
    for (const auto& v : n.values()) {
        if (v.name != name) continue;
        const std::string t = dotnet::Trim(v.value);
        bool done = false;
        for (size_t i = 0; i < N; i++)
            if (t == names[i]) {
                f = static_cast<E>(static_cast<int32_t>(i));
                done = true;
                break;
            }
        if (done) continue;
        int32_t num = 0;
        if (!t.empty() && cs::IntTryParse(t, num)) {
            f = static_cast<E>(num);
            continue;
        }
        throw ArgumentException("Requested value '" + t + "' was not found.");
    }
}

inline void Write(ConfigNode& n, const char* name, const std::string& f) { n.AddValue(name, f); }
inline void Write(ConfigNode& n, const char* name, bool f) { n.AddValue(name, f ? "True" : "False"); }
inline void Write(ConfigNode& n, const char* name, int32_t f) { n.AddValue(name, std::to_string(f)); }
inline void Write(ConfigNode& n, const char* name, float f) { n.AddValue(name, dotnet::FormatSingleG(f, 9)); }
inline void Write(ConfigNode& n, const char* name, const Vector3f& f) { n.AddValue(name, ConfigNode::WriteVector(f)); }
inline void Write(ConfigNode& n, const char* name, const Vector4f& f) { n.AddValue(name, ConfigNode::WriteVector(f)); }
template <class E, size_t N>
inline void WriteEnum(ConfigNode& n, const char* name, E f, const char* const (&names)[N]) {
    const int32_t i = static_cast<int32_t>(f);
    n.AddValue(name, (i >= 0 && static_cast<size_t>(i) < N) ? std::string(names[i]) : std::to_string(i));  // Enum.ToString
}

inline const char* const kAudioChannelNames[] = {"Ship", "Voice", "Ambient", "Music", "UI"};
inline const char* const kAnimationTypeNames[] = {"HoldValue", "Play"};

// ConfigNode.CopyTo(node) with node == this (AnimationFX.OnSave): the loop runs over the Count taken before the adds, so every value appears twice
inline void SelfCopy(ConfigNode& node) {
    const int count = node.values().Count();
    for (int i = 0; i < count; i++) {
        const auto v = node.values()[i];  // copy: AddValue may reallocate
        node.AddValue(v.name, v.value, v.comment);
    }
    const int ncount = node.nodes().Count();
    for (int j = 0; j < ncount; j++) {
        const auto child = node.nodes().Shared(j);
        ConfigNode* t = node.AddNode(child->name);
        if (!t) throw NullReferenceException("ConfigNode.CopyToRecursive: AddNode returned null (blank node name)");
        child->CopyTo(*t);
    }
}

}  // namespace ksp::persist
