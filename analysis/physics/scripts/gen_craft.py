#!/usr/bin/env python3
"""Generate parametric stock-part test rockets (.craft ConfigNode files) for the KSP physics profile.

Design (same booster type at every size, so the scaling curve compares like with like):
  core  : probeStackLarge (RC-L01, root) / K x Rockomax32.BW (X200-32) / liquidEngineMainsail.v2
  rows  : one ring of B radial boosters on every ROW_EVERY-th core tank; successive rows are rotated by
          360/(B*PHASES) deg so that no booster sits within 10 m (ModuleEngines exhaustDamageMaxRange)
          below another booster's engine. (Run 1 used ROW_EVERY=1, PHASES=2: the 446-part version put
          booster nose cones 0.7 m under the engines of the row above and destroyed itself on ignition.)
  booster: radialDecoupler1-2 (HDM, srf-attached to the core tank)
           -> T x fuelTankSmallFlat (FL-T100), the bottom one srf-attached to the decoupler
           -> liquidEngine2 (LV-T45) under the bottom tank, pointyNoseConeB on the top tank
  staging: all engines istg=1 (first stage), decouplers istg=0 (never fired during the profile)
Relative placements are copied from the stock Kerbal X craft (radial decoupler 1.2 m from the core
axis, 0.80 m below the tank centre; booster tank 1.98877 m out, 0.0985 m below the decoupler).
Part count = (K + 2) + ceil(K / ROW_EVERY) * B * (T + 3).

Usage: python gen_craft.py <outdir>
"""
import math
import os
import sys

NODE = {
    # part: (top node y, bottom node y)
    "probeStackLarge": (0.19, -0.19),
    "Rockomax32.BW": (1.86, -1.86),
    "liquidEngineMainsail.v2": (1.01359, -1.95705),
    "fuelTankSmallFlat": (0.3125, -0.3125),
    "liquidEngine2": (0.9018263, -0.7179225),
    "pointyNoseConeB": (None, -0.625),
}
Q0 = (5.53134996e-08, 0.707106888, -5.53134925e-08, -0.707106769)  # Kerbal X radial decoupler rot at azimuth "-z"


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def qinv(q):
    return (-q[0], -q[1], -q[2], q[3])


def qrot(q, v):
    x, y, z = v
    p = qmul(qmul(q, (x, y, z, 0.0)), qinv(q))
    return (p[0], p[1], p[2])


def ry(deg):
    h = math.radians(deg) / 2.0
    return (0.0, math.sin(h), 0.0, math.cos(h))


def fv(v):
    return ",".join("%.7g" % c for c in v)


class P:
    n = 4294000000

    def __init__(self, name, pos, rot=(0.0, 0.0, 0.0, 1.0)):
        P.n += 7
        self.name, self.id, self.pos, self.rot = name, P.n, pos, rot
        self.parent = None
        self.children = []
        self.attn = []   # (nodeName, otherPart, nodeY)
        self.srf = None  # parent part for srfAttach
        self.istg = -1
        self.sidx = -1
        self.attm = 0

    @property
    def ref(self):
        return "%s_%d" % (self.name, self.id)


def link(parent, child):
    child.parent = parent
    parent.children.append(child)


def build(K, B, T, row_every=2, phases=3):
    parts = []
    y_probe = 0.0
    probe = P("probeStackLarge", (0.0, y_probe, 0.0))
    parts.append(probe)
    prev = probe
    prev_bottom = NODE["probeStackLarge"][1]
    tanks = []
    y = y_probe
    for i in range(K):
        y = y + prev_bottom - NODE["Rockomax32.BW"][0]
        t = P("Rockomax32.BW", (0.0, y, 0.0))
        link(prev, t)
        prev.attn.append(("bottom", t, prev_bottom))
        t.attn.append(("top", prev, NODE["Rockomax32.BW"][0]))
        parts.append(t)
        tanks.append(t)
        prev, prev_bottom = t, NODE["Rockomax32.BW"][1]
    y = y + prev_bottom - NODE["liquidEngineMainsail.v2"][0]
    me = P("liquidEngineMainsail.v2", (0.0, y, 0.0))
    link(prev, me)
    prev.attn.append(("bottom", me, prev_bottom))
    me.attn.append(("top", prev, NODE["liquidEngineMainsail.v2"][0]))
    me.istg, me.sidx = 1, 0
    parts.append(me)
    eng_idx, dec_idx = 1, 0
    for i, tank in enumerate(tanks):
        if i % row_every:
            continue
        row = i // row_every
        for j in range(B):
            az = j * 360.0 / B + (row % phases) * (360.0 / B / phases)
            q = qmul(ry(az), Q0)
            ty = tank.pos[1]
            dpos = qrot(ry(az), (0.0, -0.8, -1.2))
            dec = P("radialDecoupler1-2", (dpos[0], ty + dpos[1], dpos[2]), q)
            dec.srf, dec.attm, dec.istg, dec.sidx = tank, 1, 0, dec_idx
            dec_idx += 1
            link(tank, dec)
            parts.append(dec)
            bpos = qrot(ry(az), (0.0, 0.0, -1.98877))
            by = dec.pos[1] - 0.0984839
            bottom = P("fuelTankSmallFlat", (bpos[0], by, bpos[2]), q)
            bottom.srf, bottom.attm = dec, 1
            link(dec, bottom)
            parts.append(bottom)
            # engine under the bottom tank
            ey = by + NODE["fuelTankSmallFlat"][1] - NODE["liquidEngine2"][0]
            eng = P("liquidEngine2", (bpos[0], ey, bpos[2]), q)
            eng.istg, eng.sidx = 1, eng_idx
            eng_idx += 1
            link(bottom, eng)
            bottom.attn.append(("bottom", eng, NODE["fuelTankSmallFlat"][1]))
            eng.attn.append(("top", bottom, NODE["liquidEngine2"][0]))
            parts.append(eng)
            # tanks above
            cur, cy = bottom, by
            for k in range(1, T):
                cy = cy + NODE["fuelTankSmallFlat"][0] - NODE["fuelTankSmallFlat"][1]
                t2 = P("fuelTankSmallFlat", (bpos[0], cy, bpos[2]), q)
                link(cur, t2)
                cur.attn.append(("top", t2, NODE["fuelTankSmallFlat"][0]))
                t2.attn.append(("bottom", cur, NODE["fuelTankSmallFlat"][1]))
                parts.append(t2)
                cur = t2
            ny = cy + NODE["fuelTankSmallFlat"][0] - NODE["pointyNoseConeB"][1]
            nose = P("pointyNoseConeB", (bpos[0], ny, bpos[2]), q)
            link(cur, nose)
            cur.attn.append(("top", nose, NODE["fuelTankSmallFlat"][0]))
            nose.attn.append(("bottom01", cur, NODE["pointyNoseConeB"][1]))
            parts.append(nose)
    return parts


def write(path, shipname, parts, autostrut="Off"):
    xs = [p.pos[0] for p in parts]
    ys = [p.pos[1] for p in parts]
    zs = [p.pos[2] for p in parts]
    out = []
    out.append("ship = %s" % shipname)
    out.append("version = 1.12.5")
    out.append("description = Generated physics-profile test rocket (%d parts). analysis/physics/scripts/gen_craft.py" % len(parts))
    out.append("type = VAB")
    out.append("size = %.3f,%.3f,%.3f" % (max(xs) - min(xs) + 2.5, max(ys) - min(ys) + 4, max(zs) - min(zs) + 2.5))
    out.append("steamPublishedFileId = 0")
    out.append("persistentId = %d" % (1000000 + len(parts)))
    out.append("rot = 0,0,0,1")
    out.append("missionFlag = Squad/Flags/default")
    out.append("vesselType = Ship")
    for p in parts:
        if p.parent is not None:
            ip = qinv(p.parent.rot)
            rel = qrot(ip, tuple(a - b for a, b in zip(p.pos, p.parent.pos)))
            relrot = qmul(ip, p.rot)
        else:
            rel, relrot = p.pos, p.rot
        out.append("PART")
        out.append("{")
        out.append("\tpart = %s" % p.ref)
        out.append("\tpartName = Part")
        out.append("\tpersistentId = %d" % (p.id % 1000000000))
        out.append("\tpos = %s" % fv(p.pos))
        out.append("\tattPos = 0,0,0")
        out.append("\tattPos0 = %s" % fv(rel))
        out.append("\trot = %s" % fv(p.rot))
        out.append("\tattRot = 0,0,0,1")
        out.append("\tattRot0 = %s" % fv(relrot))
        out.append("\tmir = 1,1,1")
        out.append("\tsymMethod = Radial")
        out.append("	autostrutMode = %s" % (autostrut if p.parent is not None else "Off"))
        out.append("\trigidAttachment = False")
        out.append("\tistg = %d" % p.istg)
        out.append("\tresPri = 0")
        out.append("\tdstg = %d" % max(p.istg, 0))
        out.append("\tsidx = %d" % p.sidx)
        out.append("\tsqor = %d" % p.istg)
        out.append("\tsepI = 0")
        out.append("\tattm = %d" % p.attm)
        out.append("\tsameVesselCollision = False")
        out.append("\tmodCost = 0")
        out.append("\tmodMass = 0")
        out.append("\tmodSize = 0,0,0")
        for c in p.children:
            out.append("\tlink = %s" % c.ref)
        for (node, other, ny) in p.attn:
            out.append("\tattN = %s,%s_0|%.7g|0" % (node, other.ref, ny))
        if p.srf is not None:
            out.append("\tsrfN = srfAttach,%s" % p.srf.ref)
        out.append("\tEVENTS")
        out.append("\t{")
        out.append("\t}")
        out.append("\tACTIONS")
        out.append("\t{")
        out.append("\t}")
        out.append("\tPARTDATA")
        out.append("\t{")
        out.append("\t}")
        out.append("}")
    with open(path, "w", newline="\r\n") as f:
        f.write("\n".join(out) + "\n")


def main():
    outdir = sys.argv[1]
    os.makedirs(outdir, exist_ok=True)
    for name, (K, B, T), auto in [("PhysProf-S", (1, 2, 6), "Off"), ("PhysProf-M2", (7, 4, 6), "Off"), ("PhysProf-L2", (23, 4, 6), "Off"),
                                  ("PhysProf-S-AS", (1, 2, 6), "Grandparent"), ("PhysProf-M2-AS", (7, 4, 6), "Grandparent"),
                                  ("PhysProf-L2-AS", (23, 4, 6), "Grandparent")]:
        P.n = 4294000000
        parts = build(K, B, T)
        write(os.path.join(outdir, name + ".craft"), name, parts, auto)
        print("%s: K=%d B=%d T=%d parts=%d engines=%d height=%.1f m" % (name, K, B, T, len(parts), sum(1 for p in parts if "ngine" in p.name), max(p.pos[1] for p in parts) - min(p.pos[1] for p in parts)))


if __name__ == "__main__":
    main()
