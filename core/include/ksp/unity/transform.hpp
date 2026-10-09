// UnityEngine.Transform / GameObject as far as engine-free KSP code needs them: a world position (float). The real
// transforms live in the engine shell; this is the stand-in for objects the core creates itself (PositionTarget's
// `new GameObject()`), completing the opaque forward declaration in ksp/events/game_event_types.hpp.
#pragma once

#include "ksp/unity/vector.hpp"

namespace ksp::UnityEngine {

class Transform {
public:
    Vector3f position;  // Transform.position (world space)
};

}  // namespace ksp::UnityEngine
