// WarpTransition / WarpTransitionCalculator (WarpTransition.cs, WarpTransitionCalculator.cs); see
// include/ksp/orbit/warp_transition.hpp.
#include "ksp/orbit/warp_transition.hpp"

#include <algorithm>

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/orbit/time_warp.hpp"

namespace ksp {

// WarpTransition.cs:13
WarpTransition::WarpTransition(int Index, double Rate) : index_(Index), rate_(Rate) {
    UTToRateUp = 0.0;
    UTToRateDown = 0.0;
    UTTo1Times = 0.0;
}

// WarpTransitionCalculator.cs:10
double WarpTransitionCalculator::UTToRateDownOne() {
    return UTToRateDown(TimeWarp::CurrentRateIndex(), std::max(TimeWarp::CurrentRateIndex() - 1, 0));
}

// WarpTransitionCalculator.cs:12
double WarpTransitionCalculator::UTToRateTimesOne() { return UTToRateDown(TimeWarp::CurrentRateIndex(), 0); }

// WarpTransitionCalculator.cs:14
bool WarpTransitionCalculator::CheckForTransitionChanges() {
    if (TimeWarp::fetch == nullptr) return false;
    const std::vector<float>& array = TimeWarp::fetch->warpRates;
    bool flag = false;
    if (!warpRates.has_value()) {
        flag = true;
    } else if (warpRates->size() != array.size()) {
        flag = true;
    } else {
        for (size_t i = 0; i < array.size(); ++i) {
            if ((*warpRates)[i] != array[i]) {
                flag = true;
                break;
            }
        }
    }
    if (flag) {
        warpRates = array;  // copied element by element in KSP
        CalcWarpRateTransitions();
    }
    return flag;
}

// WarpTransitionCalculator.cs:53. `(rate[i] + rate[i-1]) / 2f` is a float expression assigned to a double property:
// evaluated in double under Mono, no rounding before the store (ksp/dotnet/mono_float.hpp).
void WarpTransitionCalculator::CalcWarpRateTransitions() {
    if (TimeWarp::fetch == nullptr) return;
    const std::vector<float>& rates = TimeWarp::fetch->warpRates;
    WarpRateTransitionPeriods = std::vector<WarpTransition>();
    for (size_t i = 0; i < rates.size(); ++i) {
        WarpTransition warpTransition(static_cast<int>(i), static_cast<double>(rates[i]));
        if (i > 0) warpTransition.UTToRateDown = (fx(rates[i]) + fx(rates[i - 1])) / fx(2.0f);
        if (i + 1 < rates.size()) warpTransition.UTToRateUp = (fx(rates[i]) + fx(rates[i + 1])) / fx(2.0f);
        WarpRateTransitionPeriods.push_back(warpTransition);
    }
    for (size_t j = 0; j < WarpRateTransitionPeriods.size(); ++j) {
        WarpTransition& warpTransition2 = WarpRateTransitionPeriods[j];
        warpTransition2.UTTo1Times = 0.0;
        for (size_t k = 0; k < WarpRateTransitionPeriods.size(); ++k) {
            const WarpTransition& warpTransition3 = WarpRateTransitionPeriods[k];
            if (warpTransition3.Index() <= warpTransition2.Index()) warpTransition2.UTTo1Times += warpTransition3.UTToRateDown;
        }
    }
}

// WarpTransitionCalculator.cs:88
double WarpTransitionCalculator::UTToRateDown(int FromIndex, int ToIndex) {
    double num = 0.0;
    for (size_t i = 0, count = WarpRateTransitionPeriods.size(); i < count; ++i)
        if (WarpRateTransitionPeriods[i].Index() == FromIndex) num = WarpRateTransitionPeriods[i].UTTo1Times;
    for (size_t j = 0, count2 = WarpRateTransitionPeriods.size(); j < count2; ++j)
        if (WarpRateTransitionPeriods[j].Index() == ToIndex) num -= WarpRateTransitionPeriods[j].UTTo1Times;
    return num;
}

// WarpTransitionCalculator.cs:110
int WarpTransitionCalculator::SafeRateToUTPeriod(double seconds) {
    int count = static_cast<int>(WarpRateTransitionPeriods.size());
    do {
        if (count-- <= 0) return 0;
    } while (WarpRateTransitionPeriods[static_cast<size_t>(count)].UTTo1Times >= seconds);
    return std::min(count + 1, static_cast<int>(WarpRateTransitionPeriods.size()) - 1);
}

}  // namespace ksp
