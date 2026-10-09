using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Net;
using System.Net.Security;
using System.Runtime.ExceptionServices;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using System.Threading.Tasks;
using Mono.Security.Interface;

namespace Mono.Net.Security;

internal abstract class MobileAuthenticatedStream : AuthenticatedStream, IMonoSslStream, IDisposable
{
	private enum OperationType
	{
		Read,
		Write,
		Shutdown
	}

	private MobileTlsContext xobileTlsContext;

	private ExceptionDispatchInfo lastException;

	private AsyncProtocolRequest asyncHandshakeRequest;

	private AsyncProtocolRequest asyncReadRequest;

	private AsyncProtocolRequest asyncWriteRequest;

	private BufferOffsetSize2 readBuffer;

	private BufferOffsetSize2 writeBuffer;

	private object ioLock = new object();

	private int closeRequested;

	private bool shutdown;

	private static int uniqueNameInteger = 123;

	private static int nextId;

	internal readonly int ID = ++nextId;

	public SslStream SslStream { get; }

	public MonoTlsSettings Settings { get; }

	public MonoTlsProvider Provider { get; }

	internal bool HasContext => xobileTlsContext != null;

	private SslProtocols DefaultProtocols => SslProtocols.Tls | SslProtocols.Tls11 | SslProtocols.Tls12;

	public AuthenticatedStream AuthenticatedStream => this;

	public override bool IsServer
	{
		get
		{
			CheckThrow(authSuccessCheck: false);
			if (xobileTlsContext != null)
			{
				return xobileTlsContext.IsServer;
			}
			return false;
		}
	}

	public override bool IsAuthenticated
	{
		get
		{
			lock (ioLock)
			{
				return xobileTlsContext != null && lastException == null && xobileTlsContext.IsAuthenticated;
			}
		}
	}

	public override bool IsMutuallyAuthenticated
	{
		get
		{
			lock (ioLock)
			{
				if (!IsAuthenticated)
				{
					return false;
				}
				if ((xobileTlsContext.IsServer ? xobileTlsContext.LocalServerCertificate : xobileTlsContext.LocalClientCertificate) == null)
				{
					return false;
				}
				return xobileTlsContext.IsRemoteCertificateAvailable;
			}
		}
	}

	public SslProtocols SslProtocol
	{
		get
		{
			lock (ioLock)
			{
				CheckThrow(authSuccessCheck: true);
				return (SslProtocols)xobileTlsContext.NegotiatedProtocol;
			}
		}
	}

	public X509Certificate RemoteCertificate
	{
		get
		{
			lock (ioLock)
			{
				CheckThrow(authSuccessCheck: true);
				return xobileTlsContext.RemoteCertificate;
			}
		}
	}

	public X509Certificate LocalCertificate
	{
		get
		{
			lock (ioLock)
			{
				CheckThrow(authSuccessCheck: true);
				return InternalLocalCertificate;
			}
		}
	}

	public X509Certificate InternalLocalCertificate
	{
		get
		{
			lock (ioLock)
			{
				CheckThrow(authSuccessCheck: false);
				if (xobileTlsContext == null)
				{
					return null;
				}
				return xobileTlsContext.IsServer ? xobileTlsContext.LocalServerCertificate : xobileTlsContext.LocalClientCertificate;
			}
		}
	}

	public TransportContext TransportContext
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public override bool CanRead
	{
		get
		{
			if (IsAuthenticated)
			{
				return base.InnerStream.CanRead;
			}
			return false;
		}
	}

	public override bool CanTimeout => base.InnerStream.CanTimeout;

	public override bool CanWrite
	{
		get
		{
			if (IsAuthenticated & base.InnerStream.CanWrite)
			{
				return !shutdown;
			}
			return false;
		}
	}

	public override bool CanSeek => false;

	public override long Length => base.InnerStream.Length;

	public override long Position
	{
		get
		{
			return base.InnerStream.Position;
		}
		set
		{
			throw new NotSupportedException();
		}
	}

	public override bool IsEncrypted => IsAuthenticated;

	public override bool IsSigned => IsAuthenticated;

	public override int ReadTimeout
	{
		get
		{
			return base.InnerStream.ReadTimeout;
		}
		set
		{
			base.InnerStream.ReadTimeout = value;
		}
	}

	public override int WriteTimeout
	{
		get
		{
			return base.InnerStream.WriteTimeout;
		}
		set
		{
			base.InnerStream.WriteTimeout = value;
		}
	}

	public System.Security.Authentication.CipherAlgorithmType CipherAlgorithm
	{
		get
		{
			CheckThrow(authSuccessCheck: true);
			MonoTlsConnectionInfo connectionInfo = GetConnectionInfo();
			if (connectionInfo == null)
			{
				return System.Security.Authentication.CipherAlgorithmType.None;
			}
			switch (connectionInfo.CipherAlgorithmType)
			{
			case Mono.Security.Interface.CipherAlgorithmType.Aes128:
			case Mono.Security.Interface.CipherAlgorithmType.AesGcm128:
				return System.Security.Authentication.CipherAlgorithmType.Aes128;
			case Mono.Security.Interface.CipherAlgorithmType.Aes256:
			case Mono.Security.Interface.CipherAlgorithmType.AesGcm256:
				return System.Security.Authentication.CipherAlgorithmType.Aes256;
			default:
				return System.Security.Authentication.CipherAlgorithmType.None;
			}
		}
	}

	public System.Security.Authentication.HashAlgorithmType HashAlgorithm
	{
		get
		{
			CheckThrow(authSuccessCheck: true);
			MonoTlsConnectionInfo connectionInfo = GetConnectionInfo();
			if (connectionInfo == null)
			{
				return System.Security.Authentication.HashAlgorithmType.None;
			}
			switch (connectionInfo.HashAlgorithmType)
			{
			case Mono.Security.Interface.HashAlgorithmType.Md5:
			case Mono.Security.Interface.HashAlgorithmType.Md5Sha1:
				return System.Security.Authentication.HashAlgorithmType.Md5;
			case Mono.Security.Interface.HashAlgorithmType.Sha1:
			case Mono.Security.Interface.HashAlgorithmType.Sha224:
			case Mono.Security.Interface.HashAlgorithmType.Sha256:
			case Mono.Security.Interface.HashAlgorithmType.Sha384:
			case Mono.Security.Interface.HashAlgorithmType.Sha512:
				return System.Security.Authentication.HashAlgorithmType.Sha1;
			default:
				return System.Security.Authentication.HashAlgorithmType.None;
			}
		}
	}

	public System.Security.Authentication.ExchangeAlgorithmType KeyExchangeAlgorithm
	{
		get
		{
			CheckThrow(authSuccessCheck: true);
			MonoTlsConnectionInfo connectionInfo = GetConnectionInfo();
			if (connectionInfo == null)
			{
				return System.Security.Authentication.ExchangeAlgorithmType.None;
			}
			switch (connectionInfo.ExchangeAlgorithmType)
			{
			case Mono.Security.Interface.ExchangeAlgorithmType.Rsa:
				return System.Security.Authentication.ExchangeAlgorithmType.RsaSign;
			case Mono.Security.Interface.ExchangeAlgorithmType.Dhe:
			case Mono.Security.Interface.ExchangeAlgorithmType.EcDhe:
				return System.Security.Authentication.ExchangeAlgorithmType.DiffieHellman;
			default:
				return System.Security.Authentication.ExchangeAlgorithmType.None;
			}
		}
	}

	public int CipherStrength
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public int HashStrength
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public int KeyExchangeStrength
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public bool CheckCertRevocationStatus
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public MobileAuthenticatedStream(Stream innerStream, bool leaveInnerStreamOpen, SslStream owner, MonoTlsSettings settings, MonoTlsProvider provider)
		: base(innerStream, leaveInnerStreamOpen)
	{
		SslStream = owner;
		Settings = settings;
		Provider = provider;
		readBuffer = new BufferOffsetSize2(16834);
		writeBuffer = new BufferOffsetSize2(16384);
	}

	internal void CheckThrow(bool authSuccessCheck, bool shutdownCheck = false)
	{
		if (lastException != null)
		{
			lastException.Throw();
		}
		if (authSuccessCheck && !IsAuthenticated)
		{
			throw new InvalidOperationException("This operation is only allowed using a successfully authenticated context.");
		}
		if (shutdownCheck && shutdown)
		{
			throw new InvalidOperationException("Write operations are not allowed after the channel was shutdown.");
		}
	}

	internal static Exception GetSSPIException(Exception e)
	{
		if (e is OperationCanceledException || e is IOException || e is ObjectDisposedException || e is AuthenticationException)
		{
			return e;
		}
		return new AuthenticationException("A call to SSPI failed, see inner exception.", e);
	}

	internal static Exception GetIOException(Exception e, string message)
	{
		if (e is OperationCanceledException || e is IOException || e is ObjectDisposedException || e is AuthenticationException)
		{
			return e;
		}
		return new IOException(message, e);
	}

	internal ExceptionDispatchInfo SetException(Exception e)
	{
		ExceptionDispatchInfo exceptionDispatchInfo = ExceptionDispatchInfo.Capture(e);
		return Interlocked.CompareExchange(ref lastException, exceptionDispatchInfo, null) ?? exceptionDispatchInfo;
	}

	public void AuthenticateAsClient(string targetHost)
	{
		AuthenticateAsClient(targetHost, new X509CertificateCollection(), DefaultProtocols, checkCertificateRevocation: false);
	}

	public void AuthenticateAsClient(string targetHost, X509CertificateCollection clientCertificates, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		ProcessAuthentication(runSynchronously: true, serverMode: false, targetHost, enabledSslProtocols, null, clientCertificates, clientCertRequired: false).Wait();
	}

	public IAsyncResult BeginAuthenticateAsClient(string targetHost, AsyncCallback asyncCallback, object asyncState)
	{
		return BeginAuthenticateAsClient(targetHost, new X509CertificateCollection(), DefaultProtocols, checkCertificateRevocation: false, asyncCallback, asyncState);
	}

	public IAsyncResult BeginAuthenticateAsClient(string targetHost, X509CertificateCollection clientCertificates, SslProtocols enabledSslProtocols, bool checkCertificateRevocation, AsyncCallback asyncCallback, object asyncState)
	{
		return TaskToApm.Begin(ProcessAuthentication(runSynchronously: false, serverMode: false, targetHost, enabledSslProtocols, null, clientCertificates, clientCertRequired: false), asyncCallback, asyncState);
	}

	public void EndAuthenticateAsClient(IAsyncResult asyncResult)
	{
		TaskToApm.End(asyncResult);
	}

	public void AuthenticateAsServer(X509Certificate serverCertificate)
	{
		AuthenticateAsServer(serverCertificate, clientCertificateRequired: false, DefaultProtocols, checkCertificateRevocation: false);
	}

	public void AuthenticateAsServer(X509Certificate serverCertificate, bool clientCertificateRequired, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		ProcessAuthentication(runSynchronously: true, serverMode: true, string.Empty, enabledSslProtocols, serverCertificate, null, clientCertificateRequired).Wait();
	}

	public IAsyncResult BeginAuthenticateAsServer(X509Certificate serverCertificate, AsyncCallback asyncCallback, object asyncState)
	{
		return BeginAuthenticateAsServer(serverCertificate, clientCertificateRequired: false, DefaultProtocols, checkCertificateRevocation: false, asyncCallback, asyncState);
	}

	public IAsyncResult BeginAuthenticateAsServer(X509Certificate serverCertificate, bool clientCertificateRequired, SslProtocols enabledSslProtocols, bool checkCertificateRevocation, AsyncCallback asyncCallback, object asyncState)
	{
		return TaskToApm.Begin(ProcessAuthentication(runSynchronously: false, serverMode: true, string.Empty, enabledSslProtocols, serverCertificate, null, clientCertificateRequired), asyncCallback, asyncState);
	}

	public void EndAuthenticateAsServer(IAsyncResult asyncResult)
	{
		TaskToApm.End(asyncResult);
	}

	public Task AuthenticateAsClientAsync(string targetHost)
	{
		return ProcessAuthentication(runSynchronously: false, serverMode: false, targetHost, DefaultProtocols, null, null, clientCertRequired: false);
	}

	public Task AuthenticateAsClientAsync(string targetHost, X509CertificateCollection clientCertificates, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		return ProcessAuthentication(runSynchronously: false, serverMode: false, targetHost, enabledSslProtocols, null, clientCertificates, clientCertRequired: false);
	}

	public Task AuthenticateAsServerAsync(X509Certificate serverCertificate)
	{
		return AuthenticateAsServerAsync(serverCertificate, clientCertificateRequired: false, DefaultProtocols, checkCertificateRevocation: false);
	}

	public Task AuthenticateAsServerAsync(X509Certificate serverCertificate, bool clientCertificateRequired, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		return ProcessAuthentication(runSynchronously: false, serverMode: true, string.Empty, enabledSslProtocols, serverCertificate, null, clientCertificateRequired);
	}

	public Task ShutdownAsync()
	{
		AsyncShutdownRequest asyncRequest = new AsyncShutdownRequest(this);
		return StartOperation(OperationType.Shutdown, asyncRequest, CancellationToken.None);
	}

	private async Task ProcessAuthentication(bool runSynchronously, bool serverMode, string targetHost, SslProtocols enabledProtocols, X509Certificate serverCertificate, X509CertificateCollection clientCertificates, bool clientCertRequired)
	{
		if (serverMode)
		{
			if (serverCertificate == null)
			{
				throw new ArgumentException("serverCertificate");
			}
		}
		else
		{
			if (targetHost == null)
			{
				throw new ArgumentException("targetHost");
			}
			if (targetHost.Length == 0)
			{
				targetHost = "?" + Interlocked.Increment(ref uniqueNameInteger).ToString(NumberFormatInfo.InvariantInfo);
			}
		}
		if (lastException != null)
		{
			lastException.Throw();
		}
		AsyncHandshakeRequest asyncHandshakeRequest = new AsyncHandshakeRequest(this, runSynchronously);
		if (Interlocked.CompareExchange(ref this.asyncHandshakeRequest, asyncHandshakeRequest, null) != null)
		{
			throw new InvalidOperationException("Invalid nested call.");
		}
		if (Interlocked.CompareExchange(ref asyncReadRequest, asyncHandshakeRequest, null) != null)
		{
			throw new InvalidOperationException("Invalid nested call.");
		}
		if (Interlocked.CompareExchange(ref asyncWriteRequest, asyncHandshakeRequest, null) != null)
		{
			throw new InvalidOperationException("Invalid nested call.");
		}
		AsyncProtocolResult asyncProtocolResult;
		try
		{
			lock (ioLock)
			{
				if (xobileTlsContext != null)
				{
					throw new InvalidOperationException();
				}
				readBuffer.Reset();
				writeBuffer.Reset();
				xobileTlsContext = CreateContext(serverMode, targetHost, enabledProtocols, serverCertificate, clientCertificates, clientCertRequired);
			}
			try
			{
				asyncProtocolResult = await asyncHandshakeRequest.StartOperation(CancellationToken.None).ConfigureAwait(continueOnCapturedContext: false);
			}
			catch (Exception e)
			{
				asyncProtocolResult = new AsyncProtocolResult(SetException(GetSSPIException(e)));
			}
		}
		finally
		{
			lock (ioLock)
			{
				readBuffer.Reset();
				writeBuffer.Reset();
				asyncWriteRequest = null;
				asyncReadRequest = null;
				this.asyncHandshakeRequest = null;
			}
		}
		if (asyncProtocolResult.Error != null)
		{
			asyncProtocolResult.Error.Throw();
		}
	}

	protected abstract MobileTlsContext CreateContext(bool serverMode, string targetHost, SslProtocols enabledProtocols, X509Certificate serverCertificate, X509CertificateCollection clientCertificates, bool askForClientCert);

	public override IAsyncResult BeginRead(byte[] buffer, int offset, int count, AsyncCallback asyncCallback, object asyncState)
	{
		AsyncReadRequest asyncRequest = new AsyncReadRequest(this, sync: false, buffer, offset, count);
		return TaskToApm.Begin(StartOperation(OperationType.Read, asyncRequest, CancellationToken.None), asyncCallback, asyncState);
	}

	public override int EndRead(IAsyncResult asyncResult)
	{
		return TaskToApm.End<int>(asyncResult);
	}

	public override IAsyncResult BeginWrite(byte[] buffer, int offset, int count, AsyncCallback asyncCallback, object asyncState)
	{
		AsyncWriteRequest asyncRequest = new AsyncWriteRequest(this, sync: false, buffer, offset, count);
		return TaskToApm.Begin(StartOperation(OperationType.Write, asyncRequest, CancellationToken.None), asyncCallback, asyncState);
	}

	public override void EndWrite(IAsyncResult asyncResult)
	{
		TaskToApm.End(asyncResult);
	}

	public override int Read(byte[] buffer, int offset, int count)
	{
		AsyncReadRequest asyncRequest = new AsyncReadRequest(this, sync: true, buffer, offset, count);
		return StartOperation(OperationType.Read, asyncRequest, CancellationToken.None).Result;
	}

	public void Write(byte[] buffer)
	{
		Write(buffer, 0, buffer.Length);
	}

	public override void Write(byte[] buffer, int offset, int count)
	{
		AsyncWriteRequest asyncRequest = new AsyncWriteRequest(this, sync: true, buffer, offset, count);
		StartOperation(OperationType.Write, asyncRequest, CancellationToken.None).Wait();
	}

	public override Task<int> ReadAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
	{
		AsyncReadRequest asyncRequest = new AsyncReadRequest(this, sync: false, buffer, offset, count);
		return StartOperation(OperationType.Read, asyncRequest, cancellationToken);
	}

	public override Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
	{
		AsyncWriteRequest asyncRequest = new AsyncWriteRequest(this, sync: false, buffer, offset, count);
		return StartOperation(OperationType.Write, asyncRequest, cancellationToken);
	}

	private async Task<int> StartOperation(OperationType type, AsyncProtocolRequest asyncRequest, CancellationToken cancellationToken)
	{
		CheckThrow(authSuccessCheck: true, type != OperationType.Read);
		if (type == OperationType.Read)
		{
			if (Interlocked.CompareExchange(ref asyncReadRequest, asyncRequest, null) != null)
			{
				throw new InvalidOperationException("Invalid nested call.");
			}
		}
		else if (Interlocked.CompareExchange(ref asyncWriteRequest, asyncRequest, null) != null)
		{
			throw new InvalidOperationException("Invalid nested call.");
		}
		AsyncProtocolResult asyncProtocolResult;
		try
		{
			lock (ioLock)
			{
				if (type == OperationType.Read)
				{
					readBuffer.Reset();
				}
				else
				{
					writeBuffer.Reset();
				}
			}
			asyncProtocolResult = await asyncRequest.StartOperation(cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
		}
		catch (Exception e)
		{
			asyncProtocolResult = new AsyncProtocolResult(SetException(GetIOException(e, asyncRequest.Name + " failed")));
		}
		finally
		{
			lock (ioLock)
			{
				if (type == OperationType.Read)
				{
					readBuffer.Reset();
					asyncReadRequest = null;
				}
				else
				{
					writeBuffer.Reset();
					asyncWriteRequest = null;
				}
			}
		}
		if (asyncProtocolResult.Error != null)
		{
			asyncProtocolResult.Error.Throw();
		}
		return asyncProtocolResult.UserResult;
	}

	[Conditional("MONO_TLS_DEBUG")]
	protected internal void Debug(string message, params object[] args)
	{
	}

	internal int InternalRead(byte[] buffer, int offset, int size, out bool outWantMore)
	{
		try
		{
			AsyncProtocolRequest asyncRequest = asyncHandshakeRequest ?? asyncReadRequest;
			(int, bool) tuple = InternalRead(asyncRequest, readBuffer, buffer, offset, size);
			int item = tuple.Item1;
			bool item2 = tuple.Item2;
			outWantMore = item2;
			return item;
		}
		catch (Exception e)
		{
			SetException(GetIOException(e, "InternalRead() failed"));
			outWantMore = false;
			return -1;
		}
	}

	private (int, bool) InternalRead(AsyncProtocolRequest asyncRequest, BufferOffsetSize internalBuffer, byte[] buffer, int offset, int size)
	{
		if (asyncRequest == null)
		{
			throw new InvalidOperationException();
		}
		if (internalBuffer.Size == 0 && !internalBuffer.Complete)
		{
			internalBuffer.Offset = (internalBuffer.Size = 0);
			asyncRequest.RequestRead(size);
			return (0, true);
		}
		int num = System.Math.Min(internalBuffer.Size, size);
		Buffer.BlockCopy(internalBuffer.Buffer, internalBuffer.Offset, buffer, offset, num);
		internalBuffer.Offset += num;
		internalBuffer.Size -= num;
		return (num, !internalBuffer.Complete && num < size);
	}

	internal bool InternalWrite(byte[] buffer, int offset, int size)
	{
		try
		{
			AsyncProtocolRequest asyncRequest = asyncHandshakeRequest ?? asyncWriteRequest;
			return InternalWrite(asyncRequest, writeBuffer, buffer, offset, size);
		}
		catch (Exception e)
		{
			SetException(GetIOException(e, "InternalWrite() failed"));
			return false;
		}
	}

	private bool InternalWrite(AsyncProtocolRequest asyncRequest, BufferOffsetSize2 internalBuffer, byte[] buffer, int offset, int size)
	{
		if (asyncRequest == null)
		{
			if (lastException != null)
			{
				return false;
			}
			if (Interlocked.Exchange(ref closeRequested, 1) == 0)
			{
				internalBuffer.Reset();
			}
			else if (internalBuffer.Remaining == 0)
			{
				throw new InvalidOperationException();
			}
		}
		internalBuffer.AppendData(buffer, offset, size);
		asyncRequest?.RequestWrite();
		return true;
	}

	internal async Task<int> InnerRead(bool sync, int requestedSize, CancellationToken cancellationToken)
	{
		cancellationToken.ThrowIfCancellationRequested();
		int len = System.Math.Min(readBuffer.Remaining, requestedSize);
		if (len == 0)
		{
			throw new InvalidOperationException();
		}
		Task<int> task = ((!sync) ? base.InnerStream.ReadAsync(readBuffer.Buffer, readBuffer.EndOffset, len, cancellationToken) : Task.Run(() => base.InnerStream.Read(readBuffer.Buffer, readBuffer.EndOffset, len)));
		int num = await task.ConfigureAwait(continueOnCapturedContext: false);
		if (num >= 0)
		{
			readBuffer.Size += num;
			readBuffer.TotalBytes += num;
		}
		if (num == 0)
		{
			readBuffer.Complete = true;
			if (readBuffer.TotalBytes > 0)
			{
				num = -1;
			}
		}
		return num;
	}

	internal async Task InnerWrite(bool sync, CancellationToken cancellationToken)
	{
		cancellationToken.ThrowIfCancellationRequested();
		if (writeBuffer.Size != 0)
		{
			Task task = ((!sync) ? base.InnerStream.WriteAsync(writeBuffer.Buffer, writeBuffer.Offset, writeBuffer.Size) : Task.Run(delegate
			{
				base.InnerStream.Write(writeBuffer.Buffer, writeBuffer.Offset, writeBuffer.Size);
			}));
			await task.ConfigureAwait(continueOnCapturedContext: false);
			writeBuffer.TotalBytes += writeBuffer.Size;
			writeBuffer.Offset = (writeBuffer.Size = 0);
		}
	}

	internal AsyncOperationStatus ProcessHandshake(AsyncOperationStatus status)
	{
		lock (ioLock)
		{
			switch (status)
			{
			case AsyncOperationStatus.Initialize:
				xobileTlsContext.StartHandshake();
				return AsyncOperationStatus.Continue;
			case AsyncOperationStatus.ReadDone:
				throw new IOException("Authentication failed because the remote party has closed the transport stream.");
			default:
				throw new InvalidOperationException();
			case AsyncOperationStatus.Continue:
			{
				AsyncOperationStatus result = AsyncOperationStatus.Continue;
				if (xobileTlsContext.ProcessHandshake())
				{
					xobileTlsContext.FinishHandshake();
					result = AsyncOperationStatus.Complete;
				}
				if (lastException != null)
				{
					lastException.Throw();
				}
				return result;
			}
			}
		}
	}

	internal (int, bool) ProcessRead(BufferOffsetSize userBuffer)
	{
		lock (ioLock)
		{
			(int ret, bool wantMore) result = xobileTlsContext.Read(userBuffer.Buffer, userBuffer.Offset, userBuffer.Size);
			if (lastException != null)
			{
				lastException.Throw();
			}
			return result;
		}
	}

	internal (int, bool) ProcessWrite(BufferOffsetSize userBuffer)
	{
		lock (ioLock)
		{
			(int ret, bool wantMore) result = xobileTlsContext.Write(userBuffer.Buffer, userBuffer.Offset, userBuffer.Size);
			if (lastException != null)
			{
				lastException.Throw();
			}
			return result;
		}
	}

	internal AsyncOperationStatus ProcessShutdown(AsyncOperationStatus status)
	{
		lock (ioLock)
		{
			xobileTlsContext.Shutdown();
			shutdown = true;
			return AsyncOperationStatus.Complete;
		}
	}

	protected override void Dispose(bool disposing)
	{
		try
		{
			lock (ioLock)
			{
				lastException = ExceptionDispatchInfo.Capture(new ObjectDisposedException("MobileAuthenticatedStream"));
				if (xobileTlsContext != null)
				{
					xobileTlsContext.Dispose();
					xobileTlsContext = null;
				}
			}
		}
		finally
		{
			base.Dispose(disposing);
		}
	}

	public override void Flush()
	{
		base.InnerStream.Flush();
	}

	public MonoTlsConnectionInfo GetConnectionInfo()
	{
		lock (ioLock)
		{
			CheckThrow(authSuccessCheck: true);
			return xobileTlsContext.ConnectionInfo;
		}
	}

	public override long Seek(long offset, SeekOrigin origin)
	{
		throw new NotSupportedException();
	}

	public override void SetLength(long value)
	{
		base.InnerStream.SetLength(value);
	}
}
