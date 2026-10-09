using System.Drawing.Text;
using Unity;

namespace System.Drawing;

public sealed class FontFamily : MarshalByRefObject, IDisposable
{
	public static FontFamily[] Families
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static FontFamily GenericMonospace
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static FontFamily GenericSansSerif
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static FontFamily GenericSerif
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Name
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public FontFamily(GenericFontFamilies genericFamily)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public FontFamily(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public FontFamily(string name, FontCollection fontCollection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int GetCellAscent(FontStyle style)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int GetCellDescent(FontStyle style)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int GetEmHeight(FontStyle style)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	[Obsolete("Do not use method GetFamilies, use property Families instead")]
	public static FontFamily[] GetFamilies(Graphics graphics)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public int GetLineSpacing(FontStyle style)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public string GetName(int language)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public bool IsStyleAvailable(FontStyle style)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
