using System.Security.Permissions;

namespace System.ComponentModel;

[HostProtection(SecurityAction.LinkDemand, SharedState = true)]
public class DataErrorsChangedEventArgs : EventArgs
{
	private readonly string propertyName;

	public virtual string PropertyName => propertyName;

	public DataErrorsChangedEventArgs(string propertyName)
	{
		this.propertyName = propertyName;
	}
}
