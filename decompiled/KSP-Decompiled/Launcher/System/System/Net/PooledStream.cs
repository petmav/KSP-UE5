using System.IO;
using System.Net.Sockets;
using System.Security.Permissions;
using System.Threading;
using System.Threading.Tasks;

namespace System.Net;

internal class PooledStream : Stream
{
	private bool m_CheckLifetime;

	private TimeSpan m_Lifetime;

	private DateTime m_CreateTime;

	private bool m_ConnectionIsDoomed;

	private ConnectionPool m_ConnectionPool;

	private WeakReference m_Owner;

	private int m_PooledCount;

	private bool m_Initalizing;

	private IPAddress m_ServerAddress;

	private NetworkStream m_NetworkStream;

	private Socket m_AbortSocket;

	private Socket m_AbortSocket6;

	private bool m_JustConnected;

	private GeneralAsyncDelegate m_AsyncCallback;

	internal bool JustConnected
	{
		get
		{
			if (m_JustConnected)
			{
				m_JustConnected = false;
				return true;
			}
			return false;
		}
	}

	internal IPAddress ServerAddress => m_ServerAddress;

	internal bool IsInitalizing => m_Initalizing;

	internal bool CanBePooled
	{
		get
		{
			if (m_Initalizing)
			{
				return true;
			}
			if (!m_NetworkStream.Connected)
			{
				return false;
			}
			WeakReference owner = m_Owner;
			if (!m_ConnectionIsDoomed)
			{
				if (owner != null)
				{
					return !owner.IsAlive;
				}
				return true;
			}
			return false;
		}
		set
		{
			m_ConnectionIsDoomed |= !value;
		}
	}

	internal bool IsEmancipated
	{
		get
		{
			WeakReference owner = m_Owner;
			if (0 >= m_PooledCount)
			{
				if (owner != null)
				{
					return !owner.IsAlive;
				}
				return true;
			}
			return false;
		}
	}

	internal object Owner
	{
		get
		{
			WeakReference owner = m_Owner;
			if (owner != null && owner.IsAlive)
			{
				return owner.Target;
			}
			return null;
		}
		set
		{
			lock (this)
			{
				if (m_Owner != null)
				{
					m_Owner.Target = value;
				}
			}
		}
	}

	internal ConnectionPool Pool => m_ConnectionPool;

	internal virtual ServicePoint ServicePoint => Pool.ServicePoint;

	protected bool UsingSecureStream => false;

	internal NetworkStream NetworkStream
	{
		get
		{
			return m_NetworkStream;
		}
		set
		{
			m_Initalizing = false;
			m_NetworkStream = value;
		}
	}

	protected Socket Socket => m_NetworkStream.InternalSocket;

	public override bool CanRead => m_NetworkStream.CanRead;

	public override bool CanSeek => m_NetworkStream.CanSeek;

	public override bool CanWrite => m_NetworkStream.CanWrite;

	public override bool CanTimeout => m_NetworkStream.CanTimeout;

	public override int ReadTimeout
	{
		get
		{
			return m_NetworkStream.ReadTimeout;
		}
		set
		{
			m_NetworkStream.ReadTimeout = value;
		}
	}

	public override int WriteTimeout
	{
		get
		{
			return m_NetworkStream.WriteTimeout;
		}
		set
		{
			m_NetworkStream.WriteTimeout = value;
		}
	}

	public override long Length => m_NetworkStream.Length;

	public override long Position
	{
		get
		{
			return m_NetworkStream.Position;
		}
		set
		{
			m_NetworkStream.Position = value;
		}
	}

	internal PooledStream(object owner)
	{
		m_Owner = new WeakReference(owner);
		m_PooledCount = -1;
		m_Initalizing = true;
		m_NetworkStream = new NetworkStream();
		m_CreateTime = DateTime.UtcNow;
	}

	internal PooledStream(ConnectionPool connectionPool, TimeSpan lifetime, bool checkLifetime)
	{
		m_ConnectionPool = connectionPool;
		m_Lifetime = lifetime;
		m_CheckLifetime = checkLifetime;
		m_Initalizing = true;
		m_NetworkStream = new NetworkStream();
		m_CreateTime = DateTime.UtcNow;
	}

	internal bool Activate(object owningObject, GeneralAsyncDelegate asyncCallback)
	{
		return Activate(owningObject, asyncCallback != null, asyncCallback);
	}

	protected bool Activate(object owningObject, bool async, GeneralAsyncDelegate asyncCallback)
	{
		try
		{
			if (m_Initalizing)
			{
				IPAddress address = null;
				m_AsyncCallback = asyncCallback;
				Socket connection = ServicePoint.GetConnection(this, owningObject, async, out address, ref m_AbortSocket, ref m_AbortSocket6);
				if (connection != null)
				{
					_ = Logging.On;
					m_NetworkStream.InitNetworkStream(connection, FileAccess.ReadWrite);
					m_ServerAddress = address;
					m_Initalizing = false;
					m_JustConnected = true;
					m_AbortSocket = null;
					m_AbortSocket6 = null;
					return true;
				}
				return false;
			}
			if (async)
			{
				asyncCallback?.Invoke(owningObject, this);
			}
			return true;
		}
		catch
		{
			m_Initalizing = false;
			throw;
		}
	}

	internal void Deactivate()
	{
		m_AsyncCallback = null;
		if (!m_ConnectionIsDoomed && m_CheckLifetime)
		{
			CheckLifetime();
		}
	}

	internal virtual void ConnectionCallback(object owningObject, Exception e, Socket socket, IPAddress address)
	{
		object obj = null;
		if (e != null)
		{
			m_Initalizing = false;
			obj = e;
		}
		else
		{
			try
			{
				_ = Logging.On;
				m_NetworkStream.InitNetworkStream(socket, FileAccess.ReadWrite);
				obj = this;
			}
			catch (Exception ex)
			{
				if (NclUtilities.IsFatal(ex))
				{
					throw;
				}
				obj = ex;
			}
			m_ServerAddress = address;
			m_Initalizing = false;
			m_JustConnected = true;
		}
		if (m_AsyncCallback != null)
		{
			m_AsyncCallback(owningObject, obj);
		}
		m_AbortSocket = null;
		m_AbortSocket6 = null;
	}

	protected void CheckLifetime()
	{
		if (!m_ConnectionIsDoomed)
		{
			TimeSpan t = DateTime.UtcNow.Subtract(m_CreateTime);
			m_ConnectionIsDoomed = 0 < TimeSpan.Compare(m_Lifetime, t);
		}
	}

	internal void UpdateLifetime()
	{
		int connectionLeaseTimeout = ServicePoint.ConnectionLeaseTimeout;
		TimeSpan timeSpan;
		if (connectionLeaseTimeout == -1)
		{
			timeSpan = TimeSpan.MaxValue;
			m_CheckLifetime = false;
		}
		else
		{
			timeSpan = new TimeSpan(0, 0, 0, 0, connectionLeaseTimeout);
			m_CheckLifetime = true;
		}
		if (timeSpan != m_Lifetime)
		{
			m_Lifetime = timeSpan;
		}
	}

	internal void PrePush(object expectedOwner)
	{
		lock (this)
		{
			if (expectedOwner == null)
			{
				if (m_Owner != null && m_Owner.Target != null)
				{
					throw new InternalException();
				}
			}
			else if (m_Owner == null || m_Owner.Target != expectedOwner)
			{
				throw new InternalException();
			}
			m_PooledCount++;
			if (1 != m_PooledCount)
			{
				throw new InternalException();
			}
			if (m_Owner != null)
			{
				m_Owner.Target = null;
			}
		}
	}

	internal void PostPop(object newOwner)
	{
		lock (this)
		{
			if (m_Owner == null)
			{
				m_Owner = new WeakReference(newOwner);
			}
			else
			{
				if (m_Owner.Target != null)
				{
					throw new InternalException();
				}
				m_Owner.Target = newOwner;
			}
			m_PooledCount--;
			if (Pool != null)
			{
				if (m_PooledCount != 0)
				{
					throw new InternalException();
				}
			}
			else if (-1 != m_PooledCount)
			{
				throw new InternalException();
			}
		}
	}

	public override long Seek(long offset, SeekOrigin origin)
	{
		return m_NetworkStream.Seek(offset, origin);
	}

	public override int Read(byte[] buffer, int offset, int size)
	{
		return m_NetworkStream.Read(buffer, offset, size);
	}

	public override void Write(byte[] buffer, int offset, int size)
	{
		m_NetworkStream.Write(buffer, offset, size);
	}

	internal void MultipleWrite(BufferOffsetSize[] buffers)
	{
		m_NetworkStream.MultipleWrite(buffers);
	}

	protected override void Dispose(bool disposing)
	{
		try
		{
			if (disposing)
			{
				m_Owner = null;
				m_ConnectionIsDoomed = true;
				CloseSocket();
			}
		}
		finally
		{
			base.Dispose(disposing);
		}
	}

	internal void CloseSocket()
	{
		Socket abortSocket = m_AbortSocket;
		Socket abortSocket2 = m_AbortSocket6;
		m_NetworkStream.Close();
		abortSocket?.Close();
		abortSocket2?.Close();
	}

	public void Close(int timeout)
	{
		Socket abortSocket = m_AbortSocket;
		Socket abortSocket2 = m_AbortSocket6;
		m_NetworkStream.Close(timeout);
		abortSocket?.Close(timeout);
		abortSocket2?.Close(timeout);
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public override IAsyncResult BeginRead(byte[] buffer, int offset, int size, AsyncCallback callback, object state)
	{
		return m_NetworkStream.BeginRead(buffer, offset, size, callback, state);
	}

	internal virtual IAsyncResult UnsafeBeginRead(byte[] buffer, int offset, int size, AsyncCallback callback, object state)
	{
		return m_NetworkStream.UnsafeBeginRead(buffer, offset, size, callback, state);
	}

	public override int EndRead(IAsyncResult asyncResult)
	{
		return m_NetworkStream.EndRead(asyncResult);
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public override IAsyncResult BeginWrite(byte[] buffer, int offset, int size, AsyncCallback callback, object state)
	{
		return m_NetworkStream.BeginWrite(buffer, offset, size, callback, state);
	}

	internal virtual IAsyncResult UnsafeBeginWrite(byte[] buffer, int offset, int size, AsyncCallback callback, object state)
	{
		return m_NetworkStream.UnsafeBeginWrite(buffer, offset, size, callback, state);
	}

	public override void EndWrite(IAsyncResult asyncResult)
	{
		m_NetworkStream.EndWrite(asyncResult);
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	internal IAsyncResult BeginMultipleWrite(BufferOffsetSize[] buffers, AsyncCallback callback, object state)
	{
		return m_NetworkStream.BeginMultipleWrite(buffers, callback, state);
	}

	internal void EndMultipleWrite(IAsyncResult asyncResult)
	{
		m_NetworkStream.EndMultipleWrite(asyncResult);
	}

	public override void Flush()
	{
		m_NetworkStream.Flush();
	}

	public override Task FlushAsync(CancellationToken cancellationToken)
	{
		return m_NetworkStream.FlushAsync(cancellationToken);
	}

	public override void SetLength(long value)
	{
		m_NetworkStream.SetLength(value);
	}

	internal void SetSocketTimeoutOption(SocketShutdown mode, int timeout, bool silent)
	{
		m_NetworkStream.SetSocketTimeoutOption(mode, timeout, silent);
	}

	internal bool Poll(int microSeconds, SelectMode mode)
	{
		return m_NetworkStream.Poll(microSeconds, mode);
	}

	internal bool PollRead()
	{
		return m_NetworkStream.PollRead();
	}
}
