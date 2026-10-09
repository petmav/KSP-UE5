using System;
using System.Collections.Generic;
using System.Linq;
using KspGolden.Infra;
using UnityEngine;

namespace KspGolden.Generators
{
    /// <summary>
    /// math_shim: the managed (C#) parts of Unity's float math (Mathf, Vector3, Quaternion operators) and KSP's double math
    /// (Vector3d, QuaternionD, UtilMath, Planetarium.CelestialFrame), evaluated directly. Unity's native maths
    /// (Quaternion.Euler/AngleAxis/LookRotation/Slerp/Inverse, Vector3.Slerp, Mathf.PerlinNoise, ...) is capture-spec only.
    /// </summary>
    public static class MathGen
    {
        static readonly float[] F = { 0f, -0f, 0.5f, -0.5f, 1.5f, 2.5f, -2.5f, 1e-7f, 3.1415927f, -7.25f, 123.456f, 359.9f, 720.5f, -1000.75f, 1e20f };

        public static IEnumerable<(string, GoldenFile)> Generate()
        {
            var g = new GoldenFile("math_shim", "direct",
                "Direct evaluations of managed Unity 2019.4 math (UnityEngine.CoreModule IL: Mathf/Vector3/Quaternion operators; float32) " +
                "and KSP double-precision math (Vector3d, UnityEngine.QuaternionD in Assembly-CSharp, UtilMath, Planetarium.CelestialFrame). " +
                "Columns a0..a5 are inputs, r0..r3 outputs; unused slots are null. Note: Mathf functions that wrap System.Math use the .NET " +
                "Framework CRT here, not Unity's Mono/UCRT, so transcendental results may differ by 1 ulp from in-game values.");
            g.Oracle("UnityEngine.Mathf.*").Oracle("UnityEngine.Vector3.*").Oracle("UnityEngine.Quaternion operator*/Dot/Angle/Normalize")
             .Oracle("Vector3d.*").Oracle("QuaternionD.AngleAxis/Inverse/LookRotation/operator*/Angle").Oracle("UtilMath.*").Oracle("Planetarium.CelestialFrame.SetFrame/OrbitalFrame/PlanetaryFrame");
            var t32 = g.AddTable(new Table("f32", "Unity float math."));
            var t64 = g.AddTable(new Table("f64", "KSP double math."));
            foreach (var t in new[] { t32, t64 })
            {
                var ct = t == t32 ? ColType.F32 : ColType.F64;
                t.Col("fn", ColType.Str).Col("i", ColType.I64);
                for (int k = 0; k < 6; k++) t.Col("a" + k, ct);
                for (int k = 0; k < 4; k++) t.Col("r" + k, ct);
                t.Keys("fn", "i");
            }
            var c32 = new Dictionary<string, int>();
            void A32(string fn, float[] a, params float[] r)
            {
                c32.TryGetValue(fn, out int i); c32[fn] = i + 1;
                var row = new object[12]; row[0] = fn; row[1] = i;
                for (int k = 0; k < a.Length; k++) row[2 + k] = a[k];
                for (int k = 0; k < r.Length; k++) row[8 + k] = r[k];
                t32.Add(row);
            }
            var c64 = new Dictionary<string, int>();
            void A64(string fn, double[] a, params double[] r)
            {
                c64.TryGetValue(fn, out int i); c64[fn] = i + 1;
                var row = new object[12]; row[0] = fn; row[1] = i;
                for (int k = 0; k < a.Length; k++) row[2 + k] = a[k];
                for (int k = 0; k < r.Length; k++) row[8 + k] = r[k];
                t64.Add(row);
            }
            float B(bool b) => b ? 1f : 0f;
            foreach (var x in F)
            {
                A32("Mathf.Round", new[] { x }, Mathf.Round(x));
                A32("Mathf.RoundToInt", new[] { x }, (float)Mathf.RoundToInt(Math.Abs(x) < 1e9f ? x : 0f));
                A32("Mathf.Sign", new[] { x }, Mathf.Sign(x));
                A32("Mathf.Clamp01", new[] { x }, Mathf.Clamp01(x));
                A32("Mathf.Sqrt", new[] { x }, Mathf.Sqrt(x));
                A32("Mathf.Sin", new[] { x }, Mathf.Sin(x));
                A32("Mathf.Cos", new[] { x }, Mathf.Cos(x));
                A32("Mathf.Exp", new[] { x }, Mathf.Exp(x));
                A32("Mathf.Log", new[] { x }, Mathf.Log(x));
                A32("Mathf.Floor", new[] { x }, Mathf.Floor(x));
                A32("Mathf.Ceil", new[] { x }, Mathf.Ceil(x));
                A32("Mathf.Repeat", new[] { x, 360f }, Mathf.Repeat(x, 360f));
                A32("Mathf.PingPong", new[] { x, 3f }, Mathf.PingPong(x, 3f));
                A32("Mathf.DeltaAngle", new[] { x, 10f }, Mathf.DeltaAngle(x, 10f));
                A32("Mathf.Approximately", new[] { x, x + 1e-6f }, B(Mathf.Approximately(x, x + 1e-6f)));
                foreach (var y in new[] { 0.25f, 2f, -3f })
                {
                    A32("Mathf.Pow", new[] { x, y }, Mathf.Pow(x, y));
                    A32("Mathf.Atan2", new[] { x, y }, Mathf.Atan2(x, y));
                    A32("Mathf.Lerp", new[] { 2f, 7f, x / 3f }, Mathf.Lerp(2f, 7f, x / 3f));
                    A32("Mathf.LerpUnclamped", new[] { 2f, 7f, x / 3f }, Mathf.LerpUnclamped(2f, 7f, x / 3f));
                    A32("Mathf.InverseLerp", new[] { y, 7f, x }, Mathf.InverseLerp(y, 7f, x));
                    A32("Mathf.MoveTowards", new[] { x, y, 0.75f }, Mathf.MoveTowards(x, y, 0.75f));
                    A32("Mathf.SmoothStep", new[] { y, 7f, x / 5f }, Mathf.SmoothStep(y, 7f, x / 5f));
                    A32("Mathf.Clamp", new[] { x, y, 5f }, Mathf.Clamp(x, y, 5f));
                }
            }
            var V = new[] { new Vector3(0, 0, 0), new Vector3(1, 0, 0), new Vector3(3, 4, 12), new Vector3(-0.3f, 0.9f, -0.2f), new Vector3(1e-6f, 2e-6f, -1e-6f), new Vector3(1000.5f, -250.25f, 0.125f), new Vector3(0, 1, 0) };
            foreach (var a in V)
            {
                var n = a.normalized;
                A32("Vector3.normalized", new[] { a.x, a.y, a.z }, n.x, n.y, n.z);
                A32("Vector3.magnitude", new[] { a.x, a.y, a.z }, a.magnitude, a.sqrMagnitude);
                var cm = Vector3.ClampMagnitude(a, 2f);
                A32("Vector3.ClampMagnitude2", new[] { a.x, a.y, a.z }, cm.x, cm.y, cm.z);
                foreach (var b in V)
                {
                    var c = Vector3.Cross(a, b);
                    A32("Vector3.Cross", new[] { a.x, a.y, a.z, b.x, b.y, b.z }, c.x, c.y, c.z);
                    A32("Vector3.Dot", new[] { a.x, a.y, a.z, b.x, b.y, b.z }, Vector3.Dot(a, b));
                    A32("Vector3.Angle", new[] { a.x, a.y, a.z, b.x, b.y, b.z }, Vector3.Angle(a, b));
                    var p = Vector3.Project(a, b);
                    A32("Vector3.Project", new[] { a.x, a.y, a.z, b.x, b.y, b.z }, p.x, p.y, p.z);
                    var q = Vector3.ProjectOnPlane(a, b);
                    A32("Vector3.ProjectOnPlane", new[] { a.x, a.y, a.z, b.x, b.y, b.z }, q.x, q.y, q.z);
                }
            }
            var Q = new[] { new Quaternion(0, 0, 0, 1), new Quaternion(0.70710677f, 0, 0, 0.70710677f), new Quaternion(0.1f, -0.2f, 0.3f, 0.9f), new Quaternion(0.5f, 0.5f, -0.5f, 0.5f) };
            foreach (var q in Q)
            {
                foreach (var v in V.Take(4))
                {
                    var r = q * v;
                    A32("Quaternion*Vector3", new[] { q.x, q.y, q.z, q.w, v.x, v.y }, r.x, r.y, r.z);
                }
                foreach (var q2 in Q)
                {
                    var m = q * q2;
                    A32("Quaternion*Quaternion", new[] { q.x, q.y, q.z, q.w, q2.x, q2.y }, m.x, m.y, m.z, m.w);
                    A32("Quaternion.Angle", new[] { q.x, q.y, q.z, q.w, q2.x, q2.y }, Quaternion.Angle(q, q2), Quaternion.Dot(q, q2));
                }
                var nq = Quaternion.Normalize(q);
                A32("Quaternion.Normalize", new[] { q.x, q.y, q.z, q.w }, nq.x, nq.y, nq.z, nq.w);
            }

            // ---- KSP double math
            var D = new[] { new Vector3d(0, 0, 0), new Vector3d(1, 0, 0), new Vector3d(3, 4, 12), new Vector3d(-0.3, 0.9, -0.2), new Vector3d(1e-9, 2e-9, -1e-9), new Vector3d(13599840256.0, -21658398.5, 0.125), new Vector3d(0, 0, 1) };
            foreach (var a in D)
            {
                var n = a.normalized;
                A64("Vector3d.normalized", new[] { a.x, a.y, a.z }, n.x, n.y, n.z);
                A64("Vector3d.magnitude", new[] { a.x, a.y, a.z }, a.magnitude, a.sqrMagnitude);
                foreach (var b in D)
                {
                    var c = Vector3d.Cross(a, b);
                    A64("Vector3d.Cross", new[] { a.x, a.y, a.z, b.x, b.y, b.z }, c.x, c.y, c.z);
                    A64("Vector3d.Dot", new[] { a.x, a.y, a.z, b.x, b.y, b.z }, Vector3d.Dot(a, b));
                    A64("Vector3d.Angle", new[] { a.x, a.y, a.z, b.x, b.y, b.z }, Vector3d.Angle(a, b));
                    A64("UtilMath.AngleBetween", new[] { a.x, a.y, a.z, b.x, b.y, b.z }, UtilMath.AngleBetween(a, b));
                    var ex = Vector3d.Exclude(a, b);
                    A64("Vector3d.Exclude", new[] { a.x, a.y, a.z, b.x, b.y, b.z }, ex.x, ex.y, ex.z);
                    var pr = Vector3d.Project(a, b);
                    A64("Vector3d.Project", new[] { a.x, a.y, a.z, b.x, b.y, b.z }, pr.x, pr.y, pr.z);
                }
            }
            foreach (var ang in new[] { 0.0, 1e-8, 30.0, 90.0, 180.0, -45.0, 359.0, 720.0 })
                foreach (var axis in new[] { new Vector3d(0, 0, 1), new Vector3d(1, 1, 0), new Vector3d(0.3, -0.4, 0.5) })
                {
                    var q = QuaternionD.AngleAxis(ang, axis);
                    A64("QuaternionD.AngleAxis", new[] { ang, axis.x, axis.y, axis.z }, q.x, q.y, q.z, q.w);
                    var iq = QuaternionD.Inverse(q);
                    A64("QuaternionD.Inverse", new[] { q.x, q.y, q.z, q.w }, iq.x, iq.y, iq.z, iq.w);
                    var r = q * new Vector3d(1, 2, 3);
                    A64("QuaternionD*Vector3d(1,2,3)", new[] { q.x, q.y, q.z, q.w }, r.x, r.y, r.z);
                    var q2 = QuaternionD.AngleAxis(17.0, new Vector3d(0, 1, 0));
                    var m = q * q2;
                    A64("QuaternionD*AngleAxis(17,Y)", new[] { q.x, q.y, q.z, q.w }, m.x, m.y, m.z, m.w);
                    A64("QuaternionD.Angle(q,AngleAxis(17,Y))", new[] { q.x, q.y, q.z, q.w }, QuaternionD.Angle(q, q2));
                }
            foreach (var (f, u) in new[] { (new Vector3d(0, 0, 1), new Vector3d(0, 1, 0)), (new Vector3d(1, 2, 3), new Vector3d(0, 0, 1)), (new Vector3d(-0.5, 0.1, 0.2), new Vector3d(0.3, 0.9, 0)) })
            {
                var q = QuaternionD.LookRotation(f, u);
                A64("QuaternionD.LookRotation", new[] { f.x, f.y, f.z, u.x, u.y, u.z }, q.x, q.y, q.z, q.w);
            }
            foreach (var x in new[] { -10.0, -Math.PI, -1e-12, 0.0, 1.0, Math.PI, 2 * Math.PI, 7.0, 1e6 })
            {
                A64("UtilMath.ClampRadiansTwoPI", new[] { x }, UtilMath.ClampRadiansTwoPI(x));
                A64("UtilMath.ClampRadiansPI", new[] { x }, UtilMath.ClampRadiansPI(x));
                A64("UtilMath.ClampDegrees360", new[] { x * 57.0 }, UtilMath.ClampDegrees360(x * 57.0));
                A64("UtilMath.ClampDegrees180", new[] { x * 57.0 }, UtilMath.ClampDegrees180(x * 57.0));
                A64("UtilMath.WrapAround", new[] { x, -1.0, 3.0 }, UtilMath.WrapAround(x, -1.0, 3.0));
                A64("UtilMath.Lerp", new[] { 2.0, 7.0, x / 5 }, UtilMath.Lerp(2.0, 7.0, x / 5));
                A64("UtilMath.Clamp01", new[] { x }, UtilMath.Clamp01(x));
            }
            foreach (var (lan, inc, ape) in new[] { (0.0, 0.0, 0.0), (77.7, 28.5, 123.4), (300.0, 135.0, 200.0), (0.0, 180.0, 30.0), (52.0, 1.304, 0.0) })
            {
                var cf = default(Planetarium.CelestialFrame);
                Planetarium.CelestialFrame.OrbitalFrame(lan, inc, ape, ref cf);
                A64("CelestialFrame.OrbitalFrame.X", new[] { lan, inc, ape }, cf.X.x, cf.X.y, cf.X.z);
                A64("CelestialFrame.OrbitalFrame.Y", new[] { lan, inc, ape }, cf.Y.x, cf.Y.y, cf.Y.z);
                A64("CelestialFrame.OrbitalFrame.Z", new[] { lan, inc, ape }, cf.Z.x, cf.Z.y, cf.Z.z);
                Planetarium.CelestialFrame.PlanetaryFrame(lan, inc, ape, ref cf);
                A64("CelestialFrame.PlanetaryFrame.X", new[] { lan, inc, ape }, cf.X.x, cf.X.y, cf.X.z);
                A64("CelestialFrame.PlanetaryFrame.Y", new[] { lan, inc, ape }, cf.Y.x, cf.Y.y, cf.Y.z);
                A64("CelestialFrame.PlanetaryFrame.Z", new[] { lan, inc, ape }, cf.Z.x, cf.Z.y, cf.Z.z);
                var rq = cf.Rotation;
                A64("CelestialFrame.Rotation", new[] { lan, inc, ape }, rq.x, rq.y, rq.z, rq.w);
            }
            var z = Planetarium.Zup;
            A64("Planetarium.Zup(after Awake)", new double[0], z.X.x, z.X.y, z.X.z);
            A64("Planetarium.Zup(after Awake).Y", new double[0], z.Y.x, z.Y.y, z.Y.z);
            A64("Planetarium.Zup(after Awake).Z", new double[0], z.Z.x, z.Z.y, z.Z.z);
            yield return ("math_shim/math_shim.json", g);
        }
    }
}
