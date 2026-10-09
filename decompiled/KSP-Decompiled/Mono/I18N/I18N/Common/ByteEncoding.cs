using System;
using System.Runtime.CompilerServices;
using System.Text;

namespace I18N.Common;

[Serializable]
public abstract class ByteEncoding : MonoEncoding
{
	protected char[] toChars;

	protected string encodingName;

	protected string bodyName;

	protected string headerName;

	protected string webName;

	protected bool isBrowserDisplay;

	protected bool isBrowserSave;

	protected bool isMailNewsDisplay;

	protected bool isMailNewsSave;

	protected int windowsCodePage;

	private static byte[] isNormalized;

	private static byte[] isNormalizedComputed;

	private static byte[] normalization_bytes;

	public override bool IsSingleByte => true;

	public override string BodyName => bodyName;

	public override string EncodingName => encodingName;

	public override string HeaderName => headerName;

	public override bool IsBrowserDisplay => isBrowserDisplay;

	public override bool IsBrowserSave => isBrowserSave;

	public override bool IsMailNewsDisplay => isMailNewsDisplay;

	public override bool IsMailNewsSave => isMailNewsSave;

	public override string WebName => webName;

	public override int WindowsCodePage => windowsCodePage;

	protected ByteEncoding(int codePage, char[] toChars, string encodingName, string bodyName, string headerName, string webName, bool isBrowserDisplay, bool isBrowserSave, bool isMailNewsDisplay, bool isMailNewsSave, int windowsCodePage)
		: base(codePage)
	{
		if (toChars.Length != 256)
		{
			throw new ArgumentException("toChars");
		}
		this.toChars = toChars;
		this.encodingName = encodingName;
		this.bodyName = bodyName;
		this.headerName = headerName;
		this.webName = webName;
		this.isBrowserDisplay = isBrowserDisplay;
		this.isBrowserSave = isBrowserSave;
		this.isMailNewsDisplay = isMailNewsDisplay;
		this.isMailNewsSave = isMailNewsSave;
		this.windowsCodePage = windowsCodePage;
	}

	public override bool IsAlwaysNormalized(NormalizationForm form)
	{
		if (form != NormalizationForm.FormC)
		{
			return false;
		}
		if (isNormalized == null)
		{
			isNormalized = new byte[8192];
		}
		if (isNormalizedComputed == null)
		{
			isNormalizedComputed = new byte[8192];
		}
		if (normalization_bytes == null)
		{
			normalization_bytes = new byte[256];
			lock (normalization_bytes)
			{
				for (int i = 0; i < 256; i++)
				{
					normalization_bytes[i] = (byte)i;
				}
			}
		}
		byte b = (byte)(1 << CodePage % 8);
		if ((isNormalizedComputed[CodePage / 8] & b) == 0)
		{
			Encoding encoding = Clone() as Encoding;
			encoding.DecoderFallback = new DecoderReplacementFallback(string.Empty);
			string @string = encoding.GetString(normalization_bytes);
			if (@string != @string.Normalize(form))
			{
				isNormalized[CodePage / 8] |= b;
			}
			isNormalizedComputed[CodePage / 8] |= b;
		}
		return (isNormalized[CodePage / 8] & b) == 0;
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
		return count;
	}

	protected unsafe abstract void ToBytes(char* chars, int charCount, byte* bytes, int byteCount);

	protected unsafe virtual void ToBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex)
	{
		//IL_0027->IL002e: Incompatible stack types: I vs Ref
		//IL_0046->IL004e: Incompatible stack types: I vs Ref
		if (charCount == 0 || bytes.Length == byteIndex)
		{
			return;
		}
		fixed (char* ptr = &System.Runtime.CompilerServices.Unsafe.AsRef<char>((char*)System.Runtime.CompilerServices.Unsafe.AsPointer(ref chars != null && chars.Length != 0 ? ref System.Runtime.CompilerServices.Unsafe.As<char, _003F>(ref chars[0]) : ref *(_003F*)null)))
		{
			fixed (byte* ptr2 = &System.Runtime.CompilerServices.Unsafe.AsRef<byte>((byte*)System.Runtime.CompilerServices.Unsafe.AsPointer(ref bytes != null && bytes.Length != 0 ? ref System.Runtime.CompilerServices.Unsafe.As<byte, _003F>(ref bytes[0]) : ref *(_003F*)null)))
			{
				ToBytes((char*)((byte*)ptr + charIndex * 2), charCount, ptr2 + byteIndex, bytes.Length - byteIndex);
			}
		}
	}

	public unsafe override int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount)
	{
		ToBytes(chars, charCount, bytes, byteCount);
		return charCount;
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
			throw new ArgumentException(Strings.GetString("Arg_InsufficientSpace"));
		}
		int num = byteCount;
		char[] array = toChars;
		while (num-- > 0)
		{
			chars[charIndex++] = array[bytes[byteIndex++]];
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

	public unsafe override string GetString(byte[] bytes, int index, int count)
	{
		//IL_0086->IL008d: Incompatible stack types: I vs Ref
		//IL_00ba->IL00c6: Incompatible stack types: I vs Ref
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
		if (count == 0)
		{
			return string.Empty;
		}
		string text = new string('\0', count);
		fixed (byte* ptr = &System.Runtime.CompilerServices.Unsafe.AsRef<byte>((byte*)System.Runtime.CompilerServices.Unsafe.AsPointer(ref bytes != null && bytes.Length != 0 ? ref System.Runtime.CompilerServices.Unsafe.As<byte, _003F>(ref bytes[0]) : ref *(_003F*)null)))
		{
			fixed (char* ptr3 = text)
			{
				fixed (char* ptr5 = &System.Runtime.CompilerServices.Unsafe.AsRef<char>((char*)System.Runtime.CompilerServices.Unsafe.AsPointer(ref toChars != null && toChars.Length != 0 ? ref System.Runtime.CompilerServices.Unsafe.As<char, _003F>(ref toChars[0]) : ref *(_003F*)null)))
				{
					byte* ptr2 = ptr + index;
					char* ptr4 = ptr3;
					while (count-- != 0)
					{
						*(ptr4++) = ptr5[(int)(*(ptr2++))];
					}
				}
			}
		}
		return text;
	}

	public override string GetString(byte[] bytes)
	{
		if (bytes == null)
		{
			throw new ArgumentNullException("bytes");
		}
		return GetString(bytes, 0, bytes.Length);
	}
}
