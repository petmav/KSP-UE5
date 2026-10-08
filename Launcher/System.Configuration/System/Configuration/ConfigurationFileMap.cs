using Unity;

namespace System.Configuration;

public class ConfigurationFileMap : ICloneable
{
	public string MachineConfigFilename
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

	public ConfigurationFileMap()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationFileMap(string machineConfigFilename)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
