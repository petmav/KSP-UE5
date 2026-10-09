using System;
using System.Text;
using I18N.Common;

namespace I18N.Other;

[Serializable]
public class CP28594 : ByteEncoding
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
		'x', 'y', 'z', '{', '|', '}', '~', '\u007f', '\u0080', '\u0081',
		'\u0082', '\u0083', '\u0084', '\u0085', '\u0086', '\u0087', '\u0088', '\u0089', '\u008a', '\u008b',
		'\u008c', '\u008d', '\u008e', '\u008f', '\u0090', '\u0091', '\u0092', '\u0093', '\u0094', '\u0095',
		'\u0096', '\u0097', '\u0098', '\u0099', '\u009a', '\u009b', '\u009c', '\u009d', '\u009e', '\u009f',
		'\u00a0', 'Ą', 'ĸ', 'Ŗ', '¤', 'Ĩ', 'Ļ', '§', '\u00a8', 'Š',
		'Ē', 'Ģ', 'Ŧ', '\u00ad', 'Ž', '\u00af', '°', 'ą', '\u02db', 'ŗ',
		'\u00b4', 'ĩ', 'ļ', 'ˇ', '\u00b8', 'š', 'ē', 'ģ', 'ŧ', 'Ŋ',
		'ž', 'ŋ', 'Ā', 'Á', 'Â', 'Ã', 'Ä', 'Å', 'Æ', 'Į',
		'Č', 'É', 'Ę', 'Ë', 'Ė', 'Í', 'Î', 'Ī', 'Đ', 'Ņ',
		'Ō', 'Ķ', 'Ô', 'Õ', 'Ö', '×', 'Ø', 'Ų', 'Ú', 'Û',
		'Ü', 'Ũ', 'Ū', 'ß', 'ā', 'á', 'â', 'ã', 'ä', 'å',
		'æ', 'į', 'č', 'é', 'ę', 'ë', 'ė', 'í', 'î', 'ī',
		'đ', 'ņ', 'ō', 'ķ', 'ô', 'õ', 'ö', '÷', 'ø', 'ų',
		'ú', 'û', 'ü', 'ũ', 'ū', '\u02d9'
	};

	public CP28594()
		: base(28594, ToChars, "Baltic (ISO)", "iso-8859-4", "iso-8859-4", "iso-8859-4", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 1257)
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
			if (num >= 161)
			{
				switch (num)
				{
				case 256:
					num = 192;
					break;
				case 257:
					num = 224;
					break;
				case 260:
					num = 161;
					break;
				case 261:
					num = 177;
					break;
				case 268:
					num = 200;
					break;
				case 269:
					num = 232;
					break;
				case 272:
					num = 208;
					break;
				case 273:
					num = 240;
					break;
				case 274:
					num = 170;
					break;
				case 275:
					num = 186;
					break;
				case 278:
					num = 204;
					break;
				case 279:
					num = 236;
					break;
				case 280:
					num = 202;
					break;
				case 281:
					num = 234;
					break;
				case 290:
					num = 171;
					break;
				case 291:
					num = 187;
					break;
				case 296:
					num = 165;
					break;
				case 297:
					num = 181;
					break;
				case 298:
					num = 207;
					break;
				case 299:
					num = 239;
					break;
				case 302:
					num = 199;
					break;
				case 303:
					num = 231;
					break;
				case 310:
					num = 211;
					break;
				case 311:
					num = 243;
					break;
				case 312:
					num = 162;
					break;
				case 315:
					num = 166;
					break;
				case 316:
					num = 182;
					break;
				case 325:
					num = 209;
					break;
				case 326:
					num = 241;
					break;
				case 330:
					num = 189;
					break;
				case 331:
					num = 191;
					break;
				case 332:
					num = 210;
					break;
				case 333:
					num = 242;
					break;
				case 342:
					num = 163;
					break;
				case 343:
					num = 179;
					break;
				case 352:
					num = 169;
					break;
				case 353:
					num = 185;
					break;
				case 358:
					num = 172;
					break;
				case 359:
					num = 188;
					break;
				case 360:
					num = 221;
					break;
				case 361:
					num = 253;
					break;
				case 362:
					num = 222;
					break;
				case 363:
					num = 254;
					break;
				case 370:
					num = 217;
					break;
				case 371:
					num = 249;
					break;
				case 381:
					num = 174;
					break;
				case 382:
					num = 190;
					break;
				case 711:
					num = 183;
					break;
				case 729:
					num = 255;
					break;
				case 731:
					num = 178;
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
				case 164:
				case 167:
				case 168:
				case 173:
				case 175:
				case 176:
				case 180:
				case 184:
				case 193:
				case 194:
				case 195:
				case 196:
				case 197:
				case 198:
				case 201:
				case 203:
				case 205:
				case 206:
				case 212:
				case 213:
				case 214:
				case 215:
				case 216:
				case 218:
				case 219:
				case 220:
				case 223:
				case 225:
				case 226:
				case 227:
				case 228:
				case 229:
				case 230:
				case 233:
				case 235:
				case 237:
				case 238:
				case 244:
				case 245:
				case 246:
				case 247:
				case 248:
				case 250:
				case 251:
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
