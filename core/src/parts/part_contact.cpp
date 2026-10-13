// WP-20e: Part.cs's collision messages, landed / splashed checks and explode (Part.cs:3355-3745, 3941-3969, 4368-4407). C# paths are
// relative to decompiled/KSP-Decompiled/Assembly-CSharp/. The Collision record is ksp/physics/collision.hpp: the contact host
// (ksp/physics/contact_host.hpp) builds it from the PhysicsWorld contact events and calls these on the part whose Rigidbody owns the
// touching collider, as Unity dispatched PhysX's contact reports. Float statements follow the Mono rule (ksp/dotnet/mono_float.hpp).
//
// HighLogic.LoadedSceneIsFlight is HighLogic::LoadedScene() == FLIGHT (the SaveEnvironment storage every flight test sets).
#include <algorithm>

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;
using UnityEngine::Collision;
using UnityEngine::ColliderRef;
using UnityEngine::ContactPoint;

namespace {
bool SceneIsFlight() { return HighLogic::LoadedScene() == GameScenes::FLIGHT; }

// DictionaryValueList<Collider, int>: ContainsKey / index of a key
int CollisionIndex(const std::vector<std::pair<ColliderRef, int>>& d, const ColliderRef& key) {
    for (size_t i = 0; i < d.size(); ++i)
        if (d[i].first == key) return static_cast<int>(i);
    return -1;
}

std::string Title(const Part& p) { return p.partInfo != nullptr ? p.partInfo->title : std::string(); }
}  // namespace

// Part.cs:3355
void Part::OnCollisionEnter(const Collision& c) {
    if (!SceneIsFlight() || state_ == PartStates::DEAD || state_ == PartStates::PLACEMENT || state_ == PartStates::CARGO) return;
    if (checkAfterResetCollisions && !c.contacts.empty()) {
        // C# lines 3363-3378: FindModelComponents<Collider>() contains the contact's otherCollider or thisCollider: in the reduced model
        // a part's colliders are its own proxies, so: the contact touches one of this part's colliders
        const ContactPoint& cp = c.contacts[0];
        const bool flag = cp.otherColliderPart == this || cp.thisColliderPart == this;
        if (!flag) return;
    }
    const int k = CollisionIndex(currentCollisions, c.collider);  // :3380-3387
    if (k >= 0) currentCollisions[static_cast<size_t>(k)].second++;
    else currentCollisions.emplace_back(c.collider, 1);
    if (c.layer == 0) {  // :3388-3395
        const Part* partUpwardsCached = c.otherPart;
        if (partUpwardsCached != nullptr && (partUpwardsCached->State() == PartStates::PLACEMENT || partUpwardsCached->State() == PartStates::CARGO))
            return;
    }
    LandedCollisionChecks(c);  // :3396
    if (vessel != nullptr && vessel->IgnoreCollisionsFrames() > 0) return;
    crashObjectName = c.crashObjectName;  // :3401 GetComponentUpwards<CrashObjectName>(c.gameObject)
    if (vessel != nullptr) vessel->crashObjectName = crashObjectName;
    if (c.tag == "ROC" && PartContactEnvironment::serenityInstalled && PartContactEnvironment::rocCollision)  // :3406-3413
        PartContactEnvironment::rocCollision(c, *this);
    CheckCollision(c);
}

// Part.cs:3417 (the ROC terrain-tag branch of :3429-3434 is Breaking Ground's: a ROC's own tag is used)
void Part::LandedCollisionChecks(const Collision& c) {
    if (c.layer == 15 && state_ != PartStates::DEAD) {
        fields.GroundContact = true;
        if (std::find(vessel->GroundContacts.begin(), vessel->GroundContacts.end(), this) == vessel->GroundContacts.end())
            vessel->GroundContacts.push_back(this);
        vessel->Landed = true;
        if (!fields.PermanentGroundContact && c.tag != "Untagged") vessel->SetLandedAt(c.tag, c.tag);
    }
    if (c.layer != 0) return;
    Part* partUpwardsCached = c.otherPart;  // :3445
    if (partUpwardsCached != nullptr && !CheckOtherGroundContact(c) && partUpwardsCached->State() != PartStates::PLACEMENT &&
        partUpwardsCached->State() != PartStates::CARGO &&
        std::find(vessel->parts.begin(), vessel->parts.end(), partUpwardsCached) == vessel->parts.end() &&
        partUpwardsCached->vessel->LandedOrSplashed()) {
        if (isVesselEVA()) {
            vessel->Landed = true;
        } else if (!partUpwardsCached->isVesselEVA()) {
            vessel->Landed = partUpwardsCached->vessel->Landed;
        } else {
            vessel->Landed = partUpwardsCached->vessel->Landed && !partUpwardsCached->vessel->Splashed;
        }
        if (partUpwardsCached->vessel->Splashed) vessel->Splashed = true;
        if (!fields.PermanentGroundContact)
            vessel->SetLandedAt(partUpwardsCached->vessel->landedAt, "", partUpwardsCached->vessel->displaylandedAt);
    }
}

// Part.cs:3471
void Part::OnCollisionExit(const Collision& c) {
    const int k = CollisionIndex(currentCollisions, c.collider);
    if (k >= 0) {
        currentCollisions[static_cast<size_t>(k)].second--;
        if (currentCollisions[static_cast<size_t>(k)].second > 0) return;
        currentCollisions.erase(currentCollisions.begin() + k);
    } else if (SceneIsFlight() && vessel != nullptr && !vessel->isEVA()) {
        log::Info("Part " + gameObjectName + " exited collision with " + c.name + ", but it wasn't in collision count list!");
    }
    Part* part = nullptr;
    if (c.layer == 0) {  // :3487-3494
        part = c.otherPart;
        if (part != nullptr && part->State() != PartStates::PLACEMENT && part->State() != PartStates::CARGO && vessel != nullptr &&
            std::find(vessel->parts.begin(), vessel->parts.end(), part) == vessel->parts.end() && part->vessel != nullptr && part->vessel->Splashed)
            vessel->Splashed = false;
    }
    if (vessel != nullptr &&
        (c.layer == 15 || (c.layer == 0 && vessel != nullptr && std::find(vessel->parts.begin(), vessel->parts.end(), part) == vessel->parts.end())) &&
        !CheckOtherGroundContact(c)) {  // :3495
        fields.GroundContact = false;
        auto it = std::find(vessel->GroundContacts.begin(), vessel->GroundContacts.end(), this);
        if (it != vessel->GroundContacts.end()) {
            vessel->GroundContacts.erase(it);
            vessel->ResetRBAnchor();
        }
        for (size_t i = 0; i < vessel->parts.size(); i++)  // :3503-3509 modules[j].ResetWheelGroundCheck()
            for (size_t j = 0; j < vessel->parts[i]->Modules().Count(); j++) vessel->parts[i]->Modules().GetModule(j)->ResetWheelGroundCheck();
        vessel->UpdateLandedSplashed();
    }
    crashObjectName.clear();
    if (vessel != nullptr) vessel->crashObjectName = crashObjectName;
    // :3517 currentCollisions.Remove(c.collider) (a no-op when the count went to zero above)
    const int r = CollisionIndex(currentCollisions, c.collider);
    if (r >= 0) currentCollisions.erase(currentCollisions.begin() + r);
}

// Part.cs:3520: another collider in this part's collision list, on layer 15 (KSP compares with c.collider: a null key never matches)
bool Part::CheckOtherGroundContact(const Collision& c) const {
    for (size_t num = 0; num < currentCollisions.size(); ++num) {
        const ColliderRef& key = currentCollisions[num].first;
        if (key != c.collider && key.layer == 15) return true;
    }
    return false;
}

// Part.cs:3539 (the asteroid-collider and the fairing ClosedColliders branches compare against model colliders the proxies do not have:
// a part's only collider is its own `collider`)
bool Part::CheckCollision(const Collision& c) {
    if (GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED && PartContactEnvironment::wheelDamage) {
        for (size_t i = 0; i < modules_.Count(); i++)
            if (modules_.GetModule(i)->ClassName() == "ModuleWheelDamage") PartContactEnvironment::wheelDamage(*this, c);
    }
    for (size_t num = 0, num2 = c.contacts.size(); num < num2; num++) {
        const ContactPoint& contactPoint = c.contacts[num];
        if (contactPoint.thisColliderPart == this || contactPoint.otherColliderPart == this) {
            HandleCollision(c);
            return true;
        }
    }
    for (size_t num3 = 0, count2 = children.size(); num3 < count2; num3++) {  // :3591-3606
        Part* part = children[num3];
        if (part->fields.physicalSignificance != static_cast<int32_t>(Part_PhysicalSignificance::FULL) && part->isAttached() &&
            part->State() != PartStates::DEAD && part->CheckCollision(c))
            return true;
    }
    return false;
}

// Part.cs:3613. The early-out compares in double (Mono: `magnitude` is a float return, the product of two floats is not rounded).
void Part::HandleCollision(const Collision& c) {
    const float magnitude = c.relativeVelocity.magnitude();
    if (!c.colliderEnabled || !c.colliderActive || c.colliderIsTrigger || (!c.contacts.empty() && c.contacts[0].thisColliderIsWheelPiston) ||
        fx(magnitude) <= fx(fields.crashTolerance) ||
        (c.otherHasRigidbody && fx(magnitude) * fx(c.otherRigidbodyMass) <= fx(fields.crashTolerance)))
        return;
    // :3619-3625 the PQ's sphere's CelestialBody (VERBOSE_DEBUG_LOG only: the momentum string)
    if (GameSettings::VERBOSE_DEBUG_LOG)
        log::Info(gameObjectName + " collided into " + c.name + " - relative velocity: " + std::to_string(magnitude));
    if (c.isPQ) GE::onPartExplodeGroundCollision.Fire(this);  // :3630
    if (!PartContactEnvironment::noCrashDamage) {
        explode();
        Part* partUpwardsCached = c.otherPart;
        if (c.isPQ) {
            EventReport r(FlightEvents::CRASH, this, Title(*this), Localizer::Format("#autoLOC_438839"), 0, "", c.relativeVelocity.magnitude());
            GE::onCrash.Fire(&r);
        } else if (partUpwardsCached != nullptr) {
            EventReport r(FlightEvents::COLLISION, this, Title(*this), Title(*partUpwardsCached), 0, "", c.relativeVelocity.magnitude());
            GE::onCollision.Fire(&r);
        } else if (!crashObjectName.empty()) {
            EventReport r(FlightEvents::CRASH, this, Title(*this), crashObjectName, 0, "", c.relativeVelocity.magnitude());
            GE::onCrash.Fire(&r);
        } else {
            EventReport r(FlightEvents::CRASH, this, Title(*this), c.name, 0, "", c.relativeVelocity.magnitude());
            GE::onCrash.Fire(&r);
        }
    }
}

// Part.cs:3672 (Part.Rigidbody.velocity, Part.cs:1049: the part's rb, else its parent's: PartContactEnvironment::rigidbodyVelocity)
bool Part::checkLanded() {
    const Vector3f v = PartContactEnvironment::rigidbodyVelocity ? PartContactEnvironment::rigidbodyVelocity(*this) : Vector3f();
    if (!fields.PermanentGroundContact && (!fields.GroundContact || fx(v.sqrMagnitude()) >= 0.25)) {
        for (size_t num = 0, count = children.size(); num < count; num++) {
            Part* part = children[num];
            if (part->isAttached() && part->checkLanded()) return true;
        }
        return false;
    }
    return true;
}

// Part.cs:3697
bool Part::checkSplashed() {
    if (fields.WaterContact) return true;
    for (size_t num = 0, count = children.size(); num < count; num++) {
        Part* part = children[num];
        if (part->isAttached() && part->checkSplashed()) return true;
    }
    return false;
}

// Part.cs:3722 (vessel.objectUnderVessel: its tag; the ROC branch is Breaking Ground's)
void Part::checkPermanentLandedAt(const std::string& objectUnderVesselTag) {
    if (vessel == nullptr || objectUnderVesselTag.empty()) return;
    if (std::find(vessel->GroundContacts.begin(), vessel->GroundContacts.end(), this) == vessel->GroundContacts.end())
        vessel->GroundContacts.push_back(this);
    if (objectUnderVesselTag != "Untagged") vessel->SetLandedAt(objectUnderVesselTag, objectUnderVesselTag);
}

// Part.cs:3941
void Part::OnCollisionStay(const Collision& c) {
    if (checkAfterResetCollisions) OnCollisionEnter(c);
}

// Part.cs:3949
void Part::ResetCollisions() {
    currentCollisions.clear();
    fields.GroundContact = false;
    auto it = std::find(vessel->GroundContacts.begin(), vessel->GroundContacts.end(), this);
    if (it != vessel->GroundContacts.end()) {
        vessel->GroundContacts.erase(it);
        vessel->ResetRBAnchor();
    }
    if (vessel != nullptr) vessel->UpdateLandedSplashed();
    checkAfterResetCollisions = true;
}

// Part.cs:4368
void Part::explode() { explode(0.0f); }

// Part.cs:4373. The transform moves (partTransform.up = local up, Translate(0, offset, 0)) only matter for the FX; FXMonger.Explode is
// PartContactEnvironment::explodeFx. onPartExplode's distance: `this == FlightGlobals.ActiveVessel` compares a Part with a Vessel and is
// never true, so the distance to the active vessel is always taken (0 without an active vessel: the C# would throw).
void Part::explode(float offset) {
    if (PartContactEnvironment::explodeOverride && PartContactEnvironment::explodeOverride(*this, offset)) return;
    if (state_ == PartStates::DEAD) return;
    float num = 0.0f;
    const double srf = FlightGlobals::ship_srfSpeed();
    if (srf > 1000.0 && srf < 200.0) num = 0.12f;  // sic (never true)
    else if (srf >= 2000.0) num = 0.25f;
    (void)offset;
    // :4387 ModuleComet with canSpawnFragments: the fragments are the comet package's; FX otherwise
    if (PartContactEnvironment::explodeFx) PartContactEnvironment::explodeFx(*this, partTransform.position, f32store(fx(fields.explosionPotential) + fx(num)));
    log::Info(gameObjectName + " Exploded!! - blast awesomeness: " + std::to_string(fields.explosionPotential));
    const Vessel* active = FlightGlobals::ActiveVessel;
    const float distance = active != nullptr ? Vector3f::Distance(partTransform.position, active->vesselTransform.position) : 0.0f;
    GE::onPartExplode.Fire(GE::ExplosionReaction(distance, fields.explosionPotential));
    // :4396-4403 AnalyticsUtil (asteroid / comet): telemetry
    Die();
}

}  // namespace ksp
