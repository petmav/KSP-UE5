using Unity;

namespace System.Drawing;

public sealed class BufferedGraphicsManager
{
	public static BufferedGraphicsContext Current
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal BufferedGraphicsManager()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
