// BaseGameEvent.cs: the global name -> event table behind GameEvents.FindEvent<T>.
#include <string>
#include <unordered_map>

#include "ksp/events/event_data.hpp"

namespace ksp::events {

namespace {
// private static Dictionary<string, BaseGameEvent> eventsByName (function-local: no static-initialisation order issue
// with the GameEvents registry, whose events are namespace-scope statics in another translation unit)
std::unordered_map<std::string, BaseGameEvent*>& EventsByName() {
    static std::unordered_map<std::string, BaseGameEvent*> map;
    return map;
}
}  // namespace

// BaseGameEvent.cs:15
BaseGameEvent::BaseGameEvent(std::string eventName) : eventName_(std::move(eventName)) {
    EventsByName()[eventName_] = this;
}

// C++ only: C# never removes an entry (the dictionary keeps the event alive); here a destroyed event is unregistered
// if it is still the entry for its name.
BaseGameEvent::~BaseGameEvent() {
    auto& map = EventsByName();
    auto it = map.find(eventName_);
    if (it != map.end() && it->second == this) map.erase(it);
}

BaseGameEvent* BaseGameEvent::Lookup(const std::string& eventName) {
    auto& map = EventsByName();
    auto it = map.find(eventName);
    return it == map.end() ? nullptr : it->second;
}

// BaseGameEvent.cs:30 (type names are C++ type names here, C#: typeof(T) / value.GetType())
void BaseGameEvent::LogTypeMismatch(const std::string& eventName, const std::string& expected, const std::string& found) {
    log::Error("Type mismatch when searching for event with name " + eventName + ".  Expected type " + expected +
               ", but found " + found);
}

}  // namespace ksp::events
