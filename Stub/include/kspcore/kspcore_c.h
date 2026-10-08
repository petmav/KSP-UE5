/*
 * kspcore C ABI consumed by ue5/Plugins/KspCoreBridge.
 *
 * This is the PLACEHOLDER (stub) copy. The real core (C:\Projects\KSP-UE5\core) is expected to ship a header
 * at the same include path (core/include/kspcore/kspcore_c.h) exporting at least the symbols below.
 * Only plain C types cross this boundary: Unreal compiles with /GR- and _HAS_EXCEPTIONS=0, the core does
 * not, so no STL types, exceptions or RTTI may cross it.
 */
#pragma once

#define KSPCORE_ABI_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

/* Static NUL-terminated string, never freed. Describes the build: ABI version, compiler, CRT flavour,
 * iterator debug level and C++ language level, so the bridge can verify it linked a compatible lib. */
const char* kspcore_build_info(void);

#ifdef __cplusplus
}
#endif
