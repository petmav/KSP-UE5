// WP-19c: FlightGlobals' debug-menu setters (FlightGlobals.cs:1090-1256: SetShipOrbit, SetShipOrbitRendezvous, PrepForOrbitSet, PostOrbitSet,
// SetVesselPosition, OnGameUnpaused, ActiveVesselGoOffRails) and clearInverseRotation (:1292). The Cheats menu (and Making History's vessel
// situations) call them; every engine action goes through the ports they reach (Vessel.GoOnRails / GoOffRails, OrbitPhysicsManager,
// FloatingOrigin, Vessel.SetRotation).
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/physics/collision_enhancer.hpp"
#include "ksp/physics/floating_origin.hpp"
#include "ksp/physics/orbit_physics_manager.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
Vessel& Active() {
    if (FlightGlobals::ActiveVessel == nullptr) throw NullReferenceException("FlightGlobals.ActiveVessel is null");
    return *FlightGlobals::ActiveVessel;
}
CelestialBody* BodyAt(int index) {
    const std::vector<CelestialBody*>& b = FlightGlobals::Bodies();
    if (index < 0 || static_cast<size_t>(index) >= b.size()) throw std::out_of_range("index");  // bodies[selBodyIndex]: List<T> ArgumentOutOfRangeException
    return b[static_cast<size_t>(index)];
}
// FlightGlobals.cs:1252 ActiveVesselGoOffRails
Enumerator ActiveVesselGoOffRails() {
    co_yield YieldInstruction::Null();
    Active().GoOffRails();
}
}  // namespace

// FlightGlobals.cs:1090
void FlightGlobals::SetShipOrbit(int selBodyIndex, double ecc, double sma, double inc, double LAN, double mna, double argPe, double /*ObT*/) {
    Vessel& av = Active();
    if (av.easingInToSurface) ToggleVesselEaseIn(&av, false);
    CelestialBody* mainBody = av.mainBody();
    currentMainBody = BodyAt(selBodyIndex);
    PrepForOrbitSet();
    sma = dotnet::Math::Min(currentMainBody->sphereOfInfluence * 0.99, sma);
    av.orbit().SetOrbit(inc, ecc, sma, LAN, argPe, mna, Planetarium::GetUniversalTime(), currentMainBody);
    PostOrbitSet(mainBody);
}

// FlightGlobals.cs:1104
void FlightGlobals::SetShipOrbitRendezvous(Vessel* target, const Vector3d& relPosition, const Vector3d& relVelocity) {
    Vessel& av = Active();
    if (av.easingInToSurface) ToggleVesselEaseIn(&av, false);
    CelestialBody* mainBody = av.mainBody();
    if (target == nullptr) throw NullReferenceException("target is null");
    currentMainBody = target->mainBody();
    PrepForOrbitSet();
    const double universalTime = Planetarium::GetUniversalTime();
    Orbit* currentOrbit = target->GetCurrentOrbit();
    if (currentOrbit == nullptr) throw NullReferenceException("target.GetCurrentOrbit() is null");
    const Vector3d normalized = currentOrbit->vel.normalized();
    const Vector3d orbitNormal = currentOrbit->GetOrbitNormal();
    const Vector3d x = Vector3d::Cross(normalized, orbitNormal);
    const Vector3d pos = currentOrbit->pos + relPosition.Basis(x, normalized, orbitNormal);
    const Vector3d vel = currentOrbit->vel + relVelocity.Basis(x, normalized, orbitNormal);
    av.orbit().UpdateFromStateVectors(pos, vel, currentOrbit->referenceBody, universalTime);
    PostOrbitSet(mainBody);
}

// FlightGlobals.cs:1124
void FlightGlobals::PrepForOrbitSet() {
    Vessel& av = Active();
    overrideOrbit = true;
    disableOverrideAt_.push_back(Time::time + 2.0f);  // Invoke("disableOverride", 2f)
    av.Landed = false;
    av.Splashed = false;
    av.SetLandedAt("", "", "");
    av.KillPermanentGroundContact();
    av.ResetGroundContact();
    const int count = static_cast<int>(Vessels.size());
    for (int i = 0; i < count; i++) {
        Vessel* vessel = Vessels[static_cast<size_t>(i)];
        if (!vessel->packed) vessel->GoOnRails();
    }
    ClearInverseRotation();  // clearInverseRotation (:1292)
    OrbitPhysicsManager::SetDominantBody(currentMainBody);
}

// FlightGlobals.cs:1146
void FlightGlobals::PostOrbitSet(CelestialBody* oldBody) {
    Vessel& av = Active();
    av.orbitDriver->updateFromParameters();
    physics::CollisionEnhancer::bypass = true;
    FloatingOrigin::SetOffset(Vector3d(av.vesselTransform.position));
    OrbitPhysicsManager::CheckReferenceFrame();
    OrbitPhysicsManager::HoldVesselUnpack(10);
    if (av.mainBody() != oldBody)
        GE::onVesselSOIChanged.Fire(GE::HostedFromToAction<Vessel*, CelestialBody*>(&av, oldBody, av.mainBody()));
    av.IgnoreGForces(20);
    av.IgnoreSpeed(20);
}

// FlightGlobals.cs:1161
void FlightGlobals::SetVesselPosition(int selBodyIndex, double latitude, double longitude, double altitude, double inclination, double heading, bool asl,
                                      bool easeToSurface, double gravityMultiplier) {
    SetVesselPosition(selBodyIndex, latitude, longitude, altitude, Vector3f(static_cast<float>(inclination), 0.0f, static_cast<float>(heading)), asl,
                      easeToSurface, gravityMultiplier);
}

// FlightGlobals.cs:1166
void FlightGlobals::SetVesselPosition(int selBodyIndex, double latitude, double longitude, double altitude, const Vector3f& rotation, bool asl,
                                      bool easeToSurface, double easeInMultiplier) {
    Vessel& av = Active();
    CelestialBody* mainBody = av.mainBody();
    currentMainBody = BodyAt(selBodyIndex);
    PrepForOrbitSet();
    const Vector3d relSurfaceNVector = currentMainBody->GetRelSurfaceNVector(latitude, longitude);
    double num = 0.0;
    if (currentMainBody->pqsController != nullptr) num = currentMainBody->pqsController->GetSurfaceHeight(relSurfaceNVector) - currentMainBody->Radius;
    else easeToSurface = false;
    if (asl && num < 0.0) num = 0.0;
    altitude = dotnet::Math::Abs(altitude);
    if (altitude > GameSettings::DEBUG_MAX_SETPOSITION_ALTITUDE) {
        log::Warning("[FlightGlobals]: SetPosition altitude limited to DEBUG_MAX_SETPOSITION_ALTITUDE (" +
                     dotnet::FormatDoubleG(GameSettings::DEBUG_MAX_SETPOSITION_ALTITUDE) + ")");
        altitude = GameSettings::DEBUG_MAX_SETPOSITION_ALTITUDE;
    }
    if (latitude < -89.999) latitude = -89.999;
    if (latitude > 89.999) latitude = 89.999;
    if (longitude < -180.0) longitude = -180.0;
    if (longitude > 180.0) longitude = 180.0;
    Vector3d rhs = currentMainBody->GetWorldSurfacePosition(latitude, longitude, altitude + num) - currentMainBody->position;
    Vector3d vector3d = Vector3d::Cross(currentMainBody->angularVelocity, rhs);
    rhs = rhs.xzy();
    vector3d = vector3d.xzy();
    av.orbit().UpdateFromStateVectors(rhs, vector3d, currentMainBody, Planetarium::GetUniversalTime());
    PostOrbitSet(mainBody);
    av.SetRotation(Quaternionf::identity());
    // Quaternion.LookRotation(ProjectOnPlane(north pole point - vessel, up).normalized, up) * Inverse(ReferenceTransform.rotation) * the two
    // AngleAxis turns (the Unity-native quaternion functions of quaternion_native.hpp: UNVERIFIED against the game, approximate)
    const Vector3d vesselPos(av.vesselTransform.position);
    const Vector3f up = (vesselPos - currentMainBody->position).normalized().ToVector3f();
    Quaternionf quaternion = UnityNative::LookRotation(
        Vector3f::ProjectOnPlane((currentMainBody->position + currentMainBody->transformUp * currentMainBody->Radius - vesselPos).ToVector3f(), up).normalized(),
        up);
    PartTransform& reference = av.ReferenceTransform();
    quaternion = quaternion * UnityNative::Inverse(reference.rotation);
    quaternion = quaternion * UnityNative::AngleAxis(90.0f, reference.right());
    quaternion = quaternion * (UnityNative::AngleAxis(rotation.z, -reference.forward()) * UnityNative::AngleAxis(rotation.x, -reference.right()));
    av.SetRotation(quaternion);
    if (FlightDriver::Pause()) {
        setPositionInProgress = true;
        setPositionInProgressVesselID = av.persistentId;
    } else {
        setPositionInProgress = false;
        StartActiveVesselGoOffRails();
    }
    if (easeToSurface) ToggleVesselEaseIn(&av, true, easeInMultiplier);
    else if (av.easingInToSurface) ToggleVesselEaseIn(&av, false);
}

// FlightGlobals.cs:1240
void FlightGlobals::OnGameUnpaused() {
    if (setPositionInProgress) {
        if (setPositionInProgressVesselID == Active().persistentId) StartActiveVesselGoOffRails();
        setPositionInProgress = false;
    }
}

// StartCoroutine(ActiveVesselGoOffRails()) on the scene's scheduler (HighLogic::Scheduler(): the host advances it once per frame)
void FlightGlobals::StartActiveVesselGoOffRails() { HighLogic::Scheduler().StartCoroutine(ActiveVesselGoOffRails()); }

}  // namespace ksp
