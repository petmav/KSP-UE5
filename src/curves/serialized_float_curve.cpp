#include "ksp/curves/serialized_float_curve.hpp"

namespace ksp {

FloatCurve MakeSerializedFloatCurve(const std::vector<SerializedKeyframe>& keys, int preInfinity, int postInfinity,
                                    float minTime, float maxTime) {
    std::vector<Keyframe> k;
    k.reserve(keys.size());
    for (const SerializedKeyframe& s : keys) {
        Keyframe kf(s.time, s.value, s.inSlope, s.outSlope);
        kf.weightedMode = static_cast<WeightedMode>(s.weightedMode);
        kf.inWeight = s.inWeight;
        kf.outWeight = s.outWeight;
        k.push_back(kf);
    }
    FloatCurve fc;
    fc.SetCurve(AnimationCurve::CreateFromSerialized(k, preInfinity, postInfinity));
    fc.SetSerializedRange(minTime, maxTime);
    return fc;
}

}  // namespace ksp
