using System.Configuration;
using Unity;

namespace System.Security.Authentication.ExtendedProtection.Configuration;

public sealed class ServiceNameElement : ConfigurationElement
{
	public string Name
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

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ServiceNameElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
