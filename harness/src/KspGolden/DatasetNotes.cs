using System.Collections.Generic;

namespace KspGolden
{
    /// <summary>Hand-written frame/unit/input notes appended to the generated per-dataset README.md files.</summary>
    public static class DatasetNotes
    {
        const string Common =
            "## Conventions\n\n" +
            "- Numbers are exact: doubles are written as the shortest of G15/G16/G17 that round-trips, floats as the shortest of G6..G9; " +
            "NaN and infinities are the strings \"NaN\", \"Infinity\", \"-Infinity\". Columns typed `f32` hold float32 values (compare in float32 ULPs).\n" +
            "- Rows are identified by the table's `key` columns; compare with `python -I harness/compare/compare.py`.\n" +
            "- Units: SI unless stated (m, s, kg, K); KSP pressure is kPa in CelestialBody and atm in engine curves; masses in engine tables are tonnes; " +
            "forces from FlightIntegrator/ModuleEngines/ModuleLiftingSurface are kN.";

        public static IEnumerable<string> For(string dataset)
        {
            yield return Common;
            switch (dataset)
            {
                case "curves_eval":
                    yield return "## Inputs\n\n- PhysicsGlobals curves after Physics.cfg load; LiftingSurface curve sets; every CelestialBody FloatCurve as deserialized from the PSystem prefab; " +
                                 "every node with `key =` values in the stock part cfgs (harness/inputs/GameData/Squad/Parts), loaded with KSP's ConfigNode.Load + FloatCurve.Load; synthetic edge cases.\n" +
                                 "- Curve ids: `PhysicsGlobals.<field>`, `LiftingSurface.<set>.<curve>`, `Body.<body>.<field>`, `cfg:<relative file>:<node path>`, `synthetic.<case>`.\n\n" +
                                 "## Semantics a port must reproduce\n\n" +
                                 "- Evaluation (Unity AnimationCurveTpl<float>::Evaluate): outside [first,last] key time the value is clamped (t >= last returns last value exactly, t < first returns first value); " +
                                 "inside, segment [L,R] with L.time <= t < R.time, dx = R.time-L.time, length = max(1e-4f, dx), d0 = length*L.outTangent, d1 = length*R.inTangent, dy = R.value-L.value, " +
                                 "inv = 1/(length*length); a = (((d1+d0)-dy)-dy)*inv/length; b = (((dy+dy)+dy)-d0-d0-d1)*inv; c = L.outTangent; d = L.value; result = ((a*u+b)*u+c)*u+d with u = t-L.time. " +
                                 "All float32, in this operation order. If L.outTangent or R.inTangent is +Infinity the segment is constant L.value (-Infinity is not special). One key: that value. No keys: 0.\n" +
                                 "- FloatCurve.Load: values split on ' ', ',', ';', tab; exactly 4 numbers -> Keyframe(t,v,in,out) via AddKey (no smoothing); any other count >= 2 -> AddKey(t,v), " +
                                 "which inserts the key and recomputes the tangents of the key and both neighbours (first/last key: one-sided slope; middle: 0.5*m_right + 0.5*m_left with " +
                                 "slopes zeroed when |dx| <= 1e-5). A later 2-value key therefore overwrites explicit tangents of its neighbours. A key whose time equals an existing key is dropped.";
                    break;
                case "body_catalog":
                    yield return "## Frames and inputs\n\n- Inputs: golden/serialized/celestial_bodies.serialized.json (extracted from sharedassets9.assets by tools/extract_assets.py).\n" +
                                 "- Orbit vectors are in KSP's 'zup' frame (Orbit.pos/vel: Planetarium.Zup.WorldToLocal of the orbital-frame vectors; Unity world = .xzy swizzle). " +
                                 "Kerbin's OrbitDriver has reverse = true (KSP keeps Kerbin fixed and moves the Sun; irrelevant for relative orbits).\n" +
                                 "- G = 6.67408e-11, g0 = 9.80665. gravParameter is computed as (R*R*(GeeASL*g0)/G)*G, so it is not exactly R^2*g0*GeeASL.";
                    break;
                case "orbit_propagation":
                    yield return "## Frames\n\n- Elements: inclination, LAN, argument of periapsis in degrees; anomalies in radians; sma < 0 for hyperbolic orbits (KSP convention).\n" +
                                 "- Vectors: KSP 'zup' frame as returned by Orbit.getRelativePositionAtUT / getOrbitalVelocityAtUT (right-handed, z = orbit-normal of an equatorial prograde orbit; " +
                                 "Unity world coordinates are the .xzy swizzle). Planetarium.Zup = PlanetaryFrame(0, 90, 0) which is the identity up to ~6e-17 cross terms.\n" +
                                 "- Solver branches: e < 0.8 Newton (|dE| <= 1e-7, unbounded iterations), 0.8 <= e < 1 eight fixed iterations of a Laguerre-type update, e >= 1 hyperbolic Newton (|dE| <= 1e-7).";
                    break;
                case "patched_conics":
                    yield return "## Inputs\n\n- Start orbits are built in OrbitGen.PatchCases (Kerbin LEO, Kerbin->Mun transfer timed for an encounter, Kerbin escape, Mun escape, Kerbin orbit crossing Mun/Minmus). " +
                                 "Body orbits are the stock ones at their in-game elements (body_catalog/orbits). Times are UT seconds.";
                    break;
                case "atmosphere_curves":
                    yield return "## Inputs\n\n- Altitude in m above sea level; speed in m/s (air-relative); sunDot = dot(sun direction, local up); atmTempOffset = the latitude/sun temperature offset normally produced " +
                                 "by CelestialBody.GetAtmoThermalStats (needs scene transforms; capture-spec item C4).\n- Pressures in kPa (staticPressurekPa) and atm (staticPressureAtm = kPa * 0.009869232667160128).";
                    break;
                case "aero_forces":
                    yield return "## Frames\n\n- Part-local Unity coordinates (left-handed, +Y = part 'up' / stack axis). Face order XP, XN, YP, YN, ZP, ZN. flow_dir = air-relative velocity direction in part space.\n" +
                                 "- In game FlightIntegrator multiplies AreaDrag by PhysicsGlobals.DragCubeMultiplier (0.1), pseudo-Reynolds multiplier, q (kPa) and DragMultiplier (8): see drag_chain_kerbin.";
                    break;
                case "engine_thrust":
                    yield return "## Inputs\n\n- Engine modules from the stock part cfgs (harness/inputs). Pressure in atm. Kerbin conditions from Kerbin's real atmosphere functions. " +
                                 "rocket_equation: masses in tonnes, thrust kN, Isp s, g0 = 9.80665.";
                    break;
                case "confignode_roundtrip":
                    yield return "## Files\n\n- saved/<id>.cfg: exact bytes written by ConfigNode.Save (CRLF, UTF-8 without BOM). trees/<id>.json: parsed tree. synthetic_inputs/<id>.cfg: the exact synthetic input bytes.\n" +
                                 "- Notable behaviour: '//' starts a comment anywhere (so 'http://x' becomes 'http:'), comments are not preserved, node ids ('PART (id)') are kept by ToString but dropped by Save, " +
                                 "a stray '}' at root level ends parsing (everything after it is lost), bare tokens not followed by '{' are dropped, '{'/'}' on the same line as text are split out.";
                    break;
                case "math_shim":
                    yield return "## Scope\n\n- Only managed code is evaluated. Unity's native Quaternion/Vector3 functions and KSP's QuaternionD members that are declared extern in Assembly-CSharp " +
                                 "(Euler, FromToRotation, Slerp, Lerp, AxisAngle, ToEuler) are capture-spec items (CAPTURE-SPEC.md C3); note those QuaternionD externs are not registered by Unity either.";
                    break;
            }
        }
    }
}
