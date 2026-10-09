using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.ConstrainedExecution;

namespace System.Diagnostics;

[InstallerType(typeof(PerformanceCounterInstaller))]
public sealed class PerformanceCounter : Component, ISupportInitialize
{
	private string categoryName;

	private string counterName;

	private string instanceName;

	private string machineName;

	private IntPtr impl;

	private PerformanceCounterType type;

	private CounterSample old_sample;

	private bool readOnly;

	private bool valid_old;

	private bool changed;

	private bool is_custom;

	private PerformanceCounterInstanceLifetime lifetime;

	[Obsolete]
	public static int DefaultFileMappingSize = 524288;

	[DefaultValue("")]
	[ReadOnly(true)]
	[SRDescription("The category name for this performance counter.")]
	[SettingsBindable(true)]
	[TypeConverter("System.Diagnostics.Design.CategoryValueConverter, System.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
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
				throw new ArgumentNullException("categoryName");
			}
			categoryName = value;
			changed = true;
		}
	}

	[MonitoringDescription("A description describing the counter.")]
	[DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
	[ReadOnly(true)]
	[System.MonoTODO]
	public string CounterHelp => "";

	[SettingsBindable(true)]
	[ReadOnly(true)]
	[DefaultValue("")]
	[TypeConverter("System.Diagnostics.Design.CounterNameConverter, System.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
	[SRDescription("The name of this performance counter.")]
	public string CounterName
	{
		get
		{
			return counterName;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("counterName");
			}
			counterName = value;
			changed = true;
		}
	}

	[MonitoringDescription("The type of the counter.")]
	[DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
	public PerformanceCounterType CounterType
	{
		get
		{
			if (changed)
			{
				UpdateInfo();
			}
			return type;
		}
	}

	[System.MonoTODO]
	[DefaultValue(PerformanceCounterInstanceLifetime.Global)]
	public PerformanceCounterInstanceLifetime InstanceLifetime
	{
		get
		{
			return lifetime;
		}
		set
		{
			lifetime = value;
		}
	}

	[SRDescription("The instance name for this performance counter.")]
	[DefaultValue("")]
	[ReadOnly(true)]
	[SettingsBindable(true)]
	[TypeConverter("System.Diagnostics.Design.InstanceNameConverter, System.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
	public string InstanceName
	{
		get
		{
			return instanceName;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			instanceName = value;
			changed = true;
		}
	}

	[SRDescription("The machine where this performance counter resides.")]
	[SettingsBindable(true)]
	[Browsable(false)]
	[DefaultValue(".")]
	[System.MonoTODO("What's the machine name format?")]
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
			if (value == "" || value == ".")
			{
				machineName = ".";
				changed = true;
				return;
			}
			throw new PlatformNotSupportedException();
		}
	}

	[Browsable(false)]
	[DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
	[MonitoringDescription("The raw value of the counter.")]
	public long RawValue
	{
		get
		{
			if (changed)
			{
				UpdateInfo();
			}
			GetSample(impl, only_value: true, out var sample);
			return sample.RawValue;
		}
		set
		{
			if (changed)
			{
				UpdateInfo();
			}
			if (readOnly)
			{
				throw new InvalidOperationException();
			}
			UpdateValue(impl, do_incr: false, value);
		}
	}

	[MonitoringDescription("The accessability level of the counter.")]
	[DefaultValue(true)]
	[Browsable(false)]
	public bool ReadOnly
	{
		get
		{
			return readOnly;
		}
		set
		{
			readOnly = value;
		}
	}

	public PerformanceCounter()
	{
		categoryName = (counterName = (instanceName = ""));
		machineName = ".";
	}

	public PerformanceCounter(string categoryName, string counterName)
		: this(categoryName, counterName, readOnly: false)
	{
	}

	public PerformanceCounter(string categoryName, string counterName, bool readOnly)
		: this(categoryName, counterName, "", readOnly)
	{
	}

	public PerformanceCounter(string categoryName, string counterName, string instanceName)
		: this(categoryName, counterName, instanceName, readOnly: false)
	{
	}

	public PerformanceCounter(string categoryName, string counterName, string instanceName, bool readOnly)
	{
		if (categoryName == null)
		{
			throw new ArgumentNullException("categoryName");
		}
		if (counterName == null)
		{
			throw new ArgumentNullException("counterName");
		}
		if (instanceName == null)
		{
			throw new ArgumentNullException("instanceName");
		}
		CategoryName = categoryName;
		CounterName = counterName;
		if (categoryName == "" || counterName == "")
		{
			throw new InvalidOperationException();
		}
		InstanceName = instanceName;
		this.instanceName = instanceName;
		machineName = ".";
		this.readOnly = readOnly;
		changed = true;
	}

	public PerformanceCounter(string categoryName, string counterName, string instanceName, string machineName)
		: this(categoryName, counterName, instanceName, readOnly: false)
	{
		this.machineName = machineName;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern IntPtr GetImpl(string category, string counter, string instance, string machine, out PerformanceCounterType ctype, out bool custom);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool GetSample(IntPtr impl, bool only_value, out CounterSample sample);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern long UpdateValue(IntPtr impl, bool do_incr, long value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void FreeData(IntPtr impl);

	private void UpdateInfo()
	{
		if (impl != IntPtr.Zero)
		{
			Close();
		}
		impl = GetImpl(categoryName, counterName, instanceName, machineName, out type, out is_custom);
		if (!is_custom)
		{
			readOnly = true;
		}
		changed = false;
	}

	public void BeginInit()
	{
	}

	public void EndInit()
	{
	}

	public void Close()
	{
		IntPtr intPtr = impl;
		impl = IntPtr.Zero;
		if (intPtr != IntPtr.Zero)
		{
			FreeData(intPtr);
		}
	}

	public static void CloseSharedResources()
	{
	}

	public long Decrement()
	{
		return IncrementBy(-1L);
	}

	protected override void Dispose(bool disposing)
	{
		Close();
	}

	public long Increment()
	{
		return IncrementBy(1L);
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public long IncrementBy(long value)
	{
		if (changed)
		{
			UpdateInfo();
		}
		if (readOnly)
		{
			return 0L;
		}
		return UpdateValue(impl, do_incr: true, value);
	}

	public CounterSample NextSample()
	{
		if (changed)
		{
			UpdateInfo();
		}
		GetSample(impl, only_value: false, out var sample);
		valid_old = true;
		old_sample = sample;
		return sample;
	}

	public float NextValue()
	{
		if (changed)
		{
			UpdateInfo();
		}
		GetSample(impl, only_value: false, out var sample);
		float result = ((!valid_old) ? CounterSampleCalculator.ComputeCounterValue(sample) : CounterSampleCalculator.ComputeCounterValue(old_sample, sample));
		valid_old = true;
		old_sample = sample;
		return result;
	}

	[System.MonoTODO]
	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public void RemoveInstance()
	{
		throw new NotImplementedException();
	}
}
