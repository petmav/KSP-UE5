using System.Configuration;
using System.Net.Sockets;
using Unity;

namespace System.Net.Configuration;

public sealed class SocketElement : ConfigurationElement
{
	public bool AlwaysUseCompletionPortsForAccept
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool AlwaysUseCompletionPortsForConnect
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

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

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SocketElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void PostDeserialize()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
