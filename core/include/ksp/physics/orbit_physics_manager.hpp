// WP-19a: OrbitPhysicsManager (OrbitPhysicsManager.cs, 353 lines), the part that decides which vessels are on rails.
//
// Ported here: the singleton state (`dominantBody`, `holdVesselUnpack`, `releaseUnpackIn`, `started`), Start (the wait for
// FlightGlobals.ready), HoldVesselUnpack, FixedUpdate (the hold counter) and LateUpdate (the pack / unpack decision of every vessel by
// distance to the active vessel, with the time-warp rule "above the physics warp rates everything goes on rails"). Pure state: the
// outcome is `Vessel::GoOnRails()` / `Vessel::GoOffRails()` calls on the registry (flags, orbit-driver modes, events) and no physics body is created
// or destroyed here (the VesselPhysics objects follow GoOffRails / GoOnRails through Vessel::env.packPart / unpackPart: WP-20b).
//
// WP-19b ("frames and bubble") owns what `setRotatingFrame` / `setDominantBody` do to the physics bodies (re-expressing every body's velocity at
// the rotating <-> inertial switch, the dominant-body velocity resume). WP-20a already has the maths (ksp/physics/reference_frame.hpp:
// ReferenceFrame::SetRotatingFrame / CheckReferenceFrame). Here `checkReferenceFrame` is the C# decision chain (dominant body vs the active
// vessel's reference body, the rotating-frame altitude threshold) and the two actions are the seams `Environment::setDominantBody` /
// `setRotatingFrame`. Unset, an action does the state part only: `dominantBody = body` and `body.inverseRotation = state` (no velocity changes).
// `FixedUpdate` and `FlightGlobals::SetActiveVessel` call them exactly where the C# calls the private methods.
#pragma once

#include <functional>

namespace ksp {

class CelestialBody;

class OrbitPhysicsManager {
public:
    // ---- seams (WP-19b) -------------------------------------------------------------------------------------------------------------
    struct Environment {
        std::function<void(OrbitPhysicsManager&, CelestialBody*)> setDominantBody;   // OrbitPhysicsManager.cs:155 (the new dominant body)
        std::function<void(OrbitPhysicsManager&, bool)> setRotatingFrame;            // OrbitPhysicsManager.cs:235
    };
    static inline Environment env;

    // ---- fields (OrbitPhysicsManager.cs:7-32, same names) -------------------------------------------------------------------------
    CelestialBody* dominantBody = nullptr;
    bool toggleDominantBodyRotation = false;
    bool degub = false;
    bool holdVesselUnpack = false;

    // The singleton (`fetch`, set by Awake, cleared by OnDestroy). A second instance destroys itself.
    static OrbitPhysicsManager* Instance() { return fetch_; }
    static CelestialBody* DominantBody();  // :33 (null without an instance)

    OrbitPhysicsManager();   // Awake (:45)
    ~OrbitPhysicsManager();  // OnDestroy (:57)
    OrbitPhysicsManager(const OrbitPhysicsManager&) = delete;
    OrbitPhysicsManager& operator=(const OrbitPhysicsManager&) = delete;
    // false when this instance is the destroyed duplicate of an existing one (Object.Destroy(this))
    bool Alive() const { return alive_; }

    bool started() const { return started_; }
    int releaseUnpackIn() const { return releaseUnpackIn_; }

    bool Start();                                   // :65 coroutine body, polled: true once FlightGlobals.ready set `started`
    static void HoldVesselUnpack(int releaseAfter = 1);  // :75
    void FixedUpdate();                             // :84
    void LateUpdate();                              // :107

    void checkReferenceFrame();                     // :322 (the decision chain; the actions are the seams)
    void setDominantBody(CelestialBody* body);      // :155
    void setRotatingFrame(bool rotatingFrameState);  // :235
    // static facades (no instance: nothing, like `if ((bool)fetch)`)
    static void SetDominantBody(CelestialBody* body);   // :218
    static void SetRotatingFrame(bool rotating);        // :314
    static void CheckReferenceFrame();                  // :346

private:
    static inline OrbitPhysicsManager* fetch_ = nullptr;
    int releaseUnpackIn_ = 0;
    bool started_ = false;
    bool alive_ = true;
};

}  // namespace ksp
