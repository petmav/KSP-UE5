using System.ComponentModel;
using System.Drawing.Design;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using Unity;

namespace System.Drawing;

[Serializable]
[ComVisible(true)]
[Editor("System.Drawing.Design.BitmapEditor, System.Drawing.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a", typeof(UITypeEditor))]
public sealed class Bitmap : Image
{
	public Bitmap(Image original)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap(Image original, Size newSize)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap(Image original, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap(int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap(int width, int height, Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap(int width, int height, PixelFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap(int width, int height, int stride, PixelFormat format, IntPtr scan0)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap(Stream stream)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap(Stream stream, bool useIcm)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap(string filename)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap(string filename, bool useIcm)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap(Type type, string resource)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Bitmap Clone(Rectangle rect, PixelFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public Bitmap Clone(RectangleF rect, PixelFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Bitmap FromHicon(IntPtr hicon)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Bitmap FromResource(IntPtr hinstance, string bitmapName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public IntPtr GetHbitmap()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public IntPtr GetHbitmap(Color background)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public IntPtr GetHicon()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	public Color GetPixel(int x, int y)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(Color);
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public BitmapData LockBits(Rectangle rect, ImageLockMode flags, PixelFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public BitmapData LockBits(Rectangle rect, ImageLockMode flags, PixelFormat format, BitmapData bitmapData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void MakeTransparent()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void MakeTransparent(Color transparentColor)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetPixel(int x, int y, Color color)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetResolution(float xDpi, float yDpi)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public void UnlockBits(BitmapData bitmapdata)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
