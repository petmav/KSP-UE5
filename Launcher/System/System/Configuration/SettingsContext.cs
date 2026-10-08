using System.Collections;
using Unity;

namespace System.Configuration;

[Serializable]
public class SettingsContext : Hashtable
{
	public SettingsContext()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
