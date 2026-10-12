"""WP-12a reference, part 2: the PQS statics, PQ, the PQSCache pool (from PQS.cs, PQ.cs, PQSCache.cs). No kspcore code."""
from pqs_ref_math import (Acos, Atan, Pow, V3ZERO, d2f, dadd, ddot, dmul, dnormalized, f32, native_from_to_rotation,
                          native_orthonormalize, native_trs, qd_mul_v, v3add, v3cross, v3dot)

DMAX = 1.7976931348623157e308


# =====================================================================================================================
# PQS statics: the mesh cache (PQS.cs:387-433, CreateCache :1517)
# =====================================================================================================================
class MeshCache:
    def __init__(self, side=15, mesh_size=1.0):
        self.side = side
        self.mesh_size = f32(mesh_size)
        self.created = False

    def create(self):  # PQS.cs:1517
        side = self.side
        self.res = side - 1
        self.resDiv2 = self.res // 2
        self.resDiv2Plus1 = self.resDiv2 + 1
        self.vertCount = side * side
        self.triIndexCount = self.res * self.res * 6
        self.triCount = self.triIndexCount // 3
        n = self.vertCount
        self.cacheVerts = [None] * n
        array = [None] * n
        self.cacheUVs = [None] * n
        self.cacheUVQuad = [None] * n
        self.cacheNormals = [V3ZERO] * n                     # CreateCacheNormals :1729
        self.cacheColors = [(1.0, 1.0, 1.0, 1.0)] * n         # CreateCacheColors :1738 (Color.white)
        self.cacheUV2s = [(0.0, 0.0)] * n                     # CreateCacheUVExtras :1747
        self.cacheUV3s = [(0.0, 0.0)] * n
        self.cacheUV4s = [(0.0, 0.0)] * n
        num = f32(self.mesh_size / f32(float(self.res)))
        num2 = f32(self.mesh_size / f32(float(self.res)))
        num3 = f32(1.0 / f32(float(side)))
        num4 = f32(1.0 / f32(float(side)))
        num5 = f32(self.mesh_size / 2.0)
        num6 = f32(self.mesh_size / 2.0)
        k = 0
        for i in range(side):
            for j in range(side):
                self.cacheVerts[k] = (f32(num5 - float(j) * num), 0.0, f32(num6 - float(i) * num2))
                self.cacheUVs[k] = (f32(float(j) * num3), f32(float(i) * num4))
                # (double)((float)num8 * num3): no float rounding before the conversion (Mono rule)
                self.cacheUVQuad[k] = (1.0 - float(j) * num3, 1.0 - float(i) * num4)
                array[k] = (0.0, 1.0, 0.0)
                k += 1
        self.create_index_cache()
        self.tan1 = [V3ZERO] * n
        self.tan2 = [V3ZERO] * n
        c0 = self.cacheIndices[0]
        for t in range(self.triCount):
            a, b, c = c0[3 * t], c0[3 * t + 1], c0[3 * t + 2]
            v2, v3, v4 = self.cacheVerts[a], self.cacheVerts[b], self.cacheVerts[c]
            u5 = self.cacheUVs[a] + (0.0,)
            u6 = self.cacheUVs[b] + (0.0,)
            u7 = self.cacheUVs[c] + (0.0,)
            n14 = f32(v3[0] - v2[0])
            n15 = f32(v4[0] - v2[0])
            n16 = f32(v3[1] - v2[1])
            n17 = f32(v4[1] - v2[1])
            n18 = f32(v3[2] - v2[2])
            n19 = f32(v4[2] - v2[2])
            n20 = f32(u6[0] - u5[0])
            n21 = f32(u7[0] - u5[0])
            n22 = f32(u6[1] - u5[1])
            n23 = f32(u7[1] - u5[1])
            n24 = f32(1.0 / (n20 * n23 - n21 * n22))
            v8 = (f32((n23 * n14 - n22 * n15) * n24), f32((n23 * n16 - n22 * n17) * n24), f32((n23 * n18 - n22 * n19) * n24))
            v9 = (f32((n20 * n15 - n21 * n14) * n24), f32((n20 * n17 - n21 * n16) * n24), f32((n20 * n19 - n21 * n18) * n24))
            for idx in (a, b, c):
                self.tan1[idx] = v3add(self.tan1[idx], v8)
            for idx in (a, b, c):
                self.tan2[idx] = v3add(self.tan2[idx], v9)
        self.cacheTangents = [None] * n
        for i in range(n):
            u, tg = native_orthonormalize(array[i], self.tan1[i])
            w = -1.0 if v3dot(v3cross(u, tg), self.tan2[i]) < 0.0 else 1.0
            self.cacheTangents[i] = (tg[0], tg[1], tg[2], w)
        self.verts = [(0.0, 0.0, 0.0)] * n     # PQS.verts (Vector3d scratch)
        self.uvs = [(0.0, 0.0)] * n
        self.normals = [V3ZERO] * n
        self.triNormals = [V3ZERO] * self.triCount
        self.created = True

    def ti(self, x, z):  # PQS.cs:1719
        return x * 6 + z * self.res * 6

    def vi(self, x, z):  # PQS.cs:1724
        return z * self.side + x

    def create_index_cache(self):  # PQS.cs:1634
        c0 = [0] * self.triIndexCount
        k = 0
        v = 0
        for _ in range(self.res):
            for _ in range(self.res):
                c0[k:k + 6] = [v, v + self.side, v + 1, v + 1, v + self.side, v + self.side + 1]
                k += 6
                v += 1
            v += 1
        self.cacheIndices = [c0]
        for st in range(1, 17):
            tris = list(c0)
            self.create_index_state(st, tris, c0)
            self.cacheIndices.append(tris)

    def create_index_state(self, state, tris, bck):  # PQS.cs:1666
        res = self.res
        if state & 1:
            for i in range(0, res, 2):
                a, b = self.ti(i, 0), self.ti(i + 1, 0)
                tris[a + 3] = bck[a]
                tris[b] = tris[a + 3]
                tris[a] = tris[a + 1] = tris[a + 2] = 0
        if state & 2:
            for i in range(0, res, 2):
                a, b = self.ti(i, res - 1), self.ti(i + 1, res - 1)
                tris[a + 5] = bck[b + 5]
                tris[b + 1] = tris[a + 5]
                tris[b + 3] = tris[b + 4] = tris[b + 5] = 0
        if state & 4:
            for i in range(0, res, 2):
                a, b = self.ti(0, i), self.ti(0, i + 1)
                tris[a + 4] = bck[a]
                tris[b] = tris[a + 4]
                tris[a] = tris[a + 1] = tris[a + 2] = 0
        if state & 8:
            for i in range(0, res, 2):
                a, b = self.ti(res - 1, i), self.ti(res - 1, i + 1)
                tris[a + 5] = bck[b + 5]
                tris[b + 2] = tris[a + 5]
                tris[b + 3] = tris[b + 4] = tris[b + 5] = 0


# =====================================================================================================================
# PQ (PQ.cs)
# =====================================================================================================================
QE_NORTH, QE_SOUTH, QE_EAST, QE_WEST, QE_NULL = 0, 1, 2, 3, -1


class Mesh:
    def __init__(self):
        self.vertices = []
        self.normals = []
        self.tangents = []
        self.uv = []
        self.uv2 = []
        self.uv3 = []
        self.uv4 = []
        self.colors = []
        self.triangleSet = 0


class PQ:
    def __init__(self, cache, tmpl):
        # PQSCache.IncreasePQCache :404-430 + PQ.Awake :169
        self.id = 0
        self.subdivision = 0
        self.sphereRoot = None
        self.quadRoot = None
        self.parent = None
        self.north = self.south = self.east = self.west = None
        self.subNodes = [None, None, None, None]
        self.isActive = False
        self.isSubdivided = False
        self.isVisible = False
        self.isForcedInvisible = False
        self.isBuilt = False
        self.isQueuedForNormalUpdate = False
        self.isQueuedOnlyForCornerNormalUpdate = False
        self.isCached = False
        self.isPendingCollapse = False
        self.mesh = Mesh()
        self.mesh.vertices = list(cache.cacheVerts)
        self.mesh.uv = list(tmpl["uv"])
        self.mesh.normals = list(cache.cacheNormals)
        self.mesh.tangents = list(tmpl["tangents"])
        self.mesh.triangleSet = 0
        self.mesh.colors = list(tmpl["colors"])
        self.verts = list(cache.cacheVerts)
        self.vertNormals = list(cache.cacheNormals)
        self.edgeNormals = [[V3ZERO] * cache.side for _ in range(4)]
        self.vertsD = [(0.0, 0.0, 0.0)] * cache.vertCount
        self.meshOrigin = (0.0, 0.0, 0.0)
        self.quadMatrix = None
        self.planeRotation = (0.0, 0.0, 0.0, 0.0)
        self.plane = 0
        self.positionPlanetRelative = (0.0, 0.0, 0.0)
        self.positionPlanet = (0.0, 0.0, 0.0)
        self.positionPlanePosition = (0.0, 0.0, 0.0)
        self.positionParentRelative = (0.0, 0.0, 0.0)
        self.scalePlanetRelative = 0.0
        self.scalePlaneRelative = 0.0
        self.quadRotation = (0.0, 0.0, 0.0, 0.0)
        self.quadScaleFactor = 0.0
        self.quadScale = (0.0, 0.0, 0.0)
        self.quadArea = 0.0
        self.meshVertMin = 0.0
        self.meshVertMax = 0.0
        self.angularinterval = 0.0
        self.subdivideThresholdFactor = 0.0
        self.gcDist = 0.0
        self.gcd1 = 0.0
        self.edgeState = -1
        self.uvSW = (0.0, 0.0)
        self.uvDelta = (0.0, 0.0)
        self.quadIndex = 0
        self.Corner = 0
        self.CreateParent = None
        self.name = ""
        self.rendererEnabled = True  # a new MeshRenderer is enabled
        self.meshColliderCreated = False
        self.meshColliderEnabled = False
        self.outOfTime = False
        # bookkeeping of the reference (not KSP state): is the mesh data exact (real bodies: built with oracle heights),
        # and which build (quad name) produced the current normals / edge normals
        self.exact = True
        self.buildKey = None
        self.edgeKey = None

    # ---- PQ.cs ---------------------------------------------------------------------------------------------------------
    def SetupQuad(self, parentQuad, parentChildPosition):  # :178
        s = self.sphereRoot
        self.angularinterval = s.circumference * Atan(self.scalePlaneRelative / s.radius)
        self.quadRotation = native_from_to_rotation((0.0, 1.0, 0.0), d2f(self.positionPlanetRelative))
        if self.quadRoot is not None:
            self.quadScaleFactor = 1.0 / Pow(2.0, float(self.subdivision - 1))
            self.quadArea = self.quadRoot.quadArea * self.quadScaleFactor
            self.quadScale = (self.quadScaleFactor, self.quadScaleFactor, self.quadScaleFactor)
            self.quadMatrix = native_trs(d2f(self.positionPlanePosition), tuple(f32(c) for c in self.quadRoot.planeRotation),
                                         d2f(self.quadScale))
        else:
            self.planeRotation = native_from_to_rotation((0.0, 1.0, 0.0), d2f(dnormalized(self.positionPlanePosition)))
            self.quadScaleFactor = 2.0
            self.quadArea = Pow(s.radius * 2.0, 2.0)
            self.quadScale = (2.0, 2.0, 2.0)
            self.quadMatrix = native_trs(d2f(self.positionPlanePosition), tuple(f32(c) for c in self.planeRotation),
                                         d2f(self.quadScale))
        # quadTransform.localPosition = positionPlanet (surfaceRelativeQuads) or zero: a float Vector3
        self.meshOrigin = d2f(self.positionPlanet) if s.surfaceRelativeQuads else (0.0, 0.0, 0.0)
        if parentQuad is None:
            self.quadIndex = 0
        elif self.subdivision == 1:
            self.quadIndex = parentQuad.quadIndex + parentChildPosition
        else:
            self.quadIndex = parentQuad.quadIndex + (parentChildPosition << ((2 * (self.subdivision - 1)) & 31))
        self.UpdateTargetRelativity()

    def UpdateTargetRelativity(self):  # :280
        s = self.sphereRoot
        self.gcd1 = Acos(ddot(self.positionPlanetRelative, s.relativeTargetPositionNormalized)) * s.radius * 1.3
        self.gcDist = self.gcd1 + abs(s.targetHeight) - self.angularinterval

    def UpdateVisibility(self):  # :286
        if not self.isSubdivided and self.gcd1 < self.sphereRoot.visibleRadius:
            self.SetVisible()
            new = self.GetEdgeState()
            if new != self.edgeState:
                self.mesh.triangleSet = new
                self.edgeState = new
                self.QueueForNormalUpdate()
        else:
            self.SetInvisible()

    def UpdateSubdivision(self):  # :305
        self.UpdateTargetRelativity()
        s = self.sphereRoot
        self.outOfTime = s.now() > s.maxFrameEnd
        self.isPendingCollapse = False
        if self.isSubdivided:
            self.rendererEnabled = False
            flag = self.gcDist > s.collapseThresholds[self.subdivision] * self.subdivideThresholdFactor
            if self.subdivision <= s.maxLevel and (not flag or self.outOfTime):
                if flag:
                    self.isPendingCollapse = True
                for i in range(4):
                    if self.subNodes[i] is not None:
                        self.subNodes[i].UpdateSubdivision()
            elif not self.Collapse():
                for j in range(4):
                    if self.subNodes[j] is not None:
                        self.subNodes[j].UpdateSubdivision()
        elif self.subdivision >= s.minLevel and (
                not (self.gcDist < s.subdivisionThresholds[self.subdivision] * self.subdivideThresholdFactor)
                or self.subdivision >= s.maxLevelAtCurrentTgtSpeed or self.outOfTime):
            self.UpdateVisibility()
        else:
            self.Subdivide()

    def UpdateSubdivisionInit(self):  # :353
        self.UpdateTargetRelativity()
        s = self.sphereRoot
        if self.isSubdivided:
            if self.subdivision > s.minLevel and (
                    self.subdivision > s.maxLevel
                    or self.gcDist > s.collapseThresholds[self.subdivision] * self.subdivideThresholdFactor):
                self.Collapse()
                return
            for i in range(4):
                if self.subNodes[i] is not None:
                    self.subNodes[i].UpdateSubdivisionInit()
        elif self.gcDist < s.subdivisionThresholds[self.subdivision] * self.subdivideThresholdFactor and \
                self.subdivision < s.maxLevel:
            self.Subdivide()

    def Build(self):  # :381
        if self.isBuilt or not self.isActive or not self.sphereRoot.quadAllowBuild or self.isCached:
            return
        if self.isSubdivided:
            for i in range(4):
                self.subNodes[i].Build()
            return
        self.isBuilt = self.sphereRoot.BuildQuad(self)
        if self.isBuilt:
            self.QueueForNormalUpdate()

    def Subdivide(self):  # :402
        if self.north.subdivision < self.subdivision:
            return False
        if self.east.subdivision < self.subdivision:
            return False
        if self.south.subdivision < self.subdivision:
            return False
        if self.west.subdivision < self.subdivision:
            return False
        if self.isSubdivided:
            return True
        if not self.isActive:
            return False
        s = self.sphereRoot
        for i in range(4):
            if self.subNodes[i] is not None:
                self.subNodes[i].isActive = True
                continue
            q = s.AssignQuad(self.subdivision + 1)
            n1 = i % 2
            n2 = i // 2
            q.scalePlaneRelative = self.scalePlaneRelative * 0.5
            q.scalePlanetRelative = s.radius * q.scalePlaneRelative
            q.quadRoot = self if self.quadRoot is None else self.quadRoot
            q.CreateParent = self
            q.positionParentRelative = qd_mul_v(q.quadRoot.planeRotation, ((float(n1) - 0.5) * self.scalePlaneRelative, 0.0,
                                                                            (float(n2) - 0.5) * self.scalePlaneRelative))
            q.positionPlanePosition = dadd(self.positionPlanePosition, q.positionParentRelative)
            q.positionPlanetRelative = dnormalized(q.positionPlanePosition)
            q.positionPlanet = dmul(q.positionPlanetRelative, s.GetSurfaceHeight(q.positionPlanetRelative))
            q.plane = self.plane
            q.sphereRoot = s
            q.subdivision = self.subdivision + 1
            q.parent = self
            q.Corner = i
            q.name = self.name + str(i)
            s.QuadCreated(q)
            self.subNodes[i] = q
        n0, n1_, n2_, n3 = self.subNodes
        n0.north = n2_
        n0.east = n1_
        n1_.north = n3
        n1_.west = n0
        n2_.south = n0
        n2_.east = n3
        n3.south = n1_
        n3.west = n2_
        if self.north.subdivision == self.subdivision and self.north.isSubdivided:
            left, right = self.north.GetEdgeQuads(self)
            n2_.north = left
            n3.north = right
            left.SetNeighbour(self, n2_)
            right.SetNeighbour(self, n3)
        else:
            n2_.north = self.north
            n3.north = self.north
        if self.south.subdivision == self.subdivision and self.south.isSubdivided:
            left, right = self.south.GetEdgeQuads(self)
            n1_.south = left
            n0.south = right
            left.SetNeighbour(self, n1_)
            right.SetNeighbour(self, n0)
        else:
            n1_.south = self.south
            n0.south = self.south
        if self.east.subdivision == self.subdivision and self.east.isSubdivided:
            left, right = self.east.GetEdgeQuads(self)
            n3.east = left
            n1_.east = right
            left.SetNeighbour(self, n3)
            right.SetNeighbour(self, n1_)
        else:
            n3.east = self.east
            n1_.east = self.east
        if self.west.subdivision == self.subdivision and self.west.isSubdivided:
            left, right = self.west.GetEdgeQuads(self)
            n0.west = left
            n2_.west = right
            left.SetNeighbour(self, n0)
            right.SetNeighbour(self, n2_)
        else:
            n0.west = self.west
            n2_.west = self.west
        if s.reqUVQuad:  # :527 (static uvDel / uvMidPoint / uvMidS / uvMidW)
            uvDel = (f32(self.uvDelta[0] * f32(0.5)), f32(self.uvDelta[1] * f32(0.5)))
            mid = (f32(self.uvSW[0] + uvDel[0]), f32(self.uvSW[1] + uvDel[1]))
            midS = (mid[0], self.uvSW[1])
            midW = (self.uvSW[0], mid[1])
            n0.uvSW, n0.uvDelta = mid, uvDel
            n1_.uvSW, n1_.uvDelta = midW, uvDel
            n2_.uvSW, n2_.uvDelta = midS, uvDel
            n3.uvSW, n3.uvDelta = self.uvSW, uvDel
        self.isSubdivided = True
        self.SetInvisible()
        for i in range(4):
            c = self.subNodes[i]
            if not c.isCached:
                c.SetupQuad(self, i)
            if s.quadAllowBuild:
                c.UpdateVisibility()
        self.north.QueueForNormalUpdate()
        self.south.QueueForNormalUpdate()
        self.east.QueueForNormalUpdate()
        self.west.QueueForNormalUpdate()
        for nb in (self.north, self.west, self.south, self.east):
            r = self.GetRightmostCornerPQ(nb)
            if r is not None:
                r.QueueForCornerNormalUpdate()
        return True

    def Collapse(self):  # :590
        s = self.sphereRoot
        if self in s.normalUpdateList:
            self.UnqueueForNormalUpdate()
        if not self.isSubdivided:
            return False
        if self.north.subdivision > self.subdivision:
            return False
        if self.south.subdivision > self.subdivision:
            return False
        if self.east.subdivision > self.subdivision:
            return False
        if self.west.subdivision > self.subdivision:
            return False
        for i in range(4):
            c = self.subNodes[i]
            if c.isCached:
                c.SetInvisible()
                c.isActive = False
                if c in s.normalUpdateList:
                    c.UnqueueForNormalUpdate()
            else:
                c.ClearAndDestroy()
                self.subNodes[i] = None
        if self.north is None:
            self.north = self.parent.north
        if self.south is None:
            self.south = self.parent.south
        if self.east is None:
            self.east = self.parent.east
        if self.west is None:
            self.west = self.parent.west
        self.isSubdivided = False
        self.UpdateVisibility()
        self.north.QueueForNormalUpdate()
        self.south.QueueForNormalUpdate()
        self.east.QueueForNormalUpdate()
        self.west.QueueForNormalUpdate()
        for nb in (self.north, self.west, self.south, self.east):
            r = self.GetRightmostCornerPQ(nb)
            if r is not None:
                r.QueueForCornerNormalUpdate()
        return True

    def ClearAndDestroy(self):  # :682
        s = self.sphereRoot
        if self.isSubdivided:
            for i in range(4):
                if self.subNodes[i] is not None:
                    self.subNodes[i].ClearAndDestroy()
                    self.subNodes[i] = None
        for nb in (self.north, self.east, self.south, self.west):
            if nb is not None and nb.subdivision == self.subdivision:
                nb.SetNeighbour(self, self.parent)
        if self in s.normalUpdateList:
            self.UnqueueForNormalUpdate()
        s.DestroyQuad(self)

    def Destroy(self):  # :726
        s = self.sphereRoot
        if self.isSubdivided:
            for i in range(4):
                if self.subNodes[i] is not None:
                    self.subNodes[i].Destroy()
                    self.subNodes[i] = None
        if self in s.normalUpdateList:
            self.UnqueueForNormalUpdate()
        s.DestroyQuad(self)

    def SetVisible(self):  # :778
        if not self.isVisible:
            self.isVisible = True
            if not self.isBuilt:
                self.Build()
            self.rendererEnabled = not self.isForcedInvisible

    def SetInvisible(self):  # :802
        if self.isVisible:
            self.isVisible = False
            self.rendererEnabled = False

    def SetMasterVisible(self):  # :815
        if self.isVisible and not self.isForcedInvisible:
            self.rendererEnabled = True
        if self.isSubdivided:
            for i in (3, 2, 1, 0):
                self.subNodes[i].SetMasterVisible()

    def SetMasterInvisible(self):  # :830
        if self.isVisible:
            self.rendererEnabled = False
        if self.isSubdivided:
            for i in (3, 2, 1, 0):
                self.subNodes[i].SetMasterInvisible()

    def QueueForNormalUpdate(self):  # :846
        s = self.sphereRoot
        if s is not None and s.reqCustomNormals and not self.isQueuedForNormalUpdate and s.quadAllowBuild:
            s.normalUpdateList.append(self)
            self.isQueuedForNormalUpdate = True

    def QueueForCornerNormalUpdate(self):  # :855
        s = self.sphereRoot
        if s is not None and s.reqCustomNormals and not self.isQueuedForNormalUpdate and s.quadAllowBuild:
            s.normalUpdateList.append(self)
            self.isQueuedForNormalUpdate = True
            self.isQueuedOnlyForCornerNormalUpdate = True

    def UnqueueForNormalUpdate(self):  # :865
        s = self.sphereRoot
        if s is not None and self.isQueuedForNormalUpdate:
            if self in s.normalUpdateList:
                s.normalUpdateList.remove(self)
            self.isQueuedForNormalUpdate = False
            self.isQueuedOnlyForCornerNormalUpdate = False

    def GetEdgeState(self):  # :875
        e = 0
        if self.north.subdivision < self.subdivision:
            e |= 1
        if self.south.subdivision < self.subdivision:
            e |= 2
        if self.east.subdivision < self.subdivision:
            e |= 4
        if self.west.subdivision < self.subdivision:
            e |= 8
        return e

    def GetEdgeQuads(self, caller):  # :918
        sn = self.subNodes
        if self.north is caller:
            return sn[3], sn[2]
        if self.south is caller:
            return sn[0], sn[1]
        if self.east is caller:
            return sn[1], sn[3]
        if self.west is caller:
            return sn[2], sn[0]
        cp = caller.parent
        if self.north is cp:
            return sn[3], sn[2]
        if self.south is cp:
            return sn[0], sn[1]
        if self.east is cp:
            return sn[1], sn[3]
        if self.west is cp:
            return sn[2], sn[0]
        return None, None

    def GetEdge(self, caller):  # :964
        if caller is self.north:
            return QE_NORTH
        if caller is self.south:
            return QE_SOUTH
        if caller is self.east:
            return QE_EAST
        if caller is self.west:
            return QE_WEST
        return QE_NULL

    def GetRightmostCornerPQ(self, nextQuad):  # :985
        pq = self
        if nextQuad.subdivision < self.subdivision:
            pq = pq.parent
        edge = nextQuad.GetEdge(pq)
        if edge == QE_NULL:
            return None
        rot = {QE_NORTH: QE_WEST, QE_WEST: QE_SOUTH, QE_SOUTH: QE_EAST, QE_EAST: QE_NORTH}[edge]  # PQS.cs:3025
        pq2 = nextQuad.GetSidePQ(rot)
        if not pq2.isBuilt:
            pq2.Build()
        if pq2.isSubdivided:
            _, right = pq2.GetEdgeQuads(nextQuad)
            pq2 = right
        if not pq2.isBuilt:
            pq2.Build()
        return pq2

    def GetSidePQ(self, edge):  # :1020
        if edge == QE_NORTH:
            return self.north
        if edge == QE_EAST:
            return self.east
        if edge == QE_SOUTH:
            return self.south
        return self.west

    def SetNeighbour(self, old, new):  # :1035
        if new is self:
            return
        if old is self.north:
            self.north = new
        elif old is self.south:
            self.south = new
        elif old is self.east:
            self.east = new
        elif old is self.west:
            self.west = new
        elif self.parent is not None:
            p = self.parent
            if p.north is old:
                self.north = new
            elif p.south is old:
                self.south = new
            elif p.east is old:
                self.east = new
            elif p.west is old:
                self.west = new


# =====================================================================================================================
# PQSCache (PQSCache.cs:217-468): Stack<PQ> pool, fresh quads below the returned ones
# =====================================================================================================================
class Pool:
    def __init__(self, cache, initial=9216, increase=256):
        self.cache = cache
        self.initial = initial
        self.increase = increase
        self.reused = []
        self.fresh = 0
        self.assigned = 0
        self.unassigned = 0
        self.total = 0
        self.materialised = 0
        self.tmpl = None
        self.ready = False

    def create(self):  # :378
        if not self.cache.created:
            self.cache.create()
        self.reused = []
        self.fresh = 0
        self.unassigned = 0
        self.assigned = 0
        self.increase_cache(self.initial)
        self.ready = True

    def increase_cache(self, n):  # :400 (the new meshes copy the static arrays as they are now)
        c = self.cache
        self.tmpl = {"uv": list(c.cacheUVs), "tangents": list(c.cacheTangents), "colors": list(c.cacheColors)}
        self.fresh += n
        self.unassigned += n
        self.total += n

    def get_quad(self):  # :436
        if self.unassigned == 0:
            self.increase_cache(self.increase)
        self.assigned += 1
        self.unassigned -= 1
        if self.reused:
            return self.reused.pop()
        self.fresh -= 1
        self.materialised += 1
        return PQ(self.cache, self.tmpl)

    def destroy_quad(self, q):  # :448
        q.north = q.south = q.east = q.west = None
        q.parent = None
        q.quadRoot = None
        q.sphereRoot = None
        q.subdivideThresholdFactor = 1.0
        q.isBuilt = False
        q.isSubdivided = False
        q.isActive = False
        q.isVisible = False
        q.rendererEnabled = False
        self.assigned -= 1
        self.unassigned += 1
        self.reused.append(q)
