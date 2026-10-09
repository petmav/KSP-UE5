// WP-11: port of the height-evaluation path of Assembly-CSharp/PQS.cs and the PQSMod base (PQSMod.cs).
#include "ksp/terrain/pqs.hpp"

#include <cmath>
#include <limits>

#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/terrain/map_so.hpp"

namespace ksp {

SerializedValue SerializedValue::Of(const std::shared_ptr<const MapSO>& v) {
    SerializedValue s;
    s.kind = Kind::Map;
    s.text = v ? v->GetOrigin().key : std::string();
    return s;
}

// PQSMod.cs:29
void PQSMod::OnSetup() {
    requirements = PQS::Default;
    if (modExpansionDisabled) modEnabled = false;
}

void PQSMod::VisitSerialized(const SerializedFieldVisitor& v) const {
    v("requirements", SerializedValue::Of(requirements));
    v("modEnabled", SerializedValue::Of(modEnabled));
    v("order", SerializedValue::Of(order));
    v("modExpansionDisabled", SerializedValue::Of(modExpansionDisabled));
}

// PQS.cs:758-761 (the part of the sphere setup the height path depends on)
void PQS::SetupForHeight() {
    circumference = 6.2831854820251465 * radius;  // PQS.cs:758 (float 2*pi widened)
    radiusSquared = radius * radius;
    SetupMods();
    SetupBuildDelegates(false);
}

// PQS.cs:2109
void PQS::SetupMods() {
    std::vector<PQSMod*> list;
    list.reserve(childMods.size());
    for (auto& m : childMods) list.push_back(m.get());
    // list.Sort((a, b) => a.order.CompareTo(b.order)): List<T>.Sort -> Array.Sort(_items, 0, _size) whose introsort
    // depth limit is 2 * FloorLog2(_items.Length) (the List's capacity).
    dotnet::ListSort(
        list, [](PQSMod* a, PQSMod* b) { return a->order < b->order ? -1 : (a->order > b->order ? 1 : 0); },
        childModListCapacity > 0 ? childModListCapacity : static_cast<int>(list.size()));
    if (!list.empty()) {
        size_t count = list.size();
        while (count-- > 0) {
            list[count]->sphere = this;
            if (!list[count]->modEnabled || !list[count]->gameObjectActiveSelf || list[count]->modExpansionDisabled)
                list.erase(list.begin() + static_cast<std::ptrdiff_t>(count));
        }
    }
    mods = list;
    Mod_OnSetup();
    modRequirements = Default;
    for (PQSMod* m : mods) modRequirements |= m->requirements;
}

// PQS.cs:2177 (only the flags and radii; the build delegates are WP-12)
void PQS::SetupBuildDelegates(bool fakeBuild) {
    isFakeBuild = fakeBuild;
    reqVertexMapCoods = (modRequirements & VertexMapCoords) != 0;
    reqGnomonicCoords = (modRequirements & VertexGnomonicMapCoords) != 0;
    reqCustomNormals = (modRequirements & MeshCustomNormals) != 0;
    reqColorChannel = (modRequirements & MeshColorChannel) != 0;
    reqBuildTangents = (modRequirements & MeshBuildTangents) != 0 || buildTangents;
    reqAssignTangents = (modRequirements & MeshAssignTangents) != 0 || reqBuildTangents;
    reqUV2 = (modRequirements & MeshUV2) != 0;
    reqUV3 = (modRequirements & MeshUV3) != 0;
    reqUV4 = (modRequirements & MeshUV4) != 0;
    reqSphereUV = (modRequirements & UVSphereCoords) != 0;
    reqUVQuad = (modRequirements & UVQuadCoords) != 0;
    if (reqSphereUV) {
        reqVertexMapCoods = true;
        if (reqUVQuad) reqUVQuad = false;  // Debug.LogWarning("Cannot have both Sphere UV and Quad UV mods enabled...")
    }
    useSharedMaterial = (modRequirements & UniqueMaterialInstances) == 0;
    radiusMax = radius + Mod_GetVertexMaxHeight();
    radiusMin = radius + Mod_GetVertexMinHeight();
    radiusDelta = radiusMax - radiusMin;
    halfChord = dotnet::Math::Sqrt(radiusMax * radiusMax - radiusMin * radiusMin);
}

// PQS.cs:1827
void PQS::Mod_OnSetup() {
    for (PQSMod* m : mods) m->OnSetup();
}

// PQS.cs:1892
double PQS::Mod_GetVertexMaxHeight() {
    double num = 0.0;
    for (PQSMod* m : mods) num += m->GetVertexMaxHeight();
    return num;
}

// PQS.cs:1902
double PQS::Mod_GetVertexMinHeight() {
    double num = 0.0;
    for (PQSMod* m : mods) num += m->GetVertexMinHeight();
    return num;
}

// PQS.cs:2077
void PQS::Mod_OnQuadPreBuild(const PQ& quad) {
    for (PQSMod* m : mods) m->OnQuadPreBuild(quad);
}

// PQS.cs:2085
void PQS::Mod_OnQuadBuilt(const PQ& quad) {
    for (PQSMod* m : mods) m->OnQuadBuilt(quad);
}

// PQS.cs:1956
void PQS::Mod_OnVertexBuildHeight(VertexBuildData& data, bool overrideQuadBuildCheck) {
    if (reqVertexMapCoods) BuildVertexMapCoords(data);
    for (PQSMod* m : mods) {
        m->overrideQuadBuildCheck = overrideQuadBuildCheck;
        m->OnVertexBuildHeight(data);
        m->overrideQuadBuildCheck = false;
    }
}

// PQS.cs:2294 (System.Math.Asin: the runtime's CRT asin; double.IsNaN)
void PQS::BuildVertexMapCoords(VertexBuildData& data) {
    latitude = dotnet::Math::Asin(data.directionFromCenter.y);
    if (std::isnan(latitude)) latitude = 3.141592653589793 / 2.0;
    directionXZ = Vector3d(data.directionFromCenter.x, 0.0, data.directionFromCenter.z).normalized();
    if (directionXZ.magnitude() > 0.0) {
        longitude = (directionXZ.z < 0.0) ? (3.141592653589793 - dotnet::Math::Asin(directionXZ.x / directionXZ.magnitude()))
                                          : dotnet::Math::Asin(directionXZ.x / directionXZ.magnitude());
    } else {
        longitude = 0.0;
    }
    data.latitude = latitude;
    data.longitude = longitude;
    sy = latitude / 3.141592653589793 + 0.5;
    sx = longitude / 3.141592653589793 * 0.5;
    data.u = sx;
    data.v = sy;
}

// PQS.cs:3088
double PQS::GetSurfaceHeight(const Vector3d& radialVector) { return GetSurfaceHeight(radialVector, false); }

// PQS.cs:3093
double PQS::GetSurfaceHeight(const Vector3d& radialVector, bool overrideQuadBuildCheck) {
    vbData_.Reset();
    vbData_.directionFromCenter = radialVector.normalized();
    vbData_.vertHeight = radius;
    vbData_.vertIndex = vertexIndex;
    Mod_OnVertexBuildHeight(vbData_, overrideQuadBuildCheck);
    return vbData_.vertHeight;
}

}  // namespace ksp
