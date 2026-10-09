using System.Runtime.InteropServices;
using System.Security.Permissions;

namespace System.ComponentModel.Design;

[ComVisible(true)]
[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[HostProtection(SecurityAction.LinkDemand, SharedState = true)]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class ComponentRenameEventArgs : EventArgs
{
	private object component;

	private string oldName;

	private string newName;

	public object Component => component;

	public virtual string OldName => oldName;

	public virtual string NewName => newName;

	public ComponentRenameEventArgs(object component, string oldName, string newName)
	{
		this.oldName = oldName;
		this.newName = newName;
		this.component = component;
	}
}
