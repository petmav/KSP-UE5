using System.Collections;

namespace System.Diagnostics;

public class InstanceDataCollectionCollection : DictionaryBase
{
	public InstanceDataCollection this[string counterName]
	{
		get
		{
			CheckNull(counterName, "counterName");
			return (InstanceDataCollection)base.Dictionary[counterName];
		}
	}

	public ICollection Keys => base.Dictionary.Keys;

	public ICollection Values => base.Dictionary.Values;

	private static void CheckNull(object value, string name)
	{
		if (value == null)
		{
			throw new ArgumentNullException(name);
		}
	}

	[Obsolete("Use PerformanceCounterCategory.ReadCategory()")]
	public InstanceDataCollectionCollection()
	{
	}

	public bool Contains(string counterName)
	{
		CheckNull(counterName, "counterName");
		return base.Dictionary.Contains(counterName);
	}

	public void CopyTo(InstanceDataCollection[] counters, int index)
	{
		base.Dictionary.CopyTo(counters, index);
	}
}
