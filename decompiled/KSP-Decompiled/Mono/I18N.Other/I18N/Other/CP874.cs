using System;
using System.Text;
using I18N.Common;

namespace I18N.Other;

[Serializable]
public class CP874 : ByteEncoding
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
		'x', 'y', 'z', '{', '|', '}', '~', '\u001a', '?', '?',
		'?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
		'?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
		'?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
		'\u0e48', 'ก', 'ข', 'ฃ', 'ค', 'ฅ', 'ฆ', 'ง', 'จ', 'ฉ',
		'ช', 'ซ', 'ฌ', 'ญ', 'ฎ', 'ฏ', 'ฐ', 'ฑ', 'ฒ', 'ณ',
		'ด', 'ต', 'ถ', 'ท', 'ธ', 'น', 'บ', 'ป', 'ผ', 'ฝ',
		'พ', 'ฟ', 'ภ', 'ม', 'ย', 'ร', 'ฤ', 'ล', 'ฦ', 'ว',
		'ศ', 'ษ', 'ส', 'ห', 'ฬ', 'อ', 'ฮ', 'ฯ', 'ะ', '\u0e31',
		'า', 'ำ', '\u0e34', '\u0e35', '\u0e36', '\u0e37', '\u0e38', '\u0e39', '\u0e3a', '\u0e49',
		'\u0e4a', '\u0e4b', '\u0e4c', '฿', 'เ', 'แ', 'โ', 'ใ', 'ไ', 'ๅ',
		'ๆ', '\u0e47', '\u0e48', '\u0e49', '\u0e4a', '\u0e4b', '\u0e4c', '\u0e4d', '\u0e4e', '๏',
		'๐', '๑', '๒', '๓', '๔', '๕', '๖', '๗', '๘', '๙',
		'๚', '๛', '¢', '¬', '¦', '\u00a0'
	};

	public CP874()
		: base(874, ToChars, "Thai (Windows)", "windows-874", "windows-874", "windows-874", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 874)
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
				case 162:
					num = 252;
					break;
				case 166:
					num = 254;
					break;
				case 172:
					num = 253;
					break;
				case 3585:
				case 3586:
				case 3587:
				case 3588:
				case 3589:
				case 3590:
				case 3591:
				case 3592:
				case 3593:
				case 3594:
				case 3595:
				case 3596:
				case 3597:
				case 3598:
				case 3599:
				case 3600:
				case 3601:
				case 3602:
				case 3603:
				case 3604:
				case 3605:
				case 3606:
				case 3607:
				case 3608:
				case 3609:
				case 3610:
				case 3611:
				case 3612:
				case 3613:
				case 3614:
				case 3615:
				case 3616:
				case 3617:
				case 3618:
				case 3619:
				case 3620:
				case 3621:
				case 3622:
				case 3623:
				case 3624:
				case 3625:
				case 3626:
				case 3627:
				case 3628:
				case 3629:
				case 3630:
				case 3631:
				case 3632:
				case 3633:
				case 3634:
				case 3635:
				case 3636:
				case 3637:
				case 3638:
				case 3639:
				case 3640:
				case 3641:
				case 3642:
					num -= 3424;
					break;
				case 3647:
				case 3648:
				case 3649:
				case 3650:
				case 3651:
				case 3652:
				case 3653:
				case 3654:
				case 3655:
				case 3656:
				case 3657:
				case 3658:
				case 3659:
				case 3660:
				case 3661:
				case 3662:
				case 3663:
				case 3664:
				case 3665:
				case 3666:
				case 3667:
				case 3668:
				case 3669:
				case 3670:
				case 3671:
				case 3672:
				case 3673:
				case 3674:
				case 3675:
					num -= 3424;
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
