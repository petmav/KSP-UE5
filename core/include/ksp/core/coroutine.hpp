// WP-00: Unity coroutine scheduling model (UnityEngine.Coroutine / YieldInstruction / MonoBehaviour.StartCoroutine) for
// KSP's IEnumerator coroutines (CallbackUtil.DelayedCallback, VesselDeltaV.RunCalculations, ...).
//
// A C# iterator becomes a C++20 coroutine returning Enumerator; `yield return null` is `co_yield YieldInstruction::Null()`,
// `yield break` is `co_return`. The CoroutineScheduler replays Unity's three delayed-call passes. The pass order inside a
// frame is the measured player-loop order (analysis/physics/raw/run1/run1-L_calltrace.csv, see timing_manager.hpp):
//     FixedUpdate.ScriptRunBehaviourFixedUpdate -> PhysicsFixedUpdate -> FixedUpdate.ScriptRunDelayedFixedFrameRate
//     Update.ScriptRunBehaviourUpdate -> Update.ScriptRunDelayedDynamicFrameRate -> PreLateUpdate.ScriptRunBehaviourLateUpdate
//     ... -> PostLateUpdate.TriggerEndOfFrameCallbacks
// Modelled semantics (the engine source is not available; each point is an assumption, none is gated by a capture):
//   * StartCoroutine runs the routine synchronously up to its first yield (Unity does).
//   * yield null: resumes in the first RunDelayedDynamicFrameRate() pass of a LATER frame than the one it yielded in.
//   * WaitForSeconds(s): wake time = curTime + (double)s on the scheduler clock (the engine's scaled frame time, a double
//     accumulating the float Time.deltaTime of every frame, AdvanceFrame); resumes in the first dynamic pass with curTime >= wake.
//   * WaitForFixedUpdate: resumes in the next RunDelayedFixedFrameRate() (after PhysicsFixedUpdate); a routine that yields it
//     during a FixedUpdate callback resumes in the same step's pass.
//   * WaitForEndOfFrame: resumes in RunEndOfFrame() of the frame it yielded in (or the next one if yielded inside that pass).
//   * WaitUntil / WaitWhile: the predicate is evaluated in each dynamic pass.
//   * yield return StartCoroutine(child): the parent is resumed immediately when the child finishes (inside the child's
//     final resume); a child that already finished does not block.
//   * A pass works on a snapshot: routines that yield during a pass wait for the next pass of that kind.
//   * An exception escaping a routine is logged (Debug.LogError) and ends that routine; StopCoroutine of a routine that is
//     currently running only marks it, it finishes its current step.
// All time values are the caller's: nothing here reads a wall clock.
#pragma once

#include <coroutine>
#include <cstdint>
#include <exception>
#include <functional>
#include <list>
#include <memory>
#include <utility>
#include <vector>

namespace ksp {

struct Coroutine;

// UnityEngine.YieldInstruction family, as the value a routine yields.
struct YieldInstruction {
    enum class Kind { Null, WaitForFixedUpdate, WaitForEndOfFrame, WaitForSeconds, WaitUntil, WaitWhile, WaitForCoroutine };
    Kind kind = Kind::Null;
    float seconds = 0.0f;                   // WaitForSeconds
    std::function<bool()> predicate;        // WaitUntil: resume when true; WaitWhile: resume when false
    std::shared_ptr<Coroutine> coroutine;   // yield return StartCoroutine(...)

    static YieldInstruction Null() { return {}; }
    static YieldInstruction WaitForFixedUpdate() { YieldInstruction y; y.kind = Kind::WaitForFixedUpdate; return y; }
    static YieldInstruction WaitForEndOfFrame() { YieldInstruction y; y.kind = Kind::WaitForEndOfFrame; return y; }
    static YieldInstruction WaitForSeconds(float s) { YieldInstruction y; y.kind = Kind::WaitForSeconds; y.seconds = s; return y; }
    static YieldInstruction WaitUntil(std::function<bool()> p) { YieldInstruction y; y.kind = Kind::WaitUntil; y.predicate = std::move(p); return y; }
    static YieldInstruction WaitWhile(std::function<bool()> p) { YieldInstruction y; y.kind = Kind::WaitWhile; y.predicate = std::move(p); return y; }
    static YieldInstruction Wait(std::shared_ptr<Coroutine> c) { YieldInstruction y; y.kind = Kind::WaitForCoroutine; y.coroutine = std::move(c); return y; }
};

// System.Collections.IEnumerator produced by a C# iterator method (C++20 coroutine).
class Enumerator {
public:
    struct promise_type {
        YieldInstruction current;
        std::exception_ptr error;
        Enumerator get_return_object() { return Enumerator(std::coroutine_handle<promise_type>::from_promise(*this)); }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(YieldInstruction y) { current = std::move(y); return {}; }
        void return_void() {}
        void unhandled_exception() { error = std::current_exception(); }
    };
    using Handle = std::coroutine_handle<promise_type>;

    Enumerator() = default;
    explicit Enumerator(Handle h) : h_(h) {}
    Enumerator(Enumerator&& o) noexcept : h_(std::exchange(o.h_, nullptr)) {}
    Enumerator& operator=(Enumerator&& o) noexcept {
        if (this != &o) { Destroy(); h_ = std::exchange(o.h_, nullptr); }
        return *this;
    }
    Enumerator(const Enumerator&) = delete;
    Enumerator& operator=(const Enumerator&) = delete;
    ~Enumerator() { Destroy(); }

    // IEnumerator.MoveNext: runs to the next yield; false when the routine ended (an escaping exception is rethrown).
    bool MoveNext() {
        if (!h_ || h_.done()) return false;
        h_.resume();
        if (h_.promise().error) {
            std::exception_ptr e = std::exchange(h_.promise().error, nullptr);
            std::rethrow_exception(e);
        }
        return !h_.done();
    }
    const YieldInstruction& Current() const { return h_.promise().current; }
    bool Valid() const { return static_cast<bool>(h_); }

private:
    void Destroy() { if (h_) { h_.destroy(); h_ = nullptr; } }
    Handle h_{};
};

// UnityEngine.Coroutine: the handle StartCoroutine returns.
struct Coroutine {
    Enumerator routine;
    YieldInstruction wait;
    double wakeTime = 0.0;                  // WaitForSeconds: scheduler clock value at which it is due
    int64_t yieldFrame = -1;                // frame the routine last yielded in
    int owner = 0;                          // MonoBehaviour that started it (StopAllCoroutines scope)
    bool finished = false;
    bool stopped = false;
    bool running = false;
    std::shared_ptr<Coroutine> waiter;      // parent blocked on this routine (yield return StartCoroutine(child))
    bool Finished() const { return finished; }
};

// Unity's DelayedCallManager. One instance = one player; MonoBehaviours pass their owner id.
class CoroutineScheduler {
public:
    // MonoBehaviour.StartCoroutine: runs `routine` to its first yield now.
    std::shared_ptr<Coroutine> StartCoroutine(Enumerator routine, int owner = 0);
    void StopCoroutine(const std::shared_ptr<Coroutine>& c);
    void StopAllCoroutines(int owner);
    // The scheduler clock for the next frame: curTime += (double)deltaTime, frame++ (called once per frame, before the
    // fixed steps and Update of that frame; see PlayerLoop::Frame).
    void AdvanceFrame(float deltaTime);

    // The three passes (documented order above).
    void RunDelayedFixedFrameRate();     // FixedUpdate.ScriptRunDelayedFixedFrameRate (WaitForFixedUpdate)
    void RunDelayedDynamicFrameRate();   // Update.ScriptRunDelayedDynamicFrameRate (null, WaitForSeconds, WaitUntil/While)
    void RunEndOfFrame();                // PostLateUpdate.TriggerEndOfFrameCallbacks (WaitForEndOfFrame)

    double CurTime() const { return curTime_; }
    int64_t Frame() const { return frame_; }
    size_t Pending() const { return queue_.size(); }

private:
    enum class Pass { Fixed, Dynamic, EndOfFrame };
    void Resume(std::shared_ptr<Coroutine> c);
    void Finish(const std::shared_ptr<Coroutine>& c);
    bool Due(const Coroutine& c, Pass pass) const;
    void RunPass(Pass pass);
    void Unqueue(const std::shared_ptr<Coroutine>& c);

    std::list<std::shared_ptr<Coroutine>> live_;   // every started routine that has not finished or been stopped
    std::list<std::shared_ptr<Coroutine>> queue_;  // waiting routines, in the order they yielded
    double curTime_ = 0.0;
    int64_t frame_ = 0;
};

}  // namespace ksp
