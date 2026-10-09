using System.Runtime.InteropServices;
using Unity;

namespace System.Drawing.Imaging;

[StructLayout(LayoutKind.Sequential)]
public sealed class EncoderParameter : IDisposable
{
	public Encoder Encoder
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int NumberOfValues
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public EncoderParameterValueType Type
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(EncoderParameterValueType);
		}
	}

	public EncoderParameterValueType ValueType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(EncoderParameterValueType);
		}
	}

	public EncoderParameter(Encoder encoder, byte value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, byte value, bool undefined)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, byte[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, byte[] value, bool undefined)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, short value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, short[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, int numberValues, EncoderParameterValueType type, IntPtr value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, int numerator, int denominator)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("This constructor has been deprecated. Use EncoderParameter(Encoder encoder, int numberValues, EncoderParameterValueType type, IntPtr value) instead.  http://go.microsoft.com/fwlink/?linkid=14202")]
	public EncoderParameter(Encoder encoder, int NumberOfValues, int Type, int Value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, int numerator1, int demoninator1, int numerator2, int demoninator2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, int[] numerator, int[] denominator)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, int[] numerator1, int[] denominator1, int[] numerator2, int[] denominator2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, long value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, long rangebegin, long rangeend)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, long[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, long[] rangebegin, long[] rangeend)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameter(Encoder encoder, string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
