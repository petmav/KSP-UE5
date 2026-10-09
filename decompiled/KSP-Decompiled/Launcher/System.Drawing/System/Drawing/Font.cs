using System.ComponentModel;
using System.Drawing.Design;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Drawing;

[Serializable]
[Editor("System.Drawing.Design.FontEditor, System.Drawing.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a", typeof(UITypeEditor))]
[TypeConverter(typeof(FontConverter))]
[ComVisible(true)]
public sealed class Font : MarshalByRefObject, ICloneable, IDisposable, ISerializable
{
	public bool Bold
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public FontFamily FontFamily
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public byte GdiCharSet
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(byte);
		}
	}

	public bool GdiVerticalFont
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public int Height
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public bool IsSystemFont
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool Italic
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
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

	public string OriginalFontName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public float Size
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
	}

	public float SizeInPoints
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
	}

	public bool Strikeout
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public FontStyle Style
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(FontStyle);
		}
	}

	public string SystemFontName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool Underline
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public GraphicsUnit Unit
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(GraphicsUnit);
		}
	}

	public Font(Font prototype, FontStyle newStyle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(FontFamily family, float emSize)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(FontFamily family, float emSize, FontStyle style)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(FontFamily family, float emSize, FontStyle style, GraphicsUnit unit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(FontFamily family, float emSize, FontStyle style, GraphicsUnit unit, byte gdiCharSet)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(FontFamily family, float emSize, FontStyle style, GraphicsUnit unit, byte gdiCharSet, bool gdiVerticalFont)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(FontFamily family, float emSize, GraphicsUnit unit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(string familyName, float emSize)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(string familyName, float emSize, FontStyle style)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(string familyName, float emSize, FontStyle style, GraphicsUnit unit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(string familyName, float emSize, FontStyle style, GraphicsUnit unit, byte gdiCharSet)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(string familyName, float emSize, FontStyle style, GraphicsUnit unit, byte gdiCharSet, bool gdiVerticalFont)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Font(string familyName, float emSize, GraphicsUnit unit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static Font FromHdc(IntPtr hdc)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Font FromHfont(IntPtr hfont)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Font FromLogFont(object lf)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Font FromLogFont(object lf, IntPtr hdc)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public float GetHeight()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(float);
	}

	public float GetHeight(Graphics graphics)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(float);
	}

	public float GetHeight(float dpi)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(float);
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.SerializationFormatter)]
	void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IntPtr ToHfont()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	public void ToLogFont(object logFont)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ToLogFont(object logFont, Graphics graphics)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
