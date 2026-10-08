using System;
using System.Text;
using I18N.Common;

namespace I18N.Rare;

[Serializable]
public class CP858 : ByteEncoding
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
		'x', 'y', 'z', '{', '|', '}', '~', '\u001a', 'Ç', 'ü',
		'é', 'â', 'ä', 'à', 'å', 'ç', 'ê', 'ë', 'è', 'ï',
		'î', 'ì', 'Ä', 'Å', 'É', 'æ', 'Æ', 'ô', 'ö', 'ò',
		'û', 'ù', 'ÿ', 'Ö', 'Ü', 'ø', '£', 'Ø', '×', 'ƒ',
		'á', 'í', 'ó', 'ú', 'ñ', 'Ñ', 'ª', 'º', '¿', '®',
		'¬', '½', '¼', '¡', '«', '»', '░', '▒', '▓', '│',
		'┤', 'Á', 'Â', 'À', '©', '╣', '║', '╗', '╝', '¢',
		'¥', '┐', '└', '┴', '┬', '├', '─', '┼', 'ã', 'Ã',
		'╚', '╔', '╩', '╦', '╠', '═', '╬', '¤', 'ð', 'Ð',
		'Ê', 'Ë', 'È', '€', 'Í', 'Î', 'Ï', '┘', '┌', '█',
		'▄', '¦', 'Ì', '▀', 'Ó', 'ß', 'Ô', 'Ò', 'õ', 'Õ',
		'µ', 'þ', 'Þ', 'Ú', 'Û', 'Ù', 'ý', 'Ý', '\u00af', '\u00b4',
		'\u00ad', '±', '‗', '¾', '¶', '§', '÷', '\u00b8', '°', '\u00a8',
		'·', '¹', '³', '²', '■', '\u00a0'
	};

	public CP858()
		: base(858, ToChars, "Western European (DOS with Euro)", "IBM00858", "IBM00858", "IBM00858", isBrowserDisplay: false, isBrowserSave: false, isMailNewsDisplay: false, isMailNewsSave: false, 1252)
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
				switch (num)
				{
				case 26:
					num = 127;
					break;
				case 28:
					num = 26;
					break;
				case 127:
					num = 28;
					break;
				case 160:
					num = 255;
					break;
				case 161:
					num = 173;
					break;
				case 162:
					num = 189;
					break;
				case 163:
					num = 156;
					break;
				case 164:
					num = 207;
					break;
				case 165:
					num = 190;
					break;
				case 166:
					num = 221;
					break;
				case 167:
					num = 245;
					break;
				case 168:
					num = 249;
					break;
				case 169:
					num = 184;
					break;
				case 170:
					num = 166;
					break;
				case 171:
					num = 174;
					break;
				case 172:
					num = 170;
					break;
				case 173:
					num = 240;
					break;
				case 174:
					num = 169;
					break;
				case 175:
					num = 238;
					break;
				case 176:
					num = 248;
					break;
				case 177:
					num = 241;
					break;
				case 178:
					num = 253;
					break;
				case 179:
					num = 252;
					break;
				case 180:
					num = 239;
					break;
				case 181:
					num = 230;
					break;
				case 182:
					num = 244;
					break;
				case 183:
					num = 250;
					break;
				case 184:
					num = 247;
					break;
				case 185:
					num = 251;
					break;
				case 186:
					num = 167;
					break;
				case 187:
					num = 175;
					break;
				case 188:
					num = 172;
					break;
				case 189:
					num = 171;
					break;
				case 190:
					num = 243;
					break;
				case 191:
					num = 168;
					break;
				case 192:
					num = 183;
					break;
				case 193:
					num = 181;
					break;
				case 194:
					num = 182;
					break;
				case 195:
					num = 199;
					break;
				case 196:
					num = 142;
					break;
				case 197:
					num = 143;
					break;
				case 198:
					num = 146;
					break;
				case 199:
					num = 128;
					break;
				case 200:
					num = 212;
					break;
				case 201:
					num = 144;
					break;
				case 202:
					num = 210;
					break;
				case 203:
					num = 211;
					break;
				case 204:
					num = 222;
					break;
				case 205:
					num = 214;
					break;
				case 206:
					num = 215;
					break;
				case 207:
					num = 216;
					break;
				case 208:
					num = 209;
					break;
				case 209:
					num = 165;
					break;
				case 210:
					num = 227;
					break;
				case 211:
					num = 224;
					break;
				case 212:
					num = 226;
					break;
				case 213:
					num = 229;
					break;
				case 214:
					num = 153;
					break;
				case 215:
					num = 158;
					break;
				case 216:
					num = 157;
					break;
				case 217:
					num = 235;
					break;
				case 218:
					num = 233;
					break;
				case 219:
					num = 234;
					break;
				case 220:
					num = 154;
					break;
				case 221:
					num = 237;
					break;
				case 222:
					num = 232;
					break;
				case 223:
					num = 225;
					break;
				case 224:
					num = 133;
					break;
				case 225:
					num = 160;
					break;
				case 226:
					num = 131;
					break;
				case 227:
					num = 198;
					break;
				case 228:
					num = 132;
					break;
				case 229:
					num = 134;
					break;
				case 230:
					num = 145;
					break;
				case 231:
					num = 135;
					break;
				case 232:
					num = 138;
					break;
				case 233:
					num = 130;
					break;
				case 234:
					num = 136;
					break;
				case 235:
					num = 137;
					break;
				case 236:
					num = 141;
					break;
				case 237:
					num = 161;
					break;
				case 238:
					num = 140;
					break;
				case 239:
					num = 139;
					break;
				case 240:
					num = 208;
					break;
				case 241:
					num = 164;
					break;
				case 242:
					num = 149;
					break;
				case 243:
					num = 162;
					break;
				case 244:
					num = 147;
					break;
				case 245:
					num = 228;
					break;
				case 246:
					num = 148;
					break;
				case 247:
					num = 246;
					break;
				case 248:
					num = 155;
					break;
				case 249:
					num = 151;
					break;
				case 250:
					num = 163;
					break;
				case 251:
					num = 150;
					break;
				case 252:
					num = 129;
					break;
				case 253:
					num = 236;
					break;
				case 254:
					num = 231;
					break;
				case 255:
					num = 152;
					break;
				case 402:
					num = 159;
					break;
				case 8215:
					num = 242;
					break;
				case 8226:
					num = 7;
					break;
				case 8252:
					num = 19;
					break;
				case 8254:
					num = 238;
					break;
				case 8364:
					num = 213;
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
				case 8962:
					num = 127;
					break;
				case 9472:
					num = 196;
					break;
				case 9474:
					num = 179;
					break;
				case 9484:
					num = 218;
					break;
				case 9488:
					num = 191;
					break;
				case 9492:
					num = 192;
					break;
				case 9496:
					num = 217;
					break;
				case 9500:
					num = 195;
					break;
				case 9508:
					num = 180;
					break;
				case 9516:
					num = 194;
					break;
				case 9524:
					num = 193;
					break;
				case 9532:
					num = 197;
					break;
				case 9552:
					num = 205;
					break;
				case 9553:
					num = 186;
					break;
				case 9556:
					num = 201;
					break;
				case 9559:
					num = 187;
					break;
				case 9562:
					num = 200;
					break;
				case 9565:
					num = 188;
					break;
				case 9568:
					num = 204;
					break;
				case 9571:
					num = 185;
					break;
				case 9574:
					num = 203;
					break;
				case 9577:
					num = 202;
					break;
				case 9580:
					num = 206;
					break;
				case 9600:
					num = 223;
					break;
				case 9604:
					num = 220;
					break;
				case 9608:
					num = 219;
					break;
				case 9617:
					num = 176;
					break;
				case 9618:
					num = 177;
					break;
				case 9619:
					num = 178;
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
					num = 179;
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
					break;
				}
			}
			bytes[byteIndex++] = (byte)num;
			charCount--;
			byteCount--;
		}
	}
}
