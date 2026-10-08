using Unity;

namespace System.Drawing.Text;

public abstract class FontCollection : IDisposable
{
	public FontFamily[] Families
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal FontCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void Dispose(bool disposing)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
