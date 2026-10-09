using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Net;
using System.Net.Security;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using System.Threading.Tasks;

namespace System.Data.SqlClient.SNI;

internal class SNITCPHandle : SNIHandle
{
	private readonly string _targetServer;

	private readonly object _callbackObject;

	private readonly Socket _socket;

	private NetworkStream _tcpStream;

	private readonly TaskScheduler _writeScheduler;

	private readonly TaskFactory _writeTaskFactory;

	private Stream _stream;

	private SslStream _sslStream;

	private SslOverTdsStream _sslOverTdsStream;

	private SNIAsyncCallback _receiveCallback;

	private SNIAsyncCallback _sendCallback;

	private bool _validateCert = true;

	private int _bufferSize = 4096;

	private uint _status = uint.MaxValue;

	private Guid _connectionId = Guid.NewGuid();

	private const int MaxParallelIpAddresses = 64;

	public override Guid ConnectionId => _connectionId;

	public override uint Status => _status;

	public override void Dispose()
	{
		lock (this)
		{
			if (_sslOverTdsStream != null)
			{
				_sslOverTdsStream.Dispose();
				_sslOverTdsStream = null;
			}
			if (_sslStream != null)
			{
				_sslStream.Dispose();
				_sslStream = null;
			}
			if (_tcpStream != null)
			{
				_tcpStream.Dispose();
				_tcpStream = null;
			}
			_stream = null;
		}
	}

	public SNITCPHandle(string serverName, int port, long timerExpire, object callbackObject, bool parallel)
	{
		_writeScheduler = new ConcurrentExclusiveSchedulerPair().ExclusiveScheduler;
		_writeTaskFactory = new TaskFactory(_writeScheduler);
		_callbackObject = callbackObject;
		_targetServer = serverName;
		try
		{
			TimeSpan timeout = default(TimeSpan);
			bool flag = long.MaxValue == timerExpire;
			if (!flag)
			{
				timeout = DateTime.FromFileTime(timerExpire) - DateTime.Now;
				timeout = ((timeout.Ticks < 0) ? TimeSpan.FromTicks(0L) : timeout);
			}
			Task<Socket> task;
			if (parallel)
			{
				Task<IPAddress[]> hostAddressesAsync = Dns.GetHostAddressesAsync(serverName);
				hostAddressesAsync.Wait(timeout);
				IPAddress[] result = hostAddressesAsync.Result;
				if (result.Length > 64)
				{
					ReportTcpSNIError(0u, 47u, string.Empty);
					return;
				}
				task = ParallelConnectAsync(result, port);
			}
			else
			{
				task = ConnectAsync(serverName, port);
			}
			if (!(flag ? task.Wait(-1) : task.Wait(timeout)))
			{
				ReportTcpSNIError(0u, 40u, string.Empty);
				return;
			}
			_socket = task.Result;
			if (_socket == null || !_socket.Connected)
			{
				if (_socket != null)
				{
					_socket.Dispose();
					_socket = null;
				}
				ReportTcpSNIError(0u, 40u, string.Empty);
				return;
			}
			_socket.NoDelay = true;
			_tcpStream = new NetworkStream(_socket, ownsSocket: true);
			_sslOverTdsStream = new SslOverTdsStream(_tcpStream);
			_sslStream = new SslStream(_sslOverTdsStream, leaveInnerStreamOpen: true, ValidateServerCertificate, null);
		}
		catch (SocketException sniException)
		{
			ReportTcpSNIError(sniException);
			return;
		}
		catch (Exception sniException2)
		{
			ReportTcpSNIError(sniException2);
			return;
		}
		_stream = _tcpStream;
		_status = 0u;
	}

	private static async Task<Socket> ConnectAsync(string serverName, int port)
	{
		IPAddress[] array = await Dns.GetHostAddressesAsync(serverName).ConfigureAwait(continueOnCapturedContext: false);
		IPAddress targetAddrV4 = Array.Find(array, (IPAddress addr) => addr.AddressFamily == AddressFamily.InterNetwork);
		IPAddress targetAddrV5 = Array.Find(array, (IPAddress addr) => addr.AddressFamily == AddressFamily.InterNetworkV6);
		if (targetAddrV4 != null && targetAddrV5 != null)
		{
			return await ParallelConnectAsync(new IPAddress[2] { targetAddrV4, targetAddrV5 }, port).ConfigureAwait(continueOnCapturedContext: false);
		}
		IPAddress iPAddress = ((targetAddrV4 != null) ? targetAddrV4 : targetAddrV5);
		Socket socket = new Socket(iPAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
		try
		{
			await socket.ConnectAsync(iPAddress, port).ConfigureAwait(continueOnCapturedContext: false);
		}
		catch
		{
			socket.Dispose();
			throw;
		}
		return socket;
	}

	private static Task<Socket> ParallelConnectAsync(IPAddress[] serverAddresses, int port)
	{
		if (serverAddresses == null)
		{
			throw new ArgumentNullException("serverAddresses");
		}
		if (serverAddresses.Length == 0)
		{
			throw new ArgumentOutOfRangeException("serverAddresses");
		}
		List<Socket> list = new List<Socket>(serverAddresses.Length);
		List<Task> list2 = new List<Task>(serverAddresses.Length);
		TaskCompletionSource<Socket> taskCompletionSource = new TaskCompletionSource<Socket>();
		StrongBox<Exception> lastError = new StrongBox<Exception>();
		StrongBox<int> pendingCompleteCount = new StrongBox<int>(serverAddresses.Length);
		foreach (IPAddress iPAddress in serverAddresses)
		{
			Socket socket = new Socket(iPAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
			list.Add(socket);
			try
			{
				list2.Add(socket.ConnectAsync(iPAddress, port));
			}
			catch (Exception exception)
			{
				list2.Add(Task.FromException(exception));
			}
		}
		for (int j = 0; j < list.Count; j++)
		{
			ParallelConnectHelper(list[j], list2[j], taskCompletionSource, pendingCompleteCount, lastError, list);
		}
		return taskCompletionSource.Task;
	}

	private static async void ParallelConnectHelper(Socket socket, Task connectTask, TaskCompletionSource<Socket> tcs, StrongBox<int> pendingCompleteCount, StrongBox<Exception> lastError, List<Socket> sockets)
	{
		bool success = false;
		try
		{
			await connectTask.ConfigureAwait(continueOnCapturedContext: false);
			success = tcs.TrySetResult(socket);
			if (!success)
			{
				return;
			}
			foreach (Socket socket2 in sockets)
			{
				if (socket2 != socket)
				{
					socket2.Dispose();
				}
			}
		}
		catch (Exception value)
		{
			Interlocked.Exchange(ref lastError.Value, value);
		}
		finally
		{
			if (!success && Interlocked.Decrement(ref pendingCompleteCount.Value) == 0)
			{
				if (lastError.Value != null)
				{
					tcs.TrySetException(lastError.Value);
				}
				else
				{
					tcs.TrySetCanceled();
				}
				foreach (Socket socket3 in sockets)
				{
					socket3.Dispose();
				}
			}
		}
	}

	public override uint EnableSsl(uint options)
	{
		_validateCert = (options & 1) != 0;
		try
		{
			_sslStream.AuthenticateAsClientAsync(_targetServer).GetAwaiter().GetResult();
			_sslOverTdsStream.FinishHandshake();
		}
		catch (AuthenticationException sniException)
		{
			return ReportTcpSNIError(sniException);
		}
		catch (InvalidOperationException sniException2)
		{
			return ReportTcpSNIError(sniException2);
		}
		_stream = _sslStream;
		return 0u;
	}

	public override void DisableSsl()
	{
		if (Environment.OSVersion.Platform != PlatformID.Win32NT)
		{
			_sslStream.Dispose();
		}
		_sslStream = null;
		_sslOverTdsStream.Dispose();
		_sslOverTdsStream = null;
		_stream = _tcpStream;
	}

	private bool ValidateServerCertificate(object sender, X509Certificate cert, X509Chain chain, SslPolicyErrors policyErrors)
	{
		if (!_validateCert)
		{
			return true;
		}
		return SNICommon.ValidateSslServerCertificate(_targetServer, sender, cert, chain, policyErrors);
	}

	public override void SetBufferSize(int bufferSize)
	{
		_bufferSize = bufferSize;
		_socket.SendBufferSize = bufferSize;
		_socket.ReceiveBufferSize = bufferSize;
	}

	public override uint Send(SNIPacket packet)
	{
		lock (this)
		{
			try
			{
				packet.WriteToStream(_stream);
				return 0u;
			}
			catch (ObjectDisposedException sniException)
			{
				return ReportTcpSNIError(sniException);
			}
			catch (SocketException sniException2)
			{
				return ReportTcpSNIError(sniException2);
			}
			catch (IOException sniException3)
			{
				return ReportTcpSNIError(sniException3);
			}
		}
	}

	public override uint Receive(out SNIPacket packet, int timeoutInMilliseconds)
	{
		lock (this)
		{
			packet = null;
			try
			{
				if (timeoutInMilliseconds > 0)
				{
					_socket.ReceiveTimeout = timeoutInMilliseconds;
				}
				else
				{
					if (timeoutInMilliseconds != -1)
					{
						ReportTcpSNIError(0u, 11u, string.Empty);
						return 258u;
					}
					_socket.ReceiveTimeout = 0;
				}
				packet = new SNIPacket(null);
				packet.Allocate(_bufferSize);
				packet.ReadFromStream(_stream);
				if (packet.Length == 0)
				{
					Win32Exception ex = new Win32Exception();
					return ReportErrorAndReleasePacket(packet, (uint)ex.NativeErrorCode, 0u, ex.Message);
				}
				return 0u;
			}
			catch (ObjectDisposedException sniException)
			{
				return ReportErrorAndReleasePacket(packet, sniException);
			}
			catch (SocketException sniException2)
			{
				return ReportErrorAndReleasePacket(packet, sniException2);
			}
			catch (IOException ex2)
			{
				uint result = ReportErrorAndReleasePacket(packet, ex2);
				if (ex2.InnerException is SocketException && ((SocketException)ex2.InnerException).SocketErrorCode == SocketError.TimedOut)
				{
					result = 258u;
				}
				return result;
			}
			finally
			{
				_socket.ReceiveTimeout = 0;
			}
		}
	}

	public override void SetAsyncCallbacks(SNIAsyncCallback receiveCallback, SNIAsyncCallback sendCallback)
	{
		_receiveCallback = receiveCallback;
		_sendCallback = sendCallback;
	}

	public override uint SendAsync(SNIPacket packet, SNIAsyncCallback callback = null)
	{
		_ = packet;
		_writeTaskFactory.StartNew(delegate
		{
			try
			{
				lock (this)
				{
					packet.WriteToStream(_stream);
				}
			}
			catch (Exception sniException)
			{
				SNILoadHandle.SingletonInstance.LastError = new SNIError(SNIProviders.TCP_PROV, 35u, sniException);
				if (callback != null)
				{
					callback(packet, 1u);
				}
				else
				{
					_sendCallback(packet, 1u);
				}
				return;
			}
			if (callback != null)
			{
				callback(packet, 0u);
			}
			else
			{
				_sendCallback(packet, 0u);
			}
		});
		return 997u;
	}

	public override uint ReceiveAsync(ref SNIPacket packet)
	{
		lock (this)
		{
			packet = new SNIPacket(null);
			packet.Allocate(_bufferSize);
			try
			{
				packet.ReadFromStreamAsync(_stream, _receiveCallback);
				return 997u;
			}
			catch (ObjectDisposedException sniException)
			{
				return ReportErrorAndReleasePacket(packet, sniException);
			}
			catch (SocketException sniException2)
			{
				return ReportErrorAndReleasePacket(packet, sniException2);
			}
			catch (IOException sniException3)
			{
				return ReportErrorAndReleasePacket(packet, sniException3);
			}
		}
	}

	public override uint CheckConnection()
	{
		try
		{
			if (!_socket.Connected || _socket.Poll(0, SelectMode.SelectError))
			{
				return 1u;
			}
		}
		catch (SocketException sniException)
		{
			return ReportTcpSNIError(sniException);
		}
		catch (ObjectDisposedException sniException2)
		{
			return ReportTcpSNIError(sniException2);
		}
		return 0u;
	}

	private uint ReportTcpSNIError(Exception sniException)
	{
		_status = 1u;
		return SNICommon.ReportSNIError(SNIProviders.TCP_PROV, 35u, sniException);
	}

	private uint ReportTcpSNIError(uint nativeError, uint sniError, string errorMessage)
	{
		_status = 1u;
		return SNICommon.ReportSNIError(SNIProviders.TCP_PROV, nativeError, sniError, errorMessage);
	}

	private uint ReportErrorAndReleasePacket(SNIPacket packet, Exception sniException)
	{
		packet?.Release();
		return ReportTcpSNIError(sniException);
	}

	private uint ReportErrorAndReleasePacket(SNIPacket packet, uint nativeError, uint sniError, string errorMessage)
	{
		packet?.Release();
		return ReportTcpSNIError(nativeError, sniError, errorMessage);
	}
}
