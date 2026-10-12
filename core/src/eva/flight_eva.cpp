// WP-32: FlightEVA.cs (see include/ksp/eva/flight_eva.hpp). Every function cites FlightEVA.cs:<line>.
#include "ksp/eva/flight_eva.hpp"

#include <algorithm>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/staging/staging_types.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/inventory/inventory.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/science/science.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

using GE = events::GameEvents;
using KSP::Localization::Localizer;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
constexpr int kHatchMask = 8519681 | 0x8000 | 0x80000 | 0x4000000;  // LayerUtil.DefaultEquivalent | 0x8000 | 0x80000 | 0x4000000
constexpr int kOverlapMask = 67665921;
void Post(const std::string& text) { FlightEvaEnvironment::messages.push_back(text); }
}  // namespace

void FlightEvaEnvironment::Reset() {
    raycast = nullptr; overlapSphere = nullptr; airlockTransforms = nullptr; partAirlock = nullptr; fairingCappedNoInterstage = nullptr;
    instantiateKerbalEVA = nullptr; addVessel = nullptr; newGuid = nullptr; pointVelocity = nullptr; partAngularVelocity = nullptr; uniqueFlightID = nullptr;
    kerbalBailedOut = nullptr; collisionEnhancerBypass = false; mainBodyAt = nullptr; setNeutralControls = nullptr; messages.clear();
}

// FlightEVA.cs:24 Awake / :35 OnDestroy
FlightEVA::FlightEVA() {
    if (fetch != nullptr) return;  // Object.Destroy(this)
    fetch = this;
    if (!Spawn) Spawn = &FlightEVA::_Spawn;  // `public static SpawnKerbalEVADelegate Spawn = _Spawn;`
}
FlightEVA::~FlightEVA() {
    if (fetch != nullptr && fetch == this) fetch = nullptr;
}

// FlightEVA.cs:51
KerbalEVA* FlightEVA::_Spawn(ProtoCrewMember& pcm) {
    if (!FlightEvaEnvironment::instantiateKerbalEVA) throw Nre();
    return FlightEvaEnvironment::instantiateKerbalEVA(pcm);
}

// FlightEVA.cs:87
bool FlightEVA::HatchIsObstructed(Part& fromPart, const PartTransform& fromAirlock) {
    if (!FlightEvaEnvironment::raycast) return false;
    const Vector3f dir = (fromAirlock.position - fromPart.partTransform.position).normalized();
    const Vector3f origin = fromAirlock.position - dir * 0.5f;
    return FlightEvaEnvironment::raycast(origin, dir, f32store(fx(fromPart.fields.hatchObstructionCheckOutwardDistance) + fx(0.5f)), kHatchMask).hit;
}

// FlightEVA.cs:93
bool FlightEVA::HatchIsObstructedMore(Part& fromPart, const PartTransform& fromAirlock) {
    if (HatchIsObstructed(fromPart, fromAirlock)) return true;
    Vector3f normalized = (fromPart.partTransform.position - fromAirlock.position).normalized();
    if (FlightEvaEnvironment::raycast) {
        HatchHit h = FlightEvaEnvironment::raycast(fromAirlock.position - normalized * fromPart.fields.hatchObstructionCheckInwardOffset, normalized,
                                                    fromPart.fields.hatchObstructionCheckInwardDistance, kHatchMask);
        if (h.hit) {
            Part* component = h.part;
            if (component != nullptr && component != &fromPart && !component->fields.frozen) return true;
        }
    }
    if (FlightEvaEnvironment::overlapSphere) {
        std::vector<HatchOverlap> array = FlightEvaEnvironment::overlapSphere(fromAirlock.position + normalized * fromPart.fields.hatchObstructionCheckSphereOffset,
                                                                               fromPart.fields.hatchObstructionCheckSphereRadius, kOverlapMask);
        for (const HatchOverlap& o : array) {
            Part* componentInParent = o.parentPart;
            if (componentInParent != nullptr && componentInParent != &fromPart && !componentInParent->fields.frozen && !o.isTrigger) return true;
        }
    }
    return false;
}

// FlightEVA.cs:130
bool FlightEVA::hatchInsideFairing(Part& fromPart) {
    if (fromPart.ShieldedFromAirstream() && fromPart.airstreamShields.size() > 0) {
        Part* shieldPart = fromPart.airstreamShields[0]->GetPart();
        if (FlightEvaEnvironment::fairingCappedNoInterstage && shieldPart != nullptr && FlightEvaEnvironment::fairingCappedNoInterstage(*shieldPart)) return true;
    }
    return false;
}

// FlightEVA.cs:144
KerbalEVA* FlightEVA::spawnEVA(std::shared_ptr<ProtoCrewMember> pCrew, Part* fromPart, const PartTransform* fromAirlock, bool tryAllHatches) {
    auto it = std::find(fromPart->protoModuleCrew.begin(), fromPart->protoModuleCrew.end(), pCrew);
    if (it == fromPart->protoModuleCrew.end()) {
        log::Error("[FlightEVA]: Tried to bail out " + pCrew->name().value_or("") + " from part " + fromPart->partInfo->name + " but part doesn't contain that kerbal!");
        return nullptr;
    }
    if (fromAirlock == nullptr) {
        Post(Localizer::Format("#autoLOC_111934"));
        return nullptr;
    }
    const PartTransform* data = fromAirlock;
    if (tryAllHatches) {
        bool flag = false;
        if (!HatchIsObstructedMore(*fromPart, *data)) {
            if (!hatchInsideFairing(*fromPart)) {
                flag = true;
                data = fromAirlock;
            }
        } else if (FlightEvaEnvironment::airlockTransforms) {
            std::vector<const PartTransform*> airlocks = FlightEvaEnvironment::airlockTransforms(*fromPart);
            for (const PartTransform* t : airlocks) {
                if (!(fromAirlock == t) && !HatchIsObstructed(*fromPart, *t) && !hatchInsideFairing(*fromPart)) {
                    flag = true;
                    data = t;
                    break;
                }
            }
        }
        if (!flag) {
            Post(Localizer::Format("#autoLOC_111971"));
            return nullptr;
        }
    } else if (HatchIsObstructed(*fromPart, *data) || hatchInsideFairing(*fromPart)) {
        Post(Localizer::Format("#autoLOC_111978"));
        return nullptr;
    }
    overrideEVA = false;
    GE::onAttemptEva.Fire(pCrew.get(), fromPart, static_cast<UnityEngine::Transform*>(nullptr));  // the airlock Transform is the shell's (PartTransform here)
    if (overrideEVA) return nullptr;
    pCrew_ = pCrew;
    fromPart_ = fromPart;
    airlockCopy_ = *data;
    fromAirlock_ = &airlockCopy_;
    return onGoForEVA();
}

// FlightEVA.cs:218
KerbalEVA* FlightEVA::onGoForEVA() {
    ProtoCrewMember& pCrew = *pCrew_;
    Part* fromPart = fromPart_;
    if (FlightEvaEnvironment::kerbalBailedOut) FlightEvaEnvironment::kerbalBailedOut(pCrew);
    if (!Spawn) throw Nre();
    KerbalEVA* kerbalEVA = Spawn(pCrew);
    if (kerbalEVA == nullptr) throw Nre();
    FlightEvaEnvironment::collisionEnhancerBypass = true;
    if (!FlightEvaEnvironment::addVessel) throw Nre();
    Vessel* vessel = FlightEvaEnvironment::addVessel(*kerbalEVA);
    Part* evaPart = kerbalEVA->PartPtr();
    evaPart->vessel = vessel;
    if (fromAirlock_ != nullptr) {
        evaPart->partTransform.rotation = fromAirlock_->rotation;
        evaPart->partTransform.position = fromAirlock_->position + fromAirlock_->rotation * (Vector3f::back() * airlockStandoff);
    } else {
        evaPart->partTransform.position = fromPart->partTransform.position + fromPart->partTransform.rotation * Vector3f::right() * 2.0f;
    }
    if (vessel->evaController == nullptr) vessel->evaController = kerbalEVA;  // Vessel.cs:1662 evaController = GetComponent<KerbalEVA>()
    vessel->id = FlightEvaEnvironment::newGuid ? FlightEvaEnvironment::newGuid() : Guid::Empty();
    if (fromAirlock_ != nullptr) {
        const Vector3f pv = FlightEvaEnvironment::pointVelocity ? FlightEvaEnvironment::pointVelocity(*fromPart, fromAirlock_->position) : Vector3f::zero();
        kerbalEVA->Engine().SetRbVelocity(pv);
        kerbalEVA->Engine().SetRbAngularVelocity(FlightEvaEnvironment::partAngularVelocity ? FlightEvaEnvironment::partAngularVelocity(*fromPart) : Vector3f::zero());
    }
    if (fromPart != nullptr) {
        double num = fromPart->fields.temperature;
        if (num > evaPart->fields.maxTemp - 50.0) num = evaPart->fields.maxTemp - 50.0;
        PhysicsGlobals* pg = PhysicsGlobals::Instance();
        if (pg == nullptr) throw Nre();
        if (num < pg->spaceTemperature) num = pg->spaceTemperature;
        if (GameSettings::EVA_INHERIT_PART_TEMPERATURE) {
            evaPart->fields.skinTemperature = num;
            evaPart->fields.skinUnexposedTemperature = num;
            evaPart->fields.temperature = num;
        } else {
            const double t = fx(kerbalEVA->evaExitTemperature());
            evaPart->fields.skinTemperature = t;
            evaPart->fields.skinUnexposedTemperature = t;
            evaPart->fields.temperature = t;
        }
        evaPart->fields.staticPressureAtm = fromPart->fields.staticPressureAtm;
    }
    pCrew.flightLog.AddEntryUnique(FlightLog::EntryType::ExitVessel, fromPart->vessel->orbitDriver->orbit->referenceBody->bodyName);
    fromPart->RemoveCrewmember(pCrew_);
    evaPart->AddCrewmember(pCrew_);
    if (KerbalInventoryScenario::Instance != nullptr) KerbalInventoryScenario::Instance->RemoveKerbalInventoryInstance(pCrew.name().value_or(""));
    vessel->vesselName = pCrew.name().value_or("");
    vessel->vesselType = VesselType::EVA;
    CelestialBody* body = FlightEvaEnvironment::mainBodyAt ? FlightEvaEnvironment::mainBodyAt(evaPart->partTransform.position) : nullptr;
    if (body != nullptr) vessel->orbitDriver->orbit->referenceBody = body;
    {
        Orbit& orbit = vessel->orbit();
        vessel->lastVel = orbit.GetRelativeVel() - orbit.GetRotFrameVel(*orbit.referenceBody);
    }
    vessel->launchedFrom = fromPart->vessel->launchedFrom;
    vessel->IgnoreGForces(10);
    evaPart->fields.flightID = FlightEvaEnvironment::uniqueFlightID ? FlightEvaEnvironment::uniqueFlightID() : 0u;
    evaPart->fields.missionID = fromPart->fields.missionID;
    evaPart->fields.launchID = fromPart->fields.launchID;
    evaPart->fields.flagURL = fromPart->fields.flagURL;
    GE::onCrewOnEva.Fire(GE::FromToAction<Part*, Part*>(fromPart, evaPart));
    GE::onCrewTransferred.Fire(GE::HostedFromToAction<ProtoCrewMember*, Part*>(&pCrew, fromPart, evaPart));
    Vessel::CrewWasModified(fromPart->vessel, vessel);
    lastSwitch = ScienceEnvironment::Scheduler().StartCoroutine(SwitchToEVAVesselWhenReady(kerbalEVA), 0);
    if (!fromPart->vessel->LandedOrSplashed()) {
        const PartTransform* airlock = fromAirlock_;
        ScienceEnvironment::Scheduler().StartCoroutine(kerbalEVA->StartNonCollidePeriod(1.0f, airlockStandoff, fromPart, airlock), kerbalEVA->CoroutineOwner());
    }
    pCrew_.reset();
    fromPart_ = nullptr;
    return kerbalEVA;
}

// FlightEVA.cs:290
void FlightEVA::onEVAAborted() {
    pCrew_.reset();
    fromPart_ = nullptr;
}

// FlightEVA.cs:299
Enumerator FlightEVA::SwitchToEVAVesselWhenReady(KerbalEVA* eva) {
    while (!eva->Ready) co_yield YieldInstruction::Null();
    if (FlightGlobals::ForceSetActiveVessel(eva->VesselPtr())) {
        if (FlightEvaEnvironment::setNeutralControls) FlightEvaEnvironment::setNeutralControls();
    }
}

}  // namespace ksp
