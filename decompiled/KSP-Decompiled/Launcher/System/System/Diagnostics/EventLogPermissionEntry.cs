using Unity;

namespace System.Diagnostics;

[Serializable]
public class EventLogPermissionEntry
{
	public string MachineName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public EventLogPermissionAccess PermissionAccess
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(EventLogPermissionAccess);
		}
	}

	public EventLogPermissionEntry(EventLogPermissionAccess permissionAccess, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
