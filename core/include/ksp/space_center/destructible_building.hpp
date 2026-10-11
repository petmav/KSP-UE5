// WP-42: DestructibleBuilding.cs (a KSC structure that takes impact damage, collapses and can be repaired; the state machine
// ScenarioDestructibles persists).
//
// Seams (WP-47 scene / WP-12 physics): the GameObjects (collapseObject / replacementObject active flags are plain bools here), the
// collapsing object's local transform (a position / rotation pair), VFXSequencer prefabs (hasDemolitionFX etc.: when absent the
// delayedCallback path of the C# runs), colliders (`OnCollisionEnter` takes the contact data as a plain struct), Time.timeSinceLevelLoad
// (`timeSinceLevelLoad`), HierarchyUtil.CompileID (`id` is serialized with preCompiledId; an uncompiled id comes from `compiledId`).
// Quaternion.Lerp / Quaternion.Euler are Unity natives: the Lerp used here is written from Unity's documented formulation and is NOT
// verified against the game (STATUS.md).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/career/scenario_misc.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/events/game_event_types.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Vessel;
class ConfigNode;
class DestructibleBuilding;

// Unity's native Quaternion.Lerp (clamped t, short-way blend, normalised), per-operation float32. UNVERIFIED (no capture).
Quaternionf QuaternionLerpNative(const Quaternionf& a, const Quaternionf& b, float t);

// DestructibleBuilding.cs:6
class DestructibleBuilding : public IDestructibleBuilding {
public:
    // :10
    class CollapsibleObject {
    public:
        enum class Behaviour : int { Disable, Collapse };   // :12
        bool hasCollapseObject = true;        // collapseObject != null
        bool collapseObjectActive = true;     // collapseObject.activeSelf
        bool hasReplacementObject = false;    // replacementObject != null
        bool replacementObjectActive = false; // replacementObject.activeSelf
        Behaviour collapseBehaviour = Behaviour::Disable;   // :22
        float collapseDuration = 0.0f;        // :24
        float repairDuration = 0.0f;          // :26
        float replaceDelay = 1.0f;            // :28
        Vector3f collapseOffset{0.0f, 0.0f, 0.0f};      // :30
        Vector3f collapseTiltMax{0.0f, 0.0f, 0.0f};     // :32
        bool hasSecondaryFXPrefab = false;    // SecondaryFXPrefab != null (:34)
        DestructibleBuilding* sharedWith = nullptr;     // :36
        // collapseObject.transform.localPosition / localRotation (cTrf)
        Vector3f localPosition{0.0f, 0.0f, 0.0f};
        Quaternionf localRotation = Quaternionf::identity();

        bool setup() const { return init; }   // :52
        void Init();                          // :54
        void Load(const ConfigNode& node);    // :66
        void Save(ConfigNode& node);          // :79
        void Collapse(DestructibleBuilding& host);   // :84
        void Repair(DestructibleBuilding& host);     // :140
        void SetDestroyed(bool st);           // :163
        void Reset();                         // :187

    private:
        Enumerator disableCoroutine();        // :111
        Enumerator collapseCoroutine();       // :118
        Enumerator repairCoroutine();         // :156
        Vector3f stPos, endPos;               // :40-42
        Quaternionf stRot = Quaternionf::identity();   // :44
        bool intact = true;                   // :46
        bool destroyed = false;               // :48
        bool init = false;                    // :50
    };

    std::vector<CollapsibleObject> CollapsibleObjects;   // :193
    bool hasDemolitionFXPrefab = false;       // :195 DemolitionFXPrefab != null
    bool hasRepairFXPrefab = false;           // :197 RepairFXPrefab != null
    float FacilityDamageFractionValue = 0.0f; // :205 FacilityDamageFraction
    float RepairCost = 0.0f;                  // :207
    float CollapseReputationHit = 0.0f;       // :209
    float impactMomentumThreshold = 500.0f;   // :211
    float damageDecay = 5.0f;                 // :213
    static inline float BuildingToughnessFactor = 0.75f;   // :219
    static inline events::EventData<DestructibleBuilding*> OnLoaded{"OnDestructibleLoaded"};   // :221
    bool preCompiledId = false;               // :223
    std::string id;                           // :225
    std::string compiledId;                   // HierarchyUtil.CompileID(transform, "SpaceCenter") (the scene fills it)
    std::string tag;                          // gameObject.tag (FindVesselsOverStructure)
    bool activeInHierarchy = true;
    int owner = 0;                            // StartCoroutine owner id

    // seams
    static inline std::function<float()> timeSinceLevelLoad;                // Time.timeSinceLevelLoad (unset: 0)
    // VFXSequencer.Play(callback): the host runs the sequence and calls back; unset: the callback is never called (the FX is "playing")
    std::function<void(const std::function<void()>& onComplete)> playDemolitionFX, playRepairFX;
    // GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE) after a collapse (unset: nothing)
    static inline std::function<void()> saveGame;
    std::function<void(Vessel*)> onEndFlightNoDamage;   // unused hook for hosts

    DestructibleBuilding();
    ~DestructibleBuilding() override;
    DestructibleBuilding(const DestructibleBuilding&) = delete;
    DestructibleBuilding& operator=(const DestructibleBuilding&) = delete;

    bool IsIntactValue() const { return intact_; }
    bool IsDestroyed() const { return destroyed_; }       // :237
    float Damage() const { return damage_; }              // :239

    // IDestructibleBuilding (the interface ScenarioDestructibles uses)
    const std::string& BuildingId() const override { return id; }
    void Save(ConfigNode& node) override;                 // :374
    void Load(const ConfigNode& node) override;           // :351
    bool IsIntact() const override { return intact_; }    // :235
    float FacilityDamageFraction() const override { return FacilityDamageFractionValue; }

    void Awake();                                          // :241
    void Start();                                          // :260
    void OnPhysicsEase(Vessel* v);                         // :268
    void OnLevelLoaded(GameScenes scn);                    // :276
    void OnSceneStart(GameScenes scn);                     // :284
    Enumerator LoadOnSceneStart();                         // :293
    void CompileID();                                      // :303
    void ClearID();                                        // :310
    bool RegisterInstance();                               // :316
    void UnregisterInstance(bool saveState);               // :335
    void FixedUpdate(float fixedDeltaTime);                // :379
    void Demolish();                                       // :401
    void Repair();                                         // :448
    void Reset();                                          // :495
    float GetRepairSequenceDuration() const;               // :512
    void OnLeavingScene(GameScenes scn);                   // :549
    void OnDisable();                                      // :563
    void OnEnable();                                       // :568
    void OnDestroy();                                      // :580
    // OnCollisionEnter's contact data: the C# reads Collision.contacts / relativeVelocity / rigidbody and the part's rigidbody / vessel
    struct ContactInfo {
        std::vector<Vector3f> contactNormals;   // c.contacts[i].normal
        Vector3f relativeVelocity;              // c.relativeVelocity
        float rigidbodyMass = 0.0f;             // c.rigidbody.mass
        bool hasPart = false;                   // FlightGlobals.GetPartUpwardsCached(c.gameObject) != null
        bool hasPartRigidbody = false;          // partUpwardsCached.Rigidbody != null
        Vector3f partRigidbodyVelocity;         // partUpwardsCached.Rigidbody.velocity
        bool hasVessel = false;                 // partUpwardsCached.vessel != null
        Vector3f vesselRbVelocity;              // partUpwardsCached.vessel.rb_velocity
        double vesselTotalMass = 0.0;           // (float)partUpwardsCached.vessel.totalMass
    };
    void OnCollisionEnter(const ContactInfo& c);           // :606
    void AddDamage(float d);                               // :682
    bool Indestructible() const { return indestructible_; }

    static void ClearStatics();

private:
    Enumerator delayedCallback(float delay, std::function<void()> cb);   // :600
    void onDemolitionComplete();                           // :440
    void onRepairComplete();                               // :488
    void onFXComplete() {}                                 // :435
    void StartFlightNoDamagePeriod();                      // :709
    void EndFlightNoDamagePeriod(Vessel* v);               // :715
    void clearVFX() {}                                     // :541
    void damageUpdate();                                   // :388
    friend class CollapsibleObject;
    bool intact_ = true;                                   // :201
    bool destroyed_ = false;                               // :203
    float damage_ = 0.0f;                                  // :215
    float invulnerableTime_ = 0.0f;                        // :217
    bool indestructible_ = false;                          // :227
    bool registered_ = false;                              // :229
    bool needsResetOnReEnable_ = false;                    // :233
    bool eventsAdded_ = false;
    bool noDamageHandlerAdded_ = false;
};

}  // namespace ksp
