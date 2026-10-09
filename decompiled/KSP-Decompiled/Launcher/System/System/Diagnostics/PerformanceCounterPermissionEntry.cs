using Unity;

namespace System.Diagnostics;

[Serializable]
public class PerformanceCounterPermissionEntry
{
	public string CategoryName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string MachineName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public PerformanceCounterPermissionAccess PermissionAccess
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PerformanceCounterPermissionAccess);
		}
	}

	public PerformanceCounterPermissionEntry(PerformanceCounterPermissionAccess permissionAccess, string machineName, string categoryName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
