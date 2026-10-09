using System;
using System.Text;

namespace I18N.CJK;

[Serializable]
internal class CP936 : DbcsEncoding
{
	private const int GB2312_CODE_PAGE = 936;

	public override string BodyName => "gb2312";

	public override string EncodingName => "Chinese Simplified (GB2312)";

	public override string HeaderName => "gb2312";

	public override bool IsBrowserDisplay => true;

	public override bool IsBrowserSave => true;

	public override bool IsMailNewsDisplay => true;

	public override bool IsMailNewsSave => true;

	public override string WebName => "gb2312";

	public CP936()
		: base(936)
	{
	}

	internal override DbcsConvert GetConvert()
	{
		return DbcsConvert.Gb2312;
	}

	public unsafe override int GetByteCountImpl(char* chars, int count)
	{
		DbcsConvert convert = GetConvert();
		int num = 0;
		int num2 = 0;
		while (count-- > 0)
		{
			char c = *(char*)((byte*)chars + num++ * 2);
			if (c <= '\u0080' || c == 'ÿ')
			{
				num2++;
				continue;
			}
			byte b = convert.u2n[c * 2 + 1];
			byte b2 = convert.u2n[c * 2];
			if (b != 0 || b2 != 0)
			{
				num2 += 2;
			}
		}
		return num2;
	}

	public unsafe override int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount)
	{
		DbcsConvert convert = GetConvert();
		int charIndex = 0;
		int byteIndex = 0;
		EncoderFallbackBuffer buffer = null;
		int num = byteIndex;
		while (charCount-- > 0)
		{
			char c = *(char*)((byte*)chars + charIndex++ * 2);
			if (c <= '\u0080' || c == 'ÿ')
			{
				bytes[byteIndex++] = (byte)c;
				continue;
			}
			byte b = convert.u2n[c * 2 + 1];
			byte b2 = convert.u2n[c * 2];
			if (b == 0 && b2 == 0)
			{
				HandleFallback(ref buffer, chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
				continue;
			}
			bytes[byteIndex++] = b;
			bytes[byteIndex++] = b2;
		}
		return byteIndex - num;
	}

	public override int GetCharCount(byte[] bytes, int index, int count)
	{
		return GetDecoder().GetCharCount(bytes, index, count);
	}

	public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
	{
		return GetDecoder().GetChars(bytes, byteIndex, byteCount, chars, charIndex);
	}

	public override Decoder GetDecoder()
	{
		return new CP936Decoder(GetConvert());
	}
}
