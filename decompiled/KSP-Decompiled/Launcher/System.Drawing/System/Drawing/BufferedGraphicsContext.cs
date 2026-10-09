using System.Security.Permissions;
using Unity;

namespace System.Drawing;

public sealed class BufferedGraphicsContext : IDisposable
{
	public Size MaximumBuffer
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Size);
		}
		[UIPermission(SecurityAction.Demand, Window = UIPermissionWindow.AllWindows)]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public BufferedGraphicsContext()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public BufferedGraphics Allocate(Graphics targetGraphics, Rectangle targetRectangle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public BufferedGraphics Allocate(IntPtr targetDC, Rectangle targetRectangle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Invalidate()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
