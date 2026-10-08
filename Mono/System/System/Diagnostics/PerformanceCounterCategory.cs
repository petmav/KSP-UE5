using System.Runtime.CompilerServices;
using System.Security.Permissions;

namespace System.Diagnostics;

[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
public sealed class PerformanceCounterCategory
{
	private string categoryName;

	private string machineName;

	private PerformanceCounterCategoryType type = PerformanceCounterCategoryType.Unknown;

	public string CategoryHelp
	{
		get
		{
			string text = CategoryHelpInternal(categoryName, machineName);
			if (text != null)
			{
				return text;
			}
			throw new InvalidOperationException();
		}
	}

	public string CategoryName
	{
		get
		{
			return categoryName;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			if (value == "")
			{
				throw new ArgumentException("value");
			}
			categoryName = value;
		}
	}

	public string MachineName
	{
		get
		{
			return machineName;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			if (value == "")
			{
				throw new ArgumentException("value");
			}
			machineName = value;
		}
	}

	public PerformanceCounterCategoryType CategoryType => type;

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool CategoryDelete(string name);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern string CategoryHelpInternal(string category, string machine);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool CounterCategoryExists(string counter, string category, string machine);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool Create(string categoryName, string categoryHelp, PerformanceCounterCategoryType categoryType, CounterCreationData[] items);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern int InstanceExistsInternal(string instance, string category, string machine);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern string[] GetCategoryNames(string machine);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern string[] GetCounterNames(string category, string machine);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern string[] GetInstanceNames(string category, string machine);

	private static void CheckCategory(string categoryName)
	{
		if (categoryName == null)
		{
			throw new ArgumentNullException("categoryName");
		}
		if (categoryName == "")
		{
			throw new ArgumentException("categoryName");
		}
	}

	public PerformanceCounterCategory()
		: this("", ".")
	{
	}

	public PerformanceCounterCategory(string categoryName)
		: this(categoryName, ".")
	{
	}

	public PerformanceCounterCategory(string categoryName, string machineName)
	{
		CheckCategory(categoryName);
		if (machineName == null)
		{
			throw new ArgumentNullException("machineName");
		}
		this.categoryName = categoryName;
		this.machineName = machineName;
	}

	public bool CounterExists(string counterName)
	{
		return CounterExists(counterName, categoryName, machineName);
	}

	public static bool CounterExists(string counterName, string categoryName)
	{
		return CounterExists(counterName, categoryName, ".");
	}

	public static bool CounterExists(string counterName, string categoryName, string machineName)
	{
		if (counterName == null)
		{
			throw new ArgumentNullException("counterName");
		}
		CheckCategory(categoryName);
		if (machineName == null)
		{
			throw new ArgumentNullException("machineName");
		}
		return CounterCategoryExists(counterName, categoryName, machineName);
	}

	[Obsolete("Use another overload that uses PerformanceCounterCategoryType instead")]
	public static PerformanceCounterCategory Create(string categoryName, string categoryHelp, CounterCreationDataCollection counterData)
	{
		return Create(categoryName, categoryHelp, PerformanceCounterCategoryType.Unknown, counterData);
	}

	[Obsolete("Use another overload that uses PerformanceCounterCategoryType instead")]
	public static PerformanceCounterCategory Create(string categoryName, string categoryHelp, string counterName, string counterHelp)
	{
		return Create(categoryName, categoryHelp, PerformanceCounterCategoryType.Unknown, counterName, counterHelp);
	}

	public static PerformanceCounterCategory Create(string categoryName, string categoryHelp, PerformanceCounterCategoryType categoryType, CounterCreationDataCollection counterData)
	{
		CheckCategory(categoryName);
		if (counterData == null)
		{
			throw new ArgumentNullException("counterData");
		}
		if (counterData.Count == 0)
		{
			throw new ArgumentException("counterData");
		}
		CounterCreationData[] array = new CounterCreationData[counterData.Count];
		counterData.CopyTo(array, 0);
		if (!Create(categoryName, categoryHelp, categoryType, array))
		{
			throw new InvalidOperationException();
		}
		return new PerformanceCounterCategory(categoryName, categoryHelp);
	}

	public static PerformanceCounterCategory Create(string categoryName, string categoryHelp, PerformanceCounterCategoryType categoryType, string counterName, string counterHelp)
	{
		CheckCategory(categoryName);
		if (!Create(categoryName, categoryHelp, categoryType, new CounterCreationData[1]
		{
			new CounterCreationData(counterName, counterHelp, PerformanceCounterType.NumberOfItems32)
		}))
		{
			throw new InvalidOperationException();
		}
		return new PerformanceCounterCategory(categoryName, categoryHelp);
	}

	public static void Delete(string categoryName)
	{
		CheckCategory(categoryName);
		if (!CategoryDelete(categoryName))
		{
			throw new InvalidOperationException();
		}
	}

	public static bool Exists(string categoryName)
	{
		return Exists(categoryName, ".");
	}

	public static bool Exists(string categoryName, string machineName)
	{
		CheckCategory(categoryName);
		return CounterCategoryExists(null, categoryName, machineName);
	}

	public static PerformanceCounterCategory[] GetCategories()
	{
		return GetCategories(".");
	}

	public static PerformanceCounterCategory[] GetCategories(string machineName)
	{
		if (machineName == null)
		{
			throw new ArgumentNullException("machineName");
		}
		string[] categoryNames = GetCategoryNames(machineName);
		PerformanceCounterCategory[] array = new PerformanceCounterCategory[categoryNames.Length];
		for (int i = 0; i < categoryNames.Length; i++)
		{
			array[i] = new PerformanceCounterCategory(categoryNames[i], machineName);
		}
		return array;
	}

	public PerformanceCounter[] GetCounters()
	{
		return GetCounters("");
	}

	public PerformanceCounter[] GetCounters(string instanceName)
	{
		string[] counterNames = GetCounterNames(categoryName, machineName);
		PerformanceCounter[] array = new PerformanceCounter[counterNames.Length];
		for (int i = 0; i < counterNames.Length; i++)
		{
			array[i] = new PerformanceCounter(categoryName, counterNames[i], instanceName, machineName);
		}
		return array;
	}

	public string[] GetInstanceNames()
	{
		return GetInstanceNames(categoryName, machineName);
	}

	public bool InstanceExists(string instanceName)
	{
		return InstanceExists(instanceName, categoryName, machineName);
	}

	public static bool InstanceExists(string instanceName, string categoryName)
	{
		return InstanceExists(instanceName, categoryName, ".");
	}

	public static bool InstanceExists(string instanceName, string categoryName, string machineName)
	{
		if (instanceName == null)
		{
			throw new ArgumentNullException("instanceName");
		}
		CheckCategory(categoryName);
		if (machineName == null)
		{
			throw new ArgumentNullException("machineName");
		}
		return InstanceExistsInternal(instanceName, categoryName, machineName) switch
		{
			0 => false, 
			1 => true, 
			_ => throw new InvalidOperationException(), 
		};
	}

	[System.MonoTODO]
	public InstanceDataCollectionCollection ReadCategory()
	{
		throw new NotImplementedException();
	}
}
