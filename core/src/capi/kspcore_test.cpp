// Test C ABI of kspcore (include/kspcore/kspcore_test.h): the physics scenario whose state hash the Unreal automation test
// KspUe5.Bridge.JoltStep compares with its ctest twin (capi.jolt_step), the MXCSR probe of the bridge's FP guard, and the
// Jolt configuration probe. Every entry point catches all C++ exceptions; nothing but C types crosses the boundary.
#include "kspcore/kspcore_test.h"

#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
#include <xmmintrin.h>
#define KSP_TEST_HAS_MXCSR 1
#endif

#include "ksp/physics/physics_world.hpp"

namespace {

using namespace ksp;
using namespace ksp::physics;

constexpr double kG = 9.80665;

// Same scenario as tests/test_physics_world.cpp RunPile (WP-20a): 96 boxes / spheres in a column grid with an LCG-seeded
// jitter, dropped on a static floor, gravity applied as a per-body force each step (KSP applies gravity as a force; the world
// gravity is zero).
uint64_t RunPile(int threads, int steps) {
    PhysicsWorldConfig cfg;
    cfg.threads = threads;
    PhysicsWorld w(cfg);
    {
        BodyDesc floor;
        floor.shape = ShapeDesc::Box(Vector3f(40, 0.5f, 40));
        floor.motion = MotionType::Static;
        floor.position = Vector3d(0, -0.5, 0);
        floor.mass = 10.0f;
        w.CreateBody(floor);
    }
    std::vector<BodyId> ids;
    uint32_t s = 12345u;
    auto rnd = [&s] {
        s = s * 1664525u + 1013904223u;
        return static_cast<double>(s >> 8) / 16777216.0;
    };
    for (int i = 0; i < 96; ++i) {
        const int ix = i % 8, iz = (i / 8) % 4, iy = i / 32;
        Vector3d p(ix * 1.3 - 4.5 + rnd() * 0.2, 1.0 + iy * 1.6 + rnd() * 0.3, iz * 1.3 - 2.0 + rnd() * 0.2);
        BodyDesc d;
        d.shape = (i % 3 == 0) ? ShapeDesc::Sphere(0.55f) : ShapeDesc::Box(Vector3f(0.5f, 0.4f, 0.5f));
        d.position = p;
        d.mass = 5.0f + static_cast<float>(i % 7);
        d.linearVelocity = Vector3d(rnd() - 0.5, 0, rnd() - 0.5);
        ids.push_back(w.CreateBody(d));
    }
    for (int k = 0; k < steps; ++k) {
        for (BodyId id : ids) w.AddForce(id, Vector3d(0, -kG * w.GetMass(id), 0));
        w.Step(0.02f);
    }
    return w.StateHash();
}

// What the linked Jolt actually is, by behaviour: a position single precision cannot hold survives a round trip only when
// JPH_DOUBLE_PRECISION is on.
bool DoublePrecision() {
    PhysicsWorld w;
    BodyDesc d;
    d.shape = ShapeDesc::Sphere(1.0f);
    d.motion = MotionType::Kinematic;
    d.position = Vector3d(600000.001, -3.5e-3, 12345678.125);
    const BodyId id = w.CreateBody(d);
    const Vector3d p = w.GetPosition(id);
    return p.x == 600000.001 && p.y == -3.5e-3 && p.z == 12345678.125;
}

}  // namespace

extern "C" {

int kspcore_test_abi_version(void) { return KSPCORE_TEST_ABI_VERSION; }

int kspcore_test_physics_scenario(int scenario, int threads, int steps, uint64_t* out_hash) {
    if (!out_hash || scenario != KSPCORE_TEST_SCENARIO_PILE || threads < 1 || threads > 64 || steps < 0 || steps > 100000)
        return 0;
    try {
        *out_hash = RunPile(threads, steps);
        return 1;
    } catch (...) {
        return 0;
    }
}

unsigned kspcore_test_mxcsr(void) {
#ifdef KSP_TEST_HAS_MXCSR
    return _mm_getcsr();
#else
    return 0;
#endif
}

const char* kspcore_test_jolt_config(void) {
    static const std::string cfg = [] {
        try {
            return std::string("double_precision=") + (DoublePrecision() ? "1" : "0") + "\n";
        } catch (...) {
            return std::string("error=1\n");
        }
    }();
    return cfg.c_str();
}

}  // extern "C"
