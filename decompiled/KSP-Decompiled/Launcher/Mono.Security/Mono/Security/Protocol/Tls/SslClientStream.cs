using System;
using System.IO;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using Mono.Security.Interface;
using Mono.Security.Protocol.Tls.Handshake;
using Mono.Security.X509;

namespace Mono.Security.Protocol.Tls;

public class SslClientStream : SslStreamBase
{
	private enum NegotiateState
	{
		SentClientHello,
		ReceiveClientHelloResponse,
		SentCipherSpec,
		ReceiveCipherSpecResponse,
		SentKeyExchange,
		ReceiveFinishResponse,
		SentFinished
	}

	private class NegotiateAsyncResult : IAsyncResult
	{
		private object locker = new object();

		private AsyncCallback _userCallback;

		private object _userState;

		private Exception _asyncException;

		private ManualResetEvent handle;

		private NegotiateState _state;

		private bool completed;

		public NegotiateState State
		{
			get
			{
				return _state;
			}
			set
			{
				_state = value;
			}
		}

		public object AsyncState => _userState;

		public Exception AsyncException => _asyncException;

		public bool CompletedWithError
		{
			get
			{
				if (!IsCompleted)
				{
					return false;
				}
				return _asyncException != null;
			}
		}

		public WaitHandle AsyncWaitHandle
		{
			get
			{
				lock (locker)
				{
					if (handle == null)
					{
						handle = new ManualResetEvent(completed);
					}
				}
				return handle;
			}
		}

		public bool CompletedSynchronously => false;

		public bool IsCompleted
		{
			get
			{
				lock (locker)
				{
					return completed;
				}
			}
		}

		public NegotiateAsyncResult(AsyncCallback userCallback, object userState, NegotiateState state)
		{
			_userCallback = userCallback;
			_userState = userState;
			_state = state;
		}

		public void SetComplete(Exception ex)
		{
			lock (locker)
			{
				if (!completed)
				{
					completed = true;
					if (handle != null)
					{
						handle.Set();
					}
					if (_userCallback != null)
					{
						_userCallback.BeginInvoke(this, null, null);
					}
					_asyncException = ex;
				}
			}
		}

		public void SetComplete()
		{
			SetComplete(null);
		}
	}

	internal Stream InputBuffer => inputBuffer;

	public System.Security.Cryptography.X509Certificates.X509CertificateCollection ClientCertificates => context.ClientSettings.Certificates;

	public System.Security.Cryptography.X509Certificates.X509Certificate SelectedClientCertificate => context.ClientSettings.ClientCertificate;

	public CertificateValidationCallback ServerCertValidationDelegate
	{
		get
		{
			return this.ServerCertValidation;
		}
		set
		{
			this.ServerCertValidation = value;
		}
	}

	public CertificateSelectionCallback ClientCertSelectionDelegate
	{
		get
		{
			return this.ClientCertSelection;
		}
		set
		{
			this.ClientCertSelection = value;
		}
	}

	public PrivateKeySelectionCallback PrivateKeyCertSelectionDelegate
	{
		get
		{
			return this.PrivateKeySelection;
		}
		set
		{
			this.PrivateKeySelection = value;
		}
	}

	internal override bool HaveRemoteValidation2Callback => this.ServerCertValidation2 != null;

	internal event CertificateValidationCallback ServerCertValidation;

	internal event CertificateSelectionCallback ClientCertSelection;

	internal event PrivateKeySelectionCallback PrivateKeySelection;

	public event CertificateValidationCallback2 ServerCertValidation2;

	public SslClientStream(Stream stream, string targetHost, bool ownsStream)
		: this(stream, targetHost, ownsStream, SecurityProtocolType.Default, null)
	{
	}

	public SslClientStream(Stream stream, string targetHost, System.Security.Cryptography.X509Certificates.X509Certificate clientCertificate)
		: this(stream, targetHost, ownsStream: false, SecurityProtocolType.Default, new System.Security.Cryptography.X509Certificates.X509CertificateCollection(new System.Security.Cryptography.X509Certificates.X509Certificate[1] { clientCertificate }))
	{
	}

	public SslClientStream(Stream stream, string targetHost, System.Security.Cryptography.X509Certificates.X509CertificateCollection clientCertificates)
		: this(stream, targetHost, ownsStream: false, SecurityProtocolType.Default, clientCertificates)
	{
	}

	public SslClientStream(Stream stream, string targetHost, bool ownsStream, SecurityProtocolType securityProtocolType)
		: this(stream, targetHost, ownsStream, securityProtocolType, new System.Security.Cryptography.X509Certificates.X509CertificateCollection())
	{
	}

	public SslClientStream(Stream stream, string targetHost, bool ownsStream, SecurityProtocolType securityProtocolType, System.Security.Cryptography.X509Certificates.X509CertificateCollection clientCertificates)
		: base(stream, ownsStream)
	{
		if (targetHost == null || targetHost.Length == 0)
		{
			throw new ArgumentNullException("targetHost is null or an empty string.");
		}
		context = new ClientContext(this, securityProtocolType, targetHost, clientCertificates);
		protocol = new ClientRecordProtocol(innerStream, (ClientContext)context);
	}

	~SslClientStream()
	{
		base.Dispose(disposing: false);
	}

	protected override void Dispose(bool disposing)
	{
		base.Dispose(disposing);
		if (disposing)
		{
			this.ServerCertValidation = null;
			this.ClientCertSelection = null;
			this.PrivateKeySelection = null;
			this.ServerCertValidation2 = null;
		}
	}

	private void SafeEndReceiveRecord(IAsyncResult ar, bool ignoreEmpty = false)
	{
		byte[] array = protocol.EndReceiveRecord(ar);
		if (!ignoreEmpty && (array == null || array.Length == 0))
		{
			throw new TlsException(AlertDescription.HandshakeFailiure, "The server stopped the handshake.");
		}
	}

	internal override IAsyncResult BeginNegotiateHandshake(AsyncCallback callback, object state)
	{
		if (context.HandshakeState != 0)
		{
			context.Clear();
		}
		context.SupportedCiphers = CipherSuiteFactory.GetSupportedCiphers(server: false, context.SecurityProtocol);
		context.HandshakeState = HandshakeState.Started;
		NegotiateAsyncResult negotiateAsyncResult = new NegotiateAsyncResult(callback, state, NegotiateState.SentClientHello);
		protocol.BeginSendRecord(HandshakeType.ClientHello, NegotiateAsyncWorker, negotiateAsyncResult);
		return negotiateAsyncResult;
	}

	internal override void EndNegotiateHandshake(IAsyncResult result)
	{
		if (!(result is NegotiateAsyncResult negotiateAsyncResult))
		{
			throw new ArgumentNullException();
		}
		if (!negotiateAsyncResult.IsCompleted)
		{
			negotiateAsyncResult.AsyncWaitHandle.WaitOne();
		}
		if (negotiateAsyncResult.CompletedWithError)
		{
			throw negotiateAsyncResult.AsyncException;
		}
	}

	private void NegotiateAsyncWorker(IAsyncResult result)
	{
		NegotiateAsyncResult negotiateAsyncResult = result.AsyncState as NegotiateAsyncResult;
		try
		{
			switch (negotiateAsyncResult.State)
			{
			case NegotiateState.SentClientHello:
				protocol.EndSendRecord(result);
				negotiateAsyncResult.State = NegotiateState.ReceiveClientHelloResponse;
				protocol.BeginReceiveRecord(innerStream, NegotiateAsyncWorker, negotiateAsyncResult);
				break;
			case NegotiateState.ReceiveClientHelloResponse:
			{
				SafeEndReceiveRecord(result, ignoreEmpty: true);
				if (context.LastHandshakeMsg != HandshakeType.ServerHelloDone && (!context.AbbreviatedHandshake || context.LastHandshakeMsg != HandshakeType.ServerHello))
				{
					protocol.BeginReceiveRecord(innerStream, NegotiateAsyncWorker, negotiateAsyncResult);
					break;
				}
				if (context.AbbreviatedHandshake)
				{
					ClientSessionCache.SetContextFromCache(context);
					context.Negotiating.Cipher.ComputeKeys();
					context.Negotiating.Cipher.InitializeCipher();
					negotiateAsyncResult.State = NegotiateState.SentCipherSpec;
					protocol.BeginSendChangeCipherSpec(NegotiateAsyncWorker, negotiateAsyncResult);
					break;
				}
				bool flag = context.ServerSettings.CertificateRequest;
				using MemoryStream memoryStream = new MemoryStream();
				if (context.SecurityProtocol == SecurityProtocolType.Ssl3)
				{
					flag = context.ClientSettings.Certificates != null && context.ClientSettings.Certificates.Count > 0;
				}
				byte[] array = null;
				if (flag)
				{
					array = protocol.EncodeHandshakeRecord(HandshakeType.Certificate);
					memoryStream.Write(array, 0, array.Length);
				}
				array = protocol.EncodeHandshakeRecord(HandshakeType.ClientKeyExchange);
				memoryStream.Write(array, 0, array.Length);
				context.Negotiating.Cipher.InitializeCipher();
				if (flag && context.ClientSettings.ClientCertificate != null)
				{
					array = protocol.EncodeHandshakeRecord(HandshakeType.CertificateVerify);
					memoryStream.Write(array, 0, array.Length);
				}
				protocol.SendChangeCipherSpec(memoryStream);
				array = protocol.EncodeHandshakeRecord(HandshakeType.Finished);
				memoryStream.Write(array, 0, array.Length);
				negotiateAsyncResult.State = NegotiateState.SentKeyExchange;
				innerStream.BeginWrite(memoryStream.GetBuffer(), 0, (int)memoryStream.Length, NegotiateAsyncWorker, negotiateAsyncResult);
				break;
			}
			case NegotiateState.SentKeyExchange:
				innerStream.EndWrite(result);
				negotiateAsyncResult.State = NegotiateState.ReceiveFinishResponse;
				protocol.BeginReceiveRecord(innerStream, NegotiateAsyncWorker, negotiateAsyncResult);
				break;
			case NegotiateState.ReceiveFinishResponse:
				SafeEndReceiveRecord(result);
				if (context.HandshakeState != HandshakeState.Finished)
				{
					protocol.BeginReceiveRecord(innerStream, NegotiateAsyncWorker, negotiateAsyncResult);
					break;
				}
				context.HandshakeMessages.Reset();
				context.ClearKeyInfo();
				negotiateAsyncResult.SetComplete();
				break;
			case NegotiateState.SentCipherSpec:
				protocol.EndSendChangeCipherSpec(result);
				negotiateAsyncResult.State = NegotiateState.ReceiveCipherSpecResponse;
				protocol.BeginReceiveRecord(innerStream, NegotiateAsyncWorker, negotiateAsyncResult);
				break;
			case NegotiateState.ReceiveCipherSpecResponse:
				SafeEndReceiveRecord(result, ignoreEmpty: true);
				if (context.HandshakeState != HandshakeState.Finished)
				{
					protocol.BeginReceiveRecord(innerStream, NegotiateAsyncWorker, negotiateAsyncResult);
					break;
				}
				negotiateAsyncResult.State = NegotiateState.SentFinished;
				protocol.BeginSendRecord(HandshakeType.Finished, NegotiateAsyncWorker, negotiateAsyncResult);
				break;
			case NegotiateState.SentFinished:
				protocol.EndSendRecord(result);
				context.HandshakeMessages.Reset();
				context.ClearKeyInfo();
				negotiateAsyncResult.SetComplete();
				break;
			}
		}
		catch (TlsException ex)
		{
			try
			{
				Exception ex2 = ex;
				protocol.SendAlert(ref ex2);
			}
			catch
			{
			}
			negotiateAsyncResult.SetComplete(new IOException("The authentication or decryption has failed.", ex));
		}
		catch (Exception innerException)
		{
			try
			{
				protocol.SendAlert(AlertDescription.InternalError);
			}
			catch
			{
			}
			negotiateAsyncResult.SetComplete(new IOException("The authentication or decryption has failed.", innerException));
		}
	}

	internal override System.Security.Cryptography.X509Certificates.X509Certificate OnLocalCertificateSelection(System.Security.Cryptography.X509Certificates.X509CertificateCollection clientCertificates, System.Security.Cryptography.X509Certificates.X509Certificate serverCertificate, string targetHost, System.Security.Cryptography.X509Certificates.X509CertificateCollection serverRequestedCertificates)
	{
		if (this.ClientCertSelection != null)
		{
			return this.ClientCertSelection(clientCertificates, serverCertificate, targetHost, serverRequestedCertificates);
		}
		return null;
	}

	internal override ValidationResult OnRemoteCertificateValidation2(Mono.Security.X509.X509CertificateCollection collection)
	{
		return this.ServerCertValidation2?.Invoke(collection);
	}

	internal override bool OnRemoteCertificateValidation(System.Security.Cryptography.X509Certificates.X509Certificate certificate, int[] errors)
	{
		if (this.ServerCertValidation != null)
		{
			return this.ServerCertValidation(certificate, errors);
		}
		if (errors != null)
		{
			return errors.Length == 0;
		}
		return false;
	}

	internal virtual bool RaiseServerCertificateValidation(System.Security.Cryptography.X509Certificates.X509Certificate certificate, int[] certificateErrors)
	{
		return RaiseRemoteCertificateValidation(certificate, certificateErrors);
	}

	internal virtual ValidationResult RaiseServerCertificateValidation2(Mono.Security.X509.X509CertificateCollection collection)
	{
		return RaiseRemoteCertificateValidation2(collection);
	}

	internal System.Security.Cryptography.X509Certificates.X509Certificate RaiseClientCertificateSelection(System.Security.Cryptography.X509Certificates.X509CertificateCollection clientCertificates, System.Security.Cryptography.X509Certificates.X509Certificate serverCertificate, string targetHost, System.Security.Cryptography.X509Certificates.X509CertificateCollection serverRequestedCertificates)
	{
		return RaiseLocalCertificateSelection(clientCertificates, serverCertificate, targetHost, serverRequestedCertificates);
	}

	internal override AsymmetricAlgorithm OnLocalPrivateKeySelection(System.Security.Cryptography.X509Certificates.X509Certificate certificate, string targetHost)
	{
		if (this.PrivateKeySelection != null)
		{
			return this.PrivateKeySelection(certificate, targetHost);
		}
		return null;
	}

	internal AsymmetricAlgorithm RaisePrivateKeySelection(System.Security.Cryptography.X509Certificates.X509Certificate certificate, string targetHost)
	{
		return RaiseLocalPrivateKeySelection(certificate, targetHost);
	}
}
