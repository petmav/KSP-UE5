using System.ComponentModel;
using Unity;

namespace System.Diagnostics;

[Serializable]
[TypeConverter("System.Diagnostics.Design.CounterCreationDataConverter, System.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
public class CounterCreationData
{
	public string CounterHelp
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

	public string CounterName
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

	public PerformanceCounterType CounterType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PerformanceCounterType);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CounterCreationData()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CounterCreationData(string counterName, string counterHelp, PerformanceCounterType counterType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
