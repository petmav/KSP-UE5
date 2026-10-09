using System.Security.Permissions;
using Unity;

namespace System.Security.Cryptography;

[Serializable]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class ECDiffieHellmanCngPublicKey : ECDiffieHellmanPublicKey
{
	public CngKeyBlobFormat BlobFormat
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal ECDiffieHellmanCngPublicKey()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public static ECDiffieHellmanPublicKey FromByteArray(byte[] publicKeyBlob, CngKeyBlobFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public static ECDiffieHellmanCngPublicKey FromXmlString(string xml)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public CngKey Import()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
