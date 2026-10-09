// KerbalFSM: KSP's finite-state machine (EVA kerbals, ladders, docking, EVA construction, ...).
// Sources: Assembly-CSharp/KerbalFSM.cs, KFSMState.cs, KFSMEvent.cs, KFSMTimedEvent.cs, KFSMUpdateMode.cs,
// KFSMCallback.cs, KFSMEventCondition.cs, KFSMStateChange.cs.
//
// Behaviour reproduced (including quirks):
//  * updateFSM (KerbalFSM.cs:199) walks currentState.StateEvents by index and keeps the index across a transition: after
//    RunEvent switches state, the loop continues at i+1 of the NEW state's event list.
//  * RunEvent (KerbalFSM.cs:106): OnEventCalled(evt), evt.OnEvent(), lastEvent/lastEventName, then
//    currentStateName = GoToStateOnEvent.name (NullReferenceException when the event has no target state, after
//    lastEvent was already updated), OnLeave(next), state switch, time/frame stamps, OnStateChange(last, next, evt),
//    OnEnter(last). An event not in the current state's list only logs "Event X not assigned to state Y".
//  * StartFSM calls OnEnter(null) BEFORE stamping TimeAtStateEnter / FrameCountAtStateEnter.
//  * AddEvent(ev, states...) processes the states from the last to the first, adds ev to every registration of each
//    state, and throws "FSM Error: Cannot add events to state <name>. ..." at the first unregistered state (events
//    already added to later-listed states stay added). Duplicate registrations add the event twice.
//  * Time comes from ksp::Time (UnityEngine.Time.time is a float, widened to double).
//
// Ownership (C++ only): the FSM and its states hold non-owning pointers, like the C# references; the module that
// creates the states and events (e.g. KerbalEVA) owns them and must outlive the FSM's use of them.
#pragma once

#include <initializer_list>
#include <string>
#include <vector>

#include "ksp/core/delegate.hpp"

namespace ksp {

// KFSMUpdateMode.cs
enum class KFSMUpdateMode : int { MANUAL_TRIGGER = 0, UPDATE = 1, FIXEDUPDATE = 2, LATEUPDATE = 4 };

class KFSMState;
class KFSMEvent;

using KFSMCallback = Delegate<void()>;                       // KFSMCallback.cs
using KFSMEventCondition = Delegate<bool(KFSMState*)>;       // KFSMEventCondition.cs
using KFSMStateChange = Delegate<void(KFSMState*)>;          // KFSMStateChange.cs

namespace detail {
// Target of C#'s cached anonymous-method delegates (`delegate { }`: Roslyn's `<>c` singleton, never null).
struct AnonymousMethodClosure {};
inline AnonymousMethodClosure kAnonymousMethodClosure;
}  // namespace detail

// KFSMEvent.cs
class KFSMEvent {
public:
    std::string name;
    KFSMState* GoToStateOnEvent = nullptr;
    KFSMUpdateMode updateMode = KFSMUpdateMode::UPDATE;
    KFSMCallback OnEvent = KFSMCallback::Lambda(&detail::kAnonymousMethodClosure, [] {});
    KFSMEventCondition OnCheckCondition =
        KFSMEventCondition::Lambda(&detail::kAnonymousMethodClosure, [](KFSMState*) { return false; });

    explicit KFSMEvent(std::string name_) : name(std::move(name_)) {}
    virtual ~KFSMEvent() = default;

    // KFSMEvent.cs:20 (NullReferenceException for a null state)
    bool IsValid(const KFSMState* state) const;
};

// KFSMState.cs
class KFSMState {
public:
    std::string name;
    double TimeAtStateEnter = 0.0;
    int FrameCountAtStateEnter = 0;
    KFSMUpdateMode updateMode = KFSMUpdateMode::FIXEDUPDATE;
    KFSMStateChange OnEnter = KFSMStateChange::Lambda(&detail::kAnonymousMethodClosure, [](KFSMState*) {});
    KFSMCallback OnUpdate = KFSMCallback::Lambda(&detail::kAnonymousMethodClosure, [] {});
    KFSMCallback OnFixedUpdate = KFSMCallback::Lambda(&detail::kAnonymousMethodClosure, [] {});
    KFSMCallback OnLateUpdate = KFSMCallback::Lambda(&detail::kAnonymousMethodClosure, [] {});
    KFSMStateChange OnLeave = KFSMStateChange::Lambda(&detail::kAnonymousMethodClosure, [](KFSMState*) {});

    explicit KFSMState(std::string name_) : name(std::move(name_)) {}
    virtual ~KFSMState() = default;

    std::vector<KFSMEvent*>& StateEvents() { return stateEvents; }
    const std::vector<KFSMEvent*>& StateEvents() const { return stateEvents; }

    // KFSMState.cs:43 / 48 (List.Contains: reference equality)
    bool IsValid(const KFSMEvent* ev) const;
    void AddEvent(KFSMEvent* ev) { stateEvents.push_back(ev); }
    // KFSMState.cs:53
    std::string ToString() const { return name; }

protected:
    std::vector<KFSMEvent*> stateEvents;
};

// KFSMTimedEvent.cs: fires once (double)Time.time - currentState.TimeAtStateEnter > TimerDuration.
class KFSMTimedEvent : public KFSMEvent {
public:
    double TimerDuration;
    KFSMTimedEvent(std::string name_, double time);

private:
    bool WaitForTimer(KFSMState* currentState);
};

// KerbalFSM.cs
class KerbalFSM {
public:
    std::string currentStateName;
    std::string lastEventName;
    bool DebugBreakOnStateChange = false;  // Debug.Break() pauses the Unity editor: no-op here
    Callback<KFSMState*, KFSMState*, KFSMEvent*> OnStateChange =
        Callback<KFSMState*, KFSMState*, KFSMEvent*>::Lambda(&detail::kAnonymousMethodClosure,
                                                             [](KFSMState*, KFSMState*, KFSMEvent*) {});
    Callback<KFSMEvent*> OnEventCalled =
        Callback<KFSMEvent*>::Lambda(&detail::kAnonymousMethodClosure, [](KFSMEvent*) {});

    KerbalFSM() = default;
    virtual ~KerbalFSM() = default;

    KFSMState* CurrentState() const { return currentState; }
    KFSMEvent* LastEvent() const { return lastEvent; }
    KFSMState* LastState() const { return lastState; }
    bool Started() const { return fsmStarted; }
    // KerbalFSM.cs:42 (double)Time.time - CurrentState.TimeAtStateEnter
    double TimeAtCurrentState() const;
    // KerbalFSM.cs:44 Time.frameCount - CurrentState.FrameCountAtStateEnter
    int FramesInCurrentState() const;

    void AddState(KFSMState* st) { States.push_back(st); }
    void AddEvent(KFSMEvent* ev, std::initializer_list<KFSMState*> toStates) { AddEvent(ev, std::vector<KFSMState*>(toStates)); }
    void AddEvent(KFSMEvent* ev, const std::vector<KFSMState*>& toStates);
    void AddEventExcluding(KFSMEvent* ev, std::initializer_list<KFSMState*> excStates) {
        AddEventExcluding(ev, std::vector<KFSMState*>(excStates));
    }
    void AddEventExcluding(KFSMEvent* ev, const std::vector<KFSMState*>& excStates);
    void RunEvent(KFSMEvent* evt);
    void StartFSM(const std::string& initialStateName);
    void StartFSM(KFSMState* initialState);
    void UpdateFSM();
    void FixedUpdateFSM();
    void LateUpdateFSM();

    const std::vector<KFSMState*>& GetStates() const { return States; }  // kspcore addition (C#: protected field)

protected:
    void updateFSM(KFSMUpdateMode mode);

    std::vector<KFSMState*> States;
    KFSMState* currentState = nullptr;
    KFSMEvent* lastEvent = nullptr;
    KFSMState* lastState = nullptr;
    bool fsmStarted = false;

private:
    std::string lastCurrentState;
};

}  // namespace ksp
