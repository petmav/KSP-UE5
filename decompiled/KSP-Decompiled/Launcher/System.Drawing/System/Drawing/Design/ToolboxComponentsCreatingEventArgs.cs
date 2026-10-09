using System.ComponentModel.Design;
using System.Security.Permissions;
using Unity;

namespace System.Drawing.Design;

[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
public class ToolboxComponentsCreatingEventArgs : EventArgs
{
	public IDesignerHost DesignerHost
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ToolboxComponentsCreatingEventArgs(IDesignerHost host)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
