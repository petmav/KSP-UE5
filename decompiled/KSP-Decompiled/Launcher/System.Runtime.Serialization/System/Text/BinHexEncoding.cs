using System.Globalization;
using System.Runtime.Serialization;
using System.Security;

namespace System.Text;

internal class BinHexEncoding : Encoding
{
	private static byte[] char2val = new byte[128]
	{
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 0, 1,
		2, 3, 4, 5, 6, 7, 8, 9, 255, 255,
		255, 255, 255, 255, 255, 10, 11, 12, 13, 14,
		15, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 10, 11, 12,
		13, 14, 15, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255
	};

	private static string val2char = "0123456789ABCDEF";

	public override int GetMaxByteCount(int charCount)
	{
		if (charCount < 0)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("charCount", SR.GetString("The value of this argument must be non-negative.")));
		}
		if (charCount % 2 != 0)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new FormatException(SR.GetString("BinHex sequence length ({0}) not valid. Must be a multiple of 2.", charCount.ToString(NumberFormatInfo.CurrentInfo))));
		}
		return charCount / 2;
	}

	public override int GetByteCount(char[] chars, int index, int count)
	{
		return GetMaxByteCount(count);
	}

	[SecuritySafeCritical]
	public unsafe override int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex)
	{
		if (chars == null)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentNullException("chars"));
		}
		if (charIndex < 0)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("charIndex", SR.GetString("The value of this argument must be non-negative.")));
		}
		if (charIndex > chars.Length)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("charIndex", SR.GetString("The specified offset exceeds the buffer size ({0} bytes).", chars.Length)));
		}
		if (charCount < 0)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("charCount", SR.GetString("The value of this argument must be non-negative.")));
		}
		if (charCount > chars.Length - charIndex)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("charCount", SR.GetString("The specified size exceeds the remaining buffer space ({0} bytes).", chars.Length - charIndex)));
		}
		if (bytes == null)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentNullException("bytes"));
		}
		if (byteIndex < 0)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("byteIndex", SR.GetString("The value of this argument must be non-negative.")));
		}
		if (byteIndex > bytes.Length)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("byteIndex", SR.GetString("The specified offset exceeds the buffer size ({0} bytes).", bytes.Length)));
		}
		int byteCount = GetByteCount(chars, charIndex, charCount);
		if (byteCount < 0 || byteCount > bytes.Length - byteIndex)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentException(SR.GetString("Array too small."), "bytes"));
		}
		if (charCount > 0)
		{
			fixed (byte* ptr6 = char2val)
			{
				fixed (byte* ptr4 = &bytes[byteIndex])
				{
					fixed (char* ptr = &chars[charIndex])
					{
						char* ptr2 = ptr;
						char* ptr3 = ptr + charCount;
						byte* ptr5 = ptr4;
						while (ptr2 < ptr3)
						{
							char c = *ptr2;
							char c2 = ptr2[1];
							if ((c | c2) >= 128)
							{
								throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new FormatException(SR.GetString("The characters '{0}' at offset {1} are not a valid BinHex sequence.", new string(ptr2, 0, 2), charIndex + (int)(ptr2 - ptr))));
							}
							byte b = ptr6[(int)c];
							byte b2 = ptr6[(int)c2];
							if ((b | b2) == 255)
							{
								throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new FormatException(SR.GetString("The characters '{0}' at offset {1} are not a valid BinHex sequence.", new string(ptr2, 0, 2), charIndex + (int)(ptr2 - ptr))));
							}
							*ptr5 = (byte)((b << 4) + b2);
							ptr2 += 2;
							ptr5++;
						}
					}
				}
			}
		}
		return byteCount;
	}

	public override int GetMaxCharCount(int byteCount)
	{
		if (byteCount < 0 || byteCount > 1073741823)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("byteCount", SR.GetString("The value of this argument must fall within the range {0} to {1}.", 0, 1073741823)));
		}
		return byteCount * 2;
	}

	public override int GetCharCount(byte[] bytes, int index, int count)
	{
		return GetMaxCharCount(count);
	}

	[SecuritySafeCritical]
	public unsafe override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
	{
		if (bytes == null)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentNullException("bytes"));
		}
		if (byteIndex < 0)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("byteIndex", SR.GetString("The value of this argument must be non-negative.")));
		}
		if (byteIndex > bytes.Length)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("byteIndex", SR.GetString("The specified offset exceeds the buffer size ({0} bytes).", bytes.Length)));
		}
		if (byteCount < 0)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("byteCount", SR.GetString("The value of this argument must be non-negative.")));
		}
		if (byteCount > bytes.Length - byteIndex)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("byteCount", SR.GetString("The specified size exceeds the remaining buffer space ({0} bytes).", bytes.Length - byteIndex)));
		}
		int charCount = GetCharCount(bytes, byteIndex, byteCount);
		if (chars == null)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentNullException("chars"));
		}
		if (charIndex < 0)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("charIndex", SR.GetString("The value of this argument must be non-negative.")));
		}
		if (charIndex > chars.Length)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentOutOfRangeException("charIndex", SR.GetString("The specified offset exceeds the buffer size ({0} bytes).", chars.Length)));
		}
		if (charCount < 0 || charCount > chars.Length - charIndex)
		{
			throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentException(SR.GetString("Array too small."), "chars"));
		}
		if (byteCount > 0)
		{
			fixed (char* ptr6 = val2char)
			{
				fixed (byte* ptr3 = &bytes[byteIndex])
				{
					fixed (char* ptr = &chars[charIndex])
					{
						char* ptr2 = ptr;
						byte* ptr4 = ptr3;
						byte* ptr5 = ptr3 + byteCount;
						while (ptr4 < ptr5)
						{
							*ptr2 = ptr6[*ptr4 >> 4];
							ptr2[1] = ptr6[*ptr4 & 0xF];
							ptr4++;
							ptr2 += 2;
						}
					}
				}
			}
		}
		return charCount;
	}
}
