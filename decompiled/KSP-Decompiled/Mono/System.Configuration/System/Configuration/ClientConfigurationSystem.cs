using System.Configuration.Internal;
using System.Reflection;

namespace System.Configuration;

internal class ClientConfigurationSystem : IInternalConfigSystem
{
	private Configuration cfg;

	private Configuration Configuration
	{
		get
		{
			if (cfg == null)
			{
				Assembly entryAssembly = Assembly.GetEntryAssembly();
				try
				{
					cfg = ConfigurationManager.OpenExeConfigurationInternal(ConfigurationUserLevel.None, entryAssembly, null);
				}
				catch (Exception inner)
				{
					throw new ConfigurationErrorsException("Error Initializing the configuration system.", inner);
				}
			}
			return cfg;
		}
	}

	bool IInternalConfigSystem.SupportsUserConfig => false;

	object IInternalConfigSystem.GetSection(string configKey)
	{
		return Configuration.GetSection(configKey)?.GetRuntimeObject();
	}

	void IInternalConfigSystem.RefreshConfig(string sectionName)
	{
	}
}
