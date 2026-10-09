#include "ksp/parts/part_data.hpp"

#include <stdexcept>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/resources/part_resource_library.hpp"

namespace ksp {

// ---- AttachRules.cs --------------------------------------------------------------------------------------------------
// AttachRules.cs:34
AttachRules AttachRules::Parse(std::string_view value) {
    const std::vector<std::string> a = dotnet::Split(value, ",", false);
    if (a.size() < 5) throw Exception("Attach rules are 8 ints, either ones or zeros, separated by commas!");
    AttachRules r;
    r.stack = a[0] == "1";
    r.srfAttach = a[1] == "1";
    r.allowStack = a[2] == "1";
    r.allowSrfAttach = a[3] == "1";
    r.allowCollision = a[4] == "1";
    r.allowDock = a.size() >= 6 ? a[5] == "1" : false;
    r.allowRotate = a.size() >= 7 ? a[6] == "1" : false;
    r.allowRoot = a.size() >= 8 ? a[7] == "1" : true;
    return r;
}

// AttachRules.cs:74
std::string AttachRules::String() const {
    auto b = [](bool v) { return v ? "1" : "0"; };
    return std::string(b(stack)) + "," + b(srfAttach) + "," + b(allowStack) + "," + b(allowSrfAttach) + "," +
           b(allowCollision) + "," + b(allowDock) + "," + b(allowRotate);
}

// ---- field table -------------------------------------------------------------------------------------------------------
namespace {

#define KSP_PF_ADDR_Object(n, owner) nullptr
#define KSP_PF_ADDR_Single(n, owner) [](PartFields& p, CompoundPartFields& c) -> void* { return &owner.n; }
#define KSP_PF_ADDR_Double KSP_PF_ADDR_Single
#define KSP_PF_ADDR_Int32 KSP_PF_ADDR_Single
#define KSP_PF_ADDR_UInt32 KSP_PF_ADDR_Single
#define KSP_PF_ADDR_Boolean KSP_PF_ADDR_Single
#define KSP_PF_ADDR_String KSP_PF_ADDR_Single
#define KSP_PF_ADDR_Vector3 KSP_PF_ADDR_Single
#define KSP_PF_ADDR_Vector3d KSP_PF_ADDR_Single
#define KSP_PF_ADDR_Quaternion KSP_PF_ADDR_Single
#define KSP_PF_ADDR_Color KSP_PF_ADDR_Single
#define KSP_PF_ADDR_Enum KSP_PF_ADDR_Single
#define KSP_PF_ADDR_AttachRules KSP_PF_ADDR_Single
#define KSP_PF_ADDR_StringList KSP_PF_ADDR_Single

std::vector<PartFieldInfo> BuildTable() {
    std::vector<PartFieldInfo> t;
#define KSP_PART_FIELD(name, kind, cppType, csType, enumTable, def, line) \
    t.push_back({#name, FieldKind::kind, csType, enumTable, line, false, KSP_PF_ADDR_##kind(name, p)});
#include "ksp/parts/part_fields.inc"
#undef KSP_PART_FIELD
#define KSP_COMPOUNDPART_FIELD(name, kind, cppType, csType, enumTable, def, line) \
    t.push_back({#name, FieldKind::kind, csType, enumTable, line, true, KSP_PF_ADDR_##kind(name, c)});
#include "ksp/parts/compound_part_fields.inc"
#undef KSP_COMPOUNDPART_FIELD
    return t;
}

}  // namespace

const std::vector<PartFieldInfo>& PartFieldTable() {
    static const std::vector<PartFieldInfo> t = BuildTable();
    return t;
}

const PartFieldInfo* FindPartField(std::string_view name, bool compoundPart) {
    const auto& t = PartFieldTable();
    if (compoundPart)
        for (const PartFieldInfo& f : t)
            if (f.compoundPart && name == f.name) return &f;
    for (const PartFieldInfo& f : t)
        if (!f.compoundPart && name == f.name) return &f;
    return nullptr;
}

// ---- DragCube.cs -------------------------------------------------------------------------------------------------------
// DragCube.cs:106
bool DragCube::Load(const std::vector<std::string>& data) {
    if (data.size() != 25 && data.size() != 24) {
        log::Error("DragCube: Config string does not have correct number of values");
        return false;
    }
    // data.Length == 12 can never be true here: the name is always data[0] (sic)
    name_ = data[0];
    auto at = [&](size_t i) -> const std::string& {
        if (i >= data.size()) throw std::out_of_range("Index was outside the bounds of the array.");
        return data[i];
    };
    float result = 0.0f;
    size_t num2 = 1;
    for (int num = 0; num < 6; num++) {
        if (!cs::FloatTryParse(at(num2), result)) {
            log::Error("DragCube: Cannot load, area value " + std::to_string(num) + "'" + at(num2) + "' cannot be parsed");
            return false;
        }
        area_[num] = result;
        if (!cs::FloatTryParse(at(num2 + 1), result)) {
            log::Error("DragCube: Cannot load, drag value " + std::to_string(num) + "'" + at(num2 + 1) + "' cannot be parsed");
            return false;
        }
        drag_[num] = result;
        if (!cs::FloatTryParse(at(num2 + 2), result)) {
            log::Error("DragCube: Cannot load, depth value " + std::to_string(num) + "'" + at(num2 + 2) + "' cannot be parsed");
            return false;
        }
        depth_[num] = result;
        num2 += 3;
    }
    center_ = Vector3f{};
    size_ = Vector3f{};
    const char* what[6] = {"bounds x value '", "bounds y value '", "bounds z value '",
                           "bounds size x value '", "bounds size y value '", "bounds size z value '"};
    float* dst[6] = {&center_.x, &center_.y, &center_.z, &size_.x, &size_.y, &size_.z};
    for (int k = 0; k < 6; k++) {
        if (!cs::FloatTryParse(at(num2 + k), result)) {
            log::Error(std::string("DragCube: Cannot load, ") + what[k] + at(num2 + k) + "' cannot be parsed");
            return false;
        }
        *dst[k] = result;
    }
    return true;
}

// DragCube.cs:196
std::string DragCube::SaveToString() const {
    auto g4 = [](float f) { return dotnet::FormatSingleG(f, 4); };
    std::string text;
    if (!name_.empty()) text += name_;
    for (int i = 0; i < 6; i++) {
        if (!text.empty()) text += ", ";
        text += g4(area_[i]) + "," + g4(drag_[i]) + "," + g4(depth_[i]);
    }
    text += ", " + g4(center_.x) + "," + g4(center_.y) + "," + g4(center_.z);
    return text + ", " + g4(size_.x) + "," + g4(size_.y) + "," + g4(size_.z);
}

// ---- DragCubeList.cs (data side) ---------------------------------------------------------------------------------------
void DragCubeList::ClearCubes() {
    cubes_.clear();
    procedural_ = false;
}

// DragCubeList.cs:864
ConfigNode::NodePtr DragCubeList::SaveCubes() const {
    ConfigNode::NodePtr n = ConfigNode::Create("DRAG_CUBE");
    if (none_) {
        n->AddValue("none", "True");
        return n;
    }
    if (procedural_) {
        n->AddValue("procedural", "True");
        return n;
    }
    for (size_t i = cubes_.size(); i-- > 0;) n->AddValue("cube", cubes_[i].SaveToString());
    return n;
}

// DragCubeList.cs:885
bool DragCubeList::LoadCubes(const ConfigNode& node) {
    ClearCubes();
    if (node.HasValue("none")) {
        none_ = true;
        return true;
    }
    if (node.HasValue("procedural")) {
        procedural_ = true;
        return true;
    }
    const std::vector<std::string> values = node.GetValues("cube");
    for (size_t num = values.size(); num-- > 0;) {
        const std::vector<std::string> data = dotnet::Split(values[num], ", ", true);
        DragCube c;
        if (!c.Load(data)) {
            log::Error("DragCube: Invalid cube line " + std::to_string(num + 1));
            return false;
        }
        cubes_.push_back(std::move(c));
    }
    return true;
}

// DragCubeList.cs:919
bool DragCubeList::LoadCubes(const DragCubeList& cubes) {
    bool result = false;
    const bool flag = procedural_;
    ClearCubes();
    if (cubes.cubes_.empty()) {
        procedural_ = flag;
        return false;
    }
    for (const DragCube& c : cubes.cubes_) {
        cubes_.push_back(c);
        result = true;
    }
    procedural_ = flag;
    return result;
}

// ---- PartResource.cs ---------------------------------------------------------------------------------------------------
// PartResource.cs:108
void PartResource::Load(const ConfigNode& node) {
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    if (node.HasValue("amount")) amount = cs::DoubleParse(*node.GetValue("amount"));
    if (node.HasValue("maxAmount")) maxAmount = cs::DoubleParse(*node.GetValue("maxAmount"));
    SetFlowState(node.HasValue("flowState") ? cs::BoolParse(*node.GetValue("flowState")) : true);  // the property: fires the event
    auto def = [&]() -> const PartResourceDefinition& {
        const PartResourceDefinition* d = lib ? lib->GetDefinition(MonoStringHash(resourceName)) : nullptr;
        if (!d) throw NullReferenceException("Object reference not set to an instance of an object");
        return *d;
    };
    isTweakable = node.HasValue("isTweakable") ? cs::BoolParse(*node.GetValue("isTweakable")) : def().isTweakable();
    isVisible = node.HasValue("isVisible") ? cs::BoolParse(*node.GetValue("isVisible")) : def().isVisible();
    hideFlow = node.HasValue("hideFlow") ? cs::BoolParse(*node.GetValue("hideFlow")) : false;
    if (node.HasValue("flowMode")) {
        // ConfigNode.TryGetEnum (ConfigNode.cs:4298) into the field `_flowMode` (no event): default Both, Enum.IsDefined(name)
        // then Enum.Parse
        flowMode = PartResourceFlowMode::Both;
        const std::string v = *node.GetValue("flowMode");
        for (const auto& [n, val] : kEnum_PartResource_FlowMode.members)
            if (n == v) flowMode = static_cast<PartResourceFlowMode>(val);
    } else {
        SetFlowMode(PartResourceFlowMode::Both);  // the property: fires the event
    }
}

// PartResource.cs:160
void PartResource::Save(ConfigNode& node) const {
    node.AddValue("name", info->name());
    node.AddValue("amount", amount);
    node.AddValue("maxAmount", maxAmount);
    node.AddValue("flowState", flowState);
    node.AddValue("isTweakable", isTweakable);
    node.AddValue("hideFlow", hideFlow);
    node.AddValue("isVisible", isVisible);
    node.AddValue("flowMode", kEnum_PartResource_FlowMode.ToString(static_cast<int64_t>(flowMode)));
}

// PartResource.cs:36 flowState setter
void PartResource::SetFlowState(bool value) {
    const bool flag = flowState;
    flowState = value;
    if (value != flag)
        events::GameEvents::onPartResourceFlowStateChange.Fire(events::GameEvents::HostedFromToAction<PartResource*, bool>(this, flag, value));
}

// PartResource.cs:53 flowMode setter
void PartResource::SetFlowMode(PartResourceFlowMode value) {
    const PartResourceFlowMode old = flowMode;
    flowMode = value;
    if (value != old)
        events::GameEvents::onPartResourceFlowModeChange.Fire(events::GameEvents::HostedFromToAction<PartResource*, PartResource_FlowMode>(
            this, static_cast<PartResource_FlowMode>(static_cast<int>(old)), static_cast<PartResource_FlowMode>(static_cast<int>(value))));
}

// PartResource.cs:172
void PartResource::SetInfo(const PartResourceDefinition* i) {
    info = i;
    resourceName = i->name();
}

// PartResource.cs:178 (string concatenation of a double: Mono's Double.ToString() = "G15"-style shortest form, dotnet "G")
std::string PartResource::GetInfo() const {
    return info->displayName() + ": " + dotnet::FormatDoubleG(amount) + " / " + dotnet::FormatDoubleG(maxAmount);
}

// PartResource.cs:183
bool PartResource::CanProvide(double demand) const {
    if (demand > 0.0) return amount > 0.0;
    if (demand < 0.0) return amount < maxAmount;
    return false;
}

// PartResource.cs:196
bool PartResource::CanProvide(bool pulling) const {
    if (pulling) {
        if (amount > 0.0 && flowState) return (static_cast<int>(flowMode) & static_cast<int>(PartResourceFlowMode::Out)) > 0;
        return false;
    }
    if (amount < maxAmount && flowState) return (static_cast<int>(flowMode) & static_cast<int>(PartResourceFlowMode::In)) > 0;
    return false;
}

// PartResource.cs:213
bool PartResource::Flowing(bool pulling) const {
    if (flowState) {
        if (pulling) {
            if ((static_cast<int>(flowMode) & static_cast<int>(PartResourceFlowMode::Out)) > 0) return true;
        } else if ((static_cast<int>(flowMode) & static_cast<int>(PartResourceFlowMode::In)) > 0) {
            return true;
        }
    }
    return false;
}

// PartResource.cs:94 Copy (the `part` link is not copied: the copy constructors set it from the source before Copy)
void PartResource::Copy(const PartResource& res) {
    info = res.info;
    resourceName = res.resourceName;
    amount = res.amount;
    maxAmount = res.maxAmount;
    flowState = res.flowState;
    isTweakable = res.isTweakable;
    isVisible = res.isVisible;
    hideFlow = res.hideFlow;
    flowMode = res.flowMode;
    simulationResource = res.simulationResource;
}

}  // namespace ksp
