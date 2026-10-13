// WP-19b: physicalObject; see ksp/physics/physical_object.hpp. C# paths relative to decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/physics/physical_object.hpp"

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// physicalObject.cs:17 Awake
physicalObject::physicalObject(physics::PhysicsWorld& w, physics::BodyId body) : world(&w), rb(body) { FlightGlobals::addPhysicalObject(this); }

// physicalObject.cs:42 OnDestroy (and the Rigidbody of the destroyed GameObject)
physicalObject::~physicalObject() {
    FlightGlobals::removePhysicalObject(this);
    if (world != nullptr && world->IsValid(rb)) world->RemoveBody(rb);
}

// physicalObject.cs:48
bool physicalObject::UpdateWantsDestroy(const Vector3f& position, bool hasActiveVessel, const Vector3f& activeVesselPosition) const {
    if (TimeWarp::CurrentRate() > 1.0f && TimeWarp::WarpMode() == TimeWarp::Modes::HIGH) return true;
    return hasActiveVessel && Vector3f::Distance(position, activeVesselPosition) > maxDistance;
}

}  // namespace ksp
