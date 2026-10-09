using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Permissions;

namespace System.Threading;

[DebuggerDisplay("Initial Count={InitialCount}, Current Count={CurrentCount}")]
[ComVisible(false)]
[HostProtection(SecurityAction.LinkDemand, Synchronization = true, ExternalThreading = true)]
public class CountdownEvent : IDisposable
{
	private int m_initialCount;

	private volatile int m_currentCount;

	private ManualResetEventSlim m_event;

	private volatile bool m_disposed;

	public int CurrentCount
	{
		get
		{
			int currentCount = m_currentCount;
			if (currentCount >= 0)
			{
				return currentCount;
			}
			return 0;
		}
	}

	public int InitialCount => m_initialCount;

	public bool IsSet => m_currentCount <= 0;

	public WaitHandle WaitHandle
	{
		get
		{
			ThrowIfDisposed();
			return m_event.WaitHandle;
		}
	}

	public CountdownEvent(int initialCount)
	{
		if (initialCount < 0)
		{
			throw new ArgumentOutOfRangeException("initialCount");
		}
		m_initialCount = initialCount;
		m_currentCount = initialCount;
		m_event = new ManualResetEventSlim();
		if (initialCount == 0)
		{
			m_event.Set();
		}
	}

	public void Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	protected virtual void Dispose(bool disposing)
	{
		if (disposing)
		{
			m_event.Dispose();
			m_disposed = true;
		}
	}

	public bool Signal()
	{
		ThrowIfDisposed();
		if (m_currentCount <= 0)
		{
			throw new InvalidOperationException(Environment.GetResourceString("Invalid attempt made to decrement the event's count below zero."));
		}
		int num = Interlocked.Decrement(ref m_currentCount);
		if (num == 0)
		{
			m_event.Set();
			return true;
		}
		if (num < 0)
		{
			throw new InvalidOperationException(Environment.GetResourceString("Invalid attempt made to decrement the event's count below zero."));
		}
		return false;
	}

	public bool Signal(int signalCount)
	{
		if (signalCount <= 0)
		{
			throw new ArgumentOutOfRangeException("signalCount");
		}
		ThrowIfDisposed();
		SpinWait spinWait = default(SpinWait);
		int currentCount;
		while (true)
		{
			currentCount = m_currentCount;
			if (currentCount < signalCount)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Invalid attempt made to decrement the event's count below zero."));
			}
			if (Interlocked.CompareExchange(ref m_currentCount, currentCount - signalCount, currentCount) == currentCount)
			{
				break;
			}
			spinWait.SpinOnce();
		}
		if (currentCount == signalCount)
		{
			m_event.Set();
			return true;
		}
		return false;
	}

	public void AddCount()
	{
		AddCount(1);
	}

	public bool TryAddCount()
	{
		return TryAddCount(1);
	}

	public void AddCount(int signalCount)
	{
		if (!TryAddCount(signalCount))
		{
			throw new InvalidOperationException(Environment.GetResourceString("The event is already signaled and cannot be incremented."));
		}
	}

	public bool TryAddCount(int signalCount)
	{
		if (signalCount <= 0)
		{
			throw new ArgumentOutOfRangeException("signalCount");
		}
		ThrowIfDisposed();
		SpinWait spinWait = default(SpinWait);
		while (true)
		{
			int currentCount = m_currentCount;
			if (currentCount <= 0)
			{
				return false;
			}
			if (currentCount > int.MaxValue - signalCount)
			{
				throw new InvalidOperationException(Environment.GetResourceString("The increment operation would cause the CurrentCount to overflow."));
			}
			if (Interlocked.CompareExchange(ref m_currentCount, currentCount + signalCount, currentCount) == currentCount)
			{
				break;
			}
			spinWait.SpinOnce();
		}
		return true;
	}

	public void Reset()
	{
		Reset(m_initialCount);
	}

	public void Reset(int count)
	{
		ThrowIfDisposed();
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count");
		}
		m_currentCount = count;
		m_initialCount = count;
		if (count == 0)
		{
			m_event.Set();
		}
		else
		{
			m_event.Reset();
		}
	}

	public void Wait()
	{
		Wait(-1, default(CancellationToken));
	}

	public void Wait(CancellationToken cancellationToken)
	{
		Wait(-1, cancellationToken);
	}

	public bool Wait(TimeSpan timeout)
	{
		long num = (long)timeout.TotalMilliseconds;
		if (num < -1 || num > int.MaxValue)
		{
			throw new ArgumentOutOfRangeException("timeout");
		}
		return Wait((int)num, default(CancellationToken));
	}

	public bool Wait(TimeSpan timeout, CancellationToken cancellationToken)
	{
		long num = (long)timeout.TotalMilliseconds;
		if (num < -1 || num > int.MaxValue)
		{
			throw new ArgumentOutOfRangeException("timeout");
		}
		return Wait((int)num, cancellationToken);
	}

	public bool Wait(int millisecondsTimeout)
	{
		return Wait(millisecondsTimeout, default(CancellationToken));
	}

	public bool Wait(int millisecondsTimeout, CancellationToken cancellationToken)
	{
		if (millisecondsTimeout < -1)
		{
			throw new ArgumentOutOfRangeException("millisecondsTimeout");
		}
		ThrowIfDisposed();
		cancellationToken.ThrowIfCancellationRequested();
		bool flag = IsSet;
		if (!flag)
		{
			flag = m_event.Wait(millisecondsTimeout, cancellationToken);
		}
		return flag;
	}

	private void ThrowIfDisposed()
	{
		if (m_disposed)
		{
			throw new ObjectDisposedException("CountdownEvent");
		}
	}
}
