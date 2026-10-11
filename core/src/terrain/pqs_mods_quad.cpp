// WP-12a: quad-side PQSMods (see include/ksp/terrain/pqs_mods_quad.hpp).
#include "ksp/terrain/pqs_mods_quad.hpp"

#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace {
using V = SerializedValue;
size_t Z(int i) { return static_cast<size_t>(i); }
}  // namespace

// ---- PQSMod_QuadEnhanceCoast.cs ----------------------------------------------------------------------------------------

// PQSMod_QuadEnhanceCoast.cs:20
void PQSMod_QuadEnhanceCoast::OnSetup() { coastTest_ = sphere->radius + coastLessThan; }

// PQSMod_QuadEnhanceCoast.cs:25. quad.meshVertMin / meshVertMax are heights above the radius (BuildQuad subtracts it)
// while coastTest = radius + coastLessThan, so every quad of a real planet takes the "ocean" branch: with the stock
// oceanFactor 1.0 (Kerbin, Eve, Laythe) the mod changes nothing. Kept as written.
void PQSMod_QuadEnhanceCoast::OnQuadBuilt(PQ& quad) {
    if (quad.meshVertMin < coastTest_) {
        if (quad.meshVertMax > coastTest_) {
            quad.subdivideThresholdFactor *= coastFactor;
        } else {
            quad.subdivideThresholdFactor *= oceanFactor;
        }
    }
}

void PQSMod_QuadEnhanceCoast::VisitSerialized(const SerializedFieldVisitor& v) const {
    PQSMod::VisitSerialized(v);
    v("coastLessThan", V::Of(coastLessThan));
    v("oceanFactor", V::Of(oceanFactor));
    v("coastFactor", V::Of(coastFactor));
}

// ---- PQSMod_CelestialBodyTransform.cs ----------------------------------------------------------------------------------

// PQSMod_CelestialBodyTransform.cs:35 DoFade(double)
void PQSMod_CelestialBodyTransform::AltitudeFade::DoFade(double alt) {
    float num = fadeStart;
    if (highQualityShaderFadeStart > 0.0f && GameSettings::TERRAIN_SHADER_QUALITY >= 2) num = highQualityShaderFadeStart;
    float num2 = fadeEnd;
    if (highQualityShaderFadeEnd > 0.0f && GameSettings::TERRAIN_SHADER_QUALITY >= 2) num2 = highQualityShaderFadeEnd;
    if (alt <= static_cast<double>(num)) {
        a = valueStart;
    } else if (alt >= static_cast<double>(num2)) {
        if (resetTerrainShaderOffset) resetTerrainShaderOffset();
        a = valueEnd;
    } else {
        // (double)(num2 - num): a conv.r8 of the float expression, no float rounding (Mono rule)
        a = static_cast<float>((alt - fx(num)) / (fx(num2) - fx(num)));
    }
    // renderers[i].sharedMaterial.SetFloat(fadeIntID, a) and every pqs surface / update material: engine side
}

// PQSMod_CelestialBodyTransform.cs:76 DoFade(bool)
void PQSMod_CelestialBodyTransform::AltitudeFade::DoFade(bool fade) { a = fade ? valueStart : valueEnd; }

// PQSMod_CelestialBodyTransform.cs:170 OnSetup (no base call). AltitudeFade.Setup (:102) only resolves shader ids and
// renderers (engine side).
void PQSMod_CelestialBodyTransform::OnSetup() {
    if (!(host.hasBody && host.hasBody())) forceActivate = true;  // Debug.LogWarning("CelestialBodyTransform: Cannot find CelestialBody.")
    if (forceActivate) {
        planetFade.DoFade(true);
        int num2 = static_cast<int>(secondaryFades.size());
        while (num2-- > 0) secondaryFades[Z(num2)].DoFade(true);
    } else {
        planetFade.DoFade(false);
        int num3 = static_cast<int>(secondaryFades.size());
        while (num3-- > 0) secondaryFades[Z(num3)].DoFade(false);
    }
}

// PQSMod_CelestialBodyTransform.cs:204 OnSphereStart
bool PQSMod_CelestialBodyTransform::OnSphereStart() {
    if (forceActivate) {
        if (sphere->target == nullptr) {
            if (sphere->secondaryTarget == nullptr) return true;  // Debug.LogError("[PQS Error]: Sphere target is null!")
            sphere->SetTarget(sphere->secondaryTarget);
        }
        sphere->isAlive = true;
        return true;
    }
    if (host.flightGlobalsExists && host.flightGlobalsExists()) {
        const PQSTarget* v = host.activeVessel ? host.activeVessel() : nullptr;
        if (v != nullptr) sphere->SetTarget(v);
        if (host.isCurrentMainBody && host.isCurrentMainBody()) sphere->isAlive = true;
    }
    return true;
}

// PQSMod_CelestialBodyTransform.cs:234 OnPreUpdate: activation / deactivation of the sphere by visible altitude
void PQSMod_CelestialBodyTransform::OnPreUpdate() {
    PQS& s = *sphere;
    const int quality = GameSettings::TERRAIN_SHADER_QUALITY;
    if (forceActivate) {
        if (!s.isActive) {
            s.ActivateSphere();
            s.isAlive = true;
        }
    } else if (!(host.flightGlobalsExists && host.flightGlobalsExists())) {
        if (!s.isActive) s.ActivateSphere();
    } else {
        const int scene = host.loadedScene ? host.loadedScene() : -1;
        if (scene == static_cast<int>(GameScenes::PSYSTEM) || scene == static_cast<int>(GameScenes::MAINMENU)) return;
        if (scene == static_cast<int>(GameScenes::MISSIONBUILDER)) {
            if (!s.isActive) s.ActivateSphere();
            return;
        }
        const PQSTarget* activeVessel = host.activeVessel ? host.activeVessel() : nullptr;
        if (activeVessel != s.target) {
            if (activeVessel == nullptr) {
                s.SetTarget(nullptr);
                return;
            }
            s.SetTarget(activeVessel);
        }
        const bool mapIsEnabled = host.mapIsEnabled && host.mapIsEnabled();
        if (!(host.isCurrentMainBody && host.isCurrentMainBody())) {
            if (s.isActive) {
                s.DeactivateSphere();  // Debug.Log(name + ": Resetting")
                s.ResetAndWait();
            }
        } else if (s.isActive) {
            if (mapIsEnabled ||
                (s.visibleAltitude > deactivateAltitude && (quality < 2 || highQualityShaderDeactivateAltitude < 0.0)) ||
                (highQualityShaderDeactivateAltitude > 0.0 && s.visibleAltitude > highQualityShaderDeactivateAltitude && quality >= 2)) {
                s.DeactivateSphere();
            }
        } else if (!mapIsEnabled &&
                   ((s.visibleAltitude < deactivateAltitude && (quality < 2 || highQualityShaderDeactivateAltitude < 0.0)) ||
                    (highQualityShaderDeactivateAltitude > 0.0 && s.visibleAltitude < highQualityShaderDeactivateAltitude && quality >= 2))) {
            s.ActivateSphere();
        }
    }
}

// PQSMod_CelestialBodyTransform.cs:297 (without a body: sphere.transform.position / rotation, the identity here)
void PQSMod_CelestialBodyTransform::OnSphereTransformUpdate() {
    if (host.hasBody && host.hasBody() && host.bodyPose) {
        host.bodyPose(sphere->transformPosition, sphere->transformRotation);
    } else {
        sphere->transformPosition = Vector3d::zero();
        sphere->transformRotation = QuaternionD::identity();
    }
}

// PQSMod_CelestialBodyTransform.cs:311
void PQSMod_CelestialBodyTransform::OnUpdateFinished() {
    if (!overrideFade) {
        if (sphere->isActive) {
            planetFade.DoFade(sphere->visibleAltitude);
            int num = static_cast<int>(secondaryFades.size());
            while (num-- > 0) secondaryFades[Z(num)].DoFade(sphere->visibleAltitude);
        }
    } else {
        planetFade.DoFade(0.0);
        int num2 = static_cast<int>(secondaryFades.size());
        while (num2-- > 0) secondaryFades[Z(num2)].DoFade(0.0);
    }
}

// PQSMod_CelestialBodyTransform.cs:336 / :341
void PQSMod_CelestialBodyTransform::OnSphereReset() { FadeOutSphere(); }
void PQSMod_CelestialBodyTransform::OnSphereInactive() { FadeOutSphere(); }

// PQSMod_CelestialBodyTransform.cs:346
void PQSMod_CelestialBodyTransform::FadeOutSphere() {
    planetFade.DoFade(false);
    int num = static_cast<int>(secondaryFades.size());
    while (num-- > 0) secondaryFades[Z(num)].DoFade(false);
}

void PQSMod_CelestialBodyTransform::VisitSerialized(const SerializedFieldVisitor& v) const {
    PQSMod::VisitSerialized(v);
    v("deactivateAltitude", V::Of(deactivateAltitude));
    v("highQualityShaderDeactivateAltitude", V::Of(highQualityShaderDeactivateAltitude));
    v("forceRebuildOnTargetChange", V::Of(forceRebuildOnTargetChange));
    v("forceActivate", V::Of(forceActivate));
    v("overrideFade", V::Of(overrideFade));
    auto fade = [&](const std::string& p, const AltitudeFade& f) {
        v(p + ".fadeFloatName", V::Of(f.fadeFloatName));
        v(p + ".fadeStart", V::Of(f.fadeStart));
        v(p + ".fadeEnd", V::Of(f.fadeEnd));
        v(p + ".valueStart", V::Of(f.valueStart));
        v(p + ".valueEnd", V::Of(f.valueEnd));
        v(p + ".highQualityShaderFadeStart", V::Of(f.highQualityShaderFadeStart));
        v(p + ".highQualityShaderFadeEnd", V::Of(f.highQualityShaderFadeEnd));
    };
    fade("planetFade", planetFade);
    for (size_t i = 0; i < secondaryFades.size(); i++) fade("secondaryFades[" + std::to_string(i) + "]", secondaryFades[i]);
}

// ---- PQSMod_UVPlanetRelativePosition.cs --------------------------------------------------------------------------------

// PQSMod_UVPlanetRelativePosition.cs:12: rewrites the STATIC PQS.cacheUVs / cacheUV2s from the double scratch vertices
void PQSMod_UVPlanetRelativePosition::OnQuadBuilt(PQ& quad) {
    PQSMeshCache& c = sphere->meshCache();
    for (int i = 0; i < c.cacheVertCount; i++) {
        const Vector3d v = c.verts[Z(i)];
        c.cacheUVs[Z(i)].x = static_cast<float>(v.x);
        c.cacheUVs[Z(i)].y = static_cast<float>(v.y);
        c.cacheUV2s[Z(i)].x = static_cast<float>(v.z);
        c.cacheUV2s[Z(i)].y = static_cast<float>(1.0 - Vector3d::Dot(v.normalized(), Vector3d(quad.vertNormals[Z(i)])));
    }
    quad.mesh.uv = c.cacheUVs;
    quad.mesh.uv2 = c.cacheUV2s;
}

// PQSMod_UVPlanetRelativePosition.cs:27 (quad.verts[i] + quad.positionPlanet: the float mesh vertex plus the double
// quad position; with surfaceRelativeQuads that is the planet-relative vertex up to the float origin rounding)
void PQSMod_UVPlanetRelativePosition::OnQuadUpdateNormals(PQ& quad) {
    PQSMeshCache& c = sphere->meshCache();
    for (int i = 0; i < c.cacheVertCount; i++) {
        const Vector3d v = quad.verts[Z(i)] + quad.positionPlanet;
        c.cacheUV2s[Z(i)].x = static_cast<float>(v.z);
        c.cacheUV2s[Z(i)].y = static_cast<float>(1.0 - Vector3d::Dot(v.normalized(), Vector3d(quad.vertNormals[Z(i)])));
    }
    quad.mesh.uv2 = c.cacheUV2s;
}

// ---- PQSMod_AltitudeUV.cs ----------------------------------------------------------------------------------------------

// PQSMod_AltitudeUV.cs:21
void PQSMod_AltitudeUV::OnSetup() { requirements = PQS::MeshUV3; }

// PQSMod_AltitudeUV.cs:26 (vertHeight is still radius + terrain height here)
void PQSMod_AltitudeUV::OnVertexBuild(PQSVertexBuildData& vbData) {
    h_ = vbData.vertHeight - sphere->radius;
    if (h_ >= 0.0) {
        h_ /= atmosphereHeight;
    } else {
        h_ /= oceanDepth;
    }
    h_ = UtilMath::Clamp(h_, -1.0, 1.0);
    vbData.u3 = invert ? 1.0 - h_ : h_;
}

void PQSMod_AltitudeUV::VisitSerialized(const SerializedFieldVisitor& v) const {
    PQSMod::VisitSerialized(v);
    v("atmosphereHeight", V::Of(atmosphereHeight));
    v("oceanDepth", V::Of(oceanDepth));
    v("invert", V::Of(invert));
}

// ---- PQSMod_AltitudeAlpha.cs -------------------------------------------------------------------------------------------

// PQSMod_AltitudeAlpha.cs:18
void PQSMod_AltitudeAlpha::OnSetup() { requirements = PQS::MeshColorChannel; }

// PQSMod_AltitudeAlpha.cs:23
void PQSMod_AltitudeAlpha::OnVertexBuild(PQSVertexBuildData& vbData) {
    h_ = (vbData.vertHeight - sphere->radius) / atmosphereDepth;
    vbData.vertColor.a = invert ? static_cast<float>(1.0 - h_) : static_cast<float>(h_);
}

void PQSMod_AltitudeAlpha::VisitSerialized(const SerializedFieldVisitor& v) const {
    PQSMod::VisitSerialized(v);
    v("atmosphereDepth", V::Of(atmosphereDepth));
    v("invert", V::Of(invert));
}

// ---- PQSMod_QuadMeshColliders.cs ---------------------------------------------------------------------------------------

// PQSMod_QuadMeshColliders.cs:17 (no base call)
void PQSMod_QuadMeshColliders::OnSetup() { minLevel_ = sphere->maxLevel - Mathf::Abs(maxLevelOffset); }

// PQSMod_QuadMeshColliders.cs:22: AddComponent<MeshCollider> once, enable, sharedMesh = quad.mesh (Unity bakes the
// collider from the mesh as it is now: triangles = cacheIndices[0], the vertices just built)
void PQSMod_QuadMeshColliders::OnQuadBuilt(PQ& quad) {
    if (quad.subdivision >= minLevel_) {
        quad.meshColliderCreated = true;
        quad.meshColliderEnabled = true;
        if (sphere->onQuadColliderEnabled) sphere->onQuadColliderEnabled(quad);
    }
}

// PQSMod_QuadMeshColliders.cs:36
void PQSMod_QuadMeshColliders::OnQuadDestroy(PQ& quad) {
    if (quad.meshColliderCreated) {
        quad.meshColliderEnabled = false;
        if (sphere->onQuadColliderDisabled) sphere->onQuadColliderDisabled(quad);
    }
}

void PQSMod_QuadMeshColliders::VisitSerialized(const SerializedFieldVisitor& v) const {
    PQSMod::VisitSerialized(v);
    v("maxLevelOffset", V::Of(maxLevelOffset));
}

}  // namespace ksp
