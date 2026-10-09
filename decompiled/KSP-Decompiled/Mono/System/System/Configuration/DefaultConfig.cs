using System.Runtime.CompilerServices;

namespace System.Configuration;

internal class DefaultConfig : IConfigurationSystem
{
	private static readonly DefaultConfig instance = new DefaultConfig();

	private ConfigurationData config;

	private DefaultConfig()
	{
	}

	public static DefaultConfig GetInstance()
	{
		return instance;
	}

	[Obsolete("This method is obsolete.  Please use System.Configuration.ConfigurationManager.GetConfig")]
	public object GetConfig(string sectionName)
	{
		Init();
		return config.GetConfig(sectionName);
	}

	public void Init()
	{
		lock (this)
		{
			if (config != null)
			{
				return;
			}
			ConfigurationData configurationData = new ConfigurationData();
			if (!configurationData.LoadString(GetBundledMachineConfig()) && !configurationData.Load(GetMachineConfigPath()))
			{
				throw new ConfigurationException("Cannot find " + GetMachineConfigPath());
			}
			string appConfigPath = GetAppConfigPath();
			if (appConfigPath == null)
			{
				config = configurationData;
				return;
			}
			ConfigurationData configurationData2 = new ConfigurationData(configurationData);
			if (configurationData2.Load(appConfigPath))
			{
				config = configurationData2;
			}
			else
			{
				config = configurationData;
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern string get_bundled_machine_config();

	internal static string GetBundledMachineConfig()
	{
		return get_bundled_machine_config();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern string get_machine_config_path();

	internal static string GetMachineConfigPath()
	{
		return get_machine_config_path();
	}

	private static string GetAppConfigPath()
	{
		string configurationFile = AppDomain.CurrentDomain.SetupInformation.ConfigurationFile;
		if (configurationFile == null || configurationFile.Length == 0)
		{
			return null;
		}
		return configurationFile;
	}
}
