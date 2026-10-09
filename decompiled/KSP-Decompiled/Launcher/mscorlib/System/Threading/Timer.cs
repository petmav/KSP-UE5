using System.Collections;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace System.Threading;

[ComVisible(true)]
public sealed class Timer : MarshalByRefObject, IDisposable
{
	private sealed class TimerComparer : IComparer
	{
		public int Compare(object x, object y)
		{
			if (!(x is Timer timer))
			{
				return -1;
			}
			if (!(y is Timer timer2))
			{
				return 1;
			}
			long num = timer.next_run - timer2.next_run;
			if (num == 0L)
			{
				if (x != y)
				{
					return -1;
				}
				return 0;
			}
			if (num <= 0)
			{
				return -1;
			}
			return 1;
		}
	}

	private sealed class Scheduler
	{
		private static Scheduler instance;

		private SortedList list;

		private ManualResetEvent changed;

		public static Scheduler Instance => instance;

		static Scheduler()
		{
			instance = new Scheduler();
		}

		private Scheduler()
		{
			changed = new ManualResetEvent(initialState: false);
			list = new SortedList(new TimerComparer(), 1024);
			Thread thread = new Thread(SchedulerThread);
			thread.IsBackground = true;
			thread.Start();
		}

		public void Remove(Timer timer)
		{
			if (timer.next_run == 0L || timer.next_run == long.MaxValue)
			{
				return;
			}
			lock (this)
			{
				InternalRemove(timer);
			}
		}

		public void Change(Timer timer, long new_next_run)
		{
			bool flag = false;
			lock (this)
			{
				InternalRemove(timer);
				if (new_next_run == long.MaxValue)
				{
					timer.next_run = new_next_run;
					return;
				}
				if (!timer.disposed)
				{
					timer.next_run = new_next_run;
					Add(timer);
					flag = list.GetByIndex(0) == timer;
				}
			}
			if (flag)
			{
				changed.Set();
			}
		}

		private int FindByDueTime(long nr)
		{
			int i = 0;
			int num = list.Count - 1;
			if (num < 0)
			{
				return -1;
			}
			if (num < 20)
			{
				for (; i <= num; i++)
				{
					Timer timer = (Timer)list.GetByIndex(i);
					if (timer.next_run == nr)
					{
						return i;
					}
					if (timer.next_run > nr)
					{
						return -1;
					}
				}
				return -1;
			}
			while (i <= num)
			{
				int num2 = i + (num - i >> 1);
				Timer timer2 = (Timer)list.GetByIndex(num2);
				if (nr == timer2.next_run)
				{
					return num2;
				}
				if (nr > timer2.next_run)
				{
					i = num2 + 1;
				}
				else
				{
					num = num2 - 1;
				}
			}
			return -1;
		}

		private void Add(Timer timer)
		{
			int num = FindByDueTime(timer.next_run);
			if (num != -1)
			{
				bool flag = ((long.MaxValue - timer.next_run > 20000) ? true : false);
				do
				{
					num++;
					if (flag)
					{
						timer.next_run++;
					}
					else
					{
						timer.next_run--;
					}
				}
				while (num < list.Count && ((Timer)list.GetByIndex(num)).next_run == timer.next_run);
			}
			list.Add(timer, timer);
		}

		private int InternalRemove(Timer timer)
		{
			int num = list.IndexOfKey(timer);
			if (num >= 0)
			{
				list.RemoveAt(num);
			}
			return num;
		}

		private static void TimerCB(object o)
		{
			Timer timer = (Timer)o;
			timer.callback(timer.state);
		}

		private void SchedulerThread()
		{
			Thread.CurrentThread.Name = "Timer-Scheduler";
			List<Timer> list = new List<Timer>(512);
			while (true)
			{
				int num = -1;
				long timeMonotonic = GetTimeMonotonic();
				lock (this)
				{
					changed.Reset();
					int num2 = this.list.Count;
					int num3;
					for (num3 = 0; num3 < num2; num3++)
					{
						Timer timer = (Timer)this.list.GetByIndex(num3);
						if (timer.next_run > timeMonotonic)
						{
							break;
						}
						this.list.RemoveAt(num3);
						num2--;
						num3--;
						ThreadPool.UnsafeQueueUserWorkItem(TimerCB, timer);
						long period_ms = timer.period_ms;
						long due_time_ms = timer.due_time_ms;
						if (period_ms == -1 || ((period_ms == 0L || period_ms == -1) && due_time_ms != -1))
						{
							timer.next_run = long.MaxValue;
						}
						else
						{
							timer.next_run = GetTimeMonotonic() + 10000 * timer.period_ms;
							list.Add(timer);
						}
					}
					num2 = list.Count;
					for (num3 = 0; num3 < num2; num3++)
					{
						Timer timer2 = list[num3];
						Add(timer2);
					}
					list.Clear();
					ShrinkIfNeeded(list, 512);
					int capacity = this.list.Capacity;
					num2 = this.list.Count;
					if (capacity > 1024 && num2 > 0 && capacity / num2 > 3)
					{
						this.list.Capacity = num2 * 2;
					}
					long num4 = long.MaxValue;
					if (this.list.Count > 0)
					{
						num4 = ((Timer)this.list.GetByIndex(0)).next_run;
					}
					num = -1;
					if (num4 != long.MaxValue)
					{
						long num5 = (num4 - GetTimeMonotonic()) / 10000;
						if (num5 > int.MaxValue)
						{
							num = 2147483646;
						}
						else
						{
							num = (int)num5;
							if (num < 0)
							{
								num = 0;
							}
						}
					}
				}
				changed.WaitOne(num);
			}
		}

		private void ShrinkIfNeeded(List<Timer> list, int initial)
		{
			int capacity = list.Capacity;
			int count = list.Count;
			if (capacity > initial && count > 0 && capacity / count > 3)
			{
				list.Capacity = count * 2;
			}
		}
	}

	private static readonly Scheduler scheduler = Scheduler.Instance;

	private TimerCallback callback;

	private object state;

	private long due_time_ms;

	private long period_ms;

	private long next_run;

	private bool disposed;

	private const long MaxValue = 4294967294L;

	public Timer(TimerCallback callback, object state, int dueTime, int period)
	{
		Init(callback, state, dueTime, period);
	}

	public Timer(TimerCallback callback, object state, long dueTime, long period)
	{
		Init(callback, state, dueTime, period);
	}

	public Timer(TimerCallback callback, object state, TimeSpan dueTime, TimeSpan period)
	{
		Init(callback, state, (long)dueTime.TotalMilliseconds, (long)period.TotalMilliseconds);
	}

	[CLSCompliant(false)]
	public Timer(TimerCallback callback, object state, uint dueTime, uint period)
	{
		Init(callback, state, (dueTime == uint.MaxValue) ? (-1L) : ((long)dueTime), (period == uint.MaxValue) ? (-1L) : ((long)period));
	}

	public Timer(TimerCallback callback)
	{
		Init(callback, this, -1L, -1L);
	}

	private void Init(TimerCallback callback, object state, long dueTime, long period)
	{
		if (callback == null)
		{
			throw new ArgumentNullException("callback");
		}
		this.callback = callback;
		this.state = state;
		Change(dueTime, period, first: true);
	}

	public bool Change(int dueTime, int period)
	{
		return Change(dueTime, period, first: false);
	}

	public bool Change(TimeSpan dueTime, TimeSpan period)
	{
		return Change((long)dueTime.TotalMilliseconds, (long)period.TotalMilliseconds, first: false);
	}

	[CLSCompliant(false)]
	public bool Change(uint dueTime, uint period)
	{
		long dueTime2 = ((dueTime == uint.MaxValue) ? (-1L) : ((long)dueTime));
		long period2 = ((period == uint.MaxValue) ? (-1L) : ((long)period));
		return Change(dueTime2, period2, first: false);
	}

	public void Dispose()
	{
		if (!disposed)
		{
			disposed = true;
			scheduler.Remove(this);
		}
	}

	public bool Change(long dueTime, long period)
	{
		return Change(dueTime, period, first: false);
	}

	private bool Change(long dueTime, long period, bool first)
	{
		if (dueTime > 4294967294u)
		{
			throw new ArgumentOutOfRangeException("dueTime", "Due time too large");
		}
		if (period > 4294967294u)
		{
			throw new ArgumentOutOfRangeException("period", "Period too large");
		}
		if (dueTime < -1)
		{
			throw new ArgumentOutOfRangeException("dueTime");
		}
		if (period < -1)
		{
			throw new ArgumentOutOfRangeException("period");
		}
		if (disposed)
		{
			throw new ObjectDisposedException(null, Environment.GetResourceString("Cannot access a disposed object."));
		}
		due_time_ms = dueTime;
		period_ms = period;
		long new_next_run;
		if (dueTime == 0L)
		{
			new_next_run = 0L;
		}
		else if (dueTime < 0)
		{
			new_next_run = long.MaxValue;
			if (first)
			{
				next_run = new_next_run;
				return true;
			}
		}
		else
		{
			new_next_run = dueTime * 10000 + GetTimeMonotonic();
		}
		scheduler.Change(this, new_next_run);
		return true;
	}

	public bool Dispose(WaitHandle notifyObject)
	{
		if (notifyObject == null)
		{
			throw new ArgumentNullException("notifyObject");
		}
		Dispose();
		NativeEventCalls.SetEvent(notifyObject.SafeWaitHandle);
		return true;
	}

	internal void KeepRootedWhileScheduled()
	{
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern long GetTimeMonotonic();
}
