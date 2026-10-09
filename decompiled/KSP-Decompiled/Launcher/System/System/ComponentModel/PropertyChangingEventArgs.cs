using System.Security.Permissions;

namespace System.ComponentModel;

[HostProtection(SecurityAction.LinkDemand, SharedState = true)]
public class PropertyChangingEventArgs : EventArgs
{
	private readonly string propertyName;

	public virtual string PropertyName => propertyName;

	public PropertyChangingEventArgs(string propertyName)
	{
		this.propertyName = propertyName;
	}
}
