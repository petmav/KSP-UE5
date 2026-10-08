using System.Globalization;
using System.Security;
using System.Text;

namespace System.Numerics;

internal static class BigNumber
{
	private struct BigNumberBuffer
	{
		public StringBuilder digits;

		public int precision;

		public int scale;

		public bool sign;

		public static BigNumberBuffer Create()
		{
			BigNumberBuffer result = default(BigNumberBuffer);
			result.digits = new StringBuilder();
			return result;
		}
	}

	private const NumberStyles InvalidNumberStyles = ~(NumberStyles.Any | NumberStyles.AllowHexSpecifier);

	internal static bool TryValidateParseStyleInteger(NumberStyles style, out ArgumentException e)
	{
		if (((uint)style & 0xFFFFFC00u) != 0)
		{
			e = new ArgumentException(global::SR.Format("An undefined NumberStyles value is being used.", "style"));
			return false;
		}
		if ((style & NumberStyles.AllowHexSpecifier) != 0 && ((uint)style & 0xFFFFFDFCu) != 0)
		{
			e = new ArgumentException("With the AllowHexSpecifier bit set in the enum bit field, the only other valid bits that can be combined into the enum value must be a subset of those in HexNumber.");
			return false;
		}
		e = null;
		return true;
	}

	[SecuritySafeCritical]
	internal static bool TryParseBigInteger(string value, NumberStyles style, NumberFormatInfo info, out BigInteger result)
	{
		if (value == null)
		{
			result = default(BigInteger);
			return false;
		}
		return TryParseBigInteger(AsReadOnlySpan(value), style, info, out result);
	}

	[SecuritySafeCritical]
	internal static bool TryParseBigInteger(ReadOnlySpan<char> value, NumberStyles style, NumberFormatInfo info, out BigInteger result)
	{
		result = BigInteger.Zero;
		if (!TryValidateParseStyleInteger(style, out var e))
		{
			throw e;
		}
		BigNumberBuffer number = BigNumberBuffer.Create();
		if (!FormatProvider.TryStringToBigInteger(value, style, info, number.digits, out number.precision, out number.scale, out number.sign))
		{
			return false;
		}
		if ((style & NumberStyles.AllowHexSpecifier) != 0)
		{
			if (!HexNumberToBigInteger(ref number, ref result))
			{
				return false;
			}
		}
		else if (!NumberToBigInteger(ref number, ref result))
		{
			return false;
		}
		return true;
	}

	internal static BigInteger ParseBigInteger(string value, NumberStyles style, NumberFormatInfo info)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		return ParseBigInteger(AsReadOnlySpan(value), style, info);
	}

	private unsafe static ReadOnlySpan<char> AsReadOnlySpan(string s)
	{
		fixed (char* pointer = s)
		{
			return new ReadOnlySpan<char>(pointer, s.Length);
		}
	}

	internal static BigInteger ParseBigInteger(ReadOnlySpan<char> value, NumberStyles style, NumberFormatInfo info)
	{
		if (!TryValidateParseStyleInteger(style, out var e))
		{
			throw e;
		}
		BigInteger result = BigInteger.Zero;
		if (!TryParseBigInteger(value, style, info, out result))
		{
			throw new FormatException("The value could not be parsed.");
		}
		return result;
	}

	private static bool HexNumberToBigInteger(ref BigNumberBuffer number, ref BigInteger value)
	{
		if (number.digits == null || number.digits.Length == 0)
		{
			return false;
		}
		int num = number.digits.Length - 1;
		byte[] array = new byte[num / 2 + num % 2];
		bool flag = false;
		bool flag2 = false;
		int num2 = 0;
		for (int num3 = num - 1; num3 > -1; num3--)
		{
			char c = number.digits[num3];
			byte b = ((c >= '0' && c <= '9') ? ((byte)(c - 48)) : ((c < 'A' || c > 'F') ? ((byte)(c - 97 + 10)) : ((byte)(c - 65 + 10))));
			if (num3 == 0 && (b & 8) == 8)
			{
				flag2 = true;
			}
			if (flag)
			{
				array[num2] = (byte)(array[num2] | (b << 4));
				num2++;
			}
			else
			{
				array[num2] = (flag2 ? ((byte)(b | 0xF0u)) : b);
			}
			flag = !flag;
		}
		value = new BigInteger(array);
		return true;
	}

	private static bool NumberToBigInteger(ref BigNumberBuffer number, ref BigInteger value)
	{
		int num = number.scale;
		int index = 0;
		BigInteger bigInteger = 10;
		value = 0;
		while (--num >= 0)
		{
			value *= bigInteger;
			if (number.digits[index] != 0)
			{
				value += (BigInteger)(number.digits[index++] - 48);
			}
		}
		while (number.digits[index] != 0)
		{
			if (number.digits[index++] != '0')
			{
				return false;
			}
		}
		if (number.sign)
		{
			value = -value;
		}
		return true;
	}

	internal static char ParseFormatSpecifier(string format, out int digits)
	{
		digits = -1;
		if (string.IsNullOrEmpty(format))
		{
			return 'R';
		}
		int num = 0;
		char c = format[num];
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
		{
			num++;
			int num2 = -1;
			if (num < format.Length && format[num] >= '0' && format[num] <= '9')
			{
				num2 = format[num++] - 48;
				while (num < format.Length && format[num] >= '0' && format[num] <= '9')
				{
					num2 = num2 * 10 + (format[num++] - 48);
					if (num2 >= 10)
					{
						break;
					}
				}
			}
			if (num >= format.Length || format[num] == '\0')
			{
				digits = num2;
				return c;
			}
		}
		return '\0';
	}

	private static string FormatBigIntegerToHexString(BigInteger value, char format, int digits, NumberFormatInfo info)
	{
		StringBuilder stringBuilder = new StringBuilder();
		byte[] array = value.ToByteArray();
		string text = null;
		int num = array.Length - 1;
		if (num > -1)
		{
			bool flag = false;
			byte b = array[num];
			if (b > 247)
			{
				b -= 240;
				flag = true;
			}
			if (b < 8 || flag)
			{
				text = string.Format(CultureInfo.InvariantCulture, "{0}1", format);
				stringBuilder.Append(b.ToString(text, info));
				num--;
			}
		}
		if (num > -1)
		{
			text = string.Format(CultureInfo.InvariantCulture, "{0}2", format);
			while (num > -1)
			{
				stringBuilder.Append(array[num--].ToString(text, info));
			}
		}
		if (digits > 0 && digits > stringBuilder.Length)
		{
			stringBuilder.Insert(0, (value._sign >= 0) ? "0" : ((format == 'x') ? "f" : "F"), digits - stringBuilder.Length);
		}
		return stringBuilder.ToString();
	}

	[SecuritySafeCritical]
	internal static string FormatBigInteger(BigInteger value, string format, NumberFormatInfo info)
	{
		int digits = 0;
		char c = ParseFormatSpecifier(format, out digits);
		int num;
		switch (c)
		{
		case 'X':
		case 'x':
			return FormatBigIntegerToHexString(value, c, digits, info);
		default:
			num = ((c == 'R') ? 1 : 0);
			break;
		case 'D':
		case 'G':
		case 'd':
		case 'g':
		case 'r':
			num = 1;
			break;
		}
		bool flag = (byte)num != 0;
		if (value._bits == null)
		{
			if (c == 'g' || c == 'G' || c == 'r' || c == 'R')
			{
				format = ((digits <= 0) ? "D" : string.Format(CultureInfo.InvariantCulture, "D{0}", digits.ToString(CultureInfo.InvariantCulture)));
			}
			return value._sign.ToString(format, info);
		}
		int num2 = value._bits.Length;
		uint[] array;
		int num4;
		int num5;
		checked
		{
			int num3;
			try
			{
				num3 = unchecked(checked(num2 * 10) / 9) + 2;
			}
			catch (OverflowException innerException)
			{
				throw new FormatException("The value is too large to be represented by this format specifier.", innerException);
			}
			array = new uint[num3];
			num4 = 0;
			num5 = num2;
		}
		while (--num5 >= 0)
		{
			uint num6 = value._bits[num5];
			for (int i = 0; i < num4; i++)
			{
				ulong num7 = NumericsHelpers.MakeUlong(array[i], num6);
				array[i] = (uint)(num7 % 1000000000);
				num6 = (uint)(num7 / 1000000000);
			}
			if (num6 != 0)
			{
				array[num4++] = num6 % 1000000000;
				num6 /= 1000000000;
				if (num6 != 0)
				{
					array[num4++] = num6;
				}
			}
		}
		int num8;
		char[] array2;
		int num10;
		checked
		{
			try
			{
				num8 = num4 * 9;
			}
			catch (OverflowException innerException2)
			{
				throw new FormatException("The value is too large to be represented by this format specifier.", innerException2);
			}
			if (flag)
			{
				if (digits > 0 && digits > num8)
				{
					num8 = digits;
				}
				if (value._sign < 0)
				{
					try
					{
						num8 += info.NegativeSign.Length;
					}
					catch (OverflowException innerException3)
					{
						throw new FormatException("The value is too large to be represented by this format specifier.", innerException3);
					}
				}
			}
			int num9;
			try
			{
				num9 = num8 + 1;
			}
			catch (OverflowException innerException4)
			{
				throw new FormatException("The value is too large to be represented by this format specifier.", innerException4);
			}
			array2 = new char[num9];
			num10 = num8;
		}
		for (int j = 0; j < num4 - 1; j++)
		{
			uint num11 = array[j];
			int num12 = 9;
			while (--num12 >= 0)
			{
				array2[--num10] = (char)(48 + num11 % 10);
				num11 /= 10;
			}
		}
		for (uint num13 = array[num4 - 1]; num13 != 0; num13 /= 10)
		{
			array2[--num10] = (char)(48 + num13 % 10);
		}
		if (!flag)
		{
			bool sign = value._sign < 0;
			int scale = num8 - num10;
			return FormatProvider.FormatBigInteger(29, scale, sign, format, info, array2, num10);
		}
		int num14 = num8 - num10;
		while (digits > 0 && digits > num14)
		{
			array2[--num10] = '0';
			digits--;
		}
		if (value._sign < 0)
		{
			_ = info.NegativeSign;
			for (int num15 = info.NegativeSign.Length - 1; num15 > -1; num15--)
			{
				array2[--num10] = info.NegativeSign[num15];
			}
		}
		return new string(array2, num10, num8 - num10);
	}
}
