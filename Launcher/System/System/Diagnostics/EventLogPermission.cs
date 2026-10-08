using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[Serializable]
public sealed class EventLogPermission : ResourcePermissionBase
{
	public EventLogPermissionEntryCollection PermissionEntries
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public EventLogPermission()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogPermission(EventLogPermissionAccess permissionAccess, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogPermission(EventLogPermissionEntry[] permissionAccessEntries)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogPermission(PermissionState state)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
