using System;
using System.Text;
using I18N.Common;

namespace I18N.CJK;

[Serializable]
public class CP932 : MonoEncoding
{
	private const int SHIFTJIS_CODE_PAGE = 932;

	public override string BodyName => "iso-2022-jp";

	public override string EncodingName => "Japanese (Shift-JIS)";

	public override string HeaderName => "iso-2022-jp";

	public override bool IsBrowserDisplay => true;

	public override bool IsBrowserSave => true;

	public override bool IsMailNewsDisplay => true;

	public override bool IsMailNewsSave => true;

	public override string WebName => "shift_jis";

	public override int WindowsCodePage => 932;

	public CP932()
		: base(932)
	{
	}

	public unsafe override int GetByteCountImpl(char* chars, int count)
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
			else if (num3 >= 57344 && num3 <= 59223)
			{
				num2++;
			}
			else if (num3 >= 65281 && num3 <= 65519)
			{
				int num4 = (num3 - 65281) * 2;
				num4 = extraToJis[num4] | (extraToJis[num4 + 1] << 8);
				if (num4 >= 256)
				{
					num2++;
				}
			}
		}
		return num2;
	}

	public unsafe override int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount)
	{
		int charIndex = 0;
		int num = 0;
		EncoderFallbackBuffer buffer = null;
		int byteIndex = num;
		int num2 = byteCount;
		byte[] cjkToJis = JISConvert.Convert.cjkToJis;
		byte[] greekToJis = JISConvert.Convert.greekToJis;
		byte[] extraToJis = JISConvert.Convert.extraToJis;
		while (charCount > 0)
		{
			int num3 = *(ushort*)((byte*)chars + charIndex++ * 2);
			charCount--;
			if (byteIndex >= num2)
			{
				throw new ArgumentException(Strings.GetString("Arg_InsufficientSpace"), "bytes");
			}
			if (num3 < 128)
			{
				bytes[byteIndex++] = (byte)num3;
				continue;
			}
			if (num3 < 256)
			{
				switch (num3)
				{
				case 162:
				case 163:
				case 167:
				case 168:
				case 172:
				case 176:
				case 177:
				case 180:
				case 182:
				case 215:
				case 247:
					if (byteIndex + 1 >= num2)
					{
						throw new ArgumentException(Strings.GetString("Arg_InsufficientSpace"), "bytes");
					}
					switch (num3)
					{
					case 162:
						bytes[byteIndex++] = 129;
						bytes[byteIndex++] = 145;
						break;
					case 163:
						bytes[byteIndex++] = 129;
						bytes[byteIndex++] = 146;
						break;
					case 167:
						bytes[byteIndex++] = 129;
						bytes[byteIndex++] = 152;
						break;
					case 168:
						bytes[byteIndex++] = 129;
						bytes[byteIndex++] = 78;
						break;
					case 172:
						bytes[byteIndex++] = 129;
						bytes[byteIndex++] = 202;
						break;
					case 176:
						bytes[byteIndex++] = 129;
						bytes[byteIndex++] = 139;
						break;
					case 177:
						bytes[byteIndex++] = 129;
						bytes[byteIndex++] = 125;
						break;
					case 180:
						bytes[byteIndex++] = 129;
						bytes[byteIndex++] = 76;
						break;
					case 182:
						bytes[byteIndex++] = 129;
						bytes[byteIndex++] = 247;
						break;
					case 215:
						bytes[byteIndex++] = 129;
						bytes[byteIndex++] = 126;
						break;
					case 247:
						bytes[byteIndex++] = 129;
						bytes[byteIndex++] = 128;
						break;
					}
					break;
				case 165:
					bytes[byteIndex++] = 92;
					break;
				default:
					HandleFallback(ref buffer, chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
					break;
				}
				continue;
			}
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
			else if (num3 >= 57344 && num3 <= 59223)
			{
				int num5 = num3 - 57344;
				num4 = (num5 / 188 << 8) + num5 % 188 + 61504;
				if (num4 % 256 >= 127)
				{
					num4++;
				}
			}
			else if (num3 < 65281 || num3 > 65376)
			{
				num4 = ((num3 >= 65376 && num3 <= 65440) ? (num3 - 65376 + 160) : 0);
			}
			else
			{
				num4 = (num3 - 65281) * 2;
				num4 = extraToJis[num4] | (extraToJis[num4 + 1] << 8);
			}
			if (num4 == 0)
			{
				HandleFallback(ref buffer, chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
				continue;
			}
			if (num4 < 256)
			{
				bytes[byteIndex++] = (byte)num4;
				continue;
			}
			if (byteIndex + 1 >= num2)
			{
				throw new ArgumentException(Strings.GetString("Arg_InsufficientSpace"), "bytes");
			}
			if (num4 < 32768)
			{
				num4 -= 256;
				num3 = num4 / 188;
				num4 = num4 % 188 + 64;
				if (num4 >= 127)
				{
					num4++;
				}
				if (num3 < 31)
				{
					bytes[byteIndex++] = (byte)(num3 + 129);
				}
				else
				{
					bytes[byteIndex++] = (byte)(num3 - 31 + 224);
				}
				bytes[byteIndex++] = (byte)num4;
			}
			else if (num4 >= 61504 && num4 <= 63996)
			{
				bytes[byteIndex++] = (byte)(num4 / 256);
				bytes[byteIndex++] = (byte)(num4 % 256);
			}
			else
			{
				bytes[byteIndex++] = 63;
				bytes[byteIndex++] = 63;
			}
		}
		return byteIndex - num;
	}

	public override int GetCharCount(byte[] bytes, int index, int count)
	{
		return new CP932Decoder(JISConvert.Convert).GetCharCount(bytes, index, count, refresh: true);
	}

	public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
	{
		return new CP932Decoder(JISConvert.Convert).GetChars(bytes, byteIndex, byteCount, chars, charIndex, refresh: true);
	}

	public override int GetMaxByteCount(int charCount)
	{
		if (charCount < 0)
		{
			throw new ArgumentOutOfRangeException("charCount", Strings.GetString("ArgRange_NonNegative"));
		}
		return charCount * 2;
	}

	public override int GetMaxCharCount(int byteCount)
	{
		if (byteCount < 0)
		{
			throw new ArgumentOutOfRangeException("byteCount", Strings.GetString("ArgRange_NonNegative"));
		}
		return byteCount;
	}

	public override Decoder GetDecoder()
	{
		return new CP932Decoder(JISConvert.Convert);
	}
}
