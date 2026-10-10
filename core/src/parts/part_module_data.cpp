#include "ksp/parts/part_module_data.hpp"

#include <map>
#include <memory>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/dotnet/utf16.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"

namespace ksp {

// ---- generated class table ---------------------------------------------------------------------------------------------
#define KSP_MODULE_ENUM_BEGIN(eid, name, full, isFlags, file, line) const EnumTable eid{name, isFlags, file, line, {
#define KSP_MODULE_ENUM_MEMBER(eid, name, value) {name, value},
#define KSP_MODULE_ENUM_END(eid) }};
#define KSP_MODULE_CLASS(id, name, full, base, isAbstract, file, line)
#define KSP_MODULE_FIELD(id, field, cstype, tname, pers, prop, priv, init, line, enumId)
#define KSP_MODULE_CLASS_END(id)
#include "ksp/parts/module_schema.inc"
#define KSP_HOST_CLASS(id, name, full, base, isAbstract, file, line)
#define KSP_HOST_FIELD(id, field, cstype, tname, pers, prop, priv, init, line, enumId)
#define KSP_HOST_CLASS_END(id)
#include "ksp/parts/host_schema.inc"
#undef KSP_HOST_CLASS
#undef KSP_HOST_FIELD
#undef KSP_HOST_CLASS_END
#undef KSP_MODULE_ENUM_BEGIN
#undef KSP_MODULE_ENUM_MEMBER
#undef KSP_MODULE_ENUM_END
#undef KSP_MODULE_CLASS
#undef KSP_MODULE_FIELD
#undef KSP_MODULE_CLASS_END

namespace {

// Every enum table of the schemas, by Type.Name and full name (FindModuleEnum).
const std::map<std::string, const EnumTable*, std::less<>>& EnumIndex() {
    static const std::map<std::string, const EnumTable*, std::less<>> idx = [] {
        std::map<std::string, const EnumTable*, std::less<>> m;
#define KSP_MODULE_ENUM_BEGIN(eid, name, full, isFlags, file, line) \
    m.emplace(name, &eid);                                          \
    m.emplace(full, &eid);
#define KSP_MODULE_ENUM_MEMBER(eid, name, value)
#define KSP_MODULE_ENUM_END(eid)
#define KSP_MODULE_CLASS(id, name, full, base, isAbstract, file, line)
#define KSP_MODULE_FIELD(id, field, cstype, tname, pers, prop, priv, init, line, enumId)
#define KSP_MODULE_CLASS_END(id)
#include "ksp/parts/module_schema.inc"
#define KSP_HOST_CLASS(id, name, full, base, isAbstract, file, line)
#define KSP_HOST_FIELD(id, field, cstype, tname, pers, prop, priv, init, line, enumId)
#define KSP_HOST_CLASS_END(id)
#include "ksp/parts/host_schema.inc"
#undef KSP_HOST_CLASS
#undef KSP_HOST_FIELD
#undef KSP_HOST_CLASS_END
#undef KSP_MODULE_ENUM_BEGIN
#undef KSP_MODULE_ENUM_MEMBER
#undef KSP_MODULE_ENUM_END
#undef KSP_MODULE_CLASS
#undef KSP_MODULE_FIELD
#undef KSP_MODULE_CLASS_END
        return m;
    }();
    return idx;
}

void LinkClasses(std::vector<ModuleClassInfo>& v) {
    // link bases by Type.Name (the last class of a name, like AssemblyLoader's dictionary)
    std::map<std::string, const ModuleClassInfo*, std::less<>> byName;
    for (const ModuleClassInfo& c : v) byName[c.name] = &c;
    for (ModuleClassInfo& c : v) {
        auto it = byName.find(c.baseName);
        c.base = (it != byName.end() && it->second != &c) ? it->second : nullptr;
    }
}

std::vector<ModuleClassInfo> BuildHostClasses() {
    std::vector<ModuleClassInfo> v;
#define KSP_MODULE_ENUM_BEGIN(eid, name, full, isFlags, file, line)
#define KSP_MODULE_ENUM_MEMBER(eid, name, value)
#define KSP_MODULE_ENUM_END(eid)
#define KSP_HOST_CLASS(id, name, full, base, isAbstract, file, line) \
    v.push_back(ModuleClassInfo{name, full, base, isAbstract, file, line, {}, nullptr});
#define KSP_HOST_FIELD(id, field, cstype, tname, pers, prop, priv, init, line, enumId) \
    v.back().ownFields.push_back(ModuleFieldInfo{field, cstype, tname, pers, prop, priv, init, line, enumId});
#define KSP_HOST_CLASS_END(id)
#include "ksp/parts/host_schema.inc"
#undef KSP_HOST_CLASS
#undef KSP_HOST_FIELD
#undef KSP_HOST_CLASS_END
#undef KSP_MODULE_ENUM_BEGIN
#undef KSP_MODULE_ENUM_MEMBER
#undef KSP_MODULE_ENUM_END
    LinkClasses(v);
    for (ModuleClassInfo& c : v)
        for (ModuleFieldInfo& f : c.ownFields) f.owner = &c;
    return v;
}

std::vector<ModuleClassInfo> BuildClasses() {
    std::vector<ModuleClassInfo> v;
#define KSP_MODULE_ENUM_BEGIN(eid, name, full, isFlags, file, line)
#define KSP_MODULE_ENUM_MEMBER(eid, name, value)
#define KSP_MODULE_ENUM_END(eid)
#define KSP_MODULE_CLASS(id, name, full, base, isAbstract, file, line) \
    v.push_back(ModuleClassInfo{name, full, base, isAbstract, file, line, {}, nullptr});
#define KSP_MODULE_FIELD(id, field, cstype, tname, pers, prop, priv, init, line, enumId) \
    v.back().ownFields.push_back(ModuleFieldInfo{field, cstype, tname, pers, prop, priv, init, line, enumId});
#define KSP_MODULE_CLASS_END(id)
#include "ksp/parts/module_schema.inc"
#undef KSP_MODULE_ENUM_BEGIN
#undef KSP_MODULE_ENUM_MEMBER
#undef KSP_MODULE_ENUM_END
#undef KSP_MODULE_CLASS
#undef KSP_MODULE_FIELD
#undef KSP_MODULE_CLASS_END
    LinkClasses(v);
    for (ModuleClassInfo& c : v)
        for (ModuleFieldInfo& f : c.ownFields) f.owner = &c;
    return v;
}

const std::map<std::string, const ModuleClassInfo*, std::less<>>& ClassIndex() {
    static const std::map<std::string, const ModuleClassInfo*, std::less<>> idx = [] {
        std::map<std::string, const ModuleClassInfo*, std::less<>> m;
        for (const ModuleClassInfo& c : ModuleClasses()) m[c.name] = &c;
        return m;
    }();
    return idx;
}

// BaseField.ReadPvt's vector/quaternion split
std::vector<std::string> SplitVector(std::string_view s) { return dotnet::Split(s, "(, )", true); }

}  // namespace

const std::vector<ModuleClassInfo>& ModuleClasses() {
    static const std::vector<ModuleClassInfo> classes = BuildClasses();
    return classes;
}

const std::vector<ModuleClassInfo>& HostClasses() {
    static const std::vector<ModuleClassInfo> classes = BuildHostClasses();
    return classes;
}

namespace {
const ModuleClassInfo* GetHostClassDerivedFrom(std::string_view name, std::string_view root) {
    const ModuleClassInfo* r = nullptr;  // the last class of a name
    for (const ModuleClassInfo& c : HostClasses())
        if (name == c.name) r = &c;
    return r && r->Is(root) ? r : nullptr;
}
}  // namespace

const ModuleClassInfo* GetScenarioModuleClass(std::string_view name) { return GetHostClassDerivedFrom(name, "ScenarioModule"); }
const ModuleClassInfo* GetVesselModuleClass(std::string_view name) { return GetHostClassDerivedFrom(name, "VesselModule"); }

const EnumTable* FindModuleEnum(std::string_view name) {
    const auto& idx = EnumIndex();
    auto it = idx.find(name);
    return it == idx.end() ? nullptr : it->second;
}

const ModuleClassInfo* GetPartModuleClass(std::string_view name) {
    const auto& idx = ClassIndex();
    auto it = idx.find(name);
    return it == idx.end() ? nullptr : it->second;
}

bool ModuleClassInfo::Is(std::string_view className) const {
    for (const ModuleClassInfo* c = this; c; c = c->base)
        if (className == c->name) return true;
    return false;
}

std::vector<const ModuleFieldInfo*> ModuleClassInfo::Fields() const {
    std::vector<const ModuleFieldInfo*> out;
    for (const ModuleFieldInfo& f : ownFields) out.push_back(&f);
    for (const ModuleClassInfo* c = base; c; c = c->base)
        for (const ModuleFieldInfo& f : c->ownFields)
            if (!f.isPrivate) out.push_back(&f);
    return out;
}

const ModuleFieldInfo* ModuleClassInfo::FindField(std::string_view name) const {
    for (const ModuleFieldInfo* f : Fields())
        if (name == f->name) return f;
    return nullptr;
}

const ModuleFieldValue* PartModuleData::FindCfgField(std::string_view name) const {
    const ModuleFieldValue* r = nullptr;
    for (const ModuleFieldValue& v : cfgFields)
        if (name == v.field->name) r = &v;
    return r;
}

bool IsConfigNodeFieldType(const ModuleFieldInfo& info) {
    const std::string_view t = info.typeName;
    return t == "FloatCurve" || t == "DoubleCurve";
}

// BaseField.cs:712 ReadPvt
bool ReadModuleFieldValue(const ModuleFieldInfo& info, std::string_view value, ModuleValue& out) {
    const std::string_view t = info.typeName;
    out = std::monostate{};
    if (t == "Boolean") {
        bool r;
        if (!cs::BoolTryParse(value, r)) return false;
        out = r;
        return true;
    }
    if (t == "Byte") {
        uint8_t r;
        if (!dotnet::TryParseByte(value, r)) return false;
        out = r;
        return true;
    }
    if (t == "UInt16") {
        uint16_t r;
        if (!dotnet::TryParseUInt16(value, r)) return false;
        out = r;
        return true;
    }
    if (t == "Char") {  // char.TryParse: exactly one UTF-16 unit
        const std::u16string u = dotnet::ToUtf16(value);
        if (u.size() != 1) return false;
        out = u[0];
        return true;
    }
    if (t == "UInt64") {
        uint64_t r;
        if (!dotnet::TryParseUInt64(value, r)) return false;
        out = r;
        return true;
    }
    if (t == "Int32") {
        int32_t r;
        if (!dotnet::TryParseInt32(value, r)) return false;
        out = r;
        return true;
    }
    if (t == "Int64") {
        int64_t r;
        if (!dotnet::TryParseInt64(value, r)) return false;
        out = r;
        return true;
    }
    if (t == "Single") {
        float r;
        if (!dotnet::TryParseSingle(value, r)) return false;
        out = r;
        return true;
    }
    if (t == "UInt32") {
        uint32_t r;
        if (!dotnet::TryParseUInt32(value, r)) return false;
        out = r;
        return true;
    }
    if (t == "Int16") {
        int16_t r;
        if (!dotnet::TryParseInt16(value, r)) return false;
        out = r;
        return true;
    }
    if (t == "Double") {
        double r;
        if (!dotnet::TryParseDouble(value, r)) return false;
        out = r;
        return true;
    }
    try {
        if (t == "Vector2") {
            const auto a = SplitVector(value);
            if (a.size() != 2) return false;
            out = Vector2f{cs::FloatParse(a[0]), cs::FloatParse(a[1])};
            return true;
        }
        if (t == "Vector3") {
            const auto a = SplitVector(value);
            if (a.size() != 3) return false;
            out = Vector3f{cs::FloatParse(a[0]), cs::FloatParse(a[1]), cs::FloatParse(a[2])};
            return true;
        }
        if (t == "Vector4") {
            const auto a = SplitVector(value);
            if (a.size() != 4) return false;
            out = Vector4f{cs::FloatParse(a[0]), cs::FloatParse(a[1]), cs::FloatParse(a[2]), cs::FloatParse(a[3])};
            return true;
        }
        if (t == "Quaternion") {
            const auto a = SplitVector(value);
            if (a.size() != 4) return false;
            out = Quaternionf{cs::FloatParse(a[0]), cs::FloatParse(a[1]), cs::FloatParse(a[2]), cs::FloatParse(a[3])};
            return true;
        }
        if (info.enumTable) {
            out = EnumValue{info.enumTable->Parse(value), info.enumTable};
            return true;
        }
    } catch (const std::exception& e) {
        log::Error(std::string("Cannot parse ") + info.typeName + " value. Field " + info.name + ", value " +
                   std::string(value) + ": " + e.what());
        return false;
    }
    if (t == "String") {
        out = std::string(value);
        return true;
    }
    return false;  // other types (Color, reference types) are not read by ReadPvt
}

// Propellant.cs:62
void Propellant::Load(const ConfigNode& node) {
    const auto n = node.GetValue("name");
    if (!n) throw NullReferenceException("Object reference not set to an instance of an object");
    name = *n;
    id = MonoStringHash(name);
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    if (const PartResourceDefinition* def = lib ? lib->GetDefinition(id) : nullptr) {
        if (dotnet::ToUtf16(def->displayName()).size() > 14 && def->displayName() != "Electric Charge")
            displayName = def->abbreviation();
        else
            displayName = def->displayName();
    }
    if (displayName.empty()) displayName = name;
    node.TryGetValue("ratio", ratio);
    node.TryGetValue("minResToLeave", minResToLeave);
    node.TryGetValue("ignoreForIsp", ignoreForIsp);
    node.TryGetValue("ignoreForThrustCurve", ignoreForThrustCurve);
    node.TryGetValue("DrawGauge", drawStackGauge);
    if (node.HasValue("resourceFlowMode")) {
        flowMode = static_cast<int32_t>(kEnum_ResourceFlowMode.Parse(*node.GetValue("resourceFlowMode")));
        fMode = flowMode;
    }
}

// Propellant.cs:114
int32_t Propellant::GetFlowMode() {
    if (flowMode == 8) flowMode = PartResourceLibrary::GetDefaultFlowMode(id);
    return flowMode;
}

// Propellant.cs:58
void Propellant::UpdateConnectedResources(Part& p) {
    p.GetConnectedResourceTotals(id, static_cast<ResourceFlowMode>(GetFlowMode()), actualTotalAvailable, totalResourceCapacity);
}

namespace {

// ModuleEngines.cs:598 OnLoad / ModuleRCS.cs:615 SetupPropellant (PROPELLANT nodes)
void LoadPropellants(PartModuleData& m, const ConfigNode& node) {
    if (node.HasNode("PROPELLANT")) m.propellants.clear();
    for (ConfigNode* n : node.nodes().GetNodes()) {
        if (n->name != "PROPELLANT") continue;
        if (!n->HasValue("name")) {
            log::Info("Propellant must have value 'name'");
            continue;
        }
        Propellant p;
        p.Load(*n);
        m.propellants.push_back(std::move(p));
    }
    // ModuleEngines.SetupPropellant / ModuleRCS.SetupPropellant call GetFlowMode on every propellant whose resource is
    // defined (it caches the default flow mode).
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    for (Propellant& p : m.propellants)
        if (lib && lib->GetDefinition(MonoStringHash(p.name))) p.GetFlowMode();
}

std::string DefaultString(const ModuleClassInfo& cls, std::string_view field) {
    const ModuleFieldInfo* f = cls.FindField(field);
    if (!f) return {};
    std::string init = f->initializer;
    if (init.size() >= 2 && init.front() == '"' && init.back() == '"') return init.substr(1, init.size() - 2);
    return {};
}

}  // namespace

// PartModule.cs:787 Load + BaseFieldList.cs Load (WP-06 host) + the propellant parts of ModuleEngines/ModuleRCS OnLoad
void LoadModuleData(PartModuleData& m, ConfigNode& node, PartPrefab* part) {
    struct CopyOnExit {
        PartModuleData& m;
        ConfigNode& n;
        ~CopyOnExit() { m.config = n.CreateCopy(); }
    } copyOnExit{m, node};
    if (!m.moduleClass) {
        m.isEnabled = node.HasValue("isEnabled") ? cs::BoolParse(*node.GetValue("isEnabled")) : true;
        return;
    }
    if (!m.module) {  // prefab-overlay modules come with the Unity prefab: AddComponent + Awake
        m.module = ModuleFactory::Create(*m.moduleClass);
        m.module->Awake();
    }
    if (part) m.module->part = part;
    m.cfgFields.clear();
    m.module->Load(node);
    m.isEnabled = m.module->isEnabled;
    m.cfgFields = m.module->LastLoadTrace();
    // Propellants (ModuleEngines.OnLoad; ModuleRCS.OnLoad synthesises one PROPELLANT from resourceName /
    // resourceFlowMode when the module has none)
    if (m.moduleClass->Is("ModuleEngines")) {
        LoadPropellants(m, node);
    } else if (m.moduleClass->Is("ModuleRCS")) {
        if (!node.HasNode("PROPELLANT") && m.propellants.empty()) {
            std::string resourceName = DefaultString(*m.moduleClass, "resourceName");
            std::string resourceFlowMode = DefaultString(*m.moduleClass, "resourceFlowMode");
            if (const ModuleFieldValue* fv = m.FindCfgField("resourceName"))
                if (auto* s = std::get_if<std::string>(&fv->value)) resourceName = *s;
            if (const ModuleFieldValue* fv = m.FindCfgField("resourceFlowMode"))
                if (auto* s = std::get_if<std::string>(&fv->value)) resourceFlowMode = *s;
            ConfigNode* p = node.AddNode("PROPELLANT");
            p->AddValue("name", resourceName);
            p->AddValue("ratio", "1.0");
            p->AddValue("resourceFlowMode", resourceFlowMode);
        }
        LoadPropellants(m, node);
    }
}

}  // namespace ksp
