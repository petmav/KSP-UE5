"""WP-12a reference, part 3: the PQS sphere (PQS.cs) and the PQSMods the scenarios use. No kspcore code."""
import math

from pqs_ref_math import (Acos, Asin, Atan, Cos, Pow, Sin, V3ZERO, d2f, dadd, ddot, dmag, dmul, dnormalized, dsqrmag,
                          dsub, f32, mul_point3x4, native_orthonormalize, v3add, v3cross, v3dot, v3mul, v3normalized)
from pqs_ref_tree import DMAX, PQ, QE_EAST, QE_NORTH, QE_NULL, QE_SOUTH, QE_WEST

# PQS.ModiferRequirements (PQS.cs:60)
REQ = {"Default": 0, "UniqueMaterialInstances": 1, "VertexMapCoords": 2, "VertexGnomonicMapCoords": 4, "UVSphereCoords": 8,
       "UVQuadCoords": 0x10, "MeshColorChannel": 0x20, "MeshCustomNormals": 0x40, "MeshBuildTangents": 0x80,
       "MeshUV2": 0x100, "MeshUV3": 0x200, "MeshUV4": 0x400, "MeshAssignTangents": 0x800}

GS_MAINMENU, GS_FLIGHT, GS_PSYSTEM, GS_MISSIONBUILDER = 2, 7, 9, 21


class VBData:  # PQS.VertexBuildData (PQS.cs:77)
    def __init__(self):
        self.buildQuad = None
        self.globalV = (0.0, 0.0, 0.0)
        self.directionFromCenter = (0.0, 0.0, 0.0)
        self.vertIndex = 0
        self.vertHeight = 0.0
        self.vertColor = (0.0, 0.0, 0.0, 0.0)
        self.u = self.v = self.u2 = self.v2 = self.u3 = self.v3 = self.u4 = self.v4 = 0.0
        self.allowScatter = False
        self.gnomonicPlane = 0
        self.latitude = 0.0
        self.longitude = 0.0


# =====================================================================================================================
# mods
# =====================================================================================================================
class Mod:  # PQSMod.cs
    affects_height = False

    def __init__(self, order=100, enabled=True, requirements=0):
        self.order = order
        self.modEnabled = enabled
        self.requirements = requirements
        self.sphere = None
        self.overrideQuadBuildCheck = False

    def OnSetup(self):  # PQSMod.cs:29
        self.requirements = 0

    def OnSphereStart(self):
        return False

    def OnVertexBuildHeight(self, d):
        pass

    def OnVertexBuild(self, d):
        pass

    def GetVertexMaxHeight(self):
        return 0.0

    def GetVertexMinHeight(self):
        return 0.0

    def OnQuadPreBuild(self, q):
        pass

    def OnQuadBuilt(self, q):
        pass

    def OnQuadDestroy(self, q):
        pass

    def OnQuadUpdateNormals(self, q):
        pass

    def OnPreUpdate(self):
        pass

    def OnUpdateFinished(self):
        pass

    def OnSphereReset(self):
        pass

    def OnSphereInactive(self):
        pass

    def OnSphereTransformUpdate(self):
        pass


class PassiveReq(Mod):
    """a mod whose only effect here is its requirement bits (the real stacks: values from the in-game capture)"""

    def __init__(self, order, requirements):
        super().__init__(order, True, requirements)
        self.fixed = requirements

    def OnSetup(self):
        self.requirements = self.fixed


class SynthHeight(Mod):
    """test-only height mod (defined identically in tests/test_pqs_quadtree.cpp, class SynthHeightMod)"""
    affects_height = True

    def __init__(self, order, setup_req, A, B, C, D, E, K1, K2, F, hmax):
        super().__init__(order, True, 0)
        self.setup_req = setup_req
        self.A, self.B, self.C, self.D, self.E, self.K1, self.K2, self.F, self.hmax = A, B, C, D, E, K1, K2, F, hmax

    def OnSetup(self):
        self.requirements = self.setup_req

    def OnVertexBuildHeight(self, d):
        x, y, z = d.directionFromCenter
        h = self.A * (x * y) + self.B * ((z * z) * x) - self.C * (y * z) + self.D * (x - 0.3 * z)
        tx = x * self.K1
        tz = z * self.K2
        wx = abs((tx - math.floor(tx)) - 0.5)
        wz = abs((tz - math.floor(tz)) - 0.5)
        h += self.E * (wx * wz)
        if self.F != 0.0:
            h += self.F * (d.latitude * d.longitude)
        d.vertHeight += h

    def OnVertexBuild(self, d):
        # colour / uv channels the test mod writes so that every PQS channel builder has data
        d.vertColor = (f32(d.directionFromCenter[0]), f32(d.directionFromCenter[1]), f32(d.directionFromCenter[2]), d.vertColor[3])
        d.u2 = d.vertHeight * 0.001
        d.v2 = d.directionFromCenter[1]
        d.u4 = d.directionFromCenter[0] - d.directionFromCenter[2]
        d.v4 = d.vertHeight

    def GetVertexMaxHeight(self):
        return self.hmax

    def GetVertexMinHeight(self):
        return -self.hmax


class FlattenArea(Mod):  # PQSMod_FlattenArea.cs (useLatLon false, DEBUG_showColors false)
    affects_height = True

    def __init__(self, order, outerRadius, innerRadius, position, flattenTo, smoothStart, smoothEnd):
        super().__init__(order, True, 0)
        self.outerRadius, self.innerRadius, self.position = outerRadius, innerRadius, tuple(f32(c) for c in position)
        self.flattenTo, self.smoothStart, self.smoothEnd = flattenTo, smoothStart, smoothEnd
        self.quadActive = False

    def OnSetup(self):  # :64
        self.requirements = REQ["MeshColorChannel"] | REQ["MeshCustomNormals"]
        if self.modEnabled:
            self.posNorm = v3normalized(self.position)
            self.angleInner = Atan(self.innerRadius / self.sphere.radius)
            self.angleOuter = Atan(self.outerRadius / self.sphere.radius)
            self.angleQuadInclusion = self.angleOuter * 2.0
            self.angleDelta = self.angleOuter - self.angleInner
            self.flattenToRadius = self.sphere.radius + self.flattenTo

    def OnQuadPreBuild(self, q):  # :89
        t = Acos(ddot(q.positionPlanetRelative, self.posNorm))
        self.quadActive = t < self.angleQuadInclusion

    def OnQuadBuilt(self, q):  # :102
        self.quadActive = True

    def OnVertexBuildHeight(self, d):  # :107
        if not self.overrideQuadBuildCheck and not self.quadActive:
            return
        t = Acos(ddot(d.directionFromCenter, self.posNorm))
        if not (t < self.angleQuadInclusion):
            return
        if not (t < self.angleOuter):
            return
        if t < self.angleInner:
            d.vertHeight = self.flattenToRadius
            return
        a = (t - self.angleInner) / self.angleDelta
        ct2 = a * a
        ct3 = ct2 * a
        start, end = self.flattenToRadius, d.vertHeight
        d.vertHeight = (start * (2.0 * ct3 - 3.0 * ct2 + 1.0) + self.smoothStart * (ct3 - 2.0 * ct2 + a)
                        + end * (-2.0 * ct3 + 3.0 * ct2) + self.smoothEnd * (ct3 - ct2))


class QuadEnhanceCoast(Mod):  # PQSMod_QuadEnhanceCoast.cs
    def __init__(self, order, coastLessThan, oceanFactor, coastFactor, enabled=True, requirements=0):
        super().__init__(order, enabled, requirements)
        self.coastLessThan, self.oceanFactor, self.coastFactor = coastLessThan, oceanFactor, coastFactor

    def OnSetup(self):  # :20 (no base call)
        self.coastTest = self.sphere.radius + self.coastLessThan

    def OnQuadBuilt(self, q):  # :25
        if q.meshVertMin < self.coastTest:
            if q.meshVertMax > self.coastTest:
                q.subdivideThresholdFactor *= self.coastFactor
            else:
                q.subdivideThresholdFactor *= self.oceanFactor


class UVPlanetRelativePosition(Mod):  # PQSMod_UVPlanetRelativePosition.cs
    def __init__(self, order, enabled=True, requirements=0):
        super().__init__(order, enabled, requirements)

    def OnQuadBuilt(self, q):  # :12
        c = self.sphere.cache
        for i in range(c.vertCount):
            v = c.verts[i]
            c.cacheUVs[i] = (f32(v[0]), f32(v[1]))
            c.cacheUV2s[i] = (f32(v[2]), f32(1.0 - ddot(dnormalized(v), q.vertNormals[i])))
        q.mesh.uv = list(c.cacheUVs)
        q.mesh.uv2 = list(c.cacheUV2s)

    def OnQuadUpdateNormals(self, q):  # :27
        c = self.sphere.cache
        for i in range(c.vertCount):
            v = dadd(q.verts[i], q.positionPlanet)
            c.cacheUV2s[i] = (f32(v[2]), f32(1.0 - ddot(dnormalized(v), q.vertNormals[i])))
        q.mesh.uv2 = list(c.cacheUV2s)


class AltitudeUV(Mod):  # PQSMod_AltitudeUV.cs
    def __init__(self, order, atmosphereHeight, oceanDepth, invert, enabled=True, requirements=0):
        super().__init__(order, enabled, requirements)
        self.atmosphereHeight, self.oceanDepth, self.invert = atmosphereHeight, oceanDepth, invert

    def OnSetup(self):
        self.requirements = REQ["MeshUV3"]

    def OnVertexBuild(self, d):  # :26
        h = d.vertHeight - self.sphere.radius
        if h >= 0.0:
            h /= self.atmosphereHeight
        else:
            h /= self.oceanDepth
        h = -1.0 if h < -1.0 else (1.0 if h > 1.0 else h)  # UtilMath.Clamp
        d.u3 = 1.0 - h if self.invert else h


class AltitudeAlpha(Mod):  # PQSMod_AltitudeAlpha.cs
    def __init__(self, order, atmosphereDepth, invert, enabled=True, requirements=0):
        super().__init__(order, enabled, requirements)
        self.atmosphereDepth, self.invert = atmosphereDepth, invert

    def OnSetup(self):
        self.requirements = REQ["MeshColorChannel"]

    def OnVertexBuild(self, d):  # :23
        h = (d.vertHeight - self.sphere.radius) / self.atmosphereDepth
        a = f32(1.0 - h) if self.invert else f32(h)
        d.vertColor = (d.vertColor[0], d.vertColor[1], d.vertColor[2], a)


class QuadMeshColliders(Mod):  # PQSMod_QuadMeshColliders.cs
    def __init__(self, order, maxLevelOffset, enabled=True, requirements=0):
        super().__init__(order, enabled, requirements)
        self.maxLevelOffset = maxLevelOffset

    def OnSetup(self):  # :17
        self.minLevel = self.sphere.maxLevel - abs(self.maxLevelOffset)

    def OnQuadBuilt(self, q):  # :22
        if q.subdivision >= self.minLevel:
            q.meshColliderCreated = True
            q.meshColliderEnabled = True

    def OnQuadDestroy(self, q):  # :36
        if q.meshColliderCreated:
            q.meshColliderEnabled = False


class AltitudeFade:  # PQSMod_CelestialBodyTransform.cs:9
    def __init__(self, fadeStart, fadeEnd, valueStart, valueEnd, hqStart=-1.0, hqEnd=-1.0):
        self.fadeStart, self.fadeEnd = f32(fadeStart), f32(fadeEnd)
        self.valueStart, self.valueEnd = f32(valueStart), f32(valueEnd)
        self.hqStart, self.hqEnd = f32(hqStart), f32(hqEnd)
        self.a = 0.0

    def DoFadeAlt(self, alt, quality=0):  # :35
        num = self.fadeStart
        if self.hqStart > 0.0 and quality >= 2:
            num = self.hqStart
        num2 = self.fadeEnd
        if self.hqEnd > 0.0 and quality >= 2:
            num2 = self.hqEnd
        if alt <= num:
            self.a = self.valueStart
        elif alt >= num2:
            self.a = self.valueEnd
        else:
            self.a = f32((alt - num) / (num2 - num))

    def DoFade(self, fade):  # :76
        self.a = self.valueStart if fade else self.valueEnd


class CelestialBodyTransform(Mod):  # PQSMod_CelestialBodyTransform.cs, flight host: FlightGlobals present, this body is
    # the main body, the active vessel is the sphere's target object, map view off, TERRAIN_SHADER_QUALITY 0
    def __init__(self, order, deactivateAltitude, hqDeactivate, planetFade, secondaryFades, host):
        super().__init__(order, True, 0)
        self.deactivateAltitude, self.hqDeactivate = deactivateAltitude, hqDeactivate
        self.planetFade, self.secondaryFades = planetFade, secondaryFades
        self.forceActivate = False
        self.overrideFade = False
        self.host = host  # {"vessel": target object}

    def OnSetup(self):  # :170 (a body exists)
        self.planetFade.DoFade(False)
        for f in reversed(self.secondaryFades):
            f.DoFade(False)

    def OnSphereStart(self):  # :204
        s = self.sphere
        v = self.host["vessel"]
        if v is not None:
            s.SetTarget(v)
        s.isAlive = True
        return True

    def OnPreUpdate(self):  # :234 (FLIGHT branch)
        s = self.sphere
        v = self.host["vessel"]
        if v is not s.target:
            if v is None:
                s.SetTarget(None)
                return
            s.SetTarget(v)
        if s.isActive:
            if s.visibleAltitude > self.deactivateAltitude and (0 < 2 or self.hqDeactivate < 0.0):
                s.DeactivateSphere()
        elif s.visibleAltitude < self.deactivateAltitude and (0 < 2 or self.hqDeactivate < 0.0):
            s.ActivateSphere()

    def OnUpdateFinished(self):  # :311
        if not self.overrideFade:
            if self.sphere.isActive:
                self.planetFade.DoFadeAlt(self.sphere.visibleAltitude)
                for f in reversed(self.secondaryFades):
                    f.DoFadeAlt(self.sphere.visibleAltitude)

    def OnSphereReset(self):  # :336
        self.FadeOutSphere()

    def OnSphereInactive(self):  # :341
        self.FadeOutSphere()

    def FadeOutSphere(self):  # :346
        self.planetFade.DoFade(False)
        for f in reversed(self.secondaryFades):
            f.DoFade(False)


class Target:
    def __init__(self, pos):
        self.position = pos


# =====================================================================================================================
# the sphere (PQS.cs)
# =====================================================================================================================
class Sphere:
    def __init__(self, name, radius, mods, pool, heights=None):
        self.name = name
        self.radius = radius
        self.mods_all = mods
        self.pool = pool
        self.cache = pool.cache
        self.heights = heights  # None: evaluate the mod stack; else a provider (real bodies, see pqs_reference.py)
        # PQS.Reset() (PQS.cs:565) defaults
        self.minLevel, self.maxLevel = 3, 10
        self.maxQuadLenghtsPerFrame = f32(0.03)
        self.maxFrameTime = f32(0.1)
        self.frameTimeDelta = 0.0
        self.subdivisionThreshold = 1.0
        self.collapseSeaLevelValue, self.collapseAltitudeValue, self.collapseAltitudeMax = 2.0, 16.0, 10000000.0
        self.visRadSeaLevelValue, self.visRadAltitudeValue, self.visRadAltitudeMax = 4.0, 1.5, 100000.0
        self.minDetailDistance, self.maxDetailDistance = 7.0, 20.0
        self.surfaceRelativeQuads = False
        self.buildTangents = False
        self.isBuildingMaps = False
        self.isFakeBuild = False
        self.preset = None  # (minDistance, minSubdivision, maxSubdivision) from PQSCache.PresetList
        self.clock = None   # Time.realtimeSinceStartup provider (None: 0)
        self.isDisabled = self.isAlive = self.isActive = self.isThinking = self.isStarted = False
        self.isSubdivisionEnabled = False
        self.target = None
        self.secondaryTarget = None
        self.relativeTargetPosition = (0.0, 0.0, 0.0)
        self.relativeTargetPositionNormalized = (0.0, 0.0, 0.0)
        self.lastRelTgtPosNormalized = (0.0, 0.0, 0.0)
        self.targetPositionPrev = (0.0, 0.0, 0.0)
        self.targetAltitude = self.targetHeight = self.targetDistance = 0.0
        self.targetVelocity = (0.0, 0.0, 0.0)
        self.targetSpeed = 0.0
        self.visibleAltitude = self.visibleRadius = self.horizonDistance = self.horizonAngle = 0.0
        self.visRad = self.collapseThreshold = 0.0
        self.angularTargetSpeed = 0.0
        self.maxLevelAtCurrentTgtSpeed = 10
        self.fixedUpdateFrame = 0
        self.fixedFrameTime = self.prevFixedFrameTime = 0.0
        self.maxFrameEnd = 0.0
        self.normalUpdateList = []
        self.quads = None
        self.quadCount = 0
        self.quadAllowBuild = False
        self.pqID = 0
        self.meshVertMin, self.meshVertMax = DMAX, -DMAX
        self.vb = VBData()
        self.vertexIndex = 0
        self.lat = self.lon = self.sx = self.sy = 0.0
        self.routines = []  # suspended coroutines in resume order: "update" / "reset"
        self.any_built = False   # reference bookkeeping: has any quad been built (mod quad state of the real stacks)
        self.mods = []

    def now(self):  # Time.realtimeSinceStartup; default: a constant 1 s (the C++ default)
        return self.clock() if self.clock else 1.0

    # ---- SetupMods / SetupBuildDelegates ------------------------------------------------------------------------------
    def SetupMods(self):  # PQS.cs:2109 (the scenario lists are already in their List.Sort order: distinct `order`s)
        lst = sorted(self.mods_all, key=lambda m: m.order)
        for m in lst:
            m.sphere = self
        self.mods = [m for m in lst if m.modEnabled]
        for m in self.mods:
            m.OnSetup()
        self.modRequirements = 0
        for m in self.mods:
            self.modRequirements |= m.requirements

    def SetupBuildDelegates(self, fake):  # PQS.cs:2177
        r = self.modRequirements
        self.isFakeBuild = fake
        self.reqVertexMapCoods = (r & REQ["VertexMapCoords"]) != 0
        self.reqCustomNormals = (r & REQ["MeshCustomNormals"]) != 0
        self.reqColorChannel = (r & REQ["MeshColorChannel"]) != 0
        self.reqBuildTangents = (r & REQ["MeshBuildTangents"]) != 0 or self.buildTangents
        self.reqAssignTangents = (r & REQ["MeshAssignTangents"]) != 0 or self.reqBuildTangents
        self.reqUV2 = (r & REQ["MeshUV2"]) != 0
        self.reqUV3 = (r & REQ["MeshUV3"]) != 0
        self.reqUV4 = (r & REQ["MeshUV4"]) != 0
        self.reqSphereUV = (r & REQ["UVSphereCoords"]) != 0
        self.reqUVQuad = (r & REQ["UVQuadCoords"]) != 0
        if self.reqSphereUV:
            self.reqVertexMapCoods = True
            if self.reqUVQuad:
                self.reqUVQuad = False
        self.meshVertMin = DMAX
        self.meshVertMax = -DMAX
        if self.heights is not None:
            self.radiusMax, self.radiusMin = self.heights.radius_max, self.heights.radius_min
        else:
            mx = 0.0
            for m in self.mods:
                mx += m.GetVertexMaxHeight()
            mn = 0.0
            for m in self.mods:
                mn += m.GetVertexMinHeight()
            self.radiusMax = self.radius + mx
            self.radiusMin = self.radius + mn
        self.radiusDelta = self.radiusMax - self.radiusMin
        self.halfChord = math.sqrt(self.radiusMax * self.radiusMax - self.radiusMin * self.radiusMin)

    # ---- heights -------------------------------------------------------------------------------------------------------
    def BuildVertexMapCoords(self, d):  # PQS.cs:2294
        lat = Asin(d.directionFromCenter[1])
        if lat != lat:
            lat = math.pi / 2.0
        xz = dnormalized((d.directionFromCenter[0], 0.0, d.directionFromCenter[2]))
        if dmag(xz) > 0.0:
            lon = (math.pi - Asin(xz[0] / dmag(xz))) if xz[2] < 0.0 else Asin(xz[0] / dmag(xz))
        else:
            lon = 0.0
        d.latitude, d.longitude = lat, lon
        self.sy = lat / math.pi + 0.5
        self.sx = lon / math.pi * 0.5
        d.u, d.v = self.sx, self.sy

    def Mod_OnVertexBuildHeight(self, d, override=False):  # PQS.cs:1956
        if self.reqVertexMapCoods:
            self.BuildVertexMapCoords(d)
        for m in self.mods:
            m.overrideQuadBuildCheck = override
            m.OnVertexBuildHeight(d)
            m.overrideQuadBuildCheck = False

    def GetSurfaceHeight(self, v):  # PQS.cs:3088 / :3093
        if self.heights is not None:
            return self.heights.public(self, v)
        d = self.vb
        d.buildQuad = None
        d.directionFromCenter = dnormalized(v)
        d.vertHeight = self.radius
        d.vertIndex = self.vertexIndex
        self.Mod_OnVertexBuildHeight(d)
        return d.vertHeight

    # ---- lifecycle -----------------------------------------------------------------------------------------------------
    def Start(self):  # PQS.cs:591
        if not self.cache.created:
            self.cache.create()
        if not self.pool.ready:
            self.pool.create()
        if not self.isStarted:
            self.StartSphere(False)

    def StartSphere(self, force):  # PQS.cs:717
        if self.isStarted and not force:
            return
        self.normalUpdateList = []
        self.vb.buildQuad = None
        self.targetPositionPrev = (0.0, -1.0, 0.0)
        self.pqID = 0
        self.fixedUpdateFrame = 0
        if self.preset is not None:
            self.minDetailDistance, self.minLevel, self.maxLevel = self.preset
        self.minLevel = min(self.minLevel, self.maxLevel - 1)
        self.minLevel = max(self.minLevel, 2)
        self.maxLevel = max(self.maxLevel, self.minLevel + 1)
        self.maxLevel1 = self.maxLevel + 1
        self.maxLevelAtCurrentTgtSpeed = self.maxLevel
        self.subdivisionThresholds = [self.radius * self.minDetailDistance / f32(Pow(2.0, float(i))) * self.subdivisionThreshold
                                      for i in range(self.maxLevel1)]
        self.collapseThresholds = [0.0] * self.maxLevel1
        self.collapseDelta = (self.collapseAltitudeValue - self.collapseSeaLevelValue) / self.collapseAltitudeMax
        self.visRadDelta = (self.visRadAltitudeValue - self.visRadSeaLevelValue) / self.visRadAltitudeMax
        self.circumference = 6.2831854820251465 * self.radius
        self.radiusSquared = self.radius * self.radius
        self.SetupMods()
        self.SetupBuildDelegates(False)
        self.isActive = False
        self.isAlive = False
        self.isThinking = False
        self.isStarted = True
        self.isDisabled = False
        if force:
            self.isAlive = True
        else:
            self.Mod_OnSphereStart()
        self.UpdateVisual()
        if not self.isAlive or self.target is None:
            return
        self.UpdateQuadsInit()
        self.start_update_routine()

    def ResetSphere(self):  # PQS.cs:796
        if self.isAlive or self.isStarted:
            if self.quads is not None and len(self.quads) == 6:
                for i in range(6):
                    if self.quads[i] is not None:
                        self.quads[i].Destroy()
                        self.quads[i] = None
                self.quads = None
            for m in self.mods:
                m.OnSphereReset()
            self.normalUpdateList.clear()
            self.quadCount = 0
            self.isAlive = False
            self.isStarted = False
        # StopCoroutine("UpdateSphere"): no effect on a coroutine started from an IEnumerator

    def RebuildSphere(self):  # PQS.cs:832
        self.ResetSphere()
        self.StartSphere(False)

    def Mod_OnSphereStart(self):  # PQS.cs:1851
        n = 0
        for m in self.mods:
            if m.OnSphereStart():
                n += 1
        if n == 0:
            self.isAlive = True

    def start_update_routine(self):  # StartCoroutine(UpdateSphere()): the first pass runs inside StartCoroutine
        if not self.isAlive:
            return
        self.update_iteration()
        self.routines.append("update")

    def update_iteration(self):  # PQS.cs:1108-1117
        if not self.isDisabled:
            for m in self.mods:
                m.OnPreUpdate()
            self.UpdateVisual()
            if self.isAlive and self.isActive and self.target is not None:
                self.UpdateQuads()
                for m in self.mods:
                    m.OnUpdateFinished()

    def FrameUpdate(self):
        if self.isAlive and self.isStarted and self.isThinking:  # PQS.cs:1469
            self.RebuildSphere()
        cur, self.routines = self.routines, []
        for r in cur:
            if r == "update":
                if not self.isAlive:
                    continue
                self.update_iteration()
                self.routines.append("update")

    def FixedUpdate(self):  # PQS.cs:1462 (the transform pose is not used by the quad build)
        self.fixedUpdateFrame += 1
        for m in self.mods:
            m.OnSphereTransformUpdate()

    def ActivateSphere(self):  # PQS.cs:1183
        if self.isActive:
            return
        if not self.isAlive:
            if not self.isStarted:
                self.StartSphere(False)
                if not self.isAlive:
                    self.UpdateQuadsInit()
                    self.isAlive = True
            else:
                self.UpdateQuadsInit()
                self.isAlive = True
        self.isActive = True
        self.SetVisible(True)

    def DeactivateSphere(self):  # PQS.cs:1223
        if self.isActive:
            self.isActive = False
            for m in self.mods:
                m.OnSphereInactive()
            self.SetVisible(False)

    def SetVisible(self, visible):  # PQS.cs:1238
        for i in (5, 4, 3, 2, 1, 0):
            if visible:
                self.quads[i].SetMasterVisible()
            else:
                self.quads[i].SetMasterInvisible()

    def SetTarget(self, t):  # PQS.cs:1261
        self.target = t

    # ---- quads ---------------------------------------------------------------------------------------------------------
    def AssignQuad(self, subdiv):  # PQS.cs:991
        q = self.pool.get_quad()
        q.sphereRoot = self
        q.subdivision = subdiv
        q.subdivideThresholdFactor = 1.0
        q.quadRoot = None
        q.id = self.pqID
        self.pqID += 1
        q.isActive = True
        q.isForcedInvisible = False
        return q

    def QuadCreated(self, q):  # PQS.cs:1026
        pass

    def DestroyQuad(self, q):  # PQS.cs:1031
        for m in self.mods:
            m.OnQuadDestroy(q)
        self.pool.destroy_quad(q)

    def CreateQuads(self):  # PQS.cs:1055
        if self.quads is not None and len(self.quads) == 6:
            for q in self.quads:
                if q is not None:
                    q.Destroy()
        self.quads = [None] * 6
        for i in (5, 4, 3, 2, 1, 0):
            self.quads[i] = self.AssignQuad(0)
        xP, xN, yP, yN, zP, zN = self.quads
        self.setup_root(xP, self.name + " Xp", 0, zP, zN, yN, yP, (1.0, 0.0, 0.0))
        self.setup_root(xN, self.name + " Xn", 1, zP, zN, yP, yN, (-1.0, 0.0, 0.0))
        self.setup_root(yP, self.name + " Yp", 2, zP, zN, xP, xN, (0.0, 1.0, 0.0))
        self.setup_root(yN, self.name + " Yn", 3, zN, zP, xP, xN, (0.0, -1.0, 0.0))
        self.setup_root(zP, self.name + " Zp", 4, yN, yP, xP, xN, (0.0, 0.0, 1.0))
        self.setup_root(zN, self.name + " Zn", 5, yP, yN, xP, xN, (0.0, 0.0, -1.0))
        self.quadCount += 6
        for i in (5, 4, 3, 2, 1, 0):
            self.quads[i].SetupQuad(None, -1)

    def setup_root(self, q, name, plane, north, south, east, west, pos):  # PQS.cs:1086
        q.scalePlaneRelative = 1.0
        q.scalePlanetRelative = self.radius
        q.positionPlanePosition = dnormalized(pos)
        q.positionParentRelative = (0.0, 0.0, 0.0)
        q.positionPlanetRelative = q.positionPlanePosition
        q.positionPlanet = dmul(q.positionPlanetRelative, self.radius)
        q.plane = plane
        q.name = name
        q.north, q.south, q.east, q.west = north, south, east, west
        q.uvSW = (0.0, 0.0)
        q.uvDelta = (1.0, 1.0)

    def UpdateVisual(self):  # PQS.cs:1281
        if self.target is None:
            return
        self.relativeTargetPosition = self.target.position
        self.relativeTargetPositionNormalized = dnormalized(self.relativeTargetPosition)
        if self.fixedUpdateFrame != 0:
            self.fixedFrameTime = f32(self.now() - self.prevFixedFrameTime)  # two reads of the clock (PQS.cs:1292-1293)
            self.prevFixedFrameTime = self.now()
            self.fixedUpdateFrame = 0
            ft = self.fixedFrameTime
            dv = dsub(self.relativeTargetPosition, self.targetPositionPrev)
            if ft != 0.0:
                self.targetVelocity = (dv[0] / ft, dv[1] / ft, dv[2] / ft)
            else:
                self.targetVelocity = tuple((math.copysign(math.inf, c) if c != 0.0 else math.nan) for c in dv)
            self.targetSpeed = dmag(self.targetVelocity)
            self.targetPositionPrev = self.relativeTargetPosition
            a = f32(Acos(ddot(self.relativeTargetPositionNormalized, self.lastRelTgtPosNormalized)))
            self.lastRelTgtPosNormalized = self.relativeTargetPositionNormalized
            self.angularTargetSpeed = 0.0 if a != a else a
        if self.maxQuadLenghtsPerFrame > 0.0:
            self.maxLevelAtCurrentTgtSpeed = self.minLevel
            itr = self.maxLevel
            while itr >= self.minLevel:
                if self.angularTargetSpeed >= f32(1.5707963) / f32(Pow(2.0, float(itr))) * self.maxQuadLenghtsPerFrame:
                    itr -= 1
                    continue
                self.maxLevelAtCurrentTgtSpeed = itr
                break
        else:
            self.maxLevelAtCurrentTgtSpeed = self.maxLevel
        self.targetDistance = dmag(self.relativeTargetPosition)
        self.targetAltitude = self.targetDistance - self.radius
        self.visibleAltitude = self.targetAltitude
        self.relativeTargetPosition = dmul(self.relativeTargetPositionNormalized, self.radius)
        self.targetHeight = self.targetDistance - self.GetSurfaceHeight(self.relativeTargetPositionNormalized)
        num = self.radius + self.visibleAltitude
        self.horizonDistance = math.sqrt(num * num - self.radiusMin * self.radiusMin) + self.halfChord
        self.horizonAngle = Pow(Cos(self.radius / (self.radius + self.visibleAltitude)), -1.0)
        self.visRad = self.visRadSeaLevelValue + self.visibleAltitude * self.visRadDelta
        if self.visRad < self.visRadAltitudeValue:
            self.visRad = self.visRadAltitudeValue
        elif self.visRad > self.visRadSeaLevelValue:
            self.visRad = self.visRadSeaLevelValue
        self.visibleRadius = self.horizonDistance * Sin(self.horizonAngle) * self.visRad
        for i in range(self.maxLevel1):
            ct = self.collapseSeaLevelValue + self.targetAltitude * self.collapseDelta
            if ct < self.collapseSeaLevelValue:
                ct = self.collapseSeaLevelValue
            elif ct > self.collapseAltitudeValue:
                ct = self.collapseAltitudeValue
            self.collapseThreshold = ct
            self.collapseThresholds[i] = self.subdivisionThresholds[i] * ct
        if self.targetAltitude > self.maxDetailDistance * self.radius:
            if self.isSubdivisionEnabled:
                self.UpdateQuads()
                self.isSubdivisionEnabled = False
        elif not self.isSubdivisionEnabled:
            self.isSubdivisionEnabled = True

    def UpdateQuadsInit(self):  # PQS.cs:1388 (10 passes)
        self.CreateQuads()
        self.isThinking = True
        self.quadAllowBuild = False
        for _ in range(10):
            for i in (5, 4, 3, 2, 1, 0):
                self.quads[i].UpdateSubdivisionInit()
        self.quadAllowBuild = True
        for i in (5, 4, 3, 2, 1, 0):
            self.quads[i].UpdateSubdivision()
        self.isThinking = False

    def UpdateQuads(self):  # PQS.cs:1411
        if self.quads is None:
            return
        self.isThinking = True
        self.maxFrameEnd = f32(self.now() + self.maxFrameTime)
        q = self.quads
        for i in range(1, len(q)):
            p = q[i]
            j = i - 1
            while j >= 0 and not (dsqrmag(dsub(self.relativeTargetPosition, q[j].positionPlanetRelative))
                                  <= dsqrmag(dsub(self.relativeTargetPosition, p.positionPlanetRelative))):
                q[j + 1] = q[j]
                j -= 1
            q[j + 1] = p
        for j in range(min(len(q), 5)):
            q[j].UpdateSubdivision()
        if self.reqCustomNormals:
            self.UpdateEdges()
        self.isThinking = False

    def UpdateEdges(self):  # PQS.cs:1442
        while self.normalUpdateList:
            q = self.normalUpdateList.pop()
            if not q.isSubdivided and q.isVisible:
                self.UpdateEdgeNormals(q)
            q.isQueuedForNormalUpdate = False
            q.isQueuedOnlyForCornerNormalUpdate = False

    # ---- the quad build ------------------------------------------------------------------------------------------------
    def BuildQuad(self, q):  # PQS.cs:2237
        if q.isBuilt or q.isSubdivided:
            return False
        c = self.cache
        for m in self.mods:
            m.OnQuadPreBuild(q)
        d = self.vb
        d.buildQuad = q
        d.allowScatter = True
        d.gnomonicPlane = q.plane
        q.meshVertMin = DMAX
        q.meshVertMax = -DMAX
        provided = None
        if self.heights is not None:
            gvs = [mul_point3x4(q.quadMatrix, c.cacheVerts[i]) for i in range(c.vertCount)]
            provided = self.heights.build(self, q, gvs)
            q.exact = provided is not None
        q.buildKey = q.name
        for i in range(c.vertCount):
            self.vertexIndex = i
            d.globalV = mul_point3x4(q.quadMatrix, c.cacheVerts[i])
            d.directionFromCenter = dnormalized(d.globalV)
            d.vertHeight = self.radius
            d.vertIndex = i
            if self.heights is None:
                self.Mod_OnVertexBuildHeight(d)
            else:
                if self.reqVertexMapCoods:
                    self.BuildVertexMapCoords(d)
                if provided is not None:
                    d.vertHeight, d.vertColor = provided[i]
                else:
                    d.vertColor = (0.0, 0.0, 0.0, 1.0)
            self.Mod_OnVertexBuild(d)
            d.vertHeight -= self.radius
            if d.vertHeight > q.meshVertMax:
                q.meshVertMax = d.vertHeight
            if d.vertHeight < q.meshVertMin:
                q.meshVertMin = d.vertHeight
            if d.vertHeight > self.meshVertMax:
                self.meshVertMax = d.vertHeight
            if d.vertHeight < self.meshVertMin:
                self.meshVertMin = d.vertHeight
        self.vertexIndex = c.vertCount
        q.mesh.vertices = list(q.verts)
        q.mesh.triangleSet = 0
        q.edgeState = -1
        self.Mod_OnMeshBuild(q)
        for m in self.mods:
            m.OnQuadBuilt(q)
        self.any_built = True
        return True

    def Mod_OnVertexBuild(self, d):  # PQS.cs:1970
        for m in self.mods:
            m.OnVertexBuild(d)
        if self.isFakeBuild:
            return
        c = self.cache
        i = self.vertexIndex
        q = d.buildQuad
        if self.surfaceRelativeQuads:  # :2324 with an identity sphere transform (see the C++ BuildVertexSurfaceRelative)
            vr = dmul(d.directionFromCenter, d.vertHeight)
            vf = d2f(vr)
            c.verts[i] = vr
            o = d2f(q.meshOrigin)
            q.verts[i] = (f32(vf[0] - o[0]), f32(vf[1] - o[1]), f32(vf[2] - o[2]))
            q.vertsD[i] = vr
        else:  # :2318
            c.verts[i] = dmul(d.directionFromCenter, d.vertHeight)
            q.verts[i] = d2f(c.verts[i])
            q.vertsD[i] = c.verts[i]
        if not self.reqCustomNormals:  # :2355
            c.normals[i] = d2f(d.directionFromCenter)
        if self.reqColorChannel:  # :2332
            c.cacheColors[i] = d.vertColor
        if self.reqSphereUV:  # :2366
            c.uvs[i] = (f32(self.sx), f32(self.sy))
        if self.reqUVQuad:  # :2360
            c.uvs[i] = (f32(q.uvSW[0] + c.cacheUVs[i][0] * q.uvDelta[0]), f32(q.uvSW[1] + c.cacheUVs[i][1] * q.uvDelta[1]))
        if self.reqUV2:
            c.cacheUV2s[i] = (f32(d.u2), f32(d.v2))
        if self.reqUV3:
            c.cacheUV3s[i] = (f32(d.u3), f32(d.v3))
        if self.reqUV4:
            c.cacheUV4s[i] = (f32(d.u4), f32(d.v4))

    def Mod_OnMeshBuild(self, q):  # PQS.cs:2017
        c = self.cache
        if self.reqCustomNormals:  # :2377
            self.BuildNormals(q)
            self.BackupEdgeNormals(q)
            q.mesh.normals = list(q.vertNormals)
        else:  # :2372
            q.mesh.normals = list(c.normals)
        q.edgeExact = q.exact
        if self.reqSphereUV or self.reqUVQuad:
            q.mesh.uv = list(c.cacheUVs)
        if self.reqUV2:
            q.mesh.uv2 = list(c.cacheUV2s)
        if self.reqUV3:
            q.mesh.uv3 = list(c.cacheUV3s)
        if self.reqUV4:
            q.mesh.uv4 = list(c.cacheUV4s)
        if self.reqColorChannel:
            q.mesh.colors = list(c.cacheColors)
        if self.reqBuildTangents:
            self.BuildTangents(q)
        if self.reqAssignTangents:
            q.mesh.tangents = list(c.cacheTangents)

    def BuildNormals(self, q):  # PQS.cs:1760
        c = self.cache
        idx = c.cacheIndices[0]
        V = c.verts
        for t in range(c.triCount):
            a, b, e = idx[3 * t], idx[3 * t + 1], idx[3 * t + 2]
            va, vb, vc = V[a], V[b], V[e]
            v1 = (f32(vb[0] - va[0]), f32(vb[1] - va[1]), f32(vb[2] - va[2]))
            v2 = (f32(vc[0] - va[0]), f32(vc[1] - va[1]), f32(vc[2] - va[2]))
            n = (f32(v1[1] * v2[2] - v1[2] * v2[1]), f32(v1[2] * v2[0] - v1[0] * v2[2]), f32(v1[0] * v2[1] - v1[1] * v2[0]))
            c.triNormals[t] = v3normalized(n)
        vn = [V3ZERO] * c.vertCount
        for t in range(c.triCount):
            tn = c.triNormals[t]
            for k in (idx[3 * t], idx[3 * t + 1], idx[3 * t + 2]):
                vn[k] = v3add(vn[k], tn)
        q.vertNormals = [v3normalized(x) for x in vn]

    def BuildTangents(self, q):  # PQS.cs:1619
        c = self.cache
        for i in range(c.vertCount):
            u, tg = native_orthonormalize(q.mesh.normals[i], V3ZERO)
            w = -1.0 if v3dot(v3cross(u, tg), c.tan2[i]) < 0.0 else 1.0
            c.cacheTangents[i] = (tg[0], tg[1], tg[2], w)
        q.mesh.tangents = list(c.cacheTangents)

    def BackupEdgeNormals(self, q):  # PQS.cs:2471 / :2493
        c = self.cache
        side, res = c.side, c.res
        for e, (start, delta) in enumerate(((c.vi(0, 0), 1), (c.vi(res, res), -1), (c.vi(0, res), -side), (c.vi(res, 0), side))):
            k = start
            row = []
            for _ in range(side):
                row.append(q.vertNormals[k])
                k += delta
            q.edgeNormals[e] = row

    def CombineEdgeNormals(self, q, vCount, vertStart, vertStep, local, localStart, localStep, remote, remoteStart,
                           remoteStep):  # PQS.cs:2951
        a, b, r = vertStart, localStart, remoteStart
        for _ in range(vCount):
            q.vertNormals[a] = v3mul(v3add(local[b], remote[r]), f32(0.5))
            a += vertStep
            b += localStep
            r += remoteStep

    def GetRightmostCornerNormal(self, caller, nextQuad, reads):  # PQS.cs:2967
        if caller is None or nextQuad is None:
            return V3ZERO
        cq = caller.GetRightmostCornerPQ(nextQuad)
        if cq is None:
            return V3ZERO
        if nextQuad.subdivision < cq.subdivision:
            e = cq.parent.GetEdge(nextQuad)
        elif nextQuad.subdivision <= cq.subdivision:
            e = cq.GetEdge(nextQuad)
        else:
            e = cq.GetEdge(nextQuad.parent)
        if not cq.isBuilt:
            cq.Build()
        if e == QE_NULL:
            return V3ZERO
        reads.append(cq)
        return cq.edgeNormals[e][self.cache.res]

    def UpdateEdgeNormals(self, q):  # PQS.cs:2503
        if not q.isActive or (q.parent is not None and not q.parent.isSubdivided) or q.parent is None or not q.isBuilt:
            return
        c = self.cache
        res, half, half1, side = c.res, c.resDiv2, c.resDiv2Plus1, c.side
        vi = c.vi
        reads = []
        flag = not q.isQueuedOnlyForCornerNormalUpdate
        v1 = q.edgeNormals[0][0]
        v2 = q.edgeNormals[0][res]
        v3 = q.edgeNormals[1][0]
        v4 = q.edgeNormals[1][res]
        f2 = f3 = f4 = f5 = False
        CEN = self.CombineEdgeNormals

        def EN(p, e):
            reads.append(p)
            return p.edgeNormals[e]

        # ---- north
        n = q.north
        if n.subdivision == q.subdivision:
            if n.isSubdivided:
                left, right = n.GetEdgeQuads(q)
                if left is not None and right is not None:
                    e1 = left.GetEdge(q)
                    e2 = right.GetEdge(q)
                    if e1 == QE_NULL or e2 == QE_NULL:
                        return
                    if not left.isBuilt:
                        left.Build()
                    if not right.isBuilt:
                        right.Build()
                    zero = q.edgeNormals[0][half]
                    if right.isBuilt:
                        if flag:
                            CEN(q, half1, vi(0, 0), 1, q.edgeNormals[0], 0, 1, EN(right, e2), res, -2)
                        v1 = v3add(v1, EN(right, e2)[res])
                        zero = v3add(zero, EN(right, e2)[0])
                    if left.isBuilt:
                        if flag:
                            CEN(q, half1, vi(half, 0), 1, q.edgeNormals[0], half, 1, EN(left, e1), res, -2)
                        v2 = v3add(v2, EN(left, e1)[0])
                        zero = v3add(zero, EN(left, e1)[res])
                    q.vertNormals[vi(half, 0)] = v3normalized(zero)
            else:
                e1 = n.GetEdge(q)
                if e1 == QE_NULL:
                    return
                if not n.isBuilt:
                    n.Build()
                if n.isBuilt:
                    if flag:
                        CEN(q, side, vi(0, 0), 1, q.edgeNormals[0], 0, 1, EN(n, e1), res, -1)
                    v1 = v3add(v1, EN(n, e1)[res])
                    v2 = v3add(v2, EN(n, e1)[0])
        elif n.subdivision < q.subdivision:
            left, right = q.parent.GetEdgeQuads(n)
            e1 = n.GetEdge(q.parent)
            if e1 == QE_NULL:
                return
            if not n.isBuilt:
                n.Build()
            if n.isBuilt:
                if left is q:
                    if flag:
                        CEN(q, half1, vi(0, 0), 2, q.edgeNormals[0], 0, 2, EN(n, e1), res, -1)
                    v1 = v3add(v1, EN(n, e1)[res])
                    v2 = v3add(v2, EN(n, e1)[half])
                    f3 = True
                elif right is q:
                    if flag:
                        CEN(q, half1, vi(0, 0), 2, q.edgeNormals[0], 0, 2, EN(n, e1), half, -1)
                    v1 = v3add(v1, EN(n, e1)[half])
                    v2 = v3add(v2, EN(n, e1)[0])
                    f2 = True
        # ---- south
        s = q.south
        if s.subdivision == q.subdivision:
            if s.isSubdivided:
                left, right = s.GetEdgeQuads(q)
                if left is not None and right is not None:
                    e1 = left.GetEdge(q)
                    e2 = right.GetEdge(q)
                    if e1 == QE_NULL or e2 == QE_NULL:
                        return
                    if not left.isBuilt:
                        left.Build()
                    if not right.isBuilt:
                        right.Build()
                    zero = q.edgeNormals[1][half]
                    if right.isBuilt:
                        if flag:
                            CEN(q, half1, vi(res, res), -1, q.edgeNormals[1], 0, 1, EN(right, e2), res, -2)
                        v3 = v3add(v3, EN(right, e2)[res])
                        zero = v3add(zero, EN(right, e2)[0])
                    if left.isBuilt:
                        if flag:
                            CEN(q, half1, vi(half, res), -1, q.edgeNormals[1], half, 1, EN(left, e1), res, -2)
                        v4 = v3add(v4, EN(left, e1)[0])
                        zero = v3add(zero, EN(left, e1)[res])
                    q.vertNormals[vi(half, res)] = v3normalized(zero)
            else:
                e1 = s.GetEdge(q)
                if e1 == QE_NULL:
                    return
                if not s.isBuilt:
                    s.Build()
                if s.isBuilt:
                    if flag:
                        CEN(q, side, vi(res, res), -1, q.edgeNormals[1], 0, 1, EN(s, e1), res, -1)
                    v3 = v3add(v3, EN(s, e1)[res])
                    v4 = v3add(v4, EN(s, e1)[0])
        elif s.subdivision < q.subdivision:
            left, right = q.parent.GetEdgeQuads(s)
            e1 = s.GetEdge(q.parent)
            if e1 == QE_NULL:
                return
            if not s.isBuilt:
                s.Build()
            if s.isBuilt:
                if left is q:
                    if flag:
                        CEN(q, half1, vi(res, res), -2, q.edgeNormals[1], 0, 2, EN(s, e1), res, -1)
                    v3 = v3add(v3, EN(s, e1)[res])
                    v4 = v3add(v4, EN(s, e1)[half])
                    f5 = True
                elif right is q:
                    if flag:
                        CEN(q, half1, vi(res, res), -2, q.edgeNormals[1], 0, 2, EN(s, e1), half, -1)
                    v3 = v3add(v3, EN(s, e1)[half])
                    v4 = v3add(v4, EN(s, e1)[0])
                    f4 = True
        # ---- east
        e = q.east
        if e.subdivision == q.subdivision:
            if e.isSubdivided:
                left, right = e.GetEdgeQuads(q)
                if left is not None and right is not None:
                    e1 = left.GetEdge(q)
                    e2 = right.GetEdge(q)
                    if e1 == QE_NULL or e2 == QE_NULL:
                        return
                    if not left.isBuilt:
                        left.Build()
                    if not right.isBuilt:
                        right.Build()
                    zero = q.edgeNormals[2][half]
                    if right.isBuilt:
                        if flag:
                            CEN(q, half1, vi(0, res), -side, q.edgeNormals[2], 0, 1, EN(right, e2), res, -2)
                        v4 = v3add(v4, EN(right, e2)[res])
                        zero = v3add(zero, EN(right, e2)[0])
                    if left.isBuilt:
                        if flag:
                            CEN(q, half1, vi(0, half), -side, q.edgeNormals[2], half, 1, EN(left, e1), res, -2)
                        v1 = v3add(v1, EN(left, e1)[0])
                        zero = v3add(zero, EN(left, e1)[res])
                    q.vertNormals[vi(0, half)] = v3normalized(zero)
            else:
                e1 = e.GetEdge(q)
                if e1 == QE_NULL:
                    return
                if not e.isBuilt:
                    e.Build()
                if e.isBuilt:
                    if flag:
                        CEN(q, side, vi(0, res), -side, q.edgeNormals[2], 0, 1, EN(e, e1), res, -1)
                    v1 = v3add(v1, EN(e, e1)[0])
                    v4 = v3add(v4, EN(e, e1)[res])
        elif e.subdivision < q.subdivision:
            left, right = q.parent.GetEdgeQuads(e)
            e1 = e.GetEdge(q.parent)
            if e1 == QE_NULL:
                return
            if not e.isBuilt:
                e.Build()
            if e.isBuilt:
                if right is q:
                    if flag:
                        CEN(q, half1, vi(0, res), -(side * 2), q.edgeNormals[2], 0, 2, EN(e, e1), half, -1)
                    v1 = v3add(v1, EN(e, e1)[0])
                    v4 = v3add(v4, EN(e, e1)[half])
                    f5 = True
                elif left is q:
                    if flag:
                        CEN(q, half1, vi(0, res), -(side * 2), q.edgeNormals[2], 0, 2, EN(e, e1), res, -1)
                    v1 = v3add(v1, EN(e, e1)[half])
                    v4 = v3add(v4, EN(e, e1)[res])
                    f2 = True
        # ---- west
        w = q.west
        if w.subdivision == q.subdivision:
            if w.isSubdivided:
                left, right = w.GetEdgeQuads(q)
                if left is not None and right is not None:
                    e1 = left.GetEdge(q)
                    e2 = right.GetEdge(q)
                    if e1 == QE_NULL or e2 == QE_NULL:
                        return
                    if not left.isBuilt:
                        left.Build()
                    if not right.isBuilt:
                        right.Build()
                    zero = q.edgeNormals[3][half]
                    if right.isBuilt:
                        if flag:
                            CEN(q, half1, vi(res, 0), side, q.edgeNormals[3], 0, 1, EN(right, e2), res, -2)
                        v2 = v3add(v2, EN(right, e2)[res])
                        zero = v3add(zero, EN(right, e2)[0])
                    if left.isBuilt:
                        if flag:
                            CEN(q, half1, vi(res, half), side, q.edgeNormals[3], half, 1, EN(left, e1), res, -2)
                        v3 = v3add(v3, EN(left, e1)[0])
                        zero = v3add(zero, EN(left, e1)[res])
                    q.vertNormals[vi(res, half)] = v3normalized(zero)
            else:
                e1 = w.GetEdge(q)
                if e1 == QE_NULL:
                    return
                if not w.isBuilt:
                    w.Build()
                if w.isBuilt:
                    if flag:
                        CEN(q, side, vi(res, 0), side, q.edgeNormals[3], 0, 1, EN(w, e1), res, -1)
                    v2 = v3add(v2, EN(w, e1)[res])
                    v3 = v3add(v3, EN(w, e1)[0])
        elif w.subdivision < q.subdivision:
            left, right = q.parent.GetEdgeQuads(w)
            e1 = w.GetEdge(q.parent)
            if not w.isBuilt:
                w.Build()
            if w.isBuilt:
                if left is q:
                    if flag:
                        CEN(q, half1, vi(res, 0), side * 2, q.edgeNormals[3], 0, 2, EN(w, e1), res, -1)
                    v2 = v3add(v2, EN(w, e1)[res])
                    v3 = v3add(v3, EN(w, e1)[half])
                    f4 = True
                elif right is q:
                    if flag:
                        CEN(q, half1, vi(res, 0), side * 2, q.edgeNormals[3], 0, 2, EN(w, e1), half, -1)
                    v2 = v3add(v2, EN(w, e1)[half])
                    v3 = v3add(v3, EN(w, e1)[0])
                    f3 = True
        if not f2:
            v1 = v3add(v1, self.GetRightmostCornerNormal(q, q.north, reads))
        if not f3:
            v2 = v3add(v2, self.GetRightmostCornerNormal(q, q.west, reads))
        if not f4:
            v3 = v3add(v3, self.GetRightmostCornerNormal(q, q.south, reads))
        if not f5:
            v4 = v3add(v4, self.GetRightmostCornerNormal(q, q.east, reads))
        q.vertNormals[vi(0, 0)] = v3normalized(v1)
        q.vertNormals[vi(res, 0)] = v3normalized(v2)
        q.vertNormals[vi(res, res)] = v3normalized(v3)
        q.vertNormals[vi(0, res)] = v3normalized(v4)
        q.mesh.normals = list(q.vertNormals)
        # exactness bookkeeping (real bodies): the stitched normals are exact when every source was
        q.exact = q.exact and all(getattr(p, "edgeExact", False) for p in reads)
        if self.heights is not None:
            self.heights.record_reads(q, reads)
        for m in self.mods:
            m.OnQuadUpdateNormals(q)
        if self.reqBuildTangents:
            self.BuildTangents(q)
