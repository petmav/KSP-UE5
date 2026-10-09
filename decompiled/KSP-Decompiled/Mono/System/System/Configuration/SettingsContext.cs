using System.Collections;

namespace System.Configuration;

[Serializable]
public class SettingsContext : Hashtable
{
	[NonSerialized]
	private ApplicationSettingsBase current;

	internal ApplicationSettingsBase CurrentSettings
	{
		get
		{
			return current;
		}
		set
		{
			current = value;
		}
	}
}
