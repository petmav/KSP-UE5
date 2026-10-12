// WP-11: height mods (Assembly-CSharp/PQSMod_*.cs). Each function cites its C# source line. Managed float code follows
// the Mono rule (ksp/dotnet/mono_float.hpp); System.Math calls go through ksp::dotnet::Math.
#include "ksp/terrain/pqs_mods.hpp"

#include <cmath>
#include <stdexcept>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp {

namespace {

constexpr double kPI = 3.141592653589793;  // System.Math.PI

using V = SerializedValue;

// Vector3d -> Vector3 (Vector3d.cs:97 implicit operator) for Unity float APIs
inline Vector3f ToF(const Vector3d& v) { return v.ToVector3f(); }

void VisitCurveless(const PQSMod& m, const SerializedFieldVisitor& v) { m.PQSMod::VisitSerialized(v); }

std::shared_ptr<LibNoise::IModule> MakeNoise(PQSNoiseType type, float frequency, float lacunarity, float persistance,
                                             int octaves, int seed, LibNoise::NoiseQuality mode) {
    switch (type) {
    default:
        return std::make_shared<LibNoise::Perlin>(frequency, lacunarity, persistance, octaves, seed, mode);
    case PQSNoiseType::RidgedMultifractal:
        return std::make_shared<LibNoise::RidgedMultifractal>(frequency, lacunarity, octaves, seed, mode);
    case PQSNoiseType::Billow:
        return std::make_shared<LibNoise::Billow>(frequency, lacunarity, persistance, octaves, seed, mode);
    }
}

}  // namespace

// ---- PQSModPassive -------------------------------------------------------------------------------------------------
void PQSModPassive::OnSetup() {
    // the class's own OnSetup only assigns requirements (generated); classes without an override run PQSMod.OnSetup
    if (setupRequirements >= 0) requirements = setupRequirements;
}

// ---- PQSMod_FlattenArea.cs -----------------------------------------------------------------------------------------
// PQSMod_FlattenArea.cs:64
void PQSMod_FlattenArea::OnSetup() {
    requirements = PQS::MeshColorChannel | PQS::MeshCustomNormals;
    if (useLatLon) {
        // LatLon.GetSurfaceNVector(cf, lat, lon) with cf = SetFrame(0, 0, 0) (LatLon.cs:13)
        CelestialFrame cf;
        CelestialFrame::SetFrame(0.0, 0.0, 0.0, cf);
        const double la = lat * (kPI / 180.0);
        const double lo = lon * (kPI / 180.0);
        const Vector3d n = cf.LocalToWorld(Planetarium::SphericalVector(la, lo)).xzy();
        const Vector3d vector3d = n * (sphere->radius + alt);
        position = ToF(vector3d);  // Vector3 field (float)
    }
    if (modEnabled) {
        posNorm = Vector3d(position.normalized());
        angleInner = sphere ? dotnet::Math::Atan(innerRadius / sphere->radius) : dotnet::Math::Atan(innerRadius);
        angleOuter = sphere ? dotnet::Math::Atan(outerRadius / sphere->radius) : dotnet::Math::Atan(outerRadius);
        angleQuadInclusion = angleOuter * 2.0;
        angleDelta = angleOuter - angleInner;
        flattenToRadius = sphere ? (sphere->radius + flattenTo) : flattenTo;
    }
}

// PQSMod_FlattenArea.cs:89
void PQSMod_FlattenArea::OnQuadPreBuild(PQ& quad) {
    testAngle = dotnet::Math::Acos(Vector3d::Dot(quad.positionPlanetRelative, posNorm));
    quadActive = testAngle < angleQuadInclusion;
}

// PQSMod_FlattenArea.cs:102
void PQSMod_FlattenArea::OnQuadBuilt(PQ&) { quadActive = true; }

// PQSMod_FlattenArea.cs:107
void PQSMod_FlattenArea::OnVertexBuildHeight(PQSVertexBuildData& vbData) {
    if (!overrideQuadBuildCheck && !quadActive) return;
    testAngle = dotnet::Math::Acos(Vector3d::Dot(vbData.directionFromCenter, posNorm));
    if (!(testAngle < angleQuadInclusion)) return;
    if (removeScatter) vbData.allowScatter = false;
    if (DEBUG_showColors) vbData.vertColor = Color(0.0f, 1.0f, 0.0f, 1.0f);  // Color.green
    if (!(testAngle < angleOuter)) return;
    if (testAngle < angleInner) {
        vbData.vertHeight = flattenToRadius;
        if (DEBUG_showColors) vbData.vertColor = Color(1.0f, 0.921568632f, 0.0156862754f, 1.0f);  // Color.yellow
        return;
    }
    aDelta = (testAngle - angleInner) / angleDelta;
    vbData.vertHeight = CubicHermite(flattenToRadius, vbData.vertHeight, smoothStart, smoothEnd, aDelta);
    // DEBUG_showColors: Color.Lerp(blue, yellow, (float)aDelta) -- colour output not ported
}

// PQSMod_FlattenArea.cs:152
double PQSMod_FlattenArea::CubicHermite(double start, double end, double startTangent, double endTangent, double t) {
    const double c2 = t * t;
    const double c3 = c2 * t;
    return start * (2.0 * c3 - 3.0 * c2 + 1.0) + startTangent * (c3 - 2.0 * c2 + t) + end * (-2.0 * c3 + 3.0 * c2) +
           endTangent * (c3 - c2);
}

void PQSMod_FlattenArea::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("outerRadius", V::Of(outerRadius));
    v("innerRadius", V::Of(innerRadius));
    v("position", V::Of(position));
    v("flattenTo", V::Of(flattenTo));
    v("removeScatter", V::Of(removeScatter));
    v("DEBUG_showColors", V::Of(DEBUG_showColors));
    v("smoothStart", V::Of(smoothStart));
    v("smoothEnd", V::Of(smoothEnd));
    v("useLatLon", V::Of(useLatLon));
    v("bodyName", V::Of(bodyName));
    v("lat", V::Of(lat));
    v("lon", V::Of(lon));
    v("alt", V::Of(alt));
}

// ---- PQSMod_FlattenAreaTangential.cs -------------------------------------------------------------------------------
// PQSMod_FlattenAreaTangential.cs:52
void PQSMod_FlattenAreaTangential::OnSetup() {
    requirements = PQS::MeshColorChannel | PQS::MeshCustomNormals;
    posNorm = Vector3d(position.normalized());
    angleInner = dotnet::Math::Atan(innerRadius / sphere->radius);
    angleOuter = dotnet::Math::Atan(outerRadius / sphere->radius);
    angleQuadInclusion = angleOuter * 2.0;
    angleDelta = angleOuter - angleInner;
    flattenToRadius = sphere->radius + flattenTo;
}

// PQSMod_FlattenAreaTangential.cs:63
void PQSMod_FlattenAreaTangential::OnQuadPreBuild(PQ& quad) {
    testAngle = dotnet::Math::Acos(Vector3d::Dot(quad.positionPlanetRelative, posNorm));
    quadActive = testAngle < angleQuadInclusion;
}

// PQSMod_FlattenAreaTangential.cs:76
void PQSMod_FlattenAreaTangential::OnQuadBuilt(PQ&) { quadActive = true; }

// PQSMod_FlattenAreaTangential.cs:81 (sic: no overrideQuadBuildCheck test; inactive until a quad was built)
void PQSMod_FlattenAreaTangential::OnVertexBuildHeight(PQSVertexBuildData& vbData) {
    if (!quadActive) return;
    if (DEBUG_showColors) vbData.vertColor = Color(0.0f, 1.0f, 0.0f, 1.0f);
    testAngle = dotnet::Math::Acos(Vector3d::Dot(vbData.directionFromCenter, posNorm));
    vHeight = flattenToRadius / dotnet::Math::Cos(testAngle);
    if (!(testAngle < angleOuter)) return;
    if (testAngle < angleInner) {
        vbData.vertHeight = vHeight;
        if (DEBUG_showColors) vbData.vertColor = Color(1.0f, 0.921568632f, 0.0156862754f, 1.0f);
        return;
    }
    aDelta = (testAngle - angleInner) / angleDelta;
    vbData.vertHeight = CubicHermite(vHeight, vbData.vertHeight, smoothStart, smoothEnd, aDelta);
}

// PQSMod_FlattenAreaTangential.cs:114
double PQSMod_FlattenAreaTangential::CubicHermite(double start, double end, double startTangent, double endTangent, double t) {
    const double c2 = t * t;
    const double c3 = c2 * t;
    return start * (2.0 * c3 - 3.0 * c2 + 1.0) + startTangent * (c3 - 2.0 * c2 + t) + end * (-2.0 * c3 + 3.0 * c2) +
           endTangent * (c3 - c2);
}

void PQSMod_FlattenAreaTangential::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("outerRadius", V::Of(outerRadius));
    v("innerRadius", V::Of(innerRadius));
    v("position", V::Of(position));
    v("flattenTo", V::Of(flattenTo));
    v("smoothStart", V::Of(smoothStart));
    v("smoothEnd", V::Of(smoothEnd));
    v("DEBUG_showColors", V::Of(DEBUG_showColors));
}

// ---- PQSMod_FlattenOcean.cs ----------------------------------------------------------------------------------------
// PQSMod_FlattenOcean.cs:15
void PQSMod_FlattenOcean::OnSetup() {
    requirements = PQS::MeshCustomNormals;
    oceanRad = static_cast<float>(sphere->radius + oceanRadius);
}

// PQSMod_FlattenOcean.cs:21
void PQSMod_FlattenOcean::OnVertexBuildHeight(PQSVertexBuildData& data) {
    if (data.vertHeight < oceanRad) data.vertHeight = oceanRad;
}

void PQSMod_FlattenOcean::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("oceanRadius", V::Of(oceanRadius));
}

// ---- PQSMod_MapDecal.cs / PQSMod_MapDecalTangent.cs ------------------------------------------------------------------
// PQSMod_MapDecal.cs:88 / PQSMod_MapDecalTangent.cs:92 (identical)
void PQSMod_MapDecalBase::SetupCommon() {
    requirements = PQS::MeshColorChannel | PQS::MeshCustomNormals;
    posNorm = Vector3d(position.normalized());
    // Quaternion.AngleAxis(angle, Vector3.up) * Quaternion.FromToRotation(posNorm, Vector3.up): native AngleAxis and
    // FromToRotation, managed operator*(Quaternion, Quaternion)
    rot = UnityNative::AngleAxis(angle, Vector3f(0.0f, 1.0f, 0.0f)) *
          UnityNative::FromToRotation(ToF(posNorm), Vector3f(0.0f, 1.0f, 0.0f));
    inclusionAngle = dotnet::Math::Atan(radius / sphere->radius) * 4.0;
    smoothC1M = f32store(1.0 - fx(smoothColor));
    smoothCR = f32store(1.0 / fx(smoothColor));
    smoothH1M = f32store(1.0 - fx(smoothHeight));
    smoothHR = f32store(1.0 / fx(smoothHeight));
}

// PQSMod_MapDecal.cs:185 / PQSMod_MapDecalTangent.cs:183
float PQSMod_MapDecalBase::GetHeightSmoothing(float u_, float v_) {
    if (u_ < smoothHeight) {
        smoothU = f32store(fx(u_) * fx(smoothHR));
    } else if (u_ > smoothH1M) {
        smoothU = f32store((1.0 - fx(u_)) * fx(smoothHR));
    } else {
        smoothU = 1.0f;
    }
    if (v_ < smoothHeight) {
        smoothV = f32store(fx(v_) * fx(smoothHR));
    } else if (v_ > smoothH1M) {
        smoothV = f32store((1.0 - fx(v_)) * fx(smoothHR));
    } else {
        smoothV = 1.0f;
    }
    return Mathf::Min(smoothU, smoothV);
}

// PQSMod_MapDecal.cs:273 / PQSMod_MapDecalTangent.cs:271
void PQSMod_MapDecalBase::OnQuadBuilt(PQ&) {
    quadActive = true;
    buildHeight = false;
}

// PQSMod_MapDecal.cs:279
double PQSMod_MapDecalBase::Lerp(double v2, double v1, double dt) { return v1 * dt + v2 * (1.0 - dt); }

void PQSMod_MapDecalBase::VisitCommon(const SerializedFieldVisitor& vis) const {
    VisitCurveless(*this, vis);
    vis("radius", V::Of(radius));
    vis("position", V::Of(position));
    vis("angle", V::Of(angle));
    vis("heightMap", V::Of(heightMap));
    vis("heightMapDeformity", V::Of(heightMapDeformity));
    vis("cullBlack", V::Of(cullBlack));
    vis("useAlphaHeightSmoothing", V::Of(useAlphaHeightSmoothing));
    vis("absolute", V::Of(absolute));
    V cm;
    cm.kind = V::Kind::Map;
    cm.text = colorMapKey;
    vis("colorMap", cm);
    vis("smoothHeight", V::Of(smoothHeight));
    vis("smoothColor", V::Of(smoothColor));
    vis("removeScatter", V::Of(removeScatter));
    vis("DEBUG_HighlightInclusion", V::Of(DEBUG_HighlightInclusion));
}

void PQSMod_MapDecal::OnSetup() { SetupCommon(); }

// PQSMod_MapDecal.cs:100
void PQSMod_MapDecal::OnQuadPreBuild(PQ& quad) {
    quadAngle = dotnet::Math::Acos(Vector3d::Dot(quad.positionPlanetRelative, posNorm));
    quadActive = quadAngle < inclusionAngle;
    buildHeight = true;
}

// PQSMod_MapDecal.cs:115
void PQSMod_MapDecal::OnVertexBuildHeight(PQSVertexBuildData& vbData) {
    if (!quadActive && buildHeight) return;
    if (sphere->isBuildingMaps) {
        quadAngle = dotnet::Math::Acos(Vector3d::Dot(vbData.directionFromCenter, posNorm));
        if (quadAngle > inclusionAngle) return;
    }
    // rot * vbData.directionFromCenter resolves to Quaternion * Vector3 (Vector3d -> Vector3 implicit conversion)
    vertRot = Vector3d(rot * ToF(vbData.directionFromCenter));
    u = static_cast<float>((vertRot.x * sphere->radius / radius + 1.0) * 0.5);
    v = static_cast<float>((vertRot.z * sphere->radius / radius + 1.0) * 0.5);
    if (!(u < 0.0f) && !(u > 1.0f) && !(v < 0.0f) && v <= 1.0f) {
        if (buildHeight || sphere->isBuildingMaps) vertActive = true;
        if (!heightMap) return;
        ha = heightMap->GetPixelHeightAlpha(u, v);
        smoothFactor = GetHeightSmoothing(u, v);
        if (useAlphaHeightSmoothing) smoothFactor = f32store(fx(smoothFactor) * fx(ha.alpha));
        if (!(smoothFactor > 0.0f)) return;
        if (removeScatter) vbData.allowScatter = false;
        if (cullBlack) {
            if (ha.height > 0.0f) {
                if (absolute) {
                    vbData.vertHeight = Lerp(vbData.vertHeight,
                                             sphere->radius + static_cast<double>(absoluteOffset) + heightMapDeformity * static_cast<double>(ha.height),
                                             smoothFactor);
                } else {
                    vbData.vertHeight = Lerp(vbData.vertHeight, vbData.vertHeight + heightMapDeformity * static_cast<double>(ha.height), smoothFactor);
                }
            }
        } else if (absolute) {
            vbData.vertHeight = Lerp(vbData.vertHeight,
                                     sphere->radius + static_cast<double>(absoluteOffset) + heightMapDeformity * static_cast<double>(ha.height),
                                     smoothFactor);
        } else {
            vbData.vertHeight = Lerp(vbData.vertHeight, vbData.vertHeight + heightMapDeformity * static_cast<double>(ha.height), smoothFactor);
        }
    } else {
        vertActive = false;
    }
}

void PQSMod_MapDecal::VisitSerialized(const SerializedFieldVisitor& vis) const {
    VisitCommon(vis);
    vis("absoluteOffset", V::Of(absoluteOffset));
}

void PQSMod_MapDecalTangent::OnSetup() { SetupCommon(); }

// PQSMod_MapDecalTangent.cs:104 (<= where MapDecal has <)
void PQSMod_MapDecalTangent::OnQuadPreBuild(PQ& quad) {
    quadAngle = dotnet::Math::Acos(Vector3d::Dot(quad.positionPlanetRelative, posNorm));
    quadActive = quadAngle <= inclusionAngle;
    buildHeight = true;
}

// PQSMod_MapDecalTangent.cs:119
void PQSMod_MapDecalTangent::OnVertexBuildHeight(PQSVertexBuildData& vbData) {
    if (!quadActive && buildHeight) return;
    if (sphere->isBuildingMaps) {
        quadAngle = dotnet::Math::Acos(Vector3d::Dot(vbData.directionFromCenter, posNorm));
        if (quadAngle > inclusionAngle) return;
    }
    vertRot = Vector3d(rot * ToF(vbData.directionFromCenter));
    u = static_cast<float>((vertRot.x * sphere->radius / radius + 1.0) * 0.5);
    v = static_cast<float>((vertRot.z * sphere->radius / radius + 1.0) * 0.5);
    if (!(u < 0.0f) && !(u > 1.0f) && !(v < 0.0f) && v <= 1.0f) {
        if (buildHeight || sphere->isBuildingMaps) vertActive = true;
        if (!heightMap) return;
        ha = heightMap->GetPixelHeightAlpha(u, v);
        smoothFactor = GetHeightSmoothing(u, v);
        if (useAlphaHeightSmoothing) smoothFactor = f32store(fx(smoothFactor) * fx(ha.alpha));
        double num = heightMapDeformity * static_cast<double>(ha.height) / Vector3d::Dot(vbData.directionFromCenter, posNorm);
        if (absolute) num += sphere->radius + absoluteOffset;
        if (!(smoothFactor > 0.0f)) return;
        if (removeScatter) vbData.allowScatter = false;
        if (cullBlack) {
            if (ha.height > 0.0f) vbData.vertHeight = Lerp(vbData.vertHeight, num, smoothFactor);
        } else {
            vbData.vertHeight = Lerp(vbData.vertHeight, num, smoothFactor);
        }
    } else {
        vertActive = false;
    }
}

void PQSMod_MapDecalTangent::VisitSerialized(const SerializedFieldVisitor& vis) const {
    VisitCommon(vis);
    vis("absoluteOffset", V::Of(absoluteOffset));
}

// ---- PQSMod_SmoothLatitudeRange.cs ---------------------------------------------------------------------------------
// PQSMod_SmoothLatitudeRange.cs:22
void PQSMod_SmoothLatitudeRange::OnSetup() {
    requirements = PQS::MeshCustomNormals;
    latitudeRange.Setup();
}

// PQSMod_SmoothLatitudeRange.cs:28
void PQSMod_SmoothLatitudeRange::OnVertexBuildHeight(PQSVertexBuildData& data) {
    const double smooth = latitudeRange.Lerp(sphere->sy);
    if (smooth != 0.0) {
        const double alt = data.vertHeight - sphere->radius;
        const double result = alt * (1.0 - smooth) + smoothToAltitude * smooth;
        data.vertHeight = sphere->radius + result;
    }
}

void PQSMod_SmoothLatitudeRange::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("latitudeRange.startStart", V::Of(latitudeRange.startStart));
    v("latitudeRange.startEnd", V::Of(latitudeRange.startEnd));
    v("latitudeRange.endStart", V::Of(latitudeRange.endStart));
    v("latitudeRange.endEnd", V::Of(latitudeRange.endEnd));
    v("latitudeRange.startDelta", V::Of(latitudeRange.startDelta));
    v("latitudeRange.endDelta", V::Of(latitudeRange.endDelta));
    v("smoothToAltitude", V::Of(smoothToAltitude));
}

// ---- PQSMod_VertexDefineCoastLine.cs -------------------------------------------------------------------------------
// PQSMod_VertexDefineCoastLine.cs:18
void PQSMod_VertexDefineCoastLine::OnSetup() {
    requirements = PQS::MeshCustomNormals;
    oceanRadius = static_cast<float>(sphere->radius + oceanRadiusOffset);
}

// PQSMod_VertexDefineCoastLine.cs:24
void PQSMod_VertexDefineCoastLine::OnVertexBuildHeight(PQSVertexBuildData& data) {
    if (data.vertHeight < oceanRadius) data.vertHeight += depthOffset;
}

void PQSMod_VertexDefineCoastLine::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("oceanRadiusOffset", V::Of(oceanRadiusOffset));
    v("depthOffset", V::Of(depthOffset));
}

// ---- PQSMod_VertexHeightOffset.cs / PQSMod_VertexHeightOblate.cs ---------------------------------------------------
// PQSMod_VertexHeightOffset.cs:13 (no OnSetup override: PQSMod.OnSetup)
void PQSMod_VertexHeightOffset::OnVertexBuildHeight(PQSVertexBuildData& data) { data.vertHeight += offset; }

void PQSMod_VertexHeightOffset::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("offset", V::Of(offset));
}

// PQSMod_VertexHeightOblate.cs:19
void PQSMod_VertexHeightOblate::OnSetup() { requirements = PQS::UVSphereCoords; }

// PQSMod_VertexHeightOblate.cs:24
void PQSMod_VertexHeightOblate::OnVertexBuildHeight(PQSVertexBuildData& data) {
    double a = dotnet::Math::Sin(kPI * data.v);
    a = dotnet::Math::Pow(a, pow);
    data.vertHeight += a * height;
}

void PQSMod_VertexHeightOblate::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("height", V::Of(height));
    v("pow", V::Of(pow));
}

// ---- PQSMod_VertexHeightMap.cs -------------------------------------------------------------------------------------
// PQSMod_VertexHeightMap.cs:22
void PQSMod_VertexHeightMap::OnSetup() {
    requirements = PQS::VertexMapCoords | PQS::MeshCustomNormals;
    heightDeformity = scaleDeformityByRadius ? static_cast<double>(static_cast<float>(sphere->radius * heightMapDeformity))
                                             : heightMapDeformity;
}

// PQSMod_VertexHeightMap.cs:35 (sic: heightMapDeformity, not the radius-scaled heightDeformity)
void PQSMod_VertexHeightMap::OnVertexBuildHeight(PQSVertexBuildData& data) {
    data.vertHeight += heightMapOffset + heightMapDeformity * static_cast<double>(heightMap->GetPixelFloat(data.u, data.v));
}

void PQSMod_VertexHeightMap::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("heightMap", V::Of(heightMap));
    v("heightMapDeformity", V::Of(heightMapDeformity));
    v("heightMapOffset", V::Of(heightMapOffset));
    v("scaleDeformityByRadius", V::Of(scaleDeformityByRadius));
}

// ---- PQSMod_VertexHeightMapStep.cs ---------------------------------------------------------------------------------
// PQSMod_VertexHeightMapStep.cs:27
void PQSMod_VertexHeightMapStep::OnSetup() {
    requirements = PQS::VertexMapCoords | PQS::MeshCustomNormals;
    heightDeformity = scaleDeformityByRadius ? static_cast<double>(static_cast<float>(sphere->radius * heightMapDeformity))
                                             : heightMapDeformity;
}

// PQSMod_VertexHeightMapStep.cs:40; PQS.BilinearInterpFloatMap (PQS.cs:2445) =
// texture.GetPixelBilinear((float)sx, (float)sy).grayscale
void PQSMod_VertexHeightMapStep::OnVertexBuildHeight(PQSVertexBuildData& data) {
    if (!heightMapSampler) throw std::logic_error("PQSMod_VertexHeightMapStep: Texture2D sampling is not ported (set heightMapSampler)");
    h = heightMapSampler(static_cast<float>(sphere->sx), static_cast<float>(sphere->sy));
    if (h >= coastHeight) {
        data.vertHeight += heightMapOffset + h * heightDeformity;
    } else {
        data.vertHeight += heightMapOffset;
    }
}

void PQSMod_VertexHeightMapStep::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("heightMapDeformity", V::Of(heightMapDeformity));
    v("heightMapOffset", V::Of(heightMapOffset));
    v("scaleDeformityByRadius", V::Of(scaleDeformityByRadius));
    v("coastHeight", V::Of(coastHeight));
}

// ---- PQSMod_VertexSimplexHeightMap.cs ------------------------------------------------------------------------------
// PQSMod_VertexSimplexHeightMap.cs:38 ((double)(heightEnd - heightStart): the float difference widened unrounded)
void PQSMod_VertexSimplexHeightMap::OnSetup() {
    requirements = PQS::MeshCustomNormals;
    simplex = std::make_shared<Simplex>(seed, octaves, persistence, frequency);
    hDeltaR = 1.0 / (fx(heightEnd) - fx(heightStart));
}

// PQSMod_VertexSimplexHeightMap.cs:45
void PQSMod_VertexSimplexHeightMap::OnVertexBuildHeight(PQSVertexBuildData& data) {
    h = heightMap->GetPixelFloat(data.u, data.v);
    if (h >= static_cast<double>(heightStart) && h <= static_cast<double>(heightEnd)) {
        h = (h - static_cast<double>(heightStart)) * hDeltaR;
        const Vector3d& d = data.directionFromCenter;
        data.vertHeight += (simplex->noise(d.x, d.y, d.z) + 1.0) * 0.5 * deformity * h;
    }
}

void PQSMod_VertexSimplexHeightMap::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("seed", V::Of(seed));
    v("deformity", V::Of(deformity));
    v("octaves", V::Of(octaves));
    v("persistence", V::Of(persistence));
    v("frequency", V::Of(frequency));
    v("heightMap", V::Of(heightMap));
    v("heightStart", V::Of(heightStart));
    v("heightEnd", V::Of(heightEnd));
}

// ---- PQSMod_VertexSimplexHeight.cs / PQSMod_VertexSimplexHeightAbsolute.cs -----------------------------------------
// PQSMod_VertexSimplexHeight.cs:28 / PQSMod_VertexSimplexHeightAbsolute.cs:28
void PQSMod_VertexSimplexHeight::OnSetup() {
    requirements = PQS::MeshCustomNormals;
    simplex = std::make_shared<Simplex>(seed, octaves, persistence, frequency);
}

// PQSMod_VertexSimplexHeight.cs:34
void PQSMod_VertexSimplexHeight::OnVertexBuildHeight(PQSVertexBuildData& data) {
    const Vector3d& d = data.directionFromCenter;
    data.vertHeight += simplex->noise(d.x, d.y, d.z) * deformity;
}

// PQSMod_VertexSimplexHeightAbsolute.cs:34
void PQSMod_VertexSimplexHeightAbsolute::OnVertexBuildHeight(PQSVertexBuildData& data) {
    const Vector3d& d = data.directionFromCenter;
    data.vertHeight += (simplex->noise(d.x, d.y, d.z) + 1.0) * 0.5 * deformity;
}

void PQSMod_VertexSimplexHeight::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("seed", V::Of(seed));
    v("deformity", V::Of(deformity));
    v("octaves", V::Of(octaves));
    v("persistence", V::Of(persistence));
    v("frequency", V::Of(frequency));
}

// ---- PQSMod_VertexSimplexHeightFlatten.cs --------------------------------------------------------------------------
// PQSMod_VertexSimplexHeightFlatten.cs:35
void PQSMod_VertexSimplexHeightFlatten::OnSetup() {
    requirements = PQS::MeshCustomNormals;
    simplex = std::make_shared<Simplex>(seed, octaves, persistence, frequency);
    valRatio = 1.0 / cutoff;
}

// PQSMod_VertexSimplexHeightFlatten.cs:42
void PQSMod_VertexSimplexHeightFlatten::OnVertexBuildHeight(PQSVertexBuildData& data) {
    const Vector3d& d = data.directionFromCenter;
    val = simplex->noiseNormalized(d.x, d.y, d.z);
    if (val > cutoff) data.vertHeight += deformity * ((val - cutoff) * valRatio);
}

void PQSMod_VertexSimplexHeightFlatten::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("seed", V::Of(seed));
    v("cutoff", V::Of(cutoff));
    v("deformity", V::Of(deformity));
    v("octaves", V::Of(octaves));
    v("persistence", V::Of(persistence));
    v("frequency", V::Of(frequency));
}

// ---- LibNoise height mods ------------------------------------------------------------------------------------------
// PQSMod_VertexHeightNoise.cs:44 switch (noiseType) (the float fields widen exactly to the double parameters)
void PQSMod_NoiseParams::CreateNoiseMap() {
    noiseMap = MakeNoise(noiseType, frequency, lacunarity, persistance, octaves, seed, mode);
}

void PQSMod_NoiseParams::VisitNoise(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("noiseType", V::Of(static_cast<int>(noiseType)));
    v("deformity", V::Of(deformity));
    v("seed", V::Of(seed));
    v("frequency", V::Of(frequency));
    v("lacunarity", V::Of(lacunarity));
    v("persistance", V::Of(persistance));
    v("octaves", V::Of(octaves));
    v("mode", V::Of(static_cast<int>(mode)));
}

// PQSMod_VertexHeightNoise.cs:44
void PQSMod_VertexHeightNoise::OnSetup() {
    requirements = PQS::MeshColorChannel;
    CreateNoiseMap();
}

// PQSMod_VertexHeightNoise.cs:61
void PQSMod_VertexHeightNoise::OnVertexBuildHeight(PQSVertexBuildData& data) {
    data.vertHeight += noiseMap->GetValue(data.directionFromCenter) * static_cast<double>(deformity);
}

void PQSMod_VertexHeightNoise::VisitSerialized(const SerializedFieldVisitor& v) const { VisitNoise(v); }

// PQSMod_VertexHeightNoiseVertHeight.cs:56
void PQSMod_VertexHeightNoiseVertHeight::OnSetup() {
    requirements = PQS::MeshColorChannel;
    CreateNoiseMap();
    hDeltaR = 1.0 / (fx(heightEnd) - fx(heightStart));
}

// PQSMod_VertexHeightNoiseVertHeight.cs:74
void PQSMod_VertexHeightNoiseVertHeight::OnVertexBuildHeight(PQSVertexBuildData& data) {
    h = (data.vertHeight - sphere->radiusMin) / sphere->radiusDelta;
    if (h >= static_cast<double>(heightStart) && h <= static_cast<double>(heightEnd)) {
        h = (h - static_cast<double>(heightStart)) * hDeltaR;
        n = noiseMap->GetValue(data.directionFromCenter);
        if (n < -1.0) n = -1.0;
        if (n > 1.0) n = 1.0;
        data.vertHeight += (n + 1.0) * 0.5 * static_cast<double>(deformity) * h;
    }
}

void PQSMod_VertexHeightNoiseVertHeight::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitNoise(v);
    v("heightStart", V::Of(heightStart));
    v("heightEnd", V::Of(heightEnd));
}

// PQSMod_VertexHeightNoiseHeightMap.cs:58
void PQSMod_VertexHeightNoiseHeightMap::OnSetup() {
    requirements = PQS::MeshColorChannel;
    CreateNoiseMap();
    hDeltaR = 1.0 / (fx(heightEnd) - fx(heightStart));
}

// PQSMod_VertexHeightNoiseHeightMap.cs:76 (PQS.BilinearInterpFloatMap: Texture2D sampling, see the header)
void PQSMod_VertexHeightNoiseHeightMap::OnVertexBuildHeight(PQSVertexBuildData& data) {
    if (!heightMapSampler) throw std::logic_error("PQSMod_VertexHeightNoiseHeightMap: Texture2D sampling is not ported (set heightMapSampler)");
    h = heightMapSampler(static_cast<float>(sphere->sx), static_cast<float>(sphere->sy));
    if (h >= static_cast<double>(heightStart) && h <= static_cast<double>(heightEnd)) {
        h = (h - static_cast<double>(heightStart)) * hDeltaR;
        n = noiseMap->GetValue(data.directionFromCenter);
        if (n < -1.0) n = -1.0;
        if (n > 1.0) n = 1.0;
        data.vertHeight += (n + 1.0) * 0.5 * static_cast<double>(deformity) * h;
    }
}

void PQSMod_VertexHeightNoiseHeightMap::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitNoise(v);
    v("heightStart", V::Of(heightStart));
    v("heightEnd", V::Of(heightEnd));
}

// PQSMod_VertexHeightNoiseVertHeightCurve.cs:59 (sic: does not assign requirements)
void PQSMod_VertexHeightNoiseVertHeightCurve::OnSetup() { CreateNoiseMap(); }

// PQSMod_VertexHeightNoiseVertHeightCurve.cs:75
void PQSMod_VertexHeightNoiseVertHeightCurve::OnVertexBuildHeight(PQSVertexBuildData& data) {
    h = data.vertHeight - sphere->radiusMin;
    if (h <= static_cast<double>(heightStart)) {
        t = 0.0f;
    } else if (h >= static_cast<double>(heightEnd)) {
        t = 1.0f;
    } else {
        t = static_cast<float>((h - static_cast<double>(heightStart)) / (fx(heightEnd) - fx(heightStart)));
    }
    n = noiseMap->GetValue(data.directionFromCenter);
    if (n < -1.0) n = -1.0;
    if (n > 1.0) n = 1.0;
    data.vertHeight += (n + 1.0) * 0.5 * static_cast<double>(deformity) * static_cast<double>(curve.Evaluate(t));
}

void PQSMod_VertexHeightNoiseVertHeightCurve::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitNoise(v);
    v("curve", V::Of(curve));
    v("heightStart", V::Of(heightStart));
    v("heightEnd", V::Of(heightEnd));
}

// ---- PQSMod_VertexHeightNoiseVertHeightCurve2.cs ------------------------------------------------------------------
// PQSMod_VertexHeightNoiseVertHeightCurve2.cs:78
void PQSMod_VertexHeightNoiseVertHeightCurve2::OnSetup() {
    requirements = PQS::MeshCustomNormals;
    ridgedAdd = std::make_shared<LibNoise::RidgedMultifractal>(ridgedAddFrequency, ridgedAddLacunarity, ridgedAddOctaves,
                                                                ridgedAddSeed, ridgedMode);
    ridgedSub = std::make_shared<LibNoise::RidgedMultifractal>(ridgedSubFrequency, ridgedSubLacunarity, ridgedSubOctaves,
                                                                ridgedSubSeed, ridgedMode);
    simplex = std::make_shared<Simplex>(simplexSeed, simplexOctaves, simplexPersistence, simplexFrequency);
    hDeltaR = 1.0 / (simplexHeightEnd - simplexHeightStart);
}

// PQSMod_VertexHeightNoiseVertHeightCurve2.cs:87
void PQSMod_VertexHeightNoiseVertHeightCurve2::OnVertexBuildHeight(PQSVertexBuildData& data) {
    h = data.vertHeight - sphere->radiusMin;
    if (h <= simplexHeightStart) {
        t = 0.0f;
    } else if (h >= simplexHeightEnd) {
        t = 1.0f;
    } else {
        t = static_cast<float>((h - simplexHeightStart) * hDeltaR);
    }
    s = simplex->noiseNormalized(data.directionFromCenter) * static_cast<double>(simplexCurve.Evaluate(t));
    if (s != 0.0) {
        // C# evaluates the left operand first
        const double add = ridgedAdd->GetValue(data.directionFromCenter);
        r = add - ridgedSub->GetValue(data.directionFromCenter);
        if (r < -1.0) r = -1.0;
        if (r > 1.0) r = 1.0;
        data.vertHeight += (r + 1.0) * 0.5 * static_cast<double>(deformity) * s;
    }
}

void PQSMod_VertexHeightNoiseVertHeightCurve2::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("deformity", V::Of(deformity));
    v("ridgedAddSeed", V::Of(ridgedAddSeed));
    v("ridgedAddFrequency", V::Of(ridgedAddFrequency));
    v("ridgedAddLacunarity", V::Of(ridgedAddLacunarity));
    v("ridgedAddOctaves", V::Of(ridgedAddOctaves));
    v("ridgedSubSeed", V::Of(ridgedSubSeed));
    v("ridgedSubFrequency", V::Of(ridgedSubFrequency));
    v("ridgedSubLacunarity", V::Of(ridgedSubLacunarity));
    v("ridgedSubOctaves", V::Of(ridgedSubOctaves));
    v("ridgedMode", V::Of(static_cast<int>(ridgedMode)));
    v("simplexCurve", V::Of(simplexCurve));
    v("simplexHeightStart", V::Of(simplexHeightStart));
    v("simplexHeightEnd", V::Of(simplexHeightEnd));
    v("simplexSeed", V::Of(simplexSeed));
    v("simplexOctaves", V::Of(simplexOctaves));
    v("simplexPersistence", V::Of(simplexPersistence));
    v("simplexFrequency", V::Of(simplexFrequency));
}

// ---- PQSMod_VertexHeightNoiseVertHeightCurve3.cs ------------------------------------------------------------------
// PQSMod_VertexHeightNoiseVertHeightCurve3.cs:26 (sic: always NoiseQuality.Low, `quality` is ignored)
void PQSMod_VertexHeightNoiseVertHeightCurve3::RidgedNoise::Setup() {
    fractal = std::make_shared<LibNoise::RidgedMultifractal>(frequency, lacunarity, octaves, seed, LibNoise::NoiseQuality::Low);
}
double PQSMod_VertexHeightNoiseVertHeightCurve3::RidgedNoise::Noise(const Vector3d& radial) const { return fractal->GetValue(radial); }
// PQSMod_VertexHeightNoiseVertHeightCurve3.cs:58
void PQSMod_VertexHeightNoiseVertHeightCurve3::SimplexNoise::Setup() {
    fractal = std::make_shared<Simplex>(seed, octaves, persistence, frequency);
}
double PQSMod_VertexHeightNoiseVertHeightCurve3::SimplexNoise::Noise(const Vector3d& radial) const { return fractal->noise(radial); }
double PQSMod_VertexHeightNoiseVertHeightCurve3::SimplexNoise::NoiseNormalized(const Vector3d& radial) const {
    return fractal->noiseNormalized(radial);
}

// PQSMod_VertexHeightNoiseVertHeightCurve3.cs:123
void PQSMod_VertexHeightNoiseVertHeightCurve3::OnSetup() {
    requirements = PQS::MeshCustomNormals;
    ridgedAdd.Setup();
    ridgedSub.Setup();
    curveMultiplier.Setup();
    deformity.Setup();
    hDeltaR = 1.0 / (inputHeightEnd - inputHeightStart);
}

// PQSMod_VertexHeightNoiseVertHeightCurve3.cs:133
void PQSMod_VertexHeightNoiseVertHeightCurve3::OnVertexBuildHeight(PQSVertexBuildData& data) {
    h = data.vertHeight - sphere->radiusMin;
    if (h <= inputHeightStart) {
        t = 0.0f;
    } else if (h >= inputHeightEnd) {
        t = 1.0f;
    } else {
        t = static_cast<float>((h - inputHeightStart) * hDeltaR);
    }
    s = curveMultiplier.NoiseNormalized(data.directionFromCenter) * static_cast<double>(inputHeightCurve.Evaluate(t));
    if (s != 0.0) {
        const double add = ridgedAdd.Noise(data.directionFromCenter);
        r = add - ridgedSub.Noise(data.directionFromCenter);
        d = Lerp(deformityMin, deformityMax, deformity.NoiseNormalized(data.directionFromCenter));
        if (r < -1.0) r = -1.0;
        if (r > 1.0) r = 1.0;
        r = (r + 1.0) * 0.5;
        data.vertHeight += r * d * s;
    }
}

// PQSMod_VertexHeightNoiseVertHeightCurve3.cs:166
double PQSMod_VertexHeightNoiseVertHeightCurve3::Lerp(double a, double b, double t) { return a * t + b * (1.0 - t); }

void PQSMod_VertexHeightNoiseVertHeightCurve3::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("inputHeightStart", V::Of(inputHeightStart));
    v("inputHeightEnd", V::Of(inputHeightEnd));
    v("inputHeightCurve", V::Of(inputHeightCurve));
    auto simplexNoise = [&](const std::string& p, const SimplexNoise& s_) {
        v(p + ".seed", V::Of(s_.seed));
        v(p + ".frequency", V::Of(s_.frequency));
        v(p + ".persistence", V::Of(s_.persistence));
        v(p + ".octaves", V::Of(s_.octaves));
    };
    auto ridged = [&](const std::string& p, const RidgedNoise& r_) {
        v(p + ".seed", V::Of(r_.seed));
        v(p + ".frequency", V::Of(r_.frequency));
        v(p + ".lacunarity", V::Of(r_.lacunarity));
        v(p + ".octaves", V::Of(r_.octaves));
        v(p + ".quality", V::Of(static_cast<int>(r_.quality)));
    };
    simplexNoise("curveMultiplier", curveMultiplier);
    v("deformityMin", V::Of(deformityMin));
    v("deformityMax", V::Of(deformityMax));
    simplexNoise("deformity", deformity);
    ridged("ridgedAdd", ridgedAdd);
    ridged("ridgedSub", ridgedSub);
}

// ---- PQSMod_VertexRidgedAltitudeCurve.cs ---------------------------------------------------------------------------
// PQSMod_VertexRidgedAltitudeCurve.cs:67
void PQSMod_VertexRidgedAltitudeCurve::OnSetup() {
    requirements = PQS::MeshCustomNormals;
    ridgedAdd = std::make_shared<LibNoise::RidgedMultifractal>(ridgedAddFrequency, ridgedAddLacunarity, ridgedAddOctaves,
                                                                ridgedAddSeed, ridgedMode);
    simplex = std::make_shared<Simplex>(simplexSeed, simplexOctaves, simplexPersistence, simplexFrequency);
    hDeltaR = 1.0 / (simplexHeightEnd - simplexHeightStart);
}

// PQSMod_VertexRidgedAltitudeCurve.cs:75
void PQSMod_VertexRidgedAltitudeCurve::OnVertexBuildHeight(PQSVertexBuildData& data) {
    h = data.vertHeight - (sphere ? sphere->radiusMin : static_cast<double>(ridgedMinimum));
    if (h <= simplexHeightStart) {
        t = 0.0f;
    } else if (h >= simplexHeightEnd) {
        t = 1.0f;
    } else {
        t = static_cast<float>((h - simplexHeightStart) * hDeltaR);
    }
    s = simplex->noiseNormalized(data.directionFromCenter);
    if (s != 0.0) {
        r = dotnet::Math::Max(static_cast<double>(ridgedMinimum), ridgedAdd->GetValue(data.directionFromCenter)) *
            dotnet::Math::Max(s, 0.0);
        if (r < -1.0) r = -1.0;
        if (r > 1.0) r = 1.0;
        data.vertHeight += r * static_cast<double>(deformity) * static_cast<double>(simplexCurve.Evaluate(t));
    }
}

void PQSMod_VertexRidgedAltitudeCurve::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("deformity", V::Of(deformity));
    v("ridgedAddSeed", V::Of(ridgedAddSeed));
    v("ridgedAddFrequency", V::Of(ridgedAddFrequency));
    v("ridgedAddLacunarity", V::Of(ridgedAddLacunarity));
    v("ridgedAddOctaves", V::Of(ridgedAddOctaves));
    v("ridgedMinimum", V::Of(ridgedMinimum));
    v("ridgedMode", V::Of(static_cast<int>(ridgedMode)));
    v("simplexCurve", V::Of(simplexCurve));
    v("simplexHeightStart", V::Of(simplexHeightStart));
    v("simplexHeightEnd", V::Of(simplexHeightEnd));
    v("simplexSeed", V::Of(simplexSeed));
    v("simplexOctaves", V::Of(simplexOctaves));
    v("simplexPersistence", V::Of(simplexPersistence));
    v("simplexFrequency", V::Of(simplexFrequency));
}

// ---- PQSMod_VertexNoise.cs -----------------------------------------------------------------------------------------
// PQSMod_VertexNoise.cs:41. `1f / x` passed to a double parameter is evaluated in double and not rounded to float
// (Mono rule: no store), so it is 1.0 / (double)x.
void PQSMod_VertexNoise::OnSetup() {
    requirements = PQS::MeshCustomNormals;
    auto billow = std::make_shared<LibNoise::Billow>();
    billow->Seed = seed;
    billow->Frequency = 2.0 * (1.0 / fx(smoothness));
    auto inputmodule = std::make_shared<LibNoise::Modifiers::ScaleBiasOutput>(1.0 / fx(plainSmoothness), fx(mesaVsPlainsBias), billow);
    auto ridgedMultifractal = std::make_shared<LibNoise::RidgedMultifractal>();
    ridgedMultifractal->Seed = seed;
    ridgedMultifractal->OctaveCount(noisePasses);
    ridgedMultifractal->Frequency = 1.0 / static_cast<double>(smoothness);
    auto perlin = std::make_shared<LibNoise::Perlin>();
    perlin->Seed = seed;
    perlin->OctaveCount(noisePasses);
    perlin->Frequency = 1.0 / static_cast<double>(plainsVsMountainSmoothness);
    perlin->Persistence = 1.0 / static_cast<double>(falloff);
    terrainHeightMap = std::make_shared<LibNoise::Modifiers::Select>(0.0, 1.0, fx(plainsVsMountainThreshold), inputmodule,
                                                                     ridgedMultifractal);
    terrainHeightMap->ControlModule = perlin;
}

// PQSMod_VertexNoise.cs:61
void PQSMod_VertexNoise::OnVertexBuildHeight(PQSVertexBuildData& data) {
    data.vertHeight += terrainHeightMap->GetValue(data.directionFromCenter * sphere->radius) * static_cast<double>(noiseDeformity);
}

void PQSMod_VertexNoise::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("seed", V::Of(seed));
    v("noiseDeformity", V::Of(noiseDeformity));
    v("noisePasses", V::Of(noisePasses));
    v("smoothness", V::Of(smoothness));
    v("falloff", V::Of(falloff));
    v("mesaVsPlainsBias", V::Of(mesaVsPlainsBias));
    v("plainsVsMountainSmoothness", V::Of(plainsVsMountainSmoothness));
    v("plainsVsMountainThreshold", V::Of(plainsVsMountainThreshold));
    v("plainSmoothness", V::Of(plainSmoothness));
}

// ---- PQSMod_VertexVoronoi.cs ---------------------------------------------------------------------------------------
// PQSMod_VertexVoronoi.cs:125 (LibNoise.Voronoi; sic: does not assign requirements)
void PQSMod_VertexVoronoi::OnSetup() {
    voronoi = std::make_shared<LibNoise::Voronoi>(voronoiFrequency, voronoiDisplacement, voronoiSeed, voronoiEnableDistance);
}

// PQSMod_VertexVoronoi.cs:130
void PQSMod_VertexVoronoi::OnVertexBuildHeight(PQSVertexBuildData& data) {
    const Vector3d& d = data.directionFromCenter;
    data.vertHeight += voronoi->GetValue(d.x, d.y, d.z) * deformation;
}

void PQSMod_VertexVoronoi::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("deformation", V::Of(deformation));
    v("voronoiSeed", V::Of(voronoiSeed));
    v("voronoiDisplacement", V::Of(voronoiDisplacement));
    v("voronoiFrequency", V::Of(voronoiFrequency));
    v("voronoiEnableDistance", V::Of(voronoiEnableDistance));
}

// ---- PQSMod_VoronoiCraters.cs --------------------------------------------------------------------------------------
// PQSMod_VoronoiCraters.cs:55 (sic: does not assign requirements)
void PQSMod_VoronoiCraters::OnSetup() {
    voronoi = std::make_shared<LibNoise::Voronoi>(voronoiFrequency, voronoiDisplacement, voronoiSeed, true);
    simplex = std::make_shared<Simplex>(simplexSeed, simplexOctaves, simplexPersistence, simplexFrequency);
}

// PQSMod_VoronoiCraters.cs:61 (managed float code: every float field store rounds)
void PQSMod_VoronoiCraters::OnVertexBuildHeight(PQSVertexBuildData& data) {
    const Vector3d& d = data.directionFromCenter;
    vorH = static_cast<float>(voronoi->GetValue(d.x, d.y, d.z));
    spxH = static_cast<float>(simplex->noise(d.x, d.y, d.z));
    jtt = f32store(fx(spxH) * fx(jitter) * fx(jitterCurve.Evaluate(vorH)));
    r = f32store(fx(vorH) + fx(jtt));
    h = craterCurve.Evaluate(r);
    data.vertHeight += (fx(h) + fx(jitterHeight) * fx(jtt) * fx(h)) * deformation;
}

void PQSMod_VoronoiCraters::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("deformation", V::Of(deformation));
    v("voronoiSeed", V::Of(voronoiSeed));
    v("voronoiDisplacement", V::Of(voronoiDisplacement));
    v("voronoiFrequency", V::Of(voronoiFrequency));
    v("craterCurve", V::Of(craterCurve));
    v("jitterCurve", V::Of(jitterCurve));
    v("simplexSeed", V::Of(simplexSeed));
    v("simplexOctaves", V::Of(simplexOctaves));
    v("simplexPersistence", V::Of(simplexPersistence));
    v("simplexFrequency", V::Of(simplexFrequency));
    v("jitter", V::Of(jitter));
    v("jitterHeight", V::Of(jitterHeight));
    v("rFactor", V::Of(rFactor));
    v("rOffset", V::Of(rOffset));
    v("colorOpacity", V::Of(colorOpacity));
    v("DebugColorMapping", V::Of(DebugColorMapping));
}

// ---- PQSMod_VoronoiCraters2.cs -------------------------------------------------------------------------------------
// PQSMod_VoronoiCraters2.cs:64
void PQSMod_VoronoiCraters2::OnSetup() {
    voronoi = std::make_shared<LibNoise::Voronoi>(voronoiFrequency, 0.0, voronoiSeed, true);
    jitterSimplex = std::make_shared<Simplex>(jitterSeed, jitterOctaves, jitterPersistence, jitterFrequency);
    deformationSimplex = std::make_shared<Simplex>(deformationSeed, deformationOctaves, deformationPersistence, deformationFrequency);
}

// PQSMod_VoronoiCraters2.cs:71 (sic: the nearest cell is in frequency-scaled space, the direction is not)
void PQSMod_VoronoiCraters2::OnVertexBuildHeight(PQSVertexBuildData& data) {
    const Vector3d& dir = data.directionFromCenter;
    nearest = voronoi->GetNearest(dir);
    xd = nearest.x - dir.x;
    yd = nearest.y - dir.y;
    zd = nearest.z - dir.z;
    h = dotnet::Math::Sqrt(xd * xd + yd * yd + zd * zd) * 1.7320508075688772;
    if (h < 0.0) {
        h = 0.0;
    } else if (h > 1.0) {
        h = 1.0;
    }
    s = jitterSimplex->noise(dir.x, dir.y, dir.z) * jitter;
    r = craterCurve.Evaluate(static_cast<float>(h + s));
    d = deformationSimplex->noiseNormalized(nearest.x, nearest.y, nearest.z) * deformation;
    data.vertHeight += d * r;
}

void PQSMod_VoronoiCraters2::VisitSerialized(const SerializedFieldVisitor& v) const {
    VisitCurveless(*this, v);
    v("deformation", V::Of(deformation));
    v("voronoiSeed", V::Of(voronoiSeed));
    v("voronoiFrequency", V::Of(voronoiFrequency));
    v("craterCurve", V::Of(craterCurve));
    v("jitterSeed", V::Of(jitterSeed));
    v("jitterOctaves", V::Of(jitterOctaves));
    v("jitterPersistence", V::Of(jitterPersistence));
    v("jitterFrequency", V::Of(jitterFrequency));
    v("jitter", V::Of(jitter));
    v("deformationSeed", V::Of(deformationSeed));
    v("deformationOctaves", V::Of(deformationOctaves));
    v("deformationPersistence", V::Of(deformationPersistence));
    v("deformationFrequency", V::Of(deformationFrequency));
    v("DebugColorMapping", V::Of(DebugColorMapping));
}

}  // namespace ksp
