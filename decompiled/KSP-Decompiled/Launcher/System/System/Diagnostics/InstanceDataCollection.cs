using System.Collections;
using Unity;

namespace System.Diagnostics;

public class InstanceDataCollection : DictionaryBase
{
	public string CounterName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public InstanceData this[string instanceName]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ICollection Keys
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ICollection Values
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	[Obsolete("This constructor has been deprecated.  Please use System.Diagnostics.InstanceDataCollectionCollection.get_Item to get an instance of this collection instead.  http://go.microsoft.com/fwlink/?linkid=14202")]
	public InstanceDataCollection(string counterName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(string instanceName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(InstanceData[] instances, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
