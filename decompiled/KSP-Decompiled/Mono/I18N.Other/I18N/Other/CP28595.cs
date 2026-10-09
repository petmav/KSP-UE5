using System;
using System.Text;
using I18N.Common;

namespace I18N.Other;

[Serializable]
public class CP28595 : ByteEncoding
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
		'\u00a0', 'Ё', 'Ђ', 'Ѓ', 'Є', 'Ѕ', 'І', 'Ї', 'Ј', 'Љ',
		'Њ', 'Ћ', 'Ќ', '\u00ad', 'Ў', 'Џ', 'А', 'Б', 'В', 'Г',
		'Д', 'Е', 'Ж', 'З', 'И', 'Й', 'К', 'Л', 'М', 'Н',
		'О', 'П', 'Р', 'С', 'Т', 'У', 'Ф', 'Х', 'Ц', 'Ч',
		'Ш', 'Щ', 'Ъ', 'Ы', 'Ь', 'Э', 'Ю', 'Я', 'а', 'б',
		'в', 'г', 'д', 'е', 'ж', 'з', 'и', 'й', 'к', 'л',
		'м', 'н', 'о', 'п', 'р', 'с', 'т', 'у', 'ф', 'х',
		'ц', 'ч', 'ш', 'щ', 'ъ', 'ы', 'ь', 'э', 'ю', 'я',
		'№', 'ё', 'ђ', 'ѓ', 'є', 'ѕ', 'і', 'ї', 'ј', 'љ',
		'њ', 'ћ', 'ќ', '§', 'ў', 'џ'
	};

	public CP28595()
		: base(28595, ToChars, "Cyrillic (ISO)", "iso-8859-5", "iso-8859-5", "iso-8859-5", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 1251)
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
				case 167:
					num = 253;
					break;
				case 1025:
				case 1026:
				case 1027:
				case 1028:
				case 1029:
				case 1030:
				case 1031:
				case 1032:
				case 1033:
				case 1034:
				case 1035:
				case 1036:
					num -= 864;
					break;
				case 1038:
				case 1039:
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
					num -= 864;
					break;
				case 1105:
				case 1106:
				case 1107:
				case 1108:
				case 1109:
				case 1110:
				case 1111:
				case 1112:
				case 1113:
				case 1114:
				case 1115:
				case 1116:
					num -= 864;
					break;
				case 1118:
					num = 254;
					break;
				case 1119:
					num = 255;
					break;
				case 8470:
					num = 240;
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
