using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Net.Configuration;
using System.Net.NetworkInformation;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Threading;
using System.Threading.Tasks;
using Mono;

namespace System.Net.Sockets;

public class Socket : IDisposable
{
	private delegate void SendFileHandler(string fileName, byte[] preBuffer, byte[] postBuffer, TransmitFileOptions flags);

	private sealed class SendFileAsyncResult : IAsyncResult
	{
		private IAsyncResult ares;

		private SendFileHandler d;

		public object AsyncState => ares.AsyncState;

		public WaitHandle AsyncWaitHandle => ares.AsyncWaitHandle;

		public bool CompletedSynchronously => ares.CompletedSynchronously;

		public bool IsCompleted => ares.IsCompleted;

		public SendFileHandler Delegate => d;

		public IAsyncResult Original => ares;

		public SendFileAsyncResult(SendFileHandler d, IAsyncResult ares)
		{
			this.d = d;
			this.ares = ares;
		}
	}

	private struct WSABUF
	{
		public int len;

		public IntPtr buf;
	}

	private static object s_InternalSyncObject;

	internal static volatile bool s_SupportsIPv4;

	internal static volatile bool s_SupportsIPv6;

	internal static volatile bool s_OSSupportsIPv6;

	internal static volatile bool s_Initialized;

	private static volatile bool s_LoggingEnabled;

	internal static volatile bool s_PerfCountersEnabled;

	internal const int DefaultCloseTimeout = -1;

	private const int SOCKET_CLOSED_CODE = 10004;

	private const string TIMEOUT_EXCEPTION_MSG = "A connection attempt failed because the connected party did not properly respondafter a period of time, or established connection failed because connected host has failed to respond";

	private bool is_closed;

	private bool is_listening;

	private bool useOverlappedIO;

	private int linger_timeout;

	private AddressFamily addressFamily;

	private SocketType socketType;

	private ProtocolType protocolType;

	internal SafeSocketHandle m_Handle;

	internal EndPoint seed_endpoint;

	internal SemaphoreSlim ReadSem = new SemaphoreSlim(1, 1);

	internal SemaphoreSlim WriteSem = new SemaphoreSlim(1, 1);

	internal bool is_blocking = true;

	internal bool is_bound;

	internal bool is_connected;

	private int m_IntCleanedUp;

	internal bool connect_in_progress;

	private static AsyncCallback AcceptAsyncCallback = delegate(IAsyncResult ares)
	{
		SocketAsyncEventArgs socketAsyncEventArgs7 = (SocketAsyncEventArgs)((SocketAsyncResult)ares).AsyncState;
		if (Interlocked.Exchange(ref socketAsyncEventArgs7.in_progress, 0) != 1)
		{
			throw new InvalidOperationException("No operation in progress");
		}
		try
		{
			socketAsyncEventArgs7.AcceptSocket = socketAsyncEventArgs7.current_socket.EndAccept(ares);
		}
		catch (SocketException ex13)
		{
			socketAsyncEventArgs7.SocketError = ex13.SocketErrorCode;
		}
		catch (ObjectDisposedException)
		{
			socketAsyncEventArgs7.SocketError = SocketError.OperationAborted;
		}
		finally
		{
			if (socketAsyncEventArgs7.AcceptSocket == null)
			{
				socketAsyncEventArgs7.AcceptSocket = new Socket(socketAsyncEventArgs7.current_socket.AddressFamily, socketAsyncEventArgs7.current_socket.SocketType, socketAsyncEventArgs7.current_socket.ProtocolType, null);
			}
			socketAsyncEventArgs7.Complete();
		}
	};

	private static IOAsyncCallback BeginAcceptCallback = delegate(IOAsyncResult ares)
	{
		SocketAsyncResult socketAsyncResult8 = (SocketAsyncResult)ares;
		Socket socket2 = null;
		try
		{
			if (socketAsyncResult8.AcceptSocket == null)
			{
				socket2 = socketAsyncResult8.socket.Accept();
			}
			else
			{
				socket2 = socketAsyncResult8.AcceptSocket;
				socketAsyncResult8.socket.Accept(socket2);
			}
		}
		catch (Exception e9)
		{
			socketAsyncResult8.Complete(e9);
			return;
		}
		socketAsyncResult8.Complete(socket2);
	};

	private static IOAsyncCallback BeginAcceptReceiveCallback = delegate(IOAsyncResult ares)
	{
		SocketAsyncResult socketAsyncResult7 = (SocketAsyncResult)ares;
		Socket socket = null;
		try
		{
			if (socketAsyncResult7.AcceptSocket == null)
			{
				socket = socketAsyncResult7.socket.Accept();
			}
			else
			{
				socket = socketAsyncResult7.AcceptSocket;
				socketAsyncResult7.socket.Accept(socket);
			}
		}
		catch (Exception e7)
		{
			socketAsyncResult7.Complete(e7);
			return;
		}
		int total2 = 0;
		if (socketAsyncResult7.Size > 0)
		{
			try
			{
				total2 = socket.Receive(socketAsyncResult7.Buffer, socketAsyncResult7.Offset, socketAsyncResult7.Size, socketAsyncResult7.SockFlags, out var errorCode2);
				if (errorCode2 != 0)
				{
					socketAsyncResult7.Complete(new SocketException((int)errorCode2));
					return;
				}
			}
			catch (Exception e8)
			{
				socketAsyncResult7.Complete(e8);
				return;
			}
		}
		socketAsyncResult7.Complete(socket, total2);
	};

	private static AsyncCallback ConnectAsyncCallback = delegate(IAsyncResult ares)
	{
		SocketAsyncEventArgs socketAsyncEventArgs6 = (SocketAsyncEventArgs)((SocketAsyncResult)ares).AsyncState;
		if (Interlocked.Exchange(ref socketAsyncEventArgs6.in_progress, 0) != 1)
		{
			throw new InvalidOperationException("No operation in progress");
		}
		try
		{
			socketAsyncEventArgs6.current_socket.EndConnect(ares);
		}
		catch (SocketException ex11)
		{
			socketAsyncEventArgs6.SocketError = ex11.SocketErrorCode;
		}
		catch (ObjectDisposedException)
		{
			socketAsyncEventArgs6.SocketError = SocketError.OperationAborted;
		}
		finally
		{
			socketAsyncEventArgs6.Complete();
		}
	};

	private static IOAsyncCallback BeginConnectCallback = delegate(IOAsyncResult ares)
	{
		SocketAsyncResult socketAsyncResult6 = (SocketAsyncResult)ares;
		if (socketAsyncResult6.EndPoint == null)
		{
			socketAsyncResult6.Complete(new SocketException(10049));
			return;
		}
		try
		{
			int num4 = (int)socketAsyncResult6.socket.GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Error);
			if (num4 == 0)
			{
				socketAsyncResult6.socket.seed_endpoint = socketAsyncResult6.EndPoint;
				socketAsyncResult6.socket.is_connected = true;
				socketAsyncResult6.socket.is_bound = true;
				socketAsyncResult6.socket.connect_in_progress = false;
				socketAsyncResult6.error = 0;
				socketAsyncResult6.Complete();
			}
			else if (socketAsyncResult6.Addresses == null)
			{
				socketAsyncResult6.socket.connect_in_progress = false;
				socketAsyncResult6.Complete(new SocketException(num4));
			}
			else if (socketAsyncResult6.CurrentAddress >= socketAsyncResult6.Addresses.Length)
			{
				socketAsyncResult6.Complete(new SocketException(num4));
			}
			else
			{
				BeginMConnect(socketAsyncResult6);
			}
		}
		catch (Exception e6)
		{
			socketAsyncResult6.socket.connect_in_progress = false;
			socketAsyncResult6.Complete(e6);
		}
	};

	private static AsyncCallback DisconnectAsyncCallback = delegate(IAsyncResult ares)
	{
		SocketAsyncEventArgs socketAsyncEventArgs5 = (SocketAsyncEventArgs)((SocketAsyncResult)ares).AsyncState;
		if (Interlocked.Exchange(ref socketAsyncEventArgs5.in_progress, 0) != 1)
		{
			throw new InvalidOperationException("No operation in progress");
		}
		try
		{
			socketAsyncEventArgs5.current_socket.EndDisconnect(ares);
		}
		catch (SocketException ex9)
		{
			socketAsyncEventArgs5.SocketError = ex9.SocketErrorCode;
		}
		catch (ObjectDisposedException)
		{
			socketAsyncEventArgs5.SocketError = SocketError.OperationAborted;
		}
		finally
		{
			socketAsyncEventArgs5.Complete();
		}
	};

	private static IOAsyncCallback BeginDisconnectCallback = delegate(IOAsyncResult ares)
	{
		SocketAsyncResult socketAsyncResult5 = (SocketAsyncResult)ares;
		try
		{
			socketAsyncResult5.socket.Disconnect(socketAsyncResult5.ReuseSocket);
		}
		catch (Exception e5)
		{
			socketAsyncResult5.Complete(e5);
			return;
		}
		socketAsyncResult5.Complete();
	};

	private static AsyncCallback ReceiveAsyncCallback = delegate(IAsyncResult ares)
	{
		SocketAsyncEventArgs socketAsyncEventArgs4 = (SocketAsyncEventArgs)((SocketAsyncResult)ares).AsyncState;
		if (Interlocked.Exchange(ref socketAsyncEventArgs4.in_progress, 0) != 1)
		{
			throw new InvalidOperationException("No operation in progress");
		}
		try
		{
			socketAsyncEventArgs4.BytesTransferred = socketAsyncEventArgs4.current_socket.EndReceive(ares);
		}
		catch (SocketException ex7)
		{
			socketAsyncEventArgs4.SocketError = ex7.SocketErrorCode;
		}
		catch (ObjectDisposedException)
		{
			socketAsyncEventArgs4.SocketError = SocketError.OperationAborted;
		}
		finally
		{
			socketAsyncEventArgs4.Complete();
		}
	};

	private unsafe static IOAsyncCallback BeginReceiveCallback = delegate(IOAsyncResult ares)
	{
		SocketAsyncResult socketAsyncResult4 = (SocketAsyncResult)ares;
		int total = 0;
		try
		{
			fixed (byte* ptr = socketAsyncResult4.Buffer)
			{
				total = Receive_internal(socketAsyncResult4.socket.m_Handle, ptr + socketAsyncResult4.Offset, socketAsyncResult4.Size, socketAsyncResult4.SockFlags, out socketAsyncResult4.error, socketAsyncResult4.socket.is_blocking);
			}
		}
		catch (Exception e4)
		{
			socketAsyncResult4.Complete(e4);
			return;
		}
		socketAsyncResult4.Complete(total);
	};

	private static IOAsyncCallback BeginReceiveGenericCallback = delegate(IOAsyncResult ares)
	{
		SocketAsyncResult socketAsyncResult3 = (SocketAsyncResult)ares;
		int num3 = 0;
		try
		{
			num3 = socketAsyncResult3.socket.Receive(socketAsyncResult3.Buffers, socketAsyncResult3.SockFlags);
		}
		catch (Exception e3)
		{
			socketAsyncResult3.Complete(e3);
			return;
		}
		socketAsyncResult3.Complete(num3);
	};

	private static AsyncCallback ReceiveFromAsyncCallback = delegate(IAsyncResult ares)
	{
		SocketAsyncEventArgs socketAsyncEventArgs3 = (SocketAsyncEventArgs)((SocketAsyncResult)ares).AsyncState;
		if (Interlocked.Exchange(ref socketAsyncEventArgs3.in_progress, 0) != 1)
		{
			throw new InvalidOperationException("No operation in progress");
		}
		try
		{
			socketAsyncEventArgs3.BytesTransferred = socketAsyncEventArgs3.current_socket.EndReceiveFrom(ares, ref socketAsyncEventArgs3.remote_ep);
		}
		catch (SocketException ex5)
		{
			socketAsyncEventArgs3.SocketError = ex5.SocketErrorCode;
		}
		catch (ObjectDisposedException)
		{
			socketAsyncEventArgs3.SocketError = SocketError.OperationAborted;
		}
		finally
		{
			socketAsyncEventArgs3.Complete();
		}
	};

	private static IOAsyncCallback BeginReceiveFromCallback = delegate(IOAsyncResult ares)
	{
		SocketAsyncResult socketAsyncResult2 = (SocketAsyncResult)ares;
		int num2 = 0;
		try
		{
			num2 = socketAsyncResult2.socket.ReceiveFrom(socketAsyncResult2.Buffer, socketAsyncResult2.Offset, socketAsyncResult2.Size, socketAsyncResult2.SockFlags, ref socketAsyncResult2.EndPoint, out var errorCode);
			if (errorCode != 0)
			{
				socketAsyncResult2.Complete(new SocketException(errorCode));
				return;
			}
		}
		catch (Exception e2)
		{
			socketAsyncResult2.Complete(e2);
			return;
		}
		socketAsyncResult2.Complete(num2);
	};

	private static AsyncCallback SendAsyncCallback = delegate(IAsyncResult ares)
	{
		SocketAsyncEventArgs socketAsyncEventArgs2 = (SocketAsyncEventArgs)((SocketAsyncResult)ares).AsyncState;
		if (Interlocked.Exchange(ref socketAsyncEventArgs2.in_progress, 0) != 1)
		{
			throw new InvalidOperationException("No operation in progress");
		}
		try
		{
			socketAsyncEventArgs2.BytesTransferred = socketAsyncEventArgs2.current_socket.EndSend(ares);
		}
		catch (SocketException ex3)
		{
			socketAsyncEventArgs2.SocketError = ex3.SocketErrorCode;
		}
		catch (ObjectDisposedException)
		{
			socketAsyncEventArgs2.SocketError = SocketError.OperationAborted;
		}
		finally
		{
			socketAsyncEventArgs2.Complete();
		}
	};

	private static IOAsyncCallback BeginSendGenericCallback = delegate(IOAsyncResult ares)
	{
		SocketAsyncResult socketAsyncResult = (SocketAsyncResult)ares;
		int num = 0;
		try
		{
			num = socketAsyncResult.socket.Send(socketAsyncResult.Buffers, socketAsyncResult.SockFlags);
		}
		catch (Exception e)
		{
			socketAsyncResult.Complete(e);
			return;
		}
		socketAsyncResult.Complete(num);
	};

	private static AsyncCallback SendToAsyncCallback = delegate(IAsyncResult ares)
	{
		SocketAsyncEventArgs socketAsyncEventArgs = (SocketAsyncEventArgs)((SocketAsyncResult)ares).AsyncState;
		if (Interlocked.Exchange(ref socketAsyncEventArgs.in_progress, 0) != 1)
		{
			throw new InvalidOperationException("No operation in progress");
		}
		try
		{
			socketAsyncEventArgs.BytesTransferred = socketAsyncEventArgs.current_socket.EndSendTo(ares);
		}
		catch (SocketException ex)
		{
			socketAsyncEventArgs.SocketError = ex.SocketErrorCode;
		}
		catch (ObjectDisposedException)
		{
			socketAsyncEventArgs.SocketError = SocketError.OperationAborted;
		}
		finally
		{
			socketAsyncEventArgs.Complete();
		}
	};

	[Obsolete("SupportsIPv4 is obsoleted for this type, please use OSSupportsIPv4 instead. http://go.microsoft.com/fwlink/?linkid=14202")]
	public static bool SupportsIPv4
	{
		get
		{
			InitializeSockets();
			return s_SupportsIPv4;
		}
	}

	public static bool OSSupportsIPv4
	{
		get
		{
			InitializeSockets();
			return s_SupportsIPv4;
		}
	}

	[Obsolete("SupportsIPv6 is obsoleted for this type, please use OSSupportsIPv6 instead. http://go.microsoft.com/fwlink/?linkid=14202")]
	public static bool SupportsIPv6
	{
		get
		{
			InitializeSockets();
			return s_SupportsIPv6;
		}
	}

	internal static bool LegacySupportsIPv6
	{
		get
		{
			InitializeSockets();
			return s_SupportsIPv6;
		}
	}

	public static bool OSSupportsIPv6
	{
		get
		{
			InitializeSockets();
			return s_OSSupportsIPv6;
		}
	}

	public IntPtr Handle => m_Handle.DangerousGetHandle();

	public bool UseOnlyOverlappedIO
	{
		get
		{
			return useOverlappedIO;
		}
		set
		{
			useOverlappedIO = value;
		}
	}

	public AddressFamily AddressFamily => addressFamily;

	public SocketType SocketType => socketType;

	public ProtocolType ProtocolType => protocolType;

	public bool ExclusiveAddressUse
	{
		get
		{
			if ((int)GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ExclusiveAddressUse) == 0)
			{
				return false;
			}
			return true;
		}
		set
		{
			if (IsBound)
			{
				throw new InvalidOperationException(global::SR.GetString("The socket must not be bound or connected."));
			}
			SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ExclusiveAddressUse, value ? 1 : 0);
		}
	}

	public int ReceiveBufferSize
	{
		get
		{
			return (int)GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReceiveBuffer);
		}
		set
		{
			if (value < 0)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReceiveBuffer, value);
		}
	}

	public int SendBufferSize
	{
		get
		{
			return (int)GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.SendBuffer);
		}
		set
		{
			if (value < 0)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.SendBuffer, value);
		}
	}

	public int ReceiveTimeout
	{
		get
		{
			return (int)GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReceiveTimeout);
		}
		set
		{
			if (value < -1)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			if (value == -1)
			{
				value = 0;
			}
			SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReceiveTimeout, value);
		}
	}

	public int SendTimeout
	{
		get
		{
			return (int)GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.SendTimeout);
		}
		set
		{
			if (value < -1)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			if (value == -1)
			{
				value = 0;
			}
			SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.SendTimeout, value);
		}
	}

	public LingerOption LingerState
	{
		get
		{
			return (LingerOption)GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Linger);
		}
		set
		{
			SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Linger, value);
		}
	}

	public short Ttl
	{
		get
		{
			if (addressFamily == AddressFamily.InterNetwork)
			{
				return (short)(int)GetSocketOption(SocketOptionLevel.IP, SocketOptionName.ReuseAddress);
			}
			if (addressFamily == AddressFamily.InterNetworkV6)
			{
				return (short)(int)GetSocketOption(SocketOptionLevel.IPv6, SocketOptionName.ReuseAddress);
			}
			throw new NotSupportedException(global::SR.GetString("This protocol version is not supported."));
		}
		set
		{
			if (value < 0 || value > 255)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			if (addressFamily == AddressFamily.InterNetwork)
			{
				SetSocketOption(SocketOptionLevel.IP, SocketOptionName.ReuseAddress, value);
				return;
			}
			if (addressFamily == AddressFamily.InterNetworkV6)
			{
				SetSocketOption(SocketOptionLevel.IPv6, SocketOptionName.ReuseAddress, value);
				return;
			}
			throw new NotSupportedException(global::SR.GetString("This protocol version is not supported."));
		}
	}

	public bool DontFragment
	{
		get
		{
			if (addressFamily == AddressFamily.InterNetwork)
			{
				if ((int)GetSocketOption(SocketOptionLevel.IP, SocketOptionName.DontFragment) == 0)
				{
					return false;
				}
				return true;
			}
			throw new NotSupportedException(global::SR.GetString("This protocol version is not supported."));
		}
		set
		{
			if (addressFamily == AddressFamily.InterNetwork)
			{
				SetSocketOption(SocketOptionLevel.IP, SocketOptionName.DontFragment, value ? 1 : 0);
				return;
			}
			throw new NotSupportedException(global::SR.GetString("This protocol version is not supported."));
		}
	}

	public bool DualMode
	{
		get
		{
			if (AddressFamily != AddressFamily.InterNetworkV6)
			{
				throw new NotSupportedException(global::SR.GetString("This protocol version is not supported."));
			}
			return (int)GetSocketOption(SocketOptionLevel.IPv6, SocketOptionName.IPv6Only) == 0;
		}
		set
		{
			if (AddressFamily != AddressFamily.InterNetworkV6)
			{
				throw new NotSupportedException(global::SR.GetString("This protocol version is not supported."));
			}
			SetSocketOption(SocketOptionLevel.IPv6, SocketOptionName.IPv6Only, (!value) ? 1 : 0);
		}
	}

	private bool IsDualMode
	{
		get
		{
			if (AddressFamily == AddressFamily.InterNetworkV6)
			{
				return DualMode;
			}
			return false;
		}
	}

	private static object InternalSyncObject
	{
		get
		{
			if (s_InternalSyncObject == null)
			{
				object value = new object();
				Interlocked.CompareExchange(ref s_InternalSyncObject, value, null);
			}
			return s_InternalSyncObject;
		}
	}

	internal bool CleanedUp => m_IntCleanedUp == 1;

	public int Available
	{
		get
		{
			ThrowIfDisposedAndClosed();
			int error;
			int result = Available_internal(m_Handle, out error);
			if (error != 0)
			{
				throw new SocketException(error);
			}
			return result;
		}
	}

	public bool EnableBroadcast
	{
		get
		{
			ThrowIfDisposedAndClosed();
			if (protocolType != ProtocolType.Udp)
			{
				throw new SocketException(10042);
			}
			return (int)GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Broadcast) != 0;
		}
		set
		{
			ThrowIfDisposedAndClosed();
			if (protocolType != ProtocolType.Udp)
			{
				throw new SocketException(10042);
			}
			SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Broadcast, value ? 1 : 0);
		}
	}

	public bool IsBound => is_bound;

	public bool MulticastLoopback
	{
		get
		{
			ThrowIfDisposedAndClosed();
			if (protocolType == ProtocolType.Tcp)
			{
				throw new SocketException(10042);
			}
			return addressFamily switch
			{
				AddressFamily.InterNetwork => (int)GetSocketOption(SocketOptionLevel.IP, SocketOptionName.MulticastLoopback) != 0, 
				AddressFamily.InterNetworkV6 => (int)GetSocketOption(SocketOptionLevel.IPv6, SocketOptionName.MulticastLoopback) != 0, 
				_ => throw new NotSupportedException("This property is only valid for InterNetwork and InterNetworkV6 sockets"), 
			};
		}
		set
		{
			ThrowIfDisposedAndClosed();
			if (protocolType == ProtocolType.Tcp)
			{
				throw new SocketException(10042);
			}
			switch (addressFamily)
			{
			case AddressFamily.InterNetwork:
				SetSocketOption(SocketOptionLevel.IP, SocketOptionName.MulticastLoopback, value ? 1 : 0);
				break;
			case AddressFamily.InterNetworkV6:
				SetSocketOption(SocketOptionLevel.IPv6, SocketOptionName.MulticastLoopback, value ? 1 : 0);
				break;
			default:
				throw new NotSupportedException("This property is only valid for InterNetwork and InterNetworkV6 sockets");
			}
		}
	}

	public EndPoint LocalEndPoint
	{
		get
		{
			ThrowIfDisposedAndClosed();
			if (seed_endpoint == null)
			{
				return null;
			}
			int error;
			SocketAddress socketAddress = LocalEndPoint_internal(m_Handle, (int)addressFamily, out error);
			if (error != 0)
			{
				throw new SocketException(error);
			}
			return seed_endpoint.Create(socketAddress);
		}
	}

	public bool Blocking
	{
		get
		{
			return is_blocking;
		}
		set
		{
			ThrowIfDisposedAndClosed();
			Blocking_internal(m_Handle, value, out var error);
			if (error != 0)
			{
				throw new SocketException(error);
			}
			is_blocking = value;
		}
	}

	public bool Connected
	{
		get
		{
			return is_connected;
		}
		internal set
		{
			is_connected = value;
		}
	}

	public bool NoDelay
	{
		get
		{
			ThrowIfDisposedAndClosed();
			ThrowIfUdp();
			return (int)GetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.Debug) != 0;
		}
		set
		{
			ThrowIfDisposedAndClosed();
			ThrowIfUdp();
			SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.Debug, value ? 1 : 0);
		}
	}

	public EndPoint RemoteEndPoint
	{
		get
		{
			ThrowIfDisposedAndClosed();
			if (!is_connected || seed_endpoint == null)
			{
				return null;
			}
			int error;
			SocketAddress socketAddress = RemoteEndPoint_internal(m_Handle, (int)addressFamily, out error);
			if (error != 0)
			{
				throw new SocketException(error);
			}
			return seed_endpoint.Create(socketAddress);
		}
	}

	internal static int FamilyHint
	{
		get
		{
			int num = 0;
			if (OSSupportsIPv4)
			{
				num = 1;
			}
			if (OSSupportsIPv6)
			{
				num = ((num == 0) ? 2 : 0);
			}
			return num;
		}
	}

	public Socket(SocketType socketType, ProtocolType protocolType)
		: this(AddressFamily.InterNetworkV6, socketType, protocolType)
	{
		DualMode = true;
	}

	public Socket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType)
	{
		s_LoggingEnabled = Logging.On;
		_ = s_LoggingEnabled;
		InitializeSockets();
		m_Handle = new SafeSocketHandle(Socket_internal(addressFamily, socketType, protocolType, out var _), ownsHandle: true);
		if (m_Handle.IsInvalid)
		{
			throw new SocketException();
		}
		this.addressFamily = addressFamily;
		this.socketType = socketType;
		this.protocolType = protocolType;
		IPProtectionLevel iPProtectionLevel = SettingsSectionInternal.Section.IPProtectionLevel;
		if (iPProtectionLevel != IPProtectionLevel.Unspecified)
		{
			SetIPProtectionLevel(iPProtectionLevel);
		}
		SocketDefaults();
		_ = s_LoggingEnabled;
	}

	internal bool CanTryAddressFamily(AddressFamily family)
	{
		if (family != addressFamily)
		{
			if (family == AddressFamily.InterNetwork)
			{
				return IsDualMode;
			}
			return false;
		}
		return true;
	}

	public void Connect(IPAddress[] addresses, int port)
	{
		_ = s_LoggingEnabled;
		if (CleanedUp)
		{
			throw new ObjectDisposedException(GetType().FullName);
		}
		if (addresses == null)
		{
			throw new ArgumentNullException("addresses");
		}
		if (addresses.Length == 0)
		{
			throw new ArgumentException(global::SR.GetString("The number of specified IP addresses has to be greater than 0."), "addresses");
		}
		if (!ValidationHelper.ValidateTcpPort(port))
		{
			throw new ArgumentOutOfRangeException("port");
		}
		if (addressFamily != AddressFamily.InterNetwork && addressFamily != AddressFamily.InterNetworkV6)
		{
			throw new NotSupportedException(global::SR.GetString("This protocol version is not supported."));
		}
		Exception ex = null;
		foreach (IPAddress iPAddress in addresses)
		{
			if (!CanTryAddressFamily(iPAddress.AddressFamily))
			{
				continue;
			}
			try
			{
				Connect(new IPEndPoint(iPAddress, port));
				ex = null;
			}
			catch (Exception ex2)
			{
				if (NclUtilities.IsFatal(ex2))
				{
					throw;
				}
				ex = ex2;
				continue;
			}
			break;
		}
		if (ex != null)
		{
			throw ex;
		}
		if (!Connected)
		{
			throw new ArgumentException(global::SR.GetString("None of the discovered or specified addresses match the socket address family."), "addresses");
		}
		_ = s_LoggingEnabled;
	}

	public int Send(byte[] buffer, int size, SocketFlags socketFlags)
	{
		return Send(buffer, 0, size, socketFlags);
	}

	public int Send(byte[] buffer, SocketFlags socketFlags)
	{
		return Send(buffer, 0, (buffer != null) ? buffer.Length : 0, socketFlags);
	}

	public int Send(byte[] buffer)
	{
		return Send(buffer, 0, (buffer != null) ? buffer.Length : 0, SocketFlags.None);
	}

	public int Send(IList<ArraySegment<byte>> buffers)
	{
		return Send(buffers, SocketFlags.None);
	}

	public int Send(IList<ArraySegment<byte>> buffers, SocketFlags socketFlags)
	{
		SocketError errorCode;
		int result = Send(buffers, socketFlags, out errorCode);
		if (errorCode != 0)
		{
			throw new SocketException(errorCode);
		}
		return result;
	}

	public void SendFile(string fileName)
	{
		SendFile(fileName, null, null, TransmitFileOptions.UseDefaultWorkerThread);
	}

	public int Send(byte[] buffer, int offset, int size, SocketFlags socketFlags)
	{
		SocketError errorCode;
		int result = Send(buffer, offset, size, socketFlags, out errorCode);
		if (errorCode != 0)
		{
			throw new SocketException(errorCode);
		}
		return result;
	}

	public int SendTo(byte[] buffer, int size, SocketFlags socketFlags, EndPoint remoteEP)
	{
		return SendTo(buffer, 0, size, socketFlags, remoteEP);
	}

	public int SendTo(byte[] buffer, SocketFlags socketFlags, EndPoint remoteEP)
	{
		return SendTo(buffer, 0, (buffer != null) ? buffer.Length : 0, socketFlags, remoteEP);
	}

	public int SendTo(byte[] buffer, EndPoint remoteEP)
	{
		return SendTo(buffer, 0, (buffer != null) ? buffer.Length : 0, SocketFlags.None, remoteEP);
	}

	public int Receive(byte[] buffer, int size, SocketFlags socketFlags)
	{
		return Receive(buffer, 0, size, socketFlags);
	}

	public int Receive(byte[] buffer, SocketFlags socketFlags)
	{
		return Receive(buffer, 0, (buffer != null) ? buffer.Length : 0, socketFlags);
	}

	public int Receive(byte[] buffer)
	{
		return Receive(buffer, 0, (buffer != null) ? buffer.Length : 0, SocketFlags.None);
	}

	public int Receive(byte[] buffer, int offset, int size, SocketFlags socketFlags)
	{
		SocketError errorCode;
		int result = Receive(buffer, offset, size, socketFlags, out errorCode);
		if (errorCode != 0)
		{
			throw new SocketException(errorCode);
		}
		return result;
	}

	public int Receive(IList<ArraySegment<byte>> buffers)
	{
		return Receive(buffers, SocketFlags.None);
	}

	public int Receive(IList<ArraySegment<byte>> buffers, SocketFlags socketFlags)
	{
		SocketError errorCode;
		int result = Receive(buffers, socketFlags, out errorCode);
		if (errorCode != 0)
		{
			throw new SocketException(errorCode);
		}
		return result;
	}

	public int ReceiveFrom(byte[] buffer, int size, SocketFlags socketFlags, ref EndPoint remoteEP)
	{
		return ReceiveFrom(buffer, 0, size, socketFlags, ref remoteEP);
	}

	public int ReceiveFrom(byte[] buffer, SocketFlags socketFlags, ref EndPoint remoteEP)
	{
		return ReceiveFrom(buffer, 0, (buffer != null) ? buffer.Length : 0, socketFlags, ref remoteEP);
	}

	public int ReceiveFrom(byte[] buffer, ref EndPoint remoteEP)
	{
		return ReceiveFrom(buffer, 0, (buffer != null) ? buffer.Length : 0, SocketFlags.None, ref remoteEP);
	}

	public int IOControl(IOControlCode ioControlCode, byte[] optionInValue, byte[] optionOutValue)
	{
		return IOControl((int)ioControlCode, optionInValue, optionOutValue);
	}

	public void SetIPProtectionLevel(IPProtectionLevel level)
	{
		if (level == IPProtectionLevel.Unspecified)
		{
			throw new ArgumentException(global::SR.GetString("The specified value is not valid."), "level");
		}
		if (addressFamily == AddressFamily.InterNetworkV6)
		{
			SetSocketOption(SocketOptionLevel.IPv6, SocketOptionName.IPProtectionLevel, (int)level);
			return;
		}
		if (addressFamily == AddressFamily.InterNetwork)
		{
			SetSocketOption(SocketOptionLevel.IP, SocketOptionName.IPProtectionLevel, (int)level);
			return;
		}
		throw new NotSupportedException(global::SR.GetString("This protocol version is not supported."));
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public IAsyncResult BeginSendFile(string fileName, AsyncCallback callback, object state)
	{
		return BeginSendFile(fileName, null, null, TransmitFileOptions.UseDefaultWorkerThread, callback, state);
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public IAsyncResult BeginConnect(IPAddress address, int port, AsyncCallback requestCallback, object state)
	{
		_ = s_LoggingEnabled;
		if (CleanedUp)
		{
			throw new ObjectDisposedException(GetType().FullName);
		}
		if (address == null)
		{
			throw new ArgumentNullException("address");
		}
		if (!ValidationHelper.ValidateTcpPort(port))
		{
			throw new ArgumentOutOfRangeException("port");
		}
		if (!CanTryAddressFamily(address.AddressFamily))
		{
			throw new NotSupportedException(global::SR.GetString("This protocol version is not supported."));
		}
		IAsyncResult result = BeginConnect(new IPEndPoint(address, port), requestCallback, state);
		_ = s_LoggingEnabled;
		return result;
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public IAsyncResult BeginSend(byte[] buffer, int offset, int size, SocketFlags socketFlags, AsyncCallback callback, object state)
	{
		SocketError errorCode;
		IAsyncResult result = BeginSend(buffer, offset, size, socketFlags, out errorCode, callback, state);
		if (errorCode != 0 && errorCode != SocketError.IOPending)
		{
			throw new SocketException(errorCode);
		}
		return result;
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public IAsyncResult BeginSend(IList<ArraySegment<byte>> buffers, SocketFlags socketFlags, AsyncCallback callback, object state)
	{
		SocketError errorCode;
		IAsyncResult result = BeginSend(buffers, socketFlags, out errorCode, callback, state);
		if (errorCode != 0 && errorCode != SocketError.IOPending)
		{
			throw new SocketException(errorCode);
		}
		return result;
	}

	public int EndSend(IAsyncResult asyncResult)
	{
		SocketError errorCode;
		int result = EndSend(asyncResult, out errorCode);
		if (errorCode != 0)
		{
			throw new SocketException(errorCode);
		}
		return result;
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public IAsyncResult BeginReceive(byte[] buffer, int offset, int size, SocketFlags socketFlags, AsyncCallback callback, object state)
	{
		SocketError errorCode;
		IAsyncResult result = BeginReceive(buffer, offset, size, socketFlags, out errorCode, callback, state);
		if (errorCode != 0 && errorCode != SocketError.IOPending)
		{
			throw new SocketException(errorCode);
		}
		return result;
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public IAsyncResult BeginReceive(IList<ArraySegment<byte>> buffers, SocketFlags socketFlags, AsyncCallback callback, object state)
	{
		SocketError errorCode;
		IAsyncResult result = BeginReceive(buffers, socketFlags, out errorCode, callback, state);
		if (errorCode != 0 && errorCode != SocketError.IOPending)
		{
			throw new SocketException(errorCode);
		}
		return result;
	}

	public int EndReceive(IAsyncResult asyncResult)
	{
		SocketError errorCode;
		int result = EndReceive(asyncResult, out errorCode);
		if (errorCode != 0)
		{
			throw new SocketException(errorCode);
		}
		return result;
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public IAsyncResult BeginAccept(int receiveSize, AsyncCallback callback, object state)
	{
		return BeginAccept(null, receiveSize, callback, state);
	}

	public Socket EndAccept(out byte[] buffer, IAsyncResult asyncResult)
	{
		byte[] buffer2;
		int bytesTransferred;
		Socket result = EndAccept(out buffer2, out bytesTransferred, asyncResult);
		buffer = new byte[bytesTransferred];
		Array.Copy(buffer2, buffer, bytesTransferred);
		return result;
	}

	internal static void InitializeSockets()
	{
		if (s_Initialized)
		{
			return;
		}
		lock (InternalSyncObject)
		{
			if (!s_Initialized)
			{
				bool flag = true;
				bool num = IsProtocolSupported(NetworkInterfaceComponent.IPv4);
				flag = IsProtocolSupported(NetworkInterfaceComponent.IPv6);
				if (flag)
				{
					s_OSSupportsIPv6 = true;
					flag = SettingsSectionInternal.Section.Ipv6Enabled;
				}
				s_SupportsIPv4 = num;
				s_SupportsIPv6 = flag;
				s_Initialized = true;
			}
		}
	}

	public void Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	~Socket()
	{
		Dispose(disposing: false);
	}

	public static bool ConnectAsync(SocketType socketType, ProtocolType protocolType, SocketAsyncEventArgs e)
	{
		_ = s_LoggingEnabled;
		if (e.m_BufferList != null)
		{
			throw new ArgumentException(global::SR.GetString("Multiple buffers cannot be used with this method."), "BufferList");
		}
		if (e.RemoteEndPoint == null)
		{
			throw new ArgumentNullException("remoteEP");
		}
		EndPoint remoteEndPoint = e.RemoteEndPoint;
		bool result;
		if (remoteEndPoint is DnsEndPoint dnsEndPoint)
		{
			Socket socket = null;
			MultipleConnectAsync multipleConnectAsync = null;
			if (dnsEndPoint.AddressFamily == AddressFamily.Unspecified)
			{
				multipleConnectAsync = new MultipleSocketMultipleConnectAsync(socketType, protocolType);
			}
			else
			{
				socket = new Socket(dnsEndPoint.AddressFamily, socketType, protocolType);
				multipleConnectAsync = new SingleSocketMultipleConnectAsync(socket, userSocket: false);
			}
			e.StartOperationCommon(socket);
			e.StartOperationWrapperConnect(multipleConnectAsync);
			result = multipleConnectAsync.StartConnectAsync(e, dnsEndPoint);
		}
		else
		{
			result = new Socket(remoteEndPoint.AddressFamily, socketType, protocolType).ConnectAsync(e);
		}
		_ = s_LoggingEnabled;
		return result;
	}

	internal void InternalShutdown(SocketShutdown how)
	{
		if (is_connected && !CleanedUp)
		{
			Shutdown_internal(m_Handle, how, out var _);
		}
	}

	internal IAsyncResult UnsafeBeginConnect(EndPoint remoteEP, AsyncCallback callback, object state)
	{
		return BeginConnect(remoteEP, callback, state);
	}

	internal IAsyncResult UnsafeBeginSend(byte[] buffer, int offset, int size, SocketFlags socketFlags, AsyncCallback callback, object state)
	{
		return BeginSend(buffer, offset, size, socketFlags, callback, state);
	}

	internal IAsyncResult UnsafeBeginReceive(byte[] buffer, int offset, int size, SocketFlags socketFlags, AsyncCallback callback, object state)
	{
		return BeginReceive(buffer, offset, size, socketFlags, callback, state);
	}

	internal IAsyncResult BeginMultipleSend(BufferOffsetSize[] buffers, SocketFlags socketFlags, AsyncCallback callback, object state)
	{
		ArraySegment<byte>[] array = new ArraySegment<byte>[buffers.Length];
		for (int i = 0; i < buffers.Length; i++)
		{
			array[i] = new ArraySegment<byte>(buffers[i].Buffer, buffers[i].Offset, buffers[i].Size);
		}
		return BeginSend(array, socketFlags, callback, state);
	}

	internal IAsyncResult UnsafeBeginMultipleSend(BufferOffsetSize[] buffers, SocketFlags socketFlags, AsyncCallback callback, object state)
	{
		return BeginMultipleSend(buffers, socketFlags, callback, state);
	}

	internal int EndMultipleSend(IAsyncResult asyncResult)
	{
		return EndSend(asyncResult);
	}

	internal void MultipleSend(BufferOffsetSize[] buffers, SocketFlags socketFlags)
	{
		ArraySegment<byte>[] array = new ArraySegment<byte>[buffers.Length];
		for (int i = 0; i < buffers.Length; i++)
		{
			array[i] = new ArraySegment<byte>(buffers[i].Buffer, buffers[i].Offset, buffers[i].Size);
		}
		Send(array, socketFlags);
	}

	internal void SetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, int optionValue, bool silent)
	{
		if (CleanedUp && is_closed)
		{
			if (!silent)
			{
				throw new ObjectDisposedException(GetType().ToString());
			}
			return;
		}
		SetSocketOption_internal(m_Handle, optionLevel, optionName, null, null, optionValue, out var error);
		if (!silent && error != 0)
		{
			throw new SocketException(error);
		}
	}

	public Socket(SocketInformation socketInformation)
	{
		is_listening = (socketInformation.Options & SocketInformationOptions.Listening) != 0;
		is_connected = (socketInformation.Options & SocketInformationOptions.Connected) != 0;
		is_blocking = (socketInformation.Options & SocketInformationOptions.NonBlocking) == 0;
		useOverlappedIO = (socketInformation.Options & SocketInformationOptions.UseOnlyOverlappedIO) != 0;
		IList list = DataConverter.Unpack("iiiil", socketInformation.ProtocolInformation, 0);
		addressFamily = (AddressFamily)(int)list[0];
		socketType = (SocketType)(int)list[1];
		protocolType = (ProtocolType)(int)list[2];
		is_bound = (int)list[3] != 0;
		m_Handle = new SafeSocketHandle((IntPtr)(long)list[4], ownsHandle: true);
		InitializeSockets();
		SocketDefaults();
	}

	internal Socket(AddressFamily family, SocketType type, ProtocolType proto, SafeSocketHandle safe_handle)
	{
		addressFamily = family;
		socketType = type;
		protocolType = proto;
		m_Handle = safe_handle;
		is_connected = true;
		InitializeSockets();
	}

	private void SocketDefaults()
	{
		try
		{
			if (addressFamily == AddressFamily.InterNetwork)
			{
				DontFragment = false;
				if (protocolType == ProtocolType.Tcp)
				{
					NoDelay = false;
				}
			}
			else if (addressFamily == AddressFamily.InterNetworkV6)
			{
				DualMode = true;
			}
		}
		catch (SocketException)
		{
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern IntPtr Socket_internal(AddressFamily family, SocketType type, ProtocolType proto, out int error);

	private static int Available_internal(SafeSocketHandle safeHandle, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			return Available_internal(safeHandle.DangerousGetHandle(), out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern int Available_internal(IntPtr socket, out int error);

	private static SocketAddress LocalEndPoint_internal(SafeSocketHandle safeHandle, int family, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			return LocalEndPoint_internal(safeHandle.DangerousGetHandle(), family, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern SocketAddress LocalEndPoint_internal(IntPtr socket, int family, out int error);

	private static void Blocking_internal(SafeSocketHandle safeHandle, bool block, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			Blocking_internal(safeHandle.DangerousGetHandle(), block, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern void Blocking_internal(IntPtr socket, bool block, out int error);

	private static SocketAddress RemoteEndPoint_internal(SafeSocketHandle safeHandle, int family, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			return RemoteEndPoint_internal(safeHandle.DangerousGetHandle(), family, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern SocketAddress RemoteEndPoint_internal(IntPtr socket, int family, out int error);

	public static void Select(IList checkRead, IList checkWrite, IList checkError, int microSeconds)
	{
		List<Socket> list = new List<Socket>();
		AddSockets(list, checkRead, "checkRead");
		AddSockets(list, checkWrite, "checkWrite");
		AddSockets(list, checkError, "checkError");
		if (list.Count == 3)
		{
			throw new ArgumentNullException("checkRead, checkWrite, checkError", "All the lists are null or empty.");
		}
		Socket[] sockets = list.ToArray();
		Select_internal(ref sockets, microSeconds, out var error);
		if (error != 0)
		{
			throw new SocketException(error);
		}
		if (sockets == null)
		{
			checkRead?.Clear();
			checkWrite?.Clear();
			checkError?.Clear();
			return;
		}
		int num = 0;
		int num2 = sockets.Length;
		IList list2 = checkRead;
		int num3 = 0;
		for (int i = 0; i < num2; i++)
		{
			Socket socket = sockets[i];
			if (socket == null)
			{
				if (list2 != null)
				{
					int num4 = list2.Count - num3;
					for (int j = 0; j < num4; j++)
					{
						list2.RemoveAt(num3);
					}
				}
				list2 = ((num == 0) ? checkWrite : checkError);
				num3 = 0;
				num++;
			}
			else
			{
				if (num == 1 && list2 == checkWrite && !socket.is_connected && (int)socket.GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Error) == 0)
				{
					socket.is_connected = true;
				}
				while ((Socket)list2[num3] != socket)
				{
					list2.RemoveAt(num3);
				}
				num3++;
			}
		}
	}

	private static void AddSockets(List<Socket> sockets, IList list, string name)
	{
		if (list != null)
		{
			foreach (Socket item in list)
			{
				if (item == null)
				{
					throw new ArgumentNullException(name, "Contains a null element");
				}
				sockets.Add(item);
			}
		}
		sockets.Add(null);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void Select_internal(ref Socket[] sockets, int microSeconds, out int error);

	public bool Poll(int microSeconds, SelectMode mode)
	{
		ThrowIfDisposedAndClosed();
		if (mode != 0 && mode != SelectMode.SelectWrite && mode != SelectMode.SelectError)
		{
			throw new NotSupportedException("'mode' parameter is not valid.");
		}
		int error;
		bool flag = Poll_internal(m_Handle, mode, microSeconds, out error);
		if (error != 0)
		{
			throw new SocketException(error);
		}
		if (mode == SelectMode.SelectWrite && flag && !is_connected && (int)GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Error) == 0)
		{
			is_connected = true;
		}
		return flag;
	}

	private static bool Poll_internal(SafeSocketHandle safeHandle, SelectMode mode, int timeout, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			return Poll_internal(safeHandle.DangerousGetHandle(), mode, timeout, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool Poll_internal(IntPtr socket, SelectMode mode, int timeout, out int error);

	public Socket Accept()
	{
		ThrowIfDisposedAndClosed();
		int error = 0;
		SafeSocketHandle safe_handle = Accept_internal(m_Handle, out error, is_blocking);
		if (error != 0)
		{
			if (is_closed)
			{
				error = 10004;
			}
			throw new SocketException(error);
		}
		return new Socket(AddressFamily, SocketType, ProtocolType, safe_handle)
		{
			seed_endpoint = seed_endpoint,
			Blocking = Blocking
		};
	}

	internal void Accept(Socket acceptSocket)
	{
		ThrowIfDisposedAndClosed();
		int error = 0;
		SafeSocketHandle handle = Accept_internal(m_Handle, out error, is_blocking);
		if (error != 0)
		{
			if (is_closed)
			{
				error = 10004;
			}
			throw new SocketException(error);
		}
		acceptSocket.addressFamily = AddressFamily;
		acceptSocket.socketType = SocketType;
		acceptSocket.protocolType = ProtocolType;
		acceptSocket.m_Handle = handle;
		acceptSocket.is_connected = true;
		acceptSocket.seed_endpoint = seed_endpoint;
		acceptSocket.Blocking = Blocking;
	}

	public bool AcceptAsync(SocketAsyncEventArgs e)
	{
		ThrowIfDisposedAndClosed();
		if (!is_bound)
		{
			throw new InvalidOperationException("You must call the Bind method before performing this operation.");
		}
		if (!is_listening)
		{
			throw new InvalidOperationException("You must call the Listen method before performing this operation.");
		}
		if (e.BufferList != null)
		{
			throw new ArgumentException("Multiple buffers cannot be used with this method.");
		}
		if (e.Count < 0)
		{
			throw new ArgumentOutOfRangeException("e.Count");
		}
		Socket acceptSocket = e.AcceptSocket;
		if (acceptSocket != null && (acceptSocket.is_bound || acceptSocket.is_connected))
		{
			throw new InvalidOperationException("AcceptSocket: The socket must not be bound or connected.");
		}
		InitSocketAsyncEventArgs(e, AcceptAsyncCallback, e, SocketOperation.Accept);
		QueueIOSelectorJob(ReadSem, e.socket_async_result.Handle, new IOSelectorJob(IOOperation.Read, BeginAcceptCallback, e.socket_async_result));
		return true;
	}

	public IAsyncResult BeginAccept(AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		if (!is_bound || !is_listening)
		{
			throw new InvalidOperationException();
		}
		SocketAsyncResult socketAsyncResult = new SocketAsyncResult(this, callback, state, SocketOperation.Accept);
		QueueIOSelectorJob(ReadSem, socketAsyncResult.Handle, new IOSelectorJob(IOOperation.Read, BeginAcceptCallback, socketAsyncResult));
		return socketAsyncResult;
	}

	public IAsyncResult BeginAccept(Socket acceptSocket, int receiveSize, AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		if (receiveSize < 0)
		{
			throw new ArgumentOutOfRangeException("receiveSize", "receiveSize is less than zero");
		}
		if (acceptSocket != null)
		{
			ThrowIfDisposedAndClosed(acceptSocket);
			if (acceptSocket.IsBound)
			{
				throw new InvalidOperationException();
			}
			if (acceptSocket.ProtocolType != ProtocolType.Tcp)
			{
				throw new SocketException(10022);
			}
		}
		SocketAsyncResult socketAsyncResult = new SocketAsyncResult(this, callback, state, SocketOperation.AcceptReceive)
		{
			Buffer = new byte[receiveSize],
			Offset = 0,
			Size = receiveSize,
			SockFlags = SocketFlags.None,
			AcceptSocket = acceptSocket
		};
		QueueIOSelectorJob(ReadSem, socketAsyncResult.Handle, new IOSelectorJob(IOOperation.Read, BeginAcceptReceiveCallback, socketAsyncResult));
		return socketAsyncResult;
	}

	public Socket EndAccept(IAsyncResult asyncResult)
	{
		byte[] buffer;
		int bytesTransferred;
		return EndAccept(out buffer, out bytesTransferred, asyncResult);
	}

	public Socket EndAccept(out byte[] buffer, out int bytesTransferred, IAsyncResult asyncResult)
	{
		ThrowIfDisposedAndClosed();
		SocketAsyncResult socketAsyncResult = ValidateEndIAsyncResult(asyncResult, "EndAccept", "asyncResult");
		if (!socketAsyncResult.IsCompleted)
		{
			socketAsyncResult.AsyncWaitHandle.WaitOne();
		}
		socketAsyncResult.CheckIfThrowDelayedException();
		buffer = socketAsyncResult.Buffer;
		bytesTransferred = socketAsyncResult.Total;
		return socketAsyncResult.AcceptedSocket;
	}

	private static SafeSocketHandle Accept_internal(SafeSocketHandle safeHandle, out int error, bool blocking)
	{
		try
		{
			safeHandle.RegisterForBlockingSyscall();
			return new SafeSocketHandle(Accept_internal(safeHandle.DangerousGetHandle(), out error, blocking), ownsHandle: true);
		}
		finally
		{
			safeHandle.UnRegisterForBlockingSyscall();
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern IntPtr Accept_internal(IntPtr sock, out int error, bool blocking);

	public void Bind(EndPoint localEP)
	{
		ThrowIfDisposedAndClosed();
		if (localEP == null)
		{
			throw new ArgumentNullException("localEP");
		}
		if (localEP is IPEndPoint input)
		{
			localEP = RemapIPEndPoint(input);
		}
		Bind_internal(m_Handle, localEP.Serialize(), out var error);
		if (error != 0)
		{
			throw new SocketException(error);
		}
		if (error == 0)
		{
			is_bound = true;
		}
		seed_endpoint = localEP;
	}

	private static void Bind_internal(SafeSocketHandle safeHandle, SocketAddress sa, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			Bind_internal(safeHandle.DangerousGetHandle(), sa, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void Bind_internal(IntPtr sock, SocketAddress sa, out int error);

	public void Listen(int backlog)
	{
		ThrowIfDisposedAndClosed();
		if (!is_bound)
		{
			throw new SocketException(10022);
		}
		Listen_internal(m_Handle, backlog, out var error);
		if (error != 0)
		{
			throw new SocketException(error);
		}
		is_listening = true;
	}

	private static void Listen_internal(SafeSocketHandle safeHandle, int backlog, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			Listen_internal(safeHandle.DangerousGetHandle(), backlog, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void Listen_internal(IntPtr sock, int backlog, out int error);

	public void Connect(IPAddress address, int port)
	{
		Connect(new IPEndPoint(address, port));
	}

	public void Connect(string host, int port)
	{
		Connect(Dns.GetHostAddresses(host), port);
	}

	public void Connect(EndPoint remoteEP)
	{
		ThrowIfDisposedAndClosed();
		if (remoteEP == null)
		{
			throw new ArgumentNullException("remoteEP");
		}
		IPEndPoint iPEndPoint = remoteEP as IPEndPoint;
		if (iPEndPoint != null && socketType != SocketType.Dgram && (iPEndPoint.Address.Equals(IPAddress.Any) || iPEndPoint.Address.Equals(IPAddress.IPv6Any)))
		{
			throw new SocketException(10049);
		}
		if (is_listening)
		{
			throw new InvalidOperationException();
		}
		if (iPEndPoint != null)
		{
			remoteEP = RemapIPEndPoint(iPEndPoint);
		}
		SocketAddress sa = remoteEP.Serialize();
		int error = 0;
		Connect_internal(m_Handle, sa, out error, is_blocking);
		if (error == 0 || error == 10035)
		{
			seed_endpoint = remoteEP;
		}
		if (error != 0)
		{
			if (is_closed)
			{
				error = 10004;
			}
			throw new SocketException(error);
		}
		is_connected = socketType != SocketType.Dgram || iPEndPoint == null || (!iPEndPoint.Address.Equals(IPAddress.Any) && !iPEndPoint.Address.Equals(IPAddress.IPv6Any));
		is_bound = true;
	}

	public bool ConnectAsync(SocketAsyncEventArgs e)
	{
		ThrowIfDisposedAndClosed();
		if (is_listening)
		{
			throw new InvalidOperationException("You may not perform this operation after calling the Listen method.");
		}
		if (e.RemoteEndPoint == null)
		{
			throw new ArgumentNullException("remoteEP");
		}
		InitSocketAsyncEventArgs(e, null, e, SocketOperation.Connect);
		try
		{
			SocketAsyncResult socketAsyncResult;
			if (!GetCheckedIPs(e, out var addresses))
			{
				socketAsyncResult = (SocketAsyncResult)BeginConnect(e.RemoteEndPoint, ConnectAsyncCallback, e);
			}
			else
			{
				DnsEndPoint dnsEndPoint = (DnsEndPoint)e.RemoteEndPoint;
				socketAsyncResult = (SocketAsyncResult)BeginConnect(addresses, dnsEndPoint.Port, ConnectAsyncCallback, e);
			}
			if (socketAsyncResult.IsCompleted && socketAsyncResult.CompletedSynchronously)
			{
				socketAsyncResult.CheckIfThrowDelayedException();
				return false;
			}
		}
		catch (Exception e2)
		{
			e.socket_async_result.Complete(e2, synch: true);
			return false;
		}
		return true;
	}

	public static void CancelConnectAsync(SocketAsyncEventArgs e)
	{
		if (e == null)
		{
			throw new ArgumentNullException("e");
		}
		if (e.in_progress != 0 && e.LastOperation == SocketAsyncOperation.Connect)
		{
			e.current_socket.Close();
		}
	}

	public IAsyncResult BeginConnect(string host, int port, AsyncCallback requestCallback, object state)
	{
		ThrowIfDisposedAndClosed();
		if (host == null)
		{
			throw new ArgumentNullException("host");
		}
		if (addressFamily != AddressFamily.InterNetwork && addressFamily != AddressFamily.InterNetworkV6)
		{
			throw new NotSupportedException("This method is valid only for sockets in the InterNetwork and InterNetworkV6 families");
		}
		if (port <= 0 || port > 65535)
		{
			throw new ArgumentOutOfRangeException("port", "Must be > 0 and < 65536");
		}
		if (is_listening)
		{
			throw new InvalidOperationException();
		}
		return BeginConnect(Dns.GetHostAddresses(host), port, requestCallback, state);
	}

	public IAsyncResult BeginConnect(EndPoint remoteEP, AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		if (remoteEP == null)
		{
			throw new ArgumentNullException("remoteEP");
		}
		if (is_listening)
		{
			throw new InvalidOperationException();
		}
		SocketAsyncResult obj = new SocketAsyncResult(this, callback, state, SocketOperation.Connect)
		{
			EndPoint = remoteEP
		};
		BeginSConnect(obj);
		return obj;
	}

	public IAsyncResult BeginConnect(IPAddress[] addresses, int port, AsyncCallback requestCallback, object state)
	{
		ThrowIfDisposedAndClosed();
		if (addresses == null)
		{
			throw new ArgumentNullException("addresses");
		}
		if (addresses.Length == 0)
		{
			throw new ArgumentException("Empty addresses list");
		}
		if (AddressFamily != AddressFamily.InterNetwork && AddressFamily != AddressFamily.InterNetworkV6)
		{
			throw new NotSupportedException("This method is only valid for addresses in the InterNetwork or InterNetworkV6 families");
		}
		if (port <= 0 || port > 65535)
		{
			throw new ArgumentOutOfRangeException("port", "Must be > 0 and < 65536");
		}
		if (is_listening)
		{
			throw new InvalidOperationException();
		}
		SocketAsyncResult obj = new SocketAsyncResult(this, requestCallback, state, SocketOperation.Connect)
		{
			Addresses = addresses,
			Port = port
		};
		is_connected = false;
		BeginMConnect(obj);
		return obj;
	}

	private static void BeginMConnect(SocketAsyncResult sockares)
	{
		Exception ex = null;
		for (int i = sockares.CurrentAddress; i < sockares.Addresses.Length; i++)
		{
			try
			{
				sockares.CurrentAddress++;
				sockares.EndPoint = new IPEndPoint(sockares.Addresses[i], sockares.Port);
				BeginSConnect(sockares);
				return;
			}
			catch (Exception ex2)
			{
				ex = ex2;
			}
		}
		throw ex;
	}

	private static void BeginSConnect(SocketAsyncResult sockares)
	{
		EndPoint endPoint = sockares.EndPoint;
		if (endPoint is IPEndPoint)
		{
			IPEndPoint iPEndPoint = (IPEndPoint)endPoint;
			if (iPEndPoint.Address.Equals(IPAddress.Any) || iPEndPoint.Address.Equals(IPAddress.IPv6Any))
			{
				sockares.Complete(new SocketException(10049), synch: true);
				return;
			}
			endPoint = (sockares.EndPoint = sockares.socket.RemapIPEndPoint(iPEndPoint));
		}
		int error = 0;
		if (sockares.socket.connect_in_progress)
		{
			sockares.socket.connect_in_progress = false;
			sockares.socket.m_Handle.Dispose();
			sockares.socket.m_Handle = new SafeSocketHandle(sockares.socket.Socket_internal(sockares.socket.addressFamily, sockares.socket.socketType, sockares.socket.protocolType, out error), ownsHandle: true);
			if (error != 0)
			{
				throw new SocketException(error);
			}
		}
		bool num = sockares.socket.is_blocking;
		if (num)
		{
			sockares.socket.Blocking = false;
		}
		Connect_internal(sockares.socket.m_Handle, endPoint.Serialize(), out error, blocking: false);
		if (num)
		{
			sockares.socket.Blocking = true;
		}
		switch (error)
		{
		case 0:
			sockares.socket.is_connected = true;
			sockares.socket.is_bound = true;
			sockares.Complete(synch: true);
			break;
		default:
			sockares.socket.is_connected = false;
			sockares.socket.is_bound = false;
			sockares.Complete(new SocketException(error), synch: true);
			break;
		case 10035:
		case 10036:
			sockares.socket.is_connected = false;
			sockares.socket.is_bound = false;
			sockares.socket.connect_in_progress = true;
			IOSelector.Add(sockares.Handle, new IOSelectorJob(IOOperation.Write, BeginConnectCallback, sockares));
			break;
		}
	}

	public void EndConnect(IAsyncResult asyncResult)
	{
		ThrowIfDisposedAndClosed();
		SocketAsyncResult socketAsyncResult = ValidateEndIAsyncResult(asyncResult, "EndConnect", "asyncResult");
		if (!socketAsyncResult.IsCompleted)
		{
			socketAsyncResult.AsyncWaitHandle.WaitOne();
		}
		socketAsyncResult.CheckIfThrowDelayedException();
	}

	private static void Connect_internal(SafeSocketHandle safeHandle, SocketAddress sa, out int error, bool blocking)
	{
		try
		{
			safeHandle.RegisterForBlockingSyscall();
			Connect_internal(safeHandle.DangerousGetHandle(), sa, out error, blocking);
		}
		finally
		{
			safeHandle.UnRegisterForBlockingSyscall();
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void Connect_internal(IntPtr sock, SocketAddress sa, out int error, bool blocking);

	private bool GetCheckedIPs(SocketAsyncEventArgs e, out IPAddress[] addresses)
	{
		addresses = null;
		if (e.RemoteEndPoint is DnsEndPoint dnsEndPoint)
		{
			if (dnsEndPoint.AddressFamily == AddressFamily.Unspecified)
			{
				addresses = Dns.GetHostAddresses(dnsEndPoint.Host);
			}
			else
			{
				IPAddress[] hostAddresses = Dns.GetHostAddresses(dnsEndPoint.Host);
				int num = 0;
				int[] array = new int[hostAddresses.Length];
				for (int i = 0; i < hostAddresses.Length; i++)
				{
					if (hostAddresses[i].AddressFamily == dnsEndPoint.AddressFamily)
					{
						array[num] = i;
						num++;
					}
				}
				addresses = new IPAddress[num];
				for (int j = 0; j < num; j++)
				{
					addresses[j] = hostAddresses[array[j]];
				}
			}
			return true;
		}
		e.ConnectByNameError = null;
		return false;
	}

	public void Disconnect(bool reuseSocket)
	{
		ThrowIfDisposedAndClosed();
		int error = 0;
		Disconnect_internal(m_Handle, reuseSocket, out error);
		switch (error)
		{
		case 50:
			throw new PlatformNotSupportedException();
		default:
			throw new SocketException(error);
		case 0:
			is_connected = false;
			break;
		}
	}

	public bool DisconnectAsync(SocketAsyncEventArgs e)
	{
		ThrowIfDisposedAndClosed();
		InitSocketAsyncEventArgs(e, DisconnectAsyncCallback, e, SocketOperation.Disconnect);
		IOSelector.Add(e.socket_async_result.Handle, new IOSelectorJob(IOOperation.Write, BeginDisconnectCallback, e.socket_async_result));
		return true;
	}

	public IAsyncResult BeginDisconnect(bool reuseSocket, AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		SocketAsyncResult socketAsyncResult = new SocketAsyncResult(this, callback, state, SocketOperation.Disconnect)
		{
			ReuseSocket = reuseSocket
		};
		IOSelector.Add(socketAsyncResult.Handle, new IOSelectorJob(IOOperation.Write, BeginDisconnectCallback, socketAsyncResult));
		return socketAsyncResult;
	}

	public void EndDisconnect(IAsyncResult asyncResult)
	{
		ThrowIfDisposedAndClosed();
		SocketAsyncResult socketAsyncResult = ValidateEndIAsyncResult(asyncResult, "EndDisconnect", "asyncResult");
		if (!socketAsyncResult.IsCompleted)
		{
			socketAsyncResult.AsyncWaitHandle.WaitOne();
		}
		socketAsyncResult.CheckIfThrowDelayedException();
	}

	private static void Disconnect_internal(SafeSocketHandle safeHandle, bool reuse, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			Disconnect_internal(safeHandle.DangerousGetHandle(), reuse, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void Disconnect_internal(IntPtr sock, bool reuse, out int error);

	public unsafe int Receive(byte[] buffer, int offset, int size, SocketFlags socketFlags, out SocketError errorCode)
	{
		ThrowIfDisposedAndClosed();
		ThrowIfBufferNull(buffer);
		ThrowIfBufferOutOfRange(buffer, offset, size);
		int result;
		int error;
		fixed (byte* ptr = buffer)
		{
			result = Receive_internal(m_Handle, ptr + offset, size, socketFlags, out error, is_blocking);
		}
		errorCode = (SocketError)error;
		if (errorCode != 0 && errorCode != SocketError.WouldBlock && errorCode != SocketError.InProgress)
		{
			is_connected = false;
			is_bound = false;
			return result;
		}
		is_connected = true;
		return result;
	}

	[CLSCompliant(false)]
	public unsafe int Receive(IList<ArraySegment<byte>> buffers, SocketFlags socketFlags, out SocketError errorCode)
	{
		ThrowIfDisposedAndClosed();
		if (buffers == null || buffers.Count == 0)
		{
			throw new ArgumentNullException("buffers");
		}
		int count = buffers.Count;
		GCHandle[] array = new GCHandle[count];
		int result;
		int error;
		try
		{
			fixed (WSABUF* ptr = new WSABUF[count])
			{
				for (int i = 0; i < count; i++)
				{
					ArraySegment<byte> arraySegment = buffers[i];
					if (arraySegment.Offset < 0 || arraySegment.Count < 0 || arraySegment.Count > arraySegment.Array.Length - arraySegment.Offset)
					{
						throw new ArgumentOutOfRangeException("segment");
					}
					try
					{
					}
					finally
					{
						array[i] = GCHandle.Alloc(arraySegment.Array, GCHandleType.Pinned);
					}
					ptr[i].len = arraySegment.Count;
					ptr[i].buf = Marshal.UnsafeAddrOfPinnedArrayElement(arraySegment.Array, arraySegment.Offset);
				}
				result = Receive_internal(m_Handle, ptr, count, socketFlags, out error, is_blocking);
			}
		}
		finally
		{
			for (int j = 0; j < count; j++)
			{
				if (array[j].IsAllocated)
				{
					array[j].Free();
				}
			}
		}
		errorCode = (SocketError)error;
		return result;
	}

	public bool ReceiveAsync(SocketAsyncEventArgs e)
	{
		ThrowIfDisposedAndClosed();
		if (e.Buffer == null && e.BufferList == null)
		{
			throw new NullReferenceException("Either e.Buffer or e.BufferList must be valid buffers.");
		}
		if (e.Buffer == null)
		{
			InitSocketAsyncEventArgs(e, ReceiveAsyncCallback, e, SocketOperation.ReceiveGeneric);
			e.socket_async_result.Buffers = e.BufferList;
			QueueIOSelectorJob(ReadSem, e.socket_async_result.Handle, new IOSelectorJob(IOOperation.Read, BeginReceiveGenericCallback, e.socket_async_result));
		}
		else
		{
			InitSocketAsyncEventArgs(e, ReceiveAsyncCallback, e, SocketOperation.Receive);
			e.socket_async_result.Buffer = e.Buffer;
			e.socket_async_result.Offset = e.Offset;
			e.socket_async_result.Size = e.Count;
			QueueIOSelectorJob(ReadSem, e.socket_async_result.Handle, new IOSelectorJob(IOOperation.Read, BeginReceiveCallback, e.socket_async_result));
		}
		return true;
	}

	public IAsyncResult BeginReceive(byte[] buffer, int offset, int size, SocketFlags socketFlags, out SocketError errorCode, AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		ThrowIfBufferNull(buffer);
		ThrowIfBufferOutOfRange(buffer, offset, size);
		errorCode = SocketError.Success;
		SocketAsyncResult socketAsyncResult = new SocketAsyncResult(this, callback, state, SocketOperation.Receive)
		{
			Buffer = buffer,
			Offset = offset,
			Size = size,
			SockFlags = socketFlags
		};
		QueueIOSelectorJob(ReadSem, socketAsyncResult.Handle, new IOSelectorJob(IOOperation.Read, BeginReceiveCallback, socketAsyncResult));
		return socketAsyncResult;
	}

	[CLSCompliant(false)]
	public IAsyncResult BeginReceive(IList<ArraySegment<byte>> buffers, SocketFlags socketFlags, out SocketError errorCode, AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		if (buffers == null)
		{
			throw new ArgumentNullException("buffers");
		}
		errorCode = SocketError.Success;
		SocketAsyncResult socketAsyncResult = new SocketAsyncResult(this, callback, state, SocketOperation.ReceiveGeneric)
		{
			Buffers = buffers,
			SockFlags = socketFlags
		};
		QueueIOSelectorJob(ReadSem, socketAsyncResult.Handle, new IOSelectorJob(IOOperation.Read, BeginReceiveGenericCallback, socketAsyncResult));
		return socketAsyncResult;
	}

	public int EndReceive(IAsyncResult asyncResult, out SocketError errorCode)
	{
		ThrowIfDisposedAndClosed();
		SocketAsyncResult socketAsyncResult = ValidateEndIAsyncResult(asyncResult, "EndReceive", "asyncResult");
		if (!socketAsyncResult.IsCompleted)
		{
			socketAsyncResult.AsyncWaitHandle.WaitOne();
		}
		errorCode = socketAsyncResult.ErrorCode;
		if (errorCode != 0 && errorCode != SocketError.WouldBlock && errorCode != SocketError.InProgress)
		{
			is_connected = false;
		}
		if (errorCode == SocketError.Success)
		{
			socketAsyncResult.CheckIfThrowDelayedException();
		}
		return socketAsyncResult.Total;
	}

	private unsafe static int Receive_internal(SafeSocketHandle safeHandle, WSABUF* bufarray, int count, SocketFlags flags, out int error, bool blocking)
	{
		try
		{
			safeHandle.RegisterForBlockingSyscall();
			return Receive_internal(safeHandle.DangerousGetHandle(), bufarray, count, flags, out error, blocking);
		}
		finally
		{
			safeHandle.UnRegisterForBlockingSyscall();
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private unsafe static extern int Receive_internal(IntPtr sock, WSABUF* bufarray, int count, SocketFlags flags, out int error, bool blocking);

	private unsafe static int Receive_internal(SafeSocketHandle safeHandle, byte* buffer, int count, SocketFlags flags, out int error, bool blocking)
	{
		try
		{
			safeHandle.RegisterForBlockingSyscall();
			return Receive_internal(safeHandle.DangerousGetHandle(), buffer, count, flags, out error, blocking);
		}
		finally
		{
			safeHandle.UnRegisterForBlockingSyscall();
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private unsafe static extern int Receive_internal(IntPtr sock, byte* buffer, int count, SocketFlags flags, out int error, bool blocking);

	public int ReceiveFrom(byte[] buffer, int offset, int size, SocketFlags socketFlags, ref EndPoint remoteEP)
	{
		ThrowIfDisposedAndClosed();
		ThrowIfBufferNull(buffer);
		ThrowIfBufferOutOfRange(buffer, offset, size);
		if (remoteEP == null)
		{
			throw new ArgumentNullException("remoteEP");
		}
		SocketError errorCode;
		int result = ReceiveFrom(buffer, offset, size, socketFlags, ref remoteEP, out errorCode);
		if (errorCode != 0)
		{
			throw new SocketException(errorCode);
		}
		return result;
	}

	internal unsafe int ReceiveFrom(byte[] buffer, int offset, int size, SocketFlags socketFlags, ref EndPoint remoteEP, out SocketError errorCode)
	{
		SocketAddress sockaddr = remoteEP.Serialize();
		int result;
		int error;
		fixed (byte* ptr = buffer)
		{
			result = ReceiveFrom_internal(m_Handle, ptr + offset, size, socketFlags, ref sockaddr, out error, is_blocking);
		}
		errorCode = (SocketError)error;
		if (errorCode != 0)
		{
			if (errorCode != SocketError.WouldBlock && errorCode != SocketError.InProgress)
			{
				is_connected = false;
			}
			else if (errorCode == SocketError.WouldBlock && is_blocking)
			{
				errorCode = SocketError.TimedOut;
			}
			return 0;
		}
		is_connected = true;
		is_bound = true;
		if (sockaddr != null)
		{
			remoteEP = remoteEP.Create(sockaddr);
		}
		seed_endpoint = remoteEP;
		return result;
	}

	public bool ReceiveFromAsync(SocketAsyncEventArgs e)
	{
		ThrowIfDisposedAndClosed();
		if (e.BufferList != null)
		{
			throw new NotSupportedException("Mono doesn't support using BufferList at this point.");
		}
		if (e.RemoteEndPoint == null)
		{
			throw new ArgumentNullException("remoteEP", "Value cannot be null.");
		}
		InitSocketAsyncEventArgs(e, ReceiveFromAsyncCallback, e, SocketOperation.ReceiveFrom);
		e.socket_async_result.Buffer = e.Buffer;
		e.socket_async_result.Offset = e.Offset;
		e.socket_async_result.Size = e.Count;
		e.socket_async_result.EndPoint = e.RemoteEndPoint;
		e.socket_async_result.SockFlags = e.SocketFlags;
		QueueIOSelectorJob(ReadSem, e.socket_async_result.Handle, new IOSelectorJob(IOOperation.Read, BeginReceiveFromCallback, e.socket_async_result));
		return true;
	}

	public IAsyncResult BeginReceiveFrom(byte[] buffer, int offset, int size, SocketFlags socketFlags, ref EndPoint remoteEP, AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		ThrowIfBufferNull(buffer);
		ThrowIfBufferOutOfRange(buffer, offset, size);
		if (remoteEP == null)
		{
			throw new ArgumentNullException("remoteEP");
		}
		SocketAsyncResult socketAsyncResult = new SocketAsyncResult(this, callback, state, SocketOperation.ReceiveFrom)
		{
			Buffer = buffer,
			Offset = offset,
			Size = size,
			SockFlags = socketFlags,
			EndPoint = remoteEP
		};
		QueueIOSelectorJob(ReadSem, socketAsyncResult.Handle, new IOSelectorJob(IOOperation.Read, BeginReceiveFromCallback, socketAsyncResult));
		return socketAsyncResult;
	}

	public int EndReceiveFrom(IAsyncResult asyncResult, ref EndPoint endPoint)
	{
		ThrowIfDisposedAndClosed();
		if (endPoint == null)
		{
			throw new ArgumentNullException("endPoint");
		}
		SocketAsyncResult socketAsyncResult = ValidateEndIAsyncResult(asyncResult, "EndReceiveFrom", "asyncResult");
		if (!socketAsyncResult.IsCompleted)
		{
			socketAsyncResult.AsyncWaitHandle.WaitOne();
		}
		socketAsyncResult.CheckIfThrowDelayedException();
		endPoint = socketAsyncResult.EndPoint;
		return socketAsyncResult.Total;
	}

	private unsafe static int ReceiveFrom_internal(SafeSocketHandle safeHandle, byte* buffer, int count, SocketFlags flags, ref SocketAddress sockaddr, out int error, bool blocking)
	{
		try
		{
			safeHandle.RegisterForBlockingSyscall();
			return ReceiveFrom_internal(safeHandle.DangerousGetHandle(), buffer, count, flags, ref sockaddr, out error, blocking);
		}
		finally
		{
			safeHandle.UnRegisterForBlockingSyscall();
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private unsafe static extern int ReceiveFrom_internal(IntPtr sock, byte* buffer, int count, SocketFlags flags, ref SocketAddress sockaddr, out int error, bool blocking);

	[System.MonoTODO("Not implemented")]
	public int ReceiveMessageFrom(byte[] buffer, int offset, int size, ref SocketFlags socketFlags, ref EndPoint remoteEP, out IPPacketInformation ipPacketInformation)
	{
		ThrowIfDisposedAndClosed();
		ThrowIfBufferNull(buffer);
		ThrowIfBufferOutOfRange(buffer, offset, size);
		if (remoteEP == null)
		{
			throw new ArgumentNullException("remoteEP");
		}
		throw new NotImplementedException();
	}

	[System.MonoTODO("Not implemented")]
	public bool ReceiveMessageFromAsync(SocketAsyncEventArgs e)
	{
		ThrowIfDisposedAndClosed();
		throw new NotImplementedException();
	}

	[System.MonoTODO]
	public IAsyncResult BeginReceiveMessageFrom(byte[] buffer, int offset, int size, SocketFlags socketFlags, ref EndPoint remoteEP, AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		ThrowIfBufferNull(buffer);
		ThrowIfBufferOutOfRange(buffer, offset, size);
		if (remoteEP == null)
		{
			throw new ArgumentNullException("remoteEP");
		}
		throw new NotImplementedException();
	}

	[System.MonoTODO]
	public int EndReceiveMessageFrom(IAsyncResult asyncResult, ref SocketFlags socketFlags, ref EndPoint endPoint, out IPPacketInformation ipPacketInformation)
	{
		ThrowIfDisposedAndClosed();
		if (endPoint == null)
		{
			throw new ArgumentNullException("endPoint");
		}
		ValidateEndIAsyncResult(asyncResult, "EndReceiveMessageFrom", "asyncResult");
		throw new NotImplementedException();
	}

	public unsafe int Send(byte[] buffer, int offset, int size, SocketFlags socketFlags, out SocketError errorCode)
	{
		ThrowIfDisposedAndClosed();
		ThrowIfBufferNull(buffer);
		ThrowIfBufferOutOfRange(buffer, offset, size);
		if (size == 0)
		{
			errorCode = SocketError.Success;
			return 0;
		}
		int num = 0;
		do
		{
			int error;
			fixed (byte* ptr = buffer)
			{
				num += Send_internal(m_Handle, ptr + (offset + num), size - num, socketFlags, out error, is_blocking);
			}
			errorCode = (SocketError)error;
			if (errorCode != 0 && errorCode != SocketError.WouldBlock && errorCode != SocketError.InProgress)
			{
				is_connected = false;
				is_bound = false;
				break;
			}
			is_connected = true;
		}
		while (num < size);
		return num;
	}

	[CLSCompliant(false)]
	public unsafe int Send(IList<ArraySegment<byte>> buffers, SocketFlags socketFlags, out SocketError errorCode)
	{
		ThrowIfDisposedAndClosed();
		if (buffers == null)
		{
			throw new ArgumentNullException("buffers");
		}
		if (buffers.Count == 0)
		{
			throw new ArgumentException("Buffer is empty", "buffers");
		}
		int count = buffers.Count;
		GCHandle[] array = new GCHandle[count];
		int result;
		int error;
		try
		{
			fixed (WSABUF* ptr = new WSABUF[count])
			{
				for (int i = 0; i < count; i++)
				{
					ArraySegment<byte> arraySegment = buffers[i];
					if (arraySegment.Offset < 0 || arraySegment.Count < 0 || arraySegment.Count > arraySegment.Array.Length - arraySegment.Offset)
					{
						throw new ArgumentOutOfRangeException("segment");
					}
					try
					{
					}
					finally
					{
						array[i] = GCHandle.Alloc(arraySegment.Array, GCHandleType.Pinned);
					}
					ptr[i].len = arraySegment.Count;
					ptr[i].buf = Marshal.UnsafeAddrOfPinnedArrayElement(arraySegment.Array, arraySegment.Offset);
				}
				result = Send_internal(m_Handle, ptr, count, socketFlags, out error, is_blocking);
			}
		}
		finally
		{
			for (int j = 0; j < count; j++)
			{
				if (array[j].IsAllocated)
				{
					array[j].Free();
				}
			}
		}
		errorCode = (SocketError)error;
		return result;
	}

	public bool SendAsync(SocketAsyncEventArgs e)
	{
		ThrowIfDisposedAndClosed();
		if (e.Buffer == null && e.BufferList == null)
		{
			throw new NullReferenceException("Either e.Buffer or e.BufferList must be valid buffers.");
		}
		if (e.Buffer == null)
		{
			InitSocketAsyncEventArgs(e, SendAsyncCallback, e, SocketOperation.SendGeneric);
			e.socket_async_result.Buffers = e.BufferList;
			QueueIOSelectorJob(WriteSem, e.socket_async_result.Handle, new IOSelectorJob(IOOperation.Write, BeginSendGenericCallback, e.socket_async_result));
		}
		else
		{
			InitSocketAsyncEventArgs(e, SendAsyncCallback, e, SocketOperation.Send);
			e.socket_async_result.Buffer = e.Buffer;
			e.socket_async_result.Offset = e.Offset;
			e.socket_async_result.Size = e.Count;
			QueueIOSelectorJob(WriteSem, e.socket_async_result.Handle, new IOSelectorJob(IOOperation.Write, delegate(IOAsyncResult s)
			{
				BeginSendCallback((SocketAsyncResult)s, 0);
			}, e.socket_async_result));
		}
		return true;
	}

	public IAsyncResult BeginSend(byte[] buffer, int offset, int size, SocketFlags socketFlags, out SocketError errorCode, AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		ThrowIfBufferNull(buffer);
		ThrowIfBufferOutOfRange(buffer, offset, size);
		if (!is_connected)
		{
			errorCode = SocketError.NotConnected;
			return null;
		}
		errorCode = SocketError.Success;
		SocketAsyncResult socketAsyncResult = new SocketAsyncResult(this, callback, state, SocketOperation.Send)
		{
			Buffer = buffer,
			Offset = offset,
			Size = size,
			SockFlags = socketFlags
		};
		QueueIOSelectorJob(WriteSem, socketAsyncResult.Handle, new IOSelectorJob(IOOperation.Write, delegate(IOAsyncResult s)
		{
			BeginSendCallback((SocketAsyncResult)s, 0);
		}, socketAsyncResult));
		return socketAsyncResult;
	}

	private unsafe static void BeginSendCallback(SocketAsyncResult sockares, int sent_so_far)
	{
		int num = 0;
		try
		{
			fixed (byte* ptr = sockares.Buffer)
			{
				num = Send_internal(sockares.socket.m_Handle, ptr + sockares.Offset, sockares.Size, sockares.SockFlags, out sockares.error, blocking: false);
			}
		}
		catch (Exception e)
		{
			sockares.Complete(e);
			return;
		}
		if (sockares.error == 0)
		{
			sent_so_far += num;
			sockares.Offset += num;
			sockares.Size -= num;
			if (sockares.socket.CleanedUp)
			{
				sockares.Complete(sent_so_far);
				return;
			}
			if (sockares.Size > 0)
			{
				IOSelector.Add(sockares.Handle, new IOSelectorJob(IOOperation.Write, delegate(IOAsyncResult s)
				{
					BeginSendCallback((SocketAsyncResult)s, sent_so_far);
				}, sockares));
				return;
			}
			sockares.Total = sent_so_far;
		}
		sockares.Complete(sent_so_far);
	}

	[CLSCompliant(false)]
	public IAsyncResult BeginSend(IList<ArraySegment<byte>> buffers, SocketFlags socketFlags, out SocketError errorCode, AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		if (buffers == null)
		{
			throw new ArgumentNullException("buffers");
		}
		if (!is_connected)
		{
			errorCode = SocketError.NotConnected;
			return null;
		}
		errorCode = SocketError.Success;
		SocketAsyncResult socketAsyncResult = new SocketAsyncResult(this, callback, state, SocketOperation.SendGeneric)
		{
			Buffers = buffers,
			SockFlags = socketFlags
		};
		QueueIOSelectorJob(WriteSem, socketAsyncResult.Handle, new IOSelectorJob(IOOperation.Write, BeginSendGenericCallback, socketAsyncResult));
		return socketAsyncResult;
	}

	public int EndSend(IAsyncResult asyncResult, out SocketError errorCode)
	{
		ThrowIfDisposedAndClosed();
		SocketAsyncResult socketAsyncResult = ValidateEndIAsyncResult(asyncResult, "EndSend", "asyncResult");
		if (!socketAsyncResult.IsCompleted)
		{
			socketAsyncResult.AsyncWaitHandle.WaitOne();
		}
		errorCode = socketAsyncResult.ErrorCode;
		if (errorCode != 0 && errorCode != SocketError.WouldBlock && errorCode != SocketError.InProgress)
		{
			is_connected = false;
		}
		if (errorCode == SocketError.Success)
		{
			socketAsyncResult.CheckIfThrowDelayedException();
		}
		return socketAsyncResult.Total;
	}

	private unsafe static int Send_internal(SafeSocketHandle safeHandle, WSABUF* bufarray, int count, SocketFlags flags, out int error, bool blocking)
	{
		try
		{
			safeHandle.RegisterForBlockingSyscall();
			return Send_internal(safeHandle.DangerousGetHandle(), bufarray, count, flags, out error, blocking);
		}
		finally
		{
			safeHandle.UnRegisterForBlockingSyscall();
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private unsafe static extern int Send_internal(IntPtr sock, WSABUF* bufarray, int count, SocketFlags flags, out int error, bool blocking);

	private unsafe static int Send_internal(SafeSocketHandle safeHandle, byte* buffer, int count, SocketFlags flags, out int error, bool blocking)
	{
		try
		{
			safeHandle.RegisterForBlockingSyscall();
			return Send_internal(safeHandle.DangerousGetHandle(), buffer, count, flags, out error, blocking);
		}
		finally
		{
			safeHandle.UnRegisterForBlockingSyscall();
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private unsafe static extern int Send_internal(IntPtr sock, byte* buffer, int count, SocketFlags flags, out int error, bool blocking);

	public unsafe int SendTo(byte[] buffer, int offset, int size, SocketFlags socketFlags, EndPoint remoteEP)
	{
		ThrowIfDisposedAndClosed();
		ThrowIfBufferNull(buffer);
		ThrowIfBufferOutOfRange(buffer, offset, size);
		if (remoteEP == null)
		{
			throw new ArgumentNullException("remoteEP");
		}
		int result;
		int error;
		fixed (byte* ptr = buffer)
		{
			result = SendTo_internal(m_Handle, ptr + offset, size, socketFlags, remoteEP.Serialize(), out error, is_blocking);
		}
		SocketError socketError = (SocketError)error;
		if (socketError != 0)
		{
			if (socketError != SocketError.WouldBlock && socketError != SocketError.InProgress)
			{
				is_connected = false;
			}
			throw new SocketException(error);
		}
		is_connected = true;
		is_bound = true;
		seed_endpoint = remoteEP;
		return result;
	}

	public bool SendToAsync(SocketAsyncEventArgs e)
	{
		ThrowIfDisposedAndClosed();
		if (e.BufferList != null)
		{
			throw new NotSupportedException("Mono doesn't support using BufferList at this point.");
		}
		if (e.RemoteEndPoint == null)
		{
			throw new ArgumentNullException("remoteEP", "Value cannot be null.");
		}
		InitSocketAsyncEventArgs(e, SendToAsyncCallback, e, SocketOperation.SendTo);
		e.socket_async_result.Buffer = e.Buffer;
		e.socket_async_result.Offset = e.Offset;
		e.socket_async_result.Size = e.Count;
		e.socket_async_result.SockFlags = e.SocketFlags;
		e.socket_async_result.EndPoint = e.RemoteEndPoint;
		QueueIOSelectorJob(WriteSem, e.socket_async_result.Handle, new IOSelectorJob(IOOperation.Write, delegate(IOAsyncResult s)
		{
			BeginSendToCallback((SocketAsyncResult)s, 0);
		}, e.socket_async_result));
		return true;
	}

	public IAsyncResult BeginSendTo(byte[] buffer, int offset, int size, SocketFlags socketFlags, EndPoint remoteEP, AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		ThrowIfBufferNull(buffer);
		ThrowIfBufferOutOfRange(buffer, offset, size);
		SocketAsyncResult socketAsyncResult = new SocketAsyncResult(this, callback, state, SocketOperation.SendTo)
		{
			Buffer = buffer,
			Offset = offset,
			Size = size,
			SockFlags = socketFlags,
			EndPoint = remoteEP
		};
		QueueIOSelectorJob(WriteSem, socketAsyncResult.Handle, new IOSelectorJob(IOOperation.Write, delegate(IOAsyncResult s)
		{
			BeginSendToCallback((SocketAsyncResult)s, 0);
		}, socketAsyncResult));
		return socketAsyncResult;
	}

	private static void BeginSendToCallback(SocketAsyncResult sockares, int sent_so_far)
	{
		int num = 0;
		try
		{
			num = sockares.socket.SendTo(sockares.Buffer, sockares.Offset, sockares.Size, sockares.SockFlags, sockares.EndPoint);
			if (sockares.error == 0)
			{
				sent_so_far += num;
				sockares.Offset += num;
				sockares.Size -= num;
			}
			if (sockares.Size > 0)
			{
				IOSelector.Add(sockares.Handle, new IOSelectorJob(IOOperation.Write, delegate(IOAsyncResult s)
				{
					BeginSendToCallback((SocketAsyncResult)s, sent_so_far);
				}, sockares));
				return;
			}
			sockares.Total = sent_so_far;
		}
		catch (Exception e)
		{
			sockares.Complete(e);
			return;
		}
		sockares.Complete();
	}

	public int EndSendTo(IAsyncResult asyncResult)
	{
		ThrowIfDisposedAndClosed();
		SocketAsyncResult socketAsyncResult = ValidateEndIAsyncResult(asyncResult, "EndSendTo", "result");
		if (!socketAsyncResult.IsCompleted)
		{
			socketAsyncResult.AsyncWaitHandle.WaitOne();
		}
		socketAsyncResult.CheckIfThrowDelayedException();
		return socketAsyncResult.Total;
	}

	private unsafe static int SendTo_internal(SafeSocketHandle safeHandle, byte* buffer, int count, SocketFlags flags, SocketAddress sa, out int error, bool blocking)
	{
		try
		{
			safeHandle.RegisterForBlockingSyscall();
			return SendTo_internal(safeHandle.DangerousGetHandle(), buffer, count, flags, sa, out error, blocking);
		}
		finally
		{
			safeHandle.UnRegisterForBlockingSyscall();
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private unsafe static extern int SendTo_internal(IntPtr sock, byte* buffer, int count, SocketFlags flags, SocketAddress sa, out int error, bool blocking);

	public void SendFile(string fileName, byte[] preBuffer, byte[] postBuffer, TransmitFileOptions flags)
	{
		ThrowIfDisposedAndClosed();
		if (!is_connected)
		{
			throw new NotSupportedException();
		}
		if (!is_blocking)
		{
			throw new InvalidOperationException();
		}
		int error = 0;
		if (!SendFile_internal(m_Handle, fileName, preBuffer, postBuffer, flags, out error, is_blocking) || error != 0)
		{
			SocketException ex = new SocketException(error);
			if (ex.ErrorCode == 2 || ex.ErrorCode == 3)
			{
				throw new FileNotFoundException();
			}
			throw ex;
		}
	}

	public IAsyncResult BeginSendFile(string fileName, byte[] preBuffer, byte[] postBuffer, TransmitFileOptions flags, AsyncCallback callback, object state)
	{
		ThrowIfDisposedAndClosed();
		if (!is_connected)
		{
			throw new NotSupportedException();
		}
		if (!File.Exists(fileName))
		{
			throw new FileNotFoundException();
		}
		SendFileHandler handler = SendFile;
		return new SendFileAsyncResult(handler, handler.BeginInvoke(fileName, preBuffer, postBuffer, flags, delegate(IAsyncResult ar)
		{
			callback(new SendFileAsyncResult(handler, ar));
		}, state));
	}

	public void EndSendFile(IAsyncResult asyncResult)
	{
		ThrowIfDisposedAndClosed();
		if (asyncResult == null)
		{
			throw new ArgumentNullException("asyncResult");
		}
		if (!(asyncResult is SendFileAsyncResult sendFileAsyncResult))
		{
			throw new ArgumentException("Invalid IAsyncResult", "asyncResult");
		}
		sendFileAsyncResult.Delegate.EndInvoke(sendFileAsyncResult.Original);
	}

	private static bool SendFile_internal(SafeSocketHandle safeHandle, string filename, byte[] pre_buffer, byte[] post_buffer, TransmitFileOptions flags, out int error, bool blocking)
	{
		try
		{
			safeHandle.RegisterForBlockingSyscall();
			return SendFile_internal(safeHandle.DangerousGetHandle(), filename, pre_buffer, post_buffer, flags, out error, blocking);
		}
		finally
		{
			safeHandle.UnRegisterForBlockingSyscall();
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool SendFile_internal(IntPtr sock, string filename, byte[] pre_buffer, byte[] post_buffer, TransmitFileOptions flags, out int error, bool blocking);

	[System.MonoTODO("Not implemented")]
	public bool SendPacketsAsync(SocketAsyncEventArgs e)
	{
		ThrowIfDisposedAndClosed();
		throw new NotImplementedException();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool Duplicate_internal(IntPtr handle, int targetProcessId, out IntPtr duplicateHandle, out MonoIOError error);

	[System.MonoLimitation("We do not support passing sockets across processes, we merely allow this API to pass the socket across AppDomains")]
	public SocketInformation DuplicateAndClose(int targetProcessId)
	{
		SocketInformation result = default(SocketInformation);
		result.Options = (SocketInformationOptions)((int)((is_listening ? SocketInformationOptions.Listening : ((SocketInformationOptions)0)) | (is_connected ? SocketInformationOptions.Connected : ((SocketInformationOptions)0))) | ((!is_blocking) ? 1 : 0) | (int)(useOverlappedIO ? SocketInformationOptions.UseOnlyOverlappedIO : ((SocketInformationOptions)0)));
		if (!Duplicate_internal(Handle, targetProcessId, out var duplicateHandle, out var error))
		{
			throw MonoIO.GetException(error);
		}
		result.ProtocolInformation = DataConverter.Pack("iiiil", (int)addressFamily, (int)socketType, (int)protocolType, is_bound ? 1 : 0, (long)duplicateHandle);
		m_Handle = null;
		return result;
	}

	public void GetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, byte[] optionValue)
	{
		ThrowIfDisposedAndClosed();
		if (optionValue == null)
		{
			throw new SocketException(10014, "Error trying to dereference an invalid pointer");
		}
		GetSocketOption_arr_internal(m_Handle, optionLevel, optionName, ref optionValue, out var error);
		if (error != 0)
		{
			throw new SocketException(error);
		}
	}

	public byte[] GetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, int optionLength)
	{
		ThrowIfDisposedAndClosed();
		byte[] byte_val = new byte[optionLength];
		GetSocketOption_arr_internal(m_Handle, optionLevel, optionName, ref byte_val, out var error);
		if (error != 0)
		{
			throw new SocketException(error);
		}
		return byte_val;
	}

	public object GetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName)
	{
		ThrowIfDisposedAndClosed();
		GetSocketOption_obj_internal(m_Handle, optionLevel, optionName, out var obj_val, out var error);
		if (error != 0)
		{
			throw new SocketException(error);
		}
		switch (optionName)
		{
		case SocketOptionName.Linger:
			return (LingerOption)obj_val;
		case SocketOptionName.AddMembership:
		case SocketOptionName.DropMembership:
			return (MulticastOption)obj_val;
		default:
			if (obj_val is int)
			{
				return (int)obj_val;
			}
			return obj_val;
		}
	}

	private static void GetSocketOption_arr_internal(SafeSocketHandle safeHandle, SocketOptionLevel level, SocketOptionName name, ref byte[] byte_val, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			GetSocketOption_arr_internal(safeHandle.DangerousGetHandle(), level, name, ref byte_val, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void GetSocketOption_arr_internal(IntPtr socket, SocketOptionLevel level, SocketOptionName name, ref byte[] byte_val, out int error);

	private static void GetSocketOption_obj_internal(SafeSocketHandle safeHandle, SocketOptionLevel level, SocketOptionName name, out object obj_val, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			GetSocketOption_obj_internal(safeHandle.DangerousGetHandle(), level, name, out obj_val, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void GetSocketOption_obj_internal(IntPtr socket, SocketOptionLevel level, SocketOptionName name, out object obj_val, out int error);

	public void SetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, byte[] optionValue)
	{
		ThrowIfDisposedAndClosed();
		if (optionValue == null)
		{
			throw new SocketException(10014, "Error trying to dereference an invalid pointer");
		}
		SetSocketOption_internal(m_Handle, optionLevel, optionName, null, optionValue, 0, out var error);
		switch (error)
		{
		case 10022:
			throw new ArgumentException();
		default:
			throw new SocketException(error);
		case 0:
			break;
		}
	}

	public void SetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, object optionValue)
	{
		ThrowIfDisposedAndClosed();
		if (optionValue == null)
		{
			throw new ArgumentNullException("optionValue");
		}
		int error;
		if (optionLevel == SocketOptionLevel.Socket && optionName == SocketOptionName.Linger)
		{
			if (!(optionValue is LingerOption obj_val))
			{
				throw new ArgumentException("A 'LingerOption' value must be specified.", "optionValue");
			}
			SetSocketOption_internal(m_Handle, optionLevel, optionName, obj_val, null, 0, out error);
		}
		else if (optionLevel == SocketOptionLevel.IP && (optionName == SocketOptionName.AddMembership || optionName == SocketOptionName.DropMembership))
		{
			if (!(optionValue is MulticastOption obj_val2))
			{
				throw new ArgumentException("A 'MulticastOption' value must be specified.", "optionValue");
			}
			SetSocketOption_internal(m_Handle, optionLevel, optionName, obj_val2, null, 0, out error);
		}
		else
		{
			if (optionLevel != SocketOptionLevel.IPv6 || (optionName != SocketOptionName.AddMembership && optionName != SocketOptionName.DropMembership))
			{
				throw new ArgumentException("Invalid value specified.", "optionValue");
			}
			if (!(optionValue is IPv6MulticastOption obj_val3))
			{
				throw new ArgumentException("A 'IPv6MulticastOption' value must be specified.", "optionValue");
			}
			SetSocketOption_internal(m_Handle, optionLevel, optionName, obj_val3, null, 0, out error);
		}
		switch (error)
		{
		case 10022:
			throw new ArgumentException();
		default:
			throw new SocketException(error);
		case 0:
			break;
		}
	}

	public void SetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, bool optionValue)
	{
		int optionValue2 = (optionValue ? 1 : 0);
		SetSocketOption(optionLevel, optionName, optionValue2);
	}

	public void SetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, int optionValue)
	{
		ThrowIfDisposedAndClosed();
		SetSocketOption_internal(m_Handle, optionLevel, optionName, null, null, optionValue, out var error);
		switch (error)
		{
		case 10022:
			throw new ArgumentException();
		default:
			throw new SocketException(error);
		case 0:
			break;
		}
	}

	private static void SetSocketOption_internal(SafeSocketHandle safeHandle, SocketOptionLevel level, SocketOptionName name, object obj_val, byte[] byte_val, int int_val, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			SetSocketOption_internal(safeHandle.DangerousGetHandle(), level, name, obj_val, byte_val, int_val, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void SetSocketOption_internal(IntPtr socket, SocketOptionLevel level, SocketOptionName name, object obj_val, byte[] byte_val, int int_val, out int error);

	public int IOControl(int ioControlCode, byte[] optionInValue, byte[] optionOutValue)
	{
		if (CleanedUp)
		{
			throw new ObjectDisposedException(GetType().ToString());
		}
		int error;
		int num = IOControl_internal(m_Handle, ioControlCode, optionInValue, optionOutValue, out error);
		if (error != 0)
		{
			throw new SocketException(error);
		}
		if (num == -1)
		{
			throw new InvalidOperationException("Must use Blocking property instead.");
		}
		return num;
	}

	private static int IOControl_internal(SafeSocketHandle safeHandle, int ioctl_code, byte[] input, byte[] output, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			return IOControl_internal(safeHandle.DangerousGetHandle(), ioctl_code, input, output, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern int IOControl_internal(IntPtr sock, int ioctl_code, byte[] input, byte[] output, out int error);

	public void Close()
	{
		linger_timeout = 0;
		Dispose();
	}

	public void Close(int timeout)
	{
		linger_timeout = timeout;
		Dispose();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern void Close_internal(IntPtr socket, out int error);

	public void Shutdown(SocketShutdown how)
	{
		ThrowIfDisposedAndClosed();
		if (!is_connected)
		{
			throw new SocketException(10057);
		}
		Shutdown_internal(m_Handle, how, out var error);
		if (error != 0)
		{
			throw new SocketException(error);
		}
	}

	private static void Shutdown_internal(SafeSocketHandle safeHandle, SocketShutdown how, out int error)
	{
		bool success = false;
		try
		{
			safeHandle.DangerousAddRef(ref success);
			Shutdown_internal(safeHandle.DangerousGetHandle(), how, out error);
		}
		finally
		{
			if (success)
			{
				safeHandle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern void Shutdown_internal(IntPtr socket, SocketShutdown how, out int error);

	protected virtual void Dispose(bool disposing)
	{
		if (CleanedUp)
		{
			return;
		}
		m_IntCleanedUp = 1;
		bool flag = is_connected;
		is_connected = false;
		if (m_Handle != null)
		{
			is_closed = true;
			IntPtr handle = Handle;
			if (flag)
			{
				Linger(handle);
			}
			m_Handle.Dispose();
		}
	}

	private void Linger(IntPtr handle)
	{
		if (!is_connected || linger_timeout <= 0)
		{
			return;
		}
		Shutdown_internal(handle, SocketShutdown.Receive, out var error);
		if (error != 0)
		{
			return;
		}
		int num = linger_timeout / 1000;
		int num2 = linger_timeout % 1000;
		if (num2 > 0)
		{
			Poll_internal(handle, SelectMode.SelectRead, num2 * 1000, out error);
			if (error != 0)
			{
				return;
			}
		}
		if (num > 0)
		{
			LingerOption obj_val = new LingerOption(enable: true, num);
			SetSocketOption_internal(handle, SocketOptionLevel.Socket, SocketOptionName.Linger, obj_val, null, 0, out error);
		}
	}

	private void ThrowIfDisposedAndClosed(Socket socket)
	{
		if (socket.CleanedUp && socket.is_closed)
		{
			throw new ObjectDisposedException(socket.GetType().ToString());
		}
	}

	private void ThrowIfDisposedAndClosed()
	{
		if (CleanedUp && is_closed)
		{
			throw new ObjectDisposedException(GetType().ToString());
		}
	}

	private void ThrowIfBufferNull(byte[] buffer)
	{
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer");
		}
	}

	private void ThrowIfBufferOutOfRange(byte[] buffer, int offset, int size)
	{
		if (offset < 0)
		{
			throw new ArgumentOutOfRangeException("offset", "offset must be >= 0");
		}
		if (offset > buffer.Length)
		{
			throw new ArgumentOutOfRangeException("offset", "offset must be <= buffer.Length");
		}
		if (size < 0)
		{
			throw new ArgumentOutOfRangeException("size", "size must be >= 0");
		}
		if (size > buffer.Length - offset)
		{
			throw new ArgumentOutOfRangeException("size", "size must be <= buffer.Length - offset");
		}
	}

	private void ThrowIfUdp()
	{
		if (protocolType == ProtocolType.Udp)
		{
			throw new SocketException(10042);
		}
	}

	private SocketAsyncResult ValidateEndIAsyncResult(IAsyncResult ares, string methodName, string argName)
	{
		if (ares == null)
		{
			throw new ArgumentNullException(argName);
		}
		SocketAsyncResult obj = (ares as SocketAsyncResult) ?? throw new ArgumentException("Invalid IAsyncResult", argName);
		if (Interlocked.CompareExchange(ref obj.EndCalled, 1, 0) == 1)
		{
			throw new InvalidOperationException(methodName + " can only be called once per asynchronous operation");
		}
		return obj;
	}

	private void QueueIOSelectorJob(SemaphoreSlim sem, IntPtr handle, IOSelectorJob job)
	{
		Task task = sem.WaitAsync();
		if (task.IsCompleted)
		{
			if (CleanedUp)
			{
				job.MarkDisposed();
			}
			else
			{
				IOSelector.Add(handle, job);
			}
			return;
		}
		task.ContinueWith(delegate
		{
			if (CleanedUp)
			{
				job.MarkDisposed();
			}
			else
			{
				IOSelector.Add(handle, job);
			}
		});
	}

	private void InitSocketAsyncEventArgs(SocketAsyncEventArgs e, AsyncCallback callback, object state, SocketOperation operation)
	{
		e.socket_async_result.Init(this, callback, state, operation);
		if (e.AcceptSocket != null)
		{
			e.socket_async_result.AcceptSocket = e.AcceptSocket;
		}
		e.current_socket = this;
		e.SetLastOperation(SocketOperationToSocketAsyncOperation(operation));
		e.SocketError = SocketError.Success;
		e.BytesTransferred = 0;
	}

	private SocketAsyncOperation SocketOperationToSocketAsyncOperation(SocketOperation op)
	{
		switch (op)
		{
		case SocketOperation.Connect:
			return SocketAsyncOperation.Connect;
		case SocketOperation.Accept:
			return SocketAsyncOperation.Accept;
		case SocketOperation.Disconnect:
			return SocketAsyncOperation.Disconnect;
		case SocketOperation.Receive:
		case SocketOperation.ReceiveGeneric:
			return SocketAsyncOperation.Receive;
		case SocketOperation.ReceiveFrom:
			return SocketAsyncOperation.ReceiveFrom;
		case SocketOperation.Send:
		case SocketOperation.SendGeneric:
			return SocketAsyncOperation.Send;
		case SocketOperation.SendTo:
			return SocketAsyncOperation.SendTo;
		default:
			throw new NotImplementedException($"Operation {op} is not implemented");
		}
	}

	private IPEndPoint RemapIPEndPoint(IPEndPoint input)
	{
		if (IsDualMode && input.AddressFamily == AddressFamily.InterNetwork)
		{
			return new IPEndPoint(input.Address.MapToIPv6(), input.Port);
		}
		return input;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern void cancel_blocking_socket_operation(Thread thread);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool SupportsPortReuse(ProtocolType proto);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool IsProtocolSupported_internal(NetworkInterfaceComponent networkInterface);

	private static bool IsProtocolSupported(NetworkInterfaceComponent networkInterface)
	{
		return IsProtocolSupported_internal(networkInterface);
	}
}
