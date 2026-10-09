using System;
using System.IO;
using System.Net.Security;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using Mono.Net.Security.Private;
using Mono.Security.Interface;

namespace Mono.Net.Security;

internal class LegacyTlsProvider : MonoTlsProvider
{
	public override Guid ID => MonoTlsProviderFactory.LegacyId;

	public override string Name => "legacy";

	public override bool SupportsSslStream => true;

	public override bool SupportsConnectionInfo => false;

	public override bool SupportsMonoExtensions => false;

	internal override bool SupportsCleanShutdown => false;

	public override SslProtocols SupportedProtocols => SslProtocols.Tls;

	public override IMonoSslStream CreateSslStream(Stream innerStream, bool leaveInnerStreamOpen, MonoTlsSettings settings = null)
	{
		return SslStream.CreateMonoSslStream(innerStream, leaveInnerStreamOpen, this, settings);
	}

	internal override IMonoSslStream CreateSslStreamInternal(SslStream sslStream, Stream innerStream, bool leaveInnerStreamOpen, MonoTlsSettings settings)
	{
		return new LegacySslStream(innerStream, leaveInnerStreamOpen, sslStream, this, settings);
	}

	internal override bool ValidateCertificate(ICertificateValidator2 validator, string targetHost, bool serverMode, X509CertificateCollection certificates, bool wantsChain, ref X509Chain chain, ref MonoSslPolicyErrors errors, ref int status11)
	{
		if (wantsChain)
		{
			chain = SystemCertificateValidator.CreateX509Chain(certificates);
		}
		SslPolicyErrors errors2 = (SslPolicyErrors)errors;
		bool result = SystemCertificateValidator.Evaluate(validator.Settings, targetHost, certificates, chain, ref errors2, ref status11);
		errors = (MonoSslPolicyErrors)errors2;
		return result;
	}
}
