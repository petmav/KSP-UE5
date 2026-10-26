// WP-12d: PQSSurfaceObject.cs (the base of PQSCity / PQSCity2: what PQS.GameBindings' city events and range queries receive).
#pragma once

#include <string>

#include "ksp/math/vector3d.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class PQS;

// PQSSurfaceObject.cs:1 (a PQSMod in C#, the base of PQSCity / PQSCity2; the space-center ports derive from this base, which carries the
// members the PQS side reads: the GameObject name, the sphere and the transform's local position)
class PQSSurfaceObject {
public:
    virtual ~PQSSurfaceObject() = default;
    std::string name;                     // gameObject.name
    PQS* sphere = nullptr;                // PQSMod.sphere
    Vector3f localPosition;               // transform.localPosition
    virtual std::string SurfaceObjectName() const { return std::string(); }          // PQSSurfaceObject.cs:3
    virtual std::string DisplaySurfaceObjectName() const { return std::string(); }   // :5
    virtual Vector3d PlanetRelativePosition() const { return Vector3d(localPosition); }   // :7
};

}  // namespace ksp
