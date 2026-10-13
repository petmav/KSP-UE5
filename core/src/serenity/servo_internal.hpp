// WP-52: helpers shared by the servo module sources (not installed)
#pragma once

#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/serenity/base_servo.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Serenity::detail {

inline NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
inline float F(double v) { return f32store(v); }
// float.Equals(float): exact, NaN equals NaN
inline bool FloatEquals(float a, float b) { return a == b || (std::isnan(a) && std::isnan(b)); }
// Vector3.operator==: (lhs - rhs).sqrMagnitude < 9.99999944E-11f (Vector3.cs)
inline bool Vector3Equal(const Vector3f& a, const Vector3f& b) {
    const double dx = fx(a.x) - fx(b.x), dy = fx(a.y) - fx(b.y), dz = fx(a.z) - fx(b.z);
    return f32store(dx * dx + dy * dy + dz * dz) < 9.99999944E-11f;
}
// TimeWarp.CurrentRateIndex > 0 && TimeWarp.WarpMode == TimeWarp.Modes.HIGH
inline bool HighWarp() { return TimeWarp::CurrentRateIndex() > 0 && TimeWarp::WarpMode() == TimeWarp::Modes::HIGH; }
// the part's own Part* (throws like a null `base.part`)
inline Part& PartRef(BaseServo& s) {
    Part* p = s.PartOf();
    if (p == nullptr) throw Nre();
    return *p;
}
// Part.symmetryCounterparts[0].Modules[index] as T
template <class T>
T* SymmetryModule(T& self) {
    Part* p = self.PartOf();
    if (p == nullptr || p->symmetryCounterparts.empty()) throw std::out_of_range("Index was out of range. Must be non-negative and less than the size of the collection.");
    const int index = p->Modules().IndexOf(&self);
    return dynamic_cast<T*>(p->symmetryCounterparts[0]->Modules().GetModule(static_cast<size_t>(index)));
}

// The shared head of OnResourceNonemptyEmpty (hinge :652, piston :604, rotor :721, rotation servo :710): false = the C# `return`
inline bool ResourceEventApplies(BaseServo& s, PartResource* resource) {
    Part& p = PartRef(s);
    if (p.vessel == nullptr || resource == nullptr || resource->part == nullptr || resource->part->vessel == nullptr) throw Nre();
    if (p.vessel->persistentId != resource->part->vessel->persistentId) return false;
    const bool inSet = ServoEnvironment::resourcePartSetContains ? ServoEnvironment::resourcePartSetContains(*p.vessel, *resource->part) : true;
    if (!inSet) return false;
    return s.resHandler.IsResourceBelowShutOffLimit(*resource);
}
inline bool ResourceIsAnInput(BaseServo& s, const PartResource& resource, size_t i) {
    return resource.resourceName == s.resHandler.inputResources[i]->name;
}

}  // namespace ksp::Expansions::Serenity::detail
