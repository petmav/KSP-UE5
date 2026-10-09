// WP-11: port of Assembly-CSharp/LibNoise/{Math,GradientNoiseBasis,ValueNoiseBasis,Perlin,Billow,RidgedMultifractal,
// Voronoi,Modifiers/ScaleBiasOutput,Modifiers/Select}.cs (KSP's own C# LibNoise). See include/ksp/terrain/libnoise.hpp.
#include "ksp/terrain/libnoise.hpp"

#include <cmath>
#include <cstdint>
#include <string>

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"

namespace ksp::LibNoise {

namespace {

// C# unchecked int arithmetic (two's complement wrap-around)
inline int32_t Wrap(int64_t v) { return static_cast<int32_t>(static_cast<uint32_t>(static_cast<uint64_t>(v))); }
inline int32_t AddI(int32_t a, int32_t b) { return static_cast<int32_t>(static_cast<uint32_t>(a) + static_cast<uint32_t>(b)); }
inline int32_t MulI(int32_t a, int32_t b) { return static_cast<int32_t>(static_cast<uint32_t>(a) * static_cast<uint32_t>(b)); }

// `x > 0.0 ? (int)x : (int)x - 1` (GradientNoiseBasis.cs:124, Voronoi.cs:47); (int)x is conv.i4 (cvttsd2si)
inline int LatticeFloor(double x) { return x > 0.0 ? dotnet::ConvToInt32(x) : AddI(dotnet::ConvToInt32(x), -1); }

// GradientNoiseBasis.cs:15 RandomVectors (generated from the C# initialiser by tools/wp11/gen_wp11_terrain.py)
constexpr std::array<double, 1024> kRandomVectors = {
#include "libnoise_random_vectors.inc"
};

void CheckOctaves(int value) {
    // Perlin.cs:28 / Billow.cs:28 / RidgedMultifractal.cs:41
    if (value < 1 || value > 30)
        throw ArgumentException("Octave count must be greater than zero and less than " + std::to_string(30));
}

}  // namespace

// ---- Math.cs ------------------------------------------------------------------------------------------------------
const double Math::PI = 3.141592653589793;           // Math.cs:7 System.Math.PI
const double Math::Sqrt2 = 1.4142135623730951;       // Math.cs:9
const double Math::Sqrt3 = 1.7320508075688772;       // Math.cs:11
const double Math::DEG_TO_RAD = 3.141592653589793 / 180.0;  // Math.cs:13 PI / 180.0

// Math.cs:15
int Math::ClampValue(int value, int lowerBound, int upperBound) {
    if (value < lowerBound) return lowerBound;
    if (value > upperBound) return upperBound;
    return value;
}

// Math.cs:28
double Math::CubicInterpolate(double n0, double n1, double n2, double n3, double a) {
    const double num = n3 - n2 - (n0 - n1);
    const double num2 = n0 - n1 - num;
    const double num3 = n2 - n0;
    return num * a * a * a + num2 * a * a + num3 * a + n1;
}

// Math.cs:36
double Math::GetSmaller(double a, double b) { return !(a < b) ? b : a; }

// Math.cs:45
double Math::GetLarger(double a, double b) { return !(a > b) ? b : a; }

// Math.cs:54
void Math::SwapValues(double& a, double& b) {
    const double num = a;
    a = b;
    b = num;
}

// Math.cs:61
double Math::LinearInterpolate(double n0, double n1, double a) { return (1.0 - a) * n0 + a * n1; }

// Math.cs:66
double Math::SCurve3(double a) { return a * a * (3.0 - 2.0 * a); }

// Math.cs:71
double Math::SCurve5(double a) {
    const double num = a * a * a;
    const double num2 = num * a;
    const double num3 = num2 * a;
    return 6.0 * num3 - 15.0 * num2 + 10.0 * num;
}

// Math.cs:79 (System.Math.Cos/Sin: the runtime's x87 functions)
void Math::LatLonToXYZ(double lat, double lon, double& x, double& y, double& z) {
    const double num = dotnet::Math::Cos(DEG_TO_RAD * lat);
    x = num * dotnet::Math::Cos(DEG_TO_RAD * lon);
    y = dotnet::Math::Sin(DEG_TO_RAD * lat);
    z = num * dotnet::Math::Sin(DEG_TO_RAD * lon);
}

// ---- GradientNoiseBasis.cs ------------------------------------------------------------------------------------------
const std::array<double, 1024>& GradientNoiseBasis::RandomVectors() { return kRandomVectors; }

// GradientNoiseBasis.cs:122
double GradientNoiseBasis::GradientCoherentNoise(double x, double y, double z, int seed, NoiseQuality noiseQuality) const {
    const int num = LatticeFloor(x);
    const int ix = AddI(num, 1);
    const int num2 = LatticeFloor(y);
    const int iy = AddI(num2, 1);
    const int num3 = LatticeFloor(z);
    const int iz = AddI(num3, 1);
    double a = 0.0;
    double a2 = 0.0;
    double a3 = 0.0;
    switch (noiseQuality) {
    case NoiseQuality::Low:
        a = x - static_cast<double>(num);
        a2 = y - static_cast<double>(num2);
        a3 = z - static_cast<double>(num3);
        break;
    case NoiseQuality::Standard:
        a = SCurve3(x - static_cast<double>(num));
        a2 = SCurve3(y - static_cast<double>(num2));
        a3 = SCurve3(z - static_cast<double>(num3));
        break;
    case NoiseQuality::High:
        a = SCurve5(x - static_cast<double>(num));
        a2 = SCurve5(y - static_cast<double>(num2));
        a3 = SCurve5(z - static_cast<double>(num3));
        break;
    }
    double n = GradientNoise(x, y, z, num, num2, num3, seed);
    double n2 = GradientNoise(x, y, z, ix, num2, num3, seed);
    double n3 = LinearInterpolate(n, n2, a);
    n = GradientNoise(x, y, z, num, iy, num3, seed);
    n2 = GradientNoise(x, y, z, ix, iy, num3, seed);
    double n4 = LinearInterpolate(n, n2, a);
    const double n5 = LinearInterpolate(n3, n4, a2);
    n = GradientNoise(x, y, z, num, num2, iz, seed);
    n2 = GradientNoise(x, y, z, ix, num2, iz, seed);
    n3 = LinearInterpolate(n, n2, a);
    n = GradientNoise(x, y, z, num, iy, iz, seed);
    n2 = GradientNoise(x, y, z, ix, iy, iz, seed);
    n4 = LinearInterpolate(n, n2, a);
    const double n6 = LinearInterpolate(n3, n4, a2);
    return LinearInterpolate(n5, n6, a3);
}

// GradientNoiseBasis.cs:168: (1619 * ix + 31337 * iy + 6971 * iz) is int arithmetic (wraps), + 1013L * seed is long;
// only the low 32 bits survive the mask, so the whole sum is taken modulo 2^32.
double GradientNoiseBasis::GradientNoise(double fx, double fy, double fz, int ix, int iy, int iz, int64_t seed) {
    const int32_t ipart = AddI(AddI(MulI(1619, ix), MulI(31337, iy)), MulI(6971, iz));
    int64_t num = (static_cast<int64_t>(ipart) + 1013LL * seed) & 0xFFFFFFFFLL;
    num ^= num >> 8;
    num &= 0xFFLL;
    const double num2 = kRandomVectors[static_cast<size_t>(num << 2)];
    const double num3 = kRandomVectors[static_cast<size_t>((num << 2) + 1)];
    const double num4 = kRandomVectors[static_cast<size_t>((num << 2) + 2)];
    const double num5 = fx - static_cast<double>(ix);
    const double num6 = fy - static_cast<double>(iy);
    const double num7 = fz - static_cast<double>(iz);
    return (num2 * num5 + num3 * num6 + num4 * num7) * 2.12;
}

// ---- ValueNoiseBasis.cs ---------------------------------------------------------------------------------------------
// ValueNoiseBasis.cs:15 (all int arithmetic, unchecked)
int ValueNoiseBasis::IntValueNoise(int x, int y, int z, int seed) const {
    int32_t num = AddI(AddI(AddI(MulI(1619, x), MulI(31337, y)), MulI(6971, z)), MulI(1013, seed)) & 0x7FFFFFFF;
    num = (num >> 13) ^ num;
    return AddI(MulI(num, AddI(MulI(MulI(num, num), 60493), 19990303)), 1376312589) & 0x7FFFFFFF;
}

// ValueNoiseBasis.cs:22
double ValueNoiseBasis::ValueNoise(int x, int y, int z) const { return ValueNoise(x, y, z, 0); }

// ValueNoiseBasis.cs:27
double ValueNoiseBasis::ValueNoise(int x, int y, int z, int seed) const {
    return 1.0 - static_cast<double>(IntValueNoise(x, y, z, seed)) / 1073741824.0;
}

// ---- Perlin.cs ------------------------------------------------------------------------------------------------------
// Perlin.cs:38
Perlin::Perlin() {
    Frequency = 1.0;
    Lacunarity = 2.0;
    OctaveCount(6);
    Persistence = 0.5;
    noiseQuality = NoiseQuality::Standard;
    Seed = 0;
}

// Perlin.cs:48
Perlin::Perlin(double frequency, double lacunarity, double persistance, int octaves, int seed, NoiseQuality mode) {
    Frequency = frequency;
    Lacunarity = lacunarity;
    OctaveCount(octaves);
    Persistence = persistance;
    noiseQuality = mode;
    Seed = seed;
}

void Perlin::OctaveCount(int value) {
    CheckOctaves(value);
    mOctaveCount = value;
}

// Perlin.cs:68
double Perlin::GetValue(double x, double y, double z) {
    double num = 0.0;
    double num2 = 0.0;
    double num3 = 1.0;
    x *= Frequency;
    y *= Frequency;
    z *= Frequency;
    for (int i = 0; i < OctaveCount(); i++) {
        const int64_t num4 = static_cast<int64_t>(AddI(Seed, i)) & 0xFFFFFFFFLL;  // (Seed + i) & 0xFFFFFFFFL
        num2 = GradientCoherentNoise(x, y, z, Wrap(num4), noiseQuality);
        num += num2 * num3;
        x *= Lacunarity;
        y *= Lacunarity;
        z *= Lacunarity;
        num3 *= Persistence;
    }
    return num;
}

// ---- Billow.cs ------------------------------------------------------------------------------------------------------
// Billow.cs:38
Billow::Billow() {
    Frequency = 1.0;
    Lacunarity = 2.0;
    OctaveCount(6);
    Persistence = 0.5;
    noiseQuality = NoiseQuality::Standard;
    Seed = 0;
}

// Billow.cs:48
Billow::Billow(double frequency, double lacunarity, double persistance, int octaves, int seed, NoiseQuality quality) {
    Frequency = frequency;
    Lacunarity = lacunarity;
    OctaveCount(octaves);
    Persistence = persistance;
    noiseQuality = quality;
    Seed = seed;
}

void Billow::OctaveCount(int value) {
    CheckOctaves(value);
    mOctaveCount = value;
}

// Billow.cs:68 (System.Math.Abs)
double Billow::GetValue(double x, double y, double z) {
    double num = 0.0;
    double num2 = 0.0;
    double num3 = 1.0;
    x *= Frequency;
    y *= Frequency;
    z *= Frequency;
    for (int i = 0; i < OctaveCount(); i++) {
        const int64_t num4 = static_cast<int64_t>(AddI(Seed, i)) & 0xFFFFFFFFLL;
        num2 = GradientCoherentNoise(x, y, z, Wrap(num4), noiseQuality);
        num2 = 2.0 * std::fabs(num2) - 1.0;
        num += num2 * num3;
        x *= Lacunarity;
        y *= Lacunarity;
        z *= Lacunarity;
        num3 *= Persistence;
    }
    return num + 0.5;
}

// ---- RidgedMultifractal.cs ------------------------------------------------------------------------------------------
// RidgedMultifractal.cs:51
RidgedMultifractal::RidgedMultifractal() {
    Frequency = 1.0;
    Lacunarity(2.0);
    OctaveCount(6);
    noiseQuality = NoiseQuality::Standard;
    Seed = 0;
}

// RidgedMultifractal.cs:60
RidgedMultifractal::RidgedMultifractal(double frequency, double lacunarity, int octaves, int seed, NoiseQuality mode) {
    Frequency = frequency;
    Lacunarity(lacunarity);
    OctaveCount(octaves);
    noiseQuality = mode;
    Seed = seed;
}

// RidgedMultifractal.cs:25 (setter)
void RidgedMultifractal::Lacunarity(double value) {
    mLacunarity = value;
    CalculateSpectralWeights();
}

void RidgedMultifractal::OctaveCount(int value) {
    CheckOctaves(value);
    mOctaveCount = value;
}

// RidgedMultifractal.cs:79 (System.Math.Abs)
double RidgedMultifractal::GetValue(double x, double y, double z) {
    x *= Frequency;
    y *= Frequency;
    z *= Frequency;
    double num = 0.0;
    double num2 = 0.0;
    double num3 = 1.0;
    const double num4 = 1.0;
    const double num5 = 2.0;
    for (int i = 0; i < OctaveCount(); i++) {
        const int64_t num6 = static_cast<int64_t>(AddI(Seed, i) & 0x7FFFFFFF);  // (Seed + i) & 0x7FFFFFFF
        num = GradientCoherentNoise(x, y, z, Wrap(num6), noiseQuality);
        num = std::fabs(num);
        num = num4 - num;
        num *= num;
        num *= num3;
        num3 = num * num5;
        if (num3 > 1.0) num3 = 1.0;
        if (num3 < 0.0) num3 = 0.0;
        num2 += num * mSpectralWeights[static_cast<size_t>(i)];
        x *= mLacunarity;
        y *= mLacunarity;
        z *= mLacunarity;
    }
    return num2 * 1.25 - 1.0;
}

// RidgedMultifractal.cs:114 (System.Math.Pow: the runtime's CRT pow)
void RidgedMultifractal::CalculateSpectralWeights() {
    const double num = 1.0;
    double num2 = 1.0;
    for (int i = 0; i < 30; i++) {
        mSpectralWeights[static_cast<size_t>(i)] = dotnet::Math::Pow(num2, 0.0 - num);
        num2 *= mLacunarity;
    }
}

// ---- Voronoi.cs -----------------------------------------------------------------------------------------------------
// Voronoi.cs:16
Voronoi::Voronoi() {
    Frequency = 1.0;
    Displacement = 1.0;
    Seed = 0;
    DistanceEnabled = false;
}

// Voronoi.cs:24
Voronoi::Voronoi(double voronoiFrequency, double voronoiDisplacement, int voronoiSeed, bool distanceEnabled) {
    Frequency = voronoiFrequency;
    Displacement = voronoiDisplacement;
    Seed = voronoiSeed;
    DistanceEnabled = distanceEnabled;
}

// Voronoi.cs:42 (System.Math.Sqrt)
double Voronoi::GetValue(double x, double y, double z) {
    x *= Frequency;
    y *= Frequency;
    z *= Frequency;
    const int num = LatticeFloor(x);
    const int num2 = LatticeFloor(y);
    const int num3 = LatticeFloor(z);
    double num4 = 2147483647.0;
    double num5 = 0.0;
    double num6 = 0.0;
    double num7 = 0.0;
    for (int i = AddI(num3, -2); i <= AddI(num3, 2); i++) {
        for (int j = AddI(num2, -2); j <= AddI(num2, 2); j++) {
            for (int k = AddI(num, -2); k <= AddI(num, 2); k++) {
                const double num8 = static_cast<double>(k) + ValueNoise(k, j, i, Seed);
                const double num9 = static_cast<double>(j) + ValueNoise(k, j, i, AddI(Seed, 1));
                const double num10 = static_cast<double>(i) + ValueNoise(k, j, i, AddI(Seed, 2));
                const double num11 = num8 - x;
                const double num12 = num9 - y;
                const double num13 = num10 - z;
                const double num14 = num11 * num11 + num12 * num12 + num13 * num13;
                if (num14 < num4) {
                    num4 = num14;
                    num5 = num8;
                    num6 = num9;
                    num7 = num10;
                }
            }
        }
    }
    double num18;
    if (DistanceEnabled) {
        const double num15 = num5 - x;
        const double num16 = num6 - y;
        const double num17 = num7 - z;
        num18 = dotnet::Math::Sqrt(num15 * num15 + num16 * num16 + num17 * num17) * Math::Sqrt3 - 1.0;
    } else {
        num18 = 0.0;
    }
    const int x2 = LatticeFloor(num5);
    const int y2 = LatticeFloor(num6);
    const int z2 = LatticeFloor(num7);
    return num18 + Displacement * ValueNoise(x2, y2, z2);
}

// Voronoi.cs:100
Vector3d Voronoi::GetNearest(double x, double y, double z) const {
    x *= Frequency;
    y *= Frequency;
    z *= Frequency;
    const int num = LatticeFloor(x);
    const int num2 = LatticeFloor(y);
    const int num3 = LatticeFloor(z);
    double num4 = 2147483647.0;
    double x2 = 0.0;
    double y2 = 0.0;
    double z2 = 0.0;
    for (int i = AddI(num3, -2); i <= AddI(num3, 2); i++) {
        for (int j = AddI(num2, -2); j <= AddI(num2, 2); j++) {
            for (int k = AddI(num, -2); k <= AddI(num, 2); k++) {
                const double num5 = static_cast<double>(k) + ValueNoise(k, j, i, Seed);
                const double num6 = static_cast<double>(j) + ValueNoise(k, j, i, AddI(Seed, 1));
                const double num7 = static_cast<double>(i) + ValueNoise(k, j, i, AddI(Seed, 2));
                const double num8 = num5 - x;
                const double num9 = num6 - y;
                const double num10 = num7 - z;
                const double num11 = num8 * num8 + num9 * num9 + num10 * num10;
                if (num11 < num4) {
                    num4 = num11;
                    x2 = num5;
                    y2 = num6;
                    z2 = num7;
                }
            }
        }
    }
    return Vector3d(x2, y2, z2);
}

namespace Modifiers {

// ScaleBiasOutput.cs:14
ScaleBiasOutput::ScaleBiasOutput(std::shared_ptr<IModule> sourceModule) {
    if (!sourceModule) throw ArgumentNullException("A source module must be provided.");
    SourceModule = std::move(sourceModule);
    Bias = 0.0;
    Scale = 1.0;
}

// ScaleBiasOutput.cs:25
ScaleBiasOutput::ScaleBiasOutput(double scale, double bias, std::shared_ptr<IModule> sourcemodule) {
    if (!sourcemodule) throw ArgumentNullException("A source module must be provided.");
    SourceModule = std::move(sourcemodule);
    Bias = bias;
    Scale = scale;
}

// ScaleBiasOutput.cs:46
double ScaleBiasOutput::GetValue(double x, double y, double z) {
    if (!SourceModule) throw Exception("A source module must be provided.");
    return SourceModule->GetValue(x, y, z) * Scale + Bias;
}

// Select.cs:22 (EdgeFalloff setter)
void Select::EdgeFalloff(double value) {
    const double num = mUpperBound - mLowerBound;
    mEdgeFalloff = (value > num / 2.0) ? (num / 2.0) : value;
}

// Select.cs:33
Select::Select(std::shared_ptr<IModule> control, std::shared_ptr<IModule> source1, std::shared_ptr<IModule> source2) {
    if (!control || !source1 || !source2) throw ArgumentNullException("Control and source modules must be provided.");
    ControlModule = std::move(control);
    SourceModule1 = std::move(source1);
    SourceModule2 = std::move(source2);
    EdgeFalloff(0.0);
    mLowerBound = -1.0;
    mUpperBound = 1.0;
}

// Select.cs:47 (sic: EdgeFalloff is clamped while both bounds are still 0; ControlModule stays null)
Select::Select(double lowerbound, double upperbound, double edgefalloff, std::shared_ptr<IModule> inputmodule1,
               std::shared_ptr<IModule> inputmodule2) {
    EdgeFalloff(edgefalloff);
    mLowerBound = lowerbound;
    mUpperBound = upperbound;
    SourceModule1 = std::move(inputmodule1);
    SourceModule2 = std::move(inputmodule2);
}

// Select.cs:66
double Select::GetValue(double x, double y, double z) {
    if (ControlModule && SourceModule1 && SourceModule2) {
        const double value = ControlModule->GetValue(x, y, z);
        if (mEdgeFalloff > 0.0) {
            if (value < mLowerBound - mEdgeFalloff) return SourceModule1->GetValue(x, y, z);
            if (value < mLowerBound + mEdgeFalloff) {
                const double num = mLowerBound - mEdgeFalloff;
                const double num2 = mLowerBound + mEdgeFalloff;
                const double a = SCurve3((value - num) / (num2 - num));
                // C# evaluates the arguments left to right: SourceModule1 first
                const double s1 = SourceModule1->GetValue(x, y, z);
                const double s2 = SourceModule2->GetValue(x, y, z);
                return LinearInterpolate(s1, s2, a);
            }
            if (value < mUpperBound - mEdgeFalloff) return SourceModule2->GetValue(x, y, z);
            if (value < mUpperBound + mEdgeFalloff) {
                const double num3 = mUpperBound - mEdgeFalloff;
                const double num4 = mUpperBound + mEdgeFalloff;
                const double a = SCurve3((value - num3) / (num4 - num3));
                const double s2 = SourceModule2->GetValue(x, y, z);
                const double s1 = SourceModule1->GetValue(x, y, z);
                return LinearInterpolate(s2, s1, a);
            }
            return SourceModule1->GetValue(x, y, z);
        }
        if (!(value < mLowerBound) && value <= mUpperBound) return SourceModule2->GetValue(x, y, z);
        return SourceModule1->GetValue(x, y, z);
    }
    throw NullReferenceException("Control and source modules must be provided.");
}

// Select.cs:106
void Select::SetBounds(double lower, double upper) {
    if (lower > upper) throw ArgumentException("The lower bounds must be lower than the upper bounds.");
    mLowerBound = lower;
    mUpperBound = upper;
    EdgeFalloff(mEdgeFalloff);
}

}  // namespace Modifiers

}  // namespace ksp::LibNoise
