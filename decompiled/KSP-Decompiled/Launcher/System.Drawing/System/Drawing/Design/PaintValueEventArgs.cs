using System.ComponentModel;
using System.Security.Permissions;
using Unity;

namespace System.Drawing.Design;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class PaintValueEventArgs : EventArgs
{
	public Rectangle Bounds
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Rectangle);
		}
	}

	public ITypeDescriptorContext Context
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Graphics Graphics
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public object Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public PaintValueEventArgs(ITypeDescriptorContext context, object value, Graphics graphics, Rectangle bounds)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
