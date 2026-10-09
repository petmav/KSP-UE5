using System.Security.Permissions;

namespace System.Diagnostics;

[Serializable]
public sealed class EventLogPermission : ResourcePermissionBase
{
	private EventLogPermissionEntryCollection innerCollection;

	public EventLogPermissionEntryCollection PermissionEntries
	{
		get
		{
			if (innerCollection == null)
			{
				innerCollection = new EventLogPermissionEntryCollection(this);
			}
			return innerCollection;
		}
	}

	public EventLogPermission()
	{
		SetUp();
	}

	public EventLogPermission(EventLogPermissionEntry[] permissionAccessEntries)
	{
		if (permissionAccessEntries == null)
		{
			throw new ArgumentNullException("permissionAccessEntries");
		}
		SetUp();
		innerCollection = new EventLogPermissionEntryCollection(this);
		innerCollection.AddRange(permissionAccessEntries);
	}

	public EventLogPermission(PermissionState state)
		: base(state)
	{
		SetUp();
	}

	public EventLogPermission(EventLogPermissionAccess permissionAccess, string machineName)
	{
		SetUp();
		innerCollection = new EventLogPermissionEntryCollection(this);
		innerCollection.Add(new EventLogPermissionEntry(permissionAccess, machineName));
	}

	private void SetUp()
	{
		base.TagNames = new string[1] { "Machine" };
		base.PermissionAccessType = typeof(EventLogPermissionAccess);
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
		EventLogPermissionEntry eventLogPermissionEntry = obj as EventLogPermissionEntry;
		AddPermissionAccess(eventLogPermissionEntry.CreateResourcePermissionBaseEntry());
	}

	internal void Remove(object obj)
	{
		EventLogPermissionEntry eventLogPermissionEntry = obj as EventLogPermissionEntry;
		RemovePermissionAccess(eventLogPermissionEntry.CreateResourcePermissionBaseEntry());
	}
}
