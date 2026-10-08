using Unity;

namespace System.Drawing.Imaging;

public sealed class EncoderParameters : IDisposable
{
	public EncoderParameter[] Param
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

	public EncoderParameters()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EncoderParameters(int count)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
