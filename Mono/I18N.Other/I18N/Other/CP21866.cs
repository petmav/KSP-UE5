using System;
using System.Text;
using I18N.Common;

namespace I18N.Other;

[Serializable]
public class CP21866 : ByteEncoding
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
		'x', 'y', 'z', '{', '|', '}', '~', '\u007f', '─', '│',
		'┌', '┐', '└', '┘', '├', '┤', '┬', '┴', '┼', '▀',
		'▄', '█', '▌', '▐', '░', '▒', '▓', '⌠', '■', '∙',
		'√', '≈', '≤', '≥', '\u00a0', '⌡', '°', '²', '·', '÷',
		'═', '║', '╒', 'ё', 'є', '╔', 'і', 'ї', '╗', '╘',
		'╙', '╚', '╛', 'ґ', '╝', '╞', '╟', '╠', '╡', 'Ё',
		'Є', '╣', 'І', 'Ї', '╦', '╧', '╨', '╩', '╪', 'Ґ',
		'╬', '©', 'ю', 'а', 'б', 'ц', 'д', 'е', 'ф', 'г',
		'х', 'и', 'й', 'к', 'л', 'м', 'н', 'о', 'п', 'я',
		'р', 'с', 'т', 'у', 'ж', 'в', 'ь', 'ы', 'з', 'ш',
		'э', 'щ', 'ч', 'ъ', 'Ю', 'А', 'Б', 'Ц', 'Д', 'Е',
		'Ф', 'Г', 'Х', 'И', 'Й', 'К', 'Л', 'М', 'Н', 'О',
		'П', 'Я', 'Р', 'С', 'Т', 'У', 'Ж', 'В', 'Ь', 'Ы',
		'З', 'Ш', 'Э', 'Щ', 'Ч', 'Ъ'
	};

	public CP21866()
		: base(21866, ToChars, "Ukrainian (KOI8-U)", "koi8-u", "koi8-u", "koi8-u", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 1251)
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
					num = 154;
					break;
				case 169:
					num = 191;
					break;
				case 176:
					num = 156;
					break;
				case 178:
					num = 157;
					break;
				case 183:
					num = 158;
					break;
				case 247:
					num = 159;
					break;
				case 1025:
					num = 179;
					break;
				case 1028:
					num = 180;
					break;
				case 1030:
					num = 182;
					break;
				case 1031:
					num = 183;
					break;
				case 1040:
					num = 225;
					break;
				case 1041:
					num = 226;
					break;
				case 1042:
					num = 247;
					break;
				case 1043:
					num = 231;
					break;
				case 1044:
					num = 228;
					break;
				case 1045:
					num = 229;
					break;
				case 1046:
					num = 246;
					break;
				case 1047:
					num = 250;
					break;
				case 1048:
				case 1049:
				case 1050:
				case 1051:
				case 1052:
				case 1053:
				case 1054:
				case 1055:
					num -= 815;
					break;
				case 1056:
				case 1057:
				case 1058:
				case 1059:
					num -= 814;
					break;
				case 1060:
					num = 230;
					break;
				case 1061:
					num = 232;
					break;
				case 1062:
					num = 227;
					break;
				case 1063:
					num = 254;
					break;
				case 1064:
					num = 251;
					break;
				case 1065:
					num = 253;
					break;
				case 1066:
					num = 255;
					break;
				case 1067:
					num = 249;
					break;
				case 1068:
					num = 248;
					break;
				case 1069:
					num = 252;
					break;
				case 1070:
					num = 224;
					break;
				case 1071:
					num = 241;
					break;
				case 1072:
					num = 193;
					break;
				case 1073:
					num = 194;
					break;
				case 1074:
					num = 215;
					break;
				case 1075:
					num = 199;
					break;
				case 1076:
					num = 196;
					break;
				case 1077:
					num = 197;
					break;
				case 1078:
					num = 214;
					break;
				case 1079:
					num = 218;
					break;
				case 1080:
				case 1081:
				case 1082:
				case 1083:
				case 1084:
				case 1085:
				case 1086:
				case 1087:
					num -= 879;
					break;
				case 1088:
				case 1089:
				case 1090:
				case 1091:
					num -= 878;
					break;
				case 1092:
					num = 198;
					break;
				case 1093:
					num = 200;
					break;
				case 1094:
					num = 195;
					break;
				case 1095:
					num = 222;
					break;
				case 1096:
					num = 219;
					break;
				case 1097:
					num = 221;
					break;
				case 1098:
					num = 223;
					break;
				case 1099:
					num = 217;
					break;
				case 1100:
					num = 216;
					break;
				case 1101:
					num = 220;
					break;
				case 1102:
					num = 192;
					break;
				case 1103:
					num = 209;
					break;
				case 1105:
					num = 163;
					break;
				case 1108:
					num = 164;
					break;
				case 1110:
					num = 166;
					break;
				case 1111:
					num = 167;
					break;
				case 1168:
					num = 189;
					break;
				case 1169:
					num = 173;
					break;
				case 8729:
					num = 149;
					break;
				case 8730:
					num = 150;
					break;
				case 8776:
					num = 151;
					break;
				case 8804:
					num = 152;
					break;
				case 8805:
					num = 153;
					break;
				case 8992:
					num = 147;
					break;
				case 8993:
					num = 155;
					break;
				case 9472:
					num = 128;
					break;
				case 9474:
					num = 129;
					break;
				case 9484:
					num = 130;
					break;
				case 9488:
					num = 131;
					break;
				case 9492:
					num = 132;
					break;
				case 9496:
					num = 133;
					break;
				case 9500:
					num = 134;
					break;
				case 9508:
					num = 135;
					break;
				case 9516:
					num = 136;
					break;
				case 9524:
					num = 137;
					break;
				case 9532:
					num = 138;
					break;
				case 9552:
					num = 160;
					break;
				case 9553:
					num = 161;
					break;
				case 9554:
					num = 162;
					break;
				case 9556:
					num = 165;
					break;
				case 9559:
				case 9560:
				case 9561:
				case 9562:
				case 9563:
					num -= 9391;
					break;
				case 9565:
				case 9566:
				case 9567:
				case 9568:
				case 9569:
					num -= 9391;
					break;
				case 9571:
					num = 181;
					break;
				case 9574:
				case 9575:
				case 9576:
				case 9577:
				case 9578:
					num -= 9390;
					break;
				case 9580:
					num = 190;
					break;
				case 9600:
					num = 139;
					break;
				case 9604:
					num = 140;
					break;
				case 9608:
					num = 141;
					break;
				case 9612:
					num = 142;
					break;
				case 9616:
				case 9617:
				case 9618:
				case 9619:
					num -= 9473;
					break;
				case 9632:
					num = 148;
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
			}
			bytes[byteIndex++] = (byte)num;
			charCount--;
			byteCount--;
		}
	}
}
