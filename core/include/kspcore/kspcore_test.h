/*
 * kspcore TEST C ABI -- entry points that exist only so the consumers' automation tests can prove the link: the Unreal bridge
 * (ue5/Plugins/KspCoreBridge, automation test KspUe5.Bridge.JoltStep / KspUe5.Bridge.FpGuard) and the core's own C consumer
 * (tests/capi/capi_jolt_step.c, ctest capi.jolt_step). Both sides call the same functions, so both compute the same value
 * from the same code; the tests then compare the two.
 *
 * Same rules as kspcore_c.h: plain C types only, no exception escapes, nothing numerically sensitive inline. This header is
 * not part of the product ABI (KSPCORE_ABI_VERSION): the version below changes independently.
 */
#pragma once

#include <stdint.h>

#define KSPCORE_TEST_ABI_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

/* KSPCORE_TEST_ABI_VERSION the library was built with. */
int kspcore_test_abi_version(void);

/* Scenario ids for kspcore_test_physics_scenario. */
#define KSPCORE_TEST_SCENARIO_PILE 0 /* 96 boxes / spheres dropped on a static floor under a per-body gravity force */

/* Steps one physics scenario (a ksp::physics::PhysicsWorld, Jolt v5.6.0, double precision) with a 0.02 s fixed step and
 * returns its state hash (FNV-1a 64 over every body's position, rotation and velocities, creation order) in *out_hash.
 * `threads` = total Jolt threads incl. the caller (the result must not depend on it). Returns 1 on success, 0 on failure
 * (unknown scenario, bad arguments, C++ exception; see kspcore_last_error). Jolt is deterministic per binary and call
 * order, so the hash is only comparable between processes that run the same kspcore.lib / Jolt.lib objects. */
int kspcore_test_physics_scenario(int scenario, int threads, int steps, uint64_t* out_hash);

/* The x86 MXCSR register as seen by code running inside kspcore.lib right now (0 on CPUs without one). Used to prove the
 * bridge's floating-point environment guard: core must run with 0x1F80 (round to nearest, exceptions masked, no FTZ / DAZ). */
unsigned kspcore_test_mxcsr(void);

/* Jolt configuration as observed from the outside, one "key=value" per line, e.g. "double_precision=1\n". Lets the consumer
 * compare what the linked Jolt actually is with the definitions in the link manifest. Static string, never freed. */
const char* kspcore_test_jolt_config(void);

#ifdef __cplusplus
}
#endif
