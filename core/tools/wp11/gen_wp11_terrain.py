"""WP-11 code generator: the stock PQS mod stacks (from the game's prefabs) and the LibNoise gradient table as C++.

    python -I tools/wp11/gen_wp11_terrain.py [core dir]          (run in core/; regenerate, never hand-edit the outputs)

Inputs (read-only):
  data/terrain/pqs_prefab.json, data/terrain/manifest.json   (tools/wp11/extract_terrain.py, from KSP_x64_Data)
  ../decompiled/KSP-Decompiled/Assembly-CSharp/**.cs        (RandomVectors initialiser; OnSetup requirements of the
                                                             mod classes the port keeps as PQSModPassive)
  ../harness/captures/out2/terrain_height.json (optional)    cross-check: requirements and stack order vs the game
Outputs:
  src/terrain/libnoise_random_vectors.inc   GradientNoiseBasis.RandomVectors (1024 doubles, the C# literals verbatim)
  src/terrain/pqs_prefab_data.cpp           one factory per body with a PQS; every serialized field of every height
                                            mod; the map table (origin, header, size, sha256)

Every JSON field of a height mod must be consumed or listed in IGNORED (with the reason) or generation fails, so the
C++ data cannot silently drop a prefab value. tests/test_terrain.cpp re-reads pqs_prefab.json and compares every
consumed field with the generated objects bit for bit.
"""
import json
import os
import re
import struct
import sys

# ---------------------------------------------------------------------------------------------------------------------
# literals (same rules as tools/gen_wp09_data.py)


def f32(v):
    return struct.unpack("<f", struct.pack("<f", v))[0]


def special(v):
    if isinstance(v, str):
        return {"Infinity": float("inf"), "-Infinity": float("-inf"), "NaN": float("nan")}[v]
    return float(v)


def lit_double(v):
    v = special(v)
    if v != v:
        return "std::numeric_limits<double>::quiet_NaN()"
    if v in (float("inf"), float("-inf")):
        return ("-" if v < 0 else "") + "std::numeric_limits<double>::infinity()"
    s = repr(v)
    if "e" not in s and "." not in s:
        s += ".0"
    return s


def lit_float(v):
    v = special(v)
    if v != v:
        return "std::numeric_limits<float>::quiet_NaN()"
    if v in (float("inf"), float("-inf")):
        return ("-" if v < 0 else "") + "std::numeric_limits<float>::infinity()"
    assert f32(v) == v, v
    if v == 0.0:
        return "-0.0f" if str(v).startswith("-") else "0.0f"
    s = None
    for p in range(6, 10):
        s = "%.*g" % (p, v)
        if f32(float(s)) == v:
            break
    if "e" not in s and "." not in s:
        s += ".0"
    return s + "f"


def lit_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def lit_bool(v):
    return "true" if v else "false"


def lit_int(v):
    v = int(v)
    if v == -2147483648:
        return "(-2147483647 - 1)"
    return str(v)


# ---------------------------------------------------------------------------------------------------------------------
# C# source scanning

REQ = {"Default": 0, "UniqueMaterialInstances": 1, "VertexMapCoords": 2, "VertexGnomonicMapCoords": 4, "UVSphereCoords": 8,
       "UVQuadCoords": 0x10, "MeshColorChannel": 0x20, "MeshCustomNormals": 0x40, "MeshBuildTangents": 0x80,
       "MeshUV2": 0x100, "MeshUV3": 0x200, "MeshUV4": 0x400, "MeshAssignTangents": 0x800}


class CSharp:
    def __init__(self, cs_dir):
        self.cs_dir = cs_dir
        self.files = {}
        self.base = {}
        for root, _, fs in os.walk(cs_dir):
            for f in fs:
                if f.endswith(".cs"):
                    p = os.path.join(root, f)
                    s = open(p, encoding="utf-8", errors="replace").read()
                    for m in re.finditer(r"\bclass\s+(\w+)\s*:\s*([\w\.]+)", s):
                        self.base.setdefault(m.group(1), m.group(2).split(".")[-1])
                        self.files.setdefault(m.group(1), (p, s))

    def method_body(self, cls, sig_regex):
        if cls not in self.files:
            return None
        _, s = self.files[cls]
        m = re.search(r"\n(\t+)(?:public|protected|private|internal)[^\n]*" + sig_regex + r"[^\n]*\n\1\{\n(.*?)\n\1\}", s, re.S)
        return m.group(2) if m else None

    def setup_requirements(self, cls):
        """requirements after OnSetup for a class kept as PQSModPassive: the OR of every `requirements = X` in its
        OnSetup override; 0 when no class in the chain overrides OnSetup or the override calls base.OnSetup() without
        assigning; -1 when the override leaves the serialized value."""
        c = cls
        while c and c != "PQSMod":
            body = self.method_body(c, r"override void OnSetup\(\)")
            if body is not None:
                # top-level assignments only: PQSMod_TextureAtlas sets MeshUV3 under CanTextureAtlasModBeUsed()
                # (graphics settings; off in the capture, no effect on height), so conditional ones are ignored
                assigns = re.findall(r"^		requirements\s*=\s*([^;]+);", body, re.M)
                if assigns:
                    v = 0
                    for a in assigns:
                        for name in re.findall(r"ModiferRequirements\.(\w+)", a):
                            v |= REQ[name]
                    return v
                return 0 if "base.OnSetup()" in body else -1
            c = self.base.get(c)
        return 0

    def random_vectors(self):
        p, s = self.files["GradientNoiseBasis"]
        m = re.search(r"RandomVectors = new double\[1024\]\s*\{(.*?)\};", s, re.S)
        vals = [x.strip() for x in m.group(1).replace("\n", " ").split(",") if x.strip()]
        assert len(vals) == 1024, len(vals)
        for v in vals:
            assert re.fullmatch(r"-?\d+\.\d+(E-?\d+)?", v), v
        return vals


# ---------------------------------------------------------------------------------------------------------------------
# field specs: JSON field -> (kind, C++ member)

def S(*pairs):
    return [(p[0], p[1], p[2] if len(p) > 2 else p[0]) for p in pairs]


NOISE = S(("noiseType", "enum:PQSNoiseType"), ("deformity", "f"), ("seed", "i"), ("frequency", "f"), ("lacunarity", "f"),
          ("persistance", "f"), ("octaves", "i"), ("mode", "enum:LibNoise::NoiseQuality"))
SIMPLEX5 = S(("seed", "i"), ("deformity", "d"), ("octaves", "d"), ("persistence", "d"), ("frequency", "d"))
DECAL = S(("radius", "d"), ("position", "v3"), ("angle", "f"), ("heightMap", "map"), ("heightMapDeformity", "d"),
          ("cullBlack", "b"), ("useAlphaHeightSmoothing", "b"), ("absolute", "b"), ("colorMap", "mapkey", "colorMapKey"),
          ("smoothHeight", "f"), ("smoothColor", "f"), ("removeScatter", "b"), ("DEBUG_HighlightInclusion", "b"))

SPECS = {
    "PQSMod_VertexHeightMap": S(("heightMap", "map"), ("heightMapDeformity", "d"), ("heightMapOffset", "d"),
                                ("scaleDeformityByRadius", "b")),
    "PQSMod_VertexHeightMapStep": S(("heightMap", "texture", "heightMapTexture"), ("heightMapDeformity", "d"),
                                    ("heightMapOffset", "d"), ("scaleDeformityByRadius", "b"), ("coastHeight", "d")),
    "PQSMod_VertexSimplexHeight": SIMPLEX5,
    "PQSMod_VertexSimplexHeightAbsolute": SIMPLEX5,
    "PQSMod_VertexSimplexHeightFlatten": S(("seed", "i"), ("cutoff", "d"), ("deformity", "d"), ("octaves", "d"),
                                           ("persistence", "d"), ("frequency", "d")),
    "PQSMod_VertexSimplexHeightMap": SIMPLEX5 + S(("heightMap", "map"), ("heightStart", "f"), ("heightEnd", "f")),
    "PQSMod_VertexHeightNoise": NOISE,
    "PQSMod_VertexHeightNoiseVertHeight": NOISE + S(("heightStart", "f"), ("heightEnd", "f")),
    "PQSMod_VertexHeightNoiseHeightMap": NOISE + S(("heightMap", "texture", "heightMapTexture"), ("heightStart", "f"),
                                                   ("heightEnd", "f")),
    "PQSMod_VertexHeightNoiseVertHeightCurve": NOISE + S(("curve", "curve"), ("heightStart", "f"), ("heightEnd", "f")),
    "PQSMod_VertexHeightNoiseVertHeightCurve2": S(
        ("deformity", "f"), ("ridgedAddSeed", "i"), ("ridgedAddFrequency", "f"), ("ridgedAddLacunarity", "f"),
        ("ridgedAddOctaves", "i"), ("ridgedSubSeed", "i"), ("ridgedSubFrequency", "f"), ("ridgedSubLacunarity", "f"),
        ("ridgedSubOctaves", "i"), ("ridgedMode", "enum:LibNoise::NoiseQuality"), ("simplexCurve", "curve"),
        ("simplexHeightStart", "d"), ("simplexHeightEnd", "d"), ("simplexSeed", "i"), ("simplexOctaves", "d"),
        ("simplexPersistence", "d"), ("simplexFrequency", "d")),
    "PQSMod_VertexHeightNoiseVertHeightCurve3": S(
        ("inputHeightStart", "d"), ("inputHeightEnd", "d"), ("inputHeightCurve", "curve"),
        ("curveMultiplier", "simplexnoise"), ("deformityMin", "d"), ("deformityMax", "d"), ("deformity", "simplexnoise"),
        ("ridgedAdd", "ridgednoise"), ("ridgedSub", "ridgednoise")),
    "PQSMod_VertexRidgedAltitudeCurve": S(
        ("deformity", "f"), ("ridgedAddSeed", "i"), ("ridgedAddFrequency", "f"), ("ridgedAddLacunarity", "f"),
        ("ridgedAddOctaves", "i"), ("ridgedMinimum", "f"), ("ridgedMode", "enum:LibNoise::NoiseQuality"),
        ("simplexCurve", "curve"), ("simplexHeightStart", "d"), ("simplexHeightEnd", "d"), ("simplexSeed", "i"),
        ("simplexOctaves", "d"), ("simplexPersistence", "d"), ("simplexFrequency", "d")),
    "PQSMod_VertexNoise": S(("seed", "i"), ("noiseDeformity", "f"), ("noisePasses", "i"), ("smoothness", "f"),
                            ("falloff", "f"), ("mesaVsPlainsBias", "f"), ("plainsVsMountainSmoothness", "f"),
                            ("plainsVsMountainThreshold", "f"), ("plainSmoothness", "f")),
    "PQSMod_VertexVoronoi": S(("deformation", "d"), ("voronoiSeed", "i"), ("voronoiDisplacement", "d"),
                              ("voronoiFrequency", "d"), ("voronoiEnableDistance", "b")),
    "PQSMod_VoronoiCraters": S(
        ("deformation", "d"), ("voronoiSeed", "i"), ("voronoiDisplacement", "d"), ("voronoiFrequency", "d"),
        ("craterCurve", "curve"), ("jitterCurve", "curve"), ("simplexSeed", "i"), ("simplexOctaves", "d"),
        ("simplexPersistence", "d"), ("simplexFrequency", "d"), ("jitter", "f"), ("jitterHeight", "f"),
        ("rFactor", "f"), ("rOffset", "f"), ("colorOpacity", "f"), ("DebugColorMapping", "b")),
    "PQSMod_VoronoiCraters2": S(
        ("deformation", "d"), ("voronoiSeed", "i"), ("voronoiFrequency", "d"), ("craterCurve", "curve"),
        ("jitterSeed", "i"), ("jitterOctaves", "d"), ("jitterPersistence", "d"), ("jitterFrequency", "d"),
        ("jitter", "d"), ("deformationSeed", "i"), ("deformationOctaves", "d"), ("deformationPersistence", "d"),
        ("deformationFrequency", "d"), ("DebugColorMapping", "b")),
    "PQSMod_FlattenArea": S(("outerRadius", "d"), ("innerRadius", "d"), ("position", "v3"), ("flattenTo", "d"),
                            ("removeScatter", "b"), ("DEBUG_showColors", "b"), ("smoothStart", "d"), ("smoothEnd", "d"),
                            ("useLatLon", "b"), ("bodyName", "s"), ("lat", "d"), ("lon", "d"), ("alt", "d")),
    "PQSMod_FlattenAreaTangential": S(("outerRadius", "d"), ("innerRadius", "d"), ("position", "v3"),
                                      ("flattenTo", "d"), ("smoothStart", "d"), ("smoothEnd", "d"),
                                      ("DEBUG_showColors", "b")),
    "PQSMod_FlattenOcean": S(("oceanRadius", "d")),
    "PQSMod_MapDecal": DECAL + S(("absoluteOffset", "f")),
    "PQSMod_MapDecalTangent": DECAL + S(("absoluteOffset", "d")),
    "PQSMod_SmoothLatitudeRange": S(("latitudeRange", "lerp"), ("smoothToAltitude", "d")),
    "PQSMod_VertexDefineCoastLine": S(("oceanRadiusOffset", "d"), ("depthOffset", "d")),
    "PQSMod_VertexHeightOffset": S(("offset", "d")),
    "PQSMod_VertexHeightOblate": S(("height", "d"), ("pow", "d")),
    "PQSLandControl": "custom",
    "PQSMod_VertexPlanet": "custom",
}
# JSON fields of height mods that this port does not consume (colour output, scatter objects, object references)
IGNORED = {
    "*": {"sphere": "PQS back-reference (set by SetupMods)"},
    "PQSMod_VoronoiCraters": {"craterColourRamp": "Gradient (colour output, OnVertexBuild)"},
    "PQSMod_VoronoiCraters2": {"craterColourRamp": "Gradient (colour output, OnVertexBuild)"},
    "PQSMod_FlattenArea": {"body": "CelestialBody reference (unused by the height path)"},
    "PQSLandControl": {"scatters": "LandClassScatter objects (scatter, materials, meshes)"},
}
BASE_FIELDS = ("requirements", "modEnabled", "order", "modExpansionDisabled")
LANDCLASS_SCALARS = [("landClassName", "s"), ("color", "color"), ("noiseColor", "color"), ("altitudeRange", "lerp"),
                     ("latitudeRange", "lerp"), ("latitudeDouble", "b"), ("latitudeDoubleRange", "lerp"),
                     ("longitudeRange", "lerp"), ("coverageBlend", "f"), ("coverageSeed", "i"),
                     ("coverageOctaves", "i"), ("coveragePersistance", "f"), ("coverageFrequency", "f"),
                     ("noiseBlend", "f"), ("noiseSeed", "i"), ("noiseOctaves", "i"), ("noisePersistance", "f"),
                     ("noiseFrequency", "f"), ("minimumRealHeight", "d"), ("alterRealHeight", "d"),
                     ("alterApparentHeight", "f"), ("altDelta", "d"), ("latDelta", "d"), ("lonDelta", "d"),
                     ("delta", "d")]
LANDCONTROL_SCALARS = [("useHeightMap", "b"), ("heightMap", "map"), ("vHeightMax", "f"), ("createColors", "b"),
                       ("createScatter", "b")] + [
    (p + s, k) for p in ("altitude", "latitude", "longitude")
    for s, k in (("Blend", "f"), ("Seed", "i"), ("Octaves", "i"), ("Persistance", "f"), ("Frequency", "f"))]
PLANET_SCALARS = [("seed", "i"), ("deformity", "d"), ("colorDeformity", "d"), ("oceanLevel", "d"), ("oceanStep", "d"),
                  ("oceanDepth", "d"), ("oceanSnap", "b"), ("terrainSmoothing", "d"), ("terrainShapeStart", "d"),
                  ("terrainShapeEnd", "d"), ("terrainRidgesMin", "d"), ("terrainRidgesMax", "d"),
                  ("buildHeightColors", "b"), ("terrainRidgeBalance", "d")]
PLANET_WRAPPERS = ["continental", "continentalRuggedness", "continentalSharpnessMap", "terrainType"]
WRAPPER_FIELDS = [("deformity", "d"), ("octaves", "d"), ("persistance", "d"), ("frequency", "d")]


class Gen:
    def __init__(self, cs):
        self.cs = cs
        self.consumed = 0

    def value(self, kind, v):
        if kind == "d":
            return lit_double(v)
        if kind == "f":
            return lit_float(v)
        if kind == "i":
            return lit_int(v)
        if kind == "b":
            return lit_bool(v)
        if kind == "s":
            return lit_str(v or "")
        if kind.startswith("enum:"):
            return "static_cast<%s>(%d)" % (kind[5:], int(v))
        if kind == "v3":
            return "Vector3f(%s, %s, %s)" % (lit_float(v["x"]), lit_float(v["y"]), lit_float(v["z"]))
        if kind == "color":
            return "Color(%s, %s, %s, %s)" % (lit_float(v["r"]), lit_float(v["g"]), lit_float(v["b"]), lit_float(v["a"]))
        if kind == "curve":
            keys = []
            for k in v["m_Curve"]:
                keys.append("K(%s, %s, %s, %s, %d, %s, %s)" % (
                    lit_float(k["time"]), lit_float(k["value"]), lit_float(k["inSlope"]), lit_float(k["outSlope"]),
                    k.get("weightedMode", 0), lit_float(k.get("inWeight", 0.0)), lit_float(k.get("outWeight", 0.0))))
            return "Curve({%s}, %d, %d)" % (", ".join(keys), v.get("m_PreInfinity", 2), v.get("m_PostInfinity", 2))
        if kind == "map":
            return "maps.Get(%s)" % lit_str(v["map"]) if v else "nullptr"
        if kind == "mapkey":
            return lit_str(v["map"]) if v else '""'
        if kind == "texture":
            return lit_str(v.get("name", "")) if v else '""'
        raise ValueError(kind)

    def assign(self, out, target, kind, v, ind):
        if kind == "lerp":
            for f in ("startStart", "startEnd", "endStart", "endEnd", "startDelta", "endDelta"):
                out.append("%s%s.%s = %s;" % (ind, target, f, lit_double(v[f])))
                self.consumed += 1
            return
        if kind == "simplexnoise":
            for f, k in (("seed", "i"), ("frequency", "d"), ("persistence", "d"), ("octaves", "i")):
                out.append("%s%s.%s = %s;" % (ind, target, f, self.value(k, v[f])))
                self.consumed += 1
            assert set(v) == {"seed", "frequency", "persistence", "octaves"}, v.keys()
            return
        if kind == "ridgednoise":
            for f, k in (("seed", "i"), ("frequency", "d"), ("lacunarity", "d"), ("octaves", "i"),
                         ("quality", "enum:LibNoise::NoiseQuality")):
                out.append("%s%s.%s = %s;" % (ind, target, f, self.value(k, v[f])))
                self.consumed += 1
            assert set(v) == {"seed", "frequency", "lacunarity", "octaves", "quality"}, v.keys()
            return
        out.append("%s%s = %s;" % (ind, target, self.value(kind, v)))
        self.consumed += 1

    def mod(self, body, m, out):
        c = m["class"]
        f = m["fields"]
        ind = "        "
        spec = SPECS.get(c) if m["height_mod"] else None
        if m["height_mod"] and spec is None:
            raise SystemExit("height mod class without a spec: " + c)
        cpp = c if spec is not None else "PQSModPassive"
        out.append("    {  // [%d] %s on %s (PathID %d)" % (m["index"], c, m["gameObject"], m["pathID"]))
        out.append("        auto m = std::make_unique<%s>();" % cpp)
        out.append("        Base(*m, %s, %s, %s, %s, %s, %s, %s);" % (
            lit_str(c), lit_str(m["gameObject"]), lit_bool(m["activeSelf"]), lit_int(f["requirements"]),
            lit_bool(f["modEnabled"]), lit_int(f["order"]), lit_bool(f["modExpansionDisabled"])))
        self.consumed += len(BASE_FIELDS)
        if spec is None:
            req = self.cs.setup_requirements(c)
            out.append("        m->setupRequirements = %d;  // %s" % (
                req, "OnSetup leaves the serialized value" if req < 0 else "OnSetup result"))
            if c == "PQSROCControl":
                out.append("        m->isROCControl = true;")
                out.append("        m->modExpansionDisabled = !options.serenityInstalled;  // PQSROCControl.Awake")
        else:
            ign = dict(IGNORED["*"])
            ign.update(IGNORED.get(c, {}))
            known = set(BASE_FIELDS) | set(ign)
            if spec == "custom":
                known |= getattr(self, "custom_" + c)(f, out, ind)
            else:
                for jf, kind, member in spec:
                    if jf not in f:
                        raise SystemExit("%s %s: field %s missing in the prefab JSON" % (body, c, jf))
                    self.assign(out, "m->" + member, kind, f[jf], ind)
                    known.add(jf)
            unknown = set(f) - known
            if unknown:
                raise SystemExit("%s %s: prefab fields not consumed and not IGNORED: %s" % (body, c, sorted(unknown)))
        out.append("        pqs->childMods.push_back(std::move(m));")
        out.append("    }")

    def custom_PQSLandControl(self, f, out, ind):
        for i, lc in enumerate(f["landClasses"]):
            out.append("%s{" % ind)
            out.append("%s    PQSLandControl::LandClass lc;" % ind)
            keys = set()
            for jf, kind in LANDCLASS_SCALARS:
                self.assign(out, "lc." + jf, kind, lc[jf], ind + "    ")
                keys.add(jf)
            for sc in lc["scatter"]:
                out.append("%s    lc.scatter.push_back({%s, %s});" % (ind, lit_str(sc["scatterName"]), lit_double(sc["density"])))
                self.consumed += 2
            keys |= {"scatter"}
            # landClasses[i].scatter[j].scatter / scatterIndex: LandClassScatter objects (scatter, not ported)
            unknown = set(lc) - keys
            if unknown:
                raise SystemExit("PQSLandControl landClasses[%d]: unknown fields %s" % (i, sorted(unknown)))
            out.append("%s    m->landClasses.push_back(std::move(lc));" % ind)
            out.append("%s}" % ind)
        known = {"landClasses"}
        for jf, kind in LANDCONTROL_SCALARS:
            self.assign(out, "m->" + jf, kind, f[jf], ind)
            known.add(jf)
        return known

    def custom_PQSMod_VertexPlanet(self, f, out, ind):
        known = set()
        for jf, kind in PLANET_SCALARS:
            self.assign(out, "m->" + jf, kind, f[jf], ind)
            known.add(jf)
        for w in PLANET_WRAPPERS:
            for jf, kind in WRAPPER_FIELDS:
                self.assign(out, "m->%s.%s" % (w, jf), kind, f[w][jf], ind)
            assert set(f[w]) == {x for x, _ in WRAPPER_FIELDS}, f[w].keys()
            known.add(w)
        cs = f["continentalSharpness"]
        for jf, kind in (("deformity", "d"), ("octaves", "i"), ("persistance", "d"), ("frequency", "d")):
            self.assign(out, "m->continentalSharpness." + jf, kind, cs[jf], ind)
        assert set(cs) == {"deformity", "octaves", "persistance", "frequency"}, cs.keys()
        known.add("continentalSharpness")
        for i, lc in enumerate(f["landClasses"]):
            out.append("%s{" % ind)
            out.append("%s    PQSMod_VertexPlanet::LandClass lc;" % ind)
            for jf, kind in (("name", "s"), ("fractalStart", "d"), ("fractalEnd", "d"), ("baseColor", "color"),
                             ("colorNoise", "color"), ("colorNoiseAmount", "d"), ("lerpToNext", "b")):
                self.assign(out, "lc." + jf, kind, lc[jf], ind + "    ")
            for jf, kind in WRAPPER_FIELDS:
                self.assign(out, "lc.colorNoiseMap." + jf, kind, lc["colorNoiseMap"][jf], ind + "    ")
            unknown = set(lc) - {"name", "fractalStart", "fractalEnd", "baseColor", "colorNoise", "colorNoiseAmount",
                                 "lerpToNext", "colorNoiseMap"}
            if unknown:
                raise SystemExit("VertexPlanet landClasses[%d]: unknown fields %s" % (i, sorted(unknown)))
            out.append("%s    m->landClasses.push_back(std::move(lc));" % ind)
            out.append("%s}" % ind)
        known.add("landClasses")
        return known


def list_capacity(batches):
    """C# List<T> capacity after AddRange of each batch (referencesource List.EnsureCapacity: 0 -> 4 -> x2)."""
    cap, size = 0, 0
    for n in batches:
        if n <= 0:
            continue
        if size + n > cap:
            new = 4 if cap == 0 else cap * 2
            cap = max(new, size + n)
        size += n
    return cap


def main():
    core = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else ".")
    root = core
    while not os.path.exists(os.path.join(root, "harness", "compare", "compare.py")):
        parent = os.path.dirname(root)
        if parent == root:
            raise SystemExit("project root (harness/compare/compare.py) not found above " + core)
        root = parent
    cs = CSharp(os.path.join(root, "decompiled", "KSP-Decompiled", "Assembly-CSharp"))
    prefab = json.load(open(os.path.join(core, "data", "terrain", "pqs_prefab.json"), encoding="utf-8"))
    manifest = json.load(open(os.path.join(core, "data", "terrain", "manifest.json"), encoding="utf-8"))

    # ---- RandomVectors ----
    vals = cs.random_vectors()
    lines = ["// GENERATED by core/tools/wp11/gen_wp11_terrain.py from decompiled/KSP-Decompiled/Assembly-CSharp/",
             "// LibNoise/GradientNoiseBasis.cs (RandomVectors initialiser, literals verbatim); do not edit."]
    for i in range(0, 1024, 8):
        lines.append("    " + ", ".join(vals[i:i + 8]) + ",")
    write(core, "src/terrain/libnoise_random_vectors.inc", "\n".join(lines) + "\n")

    # ---- stacks ----
    g = Gen(cs)
    out = []
    bodies = [b for b in prefab["bodies"] if b.get("pqs")]
    sha9 = prefab["provenance"]["asset_files"].get("sharedassets9.assets", "")
    out.append("// GENERATED by core/tools/wp11/gen_wp11_terrain.py from core/data/terrain/pqs_prefab.json and")
    out.append("// core/data/terrain/manifest.json (tools/wp11/extract_terrain.py: the PSystem prefab in")
    out.append("// decompiled/KSP-dev/KSP_x64_Data/sharedassets9.assets, sha256 %s); do not edit." % sha9)
    out.append("// One factory per body with a PQS: the PQSMod components under the PQS GameObject in PQS.GetChildMods order")
    out.append("// with every serialized field the height path consumes; other mod classes are PQSModPassive (order,")
    out.append("// modEnabled, GameObject state and the requirements their OnSetup assigns).")
    out.append('#include "ksp/terrain/pqs_prefab.hpp"')
    out.append("")
    out.append("#include <limits>")
    out.append("")
    out.append('#include "ksp/terrain/pqs_mods.hpp"')
    out.append("")
    out.append("namespace ksp::terrain {")
    out.append("")
    out.append("namespace {")
    out.append("")
    out.append("Keyframe K(float time, float value, float inSlope, float outSlope, int weightedMode, float inWeight, float outWeight) {")
    out.append("    Keyframe k(time, value, inSlope, outSlope, inWeight, outWeight);")
    out.append("    k.weightedMode = static_cast<WeightedMode>(weightedMode);")
    out.append("    return k;")
    out.append("}")
    out.append("AnimationCurve Curve(std::vector<Keyframe> keys, int preInfinity, int postInfinity) {")
    out.append("    return AnimationCurve::CreateFromSerialized(keys, preInfinity, postInfinity);")
    out.append("}")
    out.append("void Base(PQSMod& m, const char* cls, const char* go, bool active, int requirements, bool modEnabled, int order,")
    out.append("          bool modExpansionDisabled) {")
    out.append("    m.className = cls;")
    out.append("    m.gameObjectPath = go;")
    out.append("    m.gameObjectActiveSelf = active;")
    out.append("    m.requirements = requirements;")
    out.append("    m.modEnabled = modEnabled;")
    out.append("    m.order = order;")
    out.append("    m.modExpansionDisabled = modExpansionDisabled;")
    out.append("}")
    out.append("")
    factories = []
    for b in bodies:
        q = b["pqs"]
        name = b["bodyName"]
        fn = "Make" + re.sub(r"\W", "", name)
        factories.append((name, fn, b))
        # GetChildMods AddRange batches: consecutive mods on the same GameObject path
        batches, last = [], None
        for m in b["mods"]:
            key = m["gameObject"]
            if batches and key == last:
                batches[-1] += 1
            else:
                batches.append(1)
            last = key
        cap = list_capacity(batches)
        out.append("// %s: PQS %s PathID %d (GameObject \"%s\"), %d PQSMod components, %d affect height" % (
            name, q["file"], q["pathID"], q["gameObject"], len(b["mods"]), sum(1 for m in b["mods"] if m["height_mod"])))
        out.append("std::unique_ptr<PQS> %s(MapLibrary& maps, const StockPQSOptions& options) {" % fn)
        out.append("    (void)maps;")
        out.append("    (void)options;")
        out.append("    auto pqs = std::make_unique<PQS>();")
        out.append("    pqs->name = %s;" % lit_str(q["gameObject"]))
        out.append("    pqs->radius = %s;" % lit_double(q["radius"]))
        out.append("    pqs->buildTangents = %s;" % lit_bool(q.get("buildTangents", 0)))
        out.append("    pqs->isFakeBuild = %s;" % lit_bool(q.get("isFakeBuild", 0)))
        out.append("    pqs->childModListCapacity = %d;  // List<PQSMod> after AddRange batches %s" % (cap, batches))
        for m in b["mods"]:
            g.mod(name, m, out)
        out.append("    return pqs;")
        out.append("}")
        out.append("")
    out.append("}  // namespace")
    out.append("")
    # map table
    out.append("const std::vector<StockMapInfo>& StockMaps() {")
    out.append("    static const std::vector<StockMapInfo> maps = {")
    for e in manifest["maps"]:
        out.append("        {%s, %s, %d, %s, %s, %d, %d, %d, %d, %s}," % (
            lit_str(e["key"]), lit_str(e["file"]), e["pathID"], lit_str(e["name"]), lit_str(e["MapName"] or ""),
            e["width"], e["height"], e["bpp"], e["bytes"], lit_str(e["sha256"])))
    out.append("    };")
    out.append("    return maps;")
    out.append("}")
    out.append("")
    out.append("const std::vector<StockPQSInfo>& StockPQSBodies() {")
    out.append("    static const std::vector<StockPQSInfo> bodies = {")
    for name, fn, b in factories:
        q, cb = b["pqs"], b["celestialBody"]
        out.append("        {%s, %d, %s, %d, %s, %s, %s, %s, %d, %s}," % (
            lit_str(name), b["flightGlobalsIndex"], lit_str(q["file"]), q["pathID"], lit_str(q["gameObject"]),
            lit_double(q["radius"]), lit_double(cb["Radius"]), lit_bool(cb["ocean"]), len(b["mods"]), fn))
    out.append("    };")
    out.append("    return bodies;")
    out.append("}")
    out.append("")
    out.append("}  // namespace ksp::terrain")
    write(core, "src/terrain/pqs_prefab_data.cpp", "\n".join(out) + "\n")
    print("bodies:", [n for n, _, _ in factories], "fields:", g.consumed)

    # ---- optional cross-check against the in-game capture ----
    cap_path = os.path.join(root, "harness", "captures", "out2", "terrain_height.json")
    if os.path.exists(cap_path):
        check_capture(cs, prefab, json.load(open(cap_path, encoding="utf-8")))


def check_capture(cs, prefab, cap):
    T = {t["name"]: t for t in cap["tables"]}
    cols = [c["name"] for c in T["mods"]["columns"]]
    names = {v: k for k, v in REQ.items()}
    problems = 0
    for r in T["mods"]["rows"]:
        m = dict(zip(cols, r))
        b = next((b for b in prefab["bodies"] if b["bodyName"] == m["body"]), None)
        cands = [x for x in b["mods"] if x["class"] == m["class"] and x["gameObject"].split("/")[-1] == m["gameObject"]
                 and x["fields"]["order"] == m["order"]]
        if not cands:
            continue  # runtime-spawned (PQSCity2 set dressing)
        x = cands[0]
        want = 0 if m["requirements"] == "Default" else sum(REQ[s.strip()] for s in m["requirements"].split(","))
        req = cs.setup_requirements(x["class"]) if not x["height_mod"] else None
        if req is not None:
            got = x["fields"]["requirements"] if req < 0 else req
            if got != want:
                problems += 1
                print("requirements mismatch", m["body"], m["class"], m["gameObject"], got, want)
    print("capture cross-check: requirements problems:", problems)


def write(core, rel, text):
    p = os.path.join(core, rel)
    with open(p, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote", rel, len(text), "bytes")


if __name__ == "__main__":
    main()
