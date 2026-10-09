using System.Globalization;
using System.Runtime.InteropServices;
using System.Security;

namespace System;

[Serializable]
[CLSCompliant(false)]
[ComVisible(true)]
public struct SByte : IComparable, IFormattable, IConvertible, IComparable<sbyte>, IEquatable<sbyte>
{
	private sbyte m_value;

	public const sbyte MaxValue = 127;

	public const sbyte MinValue = -128;

	public int CompareTo(object obj)
	{
		if (obj == null)
		{
			return 1;
		}
		if (!(obj is sbyte))
		{
			throw new ArgumentException(Environment.GetResourceString("Object must be of type SByte."));
		}
		return this - (sbyte)obj;
	}

	public int CompareTo(sbyte value)
	{
		return this - value;
	}

	public override bool Equals(object obj)
	{
		if (!(obj is sbyte))
		{
			return false;
		}
		return this == (sbyte)obj;
	}

	public bool Equals(sbyte obj)
	{
		return this == obj;
	}

	public override int GetHashCode()
	{
		return this ^ (this << 8);
	}

	[SecuritySafeCritical]
	public override string ToString()
	{
		return Number.FormatInt32(this, null, NumberFormatInfo.CurrentInfo);
	}

	[SecuritySafeCritical]
	public string ToString(IFormatProvider provider)
	{
		return Number.FormatInt32(this, null, NumberFormatInfo.GetInstance(provider));
	}

	public string ToString(string format)
	{
		return ToString(format, NumberFormatInfo.CurrentInfo);
	}

	public string ToString(string format, IFormatProvider provider)
	{
		return ToString(format, NumberFormatInfo.GetInstance(provider));
	}

	[SecuritySafeCritical]
	private string ToString(string format, NumberFormatInfo info)
	{
		if (this < 0 && format != null && format.Length > 0 && (format[0] == 'X' || format[0] == 'x'))
		{
			return Number.FormatUInt32((uint)this & 0xFFu, format, info);
		}
		return Number.FormatInt32(this, format, info);
	}

	[CLSCompliant(false)]
	public static sbyte Parse(string s)
	{
		return Parse(s, NumberStyles.Integer, NumberFormatInfo.CurrentInfo);
	}

	[CLSCompliant(false)]
	public static sbyte Parse(string s, NumberStyles style)
	{
		NumberFormatInfo.ValidateParseStyleInteger(style);
		return Parse(s, style, NumberFormatInfo.CurrentInfo);
	}

	[CLSCompliant(false)]
	public static sbyte Parse(string s, IFormatProvider provider)
	{
		return Parse(s, NumberStyles.Integer, NumberFormatInfo.GetInstance(provider));
	}

	[CLSCompliant(false)]
	public static sbyte Parse(string s, NumberStyles style, IFormatProvider provider)
	{
		NumberFormatInfo.ValidateParseStyleInteger(style);
		return Parse(s, style, NumberFormatInfo.GetInstance(provider));
	}

	private static sbyte Parse(string s, NumberStyles style, NumberFormatInfo info)
	{
		int num = 0;
		try
		{
			num = Number.ParseInt32(s, style, info);
		}
		catch (OverflowException innerException)
		{
			throw new OverflowException(Environment.GetResourceString("Value was either too large or too small for a signed byte."), innerException);
		}
		if ((style & NumberStyles.AllowHexSpecifier) != 0)
		{
			if (num < 0 || num > 255)
			{
				throw new OverflowException(Environment.GetResourceString("Value was either too large or too small for a signed byte."));
			}
			return (sbyte)num;
		}
		if (num < -128 || num > 127)
		{
			throw new OverflowException(Environment.GetResourceString("Value was either too large or too small for a signed byte."));
		}
		return (sbyte)num;
	}

	[CLSCompliant(false)]
	public static bool TryParse(string s, out sbyte result)
	{
		return TryParse(s, NumberStyles.Integer, NumberFormatInfo.CurrentInfo, out result);
	}

	[CLSCompliant(false)]
	public static bool TryParse(string s, NumberStyles style, IFormatProvider provider, out sbyte result)
	{
		NumberFormatInfo.ValidateParseStyleInteger(style);
		return TryParse(s, style, NumberFormatInfo.GetInstance(provider), out result);
	}

	private static bool TryParse(string s, NumberStyles style, NumberFormatInfo info, out sbyte result)
	{
		result = 0;
		if (!Number.TryParseInt32(s, style, info, out var result2))
		{
			return false;
		}
		if ((style & NumberStyles.AllowHexSpecifier) != 0)
		{
			if (result2 < 0 || result2 > 255)
			{
				return false;
			}
			result = (sbyte)result2;
			return true;
		}
		if (result2 < -128 || result2 > 127)
		{
			return false;
		}
		result = (sbyte)result2;
		return true;
	}

	public TypeCode GetTypeCode()
	{
		return TypeCode.SByte;
	}

	bool IConvertible.ToBoolean(IFormatProvider provider)
	{
		return Convert.ToBoolean(this);
	}

	char IConvertible.ToChar(IFormatProvider provider)
	{
		return Convert.ToChar(this);
	}

	sbyte IConvertible.ToSByte(IFormatProvider provider)
	{
		return this;
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
		return this;
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
		return Convert.ToDouble(this);
	}

	decimal IConvertible.ToDecimal(IFormatProvider provider)
	{
		return Convert.ToDecimal(this);
	}

	DateTime IConvertible.ToDateTime(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "SByte", "DateTime"));
	}

	object IConvertible.ToType(Type type, IFormatProvider provider)
	{
		return Convert.DefaultToType(this, type, provider);
	}
}
