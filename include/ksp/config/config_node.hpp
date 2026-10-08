// ConfigNode: KSP's cfg/craft/sfs tree (Assembly-CSharp/ConfigNode.cs, 4,344 lines). This is the data, text and
// typed-value part of the class; the reflection-based object<->node mapping (CreateConfigFromObject, LoadObjectFromConfig,
// [Persistent] fields, WriteLinks/ReadLinks) belongs to WP-06 and is not here.
//
// Behaviour reproduced exactly (verified byte-for-byte against harness/golden/confignode_roundtrip):
//  * Load: File.ReadAllLines (BOM detection, CR/LF/CRLF) -> PreFormatConfig (a `//` anywhere starts a comment, lines
//    trimmed with .NET Unicode whitespace rules, '{' / '}' split out of lines, empty lines dropped) -> CustomEqualSplit
//    (first '=' separates name and value) -> RecurseFormat (a bare token not followed by '{' is dropped; a '}' at root
//    level ends parsing: everything after it is lost). Load returns a node named "root"; an empty result returns null.
//  * Node names "NAME (id)" / "NAME(id)" are split into name and id (CustomNameSplit). ToString() prints ids, Save() does
//    not. Comments read from files are discarded (only comments set through the API are written).
//  * Save: root values and child nodes without the root wrapper, tab indentation, CRLF line ends, UTF-8 without BOM.
//  * ToString: includes the root wrapper and ids, '\n' line ends.
//  * Value names/values added through ValueList.Add are sanitized ('{'->'[', '}'->']', and '='->'-' in names).
//
// Strings are UTF-8 (C# strings are UTF-16; the operations used are encoding-independent for ASCII syntax characters).
// A C# null comment is std::nullopt. Child nodes are reference objects (std::shared_ptr) as in C#: AddNode(node) adds
// the same object, GetNode returns a pointer into the tree.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// Thrown where KSP's C# code would throw NullReferenceException (e.g. CopyTo on a tree containing an anonymous node).
class NullReferenceException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// System.Guid (16 bytes, .NET byte order: Data1 LE uint32, Data2 LE uint16, Data3 LE uint16, Data4[8]).
struct Guid {
    uint8_t bytes[16] = {};
    static Guid Empty() { return {}; }
    // new Guid(string): formats "N", "D", "B", "P" (and "X"), surrounding whitespace allowed. Throws FormatException.
    static Guid Parse(std::string_view s);
    static bool TryParse(std::string_view s, Guid& out);
    std::string ToString(char format = 'D') const;
    bool operator==(const Guid&) const = default;
};

class ConfigNode {
public:
    // ConfigNode.Value (ConfigNode.cs:245)
    struct Value {
        std::string name;
        std::string value;
        std::optional<std::string> comment;

        Value() = default;
        Value(std::string n, std::string v) : name(std::move(n)), value(std::move(v)) {}
        Value(std::string n, std::string v, std::optional<std::string> c)
            : name(std::move(n)), value(std::move(v)), comment(std::move(c)) {}
        void Sanitize(bool sanitizeName);
    };

    // ConfigNode.ValueList (ConfigNode.cs:311)
    class ValueList {
    public:
        int Count() const { return static_cast<int>(values_.size()); }
        const Value& operator[](int i) const { return values_[static_cast<size_t>(i)]; }
        Value& operator[](int i) { return values_[static_cast<size_t>(i)]; }
        auto begin() const { return values_.begin(); }
        auto end() const { return values_.end(); }
        auto begin() { return values_.begin(); }
        auto end() { return values_.end(); }

        void Add(Value v);  // sanitizes name and value
        bool SetValue(std::string_view name, std::string_view newValue, const std::optional<std::string>& newComment,
                      int index, bool createIfNotFound);
        const std::string* GetValuePtr(std::string_view name, int index = 0) const;
        std::vector<std::string> GetValues() const;
        std::vector<std::string> GetValues(std::string_view name) const;
        std::vector<std::string> GetValuesStartsWith(std::string_view name) const;
        bool Contains(std::string_view name) const;
        void Clear() { values_.clear(); }
        void Remove(const Value* value);       // by identity, last match first
        void RemoveValue(std::string_view name);  // removes the LAST value with this name
        void RemoveValues(std::string_view name);
        void RemoveValuesStartWith(std::string_view startsWith);
        void SortByName();
        std::vector<std::string> DistinctNames() const;
        int CountByName(std::string_view name) const;

    private:
        friend class ConfigNode;
        std::vector<Value> values_;
    };

    using NodePtr = std::shared_ptr<ConfigNode>;

    // ConfigNode.ConfigNodeList (ConfigNode.cs:613)
    class ConfigNodeList {
    public:
        int Count() const { return static_cast<int>(nodes_.size()); }
        ConfigNode* operator[](int i) const { return nodes_[static_cast<size_t>(i)].get(); }
        const NodePtr& Shared(int i) const { return nodes_[static_cast<size_t>(i)]; }
        auto begin() const { return nodes_.begin(); }
        auto end() const { return nodes_.end(); }

        void Add(NodePtr n) { nodes_.push_back(std::move(n)); }
        ConfigNode* GetNodeID(std::string_view id) const;
        ConfigNode* GetNode(std::string_view name) const;
        ConfigNode* GetNode(std::string_view name, std::string_view valueName, const std::optional<std::string>& value) const;
        ConfigNode* GetNode(std::string_view name, int index) const;
        std::vector<ConfigNode*> GetNodes(std::string_view name) const;
        std::vector<ConfigNode*> GetNodes(std::string_view name, std::string_view valueName,
                                          const std::optional<std::string>& value) const;
        std::vector<ConfigNode*> GetNodes() const;
        bool SetNode(std::string_view name, const NodePtr& newNode, const std::optional<std::string>& newComment, int index,
                     bool createIfNotFound);
        void Remove(const ConfigNode* node);
        void RemoveNode(std::string_view name);  // removes the LAST node with this name
        void RemoveNodesStartWith(std::string_view startsWith);
        void RemoveNodes(std::string_view name);
        bool Contains(std::string_view name) const;
        std::vector<std::string> DistinctNames() const;
        int CountByName(std::string_view name) const;
        void Clear() { nodes_.clear(); }

    private:
        std::vector<NodePtr> nodes_;
    };

    // ---- construction (ConfigNode.cs:985-1017) ----
    ConfigNode();
    explicit ConfigNode(std::string_view name);
    ConfigNode(std::string_view name, std::optional<std::string> comment);
    ConfigNode(const ConfigNode&) = delete;
    ConfigNode& operator=(const ConfigNode&) = delete;
    ConfigNode(ConfigNode&&) = default;
    ConfigNode& operator=(ConfigNode&&) = default;

    static NodePtr Create(std::string_view nodeName = {}) { return std::make_shared<ConfigNode>(nodeName); }

    std::string name;
    std::string id;
    std::optional<std::string> comment;

    ValueList& values() { return values_; }
    const ValueList& values() const { return values_; }
    ConfigNodeList& nodes() { return nodes_; }
    const ConfigNodeList& nodes() const { return nodes_; }
    bool HasData() const { return values_.Count() > 0 || nodes_.Count() > 0; }
    int CountValues() const { return values_.Count(); }
    int CountNodes() const { return nodes_.Count(); }

    // ---- text I/O ----
    // ConfigNode.Load(path): nullptr if the file does not exist or contains no data. Localization is bypassed (as in
    // Load(string)); Load(path, bypassLocalization=false) would additionally run Localizer.TranslateBranch (WP-??).
    static NodePtr Load(const std::string& fileFullName);
    // LoadFromStringArray(lines)
    static NodePtr LoadFromLines(const std::vector<std::string>& cfgData);
    // ConfigNode.Parse(string): splits on '\n' and '\r'; never null.
    static NodePtr Parse(std::string_view s);
    // ConfigNode.Save(path[, header]): writes SaveToString(header). Returns false if the file cannot be written.
    bool Save(const std::string& fileFullName, const std::optional<std::string>& header = std::nullopt) const;
    // Exact bytes Save writes: CRLF line ends, UTF-8 without BOM.
    std::string SaveToString(const std::optional<std::string>& header = std::nullopt) const;
    // ConfigNode.ToString(): ToStringRecursive("") ('\n' line ends, ids shown).
    std::string ToString() const;

    // ---- copy / merge ----
    void CopyTo(ConfigNode& node, bool overwrite = false) const;
    void CopyTo(ConfigNode& node, std::string_view newName) const;
    NodePtr CreateCopy() const;
    void ClearData();
    void AddData(const ConfigNode& node);
    static void Merge(ConfigNode& mergeTo, ConfigNode& mergeFrom);

    // ---- values ----
    bool HasValue(std::string_view key) const { return values_.Contains(key); }
    bool HasValue() const { return values_.Count() > 0; }
    bool HasValues(std::initializer_list<std::string_view> names) const;

    // AddValue(string, string[, comment]): CleanupInput (\n, \r removed; \t -> ' ') then sanitized add.
    void AddValue(std::string_view key, std::string_view value);
    void AddValue(std::string_view key, std::string_view value, std::optional<std::string> comment);
    void AddValue(std::string_view key, const char* value) { AddValue(key, std::string_view(value)); }
    void AddValue(std::string_view key, const std::string& value) { AddValue(key, std::string_view(value)); }
    // Typed overloads (ToString(CultureInfo.InvariantCulture); float "G9", double "G17"; vectors via Write*).
    void AddValue(std::string_view key, bool value);
    void AddValue(std::string_view key, int32_t value);
    void AddValue(std::string_view key, uint32_t value);
    void AddValue(std::string_view key, int64_t value);
    void AddValue(std::string_view key, uint64_t value);
    void AddValue(std::string_view key, float value);
    void AddValue(std::string_view key, double value);
    void AddValue(std::string_view key, const Vector2f& value);
    void AddValue(std::string_view key, const Vector3f& value);
    void AddValue(std::string_view key, const Vector3d& value);
    void AddValue(std::string_view key, const Vector4f& value);
    void AddValue(std::string_view key, const Quaternionf& value);
    void AddValue(std::string_view key, const QuaternionD& value);
    void AddValue(std::string_view key, const Matrix4x4f& value);
    void AddValue(std::string_view key, const Color& value);
    void AddValue(std::string_view key, const Color32& value);
    // AddValue(string, char): char.ToString()
    void AddValueChar(std::string_view key, char32_t value);
    // Typed AddValue with a comment.
    template <class T>
    void AddValueWithComment(std::string_view key, const T& value, std::optional<std::string> comment) {
        AddValue(key, value);
        values_.values_.back().comment = std::move(comment);
    }

    std::optional<std::string> GetValue(std::string_view key) const;
    std::optional<std::string> GetValue(std::string_view key, int index) const;
    const std::string* GetValuePtr(std::string_view key, int index = 0) const { return values_.GetValuePtr(key, index); }
    std::vector<std::string> GetValues() const { return values_.GetValues(); }
    std::vector<std::string> GetValues(std::string_view key) const { return values_.GetValues(key); }
    std::vector<std::string> GetValuesList(std::string_view key) const { return values_.GetValues(key); }
    std::vector<std::string> GetValuesStartsWith(std::string_view key) const { return values_.GetValuesStartsWith(key); }

    // SetValue(key, string[, comment][, index], createIfNotFound). Note KSP quirks: returns false when it creates the
    // value, and a created value is added without sanitizing.
    bool SetValue(std::string_view key, std::string_view newValue, bool createIfNotFound = false);
    bool SetValue(std::string_view key, std::string_view newValue, int index, bool createIfNotFound = false);
    bool SetValue(std::string_view key, std::string_view newValue, const std::optional<std::string>& comment,
                  bool createIfNotFound = false);
    bool SetValue(std::string_view key, std::string_view newValue, const std::optional<std::string>& comment, int index,
                  bool createIfNotFound = false);
    bool SetValue(std::string_view key, const char* v, bool createIfNotFound = false) {
        return SetValue(key, std::string_view(v), createIfNotFound);
    }
    bool SetValue(std::string_view key, const std::string& v, bool createIfNotFound = false) {
        return SetValue(key, std::string_view(v), createIfNotFound);
    }
    bool SetValue(std::string_view key, bool v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, int32_t v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, uint32_t v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, int64_t v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, uint64_t v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, float v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, double v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, const Vector2f& v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, const Vector3f& v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, const Vector3d& v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, const Vector4f& v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, const Quaternionf& v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, const QuaternionD& v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, const Matrix4x4f& v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, const Color& v, bool createIfNotFound = false);
    bool SetValue(std::string_view key, const Color32& v, bool createIfNotFound = false);

    void RemoveValue(std::string_view key) { values_.RemoveValue(key); }
    void RemoveValues(std::initializer_list<std::string_view> names);
    // RemoveValues(string startsWith) -- sic: KSP removes values whose key EQUALS the argument.
    void RemoveValues(std::string_view startsWith) { values_.RemoveValues(startsWith); }
    void RemoveValuesStartWith(std::string_view startsWith) { values_.RemoveValuesStartWith(startsWith); }
    void ClearValues() { values_.Clear(); }

    // ---- nodes ----
    bool HasNodeID(std::string_view nodeId) const;
    bool HasNode(std::string_view key) const;
    bool HasNode() const { return nodes_.Count() > 0; }
    // AddNode(key[, comment]): nullptr if the trimmed key is empty.
    ConfigNode* AddNode(std::string_view key);
    ConfigNode* AddNode(std::string_view key, std::optional<std::string> comment);
    ConfigNode* AddNode(const NodePtr& node);
    ConfigNode* AddNode(std::string_view key, const NodePtr& node);
    ConfigNode* GetNodeID(std::string_view nodeId) const { return nodes_.GetNodeID(nodeId); }
    ConfigNode* GetNode(std::string_view key) const { return nodes_.GetNode(key); }
    ConfigNode* GetNode(std::string_view key, std::string_view valueName, const std::optional<std::string>& value) const {
        return nodes_.GetNode(key, valueName, value);
    }
    ConfigNode* GetNode(std::string_view key, int index) const { return nodes_.GetNode(key, index); }
    std::vector<ConfigNode*> GetNodes(std::string_view key) const { return nodes_.GetNodes(key); }
    std::vector<ConfigNode*> GetNodes(std::string_view key, std::string_view valueName,
                                      const std::optional<std::string>& value) const {
        return nodes_.GetNodes(key, valueName, value);
    }
    std::vector<ConfigNode*> GetNodes() const { return nodes_.GetNodes(); }
    bool SetNode(std::string_view key, const NodePtr& newNode, bool createIfNotFound = false) {
        return nodes_.SetNode(key, newNode, std::nullopt, 0, createIfNotFound);
    }
    bool SetNode(std::string_view key, const NodePtr& newNode, int index, bool createIfNotFound = false) {
        return nodes_.SetNode(key, newNode, std::nullopt, index, createIfNotFound);
    }
    void RemoveNode(std::string_view key) { nodes_.RemoveNode(key); }
    void RemoveNode(const ConfigNode* node) { nodes_.Remove(node); }
    void RemoveNodesStartWith(std::string_view startsWith) { nodes_.RemoveNodesStartWith(startsWith); }
    void RemoveNodes(std::string_view key) { nodes_.RemoveNodes(key); }
    void ClearNodes() { nodes_.Clear(); }
    bool TryGetNode(std::string_view key, ConfigNode*& node) const;

    // ---- typed getters (ConfigNode.cs:4033-4344). Leave `value` untouched and return false when missing/unparsable.
    bool TryGetValue(std::string_view key, std::string& value) const;
    bool TryGetValue(std::string_view key, std::vector<std::string>& value) const;  // ParseExtensions.ParseArray (',')
    bool TryGetValue(std::string_view key, float& value) const;
    bool TryGetValue(std::string_view key, double& value) const;
    bool TryGetValue(std::string_view key, int32_t& value) const;
    bool TryGetValue(std::string_view key, uint32_t& value) const;
    bool TryGetValue(std::string_view key, int64_t& value) const;
    bool TryGetValue(std::string_view key, uint64_t& value) const;
    bool TryGetValue(std::string_view key, bool& value) const;
    bool TryGetValue(std::string_view key, Vector3f& value) const;
    bool TryGetValue(std::string_view key, Vector3d& value) const;
    bool TryGetValue(std::string_view key, Vector2f& value) const;
    bool TryGetValue(std::string_view key, Vector2d& value) const;
    bool TryGetValue(std::string_view key, Vector4f& value) const;
    bool TryGetValue(std::string_view key, Vector4d& value) const;
    bool TryGetValue(std::string_view key, Quaternionf& value) const;
    bool TryGetValue(std::string_view key, QuaternionD& value) const;
    bool TryGetValue(std::string_view key, Rect& value) const;
    bool TryGetValue(std::string_view key, Color& value) const;  // r,g,b[,a] (clamped) or ColorUtility html string
    bool TryGetValue(std::string_view key, Color32& value) const;
    bool TryGetValue(std::string_view key, Guid& value) const;  // on parse failure: value = Guid.Empty, false

    // TryGetEnum<T>(key, ref value, defaultValue): value = default; true only if the string is exactly a member key
    // (Enum.IsDefined + Enum.Parse, case-sensitive). C++ has no enum reflection: pass the member table.
    template <class E>
    bool TryGetEnum(std::string_view key, E& value, E defaultValue,
                    std::span<const std::pair<std::string_view, E>> members) const {
        value = defaultValue;
        const std::string* s = GetValuePtr(key);
        if (!s) return false;
        for (const auto& [n, v] : members)
            if (n == *s) {
                value = v;
                return true;
            }
        return false;
    }

    // ---- static writers / parsers (ConfigNode.cs:3525-3813) ----
    static std::string WriteBoolArray(std::span<const bool> flags);
    static std::string WriteEnumIntArray(std::span<const int> flags);
    static std::string WriteStringArray(std::span<const std::string> strings);
    static std::string WriteVector(const Vector2f& v);
    static std::string WriteVector(const Vector3f& v);
    static std::string WriteVector(const Vector3d& v);
    static std::string WriteVector(const Vector4f& v);
    static std::string WriteQuaternion(const Quaternionf& q);
    static std::string WriteQuaternion(const QuaternionD& q);
    static std::string WriteMatrix4x4(const Matrix4x4f& m);
    static std::string WriteColor(const Color& c);
    static std::string WriteColor(const Color32& c);
    // Parse* log a warning and return the default (zero / identity / white) on a wrong component count; malformed
    // numbers throw dotnet::FormatException like float.Parse.
    static Vector2f ParseVector2(std::string_view s);
    static Vector3f ParseVector3(std::string_view s);
    static Vector3d ParseVector3D(std::string_view s);
    static Vector4f ParseVector4(std::string_view s);
    static Quaternionf ParseQuaternion(std::string_view s);
    static QuaternionD ParseQuaternionD(std::string_view s);
    static Matrix4x4f ParseMatrix4x4(std::string_view s);  // KSP bug kept: m11 written twice, m13 never set
    static Color ParseColor(std::string_view s);
    static bool CheckAndParseColor(std::string_view s, Color& color);
    static Color32 ParseColor32(std::string_view s);
    static std::string CleanupInput(std::string_view value);

private:
    std::string ToStringRecursive(const std::string& indent) const;
    void CopyToRecursive(ConfigNode& node, bool overwrite) const;
    void WriteNodeString(std::string& out, const std::string& indent) const;
    static NodePtr RecurseFormat(const std::vector<std::vector<std::string>>& cfg);
    static void RecurseFormat(const std::vector<std::vector<std::string>>& cfg, size_t& index, ConfigNode& node);
    static bool PreFormatConfig(const std::vector<std::string>& cfgData, std::vector<std::vector<std::string>>& out);
    static std::vector<std::string> CustomEqualSplit(std::string_view str);
    static std::vector<std::string> CustomNameSplit(std::string_view name);
    void InitName(std::string_view rawName);

    ValueList values_;
    ConfigNodeList nodes_;
};

}  // namespace ksp
