// WP-11: every PQSMod that overrides OnVertexBuildHeight (28 classes, Assembly-CSharp/PQSMod_*.cs + PQSLandControl.cs),
// ported for height evaluation, plus PQSModPassive for the mods that do not change height (kept in the stack because
// they take part in List.Sort's ordering and in the requirements union).
//
// Members keep the C# field names and types (float stays float: those mods are managed float code and follow the
// Mono rule, ksp/dotnet/mono_float.hpp). Private C# fields that carry state between calls (MapDecal's quadActive /
// buildHeight / vertActive / u / v, VoronoiCraters' r, PQSLandControl's land-class deltas, ...) are kept as members
// too. Colour output (OnVertexBuild), scatter and materials are not ported (later packages); their serialized fields
// are kept where they are plain values.
//
// Used by the stock Kerbol system (data/terrain/pqs_prefab.json): PQSLandControl, FlattenArea, FlattenOcean, MapDecal,
// MapDecalTangent, VertexHeightMap, VertexHeightNoise, VertexHeightNoiseVertHeight, VertexHeightNoiseVertHeightCurve2,
// VertexHeightOffset, VertexPlanet, VertexRidgedAltitudeCurve, VertexSimplexHeight, VertexSimplexHeightAbsolute,
// VertexSimplexHeightMap, VertexVoronoi, VoronoiCraters. The other 11 are ported for modded bodies; two of them
// (VertexHeightMapStep, VertexHeightNoiseHeightMap) sample a Texture2D through Unity's native GetPixelBilinear, which
// is not ported: they need a sampler set by the caller (std::logic_error otherwise).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/terrain/libnoise.hpp"
#include "ksp/terrain/map_so.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/util/simplex.hpp"

namespace ksp {

using MapSORef = std::shared_ptr<const MapSO>;

// A mod class this port does not evaluate (no OnVertexBuildHeight override): order/modEnabled/GameObject state and
// the requirements its OnSetup assigns (generated from the C# source by tools/wp11/gen_wp11_terrain.py).
class PQSModPassive : public PQSMod {
public:
    // Requirements assigned by the class's OnSetup; -1 = OnSetup leaves the serialized value unchanged.
    int setupRequirements = 0;
    // PQSROCControl.Awake (PQSROCControl.cs:40): modExpansionDisabled = !ExpansionsLoader.IsExpansionInstalled("Serenity")
    bool isROCControl = false;
    void OnSetup() override;
};

// Texture2D.GetPixelBilinear(u, v).grayscale (native Unity sampling; not ported): supplied by the caller.
using Texture2DGrayscaleSampler = std::function<double(float u, float v)>;

enum class PQSNoiseType : int { Perlin = 0, RidgedMultifractal = 1, Billow = 2 };  // PQSMod_VertexHeightNoise*.NoiseType

// ---- PQSLandControl.cs ---------------------------------------------------------------------------------------------
class PQSLandControl : public PQSMod {
public:
    // PQSLandControl.cs:10
    struct LerpRange {
        double startStart = 0.0, startEnd = 0.0, endStart = 0.0, endEnd = 0.0;
        double startDelta = 0.0, endDelta = 0.0;
        void Setup();                      // :26
        double Lerp(double point) const;   // :32
    };
    // PQSLandControl.cs:122 (scatter amounts: names/densities only; scatter objects are not ported)
    struct LandClassScatterAmount {
        std::string scatterName;
        double density = 0.0;
    };
    // PQSLandControl.cs:55
    struct LandClass {
        std::string landClassName;
        Color color, noiseColor;
        LerpRange altitudeRange, latitudeRange;
        bool latitudeDouble = false;
        LerpRange latitudeDoubleRange, longitudeRange;
        float coverageBlend = 0.0f;
        int coverageSeed = 0, coverageOctaves = 0;
        float coveragePersistance = 0.0f, coverageFrequency = 0.0f;
        std::shared_ptr<Simplex> coverageSimplex;
        float noiseBlend = 0.0f;
        int noiseSeed = 0, noiseOctaves = 0;
        float noisePersistance = 0.0f, noiseFrequency = 0.0f;
        std::shared_ptr<Simplex> noiseSimplex;
        double minimumRealHeight = 0.0, alterRealHeight = 0.0;
        float alterApparentHeight = 0.0f;
        double altDelta = 0.0, latDelta = 0.0, lonDelta = 0.0, delta = 0.0;
        std::vector<LandClassScatterAmount> scatter;
    };

    std::vector<LandClass> landClasses;
    bool useHeightMap = false;
    MapSORef heightMap;
    float vHeightMax = 0.0f;
    bool createColors = false, createScatter = false;
    float altitudeBlend = 0.0f;
    int altitudeSeed = 0, altitudeOctaves = 0;
    float altitudePersistance = 0.0f, altitudeFrequency = 0.0f;
    float latitudeBlend = 0.0f;
    int latitudeSeed = 0, latitudeOctaves = 0;
    float latitudePersistance = 0.0f, latitudeFrequency = 0.0f;
    float longitudeBlend = 0.0f;
    int longitudeSeed = 0, longitudeOctaves = 0;
    float longitudePersistance = 0.0f, longitudeFrequency = 0.0f;
    std::shared_ptr<Simplex> altitudeSimplex, latitudeSimplex, longitudeSimplex;

    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
    static double Lerp(double v1, double v2, double dt);  // :885

    // evaluation state (C# private fields)
    double vHeight = 0.0, vLat = 0.0, vLon = 0.0, totalDelta = 0.0;
    std::vector<LandClass*> lcList;
    int lcListCount = 0;
};

// ---- flatten -------------------------------------------------------------------------------------------------------
class PQSMod_FlattenArea : public PQSMod {
public:
    double outerRadius = 0.0, innerRadius = 0.0;
    Vector3f position;
    double flattenTo = 0.0;
    bool removeScatter = false, DEBUG_showColors = false;
    double smoothStart = 0.0, smoothEnd = 0.0;
    bool useLatLon = false;
    std::string bodyName;
    double lat = 0.0, lon = 0.0, alt = 0.0;
    void OnSetup() override;
    void OnQuadPreBuild(const PQ& quad) override;
    void OnQuadBuilt(const PQ& quad) override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
    double CubicHermite(double start, double end, double startTangent, double endTangent, double t);
    // private state
    double flattenToRadius = 0.0, angleInner = 0.0, angleOuter = 0.0, angleQuadInclusion = 0.0, angleDelta = 0.0;
    bool quadActive = false;
    Vector3d posNorm;
    double testAngle = 0.0, aDelta = 0.0;
};

class PQSMod_FlattenAreaTangential : public PQSMod {
public:
    double outerRadius = 0.0, innerRadius = 0.0;
    Vector3f position;
    double flattenTo = 0.0, smoothStart = 0.0, smoothEnd = 0.0;
    bool DEBUG_showColors = false;
    void OnSetup() override;
    void OnQuadPreBuild(const PQ& quad) override;
    void OnQuadBuilt(const PQ& quad) override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
    double CubicHermite(double start, double end, double startTangent, double endTangent, double t);
    double angleInner = 0.0, angleOuter = 0.0, angleQuadInclusion = 0.0, angleDelta = 0.0;
    bool quadActive = false;
    Vector3d posNorm;
    double testAngle = 0.0, aDelta = 0.0, flattenToRadius = 0.0, vHeight = 0.0;
};

class PQSMod_FlattenOcean : public PQSMod {
public:
    double oceanRadius = 0.0;
    double oceanRad = 0.0;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

// ---- decals --------------------------------------------------------------------------------------------------------
// PQSMod_MapDecal (absoluteOffset is float) and PQSMod_MapDecalTangent (double, height divided by the cosine to the
// decal centre, quad inclusion with <=) share this layout.
class PQSMod_MapDecalBase : public PQSMod {
public:
    double radius = 0.0;
    Vector3f position;
    float angle = 0.0f;
    MapSORef heightMap;
    double heightMapDeformity = 0.0;
    bool cullBlack = false, useAlphaHeightSmoothing = false, absolute = false;
    std::string colorMapKey;  // colorMap (colour output not ported): its map key
    float smoothHeight = 0.0f, smoothColor = 0.0f;
    bool removeScatter = false, DEBUG_HighlightInclusion = false;

    void OnQuadBuilt(const PQ& quad) override;  // MapDecal.cs:273 / MapDecalTangent.cs:271
    bool AffectsHeight() const override { return true; }
    static double Lerp(double v2, double v1, double dt);

    // private state (C# fields; quadActive/buildHeight start false = "no quad built yet")
    double inclusionAngle = 0.0;
    bool quadActive = false, vertActive = false, buildHeight = false;
    Vector3d posNorm;
    double quadAngle = 0.0;
    Vector3d vertRot;
    Quaternionf rot;
    float u = 0.0f, v = 0.0f;
    MapSO::HeightAlpha ha;
    float smoothCR = 0.0f, smoothC1M = 0.0f, smoothHR = 0.0f, smoothH1M = 0.0f, smoothU = 0.0f, smoothV = 0.0f;
    float smoothFactor = 0.0f;

protected:
    void SetupCommon();
    float GetHeightSmoothing(float u_, float v_);
    void VisitCommon(const SerializedFieldVisitor& vis) const;
};

class PQSMod_MapDecal : public PQSMod_MapDecalBase {
public:
    float absoluteOffset = 0.0f;
    void OnSetup() override;
    void OnQuadPreBuild(const PQ& quad) override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_MapDecalTangent : public PQSMod_MapDecalBase {
public:
    double absoluteOffset = 0.0;
    void OnSetup() override;
    void OnQuadPreBuild(const PQ& quad) override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

// ---- smoothing / coast / offsets ------------------------------------------------------------------------------------
class PQSMod_SmoothLatitudeRange : public PQSMod {
public:
    PQSLandControl::LerpRange latitudeRange;
    double smoothToAltitude = 0.0;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return smoothToAltitude; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexDefineCoastLine : public PQSMod {
public:
    double oceanRadiusOffset = 0.0, depthOffset = 0.0;
    double oceanRadius = 0.0;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMinHeight() override { return depthOffset; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexHeightOffset : public PQSMod {
public:
    double offset = 0.0;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexHeightOblate : public PQSMod {
public:
    double height = 0.0, pow = 0.0;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

// ---- height maps ---------------------------------------------------------------------------------------------------
class PQSMod_VertexHeightMap : public PQSMod {
public:
    MapSORef heightMap;
    double heightMapDeformity = 0.0, heightMapOffset = 0.0;
    bool scaleDeformityByRadius = false;
    double heightDeformity = 0.0;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return heightMapOffset + heightDeformity; }
    double GetVertexMinHeight() override { return heightMapOffset; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexHeightMapStep : public PQSMod {
public:
    std::string heightMapTexture;  // Texture2D reference (asset name)
    Texture2DGrayscaleSampler heightMapSampler;
    double heightMapDeformity = 0.0, heightMapOffset = 0.0;
    bool scaleDeformityByRadius = false;
    double coastHeight = 0.0;
    double heightDeformity = 0.0, h = 0.0;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return heightMapOffset + heightDeformity; }
    double GetVertexMinHeight() override { return heightMapOffset; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexSimplexHeightMap : public PQSMod {
public:
    int seed = 0;
    double deformity = 0.0, octaves = 0.0, persistence = 0.0, frequency = 0.0;
    MapSORef heightMap;
    float heightStart = 0.0f, heightEnd = 0.0f;
    std::shared_ptr<Simplex> simplex;
    double hDeltaR = 0.0, h = 0.0;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return deformity; }
    double GetVertexMinHeight() override { return 0.0 - deformity; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

// ---- simplex -------------------------------------------------------------------------------------------------------
class PQSMod_VertexSimplexHeight : public PQSMod {
public:
    int seed = 0;
    double deformity = 0.0, octaves = 0.0, persistence = 0.0, frequency = 0.0;
    std::shared_ptr<Simplex> simplex;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return deformity; }
    double GetVertexMinHeight() override { return 0.0 - deformity; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexSimplexHeightAbsolute : public PQSMod_VertexSimplexHeight {
public:
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
};

class PQSMod_VertexSimplexHeightFlatten : public PQSMod {
public:
    int seed = 0;
    double cutoff = 0.0, deformity = 0.0, octaves = 0.0, persistence = 0.0, frequency = 0.0;
    std::shared_ptr<Simplex> simplex;
    double val = 0.0, valRatio = 0.0;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return deformity; }
    double GetVertexMinHeight() override { return 0.0 - deformity; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

// ---- LibNoise height noise -----------------------------------------------------------------------------------------
// PQSMod_VertexHeightNoise / NoiseHeightMap / NoiseVertHeight / NoiseVertHeightCurve share the noise parameters.
class PQSMod_NoiseParams : public PQSMod {
public:
    PQSNoiseType noiseType = PQSNoiseType::Perlin;
    float deformity = 0.0f;
    int seed = 0;
    float frequency = 0.0f, lacunarity = 0.0f, persistance = 0.0f;
    int octaves = 0;
    LibNoise::NoiseQuality mode = LibNoise::NoiseQuality::Low;
    std::shared_ptr<LibNoise::IModule> noiseMap;
    bool AffectsHeight() const override { return true; }

protected:
    void CreateNoiseMap();  // the OnSetup switch (Perlin by default)
    void VisitNoise(const SerializedFieldVisitor& v) const;
};

class PQSMod_VertexHeightNoise : public PQSMod_NoiseParams {
public:
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return deformity; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexHeightNoiseVertHeight : public PQSMod_NoiseParams {
public:
    float heightStart = 0.0f, heightEnd = 0.0f;
    double hDeltaR = 0.0, h = 0.0, n = 0.0;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return deformity; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexHeightNoiseHeightMap : public PQSMod_NoiseParams {
public:
    std::string heightMapTexture;
    Texture2DGrayscaleSampler heightMapSampler;
    float heightStart = 0.0f, heightEnd = 0.0f;
    double hDeltaR = 0.0, h = 0.0, n = 0.0;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return deformity; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexHeightNoiseVertHeightCurve : public PQSMod_NoiseParams {
public:
    AnimationCurve curve;
    float heightStart = 0.0f, heightEnd = 0.0f;
    double h = 0.0, n = 0.0;
    float t = 0.0f;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return deformity; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexHeightNoiseVertHeightCurve2 : public PQSMod {
public:
    float deformity = 0.0f;
    int ridgedAddSeed = 0;
    float ridgedAddFrequency = 0.0f, ridgedAddLacunarity = 0.0f;
    int ridgedAddOctaves = 0;
    int ridgedSubSeed = 0;
    float ridgedSubFrequency = 0.0f, ridgedSubLacunarity = 0.0f;
    int ridgedSubOctaves = 0;
    LibNoise::NoiseQuality ridgedMode = LibNoise::NoiseQuality::Low;
    AnimationCurve simplexCurve;
    double simplexHeightStart = 0.0, simplexHeightEnd = 0.0;
    int simplexSeed = 0;
    double simplexOctaves = 0.0, simplexPersistence = 0.0, simplexFrequency = 0.0;
    std::shared_ptr<Simplex> simplex;
    std::shared_ptr<LibNoise::RidgedMultifractal> ridgedAdd, ridgedSub;
    double hDeltaR = 0.0, h = 0.0, r = 0.0, s = 0.0;
    float t = 0.0f;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return deformity; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexHeightNoiseVertHeightCurve3 : public PQSMod {
public:
    struct RidgedNoise {  // :8
        int seed = 0;
        double frequency = 0.0, lacunarity = 0.0;
        int octaves = 0;
        LibNoise::NoiseQuality quality = LibNoise::NoiseQuality::Low;
        std::shared_ptr<LibNoise::RidgedMultifractal> fractal;
        void Setup();
        double Noise(const Vector3d& radial) const;
    };
    struct SimplexNoise {  // :40
        int seed = 0;
        double frequency = 0.0, persistence = 0.0;
        int octaves = 0;
        std::shared_ptr<Simplex> fractal;
        void Setup();
        double Noise(const Vector3d& radial) const;
        double NoiseNormalized(const Vector3d& radial) const;
    };
    double inputHeightStart = 0.0, inputHeightEnd = 0.0;
    AnimationCurve inputHeightCurve;
    SimplexNoise curveMultiplier;
    double deformityMin = 0.0, deformityMax = 0.0;
    SimplexNoise deformity;
    RidgedNoise ridgedAdd, ridgedSub;
    double hDeltaR = 0.0, h = 0.0, r = 0.0, s = 0.0, d = 0.0;
    float t = 0.0f;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return deformityMax; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
    static double Lerp(double a, double b, double t);  // :166
};

class PQSMod_VertexRidgedAltitudeCurve : public PQSMod {
public:
    float deformity = 0.0f;
    int ridgedAddSeed = 0;
    float ridgedAddFrequency = 0.0f, ridgedAddLacunarity = 0.0f;
    int ridgedAddOctaves = 0;
    float ridgedMinimum = 0.0f;
    LibNoise::NoiseQuality ridgedMode = LibNoise::NoiseQuality::Low;
    AnimationCurve simplexCurve;
    double simplexHeightStart = 0.0, simplexHeightEnd = 0.0;
    int simplexSeed = 0;
    double simplexOctaves = 0.0, simplexPersistence = 0.0, simplexFrequency = 0.0;
    std::shared_ptr<Simplex> simplex;
    std::shared_ptr<LibNoise::RidgedMultifractal> ridgedAdd;
    double hDeltaR = 0.0, h = 0.0, r = 0.0, s = 0.0;
    float t = 0.0f;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return deformity; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VertexNoise : public PQSMod {
public:
    int seed = 0;
    float noiseDeformity = 0.0f;
    int noisePasses = 0;
    float smoothness = 0.0f, falloff = 0.0f, mesaVsPlainsBias = 0.0f, plainsVsMountainSmoothness = 0.0f;
    float plainsVsMountainThreshold = 0.0f, plainSmoothness = 0.0f;
    std::shared_ptr<LibNoise::Modifiers::Select> terrainHeightMap;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return noiseDeformity; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

// ---- voronoi -------------------------------------------------------------------------------------------------------
class PQSMod_VertexVoronoi : public PQSMod {
public:
    double deformation = 1000.0;
    int voronoiSeed = 0;
    double voronoiDisplacement = 1.0, voronoiFrequency = 1.0;
    bool voronoiEnableDistance = false;
    std::shared_ptr<LibNoise::Voronoi> voronoi;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VoronoiCraters : public PQSMod {
public:
    double deformation = 1000.0;
    int voronoiSeed = 0;
    double voronoiDisplacement = 1.0, voronoiFrequency = 1.0;
    AnimationCurve craterCurve, jitterCurve;
    int simplexSeed = 123123;
    double simplexOctaves = 2.0, simplexPersistence = 0.5, simplexFrequency = 20.0;
    float jitter = 0.5f, jitterHeight = 0.1f;
    float rFactor = 0.5f, rOffset = 1.0f, colorOpacity = 1.0f;
    bool DebugColorMapping = false;
    std::shared_ptr<LibNoise::Voronoi> voronoi;
    std::shared_ptr<Simplex> simplex;
    float vorH = 0.0f, spxH = 0.0f, jtt = 0.0f, r = 0.0f, h = 0.0f;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

class PQSMod_VoronoiCraters2 : public PQSMod {
public:
    double deformation = 1000.0;
    int voronoiSeed = 0;
    double voronoiFrequency = 1.0;
    AnimationCurve craterCurve;
    int jitterSeed = 123123;
    double jitterOctaves = 2.0, jitterPersistence = 0.5, jitterFrequency = 20.0, jitter = 0.5;
    int deformationSeed = 123123;
    double deformationOctaves = 2.0, deformationPersistence = 0.5, deformationFrequency = 20.0;
    bool DebugColorMapping = false;
    std::shared_ptr<LibNoise::Voronoi> voronoi;
    std::shared_ptr<Simplex> jitterSimplex, deformationSimplex;
    double h = 0.0, s = 0.0, r = 0.0, d = 0.0, xd = 0.0, yd = 0.0, zd = 0.0;
    Vector3d nearest;
    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

// ---- PQSMod_VertexPlanet.cs ------------------------------------------------------------------------------------------
class PQSMod_VertexPlanet : public PQSMod {
public:
    struct SimplexWrapper {  // :8
        double deformity = 0.0, octaves = 0.0, persistance = 0.0, frequency = 0.0;
        std::shared_ptr<Simplex> simplex;
        void Setup(int seed);
    };
    struct NoiseModWrapper {  // :38
        double deformity = 0.0;
        int octaves = 0;
        double persistance = 0.0, frequency = 0.0;
        int seed = 0;
        std::shared_ptr<LibNoise::IModule> noise;
        void Setup(std::shared_ptr<LibNoise::IModule> mod) { noise = std::move(mod); }
    };
    struct LandClass {  // :68
        std::string name;
        double fractalStart = 0.0, fractalEnd = 0.0;
        Color baseColor, colorNoise;
        double colorNoiseAmount = 0.0;
        SimplexWrapper colorNoiseMap;
        bool lerpToNext = false;
        double startHeight = 0.0, endHeight = 0.0, fractalDelta = 0.0;
    };
    int seed = 0;
    double deformity = 0.0, colorDeformity = 0.0, oceanLevel = 0.0, oceanStep = 0.0, oceanDepth = 0.0;
    bool oceanSnap = false;
    double terrainSmoothing = 0.25, terrainShapeStart = 2.0, terrainShapeEnd = -2.0;
    double terrainRidgesMin = 0.4, terrainRidgesMax = 1.0;
    SimplexWrapper continental, continentalRuggedness, continentalSmoothing, continentalSharpnessMap;
    NoiseModWrapper continentalSharpness;
    SimplexWrapper terrainType;
    bool buildHeightColors = false;
    std::vector<LandClass> landClasses;
    double terrainRidgeBalance = 0.1;
    // private state
    double continentialHeight = 0.0, continentialSharpnessValue = 0.0, continentialSharpnessMapValue = 0.0;
    double continentalRHeight = 0.0, continental2Height = 0.0, continentalDelta = 0.0, vHeight = 0.0;
    double continentalDeformity = 0.0, continentialHeightPreSmooth = 0.0, ct2 = 0.0, ct3 = 0.0;
    int lcCount = 0;

    void OnSetup() override;
    void OnVertexBuildHeight(PQSVertexBuildData& data) override;
    double GetVertexMaxHeight() override { return deformity * 1.2; }
    bool AffectsHeight() const override { return true; }
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
    static double Clamp(double v, double low, double high);  // :240
    static double Lerp(double v2, double v1, double dt);       // :350
    double CubicHermite(double start, double end, double startTangent, double endTangent, double t);  // :355
};

}  // namespace ksp
