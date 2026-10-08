using System;
using System.Text;
using I18N.Common;

namespace I18N.Rare;

[Serializable]
public class CP864 : ByteEncoding
{
	private static readonly char[] ToChars = new char[256]
	{
		'\0', '\u0001', '\u0002', '\u0003', '\u0004', '\u0005', '\u0006', '\a', '\b', '\t',
		'\n', '\v', '\f', '\r', '\u000e', '\u000f', '\u0010', '\u0011', '\u0012', '\u0013',
		'\u0014', '\u0015', '\u0016', '\u0017', '\u0018', '\u0019', '\u001c', '\u001b', '\u007f', '\u001d',
		'\u001e', '\u001f', ' ', '!', '"', '#', '$', '%', '&', '\'',
		'(', ')', '*', '+', ',', '-', '.', '/', '0', '1',
		'2', '3', '4', '5', '6', '7', '8', '9', ':', ';',
		'<', '=', '>', '?', '@', 'A', 'B', 'C', 'D', 'E',
		'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
		'Z', '[', '\\', ']', '^', '_', '`', 'a', 'b', 'c',
		'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
		'x', 'y', 'z', '{', '|', '}', '~', '\u001a', '°', '·',
		'∙', '√', '▒', '─', '│', '┼', '┤', '┬', '├', '┴',
		'┐', '┌', '└', '┘', 'β', '∞', 'φ', '±', '½', '¼',
		'≈', '«', '»', 'ﻷ', 'ﻸ', '?', '?', 'ﻻ', 'ﻼ', '\u200b',
		'\u00a0', '\u00ad', 'ﺂ', '£', '¤', 'ﺄ', '?', '?', 'ﺎ', 'ﺏ',
		'ﺕ', 'ﺙ', '،', 'ﺝ', 'ﺡ', 'ﺥ', '٠', '١', '٢', '٣',
		'٤', '٥', '٦', '٧', '٨', '٩', 'ﻑ', '؛', 'ﺱ', 'ﺵ',
		'ﺹ', '؟', '¢', 'ﺀ', 'ﺁ', 'ﺃ', 'ﺅ', 'ﻊ', 'ﺋ', 'ﺍ',
		'ﺑ', 'ﺓ', 'ﺗ', 'ﺛ', 'ﺟ', 'ﺣ', 'ﺧ', 'ﺩ', 'ﺫ', 'ﺭ',
		'ﺯ', 'ﺳ', 'ﺷ', 'ﺻ', 'ﺿ', 'ﻃ', 'ﻇ', 'ﻋ', 'ﻏ', '¦',
		'¬', '÷', '×', 'ﻉ', 'ـ', 'ﻓ', 'ﻗ', 'ﻛ', 'ﻟ', 'ﻣ',
		'ﻧ', 'ﻫ', 'ﻭ', 'ﻯ', 'ﻳ', 'ﺽ', 'ﻌ', 'ﻎ', 'ﻍ', 'ﻡ',
		'ﹽ', 'ﹼ', 'ﻥ', 'ﻩ', 'ﻬ', 'ﻰ', 'ﻲ', 'ﻐ', 'ﻕ', 'ﻵ',
		'ﻶ', 'ﻝ', 'ﻙ', 'ﻱ', '■', '?'
	};

	public CP864()
		: base(864, ToChars, "Arabic (DOS)", "ibm864", "ibm864", "ibm864", isBrowserDisplay: false, isBrowserSave: false, isMailNewsDisplay: false, isMailNewsSave: false, 1256)
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
			if (num >= 26)
			{
				int num2 = num;
				switch (num2)
				{
				default:
					switch (num2)
					{
					case 946:
						num = 144;
						break;
					case 966:
						num = 146;
						break;
					case 1548:
						num = 172;
						break;
					case 1563:
						num = 187;
						break;
					case 1567:
						num = 191;
						break;
					case 1569:
					case 1570:
					case 1571:
					case 1572:
						num -= 1376;
						break;
					case 1574:
					case 1575:
					case 1576:
					case 1577:
					case 1578:
					case 1579:
					case 1580:
					case 1581:
					case 1582:
					case 1583:
					case 1584:
					case 1585:
					case 1586:
					case 1587:
					case 1588:
					case 1589:
					case 1590:
					case 1591:
					case 1592:
					case 1593:
					case 1594:
						num -= 1376;
						break;
					case 1600:
					case 1601:
					case 1602:
					case 1603:
					case 1604:
					case 1605:
					case 1606:
					case 1607:
					case 1608:
					case 1609:
						num -= 1376;
						break;
					case 1610:
						num = 253;
						break;
					case 1617:
						num = 241;
						break;
					case 1632:
					case 1633:
					case 1634:
					case 1635:
					case 1636:
					case 1637:
					case 1638:
					case 1639:
					case 1640:
					case 1641:
						num -= 1456;
						break;
					case 1642:
						num = 37;
						break;
					case 1643:
						num = 44;
						break;
					case 1644:
						num = 46;
						break;
					case 1645:
						num = 42;
						break;
					case 8203:
						num = 159;
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
					case 8729:
						num = 130;
						break;
					case 8730:
						num = 131;
						break;
					case 8734:
						num = 145;
						break;
					case 8735:
						num = 28;
						break;
					case 8776:
						num = 150;
						break;
					case 8962:
						num = 127;
						break;
					case 9472:
						num = 133;
						break;
					case 9474:
						num = 134;
						break;
					case 9484:
						num = 141;
						break;
					case 9488:
						num = 140;
						break;
					case 9492:
						num = 142;
						break;
					case 9496:
						num = 143;
						break;
					case 9500:
						num = 138;
						break;
					case 9508:
						num = 136;
						break;
					case 9516:
						num = 137;
						break;
					case 9524:
						num = 139;
						break;
					case 9532:
						num = 135;
						break;
					case 9552:
						num = 5;
						break;
					case 9553:
						num = 6;
						break;
					case 9556:
						num = 13;
						break;
					case 9559:
						num = 12;
						break;
					case 9562:
						num = 14;
						break;
					case 9565:
						num = 15;
						break;
					case 9568:
						num = 10;
						break;
					case 9571:
						num = 8;
						break;
					case 9574:
						num = 9;
						break;
					case 9577:
						num = 11;
						break;
					case 9580:
						num = 7;
						break;
					case 9618:
						num = 132;
						break;
					case 9632:
						num = 254;
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
					case 9786:
						num = 1;
						break;
					case 9788:
						num = 4;
						break;
					case 9834:
						num = 2;
						break;
					case 9836:
						num = 3;
						break;
					case 65148:
						num = 241;
						break;
					case 65149:
						num = 240;
						break;
					case 65152:
						num = 193;
						break;
					case 65153:
						num = 194;
						break;
					case 65154:
						num = 162;
						break;
					case 65155:
						num = 195;
						break;
					case 65156:
						num = 165;
						break;
					case 65157:
						num = 196;
						break;
					case 65158:
						num = 196;
						break;
					case 65163:
						num = 198;
						break;
					case 65164:
						num = 198;
						break;
					case 65165:
						num = 199;
						break;
					case 65166:
						num = 168;
						break;
					case 65167:
						num = 169;
						break;
					case 65168:
						num = 169;
						break;
					case 65169:
						num = 200;
						break;
					case 65170:
						num = 200;
						break;
					case 65171:
						num = 201;
						break;
					case 65172:
						num = 201;
						break;
					case 65173:
						num = 170;
						break;
					case 65174:
						num = 170;
						break;
					case 65175:
						num = 202;
						break;
					case 65176:
						num = 202;
						break;
					case 65177:
						num = 171;
						break;
					case 65178:
						num = 171;
						break;
					case 65179:
						num = 203;
						break;
					case 65180:
						num = 203;
						break;
					case 65181:
						num = 173;
						break;
					case 65182:
						num = 173;
						break;
					case 65183:
						num = 204;
						break;
					case 65184:
						num = 204;
						break;
					case 65185:
						num = 174;
						break;
					case 65186:
						num = 174;
						break;
					case 65187:
						num = 205;
						break;
					case 65188:
						num = 205;
						break;
					case 65189:
						num = 175;
						break;
					case 65190:
						num = 175;
						break;
					case 65191:
						num = 206;
						break;
					case 65192:
						num = 206;
						break;
					case 65193:
						num = 207;
						break;
					case 65194:
						num = 207;
						break;
					case 65195:
						num = 208;
						break;
					case 65196:
						num = 208;
						break;
					case 65197:
						num = 209;
						break;
					case 65198:
						num = 209;
						break;
					case 65199:
						num = 210;
						break;
					case 65200:
						num = 210;
						break;
					case 65201:
						num = 188;
						break;
					case 65202:
						num = 188;
						break;
					case 65203:
						num = 211;
						break;
					case 65204:
						num = 211;
						break;
					case 65205:
						num = 189;
						break;
					case 65206:
						num = 189;
						break;
					case 65207:
						num = 212;
						break;
					case 65208:
						num = 212;
						break;
					case 65209:
						num = 190;
						break;
					case 65210:
						num = 190;
						break;
					case 65211:
						num = 213;
						break;
					case 65212:
						num = 213;
						break;
					case 65213:
						num = 235;
						break;
					case 65214:
						num = 235;
						break;
					case 65215:
						num = 214;
						break;
					case 65216:
						num = 214;
						break;
					case 65217:
						num = 215;
						break;
					case 65218:
						num = 215;
						break;
					case 65219:
						num = 215;
						break;
					case 65220:
						num = 215;
						break;
					case 65221:
						num = 216;
						break;
					case 65222:
						num = 216;
						break;
					case 65223:
						num = 216;
						break;
					case 65224:
						num = 216;
						break;
					case 65225:
						num = 223;
						break;
					case 65226:
						num = 197;
						break;
					case 65227:
						num = 217;
						break;
					case 65228:
						num = 236;
						break;
					case 65229:
						num = 238;
						break;
					case 65230:
						num = 237;
						break;
					case 65231:
						num = 218;
						break;
					case 65232:
						num = 247;
						break;
					case 65233:
						num = 186;
						break;
					case 65234:
						num = 186;
						break;
					case 65235:
						num = 225;
						break;
					case 65236:
						num = 225;
						break;
					case 65237:
						num = 248;
						break;
					case 65238:
						num = 248;
						break;
					case 65239:
						num = 226;
						break;
					case 65240:
						num = 226;
						break;
					case 65241:
						num = 252;
						break;
					case 65242:
						num = 252;
						break;
					case 65243:
						num = 227;
						break;
					case 65244:
						num = 227;
						break;
					case 65245:
						num = 251;
						break;
					case 65246:
						num = 251;
						break;
					case 65247:
						num = 228;
						break;
					case 65248:
						num = 228;
						break;
					case 65249:
						num = 239;
						break;
					case 65250:
						num = 239;
						break;
					case 65251:
						num = 229;
						break;
					case 65252:
						num = 229;
						break;
					case 65253:
						num = 242;
						break;
					case 65254:
						num = 242;
						break;
					case 65255:
						num = 230;
						break;
					case 65256:
						num = 230;
						break;
					case 65257:
						num = 243;
						break;
					case 65258:
						num = 243;
						break;
					case 65259:
						num = 231;
						break;
					case 65260:
						num = 244;
						break;
					case 65261:
						num = 232;
						break;
					case 65262:
						num = 232;
						break;
					case 65263:
						num = 233;
						break;
					case 65264:
						num = 245;
						break;
					case 65265:
						num = 253;
						break;
					case 65266:
						num = 246;
						break;
					case 65267:
						num = 234;
						break;
					case 65268:
						num = 234;
						break;
					case 65269:
						num = 249;
						break;
					case 65270:
						num = 250;
						break;
					case 65271:
						num = 153;
						break;
					case 65272:
						num = 154;
						break;
					case 65275:
						num = 157;
						break;
					case 65276:
						num = 158;
						break;
					case 65512:
						num = 134;
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
					case 65517:
						num = 254;
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
					}
					break;
				case 26:
					num = 127;
					break;
				case 28:
					num = 26;
					break;
				case 127:
					num = 28;
					break;
				case 162:
					num = 192;
					break;
				case 166:
					num = 219;
					break;
				case 167:
					num = 21;
					break;
				case 171:
					num = 151;
					break;
				case 172:
					num = 220;
					break;
				case 173:
					num = 161;
					break;
				case 176:
					num = 128;
					break;
				case 177:
					num = 147;
					break;
				case 182:
					num = 20;
					break;
				case 183:
					num = 129;
					break;
				case 187:
					num = 152;
					break;
				case 188:
					num = 149;
					break;
				case 189:
					num = 148;
					break;
				case 215:
					num = 222;
					break;
				case 247:
					num = 221;
					break;
				case 27:
				case 29:
				case 30:
				case 31:
				case 32:
				case 33:
				case 34:
				case 35:
				case 36:
				case 37:
				case 38:
				case 39:
				case 40:
				case 41:
				case 42:
				case 43:
				case 44:
				case 45:
				case 46:
				case 47:
				case 48:
				case 49:
				case 50:
				case 51:
				case 52:
				case 53:
				case 54:
				case 55:
				case 56:
				case 57:
				case 58:
				case 59:
				case 60:
				case 61:
				case 62:
				case 63:
				case 64:
				case 65:
				case 66:
				case 67:
				case 68:
				case 69:
				case 70:
				case 71:
				case 72:
				case 73:
				case 74:
				case 75:
				case 76:
				case 77:
				case 78:
				case 79:
				case 80:
				case 81:
				case 82:
				case 83:
				case 84:
				case 85:
				case 86:
				case 87:
				case 88:
				case 89:
				case 90:
				case 91:
				case 92:
				case 93:
				case 94:
				case 95:
				case 96:
				case 97:
				case 98:
				case 99:
				case 100:
				case 101:
				case 102:
				case 103:
				case 104:
				case 105:
				case 106:
				case 107:
				case 108:
				case 109:
				case 110:
				case 111:
				case 112:
				case 113:
				case 114:
				case 115:
				case 116:
				case 117:
				case 118:
				case 119:
				case 120:
				case 121:
				case 122:
				case 123:
				case 124:
				case 125:
				case 126:
				case 160:
				case 163:
				case 164:
					break;
				}
			}
			bytes[byteIndex++] = (byte)num;
			charCount--;
			byteCount--;
		}
	}
}
