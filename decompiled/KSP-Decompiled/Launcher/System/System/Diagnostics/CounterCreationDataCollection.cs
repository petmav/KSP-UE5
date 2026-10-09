using System.Collections;
using Unity;

namespace System.Diagnostics;

[Serializable]
public class CounterCreationDataCollection : CollectionBase
{
	public CounterCreationData this[int index]
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

	public CounterCreationDataCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CounterCreationDataCollection(CounterCreationDataCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CounterCreationDataCollection(CounterCreationData[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CounterCreationData value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void AddRange(CounterCreationDataCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CounterCreationData[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CounterCreationData value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CounterCreationData[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CounterCreationData value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CounterCreationData value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void Remove(CounterCreationData value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
