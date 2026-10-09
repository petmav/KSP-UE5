namespace I18N.CJK;

internal class GB18030Decoder : DbcsEncoding.DbcsDecoder
{
	private static DbcsConvert gb2312 = DbcsConvert.Gb2312;

	public GB18030Decoder()
		: base(null)
	{
	}

	public override int GetCharCount(byte[] bytes, int start, int len)
	{
		CheckRange(bytes, start, len);
		int num = start + len;
		int num2 = 0;
		while (start < num)
		{
			if (bytes[start] < 128)
			{
				num2++;
				start++;
				continue;
			}
			if (bytes[start] == 128)
			{
				num2++;
				start++;
				continue;
			}
			if (bytes[start] == byte.MaxValue)
			{
				num2++;
				start++;
				continue;
			}
			if (start + 1 >= num)
			{
				num2++;
				break;
			}
			byte b = bytes[start + 1];
			if (b == 127 || b == byte.MaxValue)
			{
				num2++;
				start += 2;
			}
			else if (48 <= b && b <= 57)
			{
				if (start + 3 >= num)
				{
					num2 += ((start + 3 != num) ? 2 : 3);
					break;
				}
				long num3 = GB18030Source.FromGBX(bytes, start);
				if (num3 < 0)
				{
					num2++;
					start -= (int)num3;
				}
				else if (num3 >= 65536)
				{
					num2 += 2;
					start += 4;
				}
				else
				{
					num2++;
					start += 4;
				}
			}
			else
			{
				start += 2;
				num2++;
			}
		}
		return num2;
	}

	public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
	{
		CheckRange(bytes, byteIndex, byteCount, chars, charIndex);
		int num = byteIndex + byteCount;
		int num2 = charIndex;
		while (byteIndex < num)
		{
			if (bytes[byteIndex] < 128)
			{
				chars[charIndex++] = (char)bytes[byteIndex++];
				continue;
			}
			if (bytes[byteIndex] == 128)
			{
				chars[charIndex++] = '€';
				byteIndex++;
				continue;
			}
			if (bytes[byteIndex] == byte.MaxValue)
			{
				chars[charIndex++] = '?';
				byteIndex++;
				continue;
			}
			if (byteIndex + 1 >= num)
			{
				break;
			}
			byte b = bytes[byteIndex + 1];
			if (b == 127 || b == byte.MaxValue)
			{
				chars[charIndex++] = '?';
				byteIndex += 2;
			}
			else if (48 <= b && b <= 57)
			{
				if (byteIndex + 3 >= num)
				{
					break;
				}
				long num3 = GB18030Source.FromGBX(bytes, byteIndex);
				if (num3 < 0)
				{
					chars[charIndex++] = '?';
					byteIndex -= (int)num3;
				}
				else if (num3 >= 65536)
				{
					num3 -= 65536;
					chars[charIndex++] = (char)(num3 / 1024 + 55296);
					chars[charIndex++] = (char)(num3 % 1024 + 56320);
					byteIndex += 4;
				}
				else
				{
					chars[charIndex++] = (char)num3;
					byteIndex += 4;
				}
			}
			else
			{
				byte b2 = bytes[byteIndex];
				int num4 = ((b2 - 129) * 191 + b - 64) * 2;
				char c = ((num4 >= 0 && num4 < gb2312.n2u.Length) ? ((char)(gb2312.n2u[num4] + gb2312.n2u[num4 + 1] * 256)) : '\0');
				if (c == '\0')
				{
					chars[charIndex++] = '?';
				}
				else
				{
					chars[charIndex++] = c;
				}
				byteIndex += 2;
			}
		}
		return charIndex - num2;
	}
}
