using System.Security.Permissions;

namespace System.Diagnostics;

[Serializable]
public class PerformanceCounterPermissionEntry
{
	private const PerformanceCounterPermissionAccess All = PerformanceCounterPermissionAccess.Administer;

	private PerformanceCounterPermissionAccess permissionAccess;

	private string machineName;

	private string categoryName;

	public string CategoryName => categoryName;

	public string MachineName => machineName;

	public PerformanceCounterPermissionAccess PermissionAccess => permissionAccess;

	public PerformanceCounterPermissionEntry(PerformanceCounterPermissionAccess permissionAccess, string machineName, string categoryName)
	{
		if (machineName == null)
		{
			throw new ArgumentNullException("machineName");
		}
		if ((permissionAccess | PerformanceCounterPermissionAccess.Administer) != PerformanceCounterPermissionAccess.Administer)
		{
			throw new ArgumentException("permissionAccess");
		}
		ResourcePermissionBase.ValidateMachineName(machineName);
		if (categoryName == null)
		{
			throw new ArgumentNullException("categoryName");
		}
		this.permissionAccess = permissionAccess;
		this.machineName = machineName;
		this.categoryName = categoryName;
	}

	internal ResourcePermissionBaseEntry CreateResourcePermissionBaseEntry()
	{
		return new ResourcePermissionBaseEntry((int)permissionAccess, new string[2] { machineName, categoryName });
	}
}
