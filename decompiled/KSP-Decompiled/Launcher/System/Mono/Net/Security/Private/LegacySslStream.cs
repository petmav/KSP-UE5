using System;
using System.IO;
using System.Net;
using System.Net.Security;
using System.Security.Authentication;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Threading.Tasks;
using Mono.Security.Interface;
using Mono.Security.Protocol.Tls;
using Mono.Security.X509;

namespace Mono.Net.Security.Private;

[System.MonoTODO("Non-X509Certificate2 certificate is not supported")]
internal class LegacySslStream : AuthenticatedStream, IMonoSslStream, IDisposable
{
	private SslStreamBase ssl_stream;

	private ICertificateValidator certificateValidator;

	public override bool CanRead => base.InnerStream.CanRead;

	public override bool CanSeek => base.InnerStream.CanSeek;

	public override bool CanTimeout => base.InnerStream.CanTimeout;

	public override bool CanWrite => base.InnerStream.CanWrite;

	public override long Length => base.InnerStream.Length;

	public override long Position
	{
		get
		{
			return base.InnerStream.Position;
		}
		set
		{
			throw new NotSupportedException("This stream does not support seek operations");
		}
	}

	public override bool IsAuthenticated => ssl_stream != null;

	public override bool IsEncrypted => IsAuthenticated;

	public override bool IsMutuallyAuthenticated
	{
		get
		{
			if (IsAuthenticated)
			{
				if (!IsServer)
				{
					return LocalCertificate != null;
				}
				return RemoteCertificate != null;
			}
			return false;
		}
	}

	public override bool IsServer => ssl_stream is SslServerStream;

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

	public virtual bool CheckCertRevocationStatus
	{
		get
		{
			if (!IsAuthenticated)
			{
				return false;
			}
			return ssl_stream.CheckCertRevocationStatus;
		}
	}

	public virtual System.Security.Authentication.CipherAlgorithmType CipherAlgorithm
	{
		get
		{
			CheckConnectionAuthenticated();
			switch (ssl_stream.CipherAlgorithm)
			{
			case Mono.Security.Protocol.Tls.CipherAlgorithmType.Des:
				return System.Security.Authentication.CipherAlgorithmType.Des;
			case Mono.Security.Protocol.Tls.CipherAlgorithmType.None:
				return System.Security.Authentication.CipherAlgorithmType.None;
			case Mono.Security.Protocol.Tls.CipherAlgorithmType.Rc2:
				return System.Security.Authentication.CipherAlgorithmType.Rc2;
			case Mono.Security.Protocol.Tls.CipherAlgorithmType.Rc4:
				return System.Security.Authentication.CipherAlgorithmType.Rc4;
			case Mono.Security.Protocol.Tls.CipherAlgorithmType.TripleDes:
				return System.Security.Authentication.CipherAlgorithmType.TripleDes;
			case Mono.Security.Protocol.Tls.CipherAlgorithmType.Rijndael:
				switch (ssl_stream.CipherStrength)
				{
				case 128:
					return System.Security.Authentication.CipherAlgorithmType.Aes128;
				case 192:
					return System.Security.Authentication.CipherAlgorithmType.Aes192;
				case 256:
					return System.Security.Authentication.CipherAlgorithmType.Aes256;
				}
				break;
			}
			throw new InvalidOperationException("Not supported cipher algorithm is in use. It is likely a bug in SslStream.");
		}
	}

	public virtual int CipherStrength
	{
		get
		{
			CheckConnectionAuthenticated();
			return ssl_stream.CipherStrength;
		}
	}

	public virtual System.Security.Authentication.HashAlgorithmType HashAlgorithm
	{
		get
		{
			CheckConnectionAuthenticated();
			return ssl_stream.HashAlgorithm switch
			{
				Mono.Security.Protocol.Tls.HashAlgorithmType.Md5 => System.Security.Authentication.HashAlgorithmType.Md5, 
				Mono.Security.Protocol.Tls.HashAlgorithmType.None => System.Security.Authentication.HashAlgorithmType.None, 
				Mono.Security.Protocol.Tls.HashAlgorithmType.Sha1 => System.Security.Authentication.HashAlgorithmType.Sha1, 
				_ => throw new InvalidOperationException("Not supported hash algorithm is in use. It is likely a bug in SslStream."), 
			};
		}
	}

	public virtual int HashStrength
	{
		get
		{
			CheckConnectionAuthenticated();
			return ssl_stream.HashStrength;
		}
	}

	public virtual System.Security.Authentication.ExchangeAlgorithmType KeyExchangeAlgorithm
	{
		get
		{
			CheckConnectionAuthenticated();
			return ssl_stream.KeyExchangeAlgorithm switch
			{
				Mono.Security.Protocol.Tls.ExchangeAlgorithmType.DiffieHellman => System.Security.Authentication.ExchangeAlgorithmType.DiffieHellman, 
				Mono.Security.Protocol.Tls.ExchangeAlgorithmType.None => System.Security.Authentication.ExchangeAlgorithmType.None, 
				Mono.Security.Protocol.Tls.ExchangeAlgorithmType.RsaKeyX => System.Security.Authentication.ExchangeAlgorithmType.RsaKeyX, 
				Mono.Security.Protocol.Tls.ExchangeAlgorithmType.RsaSign => System.Security.Authentication.ExchangeAlgorithmType.RsaSign, 
				_ => throw new InvalidOperationException("Not supported exchange algorithm is in use. It is likely a bug in SslStream."), 
			};
		}
	}

	public virtual int KeyExchangeStrength
	{
		get
		{
			CheckConnectionAuthenticated();
			return ssl_stream.KeyExchangeStrength;
		}
	}

	System.Security.Cryptography.X509Certificates.X509Certificate IMonoSslStream.InternalLocalCertificate
	{
		get
		{
			if (!IsServer)
			{
				return ((SslClientStream)ssl_stream).SelectedClientCertificate;
			}
			return ssl_stream.ServerCertificate;
		}
	}

	public virtual System.Security.Cryptography.X509Certificates.X509Certificate LocalCertificate
	{
		get
		{
			CheckConnectionAuthenticated();
			if (!IsServer)
			{
				return ((SslClientStream)ssl_stream).SelectedClientCertificate;
			}
			return ssl_stream.ServerCertificate;
		}
	}

	public virtual System.Security.Cryptography.X509Certificates.X509Certificate RemoteCertificate
	{
		get
		{
			CheckConnectionAuthenticated();
			if (IsServer)
			{
				return ((SslServerStream)ssl_stream).ClientCertificate;
			}
			return ssl_stream.ServerCertificate;
		}
	}

	public virtual SslProtocols SslProtocol
	{
		get
		{
			CheckConnectionAuthenticated();
			return ssl_stream.SecurityProtocol switch
			{
				Mono.Security.Protocol.Tls.SecurityProtocolType.Default => SslProtocols.Default, 
				Mono.Security.Protocol.Tls.SecurityProtocolType.Ssl2 => SslProtocols.Ssl2, 
				Mono.Security.Protocol.Tls.SecurityProtocolType.Ssl3 => SslProtocols.Ssl3, 
				Mono.Security.Protocol.Tls.SecurityProtocolType.Tls => SslProtocols.Tls, 
				_ => throw new InvalidOperationException("Not supported SSL/TLS protocol is in use. It is likely a bug in SslStream."), 
			};
		}
	}

	AuthenticatedStream IMonoSslStream.AuthenticatedStream => this;

	TransportContext IMonoSslStream.TransportContext
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public SslStream SslStream { get; }

	public MonoTlsProvider Provider { get; }

	public LegacySslStream(Stream innerStream, bool leaveInnerStreamOpen, SslStream owner, MonoTlsProvider provider, MonoTlsSettings settings)
		: base(innerStream, leaveInnerStreamOpen)
	{
		SslStream = owner;
		Provider = provider;
		certificateValidator = ChainValidationHelper.GetInternalValidator(provider, settings);
	}

	private System.Security.Cryptography.X509Certificates.X509Certificate OnCertificateSelection(System.Security.Cryptography.X509Certificates.X509CertificateCollection clientCerts, System.Security.Cryptography.X509Certificates.X509Certificate serverCert, string targetHost, System.Security.Cryptography.X509Certificates.X509CertificateCollection serverRequestedCerts)
	{
		string[] array = new string[serverRequestedCerts?.Count ?? 0];
		for (int i = 0; i < array.Length; i++)
		{
			array[i] = serverRequestedCerts[i].GetIssuerName();
		}
		certificateValidator.SelectClientCertificate(targetHost, clientCerts, serverCert, array, out var clientCertificate);
		return clientCertificate;
	}

	public virtual IAsyncResult BeginAuthenticateAsClient(string targetHost, AsyncCallback asyncCallback, object asyncState)
	{
		return BeginAuthenticateAsClient(targetHost, new System.Security.Cryptography.X509Certificates.X509CertificateCollection(), SslProtocols.Tls, checkCertificateRevocation: false, asyncCallback, asyncState);
	}

	public virtual IAsyncResult BeginAuthenticateAsClient(string targetHost, System.Security.Cryptography.X509Certificates.X509CertificateCollection clientCertificates, SslProtocols enabledSslProtocols, bool checkCertificateRevocation, AsyncCallback asyncCallback, object asyncState)
	{
		if (IsAuthenticated)
		{
			throw new InvalidOperationException("This SslStream is already authenticated");
		}
		SslClientStream sslClientStream = new SslClientStream(base.InnerStream, targetHost, !base.LeaveInnerStreamOpen, GetMonoSslProtocol(enabledSslProtocols), clientCertificates);
		sslClientStream.CheckCertRevocationStatus = checkCertificateRevocation;
		sslClientStream.PrivateKeyCertSelectionDelegate = delegate(System.Security.Cryptography.X509Certificates.X509Certificate cert, string host)
		{
			string certHashString = cert.GetCertHashString();
			foreach (System.Security.Cryptography.X509Certificates.X509Certificate clientCertificate in clientCertificates)
			{
				if (!(clientCertificate.GetCertHashString() != certHashString))
				{
					return ((clientCertificate as X509Certificate2) ?? new X509Certificate2(clientCertificate)).PrivateKey;
				}
			}
			return (AsymmetricAlgorithm)null;
		};
		sslClientStream.ServerCertValidation2 += delegate(Mono.Security.X509.X509CertificateCollection mcerts)
		{
			System.Security.Cryptography.X509Certificates.X509CertificateCollection x509CertificateCollection = null;
			if (mcerts != null)
			{
				x509CertificateCollection = new System.Security.Cryptography.X509Certificates.X509CertificateCollection();
				for (int i = 0; i < mcerts.Count; i++)
				{
					x509CertificateCollection.Add(new X509Certificate2(mcerts[i].RawData));
				}
			}
			return ((ChainValidationHelper)certificateValidator).ValidateCertificate(targetHost, serverMode: false, x509CertificateCollection);
		};
		sslClientStream.ClientCertSelectionDelegate = OnCertificateSelection;
		ssl_stream = sslClientStream;
		return BeginWrite(new byte[0], 0, 0, asyncCallback, asyncState);
	}

	public override IAsyncResult BeginRead(byte[] buffer, int offset, int count, AsyncCallback asyncCallback, object asyncState)
	{
		CheckConnectionAuthenticated();
		return ssl_stream.BeginRead(buffer, offset, count, asyncCallback, asyncState);
	}

	public virtual IAsyncResult BeginAuthenticateAsServer(System.Security.Cryptography.X509Certificates.X509Certificate serverCertificate, AsyncCallback asyncCallback, object asyncState)
	{
		return BeginAuthenticateAsServer(serverCertificate, clientCertificateRequired: false, SslProtocols.Tls, checkCertificateRevocation: false, asyncCallback, asyncState);
	}

	public virtual IAsyncResult BeginAuthenticateAsServer(System.Security.Cryptography.X509Certificates.X509Certificate serverCertificate, bool clientCertificateRequired, SslProtocols enabledSslProtocols, bool checkCertificateRevocation, AsyncCallback asyncCallback, object asyncState)
	{
		if (IsAuthenticated)
		{
			throw new InvalidOperationException("This SslStream is already authenticated");
		}
		SslServerStream sslServerStream = new SslServerStream(base.InnerStream, serverCertificate, clientCertificateRequired: false, clientCertificateRequired, !base.LeaveInnerStreamOpen, GetMonoSslProtocol(enabledSslProtocols));
		sslServerStream.CheckCertRevocationStatus = checkCertificateRevocation;
		sslServerStream.PrivateKeyCertSelectionDelegate = (System.Security.Cryptography.X509Certificates.X509Certificate cert, string targetHost) => ((serverCertificate as X509Certificate2) ?? new X509Certificate2(serverCertificate))?.PrivateKey;
		sslServerStream.ClientCertValidationDelegate = delegate(System.Security.Cryptography.X509Certificates.X509Certificate cert, int[] certErrors)
		{
			MonoSslPolicyErrors errors = ((certErrors.Length != 0) ? MonoSslPolicyErrors.RemoteCertificateChainErrors : MonoSslPolicyErrors.None);
			return ((ChainValidationHelper)certificateValidator).ValidateClientCertificate(cert, errors);
		};
		ssl_stream = sslServerStream;
		return BeginWrite(new byte[0], 0, 0, asyncCallback, asyncState);
	}

	private Mono.Security.Protocol.Tls.SecurityProtocolType GetMonoSslProtocol(SslProtocols ms)
	{
		return ms switch
		{
			SslProtocols.Ssl2 => Mono.Security.Protocol.Tls.SecurityProtocolType.Ssl2, 
			SslProtocols.Ssl3 => Mono.Security.Protocol.Tls.SecurityProtocolType.Ssl3, 
			SslProtocols.Tls => Mono.Security.Protocol.Tls.SecurityProtocolType.Tls, 
			_ => Mono.Security.Protocol.Tls.SecurityProtocolType.Default, 
		};
	}

	public override IAsyncResult BeginWrite(byte[] buffer, int offset, int count, AsyncCallback asyncCallback, object asyncState)
	{
		CheckConnectionAuthenticated();
		return ssl_stream.BeginWrite(buffer, offset, count, asyncCallback, asyncState);
	}

	public virtual void AuthenticateAsClient(string targetHost)
	{
		AuthenticateAsClient(targetHost, new System.Security.Cryptography.X509Certificates.X509CertificateCollection(), SslProtocols.Tls, checkCertificateRevocation: false);
	}

	public virtual void AuthenticateAsClient(string targetHost, System.Security.Cryptography.X509Certificates.X509CertificateCollection clientCertificates, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		EndAuthenticateAsClient(BeginAuthenticateAsClient(targetHost, clientCertificates, enabledSslProtocols, checkCertificateRevocation, null, null));
	}

	public virtual void AuthenticateAsServer(System.Security.Cryptography.X509Certificates.X509Certificate serverCertificate)
	{
		AuthenticateAsServer(serverCertificate, clientCertificateRequired: false, SslProtocols.Tls, checkCertificateRevocation: false);
	}

	public virtual void AuthenticateAsServer(System.Security.Cryptography.X509Certificates.X509Certificate serverCertificate, bool clientCertificateRequired, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		EndAuthenticateAsServer(BeginAuthenticateAsServer(serverCertificate, clientCertificateRequired, enabledSslProtocols, checkCertificateRevocation, null, null));
	}

	protected override void Dispose(bool disposing)
	{
		if (disposing)
		{
			if (ssl_stream != null)
			{
				ssl_stream.Dispose();
			}
			ssl_stream = null;
		}
		base.Dispose(disposing);
	}

	public virtual void EndAuthenticateAsClient(IAsyncResult asyncResult)
	{
		CheckConnectionAuthenticated();
		if (CanRead)
		{
			ssl_stream.EndRead(asyncResult);
		}
		else
		{
			ssl_stream.EndWrite(asyncResult);
		}
	}

	public virtual void EndAuthenticateAsServer(IAsyncResult asyncResult)
	{
		CheckConnectionAuthenticated();
		if (CanRead)
		{
			ssl_stream.EndRead(asyncResult);
		}
		else
		{
			ssl_stream.EndWrite(asyncResult);
		}
	}

	public override int EndRead(IAsyncResult asyncResult)
	{
		CheckConnectionAuthenticated();
		return ssl_stream.EndRead(asyncResult);
	}

	public override void EndWrite(IAsyncResult asyncResult)
	{
		CheckConnectionAuthenticated();
		ssl_stream.EndWrite(asyncResult);
	}

	public override void Flush()
	{
		CheckConnectionAuthenticated();
		base.InnerStream.Flush();
	}

	public override int Read(byte[] buffer, int offset, int count)
	{
		return EndRead(BeginRead(buffer, offset, count, null, null));
	}

	public override long Seek(long offset, SeekOrigin origin)
	{
		throw new NotSupportedException("This stream does not support seek operations");
	}

	public override void SetLength(long value)
	{
		base.InnerStream.SetLength(value);
	}

	public override void Write(byte[] buffer, int offset, int count)
	{
		EndWrite(BeginWrite(buffer, offset, count, null, null));
	}

	public void Write(byte[] buffer)
	{
		Write(buffer, 0, buffer.Length);
	}

	private void CheckConnectionAuthenticated()
	{
		if (!IsAuthenticated)
		{
			throw new InvalidOperationException("This operation is invalid until it is successfully authenticated");
		}
	}

	public virtual Task AuthenticateAsClientAsync(string targetHost)
	{
		return Task.Factory.FromAsync(BeginAuthenticateAsClient, EndAuthenticateAsClient, targetHost, null);
	}

	public virtual Task AuthenticateAsClientAsync(string targetHost, System.Security.Cryptography.X509Certificates.X509CertificateCollection clientCertificates, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		Tuple<string, System.Security.Cryptography.X509Certificates.X509CertificateCollection, SslProtocols, bool, LegacySslStream> state2 = Tuple.Create(targetHost, clientCertificates, enabledSslProtocols, checkCertificateRevocation, this);
		return Task.Factory.FromAsync(delegate(AsyncCallback callback, object state)
		{
			Tuple<string, System.Security.Cryptography.X509Certificates.X509CertificateCollection, SslProtocols, bool, LegacySslStream> tuple = (Tuple<string, System.Security.Cryptography.X509Certificates.X509CertificateCollection, SslProtocols, bool, LegacySslStream>)state;
			return tuple.Item5.BeginAuthenticateAsClient(tuple.Item1, tuple.Item2, tuple.Item3, tuple.Item4, callback, null);
		}, EndAuthenticateAsClient, state2);
	}

	public virtual Task AuthenticateAsServerAsync(System.Security.Cryptography.X509Certificates.X509Certificate serverCertificate)
	{
		return Task.Factory.FromAsync(BeginAuthenticateAsServer, EndAuthenticateAsServer, serverCertificate, null);
	}

	public virtual Task AuthenticateAsServerAsync(System.Security.Cryptography.X509Certificates.X509Certificate serverCertificate, bool clientCertificateRequired, SslProtocols enabledSslProtocols, bool checkCertificateRevocation)
	{
		Tuple<System.Security.Cryptography.X509Certificates.X509Certificate, bool, SslProtocols, bool, LegacySslStream> state2 = Tuple.Create(serverCertificate, clientCertificateRequired, enabledSslProtocols, checkCertificateRevocation, this);
		return Task.Factory.FromAsync(delegate(AsyncCallback callback, object state)
		{
			Tuple<System.Security.Cryptography.X509Certificates.X509Certificate, bool, SslProtocols, bool, LegacySslStream> tuple = (Tuple<System.Security.Cryptography.X509Certificates.X509Certificate, bool, SslProtocols, bool, LegacySslStream>)state;
			return tuple.Item5.BeginAuthenticateAsServer(tuple.Item1, tuple.Item2, tuple.Item3, tuple.Item4, callback, null);
		}, EndAuthenticateAsServer, state2);
	}

	Task IMonoSslStream.ShutdownAsync()
	{
		return Task.CompletedTask;
	}

	public MonoTlsConnectionInfo GetConnectionInfo()
	{
		return null;
	}
}
