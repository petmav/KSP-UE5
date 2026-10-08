// Minimal replacement for UnityEngine.Debug.Log/LogWarning/LogError. KSP logs and continues; kspcore forwards the
// message to an optional sink (the host shell installs one) and otherwise drops it.
#pragma once

#include <functional>
#include <string_view>

namespace ksp::log {

enum class Level { Info, Warning, Error };

using Sink = std::function<void(Level, std::string_view)>;

// Install a sink (nullptr to silence). Not thread-safe; install once at startup.
void SetSink(Sink sink);

void Write(Level level, std::string_view message);
inline void Info(std::string_view m) { Write(Level::Info, m); }
inline void Warning(std::string_view m) { Write(Level::Warning, m); }
inline void Error(std::string_view m) { Write(Level::Error, m); }

}  // namespace ksp::log
