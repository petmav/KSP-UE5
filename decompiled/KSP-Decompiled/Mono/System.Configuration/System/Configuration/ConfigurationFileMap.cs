using System.Runtime.InteropServices;

namespace System.Configuration;

public class ConfigurationFileMap : ICloneable
{
	private string machineConfigFilename;

	public string MachineConfigFilename
	{
		get
		{
			return machineConfigFilename;
		}
		set
		{
			machineConfigFilename = value;
		}
	}

	public ConfigurationFileMap()
	{
		machineConfigFilename = RuntimeEnvironment.SystemConfigurationFile;
	}

	public ConfigurationFileMap(string machineConfigFilename)
	{
		this.machineConfigFilename = machineConfigFilename;
	}

	public virtual object Clone()
	{
		return new ConfigurationFileMap(machineConfigFilename);
	}
}
