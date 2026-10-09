using System.ComponentModel;
using System.ComponentModel.Design;
using System.Security.Permissions;
using System.Threading;

namespace System.Timers;

[DefaultEvent("Elapsed")]
[DefaultProperty("Interval")]
[HostProtection(SecurityAction.LinkDemand, Synchronization = true, ExternalThreading = true)]
public class Timer : Component, ISupportInitialize
{
	private double interval;

	private bool enabled;

	private bool initializing;

	private bool delayedEnable;

	private ElapsedEventHandler onIntervalElapsed;

	private bool autoReset;

	private ISynchronizeInvoke synchronizingObject;

	private bool disposed;

	private System.Threading.Timer timer;

	private TimerCallback callback;

	private object cookie;

	[Category("Behavior")]
	[TimersDescription("Indicates whether the timer will be restarted when it is enabled.")]
	[DefaultValue(true)]
	public bool AutoReset
	{
		get
		{
			return autoReset;
		}
		set
		{
			if (base.DesignMode)
			{
				autoReset = value;
			}
			else if (autoReset != value)
			{
				autoReset = value;
				if (timer != null)
				{
					UpdateTimer();
				}
			}
		}
	}

	[Category("Behavior")]
	[DefaultValue(false)]
	[TimersDescription("Indicates whether the timer is enabled to fire events at a defined interval.")]
	public bool Enabled
	{
		get
		{
			return enabled;
		}
		set
		{
			if (base.DesignMode)
			{
				delayedEnable = value;
				enabled = value;
			}
			else if (initializing)
			{
				delayedEnable = value;
			}
			else
			{
				if (enabled == value)
				{
					return;
				}
				if (!value)
				{
					if (timer != null)
					{
						cookie = null;
						timer.Dispose();
						timer = null;
					}
					enabled = value;
					return;
				}
				enabled = value;
				if (timer == null)
				{
					if (disposed)
					{
						throw new ObjectDisposedException(GetType().Name);
					}
					int num = CalculateRoundedInterval(interval);
					cookie = new object();
					timer = new System.Threading.Timer(callback, cookie, num, autoReset ? num : (-1));
				}
				else
				{
					UpdateTimer();
				}
			}
		}
	}

	[SettingsBindable(true)]
	[DefaultValue(100.0)]
	[TimersDescription("The number of milliseconds between timer events.")]
	[Category("Behavior")]
	public double Interval
	{
		get
		{
			return interval;
		}
		set
		{
			if (value <= 0.0)
			{
				throw new ArgumentException(global::SR.GetString("'{0}' is not a valid value for 'Interval'. 'Interval' must be greater than {1}.", value, 0));
			}
			interval = value;
			if (timer != null)
			{
				UpdateTimer();
			}
		}
	}

	public override ISite Site
	{
		get
		{
			return base.Site;
		}
		set
		{
			base.Site = value;
			if (base.DesignMode)
			{
				enabled = true;
			}
		}
	}

	[TimersDescription("The object used to marshal the event handler calls issued when an interval has elapsed.")]
	[Browsable(false)]
	[DefaultValue(null)]
	public ISynchronizeInvoke SynchronizingObject
	{
		get
		{
			if (synchronizingObject == null && base.DesignMode)
			{
				IDesignerHost designerHost = (IDesignerHost)GetService(typeof(IDesignerHost));
				if (designerHost != null)
				{
					object rootComponent = designerHost.RootComponent;
					if (rootComponent != null && rootComponent is ISynchronizeInvoke)
					{
						synchronizingObject = (ISynchronizeInvoke)rootComponent;
					}
				}
			}
			return synchronizingObject;
		}
		set
		{
			synchronizingObject = value;
		}
	}

	[Category("Behavior")]
	[TimersDescription("Occurs when the Interval has elapsed.")]
	public event ElapsedEventHandler Elapsed
	{
		add
		{
			onIntervalElapsed = (ElapsedEventHandler)Delegate.Combine(onIntervalElapsed, value);
		}
		remove
		{
			onIntervalElapsed = (ElapsedEventHandler)Delegate.Remove(onIntervalElapsed, value);
		}
	}

	public Timer()
	{
		interval = 100.0;
		enabled = false;
		autoReset = true;
		initializing = false;
		delayedEnable = false;
		callback = MyTimerCallback;
	}

	public Timer(double interval)
		: this()
	{
		if (interval <= 0.0)
		{
			throw new ArgumentException(global::SR.GetString("Invalid value '{1}' for parameter '{0}'.", "interval", interval));
		}
		this.interval = CalculateRoundedInterval(interval, argumentCheck: true);
	}

	private static int CalculateRoundedInterval(double interval, bool argumentCheck = false)
	{
		double num = Math.Ceiling(interval);
		if (num > 2147483647.0 || num <= 0.0)
		{
			if (argumentCheck)
			{
				throw new ArgumentException(global::SR.GetString("Invalid value '{1}' for parameter '{0}'.", "interval", interval));
			}
			throw new ArgumentOutOfRangeException(global::SR.GetString("Invalid value '{1}' for parameter '{0}'.", "interval", interval));
		}
		return (int)num;
	}

	private void UpdateTimer()
	{
		int num = CalculateRoundedInterval(interval);
		timer.Change(num, autoReset ? num : (-1));
	}

	public void BeginInit()
	{
		Close();
		initializing = true;
	}

	public void Close()
	{
		initializing = false;
		delayedEnable = false;
		enabled = false;
		if (timer != null)
		{
			timer.Dispose();
			timer = null;
		}
	}

	protected override void Dispose(bool disposing)
	{
		Close();
		disposed = true;
		base.Dispose(disposing);
	}

	public void EndInit()
	{
		initializing = false;
		Enabled = delayedEnable;
	}

	public void Start()
	{
		Enabled = true;
	}

	public void Stop()
	{
		Enabled = false;
	}

	private void MyTimerCallback(object state)
	{
		if (state != cookie)
		{
			return;
		}
		if (!autoReset)
		{
			enabled = false;
		}
		ElapsedEventArgs elapsedEventArgs = new ElapsedEventArgs(DateTime.Now);
		try
		{
			ElapsedEventHandler elapsedEventHandler = onIntervalElapsed;
			if (elapsedEventHandler != null)
			{
				if (SynchronizingObject != null && SynchronizingObject.InvokeRequired)
				{
					SynchronizingObject.BeginInvoke(elapsedEventHandler, new object[2] { this, elapsedEventArgs });
				}
				else
				{
					elapsedEventHandler(this, elapsedEventArgs);
				}
			}
		}
		catch
		{
		}
	}
}
