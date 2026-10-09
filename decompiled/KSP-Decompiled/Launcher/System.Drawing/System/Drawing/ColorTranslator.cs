using Unity;

namespace System.Drawing;

public sealed class ColorTranslator
{
	internal ColorTranslator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static Color FromHtml(string htmlColor)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(Color);
	}

	public static Color FromOle(int oleColor)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(Color);
	}

	public static Color FromWin32(int win32Color)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(Color);
	}

	public static string ToHtml(Color c)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static int ToOle(Color c)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public static int ToWin32(Color c)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}
}
