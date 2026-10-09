using System;
using System.Text;
using I18N.Common;

namespace I18N.Other;

[Serializable]
public abstract class ISCIIEncoding : MonoEncoding
{
	private int shift;

	private string encodingName;

	private string webName;

	public override string BodyName => webName;

	public override string EncodingName => encodingName;

	public override string HeaderName => webName;

	public override string WebName => webName;

	protected ISCIIEncoding(int codePage, int shift, string encodingName, string webName)
		: base(codePage)
	{
		this.shift = shift;
		this.encodingName = encodingName;
		this.webName = webName;
	}

	public override int GetByteCount(char[] chars, int index, int count)
	{
		if (chars == null)
		{
			throw new ArgumentNullException("chars");
		}
		if (index < 0 || index > chars.Length)
		{
			throw new ArgumentOutOfRangeException("index", Strings.GetString("ArgRange_Array"));
		}
		if (count < 0 || count > chars.Length - index)
		{
			throw new ArgumentOutOfRangeException("count", Strings.GetString("ArgRange_Array"));
		}
		return count;
	}

	public override int GetByteCount(string s)
	{
		if (s == null)
		{
			throw new ArgumentNullException("s");
		}
		return s.Length;
	}

	public unsafe override int GetByteCountImpl(char* chars, int count)
	{
		int num = 0;
		int num2 = 0;
		char c = (char)shift;
		char c2 = (char)(shift + 127);
		while (count-- > 0)
		{
			char c3 = *(char*)((byte*)chars + num++ * 2);
			num2 = ((c3 >= '\u0080') ? ((c3 >= c && c3 <= c2) ? (num2 + 1) : ((c3 < '！' || c3 > '～') ? (num2 + 1) : (num2 + 1))) : (num2 + 1));
			count--;
		}
		return num2;
	}

	public unsafe override int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount)
	{
		EncoderFallbackBuffer buffer = null;
		int charIndex = 0;
		int num = 0;
		if (chars == null)
		{
			throw new ArgumentNullException("chars");
		}
		if (bytes == null)
		{
			throw new ArgumentNullException("bytes");
		}
		int byteIndex = num;
		char c = (char)shift;
		char c2 = (char)(shift + 127);
		while (charCount-- > 0)
		{
			char c3 = *(char*)((byte*)chars + charIndex++ * 2);
			if (c3 < '\u0080')
			{
				bytes[byteIndex++] = (byte)c3;
			}
			else if (c3 >= c && c3 <= c2)
			{
				bytes[byteIndex++] = (byte)(c3 - c + 128);
			}
			else
			{
				if (c3 < '！' || c3 > '～')
				{
					HandleFallback(ref buffer, chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
					continue;
				}
				bytes[byteIndex++] = (byte)(c3 - 65248);
			}
			byteCount--;
		}
		return byteIndex - num;
	}

	public override int GetCharCount(byte[] bytes, int index, int count)
	{
		if (bytes == null)
		{
			throw new ArgumentNullException("bytes");
		}
		if (index < 0 || index > bytes.Length)
		{
			throw new ArgumentOutOfRangeException("index", Strings.GetString("ArgRange_Array"));
		}
		if (count < 0 || count > bytes.Length - index)
		{
			throw new ArgumentOutOfRangeException("count", Strings.GetString("ArgRange_Array"));
		}
		return count;
	}

	public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
	{
		if (bytes == null)
		{
			throw new ArgumentNullException("bytes");
		}
		if (chars == null)
		{
			throw new ArgumentNullException("chars");
		}
		if (byteIndex < 0 || byteIndex > bytes.Length)
		{
			throw new ArgumentOutOfRangeException("byteIndex", Strings.GetString("ArgRange_Array"));
		}
		if (byteCount < 0 || byteCount > bytes.Length - byteIndex)
		{
			throw new ArgumentOutOfRangeException("byteCount", Strings.GetString("ArgRange_Array"));
		}
		if (charIndex < 0 || charIndex > chars.Length)
		{
			throw new ArgumentOutOfRangeException("charIndex", Strings.GetString("ArgRange_Array"));
		}
		if (chars.Length - charIndex < byteCount)
		{
			throw new ArgumentException(Strings.GetString("Arg_InsufficientSpace"), "chars");
		}
		int num = byteCount;
		int num2 = shift - 128;
		while (num-- > 0)
		{
			int num3 = bytes[byteIndex++];
			if (num3 < 128)
			{
				chars[charIndex++] = (char)num3;
			}
			else
			{
				chars[charIndex++] = (char)(num3 + num2);
			}
		}
		return byteCount;
	}

	public override int GetMaxByteCount(int charCount)
	{
		if (charCount < 0)
		{
			throw new ArgumentOutOfRangeException("charCount", Strings.GetString("ArgRange_NonNegative"));
		}
		return charCount;
	}

	public override int GetMaxCharCount(int byteCount)
	{
		if (byteCount < 0)
		{
			throw new ArgumentOutOfRangeException("byteCount", Strings.GetString("ArgRange_NonNegative"));
		}
		return byteCount;
	}
}
