// WP-12d: SurfaceObject.cs, SpaceCenterSurfaceObject.cs, PQSMod_SurfaceObjectQuads.cs.
//
// A SurfaceObject pins a Transform to a latitude / longitude / altitude of a body: Start (a coroutine) waits `initDelay` frames, reads the
// transform's position as lat / lon / alt (GrabCoordsAtStart) and pops the transform to the scene root (PopToSceneRootAtStart: no parent,
// the children's local poses restored); from then on every FixedUpdate / Update / LateUpdate (updateMode) sets
// position = cb.position + cb.GetSurfaceNVector(lat, lon) * (cb.Radius + altitude). ReturnToParent puts it back under its old parent.
// KSP uses it for the KSC (SpaceCenterSurfaceObject: the space-center scene and flight keep the KSC on the rotating body, and the scene
// change puts it back under the PQS), the space-center camera pivot (SpaceCenterCamera2.cs:129, WP-47) and, through
// PQSMod_SurfaceObjectQuads (on no stock prefab), the terrain quads of a sphere.
//
// Engine side: the Transform is the core's UnityEngine::Transform shim (world position / rotation and the local fields are plain data;
// SetParent re-links only), so PopToSceneRoot / ReturnToParent restore the local fields exactly as the C# does and the world pose stays.
// Unity calls Start before the first Update after AddComponent: SurfaceObject::Create starts the Start coroutine on HighLogic's scheduler
// with one leading frame (`yield return null`) for that. Unity's FixedUpdate / Update / LateUpdate calls are the host's (the PQSMod and
// SpaceCenterSurfaceObject forward them).
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/fsm/kerbal_fsm.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/transform.hpp"

namespace ksp {

class CelestialBody;

// SurfaceObject.cs:4
class SurfaceObject {
public:
    double latitude = 0.0;            // :6
    double longitude = 0.0;           // :8
    double altitude = 0.0;            // :10
    bool GrabCoordsAtStart = false;   // :12
    bool PopToSceneRootAtStart = false;   // :14
    KFSMUpdateMode updateMode = KFSMUpdateMode::MANUAL_TRIGGER;   // :18
    int initDelay = 0;                // :26
    bool IsPopped() const { return popped; }   // :40
    bool Started() const { return started; }
    bool Destroyed() const { return destroyed_; }
    CelestialBody* Body() const { return cb; }
    UnityEngine::Transform* Trf() const { return trf; }
    // GetComponentUpwards<CelestialBody>() of Start when no body was given (unset: none)
    std::function<CelestialBody*()> findBody;

    // SurfaceObject.cs:159 / :171 / :186 (AddComponent on `host`; the Start coroutine is begun on HighLogic's scheduler, owner `owner`)
    static std::unique_ptr<SurfaceObject> Create(UnityEngine::Transform* host, CelestialBody* body, int initDelay, KFSMUpdateMode updateMode,
                                                 int owner = 0);
    static std::unique_ptr<SurfaceObject> Create(UnityEngine::Transform* host, CelestialBody* body, double latitude, double longitude,
                                                 double altitude, int initDelay, KFSMUpdateMode updateMode, int owner = 0);
    static std::unique_ptr<SurfaceObject> Create(UnityEngine::Transform* host, CelestialBody* body, double latitude, double longitude,
                                                 double altitude, bool grabCoordsAtStart, bool popToSceneRootAtStart, int initDelay,
                                                 KFSMUpdateMode updateMode, int owner = 0);

    Enumerator Start();               // :42
    void PopToSceneRoot();            // :70
    void ReturnToParent();            // :107
    void FixedUpdate();               // :135
    void Update();                    // :143
    void LateUpdate();                // :151
    void Terminate();                 // :201 (ReturnToParent, then Object.Destroy(this): Destroyed() and its coroutine stopped)
    ~SurfaceObject();

private:
    void onUpdate();                  // :64
    static Enumerator StartNextFrame(SurfaceObject* self);
    CelestialBody* cb = nullptr;      // :16
    Vector3d srfNVector;              // :20
    UnityEngine::Transform* trf = nullptr;            // :22
    UnityEngine::Transform* originalParent = nullptr; // :24
    bool started = false;             // :28
    bool popped = false;              // :30
    std::vector<UnityEngine::Transform*> children;    // :32
    std::vector<Vector3f> pristineChildPositions;     // :34
    std::vector<Quaternionf> pristineChildRotations;  // :36
    std::vector<Vector3f> pristineChildLocalScales;   // :38
    bool destroyed_ = false;
    std::shared_ptr<Coroutine> start_, startChild_;
    int owner_ = 0;
};

// Transform.GetComponentsInChildren<Transform>(includeInactive: true): the transform and every descendant, depth first in child order
std::vector<UnityEngine::Transform*> TransformsInChildren(UnityEngine::Transform* root);

// SpaceCenterSurfaceObject.cs:3 (on the KSC GameObject)
class SpaceCenterSurfaceObject {
public:
    explicit SpaceCenterSurfaceObject(UnityEngine::Transform* gameObject) : gameObject_(gameObject) {}
    ~SpaceCenterSurfaceObject();     // OnDestroy (:48) once Awake ran
    SpaceCenterSurfaceObject(const SpaceCenterSurfaceObject&) = delete;
    SpaceCenterSurfaceObject& operator=(const SpaceCenterSurfaceObject&) = delete;

    // gameObject.GetComponentUpwards<CelestialBody>() (unset: none)
    std::function<CelestialBody*()> findBody;
    int owner = 0;                    // coroutine owner (HighLogic.fetch.StartCoroutine)

    void Awake();                     // :11
    void OnGameSceneLoaded(GameScenes scn);          // :24
    void OnGameSceneLoadRequested(GameScenes scn);   // :39
    void OnDestroy();                 // :48
    SurfaceObject* SrfObj() const { return srfObj.get(); }
    bool IsSetup() const { return setup; }

private:
    void Setup();                     // :17
    UnityEngine::Transform* gameObject_;
    std::unique_ptr<SurfaceObject> srfObj;   // :5
    CelestialBody* cb = nullptr;      // :7
    bool setup = false;               // :9
    bool awake_ = false;
};

// PQSMod_SurfaceObjectQuads.cs:4: a SurfaceObject on every built quad at subdivision >= maxLevel - |maxLevelOffset|, terminated when the
// quad is destroyed. The quads' GameObjects are Transforms owned by the mod (world position: `quadWorldPosition`, default the quad
// transform's position sphere.GetWorldPosition(meshOrigin)); LateUpdate() forwards Unity's LateUpdate to every live SurfaceObject.
class PQSMod_SurfaceObjectQuads : public PQSMod {
public:
    PQSMod_SurfaceObjectQuads() { className = "PQSMod_SurfaceObjectQuads"; }
    int maxLevelOffset = 0;           // :6 (Reset :12)
    std::function<CelestialBody*()> findBody;               // sphere.GetComponentInParent<CelestialBody>() (unset: FlightGlobals.Bodies by pqsController)
    std::function<Vector3d(const PQ&)> quadWorldPosition;   // the quad transform's world position
    int owner = 0;

    void OnSetup() override;          // :17
    void OnQuadBuilt(PQ& quad) override;     // :23
    void OnQuadDestroy(PQ& quad) override;   // :31
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
    void LateUpdate();
    SurfaceObject* SurfaceObjectOf(const PQ& quad) const;
    size_t Count() const { return quads_.size(); }
    int MinLevel() const { return minLevel; }

private:
    struct QuadObject {
        UnityEngine::Transform transform;
        std::unique_ptr<SurfaceObject> surfaceObject;
    };
    int minLevel = 0;                 // :8
    CelestialBody* cb = nullptr;      // :10
    std::map<const PQ*, std::unique_ptr<QuadObject>> quads_;
    UnityEngine::Transform sphereTransform_;
};

}  // namespace ksp
