// VesselValues.cs (144 lines): the vessel-wide extremum of the per-part event values (Part.PartValues) with a per-frame cache.
// The host Vessel is reached through two accessors so this header does not depend on vessel.hpp.
#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace ksp {

class Part;
class Vessel;

// VesselValues.PartValuesComparison<T> (VesselValues.cs:6): the best value over host.Parts, scanned from the last part to the
// first (the comparer decides whether a part's value replaces the running one), cached for the current Time.frameCount.
template <class T>
class PartValuesComparison {
public:
    using Comparer = std::function<bool(T, T)>;
    using Accessor = std::function<T(Part&)>;
    using PartsFn = std::function<std::vector<Part*>&()>;
    using FrameFn = std::function<int()>;

    T defaultValue;

    // VesselValues.cs:58
    PartValuesComparison(PartsFn parts, FrameFn frame, T defaultValue_, Comparer comparison, Accessor valueAccessor)
        : defaultValue(defaultValue_), parts_(std::move(parts)), frame_(std::move(frame)), comparer_(std::move(comparison)),
          valueAccessor_(std::move(valueAccessor)) {}

    // VesselValues.cs:20 `value`
    T value() {
        if (frame_() == currentFrame_) return lastValue_;
        T val = defaultValue;
        std::vector<Part*>& ps = parts_();
        int count = static_cast<int>(ps.size());
        while (count-- > 0) {
            T val2 = valueAccessor_(*ps[static_cast<size_t>(count)]);
            if (comparer_(val2, val)) val = val2;
        }
        currentFrame_ = frame_();
        lastValue_ = val;
        return val;
    }
    // VesselValues.cs:44 `valueUncached`
    T valueUncached() {
        currentFrame_ = -1;
        return value();
    }
    // VesselValues.cs:62
    void ResetValueCache() { currentFrame_ = -1; }

private:
    PartsFn parts_;
    FrameFn frame_;
    Comparer comparer_;
    Accessor valueAccessor_;
    int currentFrame_ = -1;
    T lastValue_{};
};

// VesselValues.PartValuesOperation<T> (VesselValues.cs:73): the fold of an operation over host.Parts (last to first)
template <class T>
class PartValuesOperation {
public:
    using Operation = std::function<T(T, T)>;
    using Accessor = std::function<T(Part&)>;
    using PartsFn = std::function<std::vector<Part*>&()>;

    T defaultValue;

    PartValuesOperation(PartsFn parts, T defaultValue_, Operation operation, Accessor valueAccessor)  // VesselValues.cs:97
        : defaultValue(defaultValue_), parts_(std::move(parts)), operation_(std::move(operation)), valueAccessor_(std::move(valueAccessor)) {}

    T value() {  // VesselValues.cs:83
        T val = defaultValue;
        std::vector<Part*>& ps = parts_();
        int count = static_cast<int>(ps.size());
        while (count-- > 0) val = operation_(valueAccessor_(*ps[static_cast<size_t>(count)]), val);
        return val;
    }

private:
    PartsFn parts_;
    Operation operation_;
    Accessor valueAccessor_;
};

class VesselValues {
public:
    PartValuesComparison<float> MaxThrottle;
    PartValuesComparison<float> HeatProduction;
    PartValuesComparison<float> FuelUsage;
    PartValuesComparison<float> EnginePower;
    PartValuesComparison<float> SteeringRadius;
    PartValuesComparison<int> AutopilotSkill;
    PartValuesComparison<int> AutopilotKerbalSkill;
    PartValuesComparison<int> AutopilotSASSkill;
    PartValuesComparison<int> RepairSkill;
    PartValuesComparison<int> FailureRepairSkill;
    PartValuesComparison<int> ScienceSkill;
    PartValuesComparison<int> EVAChuteSkill;
    PartValuesComparison<float> CommsRange;

    explicit VesselValues(Vessel& host);  // VesselValues.cs:115
};

}  // namespace ksp
