using System.Threading;

namespace System.Net;

internal class DnsAsyncResult : IAsyncResult
{
	private static WaitCallback internal_cb = CB;

	private ManualResetEvent handle;

	private bool synch;

	private bool is_completed;

	private AsyncCallback callback;

	private object state;

	private IPHostEntry entry;

	private Exception exc;

	public object AsyncState => state;

	public WaitHandle AsyncWaitHandle
	{
		get
		{
			lock (this)
			{
				if (handle == null)
				{
					handle = new ManualResetEvent(is_completed);
				}
			}
			return handle;
		}
	}

	public Exception Exception => exc;

	public IPHostEntry HostEntry => entry;

	public bool CompletedSynchronously => synch;

	public bool IsCompleted
	{
		get
		{
			lock (this)
			{
				return is_completed;
			}
		}
	}

	public DnsAsyncResult(AsyncCallback cb, object state)
	{
		callback = cb;
		this.state = state;
	}

	public void SetCompleted(bool synch, IPHostEntry entry, Exception e)
	{
		this.synch = synch;
		this.entry = entry;
		exc = e;
		lock (this)
		{
			if (is_completed)
			{
				return;
			}
			is_completed = true;
			if (handle != null)
			{
				handle.Set();
			}
		}
		if (callback != null)
		{
			ThreadPool.QueueUserWorkItem(internal_cb, this);
		}
	}

	public void SetCompleted(bool synch, Exception e)
	{
		SetCompleted(synch, null, e);
	}

	public void SetCompleted(bool synch, IPHostEntry entry)
	{
		SetCompleted(synch, entry, null);
	}

	private static void CB(object _this)
	{
		DnsAsyncResult dnsAsyncResult = (DnsAsyncResult)_this;
		dnsAsyncResult.callback(dnsAsyncResult);
	}
}
