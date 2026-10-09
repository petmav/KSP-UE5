// Profiling branch only. CAPTURE-SPEC C7 kernel: System.Math (libm), float32 evaluation mode, numeric conversions,
// number formatting/parsing and System.Random, evaluated by whatever runtime runs this code.
// Unity-free: the identical file is compiled into the .NET Framework 4.8 x64 reference tool (harness/captures/tools/netfx)
// and into Assembly-CSharp (KSP's Mono). Both write the same tables; compare.py diffs them.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Runtime.CompilerServices;

public static class HcC7
{
	public const int LibmRandomCount = 1000;

	public const int F32Count = 10000;

	// ---------------- input generation (spec: System.Random(7) for libm, System.Random(11) for float32 probes) -------------

	public static List<double> GenLibmRandom()
	{
		System.Random r = new System.Random(7);
		List<double> list = new List<double>();
		for (int i = 0; i < LibmRandomCount; i++)
		{
			list.Add(r.NextDouble() * 200.0 - 100.0);
		}
		return list;
	}

	// a,b,c,d in [-100,100), e in [-1e4,1e4) (same magnitude as the products, so the sums cancel), x in [-10,10).
	public static float[][] GenF32Inputs()
	{
		System.Random r = new System.Random(11);
		float[][] rows = new float[F32Count][];
		for (int i = 0; i < F32Count; i++)
		{
			float a = (float)(r.NextDouble() * 200.0 - 100.0);
			float b = (float)(r.NextDouble() * 200.0 - 100.0);
			float c = (float)(r.NextDouble() * 200.0 - 100.0);
			float d = (float)(r.NextDouble() * 200.0 - 100.0);
			float e = (float)(r.NextDouble() * 20000.0 - 10000.0);
			float x = (float)(r.NextDouble() * 20.0 - 10.0);
			rows[i] = new float[6] { a, b, c, d, e, x };
		}
		return rows;
	}

	public static List<double> Specials()
	{
		return new List<double>
		{
			0.0, -0.0, 1.0, -1.0, 0.5, -0.5, 0.7, 2.0, Math.PI, Math.PI / 2.0, Math.PI / 4.0, 2.0 * Math.PI, -Math.PI, 1e-300, 4.9406564584124654E-324,
			1e-8, 1e-20, 100.0, 709.0, 709.78, 710.0, -745.0, -746.0, 1e10, 1e15, 1e16, 1e22, 1e300, 3.4028234663852886E+38,
			double.MaxValue, double.Epsilon, double.NaN, double.PositiveInfinity, double.NegativeInfinity, 0.9999999999999999, 1.0000000000000002,
			13599840256.0, 3531599999999.9995, 21549.425183089825, 0.0033513700562331824
		};
	}

	// ---------------- probes (each a separate method so the JIT compiles them as written) ----------------

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbePoly(float[][] inp, float[] o)
	{
		for (int i = 0; i < inp.Length; i++)
		{
			float x = inp[i][5];
			float r = ((x * 1.1f + 0.3f) * x - 0.7f) / 3.3f;
			o[i] = r;
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbeDot2(float[][] inp, float[] o)
	{
		for (int i = 0; i < inp.Length; i++)
		{
			float a = inp[i][0];
			float b = inp[i][1];
			float c = inp[i][2];
			float d = inp[i][3];
			float e = inp[i][4];
			float r = a * b + c * d - e;
			o[i] = r;
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbeMulWide(float[][] inp, double[] o)
	{
		for (int i = 0; i < inp.Length; i++)
		{
			float a = inp[i][0];
			float b = inp[i][1];
			double r = a * b;
			o[i] = r;
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbeMulAddWide(float[][] inp, double[] o)
	{
		for (int i = 0; i < inp.Length; i++)
		{
			float a = inp[i][0];
			float b = inp[i][1];
			float c = inp[i][2];
			double r = a * b + c;
			o[i] = r;
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbeCastEach(float[][] inp, float[] o)
	{
		for (int i = 0; i < inp.Length; i++)
		{
			float a = inp[i][0];
			float b = inp[i][1];
			float c = inp[i][2];
			float d = inp[i][3];
			float e = inp[i][4];
			float r = (float)((float)((float)(a * b) + (float)(c * d)) - e);
			o[i] = r;
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbeAccLoop(float[][] inp, float[] o)
	{
		for (int i = 0; i < inp.Length; i++)
		{
			float a = inp[i][0];
			float b = inp[i][1];
			float acc = 0f;
			for (int k = 0; k < 16; k++)
			{
				acc = acc * 0.999f + a * b;
			}
			o[i] = acc;
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static float Dot2(float a, float b, float c, float d, float e)
	{
		return a * b + c * d - e;
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbeDot2Call(float[][] inp, float[] o)
	{
		for (int i = 0; i < inp.Length; i++)
		{
			o[i] = Dot2(inp[i][0], inp[i][1], inp[i][2], inp[i][3], inp[i][4]);
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbeLocals(float[][] inp, float[] o)
	{
		for (int i = 0; i < inp.Length; i++)
		{
			float t1 = inp[i][0] * inp[i][1];
			float t2 = inp[i][2] * inp[i][3];
			float r = t1 + t2 - inp[i][4];
			o[i] = r;
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbeSqrtDiv(float[][] inp, float[] o)
	{
		for (int i = 0; i < inp.Length; i++)
		{
			float a = inp[i][0];
			float b = inp[i][1];
			float c = inp[i][2];
			o[i] = (float)Math.Sqrt(a * a + b * b) / c;
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbeCompare(float[][] inp, float[] o)
	{
		for (int i = 0; i < inp.Length; i++)
		{
			float a = inp[i][0];
			float b = inp[i][1];
			float c = inp[i][2];
			float p = a * b;
			// true iff a*b+c is evaluated with the product rounded to float32 (or the comparison widens both sides identically)
			o[i] = (a * b + c == p + c) ? 1f : 0f;
		}
	}

	// ---- rounding points: call arguments, return values, field/static/struct/local stores
	[MethodImpl(MethodImplOptions.NoInlining)]
	static float AddNI(float p, float q)
	{
		return p + q;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	static float AddAI(float p, float q)
	{
		return p + q;
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static float MulNI(float p, float q)
	{
		return p * q;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	static float MulAI(float p, float q)
	{
		return p * q;
	}

	sealed class Box
	{
		public float f;
	}

	struct SBox
	{
		public float f;
	}

	static float sField;

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbeArgs(float[][] inp, float[] oNI, float[] oAI)
	{
		for (int i = 0; i < inp.Length; i++)
		{
			float a = inp[i][0];
			float b = inp[i][1];
			float c = inp[i][2];
			oNI[i] = AddNI(a * b, c);
			oAI[i] = AddAI(a * b, c);
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	static void ProbeStores(float[][] inp, double[] oRetNI, double[] oRetAI, double[] oField, double[] oStruct, double[] oStatic, double[] oLocal)
	{
		Box box = new Box();
		for (int i = 0; i < inp.Length; i++)
		{
			float a = inp[i][0];
			float b = inp[i][1];
			oRetNI[i] = MulNI(a, b);
			oRetAI[i] = MulAI(a, b);
			box.f = a * b;
			oField[i] = box.f;
			SBox sb = default(SBox);
			sb.f = a * b;
			oStruct[i] = sb.f;
			sField = a * b;
			oStatic[i] = sField;
			float t = a * b;
			oLocal[i] = t;
		}
	}

	// ---------------- main builder ----------------

	public static HcJson.GFile Build(string dataset, string inDir, Action<string> log)
	{
		HcJson.GFile g = new HcJson.GFile(dataset, "capture", "CAPTURE-SPEC C7: System.Math transcendental results, float32 expression evaluation mode, numeric conversions, number formatting/parsing and System.Random, as evaluated by the runtime named in provenance.runtime. The same source file (HarnessCaptureC7.cs) runs in KSP's Mono (Assembly-CSharp, profiling branch) and in the .NET Framework 4.8 x64 reference tool, so any difference is the runtime's.");
		g.Oracles.Add("System.Math.* (Sin Cos Tan Asin Acos Atan Atan2 Sinh Cosh Tanh Exp Log Log10 Sqrt Pow Floor Ceiling Round IEEERemainder), C# % on double/float");
		g.Oracles.Add("C# float32 expressions compiled by Roslyn, executed by the runtime's JIT");
		g.Oracles.Add("C# numeric conversions (unchecked), double/float ToString/Parse (InvariantCulture), System.Random");

		// ---- inputs
		List<double> xs = new List<double>();
		List<string> xsrc = new List<string>();
		foreach (string[] f in HcJson.ReadTsv(Path.Combine(inDir, "libm_inputs.tsv")))
		{
			xs.Add(HcJson.FromHex64(f[0]));
			xsrc.Add((f.Length > 1) ? f[1] : "");
		}
		List<float[]> fin = new List<float[]>();
		foreach (string[] f in HcJson.ReadTsv(Path.Combine(inDir, "f32_inputs.tsv")))
		{
			float[] row = new float[6];
			for (int k = 0; k < 6; k++)
			{
				row[k] = HcJson.FromHex32(f[k]);
			}
			fin.Add(row);
		}
		float[][] F = fin.ToArray();
		// regenerate the System.Random-derived inputs with this runtime and check they equal the file (= .NET's)
		List<double> rr = GenLibmRandom();
		int libmRandMismatch = 0;
		int libmRandFound = 0;
		for (int i = 0; i < xs.Count; i++)
		{
			if (xsrc[i].StartsWith("System.Random(7)#", StringComparison.Ordinal))
			{
				int k = int.Parse(xsrc[i].Substring("System.Random(7)#".Length), CultureInfo.InvariantCulture);
				libmRandFound++;
				if (BitConverter.DoubleToInt64Bits(rr[k]) != BitConverter.DoubleToInt64Bits(xs[i]))
				{
					libmRandMismatch++;
				}
			}
		}
		float[][] fr = GenF32Inputs();
		int f32RandMismatch = 0;
		for (int i = 0; i < Math.Min(fr.Length, F.Length); i++)
		{
			for (int k = 0; k < 6; k++)
			{
				if (BitConverter.ToInt32(BitConverter.GetBytes(fr[i][k]), 0) != BitConverter.ToInt32(BitConverter.GetBytes(F[i][k]), 0))
				{
					f32RandMismatch++;
				}
			}
		}
		g.P("libm_inputs", xs.Count).P("libm_inputs_from_system_random7", libmRandFound).P("system_random7_regenerated_mismatches", libmRandMismatch);
		g.P("f32_inputs", F.Length).P("system_random11_regenerated_mismatches", f32RandMismatch);
		log("C7 inputs: libm " + xs.Count + " (random7 mismatches " + libmRandMismatch + "), f32 " + F.Length + " (random11 mismatches " + f32RandMismatch + ")");

		// ---- libm f64
		HcJson.Table t64 = g.AddTable(new HcJson.Table("f64", "System.Math on double inputs. i indexes libm_inputs.tsv (x); y is the second argument where there is one."));
		t64.C("fn", HcJson.CT.Str).C("i", HcJson.CT.I64).C("x", HcJson.CT.F64).C("y", HcJson.CT.F64).C("r", HcJson.CT.F64).C("src", HcJson.CT.Str).Keys("fn", "i");
		double third = 1.0 / 3.0;
		double twoPi = Math.PI * 2.0;
		for (int i = 0; i < xs.Count; i++)
		{
			double x = xs[i];
			string s = xsrc[i];
			t64.Add("Math.Sin", i, x, null, Math.Sin(x), s);
			t64.Add("Math.Cos", i, x, null, Math.Cos(x), s);
			t64.Add("Math.Tan", i, x, null, Math.Tan(x), s);
			t64.Add("Math.Asin", i, x, null, Math.Asin(x), s);
			t64.Add("Math.Acos", i, x, null, Math.Acos(x), s);
			t64.Add("Math.Atan", i, x, null, Math.Atan(x), s);
			t64.Add("Math.Atan2(x,0.7)", i, x, 0.7, Math.Atan2(x, 0.7), s);
			t64.Add("Math.Atan2(0.7,x)", i, x, 0.7, Math.Atan2(0.7, x), s);
			t64.Add("Math.Sinh", i, x, null, Math.Sinh(x), s);
			t64.Add("Math.Cosh", i, x, null, Math.Cosh(x), s);
			t64.Add("Math.Tanh", i, x, null, Math.Tanh(x), s);
			t64.Add("Math.Exp", i, x, null, Math.Exp(x), s);
			t64.Add("Math.Log", i, x, null, Math.Log(x), s);
			t64.Add("Math.Log10", i, x, null, Math.Log10(x), s);
			t64.Add("Math.Sqrt", i, x, null, Math.Sqrt(x), s);
			t64.Add("Math.Pow(x,0.4)", i, x, 0.4, Math.Pow(x, 0.4), s);
			t64.Add("Math.Pow(|x|,1/3)", i, Math.Abs(x), third, Math.Pow(Math.Abs(x), third), s);
			t64.Add("Math.Pow(x,2.0)", i, x, 2.0, Math.Pow(x, 2.0), s);
			t64.Add("Math.Pow(1.0000001,x)", i, 1.0000001, x, Math.Pow(1.0000001, x), s);
			t64.Add("Math.Floor", i, x, null, Math.Floor(x), s);
			t64.Add("Math.Ceiling", i, x, null, Math.Ceiling(x), s);
			t64.Add("Math.Round", i, x, null, Math.Round(x), s);
			t64.Add("Math.Round(AwayFromZero)", i, x, null, Math.Round(x, MidpointRounding.AwayFromZero), s);
			t64.Add("Math.IEEERemainder(x,2pi)", i, x, twoPi, Math.IEEERemainder(x, twoPi), s);
			t64.Add("x%2pi", i, x, twoPi, x % twoPi, s);
		}

		// ---- float32 wrappers (the bodies of UnityEngine.Mathf.Sin etc. are exactly (float)Math.Sin((double)f))
		HcJson.Table t32 = g.AddTable(new HcJson.Table("f32", "float32 wrappers of System.Math on f = (float)x (Mathf bodies), plus float %."));
		t32.C("fn", HcJson.CT.Str).C("i", HcJson.CT.I64).C("x", HcJson.CT.F32).C("r", HcJson.CT.F32).Keys("fn", "i");
		for (int i = 0; i < xs.Count; i++)
		{
			float f = (float)xs[i];
			t32.Add("(float)Math.Sin(f)", i, f, (float)Math.Sin(f));
			t32.Add("(float)Math.Cos(f)", i, f, (float)Math.Cos(f));
			t32.Add("(float)Math.Tan(f)", i, f, (float)Math.Tan(f));
			t32.Add("(float)Math.Asin(f)", i, f, (float)Math.Asin(f));
			t32.Add("(float)Math.Acos(f)", i, f, (float)Math.Acos(f));
			t32.Add("(float)Math.Atan(f)", i, f, (float)Math.Atan(f));
			t32.Add("(float)Math.Atan2(f,0.7f)", i, f, (float)Math.Atan2(f, 0.7f));
			t32.Add("(float)Math.Exp(f)", i, f, (float)Math.Exp(f));
			t32.Add("(float)Math.Log(f)", i, f, (float)Math.Log(f));
			t32.Add("(float)Math.Log10(f)", i, f, (float)Math.Log10(f));
			t32.Add("(float)Math.Sqrt(f)", i, f, (float)Math.Sqrt(f));
			t32.Add("(float)Math.Pow(f,0.4f)", i, f, (float)Math.Pow(f, 0.4f));
			t32.Add("(float)Math.Floor(f)", i, f, (float)Math.Floor(f));
			t32.Add("(float)Math.Round(f)", i, f, (float)Math.Round(f));
			t32.Add("f%360f", i, f, f % 360f);
		}

		// ---- float32 evaluation mode probes
		int n = F.Length;
		float[] oPoly = new float[n];
		float[] oDot2 = new float[n];
		double[] oMulW = new double[n];
		double[] oMulAddW = new double[n];
		float[] oCast = new float[n];
		float[] oAcc = new float[n];
		float[] oCall = new float[n];
		float[] oLoc = new float[n];
		float[] oSqrt = new float[n];
		float[] oCmp = new float[n];
		ProbePoly(F, oPoly);
		ProbeDot2(F, oDot2);
		ProbeMulWide(F, oMulW);
		ProbeMulAddWide(F, oMulAddW);
		ProbeCastEach(F, oCast);
		ProbeAccLoop(F, oAcc);
		ProbeDot2Call(F, oCall);
		ProbeLocals(F, oLoc);
		ProbeSqrtDiv(F, oSqrt);
		ProbeCompare(F, oCmp);
		float[] oArgNI = new float[n];
		float[] oArgAI = new float[n];
		ProbeArgs(F, oArgNI, oArgAI);
		double[] oRetNI = new double[n];
		double[] oRetAI = new double[n];
		double[] oFld = new double[n];
		double[] oStr = new double[n];
		double[] oSta = new double[n];
		double[] oLoc2 = new double[n];
		ProbeStores(F, oRetNI, oRetAI, oFld, oStr, oSta, oLoc2);
		HcJson.Table tp = g.AddTable(new HcJson.Table("f32ops", "float32 expression probes over f32_inputs.tsv rows (a,b,c,d,e,x). poly: ((x*1.1f+0.3f)*x-0.7f)/3.3f; dot2: a*b+c*d-e; cast_each: (float)((float)((float)(a*b)+(float)(c*d))-e); acc_loop: 16x acc=acc*0.999f+a*b; dot2_call: same as dot2 in a NoInlining method; locals: t1=a*b; t2=c*d; t1+t2-e; sqrt_div: (float)Math.Sqrt(a*a+b*b)/c; compare: (a*b+c == p+c) with float p=a*b, as 1/0; arg_noinline / arg_inline: AddNI(a*b, c) / AddAI(a*b, c) (float p+q in a NoInlining / AggressiveInlining method)."));
		tp.C("probe", HcJson.CT.Str).C("i", HcJson.CT.I64).C("a", HcJson.CT.F32).C("b", HcJson.CT.F32).C("c", HcJson.CT.F32).C("d", HcJson.CT.F32).C("e", HcJson.CT.F32).C("x", HcJson.CT.F32).C("r", HcJson.CT.F32).Keys("probe", "i");
		HcJson.Table tw = g.AddTable(new HcJson.Table("f32ops_wide", "float32 expressions assigned to double. mul_wide: double r = a*b; muladd_wide: double r = a*b+c. With per-operation float32 rounding r is the float32-rounded value; with double intermediates it is the exact product. ret_noinline / ret_inline: double r = MulNI(a,b) / MulAI(a,b) (float p*q returned from a NoInlining / AggressiveInlining method); field / struct_field / static_field / local: a*b stored to a float class field / local struct field / static field / local, read back as double."));
		tw.C("probe", HcJson.CT.Str).C("i", HcJson.CT.I64).C("a", HcJson.CT.F32).C("b", HcJson.CT.F32).C("c", HcJson.CT.F32).C("r", HcJson.CT.F64).Keys("probe", "i");
		for (int i = 0; i < n; i++)
		{
			float[] v = F[i];
			tp.Add("poly", i, v[0], v[1], v[2], v[3], v[4], v[5], oPoly[i]);
			tp.Add("dot2", i, v[0], v[1], v[2], v[3], v[4], v[5], oDot2[i]);
			tp.Add("cast_each", i, v[0], v[1], v[2], v[3], v[4], v[5], oCast[i]);
			tp.Add("acc_loop", i, v[0], v[1], v[2], v[3], v[4], v[5], oAcc[i]);
			tp.Add("dot2_call", i, v[0], v[1], v[2], v[3], v[4], v[5], oCall[i]);
			tp.Add("locals", i, v[0], v[1], v[2], v[3], v[4], v[5], oLoc[i]);
			tp.Add("sqrt_div", i, v[0], v[1], v[2], v[3], v[4], v[5], oSqrt[i]);
			tp.Add("compare", i, v[0], v[1], v[2], v[3], v[4], v[5], oCmp[i]);
			tp.Add("arg_noinline", i, v[0], v[1], v[2], v[3], v[4], v[5], oArgNI[i]);
			tp.Add("arg_inline", i, v[0], v[1], v[2], v[3], v[4], v[5], oArgAI[i]);
			tw.Add("ret_noinline", i, v[0], v[1], v[2], oRetNI[i]);
			tw.Add("ret_inline", i, v[0], v[1], v[2], oRetAI[i]);
			tw.Add("field", i, v[0], v[1], v[2], oFld[i]);
			tw.Add("struct_field", i, v[0], v[1], v[2], oStr[i]);
			tw.Add("static_field", i, v[0], v[1], v[2], oSta[i]);
			tw.Add("local", i, v[0], v[1], v[2], oLoc2[i]);
			tw.Add("mul_wide", i, v[0], v[1], v[2], oMulW[i]);
			tw.Add("muladd_wide", i, v[0], v[1], v[2], oMulAddW[i]);
		}

		// ---- conversions
		HcJson.Table tc = g.AddTable(new HcJson.Table("conv", "Unchecked C# conversions of special doubles/floats to integers (the ECMA spec leaves out-of-range and NaN results unspecified)."));
		tc.C("fn", HcJson.CT.Str).C("i", HcJson.CT.I64).C("x", HcJson.CT.F64).C("r", HcJson.CT.I64).C("err", HcJson.CT.Str).Keys("fn", "i");
		double[] cx = new double[]
		{
			double.NaN, double.PositiveInfinity, double.NegativeInfinity, 0.0, -0.0, 0.5, -0.5, 0.9999999, -0.9999999, 1.5, 2.5, -2.5, 3.5, 255.5, 256.0, -129.0,
			32767.5, 32768.0, 65536.0, 2147483647.0, 2147483647.5, 2147483648.0, -2147483648.0, -2147483648.5, -2147483649.0, 4294967295.0, 4294967296.0,
			-4294967296.0, 9007199254740993.0, 9.2233720368547758E+18, -9.2233720368547758E+18, 1e19, 1.8446744073709552E+19, -1e19, 1e20, -1e20, 1e30,
			3.4028234663852886E+38, 1e300
		};
		for (int i = 0; i < cx.Length; i++)
		{
			double x = cx[i];
			float f = (float)x;
			tc.Add("(int)d", i, x, (long)unchecked((int)x), null);
			tc.Add("(long)d", i, x, unchecked((long)x), null);
			tc.Add("(uint)d", i, x, (long)unchecked((uint)x), null);
			tc.Add("(ulong)d", i, x, unchecked((long)(ulong)x), null);
			tc.Add("(short)d", i, x, (long)unchecked((short)x), null);
			tc.Add("(byte)d", i, x, (long)unchecked((byte)x), null);
			tc.Add("(int)(float)d", i, x, (long)unchecked((int)f), null);
			tc.Add("(long)(float)d", i, x, unchecked((long)f), null);
			tc.Add("(uint)(float)d", i, x, (long)unchecked((uint)f), null);
			long r1 = 0;
			string e1 = null;
			try
			{
				r1 = Convert.ToInt32(x);
			}
			catch (Exception ex)
			{
				e1 = ex.GetType().Name;
			}
			tc.Add("Convert.ToInt32(d)", i, x, (e1 == null) ? (object)r1 : null, e1);
		}
		HcJson.Table ti = g.AddTable(new HcJson.Table("iconv", "Integer to floating-point conversions. v is the integer (decimal string, unsigned for ulong rows)."));
		ti.C("fn", HcJson.CT.Str).C("i", HcJson.CT.I64).C("v", HcJson.CT.Str).C("r", HcJson.CT.F64).Keys("fn", "i");
		ulong[] uv = new ulong[]
		{
			0uL, 1uL, 16777217uL, 9007199254740993uL, 9223372036854775807uL, 9223372036854775808uL, 9223372036854776833uL, 9223372036854777856uL,
			18446744073709550592uL, 18446744073709551615uL, 18014398509481985uL, 2147483649uL
		};
		for (int i = 0; i < uv.Length; i++)
		{
			ulong u = uv[i];
			long l = unchecked((long)u);
			string us = u.ToString(CultureInfo.InvariantCulture);
			ti.Add("(double)ulong", i, us, (double)u);
			ti.Add("(double)(float)ulong", i, us, (double)(float)u);
			ti.Add("(double)long", i, l.ToString(CultureInfo.InvariantCulture), (double)l);
			ti.Add("(double)(float)long", i, l.ToString(CultureInfo.InvariantCulture), (double)(float)l);
			uint ui = unchecked((uint)u);
			ti.Add("(double)(float)uint", i, ui.ToString(CultureInfo.InvariantCulture), (double)(float)ui);
			int ii = unchecked((int)u);
			ti.Add("(double)(float)int", i, ii.ToString(CultureInfo.InvariantCulture), (double)(float)ii);
		}

		// ---- formatting / parsing (InvariantCulture)
		HcJson.Table ts = g.AddTable(new HcJson.Table("tostring", "double/float formatting with InvariantCulture; rt = Parse(s) gives back the same bits."));
		ts.C("fn", HcJson.CT.Str).C("i", HcJson.CT.I64).C("x", HcJson.CT.F64).C("s", HcJson.CT.Str).C("rt", HcJson.CT.Bool).Keys("fn", "i");
		CultureInfo inv = CultureInfo.InvariantCulture;
		int nts = Math.Min(xs.Count, 600);
		for (int i = 0; i < nts; i++)
		{
			double x = xs[i];
			float f = (float)x;
			string s1 = x.ToString(inv);
			string s2 = x.ToString("R", inv);
			string s3 = x.ToString("G17", inv);
			string s4 = f.ToString(inv);
			string s5 = f.ToString("R", inv);
			string s6 = x.ToString("F6", inv);
			string s7 = f.ToString("G9", inv);
			ts.Add("d.ToString()", i, x, s1, RtD(s1, x));
			ts.Add("d.ToString(R)", i, x, s2, RtD(s2, x));
			ts.Add("d.ToString(G17)", i, x, s3, RtD(s3, x));
			ts.Add("d.ToString(F6)", i, x, s6, null);
			ts.Add("f.ToString()", i, x, s4, RtF(s4, f));
			ts.Add("f.ToString(R)", i, x, s5, RtF(s5, f));
			ts.Add("f.ToString(G9)", i, x, s7, RtF(s7, f));
		}
		HcJson.Table tq = g.AddTable(new HcJson.Table("parse", "float.Parse / double.Parse(s, NumberStyles.Float|AllowThousands, InvariantCulture) of every numeric token in the curves_eval cfg keys plus edge cases (parse_tokens.tsv)."));
		tq.C("i", HcJson.CT.I64).C("s", HcJson.CT.Str).C("f", HcJson.CT.F32).C("d", HcJson.CT.F64).C("err", HcJson.CT.Str).Keys("i");
		List<string[]> toks = HcJson.ReadTsv(Path.Combine(inDir, "parse_tokens.tsv"));
		NumberStyles ns = NumberStyles.Float | NumberStyles.AllowThousands;
		for (int i = 0; i < toks.Count; i++)
		{
			string s = toks[i][0];
			object fv = null;
			object dv = null;
			string err = null;
			try
			{
				fv = float.Parse(s, ns, inv);
			}
			catch (Exception ex2)
			{
				err = "float:" + ex2.GetType().Name;
			}
			try
			{
				dv = double.Parse(s, ns, inv);
			}
			catch (Exception ex3)
			{
				err = ((err == null) ? "" : (err + " ")) + "double:" + ex3.GetType().Name;
			}
			tq.Add(i, s, fv, dv, err);
		}

		// ---- System.Random
		HcJson.Table tr = g.AddTable(new HcJson.Table("sysrand", "System.Random(seed): independent generators for each column (Next(), NextDouble(), Next(0,100), Next(-1000,1000))."));
		tr.C("seed", HcJson.CT.I64).C("k", HcJson.CT.I64).C("next", HcJson.CT.I64).C("nextDouble", HcJson.CT.F64).C("next0_100", HcJson.CT.I64).C("nextm1000_1000", HcJson.CT.I64).Keys("seed", "k");
		int[] seeds = new int[] { 0, 1, 7, 11, 42, 12345, 20261008, int.MaxValue, -5 };
		foreach (int seed in seeds)
		{
			System.Random r1 = new System.Random(seed);
			System.Random r2 = new System.Random(seed);
			System.Random r3 = new System.Random(seed);
			System.Random r4 = new System.Random(seed);
			for (int k = 0; k < 32; k++)
			{
				tr.Add(seed, k, r1.Next(), r2.NextDouble(), r3.Next(0, 100), r4.Next(-1000, 1000));
			}
		}
		return g;
	}

	static bool RtD(string s, double x)
	{
		try
		{
			double y = double.Parse(s, NumberStyles.Float, CultureInfo.InvariantCulture);
			return BitConverter.DoubleToInt64Bits(y) == BitConverter.DoubleToInt64Bits(x) || (double.IsNaN(x) && double.IsNaN(y));
		}
		catch (Exception)
		{
			return false;
		}
	}

	static bool RtF(string s, float f)
	{
		try
		{
			float y = float.Parse(s, NumberStyles.Float, CultureInfo.InvariantCulture);
			return BitConverter.ToInt32(BitConverter.GetBytes(y), 0) == BitConverter.ToInt32(BitConverter.GetBytes(f), 0) || (float.IsNaN(f) && float.IsNaN(y));
		}
		catch (Exception)
		{
			return false;
		}
	}
}
