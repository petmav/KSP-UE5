using System.Collections.Specialized;
using System.Security.Permissions;
using System.Xml;
using Unity;

namespace System.Configuration;

[PermissionSet(SecurityAction.Demand, Name = "FullTrust")]
public sealed class DpapiProtectedConfigurationProvider : ProtectedConfigurationProvider
{
	public bool UseMachineProtection
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public DpapiProtectedConfigurationProvider()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override XmlNode Decrypt(XmlNode encryptedNode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override XmlNode Encrypt(XmlNode node)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void Initialize(string name, NameValueCollection configurationValues)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
