// WP-12a: the PQSMods that act on quads rather than on heights, as far as the engine-agnostic quad build needs them:
//   PQSMod_QuadEnhanceCoast.cs        coastal quads subdivide earlier (subdivideThresholdFactor *= coast / ocean factor)
//   PQSMod_CelestialBodyTransform.cs  sphere activation by visible altitude / main body, fade values, body transform
//   PQSMod_UVPlanetRelativePosition.cs  mesh uv = planet-relative (x, y), uv2 = (z, 1 - dot(dir, normal))
//   PQSMod_AltitudeUV.cs              uv3.x from the vertex altitude
//   PQSMod_AltitudeAlpha.cs           colour alpha from the vertex altitude
//   PQSMod_QuadMeshColliders.cs       which quads get a MeshCollider (the WP-12b seam: PQ::meshColliderEnabled and the
//                                     PQS::onQuadColliderEnabled / onQuadColliderDisabled hooks)
// The stock stacks instantiate them from the prefab data (tools/wp11/gen_wp11_terrain.py -> pqs_prefab_data.cpp).
// Materials / renderers (AltitudeFade's material SetFloat, secondaryRenderers) and the PhysicMaterial are engine side.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ksp/terrain/pqs.hpp"

namespace ksp {

// PQSMod_QuadEnhanceCoast.cs
class PQSMod_QuadEnhanceCoast : public PQSMod {
public:
    double coastLessThan = 1.0;  // Reset() defaults (:13)
    double oceanFactor = 0.5;
    double coastFactor = 1.5;
    void OnSetup() override;          // :20 (no base call: requirements stay serialized)
    void OnQuadBuilt(PQ& quad) override;  // :25
    void VisitSerialized(const SerializedFieldVisitor& v) const override;

private:
    double coastTest_ = 0.0;
};

// PQSMod_CelestialBodyTransform.cs
class PQSMod_CelestialBodyTransform : public PQSMod {
public:
    // :9 AltitudeFade (the material float `fadeFloatName` it drives: `a`, the value the engine applies to the sphere's
    // surface material and the secondary renderers)
    struct AltitudeFade {
        std::string fadeFloatName;
        float fadeStart = 0.0f;
        float fadeEnd = 0.0f;
        float valueStart = 0.0f;
        float valueEnd = 0.0f;
        float highQualityShaderFadeStart = -1.0f;
        float highQualityShaderFadeEnd = -1.0f;
        std::vector<std::string> secondaryRendererNames;  // secondaryRenderers (GameObjects, e.g. "KerbinOcean")
        float a = 0.0f;  // private in C#: the faded value last applied
        std::function<void()> resetTerrainShaderOffset;  // FloatingOrigin.ResetTerrainShaderOffset (engine)
        void DoFade(double alt);  // :35
        void DoFade(bool fade);   // :76
    };
    // The game state the mod reads (FlightGlobals, HighLogic, MapView, the CelestialBody it belongs to). Unset hooks
    // mean "no FlightGlobals" (FlightGlobals.fetch == null), no body.
    struct Host {
        std::function<bool()> hasBody;                  // GetComponentUpwards<CelestialBody>() != null
        std::function<bool()> flightGlobalsExists;      // FlightGlobals.fetch != null
        std::function<const PQSTarget*()> activeVessel; // FlightGlobals.ActiveVessel.transform (nullptr: none)
        std::function<bool()> isCurrentMainBody;        // FlightGlobals.currentMainBody == body
        std::function<int()> loadedScene;               // HighLogic.LoadedScene (GameScenes)
        std::function<bool()> mapIsEnabled;             // MapView.MapIsEnabled
        std::function<void(Vector3d& position, QuaternionD& rotation)> bodyPose;  // body.position / body.rotation
    } host;

    double deactivateAltitude = 90500.0;  // Reset() (:157)
    double highQualityShaderDeactivateAltitude = -1.0;
    bool forceRebuildOnTargetChange = false;
    AltitudeFade planetFade;
    std::vector<AltitudeFade> secondaryFades;
    bool forceActivate = false;
    bool overrideFade = false;

    void OnSetup() override;                 // :170
    bool OnSphereStart() override;           // :204
    void OnPreUpdate() override;             // :234
    void OnSphereTransformUpdate() override; // :297
    void OnUpdateFinished() override;        // :311
    void OnSphereReset() override;           // :336
    void OnSphereInactive() override;        // :341
    void FadeOutSphere();                    // :346
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

// PQSMod_UVPlanetRelativePosition.cs
class PQSMod_UVPlanetRelativePosition : public PQSMod {
public:
    void OnQuadBuilt(PQ& quad) override;          // :12
    void OnQuadUpdateNormals(PQ& quad) override;  // :27
};

// PQSMod_AltitudeUV.cs
class PQSMod_AltitudeUV : public PQSMod {
public:
    double atmosphereHeight = 10000.0;  // Reset() (:15)
    double oceanDepth = 5000.0;
    bool invert = false;
    void OnSetup() override;                             // :21
    void OnVertexBuild(PQSVertexBuildData& vbData) override;  // :26
    void VisitSerialized(const SerializedFieldVisitor& v) const override;

private:
    double h_ = 0.0;
};

// PQSMod_AltitudeAlpha.cs
class PQSMod_AltitudeAlpha : public PQSMod {
public:
    double atmosphereDepth = 10000.0;  // Reset() (:13)
    bool invert = false;
    void OnSetup() override;                             // :18
    void OnVertexBuild(PQSVertexBuildData& vbData) override;  // :23
    void VisitSerialized(const SerializedFieldVisitor& v) const override;

private:
    double h_ = 0.0;
};

// PQSMod_QuadMeshColliders.cs
class PQSMod_QuadMeshColliders : public PQSMod {
public:
    int maxLevelOffset = 0;
    std::string physicsMaterialName;  // the PhysicMaterial asset ("Ground" on Kerbin; "" = none)
    void OnSetup() override;               // :17
    void OnQuadBuilt(PQ& quad) override;   // :22
    void OnQuadDestroy(PQ& quad) override; // :36
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
    int minLevel() const { return minLevel_; }

private:
    int minLevel_ = 0;
};

}  // namespace ksp
