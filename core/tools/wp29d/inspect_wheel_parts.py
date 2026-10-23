"""WP-29d: per wheel part, the model transform names the cfg refers to and where they sit (chain scale, chunk presence)."""
import os, sys, re, glob, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mu_wheel_reader as R
KEYS = ("wheelColliderTransformName", "wheelTransformName", "suspensionTransformName", "suspensionColliderName", "deployTargetTransformName",
        "retractTransformName", "animationTrfName", "animationStateName", "steeringTransformName", "caliperTransformName",
        "useStandInCollider", "FitWheelColliderToMesh", "bogeyTransformName", "clipObject", "TsubSys", "deployedPosition", "wheelType",
        "radius", "center", "suspensionDistance", "targetPosition", "damagedTransformName", "undamagedTransformName", "rescaleFactor")
staging = sys.argv[1]
for d in sorted(glob.glob(staging + "/*/")):
    name = os.path.basename(os.path.dirname(d))
    cfg = [f for f in glob.glob(d + "*.cfg")][0]
    text = open(cfg, encoding="utf-8-sig").read()
    mu = glob.glob(d + "*.mu")[0]
    ver, root = R.read_wheel_mu(open(mu, "rb").read())
    vals = {}
    for k in KEYS:
        for m in re.finditer(r"(?m)^\s*%s\s*=\s*(.*?)\s*(//.*)?$" % k, text):
            vals.setdefault(k, []).append(m.group(1))
    print("=== %s (%s)" % (name, os.path.basename(mu)))
    print("   cfg: " + "; ".join("%s=%s" % (k, ",".join(v)) for k, v in vals.items()))
    # find names
    def find(node, nm, chain=()):
        c = chain + (node,)
        if node.name == nm: return c
        for ch in node.children:
            r = find(ch, nm, c)
            if r: return r
        return None
    for k in ("wheelColliderTransformName", "wheelTransformName", "deployTargetTransformName", "suspensionTransformName", "steeringTransformName"):
        for nm in vals.get(k, [])[:1]:
            ch = find(root, nm)
            if not ch: print("   %s %s: NOT FOUND" % (k, nm)); continue
            sc = [1.0, 1.0, 1.0]
            for n in ch:
                sc = [a * b for a, b in zip(sc, n.scl)]
            print("   %s %s: depth %d chainScale=%s wheelChunk=%s" % (k, nm, len(ch), ["%.4g" % x for x in sc], bool(ch[-1].wheel)))
    wn = [n for n, _ in R.walk(root) if n.wheel]
    print("   WheelCollider chunks on nodes: %s" % [n.name for n in wn])
    anims = [(n.name, [(c["name"], len(c["curves"])) for a in n.anims for c in a["clips"]]) for n, _ in R.walk(root) if n.anims]
    print("   anims: %s" % anims)
