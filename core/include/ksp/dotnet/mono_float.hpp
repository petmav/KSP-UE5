// How C# `float` arithmetic is evaluated in KSP (Unity 2019.4 Mono 5.11, x64, no float32 mode).
//
// CAPTURE-SPEC C7 (harness/captures/README.md, reports/C7_analysis.md, 10,000 inputs per probe):
//   * every float32 expression is evaluated in DOUBLE precision (each operation rounded to 53 bits);
//   * the value is rounded to float32 only when it is STORED: assignment to a float local, parameter, field, static,
//     struct field or array element; passing it as a float argument (also when the callee is inlined); returning it
//     from a float method (also inlined); or an explicit (float) cast (conv.r4);
//   * comparisons of float expressions happen in double (nothing is rounded before the compare);
//   * `double r = a * b` with float a, b gives the exact product.
// .NET Framework (the golden harness) instead rounds every float operation to float32, so its float32 goldens are not
// in-game truth; kspcore follows Mono and is checked against the in-game captures (harness/captures/out/*_mono.json).
//
// Porting pattern for every managed float32 code path (Unity's managed Mathf/Vector3/Quaternion, KSP's own float code):
// widen each float operand with fx(), write the C# expression in double, and round with f32store() exactly where the C#
// source stores, passes, returns or casts the value. Example (Mathf.Lerp, `return a + (b - a) * Clamp01(t);`):
//
//     return f32store(fx(a) + (fx(b) - fx(a)) * fx(Clamp01(t)));
//
// Native Unity code (AnimationCurve, Random, Quaternion.Euler/AngleAxis/LookRotation...) is C++ with per-operation
// float32 arithmetic and must NOT use this pattern. Known limit: Mono also rounds where its JIT spills a temporary
// (the C7 `locals` probe); following the C# source's own locals reproduces that for the code checked so far.
#pragma once

namespace ksp {

// A float32 operand inside a C# float expression (exact widening).
constexpr double fx(float v) noexcept { return static_cast<double>(v); }

// Round a C# float expression at a store boundary (local/field/array store, float argument, float return, (float)).
constexpr float f32store(double v) noexcept { return static_cast<float>(v); }

}  // namespace ksp
