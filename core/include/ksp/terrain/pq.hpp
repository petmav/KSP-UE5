// WP-12a: PQ, one quad of the PQS quadtree (Assembly-CSharp/PQ.cs), without GameObject / MeshRenderer / MeshCollider.
//
// A PQ is a pooled object (PQSCache.GetQuad / DestroyQuad, ksp/terrain/pqs_cache.hpp): its arrays (verts,
// vertNormals, edgeNormals) and its PQMesh survive being returned to the pool and reused, exactly like the C#
// MonoBehaviour and its UnityEngine.Mesh, so whatever KSP reads from a reused quad before rebuilding it is reproduced.
//
// PQMesh holds what BuildQuad / UpdateEdgeNormals assign to the quad's UnityEngine.Mesh (Unity copies arrays on
// assignment; so does PQMesh). This is the render handoff: for every quad with `rendererEnabled`, upload
//   positions (float, quad-local: add `meshOrigin`, a double in the sphere's body-fixed frame), normals, tangents,
//   uv / uv2 / uv3 / uv4 (empty = channel never assigned), colors, and the triangle list
//   PQSMeshCache::cacheIndices[mesh.triangleSet] (one of the 17 edge-lerp index sets).
// Vertex positions (see PQS::BuildVertexSurfaceRelative): with surfaceRelativeQuads the mesh vertices are
// (Vector3)vertRel - (Vector3)positionPlanet and meshOrigin = (Vector3)positionPlanet widened to double, i.e. KSP's own
// float quantisation of the planet-relative vertex with an identity sphere transform (KSP additionally rounds through
// the floating-origin world transform: dropped, "changed"); without it the vertices are (Vector3)vertRel and
// meshOrigin = 0. `vertsD` keeps the exact double planet-relative vertices (PQS.verts, which KSP keeps only in the
// static build scratch) for colliders and high-precision consumers.
#pragma once

#include <array>
#include <functional>
#include <string>
#include <vector>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/bounds.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// The UnityEngine.Mesh of a quad (the channels the PQS writes)
struct PQMesh {
    std::vector<Vector3f> vertices;
    std::vector<Vector3f> normals;
    std::vector<Vector4f> tangents;
    std::vector<Vector2f> uv, uv2, uv3, uv4;
    std::vector<Color> colors;
    int triangleSet = 0;  // mesh.triangles = PQS.cacheIndices[triangleSet]
    Bounds bounds;        // RecalculateBounds (native AABB of `vertices`; center = (min+max)*0.5, extents = (max-min)*0.5)
    void RecalculateBounds();
};

class PQ {
public:
    using QuadChild = PQS::QuadChild;
    using QuadDelegate = std::function<void(PQ&)>;

    PQ();  // Awake (PQ.cs:169) state

    // ---- PQ.cs fields (:37-167) ----
    int id = 0;
    int subdivision = 0;
    PQS* sphereRoot = nullptr;
    PQ* quadRoot = nullptr;
    PQ* parent = nullptr;
    PQ* north = nullptr;
    PQ* south = nullptr;
    PQ* east = nullptr;
    PQ* west = nullptr;
    std::array<PQ*, 4> subNodes{};
    bool isActive = false;
    bool isSubdivided = false;
    bool isVisible = false;
    bool isForcedInvisible = false;
    bool isBuilt = false;
    bool isQueuedForNormalUpdate = false;
    bool isQueuedOnlyForCornerNormalUpdate = false;
    bool isCached = false;
    bool isPendingCollapse = false;
    PQMesh mesh;
    std::vector<Vector3f> verts;
    std::vector<Vector3f> vertNormals;
    std::array<std::vector<Vector3f>, 4> edgeNormals;
    Matrix4x4f quadMatrix;
    QuaternionD planeRotation;
    PQS::QuadPlane plane = PQS::QuadPlane::XP;
    Vector3d positionPlanetRelative;
    Vector3d positionPlanet;
    Vector3d positionPlanePosition;
    Vector3d positionParentRelative;
    double scalePlanetRelative = 0.0;
    double scalePlaneRelative = 0.0;
    QuaternionD quadRotation;
    double quadScaleFactor = 0.0;
    Vector3d quadScale;
    double quadArea = 0.0;
    double meshVertMin = 0.0;
    double meshVertMax = 0.0;
    double subdivideThresholdFactor = 0.0;
    double gcDist = 0.0;
    double gcd1 = 0.0;
    double gcd2 = 0.0;
    PQS::EdgeState edgeState = PQS::EdgeState::Reset;
    Vector2f uvSW;
    Vector2f uvDelta;
    QuadDelegate onUpdate, onDestroy, onVisible, onInvisible;
    int quadIndex = 0;
    PQ* CreateParent = nullptr;
    std::string name;  // gameObject.name
    // meshRenderer.enabled: the quad is drawn
    bool rendererEnabled = false;
    // meshCollider (PQSMod_QuadMeshColliders): created once, then enabled / disabled; the collider mesh is the quad mesh
    // as of the enabling OnQuadBuilt (triangles = cacheIndices[0]; Unity bakes a MeshCollider at sharedMesh assignment)
    bool meshColliderCreated = false;
    bool meshColliderEnabled = false;
    // the quad transform's local position (see the header comment) and the exact double vertices
    Vector3d meshOrigin;
    std::vector<Vector3d> vertsD;

    // ---- PQ.cs methods ----
    void SetupQuad(PQ* parentQuad, QuadChild parentChildPosition);  // :178
    void UpdateTargetRelativity();     // :280
    void UpdateVisibility();           // :286 (private in C#)
    void UpdateSubdivision();          // :305
    void UpdateSubdivisionInit();      // :353
    void Build();                      // :381
    bool Subdivide();                  // :402
    bool Collapse();                   // :590
    void ClearAndDestroy();            // :682
    void Destroy();                    // :726
    void SetCache();                   // :754
    void ClearCache();                 // :766
    void SetVisible();                 // :778
    void SetInvisible();               // :802
    void SetMasterVisible();           // :815
    void SetMasterInvisible();         // :830
    void QueueForNormalUpdate();       // :846
    void QueueForCornerNormalUpdate(); // :855
    void UnqueueForNormalUpdate();     // :865
    PQS::EdgeState GetEdgeState() const;  // :875
    void SetEdgeState();               // :897
    void GetEdgeQuads(const PQ* caller, PQ*& left, PQ*& right) const;  // :918
    PQS::QuadEdge GetEdge(const PQ* caller) const;  // :964
    PQ* GetRightmostCornerPQ(PQ* nextQuad);  // :985
    PQ* GetSidePQ(PQS::QuadEdge edge) const;  // :1020
    void SetNeighbour(PQ* oldNeighbour, PQ* newNeighbour);  // :1035

    double angularinterval() const { return angularinterval_; }
    int corner() const { return Corner_; }

private:
    friend class PQS;
    double angularinterval_ = 0.0;
    int Corner_ = 0;
    bool outOfTime_ = false;
};

}  // namespace ksp
