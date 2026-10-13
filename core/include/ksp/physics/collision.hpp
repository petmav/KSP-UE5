// WP-20e: UnityEngine.Collision as KSP's Part reads it, and the statics the Part collision code reaches (CheatOptions, the FX and the
// wheel-damage hand-off). The contact host (ksp/physics/contact_host.hpp) builds a Collision from each PhysicsWorld contact event and calls
// Part.OnCollisionEnter / OnCollisionStay / OnCollisionExit (Part.cs:3355-3518, 3941; src/parts/part_contact.cpp) on the part whose
// Rigidbody owns the touching collider, as PhysX / Unity dispatched them.
//
// What the C# reads from a Collision `c` and where it lives here:
//   c.collider                         `collider` (the OTHER collider: the key of Part.currentCollisions, with its GameObject layer)
//   c.collider.enabled / .isTrigger / .gameObject.activeInHierarchy        `colliderEnabled` / `colliderIsTrigger` / `colliderActive`
//   c.collider.attachedRigidbody (.mass)  `otherHasRigidbody` / `otherRigidbodyMass` (the rb.mass of the part whose Rigidbody owns it)
//   c.gameObject.layer / .tag / .name  `layer` (15 Local Scenery: terrain and KSC, 0 Default: parts) / `tag` / `name`
//   c.gameObject.GetComponent<PQ>()    `isPQ` / `pqBody` (a terrain quad and the body of its sphere)
//   FlightGlobals.GetPartUpwardsCached(c.gameObject)   `otherPart` (the part whose collider was hit)
//   Part.GetComponentUpwards<CrashObjectName>(c.gameObject)  `crashObjectName` (KSC buildings carry one; empty: none)
//   c.relativeVelocity                 `relativeVelocity` (only magnitudes are read by the ported code; sign: this body minus the other)
//   c.contacts[i].thisCollider / .otherCollider / .normal / .point          `contacts` (the part each collider belongs to)
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class CelestialBody;

namespace UnityEngine {
// UnityEngine.RaycastHit as the events carry it (GameEvents.OnCollisionEnhancerHit; forward-declared in game_event_types.hpp)
struct RaycastHit {
    Vector3f point;
    Vector3f normal;
    float distance = 0.0f;
};

// A Unity Collider as a dictionary key (Part.currentCollisions is a DictionaryValueList<Collider, int>): an id unique per collider and
// the layer of its GameObject (CheckOtherGroundContact reads `KeyAt(i).gameObject.layer`).
struct ColliderRef {
    uint64_t id = 0;
    int layer = 0;
    bool operator==(const ColliderRef& o) const { return id == o.id; }
    bool operator!=(const ColliderRef& o) const { return id != o.id; }
};

// UnityEngine.ContactPoint (the fields the ported code reads)
struct ContactPoint {
    Part* thisColliderPart = nullptr;    // thisCollider belongs to this part (its Part.collider)
    Part* otherColliderPart = nullptr;   // otherCollider belongs to that part (null: scenery)
    bool thisColliderIsWheelPiston = false;  // thisCollider.CompareTag("Wheel_Piston_Collider")
    Vector3f point;
    Vector3f normal;
};

struct Collision {
    ColliderRef collider;
    bool colliderEnabled = true;
    bool colliderActive = true;          // c.collider.gameObject.activeInHierarchy
    bool colliderIsTrigger = false;
    int layer = 0;                       // c.gameObject.layer
    std::string tag = "Untagged";        // c.gameObject.tag
    std::string name;                    // c.gameObject.name
    bool isPQ = false;
    CelestialBody* pqBody = nullptr;
    Part* otherPart = nullptr;
    bool otherHasRigidbody = false;
    float otherRigidbodyMass = 0.0f;
    std::string crashObjectName;
    Vector3f relativeVelocity;
    std::vector<ContactPoint> contacts;  // c.contacts (one per contact the host reports)
};

}  // namespace UnityEngine

// Statics of other classes the Part collision code reads (unset hooks: the stated default)
struct PartContactEnvironment {
    using Collision = UnityEngine::Collision;
    static inline bool noCrashDamage = false;            // CheatOptions.NoCrashDamage
    static inline bool serenityInstalled = false;        // ExpansionsLoader.IsExpansionInstalled("Serenity") (the ROC branch)
    // ModuleWheelDamage.HandleCollision(c) of a part with that module (Part.CheckCollision with WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED); unset:
    // nothing (the wheel's impactDamageCollider is a model collider the proxy colliders do not have)
    static inline std::function<void(Part&, const Collision&)> wheelDamage;
    // ROC.CheckCollision(vessel) of a ROC hit (Breaking Ground)
    static inline std::function<void(const Collision&, Part&)> rocCollision;
    // FXMonger.Explode(part, position, power) (Part.explode): FX; unset: nothing
    static inline std::function<void(Part&, const Vector3f& position, float power)> explodeFx;
    // Part.Rigidbody.velocity (Part.cs:1049: the part's rb or its parent's; Part.checkLanded); unset: zero
    static inline std::function<Vector3f(Part&)> rigidbodyVelocity;
    // a host or test taking over Part.explode(offset) (true: handled, the C# body is not run): the reference replays record the call and
    // keep the part alive. Unset: KSP's explode
    static inline std::function<bool(Part&, float offset)> explodeOverride;
    static void Reset() {
        noCrashDamage = false;
        serenityInstalled = false;
        wheelDamage = nullptr;
        rocCollision = nullptr;
        explodeFx = nullptr;
        rigidbodyVelocity = nullptr;
        explodeOverride = nullptr;
    }
};

}  // namespace ksp
