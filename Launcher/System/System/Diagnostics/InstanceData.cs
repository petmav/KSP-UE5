using Unity;

namespace System.Diagnostics;

public class InstanceData
{
	public string InstanceName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public long RawValue
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public CounterSample Sample
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(CounterSample);
		}
	}

	public InstanceData(string instanceName, CounterSample sample)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
