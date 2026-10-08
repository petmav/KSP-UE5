// Port of Assembly-CSharp/KerbalFSM.cs, KFSMState.cs, KFSMEvent.cs, KFSMTimedEvent.cs.
#include "ksp/fsm/kerbal_fsm.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"

namespace ksp {

// KFSMEvent.cs:20 state.StateEvents.Contains(this)
bool KFSMEvent::IsValid(const KFSMState* state) const {
    if (state == nullptr) throw NullReferenceException();
    const auto& ev = state->StateEvents();
    return std::find(ev.begin(), ev.end(), this) != ev.end();
}

// KFSMState.cs:43
bool KFSMState::IsValid(const KFSMEvent* ev) const {
    return std::find(stateEvents.begin(), stateEvents.end(), ev) != stateEvents.end();
}

// KFSMTimedEvent.cs:7
KFSMTimedEvent::KFSMTimedEvent(std::string name_, double time) : KFSMEvent(std::move(name_)), TimerDuration(time) {
    OnCheckCondition = KFSMEventCondition::Bind(this, &KFSMTimedEvent::WaitForTimer);
}

// KFSMTimedEvent.cs:14
bool KFSMTimedEvent::WaitForTimer(KFSMState* currentState) {
    if (currentState == nullptr) throw NullReferenceException();
    return static_cast<double>(Time::time) - currentState->TimeAtStateEnter > TimerDuration;
}

// KerbalFSM.cs:42
double KerbalFSM::TimeAtCurrentState() const {
    if (currentState == nullptr) throw NullReferenceException();
    return static_cast<double>(Time::time) - currentState->TimeAtStateEnter;
}

// KerbalFSM.cs:44 (int subtraction wraps in C#)
int KerbalFSM::FramesInCurrentState() const {
    if (currentState == nullptr) throw NullReferenceException();
    return static_cast<int>(static_cast<unsigned>(Time::frameCount) -
                            static_cast<unsigned>(currentState->FrameCountAtStateEnter));
}

// KerbalFSM.cs:57
void KerbalFSM::AddEvent(KFSMEvent* ev, const std::vector<KFSMState*>& toStates) {
    size_t num = toStates.size();
    KFSMState* kFSMState;
    bool flag;
    do {
        if (num-- <= 0) return;
        kFSMState = toStates[num];
        flag = true;
        size_t count = States.size();
        while (count-- > 0) {
            if (States[count] == toStates[num]) {
                flag = false;
                if (kFSMState == nullptr) throw NullReferenceException();  // a null registered as a state (sic)
                kFSMState->AddEvent(ev);
            }
        }
    } while (!flag);
    // string.Concat(..., kFSMState, ...): a null state prints as ""
    throw Exception("FSM Error: Cannot add events to state " + (kFSMState ? kFSMState->ToString() : std::string()) +
                    ". It is not registered in this FSM (or is null)");
}

// KerbalFSM.cs:84
void KerbalFSM::AddEventExcluding(KFSMEvent* ev, const std::vector<KFSMState*>& excStates) {
    size_t count = States.size();
    while (count-- > 0) {
        bool flag = true;
        size_t num = excStates.size();
        while (num-- > 0) {
            if (excStates[num] == States[count]) {
                flag = false;
                break;
            }
        }
        if (flag) {
            if (States[count] == nullptr) throw NullReferenceException();
            States[count]->AddEvent(ev);
        }
    }
}

// KerbalFSM.cs:106
void KerbalFSM::RunEvent(KFSMEvent* evt) {
    if (!fsmStarted)
        throw Exception("FSM Error: Cannot switch states before starting. Please call StartFSM() before running the FSM.");
    if (currentState == nullptr) throw NullReferenceException();
    const auto& events = currentState->StateEvents();
    if (std::find(events.begin(), events.end(), evt) == events.end()) {
        if (evt == nullptr) throw NullReferenceException();  // evt.name
        log::Info("Event " + evt->name + " not assigned to state " + currentState->name);
        return;
    }
    OnEventCalled(evt);
    if (evt == nullptr) throw NullReferenceException();
    evt->OnEvent();
    KFSMState* goToStateOnEvent = evt->GoToStateOnEvent;
    lastEvent = evt;
    lastEventName = evt->name;
    if (goToStateOnEvent == nullptr) throw NullReferenceException();  // goToStateOnEvent.name
    currentStateName = goToStateOnEvent->name;
    currentState->OnLeave(goToStateOnEvent);
    lastState = currentState;
    currentState = goToStateOnEvent;
    currentState->TimeAtStateEnter = static_cast<double>(Time::time);
    currentState->FrameCountAtStateEnter = Time::frameCount;
    OnStateChange(lastState, goToStateOnEvent, evt);
    currentState->OnEnter(lastState);
    // if (DebugBreakOnStateChange) Debug.Break();  -- pauses the Unity editor; nothing to do here
}

// KerbalFSM.cs:136
void KerbalFSM::StartFSM(const std::string& initialStateName) {
    size_t num = 0;
    const size_t count = States.size();
    KFSMState* kFSMState;
    while (true) {
        if (num < count) {
            kFSMState = States[num];
            if (kFSMState == nullptr) throw NullReferenceException();  // kFSMState.name
            if (kFSMState->name == initialStateName) break;
            ++num;
            continue;
        }
        throw Exception("[KFSM ERROR]: Cannot Start. Could not find a state called " + initialStateName +
                        " in states list");
    }
    StartFSM(kFSMState);
}

// KerbalFSM.cs:158
void KerbalFSM::StartFSM(KFSMState* initialState) {
    currentState = initialState;
    if (currentState == nullptr) throw NullReferenceException();
    currentState->OnEnter(nullptr);
    currentState->TimeAtStateEnter = static_cast<double>(Time::time);
    currentState->FrameCountAtStateEnter = Time::frameCount;
    currentStateName = currentState->name;
    fsmStarted = true;
}

// KerbalFSM.cs:168
void KerbalFSM::UpdateFSM() {
    if (fsmStarted) {
        if (currentStateName != lastCurrentState) lastCurrentState = currentStateName;
        currentState->OnUpdate();
        updateFSM(KFSMUpdateMode::UPDATE);
    }
}

// KerbalFSM.cs:181
void KerbalFSM::FixedUpdateFSM() {
    if (fsmStarted) {
        currentState->OnFixedUpdate();
        updateFSM(KFSMUpdateMode::FIXEDUPDATE);
    }
}

// KerbalFSM.cs:190
void KerbalFSM::LateUpdateFSM() {
    if (fsmStarted) {
        currentState->OnLateUpdate();
        updateFSM(KFSMUpdateMode::LATEUPDATE);
    }
}

// KerbalFSM.cs:199 -- the index survives a state change (the loop continues on the new state's event list)
void KerbalFSM::updateFSM(KFSMUpdateMode mode) {
    for (size_t i = 0; i < currentState->StateEvents().size(); ++i) {
        KFSMEvent* kFSMEvent = currentState->StateEvents()[i];
        if (kFSMEvent == nullptr) {
            log::Warning("[KerbalFSM]: Null Event in current State '" + currentStateName + "' encountered.");
        } else if (kFSMEvent->updateMode == mode && kFSMEvent->OnCheckCondition(currentState)) {
            RunEvent(kFSMEvent);
        }
    }
}

}  // namespace ksp
