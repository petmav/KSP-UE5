using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Reflection;
using System.Runtime.Serialization;
using UnityEngine;

namespace KspGolden.Infra
{
    /// <summary>Helpers for hosting KSP MonoBehaviours outside the Unity runtime.</summary>
    public static class U
    {
        const BindingFlags All = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static;
        static readonly FieldInfo CachedPtr = typeof(UnityEngine.Object).GetField("m_CachedPtr", BindingFlags.NonPublic | BindingFlags.Instance);

        /// <summary>Unity treats a managed Object as destroyed ("fake null") when m_CachedPtr == 0
        /// (UnityEngine.Object.IsNativeObjectAlive). Give hosted objects a dummy non-zero pointer so that
        /// KSP's `obj != null` / `(bool)obj` checks behave as in game. The pointer is never dereferenced
        /// by any code path the harness drives (GetInstanceID would throw first).</summary>
        public static T Alive<T>(T o) where T : UnityEngine.Object { CachedPtr.SetValue(o, new IntPtr(1)); return o; }

        public static T New<T>() where T : UnityEngine.Object, new() => Alive(new T());

        public static T Uninit<T>() => (T)FormatterServices.GetUninitializedObject(typeof(T));

        public static FieldInfo Field(Type t, string name)
        {
            for (var x = t; x != null; x = x.BaseType)
            {
                var f = x.GetField(name, All | BindingFlags.DeclaredOnly);
                if (f != null) return f;
            }
            return null;
        }

        public static object Get(object o, string name)
        {
            var f = Field(o.GetType(), name) ?? throw new MissingFieldException(o.GetType().Name, name);
            return f.GetValue(o);
        }

        public static T Get<T>(object o, string name) => (T)Get(o, name);

        public static void Set(object o, string name, object v)
        {
            var f = Field(o.GetType(), name) ?? throw new MissingFieldException(o.GetType().Name, name);
            f.SetValue(o, v);
        }

        public static object GetStatic(Type t, string name)
        {
            var f = Field(t, name);
            if (f != null) return f.GetValue(null);
            var p = t.GetProperty(name, All) ?? throw new MissingMemberException(t.Name, name);
            return p.GetValue(null);
        }

        public static void SetStatic(Type t, string name, object v)
        {
            var f = Field(t, name) ?? throw new MissingFieldException(t.Name, name);
            f.SetValue(null, v);
        }

        public static MethodInfo Method(Type t, string name, params Type[] sig)
        {
            for (var x = t; x != null; x = x.BaseType)
            {
                var ms = x.GetMethods(All | BindingFlags.DeclaredOnly).Where(m => m.Name == name).ToList();
                if (sig != null && sig.Length > 0) ms = ms.Where(m => m.GetParameters().Select(p => p.ParameterType).SequenceEqual(sig)).ToList();
                if (ms.Count == 1) return ms[0];
                if (ms.Count > 1) throw new AmbiguousMatchException(t.Name + "." + name);
            }
            throw new MissingMethodException(t.Name, name);
        }

        public static object Call(object o, string name, params object[] args)
        {
            var m = Method(o.GetType(), name, args.Select(a => a?.GetType()).ToArray());
            return Invoke(m, o, args);
        }

        public static object CallSig(object o, Type[] sig, string name, params object[] args) => Invoke(Method(o.GetType(), name, sig), o, args);

        public static object Invoke(MethodInfo m, object o, object[] args)
        {
            try { return m.Invoke(o, args); }
            catch (TargetInvocationException e) { System.Runtime.ExceptionServices.ExceptionDispatchInfo.Capture(e.InnerException).Throw(); throw; }
        }
    }

    /// <summary>Captures UnityEngine.Debug output (the native DebugLogHandler cannot run outside Unity).</summary>
    public sealed class CaptureLogHandler : ILogHandler
    {
        public readonly List<string> Lines = new List<string>();
        public bool Echo;
        public void LogFormat(LogType logType, UnityEngine.Object context, string format, params object[] args)
        {
            string msg = args == null || args.Length == 0 ? format : string.Format(CultureInfo.InvariantCulture, format, args);
            Lines.Add(logType + ": " + msg);
            if (Echo) Console.Error.WriteLine("[unity " + logType + "] " + msg);
        }
        public void LogException(Exception exception, UnityEngine.Object context)
        {
            Lines.Add("Exception: " + exception);
            if (Echo) Console.Error.WriteLine("[unity Exception] " + exception.Message);
        }
    }

    /// <summary>
    /// Applies values extracted from the shipped Unity assets (tools/extract_assets.py output) onto freshly
    /// constructed KSP objects, emulating Unity deserialization: ctor runs first (field initializers), then
    /// every serialized field is overwritten. Object references (PPtrs) are skipped and wired by the caller.
    /// </summary>
    public static class SerializedOverlay
    {
        static readonly HashSet<string> Skip = new HashSet<string> { "m_GameObject", "m_Enabled", "m_Script", "m_Name", "_gameObject", "_pathID" };

        public sealed class Report
        {
            public readonly List<string> Applied = new List<string>();
            public readonly List<string> Unmapped = new List<string>();
            public readonly List<string> SkippedRefs = new List<string>();
        }

        public static Report Apply(object target, JObj data, Report rep = null, string path = "")
        {
            rep = rep ?? new Report();
            foreach (var kv in data.Items)
            {
                if (Skip.Contains(kv.Key)) continue;
                var f = U.Field(target.GetType(), kv.Key);
                string p = path + kv.Key;
                if (f == null || f.IsStatic || f.IsLiteral) { rep.Unmapped.Add(p); continue; }
                if (typeof(UnityEngine.Object).IsAssignableFrom(f.FieldType) || IsPPtr(kv.Value))
                {
                    rep.SkippedRefs.Add(p); continue;
                }
                object cur = f.GetValue(target);
                object v = Convert(f.FieldType, kv.Value, cur, rep, p + ".");
                if (v is SkipMarker) { rep.SkippedRefs.Add(p); continue; }
                f.SetValue(target, v);
                rep.Applied.Add(p);
            }
            return rep;
        }

        sealed class SkipMarker { }
        static readonly SkipMarker SkipValue = new SkipMarker();

        static bool IsPPtr(object v) => v is JObj o && o.Has("m_FileID") && o.Has("m_PathID") && o.Items.Count == 2;

        public static float F(object v) => (float)Num(v);
        public static double Num(object v)
        {
            switch (v)
            {
                case JNum n: return n.Value;
                case string s: return ExactNumber.ParseDouble(s);
                case bool b: return b ? 1 : 0;
                default: throw new InvalidCastException("not a number: " + v);
            }
        }

        static object Convert(Type t, object v, object current, Report rep, string path)
        {
            if (t == typeof(double)) return Num(v);
            if (t == typeof(float))
            {
                double d = Num(v); float f = (float)d;
                if (!double.IsNaN(d) && (double)f != d) throw new InvalidOperationException($"{path}: serialized float {d:R} is not a float32 value");
                return f;
            }
            if (t == typeof(int)) return checked((int)Num(v));
            if (t == typeof(uint)) return checked((uint)Num(v));
            if (t == typeof(long)) return checked((long)Num(v));
            if (t == typeof(short)) return checked((short)Num(v));
            if (t == typeof(byte)) return checked((byte)Num(v));
            if (t == typeof(bool)) return Num(v) != 0;
            if (t == typeof(string)) return (string)v;
            if (t.IsEnum) return Enum.ToObject(t, checked((long)Num(v)));
            if (t == typeof(Vector3d)) { var o = (JObj)v; return new Vector3d(Num(o["x"]), Num(o["y"]), Num(o["z"])); }
            if (t == typeof(Vector3)) { var o = (JObj)v; return new Vector3(F(o["x"]), F(o["y"]), F(o["z"])); }
            if (t == typeof(Vector2)) { var o = (JObj)v; return new Vector2(F(o["x"]), F(o["y"])); }
            if (t == typeof(Color)) { var o = (JObj)v; return new Color(F(o["r"]), F(o["g"]), F(o["b"]), F(o["a"])); }
            if (t == typeof(Quaternion)) { var o = (JObj)v; return new Quaternion(F(o["x"]), F(o["y"]), F(o["z"]), F(o["w"])); }
            if (t == typeof(QuaternionD)) { var o = (JObj)v; return new QuaternionD(Num(o["x"]), Num(o["y"]), Num(o["z"]), Num(o["w"])); }
            if (t == typeof(FloatCurve)) return MakeFloatCurve((JObj)v);
            if (t == typeof(KspUnityShim.AnimationCurve)) return MakeCurve((JObj)v);
            if (t.IsArray)
            {
                var a = (List<object>)v; var et = t.GetElementType();
                if (typeof(UnityEngine.Object).IsAssignableFrom(et)) return SkipValue;
                var arr = Array.CreateInstance(et, a.Count);
                for (int i = 0; i < a.Count; i++)
                {
                    var ev = Convert(et, a[i], null, rep, path + "[" + i + "].");
                    if (ev is SkipMarker) return SkipValue;
                    arr.SetValue(ev, i);
                }
                return arr;
            }
            if (t.IsGenericType && t.GetGenericTypeDefinition() == typeof(List<>))
            {
                var a = (List<object>)v; var et = t.GetGenericArguments()[0];
                if (typeof(UnityEngine.Object).IsAssignableFrom(et)) return SkipValue;
                var list = (IList)Activator.CreateInstance(t);
                for (int i = 0; i < a.Count; i++)
                {
                    var ev = Convert(et, a[i], null, rep, path + "[" + i + "].");
                    if (ev is SkipMarker) return SkipValue;
                    list.Add(ev);
                }
                return list;
            }
            if (v is JObj obj && (t.IsClass || (t.IsValueType && !t.IsPrimitive)))
            {
                object inst = current ?? (t.IsValueType ? Activator.CreateInstance(t) : CreateSerializable(t));
                Apply(inst, obj, rep, path);
                return inst;
            }
            throw new NotSupportedException($"{path}: cannot convert {v?.GetType().Name} to {t.Name}");
        }

        static object CreateSerializable(Type t)
        {
            var ctor = t.GetConstructor(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, null, Type.EmptyTypes, null);
            return ctor != null ? ctor.Invoke(null) : FormatterServices.GetUninitializedObject(t);
        }

        public static Keyframe[] Keys(JObj fCurve)
        {
            var list = fCurve.Arr("m_Curve");
            var keys = new Keyframe[list.Count];
            for (int i = 0; i < keys.Length; i++)
            {
                var k = (JObj)list[i];
                var kf = new Keyframe(F(k["time"]), F(k["value"]), F(k["inSlope"]), F(k["outSlope"]));
                kf.weightedMode = (WeightedMode)(int)Num(k["weightedMode"]);
                kf.inWeight = F(k["inWeight"]);
                kf.outWeight = F(k["outWeight"]);
                keys[i] = kf;
            }
            return keys;
        }

        public static KspUnityShim.AnimationCurve MakeCurve(JObj fCurve)
        {
            return KspUnityShim.AnimationCurve.CreateFromSerialized(Keys(fCurve), (int)Num(fCurve["m_PreInfinity"]), (int)Num(fCurve["m_PostInfinity"]));
        }

        /// <summary>FloatCurve as Unity deserializes it: ctor, then fCurve/_minTime/_maxTime overwritten.</summary>
        public static FloatCurve MakeFloatCurve(JObj fc)
        {
            var c = new FloatCurve();
            U.Set(c, "fCurve", MakeCurve(fc.Obj("fCurve")));
            U.Set(c, "_minTime", F(fc["_minTime"]));
            U.Set(c, "_maxTime", F(fc["_maxTime"]));
            return c;
        }
    }
}
