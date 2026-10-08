using System.IO;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Security.Permissions;
using System.Threading;
using System.Threading.Tasks;
using Mono.Net.Security.Private;
using Mono.Security.Interface;
using Unity;

namespace System.Net.Security;

public class SslStream : AuthenticatedStream
{
	private MonoTlsProvider provider;

	private IMonoSslStream impl;

	internal IMonoSslStream Impl
	{
		get
		{
			CheckDisposed();
			return impl;
		}
	}

	internal MonoTlsProvider Provider
	{
		get
		{
			CheckDisposed();
			return provider;
		}
	}

	public TransportContext TransportContext
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public override bool IsAuthenticated => Impl.IsAuthenticated;

	public override bool IsMutuallyAuthenticated => Impl.IsMutuallyAuthenticated;

	public override bool IsEncrypted => Impl.IsEncrypted;

	public override bool IsSigned => Impl.IsSigned;

	public override bool IsServer => Impl.IsServer;

	public virtual SslProtocols SslProtocol => Impl.SslProtocol;

	public virtual bool CheckCertRevocationStatus => Impl.CheckCertRevocationStatus;

	public virtual X509Certificate LocalCertificate => Impl.LocalCertificate;

	public virtual X509Certificate RemoteCertificate => Impl.RemoteCertificate;

	public virtual System.Security.Authentication.CipherAlgorithmType CipherAlgorithm => Impl.CipherAlgorithm;

	public virtual int CipherStrength => Impl.CipherStrength;

	public virtual System.Security.Authentication.HashAlgorithmType HashAlgorithm => Impl.HashAlgorithm;

	public virtual int HashStrength => Impl.HashStrength;

	public virtual System.Security.Authentication.ExchangeAlgorithmType KeyExchangeAlgorithm => Impl.KeyExchangeAlgorithm;

	public virtual int KeyExchangeStrength => Impl.KeyExchangeStrength;

	public override bool CanSeek => false;

	public override bool CanRead
	{
		get
		{
			if (impl != null)
			{
				return impl.CanRead;
			}
			return false;
		}
	}

	public override bool CanTimeout => base.InnerStream.CanTimeout;

	public override bool CanWrite
	{
		get
		{
			if (impl != null)
			{
				return impl.CanWrite;
			}
			return false;
		}
	}

	public override int ReadTimeout
	{
		get
		{
			return Impl.ReadTimeout;
		}
		set
		{
			Impl.ReadTimeout = value;
		}
	}

	public override int WriteTimeout
	{
		get
		{
			return Impl.WriteTimeout;
		}
		set
		{
			Impl.WriteTimeout = value;
		}
	}

	public override long Length => Impl.Length;

	public override long Position
	{
		get
		{
			return Impl.Position;
		}
		set
		{
			throw new NotSupportedException(global::SR.GetString("This stream does not support seek operations."));
		}
	}

	private static MonoTlsProvider GetProvider()
	{
		return MonoTlsProviderFactory.GetProvider();
	}

	public SslStream(Stream innerStream)
		: this(innerStream, leaveInnerStreamOpen: false)
	{
	}

	public SslStream(Stream innerStream, bool leaveInnerStreamOpen)
		: base(innerStream, leaveInnerStreamOpen)
	{
		provider = GetProvider();
		impl = provider.CreateSslStreamInternal(this, innerStream, leaveInnerStreamOpen, null);
	}

	public SslStream(Stream innerStream, bool leaveInnerStreamOpen, RemoteCertificateValidationCallback userCertificateValidationCallback)
		: this(innerStream, leaveInnerStreamOpen, userCertificateValidationCallback, null)
	{
	}

	public SslStream(Stream innerStream, bool leaveInnerStreamOpen, RemoteCertificateValidationCallback userCertificateValidationCallback, LocalCertificateSelectionCallback userCertificateSelectionCallback)
		: base(innerStream, leaveInnerStreamOpen)
	{
		provider = GetProvider();
		MonoTlsSettings monoTlsSettings = MonoTlsSettings.CopyDefaultSettings();
		monoTlsSettings.RemoteCertificateValidationCallback = CallbackHelpers.PublicToMono(userCertificateValidationCallback);
		monoTlsSettings.ClientCertificateSelectionCallback = CallbackHelpers.PublicToMono(userCertificateSelectionCallback);
		impl = provider.CreateSslStream(innerStream, leaveInnerStreamOpen, monoTlsSettings);
	}

	[System.MonoLimitation("encryptionPolicy is ignored")]
	public SslStream(Stream innerStream, bool leaveInnerStreamOpen, RemoteCertificateValidationCallback userCertificateValidationCallback, LocalCertificateSelectionCallback userCertificateSelectionCallback, EncryptionPolicy encryptionPolicy)
		: this(innerStream, leaveInnerStreamOpen, userCertificateValidationCallback, userCertificateSelectionCallback)
	{
	}

	internal SslStream(Stream innerStream, bool leaveInnerStreamOpen, MonoTlsProvider provider, MonoTlsSettings settings)
		: base(innerStream, leaveInnerStreamOpen)
	{
		this.provider = provider;
		impl = provider.CreateSslStreamInternal(this, innerStream, leaveInnerStreamOpen, settings);
	}

	internal static IMonoSslStream CreateMonoSslStream(Stream innerStream, bool leaveInnerStreamOpen, MonoTlsProvider provider, MonoTlsSettings settings)
	{
		return new SslStream(innerStream, leaveInnerStreamOpen, provider, settings).Impl;
	}

	public virtual void AuthenticateAsClient(string targetHost)
	{
		Impl.AuthenticateAsClient(targetHost);
	}

	public virtual void AuthenticateAsClient(string targetHost, X509CertificateCollection clientCertificates, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		Impl.AuthenticateAsClient(targetHost, clientCertificates, enabledSslProtocols, checkCertificateRevocation);
	}

	public virtual IAsyncResult BeginAuthenticateAsClient(string targetHost, AsyncCallback asyncCallback, object asyncState)
	{
		return Impl.BeginAuthenticateAsClient(targetHost, asyncCallback, asyncState);
	}

	public virtual IAsyncResult BeginAuthenticateAsClient(string targetHost, X509CertificateCollection clientCertificates, SslProtocols enabledSslProtocols, bool checkCertificateRevocation, AsyncCallback asyncCallback, object asyncState)
	{
		return Impl.BeginAuthenticateAsClient(targetHost, clientCertificates, enabledSslProtocols, checkCertificateRevocation, asyncCallback, asyncState);
	}

	public virtual void EndAuthenticateAsClient(IAsyncResult asyncResult)
	{
		Impl.EndAuthenticateAsClient(asyncResult);
	}

	public virtual void AuthenticateAsServer(X509Certificate serverCertificate)
	{
		Impl.AuthenticateAsServer(serverCertificate);
	}

	public virtual void AuthenticateAsServer(X509Certificate serverCertificate, bool clientCertificateRequired, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		Impl.AuthenticateAsServer(serverCertificate, clientCertificateRequired, enabledSslProtocols, checkCertificateRevocation);
	}

	public virtual IAsyncResult BeginAuthenticateAsServer(X509Certificate serverCertificate, AsyncCallback asyncCallback, object asyncState)
	{
		return Impl.BeginAuthenticateAsServer(serverCertificate, asyncCallback, asyncState);
	}

	public virtual IAsyncResult BeginAuthenticateAsServer(X509Certificate serverCertificate, bool clientCertificateRequired, SslProtocols enabledSslProtocols, bool checkCertificateRevocation, AsyncCallback asyncCallback, object asyncState)
	{
		return Impl.BeginAuthenticateAsServer(serverCertificate, clientCertificateRequired, enabledSslProtocols, checkCertificateRevocation, asyncCallback, asyncState);
	}

	public virtual void EndAuthenticateAsServer(IAsyncResult asyncResult)
	{
		Impl.EndAuthenticateAsServer(asyncResult);
	}

	public virtual Task AuthenticateAsClientAsync(string targetHost)
	{
		return Impl.AuthenticateAsClientAsync(targetHost);
	}

	public virtual Task AuthenticateAsClientAsync(string targetHost, X509CertificateCollection clientCertificates, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		return Impl.AuthenticateAsClientAsync(targetHost, clientCertificates, enabledSslProtocols, checkCertificateRevocation);
	}

	public virtual Task AuthenticateAsServerAsync(X509Certificate serverCertificate)
	{
		return Impl.AuthenticateAsServerAsync(serverCertificate);
	}

	public virtual Task AuthenticateAsServerAsync(X509Certificate serverCertificate, bool clientCertificateRequired, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		return Impl.AuthenticateAsServerAsync(serverCertificate, clientCertificateRequired, enabledSslProtocols, checkCertificateRevocation);
	}

	public virtual Task ShutdownAsync()
	{
		return Impl.ShutdownAsync();
	}

	public override void SetLength(long value)
	{
		Impl.SetLength(value);
	}

	public override long Seek(long offset, SeekOrigin origin)
	{
		throw new NotSupportedException(global::SR.GetString("This stream does not support seek operations."));
	}

	public override Task FlushAsync(CancellationToken cancellationToken)
	{
		return base.InnerStream.FlushAsync(cancellationToken);
	}

	public override void Flush()
	{
		base.InnerStream.Flush();
	}

	private void CheckDisposed()
	{
		if (impl == null)
		{
			throw new ObjectDisposedException("SslStream");
		}
	}

	protected override void Dispose(bool disposing)
	{
		try
		{
			if (impl != null && disposing)
			{
				impl.Dispose();
				impl = null;
			}
		}
		finally
		{
			base.Dispose(disposing);
		}
	}

	public override int Read(byte[] buffer, int offset, int count)
	{
		return Impl.Read(buffer, offset, count);
	}

	public void Write(byte[] buffer)
	{
		Impl.Write(buffer);
	}

	public override void Write(byte[] buffer, int offset, int count)
	{
		Impl.Write(buffer, offset, count);
	}

	public override IAsyncResult BeginRead(byte[] buffer, int offset, int count, AsyncCallback asyncCallback, object asyncState)
	{
		return Impl.BeginRead(buffer, offset, count, asyncCallback, asyncState);
	}

	public override int EndRead(IAsyncResult asyncResult)
	{
		return Impl.EndRead(asyncResult);
	}

	public override IAsyncResult BeginWrite(byte[] buffer, int offset, int count, AsyncCallback asyncCallback, object asyncState)
	{
		return Impl.BeginWrite(buffer, offset, count, asyncCallback, asyncState);
	}

	public override void EndWrite(IAsyncResult asyncResult)
	{
		Impl.EndWrite(asyncResult);
	}

	public virtual void AuthenticateAsClient(string targetHost, X509CertificateCollection clientCertificates, bool checkCertificateRevocation)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public virtual Task AuthenticateAsClientAsync(string targetHost, X509CertificateCollection clientCertificates, bool checkCertificateRevocation)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual void AuthenticateAsServer(X509Certificate serverCertificate, bool clientCertificateRequired, bool checkCertificateRevocation)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public virtual Task AuthenticateAsServerAsync(X509Certificate serverCertificate, bool clientCertificateRequired, bool checkCertificateRevocation)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public virtual IAsyncResult BeginAuthenticateAsClient(string targetHost, X509CertificateCollection clientCertificates, bool checkCertificateRevocation, AsyncCallback asyncCallback, object asyncState)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public virtual IAsyncResult BeginAuthenticateAsServer(X509Certificate serverCertificate, bool clientCertificateRequired, bool checkCertificateRevocation, AsyncCallback asyncCallback, object asyncState)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
