#include "ksp/modules/module_members.hpp"

#include <map>
#include <set>

namespace ksp {

namespace {

struct Registry {
    std::vector<ClassMembers> classes;
    std::map<std::string, size_t> byFull;  // "name|fullName" -> index
    std::map<std::string, UiClassInfo> ui;
    EnumTable uiScene{"UI_Scene", false, "UI_Scene.cs", 1, {}};
};

Registry* g_registry = nullptr;

Registry& Reg() {
    if (g_registry) return *g_registry;
    static Registry r;
    ClassMembers* cur = nullptr;
    auto cls = [&](const char* name, const char* full) -> ClassMembers& {
        const std::string key = std::string(name) + "|" + full;
        auto it = r.byFull.find(key);
        if (it == r.byFull.end()) {
            r.classes.push_back(ClassMembers{name, full, {}, {}, {}});
            it = r.byFull.emplace(key, r.classes.size() - 1).first;
        }
        cur = &r.classes[it->second];
        return *cur;
    };
    (void)cur;
#define KSP_MEMBER_FIELD(cn_, full_, fld_, isAxis_, args_) \
    cls(cn_, full_).fields.push_back(FieldMemberInfo{fld_, isAxis_, args_, {}});
#define KSP_MEMBER_UI(cn_, full_, fld_, uiType_, args_)                \
    {                                                                  \
        auto& fl_ = cls(cn_, full_).fields;                            \
        for (auto& f_ : fl_)                                           \
            if (f_.field == fld_) f_.ui.push_back({uiType_, args_});   \
    }
#define KSP_MEMBER_EVENT(cn_, full_, method_, hasDetails_, isVirtual_, args_, line_) \
    cls(cn_, full_).events.push_back(EventMemberInfo{method_, hasDetails_, isVirtual_, args_, line_});
#define KSP_MEMBER_ACTION(cn_, full_, method_, isVirtual_, args_, line_) \
    cls(cn_, full_).actions.push_back(ActionMemberInfo{method_, isVirtual_, args_, line_});
#define KSP_UI_CLASS(name_, base_, file_, line_) r.ui[name_] = UiClassInfo{name_, base_, file_, line_, {}};
#define KSP_UI_FIELD(cn_, fld_, csType_, init_) r.ui[cn_].fields.push_back(UiClassField{fld_, csType_, init_});
#define KSP_MEMBER_ENUM(en_, isFlags_, file_, line_)
#define KSP_MEMBER_ENUM_VALUE(en_, member_, value_) r.uiScene.members.emplace_back(member_, value_);
#include "ksp/modules/module_members.inc"
#undef KSP_MEMBER_FIELD
#undef KSP_MEMBER_UI
#undef KSP_MEMBER_EVENT
#undef KSP_MEMBER_ACTION
#undef KSP_UI_CLASS
#undef KSP_UI_FIELD
#undef KSP_MEMBER_ENUM
#undef KSP_MEMBER_ENUM_VALUE
    g_registry = &r;
    return r;
}

}  // namespace

const ClassMembers* FindClassMembers(const ModuleClassInfo& cls) {
    Registry& r = Reg();
    auto it = r.byFull.find(std::string(cls.name) + "|" + cls.fullName);
    return it == r.byFull.end() ? nullptr : &r.classes[it->second];
}

const FieldMemberInfo* FindFieldMember(const ModuleFieldInfo& info) {
    if (!info.owner) return nullptr;
    const ClassMembers* m = FindClassMembers(*info.owner);
    if (!m) return nullptr;
    for (const FieldMemberInfo& f : m->fields)
        if (f.field == info.name) return &f;
    return nullptr;
}

const UiClassInfo* FindUiClass(std::string_view name) {
    Registry& r = Reg();
    auto it = r.ui.find(std::string(name));
    return it == r.ui.end() ? nullptr : &it->second;
}

const EnumTable& UiSceneEnum() { return Reg().uiScene; }

std::vector<const EventMemberInfo*> ReflectedEvents(const ModuleClassInfo& cls) {
    std::vector<const EventMemberInfo*> out;
    std::set<std::string> seen;  // a derived declaration hides the base's (GetMethods + FlattenHierarchy)
    for (const ModuleClassInfo* c = &cls; c; c = c->base) {
        const ClassMembers* m = FindClassMembers(*c);
        if (!m) continue;
        std::set<std::string> declared;
        for (const EventMemberInfo& e : m->events) {
            if (seen.count(e.method)) continue;
            // [KSPEvent] is AllowMultiple, but BaseEventList.Add logs "already defined" for a second event of a name
            if (declared.insert(e.method).second) out.push_back(&e);
        }
        for (const std::string& d : declared) seen.insert(d);
    }
    return out;
}

std::vector<const ActionMemberInfo*> ReflectedActions(const ModuleClassInfo& cls) {
    std::vector<const ActionMemberInfo*> out;
    std::set<std::string> seen;
    for (const ModuleClassInfo* c = &cls; c; c = c->base) {
        const ClassMembers* m = FindClassMembers(*c);
        if (!m) continue;
        std::set<std::string> declared;
        for (const ActionMemberInfo& a : m->actions) {
            if (seen.count(a.method)) continue;
            out.push_back(&a);  // BaseActionList keeps every attribute instance
            declared.insert(a.method);
        }
        for (const std::string& d : declared) seen.insert(d);
    }
    return out;
}

}  // namespace ksp
