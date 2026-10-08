using Unity;

namespace System.Drawing;

public sealed class BufferedGraphics : IDisposable
{
	public Graphics Graphics
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal BufferedGraphics()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Render()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Render(Graphics target)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Render(IntPtr targetDC)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
