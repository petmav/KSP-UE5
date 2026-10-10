// WP-00: TimingManager.cs, Timing0..5 / TimingPre / TimingFI.cs and the PlayerLoop (see ksp/core/timing_manager.hpp).
#include "ksp/core/timing_manager.hpp"

#include <algorithm>
#include <exception>

#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"

namespace ksp {

using TimingStage = TimingManager::TimingStage;

// ---- Timing0.cs ... TimingFI.cs ------------------------------------------------------------------------------------

// Timing0.cs:12 Update (Timing1.cs ... TimingFI.cs:12 are the same with their own name)
void TimingManager::TimingPhase::Update() {
    try {
        if (!onUpdate.IsNull()) onUpdate();
    } catch (const std::exception& ex) {
        log::Error(name_ + " threw during Update: " + ex.what());
    }
}

// Timing0.cs:27 FixedUpdate
void TimingManager::TimingPhase::FixedUpdate() {
    try {
        if (!onFixedUpdate.IsNull()) onFixedUpdate();
    } catch (const std::exception& ex) {
        log::Error(name_ + " threw during FixedUpdate: " + ex.what());
    }
}

// Timing0.cs:42 LateUpdate
void TimingManager::TimingPhase::LateUpdate() {
    try {
        if (!onLateUpdate.IsNull()) onLateUpdate();
    } catch (const std::exception& ex) {
        log::Error(name_ + " threw during LateUpdate: " + ex.what());
    }
}

// ---- TimingManager.cs ----------------------------------------------------------------------------------------------

TimingManager::TimingManager() = default;

TimingManager::~TimingManager() {
    if (registered_ && instance_ == this) instance_ = nullptr;
}

// TimingManager.cs:45 Awake (AddComponent<TimingN>() is the construction of the member hosts)
bool TimingManager::Awake() {
    if (instance_ != nullptr) {
        log::Error("TimingManager: Instance already exists, destroying.");
        destroyed_ = true;  // UnityEngine.Object.Destroy(this)
        return false;
    }
    instance_ = this;
    registered_ = true;
    return true;
}

// TimingManager.cs:64 OnDestroy (the hosts are destroyed with the manager; delegates are dropped)
void TimingManager::OnDestroy() {
    for (TimingPhase* p : {&timing0_, &timing1_, &timing2_, &timing3_, &timing4_, &timing5_, &timingPre_, &timingFI_}) {
        p->onUpdate = nullptr;
        p->onFixedUpdate = nullptr;
        p->onLateUpdate = nullptr;
    }
    if (instance_ == this) instance_ = nullptr;
    registered_ = false;
    destroyed_ = true;
}

// TimingManager.cs:104 Update
void TimingManager::Update() {
    if (!onUpdate.IsNull()) onUpdate();
}

// TimingManager.cs:112 FixedUpdate
void TimingManager::FixedUpdate() {
    if (!onFixedUpdate.IsNull()) onFixedUpdate();
}

// TimingManager.cs:120 LateUpdate
void TimingManager::LateUpdate() {
    if (!onLateUpdate.IsNull()) onLateUpdate();
}

TimingManager::TimingPhase* TimingManager::Phase(TimingStage stage) {
    switch (stage) {
        case TimingStage::ObscenelyEarly: return &timing0_;
        case TimingStage::Early: return &timing1_;
        case TimingStage::Precalc: return &timingPre_;
        case TimingStage::Earlyish: return &timing2_;
        case TimingStage::Normal: return nullptr;
        case TimingStage::FashionablyLate: return &timing3_;
        case TimingStage::FlightIntegrator: return &timingFI_;
        case TimingStage::Late: return &timing4_;
        case TimingStage::BetterLateThanNever: return &timing5_;
    }
    return nullptr;
}

// The delegate field a stage's *Add / *Remove switch touches (TimingManager.cs:132-185 for update, :270-325 fixed, :408-462 late).
TimingManager::UpdateAction* TimingManager::Slot(TimingStage stage, Which which) {
    TimingManager* inst = instance_;
    if (inst == nullptr) return nullptr;
    if (stage == TimingStage::Normal) {
        switch (which) {
            case Which::Update: return &inst->onUpdate;
            case Which::Fixed: return &inst->onFixedUpdate;
            case Which::Late: return &inst->onLateUpdate;
        }
        return nullptr;
    }
    TimingPhase* p = inst->Phase(stage);
    if (p == nullptr) return nullptr;
    switch (which) {
        case Which::Update: return &p->onUpdate;
        case Which::Fixed: return &p->onFixedUpdate;
        case Which::Late: return &p->onLateUpdate;
    }
    return nullptr;
}

// TimingManager.cs:128 UpdateAdd (Delegate.Combine)
void TimingManager::UpdateAdd(TimingStage stage, const UpdateAction& action) {
    if (action.IsNull() || instance_ == nullptr) return;
    if (UpdateAction* s = Slot(stage, Which::Update)) *s = *s + action;
}

// TimingManager.cs:192 UpdateRemove (Delegate.Remove: last occurrence)
void TimingManager::UpdateRemove(TimingStage stage, const UpdateAction& action) {
    if (action.IsNull() || instance_ == nullptr) return;
    if (UpdateAction* s = Slot(stage, Which::Update)) {
        if (!s->IsNull()) *s = *s - action;
    }
}

// TimingManager.cs:266 FixedUpdateAdd
void TimingManager::FixedUpdateAdd(TimingStage stage, const UpdateAction& action) {
    if (action.IsNull() || instance_ == nullptr) return;
    if (UpdateAction* s = Slot(stage, Which::Fixed)) *s = *s + action;
}

// TimingManager.cs:330 FixedUpdateRemove
void TimingManager::FixedUpdateRemove(TimingStage stage, const UpdateAction& action) {
    if (action.IsNull() || instance_ == nullptr) return;
    if (UpdateAction* s = Slot(stage, Which::Fixed)) {
        if (!s->IsNull()) *s = *s - action;
    }
}

// TimingManager.cs:404 LateUpdateAdd
void TimingManager::LateUpdateAdd(TimingStage stage, const UpdateAction& action) {
    if (action.IsNull() || instance_ == nullptr) return;
    if (UpdateAction* s = Slot(stage, Which::Late)) *s = *s + action;
}

// TimingManager.cs:468 LateUpdateRemove
void TimingManager::LateUpdateRemove(TimingStage stage, const UpdateAction& action) {
    if (action.IsNull() || instance_ == nullptr) return;
    if (UpdateAction* s = Slot(stage, Which::Late)) {
        if (!s->IsNull()) *s = *s - action;
    }
}

// Measured order (analysis/physics/raw/run1/run1-L_calltrace.csv, rows `Timing0.FixedUpdate` seq 5 ... `Timing5.LateUpdate` seq 897).
const TimingManager::ScriptSlot (&TimingManager::ExecutionOrder())[9] {
    static const ScriptSlot kOrder[9] = {
        {"Timing0", TimingStage::ObscenelyEarly, 5, 310, 622},
        {"TimingPre", TimingStage::Precalc, 8, 314, 623},
        {"Timing1", TimingStage::Early, 10, 315, 624},
        {"Timing2", TimingStage::Earlyish, 13, 317, 625},
        {"TimingManager", TimingStage::Normal, 14, 325, 628},
        {"Timing3", TimingStage::FashionablyLate, 277, 575, 877},
        {"TimingFI", TimingStage::FlightIntegrator, 279, 577, 878},
        {"Timing4", TimingStage::Late, 283, 580, 881},
        {"Timing5", TimingStage::BetterLateThanNever, 286, 584, 897},
    };
    return kOrder;
}

// ---- PlayerLoop ----------------------------------------------------------------------------------------------------

int PlayerLoop::TimingOrderKey(TimingStage stage) {
    const auto& order = TimingManager::ExecutionOrder();
    for (int i = 0; i < 9; i++)
        if (order[i].stage == stage) return i * 100;
    return 0;
}

void PlayerLoop::AddTimingManager(TimingManager& tm) {
    const auto& order = TimingManager::ExecutionOrder();
    for (int i = 0; i < 9; i++) {
        const TimingStage st = order[i].stage;
        if (st == TimingStage::Normal) {
            AddScript(order[i].name, i * 100, Delegate<void()>::Bind(&tm, &TimingManager::Update),
                      Delegate<void()>::Bind(&tm, &TimingManager::FixedUpdate), Delegate<void()>::Bind(&tm, &TimingManager::LateUpdate));
        } else {
            TimingManager::TimingPhase* p = tm.Phase(st);
            AddScript(order[i].name, i * 100, Delegate<void()>::Bind(p, &TimingManager::TimingPhase::Update),
                      Delegate<void()>::Bind(p, &TimingManager::TimingPhase::FixedUpdate),
                      Delegate<void()>::Bind(p, &TimingManager::TimingPhase::LateUpdate));
        }
    }
}

void PlayerLoop::AddScript(std::string name, int order, Delegate<void()> update, Delegate<void()> fixedUpdate, Delegate<void()> lateUpdate) {
    ScriptBehaviour s;
    s.name = std::move(name);
    s.order = order;
    s.update = std::move(update);
    s.fixedUpdate = std::move(fixedUpdate);
    s.lateUpdate = std::move(lateUpdate);
    s.seq = nextSeq_++;
    scripts_.push_back(std::move(s));
    sorted_ = false;
}

void PlayerLoop::RemoveScript(const std::string& name) {
    scripts_.erase(std::remove_if(scripts_.begin(), scripts_.end(), [&](const ScriptBehaviour& s) { return s.name == name; }), scripts_.end());
}

void PlayerLoop::SortScripts() {
    if (sorted_) return;
    std::stable_sort(scripts_.begin(), scripts_.end(), [](const ScriptBehaviour& a, const ScriptBehaviour& b) {
        return a.order != b.order ? a.order < b.order : a.seq < b.seq;
    });
    sorted_ = true;
}

// One of the three ScriptRunBehaviour* slots: every registered script in execution order. An exception in a script is logged and
// the next script still runs (Unity logs and continues).
void PlayerLoop::RunScripts(Kind k) {
    SortScripts();
    const char* suffix = k == Kind::Update ? ".Update" : k == Kind::Fixed ? ".FixedUpdate" : ".LateUpdate";
    const std::vector<ScriptBehaviour> snapshot = scripts_;
    for (const ScriptBehaviour& s : snapshot) {
        const Delegate<void()>& d = k == Kind::Update ? s.update : k == Kind::Fixed ? s.fixedUpdate : s.lateUpdate;
        if (d.IsNull()) continue;
        if (recordTrace) trace.push_back(s.name + suffix);
        try {
            d();
        } catch (const std::exception& ex) {
            log::Error(s.name + suffix + " threw: " + ex.what());
        }
    }
}

void PlayerLoop::FixedStep() {
    const float savedTime = Time::time, savedDelta = Time::deltaTime;
    fixedTime_ += static_cast<double>(Time::fixedDeltaTime);
    if (writeTimeStatics) {
        Time::time = static_cast<float>(fixedTime_);
        Time::deltaTime = Time::fixedDeltaTime;
    }
    Mark("FixedUpdate.ScriptRunBehaviourFixedUpdate");
    RunScripts(Kind::Fixed);
    Mark("FixedUpdate.PhysicsFixedUpdate");
    if (!physicsStep.IsNull()) physicsStep();
    Mark("FixedUpdate.ScriptRunDelayedFixedFrameRate");
    scheduler_->RunDelayedFixedFrameRate();
    if (writeTimeStatics) {
        Time::time = savedTime;
        Time::deltaTime = savedDelta;
    }
}

void PlayerLoop::Frame(float deltaTime, int fixedSteps) {
    scheduler_->AdvanceFrame(deltaTime);
    frameDelta_ = deltaTime;
    if (writeTimeStatics) {
        Time::frameCount++;
        Time::time = static_cast<float>(scheduler_->CurTime());
        Time::deltaTime = deltaTime;
    }
    for (int i = 0; i < fixedSteps; i++) FixedStep();
    Mark("PreUpdate.PhysicsUpdate");
    Mark("Update.ScriptRunBehaviourUpdate");
    RunScripts(Kind::Update);
    Mark("Update.ScriptRunDelayedDynamicFrameRate");
    scheduler_->RunDelayedDynamicFrameRate();
    Mark("PreLateUpdate.ScriptRunBehaviourLateUpdate");
    RunScripts(Kind::Late);
    Mark("PostLateUpdate.ScriptRunDelayedDynamicFrameRate");  // marker only (coroutine.hpp)
    Mark("PostLateUpdate.TriggerEndOfFrameCallbacks");
    scheduler_->RunEndOfFrame();
}

int PlayerLoop::Frame(float deltaTime) {
    const double cur = scheduler_->CurTime() + static_cast<double>(deltaTime);
    const double step = static_cast<double>(Time::fixedDeltaTime);
    int n = 0;
    if (step > 0.0) {
        double ft = fixedTime_;
        while (ft + step <= cur && n < 1000) {
            ft += step;
            n++;
        }
    }
    Frame(deltaTime, n);
    return n;
}

}  // namespace ksp
