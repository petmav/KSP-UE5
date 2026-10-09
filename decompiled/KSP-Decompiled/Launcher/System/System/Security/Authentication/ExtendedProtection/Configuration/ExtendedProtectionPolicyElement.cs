using System.Configuration;
using Unity;

namespace System.Security.Authentication.ExtendedProtection.Configuration;

public sealed class ExtendedProtectionPolicyElement : ConfigurationElement
{
	public ServiceNameElementCollection CustomServiceNames
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public PolicyEnforcement PolicyEnforcement
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PolicyEnforcement);
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

	public ProtectionScenario ProtectionScenario
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ProtectionScenario);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ExtendedProtectionPolicyElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ExtendedProtectionPolicy BuildPolicy()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
