using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class ConnectionManagementElement : ConfigurationElement
{
	public string Address
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

	public int MaxConnection
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
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

	public ConnectionManagementElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConnectionManagementElement(string address, int maxConnection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
