using Unity;

namespace System.Drawing.Text;

public sealed class InstalledFontCollection : FontCollection
{
	public InstalledFontCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
