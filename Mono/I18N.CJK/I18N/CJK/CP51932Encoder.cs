using System;
using I18N.Common;

namespace I18N.CJK;

public class CP51932Encoder : MonoEncoder
{
	public CP51932Encoder(MonoEncoding encoding)
		: base(encoding)
	{
	}

	public unsafe override int GetByteCountImpl(char* chars, int count, bool refresh)
	{
		int num = 0;
		int num2 = 0;
		byte[] cjkToJis = JISConvert.Convert.cjkToJis;
		byte[] extraToJis = JISConvert.Convert.extraToJis;
		while (count > 0)
		{
			int num3 = *(ushort*)((byte*)chars + num++ * 2);
			count--;
			num2++;
			if (num3 < 128)
			{
				continue;
			}
			if (num3 < 256)
			{
				if (num3 == 162 || num3 == 163 || num3 == 167 || num3 == 168 || num3 == 172 || num3 == 176 || num3 == 177 || num3 == 180 || num3 == 182 || num3 == 215 || num3 == 247)
				{
					num2++;
				}
			}
			else if (num3 >= 913 && num3 <= 1105)
			{
				num2++;
			}
			else if (num3 >= 8208 && num3 <= 40869)
			{
				int num4 = (num3 - 8208) * 2;
				num4 = cjkToJis[num4] | (cjkToJis[num4 + 1] << 8);
				if (num4 >= 256)
				{
					num2++;
				}
			}
			else if (num3 >= 65281 && num3 < 65376)
			{
				int num4 = (num3 - 65281) * 2;
				num4 = extraToJis[num4] | (extraToJis[num4 + 1] << 8);
				if (num4 >= 256)
				{
					num2++;
				}
			}
			else if (num3 >= 65376 && num3 <= 65440)
			{
				num2++;
			}
		}
		return num2;
	}

	public unsafe override int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount, bool refresh)
	{
		int charIndex = 0;
		int num = 0;
		int byteIndex = num;
		int num2 = byteCount;
		byte[] cjkToJis = JISConvert.Convert.cjkToJis;
		byte[] greekToJis = JISConvert.Convert.greekToJis;
		byte[] extraToJis = JISConvert.Convert.extraToJis;
		while (charCount > 0)
		{
			int num3 = *(ushort*)((byte*)chars + charIndex * 2);
			if (byteIndex >= num2)
			{
				throw new ArgumentException(Strings.GetString("Arg_InsufficientSpace"), "bytes");
			}
			if (num3 < 128)
			{
				bytes[byteIndex++] = (byte)num3;
			}
			else
			{
				int num4;
				if (num3 >= 913 && num3 <= 1105)
				{
					num4 = (num3 - 913) * 2;
					num4 = greekToJis[num4] | (greekToJis[num4 + 1] << 8);
				}
				else if (num3 >= 8208 && num3 <= 40869)
				{
					num4 = (num3 - 8208) * 2;
					num4 = cjkToJis[num4] | (cjkToJis[num4 + 1] << 8);
				}
				else if (num3 < 65281 || num3 > 65376)
				{
					num4 = ((num3 >= 65376 && num3 <= 65440) ? (num3 - 65376 + 36512) : 0);
				}
				else
				{
					num4 = (num3 - 65281) * 2;
					num4 = extraToJis[num4] | (extraToJis[num4 + 1] << 8);
				}
				if (num4 == 0)
				{
					HandleFallback(chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
				}
				else if (num4 < 256)
				{
					bytes[byteIndex++] = (byte)num4;
				}
				else
				{
					if (byteIndex + 1 >= num2)
					{
						throw new ArgumentException(Strings.GetString("Arg_InsufficientSpace"), "bytes");
					}
					if (num4 < 32768)
					{
						num4 -= 256;
						bytes[byteIndex++] = (byte)(num4 / 94 + 161);
						bytes[byteIndex++] = (byte)(num4 % 94 + 161);
					}
					else
					{
						bytes[byteIndex++] = 142;
						bytes[byteIndex++] = (byte)(num4 - 36352);
					}
				}
			}
			charIndex++;
			charCount--;
		}
		return byteIndex - num;
	}
}
