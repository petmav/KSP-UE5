// UnityEngine.Transform / GameObject as far as engine-free KSP code needs them: the GameObject name, a world position and
// rotation (float). The real transforms live in the engine shell; this is the stand-in for objects the core creates itself
// (PositionTarget's `new GameObject()`, FlightCoMTracker's marker) and for the transform an ITargetable hands out
// (Vessel.GetTransform mirrors its ReferenceTransform here). Completes the opaque forward declaration in
// ksp/events/game_event_types.hpp.
#pragma once

#include <string>

#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::UnityEngine {

class Transform {
public:
    std::string name;                                   // Object.name (the GameObject's name)
    Vector3f position;                                  // Transform.position (world space)
    Quaternionf rotation = Quaternionf::identity();     // Transform.rotation (world space)
};

}  // namespace ksp::UnityEngine
