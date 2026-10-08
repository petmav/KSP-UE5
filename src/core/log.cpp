#include "ksp/core/log.hpp"

#include <utility>

namespace ksp::log {

namespace {
Sink& GlobalSink() {
    static Sink sink;
    return sink;
}
}  // namespace

void SetSink(Sink sink) { GlobalSink() = std::move(sink); }

void Write(Level level, std::string_view message) {
    if (auto& s = GlobalSink()) s(level, message);
}

}  // namespace ksp::log
