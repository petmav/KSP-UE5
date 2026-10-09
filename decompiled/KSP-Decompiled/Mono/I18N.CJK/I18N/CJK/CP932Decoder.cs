using System;
using I18N.Common;

namespace I18N.CJK;

internal sealed class CP932Decoder : DbcsEncoding.DbcsDecoder
{
	private new JISConvert convert;

	private int last_byte_count;

	private int last_byte_chars;

	public CP932Decoder(JISConvert convert)
		: base(null)
	{
		this.convert = convert;
	}

	public override int GetCharCount(byte[] bytes, int index, int count)
	{
		return GetCharCount(bytes, index, count, refresh: false);
	}

	public override int GetCharCount(byte[] bytes, int index, int count, bool refresh)
	{
		CheckRange(bytes, index, count);
		int num = 0;
		int num2 = last_byte_count;
		while (count > 0)
		{
			int num3 = bytes[index++];
			count--;
			if (num2 == 0)
			{
				if ((num3 >= 129 && num3 <= 159) || (num3 >= 224 && num3 <= 239))
				{
					num2 = num3;
				}
				num++;
			}
			else
			{
				num2 = 0;
			}
		}
		if (refresh)
		{
			if (num2 != 0)
			{
				num++;
			}
			last_byte_count = 0;
		}
		else
		{
			last_byte_count = num2;
		}
		return num;
	}

	public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
	{
		return GetChars(bytes, byteIndex, byteCount, chars, charIndex, refresh: false);
	}

	public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex, bool refresh)
	{
		CheckRange(bytes, byteIndex, byteCount, chars, charIndex);
		int num = charIndex;
		int num2 = chars.Length;
		int num3 = last_byte_chars;
		byte[] jisx0208ToUnicode = convert.jisx0208ToUnicode;
		while (byteCount > 0)
		{
			int num4 = bytes[byteIndex++];
			byteCount--;
			int num5;
			switch (num3)
			{
			case 0:
				if (num >= num2)
				{
					throw new ArgumentException(Strings.GetString("Arg_InsufficientSpace"), "chars");
				}
				if ((num4 >= 129 && num4 <= 159) || (num4 >= 224 && num4 <= 239))
				{
					num3 = num4;
				}
				else if (num4 < 128)
				{
					chars[num++] = (char)num4;
				}
				else if (num4 >= 161 && num4 <= 223)
				{
					chars[num++] = (char)(num4 - 161 + 65377);
				}
				else
				{
					chars[num++] = '?';
				}
				continue;
			case 129:
			case 130:
			case 131:
			case 132:
			case 133:
			case 134:
			case 135:
			case 136:
			case 137:
			case 138:
			case 139:
			case 140:
			case 141:
			case 142:
			case 143:
			case 144:
			case 145:
			case 146:
			case 147:
			case 148:
			case 149:
			case 150:
			case 151:
			case 152:
			case 153:
			case 154:
			case 155:
			case 156:
			case 157:
			case 158:
			case 159:
				num5 = (num3 - 129) * 188;
				break;
			default:
				if (num3 >= 240 && num3 <= 252 && num4 <= 252)
				{
					num5 = 57344 + (num3 - 240) * 188 + num4;
					if (num4 > 127)
					{
						num5--;
					}
				}
				else
				{
					num5 = (num3 - 224 + 31) * 188;
				}
				break;
			}
			num3 = 0;
			if (num4 >= 64 && num4 <= 126)
			{
				num5 += num4 - 64;
			}
			else
			{
				if (num4 < 128 || num4 > 252)
				{
					chars[num++] = '?';
					continue;
				}
				num5 += num4 - 128 + 63;
			}
			num5 *= 2;
			num5 = jisx0208ToUnicode[num5] | (jisx0208ToUnicode[num5 + 1] << 8);
			if (num5 != 0)
			{
				chars[num++] = (char)num5;
			}
			else
			{
				chars[num++] = '?';
			}
		}
		if (refresh)
		{
			if (num3 != 0)
			{
				chars[num++] = '・';
			}
			last_byte_chars = 0;
		}
		else
		{
			last_byte_chars = num3;
		}
		return num - charIndex;
	}
}
