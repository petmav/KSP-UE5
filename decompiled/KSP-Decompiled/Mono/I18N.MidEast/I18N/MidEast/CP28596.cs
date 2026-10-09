using System;
using System.Text;
using I18N.Common;

namespace I18N.MidEast;

[Serializable]
public class CP28596 : ByteEncoding
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
		'\u00a0', '\uf7c8', '\uf7c9', '\uf7ca', '¤', '\uf7cb', '\uf7cc', '\uf7cd', '\uf7ce', '\uf7cf',
		'\uf7d0', '\uf7d1', '،', '\u00ad', '\uf7d2', '\uf7d3', '\uf7d4', '\uf7d5', '\uf7d6', '\uf7d7',
		'\uf7d8', '\uf7d9', '\uf7da', '\uf7db', '\uf7dc', '\uf7dd', '\uf7de', '؛', '\uf7df', '\uf7e0',
		'\uf7e1', '؟', '\uf7e2', 'ء', 'آ', 'أ', 'ؤ', 'إ', 'ئ', 'ا',
		'ب', 'ة', 'ت', 'ث', 'ج', 'ح', 'خ', 'د', 'ذ', 'ر',
		'ز', 'س', 'ش', 'ص', 'ض', 'ط', 'ظ', 'ع', 'غ', '\uf7e3',
		'\uf7e4', '\uf7e5', '\uf7e6', '\uf7e7', 'ـ', 'ف', 'ق', 'ك', 'ل', 'م',
		'ن', 'ه', 'و', 'ى', 'ي', '\u064b', '\u064c', '\u064d', '\u064e', '\u064f',
		'\u0650', '\u0651', '\u0652', '\uf7e8', '\uf7e9', '\uf7ea', '\uf7eb', '\uf7ec', '\uf7ed', '\uf7ee',
		'\uf7ef', '\uf7f0', '\uf7f1', '\uf7f2', '\uf7f3', '\uf7f4'
	};

	public CP28596()
		: base(28596, ToChars, "Arabic (ISO)", "iso-8859-6", "iso-8859-6", "iso-8859-6", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 1256)
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
				case 161:
					num = 33;
					break;
				case 162:
					num = 99;
					break;
				case 165:
					num = 89;
					break;
				case 166:
					num = 124;
					break;
				case 169:
					num = 67;
					break;
				case 170:
					num = 97;
					break;
				case 171:
					num = 60;
					break;
				case 174:
					num = 82;
					break;
				case 178:
					num = 50;
					break;
				case 179:
					num = 51;
					break;
				case 183:
					num = 46;
					break;
				case 184:
					num = 44;
					break;
				case 185:
					num = 49;
					break;
				case 186:
					num = 111;
					break;
				case 187:
					num = 62;
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
					num = 65;
					break;
				case 197:
					num = 65;
					break;
				case 198:
					num = 65;
					break;
				case 199:
					num = 67;
					break;
				case 200:
					num = 69;
					break;
				case 201:
					num = 69;
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
					num = 78;
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
					num = 79;
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
					num = 85;
					break;
				case 221:
					num = 89;
					break;
				case 224:
					num = 97;
					break;
				case 225:
					num = 97;
					break;
				case 226:
					num = 97;
					break;
				case 227:
					num = 97;
					break;
				case 228:
					num = 97;
					break;
				case 229:
					num = 97;
					break;
				case 230:
					num = 97;
					break;
				case 231:
					num = 99;
					break;
				case 232:
					num = 101;
					break;
				case 233:
					num = 101;
					break;
				case 234:
					num = 101;
					break;
				case 235:
					num = 101;
					break;
				case 236:
					num = 105;
					break;
				case 237:
					num = 105;
					break;
				case 238:
					num = 105;
					break;
				case 239:
					num = 105;
					break;
				case 241:
					num = 110;
					break;
				case 242:
					num = 111;
					break;
				case 243:
					num = 111;
					break;
				case 244:
					num = 111;
					break;
				case 245:
					num = 111;
					break;
				case 246:
					num = 111;
					break;
				case 248:
					num = 111;
					break;
				case 249:
					num = 117;
					break;
				case 250:
					num = 117;
					break;
				case 251:
					num = 117;
					break;
				case 252:
					num = 117;
					break;
				case 253:
					num = 121;
					break;
				case 255:
					num = 121;
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
					num = 70;
					break;
				case 402:
					num = 102;
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
				case 715:
					num = 96;
					break;
				case 717:
					num = 95;
					break;
				case 732:
					num = 126;
					break;
				case 768:
					num = 96;
					break;
				case 770:
					num = 94;
					break;
				case 771:
					num = 126;
					break;
				case 782:
					num = 34;
					break;
				case 817:
					num = 95;
					break;
				case 818:
					num = 95;
					break;
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
				case 8216:
					num = 39;
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
					num = 34;
					break;
				case 8226:
					num = 46;
					break;
				case 8230:
					num = 46;
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
				case 8482:
					num = 84;
					break;
				case 63432:
					num = 161;
					break;
				case 63433:
					num = 162;
					break;
				case 63434:
					num = 163;
					break;
				case 63435:
				case 63436:
				case 63437:
				case 63438:
				case 63439:
				case 63440:
				case 63441:
					num -= 63270;
					break;
				case 63442:
				case 63443:
				case 63444:
				case 63445:
				case 63446:
				case 63447:
				case 63448:
				case 63449:
				case 63450:
				case 63451:
				case 63452:
				case 63453:
				case 63454:
					num -= 63268;
					break;
				case 63455:
					num = 188;
					break;
				case 63456:
					num = 189;
					break;
				case 63457:
					num = 190;
					break;
				case 63458:
					num = 192;
					break;
				case 63459:
				case 63460:
				case 63461:
				case 63462:
				case 63463:
					num -= 63240;
					break;
				case 63464:
				case 63465:
				case 63466:
				case 63467:
				case 63468:
				case 63469:
				case 63470:
				case 63471:
				case 63472:
				case 63473:
				case 63474:
				case 63475:
				case 63476:
					num -= 63221;
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
