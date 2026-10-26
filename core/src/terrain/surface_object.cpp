// WP-12d: SurfaceObject.cs, SpaceCenterSurfaceObject.cs, PQSMod_SurfaceObjectQuads.cs (see ksp/terrain/surface_object.hpp).
#include "ksp/terrain/surface_object.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/coroutine_host.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/terrain/pq.hpp"

namespace ksp {

namespace {
using GE = events::GameEvents;
using UnityEngine::Transform;

void Collect(Transform* t, std::vector<Transform*>& out) {
    out.push_back(t);
    for (int i = 0; i < t->childCount(); ++i) Collect(t->GetChild(i), out);
}
}  // namespace

std::vector<Transform*> TransformsInChildren(Transform* root) {
    std::vector<Transform*> out;
    if (root != nullptr) Collect(root, out);
    return out;
}

// ---- SurfaceObject.cs ------------------------------------------------------------------------------------------------------------

SurfaceObject::~SurfaceObject() {
    if (startChild_) HighLogic::Scheduler().StopCoroutine(startChild_);
    if (start_) HighLogic::Scheduler().StopCoroutine(start_);
}

// Unity runs Start before the first Update after AddComponent: one frame, then Start
Enumerator SurfaceObject::StartNextFrame(SurfaceObject* self) {
    co_yield YieldInstruction::Null();
    self->startChild_ = HighLogic::Scheduler().StartCoroutine(self->Start(), self->owner_);
    co_yield YieldInstruction::Wait(self->startChild_);
}

// SurfaceObject.cs:42 (GetComponentUpwards: `findBody`)
Enumerator SurfaceObject::Start() {
    for (int i = 0; i < initDelay; i++) co_yield YieldInstruction::Null();
    const Vector3d worldPos(trf->position);
    if (cb == nullptr && findBody) cb = findBody();
    if (GrabCoordsAtStart) {
        if (cb == nullptr) throw NullReferenceException();
        cb->GetLatLonAlt(worldPos, latitude, longitude, altitude);
    }
    if (PopToSceneRootAtStart) PopToSceneRoot();
    started = true;
}

// SurfaceObject.cs:64
void SurfaceObject::onUpdate() {
    srfNVector = cb->GetSurfaceNVector(latitude, longitude);
    trf->position = (cb->position + srfNVector * (cb->Radius + altitude)).ToVector3f();
}

// SurfaceObject.cs:70
void SurfaceObject::PopToSceneRoot() {
    if (popped) return;
    children = TransformsInChildren(trf);
    pristineChildPositions.assign(children.size(), Vector3f::zero());
    pristineChildRotations.assign(children.size(), Quaternionf::identity());
    pristineChildLocalScales.assign(children.size(), Vector3f::zero());
    size_t num = children.size();
    while (num-- > 0) {
        Transform* transform = children[num];
        if (transform != trf) {
            pristineChildPositions[num] = transform->localPosition;
            pristineChildRotations[num] = transform->localRotation;
            pristineChildLocalScales[num] = transform->localScale;
        }
    }
    originalParent = trf->parent();
    trf->SetParent(nullptr);
    size_t num2 = children.size();
    while (num2-- > 0) {
        Transform* transform = children[num2];
        if (transform != trf) {
            transform->localPosition = pristineChildPositions[num2];
            transform->localRotation = pristineChildRotations[num2];
            transform->localScale = pristineChildLocalScales[num2];
        }
    }
    popped = true;
}

// SurfaceObject.cs:107
void SurfaceObject::ReturnToParent() {
    if (originalParent != nullptr) {
        if (!popped) return;
        trf->SetParent(originalParent);
        popped = false;
        size_t num = children.size();
        while (num-- > 0) {
            Transform* transform = children[num];
            if (transform != trf) {
                transform->localPosition = pristineChildPositions[num];
                transform->localRotation = pristineChildRotations[num];
                transform->localScale = pristineChildLocalScales[num];
            }
        }
    } else {
        log::Error("[SurfaceObject]: Cannot return to original parent, it no longer exists.");
    }
}

// SurfaceObject.cs:135
void SurfaceObject::FixedUpdate() {
    if (updateMode == KFSMUpdateMode::FIXEDUPDATE && started && !destroyed_) onUpdate();
}

// SurfaceObject.cs:143
void SurfaceObject::Update() {
    if (updateMode == KFSMUpdateMode::UPDATE && started && !destroyed_) onUpdate();
}

// SurfaceObject.cs:151
void SurfaceObject::LateUpdate() {
    if (updateMode == KFSMUpdateMode::LATEUPDATE && started && !destroyed_) onUpdate();
}

// SurfaceObject.cs:159
std::unique_ptr<SurfaceObject> SurfaceObject::Create(Transform* host, CelestialBody* body, int initDelay, KFSMUpdateMode updateMode, int owner) {
    auto surfaceObject = std::make_unique<SurfaceObject>();
    surfaceObject->cb = body;
    surfaceObject->initDelay = initDelay;
    surfaceObject->GrabCoordsAtStart = true;
    surfaceObject->PopToSceneRootAtStart = true;
    surfaceObject->updateMode = updateMode;
    surfaceObject->trf = host;
    surfaceObject->owner_ = owner;
    surfaceObject->start_ = HighLogic::Scheduler().StartCoroutine(StartNextFrame(surfaceObject.get()), owner);
    return surfaceObject;
}

// SurfaceObject.cs:171
std::unique_ptr<SurfaceObject> SurfaceObject::Create(Transform* host, CelestialBody* body, double latitude, double longitude, double altitude,
                                                     int initDelay, KFSMUpdateMode updateMode, int owner) {
    auto surfaceObject = std::make_unique<SurfaceObject>();
    surfaceObject->cb = body;
    surfaceObject->initDelay = initDelay;
    surfaceObject->GrabCoordsAtStart = false;
    surfaceObject->PopToSceneRootAtStart = true;
    surfaceObject->updateMode = updateMode;
    surfaceObject->latitude = latitude;
    surfaceObject->longitude = longitude;
    surfaceObject->altitude = altitude;
    surfaceObject->trf = host;
    surfaceObject->owner_ = owner;
    surfaceObject->start_ = HighLogic::Scheduler().StartCoroutine(StartNextFrame(surfaceObject.get()), owner);
    return surfaceObject;
}

// SurfaceObject.cs:186
std::unique_ptr<SurfaceObject> SurfaceObject::Create(Transform* host, CelestialBody* body, double latitude, double longitude, double altitude,
                                                     bool grabCoordsAtStart, bool popToSceneRootAtStart, int initDelay,
                                                     KFSMUpdateMode updateMode, int owner) {
    auto surfaceObject = std::make_unique<SurfaceObject>();
    surfaceObject->cb = body;
    surfaceObject->initDelay = initDelay;
    surfaceObject->GrabCoordsAtStart = grabCoordsAtStart;
    surfaceObject->PopToSceneRootAtStart = popToSceneRootAtStart;
    surfaceObject->updateMode = updateMode;
    surfaceObject->latitude = latitude;
    surfaceObject->longitude = longitude;
    surfaceObject->altitude = altitude;
    surfaceObject->trf = host;
    surfaceObject->owner_ = owner;
    surfaceObject->start_ = HighLogic::Scheduler().StartCoroutine(StartNextFrame(surfaceObject.get()), owner);
    return surfaceObject;
}

// SurfaceObject.cs:201
void SurfaceObject::Terminate() {
    ReturnToParent();
    destroyed_ = true;
    if (startChild_) {
        HighLogic::Scheduler().StopCoroutine(startChild_);
        startChild_.reset();
    }
    if (start_) {
        HighLogic::Scheduler().StopCoroutine(start_);
        start_.reset();
    }
}

// ---- SpaceCenterSurfaceObject.cs ---------------------------------------------------------------------------------------------------

// SpaceCenterSurfaceObject.cs:11
void SpaceCenterSurfaceObject::Awake() {
    GE::onLevelWasLoaded.Add(this, &SpaceCenterSurfaceObject::OnGameSceneLoaded);
    GE::onGameSceneLoadRequested.Add(this, &SpaceCenterSurfaceObject::OnGameSceneLoadRequested);
    awake_ = true;
}

// SpaceCenterSurfaceObject.cs:17
void SpaceCenterSurfaceObject::Setup() {
    cb = findBody ? findBody() : nullptr;
    srfObj = SurfaceObject::Create(gameObject_, cb, 5, KFSMUpdateMode::FIXEDUPDATE, owner);
    setup = true;
}

// SpaceCenterSurfaceObject.cs:24
void SpaceCenterSurfaceObject::OnGameSceneLoaded(GameScenes scn) {
    if (scn == GameScenes::SPACECENTER || scn == GameScenes::FLIGHT) {
        if (!setup) {
            Setup();
        } else {
            SurfaceObject* so = srfObj.get();
            HighLogic::Scheduler().StartCoroutine(
                CallbackUtil::DelayedCallback(5, Callback<>::Lambda(this, [so] { so->PopToSceneRoot(); })), owner);
        }
    }
}

// SpaceCenterSurfaceObject.cs:39
void SpaceCenterSurfaceObject::OnGameSceneLoadRequested(GameScenes) {
    const GameScenes loadedScene = HighLogic::LoadedScene();
    if ((loadedScene == GameScenes::SPACECENTER || loadedScene == GameScenes::FLIGHT) && setup) srfObj->ReturnToParent();
}

// SpaceCenterSurfaceObject.cs:48
void SpaceCenterSurfaceObject::OnDestroy() {
    if (!awake_) return;
    if (setup && srfObj->IsPopped()) srfObj->ReturnToParent();
    GE::onLevelWasLoaded.Remove(this, &SpaceCenterSurfaceObject::OnGameSceneLoaded);
    GE::onGameSceneLoadRequested.Remove(this, &SpaceCenterSurfaceObject::OnGameSceneLoadRequested);
    HighLogic::Scheduler().StopAllCoroutines(owner);
    awake_ = false;
}

SpaceCenterSurfaceObject::~SpaceCenterSurfaceObject() { OnDestroy(); }

// ---- PQSMod_SurfaceObjectQuads.cs --------------------------------------------------------------------------------------------------

// PQSMod_SurfaceObjectQuads.cs:17
void PQSMod_SurfaceObjectQuads::OnSetup() {
    minLevel = sphere->maxLevel - (maxLevelOffset < 0 ? -maxLevelOffset : maxLevelOffset);
    cb = nullptr;
    if (findBody) {
        cb = findBody();
    } else {
        for (CelestialBody* b : FlightGlobals::Bodies())
            if (b != nullptr && b->pqsController == sphere) cb = b;
    }
}

// PQSMod_SurfaceObjectQuads.cs:23 (the quad's GameObject is a Transform of this mod under the sphere's)
void PQSMod_SurfaceObjectQuads::OnQuadBuilt(PQ& quad) {
    if (quad.subdivision >= minLevel && quads_.find(&quad) == quads_.end()) {
        auto q = std::make_unique<QuadObject>();
        q->transform.name = quad.name;
        q->transform.position = (quadWorldPosition ? quadWorldPosition(quad) : sphere->GetWorldPosition(quad.meshOrigin)).ToVector3f();
        q->transform.localPosition = quad.meshOrigin.ToVector3f();
        q->transform.SetParent(&sphereTransform_);
        q->surfaceObject = SurfaceObject::Create(&q->transform, cb, 1, KFSMUpdateMode::LATEUPDATE, owner);
        quads_[&quad] = std::move(q);
    }
}

// PQSMod_SurfaceObjectQuads.cs:31 (Terminate: ReturnToParent + Object.Destroy(this): the component is gone)
void PQSMod_SurfaceObjectQuads::OnQuadDestroy(PQ& quad) {
    const auto it = quads_.find(&quad);
    if (it != quads_.end()) {
        it->second->surfaceObject->Terminate();
        it->second->transform.SetParent(nullptr);
        quads_.erase(it);
    }
}

void PQSMod_SurfaceObjectQuads::LateUpdate() {
    for (auto& [q, o] : quads_) o->surfaceObject->LateUpdate();
}

SurfaceObject* PQSMod_SurfaceObjectQuads::SurfaceObjectOf(const PQ& quad) const {
    const auto it = quads_.find(&quad);
    return it != quads_.end() ? it->second->surfaceObject.get() : nullptr;
}

void PQSMod_SurfaceObjectQuads::VisitSerialized(const SerializedFieldVisitor& v) const {
    PQSMod::VisitSerialized(v);
    v("maxLevelOffset", SerializedValue::Of(maxLevelOffset));
}

}  // namespace ksp
