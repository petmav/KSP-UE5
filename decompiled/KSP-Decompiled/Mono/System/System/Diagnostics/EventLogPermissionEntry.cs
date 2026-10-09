using System.Security.Permissions;

namespace System.Diagnostics;

[Serializable]
public class EventLogPermissionEntry
{
	private EventLogPermissionAccess permissionAccess;

	private string machineName;

	public string MachineName => machineName;

	public EventLogPermissionAccess PermissionAccess => permissionAccess;

	public EventLogPermissionEntry(EventLogPermissionAccess permissionAccess, string machineName)
	{
		ResourcePermissionBase.ValidateMachineName(machineName);
		this.permissionAccess = permissionAccess;
		this.machineName = machineName;
	}

	internal ResourcePermissionBaseEntry CreateResourcePermissionBaseEntry()
	{
		return new ResourcePermissionBaseEntry((int)permissionAccess, new string[1] { machineName });
	}
}
