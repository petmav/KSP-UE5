"""WP-29d: print the wheel-relevant content of the staged .mu models (transform tree, colliders, WheelCollider chunk, animation clips)."""
import os, sys, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mu_wheel_reader as R

def r3(v): return "(" + ",".join("%.4g" % x for x in v) + ")"

def dump(node, depth, full):
    extra = []
    if node.tag is not None: extra.append("tag=%s/L%d" % (node.tag, node.layer))
    for c in node.colliders:
        extra.append("%s%s" % (c["kind"], "(T)" if c["trigger"] else ""))
    if node.wheel: extra.append("WHEEL r=%.4g D=%.4g c=%s k=%.4g d=%.4g t=%.4g m=%.4g" % (node.wheel["radius"], node.wheel["suspensionDistance"], r3(node.wheel["center"]), node.wheel["spring"], node.wheel["damper"], node.wheel["targetPosition"], node.wheel["mass"]))
    for a in node.anims:
        for cl in a["clips"]:
            ts = [k[0] for c in cl["curves"] for k in c["keys"]]
            extra.append("ANIM %s wrap=%d curves=%d t=[%.4g..%.4g]" % (cl["name"], cl["wrap"], len(cl["curves"]), min(ts) if ts else 0, max(ts) if ts else 0))
    print("  " * depth + node.name, "p=" + r3(node.pos), "q=" + r3(node.rot), "s=" + r3(node.scl), " ".join(extra))
    for c in node.children:
        dump(c, depth + 1, full)

for f in sorted(glob.glob(sys.argv[1] + "/*/*.mu")):
    ver, root = R.read_wheel_mu(open(f, "rb").read())
    print("=== %s v%d" % (os.path.basename(os.path.dirname(f)), ver))
    dump(root, 0, False)
