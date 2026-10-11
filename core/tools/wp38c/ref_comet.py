"""WP-38 slice 3 reference, part 5: CometSampleContract generation (FinePrint/Contracts/CometSampleContract.cs) over a fixture set of discovered comets.

Independent of kspcore. CometManager / the comet vessels are not ported (CometManagerSeam::discoveredComets): the comets are FIXTURES made here (a name, a
persistentId, a comet type name and heliocentric orbit elements; ApR = (1 + e) * a is Orbit.ApR). Everything else is the game's: the contract defaults of
Contracts.cfg, the progress model, agents, mentalities, TextGen, the localization tags.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
for sub in ("wp37", "wp38", "wp38b"):
    sys.path.insert(0, os.path.join(CORE, "tools", sub))
import contract_reference as w37  # noqa: E402
import fineprint_reference as w38  # noqa: E402
import fineprint2_reference as w38b  # noqa: E402
import ref_sentinel as rs  # noqa: E402
import ref_templates as rt  # noqa: E402

KEY = "comet"
F = w37.F
LOC = w38b.LOC
HOME = w38.HOME
BD = w38.BD

# (persistentId, name, comet type name, inc, ecc, sma, lan, argpe, mean anomaly at epoch)
COMETS = [
    (1101, "Comet Alpha", "Short", 12.0, 0.30, 3.0e10, 20.0, 40.0, 0.5),       # ApR 3.9e10: below 6 x home sma
    (1102, "Comet Beta", "Medium", 33.0, 0.50, 1.0e11, 120.0, 10.0, 1.0),      # ApR 1.5e11: between 6 x and 15 x
    (1103, "Comet Gamma", "Long", 80.0, 0.90, 2.0e11, 220.0, 70.0, 2.0),       # ApR 3.8e11: beyond 15 x
    (1104, "Comet Delta", "Hyperbolic", 5.0, 1.50, -5.0e10, 300.0, 15.0, 0.1), # hyperbolic: negative sma, negative ApR
    (1105, "Comet Epsilon", "Short", 1.0, 0.00, 1.5e11, 0.0, 0.0, 0.0),        # ApR 1.5e11 (between), circular
]


def apr(c):
    return (1.0 + c[4]) * c[5]


def synopsis(c, name):
    home = BD[HOME]["sma"]
    sma, ap = c[5], apr(c)
    if sma < 0.0:
        return LOC.fmt("#autoLOC_8012044", name, LOC.fmt("#autoLOC_8012049"))
    if ap < home * 6.0:
        return LOC.fmt("#autoLOC_8012044", name, LOC.fmt("#autoLOC_8012047"))
    if ap > home * 6.0 and ap < home * 15.0:
        return LOC.fmt("#autoLOC_8012044", name, LOC.fmt("#autoLOC_8012048"))
    if ap > home * 15.0:
        return LOC.fmt("#autoLOC_8012044", name, LOC.fmt("#autoLOC_8012049"))
    return LOC.fmt("#autoLOC_8012044", name)


def gen_comet(Vf, seed, p, s, W, mult, guid):
    where = lambda n: n != HOME
    if not w38b.bp(s, "LANDING", True, "ANY", where, reached=True):  # MeetRequirements
        return None
    rng = w37.Unity(seed)
    index = rng.range_i(0, len(COMETS))
    c0 = COMETS[index]
    name = c0[1]
    subject = "cometSample_" + c0[2] + "@SunInSpaceHigh_" + name
    ap = apr(c0)
    home = BD[HOME]["sma"]
    newp = 1 if (ap > 0.0 and ap < home * 15.0) else 2
    c = w38b.CC(newp, mult)
    ss = w38b.ss_hash(guid, seed)
    traits = {}
    for nd in w37.parse_cfg(os.path.join(w37.SQUAD, "Experience", "Traits.cfg"))["nodes"]:
        if nd["name"] == "EXPERIENCE_TRAIT":
            traits[w37.val(nd, "name")] = w37.val(nd, "title")
    disp = traits.get("Scientist", "Scientist").lower()
    params = [{"type": "CrewTraitParameter", "hash": ss, "title": LOC.fmt("#autoLOC_6001095", disp, LOC.fmt("#autoLOC_8004217"))},
              {"type": "CometScienceParameter", "hash": name + str(c0[0]), "title": LOC.fmt("#autoLOC_8012045", name)}]
    kws = []
    w37.add_keywords(kws, "Scientific")
    c.set_expiry_draw(rng.range_i(Vf("CometSample.Expire.MinimumExpireDays"), Vf("CometSample.Expire.MaximumExpireDays")))
    c.set_deadline_days(float(F(float(Vf("CometSample.Expire.DeadlineDays")) * 1.0)))
    sub = ["Trivial", "Significant", "Exceptional"][newp]
    r = lambda x: float(w37.round_to_int(F(x)))
    m3 = lambda a, b: F(a * 1.0 * b)
    c.set_science(r(m3(Vf("CometSample.Science.BaseReward"), Vf("CometSample.%s.ScienceMultiplier" % sub))))
    c.set_rep(r(m3(Vf("CometSample.Reputation.BaseReward"), Vf("CometSample.%s.ReputationMultiplier" % sub))),
              r(m3(Vf("CometSample.Reputation.BaseFailure"), Vf("CometSample.%s.ReputationMultiplier" % sub))))
    c.set_funds(r(m3(Vf("CometSample.Funds.BaseAdvance"), Vf("CometSample.%s.FundsMultiplier" % sub))), r(m3(Vf("CometSample.Funds.BaseReward"), Vf("CometSample.%s.FundsMultiplier" % sub))),
                r(m3(Vf("CometSample.Funds.BaseFailure"), Vf("CometSample.%s.FundsMultiplier" % sub))), None)
    return rt.finish2("CometSampleContract", "CometSample", newp, seed, mult, guid, rng, kws, c, params, LOC.fmt("#autoLOC_8012043", name), synopsis(c0, name), name,
                      LOC.fmt("#autoLOC_6002299"),
                      {"cometId": c0[0], "cometName": name, "subjectId": subject, "completed": LOC.fmt("#autoLOC_8012046", name)})


def build(cs_root, mirror):
    loaded, defaults = rs.defs(cs_root, mirror)

    def Vf(path):
        t = defaults[path][0]
        x = loaded[path]
        return F(float(x)) if t == "float" else x

    W = w38b.weights()
    cases = []
    for si, scn in enumerate(rt.SCENARIOS4):
        s = w38.Scn(scn["bodies"], scn["top"])
        for p in range(3):
            for seed in range(1, 17):
                for mi, mult in enumerate(w37.MULT_SETS[:2]):
                    if mi == 1 and seed > 4:
                        continue
                    sd = seed * 7001 + 13 + p
                    g = bytes(((b + seed * 3 + p) & 0xFF) for b in w38b.GUID)
                    cases.append({"scenario": si, "prestige": p, "seed": sd, "mult": mi, "guid": list(g), "result": gen_comet(Vf, sd, p, s, W, mult, g)})
    gen = sum(1 for c in cases if c["result"])
    kinds = {}
    for c in cases:
        if c["result"]:
            kinds[c["result"]["cometName"]] = kinds.get(c["result"]["cometName"], 0) + 1
    print("comet sample contracts: %d cases, %d generated %s" % (len(cases), gen, sorted(kinds.items())))
    return {"comets": [list(c) for c in COMETS], "contracts": cases, "scenarios": rt.SCENARIOS4}
