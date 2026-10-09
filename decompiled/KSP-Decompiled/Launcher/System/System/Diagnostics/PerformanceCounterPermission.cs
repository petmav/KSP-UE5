using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[Serializable]
public sealed class PerformanceCounterPermission : ResourcePermissionBase
{
	public PerformanceCounterPermissionEntryCollection PermissionEntries
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public PerformanceCounterPermission()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PerformanceCounterPermission(PerformanceCounterPermissionAccess permissionAccess, string machineName, string categoryName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PerformanceCounterPermission(PerformanceCounterPermissionEntry[] permissionAccessEntries)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PerformanceCounterPermission(PermissionState state)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
