using System.Collections;
using Unity;

namespace System.Diagnostics;

[Serializable]
public class EventLogPermissionEntryCollection : CollectionBase
{
	public EventLogPermissionEntry this[int index]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	internal EventLogPermissionEntryCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(EventLogPermissionEntry value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void AddRange(EventLogPermissionEntryCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(EventLogPermissionEntry[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(EventLogPermissionEntry value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(EventLogPermissionEntry[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(EventLogPermissionEntry value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, EventLogPermissionEntry value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(EventLogPermissionEntry value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
