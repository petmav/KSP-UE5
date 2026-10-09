using System;
using System.IO;
using System.Net.Security;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using Mono.Net.Security;
using Mono.Security.Interface;

namespace Mono.Unity;

internal class UnityTlsProvider : MonoTlsProvider
{
	public override string Name => "unitytls";

	public override Guid ID => Mono.Net.Security.MonoTlsProviderFactory.UnityTlsId;

	public override bool SupportsSslStream => true;

	public override bool SupportsMonoExtensions => true;

	public override bool SupportsConnectionInfo => true;

	internal override bool SupportsCleanShutdown => true;

	public override SslProtocols SupportedProtocols => SslProtocols.Tls | SslProtocols.Tls11 | SslProtocols.Tls12;

	public override IMonoSslStream CreateSslStream(Stream innerStream, bool leaveInnerStreamOpen, MonoTlsSettings settings = null)
	{
		return SslStream.CreateMonoSslStream(innerStream, leaveInnerStreamOpen, this, settings);
	}

	internal override IMonoSslStream CreateSslStreamInternal(SslStream sslStream, Stream innerStream, bool leaveInnerStreamOpen, MonoTlsSettings settings)
	{
		return new UnityTlsStream(innerStream, leaveInnerStreamOpen, sslStream, settings, this);
	}

	internal unsafe override bool ValidateCertificate(ICertificateValidator2 validator, string targetHost, bool serverMode, X509CertificateCollection certificates, bool wantsChain, ref X509Chain chain, ref MonoSslPolicyErrors errors, ref int status11)
	{
		if (certificates == null)
		{
			errors |= MonoSslPolicyErrors.RemoteCertificateNotAvailable;
			return false;
		}
		if (wantsChain)
		{
			chain = SystemCertificateValidator.CreateX509Chain(certificates);
		}
		if (certificates == null || certificates.Count == 0)
		{
			errors |= MonoSslPolicyErrors.RemoteCertificateNotAvailable;
			return false;
		}
		if (!string.IsNullOrEmpty(targetHost))
		{
			int num = targetHost.IndexOf(':');
			if (num > 0)
			{
				targetHost = targetHost.Substring(0, num);
			}
		}
		UnityTls.unitytls_errorstate unitytls_errorstate = UnityTls.NativeInterface.unitytls_errorstate_create();
		UnityTls.unitytls_x509list* ptr = UnityTls.NativeInterface.unitytls_x509list_create(&unitytls_errorstate);
		UnityTls.unitytls_x509verify_result unitytls_x509verify_result = UnityTls.unitytls_x509verify_result.UNITYTLS_X509VERIFY_NOT_DONE;
		try
		{
			CertHelper.AddCertificatesToNativeChain(ptr, certificates, &unitytls_errorstate);
			UnityTls.unitytls_x509list_ref chain2 = UnityTls.NativeInterface.unitytls_x509list_get_ref(ptr, &unitytls_errorstate);
			byte[] bytes = Encoding.UTF8.GetBytes(targetHost);
			if (validator.Settings.TrustAnchors != null)
			{
				UnityTls.unitytls_x509list* ptr2 = UnityTls.NativeInterface.unitytls_x509list_create(&unitytls_errorstate);
				CertHelper.AddCertificatesToNativeChain(ptr2, validator.Settings.TrustAnchors, &unitytls_errorstate);
				UnityTls.unitytls_x509list_ref trustCA = UnityTls.NativeInterface.unitytls_x509list_get_ref(ptr, &unitytls_errorstate);
				fixed (byte* cn = bytes)
				{
					unitytls_x509verify_result = UnityTls.NativeInterface.unitytls_x509verify_explicit_ca(chain2, trustCA, cn, (IntPtr)bytes.Length, null, null, &unitytls_errorstate);
				}
				UnityTls.NativeInterface.unitytls_x509list_free(ptr2);
			}
			else
			{
				fixed (byte* cn2 = bytes)
				{
					unitytls_x509verify_result = UnityTls.NativeInterface.unitytls_x509verify_default_ca(chain2, cn2, (IntPtr)bytes.Length, null, null, &unitytls_errorstate);
				}
			}
		}
		finally
		{
			UnityTls.NativeInterface.unitytls_x509list_free(ptr);
		}
		errors = UnityTlsConversions.VerifyResultToPolicyErrror(unitytls_x509verify_result);
		if (unitytls_x509verify_result == UnityTls.unitytls_x509verify_result.UNITYTLS_X509VERIFY_SUCCESS)
		{
			return unitytls_errorstate.code == UnityTls.unitytls_error_code.UNITYTLS_SUCCESS;
		}
		return false;
	}
}
