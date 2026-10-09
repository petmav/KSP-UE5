using System.Security.Permissions;

namespace System.ComponentModel;

[HostProtection(SecurityAction.LinkDemand, SharedState = true)]
public class PropertyChangedEventArgs : EventArgs
{
	private readonly string propertyName;

	public virtual string PropertyName => propertyName;

	public PropertyChangedEventArgs(string propertyName)
	{
		this.propertyName = propertyName;
	}
}
