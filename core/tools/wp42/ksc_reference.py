#!/usr/bin/env python3
"""WP-42 independent reference (no kspcore imports) for the KSC / launch-site geometry and the save round trip.

Writes core/tests/data/wp42_reference.json. Inputs (all original game data, nothing hand-authored):
  * core/data/terrain/pqs_prefab.json: serialized fields of every stock PQSCity / PQSCity2 (WP-11 extraction of sharedassets9.assets)
  * core/testdata/saves/physprofile/persistent.sfs: the ScenarioDestructibles node (the real KSC DestructibleBuilding ids)
Everything below follows PQSCity.cs / PQSCity2.cs / SpaceCenter.cs / LaunchSite.cs / CelestialBody.cs written out in plain Python:
float32 stores through struct, Mono's rule for managed float arithmetic (a float expression is evaluated in double and stored as float
at each assignment), math.* for the C runtime functions.
Fixtures: the 'sites' grid lat/lon list is a test input (documented as FIXTURE in the output), not game data.
Run:  python -I core/tools/wp42/ksc_reference.py
"""
import json
import math
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.normpath(os.path.join(HERE, "..", ".."))
PQS = os.path.join(CORE, "data", "terrain", "pqs_prefab.json")
SFS = os.path.join(CORE, "testdata", "saves", "physprofile", "persistent.sfs")
OUT = os.path.join(CORE, "tests", "data", "wp42_reference.json")

PI = math.pi
RAD2DEG_C = 57.295780181884766  # the literal CelestialBody.GetLatitudeAndLongitude uses


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def j(x):
    return None if (isinstance(x, float) and math.isnan(x)) else x


def v3f_normalized(v):
    """Vector3.normalized (managed): num = (float)sqrt(x*x+y*y+z*z); num > 1e-5 ? value / num : zero"""
    num = f32(math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]))
    if num > f32(1e-05):
        return [f32(v[0] / num), f32(v[1] / num), f32(v[2] / num)]
    return [0.0, 0.0, 0.0]


def v3d_normalized(v):
    """Vector3d.normalized: Magnitude = sqrt(x*x+y*y+z*z); > 1e-15 ? value / num : zero"""
    num = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
    if num > 1e-15:
        return [v[0] / num, v[1] / num, v[2] / num]
    return [0.0, 0.0, 0.0]


def xzy(v):
    return [v[0], v[2], v[1]]


def spherical_vector(lat, lon):
    a = math.cos(lat)
    z = math.sin(lat)
    b = math.cos(lon)
    c = math.sin(lon)
    return [a * b, a * c, z]


def rel_surface_nvector(lat, lon):
    """CelestialBody.GetRelSurfaceNVector: SphericalVector(lat*pi/180, lon*pi/180).xzy"""
    lat *= PI / 180.0
    lon *= PI / 180.0
    return xzy(spherical_vector(lat, lon))


def set_frame(A, B, C):
    n1, n2 = math.cos(A), math.sin(A)
    n3, n4 = math.cos(B), math.sin(B)
    n5, n6 = math.cos(C), math.sin(C)
    X = [n1 * n5 - n2 * n3 * n6, n2 * n5 + n1 * n3 * n6, n4 * n6]
    Y = [(0.0 - n1) * n6 - n2 * n3 * n5, (0.0 - n2) * n6 + n1 * n3 * n5, n4 * n5]
    Z = [n2 * n4, (0.0 - n1) * n4, n3]
    return X, Y, Z


def latlon_get_surface_nvector(frame, lat, lon):
    """LatLon.GetSurfaceNVector(frame, lat, lon): frame.LocalToWorld(SphericalVector).xzy"""
    lat *= PI / 180.0
    lon *= PI / 180.0
    r = spherical_vector(lat, lon)
    X, Y, Z = frame
    w = [r[0] * X[i] + r[1] * Y[i] + r[2] * Z[i] for i in range(3)]
    return xzy(w)


def latlonalt_identity(local, radius):
    """CelestialBody.GetLatLonAlt with an identity body frame at the origin"""
    r = xzy(local)
    mag = math.sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2])
    alt = mag - radius
    r = [r[0] / mag, r[1] / mag, r[2] / mag]
    lat = math.asin(r[2]) * (180.0 / PI)
    lon = math.atan2(r[1], r[0]) * (180.0 / PI)
    return lat, lon, alt


def lat_lon_start(local):
    """SpaceCenter.Start: GetLatitudeAndLongitude(pos) with the identity frame (isRadial false, body at the origin)"""
    r = xzy(v3d_normalized(local))
    lat = math.asin(r[2]) * RAD2DEG_C
    lon = math.atan2(r[1], r[0]) * RAD2DEG_C
    return lat, lon


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def great_circle(radius, srf, frm):
    d = dot(srf, frm)
    if abs(d) > 1.0:
        return float("nan")
    return radius * math.acos(d)


def quat_mul(l, r):
    lx, ly, lz, lw = l
    rx, ry, rz, rw = r
    return [lw * rx + lx * rw + ly * rz - lz * ry, lw * ry + ly * rw + lz * rx - lx * rz, lw * rz + lz * rw + lx * ry - ly * rx,
            lw * rw - lx * rx - ly * ry - lz * rz]


def quat_from_to(a, b):
    """Double-precision analytic FromToRotation (a consistency check for the float32 native, NOT bit-exact evidence)."""
    na = math.sqrt(dot(a, a))
    nb = math.sqrt(dot(b, b))
    a = [c / na for c in a]
    b = [c / nb for c in b]
    d = dot(a, b)
    if d > 1 - 1e-12:
        return [0.0, 0.0, 0.0, 1.0]
    axis = [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]
    s = math.sqrt((1 + d) * 2)
    q = [axis[0] / s, axis[1] / s, axis[2] / s, s / 2]
    n = math.sqrt(sum(c * c for c in q))
    return [c / n for c in q]


def quat_angle_axis(deg, axis):
    h = math.radians(deg) / 2
    s = math.sin(h)
    return [axis[0] * s, axis[1] * s, axis[2] * s, math.cos(h)]


def city_rows(data):
    rows = []
    for body in data["bodies"]:
        pqs = body.get("pqs")
        if not pqs:
            continue
        radius = float(pqs["radius"])
        for m in body.get("mods", []):
            if m["class"] not in ("PQSCity", "PQSCity2"):
                continue
            f = m["fields"]
            row = {"body": body["bodyName"], "name": m["gameObject"].split("/")[-1], "class": m["class"], "modIndex": m["index"], "radius": radius}
            if m["class"] == "PQSCity":
                if not f["repositionToSphere"] or f["repositionToSphereSurface"] or f["randomizeOnSphere"]:
                    continue  # needs terrain / the game's random seed: not in the reference
                rr = [f32(f["repositionRadial"][k]) for k in "xyz"]
                n = v3f_normalized(rr)
                rel = [n[k] * (radius + f["repositionRadiusOffset"]) for k in range(3)]
                local = [f32(c) for c in rel]
                lat, lon, alt = latlonalt_identity(local, radius)
                row.update({"local": local, "lat": lat, "lon": lon, "alt": alt})
                # rotation: reorientToSphere (double analytic; the native float32 quaternion is checked against it with a tolerance)
                if f["reorientToSphere"]:
                    up = [f["reorientInitialUp"][k] for k in "xyz"]
                    q = quat_mul(quat_from_to(up, v3f_normalized(local)), quat_angle_axis(f["reorientFinalAngle"], [0.0, 1.0, 0.0]))
                    row["rotation_double"] = q
            else:
                if f["snapToSurface"] or f["setOnWaterSurface"] or f["baseRotFacesNorthPole"]:
                    # surface snap needs the terrain height; recorded as the pre-snap placement only
                    snap = True
                else:
                    snap = False
                frame = set_frame(0.0, 0.0, 0.0)
                nv = latlon_get_surface_nvector(frame, f["lat"], f["lon"])
                rel = [nv[k] * (radius + f["alt"]) for k in range(3)]
                local = [f32(c) for c in rel]
                row.update({"snap": snap, "prefab_lat": f["lat"], "prefab_lon": f["lon"], "prefab_alt": f["alt"], "nvector": nv, "local": local})
                if not snap:
                    lat, lon, alt = latlonalt_identity(local, radius)
                    row.update({"lat": lat, "lon": lon, "alt": alt})
                row["srf_nvector"] = rel_surface_nvector(f["lat"], f["lon"])
            rows.append(row)
    return rows


def destructibles():
    """the building nodes of the SCENARIO ScenarioDestructibles block: id (node name) and the raw `intact` value text"""
    lines = open(SFS, newline="").read().split("\n")
    start = next(i for i, l in enumerate(lines) if l.strip() == "name = ScenarioDestructibles")
    ids = []
    i = start + 1
    while lines[i].rstrip("\r") != "\t}":
        s = lines[i].strip()
        if s.startswith("SpaceCenter/"):
            val = None
            k = i + 1
            while lines[k].strip() != "}":
                if lines[k].strip().startswith("intact"):
                    val = lines[k].strip().split("=")[1].strip()
                k += 1
            ids.append({"id": s, "intact": val})
            i = k
        i += 1
    return ids


def main():
    data = json.load(open(PQS))
    rows = city_rows(data)
    kerbin_ksc = next(r for r in rows if r["body"] == "Kerbin" and r["name"] == "KSC")
    radius = kerbin_ksc["radius"]
    sc_lat, sc_lon = lat_lon_start(kerbin_ksc["local"])
    sc_srf = rel_surface_nvector(sc_lat, sc_lon)
    grid = []
    for lat in range(-90, 91, 15):
        for lon in range(-180, 181, 20):
            frm = rel_surface_nvector(float(lat), float(lon))
            grid.append({"lat": float(lat), "lon": float(lon), "distance": j(great_circle(radius, sc_srf, frm))})
    # sites around Kerbin: the PQSCity2 launch sites of the prefab (pre-Orientate lat / lon) against each other
    sites = [r for r in rows if r["body"] == "Kerbin" and r["class"] == "PQSCity2" and r["name"].endswith("_Launch_Site")]
    matrix = []
    for a in sites:
        for b in sites:
            matrix.append({"a": a["name"], "b": b["name"], "distance": j(great_circle(radius, a["srf_nvector"], b["srf_nvector"]))})
    out = {
        "provenance": {
            "cities": "core/data/terrain/pqs_prefab.json (WP-11 extraction of KSP_x64_Data/sharedassets9.assets)",
            "destructibles": "core/testdata/saves/physprofile/persistent.sfs ScenarioDestructibles",
            "fixtures": "grid: FIXTURE lat/lon list (-90..90 step 15, -180..180 step 20), not game data",
        },
        "cities": [{k: ([j(c) for c in v] if isinstance(v, list) else j(v)) for k, v in r.items()} for r in rows],
        "spaceCenter": {"lat": sc_lat, "lon": sc_lon, "srfNVector": sc_srf, "radius": radius, "grid": grid},
        "siteMatrix": matrix,
        "destructibles": destructibles(),
    }
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", newline="\n") as fh:
        json.dump(out, fh, indent=None, separators=(",", ":"), allow_nan=False)
    print("wrote", OUT, len(rows), "cities", len(grid), "grid rows", len(matrix), "site pairs", len(out["destructibles"]), "destructibles")


if __name__ == "__main__":
    sys.exit(main())
