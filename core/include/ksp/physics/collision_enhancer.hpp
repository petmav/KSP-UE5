// WP-20e: CollisionEnhancer (CollisionEnhancer.cs), KSP's anti-tunnelling component: every physical part (Part.Start adds one to each part
// with a Rigidbody, Part.cs:1884) linecasts from its last position to its current one against the terrain layer (32768) when it moved more
// than 0.1 m in a tick (seq 7 of the measured order, PHYSICS-ENGINE-REVIEW.md 1.2); a hit explodes the part when it is fast enough (its
// speed above crashTolerance, or against a rigid body the relative speed times that body's mass), otherwise puts it back 0.25 m above the
// hit point with its velocity reflected and scaled by 0.2. The decisions and the float statements are the C#'s (Mono rule); the scene
// access goes through `Env` (the contact host fills it from the bubble: part transforms, the cluster rigid bodies, PhysicsWorld::RayCast).
//
// kspcore frame note (changed): the C# subtracts FloatingOrigin.Offset from lastPos because KSP shifts the whole world every tick while
// Krakensbane is engaged. kspcore's world moves only when FloatingOrigin rebases (PhysicsBubble::ShiftWorld): `Env::worldShift` is that
// tick's shift (zero otherwise), and the Galilean frame drift is not a shift (the part transforms move at their true velocity).
#pragma once

#include <functional>
#include <string>

#include "ksp/math/vector3d.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {
class Part;
}

namespace ksp::physics {

// CollisionEnhancerBehaviour.cs
enum class CollisionEnhancerBehaviour : int { EXPLODE = 0, TRANSLATE_BACK = 1, TRANSLATE_BACK_SPLAT = 2, COLLIDE = 3, DO_NOTHING = 4 };

class CollisionEnhancer {
public:
    // CollisionEnhancer.cs:8-14 statics
    static inline bool bypass = false;
    static inline double UnderTerrainTolerance = 0.5;
    static inline float upFactor = 0.25f;
    static inline float minDistSqr = 0.010000001f;

    struct LinecastHit {
        bool hit = false;
        Vector3f point;
        Vector3f normal;
        bool hasRigidbody = false;     // hit.collider.attachedRigidbody != null
        Vector3f rigidbodyVelocity;    // its velocity
        float rigidbodyMass = 0.0f;
        std::string rigidbodyName;
    };
    struct Env {
        std::function<bool(Part&)> packed;                                    // part.packed (the vessel's)
        std::function<Vector3f(Part&)> position;                              // base.transform.position (world)
        std::function<void(Part&, const Vector3f&)> setPosition;              // base.transform.position = (moves the part's rigid body)
        std::function<Vector3f(Part&)> velocity;                              // rb.velocity
        std::function<void(Part&, const Vector3f&)> setVelocity;              // rb.velocity =
        std::function<LinecastHit(const Vector3f& from, const Vector3f& to)> linecast;  // Physics.Linecast(.., 32768) (terrain layer)
        std::function<Vector3f(const Vector3f& point)> upAxis;                // FlightGlobals.getUpAxis(FlightGlobals.currentMainBody, point)
        std::function<Vector3d()> frameVelocity;                              // Krakensbane.GetFrameVelocity()
        std::function<Vector3d()> worldShift;                                 // see the header comment (FloatingOrigin.Offset in KSP)
    };

    // CollisionEnhancer.cs:6-30 fields
    Vector3f lastPos;
    Part* part = nullptr;
    bool wasPacked = true;
    int framesToSkip = 0;
    float instanceMinDistSqr = 0.0f;
    CollisionEnhancerBehaviour OnTerrainPunchThrough = CollisionEnhancerBehaviour::EXPLODE;
    float translateBackVelocityFactor = 0.2f;

    // what the last FixedUpdate did (gates)
    bool lastLinecast = false;   // a linecast was cast
    bool lastHit = false;        // it hit (flag)
    bool lastExploded = false;   // part.explode() was called

    void Start(Part& p, const Env& env);   // :32
    void FixedUpdate(const Env& env);       // :47
};

}  // namespace ksp::physics
