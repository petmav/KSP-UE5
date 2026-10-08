using System;
using System.Text;
using I18N.Common;

namespace I18N.Other;

[Serializable]
public class CP1257 : ByteEncoding
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
		'‚', '\u0083', '„', '…', '†', '‡', '\u0088', '‰', '\u008a', '‹',
		'\u008c', '\u00a8', 'ˇ', '\u00b8', '\u0090', '‘', '’', '“', '”', '•',
		'–', '—', '\u0098', '™', '\u009a', '›', '\u009c', '\u00af', '\u02db', '\u009f',
		'\u00a0', '?', '¢', '£', '¤', '?', '¦', '§', 'Ø', '©',
		'Ŗ', '«', '¬', '\u00ad', '®', 'Æ', '°', '±', '²', '³',
		'\u00b4', 'µ', '¶', '·', 'ø', '¹', 'ŗ', '»', '¼', '½',
		'¾', 'æ', 'Ą', 'Į', 'Ā', 'Ć', 'Ä', 'Å', 'Ę', 'Ē',
		'Č', 'É', 'Ź', 'Ė', 'Ģ', 'Ķ', 'Ī', 'Ļ', 'Š', 'Ń',
		'Ņ', 'Ó', 'Ō', 'Õ', 'Ö', '×', 'Ų', 'Ł', 'Ś', 'Ū',
		'Ü', 'Ż', 'Ž', 'ß', 'ą', 'į', 'ā', 'ć', 'ä', 'å',
		'ę', 'ē', 'č', 'é', 'ź', 'ė', 'ģ', 'ķ', 'ī', 'ļ',
		'š', 'ń', 'ņ', 'ó', 'ō', 'õ', 'ö', '÷', 'ų', 'ł',
		'ś', 'ū', 'ü', 'ż', 'ž', '\u02d9'
	};

	public CP1257()
		: base(1257, ToChars, "Baltic (Windows)", "iso-8859-4", "windows-1257", "windows-1257", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 1257)
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
				case 168:
					num = 141;
					break;
				case 175:
					num = 157;
					break;
				case 184:
					num = 143;
					break;
				case 198:
					num = 175;
					break;
				case 216:
					num = 168;
					break;
				case 230:
					num = 191;
					break;
				case 248:
					num = 184;
					break;
				case 256:
					num = 194;
					break;
				case 257:
					num = 226;
					break;
				case 260:
					num = 192;
					break;
				case 261:
					num = 224;
					break;
				case 262:
					num = 195;
					break;
				case 263:
					num = 227;
					break;
				case 268:
					num = 200;
					break;
				case 269:
					num = 232;
					break;
				case 274:
					num = 199;
					break;
				case 275:
					num = 231;
					break;
				case 278:
					num = 203;
					break;
				case 279:
					num = 235;
					break;
				case 280:
					num = 198;
					break;
				case 281:
					num = 230;
					break;
				case 290:
					num = 204;
					break;
				case 291:
					num = 236;
					break;
				case 298:
					num = 206;
					break;
				case 299:
					num = 238;
					break;
				case 302:
					num = 193;
					break;
				case 303:
					num = 225;
					break;
				case 310:
					num = 205;
					break;
				case 311:
					num = 237;
					break;
				case 315:
					num = 207;
					break;
				case 316:
					num = 239;
					break;
				case 321:
					num = 217;
					break;
				case 322:
					num = 249;
					break;
				case 323:
					num = 209;
					break;
				case 324:
					num = 241;
					break;
				case 325:
					num = 210;
					break;
				case 326:
					num = 242;
					break;
				case 332:
					num = 212;
					break;
				case 333:
					num = 244;
					break;
				case 342:
					num = 170;
					break;
				case 343:
					num = 186;
					break;
				case 346:
					num = 218;
					break;
				case 347:
					num = 250;
					break;
				case 352:
					num = 208;
					break;
				case 353:
					num = 240;
					break;
				case 362:
					num = 219;
					break;
				case 363:
					num = 251;
					break;
				case 370:
					num = 216;
					break;
				case 371:
					num = 248;
					break;
				case 377:
					num = 202;
					break;
				case 378:
					num = 234;
					break;
				case 379:
					num = 221;
					break;
				case 380:
					num = 253;
					break;
				case 381:
					num = 222;
					break;
				case 382:
					num = 254;
					break;
				case 711:
					num = 142;
					break;
				case 729:
					num = 255;
					break;
				case 731:
					num = 158;
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
					}
					else
					{
						HandleFallback(ref buffer, chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
					}
					break;
				case 129:
				case 131:
				case 136:
				case 138:
				case 140:
				case 144:
				case 152:
				case 154:
				case 156:
				case 159:
				case 160:
				case 162:
				case 163:
				case 164:
				case 166:
				case 167:
				case 169:
				case 171:
				case 172:
				case 173:
				case 174:
				case 176:
				case 177:
				case 178:
				case 179:
				case 180:
				case 181:
				case 182:
				case 183:
				case 185:
				case 187:
				case 188:
				case 189:
				case 190:
				case 196:
				case 197:
				case 201:
				case 211:
				case 213:
				case 214:
				case 215:
				case 220:
				case 223:
				case 228:
				case 229:
				case 233:
				case 243:
				case 245:
				case 246:
				case 247:
				case 252:
					break;
				}
			}
			bytes[byteIndex++] = (byte)num;
			charCount--;
			byteCount--;
		}
	}
}
