// WP-UE-02: internals shared by the host's translation units (not installed).
#pragma once

#include <string>

#include "ksp/database/game_data.hpp"
#include "ksp/host/game_host.hpp"
#include "ksp/terrain/pqs_prefab.hpp"

namespace ksp::host {

GameData& ProcessGameData();  // the process-level GameDatabase / PartLoader (GameProcess::LoadDatabase)

terrain::MapLibrary& ProcessMaps();  // Kerbin's data maps (<data_dir>/terrain), loaded once per process

// The file to read for a part model path: the path itself when it is a non-empty file (a real game root), else the reduced model the
// reference of reduced_model_dir lists for it (the repository's GameData mirror keeps zero-byte .mu stubs), else "".
std::string ProcessModelFile(const std::string& path);

// UTF-8 -> UTF-16 (Input.inputString is a C# string)
std::u16string Utf8ToUtf16(const char* s);

// the failure text of the last failed ABI call on this thread (kspcore_last_error)
void SetLastError(const std::string& what);

}  // namespace ksp::host
