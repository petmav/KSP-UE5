// WP-32: FlightEVA.cs (kerbal EVA spawning: hatch obstruction tests, fairing test, spawnEVA / onGoForEVA bookkeeping, SwitchToEVAVesselWhenReady).
// The Unity side (Instantiate of the kerbalEVA prefab, AddComponent<Vessel>, rays and overlap spheres, Collider tags) is FlightEvaEnvironment.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/eva/kerbal_eva.hpp"

namespace ksp {

// What a hatch ray hit: whether anything was hit, and the Part component on the hit transform (hitInfo.transform.gameObject.GetComponent<Part>())
struct HatchHit {
    bool hit = false;
    Part* part = nullptr;
};

// One collider in the overlap sphere: GetComponentInParent<Part>() and isTrigger
struct HatchOverlap {
    Part* parentPart = nullptr;
    bool isTrigger = false;
};

struct FlightEvaEnvironment {
    // UnityEngine.Physics.Raycast(origin, direction, out hit, distance, mask[, Ignore])
    static inline std::function<HatchHit(const Vector3f& origin, const Vector3f& direction, float distance, int mask)> raycast;
    // Physics.OverlapSphere(center, radius, 67665921) -> colliders
    static inline std::function<std::vector<HatchOverlap>(const Vector3f& center, float radius, int mask)> overlapSphere;
    // fromPart.GetComponentsInChildren<Collider>() filtered by CompareTag("Airlock"): their transforms
    static inline std::function<std::vector<const PartTransform*>(Part&)> airlockTransforms;
    // Part.airlock (the part's main airlock transform)
    static inline std::function<const PartTransform*(Part&)> partAirlock;
    // fromPart.airstreamShields[0].GetPart().Modules.GetModule<ModuleProceduralFairing>() with interstageCraftID == 0 && isCapped
    static inline std::function<bool(Part& shieldPart)> fairingCappedNoInterstage;
    // PartLoader.getPartInfoByName(...).partPrefab Instantiate + KerbalEVA lookup (FlightEVA._Spawn); the default Spawn delegate calls this
    static inline std::function<KerbalEVA*(ProtoCrewMember&)> instantiateKerbalEVA;
    // kerbalEVA.part.vessel = AddComponent<Vessel>(); vessel.Initialize()
    static inline std::function<Vessel*(KerbalEVA&)> addVessel;
    // Guid.NewGuid() (unset: Guid::Empty)
    static inline std::function<Guid()> newGuid;
    // fromPart.Rigidbody.GetPointVelocity(point) / angularVelocity / velocity
    static inline std::function<Vector3f(Part&, const Vector3f& point)> pointVelocity;
    static inline std::function<Vector3f(Part&)> partAngularVelocity;
    // ShipConstruction.GetUniqueFlightID(flightState)
    static inline std::function<uint32_t()> uniqueFlightID;
    // pCrew.KerbalRef.state = Kerbal.States.BAILED_OUT
    static inline std::function<void(ProtoCrewMember&)> kerbalBailedOut;
    // CollisionEnhancer.bypass
    static inline bool collisionEnhancerBypass = false;
    // Orbit seam: FlightGlobals.getMainBody(position)
    static inline std::function<CelestialBody*(const Vector3f&)> mainBodyAt;
    // FlightInputHandler.SetNeutralControls
    static inline std::function<void()> setNeutralControls;
    static inline std::vector<std::string> messages;  // ScreenMessages posted
    static void Reset();
};

class FlightEVA {
public:
    using SpawnKerbalEVADelegate = std::function<KerbalEVA*(ProtoCrewMember&)>;

    static inline FlightEVA* fetch = nullptr;               // :10
    float airlockStandoff = 0.18f;                          // :12
    static inline SpawnKerbalEVADelegate Spawn;             // :14 (default: _Spawn)
    bool overrideEVA = false;                               // :22

    FlightEVA();                                            // Awake (:24)
    ~FlightEVA();                                           // OnDestroy (:35)
    static KerbalEVA* _Spawn(ProtoCrewMember& pcm);         // :51
    static bool HatchIsObstructed(Part& fromPart, const PartTransform& fromAirlock);       // :87
    static bool HatchIsObstructedMore(Part& fromPart, const PartTransform& fromAirlock);   // :93
    static bool hatchInsideFairing(Part& fromPart);         // :130
    KerbalEVA* spawnEVA(std::shared_ptr<ProtoCrewMember> pCrew, Part* fromPart, const PartTransform* fromAirlock, bool tryAllHatches = false);  // :144
    Enumerator SwitchToEVAVesselWhenReady(KerbalEVA* eva);  // :299
    std::shared_ptr<Coroutine> lastSwitch;

private:
    KerbalEVA* onGoForEVA();                                // :218
    void onEVAAborted();                                    // :290
    std::shared_ptr<ProtoCrewMember> pCrew_;
    Part* fromPart_ = nullptr;
    const PartTransform* fromAirlock_ = nullptr;
    PartTransform airlockCopy_;
};

}  // namespace ksp
