using System.ComponentModel;
using System.Runtime.ConstrainedExecution;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[InstallerType("System.Diagnostics.PerformanceCounterInstaller,System.Configuration.Install, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
[HostProtection(SecurityAction.LinkDemand, Synchronization = true, SharedState = true)]
public sealed class PerformanceCounter : Component, ISupportInitialize
{
	[Obsolete("This field has been deprecated and is not used.  Use machine.config or an application configuration file to set the size of the PerformanceCounter file mapping.")]
	public static int DefaultFileMappingSize;

	public string CategoryName
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

	public string CounterHelp
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
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
	}

	public PerformanceCounterInstanceLifetime InstanceLifetime
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PerformanceCounterInstanceLifetime);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string InstanceName
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

	public string MachineName
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

	public long RawValue
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool ReadOnly
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

	public PerformanceCounter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PerformanceCounter(string categoryName, string counterName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PerformanceCounter(string categoryName, string counterName, bool readOnly)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PerformanceCounter(string categoryName, string counterName, string instanceName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PerformanceCounter(string categoryName, string counterName, string instanceName, bool readOnly)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PerformanceCounter(string categoryName, string counterName, string instanceName, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void BeginInit()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Close()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void CloseSharedResources()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public long Decrement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(long);
	}

	public void EndInit()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public long Increment()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(long);
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public long IncrementBy(long value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(long);
	}

	public CounterSample NextSample()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(CounterSample);
	}

	public float NextValue()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(float);
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public void RemoveInstance()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
