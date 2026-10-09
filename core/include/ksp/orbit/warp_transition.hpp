// WarpTransition.cs (23 lines) and WarpTransitionCalculator.cs (123 lines): the UT-based warp-down planner. How many
// seconds of universal time the game needs to ramp the warp rate from rate index A down to index B (the rate lerps
// linearly in real time, so the UT elapsed while stepping down one index is the average of the two rates).
// `internal` in KSP; the callers are the alarm clock / flight UI (WP-08 / UI packages).
#pragma once

#include <optional>
#include <vector>

namespace ksp {

// WarpTransition.cs:1
class WarpTransition {
public:
    WarpTransition(int Index, double Rate);  // WarpTransition.cs:13 (internal)

    int Index() const { return index_; }
    double Rate() const { return rate_; }
    double UTTo1Times = 0.0;
    double UTToRateDown = 0.0;
    double UTToRateUp = 0.0;

private:
    int index_;
    double rate_;
};

// WarpTransitionCalculator.cs:4 (static class)
struct WarpTransitionCalculator {
    static inline std::vector<WarpTransition> WarpRateTransitionPeriods;  // :6
    static inline std::optional<std::vector<float>> warpRates;             // :8 (null until the first check)

    static double UTToRateDownOne();    // :10 UTToRateDown(CurrentRateIndex, Math.Max(CurrentRateIndex - 1, 0))
    static double UTToRateTimesOne();   // :12 UTToRateDown(CurrentRateIndex, 0)
    static bool CheckForTransitionChanges();  // :14
    static void CalcWarpRateTransitions();    // :53
    static double UTToRateDown(int FromIndex, int ToIndex);  // :88
    static int SafeRateToUTPeriod(double seconds);           // :110
};

}  // namespace ksp
