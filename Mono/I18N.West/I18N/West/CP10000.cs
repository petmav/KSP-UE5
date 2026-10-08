using System;
using System.Text;
using I18N.Common;

namespace I18N.West;

[Serializable]
public class CP10000 : ByteEncoding
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
		'x', 'y', 'z', '{', '|', '}', '~', '\u007f', 'Ä', 'Å',
		'Ç', 'É', 'Ñ', 'Ö', 'Ü', 'á', 'à', 'â', 'ä', 'ã',
		'å', 'ç', 'é', 'è', 'ê', 'ë', 'í', 'ì', 'î', 'ï',
		'ñ', 'ó', 'ò', 'ô', 'ö', 'õ', 'ú', 'ù', 'û', 'ü',
		'†', '°', '¢', '£', '§', '•', '¶', 'ß', '®', '©',
		'™', '\u00b4', '\u00a8', '≠', 'Æ', 'Ø', '∞', '±', '≤', '≥',
		'¥', 'µ', '∂', '∑', '∏', 'π', '∫', 'ª', 'º', 'Ω',
		'æ', 'ø', '¿', '¡', '¬', '√', 'ƒ', '≈', '∆', '«',
		'»', '…', '\u00a0', 'À', 'Ã', 'Õ', 'Œ', 'œ', '–', '—',
		'“', '”', '‘', '’', '÷', '◊', 'ÿ', 'Ÿ', '⁄', '¤',
		'‹', '›', 'ﬁ', 'ﬂ', '‡', '·', '‚', '„', '‰', 'Â',
		'Ê', 'Á', 'Ë', 'È', 'Í', 'Î', 'Ï', 'Ì', 'Ó', 'Ô',
		'\uf8ff', 'Ò', 'Ú', 'Û', 'Ù', 'ı', 'ˆ', '\u02dc', '\u00af', '\u02d8',
		'\u02d9', '\u02da', '\u00b8', '\u02dd', '\u02db', 'ˇ'
	};

	public CP10000()
		: base(10000, ToChars, "Western European (Mac)", "macintosh", "macintosh", "macintosh", isBrowserDisplay: false, isBrowserSave: false, isMailNewsDisplay: false, isMailNewsSave: false, 1252)
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
				case 160:
					num = 202;
					break;
				case 161:
					num = 193;
					break;
				case 164:
					num = 219;
					break;
				case 165:
					num = 180;
					break;
				case 167:
					num = 164;
					break;
				case 168:
					num = 172;
					break;
				case 170:
					num = 187;
					break;
				case 171:
					num = 199;
					break;
				case 172:
					num = 194;
					break;
				case 174:
					num = 168;
					break;
				case 175:
					num = 248;
					break;
				case 176:
					num = 161;
					break;
				case 180:
					num = 171;
					break;
				case 182:
					num = 166;
					break;
				case 183:
					num = 225;
					break;
				case 184:
					num = 252;
					break;
				case 186:
					num = 188;
					break;
				case 187:
					num = 200;
					break;
				case 191:
					num = 192;
					break;
				case 192:
					num = 203;
					break;
				case 193:
					num = 231;
					break;
				case 194:
					num = 229;
					break;
				case 195:
					num = 204;
					break;
				case 196:
					num = 128;
					break;
				case 197:
					num = 129;
					break;
				case 198:
					num = 174;
					break;
				case 199:
					num = 130;
					break;
				case 200:
					num = 233;
					break;
				case 201:
					num = 131;
					break;
				case 202:
					num = 230;
					break;
				case 203:
					num = 232;
					break;
				case 204:
					num = 237;
					break;
				case 205:
					num = 234;
					break;
				case 206:
					num = 235;
					break;
				case 207:
					num = 236;
					break;
				case 209:
					num = 132;
					break;
				case 210:
					num = 241;
					break;
				case 211:
					num = 238;
					break;
				case 212:
					num = 239;
					break;
				case 213:
					num = 205;
					break;
				case 214:
					num = 133;
					break;
				case 216:
					num = 175;
					break;
				case 217:
					num = 244;
					break;
				case 218:
					num = 242;
					break;
				case 219:
					num = 243;
					break;
				case 220:
					num = 134;
					break;
				case 223:
					num = 167;
					break;
				case 224:
					num = 136;
					break;
				case 225:
					num = 135;
					break;
				case 226:
					num = 137;
					break;
				case 227:
					num = 139;
					break;
				case 228:
					num = 138;
					break;
				case 229:
					num = 140;
					break;
				case 230:
					num = 190;
					break;
				case 231:
					num = 141;
					break;
				case 232:
					num = 143;
					break;
				case 233:
					num = 142;
					break;
				case 234:
					num = 144;
					break;
				case 235:
					num = 145;
					break;
				case 236:
					num = 147;
					break;
				case 237:
					num = 146;
					break;
				case 238:
					num = 148;
					break;
				case 239:
					num = 149;
					break;
				case 241:
					num = 150;
					break;
				case 242:
					num = 152;
					break;
				case 243:
					num = 151;
					break;
				case 244:
					num = 153;
					break;
				case 245:
					num = 155;
					break;
				case 246:
					num = 154;
					break;
				case 247:
					num = 214;
					break;
				case 248:
					num = 191;
					break;
				case 249:
					num = 157;
					break;
				case 250:
					num = 156;
					break;
				case 251:
					num = 158;
					break;
				case 252:
					num = 159;
					break;
				case 255:
					num = 216;
					break;
				case 305:
					num = 245;
					break;
				case 338:
					num = 206;
					break;
				case 339:
					num = 207;
					break;
				case 376:
					num = 217;
					break;
				case 402:
					num = 196;
					break;
				case 710:
					num = 246;
					break;
				case 711:
					num = 255;
					break;
				case 728:
					num = 249;
					break;
				case 729:
					num = 250;
					break;
				case 730:
					num = 251;
					break;
				case 731:
					num = 254;
					break;
				case 732:
					num = 247;
					break;
				case 733:
					num = 253;
					break;
				case 960:
					num = 185;
					break;
				case 8211:
					num = 208;
					break;
				case 8212:
					num = 209;
					break;
				case 8216:
					num = 212;
					break;
				case 8217:
					num = 213;
					break;
				case 8218:
					num = 226;
					break;
				case 8220:
					num = 210;
					break;
				case 8221:
					num = 211;
					break;
				case 8222:
					num = 227;
					break;
				case 8224:
					num = 160;
					break;
				case 8225:
					num = 224;
					break;
				case 8226:
					num = 165;
					break;
				case 8230:
					num = 201;
					break;
				case 8240:
					num = 228;
					break;
				case 8249:
					num = 220;
					break;
				case 8250:
					num = 221;
					break;
				case 8260:
					num = 218;
					break;
				case 8482:
					num = 170;
					break;
				case 8486:
					num = 189;
					break;
				case 8706:
					num = 182;
					break;
				case 8710:
					num = 198;
					break;
				case 8719:
					num = 184;
					break;
				case 8721:
					num = 183;
					break;
				case 8730:
					num = 195;
					break;
				case 8734:
					num = 176;
					break;
				case 8747:
					num = 186;
					break;
				case 8776:
					num = 197;
					break;
				case 8800:
					num = 173;
					break;
				case 8804:
					num = 178;
					break;
				case 8805:
					num = 179;
					break;
				case 8984:
					num = 17;
					break;
				case 9674:
					num = 215;
					break;
				case 9830:
					num = 19;
					break;
				case 10003:
					num = 18;
					break;
				case 63743:
					num = 240;
					break;
				case 64257:
					num = 222;
					break;
				case 64258:
					num = 223;
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
				case 162:
				case 163:
				case 169:
				case 177:
				case 181:
					break;
				}
			}
			bytes[byteIndex++] = (byte)num;
			charCount--;
			byteCount--;
		}
	}
}
