using System.ComponentModel;
using System.Drawing.Design;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Drawing;

[Serializable]
[TypeConverter(typeof(ImageConverter))]
[ImmutableObject(true)]
[Editor("System.Drawing.Design.ImageEditor, System.Drawing.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a", typeof(UITypeEditor))]
[ComVisible(true)]
public abstract class Image : MarshalByRefObject, ICloneable, IDisposable, ISerializable
{
	public delegate bool GetThumbnailImageAbort();

	public int Flags
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public Guid[] FrameDimensionsList
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
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

	public float HorizontalResolution
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
	}

	public ColorPalette Palette
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public SizeF PhysicalDimension
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SizeF);
		}
	}

	public PixelFormat PixelFormat
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PixelFormat);
		}
	}

	public int[] PropertyIdList
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public PropertyItem[] PropertyItems
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ImageFormat RawFormat
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
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

	public object Tag
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public float VerticalResolution
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
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

	internal Image()
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

	protected virtual void Dispose(bool disposing)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static Image FromFile(string filename)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Image FromFile(string filename, bool useEmbeddedColorManagement)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Bitmap FromHbitmap(IntPtr hbitmap)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Bitmap FromHbitmap(IntPtr hbitmap, IntPtr hpalette)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Image FromStream(Stream stream)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Image FromStream(Stream stream, bool useEmbeddedColorManagement)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Image FromStream(Stream stream, bool useEmbeddedColorManagement, bool validateImageData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public RectangleF GetBounds(ref GraphicsUnit pageUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(RectangleF);
	}

	public EncoderParameters GetEncoderParameterList(Guid encoder)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public int GetFrameCount(FrameDimension dimension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public static int GetPixelFormatSize(PixelFormat pixfmt)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public PropertyItem GetPropertyItem(int propid)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public Image GetThumbnailImage(int thumbWidth, int thumbHeight, GetThumbnailImageAbort callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static bool IsAlphaPixelFormat(PixelFormat pixfmt)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool IsCanonicalPixelFormat(PixelFormat pixfmt)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool IsExtendedPixelFormat(PixelFormat pixfmt)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void RemovePropertyItem(int propid)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RotateFlip(RotateFlipType rotateFlipType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Save(Stream stream, ImageCodecInfo encoder, EncoderParameters encoderParams)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Save(Stream stream, ImageFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Save(string filename)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Save(string filename, ImageCodecInfo encoder, EncoderParameters encoderParams)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Save(string filename, ImageFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SaveAdd(Image image, EncoderParameters encoderParams)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SaveAdd(EncoderParameters encoderParams)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int SelectActiveFrame(FrameDimension dimension, int frameIndex)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void SetPropertyItem(PropertyItem propitem)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.SerializationFormatter)]
	void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
