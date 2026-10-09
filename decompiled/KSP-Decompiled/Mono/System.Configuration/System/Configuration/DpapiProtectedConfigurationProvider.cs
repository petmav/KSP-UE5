using System.Collections.Specialized;
using System.Xml;

namespace System.Configuration;

public sealed class DpapiProtectedConfigurationProvider : ProtectedConfigurationProvider
{
	private bool useMachineProtection;

	private const string NotSupportedReason = "DpapiProtectedConfigurationProvider depends on the Microsoft Data\nProtection API, and is unimplemented in Mono.  For portability's sake,\nit is suggested that you use the RsaProtectedConfigurationProvider.";

	public bool UseMachineProtection => useMachineProtection;

	[System.MonoNotSupported("DpapiProtectedConfigurationProvider depends on the Microsoft Data\nProtection API, and is unimplemented in Mono.  For portability's sake,\nit is suggested that you use the RsaProtectedConfigurationProvider.")]
	public override XmlNode Decrypt(XmlNode encryptedNode)
	{
		throw new NotSupportedException("DpapiProtectedConfigurationProvider depends on the Microsoft Data\nProtection API, and is unimplemented in Mono.  For portability's sake,\nit is suggested that you use the RsaProtectedConfigurationProvider.");
	}

	[System.MonoNotSupported("DpapiProtectedConfigurationProvider depends on the Microsoft Data\nProtection API, and is unimplemented in Mono.  For portability's sake,\nit is suggested that you use the RsaProtectedConfigurationProvider.")]
	public override XmlNode Encrypt(XmlNode node)
	{
		throw new NotSupportedException("DpapiProtectedConfigurationProvider depends on the Microsoft Data\nProtection API, and is unimplemented in Mono.  For portability's sake,\nit is suggested that you use the RsaProtectedConfigurationProvider.");
	}

	[System.MonoTODO]
	public override void Initialize(string name, NameValueCollection configurationValues)
	{
		base.Initialize(name, configurationValues);
		string text = configurationValues["useMachineProtection"];
		if (text != null && text.ToLowerInvariant() == "true")
		{
			useMachineProtection = true;
		}
	}
}
