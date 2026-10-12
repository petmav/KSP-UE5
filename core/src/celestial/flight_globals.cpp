// FlightGlobals (FlightGlobals.cs): the body-only half (no Vessel). The registry and the vessel-dependent members are in
// src/vessel/flight_globals_vessels.cpp. See include/ksp/celestial/flight_globals.hpp. C# paths: Assembly-CSharp/FlightGlobals.cs.
#include "ksp/celestial/flight_globals.hpp"

#include <algorithm>
#include <limits>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp {

namespace M = dotnet::Math;
using GE = events::GameEvents;

namespace {
// `fetch != null`: a FlightGlobals instance with a body list exists
bool FetchPresent() { return FlightGlobals::bodies != nullptr || !SaveEnvironment::Current().localBodies.empty(); }
bool InBodies(const CelestialBody* b) {
    const auto& v = FlightGlobals::Bodies();
    return std::find(v.begin(), v.end(), b) != v.end();
}
}  // namespace

// FlightGlobals.cs:132 (+ SaveEnvironment::localBodies when no scene registered `bodies`)
const std::vector<CelestialBody*>& FlightGlobals::Bodies() {
    if (bodies != nullptr) return *bodies;
    return SaveEnvironment::Current().localBodies;
}

// FlightGlobals.cs:1311 getMainBody(refPos) => inSOI(refPos, fetch.bodies[0])
CelestialBody* FlightGlobals::getMainBody(const Vector3d& refPos) {
    if (bodies == nullptr) throw NullReferenceException("FlightGlobals.getMainBody: no FlightGlobals instance");
    if (bodies->empty()) throw std::out_of_range("FlightGlobals.getMainBody: bodies is empty");
    return inSOI(refPos, (*bodies)[0]);
}

// FlightGlobals.cs:1316
CelestialBody* FlightGlobals::inSOI(const Vector3d& pos, CelestialBody* body) {
    const size_t count = body->orbitingBodies.size();
    if (count > 0) {
        for (size_t i = 0; i < count; ++i) {
            CelestialBody* celestialBody = body->orbitingBodies[i];
            if (!((pos - celestialBody->position).sqrMagnitude() >=
                  celestialBody->sphereOfInfluence * celestialBody->sphereOfInfluence))
                return inSOI(pos, celestialBody);
        }
    }
    return body;
}

// FlightGlobals.cs:1342
Vector3d FlightGlobals::getGeeForceAtPosition(const Vector3d& pos) { return getGeeForceAtPosition(pos, *getMainBody(pos)); }

// FlightGlobals.cs:1347 (the same arithmetic as PrecalcMath::GeeForceAtPosition, kept in this TU for the registry's callers)
Vector3d FlightGlobals::getGeeForceAtPosition(const Vector3d& posIn, const CelestialBody& mainBody) {
    Vector3d pos = posIn - mainBody.position;
    double sqrMagnitude = pos.sqrMagnitude();
    sqrMagnitude *= M::Sqrt(sqrMagnitude);
    return pos * (0.0 - mainBody.gMagnitudeAtCenter) / sqrMagnitude;
}

// FlightGlobals.cs:1356
Vector3d FlightGlobals::getCentrifugalAcc(const Vector3d& posIn, const CelestialBody& body) {
    if (!body.inverseRotation) return Vector3d::zero();
    const Vector3d pos = body.position - posIn;
    return Vector3d::Cross(body.angularVelocity, Vector3d::Cross(body.angularVelocity, pos));
}

// FlightGlobals.cs:1366
Vector3d FlightGlobals::getCoriolisAcc(const Vector3d& vel, const CelestialBody& body) {
    if (!body.inverseRotation) return Vector3d::zero();
    return Vector3d::Cross(body.angularVelocity, vel) * -2.0;
}

// FlightGlobals.cs:1380
double FlightGlobals::getExternalTemperature(double altitude, CelestialBody* body) {
    if (body == nullptr) body = currentMainBody;
    if (body == nullptr) throw NullReferenceException("FlightGlobals.getExternalTemperature: no currentMainBody");
    if (!body->atmosphere) return 0.0;
    return body->GetTemperature(altitude);
}

// FlightGlobals.cs:1393
double FlightGlobals::getExternalTemperature(const Vector3d& pos, CelestialBody* body) {
    if (body == nullptr) body = currentMainBody;
    if (body == nullptr) throw NullReferenceException("FlightGlobals.getExternalTemperature: no currentMainBody");
    if (!body->atmosphere) return 0.0;
    return body->GetTemperature(getAltitudeAtPos(pos, body));
}

// FlightGlobals.cs:1406
double FlightGlobals::getStaticPressure(double altitude, CelestialBody* body) {
    if (body == nullptr) body = currentMainBody;
    if (body == nullptr) throw NullReferenceException("FlightGlobals.getStaticPressure: no currentMainBody");
    if (!body->atmosphere) return 0.0;
    return body->GetPressure(altitude);
}

// FlightGlobals.cs:1429
double FlightGlobals::getStaticPressure(const Vector3d& position, const CelestialBody& body) {
    return body.GetPressure(getAltitudeAtPos(position, const_cast<CelestialBody*>(&body)));
}

// FlightGlobals.cs:1443
double FlightGlobals::getAtmDensity(double pressure, double temperature, CelestialBody* body) {
    if (body == nullptr) body = currentMainBody;
    if (body == nullptr) throw NullReferenceException("FlightGlobals.getAtmDensity: no currentMainBody");
    if (!body->atmosphere) return 0.0;
    return body->GetDensity(pressure, temperature);
}

// FlightGlobals.cs:1456
double FlightGlobals::getAltitudeAtPos(const Vector3d& position) { return getAltitudeAtPos(position, getMainBody(position)); }

// FlightGlobals.cs:1461: `(float)getAltitudeAtPos((Vector3d)position, getMainBody(position))`
float FlightGlobals::getAltitudeAtPos(const Vector3f& position) {
    return static_cast<float>(getAltitudeAtPos(Vector3d(position), getMainBody(Vector3d(position))));
}

// FlightGlobals.cs:1466: `return (float)getAltitudeAtPos((Vector3d)position, body);`
float FlightGlobals::getAltitudeAtPos(const Vector3f& position, CelestialBody* body) {
    return static_cast<float>(getAltitudeAtPos(Vector3d(position), body));
}

// FlightGlobals.cs:1471
double FlightGlobals::getAltitudeAtPos(const Vector3d& position, CelestialBody* body) {
    return Vector3d::Distance(position, body->position) - body->Radius;
}

// FlightGlobals.cs:1476
double FlightGlobals::GetSqrAltitude(const Vector3d& position, const CelestialBody& body) {
    return (position - body.position).sqrMagnitude() - body.Radius * body.Radius;
}

// FlightGlobals.cs:1486
Vector3d FlightGlobals::getUpAxis(const Vector3d& position) { return (position - getMainBody(position)->position).normalized(); }

// FlightGlobals.cs:1491
Vector3d FlightGlobals::getUpAxis(const CelestialBody& body, const Vector3d& position) { return (position - body.position).normalized(); }

// FlightGlobals.cs:1300 clearInverseRotation / :1300
void FlightGlobals::ClearInverseRotation() {
    for (CelestialBody* b : Bodies()) b->inverseRotation = false;
}

// FlightGlobals.cs:1736
CelestialBody* FlightGlobals::GetBodyByName(const std::string& bodyName) {
    if (FetchPresent()) {
        auto it = bodyNames.find(bodyName);
        if (it != bodyNames.end()) {
            CelestialBody* celestialBody = it->second;
            if (celestialBody != nullptr && InBodies(celestialBody)) return celestialBody;  // `!= null`: a destroyed body is gone from the list
        }
        const auto& all = Bodies();
        int count = static_cast<int>(all.size());
        while (count-- > 0) {
            CelestialBody* celestialBody2 = all[static_cast<size_t>(count)];
            if (celestialBody2->name() == bodyName) {
                bodyNames[bodyName] = celestialBody2;
                return celestialBody2;
            }
        }
        return nullptr;
    }
    return nullptr;
}

// FlightGlobals.cs:1768
int FlightGlobals::GetBodyIndex(const CelestialBody* body) {
    const auto& all = Bodies();
    const int count = static_cast<int>(all.size());
    for (int i = 0; i < count; i++)
        if (all[static_cast<size_t>(i)] == body) return i;
    return -1;
}

// FlightGlobals.cs:1788
int FlightGlobals::GetHomeBodyIndex() {
    if (homeBody == nullptr || !InBodies(homeBody)) homeBody = GetHomeBody();
    return homeBodyIndex;
}

// FlightGlobals.cs:1797
CelestialBody* FlightGlobals::GetHomeBody() {
    const bool fetch = FetchPresent();
    if (!fetch || homeBody == nullptr || !InBodies(homeBody)) {
        const auto& all = Bodies();
        const int count = static_cast<int>(all.size());
        for (int i = 0; i < count; i++) {
            if (all[static_cast<size_t>(i)]->isHomeWorld) {
                if (!fetch) return all[static_cast<size_t>(i)];
                homeBody = all[static_cast<size_t>(i)];
                homeBodyIndex = i;
                break;
            }
        }
    }
    if (!fetch) return Bodies().at(0);
    return homeBody;
}

// FlightGlobals.cs:1823
std::string FlightGlobals::GetHomeBodyName() {
    if (homeBody == nullptr || !InBodies(homeBody)) homeBody = GetHomeBody();
    if (homeBody == nullptr) throw NullReferenceException("FlightGlobals.GetHomeBodyName: no home body");
    return homeBody->name();
}

// FlightGlobals.cs:1832
std::string FlightGlobals::GetHomeBodyDisplayName() {
    if (homeBody == nullptr || !InBodies(homeBody)) homeBody = GetHomeBody();
    if (homeBody == nullptr) throw NullReferenceException("FlightGlobals.GetHomeBodyDisplayName: no home body");
    return homeBody->displayName();
}

// FlightGlobals.cs:1841
int FlightGlobals::GetCometPerturberIndex() {
    if (cometPerturber == nullptr || !InBodies(cometPerturber)) cometPerturber = GetHomeBody();
    return cometPerturberIndex;
}

namespace {
// CelestialBody.orbit.referenceBody / semiMajorAxis (the Sun has no orbit: the C# reads an OrbitDriver with no reference body, semiMajorAxis 0)
CelestialBody* RefBodyOf(const CelestialBody* b) {
    ICelestialBodyOrbit* o = b->orbit();
    Orbit* orbit = o != nullptr ? o->GetOrbit() : nullptr;
    return orbit != nullptr ? orbit->referenceBody : nullptr;
}
double SmaOf(const CelestialBody* b) {
    ICelestialBodyOrbit* o = b->orbit();
    Orbit* orbit = o != nullptr ? o->GetOrbit() : nullptr;
    return orbit != nullptr ? orbit->semiMajorAxis : 0.0;
}
// FlightGlobals.cs:1881
CelestialBody* GetMidSystemPlanet(const std::vector<CelestialBody*>& all, CelestialBody* parentBody) {
    const int count = static_cast<int>(all.size());
    double num = std::numeric_limits<double>::max();
    double num2 = std::numeric_limits<double>::lowest();
    for (int i = 0; i < count; i++) {
        if (RefBodyOf(all[static_cast<size_t>(i)]) != parentBody) continue;
        if (SmaOf(all[static_cast<size_t>(i)]) > num2) num2 = SmaOf(all[static_cast<size_t>(i)]);
        if (SmaOf(all[static_cast<size_t>(i)]) < num) num = SmaOf(all[static_cast<size_t>(i)]);
    }
    const double num3 = (num + num2) / 2.0;
    CelestialBody* result = all[0];
    double num4 = std::numeric_limits<double>::max();
    for (int j = 0; j < count; j++) {
        if (M::Abs(SmaOf(all[static_cast<size_t>(j)]) - num3) < num4) {
            num4 = M::Abs(SmaOf(all[static_cast<size_t>(j)]) - num3);
            result = all[static_cast<size_t>(j)];
        }
    }
    return result;
}
}  // namespace

// FlightGlobals.cs:1850
CelestialBody* FlightGlobals::GetCometPerturber() {
    const bool fetch = FetchPresent();
    if (!fetch || cometPerturber == nullptr || !InBodies(cometPerturber)) {
        const auto& all = Bodies();
        const int count = static_cast<int>(all.size());
        for (int i = 0; i < count; i++) {
            if (all[static_cast<size_t>(i)]->isCometPerturber) {
                if (!fetch) return all[static_cast<size_t>(i)];
                cometPerturber = all[static_cast<size_t>(i)];
                cometPerturberIndex = i;
                break;
            }
        }
    }
    if (!fetch) {
        CelestialBody* sun = Planetarium::fetch != nullptr ? Planetarium::fetch->Sun : nullptr;
        if (sun == nullptr) return Bodies().at(0);
        return GetMidSystemPlanet(Bodies(), sun);
    }
    return cometPerturber;
}

// FlightGlobals.cs:1914
std::string FlightGlobals::GetCometPerturberName() {
    if (cometPerturber == nullptr || !InBodies(cometPerturber)) cometPerturber = GetHomeBody();
    if (cometPerturber == nullptr) throw NullReferenceException("FlightGlobals.GetCometPerturberName: no body");
    return cometPerturber->name();
}

// FlightGlobals.cs:1923
std::string FlightGlobals::GGetCometPerturberDisplayName() {
    if (cometPerturber == nullptr || !InBodies(cometPerturber)) cometPerturber = GetHomeBody();
    if (cometPerturber == nullptr) throw NullReferenceException("FlightGlobals.GGetCometPerturberDisplayName: no body");
    return cometPerturber->displayName();
}

// FlightGlobals.cs:1959
void FlightGlobals::SetSpeedMode(SpeedDisplayModes newSpeedMode) {
    speedMode = newSpeedMode;
    GE::onSetSpeedMode.Fire(speedDisplayMode());
}

// FlightGlobals.cs:1965
void FlightGlobals::CycleSpeedModes() {
    speedMode = static_cast<SpeedDisplayModes>((static_cast<int>(speedMode) + 1) % 3);
    if (speedDisplayMode() == SpeedDisplayModes::Target && VesselTarget() == nullptr) CycleSpeedModes();
    else SetSpeedMode(speedMode);
}

}  // namespace ksp
