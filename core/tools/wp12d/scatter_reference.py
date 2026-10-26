"""WP-12d independent reference: PQSLandControl.LandClassScatter.AddScatterMeshController (count and seed) and CreateScatterMesh (the
placement of the scatter instances and the transformed quad mesh), written again from PQSLandControl.cs:292-497 with no kspcore code.

Numerics: UnityEngine.Random (native xorshift128: InitState, value, Range(float, float), Range(int, int)) written from Unity's
algorithm and checked at start-up against every row of the in-game capture harness/captures/out/unity_random.json (CAPTURE-SPEC C2);
Quaternion.AngleAxis (native, float32 per operation, the CRT's sinf / cosf of msvcr120_clr0400.dll) checked against the 84
Quaternion.AngleAxis rows of harness/captures/out/math_shim_native.json (C3); the managed Unity maths (Vector3.Lerp / normalized / *,
Quaternion * Quaternion, Quaternion * Vector3) under the Mono float rule (tools/wp12/pqs_ref_math.py); the .NET 3.5 string hash of
StringExtensions.cs:3; BuildCacheQuad's 8-vertex quad (PQSLandControl.cs:345).

Inputs: tools/wp12d/scatter_inputs.json from tools/wp12d/scatter_inputs.cpp (the quads as the WP-12a quad build left them and the density
OnQuadBuilt passed: inputs of the stage gated here, not outputs of it).

  python -I tools/wp12d/scatter_reference.py   -> tests/data/wp12d_scatter_reference.json
"""
import ctypes
import hashlib
import json
import math
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(CORE, "tools", "wp12"))
import pqs_ref_math as M  # noqa: E402

f32 = M.f32
INPUTS = os.path.join(HERE, "scatter_inputs.json")
OUT = os.path.join(CORE, "tests", "data", "wp12d_scatter_reference.json")
MASK = 0xFFFFFFFF


def fh(x):
    return "%08x" % M.fbits(x)


def dh(x):
    return "%016x" % M.dbits(x)


def i32(v):
    v &= MASK
    return v - (1 << 32) if v & 0x80000000 else v


# ---- UnityEngine.Random (native) ---------------------------------------------------------------------------------------------
class URandom:
    def init(self, seed):
        s0 = seed & MASK
        s1 = (s0 * 1812433253 + 1) & MASK
        s2 = (s1 * 1812433253 + 1) & MASK
        s3 = (s2 * 1812433253 + 1) & MASK
        self.s = [s0, s1, s2, s3]

    def next(self):
        s0, s1, s2, s3 = self.s
        t = (s0 ^ (s0 << 11)) & MASK
        s3n = (s3 ^ (s3 >> 19) ^ t ^ (t >> 8)) & MASK
        self.s = [s1, s2, s3, s3n]
        return s3n

    def value(self):
        # (float)(r & 0x7FFFFF) / 8388607f, one float division
        return f32(f32(float(self.next() & 0x7FFFFF)) / f32(8388607.0))

    def range_f(self, mn, mx):
        t = self.value()
        # t * min + (1 - t) * max in float32 per operation (native)
        return f32(f32(t * mn) + f32(f32(1.0 - t) * mx))

    def range_i(self, mn, mx):
        if mn < mx:
            return i32(mn + (self.next() % ((mx - mn) & MASK)))
        if mn > mx:
            return i32(mn - (self.next() % ((mn - mx) & MASK)))
        return mn


def check_random():
    d = json.load(open(os.path.join(M.ROOT, "harness", "captures", "out", "unity_random.json"), encoding="utf-8"))
    T = {t["name"]: t for t in d["tables"]}
    r = URandom()
    n = 0
    for name, call in (("value", lambda: r.value()), ("range_f", lambda: r.range_f(-10.0, 10.0))):
        cols = [c["name"] for c in T[name]["columns"]]
        rows = T[name]["rows"]
        seed = None
        for row in rows:
            x = dict(zip(cols, row))
            if x["seed"] != seed:
                seed = x["seed"]
                r.init(seed)
            v = call()
            if fh(v) != x["value_hex"].lower():
                raise SystemExit("UnityEngine.Random %s seed %d k %d: %s != game %s" % (name, seed, x["k"], fh(v), x["value_hex"]))
            n += 1
    cols = [c["name"] for c in T["range_i"]["columns"]]
    seed, call = None, None
    for row in T["range_i"]["rows"]:
        x = dict(zip(cols, row))
        if x["seed"] != seed or x["call"] != call:
            if x["seed"] != seed:
                r.init(x["seed"])
            seed, call = x["seed"], x["call"]
        a, b = (int(t) for t in call.split("_"))
        v = r.range_i(a, b)
        if v != x["value"]:
            raise SystemExit("UnityEngine.Random.Range(%d, %d) seed %d k %d: %d != game %d" % (a, b, seed, x["k"], v, x["value"]))
        n += 1
    return n


# ---- Quaternion.AngleAxis (native) -------------------------------------------------------------------------------------------
_crt = None
for _name in ("msvcr120_clr0400.dll", "msvcr120.dll"):
    try:
        _crt = ctypes.CDLL(os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32", _name))
        break
    except OSError:
        pass
for _fn in ("sinf", "cosf"):
    getattr(_crt, _fn).restype = ctypes.c_float
    getattr(_crt, _fn).argtypes = [ctypes.c_float]


def angle_axis(angle, axis):
    """AxisAngleToQuaternionSafe: deg / 360 * 2 * kPI, the axis magnitude, sinf / cosf of the half angle, per-operation float32"""
    pi = f32(math.pi)
    rad = f32(f32(f32(angle / f32(360.0)) * f32(2.0)) * pi)
    mag = M.nf_sqrt(f32(f32(f32(axis[0] * axis[0]) + f32(axis[1] * axis[1])) + f32(axis[2] * axis[2])))
    if mag > f32(0.000001):
        half = f32(rad * f32(0.5))
        w = f32(_crt.cosf(half))
        s = f32(f32(_crt.sinf(half)) / mag)
        return (f32(s * axis[0]), f32(s * axis[1]), f32(s * axis[2]), w)
    return (0.0, 0.0, 0.0, 1.0)


def check_angle_axis():
    d = json.load(open(os.path.join(M.ROOT, "harness", "captures", "out", "math_shim_native.json"), encoding="utf-8"))
    tab = d["tables"][0]
    cols = [c["name"] for c in tab["columns"]]
    n = 0
    for row in tab["rows"]:
        x = dict(zip(cols, row))
        if x["fn"] != "Quaternion.AngleAxis":
            continue
        a = [M.from_fhex(x["a%d_hex" % i]) for i in range(4)]
        q = angle_axis(a[0], tuple(a[1:4]))
        want = [M.from_fhex(x["r%d_hex" % i]) for i in range(4)]
        if [M.fbits(c) for c in q] != [M.fbits(c) for c in want]:
            raise SystemExit("Quaternion.AngleAxis row %s: %s != game %s" % (x["i"], q, want))
        n += 1
    return n


# ---- managed Unity maths (Mono rule) -----------------------------------------------------------------------------------------
def q_mul_q(l, r):  # Quaternion.cs operator*(Quaternion, Quaternion)
    lx, ly, lz, lw = l
    rx, ry, rz, rw = r
    return (f32(lw * rx + lx * rw + ly * rz - lz * ry), f32(lw * ry + ly * rw + lz * rx - lx * rz),
            f32(lw * rz + lz * rw + lx * ry - ly * rx), f32(lw * rw - lx * rx - ly * ry - lz * rz))


def q_mul_v3(q, p):  # Quaternion.cs:181
    x, y, z, w = q
    num, num2, num3 = f32(x * 2.0), f32(y * 2.0), f32(z * 2.0)
    num4, num5, num6 = f32(x * num), f32(y * num2), f32(z * num3)
    num7, num8, num9 = f32(x * num2), f32(x * num3), f32(y * num3)
    num10, num11, num12 = f32(w * num), f32(w * num2), f32(w * num3)
    return (f32((1.0 - (num5 + num6)) * p[0] + (num7 - num12) * p[1] + (num8 + num11) * p[2]),
            f32((num7 + num12) * p[0] + (1.0 - (num4 + num6)) * p[1] + (num9 - num10) * p[2]),
            f32((num8 - num11) * p[0] + (num9 + num10) * p[1] + (1.0 - (num4 + num5)) * p[2]))


def clamp01(t):
    return 0.0 if t < 0.0 else (1.0 if t > 1.0 else t)


def v3lerp(a, b, t):  # Vector3.Lerp
    t = clamp01(t)
    return (f32(a[0] + (b[0] - a[0]) * t), f32(a[1] + (b[1] - a[1]) * t), f32(a[2] + (b[2] - a[2]) * t))


def hash_net35(s):  # StringExtensions.cs:3 GetHashCode_Net35 over UTF-16 code units
    u = s.encode("utf-16-le")
    cu = [u[i] | (u[i + 1] << 8) for i in range(0, len(u), 2)]
    num, i = 0, 0
    while i < len(cu) - 1:
        num2 = i32((num << 5) - num + cu[i])
        num = i32((num2 << 5) - num2 + cu[i + 1])
        i += 2
    if i < len(cu):
        num = i32((num << 5) - num + cu[i])
    return num


def conv_i4(d):  # (int) of a double (conv.i4, x64 cvttsd2si: 0x80000000 out of range)
    if d != d or d >= 2147483648.0 or d < -2147483648.0:
        return -2147483648
    return int(d)


# ---- PQSLandControl.LandClassScatter -----------------------------------------------------------------------------------------
QUAD_VERTS = [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (1.0, 1.0, 0.0)] * 2   # :347
QUAD_NORMS = [(0.0, 0.0, 1.0)] * 4 + [(0.0, 0.0, -1.0)] * 4                              # :372


def place(scene, sc, q):
    side = scene["cacheSideVertCount"]
    cache_res = scene["cacheRes"]
    radius = M.from_dhex(scene["radius"])
    max_scatter = sc["maxScatter"]
    density = M.from_dhex(q["density"])
    area = M.from_dhex(q["quadArea"])
    # AddScatterMeshController (:292)
    n = conv_i4(density * M.from_dhex(sc["densityFactor"]) * (area / radius / 1000.0) * float(max_scatter))
    if n < 1:
        return None
    n = min(n, max_scatter)
    seed = i32(sc["seed"] + hash_net35(q["name"]))
    verts = [tuple(M.from_fhex(c) for c in v) for v in q["verts"]]
    pp = tuple(M.from_dhex(c) for c in q["positionPlanet"])
    qr = tuple(f32(M.from_dhex(c)) for c in q["quadRotation"])   # (Quaternion)quadRotation
    vo, mn, mx = M.from_fhex(sc["verticalOffset"]), M.from_fhex(sc["minScale"]), M.from_fhex(sc["maxScale"])
    base_v, base_n = ([], []) if sc["hasBaseMesh"] else (QUAD_VERTS, QUAD_NORMS)   # stock base meshes are not extracted
    stride = len(base_v)
    r = URandom()
    r.init(seed)                                   # CreateScatterMesh (:442)
    inst, vt, nt = [], [], []
    for _ in range(n):
        num2 = num3 = -1
        while num3 == num2:
            num4 = r.range_i(1, cache_res + 1)
            num5 = r.range_i(1, cache_res + 1)
            x = num4 + r.range_i(-1, 1)
            z = num5 + r.range_i(-1, 1)
            num3 = num5 * side + num4             # PQS.vi (PQS.cs:1724)
            num2 = z * side + x
        pos = v3lerp(verts[num3], verts[num2], r.value())
        if scene["surfaceRelativeQuads"]:
            up = M.d2f(M.dnormalized(M.dadd(pos, pp)))
        else:
            up = M.v3normalized(pos)
        pos = M.v3add(pos, M.v3mul(up, vo))
        ang = f32(r.value() * 360.0)
        rot = q_mul_q(angle_axis(ang, up), qr)
        scl = r.range_f(mn, mx)
        inst.append((pos, up, ang, rot, scl, num3, num2))
        for k in range(stride):
            vt.append(M.v3add(pos, q_mul_v3(rot, M.v3mul(base_v[k], scl))))
            nt.append(q_mul_v3(rot, base_n[k]))
    vt += [(0.0, 0.0, 0.0)] * (stride * (max_scatter - n))
    return n, seed, inst, vt, nt, stride * max_scatter


def digest(floats):
    h = hashlib.sha256()
    for x in floats:
        h.update(struct.pack("<I", M.fbits(x)))
    return h.hexdigest()


def main():
    nrand = check_random()
    nang = check_angle_axis()
    inp = json.load(open(INPUTS, encoding="utf-8"))
    quads = []
    for si, scene in enumerate(inp["scenes"]):
        for sc in scene["scatters"]:
            for qi, q in enumerate(sc["quads"]):
                res = place(scene, sc, q)
                if res is None:
                    raise SystemExit("an input quad without a scatter count: %s" % q["name"])
                n, seed, inst, vt, nt, nverts = res
                inst_floats = []
                for (pos, up, ang, rot, scl, a, b) in inst:
                    inst_floats += list(pos) + list(up) + [ang] + list(rot) + [scl]
                row = {"scene": si, "scatter": sc["index"], "scatterName": sc["scatterName"], "quad": q["name"], "count": n, "seed": seed,
                       "vertexCount": nverts, "instances": digest(inst_floats), "viPairs": hashlib.sha256(json.dumps([[a, b] for *_, a, b in inst]).encode()).hexdigest(),
                       "vertices": digest([c for v in vt for c in v]), "normals": digest([c for v in nt for c in v])}
                if qi == 0:
                    row["firstInstances"] = [{"position": [fh(c) for c in pos], "up": [fh(c) for c in up], "angle": fh(ang),
                                              "rotation": [fh(c) for c in rot], "scale": fh(scl), "vi": [a, b]}
                                             for (pos, up, ang, rot, scl, a, b) in inst[:5]]
                quads.append(row)
    doc = {"schema": "kspcore-wp12d-scatter/1",
           "provenance": {"tool": "core/tools/wp12d/scatter_reference.py", "spec": "PQSLandControl.cs:292-497, StringExtensions.cs:3",
                          "inputs": "core/tools/wp12d/scatter_inputs.json (sha256 %s)" % hashlib.sha256(open(INPUTS, "rb").read()).hexdigest(),
                          "checked": {"unity_random_rows": nrand, "angle_axis_rows": nang}},
           "quads": quads}
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        json.dump(doc, f, sort_keys=True, indent=0)
        f.write("\n")
    print("wrote", OUT, len(quads), "quads;", nrand, "Random rows and", nang, "AngleAxis rows checked")


if __name__ == "__main__":
    main()
