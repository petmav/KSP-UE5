using System.Text;

namespace System;

internal static class ParseNumbers
{
	internal const int PrintAsI1 = 64;

	internal const int PrintAsI2 = 128;

	internal const int TreatAsUnsigned = 512;

	internal const int TreatAsI1 = 1024;

	internal const int TreatAsI2 = 2048;

	internal const int IsTight = 4096;

	internal const int NoSpace = 8192;

	private const ulong base16MaxOverflowFreeValue = 72057594037927935uL;

	private const ulong longMinValue = 9223372036854775808uL;

	public unsafe static int StringToInt(string value, int fromBase, int flags)
	{
		return StringToInt(value, fromBase, flags, null);
	}

	public unsafe static int StringToInt(string value, int fromBase, int flags, int* parsePos)
	{
		if ((flags & 0x3000) == 0)
		{
			throw new NotImplementedException(flags.ToString());
		}
		if (value == null)
		{
			return 0;
		}
		int num = 0;
		uint num2 = 0u;
		int length = value.Length;
		bool flag = false;
		if (length == 0)
		{
			throw new ArgumentOutOfRangeException("Empty string");
		}
		int i = ((parsePos != null) ? (*parsePos) : 0);
		if (value[i] == '-')
		{
			if (fromBase != 10)
			{
				throw new ArgumentException("String cannot contain a minus sign if the base is not 10.");
			}
			if (((uint)flags & 0x200u) != 0)
			{
				throw new OverflowException("Negative number");
			}
			flag = true;
			i++;
		}
		else if (value[i] == '+')
		{
			i++;
		}
		if (fromBase == 16 && i + 1 < length && value[i] == '0' && (value[i + 1] == 'x' || value[i + 1] == 'X'))
		{
			i += 2;
		}
		uint num3 = ((((uint)flags & 0x400u) != 0) ? 255u : (((flags & 0x800) == 0) ? uint.MaxValue : 65535u));
		for (; i < length; i++)
		{
			char c = value[i];
			int num4;
			if (char.IsNumber(c))
			{
				num4 = c - 48;
			}
			else
			{
				if (!char.IsLetter(c))
				{
					if (i == 0)
					{
						throw new FormatException("Could not find any parsable digits.");
					}
					if ((flags & 0x1000) == 0)
					{
						break;
					}
					throw new FormatException("Additional unparsable characters are at the end of the string.");
				}
				num4 = char.ToLowerInvariant(c) - 97 + 10;
			}
			if (num4 >= fromBase)
			{
				if (num > 0)
				{
					throw new FormatException("Additional unparsable characters are at the end of the string.");
				}
				throw new FormatException("Could not find any parsable digits.");
			}
			long num5 = fromBase * num2 + num4;
			if (num5 > num3)
			{
				throw new OverflowException();
			}
			num2 = (uint)num5;
			num++;
		}
		if (num == 0)
		{
			throw new FormatException("Could not find any parsable digits.");
		}
		if (parsePos != null)
		{
			*parsePos = i;
		}
		if (!flag)
		{
			return (int)num2;
		}
		return (int)(0 - num2);
	}

	public static string LongToString(long value, int toBase, int width, char paddingChar, int flags)
	{
		if (value == 0L)
		{
			return "0";
		}
		if (toBase == 10)
		{
			return value.ToString();
		}
		byte[] bytes = BitConverter.GetBytes(value);
		return toBase switch
		{
			2 => ConvertToBase2(bytes).ToString(), 
			8 => ConvertToBase8(bytes).ToString(), 
			16 => ConvertToBase16(bytes).ToString(), 
			_ => throw new NotImplementedException(), 
		};
	}

	public unsafe static long StringToLong(string value, int fromBase, int flags)
	{
		return StringToLong(value, fromBase, flags, null);
	}

	public unsafe static long StringToLong(string value, int fromBase, int flags, int* parsePos)
	{
		if ((flags & 0x3000) == 0)
		{
			throw new NotImplementedException(flags.ToString());
		}
		if (value == null)
		{
			return 0L;
		}
		int num = 0;
		ulong num2 = (ulong)fromBase;
		ulong num3 = 0uL;
		ulong num4 = 0uL;
		int length = value.Length;
		bool flag = false;
		bool flag2 = (flags & 0x200) != 0;
		if (length == 0)
		{
			throw new ArgumentOutOfRangeException("Empty string");
		}
		int i = ((parsePos != null) ? (*parsePos) : 0);
		if (value[i] == '-')
		{
			if (fromBase != 10)
			{
				throw new ArgumentException("String cannot contain a minus sign if the base is not 10.");
			}
			if (flag2)
			{
				throw new OverflowException("Negative number");
			}
			flag = true;
			i++;
		}
		else if (value[i] == '+')
		{
			i++;
		}
		if (fromBase == 16 && i + 1 < length && value[i] == '0' && (value[i + 1] == 'x' || value[i + 1] == 'X'))
		{
			i += 2;
		}
		for (; i < length; i++)
		{
			char c = value[i];
			if (char.IsNumber(c))
			{
				num3 = (ulong)(c - 48);
			}
			else
			{
				if (!char.IsLetter(c))
				{
					if (i == 0)
					{
						throw new FormatException("Could not find any parsable digits.");
					}
					if ((flags & 0x1000) == 0)
					{
						break;
					}
					throw new FormatException("Additional unparsable characters are at the end of the string.");
				}
				num3 = (ulong)(char.ToLowerInvariant(c) - 97 + 10);
			}
			if (num3 >= num2)
			{
				if (num > 0)
				{
					throw new FormatException("Additional unparsable characters are at the end of the string.");
				}
				throw new FormatException("Could not find any parsable digits.");
			}
			if (num4 <= 72057594037927935L)
			{
				num4 = num4 * num2 + num3;
			}
			else
			{
				ulong num5 = (num4 >> 32) * num2;
				ulong num6 = (num4 & 0xFFFFFFFFu) * num2 + num3;
				if ((num6 >> 32) + num5 > uint.MaxValue)
				{
					throw new OverflowException();
				}
				num4 = (num5 << 32) + num6;
			}
			num++;
		}
		if (num == 0)
		{
			throw new FormatException("Could not find any parsable digits.");
		}
		if (parsePos != null)
		{
			*parsePos = i;
		}
		if (flag2)
		{
			return (long)num4;
		}
		if (!flag)
		{
			if (fromBase == 10 && num4 > long.MaxValue)
			{
				throw new OverflowException();
			}
			return (long)num4;
		}
		if (num4 <= long.MaxValue)
		{
			return (long)(0L - num4);
		}
		if (num4 > 9223372036854775808uL)
		{
			throw new OverflowException();
		}
		return long.MinValue + (long.MinValue - (long)num4);
	}

	public static string IntToString(int value, int toBase, int width, char paddingChar, int flags)
	{
		StringBuilder stringBuilder;
		if (value == 0)
		{
			if (width <= 0)
			{
				return "0";
			}
			stringBuilder = new StringBuilder("0", width);
		}
		else if (toBase != 10)
		{
			byte[] value2 = ((((uint)flags & 0x40u) != 0) ? BitConverter.GetBytes((byte)value) : (((flags & 0x80) == 0) ? BitConverter.GetBytes(value) : BitConverter.GetBytes((short)value)));
			stringBuilder = toBase switch
			{
				2 => ConvertToBase2(value2), 
				8 => ConvertToBase8(value2), 
				16 => ConvertToBase16(value2), 
				_ => throw new NotImplementedException(), 
			};
		}
		else
		{
			stringBuilder = new StringBuilder(value.ToString());
		}
		for (int num = width - stringBuilder.Length; num > 0; num--)
		{
			stringBuilder.Insert(0, paddingChar);
		}
		return stringBuilder.ToString();
	}

	private static void EndianSwap(ref byte[] value)
	{
		byte[] array = new byte[value.Length];
		for (int i = 0; i < value.Length; i++)
		{
			array[i] = value[value.Length - 1 - i];
		}
		value = array;
	}

	private static StringBuilder ConvertToBase2(byte[] value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			EndianSwap(ref value);
		}
		StringBuilder stringBuilder = new StringBuilder();
		for (int num = value.Length - 1; num >= 0; num--)
		{
			byte b = value[num];
			for (int i = 0; i < 8; i++)
			{
				if ((b & 0x80) == 128)
				{
					stringBuilder.Append('1');
				}
				else if (stringBuilder.Length > 0)
				{
					stringBuilder.Append('0');
				}
				b <<= 1;
			}
		}
		return stringBuilder;
	}

	private static StringBuilder ConvertToBase8(byte[] value)
	{
		ulong num = 0uL;
		num = value.Length switch
		{
			1 => value[0], 
			2 => BitConverter.ToUInt16(value, 0), 
			4 => BitConverter.ToUInt32(value, 0), 
			8 => BitConverter.ToUInt64(value, 0), 
			_ => throw new ArgumentException("value"), 
		};
		StringBuilder stringBuilder = new StringBuilder();
		for (int num2 = 21; num2 >= 0; num2--)
		{
			char c = (char)((num >> num2 * 3) & 7);
			if (c != 0 || stringBuilder.Length > 0)
			{
				c = (char)(c + 48);
				stringBuilder.Append(c);
			}
		}
		return stringBuilder;
	}

	private static StringBuilder ConvertToBase16(byte[] value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			EndianSwap(ref value);
		}
		StringBuilder stringBuilder = new StringBuilder();
		for (int num = value.Length - 1; num >= 0; num--)
		{
			char c = (char)((uint)(value[num] >> 4) & 0xFu);
			if (c != 0 || stringBuilder.Length > 0)
			{
				if (c < '\n')
				{
					c = (char)(c + 48);
				}
				else
				{
					c = (char)(c - 10);
					c = (char)(c + 97);
				}
				stringBuilder.Append(c);
			}
			char c2 = (char)(value[num] & 0xFu);
			if (c2 != 0 || stringBuilder.Length > 0)
			{
				if (c2 < '\n')
				{
					c2 = (char)(c2 + 48);
				}
				else
				{
					c2 = (char)(c2 - 10);
					c2 = (char)(c2 + 97);
				}
				stringBuilder.Append(c2);
			}
		}
		return stringBuilder;
	}
}
