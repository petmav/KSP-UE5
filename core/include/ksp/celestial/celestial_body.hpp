// CelestialBody (CelestialBody.cs, 1,231 lines): the data model of a planet/moon/star and every number KSP derives
// from it (mass, gravParameter, SOI, Hill sphere, rotation, atmosphere pressure/temperature/density/speed of sound,
// solar attenuation, thermal constants), ported operation for operation.
//
// Engine-free: Unity transforms/renderers (bodyTransform, scaledBody, pqsController, afg, MapObject) are not here; the
// shell owns them. Orbit maths is NOT here either: a body holds its serialized orbital elements (orbitDriver->orbit)
// and, at run time, a non-owning link to its ksp::Orbit (ICelestialBodyOrbit; CelestialBodyOrbitAdapter in
// celestial_body_orbit.hpp, attached by CelestialSystem::Spawn). Everything that needs Kepler propagation goes through
// that link; Orbit code reaches the ksp::Orbit itself through BodyOrbit(body) (ksp/orbit/orbit.hpp).
//
// Prefab data (the 17 stock bodies, their curves, biome maps and asset origins) lives in kerbol_system.hpp.
#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "ksp/curves/float_curve.hpp"
#include "ksp/math/quaterniond.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class CelestialBody;
namespace KSPAchievements {
class CelestialBodySubtree;  // WP-36 (CelestialBody.progressTree)
}
class Orbit;
class PQS;

// CelestialBodyType.cs
enum class CelestialBodyType : int32_t { Generic = 0 };

// CelestialBodyScienceParams.cs (C# initialisers; the prefab overwrites them per body)
struct CelestialBodyScienceParams {
    float LandedDataValue = 1.0f;
    float SplashedDataValue = 1.0f;
    float FlyingLowDataValue = 1.0f;
    float FlyingHighDataValue = 1.0f;
    float InSpaceLowDataValue = 1.0f;
    float InSpaceHighDataValue = 1.0f;
    float RecoveryValue = 1.0f;
    float flyingAltitudeThreshold = 18000.0f;
    float spaceAltitudeThreshold = 250000.0f;
};

// Origin of one of the game's own asset objects (CLAUDE.md "Original game assets"): the Unity asset file relative to
// the game root (decompiled/KSP-dev/), the object's PathID in it, its Unity/KSP type and name, and the field that
// consumes it. Later packages decode exactly this object; nothing is substituted. Index produced by
// tools/extract_body_assets.py (full index incl. PQS surface materials: data/kerbol_body_assets.json).
struct GameAssetRef {
    std::string role;   // consumer, e.g. "BiomeMap", "PQSMod_VertexHeightMap.heightMap", "scaledVersion.mesh", "scaled:_MainTex"
    std::string file;   // e.g. "KSP_x64_Data/sharedassets9.assets"
    int64_t pathID = 0;
    std::string type;   // Texture2D, MapSO, CBAttributeMapSO, CBTextureAtlasSO, Mesh, Material, ...
    std::string name;
    int32_t width = 0;   // Texture2D size, or MapSO _width/_height
    int32_t height = 0;
    int32_t format = 0;  // Texture2D m_TextureFormat, or MapSO _bpp
    std::string stream;  // Texture2D pixel data location (e.g. "KSP_x64_Data/sharedassets9.assets.resS"), MapSO: ""
    int64_t streamOffset = 0;
    int64_t streamSize = 0;  // bytes (MapSO: _data length, stored inside the .assets file)
};

// CBAttributeMapSO (CBAttributeMapSO.cs): a body's biome map. The metadata and the attribute table are the game's
// own (copied from the asset); the bitmap (_data, width*height*bpp bytes) stays in the asset file named by `origin`
// and is decoded by a later package (MapSO pixel access + GetAtt).
struct CBAttributeMapSO {
    struct MapAttribute {  // CBAttributeMapSO.cs:9
        Color mapColor = Color::white();
        std::string name;
        std::string localizationTag;
        std::string displayname;  // Localizer.Format(localizationTag) at Awake (localization: later package)
        float value = 0.0f;
        bool notNear = false;
    };
    GameAssetRef origin;
    std::string mapName;  // MapSO._name
    int32_t width = 0, height = 0, bpp = 0, rowWidth = 0;
    bool isCompiled = false;
    std::vector<MapAttribute> Attributes;
    bool exactSearch = false;
    float nonExactThreshold = -1.0f;
    float neighborColorThresh = 0.0009765625f;

    // CBAttributeMapSO.cs:148 RGBColorSqrMag (managed float code: Mono float rule)
    static float RGBColorSqrMag(const Color& colA, const Color& colB);
};

// MiniBiome (MiniBiome.cs): a named sub-biome volume (KSC buildings, ...); the collider geometry is in the asset.
struct MiniBiome {
    GameAssetRef origin;
    std::string gameObject;
    std::string TagKey;
    std::string LocalizedTag;
    std::string TagKeyID;
};

// The orbital elements serialized on a body's OrbitDriver (Orbit fields of the PSystem prefab). Input of ksp::Orbit
// (WP-07): Orbit(inclination, eccentricity, semiMajorAxis, LAN, argumentOfPeriapsis, meanAnomalyAtEpoch, epoch,
// referenceBody) then Orbit.Init().
struct CelestialOrbitElements {
    double inclination = 0.0;          // deg
    double eccentricity = 0.0;
    double semiMajorAxis = 0.0;        // m
    double LAN = 0.0;                  // deg
    double argumentOfPeriapsis = 0.0;  // deg
    double meanAnomalyAtEpoch = 0.0;   // rad
    double epoch = 0.0;                // s
    int32_t objectType = 0;            // Orbit.ObjectType (serialized)
};

// What CelestialBody reads from its run-time Orbit object (CelestialBody.orbit => orbitDriver.orbit): exactly the
// members used in CelestialBody.cs. Implemented over ksp::Orbit by CelestialBodyOrbitAdapter (celestial_body_orbit.hpp;
// an adapter because Orbit's fields period/radius/vel/trueAnomaly carry the names of these methods); CelestialBody
// never does Kepler maths itself.
class ICelestialBodyOrbit {
public:
    virtual ~ICelestialBodyOrbit() = default;
    // The ksp::Orbit behind this link: CelestialBody.orbit as Orbit/OrbitUtil/Targeting code reads it (BodyOrbit()).
    // nullptr for links that are not backed by an Orbit (test doubles); Orbit code then cannot use the body as a parent.
    virtual Orbit* GetOrbit() = 0;
    virtual double period() const = 0;       // Orbit.period (tidal lock, solar day length)
    virtual double trueAnomaly() const = 0;  // GetAtmoThermalStats
    virtual double radius() const = 0;       // GetAtmoThermalStats
    virtual double PeR() const = 0;
    virtual double ApR() const = 0;
    virtual double eccentricity() const = 0;  // GetAtmoThermalStats
    virtual double inclination() const = 0;   // GetAtmoThermalStats
    virtual Vector3d vel() const = 0;        // GetFrameVel
    virtual Vector3d getRelativePositionAtUT(double UT) const = 0;  // getPositionAtUT / getTruePositionAtUT
    virtual Vector3d getOrbitalVelocityAtUT(double UT) const = 0;   // GetFrameVelAtUT
    // OrbitDriver.UpdateOrbit() as called by CBUpdate (updates the orbit to Planetarium UT and moves the body).
    virtual void UpdateOrbit() = 0;
};

// OrbitDriver, as far as a CelestialBody uses it (OrbitDriver.cs is WP-07/08).
struct CelestialBodyOrbitDriver {
    CelestialOrbitElements orbit;            // serialized elements
    CelestialBody* referenceBody = nullptr;  // orbit.referenceBody, wired by the spawn (PSystemManager.SpawnBody)
    bool reverse = false;                    // Kerbin's driver is reversed (KSP keeps the home body still)
    int32_t updateMode = 0;                  // OrbitDriver.UpdateMode (serialized)
    bool enabled = true;                     // MonoBehaviour.enabled
    ICelestialBodyOrbit* link = nullptr;     // run-time Orbit (not owned; CelestialSystem owns it), set by Spawn
};

class CelestialBody {
public:
    // ---- serialized fields (CelestialBody.cs:12-262), C# initialisers; the prefab overwrites them ----
    std::string bodyName = "Unnamed";
    std::string bodyDisplayName = "Unnamed";
    std::string bodyAdjectiveDisplayName = "Unnamed";
    std::string bodyDescription = "A mysterious uncharted celestial body.";
    double GeeASL = 0.0;
    double Radius = 0.0;
    double Mass = 0.0;
    double Density = 0.0;
    double SurfaceArea = 0.0;
    double gravParameter = 0.0;
    double sphereOfInfluence = std::numeric_limits<double>::infinity();
    double hillSphere = 0.0;
    double gMagnitudeAtCenter = 0.0;
    double atmDensityASL = 0.0;
    double atmPressureASL = 0.0;
    bool scaledEllipsoid = true;
    Vector3d scaledElipRadMult{1.0, 1.0, 1.0};
    double scaledRadiusHorizonMultiplier = 1.0;
    double navballSwitchRadiusMult = 0.06;
    double navballSwitchRadiusMultLow = 0.055;
    bool use_The_InName = false;
    bool isHomeWorld = false;
    bool isCometPerturber = false;
    bool ocean = false;
    bool oceanUseFog = true;
    double oceanFogPQSDepth = 1000.0;
    float oceanFogPQSDepthRecip = 0.001f;
    float oceanFogDensityStart = 0.015f;
    float oceanFogDensityEnd = 0.13f;
    float oceanFogDensityPQSMult = 0.02f;
    float oceanFogDensityAltScalar = -0.0008f;
    float oceanFogDensityExponent = 1.0f;
    Color oceanFogColorStart{0.0f, 0.3372549f, 0.4862745f, 1.0f};
    Color oceanFogColorEnd{0.0f, 0.084f, 0.122f, 1.0f};
    float oceanFogDawnFactor = 10.0f;
    float oceanSkyColorMult = 1.2f;
    float oceanSkyColorOpacityBase = 0.2f;
    float oceanSkyColorOpacityAltMult = 2.0f;
    double oceanDensity = 1.0;
    float oceanAFGBase = 0.6f;
    float oceanAFGAltMult = 0.05f;
    float oceanAFGMin = 0.05f;
    float oceanSunBase = 0.5f;
    float oceanSunAltMult = 0.01f;
    float oceanSunMin = 0.05f;
    bool oceanAFGLerp = false;
    float oceanMinAlphaFogDistance = 200.0f;
    float oceanMaxAlbedoFog = 0.95f;
    float oceanMaxAlphaFog = 0.9f;
    float oceanAlbedoDistanceScalar = 0.01f;
    float oceanAlphaDistanceScalar = 0.01f;
    double minOrbitalDistance = 100000.0;
    bool atmosphere = false;
    bool atmosphereContainsOxygen = false;
    double atmosphereDepth = 0.0;
    double atmosphereTemperatureSeaLevel = 288.0;
    double atmospherePressureSeaLevel = 101.325;
    double atmosphereMolarMass = 0.0289644;
    double atmosphereAdiabaticIndex = 1.4;
    double atmosphereTemperatureLapseRate = 0.0;
    double atmosphereGasMassLapseRate = 0.0;
    bool atmosphereUseTemperatureCurve = false;
    bool atmosphereTemperatureCurveIsNormalized = false;
    FloatCurve atmosphereTemperatureCurve;
    FloatCurve latitudeTemperatureBiasCurve{std::vector<Keyframe>{Keyframe(0.0f, 0.0f)}};
    FloatCurve latitudeTemperatureSunMultCurve{std::vector<Keyframe>{Keyframe(0.0f, 0.0f)}};
    FloatCurve axialTemperatureSunMultCurve{std::vector<Keyframe>{Keyframe(0.0f, 0.0f)}};
    FloatCurve axialTemperatureSunBiasCurve{std::vector<Keyframe>{Keyframe(0.0f, 0.0f)}};
    FloatCurve atmosphereTemperatureSunMultCurve{std::vector<Keyframe>{Keyframe(0.0f, 0.0f)}};
    double maxAxialDot = 0.0;
    FloatCurve eccentricityTemperatureBiasCurve{std::vector<Keyframe>{Keyframe(0.0f, 0.0f)}};
    double albedo = 0.35;
    double emissivity = 0.65;
    double coreTemperatureOffset = 0.0;
    double convectionMultiplier = 1.0;
    double shockTemperatureMultiplier = 1.0;
    double bodyTemperature = 0.0;
    bool atmosphereUsePressureCurve = false;
    bool atmospherePressureCurveIsNormalized = false;
    FloatCurve atmospherePressureCurve;
    double radiusAtmoFactor = 1.0;
    bool hasSolidSurface = false;
    PQS* pqsController = nullptr;  // CelestialBody.pqsController (non-owning; null = no terrain sphere attached)
    bool isStar = false;
    Vector3d transformRight;
    Vector3d transformUp;
    bool rotates = false;
    double rotationPeriod = 0.0;
    double rotPeriodRecip = 0.0;
    double solarDayLength = 0.0;
    bool solarRotationPeriod = false;
    double initialRotation = 0.0;
    double rotationAngle = 0.0;
    double directRotAngle = 0.0;
    Vector3d angularVelocity;
    Vector3d zUpAngularVelocity;
    bool tidallyLocked = false;
    bool clampInverseRotThreshold = true;
    bool inverseRotation = false;
    float inverseRotThresholdAltitude = 15000.0f;
    double angularV = 0.0;
    std::vector<float> timeWarpAltitudeLimits;
    Color atmosphericAmbientColor;
    CelestialBodyType bodyType = CelestialBodyType::Generic;
    CelestialBodyScienceParams scienceValues;
    std::optional<CBAttributeMapSO> BiomeMap;  // null for the Sun and Jool
    std::vector<MiniBiome> MiniBiomes;

    // ---- run-time state (not serialized) ----
    std::optional<CelestialBodyOrbitDriver> orbitDriver;  // none for the Sun
    std::vector<CelestialBody*> orbitingBodies;           // children, PSystem order
    KSPAchievements::CelestialBodySubtree* progressTree = nullptr;  // CelestialBody.progressTree (WP-36: set by CelestialBodySubtree)
    int32_t flightGlobalsIndex = 0;                       // set by PSystemManager.SpawnBody
    Vector3d position;                                    // world position (KSP: bodyTransform.position; shell-driven)
    QuaternionD rotation;                                 // BodyFrame.Rotation.swizzle (CBUpdate)
    CelestialFrame BodyFrame;
    // Game assets consumed by the renderer/terrain (PQS height/colour maps, scaled-space mesh and textures), by origin.
    std::vector<GameAssetRef> assets;

    // ---- KSP properties ----
    const std::string& name() const { return bodyName; }
    const std::string& displayName() const { return bodyDisplayName; }
    std::string theName() const { return (use_The_InName ? "the " : "") + bodyName; }
    // CelestialBody.orbit (CelestialBody.cs:324): the run-time Orbit, null without an OrbitDriver (or before Spawn
    // attaches one). The ksp::Orbit itself: BodyOrbit(this) (ksp/orbit/orbit.hpp) or orbit()->GetOrbit().
    ICelestialBodyOrbit* orbit() const { return orbitDriver ? orbitDriver->link : nullptr; }
    // CelestialBody.referenceBody (CelestialBody.cs:348): the parent, or the body itself when it has no OrbitDriver
    CelestialBody* referenceBody() { return orbitDriver ? orbitDriver->referenceBody : this; }
    const CelestialBody* referenceBody() const { return orbitDriver ? orbitDriver->referenceBody : this; }
    static Vector3d RotationAxis() { return Vector3d::up(); }

    // ---- getters for ksp::Orbit / patched conics (WP-07/08): what an Orbit reads from its reference body ----
    double GetGravParameter() const { return gravParameter; }
    double GetSphereOfInfluence() const { return sphereOfInfluence; }
    double GetRadius() const { return Radius; }
    double GetMass() const { return Mass; }
    double GetAtmosphereDepth() const { return atmosphereDepth; }
    const std::string& GetName() const { return bodyName; }
    const CelestialBody* GetReferenceBody() const { return referenceBody(); }  // itself for the Sun
    const CelestialBody* GetParent() const { return orbitDriver ? orbitDriver->referenceBody : nullptr; }
    const CelestialOrbitElements* GetOrbitElements() const { return orbitDriver ? &orbitDriver->orbit : nullptr; }
    const Vector3d& GetPosition() const { return position; }
    Vector3d GetPositionAtUT(double UT) const { return getPositionAtUT(UT); }
    bool IsRotating() const { return rotates; }
    bool IsInverseRotation() const { return inverseRotation; }
    const Vector3d& GetAngularVelocity() const { return angularVelocity; }
    const Vector3d& GetZUpAngularVelocity() const { return zUpAngularVelocity; }

    // ---- life cycle (CelestialBody.cs:455-590) ----
    void Awake();     // :455 Mass, gravParameter, gMagnitudeAtCenter; registers with the parent's orbitingBodies
    void Start();     // :477 SOI, Hill sphere, solar->sidereal rotation period, SetupConstants, ocean fog recip
    void CBUpdate();  // :535 per physics frame (Planetarium.UpdateCBsRecursive)
    void resetTimeWarpLimits();  // :1013 (WP-10): timeWarpAltitudeLimits = TimeWarp.fetch.altitudeLimits * Radius * modifier
    void SetupConstants();  // :363 (minOrbitalDistance: see .cpp)

    // ---- atmosphere (CelestialBody.cs:387-453, 971-999) ----
    double GetPressure(double altitude) const;     // kPa
    double GetPressureAtm(double altitude) const;  // atm
    double GetTemperature(double altitude) const;  // K, without latitude/sun offset
    double GetDensity(double pressure, double temperature) const;
    double GetSpeedOfSound(double pressure, double density) const;
    double GetSolarPowerFactor(double density) const;
    void GetSolarAtmosphericEffects(double sunDot, double density, double& solarAirMass, double& solarFluxMultiplier) const;
    double GetFullTemperature(double altitude, double atmoSunBasedTempOffset) const;

    // GetAtmoThermalStats (CelestialBody.cs:855, WP-22). The Unity transform state it reads is the `frame` argument (the
    // shell fills it from scaledBody / bodyTransform / ScaledSpace); the sun is `sunBody`, the orbit values come from the
    // ICelestialBodyOrbit links of this body and of GetBodyReferencing(this, sunBody).
    struct AtmoThermalFrame {
        Vector3f sunScaledPos;       // sunBody.scaledBody.transform.position
        Vector3f scaledBodyPos;      // scaledBody.transform.position
        float scaleFactor = 6000.0f;  // ScaledSpace.ScaleFactor
        Vector3f bodyUp;             // bodyTransform.up
    };
    void GetAtmoThermalStats(bool getBodyFlux, const CelestialBody* sunBody, const Vector3d& sunVector, double sunDot,
                             const Vector3d& upAxis, double altitude, const AtmoThermalFrame& frame,
                             double& atmosphereTemperatureOffset, double& bodyEmissiveFlux, double& bodyAlbedoFlux);
    // GetFullTemperature(Vector3d pos) (CelestialBody.cs:991): the shell supplies Planetarium.fetch.Sun.scaledBody.transform.position -
    // ScaledSpace.LocalToScaledSpace(pos) (UNnormalised: the C# passes it as the sun vector), FlightGlobals.getUpAxis(this, pos) and
    // FlightGlobals.getAltitudeAtPos(pos, this). Returns the atmosphereTemperatureOffset (not a temperature, as in 1.12.5).
    double GetFullTemperature(const CelestialBody* sunBody, double altitudeAtPos, const Vector3d& sunMinusScaledPos,
                              const Vector3d& upAxis, const AtmoThermalFrame& frame);

    // ---- frames and surface coordinates (CelestialBody.cs:610-845, no PQS) ----
    Vector3d GetFrameVel() const;
    Vector3d GetFrameVelAtUT(double UT) const;
    Vector3d getRFrmVel(const Vector3d& worldPos) const;
    Vector3d getRFrmVelOrbit(const Orbit& o) const;  // CelestialBody.cs:633
    Vector3d getTruePositionAtUT(double UT) const;
    Vector3d getPositionAtUT(double UT) const;
    Vector3d GetRelSurfaceNVector(double lat, double lon) const;
    // CelestialBody.cs:831 (WP-10): 0 without a PQS; else pqsController.GetSurfaceHeight(n) - pqsController.radius,
    // clamped at 0 unless allowNegative. The sphere is attached by the host (ksp::terrain::LoadStockPQS).
    double TerrainAltitude(double latitude, double longitude, bool allowNegative = false) const;
    Vector3d GetSurfaceNVector(double lat, double lon) const;
    Vector3d GetRelSurfacePosition(double lat, double lon, double alt) const;
    Vector3d GetRelSurfacePosition(const Vector3d& worldPosition) const;
    Vector3d GetRelSurfaceDirection(const Vector3d& worldDirection) const;
    Vector3d GetWorldSurfacePosition(double lat, double lon, double alt) const;
    double GetLatitude(const Vector3d& pos, bool isRadial = false) const;
    double GetLongitude(const Vector3d& pos, bool isRadial = false) const;
    Vector2d GetLatitudeAndLongitude(const Vector3d& pos, bool isRadial = false) const;
    bool GetImpactLatitudeAndLongitude(const Vector3d& position, const Vector3d& velocity, double& latitude,
                                       double& longitude) const;
    double GetAltitude(const Vector3d& worldPos) const;
    void GetLatLonAlt(const Vector3d& worldPos, double& lat, double& lon, double& alt) const;
    void GetLatLonAltOrbital(const Vector3d& worldPos, double& lat, double& lon, double& alt) const;

    // ---- hierarchy (CelestialBody.cs:846, 1024-1060) ----
    static const CelestialBody* GetBodyReferencing(const CelestialBody* body, const CelestialBody* sun);
    bool HasParent(const CelestialBody* body) const;
    bool HasChild(const CelestialBody* body) const;
};

}  // namespace ksp
