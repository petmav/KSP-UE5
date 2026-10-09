using System.Collections;
using Unity;

namespace System.Configuration;

[Serializable]
public class SettingsAttributeDictionary : Hashtable
{
	public SettingsAttributeDictionary()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SettingsAttributeDictionary(SettingsAttributeDictionary attributes)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
