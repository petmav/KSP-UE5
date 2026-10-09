// The KSP game root the tests and the golden runner read (GameData/, Physics.cfg, PartDatabase.cfg).
// Default: core/testdata/GameData-mirror (tools/mk_gamedata_mirror.py: every cfg verbatim, zero-byte placeholders for
// everything else). Env KSP_GAMEDATA_ROOT points at a real install instead (e.g. decompiled/KSP-dev).
#pragma once

#include <cstdlib>
#include <string>

#ifndef KSPCORE_DEFAULT_GAME_ROOT
#error "KSPCORE_DEFAULT_GAME_ROOT must be defined by the build (testdata/GameData-mirror)"
#endif

namespace ksp::testing {
inline std::string GameRoot() {
    const char* e = std::getenv("KSP_GAMEDATA_ROOT");
    if (e && *e) return e;
    return KSPCORE_DEFAULT_GAME_ROOT;
}
}  // namespace ksp::testing
