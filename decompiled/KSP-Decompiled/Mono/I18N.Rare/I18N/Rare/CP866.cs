using System;
using System.Text;
using I18N.Common;

namespace I18N.Rare;

[Serializable]
public class CP866 : ByteEncoding
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
		'x', 'y', 'z', '{', '|', '}', '~', '\u001a', 'А', 'Б',
		'В', 'Г', 'Д', 'Е', 'Ж', 'З', 'И', 'Й', 'К', 'Л',
		'М', 'Н', 'О', 'П', 'Р', 'С', 'Т', 'У', 'Ф', 'Х',
		'Ц', 'Ч', 'Ш', 'Щ', 'Ъ', 'Ы', 'Ь', 'Э', 'Ю', 'Я',
		'а', 'б', 'в', 'г', 'д', 'е', 'ж', 'з', 'и', 'й',
		'к', 'л', 'м', 'н', 'о', 'п', '░', '▒', '▓', '│',
		'┤', '╡', '╢', '╖', '╕', '╣', '║', '╗', '╝', '╜',
		'╛', '┐', '└', '┴', '┬', '├', '─', '┼', '╞', '╟',
		'╚', '╔', '╩', '╦', '╠', '═', '╬', '╧', '╨', '╤',
		'╥', '╙', '╘', '╒', '╓', '╫', '╪', '┘', '┌', '█',
		'▄', '▌', '▐', '▀', 'р', 'с', 'т', 'у', 'ф', 'х',
		'ц', 'ч', 'ш', 'щ', 'ъ', 'ы', 'ь', 'э', 'ю', 'я',
		'Ё', 'ё', 'Є', 'є', 'Ї', 'ї', 'Ў', 'ў', '°', '∙',
		'·', '√', '№', '¤', '■', '\u00a0'
	};

	public CP866()
		: base(866, ToChars, "Russian (DOS)", "ibm866", "ibm866", "ibm866", isBrowserDisplay: false, isBrowserSave: false, isMailNewsDisplay: false, isMailNewsSave: false, 1251)
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
				case 164:
					num = 253;
					break;
				case 167:
					num = 21;
					break;
				case 176:
					num = 248;
					break;
				case 182:
					num = 20;
					break;
				case 183:
					num = 250;
					break;
				case 1025:
					num = 240;
					break;
				case 1028:
					num = 242;
					break;
				case 1031:
					num = 244;
					break;
				case 1038:
					num = 246;
					break;
				case 1040:
				case 1041:
				case 1042:
				case 1043:
				case 1044:
				case 1045:
				case 1046:
				case 1047:
				case 1048:
				case 1049:
				case 1050:
				case 1051:
				case 1052:
				case 1053:
				case 1054:
				case 1055:
				case 1056:
				case 1057:
				case 1058:
				case 1059:
				case 1060:
				case 1061:
				case 1062:
				case 1063:
				case 1064:
				case 1065:
				case 1066:
				case 1067:
				case 1068:
				case 1069:
				case 1070:
				case 1071:
				case 1072:
				case 1073:
				case 1074:
				case 1075:
				case 1076:
				case 1077:
				case 1078:
				case 1079:
				case 1080:
				case 1081:
				case 1082:
				case 1083:
				case 1084:
				case 1085:
				case 1086:
				case 1087:
					num -= 912;
					break;
				case 1088:
				case 1089:
				case 1090:
				case 1091:
				case 1092:
				case 1093:
				case 1094:
				case 1095:
				case 1096:
				case 1097:
				case 1098:
				case 1099:
				case 1100:
				case 1101:
				case 1102:
				case 1103:
					num -= 864;
					break;
				case 1105:
					num = 241;
					break;
				case 1108:
					num = 243;
					break;
				case 1111:
					num = 245;
					break;
				case 1118:
					num = 247;
					break;
				case 8226:
					num = 7;
					break;
				case 8252:
					num = 19;
					break;
				case 8470:
					num = 252;
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
					num = 249;
					break;
				case 8730:
					num = 251;
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
				case 9554:
					num = 213;
					break;
				case 9555:
					num = 214;
					break;
				case 9556:
					num = 201;
					break;
				case 9557:
					num = 184;
					break;
				case 9558:
					num = 183;
					break;
				case 9559:
					num = 187;
					break;
				case 9560:
					num = 212;
					break;
				case 9561:
					num = 211;
					break;
				case 9562:
					num = 200;
					break;
				case 9563:
					num = 190;
					break;
				case 9564:
					num = 189;
					break;
				case 9565:
					num = 188;
					break;
				case 9566:
					num = 198;
					break;
				case 9567:
					num = 199;
					break;
				case 9568:
					num = 204;
					break;
				case 9569:
					num = 181;
					break;
				case 9570:
					num = 182;
					break;
				case 9571:
					num = 185;
					break;
				case 9572:
					num = 209;
					break;
				case 9573:
					num = 210;
					break;
				case 9574:
					num = 203;
					break;
				case 9575:
					num = 207;
					break;
				case 9576:
					num = 208;
					break;
				case 9577:
					num = 202;
					break;
				case 9578:
					num = 216;
					break;
				case 9579:
					num = 215;
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
				case 9612:
					num = 221;
					break;
				case 9616:
					num = 222;
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
				case 9835:
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
