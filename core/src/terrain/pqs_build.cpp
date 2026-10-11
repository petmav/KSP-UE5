// WP-12a: the PQS quad build and normal stitching (Assembly-CSharp/PQS.cs BuildQuad :2237, the BuildVertex* / BuildMesh*
// channel builders :2318-2417, BuildNormals :1760, BuildTangents :1619, BackupEdgeNormals :2471, UpdateEdgeNormals :2503,
// CombineEdgeNormals :2951, GetRightmostCornerNormal :2967). See include/ksp/terrain/pqs.hpp and pq.hpp.
//
// Float evaluation: Vector3 / Vector2 / Vector4 code is managed (Mono rule through the ksp::Vector*f operators and
// f32store); Matrix4x4.TRS and Vector3.OrthoNormalize are native (ksp/unity/matrix_native.hpp); Vector3d is double.
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/terrain/pqs_parallel.hpp"
#include "ksp/unity/matrix_native.hpp"

namespace ksp {

namespace {
template <class T>
T& Deref(T* p) {
    if (p == nullptr) throw NullReferenceException();
    return *p;
}
size_t Z(int i) { return static_cast<size_t>(i); }
// Vector3d -> Vector3 implicit conversion (Vector3d.cs: (float) per component)
Vector3f F(const Vector3d& v) { return v.ToVector3f(); }
}  // namespace

// PQS.cs:1619 BuildTangents (static)
void PQS::BuildTangents(PQ& quad) {
    PQSMeshCache& c = meshCache();
    for (int i = 0; i < c.cacheVertCount; i++) {
        Vector3f normal = quad.mesh.normals[Z(i)];
        Vector3f tangent = Vector3f::zero();
        UnityNative::OrthoNormalize(normal, tangent);
        c.cacheTangents[Z(i)].x = tangent.x;
        c.cacheTangents[Z(i)].y = tangent.y;
        c.cacheTangents[Z(i)].z = tangent.z;
        c.cacheTangents[Z(i)].w = (Vector3f::Dot(Vector3f::Cross(normal, tangent), c.tan2[Z(i)]) < 0.0f) ? -1.0f : 1.0f;
    }
    quad.mesh.tangents = c.cacheTangents;
}

// PQS.cs:1760 BuildNormals (static): triangle normals from the double scratch vertices, summed per vertex, normalised
void PQS::BuildNormals(PQ& quad) {
    PQSMeshCache& c = meshCache();
    const std::vector<int>& idx = c.cacheIndices[0];
    int num = 0;
    int num2 = 0;
    Vector3f vector3;
    while (num < c.cacheTriCount) {
        // Vector3d - Vector3d, then the implicit Vector3d -> Vector3 conversion
        const Vector3f vector = F(c.verts[Z(idx[Z(num2 + 1)])] - c.verts[Z(idx[Z(num2)])]);
        const Vector3f vector2 = F(c.verts[Z(idx[Z(num2 + 2)])] - c.verts[Z(idx[Z(num2)])]);
        vector3.x = f32store(fx(vector.y) * fx(vector2.z) - fx(vector.z) * fx(vector2.y));
        vector3.y = f32store(fx(vector.z) * fx(vector2.x) - fx(vector.x) * fx(vector2.z));
        vector3.z = f32store(fx(vector.x) * fx(vector2.y) - fx(vector.y) * fx(vector2.x));
        c.triNormals[Z(num)] = vector3.normalized();
        num++;
        num2 += 3;
    }
    for (num2 = 0; num2 < c.cacheVertCount; num2++) {
        quad.vertNormals[Z(num2)].x = 0.0f;
        quad.vertNormals[Z(num2)].y = 0.0f;
        quad.vertNormals[Z(num2)].z = 0.0f;
    }
    num = 0;
    num2 = 0;
    while (num < c.cacheTriCount) {
        quad.vertNormals[Z(idx[Z(num2)])] = quad.vertNormals[Z(idx[Z(num2)])] + c.triNormals[Z(num)];
        quad.vertNormals[Z(idx[Z(num2 + 1)])] = quad.vertNormals[Z(idx[Z(num2 + 1)])] + c.triNormals[Z(num)];
        quad.vertNormals[Z(idx[Z(num2 + 2)])] = quad.vertNormals[Z(idx[Z(num2 + 2)])] + c.triNormals[Z(num)];
        num++;
        num2 += 3;
    }
    for (num2 = 0; num2 < static_cast<int>(quad.vertNormals.size()); num2++)
        quad.vertNormals[Z(num2)] = quad.vertNormals[Z(num2)].normalized();
}

// PQS.cs:1798 BuildNormalsNoClear (static; no caller in Assembly-CSharp)
void PQS::BuildNormalsNoClear(PQ& quad) {
    PQSMeshCache& c = meshCache();
    const std::vector<int>& array = c.cacheIndices[0];
    int num = 0;
    int num2 = 0;
    Vector3f vector3;
    while (num < c.cacheTriCount) {
        const Vector3f vector = F(c.verts[Z(array[Z(num2 + 1)])] - c.verts[Z(array[Z(num2)])]);
        const Vector3f vector2 = F(c.verts[Z(array[Z(num2 + 2)])] - c.verts[Z(array[Z(num2)])]);
        vector3.x = f32store(fx(vector.y) * fx(vector2.z) - fx(vector.z) * fx(vector2.y));
        vector3.y = f32store(fx(vector.z) * fx(vector2.x) - fx(vector.x) * fx(vector2.z));
        vector3.z = f32store(fx(vector.x) * fx(vector2.y) - fx(vector.y) * fx(vector2.x));
        c.triNormals[Z(num)] = vector3.normalized();
        num++;
        num2 += 3;
    }
    num = 0;
    num2 = 0;
    while (num < c.cacheTriCount) {
        quad.vertNormals[Z(array[Z(num2)])] = (quad.vertNormals[Z(array[Z(num2)])] + c.triNormals[Z(num)]).normalized();
        quad.vertNormals[Z(array[Z(num2 + 1)])] = (quad.vertNormals[Z(array[Z(num2 + 1)])] + c.triNormals[Z(num)]).normalized();
        quad.vertNormals[Z(array[Z(num2 + 2)])] = (quad.vertNormals[Z(array[Z(num2 + 2)])] + c.triNormals[Z(num)]).normalized();
        num++;
        num2 += 3;
    }
}

// PQS.cs:1970 Mod_OnVertexBuild
void PQS::Mod_OnVertexBuild(VertexBuildData& data) {
    for (PQSMod* m : mods) m->OnVertexBuild(data);
    if (!isFakeBuild) {
        if (surfaceRelativeQuads) {
            BuildVertexSurfaceRelative(data);
        } else {
            BuildVertexHeight(data);
        }
        if (!reqCustomNormals) BuildVertexSphereNormal(data);
        if (reqColorChannel) BuildVertexColor(data);
        if (reqSphereUV) BuildVertexSphereUV(data);
        if (reqUVQuad) BuildVertexQuadUV(data);
        if (reqUV2) BuildVertexUV2(data);
        if (reqUV3) BuildVertexUV3(data);
        if (reqUV4) BuildVertexUV4(data);
    }
}

// PQS.cs:2017 Mod_OnMeshBuild
void PQS::Mod_OnMeshBuild() {
    for (PQSMod* m : mods) m->OnMeshBuild();
    if (reqCustomNormals) {
        BuildMeshCustomNormals();
    } else {
        BuildMeshSphereNormals();
    }
    if (reqSphereUV || reqUVQuad) BuildMeshUVChannel();
    if (reqUV2) BuildMeshUV2Channel();
    if (reqUV3) BuildMeshUV3Channel();
    if (reqUV4) BuildMeshUV4Channel();
    if (reqColorChannel) BuildMeshColorChannel();
    if (reqBuildTangents) BuildMeshTangents();
    if (reqAssignTangents) AssignMeshTangents();
}

// PQS.cs:2237 BuildQuad
bool PQS::BuildQuad(PQ& quad) {
    if (quad.isBuilt) return false;
    if (quad.isSubdivided) return false;
    PQSMeshCache& c = meshCache();
    buildQuad_ = &quad;
    Mod_OnQuadPreBuild(quad);
    vbData_.buildQuad = buildQuad_;
    vbData_.allowScatter = true;
    vbData_.gnomonicPlane = static_cast<int>(buildQuad_->plane);
    buildQuad_->meshVertMin = 1.7976931348623157e308;   // double.MaxValue
    buildQuad_->meshVertMax = -1.7976931348623157e308;  // double.MinValue
    // WP-12a optimisation: the height pass evaluated in parallel beforehand (pqs_parallel.hpp), else inline
    const PQSBuildHeights* pre = heightWorkers != nullptr ? heightWorkers->Find(quad) : nullptr;
    if (pre != nullptr) heightWorkers->used++;
    for (vertexIndex = 0; vertexIndex < c.cacheVertCount; vertexIndex++) {
        vbData_.globalV = Vector3d(MultiplyPoint3x4(buildQuad_->quadMatrix, c.cacheVerts[Z(vertexIndex)]));
        vbData_.directionFromCenter = vbData_.globalV.normalized();
        vbData_.vertHeight = radius;
        vbData_.vertIndex = vertexIndex;
        if (pre == nullptr) {
            Mod_OnVertexBuildHeight(vbData_, false);
        } else {
            const size_t k = Z(vertexIndex);
            if (reqVertexMapCoods) BuildVertexMapCoords(vbData_);
            vbData_.vertHeight = pre->vertHeight[k];
            const uint8_t w = pre->colorWritten[k];
            if (w & 1) vbData_.vertColor.r = pre->vertColor[k].r;
            if (w & 2) vbData_.vertColor.g = pre->vertColor[k].g;
            if (w & 4) vbData_.vertColor.b = pre->vertColor[k].b;
            if (w & 8) vbData_.vertColor.a = pre->vertColor[k].a;
            vbData_.allowScatter = pre->allowScatter[k] != 0;
        }
        Mod_OnVertexBuild(vbData_);
        vbData_.vertHeight -= radius;
        if (vbData_.vertHeight > buildQuad_->meshVertMax) buildQuad_->meshVertMax = vbData_.vertHeight;
        if (vbData_.vertHeight < buildQuad_->meshVertMin) buildQuad_->meshVertMin = vbData_.vertHeight;
        if (vbData_.vertHeight > meshVertMax) meshVertMax = vbData_.vertHeight;
        if (vbData_.vertHeight < meshVertMin) meshVertMin = vbData_.vertHeight;
    }
    buildQuad_->mesh.vertices = buildQuad_->verts;
    buildQuad_->mesh.triangleSet = 0;  // mesh.triangles = cacheIndices[0]
    buildQuad_->mesh.RecalculateBounds();
    buildQuad_->edgeState = EdgeState::Reset;
    Mod_OnMeshBuild();
    Mod_OnQuadBuilt(quad);
    buildQuad_ = nullptr;
    return true;
}

// PQS.cs:2318 BuildVertexHeight (reads the static vbData, which is `data` here)
void PQS::BuildVertexHeight(VertexBuildData& data) {
    PQSMeshCache& c = meshCache();
    c.verts[Z(vertexIndex)] = data.directionFromCenter * data.vertHeight;
    buildQuad_->verts[Z(vertexIndex)] = F(c.verts[Z(vertexIndex)]);
    buildQuad_->vertsD[Z(vertexIndex)] = c.verts[Z(vertexIndex)];
}

// PQS.cs:2324 BuildVertexSurfaceRelative: planetRel = sphere.transform.TransformPoint(vertRel) and
// verts = quad.transform.InverseTransformPoint(planetRel) are Unity float world transforms; with the sphere's transform
// at the identity (the body-fixed frame) and the quad at localPosition (Vector3)positionPlanet, rotation identity,
// scale 1, they reduce to (Vector3)vertRel - (Vector3)positionPlanet: one float subtraction per component (the C#
// passes vertRel to TransformPoint through the implicit Vector3d -> Vector3 conversion). KSP's floating-origin world
// pose adds further float rounding that depends on where the scene origin is: not reproduced (dropped).
void PQS::BuildVertexSurfaceRelative(VertexBuildData& data) {
    PQSMeshCache& c = meshCache();
    c.vertRel = data.directionFromCenter * data.vertHeight;
    const Vector3f vertRelF = F(c.vertRel);
    c.planetRel = Vector3d(vertRelF);
    c.verts[Z(vertexIndex)] = c.vertRel;
    const Vector3f origin = F(buildQuad_->meshOrigin);
    buildQuad_->verts[Z(vertexIndex)] = Vector3f(vertRelF.x - origin.x, vertRelF.y - origin.y, vertRelF.z - origin.z);
    buildQuad_->vertsD[Z(vertexIndex)] = c.vertRel;
}

// PQS.cs:2332
void PQS::BuildVertexColor(VertexBuildData& data) { meshCache().cacheColors[Z(vertexIndex)] = data.vertColor; }

// PQS.cs:2337
void PQS::BuildVertexUV2(VertexBuildData& data) {
    PQSMeshCache& c = meshCache();
    c.cacheUV2s[Z(vertexIndex)].x = static_cast<float>(data.u2);
    c.cacheUV2s[Z(vertexIndex)].y = static_cast<float>(data.v2);
}

// PQS.cs:2343
void PQS::BuildVertexUV3(VertexBuildData& data) {
    PQSMeshCache& c = meshCache();
    c.cacheUV3s[Z(vertexIndex)].x = static_cast<float>(data.u3);
    c.cacheUV3s[Z(vertexIndex)].y = static_cast<float>(data.v3);
}

// PQS.cs:2349
void PQS::BuildVertexUV4(VertexBuildData& data) {
    PQSMeshCache& c = meshCache();
    c.cacheUV4s[Z(vertexIndex)].x = static_cast<float>(data.u4);
    c.cacheUV4s[Z(vertexIndex)].y = static_cast<float>(data.v4);
}

// PQS.cs:2355
void PQS::BuildVertexSphereNormal(VertexBuildData& data) { meshCache().normals[Z(vertexIndex)] = F(data.directionFromCenter); }

// PQS.cs:2360 (writes the `uvs` scratch, which no code assigns to a mesh)
void PQS::BuildVertexQuadUV(VertexBuildData& data) {
    PQSMeshCache& c = meshCache();
    const PQ& q = Deref(data.buildQuad);
    c.uvs[Z(vertexIndex)].x = f32store(fx(q.uvSW.x) + fx(c.cacheUVs[Z(vertexIndex)].x) * fx(q.uvDelta.x));
    c.uvs[Z(vertexIndex)].y = f32store(fx(q.uvSW.y) + fx(c.cacheUVs[Z(vertexIndex)].y) * fx(q.uvDelta.y));
}

// PQS.cs:2366
void PQS::BuildVertexSphereUV(VertexBuildData&) {
    PQSMeshCache& c = meshCache();
    c.uvs[Z(vertexIndex)].x = static_cast<float>(sx);
    c.uvs[Z(vertexIndex)].y = static_cast<float>(sy);
}

// PQS.cs:2372
void PQS::BuildMeshSphereNormals() { buildQuad_->mesh.normals = meshCache().normals; }

// PQS.cs:2377
void PQS::BuildMeshCustomNormals() {
    BuildNormals(*buildQuad_);
    BackupEdgeNormals();
    buildQuad_->mesh.normals = buildQuad_->vertNormals;
}

// PQS.cs:2384
void PQS::BuildMeshTangents() { BuildTangents(*buildQuad_); }

// PQS.cs:2389
void PQS::AssignMeshTangents() { buildQuad_->mesh.tangents = meshCache().cacheTangents; }

// PQS.cs:2394
void PQS::BuildMeshColorChannel() { buildQuad_->mesh.colors = meshCache().cacheColors; }

// PQS.cs:2399 (sic: the static cacheUVs, which PQSMod_UVPlanetRelativePosition rewrites)
void PQS::BuildMeshUVChannel() { buildQuad_->mesh.uv = meshCache().cacheUVs; }

// PQS.cs:2404
void PQS::BuildMeshUV2Channel() { buildQuad_->mesh.uv2 = meshCache().cacheUV2s; }

// PQS.cs:2409
void PQS::BuildMeshUV3Channel() { buildQuad_->mesh.uv3 = meshCache().cacheUV3s; }

// PQS.cs:2414
void PQS::BuildMeshUV4Channel() { buildQuad_->mesh.uv4 = meshCache().cacheUV4s; }

// PQS.cs:2471 BackupEdgeNormals
void PQS::BackupEdgeNormals() {
    const PQSMeshCache& c = meshCache();
    for (int i = 0; i < 4; i++) {
        switch (static_cast<QuadEdge>(i)) {
            case QuadEdge::North:
                BackupEdgeNormals(buildQuad_->edgeNormals[Z(i)], c.cacheSideVertCount, c.vi(0, 0), 1);
                break;
            case QuadEdge::South:
                BackupEdgeNormals(buildQuad_->edgeNormals[Z(i)], c.cacheSideVertCount, c.vi(c.cacheRes, c.cacheRes), -1);
                break;
            case QuadEdge::East:
                BackupEdgeNormals(buildQuad_->edgeNormals[Z(i)], c.cacheSideVertCount, c.vi(0, c.cacheRes), -c.cacheSideVertCount);
                break;
            case QuadEdge::West:
                BackupEdgeNormals(buildQuad_->edgeNormals[Z(i)], c.cacheSideVertCount, c.vi(c.cacheRes, 0), c.cacheSideVertCount);
                break;
            default:
                break;
        }
    }
}

// PQS.cs:2493
void PQS::BackupEdgeNormals(std::vector<Vector3f>& edge, int vCount, int localStart, int localDelta) {
    int num = localStart;
    for (int i = 0; i < vCount; i++) {
        edge[Z(i)] = buildQuad_->vertNormals[Z(num)];
        num += localDelta;
    }
}

// PQS.cs:2503 UpdateEdgeNormals: average the edge normals with the neighbours' backed-up edge normals (same-level
// neighbour, subdivided neighbour = two half edges, coarser neighbour = every other vertex), then the four corners
// from up to four quads (GetRightmostCornerNormal for the ones not covered by a coarser neighbour)
void PQS::UpdateEdgeNormals(PQ& q) {
    if (!q.isActive || (q.parent != nullptr && !q.parent->isSubdivided) || q.parent == nullptr || !q.isBuilt) return;
    const PQSMeshCache& c = meshCache();
    const int cacheRes = c.cacheRes;
    const int cacheResDiv2 = c.cacheResDiv2;
    const int cacheResDiv2Plus1 = c.cacheResDiv2Plus1;
    const int side = c.cacheSideVertCount;
    auto vi = [&](int x, int z) { return c.vi(x, z); };
    auto EN = [](PQ* p, PQS::QuadEdge e) -> std::vector<Vector3f>& { return p->edgeNormals.at(Z(static_cast<int>(e))); };
    const bool flag = !q.isQueuedOnlyForCornerNormalUpdate;
    Vector3f vector = q.edgeNormals[0][0];
    Vector3f vector2 = q.edgeNormals[0][Z(cacheRes)];
    Vector3f vector3 = q.edgeNormals[1][0];
    Vector3f vector4 = q.edgeNormals[1][Z(cacheRes)];
    Vector3f zero = Vector3f::zero();
    bool flag2 = false;
    bool flag3 = false;
    bool flag4 = false;
    bool flag5 = false;
    PQ* left = nullptr;
    PQ* right = nullptr;
    QuadEdge quadEdge = QuadEdge::Null;
    QuadEdge quadEdge2 = QuadEdge::Null;
    PQ* n = &Deref(q.north);
    if (n->subdivision == q.subdivision) {
        if (n->isSubdivided) {
            n->GetEdgeQuads(&q, left, right);
            if (left != nullptr && right != nullptr) {
                quadEdge = left->GetEdge(&q);
                quadEdge2 = right->GetEdge(&q);
                if (quadEdge == QuadEdge::Null || quadEdge2 == QuadEdge::Null) return;
                if (!left->isBuilt) left->Build();
                if (!right->isBuilt) right->Build();
                zero = q.edgeNormals[0][Z(cacheResDiv2)];
                if (right->isBuilt) {
                    if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(0, 0), 1, q.edgeNormals[0], 0, 1, EN(right, quadEdge2), cacheRes, -2);
                    vector += EN(right, quadEdge2)[Z(cacheRes)];
                    zero += EN(right, quadEdge2)[0];
                }
                if (left->isBuilt) {
                    if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(cacheResDiv2, 0), 1, q.edgeNormals[0], cacheResDiv2, 1, EN(left, quadEdge), cacheRes, -2);
                    vector2 += EN(left, quadEdge)[0];
                    zero += EN(left, quadEdge)[Z(cacheRes)];
                }
                q.vertNormals[Z(vi(cacheResDiv2, 0))] = zero.normalized();
            }
        } else {
            quadEdge = n->GetEdge(&q);
            if (quadEdge == QuadEdge::Null) return;
            if (!n->isBuilt) n->Build();
            if (n->isBuilt) {
                if (flag) CombineEdgeNormals(q, side, vi(0, 0), 1, q.edgeNormals[0], 0, 1, EN(n, quadEdge), cacheRes, -1);
                vector += EN(n, quadEdge)[Z(cacheRes)];
                vector2 += EN(n, quadEdge)[0];
            }
        }
    } else if (n->subdivision < q.subdivision) {
        q.parent->GetEdgeQuads(n, left, right);
        quadEdge = n->GetEdge(q.parent);
        if (quadEdge == QuadEdge::Null) return;
        if (!n->isBuilt) n->Build();
        if (n->isBuilt) {
            if (left == &q) {
                if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(0, 0), 2, q.edgeNormals[0], 0, 2, EN(n, quadEdge), cacheRes, -1);
                vector += EN(n, quadEdge)[Z(cacheRes)];
                vector2 += EN(n, quadEdge)[Z(cacheResDiv2)];
                flag3 = true;
            } else if (right == &q) {
                if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(0, 0), 2, q.edgeNormals[0], 0, 2, EN(n, quadEdge), cacheResDiv2, -1);
                vector += EN(n, quadEdge)[Z(cacheResDiv2)];
                vector2 += EN(n, quadEdge)[0];
                flag2 = true;
            }
        }
    }
    PQ* s = &Deref(q.south);
    if (s->subdivision == q.subdivision) {
        if (s->isSubdivided) {
            s->GetEdgeQuads(&q, left, right);
            if (left != nullptr && right != nullptr) {
                quadEdge = left->GetEdge(&q);
                quadEdge2 = right->GetEdge(&q);
                if (quadEdge == QuadEdge::Null || quadEdge2 == QuadEdge::Null) return;
                if (!left->isBuilt) left->Build();
                if (!right->isBuilt) right->Build();
                zero = q.edgeNormals[1][Z(cacheResDiv2)];
                if (right->isBuilt) {
                    if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(cacheRes, cacheRes), -1, q.edgeNormals[1], 0, 1, EN(right, quadEdge2), cacheRes, -2);
                    vector3 += EN(right, quadEdge2)[Z(cacheRes)];
                    zero += EN(right, quadEdge2)[0];
                }
                if (left->isBuilt) {
                    if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(cacheResDiv2, cacheRes), -1, q.edgeNormals[1], cacheResDiv2, 1, EN(left, quadEdge), cacheRes, -2);
                    vector4 += EN(left, quadEdge)[0];
                    zero += EN(left, quadEdge)[Z(cacheRes)];
                }
                q.vertNormals[Z(vi(cacheResDiv2, cacheRes))] = zero.normalized();
            }
        } else {
            quadEdge = s->GetEdge(&q);
            if (quadEdge == QuadEdge::Null) return;
            if (!s->isBuilt) s->Build();
            if (s->isBuilt) {
                if (flag) CombineEdgeNormals(q, side, vi(cacheRes, cacheRes), -1, q.edgeNormals[1], 0, 1, EN(s, quadEdge), cacheRes, -1);
                vector3 += EN(s, quadEdge)[Z(cacheRes)];
                vector4 += EN(s, quadEdge)[0];
            }
        }
    } else if (s->subdivision < q.subdivision) {
        q.parent->GetEdgeQuads(s, left, right);
        quadEdge = s->GetEdge(q.parent);
        if (quadEdge == QuadEdge::Null) return;
        if (!s->isBuilt) s->Build();
        if (s->isBuilt) {
            if (left == &q) {
                if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(cacheRes, cacheRes), -2, q.edgeNormals[1], 0, 2, EN(s, quadEdge), cacheRes, -1);
                vector3 += EN(s, quadEdge)[Z(cacheRes)];
                vector4 += EN(s, quadEdge)[Z(cacheResDiv2)];
                flag5 = true;
            } else if (right == &q) {
                if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(cacheRes, cacheRes), -2, q.edgeNormals[1], 0, 2, EN(s, quadEdge), cacheResDiv2, -1);
                vector3 += EN(s, quadEdge)[Z(cacheResDiv2)];
                vector4 += EN(s, quadEdge)[0];
                flag4 = true;
            }
        }
    }
    PQ* e = &Deref(q.east);
    if (e->subdivision == q.subdivision) {
        if (e->isSubdivided) {
            e->GetEdgeQuads(&q, left, right);
            if (left != nullptr && right != nullptr) {
                quadEdge = left->GetEdge(&q);
                quadEdge2 = right->GetEdge(&q);
                if (quadEdge == QuadEdge::Null || quadEdge2 == QuadEdge::Null) return;
                if (!left->isBuilt) left->Build();
                if (!right->isBuilt) right->Build();
                zero = q.edgeNormals[2][Z(cacheResDiv2)];
                if (right->isBuilt) {
                    if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(0, cacheRes), -side, q.edgeNormals[2], 0, 1, EN(right, quadEdge2), cacheRes, -2);
                    vector4 += EN(right, quadEdge2)[Z(cacheRes)];
                    zero += EN(right, quadEdge2)[0];
                }
                if (left->isBuilt) {
                    if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(0, cacheResDiv2), -side, q.edgeNormals[2], cacheResDiv2, 1, EN(left, quadEdge), cacheRes, -2);
                    vector += EN(left, quadEdge)[0];
                    zero += EN(left, quadEdge)[Z(cacheRes)];
                }
                q.vertNormals[Z(vi(0, cacheResDiv2))] = zero.normalized();
            }
        } else {
            quadEdge = e->GetEdge(&q);
            if (quadEdge == QuadEdge::Null) return;
            if (!e->isBuilt) e->Build();
            if (e->isBuilt) {
                if (flag) CombineEdgeNormals(q, side, vi(0, cacheRes), -side, q.edgeNormals[2], 0, 1, EN(e, quadEdge), cacheRes, -1);
                vector += EN(e, quadEdge)[0];
                vector4 += EN(e, quadEdge)[Z(cacheRes)];
            }
        }
    } else if (e->subdivision < q.subdivision) {
        q.parent->GetEdgeQuads(e, left, right);
        quadEdge = e->GetEdge(q.parent);
        if (quadEdge == QuadEdge::Null) return;
        if (!e->isBuilt) e->Build();
        if (e->isBuilt) {
            if (right == &q) {
                if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(0, cacheRes), -(side * 2), q.edgeNormals[2], 0, 2, EN(e, quadEdge), cacheResDiv2, -1);
                vector += EN(e, quadEdge)[0];
                vector4 += EN(e, quadEdge)[Z(cacheResDiv2)];
                flag5 = true;
            } else if (left == &q) {
                if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(0, cacheRes), -(side * 2), q.edgeNormals[2], 0, 2, EN(e, quadEdge), cacheRes, -1);
                vector += EN(e, quadEdge)[Z(cacheResDiv2)];
                vector4 += EN(e, quadEdge)[Z(cacheRes)];
                flag2 = true;
            }
        }
    }
    PQ* w = &Deref(q.west);
    if (w->subdivision == q.subdivision) {
        if (w->isSubdivided) {
            w->GetEdgeQuads(&q, left, right);
            if (left != nullptr && right != nullptr) {
                quadEdge = left->GetEdge(&q);
                quadEdge2 = right->GetEdge(&q);
                if (quadEdge == QuadEdge::Null || quadEdge2 == QuadEdge::Null) return;
                if (!left->isBuilt) left->Build();
                if (!right->isBuilt) right->Build();
                zero = q.edgeNormals[3][Z(cacheResDiv2)];
                if (right->isBuilt) {
                    if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(cacheRes, 0), side, q.edgeNormals[3], 0, 1, EN(right, quadEdge2), cacheRes, -2);
                    vector2 += EN(right, quadEdge2)[Z(cacheRes)];
                    zero += EN(right, quadEdge2)[0];
                }
                if (left->isBuilt) {
                    if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(cacheRes, cacheResDiv2), side, q.edgeNormals[3], cacheResDiv2, 1, EN(left, quadEdge), cacheRes, -2);
                    vector3 += EN(left, quadEdge)[0];
                    zero += EN(left, quadEdge)[Z(cacheRes)];
                }
                q.vertNormals[Z(vi(cacheRes, cacheResDiv2))] = zero.normalized();
            }
        } else {
            quadEdge = w->GetEdge(&q);
            if (quadEdge == QuadEdge::Null) return;
            if (!w->isBuilt) w->Build();
            if (w->isBuilt) {
                if (flag) CombineEdgeNormals(q, side, vi(cacheRes, 0), side, q.edgeNormals[3], 0, 1, EN(w, quadEdge), cacheRes, -1);
                vector2 += EN(w, quadEdge)[Z(cacheRes)];
                vector3 += EN(w, quadEdge)[0];
            }
        }
    } else if (w->subdivision < q.subdivision) {
        // (sic: no Null-edge test on this branch, unlike the other three)
        q.parent->GetEdgeQuads(w, left, right);
        quadEdge = w->GetEdge(q.parent);
        if (!w->isBuilt) w->Build();
        if (w->isBuilt) {
            if (left == &q) {
                if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(cacheRes, 0), side * 2, q.edgeNormals[3], 0, 2, EN(w, quadEdge), cacheRes, -1);
                vector2 += EN(w, quadEdge)[Z(cacheRes)];
                vector3 += EN(w, quadEdge)[Z(cacheResDiv2)];
                flag4 = true;
            } else if (right == &q) {
                if (flag) CombineEdgeNormals(q, cacheResDiv2Plus1, vi(cacheRes, 0), side * 2, q.edgeNormals[3], 0, 2, EN(w, quadEdge), cacheResDiv2, -1);
                vector2 += EN(w, quadEdge)[Z(cacheResDiv2)];
                vector3 += EN(w, quadEdge)[0];
                flag3 = true;
            }
        }
    }
    if (!flag2) vector += GetRightmostCornerNormal(&q, q.north);
    if (!flag3) vector2 += GetRightmostCornerNormal(&q, q.west);
    if (!flag4) vector3 += GetRightmostCornerNormal(&q, q.south);
    if (!flag5) vector4 += GetRightmostCornerNormal(&q, q.east);
    q.vertNormals[Z(vi(0, 0))] = vector.normalized();
    q.vertNormals[Z(vi(cacheRes, 0))] = vector2.normalized();
    q.vertNormals[Z(vi(cacheRes, cacheRes))] = vector3.normalized();
    q.vertNormals[Z(vi(0, cacheRes))] = vector4.normalized();
    q.mesh.normals = q.vertNormals;
    Mod_OnQuadUpdateNormals(q);
    if (reqBuildTangents) BuildTangents(q);
}

// PQS.cs:2951 CombineEdgeNormals: (local + remote) * 0.5f (not renormalised)
void PQS::CombineEdgeNormals(PQ& q, int vCount, int vertStart, int vertStep, const std::vector<Vector3f>& bkpLocal,
                             int localBackupStart, int localBackupStep, const std::vector<Vector3f>& bkpRemote,
                             int remoteBackupStart, int remoteBackupStep) {
    int num = 0;
    int num2 = vertStart;
    int num3 = localBackupStart;
    int num4 = remoteBackupStart;
    while (num < vCount) {
        q.vertNormals.at(Z(num2)) = (bkpLocal.at(Z(num3)) + bkpRemote.at(Z(num4))) * 0.5f;
        num++;
        num2 += vertStep;
        num3 += localBackupStep;
        num4 += remoteBackupStep;
    }
}

// PQS.cs:2967 GetRightmostCornerNormal
Vector3f PQS::GetRightmostCornerNormal(PQ* caller, PQ* nextQuad) {
    if (caller != nullptr && nextQuad != nullptr) {
        PQ* rightmostCornerPQ = caller->GetRightmostCornerPQ(nextQuad);
        if (rightmostCornerPQ == nullptr) {
            // (Debug.LogFormat "[PQS] cornerQuad in GetRightmostCornerNormal is null! ..." unless pending collapse)
            return Vector3f::zero();
        }
        QuadEdge quadEdge = QuadEdge::Null;
        if (nextQuad->subdivision < rightmostCornerPQ->subdivision) {
            quadEdge = Deref(rightmostCornerPQ->parent).GetEdge(nextQuad);
        } else if (nextQuad->subdivision <= rightmostCornerPQ->subdivision) {
            quadEdge = rightmostCornerPQ->GetEdge(nextQuad);
        } else {
            quadEdge = rightmostCornerPQ->GetEdge(nextQuad->parent);
        }
        if (!rightmostCornerPQ->isBuilt) rightmostCornerPQ->Build();
        if (quadEdge == QuadEdge::Null) {
            // (Debug.LogFormat "[PQS] Last edge accessor in GetRightmostCornerNormal is null! ...")
            return Vector3f::zero();
        }
        const int cacheRes = meshCache().cacheRes;
        const auto& edge = rightmostCornerPQ->edgeNormals;
        if (static_cast<int>(edge.size()) >= static_cast<int>(quadEdge) &&
            static_cast<int>(edge[Z(static_cast<int>(quadEdge))].size()) >= cacheRes && cacheRes >= 0)
            return edge[Z(static_cast<int>(quadEdge))].at(Z(cacheRes));
        // (Debug.LogFormat "[PQS] Unable to Calculate CornerQuad Normal ...")
        return Vector3f::zero();
    }
    // (Debug.LogFormat "[PQS] Null parameter in GetRightmostCornerNormal! ...")
    return Vector3f::zero();
}

// PQS.cs:3008
PQS::QuadEdge PQS::GetEdgeRotatedClockwise(QuadEdge edge) {
    switch (edge) {
        case QuadEdge::North: return QuadEdge::East;
        case QuadEdge::East: return QuadEdge::South;
        case QuadEdge::South: return QuadEdge::West;
        case QuadEdge::West: return QuadEdge::North;
        default: return QuadEdge::Null;
    }
}

// PQS.cs:3025
PQS::QuadEdge PQS::GetEdgeRotatedCounterclockwise(QuadEdge edge) {
    switch (edge) {
        case QuadEdge::North: return QuadEdge::West;
        case QuadEdge::West: return QuadEdge::South;
        case QuadEdge::South: return QuadEdge::East;
        case QuadEdge::East: return QuadEdge::North;
        default: return QuadEdge::Null;
    }
}

}  // namespace ksp
