// WP-00: TimingManager.cs + Timing0 / Timing1 / Timing2 / Timing3 / Timing4 / Timing5 / TimingPre / TimingFI.cs (the phase hosts),
// and the PlayerLoop that runs them in Unity's measured order.
//
// TimingManager is a MonoBehaviour that, in Awake, adds eight sibling MonoBehaviours (Timing0 ... TimingFI) to its GameObject. Each of
// the nine (the eight hosts and the manager itself) has onUpdate / onFixedUpdate / onLateUpdate multicast delegates; KSP code
// subscribes with TimingManager.UpdateAdd / FixedUpdateAdd / LateUpdateAdd(stage, action). WHICH host a stage maps to is in the C# (the
// switch in each *Add / *Remove); WHEN the hosts run relative to each other is Unity's script execution order, which is project
// data and not in the C# (there is no [DefaultExecutionOrder] on any of them). It was measured in game: one-frame call trace
// analysis/physics/raw/run1/run1-L_calltrace.csv (rows cited below as `trace seq`). All three of FixedUpdate / Update / LateUpdate use
// the same order:
//
//   Timing0 (ObscenelyEarly) -> TimingPre (Precalc) -> Timing1 (Early) -> Timing2 (Earlyish) -> TimingManager (Normal)
//   -> Timing3 (FashionablyLate) -> TimingFI (FlightIntegrator) -> Timing4 (Late) -> Timing5 (BetterLateThanNever)
//
// NOTE the measured order puts Precalc BEFORE Early although the TimingStage enum lists Early before Precalc (TimingManager.cs:5-15);
// the measurement wins. Other MonoBehaviours (Planetarium, OrbitDriver, FlightGlobals, Part, ...) are interleaved by the same script
// execution order table in the real trace; PlayerLoop::AddScript lets a host place such a script by order key.
//
// Delegate semantics (add / remove during iteration) come from ksp::Delegate (core/delegate.hpp): Combine appends, Remove drops the
// LAST occurrence, and invoking runs a snapshot, so a callback that adds or removes listeners during a phase does not change that
// invocation's list; an exception escaping a callback skips the remaining entries of that delegate.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/core/delegate.hpp"

namespace ksp {

class TimingManager {
public:
    // TimingManager.cs:5
    enum class TimingStage {
        ObscenelyEarly,
        Early,
        Precalc,
        Earlyish,
        Normal,
        FashionablyLate,
        FlightIntegrator,
        Late,
        BetterLateThanNever
    };
    using UpdateAction = Delegate<void()>;  // TimingManager.cs:19 delegate void UpdateAction()

    // Timing0.cs ... TimingFI.cs: identical classes differing only in the name used in the error text.
    class TimingPhase {
    public:
        explicit TimingPhase(std::string name) : name_(std::move(name)) {}
        UpdateAction onUpdate;       // Timing0.cs:6
        UpdateAction onFixedUpdate;  // Timing0.cs:8
        UpdateAction onLateUpdate;   // Timing0.cs:10
        void Update();               // Timing0.cs:12
        void FixedUpdate();          // Timing0.cs:27
        void LateUpdate();           // Timing0.cs:42
        const std::string& Name() const { return name_; }
    private:
        std::string name_;
    };

    TimingManager();
    ~TimingManager();
    TimingManager(const TimingManager&) = delete;
    TimingManager& operator=(const TimingManager&) = delete;

    // TimingManager.cs:45 Awake. Returns false when an Instance already exists (the second manager destroys itself).
    bool Awake();
    // TimingManager.cs:64 OnDestroy
    void OnDestroy();
    bool IsDestroyed() const { return destroyed_; }
    // TimingManager.cs:37 (protected static Instance)
    static TimingManager* Instance() { return instance_; }

    UpdateAction onUpdate;        // TimingManager.cs:39
    UpdateAction onFixedUpdate;   // TimingManager.cs:41
    UpdateAction onLateUpdate;    // TimingManager.cs:43
    void Update();                // TimingManager.cs:104
    void FixedUpdate();           // TimingManager.cs:112
    void LateUpdate();            // TimingManager.cs:120

    // TimingManager.cs:128 / 192 / 266 / 330 / 404 / 468. No-ops when action is null or there is no Instance.
    static void UpdateAdd(TimingStage stage, const UpdateAction& action);
    static void UpdateRemove(TimingStage stage, const UpdateAction& action);
    static void FixedUpdateAdd(TimingStage stage, const UpdateAction& action);
    static void FixedUpdateRemove(TimingStage stage, const UpdateAction& action);
    static void LateUpdateAdd(TimingStage stage, const UpdateAction& action);
    static void LateUpdateRemove(TimingStage stage, const UpdateAction& action);

    // The host a stage maps to (nullptr for Normal: the manager's own delegates, and for values outside the enum).
    TimingPhase* Phase(TimingStage stage);

    // Measured script execution order of the nine components (relative order only; see the header comment). Index = position
    // in the run order; `traceSeq` is the row of run1-L_calltrace.csv for the FixedUpdate callback of that component.
    struct ScriptSlot { const char* name; TimingStage stage; int traceSeqFixed; int traceSeqUpdate; int traceSeqLate; };
    static const ScriptSlot (&ExecutionOrder())[9];

private:
    enum class Which { Update, Fixed, Late };
    static UpdateAction* Slot(TimingStage stage, Which which);

    TimingPhase timing0_{"Timing0"}, timing1_{"Timing1"}, timing2_{"Timing2"}, timing3_{"Timing3"};
    TimingPhase timing4_{"Timing4"}, timing5_{"Timing5"}, timingPre_{"TimingPre"}, timingFI_{"TimingFI"};
    bool registered_ = false;  // this object is the Instance
    bool destroyed_ = false;
    static inline TimingManager* instance_ = nullptr;
};

// One MonoBehaviour registered with the player loop at a script execution order key.
struct ScriptBehaviour {
    std::string name;
    int order = 0;
    Delegate<void()> update, fixedUpdate, lateUpdate;
    uint64_t seq = 0;  // registration order; ties in `order` run in registration order
};

// The part of Unity's player loop that matters to KSP scripts, in the order measured in game
// (analysis/physics/raw/run1/run1-L_calltrace.csv; marker names are the player-loop slots of that file):
//
//   one fixed step:  FixedUpdate.ScriptRunBehaviourFixedUpdate (scripts' FixedUpdate in script order)
//                    -> FixedUpdate.PhysicsFixedUpdate (physicsStep seam) -> FixedUpdate.ScriptRunDelayedFixedFrameRate (WaitForFixedUpdate)
//   one frame:       [fixed steps] -> PreUpdate.PhysicsUpdate (marker)
//                    -> Update.ScriptRunBehaviourUpdate (Update of scripts) -> Update.ScriptRunDelayedDynamicFrameRate (null, WaitForSeconds)
//                    -> PreLateUpdate.ScriptRunBehaviourLateUpdate (LateUpdate of scripts)
//                    -> PostLateUpdate.ScriptRunDelayedDynamicFrameRate (marker only, see coroutine.hpp)
//                    -> PostLateUpdate.TriggerEndOfFrameCallbacks (WaitForEndOfFrame)
//
// Time plumbing (Unity's rules): inside a fixed step Time.time = fixedTime and Time.deltaTime = Time.fixedDeltaTime; in Update /
// LateUpdate Time.time = the frame time and Time.deltaTime = the frame's delta (restored after the fixed steps). fixedTime and the
// frame time are doubles that accumulate the float deltas; the statics in core/unity_time.hpp get their float casts.
class PlayerLoop {
public:
    explicit PlayerLoop(CoroutineScheduler& scheduler) : scheduler_(&scheduler) {}

    // Registers the nine timing components of `tm` at their measured positions (orders 0, 100, ... 800).
    void AddTimingManager(TimingManager& tm);
    // Registers another script; `order` is relative to the timing components' keys (see TimingOrderKey).
    void AddScript(std::string name, int order, Delegate<void()> update, Delegate<void()> fixedUpdate, Delegate<void()> lateUpdate);
    void RemoveScript(const std::string& name);   // removes every script with that name
    // Order key AddTimingManager gave a stage's component (for placing a script between two of them).
    static int TimingOrderKey(TimingManager::TimingStage stage);

    Delegate<void()> physicsStep;  // seam: FixedUpdate.PhysicsFixedUpdate (the engine's physics step)

    void FixedStep();                              // one FixedUpdate iteration
    void Frame(float deltaTime, int fixedSteps);   // one frame with an explicit number of fixed steps
    // Frame() with Unity's step count: fixed steps run while fixedTime + fixedDeltaTime <= frame time.
    int Frame(float deltaTime);

    bool recordTrace = false;
    std::vector<std::string> trace;  // "Timing0.FixedUpdate", "FixedUpdate.PhysicsFixedUpdate", ... when recordTrace is set

    double FrameTime() const { return scheduler_->CurTime(); }
    double FixedTime() const { return fixedTime_; }
    bool writeTimeStatics = true;    // maintain ksp::Time::time / deltaTime / frameCount

private:
    enum class Kind { Update, Fixed, Late };
    void Mark(const char* s) { if (recordTrace) trace.emplace_back(s); }
    void RunScripts(Kind k);
    void SortScripts();

    CoroutineScheduler* scheduler_;
    std::vector<ScriptBehaviour> scripts_;
    bool sorted_ = true;
    uint64_t nextSeq_ = 0;
    double fixedTime_ = 0.0;
    float frameDelta_ = 0.0f;
};

}  // namespace ksp
