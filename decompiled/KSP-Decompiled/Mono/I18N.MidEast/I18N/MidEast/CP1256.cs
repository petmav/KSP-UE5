using System;
using System.Text;
using I18N.Common;

namespace I18N.MidEast;

[Serializable]
public class CP1256 : ByteEncoding
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
		'x', 'y', 'z', '{', '|', '}', '~', '\u007f', '€', 'پ',
		'‚', 'ƒ', '„', '…', '†', '‡', 'ˆ', '‰', 'ٹ', '‹',
		'Œ', 'چ', 'ژ', 'ڈ', 'گ', '‘', '’', '“', '”', '•',
		'–', '—', 'ک', '™', 'ڑ', '›', 'œ', '\u200c', '\u200d', 'ں',
		'\u00a0', '،', '¢', '£', '¤', '¥', '¦', '§', '\u00a8', '©',
		'ھ', '«', '¬', '\u00ad', '®', '\u00af', '°', '±', '²', '³',
		'\u00b4', 'µ', '¶', '·', '\u00b8', '¹', '؛', '»', '¼', '½',
		'¾', '؟', 'ہ', 'ء', 'آ', 'أ', 'ؤ', 'إ', 'ئ', 'ا',
		'ب', 'ة', 'ت', 'ث', 'ج', 'ح', 'خ', 'د', 'ذ', 'ر',
		'ز', 'س', 'ش', 'ص', 'ض', '×', 'ط', 'ظ', 'ع', 'غ',
		'ـ', 'ف', 'ق', 'ك', 'à', 'ل', 'â', 'م', 'ن', 'ه',
		'و', 'ç', 'è', 'é', 'ê', 'ë', 'ى', 'ي', 'î', 'ï',
		'\u064b', '\u064c', '\u064d', '\u064e', 'ô', '\u064f', '\u0650', '÷', '\u0651', 'ù',
		'\u0652', 'û', 'ü', '\u200e', '\u200f', 'ے'
	};

	public CP1256()
		: base(1256, ToChars, "Arabic (Windows)", "windows-1256", "windows-1256", "windows-1256", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 1256)
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
				case 338:
					num = 140;
					break;
				case 339:
					num = 156;
					break;
				case 402:
					num = 131;
					break;
				case 710:
					num = 136;
					break;
				case 1548:
					num = 161;
					break;
				case 1563:
					num = 186;
					break;
				case 1567:
					num = 191;
					break;
				case 1569:
				case 1570:
				case 1571:
				case 1572:
				case 1573:
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
					num -= 1376;
					break;
				case 1591:
				case 1592:
				case 1593:
				case 1594:
					num -= 1375;
					break;
				case 1600:
				case 1601:
				case 1602:
				case 1603:
					num -= 1380;
					break;
				case 1604:
					num = 225;
					break;
				case 1605:
				case 1606:
				case 1607:
				case 1608:
					num -= 1378;
					break;
				case 1609:
					num = 236;
					break;
				case 1610:
					num = 237;
					break;
				case 1611:
				case 1612:
				case 1613:
				case 1614:
					num -= 1371;
					break;
				case 1615:
					num = 245;
					break;
				case 1616:
					num = 246;
					break;
				case 1617:
					num = 248;
					break;
				case 1618:
					num = 250;
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
					num -= 1584;
					break;
				case 1643:
					num = 44;
					break;
				case 1644:
					num = 46;
					break;
				case 1657:
					num = 138;
					break;
				case 1662:
					num = 129;
					break;
				case 1670:
					num = 141;
					break;
				case 1672:
					num = 143;
					break;
				case 1681:
					num = 154;
					break;
				case 1688:
					num = 142;
					break;
				case 1705:
					num = 152;
					break;
				case 1711:
					num = 144;
					break;
				case 1722:
					num = 159;
					break;
				case 1726:
					num = 170;
					break;
				case 1729:
					num = 192;
					break;
				case 1746:
					num = 255;
					break;
				case 1776:
				case 1777:
				case 1778:
				case 1779:
				case 1780:
				case 1781:
				case 1782:
				case 1783:
				case 1784:
				case 1785:
					num -= 1728;
					break;
				case 8204:
					num = 157;
					break;
				case 8205:
					num = 158;
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
				case 8364:
					num = 128;
					break;
				case 8482:
					num = 153;
					break;
				case 64342:
					num = 129;
					break;
				case 64344:
					num = 129;
					break;
				case 64358:
					num = 138;
					break;
				case 64360:
					num = 138;
					break;
				case 64378:
					num = 141;
					break;
				case 64380:
					num = 141;
					break;
				case 64392:
					num = 143;
					break;
				case 64394:
					num = 142;
					break;
				case 64396:
					num = 154;
					break;
				case 64398:
					num = 152;
					break;
				case 64400:
					num = 152;
					break;
				case 64402:
					num = 144;
					break;
				case 64404:
					num = 144;
					break;
				case 64414:
					num = 159;
					break;
				case 64422:
					num = 192;
					break;
				case 64424:
					num = 192;
					break;
				case 64426:
					num = 170;
					break;
				case 64428:
					num = 170;
					break;
				case 64430:
					num = 255;
					break;
				case 65136:
					num = 240;
					break;
				case 65137:
					num = 240;
					break;
				case 65138:
					num = 241;
					break;
				case 65140:
					num = 242;
					break;
				case 65142:
					num = 243;
					break;
				case 65143:
					num = 243;
					break;
				case 65144:
					num = 245;
					break;
				case 65145:
					num = 245;
					break;
				case 65146:
					num = 246;
					break;
				case 65147:
					num = 246;
					break;
				case 65148:
					num = 248;
					break;
				case 65149:
					num = 248;
					break;
				case 65150:
					num = 250;
					break;
				case 65151:
					num = 250;
					break;
				case 65152:
					num = 193;
					break;
				case 65153:
					num = 194;
					break;
				case 65154:
					num = 194;
					break;
				case 65155:
					num = 195;
					break;
				case 65156:
					num = 195;
					break;
				case 65157:
					num = 196;
					break;
				case 65158:
					num = 196;
					break;
				case 65159:
					num = 197;
					break;
				case 65160:
					num = 197;
					break;
				case 65161:
					num = 198;
					break;
				case 65162:
					num = 198;
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
					num = 199;
					break;
				case 65167:
					num = 200;
					break;
				case 65168:
					num = 200;
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
					num = 202;
					break;
				case 65174:
					num = 202;
					break;
				case 65175:
					num = 202;
					break;
				case 65176:
					num = 202;
					break;
				case 65177:
					num = 203;
					break;
				case 65178:
					num = 203;
					break;
				case 65179:
					num = 203;
					break;
				case 65180:
					num = 203;
					break;
				case 65181:
					num = 204;
					break;
				case 65182:
					num = 204;
					break;
				case 65183:
					num = 204;
					break;
				case 65184:
					num = 204;
					break;
				case 65185:
					num = 205;
					break;
				case 65186:
					num = 205;
					break;
				case 65187:
					num = 205;
					break;
				case 65188:
					num = 205;
					break;
				case 65189:
					num = 206;
					break;
				case 65190:
					num = 206;
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
					num = 211;
					break;
				case 65202:
					num = 211;
					break;
				case 65203:
					num = 211;
					break;
				case 65204:
					num = 211;
					break;
				case 65205:
					num = 212;
					break;
				case 65206:
					num = 212;
					break;
				case 65207:
					num = 212;
					break;
				case 65208:
					num = 212;
					break;
				case 65209:
					num = 213;
					break;
				case 65210:
					num = 213;
					break;
				case 65211:
					num = 213;
					break;
				case 65212:
					num = 213;
					break;
				case 65213:
					num = 214;
					break;
				case 65214:
					num = 214;
					break;
				case 65215:
					num = 214;
					break;
				case 65216:
					num = 214;
					break;
				case 65217:
					num = 216;
					break;
				case 65218:
					num = 216;
					break;
				case 65219:
					num = 216;
					break;
				case 65220:
					num = 216;
					break;
				case 65221:
					num = 217;
					break;
				case 65222:
					num = 217;
					break;
				case 65223:
					num = 217;
					break;
				case 65224:
					num = 217;
					break;
				case 65225:
					num = 218;
					break;
				case 65226:
					num = 218;
					break;
				case 65227:
					num = 218;
					break;
				case 65228:
					num = 218;
					break;
				case 65229:
					num = 219;
					break;
				case 65230:
					num = 219;
					break;
				case 65231:
					num = 219;
					break;
				case 65232:
					num = 219;
					break;
				case 65233:
					num = 221;
					break;
				case 65234:
					num = 221;
					break;
				case 65235:
					num = 221;
					break;
				case 65236:
					num = 221;
					break;
				case 65237:
					num = 222;
					break;
				case 65238:
					num = 222;
					break;
				case 65239:
					num = 222;
					break;
				case 65240:
					num = 222;
					break;
				case 65241:
					num = 223;
					break;
				case 65242:
					num = 223;
					break;
				case 65243:
					num = 223;
					break;
				case 65244:
					num = 223;
					break;
				case 65245:
					num = 225;
					break;
				case 65246:
					num = 225;
					break;
				case 65247:
					num = 225;
					break;
				case 65248:
					num = 225;
					break;
				case 65249:
					num = 227;
					break;
				case 65250:
					num = 227;
					break;
				case 65251:
					num = 227;
					break;
				case 65252:
					num = 227;
					break;
				case 65253:
					num = 228;
					break;
				case 65254:
					num = 228;
					break;
				case 65255:
					num = 228;
					break;
				case 65256:
					num = 228;
					break;
				case 65257:
					num = 229;
					break;
				case 65258:
					num = 229;
					break;
				case 65259:
					num = 229;
					break;
				case 65260:
					num = 229;
					break;
				case 65261:
					num = 230;
					break;
				case 65262:
					num = 230;
					break;
				case 65263:
					num = 236;
					break;
				case 65264:
					num = 236;
					break;
				case 65265:
					num = 237;
					break;
				case 65266:
					num = 237;
					break;
				case 65267:
					num = 237;
					break;
				case 65268:
					num = 237;
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
				case 160:
				case 162:
				case 163:
				case 164:
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
				case 215:
				case 224:
				case 226:
				case 231:
				case 232:
				case 233:
				case 234:
				case 235:
				case 238:
				case 239:
				case 244:
				case 247:
				case 249:
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
