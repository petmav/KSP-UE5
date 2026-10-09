using System;
using System.Text;
using I18N.Common;

namespace I18N.West;

[Serializable]
public class CP437 : ByteEncoding
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
		'x', 'y', 'z', '{', '|', '}', '~', '\u007f', 'Ç', 'ü',
		'é', 'â', 'ä', 'à', 'å', 'ç', 'ê', 'ë', 'è', 'ï',
		'î', 'ì', 'Ä', 'Å', 'É', 'æ', 'Æ', 'ô', 'ö', 'ò',
		'û', 'ù', 'ÿ', 'Ö', 'Ü', '¢', '£', '¥', '₧', 'ƒ',
		'á', 'í', 'ó', 'ú', 'ñ', 'Ñ', 'ª', 'º', '¿', '⌐',
		'¬', '½', '¼', '¡', '«', '»', '░', '▒', '▓', '│',
		'┤', '╡', '╢', '╖', '╕', '╣', '║', '╗', '╝', '╜',
		'╛', '┐', '└', '┴', '┬', '├', '─', '┼', '╞', '╟',
		'╚', '╔', '╩', '╦', '╠', '═', '╬', '╧', '╨', '╤',
		'╥', '╙', '╘', '╒', '╓', '╫', '╪', '┘', '┌', '█',
		'▄', '▌', '▐', '▀', 'α', 'ß', 'Γ', 'π', 'Σ', 'σ',
		'µ', 'τ', 'Φ', 'Θ', 'Ω', 'δ', '∞', 'φ', 'ε', '∩',
		'≡', '±', '≥', '≤', '⌠', '⌡', '÷', '≈', '°', '∙',
		'·', '√', 'ⁿ', '²', '■', '\u00a0'
	};

	public CP437()
		: base(437, ToChars, "OEM United States", "IBM437", "IBM437", "IBM437", isBrowserDisplay: false, isBrowserSave: false, isMailNewsDisplay: false, isMailNewsSave: false, 1252)
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
					num = 255;
					break;
				case 161:
					num = 173;
					break;
				case 162:
					num = 155;
					break;
				case 163:
					num = 156;
					break;
				case 164:
					num = 15;
					break;
				case 165:
					num = 157;
					break;
				case 166:
					num = 221;
					break;
				case 167:
					num = 21;
					break;
				case 168:
					num = 34;
					break;
				case 169:
					num = 99;
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
					num = 45;
					break;
				case 174:
					num = 114;
					break;
				case 175:
					num = 95;
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
					num = 51;
					break;
				case 180:
					num = 39;
					break;
				case 181:
					num = 230;
					break;
				case 182:
					num = 20;
					break;
				case 183:
					num = 250;
					break;
				case 184:
					num = 44;
					break;
				case 185:
					num = 49;
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
					num = 95;
					break;
				case 191:
					num = 168;
					break;
				case 192:
					num = 65;
					break;
				case 193:
					num = 65;
					break;
				case 194:
					num = 65;
					break;
				case 195:
					num = 65;
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
					num = 69;
					break;
				case 201:
					num = 144;
					break;
				case 202:
					num = 69;
					break;
				case 203:
					num = 69;
					break;
				case 204:
					num = 73;
					break;
				case 205:
					num = 73;
					break;
				case 206:
					num = 73;
					break;
				case 207:
					num = 73;
					break;
				case 208:
					num = 68;
					break;
				case 209:
					num = 165;
					break;
				case 210:
					num = 79;
					break;
				case 211:
					num = 79;
					break;
				case 212:
					num = 79;
					break;
				case 213:
					num = 79;
					break;
				case 214:
					num = 153;
					break;
				case 215:
					num = 120;
					break;
				case 216:
					num = 79;
					break;
				case 217:
					num = 85;
					break;
				case 218:
					num = 85;
					break;
				case 219:
					num = 85;
					break;
				case 220:
					num = 154;
					break;
				case 221:
					num = 89;
					break;
				case 222:
					num = 95;
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
					num = 97;
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
					num = 100;
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
					num = 111;
					break;
				case 246:
					num = 148;
					break;
				case 247:
					num = 246;
					break;
				case 248:
					num = 111;
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
					num = 121;
					break;
				case 254:
					num = 95;
					break;
				case 255:
					num = 152;
					break;
				case 256:
					num = 65;
					break;
				case 257:
					num = 97;
					break;
				case 258:
					num = 65;
					break;
				case 259:
					num = 97;
					break;
				case 260:
					num = 65;
					break;
				case 261:
					num = 97;
					break;
				case 262:
					num = 67;
					break;
				case 263:
					num = 99;
					break;
				case 264:
					num = 67;
					break;
				case 265:
					num = 99;
					break;
				case 266:
					num = 67;
					break;
				case 267:
					num = 99;
					break;
				case 268:
					num = 67;
					break;
				case 269:
					num = 99;
					break;
				case 270:
					num = 68;
					break;
				case 271:
					num = 100;
					break;
				case 272:
					num = 68;
					break;
				case 273:
					num = 100;
					break;
				case 274:
					num = 69;
					break;
				case 275:
					num = 101;
					break;
				case 276:
					num = 69;
					break;
				case 277:
					num = 101;
					break;
				case 278:
					num = 69;
					break;
				case 279:
					num = 101;
					break;
				case 280:
					num = 69;
					break;
				case 281:
					num = 101;
					break;
				case 282:
					num = 69;
					break;
				case 283:
					num = 101;
					break;
				case 284:
					num = 71;
					break;
				case 285:
					num = 103;
					break;
				case 286:
					num = 71;
					break;
				case 287:
					num = 103;
					break;
				case 288:
					num = 71;
					break;
				case 289:
					num = 103;
					break;
				case 290:
					num = 71;
					break;
				case 291:
					num = 103;
					break;
				case 292:
					num = 72;
					break;
				case 293:
					num = 104;
					break;
				case 294:
					num = 72;
					break;
				case 295:
					num = 104;
					break;
				case 296:
					num = 73;
					break;
				case 297:
					num = 105;
					break;
				case 298:
					num = 73;
					break;
				case 299:
					num = 105;
					break;
				case 300:
					num = 73;
					break;
				case 301:
					num = 105;
					break;
				case 302:
					num = 73;
					break;
				case 303:
					num = 105;
					break;
				case 304:
					num = 73;
					break;
				case 305:
					num = 105;
					break;
				case 308:
					num = 74;
					break;
				case 309:
					num = 106;
					break;
				case 310:
					num = 75;
					break;
				case 311:
					num = 107;
					break;
				case 313:
					num = 76;
					break;
				case 314:
					num = 108;
					break;
				case 315:
					num = 76;
					break;
				case 316:
					num = 108;
					break;
				case 317:
					num = 76;
					break;
				case 318:
					num = 108;
					break;
				case 321:
					num = 76;
					break;
				case 322:
					num = 108;
					break;
				case 323:
					num = 78;
					break;
				case 324:
					num = 110;
					break;
				case 325:
					num = 78;
					break;
				case 326:
					num = 110;
					break;
				case 327:
					num = 78;
					break;
				case 328:
					num = 110;
					break;
				case 332:
					num = 79;
					break;
				case 333:
					num = 111;
					break;
				case 334:
					num = 79;
					break;
				case 335:
					num = 111;
					break;
				case 336:
					num = 79;
					break;
				case 337:
					num = 111;
					break;
				case 338:
					num = 79;
					break;
				case 339:
					num = 111;
					break;
				case 340:
					num = 82;
					break;
				case 341:
					num = 114;
					break;
				case 342:
					num = 82;
					break;
				case 343:
					num = 114;
					break;
				case 344:
					num = 82;
					break;
				case 345:
					num = 114;
					break;
				case 346:
					num = 83;
					break;
				case 347:
					num = 115;
					break;
				case 348:
					num = 83;
					break;
				case 349:
					num = 115;
					break;
				case 350:
					num = 83;
					break;
				case 351:
					num = 115;
					break;
				case 352:
					num = 83;
					break;
				case 353:
					num = 115;
					break;
				case 354:
					num = 84;
					break;
				case 355:
					num = 116;
					break;
				case 356:
					num = 84;
					break;
				case 357:
					num = 116;
					break;
				case 358:
					num = 84;
					break;
				case 359:
					num = 116;
					break;
				case 360:
					num = 85;
					break;
				case 361:
					num = 117;
					break;
				case 362:
					num = 85;
					break;
				case 363:
					num = 117;
					break;
				case 364:
					num = 85;
					break;
				case 365:
					num = 117;
					break;
				case 366:
					num = 85;
					break;
				case 367:
					num = 117;
					break;
				case 368:
					num = 85;
					break;
				case 369:
					num = 117;
					break;
				case 370:
					num = 85;
					break;
				case 371:
					num = 117;
					break;
				case 372:
					num = 87;
					break;
				case 373:
					num = 119;
					break;
				case 374:
					num = 89;
					break;
				case 375:
					num = 121;
					break;
				case 376:
					num = 89;
					break;
				case 377:
					num = 90;
					break;
				case 378:
					num = 122;
					break;
				case 379:
					num = 90;
					break;
				case 380:
					num = 122;
					break;
				case 381:
					num = 90;
					break;
				case 382:
					num = 122;
					break;
				case 384:
					num = 98;
					break;
				case 393:
					num = 68;
					break;
				case 401:
					num = 159;
					break;
				case 402:
					num = 159;
					break;
				case 407:
					num = 73;
					break;
				case 410:
					num = 108;
					break;
				case 415:
					num = 79;
					break;
				case 416:
					num = 79;
					break;
				case 417:
					num = 111;
					break;
				case 425:
					num = 228;
					break;
				case 427:
					num = 116;
					break;
				case 430:
					num = 84;
					break;
				case 431:
					num = 85;
					break;
				case 432:
					num = 117;
					break;
				case 438:
					num = 122;
					break;
				case 448:
					num = 124;
					break;
				case 451:
					num = 33;
					break;
				case 461:
					num = 65;
					break;
				case 462:
					num = 97;
					break;
				case 463:
					num = 73;
					break;
				case 464:
					num = 105;
					break;
				case 465:
					num = 79;
					break;
				case 466:
					num = 111;
					break;
				case 467:
					num = 85;
					break;
				case 468:
					num = 117;
					break;
				case 469:
					num = 85;
					break;
				case 470:
					num = 117;
					break;
				case 471:
					num = 85;
					break;
				case 472:
					num = 117;
					break;
				case 473:
					num = 85;
					break;
				case 474:
					num = 117;
					break;
				case 475:
					num = 85;
					break;
				case 476:
					num = 117;
					break;
				case 478:
					num = 65;
					break;
				case 479:
					num = 97;
					break;
				case 484:
					num = 71;
					break;
				case 485:
					num = 103;
					break;
				case 486:
					num = 71;
					break;
				case 487:
					num = 103;
					break;
				case 488:
					num = 75;
					break;
				case 489:
					num = 107;
					break;
				case 490:
					num = 79;
					break;
				case 491:
					num = 111;
					break;
				case 492:
					num = 79;
					break;
				case 493:
					num = 111;
					break;
				case 496:
					num = 106;
					break;
				case 609:
					num = 103;
					break;
				case 632:
					num = 237;
					break;
				case 697:
					num = 39;
					break;
				case 698:
					num = 34;
					break;
				case 700:
					num = 39;
					break;
				case 708:
					num = 94;
					break;
				case 710:
					num = 94;
					break;
				case 712:
					num = 39;
					break;
				case 713:
					num = 196;
					break;
				case 714:
					num = 39;
					break;
				case 715:
					num = 96;
					break;
				case 717:
					num = 95;
					break;
				case 730:
					num = 248;
					break;
				case 732:
					num = 126;
					break;
				case 768:
					num = 96;
					break;
				case 769:
					num = 39;
					break;
				case 770:
					num = 94;
					break;
				case 771:
					num = 126;
					break;
				case 772:
					num = 196;
					break;
				case 776:
					num = 34;
					break;
				case 778:
					num = 248;
					break;
				case 782:
					num = 34;
					break;
				case 807:
					num = 44;
					break;
				case 817:
					num = 95;
					break;
				case 818:
					num = 95;
					break;
				case 894:
					num = 59;
					break;
				case 913:
					num = 224;
					break;
				case 915:
					num = 226;
					break;
				case 916:
					num = 235;
					break;
				case 917:
					num = 238;
					break;
				case 920:
					num = 233;
					break;
				case 928:
					num = 227;
					break;
				case 931:
					num = 228;
					break;
				case 932:
					num = 231;
					break;
				case 934:
					num = 232;
					break;
				case 937:
					num = 234;
					break;
				case 945:
					num = 224;
					break;
				case 946:
					num = 225;
					break;
				case 948:
					num = 235;
					break;
				case 949:
					num = 238;
					break;
				case 956:
					num = 230;
					break;
				case 960:
					num = 227;
					break;
				case 963:
					num = 229;
					break;
				case 964:
					num = 231;
					break;
				case 966:
					num = 237;
					break;
				case 1211:
					num = 104;
					break;
				case 1417:
					num = 58;
					break;
				case 1642:
					num = 37;
					break;
				case 8192:
					num = 32;
					break;
				case 8193:
					num = 32;
					break;
				case 8194:
					num = 32;
					break;
				case 8195:
					num = 32;
					break;
				case 8196:
					num = 32;
					break;
				case 8197:
					num = 32;
					break;
				case 8198:
					num = 32;
					break;
				case 8208:
					num = 45;
					break;
				case 8209:
					num = 45;
					break;
				case 8211:
					num = 45;
					break;
				case 8212:
					num = 45;
					break;
				case 8215:
					num = 95;
					break;
				case 8216:
					num = 96;
					break;
				case 8217:
					num = 39;
					break;
				case 8218:
					num = 44;
					break;
				case 8220:
					num = 34;
					break;
				case 8221:
					num = 34;
					break;
				case 8222:
					num = 44;
					break;
				case 8224:
					num = 43;
					break;
				case 8225:
					num = 216;
					break;
				case 8226:
					num = 7;
					break;
				case 8228:
					num = 250;
					break;
				case 8230:
					num = 46;
					break;
				case 8240:
					num = 37;
					break;
				case 8242:
					num = 39;
					break;
				case 8245:
					num = 96;
					break;
				case 8249:
					num = 60;
					break;
				case 8250:
					num = 62;
					break;
				case 8252:
					num = 19;
					break;
				case 8260:
					num = 47;
					break;
				case 8304:
					num = 248;
					break;
				case 8308:
				case 8309:
				case 8310:
				case 8311:
				case 8312:
					num -= 8256;
					break;
				case 8319:
					num = 252;
					break;
				case 8320:
				case 8321:
				case 8322:
				case 8323:
				case 8324:
				case 8325:
				case 8326:
				case 8327:
				case 8328:
				case 8329:
					num -= 8272;
					break;
				case 8356:
					num = 156;
					break;
				case 8359:
					num = 158;
					break;
				case 8413:
					num = 9;
					break;
				case 8450:
					num = 67;
					break;
				case 8455:
					num = 69;
					break;
				case 8458:
					num = 103;
					break;
				case 8459:
					num = 72;
					break;
				case 8460:
					num = 72;
					break;
				case 8461:
					num = 72;
					break;
				case 8462:
					num = 104;
					break;
				case 8464:
					num = 73;
					break;
				case 8465:
					num = 73;
					break;
				case 8466:
					num = 76;
					break;
				case 8467:
					num = 108;
					break;
				case 8469:
					num = 78;
					break;
				case 8472:
					num = 80;
					break;
				case 8473:
					num = 80;
					break;
				case 8474:
					num = 81;
					break;
				case 8475:
					num = 82;
					break;
				case 8476:
					num = 82;
					break;
				case 8477:
					num = 82;
					break;
				case 8482:
					num = 84;
					break;
				case 8484:
					num = 90;
					break;
				case 8486:
					num = 234;
					break;
				case 8488:
					num = 90;
					break;
				case 8490:
					num = 75;
					break;
				case 8491:
					num = 143;
					break;
				case 8492:
					num = 66;
					break;
				case 8493:
					num = 67;
					break;
				case 8494:
					num = 101;
					break;
				case 8495:
					num = 101;
					break;
				case 8496:
					num = 69;
					break;
				case 8497:
					num = 70;
					break;
				case 8499:
					num = 77;
					break;
				case 8500:
					num = 111;
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
				case 8709:
					num = 237;
					break;
				case 8721:
					num = 228;
					break;
				case 8722:
					num = 45;
					break;
				case 8723:
					num = 241;
					break;
				case 8725:
					num = 47;
					break;
				case 8726:
					num = 92;
					break;
				case 8727:
					num = 42;
					break;
				case 8728:
					num = 248;
					break;
				case 8729:
					num = 249;
					break;
				case 8730:
					num = 251;
					break;
				case 8734:
					num = 236;
					break;
				case 8735:
					num = 28;
					break;
				case 8739:
					num = 124;
					break;
				case 8745:
					num = 239;
					break;
				case 8758:
					num = 58;
					break;
				case 8764:
					num = 126;
					break;
				case 8776:
					num = 247;
					break;
				case 8801:
					num = 240;
					break;
				case 8804:
					num = 243;
					break;
				case 8805:
					num = 242;
					break;
				case 8810:
					num = 174;
					break;
				case 8811:
					num = 175;
					break;
				case 8901:
					num = 250;
					break;
				case 8962:
					num = 127;
					break;
				case 8963:
					num = 94;
					break;
				case 8976:
					num = 169;
					break;
				case 8992:
					num = 244;
					break;
				case 8993:
					num = 245;
					break;
				case 9001:
					num = 60;
					break;
				case 9002:
					num = 62;
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
				case 10003:
					num = 251;
					break;
				case 10072:
					num = 124;
					break;
				case 12288:
					num = 32;
					break;
				case 12295:
					num = 9;
					break;
				case 12296:
					num = 60;
					break;
				case 12297:
					num = 62;
					break;
				case 12298:
					num = 174;
					break;
				case 12299:
					num = 175;
					break;
				case 12314:
					num = 91;
					break;
				case 12315:
					num = 93;
					break;
				case 12539:
					num = 250;
					break;
				case 65281:
				case 65282:
				case 65283:
				case 65284:
				case 65285:
				case 65286:
				case 65287:
				case 65288:
				case 65289:
				case 65290:
				case 65291:
				case 65292:
				case 65293:
				case 65294:
				case 65295:
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
				case 65306:
				case 65307:
				case 65308:
				case 65309:
				case 65310:
					num -= 65248;
					break;
				case 65312:
				case 65313:
				case 65314:
				case 65315:
				case 65316:
				case 65317:
				case 65318:
				case 65319:
				case 65320:
				case 65321:
				case 65322:
				case 65323:
				case 65324:
				case 65325:
				case 65326:
				case 65327:
				case 65328:
				case 65329:
				case 65330:
				case 65331:
				case 65332:
				case 65333:
				case 65334:
				case 65335:
				case 65336:
				case 65337:
				case 65338:
				case 65339:
				case 65340:
				case 65341:
				case 65342:
				case 65343:
				case 65344:
				case 65345:
				case 65346:
				case 65347:
				case 65348:
				case 65349:
				case 65350:
				case 65351:
				case 65352:
				case 65353:
				case 65354:
				case 65355:
				case 65356:
				case 65357:
				case 65358:
				case 65359:
				case 65360:
				case 65361:
				case 65362:
				case 65363:
				case 65364:
				case 65365:
				case 65366:
				case 65367:
				case 65368:
				case 65369:
				case 65370:
				case 65371:
				case 65372:
				case 65373:
				case 65374:
					num -= 65248;
					break;
				default:
					HandleFallback(ref buffer, chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
					break;
				}
			}
			bytes[byteIndex++] = (byte)num;
			charCount--;
			byteCount--;
		}
	}
}
