using System;
using System.Net;
using System.Net.Security;
using System.Security.Cryptography.X509Certificates;
using Mono.Net.Security.Private;
using Mono.Security.Interface;

namespace Mono.Net.Security;

internal class ChainValidationHelper : ICertificateValidator2, ICertificateValidator
{
	private readonly object sender;

	private readonly MonoTlsSettings settings;

	private readonly MonoTlsProvider provider;

	private readonly ServerCertValidationCallback certValidationCallback;

	private readonly LocalCertSelectionCallback certSelectionCallback;

	private readonly ServerCertValidationCallbackWrapper callbackWrapper;

	private readonly MonoTlsStream tlsStream;

	private readonly HttpWebRequest request;

	public MonoTlsProvider Provider => provider;

	public MonoTlsSettings Settings => settings;

	public bool HasCertificateSelectionCallback => certSelectionCallback != null;

	internal static ICertificateValidator GetInternalValidator(MonoTlsProvider provider, MonoTlsSettings settings)
	{
		if (settings == null)
		{
			return new ChainValidationHelper(provider, null, cloneSettings: false, null, null);
		}
		if (settings.CertificateValidator != null)
		{
			return settings.CertificateValidator;
		}
		return new ChainValidationHelper(provider, settings, cloneSettings: false, null, null);
	}

	internal static ICertificateValidator GetDefaultValidator(MonoTlsSettings settings)
	{
		MonoTlsProvider monoTlsProvider = MonoTlsProviderFactory.GetProvider();
		if (settings == null)
		{
			return new ChainValidationHelper(monoTlsProvider, null, cloneSettings: false, null, null);
		}
		if (settings.CertificateValidator != null)
		{
			throw new NotSupportedException();
		}
		return new ChainValidationHelper(monoTlsProvider, settings, cloneSettings: false, null, null);
	}

	internal static ChainValidationHelper CloneWithCallbackWrapper(MonoTlsProvider provider, ref MonoTlsSettings settings, ServerCertValidationCallbackWrapper wrapper)
	{
		ChainValidationHelper chainValidationHelper = (ChainValidationHelper)settings.CertificateValidator;
		chainValidationHelper = ((chainValidationHelper != null) ? new ChainValidationHelper(chainValidationHelper, provider, settings, wrapper) : new ChainValidationHelper(provider, settings, cloneSettings: true, null, wrapper));
		settings = chainValidationHelper.settings;
		return chainValidationHelper;
	}

	internal static bool InvokeCallback(ServerCertValidationCallback callback, object sender, X509Certificate certificate, X509Chain chain, MonoSslPolicyErrors sslPolicyErrors)
	{
		return callback.Invoke(sender, certificate, chain, (SslPolicyErrors)sslPolicyErrors);
	}

	private ChainValidationHelper(ChainValidationHelper other, MonoTlsProvider provider, MonoTlsSettings settings, ServerCertValidationCallbackWrapper callbackWrapper = null)
	{
		sender = other.sender;
		certValidationCallback = other.certValidationCallback;
		certSelectionCallback = other.certSelectionCallback;
		tlsStream = other.tlsStream;
		request = other.request;
		if (settings == null)
		{
			settings = MonoTlsSettings.DefaultSettings;
		}
		this.provider = provider;
		this.settings = settings.CloneWithValidator(this);
		this.callbackWrapper = callbackWrapper;
	}

	internal static ChainValidationHelper Create(MonoTlsProvider provider, ref MonoTlsSettings settings, MonoTlsStream stream)
	{
		ChainValidationHelper chainValidationHelper = new ChainValidationHelper(provider, settings, cloneSettings: true, stream, null);
		settings = chainValidationHelper.settings;
		return chainValidationHelper;
	}

	private ChainValidationHelper(MonoTlsProvider provider, MonoTlsSettings settings, bool cloneSettings, MonoTlsStream stream, ServerCertValidationCallbackWrapper callbackWrapper)
	{
		if (settings == null)
		{
			settings = MonoTlsSettings.CopyDefaultSettings();
		}
		if (cloneSettings)
		{
			settings = settings.CloneWithValidator(this);
		}
		if (provider == null)
		{
			provider = MonoTlsProviderFactory.GetProvider();
		}
		this.provider = provider;
		this.settings = settings;
		tlsStream = stream;
		this.callbackWrapper = callbackWrapper;
		bool flag = false;
		if (settings != null)
		{
			if (settings.RemoteCertificateValidationCallback != null)
			{
				RemoteCertificateValidationCallback validationCallback = CallbackHelpers.MonoToPublic(settings.RemoteCertificateValidationCallback);
				certValidationCallback = new ServerCertValidationCallback(validationCallback);
			}
			certSelectionCallback = CallbackHelpers.MonoToInternal(settings.ClientCertificateSelectionCallback);
			flag = settings.UseServicePointManagerCallback ?? (stream != null);
		}
		if (stream != null)
		{
			request = stream.Request;
			sender = request;
			if (certValidationCallback == null)
			{
				certValidationCallback = request.ServerCertValidationCallback;
			}
			if (certSelectionCallback == null)
			{
				certSelectionCallback = DefaultSelectionCallback;
			}
			if (settings == null)
			{
				flag = true;
			}
		}
		if (flag && certValidationCallback == null)
		{
			certValidationCallback = ServicePointManager.ServerCertValidationCallback;
		}
	}

	private static X509Certificate DefaultSelectionCallback(string targetHost, X509CertificateCollection localCertificates, X509Certificate remoteCertificate, string[] acceptableIssuers)
	{
		if (localCertificates == null || localCertificates.Count == 0)
		{
			return null;
		}
		return localCertificates[0];
	}

	public bool SelectClientCertificate(string targetHost, X509CertificateCollection localCertificates, X509Certificate remoteCertificate, string[] acceptableIssuers, out X509Certificate clientCertificate)
	{
		if (certSelectionCallback == null)
		{
			clientCertificate = null;
			return false;
		}
		clientCertificate = certSelectionCallback(targetHost, localCertificates, remoteCertificate, acceptableIssuers);
		return true;
	}

	internal X509Certificate SelectClientCertificate(string targetHost, X509CertificateCollection localCertificates, X509Certificate remoteCertificate, string[] acceptableIssuers)
	{
		if (certSelectionCallback == null)
		{
			return null;
		}
		return certSelectionCallback(targetHost, localCertificates, remoteCertificate, acceptableIssuers);
	}

	internal bool ValidateClientCertificate(X509Certificate certificate, MonoSslPolicyErrors errors)
	{
		X509CertificateCollection x509CertificateCollection = new X509CertificateCollection();
		x509CertificateCollection.Add(new X509Certificate2(certificate.GetRawCertData()));
		ValidationResult validationResult = ValidateChain(string.Empty, server: true, certificate, null, x509CertificateCollection, (SslPolicyErrors)errors);
		if (validationResult == null)
		{
			return false;
		}
		if (validationResult.Trusted)
		{
			return !validationResult.UserDenied;
		}
		return false;
	}

	public ValidationResult ValidateCertificate(string host, bool serverMode, X509CertificateCollection certs)
	{
		try
		{
			X509Certificate leaf = ((certs == null || certs.Count == 0) ? null : certs[0]);
			ValidationResult validationResult = ValidateChain(host, serverMode, leaf, null, certs, SslPolicyErrors.None);
			if (tlsStream != null)
			{
				tlsStream.CertificateValidationFailed = validationResult == null || !validationResult.Trusted || validationResult.UserDenied;
			}
			return validationResult;
		}
		catch
		{
			if (tlsStream != null)
			{
				tlsStream.CertificateValidationFailed = true;
			}
			throw;
		}
	}

	public ValidationResult ValidateCertificate(string host, bool serverMode, X509Certificate leaf, X509Chain chain)
	{
		try
		{
			ValidationResult validationResult = ValidateChain(host, serverMode, leaf, chain, null, SslPolicyErrors.None);
			if (tlsStream != null)
			{
				tlsStream.CertificateValidationFailed = validationResult == null || !validationResult.Trusted || validationResult.UserDenied;
			}
			return validationResult;
		}
		catch
		{
			if (tlsStream != null)
			{
				tlsStream.CertificateValidationFailed = true;
			}
			throw;
		}
	}

	private ValidationResult ValidateChain(string host, bool server, X509Certificate leaf, X509Chain chain, X509CertificateCollection certs, SslPolicyErrors errors)
	{
		X509Chain x509Chain = chain;
		bool flag = chain == null;
		try
		{
			ValidationResult result = ValidateChain(host, server, leaf, ref chain, certs, errors);
			if (chain != x509Chain)
			{
				flag = true;
			}
			return result;
		}
		finally
		{
			if (flag)
			{
				chain?.Dispose();
			}
		}
	}

	private ValidationResult ValidateChain(string host, bool server, X509Certificate leaf, ref X509Chain chain, X509CertificateCollection certs, SslPolicyErrors errors)
	{
		bool user_denied = false;
		bool flag = false;
		bool flag2 = certValidationCallback != null || callbackWrapper != null;
		if (tlsStream != null)
		{
			request.ServicePoint.UpdateServerCertificate(leaf);
		}
		if (leaf == null)
		{
			errors |= SslPolicyErrors.RemoteCertificateNotAvailable;
			if (flag2)
			{
				flag = ((callbackWrapper == null) ? certValidationCallback.Invoke(sender, leaf, null, errors) : callbackWrapper(certValidationCallback, leaf, null, (MonoSslPolicyErrors)errors));
				user_denied = !flag;
			}
			return new ValidationResult(flag, user_denied, 0, (MonoSslPolicyErrors)errors);
		}
		if (!string.IsNullOrEmpty(host))
		{
			int num = host.IndexOf(':');
			if (num > 0)
			{
				host = host.Substring(0, num);
			}
		}
		ICertificatePolicy legacyCertificatePolicy = ServicePointManager.GetLegacyCertificatePolicy();
		int status = 0;
		bool flag3 = SystemCertificateValidator.NeedsChain(settings);
		if (!flag3 && flag2 && (settings == null || settings.CallbackNeedsCertificateChain))
		{
			flag3 = true;
		}
		MonoSslPolicyErrors errors2 = (MonoSslPolicyErrors)errors;
		flag = provider.ValidateCertificate(this, host, server, certs, flag3, ref chain, ref errors2, ref status);
		errors = (SslPolicyErrors)errors2;
		if (status == 0 && errors != 0)
		{
			status = -2146762485;
		}
		if (legacyCertificatePolicy != null && (!(legacyCertificatePolicy is DefaultCertificatePolicy) || certValidationCallback == null))
		{
			ServicePoint srvPoint = null;
			if (request != null)
			{
				srvPoint = request.ServicePointNoLock;
			}
			flag = legacyCertificatePolicy.CheckValidationResult(srvPoint, leaf, request, status);
			user_denied = !flag && !(legacyCertificatePolicy is DefaultCertificatePolicy);
		}
		if (flag2)
		{
			flag = ((callbackWrapper == null) ? certValidationCallback.Invoke(sender, leaf, chain, errors) : callbackWrapper(certValidationCallback, leaf, chain, (MonoSslPolicyErrors)errors));
			user_denied = !flag;
		}
		return new ValidationResult(flag, user_denied, status, (MonoSslPolicyErrors)errors);
	}

	private bool InvokeSystemValidator(string targetHost, bool serverMode, X509CertificateCollection certificates, X509Chain chain, ref MonoSslPolicyErrors xerrors, ref int status11)
	{
		SslPolicyErrors errors = (SslPolicyErrors)xerrors;
		bool result = SystemCertificateValidator.Evaluate(settings, targetHost, certificates, chain, ref errors, ref status11);
		xerrors = (MonoSslPolicyErrors)errors;
		return result;
	}
}
