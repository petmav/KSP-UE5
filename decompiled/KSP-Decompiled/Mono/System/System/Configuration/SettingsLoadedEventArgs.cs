namespace System.Configuration;

public class SettingsLoadedEventArgs : EventArgs
{
	private SettingsProvider provider;

	public SettingsProvider Provider => provider;

	public SettingsLoadedEventArgs(SettingsProvider provider)
	{
		this.provider = provider;
	}
}
