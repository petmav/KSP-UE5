using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[HostProtection(SecurityAction.LinkDemand, Synchronization = true, SharedState = true)]
public sealed class PerformanceCounterCategory
{
	public string CategoryHelp
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

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

	public PerformanceCounterCategoryType CategoryType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PerformanceCounterCategoryType);
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

	public PerformanceCounterCategory()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PerformanceCounterCategory(string categoryName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PerformanceCounterCategory(string categoryName, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool CounterExists(string counterName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool CounterExists(string counterName, string categoryName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool CounterExists(string counterName, string categoryName, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[Obsolete("This method has been deprecated.  Please use System.Diagnostics.PerformanceCounterCategory.Create(string categoryName, string categoryHelp, PerformanceCounterCategoryType categoryType, CounterCreationDataCollection counterData) instead.  http://go.microsoft.com/fwlink/?linkid=14202")]
	public static PerformanceCounterCategory Create(string categoryName, string categoryHelp, CounterCreationDataCollection counterData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static PerformanceCounterCategory Create(string categoryName, string categoryHelp, PerformanceCounterCategoryType categoryType, CounterCreationDataCollection counterData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static PerformanceCounterCategory Create(string categoryName, string categoryHelp, PerformanceCounterCategoryType categoryType, string counterName, string counterHelp)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[Obsolete("This method has been deprecated.  Please use System.Diagnostics.PerformanceCounterCategory.Create(string categoryName, string categoryHelp, PerformanceCounterCategoryType categoryType, string counterName, string counterHelp) instead.  http://go.microsoft.com/fwlink/?linkid=14202")]
	public static PerformanceCounterCategory Create(string categoryName, string categoryHelp, string counterName, string counterHelp)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static void Delete(string categoryName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static bool Exists(string categoryName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool Exists(string categoryName, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static PerformanceCounterCategory[] GetCategories()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static PerformanceCounterCategory[] GetCategories(string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public PerformanceCounter[] GetCounters()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public PerformanceCounter[] GetCounters(string instanceName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string[] GetInstanceNames()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public bool InstanceExists(string instanceName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool InstanceExists(string instanceName, string categoryName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool InstanceExists(string instanceName, string categoryName, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public InstanceDataCollectionCollection ReadCategory()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
