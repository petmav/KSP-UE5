using I18N.Common;

namespace I18N.CJK;

internal class GB18030Encoder : MonoEncoder
{
	private static DbcsConvert gb2312 = DbcsConvert.Gb2312;

	private char incomplete_byte_count;

	private char incomplete_bytes;

	public GB18030Encoder(MonoEncoding owner)
		: base(owner)
	{
	}

	public unsafe override int GetByteCountImpl(char* chars, int count, bool refresh)
	{
		int num = 0;
		int num2 = 0;
		while (num < count)
		{
			char c = *(char*)((byte*)chars + num * 2);
			if (c < '\u0080')
			{
				num2++;
				num++;
				continue;
			}
			if (char.IsSurrogate(c))
			{
				if (num + 1 == count)
				{
					incomplete_byte_count = c;
					num++;
				}
				else
				{
					num2 += 4;
					num += 2;
				}
				continue;
			}
			if (c < '\u0080' || c == 'ÿ')
			{
				num2++;
				num++;
				continue;
			}
			byte b = gb2312.u2n[c * 2 + 1];
			byte b2 = gb2312.u2n[c * 2];
			if (b != 0 && b2 != 0)
			{
				num2 += 2;
				num++;
			}
			else
			{
				long num3 = GB18030Source.FromUCS(c);
				num2 = ((num3 >= 0) ? (num2 + 4) : (num2 + 1));
				num++;
			}
		}
		if (refresh)
		{
			if (incomplete_byte_count != 0)
			{
				num2++;
			}
			incomplete_byte_count = '\0';
		}
		return num2;
	}

	public unsafe override int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount, bool refresh)
	{
		int charIndex = 0;
		int byteIndex = 0;
		int num = charIndex + charCount;
		int num2 = byteIndex;
		char c = incomplete_bytes;
		while (charIndex < num)
		{
			if (incomplete_bytes == '\0')
			{
				c = *(char*)((byte*)chars + charIndex++ * 2);
			}
			else
			{
				incomplete_bytes = '\0';
			}
			if (c < '\u0080')
			{
				bytes[byteIndex++] = (byte)c;
				continue;
			}
			if (char.IsSurrogate(c))
			{
				if (charIndex == num)
				{
					incomplete_bytes = c;
					break;
				}
				char c2 = *(char*)((byte*)chars + charIndex++ * 2);
				if (!char.IsSurrogate(c2))
				{
					HandleFallback(chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
					continue;
				}
				int cp = (c - 55296) * 1024 + c2 - 56320;
				GB18030Source.Unlinear(bytes + byteIndex, GB18030Source.FromUCSSurrogate(cp));
				byteIndex += 4;
				continue;
			}
			if (c <= '\u0080' || c == 'ÿ')
			{
				bytes[byteIndex++] = (byte)c;
				continue;
			}
			byte b = gb2312.u2n[c * 2 + 1];
			byte b2 = gb2312.u2n[c * 2];
			if (b != 0 && b2 != 0)
			{
				bytes[byteIndex++] = b;
				bytes[byteIndex++] = b2;
				continue;
			}
			long num3 = GB18030Source.FromUCS(c);
			if (num3 < 0)
			{
				bytes[byteIndex++] = 63;
				continue;
			}
			GB18030Source.Unlinear(bytes + byteIndex, num3);
			byteIndex += 4;
		}
		if (refresh)
		{
			if (incomplete_bytes != 0)
			{
				bytes[byteIndex++] = 63;
			}
			incomplete_bytes = '\0';
		}
		return byteIndex - num2;
	}
}
