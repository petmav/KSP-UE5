using System.Collections;
using Unity;

namespace System.Configuration;

public class ConfigurationLocationCollection : ReadOnlyCollectionBase
{
	public ConfigurationLocation this[int index]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal ConfigurationLocationCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
