using System.Threading;

namespace System.Net;

internal class SimpleAsyncResult : IAsyncResult
{
	private ManualResetEvent handle;

	private bool synch;

	private bool isCompleted;

	private readonly SimpleAsyncCallback cb;

	private object state;

	private bool callbackDone;

	private Exception exc;

	private object locker = new object();

	private bool? user_read_synch;

	public object AsyncState => state;

	public WaitHandle AsyncWaitHandle
	{
		get
		{
			lock (locker)
			{
				if (handle == null)
				{
					handle = new ManualResetEvent(isCompleted);
				}
			}
			return handle;
		}
	}

	public bool CompletedSynchronously
	{
		get
		{
			if (user_read_synch.HasValue)
			{
				return user_read_synch.Value;
			}
			user_read_synch = synch;
			return user_read_synch.Value;
		}
	}

	internal bool CompletedSynchronouslyPeek => synch;

	public bool IsCompleted
	{
		get
		{
			lock (locker)
			{
				return isCompleted;
			}
		}
	}

	internal bool GotException => exc != null;

	internal Exception Exception => exc;

	private SimpleAsyncResult(SimpleAsyncCallback cb)
	{
		this.cb = cb;
	}

	protected SimpleAsyncResult(AsyncCallback cb, object state)
	{
		SimpleAsyncResult ar = this;
		this.state = state;
		this.cb = delegate
		{
			if (cb != null)
			{
				cb(ar);
			}
		};
	}

	public static void Run(Func<SimpleAsyncResult, bool> func, SimpleAsyncCallback callback)
	{
		SimpleAsyncResult simpleAsyncResult = new SimpleAsyncResult(callback);
		try
		{
			if (!func(simpleAsyncResult))
			{
				simpleAsyncResult.SetCompleted(synch: true);
			}
		}
		catch (Exception e)
		{
			simpleAsyncResult.SetCompleted(synch: true, e);
		}
	}

	public static void RunWithLock(object locker, Func<SimpleAsyncResult, bool> func, SimpleAsyncCallback callback)
	{
		Run(delegate(SimpleAsyncResult inner)
		{
			bool num = func(inner);
			if (num)
			{
				Monitor.Exit(locker);
			}
			return num;
		}, delegate(SimpleAsyncResult inner)
		{
			if (inner.GotException)
			{
				if (inner.synch)
				{
					Monitor.Exit(locker);
				}
				callback(inner);
				return;
			}
			try
			{
				if (!inner.synch)
				{
					Monitor.Enter(locker);
				}
				callback(inner);
			}
			finally
			{
				Monitor.Exit(locker);
			}
		});
	}

	protected void Reset_internal()
	{
		callbackDone = false;
		exc = null;
		lock (locker)
		{
			isCompleted = false;
			if (handle != null)
			{
				handle.Reset();
			}
		}
	}

	internal void SetCompleted(bool synch, Exception e)
	{
		SetCompleted_internal(synch, e);
		DoCallback_private();
	}

	internal void SetCompleted(bool synch)
	{
		SetCompleted_internal(synch);
		DoCallback_private();
	}

	private void SetCompleted_internal(bool synch, Exception e)
	{
		this.synch = synch;
		exc = e;
		lock (locker)
		{
			isCompleted = true;
			if (handle != null)
			{
				handle.Set();
			}
		}
	}

	protected void SetCompleted_internal(bool synch)
	{
		SetCompleted_internal(synch, null);
	}

	private void DoCallback_private()
	{
		if (!callbackDone)
		{
			callbackDone = true;
			if (cb != null)
			{
				cb(this);
			}
		}
	}

	protected void DoCallback_internal()
	{
		if (!callbackDone && cb != null)
		{
			callbackDone = true;
			cb(this);
		}
	}

	internal void WaitUntilComplete()
	{
		if (!IsCompleted)
		{
			AsyncWaitHandle.WaitOne();
		}
	}

	internal bool WaitUntilComplete(int timeout, bool exitContext)
	{
		if (IsCompleted)
		{
			return true;
		}
		return AsyncWaitHandle.WaitOne(timeout, exitContext);
	}
}
