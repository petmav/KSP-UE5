using System;
using System.Text;
using I18N.Common;

namespace I18N.CJK;

[Serializable]
public class CP51932 : MonoEncoding
{
	private const int EUC_JP_CODE_PAGE = 51932;

	public override string BodyName => "euc-jp";

	public override string EncodingName => "Japanese (EUC)";

	public override string HeaderName => "euc-jp";

	public override bool IsBrowserDisplay => true;

	public override bool IsBrowserSave => true;

	public override bool IsMailNewsDisplay => true;

	public override bool IsMailNewsSave => true;

	public override string WebName => "euc-jp";

	public CP51932()
		: base(51932, 932)
	{
	}

	public override int GetByteCount(char[] chars, int index, int length)
	{
		return new CP51932Encoder(this).GetByteCount(chars, index, length, refresh: true);
	}

	public unsafe override int GetByteCountImpl(char* chars, int count)
	{
		return new CP51932Encoder(this).GetByteCountImpl(chars, count, refresh: true);
	}

	public unsafe override int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount)
	{
		return new CP51932Encoder(this).GetBytesImpl(chars, charCount, bytes, byteCount, refresh: true);
	}

	public override int GetCharCount(byte[] bytes, int index, int count)
	{
		return new CP51932Decoder().GetCharCount(bytes, index, count, refresh: true);
	}

	public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
	{
		return new CP51932Decoder().GetChars(bytes, byteIndex, byteCount, chars, charIndex, refresh: true);
	}

	public override int GetMaxByteCount(int charCount)
	{
		if (charCount < 0)
		{
			throw new ArgumentOutOfRangeException("charCount", Strings.GetString("ArgRange_NonNegative"));
		}
		return charCount * 3;
	}

	public override int GetMaxCharCount(int byteCount)
	{
		if (byteCount < 0)
		{
			throw new ArgumentOutOfRangeException("byteCount", Strings.GetString("ArgRange_NonNegative"));
		}
		return byteCount;
	}

	public override Encoder GetEncoder()
	{
		return new CP51932Encoder(this);
	}

	public override Decoder GetDecoder()
	{
		return new CP51932Decoder();
	}
}
