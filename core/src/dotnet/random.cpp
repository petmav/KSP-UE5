#include "ksp/dotnet/random.hpp"

#include <cstdlib>
#include <stdexcept>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"

namespace ksp::dotnet {

namespace {
constexpr int32_t MBIG = 2147483647;
constexpr int32_t MSEED = 161803398;

// Shared seeding of the subtractive generator (Random.cs ctor / KSPRandom.cs ctor), int32 wrap-around arithmetic.
void Seed(int32_t (&seedArray)[56], int32_t mj) {
    seedArray[55] = mj;
    int32_t mk = 1;
    for (int i = 1; i < 55; ++i) {
        const int ii = (21 * i) % 55;
        seedArray[ii] = mk;
        mk = static_cast<int32_t>(static_cast<uint32_t>(mj) - static_cast<uint32_t>(mk));
        if (mk < 0) mk += MBIG;
        mj = seedArray[ii];
    }
    for (int k = 1; k < 5; ++k) {
        for (int i = 1; i < 56; ++i) {
            seedArray[i] -= seedArray[1 + (i + 30) % 55];
            if (seedArray[i] < 0) seedArray[i] += MBIG;
        }
    }
}
}  // namespace

// Random.cs: Random(int Seed)
SystemRandom::SystemRandom(int32_t seed) {
    const int32_t subtraction = (seed == INT32_MIN) ? INT32_MAX : std::abs(seed);
    Seed(seedArray_, MSEED - subtraction);
    inext_ = 0;
    inextp_ = 21;
}

// Random.cs: InternalSample
int32_t SystemRandom::InternalSample() {
    int32_t locINext = inext_;
    int32_t locINextp = inextp_;
    if (++locINext >= 56) locINext = 1;
    if (++locINextp >= 56) locINextp = 1;
    int32_t retVal = seedArray_[locINext] - seedArray_[locINextp];
    if (retVal == MBIG) retVal--;
    if (retVal < 0) retVal += MBIG;
    seedArray_[locINext] = retVal;
    inext_ = locINext;
    inextp_ = locINextp;
    return retVal;
}

// Random.cs: Sample
double SystemRandom::Sample() { return static_cast<double>(InternalSample()) * 4.656612875245797E-10; }

int32_t SystemRandom::Next() { return InternalSample(); }

// Random.cs: GetSampleForLargeRange
double SystemRandom::GetSampleForLargeRange() {
    int32_t result = InternalSample();
    if (InternalSample() % 2 == 0) result = -result;
    double d = result;
    d += (2147483647 - 1);
    d /= 2.0 * 2147483647 - 1;
    return d;
}

// Random.cs: Next(int minValue, int maxValue)
int32_t SystemRandom::Next(int32_t minValue, int32_t maxValue) {
    if (minValue > maxValue) throw std::out_of_range("Random.Next: minValue > maxValue");
    const int64_t range = static_cast<int64_t>(maxValue) - minValue;
    if (range <= INT32_MAX) return ConvToInt32(Sample() * static_cast<double>(range)) + minValue;
    return static_cast<int32_t>(ConvToInt64(GetSampleForLargeRange() * static_cast<double>(range)) + minValue);
}

// Random.cs: Next(int maxValue)
int32_t SystemRandom::Next(int32_t maxValue) {
    if (maxValue < 0) throw std::out_of_range("Random.Next: maxValue < 0");
    return ConvToInt32(Sample() * static_cast<double>(maxValue));
}

double SystemRandom::NextDouble() { return Sample(); }

void SystemRandom::NextBytes(std::vector<uint8_t>& buffer) {
    for (auto& b : buffer) b = static_cast<uint8_t>(InternalSample() % 256);
}

}  // namespace ksp::dotnet

namespace ksp {

using dotnet::ConvToInt32;

// KSPRandom.cs: KSPRandom(int Seed)
KSPRandom::KSPRandom(int32_t seed) {
    if (seed == INT32_MIN) throw dotnet::OverflowException("Negating the minimum value of a twos complement number is invalid.");
    dotnet::Seed(seedArray_, dotnet::MSEED - std::abs(seed));
    inexta_ = 0;
    inextb_ = 31;
}

// KSPRandom.cs: Sample
double KSPRandom::Sample() {
    if (++inexta_ >= 56) inexta_ = 1;
    if (++inextb_ >= 56) inextb_ = 1;
    int32_t num = seedArray_[inexta_] - seedArray_[inextb_];
    if (num < 0) num += dotnet::MBIG;
    seedArray_[inexta_] = num;
    return static_cast<double>(num) * 4.6566128752458E-10;
}

// KSPRandom.cs: Next()
int32_t KSPRandom::Next() { return ConvToInt32(Sample() * 2147483647.0); }

// KSPRandom.cs: Next(int maxValue)
int32_t KSPRandom::Next(int32_t maxValue) {
    if (maxValue < 0) throw std::out_of_range("Max value is less than min value.");
    return ConvToInt32(Sample() * static_cast<double>(maxValue));
}

// KSPRandom.cs: Next(int minValue, int maxValue): (int)((uint)(Sample() * num) + minValue), computed as long
int32_t KSPRandom::Next(int32_t minValue, int32_t maxValue) {
    if (minValue > maxValue) throw std::out_of_range("Min value is greater than max value.");
    const uint32_t num = static_cast<uint32_t>(maxValue) - static_cast<uint32_t>(minValue);
    if (num <= 1) return minValue;
    const int64_t r = static_cast<int64_t>(dotnet::ConvToUInt32(Sample() * static_cast<double>(num))) + minValue;
    return static_cast<int32_t>(static_cast<uint32_t>(static_cast<uint64_t>(r)));
}

double KSPRandom::NextDouble() { return Sample(); }

// KSPRandom.cs: NextDouble(min, max)
double KSPRandom::NextDouble(double minValue, double maxValue) {
    if (minValue > maxValue) {
        log::Error("[KSPRandom]: Min value is greater than max value.");
        return 0.0;
    }
    const double num = maxValue - minValue;
    if (num <= 1.0) return minValue;
    return Sample() * num + minValue;
}

// KSPRandom.cs: NextBytes
void KSPRandom::NextBytes(std::vector<uint8_t>& buffer) {
    for (auto& b : buffer) b = dotnet::ConvToByte(Sample() * 256.0);
}

}  // namespace ksp
