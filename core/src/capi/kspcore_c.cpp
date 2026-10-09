// C ABI of kspcore (include/kspcore/kspcore_c.h). Every entry point catches all C++ exceptions; nothing but C types
// crosses the boundary.
#include "kspcore/kspcore_c.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <string>

#include "ksp/config/config_node.hpp"
#include "ksp/core/log.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/dotnet/math.hpp"

struct kspcore_sim {
    int reserved = 0;  // will own the game state as work packages land
};

struct kspcore_confignode {
    std::shared_ptr<ksp::ConfigNode> node;
};

struct kspcore_floatcurve {
    ksp::FloatCurve curve;
};

namespace {

constexpr const char* kVersion = "0.1.0";

thread_local std::string t_lastError;

void SetError(const char* where, const char* what) {
    t_lastError = std::string(where) + ": " + (what ? what : "unknown error");
}

// Runs `f`, converting any exception into `fallback` + kspcore_last_error.
template <class F, class R>
R Guard(const char* where, R fallback, F&& f) noexcept {
    try {
        return f();
    } catch (const std::exception& e) {
        SetError(where, e.what());
    } catch (...) {
        SetError(where, "non-standard exception");
    }
    return fallback;
}

kspcore_confignode* Wrap(std::shared_ptr<ksp::ConfigNode> n) {
    if (!n) return nullptr;
    return new kspcore_confignode{std::move(n)};
}

size_t CopyOut(const std::string& s, char* buffer, size_t capacity) {
    if (buffer && capacity > 0) {
        const size_t n = s.size() < capacity - 1 ? s.size() : capacity - 1;
        std::memcpy(buffer, s.data(), n);
        buffer[n] = '\0';
    }
    return s.size();
}

std::string MakeBuildInfo() {
    char b[512];
#if defined(_MSC_VER)
    const int msc = _MSC_VER;
    const long lang = static_cast<long>(_MSVC_LANG);
    const int idl = _ITERATOR_DEBUG_LEVEL;
#else
    const int msc = 0;
    const long lang = static_cast<long>(__cplusplus);
    const int idl = 0;
#endif
#if !defined(_MSC_VER)
    const char* crt = "host";  // glibc/libc++ etc.: only MSVC builds have the MD/MT distinction the Unreal link requires
#elif defined(_DLL) && defined(_DEBUG)
    const char* crt = "MDd";
#elif defined(_DLL)
    const char* crt = "MD";
#elif defined(_DEBUG)
    const char* crt = "MTd";
#else
    const char* crt = "MT";
#endif
    const char* runtime = ksp::dotnet::GetRuntime() == ksp::dotnet::Runtime::Mono ? "Mono" : "NetFramework";
    std::snprintf(b, sizeof b, "kspcore abi=%d version=%s msc=%d crt=%s idl=%d lang=%ld runtime=%s libm=%s",
                  KSPCORE_ABI_VERSION, kVersion, msc, crt, idl, lang, runtime, ksp::dotnet::LibmBackend());
    return b;
}

}  // namespace

extern "C" {

const char* kspcore_build_info(void) {
    static const std::string info = Guard("kspcore_build_info", std::string("kspcore abi=1 (build info unavailable)"),
                                          [] { return MakeBuildInfo(); });
    return info.c_str();
}

const char* kspcore_version(void) { return kVersion; }
int kspcore_abi_version(void) { return KSPCORE_ABI_VERSION; }
const char* kspcore_last_error(void) { return t_lastError.c_str(); }

kspcore_sim* kspcore_sim_create(void) {
    return Guard("kspcore_sim_create", static_cast<kspcore_sim*>(nullptr), [] { return new kspcore_sim(); });
}

void kspcore_sim_destroy(kspcore_sim* sim) { delete sim; }

void kspcore_set_log_callback(kspcore_log_fn fn, void* user) {
    Guard("kspcore_set_log_callback", 0, [&] {
        if (!fn) {
            ksp::log::SetSink(nullptr);
        } else {
            ksp::log::SetSink([fn, user](ksp::log::Level level, std::string_view message) {
                const std::string m(message);
                fn(level == ksp::log::Level::Info ? KSPCORE_LOG_INFO
                                                  : (level == ksp::log::Level::Warning ? KSPCORE_LOG_WARNING : KSPCORE_LOG_ERROR),
                   m.c_str(), user);
            });
        }
        return 0;
    });
}

// ---- ConfigNode ------------------------------------------------------------------------------------------------------

kspcore_confignode* kspcore_confignode_load(const char* utf8_path) {
    return Guard("kspcore_confignode_load", static_cast<kspcore_confignode*>(nullptr), [&] {
        if (!utf8_path) throw std::invalid_argument("path is NULL");
        auto n = ksp::ConfigNode::Load(utf8_path);
        if (!n) t_lastError = std::string("kspcore_confignode_load: no data in '") + utf8_path + "'";
        return Wrap(std::move(n));
    });
}

kspcore_confignode* kspcore_confignode_parse(const char* utf8_text) {
    return Guard("kspcore_confignode_parse", static_cast<kspcore_confignode*>(nullptr), [&] {
        if (!utf8_text) throw std::invalid_argument("text is NULL");
        return Wrap(ksp::ConfigNode::Parse(utf8_text));
    });
}

kspcore_confignode* kspcore_confignode_create(const char* name) {
    return Guard("kspcore_confignode_create", static_cast<kspcore_confignode*>(nullptr),
                 [&] { return Wrap(ksp::ConfigNode::Create(name ? name : "")); });
}

void kspcore_confignode_release(kspcore_confignode* node) { delete node; }

const char* kspcore_confignode_name(const kspcore_confignode* node) { return node ? node->node->name.c_str() : nullptr; }
const char* kspcore_confignode_id(const kspcore_confignode* node) { return node ? node->node->id.c_str() : nullptr; }

int kspcore_confignode_value_count(const kspcore_confignode* node) { return node ? node->node->values().Count() : 0; }

const char* kspcore_confignode_value_name(const kspcore_confignode* node, int index) {
    if (!node || index < 0 || index >= node->node->values().Count()) return nullptr;
    return node->node->values()[index].name.c_str();
}

const char* kspcore_confignode_value_at(const kspcore_confignode* node, int index) {
    if (!node || index < 0 || index >= node->node->values().Count()) return nullptr;
    return node->node->values()[index].value.c_str();
}

const char* kspcore_confignode_get_value(const kspcore_confignode* node, const char* name, int index) {
    if (!node || !name) return nullptr;
    const std::string* v = node->node->GetValuePtr(name, index);
    return v ? v->c_str() : nullptr;
}

int kspcore_confignode_set_value(kspcore_confignode* node, const char* name, const char* value, int create_if_not_found) {
    return Guard("kspcore_confignode_set_value", 0, [&] {
        if (!node || !name || !value) throw std::invalid_argument("NULL argument");
        return node->node->SetValue(std::string_view(name), std::string_view(value), create_if_not_found != 0) ? 1 : 0;
    });
}

void kspcore_confignode_add_value(kspcore_confignode* node, const char* name, const char* value) {
    Guard("kspcore_confignode_add_value", 0, [&] {
        if (!node || !name || !value) throw std::invalid_argument("NULL argument");
        node->node->AddValue(std::string_view(name), std::string_view(value));
        return 0;
    });
}

int kspcore_confignode_node_count(const kspcore_confignode* node) { return node ? node->node->nodes().Count() : 0; }

kspcore_confignode* kspcore_confignode_child_at(const kspcore_confignode* node, int index) {
    return Guard("kspcore_confignode_child_at", static_cast<kspcore_confignode*>(nullptr), [&]() -> kspcore_confignode* {
        if (!node || index < 0 || index >= node->node->nodes().Count()) return nullptr;
        return Wrap(node->node->nodes().Shared(index));
    });
}

kspcore_confignode* kspcore_confignode_get_node(const kspcore_confignode* node, const char* name, int index) {
    return Guard("kspcore_confignode_get_node", static_cast<kspcore_confignode*>(nullptr), [&]() -> kspcore_confignode* {
        if (!node || !name) return nullptr;
        int seen = 0;
        const auto& list = node->node->nodes();
        for (int i = 0; i < list.Count(); ++i) {
            if (list[i]->name == name) {
                if (seen == index) return Wrap(list.Shared(i));
                ++seen;
            }
        }
        return nullptr;
    });
}

kspcore_confignode* kspcore_confignode_add_node(kspcore_confignode* node, const char* name) {
    return Guard("kspcore_confignode_add_node", static_cast<kspcore_confignode*>(nullptr), [&]() -> kspcore_confignode* {
        if (!node || !name) return nullptr;
        ksp::ConfigNode* added = node->node->AddNode(std::string_view(name));
        if (!added) return nullptr;
        const auto& list = node->node->nodes();
        return Wrap(list.Shared(list.Count() - 1));
    });
}

int kspcore_confignode_try_get_double(const kspcore_confignode* node, const char* name, double* out) {
    return Guard("kspcore_confignode_try_get_double", 0, [&] {
        double v = 0;
        if (!node || !name || !out || !node->node->TryGetValue(name, v)) return 0;
        *out = v;
        return 1;
    });
}

int kspcore_confignode_try_get_float(const kspcore_confignode* node, const char* name, float* out) {
    return Guard("kspcore_confignode_try_get_float", 0, [&] {
        float v = 0;
        if (!node || !name || !out || !node->node->TryGetValue(name, v)) return 0;
        *out = v;
        return 1;
    });
}

int kspcore_confignode_try_get_int(const kspcore_confignode* node, const char* name, int* out) {
    return Guard("kspcore_confignode_try_get_int", 0, [&] {
        int32_t v = 0;
        if (!node || !name || !out || !node->node->TryGetValue(name, v)) return 0;
        *out = v;
        return 1;
    });
}

int kspcore_confignode_try_get_bool(const kspcore_confignode* node, const char* name, int* out) {
    return Guard("kspcore_confignode_try_get_bool", 0, [&] {
        bool v = false;
        if (!node || !name || !out || !node->node->TryGetValue(name, v)) return 0;
        *out = v ? 1 : 0;
        return 1;
    });
}

int kspcore_confignode_try_get_vector3d(const kspcore_confignode* node, const char* name, double out_xyz[3]) {
    return Guard("kspcore_confignode_try_get_vector3d", 0, [&] {
        ksp::Vector3d v;
        if (!node || !name || !out_xyz || !node->node->TryGetValue(name, v)) return 0;
        out_xyz[0] = v.x;
        out_xyz[1] = v.y;
        out_xyz[2] = v.z;
        return 1;
    });
}

size_t kspcore_confignode_save_to_buffer(const kspcore_confignode* node, char* buffer, size_t capacity) {
    return Guard("kspcore_confignode_save_to_buffer", static_cast<size_t>(0), [&] {
        if (!node) return static_cast<size_t>(0);
        return CopyOut(node->node->SaveToString(), buffer, capacity);
    });
}

int kspcore_confignode_save(const kspcore_confignode* node, const char* utf8_path) {
    return Guard("kspcore_confignode_save", 0, [&] {
        if (!node || !utf8_path) throw std::invalid_argument("NULL argument");
        if (!node->node->Save(utf8_path)) throw std::runtime_error(std::string("cannot write '") + utf8_path + "'");
        return 1;
    });
}

size_t kspcore_confignode_to_string(const kspcore_confignode* node, char* buffer, size_t capacity) {
    return Guard("kspcore_confignode_to_string", static_cast<size_t>(0), [&] {
        if (!node) return static_cast<size_t>(0);
        return CopyOut(node->node->ToString(), buffer, capacity);
    });
}

// ---- FloatCurve ------------------------------------------------------------------------------------------------------

kspcore_floatcurve* kspcore_floatcurve_create(void) {
    return Guard("kspcore_floatcurve_create", static_cast<kspcore_floatcurve*>(nullptr), [] { return new kspcore_floatcurve(); });
}

kspcore_floatcurve* kspcore_floatcurve_load(const kspcore_confignode* node) {
    return Guard("kspcore_floatcurve_load", static_cast<kspcore_floatcurve*>(nullptr), [&]() -> kspcore_floatcurve* {
        if (!node) throw std::invalid_argument("node is NULL");
        auto c = std::make_unique<kspcore_floatcurve>();
        c->curve.Load(*node->node);
        return c.release();
    });
}

void kspcore_floatcurve_destroy(kspcore_floatcurve* curve) { delete curve; }

void kspcore_floatcurve_add(kspcore_floatcurve* curve, float time, float value) {
    Guard("kspcore_floatcurve_add", 0, [&] {
        if (curve) curve->curve.Add(time, value);
        return 0;
    });
}

void kspcore_floatcurve_add_tangents(kspcore_floatcurve* curve, float time, float value, float in_tangent,
                                     float out_tangent) {
    Guard("kspcore_floatcurve_add_tangents", 0, [&] {
        if (curve) curve->curve.Add(time, value, in_tangent, out_tangent);
        return 0;
    });
}

float kspcore_floatcurve_evaluate(const kspcore_floatcurve* curve, float time) {
    return Guard("kspcore_floatcurve_evaluate", std::numeric_limits<float>::quiet_NaN(), [&] {
        if (!curve) throw std::invalid_argument("curve is NULL");
        return curve->curve.Evaluate(time);
    });
}

int kspcore_floatcurve_key_count(const kspcore_floatcurve* curve) { return curve ? curve->curve.Curve().length() : 0; }
float kspcore_floatcurve_min_time(const kspcore_floatcurve* curve) { return curve ? curve->curve.minTime() : 0.0f; }
float kspcore_floatcurve_max_time(const kspcore_floatcurve* curve) { return curve ? curve->curve.maxTime() : 0.0f; }

}  // extern "C"
