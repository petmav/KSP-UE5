// UnityEngine.Bounds (float32), the managed part of Unity 2019.4.18f1.
// Spec: decompiled/KSP-Decompiled/Unity/UnityEngine.CoreModule/UnityEngine/Bounds.cs (center/extents storage, size, min,
// max, SetMinMax, Encapsulate, Expand). Intersect* are native and not ported. Managed float arithmetic follows Mono's
// rule through the Vector3f operators (ksp/dotnet/mono_float.hpp).
#pragma once

#include "ksp/unity/vector.hpp"

namespace ksp {

struct Bounds {
    Vector3f m_Center{};
    Vector3f m_Extents{};

    constexpr Bounds() = default;
    // Bounds.cs:83
    Bounds(const Vector3f& center, const Vector3f& size) : m_Center(center), m_Extents(size * 0.5f) {}

    const Vector3f& center() const { return m_Center; }
    void set_center(const Vector3f& v) { m_Center = v; }
    // Bounds.cs:34
    Vector3f size() const { return m_Extents * 2.0f; }
    void set_size(const Vector3f& v) { m_Extents = v * 0.5f; }
    const Vector3f& extents() const { return m_Extents; }
    void set_extents(const Vector3f& v) { m_Extents = v; }
    // Bounds.cs:62 / 76
    Vector3f min() const { return center() - extents(); }
    Vector3f max() const { return center() + extents(); }

    // Bounds.cs:113
    bool Equals(const Bounds& o) const { return m_Center.Equals(o.m_Center) && m_Extents.Equals(o.m_Extents); }
    // Bounds.cs:130 SetMinMax
    void SetMinMax(const Vector3f& min_, const Vector3f& max_) {
        m_Extents = (max_ - min_) * 0.5f;
        m_Center = min_ + m_Extents;
    }
    // Bounds.cs:136 Encapsulate(Vector3)
    void Encapsulate(const Vector3f& point) { SetMinMax(Vector3f::Min(min(), point), Vector3f::Max(max(), point)); }
    // Bounds.cs:141 Encapsulate(Bounds)
    void Encapsulate(const Bounds& bounds) {
        Encapsulate(bounds.center() - bounds.extents());
        Encapsulate(bounds.center() + bounds.extents());
    }
    // Bounds.cs:147 Expand(float)
    void Expand(float amount) {
        amount = f32store(fx(amount) * 0.5);
        m_Extents = m_Extents + Vector3f(amount, amount, amount);
    }
    // Bounds.cs:153 Expand(Vector3)
    void Expand(const Vector3f& amount) { m_Extents = m_Extents + amount * 0.5f; }
};

inline bool operator==(const Bounds& l, const Bounds& r) { return l.center() == r.center() && l.extents() == r.extents(); }
inline bool operator!=(const Bounds& l, const Bounds& r) { return !(l == r); }

}  // namespace ksp
