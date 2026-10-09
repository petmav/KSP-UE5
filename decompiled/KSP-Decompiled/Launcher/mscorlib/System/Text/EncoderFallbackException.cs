using System.Runtime.Serialization;

namespace System.Text;

[Serializable]
public sealed class EncoderFallbackException : ArgumentException
{
	private char charUnknown;

	private char charUnknownHigh;

	private char charUnknownLow;

	private int index;

	public char CharUnknown => charUnknown;

	public char CharUnknownHigh => charUnknownHigh;

	public char CharUnknownLow => charUnknownLow;

	public int Index => index;

	public EncoderFallbackException()
		: base(Environment.GetResourceString("Value does not fall within the expected range."))
	{
		SetErrorCode(-2147024809);
	}

	public EncoderFallbackException(string message)
		: base(message)
	{
		SetErrorCode(-2147024809);
	}

	public EncoderFallbackException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2147024809);
	}

	internal EncoderFallbackException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}

	internal EncoderFallbackException(string message, char charUnknown, int index)
		: base(message)
	{
		this.charUnknown = charUnknown;
		this.index = index;
	}

	internal EncoderFallbackException(string message, char charUnknownHigh, char charUnknownLow, int index)
		: base(message)
	{
		if (!char.IsHighSurrogate(charUnknownHigh))
		{
			throw new ArgumentOutOfRangeException("charUnknownHigh", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", 55296, 56319));
		}
		if (!char.IsLowSurrogate(charUnknownLow))
		{
			throw new ArgumentOutOfRangeException("charUnknownLow", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", 56320, 57343));
		}
		this.charUnknownHigh = charUnknownHigh;
		this.charUnknownLow = charUnknownLow;
		this.index = index;
	}

	public bool IsUnknownSurrogate()
	{
		return charUnknownHigh != '\0';
	}
}
