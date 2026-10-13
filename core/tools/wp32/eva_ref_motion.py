"""WP-32 reference, motion families: walk / run / low-g bound blending, UpdateMovement, UpdateHeading, the orientation PID, the jetpack (linear,
angular, fuel). Each function cites the KerbalEVA.cs lines it translates."""
from eva_common import anim_string, anim_table, cfg_float
from eva_math import (ROTATIONS, axes, bits, clamp, clamp01, clamp_magnitude, cross, dot, f32, inverse_lerp, lerp, macos, magnitude, mpow, normalize,
                      qconj, qmul_vec, sign, v_approx_zero, vadd, vdiv, vlerp, vmul, vsub)

T = anim_table()


class LCG:
    def __init__(self, seed):
        self.s = seed & 0xFFFFFFFF

    def next(self):
        self.s = (self.s * 1664525 + 1013904223) & 0xFFFFFFFF
        return self.s

    def uni(self, lo, hi):
        return lo + (hi - lo) * (self.next() >> 8) / float(1 << 24)

    def pick(self, seq):
        return seq[self.next() % len(seq)]


class Anim:
    """the Animation component: call log + per-state attributes"""

    def __init__(self, jetpack):
        self.jp = jetpack
        self.calls = []
        self.state = {}

    def s(self, name):
        return self.state.setdefault(name, dict(speed=1.0, nspeed=0.0, ntime=0.0, weight=0.0))

    def name(self, field):
        return anim_string(T, field, self.jp)

    def cross_fade(self, name, fade=0.3, mode=0):
        self.calls.append(["CrossFade", name, bits(fade), bits(0.0), mode])

    def blend(self, name, weight=1.0, fade=0.3):
        self.calls.append(["Blend", name, bits(weight), bits(fade), 0])


def rnd_vec(rng):
    """a direction-ish input: a mix of exact axes, zeros and random components"""
    def c():
        r = rng.next() % 5
        if r == 0:
            return 0.0
        if r == 1:
            return 1.0
        if r == 2:
            return -1.0
        return f32(rng.uni(-1.0, 1.0))
    return (c(), c(), c())


def unit_or(v, dflt):
    n = normalize(v)
    return dflt if n == (0.0, 0.0, 0.0) else n


def b3(v):
    return [bits(x) for x in v]


def fps_family():
    """walk_fps_OnUpdate (:2074), run_fps_OnUpdate (:2148), bound_gr_fps_OnUpdate (:2233)"""
    rng = LCG(32011)
    walk, run, strafe, bound = cfg_float("walkSpeed"), cfg_float("runSpeed"), cfg_float("strafeSpeed"), cfg_float("boundSpeed")
    min_walk, attack = cfg_float("minWalkingGee"), cfg_float("boundAttack")
    cases = []
    gees = [1.0, 0.5, 0.17, 0.3, 0.9, 0.0, 2.5]
    for kind in ("walk", "run", "bound"):
        for ri, rot in enumerate(ROTATIONS):
            for _ in range(14):
                up, fwd, right = axes(rot)
                tgt = rnd_vec(rng)
                gee = rng.pick(gees)
                jp = bool(rng.next() & 1)
                ns, nt = f32(rng.uni(0.0, 2.0)), f32(rng.uni(0.0, 1.0))
                last_step = f32(rng.uni(0.1, 3.0))
                a = Anim(jp)
                a.s(a.name("walkLowGee"))
                num = dot(tgt, fwd)
                num2 = clamp01(num)
                num3 = clamp01(f32(0.0 - num))
                num4 = dot(tgt, right)
                num5 = clamp01(num4)
                num6 = clamp01(f32(0.0 - num4))
                out = {}
                gee_f = f32(gee)
                if kind in ("walk", "run"):
                    if kind == "walk":
                        tgt_speed = f32(walk * (num2 + num3) + strafe * (num6 + num5))
                    else:
                        tgt_speed = f32(run * num2 + walk * num3 + strafe * (num6 + num5))
                    main = "walkFwd" if kind == "walk" else "run"
                    if num2 > f32(0.01):
                        a.cross_fade(a.name(main), f32(0.2))
                        a.blend(a.name("walkLowGee"), inverse_lerp(1.0, min_walk, gee_f))
                        a.s(a.name("walkLowGee"))["speed"] = f32(2.7)
                        if num5 > 0.0:
                            a.blend(a.name("strafeRight"), num5)
                        elif num6 > 0.0:
                            a.blend(a.name("strafeLeft"), num6)
                    elif num3 > f32(0.01):
                        a.cross_fade(a.name("walkBack"), f32(0.2))
                        if num5 > 0.0:
                            a.blend(a.name("strafeRight"), num5)
                        elif num6 > 0.0:
                            a.blend(a.name("strafeLeft"), num6)
                    elif num5 > f32(0.01):
                        a.cross_fade(a.name("strafeRight"), f32(0.2))
                    elif num6 > f32(0.01):
                        a.cross_fade(a.name("strafeLeft"), f32(0.2))
                    out["lowGeeSpeed"] = bits(a.s(a.name("walkLowGee"))["speed"])
                else:
                    tgt_speed = f32(bound * (num2 + num3) + strafe * (num6 + num5))
                    s1 = s2 = None
                    if num2 > f32(0.01):
                        s1 = "walkLowGee"
                        if num5 > 0.0:
                            s2 = "strafeRight"
                        elif num6 > 0.0:
                            s2 = "strafeLeft"
                    elif num3 > f32(0.01):
                        s1 = "walkBack"
                        if num5 > 0.0:
                            s2 = "strafeRight"
                        elif num6 > 0.0:
                            s2 = "strafeLeft"
                    elif num5 > f32(0.01):
                        s1 = "strafeRight"
                    elif num6 > f32(0.01):
                        s1 = "strafeLeft"
                    step = 0.0
                    if s1 is not None:
                        n1 = a.name(s1)
                        st1 = a.s(n1)
                        st1["nspeed"], st1["ntime"] = ns, nt
                        a.cross_fade(n1, f32(attack * f32(0.5)), 0)
                        step = clamp(f32(T[s1]["length"] * f32(0.5) / last_step), 0.0, 1.0)
                        st1["speed"] = step
                        if s2 is not None:
                            n2 = a.name(s2)
                            st2 = a.s(n2)
                            a.blend(n2, f32(attack * f32(0.5)), f32(0.3))
                            st2["nspeed"] = st1["nspeed"]
                            st2["ntime"] = st1["ntime"]
                            out["s2"] = [n2, bits(st2["nspeed"]), bits(st2["ntime"])]
                        out["s1"] = [n1, bits(st1["speed"])]
                        out["boundStepSpeed"] = bits(step)   # the field is left alone when no clip is selected (tgtRpos = 0)
                cases.append(dict(kind=kind, rot=ri, tgt=b3(tgt), gee=gee, jp=jp, ns=bits(ns), nt=bits(nt), lastStep=bits(last_step),
                                  tgtSpeed=bits(tgt_speed), calls=a.calls, **out))
    return cases


def movement_family():
    """UpdateMovement (:4776), slope = identity (the hit normal equals the up axis: Quaternion.FromToRotation(up, up) is the identity)"""
    rng = LCG(32021)
    cases = []
    for ri, rot in enumerate(ROTATIONS):
        for _ in range(30):
            up, fwd, right = axes(rot)
            t_state = f32(rng.pick([0.0, 0.05, 0.1, 0.2, 0.29, 0.3, 0.31, 1.0, 5.0]))
            last_tgt = f32(rng.uni(0.0, 2.5))
            tgt_speed = f32(rng.uni(0.0, 2.5))
            tgt_rpos = rnd_vec(rng)
            cmd_dir_in = rnd_vec(rng)
            char_frame = bool(rng.next() & 1)
            fup = unit_or(rnd_vec(rng), (0.0, 1.0, 0.0))
            enter = float(f32(rng.uni(50.0, 150.0)))  # TimeAtStateEnter (double); Time.time = enter + t_state (a float)
            time = f32(enter + t_state)
            num = f32(float(time) - enter)  # (float)((double)Time.time - TimeAtStateEnter)
            if num >= f32(0.3):
                num = 1.0
            elif not num > 0.0:
                num = 0.0
            else:
                num = f32(num * f32(3.3333333))
            current_spd = lerp(last_tgt, tgt_speed, num)
            cmd_dir = cmd_dir_in
            if not v_approx_zero(tgt_rpos):
                cmd_dir = vlerp(cmd_dir, tgt_rpos if char_frame else fwd, num)
            vel = vmul(cmd_dir, current_spd)
            vel = vadd(qmul_vec((0.0, 0.0, 0.0, 1.0), vel), vmul(fup, dot(vel, fup)))
            cases.append(dict(rot=ri, enter=enter, time=bits(time), lastTgt=bits(last_tgt), tgtSpeed=bits(tgt_speed), tgtRpos=b3(tgt_rpos),
                              cmdDirIn=b3(cmd_dir_in), charFrame=char_frame, fUp=b3(fup), currentSpd=bits(current_spd), cmdDir=b3(cmd_dir), vel=b3(vel)))
    return cases


def heading_family():
    """UpdateHeading (:4793)"""
    rng = LCG(32031)
    turn = cfg_float("turnRate")
    cases = []
    for ri, rot in enumerate(ROTATIONS):
        for _ in range(30):
            tgt_fwd = unit_or(rnd_vec(rng), (0.0, 0.0, 1.0))
            tgt_rpos = rng.pick([(0.0, 0.0, 0.0), (0.0, 0.0, 1.0), (1.0, 0.0, 0.0), rnd_vec(rng)])
            delta_in = f32(rng.uni(-90.0, 90.0))
            fup = unit_or(rnd_vec(rng), (0.0, 1.0, 0.0))
            up, fwd, right = axes(rot)
            delta, ang = delta_in, None
            returned = False
            if not v_approx_zero(tgt_rpos):
                num = dot(fwd, tgt_fwd)
                if num <= -1.0 or not num < 1.0:
                    returned = True
                else:
                    delta = f32(macos(num) * f32(57.29578))
            if not returned:
                num2 = sign(qmul_vec(qconj(rot), tgt_fwd)[0])
                delta = f32(delta * num2)
                if abs(delta) < turn * 2.0:
                    ang = vmul(fup, f32(delta * f32(0.5)))
                else:
                    ang = vmul(fup, f32(turn * num2))
            cases.append(dict(rot=ri, tgtFwd=b3(tgt_fwd), tgtRpos=b3(tgt_rpos), deltaIn=bits(delta_in), fUp=b3(fup), returned=returned, delta=bits(delta),
                              ang=None if ang is None else b3(ang)))
    return cases


def pid_family():
    """UpdateOrientationPID (:4853)"""
    rng = LCG(32041)
    kp, ki, kd = cfg_float("Kp"), cfg_float("Ki"), cfg_float("Kd")
    thr_thresh, expo, mult = f32(55.0), f32(2.7), f32(0.00012)
    fdt = f32(0.02)
    cases = []
    for ri, rot in enumerate(ROTATIONS):
        for _ in range(25):
            up, fwd, right = axes(rot)
            tgt_fwd = unit_or(rnd_vec(rng), (0.0, 0.0, 1.0))
            tgt_up = unit_or(rnd_vec(rng), (0.0, 1.0, 0.0))
            integral = vmul(rnd_vec(rng), f32(rng.uni(0.0, 1.5)))
            prev = vmul(rnd_vec(rng), f32(rng.uni(0.0, 1.0)))
            thrust = f32(rng.pick([100.0, 55.0, 54.0, 30.0, 10.0, 70.0, 20.5]))
            boost_flag = bool(rng.next() & 1)
            if thrust > thr_thresh:
                boost_flag = True   # boost off above the threshold gives Math.Pow(negative, 2.7) = NaN (the sign bit of a NaN is not portable): not compared
            manual = rng.next() % 5 == 0
            out = dict(manual=manual)
            if not manual:
                if boost_flag and thrust > thr_thresh:
                    boost = 1.0
                else:
                    boost = f32(1.0 + mpow(f32(thr_thresh - thrust), expo) * mult)
                error = vadd(clamp_magnitude(cross(fwd, tgt_fwd), 0.5), cross(up, tgt_up))
                integ = clamp_magnitude(vadd(integral, vmul(error, fdt)), 1.0)
                deriv = vdiv(vsub(error, prev), fdt)
                cmd = vmul(vadd(vadd(vmul(error, kp), vmul(integ, ki)), vmul(deriv, kd)), boost)
                out.update(boost=bits(boost), error=b3(error), integral=b3(integ), derivative=b3(deriv), cmdRot=b3(cmd))
            cases.append(dict(rot=ri, tgtFwd=b3(tgt_fwd), tgtUp=b3(tgt_up), integralIn=b3(integral), prevIn=b3(prev), thrust=bits(thrust),
                              boostFlag=boost_flag, **out))
    return cases


def pack_family():
    """UpdatePackLinear (:5047), UpdatePackAngular (:4873), UpdatePackFuel (:5060)"""
    rng = LCG(32051)
    lin_power = f32(cfg_float("linPower") * cfg_float("massMultiplier"))   # StartEVA: linPower *= massMultiplier
    rot_power, mass_mult = cfg_float("rotPower"), cfg_float("massMultiplier")
    consumption = cfg_float("PropellantConsumption")
    fdt = f32(0.02)
    cases = []
    for _ in range(120):
        thrust = f32(rng.pick([100.0, 50.0, 10.0, 77.5, 33.3]))
        pack_tgt = vmul(rnd_vec(rng), f32(rng.pick([0.0, 1.0, 1.0, 0.5, 2.0])))
        cmd_rot = vmul(rnd_vec(rng), f32(rng.pick([0.0, 1.0, 0.1, 0.5])))
        fuel = rng.pick([0.0, 0.001, 1.0, 5.0])
        jet = rng.next() % 6 != 0
        packed = rng.next() % 9 == 0
        ragdoll = rng.next() % 9 == 0
        restricted = False   # EVAConstructionModeController.MovementRestricted is exercised by its own test (needs the construction controller)
        rng.next()
        ffr_in = f32(rng.pick([0.0, 0.1]))
        forces, torques = [], []
        ffr = ffr_in
        pack_linear = (0.0, 0.0, 0.0)
        if jet and not packed and not ragdoll and not restricted:
            pack_linear = vmul(pack_tgt, f32(thrust * f32(0.01)))
            if not v_approx_zero(pack_linear) and fuel > 0.0:
                forces.append(vmul(pack_linear, lin_power))
                ffr = f32(ffr + magnitude(pack_linear) * fdt)
        if jet and not packed and not v_approx_zero(cmd_rot) and fuel > 0.0:
            torques.append(vmul(vmul(vmul(cmd_rot, mass_mult), rot_power), f32(thrust * f32(0.01))))
            ffr = f32(ffr + magnitude(cmd_rot) * fdt)
        cases.append(dict(thrust=bits(thrust), packTgt=b3(pack_tgt), cmdRot=b3(cmd_rot), fuel=fuel, jet=jet, packed=packed, ragdoll=ragdoll,
                          restricted=restricted, ffrIn=bits(ffr_in), packLinear=b3(pack_linear), forces=[b3(f) for f in forces],
                          torques=[b3(t) for t in torques], ffr=bits(ffr)))
    # fuel drain over the inventory snapshots (the caller's order is the sorted priority order); amounts in double
    drains = []
    for _ in range(60):
        n = 1 + rng.next() % 4
        snaps = [float(f32(rng.uni(0.0, 3.0))) if rng.next() % 5 else 0.0 for _ in range(n)]
        res = float(sum(snaps))
        ffr = f32(rng.uni(0.0, 4.0))
        num = float(consumption) * float(ffr)                  # double num = PropellantConsumption * fuelFlowRate (exact product)
        # Part.TransferResource(resource, -num, part) on the off-list propellant resource (Part.cs:6600, negative branch)
        res_after = 0.0 if num >= res else res - num
        left = num
        after = list(snaps)
        for i in range(len(snaps)):
            if after[i] <= left:
                left -= after[i]
                after[i] = 0.0
            else:
                after[i] -= left
                left = 0.0
            if left <= 0.0:
                break
        drains.append(dict(snaps=snaps, ffr=bits(ffr), resource=res, resourceAfter=res_after, snapsAfter=after))
    return dict(linear=cases, drains=drains)
