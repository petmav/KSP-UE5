using System.ComponentModel;
using System.Security.Permissions;
using Unity;

namespace System.Drawing.Design;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class ToolboxComponentsCreatedEventArgs : EventArgs
{
	public IComponent[] Components
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ToolboxComponentsCreatedEventArgs(IComponent[] components)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
