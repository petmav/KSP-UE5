using System.Runtime.InteropServices;
using Unity;

namespace System.Drawing.Imaging;

[StructLayout(LayoutKind.Sequential)]
public sealed class MetafileHeader
{
	public Rectangle Bounds
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Rectangle);
		}
	}

	public float DpiX
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
	}

	public float DpiY
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
	}

	public int EmfPlusHeaderSize
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public int LogicalDpiX
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public int LogicalDpiY
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public int MetafileSize
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public MetafileType Type
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(MetafileType);
		}
	}

	public int Version
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public MetaHeader WmfHeader
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal MetafileHeader()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool IsDisplay()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsEmf()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsEmfOrEmfPlus()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsEmfPlus()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsEmfPlusDual()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsEmfPlusOnly()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsWmf()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsWmfPlaceable()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
