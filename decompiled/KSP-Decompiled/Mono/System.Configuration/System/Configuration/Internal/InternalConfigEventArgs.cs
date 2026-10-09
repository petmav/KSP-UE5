namespace System.Configuration.Internal;

public sealed class InternalConfigEventArgs : EventArgs
{
	private string configPath;

	public string ConfigPath
	{
		get
		{
			return configPath;
		}
		set
		{
			configPath = value;
		}
	}

	public InternalConfigEventArgs(string configPath)
	{
		this.configPath = configPath;
	}
}
