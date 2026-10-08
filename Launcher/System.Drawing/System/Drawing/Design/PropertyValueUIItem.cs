using System.Security.Permissions;
using Unity;

namespace System.Drawing.Design;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class PropertyValueUIItem
{
	public virtual Image Image
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public virtual PropertyValueUIItemInvokeHandler InvokeHandler
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public virtual string ToolTip
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public PropertyValueUIItem(Image uiItemImage, PropertyValueUIItemInvokeHandler handler, string tooltip)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void Reset()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
