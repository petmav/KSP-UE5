using System.Runtime.InteropServices;
using Unity;

namespace System.Drawing.Imaging;

[StructLayout(LayoutKind.Sequential)]
public sealed class BitmapData
{
	public int Height
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PixelFormat PixelFormat
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PixelFormat);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int Reserved
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public IntPtr Scan0
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(IntPtr);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int Stride
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int Width
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public BitmapData()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
