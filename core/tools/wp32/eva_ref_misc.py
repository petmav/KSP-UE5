"""WP-32 reference, further families: helmet safety (CheckHelmetOffSafe), visor animation, jump / bound impulses, the low-g collider curve, clamber path
points, ladder bookkeeping (trigger sorting, ladder switching, ladder movement), part placement mode, flag rules, CrewTransfer part rules, camera bob.
Each function cites the C# lines it translates."""
import math

from eva_common import cfg_float
from eva_math import (F3, ROTATIONS, axes, bits, clamp, clamp01, dot, f32, lerp, magnitude, mpow, mround, normalize, project_on_plane, qmul_vec, sign,
                      sqr_magnitude, v_approx_zero, vadd, vlerp, vmul, vsub, cross)
from eva_ref_motion import LCG, b3, rnd_vec, unit_or


def mf(name):
    return cfg_float(name)


# ---- helmet: CheckHelmetOffSafe (KerbalEVA.cs:7158) with startEVAChecks = false -----------------------------------------------------------------
def helmet_family():
    rng = LCG(32101)
    pressures = [0.0, 0.1, 0.177, 0.2, 0.214, 0.215, 0.22, 1.0, 4.79, 4.8, 5.8, 14.99, 15.0, 15.01, 19.9, 20.0, 25.0, 5.81, 4.81]
    temps = [100.0, 222.9, 223.0, 232.9, 233.0, 300.0, 322.9, 323.0, 333.0, 333.1, 400.0]
    cases = []
    for _ in range(600):
        atm = rng.next() % 6 != 0
        depth = rng.pick([70000.0, 0.0, 1000.0])
        alt = rng.pick([-10.0, 0.0, 500.0, 69999.0, 70000.0, 80000.0])
        oxy = rng.next() % 5 != 0
        press = rng.pick(pressures)
        skin, temp = rng.pick(temps), rng.pick(temps)
        splashed = rng.next() % 4 == 0
        margins = rng.next() % 3 != 0
        # --- translation
        if not (atm and alt < depth):
            ok, why = False, "L6010008"
        elif not oxy:
            ok, why = False, "L8003204"
        else:
            m = lambda n: float(mf(n)) if margins else 0.0   # (includeSafetyMargins ? margin : 0f); the float sum is widened, not rounded
            if press < float(mf("helmetOffMinSafePressureAtm")) + m("helmetOffMinSafePressureAtmMargin"):
                ok, why = False, "L6010009"
            elif press > float(mf("helmetOffMaxSafePressureAtm")) - m("helmetOffMaxSafePressureAtmMargin"):
                ok, why = False, "L6006049"
            elif splashed and press > float(mf("helmetOffMaxOceanPressureAtm")) - m("helmetOffMaxOceanPressureAtmMargin"):
                ok, why = False, "L6010015"
            else:
                if (not (skin < float(mf("helmetOffMinSafeTempK")) + m("helmetOffMinSafeTempKMargin"))) and temp >= float(mf("helmetOffMinSafeTempK")):
                    if (not (skin > float(mf("helmetOffMaxSafeTempK")) - m("helmetOffMaxSafeTempKMargin"))) and temp <= float(mf("helmetOffMaxSafeTempK")):
                        ok, why = True, ""
                    else:
                        ok, why = False, "L6010011"
                else:
                    ok, why = False, "L6010010"
        cases.append(dict(atm=atm, depth=depth, alt=alt, oxy=oxy, press=press, skin=skin, temp=temp, splashed=splashed, margins=margins, ok=ok, why=why))
    return cases


# ---- visor: UpdateVisorPosition (:6164) ------------------------------------------------------------------------------------------------------------
def visor_family():
    speed = mf("VisorAnimationSpeed")
    dts = [f32(0.02), f32(0.016), f32(0.05)]
    cases = []
    for lowering in (True, False):
        for dt in dts:
            target = f32(0.5) if lowering else f32(0.0)
            cur = f32(0.0) if lowering else f32(0.5)
            series = []
            final = None
            for step in range(400):
                current = cur
                tgt = target
                current = lerp(current, tgt, f32(dt * speed))
                cur = current
                done = abs(f32(tgt - current)) < f32(0.001)
                series.append(bits(current))
                if done:
                    final = "Lowered" if lowering else "Raised"
                    break
            cases.append(dict(lowering=lowering, dt=bits(dt), series=series, final=final, enabled=lowering))
    return cases


# ---- impulses --------------------------------------------------------------------------------------------------------------------------------
def impulse_family():
    rng = LCG(32111)
    mm = mf("massMultiplier")
    max_jump = f32(mf("maxJumpForce") * mm)             # StartEVA: maxJumpForce *= massMultiplier
    jump_mult = mf("jumpMultiplier")
    bound_speed = mf("boundSpeed")
    reduction = f32(0.094)                                # PhysicsGlobals.perCommandSeatReduction default (Physics.cfg does not set it)
    jumps, bounds = [], []
    for ri, rot in enumerate(ROTATIONS):
        up, fwd, right = axes(rot)
        for _ in range(12):
            mass = f32(rng.pick([0.045, 0.05, 0.1, 1.0, 0.0094, f32(rng.uni(0.01, 0.2))]))
            tgt_speed = f32(rng.uni(0.0, 2.5))
            ratio = f32(mass / reduction)
            powv = mpow(ratio, jump_mult)
            imp = vadd(vmul(vmul(up, powv), max_jump), vmul(vmul(fwd, tgt_speed), mm))
            jumps.append(dict(rot=ri, mass=bits(mass), tgtSpeed=bits(tgt_speed), impulse=b3(imp)))
        for _ in range(12):
            mass = f32(rng.pick([0.045, 0.05, 0.1, 1.0, f32(rng.uni(0.01, 0.2))]))
            # a horizontal target direction (perpendicular to the transform up axis): the strip of the up component leaves it unchanged and
            # FromToRotation(tgt, vel) is the identity
            h = right if rng.next() & 1 else fwd
            sgn = 1.0 if rng.next() & 1 else -1.0
            tgt = vmul(h, f32(sgn * rng.pick([1.0, 0.5, 0.25])))
            vel_scale = f32(rng.pick([1.0, 2.0, 0.5]))
            vel_in = vmul(tgt, vel_scale)
            bf = f32(mround(f32(mass * f32(0.533) * f32(100.0))) / 100.0)
            tn = normalize(tgt)
            imp = vadd(vmul(up, bf), vmul(vmul(tn, bound_speed), mm))
            bounds.append(dict(rot=ri, mass=bits(mass), tgt=b3(tgt), velIn=b3(vel_in), boundForce=bits(bf), impulse=b3(imp)))
    return dict(jump=jumps, bound=bounds)


# ---- UpdateLowGBodyColliders (:4820) -----------------------------------------------------------------------------------------------------------
def lowg_family():
    x, y = f32(0.01), f32(0.04)
    cases = []
    for nt in [0.0, 0.05, 0.1, 0.2, 0.24999, 0.25, 0.3, 0.49, 0.5, 0.6, 0.74, 0.75, 0.8, 0.99, 1.0, 1.3, 2.75, 7.5, 123.456, -0.2]:
        for counter in (0.1, 0.3):
            nt = f32(nt)
            if not counter < 0.25:
                t = f32(math.fmod(nt, 1.0))
                d = float(y) - float(x)
                if t >= 0.0 and t < f32(0.25):
                    h = f32(d * (-4.0 * t + 1.0) + x)
                elif t >= f32(0.25) and t < f32(0.5):
                    h = f32(d * (4.0 * t - 1.0) + x)
                elif t >= f32(0.5) and t < f32(0.75):
                    h = f32(d * (-4.0 * t + 3.0) + x)
                else:
                    h = f32(d * (4.0 * t - 3.0) + x)
                cases.append(dict(nt=bits(nt), counter=counter, ran=True, nbt=bits(t), height=bits(h)))
            else:
                cases.append(dict(nt=bits(nt), counter=counter, ran=False))
    return cases


# ---- ClamberPath constructor (:47) ---------------------------------------------------------------------------------------------------------------
def clamber_family():
    rng = LCG(32121)
    cases = []
    for _ in range(60):
        e, s_, p1, p2, p3 = (rnd_vec(rng) for _ in range(5))
        h, so = f32(rng.uni(0.0, 3.0)), f32(rng.pick([0.2, 0.269, 0.3, 0.45]))
        q1 = vadd(p1, vmul(e, so))
        q2 = vadd(p2, vmul(vadd(e, s_), so))
        q3 = vadd(p3, vmul(s_, so))
        cases.append(dict(e=b3(e), s=b3(s_), p1=b3(p1), p2=b3(p2), p3=b3(p3), h=bits(h), so=bits(so), q1=b3(q1), q2=b3(q2), q3=b3(q3)))
    return cases


# ---- seat camera bob (Kerbal.cs:240) -----------------------------------------------------------------------------------------------------------
def seatcam_family():
    rng = LCG(32131)
    cases = []
    for _ in range(80):
        px, py, pz = f32(rng.uni(0, 1)), f32(rng.uni(0, 1)), f32(rng.uni(0, 1))
        acc = rng.pick([0.0, 0.5, 5.0, 9.81, 100.0, 12.345])
        ang = rng.pick([0.0, 0.5, 5.0, 50.0, 250.0])
        bob = f32(rng.pick([10.0, 0.0, 3.5]))
        num, num2, num3 = f32(px - f32(0.5)), f32(py - f32(0.5)), f32(pz - f32(0.5))
        cl = clamp(f32(f32(acc) * 5.0), 0.0, 50.0)   # (float)acc * 5f stored as the Clamp argument
        x = f32(num * bob * cl * f32(0.4))
        y = f32(num2 * bob * cl * f32(0.4))
        z = f32(num3 * bob * clamp(f32(ang), 0.0, 100.0) * f32(0.5))
        cases.append(dict(px=bits(px), py=bits(py), pz=bits(pz), acc=acc, ang=ang, bob=bits(bob), x=bits(x), y=bits(y), z=bits(z)))
    return cases


# ---- part placement mode (:7296) / flag rules (:6729) / crew transfer rules (CrewTransfer.cs:41-55) ---------------------------------------------------------
def rules_family():
    rng = LCG(32141)
    placement, flag, transfer = [], [], []
    for _ in range(200):
        mode = bool(rng.next() & 1)
        rag = rng.next() % 4 == 0
        uc = rng.next() % 4 != 0
        lad = rng.next() % 4 == 0
        landed = rng.next() % 3 != 0
        pm_in = bool(rng.next() & 1)
        if mode:
            ok = (not rag) and uc and (not lad) and landed
            result = ok
            pm = True if ok else pm_in      # a refused request leaves partPlacementMode alone
        else:
            result, pm = True, False
        placement.append(dict(mode=mode, rag=rag, uc=uc, lad=lad, landed=landed, pmIn=pm_in, result=result, pm=pm))
    for _ in range(200):
        active = rng.next() % 4 != 0
        ground = rng.next() % 3 != 0
        items = rng.pick([0, 1, 2, -1])
        rag = rng.next() % 4 == 0
        constr = rng.next() % 4 == 0
        res = active and ground and items > 0 and (not rag) and (not constr)
        flag.append(dict(active=active, ground=ground, items=items, rag=rag, constr=constr, result=res))
    for _ in range(80):
        cap = rng.pick([0, 1, 2, 3])
        n = rng.pick([0, 1, 2, 3])
        avail = bool(rng.next() & 1)
        semi = cap > 0 and n >= cap and avail
        valid = cap > 0 and n < cap and avail
        transfer.append(dict(cap=cap, n=n, avail=avail, semi=semi, valid=valid))
    return dict(placement=placement, flag=flag, transfer=transfer)


# ---- ladder bookkeeping --------------------------------------------------------------------------------------------------------------------------
def net_sort(items, cmp):
    """List<T>.Sort(Comparison) for n <= 16 (ArraySortHelper.IntrospectiveSort: sizes 2 and 3 are fixed swap sequences, larger partitions an insertion sort)"""
    a = list(items)
    n = len(a)
    if n < 2:
        return a

    def swap_if_greater(i, j):
        if i != j and cmp(a[i], a[j]) > 0:
            a[i], a[j] = a[j], a[i]
    if n == 2:
        swap_if_greater(0, 1)
        return a
    if n == 3:
        swap_if_greater(0, 1)
        swap_if_greater(0, 2)
        swap_if_greater(1, 2)
        return a
    for i in range(0, n - 1):
        j = i
        t = a[i + 1]
        while j >= 0 and cmp(t, a[j]) < 0:
            a[j + 1] = a[j]
            j -= 1
        a[j + 1] = t
    return a


def ladder_family():
    rng = LCG(32151)
    out = dict(distance=[], alignment=[], idle=[], movement=[], position=[])
    # OnTriggerStay: the triggers are added one by one and the list is sorted by the distance of ClosestPointOnBounds(ladderPivot) after each add
    for _ in range(120):
        n = 1 + rng.next() % 6
        pivot = rnd_vec(rng)
        pts = [vmul(rnd_vec(rng), f32(rng.pick([0.0, 1.0, 2.0]))) for _ in range(n)]
        # equal distances occur: duplicate a point
        if n > 2 and rng.next() % 2:
            pts[2] = pts[0]
        lst = []
        for i in range(n):
            lst.append(i)

            def cmp(c1, c2):
                v1 = vsub(pts[c1], pivot)
                v2 = vsub(pts[c2], pivot)
                return -1 if sqr_magnitude(v1) <= sqr_magnitude(v2) else 1
            lst = net_sort(lst, cmp)
        out["distance"].append(dict(pivot=b3(pivot), pts=[b3(p) for p in pts], order=lst))
    # UpdateCurrentLadder (:5858) with secondaryLadder = null and ladderTransition = false (AutoTransition returns at once)
    for _ in range(150):
        n = 1 + rng.next() % 5
        rots = [rng.next() % len(ROTATIONS) for _ in range(n)]
        ups = [axes(ROTATIONS[r])[0] for r in rots]
        ltr = rng.pick([F3(v) for v in ((0.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, -1.0, 0.0), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0))] + [rnd_vec(rng)])
        tgt_speed = f32(rng.pick([0.0, 0.6, -0.6]))
        left_held, right_held = rng.next() % 3 == 0, rng.next() % 3 == 0
        left_up, right_up = rng.next() % 3 == 0, rng.next() % 3 == 0
        cur_idx = 0
        inv_in = bool(rng.next() & 1)
        trans_in = bool(rng.next() & 1)
        # the C# body (secondary == null so AutoTransition does nothing, whatever ladderTransition is)
        lst = list(range(n))
        cur = cur_idx
        inv = inv_in
        trans = trans_in
        if not v_approx_zero(ltr) and not trans:
            ln = normalize(ltr)

            def cmp(c1, c2):
                return -1 if abs(dot(ln, ups[c1])) >= abs(dot(ln, ups[c2])) - f32(0.001) else 1
            lst = net_sort(lst, cmp)
            collider = lst[0]
            if cur != collider and abs(dot(ltr, ups[collider])) > f32(0.3):
                cur = collider
                inv = (dot(normalize(ltr), ups[cur]) * (sign(tgt_speed) if tgt_speed != 0.0 else 1.0)) < 0.0
                if left_held or right_held:
                    trans = True
        if left_up or right_up or n == 1:
            trans = False
        out["alignment"].append(dict(rots=rots, ltr=b3(ltr), tgtSpeed=bits(tgt_speed), leftHeld=left_held, rightHeld=right_held, leftUp=left_up,
                                     rightUp=right_up, curIdx=cur_idx, invIn=inv_in, transIn=trans_in, order=lst, cur=cur, inv=inv, trans=trans))
    # UpdateCurrentLadderIdle (:5895)
    for _ in range(150):
        n = 1 + rng.next() % 5
        rots = [rng.next() % len(ROTATIONS) for _ in range(n)]
        ups = [axes(ROTATIONS[r])[0] for r in rots]
        frot = rng.next() % len(ROTATIONS)
        up, fwd, right = axes(ROTATIONS[frot])
        ltr = rng.pick([F3(v) for v in ((0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (-1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.95, 0.1, 0.0))] + [rnd_vec(rng)])
        left_up, right_up = rng.next() % 3 == 0, rng.next() % 3 == 0
        trans_in = bool(rng.next() & 1)
        inv_in = bool(rng.next() & 1)
        lst = list(range(n))
        cur = 0
        inv = inv_in
        trans = trans_in
        ltr_out = ltr
        if n == 1:
            trans = False
        else:
            if abs(dot(ltr, axes(ROTATIONS[frot])[2])) > f32(0.9) and not trans:
                ltr_out = project_on_plane(ltr, axes(ROTATIONS[frot])[0])
                ln = normalize(ltr_out)

                def cmp(c1, c2):
                    return -1 if abs(dot(ln, ups[c1])) >= abs(dot(ln, ups[c2])) - f32(0.001) else 1
                lst = net_sort(lst, cmp)
                collider = lst[0]
                if cur != collider and abs(dot(ltr_out, ups[collider])) > f32(0.3):
                    cur = collider
                    inv = dot(normalize(ltr_out), ups[cur]) < 0.0
                    trans = True
            if left_up or right_up:
                trans = False
        out["idle"].append(dict(rots=rots, frot=frot, ltr=b3(ltr), leftUp=left_up, rightUp=right_up, invIn=inv_in, transIn=trans_in, order=lst, cur=cur,
                                inv=inv, trans=trans, ltrOut=b3(ltr_out)))
    # UpdateLadderMovement (:5787)
    for _ in range(120):
        rot = rng.next() % len(ROTATIONS)
        lup = vmul(axes(ROTATIONS[rot])[0], f32(rng.pick([1.0, -1.0])))
        t_state = f32(rng.pick([0.0, 0.1, 0.29, 0.3, 1.0]))
        last_tgt = f32(rng.pick([0.0, 0.6, -0.6, 0.3]))
        tgt = f32(rng.pick([0.0, 0.6, -0.6]))
        can_climb, can_desc, chk = rng.next() % 2 == 0, rng.next() % 2 == 0, rng.next() % 2 == 0
        has_rb = rng.next() % 2 == 0
        rbv, rba, rbc, com = rnd_vec(rng), rnd_vec(rng), rnd_vec(rng), rnd_vec(rng)
        enter = 10.0
        time = f32(enter + t_state)
        num = f32(float(time) - enter)
        if num >= f32(0.3):
            num = 1.0
        elif not num > 0.0:
            num = 0.0
        else:
            num = f32(num * f32(3.3333333))
        spd = lerp(last_tgt, tgt, num)
        if ((spd < 0.0 and not can_desc) or (spd > 0.0 and not can_climb)) and chk:
            spd = 0.0
        base = vadd(rbv, cross(rba, vsub(com, rbc))) if has_rb else (0.0, 0.0, 0.0)
        vel = vadd(base, vmul(lup, spd))
        out["movement"].append(dict(rot=rot, lup=b3(lup), time=bits(time), enter=enter, lastTgt=bits(last_tgt), tgt=bits(tgt), canClimb=can_climb, canDesc=can_desc,
                                    chk=chk, hasRb=has_rb, rbv=b3(rbv), rba=b3(rba), rbc=b3(rbc), com=b3(com), spd=bits(spd), vel=b3(vel)))
    # correctLadderPosition (:5749), not jointed: Vtgt, ladderPos and the lerped position
    for _ in range(100):
        rot = rng.next() % len(ROTATIONS)
        lup = axes(ROTATIONS[rot])[0]
        lpos, pivot, pos = vmul(rnd_vec(rng), 2.0), rnd_vec(rng), vmul(rnd_vec(rng), 3.0)
        dt = f32(rng.pick([0.02, 0.016, 0.05, 0.2]))
        has_rb = rng.next() % 2 == 0
        rbv, rba, rbc, com = rnd_vec(rng), rnd_vec(rng), rnd_vec(rng), rnd_vec(rng)
        vtgt = vadd(rbv, cross(rba, vsub(com, rbc))) if has_rb else (0.0, 0.0, 0.0)
        ladder_pos = project_on_plane(vsub(lpos, pivot), lup)
        new_pos = vlerp(pos, vadd(ladder_pos, pos), f32(10.0 * dt))
        out["position"].append(dict(rot=rot, lpos=b3(lpos), pivot=b3(pivot), pos=b3(pos), dt=bits(dt), hasRb=has_rb, rbv=b3(rbv), rba=b3(rba), rbc=b3(rbc),
                                    com=b3(com), vtgt=b3(vtgt), ladderPos=b3(ladder_pos), newPos=b3(new_pos)))
    return out
