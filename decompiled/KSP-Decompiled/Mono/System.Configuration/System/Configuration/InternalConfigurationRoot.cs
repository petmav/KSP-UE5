using System.Configuration.Internal;

namespace System.Configuration;

internal class InternalConfigurationRoot : IInternalConfigRoot
{
	private IInternalConfigHost host;

	private bool isDesignTime;

	public bool IsDesignTime => isDesignTime;

	public event InternalConfigEventHandler ConfigChanged;

	public event InternalConfigEventHandler ConfigRemoved;

	[System.MonoTODO]
	public IInternalConfigRecord GetConfigRecord(string configPath)
	{
		throw new NotImplementedException();
	}

	public object GetSection(string section, string configPath)
	{
		return GetConfigRecord(configPath).GetSection(section);
	}

	[System.MonoTODO]
	public string GetUniqueConfigPath(string configPath)
	{
		return configPath;
	}

	[System.MonoTODO]
	public IInternalConfigRecord GetUniqueConfigRecord(string configPath)
	{
		return GetConfigRecord(GetUniqueConfigPath(configPath));
	}

	public void Init(IInternalConfigHost host, bool isDesignTime)
	{
		this.host = host;
		this.isDesignTime = isDesignTime;
	}

	[System.MonoTODO]
	public void RemoveConfig(string configPath)
	{
		host.DeleteStream(configPath);
		if (this.ConfigRemoved != null)
		{
			this.ConfigRemoved(this, new InternalConfigEventArgs(configPath));
		}
	}
}
