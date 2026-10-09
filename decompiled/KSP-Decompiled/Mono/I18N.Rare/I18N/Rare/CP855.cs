using System;
using System.Text;
using I18N.Common;

namespace I18N.Rare;

[Serializable]
public class CP855 : ByteEncoding
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
		'x', 'y', 'z', '{', '|', '}', '~', '\u001a', 'ђ', 'Ђ',
		'ѓ', 'Ѓ', 'ё', 'Ё', 'є', 'Є', 'ѕ', 'Ѕ', 'і', 'І',
		'ї', 'Ї', 'ј', 'Ј', 'љ', 'Љ', 'њ', 'Њ', 'ћ', 'Ћ',
		'ќ', 'Ќ', 'ў', 'Ў', 'џ', 'Џ', 'ю', 'Ю', 'ъ', 'Ъ',
		'а', 'А', 'б', 'Б', 'ц', 'Ц', 'д', 'Д', 'е', 'Е',
		'ф', 'Ф', 'г', 'Г', '«', '»', '░', '▒', '▓', '│',
		'┤', 'х', 'Х', 'и', 'И', '╣', '║', '╗', '╝', 'й',
		'Й', '┐', '└', '┴', '┬', '├', '─', '┼', 'к', 'К',
		'╚', '╔', '╩', '╦', '╠', '═', '╬', '¤', 'л', 'Л',
		'м', 'М', 'н', 'Н', 'о', 'О', 'п', '┘', '┌', '█',
		'▄', 'П', 'я', '▀', 'Я', 'р', 'Р', 'с', 'С', 'т',
		'Т', 'у', 'У', 'ж', 'Ж', 'в', 'В', 'ь', 'Ь', '№',
		'\u00ad', 'ы', 'Ы', 'з', 'З', 'ш', 'Ш', 'э', 'Э', 'щ',
		'Щ', 'ч', 'Ч', '§', '■', '\u00a0'
	};

	public CP855()
		: base(855, ToChars, "Cyrillic (DOS)", "ibm855", "ibm855", "ibm855", isBrowserDisplay: false, isBrowserSave: false, isMailNewsDisplay: false, isMailNewsSave: false, 1251)
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
					num = 207;
					break;
				case 167:
					num = 253;
					break;
				case 171:
					num = 174;
					break;
				case 173:
					num = 240;
					break;
				case 182:
					num = 20;
					break;
				case 187:
					num = 175;
					break;
				case 1025:
					num = 133;
					break;
				case 1026:
					num = 129;
					break;
				case 1027:
					num = 131;
					break;
				case 1028:
					num = 135;
					break;
				case 1029:
					num = 137;
					break;
				case 1030:
					num = 139;
					break;
				case 1031:
					num = 141;
					break;
				case 1032:
					num = 143;
					break;
				case 1033:
					num = 145;
					break;
				case 1034:
					num = 147;
					break;
				case 1035:
					num = 149;
					break;
				case 1036:
					num = 151;
					break;
				case 1038:
					num = 153;
					break;
				case 1039:
					num = 155;
					break;
				case 1040:
					num = 161;
					break;
				case 1041:
					num = 163;
					break;
				case 1042:
					num = 236;
					break;
				case 1043:
					num = 173;
					break;
				case 1044:
					num = 167;
					break;
				case 1045:
					num = 169;
					break;
				case 1046:
					num = 234;
					break;
				case 1047:
					num = 244;
					break;
				case 1048:
					num = 184;
					break;
				case 1049:
					num = 190;
					break;
				case 1050:
					num = 199;
					break;
				case 1051:
					num = 209;
					break;
				case 1052:
					num = 211;
					break;
				case 1053:
					num = 213;
					break;
				case 1054:
					num = 215;
					break;
				case 1055:
					num = 221;
					break;
				case 1056:
					num = 226;
					break;
				case 1057:
					num = 228;
					break;
				case 1058:
					num = 230;
					break;
				case 1059:
					num = 232;
					break;
				case 1060:
					num = 171;
					break;
				case 1061:
					num = 182;
					break;
				case 1062:
					num = 165;
					break;
				case 1063:
					num = 252;
					break;
				case 1064:
					num = 246;
					break;
				case 1065:
					num = 250;
					break;
				case 1066:
					num = 159;
					break;
				case 1067:
					num = 242;
					break;
				case 1068:
					num = 238;
					break;
				case 1069:
					num = 248;
					break;
				case 1070:
					num = 157;
					break;
				case 1071:
					num = 224;
					break;
				case 1072:
					num = 160;
					break;
				case 1073:
					num = 162;
					break;
				case 1074:
					num = 235;
					break;
				case 1075:
					num = 172;
					break;
				case 1076:
					num = 166;
					break;
				case 1077:
					num = 168;
					break;
				case 1078:
					num = 233;
					break;
				case 1079:
					num = 243;
					break;
				case 1080:
					num = 183;
					break;
				case 1081:
					num = 189;
					break;
				case 1082:
					num = 198;
					break;
				case 1083:
					num = 208;
					break;
				case 1084:
					num = 210;
					break;
				case 1085:
					num = 212;
					break;
				case 1086:
					num = 214;
					break;
				case 1087:
					num = 216;
					break;
				case 1088:
					num = 225;
					break;
				case 1089:
					num = 227;
					break;
				case 1090:
					num = 229;
					break;
				case 1091:
					num = 231;
					break;
				case 1092:
					num = 170;
					break;
				case 1093:
					num = 181;
					break;
				case 1094:
					num = 164;
					break;
				case 1095:
					num = 251;
					break;
				case 1096:
					num = 245;
					break;
				case 1097:
					num = 249;
					break;
				case 1098:
					num = 158;
					break;
				case 1099:
					num = 241;
					break;
				case 1100:
					num = 237;
					break;
				case 1101:
					num = 247;
					break;
				case 1102:
					num = 156;
					break;
				case 1103:
					num = 222;
					break;
				case 1105:
					num = 132;
					break;
				case 1106:
					num = 128;
					break;
				case 1107:
					num = 130;
					break;
				case 1108:
					num = 134;
					break;
				case 1109:
					num = 136;
					break;
				case 1110:
					num = 138;
					break;
				case 1111:
					num = 140;
					break;
				case 1112:
					num = 142;
					break;
				case 1113:
					num = 144;
					break;
				case 1114:
					num = 146;
					break;
				case 1115:
					num = 148;
					break;
				case 1116:
					num = 150;
					break;
				case 1118:
					num = 152;
					break;
				case 1119:
					num = 154;
					break;
				case 8226:
					num = 7;
					break;
				case 8252:
					num = 19;
					break;
				case 8470:
					num = 239;
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
