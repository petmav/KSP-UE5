// WP-UE-02: what the C ABI translation units (kspcore_c.cpp, kspcore_game.cpp, kspcore_frame.cpp) share. Not installed.
#pragma once

#include <exception>
#include <memory>
#include <string>

#include "kspcore/kspcore_c.h"
#include "ksp/host/game_host.hpp"

// the simulation context: owns the game host (kspcore_game.h / kspcore_frame.h); created empty by kspcore_sim_create
struct kspcore_sim {
    std::unique_ptr<ksp::host::GameHost> host;
};

namespace kspcore_capi {

inline std::string& LastError() {
    static thread_local std::string e;
    return e;
}

inline void SetError(const char* where, const char* what) { LastError() = std::string(where) + ": " + (what ? what : "unknown error"); }

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

}  // namespace kspcore_capi
