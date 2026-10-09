// TimeWarp subset for WP-08; see include/ksp/orbit/time_warp.hpp.
#include "ksp/orbit/time_warp.hpp"

#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"

namespace ksp {


// TimeWarp.cs:84
TimeWarp::Modes TimeWarp::WarpMode() { return fetch == nullptr ? Modes::HIGH : fetch->Mode; }

// TimeWarp.cs:98
float TimeWarp::CurrentRate() { return fetch == nullptr ? 1.0f : fetch->curr_rate; }

// TimeWarp.cs:174 (managed float code: Mono rule, the product is rounded at the return)
float TimeWarp::fixedDeltaTime() {
    if (fetch != nullptr) {
        if (WarpMode() != Modes::HIGH) return Time::fixedDeltaTime;
        return f32store(fx(Time::fixedDeltaTime) * fx(fetch->curr_rate));
    }
    return Time::fixedDeltaTime;
}

// TimeWarp.cs:419 updateRate (numeric statements)
void TimeWarp::updateRate(float r) {
    if (Planetarium::fetch != nullptr) Planetarium::fetch->timeScale = static_cast<double>(r);  // Planetarium.TimeScale = r
    Time::timeScale = (Mode == Modes::LOW) ? r : 1.0f;
    Time::fixedDeltaTime = f32store(fx(0.02f) * fx(Time::timeScale));
}

}  // namespace ksp
