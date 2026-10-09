using System.Text;

namespace I18N.CJK;

internal class ISO2022JPDecoder : Decoder
{
	private static JISConvert convert = JISConvert.Convert;

	private readonly bool allow_shift_io;

	private ISO2022JPMode m;

	private bool shifted_in_conv;

	private bool shifted_in_count;

	public ISO2022JPDecoder(bool allow1ByteKana, bool allowShiftIO)
	{
		allow_shift_io = allowShiftIO;
	}

	public override int GetCharCount(byte[] bytes, int index, int count)
	{
		int num = 0;
		int num2 = index + count;
		for (int i = index; i < num2; i++)
		{
			if (allow_shift_io)
			{
				switch (bytes[i])
				{
				case 15:
					shifted_in_count = false;
					continue;
				case 14:
					shifted_in_count = true;
					continue;
				}
			}
			if (bytes[i] != 27)
			{
				if (!shifted_in_count && m == ISO2022JPMode.JISX0208)
				{
					if (i + 1 == num2)
					{
						break;
					}
					num++;
					i++;
				}
				else
				{
					num++;
				}
				continue;
			}
			if (i + 2 >= num2)
			{
				break;
			}
			i++;
			bool flag = false;
			if (bytes[i] == 36)
			{
				flag = true;
			}
			else
			{
				if (bytes[i] != 40)
				{
					num += 2;
					continue;
				}
				flag = false;
			}
			i++;
			if (bytes[i] == 66)
			{
				m = (flag ? ISO2022JPMode.JISX0208 : ISO2022JPMode.ASCII);
			}
			else if (bytes[i] == 74)
			{
				m = ISO2022JPMode.ASCII;
			}
			else if (bytes[i] == 73)
			{
				m = ISO2022JPMode.JISX0201;
			}
			else
			{
				num += 3;
			}
		}
		return num;
	}

	private int ToChar(int value)
	{
		value <<= 1;
		return (value + 1 < convert.jisx0208ToUnicode.Length && value >= 0) ? (convert.jisx0208ToUnicode[value] | (convert.jisx0208ToUnicode[value + 1] << 8)) : (-1);
	}

	public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
	{
		int num = charIndex;
		int num2 = byteIndex + byteCount;
		for (int i = byteIndex; i < num2 && charIndex < chars.Length; i++)
		{
			if (allow_shift_io)
			{
				switch (bytes[i])
				{
				case 15:
					shifted_in_conv = false;
					continue;
				case 14:
					shifted_in_conv = true;
					continue;
				}
			}
			if (bytes[i] != 27)
			{
				if (shifted_in_conv || m == ISO2022JPMode.JISX0201)
				{
					if (bytes[i] < 96)
					{
						chars[charIndex++] = (char)(bytes[i] + 65344);
					}
					else
					{
						chars[charIndex++] = '?';
					}
				}
				else if (m == ISO2022JPMode.JISX0208)
				{
					if (i + 1 == num2)
					{
						break;
					}
					int num3 = (bytes[i] - 1 >> 1) + ((bytes[i] > 94) ? 177 : 113);
					int num4 = bytes[i + 1] + (((bytes[i] & 1) == 0) ? 126 : 32);
					int num5 = (num3 - 129) * 188;
					num5 += num4 - 65;
					int num6 = ToChar(num5);
					if (num6 < 0)
					{
						chars[charIndex++] = '?';
					}
					else
					{
						chars[charIndex++] = (char)num6;
					}
					i++;
				}
				else if (bytes[i] > 160 && bytes[i] < 224)
				{
					chars[charIndex++] = (char)(bytes[i] - 160 + 65376);
				}
				else
				{
					chars[charIndex++] = (char)bytes[i];
				}
				continue;
			}
			if (i + 2 >= num2)
			{
				break;
			}
			i++;
			bool flag = false;
			if (bytes[i] == 36)
			{
				flag = true;
			}
			else
			{
				if (bytes[i] != 40)
				{
					chars[charIndex++] = '\u001b';
					chars[charIndex++] = (char)bytes[i];
					continue;
				}
				flag = false;
			}
			i++;
			if (bytes[i] == 66)
			{
				m = (flag ? ISO2022JPMode.JISX0208 : ISO2022JPMode.ASCII);
				continue;
			}
			if (bytes[i] == 74)
			{
				m = ISO2022JPMode.ASCII;
				continue;
			}
			if (bytes[i] == 73)
			{
				m = ISO2022JPMode.JISX0201;
				continue;
			}
			chars[charIndex++] = '\u001b';
			chars[charIndex++] = (char)bytes[i - 1];
			chars[charIndex++] = (char)bytes[i];
		}
		return charIndex - num;
	}

	public override void Reset()
	{
		m = ISO2022JPMode.ASCII;
		shifted_in_count = (shifted_in_conv = false);
	}
}
