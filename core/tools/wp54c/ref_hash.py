"""WP-54c reference, part 5: the GetHashCode() overrides of the Making History classes (ScoreRange.cs:42, VesselPartIDPair.cs:38, MissionInstructor.cs:112, MissionBiome.cs:83,
MissionOrbit.cs:308, VesselGroundLocation.cs:161, SurfaceArea.cs:91, SurfaceVolume.cs:169, MissionKerbal.cs:331, ProtoCrewMember.cs:1371 for the kerbal part,
DynamicModule.cs:67, DynamicModuleList.cs:107). Written from the C# and the Mono corlib rules (Int32 / UInt32 / enum: the value, Boolean: 1 / 0, Single / Double: the bits
with +-0.0 hashing to 0, String: h = h * 31 + c over the UTF-16 code units). No kspcore code.

The Unity Object / reference hash codes are not reproducible values: the test pins them through MissionDataEnvironment::objectHashCode (body index + 1000, rotation 7777,
the module list 4242) and the reference uses the same constants."""
import struct

from ref_c_common import *  # noqa: F401,F403
from ref_c_common import Cfg, rng_pick, BODIES, f32

BODY_HASH = lambda idx: 1000 + idx
ROT_HASH = 7777
LIST_HASH = 4242


def i32(x):
    return ((x + 2 ** 31) % 2 ** 32) - 2 ** 31


def h_double(d):
    if d == 0:
        return 0
    b = struct.unpack("<q", struct.pack("<d", d))[0]
    lo = struct.unpack("<i", struct.pack("<I", b & 0xFFFFFFFF))[0]
    hi = b >> 32
    return i32(lo ^ hi)


def h_float(f):
    f = f32(f)
    if f == 0:
        return 0
    return struct.unpack("<i", struct.pack("<f", f))[0]


def h_uint(u):
    return i32(u)


def h_bool(b):
    return 1 if b else 0


def h_string(s):
    """GetHashCode_Net35 / Mono String.GetHashCode: h = (h << 5) - h + c over UTF-16 code units, int wrap-around"""
    data = s.encode("utf-16-le")
    units = [data[i] | (data[i + 1] << 8) for i in range(0, len(data), 2)]
    h = 0
    i = 0
    while i < len(units) - 1:
        n2 = i32(i32(i32(h << 5) - h) + units[i])
        h = i32(i32(i32(n2 << 5) - n2) + units[i + 1])
        i += 2
    if i < len(units):
        h = i32(i32(i32(h << 5) - h) + units[i])
    return h


NAMES = ["Kerbin", "Mun", "Minmus", "Sun"]
STR_POOL = ["", "a", "ab", "abc", "Jebediah Kerman", "Wernher von Kerman", "caf" + chr(233), "x" + chr(0x1F600) + "y", "ScoreModule_Completion"]
DBL_POOL = [0.0, 1.0, -1.0, 12.5, 600000.0, 1e-5, 3.14159265358979, 84159286.0, -45.5625, 1234567.890123, 9.5e12]
FLT_POOL = [0.0, 1.0, -1.0, 12.5, 150000.0, 0.1, 1e-5, 4.5e7]
UINT_POOL = [0, 1, 107, 4294967295, 2147483648, 3827239225]


def gen_cases(r):
    cases = []
    for _ in range(40):
        mn, mx, sc = rng_pick(r, DBL_POOL), rng_pick(r, DBL_POOL), rng_pick(r, FLT_POOL)
        cases.append({"cls": "ScoreRange", "args": {"min": mn, "max": mx, "score": sc}, "out": i32(h_double(mn) ^ h_double(mx) ^ h_float(sc))})
    for _ in range(30):
        v, p = rng_pick(r, UINT_POOL), rng_pick(r, UINT_POOL)
        cases.append({"cls": "VesselPartIDPair", "args": {"v": v, "p": p}, "out": i32(h_uint(v) ^ h_uint(p))})
    for _ in range(30):
        n, vs = rng_pick(r, STR_POOL), r.random() < 0.5
        cases.append({"cls": "MissionInstructor", "args": {"name": n, "vintage": vs}, "out": i32(h_string(n) ^ h_bool(vs))})
    for _ in range(30):
        n, bi = rng_pick(r, STR_POOL), r.randrange(len(BODIES))
        cases.append({"cls": "MissionBiome", "args": {"name": n, "body": bi}, "out": i32(h_string(n) ^ BODY_HASH(bi))})
    for _ in range(40):
        vals = {"SMA": rng_pick(r, DBL_POOL[1:]), "ECC": rng_pick(r, [0.0, 0.25, 0.5, 0.999]), "INC": rng_pick(r, DBL_POOL), "LPE": rng_pick(r, DBL_POOL), "LAN": rng_pick(r, DBL_POOL),
                "MNA": rng_pick(r, DBL_POOL), "EPH": rng_pick(r, DBL_POOL), "REF": r.randrange(len(BODIES))}
        out = h_uint(vals["REF"])
        for k in ("SMA", "ECC", "INC", "LAN", "LPE", "EPH", "MNA"):
            out ^= h_double(vals[k])
        cases.append({"cls": "MissionOrbit", "args": vals, "out": i32(out)})
    for _ in range(40):
        bi, alt, lat, lon = r.randrange(len(BODIES)), rng_pick(r, DBL_POOL), rng_pick(r, DBL_POOL), rng_pick(r, DBL_POOL)
        cases.append({"cls": "VesselGroundLocation", "args": {"body": bi, "alt": alt, "lat": lat, "lon": lon}, "out": i32(BODY_HASH(bi) ^ h_double(alt) ^ h_double(lat) ^ h_double(lon) ^ ROT_HASH)})
    for _ in range(40):
        bi, lat, lon, rad = r.randrange(len(BODIES)), rng_pick(r, DBL_POOL), rng_pick(r, DBL_POOL), rng_pick(r, FLT_POOL)
        cases.append({"cls": "SurfaceArea", "args": {"body": bi, "lat": lat, "lon": lon, "radius": rad}, "out": i32(h_string(BODIES[bi]["name"]) ^ h_double(lat) ^ h_double(lon) ^ h_float(rad))})
    for _ in range(40):
        bi, lat, lon = r.randrange(len(BODIES)), rng_pick(r, DBL_POOL), rng_pick(r, DBL_POOL)
        rad, hs, hmin, hmax = rng_pick(r, FLT_POOL), rng_pick(r, FLT_POOL), rng_pick(r, FLT_POOL), rng_pick(r, FLT_POOL)
        shape = r.randrange(2)
        out = BODY_HASH(bi) ^ h_double(lat) ^ h_double(lon) ^ h_float(rad) ^ shape ^ h_float(hs) ^ h_float(hmin) ^ h_float(hmax)
        cases.append({"cls": "SurfaceVolume", "args": {"body": bi, "lat": lat, "lon": lon, "radius": rad, "shape": shape, "hs": hs, "hmin": hmin, "hmax": hmax}, "out": i32(out)})
    for _ in range(40):
        name = rng_pick(r, STR_POOL[1:])
        stupid, courage = rng_pick(r, FLT_POOL), rng_pick(r, FLT_POOL)
        bad, vet, toured = r.random() < 0.5, r.random() < 0.5, r.random() < 0.5
        utar = rng_pick(r, DBL_POOL)
        kt = r.randrange(4)    # KerbalType Crew=0, Applicant=1, Unowned=2, Tourist=3
        kerbal_hash = h_string(name) ^ h_float(stupid) ^ h_float(courage) ^ h_bool(bad) ^ h_bool(vet) ^ h_bool(toured) ^ h_double(utar)
        cases.append({"cls": "MissionKerbal", "args": {"name": name, "stupidity": stupid, "courage": courage, "bad": bad, "vet": vet, "toured": toured, "utar": utar, "type": kt},
                      "out": i32(kt ^ kerbal_hash)})
    cases.append({"cls": "MissionKerbal", "args": None, "out": "throws"})
    for name in ["ScoreModule_Completion", "ScoreModule_Time", "AdjusterEnginesThrottle", "VesselRestriction_Cost", "AwardModule_Score", "FailureLightStuck"]:
        cases.append({"cls": "DynamicModule", "args": {"name": name}, "out": h_string(name)})
    cases.append({"cls": "DynamicModuleList", "args": {}, "out": LIST_HASH})
    return cases


def generate(r):
    return [{"kind": "hash", "cls": c["cls"], "args": c["args"], "out": c["out"]} for c in gen_cases(r)]
