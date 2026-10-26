// WP-11: PQSLandControl (Assembly-CSharp/PQSLandControl.cs, height path: land-class blending and the
// minimumRealHeight / alterRealHeight adjustment; the scatter half is WP-12d's, pqs_land_scatter.cpp) and PQSMod_VertexPlanet
// (PQSMod_VertexPlanet.cs).
#include <limits>
#include <string>

#include "ksp/dotnet/math.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/terrain/pqs_mods.hpp"

namespace ksp {

namespace {
using V = SerializedValue;

void VisitLerpRange(const SerializedFieldVisitor& v, const std::string& p, const PQSLandControl::LerpRange& r) {
    v(p + ".startStart", V::Of(r.startStart));
    v(p + ".startEnd", V::Of(r.startEnd));
    v(p + ".endStart", V::Of(r.endStart));
    v(p + ".endEnd", V::Of(r.endEnd));
    v(p + ".startDelta", V::Of(r.startDelta));
    v(p + ".endDelta", V::Of(r.endDelta));
}
}  // namespace

// ---- PQSLandControl.LerpRange ----------------------------------------------------------------------------------------
// PQSLandControl.cs:26
void PQSLandControl::LerpRange::Setup() {
    startDelta = 1.0 / (startEnd - startStart);
    endDelta = 1.0 / (endEnd - endStart);
}

// PQSLandControl.cs:32
double PQSLandControl::LerpRange::Lerp(double point) const {
    if (!(point <= startStart) && point < endEnd) {
        if (point < startEnd) return (point - startStart) * startDelta;
        if (point <= endStart) return 1.0;
        if (point < endEnd) return 1.0 - (point - endStart) * endDelta;
        return 0.0;
    }
    return 0.0;
}

// PQSLandControl.cs:885
double PQSLandControl::Lerp(double v1, double v2, double dt) { return v2 * dt + v1 * (1.0 - dt); }

// PQSLandControl.cs:662 (WP-12d: with the scatter matching and LandClassScatter.Setup)
void PQSLandControl::OnSetup() {
    requirements = PQS::VertexMapCoords | PQS::MeshColorChannel;
    lcList.clear();
    lcList.reserve(landClasses.size());
    for (LandClass& landClass : landClasses) {
        landClass.altitudeRange.Setup();
        landClass.latitudeRange.Setup();
        landClass.latitudeDoubleRange.startStart = 1.0 - landClass.latitudeRange.endEnd;
        landClass.latitudeDoubleRange.startEnd = 1.0 - landClass.latitudeRange.endStart;
        landClass.latitudeDoubleRange.endStart = 1.0 - landClass.latitudeRange.startEnd;
        landClass.latitudeDoubleRange.endEnd = 1.0 - landClass.latitudeRange.startStart;
        landClass.latitudeDoubleRange.Setup();
        landClass.longitudeRange.Setup();
        landClass.coverageSimplex = std::make_shared<Simplex>(landClass.coverageSeed, landClass.coverageOctaves,
                                                              landClass.coveragePersistance, landClass.coverageFrequency);
        landClass.noiseSimplex = std::make_shared<Simplex>(landClass.noiseSeed, landClass.noiseOctaves, landClass.noisePersistance,
                                                           landClass.noiseFrequency);
        for (LandClassScatterAmount& landClassScatterAmount : landClass.scatter) {
            for (size_t k = 0; k < scatters.size(); k++) {
                if (landClassScatterAmount.scatterName == scatters[k].scatterName) {
                    landClassScatterAmount.scatter = &scatters[k];
                    landClassScatterAmount.scatter->castShadows = GameSettings::CELESTIAL_BODIES_CAST_SHADOWS;
                    landClassScatterAmount.scatter->recieveShadows = GameSettings::CELESTIAL_BODIES_CAST_SHADOWS;
                    landClassScatterAmount.scatterIndex = static_cast<int>(k);
                    break;
                }
            }
        }
    }
    if (PQS::Global_AllowScatter) {
        scatterCount = static_cast<int>(scatters.size());
        lcScatterList.assign(scatters.size(), 0.0);
        scatterMinSubdiv = std::numeric_limits<int>::max();
        for (LandClassScatter& landClassScatter : scatters) {
            landClassScatter.Setup(sphere);
            if (landClassScatter.minLevel() < scatterMinSubdiv) scatterMinSubdiv = landClassScatter.minLevel();
        }
    }
    altitudeSimplex = std::make_shared<Simplex>(altitudeSeed, altitudeOctaves, altitudePersistance, altitudeFrequency);
    latitudeSimplex = std::make_shared<Simplex>(latitudeSeed, latitudeOctaves, latitudePersistance, latitudeFrequency);
    longitudeSimplex = std::make_shared<Simplex>(longitudeSeed, longitudeOctaves, longitudePersistance, longitudeFrequency);
}

// PQSLandControl.cs:738
void PQSLandControl::OnVertexBuildHeight(PQSVertexBuildData& data) {
    lcList.clear();
    lcListCount = 0;
    totalDelta = 0.0;
    data.vertColor = Color::black();
    if (useHeightMap) {
        vHeight = heightMap->GetPixelFloat(data.u, data.v);
    } else {
        vHeight = (data.vertHeight - sphere->radius) / static_cast<double>(vHeightMax);
    }
    vHeight += static_cast<double>(altitudeBlend) * altitudeSimplex->noise(data.directionFromCenter);
    if (vHeight > 1.0) vHeight = 1.0;
    vLat = sphere->sy + static_cast<double>(latitudeBlend) * latitudeSimplex->noise(data.directionFromCenter);
    if (vLat > 1.0) {
        vLat = 1.0;
    } else if (vLat < 0.0) {
        vLat = 0.0;
    }
    vLon = sphere->sx + static_cast<double>(longitudeBlend) * longitudeSimplex->noise(data.directionFromCenter);
    if (vLon > 1.0) {
        vLon = 1.0;
    } else if (vLon < 0.0) {
        vLon = 0.0;
    }
    for (LandClass& lc : landClasses) {
        lc.altDelta = lc.altitudeRange.Lerp(vHeight);
        lc.latDelta = lc.latitudeRange.Lerp(vLat);
        if (lc.latitudeDouble) lc.latDelta = dotnet::Math::Max(lc.latitudeDoubleRange.Lerp(vLat), lc.latDelta);
        lc.lonDelta = lc.longitudeRange.Lerp(vLon);
        lc.delta = lc.altDelta * lc.latDelta * lc.lonDelta;
        lc.delta = Lerp(lc.delta, lc.delta * lc.coverageSimplex->noiseNormalized(data.directionFromCenter),
                        static_cast<double>(lc.coverageBlend));
        if (lc.delta != 0.0) {
            totalDelta += lc.delta;
            lcList.push_back(&lc);
            lcListCount++;
        }
    }
    for (int itr = 0; itr < lcListCount; itr++) {
        LandClass& lc = *lcList[static_cast<size_t>(itr)];
        lc.delta /= totalDelta;
        if (lc.delta > 0.0) {
            if (lc.minimumRealHeight != 0.0 && data.vertHeight - sphere->radius < lc.minimumRealHeight)
                data.vertHeight = sphere->radius + lc.delta * lc.minimumRealHeight;
            data.vertHeight += lc.delta * lc.alterRealHeight;
        }
    }
}

void PQSLandControl::VisitSerialized(const SerializedFieldVisitor& v) const {
    PQSMod::VisitSerialized(v);
    for (size_t i = 0; i < landClasses.size(); ++i) {
        const LandClass& lc = landClasses[i];
        const std::string p = "landClasses[" + std::to_string(i) + "].";
        v(p + "landClassName", V::Of(lc.landClassName));
        v(p + "color", V::Of(lc.color));
        v(p + "noiseColor", V::Of(lc.noiseColor));
        VisitLerpRange(v, p + "altitudeRange", lc.altitudeRange);
        VisitLerpRange(v, p + "latitudeRange", lc.latitudeRange);
        v(p + "latitudeDouble", V::Of(lc.latitudeDouble));
        VisitLerpRange(v, p + "latitudeDoubleRange", lc.latitudeDoubleRange);
        VisitLerpRange(v, p + "longitudeRange", lc.longitudeRange);
        v(p + "coverageBlend", V::Of(lc.coverageBlend));
        v(p + "coverageSeed", V::Of(lc.coverageSeed));
        v(p + "coverageOctaves", V::Of(lc.coverageOctaves));
        v(p + "coveragePersistance", V::Of(lc.coveragePersistance));
        v(p + "coverageFrequency", V::Of(lc.coverageFrequency));
        v(p + "noiseBlend", V::Of(lc.noiseBlend));
        v(p + "noiseSeed", V::Of(lc.noiseSeed));
        v(p + "noiseOctaves", V::Of(lc.noiseOctaves));
        v(p + "noisePersistance", V::Of(lc.noisePersistance));
        v(p + "noiseFrequency", V::Of(lc.noiseFrequency));
        v(p + "minimumRealHeight", V::Of(lc.minimumRealHeight));
        v(p + "alterRealHeight", V::Of(lc.alterRealHeight));
        v(p + "alterApparentHeight", V::Of(lc.alterApparentHeight));
        v(p + "altDelta", V::Of(lc.altDelta));
        v(p + "latDelta", V::Of(lc.latDelta));
        v(p + "lonDelta", V::Of(lc.lonDelta));
        v(p + "delta", V::Of(lc.delta));
        for (size_t j = 0; j < lc.scatter.size(); ++j) {
            const std::string q = p + "scatter[" + std::to_string(j) + "].";
            v(q + "scatterName", V::Of(lc.scatter[j].scatterName));
            v(q + "density", V::Of(lc.scatter[j].density));
        }
    }
    for (size_t i = 0; i < scatters.size(); ++i) {
        const LandClassScatter& sc = scatters[i];
        const std::string p = "scatters[" + std::to_string(i) + "].";
        v(p + "scatterName", V::Of(sc.scatterName));
        v(p + "seed", V::Of(sc.seed));
        v(p + "maxCache", V::Of(sc.maxCache));
        v(p + "maxCacheDelta", V::Of(sc.maxCacheDelta));
        v(p + "maxScatter", V::Of(sc.maxScatter));
        v(p + "densityFactor", V::Of(sc.densityFactor));
        v(p + "maxLevelOffset", V::Of(sc.maxLevelOffset));
        v(p + "verticalOffset", V::Of(sc.verticalOffset));
        v(p + "minScale", V::Of(sc.minScale));
        v(p + "maxScale", V::Of(sc.maxScale));
        v(p + "maxSpeed", V::Of(sc.maxSpeed));
    }
    v("useHeightMap", V::Of(useHeightMap));
    v("heightMap", V::Of(heightMap));
    v("vHeightMax", V::Of(vHeightMax));
    v("createColors", V::Of(createColors));
    v("createScatter", V::Of(createScatter));
    v("altitudeBlend", V::Of(altitudeBlend));
    v("altitudeSeed", V::Of(altitudeSeed));
    v("altitudeOctaves", V::Of(altitudeOctaves));
    v("altitudePersistance", V::Of(altitudePersistance));
    v("altitudeFrequency", V::Of(altitudeFrequency));
    v("latitudeBlend", V::Of(latitudeBlend));
    v("latitudeSeed", V::Of(latitudeSeed));
    v("latitudeOctaves", V::Of(latitudeOctaves));
    v("latitudePersistance", V::Of(latitudePersistance));
    v("latitudeFrequency", V::Of(latitudeFrequency));
    v("longitudeBlend", V::Of(longitudeBlend));
    v("longitudeSeed", V::Of(longitudeSeed));
    v("longitudeOctaves", V::Of(longitudeOctaves));
    v("longitudePersistance", V::Of(longitudePersistance));
    v("longitudeFrequency", V::Of(longitudeFrequency));
}

// ---- PQSMod_VertexPlanet ---------------------------------------------------------------------------------------------
// PQSMod_VertexPlanet.cs:37
void PQSMod_VertexPlanet::SimplexWrapper::Setup(int seed) {
    simplex = std::make_shared<Simplex>(seed, octaves, persistance, frequency);
}

// PQSMod_VertexPlanet.cs:240
double PQSMod_VertexPlanet::Clamp(double v, double low, double high) {
    if (v < low) return low;
    if (v > high) return high;
    return v;
}

// PQSMod_VertexPlanet.cs:350
double PQSMod_VertexPlanet::Lerp(double v2, double v1, double dt) { return v1 * dt + v2 * (1.0 - dt); }

// PQSMod_VertexPlanet.cs:355
double PQSMod_VertexPlanet::CubicHermite(double start, double end, double startTangent, double endTangent, double t) {
    ct2 = t * t;
    ct3 = ct2 * t;
    return start * (2.0 * ct3 - 3.0 * ct2 + 1.0) + startTangent * (ct3 - 2.0 * ct2 + t) + end * (-2.0 * ct3 + 3.0 * ct2) +
           endTangent * (ct3 - ct2);
}

// PQSMod_VertexPlanet.cs:219 (int arithmetic unchecked; sic: the sharpness RidgedMultifractal gets `persistance` as
// its lacunarity)
void PQSMod_VertexPlanet::OnSetup() {
    requirements = PQS::MeshColorChannel | PQS::MeshCustomNormals;
    lcCount = static_cast<int>(landClasses.size());
    for (int itr = 0; itr < lcCount; itr++) {
        LandClass& lc = landClasses[static_cast<size_t>(itr)];
        lc.fractalDelta = lc.fractalEnd - lc.fractalStart;
        lc.colorNoiseMap.Setup(static_cast<int>(static_cast<uint32_t>(seed) + static_cast<uint32_t>(itr) + 10u));
    }
    int num = seed;
    auto next = [&num]() {
        const int r = num;
        num = static_cast<int>(static_cast<uint32_t>(num) + 1u);
        return r;
    };
    continental.Setup(next());
    continentalRuggedness.Setup(next());
    continentalSmoothing = SimplexWrapper();  // new SimplexWrapper(continental): copies the four parameters
    continentalSmoothing.deformity = continental.deformity;
    continentalSmoothing.octaves = continental.octaves;
    continentalSmoothing.persistance = continental.persistance;
    continentalSmoothing.frequency = continental.frequency;
    continentalSmoothing.Setup(seed);
    continentalSmoothing.persistance = continental.persistance * terrainSmoothing;
    continentalSmoothing.frequency = continental.frequency;
    terrainType.Setup(next());
    continentalSharpness.Setup(std::make_shared<LibNoise::RidgedMultifractal>(
        continentalSharpness.frequency, continentalSharpness.persistance, continentalSharpness.octaves, next(),
        LibNoise::NoiseQuality::High));
    continentalSharpnessMap.Setup(next());
}

// PQSMod_VertexPlanet.cs:253 (System.Math.Round(x, 5): banker's rounding through the runtime's InternalRound)
void PQSMod_VertexPlanet::OnVertexBuildHeight(PQSVertexBuildData& data) {
    const Vector3d& dir = data.directionFromCenter;
    continentalDeformity = 1.0;
    continental2Height = continentalSmoothing.simplex->noiseNormalized(dir);
    continental.simplex->persistence = continental.persistance - continentalSmoothing.persistance * continental2Height;
    continentialHeight = continental.simplex->noiseNormalized(dir);
    continentialSharpnessValue = (continentalSharpness.noise->GetValue(dir) + 1.0) * 0.5;
    continentialSharpnessValue *= Lerp(continentalSharpness.deformity, continentalSharpness.deformity * terrainRidgeBalance,
                                       (continental2Height + continentialSharpnessValue) * 0.5);
    continentialSharpnessMapValue =
        Clamp((continentalSharpnessMap.simplex->noise(dir) + 1.0) * 0.5, terrainRidgesMin, terrainRidgesMax);
    continentialSharpnessMapValue =
        (continentialSharpnessMapValue - terrainRidgesMin) / (terrainRidgesMax - terrainRidgesMin) * continentalSharpnessMap.deformity;
    continentialSharpnessValue += Lerp(0.0, continentialSharpnessValue, continentialSharpnessMapValue);
    continentialHeight += continentialSharpnessValue;
    continentalDeformity += continentalSharpness.deformity * continentalSharpnessMap.deformity;
    continentialHeight /= continentalDeformity;
    continentalDelta = (continentialHeight - oceanLevel) / (1.0 - oceanLevel);
    if (continentialHeight < oceanLevel) {
        if (oceanSnap) {
            vHeight = 0.0 - oceanStep;
        } else {
            vHeight = continentalDelta * oceanDepth - oceanStep;
        }
        continentialHeightPreSmooth = vHeight;
    } else {
        continentalRuggedness.simplex->persistence = continentalRuggedness.persistance * continentalDelta;
        continentalRHeight = continentalRuggedness.simplex->noiseNormalized(dir) * continentalDelta * continentalDelta;
        continentialHeight = continentalDelta * continental.deformity + continentalRHeight * continentalRuggedness.deformity;
        continentialHeight /= continental.deformity + continentalRuggedness.deformity;
        continentialHeightPreSmooth = continentialHeight;
        continentialHeight = CubicHermite(0.0, 1.0, terrainShapeStart, terrainShapeEnd, continentialHeight);
        vHeight = continentialHeight;
    }
    data.vertHeight += dotnet::Math::Round(vHeight, 5, dotnet::MidpointRounding::ToEven) * deformity;
}

void PQSMod_VertexPlanet::VisitSerialized(const SerializedFieldVisitor& v) const {
    PQSMod::VisitSerialized(v);
    auto sw = [&](const std::string& p, const SimplexWrapper& w) {
        v(p + ".deformity", V::Of(w.deformity));
        v(p + ".octaves", V::Of(w.octaves));
        v(p + ".persistance", V::Of(w.persistance));
        v(p + ".frequency", V::Of(w.frequency));
    };
    v("seed", V::Of(seed));
    v("deformity", V::Of(deformity));
    v("colorDeformity", V::Of(colorDeformity));
    v("oceanLevel", V::Of(oceanLevel));
    v("oceanStep", V::Of(oceanStep));
    v("oceanDepth", V::Of(oceanDepth));
    v("oceanSnap", V::Of(oceanSnap));
    v("terrainSmoothing", V::Of(terrainSmoothing));
    v("terrainShapeStart", V::Of(terrainShapeStart));
    v("terrainShapeEnd", V::Of(terrainShapeEnd));
    v("terrainRidgesMin", V::Of(terrainRidgesMin));
    v("terrainRidgesMax", V::Of(terrainRidgesMax));
    sw("continental", continental);
    sw("continentalRuggedness", continentalRuggedness);
    sw("continentalSharpnessMap", continentalSharpnessMap);
    v("continentalSharpness.deformity", V::Of(continentalSharpness.deformity));
    v("continentalSharpness.octaves", V::Of(continentalSharpness.octaves));
    v("continentalSharpness.persistance", V::Of(continentalSharpness.persistance));
    v("continentalSharpness.frequency", V::Of(continentalSharpness.frequency));
    sw("terrainType", terrainType);
    v("buildHeightColors", V::Of(buildHeightColors));
    for (size_t i = 0; i < landClasses.size(); ++i) {
        const LandClass& lc = landClasses[i];
        const std::string p = "landClasses[" + std::to_string(i) + "].";
        v(p + "name", V::Of(lc.name));
        v(p + "fractalStart", V::Of(lc.fractalStart));
        v(p + "fractalEnd", V::Of(lc.fractalEnd));
        v(p + "baseColor", V::Of(lc.baseColor));
        v(p + "colorNoise", V::Of(lc.colorNoise));
        v(p + "colorNoiseAmount", V::Of(lc.colorNoiseAmount));
        sw(p + "colorNoiseMap", lc.colorNoiseMap);
        v(p + "lerpToNext", V::Of(lc.lerpToNext));
    }
    v("terrainRidgeBalance", V::Of(terrainRidgeBalance));
}

}  // namespace ksp
