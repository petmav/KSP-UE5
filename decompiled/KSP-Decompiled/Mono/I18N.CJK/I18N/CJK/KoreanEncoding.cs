using System;
using System.Text;

namespace I18N.CJK;

[Serializable]
internal class KoreanEncoding : DbcsEncoding
{
	private sealed class KoreanDecoder : DbcsDecoder
	{
		private bool useUHC;

		private int last_byte_count;

		private int last_byte_conv;

		public KoreanDecoder(DbcsConvert convert, bool useUHC)
			: base(convert)
		{
			this.useUHC = useUHC;
		}

		public override int GetCharCount(byte[] bytes, int index, int count)
		{
			return GetCharCount(bytes, index, count, refresh: false);
		}

		public override int GetCharCount(byte[] bytes, int index, int count, bool refresh)
		{
			CheckRange(bytes, index, count);
			int num = last_byte_count;
			last_byte_count = 0;
			int num2 = 0;
			while (count-- > 0)
			{
				int num3 = bytes[index++];
				if (num == 0)
				{
					if (num3 <= 128 || num3 == 255)
					{
						num2++;
					}
					else
					{
						num = num3;
					}
					continue;
				}
				char c;
				if (useUHC && num < 161)
				{
					int num4 = 8836 + (num - 129) * 178;
					num4 = ((num3 >= 65 && num3 <= 90) ? (num4 + (num3 - 65)) : ((num3 >= 97 && num3 <= 122) ? (num4 + (num3 - 97 + 26)) : ((num3 < 129 || num3 > 254) ? (-1) : (num4 + (num3 - 129 + 52)))));
					c = ((num4 >= 0 && num4 * 2 <= convert.n2u.Length) ? ((char)(convert.n2u[num4 * 2] + convert.n2u[num4 * 2 + 1] * 256)) : '\0');
				}
				else if (useUHC && num <= 198 && num3 < 161)
				{
					int num5 = 14532 + (num - 161) * 84;
					num5 = ((num3 >= 65 && num3 <= 90) ? (num5 + (num3 - 65)) : ((num3 >= 97 && num3 <= 122) ? (num5 + (num3 - 97 + 26)) : ((num3 < 129 || num3 > 160) ? (-1) : (num5 + (num3 - 129 + 52)))));
					c = ((num5 >= 0 && num5 * 2 <= convert.n2u.Length) ? ((char)(convert.n2u[num5 * 2] + convert.n2u[num5 * 2 + 1] * 256)) : '\0');
				}
				else if (num3 >= 161 && num3 <= 254)
				{
					int num6 = ((num - 161) * 94 + num3 - 161) * 2;
					c = ((num6 >= 0 && num6 < convert.n2u.Length) ? ((char)(convert.n2u[num6] + convert.n2u[num6 + 1] * 256)) : '\0');
				}
				else
				{
					c = '\0';
				}
				num2 = ((c != 0) ? (num2 + 1) : (num2 + 1));
				num = 0;
			}
			if (num != 0)
			{
				if (refresh)
				{
					num2++;
					last_byte_count = 0;
				}
				else
				{
					last_byte_count = num;
				}
			}
			return num2;
		}

		public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
		{
			return GetChars(bytes, byteIndex, byteCount, chars, charIndex, refresh: false);
		}

		public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex, bool refresh)
		{
			CheckRange(bytes, byteIndex, byteCount, chars, charIndex);
			int num = charIndex;
			int num2 = last_byte_conv;
			last_byte_conv = 0;
			while (byteCount-- > 0)
			{
				int num3 = bytes[byteIndex++];
				if (num2 == 0)
				{
					if (num3 <= 128 || num3 == 255)
					{
						chars[charIndex++] = (char)num3;
					}
					else
					{
						num2 = num3;
					}
					continue;
				}
				char c;
				if (useUHC && num2 < 161)
				{
					int num4 = 8836 + (num2 - 129) * 178;
					num4 = ((num3 >= 65 && num3 <= 90) ? (num4 + (num3 - 65)) : ((num3 >= 97 && num3 <= 122) ? (num4 + (num3 - 97 + 26)) : ((num3 < 129 || num3 > 254) ? (-1) : (num4 + (num3 - 129 + 52)))));
					c = ((num4 >= 0 && num4 * 2 <= convert.n2u.Length) ? ((char)(convert.n2u[num4 * 2] + convert.n2u[num4 * 2 + 1] * 256)) : '\0');
				}
				else if (useUHC && num2 <= 198 && num3 < 161)
				{
					int num5 = 14532 + (num2 - 161) * 84;
					num5 = ((num3 >= 65 && num3 <= 90) ? (num5 + (num3 - 65)) : ((num3 >= 97 && num3 <= 122) ? (num5 + (num3 - 97 + 26)) : ((num3 < 129 || num3 > 160) ? (-1) : (num5 + (num3 - 129 + 52)))));
					c = ((num5 >= 0 && num5 * 2 <= convert.n2u.Length) ? ((char)(convert.n2u[num5 * 2] + convert.n2u[num5 * 2 + 1] * 256)) : '\0');
				}
				else if (num3 >= 161 && num3 <= 254)
				{
					int num6 = ((num2 - 161) * 94 + num3 - 161) * 2;
					c = ((num6 >= 0 && num6 < convert.n2u.Length) ? ((char)(convert.n2u[num6] + convert.n2u[num6 + 1] * 256)) : '\0');
				}
				else
				{
					c = '\0';
				}
				if (c == '\0')
				{
					chars[charIndex++] = '?';
				}
				else
				{
					chars[charIndex++] = c;
				}
				num2 = 0;
			}
			if (num2 != 0)
			{
				if (refresh)
				{
					chars[charIndex++] = '?';
					last_byte_conv = 0;
				}
				else
				{
					last_byte_conv = num2;
				}
			}
			return charIndex - num;
		}
	}

	private bool useUHC;

	public KoreanEncoding(int codepage, bool useUHC)
		: base(codepage, 949)
	{
		this.useUHC = useUHC;
	}

	internal override DbcsConvert GetConvert()
	{
		return DbcsConvert.KS;
	}

	public unsafe override int GetByteCountImpl(char* chars, int count)
	{
		int num = 0;
		int num2 = 0;
		DbcsConvert convert = GetConvert();
		while (count-- > 0)
		{
			char c = *(char*)((byte*)chars + num++ * 2);
			if (c <= '\u0080' || c == 'ÿ')
			{
				num2++;
				continue;
			}
			byte b = convert.u2n[c * 2];
			byte b2 = convert.u2n[c * 2 + 1];
			num2 = ((b != 0 || b2 != 0) ? (num2 + 2) : (num2 + 1));
		}
		return num2;
	}

	public unsafe override int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount)
	{
		int charIndex = 0;
		int byteIndex = 0;
		DbcsConvert convert = GetConvert();
		EncoderFallbackBuffer buffer = null;
		int num = byteIndex;
		while (charCount-- > 0)
		{
			char c = *(char*)((byte*)chars + charIndex++ * 2);
			if (c <= '\u0080' || c == 'ÿ')
			{
				bytes[byteIndex++] = (byte)c;
				continue;
			}
			byte b = convert.u2n[c * 2];
			byte b2 = convert.u2n[c * 2 + 1];
			if (b == 0 && b2 == 0)
			{
				HandleFallback(ref buffer, chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
				continue;
			}
			bytes[byteIndex++] = b;
			bytes[byteIndex++] = b2;
		}
		return byteIndex - num;
	}

	public override int GetCharCount(byte[] bytes, int index, int count)
	{
		return GetDecoder().GetCharCount(bytes, index, count);
	}

	public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
	{
		return GetDecoder().GetChars(bytes, byteIndex, byteCount, chars, charIndex);
	}

	public override Decoder GetDecoder()
	{
		return new KoreanDecoder(GetConvert(), useUHC);
	}
}
