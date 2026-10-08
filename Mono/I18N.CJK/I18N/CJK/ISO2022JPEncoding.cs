using System;
using I18N.Common;

namespace I18N.CJK;

[Serializable]
public class ISO2022JPEncoding : MonoEncoding
{
	private readonly bool allow_1byte_kana;

	private readonly bool allow_shift_io;

	public override string BodyName => "iso-2022-jp";

	public override string HeaderName => "iso-2022-jp";

	public override string WebName => "csISO2022JP";

	public ISO2022JPEncoding(int codePage, bool allow1ByteKana, bool allowShiftIO)
		: base(codePage, 932)
	{
		allow_1byte_kana = allow1ByteKana;
		allow_shift_io = allowShiftIO;
	}

	public override int GetMaxByteCount(int charCount)
	{
		return charCount / 2 * 5 + 4;
	}

	public override int GetMaxCharCount(int byteCount)
	{
		return byteCount;
	}

	public override int GetByteCount(char[] chars, int charIndex, int charCount)
	{
		return new ISO2022JPEncoder(this, allow_1byte_kana, allow_shift_io).GetByteCount(chars, charIndex, charCount, refresh: true);
	}

	public unsafe override int GetByteCountImpl(char* chars, int count)
	{
		return new ISO2022JPEncoder(this, allow_1byte_kana, allow_shift_io).GetByteCountImpl(chars, count, flush: true);
	}

	public unsafe override int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount)
	{
		return new ISO2022JPEncoder(this, allow_1byte_kana, allow_shift_io).GetBytesImpl(chars, charCount, bytes, byteCount, flush: true);
	}

	public override int GetCharCount(byte[] bytes, int index, int count)
	{
		return new ISO2022JPDecoder(allow_1byte_kana, allow_shift_io).GetCharCount(bytes, index, count);
	}

	public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
	{
		return new ISO2022JPDecoder(allow_1byte_kana, allow_shift_io).GetChars(bytes, byteIndex, byteCount, chars, charIndex);
	}
}
