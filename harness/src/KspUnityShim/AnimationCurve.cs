// Managed port of Unity 2019.4.18f1 native AnimationCurveTpl<float> (UnityPlayer.dll, Win64 mono nondev,
// PDB GUID 41E8D80F828B4CEC840B0E4C0496BCCF age 1), transcribed instruction-by-instruction from the
// disassembly of the functions listed below. Every float operation is single precision (SSE scalar),
// in the same order as the native code, so results are intended to be bit-identical.
//
//   AnimationCurve_CUSTOM_Evaluate            -> AnimationCurveTpl<float>::Evaluate(t, NULL)      RVA 0x75A6A0
//   AnimationCurveTpl<float>::CalculateCacheData                                                   RVA 0x759230
//   AnimationCurveTpl<float>::FindIndexForSampling                                                 RVA 0x75C770
//   AnimationCurve_CUSTOM_AddKey(t,v)         -> AddKeySmoothTangents                              RVA 0x750780
//   RecalculateSplineSlopeT<float>                                                                 RVA 0x74FDB0
//   AnimationCurve_CUSTOM_AddKey_Internal     -> AnimationCurveTpl<float>::AddKey(Keyframe)        RVA 0x758840
//   AnimationCurveBindings::Internal_Create / SetKeys (copy, drop non-finite, std::sort)           RVA 0x75D220 / 0x75E210
//   AnimationCurveBindings::RemoveKey / SmoothTangents                                             RVA 0x75E040 / 0x75E500
//   AnimationCurveTpl<float>::SetPreInfinity / SetPostInfinity                                     RVA 0x75E430 / 0x75E370
//
// Not ported (throw NotSupportedException so they can never silently diverge): weighted-tangent
// (Bezier) segments, Loop/PingPong wrap modes evaluated outside the key range, MoveKey.
// KSP's FloatCurve never produces weighted keys and always uses ClampForever.
using System;
using System.Collections.Generic;
using UnityEngine;

namespace KspUnityShim
{
    public class AnimationCurve : IEquatable<AnimationCurve>
    {
        // Native Cache struct: {int index; float time; float timeEnd; float coeff[4]} (offsets 0..0x18).
        private int cIndex;
        private float cTime = float.PositiveInfinity;
        private float cTimeEnd;
        private float cA, cB, cC, cD;

        private readonly List<Keyframe> m_Keys = new List<Keyframe>();

        // Native m_PreInfinity (+0x58) / m_PostInfinity (+0x5C). 2 = clamp (ctor default), 1 = repeat, 0 = ping-pong, 3 = Default.
        private int m_PreInfinity = 2;
        private int m_PostInfinity = 2;
        private WrapMode m_PreWrapManaged = WrapMode.ClampForever;
        private WrapMode m_PostWrapManaged = WrapMode.ClampForever;

        /// <summary>Count of Evaluate calls served by each native branch, for diagnostics.</summary>
        public static long EvalCount;

        public AnimationCurve()
        {
            Create(null);
        }

        public AnimationCurve(params Keyframe[] keys)
        {
            Create(keys);
        }

        private const float kDefaultWeight = 0.333333343f; // 0x3EAAAAAB

        // AnimationCurveBindings::Internal_Create / SetKeys: copy keys, erase keys whose value or time is
        // not finite, then std::sort by time (std::less<KeyframeTpl> compares time). No de-duplication.
        private void Create(Keyframe[] keys)
        {
            m_Keys.Clear();
            if (keys != null)
            {
                for (int i = 0; i < keys.Length; i++)
                {
                    Keyframe k = keys[i];
                    if (!IsFinite(k.value) || !IsFinite(k.time)) continue;
                    m_Keys.Add(k);
                }
                StableSortByTime(m_Keys); // std::sort is unstable; identical only for unique times (documented)
            }
            InvalidateCache();
        }

        /// <summary>
        /// Equivalent of Unity deserialization (StreamedBinaryRead Transfer of AnimationCurveTpl, RVA 0x17FE80):
        /// keys are taken verbatim (no sort, no filtering), m_PreInfinity/m_PostInfinity are the raw native
        /// enum values from the asset, then InvalidateCache.
        /// </summary>
        public static AnimationCurve CreateFromSerialized(Keyframe[] keys, int preInfinity, int postInfinity)
        {
            var c = new AnimationCurve();
            c.m_Keys.Clear();
            if (keys != null) c.m_Keys.AddRange(keys);
            c.m_PreInfinity = preInfinity;
            c.m_PostInfinity = postInfinity;
            c.m_PreWrapManaged = preInfinity == 2 ? WrapMode.ClampForever : (WrapMode)(-1);
            c.m_PostWrapManaged = postInfinity == 2 ? WrapMode.ClampForever : (WrapMode)(-1);
            c.InvalidateCache();
            return c;
        }

        /// <summary>Native wrap-mode values (+0x58/+0x5C), for dumps.</summary>
        public int NativePreInfinity => m_PreInfinity;
        public int NativePostInfinity => m_PostInfinity;

        private static void StableSortByTime(List<Keyframe> list)
        {
            var arr = list.ToArray();
            var idx = new int[arr.Length];
            for (int i = 0; i < idx.Length; i++) idx[i] = i;
            Array.Sort(idx, (x, y) => { int c = arr[x].time.CompareTo(arr[y].time); return c != 0 ? c : x.CompareTo(y); });
            list.Clear();
            foreach (var i in idx) list.Add(arr[i]);
        }

        private static bool IsFinite(float f)
        {
            return (BitConverter.ToInt32(BitConverter.GetBytes(f), 0) & 0x7F800000) != 0x7F800000;
        }

        // AnimationCurveTpl<float>::InvalidateCache (RVA 0x75DCF0): index=0, time=+inf (timeEnd untouched).
        private void InvalidateCache()
        {
            cIndex = 0;
            cTime = float.PositiveInfinity;
        }

        public Keyframe[] keys
        {
            get { return m_Keys.ToArray(); }
            set { Create(value); }
        }

        public Keyframe this[int index] => m_Keys[index];

        public int length => m_Keys.Count;

        public WrapMode preWrapMode
        {
            get { return m_PreWrapManaged; }
            set { m_PreWrapManaged = value; m_PreInfinity = MapWrap((int)value); InvalidateCache(); }
        }

        public WrapMode postWrapMode
        {
            get { return m_PostWrapManaged; }
            set { m_PostWrapManaged = value; m_PostInfinity = MapWrap((int)value); InvalidateCache(); }
        }

        // SetPostInfinity (RVA 0x75E370): Loop(2)->1, PingPong(4)->0, Default(0)->3, anything else->2 (clamp).
        private static int MapWrap(int mode)
        {
            if (mode == 2) return 1;
            if (mode == 4) return 0;
            return mode != 0 ? 2 : 3;
        }

        // ---------------------------------------------------------------- Evaluate (RVA 0x75A6A0)
        public float Evaluate(float time)
        {
            EvalCount++;
            float curveT = time;
            int n = m_Keys.Count;
            if (n == 1) return m_Keys[0].value;

            // cache hit: curveT >= cache.time && cache.timeEnd > curveT
            if (!(curveT < cTime) && !float.IsNaN(curveT) && !float.IsNaN(cTime))
            {
                if (cTimeEnd > curveT)
                {
                    return Poly(curveT - cTime);
                }
            }

            if (!IsValid()) return 0f;

            float last = m_Keys[n - 1].time;
            float first = m_Keys[0].time;
            if (curveT >= last) // comiss curveT, last; jb -> not taken when curveT >= last
            {
                if (m_PostInfinity == 2)
                {
                    cTime = last;
                    cTimeEnd = float.PositiveInfinity;
                    cA = 0f; cB = 0f; cC = 0f;
                    cD = m_Keys[n - 1].value;
                    return Poly(curveT - cTime);
                }
                throw new NotSupportedException("AnimationCurve shim: non-clamp post-wrap mode not ported");
            }
            if (first > curveT)
            {
                if (m_PreInfinity == 2)
                {
                    cTime = curveT - 1000f;
                    cTimeEnd = first;
                    cA = 0f; cB = 0f; cC = 0f;
                    cD = m_Keys[0].value;
                    return Poly(curveT - cTime);
                }
                throw new NotSupportedException("AnimationCurve shim: non-clamp pre-wrap mode not ported");
            }
            FindIndexForSampling(curveT, out int lhs, out int rhs);
            if ((m_Keys[lhs].weightedMode & (WeightedMode)2) != 0 || (m_Keys[rhs].weightedMode & (WeightedMode)1) != 0)
                throw new NotSupportedException("AnimationCurve shim: weighted (Bezier) segment not ported");
            CalculateCacheData(lhs, rhs, 0f);
            return Poly(curveT - cTime);
        }

        private float Poly(float t)
        {
            float r = t * cA;
            r = r + cB;
            r = r * t;
            r = r + cC;
            r = r * t;
            r = r + cD;
            return r;
        }

        // AnimationCurveTpl<float>::IsValid (RVA 0xD6A60): >=1 key and finite range.
        private bool IsValid()
        {
            if (m_Keys.Count < 1) return false;
            float a = m_Keys[0].time, b = m_Keys[m_Keys.Count - 1].time;
            return IsFinite(a) && IsFinite(b);
        }

        // CalculateCacheData (RVA 0x759230), timeOffset argument.
        private void CalculateCacheData(int lhsIndex, int rhsIndex, float timeOffset)
        {
            Keyframe L = m_Keys[lhsIndex];
            Keyframe R = m_Keys[rhsIndex];
            cIndex = lhsIndex;
            cTime = timeOffset + L.time;
            cTimeEnd = timeOffset + R.time;
            float dx = R.time - L.time;
            float dy = R.value - L.value;
            float length = (1e-4f > dx) ? 1e-4f : dx; // maxss(1e-4, dx)
            float m1 = L.outTangent;
            float d1 = length * R.inTangent;
            float d0 = length * m1;
            float length2 = length * length;
            float inv = 1f / length2;
            cC = m1;
            float a = d1 + d0;
            a = a - dy;
            a = a - dy;
            a = a * inv;
            a = a / length;
            cA = a;
            float b = dy + dy;
            b = b + dy;
            b = b - d0;
            b = b - d0;
            b = b - d1;
            b = b * inv;
            cB = b;
            cD = L.value;
            if (L.outTangent == float.PositiveInfinity || R.inTangent == float.PositiveInfinity)
            {
                cA = 0f; cB = 0f; cC = 0f;
                cD = L.value;
            }
        }

        // FindIndexForSampling (RVA 0x75C770): cache-hint scan (3 steps each way) then upper_bound.
        private void FindIndexForSampling(float t, out int lhs, out int rhs)
        {
            int count = m_Keys.Count;
            int r8 = cIndex;
            if (r8 != -1)
            {
                if (t > m_Keys[r8].time)
                {
                    for (int step = 0; step < 3; step++)
                    {
                        int next = r8 + 1;
                        if (next < count && m_Keys[next].time > t)
                        {
                            lhs = r8;
                            rhs = Math.Min(r8 + 1, count - 1);
                            return;
                        }
                        r8 = next;
                    }
                }
                else
                {
                    int rdx = r8;
                    for (int step = 0; step < 3; step++)
                    {
                        if (rdx >= 0 && t >= m_Keys[rdx].time)
                        {
                            lhs = r8;
                            rhs = Math.Min(r8 + 1, count - 1);
                            return;
                        }
                        r8--; rdx--;
                    }
                }
            }
            int first = 0;
            int len = count;
            while (len > 0)
            {
                int half = len >> 1;
                int mid = first + half;
                if (m_Keys[mid].time > t) len = half;
                else { first = mid + 1; len = len - half - 1; }
            }
            lhs = first - 1;
            rhs = Math.Min(first, count - 1);
        }

        // ---------------------------------------------------------------- key editing
        // AnimationCurveTpl<float>::AddKey (RVA 0x758840): lower_bound on time; if a key with equal time
        // exists the new key is rejected (returns -1). Cache invalidated first.
        public int AddKey(Keyframe key)
        {
            InvalidateCache();
            int first = 0, len = m_Keys.Count;
            while (len > 0)
            {
                int half = len >> 1;
                if (key.time > m_Keys[first + half].time) { first = first + half + 1; len = len - half - 1; }
                else len = half;
            }
            if (first != m_Keys.Count && !(m_Keys[first].time > key.time)) return -1;
            m_Keys.Insert(first, key);
            return first;
        }

        // AddKeySmoothTangents (RVA 0x750780): insert {time,value,in=0,out=0,weightedMode=0,weights=1/3},
        // then RecalculateSplineSlope(index-1), (index), (index+1) with bias 0.
        public int AddKey(float time, float value)
        {
            var k = new Keyframe(time, value, 0f, 0f);
            k.weightedMode = WeightedMode.None;
            k.inWeight = kDefaultWeight;
            k.outWeight = kDefaultWeight;
            int index = AddKey(k);
            if (index == -1) return -1;
            if (index > 0) RecalculateSplineSlope(index - 1, 0f);
            RecalculateSplineSlope(index, 0f);
            if (index + 1 < m_Keys.Count) RecalculateSplineSlope(index + 1, 0f);
            return index;
        }

        // RecalculateSplineSlopeT<float> (RVA 0x74FDB0)
        private void RecalculateSplineSlope(int key, float b)
        {
            int count = m_Keys.Count;
            if (count < 2) return;
            Keyframe k = m_Keys[key];
            if (key == 0)
            {
                float dy = m_Keys[1].value - m_Keys[0].value;
                float dx = m_Keys[1].time - m_Keys[0].time;
                float m = dy / dx;
                k.inTangent = m; k.outTangent = m;
                k.outWeight = kDefaultWeight;
                m_Keys[key] = k;
                InvalidateCache();
                return;
            }
            if (key == count - 1)
            {
                float dy = k.value - m_Keys[key - 1].value;
                float dx = k.time - m_Keys[key - 1].time;
                float m = dy / dx;
                k.inTangent = m; k.outTangent = m;
                k.inWeight = kDefaultWeight;
                m_Keys[key] = k;
                InvalidateCache();
                return;
            }
            float t0 = k.time;
            float v0 = k.value;
            float dx1 = t0 - m_Keys[key - 1].time;
            float dx2 = m_Keys[key + 1].time - t0;
            float dy2 = m_Keys[key + 1].value - v0;
            float m1;
            if (Math.Abs(dx1) > 1e-5f) { float dy1 = v0 - m_Keys[key - 1].value; m1 = dy1 / dx1; } else m1 = 0f;
            float m2;
            if (Math.Abs(dx2) > 1e-5f) m2 = dy2 / dx2; else m2 = 0f;
            float wR = 1f - b;
            float wL = b + 1f;
            wR = wR * 0.5f;
            wL = wL * 0.5f;
            wR = wR * m2;
            wL = wL * m1;
            float slope = wR + wL;
            k.inTangent = slope; k.outTangent = slope;
            k.inWeight = kDefaultWeight;
            k.outWeight = kDefaultWeight;
            m_Keys[key] = k;
            InvalidateCache();
        }

        public void SmoothTangents(int index, float weight)
        {
            if (index < 0 || index >= m_Keys.Count) throw new ArgumentOutOfRangeException(nameof(index));
            RecalculateSplineSlope(index, weight);
        }

        public void RemoveKey(int index)
        {
            if (index < 0 || index >= m_Keys.Count) throw new ArgumentOutOfRangeException(nameof(index));
            InvalidateCache();
            m_Keys.RemoveAt(index);
        }

        public int MoveKey(int index, Keyframe key)
        {
            throw new NotSupportedException("AnimationCurve shim: MoveKey not ported");
        }

        public static AnimationCurve Constant(float timeStart, float timeEnd, float value)
        {
            return Linear(timeStart, value, timeEnd, value);
        }

        public static AnimationCurve Linear(float timeStart, float valueStart, float timeEnd, float valueEnd)
        {
            float num = (valueEnd - valueStart) / (timeEnd - timeStart);
            return new AnimationCurve(new Keyframe(timeStart, valueStart, 0f, num), new Keyframe(timeEnd, valueEnd, num, 0f));
        }

        public static AnimationCurve EaseInOut(float timeStart, float valueStart, float timeEnd, float valueEnd)
        {
            return new AnimationCurve(new Keyframe(timeStart, valueStart, 0f, 0f), new Keyframe(timeEnd, valueEnd, 0f, 0f));
        }

        public override bool Equals(object o) => o is AnimationCurve c && Equals(c);
        public bool Equals(AnimationCurve other) => ReferenceEquals(this, other);
        public override int GetHashCode() => base.GetHashCode();
    }
}
