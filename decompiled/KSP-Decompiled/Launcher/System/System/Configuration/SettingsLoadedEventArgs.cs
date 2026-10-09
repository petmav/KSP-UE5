using Unity;

namespace System.Configuration;

public class SettingsLoadedEventArgs : EventArgs
{
	public SettingsProvider Provider
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SettingsLoadedEventArgs(SettingsProvider provider)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
