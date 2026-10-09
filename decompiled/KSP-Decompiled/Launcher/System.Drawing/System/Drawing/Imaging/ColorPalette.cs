using Unity;

namespace System.Drawing.Imaging;

public sealed class ColorPalette
{
	public Color[] Entries
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public int Flags
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	internal ColorPalette()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
