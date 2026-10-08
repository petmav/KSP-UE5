using System;
using System.Text;
using I18N.Common;

namespace I18N.West;

[Serializable]
public class CP28592 : ByteEncoding
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
		'\u00a0', 'Ą', '\u02d8', 'Ł', '¤', 'Ľ', 'Ś', '§', '\u00a8', 'Š',
		'Ş', 'Ť', 'Ź', '\u00ad', 'Ž', 'Ż', '°', 'ą', '\u02db', 'ł',
		'\u00b4', 'ľ', 'ś', 'ˇ', '\u00b8', 'š', 'ş', 'ť', 'ź', '\u02dd',
		'ž', 'ż', 'Ŕ', 'Á', 'Â', 'Ă', 'Ä', 'Ĺ', 'Ć', 'Ç',
		'Č', 'É', 'Ę', 'Ë', 'Ě', 'Í', 'Î', 'Ď', 'Đ', 'Ń',
		'Ň', 'Ó', 'Ô', 'Ő', 'Ö', '×', 'Ř', 'Ů', 'Ú', 'Ű',
		'Ü', 'Ý', 'Ţ', 'ß', 'ŕ', 'á', 'â', 'ă', 'ä', 'ĺ',
		'ć', 'ç', 'č', 'é', 'ę', 'ë', 'ě', 'í', 'î', 'ď',
		'đ', 'ń', 'ň', 'ó', 'ô', 'ő', 'ö', '÷', 'ř', 'ů',
		'ú', 'ű', 'ü', 'ý', 'ţ', '\u02d9'
	};

	public CP28592()
		: base(28592, ToChars, "Central European (ISO)", "iso-8859-2", "iso-8859-2", "iso-8859-2", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 1250)
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
				case 162:
					num = 141;
					break;
				case 165:
					num = 142;
					break;
				case 169:
					num = 136;
					break;
				case 174:
					num = 159;
					break;
				case 182:
					num = 20;
					break;
				case 258:
					num = 195;
					break;
				case 259:
					num = 227;
					break;
				case 260:
					num = 161;
					break;
				case 261:
					num = 177;
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
					num = 165;
					break;
				case 318:
					num = 181;
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
					num = 166;
					break;
				case 347:
					num = 182;
					break;
				case 350:
					num = 170;
					break;
				case 351:
					num = 186;
					break;
				case 352:
					num = 169;
					break;
				case 353:
					num = 185;
					break;
				case 354:
					num = 222;
					break;
				case 355:
					num = 254;
					break;
				case 356:
					num = 171;
					break;
				case 357:
					num = 187;
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
					num = 172;
					break;
				case 378:
					num = 188;
					break;
				case 379:
					num = 175;
					break;
				case 380:
					num = 191;
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
				case 8226:
					num = 7;
					break;
				case 8252:
					num = 19;
					break;
				case 8592:
					num = 27;
					break;
				case 8593:
					num = 24;
					break;
				case 8594:
					num = 26;
					break;
				case 8595:
					num = 25;
					break;
				case 8596:
					num = 29;
					break;
				case 8597:
					num = 18;
					break;
				case 8616:
					num = 23;
					break;
				case 8735:
					num = 28;
					break;
				case 9472:
					num = 148;
					break;
				case 9474:
					num = 131;
					break;
				case 9484:
					num = 134;
					break;
				case 9488:
					num = 143;
					break;
				case 9492:
					num = 144;
					break;
				case 9496:
					num = 133;
					break;
				case 9500:
					num = 147;
					break;
				case 9508:
					num = 132;
					break;
				case 9516:
					num = 146;
					break;
				case 9524:
					num = 145;
					break;
				case 9532:
					num = 149;
					break;
				case 9552:
					num = 157;
					break;
				case 9553:
					num = 138;
					break;
				case 9556:
					num = 153;
					break;
				case 9559:
					num = 139;
					break;
				case 9562:
					num = 152;
					break;
				case 9565:
					num = 140;
					break;
				case 9568:
					num = 156;
					break;
				case 9571:
					num = 137;
					break;
				case 9574:
					num = 155;
					break;
				case 9577:
					num = 154;
					break;
				case 9580:
					num = 158;
					break;
				case 9600:
					num = 151;
					break;
				case 9604:
					num = 150;
					break;
				case 9608:
					num = 135;
					break;
				case 9617:
					num = 128;
					break;
				case 9618:
					num = 129;
					break;
				case 9619:
					num = 130;
					break;
				case 9644:
					num = 22;
					break;
				case 9650:
					num = 30;
					break;
				case 9658:
					num = 16;
					break;
				case 9660:
					num = 31;
					break;
				case 9668:
					num = 17;
					break;
				case 9675:
					num = 9;
					break;
				case 9688:
					num = 8;
					break;
				case 9689:
					num = 10;
					break;
				case 9786:
					num = 1;
					break;
				case 9787:
					num = 2;
					break;
				case 9788:
					num = 15;
					break;
				case 9792:
					num = 12;
					break;
				case 9794:
					num = 11;
					break;
				case 9824:
					num = 6;
					break;
				case 9827:
					num = 5;
					break;
				case 9829:
					num = 3;
					break;
				case 9830:
					num = 4;
					break;
				case 9834:
					num = 13;
					break;
				case 9836:
					num = 14;
					break;
				case 65512:
					num = 131;
					break;
				case 65513:
					num = 27;
					break;
				case 65514:
					num = 24;
					break;
				case 65515:
					num = 26;
					break;
				case 65516:
					num = 25;
					break;
				case 65518:
					num = 9;
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
				case 176:
				case 180:
				case 184:
				case 193:
				case 194:
				case 196:
				case 199:
				case 201:
				case 203:
				case 205:
				case 206:
				case 208:
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
			charCount--;
			byteCount--;
		}
	}
}
