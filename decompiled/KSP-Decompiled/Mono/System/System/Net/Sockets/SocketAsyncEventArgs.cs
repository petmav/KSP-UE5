using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Threading;
using Unity;

namespace System.Net.Sockets;

public class SocketAsyncEventArgs : EventArgs, IDisposable
{
	private bool disposed;

	internal volatile int in_progress;

	internal EndPoint remote_ep;

	internal Socket current_socket;

	internal SocketAsyncResult socket_async_result = new SocketAsyncResult();

	internal IList<ArraySegment<byte>> m_BufferList;

	public Exception ConnectByNameError { get; internal set; }

	public Socket AcceptSocket { get; set; }

	public byte[] Buffer { get; private set; }

	public IList<ArraySegment<byte>> BufferList
	{
		get
		{
			return m_BufferList;
		}
		set
		{
			if (Buffer != null && value != null)
			{
				throw new ArgumentException("Buffer and BufferList properties cannot both be non-null.");
			}
			m_BufferList = value;
		}
	}

	public int BytesTransferred { get; internal set; }

	public int Count { get; internal set; }

	public bool DisconnectReuseSocket { get; set; }

	public SocketAsyncOperation LastOperation { get; private set; }

	public int Offset { get; private set; }

	public EndPoint RemoteEndPoint
	{
		get
		{
			return remote_ep;
		}
		set
		{
			remote_ep = value;
		}
	}

	public IPPacketInformation ReceiveMessageFromPacketInfo { get; private set; }

	public SendPacketsElement[] SendPacketsElements { get; set; }

	public TransmitFileOptions SendPacketsFlags { get; set; }

	[System.MonoTODO("unused property")]
	public int SendPacketsSendSize { get; set; }

	public SocketError SocketError { get; set; }

	public SocketFlags SocketFlags { get; set; }

	public object UserToken { get; set; }

	public Socket ConnectSocket
	{
		get
		{
			SocketError socketError = SocketError;
			if (socketError == SocketError.AccessDenied)
			{
				return null;
			}
			return current_socket;
		}
	}

	internal bool PolicyRestricted { get; private set; }

	public SocketClientAccessPolicyProtocol SocketClientAccessPolicyProtocol
	{
		[CompilerGenerated]
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SocketClientAccessPolicyProtocol);
		}
		[CompilerGenerated]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event EventHandler<SocketAsyncEventArgs> Completed;

	internal SocketAsyncEventArgs(bool policy)
		: this()
	{
		PolicyRestricted = policy;
	}

	public SocketAsyncEventArgs()
	{
		SendPacketsSendSize = -1;
	}

	~SocketAsyncEventArgs()
	{
		Dispose(disposing: false);
	}

	private void Dispose(bool disposing)
	{
		disposed = true;
		if (disposing)
		{
			_ = in_progress;
		}
	}

	public void Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	internal void SetLastOperation(SocketAsyncOperation op)
	{
		if (disposed)
		{
			throw new ObjectDisposedException("System.Net.Sockets.SocketAsyncEventArgs");
		}
		if (Interlocked.Exchange(ref in_progress, 1) != 0)
		{
			throw new InvalidOperationException("Operation already in progress");
		}
		LastOperation = op;
	}

	internal void Complete()
	{
		OnCompleted(this);
	}

	protected virtual void OnCompleted(SocketAsyncEventArgs e)
	{
		e?.Completed?.Invoke(e.current_socket, e);
	}

	public void SetBuffer(int offset, int count)
	{
		SetBuffer(Buffer, offset, count);
	}

	public void SetBuffer(byte[] buffer, int offset, int count)
	{
		if (buffer != null)
		{
			if (BufferList != null)
			{
				throw new ArgumentException("Buffer and BufferList properties cannot both be non-null.");
			}
			int num = buffer.Length;
			if (offset < 0 || (offset != 0 && offset >= num))
			{
				throw new ArgumentOutOfRangeException("offset");
			}
			if (count < 0 || count > num - offset)
			{
				throw new ArgumentOutOfRangeException("count");
			}
			Count = count;
			Offset = offset;
		}
		Buffer = buffer;
	}

	internal void StartOperationCommon(Socket socket)
	{
		current_socket = socket;
	}

	internal void StartOperationWrapperConnect(MultipleConnectAsync args)
	{
		SetLastOperation(SocketAsyncOperation.Connect);
	}

	internal void FinishConnectByNameSyncFailure(Exception exception, int bytesTransferred, SocketFlags flags)
	{
		throw new NotImplementedException();
	}

	internal void FinishOperationAsyncFailure(Exception exception, int bytesTransferred, SocketFlags flags)
	{
		throw new NotImplementedException();
	}

	internal void FinishWrapperConnectSuccess(Socket connectSocket, int bytesTransferred, SocketFlags flags)
	{
		SetResults(SocketError.Success, bytesTransferred, flags);
		current_socket = connectSocket;
		OnCompleted(this);
	}

	internal void SetResults(SocketError socketError, int bytesTransferred, SocketFlags flags)
	{
		SocketError = socketError;
		BytesTransferred = bytesTransferred;
		SocketFlags = flags;
	}
}
