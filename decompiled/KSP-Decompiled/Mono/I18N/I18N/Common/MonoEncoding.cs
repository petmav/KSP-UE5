using System;
using System.Runtime.CompilerServices;
using System.Text;

namespace I18N.Common;

[Serializable]
public abstract class MonoEncoding : Encoding
{
	private readonly int win_code_page;

	public override int WindowsCodePage => (win_code_page == 0) ? base.WindowsCodePage : win_code_page;

	public MonoEncoding(int codePage)
		: this(codePage, 0)
	{
	}

	public MonoEncoding(int codePage, int windowsCodePage)
		: base(codePage)
	{
		win_code_page = windowsCodePage;
	}

	public unsafe void HandleFallback(ref EncoderFallbackBuffer buffer, char* chars, ref int charIndex, ref int charCount, byte* bytes, ref int byteIndex, ref int byteCount)
	{
		//IL_00c3->IL00ca: Incompatible stack types: I vs Ref
		if (buffer == null)
		{
			buffer = base.EncoderFallback.CreateFallbackBuffer();
		}
		if (char.IsSurrogate(*(char*)((byte*)chars + charIndex * 2)) && charCount > 0 && char.IsSurrogate(*(char*)((byte*)chars + (charIndex + 1) * 2)))
		{
			buffer.Fallback(*(char*)((byte*)chars + charIndex * 2), *(char*)((byte*)chars + (charIndex + 1) * 2), charIndex);
			charIndex++;
			charCount--;
		}
		else
		{
			buffer.Fallback(*(char*)((byte*)chars + charIndex * 2), charIndex);
		}
		char[] array = new char[buffer.Remaining];
		int num = 0;
		while (buffer.Remaining > 0)
		{
			array[num++] = buffer.GetNextChar();
		}
		fixed (char* chars2 = &System.Runtime.CompilerServices.Unsafe.AsRef<char>((char*)System.Runtime.CompilerServices.Unsafe.AsPointer(ref array != null && array.Length != 0 ? ref System.Runtime.CompilerServices.Unsafe.As<char, _003F>(ref array[0]) : ref *(_003F*)null)))
		{
			byteIndex += GetBytes(chars2, array.Length, bytes + byteIndex, byteCount);
		}
	}

	public unsafe override int GetByteCount(char[] chars, int index, int count)
	{
		//IL_007a->IL0081: Incompatible stack types: I vs Ref
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
		if (count == 0)
		{
			return 0;
		}
		fixed (char* ptr = &System.Runtime.CompilerServices.Unsafe.AsRef<char>((char*)System.Runtime.CompilerServices.Unsafe.AsPointer(ref chars != null && chars.Length != 0 ? ref System.Runtime.CompilerServices.Unsafe.As<char, _003F>(ref chars[0]) : ref *(_003F*)null)))
		{
			return GetByteCountImpl((char*)((byte*)ptr + index * 2), count);
		}
	}

	public unsafe override int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex)
	{
		//IL_00d6->IL00dd: Incompatible stack types: I vs Ref
		//IL_00f5->IL00fd: Incompatible stack types: I vs Ref
		if (chars == null)
		{
			throw new ArgumentNullException("chars");
		}
		if (bytes == null)
		{
			throw new ArgumentNullException("bytes");
		}
		if (charIndex < 0 || charIndex > chars.Length)
		{
			throw new ArgumentOutOfRangeException("charIndex", Strings.GetString("ArgRange_Array"));
		}
		if (charCount < 0 || charCount > chars.Length - charIndex)
		{
			throw new ArgumentOutOfRangeException("charCount", Strings.GetString("ArgRange_Array"));
		}
		if (byteIndex < 0 || byteIndex > bytes.Length)
		{
			throw new ArgumentOutOfRangeException("byteIndex", Strings.GetString("ArgRange_Array"));
		}
		if (bytes.Length - byteIndex < charCount)
		{
			throw new ArgumentException(Strings.GetString("Arg_InsufficientSpace"), "bytes");
		}
		if (charCount == 0)
		{
			return 0;
		}
		fixed (char* ptr = &System.Runtime.CompilerServices.Unsafe.AsRef<char>((char*)System.Runtime.CompilerServices.Unsafe.AsPointer(ref chars != null && chars.Length != 0 ? ref System.Runtime.CompilerServices.Unsafe.As<char, _003F>(ref chars[0]) : ref *(_003F*)null)))
		{
			fixed (byte* ptr2 = &System.Runtime.CompilerServices.Unsafe.AsRef<byte>((byte*)System.Runtime.CompilerServices.Unsafe.AsPointer(ref bytes != null && bytes.Length != 0 ? ref System.Runtime.CompilerServices.Unsafe.As<byte, _003F>(ref bytes[0]) : ref *(_003F*)null)))
			{
				return GetBytesImpl((char*)((byte*)ptr + charIndex * 2), charCount, ptr2 + byteIndex, bytes.Length - byteIndex);
			}
		}
	}

	public unsafe override int GetBytes(string s, int charIndex, int charCount, byte[] bytes, int byteIndex)
	{
		//IL_00f4->IL00fc: Incompatible stack types: I vs Ref
		if (s == null)
		{
			throw new ArgumentNullException("s");
		}
		if (bytes == null)
		{
			throw new ArgumentNullException("bytes");
		}
		if (charIndex < 0 || charIndex > s.Length)
		{
			throw new ArgumentOutOfRangeException("charIndex", Strings.GetString("ArgRange_StringIndex"));
		}
		if (charCount < 0 || charCount > s.Length - charIndex)
		{
			throw new ArgumentOutOfRangeException("charCount", Strings.GetString("ArgRange_StringRange"));
		}
		if (byteIndex < 0 || byteIndex > bytes.Length)
		{
			throw new ArgumentOutOfRangeException("byteIndex", Strings.GetString("ArgRange_Array"));
		}
		if (bytes.Length - byteIndex < charCount)
		{
			throw new ArgumentException(Strings.GetString("Arg_InsufficientSpace"), "bytes");
		}
		if (charCount == 0 || bytes.Length == byteIndex)
		{
			return 0;
		}
		fixed (char* ptr = s)
		{
			fixed (byte* ptr2 = &System.Runtime.CompilerServices.Unsafe.AsRef<byte>((byte*)System.Runtime.CompilerServices.Unsafe.AsPointer(ref bytes != null && bytes.Length != 0 ? ref System.Runtime.CompilerServices.Unsafe.As<byte, _003F>(ref bytes[0]) : ref *(_003F*)null)))
			{
				return GetBytesImpl((char*)((byte*)ptr + charIndex * 2), charCount, ptr2 + byteIndex, bytes.Length - byteIndex);
			}
		}
	}

	public unsafe override int GetByteCount(char* chars, int count)
	{
		return GetByteCountImpl(chars, count);
	}

	public unsafe override int GetBytes(char* chars, int charCount, byte* bytes, int byteCount)
	{
		return GetBytesImpl(chars, charCount, bytes, byteCount);
	}

	public unsafe abstract int GetByteCountImpl(char* chars, int charCount);

	public unsafe abstract int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount);
}
