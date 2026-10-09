using System.Configuration;

namespace System.Security.Authentication.ExtendedProtection.Configuration;

[System.MonoTODO]
public sealed class ExtendedProtectionPolicyElement : ConfigurationElement
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty custom_service_names;

	private static ConfigurationProperty policy_enforcement;

	private static ConfigurationProperty protection_scenario;

	[ConfigurationProperty("customServiceNames")]
	public ServiceNameElementCollection CustomServiceNames => (ServiceNameElementCollection)base[custom_service_names];

	[ConfigurationProperty("policyEnforcement")]
	public PolicyEnforcement PolicyEnforcement
	{
		get
		{
			return (PolicyEnforcement)base[policy_enforcement];
		}
		set
		{
			base[policy_enforcement] = value;
		}
	}

	[ConfigurationProperty("protectionScenario", DefaultValue = ProtectionScenario.TransportSelected)]
	public ProtectionScenario ProtectionScenario
	{
		get
		{
			return (ProtectionScenario)base[protection_scenario];
		}
		set
		{
			base[protection_scenario] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static ExtendedProtectionPolicyElement()
	{
		properties = new ConfigurationPropertyCollection();
		Type typeFromHandle = typeof(ExtendedProtectionPolicyElement);
		custom_service_names = ConfigUtil.BuildProperty(typeFromHandle, "CustomServiceNames");
		policy_enforcement = ConfigUtil.BuildProperty(typeFromHandle, "PolicyEnforcement");
		protection_scenario = ConfigUtil.BuildProperty(typeFromHandle, "ProtectionScenario");
		ConfigurationProperty[] array = new ConfigurationProperty[3] { custom_service_names, policy_enforcement, protection_scenario };
		foreach (ConfigurationProperty property in array)
		{
			properties.Add(property);
		}
	}

	public ExtendedProtectionPolicy BuildPolicy()
	{
		throw new NotImplementedException();
	}
}
