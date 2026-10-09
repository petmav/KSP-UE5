using System;
using System.Text;
using I18N.Common;

namespace I18N.Other;

[Serializable]
public class CP1251 : ByteEncoding
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
		'x', 'y', 'z', '{', '|', '}', '~', '\u007f', 'Ђ', 'Ѓ',
		'‚', 'ѓ', '„', '…', '†', '‡', '€', '‰', 'Љ', '‹',
		'Њ', 'Ќ', 'Ћ', 'Џ', 'ђ', '‘', '’', '“', '”', '•',
		'–', '—', '\u0098', '™', 'љ', '›', 'њ', 'ќ', 'ћ', 'џ',
		'\u00a0', 'Ў', 'ў', 'Ј', '¤', 'Ґ', '¦', '§', 'Ё', '©',
		'Є', '«', '¬', '\u00ad', '®', 'Ї', '°', '±', 'І', 'і',
		'ґ', 'µ', '¶', '·', 'ё', '№', 'є', '»', 'ј', 'Ѕ',
		'ѕ', 'ї', 'А', 'Б', 'В', 'Г', 'Д', 'Е', 'Ж', 'З',
		'И', 'Й', 'К', 'Л', 'М', 'Н', 'О', 'П', 'Р', 'С',
		'Т', 'У', 'Ф', 'Х', 'Ц', 'Ч', 'Ш', 'Щ', 'Ъ', 'Ы',
		'Ь', 'Э', 'Ю', 'Я', 'а', 'б', 'в', 'г', 'д', 'е',
		'ж', 'з', 'и', 'й', 'к', 'л', 'м', 'н', 'о', 'п',
		'р', 'с', 'т', 'у', 'ф', 'х', 'ц', 'ч', 'ш', 'щ',
		'ъ', 'ы', 'ь', 'э', 'ю', 'я'
	};

	public CP1251()
		: base(1251, ToChars, "Cyrillic (Windows)", "koi8-r", "windows-1251", "windows-1251", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 1251)
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
				case 1025:
					num = 168;
					break;
				case 1026:
					num = 128;
					break;
				case 1027:
					num = 129;
					break;
				case 1028:
					num = 170;
					break;
				case 1029:
					num = 189;
					break;
				case 1030:
					num = 178;
					break;
				case 1031:
					num = 175;
					break;
				case 1032:
					num = 163;
					break;
				case 1033:
					num = 138;
					break;
				case 1034:
					num = 140;
					break;
				case 1035:
					num = 142;
					break;
				case 1036:
					num = 141;
					break;
				case 1038:
					num = 161;
					break;
				case 1039:
					num = 143;
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
					num -= 848;
					break;
				case 1105:
					num = 184;
					break;
				case 1106:
					num = 144;
					break;
				case 1107:
					num = 131;
					break;
				case 1108:
					num = 186;
					break;
				case 1109:
					num = 190;
					break;
				case 1110:
					num = 179;
					break;
				case 1111:
					num = 191;
					break;
				case 1112:
					num = 188;
					break;
				case 1113:
					num = 154;
					break;
				case 1114:
					num = 156;
					break;
				case 1115:
					num = 158;
					break;
				case 1116:
					num = 157;
					break;
				case 1118:
					num = 162;
					break;
				case 1119:
					num = 159;
					break;
				case 1168:
					num = 165;
					break;
				case 1169:
					num = 180;
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
					num = 136;
					break;
				case 8470:
					num = 185;
					break;
				case 8482:
					num = 153;
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
				case 152:
				case 160:
				case 164:
				case 166:
				case 167:
				case 169:
				case 171:
				case 172:
				case 173:
				case 174:
				case 176:
				case 177:
				case 181:
				case 182:
				case 183:
				case 187:
					break;
				}
			}
			bytes[byteIndex++] = (byte)num;
			charCount--;
			byteCount--;
		}
	}
}
