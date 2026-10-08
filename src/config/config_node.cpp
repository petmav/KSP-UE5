// Port of Assembly-CSharp/ConfigNode.cs (data/text/typed-value parts). Line numbers refer to that file.
#include "ksp/config/config_node.hpp"

#include <algorithm>

#include "ksp/config/parse_extensions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"

namespace ksp {

using dotnet::FormatDoubleG;
using dotnet::FormatSingleG;

namespace {

bool OptEq(const std::string* a, const std::optional<std::string>& b) {
    if (!a) return !b.has_value();
    return b.has_value() && *a == *b;
}

bool IsNullOrEmpty(const std::optional<std::string>& s) { return !s.has_value() || s->empty(); }

// value.name + " = " + value.value + (IsNullOrEmpty(comment) ? "" : " // " + comment)
void AppendValueLine(std::string& out, const std::string& indent, const ConfigNode::Value& v, const char* eol) {
    out += indent;
    out += v.name;
    out += " = ";
    out += v.value;
    if (!IsNullOrEmpty(v.comment)) {
        out += " // ";
        out += *v.comment;
    }
    out += eol;
}

}  // namespace

// =====================================================================================================================
// Value (ConfigNode.cs:245)
// =====================================================================================================================

// ConfigNode.cs:266 Sanitize / 275 SanitizeString
void ConfigNode::Value::Sanitize(bool sanitizeName) {
    auto sanitizeString = [](std::string& s, bool isName) {
        bool flag = false;
        for (size_t i = s.size(); i-- > 0;) {
            const char c = s[i];
            if (c == '{' || c == '}' || (c == '=' && isName)) {
                flag = true;
                break;
            }
        }
        if (flag) {
            std::replace(s.begin(), s.end(), '{', '[');
            std::replace(s.begin(), s.end(), '}', ']');
            if (isName) std::replace(s.begin(), s.end(), '=', '-');
        }
    };
    if (sanitizeName) sanitizeString(name, true);
    sanitizeString(value, false);
}

// =====================================================================================================================
// ValueList (ConfigNode.cs:311)
// =====================================================================================================================

// ConfigNode.cs:327
void ConfigNode::ValueList::Add(Value v) {
    v.Sanitize(true);
    values_.push_back(std::move(v));
}

// ConfigNode.cs:357 SetValue(name, newValue, newComment, index, createIfNotFound)
bool ConfigNode::ValueList::SetValue(std::string_view name, std::string_view newValue,
                                     const std::optional<std::string>& newComment, int index, bool createIfNotFound) {
    int num = 0;
    for (Value& value : values_) {
        if (value.name == name) {
            if (num == index) {
                value.value = std::string(newValue);
                if (newComment.has_value()) value.comment = newComment;
                value.Sanitize(false);
                return true;
            }
            ++num;
        }
    }
    if (createIfNotFound) values_.emplace_back(std::string(name), std::string(newValue), newComment);  // no sanitize
    return false;
}

// ConfigNode.cs:394 / 416 GetValue(name[, index])
const std::string* ConfigNode::ValueList::GetValuePtr(std::string_view name, int index) const {
    int num = 0;
    for (const Value& value : values_) {
        if (value.name == name) {
            if (num == index) return &value.value;
            ++num;
        }
    }
    return nullptr;
}

// ConfigNode.cs:443
std::vector<std::string> ConfigNode::ValueList::GetValues() const {
    std::vector<std::string> list;
    list.reserve(values_.size());
    for (const Value& v : values_) list.push_back(v.value);
    return list;
}

// ConfigNode.cs:455
std::vector<std::string> ConfigNode::ValueList::GetValues(std::string_view name) const {
    std::vector<std::string> list;
    for (const Value& v : values_)
        if (v.name == name) list.push_back(v.value);
    return list;
}

// ConfigNode.cs:470
std::vector<std::string> ConfigNode::ValueList::GetValuesStartsWith(std::string_view name) const {
    std::vector<std::string> list;
    for (const Value& v : values_)
        if (dotnet::StartsWith(v.name, name)) list.push_back(v.value);
    return list;
}

// ConfigNode.cs:500
bool ConfigNode::ValueList::Contains(std::string_view name) const {
    for (const Value& v : values_)
        if (v.name == name) return true;
    return false;
}

// ConfigNode.cs:525 Remove(Value): last occurrence of this exact object
void ConfigNode::ValueList::Remove(const Value* value) {
    for (size_t i = values_.size(); i-- > 0;)
        if (&values_[i] == value) {
            values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
}

// ConfigNode.cs:539 RemoveValue(name): removes the last match only
void ConfigNode::ValueList::RemoveValue(std::string_view name) {
    for (size_t i = values_.size(); i-- > 0;)
        if (values_[i].name == name) {
            values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
}

// ConfigNode.cs:553
void ConfigNode::ValueList::RemoveValues(std::string_view name) {
    for (size_t i = values_.size(); i-- > 0;)
        if (values_[i].name == name) values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(i));
}

// ConfigNode.cs:565
void ConfigNode::ValueList::RemoveValuesStartWith(std::string_view startsWith) {
    for (size_t i = values_.size(); i-- > 0;)
        if (dotnet::StartsWith(values_[i].name, startsWith)) values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(i));
}

// ConfigNode.cs:577 SortByName: List.Sort with string.CompareTo (culture). List.Sort is an unstable introsort; a
// stable sort is used here, so equal names may keep a different relative order than in KSP.
void ConfigNode::ValueList::SortByName() {
    std::stable_sort(values_.begin(), values_.end(),
                     [](const Value& a, const Value& b) { return dotnet::CompareCulture(a.name, b.name) < 0; });
}

// ConfigNode.cs:582
std::vector<std::string> ConfigNode::ValueList::DistinctNames() const {
    std::vector<std::string> list;
    for (const Value& v : values_)
        if (std::find(list.begin(), list.end(), v.name) == list.end()) list.push_back(v.name);
    return list;
}

// ConfigNode.cs:597
int ConfigNode::ValueList::CountByName(std::string_view name) const {
    int n = 0;
    for (const Value& v : values_)
        if (v.name == name) ++n;
    return n;
}

// =====================================================================================================================
// ConfigNodeList (ConfigNode.cs:613)
// =====================================================================================================================

// ConfigNode.cs:637
ConfigNode* ConfigNode::ConfigNodeList::GetNodeID(std::string_view id) const {
    for (const NodePtr& n : nodes_)
        if (n->id == id) return n.get();
    return nullptr;
}

// ConfigNode.cs:659
ConfigNode* ConfigNode::ConfigNodeList::GetNode(std::string_view name) const {
    for (const NodePtr& n : nodes_)
        if (n->name == name) return n.get();
    return nullptr;
}

// ConfigNode.cs:681
ConfigNode* ConfigNode::ConfigNodeList::GetNode(std::string_view name, std::string_view valueName,
                                                const std::optional<std::string>& value) const {
    for (const NodePtr& n : nodes_)
        if (n->name == name && OptEq(n->GetValuePtr(valueName), value)) return n.get();
    return nullptr;
}

// ConfigNode.cs:703
ConfigNode* ConfigNode::ConfigNodeList::GetNode(std::string_view name, int index) const {
    int num = 0;
    for (const NodePtr& n : nodes_) {
        if (n->name == name) {
            if (num == index) return n.get();
            ++num;
        }
    }
    return nullptr;
}

// ConfigNode.cs:730
std::vector<ConfigNode*> ConfigNode::ConfigNodeList::GetNodes(std::string_view name) const {
    std::vector<ConfigNode*> list;
    for (const NodePtr& n : nodes_)
        if (n->name == name) list.push_back(n.get());
    return list;
}

// ConfigNode.cs:745
std::vector<ConfigNode*> ConfigNode::ConfigNodeList::GetNodes(std::string_view name, std::string_view valueName,
                                                              const std::optional<std::string>& value) const {
    std::vector<ConfigNode*> list;
    for (const NodePtr& n : nodes_)
        if (n->name == name && OptEq(n->GetValuePtr(valueName), value)) list.push_back(n.get());
    return list;
}

// ConfigNode.cs:760
std::vector<ConfigNode*> ConfigNode::ConfigNodeList::GetNodes() const {
    std::vector<ConfigNode*> list;
    list.reserve(nodes_.size());
    for (const NodePtr& n : nodes_) list.push_back(n.get());
    return list;
}

// ConfigNode.cs:787 SetNode(name, newNode, newComment, index, createIfNotFound)
bool ConfigNode::ConfigNodeList::SetNode(std::string_view name, const NodePtr& newNode,
                                         const std::optional<std::string>& newComment, int index, bool createIfNotFound) {
    int num = 0;
    for (const NodePtr& n : nodes_) {
        if (n->name == name) {
            if (num == index) {
                n->ClearData();
                newNode->CopyTo(*n);
                if (!IsNullOrEmpty(newComment)) n->comment = newComment;
                return true;
            }
            ++num;
        }
    }
    if (createIfNotFound) {
        newNode->name = std::string(name);
        nodes_.push_back(newNode);
        if (!IsNullOrEmpty(newComment)) newNode->comment = newComment;
        return true;
    }
    return false;
}

// ConfigNode.cs:830
void ConfigNode::ConfigNodeList::Remove(const ConfigNode* node) {
    for (size_t i = nodes_.size(); i-- > 0;)
        if (nodes_[i].get() == node) {
            nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
}

// ConfigNode.cs:844 (last match only)
void ConfigNode::ConfigNodeList::RemoveNode(std::string_view name) {
    for (size_t i = nodes_.size(); i-- > 0;)
        if (nodes_[i]->name == name) {
            nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
}

// ConfigNode.cs:858
void ConfigNode::ConfigNodeList::RemoveNodesStartWith(std::string_view startsWith) {
    for (size_t i = nodes_.size(); i-- > 0;)
        if (dotnet::StartsWith(nodes_[i]->name, startsWith)) nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(i));
}

// ConfigNode.cs:870
void ConfigNode::ConfigNodeList::RemoveNodes(std::string_view name) {
    for (size_t i = nodes_.size(); i-- > 0;)
        if (nodes_[i]->name == name) nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(i));
}

// ConfigNode.cs:882
bool ConfigNode::ConfigNodeList::Contains(std::string_view name) const {
    for (const NodePtr& n : nodes_)
        if (n->name == name) return true;
    return false;
}

// ConfigNode.cs:896
std::vector<std::string> ConfigNode::ConfigNodeList::DistinctNames() const {
    std::vector<std::string> list;
    for (const NodePtr& n : nodes_)
        if (std::find(list.begin(), list.end(), n->name) == list.end()) list.push_back(n->name);
    return list;
}

// ConfigNode.cs:911
int ConfigNode::ConfigNodeList::CountByName(std::string_view name) const {
    int c = 0;
    for (const NodePtr& n : nodes_)
        if (n->name == name) ++c;
    return c;
}

// =====================================================================================================================
// ConfigNode
// =====================================================================================================================

// ConfigNode.cs:1012
ConfigNode::ConfigNode() = default;

// ConfigNode.cs:985
ConfigNode::ConfigNode(std::string_view rawName) { InitName(rawName); }

// ConfigNode.cs:998
ConfigNode::ConfigNode(std::string_view rawName, std::optional<std::string> vcomment) {
    InitName(rawName);
    comment = std::move(vcomment);
}

void ConfigNode::InitName(std::string_view rawName) {
    name = CleanupInput(rawName);
    std::vector<std::string> array = CustomNameSplit(name);
    if (array.size() > 1) {
        std::string n = std::move(array[0]);
        id = std::move(array[1]);
        name = std::move(n);
    }
}

// ConfigNode.cs:1019 CustomNameSplit: "NAME (id)" -> {NAME, id}; separators are '(', ' ', ')'.
std::vector<std::string> ConfigNode::CustomNameSplit(std::string_view n) {
    auto isSep = [](char c) { return c == '(' || c == ' ' || c == ')'; };
    size_t i = 0;
    while (i < n.size() && isSep(n[i])) ++i;
    if (i == n.size()) return {};
    const size_t num = i;
    while (i < n.size() && !isSep(n[i])) ++i;
    if (i == n.size()) return {};
    const size_t length = i - num;
    while (i < n.size() && isSep(n[i])) ++i;
    if (i == n.size()) return {};
    const size_t num2 = i;
    while (i < n.size() && !isSep(n[i])) ++i;
    const size_t length2 = i - num2;
    return {std::string(n.substr(num, length)), std::string(n.substr(num2, length2))};
}

// ConfigNode.cs:3517 CleanupInput
std::string ConfigNode::CleanupInput(std::string_view value) {
    std::string r;
    r.reserve(value.size());
    for (char c : value) {
        if (c == '\n' || c == '\r') continue;
        r.push_back(c == '\t' ? ' ' : c);
    }
    return r;
}

// ConfigNode.cs:1057 ToString -> ToStringRecursive("")
std::string ConfigNode::ToString() const { return ToStringRecursive(""); }

// ConfigNode.cs:3245 ToStringRecursive
std::string ConfigNode::ToStringRecursive(const std::string& indent) const {
    std::string text;
    text += indent;
    text += name;
    if (!id.empty()) {
        text += " (";
        text += id;
        text += ")";
    }
    if (!IsNullOrEmpty(comment)) {
        text += " // ";
        text += *comment;
    }
    text += "\n";
    text += indent;
    text += "{\n";
    const std::string text2 = indent + "\t";
    for (const Value& v : values_) AppendValueLine(text, text2, v, "\n");
    for (const NodePtr& n : nodes_) text += n->ToStringRecursive(text2);
    text += indent;
    text += "}\n";
    return text;
}

// ConfigNode.cs:1062-1083 CopyTo / CreateCopy
void ConfigNode::CopyTo(ConfigNode& node, bool overwrite) const { CopyToRecursive(node, overwrite); }

void ConfigNode::CopyTo(ConfigNode& node, std::string_view newName) const {
    CopyToRecursive(node, false);
    node.name = std::string(newName);
}

ConfigNode::NodePtr ConfigNode::CreateCopy() const {
    auto n = std::make_shared<ConfigNode>();
    CopyToRecursive(*n, false);
    return n;
}

// ConfigNode.cs:3264 CopyToRecursive
void ConfigNode::CopyToRecursive(ConfigNode& node, bool overwrite) const {
    if (node.name.empty()) node.name = name;
    if (node.id.empty()) node.id = id;
    if (!IsNullOrEmpty(comment)) node.comment = comment;
    for (const Value& v : values_) {
        if (overwrite) node.SetValue(v.name, v.value, v.comment, true);
        else node.AddValue(v.name, v.value, v.comment);
    }
    for (const NodePtr& child : nodes_) {
        if (overwrite) node.RemoveNode(child->name);
        ConfigNode* target = node.AddNode(child->name);
        // C#: configNode.CopyToRecursive(node.AddNode(configNode.name)) -- AddNode returns null for blank names.
        if (!target) throw NullReferenceException("ConfigNode.CopyToRecursive: AddNode returned null (blank node name)");
        child->CopyToRecursive(*target, false);
    }
}

// ConfigNode.cs:1254
void ConfigNode::ClearData() {
    nodes_.Clear();
    values_.Clear();
}

// ConfigNode.cs:1260
void ConfigNode::AddData(const ConfigNode& node) {
    for (const Value& v : node.values_) AddValue(v.name, v.value, v.comment);
    for (const NodePtr& child : node.nodes_) {
        ConfigNode* node2 = AddNode(child->name);
        if (!node2) throw NullReferenceException("ConfigNode.AddData: AddNode returned null (blank node name)");
        child->CopyTo(*node2);
    }
}

// ConfigNode.cs:2260 Merge (KSP's behaviour kept, including MergeNode copying into the parent)
namespace {
void MergeValue(ConfigNode& node, const std::vector<std::string>& id, size_t index, const std::string& value) {
    if (index == id.size() - 1) {
        if (node.HasValue(id[index])) node.SetValue(id[index], value);
        else node.AddValue(id[index], value);
        return;
    }
    ConfigNode* nodeID = node.GetNodeID(id[index]);
    if (nodeID) {
        MergeValue(*nodeID, id, index + 1, value);
        return;
    }
    log::Info("ConfigNode: Cannot merge value from id");
}

void MergeNode(ConfigNode& node, const std::vector<std::string>& id, size_t index, const ConfigNode& value) {
    ConfigNode* nodeID = node.GetNodeID(id[index]);
    if (!nodeID) {
        log::Info("ConfigNode: Cannot merge node from id");
    } else if (index == id.size() - 1) {
        nodeID->ClearData();
        value.CopyTo(node);
    } else {
        MergeNode(node, id, index + 1, value);
    }
}
}  // namespace

void ConfigNode::Merge(ConfigNode& mergeTo, ConfigNode& mergeFrom) {
    mergeTo.name = mergeFrom.name;
    mergeTo.id = mergeFrom.id;
    for (int i = 0, count = mergeTo.values_.Count(); i < count; ++i) {
        if (dotnet::StartsWith(mergeTo.values_[i].name, "@")) {
            const auto array = dotnet::Split(mergeTo.values_[i].name, "@/", true);
            MergeValue(mergeFrom, array, 0, mergeTo.values_[i].value);
        }
    }
    mergeTo.RemoveValuesStartWith("@");
    for (int j = 0, count2 = mergeTo.nodes_.Count(); j < count2; ++j) {
        if (dotnet::StartsWith(mergeTo.nodes_[j]->name, "@")) {
            const auto array2 = dotnet::Split(mergeTo.nodes_[j]->name, "@/", true);
            MergeNode(mergeFrom, array2, 0, *mergeTo.nodes_[j]);
        }
    }
    mergeTo.RemoveNodesStartWith("@");
    for (int k = 0, count3 = mergeFrom.values_.Count(); k < count3; ++k)
        mergeTo.AddValue(mergeFrom.values_[k].name, mergeFrom.values_[k].value);
    for (int l = 0, count4 = mergeFrom.nodes_.Count(); l < count4; ++l) mergeTo.AddNode(mergeFrom.nodes_[l]->CreateCopy());
}

// ---------------------------------------------------------------------------------------------------------------------
// Load / Parse / Save
// ---------------------------------------------------------------------------------------------------------------------

// ConfigNode.cs:1085 Load(string) -> Load(path, bypassLocalization: true)
ConfigNode::NodePtr ConfigNode::Load(const std::string& fileFullName) {
    auto lines = dotnet::ReadAllLines(fileFullName);
    if (!lines) {
        log::Warning("File '" + fileFullName + "' does not exist");
        return nullptr;
    }
    return LoadFromLines(*lines);
}

// ConfigNode.cs:1113 LoadFromStringArray
ConfigNode::NodePtr ConfigNode::LoadFromLines(const std::vector<std::string>& cfgData) {
    std::vector<std::vector<std::string>> list;
    if (!PreFormatConfig(cfgData, list) || list.empty()) return nullptr;
    return RecurseFormat(list);
}

// ConfigNode.cs:4028 Parse: s.Split('\n', '\r') then RecurseFormat(PreFormatConfig(...)) (no empty check)
ConfigNode::NodePtr ConfigNode::Parse(std::string_view s) {
    const std::vector<std::string> lines = dotnet::Split(s, "\n\r", false);
    std::vector<std::vector<std::string>> list;
    PreFormatConfig(lines, list);
    return RecurseFormat(list);
}

namespace {

// One original line of PreFormatConfig (ConfigNode.cs:3884). KSP processes the line list backwards, splitting a line
// at its first '}' (or, if none, its first '{') into prefix / brace / rest and re-visiting the pieces; each line's
// processing only touches the pieces derived from it, so a recursive per-line function yields the identical list
// without the O(n^2) List.Insert of the original.
void PreFormatPiece(std::string_view piece, bool stripComment, std::vector<std::string>& out) {
    if (stripComment) {
        // string.IndexOf("//") (culture-sensitive in .NET; identical for ASCII)
        const size_t num2 = piece.find("//");
        if (num2 != std::string_view::npos) {
            if (num2 == 0) return;  // list.RemoveAt(num)
            piece = piece.substr(0, num2);
        }
    }
    const std::string_view t = dotnet::TrimView(piece);
    if (t.empty()) return;
    size_t pos = t.find('}');
    char brace = '}';
    if (pos == std::string_view::npos || (pos == 0 && t.size() == 1)) {
        if (pos != std::string_view::npos) {  // exactly "}"
            out.emplace_back(t);
            return;
        }
        pos = t.find('{');
        brace = '{';
        if (pos == std::string_view::npos || (pos == 0 && t.size() == 1)) {
            out.emplace_back(t);
            return;
        }
    }
    // prefix (visited later in KSP, i.e. processed as an independent piece), the brace, then the rest
    if (pos > 0) PreFormatPiece(t.substr(0, pos), false, out);
    out.emplace_back(1, brace);
    if (pos < t.size() - 1) PreFormatPiece(t.substr(pos + 1), false, out);
}

}  // namespace

// ConfigNode.cs:3884 PreFormatConfig. Returns false for KSP's "Empty part config file" (null) case.
bool ConfigNode::PreFormatConfig(const std::vector<std::string>& cfgData, std::vector<std::vector<std::string>>& out) {
    out.clear();
    if (cfgData.empty()) {
        log::Error("Error: Empty part config file");
        return false;
    }
    std::vector<std::string> list;
    list.reserve(cfgData.size());
    for (const std::string& line : cfgData) PreFormatPiece(line, true, list);
    out.reserve(list.size());
    for (const std::string& s : list) {
        std::vector<std::string> array = CustomEqualSplit(s);
        if (!array.empty()) out.push_back(std::move(array));
    }
    return true;
}

// ConfigNode.cs:3957 CustomEqualSplit
std::vector<std::string> ConfigNode::CustomEqualSplit(std::string_view str) {
    const size_t num = str.find('=');
    if (num == std::string_view::npos) return {dotnet::Trim(str)};
    if (num == 0) return {std::string(), dotnet::Trim(str.substr(1))};
    if (num == str.size() - 1) return {dotnet::Trim(str.substr(0, str.size() - 1)), std::string()};
    return {dotnet::Trim(str.substr(0, num)), dotnet::Trim(str.substr(num + 1))};
}

// ConfigNode.cs:3815 RecurseFormat(List<string[]>)
ConfigNode::NodePtr ConfigNode::RecurseFormat(const std::vector<std::vector<std::string>>& cfg) {
    size_t index = 0;
    auto node = std::make_shared<ConfigNode>("root");
    RecurseFormat(cfg, index, *node);
    return node;
}

// ConfigNode.cs:3823 RecurseFormat(cfg, ref index, node)
void ConfigNode::RecurseFormat(const std::vector<std::vector<std::string>>& cfg, size_t& index, ConfigNode& node) {
    auto nextLineIsOpenBrace = [&cfg](size_t i) {
        const size_t num = i + 1;
        return num < cfg.size() && cfg[num].size() == 1 && cfg[num][0] == "{";
    };
    for (;;) {
        if (index >= cfg.size()) return;
        const auto& line = cfg[index];
        if (line.size() == 2) {
            node.values_.Add(Value(line[0], line[1]));
            ++index;
            continue;
        }
        if (line[0] == "{") {
            auto child = std::make_shared<ConfigNode>("");
            node.nodes_.Add(child);
            ++index;
            RecurseFormat(cfg, index, *child);
            continue;
        }
        if (line[0] == "}") break;
        if (nextLineIsOpenBrace(index)) {
            auto child = std::make_shared<ConfigNode>(line[0]);
            node.nodes_.Add(child);
            index += 2;
            RecurseFormat(cfg, index, *child);
        } else {
            ++index;  // bare token: dropped
        }
    }
    ++index;
}

// ConfigNode.cs:1137 Save(path, header) -> StreamWriter (UTF-8 no BOM, NewLine "\r\n"); WriteRootNode (3992)
std::string ConfigNode::SaveToString(const std::optional<std::string>& header) const {
    std::string out;
    if (header.has_value() && !header->empty()) {
        out += "// ";
        out += *header;
        out += "\r\n\r\n";
    }
    for (const Value& v : values_) AppendValueLine(out, "", v, "\r\n");
    for (const NodePtr& n : nodes_) n->WriteNodeString(out, "");
    return out;
}

bool ConfigNode::Save(const std::string& fileFullName, const std::optional<std::string>& header) const {
    return dotnet::WriteAllBytes(fileFullName, SaveToString(header));
}

// ConfigNode.cs:4008 WriteNodeString
void ConfigNode::WriteNodeString(std::string& out, const std::string& indent) const {
    out += indent;
    out += name;
    if (!IsNullOrEmpty(comment)) {
        out += " // ";
        out += *comment;
    }
    out += "\r\n";
    out += indent;
    out += "{\r\n";
    const std::string text = indent + "\t";
    for (const Value& v : values_) AppendValueLine(out, text, v, "\r\n");
    for (const NodePtr& n : nodes_) n->WriteNodeString(out, text);
    out += indent;
    out += "}\r\n";
}

// ---------------------------------------------------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------------------------------------------------

// ConfigNode.cs:4272 HasValues
bool ConfigNode::HasValues(std::initializer_list<std::string_view> names) const {
    for (std::string_view n : names)
        if (!HasValue(n)) return false;
    return true;
}

// ConfigNode.cs:1319 AddValue(string, string)
void ConfigNode::AddValue(std::string_view n, std::string_view value) {
    values_.Add(Value(std::string(n), CleanupInput(value)));
}

// ConfigNode.cs:1297 AddValue(string, string, string)
void ConfigNode::AddValue(std::string_view n, std::string_view value, std::optional<std::string> vcomment) {
    values_.Add(Value(std::string(n), CleanupInput(value), std::move(vcomment)));
}

// ConfigNode.cs:1330-1592 typed AddValue overloads (no CleanupInput)
void ConfigNode::AddValue(std::string_view n, bool v) { values_.Add(Value(std::string(n), dotnet::FormatBool(v))); }
void ConfigNode::AddValue(std::string_view n, int32_t v) { values_.Add(Value(std::string(n), dotnet::FormatInt64(v))); }
void ConfigNode::AddValue(std::string_view n, uint32_t v) { values_.Add(Value(std::string(n), dotnet::FormatUInt64(v))); }
void ConfigNode::AddValue(std::string_view n, int64_t v) { values_.Add(Value(std::string(n), dotnet::FormatInt64(v))); }
void ConfigNode::AddValue(std::string_view n, uint64_t v) { values_.Add(Value(std::string(n), dotnet::FormatUInt64(v))); }
void ConfigNode::AddValue(std::string_view n, float v) { values_.Add(Value(std::string(n), FormatSingleG(v, 9))); }
void ConfigNode::AddValue(std::string_view n, double v) { values_.Add(Value(std::string(n), FormatDoubleG(v, 17))); }
void ConfigNode::AddValue(std::string_view n, const Vector2f& v) { values_.Add(Value(std::string(n), WriteVector(v))); }
void ConfigNode::AddValue(std::string_view n, const Vector3f& v) { values_.Add(Value(std::string(n), WriteVector(v))); }
void ConfigNode::AddValue(std::string_view n, const Vector3d& v) { values_.Add(Value(std::string(n), WriteVector(v))); }
void ConfigNode::AddValue(std::string_view n, const Vector4f& v) { values_.Add(Value(std::string(n), WriteVector(v))); }
void ConfigNode::AddValue(std::string_view n, const Quaternionf& v) { values_.Add(Value(std::string(n), WriteQuaternion(v))); }
void ConfigNode::AddValue(std::string_view n, const QuaternionD& v) { values_.Add(Value(std::string(n), WriteQuaternion(v))); }
void ConfigNode::AddValue(std::string_view n, const Matrix4x4f& v) { values_.Add(Value(std::string(n), WriteMatrix4x4(v))); }
void ConfigNode::AddValue(std::string_view n, const Color& v) { values_.Add(Value(std::string(n), WriteColor(v))); }
void ConfigNode::AddValue(std::string_view n, const Color32& v) { values_.Add(Value(std::string(n), WriteColor(v))); }
void ConfigNode::AddValueChar(std::string_view n, char32_t c) {
    std::string s;
    dotnet::AppendUtf8(s, c);
    values_.Add(Value(std::string(n), std::move(s)));
}

// ConfigNode.cs:1594 / 1599 GetValue (null when missing)
std::optional<std::string> ConfigNode::GetValue(std::string_view n) const {
    const std::string* p = values_.GetValuePtr(n, 0);
    return p ? std::optional<std::string>(*p) : std::nullopt;
}
std::optional<std::string> ConfigNode::GetValue(std::string_view n, int index) const {
    const std::string* p = values_.GetValuePtr(n, index);
    return p ? std::optional<std::string>(*p) : std::nullopt;
}

// ConfigNode.cs:1624-1642 SetValue(string ...)
bool ConfigNode::SetValue(std::string_view n, std::string_view v, bool c) { return values_.SetValue(n, v, std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, std::string_view v, int index, bool c) {
    return values_.SetValue(n, v, std::nullopt, index, c);
}
bool ConfigNode::SetValue(std::string_view n, std::string_view v, const std::optional<std::string>& cm, bool c) {
    return values_.SetValue(n, v, cm, 0, c);
}
bool ConfigNode::SetValue(std::string_view n, std::string_view v, const std::optional<std::string>& cm, int index, bool c) {
    return values_.SetValue(n, v, cm, index, c);
}
// ConfigNode.cs:1644-2082 typed SetValue
bool ConfigNode::SetValue(std::string_view n, bool v, bool c) { return values_.SetValue(n, dotnet::FormatBool(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, int32_t v, bool c) { return values_.SetValue(n, dotnet::FormatInt64(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, uint32_t v, bool c) { return values_.SetValue(n, dotnet::FormatUInt64(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, int64_t v, bool c) { return values_.SetValue(n, dotnet::FormatInt64(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, uint64_t v, bool c) { return values_.SetValue(n, dotnet::FormatUInt64(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, float v, bool c) { return values_.SetValue(n, FormatSingleG(v, 9), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, double v, bool c) { return values_.SetValue(n, FormatDoubleG(v, 17), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, const Vector2f& v, bool c) { return values_.SetValue(n, WriteVector(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, const Vector3f& v, bool c) { return values_.SetValue(n, WriteVector(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, const Vector3d& v, bool c) { return values_.SetValue(n, WriteVector(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, const Vector4f& v, bool c) { return values_.SetValue(n, WriteVector(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, const Quaternionf& v, bool c) { return values_.SetValue(n, WriteQuaternion(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, const QuaternionD& v, bool c) { return values_.SetValue(n, WriteQuaternion(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, const Matrix4x4f& v, bool c) { return values_.SetValue(n, WriteMatrix4x4(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, const Color& v, bool c) { return values_.SetValue(n, WriteColor(v), std::nullopt, 0, c); }
bool ConfigNode::SetValue(std::string_view n, const Color32& v, bool c) { return values_.SetValue(n, WriteColor(v), std::nullopt, 0, c); }

// ConfigNode.cs:2089
void ConfigNode::RemoveValues(std::initializer_list<std::string_view> names) {
    for (std::string_view n : names) values_.RemoveValues(n);
}

// ---------------------------------------------------------------------------------------------------------------------
// Nodes
// ---------------------------------------------------------------------------------------------------------------------

// ConfigNode.cs:2113
bool ConfigNode::HasNodeID(std::string_view nodeId) const { return nodes_.GetNodeID(nodeId) != nullptr; }
// ConfigNode.cs:2127
bool ConfigNode::HasNode(std::string_view n) const { return nodes_.Contains(n); }

// ConfigNode.cs:2146
ConfigNode* ConfigNode::AddNode(std::string_view n) {
    if (dotnet::TrimView(n).empty()) return nullptr;
    auto node = std::make_shared<ConfigNode>(n);
    nodes_.Add(node);
    return node.get();
}

// ConfigNode.cs:2157
ConfigNode* ConfigNode::AddNode(std::string_view n, std::optional<std::string> vcomment) {
    if (dotnet::TrimView(n).empty()) return nullptr;
    auto node = std::make_shared<ConfigNode>(n, std::move(vcomment));
    nodes_.Add(node);
    return node.get();
}

// ConfigNode.cs:2168
ConfigNode* ConfigNode::AddNode(const NodePtr& node) {
    nodes_.Add(node);
    return node.get();
}

// ConfigNode.cs:2174
ConfigNode* ConfigNode::AddNode(std::string_view n, const NodePtr& node) {
    if (dotnet::TrimView(n).empty()) return nullptr;
    node->name = std::string(n);
    nodes_.Add(node);
    return node.get();
}

// ConfigNode.cs:4033
bool ConfigNode::TryGetNode(std::string_view n, ConfigNode*& node) const {
    ConfigNode* node2 = GetNode(n);
    if (node2) {
        node = node2;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------
// TryGetValue family (ConfigNode.cs:4044-4296)
// ---------------------------------------------------------------------------------------------------------------------

bool ConfigNode::TryGetValue(std::string_view n, std::string& value) const {
    const std::string* s = GetValuePtr(n);
    if (!s) return false;
    value = *s;
    return true;
}

bool ConfigNode::TryGetValue(std::string_view n, std::vector<std::string>& value) const {
    const std::string* s = GetValuePtr(n);
    if (!s) return false;
    value = ParseExtensions::ParseArray(*s);
    return true;
}

#define KSP_TRYGET(TYPE, PARSE)                                           \
    bool ConfigNode::TryGetValue(std::string_view n, TYPE& value) const { \
        const std::string* s = GetValuePtr(n);                           \
        TYPE result{};                                                    \
        if (s && PARSE(*s, result)) {                                     \
            value = result;                                               \
            return true;                                                  \
        }                                                                 \
        return false;                                                     \
    }

namespace {
bool PSingle(const std::string& s, float& r) { return dotnet::TryParseSingle(s, r); }
bool PDouble(const std::string& s, double& r) { return dotnet::TryParseDouble(s, r); }
bool PInt32(const std::string& s, int32_t& r) { return dotnet::TryParseInt32(s, r); }
bool PUInt32(const std::string& s, uint32_t& r) { return dotnet::TryParseUInt32(s, r); }
bool PInt64(const std::string& s, int64_t& r) { return dotnet::TryParseInt64(s, r); }
bool PUInt64(const std::string& s, uint64_t& r) { return dotnet::TryParseUInt64(s, r); }
bool PBool(const std::string& s, bool& r) { return dotnet::TryParseBool(s, r); }
}  // namespace

KSP_TRYGET(float, PSingle)
KSP_TRYGET(double, PDouble)
KSP_TRYGET(int32_t, PInt32)
KSP_TRYGET(uint32_t, PUInt32)
KSP_TRYGET(int64_t, PInt64)
KSP_TRYGET(uint64_t, PUInt64)
KSP_TRYGET(bool, PBool)
KSP_TRYGET(Vector3f, ParseExtensions::TryParseVector3)
KSP_TRYGET(Vector3d, ParseExtensions::TryParseVector3d)
KSP_TRYGET(Vector2f, ParseExtensions::TryParseVector2)
KSP_TRYGET(Vector2d, ParseExtensions::TryParseVector2d)
KSP_TRYGET(Vector4f, ParseExtensions::TryParseVector4)
KSP_TRYGET(Vector4d, ParseExtensions::TryParseVector4d)
KSP_TRYGET(Quaternionf, ParseExtensions::TryParseQuaternion)
KSP_TRYGET(QuaternionD, ParseExtensions::TryParseQuaternionD)
KSP_TRYGET(Rect, ParseExtensions::TryParseRect)
KSP_TRYGET(Color32, ParseExtensions::TryParseColor32)
#undef KSP_TRYGET

// ConfigNode.cs:4242 TryGetValue(Color): ParseExtensions.TryParseColor, then ColorUtility.TryParseHtmlString
bool ConfigNode::TryGetValue(std::string_view n, Color& value) const {
    const std::string* s = GetValuePtr(n);
    if (!s) return false;
    Color result;
    if (ParseExtensions::TryParseColor(*s, result)) {
        value = result;
        return true;
    }
    if (ColorUtility::TryParseHtmlString(*s, result)) {
        value = result;
        return true;
    }
    return false;
}

// ConfigNode.cs:4277 TryGetValue(Guid)
bool ConfigNode::TryGetValue(std::string_view n, Guid& value) const {
    const std::string* s = GetValuePtr(n);
    if (!s) return false;
    Guid g;
    if (Guid::TryParse(*s, g)) {
        value = g;
        return true;
    }
    log::Info("Unable to TryGetValue Guid " + std::string(n));
    value = Guid::Empty();
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------
// Static writers / parsers (ConfigNode.cs:3525-3813)
// ---------------------------------------------------------------------------------------------------------------------

std::string ConfigNode::WriteBoolArray(std::span<const bool> flags) {
    std::string s;
    for (size_t i = 0; i < flags.size(); ++i) {
        if (i > 0) s += ",";
        s += dotnet::FormatBool(flags[i]);
    }
    return s;
}

std::string ConfigNode::WriteEnumIntArray(std::span<const int> flags) {
    std::string s;
    for (size_t i = 0; i < flags.size(); ++i) {
        if (i > 0) s += ",";
        s += dotnet::FormatInt64(flags[i]);
    }
    return s;
}

std::string ConfigNode::WriteStringArray(std::span<const std::string> strings) {
    std::string s;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) s += ",";
        s += strings[i];
    }
    return s;
}

std::string ConfigNode::WriteVector(const Vector2f& v) { return FormatSingleG(v.x, 9) + "," + FormatSingleG(v.y, 9); }
std::string ConfigNode::WriteVector(const Vector3f& v) {
    return FormatSingleG(v.x, 9) + "," + FormatSingleG(v.y, 9) + "," + FormatSingleG(v.z, 9);
}
std::string ConfigNode::WriteVector(const Vector3d& v) {
    return FormatDoubleG(v.x, 17) + "," + FormatDoubleG(v.y, 17) + "," + FormatDoubleG(v.z, 17);
}
std::string ConfigNode::WriteVector(const Vector4f& v) {
    return FormatSingleG(v.x, 9) + "," + FormatSingleG(v.y, 9) + "," + FormatSingleG(v.z, 9) + "," + FormatSingleG(v.w, 9);
}
std::string ConfigNode::WriteQuaternion(const Quaternionf& q) {
    return FormatSingleG(q.x, 9) + "," + FormatSingleG(q.y, 9) + "," + FormatSingleG(q.z, 9) + "," + FormatSingleG(q.w, 9);
}
std::string ConfigNode::WriteQuaternion(const QuaternionD& q) {
    return FormatDoubleG(q.x, 17) + "," + FormatDoubleG(q.y, 17) + "," + FormatDoubleG(q.z, 17) + "," + FormatDoubleG(q.w, 17);
}
std::string ConfigNode::WriteMatrix4x4(const Matrix4x4f& m) {
    const float v[16] = {m.m00, m.m01, m.m02, m.m03, m.m10, m.m11, m.m12, m.m13,
                         m.m20, m.m21, m.m22, m.m23, m.m30, m.m31, m.m32, m.m33};
    std::string s;
    for (int i = 0; i < 16; ++i) {
        if (i) s += ",";
        s += FormatSingleG(v[i], 9);
    }
    return s;
}
std::string ConfigNode::WriteColor(const Color& c) {
    return FormatSingleG(c.r, 9) + "," + FormatSingleG(c.g, 9) + "," + FormatSingleG(c.b, 9) + "," + FormatSingleG(c.a, 9);
}
std::string ConfigNode::WriteColor(const Color32& c) {
    return dotnet::FormatInt64(c.r) + "," + dotnet::FormatInt64(c.g) + "," + dotnet::FormatInt64(c.b) + "," +
           dotnet::FormatInt64(c.a);
}

namespace {
std::vector<std::string> SplitCst(std::string_view s) { return dotnet::Split(s, ", \t", true); }
}  // namespace

// ConfigNode.cs:3657
Vector2f ConfigNode::ParseVector2(std::string_view s) {
    const auto a = SplitCst(s);
    if (a.size() != 2) {
        log::Warning("WARNING: Vector2 entry is not formatted properly! Proper format for Vector2 is x,y");
        return Vector2f::zero();
    }
    return {dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1])};
}

// ConfigNode.cs:3668
Vector3f ConfigNode::ParseVector3(std::string_view s) {
    const auto a = SplitCst(s);
    if (a.size() != 3) {
        log::Warning("WARNING: Vector3 entry is not formatted properly! Proper format for Vector3 is x,y,z");
        return Vector3f::zero();
    }
    return {dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1]), dotnet::ParseSingle(a[2])};
}

// ConfigNode.cs:3679
Vector3d ConfigNode::ParseVector3D(std::string_view s) {
    const auto a = SplitCst(s);
    if (a.size() != 3) {
        log::Warning("WARNING: Vector3D entry is not formatted properly! Proper format for Vector3D is x,y,z");
        return Vector3d::zero();
    }
    return {dotnet::ParseDouble(a[0]), dotnet::ParseDouble(a[1]), dotnet::ParseDouble(a[2])};
}

// ConfigNode.cs:3690
Vector4f ConfigNode::ParseVector4(std::string_view s) {
    const auto a = SplitCst(s);
    if (a.size() != 4) {
        log::Warning("WARNING: Vector4 entry is not formatted properly! Proper format for Vector4s is x,y,z,w");
        return Vector4f::zero();
    }
    return {dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1]), dotnet::ParseSingle(a[2]), dotnet::ParseSingle(a[3])};
}

// ConfigNode.cs:3701
Quaternionf ConfigNode::ParseQuaternion(std::string_view s) {
    const auto a = SplitCst(s);
    if (a.size() != 4) {
        log::Warning("WARNING: Quaternion entry is not formatted properly! Proper format for Quaternion is x,y,z,w");
        return Quaternionf::identity();
    }
    return {dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1]), dotnet::ParseSingle(a[2]), dotnet::ParseSingle(a[3])};
}

// ConfigNode.cs:3712
QuaternionD ConfigNode::ParseQuaternionD(std::string_view s) {
    const auto a = SplitCst(s);
    if (a.size() != 4) {
        log::Warning("WARNING: QuaternionD entry is not formatted properly! Proper format for QuaternionD is x,y,z,w");
        return QuaternionD::identity();
    }
    return {dotnet::ParseDouble(a[0]), dotnet::ParseDouble(a[1]), dotnet::ParseDouble(a[2]), dotnet::ParseDouble(a[3])};
}

// ConfigNode.cs:3723 (sic: m11 assigned twice, m12 gets element 7, m13 stays 0)
Matrix4x4f ConfigNode::ParseMatrix4x4(std::string_view s) {
    const auto a = SplitCst(s);
    if (a.size() != 16) {
        log::Warning("WARNING: Matrix4x4 entry is not formatted properly! Proper format for Matrix4x4 is 16 csv floats (m00,m01,m02,m03,m10,m11..m33)");
        return Matrix4x4f::identity();
    }
    Matrix4x4f m = Matrix4x4f::identity();
    m.m00 = dotnet::ParseSingle(a[0]);
    m.m01 = dotnet::ParseSingle(a[1]);
    m.m02 = dotnet::ParseSingle(a[2]);
    m.m03 = dotnet::ParseSingle(a[3]);
    m.m10 = dotnet::ParseSingle(a[4]);
    m.m11 = dotnet::ParseSingle(a[5]);
    m.m11 = dotnet::ParseSingle(a[6]);
    m.m12 = dotnet::ParseSingle(a[7]);
    m.m20 = dotnet::ParseSingle(a[8]);
    m.m21 = dotnet::ParseSingle(a[9]);
    m.m22 = dotnet::ParseSingle(a[10]);
    m.m23 = dotnet::ParseSingle(a[11]);
    m.m30 = dotnet::ParseSingle(a[12]);
    m.m31 = dotnet::ParseSingle(a[13]);
    m.m32 = dotnet::ParseSingle(a[14]);
    m.m33 = dotnet::ParseSingle(a[15]);
    return m;
}

// ConfigNode.cs:3751
Color ConfigNode::ParseColor(std::string_view s) {
    const auto a = SplitCst(s);
    if (a.size() >= 3 && a.size() <= 4) {
        if (a.size() == 3) return {dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1]), dotnet::ParseSingle(a[2])};
        return {dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1]), dotnet::ParseSingle(a[2]), dotnet::ParseSingle(a[3])};
    }
    log::Warning("WARNING: Color entry is not formatted properly! Proper format for Colors is r,g,b{,a}");
    return Color::white();
}

// ConfigNode.cs:3766
bool ConfigNode::CheckAndParseColor(std::string_view s, Color& color) {
    try {
        const auto a = SplitCst(s);
        if (a.size() < 3 || a.size() > 4) {
            log::Warning("WARNING: Color entry is not formatted properly! Proper format for Colors is r,g,b{,a}");
            color = Color::white();
            return false;
        }
        if (a.size() == 3) color = Color(dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1]), dotnet::ParseSingle(a[2]));
        else
            color = Color(dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1]), dotnet::ParseSingle(a[2]),
                          dotnet::ParseSingle(a[3]));
    } catch (const std::exception&) {
        log::Warning("WARNING: Color entry is not formatted properly! Proper format for Colors is r,g,b{,a}");
        color = Color::white();
        return false;
    }
    return true;
}

// ConfigNode.cs:3795
Color32 ConfigNode::ParseColor32(std::string_view s) {
    const auto a = SplitCst(s);
    if (a.size() >= 3 && a.size() <= 4) {
        if (a.size() == 3) return {dotnet::ParseByte(a[0]), dotnet::ParseByte(a[1]), dotnet::ParseByte(a[2]), 255};
        return {dotnet::ParseByte(a[0]), dotnet::ParseByte(a[1]), dotnet::ParseByte(a[2]), dotnet::ParseByte(a[3])};
    }
    log::Warning("WARNING: Color entry is not formatted properly! Proper format for Colors is r,g,b{,a}");
    return Color32::FromColor(Color::white());
}

// =====================================================================================================================
// Guid (System.Guid string constructor: formats N, D, B, P, X)
// =====================================================================================================================
namespace {
int Hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
bool HexRun(std::string_view s, size_t pos, size_t n, uint64_t& out) {
    if (pos + n > s.size()) return false;
    out = 0;
    for (size_t i = 0; i < n; ++i) {
        const int h = Hex(s[pos + i]);
        if (h < 0) return false;
        out = (out << 4) | static_cast<uint64_t>(h);
    }
    return true;
}
void StoreFields(Guid& g, uint64_t a, uint64_t b, uint64_t c, const uint8_t d[8]) {
    g.bytes[0] = static_cast<uint8_t>(a);
    g.bytes[1] = static_cast<uint8_t>(a >> 8);
    g.bytes[2] = static_cast<uint8_t>(a >> 16);
    g.bytes[3] = static_cast<uint8_t>(a >> 24);
    g.bytes[4] = static_cast<uint8_t>(b);
    g.bytes[5] = static_cast<uint8_t>(b >> 8);
    g.bytes[6] = static_cast<uint8_t>(c);
    g.bytes[7] = static_cast<uint8_t>(c >> 8);
    for (int i = 0; i < 8; ++i) g.bytes[8 + i] = d[i];
}
}  // namespace

bool Guid::TryParse(std::string_view s, Guid& out) {
    s = dotnet::TrimView(s);
    uint64_t a, b, c, t;
    uint8_t d[8];
    auto parseD = [&](std::string_view v) -> bool {  // xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
        if (v.size() != 36 || v[8] != '-' || v[13] != '-' || v[18] != '-' || v[23] != '-') return false;
        if (!HexRun(v, 0, 8, a) || !HexRun(v, 9, 4, b) || !HexRun(v, 14, 4, c)) return false;
        for (int i = 0; i < 2; ++i) {
            if (!HexRun(v, 19 + 2 * static_cast<size_t>(i), 2, t)) return false;
            d[i] = static_cast<uint8_t>(t);
        }
        for (int i = 0; i < 6; ++i) {
            if (!HexRun(v, 24 + 2 * static_cast<size_t>(i), 2, t)) return false;
            d[2 + i] = static_cast<uint8_t>(t);
        }
        return true;
    };
    bool ok = false;
    if (s.size() == 32) {  // N
        ok = HexRun(s, 0, 8, a) && HexRun(s, 8, 4, b) && HexRun(s, 12, 4, c);
        for (int i = 0; ok && i < 8; ++i) {
            ok = HexRun(s, 16 + 2 * static_cast<size_t>(i), 2, t);
            d[i] = static_cast<uint8_t>(t);
        }
    } else if (s.size() == 36) {  // D
        ok = parseD(s);
    } else if (s.size() == 38 && ((s.front() == '{' && s.back() == '}') || (s.front() == '(' && s.back() == ')'))) {  // B / P
        ok = parseD(s.substr(1, 36));
    } else if (!s.empty() && s.front() == '{' && s.find("0x") != std::string_view::npos) {  // X
        std::string compact;
        for (char ch : s)
            if (!dotnet::IsWhiteSpace(static_cast<unsigned char>(ch))) compact.push_back(ch);
        // {0xdddddddd,0xdddd,0xdddd,{0xdd,0xdd,0xdd,0xdd,0xdd,0xdd,0xdd,0xdd}}
        size_t p = 0;
        auto expect = [&](std::string_view lit) {
            if (compact.compare(p, lit.size(), lit) != 0) return false;
            p += lit.size();
            return true;
        };
        auto hexField = [&](uint64_t& v, size_t maxDigits) {
            if (!expect("0x") && !expect("0X")) return false;
            size_t n = 0;
            v = 0;
            while (p < compact.size() && Hex(compact[p]) >= 0 && n < maxDigits) {
                v = (v << 4) | static_cast<uint64_t>(Hex(compact[p]));
                ++p;
                ++n;
            }
            return n > 0;
        };
        ok = expect("{") && hexField(a, 8) && expect(",") && hexField(b, 4) && expect(",") && hexField(c, 4) && expect(",{");
        for (int i = 0; ok && i < 8; ++i) {
            ok = hexField(t, 2) && (i == 7 ? expect("}}") : expect(","));
            d[i] = static_cast<uint8_t>(t);
        }
        ok = ok && p == compact.size();
    }
    if (!ok) return false;
    StoreFields(out, a, b, c, d);
    return true;
}

Guid Guid::Parse(std::string_view s) {
    Guid g;
    if (!TryParse(s, g)) throw dotnet::FormatException("Guid should contain 32 digits with 4 dashes (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx).");
    return g;
}

std::string Guid::ToString(char format) const {
    static const char* hx = "0123456789abcdef";
    auto put = [&](std::string& s, uint8_t v) {
        s.push_back(hx[v >> 4]);
        s.push_back(hx[v & 15]);
    };
    std::string s;
    // Data1 (LE), Data2 (LE), Data3 (LE), Data4
    put(s, bytes[3]); put(s, bytes[2]); put(s, bytes[1]); put(s, bytes[0]);
    if (format != 'N') s.push_back('-');
    put(s, bytes[5]); put(s, bytes[4]);
    if (format != 'N') s.push_back('-');
    put(s, bytes[7]); put(s, bytes[6]);
    if (format != 'N') s.push_back('-');
    put(s, bytes[8]); put(s, bytes[9]);
    if (format != 'N') s.push_back('-');
    for (int i = 10; i < 16; ++i) put(s, bytes[i]);
    if (format == 'B') return "{" + s + "}";
    if (format == 'P') return "(" + s + ")";
    return s;
}

}  // namespace ksp
