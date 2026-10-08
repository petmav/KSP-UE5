using Unity;

namespace System.Drawing;

public abstract class Brush : MarshalByRefObject, ICloneable, IDisposable
{
	protected Brush()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public abstract object Clone();

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void Dispose(bool disposing)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal void SetNativeBrush(IntPtr brush)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
