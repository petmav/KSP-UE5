using System;
using System.Globalization;
using System.Numerics;

namespace KspGolden.Infra
{
    /// <summary>
    /// Correctly rounded decimal-string -> double conversion (BigInteger based; .NET Framework's double.Parse
    /// is not guaranteed correctly rounded for >15 significant digits) and shortest round-trip formatting.
    /// </summary>
    public static class ExactNumber
    {
        public static double ParseDouble(string s)
        {
            s = s.Trim();
            if (s == "NaN") return double.NaN;
            if (s == "Infinity" || s == "+Infinity") return double.PositiveInfinity;
            if (s == "-Infinity") return double.NegativeInfinity;
            int i = 0;
            bool neg = false;
            if (s[i] == '-') { neg = true; i++; } else if (s[i] == '+') i++;
            BigInteger mant = BigInteger.Zero;
            int exp10 = 0;
            bool any = false;
            for (; i < s.Length && char.IsDigit(s[i]); i++) { mant = mant * 10 + (s[i] - '0'); any = true; }
            if (i < s.Length && s[i] == '.')
            {
                i++;
                for (; i < s.Length && char.IsDigit(s[i]); i++) { mant = mant * 10 + (s[i] - '0'); exp10--; any = true; }
            }
            if (!any) throw new FormatException("bad number: " + s);
            if (i < s.Length && (s[i] == 'e' || s[i] == 'E'))
            {
                i++;
                int esign = 1;
                if (s[i] == '-') { esign = -1; i++; } else if (s[i] == '+') i++;
                int e = 0;
                for (; i < s.Length && char.IsDigit(s[i]); i++) e = checked(e * 10 + (s[i] - '0'));
                exp10 += esign * e;
            }
            if (i != s.Length) throw new FormatException("bad number: " + s);
            double r = mant.IsZero ? 0.0 : RatToDouble(mant, exp10);
            return neg ? -r : r;
        }

        // value = mant * 10^exp10, mant > 0. Round half to even to 53 bits (subnormals handled).
        private static double RatToDouble(BigInteger mant, int exp10)
        {
            BigInteger num = mant, den = BigInteger.One;
            if (exp10 >= 0) num *= BigInteger.Pow(10, exp10); else den = BigInteger.Pow(10, -exp10);
            int k = (int)Math.Floor(BigInteger.Log(num, 2) - BigInteger.Log(den, 2));
            while (CmpPow2(num, den, k) < 0) k--;
            while (CmpPow2(num, den, k + 1) >= 0) k++;
            int e2 = k - 52;
            if (e2 < -1074) e2 = -1074;
            BigInteger n = num, d = den;
            if (e2 >= 0) d <<= e2; else n <<= -e2;
            BigInteger q = BigInteger.DivRem(n, d, out BigInteger rem);
            int c = (rem * 2).CompareTo(d);
            if (c > 0 || (c == 0 && !q.IsEven)) q += 1;
            if (q >= (BigInteger.One << 53)) { q >>= 1; e2++; }
            if (e2 > 971) return double.PositiveInfinity;
            ulong qq = (ulong)q;
            long bits = qq < (1UL << 52) ? (long)qq : ((long)(e2 + 1075) << 52) | (long)(qq & ((1UL << 52) - 1));
            return BitConverter.Int64BitsToDouble(bits);
        }

        // compare num/den with 2^k
        private static int CmpPow2(BigInteger num, BigInteger den, int k)
        {
            return k >= 0 ? num.CompareTo(den << k) : (num << -k).CompareTo(den);
        }

        public static string FormatDouble(double v)
        {
            if (double.IsNaN(v)) return "NaN";
            if (double.IsPositiveInfinity(v)) return "Infinity";
            if (double.IsNegativeInfinity(v)) return "-Infinity";
            if (v == 0) return (1 / v < 0) ? "-0.0" : "0.0";
            for (int p = 15; p <= 17; p++)
            {
                string s = v.ToString("G" + p, CultureInfo.InvariantCulture);
                if (BitConverter.DoubleToInt64Bits(ParseDouble(s)) == BitConverter.DoubleToInt64Bits(v)) return Normalize(s);
            }
            throw new InvalidOperationException("no round-trip representation for " + BitConverter.DoubleToInt64Bits(v).ToString("X16"));
        }

        public static string FormatFloat(float f)
        {
            if (float.IsNaN(f)) return "NaN";
            if (float.IsPositiveInfinity(f)) return "Infinity";
            if (float.IsNegativeInfinity(f)) return "-Infinity";
            if (f == 0) return (1 / f < 0) ? "-0.0" : "0.0";
            for (int p = 6; p <= 9; p++)
            {
                string s = f.ToString("G" + p, CultureInfo.InvariantCulture);
                if ((float)ParseDouble(s) == f) return Normalize(s);
            }
            throw new InvalidOperationException("no round-trip representation for float");
        }

        // JSON-friendly: "1E+15" -> "1e+15"; ensure a '.' or exponent is present so readers see a float.
        private static string Normalize(string s)
        {
            s = s.Replace("E", "e");
            if (s.IndexOf('.') < 0 && s.IndexOf('e') < 0) s += ".0";
            return s;
        }

        public static string FloatBitsHex(float f) => BitConverter.ToInt32(BitConverter.GetBytes(f), 0).ToString("X8");
        public static string DoubleBitsHex(double d) => BitConverter.DoubleToInt64Bits(d).ToString("X16");
    }
}
