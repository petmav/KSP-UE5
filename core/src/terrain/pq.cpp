// WP-12a: port of Assembly-CSharp/PQ.cs (see include/ksp/terrain/pq.hpp). Every method cites its C# line.
// Unity-only statements are dropped and noted: Debug.Log / Debug.Break, the LocalSpacePQStorage re-parenting and
// PrecisePosition bookkeeping of max-level quads (floating origin: the core has none), FastUpdateSubQuadsPosition /
// PreciseUpdateSubQuadsPosition (PQ.cs:260 / :266, floating-origin moves).
#include "ksp/terrain/pq.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/unity/matrix_native.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp {

namespace {
// C# member access through a null reference
template <class T>
T& Deref(T* p) {
    if (p == nullptr) throw NullReferenceException();
    return *p;
}
bool Contains(const std::vector<PQ*>& list, const PQ* q) { return std::find(list.begin(), list.end(), q) != list.end(); }
}  // namespace

// Mesh.RecalculateBounds (native): AABB of the vertices, center = (max + min) * 0.5, extents = (max - min) * 0.5
// (per-operation float). UNVERIFIED against the game (no capture of Mesh.bounds); not part of the vertex data.
void PQMesh::RecalculateBounds() {
    if (vertices.empty()) {
        bounds = Bounds();
        return;
    }
    Vector3f mn = vertices[0], mx = vertices[0];
    for (const Vector3f& v : vertices) {
        mn.x = v.x < mn.x ? v.x : mn.x;
        mn.y = v.y < mn.y ? v.y : mn.y;
        mn.z = v.z < mn.z ? v.z : mn.z;
        mx.x = v.x > mx.x ? v.x : mx.x;
        mx.y = v.y > mx.y ? v.y : mx.y;
        mx.z = v.z > mx.z ? v.z : mx.z;
    }
    bounds.m_Center = Vector3f((mx.x + mn.x) * 0.5f, (mx.y + mn.y) * 0.5f, (mx.z + mn.z) * 0.5f);
    bounds.m_Extents = Vector3f((mx.x - mn.x) * 0.5f, (mx.y - mn.y) * 0.5f, (mx.z - mn.z) * 0.5f);
}

// PQ.cs:169 Awake
PQ::PQ() {
    subNodes = {nullptr, nullptr, nullptr, nullptr};
    isBuilt = false;
    isVisible = false;
    edgeState = PQS::EdgeState::Reset;
}

// PQ.cs:178 SetupQuad
void PQ::SetupQuad(PQ* parentQuad, QuadChild parentChildPosition) {
    // (north/south/east/west null: Debug.LogError + Debug.Break, then carries on)
    PQS& s = Deref(sphereRoot);
    angularinterval_ = s.circumference * dotnet::Math::Atan(scalePlaneRelative / s.radius);
    quadRotation = QuaternionD(UnityNative::FromToRotation(Vector3f::up(), positionPlanetRelative.ToVector3f()));
    if (quadRoot != nullptr) {
        quadScaleFactor = 1.0 / dotnet::Math::Pow(2.0, subdivision - 1);
        quadArea = quadRoot->quadArea * quadScaleFactor;
        quadScale = Vector3d::one() * quadScaleFactor;
        quadMatrix = UnityNative::TRS(positionPlanePosition.ToVector3f(), quadRoot->planeRotation.ToQuaternionf(), quadScale.ToVector3f());
    } else {
        planeRotation = QuaternionD(UnityNative::FromToRotation(Vector3f::up(), positionPlanePosition.normalized().ToVector3f()));
        quadScaleFactor = 2.0;
        quadArea = dotnet::Math::Pow(s.radius * 2.0, 2.0);
        quadScale = Vector3d::one() * quadScaleFactor;
        quadMatrix = UnityNative::TRS(positionPlanePosition.ToVector3f(), planeRotation.ToQuaternionf(), quadScale.ToVector3f());
    }
    // quadTransform.localPosition = positionPlanet (surfaceRelativeQuads) or Vector3.zero: a float Vector3
    meshOrigin = s.surfaceRelativeQuads ? Vector3d(positionPlanet.ToVector3f()) : Vector3d::zero();
    if (parentQuad == nullptr) {
        quadIndex = 0;
    } else if (subdivision == 1) {
        quadIndex = parentQuad->quadIndex + static_cast<int>(parentChildPosition);
    } else {
        quadIndex = parentQuad->quadIndex + (static_cast<int>(parentChildPosition) << ((2 * (subdivision - 1)) & 31));
    }
    UpdateTargetRelativity();
}

// PQ.cs:280 UpdateTargetRelativity
void PQ::UpdateTargetRelativity() {
    const PQS& s = Deref(sphereRoot);
    gcd1 = dotnet::Math::Acos(Vector3d::Dot(positionPlanetRelative, s.relativeTargetPositionNormalized)) * s.radius * 1.3;
    gcDist = gcd1 + dotnet::Math::Abs(s.targetHeight) - angularinterval_;
}

// PQ.cs:286 UpdateVisibility
void PQ::UpdateVisibility() {
    if (!isSubdivided && gcd1 < Deref(sphereRoot).visibleRadius) {
        SetVisible();
        const PQS::EdgeState newEdgeState = GetEdgeState();
        if (newEdgeState != edgeState) {
            mesh.triangleSet = static_cast<int>(newEdgeState);  // mesh.triangles = PQS.cacheIndices[(int)newEdgeState]
            edgeState = newEdgeState;
            QueueForNormalUpdate();
        }
    } else {
        SetInvisible();
    }
}

// PQ.cs:305 UpdateSubdivision
void PQ::UpdateSubdivision() {
    UpdateTargetRelativity();
    PQS& s = Deref(sphereRoot);
    outOfTime_ = s.Now() > s.maxFrameEnd;
    isPendingCollapse = false;
    if (isSubdivided) {
        rendererEnabled = false;
        const bool flag = gcDist > s.collapseThresholds.at(static_cast<size_t>(subdivision)) * subdivideThresholdFactor;
        if (subdivision <= s.maxLevel && (!flag || outOfTime_)) {
            if (flag) isPendingCollapse = true;
            for (int i = 0; i < 4; i++)
                if (subNodes[static_cast<size_t>(i)] != nullptr) subNodes[static_cast<size_t>(i)]->UpdateSubdivision();
        } else if (!Collapse()) {
            for (int j = 0; j < 4; j++)
                if (subNodes[static_cast<size_t>(j)] != nullptr) subNodes[static_cast<size_t>(j)]->UpdateSubdivision();
        }
    } else if (subdivision >= s.minLevel &&
               (!(gcDist < s.subdivisionThresholds.at(static_cast<size_t>(subdivision)) * subdivideThresholdFactor) ||
                subdivision >= s.maxLevelAtCurrentTgtSpeed || outOfTime_)) {
        UpdateVisibility();
    } else {
        Subdivide();
    }
    if (onUpdate) onUpdate(*this);
}

// PQ.cs:353 UpdateSubdivisionInit
void PQ::UpdateSubdivisionInit() {
    UpdateTargetRelativity();
    PQS& s = Deref(sphereRoot);
    if (isSubdivided) {
        if (subdivision > s.minLevel &&
            (subdivision > s.maxLevel ||
             gcDist > s.collapseThresholds.at(static_cast<size_t>(subdivision)) * subdivideThresholdFactor)) {
            Collapse();
            return;
        }
        for (int i = 0; i < 4; i++)
            if (subNodes[static_cast<size_t>(i)] != nullptr) subNodes[static_cast<size_t>(i)]->UpdateSubdivisionInit();
    } else if (gcDist < s.subdivisionThresholds.at(static_cast<size_t>(subdivision)) * subdivideThresholdFactor &&
               subdivision < s.maxLevel) {
        Subdivide();
    } else if (onUpdate) {
        onUpdate(*this);
    }
}

// PQ.cs:381 Build
void PQ::Build() {
    if (isBuilt || !isActive || !Deref(sphereRoot).quadAllowBuild || isCached) return;
    if (isSubdivided) {
        for (int i = 0; i < 4; i++) Deref(subNodes[static_cast<size_t>(i)]).Build();
        return;
    }
    isBuilt = sphereRoot->BuildQuad(*this);
    if (isBuilt) QueueForNormalUpdate();
}

// PQ.cs:402 Subdivide
bool PQ::Subdivide() {
    if (Deref(north).subdivision < subdivision) return false;
    if (Deref(east).subdivision < subdivision) return false;
    if (Deref(south).subdivision < subdivision) return false;
    if (Deref(west).subdivision < subdivision) return false;
    if (isSubdivided) return true;
    if (!isActive) return false;
    PQS& s = Deref(sphereRoot);
    for (int i = 0; i < 4; i++) {
        if (subNodes[static_cast<size_t>(i)] != nullptr) {
            subNodes[static_cast<size_t>(i)]->isActive = true;  // + Debug.Log(name) / Debug.Break()
            continue;
        }
        PQ* pQ = s.AssignQuad(subdivision + 1);
        const int num = i % 2;
        const int num2 = i / 2;
        pQ->scalePlaneRelative = scalePlaneRelative * 0.5;
        pQ->scalePlanetRelative = s.radius * pQ->scalePlaneRelative;
        if (quadRoot == nullptr) {
            pQ->quadRoot = this;
        } else {
            pQ->quadRoot = quadRoot;
        }
        pQ->CreateParent = this;
        pQ->positionParentRelative = pQ->quadRoot->planeRotation * Vector3d((static_cast<double>(num) - 0.5) * scalePlaneRelative, 0.0,
                                                                            (static_cast<double>(num2) - 0.5) * scalePlaneRelative);
        pQ->positionPlanePosition = positionPlanePosition + pQ->positionParentRelative;
        pQ->positionPlanetRelative = pQ->positionPlanePosition.normalized();
        pQ->positionPlanet = pQ->positionPlanetRelative * s.GetSurfaceHeight(pQ->positionPlanetRelative);
        pQ->plane = plane;
        pQ->sphereRoot = sphereRoot;
        pQ->subdivision = subdivision + 1;
        pQ->parent = this;
        pQ->Corner_ = i;
        pQ->name = name + std::to_string(i);
        s.QuadCreated(*pQ);
        subNodes[static_cast<size_t>(i)] = pQ;
    }
    PQ* n0 = subNodes[0];
    PQ* n1 = subNodes[1];
    PQ* n2 = subNodes[2];
    PQ* n3 = subNodes[3];
    n0->north = n2;
    n0->east = n1;
    n1->north = n3;
    n1->west = n0;
    n2->south = n0;
    n2->east = n3;
    n3->south = n1;
    n3->west = n2;
    PQ* left = nullptr;
    PQ* right = nullptr;
    if (north->subdivision == subdivision && north->isSubdivided) {
        north->GetEdgeQuads(this, left, right);
        n2->north = left;
        n3->north = right;
        Deref(left).SetNeighbour(this, n2);
        Deref(right).SetNeighbour(this, n3);
    } else {
        n2->north = north;
        n3->north = north;
    }
    if (south->subdivision == subdivision && south->isSubdivided) {
        south->GetEdgeQuads(this, left, right);
        n1->south = left;
        n0->south = right;
        Deref(left).SetNeighbour(this, n1);
        Deref(right).SetNeighbour(this, n0);
    } else {
        n1->south = south;
        n0->south = south;
    }
    if (east->subdivision == subdivision && east->isSubdivided) {
        east->GetEdgeQuads(this, left, right);
        n3->east = left;
        n1->east = right;
        Deref(left).SetNeighbour(this, n3);
        Deref(right).SetNeighbour(this, n1);
    } else {
        n3->east = east;
        n1->east = east;
    }
    if (west->subdivision == subdivision && west->isSubdivided) {
        west->GetEdgeQuads(this, left, right);
        n0->west = left;
        n2->west = right;
        Deref(left).SetNeighbour(this, n0);
        Deref(right).SetNeighbour(this, n2);
    } else {
        n0->west = west;
        n2->west = west;
    }
    if (s.reqUVQuad) {
        // PQ.cs:529-543 (static uvDel / uvMidPoint / uvMidS / uvMidW; Vector2 * float and float field arithmetic, Mono rule)
        const Vector2f uvDel = uvDelta * 0.5f;
        Vector2f uvMidPoint;
        uvMidPoint.x = f32store(fx(uvSW.x) + fx(uvDel.x));
        uvMidPoint.y = f32store(fx(uvSW.y) + fx(uvDel.y));
        Vector2f uvMidS(uvMidPoint.x, uvSW.y);
        Vector2f uvMidW(uvSW.x, uvMidPoint.y);
        n0->uvSW = uvMidPoint;
        n0->uvDelta = uvDel;
        n1->uvSW = uvMidW;
        n1->uvDelta = uvDel;
        n2->uvSW = uvMidS;
        n2->uvDelta = uvDel;
        n3->uvSW = uvSW;
        n3->uvDelta = uvDel;
    }
    isSubdivided = true;
    SetInvisible();
    for (int i = 0; i < 4; i++) {
        PQ* c = subNodes[static_cast<size_t>(i)];
        // (a null neighbour: Debug.Log + Debug.Break)
        if (!c->isCached) c->SetupQuad(this, static_cast<QuadChild>(i));
        if (s.quadAllowBuild) c->UpdateVisibility();
    }
    north->QueueForNormalUpdate();
    south->QueueForNormalUpdate();
    east->QueueForNormalUpdate();
    west->QueueForNormalUpdate();
    PQ* rightmostCornerPQ = GetRightmostCornerPQ(north);
    if (rightmostCornerPQ != nullptr) rightmostCornerPQ->QueueForCornerNormalUpdate();
    rightmostCornerPQ = GetRightmostCornerPQ(west);
    if (rightmostCornerPQ != nullptr) rightmostCornerPQ->QueueForCornerNormalUpdate();
    rightmostCornerPQ = GetRightmostCornerPQ(south);
    if (rightmostCornerPQ != nullptr) rightmostCornerPQ->QueueForCornerNormalUpdate();
    rightmostCornerPQ = GetRightmostCornerPQ(east);
    if (rightmostCornerPQ != nullptr) rightmostCornerPQ->QueueForCornerNormalUpdate();
    return true;
}

// PQ.cs:590 Collapse
bool PQ::Collapse() {
    PQS& s = Deref(sphereRoot);
    if (Contains(s.normalUpdateList, this)) UnqueueForNormalUpdate();
    if (!isSubdivided) return false;
    if (Deref(north).subdivision > subdivision) return false;
    if (Deref(south).subdivision > subdivision) return false;
    if (Deref(east).subdivision > subdivision) return false;
    if (Deref(west).subdivision > subdivision) return false;
    for (int i = 0; i < 4; i++) {
        // (subdivision + 1 == maxLevel: sphereRoot.RemovePQFromLocalSpaceStorage(subNodes[i]) -- floating origin, dropped)
        PQ& c = Deref(subNodes[static_cast<size_t>(i)]);
        if (c.isCached) {
            c.SetInvisible();
            c.isActive = false;
            if (Contains(s.normalUpdateList, &c)) c.UnqueueForNormalUpdate();
        } else {
            c.ClearAndDestroy();
            subNodes[static_cast<size_t>(i)] = nullptr;
        }
    }
    if (north == nullptr) north = Deref(parent).north;
    if (south == nullptr) south = Deref(parent).south;
    if (east == nullptr) east = Deref(parent).east;
    if (west == nullptr) west = Deref(parent).west;
    isSubdivided = false;
    UpdateVisibility();
    north->QueueForNormalUpdate();
    south->QueueForNormalUpdate();
    east->QueueForNormalUpdate();
    west->QueueForNormalUpdate();
    PQ* rightmostCornerPQ = GetRightmostCornerPQ(north);
    if (rightmostCornerPQ != nullptr) rightmostCornerPQ->QueueForCornerNormalUpdate();
    rightmostCornerPQ = GetRightmostCornerPQ(west);
    if (rightmostCornerPQ != nullptr) rightmostCornerPQ->QueueForCornerNormalUpdate();
    rightmostCornerPQ = GetRightmostCornerPQ(south);
    if (rightmostCornerPQ != nullptr) rightmostCornerPQ->QueueForCornerNormalUpdate();
    rightmostCornerPQ = GetRightmostCornerPQ(east);
    if (rightmostCornerPQ != nullptr) rightmostCornerPQ->QueueForCornerNormalUpdate();
    return true;
}

// PQ.cs:682 ClearAndDestroy
void PQ::ClearAndDestroy() {
    PQS& s = Deref(sphereRoot);
    if (isSubdivided) {
        for (int i = 0; i < 4; i++) {
            // (subdivision + 1 == maxLevel: RemovePQFromLocalSpaceStorage -- dropped)
            if (subNodes[static_cast<size_t>(i)] != nullptr) {
                subNodes[static_cast<size_t>(i)]->ClearAndDestroy();
                subNodes[static_cast<size_t>(i)] = nullptr;
            }
        }
    }
    if (north != nullptr && north->subdivision == subdivision) north->SetNeighbour(this, parent);
    if (east != nullptr && east->subdivision == subdivision) east->SetNeighbour(this, parent);
    if (south != nullptr && south->subdivision == subdivision) south->SetNeighbour(this, parent);
    if (west != nullptr && west->subdivision == subdivision) west->SetNeighbour(this, parent);
    if (Contains(s.normalUpdateList, this)) UnqueueForNormalUpdate();
    if (onDestroy) onDestroy(*this);
    s.DestroyQuad(*this);
}

// PQ.cs:726 Destroy
void PQ::Destroy() {
    PQS& s = Deref(sphereRoot);
    if (isSubdivided) {
        for (int i = 0; i < 4; i++) {
            if (subNodes[static_cast<size_t>(i)] != nullptr) {
                subNodes[static_cast<size_t>(i)]->Destroy();
                subNodes[static_cast<size_t>(i)] = nullptr;
            }
        }
    }
    if (Contains(s.normalUpdateList, this)) UnqueueForNormalUpdate();
    if (onDestroy) onDestroy(*this);
    s.DestroyQuad(*this);
}

// PQ.cs:754 SetCache
void PQ::SetCache() {
    isCached = true;
    if (isSubdivided)
        for (int i = 0; i < 4; i++) Deref(subNodes[static_cast<size_t>(i)]).SetCache();
}

// PQ.cs:766 ClearCache
void PQ::ClearCache() {
    isCached = false;
    if (isSubdivided)
        for (int i = 0; i < 4; i++) Deref(subNodes[static_cast<size_t>(i)]).ClearCache();
}

// PQ.cs:778 SetVisible
void PQ::SetVisible() {
    if (!isVisible) {
        isVisible = true;
        if (!isBuilt) Build();
        rendererEnabled = !isForcedInvisible;
        if (onVisible) onVisible(*this);
    }
}

// PQ.cs:802 SetInvisible
void PQ::SetInvisible() {
    if (isVisible) {
        isVisible = false;
        rendererEnabled = false;
        if (onInvisible) onInvisible(*this);
    }
}

// PQ.cs:815 SetMasterVisible
void PQ::SetMasterVisible() {
    if (isVisible && !isForcedInvisible) rendererEnabled = true;
    if (isSubdivided)
        for (int num = 3; num >= 0; num--) Deref(subNodes[static_cast<size_t>(num)]).SetMasterVisible();
}

// PQ.cs:830 SetMasterInvisible
void PQ::SetMasterInvisible() {
    if (isVisible) rendererEnabled = false;
    if (isSubdivided)
        for (int num = 3; num >= 0; num--) Deref(subNodes[static_cast<size_t>(num)]).SetMasterInvisible();
}

// PQ.cs:846 QueueForNormalUpdate
void PQ::QueueForNormalUpdate() {
    if (sphereRoot != nullptr && sphereRoot->reqCustomNormals && !isQueuedForNormalUpdate && sphereRoot->quadAllowBuild) {
        sphereRoot->normalUpdateList.push_back(this);
        isQueuedForNormalUpdate = true;
    }
}

// PQ.cs:855 QueueForCornerNormalUpdate
void PQ::QueueForCornerNormalUpdate() {
    if (sphereRoot != nullptr && sphereRoot->reqCustomNormals && !isQueuedForNormalUpdate && sphereRoot->quadAllowBuild) {
        sphereRoot->normalUpdateList.push_back(this);
        isQueuedForNormalUpdate = true;
        isQueuedOnlyForCornerNormalUpdate = true;
    }
}

// PQ.cs:865 UnqueueForNormalUpdate (List.Remove: the first occurrence)
void PQ::UnqueueForNormalUpdate() {
    if (sphereRoot != nullptr && isQueuedForNormalUpdate) {
        auto& l = sphereRoot->normalUpdateList;
        auto it = std::find(l.begin(), l.end(), this);
        if (it != l.end()) l.erase(it);
        isQueuedForNormalUpdate = false;
        isQueuedOnlyForCornerNormalUpdate = false;
    }
}

// PQ.cs:875 GetEdgeState
PQS::EdgeState PQ::GetEdgeState() const {
    int e = PQS::EdgeState::NoLerps;
    if (Deref(north).subdivision < subdivision) e |= PQS::EdgeState::NorthLerp;
    if (Deref(south).subdivision < subdivision) e |= PQS::EdgeState::SouthLerp;
    if (Deref(east).subdivision < subdivision) e |= PQS::EdgeState::EastLerp;
    if (Deref(west).subdivision < subdivision) e |= PQS::EdgeState::WestLerp;
    return static_cast<PQS::EdgeState>(e);
}

// PQ.cs:897 SetEdgeState
void PQ::SetEdgeState() { edgeState = GetEdgeState(); }

// PQ.cs:918 GetEdgeQuads
void PQ::GetEdgeQuads(const PQ* caller, PQ*& left, PQ*& right) const {
    left = nullptr;
    right = nullptr;
    if (north == caller) {
        left = subNodes[3];
        right = subNodes[2];
    } else if (south == caller) {
        left = subNodes[0];
        right = subNodes[1];
    } else if (east == caller) {
        left = subNodes[1];
        right = subNodes[3];
    } else if (west == caller) {
        left = subNodes[2];
        right = subNodes[0];
    } else {
        const PQ* cp = Deref(caller).parent;
        if (north == cp) {
            left = subNodes[3];
            right = subNodes[2];
        } else if (south == cp) {
            left = subNodes[0];
            right = subNodes[1];
        } else if (east == cp) {
            left = subNodes[1];
            right = subNodes[3];
        } else if (west == cp) {
            left = subNodes[2];
            right = subNodes[0];
        }
    }
}

// PQ.cs:964 GetEdge
PQS::QuadEdge PQ::GetEdge(const PQ* caller) const {
    if (caller == north) return PQS::QuadEdge::North;
    if (caller == south) return PQS::QuadEdge::South;
    if (caller == east) return PQS::QuadEdge::East;
    if (caller == west) return PQS::QuadEdge::West;
    return PQS::QuadEdge::Null;
}

// PQ.cs:985 GetRightmostCornerPQ
PQ* PQ::GetRightmostCornerPQ(PQ* nextQuad) {
    PQ* pQ = this;
    if (Deref(nextQuad).subdivision < subdivision) pQ = pQ->parent;
    const PQS::QuadEdge edge = nextQuad->GetEdge(pQ);
    if (edge == PQS::QuadEdge::Null) {
        // (unless a quad involved is pending collapse: Debug.LogFormat("[PQ] Edge in GetRightmostCornerPQ is null! ..."))
        return nullptr;
    }
    const PQS::QuadEdge edgeRotatedCounterclockwise = PQS::GetEdgeRotatedCounterclockwise(edge);
    PQ* pQ2 = nextQuad->GetSidePQ(edgeRotatedCounterclockwise);
    if (!Deref(pQ2).isBuilt) pQ2->Build();
    if (pQ2->isSubdivided) {
        PQ* l = nullptr;
        PQ* r = nullptr;
        pQ2->GetEdgeQuads(nextQuad, l, r);
        pQ2 = r;
    }
    if (!Deref(pQ2).isBuilt) pQ2->Build();
    return pQ2;
}

// PQ.cs:1020 GetSidePQ
PQ* PQ::GetSidePQ(PQS::QuadEdge edge) const {
    switch (edge) {
        case PQS::QuadEdge::North: return north;
        case PQS::QuadEdge::East: return east;
        case PQS::QuadEdge::South: return south;
        default: return west;
    }
}

// PQ.cs:1035 SetNeighbour
void PQ::SetNeighbour(PQ* oldNeighbour, PQ* newNeighbour) {
    if (newNeighbour == this) return;
    if (oldNeighbour == north) {
        north = newNeighbour;
    } else if (oldNeighbour == south) {
        south = newNeighbour;
    } else if (oldNeighbour == east) {
        east = newNeighbour;
    } else if (oldNeighbour == west) {
        west = newNeighbour;
    } else if (parent != nullptr) {
        if (parent->north == oldNeighbour) {
            north = newNeighbour;
        } else if (parent->south == oldNeighbour) {
            south = newNeighbour;
        } else if (parent->east == oldNeighbour) {
            east = newNeighbour;
        } else if (parent->west == oldNeighbour) {
            west = newNeighbour;
        }
    }
}

}  // namespace ksp
