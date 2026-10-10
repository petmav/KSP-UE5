// WP-00: CoroutineHost.cs (a MonoBehaviour that only exists to host coroutines) and the CallbackUtil.DelayedCallback
// coroutines (CallbackUtil.cs:26-60) on top of the CoroutineScheduler. GameObject creation / DontDestroyOnLoad are engine
// seams: the host object stays valid after Terminate() (IsDestroyed() is true, its coroutines are stopped).
#pragma once

#include <memory>
#include <string>

#include "ksp/core/coroutine.hpp"
#include "ksp/core/delegate.hpp"

namespace ksp {

namespace CallbackUtil {
// CallbackUtil.cs:26 DelayedCallback(float timeToWait, Callback cb)
Enumerator DelayedCallback(float timeToWait, Callback<> cb);
// CallbackUtil.cs:32 DelayedCallback(int framesToWait, Callback cb)
Enumerator DelayedCallback(int framesToWait, Callback<> cb);
// CallbackUtil.cs:41 DelayedCallback(IEnumerable yieldObj, Callback cb): yieldObj is a started coroutine
Enumerator DelayedCallback(std::shared_ptr<Coroutine> yieldObj, Callback<> cb);
}  // namespace CallbackUtil

class CoroutineHost {
public:
    // CoroutineHost.cs:8 Create. `persistThroughSceneChanges` is DontDestroyOnLoad (engine seam, recorded only).
    static std::unique_ptr<CoroutineHost> Create(CoroutineScheduler& scheduler, std::string name, bool persistThroughSceneChanges, bool disposable);
    ~CoroutineHost();

    std::shared_ptr<Coroutine> StartCoroutine(Enumerator coroutine);  // CoroutineHost.cs:19
    void Terminate();                                                 // CoroutineHost.cs:34
    bool IsDestroyed() const { return destroyed_; }
    bool Persistent() const { return persistent_; }
    bool Disposable() const { return disposable_; }
    const std::string& Name() const { return name_; }
    int OwnerId() const { return id_; }

private:
    CoroutineHost(CoroutineScheduler& s, std::string name, int id) : scheduler_(&s), name_(std::move(name)), id_(id) {}
    Enumerator RunAndDispose(Enumerator coroutine);                   // CoroutineHost.cs:28

    CoroutineScheduler* scheduler_;
    std::string name_;
    int id_;
    bool disposable_ = false;
    bool persistent_ = false;
    bool destroyed_ = false;
};

}  // namespace ksp
