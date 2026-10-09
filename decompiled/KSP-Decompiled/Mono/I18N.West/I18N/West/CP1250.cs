using System;
using System.Text;
using I18N.Common;

namespace I18N.West;

[Serializable]
public class CP1250 : ByteEncoding
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
		'‚', '\u0083', '„', '…', '†', '‡', '\u0088', '‰', 'Š', '‹',
		'Ś', 'Ť', 'Ž', 'Ź', '\u0090', '‘', '’', '“', '”', '•',
		'–', '—', '\u0098', '™', 'š', '›', 'ś', 'ť', 'ž', 'ź',
		'\u00a0', 'ˇ', '\u02d8', 'Ł', '¤', 'Ą', '¦', '§', '\u00a8', '©',
		'Ş', '«', '¬', '\u00ad', '®', 'Ż', '°', '±', '\u02db', 'ł',
		'\u00b4', 'µ', '¶', '·', '\u00b8', 'ą', 'ş', '»', 'Ľ', '\u02dd',
		'ľ', 'ż', 'Ŕ', 'Á', 'Â', 'Ă', 'Ä', 'Ĺ', 'Ć', 'Ç',
		'Č', 'É', 'Ę', 'Ë', 'Ě', 'Í', 'Î', 'Ď', 'Đ', 'Ń',
		'Ň', 'Ó', 'Ô', 'Ő', 'Ö', '×', 'Ř', 'Ů', 'Ú', 'Ű',
		'Ü', 'Ý', 'Ţ', 'ß', 'ŕ', 'á', 'â', 'ă', 'ä', 'ĺ',
		'ć', 'ç', 'č', 'é', 'ę', 'ë', 'ě', 'í', 'î', 'ď',
		'đ', 'ń', 'ň', 'ó', 'ô', 'ő', 'ö', '÷', 'ř', 'ů',
		'ú', 'ű', 'ü', 'ý', 'ţ', '\u02d9'
	};

	public CP1250()
		: base(1250, ToChars, "Central European (Windows)", "iso-8859-2", "windows-1250", "windows-1250", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 1250)
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
			charCount--;
			if (num >= 128)
			{
				switch (num)
				{
				case 258:
					num = 195;
					break;
				case 259:
					num = 227;
					break;
				case 260:
					num = 165;
					break;
				case 261:
					num = 185;
					break;
				case 262:
					num = 198;
					break;
				case 263:
					num = 230;
					break;
				case 268:
					num = 200;
					break;
				case 269:
					num = 232;
					break;
				case 270:
					num = 207;
					break;
				case 271:
					num = 239;
					break;
				case 272:
					num = 208;
					break;
				case 273:
					num = 240;
					break;
				case 280:
					num = 202;
					break;
				case 281:
					num = 234;
					break;
				case 282:
					num = 204;
					break;
				case 283:
					num = 236;
					break;
				case 313:
					num = 197;
					break;
				case 314:
					num = 229;
					break;
				case 317:
					num = 188;
					break;
				case 318:
					num = 190;
					break;
				case 321:
					num = 163;
					break;
				case 322:
					num = 179;
					break;
				case 323:
					num = 209;
					break;
				case 324:
					num = 241;
					break;
				case 327:
					num = 210;
					break;
				case 328:
					num = 242;
					break;
				case 336:
					num = 213;
					break;
				case 337:
					num = 245;
					break;
				case 340:
					num = 192;
					break;
				case 341:
					num = 224;
					break;
				case 344:
					num = 216;
					break;
				case 345:
					num = 248;
					break;
				case 346:
					num = 140;
					break;
				case 347:
					num = 156;
					break;
				case 350:
					num = 170;
					break;
				case 351:
					num = 186;
					break;
				case 352:
					num = 138;
					break;
				case 353:
					num = 154;
					break;
				case 354:
					num = 222;
					break;
				case 355:
					num = 254;
					break;
				case 356:
					num = 141;
					break;
				case 357:
					num = 157;
					break;
				case 366:
					num = 217;
					break;
				case 367:
					num = 249;
					break;
				case 368:
					num = 219;
					break;
				case 369:
					num = 251;
					break;
				case 377:
					num = 143;
					break;
				case 378:
					num = 159;
					break;
				case 379:
					num = 175;
					break;
				case 380:
					num = 191;
					break;
				case 381:
					num = 142;
					break;
				case 382:
					num = 158;
					break;
				case 711:
					num = 161;
					break;
				case 728:
					num = 162;
					break;
				case 729:
					num = 255;
					break;
				case 731:
					num = 178;
					break;
				case 733:
					num = 189;
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
						break;
					}
					HandleFallback(ref buffer, chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
					continue;
				case 129:
				case 131:
				case 136:
				case 144:
				case 152:
				case 160:
				case 164:
				case 166:
				case 167:
				case 168:
				case 169:
				case 171:
				case 172:
				case 173:
				case 174:
				case 176:
				case 177:
				case 180:
				case 181:
				case 182:
				case 183:
				case 184:
				case 187:
				case 193:
				case 194:
				case 196:
				case 199:
				case 201:
				case 203:
				case 205:
				case 206:
				case 211:
				case 212:
				case 214:
				case 215:
				case 218:
				case 220:
				case 221:
				case 223:
				case 225:
				case 226:
				case 228:
				case 231:
				case 233:
				case 235:
				case 237:
				case 238:
				case 243:
				case 244:
				case 246:
				case 247:
				case 250:
				case 252:
				case 253:
					break;
				}
			}
			bytes[byteIndex++] = (byte)num;
			byteCount--;
		}
	}
}
