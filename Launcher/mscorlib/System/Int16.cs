using System.Globalization;
using System.Runtime.InteropServices;
using System.Security;

namespace System;

[Serializable]
[ComVisible(true)]
public struct Int16 : IComparable, IFormattable, IConvertible, IComparable<short>, IEquatable<short>
{
	internal short m_value;

	public const short MaxValue = 32767;

	public const short MinValue = -32768;

	public int CompareTo(object value)
	{
		if (value == null)
		{
			return 1;
		}
		if (value is short)
		{
			return this - (short)value;
		}
		throw new ArgumentException(Environment.GetResourceString("Object must be of type Int16."));
	}

	public int CompareTo(short value)
	{
		return this - value;
	}

	public override bool Equals(object obj)
	{
		if (!(obj is short))
		{
			return false;
		}
		return this == (short)obj;
	}

	public bool Equals(short obj)
	{
		return this == obj;
	}

	public override int GetHashCode()
	{
		return (ushort)this | (this << 16);
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
			return Number.FormatUInt32((uint)this & 0xFFFFu, format, info);
		}
		return Number.FormatInt32(this, format, info);
	}

	public static short Parse(string s)
	{
		return Parse(s, NumberStyles.Integer, NumberFormatInfo.CurrentInfo);
	}

	public static short Parse(string s, NumberStyles style)
	{
		NumberFormatInfo.ValidateParseStyleInteger(style);
		return Parse(s, style, NumberFormatInfo.CurrentInfo);
	}

	public static short Parse(string s, IFormatProvider provider)
	{
		return Parse(s, NumberStyles.Integer, NumberFormatInfo.GetInstance(provider));
	}

	public static short Parse(string s, NumberStyles style, IFormatProvider provider)
	{
		NumberFormatInfo.ValidateParseStyleInteger(style);
		return Parse(s, style, NumberFormatInfo.GetInstance(provider));
	}

	private static short Parse(string s, NumberStyles style, NumberFormatInfo info)
	{
		int num = 0;
		try
		{
			num = Number.ParseInt32(s, style, info);
		}
		catch (OverflowException innerException)
		{
			throw new OverflowException(Environment.GetResourceString("Value was either too large or too small for an Int16."), innerException);
		}
		if ((style & NumberStyles.AllowHexSpecifier) != 0)
		{
			if (num < 0 || num > 65535)
			{
				throw new OverflowException(Environment.GetResourceString("Value was either too large or too small for an Int16."));
			}
			return (short)num;
		}
		if (num < -32768 || num > 32767)
		{
			throw new OverflowException(Environment.GetResourceString("Value was either too large or too small for an Int16."));
		}
		return (short)num;
	}

	public static bool TryParse(string s, out short result)
	{
		return TryParse(s, NumberStyles.Integer, NumberFormatInfo.CurrentInfo, out result);
	}

	public static bool TryParse(string s, NumberStyles style, IFormatProvider provider, out short result)
	{
		NumberFormatInfo.ValidateParseStyleInteger(style);
		return TryParse(s, style, NumberFormatInfo.GetInstance(provider), out result);
	}

	private static bool TryParse(string s, NumberStyles style, NumberFormatInfo info, out short result)
	{
		result = 0;
		if (!Number.TryParseInt32(s, style, info, out var result2))
		{
			return false;
		}
		if ((style & NumberStyles.AllowHexSpecifier) != 0)
		{
			if (result2 < 0 || result2 > 65535)
			{
				return false;
			}
			result = (short)result2;
			return true;
		}
		if (result2 < -32768 || result2 > 32767)
		{
			return false;
		}
		result = (short)result2;
		return true;
	}

	public TypeCode GetTypeCode()
	{
		return TypeCode.Int16;
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
		return Convert.ToSByte(this);
	}

	byte IConvertible.ToByte(IFormatProvider provider)
	{
		return Convert.ToByte(this);
	}

	short IConvertible.ToInt16(IFormatProvider provider)
	{
		return this;
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
		return Convert.ToDouble(this);
	}

	decimal IConvertible.ToDecimal(IFormatProvider provider)
	{
		return Convert.ToDecimal(this);
	}

	DateTime IConvertible.ToDateTime(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "Int16", "DateTime"));
	}

	object IConvertible.ToType(Type type, IFormatProvider provider)
	{
		return Convert.DefaultToType(this, type, provider);
	}
}
