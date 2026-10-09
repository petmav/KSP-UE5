using System;
using System.Text;
using I18N.Common;

namespace I18N.MidEast;

[Serializable]
public class CP1255 : ByteEncoding
{
	private static readonly char[] ToChars = new char[256]
	{
		'\0', '\u0001', '\u0002', '\u0003', '\u0004', '\u0005', '\u0006', '\a', '\b', '\t',
		'\n', '\v', '\f', '\r', '\u000e', '\u000f', '\u0010', '\u0011', '\u0012', '\u0013',
		'\u0014', '\u0015', '\u0016', '\u0017', '\u0018', '\u0019', '\u001a', '\u001b', '\u001c', '\u001d',
		'\u001e', '\u001f', ' ', '!', '"', '#', '$', '%', '&', '\'',
		'(', ')', '*', '+', ',', '-', '.', '/', '0', '1',
		'2', '3', '4', '5', '6', '7', '8', '9', ':', ';',
		'<', '=', '>', '?', '@', 'A', 'B', 'C', 'D', 'E',
		'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
		'Z', '[', '\\', ']', '^', '_', '`', 'a', 'b', 'c',
		'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
		'x', 'y', 'z', '{', '|', '}', '~', '\u007f', '€', '\u0081',
		'‚', 'ƒ', '„', '…', '†', '‡', 'ˆ', '‰', '\u008a', '‹',
		'\u008c', '\u008d', '\u008e', '\u008f', '\u0090', '‘', '’', '“', '”', '•',
		'–', '—', '\u02dc', '™', '\u009a', '›', '\u009c', '\u009d', '\u009e', '\u009f',
		'\u00a0', '¡', '¢', '£', '₪', '¥', '¦', '§', '\u00a8', '©',
		'×', '«', '¬', '\u00ad', '®', '\u00af', '°', '±', '²', '³',
		'\u00b4', 'µ', '¶', '·', '\u00b8', '¹', '÷', '»', '¼', '½',
		'¾', '¿', '\u05b0', '\u05b1', '\u05b2', '\u05b3', '\u05b4', '\u05b5', '\u05b6', '\u05b7',
		'\u05b8', '\u05b9', '?', '\u05bb', '\u05bc', '\u05bd', '־', '\u05bf', '׀', '\u05c1',
		'\u05c2', '׃', 'װ', 'ױ', 'ײ', '׳', '״', '?', '?', '?',
		'?', '?', '?', '?', 'א', 'ב', 'ג', 'ד', 'ה', 'ו',
		'ז', 'ח', 'ט', 'י', 'ך', 'כ', 'ל', 'ם', 'מ', 'ן',
		'נ', 'ס', 'ע', 'ף', 'פ', 'ץ', 'צ', 'ק', 'ר', 'ש',
		'ת', '?', '?', '\u200e', '\u200f', '?'
	};

	public CP1255()
		: base(1255, ToChars, "Hebrew (Windows)", "windows-1255", "windows-1255", "windows-1255", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 1255)
	{
	}

	protected unsafe override void ToBytes(char* chars, int charCount, byte* bytes, int byteCount)
	{
		int charIndex = 0;
		int byteIndex = 0;
		EncoderFallbackBuffer buffer = null;
		while (charCount > 0)
		{
			int num = *(ushort*)((byte*)chars + charIndex++ * 2);
			if (num >= 128)
			{
				switch (num)
				{
				case 215:
					num = 170;
					break;
				case 247:
					num = 186;
					break;
				case 402:
					num = 131;
					break;
				case 710:
					num = 136;
					break;
				case 732:
					num = 152;
					break;
				case 1456:
				case 1457:
				case 1458:
				case 1459:
				case 1460:
				case 1461:
				case 1462:
				case 1463:
				case 1464:
				case 1465:
					num -= 1264;
					break;
				case 1467:
				case 1468:
				case 1469:
				case 1470:
				case 1471:
				case 1472:
				case 1473:
				case 1474:
				case 1475:
					num -= 1264;
					break;
				case 1488:
				case 1489:
				case 1490:
				case 1491:
				case 1492:
				case 1493:
				case 1494:
				case 1495:
				case 1496:
				case 1497:
				case 1498:
				case 1499:
				case 1500:
				case 1501:
				case 1502:
				case 1503:
				case 1504:
				case 1505:
				case 1506:
				case 1507:
				case 1508:
				case 1509:
				case 1510:
				case 1511:
				case 1512:
				case 1513:
				case 1514:
					num -= 1264;
					break;
				case 1520:
				case 1521:
				case 1522:
				case 1523:
				case 1524:
					num -= 1308;
					break;
				case 8206:
					num = 253;
					break;
				case 8207:
					num = 254;
					break;
				case 8211:
					num = 150;
					break;
				case 8212:
					num = 151;
					break;
				case 8216:
					num = 145;
					break;
				case 8217:
					num = 146;
					break;
				case 8218:
					num = 130;
					break;
				case 8220:
					num = 147;
					break;
				case 8221:
					num = 148;
					break;
				case 8222:
					num = 132;
					break;
				case 8224:
					num = 134;
					break;
				case 8225:
					num = 135;
					break;
				case 8226:
					num = 149;
					break;
				case 8230:
					num = 133;
					break;
				case 8240:
					num = 137;
					break;
				case 8249:
					num = 139;
					break;
				case 8250:
					num = 155;
					break;
				case 8362:
					num = 164;
					break;
				case 8364:
					num = 128;
					break;
				case 8482:
					num = 153;
					break;
				default:
					if (num >= 65281 && num <= 65374)
					{
						num -= 65248;
					}
					else
					{
						HandleFallback(ref buffer, chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
					}
					break;
				case 129:
				case 138:
				case 140:
				case 141:
				case 142:
				case 143:
				case 144:
				case 154:
				case 156:
				case 157:
				case 158:
				case 159:
				case 160:
				case 161:
				case 162:
				case 163:
				case 165:
				case 166:
				case 167:
				case 168:
				case 169:
				case 171:
				case 172:
				case 173:
				case 174:
				case 175:
				case 176:
				case 177:
				case 178:
				case 179:
				case 180:
				case 181:
				case 182:
				case 183:
				case 184:
				case 185:
				case 187:
				case 188:
				case 189:
				case 190:
				case 191:
					break;
				}
			}
			bytes[byteIndex++] = (byte)num;
			charCount--;
			byteCount--;
		}
	}
}
