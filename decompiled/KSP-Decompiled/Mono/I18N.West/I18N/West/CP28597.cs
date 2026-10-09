using System;
using System.Text;
using I18N.Common;

namespace I18N.West;

[Serializable]
public class CP28597 : ByteEncoding
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
		'\u00a0', '‘', '’', '£', '€', '?', '¦', '§', '\u00a8', '©',
		'?', '«', '¬', '\u00ad', '?', '―', '°', '±', '²', '³',
		'\u00b4', '\u0385', 'Ά', '·', 'Έ', 'Ή', 'Ί', '»', 'Ό', '½',
		'Ύ', 'Ώ', 'ΐ', 'Α', 'Β', 'Γ', 'Δ', 'Ε', 'Ζ', 'Η',
		'Θ', 'Ι', 'Κ', 'Λ', 'Μ', 'Ν', 'Ξ', 'Ο', 'Π', 'Ρ',
		'?', 'Σ', 'Τ', 'Υ', 'Φ', 'Χ', 'Ψ', 'Ω', 'Ϊ', 'Ϋ',
		'ά', 'έ', 'ή', 'ί', 'ΰ', 'α', 'β', 'γ', 'δ', 'ε',
		'ζ', 'η', 'θ', 'ι', 'κ', 'λ', 'μ', 'ν', 'ξ', 'ο',
		'π', 'ρ', 'ς', 'σ', 'τ', 'υ', 'φ', 'χ', 'ψ', 'ω',
		'ϊ', 'ϋ', 'ό', 'ύ', 'ώ', '?'
	};

	public CP28597()
		: base(28597, ToChars, "Greek (ISO)", "iso-8859-7", "iso-8859-7", "iso-8859-7", isBrowserDisplay: true, isBrowserSave: true, isMailNewsDisplay: true, isMailNewsSave: true, 1253)
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
				case 901:
				case 902:
				case 903:
				case 904:
				case 905:
				case 906:
					num -= 720;
					break;
				case 908:
					num = 188;
					break;
				case 910:
				case 911:
				case 912:
				case 913:
				case 914:
				case 915:
				case 916:
				case 917:
				case 918:
				case 919:
				case 920:
				case 921:
				case 922:
				case 923:
				case 924:
				case 925:
				case 926:
				case 927:
				case 928:
				case 929:
					num -= 720;
					break;
				case 931:
				case 932:
				case 933:
				case 934:
				case 935:
				case 936:
				case 937:
				case 938:
				case 939:
				case 940:
				case 941:
				case 942:
				case 943:
				case 944:
				case 945:
				case 946:
				case 947:
				case 948:
				case 949:
				case 950:
				case 951:
				case 952:
				case 953:
				case 954:
				case 955:
				case 956:
				case 957:
				case 958:
				case 959:
				case 960:
				case 961:
				case 962:
				case 963:
				case 964:
				case 965:
				case 966:
				case 967:
				case 968:
				case 969:
				case 970:
				case 971:
				case 972:
				case 973:
				case 974:
					num -= 720;
					break;
				case 981:
					num = 246;
					break;
				case 8213:
					num = 175;
					break;
				case 8216:
					num = 161;
					break;
				case 8217:
					num = 162;
					break;
				case 8364:
					num = 164;
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
				case 163:
				case 166:
				case 167:
				case 168:
				case 169:
				case 171:
				case 172:
				case 173:
				case 176:
				case 177:
				case 178:
				case 179:
				case 180:
				case 183:
				case 187:
				case 189:
					break;
				}
			}
			bytes[byteIndex++] = (byte)num;
			charCount--;
			byteCount--;
		}
	}
}
