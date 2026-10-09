using System;
using System.Text;
using I18N.Common;

namespace I18N.Rare;

[Serializable]
public class CP870 : ByteEncoding
{
	private static readonly char[] ToChars = new char[256]
	{
		'\0', '\u0001', '\u0002', '\u0003', '\u009c', '\t', '\u0086', '\u007f', '\u0097', '\u008d',
		'\u008e', '\v', '\f', '\r', '\u000e', '\u000f', '\u0010', '\u0011', '\u0012', '\u0013',
		'\u009d', '\u0085', '\b', '\u0087', '\u0018', '\u0019', '\u0092', '\u008f', '\u001c', '\u001d',
		'\u001e', '\u001f', '\u0080', '\u0081', '\u0082', '\u0083', '\u0084', '\n', '\u0017', '\u001b',
		'\u0088', '\u0089', '\u008a', '\u008b', '\u008c', '\u0005', '\u0006', '\a', '\u0090', '\u0091',
		'\u0016', '\u0093', '\u0094', '\u0095', '\u0096', '\u0004', '\u0098', '\u0099', '\u009a', '\u009b',
		'\u0014', '\u0015', '\u009e', '\u001a', ' ', '\u00a0', 'â', 'ä', 'ţ', 'á',
		'ă', 'č', 'ç', 'ć', '[', '.', '<', '(', '+', '!',
		'&', 'é', 'ę', 'ë', 'ů', 'í', 'î', 'ľ', 'ĺ', 'ß',
		']', '$', '*', ')', ';', '^', '-', '/', 'Â', 'Ä',
		'\u02dd', 'Á', 'Ă', 'Č', 'Ç', 'Ć', '|', ',', '%', '_',
		'>', '?', 'ˇ', 'É', 'Ę', 'Ë', 'Ů', 'Í', 'Î', 'Ľ',
		'Ĺ', '`', ':', '#', '@', '\'', '=', '"', '\u02d8', 'a',
		'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'ś', 'ň',
		'đ', 'ý', 'ř', 'ş', '°', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 'ł', 'ń', 'š', '\u00b8', '\u02db', '¤',
		'ą', '~', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'Ś', 'Ň', 'Đ', 'Ý', 'Ř', 'Ş', '\u02d9', 'Ą', 'ż', 'Ţ',
		'Ż', '§', 'ž', 'ź', 'Ž', 'Ź', 'Ł', 'Ń', 'Š', '\u00a8',
		'\u00b4', '×', '{', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
		'H', 'I', '\u00ad', 'ô', 'ö', 'ŕ', 'ó', 'ő', '}', 'J',
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'Ě', 'ű',
		'ü', 'ť', 'ú', 'ě', '\\', '÷', 'S', 'T', 'U', 'V',
		'W', 'X', 'Y', 'Z', 'ď', 'Ô', 'Ö', 'Ŕ', 'Ó', 'Ő',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'Ď', 'Ű', 'Ü', 'Ť', 'Ú', '\u009f'
	};

	public CP870()
		: base(870, ToChars, "IBM EBCDIC (Latin 2)", "ibm870", "ibm870", "ibm870", isBrowserDisplay: false, isBrowserSave: false, isMailNewsDisplay: false, isMailNewsSave: false, 1250)
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
			if (num >= 4)
			{
				switch (num)
				{
				case 4:
					num = 55;
					break;
				case 5:
					num = 45;
					break;
				case 6:
					num = 46;
					break;
				case 7:
					num = 47;
					break;
				case 8:
					num = 22;
					break;
				case 9:
					num = 5;
					break;
				case 10:
					num = 37;
					break;
				case 20:
					num = 60;
					break;
				case 21:
					num = 61;
					break;
				case 22:
					num = 50;
					break;
				case 23:
					num = 38;
					break;
				case 26:
					num = 63;
					break;
				case 27:
					num = 39;
					break;
				case 32:
					num = 64;
					break;
				case 33:
					num = 79;
					break;
				case 34:
					num = 127;
					break;
				case 35:
					num = 123;
					break;
				case 36:
					num = 91;
					break;
				case 37:
					num = 108;
					break;
				case 38:
					num = 80;
					break;
				case 39:
					num = 125;
					break;
				case 40:
					num = 77;
					break;
				case 41:
					num = 93;
					break;
				case 42:
					num = 92;
					break;
				case 43:
					num = 78;
					break;
				case 44:
					num = 107;
					break;
				case 45:
					num = 96;
					break;
				case 46:
					num = 75;
					break;
				case 47:
					num = 97;
					break;
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
					num += 192;
					break;
				case 58:
					num = 122;
					break;
				case 59:
					num = 94;
					break;
				case 60:
					num = 76;
					break;
				case 61:
					num = 126;
					break;
				case 62:
					num = 110;
					break;
				case 63:
					num = 111;
					break;
				case 64:
					num = 124;
					break;
				case 65:
				case 66:
				case 67:
				case 68:
				case 69:
				case 70:
				case 71:
				case 72:
				case 73:
					num += 128;
					break;
				case 74:
				case 75:
				case 76:
				case 77:
				case 78:
				case 79:
				case 80:
				case 81:
				case 82:
					num += 135;
					break;
				case 83:
				case 84:
				case 85:
				case 86:
				case 87:
				case 88:
				case 89:
				case 90:
					num += 143;
					break;
				case 91:
					num = 74;
					break;
				case 92:
					num = 224;
					break;
				case 93:
					num = 90;
					break;
				case 94:
					num = 95;
					break;
				case 95:
					num = 109;
					break;
				case 96:
					num = 121;
					break;
				case 97:
				case 98:
				case 99:
				case 100:
				case 101:
				case 102:
				case 103:
				case 104:
				case 105:
					num += 32;
					break;
				case 106:
				case 107:
				case 108:
				case 109:
				case 110:
				case 111:
				case 112:
				case 113:
				case 114:
					num += 39;
					break;
				case 115:
				case 116:
				case 117:
				case 118:
				case 119:
				case 120:
				case 121:
				case 122:
					num += 47;
					break;
				case 123:
					num = 192;
					break;
				case 124:
					num = 106;
					break;
				case 125:
					num = 208;
					break;
				case 126:
					num = 161;
					break;
				case 127:
					num = 7;
					break;
				case 128:
				case 129:
				case 130:
				case 131:
				case 132:
					num -= 96;
					break;
				case 133:
					num = 21;
					break;
				case 134:
					num = 6;
					break;
				case 135:
					num = 23;
					break;
				case 136:
				case 137:
				case 138:
				case 139:
				case 140:
					num -= 96;
					break;
				case 141:
					num = 9;
					break;
				case 142:
					num = 10;
					break;
				case 143:
					num = 27;
					break;
				case 144:
					num = 48;
					break;
				case 145:
					num = 49;
					break;
				case 146:
					num = 26;
					break;
				case 147:
				case 148:
				case 149:
				case 150:
					num -= 96;
					break;
				case 151:
					num = 8;
					break;
				case 152:
				case 153:
				case 154:
				case 155:
					num -= 96;
					break;
				case 156:
					num = 4;
					break;
				case 157:
					num = 20;
					break;
				case 158:
					num = 62;
					break;
				case 159:
					num = 255;
					break;
				case 160:
					num = 65;
					break;
				case 164:
					num = 159;
					break;
				case 167:
					num = 181;
					break;
				case 168:
					num = 189;
					break;
				case 173:
					num = 202;
					break;
				case 176:
					num = 144;
					break;
				case 180:
					num = 190;
					break;
				case 184:
					num = 157;
					break;
				case 193:
					num = 101;
					break;
				case 194:
					num = 98;
					break;
				case 196:
					num = 99;
					break;
				case 199:
					num = 104;
					break;
				case 201:
					num = 113;
					break;
				case 203:
					num = 115;
					break;
				case 205:
					num = 117;
					break;
				case 206:
					num = 118;
					break;
				case 208:
					num = 172;
					break;
				case 211:
					num = 238;
					break;
				case 212:
					num = 235;
					break;
				case 214:
					num = 236;
					break;
				case 215:
					num = 191;
					break;
				case 218:
					num = 254;
					break;
				case 220:
					num = 252;
					break;
				case 221:
					num = 173;
					break;
				case 223:
					num = 89;
					break;
				case 225:
					num = 69;
					break;
				case 226:
					num = 66;
					break;
				case 228:
					num = 67;
					break;
				case 231:
					num = 72;
					break;
				case 233:
					num = 81;
					break;
				case 235:
					num = 83;
					break;
				case 237:
					num = 85;
					break;
				case 238:
					num = 86;
					break;
				case 243:
					num = 206;
					break;
				case 244:
					num = 203;
					break;
				case 246:
					num = 204;
					break;
				case 247:
					num = 225;
					break;
				case 250:
					num = 222;
					break;
				case 252:
					num = 220;
					break;
				case 253:
					num = 141;
					break;
				case 258:
					num = 102;
					break;
				case 259:
					num = 70;
					break;
				case 260:
					num = 177;
					break;
				case 261:
					num = 160;
					break;
				case 262:
					num = 105;
					break;
				case 263:
					num = 73;
					break;
				case 268:
					num = 103;
					break;
				case 269:
					num = 71;
					break;
				case 270:
					num = 250;
					break;
				case 271:
					num = 234;
					break;
				case 272:
					num = 172;
					break;
				case 273:
					num = 140;
					break;
				case 280:
					num = 114;
					break;
				case 281:
					num = 82;
					break;
				case 282:
					num = 218;
					break;
				case 283:
					num = 223;
					break;
				case 313:
					num = 120;
					break;
				case 314:
					num = 88;
					break;
				case 317:
					num = 119;
					break;
				case 318:
					num = 87;
					break;
				case 321:
					num = 186;
					break;
				case 322:
					num = 154;
					break;
				case 323:
					num = 187;
					break;
				case 324:
					num = 155;
					break;
				case 327:
					num = 171;
					break;
				case 328:
					num = 139;
					break;
				case 336:
					num = 239;
					break;
				case 337:
					num = 207;
					break;
				case 340:
					num = 237;
					break;
				case 341:
					num = 205;
					break;
				case 344:
					num = 174;
					break;
				case 345:
					num = 142;
					break;
				case 346:
					num = 170;
					break;
				case 347:
					num = 138;
					break;
				case 350:
					num = 175;
					break;
				case 351:
					num = 143;
					break;
				case 352:
					num = 188;
					break;
				case 353:
					num = 156;
					break;
				case 354:
					num = 179;
					break;
				case 355:
					num = 68;
					break;
				case 356:
					num = 253;
					break;
				case 357:
					num = 221;
					break;
				case 366:
					num = 116;
					break;
				case 367:
					num = 84;
					break;
				case 368:
					num = 251;
					break;
				case 369:
					num = 219;
					break;
				case 377:
					num = 185;
					break;
				case 378:
					num = 183;
					break;
				case 379:
					num = 180;
					break;
				case 380:
					num = 178;
					break;
				case 381:
					num = 184;
					break;
				case 382:
					num = 182;
					break;
				case 711:
					num = 112;
					break;
				case 728:
					num = 128;
					break;
				case 729:
					num = 176;
					break;
				case 731:
					num = 158;
					break;
				case 733:
					num = 100;
					break;
				case 65281:
					num = 79;
					break;
				case 65282:
					num = 127;
					break;
				case 65283:
					num = 123;
					break;
				case 65284:
					num = 91;
					break;
				case 65285:
					num = 108;
					break;
				case 65286:
					num = 80;
					break;
				case 65287:
					num = 125;
					break;
				case 65288:
					num = 77;
					break;
				case 65289:
					num = 93;
					break;
				case 65290:
					num = 92;
					break;
				case 65291:
					num = 78;
					break;
				case 65292:
					num = 107;
					break;
				case 65293:
					num = 96;
					break;
				case 65294:
					num = 75;
					break;
				case 65295:
					num = 97;
					break;
				case 65296:
				case 65297:
				case 65298:
				case 65299:
				case 65300:
				case 65301:
				case 65302:
				case 65303:
				case 65304:
				case 65305:
					num -= 65056;
					break;
				case 65306:
					num = 122;
					break;
				case 65307:
					num = 94;
					break;
				case 65308:
					num = 76;
					break;
				case 65309:
					num = 126;
					break;
				case 65310:
					num = 110;
					break;
				case 65311:
					num = 111;
					break;
				case 65312:
					num = 124;
					break;
				case 65313:
				case 65314:
				case 65315:
				case 65316:
				case 65317:
				case 65318:
				case 65319:
				case 65320:
				case 65321:
					num -= 65120;
					break;
				case 65322:
				case 65323:
				case 65324:
				case 65325:
				case 65326:
				case 65327:
				case 65328:
				case 65329:
				case 65330:
					num -= 65113;
					break;
				case 65331:
				case 65332:
				case 65333:
				case 65334:
				case 65335:
				case 65336:
				case 65337:
				case 65338:
					num -= 65105;
					break;
				case 65339:
					num = 74;
					break;
				case 65340:
					num = 224;
					break;
				case 65341:
					num = 90;
					break;
				case 65342:
					num = 95;
					break;
				case 65343:
					num = 109;
					break;
				case 65344:
					num = 121;
					break;
				case 65345:
				case 65346:
				case 65347:
				case 65348:
				case 65349:
				case 65350:
				case 65351:
				case 65352:
				case 65353:
					num -= 65216;
					break;
				case 65354:
				case 65355:
				case 65356:
				case 65357:
				case 65358:
				case 65359:
				case 65360:
				case 65361:
				case 65362:
					num -= 65209;
					break;
				case 65363:
				case 65364:
				case 65365:
				case 65366:
				case 65367:
				case 65368:
				case 65369:
				case 65370:
					num -= 65201;
					break;
				case 65371:
					num = 192;
					break;
				case 65372:
					num = 106;
					break;
				case 65373:
					num = 208;
					break;
				case 65374:
					num = 161;
					break;
				default:
					HandleFallback(ref buffer, chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
					break;
				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				case 17:
				case 18:
				case 19:
				case 24:
				case 25:
				case 28:
				case 29:
				case 30:
				case 31:
					break;
				}
			}
			bytes[byteIndex++] = (byte)num;
			charCount--;
			byteCount--;
		}
	}
}
