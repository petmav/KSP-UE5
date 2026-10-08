// Simplex.cs (instance, octave noise) and SimplexNoise.cs (static): Gustavson's 3D simplex noise in double precision.
//
// Quirks kept: fastfloor(x) is (int)x - 1 for every x <= 0 (so fastfloor(0) = -1 and fastfloor(-1) = -2); the 256-entry
// permutation table `p` is a STATIC array per class: Simplex.seed rewrites the table shared by all Simplex instances
// (each instance copies it into its own perm[512] when constructed or seeded) and reseeds UnityEngine.Random
// (Random.InitState(seed), then p[0..254] = Random.Range(0, 255); p[255] keeps its value); SimplexNoise.seed draws from
// the current global UnityEngine.Random state without reseeding it. noise() with octaves <= 0 is 0/0 = NaN.
// Process-wide state, not thread-safe (like KSP).
#pragma once

#include <array>

#include "ksp/math/vector3d.hpp"

namespace ksp {

class Simplex {
public:
    double octaves = 0.0;
    double persistence = 0.0;
    double frequency = 0.0;

    Simplex();                                                                       // Simplex.cs:168
    explicit Simplex(int seed);                                                      // Simplex.cs:173
    Simplex(int seed, double octaves, double persistence, double frequency);         // Simplex.cs:178

    // Simplex.cs:155 `public int seed { set; }`
    void seed(int value);

    double noiseNormalized(const Vector3d& v3d);                 // Simplex.cs:350
    double noise(const Vector3d& v3d);                           // Simplex.cs:355
    double noiseNormalized(double x, double y, double z);        // Simplex.cs:360
    double noise(double x, double y, double z);                  // Simplex.cs:365

    // private static int[] p (shared by every instance)
    static std::array<int, 256>& StaticPermutation();

private:
    void SetupPermTable();                                  // Simplex.cs:186
    double value(double xin, double yin, double zin) const;  // Simplex.cs:219
    std::array<int, 512> perm{};
};

namespace SimplexNoise {
void seed(int value);                                                                              // SimplexNoise.cs:105
double noise(double xin, double yin, double zin);                                                  // SimplexNoise.cs:199
double noise(const Vector3d& v3d);                                                                 // SimplexNoise.cs:330
double noise(double octaves, double persistence, double frequency, double x, double y, double z);  // SimplexNoise.cs:335
double noise(double octaves, double persistence, double frequency, const Vector3d& v3d);           // SimplexNoise.cs:350
std::array<int, 256>& StaticPermutation();  // private static int[] p
}  // namespace SimplexNoise

}  // namespace ksp
