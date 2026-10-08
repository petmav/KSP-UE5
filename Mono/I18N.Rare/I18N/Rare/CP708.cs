using System;
using System.Text;
using I18N.Common;

namespace I18N.Rare;

[Serializable]
public class CP708 : ByteEncoding
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
		'\u00a0', '?', '?', '?', '¤', '?', '?', '?', '?', '?',
		'?', '?', '،', '\u00ad', '?', '?', '?', '?', '?', '?',
		'?', '?', '?', '?', '?', '?', '?', '؛', '?', '?',
		'?', '؟', '?', 'ء', 'آ', 'أ', 'ؤ', 'إ', 'ئ', 'ا',
		'ب', 'ة', 'ت', 'ث', 'ج', 'ح', 'خ', 'د', 'ذ', 'ر',
		'ز', 'س', 'ش', 'ص', 'ض', 'ط', 'ظ', 'ع', 'غ', '?',
		'?', '?', '?', '?', 'ـ', 'ف', 'ق', 'ك', 'ل', 'م',
		'ن', 'ه', 'و', 'ى', 'ي', '\u064b', '\u064c', '\u064d', '\u064e', '\u064f',
		'\u0650', '\u0651', '\u0652', '?', '?', '?', '?', '?', '?', '?',
		'?', '?', '?', '?', '?', '?'
	};

	public CP708()
		: base(708, ToChars, "Arabic (ASMO 708)", "iso-8859-6", "asmo-708", "asmo-708", isBrowserDisplay: false, isBrowserSave: false, isMailNewsDisplay: false, isMailNewsSave: false, 1256)
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
				case 1610:
				case 1611:
				case 1612:
				case 1613:
				case 1614:
				case 1615:
				case 1616:
				case 1617:
				case 1618:
					num -= 1376;
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
				case 65136:
					num = 235;
					break;
				case 65137:
					num = 235;
					break;
				case 65138:
					num = 236;
					break;
				case 65140:
					num = 237;
					break;
				case 65142:
					num = 238;
					break;
				case 65143:
					num = 238;
					break;
				case 65144:
					num = 239;
					break;
				case 65145:
					num = 239;
					break;
				case 65146:
					num = 240;
					break;
				case 65147:
					num = 240;
					break;
				case 65148:
					num = 241;
					break;
				case 65149:
					num = 241;
					break;
				case 65150:
					num = 242;
					break;
				case 65151:
					num = 242;
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
					num = 217;
					break;
				case 65226:
					num = 217;
					break;
				case 65227:
					num = 217;
					break;
				case 65228:
					num = 217;
					break;
				case 65229:
					num = 218;
					break;
				case 65230:
					num = 218;
					break;
				case 65231:
					num = 218;
					break;
				case 65232:
					num = 218;
					break;
				case 65233:
					num = 225;
					break;
				case 65234:
					num = 225;
					break;
				case 65235:
					num = 225;
					break;
				case 65236:
					num = 225;
					break;
				case 65237:
					num = 226;
					break;
				case 65238:
					num = 226;
					break;
				case 65239:
					num = 226;
					break;
				case 65240:
					num = 226;
					break;
				case 65241:
					num = 227;
					break;
				case 65242:
					num = 227;
					break;
				case 65243:
					num = 227;
					break;
				case 65244:
					num = 227;
					break;
				case 65245:
					num = 228;
					break;
				case 65246:
					num = 228;
					break;
				case 65247:
					num = 228;
					break;
				case 65248:
					num = 228;
					break;
				case 65249:
					num = 229;
					break;
				case 65250:
					num = 229;
					break;
				case 65251:
					num = 229;
					break;
				case 65252:
					num = 229;
					break;
				case 65253:
					num = 230;
					break;
				case 65254:
					num = 230;
					break;
				case 65255:
					num = 230;
					break;
				case 65256:
					num = 230;
					break;
				case 65257:
					num = 231;
					break;
				case 65258:
					num = 231;
					break;
				case 65259:
					num = 231;
					break;
				case 65260:
					num = 231;
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
					num = 233;
					break;
				case 65265:
					num = 234;
					break;
				case 65266:
					num = 234;
					break;
				case 65267:
					num = 234;
					break;
				case 65268:
					num = 234;
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
				case 173:
					break;
				}
			}
			bytes[byteIndex++] = (byte)num;
			charCount--;
			byteCount--;
		}
	}
}
