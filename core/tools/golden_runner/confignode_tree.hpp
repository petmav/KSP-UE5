// ConfigNode -> JSON tree in the harness's trees/<id>.json layout (ConfigNodeGen.cs TreeJson).
#pragma once

#include "ksp/config/config_node.hpp"
#include "nlohmann/json.hpp"

nlohmann::ordered_json TreeJson(const ksp::ConfigNode& n);
