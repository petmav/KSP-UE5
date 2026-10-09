using System.Collections.Specialized;
using System.IO;
using System.Security.Cryptography;
using System.Security.Cryptography.Xml;
using System.Xml;
using Unity;

namespace System.Configuration;

public sealed class RsaProtectedConfigurationProvider : ProtectedConfigurationProvider
{
	private string cspProviderName;

	private string keyContainerName;

	private bool useMachineContainer;

	private bool useOAEP;

	private RSACryptoServiceProvider rsa;

	public string CspProviderName => cspProviderName;

	public string KeyContainerName => keyContainerName;

	public RSAParameters RsaPublicKey => GetProvider().ExportParameters(includePrivateParameters: false);

	public bool UseMachineContainer => useMachineContainer;

	public bool UseOAEP => useOAEP;

	public bool UseFIPS
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	private RSACryptoServiceProvider GetProvider()
	{
		if (rsa == null)
		{
			CspParameters cspParameters = new CspParameters();
			cspParameters.ProviderName = cspProviderName;
			cspParameters.KeyContainerName = keyContainerName;
			if (useMachineContainer)
			{
				cspParameters.Flags |= CspProviderFlags.UseMachineKeyStore;
			}
			rsa = new RSACryptoServiceProvider(cspParameters);
		}
		return rsa;
	}

	[System.MonoTODO]
	public override XmlNode Decrypt(XmlNode encryptedNode)
	{
		ConfigurationXmlDocument configurationXmlDocument = new ConfigurationXmlDocument();
		configurationXmlDocument.Load(new StringReader(encryptedNode.OuterXml));
		EncryptedXml encryptedXml = new EncryptedXml(configurationXmlDocument);
		encryptedXml.AddKeyNameMapping("Rsa Key", GetProvider());
		encryptedXml.DecryptDocument();
		return configurationXmlDocument.DocumentElement;
	}

	[System.MonoTODO]
	public override XmlNode Encrypt(XmlNode node)
	{
		XmlDocument xmlDocument = new ConfigurationXmlDocument();
		xmlDocument.Load(new StringReader(node.OuterXml));
		EncryptedXml encryptedXml = new EncryptedXml(xmlDocument);
		encryptedXml.AddKeyNameMapping("Rsa Key", GetProvider());
		return encryptedXml.Encrypt(xmlDocument.DocumentElement, "Rsa Key").GetXml();
	}

	[System.MonoTODO]
	public override void Initialize(string name, NameValueCollection configurationValues)
	{
		base.Initialize(name, configurationValues);
		keyContainerName = configurationValues["keyContainerName"];
		cspProviderName = configurationValues["cspProviderName"];
		string text = configurationValues["useMachineContainer"];
		if (text != null && text.ToLower() == "true")
		{
			useMachineContainer = true;
		}
		text = configurationValues["useOAEP"];
		if (text != null && text.ToLower() == "true")
		{
			useOAEP = true;
		}
	}

	[System.MonoTODO]
	public void AddKey(int keySize, bool exportable)
	{
		throw new NotImplementedException();
	}

	[System.MonoTODO]
	public void DeleteKey()
	{
		throw new NotImplementedException();
	}

	[System.MonoTODO]
	public void ExportKey(string xmlFileName, bool includePrivateParameters)
	{
		string value = GetProvider().ToXmlString(includePrivateParameters);
		StreamWriter streamWriter = new StreamWriter(new FileStream(xmlFileName, FileMode.OpenOrCreate, FileAccess.Write));
		streamWriter.Write(value);
		streamWriter.Close();
	}

	[System.MonoTODO]
	public void ImportKey(string xmlFileName, bool exportable)
	{
		throw new NotImplementedException();
	}
}
