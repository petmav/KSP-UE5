// C# delegates (System.Delegate / MulticastDelegate) for ported KSP code.
//
// KSP's event bus, FSM and callbacks rely on three delegate properties that std::function does not have:
//   * equality: two delegates are equal when their invocation lists have the same length and every entry has the same
//     target object (reference equality) and the same method. EventData.Remove(OnFoo) finds a listener this way;
//   * Target: the object a closed instance delegate is bound to (EventData.Add reads evt.Target and its type name and
//     throws NullReferenceException for static methods, whose Target is null);
//   * multicast: `a + b` / `a += b` (Delegate.Combine) concatenates invocation lists, `a - b` (Delegate.Remove) removes
//     the LAST occurrence of b's invocation list as a contiguous run; invoking calls every entry in order and returns
//     the last result; invoking a null delegate throws NullReferenceException.
//
// Creating delegates (C# -> C++):
//   GameEvents.onX.Add(OnX)           -> GameEvents::onX.Add(this, &Foo::OnX)   (or Delegate<...>::Bind(this, &Foo::OnX))
//   new Callback(() => ...) capturing -> Delegate<void()>::Lambda(this, [=] { ... })   (target = the closure's owner)
//   static method group               -> Delegate<void()>::Static(&Foo::StaticMethod)
// Bind: method identity = the member-function pointer (equal for the same method, like C#). Lambda: every Lambda()
// call creates a new method identity (copies of the resulting Delegate stay equal), which matches C#, where two
// evaluations of a lambda expression only compare equal when they share the cached delegate instance.
//
// Lifetime: a delegate stores a raw target pointer. C# keeps a destroyed MonoBehaviour alive as a managed object; C++
// does not, so owners must Remove their listeners before they are destroyed (KSP does this in OnDestroy).
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include "ksp/core/exceptions.hpp"

namespace ksp {

namespace detail {

// Identity of the method a delegate entry calls.
struct DelegateMethodId {
    const void* kind = nullptr;  // per-type tag (member-pointer type, static-function signature, or lambda space)
    uint64_t uid = 0;            // lambdas: unique id; static functions: the function address
    std::shared_ptr<const void> memberPtr;               // Bind: copy of the member-function pointer
    bool (*memberPtrEq)(const void*, const void*) = nullptr;

    bool operator==(const DelegateMethodId& o) const {
        if (kind != o.kind || uid != o.uid) return false;
        if (memberPtr || o.memberPtr) return memberPtr && o.memberPtr && memberPtrEq(memberPtr.get(), o.memberPtr.get());
        return true;
    }
};

template <class T>
inline constexpr char kDelegateKindTag = 0;
struct LambdaKind {};

inline uint64_t NextDelegateUid() {
    static std::atomic<uint64_t> next{1};
    return next.fetch_add(1, std::memory_order_relaxed);
}

// C# Type.Name: the simple type name without namespace (MSVC's typeid names are "class ns::Name<...>").
std::string CsTypeName(const std::type_info& ti);

template <class T>
std::string DynamicTypeName(const T* obj) {
    if constexpr (std::is_polymorphic_v<T>) {
        if (obj) return CsTypeName(typeid(*obj));
    }
    return CsTypeName(typeid(T));
}

}  // namespace detail

template <class Sig>
class Delegate;

template <class R, class... Args>
class Delegate<R(Args...)> {
public:
    using Function = std::function<R(Args...)>;

    struct Entry {
        const void* target = nullptr;  // Delegate.Target of this entry (nullptr for static methods)
        std::string targetType;        // Target.GetType().Name ("" for static methods)
        detail::DelegateMethodId method;
        Function fn;
    };

    Delegate() = default;  // null
    Delegate(std::nullptr_t) {}

    // Closed instance delegate: obj->*method. Equal to any other Bind of the same object and method.
    template <class T, class M>
    static Delegate Bind(T* obj, M method) {
        static_assert(std::is_member_function_pointer_v<M>, "Bind needs a member-function pointer");
        Delegate d;
        Entry e;
        e.target = obj;
        e.targetType = detail::DynamicTypeName(obj);
        e.method.kind = &detail::kDelegateKindTag<M>;
        e.method.memberPtr = std::make_shared<M>(method);
        e.method.memberPtrEq = [](const void* a, const void* b) { return *static_cast<const M*>(a) == *static_cast<const M*>(b); };
        e.fn = [obj, method](Args... args) -> R { return (obj->*method)(std::forward<Args>(args)...); };
        d.list_.push_back(std::move(e));
        return d;
    }

    // Closure delegate with an explicit target object (the C# closure's owner). A fresh method identity per call.
    template <class T, class F>
    static Delegate Lambda(T* target, F&& f) {
        Delegate d;
        Entry e;
        e.target = target;
        e.targetType = detail::DynamicTypeName(target);
        e.method.kind = &detail::kDelegateKindTag<detail::LambdaKind>;
        e.method.uid = detail::NextDelegateUid();
        e.fn = Function(std::forward<F>(f));
        d.list_.push_back(std::move(e));
        return d;
    }

    // Static method delegate (Target == null).
    static Delegate Static(R (*fn)(Args...)) {
        if (!fn) return Delegate();
        Delegate d;
        Entry e;
        e.method.kind = &detail::kDelegateKindTag<R (*)(Args...)>;
        e.method.uid = reinterpret_cast<uint64_t>(fn);
        e.fn = fn;
        d.list_.push_back(std::move(e));
        return d;
    }

    bool IsNull() const { return list_.empty(); }
    explicit operator bool() const { return !list_.empty(); }

    // MulticastDelegate.Target: the target of the last invocation-list entry (null delegate: NullReferenceException).
    const void* Target() const {
        if (list_.empty()) throw NullReferenceException();
        return list_.back().target;
    }
    // Target.GetType().Name (throws NullReferenceException when Target is null, like `evt.Target.GetType()`).
    const std::string& TargetTypeName() const {
        if (Target() == nullptr) throw NullReferenceException();
        return list_.back().targetType;
    }

    const std::vector<Entry>& GetInvocationList() const { return list_; }

    // Invoke: every entry in order, the last result is returned; a null delegate throws NullReferenceException.
    R operator()(Args... args) const {
        if (list_.empty()) throw NullReferenceException();
        // C# invokes a snapshot (delegates are immutable); copy so a callee reassigning the field cannot break the loop.
        if (list_.size() == 1) return list_.front().fn(std::forward<Args>(args)...);
        const std::vector<Entry> snapshot = list_;
        for (size_t i = 0; i + 1 < snapshot.size(); ++i) snapshot[i].fn(args...);
        return snapshot.back().fn(std::forward<Args>(args)...);
    }

    // Delegate equality (op_Equality): same invocation list (target + method per entry); null == null.
    friend bool operator==(const Delegate& a, const Delegate& b) {
        if (a.list_.size() != b.list_.size()) return false;
        for (size_t i = 0; i < a.list_.size(); ++i)
            if (a.list_[i].target != b.list_[i].target || !(a.list_[i].method == b.list_[i].method)) return false;
        return true;
    }
    friend bool operator==(const Delegate& a, std::nullptr_t) { return a.list_.empty(); }

    // Delegate.Combine
    friend Delegate operator+(const Delegate& a, const Delegate& b) {
        Delegate r = a;
        r.list_.insert(r.list_.end(), b.list_.begin(), b.list_.end());
        return r;
    }
    Delegate& operator+=(const Delegate& b) { return *this = *this + b; }

    // Delegate.Remove: removes the last occurrence of b's invocation list (as a contiguous run); unchanged if absent.
    friend Delegate operator-(const Delegate& a, const Delegate& b) {
        if (b.list_.empty() || a.list_.size() < b.list_.size()) return a;
        for (size_t start = a.list_.size() - b.list_.size() + 1; start-- > 0;) {
            bool match = true;
            for (size_t j = 0; j < b.list_.size() && match; ++j) {
                const Entry& x = a.list_[start + j];
                const Entry& y = b.list_[j];
                match = x.target == y.target && x.method == y.method;
            }
            if (match) {
                Delegate r;
                r.list_.reserve(a.list_.size() - b.list_.size());
                r.list_.insert(r.list_.end(), a.list_.begin(), a.list_.begin() + static_cast<std::ptrdiff_t>(start));
                r.list_.insert(r.list_.end(), a.list_.begin() + static_cast<std::ptrdiff_t>(start + b.list_.size()),
                               a.list_.end());
                return r;
            }
        }
        return a;
    }
    Delegate& operator-=(const Delegate& b) { return *this = *this - b; }

private:
    std::vector<Entry> list_;  // empty = null delegate
};

// Callback.cs: public delegate void Callback(); Callback<T>(T arg1); ... up to 4 arguments.
template <class... Ts>
using Callback = Delegate<void(Ts...)>;

}  // namespace ksp
