// KSP's typed event bus: BaseGameEvent, EventVoid, EventData<T..>, EventDataModifier<T,U>, Event{Double,Float,Int}{Max,Min},
// EventValueComparison<T>, EventValueOperation<T>, EventValueWrapper, GameEventsBase.
// Sources: Assembly-CSharp/BaseGameEvent.cs, GameEventsBase.cs, EventVoid.cs, EventData.cs, EventDataModifier.cs,
// EventDoubleMax.cs, EventDoubleMin.cs, EventFloatMax.cs, EventFloatMin.cs, EventIntMax.cs, EventIntMin.cs,
// EventValueComparison.cs, EventValueOperation.cs, EventValueWrapper.cs.
//
// Semantics reproduced exactly (they are the same in every event class; line numbers are EventData.cs's first class):
//  * Add (EventData.cs:37): wraps the delegate with its Target and Target.GetType().Name; a static-method or null
//    delegate throws NullReferenceException (evt.Target.GetType()). Duplicates are allowed.
//  * Remove (EventData.cs:47): searches from the END for the last listener whose delegate == evt (C# delegate equality:
//    same target object and same method) and removes that one entry; absent -> no-op (debug log only).
//  * Fire (EventData.cs:68): copies the listener list into a per-event shared `eventsClone` and calls the copy from the
//    LAST registered listener to the FIRST (reverse registration order). Each call is wrapped in try/catch: an exception
//    is logged ("Exception handling event <name> in class <type>:" + exception, then LogException) and the remaining
//    listeners still run. Listeners added or removed during Fire do not change the current round (snapshot), BUT the
//    snapshot list is shared by nested Fire calls of the same event: a listener that fires the same event again
//    re-fills eventsClone from the current listener list, and the outer loop then continues indexing that new list
//    (it can skip or repeat listeners, and if the new list is shorter the outer indexer throws
//    ArgumentOutOfRangeException -> std::out_of_range, outside the try, so it escapes Fire). numEventsFiring counts
//    nesting; the snapshot is cleared when it drops to 0.
//  * The "originator == null" branch of Fire is unreachable (originator is an object reference that Add made sure is
//    non-null); it is ported for completeness.
//  * EventDataModifier: the constructor registers DefaultModifier (returns d1) first, so it runs LAST; each listener
//    receives the previous listener's result; a throwing listener leaves the value unchanged.
//  * EventValueComparison.Update / EventValueOperation.Update have NO try/catch: an exception escapes Update and leaves
//    numEventsFiring incremented (sic). Their debug log lines print an empty event name (C#: a `new string eventName`
//    property / `gameEventName` property that is never assigned hides the base field).
//
// Logging goes through ksp::log (Debug.Log -> Info, LogError/LogException -> Error). Exception texts are e.what()
// (C#: Exception.ToString(), which adds the type name and stack trace).
//
// Lifetime (C++ only): events register their address in the global name table (FindEvent) and unregister in their
// destructor; listeners hold raw target pointers, so an object must Remove its listeners before it is destroyed.
// Single-threaded, like KSP.
#pragma once

#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "ksp/core/delegate.hpp"
#include "ksp/core/log.hpp"

namespace ksp::events {

// GameEventsBase.cs: public static bool debugEvents (log every Add/Remove/Fire of every event)
class GameEventsBase {
public:
    static inline bool debugEvents = false;
};

// BaseGameEvent.cs
class BaseGameEvent {
public:
    // BaseGameEvent.cs:15: eventsByName[eventName] = this (a later event with the same name replaces the entry)
    explicit BaseGameEvent(std::string eventName);
    virtual ~BaseGameEvent();
    BaseGameEvent(const BaseGameEvent&) = delete;
    BaseGameEvent& operator=(const BaseGameEvent&) = delete;

    const std::string& EventName() const { return eventName_; }

    // public bool debugEvent
    bool debugEvent = false;

    // BaseGameEvent.cs:21 FindEvent<T>(eventName): nullptr when no event has that name; when the registered event is
    // not a T, logs "Type mismatch when searching for event with name ..." and returns nullptr.
    template <class T>
    static T* FindEvent(const std::string& eventName) {
        BaseGameEvent* value = Lookup(eventName);
        if (value == nullptr) return nullptr;
        T* result = dynamic_cast<T*>(value);
        if (result == nullptr) {
            LogTypeMismatch(eventName, detail::CsTypeName(typeid(T)), detail::CsTypeName(typeid(*value)));
            return nullptr;
        }
        return result;
    }

protected:
    bool DebugLogging() const { return debugEvent || GameEventsBase::debugEvents; }

    std::string eventName_;  // C#: protected string eventName

private:
    static BaseGameEvent* Lookup(const std::string& eventName);
    static void LogTypeMismatch(const std::string& eventName, const std::string& expected, const std::string& found);
};

namespace detail {

// The listener list shared by all event classes (EvtDelegate + events + eventsClone + numEventsFiring).
template <class Fn>
class EventListeners {
public:
    // private class EvtDelegate (a reference type: List.Remove(EvtDelegate) removes that exact object)
    struct EvtDelegate {
        Fn evt;
        const void* originator;
        std::string originatorType;
        explicit EvtDelegate(Fn e) : evt(std::move(e)), originator(evt.Target()), originatorType(evt.TargetTypeName()) {}
    };
    using Ptr = std::shared_ptr<EvtDelegate>;

    void Add(Fn evt, const std::string& logName, bool debug) {
        Ptr d = std::make_shared<EvtDelegate>(std::move(evt));
        if (debug) log::Info("EventManager: Adding event '" + logName + "' for object of type '" + d->originatorType + "'");
        events_.push_back(std::move(d));
    }

    void Remove(const Fn& evt, const std::string& logName, bool debug) {
        size_t count = events_.size();
        Ptr d;
        for (;;) {
            if (count-- > 0) {
                d = events_[count];
                if (d->evt == evt) break;
                continue;
            }
            if (debug) log::Info("EventManager: Removing event '" + logName + "' - Cannot find listed event");
            return;
        }
        if (debug) log::Info("EventManager: Removing event '" + logName + "' from object of type '" + d->originatorType + "'");
        RemoveFirst(events_, d);
    }

    // The Fire / Update loop. invoke(const EvtDelegate&) performs the call and combines the result.
    template <class Invoke>
    void Fire(const std::string& logName, bool debug, bool catchExceptions, Invoke&& invoke) {
        if (debug) log::Info("EventManager: Firing event '" + logName + "'");
        ++numEventsFiring_;
        eventsClone_.clear();
        eventsClone_.insert(eventsClone_.end(), events_.begin(), events_.end());
        int count = static_cast<int>(eventsClone_.size());
        while (count-- > 0) {
            if (eventsClone_.at(static_cast<size_t>(count))->originator != nullptr) {
                const Ptr d = eventsClone_[static_cast<size_t>(count)];  // keep alive across a nested Fire
                if (!catchExceptions) {
                    invoke(*d);
                    continue;
                }
                try {
                    invoke(*d);
                } catch (const std::exception& ex) {
                    LogListenerException(logName, eventsClone_.at(static_cast<size_t>(count))->originatorType, ex.what());
                } catch (...) {
                    LogListenerException(logName, eventsClone_.at(static_cast<size_t>(count))->originatorType,
                                         "unknown exception");
                }
            } else {
                log::Info("EventManager: Removing event '" + logName + "'for object of type '" +
                          events_.at(static_cast<size_t>(count))->originatorType + "' as object is null.");
                RemoveFirst(events_, eventsClone_[static_cast<size_t>(count)]);
                eventsClone_.erase(eventsClone_.begin() + count);
            }
        }
        if (--numEventsFiring_ <= 0) {
            eventsClone_.clear();
            numEventsFiring_ = 0;
        }
    }

    // kspcore additions for tests and diagnostics (no KSP equivalent)
    size_t Count() const { return events_.size(); }
    int NumEventsFiring() const { return numEventsFiring_; }

private:
    static void RemoveFirst(std::vector<Ptr>& list, const Ptr& item) {
        for (auto it = list.begin(); it != list.end(); ++it)
            if (*it == item) {
                list.erase(it);
                return;
            }
    }
    static void LogListenerException(const std::string& logName, const std::string& type, const char* what) {
        log::Error("Exception handling event " + logName + " in class " + type + ":" + what);
        log::Error(what);  // Debug.LogException(ex)
    }

    std::vector<Ptr> events_;
    std::vector<Ptr> eventsClone_;
    int numEventsFiring_ = 0;
};

}  // namespace detail

// EventVoid.cs
class EventVoid : public BaseGameEvent {
public:
    using OnEvent = Delegate<void()>;
    explicit EventVoid(std::string eventName) : BaseGameEvent(std::move(eventName)) {}

    void Add(OnEvent evt) { core_.Add(std::move(evt), eventName_, DebugLogging()); }
    template <class C, class M>
    void Add(C* obj, M method) { Add(OnEvent::Bind(obj, method)); }
    void Remove(const OnEvent& evt) { core_.Remove(evt, eventName_, DebugLogging()); }
    template <class C, class M>
    void Remove(C* obj, M method) { Remove(OnEvent::Bind(obj, method)); }
    void Fire() {
        core_.Fire(eventName_, DebugLogging(), true, [](const auto& d) { d.evt(); });
    }

    size_t Count() const { return core_.Count(); }  // kspcore addition

private:
    detail::EventListeners<OnEvent> core_;
};

// EventData.cs: EventData<T>, EventData<T, U>, EventData<T, U, V>, EventData<T, U, V, W>
template <class... Ts>
class EventData : public BaseGameEvent {
    static_assert(sizeof...(Ts) >= 1 && sizeof...(Ts) <= 4, "KSP defines EventData with 1 to 4 type arguments");

public:
    using OnEvent = Delegate<void(Ts...)>;
    explicit EventData(std::string eventName) : BaseGameEvent(std::move(eventName)) {}

    void Add(OnEvent evt) { core_.Add(std::move(evt), eventName_, DebugLogging()); }
    template <class C, class M>
    void Add(C* obj, M method) { Add(OnEvent::Bind(obj, method)); }
    void Remove(const OnEvent& evt) { core_.Remove(evt, eventName_, DebugLogging()); }
    template <class C, class M>
    void Remove(C* obj, M method) { Remove(OnEvent::Bind(obj, method)); }
    void Fire(Ts... data) {
        core_.Fire(eventName_, DebugLogging(), true, [&](const auto& d) { d.evt(data...); });
    }

    size_t Count() const { return core_.Count(); }  // kspcore addition

private:
    detail::EventListeners<OnEvent> core_;
};

// EventDataModifier.cs: listeners transform a value in turn (reverse registration order); DefaultModifier runs last.
template <class T, class U>
class EventDataModifier : public BaseGameEvent {
public:
    using OnEvent = Delegate<T(T, U)>;
    // EventDataModifier.cs:31
    explicit EventDataModifier(std::string eventName) : BaseGameEvent(std::move(eventName)) {
        Add(OnEvent::Bind(this, &EventDataModifier::DefaultModifier));
    }

    void Add(OnEvent evt) { core_.Add(std::move(evt), eventName_, DebugLogging()); }
    template <class C, class M>
    void Add(C* obj, M method) { Add(OnEvent::Bind(obj, method)); }
    void Remove(const OnEvent& evt) { core_.Remove(evt, eventName_, DebugLogging()); }
    template <class C, class M>
    void Remove(C* obj, M method) { Remove(OnEvent::Bind(obj, method)); }
    // EventDataModifier.cs:74
    T Fire(T d1, U d2) {
        T val = d1;
        core_.Fire(eventName_, DebugLogging(), true, [&](const auto& d) { val = d.evt(val, d2); });
        return val;
    }

    size_t Count() const { return core_.Count(); }  // kspcore addition

private:
    T DefaultModifier(T d1, U /*d2*/) { return d1; }  // EventDataModifier.cs:38

    detail::EventListeners<OnEvent> core_;
};

namespace detail {
// Math.Max/Min(double) (Mono mscorlib Math.cs:419/537: NaN in the first argument wins) for EventDouble*;
// Mathf.Max/Min(float/int) (a > b ? a : b) for EventFloat* / EventInt*.
template <class T, bool IsMax>
T ExtremumCombine(T a, T b) {
    if constexpr (std::is_same_v<T, double>) {
        if constexpr (IsMax) {
            if (a > b) return a;
        } else {
            if (a < b) return a;
        }
        if (std::isnan(a)) return a;
        return b;
    } else if constexpr (IsMax) {
        return (a > b) ? a : b;
    } else {
        return (a < b) ? a : b;
    }
}
}  // namespace detail

// EventDoubleMax.cs / EventDoubleMin.cs / EventFloatMax.cs / EventFloatMin.cs / EventIntMax.cs / EventIntMin.cs:
// Fire() folds the listeners' results into defaultValue with Max/Min (reverse registration order).
template <class T, bool IsMax>
class EventValueExtremum : public BaseGameEvent {
public:
    using OnEvent = Delegate<T()>;
    T defaultValue;

    explicit EventValueExtremum(std::string eventName, T defaultValue_ = T{})
        : BaseGameEvent(std::move(eventName)), defaultValue(defaultValue_) {}

    void Add(OnEvent evt) { core_.Add(std::move(evt), eventName_, DebugLogging()); }
    template <class C, class M>
    void Add(C* obj, M method) { Add(OnEvent::Bind(obj, method)); }
    void Remove(const OnEvent& evt) { core_.Remove(evt, eventName_, DebugLogging()); }
    template <class C, class M>
    void Remove(C* obj, M method) { Remove(OnEvent::Bind(obj, method)); }
    // EventDoubleMax.cs:75
    T Fire() {
        T num = defaultValue;
        core_.Fire(eventName_, DebugLogging(), true,
                   [&](const auto& d) { num = detail::ExtremumCombine<T, IsMax>(num, d.evt()); });
        return num;
    }

    size_t Count() const { return core_.Count(); }  // kspcore addition

private:
    detail::EventListeners<OnEvent> core_;
};

using EventDoubleMax = EventValueExtremum<double, true>;
using EventDoubleMin = EventValueExtremum<double, false>;
using EventFloatMax = EventValueExtremum<float, true>;
using EventFloatMin = EventValueExtremum<float, false>;
using EventIntMax = EventValueExtremum<int32_t, true>;
using EventIntMin = EventValueExtremum<int32_t, false>;

// EventValueComparison.cs: Update() sets value = defaultValue, then value = a for every listener result a with
// comparer(a, value) (reverse registration order). No exception handling.
template <class T>
class EventValueComparison : public BaseGameEvent {
public:
    using OnCompare = Delegate<bool(T, T)>;
    using OnEvent = Delegate<T()>;

    EventValueComparison(std::string eventName, T defaultValue, OnCompare comparer)
        : BaseGameEvent(std::move(eventName)), defaultValue_(defaultValue), value_(defaultValue),
          comparer_(std::move(comparer)) {}

    T value() const { return value_; }

    void Add(OnEvent evt) { core_.Add(std::move(evt), hiddenEventName_, DebugLogging()); }
    template <class C, class M>
    void Add(C* obj, M method) { Add(OnEvent::Bind(obj, method)); }
    void Remove(const OnEvent& evt) { core_.Remove(evt, hiddenEventName_, DebugLogging()); }
    template <class C, class M>
    void Remove(C* obj, M method) { Remove(OnEvent::Bind(obj, method)); }
    // EventValueComparison.cs:83
    void Update() {
        value_ = defaultValue_;
        core_.Fire(hiddenEventName_, DebugLogging(), false, [&](const auto& d) {
            T a = d.evt();
            if (comparer_(a, value_)) value_ = a;
        });
    }

    size_t Count() const { return core_.Count(); }      // kspcore addition
    int NumEventsFiring() const { return core_.NumEventsFiring(); }  // kspcore addition

private:
    T defaultValue_;
    T value_;
    OnCompare comparer_;
    std::string hiddenEventName_;  // `public new string eventName { get; private set; }` -- never assigned (sic)
    detail::EventListeners<OnEvent> core_;
};

// EventValueOperation.cs: Update() sets value = defaultValue, then value = operation(a, value) for every listener
// result a (reverse registration order). No exception handling.
template <class T>
class EventValueOperation : public BaseGameEvent {
public:
    using OnOperation = Delegate<T(T, T)>;
    using OnEvent = Delegate<T()>;

    EventValueOperation(std::string eventName, T defaultValue, OnOperation operation)
        : BaseGameEvent(std::move(eventName)), defaultValue_(defaultValue), value_(defaultValue),
          operation_(std::move(operation)) {}

    T value() const { return value_; }
    // public string gameEventName { get; private set; } -- never assigned (sic): empty
    const std::string& gameEventName() const { return gameEventName_; }

    void Add(OnEvent evt) { core_.Add(std::move(evt), gameEventName_, DebugLogging()); }
    template <class C, class M>
    void Add(C* obj, M method) { Add(OnEvent::Bind(obj, method)); }
    void Remove(const OnEvent& evt) { core_.Remove(evt, gameEventName_, DebugLogging()); }
    template <class C, class M>
    void Remove(C* obj, M method) { Remove(OnEvent::Bind(obj, method)); }
    // EventValueOperation.cs:83
    void Update() {
        value_ = defaultValue_;
        core_.Fire(gameEventName_, DebugLogging(), false, [&](const auto& d) {
            T a = d.evt();
            value_ = operation_(a, value_);
        });
    }

    size_t Count() const { return core_.Count(); }      // kspcore addition
    int NumEventsFiring() const { return core_.NumEventsFiring(); }  // kspcore addition

private:
    T defaultValue_;
    T value_;
    OnOperation operation_;
    std::string gameEventName_;
    detail::EventListeners<OnEvent> core_;
};

// EventValueWrapper.cs: the C# constructor reflects over the derived class's public fields (then properties) and
// collects every member type's parameterless Update() method; Update() calls them in that order. C++ has no
// reflection: a derived class registers its Update()-able members in declaration order (fields first, then
// properties), e.g. `RegisterUpdates(MaxThrottle, HeatProduction, ...)` in its constructor.
class EventValueWrapper {
public:
    virtual ~EventValueWrapper() = default;
    // EventValueWrapper.cs:55
    void Update() {
        const size_t num = updateActions_.size();
        for (size_t i = 0; i < num; ++i) updateActions_[i]();
    }

protected:
    template <class... Members>
    void RegisterUpdates(Members&... members) {
        (updateActions_.push_back([&m = members] { m.Update(); }), ...);
    }

private:
    std::vector<std::function<void()>> updateActions_;
};

}  // namespace ksp::events
