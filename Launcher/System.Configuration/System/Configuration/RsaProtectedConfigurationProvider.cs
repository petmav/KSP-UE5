using System.Collections.Specialized;
using System.Security.Cryptography;
using System.Security.Permissions;
using System.Xml;
using Unity;

namespace System.Configuration;

[PermissionSet(SecurityAction.Demand, Name = "FullTrust")]
public sealed class RsaProtectedConfigurationProvider : ProtectedConfigurationProvider
{
	public string CspProviderName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string KeyContainerName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public RSAParameters RsaPublicKey
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(RSAParameters);
		}
	}

	public bool UseFIPS
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool UseMachineContainer
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool UseOAEP
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public RsaProtectedConfigurationProvider()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddKey(int keySize, bool exportable)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override XmlNode Decrypt(XmlNode encryptedNode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void DeleteKey()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override XmlNode Encrypt(XmlNode node)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void ExportKey(string xmlFileName, bool includePrivateParameters)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ImportKey(string xmlFileName, bool exportable)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void Initialize(string name, NameValueCollection configurationValues)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
