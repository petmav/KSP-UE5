using System.Globalization;
using System.Runtime.Serialization;
using System.Security;

namespace System.Text;

[Serializable]
internal class EncoderNLS : Encoder, ISerializable
{
	internal char charLeftOver;

	protected Encoding m_encoding;

	[NonSerialized]
	protected bool m_mustFlush;

	[NonSerialized]
	internal bool m_throwOnOverflow;

	[NonSerialized]
	internal int m_charsUsed;

	public Encoding Encoding => m_encoding;

	public bool MustFlush => m_mustFlush;

	internal virtual bool HasState => charLeftOver != '\0';

	internal EncoderNLS(SerializationInfo info, StreamingContext context)
	{
		throw new NotSupportedException(string.Format(CultureInfo.CurrentCulture, Environment.GetResourceString("Direct deserialization of type '{0}' is not supported."), GetType()));
	}

	[SecurityCritical]
	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		SerializeEncoder(info);
		info.AddValue("encoding", m_encoding);
		info.AddValue("charLeftOver", charLeftOver);
		info.SetType(typeof(Encoding.DefaultEncoder));
	}

	internal EncoderNLS(Encoding encoding)
	{
		m_encoding = encoding;
		m_fallback = m_encoding.EncoderFallback;
		Reset();
	}

	internal EncoderNLS()
	{
		m_encoding = null;
		Reset();
	}

	public override void Reset()
	{
		charLeftOver = '\0';
		if (m_fallbackBuffer != null)
		{
			m_fallbackBuffer.Reset();
		}
	}

	[SecuritySafeCritical]
	public unsafe override int GetByteCount(char[] chars, int index, int count, bool flush)
	{
		if (chars == null)
		{
			throw new ArgumentNullException("chars", Environment.GetResourceString("Array cannot be null."));
		}
		if (index < 0 || count < 0)
		{
			throw new ArgumentOutOfRangeException((index < 0) ? "index" : "count", Environment.GetResourceString("Non-negative number required."));
		}
		if (chars.Length - index < count)
		{
			throw new ArgumentOutOfRangeException("chars", Environment.GetResourceString("Index and count must refer to a location within the buffer."));
		}
		if (chars.Length == 0)
		{
			chars = new char[1];
		}
		int byteCount;
		fixed (char* ptr = chars)
		{
			byteCount = GetByteCount(ptr + index, count, flush);
		}
		return byteCount;
	}

	[SecurityCritical]
	public unsafe override int GetByteCount(char* chars, int count, bool flush)
	{
		if (chars == null)
		{
			throw new ArgumentNullException("chars", Environment.GetResourceString("Array cannot be null."));
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Non-negative number required."));
		}
		m_mustFlush = flush;
		m_throwOnOverflow = true;
		return m_encoding.GetByteCount(chars, count, this);
	}

	[SecuritySafeCritical]
	public unsafe override int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex, bool flush)
	{
		if (chars == null || bytes == null)
		{
			throw new ArgumentNullException((chars == null) ? "chars" : "bytes", Environment.GetResourceString("Array cannot be null."));
		}
		if (charIndex < 0 || charCount < 0)
		{
			throw new ArgumentOutOfRangeException((charIndex < 0) ? "charIndex" : "charCount", Environment.GetResourceString("Non-negative number required."));
		}
		if (chars.Length - charIndex < charCount)
		{
			throw new ArgumentOutOfRangeException("chars", Environment.GetResourceString("Index and count must refer to a location within the buffer."));
		}
		if (byteIndex < 0 || byteIndex > bytes.Length)
		{
			throw new ArgumentOutOfRangeException("byteIndex", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (chars.Length == 0)
		{
			chars = new char[1];
		}
		int byteCount = bytes.Length - byteIndex;
		if (bytes.Length == 0)
		{
			bytes = new byte[1];
		}
		fixed (char* ptr = chars)
		{
			fixed (byte* ptr2 = bytes)
			{
				return GetBytes(ptr + charIndex, charCount, ptr2 + byteIndex, byteCount, flush);
			}
		}
	}

	[SecurityCritical]
	public unsafe override int GetBytes(char* chars, int charCount, byte* bytes, int byteCount, bool flush)
	{
		if (chars == null || bytes == null)
		{
			throw new ArgumentNullException((chars == null) ? "chars" : "bytes", Environment.GetResourceString("Array cannot be null."));
		}
		if (byteCount < 0 || charCount < 0)
		{
			throw new ArgumentOutOfRangeException((byteCount < 0) ? "byteCount" : "charCount", Environment.GetResourceString("Non-negative number required."));
		}
		m_mustFlush = flush;
		m_throwOnOverflow = true;
		return m_encoding.GetBytes(chars, charCount, bytes, byteCount, this);
	}

	[SecuritySafeCritical]
	public unsafe override void Convert(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex, int byteCount, bool flush, out int charsUsed, out int bytesUsed, out bool completed)
	{
		if (chars == null || bytes == null)
		{
			throw new ArgumentNullException((chars == null) ? "chars" : "bytes", Environment.GetResourceString("Array cannot be null."));
		}
		if (charIndex < 0 || charCount < 0)
		{
			throw new ArgumentOutOfRangeException((charIndex < 0) ? "charIndex" : "charCount", Environment.GetResourceString("Non-negative number required."));
		}
		if (byteIndex < 0 || byteCount < 0)
		{
			throw new ArgumentOutOfRangeException((byteIndex < 0) ? "byteIndex" : "byteCount", Environment.GetResourceString("Non-negative number required."));
		}
		if (chars.Length - charIndex < charCount)
		{
			throw new ArgumentOutOfRangeException("chars", Environment.GetResourceString("Index and count must refer to a location within the buffer."));
		}
		if (bytes.Length - byteIndex < byteCount)
		{
			throw new ArgumentOutOfRangeException("bytes", Environment.GetResourceString("Index and count must refer to a location within the buffer."));
		}
		if (chars.Length == 0)
		{
			chars = new char[1];
		}
		if (bytes.Length == 0)
		{
			bytes = new byte[1];
		}
		fixed (char* ptr = chars)
		{
			fixed (byte* ptr2 = bytes)
			{
				Convert(ptr + charIndex, charCount, ptr2 + byteIndex, byteCount, flush, out charsUsed, out bytesUsed, out completed);
			}
		}
	}

	[SecurityCritical]
	public unsafe override void Convert(char* chars, int charCount, byte* bytes, int byteCount, bool flush, out int charsUsed, out int bytesUsed, out bool completed)
	{
		if (bytes == null || chars == null)
		{
			throw new ArgumentNullException((bytes == null) ? "bytes" : "chars", Environment.GetResourceString("Array cannot be null."));
		}
		if (charCount < 0 || byteCount < 0)
		{
			throw new ArgumentOutOfRangeException((charCount < 0) ? "charCount" : "byteCount", Environment.GetResourceString("Non-negative number required."));
		}
		m_mustFlush = flush;
		m_throwOnOverflow = false;
		m_charsUsed = 0;
		bytesUsed = m_encoding.GetBytes(chars, charCount, bytes, byteCount, this);
		charsUsed = m_charsUsed;
		completed = charsUsed == charCount && (!flush || !HasState) && (m_fallbackBuffer == null || m_fallbackBuffer.Remaining == 0);
	}

	internal void ClearMustFlush()
	{
		m_mustFlush = false;
	}
}
