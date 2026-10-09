using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class PerformanceCountersElement : ConfigurationElement
{
	public bool Enabled
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public PerformanceCountersElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
