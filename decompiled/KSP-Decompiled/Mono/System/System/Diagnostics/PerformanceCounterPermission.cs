using System.Security.Permissions;

namespace System.Diagnostics;

[Serializable]
public sealed class PerformanceCounterPermission : ResourcePermissionBase
{
	private PerformanceCounterPermissionEntryCollection innerCollection;

	public PerformanceCounterPermissionEntryCollection PermissionEntries
	{
		get
		{
			if (innerCollection == null)
			{
				innerCollection = new PerformanceCounterPermissionEntryCollection(this);
			}
			return innerCollection;
		}
	}

	public PerformanceCounterPermission()
	{
		SetUp();
	}

	public PerformanceCounterPermission(PerformanceCounterPermissionEntry[] permissionAccessEntries)
	{
		if (permissionAccessEntries == null)
		{
			throw new ArgumentNullException("permissionAccessEntries");
		}
		SetUp();
		innerCollection = new PerformanceCounterPermissionEntryCollection(this);
		innerCollection.AddRange(permissionAccessEntries);
	}

	public PerformanceCounterPermission(PermissionState state)
		: base(state)
	{
		SetUp();
	}

	public PerformanceCounterPermission(PerformanceCounterPermissionAccess permissionAccess, string machineName, string categoryName)
	{
		SetUp();
		innerCollection = new PerformanceCounterPermissionEntryCollection(this);
		innerCollection.Add(new PerformanceCounterPermissionEntry(permissionAccess, machineName, categoryName));
	}

	private void SetUp()
	{
		base.TagNames = new string[2] { "Machine", "Category" };
		base.PermissionAccessType = typeof(PerformanceCounterPermissionAccess);
	}

	internal ResourcePermissionBaseEntry[] GetEntries()
	{
		return GetPermissionEntries();
	}

	internal void ClearEntries()
	{
		Clear();
	}

	internal void Add(object obj)
	{
		PerformanceCounterPermissionEntry performanceCounterPermissionEntry = obj as PerformanceCounterPermissionEntry;
		AddPermissionAccess(performanceCounterPermissionEntry.CreateResourcePermissionBaseEntry());
	}

	internal void Remove(object obj)
	{
		PerformanceCounterPermissionEntry performanceCounterPermissionEntry = obj as PerformanceCounterPermissionEntry;
		RemovePermissionAccess(performanceCounterPermissionEntry.CreateResourcePermissionBaseEntry());
	}
}
