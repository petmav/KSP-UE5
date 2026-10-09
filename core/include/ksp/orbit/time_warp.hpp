// TimeWarp (TimeWarp.cs, 865 lines; WP-10 "Time warp, rails propagation, orbit driver"): only the state and the
// static getters that WP-08's OrbitDriver / patched-conic code reads. WP-10 replaces this with the full port (rate
// changes, altitude limits, SOI-transition rate caps, physics warp). The host sets `fetch->curr_rate` / `Mode` the way
// TimeWarp.updateRate does (TimeWarp.cs:419: Planetarium.TimeScale = r; Time.timeScale = Mode == LOW ? r : 1;
// Time.fixedDeltaTime = 0.02f * Time.timeScale).
#pragma once

#include <vector>

namespace ksp {

class TimeWarp {
public:
    enum class Modes : int { HIGH = 0, LOW = 1 };  // TimeWarp.cs:8

    Modes Mode = Modes::HIGH;     // TimeWarp.cs:30
    float curr_rate = 1.0f;       // TimeWarp.cs:36 (private in KSP)
    std::vector<float> warpRates;  // TimeWarp.cs:20 (serialized on the scene object)

    static inline TimeWarp* fetch = nullptr;

    static Modes WarpMode();       // TimeWarp.cs:84
    static float CurrentRate();    // TimeWarp.cs:98
    static float fixedDeltaTime();  // TimeWarp.cs:174

    // TimeWarp.cs:419 updateRate(r) without the screen message and the control-lock bookkeeping (needs a Planetarium
    // for Planetarium.TimeScale; without one only the Time values change, as KSP's setter is a no-op without fetch).
    void updateRate(float r);
};

}  // namespace ksp
