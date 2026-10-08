// FloatCurves as Unity deserializes them from KSP's asset files (prefab data): raw keyframes (inSlope/outSlope,
// weights), native wrap modes and FloatCurve's serialized _minTime/_maxTime. Used by generated prefab data
// (PhysicsGlobals level0 prefab, CelestialBody PSystem prefab).
#pragma once

#include <cstdint>
#include <vector>

#include "ksp/curves/float_curve.hpp"

namespace ksp {

struct SerializedKeyframe {
    float time;
    float value;
    float inSlope;
    float outSlope;
    int32_t weightedMode;
    float inWeight;
    float outWeight;
};

// AnimationCurve::CreateFromSerialized(keys, m_PreInfinity, m_PostInfinity) + FloatCurve::SetSerializedRange.
FloatCurve MakeSerializedFloatCurve(const std::vector<SerializedKeyframe>& keys, int preInfinity, int postInfinity,
                                    float minTime, float maxTime);

}  // namespace ksp
