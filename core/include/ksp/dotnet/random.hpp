// System.Random (Knuth subtractive generator) exactly as in Unity's Mono BCL (Mono/mscorlib/System/Random.cs; identical
// to .NET Framework's, CAPTURE-SPEC C7 sysrand: 0 differences) and KSP's own copy KSPRandom (Assembly-CSharp/KSPRandom.cs),
// which differs in its second index (31 instead of 21), its sample scale and its Next(min, max).
#pragma once

#include <cstdint>
#include <vector>

namespace ksp::dotnet {

class SystemRandom {
public:
    explicit SystemRandom(int32_t seed);
    virtual ~SystemRandom() = default;

    int32_t Next();
    int32_t Next(int32_t maxValue);                  // throws std::out_of_range if maxValue < 0
    int32_t Next(int32_t minValue, int32_t maxValue);  // throws std::out_of_range if minValue > maxValue
    double NextDouble();
    void NextBytes(std::vector<uint8_t>& buffer);

protected:
    virtual double Sample();
    int32_t InternalSample();

private:
    double GetSampleForLargeRange();
    int32_t inext_ = 0;
    int32_t inextp_ = 21;
    int32_t seedArray_[56] = {};
};

}  // namespace ksp::dotnet

namespace ksp {

// KSPRandom (KSPRandom.cs). KSPRandom(int.MinValue) throws OverflowException (Math.Abs) like the original.
class KSPRandom {
public:
    explicit KSPRandom(int32_t seed);

    int32_t Next();
    int32_t Next(int32_t maxValue);
    int32_t Next(int32_t minValue, int32_t maxValue);
    double NextDouble();
    double NextDouble(double minValue, double maxValue);
    void NextBytes(std::vector<uint8_t>& buffer);

private:
    double Sample();
    int32_t seedArray_[56] = {};
    int32_t inexta_ = 0;
    int32_t inextb_ = 31;
};

}  // namespace ksp
