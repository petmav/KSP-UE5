using Unity;

namespace System.Configuration;

public sealed class ExeConfigurationFileMap : ConfigurationFileMap
{
	private string exeConfigFilename;

	private string localUserConfigFilename;

	private string roamingUserConfigFilename;

	public string ExeConfigFilename
	{
		get
		{
			return exeConfigFilename;
		}
		set
		{
			exeConfigFilename = value;
		}
	}

	public string LocalUserConfigFilename
	{
		get
		{
			return localUserConfigFilename;
		}
		set
		{
			localUserConfigFilename = value;
		}
	}

	public string RoamingUserConfigFilename
	{
		get
		{
			return roamingUserConfigFilename;
		}
		set
		{
			roamingUserConfigFilename = value;
		}
	}

	public ExeConfigurationFileMap()
	{
		exeConfigFilename = "";
		localUserConfigFilename = "";
		roamingUserConfigFilename = "";
	}

	public override object Clone()
	{
		return new ExeConfigurationFileMap
		{
			exeConfigFilename = exeConfigFilename,
			localUserConfigFilename = localUserConfigFilename,
			roamingUserConfigFilename = roamingUserConfigFilename,
			MachineConfigFilename = base.MachineConfigFilename
		};
	}

	public ExeConfigurationFileMap(string machineConfigFileName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
