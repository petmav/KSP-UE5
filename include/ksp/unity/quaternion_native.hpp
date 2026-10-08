// Native (UnityPlayer.dll) Quaternion functions needed by ported KSP code, implemented as per-operation float32 C++
// (no Mono float rule: this is native code). Spec: Unity 2019.4 Runtime/Math/Quaternion.h AxisAngleToQuaternionSafe,
// checked bit-for-bit against the in-game capture harness/captures/out/math_shim_native.json (CAPTURE-SPEC C3,
// fn = "Quaternion.AngleAxis", 84 rows) by tests/test_orbit.cpp.
#pragma once

#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::UnityNative {

// UnityEngine.Quaternion.AngleAxis(float angle /*degrees*/, Vector3 axis): identity when |axis| <= 1e-6. Unity's
// Deg2Rad is `deg / 360 * 2 * kPI` (not deg * kDeg2Rad); with deg * (pi/180) 18 of the 84 captured rows differ.
Quaternionf AngleAxis(float angle, const Vector3f& axis);

}  // namespace ksp::UnityNative
