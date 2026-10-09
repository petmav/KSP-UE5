// Port of harness/src/KspUnityShim/AnimationCurve.cs (managed transcription of Unity 2019.4.18f1 UnityPlayer.dll
// AnimationCurveTpl<float>; RVAs quoted per function). float32 throughout, native operation order.
#include "ksp/unity/animation_curve.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace ksp {

namespace {
constexpr float kDefaultWeight = 0.333333343f;  // 0x3EAAAAAB
constexpr float kInf = std::numeric_limits<float>::infinity();

bool IsFinite(float f) {
    uint32_t b;
    std::memcpy(&b, &f, sizeof b);
    return (b & 0x7F800000u) != 0x7F800000u;
}

// SetPostInfinity/SetPreInfinity (RVA 0x75E370 / 0x75E430): Loop(2)->1, PingPong(4)->0, Default(0)->3, else->2 (clamp)
int MapWrap(int mode) {
    if (mode == 2) return 1;
    if (mode == 4) return 0;
    return mode != 0 ? 2 : 3;
}
}  // namespace

AnimationCurve::Cache AnimationCurve::InitialCache() {
    Cache c;
    c.index = 0;
    c.time = kInf;
    return c;
}

// AnimationCurveTpl<float>::InvalidateCache (RVA 0x75DCF0): index = 0, time = +inf (timeEnd untouched).
void AnimationCurve::InvalidateCache() const {
    cache_.index = 0;
    cache_.time = kInf;
}

void AnimationCurve::UpdateDuplicateFlag() {
    hasDuplicateTimes_ = false;
    for (size_t i = 1; i < keys_.size(); ++i)
        if (!(keys_[i - 1].time < keys_[i].time)) {
            hasDuplicateTimes_ = true;
            break;
        }
}

// AnimationCurveBindings::Internal_Create / SetKeys (RVA 0x75D220 / 0x75E210): copy keys, erase keys whose value or
// time is not finite, std::sort by time (stable here; identical for unique times), invalidate the cache.
void AnimationCurve::SetKeys(const std::vector<Keyframe>& keys) {
    keys_.clear();
    for (const Keyframe& k : keys) {
        if (!IsFinite(k.value) || !IsFinite(k.time)) continue;
        keys_.push_back(k);
    }
    std::stable_sort(keys_.begin(), keys_.end(), [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
    UpdateDuplicateFlag();
    InvalidateCache();
}

// Unity deserialization (StreamedBinaryRead Transfer of AnimationCurveTpl, RVA 0x17FE80): verbatim keys and wrap modes.
AnimationCurve AnimationCurve::CreateFromSerialized(const std::vector<Keyframe>& keys, int preInfinity, int postInfinity) {
    AnimationCurve c;
    c.keys_ = keys;
    c.preInfinity_ = preInfinity;
    c.postInfinity_ = postInfinity;
    c.preWrapManaged_ = preInfinity == 2 ? WrapMode::ClampForever : static_cast<WrapMode>(-1);
    c.postWrapManaged_ = postInfinity == 2 ? WrapMode::ClampForever : static_cast<WrapMode>(-1);
    c.UpdateDuplicateFlag();
    c.InvalidateCache();
    return c;
}

void AnimationCurve::SetPreWrapMode(WrapMode m) {
    preWrapManaged_ = m;
    preInfinity_ = MapWrap(static_cast<int>(m));
    InvalidateCache();
}

void AnimationCurve::SetPostWrapMode(WrapMode m) {
    postWrapManaged_ = m;
    postInfinity_ = MapWrap(static_cast<int>(m));
    InvalidateCache();
}

// AnimationCurveTpl<float>::IsValid (RVA 0xD6A60): >= 1 key and finite range.
bool AnimationCurve::IsValid() const {
    if (keys_.empty()) return false;
    return IsFinite(keys_.front().time) && IsFinite(keys_.back().time);
}

// AnimationCurveTpl<float>::Evaluate (AnimationCurve_CUSTOM_Evaluate, RVA 0x75A6A0)
float AnimationCurve::Evaluate(float time) const {
    if (hasDuplicateTimes_) return EvaluateWith(cache_, time);  // faithful cache emulation
    Cache fresh = InitialCache();
    return EvaluateWith(fresh, time);
}

float AnimationCurve::EvaluateWith(Cache& cache, float time) const {
    const float curveT = time;
    const int n = static_cast<int>(keys_.size());
    if (n == 1) return keys_[0].value;

    auto poly = [&cache](float t) {
        float r = t * cache.a;
        r = r + cache.b;
        r = r * t;
        r = r + cache.c;
        r = r * t;
        r = r + cache.d;
        return r;
    };

    // cache hit: curveT >= cache.time && cache.timeEnd > curveT
    if (!(curveT < cache.time) && !std::isnan(curveT) && !std::isnan(cache.time)) {
        if (cache.timeEnd > curveT) return poly(curveT - cache.time);
    }
    if (!IsValid()) return 0.0f;

    const float last = keys_[static_cast<size_t>(n - 1)].time;
    const float first = keys_[0].time;
    if (curveT >= last) {
        if (postInfinity_ == 2) {
            cache.time = last;
            cache.timeEnd = kInf;
            cache.a = 0.0f;
            cache.b = 0.0f;
            cache.c = 0.0f;
            cache.d = keys_[static_cast<size_t>(n - 1)].value;
            return poly(curveT - cache.time);
        }
        throw std::logic_error("AnimationCurve: non-clamp post-wrap mode not ported");
    }
    if (first > curveT) {
        if (preInfinity_ == 2) {
            cache.time = curveT - 1000.0f;
            cache.timeEnd = first;
            cache.a = 0.0f;
            cache.b = 0.0f;
            cache.c = 0.0f;
            cache.d = keys_[0].value;
            return poly(curveT - cache.time);
        }
        throw std::logic_error("AnimationCurve: non-clamp pre-wrap mode not ported");
    }
    int lhs, rhs;
    FindIndexForSampling(cache, curveT, lhs, rhs);
    if ((static_cast<int>(keys_[static_cast<size_t>(lhs)].weightedMode) & 2) != 0 ||
        (static_cast<int>(keys_[static_cast<size_t>(rhs)].weightedMode) & 1) != 0)
        throw std::logic_error("AnimationCurve: weighted (Bezier) segment not ported");
    CalculateCacheData(cache, lhs, rhs, 0.0f);
    return poly(curveT - cache.time);
}

// AnimationCurveTpl<float>::CalculateCacheData (RVA 0x759230)
void AnimationCurve::CalculateCacheData(Cache& cache, int lhsIndex, int rhsIndex, float timeOffset) const {
    const Keyframe& L = keys_[static_cast<size_t>(lhsIndex)];
    const Keyframe& R = keys_[static_cast<size_t>(rhsIndex)];
    cache.index = lhsIndex;
    cache.time = timeOffset + L.time;
    cache.timeEnd = timeOffset + R.time;
    const float dx = R.time - L.time;
    const float dy = R.value - L.value;
    const float length = (1e-4f > dx) ? 1e-4f : dx;  // maxss(1e-4, dx)
    const float m1 = L.outTangent;
    const float d1 = length * R.inTangent;
    const float d0 = length * m1;
    const float length2 = length * length;
    const float inv = 1.0f / length2;
    cache.c = m1;
    float a = d1 + d0;
    a = a - dy;
    a = a - dy;
    a = a * inv;
    a = a / length;
    cache.a = a;
    float b = dy + dy;
    b = b + dy;
    b = b - d0;
    b = b - d0;
    b = b - d1;
    b = b * inv;
    cache.b = b;
    cache.d = L.value;
    if (L.outTangent == kInf || R.inTangent == kInf) {
        cache.a = 0.0f;
        cache.b = 0.0f;
        cache.c = 0.0f;
        cache.d = L.value;
    }
}

// AnimationCurveTpl<float>::FindIndexForSampling (RVA 0x75C770): cache-hint scan (3 steps each way), then upper_bound.
void AnimationCurve::FindIndexForSampling(const Cache& cache, float t, int& lhs, int& rhs) const {
    const int count = static_cast<int>(keys_.size());
    auto kt = [this](int i) { return keys_[static_cast<size_t>(i)].time; };
    int r8 = cache.index;
    if (r8 != -1) {
        if (t > kt(r8)) {
            for (int step = 0; step < 3; ++step) {
                const int next = r8 + 1;
                if (next < count && kt(next) > t) {
                    lhs = r8;
                    rhs = std::min(r8 + 1, count - 1);
                    return;
                }
                r8 = next;
            }
        } else {
            int rdx = r8;
            for (int step = 0; step < 3; ++step) {
                if (rdx >= 0 && t >= kt(rdx)) {
                    lhs = r8;
                    rhs = std::min(r8 + 1, count - 1);
                    return;
                }
                --r8;
                --rdx;
            }
        }
    }
    int first = 0;
    int len = count;
    while (len > 0) {
        const int half = len >> 1;
        const int mid = first + half;
        if (kt(mid) > t) {
            len = half;
        } else {
            first = mid + 1;
            len = len - half - 1;
        }
    }
    lhs = first - 1;
    rhs = std::min(first, count - 1);
}

// AnimationCurveTpl<float>::AddKey(Keyframe) (RVA 0x758840)
int AnimationCurve::AddKey(const Keyframe& key) {
    InvalidateCache();
    int first = 0;
    int len = static_cast<int>(keys_.size());
    while (len > 0) {
        const int half = len >> 1;
        if (key.time > keys_[static_cast<size_t>(first + half)].time) {
            first = first + half + 1;
            len = len - half - 1;
        } else {
            len = half;
        }
    }
    if (first != static_cast<int>(keys_.size()) && !(keys_[static_cast<size_t>(first)].time > key.time)) return -1;
    keys_.insert(keys_.begin() + first, key);
    UpdateDuplicateFlag();
    return first;
}

// AnimationCurve_CUSTOM_AddKey(t, v) -> AddKeySmoothTangents (RVA 0x750780)
int AnimationCurve::AddKey(float time, float value) {
    Keyframe k(time, value, 0.0f, 0.0f);
    k.weightedMode = WeightedMode::None;
    k.inWeight = kDefaultWeight;
    k.outWeight = kDefaultWeight;
    const int index = AddKey(k);
    if (index == -1) return -1;
    if (index > 0) RecalculateSplineSlope(index - 1, 0.0f);
    RecalculateSplineSlope(index, 0.0f);
    if (index + 1 < static_cast<int>(keys_.size())) RecalculateSplineSlope(index + 1, 0.0f);
    return index;
}

// RecalculateSplineSlopeT<float> (RVA 0x74FDB0)
void AnimationCurve::RecalculateSplineSlope(int key, float b) {
    const int count = static_cast<int>(keys_.size());
    if (count < 2) return;
    Keyframe k = keys_[static_cast<size_t>(key)];
    auto K = [this](int i) -> const Keyframe& { return keys_[static_cast<size_t>(i)]; };
    if (key == 0) {
        const float dy = K(1).value - K(0).value;
        const float dx = K(1).time - K(0).time;
        const float m = dy / dx;
        k.inTangent = m;
        k.outTangent = m;
        k.outWeight = kDefaultWeight;
        keys_[static_cast<size_t>(key)] = k;
        InvalidateCache();
        return;
    }
    if (key == count - 1) {
        const float dy = k.value - K(key - 1).value;
        const float dx = k.time - K(key - 1).time;
        const float m = dy / dx;
        k.inTangent = m;
        k.outTangent = m;
        k.inWeight = kDefaultWeight;
        keys_[static_cast<size_t>(key)] = k;
        InvalidateCache();
        return;
    }
    const float t0 = k.time;
    const float v0 = k.value;
    const float dx1 = t0 - K(key - 1).time;
    const float dx2 = K(key + 1).time - t0;
    const float dy2 = K(key + 1).value - v0;
    float m1;
    if (std::fabs(dx1) > 1e-5f) {
        const float dy1 = v0 - K(key - 1).value;
        m1 = dy1 / dx1;
    } else {
        m1 = 0.0f;
    }
    float m2;
    if (std::fabs(dx2) > 1e-5f) m2 = dy2 / dx2;
    else m2 = 0.0f;
    float wR = 1.0f - b;
    float wL = b + 1.0f;
    wR = wR * 0.5f;
    wL = wL * 0.5f;
    wR = wR * m2;
    wL = wL * m1;
    const float slope = wR + wL;
    k.inTangent = slope;
    k.outTangent = slope;
    k.inWeight = kDefaultWeight;
    k.outWeight = kDefaultWeight;
    keys_[static_cast<size_t>(key)] = k;
    InvalidateCache();
}

// AnimationCurveBindings::SmoothTangents (RVA 0x75E500)
void AnimationCurve::SmoothTangents(int index, float weight) {
    if (index < 0 || index >= length()) throw std::out_of_range("AnimationCurve.SmoothTangents index");
    RecalculateSplineSlope(index, weight);
}

// AnimationCurveBindings::RemoveKey (RVA 0x75E040)
void AnimationCurve::RemoveKey(int index) {
    if (index < 0 || index >= length()) throw std::out_of_range("AnimationCurve.RemoveKey index");
    InvalidateCache();
    keys_.erase(keys_.begin() + index);
    UpdateDuplicateFlag();
}

// AnimationCurve.cs (managed) Constant / Linear / EaseInOut
AnimationCurve AnimationCurve::Constant(float timeStart, float timeEnd, float value) {
    return Linear(timeStart, value, timeEnd, value);
}

AnimationCurve AnimationCurve::Linear(float timeStart, float valueStart, float timeEnd, float valueEnd) {
    const float num = (valueEnd - valueStart) / (timeEnd - timeStart);
    return AnimationCurve({Keyframe(timeStart, valueStart, 0.0f, num), Keyframe(timeEnd, valueEnd, num, 0.0f)});
}

AnimationCurve AnimationCurve::EaseInOut(float timeStart, float valueStart, float timeEnd, float valueEnd) {
    return AnimationCurve({Keyframe(timeStart, valueStart, 0.0f, 0.0f), Keyframe(timeEnd, valueEnd, 0.0f, 0.0f)});
}

}  // namespace ksp
