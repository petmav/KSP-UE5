using System.Security.Cryptography.X509Certificates;
using System.Security.Permissions;
using Unity;

namespace System.Security.Cryptography;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class ManifestSignatureInformation
{
	public AuthenticodeSignatureInformation AuthenticodeSignature
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ManifestKinds Manifest
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ManifestKinds);
		}
	}

	public StrongNameSignatureInformation StrongNameSignature
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal ManifestSignatureInformation()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static ManifestSignatureInformationCollection VerifySignature(ActivationContext application)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static ManifestSignatureInformationCollection VerifySignature(ActivationContext application, ManifestKinds manifests)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public static ManifestSignatureInformationCollection VerifySignature(ActivationContext application, ManifestKinds manifests, X509RevocationFlag revocationFlag, X509RevocationMode revocationMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
