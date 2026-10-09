using System;
using System.Diagnostics;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using Mono.Security.Interface;

namespace Mono.Net.Security;

internal abstract class MobileTlsContext : IDisposable
{
	private MobileAuthenticatedStream parent;

	private bool serverMode;

	private string targetHost;

	private string serverName;

	private SslProtocols enabledProtocols;

	private X509Certificate serverCertificate;

	private X509CertificateCollection clientCertificates;

	private bool askForClientCert;

	private ICertificateValidator2 certificateValidator;

	internal MobileAuthenticatedStream Parent => parent;

	public MonoTlsSettings Settings => parent.Settings;

	public MonoTlsProvider Provider => parent.Provider;

	public abstract bool HasContext { get; }

	public abstract bool IsAuthenticated { get; }

	public bool IsServer => serverMode;

	protected string TargetHost => targetHost;

	protected string ServerName => serverName;

	protected bool AskForClientCertificate => askForClientCert;

	protected SslProtocols EnabledProtocols => enabledProtocols;

	protected X509CertificateCollection ClientCertificates => clientCertificates;

	public abstract MonoTlsConnectionInfo ConnectionInfo { get; }

	internal X509Certificate LocalServerCertificate => serverCertificate;

	internal abstract bool IsRemoteCertificateAvailable { get; }

	internal abstract X509Certificate LocalClientCertificate { get; }

	public abstract X509Certificate RemoteCertificate { get; }

	public abstract TlsProtocols NegotiatedProtocol { get; }

	public MobileTlsContext(MobileAuthenticatedStream parent, bool serverMode, string targetHost, SslProtocols enabledProtocols, X509Certificate serverCertificate, X509CertificateCollection clientCertificates, bool askForClientCert)
	{
		this.parent = parent;
		this.serverMode = serverMode;
		this.targetHost = targetHost;
		this.enabledProtocols = enabledProtocols;
		this.serverCertificate = serverCertificate;
		this.clientCertificates = clientCertificates;
		this.askForClientCert = askForClientCert;
		serverName = targetHost;
		if (!string.IsNullOrEmpty(serverName))
		{
			int num = serverName.IndexOf(':');
			if (num > 0)
			{
				serverName = serverName.Substring(0, num);
			}
		}
		certificateValidator = CertificateValidationHelper.GetInternalValidator(parent.Settings, parent.Provider);
	}

	[Conditional("MONO_TLS_DEBUG")]
	protected void Debug(string message, params object[] args)
	{
	}

	protected void GetProtocolVersions(out TlsProtocolCode min, out TlsProtocolCode max)
	{
		if ((enabledProtocols & SslProtocols.Tls) != 0)
		{
			min = TlsProtocolCode.Tls10;
		}
		else if ((enabledProtocols & SslProtocols.Tls11) != 0)
		{
			min = TlsProtocolCode.Tls11;
		}
		else
		{
			min = TlsProtocolCode.Tls12;
		}
		if ((enabledProtocols & SslProtocols.Tls12) != 0)
		{
			max = TlsProtocolCode.Tls12;
		}
		else if ((enabledProtocols & SslProtocols.Tls11) != 0)
		{
			max = TlsProtocolCode.Tls11;
		}
		else
		{
			max = TlsProtocolCode.Tls10;
		}
	}

	public abstract void StartHandshake();

	public abstract bool ProcessHandshake();

	public abstract void FinishHandshake();

	public abstract void Flush();

	public abstract (int ret, bool wantMore) Read(byte[] buffer, int offset, int count);

	public abstract (int ret, bool wantMore) Write(byte[] buffer, int offset, int count);

	public abstract void Shutdown();

	protected bool ValidateCertificate(X509Certificate leaf, X509Chain chain)
	{
		ValidationResult validationResult = certificateValidator.ValidateCertificate(TargetHost, IsServer, leaf, chain);
		if (validationResult != null && validationResult.Trusted)
		{
			return !validationResult.UserDenied;
		}
		return false;
	}

	protected bool ValidateCertificate(X509CertificateCollection certificates)
	{
		ValidationResult validationResult = certificateValidator.ValidateCertificate(TargetHost, IsServer, certificates);
		if (validationResult != null && validationResult.Trusted)
		{
			return !validationResult.UserDenied;
		}
		return false;
	}

	protected X509Certificate SelectClientCertificate(X509Certificate serverCertificate, string[] acceptableIssuers)
	{
		if (certificateValidator.SelectClientCertificate(TargetHost, ClientCertificates, serverCertificate, acceptableIssuers, out var clientCertificate))
		{
			return clientCertificate;
		}
		if (clientCertificates == null || clientCertificates.Count == 0)
		{
			return null;
		}
		if (clientCertificates.Count == 1)
		{
			return clientCertificates[0];
		}
		throw new NotImplementedException();
	}

	public void Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	protected virtual void Dispose(bool disposing)
	{
	}

	~MobileTlsContext()
	{
		Dispose(disposing: false);
	}
}
