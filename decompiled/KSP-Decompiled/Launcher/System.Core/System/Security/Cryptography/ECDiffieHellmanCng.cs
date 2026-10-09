using System.Security.Permissions;
using Microsoft.Win32.SafeHandles;
using Unity;

namespace System.Security.Cryptography;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class ECDiffieHellmanCng : ECDiffieHellman
{
	public CngAlgorithm HashAlgorithm
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public byte[] HmacKey
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CngKey Key
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ECDiffieHellmanKeyDerivationFunction KeyDerivationFunction
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ECDiffieHellmanKeyDerivationFunction);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public byte[] Label
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public override ECDiffieHellmanPublicKey PublicKey
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public byte[] SecretAppend
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public byte[] SecretPrepend
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public byte[] Seed
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool UseSecretAgreementAsHmacKey
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public ECDiffieHellmanCng()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ECDiffieHellmanCng(int keySize)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public ECDiffieHellmanCng(CngKey key)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ECDiffieHellmanCng(ECCurve curve)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public byte[] DeriveKeyMaterial(CngKey otherPartyPublicKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecurityCritical]
	[SecurityPermission(SecurityAction.Demand, UnmanagedCode = true)]
	public SafeNCryptSecretHandle DeriveSecretAgreementHandle(CngKey otherPartyPublicKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public SafeNCryptSecretHandle DeriveSecretAgreementHandle(ECDiffieHellmanPublicKey otherPartyPublicKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void FromXmlString(string xml, ECKeyXmlFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public string ToXmlString(ECKeyXmlFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
