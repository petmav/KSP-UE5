// C ABI v2, "frame" domain (include/kspcore/kspcore_frame.h). Every entry point catches all C++ exceptions.
#include <cstring>
#include <stdexcept>
#include <string>

#include "capi_internal.hpp"
#include "ksp/host/game_host.hpp"
#include "kspcore/kspcore_frame.h"

namespace {

using kspcore_capi::Guard;
using kspcore_capi::SetError;

ksp::host::GameHost* Host(kspcore_sim* sim, const char* where) {
    if (sim == nullptr || !sim->host) {
        SetError(where, sim == nullptr ? "sim is NULL" : "no game (kspcore_game_new / kspcore_game_load first)");
        return nullptr;
    }
    return sim->host.get();
}

// the input struct as this library knows it, from a caller's older or newer one (struct_size rule)
kspcore_input_frame Normalised(const kspcore_input_frame* in) {
    kspcore_input_frame out{};
    out.struct_size = sizeof(kspcore_input_frame);
    const size_t n = in->struct_size < sizeof(kspcore_input_frame) ? in->struct_size : sizeof(kspcore_input_frame);
    std::memcpy(&out, in, n);
    out.struct_size = sizeof(kspcore_input_frame);
    return out;
}

}  // namespace

extern "C" {

int kspcore_input_set_frame(kspcore_sim* sim, const kspcore_input_frame* input) {
    return Guard("kspcore_input_set_frame", 0, [&]() -> int {
        ksp::host::GameHost* h = Host(sim, "kspcore_input_set_frame");
        if (h == nullptr) return 0;
        if (input == nullptr) throw std::invalid_argument("input is NULL");
        h->SetInput(Normalised(input));
        return 1;
    });
}

size_t kspcore_commands_submit(kspcore_sim* sim, const kspcore_command* commands, size_t count) {
    return Guard("kspcore_commands_submit", static_cast<size_t>(0), [&]() -> size_t {
        ksp::host::GameHost* h = Host(sim, "kspcore_commands_submit");
        return h != nullptr ? h->Submit(commands, count) : 0;
    });
}

int kspcore_frame(kspcore_sim* sim, double dt_real) {
    return Guard("kspcore_frame", 0, [&]() -> int {
        ksp::host::GameHost* h = Host(sim, "kspcore_frame");
        if (h == nullptr) return 0;
        if (h->Frame(dt_real)) return 1;
        SetError("kspcore_frame", h->Error().c_str());
        return 0;
    });
}

const kspcore_snapshot_view* kspcore_snapshot(kspcore_sim* sim) {
    return Guard("kspcore_snapshot", static_cast<const kspcore_snapshot_view*>(nullptr), [&]() -> const kspcore_snapshot_view* {
        ksp::host::GameHost* h = Host(sim, "kspcore_snapshot");
        return h != nullptr ? &h->Snapshot() : nullptr;
    });
}

uint64_t kspcore_snapshot_hash(const kspcore_snapshot_view* snapshot) {
    return Guard("kspcore_snapshot_hash", static_cast<uint64_t>(0), [&]() -> uint64_t {
        if (snapshot == nullptr) throw std::invalid_argument("snapshot is NULL");
        return ksp::host::SnapshotHash(*snapshot);
    });
}

}  // extern "C"
