using System;
using I18N.Common;

namespace I18N.CJK;

internal class ISO2022JPEncoder : MonoEncoder
{
	private static JISConvert convert = JISConvert.Convert;

	private readonly bool allow_1byte_kana;

	private readonly bool allow_shift_io;

	private ISO2022JPMode m;

	private bool shifted_in_count;

	private bool shifted_in_conv;

	private static readonly char[] full_width_map = new char[65]
	{
		'\0', '。', '「', '」', '、', '・', 'ヲ', 'ァ', 'ィ', 'ゥ',
		'ェ', 'ォ', 'ャ', 'ュ', 'ョ', 'ッ', 'ー', 'ア', 'イ', 'ウ',
		'エ', 'オ', 'カ', 'キ', 'ク', 'ケ', 'コ', 'サ', 'シ', 'ス',
		'セ', 'ソ', 'タ', 'チ', 'ツ', 'テ', 'ト', 'ド', 'ナ', 'ニ',
		'ヌ', 'ネ', 'ハ', 'ヒ', 'フ', 'ヘ', 'ホ', 'マ', 'ミ', 'ム',
		'メ', 'モ', 'ヤ', 'ユ', 'ヨ', 'ラ', 'リ', 'ル', 'レ', 'ロ',
		'ワ', 'ヱ', 'ン', '\u309b', '\u309c'
	};

	public ISO2022JPEncoder(MonoEncoding owner, bool allow1ByteKana, bool allowShiftIO)
		: base(owner)
	{
		allow_1byte_kana = allow1ByteKana;
		allow_shift_io = allowShiftIO;
	}

	public unsafe override int GetByteCountImpl(char* chars, int charCount, bool flush)
	{
		int num = 0;
		int num2 = 0;
		for (int i = num; i < charCount; i++)
		{
			char c = *(char*)((byte*)chars + i * 2);
			if (!allow_1byte_kana && c >= '｠' && c <= 'ﾠ')
			{
				c = full_width_map[c - 65376];
			}
			int num3;
			if (c >= '‐' && c <= '龥')
			{
				if (shifted_in_count)
				{
					shifted_in_count = false;
					num2++;
				}
				if (m != ISO2022JPMode.JISX0208)
				{
					num2 += 3;
				}
				m = ISO2022JPMode.JISX0208;
				num3 = (c - 8208) * 2;
				num3 = convert.cjkToJis[num3] | (convert.cjkToJis[num3 + 1] << 8);
			}
			else if (c >= '！' && c <= '｠')
			{
				if (shifted_in_count)
				{
					shifted_in_count = false;
					num2++;
				}
				if (m != ISO2022JPMode.JISX0208)
				{
					num2 += 3;
				}
				m = ISO2022JPMode.JISX0208;
				num3 = (c - 65281) * 2;
				num3 = convert.extraToJis[num3] | (convert.extraToJis[num3 + 1] << 8);
			}
			else if (c >= '｠' && c <= 'ﾠ')
			{
				if (allow_shift_io)
				{
					if (!shifted_in_count)
					{
						num2++;
						shifted_in_count = true;
					}
				}
				else if (m != ISO2022JPMode.JISX0201)
				{
					num2 += 3;
					m = ISO2022JPMode.JISX0201;
				}
				num3 = c - 65376 + 160;
			}
			else
			{
				if (c >= '\u0080')
				{
					continue;
				}
				if (shifted_in_count)
				{
					shifted_in_count = false;
					num2++;
				}
				if (m != 0)
				{
					num2 += 3;
				}
				m = ISO2022JPMode.ASCII;
				num3 = c;
			}
			num2 = ((num3 <= 256) ? (num2 + 1) : (num2 + 2));
		}
		if (flush)
		{
			if (shifted_in_count)
			{
				shifted_in_count = false;
				num2++;
			}
			if (m != 0)
			{
				num2 += 3;
			}
			m = ISO2022JPMode.ASCII;
		}
		return num2;
	}

	private unsafe void SwitchMode(byte* bytes, ref int byteIndex, ref int byteCount, ref ISO2022JPMode cur, ISO2022JPMode next)
	{
		if (cur != next)
		{
			if (byteCount <= 3)
			{
				throw new ArgumentOutOfRangeException("Insufficient byte buffer.");
			}
			bytes[byteIndex++] = 27;
			switch (next)
			{
			case ISO2022JPMode.JISX0201:
				bytes[byteIndex++] = 40;
				bytes[byteIndex++] = 73;
				break;
			case ISO2022JPMode.JISX0208:
				bytes[byteIndex++] = 36;
				bytes[byteIndex++] = 66;
				break;
			default:
				bytes[byteIndex++] = 40;
				bytes[byteIndex++] = 66;
				break;
			}
			cur = next;
		}
	}

	public unsafe override int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount, bool flush)
	{
		int num = 0;
		int byteIndex = 0;
		int num2 = byteIndex;
		int num3 = num + charCount;
		for (int i = num; i < num3; i++, charCount--)
		{
			char c = *(char*)((byte*)chars + i * 2);
			if (!allow_1byte_kana && c >= '｠' && c <= 'ﾠ')
			{
				c = full_width_map[c - 65376];
			}
			int num4;
			if (c >= '‐' && c <= '龥')
			{
				if (shifted_in_conv)
				{
					bytes[byteIndex++] = 15;
					shifted_in_conv = false;
					byteCount--;
				}
				ISO2022JPMode iSO2022JPMode = m;
				if (iSO2022JPMode != ISO2022JPMode.JISX0208)
				{
					SwitchMode(bytes, ref byteIndex, ref byteCount, ref m, ISO2022JPMode.JISX0208);
				}
				num4 = (c - 8208) * 2;
				num4 = convert.cjkToJis[num4] | (convert.cjkToJis[num4 + 1] << 8);
			}
			else if (c >= '！' && c <= '｠')
			{
				if (shifted_in_conv)
				{
					bytes[byteIndex++] = 15;
					shifted_in_conv = false;
					byteCount--;
				}
				ISO2022JPMode iSO2022JPMode = m;
				if (iSO2022JPMode != ISO2022JPMode.JISX0208)
				{
					SwitchMode(bytes, ref byteIndex, ref byteCount, ref m, ISO2022JPMode.JISX0208);
				}
				num4 = (c - 65281) * 2;
				num4 = convert.extraToJis[num4] | (convert.extraToJis[num4 + 1] << 8);
			}
			else if (c >= '｠' && c <= 'ﾠ')
			{
				if (allow_shift_io)
				{
					if (!shifted_in_conv)
					{
						bytes[byteIndex++] = 14;
						shifted_in_conv = true;
						byteCount--;
					}
				}
				else
				{
					ISO2022JPMode iSO2022JPMode = m;
					if (iSO2022JPMode != ISO2022JPMode.JISX0201)
					{
						SwitchMode(bytes, ref byteIndex, ref byteCount, ref m, ISO2022JPMode.JISX0201);
					}
				}
				num4 = c - 65344;
			}
			else
			{
				if (c >= '\u0080')
				{
					HandleFallback(chars, ref i, ref charCount, bytes, ref byteIndex, ref byteCount);
					continue;
				}
				if (shifted_in_conv)
				{
					bytes[byteIndex++] = 15;
					shifted_in_conv = false;
					byteCount--;
				}
				SwitchMode(bytes, ref byteIndex, ref byteCount, ref m, ISO2022JPMode.ASCII);
				num4 = c;
			}
			if (num4 > 256)
			{
				num4 -= 256;
				bytes[byteIndex++] = (byte)(num4 / 94 + 33);
				bytes[byteIndex++] = (byte)(num4 % 94 + 33);
				byteCount -= 2;
			}
			else
			{
				bytes[byteIndex++] = (byte)num4;
				byteCount--;
			}
		}
		if (flush)
		{
			if (shifted_in_conv)
			{
				bytes[byteIndex++] = 15;
				shifted_in_conv = false;
				byteCount--;
			}
			if (m != 0)
			{
				SwitchMode(bytes, ref byteIndex, ref byteCount, ref m, ISO2022JPMode.ASCII);
			}
		}
		return byteIndex - num2;
	}

	public override void Reset()
	{
		m = ISO2022JPMode.ASCII;
		shifted_in_conv = (shifted_in_count = false);
	}
}
