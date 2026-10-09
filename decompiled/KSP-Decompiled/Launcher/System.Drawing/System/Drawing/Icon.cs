using System.ComponentModel;
using System.Drawing.Design;
using System.IO;
using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Drawing;

[Serializable]
[TypeConverter(typeof(IconConverter))]
[Editor("System.Drawing.Design.IconEditor, System.Drawing.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a", typeof(UITypeEditor))]
public sealed class Icon : MarshalByRefObject, ICloneable, IDisposable, ISerializable
{
	public IntPtr Handle
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(IntPtr);
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

	public Size Size
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Size);
		}
	}

	public int Width
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public Icon(Icon original, Size size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Icon(Icon original, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Icon(Stream stream)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Icon(Stream stream, Size size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Icon(Stream stream, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Icon(string fileName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Icon(string fileName, Size size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Icon(string fileName, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Icon(Type type, string resource)
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

	public static Icon ExtractAssociatedIcon(string filePath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Icon FromHandle(IntPtr handle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Save(Stream outputStream)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.SerializationFormatter)]
	void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap ToBitmap()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
