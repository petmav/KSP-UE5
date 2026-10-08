using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class AuthenticationModulesSection : ConfigurationSection
{
	public AuthenticationModuleElementCollection AuthenticationModules
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
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

	public AuthenticationModulesSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void InitializeDefault()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void PostDeserialize()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
