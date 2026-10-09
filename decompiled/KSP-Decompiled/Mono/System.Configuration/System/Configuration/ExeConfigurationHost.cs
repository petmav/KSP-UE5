using System.Configuration.Internal;

namespace System.Configuration;

internal class ExeConfigurationHost : InternalConfigurationHost
{
	private ExeConfigurationFileMap map;

	private ConfigurationUserLevel level;

	public override void Init(IInternalConfigRoot root, params object[] hostInitParams)
	{
		map = (ExeConfigurationFileMap)hostInitParams[0];
		level = (ConfigurationUserLevel)hostInitParams[1];
		CheckFileMap(level, map);
	}

	private static void CheckFileMap(ConfigurationUserLevel level, ExeConfigurationFileMap map)
	{
		if (level != 0)
		{
			switch (level)
			{
			default:
				return;
			case ConfigurationUserLevel.PerUserRoamingAndLocal:
				if (string.IsNullOrEmpty(map.LocalUserConfigFilename))
				{
					throw new ArgumentException("The 'LocalUserConfigFilename' argument cannot be null.");
				}
				break;
			case ConfigurationUserLevel.PerUserRoaming:
				break;
			}
			if (string.IsNullOrEmpty(map.RoamingUserConfigFilename))
			{
				throw new ArgumentException("The 'RoamingUserConfigFilename' argument cannot be null.");
			}
		}
		if (string.IsNullOrEmpty(map.ExeConfigFilename))
		{
			throw new ArgumentException("The 'ExeConfigFilename' argument cannot be null.");
		}
	}

	public override string GetStreamName(string configPath)
	{
		return configPath switch
		{
			"exe" => map.ExeConfigFilename, 
			"local" => map.LocalUserConfigFilename, 
			"roaming" => map.RoamingUserConfigFilename, 
			"machine" => map.MachineConfigFilename, 
			_ => level switch
			{
				ConfigurationUserLevel.None => map.ExeConfigFilename, 
				ConfigurationUserLevel.PerUserRoaming => map.RoamingUserConfigFilename, 
				ConfigurationUserLevel.PerUserRoamingAndLocal => map.LocalUserConfigFilename, 
				_ => map.MachineConfigFilename, 
			}, 
		};
	}

	public override void InitForConfiguration(ref string locationSubPath, out string configPath, out string locationConfigPath, IInternalConfigRoot root, params object[] hostInitConfigurationParams)
	{
		map = (ExeConfigurationFileMap)hostInitConfigurationParams[0];
		if (hostInitConfigurationParams.Length > 1 && hostInitConfigurationParams[1] is ConfigurationUserLevel)
		{
			level = (ConfigurationUserLevel)hostInitConfigurationParams[1];
		}
		CheckFileMap(level, map);
		if (locationSubPath == null)
		{
			switch (level)
			{
			case ConfigurationUserLevel.PerUserRoaming:
				if (map.RoamingUserConfigFilename == null)
				{
					throw new ArgumentException("RoamingUserConfigFilename must be set correctly");
				}
				locationSubPath = "roaming";
				break;
			case ConfigurationUserLevel.PerUserRoamingAndLocal:
				if (map.LocalUserConfigFilename == null)
				{
					throw new ArgumentException("LocalUserConfigFilename must be set correctly");
				}
				locationSubPath = "local";
				break;
			}
		}
		if (locationSubPath == "exe" || (locationSubPath == null && map.ExeConfigFilename != null))
		{
			configPath = "exe";
			locationSubPath = "machine";
			locationConfigPath = map.ExeConfigFilename;
			return;
		}
		if (locationSubPath == "local" && map.LocalUserConfigFilename != null)
		{
			configPath = "local";
			locationSubPath = "roaming";
			locationConfigPath = map.LocalUserConfigFilename;
			return;
		}
		if (locationSubPath == "roaming" && map.RoamingUserConfigFilename != null)
		{
			configPath = "roaming";
			locationSubPath = "exe";
			locationConfigPath = map.RoamingUserConfigFilename;
			return;
		}
		if (locationSubPath == "machine" && map.MachineConfigFilename != null)
		{
			configPath = "machine";
			locationSubPath = null;
			locationConfigPath = null;
			return;
		}
		throw new NotImplementedException();
	}
}
