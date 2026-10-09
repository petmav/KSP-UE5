using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System.Text;

[Serializable]
[ComVisible(true)]
public abstract class Decoder
{
	internal DecoderFallback m_fallback;

	[NonSerialized]
	internal DecoderFallbackBuffer m_fallbackBuffer;

	[ComVisible(false)]
	public DecoderFallback Fallback
	{
		get
		{
			return m_fallback;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			if (m_fallbackBuffer != null && m_fallbackBuffer.Remaining > 0)
			{
				throw new ArgumentException(Environment.GetResourceString("Cannot change fallback when buffer is not empty. Previous Convert() call left data in the fallback buffer."), "value");
			}
			m_fallback = value;
			m_fallbackBuffer = null;
		}
	}

	[ComVisible(false)]
	public DecoderFallbackBuffer FallbackBuffer
	{
		get
		{
			if (m_fallbackBuffer == null)
			{
				if (m_fallback != null)
				{
					m_fallbackBuffer = m_fallback.CreateFallbackBuffer();
				}
				else
				{
					m_fallbackBuffer = DecoderFallback.ReplacementFallback.CreateFallbackBuffer();
				}
			}
			return m_fallbackBuffer;
		}
	}

	internal bool InternalHasFallbackBuffer => m_fallbackBuffer != null;

	internal void SerializeDecoder(SerializationInfo info)
	{
		info.AddValue("m_fallback", m_fallback);
	}

	[ComVisible(false)]
	public virtual void Reset()
	{
		byte[] bytes = new byte[0];
		char[] chars = new char[GetCharCount(bytes, 0, 0, flush: true)];
		GetChars(bytes, 0, 0, chars, 0, flush: true);
		if (m_fallbackBuffer != null)
		{
			m_fallbackBuffer.Reset();
		}
	}

	public abstract int GetCharCount(byte[] bytes, int index, int count);

	[ComVisible(false)]
	public virtual int GetCharCount(byte[] bytes, int index, int count, bool flush)
	{
		return GetCharCount(bytes, index, count);
	}

	[ComVisible(false)]
	[SecurityCritical]
	[CLSCompliant(false)]
	public unsafe virtual int GetCharCount(byte* bytes, int count, bool flush)
	{
		if (bytes == null)
		{
			throw new ArgumentNullException("bytes", Environment.GetResourceString("Array cannot be null."));
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Non-negative number required."));
		}
		byte[] array = new byte[count];
		for (int i = 0; i < count; i++)
		{
			array[i] = bytes[i];
		}
		return GetCharCount(array, 0, count);
	}

	public abstract int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex);

	public virtual int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex, bool flush)
	{
		return GetChars(bytes, byteIndex, byteCount, chars, charIndex);
	}

	[CLSCompliant(false)]
	[SecurityCritical]
	[ComVisible(false)]
	public unsafe virtual int GetChars(byte* bytes, int byteCount, char* chars, int charCount, bool flush)
	{
		if (chars == null || bytes == null)
		{
			throw new ArgumentNullException((chars == null) ? "chars" : "bytes", Environment.GetResourceString("Array cannot be null."));
		}
		if (byteCount < 0 || charCount < 0)
		{
			throw new ArgumentOutOfRangeException((byteCount < 0) ? "byteCount" : "charCount", Environment.GetResourceString("Non-negative number required."));
		}
		byte[] array = new byte[byteCount];
		for (int i = 0; i < byteCount; i++)
		{
			array[i] = bytes[i];
		}
		char[] array2 = new char[charCount];
		int chars2 = GetChars(array, 0, byteCount, array2, 0, flush);
		if (chars2 < charCount)
		{
			charCount = chars2;
		}
		for (int i = 0; i < charCount; i++)
		{
			chars[i] = array2[i];
		}
		return charCount;
	}

	[ComVisible(false)]
	public virtual void Convert(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex, int charCount, bool flush, out int bytesUsed, out int charsUsed, out bool completed)
	{
		if (bytes == null || chars == null)
		{
			throw new ArgumentNullException((bytes == null) ? "bytes" : "chars", Environment.GetResourceString("Array cannot be null."));
		}
		if (byteIndex < 0 || byteCount < 0)
		{
			throw new ArgumentOutOfRangeException((byteIndex < 0) ? "byteIndex" : "byteCount", Environment.GetResourceString("Non-negative number required."));
		}
		if (charIndex < 0 || charCount < 0)
		{
			throw new ArgumentOutOfRangeException((charIndex < 0) ? "charIndex" : "charCount", Environment.GetResourceString("Non-negative number required."));
		}
		if (bytes.Length - byteIndex < byteCount)
		{
			throw new ArgumentOutOfRangeException("bytes", Environment.GetResourceString("Index and count must refer to a location within the buffer."));
		}
		if (chars.Length - charIndex < charCount)
		{
			throw new ArgumentOutOfRangeException("chars", Environment.GetResourceString("Index and count must refer to a location within the buffer."));
		}
		for (bytesUsed = byteCount; bytesUsed > 0; bytesUsed /= 2)
		{
			if (GetCharCount(bytes, byteIndex, bytesUsed, flush) <= charCount)
			{
				charsUsed = GetChars(bytes, byteIndex, bytesUsed, chars, charIndex, flush);
				completed = bytesUsed == byteCount && (m_fallbackBuffer == null || m_fallbackBuffer.Remaining == 0);
				return;
			}
			flush = false;
		}
		throw new ArgumentException(Environment.GetResourceString("Conversion buffer overflow."));
	}

	[SecurityCritical]
	[CLSCompliant(false)]
	[ComVisible(false)]
	public unsafe virtual void Convert(byte* bytes, int byteCount, char* chars, int charCount, bool flush, out int bytesUsed, out int charsUsed, out bool completed)
	{
		if (chars == null || bytes == null)
		{
			throw new ArgumentNullException((chars == null) ? "chars" : "bytes", Environment.GetResourceString("Array cannot be null."));
		}
		if (byteCount < 0 || charCount < 0)
		{
			throw new ArgumentOutOfRangeException((byteCount < 0) ? "byteCount" : "charCount", Environment.GetResourceString("Non-negative number required."));
		}
		for (bytesUsed = byteCount; bytesUsed > 0; bytesUsed /= 2)
		{
			if (GetCharCount(bytes, bytesUsed, flush) <= charCount)
			{
				charsUsed = GetChars(bytes, bytesUsed, chars, charCount, flush);
				completed = bytesUsed == byteCount && (m_fallbackBuffer == null || m_fallbackBuffer.Remaining == 0);
				return;
			}
			flush = false;
		}
		throw new ArgumentException(Environment.GetResourceString("Conversion buffer overflow."));
	}
}
