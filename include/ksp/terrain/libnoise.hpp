// WP-11: the LibNoise subset KSP's PQS mods reach (Assembly-CSharp/LibNoise/, KSP's own C# port of libnoise; not the
// LGPL C++ original). Pure double-precision maths ported operation for operation; C# int arithmetic wraps (unchecked)
// and (int)double is dotnet::ConvToInt32 (cvttsd2si). Reachable files (analysis/tiering/third-party.md): IModule,
// NoiseQuality, Math, GradientNoiseBasis (incl. its 1024-entry RandomVectors table, generated from the C# source by
// tools/wp11/gen_wp11_terrain.py), ValueNoiseBasis, Perlin, Billow, RidgedMultifractal, Voronoi,
// Modifiers/ScaleBiasOutput, Modifiers/Select.
//
// Quirks kept: the floor used for lattice coordinates is `x > 0 ? (int)x : (int)x - 1` (so 0.0 -> -1);
// RidgedMultifractal hashes octave seeds with `(Seed + i) & 0x7FFFFFFF`, Perlin/Billow with `& 0xFFFFFFFF`;
// Select(lower, upper, falloff, a, b) sets EdgeFalloff before the bounds (so the falloff is clamped against 0 - 0) and
// leaves ControlModule null (GetValue throws NullReferenceException until it is set, as PQSMod_VertexNoise does).
#pragma once

#include <array>
#include <memory>

#include "ksp/math/vector3d.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::LibNoise {

// LibNoise/NoiseQuality.cs
enum class NoiseQuality : int { Low = 0, Standard = 1, High = 2 };

// LibNoise/IModule.cs
class IModule {
public:
    virtual ~IModule() = default;
    virtual double GetValue(double x, double y, double z) = 0;
    double GetValue(const Vector3d& coordinate) { return GetValue(coordinate.x, coordinate.y, coordinate.z); }
    double GetValue(const Vector3f& coordinate) { return GetValue(coordinate.x, coordinate.y, coordinate.z); }
};

// LibNoise/Math.cs
class Math {
public:
    static const double PI;          // Math.cs:7
    static const double Sqrt2;       // Math.cs:9
    static const double Sqrt3;       // Math.cs:11
    static const double DEG_TO_RAD;  // Math.cs:13
    static int ClampValue(int value, int lowerBound, int upperBound);  // Math.cs:15
    static double GetSmaller(double a, double b);                      // Math.cs:36
    static double GetLarger(double a, double b);                       // Math.cs:45
    static void SwapValues(double& a, double& b);                      // Math.cs:54

protected:
    static double CubicInterpolate(double n0, double n1, double n2, double n3, double a);  // Math.cs:28
    static double LinearInterpolate(double n0, double n1, double a);                      // Math.cs:61
    static double SCurve3(double a);                                                      // Math.cs:66
    static double SCurve5(double a);                                                      // Math.cs:71
    static void LatLonToXYZ(double lat, double lon, double& x, double& y, double& z);    // Math.cs:79
};

// LibNoise/GradientNoiseBasis.cs
class GradientNoiseBasis : public Math {
public:
    double GradientCoherentNoise(double x, double y, double z, int seed, NoiseQuality noiseQuality) const;
    static const std::array<double, 1024>& RandomVectors();

private:
    static double GradientNoise(double fx, double fy, double fz, int ix, int iy, int iz, int64_t seed);
};

// LibNoise/ValueNoiseBasis.cs
class ValueNoiseBasis {
public:
    int IntValueNoise(int x, int y, int z, int seed) const;
    double ValueNoise(int x, int y, int z) const;
    double ValueNoise(int x, int y, int z, int seed) const;
};

// LibNoise/Perlin.cs
class Perlin : public GradientNoiseBasis, public IModule {
public:
    double Frequency = 0.0;
    double Persistence = 0.0;
    NoiseQuality noiseQuality = NoiseQuality::Low;  // property NoiseQuality
    int Seed = 0;
    double Lacunarity = 0.0;

    Perlin();
    Perlin(double frequency, double lacunarity, double persistance, int octaves, int seed, NoiseQuality mode);
    int OctaveCount() const { return mOctaveCount; }
    void OctaveCount(int value);  // throws ksp::ArgumentException outside [1, 30]
    using IModule::GetValue;
    double GetValue(double x, double y, double z) override;

private:
    int mOctaveCount = 0;
};

// LibNoise/Billow.cs
class Billow : public GradientNoiseBasis, public IModule {
public:
    double Frequency = 0.0;
    double Persistence = 0.0;
    NoiseQuality noiseQuality = NoiseQuality::Low;
    int Seed = 0;
    double Lacunarity = 0.0;

    Billow();
    Billow(double frequency, double lacunarity, double persistance, int octaves, int seed, NoiseQuality quality);
    int OctaveCount() const { return mOctaveCount; }
    void OctaveCount(int value);
    using IModule::GetValue;
    double GetValue(double x, double y, double z) override;

private:
    int mOctaveCount = 0;
};

// LibNoise/RidgedMultifractal.cs
class RidgedMultifractal : public GradientNoiseBasis, public IModule {
public:
    double Frequency = 0.0;
    NoiseQuality noiseQuality = NoiseQuality::Low;
    int Seed = 0;

    RidgedMultifractal();
    RidgedMultifractal(double frequency, double lacunarity, int octaves, int seed, NoiseQuality mode);
    double Lacunarity() const { return mLacunarity; }
    void Lacunarity(double value);  // recomputes the spectral weights
    int OctaveCount() const { return mOctaveCount; }
    void OctaveCount(int value);
    using IModule::GetValue;
    double GetValue(double x, double y, double z) override;
    const std::array<double, 30>& SpectralWeights() const { return mSpectralWeights; }

private:
    void CalculateSpectralWeights();
    int mOctaveCount = 0;
    double mLacunarity = 0.0;
    std::array<double, 30> mSpectralWeights{};
};

// LibNoise/Voronoi.cs
class Voronoi : public ValueNoiseBasis, public IModule {
public:
    double Frequency = 0.0;
    double Displacement = 0.0;
    bool DistanceEnabled = false;
    int Seed = 0;

    Voronoi();
    Voronoi(double voronoiFrequency, double voronoiDisplacement, int voronoiSeed, bool distanceEnabled);
    using IModule::GetValue;
    double GetValue(double x, double y, double z) override;
    Vector3d GetNearest(const Vector3d& candidate) const { return GetNearest(candidate.x, candidate.y, candidate.z); }
    Vector3d GetNearest(double x, double y, double z) const;
};

namespace Modifiers {

// LibNoise/Modifiers/ScaleBiasOutput.cs (modules are shared: C# object references)
class ScaleBiasOutput : public IModule {
public:
    double Scale = 0.0;
    double Bias = 0.0;
    std::shared_ptr<IModule> SourceModule;

    explicit ScaleBiasOutput(std::shared_ptr<IModule> sourceModule);
    ScaleBiasOutput(double scale, double bias, std::shared_ptr<IModule> sourcemodule);
    using IModule::GetValue;
    double GetValue(double x, double y, double z) override;
};

// LibNoise/Modifiers/Select.cs
class Select : public Math, public IModule {
public:
    std::shared_ptr<IModule> ControlModule;
    std::shared_ptr<IModule> SourceModule1;
    std::shared_ptr<IModule> SourceModule2;

    Select(std::shared_ptr<IModule> control, std::shared_ptr<IModule> source1, std::shared_ptr<IModule> source2);
    Select(double lowerbound, double upperbound, double edgefalloff, std::shared_ptr<IModule> inputmodule1,
           std::shared_ptr<IModule> inputmodule2);
    double UpperBound() const { return mUpperBound; }
    double LowerBound() const { return mLowerBound; }
    double EdgeFalloff() const { return mEdgeFalloff; }
    void EdgeFalloff(double value);
    void SetBounds(double lower, double upper);  // Select.cs:106, throws ksp::ArgumentException if lower > upper
    using IModule::GetValue;
    double GetValue(double x, double y, double z) override;

private:
    double mEdgeFalloff = 0.0;
    double mUpperBound = 0.0;
    double mLowerBound = 0.0;
};

}  // namespace Modifiers

}  // namespace ksp::LibNoise
