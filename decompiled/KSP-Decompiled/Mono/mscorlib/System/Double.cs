using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;
using System.Security;

namespace System;

[Serializable]
[ComVisible(true)]
public struct Double : IComparable, IFormattable, IConvertible, IComparable<double>, IEquatable<double>
{
	internal double m_value;

	public const double MinValue = -1.7976931348623157E+308;

	public const double MaxValue = 1.7976931348623157E+308;

	public const double Epsilon = 5E-324;

	public const double NegativeInfinity = -1.0 / 0.0;

	public const double PositiveInfinity = 1.0 / 0.0;

	public const double NaN = 0.0 / 0.0;

	internal static double NegativeZero = BitConverter.Int64BitsToDouble(long.MinValue);

	[SecuritySafeCritical]
	public unsafe static bool IsInfinity(double d)
	{
		return (*(long*)(&d) & 0x7FFFFFFFFFFFFFFFL) == 9218868437227405312L;
	}

	public static bool IsPositiveInfinity(double d)
	{
		if (d == double.PositiveInfinity)
		{
			return true;
		}
		return false;
	}

	public static bool IsNegativeInfinity(double d)
	{
		if (d == double.NegativeInfinity)
		{
			return true;
		}
		return false;
	}

	[SecuritySafeCritical]
	internal unsafe static bool IsNegative(double d)
	{
		return (*(long*)(&d) & long.MinValue) == long.MinValue;
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
	[SecuritySafeCritical]
	public unsafe static bool IsNaN(double d)
	{
		return (ulong)(*(long*)(&d) & 0x7FFFFFFFFFFFFFFFL) > 9218868437227405312uL;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool IsFinite(double d)
	{
		return (BitConverter.DoubleToInt64Bits(d) & 0x7FFFFFFFFFFFFFFFL) < 9218868437227405312L;
	}

	public int CompareTo(object value)
	{
		if (value == null)
		{
			return 1;
		}
		if (value is double num)
		{
			if (this < num)
			{
				return -1;
			}
			if (this > num)
			{
				return 1;
			}
			if (this == num)
			{
				return 0;
			}
			if (IsNaN(this))
			{
				if (!IsNaN(num))
				{
					return -1;
				}
				return 0;
			}
			return 1;
		}
		throw new ArgumentException(Environment.GetResourceString("Object must be of type Double."));
	}

	public int CompareTo(double value)
	{
		if (this < value)
		{
			return -1;
		}
		if (this > value)
		{
			return 1;
		}
		if (this == value)
		{
			return 0;
		}
		if (IsNaN(this))
		{
			if (!IsNaN(value))
			{
				return -1;
			}
			return 0;
		}
		return 1;
	}

	public override bool Equals(object obj)
	{
		if (!(obj is double num))
		{
			return false;
		}
		if (num == this)
		{
			return true;
		}
		if (IsNaN(num))
		{
			return IsNaN(this);
		}
		return false;
	}

	public static bool operator ==(double left, double right)
	{
		return left == right;
	}

	public static bool operator !=(double left, double right)
	{
		return left != right;
	}

	public static bool operator <(double left, double right)
	{
		return left < right;
	}

	public static bool operator >(double left, double right)
	{
		return left > right;
	}

	public static bool operator <=(double left, double right)
	{
		return left <= right;
	}

	public static bool operator >=(double left, double right)
	{
		return left >= right;
	}

	public bool Equals(double obj)
	{
		if (obj == this)
		{
			return true;
		}
		if (IsNaN(obj))
		{
			return IsNaN(this);
		}
		return false;
	}

	[SecuritySafeCritical]
	public unsafe override int GetHashCode()
	{
		double num = this;
		if (num == 0.0)
		{
			return 0;
		}
		long num2 = *(long*)(&num);
		return (int)num2 ^ (int)(num2 >> 32);
	}

	[SecuritySafeCritical]
	public override string ToString()
	{
		return Number.FormatDouble(this, null, NumberFormatInfo.CurrentInfo);
	}

	[SecuritySafeCritical]
	public string ToString(string format)
	{
		return Number.FormatDouble(this, format, NumberFormatInfo.CurrentInfo);
	}

	[SecuritySafeCritical]
	public string ToString(IFormatProvider provider)
	{
		return Number.FormatDouble(this, null, NumberFormatInfo.GetInstance(provider));
	}

	[SecuritySafeCritical]
	public string ToString(string format, IFormatProvider provider)
	{
		return Number.FormatDouble(this, format, NumberFormatInfo.GetInstance(provider));
	}

	public static double Parse(string s)
	{
		return Parse(s, NumberStyles.Float | NumberStyles.AllowThousands, NumberFormatInfo.CurrentInfo);
	}

	public static double Parse(string s, NumberStyles style)
	{
		NumberFormatInfo.ValidateParseStyleFloatingPoint(style);
		return Parse(s, style, NumberFormatInfo.CurrentInfo);
	}

	public static double Parse(string s, IFormatProvider provider)
	{
		return Parse(s, NumberStyles.Float | NumberStyles.AllowThousands, NumberFormatInfo.GetInstance(provider));
	}

	public static double Parse(string s, NumberStyles style, IFormatProvider provider)
	{
		NumberFormatInfo.ValidateParseStyleFloatingPoint(style);
		return Parse(s, style, NumberFormatInfo.GetInstance(provider));
	}

	private static double Parse(string s, NumberStyles style, NumberFormatInfo info)
	{
		return Number.ParseDouble(s, style, info);
	}

	public static bool TryParse(string s, out double result)
	{
		return TryParse(s, NumberStyles.Float | NumberStyles.AllowThousands, NumberFormatInfo.CurrentInfo, out result);
	}

	public static bool TryParse(string s, NumberStyles style, IFormatProvider provider, out double result)
	{
		NumberFormatInfo.ValidateParseStyleFloatingPoint(style);
		return TryParse(s, style, NumberFormatInfo.GetInstance(provider), out result);
	}

	private static bool TryParse(string s, NumberStyles style, NumberFormatInfo info, out double result)
	{
		if (s == null)
		{
			result = 0.0;
			return false;
		}
		if (!Number.TryParseDouble(s, style, info, out result))
		{
			string text = s.Trim();
			if (text.Equals(info.PositiveInfinitySymbol))
			{
				result = double.PositiveInfinity;
			}
			else if (text.Equals(info.NegativeInfinitySymbol))
			{
				result = double.NegativeInfinity;
			}
			else
			{
				if (!text.Equals(info.NaNSymbol))
				{
					return false;
				}
				result = double.NaN;
			}
		}
		return true;
	}

	public TypeCode GetTypeCode()
	{
		return TypeCode.Double;
	}

	bool IConvertible.ToBoolean(IFormatProvider provider)
	{
		return Convert.ToBoolean(this);
	}

	char IConvertible.ToChar(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "Double", "Char"));
	}

	sbyte IConvertible.ToSByte(IFormatProvider provider)
	{
		return Convert.ToSByte(this);
	}

	byte IConvertible.ToByte(IFormatProvider provider)
	{
		return Convert.ToByte(this);
	}

	short IConvertible.ToInt16(IFormatProvider provider)
	{
		return Convert.ToInt16(this);
	}

	ushort IConvertible.ToUInt16(IFormatProvider provider)
	{
		return Convert.ToUInt16(this);
	}

	int IConvertible.ToInt32(IFormatProvider provider)
	{
		return Convert.ToInt32(this);
	}

	uint IConvertible.ToUInt32(IFormatProvider provider)
	{
		return Convert.ToUInt32(this);
	}

	long IConvertible.ToInt64(IFormatProvider provider)
	{
		return Convert.ToInt64(this);
	}

	ulong IConvertible.ToUInt64(IFormatProvider provider)
	{
		return Convert.ToUInt64(this);
	}

	float IConvertible.ToSingle(IFormatProvider provider)
	{
		return Convert.ToSingle(this);
	}

	double IConvertible.ToDouble(IFormatProvider provider)
	{
		return this;
	}

	decimal IConvertible.ToDecimal(IFormatProvider provider)
	{
		return Convert.ToDecimal(this);
	}

	DateTime IConvertible.ToDateTime(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "Double", "DateTime"));
	}

	object IConvertible.ToType(Type type, IFormatProvider provider)
	{
		return Convert.DefaultToType(this, type, provider);
	}
}
