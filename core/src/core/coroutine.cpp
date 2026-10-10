// WP-00: CoroutineScheduler (see ksp/core/coroutine.hpp for the modelled semantics) and CoroutineHost / CallbackUtil.
#include "ksp/core/coroutine.hpp"

#include <algorithm>
#include <string>

#include "ksp/core/coroutine_host.hpp"
#include "ksp/core/log.hpp"

namespace ksp {

namespace {
int g_nextOwnerId = 1;
}

std::shared_ptr<Coroutine> CoroutineScheduler::StartCoroutine(Enumerator routine, int owner) {
    auto c = std::make_shared<Coroutine>();
    c->routine = std::move(routine);
    c->owner = owner;
    live_.push_back(c);
    Resume(c);  // Unity runs the routine up to its first yield inside StartCoroutine
    return c;
}

void CoroutineScheduler::StopCoroutine(const std::shared_ptr<Coroutine>& c) {
    if (!c || c->finished) return;
    c->stopped = true;
    c->waiter.reset();
    Unqueue(c);
    live_.remove(c);
    if (!c->running) {
        c->finished = true;
        c->routine = Enumerator();  // release the routine's frame; a running one is released when its step ends
    }
}

void CoroutineScheduler::StopAllCoroutines(int owner) {
    std::vector<std::shared_ptr<Coroutine>> mine;
    for (const auto& c : live_)
        if (c->owner == owner) mine.push_back(c);
    for (const auto& c : mine) StopCoroutine(c);
}

void CoroutineScheduler::AdvanceFrame(float deltaTime) {
    curTime_ += static_cast<double>(deltaTime);
    frame_++;
}

void CoroutineScheduler::RunDelayedFixedFrameRate() { RunPass(Pass::Fixed); }
void CoroutineScheduler::RunDelayedDynamicFrameRate() { RunPass(Pass::Dynamic); }
void CoroutineScheduler::RunEndOfFrame() { RunPass(Pass::EndOfFrame); }

void CoroutineScheduler::Unqueue(const std::shared_ptr<Coroutine>& c) { queue_.remove(c); }

bool CoroutineScheduler::Due(const Coroutine& c, Pass pass) const {
    const YieldInstruction& y = c.wait;
    switch (y.kind) {
        case YieldInstruction::Kind::WaitForFixedUpdate: return pass == Pass::Fixed;
        case YieldInstruction::Kind::WaitForEndOfFrame: return pass == Pass::EndOfFrame;
        case YieldInstruction::Kind::Null: return pass == Pass::Dynamic && frame_ > c.yieldFrame;
        case YieldInstruction::Kind::WaitForSeconds: return pass == Pass::Dynamic && curTime_ >= c.wakeTime;
        case YieldInstruction::Kind::WaitUntil: return pass == Pass::Dynamic && y.predicate && y.predicate();
        case YieldInstruction::Kind::WaitWhile: return pass == Pass::Dynamic && !(y.predicate && y.predicate());
        case YieldInstruction::Kind::WaitForCoroutine: return false;  // resumed by Finish() of the child
    }
    return false;
}

void CoroutineScheduler::RunPass(Pass pass) {
    const std::vector<std::shared_ptr<Coroutine>> batch(queue_.begin(), queue_.end());
    for (const auto& c : batch) {
        if (c->stopped || c->finished) continue;
        if (std::find(queue_.begin(), queue_.end(), c) == queue_.end()) continue;
        if (!Due(*c, pass)) continue;
        Unqueue(c);
        Resume(c);
    }
}

void CoroutineScheduler::Resume(std::shared_ptr<Coroutine> c) {
    for (;;) {
        if (c->stopped || c->finished) return;
        bool more = false;
        c->running = true;
        try {
            more = c->routine.MoveNext();
        } catch (const std::exception& e) {
            log::Error(std::string("Coroutine threw: ") + e.what());
        } catch (...) {
            log::Error("Coroutine threw an unknown exception");
        }
        c->running = false;
        if (c->stopped) {
            c->finished = true;
            c->routine = Enumerator();
            return;
        }
        if (!more) {
            Finish(c);
            return;
        }
        c->wait = c->routine.Current();
        c->yieldFrame = frame_;
        if (c->wait.kind == YieldInstruction::Kind::WaitForSeconds)
            c->wakeTime = curTime_ + static_cast<double>(c->wait.seconds);
        if (c->wait.kind == YieldInstruction::Kind::WaitForCoroutine) {
            const std::shared_ptr<Coroutine> child = c->wait.coroutine;
            if (!child || child->finished) {
                if (child && child->stopped) return;  // a stopped child never wakes its parent
                c->wait = YieldInstruction::Null();
                continue;                              // finished child: no wait
            }
            child->waiter = c;
            return;
        }
        queue_.push_back(c);
        return;
    }
}

void CoroutineScheduler::Finish(const std::shared_ptr<Coroutine>& c) {
    c->finished = true;
    live_.remove(c);
    if (c->waiter) {
        std::shared_ptr<Coroutine> parent = std::move(c->waiter);
        c->waiter.reset();
        parent->wait = YieldInstruction::Null();
        Resume(parent);
    }
}

// ---- CallbackUtil.DelayedCallback ----------------------------------------------------------------------------------
namespace CallbackUtil {

// CallbackUtil.cs:26 DelayedCallback
Enumerator DelayedCallback(float timeToWait, Callback<> cb) {
    co_yield YieldInstruction::WaitForSeconds(timeToWait);
    cb();
}

// CallbackUtil.cs:32 DelayedCallback
Enumerator DelayedCallback(int framesToWait, Callback<> cb) {
    for (int i = 0; i < framesToWait; i++) co_yield YieldInstruction::Null();
    cb();
}

// CallbackUtil.cs:41 DelayedCallback
Enumerator DelayedCallback(std::shared_ptr<Coroutine> yieldObj, Callback<> cb) {
    co_yield YieldInstruction::Wait(std::move(yieldObj));
    cb();
}

}  // namespace CallbackUtil

// ---- CoroutineHost -------------------------------------------------------------------------------------------------

// CoroutineHost.cs:8 Create
std::unique_ptr<CoroutineHost> CoroutineHost::Create(CoroutineScheduler& scheduler, std::string name, bool persistThroughSceneChanges, bool disposable) {
    std::unique_ptr<CoroutineHost> h(new CoroutineHost(scheduler, std::move(name), g_nextOwnerId++));
    h->disposable_ = disposable;
    h->persistent_ = persistThroughSceneChanges;  // Object.DontDestroyOnLoad: engine seam
    return h;
}

CoroutineHost::~CoroutineHost() {
    if (scheduler_ != nullptr) scheduler_->StopAllCoroutines(id_);
}

// CoroutineHost.cs:19 StartCoroutine
std::shared_ptr<Coroutine> CoroutineHost::StartCoroutine(Enumerator coroutine) {
    if (!disposable_) return scheduler_->StartCoroutine(std::move(coroutine), id_);
    return scheduler_->StartCoroutine(RunAndDispose(std::move(coroutine)), id_);
}

// CoroutineHost.cs:28 RunAndDispose
Enumerator CoroutineHost::RunAndDispose(Enumerator coroutine) {
    co_yield YieldInstruction::Wait(scheduler_->StartCoroutine(std::move(coroutine), id_));
    Terminate();
}

// CoroutineHost.cs:34 Terminate (Object.DestroyImmediate(gameObject): destroying the host stops its coroutines)
void CoroutineHost::Terminate() {
    destroyed_ = true;
    scheduler_->StopAllCoroutines(id_);
}

}  // namespace ksp
