using Unity;

namespace System.Configuration;

public sealed class ExeConfigurationFileMap : ConfigurationFileMap
{
	public string ExeConfigFilename
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

	public string LocalUserConfigFilename
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

	public string RoamingUserConfigFilename
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

	public ExeConfigurationFileMap()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ExeConfigurationFileMap(string machineConfigFileName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
