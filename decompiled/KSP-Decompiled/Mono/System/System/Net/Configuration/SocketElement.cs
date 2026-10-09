using System.Configuration;
using System.Net.Sockets;
using Unity;

namespace System.Net.Configuration;

public sealed class SocketElement : ConfigurationElement
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty alwaysUseCompletionPortsForAcceptProp;

	private static ConfigurationProperty alwaysUseCompletionPortsForConnectProp;

	[ConfigurationProperty("alwaysUseCompletionPortsForAccept", DefaultValue = "False")]
	public bool AlwaysUseCompletionPortsForAccept
	{
		get
		{
			return (bool)base[alwaysUseCompletionPortsForAcceptProp];
		}
		set
		{
			base[alwaysUseCompletionPortsForAcceptProp] = value;
		}
	}

	[ConfigurationProperty("alwaysUseCompletionPortsForConnect", DefaultValue = "False")]
	public bool AlwaysUseCompletionPortsForConnect
	{
		get
		{
			return (bool)base[alwaysUseCompletionPortsForConnectProp];
		}
		set
		{
			base[alwaysUseCompletionPortsForConnectProp] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	public IPProtectionLevel IPProtectionLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(IPProtectionLevel);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public SocketElement()
	{
		alwaysUseCompletionPortsForAcceptProp = new ConfigurationProperty("alwaysUseCompletionPortsForAccept", typeof(bool), false);
		alwaysUseCompletionPortsForConnectProp = new ConfigurationProperty("alwaysUseCompletionPortsForConnect", typeof(bool), false);
		properties = new ConfigurationPropertyCollection();
		properties.Add(alwaysUseCompletionPortsForAcceptProp);
		properties.Add(alwaysUseCompletionPortsForConnectProp);
	}

	[System.MonoTODO]
	protected override void PostDeserialize()
	{
	}
}
