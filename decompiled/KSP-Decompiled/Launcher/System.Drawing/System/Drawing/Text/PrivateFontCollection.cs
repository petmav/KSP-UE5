using Unity;

namespace System.Drawing.Text;

public sealed class PrivateFontCollection : FontCollection
{
	public PrivateFontCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddFontFile(string filename)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddMemoryFont(IntPtr memory, int length)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void Dispose(bool disposing)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
