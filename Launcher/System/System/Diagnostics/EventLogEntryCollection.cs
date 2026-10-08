using System.Collections;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Diagnostics;

public class EventLogEntryCollection : ICollection, IEnumerable
{
	public int Count
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public virtual EventLogEntry this[int index]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal EventLogEntryCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	bool ICollection.get_IsSynchronized()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SpecialName]
	object ICollection.get_SyncRoot()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void CopyTo(EventLogEntry[] entries, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	void ICollection.CopyTo(Array array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
