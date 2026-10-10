// WP-42: SpaceCenter.cs (the KSC position and the great-circle distance to it).
//
// Seams: the Unity transform (`trf.position`) is the host's: install `transformPosition` (the engine's world position of the
// SpaceCenter transform), or call AttachToCity(city) to take the KSC PQSCity's placement (identity body frame, see pqs_city.hpp).
// spaceCenterAreaTrigger.gameObject.SetActive is `setAreaTriggerActive` (a collider; WP-47).
#pragma once

#include <functional>

#include "ksp/events/game_event_types.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/save/save_enums.hpp"

namespace ksp {

class CelestialBody;
class PQSCity;

class SpaceCenter {
public:
    static inline SpaceCenter* Instance = nullptr;   // :7

    CelestialBody* cb = nullptr;                     // :16 (Part.GetComponentUpwards<CelestialBody>)
    double AreaRadius = 1500.0;                      // :14
    bool destroyed = false;                          // Object.Destroy(this) of Awake
    bool identityFrame = false;                      // AttachToCity: the body frame is the identity (see the file comment)

    std::function<Vector3d()> transformPosition;     // trf.position
    std::function<void(bool)> setAreaTriggerActive;  // spaceCenterAreaTrigger.gameObject.SetActive

    double Latitude() const { return latitude_; }    // :32
    double Longitude() const { return longitude_; }  // :34
    const Vector3d& SrfNVector() const { return srfNVector_; }   // :36

    void AttachToCity(const PQSCity& city);          // host helper: transformPosition = the city's local position
    void Awake();                                    // :38
    void Start();                                    // :48
    void OnDestroy();                                // :57
    void OnGameSceneLoaded(GameScenes scn);          // :66
    double GreatCircleDistance(const Vector3d& fromSrfNrm) const;   // :79
    ~SpaceCenter();

private:
    double latitude_ = 0.0;
    double longitude_ = 0.0;
    Vector3d srfNVector_;
    bool registered_ = false;
};

}  // namespace ksp
